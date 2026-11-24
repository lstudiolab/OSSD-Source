/*
 * Copyright (c) 1998-2021 Apple Inc. All rights reserved.
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
 *	@(#)uipc_socket.c	8.3 (Berkeley) 4/15/94
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/file_internal.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/kernel.h>
#include <sys/event.h>
#include <sys/poll.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/uio.h>
#include <sys/uio_internal.h>
#include <sys/ev.h>
#include <sys/kdebug.h>
#include <sys/un.h>
#include <sys/user.h>
#include <sys/priv.h>
#include <sys/kern_event.h>
#include <net/route.h>
#include <net/init.h>
#include <net/net_api_stats.h>
#include <net/ntstat.h>
#include <net/content_filter.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_tclass.h>
#include <netinet/in_var.h>
#include <netinet/tcp_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/flow_divert.h>
#include <kern/zalloc.h>
#include <kern/locks.h>
#include <machine/limits.h>
#include <libkern/OSAtomic.h>
#include <pexpert/pexpert.h>
#include <kern/assert.h>
#include <kern/task.h>
#include <kern/policy_internal.h>

#include <sys/kpi_mbuf.h>
#include <sys/mcache.h>
#include <sys/unpcb.h>
#include <libkern/section_keywords.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif          

#if MULTIPATH
#include <netinet/mp_pcb.h>
#include <netinet/mptcp_var.h>
#endif                

#define ROUNDUP(a, b) (((a) + ((b) - 1)) & (~((b) - 1)))

#if DEBUG || DEVELOPMENT
#define DEBUG_KERNEL_ADDRPERM(_v) (_v)
#else
#define DEBUG_KERNEL_ADDRPERM(_v) VM_KERNEL_ADDRPERM(_v)
#endif

                                                     
extern char *proc_name_address(void *p);

static u_int32_t        so_cache_hw;                                     
static u_int32_t        so_cache_timeouts;                              
static u_int32_t        so_cache_max_freed;                                
static u_int32_t        cached_sock_count = 0;
STAILQ_HEAD(, socket)   so_cache_head;
int     max_cached_sock_count = MAX_CACHED_SOCKETS;
static u_int32_t        so_cache_time;
static int              socketinit_done;
static struct zone      *so_cache_zone;

static LCK_GRP_DECLARE(so_cache_mtx_grp, "so_cache");
static LCK_MTX_DECLARE(so_cache_mtx, &so_cache_mtx_grp);

#include <machine/limits.h>

static int      filt_sorattach(struct knote *kn, struct kevent_qos_s *kev);
static void     filt_sordetach(struct knote *kn);
static int      filt_soread(struct knote *kn, long hint);
static int      filt_sortouch(struct knote *kn, struct kevent_qos_s *kev);
static int      filt_sorprocess(struct knote *kn, struct kevent_qos_s *kev);

static int      filt_sowattach(struct knote *kn, struct kevent_qos_s *kev);
static void     filt_sowdetach(struct knote *kn);
static int      filt_sowrite(struct knote *kn, long hint);
static int      filt_sowtouch(struct knote *kn, struct kevent_qos_s *kev);
static int      filt_sowprocess(struct knote *kn, struct kevent_qos_s *kev);

static int      filt_sockattach(struct knote *kn, struct kevent_qos_s *kev);
static void     filt_sockdetach(struct knote *kn);
static int      filt_sockev(struct knote *kn, long hint);
static int      filt_socktouch(struct knote *kn, struct kevent_qos_s *kev);
static int      filt_sockprocess(struct knote *kn, struct kevent_qos_s *kev);

static int sooptcopyin_timeval(struct sockopt *, struct timeval *);
static int sooptcopyout_timeval(struct sockopt *, const struct timeval *);

SECURITY_READ_ONLY_EARLY(struct filterops) soread_filtops = {
	.f_isfd = 1,
	.f_attach = filt_sorattach,
	.f_detach = filt_sordetach,
	.f_event = filt_soread,
	.f_touch = filt_sortouch,
	.f_process = filt_sorprocess,
};

SECURITY_READ_ONLY_EARLY(struct filterops) sowrite_filtops = {
	.f_isfd = 1,
	.f_attach = filt_sowattach,
	.f_detach = filt_sowdetach,
	.f_event = filt_sowrite,
	.f_touch = filt_sowtouch,
	.f_process = filt_sowprocess,
};

SECURITY_READ_ONLY_EARLY(struct filterops) sock_filtops = {
	.f_isfd = 1,
	.f_attach = filt_sockattach,
	.f_detach = filt_sockdetach,
	.f_event = filt_sockev,
	.f_touch = filt_socktouch,
	.f_process = filt_sockprocess,
};

SECURITY_READ_ONLY_EARLY(struct filterops) soexcept_filtops = {
	.f_isfd = 1,
	.f_attach = filt_sorattach,
	.f_detach = filt_sordetach,
	.f_event = filt_soread,
	.f_touch = filt_sortouch,
	.f_process = filt_sorprocess,
};

SYSCTL_DECL(_kern_ipc);

#define EVEN_MORE_LOCKING_DEBUG 0

int socket_debug = 0;
SYSCTL_INT(_kern_ipc, OID_AUTO, socket_debug,
    CTLFLAG_RW | CTLFLAG_LOCKED, &socket_debug, 0, "");

static unsigned long sodefunct_calls = 0;
SYSCTL_LONG(_kern_ipc, OID_AUTO, sodefunct_calls, CTLFLAG_LOCKED,
    &sodefunct_calls, "");

ZONE_DECLARE(socket_zone, "socket", sizeof(struct socket), ZC_ZFREE_CLEARMEM);
so_gen_t        so_gencnt;                                        

MALLOC_DEFINE(M_SONAME, "soname", "socket name");
MALLOC_DEFINE(M_PCB, "pcb", "protocol control block");

#define DBG_LAYER_IN_BEG        NETDBG_CODE(DBG_NETSOCK, 0)
#define DBG_LAYER_IN_END        NETDBG_CODE(DBG_NETSOCK, 2)
#define DBG_LAYER_OUT_BEG       NETDBG_CODE(DBG_NETSOCK, 1)
#define DBG_LAYER_OUT_END       NETDBG_CODE(DBG_NETSOCK, 3)
#define DBG_FNC_SOSEND          NETDBG_CODE(DBG_NETSOCK, (4 << 8) | 1)
#define DBG_FNC_SOSEND_LIST     NETDBG_CODE(DBG_NETSOCK, (4 << 8) | 3)
#define DBG_FNC_SORECEIVE       NETDBG_CODE(DBG_NETSOCK, (8 << 8))
#define DBG_FNC_SORECEIVE_LIST  NETDBG_CODE(DBG_NETSOCK, (8 << 8) | 3)
#define DBG_FNC_SOSHUTDOWN      NETDBG_CODE(DBG_NETSOCK, (9 << 8))

#define MAX_SOOPTGETM_SIZE      (128 * MCLBYTES)

int somaxconn = SOMAXCONN;
SYSCTL_INT(_kern_ipc, KIPC_SOMAXCONN, somaxconn,
    CTLFLAG_RW | CTLFLAG_LOCKED, &somaxconn, 0, "");

                                      
static int sosendmaxchain = 65536;
static int sosendminchain = 16384;
static int sorecvmincopy  = 16384;
SYSCTL_INT(_kern_ipc, OID_AUTO, sosendminchain,
    CTLFLAG_RW | CTLFLAG_LOCKED, &sosendminchain, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, sorecvmincopy,
    CTLFLAG_RW | CTLFLAG_LOCKED, &sorecvmincopy, 0, "");

  
                                                                    
                                                       
   
int sosendjcl = 1;
SYSCTL_INT(_kern_ipc, OID_AUTO, sosendjcl,
    CTLFLAG_RW | CTLFLAG_LOCKED, &sosendjcl, 0, "");

  
                                                                     
                                                                    
                                                                      
                                                                       
                                                                    
                                                                         
                                                                      
                                                                    
                                                               
   
int sosendjcl_ignore_capab = 0;
SYSCTL_INT(_kern_ipc, OID_AUTO, sosendjcl_ignore_capab,
    CTLFLAG_RW | CTLFLAG_LOCKED, &sosendjcl_ignore_capab, 0, "");

  
                                                                 
                                                                    
                                                                             
                                                                            
                                                                    
                            
                                                      
   
int sosendbigcl_ignore_capab = 0;
SYSCTL_INT(_kern_ipc, OID_AUTO, sosendbigcl_ignore_capab,
    CTLFLAG_RW | CTLFLAG_LOCKED, &sosendbigcl_ignore_capab, 0, "");

int sodefunctlog = 0;
SYSCTL_INT(_kern_ipc, OID_AUTO, sodefunctlog, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sodefunctlog, 0, "");

int sothrottlelog = 0;
SYSCTL_INT(_kern_ipc, OID_AUTO, sothrottlelog, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sothrottlelog, 0, "");

int sorestrictrecv = 1;
SYSCTL_INT(_kern_ipc, OID_AUTO, sorestrictrecv, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sorestrictrecv, 0, "Enable inbound interface restrictions");

int sorestrictsend = 1;
SYSCTL_INT(_kern_ipc, OID_AUTO, sorestrictsend, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sorestrictsend, 0, "Enable outbound interface restrictions");

int soreserveheadroom = 1;
SYSCTL_INT(_kern_ipc, OID_AUTO, soreserveheadroom, CTLFLAG_RW | CTLFLAG_LOCKED,
    &soreserveheadroom, 0, "To allocate contiguous datagram buffers");

#if (DEBUG || DEVELOPMENT)
int so_notsent_lowat_check = 1;
SYSCTL_INT(_kern_ipc, OID_AUTO, notsent_lowat, CTLFLAG_RW | CTLFLAG_LOCKED,
    &so_notsent_lowat_check, 0, "enable/disable notsnet lowat check");
#endif                           

int so_accept_list_waits = 0;
#if (DEBUG || DEVELOPMENT)
SYSCTL_INT(_kern_ipc, OID_AUTO, accept_list_waits, CTLFLAG_RW | CTLFLAG_LOCKED,
    &so_accept_list_waits, 0, "number of waits for listener incomp list");
#endif                           

extern struct inpcbinfo tcbinfo;

                                          
extern int get_inpcb_str_size(void);
extern int get_tcp_str_size(void);

vm_size_t       so_cache_zone_element_size;

static int sodelayed_copy(struct socket *, struct uio *, struct mbuf **,
    user_ssize_t *);
static void cached_sock_alloc(struct socket **, zalloc_flags_t);
static void cached_sock_free(struct socket *);

  
                                                          
                                                       
   

#define SO_IDLE_BK_IDLE_MAX_PER_PROC    1
#define SO_IDLE_BK_IDLE_TIME            600
#define SO_IDLE_BK_IDLE_RCV_HIWAT       131072

struct soextbkidlestat soextbkidlestat;

SYSCTL_UINT(_kern_ipc, OID_AUTO, maxextbkidleperproc,
    CTLFLAG_RW | CTLFLAG_LOCKED, &soextbkidlestat.so_xbkidle_maxperproc, 0,
    "Maximum of extended background idle sockets per process");

SYSCTL_UINT(_kern_ipc, OID_AUTO, extbkidletime, CTLFLAG_RW | CTLFLAG_LOCKED,
    &soextbkidlestat.so_xbkidle_time, 0,
    "Time in seconds to keep extended background idle sockets");

SYSCTL_UINT(_kern_ipc, OID_AUTO, extbkidlercvhiwat, CTLFLAG_RW | CTLFLAG_LOCKED,
    &soextbkidlestat.so_xbkidle_rcvhiwat, 0,
    "High water mark for extended background idle sockets");

SYSCTL_STRUCT(_kern_ipc, OID_AUTO, extbkidlestat, CTLFLAG_RD | CTLFLAG_LOCKED,
    &soextbkidlestat, soextbkidlestat, "");

int so_set_extended_bk_idle(struct socket *, int);


  
                                                                         
                                                                      
                                         
   
__private_extern__ u_int32_t sotcdb = 0;
SYSCTL_INT(_kern_ipc, OID_AUTO, sotcdb, CTLFLAG_RW | CTLFLAG_LOCKED,
    &sotcdb, 0, "");

void
socketinit(void)
{
	_CASSERT(sizeof(so_gencnt) == sizeof(uint64_t));
	VERIFY(IS_P2ALIGNED(&so_gencnt, sizeof(uint32_t)));

#ifdef __LP64__
	_CASSERT(sizeof(struct sa_endpoints) == sizeof(struct user64_sa_endpoints));
	_CASSERT(offsetof(struct sa_endpoints, sae_srcif) == offsetof(struct user64_sa_endpoints, sae_srcif));
	_CASSERT(offsetof(struct sa_endpoints, sae_srcaddr) == offsetof(struct user64_sa_endpoints, sae_srcaddr));
	_CASSERT(offsetof(struct sa_endpoints, sae_srcaddrlen) == offsetof(struct user64_sa_endpoints, sae_srcaddrlen));
	_CASSERT(offsetof(struct sa_endpoints, sae_dstaddr) == offsetof(struct user64_sa_endpoints, sae_dstaddr));
	_CASSERT(offsetof(struct sa_endpoints, sae_dstaddrlen) == offsetof(struct user64_sa_endpoints, sae_dstaddrlen));
#else
	_CASSERT(sizeof(struct sa_endpoints) == sizeof(struct user32_sa_endpoints));
	_CASSERT(offsetof(struct sa_endpoints, sae_srcif) == offsetof(struct user32_sa_endpoints, sae_srcif));
	_CASSERT(offsetof(struct sa_endpoints, sae_srcaddr) == offsetof(struct user32_sa_endpoints, sae_srcaddr));
	_CASSERT(offsetof(struct sa_endpoints, sae_srcaddrlen) == offsetof(struct user32_sa_endpoints, sae_srcaddrlen));
	_CASSERT(offsetof(struct sa_endpoints, sae_dstaddr) == offsetof(struct user32_sa_endpoints, sae_dstaddr));
	_CASSERT(offsetof(struct sa_endpoints, sae_dstaddrlen) == offsetof(struct user32_sa_endpoints, sae_dstaddrlen));
#endif

	if (socketinit_done) {
		printf("socketinit: already called...\n");
		return;
	}
	socketinit_done = 1;

	PE_parse_boot_argn("socket_debug", &socket_debug,
	    sizeof(socket_debug));

	STAILQ_INIT(&so_cache_head);

	so_cache_zone_element_size = (vm_size_t)(sizeof(struct socket) + 4
	    + get_inpcb_str_size() + 4 + get_tcp_str_size());

	so_cache_zone = zone_create("socache zone", so_cache_zone_element_size,
	    ZC_ZFREE_CLEARMEM | ZC_NOENCRYPT);

	bzero(&soextbkidlestat, sizeof(struct soextbkidlestat));
	soextbkidlestat.so_xbkidle_maxperproc = SO_IDLE_BK_IDLE_MAX_PER_PROC;
	soextbkidlestat.so_xbkidle_time = SO_IDLE_BK_IDLE_TIME;
	soextbkidlestat.so_xbkidle_rcvhiwat = SO_IDLE_BK_IDLE_RCV_HIWAT;

	in_pcbinit();
	socket_tclass_init();
#if MULTIPATH
	mp_pcbinit();
#endif                
}

static void
cached_sock_alloc(struct socket **so, zalloc_flags_t how)
{
	caddr_t temp;
	uintptr_t offset;

	lck_mtx_lock(&so_cache_mtx);

	if (!STAILQ_EMPTY(&so_cache_head)) {
		VERIFY(cached_sock_count > 0);

		*so = STAILQ_FIRST(&so_cache_head);
		STAILQ_REMOVE_HEAD(&so_cache_head, so_cache_ent);
		STAILQ_NEXT((*so), so_cache_ent) = NULL;

		cached_sock_count--;
		lck_mtx_unlock(&so_cache_mtx);

		temp = (*so)->so_saved_pcb;
		bzero((caddr_t)*so, sizeof(struct socket));

		(*so)->so_saved_pcb = temp;
	} else {
		lck_mtx_unlock(&so_cache_mtx);

		*so = zalloc_flags(so_cache_zone, how | Z_ZERO);

		  
                                                 
                                                   
                            
     

		offset = (uintptr_t)*so;
		offset += sizeof(struct socket);

		offset = ALIGN(offset);

		(*so)->so_saved_pcb = (caddr_t)offset;
		offset += get_inpcb_str_size();

		offset = ALIGN(offset);

		((struct inpcb *)(void *)(*so)->so_saved_pcb)->inp_saved_ppcb =
		    (caddr_t)offset;
	}

	OSBitOrAtomic(SOF1_CACHED_IN_SOCK_LAYER, &(*so)->so_flags1);
}

static void
cached_sock_free(struct socket *so)
{
	lck_mtx_lock(&so_cache_mtx);

	so_cache_time = net_uptime();
	if (++cached_sock_count > max_cached_sock_count) {
		--cached_sock_count;
		lck_mtx_unlock(&so_cache_mtx);
		zfree(so_cache_zone, so);
	} else {
		if (so_cache_hw < cached_sock_count) {
			so_cache_hw = cached_sock_count;
		}

		STAILQ_INSERT_TAIL(&so_cache_head, so, so_cache_ent);

		so->cache_timestamp = so_cache_time;
		lck_mtx_unlock(&so_cache_mtx);
	}
}

void
so_update_last_owner_locked(struct socket *so, proc_t self)
{
	if (so->last_pid != 0) {
		  
                                                          
                                                             
     
		if (self == PROC_NULL) {
			self = current_proc();
		}

		if (so->last_upid != proc_uniqueid(self) ||
		    so->last_pid != proc_pid(self)) {
			so->last_upid = proc_uniqueid(self);
			so->last_pid = proc_pid(self);
			proc_getexecutableuuid(self, so->last_uuid,
			    sizeof(so->last_uuid));
			if (so->so_proto != NULL && so->so_proto->pr_update_last_owner != NULL) {
				(*so->so_proto->pr_update_last_owner)(so, self, NULL);
			}
		}
		proc_pidoriginatoruuid(so->so_vuuid, sizeof(so->so_vuuid));
	}
}

void
so_update_policy(struct socket *so)
{
	if (SOCK_DOM(so) == PF_INET || SOCK_DOM(so) == PF_INET6) {
		(void) inp_update_policy(sotoinpcb(so));
	}
}

#if NECP
static void
so_update_necp_policy(struct socket *so, struct sockaddr *override_local_addr,
    struct sockaddr *override_remote_addr)
{
	if (SOCK_DOM(so) == PF_INET || SOCK_DOM(so) == PF_INET6) {
		inp_update_necp_policy(sotoinpcb(so), override_local_addr,
		    override_remote_addr, 0);
	}
}
#endif           

boolean_t
so_cache_timer(void)
{
	struct socket   *p;
	int             n_freed = 0;
	boolean_t rc = FALSE;

	lck_mtx_lock(&so_cache_mtx);
	so_cache_timeouts++;
	so_cache_time = net_uptime();

	while (!STAILQ_EMPTY(&so_cache_head)) {
		VERIFY(cached_sock_count > 0);
		p = STAILQ_FIRST(&so_cache_head);
		if ((so_cache_time - p->cache_timestamp) <
		    SO_CACHE_TIME_LIMIT) {
			break;
		}

		STAILQ_REMOVE_HEAD(&so_cache_head, so_cache_ent);
		--cached_sock_count;

		zfree(so_cache_zone, p);

		if (++n_freed >= SO_CACHE_MAX_FREE_BATCH) {
			so_cache_max_freed++;
			break;
		}
	}

	                                                
	if (!STAILQ_EMPTY(&so_cache_head)) {
		rc = TRUE;
	}

	lck_mtx_unlock(&so_cache_mtx);
	return rc;
}

  
                                                           
                                                                   
                                                           
                                                           
                                                   
   
struct socket *
soalloc(int waitok, int dom, int type)
{
	zalloc_flags_t how = waitok ? Z_WAITOK : Z_NOWAIT;
	struct socket *so;

	if ((dom == PF_INET) && (type == SOCK_STREAM)) {
		cached_sock_alloc(&so, how);
	} else {
		so = zalloc_flags(socket_zone, how | Z_ZERO);
	}
	if (so != NULL) {
		so->so_gencnt = OSIncrementAtomic64((SInt64 *)&so_gencnt);

		  
                                               
     
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_alloc_total);
	}

	return so;
}

int
socreate_internal(int dom, struct socket **aso, int type, int proto,
    struct proc *p, uint32_t flags, struct proc *ep)
{
	struct protosw *prp;
	struct socket *so;
	int error = 0;
#if defined(XNU_TARGET_OS_OSX)
	pid_t rpid = -1;
#endif

#if TCPDEBUG
	extern int tcpconsdebug;
#endif

	VERIFY(aso != NULL);
	*aso = NULL;

	if (proto != 0) {
		prp = pffindproto(dom, proto, type);
	} else {
		prp = pffindtype(dom, type);
	}

	if (prp == NULL || prp->pr_usrreqs->pru_attach == NULL) {
		if (pffinddomain(dom) == NULL) {
			return EAFNOSUPPORT;
		}
		if (proto != 0) {
			if (pffindprotonotype(dom, proto) != NULL) {
				return EPROTOTYPE;
			}
		}
		return EPROTONOSUPPORT;
	}
	if (prp->pr_type != type) {
		return EPROTOTYPE;
	}
	so = soalloc(1, dom, type);
	if (so == NULL) {
		return ENOBUFS;
	}

	switch (dom) {
	case PF_LOCAL:
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_domain_local_total);
		break;
	case PF_INET:
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_domain_inet_total);
		if (type == SOCK_STREAM) {
			INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_inet_stream_total);
		} else {
			INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_inet_dgram_total);
		}
		break;
	case PF_ROUTE:
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_domain_route_total);
		break;
	case PF_NDRV:
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_domain_ndrv_total);
		break;
	case PF_KEY:
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_domain_key_total);
		break;
	case PF_INET6:
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_domain_inet6_total);
		if (type == SOCK_STREAM) {
			INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_inet6_stream_total);
		} else {
			INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_inet6_dgram_total);
		}
		break;
	case PF_SYSTEM:
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_domain_system_total);
		break;
	case PF_MULTIPATH:
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_domain_multipath_total);
		break;
	default:
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_domain_other_total);
		break;
	}

	if (flags & SOCF_MPTCP) {
		so->so_state |= SS_NBIO;
	}

	TAILQ_INIT(&so->so_incomp);
	TAILQ_INIT(&so->so_comp);
	so->so_type = type;
	so->last_upid = proc_uniqueid(p);
	so->last_pid = proc_pid(p);
	proc_getexecutableuuid(p, so->last_uuid, sizeof(so->last_uuid));
	proc_pidoriginatoruuid(so->so_vuuid, sizeof(so->so_vuuid));

	if (ep != PROC_NULL && ep != p) {
		so->e_upid = proc_uniqueid(ep);
		so->e_pid = proc_pid(ep);
		proc_getexecutableuuid(ep, so->e_uuid, sizeof(so->e_uuid));
		so->so_flags |= SOF_DELEGATED;
#if defined(XNU_TARGET_OS_OSX)
		if (ep->p_responsible_pid != so->e_pid) {
			rpid = ep->p_responsible_pid;
		}
#endif
	}

#if defined(XNU_TARGET_OS_OSX)
	if (rpid < 0 && p->p_responsible_pid != so->last_pid) {
		rpid = p->p_responsible_pid;
	}

	so->so_rpid = -1;
	uuid_clear(so->so_ruuid);
	if (rpid >= 0) {
		proc_t rp = proc_find(rpid);
		if (rp != PROC_NULL) {
			proc_getexecutableuuid(rp, so->so_ruuid, sizeof(so->so_ruuid));
			so->so_rpid = rpid;
			proc_rele(rp);
		}
	}
#endif

	so->so_cred = kauth_cred_proc_ref(p);
	if (!suser(kauth_cred_get(), NULL)) {
		so->so_state |= SS_PRIV;
	}

	so->so_proto = prp;
	so->so_rcv.sb_flags |= SB_RECV;
	so->so_rcv.sb_so = so->so_snd.sb_so = so;
	so->next_lock_lr = 0;
	so->next_unlock_lr = 0;

	  
                                                            
                                                              
                                
    
	so->so_usecount++;

	error = (*prp->pr_usrreqs->pru_attach)(so, proto, p);
	if (error != 0) {
		  
             
                                                      
                                                          
     
		so->so_state |= SS_NOFDREF;
		VERIFY(so->so_usecount > 0);
		so->so_usecount--;
		sofreelastref(so, 1);                                   
		return error;
	}

	  
                                                 
    
	if (prp->pr_update_last_owner != NULL) {
		(*prp->pr_update_last_owner)(so, p, ep);
	}

	atomic_add_32(&prp->pr_domain->dom_refs, 1);

	                                             
	sflt_initsock(so);
#if TCPDEBUG
	if (tcpconsdebug == 2) {
		so->so_options |= SO_DEBUG;
	}
#endif
	so_set_default_traffic_class(so);

	  
                                                                    
                                  
    
	if (!(flags & SOCF_MPTCP) &&
	    proc_get_effective_thread_policy(current_thread(), TASK_POLICY_NEW_SOCKETS_BG)) {
		socket_set_traffic_mgt_flags(so, TRAFFIC_MGT_SO_BACKGROUND);
		so->so_background_thread = current_thread();
	}

	switch (dom) {
	  
                                    
                                    
    
	case PF_LOCAL:
	case PF_SYSTEM:
		so->so_flags |= SOF_NODEFUNCT;
		break;
	default:
		break;
	}

	  
                                                                       
                                                                        
                
                                                                       
                                                                     
                                                                   
                                                                     
         
    

	*aso = so;

	return 0;
}

  
                       
                
              
                   
           
                                
                            
                           
                                                     
   
int
socreate(int dom, struct socket **aso, int type, int proto)
{
	return socreate_internal(dom, aso, type, proto, current_proc(), 0,
	           PROC_NULL);
}

int
socreate_delegate(int dom, struct socket **aso, int type, int proto, pid_t epid)
{
	int error = 0;
	struct proc *ep = PROC_NULL;

	if ((proc_selfpid() != epid) && ((ep = proc_find(epid)) == PROC_NULL)) {
		error = ESRCH;
		goto done;
	}

	error = socreate_internal(dom, aso, type, proto, current_proc(), 0, ep);

	  
                                                                
                                                          
    
done:
	if (ep != PROC_NULL) {
		proc_rele(ep);
	}

	return error;
}

  
                       
                                                     
                                                        
                                                  
                                      
                                                                 
                                       
                                        
                                                     
                                            
                 
                
  
                                                                      
                                                               
                                                                 
                                                                  
  
                                                                 
                                                      
   
int
sobindlock(struct socket *so, struct sockaddr *nam, int dolock)
{
	struct proc *p = current_proc();
	int error = 0;

	if (dolock) {
		socket_lock(so, 1);
	}

	so_update_last_owner_locked(so, p);
	so_update_policy(so);

#if NECP
	so_update_necp_policy(so, nam, NULL);
#endif           

	  
                                                              
                                                        
    
	if (so->so_flags & SOF_DEFUNCT) {
		error = EINVAL;
		SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] (%d)\n",
		    __func__, proc_pid(p), proc_best_name(p),
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    SOCK_DOM(so), SOCK_TYPE(so), error);
		goto out;
	}

	                   
	error = sflt_bind(so, nam);

	if (error == 0) {
		error = (*so->so_proto->pr_usrreqs->pru_bind)(so, nam, p);
	}
out:
	if (dolock) {
		socket_unlock(so, 1);
	}

	if (error == EJUSTRETURN) {
		error = 0;
	}

	return error;
}

void
sodealloc(struct socket *so)
{
	kauth_cred_unref(&so->so_cred);

	                        
	sflt_termsock(so);

#if CONTENT_FILTER
	cfil_sock_detach(so);
#endif                     

	so->so_gencnt = OSIncrementAtomic64((SInt64 *)&so_gencnt);

	if (so->so_flags1 & SOF1_CACHED_IN_SOCK_LAYER) {
		cached_sock_free(so);
	} else {
		zfree(socket_zone, so);
	}
}

  
                       
          
              
                               
                           
                                                         
                                            
                                                                       
                                             
                                              
                                                           
                                                   
                  
  
                                                                       
                                                               
                           
   
int
solisten(struct socket *so, int backlog)
{
	struct proc *p = current_proc();
	int error = 0;

	socket_lock(so, 1);

	so_update_last_owner_locked(so, p);
	so_update_policy(so);

#if NECP
	so_update_necp_policy(so, NULL, NULL);
#endif           

	if (so->so_proto == NULL) {
		error = EINVAL;
		goto out;
	}
	if ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0) {
		error = EOPNOTSUPP;
		goto out;
	}

	  
                                                               
                                                                  
                           
    
	if ((so->so_state &
	    (SS_ISCONNECTED | SS_ISCONNECTING | SS_ISDISCONNECTING)) ||
	    (so->so_flags & SOF_DEFUNCT)) {
		error = EINVAL;
		if (so->so_flags & SOF_DEFUNCT) {
			SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] "
			    "(%d)\n", __func__, proc_pid(p),
			    proc_best_name(p),
			    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
			    SOCK_DOM(so), SOCK_TYPE(so), error);
		}
		goto out;
	}

	if ((so->so_restrictions & SO_RESTRICT_DENY_IN) != 0) {
		error = EPERM;
		goto out;
	}

	error = sflt_listen(so);
	if (error == 0) {
		error = (*so->so_proto->pr_usrreqs->pru_listen)(so, p);
	}

	if (error) {
		if (error == EJUSTRETURN) {
			error = 0;
		}
		goto out;
	}

	if (TAILQ_EMPTY(&so->so_comp)) {
		so->so_options |= SO_ACCEPTCONN;
	}
	  
                                                                      
                                                                      
                                                                    
          
   
                                                                    
                                                                        
                        
   
                                                                       
                                                                 
                                         
    
	if (backlog <= 0 || backlog > somaxconn) {
		backlog = somaxconn;
	}

	so->so_qlimit = backlog;
out:
	socket_unlock(so, 1);
	return error;
}

  
                                                                            
                                                                      
                                                                             
                                                        
  
                                                                  
                            
              
                
              
                
                                                        
              
              
  
                                                                     
                                 
  
                                                                          
                                                                         
                                                                        
                                                 
   
void
so_acquire_accept_list(struct socket *head, struct socket *so)
{
	lck_mtx_t *mutex_held;

	if (head->so_proto->pr_getlock == NULL) {
		return;
	}
	mutex_held = (*head->so_proto->pr_getlock)(head, PR_F_WILLUNLOCK);
	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);

	if (!(head->so_flags1 & SOF1_ACCEPT_LIST_HELD)) {
		head->so_flags1 |= SOF1_ACCEPT_LIST_HELD;
		return;
	}
	if (so != NULL) {
		socket_unlock(so, 0);
	}
	while (head->so_flags1 & SOF1_ACCEPT_LIST_HELD) {
		so_accept_list_waits += 1;
		msleep((caddr_t)&head->so_incomp, mutex_held,
		    PSOCK | PCATCH, __func__, NULL);
	}
	head->so_flags1 |= SOF1_ACCEPT_LIST_HELD;
	if (so != NULL) {
		socket_unlock(head, 0);
		socket_lock(so, 0);
		socket_lock(head, 0);
	}
}

void
so_release_accept_list(struct socket *head)
{
	if (head->so_proto->pr_getlock != NULL) {
		lck_mtx_t *mutex_held;

		mutex_held = (*head->so_proto->pr_getlock)(head, 0);
		LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);

		head->so_flags1 &= ~SOF1_ACCEPT_LIST_HELD;
		wakeup((caddr_t)&head->so_incomp);
	}
}

void
sofreelastref(struct socket *so, int dealloc)
{
	struct socket *head = so->so_head;

	                             

	if (!(so->so_flags & SOF_PCBCLEARING) || !(so->so_state & SS_NOFDREF)) {
		selthreadclear(&so->so_snd.sb_sel);
		selthreadclear(&so->so_rcv.sb_sel);
		so->so_rcv.sb_flags &= ~(SB_SEL | SB_UPCALL);
		so->so_snd.sb_flags &= ~(SB_SEL | SB_UPCALL);
		so->so_event = sonullevent;
		return;
	}
	if (head != NULL) {
		  
                                                    
                     
     
		if (head->so_proto->pr_getlock != NULL) {
			socket_lock(head, 1);
			so_acquire_accept_list(head, so);
		}
		if (so->so_state & SS_INCOMP) {
			so->so_state &= ~SS_INCOMP;
			TAILQ_REMOVE(&head->so_incomp, so, so_list);
			head->so_incqlen--;
			head->so_qlen--;
			so->so_head = NULL;

			if (head->so_proto->pr_getlock != NULL) {
				so_release_accept_list(head);
				socket_unlock(head, 1);
			}
		} else if (so->so_state & SS_COMP) {
			if (head->so_proto->pr_getlock != NULL) {
				so_release_accept_list(head);
				socket_unlock(head, 1);
			}
			  
                                              
                                             
                                                  
                                          
      
			selthreadclear(&so->so_snd.sb_sel);
			selthreadclear(&so->so_rcv.sb_sel);
			so->so_rcv.sb_flags &= ~(SB_SEL | SB_UPCALL);
			so->so_snd.sb_flags &= ~(SB_SEL | SB_UPCALL);
			so->so_event = sonullevent;
			return;
		} else {
			if (head->so_proto->pr_getlock != NULL) {
				so_release_accept_list(head);
				socket_unlock(head, 1);
			}
			printf("sofree: not queued\n");
		}
	}
	sowflush(so);
	sorflush(so);

#if FLOW_DIVERT
	if (so->so_flags & SOF_FLOW_DIVERT) {
		flow_divert_detach(so);
	}
#endif                   

	                             
	so->so_rcv.sb_flags &= ~SB_UPCALL;
	so->so_snd.sb_flags &= ~(SB_UPCALL | SB_SNDBYTE_CNT);
	so->so_event = sonullevent;

	if (dealloc) {
		sodealloc(so);
	}
}

void
soclose_wait_locked(struct socket *so)
{
	lck_mtx_t *mutex_held;

	if (so->so_proto->pr_getlock != NULL) {
		mutex_held = (*so->so_proto->pr_getlock)(so, PR_F_WILLUNLOCK);
	} else {
		mutex_held = so->so_proto->pr_domain->dom_mtx;
	}
	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);

	  
                                                                  
                                                                 
    
	if (!so->so_upcallusecount || !(so->so_flags & SOF_UPCALLCLOSEWAIT)) {
		return;
	}
	so->so_rcv.sb_flags &= ~SB_UPCALL;
	so->so_snd.sb_flags &= ~SB_UPCALL;
	so->so_flags |= SOF_CLOSEWAIT;

	(void) msleep((caddr_t)&so->so_upcallusecount, mutex_held, (PZERO - 1),
	    "soclose_wait_locked", NULL);
	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);
	so->so_flags &= ~SOF_CLOSEWAIT;
}

  
                                                       
                                    
                                        
   
int
soclose_locked(struct socket *so)
{
	int error = 0;
	struct timespec ts;

	if (so->so_usecount == 0) {
		panic("soclose: so=%p refcount=0\n", so);
		                
	}

	sflt_notify(so, sock_evt_closing, NULL);

	if (so->so_upcallusecount) {
		soclose_wait_locked(so);
	}

#if CONTENT_FILTER
	  
                                                      
    
	if ((so->so_flags & SOF_CONTENT_FILTER) != 0) {
		cfil_sock_close_wait(so);
		cfil_sock_is_closed(so);
		cfil_sock_detach(so);
	}
#endif                     

	if (so->so_flags1 & SOF1_EXTEND_BK_IDLE_INPROG) {
		soresume(current_proc(), so, 1);
		so->so_flags1 &= ~SOF1_EXTEND_BK_IDLE_WANTED;
	}

	if ((so->so_options & SO_ACCEPTCONN)) {
		struct socket *sp, *sonext;
		int persocklock = 0;
		int incomp_overflow_only;

		  
                                              
                             
     
		so->so_options &= ~SO_ACCEPTCONN;

		  
                                              
                                     
     
		if (so->so_proto->pr_getlock != NULL) {
			persocklock = 1;
			so_acquire_accept_list(so, NULL);
			socket_unlock(so, 0);
		}
again:
		incomp_overflow_only = 1;

		TAILQ_FOREACH_SAFE(sp, &so->so_incomp, so_list, sonext) {
			  
                   
                                                  
                                                      
                                                        
                                                      
      
			if (sp->so_flags & SOF_OVERFLOW) {
				continue;
			}

			if (persocklock != 0) {
				socket_lock(sp, 1);
			}

			  
                    
                                                 
                                                        
                              
      
			if (sp->so_state & SS_INCOMP) {
				sp->so_state &= ~SS_INCOMP;
				sp->so_head = NULL;
				TAILQ_REMOVE(&so->so_incomp, sp, so_list);
				so->so_incqlen--;
				so->so_qlen--;

				(void) soabort(sp);
			} else {
				panic("%s sp %p in so_incomp but !SS_INCOMP",
				    __func__, sp);
			}

			if (persocklock != 0) {
				socket_unlock(sp, 1);
			}
		}

		TAILQ_FOREACH_SAFE(sp, &so->so_comp, so_list, sonext) {
			                                                     
			if (persocklock != 0) {
				socket_lock(sp, 1);
			}

			if (sp->so_state & SS_COMP) {
				sp->so_state &= ~SS_COMP;
				sp->so_head = NULL;
				TAILQ_REMOVE(&so->so_comp, sp, so_list);
				so->so_qlen--;

				(void) soabort(sp);
			} else {
				panic("%s sp %p in so_comp but !SS_COMP",
				    __func__, sp);
			}

			if (persocklock) {
				socket_unlock(sp, 1);
			}
		}

		if (incomp_overflow_only == 0 && !TAILQ_EMPTY(&so->so_incomp)) {
#if (DEBUG | DEVELOPMENT)
			panic("%s head %p so_comp not empty\n", __func__, so);
#endif                             

			goto again;
		}

		if (!TAILQ_EMPTY(&so->so_comp)) {
#if (DEBUG | DEVELOPMENT)
			panic("%s head %p so_comp not empty\n", __func__, so);
#endif                             

			goto again;
		}

		if (persocklock) {
			socket_lock(so, 0);
			so_release_accept_list(so);
		}
	}
	if (so->so_pcb == NULL) {
		                                                   
		so->so_flags |= SOF_PCBCLEARING;
		goto discard;
	}
	if (so->so_state & SS_ISCONNECTED) {
		if ((so->so_state & SS_ISDISCONNECTING) == 0) {
			error = sodisconnectlocked(so);
			if (error) {
				goto drop;
			}
		}
		if (so->so_options & SO_LINGER) {
			lck_mtx_t *mutex_held;

			if ((so->so_state & SS_ISDISCONNECTING) &&
			    (so->so_state & SS_NBIO)) {
				goto drop;
			}
			if (so->so_proto->pr_getlock != NULL) {
				mutex_held = (*so->so_proto->pr_getlock)(so, PR_F_WILLUNLOCK);
			} else {
				mutex_held = so->so_proto->pr_domain->dom_mtx;
			}
			while (so->so_state & SS_ISCONNECTED) {
				ts.tv_sec = (so->so_linger / 100);
				ts.tv_nsec = (so->so_linger % 100) *
				    NSEC_PER_USEC * 1000 * 10;
				error = msleep((caddr_t)&so->so_timeo,
				    mutex_held, PSOCK | PCATCH, "soclose", &ts);
				if (error) {
					  
                                    
                             
        
					if (error == EWOULDBLOCK) {
						error = 0;
					}
					break;
				}
			}
		}
	}
drop:
	if (so->so_usecount == 0) {
		panic("soclose: usecount is zero so=%p\n", so);
		                
	}
	if (so->so_pcb != NULL && !(so->so_flags & SOF_PCBCLEARING)) {
		int error2 = (*so->so_proto->pr_usrreqs->pru_detach)(so);
		if (error == 0) {
			error = error2;
		}
	}
	if (so->so_usecount <= 0) {
		panic("soclose: usecount is zero so=%p\n", so);
		                
	}
discard:
	if (so->so_pcb != NULL && !(so->so_flags & SOF_MP_SUBFLOW) &&
	    (so->so_state & SS_NOFDREF)) {
		panic("soclose: NOFDREF");
		                
	}
	so->so_state |= SS_NOFDREF;

	if ((so->so_flags & SOF_KNOTE) != 0) {
		KNOTE(&so->so_klist, SO_FILT_HINT_LOCKED);
	}

	atomic_add_32(&so->so_proto->pr_domain->dom_refs, -1);

	VERIFY(so->so_usecount > 0);
	so->so_usecount--;
	sofree(so);
	return error;
}

int
soclose(struct socket *so)
{
	int error = 0;
	socket_lock(so, 1);

	if (so->so_retaincnt == 0) {
		error = soclose_locked(so);
	} else {
		  
                                           
                                            
     
		so->so_usecount--;
		if (so->so_usecount < 2) {
			panic("soclose: retaincnt non null and so=%p "
			    "usecount=%d\n", so, so->so_usecount);
		}
	}
	socket_unlock(so, 1);
	return error;
}

  
                              
   
                              
int
soabort(struct socket *so)
{
	int error;

#ifdef MORE_LOCKING_DEBUG
	lck_mtx_t *mutex_held;

	if (so->so_proto->pr_getlock != NULL) {
		mutex_held = (*so->so_proto->pr_getlock)(so, 0);
	} else {
		mutex_held = so->so_proto->pr_domain->dom_mtx;
	}
	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);
#endif

	if ((so->so_flags & SOF_ABORTED) == 0) {
		so->so_flags |= SOF_ABORTED;
		error = (*so->so_proto->pr_usrreqs->pru_abort)(so);
		if (error) {
			sofree(so);
			return error;
		}
	}
	return 0;
}

int
soacceptlock(struct socket *so, struct sockaddr **nam, int dolock)
{
	int error;

	if (dolock) {
		socket_lock(so, 1);
	}

	so_update_last_owner_locked(so, PROC_NULL);
	so_update_policy(so);
#if NECP
	so_update_necp_policy(so, NULL, NULL);
#endif           

	if ((so->so_state & SS_NOFDREF) == 0) {
		panic("soaccept: !NOFDREF");
	}
	so->so_state &= ~SS_NOFDREF;
	error = (*so->so_proto->pr_usrreqs->pru_accept)(so, nam);

	if (dolock) {
		socket_unlock(so, 1);
	}
	return error;
}

int
soaccept(struct socket *so, struct sockaddr **nam)
{
	return soacceptlock(so, nam, 1);
}

int
soacceptfilter(struct socket *so, struct socket *head)
{
	struct sockaddr *local = NULL, *remote = NULL;
	int error = 0;

	  
                                                               
                                                                   
                                       
    
	socket_lock(so, 1);
	if (sogetaddr_locked(so, &remote, 1) != 0 ||
	    sogetaddr_locked(so, &local, 0) != 0) {
		so->so_state &= ~SS_NOFDREF;
		socket_unlock(so, 1);
		soclose(so);
		                                              
		error = ECONNABORTED;
		goto done;
	}

	error = sflt_accept(head, so, local, remote);

	  
                                                                   
                                                                 
                                                                   
    
	if (error == EJUSTRETURN) {
		error = 0;
		(void) sosetdefunct(current_proc(), so,
		    SHUTDOWN_SOCKET_LEVEL_DISCONNECT_INTERNAL, FALSE);
	}

	if (error != 0) {
		  
                                                        
                                                      
                                                       
                                                          
     
		so->so_state &= ~SS_NOFDREF;
		socket_unlock(so, 1);
		soclose(so);
		                                                        
	} else {
		socket_unlock(so, 1);
	}
done:
	                                    
	sock_freeaddr(remote);
	sock_freeaddr(local);
	return error;
}

  
                       
                                                 
                                 
                                                     
                                         
                                                                   
                                          
                                          
                                                        
                                               
                                                             
   
int
soconnectlock(struct socket *so, struct sockaddr *nam, int dolock)
{
	int error;
	struct proc *p = current_proc();

	if (dolock) {
		socket_lock(so, 1);
	}

	so_update_last_owner_locked(so, p);
	so_update_policy(so);

#if NECP
	so_update_necp_policy(so, NULL, nam);
#endif           

	  
                                                                     
                                                                        
    
	if ((so->so_options & SO_ACCEPTCONN) || (so->so_flags & SOF_DEFUNCT)) {
		error = EOPNOTSUPP;
		if (so->so_flags & SOF_DEFUNCT) {
			SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] "
			    "(%d)\n", __func__, proc_pid(p),
			    proc_best_name(p),
			    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
			    SOCK_DOM(so), SOCK_TYPE(so), error);
		}
		if (dolock) {
			socket_unlock(so, 1);
		}
		return error;
	}

	if ((so->so_restrictions & SO_RESTRICT_DENY_OUT) != 0) {
		if (dolock) {
			socket_unlock(so, 1);
		}
		return EPERM;
	}

	  
                                                           
                                                     
                                                          
                   
    
	if (so->so_state & (SS_ISCONNECTED | SS_ISCONNECTING) &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) ||
	    (error = sodisconnectlocked(so)))) {
		error = EISCONN;
	} else {
		  
                                                
                                                       
     
		error = sflt_connectout(so, nam);
		if (error != 0) {
			if (error == EJUSTRETURN) {
				error = 0;
			}
		} else {
			error = (*so->so_proto->pr_usrreqs->pru_connect)
			    (so, nam, p);
			if (error != 0) {
				so->so_state &= ~SS_ISCONNECTING;
			}
		}
	}
	if (dolock) {
		socket_unlock(so, 1);
	}
	return error;
}

int
soconnect(struct socket *so, struct sockaddr *nam)
{
	return soconnectlock(so, nam, 1);
}

  
                       
                                 
                                     
                                                
  
                                                   
   
int
soconnect2(struct socket *so1, struct socket *so2)
{
	int error;

	socket_lock(so1, 1);
	if (so2->so_proto->pr_lock) {
		socket_lock(so2, 1);
	}

	error = (*so1->so_proto->pr_usrreqs->pru_connect2)(so1, so2);

	socket_unlock(so1, 1);
	if (so2->so_proto->pr_lock) {
		socket_unlock(so2, 1);
	}
	return error;
}

int
soconnectxlocked(struct socket *so, struct sockaddr *src,
    struct sockaddr *dst, struct proc *p, uint32_t ifscope,
    sae_associd_t aid, sae_connid_t *pcid, uint32_t flags, void *arg,
    uint32_t arglen, uio_t auio, user_ssize_t *bytes_written)
{
	int error;

	so_update_last_owner_locked(so, p);
	so_update_policy(so);

	  
                                                                     
                                                                        
    
	if ((so->so_options & SO_ACCEPTCONN) || (so->so_flags & SOF_DEFUNCT)) {
		error = EOPNOTSUPP;
		if (so->so_flags & SOF_DEFUNCT) {
			SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] "
			    "(%d)\n", __func__, proc_pid(p),
			    proc_best_name(p),
			    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
			    SOCK_DOM(so), SOCK_TYPE(so), error);
		}
		return error;
	}

	if ((so->so_restrictions & SO_RESTRICT_DENY_OUT) != 0) {
		return EPERM;
	}

	  
                                                          
                                                         
                                                            
                                           
    
	if ((so->so_state & (SS_ISCONNECTED | SS_ISCONNECTING)) &&
	    !(so->so_proto->pr_flags & PR_MULTICONN) &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) ||
	    (error = sodisconnectlocked(so)) != 0)) {
		error = EISCONN;
	} else {
		if ((so->so_proto->pr_flags & PR_DATA_IDEMPOTENT) &&
		    (flags & CONNECT_DATA_IDEMPOTENT)) {
			so->so_flags1 |= SOF1_DATA_IDEMPOTENT;

			if (flags & CONNECT_DATA_AUTHENTICATED) {
				so->so_flags1 |= SOF1_DATA_AUTHENTICATED;
			}
		}

		  
                                                       
                                                                     
                                                            
                                                                       
                                                   
                                                                         
     
		if ((so->so_proto->pr_flags & PR_PRECONN_WRITE) &&
		    ((flags & CONNECT_RESUME_ON_READ_WRITE) || auio)) {
			so->so_flags1 |= SOF1_PRECONNECT_DATA;
		}

		  
                                                                
                                                               
                          
     
		if (!(so->so_flags1 & SOF1_PRECONNECT_DATA) &&
		    (so->so_flags1 & SOF1_DATA_IDEMPOTENT)) {
			                                              
			so->so_flags1 &= ~SOF1_DATA_IDEMPOTENT;
		}

		  
                                                
                                                       
     
		error = sflt_connectout(so, dst);
		if (error != 0) {
			                                                                      
			so->so_flags1 &= ~SOF1_PRECONNECT_DATA;
			if (error == EJUSTRETURN) {
				error = 0;
			}
		} else {
			error = (*so->so_proto->pr_usrreqs->pru_connectx)
			    (so, src, dst, p, ifscope, aid, pcid,
			    flags, arg, arglen, auio, bytes_written);
			if (error != 0) {
				so->so_state &= ~SS_ISCONNECTING;
				if (error != EINPROGRESS) {
					so->so_flags1 &= ~SOF1_PRECONNECT_DATA;
				}
			}
		}
	}

	return error;
}

int
sodisconnectlocked(struct socket *so)
{
	int error;

	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto bad;
	}
	if (so->so_state & SS_ISDISCONNECTING) {
		error = EALREADY;
		goto bad;
	}

	error = (*so->so_proto->pr_usrreqs->pru_disconnect)(so);
	if (error == 0) {
		sflt_notify(so, sock_evt_disconnected, NULL);
	}

bad:
	return error;
}

                     
int
sodisconnect(struct socket *so)
{
	int error;

	socket_lock(so, 1);
	error = sodisconnectlocked(so);
	socket_unlock(so, 1);
	return error;
}

int
sodisconnectxlocked(struct socket *so, sae_associd_t aid, sae_connid_t cid)
{
	int error;

	  
                                                            
                                                            
    
	error = (*so->so_proto->pr_usrreqs->pru_disconnectx)(so, aid, cid);
	if (error == 0) {
		  
                                                    
                                              
     
		if (so->so_state & (SS_ISDISCONNECTING | SS_ISDISCONNECTED)) {
			sflt_notify(so, sock_evt_disconnected, NULL);
		}
	}
	return error;
}

int
sodisconnectx(struct socket *so, sae_associd_t aid, sae_connid_t cid)
{
	int error;

	socket_lock(so, 1);
	error = sodisconnectxlocked(so, aid, cid);
	socket_unlock(so, 1);
	return error;
}

#define SBLOCKWAIT(f)   (((f) & MSG_DONTWAIT) ? 0 : SBL_WAIT)

  
                                                                 
                                                          
  
                       
         
                     
               
               
               
                 
   
int
sosendcheck(struct socket *so, struct sockaddr *addr, user_ssize_t resid,
    int32_t clen, int32_t atomic, int flags, int *sblocked)
{
	int     error = 0;
	int32_t space;
	int     assumelock = 0;

restart:
	if (*sblocked == 0) {
		if ((so->so_snd.sb_flags & SB_LOCK) != 0 &&
		    so->so_send_filt_thread != 0 &&
		    so->so_send_filt_thread == current_thread()) {
			  
                                                   
                                            
                                              
                                 
      
			assumelock = 1;
		} else {
			error = sblock(&so->so_snd, SBLOCKWAIT(flags));
			if (error) {
				if (so->so_flags & SOF_DEFUNCT) {
					goto defunct;
				}
				return error;
			}
			*sblocked = 1;
		}
	}

	  
                                                              
                                                   
    
	if (so->so_flags & SOF_DEFUNCT) {
defunct:
		error = EPIPE;
		SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] (%d)\n",
		    __func__, proc_selfpid(), proc_best_name(current_proc()),
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    SOCK_DOM(so), SOCK_TYPE(so), error);
		return error;
	}

	if (so->so_state & SS_CANTSENDMORE) {
#if CONTENT_FILTER
		  
                                                  
     
		if ((so->so_state & SS_ISDISCONNECTED) == 0 &&
		    so->so_snd.sb_cfil_thread == current_thread() &&
		    cfil_sock_data_pending(&so->so_snd) != 0) {
			CFIL_LOG(LOG_INFO,
			    "so %llx ignore SS_CANTSENDMORE",
			    (uint64_t)DEBUG_KERNEL_ADDRPERM(so));
		} else
#endif                     
		return EPIPE;
	}
	if (so->so_error) {
		error = so->so_error;
		so->so_error = 0;
		return error;
	}

	if ((so->so_state & SS_ISCONNECTED) == 0) {
		if ((so->so_proto->pr_flags & PR_CONNREQUIRED) != 0) {
			if (((so->so_state & SS_ISCONFIRMING) == 0) &&
			    (resid != 0 || clen == 0) &&
			    !(so->so_flags1 & SOF1_PRECONNECT_DATA)) {
				return ENOTCONN;
			}
		} else if (addr == 0) {
			return (so->so_proto->pr_flags & PR_CONNREQUIRED) ?
			       ENOTCONN : EDESTADDRREQ;
		}
	}

	space = sbspace(&so->so_snd);

	if (flags & MSG_OOB) {
		space += 1024;
	}
	if ((atomic && resid > so->so_snd.sb_hiwat) ||
	    clen > so->so_snd.sb_hiwat) {
		return EMSGSIZE;
	}

	if ((space < resid + clen &&
	    (atomic || (space < (int32_t)so->so_snd.sb_lowat) ||
	    space < clen)) ||
	    (so->so_type == SOCK_STREAM && so_wait_for_if_feedback(so))) {
		  
                                                         
                        
     
		if (so->so_flags1 & SOF1_PRECONNECT_DATA) {
			if (space == 0) {
				return EWOULDBLOCK;
			}
			if (space < (int32_t)so->so_snd.sb_lowat) {
				return 0;
			}
		}
		if ((so->so_state & SS_NBIO) || (flags & MSG_NBIO) ||
		    assumelock) {
			return EWOULDBLOCK;
		}
		sbunlock(&so->so_snd, TRUE);                            
		*sblocked = 0;
		error = sbwait(&so->so_snd);
		if (error) {
			if (so->so_flags & SOF_DEFUNCT) {
				goto defunct;
			}
			return error;
		}
		goto restart;
	}
	return 0;
}

  
                    
                                                         
                                   
                              
                                                       
                                                    
                                                       
                                                        
                                                        
                                                             
                              
  
                                                       
                                                              
                                                
  
                       
              
          
           
                 
                    
                          
                    
                    
                    
                                          
                             
                         
                          
                             
                                
                               
                         
                         
                        
                           
                               
                              
                           
                         
                          
                                                               
                             
                                 
                            
                               
                              
                                                              
                                                        
  
                                                                     
                                                                 
                           
   
int
sosend(struct socket *so, struct sockaddr *addr, struct uio *uio,
    struct mbuf *top, struct mbuf *control, int flags)
{
	struct mbuf **mp;
	struct mbuf *m, *freelist = NULL;
	user_ssize_t space, len, resid, orig_resid;
	int clen = 0, error, dontroute, mlen, sendflags;
	int atomic = sosendallatonce(so) || top;
	int sblocked = 0;
	struct proc *p = current_proc();
	uint16_t headroom = 0;
	boolean_t en_tracing = FALSE;

	if (uio != NULL) {
		resid = uio_resid(uio);
	} else {
		resid = top->m_pkthdr.len;
	}

	KERNEL_DEBUG((DBG_FNC_SOSEND | DBG_FUNC_START), so, resid,
	    so->so_snd.sb_cc, so->so_snd.sb_lowat, so->so_snd.sb_hiwat);

	socket_lock(so, 1);

	  
                                                       
                
    
	if (ENTR_SHOULDTRACE &&
	    (SOCK_CHECK_DOM(so, AF_INET) || SOCK_CHECK_DOM(so, AF_INET6))) {
		struct inpcb *inp = sotoinpcb(so);
		if (inp->inp_last_outifp != NULL &&
		    !(inp->inp_last_outifp->if_flags & IFF_LOOPBACK)) {
			en_tracing = TRUE;
			KERNEL_ENERGYTRACE(kEnTrActKernSockWrite, DBG_FUNC_START,
			    VM_KERNEL_ADDRPERM(so),
			    ((so->so_state & SS_NBIO) ? kEnTrFlagNonBlocking : 0),
			    (int64_t)resid);
			orig_resid = resid;
		}
	}

	  
                                                     
    
	if ((flags & MSG_SKIPCFIL) == 0) {
		so_update_last_owner_locked(so, p);
		so_update_policy(so);

#if NECP
		so_update_necp_policy(so, NULL, addr);
#endif           
	}

	if (so->so_type != SOCK_STREAM && (flags & MSG_OOB) != 0) {
		error = EOPNOTSUPP;
		goto out_locked;
	}

	  
                                       
                                                             
                                                             
                                                            
                                                                
   
                                                            
   
                                                             
                                                  
    
	if (resid < 0 || resid > INT_MAX ||
	    (so->so_type == SOCK_STREAM && (flags & MSG_EOR))) {
		error = EINVAL;
		goto out_locked;
	}

	dontroute = (flags & MSG_DONTROUTE) &&
	    (so->so_options & SO_DONTROUTE) == 0 &&
	    (so->so_proto->pr_flags & PR_ATOMIC);
	OSIncrementAtomicLong(&p->p_stats->p_ru.ru_msgsnd);

	if (control != NULL) {
		clen = control->m_len;
	}

	if (soreserveheadroom != 0) {
		headroom = so->so_pktheadroom;
	}

	do {
		error = sosendcheck(so, addr, resid, clen, atomic, flags,
		    &sblocked);
		if (error) {
			goto out_locked;
		}

		mp = &top;
		space = sbspace(&so->so_snd) - clen;
		space += ((flags & MSG_OOB) ? 1024 : 0);

		do {
			if (uio == NULL) {
				  
                                    
       
				resid = 0;
				if (flags & MSG_EOR) {
					top->m_flags |= M_EOR;
				}
			} else {
				int chainlength;
				int bytes_to_copy;
				boolean_t jumbocl;
				boolean_t bigcl;
				int bytes_to_alloc;

				bytes_to_copy = imin(resid, space);

				bytes_to_alloc = bytes_to_copy;
				if (top == NULL) {
					bytes_to_alloc += headroom;
				}

				if (sosendminchain > 0) {
					chainlength = 0;
				} else {
					chainlength = sosendmaxchain;
				}

				  
                                                       
                                    
       
				bigcl = !(so->so_flags1 & SOF1_IF_2KCL) ||
				    sosendbigcl_ignore_capab;

				  
                                                  
                                                 
                                                
                          
       
				jumbocl = sosendjcl && njcl > 0 &&
				    ((so->so_flags & SOF_MULTIPAGES) ||
				    sosendjcl_ignore_capab) &&
				    bigcl;

				socket_unlock(so, 0);

				do {
					int num_needed;
					int hdrs_needed = (top == NULL) ? 1 : 0;

					  
                                             
                                        
                                            
                                            
                                         
                                       
                                             
                                        
                                          
                                      
                                            
                                       
                             
        
					if (freelist == NULL &&
					    bytes_to_alloc > MBIGCLBYTES &&
					    jumbocl) {
						num_needed =
						    bytes_to_alloc / M16KCLBYTES;

						if ((bytes_to_alloc -
						    (num_needed * M16KCLBYTES))
						    >= MINCLSIZE) {
							num_needed++;
						}

						freelist =
						    m_getpackets_internal(
							(unsigned int *)&num_needed,
							hdrs_needed, M_WAIT, 0,
							M16KCLBYTES);
						  
                                     
                             
         
					}

					if (freelist == NULL &&
					    bytes_to_alloc > MCLBYTES &&
					    bigcl) {
						num_needed =
						    bytes_to_alloc / MBIGCLBYTES;

						if ((bytes_to_alloc -
						    (num_needed * MBIGCLBYTES)) >=
						    MINCLSIZE) {
							num_needed++;
						}

						freelist =
						    m_getpackets_internal(
							(unsigned int *)&num_needed,
							hdrs_needed, M_WAIT, 0,
							MBIGCLBYTES);
						  
                                  
                             
         
					}

					  
                                        
                                       
                                            
                                           
        
					if (soreserveheadroom != 0 &&
					    freelist == NULL &&
					    ((top == NULL &&
					    bytes_to_alloc > _MHLEN) ||
					    bytes_to_alloc > _MLEN)) {
						num_needed = ROUNDUP(bytes_to_alloc, MCLBYTES) /
						    MCLBYTES;
						freelist =
						    m_getpackets_internal(
							(unsigned int *)&num_needed,
							hdrs_needed, M_WAIT, 0,
							MCLBYTES);
						  
                                   
                             
         
					} else if (freelist == NULL &&
					    bytes_to_alloc > MINCLSIZE) {
						num_needed =
						    bytes_to_alloc / MCLBYTES;

						if ((bytes_to_alloc -
						    (num_needed * MCLBYTES)) >=
						    MINCLSIZE) {
							num_needed++;
						}

						freelist =
						    m_getpackets_internal(
							(unsigned int *)&num_needed,
							hdrs_needed, M_WAIT, 0,
							MCLBYTES);
						  
                                   
                             
         
					}
					  
                                     
                                     
                                         
        
					if (freelist != NULL && atomic &&
					    top == NULL && headroom > 0) {
						freelist->m_data += headroom;
					}

					  
                                          
                                     
        
					if (freelist == NULL) {
						if (SOCK_TYPE(so) != SOCK_STREAM || bytes_to_alloc <= MINCLSIZE) {
							if (top == NULL) {
								MGETHDR(freelist,
								    M_WAIT, MT_DATA);
							} else {
								MGET(freelist,
								    M_WAIT, MT_DATA);
							}
						}

						if (freelist == NULL) {
							error = ENOBUFS;
							socket_lock(so, 0);
							goto out_locked;
						}
						  
                                
                                
                               
         
						if (atomic && top == NULL &&
						    bytes_to_copy < MHLEN) {
							MH_ALIGN(freelist,
							    bytes_to_copy);
						}
					}
					m = freelist;
					freelist = m->m_next;
					m->m_next = NULL;

					if ((m->m_flags & M_EXT)) {
						mlen = m->m_ext.ext_size -
						    M_LEADINGSPACE(m);
					} else if ((m->m_flags & M_PKTHDR)) {
						mlen =
						    MHLEN - M_LEADINGSPACE(m);
					} else {
						mlen = MLEN - M_LEADINGSPACE(m);
					}
					len = imin(mlen, bytes_to_copy);

					chainlength += len;

					space -= len;

					error = uiomove(mtod(m, caddr_t),
					    len, uio);

					resid = uio_resid(uio);

					m->m_len = len;
					*mp = m;
					top->m_pkthdr.len += len;
					if (error) {
						break;
					}
					mp = &m->m_next;
					if (resid <= 0) {
						if (flags & MSG_EOR) {
							top->m_flags |= M_EOR;
						}
						break;
					}
					bytes_to_copy = min(resid, space);
				} while (space > 0 &&
				    (chainlength < sosendmaxchain || atomic ||
				    resid < MINCLSIZE));

				socket_lock(so, 0);

				if (error) {
					goto out_locked;
				}
			}

			if (dontroute) {
				so->so_options |= SO_DONTROUTE;
			}

			  
                                               
     
                                           
                                               
                                                     
      
			sendflags = (flags & MSG_OOB) ? PRUS_OOB :
			    ((flags & MSG_EOF) &&
			    (so->so_proto->pr_flags & PR_IMPLOPCL) &&
			    (resid <= 0)) ? PRUS_EOF :
			                                                      
			    (resid > 0 && space > 0) ? PRUS_MORETOCOME : 0;

			if ((flags & MSG_SKIPCFIL) == 0) {
				  
                               
       
				error = sflt_data_out(so, addr, &top,
				    &control, (sendflags & MSG_OOB) ?
				    sock_data_filt_flag_oob : 0);
				if (error) {
					if (error == EJUSTRETURN) {
						error = 0;
						goto packet_consumed;
					}
					goto out_locked;
				}
#if CONTENT_FILTER
				  
                                
       
				error = cfil_sock_data_out(so, addr, top,
				    control, sendflags);
				if (error) {
					if (error == EJUSTRETURN) {
						error = 0;
						goto packet_consumed;
					}
					goto out_locked;
				}
#endif                     
			}
			error = (*so->so_proto->pr_usrreqs->pru_send)
			    (so, sendflags, top, addr, control, p);

packet_consumed:
			if (dontroute) {
				so->so_options &= ~SO_DONTROUTE;
			}

			clen = 0;
			control = NULL;
			top = NULL;
			mp = &top;
			if (error) {
				goto out_locked;
			}
		} while (resid && space > 0);
	} while (resid);

out_locked:
	if (sblocked) {
		sbunlock(&so->so_snd, FALSE);                           
	} else {
		socket_unlock(so, 1);
	}
	if (top != NULL) {
		m_freem(top);
	}
	if (control != NULL) {
		m_freem(control);
	}
	if (freelist != NULL) {
		m_freem_list(freelist);
	}

	soclearfastopen(so);

	if (en_tracing) {
		                                                
		KERNEL_ENERGYTRACE(kEnTrActKernSockWrite, DBG_FUNC_END,
		    VM_KERNEL_ADDRPERM(so),
		    ((error == EWOULDBLOCK) ? kEnTrFlagNoWork : 0),
		    (int64_t)(orig_resid - resid));
	}
	KERNEL_DEBUG(DBG_FNC_SOSEND | DBG_FUNC_END, so, resid,
	    so->so_snd.sb_cc, space, error);

	return error;
}

int
sosend_reinject(struct socket *so, struct sockaddr *addr, struct mbuf *top, struct mbuf *control, uint32_t sendflags)
{
	struct mbuf *m0 = NULL, *control_end = NULL;

	socket_lock_assert_owned(so);

	  
                                             
                                                     
    
	VERIFY(top != NULL &&
	    (control == NULL || top->m_flags & M_PKTHDR));

	  
                                                     
             
    
	if (control == NULL && (top->m_flags & M_PKTHDR) == 0) {
		                                                       
		for (m0 = top; m0 != NULL; m0 = m0->m_next) {
			if (m0->m_flags & M_PKTHDR) {
				top = m0;
				break;
			} else if (m0->m_type == MT_CONTROL) {
				if (control == NULL) {
					                         
					control = m0;
				}
				if (control != NULL && m0->m_next != NULL && m0->m_next->m_type != MT_CONTROL) {
					                       
					control_end = m0;
				}
			}
		}
		if (control_end != NULL) {
			control_end->m_next = NULL;
		}
	}

	int error = (*so->so_proto->pr_usrreqs->pru_send)
	    (so, sendflags, top, addr, control, current_proc());

	return error;
}

  
                                                                       
                                      
   
int
sosend_list(struct socket *so, struct uio **uioarray, u_int uiocnt, int flags)
{
	struct mbuf *m, *freelist = NULL;
	user_ssize_t len, resid;
	int error, dontroute, mlen;
	int atomic = sosendallatonce(so);
	int sblocked = 0;
	struct proc *p = current_proc();
	u_int uiofirst = 0;
	u_int uiolast = 0;
	struct mbuf *top = NULL;
	uint16_t headroom = 0;
	boolean_t bigcl;

	KERNEL_DEBUG((DBG_FNC_SOSEND_LIST | DBG_FUNC_START), so, uiocnt,
	    so->so_snd.sb_cc, so->so_snd.sb_lowat, so->so_snd.sb_hiwat);

	if (so->so_type != SOCK_DGRAM) {
		error = EINVAL;
		goto out;
	}
	if (atomic == 0) {
		error = EINVAL;
		goto out;
	}
	if (so->so_proto->pr_usrreqs->pru_send_list == NULL) {
		error = EPROTONOSUPPORT;
		goto out;
	}
	if (flags & ~(MSG_DONTWAIT | MSG_NBIO)) {
		error = EINVAL;
		goto out;
	}
	resid = uio_array_resid(uioarray, uiocnt);

	  
                                       
                                                             
                                                             
                                                            
                                                                
   
                                                             
                                                  
    
	if (resid < 0 || resid > INT_MAX) {
		error = EINVAL;
		goto out;
	}

	socket_lock(so, 1);
	so_update_last_owner_locked(so, p);
	so_update_policy(so);

#if NECP
	so_update_necp_policy(so, NULL, NULL);
#endif           

	dontroute = (flags & MSG_DONTROUTE) &&
	    (so->so_options & SO_DONTROUTE) == 0 &&
	    (so->so_proto->pr_flags & PR_ATOMIC);
	OSIncrementAtomicLong(&p->p_stats->p_ru.ru_msgsnd);

	error = sosendcheck(so, NULL, resid, 0, atomic, flags, &sblocked);
	if (error) {
		goto release;
	}

	  
                                                                     
                 
    
	bigcl = !(so->so_flags1 & SOF1_IF_2KCL) || sosendbigcl_ignore_capab;

	if (soreserveheadroom != 0) {
		headroom = so->so_pktheadroom;
	}

	do {
		int i;
		int num_needed = 0;
		int chainlength;
		size_t maxpktlen = 0;
		int bytes_to_alloc;

		if (sosendminchain > 0) {
			chainlength = 0;
		} else {
			chainlength = sosendmaxchain;
		}

		socket_unlock(so, 0);

		  
                                                      
                    
     
		for (i = uiofirst; i < uiocnt; i++) {
			struct uio *auio = uioarray[i];

			len = uio_resid(auio);

			                                   
			if (len == 0) {
				continue;
			}

			num_needed += 1;
			uiolast += 1;

			if (len > maxpktlen) {
				maxpktlen = len;
			}

			chainlength += len;
			if (chainlength > sosendmaxchain) {
				break;
			}
		}
		  
                         
     
		if (num_needed == 0) {
			socket_lock(so, 0);
			break;
		}
		  
                                                               
                            
    
     
		bytes_to_alloc = maxpktlen + headroom;

		  
                                                                  
                       
     
		if (bytes_to_alloc > MCLBYTES &&
		    bytes_to_alloc <= MBIGCLBYTES && bigcl) {
			freelist = m_getpackets_internal(
				(unsigned int *)&num_needed,
				num_needed, M_WAIT, 1,
				MBIGCLBYTES);
		} else if (bytes_to_alloc > _MHLEN &&
		    bytes_to_alloc <= MCLBYTES) {
			freelist = m_getpackets_internal(
				(unsigned int *)&num_needed,
				num_needed, M_WAIT, 1,
				MCLBYTES);
		} else {
			freelist = m_allocpacket_internal(
				(unsigned int *)&num_needed,
				bytes_to_alloc, NULL, M_WAIT, 1, 0);
		}

		if (freelist == NULL) {
			socket_lock(so, 0);
			error = ENOMEM;
			goto release;
		}
		  
                                                      
     
		for (i = uiofirst, m = freelist;
		    i < uiolast && m != NULL;
		    i++) {
			int bytes_to_copy;
			struct mbuf *n;
			struct uio *auio = uioarray[i];

			bytes_to_copy = uio_resid(auio);

			                                   
			if (bytes_to_copy == 0) {
				continue;
			}
			  
                                         
                                    
      
			m->m_data += headroom;

			for (n = m; n != NULL; n = n->m_next) {
				if ((m->m_flags & M_EXT)) {
					mlen = m->m_ext.ext_size -
					    M_LEADINGSPACE(m);
				} else if ((m->m_flags & M_PKTHDR)) {
					mlen =
					    MHLEN - M_LEADINGSPACE(m);
				} else {
					mlen = MLEN - M_LEADINGSPACE(m);
				}
				len = imin(mlen, bytes_to_copy);

				  
                                           
             
       
				error = uiomove(mtod(n, caddr_t),
				    len, auio);
				if (error != 0) {
					break;
				}
				n->m_len = len;
				m->m_pkthdr.len += len;

				VERIFY(m->m_pkthdr.len <= maxpktlen);

				bytes_to_copy -= len;
				resid -= len;
			}
			if (m->m_pkthdr.len == 0) {
				printf(
					"%s:%d so %llx pkt %llx type %u len null\n",
					__func__, __LINE__,
					(uint64_t)DEBUG_KERNEL_ADDRPERM(so),
					(uint64_t)DEBUG_KERNEL_ADDRPERM(m),
					m->m_type);
			}
			if (error != 0) {
				break;
			}
			m = m->m_nextpkt;
		}

		socket_lock(so, 0);

		if (error) {
			goto release;
		}
		top = freelist;
		freelist = NULL;

		if (dontroute) {
			so->so_options |= SO_DONTROUTE;
		}

		if ((flags & MSG_SKIPCFIL) == 0) {
			struct mbuf **prevnextp = NULL;

			for (i = uiofirst, m = top;
			    i < uiolast && m != NULL;
			    i++) {
				struct mbuf *nextpkt = m->m_nextpkt;

				  
                               
       
				error = sflt_data_out(so, NULL, &m,
				    NULL, 0);
				if (error != 0 && error != EJUSTRETURN) {
					goto release;
				}

#if CONTENT_FILTER
				if (error == 0) {
					  
                                 
        
					error = cfil_sock_data_out(so, NULL, m,
					    NULL, 0);
					if (error != 0 && error != EJUSTRETURN) {
						goto release;
					}
				}
#endif                     
				  
                                       
                            
       
				if (error == EJUSTRETURN) {
					error = 0;
					if (prevnextp != NULL) {
						*prevnextp = nextpkt;
					} else {
						top = nextpkt;
					}
				}

				m = nextpkt;
				if (m != NULL) {
					prevnextp = &m->m_nextpkt;
				}
			}
		}
		if (top != NULL) {
			error = (*so->so_proto->pr_usrreqs->pru_send_list)
			    (so, 0, top, NULL, NULL, p);
		}

		if (dontroute) {
			so->so_options &= ~SO_DONTROUTE;
		}

		top = NULL;
		uiofirst = uiolast;
	} while (resid > 0 && error == 0);
release:
	if (sblocked) {
		sbunlock(&so->so_snd, FALSE);                           
	} else {
		socket_unlock(so, 1);
	}
out:
	if (top != NULL) {
		m_freem(top);
	}
	if (freelist != NULL) {
		m_freem_list(freelist);
	}

	KERNEL_DEBUG(DBG_FNC_SOSEND_LIST | DBG_FUNC_END, so, resid,
	    so->so_snd.sb_cc, 0, error);

	return error;
}

  
                                                                 
   
static int
soreceive_addr(struct proc *p, struct socket *so, struct sockaddr **psa,
    int flags, struct mbuf **mp, struct mbuf **nextrecordp, int canwait)
{
	int error = 0;
	struct mbuf *m = *mp;
	struct mbuf *nextrecord = *nextrecordp;

	KASSERT(m->m_type == MT_SONAME, ("receive 1a"));
#if CONFIG_MACF_SOCKET_SUBSET
	  
                                                          
                                                            
    
	if (p != kernproc && !(so->so_state & SS_ISCONNECTED)) {
		struct mbuf *m0 = m;
		  
                                                       
                                                     
                                                     
                                                   
                                                    
                                                     
                                                     
     
		do {
			m->m_nextpkt = NULL;
			sbfree(&so->so_rcv, m);
			m = m->m_next;
		} while (m != NULL);
		m = m0;
		so->so_rcv.sb_mb = nextrecord;
		SB_EMPTY_FIXUP(&so->so_rcv);
		SBLASTRECORDCHK(&so->so_rcv, "soreceive 1a");
		SBLASTMBUFCHK(&so->so_rcv, "soreceive 1a");
		socket_unlock(so, 0);

		error = mac_socket_check_received(kauth_cred_get(), so,
		    mtod(m, struct sockaddr *));

		if (error != 0) {
			  
                                              
                                             
                                                
                                               
                          
      
			m_freem(m);
			  
                                                
                                              
      
			socket_lock(so, 0);
			sbunlock(&so->so_rcv, TRUE);                  
			error = ERESTART;
			goto done;
		}
		socket_lock(so, 0);
		  
                                               
     
		if (so->so_flags & SOF_DEFUNCT) {
			m_freem(m);
			error = ENOTCONN;
			goto done;
		}
		  
                                                     
                                                      
                                             
     
		for (m = m0; m->m_next != NULL; m = m->m_next) {
			sballoc(&so->so_rcv, m);
		}
		sballoc(&so->so_rcv, m);
		if (so->so_rcv.sb_mb == NULL) {
			so->so_rcv.sb_lastrecord = m0;
			so->so_rcv.sb_mbtail = m;
		}
		m = m0;
		nextrecord = m->m_nextpkt = so->so_rcv.sb_mb;
		so->so_rcv.sb_mb = m;
		SBLASTRECORDCHK(&so->so_rcv, "soreceive 1b");
		SBLASTMBUFCHK(&so->so_rcv, "soreceive 1b");
	}
#endif                                
	if (psa != NULL) {
		*psa = dup_sockaddr(mtod(m, struct sockaddr *), canwait);
		if ((*psa == NULL) && (flags & MSG_NEEDSA)) {
			error = EWOULDBLOCK;
			goto done;
		}
	}
	if (flags & MSG_PEEK) {
		m = m->m_next;
	} else {
		sbfree(&so->so_rcv, m);
		if (m->m_next == NULL && so->so_rcv.sb_cc != 0) {
			panic("%s: about to create invalid socketbuf",
			    __func__);
			                
		}
		MFREE(m, so->so_rcv.sb_mb);
		m = so->so_rcv.sb_mb;
		if (m != NULL) {
			m->m_nextpkt = nextrecord;
		} else {
			so->so_rcv.sb_mb = nextrecord;
			SB_EMPTY_FIXUP(&so->so_rcv);
		}
	}
done:
	*mp = m;
	*nextrecordp = nextrecord;

	return error;
}

  
                                                                           
                                                                   
   
static void
sopeek_scm_rights(struct mbuf *rights)
{
	struct cmsghdr *cm = mtod(rights, struct cmsghdr *);

	if (cm->cmsg_type == SCM_RIGHTS) {
		memset(cm + 1, 0, cm->cmsg_len - sizeof(*cm));
	}
}

  
                                                                     
                                                                 
                                                                 
                           
   
static int
soreceive_ctl(struct socket *so, struct mbuf **controlp, int flags,
    struct mbuf **mp, struct mbuf **nextrecordp)
{
	int error = 0;
	struct mbuf *cm = NULL, *cmn;
	struct mbuf **cme = &cm;
	struct sockbuf *sb_rcv = &so->so_rcv;
	struct mbuf **msgpcm = NULL;
	struct mbuf *m = *mp;
	struct mbuf *nextrecord = *nextrecordp;
	struct protosw *pr = so->so_proto;

	  
                                                          
                                                         
                                                            
                                                        
                                                         
                           
    
	do {
		if (flags & MSG_PEEK) {
			if (controlp != NULL) {
				if (*controlp == NULL) {
					msgpcm = controlp;
				}
				*controlp = m_copy(m, 0, m->m_len);

				  
                                        
                                       
                                     
                                      
                              
                     
       
				if (*controlp == NULL) {
					m_freem(*msgpcm);
					error = ENOBUFS;
					goto done;
				}

				sopeek_scm_rights(*controlp);

				controlp = &(*controlp)->m_next;
			}
			m = m->m_next;
		} else {
			m->m_nextpkt = NULL;
			sbfree(sb_rcv, m);
			sb_rcv->sb_mb = m->m_next;
			m->m_next = NULL;
			*cme = m;
			cme = &(*cme)->m_next;
			m = sb_rcv->sb_mb;
		}
	} while (m != NULL && m->m_type == MT_CONTROL);

	if (!(flags & MSG_PEEK)) {
		if (sb_rcv->sb_mb != NULL) {
			sb_rcv->sb_mb->m_nextpkt = nextrecord;
		} else {
			sb_rcv->sb_mb = nextrecord;
			SB_EMPTY_FIXUP(sb_rcv);
		}
		if (nextrecord == NULL) {
			sb_rcv->sb_lastrecord = m;
		}
	}

	SBLASTRECORDCHK(&so->so_rcv, "soreceive ctl");
	SBLASTMBUFCHK(&so->so_rcv, "soreceive ctl");

	while (cm != NULL) {
		int cmsg_type;

		cmn = cm->m_next;
		cm->m_next = NULL;
		cmsg_type = mtod(cm, struct cmsghdr *)->cmsg_type;

		  
                                                        
                                                       
                                                        
                                                     
                                                    
     
		if (pr->pr_domain->dom_externalize != NULL &&
		    cmsg_type == SCM_RIGHTS) {
			  
                                             
                                                  
                                                  
                                                
                                     
      
			socket_unlock(so, 0);
			error = (*pr->pr_domain->dom_externalize)(cm);
			socket_lock(so, 0);
		} else {
			error = 0;
		}

		if (controlp != NULL && error == 0) {
			*controlp = cm;
			controlp = &(*controlp)->m_next;
		} else {
			(void) m_free(cm);
		}
		cm = cmn;
	}
	  
                                                          
                                                  
                             
    
	if (m != NULL) {
		nextrecord = sb_rcv->sb_mb->m_nextpkt;
	} else {
		nextrecord = sb_rcv->sb_mb;
	}

done:
	*mp = m;
	*nextrecordp = nextrecord;

	return error;
}

  
                                                           
                               
                                                             
                                                               
                                                          
                               
                                                                     
                                                                 
                                                              
   
static boolean_t
so_should_wait(struct socket *so, struct uio *uio, struct mbuf *m, int flags)
{
	struct protosw *pr = so->so_proto;

	                                          
	if (m == NULL) {
		return true;
	}

	                                                                        
	if ((flags & MSG_DONTWAIT) == 0 && so->so_rcv.sb_cc < uio_resid(uio) &&
	    m->m_nextpkt == NULL && (pr->pr_flags & PR_ATOMIC) == 0) {
		  
                                                                
                             
     
		if (so->so_rcv.sb_cc < so->so_rcv.sb_lowat) {
			return true;
		}

		  
                                                            
                                                           
              
     
		if ((flags & MSG_WAITALL) && uio_resid(uio) <= so->so_rcv.sb_hiwat) {
			return true;
		}
	}

	return false;
}

  
                                            
                                                             
                                                                          
                                                           
                                                                   
                                       
                                                                          
                                                       
                                                                  
                                                                         
  
                                                                      
                                                                      
                                   
  
                       
           
            
               
                 
                     
               
               
               
                        
                           
                                
                   
                                                 
                                                
                                   
  
                                                                      
                                                               
                                               
   
int
soreceive(struct socket *so, struct sockaddr **psa, struct uio *uio,
    struct mbuf **mp0, struct mbuf **controlp, int *flagsp)
{
	struct mbuf *m, **mp, *ml = NULL;
	struct mbuf *nextrecord, *free_list;
	int flags, error, offset;
	user_ssize_t len;
	struct protosw *pr = so->so_proto;
	int moff, type = 0;
	user_ssize_t orig_resid = uio_resid(uio);
	user_ssize_t delayed_copy_len;
	int can_delay;
	struct proc *p = current_proc();
	boolean_t en_tracing = FALSE;

	  
                                                                      
               
    
	if (orig_resid < 0 || orig_resid > INT_MAX) {
		return EINVAL;
	}

	KERNEL_DEBUG(DBG_FNC_SORECEIVE | DBG_FUNC_START, so,
	    uio_resid(uio), so->so_rcv.sb_cc, so->so_rcv.sb_lowat,
	    so->so_rcv.sb_hiwat);

	socket_lock(so, 1);
	so_update_last_owner_locked(so, p);
	so_update_policy(so);

#ifdef MORE_LOCKING_DEBUG
	if (so->so_usecount == 1) {
		panic("%s: so=%x no other reference on socket\n", __func__, so);
		                
	}
#endif
	mp = mp0;
	if (psa != NULL) {
		*psa = NULL;
	}
	if (controlp != NULL) {
		*controlp = NULL;
	}
	if (flagsp != NULL) {
		flags = *flagsp & ~MSG_EOR;
	} else {
		flags = 0;
	}

	  
                                                             
                                                           
                
    
	if (so->so_flags & SOF_DEFUNCT) {
		struct sockbuf *sb = &so->so_rcv;

		error = ENOTCONN;
		SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] (%d)\n",
		    __func__, proc_pid(p), proc_best_name(p),
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    SOCK_DOM(so), SOCK_TYPE(so), error);
		  
                                                          
                                                           
                                                        
     
		if (so->so_state & SS_DEFUNCT) {
			sb_empty_assert(sb, __func__);
		}
		socket_unlock(so, 1);
		return error;
	}

	if ((so->so_flags1 & SOF1_PRECONNECT_DATA) &&
	    pr->pr_usrreqs->pru_preconnect) {
		  
                                                                 
                                                                
                                                          
                                                             
     
		error = (*pr->pr_usrreqs->pru_preconnect)(so);

		if (error) {
			socket_unlock(so, 1);
			return error;
		}
	}

	if (ENTR_SHOULDTRACE &&
	    (SOCK_CHECK_DOM(so, AF_INET) || SOCK_CHECK_DOM(so, AF_INET6))) {
		  
                                                        
                                  
     
		struct inpcb *inp = sotoinpcb(so);
		if (inp->inp_last_outifp != NULL &&
		    !(inp->inp_last_outifp->if_flags & IFF_LOOPBACK)) {
			en_tracing = TRUE;
			KERNEL_ENERGYTRACE(kEnTrActKernSockRead, DBG_FUNC_START,
			    VM_KERNEL_ADDRPERM(so),
			    ((so->so_state & SS_NBIO) ?
			    kEnTrFlagNonBlocking : 0),
			    (int64_t)orig_resid);
		}
	}

	  
                                                             
                                                           
                                   
    
	if ((flags & MSG_OOB) ||
	    ((so->so_options & SO_WANTOOBFLAG) != 0 &&
	    (so->so_options & SO_OOBINLINE) == 0 &&
	    (so->so_oobmark || (so->so_state & SS_RCVATMARK)))) {
		m = m_get(M_WAIT, MT_DATA);
		if (m == NULL) {
			socket_unlock(so, 1);
			KERNEL_DEBUG(DBG_FNC_SORECEIVE | DBG_FUNC_END,
			    ENOBUFS, 0, 0, 0, 0);
			return ENOBUFS;
		}
		error = (*pr->pr_usrreqs->pru_rcvoob)(so, m, flags & MSG_PEEK);
		if (error) {
			goto bad;
		}
		socket_unlock(so, 0);
		do {
			error = uiomove(mtod(m, caddr_t),
			    imin(uio_resid(uio), m->m_len), uio);
			m = m_free(m);
		} while (uio_resid(uio) && error == 0 && m != NULL);
		socket_lock(so, 0);
bad:
		if (m != NULL) {
			m_freem(m);
		}

		if ((so->so_options & SO_WANTOOBFLAG) != 0) {
			if (error == EWOULDBLOCK || error == EINVAL) {
				  
                                    
                                        
                                            
                    
       
				error = 0;
				goto nooob;
			} else if (error == 0 && flagsp != NULL) {
				*flagsp |= MSG_OOB;
			}
		}
		socket_unlock(so, 1);
		if (en_tracing) {
			KERNEL_ENERGYTRACE(kEnTrActKernSockRead, DBG_FUNC_END,
			    VM_KERNEL_ADDRPERM(so), 0,
			    (int64_t)(orig_resid - uio_resid(uio)));
		}
		KERNEL_DEBUG(DBG_FNC_SORECEIVE | DBG_FUNC_END, error,
		    0, 0, 0, 0);

		return error;
	}
nooob:
	if (mp != NULL) {
		*mp = NULL;
	}

	if (so->so_state & SS_ISCONFIRMING && uio_resid(uio)) {
		(*pr->pr_usrreqs->pru_rcvd)(so, 0);
	}

	free_list = NULL;
	delayed_copy_len = 0;
restart:
#ifdef MORE_LOCKING_DEBUG
	if (so->so_usecount <= 1) {
		printf("soreceive: sblock so=0x%llx ref=%d on socket\n",
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so), so->so_usecount);
	}
#endif
	  
                                                                 
                                                                
                                                                
                                                                
                                                                 
                                                                
                                                                
                                                                   
                                                               
                          
   
                                                               
                                                              
                                                               
    
	if ((so->so_state & (SS_NOFDREF | SS_CANTRCVMORE)) ==
	    (SS_NOFDREF | SS_CANTRCVMORE) && !(so->so_flags & SOF_MP_SUBFLOW)) {
		socket_unlock(so, 1);
		return 0;
	}

	error = sblock(&so->so_rcv, SBLOCKWAIT(flags));
	if (error) {
		socket_unlock(so, 1);
		KERNEL_DEBUG(DBG_FNC_SORECEIVE | DBG_FUNC_END, error,
		    0, 0, 0, 0);
		if (en_tracing) {
			KERNEL_ENERGYTRACE(kEnTrActKernSockRead, DBG_FUNC_END,
			    VM_KERNEL_ADDRPERM(so), 0,
			    (int64_t)(orig_resid - uio_resid(uio)));
		}
		return error;
	}

	m = so->so_rcv.sb_mb;
	if (so_should_wait(so, uio, m, flags)) {
		  
                                                       
                                                        
                                                       
                                                          
                                                    
     
		SB_MB_CHECK(&so->so_rcv);

		if (so->so_error) {
			if (m != NULL) {
				goto dontblock;
			}
			error = so->so_error;
			if ((flags & MSG_PEEK) == 0) {
				so->so_error = 0;
			}
			goto release;
		}
		if (so->so_state & SS_CANTRCVMORE) {
#if CONTENT_FILTER
			  
                                       
      
			if ((so->so_state & SS_ISDISCONNECTED) == 0 &&
			    cfil_sock_data_pending(&so->so_rcv) != 0) {
				CFIL_LOG(LOG_INFO,
				    "so %llx ignore SS_CANTRCVMORE",
				    (uint64_t)DEBUG_KERNEL_ADDRPERM(so));
			} else
#endif                     
			if (m != NULL) {
				goto dontblock;
			} else {
				goto release;
			}
		}
		for (; m != NULL; m = m->m_next) {
			if (m->m_type == MT_OOBDATA || (m->m_flags & M_EOR)) {
				m = so->so_rcv.sb_mb;
				goto dontblock;
			}
		}
		if ((so->so_state & (SS_ISCONNECTED | SS_ISCONNECTING)) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED)) {
			error = ENOTCONN;
			goto release;
		}
		if (uio_resid(uio) == 0) {
			goto release;
		}

		if ((so->so_state & SS_NBIO) ||
		    (flags & (MSG_DONTWAIT | MSG_NBIO))) {
			error = EWOULDBLOCK;
			goto release;
		}
		SBLASTRECORDCHK(&so->so_rcv, "soreceive sbwait 1");
		SBLASTMBUFCHK(&so->so_rcv, "soreceive sbwait 1");
		sbunlock(&so->so_rcv, TRUE);                            
#if EVEN_MORE_LOCKING_DEBUG
		if (socket_debug) {
			printf("Waiting for socket data\n");
		}
#endif

		  
                                                        
                                                        
                                                        
                                                       
                                                      
                                                              
          
     
		if ((pr->pr_flags & PR_WANTRCVD) && so->so_pcb != NULL) {
			(*pr->pr_usrreqs->pru_rcvd)(so, flags);
		}

		error = 0;
		if (so_should_wait(so, uio, so->so_rcv.sb_mb, flags)) {
			error = sbwait(&so->so_rcv);
		}

#if EVEN_MORE_LOCKING_DEBUG
		if (socket_debug) {
			printf("SORECEIVE - sbwait returned %d\n", error);
		}
#endif
		if (so->so_usecount < 1) {
			panic("%s: after 2nd sblock so=%p ref=%d on socket\n",
			    __func__, so, so->so_usecount);
			                
		}
		if (error) {
			socket_unlock(so, 1);
			KERNEL_DEBUG(DBG_FNC_SORECEIVE | DBG_FUNC_END, error,
			    0, 0, 0, 0);
			if (en_tracing) {
				KERNEL_ENERGYTRACE(kEnTrActKernSockRead, DBG_FUNC_END,
				    VM_KERNEL_ADDRPERM(so), 0,
				    (int64_t)(orig_resid - uio_resid(uio)));
			}
			return error;
		}
		goto restart;
	}
dontblock:
	OSIncrementAtomicLong(&p->p_stats->p_ru.ru_msgrcv);
	SBLASTRECORDCHK(&so->so_rcv, "soreceive 1");
	SBLASTMBUFCHK(&so->so_rcv, "soreceive 1");
	nextrecord = m->m_nextpkt;

	if ((pr->pr_flags & PR_ADDR) && m->m_type == MT_SONAME) {
		error = soreceive_addr(p, so, psa, flags, &m, &nextrecord,
		    mp0 == NULL);
		if (error == ERESTART) {
			goto restart;
		} else if (error != 0) {
			goto release;
		}
		orig_resid = 0;
	}

	  
                                                                      
                                                                  
                                                                  
                            
    
	if (m != NULL && m->m_type == MT_CONTROL) {
		error = soreceive_ctl(so, controlp, flags, &m, &nextrecord);
		if (error != 0) {
			goto release;
		}
		orig_resid = 0;
	}

	if (m != NULL) {
		if (!(flags & MSG_PEEK)) {
			  
                                                       
                                                       
                                                         
                                                       
                                                     
                                       
      
			if (m != so->so_rcv.sb_mb ||
			    m->m_nextpkt != nextrecord) {
				panic("%s: post-control !sync so=%p m=%p "
				    "nextrecord=%p\n", __func__, so, m,
				    nextrecord);
				                
			}
			if (nextrecord == NULL) {
				so->so_rcv.sb_lastrecord = m;
			}
		}
		type = m->m_type;
		if (type == MT_OOBDATA) {
			flags |= MSG_OOB;
		}
	} else {
		if (!(flags & MSG_PEEK)) {
			SB_EMPTY_FIXUP(&so->so_rcv);
		}
	}
	SBLASTRECORDCHK(&so->so_rcv, "soreceive 2");
	SBLASTMBUFCHK(&so->so_rcv, "soreceive 2");

	moff = 0;
	offset = 0;

	if (!(flags & MSG_PEEK) && uio_resid(uio) > sorecvmincopy) {
		can_delay = 1;
	} else {
		can_delay = 0;
	}

	while (m != NULL &&
	    (uio_resid(uio) - delayed_copy_len) > 0 && error == 0) {
		if (m->m_type == MT_OOBDATA) {
			if (type != MT_OOBDATA) {
				break;
			}
		} else if (type == MT_OOBDATA) {
			break;
		}

		if (m->m_type != MT_OOBDATA && m->m_type != MT_DATA &&
		    m->m_type != MT_HEADER) {
			break;
		}
		  
                                                        
                             
     
		if ((so->so_options & SO_WANTOOBFLAG) != 0 &&
		    (so->so_options & SO_OOBINLINE) != 0 &&
		    (so->so_state & SS_RCVATMARK) != 0) {
			flags |= MSG_OOB;
		}
		so->so_state &= ~SS_RCVATMARK;
		len = uio_resid(uio) - delayed_copy_len;
		if (so->so_oobmark && len > so->so_oobmark - offset) {
			len = so->so_oobmark - offset;
		}
		if (len > m->m_len - moff) {
			len = m->m_len - moff;
		}
		  
                                            
                                                    
                                                             
                                                     
                                                      
                            
     
		if (mp == NULL) {
			SBLASTRECORDCHK(&so->so_rcv, "soreceive uiomove");
			SBLASTMBUFCHK(&so->so_rcv, "soreceive uiomove");
			if (can_delay && len == m->m_len) {
				  
                                                 
                                          
                                                   
                                               
                                         
                                             
                     
       
				delayed_copy_len += len;
			} else {
				if (delayed_copy_len) {
					error = sodelayed_copy(so, uio,
					    &free_list, &delayed_copy_len);

					if (error) {
						goto release;
					}
					  
                                            
                                            
                                             
                                      
                                             
                                           
                                          
                                             
                                             
                                           
                      
        
					if (m != so->so_rcv.sb_mb) {
						break;
					}
				}
				socket_unlock(so, 0);
				error = uiomove(mtod(m, caddr_t) + moff,
				    (int)len, uio);
				socket_lock(so, 0);

				if (error) {
					goto release;
				}
			}
		} else {
			uio_setresid(uio, (uio_resid(uio) - len));
		}
		if (len == m->m_len - moff) {
			if (m->m_flags & M_EOR) {
				flags |= MSG_EOR;
			}
			if (flags & MSG_PEEK) {
				m = m->m_next;
				moff = 0;
			} else {
				nextrecord = m->m_nextpkt;
				sbfree(&so->so_rcv, m);
				m->m_nextpkt = NULL;

				if (mp != NULL) {
					*mp = m;
					mp = &m->m_next;
					so->so_rcv.sb_mb = m = m->m_next;
					*mp = NULL;
				} else {
					if (free_list == NULL) {
						free_list = m;
					} else {
						ml->m_next = m;
					}
					ml = m;
					so->so_rcv.sb_mb = m = m->m_next;
					ml->m_next = NULL;
				}
				if (m != NULL) {
					m->m_nextpkt = nextrecord;
					if (nextrecord == NULL) {
						so->so_rcv.sb_lastrecord = m;
					}
				} else {
					so->so_rcv.sb_mb = nextrecord;
					SB_EMPTY_FIXUP(&so->so_rcv);
				}
				SBLASTRECORDCHK(&so->so_rcv, "soreceive 3");
				SBLASTMBUFCHK(&so->so_rcv, "soreceive 3");
			}
		} else {
			if (flags & MSG_PEEK) {
				moff += len;
			} else {
				if (mp != NULL) {
					int copy_flag;

					if (flags & MSG_DONTWAIT) {
						copy_flag = M_DONTWAIT;
					} else {
						copy_flag = M_WAIT;
					}
					*mp = m_copym(m, 0, len, copy_flag);
					  
                                   
                                     
                                        
                            
        
					if (*mp == NULL) {
						uio_setresid(uio,
						    (uio_resid(uio) + len));
						break;
					}
				}
				m->m_data += len;
				m->m_len -= len;
				so->so_rcv.sb_cc -= len;
			}
		}
		if (so->so_oobmark) {
			if ((flags & MSG_PEEK) == 0) {
				so->so_oobmark -= len;
				if (so->so_oobmark == 0) {
					so->so_state |= SS_RCVATMARK;
					break;
				}
			} else {
				offset += len;
				if (offset == so->so_oobmark) {
					break;
				}
			}
		}
		if (flags & MSG_EOR) {
			break;
		}
		  
                                                     
                                                    
                                                   
                                                    
                                                  
                           
     
		while (flags & (MSG_WAITALL | MSG_WAITSTREAM) && m == NULL &&
		    (uio_resid(uio) - delayed_copy_len) > 0 &&
		    !sosendallatonce(so) && !nextrecord) {
			if (so->so_error || ((so->so_state & SS_CANTRCVMORE)
#if CONTENT_FILTER
			    && cfil_sock_data_pending(&so->so_rcv) == 0
#endif                     
			    )) {
				goto release;
			}

			  
                                                         
                                                         
                                                         
                                                        
                                                       
                                                        
                                             
      
			if ((pr->pr_flags & PR_WANTRCVD) && so->so_pcb != NULL) {
				(*pr->pr_usrreqs->pru_rcvd)(so, flags);
			}

			SBLASTRECORDCHK(&so->so_rcv, "soreceive sbwait 2");
			SBLASTMBUFCHK(&so->so_rcv, "soreceive sbwait 2");

			if (so->so_rcv.sb_mb == NULL && sbwait(&so->so_rcv)) {
				error = 0;
				goto release;
			}
			  
                                                          
                                                        
                                                           
                                                      
                                                          
                                                          
                                                         
                                                           
                                                           
                                                        
                                                         
           
     
                                                         
                                                          
                                                    
      
			if (delayed_copy_len > sorecvmincopy &&
			    (delayed_copy_len > (so->so_rcv.sb_hiwat / 2))) {
				error = sodelayed_copy(so, uio,
				    &free_list, &delayed_copy_len);

				if (error) {
					goto release;
				}
			}
			m = so->so_rcv.sb_mb;
			if (m != NULL) {
				nextrecord = m->m_nextpkt;
			}
			SB_MB_CHECK(&so->so_rcv);
		}
	}
#ifdef MORE_LOCKING_DEBUG
	if (so->so_usecount <= 1) {
		panic("%s: after big while so=%p ref=%d on socket\n",
		    __func__, so, so->so_usecount);
		                
	}
#endif

	if (m != NULL && pr->pr_flags & PR_ATOMIC) {
		if (so->so_options & SO_DONTTRUNC) {
			flags |= MSG_RCVMORE;
		} else {
			flags |= MSG_TRUNC;
			if ((flags & MSG_PEEK) == 0) {
				(void) sbdroprecord(&so->so_rcv);
			}
		}
	}

	  
                                                               
                                                                
                                                            
                                                               
                                                             
                              
    
	if ((so->so_options & SO_WANTMORE) && so->so_rcv.sb_cc > 0) {
		flags |= MSG_HAVEMORE;
	}

	if ((flags & MSG_PEEK) == 0) {
		if (m == NULL) {
			so->so_rcv.sb_mb = nextrecord;
			  
                                                       
                                                    
                                               
      
			if (so->so_rcv.sb_mb == NULL) {
				so->so_rcv.sb_mbtail = NULL;
				so->so_rcv.sb_lastrecord = NULL;
			} else if (nextrecord->m_nextpkt == NULL) {
				so->so_rcv.sb_lastrecord = nextrecord;
			}
			SB_MB_CHECK(&so->so_rcv);
		}
		SBLASTRECORDCHK(&so->so_rcv, "soreceive 4");
		SBLASTMBUFCHK(&so->so_rcv, "soreceive 4");
		if (pr->pr_flags & PR_WANTRCVD && so->so_pcb) {
			(*pr->pr_usrreqs->pru_rcvd)(so, flags);
		}
	}

	if (delayed_copy_len) {
		error = sodelayed_copy(so, uio, &free_list, &delayed_copy_len);
		if (error) {
			goto release;
		}
	}
	if (free_list != NULL) {
		m_freem_list(free_list);
		free_list = NULL;
	}

	if (orig_resid == uio_resid(uio) && orig_resid &&
	    (flags & MSG_EOR) == 0 && (so->so_state & SS_CANTRCVMORE) == 0) {
		sbunlock(&so->so_rcv, TRUE);                            
		goto restart;
	}

	if (flagsp != NULL) {
		*flagsp |= flags;
	}
release:
#ifdef MORE_LOCKING_DEBUG
	if (so->so_usecount <= 1) {
		panic("%s: release so=%p ref=%d on socket\n", __func__,
		    so, so->so_usecount);
		                
	}
#endif
	if (delayed_copy_len) {
		error = sodelayed_copy(so, uio, &free_list, &delayed_copy_len);
	}

	if (free_list != NULL) {
		m_freem_list(free_list);
	}

	sbunlock(&so->so_rcv, FALSE);                           

	if (en_tracing) {
		KERNEL_ENERGYTRACE(kEnTrActKernSockRead, DBG_FUNC_END,
		    VM_KERNEL_ADDRPERM(so),
		    ((error == EWOULDBLOCK) ? kEnTrFlagNoWork : 0),
		    (int64_t)(orig_resid - uio_resid(uio)));
	}
	KERNEL_DEBUG(DBG_FNC_SORECEIVE | DBG_FUNC_END, so, uio_resid(uio),
	    so->so_rcv.sb_cc, 0, error);

	return error;
}

  
                       
                 
   
static int
sodelayed_copy(struct socket *so, struct uio *uio, struct mbuf **free_list,
    user_ssize_t *resid)
{
	int error = 0;
	struct mbuf *m;

	m = *free_list;

	socket_unlock(so, 0);

	while (m != NULL && error == 0) {
		error = uiomove(mtod(m, caddr_t), (int)m->m_len, uio);
		m = m->m_next;
	}
	m_freem_list(*free_list);

	*free_list = NULL;
	*resid = 0;

	socket_lock(so, 0);

	return error;
}

static int
sodelayed_copy_list(struct socket *so, struct recv_msg_elem *msgarray,
    u_int uiocnt, struct mbuf **free_list, user_ssize_t *resid)
{
#pragma unused(so)
	int error = 0;
	struct mbuf *ml, *m;
	int i = 0;
	struct uio *auio;

	for (ml = *free_list, i = 0; ml != NULL && i < uiocnt;
	    ml = ml->m_nextpkt, i++) {
		auio = msgarray[i].uio;
		for (m = ml; m != NULL; m = m->m_next) {
			error = uiomove(mtod(m, caddr_t), m->m_len, auio);
			if (error != 0) {
				goto out;
			}
		}
	}
out:
	m_freem_list(*free_list);

	*free_list = NULL;
	*resid = 0;

	return error;
}

int
soreceive_list(struct socket *so, struct recv_msg_elem *msgarray, u_int uiocnt,
    int *flagsp)
{
	struct mbuf *m;
	struct mbuf *nextrecord;
	struct mbuf *ml = NULL, *free_list = NULL, *free_tail = NULL;
	int error;
	user_ssize_t len, pktlen, delayed_copy_len = 0;
	struct protosw *pr = so->so_proto;
	user_ssize_t resid;
	struct proc *p = current_proc();
	struct uio *auio = NULL;
	int npkts = 0;
	int sblocked = 0;
	struct sockaddr **psa = NULL;
	struct mbuf **controlp = NULL;
	int can_delay;
	int flags;
	struct mbuf *free_others = NULL;

	KERNEL_DEBUG(DBG_FNC_SORECEIVE_LIST | DBG_FUNC_START,
	    so, uiocnt,
	    so->so_rcv.sb_cc, so->so_rcv.sb_lowat, so->so_rcv.sb_hiwat);

	  
                  
                                    
                                                              
                    
                                         
                                              
    
	if (flagsp != NULL) {
		flags = *flagsp;
	} else {
		flags = 0;
	}
	if (flags & ~(MSG_PEEK | MSG_WAITALL | MSG_DONTWAIT | MSG_NEEDSA |
	    MSG_NBIO)) {
		printf("%s invalid flags 0x%x\n", __func__, flags);
		error = EINVAL;
		goto out;
	}
	if (so->so_type != SOCK_DGRAM) {
		error = EINVAL;
		goto out;
	}
	if (sosendallatonce(so) == 0) {
		error = EINVAL;
		goto out;
	}
	if (so->so_proto->pr_usrreqs->pru_send_list == NULL) {
		error = EPROTONOSUPPORT;
		goto out;
	}
	if (msgarray == NULL) {
		printf("%s uioarray is NULL\n", __func__);
		error = EINVAL;
		goto out;
	}
	if (uiocnt == 0) {
		printf("%s uiocnt is 0\n", __func__);
		error = EINVAL;
		goto out;
	}
	  
                                                                      
               
    
	resid = recv_msg_array_resid(msgarray, uiocnt);
	if (resid < 0 || resid > INT_MAX) {
		error = EINVAL;
		goto out;
	}

	if (!(flags & MSG_PEEK) && sorecvmincopy > 0) {
		can_delay = 1;
	} else {
		can_delay = 0;
	}

	socket_lock(so, 1);
	so_update_last_owner_locked(so, p);
	so_update_policy(so);

#if NECP
	so_update_necp_policy(so, NULL, NULL);
#endif           

	  
                                                             
                                                           
                
    
	if (so->so_flags & SOF_DEFUNCT) {
		struct sockbuf *sb = &so->so_rcv;

		error = ENOTCONN;
		SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] (%d)\n",
		    __func__, proc_pid(p), proc_best_name(p),
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    SOCK_DOM(so), SOCK_TYPE(so), error);
		  
                                                          
                                                           
                                                        
     
		if (so->so_state & SS_DEFUNCT) {
			sb_empty_assert(sb, __func__);
		}
		goto release;
	}

next:
	  
                        
    
	if (npkts >= uiocnt) {
		error = 0;
		goto release;
	}
restart:
	  
                                                                 
                                                                
                                                                
                                                                
                                                                 
                                                                
                                                                
                                                                   
                                                               
                          
    
	if ((so->so_state & (SS_NOFDREF | SS_CANTRCVMORE)) ==
	    (SS_NOFDREF | SS_CANTRCVMORE)) {
		error = 0;
		goto release;
	}

	error = sblock(&so->so_rcv, SBLOCKWAIT(flags));
	if (error) {
		goto release;
	}
	sblocked = 1;

	m = so->so_rcv.sb_mb;
	  
                                          
    
	if (m == NULL || (((flags & MSG_DONTWAIT) == 0 &&
	    (so->so_rcv.sb_cc < so->so_rcv.sb_lowat ||
	    ((flags & MSG_WAITALL) && npkts < uiocnt))))) {
		  
                                                       
                                                        
                                                       
                                                          
                                                    
     
		SB_MB_CHECK(&so->so_rcv);

		if (so->so_error) {
			error = so->so_error;
			if ((flags & MSG_PEEK) == 0) {
				so->so_error = 0;
			}
			goto release;
		}
		if (so->so_state & SS_CANTRCVMORE) {
			goto release;
		}
		if ((so->so_state & (SS_ISCONNECTED | SS_ISCONNECTING)) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED)) {
			error = ENOTCONN;
			goto release;
		}
		if ((so->so_state & SS_NBIO) ||
		    (flags & (MSG_DONTWAIT | MSG_NBIO))) {
			error = EWOULDBLOCK;
			goto release;
		}
		  
                                     
     
		if (free_list != NULL) {
			error = 0;
			goto release;
		}

		SBLASTRECORDCHK(&so->so_rcv, "soreceive sbwait 1");
		SBLASTMBUFCHK(&so->so_rcv, "soreceive sbwait 1");

		sbunlock(&so->so_rcv, TRUE);                            
		sblocked = 0;

		error = sbwait(&so->so_rcv);
		if (error) {
			goto release;
		}
		goto restart;
	}

	OSIncrementAtomicLong(&p->p_stats->p_ru.ru_msgrcv);
	SBLASTRECORDCHK(&so->so_rcv, "soreceive 1");
	SBLASTMBUFCHK(&so->so_rcv, "soreceive 1");

	  
                                                       
    
	auio = msgarray[npkts].uio;
	resid = uio_resid(auio);
	msgarray[npkts].which |= SOCK_MSG_DATA;
	psa = (msgarray[npkts].which & SOCK_MSG_SA) ?
	    &msgarray[npkts].psa : NULL;
	controlp = (msgarray[npkts].which & SOCK_MSG_CONTROL) ?
	    &msgarray[npkts].controlp : NULL;
	npkts += 1;
	nextrecord = m->m_nextpkt;

	if ((pr->pr_flags & PR_ADDR) && m->m_type == MT_SONAME) {
		error = soreceive_addr(p, so, psa, flags, &m, &nextrecord, 1);
		if (error == ERESTART) {
			goto restart;
		} else if (error != 0) {
			goto release;
		}
	}

	if (m != NULL && m->m_type == MT_CONTROL) {
		error = soreceive_ctl(so, controlp, flags, &m, &nextrecord);
		if (error != 0) {
			goto release;
		}
	}

	if (m->m_pkthdr.len == 0) {
		printf("%s:%d so %llx pkt %llx type %u pktlen null\n",
		    __func__, __LINE__,
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(m),
		    m->m_type);
	}

	  
                                                
                               
    
	ml = NULL;
	pktlen = 0;
	while (m != NULL && (len = resid - pktlen) >= 0 && error == 0) {
		if (m->m_len == 0) {
			panic("%p m_len zero", m);
		}
		if (m->m_type == 0) {
			panic("%p m_type zero", m);
		}
		  
                                
     
		if (len > m->m_len) {
			len = m->m_len;
		}
		pktlen += len;
		  
                                                 
                                                             
                                                     
                                                      
                            
     
		if (len > 0 && can_delay == 0) {
			socket_unlock(so, 0);
			error = uiomove(mtod(m, caddr_t), (int)len, auio);
			socket_lock(so, 0);
			if (error) {
				goto release;
			}
		} else {
			delayed_copy_len += len;
		}

		if (len == m->m_len) {
			  
                           
      
			sbfree(&so->so_rcv, m);
			nextrecord = m->m_nextpkt;
			m->m_nextpkt = NULL;

			  
                                                       
      
			if (free_list == NULL) {
				free_list = m;
			}
			  
                                              
      
			if (ml == NULL) {
				if (free_tail != NULL) {
					free_tail->m_nextpkt = m;
				}
				free_tail = m;
			}
			  
                                                      
      
			if (ml != NULL) {
				ml->m_next = m;
			}
			ml = m;

			  
                                            
      
			so->so_rcv.sb_mb = m = ml->m_next;
			ml->m_next = NULL;

			if (m != NULL) {
				m->m_nextpkt = nextrecord;
				if (nextrecord == NULL) {
					so->so_rcv.sb_lastrecord = m;
				}
			} else {
				so->so_rcv.sb_mb = nextrecord;
				SB_EMPTY_FIXUP(&so->so_rcv);
			}
			SBLASTRECORDCHK(&so->so_rcv, "soreceive 3");
			SBLASTMBUFCHK(&so->so_rcv, "soreceive 3");
		} else {
			  
                                   
      
			break;
		}
	}
#ifdef MORE_LOCKING_DEBUG
	if (so->so_usecount <= 1) {
		panic("%s: after big while so=%llx ref=%d on socket\n",
		    __func__,
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so), so->so_usecount);
		                
	}
#endif
	  
                                          
    
	if (m != NULL) {
		if (so->so_options & SO_DONTTRUNC) {
			  
                                                      
      
			socket_unlock(so, 0);
			if (delayed_copy_len) {
				error = sodelayed_copy_list(so, msgarray,
				    uiocnt, &free_list, &delayed_copy_len);
			}

			if (error == 0) {
				error = uiomove(mtod(m, caddr_t), (int)len,
				    auio);
			}
			socket_lock(so, 0);
			if (error) {
				goto release;
			}

			m->m_data += len;
			m->m_len -= len;
			so->so_rcv.sb_cc -= len;
			flags |= MSG_RCVMORE;
		} else {
			(void) sbdroprecord(&so->so_rcv);
			nextrecord = so->so_rcv.sb_mb;
			m = NULL;
			flags |= MSG_TRUNC;
		}
	}

	if (m == NULL) {
		so->so_rcv.sb_mb = nextrecord;
		  
                                                      
                                                   
                                              
     
		if (so->so_rcv.sb_mb == NULL) {
			so->so_rcv.sb_mbtail = NULL;
			so->so_rcv.sb_lastrecord = NULL;
		} else if (nextrecord->m_nextpkt == NULL) {
			so->so_rcv.sb_lastrecord = nextrecord;
		}
		SB_MB_CHECK(&so->so_rcv);
	}
	SBLASTRECORDCHK(&so->so_rcv, "soreceive 4");
	SBLASTMBUFCHK(&so->so_rcv, "soreceive 4");

	  
                                                  
                                        
                        
                                
                                    
    
	if (npkts < uiocnt && error == 0 &&
	    (flags & (MSG_RCVMORE | MSG_TRUNC)) == 0 &&
	    (so->so_state & SS_CANTRCVMORE) == 0) {
		sbunlock(&so->so_rcv, TRUE);                            
		sblocked = 0;

		goto next;
	}
	if (flagsp != NULL) {
		*flagsp |= flags;
	}

release:
	  
                                                                  
                                                                
                                                                   
                   
    
	if ((so->so_options & SO_WANTMORE) && so->so_rcv.sb_cc > 0) {
		flags |= MSG_HAVEMORE;
	}

	if (pr->pr_flags & PR_WANTRCVD && so->so_pcb) {
		(*pr->pr_usrreqs->pru_rcvd)(so, flags);
	}

	if (sblocked) {
		sbunlock(&so->so_rcv, FALSE);                           
	} else {
		socket_unlock(so, 1);
	}

	if (delayed_copy_len) {
		error = sodelayed_copy_list(so, msgarray, uiocnt,
		    &free_list, &delayed_copy_len);
	}
out:
	  
                                          
    
	if (free_list != NULL) {
		m_freem_list(free_list);
	}
	if (free_others != NULL) {
		m_freem_list(free_others);
	}

	KERNEL_DEBUG(DBG_FNC_SORECEIVE_LIST | DBG_FUNC_END, error,
	    0, 0, 0, 0);
	return error;
}

static int
so_statistics_event_to_nstat_event(int64_t *input_options,
    uint64_t *nstat_event)
{
	int error = 0;
	switch (*input_options) {
	case SO_STATISTICS_EVENT_ENTER_CELLFALLBACK:
		*nstat_event = NSTAT_EVENT_SRC_ENTER_CELLFALLBACK;
		break;
	case SO_STATISTICS_EVENT_EXIT_CELLFALLBACK:
		*nstat_event = NSTAT_EVENT_SRC_EXIT_CELLFALLBACK;
		break;
#if (DEBUG || DEVELOPMENT)
	case SO_STATISTICS_EVENT_RESERVED_1:
		*nstat_event = NSTAT_EVENT_SRC_RESERVED_1;
		break;
	case SO_STATISTICS_EVENT_RESERVED_2:
		*nstat_event = NSTAT_EVENT_SRC_RESERVED_2;
		break;
#endif                             
	default:
		error = EINVAL;
		break;
	}
	return error;
}

  
                       
          
            
                        
                                    
                              
                               
                                   
                                  
                               
                             
                             
                               
                              
                                                                   
                                                
   
int
soshutdown(struct socket *so, int how)
{
	int error;

	KERNEL_DEBUG(DBG_FNC_SOSHUTDOWN | DBG_FUNC_START, how, 0, 0, 0, 0);

	switch (how) {
	case SHUT_RD:
	case SHUT_WR:
	case SHUT_RDWR:
		socket_lock(so, 1);
		if ((so->so_state &
		    (SS_ISCONNECTED | SS_ISCONNECTING | SS_ISDISCONNECTING)) == 0) {
			error = ENOTCONN;
		} else {
			error = soshutdownlock(so, how);
		}
		socket_unlock(so, 1);
		break;
	default:
		error = EINVAL;
		break;
	}

	KERNEL_DEBUG(DBG_FNC_SOSHUTDOWN | DBG_FUNC_END, how, error, 0, 0, 0);

	return error;
}

int
soshutdownlock_final(struct socket *so, int how)
{
	struct protosw *pr = so->so_proto;
	int error = 0;

	sflt_notify(so, sock_evt_shutdown, &how);

	if (how != SHUT_WR) {
		if ((so->so_state & SS_CANTRCVMORE) != 0) {
			                            
			error = ENOTCONN;
			goto done;
		}
		sorflush(so);
	}
	if (how != SHUT_RD) {
		if ((so->so_state & SS_CANTSENDMORE) != 0) {
			                             
			error = ENOTCONN;
			goto done;
		}
		error = (*pr->pr_usrreqs->pru_shutdown)(so);
	}
done:
	KERNEL_DEBUG(DBG_FNC_SOSHUTDOWN, how, 1, 0, 0, 0);
	return error;
}

int
soshutdownlock(struct socket *so, int how)
{
	int error = 0;

#if CONTENT_FILTER
	  
                                                           
                                  
    
	if (so->so_flags & SOF_CONTENT_FILTER) {
		error = cfil_sock_shutdown(so, &how);
		if (error == EJUSTRETURN) {
			error = 0;
			goto done;
		} else if (error != 0) {
			goto done;
		}
	}
#endif                     

	error = soshutdownlock_final(so, how);

done:
	return error;
}

void
sowflush(struct socket *so)
{
	struct sockbuf *sb = &so->so_snd;

	  
                                                                 
                                                                
                                                              
   
                                                     
    
	(void) sblock(sb, SBL_WAIT | SBL_NOINTR | SBL_IGNDEFUNCT);
	VERIFY(sb->sb_flags & SB_LOCK);

	sb->sb_flags            &= ~(SB_SEL | SB_UPCALL);
	sb->sb_flags            |= SB_DROP;
	sb->sb_upcall           = NULL;
	sb->sb_upcallarg        = NULL;

	sbunlock(sb, TRUE);                             

	selthreadclear(&sb->sb_sel);
	sbrelease(sb);
}

void
sorflush(struct socket *so)
{
	struct sockbuf *sb = &so->so_rcv;
	struct protosw *pr = so->so_proto;
	struct sockbuf asb;
#ifdef notyet
	lck_mtx_t *mutex_held;
	  
                                                                      
                                                                     
                                                                       
    
	if (so->so_proto->pr_getlock != NULL) {
		mutex_held = (*so->so_proto->pr_getlock)(so, 0);
	} else {
		mutex_held = so->so_proto->pr_domain->dom_mtx;
	}

	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);
#endif             

	sflt_notify(so, sock_evt_flush_read, NULL);

	socantrcvmore(so);

	  
                                                                 
                                                                
                                                              
   
                                                     
    
	(void) sblock(sb, SBL_WAIT | SBL_NOINTR | SBL_IGNDEFUNCT);
	VERIFY(sb->sb_flags & SB_LOCK);

	  
                                                             
                                                          
                                                             
                                                              
                                                            
                                               
    
	bzero(&asb, sizeof(asb));
	asb.sb_cc               = sb->sb_cc;
	asb.sb_hiwat            = sb->sb_hiwat;
	asb.sb_mbcnt            = sb->sb_mbcnt;
	asb.sb_mbmax            = sb->sb_mbmax;
	asb.sb_ctl              = sb->sb_ctl;
	asb.sb_lowat            = sb->sb_lowat;
	asb.sb_mb               = sb->sb_mb;
	asb.sb_mbtail           = sb->sb_mbtail;
	asb.sb_lastrecord       = sb->sb_lastrecord;
	asb.sb_so               = sb->sb_so;
	asb.sb_flags            = sb->sb_flags;
	asb.sb_flags            &= ~(SB_LOCK | SB_SEL | SB_KNOTE | SB_UPCALL);
	asb.sb_flags            |= SB_DROP;

	  
                                                             
                                                            
                                                           
                                                        
   
                                                                 
                                                       
    
	sb->sb_cc               = 0;
	sb->sb_hiwat            = 0;
	sb->sb_mbcnt            = 0;
	sb->sb_mbmax            = 0;
	sb->sb_ctl              = 0;
	sb->sb_lowat            = 0;
	sb->sb_mb               = NULL;
	sb->sb_mbtail           = NULL;
	sb->sb_lastrecord       = NULL;
	sb->sb_timeo.tv_sec     = 0;
	sb->sb_timeo.tv_usec    = 0;
	sb->sb_upcall           = NULL;
	sb->sb_upcallarg        = NULL;
	sb->sb_flags            &= ~(SB_SEL | SB_UPCALL);
	sb->sb_flags            |= SB_DROP;

	sbunlock(sb, TRUE);                             

	  
                                                                 
                                                                
                                                                 
                                            
    
	selthreadclear(&sb->sb_sel);

	if ((pr->pr_flags & PR_RIGHTS) && pr->pr_domain->dom_dispose) {
		(*pr->pr_domain->dom_dispose)(asb.sb_mb);
	}

	sbrelease(&asb);
}

  
                                                                    
                                                                        
                                                       
                                                                        
                                          
  
                       
          
                
   
int
sooptcopyin(struct sockopt *sopt, void *buf, size_t len, size_t minlen)
{
	size_t  valsize;

	  
                                                           
                                                     
                                                            
                                                 
    
	if ((valsize = sopt->sopt_valsize) < minlen) {
		return EINVAL;
	}
	if (valsize > len) {
		sopt->sopt_valsize = valsize = len;
	}

	if (sopt->sopt_p != kernproc) {
		return copyin(sopt->sopt_val, buf, valsize);
	}

	bcopy(CAST_DOWN(caddr_t, sopt->sopt_val), buf, valsize);
	return 0;
}

  
                      
                                                                         
                                                                        
                                                                           
                                                           
   
static int
sooptcopyin_timeval(struct sockopt *sopt, struct timeval *tv_p)
{
	int                     error;

	if (proc_is64bit(sopt->sopt_p)) {
		struct user64_timeval   tv64;

		if (sopt->sopt_valsize < sizeof(tv64)) {
			return EINVAL;
		}

		sopt->sopt_valsize = sizeof(tv64);
		if (sopt->sopt_p != kernproc) {
			error = copyin(sopt->sopt_val, &tv64, sizeof(tv64));
			if (error != 0) {
				return error;
			}
		} else {
			bcopy(CAST_DOWN(caddr_t, sopt->sopt_val), &tv64,
			    sizeof(tv64));
		}
		if (tv64.tv_sec < 0 || tv64.tv_sec > LONG_MAX ||
		    tv64.tv_usec < 0 || tv64.tv_usec >= 1000000) {
			return EDOM;
		}

		tv_p->tv_sec = tv64.tv_sec;
		tv_p->tv_usec = tv64.tv_usec;
	} else {
		struct user32_timeval   tv32;

		if (sopt->sopt_valsize < sizeof(tv32)) {
			return EINVAL;
		}

		sopt->sopt_valsize = sizeof(tv32);
		if (sopt->sopt_p != kernproc) {
			error = copyin(sopt->sopt_val, &tv32, sizeof(tv32));
			if (error != 0) {
				return error;
			}
		} else {
			bcopy(CAST_DOWN(caddr_t, sopt->sopt_val), &tv32,
			    sizeof(tv32));
		}
#ifndef __LP64__
		  
                                               
                                
     
		if (tv32.tv_sec < 0 || tv32.tv_sec > LONG_MAX ||
		    tv32.tv_usec < 0 || tv32.tv_usec >= 1000000) {
			return EDOM;
		}
#endif
		tv_p->tv_sec = tv32.tv_sec;
		tv_p->tv_usec = tv32.tv_usec;
	}
	return 0;
}

int
soopt_cred_check(struct socket *so, int priv, boolean_t allow_root,
    boolean_t ignore_delegate)
{
	kauth_cred_t cred =  NULL;
	proc_t ep = PROC_NULL;
	uid_t uid;
	int error = 0;

	if (ignore_delegate == false && so->so_flags & SOF_DELEGATED) {
		ep = proc_find(so->e_pid);
		if (ep) {
			cred = kauth_cred_proc_ref(ep);
		}
	}

	uid = kauth_cred_getuid(cred ? cred : so->so_cred);

	                       
	if (uid != 0 || !allow_root) {
		error = priv_check_cred(cred ? cred : so->so_cred, priv, 0);
	}
	if (cred) {
		kauth_cred_unref(&cred);
	}
	if (ep != PROC_NULL) {
		proc_rele(ep);
	}

	return error;
}

  
                       
          
               
           
        
                     
                     
                             
                             
                           
                                     
                      
                                                              
                                                         
  
                                                                       
                                                               
                           
   
int
sosetoptlock(struct socket *so, struct sockopt *sopt, int dolock)
{
	int     error, optval;
	int64_t long_optval;
	struct  linger l;
	struct  timeval tv;

	if (sopt->sopt_dir != SOPT_SET) {
		sopt->sopt_dir = SOPT_SET;
	}

	if (dolock) {
		socket_lock(so, 1);
	}

	if ((so->so_state & (SS_CANTRCVMORE | SS_CANTSENDMORE)) ==
	    (SS_CANTRCVMORE | SS_CANTSENDMORE) &&
	    (so->so_flags & SOF_NPX_SETOPTSHUT) == 0) {
		                                                     
		error = EINVAL;
		goto out;
	}

	error = sflt_setsockopt(so, sopt);
	if (error != 0) {
		if (error == EJUSTRETURN) {
			error = 0;
		}
		goto out;
	}

	if (sopt->sopt_level != SOL_SOCKET) {
		if (so->so_proto != NULL &&
		    so->so_proto->pr_ctloutput != NULL) {
			error = (*so->so_proto->pr_ctloutput)(so, sopt);
			goto out;
		}
		error = ENOPROTOOPT;
	} else {
		  
                                                              
                                                               
                                                             
                                                            
                                                           
     
		if (so->so_proto != NULL && (error = so->so_proto->pr_usrreqs->
		    pru_socheckopt(so, sopt)) != 0) {
			goto out;
		}

		error = 0;
		switch (sopt->sopt_name) {
		case SO_LINGER:
		case SO_LINGER_SEC:
			error = sooptcopyin(sopt, &l, sizeof(l), sizeof(l));
			if (error != 0) {
				goto out;
			}

			so->so_linger = (sopt->sopt_name == SO_LINGER) ?
			    l.l_linger : l.l_linger * hz;
			if (l.l_onoff != 0) {
				so->so_options |= SO_LINGER;
			} else {
				so->so_options &= ~SO_LINGER;
			}
			break;

		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_REUSEPORT:
		case SO_OOBINLINE:
		case SO_TIMESTAMP:
		case SO_TIMESTAMP_MONOTONIC:
		case SO_TIMESTAMP_CONTINUOUS:
		case SO_DONTTRUNC:
		case SO_WANTMORE:
		case SO_WANTOOBFLAG:
		case SO_NOWAKEFROMSLEEP:
		case SO_NOAPNFALLBK:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval) {
				so->so_options |= sopt->sopt_name;
			} else {
				so->so_options &= ~sopt->sopt_name;
			}
			break;

		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}

			  
                                               
                                
      
			if (optval < 1) {
				error = EINVAL;
				goto out;
			}

			switch (sopt->sopt_name) {
			case SO_SNDBUF:
			case SO_RCVBUF: {
				struct sockbuf *sb =
				    (sopt->sopt_name == SO_SNDBUF) ?
				    &so->so_snd : &so->so_rcv;
				if (sbreserve(sb, (u_int32_t)optval) == 0) {
					error = ENOBUFS;
					goto out;
				}
				sb->sb_flags |= SB_USRSIZE;
				sb->sb_flags &= ~SB_AUTOSIZE;
				sb->sb_idealsize = (u_int32_t)optval;
				break;
			}
			  
                                                   
                     
      
			case SO_SNDLOWAT: {
				int space = sbspace(&so->so_snd);
				u_int32_t hiwat = so->so_snd.sb_hiwat;

				if (so->so_snd.sb_flags & SB_UNIX) {
					struct unpcb *unp =
					    (struct unpcb *)(so->so_pcb);
					if (unp != NULL &&
					    unp->unp_conn != NULL) {
						hiwat += unp->unp_conn->unp_cc;
					}
				}

				so->so_snd.sb_lowat =
				    (optval > hiwat) ?
				    hiwat : optval;

				if (space >= so->so_snd.sb_lowat) {
					sowwakeup(so);
				}
				break;
			}
			case SO_RCVLOWAT: {
				int64_t data_len;
				so->so_rcv.sb_lowat =
				    (optval > so->so_rcv.sb_hiwat) ?
				    so->so_rcv.sb_hiwat : optval;
				data_len = so->so_rcv.sb_cc
				    - so->so_rcv.sb_ctl;
				if (data_len >= so->so_rcv.sb_lowat) {
					sorwakeup(so);
				}
				break;
			}
			}
			break;

		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
			error = sooptcopyin_timeval(sopt, &tv);
			if (error != 0) {
				goto out;
			}

			switch (sopt->sopt_name) {
			case SO_SNDTIMEO:
				so->so_snd.sb_timeo = tv;
				break;
			case SO_RCVTIMEO:
				so->so_rcv.sb_timeo = tv;
				break;
			}
			break;

		case SO_NKE: {
			struct so_nke nke;

			error = sooptcopyin(sopt, &nke, sizeof(nke),
			    sizeof(nke));
			if (error != 0) {
				goto out;
			}

			error = sflt_attach_internal(so, nke.nke_handle);
			break;
		}

		case SO_NOSIGPIPE:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval != 0) {
				so->so_flags |= SOF_NOSIGPIPE;
			} else {
				so->so_flags &= ~SOF_NOSIGPIPE;
			}
			break;

		case SO_NOADDRERR:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval != 0) {
				so->so_flags |= SOF_NOADDRAVAIL;
			} else {
				so->so_flags &= ~SOF_NOADDRAVAIL;
			}
			break;

		case SO_REUSESHAREUID:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval != 0) {
				so->so_flags |= SOF_REUSESHAREUID;
			} else {
				so->so_flags &= ~SOF_REUSESHAREUID;
			}
			break;

		case SO_NOTIFYCONFLICT:
			if (kauth_cred_issuser(kauth_cred_get()) == 0) {
				error = EPERM;
				goto out;
			}
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval != 0) {
				so->so_flags |= SOF_NOTIFYCONFLICT;
			} else {
				so->so_flags &= ~SOF_NOTIFYCONFLICT;
			}
			break;

		case SO_RESTRICTIONS:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}

			error = so_set_restrictions(so, optval);
			break;

		case SO_AWDL_UNRESTRICTED:
			if (SOCK_DOM(so) != PF_INET &&
			    SOCK_DOM(so) != PF_INET6) {
				error = EOPNOTSUPP;
				goto out;
			}
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval != 0) {
				error = soopt_cred_check(so,
				    PRIV_NET_RESTRICTED_AWDL, false, false);
				if (error == 0) {
					inp_set_awdl_unrestricted(
						sotoinpcb(so));
				}
			} else {
				inp_clear_awdl_unrestricted(sotoinpcb(so));
			}
			break;
		case SO_INTCOPROC_ALLOW:
			if (SOCK_DOM(so) != PF_INET6) {
				error = EOPNOTSUPP;
				goto out;
			}
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval != 0 &&
			    inp_get_intcoproc_allowed(sotoinpcb(so)) == FALSE) {
				error = soopt_cred_check(so,
				    PRIV_NET_RESTRICTED_INTCOPROC, false, false);
				if (error == 0) {
					inp_set_intcoproc_allowed(
						sotoinpcb(so));
				}
			} else if (optval == 0) {
				inp_clear_intcoproc_allowed(sotoinpcb(so));
			}
			break;

		case SO_LABEL:
			error = EOPNOTSUPP;
			break;

		case SO_UPCALLCLOSEWAIT:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval != 0) {
				so->so_flags |= SOF_UPCALLCLOSEWAIT;
			} else {
				so->so_flags &= ~SOF_UPCALLCLOSEWAIT;
			}
			break;

		case SO_RANDOMPORT:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval != 0) {
				so->so_flags |= SOF_BINDRANDOMPORT;
			} else {
				so->so_flags &= ~SOF_BINDRANDOMPORT;
			}
			break;

		case SO_NP_EXTENSIONS: {
			struct so_np_extensions sonpx;

			error = sooptcopyin(sopt, &sonpx, sizeof(sonpx),
			    sizeof(sonpx));
			if (error != 0) {
				goto out;
			}
			if (sonpx.npx_mask & ~SONPX_MASK_VALID) {
				error = EINVAL;
				goto out;
			}
			  
                                  
      
			if ((sonpx.npx_mask & SONPX_SETOPTSHUT)) {
				if ((sonpx.npx_flags & SONPX_SETOPTSHUT)) {
					so->so_flags |= SOF_NPX_SETOPTSHUT;
				} else {
					so->so_flags &= ~SOF_NPX_SETOPTSHUT;
				}
			}
			break;
		}

		case SO_TRAFFIC_CLASS: {
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval >= SO_TC_NET_SERVICE_OFFSET) {
				int netsvc = optval - SO_TC_NET_SERVICE_OFFSET;
				error = so_set_net_service_type(so, netsvc);
				goto out;
			}
			error = so_set_traffic_class(so, optval);
			if (error != 0) {
				goto out;
			}
			so->so_flags1 &= ~SOF1_TC_NET_SERV_TYPE;
			so->so_netsvctype = _NET_SERVICE_TYPE_UNSPEC;
			break;
		}

		case SO_RECV_TRAFFIC_CLASS: {
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval == 0) {
				so->so_flags &= ~SOF_RECV_TRAFFIC_CLASS;
			} else {
				so->so_flags |= SOF_RECV_TRAFFIC_CLASS;
			}
			break;
		}

#if (DEVELOPMENT || DEBUG)
		case SO_TRAFFIC_CLASS_DBG: {
			struct so_tcdbg so_tcdbg;

			error = sooptcopyin(sopt, &so_tcdbg,
			    sizeof(struct so_tcdbg), sizeof(struct so_tcdbg));
			if (error != 0) {
				goto out;
			}
			error = so_set_tcdbg(so, &so_tcdbg);
			if (error != 0) {
				goto out;
			}
			break;
		}
#endif                             

		case SO_PRIVILEGED_TRAFFIC_CLASS:
			error = priv_check_cred(kauth_cred_get(),
			    PRIV_NET_PRIVILEGED_TRAFFIC_CLASS, 0);
			if (error != 0) {
				goto out;
			}
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval == 0) {
				so->so_flags &= ~SOF_PRIVILEGED_TRAFFIC_CLASS;
			} else {
				so->so_flags |= SOF_PRIVILEGED_TRAFFIC_CLASS;
			}
			break;

#if (DEVELOPMENT || DEBUG)
		case SO_DEFUNCTIT:
			error = sosetdefunct(current_proc(), so, 0, FALSE);
			if (error == 0) {
				error = sodefunct(current_proc(), so, 0);
			}

			break;
#endif                             

		case SO_DEFUNCTOK:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0 || (so->so_flags & SOF_DEFUNCT)) {
				if (error == 0) {
					error = EBADF;
				}
				goto out;
			}
			  
                                             
                                             
                                       
      
			if (optval == 0 &&
			    kauth_cred_issuser(kauth_cred_get()) == 0) {
				error = EPERM;
				goto out;
			}
			if (optval) {
				so->so_flags &= ~SOF_NODEFUNCT;
			} else {
				so->so_flags |= SOF_NODEFUNCT;
			}

			if (SOCK_DOM(so) == PF_INET ||
			    SOCK_DOM(so) == PF_INET6) {
				char s[MAX_IPv6_STR_LEN];
				char d[MAX_IPv6_STR_LEN];
				struct inpcb *inp = sotoinpcb(so);

				SODEFUNCTLOG("%s[%d, %s]: so 0x%llx "
				    "[%s %s:%d -> %s:%d] is now marked "
				    "as %seligible for "
				    "defunct\n", __func__, proc_selfpid(),
				    proc_best_name(current_proc()),
				    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
				    (SOCK_TYPE(so) == SOCK_STREAM) ?
				    "TCP" : "UDP", inet_ntop(SOCK_DOM(so),
				    ((SOCK_DOM(so) == PF_INET) ?
				    (void *)&inp->inp_laddr.s_addr :
				    (void *)&inp->in6p_laddr), s, sizeof(s)),
				    ntohs(inp->in6p_lport),
				    inet_ntop(SOCK_DOM(so),
				    (SOCK_DOM(so) == PF_INET) ?
				    (void *)&inp->inp_faddr.s_addr :
				    (void *)&inp->in6p_faddr, d, sizeof(d)),
				    ntohs(inp->in6p_fport),
				    (so->so_flags & SOF_NODEFUNCT) ?
				    "not " : "");
			} else {
				SODEFUNCTLOG("%s[%d, %s]: so 0x%llx [%d,%d] "
				    "is now marked as %seligible for "
				    "defunct\n",
				    __func__, proc_selfpid(),
				    proc_best_name(current_proc()),
				    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
				    SOCK_DOM(so), SOCK_TYPE(so),
				    (so->so_flags & SOF_NODEFUNCT) ?
				    "not " : "");
			}
			break;

		case SO_ISDEFUNCT:
			                                 
			error = EINVAL;
			break;

		case SO_OPPORTUNISTIC:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error == 0) {
				error = so_set_opportunistic(so, optval);
			}
			break;

		case SO_FLUSH:
			                                              
			error = 0;
			break;

		case SO_RECV_ANYIF:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error == 0) {
				error = so_set_recv_anyif(so, optval);
			}
			break;

		case SO_TRAFFIC_MGT_BACKGROUND: {
			                                              
			error = 0;
			break;
		}

#if FLOW_DIVERT
		case SO_FLOW_DIVERT_TOKEN:
			error = flow_divert_token_set(so, sopt);
			break;
#endif                   


		case SO_DELEGATED:
			if ((error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval))) != 0) {
				break;
			}

			error = so_set_effective_pid(so, optval, sopt->sopt_p, true);
			break;

		case SO_DELEGATED_UUID: {
			uuid_t euuid;

			if ((error = sooptcopyin(sopt, &euuid, sizeof(euuid),
			    sizeof(euuid))) != 0) {
				break;
			}

			error = so_set_effective_uuid(so, euuid, sopt->sopt_p, true);
			break;
		}

#if NECP
		case SO_NECP_ATTRIBUTES:
			error = necp_set_socket_attributes(so, sopt);
			break;

		case SO_NECP_CLIENTUUID: {
			if (SOCK_DOM(so) == PF_MULTIPATH) {
				                             
				break;
			}

			if (SOCK_DOM(so) != PF_INET && SOCK_DOM(so) != PF_INET6) {
				error = EINVAL;
				goto out;
			}

			struct inpcb *inp = sotoinpcb(so);
			if (!uuid_is_null(inp->necp_client_uuid)) {
				                                           
				necp_inpcb_remove_cb(inp);
			}

			error = sooptcopyin(sopt, &inp->necp_client_uuid,
			    sizeof(uuid_t), sizeof(uuid_t));
			if (error != 0) {
				goto out;
			}

			if (uuid_is_null(inp->necp_client_uuid)) {
				error = EINVAL;
				goto out;
			}

			pid_t current_pid = proc_pid(current_proc());
			error = necp_client_register_socket_flow(current_pid,
			    inp->necp_client_uuid, inp);
			if (error != 0) {
				uuid_clear(inp->necp_client_uuid);
				goto out;
			}

			if (inp->inp_lport != 0) {
				                                              
				                                        
				necp_client_assign_from_socket(current_pid, inp->necp_client_uuid, inp);
			}

			break;
		}
		case SO_NECP_LISTENUUID: {
			if (SOCK_DOM(so) != PF_INET && SOCK_DOM(so) != PF_INET6) {
				error = EINVAL;
				goto out;
			}

			struct inpcb *inp = sotoinpcb(so);
			if (!uuid_is_null(inp->necp_client_uuid)) {
				error = EINVAL;
				goto out;
			}

			error = sooptcopyin(sopt, &inp->necp_client_uuid,
			    sizeof(uuid_t), sizeof(uuid_t));
			if (error != 0) {
				goto out;
			}

			if (uuid_is_null(inp->necp_client_uuid)) {
				error = EINVAL;
				goto out;
			}

			error = necp_client_register_socket_listener(proc_pid(current_proc()),
			    inp->necp_client_uuid, inp);
			if (error != 0) {
				uuid_clear(inp->necp_client_uuid);
				goto out;
			}

			                                                  
			inp->inp_flags2 |= INP2_EXTERNAL_PORT;

			break;
		}
#endif           

		case SO_EXTENDED_BK_IDLE:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error == 0) {
				error = so_set_extended_bk_idle(so, optval);
			}
			break;

		case SO_MARK_CELLFALLBACK:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval < 0) {
				error = EINVAL;
				goto out;
			}
			if (optval == 0) {
				so->so_flags1 &= ~SOF1_CELLFALLBACK;
			} else {
				so->so_flags1 |= SOF1_CELLFALLBACK;
			}
			break;

		case SO_STATISTICS_EVENT:
			error = sooptcopyin(sopt, &long_optval,
			    sizeof(long_optval), sizeof(long_optval));
			if (error != 0) {
				goto out;
			}
			u_int64_t nstat_event = 0;
			error = so_statistics_event_to_nstat_event(
				&long_optval, &nstat_event);
			if (error != 0) {
				goto out;
			}
			nstat_pcb_event(sotoinpcb(so), nstat_event);
			break;

		case SO_NET_SERVICE_TYPE: {
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			error = so_set_net_service_type(so, optval);
			break;
		}

		case SO_QOSMARKING_POLICY_OVERRIDE:
			error = priv_check_cred(kauth_cred_get(),
			    PRIV_NET_QOSMARKING_POLICY_OVERRIDE, 0);
			if (error != 0) {
				goto out;
			}
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval == 0) {
				so->so_flags1 &= ~SOF1_QOSMARKING_POLICY_OVERRIDE;
			} else {
				so->so_flags1 |= SOF1_QOSMARKING_POLICY_OVERRIDE;
			}
			break;

		case SO_MPKL_SEND_INFO: {
			struct so_mpkl_send_info so_mpkl_send_info;

			error = sooptcopyin(sopt, &so_mpkl_send_info,
			    sizeof(struct so_mpkl_send_info), sizeof(struct so_mpkl_send_info));
			if (error != 0) {
				goto out;
			}
			uuid_copy(so->so_mpkl_send_uuid, so_mpkl_send_info.mpkl_uuid);
			so->so_mpkl_send_proto = so_mpkl_send_info.mpkl_proto;

			if (uuid_is_null(so->so_mpkl_send_uuid) && so->so_mpkl_send_proto == 0) {
				so->so_flags1 &= ~SOF1_MPKL_SEND_INFO;
			} else {
				so->so_flags1 |= SOF1_MPKL_SEND_INFO;
			}
			break;
		}
		case SO_WANT_KEV_SOCKET_CLOSED: {
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error != 0) {
				goto out;
			}
			if (optval == 0) {
				so->so_flags1 &= ~SOF1_WANT_KEV_SOCK_CLOSED;
			} else {
				so->so_flags1 |= SOF1_WANT_KEV_SOCK_CLOSED;
			}
			break;
		}
		default:
			error = ENOPROTOOPT;
			break;
		}
		if (error == 0 && so->so_proto != NULL &&
		    so->so_proto->pr_ctloutput != NULL) {
			(void) so->so_proto->pr_ctloutput(so, sopt);
		}
	}
out:
	if (dolock) {
		socket_unlock(so, 1);
	}
	return error;
}

                                    
int
sooptcopyout(struct sockopt *sopt, void *buf, size_t len)
{
	int     error;
	size_t  valsize;

	error = 0;

	  
                                                             
                                                   
                                                        
                                                              
                                                   
                                                                      
                            
    
	valsize = min(len, sopt->sopt_valsize);
	sopt->sopt_valsize = valsize;
	if (sopt->sopt_val != USER_ADDR_NULL) {
		if (sopt->sopt_p != kernproc) {
			error = copyout(buf, sopt->sopt_val, valsize);
		} else {
			bcopy(buf, CAST_DOWN(caddr_t, sopt->sopt_val), valsize);
		}
	}
	return error;
}

static int
sooptcopyout_timeval(struct sockopt *sopt, const struct timeval *tv_p)
{
	int                     error;
	size_t                  len;
	struct user64_timeval   tv64 = {};
	struct user32_timeval   tv32 = {};
	const void *            val;
	size_t                  valsize;

	error = 0;
	if (proc_is64bit(sopt->sopt_p)) {
		len = sizeof(tv64);
		tv64.tv_sec = tv_p->tv_sec;
		tv64.tv_usec = tv_p->tv_usec;
		val = &tv64;
	} else {
		len = sizeof(tv32);
		tv32.tv_sec = tv_p->tv_sec;
		tv32.tv_usec = tv_p->tv_usec;
		val = &tv32;
	}
	valsize = min(len, sopt->sopt_valsize);
	sopt->sopt_valsize = valsize;
	if (sopt->sopt_val != USER_ADDR_NULL) {
		if (sopt->sopt_p != kernproc) {
			error = copyout(val, sopt->sopt_val, valsize);
		} else {
			bcopy(val, CAST_DOWN(caddr_t, sopt->sopt_val), valsize);
		}
	}
	return error;
}

  
                      
               
                                     
                     
                     
   
int
sogetoptlock(struct socket *so, struct sockopt *sopt, int dolock)
{
	int     error, optval;
	struct  linger l;
	struct  timeval tv;

	if (sopt->sopt_dir != SOPT_GET) {
		sopt->sopt_dir = SOPT_GET;
	}

	if (dolock) {
		socket_lock(so, 1);
	}

	error = sflt_getsockopt(so, sopt);
	if (error != 0) {
		if (error == EJUSTRETURN) {
			error = 0;
		}
		goto out;
	}

	if (sopt->sopt_level != SOL_SOCKET) {
		if (so->so_proto != NULL &&
		    so->so_proto->pr_ctloutput != NULL) {
			error = (*so->so_proto->pr_ctloutput)(so, sopt);
			goto out;
		}
		error = ENOPROTOOPT;
	} else {
		  
                                                              
                                                               
                                                             
                                                            
                                                           
     
		if (so->so_proto != NULL && (error = so->so_proto->pr_usrreqs->
		    pru_socheckopt(so, sopt)) != 0) {
			goto out;
		}

		error = 0;
		switch (sopt->sopt_name) {
		case SO_LINGER:
		case SO_LINGER_SEC:
			l.l_onoff = ((so->so_options & SO_LINGER) ? 1 : 0);
			l.l_linger = (sopt->sopt_name == SO_LINGER) ?
			    so->so_linger : so->so_linger / hz;
			error = sooptcopyout(sopt, &l, sizeof(l));
			break;

		case SO_USELOOPBACK:
		case SO_DONTROUTE:
		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_REUSEADDR:
		case SO_REUSEPORT:
		case SO_BROADCAST:
		case SO_OOBINLINE:
		case SO_TIMESTAMP:
		case SO_TIMESTAMP_MONOTONIC:
		case SO_TIMESTAMP_CONTINUOUS:
		case SO_DONTTRUNC:
		case SO_WANTMORE:
		case SO_WANTOOBFLAG:
		case SO_NOWAKEFROMSLEEP:
		case SO_NOAPNFALLBK:
			optval = so->so_options & sopt->sopt_name;
integer:
			error = sooptcopyout(sopt, &optval, sizeof(optval));
			break;

		case SO_TYPE:
			optval = so->so_type;
			goto integer;

		case SO_NREAD:
			if (so->so_proto->pr_flags & PR_ATOMIC) {
				int pkt_total;
				struct mbuf *m1;

				pkt_total = 0;
				m1 = so->so_rcv.sb_mb;
				while (m1 != NULL) {
					if (m1->m_type == MT_DATA ||
					    m1->m_type == MT_HEADER ||
					    m1->m_type == MT_OOBDATA) {
						pkt_total += m1->m_len;
					}
					m1 = m1->m_next;
				}
				optval = pkt_total;
			} else {
				optval = so->so_rcv.sb_cc - so->so_rcv.sb_ctl;
			}
			goto integer;

		case SO_NUMRCVPKT:
			if (so->so_proto->pr_flags & PR_ATOMIC) {
				int cnt = 0;
				struct mbuf *m1;

				m1 = so->so_rcv.sb_mb;
				while (m1 != NULL) {
					cnt += 1;
					m1 = m1->m_nextpkt;
				}
				optval = cnt;
				goto integer;
			} else {
				error = ENOPROTOOPT;
				break;
			}

		case SO_NWRITE:
			optval = so->so_snd.sb_cc;
			goto integer;

		case SO_ERROR:
			optval = so->so_error;
			so->so_error = 0;
			goto integer;

		case SO_SNDBUF: {
			u_int32_t hiwat = so->so_snd.sb_hiwat;

			if (so->so_snd.sb_flags & SB_UNIX) {
				struct unpcb *unp =
				    (struct unpcb *)(so->so_pcb);
				if (unp != NULL && unp->unp_conn != NULL) {
					hiwat += unp->unp_conn->unp_cc;
				}
			}

			optval = hiwat;
			goto integer;
		}
		case SO_RCVBUF:
			optval = so->so_rcv.sb_hiwat;
			goto integer;

		case SO_SNDLOWAT:
			optval = so->so_snd.sb_lowat;
			goto integer;

		case SO_RCVLOWAT:
			optval = so->so_rcv.sb_lowat;
			goto integer;

		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
			tv = (sopt->sopt_name == SO_SNDTIMEO ?
			    so->so_snd.sb_timeo : so->so_rcv.sb_timeo);

			error = sooptcopyout_timeval(sopt, &tv);
			break;

		case SO_NOSIGPIPE:
			optval = (so->so_flags & SOF_NOSIGPIPE);
			goto integer;

		case SO_NOADDRERR:
			optval = (so->so_flags & SOF_NOADDRAVAIL);
			goto integer;

		case SO_REUSESHAREUID:
			optval = (so->so_flags & SOF_REUSESHAREUID);
			goto integer;


		case SO_NOTIFYCONFLICT:
			optval = (so->so_flags & SOF_NOTIFYCONFLICT);
			goto integer;

		case SO_RESTRICTIONS:
			optval = so_get_restrictions(so);
			goto integer;

		case SO_AWDL_UNRESTRICTED:
			if (SOCK_DOM(so) == PF_INET ||
			    SOCK_DOM(so) == PF_INET6) {
				optval = inp_get_awdl_unrestricted(
					sotoinpcb(so));
				goto integer;
			} else {
				error = EOPNOTSUPP;
			}
			break;

		case SO_INTCOPROC_ALLOW:
			if (SOCK_DOM(so) == PF_INET6) {
				optval = inp_get_intcoproc_allowed(
					sotoinpcb(so));
				goto integer;
			} else {
				error = EOPNOTSUPP;
			}
			break;

		case SO_LABEL:
			error = EOPNOTSUPP;
			break;

		case SO_PEERLABEL:
			error = EOPNOTSUPP;
			break;

#ifdef __APPLE_API_PRIVATE
		case SO_UPCALLCLOSEWAIT:
			optval = (so->so_flags & SOF_UPCALLCLOSEWAIT);
			goto integer;
#endif
		case SO_RANDOMPORT:
			optval = (so->so_flags & SOF_BINDRANDOMPORT);
			goto integer;

		case SO_NP_EXTENSIONS: {
			struct so_np_extensions sonpx = {};

			sonpx.npx_flags = (so->so_flags & SOF_NPX_SETOPTSHUT) ?
			    SONPX_SETOPTSHUT : 0;
			sonpx.npx_mask = SONPX_MASK_VALID;

			error = sooptcopyout(sopt, &sonpx,
			    sizeof(struct so_np_extensions));
			break;
		}

		case SO_TRAFFIC_CLASS:
			optval = so->so_traffic_class;
			goto integer;

		case SO_RECV_TRAFFIC_CLASS:
			optval = (so->so_flags & SOF_RECV_TRAFFIC_CLASS);
			goto integer;

#if (DEVELOPMENT || DEBUG)
		case SO_TRAFFIC_CLASS_DBG:
			error = sogetopt_tcdbg(so, sopt);
			break;
#endif                             

		case SO_PRIVILEGED_TRAFFIC_CLASS:
			optval = (so->so_flags & SOF_PRIVILEGED_TRAFFIC_CLASS);
			goto integer;

		case SO_DEFUNCTOK:
			optval = !(so->so_flags & SOF_NODEFUNCT);
			goto integer;

		case SO_ISDEFUNCT:
			optval = (so->so_flags & SOF_DEFUNCT);
			goto integer;

		case SO_OPPORTUNISTIC:
			optval = so_get_opportunistic(so);
			goto integer;

		case SO_FLUSH:
			                                 
			error = EINVAL;
			break;

		case SO_RECV_ANYIF:
			optval = so_get_recv_anyif(so);
			goto integer;

		case SO_TRAFFIC_MGT_BACKGROUND:
			                                              
			if (so->so_proto != NULL &&
			    so->so_proto->pr_ctloutput != NULL) {
				(void) so->so_proto->pr_ctloutput(so, sopt);
			}
			break;

#if FLOW_DIVERT
		case SO_FLOW_DIVERT_TOKEN:
			error = flow_divert_token_get(so, sopt);
			break;
#endif                   

#if NECP
		case SO_NECP_ATTRIBUTES:
			error = necp_get_socket_attributes(so, sopt);
			break;

		case SO_NECP_CLIENTUUID: {
			uuid_t *ncu;

			if (SOCK_DOM(so) == PF_MULTIPATH) {
				ncu = &mpsotomppcb(so)->necp_client_uuid;
			} else if (SOCK_DOM(so) == PF_INET || SOCK_DOM(so) == PF_INET6) {
				ncu = &sotoinpcb(so)->necp_client_uuid;
			} else {
				error = EINVAL;
				goto out;
			}

			error = sooptcopyout(sopt, ncu, sizeof(uuid_t));
			break;
		}

		case SO_NECP_LISTENUUID: {
			uuid_t *nlu;

			if (SOCK_DOM(so) == PF_INET || SOCK_DOM(so) == PF_INET6) {
				if (sotoinpcb(so)->inp_flags2 & INP2_EXTERNAL_PORT) {
					nlu = &sotoinpcb(so)->necp_client_uuid;
				} else {
					error = ENOENT;
					goto out;
				}
			} else {
				error = EINVAL;
				goto out;
			}

			error = sooptcopyout(sopt, nlu, sizeof(uuid_t));
			break;
		}
#endif           

#if CONTENT_FILTER
		case SO_CFIL_SOCK_ID: {
			cfil_sock_id_t sock_id;

			sock_id = cfil_sock_id_from_socket(so);

			error = sooptcopyout(sopt, &sock_id,
			    sizeof(cfil_sock_id_t));
			break;
		}
#endif                      

		case SO_EXTENDED_BK_IDLE:
			optval = (so->so_flags1 & SOF1_EXTEND_BK_IDLE_WANTED);
			goto integer;
		case SO_MARK_CELLFALLBACK:
			optval = ((so->so_flags1 & SOF1_CELLFALLBACK) > 0)
			    ? 1 : 0;
			goto integer;
		case SO_NET_SERVICE_TYPE: {
			if ((so->so_flags1 & SOF1_TC_NET_SERV_TYPE)) {
				optval = so->so_netsvctype;
			} else {
				optval = NET_SERVICE_TYPE_BE;
			}
			goto integer;
		}
		case SO_NETSVC_MARKING_LEVEL:
			optval = so_get_netsvc_marking_level(so);
			goto integer;

		case SO_MPKL_SEND_INFO: {
			struct so_mpkl_send_info so_mpkl_send_info;

			uuid_copy(so_mpkl_send_info.mpkl_uuid, so->so_mpkl_send_uuid);
			so_mpkl_send_info.mpkl_proto = so->so_mpkl_send_proto;
			error = sooptcopyout(sopt, &so_mpkl_send_info,
			    sizeof(struct so_mpkl_send_info));
			break;
		}
		default:
			error = ENOPROTOOPT;
			break;
		}
	}
out:
	if (dolock) {
		socket_unlock(so, 1);
	}
	return error;
}

  
                                                                       
                                                                     
                                                                   
   
int
soopt_getm(struct sockopt *sopt, struct mbuf **mp)
{
	struct mbuf *m, *m_prev;
	int sopt_size = sopt->sopt_valsize;
	int how;

	if (sopt_size <= 0 || sopt_size > MCLBYTES) {
		return EMSGSIZE;
	}

	how = sopt->sopt_p != kernproc ? M_WAIT : M_DONTWAIT;
	MGET(m, how, MT_DATA);
	if (m == NULL) {
		return ENOBUFS;
	}
	if (sopt_size > MLEN) {
		MCLGET(m, how);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			return ENOBUFS;
		}
		m->m_len = min(MCLBYTES, sopt_size);
	} else {
		m->m_len = min(MLEN, sopt_size);
	}
	sopt_size -= m->m_len;
	*mp = m;
	m_prev = m;

	while (sopt_size > 0) {
		MGET(m, how, MT_DATA);
		if (m == NULL) {
			m_freem(*mp);
			return ENOBUFS;
		}
		if (sopt_size > MLEN) {
			MCLGET(m, how);
			if ((m->m_flags & M_EXT) == 0) {
				m_freem(*mp);
				m_freem(m);
				return ENOBUFS;
			}
			m->m_len = min(MCLBYTES, sopt_size);
		} else {
			m->m_len = min(MLEN, sopt_size);
		}
		sopt_size -= m->m_len;
		m_prev->m_next = m;
		m_prev = m;
	}
	return 0;
}

                                      
int
soopt_mcopyin(struct sockopt *sopt, struct mbuf *m)
{
	struct mbuf *m0 = m;

	if (sopt->sopt_val == USER_ADDR_NULL) {
		return 0;
	}
	while (m != NULL && sopt->sopt_valsize >= m->m_len) {
		if (sopt->sopt_p != kernproc) {
			int error;

			error = copyin(sopt->sopt_val, mtod(m, char *),
			    m->m_len);
			if (error != 0) {
				m_freem(m0);
				return error;
			}
		} else {
			bcopy(CAST_DOWN(caddr_t, sopt->sopt_val),
			    mtod(m, char *), m->m_len);
		}
		sopt->sopt_valsize -= m->m_len;
		sopt->sopt_val += m->m_len;
		m = m->m_next;
	}
	                                                        
	if (m != NULL) {
		panic("soopt_mcopyin");
		                
	}
	return 0;
}

                                        
int
soopt_mcopyout(struct sockopt *sopt, struct mbuf *m)
{
	struct mbuf *m0 = m;
	size_t valsize = 0;

	if (sopt->sopt_val == USER_ADDR_NULL) {
		return 0;
	}
	while (m != NULL && sopt->sopt_valsize >= m->m_len) {
		if (sopt->sopt_p != kernproc) {
			int error;

			error = copyout(mtod(m, char *), sopt->sopt_val,
			    m->m_len);
			if (error != 0) {
				m_freem(m0);
				return error;
			}
		} else {
			bcopy(mtod(m, char *),
			    CAST_DOWN(caddr_t, sopt->sopt_val), m->m_len);
		}
		sopt->sopt_valsize -= m->m_len;
		sopt->sopt_val += m->m_len;
		valsize += m->m_len;
		m = m->m_next;
	}
	if (m != NULL) {
		                                                        
		m_freem(m0);
		return EINVAL;
	}
	sopt->sopt_valsize = valsize;
	return 0;
}

void
sohasoutofband(struct socket *so)
{
	if (so->so_pgid < 0) {
		gsignal(-so->so_pgid, SIGURG);
	} else if (so->so_pgid > 0) {
		proc_signal(so->so_pgid, SIGURG);
	}
	selwakeup(&so->so_rcv.sb_sel);
	if (so->so_rcv.sb_flags & SB_KNOTE) {
		KNOTE(&so->so_rcv.sb_sel.si_note,
		    (NOTE_OOB | SO_FILT_HINT_LOCKED));
	}
}

int
sopoll(struct socket *so, int events, kauth_cred_t cred, void * wql)
{
#pragma unused(cred)
	struct proc *p = current_proc();
	int revents = 0;

	socket_lock(so, 1);
	so_update_last_owner_locked(so, PROC_NULL);
	so_update_policy(so);

	if (events & (POLLIN | POLLRDNORM)) {
		if (soreadable(so)) {
			revents |= events & (POLLIN | POLLRDNORM);
		}
	}

	if (events & (POLLOUT | POLLWRNORM)) {
		if (sowriteable(so)) {
			revents |= events & (POLLOUT | POLLWRNORM);
		}
	}

	if (events & (POLLPRI | POLLRDBAND)) {
		if (so->so_oobmark || (so->so_state & SS_RCVATMARK)) {
			revents |= events & (POLLPRI | POLLRDBAND);
		}
	}

	if (revents == 0) {
		if (events & (POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND)) {
			  
                                 
                               
      
			so->so_rcv.sb_flags |= SB_SEL;
			selrecord(p, &so->so_rcv.sb_sel, wql);
		}

		if (events & (POLLOUT | POLLWRNORM)) {
			  
                                 
                               
      
			so->so_snd.sb_flags |= SB_SEL;
			selrecord(p, &so->so_snd.sb_sel, wql);
		}
	}

	socket_unlock(so, 1);
	return revents;
}

int
soo_kqfilter(struct fileproc *fp, struct knote *kn, struct kevent_qos_s *kev)
{
	struct socket *so = (struct socket *)fp->fp_glob->fg_data;
	int result;

	socket_lock(so, 1);
	so_update_last_owner_locked(so, PROC_NULL);
	so_update_policy(so);

	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_filtid = EVFILTID_SOREAD;
		break;
	case EVFILT_WRITE:
		kn->kn_filtid = EVFILTID_SOWRITE;
		break;
	case EVFILT_SOCK:
		kn->kn_filtid = EVFILTID_SCK;
		break;
	case EVFILT_EXCEPT:
		kn->kn_filtid = EVFILTID_SOEXCEPT;
		break;
	default:
		socket_unlock(so, 1);
		knote_set_error(kn, EINVAL);
		return 0;
	}

	  
                                          
                                
    
	result = knote_fops(kn)->f_attach(kn, kev);

	socket_unlock(so, 1);

	return result;
}

static int
filt_soread_common(struct knote *kn, struct kevent_qos_s *kev, struct socket *so)
{
	int retval = 0;
	int64_t data = 0;

	if (so->so_options & SO_ACCEPTCONN) {
		  
                                                     
                                                         
                                              
     

		retval = !TAILQ_EMPTY(&so->so_comp);
		data = so->so_qlen;
		goto out;
	}

	                             
	  
                                                         
                                                        
                  
    
	data = so->so_rcv.sb_cc - so->so_rcv.sb_ctl;

	if (kn->kn_sfflags & NOTE_OOB) {
		if (so->so_oobmark || (so->so_state & SS_RCVATMARK)) {
			kn->kn_fflags |= NOTE_OOB;
			data -= so->so_oobmark;
			retval = 1;
			goto out;
		}
	}

	if ((so->so_state & SS_CANTRCVMORE)
#if CONTENT_FILTER
	    && cfil_sock_data_pending(&so->so_rcv) == 0
#endif                     
	    ) {
		kn->kn_flags |= EV_EOF;
		kn->kn_fflags = so->so_error;
		retval = 1;
		goto out;
	}

	if (so->so_error) {                              
		retval = 1;
		goto out;
	}

	int64_t lowwat = so->so_rcv.sb_lowat;
	  
                                                    
                                                   
                                   
    
	if (kn->kn_sfflags & NOTE_LOWAT) {
		if (kn->kn_sdata > so->so_rcv.sb_hiwat) {
			lowwat = so->so_rcv.sb_hiwat;
		} else if (kn->kn_sdata > lowwat) {
			lowwat = kn->kn_sdata;
		}
	}

	  
                                                         
                                                             
                                                      
    
	retval = (so->so_rcv.sb_cc >= lowwat);

out:
	if (retval && kev) {
		knote_fill_kevent(kn, kev, data);
	}
	return retval;
}

static int
filt_sorattach(struct knote *kn, __unused struct kevent_qos_s *kev)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;

	                   

	  
                                                                
                                                            
                                                           
    
	if (kn->kn_filter == EVFILT_READ &&
	    kn->kn_flags & EV_OOBAND) {
		kn->kn_flags &= ~EV_OOBAND;
		kn->kn_hook32 = EV_OOBAND;
	} else {
		kn->kn_hook32 = 0;
	}
	if (KNOTE_ATTACH(&so->so_rcv.sb_sel.si_note, kn)) {
		so->so_rcv.sb_flags |= SB_KNOTE;
	}

	                                        
	return filt_soread_common(kn, NULL, so);
}

static void
filt_sordetach(struct knote *kn)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;

	socket_lock(so, 1);
	if (so->so_rcv.sb_flags & SB_KNOTE) {
		if (KNOTE_DETACH(&so->so_rcv.sb_sel.si_note, kn)) {
			so->so_rcv.sb_flags &= ~SB_KNOTE;
		}
	}
	socket_unlock(so, 1);
}

            
static int
filt_soread(struct knote *kn, long hint)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	int retval;

	if ((hint & SO_FILT_HINT_LOCKED) == 0) {
		socket_lock(so, 1);
	}

	retval = filt_soread_common(kn, NULL, so);

	if ((hint & SO_FILT_HINT_LOCKED) == 0) {
		socket_unlock(so, 1);
	}

	return retval;
}

static int
filt_sortouch(struct knote *kn, struct kevent_qos_s *kev)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	int retval;

	socket_lock(so, 1);

	                                            
	kn->kn_sfflags = kev->fflags;
	kn->kn_sdata = kev->data;

	                                                 
	retval = filt_soread_common(kn, NULL, so);

	socket_unlock(so, 1);

	return retval;
}

static int
filt_sorprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	int retval;

	socket_lock(so, 1);
	retval = filt_soread_common(kn, kev, so);
	socket_unlock(so, 1);

	return retval;
}

int
so_wait_for_if_feedback(struct socket *so)
{
	if ((SOCK_DOM(so) == PF_INET || SOCK_DOM(so) == PF_INET6) &&
	    (so->so_state & SS_ISCONNECTED)) {
		struct inpcb *inp = sotoinpcb(so);
		if (INP_WAIT_FOR_IF_FEEDBACK(inp)) {
			return 1;
		}
	}
	return 0;
}

static int
filt_sowrite_common(struct knote *kn, struct kevent_qos_s *kev, struct socket *so)
{
	int ret = 0;
	int64_t data = sbspace(&so->so_snd);

	if (so->so_state & SS_CANTSENDMORE) {
		kn->kn_flags |= EV_EOF;
		kn->kn_fflags = so->so_error;
		ret = 1;
		goto out;
	}

	if (so->so_error) {                              
		ret = 1;
		goto out;
	}

	if (!socanwrite(so)) {
		ret = 0;
		goto out;
	}

	if (so->so_flags1 & SOF1_PRECONNECT_DATA) {
		ret = 1;
		goto out;
	}

	int64_t lowwat = so->so_snd.sb_lowat;

	if (kn->kn_sfflags & NOTE_LOWAT) {
		if (kn->kn_sdata > so->so_snd.sb_hiwat) {
			lowwat = so->so_snd.sb_hiwat;
		} else if (kn->kn_sdata > lowwat) {
			lowwat = kn->kn_sdata;
		}
	}

	if (data >= lowwat) {
		if ((so->so_flags & SOF_NOTSENT_LOWAT)
#if (DEBUG || DEVELOPMENT)
		    && so_notsent_lowat_check == 1
#endif                           
		    ) {
			if ((SOCK_DOM(so) == PF_INET ||
			    SOCK_DOM(so) == PF_INET6) &&
			    so->so_type == SOCK_STREAM) {
				ret = tcp_notsent_lowat_check(so);
			}
#if MPTCP
			else if ((SOCK_DOM(so) == PF_MULTIPATH) &&
			    (SOCK_PROTO(so) == IPPROTO_TCP)) {
				ret = mptcp_notsent_lowat_check(so);
			}
#endif
			else {
				ret = 1;
				goto out;
			}
		} else {
			ret = 1;
		}
	}
	if (so_wait_for_if_feedback(so)) {
		ret = 0;
	}

out:
	if (ret && kev) {
		knote_fill_kevent(kn, kev, data);
	}
	return ret;
}

static int
filt_sowattach(struct knote *kn, __unused struct kevent_qos_s *kev)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;

	                   
	if (KNOTE_ATTACH(&so->so_snd.sb_sel.si_note, kn)) {
		so->so_snd.sb_flags |= SB_KNOTE;
	}

	                                    
	return filt_sowrite_common(kn, NULL, so);
}

static void
filt_sowdetach(struct knote *kn)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	socket_lock(so, 1);

	if (so->so_snd.sb_flags & SB_KNOTE) {
		if (KNOTE_DETACH(&so->so_snd.sb_sel.si_note, kn)) {
			so->so_snd.sb_flags &= ~SB_KNOTE;
		}
	}
	socket_unlock(so, 1);
}

            
static int
filt_sowrite(struct knote *kn, long hint)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	int ret;

	if ((hint & SO_FILT_HINT_LOCKED) == 0) {
		socket_lock(so, 1);
	}

	ret = filt_sowrite_common(kn, NULL, so);

	if ((hint & SO_FILT_HINT_LOCKED) == 0) {
		socket_unlock(so, 1);
	}

	return ret;
}

static int
filt_sowtouch(struct knote *kn, struct kevent_qos_s *kev)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	int ret;

	socket_lock(so, 1);

	                                           
	kn->kn_sfflags = kev->fflags;
	kn->kn_sdata = kev->data;

	                                                            
	ret = filt_sowrite_common(kn, NULL, so);

	socket_unlock(so, 1);

	return ret;
}

static int
filt_sowprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	int ret;

	socket_lock(so, 1);
	ret = filt_sowrite_common(kn, kev, so);
	socket_unlock(so, 1);

	return ret;
}

static int
filt_sockev_common(struct knote *kn, struct kevent_qos_s *kev,
    struct socket *so, long ev_hint)
{
	int ret = 0;
	int64_t data = 0;
	uint32_t level_trigger = 0;

	if (ev_hint & SO_FILT_HINT_CONNRESET) {
		kn->kn_fflags |= NOTE_CONNRESET;
	}
	if (ev_hint & SO_FILT_HINT_TIMEOUT) {
		kn->kn_fflags |= NOTE_TIMEOUT;
	}
	if (ev_hint & SO_FILT_HINT_NOSRCADDR) {
		kn->kn_fflags |= NOTE_NOSRCADDR;
	}
	if (ev_hint & SO_FILT_HINT_IFDENIED) {
		kn->kn_fflags |= NOTE_IFDENIED;
	}
	if (ev_hint & SO_FILT_HINT_KEEPALIVE) {
		kn->kn_fflags |= NOTE_KEEPALIVE;
	}
	if (ev_hint & SO_FILT_HINT_ADAPTIVE_WTIMO) {
		kn->kn_fflags |= NOTE_ADAPTIVE_WTIMO;
	}
	if (ev_hint & SO_FILT_HINT_ADAPTIVE_RTIMO) {
		kn->kn_fflags |= NOTE_ADAPTIVE_RTIMO;
	}
	if ((ev_hint & SO_FILT_HINT_CONNECTED) ||
	    (so->so_state & SS_ISCONNECTED)) {
		kn->kn_fflags |= NOTE_CONNECTED;
		level_trigger |= NOTE_CONNECTED;
	}
	if ((ev_hint & SO_FILT_HINT_DISCONNECTED) ||
	    (so->so_state & SS_ISDISCONNECTED)) {
		kn->kn_fflags |= NOTE_DISCONNECTED;
		level_trigger |= NOTE_DISCONNECTED;
	}
	if (ev_hint & SO_FILT_HINT_CONNINFO_UPDATED) {
		if (so->so_proto != NULL &&
		    (so->so_proto->pr_flags & PR_EVCONNINFO)) {
			kn->kn_fflags |= NOTE_CONNINFO_UPDATED;
		}
	}

	if ((ev_hint & SO_FILT_HINT_NOTIFY_ACK) ||
	    tcp_notify_ack_active(so)) {
		kn->kn_fflags |= NOTE_NOTIFY_ACK;
	}

	if ((so->so_state & SS_CANTRCVMORE)
#if CONTENT_FILTER
	    && cfil_sock_data_pending(&so->so_rcv) == 0
#endif                     
	    ) {
		kn->kn_fflags |= NOTE_READCLOSED;
		level_trigger |= NOTE_READCLOSED;
	}

	if (so->so_state & SS_CANTSENDMORE) {
		kn->kn_fflags |= NOTE_WRITECLOSED;
		level_trigger |= NOTE_WRITECLOSED;
	}

	if ((ev_hint & SO_FILT_HINT_SUSPEND) ||
	    (so->so_flags & SOF_SUSPENDED)) {
		kn->kn_fflags &= ~(NOTE_SUSPEND | NOTE_RESUME);

		                                                    
		kn->kn_hook32 &= ~NOTE_RESUME;

		kn->kn_fflags |= NOTE_SUSPEND;
		level_trigger |= NOTE_SUSPEND;
	}

	if ((ev_hint & SO_FILT_HINT_RESUME) ||
	    (so->so_flags & SOF_SUSPENDED) == 0) {
		kn->kn_fflags &= ~(NOTE_SUSPEND | NOTE_RESUME);

		                                                     
		kn->kn_hook32 &= ~NOTE_SUSPEND;

		kn->kn_fflags |= NOTE_RESUME;
		level_trigger |= NOTE_RESUME;
	}

	if (so->so_error != 0) {
		ret = 1;
		data = so->so_error;
		kn->kn_flags |= EV_EOF;
	} else {
		u_int32_t data32 = 0;
		get_sockev_state(so, &data32);
		data = data32;
	}

	                                                           
	kn->kn_fflags &= (kn->kn_sfflags & EVFILT_SOCK_ALL_MASK);
	level_trigger &= (kn->kn_sfflags & EVFILT_SOCK_ALL_MASK);

	                                                                 
	level_trigger &= kn->kn_hook32;
	level_trigger &= EVFILT_SOCK_LEVEL_TRIGGER_MASK;

	                                                           
	if ((kn->kn_fflags & ~level_trigger) != 0) {
		ret = 1;
	}

	if (ret && kev) {
		  
                                                        
                                                        
                                                          
                                                   
     
		if (kn->kn_fflags != 0) {
			kn->kn_hook32 |= (kn->kn_fflags &
			    EVFILT_SOCK_LEVEL_TRIGGER_MASK);
		}

		  
                                                           
                                                        
                   
     
		if (kn->kn_fflags & NOTE_SUSPEND) {
			kn->kn_hook32 &= ~NOTE_RESUME;
		}
		if (kn->kn_fflags & NOTE_RESUME) {
			kn->kn_hook32 &= ~NOTE_SUSPEND;
		}

		knote_fill_kevent(kn, kev, data);
	}
	return ret;
}

static int
filt_sockattach(struct knote *kn, __unused struct kevent_qos_s *kev)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;

	                   
	kn->kn_hook32 = 0;
	if (KNOTE_ATTACH(&so->so_klist, kn)) {
		so->so_flags |= SOF_KNOTE;
	}

	                                      
	return filt_sockev_common(kn, NULL, so, 0);
}

static void
filt_sockdetach(struct knote *kn)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	socket_lock(so, 1);

	if ((so->so_flags & SOF_KNOTE) != 0) {
		if (KNOTE_DETACH(&so->so_klist, kn)) {
			so->so_flags &= ~SOF_KNOTE;
		}
	}
	socket_unlock(so, 1);
}

static int
filt_sockev(struct knote *kn, long hint)
{
	int ret = 0, locked = 0;
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	long ev_hint = (hint & SO_FILT_HINT_EV);

	if ((hint & SO_FILT_HINT_LOCKED) == 0) {
		socket_lock(so, 1);
		locked = 1;
	}

	ret = filt_sockev_common(kn, NULL, so, ev_hint);

	if (locked) {
		socket_unlock(so, 1);
	}

	return ret;
}



  
                                      
   
static int
filt_socktouch(
	struct knote *kn,
	struct kevent_qos_s *kev)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	uint32_t changed_flags;
	int ret;

	socket_lock(so, 1);

	                                           
	changed_flags = (kn->kn_sfflags ^ kn->kn_hook32);

	                                            
	kn->kn_sfflags = kev->fflags;
	kn->kn_sdata = kev->data;

	                                                                        
	  
                                                                       
                        
    
	                               

	  
                                                  
                                                       
                                                  
    
	kn->kn_hook32 &= ~(changed_flags & EVFILT_SOCK_LEVEL_TRIGGER_MASK);

	                                            
	ret = filt_sockev_common(kn, NULL, so, 0);

	socket_unlock(so, 1);

	return ret;
}

  
                                                             
   
static int
filt_sockprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	struct socket *so = (struct socket *)kn->kn_fp->fp_glob->fg_data;
	int ret = 0;

	socket_lock(so, 1);

	ret = filt_sockev_common(kn, kev, so, 0);

	socket_unlock(so, 1);

	return ret;
}

void
get_sockev_state(struct socket *so, u_int32_t *statep)
{
	u_int32_t state = *(statep);

	  
                                                              
             
    
	if (state != 0) {
		return;
	}

	if (so->so_state & SS_ISCONNECTED) {
		state |= SOCKEV_CONNECTED;
	} else {
		state &= ~(SOCKEV_CONNECTED);
	}
	state |= ((so->so_state & SS_ISDISCONNECTED) ? SOCKEV_DISCONNECTED : 0);
	*(statep) = state;
}

#define SO_LOCK_HISTORY_STR_LEN \
	(2 * SO_LCKDBG_MAX * (2 + (2 * sizeof (void *)) + 1) + 1)

__private_extern__ const char *
solockhistory_nr(struct socket *so)
{
	size_t n = 0;
	int i;
	static char lock_history_str[SO_LOCK_HISTORY_STR_LEN];

	bzero(lock_history_str, sizeof(lock_history_str));
	for (i = SO_LCKDBG_MAX - 1; i >= 0; i--) {
		n += scnprintf(lock_history_str + n,
		    SO_LOCK_HISTORY_STR_LEN - n, "%p:%p ",
		    so->lock_lr[(so->next_lock_lr + i) % SO_LCKDBG_MAX],
		    so->unlock_lr[(so->next_unlock_lr + i) % SO_LCKDBG_MAX]);
	}
	return lock_history_str;
}

lck_mtx_t *
socket_getlock(struct socket *so, int flags)
{
	if (so->so_proto->pr_getlock != NULL) {
		return (*so->so_proto->pr_getlock)(so, flags);
	} else {
		return so->so_proto->pr_domain->dom_mtx;
	}
}

void
socket_lock(struct socket *so, int refcount)
{
	void *lr_saved;

	lr_saved = __builtin_return_address(0);

	if (so->so_proto->pr_lock) {
		(*so->so_proto->pr_lock)(so, refcount, lr_saved);
	} else {
#ifdef MORE_LOCKING_DEBUG
		LCK_MTX_ASSERT(so->so_proto->pr_domain->dom_mtx,
		    LCK_MTX_ASSERT_NOTOWNED);
#endif
		lck_mtx_lock(so->so_proto->pr_domain->dom_mtx);
		if (refcount) {
			so->so_usecount++;
		}
		so->lock_lr[so->next_lock_lr] = lr_saved;
		so->next_lock_lr = (so->next_lock_lr + 1) % SO_LCKDBG_MAX;
	}
}

void
socket_lock_assert_owned(struct socket *so)
{
	lck_mtx_t *mutex_held;

	if (so->so_proto->pr_getlock != NULL) {
		mutex_held = (*so->so_proto->pr_getlock)(so, 0);
	} else {
		mutex_held = so->so_proto->pr_domain->dom_mtx;
	}

	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);
}

int
socket_try_lock(struct socket *so)
{
	lck_mtx_t *mtx;

	if (so->so_proto->pr_getlock != NULL) {
		mtx = (*so->so_proto->pr_getlock)(so, 0);
	} else {
		mtx = so->so_proto->pr_domain->dom_mtx;
	}

	return lck_mtx_try_lock(mtx);
}

void
socket_unlock(struct socket *so, int refcount)
{
	void *lr_saved;
	lck_mtx_t *mutex_held;

	lr_saved = __builtin_return_address(0);

	if (so == NULL || so->so_proto == NULL) {
		panic("%s: null so_proto so=%p\n", __func__, so);
		                
	}

	if (so->so_proto->pr_unlock) {
		(*so->so_proto->pr_unlock)(so, refcount, lr_saved);
	} else {
		mutex_held = so->so_proto->pr_domain->dom_mtx;
#ifdef MORE_LOCKING_DEBUG
		LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);
#endif
		so->unlock_lr[so->next_unlock_lr] = lr_saved;
		so->next_unlock_lr = (so->next_unlock_lr + 1) % SO_LCKDBG_MAX;

		if (refcount) {
			if (so->so_usecount <= 0) {
				panic("%s: bad refcount=%d so=%p (%d, %d, %d) "
				    "lrh=%s", __func__, so->so_usecount, so,
				    SOCK_DOM(so), so->so_type,
				    SOCK_PROTO(so), solockhistory_nr(so));
				                
			}

			so->so_usecount--;
			if (so->so_usecount == 0) {
				sofreelastref(so, 1);
			}
		}
		lck_mtx_unlock(mutex_held);
	}
}

                                                   
void
sofree(struct socket *so)
{
	lck_mtx_t *mutex_held;

	if (so->so_proto->pr_getlock != NULL) {
		mutex_held = (*so->so_proto->pr_getlock)(so, 0);
	} else {
		mutex_held = so->so_proto->pr_domain->dom_mtx;
	}
	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);

	sofreelastref(so, 0);
}

void
soreference(struct socket *so)
{
	socket_lock(so, 1);                                               
	socket_unlock(so, 0);                    
}

void
sodereference(struct socket *so)
{
	socket_lock(so, 0);
	socket_unlock(so, 1);
}

  
                                                                     
                                                                   
                   
   
void
somultipages(struct socket *so, boolean_t set)
{
	if (set) {
		so->so_flags |= SOF_MULTIPAGES;
	} else {
		so->so_flags &= ~SOF_MULTIPAGES;
	}
}

void
soif2kcl(struct socket *so, boolean_t set)
{
	if (set) {
		so->so_flags1 |= SOF1_IF_2KCL;
	} else {
		so->so_flags1 &= ~SOF1_IF_2KCL;
	}
}

int
so_isdstlocal(struct socket *so)
{
	struct inpcb *inp = (struct inpcb *)so->so_pcb;

	if (SOCK_DOM(so) == PF_INET) {
		return inaddr_local(inp->inp_faddr);
	} else if (SOCK_DOM(so) == PF_INET6) {
		return in6addr_local(&inp->in6p_faddr);
	}

	return 0;
}

int
sosetdefunct(struct proc *p, struct socket *so, int level, boolean_t noforce)
{
	struct sockbuf *rcv, *snd;
	int err = 0, defunct;

	rcv = &so->so_rcv;
	snd = &so->so_snd;

	defunct = (so->so_flags & SOF_DEFUNCT);
	if (defunct) {
		if (!(snd->sb_flags & rcv->sb_flags & SB_DROP)) {
			panic("%s: SB_DROP not set", __func__);
			                
		}
		goto done;
	}

	if (so->so_flags & SOF_NODEFUNCT) {
		if (noforce) {
			err = EOPNOTSUPP;
			if (p != PROC_NULL) {
				SODEFUNCTLOG("%s[%d, %s]: (target pid %d "
				    "name %s level %d) so 0x%llx [%d,%d] "
				    "is not eligible for defunct "
				    "(%d)\n", __func__, proc_selfpid(),
				    proc_best_name(current_proc()), proc_pid(p),
				    proc_best_name(p), level,
				    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
				    SOCK_DOM(so), SOCK_TYPE(so), err);
			}
			return err;
		}
		so->so_flags &= ~SOF_NODEFUNCT;
		if (p != PROC_NULL) {
			SODEFUNCTLOG("%s[%d, %s]: (target pid %d "
			    "name %s level %d) so 0x%llx [%d,%d] "
			    "defunct by force "
			    "(%d)\n", __func__, proc_selfpid(),
			    proc_best_name(current_proc()), proc_pid(p),
			    proc_best_name(p), level,
			    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
			    SOCK_DOM(so), SOCK_TYPE(so), err);
		}
	} else if (so->so_flags1 & SOF1_EXTEND_BK_IDLE_WANTED) {
		struct inpcb *inp = (struct inpcb *)so->so_pcb;
		struct ifnet *ifp = inp->inp_last_outifp;

		if (ifp && IFNET_IS_CELLULAR(ifp)) {
			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_nocell);
		} else if (so->so_flags & SOF_DELEGATED) {
			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_nodlgtd);
		} else if (soextbkidlestat.so_xbkidle_time == 0) {
			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_notime);
		} else if (noforce && p != PROC_NULL) {
			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_active);

			so->so_flags1 |= SOF1_EXTEND_BK_IDLE_INPROG;
			so->so_extended_bk_start = net_uptime();
			OSBitOrAtomic(P_LXBKIDLEINPROG, &p->p_ladvflag);

			inpcb_timer_sched(inp->inp_pcbinfo, INPCB_TIMER_LAZY);

			err = EOPNOTSUPP;
			SODEFUNCTLOG("%s[%d, %s]: (target pid %d "
			    "name %s level %d) so 0x%llx [%d,%d] "
			    "extend bk idle "
			    "(%d)\n", __func__, proc_selfpid(),
			    proc_best_name(current_proc()), proc_pid(p),
			    proc_best_name(p), level,
			    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
			    SOCK_DOM(so), SOCK_TYPE(so), err);
			return err;
		} else {
			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_forced);
		}
	}

	so->so_flags |= SOF_DEFUNCT;

	                                                                    
	snd->sb_flags |= SB_DROP;
	rcv->sb_flags |= SB_DROP;

	                                                   
	if (rcv->sb_cc != 0) {
		rcv->sb_flags &= ~SB_SEL;
		selthreadclear(&rcv->sb_sel);
		sbrelease(rcv);
	}
	if (snd->sb_cc != 0) {
		snd->sb_flags &= ~SB_SEL;
		selthreadclear(&snd->sb_sel);
		sbrelease(snd);
	}

done:
	if (p != PROC_NULL) {
		SODEFUNCTLOG("%s[%d, %s]: (target pid %d name %s level %d) "
		    "so 0x%llx [%d,%d] %s defunct%s\n", __func__,
		    proc_selfpid(), proc_best_name(current_proc()),
		    proc_pid(p), proc_best_name(p), level,
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so), SOCK_DOM(so),
		    SOCK_TYPE(so), defunct ? "is already" : "marked as",
		    (so->so_flags1 & SOF1_EXTEND_BK_IDLE_WANTED) ?
		    " extbkidle" : "");
	}
	return err;
}

int
sodefunct(struct proc *p, struct socket *so, int level)
{
	struct sockbuf *rcv, *snd;

	if (!(so->so_flags & SOF_DEFUNCT)) {
		panic("%s improperly called", __func__);
		                
	}
	if (so->so_state & SS_DEFUNCT) {
		goto done;
	}

	rcv = &so->so_rcv;
	snd = &so->so_snd;

	if (SOCK_DOM(so) == PF_INET || SOCK_DOM(so) == PF_INET6) {
		char s[MAX_IPv6_STR_LEN];
		char d[MAX_IPv6_STR_LEN];
		struct inpcb *inp = sotoinpcb(so);

		if (p != PROC_NULL) {
			SODEFUNCTLOG(
				"%s[%d, %s]: (target pid %d name %s level %d) "
				"so 0x%llx [%s %s:%d -> %s:%d] is now defunct "
				"[rcv_si 0x%x, snd_si 0x%x, rcv_fl 0x%x, "
				" snd_fl 0x%x]\n", __func__,
				proc_selfpid(), proc_best_name(current_proc()),
				proc_pid(p), proc_best_name(p), level,
				(uint64_t)DEBUG_KERNEL_ADDRPERM(so),
				(SOCK_TYPE(so) == SOCK_STREAM) ? "TCP" : "UDP",
				inet_ntop(SOCK_DOM(so), ((SOCK_DOM(so) == PF_INET) ?
				(void *)&inp->inp_laddr.s_addr :
				(void *)&inp->in6p_laddr),
				s, sizeof(s)), ntohs(inp->in6p_lport),
				inet_ntop(SOCK_DOM(so), (SOCK_DOM(so) == PF_INET) ?
				(void *)&inp->inp_faddr.s_addr :
				(void *)&inp->in6p_faddr,
				d, sizeof(d)), ntohs(inp->in6p_fport),
				(uint32_t)rcv->sb_sel.si_flags,
				(uint32_t)snd->sb_sel.si_flags,
				rcv->sb_flags, snd->sb_flags);
		}
	} else if (p != PROC_NULL) {
		SODEFUNCTLOG("%s[%d, %s]: (target pid %d name %s level %d) "
		    "so 0x%llx [%d,%d] is now defunct [rcv_si 0x%x, "
		    "snd_si 0x%x, rcv_fl 0x%x, snd_fl 0x%x]\n", __func__,
		    proc_selfpid(), proc_best_name(current_proc()),
		    proc_pid(p), proc_best_name(p), level,
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    SOCK_DOM(so), SOCK_TYPE(so),
		    (uint32_t)rcv->sb_sel.si_flags,
		    (uint32_t)snd->sb_sel.si_flags, rcv->sb_flags,
		    snd->sb_flags);
	}

	  
                                                      
    
	sbwakeup(rcv);
	sbwakeup(snd);

	so->so_flags1 |= SOF1_DEFUNCTINPROG;
	if (rcv->sb_flags & SB_LOCK) {
		sbunlock(rcv, TRUE);                            
	}
	if (snd->sb_flags & SB_LOCK) {
		sbunlock(snd, TRUE);                            
	}
	  
                                                                  
                                                                
                                                                  
                                                
    
	(void) soshutdownlock_final(so, SHUT_RD);
	(void) soshutdownlock_final(so, SHUT_WR);
	(void) sodisconnectlocked(so);

	  
                                                           
                                                         
    
	if (!(so->so_state & SS_ISDISCONNECTED)) {
		(void) soisdisconnected(so);
	}

	if (so->so_error == 0) {
		so->so_error = EBADF;
	}

	if (rcv->sb_cc != 0) {
		rcv->sb_flags &= ~SB_SEL;
		selthreadclear(&rcv->sb_sel);
		sbrelease(rcv);
	}
	if (snd->sb_cc != 0) {
		snd->sb_flags &= ~SB_SEL;
		selthreadclear(&snd->sb_sel);
		sbrelease(snd);
	}
	so->so_state |= SS_DEFUNCT;
	OSIncrementAtomicLong((volatile long *)&sodefunct_calls);

done:
	return 0;
}

int
soresume(struct proc *p, struct socket *so, int locked)
{
	if (locked == 0) {
		socket_lock(so, 1);
	}

	if (so->so_flags1 & SOF1_EXTEND_BK_IDLE_INPROG) {
		SODEFUNCTLOG("%s[%d, %s]: (target pid %d name %s) so 0x%llx "
		    "[%d,%d] resumed from bk idle\n",
		    __func__, proc_selfpid(), proc_best_name(current_proc()),
		    proc_pid(p), proc_best_name(p),
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    SOCK_DOM(so), SOCK_TYPE(so));

		so->so_flags1 &= ~SOF1_EXTEND_BK_IDLE_INPROG;
		so->so_extended_bk_start = 0;
		OSBitAndAtomic(~P_LXBKIDLEINPROG, &p->p_ladvflag);

		OSIncrementAtomic(&soextbkidlestat.so_xbkidle_resumed);
		OSDecrementAtomic(&soextbkidlestat.so_xbkidle_active);
		VERIFY(soextbkidlestat.so_xbkidle_active >= 0);
	}
	if (locked == 0) {
		socket_unlock(so, 1);
	}

	return 0;
}

  
                                                                  
                      
   
int
so_set_extended_bk_idle(struct socket *so, int optval)
{
	int error = 0;

	if ((SOCK_DOM(so) != PF_INET && SOCK_DOM(so) != PF_INET6) ||
	    SOCK_PROTO(so) != IPPROTO_TCP) {
		OSDecrementAtomic(&soextbkidlestat.so_xbkidle_notsupp);
		error = EOPNOTSUPP;
	} else if (optval == 0) {
		so->so_flags1 &= ~SOF1_EXTEND_BK_IDLE_WANTED;

		soresume(current_proc(), so, 1);
	} else {
		struct proc *p = current_proc();
		struct fileproc *fp;
		int count = 0;

		  
                                                    
                           
     
		socket_unlock(so, 0);

		proc_fdlock(p);
		fdt_foreach(fp, p) {
			struct socket *so2;

			if (FILEGLOB_DTYPE(fp->fp_glob) != DTYPE_SOCKET) {
				continue;
			}

			so2 = (struct socket *)fp->fp_glob->fg_data;
			if (so != so2 &&
			    so2->so_flags1 & SOF1_EXTEND_BK_IDLE_WANTED) {
				count++;
			}
			if (count >= soextbkidlestat.so_xbkidle_maxperproc) {
				break;
			}
		}
		proc_fdunlock(p);

		socket_lock(so, 0);

		if (count >= soextbkidlestat.so_xbkidle_maxperproc) {
			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_toomany);
			error = EBUSY;
		} else if (so->so_flags & SOF_DELEGATED) {
			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_nodlgtd);
			error = EBUSY;
		} else {
			so->so_flags1 |= SOF1_EXTEND_BK_IDLE_WANTED;
			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_wantok);
		}
		SODEFUNCTLOG("%s[%d, %s]: so 0x%llx [%d,%d] "
		    "%s marked for extended bk idle\n",
		    __func__, proc_selfpid(), proc_best_name(current_proc()),
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    SOCK_DOM(so), SOCK_TYPE(so),
		    (so->so_flags1 & SOF1_EXTEND_BK_IDLE_WANTED) ?
		    "is" : "not");
	}

	return error;
}

static void
so_stop_extended_bk_idle(struct socket *so)
{
	so->so_flags1 &= ~SOF1_EXTEND_BK_IDLE_INPROG;
	so->so_extended_bk_start = 0;

	OSDecrementAtomic(&soextbkidlestat.so_xbkidle_active);
	VERIFY(soextbkidlestat.so_xbkidle_active >= 0);
	  
                 
    
	sosetdefunct(current_proc(), so,
	    SHUTDOWN_SOCKET_LEVEL_DISCONNECT_INTERNAL, FALSE);
	if (so->so_flags & SOF_DEFUNCT) {
		sodefunct(current_proc(), so,
		    SHUTDOWN_SOCKET_LEVEL_DISCONNECT_INTERNAL);
	}
}

void
so_drain_extended_bk_idle(struct socket *so)
{
	if (so && (so->so_flags1 & SOF1_EXTEND_BK_IDLE_INPROG)) {
		  
                                                     
     
		if (so->so_rcv.sb_cc || so->so_snd.sb_cc) {
			so_stop_extended_bk_idle(so);

			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_drained);
		}
	}
}

  
                                                                     
   
int
so_check_extended_bk_idle_time(struct socket *so)
{
	int ret = 1;

	if ((so->so_flags1 & SOF1_EXTEND_BK_IDLE_INPROG)) {
		SODEFUNCTLOG("%s[%d, %s]: so 0x%llx [%d,%d]\n",
		    __func__, proc_selfpid(), proc_best_name(current_proc()),
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    SOCK_DOM(so), SOCK_TYPE(so));
		if (net_uptime() - so->so_extended_bk_start >
		    soextbkidlestat.so_xbkidle_time) {
			so_stop_extended_bk_idle(so);

			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_expired);

			ret = 0;
		} else {
			struct inpcb *inp = (struct inpcb *)so->so_pcb;

			inpcb_timer_sched(inp->inp_pcbinfo, INPCB_TIMER_LAZY);
			OSIncrementAtomic(&soextbkidlestat.so_xbkidle_resched);
		}
	}

	return ret;
}

void
resume_proc_sockets(proc_t p)
{
	if (p->p_ladvflag & P_LXBKIDLEINPROG) {
		struct fileproc *fp;
		struct socket *so;

		proc_fdlock(p);
		fdt_foreach(fp, p) {
			if (FILEGLOB_DTYPE(fp->fp_glob) != DTYPE_SOCKET) {
				continue;
			}

			so = (struct socket *)fp->fp_glob->fg_data;
			(void) soresume(p, so, 0);
		}
		proc_fdunlock(p);

		OSBitAndAtomic(~P_LXBKIDLEINPROG, &p->p_ladvflag);
	}
}

__private_extern__ int
so_set_recv_anyif(struct socket *so, int optval)
{
	int ret = 0;

	if (SOCK_DOM(so) == PF_INET || SOCK_DOM(so) == PF_INET6) {
		if (optval) {
			sotoinpcb(so)->inp_flags |= INP_RECV_ANYIF;
		} else {
			sotoinpcb(so)->inp_flags &= ~INP_RECV_ANYIF;
		}
	}


	return ret;
}

__private_extern__ int
so_get_recv_anyif(struct socket *so)
{
	int ret = 0;

	if (SOCK_DOM(so) == PF_INET || SOCK_DOM(so) == PF_INET6) {
		ret = (sotoinpcb(so)->inp_flags & INP_RECV_ANYIF) ? 1 : 0;
	}

	return ret;
}

int
so_set_restrictions(struct socket *so, uint32_t vals)
{
	int nocell_old, nocell_new;
	int noexpensive_old, noexpensive_new;
	int noconstrained_old, noconstrained_new;

	  
                                                                 
                                                                 
                                                              
                                            
   
                                                                   
                                                                
                                                                    
                                                                 
                                                               
                                                            
    
	nocell_old = (so->so_restrictions & SO_RESTRICT_DENY_CELLULAR);
	noexpensive_old = (so->so_restrictions & SO_RESTRICT_DENY_EXPENSIVE);
	noconstrained_old = (so->so_restrictions & SO_RESTRICT_DENY_CONSTRAINED);
	so->so_restrictions |= (vals & (SO_RESTRICT_DENY_IN |
	    SO_RESTRICT_DENY_OUT | SO_RESTRICT_DENY_CELLULAR |
	    SO_RESTRICT_DENY_EXPENSIVE | SO_RESTRICT_DENY_CONSTRAINED));
	nocell_new = (so->so_restrictions & SO_RESTRICT_DENY_CELLULAR);
	noexpensive_new = (so->so_restrictions & SO_RESTRICT_DENY_EXPENSIVE);
	noconstrained_new = (so->so_restrictions & SO_RESTRICT_DENY_CONSTRAINED);

	                                             
	if ((nocell_new - nocell_old) == 0 &&
	    (noexpensive_new - noexpensive_old) == 0 &&
	    (noconstrained_new - noconstrained_old) == 0) {
		return 0;
	}
	if (SOCK_DOM(so) == PF_INET || SOCK_DOM(so) == PF_INET6) {
		if (nocell_new - nocell_old != 0) {
			  
                                                   
               
      
			inp_set_nocellular(sotoinpcb(so));
		}
		if (noexpensive_new - noexpensive_old != 0) {
			inp_set_noexpensive(sotoinpcb(so));
		}
		if (noconstrained_new - noconstrained_old != 0) {
			inp_set_noconstrained(sotoinpcb(so));
		}
	}

	if (SOCK_DOM(so) == PF_MULTIPATH) {
		mptcp_set_restrictions(so);
	}

	return 0;
}

uint32_t
so_get_restrictions(struct socket *so)
{
	return so->so_restrictions & (SO_RESTRICT_DENY_IN |
	       SO_RESTRICT_DENY_OUT |
	       SO_RESTRICT_DENY_CELLULAR | SO_RESTRICT_DENY_EXPENSIVE);
}

int
so_set_effective_pid(struct socket *so, int epid, struct proc *p, boolean_t check_cred)
{
	struct proc *ep = PROC_NULL;
	int error = 0;

	                                  
	if (epid == 0) {
		error = EINVAL;
		goto done;
	}

	  
                                                        
                                                         
                                         
    
	if (so->last_pid == 0 && p != kernproc) {
		error = EACCES;
		goto done;
	}

	  
                                                         
                                                          
                                                          
                                                          
    
	if (check_cred && (epid != so->last_pid || epid != proc_pid(p))) {
		if ((error = priv_check_cred(kauth_cred_get(),
		    PRIV_NET_PRIVILEGED_SOCKET_DELEGATE, 0))) {
			error = EACCES;
			goto done;
		}
	}

	                                                            
	if ((ep = proc_find(epid)) == PROC_NULL) {
		error = ESRCH;
		goto done;
	}

	  
                                                             
                                                           
                                                           
                                                          
                                                             
                                                             
                                                             
                                                               
    
	if (epid == proc_pid(p)) {
		so->so_flags &= ~SOF_DELEGATED;
		so->e_upid = 0;
		so->e_pid = 0;
		uuid_clear(so->e_uuid);
	} else {
		so->so_flags |= SOF_DELEGATED;
		so->e_upid = proc_uniqueid(ep);
		so->e_pid = proc_pid(ep);
		proc_getexecutableuuid(ep, so->e_uuid, sizeof(so->e_uuid));

#if defined(XNU_TARGET_OS_OSX)
		if (ep->p_responsible_pid != so->e_pid) {
			proc_t rp = proc_find(ep->p_responsible_pid);
			if (rp != PROC_NULL) {
				proc_getexecutableuuid(rp, so->so_ruuid, sizeof(so->so_ruuid));
				so->so_rpid = ep->p_responsible_pid;
				proc_rele(rp);
			} else {
				uuid_clear(so->so_ruuid);
				so->so_rpid = -1;
			}
		}
#endif
	}
	if (so->so_proto != NULL && so->so_proto->pr_update_last_owner != NULL) {
		(*so->so_proto->pr_update_last_owner)(so, NULL, ep);
	}
done:
	if (error == 0 && net_io_policy_log) {
		uuid_string_t buf;

		uuid_unparse(so->e_uuid, buf);
		log(LOG_DEBUG, "%s[%s,%d]: so 0x%llx [%d,%d] epid %d (%s) "
		    "euuid %s%s\n", __func__, proc_name_address(p),
		    proc_pid(p), (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    SOCK_DOM(so), SOCK_TYPE(so),
		    so->e_pid, proc_name_address(ep), buf,
		    ((so->so_flags & SOF_DELEGATED) ? " [delegated]" : ""));
	} else if (error != 0 && net_io_policy_log) {
		log(LOG_ERR, "%s[%s,%d]: so 0x%llx [%d,%d] epid %d (%s) "
		    "ERROR (%d)\n", __func__, proc_name_address(p),
		    proc_pid(p), (uint64_t)DEBUG_KERNEL_ADDRPERM(so),
		    SOCK_DOM(so), SOCK_TYPE(so),
		    epid, (ep == PROC_NULL) ? "PROC_NULL" :
		    proc_name_address(ep), error);
	}

	                                              
	if (error == 0) {
		so->so_policy_gencnt *= -1;
		so_update_policy(so);
#if NECP
		so_update_necp_policy(so, NULL, NULL);
#endif           
	}

	if (ep != PROC_NULL) {
		proc_rele(ep);
	}

	return error;
}

int
so_set_effective_uuid(struct socket *so, uuid_t euuid, struct proc *p, boolean_t check_cred)
{
	uuid_string_t buf;
	uuid_t uuid;
	int error = 0;

	                                                       
	if (uuid_is_null(euuid)) {
		error = EINVAL;
		goto done;
	}

	  
                                                        
                                                         
                                         
    
	if (so->last_pid == 0 && p != kernproc) {
		error = EACCES;
		goto done;
	}

	                                         
	proc_getexecutableuuid(p, uuid, sizeof(uuid));

	  
                                                         
                                                           
                                                           
                                                          
    
	if (check_cred &&
	    (uuid_compare(euuid, so->last_uuid) != 0 ||
	    uuid_compare(euuid, uuid) != 0)) {
		if ((error = priv_check_cred(kauth_cred_get(),
		    PRIV_NET_PRIVILEGED_SOCKET_DELEGATE, 0))) {
			error = EACCES;
			goto done;
		}
	}

	  
                                                             
                                                           
                                                           
                                                          
                                                            
                                                            
                                                           
                                                           
                             
    
	if (uuid_compare(euuid, uuid) == 0) {
		so->so_flags &= ~SOF_DELEGATED;
		so->e_upid = 0;
		so->e_pid = 0;
		uuid_clear(so->e_uuid);
	} else {
		so->so_flags |= SOF_DELEGATED;
		  
                                                         
                                                       
                                   
     
		so->e_upid = so->last_upid;
		so->e_pid = so->last_pid;
		uuid_copy(so->e_uuid, euuid);
	}
	  
                                                                        
                       
    
	if (so->so_proto != NULL && so->so_proto->pr_update_last_owner != NULL) {
		(*so->so_proto->pr_update_last_owner)(so, NULL, NULL);
	}
done:
	if (error == 0 && net_io_policy_log) {
		uuid_unparse(so->e_uuid, buf);
		log(LOG_DEBUG, "%s[%s,%d]: so 0x%llx [%d,%d] epid %d "
		    "euuid %s%s\n", __func__, proc_name_address(p), proc_pid(p),
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so), SOCK_DOM(so),
		    SOCK_TYPE(so), so->e_pid, buf,
		    ((so->so_flags & SOF_DELEGATED) ? " [delegated]" : ""));
	} else if (error != 0 && net_io_policy_log) {
		uuid_unparse(euuid, buf);
		log(LOG_DEBUG, "%s[%s,%d]: so 0x%llx [%d,%d] euuid %s "
		    "ERROR (%d)\n", __func__, proc_name_address(p), proc_pid(p),
		    (uint64_t)DEBUG_KERNEL_ADDRPERM(so), SOCK_DOM(so),
		    SOCK_TYPE(so), buf, error);
	}

	                                              
	if (error == 0) {
		so->so_policy_gencnt *= -1;
		so_update_policy(so);
#if NECP
		so_update_necp_policy(so, NULL, NULL);
#endif           
	}

	return error;
}

void
netpolicy_post_msg(uint32_t ev_code, struct netpolicy_event_data *ev_data,
    uint32_t ev_datalen)
{
	struct kev_msg ev_msg;

	  
                                                               
                                                            
                                                   
    
	VERIFY(ev_data != NULL && ev_datalen >= sizeof(*ev_data));

	bzero(&ev_msg, sizeof(ev_msg));
	ev_msg.vendor_code      = KEV_VENDOR_APPLE;
	ev_msg.kev_class        = KEV_NETWORK_CLASS;
	ev_msg.kev_subclass     = KEV_NETPOLICY_SUBCLASS;
	ev_msg.event_code       = ev_code;

	ev_msg.dv[0].data_ptr   = ev_data;
	ev_msg.dv[0].data_length = ev_datalen;

	kev_post_msg(&ev_msg);
}

void
socket_post_kev_msg(uint32_t ev_code,
    struct kev_socket_event_data *ev_data,
    uint32_t ev_datalen)
{
	struct kev_msg ev_msg;

	bzero(&ev_msg, sizeof(ev_msg));
	ev_msg.vendor_code = KEV_VENDOR_APPLE;
	ev_msg.kev_class = KEV_NETWORK_CLASS;
	ev_msg.kev_subclass = KEV_SOCKET_SUBCLASS;
	ev_msg.event_code = ev_code;

	ev_msg.dv[0].data_ptr = ev_data;
	ev_msg.dv[0].data_length = ev_datalen;

	kev_post_msg(&ev_msg);
}

void
socket_post_kev_msg_closed(struct socket *so)
{
	struct kev_socket_closed ev = {};
	struct sockaddr *socksa = NULL, *peersa = NULL;
	int err;

	if ((so->so_flags1 & SOF1_WANT_KEV_SOCK_CLOSED) == 0) {
		return;
	}
	err = (*so->so_proto->pr_usrreqs->pru_sockaddr)(so, &socksa);
	if (err == 0) {
		err = (*so->so_proto->pr_usrreqs->pru_peeraddr)(so,
		    &peersa);
		if (err == 0) {
			memcpy(&ev.ev_data.kev_sockname, socksa,
			    min(socksa->sa_len,
			    sizeof(ev.ev_data.kev_sockname)));
			memcpy(&ev.ev_data.kev_peername, peersa,
			    min(peersa->sa_len,
			    sizeof(ev.ev_data.kev_peername)));
			socket_post_kev_msg(KEV_SOCKET_CLOSED,
			    &ev.ev_data, sizeof(ev));
		}
	}
	FREE(socksa, M_SONAME);
	FREE(peersa, M_SONAME);
}
