/*
 * Copyright (c) 2000-2015 Apple Inc. All rights reserved.
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
 * Copyright (c) 1982, 1986, 1989, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * sendfile(2) and related extensions:
 * Copyright (c) 1998, David Greenman. All rights reserved.
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
 *	@(#)uipc_syscalls.c	8.4 (Berkeley) 2/21/94
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/proc_internal.h>
#include <sys/file_internal.h>
#include <sys/vnode_internal.h>
#include <sys/malloc.h>
#include <sys/mcache.h>
#include <sys/mbuf.h>
#include <kern/locks.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/signalvar.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/kernel.h>
#include <sys/uio_internal.h>
#include <sys/kauth.h>
#include <kern/task.h>
#include <sys/priv.h>
#include <sys/sysctl.h>
#include <sys/sys_domain.h>

#include <security/audit/audit.h>

#include <sys/kdebug.h>
#include <sys/sysproto.h>
#include <netinet/in.h>
#include <net/route.h>
#include <netinet/in_pcb.h>

#include <os/ptrtools.h>

#if CONFIG_MACF_SOCKET_SUBSET
#include <security/mac_framework.h>
#endif                        

#define f_flag fp_glob->fg_flag
#define f_ops fp_glob->fg_ops
#define f_data fp_glob->fg_data

#define DBG_LAYER_IN_BEG        NETDBG_CODE(DBG_NETSOCK, 0)
#define DBG_LAYER_IN_END        NETDBG_CODE(DBG_NETSOCK, 2)
#define DBG_LAYER_OUT_BEG       NETDBG_CODE(DBG_NETSOCK, 1)
#define DBG_LAYER_OUT_END       NETDBG_CODE(DBG_NETSOCK, 3)
#define DBG_FNC_SENDMSG         NETDBG_CODE(DBG_NETSOCK, (1 << 8) | 1)
#define DBG_FNC_SENDTO          NETDBG_CODE(DBG_NETSOCK, (2 << 8) | 1)
#define DBG_FNC_SENDIT          NETDBG_CODE(DBG_NETSOCK, (3 << 8) | 1)
#define DBG_FNC_RECVFROM        NETDBG_CODE(DBG_NETSOCK, (5 << 8))
#define DBG_FNC_RECVMSG         NETDBG_CODE(DBG_NETSOCK, (6 << 8))
#define DBG_FNC_RECVIT          NETDBG_CODE(DBG_NETSOCK, (7 << 8))
#define DBG_FNC_SENDFILE        NETDBG_CODE(DBG_NETSOCK, (10 << 8))
#define DBG_FNC_SENDFILE_WAIT   NETDBG_CODE(DBG_NETSOCK, ((10 << 8) | 1))
#define DBG_FNC_SENDFILE_READ   NETDBG_CODE(DBG_NETSOCK, ((10 << 8) | 2))
#define DBG_FNC_SENDFILE_SEND   NETDBG_CODE(DBG_NETSOCK, ((10 << 8) | 3))
#define DBG_FNC_SENDMSG_X       NETDBG_CODE(DBG_NETSOCK, (11 << 8))
#define DBG_FNC_RECVMSG_X       NETDBG_CODE(DBG_NETSOCK, (12 << 8))

#if DEBUG || DEVELOPMENT
#define DEBUG_KERNEL_ADDRPERM(_v) (_v)
#define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_KERNEL_ADDRPERM(_v) VM_KERNEL_ADDRPERM(_v)
#define DBG_PRINTF(...) do { } while (0)
#endif

static int sendit(struct proc *, struct socket *, struct user_msghdr *, uio_t,
    int, int32_t *);
static int recvit(struct proc *, int, struct user_msghdr *, uio_t, user_addr_t,
    int32_t *);
static int connectit(struct socket *, struct sockaddr *);
static int getsockaddr(struct socket *, struct sockaddr **, user_addr_t,
    size_t, boolean_t);
static int getsockaddr_s(struct socket *, struct sockaddr_storage *,
    user_addr_t, size_t, boolean_t);
#if SENDFILE
static void alloc_sendpkt(int, size_t, unsigned int *, struct mbuf **,
    boolean_t);
#endif               
static int connectx_nocancel(struct proc *, struct connectx_args *, int *);
static int connectitx(struct socket *, struct sockaddr *,
    struct sockaddr *, struct proc *, uint32_t, sae_associd_t,
    sae_connid_t *, uio_t, unsigned int, user_ssize_t *);
static int disconnectx_nocancel(struct proc *, struct disconnectx_args *,
    int *);
static int socket_common(struct proc *, int, int, int, pid_t, int32_t *, int);

static int internalize_user_msghdr_array(const void *, int, int, u_int,
    struct user_msghdr_x *, struct uio **);
static u_int externalize_user_msghdr_array(void *, int, int, u_int,
    const struct user_msghdr_x *, struct uio **);

static void free_uio_array(struct uio **, u_int);
static boolean_t uio_array_is_valid(struct uio **, u_int);
static int recv_msg_array_is_valid(struct recv_msg_elem *, u_int);
static int internalize_recv_msghdr_array(const void *, int, int,
    u_int, struct user_msghdr_x *, struct recv_msg_elem *);
static u_int externalize_recv_msghdr_array(struct proc *, struct socket *, void *, u_int,
    struct user_msghdr_x *, struct recv_msg_elem *, int *);
static struct recv_msg_elem *alloc_recv_msg_array(u_int count);
static void free_recv_msg_array(struct recv_msg_elem *, u_int);

SYSCTL_DECL(_kern_ipc);

static u_int somaxsendmsgx = 100;
SYSCTL_UINT(_kern_ipc, OID_AUTO, maxsendmsgx,
    CTLFLAG_RW | CTLFLAG_LOCKED, &somaxsendmsgx, 0, "");
static u_int somaxrecvmsgx = 100;
SYSCTL_UINT(_kern_ipc, OID_AUTO, maxrecvmsgx,
    CTLFLAG_RW | CTLFLAG_LOCKED, &somaxrecvmsgx, 0, "");

  
                                                   
   

extern const struct fileops socketops;

  
                       
                                             
                
                
                
                        
                      
                           
                   
                  
                                                  
   
int
socket(struct proc *p,
    struct socket_args *uap,
    int32_t *retval)
{
	return socket_common(p, uap->domain, uap->type, uap->protocol,
	           proc_selfpid(), retval, 0);
}

int
socket_delegate(struct proc *p,
    struct socket_delegate_args *uap,
    int32_t *retval)
{
	return socket_common(p, uap->domain, uap->type, uap->protocol,
	           uap->epid, retval, 1);
}

static int
socket_common(struct proc *p,
    int domain,
    int type,
    int protocol,
    pid_t epid,
    int32_t *retval,
    int delegate)
{
	struct socket *so;
	struct fileproc *fp;
	int fd, error;

	AUDIT_ARG(socket, domain, type, protocol);
#if CONFIG_MACF_SOCKET_SUBSET
	if ((error = mac_socket_check_create(kauth_cred_get(), domain,
	    type, protocol)) != 0) {
		return error;
	}
#endif                        

	if (delegate) {
		error = priv_check_cred(kauth_cred_get(),
		    PRIV_NET_PRIVILEGED_SOCKET_DELEGATE, 0);
		if (error) {
			return EACCES;
		}
	}

	error = falloc(p, &fp, &fd, vfs_context_current());
	if (error) {
		return error;
	}
	fp->f_flag = FREAD | FWRITE;
	fp->f_ops = &socketops;

	if (delegate) {
		error = socreate_delegate(domain, &so, type, protocol, epid);
	} else {
		error = socreate(domain, &so, type, protocol);
	}

	if (error) {
		fp_free(p, fd, fp);
	} else {
		fp->f_data = (caddr_t)so;

		proc_fdlock(p);
		procfdtbl_releasefd(p, fd, NULL);

		fp_drop(p, fd, fp, 1);
		proc_fdunlock(p);

		*retval = fd;
		if (ENTR_SHOULDTRACE) {
			KERNEL_ENERGYTRACE(kEnTrActKernSocket, DBG_FUNC_START,
			    fd, 0, (int64_t)VM_KERNEL_ADDRPERM(so));
		}
	}
	return error;
}

  
                       
                                              
                               
                                             
                       
                    
                                             
                                       
                                       
                                  
                 
   
              
int
bind(__unused proc_t p, struct bind_args *uap, __unused int32_t *retval)
{
	struct sockaddr_storage ss;
	struct sockaddr *sa = NULL;
	struct socket *so;
	boolean_t want_free = TRUE;
	int error;

	AUDIT_ARG(fd, uap->s);
	error = file_socket(uap->s, &so);
	if (error != 0) {
		return error;
	}
	if (so == NULL) {
		error = EBADF;
		goto out;
	}
	if (uap->name == USER_ADDR_NULL) {
		error = EDESTADDRREQ;
		goto out;
	}
	if (uap->namelen > sizeof(ss)) {
		error = getsockaddr(so, &sa, uap->name, uap->namelen, TRUE);
	} else {
		error = getsockaddr_s(so, &ss, uap->name, uap->namelen, TRUE);
		if (error == 0) {
			sa = (struct sockaddr *)&ss;
			want_free = FALSE;
		}
	}
	if (error != 0) {
		goto out;
	}
	AUDIT_ARG(sockaddr, vfs_context_cwd(vfs_context_current()), sa);
#if CONFIG_MACF_SOCKET_SUBSET
	if ((sa != NULL && sa->sa_family == AF_SYSTEM) ||
	    (error = mac_socket_check_bind(kauth_cred_get(), so, sa)) == 0) {
		error = sobindlock(so, sa, 1);                        
	}
#else
	error = sobindlock(so, sa, 1);                                
#endif                        
	if (want_free) {
		FREE(sa, M_SONAME);
	}
out:
	file_drop(uap->s);
	return error;
}

  
                       
         
                                             
                       
                    
                  
                      
               
   
int
listen(__unused struct proc *p, struct listen_args *uap,
    __unused int32_t *retval)
{
	int error;
	struct socket *so;

	AUDIT_ARG(fd, uap->s);
	error = file_socket(uap->s, &so);
	if (error) {
		return error;
	}
	if (so != NULL)
#if CONFIG_MACF_SOCKET_SUBSET
	{
		error = mac_socket_check_listen(kauth_cred_get(), so);
		if (error == 0) {
			error = solisten(so, uap->backlog);
		}
	}
#else
	{ error = solisten(so, uap->backlog);}
#endif                        
	else {
		error = EBADF;
	}

	file_drop(uap->s);
	return error;
}

  
                                                  
                                                        
                                           
                                
                                                  
                              
                                       
                                     
                                 
                                              
                                                
                                      
                                   
               
   
int
accept_nocancel(struct proc *p, struct accept_nocancel_args *uap,
    int32_t *retval)
{
	struct fileproc *fp;
	struct sockaddr *sa = NULL;
	socklen_t namelen;
	int error;
	struct socket *head, *so = NULL;
	lck_mtx_t *mutex_held;
	int fd = uap->s;
	int newfd;
	unsigned int fflag;
	int dosocklock = 0;

	*retval = -1;

	AUDIT_ARG(fd, uap->s);

	if (uap->name) {
		error = copyin(uap->anamelen, (caddr_t)&namelen,
		    sizeof(socklen_t));
		if (error) {
			return error;
		}
	}
	error = fp_get_ftype(p, fd, DTYPE_SOCKET, ENOTSOCK, &fp);
	if (error) {
		return error;
	}
	head = fp->f_data;

#if CONFIG_MACF_SOCKET_SUBSET
	if ((error = mac_socket_check_accept(kauth_cred_get(), head)) != 0) {
		goto out;
	}
#endif                        

	socket_lock(head, 1);

	if (head->so_proto->pr_getlock != NULL) {
		mutex_held = (*head->so_proto->pr_getlock)(head, PR_F_WILLUNLOCK);
		dosocklock = 1;
	} else {
		mutex_held = head->so_proto->pr_domain->dom_mtx;
		dosocklock = 0;
	}

	if ((head->so_options & SO_ACCEPTCONN) == 0) {
		if ((head->so_proto->pr_flags & PR_CONNREQUIRED) == 0) {
			error = EOPNOTSUPP;
		} else {
			                                                    
			error = EINVAL;
		}
		socket_unlock(head, 1);
		goto out;
	}
check_again:
	if ((head->so_state & SS_NBIO) && head->so_comp.tqh_first == NULL) {
		socket_unlock(head, 1);
		error = EWOULDBLOCK;
		goto out;
	}
	while (TAILQ_EMPTY(&head->so_comp) && head->so_error == 0) {
		if (head->so_state & SS_CANTRCVMORE) {
			head->so_error = ECONNABORTED;
			break;
		}
		if (head->so_usecount < 1) {
			panic("accept: head=%p refcount=%d\n", head,
			    head->so_usecount);
		}
		error = msleep((caddr_t)&head->so_timeo, mutex_held,
		    PSOCK | PCATCH, "accept", 0);
		if (head->so_usecount < 1) {
			panic("accept: 2 head=%p refcount=%d\n", head,
			    head->so_usecount);
		}
		if ((head->so_state & SS_DRAINING)) {
			error = ECONNABORTED;
		}
		if (error) {
			socket_unlock(head, 1);
			goto out;
		}
	}
	if (head->so_error) {
		error = head->so_error;
		head->so_error = 0;
		socket_unlock(head, 1);
		goto out;
	}

	  
                                                               
                                                           
                                                            
                                                           
            
    
	lck_mtx_assert(mutex_held, LCK_MTX_ASSERT_OWNED);

	so_acquire_accept_list(head, NULL);
	if (TAILQ_EMPTY(&head->so_comp)) {
		so_release_accept_list(head);
		goto check_again;
	}

	so = TAILQ_FIRST(&head->so_comp);
	TAILQ_REMOVE(&head->so_comp, so, so_list);
	so->so_head = NULL;
	so->so_state &= ~SS_COMP;
	head->so_qlen--;
	so_release_accept_list(head);

	                                                                   
	socket_unlock(head, 0);

#if CONFIG_MACF_SOCKET_SUBSET
	  
                                                              
                                                             
                                                              
                                                   
    
	if ((error = mac_socket_check_accepted(kauth_cred_get(), so)) != 0) {
		socket_lock(so, 1);
		so->so_state &= ~SS_NOFDREF;
		socket_unlock(so, 1);
		soclose(so);
		                                        
		sodereference(head);
		goto out;
	}
#endif                        

	  
                                                                    
                                                                  
    
	if (so->so_filt != NULL && (error = soacceptfilter(so, head)) != 0) {
		                                        
		sodereference(head);
		                                                        
		goto out;
	}

	fflag = fp->f_flag;
	error = falloc(p, &fp, &newfd, vfs_context_current());
	if (error) {
		  
                                          
    
                             
                                                            
                                                     
     
		socket_lock(so, 1);
		so->so_state &= ~SS_NOFDREF;
		socket_unlock(so, 1);
		soclose(so);
		sodereference(head);
		goto out;
	}
	*retval = newfd;
	fp->f_flag = fflag;
	fp->f_ops = &socketops;
	fp->f_data = (caddr_t)so;

	socket_lock(head, 0);
	if (dosocklock) {
		socket_lock(so, 1);
	}

	                                                          
	if (fp->f_flag & FNONBLOCK) {
		so->so_state |= SS_NBIO;
	} else {
		so->so_state &= ~SS_NBIO;
	}

	if (fp->f_flag & FASYNC) {
		so->so_state |= SS_ASYNC;
		so->so_rcv.sb_flags |= SB_ASYNC;
		so->so_snd.sb_flags |= SB_ASYNC;
	} else {
		so->so_state &= ~SS_ASYNC;
		so->so_rcv.sb_flags &= ~SB_ASYNC;
		so->so_snd.sb_flags &= ~SB_ASYNC;
	}

	(void) soacceptlock(so, &sa, 0);
	socket_unlock(head, 1);
	if (sa == NULL) {
		namelen = 0;
		if (uap->name) {
			goto gotnoname;
		}
		error = 0;
		goto releasefd;
	}
	AUDIT_ARG(sockaddr, vfs_context_cwd(vfs_context_current()), sa);

	if (uap->name) {
		socklen_t       sa_len;

		                                        
		sa_len = sa->sa_len;
		namelen = MIN(namelen, sa_len);
		error = copyout(sa, uap->name, namelen);
		if (!error) {
			                                                   
			namelen = sa_len;
		}
gotnoname:
		error = copyout((caddr_t)&namelen, uap->anamelen,
		    sizeof(socklen_t));
	}
	FREE(sa, M_SONAME);

releasefd:
	  
                                                                
                                      
    
	if (so->so_flags & SOF_DEFUNCT) {
		sodefunct(current_proc(), so,
		    SHUTDOWN_SOCKET_LEVEL_DISCONNECT_INTERNAL);
	}

	if (dosocklock) {
		socket_unlock(so, 1);
	}

	proc_fdlock(p);
	procfdtbl_releasefd(p, newfd, NULL);
	fp_drop(p, newfd, fp, 1);
	proc_fdunlock(p);

out:
	file_drop(fd);

	if (error == 0 && ENTR_SHOULDTRACE) {
		KERNEL_ENERGYTRACE(kEnTrActKernSocket, DBG_FUNC_START,
		    newfd, 0, (int64_t)VM_KERNEL_ADDRPERM(so));
	}
	return error;
}

int
accept(struct proc *p, struct accept_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return accept_nocancel(p, (struct accept_nocancel_args *)uap,
	           retval);
}

  
                       
                               
                                            
                                      
                                    
                                
                                             
                       
                    
                                             
                                       
                                       
                                  
                           
                        
                                                    
               
  
                                                           
                                          
   
              
int
connect(struct proc *p, struct connect_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return connect_nocancel(p, (struct connect_nocancel_args *)uap,
	           retval);
}

int
connect_nocancel(proc_t p, struct connect_nocancel_args *uap, int32_t *retval)
{
#pragma unused(p, retval)
	struct socket *so;
	struct sockaddr_storage ss;
	struct sockaddr *sa = NULL;
	int error;
	int fd = uap->s;
	boolean_t dgram;

	AUDIT_ARG(fd, uap->s);
	error = file_socket(fd, &so);
	if (error != 0) {
		return error;
	}
	if (so == NULL) {
		error = EBADF;
		goto out;
	}

	  
                                                             
                                                            
    
	dgram = (so->so_type == SOCK_DGRAM);

	                                                         
	if (uap->namelen > sizeof(ss)) {
		error = getsockaddr(so, &sa, uap->name, uap->namelen, !dgram);
	} else {
		error = getsockaddr_s(so, &ss, uap->name, uap->namelen, !dgram);
		if (error == 0) {
			sa = (struct sockaddr *)&ss;
		}
	}
	if (error != 0) {
		goto out;
	}

	error = connectit(so, sa);

	if (sa != NULL && sa != SA(&ss)) {
		FREE(sa, M_SONAME);
	}
	if (error == ERESTART) {
		error = EINTR;
	}
out:
	file_drop(fd);
	return error;
}

static int
connectx_nocancel(struct proc *p, struct connectx_args *uap, int *retval)
{
#pragma unused(p, retval)
	struct sockaddr_storage ss, sd;
	struct sockaddr *src = NULL, *dst = NULL;
	struct socket *so;
	int error, error1, fd = uap->socket;
	boolean_t dgram;
	sae_connid_t cid = SAE_CONNID_ANY;
	struct user32_sa_endpoints ep32;
	struct user64_sa_endpoints ep64;
	struct user_sa_endpoints ep;
	user_ssize_t bytes_written = 0;
	struct user_iovec *iovp;
	uio_t auio = NULL;

	AUDIT_ARG(fd, uap->socket);
	error = file_socket(fd, &so);
	if (error != 0) {
		return error;
	}
	if (so == NULL) {
		error = EBADF;
		goto out;
	}

	if (uap->endpoints == USER_ADDR_NULL) {
		error = EINVAL;
		goto out;
	}

	if (IS_64BIT_PROCESS(p)) {
		error = copyin(uap->endpoints, (caddr_t)&ep64, sizeof(ep64));
		if (error != 0) {
			goto out;
		}

		ep.sae_srcif = ep64.sae_srcif;
		ep.sae_srcaddr = (user_addr_t)ep64.sae_srcaddr;
		ep.sae_srcaddrlen = ep64.sae_srcaddrlen;
		ep.sae_dstaddr = (user_addr_t)ep64.sae_dstaddr;
		ep.sae_dstaddrlen = ep64.sae_dstaddrlen;
	} else {
		error = copyin(uap->endpoints, (caddr_t)&ep32, sizeof(ep32));
		if (error != 0) {
			goto out;
		}

		ep.sae_srcif = ep32.sae_srcif;
		ep.sae_srcaddr = ep32.sae_srcaddr;
		ep.sae_srcaddrlen = ep32.sae_srcaddrlen;
		ep.sae_dstaddr = ep32.sae_dstaddr;
		ep.sae_dstaddrlen = ep32.sae_dstaddrlen;
	}

	  
                                                             
                                                            
    
	dgram = (so->so_type == SOCK_DGRAM);

	                                                         
	if (ep.sae_srcaddr != USER_ADDR_NULL) {
		if (ep.sae_srcaddrlen > sizeof(ss)) {
			error = getsockaddr(so, &src, ep.sae_srcaddr, ep.sae_srcaddrlen, dgram);
		} else {
			error = getsockaddr_s(so, &ss, ep.sae_srcaddr, ep.sae_srcaddrlen, dgram);
			if (error == 0) {
				src = (struct sockaddr *)&ss;
			}
		}

		if (error) {
			goto out;
		}
	}

	if (ep.sae_dstaddr == USER_ADDR_NULL) {
		error = EINVAL;
		goto out;
	}

	                                                         
	if (ep.sae_dstaddrlen > sizeof(sd)) {
		error = getsockaddr(so, &dst, ep.sae_dstaddr, ep.sae_dstaddrlen, dgram);
	} else {
		error = getsockaddr_s(so, &sd, ep.sae_dstaddr, ep.sae_dstaddrlen, dgram);
		if (error == 0) {
			dst = (struct sockaddr *)&sd;
		}
	}

	if (error) {
		goto out;
	}

	VERIFY(dst != NULL);

	if (uap->iov != USER_ADDR_NULL) {
		                                              
		if (uap->iovcnt <= 0 || uap->iovcnt > UIO_MAXIOV) {
			error = EINVAL;
			goto out;
		}

		if (uap->len == USER_ADDR_NULL) {
			error = EINVAL;
			goto out;
		}

		                                                        
		auio = uio_create(uap->iovcnt, 0,
		    (IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32),
		    UIO_WRITE);

		if (auio == NULL) {
			error = ENOMEM;
			goto out;
		}

		  
                                           
                                            
     
		iovp = uio_iovsaddr(auio);
		if (iovp == NULL) {
			error = ENOMEM;
			goto out;
		}
		error = copyin_user_iovec_array(uap->iov,
		    IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32,
		    uap->iovcnt, iovp);
		if (error != 0) {
			goto out;
		}

		                           
		error = uio_calculateresid(auio);
		if (error != 0) {
			goto out;
		}
	}

	error = connectitx(so, src, dst, p, ep.sae_srcif, uap->associd,
	    &cid, auio, uap->flags, &bytes_written);
	if (error == ERESTART) {
		error = EINTR;
	}

	if (uap->len != USER_ADDR_NULL) {
		error1 = copyout(&bytes_written, uap->len, sizeof(uap->len));
		                                          
		if ((error1 != 0) && (error == 0)) {
			error = error1;
		}
	}

	if (uap->connid != USER_ADDR_NULL) {
		error1 = copyout(&cid, uap->connid, sizeof(cid));
		                                          
		if ((error1 != 0) && (error == 0)) {
			error = error1;
		}
	}
out:
	file_drop(fd);
	if (auio != NULL) {
		uio_free(auio);
	}
	if (src != NULL && src != SA(&ss)) {
		FREE(src, M_SONAME);
	}
	if (dst != NULL && dst != SA(&sd)) {
		FREE(dst, M_SONAME);
	}
	return error;
}

int
connectx(struct proc *p, struct connectx_args *uap, int *retval)
{
	  
                                                        
                                     
    
	__pthread_testcancel(1);
	return connectx_nocancel(p, uap, retval);
}

static int
connectit(struct socket *so, struct sockaddr *sa)
{
	int error;

	AUDIT_ARG(sockaddr, vfs_context_cwd(vfs_context_current()), sa);
#if CONFIG_MACF_SOCKET_SUBSET
	if ((error = mac_socket_check_connect(kauth_cred_get(), so, sa)) != 0) {
		return error;
	}
#endif                        

	socket_lock(so, 1);
	if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING)) {
		error = EALREADY;
		goto out;
	}
	error = soconnectlock(so, sa, 0);
	if (error != 0) {
		goto out;
	}
	if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING)) {
		error = EINPROGRESS;
		goto out;
	}
	while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0) {
		lck_mtx_t *mutex_held;

		if (so->so_proto->pr_getlock != NULL) {
			mutex_held = (*so->so_proto->pr_getlock)(so, PR_F_WILLUNLOCK);
		} else {
			mutex_held = so->so_proto->pr_domain->dom_mtx;
		}
		error = msleep((caddr_t)&so->so_timeo, mutex_held,
		    PSOCK | PCATCH, __func__, 0);
		if (so->so_state & SS_DRAINING) {
			error = ECONNABORTED;
		}
		if (error != 0) {
			break;
		}
	}
	if (error == 0) {
		error = so->so_error;
		so->so_error = 0;
	}
out:
	socket_unlock(so, 1);
	return error;
}

static int
connectitx(struct socket *so, struct sockaddr *src,
    struct sockaddr *dst, struct proc *p, uint32_t ifscope,
    sae_associd_t aid, sae_connid_t *pcid, uio_t auio, unsigned int flags,
    user_ssize_t *bytes_written)
{
	int error;

	VERIFY(dst != NULL);

	AUDIT_ARG(sockaddr, vfs_context_cwd(vfs_context_current()), dst);
#if CONFIG_MACF_SOCKET_SUBSET
	if ((error = mac_socket_check_connect(kauth_cred_get(), so, dst)) != 0) {
		return error;
	}

	if (auio != NULL) {
		if ((error = mac_socket_check_send(kauth_cred_get(), so, dst)) != 0) {
			return error;
		}
	}
#endif                        

	socket_lock(so, 1);
	if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING)) {
		error = EALREADY;
		goto out;
	}

	error = soconnectxlocked(so, src, dst, p, ifscope,
	    aid, pcid, flags, NULL, 0, auio, bytes_written);
	if (error != 0) {
		goto out;
	}
	  
                                                                         
                                                                       
                                                                         
                                      
    
	if (so->so_flags1 & SOF1_PRECONNECT_DATA) {
		error = 0;
		goto out;
	}
	if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING)) {
		error = EINPROGRESS;
		goto out;
	}
	while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0) {
		lck_mtx_t *mutex_held;

		if (so->so_proto->pr_getlock != NULL) {
			mutex_held = (*so->so_proto->pr_getlock)(so, PR_F_WILLUNLOCK);
		} else {
			mutex_held = so->so_proto->pr_domain->dom_mtx;
		}
		error = msleep((caddr_t)&so->so_timeo, mutex_held,
		    PSOCK | PCATCH, __func__, 0);
		if (so->so_state & SS_DRAINING) {
			error = ECONNABORTED;
		}
		if (error != 0) {
			break;
		}
	}
	if (error == 0) {
		error = so->so_error;
		so->so_error = 0;
	}
out:
	socket_unlock(so, 1);
	return error;
}

int
peeloff(struct proc *p, struct peeloff_args *uap, int *retval)
{
#pragma unused(p, uap, retval)
	  
                                                        
                                     
    
	__pthread_testcancel(1);
	return 0;
}

int
disconnectx(struct proc *p, struct disconnectx_args *uap, int *retval)
{
	  
                                                        
                                     
    
	__pthread_testcancel(1);
	return disconnectx_nocancel(p, uap, retval);
}

static int
disconnectx_nocancel(struct proc *p, struct disconnectx_args *uap, int *retval)
{
#pragma unused(p, retval)
	struct socket *so;
	int fd = uap->s;
	int error;

	error = file_socket(fd, &so);
	if (error != 0) {
		return error;
	}
	if (so == NULL) {
		error = EBADF;
		goto out;
	}

	error = sodisconnectx(so, uap->aid, uap->cid);
out:
	file_drop(fd);
	return error;
}

  
                       
                        
                      
                           
                   
                  
                   
                                                  
                
                
                
                 
                    
                        
                                             
   
int
socketpair(struct proc *p, struct socketpair_args *uap,
    __unused int32_t *retval)
{
	struct fileproc *fp1, *fp2;
	struct socket *so1, *so2;
	int fd, error, sv[2];

	AUDIT_ARG(socket, uap->domain, uap->type, uap->protocol);
	error = socreate(uap->domain, &so1, uap->type, uap->protocol);
	if (error) {
		return error;
	}
	error = socreate(uap->domain, &so2, uap->type, uap->protocol);
	if (error) {
		goto free1;
	}

	error = falloc(p, &fp1, &fd, vfs_context_current());
	if (error) {
		goto free2;
	}
	fp1->f_flag = FREAD | FWRITE;
	fp1->f_ops = &socketops;
	fp1->f_data = (caddr_t)so1;
	sv[0] = fd;

	error = falloc(p, &fp2, &fd, vfs_context_current());
	if (error) {
		goto free3;
	}
	fp2->f_flag = FREAD | FWRITE;
	fp2->f_ops = &socketops;
	fp2->f_data = (caddr_t)so2;
	sv[1] = fd;

	error = soconnect2(so1, so2);
	if (error) {
		goto free4;
	}
	if (uap->type == SOCK_DGRAM) {
		  
                                              
     
		error = soconnect2(so2, so1);
		if (error) {
			goto free4;
		}
	}

	if ((error = copyout(sv, uap->rsv, 2 * sizeof(int))) != 0) {
		goto free4;
	}

	proc_fdlock(p);
	procfdtbl_releasefd(p, sv[0], NULL);
	procfdtbl_releasefd(p, sv[1], NULL);
	fp_drop(p, sv[0], fp1, 1);
	fp_drop(p, sv[1], fp2, 1);
	proc_fdunlock(p);

	return 0;
free4:
	fp_free(p, sv[1], fp2);
free3:
	fp_free(p, sv[0], fp1);
free2:
	(void) soclose(so2);
free1:
	(void) soclose(so1);
	return error;
}

  
                       
          
           
         
         
                                             
                       
                    
                                             
                                       
                                       
                                  
                           
                               
                                  
                                 
                           
                     
                               
                      
                                 
                     
                      
                                
                             
                             
                                
                       
                           
                                 
                          
                          
                     
                           
                                                                 
                                                               
                                          
               
   
static int
sendit(struct proc *p, struct socket *so, struct user_msghdr *mp, uio_t uiop,
    int flags, int32_t *retval)
{
	struct mbuf *control = NULL;
	struct sockaddr_storage ss;
	struct sockaddr *to = NULL;
	boolean_t want_free = TRUE;
	int error;
	user_ssize_t len;

	KERNEL_DEBUG(DBG_FNC_SENDIT | DBG_FUNC_START, 0, 0, 0, 0, 0);

	if (mp->msg_name != USER_ADDR_NULL) {
		if (mp->msg_namelen > sizeof(ss)) {
			error = getsockaddr(so, &to, mp->msg_name,
			    mp->msg_namelen, TRUE);
		} else {
			error = getsockaddr_s(so, &ss, mp->msg_name,
			    mp->msg_namelen, TRUE);
			if (error == 0) {
				to = (struct sockaddr *)&ss;
				want_free = FALSE;
			}
		}
		if (error != 0) {
			goto out;
		}
		AUDIT_ARG(sockaddr, vfs_context_cwd(vfs_context_current()), to);
	}
	if (mp->msg_control != USER_ADDR_NULL) {
		if (mp->msg_controllen < sizeof(struct cmsghdr)) {
			error = EINVAL;
			goto bad;
		}
		error = sockargs(&control, mp->msg_control,
		    mp->msg_controllen, MT_CONTROL);
		if (error != 0) {
			goto bad;
		}
	}

#if CONFIG_MACF_SOCKET_SUBSET
	  
                                                       
                                                      
                                               
    
	if (to != NULL &&
	    !(so->so_state & SS_DEFUNCT) &&
	    (error = mac_socket_check_send(kauth_cred_get(), so, to)) != 0) {
		goto bad;
	}
#endif                        

	len = uio_resid(uiop);
	error = so->so_proto->pr_usrreqs->pru_sosend(so, to, uiop, 0,
	    control, flags);
	if (error != 0) {
		if (uio_resid(uiop) != len && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK)) {
			error = 0;
		}
		                                                        
		if (error == EPIPE && !(so->so_flags & SOF_NOSIGPIPE) &&
		    !(flags & MSG_NOSIGNAL)) {
			psignal(p, SIGPIPE);
		}
	}
	if (error == 0) {
		*retval = (int)(len - uio_resid(uiop));
	}
bad:
	if (want_free) {
		FREE(to, M_SONAME);
	}
out:
	KERNEL_DEBUG(DBG_FNC_SENDIT | DBG_FUNC_END, error, 0, 0, 0, 0);

	return error;
}

  
                       
          
                                                    
                                              
   
int
sendto(struct proc *p, struct sendto_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return sendto_nocancel(p, (struct sendto_nocancel_args *)uap, retval);
}

int
sendto_nocancel(struct proc *p,
    struct sendto_nocancel_args *uap,
    int32_t *retval)
{
	struct user_msghdr msg;
	int error;
	uio_t auio = NULL;
	struct socket *so;

	KERNEL_DEBUG(DBG_FNC_SENDTO | DBG_FUNC_START, 0, 0, 0, 0, 0);
	AUDIT_ARG(fd, uap->s);

	if (uap->flags & MSG_SKIPCFIL) {
		error = EPERM;
		goto done;
	}

	if (uap->len > LONG_MAX) {
		error = EINVAL;
		goto done;
	}

	auio = uio_create(1, 0,
	    (IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32),
	    UIO_WRITE);
	if (auio == NULL) {
		error = ENOMEM;
		goto done;
	}
	uio_addiov(auio, uap->buf, uap->len);

	msg.msg_name = uap->to;
	msg.msg_namelen = uap->tolen;
	                                                              
	msg.msg_iov = 0;
	msg.msg_iovlen = 0;
	msg.msg_control = 0;
	msg.msg_flags = 0;

	error = file_socket(uap->s, &so);
	if (error) {
		goto done;
	}

	if (so == NULL) {
		error = EBADF;
	} else {
		error = sendit(p, so, &msg, auio, uap->flags, retval);
	}

	file_drop(uap->s);
done:
	if (auio != NULL) {
		uio_free(auio);
	}

	KERNEL_DEBUG(DBG_FNC_SENDTO | DBG_FUNC_END, error, *retval, 0, 0, 0);

	return error;
}

  
                       
           
                
                                                    
   
int
sendmsg(struct proc *p, struct sendmsg_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return sendmsg_nocancel(p, (struct sendmsg_nocancel_args *)uap,
	           retval);
}

int
sendmsg_nocancel(struct proc *p, struct sendmsg_nocancel_args *uap,
    int32_t *retval)
{
	struct user32_msghdr msg32;
	struct user64_msghdr msg64;
	struct user_msghdr user_msg;
	caddr_t msghdrp;
	int     size_of_msghdr;
	int error;
	uio_t auio = NULL;
	struct user_iovec *iovp;
	struct socket *so;

	KERNEL_DEBUG(DBG_FNC_SENDMSG | DBG_FUNC_START, 0, 0, 0, 0, 0);
	AUDIT_ARG(fd, uap->s);

	if (uap->flags & MSG_SKIPCFIL) {
		error = EPERM;
		goto done;
	}

	if (IS_64BIT_PROCESS(p)) {
		msghdrp = (caddr_t)&msg64;
		size_of_msghdr = sizeof(msg64);
	} else {
		msghdrp = (caddr_t)&msg32;
		size_of_msghdr = sizeof(msg32);
	}
	error = copyin(uap->msg, msghdrp, size_of_msghdr);
	if (error) {
		KERNEL_DEBUG(DBG_FNC_SENDMSG | DBG_FUNC_END, error, 0, 0, 0, 0);
		return error;
	}

	if (IS_64BIT_PROCESS(p)) {
		user_msg.msg_flags = msg64.msg_flags;
		user_msg.msg_controllen = msg64.msg_controllen;
		user_msg.msg_control = (user_addr_t)msg64.msg_control;
		user_msg.msg_iovlen = msg64.msg_iovlen;
		user_msg.msg_iov = (user_addr_t)msg64.msg_iov;
		user_msg.msg_namelen = msg64.msg_namelen;
		user_msg.msg_name = (user_addr_t)msg64.msg_name;
	} else {
		user_msg.msg_flags = msg32.msg_flags;
		user_msg.msg_controllen = msg32.msg_controllen;
		user_msg.msg_control = msg32.msg_control;
		user_msg.msg_iovlen = msg32.msg_iovlen;
		user_msg.msg_iov = msg32.msg_iov;
		user_msg.msg_namelen = msg32.msg_namelen;
		user_msg.msg_name = msg32.msg_name;
	}

	if (user_msg.msg_iovlen <= 0 || user_msg.msg_iovlen > UIO_MAXIOV) {
		KERNEL_DEBUG(DBG_FNC_SENDMSG | DBG_FUNC_END, EMSGSIZE,
		    0, 0, 0, 0);
		return EMSGSIZE;
	}

	                                                                     
	auio = uio_create(user_msg.msg_iovlen, 0,
	    (IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32),
	    UIO_WRITE);
	if (auio == NULL) {
		error = ENOBUFS;
		goto done;
	}

	if (user_msg.msg_iovlen) {
		  
                                           
                                            
     
		iovp = uio_iovsaddr(auio);
		if (iovp == NULL) {
			error = ENOBUFS;
			goto done;
		}
		error = copyin_user_iovec_array(user_msg.msg_iov,
		    IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32,
		    user_msg.msg_iovlen, iovp);
		if (error) {
			goto done;
		}
		user_msg.msg_iov = CAST_USER_ADDR_T(iovp);

		                           
		error = uio_calculateresid(auio);
		if (error) {
			goto done;
		}
	} else {
		user_msg.msg_iov = 0;
	}

	                                   
	user_msg.msg_flags = 0;

	error = file_socket(uap->s, &so);
	if (error) {
		goto done;
	}
	if (so == NULL) {
		error = EBADF;
	} else {
		error = sendit(p, so, &user_msg, auio, uap->flags, retval);
	}
	file_drop(uap->s);
done:
	if (auio != NULL) {
		uio_free(auio);
	}
	KERNEL_DEBUG(DBG_FNC_SENDMSG | DBG_FUNC_END, error, 0, 0, 0, 0);

	return error;
}

int
sendmsg_x(struct proc *p, struct sendmsg_x_args *uap, user_ssize_t *retval)
{
	int error = 0;
	struct user_msghdr_x *user_msg_x = NULL;
	struct uio **uiop = NULL;
	struct socket *so;
	u_int i;
	struct sockaddr *to = NULL;
	user_ssize_t len_before = 0, len_after;
	int need_drop = 0;
	size_t size_of_msghdr;
	void *umsgp = NULL;
	u_int uiocnt;
	int has_addr_or_ctl = 0;

	KERNEL_DEBUG(DBG_FNC_SENDMSG_X | DBG_FUNC_START, 0, 0, 0, 0, 0);

	size_of_msghdr = IS_64BIT_PROCESS(p) ?
	    sizeof(struct user64_msghdr_x) : sizeof(struct user32_msghdr_x);

	if (uap->flags & MSG_SKIPCFIL) {
		error = EPERM;
		goto out;
	}

	error = file_socket(uap->s, &so);
	if (error) {
		goto out;
	}
	need_drop = 1;
	if (so == NULL) {
		error = EBADF;
		goto out;
	}

	  
                               
    
	if (uap->cnt == 0 || uap->cnt > UIO_MAXIOV) {
		error = EINVAL;
		goto out;
	}
	  
                                 
    
	if (uap->cnt > somaxsendmsgx) {
		uap->cnt = somaxsendmsgx;
	}

	user_msg_x = kheap_alloc(KHEAP_TEMP,
	    uap->cnt * sizeof(struct user_msghdr_x), Z_WAITOK | Z_ZERO);
	if (user_msg_x == NULL) {
		DBG_PRINTF("%s kheap_alloc user_msg_x failed\n", __func__);
		error = ENOMEM;
		goto out;
	}
	uiop = kheap_alloc(KHEAP_TEMP,
	    uap->cnt * sizeof(struct uio *), Z_WAITOK | Z_ZERO);
	if (uiop == NULL) {
		DBG_PRINTF("%s kheap_alloc uiop failed\n", __func__);
		error = ENOMEM;
		goto out;
	}

	umsgp = kheap_alloc(KHEAP_TEMP,
	    uap->cnt * size_of_msghdr, Z_WAITOK | Z_ZERO);
	if (umsgp == NULL) {
		printf("%s kheap_alloc user_msg_x failed\n", __func__);
		error = ENOMEM;
		goto out;
	}
	error = copyin(uap->msgp, umsgp, uap->cnt * size_of_msghdr);
	if (error) {
		DBG_PRINTF("%s copyin() failed\n", __func__);
		goto out;
	}
	error = internalize_user_msghdr_array(umsgp,
	    IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32,
	    UIO_WRITE, uap->cnt, user_msg_x, uiop);
	if (error) {
		DBG_PRINTF("%s copyin_user_msghdr_array() failed\n", __func__);
		goto out;
	}
	  
                                                
                                                
    
	if (uio_array_is_valid(uiop, uap->cnt) == false) {
		error = EINVAL;
		goto out;
	}

	  
                                    
    
	for (i = 0; i < uap->cnt; i++) {
		struct user_msghdr_x *mp = user_msg_x + i;

		  
                             
     
		if (mp->msg_flags != 0) {
			error = EINVAL;
			goto out;
		}
		  
                                                   
     
		if (mp->msg_name != USER_ADDR_NULL || mp->msg_namelen != 0) {
			has_addr_or_ctl = 1;
		}

		if (mp->msg_control != USER_ADDR_NULL ||
		    mp->msg_controllen != 0) {
			has_addr_or_ctl = 1;
		}

#if CONFIG_MACF_SOCKET_SUBSET
		  
                                                        
                                                       
                                                
    
                                                           
                                                                 
                                                                
                                                    
     
		if (to != NULL &&
		    !(so->so_state & SS_DEFUNCT) &&
		    (error = mac_socket_check_send(kauth_cred_get(), so, to))
		    != 0) {
			goto out;
		}
#endif                        
	}

	len_before = uio_array_resid(uiop, uap->cnt);

	  
                                                                  
                   
    
	if (so->so_proto->pr_usrreqs->pru_sosend_list !=
	    pru_sosend_list_notsupp &&
	    has_addr_or_ctl == 0 && somaxsendmsgx == 0) {
		error = so->so_proto->pr_usrreqs->pru_sosend_list(so, uiop,
		    uap->cnt, uap->flags);
	} else {
		for (i = 0; i < uap->cnt; i++) {
			struct user_msghdr_x *mp = user_msg_x + i;
			struct user_msghdr user_msg;
			uio_t auio = uiop[i];
			int32_t tmpval;

			user_msg.msg_flags = mp->msg_flags;
			user_msg.msg_controllen = mp->msg_controllen;
			user_msg.msg_control = mp->msg_control;
			user_msg.msg_iovlen = mp->msg_iovlen;
			user_msg.msg_iov = mp->msg_iov;
			user_msg.msg_namelen = mp->msg_namelen;
			user_msg.msg_name = mp->msg_name;

			error = sendit(p, so, &user_msg, auio, uap->flags,
			    &tmpval);
			if (error != 0) {
				break;
			}
		}
	}
	len_after = uio_array_resid(uiop, uap->cnt);

	VERIFY(len_after <= len_before);

	if (error != 0) {
		if (len_after != len_before && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK ||
		    error == ENOBUFS)) {
			error = 0;
		}
		                                                        
		if (error == EPIPE && !(so->so_flags & SOF_NOSIGPIPE) &&
		    !(uap->flags & MSG_NOSIGNAL)) {
			psignal(p, SIGPIPE);
		}
	}
	if (error == 0) {
		uiocnt = externalize_user_msghdr_array(umsgp,
		    IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32,
		    UIO_WRITE, uap->cnt, user_msg_x, uiop);

		*retval = (int)(uiocnt);
	}
out:
	if (need_drop) {
		file_drop(uap->s);
	}
	kheap_free(KHEAP_TEMP, umsgp, uap->cnt * size_of_msghdr);
	if (uiop != NULL) {
		free_uio_array(uiop, uap->cnt);
		kheap_free(KHEAP_TEMP, uiop,
		    uap->cnt * sizeof(struct uio *));
	}
	kheap_free(KHEAP_TEMP, user_msg_x,
	    uap->cnt * sizeof(struct user_msghdr_x));

	KERNEL_DEBUG(DBG_FNC_SENDMSG_X | DBG_FUNC_END, error, 0, 0, 0, 0);

	return error;
}


static int
copyout_sa(struct sockaddr *fromsa, user_addr_t name, socklen_t *namelen)
{
	int error = 0;
	socklen_t sa_len = 0;
	ssize_t len;

	len = *namelen;
	if (len <= 0 || fromsa == 0) {
		len = 0;
	} else {
#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif
		sa_len = fromsa->sa_len;
		len = MIN((unsigned int)len, sa_len);
		error = copyout(fromsa, name, (unsigned)len);
		if (error) {
			goto out;
		}
	}
	*namelen = sa_len;
out:
	return 0;
}

static int
copyout_control(struct proc *p, struct mbuf *m, user_addr_t control,
    socklen_t *controllen, int *flags, struct socket *so)
{
	int error = 0;
	socklen_t len;
	user_addr_t ctlbuf;
	struct inpcb *inp = so ? sotoinpcb(so) : NULL;

	len = *controllen;
	*controllen = 0;
	ctlbuf = control;

	while (m && len > 0) {
		socklen_t tocopy;
		struct cmsghdr *cp = mtod(m, struct cmsghdr *);
		socklen_t cp_size = CMSG_ALIGN(cp->cmsg_len);
		socklen_t buflen = m->m_len;

		while (buflen > 0 && len > 0) {
			  
                                                      
                                                      
      
			if (cp->cmsg_level == SOL_SOCKET && cp->cmsg_type == SCM_TIMESTAMP) {
				unsigned char tmp_buffer[CMSG_SPACE(sizeof(struct user64_timeval))] = {};
				struct cmsghdr *tmp_cp = (struct cmsghdr *)(void *)tmp_buffer;
				socklen_t tmp_space;
				struct timeval *tv = (struct timeval *)(void *)CMSG_DATA(cp);

				tmp_cp->cmsg_level = SOL_SOCKET;
				tmp_cp->cmsg_type = SCM_TIMESTAMP;

				if (proc_is64bit(p)) {
					struct user64_timeval *tv64 = (struct user64_timeval *)(void *)CMSG_DATA(tmp_cp);

					os_unaligned_deref(&tv64->tv_sec) = tv->tv_sec;
					os_unaligned_deref(&tv64->tv_usec) = tv->tv_usec;

					tmp_cp->cmsg_len = CMSG_LEN(sizeof(struct user64_timeval));
					tmp_space = CMSG_SPACE(sizeof(struct user64_timeval));
				} else {
					struct user32_timeval *tv32 = (struct user32_timeval *)(void *)CMSG_DATA(tmp_cp);

					tv32->tv_sec = (user32_time_t)tv->tv_sec;
					tv32->tv_usec = tv->tv_usec;

					tmp_cp->cmsg_len = CMSG_LEN(sizeof(struct user32_timeval));
					tmp_space = CMSG_SPACE(sizeof(struct user32_timeval));
				}
				if (len >= tmp_space) {
					tocopy = tmp_space;
				} else {
					*flags |= MSG_CTRUNC;
					tocopy = len;
				}
				error = copyout(tmp_buffer, ctlbuf, tocopy);
				if (error) {
					goto out;
				}
			} else {
#if CONTENT_FILTER
				                                                                                           
				if ((so != NULL) && (so->so_cfil_db != NULL) &&
				    ((cp->cmsg_level == IPPROTO_IP && cp->cmsg_type == IP_RECVDSTADDR && inp &&
				    !(inp->inp_flags & INP_RECVDSTADDR)) ||
				    (cp->cmsg_level == IPPROTO_IPV6 && (cp->cmsg_type == IPV6_PKTINFO || cp->cmsg_type == IPV6_2292PKTINFO) && inp &&
				    !(inp->inp_flags & IN6P_PKTINFO)))) {
					tocopy = 0;
				} else
#endif
				{
					if (cp_size > buflen) {
						panic("cp_size > buflen, something"
						    "wrong with alignment!");
					}
					if (len >= cp_size) {
						tocopy = cp_size;
					} else {
						*flags |= MSG_CTRUNC;
						tocopy = len;
					}
					error = copyout((caddr_t) cp, ctlbuf, tocopy);
					if (error) {
						goto out;
					}
				}
			}

			ctlbuf += tocopy;
			len -= tocopy;

			buflen -= cp_size;
			cp = (struct cmsghdr *)(void *)
			    ((unsigned char *) cp + cp_size);
			cp_size = CMSG_ALIGN(cp->cmsg_len);
		}

		m = m->m_next;
	}
	*controllen = (socklen_t)(ctlbuf - control);
out:
	return error;
}

  
                       
            
          
         
                                             
                 
                  
                          
                           
                              
                         
                        
                        
                         
                           
                      
  
                                                                     
                                                            
                     
   
static int
recvit(struct proc *p, int s, struct user_msghdr *mp, uio_t uiop,
    user_addr_t namelenp, int32_t *retval)
{
	ssize_t len;
	int error;
	struct mbuf *control = 0;
	struct socket *so;
	struct sockaddr *fromsa = 0;
	struct fileproc *fp;

	KERNEL_DEBUG(DBG_FNC_RECVIT | DBG_FUNC_START, 0, 0, 0, 0, 0);
	if ((error = fp_get_ftype(p, s, DTYPE_SOCKET, ENOTSOCK, &fp))) {
		KERNEL_DEBUG(DBG_FNC_RECVIT | DBG_FUNC_END, error, 0, 0, 0, 0);
		return error;
	}
	so = fp->f_data;

#if CONFIG_MACF_SOCKET_SUBSET
	  
                                                       
                                                      
                                               
    
	if (!(so->so_state & SS_DEFUNCT) &&
	    !(so->so_state & SS_ISCONNECTED) &&
	    !(so->so_proto->pr_flags & PR_CONNREQUIRED) &&
	    (error = mac_socket_check_receive(kauth_cred_get(), so)) != 0) {
		goto out1;
	}
#endif                        
	if (uio_resid(uiop) < 0 || uio_resid(uiop) > INT_MAX) {
		KERNEL_DEBUG(DBG_FNC_RECVIT | DBG_FUNC_END, EINVAL, 0, 0, 0, 0);
		error = EINVAL;
		goto out1;
	}

	len = uio_resid(uiop);
	error = so->so_proto->pr_usrreqs->pru_soreceive(so, &fromsa, uiop,
	    (struct mbuf **)0, mp->msg_control ? &control : (struct mbuf **)0,
	    &mp->msg_flags);
	if (fromsa) {
		AUDIT_ARG(sockaddr, vfs_context_cwd(vfs_context_current()),
		    fromsa);
	}
	if (error) {
		if (uio_resid(uiop) != len && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK)) {
			error = 0;
		}
	}
	if (error) {
		goto out;
	}

	*retval = (int32_t)(len - uio_resid(uiop));

	if (mp->msg_name) {
		error = copyout_sa(fromsa, mp->msg_name, &mp->msg_namelen);
		if (error) {
			goto out;
		}
		                                                   
		if (namelenp &&
		    (error = copyout((caddr_t)&mp->msg_namelen, namelenp,
		    sizeof(int)))) {
			goto out;
		}
	}

	if (mp->msg_control) {
		error = copyout_control(p, control, mp->msg_control,
		    &mp->msg_controllen, &mp->msg_flags, so);
	}
out:
	FREE(fromsa, M_SONAME);
	if (control) {
		m_freem(control);
	}
	KERNEL_DEBUG(DBG_FNC_RECVIT | DBG_FUNC_END, error, 0, 0, 0, 0);
out1:
	fp_drop(p, s, fp, 0);
	return error;
}

  
                       
          
                
             
                                             
  
                                                                    
                                                           
                                                            
               
  
                                                               
                                             
   
int
recvfrom(struct proc *p, struct recvfrom_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return recvfrom_nocancel(p, (struct recvfrom_nocancel_args *)uap,
	           retval);
}

int
recvfrom_nocancel(struct proc *p, struct recvfrom_nocancel_args *uap,
    int32_t *retval)
{
	struct user_msghdr msg;
	int error;
	uio_t auio = NULL;

	KERNEL_DEBUG(DBG_FNC_RECVFROM | DBG_FUNC_START, 0, 0, 0, 0, 0);
	AUDIT_ARG(fd, uap->s);

	if (uap->fromlenaddr) {
		error = copyin(uap->fromlenaddr,
		    (caddr_t)&msg.msg_namelen, sizeof(msg.msg_namelen));
		if (error) {
			return error;
		}
	} else {
		msg.msg_namelen = 0;
	}
	msg.msg_name = uap->from;
	auio = uio_create(1, 0,
	    (IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32),
	    UIO_READ);
	if (auio == NULL) {
		return ENOMEM;
	}

	uio_addiov(auio, uap->buf, uap->len);
	                                                              
	msg.msg_iov = 0;
	msg.msg_iovlen = 0;
	msg.msg_control = 0;
	msg.msg_controllen = 0;
	msg.msg_flags = uap->flags;
	error = recvit(p, uap->s, &msg, auio, uap->fromlenaddr, retval);
	if (auio != NULL) {
		uio_free(auio);
	}

	KERNEL_DEBUG(DBG_FNC_RECVFROM | DBG_FUNC_END, error, 0, 0, 0, 0);

	return error;
}

  
                       
            
          
                
                 
             
  
                                                                     
                                             
   
int
recvmsg(struct proc *p, struct recvmsg_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return recvmsg_nocancel(p, (struct recvmsg_nocancel_args *)uap,
	           retval);
}

int
recvmsg_nocancel(struct proc *p, struct recvmsg_nocancel_args *uap,
    int32_t *retval)
{
	struct user32_msghdr msg32;
	struct user64_msghdr msg64;
	struct user_msghdr user_msg;
	caddr_t msghdrp;
	int     size_of_msghdr;
	user_addr_t uiov;
	int error;
	uio_t auio = NULL;
	struct user_iovec *iovp;

	KERNEL_DEBUG(DBG_FNC_RECVMSG | DBG_FUNC_START, 0, 0, 0, 0, 0);
	AUDIT_ARG(fd, uap->s);
	if (IS_64BIT_PROCESS(p)) {
		msghdrp = (caddr_t)&msg64;
		size_of_msghdr = sizeof(msg64);
	} else {
		msghdrp = (caddr_t)&msg32;
		size_of_msghdr = sizeof(msg32);
	}
	error = copyin(uap->msg, msghdrp, size_of_msghdr);
	if (error) {
		KERNEL_DEBUG(DBG_FNC_RECVMSG | DBG_FUNC_END, error, 0, 0, 0, 0);
		return error;
	}

	                                                     
	if (IS_64BIT_PROCESS(p)) {
		user_msg.msg_flags = msg64.msg_flags;
		user_msg.msg_controllen = msg64.msg_controllen;
		user_msg.msg_control = (user_addr_t)msg64.msg_control;
		user_msg.msg_iovlen = msg64.msg_iovlen;
		user_msg.msg_iov = (user_addr_t)msg64.msg_iov;
		user_msg.msg_namelen = msg64.msg_namelen;
		user_msg.msg_name = (user_addr_t)msg64.msg_name;
	} else {
		user_msg.msg_flags = msg32.msg_flags;
		user_msg.msg_controllen = msg32.msg_controllen;
		user_msg.msg_control = msg32.msg_control;
		user_msg.msg_iovlen = msg32.msg_iovlen;
		user_msg.msg_iov = msg32.msg_iov;
		user_msg.msg_namelen = msg32.msg_namelen;
		user_msg.msg_name = msg32.msg_name;
	}

	if (user_msg.msg_iovlen <= 0 || user_msg.msg_iovlen > UIO_MAXIOV) {
		KERNEL_DEBUG(DBG_FNC_RECVMSG | DBG_FUNC_END, EMSGSIZE,
		    0, 0, 0, 0);
		return EMSGSIZE;
	}

	user_msg.msg_flags = uap->flags;

	                                                                     
	auio = uio_create(user_msg.msg_iovlen, 0,
	    (IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32),
	    UIO_READ);
	if (auio == NULL) {
		error = ENOMEM;
		goto done;
	}

	  
                                                                       
               
    
	iovp = uio_iovsaddr(auio);
	if (iovp == NULL) {
		error = ENOMEM;
		goto done;
	}
	uiov = user_msg.msg_iov;
	user_msg.msg_iov = CAST_USER_ADDR_T(iovp);
	error = copyin_user_iovec_array(uiov,
	    IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32,
	    user_msg.msg_iovlen, iovp);
	if (error) {
		goto done;
	}

	                           
	error = uio_calculateresid(auio);
	if (error) {
		goto done;
	}

	error = recvit(p, uap->s, &user_msg, auio, 0, retval);
	if (!error) {
		user_msg.msg_iov = uiov;
		if (IS_64BIT_PROCESS(p)) {
			msg64.msg_flags = user_msg.msg_flags;
			msg64.msg_controllen = user_msg.msg_controllen;
			msg64.msg_control = user_msg.msg_control;
			msg64.msg_iovlen = user_msg.msg_iovlen;
			msg64.msg_iov = user_msg.msg_iov;
			msg64.msg_namelen = user_msg.msg_namelen;
			msg64.msg_name = user_msg.msg_name;
		} else {
			msg32.msg_flags = user_msg.msg_flags;
			msg32.msg_controllen = user_msg.msg_controllen;
			msg32.msg_control = (user32_addr_t)user_msg.msg_control;
			msg32.msg_iovlen = user_msg.msg_iovlen;
			msg32.msg_iov = (user32_addr_t)user_msg.msg_iov;
			msg32.msg_namelen = user_msg.msg_namelen;
			msg32.msg_name = (user32_addr_t)user_msg.msg_name;
		}
		error = copyout(msghdrp, uap->msg, size_of_msghdr);
	}
done:
	if (auio != NULL) {
		uio_free(auio);
	}
	KERNEL_DEBUG(DBG_FNC_RECVMSG | DBG_FUNC_END, error, 0, 0, 0, 0);
	return error;
}

int
recvmsg_x(struct proc *p, struct recvmsg_x_args *uap, user_ssize_t *retval)
{
	int error = EOPNOTSUPP;
	struct user_msghdr_x *user_msg_x = NULL;
	struct recv_msg_elem *recv_msg_array = NULL;
	struct socket *so;
	user_ssize_t len_before = 0, len_after;
	int need_drop = 0;
	size_t size_of_msghdr;
	void *umsgp = NULL;
	u_int i;
	u_int uiocnt;

	KERNEL_DEBUG(DBG_FNC_RECVMSG_X | DBG_FUNC_START, 0, 0, 0, 0, 0);

	size_of_msghdr = IS_64BIT_PROCESS(p) ?
	    sizeof(struct user64_msghdr_x) : sizeof(struct user32_msghdr_x);

	error = file_socket(uap->s, &so);
	if (error) {
		goto out;
	}
	need_drop = 1;
	if (so == NULL) {
		error = EBADF;
		goto out;
	}
	  
                                          
    
	if (uap->flags & ~(MSG_PEEK | MSG_WAITALL | MSG_DONTWAIT | MSG_NEEDSA |  MSG_NBIO)) {
		return EOPNOTSUPP;
	}
	  
                               
    
	if (uap->cnt == 0 || uap->cnt > UIO_MAXIOV) {
		error = EINVAL;
		goto out;
	}
	if (uap->cnt > somaxrecvmsgx) {
		uap->cnt = somaxrecvmsgx;
	}

	user_msg_x = kheap_alloc(KHEAP_TEMP,
	    uap->cnt * sizeof(struct user_msghdr_x), Z_WAITOK | Z_ZERO);
	if (user_msg_x == NULL) {
		DBG_PRINTF("%s kheap_alloc user_msg_x failed\n", __func__);
		error = ENOMEM;
		goto out;
	}
	recv_msg_array = alloc_recv_msg_array(uap->cnt);
	if (recv_msg_array == NULL) {
		DBG_PRINTF("%s alloc_recv_msg_array() failed\n", __func__);
		error = ENOMEM;
		goto out;
	}

	umsgp = kheap_alloc(KHEAP_TEMP,
	    uap->cnt * size_of_msghdr, Z_WAITOK | Z_ZERO);
	if (umsgp == NULL) {
		DBG_PRINTF("%s kheap_alloc umsgp failed\n", __func__);
		error = ENOMEM;
		goto out;
	}
	error = copyin(uap->msgp, umsgp, uap->cnt * size_of_msghdr);
	if (error) {
		DBG_PRINTF("%s copyin() failed\n", __func__);
		goto out;
	}
	error = internalize_recv_msghdr_array(umsgp,
	    IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32,
	    UIO_READ, uap->cnt, user_msg_x, recv_msg_array);
	if (error) {
		DBG_PRINTF("%s copyin_user_msghdr_array() failed\n", __func__);
		goto out;
	}
	  
                                                
                                                
    
	if (recv_msg_array_is_valid(recv_msg_array, uap->cnt) == 0) {
		error = EINVAL;
		goto out;
	}
	  
                                    
    
	for (i = 0; i < uap->cnt; i++) {
		struct user_msghdr_x *mp = user_msg_x + i;

		if (mp->msg_flags != 0) {
			error = EINVAL;
			goto out;
		}
	}
#if CONFIG_MACF_SOCKET_SUBSET
	  
                                                       
                                                      
                                               
    
	if (!(so->so_state & SS_DEFUNCT) &&
	    !(so->so_state & SS_ISCONNECTED) &&
	    !(so->so_proto->pr_flags & PR_CONNREQUIRED) &&
	    (error = mac_socket_check_receive(kauth_cred_get(), so)) != 0) {
		goto out;
	}
#endif                        

	len_before = recv_msg_array_resid(recv_msg_array, uap->cnt);

	if (so->so_proto->pr_usrreqs->pru_soreceive_list !=
	    pru_soreceive_list_notsupp &&
	    somaxrecvmsgx == 0) {
		error = so->so_proto->pr_usrreqs->pru_soreceive_list(so,
		    recv_msg_array, uap->cnt, &uap->flags);
	} else {
		int flags = uap->flags;

		for (i = 0; i < uap->cnt; i++) {
			struct recv_msg_elem *recv_msg_elem;
			uio_t auio;
			struct sockaddr **psa;
			struct mbuf **controlp;

			recv_msg_elem = recv_msg_array + i;
			auio = recv_msg_elem->uio;

			  
                                                
      
			if (i > 0) {
				flags |= MSG_DONTWAIT;
			}

			psa = (recv_msg_elem->which & SOCK_MSG_SA) ?
			    &recv_msg_elem->psa : NULL;
			controlp = (recv_msg_elem->which & SOCK_MSG_CONTROL) ?
			    &recv_msg_elem->controlp : NULL;

			error = so->so_proto->pr_usrreqs->pru_soreceive(so, psa,
			    auio, (struct mbuf **)NULL, controlp, &flags);
			if (error) {
				break;
			}
			  
                       
      
			recv_msg_elem->which |= SOCK_MSG_DATA;
			  
                                            
      
			flags &= ~MSG_DONTWAIT;
			recv_msg_elem->flags = flags;
			  
                          
      
			if (recv_msg_elem->flags & (MSG_RCVMORE | MSG_TRUNC)) {
				break;
			}
		}
	}

	len_after = recv_msg_array_resid(recv_msg_array, uap->cnt);

	if (error) {
		if (len_after != len_before && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK)) {
			error = 0;
		} else {
			goto out;
		}
	}

	uiocnt = externalize_recv_msghdr_array(p, so, umsgp,
	    uap->cnt, user_msg_x, recv_msg_array, &error);
	if (error != 0) {
		goto out;
	}

	error = copyout(umsgp, uap->msgp, uap->cnt * size_of_msghdr);
	if (error) {
		DBG_PRINTF("%s copyout() failed\n", __func__);
		goto out;
	}
	*retval = (int)(uiocnt);

out:
	if (need_drop) {
		file_drop(uap->s);
	}
	kheap_free(KHEAP_TEMP, umsgp, uap->cnt * size_of_msghdr);
	free_recv_msg_array(recv_msg_array, uap->cnt);
	kheap_free(KHEAP_TEMP, user_msg_x,
	    uap->cnt * sizeof(struct user_msghdr_x));

	KERNEL_DEBUG(DBG_FNC_RECVMSG_X | DBG_FUNC_END, error, 0, 0, 0, 0);

	return error;
}

  
                       
         
                       
                    
                    
                      
                                
                          
                           
                               
                              
                           
                         
                         
                           
                          
                                                               
                                             
   
              
int
shutdown(__unused struct proc *p, struct shutdown_args *uap,
    __unused int32_t *retval)
{
	struct socket *so;
	int error;

	AUDIT_ARG(fd, uap->s);
	error = file_socket(uap->s, &so);
	if (error) {
		return error;
	}
	if (so == NULL) {
		error = EBADF;
		goto out;
	}
	error =  soshutdown((struct socket *)so, uap->how);
out:
	file_drop(uap->s);
	return error;
}

  
                       
          
          
                                             
                       
                    
                  
                       
                   
                
                  
                               
               
   
              
int
setsockopt(struct proc *p, struct setsockopt_args *uap,
    __unused int32_t *retval)
{
	struct socket *so;
	struct sockopt sopt;
	int error;

	AUDIT_ARG(fd, uap->s);
	if (uap->val == 0 && uap->valsize != 0) {
		return EFAULT;
	}
	                                                

	error = file_socket(uap->s, &so);
	if (error) {
		return error;
	}

	sopt.sopt_dir = SOPT_SET;
	sopt.sopt_level = uap->level;
	sopt.sopt_name = uap->name;
	sopt.sopt_val = uap->val;
	sopt.sopt_valsize = uap->valsize;
	sopt.sopt_p = p;

	if (so == NULL) {
		error = EINVAL;
		goto out;
	}
#if CONFIG_MACF_SOCKET_SUBSET
	if ((error = mac_socket_check_setsockopt(kauth_cred_get(), so,
	    &sopt)) != 0) {
		goto out;
	}
#endif                        
	error = sosetoptlock(so, &sopt, 1);                           
out:
	file_drop(uap->s);
	return error;
}



  
                       
          
         
                                             
                
                 
                       
                    
               
   
int
getsockopt(struct proc *p, struct getsockopt_args  *uap,
    __unused int32_t *retval)
{
	int             error;
	socklen_t       valsize;
	struct sockopt  sopt;
	struct socket *so;

	error = file_socket(uap->s, &so);
	if (error) {
		return error;
	}
	if (uap->val) {
		error = copyin(uap->avalsize, (caddr_t)&valsize,
		    sizeof(valsize));
		if (error) {
			goto out;
		}
		                                                
	} else {
		valsize = 0;
	}
	sopt.sopt_dir = SOPT_GET;
	sopt.sopt_level = uap->level;
	sopt.sopt_name = uap->name;
	sopt.sopt_val = uap->val;
	sopt.sopt_valsize = (size_t)valsize;                                 
	sopt.sopt_p = p;

	if (so == NULL) {
		error = EBADF;
		goto out;
	}
#if CONFIG_MACF_SOCKET_SUBSET
	if ((error = mac_socket_check_getsockopt(kauth_cred_get(), so,
	    &sopt)) != 0) {
		goto out;
	}
#endif                        
	error = sogetoptlock((struct socket *)so, &sopt, 1);                   
	if (error == 0) {
		valsize = (socklen_t)sopt.sopt_valsize;
		error = copyout((caddr_t)&valsize, uap->avalsize,
		    sizeof(valsize));
	}
out:
	file_drop(uap->s);
	return error;
}


  
                   
  
                       
         
                       
                    
                
                 
                              
                                 
                                 
                       
   
              
int
getsockname(__unused struct proc *p, struct getsockname_args *uap,
    __unused int32_t *retval)
{
	struct socket *so;
	struct sockaddr *sa;
	socklen_t len;
	socklen_t sa_len;
	int error;

	error = file_socket(uap->fdes, &so);
	if (error) {
		return error;
	}
	error = copyin(uap->alen, (caddr_t)&len, sizeof(socklen_t));
	if (error) {
		goto out;
	}
	if (so == NULL) {
		error = EBADF;
		goto out;
	}
	sa = 0;
	socket_lock(so, 1);
	error = (*so->so_proto->pr_usrreqs->pru_sockaddr)(so, &sa);
	if (error == 0) {
		error = sflt_getsockname(so, &sa);
		if (error == EJUSTRETURN) {
			error = 0;
		}
	}
	socket_unlock(so, 1);
	if (error) {
		goto bad;
	}
	if (sa == 0) {
		len = 0;
		goto gotnothing;
	}

	sa_len = sa->sa_len;
	len = MIN(len, sa_len);
	error = copyout((caddr_t)sa, uap->asa, len);
	if (error) {
		goto bad;
	}
	                                                   
	len = sa_len;
gotnothing:
	error = copyout((caddr_t)&len, uap->alen, sizeof(socklen_t));
bad:
	FREE(sa, M_SONAME);
out:
	file_drop(uap->fdes);
	return error;
}

  
                                         
  
                       
         
          
            
                       
                    
                
                 
                     
                       
   
              
int
getpeername(__unused struct proc *p, struct getpeername_args *uap,
    __unused int32_t *retval)
{
	struct socket *so;
	struct sockaddr *sa;
	socklen_t len;
	socklen_t sa_len;
	int error;

	error = file_socket(uap->fdes, &so);
	if (error) {
		return error;
	}
	if (so == NULL) {
		error = EBADF;
		goto out;
	}

	socket_lock(so, 1);

	if ((so->so_state & (SS_CANTRCVMORE | SS_CANTSENDMORE)) ==
	    (SS_CANTRCVMORE | SS_CANTSENDMORE)) {
		                                                         
		socket_unlock(so, 1);
		error = EINVAL;
		goto out;
	}

	if ((so->so_state & (SS_ISCONNECTED | SS_ISCONFIRMING)) == 0) {
		socket_unlock(so, 1);
		error = ENOTCONN;
		goto out;
	}
	error = copyin(uap->alen, (caddr_t)&len, sizeof(socklen_t));
	if (error) {
		socket_unlock(so, 1);
		goto out;
	}
	sa = 0;
	error = (*so->so_proto->pr_usrreqs->pru_peeraddr)(so, &sa);
	if (error == 0) {
		error = sflt_getpeername(so, &sa);
		if (error == EJUSTRETURN) {
			error = 0;
		}
	}
	socket_unlock(so, 1);
	if (error) {
		goto bad;
	}
	if (sa == 0) {
		len = 0;
		goto gotnothing;
	}
	sa_len = sa->sa_len;
	len = MIN(len, sa_len);
	error = copyout(sa, uap->asa, len);
	if (error) {
		goto bad;
	}
	                                                   
	len = sa_len;
gotnothing:
	error = copyout((caddr_t)&len, uap->alen, sizeof(socklen_t));
bad:
	FREE(sa, M_SONAME);
out:
	file_drop(uap->fdes);
	return error;
}

int
sockargs(struct mbuf **mp, user_addr_t data, socklen_t buflen, int type)
{
	struct sockaddr *sa;
	struct mbuf *m;
	int error;
	socklen_t alloc_buflen = buflen;

	if (buflen > INT_MAX / 2) {
		return EINVAL;
	}
	if (type == MT_SONAME && buflen > SOCK_MAXADDRLEN) {
		return EINVAL;
	}

#ifdef __LP64__
	  
                                                                         
                 
    
	if (type == MT_CONTROL) {
		alloc_buflen = ((buflen - sizeof(struct cmsghdr)) * 2) +
		    sizeof(struct cmsghdr);
	}
#endif
	if (alloc_buflen > MLEN) {
		if (type == MT_SONAME && alloc_buflen <= 112) {
			alloc_buflen = MLEN;                                  
		} else if (alloc_buflen > MCLBYTES) {
			return EINVAL;
		}
	}
	m = m_get(M_WAIT, type);
	if (m == NULL) {
		return ENOBUFS;
	}
	if (alloc_buflen > MLEN) {
		MCLGET(m, M_WAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			return ENOBUFS;
		}
	}
	  
                                                                     
                                                                       
                   
    
	m->m_len = buflen;
	error = copyin(data, mtod(m, caddr_t), (u_int)buflen);
	if (error) {
		(void) m_free(m);
	} else {
		*mp = m;
		if (type == MT_SONAME) {
			sa = mtod(m, struct sockaddr *);
			VERIFY(buflen <= SOCK_MAXADDRLEN);
			sa->sa_len = (__uint8_t)buflen;
		}
	}
	return error;
}

  
                                                                  
  
                       
                                   
                             
                             
                              
   
static int
getsockaddr(struct socket *so, struct sockaddr **namp, user_addr_t uaddr,
    size_t len, boolean_t translate_unspec)
{
	struct sockaddr *sa;
	int error;

	if (len > SOCK_MAXADDRLEN) {
		return ENAMETOOLONG;
	}

	if (len < offsetof(struct sockaddr, sa_data[0])) {
		return EINVAL;
	}

	MALLOC(sa, struct sockaddr *, len, M_SONAME, M_WAITOK | M_ZERO);
	if (sa == NULL) {
		return ENOMEM;
	}
	error = copyin(uaddr, (caddr_t)sa, len);
	if (error) {
		FREE(sa, M_SONAME);
	} else {
		  
                                                            
                                                              
                                                          
               
     
		if (translate_unspec && sa->sa_family == AF_UNSPEC &&
		    SOCK_CHECK_DOM(so, PF_INET) &&
		    len == sizeof(struct sockaddr_in)) {
			sa->sa_family = AF_INET;
		}
		VERIFY(len <= SOCK_MAXADDRLEN);
		sa->sa_len = (__uint8_t)len;
		*namp = sa;
	}
	return error;
}

static int
getsockaddr_s(struct socket *so, struct sockaddr_storage *ss,
    user_addr_t uaddr, size_t len, boolean_t translate_unspec)
{
	int error;

	if (ss == NULL || uaddr == USER_ADDR_NULL ||
	    len < offsetof(struct sockaddr, sa_data[0])) {
		return EINVAL;
	}

	  
                                                       
                                   
    
	if (len > sizeof(*ss)) {
		return ENAMETOOLONG;
	}

	bzero(ss, sizeof(*ss));
	error = copyin(uaddr, (caddr_t)ss, len);
	if (error == 0) {
		  
                                                            
                                                              
                                                          
               
     
		if (translate_unspec && ss->ss_family == AF_UNSPEC &&
		    SOCK_CHECK_DOM(so, PF_INET) &&
		    len == sizeof(struct sockaddr_in)) {
			ss->ss_family = AF_INET;
		}

		ss->ss_len = (__uint8_t)len;
	}
	return error;
}

int
internalize_user_msghdr_array(const void *src, int spacetype, int direction,
    u_int count, struct user_msghdr_x *dst, struct uio **uiop)
{
	int error = 0;
	u_int i;
	u_int namecnt = 0;
	u_int ctlcnt = 0;

	for (i = 0; i < count; i++) {
		uio_t auio;
		struct user_iovec *iovp;
		struct user_msghdr_x *user_msg = dst + i;

		if (spacetype == UIO_USERSPACE64) {
			const struct user64_msghdr_x *msghdr64;

			msghdr64 = ((const struct user64_msghdr_x *)src) + i;

			user_msg->msg_name = (user_addr_t)msghdr64->msg_name;
			user_msg->msg_namelen = msghdr64->msg_namelen;
			user_msg->msg_iov = (user_addr_t)msghdr64->msg_iov;
			user_msg->msg_iovlen = msghdr64->msg_iovlen;
			user_msg->msg_control = (user_addr_t)msghdr64->msg_control;
			user_msg->msg_controllen = msghdr64->msg_controllen;
			user_msg->msg_flags = msghdr64->msg_flags;
			user_msg->msg_datalen = (size_t)msghdr64->msg_datalen;
		} else {
			const struct user32_msghdr_x *msghdr32;

			msghdr32 = ((const struct user32_msghdr_x *)src) + i;

			user_msg->msg_name = msghdr32->msg_name;
			user_msg->msg_namelen = msghdr32->msg_namelen;
			user_msg->msg_iov = msghdr32->msg_iov;
			user_msg->msg_iovlen = msghdr32->msg_iovlen;
			user_msg->msg_control = msghdr32->msg_control;
			user_msg->msg_controllen = msghdr32->msg_controllen;
			user_msg->msg_flags = msghdr32->msg_flags;
			user_msg->msg_datalen = msghdr32->msg_datalen;
		}

		if (user_msg->msg_iovlen <= 0 ||
		    user_msg->msg_iovlen > UIO_MAXIOV) {
			error = EMSGSIZE;
			goto done;
		}
		auio = uio_create(user_msg->msg_iovlen, 0, spacetype,
		    direction);
		if (auio == NULL) {
			error = ENOMEM;
			goto done;
		}
		uiop[i] = auio;

		iovp = uio_iovsaddr(auio);
		if (iovp == NULL) {
			error = ENOMEM;
			goto done;
		}
		error = copyin_user_iovec_array(user_msg->msg_iov,
		    spacetype, user_msg->msg_iovlen, iovp);
		if (error) {
			goto done;
		}
		user_msg->msg_iov = CAST_USER_ADDR_T(iovp);

		error = uio_calculateresid(auio);
		if (error) {
			goto done;
		}
		user_msg->msg_datalen = uio_resid(auio);

		if (user_msg->msg_name && user_msg->msg_namelen) {
			namecnt++;
		}
		if (user_msg->msg_control && user_msg->msg_controllen) {
			ctlcnt++;
		}
	}
done:

	return error;
}

int
internalize_recv_msghdr_array(const void *src, int spacetype, int direction,
    u_int count, struct user_msghdr_x *dst,
    struct recv_msg_elem *recv_msg_array)
{
	int error = 0;
	u_int i;

	for (i = 0; i < count; i++) {
		struct user_iovec *iovp;
		struct user_msghdr_x *user_msg = dst + i;
		struct recv_msg_elem *recv_msg_elem = recv_msg_array + i;

		if (spacetype == UIO_USERSPACE64) {
			const struct user64_msghdr_x *msghdr64;

			msghdr64 = ((const struct user64_msghdr_x *)src) + i;

			user_msg->msg_name = (user_addr_t)msghdr64->msg_name;
			user_msg->msg_namelen = msghdr64->msg_namelen;
			user_msg->msg_iov = (user_addr_t)msghdr64->msg_iov;
			user_msg->msg_iovlen = msghdr64->msg_iovlen;
			user_msg->msg_control = (user_addr_t)msghdr64->msg_control;
			user_msg->msg_controllen = msghdr64->msg_controllen;
			user_msg->msg_flags = msghdr64->msg_flags;
			user_msg->msg_datalen = (size_t)msghdr64->msg_datalen;
		} else {
			const struct user32_msghdr_x *msghdr32;

			msghdr32 = ((const struct user32_msghdr_x *)src) + i;

			user_msg->msg_name = msghdr32->msg_name;
			user_msg->msg_namelen = msghdr32->msg_namelen;
			user_msg->msg_iov = msghdr32->msg_iov;
			user_msg->msg_iovlen = msghdr32->msg_iovlen;
			user_msg->msg_control = msghdr32->msg_control;
			user_msg->msg_controllen = msghdr32->msg_controllen;
			user_msg->msg_flags = msghdr32->msg_flags;
			user_msg->msg_datalen = msghdr32->msg_datalen;
		}

		if (user_msg->msg_iovlen <= 0 ||
		    user_msg->msg_iovlen > UIO_MAXIOV) {
			error = EMSGSIZE;
			goto done;
		}
		recv_msg_elem->uio = uio_create(user_msg->msg_iovlen, 0,
		    spacetype, direction);
		if (recv_msg_elem->uio == NULL) {
			error = ENOMEM;
			goto done;
		}

		iovp = uio_iovsaddr(recv_msg_elem->uio);
		if (iovp == NULL) {
			error = ENOMEM;
			goto done;
		}
		error = copyin_user_iovec_array(user_msg->msg_iov,
		    spacetype, user_msg->msg_iovlen, iovp);
		if (error) {
			goto done;
		}
		user_msg->msg_iov = CAST_USER_ADDR_T(iovp);

		error = uio_calculateresid(recv_msg_elem->uio);
		if (error) {
			goto done;
		}
		user_msg->msg_datalen = uio_resid(recv_msg_elem->uio);

		if (user_msg->msg_name && user_msg->msg_namelen) {
			recv_msg_elem->which |= SOCK_MSG_SA;
		}
		if (user_msg->msg_control && user_msg->msg_controllen) {
			recv_msg_elem->which |= SOCK_MSG_CONTROL;
		}
	}
done:

	return error;
}

u_int
externalize_user_msghdr_array(void *dst, int spacetype, int direction,
    u_int count, const struct user_msghdr_x *src, struct uio **uiop)
{
#pragma unused(direction)
	u_int i;
	int seenlast = 0;
	u_int retcnt = 0;

	for (i = 0; i < count; i++) {
		const struct user_msghdr_x *user_msg = src + i;
		uio_t auio = uiop[i];
		user_ssize_t len = user_msg->msg_datalen - uio_resid(auio);

		if (user_msg->msg_datalen != 0 && len == 0) {
			seenlast = 1;
		}

		if (seenlast == 0) {
			retcnt++;
		}

		if (spacetype == UIO_USERSPACE64) {
			struct user64_msghdr_x *msghdr64;

			msghdr64 = ((struct user64_msghdr_x *)dst) + i;

			msghdr64->msg_flags = user_msg->msg_flags;
			msghdr64->msg_datalen = len;
		} else {
			struct user32_msghdr_x *msghdr32;

			msghdr32 = ((struct user32_msghdr_x *)dst) + i;

			msghdr32->msg_flags = user_msg->msg_flags;
			msghdr32->msg_datalen = (user32_size_t)len;
		}
	}
	return retcnt;
}

u_int
externalize_recv_msghdr_array(struct proc *p, struct socket *so, void *dst,
    u_int count, struct user_msghdr_x *src,
    struct recv_msg_elem *recv_msg_array, int *ret_error)
{
	u_int i;
	u_int retcnt = 0;
	int spacetype = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;

	*ret_error = 0;

	for (i = 0; i < count; i++) {
		struct user_msghdr_x *user_msg = src + i;
		struct recv_msg_elem *recv_msg_elem = recv_msg_array + i;
		user_ssize_t len = 0;
		int error;

		len = user_msg->msg_datalen - uio_resid(recv_msg_elem->uio);

		if ((recv_msg_elem->which & SOCK_MSG_DATA)) {
			retcnt++;


			if (recv_msg_elem->which & SOCK_MSG_SA) {
				error = copyout_sa(recv_msg_elem->psa, user_msg->msg_name,
				    &user_msg->msg_namelen);
				if (error != 0) {
					*ret_error = error;
					return 0;
				}
			}
			if (recv_msg_elem->which & SOCK_MSG_CONTROL) {
				error = copyout_control(p, recv_msg_elem->controlp,
				    user_msg->msg_control, &user_msg->msg_controllen,
				    &recv_msg_elem->flags, so);
				if (error != 0) {
					*ret_error = error;
					return 0;
				}
			}
		}

		if (spacetype == UIO_USERSPACE64) {
			struct user64_msghdr_x *msghdr64 = ((struct user64_msghdr_x *)dst) + i;

			msghdr64->msg_namelen = user_msg->msg_namelen;
			msghdr64->msg_controllen = user_msg->msg_controllen;
			msghdr64->msg_flags = recv_msg_elem->flags;
			msghdr64->msg_datalen = len;
		} else {
			struct user32_msghdr_x *msghdr32 = ((struct user32_msghdr_x *)dst) + i;

			msghdr32->msg_namelen = user_msg->msg_namelen;
			msghdr32->msg_controllen = user_msg->msg_controllen;
			msghdr32->msg_flags = recv_msg_elem->flags;
			msghdr32->msg_datalen = (user32_size_t)len;
		}
	}
	return retcnt;
}

void
free_uio_array(struct uio **uiop, u_int count)
{
	u_int i;

	for (i = 0; i < count; i++) {
		if (uiop[i] != NULL) {
			uio_free(uiop[i]);
		}
	}
}

__private_extern__ user_ssize_t
uio_array_resid(struct uio **uiop, u_int count)
{
	user_ssize_t len = 0;
	u_int i;

	for (i = 0; i < count; i++) {
		struct uio *auio = uiop[i];

		if (auio != NULL) {
			len += uio_resid(auio);
		}
	}
	return len;
}

static boolean_t
uio_array_is_valid(struct uio **uiop, u_int count)
{
	user_ssize_t len = 0;
	u_int i;

	for (i = 0; i < count; i++) {
		struct uio *auio = uiop[i];

		if (auio != NULL) {
			user_ssize_t resid = uio_resid(auio);

			  
                                                
                                   
      
			if (resid < 0 || resid > (user_ssize_t)sb_max) {
				return false;
			}

			len += resid;
			if (len < 0 || len > (user_ssize_t)sb_max) {
				return false;
			}
		}
	}
	return true;
}


struct recv_msg_elem *
alloc_recv_msg_array(u_int count)
{
	return kheap_alloc(KHEAP_TEMP,
	           count * sizeof(struct recv_msg_elem), Z_WAITOK | Z_ZERO);
}

void
free_recv_msg_array(struct recv_msg_elem *recv_msg_array, u_int count)
{
	if (recv_msg_array == NULL) {
		return;
	}
	for (uint32_t i = 0; i < count; i++) {
		struct recv_msg_elem *recv_msg_elem = recv_msg_array + i;

		if (recv_msg_elem->uio != NULL) {
			uio_free(recv_msg_elem->uio);
		}
		_FREE(recv_msg_elem->psa, M_TEMP);
		if (recv_msg_elem->controlp != NULL) {
			m_freem(recv_msg_elem->controlp);
		}
	}
	kheap_free(KHEAP_TEMP, recv_msg_array,
	    count * sizeof(struct recv_msg_elem));
}


__private_extern__ user_ssize_t
recv_msg_array_resid(struct recv_msg_elem *recv_msg_array, u_int count)
{
	user_ssize_t len = 0;
	u_int i;

	for (i = 0; i < count; i++) {
		struct recv_msg_elem *recv_msg_elem = recv_msg_array + i;

		if (recv_msg_elem->uio != NULL) {
			len += uio_resid(recv_msg_elem->uio);
		}
	}
	return len;
}

int
recv_msg_array_is_valid(struct recv_msg_elem *recv_msg_array, u_int count)
{
	user_ssize_t len = 0;
	u_int i;

	for (i = 0; i < count; i++) {
		struct recv_msg_elem *recv_msg_elem = recv_msg_array + i;

		if (recv_msg_elem->uio != NULL) {
			user_ssize_t resid = uio_resid(recv_msg_elem->uio);

			  
                                                
                                   
      
			if (resid < 0 || (u_int32_t)resid > sb_max) {
				return 0;
			}

			len += resid;
			if (len < 0 || (u_int32_t)len > sb_max) {
				return 0;
			}
		}
	}
	return 1;
}

#if SENDFILE

#define SFUIOBUFS 64

                                                                            
#define HOWMANY_16K(n)  ((((unsigned int)(n) - 1) >> M16KCLSHIFT) + 1)
#define HOWMANY_4K(n)   ((((unsigned int)(n) - 1) >> MBIGCLSHIFT) + 1)

                                                      
#define SENDFILE_MAX_BYTES      (SFUIOBUFS << PGSHIFT)

                                                     
#define SENDFILE_MAX_16K        HOWMANY_16K(SENDFILE_MAX_BYTES)
#define SENDFILE_MAX_4K         HOWMANY_4K(SENDFILE_MAX_BYTES)

static void
alloc_sendpkt(int how, size_t pktlen, unsigned int *maxchunks,
    struct mbuf **m, boolean_t jumbocl)
{
	unsigned int needed;

	if (pktlen == 0) {
		panic("%s: pktlen (%ld) must be non-zero\n", __func__, pktlen);
	}

	  
                                                                    
                                                                       
                                                                    
    
	*m = NULL;
	if (pktlen > MBIGCLBYTES && jumbocl) {
		needed = MIN(SENDFILE_MAX_16K, HOWMANY_16K(pktlen));
		*m = m_getpackets_internal(&needed, 1, how, 0, M16KCLBYTES);
	}
	if (*m == NULL) {
		needed = MIN(SENDFILE_MAX_4K, HOWMANY_4K(pktlen));
		*m = m_getpackets_internal(&needed, 1, how, 0, MBIGCLBYTES);
	}

	  
                                                                
                                                              
                                                               
                    
    
	if (*m == NULL) {
		needed = 1;
		*m = m_getpackets_internal(&needed, 1, M_WAIT, 1, MBIGCLBYTES);
	}
	if (*m == NULL) {
		panic("%s: blocking allocation returned NULL\n", __func__);
	}

	*maxchunks = needed;
}

  
               
                                                           
                                    
  
                                                                     
                                                                    
                                                                     
                                                                           
   
int
sendfile(struct proc *p, struct sendfile_args *uap, __unused int *retval)
{
	struct fileproc *fp;
	struct vnode *vp;
	struct socket *so;
	struct writev_nocancel_args nuap;
	user_ssize_t writev_retval;
	struct user_sf_hdtr user_hdtr;
	struct user32_sf_hdtr user32_hdtr;
	struct user64_sf_hdtr user64_hdtr;
	off_t off, xfsize;
	off_t nbytes = 0, sbytes = 0;
	int error = 0;
	size_t sizeof_hdtr;
	off_t file_size;
	struct vfs_context context = *vfs_context_current();

	KERNEL_DEBUG_CONSTANT((DBG_FNC_SENDFILE | DBG_FUNC_START), uap->s,
	    0, 0, 0, 0);

	AUDIT_ARG(fd, uap->fd);
	AUDIT_ARG(value32, uap->s);

	  
                                                           
                                                   
    
	if ((error = fp_getfvp(p, uap->fd, &fp, &vp))) {
		goto done;
	}
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto done1;
	}
	if (vnode_isreg(vp) == 0) {
		error = ENOTSUP;
		goto done1;
	}
	error = file_socket(uap->s, &so);
	if (error) {
		goto done1;
	}
	if (so == NULL) {
		error = EBADF;
		goto done2;
	}
	if (so->so_type != SOCK_STREAM) {
		error = EINVAL;
		goto done2;
	}
	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto done2;
	}
	if (uap->offset < 0) {
		error = EINVAL;
		goto done2;
	}
	if (uap->nbytes == USER_ADDR_NULL) {
		error = EINVAL;
		goto done2;
	}
	if (uap->flags != 0) {
		error = EINVAL;
		goto done2;
	}

	context.vc_ucred = fp->fp_glob->fg_cred;

#if CONFIG_MACF_SOCKET_SUBSET
	                                     
	error = mac_socket_check_send(context.vc_ucred, so, NULL);
	if (error) {
		goto done2;
	}
#endif

	  
                               
                                                    
    
	error = copyin(uap->nbytes, &nbytes, sizeof(off_t));
	if (error) {
		goto done2;
	}

	  
                                                           
                         
    
	if (uap->hdtr != USER_ADDR_NULL) {
		caddr_t hdtrp;

		bzero(&user_hdtr, sizeof(user_hdtr));
		if (IS_64BIT_PROCESS(p)) {
			hdtrp = (caddr_t)&user64_hdtr;
			sizeof_hdtr = sizeof(user64_hdtr);
		} else {
			hdtrp = (caddr_t)&user32_hdtr;
			sizeof_hdtr = sizeof(user32_hdtr);
		}
		error = copyin(uap->hdtr, hdtrp, sizeof_hdtr);
		if (error) {
			goto done2;
		}
		if (IS_64BIT_PROCESS(p)) {
			user_hdtr.headers = user64_hdtr.headers;
			user_hdtr.hdr_cnt = user64_hdtr.hdr_cnt;
			user_hdtr.trailers = user64_hdtr.trailers;
			user_hdtr.trl_cnt = user64_hdtr.trl_cnt;
		} else {
			user_hdtr.headers = user32_hdtr.headers;
			user_hdtr.hdr_cnt = user32_hdtr.hdr_cnt;
			user_hdtr.trailers = user32_hdtr.trailers;
			user_hdtr.trl_cnt = user32_hdtr.trl_cnt;
		}

		  
                                                  
     
		if (user_hdtr.headers != USER_ADDR_NULL) {
			bzero(&nuap, sizeof(struct writev_args));
			nuap.fd = uap->s;
			nuap.iovp = user_hdtr.headers;
			nuap.iovcnt = user_hdtr.hdr_cnt;
			error = writev_nocancel(p, &nuap, &writev_retval);
			if (error) {
				goto done2;
			}
			sbytes += writev_retval;
		}
	}

	  
                                    
                                                           
                                                  
    
	if ((error = vnode_size(vp, &file_size, vfs_context_current())) != 0) {
		goto done2;
	}

	  
                                                                      
                                                                  
                                       
    
	socket_lock(so, 1);
	error = sblock(&so->so_snd, SBL_WAIT);
	if (error) {
		socket_unlock(so, 1);
		goto done2;
	}
	for (off = uap->offset;; off += xfsize, sbytes += xfsize) {
		mbuf_t  m0 = NULL, m;
		unsigned int    nbufs = SFUIOBUFS, i;
		uio_t   auio;
		char    uio_buf[UIO_SIZEOF(SFUIOBUFS)];               
		size_t  uiolen;
		user_ssize_t    rlen;
		off_t   pgoff;
		size_t  pktlen;
		boolean_t jumbocl;

		  
                                      
                                    
                                      
                                      
     
		xfsize = sbspace(&so->so_snd);

		if (xfsize <= 0) {
			if (so->so_state & SS_CANTSENDMORE) {
				error = EPIPE;
				goto done3;
			} else if ((so->so_state & SS_NBIO)) {
				error = EAGAIN;
				goto done3;
			} else {
				xfsize = PAGE_SIZE;
			}
		}

		if (xfsize > SENDFILE_MAX_BYTES) {
			xfsize = SENDFILE_MAX_BYTES;
		} else if (xfsize > PAGE_SIZE) {
			xfsize = trunc_page(xfsize);
		}
		pgoff = off & PAGE_MASK_64;
		if (pgoff > 0 && PAGE_SIZE - pgoff < xfsize) {
			xfsize = PAGE_SIZE_64 - pgoff;
		}
		if (nbytes && xfsize > (nbytes - sbytes)) {
			xfsize = nbytes - sbytes;
		}
		if (xfsize <= 0) {
			break;
		}
		if (off + xfsize > file_size) {
			xfsize = file_size - off;
		}
		if (xfsize <= 0) {
			break;
		}

		  
                                                             
                                                           
                                         
     
		jumbocl = sosendjcl && njcl > 0 &&
		    ((so->so_flags & SOF_MULTIPAGES) || sosendjcl_ignore_capab);

		socket_unlock(so, 0);
		alloc_sendpkt(M_WAIT, xfsize, &nbufs, &m0, jumbocl);
		pktlen = mbuf_pkthdr_maxlen(m0);
		if (pktlen < (size_t)xfsize) {
			xfsize = pktlen;
		}

		auio = uio_createwithbuffer(nbufs, off, UIO_SYSSPACE,
		    UIO_READ, &uio_buf[0], sizeof(uio_buf));
		if (auio == NULL) {
			printf("sendfile failed. nbufs = %d. %s", nbufs,
			    "File a radar related to rdar://10146739.\n");
			mbuf_freem(m0);
			error = ENXIO;
			socket_lock(so, 0);
			goto done3;
		}

		for (i = 0, m = m0, uiolen = 0;
		    i < nbufs && m != NULL && uiolen < (size_t)xfsize;
		    i++, m = mbuf_next(m)) {
			size_t mlen = mbuf_maxlen(m);

			if (mlen + uiolen > (size_t)xfsize) {
				mlen = xfsize - uiolen;
			}
			mbuf_setlen(m, mlen);
			uio_addiov(auio, CAST_USER_ADDR_T(mbuf_datastart(m)),
			    mlen);
			uiolen += mlen;
		}

		if (xfsize != uio_resid(auio)) {
			printf("sendfile: xfsize: %lld != uio_resid(auio): "
			    "%lld\n", xfsize, (long long)uio_resid(auio));
		}

		KERNEL_DEBUG_CONSTANT((DBG_FNC_SENDFILE_READ | DBG_FUNC_START),
		    uap->s, (unsigned int)((xfsize >> 32) & 0x0ffffffff),
		    (unsigned int)(xfsize & 0x0ffffffff), 0, 0);
		error = fo_read(fp, auio, FOF_OFFSET, &context);
		socket_lock(so, 0);
		if (error != 0) {
			if (uio_resid(auio) != xfsize && (error == ERESTART ||
			    error == EINTR || error == EWOULDBLOCK)) {
				error = 0;
			} else {
				mbuf_freem(m0);
				goto done3;
			}
		}
		xfsize -= uio_resid(auio);
		KERNEL_DEBUG_CONSTANT((DBG_FNC_SENDFILE_READ | DBG_FUNC_END),
		    uap->s, (unsigned int)((xfsize >> 32) & 0x0ffffffff),
		    (unsigned int)(xfsize & 0x0ffffffff), 0, 0);

		if (xfsize == 0) {
			                                              
			break;
		}
		if (xfsize + off > file_size) {
			printf("sendfile: xfsize: %lld + off: %lld > file_size:"
			    "%lld\n", xfsize, off, file_size);
		}
		for (i = 0, m = m0, rlen = 0;
		    i < nbufs && m != NULL && rlen < xfsize;
		    i++, m = mbuf_next(m)) {
			size_t mlen = mbuf_maxlen(m);

			if (rlen + mlen > (size_t)xfsize) {
				mlen = xfsize - rlen;
			}
			mbuf_setlen(m, mlen);

			rlen += mlen;
		}
		mbuf_pkthdr_setlen(m0, xfsize);

retry_space:
		  
                                                               
                                                              
                                                                
                     
                                                           
                                                               
                                                              
                                                                 
                                                 
     
		if ((so->so_state & SS_CANTSENDMORE) || so->so_error) {
			if (so->so_state & SS_CANTSENDMORE) {
				error = EPIPE;
			} else {
				error = so->so_error;
				so->so_error = 0;
			}
			m_freem(m0);
			goto done3;
		}
		  
                                                               
                                                                
                                    
     
		if (sbspace(&so->so_snd) < (long)so->so_snd.sb_lowat) {
			if (so->so_state & SS_NBIO) {
				m_freem(m0);
				error = EAGAIN;
				goto done3;
			}
			KERNEL_DEBUG_CONSTANT((DBG_FNC_SENDFILE_WAIT |
			    DBG_FUNC_START), uap->s, 0, 0, 0, 0);
			error = sbwait(&so->so_snd);
			KERNEL_DEBUG_CONSTANT((DBG_FNC_SENDFILE_WAIT |
			    DBG_FUNC_END), uap->s, 0, 0, 0, 0);
			  
                                                       
                                                          
                                                         
      
			if (error) {
				m_freem(m0);
				goto done3;
			}
			goto retry_space;
		}

		struct mbuf *control = NULL;
		{
			  
                              
      

			error = sflt_data_out(so, NULL, &m0, &control, 0);
			if (error) {
				if (error == EJUSTRETURN) {
					error = 0;
					continue;
				}
				goto done3;
			}
			  
                                  
      
		}
		KERNEL_DEBUG_CONSTANT((DBG_FNC_SENDFILE_SEND | DBG_FUNC_START),
		    uap->s, 0, 0, 0, 0);
		error = (*so->so_proto->pr_usrreqs->pru_send)(so, 0, m0,
		    0, control, p);
		KERNEL_DEBUG_CONSTANT((DBG_FNC_SENDFILE_SEND | DBG_FUNC_START),
		    uap->s, 0, 0, 0, 0);
		if (error) {
			goto done3;
		}
	}
	sbunlock(&so->so_snd, FALSE);                           
	  
                                              
    
	if (uap->hdtr != USER_ADDR_NULL &&
	    user_hdtr.trailers != USER_ADDR_NULL) {
		bzero(&nuap, sizeof(struct writev_args));
		nuap.fd = uap->s;
		nuap.iovp = user_hdtr.trailers;
		nuap.iovcnt = user_hdtr.trl_cnt;
		error = writev_nocancel(p, &nuap, &writev_retval);
		if (error) {
			goto done2;
		}
		sbytes += writev_retval;
	}
done2:
	file_drop(uap->s);
done1:
	file_drop(uap->fd);
done:
	if (uap->nbytes != USER_ADDR_NULL) {
		                                                              
		copyout(&sbytes, uap->nbytes, sizeof(off_t));
	}
	KERNEL_DEBUG_CONSTANT((DBG_FNC_SENDFILE | DBG_FUNC_END), uap->s,
	    (unsigned int)((sbytes >> 32) & 0x0ffffffff),
	    (unsigned int)(sbytes & 0x0ffffffff), error, 0);
	return error;
done3:
	sbunlock(&so->so_snd, FALSE);                           
	goto done2;
}


#endif               
