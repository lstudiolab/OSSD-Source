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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)sys_generic.c	8.9 (Berkeley) 2/14/95
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/file_internal.h>
#include <sys/proc_internal.h>
#include <sys/socketvar.h>
#include <sys/uio_internal.h>
#include <sys/kernel.h>
#include <sys/guarded.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/sysproto.h>

#include <sys/mount_internal.h>
#include <sys/protosw.h>
#include <sys/ev.h>
#include <sys/user.h>
#include <sys/kdebug.h>
#include <sys/poll.h>
#include <sys/event.h>
#include <sys/eventvar.h>
#include <sys/proc.h>
#include <sys/kauth.h>

#include <machine/smp.h>
#include <mach/mach_types.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kalloc.h>
#include <kern/thread.h>
#include <kern/clock.h>
#include <kern/ledger.h>
#include <kern/task.h>
#include <kern/telemetry.h>
#include <kern/waitq.h>
#include <kern/sched_prim.h>
#include <kern/mpsc_queue.h>
#include <kern/debug.h>

#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/pipe.h>

#include <security/audit/audit.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_debug.h>
                                 
#include <kern/waitq.h>
#include <sys/vnode_internal.h>
                        
#include <kern/remote_time.h>
#include <os/log.h>
#include <sys/log_data.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

                           
#include <IOKit/IOBSD.h>
  
                                                      
                                       
   
#define KM_SELECT       KHEAP_DEFAULT

                                              
extern kern_return_t IOBSDGetPlatformUUID(__darwin_uuid_t uuid, mach_timespec_t timeoutp);

int rd_uio(struct proc *p, int fdes, uio_t uio, int is_preadv, user_ssize_t *retval);
int wr_uio(struct proc *p, int fdes, uio_t uio, int is_pwritev, user_ssize_t *retval);
int do_uiowrite(struct proc *p, struct fileproc *fp, uio_t uio, int flags, user_ssize_t *retval);

__private_extern__ int  dofileread(vfs_context_t ctx, struct fileproc *fp,
    user_addr_t bufp, user_size_t nbyte,
    off_t offset, int flags, user_ssize_t *retval);
__private_extern__ int  dofilewrite(vfs_context_t ctx, struct fileproc *fp,
    user_addr_t bufp, user_size_t nbyte,
    off_t offset, int flags, user_ssize_t *retval);
static int preparefileread(struct proc *p, struct fileproc **fp_ret, int fd, int check_for_vnode);

                                                                
struct waitq select_conflict_queue;

  
                                      
   
void select_waitq_init(void);
void
select_waitq_init(void)
{
	waitq_init(&select_conflict_queue, SYNC_POLICY_FIFO);
}

#define f_flag fp_glob->fg_flag
#define f_type fp_glob->fg_ops->fo_type
#define f_cred fp_glob->fg_cred
#define f_ops fp_glob->fg_ops
#define f_data fp_glob->fg_data

  
                    
  
                       
                        
                         
                        
                        
                 
   
int
read(struct proc *p, struct read_args *uap, user_ssize_t *retval)
{
	__pthread_testcancel(1);
	return read_nocancel(p, (struct read_nocancel_args *)uap, retval);
}

int
read_nocancel(struct proc *p, struct read_nocancel_args *uap, user_ssize_t *retval)
{
	struct fileproc *fp;
	int error;
	int fd = uap->fd;
	struct vfs_context context;

	if ((error = preparefileread(p, &fp, fd, 0))) {
		return error;
	}

	context = *(vfs_context_current());
	context.vc_ucred = fp->fp_glob->fg_cred;

	error = dofileread(&context, fp, uap->cbuf, uap->nbyte,
	    (off_t)-1, 0, retval);

	fp_drop(p, fd, fp, 0);

	return error;
}

  
                    
  
                       
                        
                         
                        
                        
                 
   
int
pread(struct proc *p, struct pread_args *uap, user_ssize_t *retval)
{
	__pthread_testcancel(1);
	return pread_nocancel(p, (struct pread_nocancel_args *)uap, retval);
}

int
pread_nocancel(struct proc *p, struct pread_nocancel_args *uap, user_ssize_t *retval)
{
	struct fileproc *fp = NULL;                                      
	int fd = uap->fd;
	int error;
	struct vfs_context context;

	if ((error = preparefileread(p, &fp, fd, 1))) {
		goto out;
	}

	context = *(vfs_context_current());
	context.vc_ucred = fp->fp_glob->fg_cred;

	error = dofileread(&context, fp, uap->buf, uap->nbyte,
	    uap->offset, FOF_OFFSET, retval);

	fp_drop(p, fd, fp, 0);

	KERNEL_DEBUG_CONSTANT((BSDDBG_CODE(DBG_BSD_SC_EXTENDED_INFO, SYS_pread) | DBG_FUNC_NONE),
	    uap->fd, uap->nbyte, (unsigned int)((uap->offset >> 32)), (unsigned int)(uap->offset), 0);

out:
	return error;
}

  
                                 
   

  
                       
         
          
         
                  
   
static int
preparefileread(struct proc *p, struct fileproc **fp_ret, int fd, int check_for_pread)
{
	vnode_t vp;
	int     error;
	struct fileproc *fp;

	AUDIT_ARG(fd, fd);

	proc_fdlock_spin(p);

	error = fp_lookup(p, fd, &fp, 1);

	if (error) {
		proc_fdunlock(p);
		return error;
	}
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out;
	}
	if (check_for_pread && (fp->f_type != DTYPE_VNODE)) {
		error = ESPIPE;
		goto out;
	}
	if (fp->f_type == DTYPE_VNODE) {
		vp = (struct vnode *)fp->fp_glob->fg_data;

		if (check_for_pread && (vnode_isfifo(vp))) {
			error = ESPIPE;
			goto out;
		}
		if (check_for_pread && (vp->v_flag & VISTTY)) {
			error = ENXIO;
			goto out;
		}
	}

	*fp_ret = fp;

	proc_fdunlock(p);
	return 0;

out:
	fp_drop(p, fd, fp, 1);
	proc_fdunlock(p);
	return error;
}


  
                       
          
              
   
__private_extern__ int
dofileread(vfs_context_t ctx, struct fileproc *fp,
    user_addr_t bufp, user_size_t nbyte, off_t offset, int flags,
    user_ssize_t *retval)
{
	uio_t auio;
	user_ssize_t bytecnt;
	int error = 0;
	char uio_buf[UIO_SIZEOF(1)];

	if (nbyte > INT_MAX) {
		return EINVAL;
	}

	if (IS_64BIT_PROCESS(vfs_context_proc(ctx))) {
		auio = uio_createwithbuffer(1, offset, UIO_USERSPACE64, UIO_READ,
		    &uio_buf[0], sizeof(uio_buf));
	} else {
		auio = uio_createwithbuffer(1, offset, UIO_USERSPACE32, UIO_READ,
		    &uio_buf[0], sizeof(uio_buf));
	}
	if (uio_addiov(auio, bufp, nbyte) != 0) {
		*retval = 0;
		return EINVAL;
	}

	bytecnt = nbyte;

	if ((error = fo_read(fp, auio, flags, ctx))) {
		if (uio_resid(auio) != bytecnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK)) {
			error = 0;
		}
	}
	bytecnt -= uio_resid(auio);

	*retval = bytecnt;

	return error;
}

  
               
  
                                              
                     
                     
                            
                             
                            
                            
                    
                 
   
static int
readv_preadv_uio(struct proc *p, int fdes,
    user_addr_t user_iovp, int iovcnt, off_t offset, int is_preadv,
    user_ssize_t *retval)
{
	uio_t auio = NULL;
	int error;
	struct user_iovec *iovp;

	                                              
	if (iovcnt <= 0 || iovcnt > UIO_MAXIOV) {
		return EINVAL;
	}

	                                                                     
	auio = uio_create(iovcnt, offset,
	    (IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32),
	    UIO_READ);

	                                                                      
               
    
	iovp = uio_iovsaddr(auio);
	if (iovp == NULL) {
		error = ENOMEM;
		goto ExitThisRoutine;
	}
	error = copyin_user_iovec_array(user_iovp,
	    IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32,
	    iovcnt, iovp);
	if (error) {
		goto ExitThisRoutine;
	}

	                                       
    
	error = uio_calculateresid(auio);
	if (error) {
		goto ExitThisRoutine;
	}
	error = rd_uio(p, fdes, auio, is_preadv, retval);

ExitThisRoutine:
	if (auio != NULL) {
		uio_free(auio);
	}
	return error;
}

  
                            
   
int
readv(struct proc *p, struct readv_args *uap, user_ssize_t *retval)
{
	__pthread_testcancel(1);
	return readv_nocancel(p, (struct readv_nocancel_args *)uap, retval);
}

int
readv_nocancel(struct proc *p, struct readv_nocancel_args *uap, user_ssize_t *retval)
{
	return readv_preadv_uio(p, uap->fd, uap->iovp, uap->iovcnt, 0, 0, retval);
}

  
                     
   
int
sys_preadv(struct proc *p, struct preadv_args *uap, user_ssize_t *retval)
{
	__pthread_testcancel(1);
	return sys_preadv_nocancel(p, (struct preadv_nocancel_args *)uap, retval);
}

int
sys_preadv_nocancel(struct proc *p, struct preadv_nocancel_args *uap, user_ssize_t *retval)
{
	return readv_preadv_uio(p, uap->fd, uap->iovp, uap->iovcnt, uap->offset, 1, retval);
}

  
                    
  
                       
         
                  
                  
   
int
write(struct proc *p, struct write_args *uap, user_ssize_t *retval)
{
	__pthread_testcancel(1);
	return write_nocancel(p, (struct write_nocancel_args *)uap, retval);
}

int
write_nocancel(struct proc *p, struct write_nocancel_args *uap, user_ssize_t *retval)
{
	struct fileproc *fp;
	int error;
	int fd = uap->fd;

	AUDIT_ARG(fd, fd);

	error = fp_lookup(p, fd, &fp, 0);
	if (error) {
		return error;
	}
	if ((fp->f_flag & FWRITE) == 0) {
		error = EBADF;
	} else if (fp_isguarded(fp, GUARD_WRITE)) {
		proc_fdlock(p);
		error = fp_guard_exception(p, fd, fp, kGUARD_EXC_WRITE);
		proc_fdunlock(p);
	} else {
		struct vfs_context context = *(vfs_context_current());
		context.vc_ucred = fp->fp_glob->fg_cred;

		error = dofilewrite(&context, fp, uap->cbuf, uap->nbyte,
		    (off_t)-1, 0, retval);
	}
	fp_drop(p, fd, fp, 0);
	return error;
}

  
                     
  
                       
         
          
         
          
                  
                  
   
int
pwrite(struct proc *p, struct pwrite_args *uap, user_ssize_t *retval)
{
	__pthread_testcancel(1);
	return pwrite_nocancel(p, (struct pwrite_nocancel_args *)uap, retval);
}

int
pwrite_nocancel(struct proc *p, struct pwrite_nocancel_args *uap, user_ssize_t *retval)
{
	struct fileproc *fp;
	int error;
	int fd = uap->fd;
	vnode_t vp  = (vnode_t)0;

	AUDIT_ARG(fd, fd);

	error = fp_get_ftype(p, fd, DTYPE_VNODE, ESPIPE, &fp);
	if (error) {
		return error;
	}

	if ((fp->f_flag & FWRITE) == 0) {
		error = EBADF;
	} else if (fp_isguarded(fp, GUARD_WRITE)) {
		proc_fdlock(p);
		error = fp_guard_exception(p, fd, fp, kGUARD_EXC_WRITE);
		proc_fdunlock(p);
	} else {
		struct vfs_context context = *vfs_context_current();
		context.vc_ucred = fp->fp_glob->fg_cred;

		vp = (vnode_t)fp->fp_glob->fg_data;
		if (vnode_isfifo(vp)) {
			error = ESPIPE;
			goto errout;
		}
		if ((vp->v_flag & VISTTY)) {
			error = ENXIO;
			goto errout;
		}
		if (uap->offset == (off_t)-1) {
			error = EINVAL;
			goto errout;
		}

		error = dofilewrite(&context, fp, uap->buf, uap->nbyte,
		    uap->offset, FOF_OFFSET, retval);
	}
errout:
	fp_drop(p, fd, fp, 0);

	KERNEL_DEBUG_CONSTANT((BSDDBG_CODE(DBG_BSD_SC_EXTENDED_INFO, SYS_pwrite) | DBG_FUNC_NONE),
	    uap->fd, uap->nbyte, (unsigned int)((uap->offset >> 32)), (unsigned int)(uap->offset), 0);

	return error;
}

  
                       
          
                   
                                                     
   
__private_extern__ int
dofilewrite(vfs_context_t ctx, struct fileproc *fp,
    user_addr_t bufp, user_size_t nbyte, off_t offset, int flags,
    user_ssize_t *retval)
{
	uio_t auio;
	int error = 0;
	user_ssize_t bytecnt;
	char uio_buf[UIO_SIZEOF(1)];

	if (nbyte > INT_MAX) {
		*retval = 0;
		return EINVAL;
	}

	if (IS_64BIT_PROCESS(vfs_context_proc(ctx))) {
		auio = uio_createwithbuffer(1, offset, UIO_USERSPACE64, UIO_WRITE,
		    &uio_buf[0], sizeof(uio_buf));
	} else {
		auio = uio_createwithbuffer(1, offset, UIO_USERSPACE32, UIO_WRITE,
		    &uio_buf[0], sizeof(uio_buf));
	}
	if (uio_addiov(auio, bufp, nbyte) != 0) {
		*retval = 0;
		return EINVAL;
	}

	bytecnt = nbyte;
	if ((error = fo_write(fp, auio, flags, ctx))) {
		if (uio_resid(auio) != bytecnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK)) {
			error = 0;
		}
		                                      
		if (error == EPIPE && fp->f_type != DTYPE_SOCKET &&
		    (fp->fp_glob->fg_lflags & FG_NOSIGPIPE) == 0) {
			                                         
			psignal(vfs_context_proc(ctx), SIGPIPE);
		}
	}
	bytecnt -= uio_resid(auio);
	if (bytecnt) {
		os_atomic_or(&fp->fp_glob->fg_flag, FWASWRITTEN, relaxed);
	}
	*retval = bytecnt;

	return error;
}

  
                       
         
          
         
                  
                         
   
static int
preparefilewrite(struct proc *p, struct fileproc **fp_ret, int fd, int check_for_pwrite)
{
	vnode_t vp;
	int error;
	struct fileproc *fp;

	AUDIT_ARG(fd, fd);

	proc_fdlock_spin(p);

	error = fp_lookup(p, fd, &fp, 1);

	if (error) {
		proc_fdunlock(p);
		return error;
	}
	if ((fp->f_flag & FWRITE) == 0) {
		error = EBADF;
		goto ExitThisRoutine;
	}
	if (fp_isguarded(fp, GUARD_WRITE)) {
		error = fp_guard_exception(p, fd, fp, kGUARD_EXC_WRITE);
		goto ExitThisRoutine;
	}
	if (check_for_pwrite) {
		if (fp->f_type != DTYPE_VNODE) {
			error = ESPIPE;
			goto ExitThisRoutine;
		}

		vp = (vnode_t)fp->fp_glob->fg_data;
		if (vnode_isfifo(vp)) {
			error = ESPIPE;
			goto ExitThisRoutine;
		}
		if ((vp->v_flag & VISTTY)) {
			error = ENXIO;
			goto ExitThisRoutine;
		}
	}

	*fp_ret = fp;

	proc_fdunlock(p);
	return 0;

ExitThisRoutine:
	fp_drop(p, fd, fp, 1);
	proc_fdunlock(p);
	return error;
}

static int
writev_prwritev_uio(struct proc *p, int fd,
    user_addr_t user_iovp, int iovcnt, off_t offset, int is_pwritev,
    user_ssize_t *retval)
{
	uio_t auio = NULL;
	int error;
	struct user_iovec *iovp;

	                                              
	if (iovcnt <= 0 || iovcnt > UIO_MAXIOV || offset < 0) {
		return EINVAL;
	}

	                                                                     
	auio = uio_create(iovcnt, offset,
	    (IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32),
	    UIO_WRITE);

	                                                                      
               
    
	iovp = uio_iovsaddr(auio);
	if (iovp == NULL) {
		error = ENOMEM;
		goto ExitThisRoutine;
	}
	error = copyin_user_iovec_array(user_iovp,
	    IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32,
	    iovcnt, iovp);
	if (error) {
		goto ExitThisRoutine;
	}

	                                       
    
	error = uio_calculateresid(auio);
	if (error) {
		goto ExitThisRoutine;
	}

	error = wr_uio(p, fd, auio, is_pwritev, retval);

ExitThisRoutine:
	if (auio != NULL) {
		uio_free(auio);
	}
	return error;
}

  
                           
   
int
writev(struct proc *p, struct writev_args *uap, user_ssize_t *retval)
{
	__pthread_testcancel(1);
	return writev_nocancel(p, (struct writev_nocancel_args *)uap, retval);
}

int
writev_nocancel(struct proc *p, struct writev_nocancel_args *uap, user_ssize_t *retval)
{
	return writev_prwritev_uio(p, uap->fd, uap->iovp, uap->iovcnt, 0, 0, retval);
}

  
                      
   
int
sys_pwritev(struct proc *p, struct pwritev_args *uap, user_ssize_t *retval)
{
	__pthread_testcancel(1);
	return sys_pwritev_nocancel(p, (struct pwritev_nocancel_args *)uap, retval);
}

int
sys_pwritev_nocancel(struct proc *p, struct pwritev_nocancel_args *uap, user_ssize_t *retval)
{
	return writev_prwritev_uio(p, uap->fd, uap->iovp, uap->iovcnt, uap->offset, 1, retval);
}

  
                       
                        
                         
                        
                      
               
   
int
wr_uio(struct proc *p, int fd, uio_t uio, int is_pwritev, user_ssize_t *retval)
{
	struct fileproc *fp;
	int error;
	int flags;

	if ((error = preparefilewrite(p, &fp, fd, is_pwritev))) {
		return error;
	}

	flags = is_pwritev ? FOF_OFFSET : 0;
	error = do_uiowrite(p, fp, uio, flags, retval);

	fp_drop(p, fd, fp, 0);

	return error;
}

int
do_uiowrite(struct proc *p, struct fileproc *fp, uio_t uio, int flags, user_ssize_t *retval)
{
	int error;
	user_ssize_t count;
	struct vfs_context context = *vfs_context_current();

	count = uio_resid(uio);

	context.vc_ucred = fp->f_cred;
	error = fo_write(fp, uio, flags, &context);
	if (error) {
		if (uio_resid(uio) != count && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK)) {
			error = 0;
		}
		                                      
		if (error == EPIPE && fp->f_type != DTYPE_SOCKET &&
		    (fp->fp_glob->fg_lflags & FG_NOSIGPIPE) == 0) {
			psignal(p, SIGPIPE);
		}
	}
	count -= uio_resid(uio);
	if (count) {
		os_atomic_or(&fp->fp_glob->fg_flag, FWASWRITTEN, relaxed);
	}
	*retval = count;

	return error;
}

  
                       
                        
                         
                        
              
   
int
rd_uio(struct proc *p, int fdes, uio_t uio, int is_preadv, user_ssize_t *retval)
{
	struct fileproc *fp;
	int error;
	user_ssize_t count;
	struct vfs_context context = *vfs_context_current();

	if ((error = preparefileread(p, &fp, fdes, is_preadv))) {
		return error;
	}

	count = uio_resid(uio);

	context.vc_ucred = fp->f_cred;

	int flags = is_preadv ? FOF_OFFSET : 0;
	error = fo_read(fp, uio, flags, &context);

	if (error) {
		if (uio_resid(uio) != count && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK)) {
			error = 0;
		}
	}
	*retval = count - uio_resid(uio);

	fp_drop(p, fdes, fp, 0);

	return error;
}

  
                    
  
                       
         
          
          
         
                
                
                                        
               
   
int
ioctl(struct proc *p, struct ioctl_args *uap, __unused int32_t *retval)
{
	struct fileproc *fp = NULL;
	int error = 0;
	u_int size = 0;
	caddr_t datap = NULL, memp = NULL;
	boolean_t is64bit = FALSE;
	int tmp = 0;
#define STK_PARAMS      128
	char stkbuf[STK_PARAMS] = {};
	int fd = uap->fd;
	u_long com = uap->com;
	struct vfs_context context = *vfs_context_current();

	AUDIT_ARG(fd, uap->fd);
	AUDIT_ARG(addr, uap->data);

	is64bit = proc_is64bit(p);
#if CONFIG_AUDIT
	if (is64bit) {
		AUDIT_ARG(value64, com);
	} else {
		AUDIT_ARG(cmd, CAST_DOWN_EXPLICIT(int, com));
	}
#endif                   

	  
                                                          
                                            
    
	size = IOCPARM_LEN(com);
	if (size > IOCPARM_MAX) {
		return ENOTTY;
	}
	if (size > sizeof(stkbuf)) {
		memp = (caddr_t)kheap_alloc(KHEAP_TEMP, size, Z_WAITOK);
		if (memp == 0) {
			return ENOMEM;
		}
		datap = memp;
	} else {
		datap = &stkbuf[0];
	}
	if (com & IOC_IN) {
		if (size) {
			error = copyin(uap->data, datap, size);
			if (error) {
				goto out_nofp;
			}
		} else {
			                                                                         
			if (is64bit) {
				*(user_addr_t *)datap = uap->data;
			} else {
				*(uint32_t *)datap = (uint32_t)uap->data;
			}
		}
	} else if ((com & IOC_OUT) && size) {
		  
                                       
                                       
     
		bzero(datap, size);
	} else if (com & IOC_VOID) {
		                                                          
		if (is64bit) {
			*(user_addr_t *)datap = uap->data;
		} else {
			*(uint32_t *)datap = (uint32_t)uap->data;
		}
	}

	proc_fdlock(p);
	error = fp_lookup(p, fd, &fp, 1);
	if (error) {
		proc_fdunlock(p);
		goto out_nofp;
	}

	AUDIT_ARG(file, p, fp);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	context.vc_ucred = fp->fp_glob->fg_cred;

#if CONFIG_MACF
	error = mac_file_check_ioctl(context.vc_ucred, fp->fp_glob, com);
	if (error) {
		goto out;
	}
#endif

	switch (com) {
	case FIONCLEX:
		*fdflags(p, fd) &= ~UF_EXCLOSE;
		break;

	case FIOCLEX:
		*fdflags(p, fd) |= UF_EXCLOSE;
		break;

	case FIONBIO:
		                          
		  
		                                                     
		                                             
		if ((tmp = *(int *)datap)) {
			os_atomic_or(&fp->f_flag, FNONBLOCK, relaxed);
		} else {
			os_atomic_andnot(&fp->f_flag, FNONBLOCK, relaxed);
		}
		error = fo_ioctl(fp, FIONBIO, (caddr_t)&tmp, &context);
		break;

	case FIOASYNC:
		                          
		  
		                                                     
		                                             
		if ((tmp = *(int *)datap)) {
			os_atomic_or(&fp->f_flag, FASYNC, relaxed);
		} else {
			os_atomic_andnot(&fp->f_flag, FASYNC, relaxed);
		}
		error = fo_ioctl(fp, FIOASYNC, (caddr_t)&tmp, &context);
		break;

	case FIOSETOWN:
		tmp = *(int *)datap;
		if (fp->f_type == DTYPE_SOCKET) {
			((struct socket *)fp->f_data)->so_pgid = tmp;
			break;
		}
		if (fp->f_type == DTYPE_PIPE) {
			error = fo_ioctl(fp, TIOCSPGRP, (caddr_t)&tmp, &context);
			break;
		}
		if (tmp <= 0) {
			tmp = -tmp;
		} else {
			struct proc *p1 = proc_find(tmp);
			if (p1 == 0) {
				error = ESRCH;
				break;
			}
			tmp = p1->p_pgrpid;
			proc_rele(p1);
		}
		error = fo_ioctl(fp, TIOCSPGRP, (caddr_t)&tmp, &context);
		break;

	case FIOGETOWN:
		if (fp->f_type == DTYPE_SOCKET) {
			*(int *)datap = ((struct socket *)fp->f_data)->so_pgid;
			break;
		}
		error = fo_ioctl(fp, TIOCGPGRP, datap, &context);
		*(int *)datap = -*(int *)datap;
		break;

	default:
		error = fo_ioctl(fp, com, datap, &context);
		  
                                    
                                   
     
		if (error == 0 && (com & IOC_OUT) && size) {
			error = copyout(datap, uap->data, (u_int)size);
		}
		break;
	}
out:
	fp_drop(p, fd, fp, 1);
	proc_fdunlock(p);

out_nofp:
	if (memp) {
		kheap_free(KHEAP_TEMP, memp, size);
	}
	return error;
}

int     selwait, nselcoll;
#define SEL_FIRSTPASS 1
#define SEL_SECONDPASS 2
extern int selcontinue(int error);
extern int selprocess(int error, int sel_pass);
static int selscan(struct proc *p, struct _select * sel, struct _select_data * seldata,
    int nfd, int32_t *retval, int sel_pass, struct waitq_set *wqset);
static int selcount(struct proc *p, u_int32_t *ibits, int nfd, int *count);
static int seldrop_locked(struct proc *p, u_int32_t *ibits, int nfd, int lim, int *need_wakeup);
static int seldrop(struct proc *p, u_int32_t *ibits, int nfd, int lim);
static int select_internal(struct proc *p, struct select_nocancel_args *uap, uint64_t timeout, int32_t *retval);

  
                      
  
                       
                             
                                                    
   
int
select(struct proc *p, struct select_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return select_nocancel(p, (struct select_nocancel_args *)uap, retval);
}

int
select_nocancel(struct proc *p, struct select_nocancel_args *uap, int32_t *retval)
{
	uint64_t timeout = 0;

	if (uap->tv) {
		int err;
		struct timeval atv;
		if (IS_64BIT_PROCESS(p)) {
			struct user64_timeval atv64;
			err = copyin(uap->tv, (caddr_t)&atv64, sizeof(atv64));
			                                                  
			atv.tv_sec = (__darwin_time_t)atv64.tv_sec;
			atv.tv_usec = atv64.tv_usec;
		} else {
			struct user32_timeval atv32;
			err = copyin(uap->tv, (caddr_t)&atv32, sizeof(atv32));
			atv.tv_sec = atv32.tv_sec;
			atv.tv_usec = atv32.tv_usec;
		}
		if (err) {
			return err;
		}

		if (itimerfix(&atv)) {
			err = EINVAL;
			return err;
		}

		clock_absolutetime_interval_to_deadline(tvtoabstime(&atv), &timeout);
	}

	return select_internal(p, uap, timeout, retval);
}

int
pselect(struct proc *p, struct pselect_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return pselect_nocancel(p, (struct pselect_nocancel_args *)uap, retval);
}

int
pselect_nocancel(struct proc *p, struct pselect_nocancel_args *uap, int32_t *retval)
{
	int err;
	struct uthread *ut;
	uint64_t timeout = 0;

	if (uap->ts) {
		struct timespec ts;

		if (IS_64BIT_PROCESS(p)) {
			struct user64_timespec ts64;
			err = copyin(uap->ts, (caddr_t)&ts64, sizeof(ts64));
			ts.tv_sec = (__darwin_time_t)ts64.tv_sec;
			ts.tv_nsec = (long)ts64.tv_nsec;
		} else {
			struct user32_timespec ts32;
			err = copyin(uap->ts, (caddr_t)&ts32, sizeof(ts32));
			ts.tv_sec = ts32.tv_sec;
			ts.tv_nsec = ts32.tv_nsec;
		}
		if (err) {
			return err;
		}

		if (!timespec_is_valid(&ts)) {
			return EINVAL;
		}
		clock_absolutetime_interval_to_deadline(tstoabstime(&ts), &timeout);
	}

	ut = get_bsdthread_info(current_thread());

	if (uap->mask != USER_ADDR_NULL) {
		                                                     
		sigset_t newset;
		err = copyin(uap->mask, &newset, sizeof(sigset_t));
		if (err) {
			return err;
		}
		ut->uu_oldmask = ut->uu_sigmask;
		ut->uu_flag |= UT_SAS_OLDMASK;
		ut->uu_sigmask = (newset & ~sigcantmask);
	}

	err = select_internal(p, (struct select_nocancel_args *)uap, timeout, retval);

	if (err != EINTR && ut->uu_flag & UT_SAS_OLDMASK) {
		  
                                                                            
                                                                        
                                                                    
                                                         
     
		ut->uu_sigmask = ut->uu_oldmask;
		ut->uu_oldmask = 0;
		ut->uu_flag &= ~UT_SAS_OLDMASK;
	}

	return err;
}

void
select_cleanup_uthread(struct _select *sel)
{
	kheap_free(KHEAP_DATA_BUFFERS, sel->ibits, 2 * sel->nbytes);
	sel->ibits = sel->obits = NULL;
	sel->nbytes = 0;
}

static int
select_grow_uthread_cache(struct _select *sel, uint32_t nbytes)
{
	uint32_t *buf;

	buf = kheap_alloc(KHEAP_DATA_BUFFERS, 2 * nbytes, Z_WAITOK | Z_ZERO);
	if (buf) {
		select_cleanup_uthread(sel);
		sel->ibits = buf;
		sel->obits = buf + nbytes / sizeof(uint32_t);
		sel->nbytes = nbytes;
		return true;
	}
	return false;
}

static void
select_bzero_uthread_cache(struct _select *sel)
{
	bzero(sel->ibits, sel->nbytes * 2);
}

  
                                                                             
                                                                                
                                                                             
                                                                              
                                                                  
   
static int
select_internal(struct proc *p, struct select_nocancel_args *uap, uint64_t timeout, int32_t *retval)
{
	int error = 0;
	u_int ni, nw;
	thread_t th_act;
	struct uthread  *uth;
	struct _select *sel;
	struct _select_data *seldata;
	int count = 0;
	size_t sz = 0;

	th_act = current_thread();
	uth = get_bsdthread_info(th_act);
	sel = &uth->uu_select;
	seldata = &uth->uu_save.uus_select_data;
	*retval = 0;

	seldata->args = uap;
	seldata->retval = retval;
	seldata->wqp = NULL;
	seldata->count = 0;

	if (uap->nd < 0) {
		return EINVAL;
	}

	                                                                 
	if (p->p_fd == NULL) {
		return EBADF;
	}

	if (uap->nd > p->p_fd->fd_nfiles) {
		uap->nd = p->p_fd->fd_nfiles;                                
	}
	nw = howmany(uap->nd, NFDBITS);
	ni = nw * sizeof(fd_mask);

	  
                                                                  
                                                                 
                                      
   
                                                                  
                                                                  
                                                        
    
	if (sel->nbytes < (3 * ni)) {
		if (!select_grow_uthread_cache(sel, 3 * ni)) {
			return EAGAIN;
		}
	} else {
		select_bzero_uthread_cache(sel);
	}

	  
                                            
    
#define getbits(name, x) \
	do { \
	        if (uap->name && (error = copyin(uap->name, \
	                (caddr_t)&sel->ibits[(x) * nw], ni))) \
	                goto continuation; \
	} while (0)

	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);
#undef  getbits

	seldata->abstime = timeout;

	if ((error = selcount(p, sel->ibits, uap->nd, &count))) {
		goto continuation;
	}

	  
                                                                  
                                                                  
                                                                 
                                                                 
                                                                  
                                                                 
                                                                 
                                                                 
                     
   
                                                           
                                                                
                                                                  
                                
   
                                                                 
                                                                  
                                                                    
                                                                
                                                       
    
	sz = ALIGN(sizeof(struct waitq_set)) + (count * sizeof(uint64_t));
	if (sz > uth->uu_wqstate_sz) {
		                                                  
		if (uth->uu_wqset) {
			if (waitq_set_is_valid(uth->uu_wqset)) {
				waitq_set_deinit(uth->uu_wqset);
			}
			kheap_free(KM_SELECT, uth->uu_wqset, uth->uu_wqstate_sz);
		} else if (uth->uu_wqstate_sz && !uth->uu_wqset) {
			panic("select: thread structure corrupt! "
			    "uu_wqstate_sz:%ld, wqstate_buf == NULL",
			    uth->uu_wqstate_sz);
		}
		uth->uu_wqstate_sz = sz;
		uth->uu_wqset = kheap_alloc(KM_SELECT, sz, Z_WAITOK);
		if (!uth->uu_wqset) {
			panic("can't allocate %ld bytes for wqstate buffer",
			    uth->uu_wqstate_sz);
		}
		waitq_set_init(uth->uu_wqset,
		    SYNC_POLICY_FIFO | SYNC_POLICY_PREPOST, NULL, NULL);
	}

	if (!waitq_set_is_valid(uth->uu_wqset)) {
		waitq_set_init(uth->uu_wqset,
		    SYNC_POLICY_FIFO | SYNC_POLICY_PREPOST, NULL, NULL);
	}

	                                                                
	seldata->wqp = (uint64_t *)((char *)(uth->uu_wqset) + ALIGN(sizeof(struct waitq_set)));
	bzero(seldata->wqp, sz - ALIGN(sizeof(struct waitq_set)));

	seldata->count = count;

continuation:

	if (error) {
		  
                                                         
                                                           
                                                             
                            
     
		return error;
	}

	return selprocess(0, SEL_FIRSTPASS);
}

int
selcontinue(int error)
{
	return selprocess(error, SEL_SECONDPASS);
}


  
             
  
                                                     
                                
   
int
selprocess(int error, int sel_pass)
{
	int ncoll;
	u_int ni, nw;
	thread_t th_act;
	struct uthread  *uth;
	struct proc *p;
	struct select_nocancel_args *uap;
	int *retval;
	struct _select *sel;
	struct _select_data *seldata;
	int unwind = 1;
	int prepost = 0;
	int somewakeup = 0;
	int doretry = 0;
	wait_result_t wait_result;

	p = current_proc();
	th_act = current_thread();
	uth = get_bsdthread_info(th_act);
	sel = &uth->uu_select;
	seldata = &uth->uu_save.uus_select_data;
	uap = seldata->args;
	retval = seldata->retval;

	if ((error != 0) && (sel_pass == SEL_FIRSTPASS)) {
		unwind = 0;
	}
	if (seldata->count == 0) {
		unwind = 0;
	}
retry:
	if (error != 0) {
		goto done;
	}

	ncoll = nselcoll;
	OSBitOrAtomic(P_SELECT, &p->p_flag);

	                                                   
	if (seldata->count) {
		error = selscan(p, sel, seldata, uap->nd, retval, sel_pass, uth->uu_wqset);
		if (error || *retval) {
			goto done;
		}
		if (prepost || somewakeup) {
			  
                                                  
                                                   
                                        
      
			prepost = 0;
			somewakeup = 0;
			doretry = 1;
		}
	}

	if (uap->tv) {
		uint64_t        now;

		clock_get_uptime(&now);
		if (now >= seldata->abstime) {
			goto done;
		}
	}

	if (doretry) {
		                                 
		doretry = 0;
		sel_pass = SEL_FIRSTPASS;
		goto retry;
	}

	  
                                                    
                                                         
    
	if (uap->tv && seldata->abstime == 0) {
		goto done;
	}

	                                                                
	if ((sel_pass == SEL_SECONDPASS) || ((p->p_flag & P_SELECT) == 0)) {
		sel_pass = SEL_FIRSTPASS;
		goto retry;
	}

	OSBitAndAtomic(~((uint32_t)P_SELECT), &p->p_flag);

	                                                  
	if (seldata->count && (sel_pass == SEL_SECONDPASS)) {
		panic("selprocess: 2nd pass assertwaiting");
	}

	                                              
	wait_result = waitq_assert_wait64_leeway((struct waitq *)uth->uu_wqset,
	    NO_EVENT64, THREAD_ABORTSAFE,
	    TIMEOUT_URGENCY_USER_NORMAL,
	    seldata->abstime,
	    TIMEOUT_NO_LEEWAY);
	if (wait_result != THREAD_AWAKENED) {
		                                   
		error = tsleep1(NULL, PSOCK | PCATCH,
		    "select", 0, selcontinue);
	} else {
		prepost = 1;
		error = 0;
	}

	if (error == 0) {
		sel_pass = SEL_SECONDPASS;
		if (!prepost) {
			somewakeup = 1;
		}
		goto retry;
	}
done:
	if (unwind) {
		seldrop(p, sel->ibits, uap->nd, seldata->count);
		waitq_set_deinit(uth->uu_wqset);
		  
                                                             
                                                               
                                  
     
		bzero((void *)uth->uu_wqset, uth->uu_wqstate_sz);
	}
	OSBitAndAtomic(~((uint32_t)P_SELECT), &p->p_flag);
	                                              
	if (error == ERESTART) {
		error = EINTR;
	}
	if (error == EWOULDBLOCK) {
		error = 0;
	}
	nw = howmany(uap->nd, NFDBITS);
	ni = nw * sizeof(fd_mask);

#define putbits(name, x) \
	do { \
	        if (uap->name && (error2 = \
	                copyout((caddr_t)&sel->obits[(x) * nw], uap->name, ni))) \
	                error = error2; \
	} while (0)

	if (error == 0) {
		int error2;

		putbits(in, 0);
		putbits(ou, 1);
		putbits(ex, 2);
#undef putbits
	}

	if (error != EINTR && sel_pass == SEL_SECONDPASS && uth->uu_flag & UT_SAS_OLDMASK) {
		                                             
		uth->uu_sigmask = uth->uu_oldmask;
		uth->uu_oldmask = 0;
		uth->uu_flag &= ~UT_SAS_OLDMASK;
	}

	return error;
}


   
                                                                      
                                     
  
              
                                                        
                                                    
                                                     
              
                       
   
static void
selunlinkfp(struct fileproc *fp, uint64_t wqp_id, struct waitq_set *wqset)
{
	int valid_set = waitq_set_is_valid(wqset);
	int valid_q = !!wqp_id;

	  
                                                                   
                                                                     
                     
    

	                                                                       
	if (valid_q && valid_set) {
		waitq_unlink_by_prepost_id(wqp_id, wqset);
	}

	                                                   
	if (!(fp->fp_flags & (FP_INSELECT | FP_SELCONFLICT))) {
		return;
	}

	  
                                                                       
                                                                       
                                                                      
                                                                 
                                         
    
	if (valid_set && (fp->fp_flags & FP_SELCONFLICT)) {
		waitq_unlink(&select_conflict_queue, wqset);
	}

	             
                                                           
                                                             
                                                              
                                          
    
	if (valid_set && fp->fp_wset == (void *)wqset) {
		fp->fp_flags &= ~FP_INSELECT;
		fp->fp_wset = NULL;
	}
}

   
                                                                         
                                   
  
              
                                                
                                                           
                                                           
                                                     
  
              
                       
   
static uint64_t
sellinkfp(struct fileproc *fp, void **wq_data, struct waitq_set *wqset)
{
	struct waitq *f_wq = NULL;

	if ((fp->fp_flags & FP_INSELECT) != FP_INSELECT) {
		if (wq_data) {
			panic("non-null data:%p on fp:%p not in select?!"
			    "(wqset:%p)", wq_data, fp, wqset);
		}
		return 0;
	}

	if ((fp->fp_flags & FP_SELCONFLICT) == FP_SELCONFLICT) {
		waitq_link(&select_conflict_queue, wqset, WAITQ_SHOULD_LOCK, NULL);
	}

	  
                                                                      
                                                                     
                                             
   
                                                             
                                                                
                                
    
	if (wq_data) {
		memcpy(&f_wq, wq_data, sizeof(f_wq));
		if (!waitq_is_valid(f_wq)) {
			f_wq = NULL;
		}
	}

	                                                               
	if (!fp->fp_wset) {
		fp->fp_wset = (void *)wqset;
	}

	                       
	return waitq_get_prepost_id(f_wq);
}


  
          
  
                                                
                                                 
                                                
                                                   
                                                    
                                        
                                         
  
                       
                                             
                                                 
                
   
static int
selscan(struct proc *p, struct _select *sel, struct _select_data * seldata,
    int nfd, int32_t *retval, int sel_pass, struct waitq_set *wqset)
{
	struct filedesc *fdp = p->p_fd;
	int msk, i, j, fd;
	u_int32_t bits;
	struct fileproc *fp;
	int n = 0;                                 
	int nc = 0;                                                
	static int flag[3] = { FREAD, FWRITE, 0 };
	u_int32_t *iptr, *optr;
	u_int nw;
	u_int32_t *ibits, *obits;
	uint64_t reserved_link, *rl_ptr = NULL;
	int count;
	struct vfs_context context = *vfs_context_current();

	  
                                                    
                                                  
    
	if (fdp == NULL) {
		*retval = 0;
		return EIO;
	}
	ibits = sel->ibits;
	obits = sel->obits;

	nw = howmany(nfd, NFDBITS);

	count = seldata->count;

	nc = 0;
	if (!count) {
		*retval = 0;
		return 0;
	}

	proc_fdlock(p);
	for (msk = 0; msk < 3; msk++) {
		iptr = (u_int32_t *)&ibits[msk * nw];
		optr = (u_int32_t *)&obits[msk * nw];

		for (i = 0; i < nfd; i += NFDBITS) {
			bits = iptr[i / NFDBITS];

			while ((j = ffs(bits)) && (fd = i + --j) < nfd) {
				bits &= ~(1U << j);

				fp = fp_get_noref_locked(p, fd);
				if (fp == NULL) {
					  
                                    
                                    
        
					proc_fdunlock(p);
					return EBADF;
				}
				if (sel_pass == SEL_SECONDPASS) {
					reserved_link = 0;
					rl_ptr = NULL;
					selunlinkfp(fp, seldata->wqp[nc], wqset);
				} else {
					reserved_link = waitq_link_reserve((struct waitq *)wqset);
					rl_ptr = &reserved_link;
					if (fp->fp_flags & FP_INSELECT) {
						                                             
						fp->fp_flags |= FP_SELCONFLICT;
					} else {
						fp->fp_flags |= FP_INSELECT;
					}

					waitq_set_lazy_init_link(wqset);
				}

				context.vc_ucred = fp->f_cred;

				  
                                                 
                                                     
       
				uint64_t rsvd = reserved_link;

				                                      
				if (fp->f_ops && fp->f_type
				    && fo_select(fp, flag[msk], rl_ptr, &context)) {
					optr[fd / NFDBITS] |= (1U << (fd % NFDBITS));
					n++;
				}
				if (sel_pass == SEL_FIRSTPASS) {
					waitq_link_release(rsvd);
					  
                                                           
                                                        
                                                           
                               
        
					if (reserved_link == rsvd) {
						rl_ptr = NULL;                                         
					}
					  
                                                
                                                
                                             
                    
        
					seldata->wqp[nc] = sellinkfp(fp, (void **)rl_ptr, wqset);
				}
				nc++;
			}
		}
	}
	proc_fdunlock(p);

	*retval = n;
	return 0;
}

static int poll_callback(struct kevent_qos_s *, kevent_ctx_t);

int
poll(struct proc *p, struct poll_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return poll_nocancel(p, (struct poll_nocancel_args *)uap, retval);
}


int
poll_nocancel(struct proc *p, struct poll_nocancel_args *uap, int32_t *retval)
{
	struct pollfd *fds = NULL;
	struct kqueue *kq = NULL;
	int ncoll, error = 0;
	u_int nfds = uap->nfds;
	u_int rfds = 0;
	rlim_t nofile = proc_limitgetcur(p, RLIMIT_NOFILE, TRUE);
	size_t ni = nfds * sizeof(struct pollfd);

	  
                                                            
                                                              
                                                             
                                                                  
                                     
    
	if (nfds > OPEN_MAX ||
	    (nfds > nofile && (proc_suser(p) || nfds > FD_SETSIZE))) {
		return EINVAL;
	}

	kq = kqueue_alloc(p);
	if (kq == NULL) {
		return EAGAIN;
	}

	if (nfds) {
		fds = kheap_alloc(KHEAP_TEMP, ni, Z_WAITOK);
		if (NULL == fds) {
			error = EAGAIN;
			goto out;
		}

		error = copyin(uap->fds, fds, nfds * sizeof(struct pollfd));
		if (error) {
			goto out;
		}
	}

	                                            
	ncoll = nselcoll;
	OSBitOrAtomic(P_SELECT, &p->p_flag);
	for (u_int i = 0; i < nfds; i++) {
		short events = fds[i].events;
		__assert_only int rc;

		                                           
		if (fds[i].fd < 0) {
			fds[i].revents = 0;
			continue;
		}

		                                                 
		struct kevent_qos_s kev = {
			.ident = fds[i].fd,
			.flags = EV_ADD | EV_ONESHOT | EV_POLL,
			.udata = CAST_USER_ADDR_T(&fds[i])
		};

		                         
		if (events & (POLLIN | POLLRDNORM | POLLPRI | POLLRDBAND | POLLHUP)) {
			kev.filter = EVFILT_READ;
			if (events & (POLLPRI | POLLRDBAND)) {
				kev.flags |= EV_OOBAND;
			}
			rc = kevent_register(kq, &kev, NULL);
			assert((rc & FILTER_REGISTER_WAIT) == 0);
		}

		                          
		if ((kev.flags & EV_ERROR) == 0 &&
		    (events & (POLLOUT | POLLWRNORM | POLLWRBAND))) {
			kev.filter = EVFILT_WRITE;
			rc = kevent_register(kq, &kev, NULL);
			assert((rc & FILTER_REGISTER_WAIT) == 0);
		}

		                                       
		if ((kev.flags & EV_ERROR) == 0 &&
		    (events & (POLLEXTEND | POLLATTRIB | POLLNLINK | POLLWRITE))) {
			kev.filter = EVFILT_VNODE;
			kev.fflags = 0;
			if (events & POLLEXTEND) {
				kev.fflags |= NOTE_EXTEND;
			}
			if (events & POLLATTRIB) {
				kev.fflags |= NOTE_ATTRIB;
			}
			if (events & POLLNLINK) {
				kev.fflags |= NOTE_LINK;
			}
			if (events & POLLWRITE) {
				kev.fflags |= NOTE_WRITE;
			}
			rc = kevent_register(kq, &kev, NULL);
			assert((rc & FILTER_REGISTER_WAIT) == 0);
		}

		if (kev.flags & EV_ERROR) {
			fds[i].revents = POLLNVAL;
			rfds++;
		} else {
			fds[i].revents = 0;
		}
	}

	  
                                        
                                                                      
                                                                       
                                                                     
                                                                    
                                             
    
	if (nfds && (rfds == nfds)) {
		goto done;
	}

	                                                             
	kevent_ctx_t kectx = kevent_get_context(current_thread());
	*kectx = (struct kevent_ctx_s){
		.kec_process_noutputs = rfds,
		.kec_process_flags    = KEVENT_FLAG_POLL,
		.kec_deadline         = 0,                   
	};

	  
                                                                     
                                             
    
	if (rfds) {
		kectx->kec_process_flags |= KEVENT_FLAG_IMMEDIATE;
	} else if (uap->timeout != -1) {
		clock_interval_to_deadline(uap->timeout, NSEC_PER_MSEC,
		    &kectx->kec_deadline);
	}

	error = kqueue_scan(kq, kectx->kec_process_flags, kectx, poll_callback);
	rfds = kectx->kec_process_noutputs;

done:
	OSBitAndAtomic(~((uint32_t)P_SELECT), &p->p_flag);
	                                            
	if (error == ERESTART) {
		error = EINTR;
	}
	if (error == 0) {
		error = copyout(fds, uap->fds, nfds * sizeof(struct pollfd));
		*retval = rfds;
	}

out:
	kheap_free(KHEAP_TEMP, fds, ni);

	kqueue_dealloc(kq);
	return error;
}

static int
poll_callback(struct kevent_qos_s *kevp, kevent_ctx_t kectx)
{
	struct pollfd *fds = CAST_DOWN(struct pollfd *, kevp->udata);
	short prev_revents = fds->revents;
	short mask = 0;

	                                           
	if (kevp->flags & EV_EOF) {
		fds->revents |= POLLHUP;
	}
	if (kevp->flags & EV_ERROR) {
		fds->revents |= POLLERR;
	}

	switch (kevp->filter) {
	case EVFILT_READ:
		if (fds->revents & POLLHUP) {
			mask = (POLLIN | POLLRDNORM | POLLPRI | POLLRDBAND);
		} else {
			mask = (POLLIN | POLLRDNORM);
			if (kevp->flags & EV_OOBAND) {
				mask |= (POLLPRI | POLLRDBAND);
			}
		}
		fds->revents |= (fds->events & mask);
		break;

	case EVFILT_WRITE:
		if (!(fds->revents & POLLHUP)) {
			fds->revents |= (fds->events & (POLLOUT | POLLWRNORM | POLLWRBAND));
		}
		break;

	case EVFILT_VNODE:
		if (kevp->fflags & NOTE_EXTEND) {
			fds->revents |= (fds->events & POLLEXTEND);
		}
		if (kevp->fflags & NOTE_ATTRIB) {
			fds->revents |= (fds->events & POLLATTRIB);
		}
		if (kevp->fflags & NOTE_LINK) {
			fds->revents |= (fds->events & POLLNLINK);
		}
		if (kevp->fflags & NOTE_WRITE) {
			fds->revents |= (fds->events & POLLWRITE);
		}
		break;
	}

	if (fds->revents != 0 && prev_revents == 0) {
		kectx->kec_process_noutputs++;
	}

	return 0;
}

int
seltrue(__unused dev_t dev, __unused int flag, __unused struct proc *p)
{
	return 1;
}

  
           
  
                                                                         
                                                                          
                               
  
                                               
                                
                                          
                                                    
  
                       
                                         
                                                   
                                
  
                                             
  
                                                                      
                                                                  
                                                           
                                             
   
static int
selcount(struct proc *p, u_int32_t *ibits, int nfd, int *countp)
{
	struct filedesc *fdp = p->p_fd;
	int msk, i, j, fd;
	u_int32_t bits;
	struct fileproc *fp;
	int n = 0;
	u_int32_t *iptr;
	u_int nw;
	int error = 0;
	int need_wakeup = 0;

	  
                                                    
                                                  
    
	if (fdp == NULL) {
		*countp = 0;
		return EIO;
	}
	nw = howmany(nfd, NFDBITS);

	proc_fdlock(p);
	for (msk = 0; msk < 3; msk++) {
		iptr = (u_int32_t *)&ibits[msk * nw];
		for (i = 0; i < nfd; i += NFDBITS) {
			bits = iptr[i / NFDBITS];
			while ((j = ffs(bits)) && (fd = i + --j) < nfd) {
				bits &= ~(1U << j);

				fp = fp_get_noref_locked(p, fd);
				if (fp == NULL) {
					*countp = 0;
					error = EBADF;
					goto bad;
				}
				os_ref_retain_locked(&fp->fp_iocount);
				n++;
			}
		}
	}
	proc_fdunlock(p);

	*countp = n;
	return 0;

bad:
	if (n == 0) {
		goto out;
	}
	                                             
	(void)seldrop_locked(p, ibits, nfd, n, &need_wakeup);

out:
	proc_fdunlock(p);
	if (need_wakeup) {
		wakeup(&p->p_fpdrainwait);
	}
	return error;
}


  
                 
  
                                                                           
                                                                       
  
                                                
                                    
                        
                                              
                                 
                     
                                                      
                                              
  
                       
                                             
                                  
                                 
  
                                                                    
                                                               
                                                           
                                                   
   
static int
seldrop_locked(struct proc *p, u_int32_t *ibits, int nfd, int lim, int *need_wakeup)
{
	struct filedesc *fdp = p->p_fd;
	int msk, i, j, nc, fd;
	u_int32_t bits;
	struct fileproc *fp;
	u_int32_t *iptr;
	u_int nw;
	int error = 0;
	uthread_t uth = get_bsdthread_info(current_thread());
	struct _select_data *seldata;

	*need_wakeup = 0;

	  
                                                    
                                                  
    
	if (fdp == NULL) {
		return EIO;
	}

	nw = howmany(nfd, NFDBITS);
	seldata = &uth->uu_save.uus_select_data;

	nc = 0;
	for (msk = 0; msk < 3; msk++) {
		iptr = (u_int32_t *)&ibits[msk * nw];
		for (i = 0; i < nfd; i += NFDBITS) {
			bits = iptr[i / NFDBITS];
			while ((j = ffs(bits)) && (fd = i + --j) < nfd) {
				bits &= ~(1U << j);
				  
                                               
                                         
       
				if (nc >= lim) {
					goto done;
				}

				  
                                            
                                        
       
				fp = fp_get_noref_locked_with_iocount(p, fd);
				selunlinkfp(fp,
				    seldata->wqp ? seldata->wqp[nc] : 0,
				    uth->uu_wqset);

				nc++;

				const os_ref_count_t refc = os_ref_release_locked(&fp->fp_iocount);
				if (0 == refc) {
					panic("fp_iocount overdecrement!");
				}

				if (1 == refc) {
					  
                                                    
                                                    
                                                    
                                     
        
					if (fp->fp_flags & FP_SELCONFLICT) {
						fp->fp_flags &= ~FP_SELCONFLICT;
					}
					if (p->p_fpdrainwait) {
						p->p_fpdrainwait = 0;
						*need_wakeup = 1;
					}
				}
			}
		}
	}
done:
	return error;
}


static int
seldrop(struct proc *p, u_int32_t *ibits, int nfd, int lim)
{
	int error;
	int need_wakeup = 0;

	proc_fdlock(p);
	error = seldrop_locked(p, ibits, nfd, lim, &need_wakeup);
	proc_fdunlock(p);
	if (need_wakeup) {
		wakeup(&p->p_fpdrainwait);
	}
	return error;
}

  
                           
   
void
selrecord(__unused struct proc *selector, struct selinfo *sip, void *s_data)
{
	thread_t        cur_act = current_thread();
	struct uthread * ut = get_bsdthread_info(cur_act);
	                                                                        
	uint64_t *reserved_link = (uint64_t *)s_data;

	                                

	                                                   
	if (!s_data) {
		return;
	}

	if ((sip->si_flags & SI_INITED) == 0) {
		waitq_init(&sip->si_waitq, SYNC_POLICY_FIFO);
		sip->si_flags |= SI_INITED;
		sip->si_flags &= ~SI_CLEAR;
	}

	if (sip->si_flags & SI_RECORDED) {
		sip->si_flags |= SI_COLL;
	} else {
		sip->si_flags &= ~SI_COLL;
	}

	sip->si_flags |= SI_RECORDED;
	                                                
	waitq_link(&sip->si_waitq, ut->uu_wqset,
	    WAITQ_SHOULD_LOCK, reserved_link);

	  
                                     
                                                             
                                                                 
                                                                  
                                                                      
                                     
    
	waitq_link_release(*reserved_link);
	*reserved_link = 0;

	  
                                                         
                                                                 
                                                           
                                                                    
                                                                    
                                                                   
                                                             
                                               
                                  
    
	void *wqptr = (void *)&sip->si_waitq;
	memcpy((void *)s_data, (void *)&wqptr, sizeof(void *));

	return;
}

void
selwakeup(struct selinfo *sip)
{
	if ((sip->si_flags & SI_INITED) == 0) {
		return;
	}

	if (sip->si_flags & SI_COLL) {
		nselcoll++;
		sip->si_flags &= ~SI_COLL;
#if 0
		                       
		                            
#endif
	}

	if (sip->si_flags & SI_RECORDED) {
		waitq_wakeup64_all(&sip->si_waitq, NO_EVENT64,
		    THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);
		sip->si_flags &= ~SI_RECORDED;
	}
}

void
selthreadclear(struct selinfo *sip)
{
	struct waitq *wq;

	if ((sip->si_flags & SI_INITED) == 0) {
		return;
	}
	if (sip->si_flags & SI_RECORDED) {
		selwakeup(sip);
		sip->si_flags &= ~(SI_RECORDED | SI_COLL);
	}
	sip->si_flags |= SI_CLEAR;
	sip->si_flags &= ~SI_INITED;

	wq = &sip->si_waitq;

	  
                                                                    
                                                                     
                                                                       
                                                                    
               
    
	waitq_deinit(wq);
}


  
              
  
                                                                         
  
                                                          
                                 
  
                       
                                     
                                   
                                                                             
  
                                                                   
                                                     
   
int
gethostuuid(struct proc *p, struct gethostuuid_args *uap, __unused int32_t *retval)
{
	kern_return_t kret;
	int error;
	mach_timespec_t mach_ts;                            
	__darwin_uuid_t uuid_kern = {};                     

	                       
	if (!IOTaskHasEntitlement(current_task(), "com.apple.private.getprivatesysid")) {
#if !defined(XNU_TARGET_OS_OSX)
#if CONFIG_MACF
		if ((error = mac_system_check_info(kauth_cred_get(), "hw.uuid")) != 0) {
			                                               
			return error;
		}
#endif
#endif
	}

	                                                           
	if (proc_is64bit(p)) {
		struct user64_timespec ts;
		error = copyin(uap->timeoutp, &ts, sizeof(ts));
		if (error) {
			return error;
		}
		mach_ts.tv_sec = (unsigned int)ts.tv_sec;
		mach_ts.tv_nsec = (clock_res_t)ts.tv_nsec;
	} else {
		struct user32_timespec ts;
		error = copyin(uap->timeoutp, &ts, sizeof(ts));
		if (error) {
			return error;
		}
		mach_ts.tv_sec = ts.tv_sec;
		mach_ts.tv_nsec = ts.tv_nsec;
	}

	                                                      
	kret = IOBSDGetPlatformUUID(uuid_kern, mach_ts);

	  
                                                                   
                                                                   
                  
    
	if (kret == KERN_SUCCESS) {
		error = copyout(uuid_kern, uap->uuid_buf, sizeof(uuid_kern));
	} else {
		error = EWOULDBLOCK;
	}

	return error;
}

  
         
  
                                                         
   
int
ledger(struct proc *p, struct ledger_args *args, __unused int32_t *retval)
{
#if !CONFIG_MACF
#pragma unused(p)
#endif
	int rval, pid, len, error;
#ifdef LEDGER_DEBUG
	struct ledger_limit_args lla;
#endif
	task_t task;
	proc_t proc;

	                                                                      
	error = 0;
	len = 0;
	if (args->cmd == LEDGER_ENTRY_INFO) {
		error = copyin(args->arg3, (char *)&len, sizeof(len));
	} else if (args->cmd == LEDGER_TEMPLATE_INFO) {
		error = copyin(args->arg2, (char *)&len, sizeof(len));
	} else if (args->cmd == LEDGER_LIMIT)
#ifdef LEDGER_DEBUG
	{ error = copyin(args->arg2, (char *)&lla, sizeof(lla));}
#else
	{ return EINVAL; }
#endif
	else if ((args->cmd < 0) || (args->cmd > LEDGER_MAX_CMD)) {
		return EINVAL;
	}

	if (error) {
		return error;
	}
	if (len < 0) {
		return EINVAL;
	}

	rval = 0;
	if (args->cmd != LEDGER_TEMPLATE_INFO) {
		pid = (int)args->arg1;
		proc = proc_find(pid);
		if (proc == NULL) {
			return ESRCH;
		}

#if CONFIG_MACF
		error = mac_proc_check_ledger(p, proc, args->cmd);
		if (error) {
			proc_rele(proc);
			return error;
		}
#endif

		task = proc->task;
	}

	switch (args->cmd) {
#ifdef LEDGER_DEBUG
	case LEDGER_LIMIT: {
		if (!kauth_cred_issuser(kauth_cred_get())) {
			rval = EPERM;
		}
		rval = ledger_limit(task, &lla);
		proc_rele(proc);
		break;
	}
#endif
	case LEDGER_INFO: {
		struct ledger_info info = {};

		rval = ledger_info(task, &info);
		proc_rele(proc);
		if (rval == 0) {
			rval = copyout(&info, args->arg2,
			    sizeof(info));
		}
		break;
	}

	case LEDGER_ENTRY_INFO: {
		void *buf;
		int sz;

		rval = ledger_get_task_entry_info_multiple(task, &buf, &len);
		proc_rele(proc);
		if ((rval == 0) && (len >= 0)) {
			sz = len * sizeof(struct ledger_entry_info);
			rval = copyout(buf, args->arg2, sz);
			kheap_free(KHEAP_DATA_BUFFERS, buf, sz);
		}
		if (rval == 0) {
			rval = copyout(&len, args->arg3, sizeof(len));
		}
		break;
	}

	case LEDGER_TEMPLATE_INFO: {
		void *buf;
		int sz;

		rval = ledger_template_info(&buf, &len);
		if ((rval == 0) && (len >= 0)) {
			sz = len * sizeof(struct ledger_template_info);
			rval = copyout(buf, args->arg1, sz);
			kheap_free(KHEAP_DATA_BUFFERS, buf, sz);
		}
		if (rval == 0) {
			rval = copyout(&len, args->arg2, sizeof(len));
		}
		break;
	}

	default:
		panic("ledger syscall logic error -- command type %d", args->cmd);
		proc_rele(proc);
		rval = EINVAL;
	}

	return rval;
}

int
telemetry(__unused struct proc *p, struct telemetry_args *args, __unused int32_t *retval)
{
	int error = 0;

	switch (args->cmd) {
#if CONFIG_TELEMETRY
	case TELEMETRY_CMD_TIMER_EVENT:
		error = telemetry_timer_event(args->deadline, args->interval, args->leeway);
		break;
	case TELEMETRY_CMD_PMI_SETUP:
		error = telemetry_pmi_setup((enum telemetry_pmi)args->deadline, args->interval);
		break;
#endif                       
	case TELEMETRY_CMD_VOUCHER_NAME:
		if (thread_set_voucher_name((mach_port_name_t)args->deadline)) {
			error = EINVAL;
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

  
          
  
                                                               
  
        
                                                        
                                     
                                                
                         
   
int
log_data(__unused struct proc *p, struct log_data_args *args, int *retval)
{
	unsigned int tag = args->tag;
	unsigned int flags = args->flags;
	user_addr_t buffer = args->buffer;
	unsigned int size = args->size;
	int ret = 0;
	char *log_msg = NULL;
	int error;
	*retval = 0;

	  
                                                       
                                    
    
	if (tag != 0) {
		return EINVAL;
	}

	  
                                               
                                                      
    
	if (flags != OS_LOG_TYPE_DEFAULT &&
	    flags != OS_LOG_TYPE_INFO &&
	    flags != OS_LOG_TYPE_DEBUG &&
	    flags != OS_LOG_TYPE_ERROR &&
	    flags != OS_LOG_TYPE_FAULT) {
		return EINVAL;
	}

	if (size == 0) {
		return EINVAL;
	}

	                                      
	if (size > OS_LOG_DATA_MAX_SIZE) {
		printf("%s: WARNING msg is going to be truncated from %u to %u\n",
		    __func__, size, OS_LOG_DATA_MAX_SIZE);
		size = OS_LOG_DATA_MAX_SIZE;
	}

	log_msg = kheap_alloc(KHEAP_TEMP, size, Z_WAITOK);
	if (!log_msg) {
		return ENOMEM;
	}

	error = copyin(buffer, log_msg, size);
	if (error) {
		ret = EFAULT;
		goto out;
	}
	log_msg[size - 1] = '\0';

	  
                                    
                                     
                                       
    
	os_log_driverKit(&ret, OS_LOG_DEFAULT, (os_log_type_t)flags, "%s", log_msg);

out:
	if (log_msg != NULL) {
		kheap_free(KHEAP_TEMP, log_msg, size);
	}

	return ret;
}

#if DEVELOPMENT || DEBUG
#if CONFIG_WAITQ_DEBUG
static uint64_t g_wqset_num = 0;
struct g_wqset {
	queue_chain_t      link;
	struct waitq_set  *wqset;
};

static queue_head_t         g_wqset_list;
static struct waitq_set    *g_waitq_set = NULL;

static inline struct waitq_set *
sysctl_get_wqset(int idx)
{
	struct g_wqset *gwqs;

	if (!g_wqset_num) {
		queue_init(&g_wqset_list);
	}

	                                                      
	qe_foreach_element(gwqs, &g_wqset_list, link) {
		if ((int)(wqset_id(gwqs->wqset) & 0xffffffff) == idx) {
			return gwqs->wqset;
		}
	}

	                        
	++g_wqset_num;
	gwqs = (struct g_wqset *)kalloc(sizeof(*gwqs));
	assert(gwqs != NULL);

	gwqs->wqset = waitq_set_alloc(SYNC_POLICY_FIFO | SYNC_POLICY_PREPOST, NULL);
	enqueue_tail(&g_wqset_list, &gwqs->link);
	printf("[WQ]: created new waitq set 0x%llx\n", wqset_id(gwqs->wqset));

	return gwqs->wqset;
}

#define MAX_GLOBAL_TEST_QUEUES 64
static int g_wq_init = 0;
static struct waitq  g_wq[MAX_GLOBAL_TEST_QUEUES];

static inline struct waitq *
global_test_waitq(int idx)
{
	if (idx < 0) {
		return NULL;
	}

	if (!g_wq_init) {
		g_wq_init = 1;
		for (int i = 0; i < MAX_GLOBAL_TEST_QUEUES; i++) {
			waitq_init(&g_wq[i], SYNC_POLICY_FIFO);
		}
	}

	return &g_wq[idx % MAX_GLOBAL_TEST_QUEUES];
}

static int sysctl_waitq_wakeup_one SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error;
	int index;
	struct waitq *waitq;
	kern_return_t kr;
	int64_t event64 = 0;

	error = SYSCTL_IN(req, &event64, sizeof(event64));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		return SYSCTL_OUT(req, &event64, sizeof(event64));
	}

	if (event64 < 0) {
		index = (int)((-event64) & 0xffffffff);
		waitq = wqset_waitq(sysctl_get_wqset(index));
		index = -index;
	} else {
		index = (int)event64;
		waitq = global_test_waitq(index);
	}

	event64 = 0;

	printf("[WQ]: Waking one thread on waitq [%d] event:0x%llx\n",
	    index, event64);
	kr = waitq_wakeup64_one(waitq, (event64_t)event64, THREAD_AWAKENED,
	    WAITQ_ALL_PRIORITIES);
	printf("[WQ]: \tkr=%d\n", kr);

	return SYSCTL_OUT(req, &kr, sizeof(kr));
}
SYSCTL_PROC(_kern, OID_AUTO, waitq_wakeup_one, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_waitq_wakeup_one, "Q", "wakeup one thread waiting on given event");


static int sysctl_waitq_wakeup_all SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error;
	int index;
	struct waitq *waitq;
	kern_return_t kr;
	int64_t event64 = 0;

	error = SYSCTL_IN(req, &event64, sizeof(event64));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		return SYSCTL_OUT(req, &event64, sizeof(event64));
	}

	if (event64 < 0) {
		index = (int)((-event64) & 0xffffffff);
		waitq = wqset_waitq(sysctl_get_wqset(index));
		index = -index;
	} else {
		index = (int)event64;
		waitq = global_test_waitq(index);
	}

	event64 = 0;

	printf("[WQ]: Waking all threads on waitq [%d] event:0x%llx\n",
	    index, event64);
	kr = waitq_wakeup64_all(waitq, (event64_t)event64,
	    THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);
	printf("[WQ]: \tkr=%d\n", kr);

	return SYSCTL_OUT(req, &kr, sizeof(kr));
}
SYSCTL_PROC(_kern, OID_AUTO, waitq_wakeup_all, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_waitq_wakeup_all, "Q", "wakeup all threads waiting on given event");


static int sysctl_waitq_wait SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error;
	int index;
	struct waitq *waitq;
	kern_return_t kr;
	int64_t event64 = 0;

	error = SYSCTL_IN(req, &event64, sizeof(event64));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		return SYSCTL_OUT(req, &event64, sizeof(event64));
	}

	if (event64 < 0) {
		index = (int)((-event64) & 0xffffffff);
		waitq = wqset_waitq(sysctl_get_wqset(index));
		index = -index;
	} else {
		index = (int)event64;
		waitq = global_test_waitq(index);
	}

	event64 = 0;

	printf("[WQ]: Current thread waiting on waitq [%d] event:0x%llx\n",
	    index, event64);
	kr = waitq_assert_wait64(waitq, (event64_t)event64, THREAD_INTERRUPTIBLE, 0);
	if (kr == THREAD_WAITING) {
		thread_block(THREAD_CONTINUE_NULL);
	}
	printf("[WQ]: \tWoke Up: kr=%d\n", kr);

	return SYSCTL_OUT(req, &kr, sizeof(kr));
}
SYSCTL_PROC(_kern, OID_AUTO, waitq_wait, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_waitq_wait, "Q", "start waiting on given event");


static int sysctl_wqset_select SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error;
	struct waitq_set *wqset;
	uint64_t event64 = 0;

	error = SYSCTL_IN(req, &event64, sizeof(event64));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		goto out;
	}

	wqset = sysctl_get_wqset((int)(event64 & 0xffffffff));
	g_waitq_set = wqset;

	event64 = wqset_id(wqset);
	printf("[WQ]: selected wqset 0x%llx\n", event64);

out:
	if (g_waitq_set) {
		event64 = wqset_id(g_waitq_set);
	} else {
		event64 = (uint64_t)(-1);
	}

	return SYSCTL_OUT(req, &event64, sizeof(event64));
}
SYSCTL_PROC(_kern, OID_AUTO, wqset_select, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_wqset_select, "Q", "select/create a global waitq set");


static int sysctl_waitq_link SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error;
	int index;
	struct waitq *waitq;
	struct waitq_set *wqset;
	kern_return_t kr;
	uint64_t reserved_link = 0;
	int64_t event64 = 0;

	error = SYSCTL_IN(req, &event64, sizeof(event64));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		return SYSCTL_OUT(req, &event64, sizeof(event64));
	}

	if (!g_waitq_set) {
		g_waitq_set = sysctl_get_wqset(1);
	}
	wqset = g_waitq_set;

	if (event64 < 0) {
		struct waitq_set *tmp;
		index = (int)((-event64) & 0xffffffff);
		tmp = sysctl_get_wqset(index);
		if (tmp == wqset) {
			goto out;
		}
		waitq = wqset_waitq(tmp);
		index = -index;
	} else {
		index = (int)event64;
		waitq = global_test_waitq(index);
	}

	printf("[WQ]: linking waitq [%d] to global wqset (0x%llx)\n",
	    index, wqset_id(wqset));
	reserved_link = waitq_link_reserve(waitq);
	kr = waitq_link(waitq, wqset, WAITQ_SHOULD_LOCK, &reserved_link);
	waitq_link_release(reserved_link);

	printf("[WQ]: \tkr=%d\n", kr);

out:
	return SYSCTL_OUT(req, &kr, sizeof(kr));
}
SYSCTL_PROC(_kern, OID_AUTO, waitq_link, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_waitq_link, "Q", "link global waitq to test waitq set");


static int sysctl_waitq_unlink SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error;
	int index;
	struct waitq *waitq;
	struct waitq_set *wqset;
	kern_return_t kr;
	uint64_t event64 = 0;

	error = SYSCTL_IN(req, &event64, sizeof(event64));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		return SYSCTL_OUT(req, &event64, sizeof(event64));
	}

	if (!g_waitq_set) {
		g_waitq_set = sysctl_get_wqset(1);
	}
	wqset = g_waitq_set;

	index = (int)event64;
	waitq = global_test_waitq(index);

	printf("[WQ]: unlinking waitq [%d] from global wqset (0x%llx)\n",
	    index, wqset_id(wqset));

	kr = waitq_unlink(waitq, wqset);
	printf("[WQ]: \tkr=%d\n", kr);

	return SYSCTL_OUT(req, &kr, sizeof(kr));
}
SYSCTL_PROC(_kern, OID_AUTO, waitq_unlink, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_waitq_unlink, "Q", "unlink global waitq from test waitq set");


static int sysctl_waitq_clear_prepost SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	struct waitq *waitq;
	uint64_t event64 = 0;
	int error, index;

	error = SYSCTL_IN(req, &event64, sizeof(event64));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		return SYSCTL_OUT(req, &event64, sizeof(event64));
	}

	index = (int)event64;
	waitq = global_test_waitq(index);

	printf("[WQ]: clearing prepost on waitq [%d]\n", index);
	waitq_clear_prepost(waitq);

	return SYSCTL_OUT(req, &event64, sizeof(event64));
}
SYSCTL_PROC(_kern, OID_AUTO, waitq_clear_prepost, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_waitq_clear_prepost, "Q", "clear prepost on given waitq");


static int sysctl_wqset_unlink_all SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error;
	struct waitq_set *wqset;
	kern_return_t kr;
	uint64_t event64 = 0;

	error = SYSCTL_IN(req, &event64, sizeof(event64));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		return SYSCTL_OUT(req, &event64, sizeof(event64));
	}

	if (!g_waitq_set) {
		g_waitq_set = sysctl_get_wqset(1);
	}
	wqset = g_waitq_set;

	printf("[WQ]: unlinking all queues from global wqset (0x%llx)\n",
	    wqset_id(wqset));

	kr = waitq_set_unlink_all(wqset);
	printf("[WQ]: \tkr=%d\n", kr);

	return SYSCTL_OUT(req, &kr, sizeof(kr));
}
SYSCTL_PROC(_kern, OID_AUTO, wqset_unlink_all, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_wqset_unlink_all, "Q", "unlink all queues from test waitq set");


static int sysctl_wqset_clear_preposts SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	struct waitq_set *wqset = NULL;
	uint64_t event64 = 0;
	int error, index;

	error = SYSCTL_IN(req, &event64, sizeof(event64));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		goto out;
	}

	index = (int)((event64) & 0xffffffff);
	wqset = sysctl_get_wqset(index);
	assert(wqset != NULL);

	printf("[WQ]: clearing preposts on wqset 0x%llx\n", wqset_id(wqset));
	waitq_set_clear_preposts(wqset);

out:
	if (wqset) {
		event64 = wqset_id(wqset);
	} else {
		event64 = (uint64_t)(-1);
	}

	return SYSCTL_OUT(req, &event64, sizeof(event64));
}
SYSCTL_PROC(_kern, OID_AUTO, wqset_clear_preposts, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_wqset_clear_preposts, "Q", "clear preposts on given waitq set");

#endif                         

static int
sysctl_waitq_set_nelem SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int nelem;

	                
	if (req->newptr != USER_ADDR_NULL) {
		return EPERM;
	}

	nelem = sysctl_helper_waitq_set_nelem();

	return SYSCTL_OUT(req, &nelem, sizeof(nelem));
}

SYSCTL_PROC(_kern, OID_AUTO, n_ltable_entries, CTLFLAG_RD | CTLFLAG_LOCKED,
    0, 0, sysctl_waitq_set_nelem, "I", "ltable elementis currently used");


static int
sysctl_mpsc_test_pingpong SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	uint64_t value = 0;
	int error;

	error = SYSCTL_IN(req, &value, sizeof(value));
	if (error) {
		return error;
	}

	if (error == 0 && req->newptr) {
		error = mpsc_test_pingpong(value, &value);
		if (error == 0) {
			error = SYSCTL_OUT(req, &value, sizeof(value));
		}
	}

	return error;
}
SYSCTL_PROC(_kern, OID_AUTO, mpsc_test_pingpong, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_mpsc_test_pingpong, "Q", "MPSC tests: pingpong");

#endif                           

                   
SYSCTL_NODE(_machdep, OID_AUTO, remotetime, CTLFLAG_RD | CTLFLAG_LOCKED, 0, "Remote time api");

#if DEVELOPMENT || DEBUG
#if CONFIG_MACH_BRIDGE_SEND_TIME
extern _Atomic uint32_t bt_init_flag;
extern uint32_t mach_bridge_timer_enable(uint32_t, int);

SYSCTL_INT(_machdep_remotetime, OID_AUTO, bridge_timer_init_flag,
    CTLFLAG_RD | CTLFLAG_LOCKED, &bt_init_flag, 0, "");

static int sysctl_mach_bridge_timer_enable SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	uint32_t value = 0;
	int error = 0;
	                                  
	if (req->oldptr == USER_ADDR_NULL && req->newptr == USER_ADDR_NULL) {
		req->oldidx = sizeof(value);
		return 0;
	}
	if (os_atomic_load(&bt_init_flag, acquire)) {
		if (req->newptr) {
			int new_value = 0;
			error = SYSCTL_IN(req, &new_value, sizeof(new_value));
			if (error) {
				return error;
			}
			if (new_value == 0 || new_value == 1) {
				value = mach_bridge_timer_enable(new_value, 1);
			} else {
				return EPERM;
			}
		} else {
			value = mach_bridge_timer_enable(0, 0);
		}
	}
	error = SYSCTL_OUT(req, &value, sizeof(value));
	return error;
}

SYSCTL_PROC(_machdep_remotetime, OID_AUTO, bridge_timer_enable,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_mach_bridge_timer_enable, "I", "");

#endif                                   

static int sysctl_mach_bridge_remote_time SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	uint64_t ltime = 0, rtime = 0;
	if (req->oldptr == USER_ADDR_NULL) {
		req->oldidx = sizeof(rtime);
		return 0;
	}
	if (req->newptr) {
		int error = SYSCTL_IN(req, &ltime, sizeof(ltime));
		if (error) {
			return error;
		}
	}
	rtime = mach_bridge_remote_time(ltime);
	return SYSCTL_OUT(req, &rtime, sizeof(rtime));
}
SYSCTL_PROC(_machdep_remotetime, OID_AUTO, mach_bridge_remote_time,
    CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_mach_bridge_remote_time, "Q", "");

#endif                           

#if CONFIG_MACH_BRIDGE_RECV_TIME
extern struct bt_params bt_params_get_latest(void);

static int sysctl_mach_bridge_conversion_params SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	struct bt_params params = {};
	if (req->oldptr == USER_ADDR_NULL) {
		req->oldidx = sizeof(struct bt_params);
		return 0;
	}
	if (req->newptr) {
		return EPERM;
	}
	params = bt_params_get_latest();
	return SYSCTL_OUT(req, &params, MIN(sizeof(params), req->oldlen));
}

SYSCTL_PROC(_machdep_remotetime, OID_AUTO, conversion_params,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED, 0,
    0, sysctl_mach_bridge_conversion_params, "S,bt_params", "");

#endif                                   

#if DEVELOPMENT || DEBUG

#include <pexpert/pexpert.h>
extern int32_t sysctl_get_bound_cpuid(void);
extern kern_return_t sysctl_thread_bind_cpuid(int32_t cpuid);
static int
sysctl_kern_sched_thread_bind_cpu SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)

	  
                                                                  
                                                          
                                                               
    

	if (!PE_parse_boot_argn("enable_skstb", NULL, 0)) {
		return ENOENT;
	}

	int32_t cpuid = sysctl_get_bound_cpuid();

	int32_t new_value;
	int changed;
	int error = sysctl_io_number(req, cpuid, sizeof cpuid, &new_value, &changed);
	if (error) {
		return error;
	}

	if (changed) {
		kern_return_t kr = sysctl_thread_bind_cpuid(new_value);

		if (kr == KERN_NOT_SUPPORTED) {
			return ENOTSUP;
		}

		if (kr == KERN_INVALID_VALUE) {
			return ERANGE;
		}
	}

	return error;
}

SYSCTL_PROC(_kern, OID_AUTO, sched_thread_bind_cpu, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_kern_sched_thread_bind_cpu, "I", "");

#if __AMP__
extern char sysctl_get_bound_cluster_type(void);
extern void sysctl_thread_bind_cluster_type(char cluster_type);
static int
sysctl_kern_sched_thread_bind_cluster_type SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	char buff[4];

	if (!PE_parse_boot_argn("enable_skstb", NULL, 0)) {
		return ENOENT;
	}

	int error = SYSCTL_IN(req, buff, 1);
	if (error) {
		return error;
	}
	char cluster_type = buff[0];

	if (!req->newptr) {
		goto out;
	}

	sysctl_thread_bind_cluster_type(cluster_type);
out:
	cluster_type = sysctl_get_bound_cluster_type();
	buff[0] = cluster_type;

	return SYSCTL_OUT(req, buff, 1);
}

SYSCTL_PROC(_kern, OID_AUTO, sched_thread_bind_cluster_type, CTLTYPE_STRING | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_kern_sched_thread_bind_cluster_type, "A", "");

extern char sysctl_get_task_cluster_type(void);
extern void sysctl_task_set_cluster_type(char cluster_type);
static int
sysctl_kern_sched_task_set_cluster_type SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	char buff[4];

	if (!PE_parse_boot_argn("enable_skstsct", NULL, 0)) {
		return ENOENT;
	}

	int error = SYSCTL_IN(req, buff, 1);
	if (error) {
		return error;
	}
	char cluster_type = buff[0];

	if (!req->newptr) {
		goto out;
	}

	sysctl_task_set_cluster_type(cluster_type);
out:
	cluster_type = sysctl_get_task_cluster_type();
	buff[0] = cluster_type;

	return SYSCTL_OUT(req, buff, 1);
}

SYSCTL_PROC(_kern, OID_AUTO, sched_task_set_cluster_type, CTLTYPE_STRING | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_kern_sched_task_set_cluster_type, "A", "");

#if CONFIG_SCHED_EDGE

  
                         
  
                                                                       
                                                                             
                                                                           
                                  
  
                                      
   
extern sched_clutch_edge sched_edge_config_e_to_p;
extern sched_clutch_edge sched_edge_config_p_to_e;
extern kern_return_t sched_edge_sysctl_configure_e_to_p(uint64_t);
extern kern_return_t sched_edge_sysctl_configure_p_to_e(uint64_t);
extern sched_clutch_edge sched_edge_e_to_p(void);
extern sched_clutch_edge sched_edge_p_to_e(void);

static int sysctl_sched_edge_config_e_to_p SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error;
	kern_return_t kr;
	int64_t edge_config = 0;

	error = SYSCTL_IN(req, &edge_config, sizeof(edge_config));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		edge_config = sched_edge_e_to_p().sce_edge_packed;
		return SYSCTL_OUT(req, &edge_config, sizeof(edge_config));
	}

	kr = sched_edge_sysctl_configure_e_to_p(edge_config);
	return SYSCTL_OUT(req, &kr, sizeof(kr));
}
SYSCTL_PROC(_kern, OID_AUTO, sched_edge_config_e_to_p, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_sched_edge_config_e_to_p, "Q", "Edge Scheduler Config for E-to-P cluster");

static int sysctl_sched_edge_config_p_to_e SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error;
	kern_return_t kr;
	int64_t edge_config = 0;

	error = SYSCTL_IN(req, &edge_config, sizeof(edge_config));
	if (error) {
		return error;
	}

	if (!req->newptr) {
		edge_config = sched_edge_p_to_e().sce_edge_packed;
		return SYSCTL_OUT(req, &edge_config, sizeof(edge_config));
	}

	kr = sched_edge_sysctl_configure_p_to_e(edge_config);
	return SYSCTL_OUT(req, &kr, sizeof(kr));
}
SYSCTL_PROC(_kern, OID_AUTO, sched_edge_config_p_to_e, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_sched_edge_config_p_to_e, "Q", "Edge Scheduler Config for P-to-E cluster");

extern int sched_edge_restrict_ut;
SYSCTL_INT(_kern, OID_AUTO, sched_edge_restrict_ut, CTLFLAG_RW | CTLFLAG_LOCKED, &sched_edge_restrict_ut, 0, "Edge Scheduler Restrict UT Threads");
extern int sched_edge_restrict_bg;
SYSCTL_INT(_kern, OID_AUTO, sched_edge_restrict_bg, CTLFLAG_RW | CTLFLAG_LOCKED, &sched_edge_restrict_ut, 0, "Edge Scheduler Restrict BG Threads");
extern int sched_edge_migrate_ipi_immediate;
SYSCTL_INT(_kern, OID_AUTO, sched_edge_migrate_ipi_immediate, CTLFLAG_RW | CTLFLAG_LOCKED, &sched_edge_migrate_ipi_immediate, 0, "Edge Scheduler uses immediate IPIs for migration event based on execution latency");

#endif                        

#endif              

                                         
extern uint32_t ipc_control_port_options;
SYSCTL_INT(_kern, OID_AUTO, ipc_control_port_options,
    CTLFLAG_RD | CTLFLAG_LOCKED, &ipc_control_port_options, 0, "");

#endif                           

extern uint32_t task_exc_guard_default;

SYSCTL_INT(_kern, OID_AUTO, task_exc_guard_default,
    CTLFLAG_RD | CTLFLAG_LOCKED, &task_exc_guard_default, 0, "");


static int
sysctl_kern_tcsm_available SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	uint32_t value = machine_csv(CPUVN_CI) ? 1 : 0;

	if (req->newptr) {
		return EINVAL;
	}

	return SYSCTL_OUT(req, &value, sizeof(value));
}
SYSCTL_PROC(_kern, OID_AUTO, tcsm_available,
    CTLTYPE_INT | CTLFLAG_RD | CTLFLAG_LOCKED | CTLFLAG_MASKED | CTLFLAG_ANYBODY,
    0, 0, sysctl_kern_tcsm_available, "I", "");


static int
sysctl_kern_tcsm_enable SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	uint32_t soflags = 0;
	uint32_t old_value = thread_get_no_smt() ? 1 : 0;

	int error = SYSCTL_IN(req, &soflags, sizeof(soflags));
	if (error) {
		return error;
	}

	if (soflags && machine_csv(CPUVN_CI)) {
		thread_set_no_smt(true);
		machine_tecs(current_thread());
	}

	return SYSCTL_OUT(req, &old_value, sizeof(old_value));
}
SYSCTL_PROC(_kern, OID_AUTO, tcsm_enable,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED | CTLFLAG_MASKED | CTLFLAG_ANYBODY,
    0, 0, sysctl_kern_tcsm_enable, "I", "");


#if DEVELOPMENT || DEBUG
extern void sysctl_task_set_no_smt(char no_smt);
extern char sysctl_task_get_no_smt(void);

static int
sysctl_kern_sched_task_set_no_smt SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	char buff[4];

	int error = SYSCTL_IN(req, buff, 1);
	if (error) {
		return error;
	}
	char no_smt = buff[0];

	if (!req->newptr) {
		goto out;
	}

	sysctl_task_set_no_smt(no_smt);
out:
	no_smt = sysctl_task_get_no_smt();
	buff[0] = no_smt;

	return SYSCTL_OUT(req, buff, 1);
}

SYSCTL_PROC(_kern, OID_AUTO, sched_task_set_no_smt, CTLTYPE_STRING | CTLFLAG_RW | CTLFLAG_LOCKED | CTLFLAG_ANYBODY,
    0, 0, sysctl_kern_sched_task_set_no_smt, "A", "");

static int
sysctl_kern_sched_thread_set_no_smt(__unused struct sysctl_oid *oidp, __unused void *arg1, __unused int arg2, struct sysctl_req *req)
{
	int new_value, changed;
	int old_value = thread_get_no_smt() ? 1 : 0;
	int error = sysctl_io_number(req, old_value, sizeof(int), &new_value, &changed);

	if (changed) {
		thread_set_no_smt(!!new_value);
	}

	return error;
}

SYSCTL_PROC(_kern, OID_AUTO, sched_thread_set_no_smt,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED | CTLFLAG_ANYBODY,
    0, 0, sysctl_kern_sched_thread_set_no_smt, "I", "");

static int
sysctl_kern_debug_get_preoslog SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	static bool oneshot_executed = false;
	size_t preoslog_size = 0;
	const char *preoslog = NULL;

	                                                                         
	if (req->newptr) {
		uint8_t oneshot = 0;
		int error = SYSCTL_IN(req, &oneshot, sizeof(oneshot));
		if (error) {
			return error;
		}

		if (oneshot) {
			if (!OSCompareAndSwap8(false, true, &oneshot_executed)) {
				return EPERM;
			}
		}
	}

	preoslog = sysctl_debug_get_preoslog(&preoslog_size);
	if (preoslog == NULL || preoslog_size == 0) {
		return 0;
	}

	if (req->oldptr == USER_ADDR_NULL) {
		req->oldidx = preoslog_size;
		return 0;
	}

	return SYSCTL_OUT(req, preoslog, preoslog_size);
}

SYSCTL_PROC(_kern, OID_AUTO, preoslog, CTLTYPE_OPAQUE | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_kern_debug_get_preoslog, "-", "");

static int
sysctl_kern_task_set_filter_msg_flag SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int new_value, changed;
	int old_value = task_get_filter_msg_flag(current_task()) ? 1 : 0;
	int error = sysctl_io_number(req, old_value, sizeof(int), &new_value, &changed);

	if (changed) {
		task_set_filter_msg_flag(current_task(), !!new_value);
	}

	return error;
}

SYSCTL_PROC(_kern, OID_AUTO, task_set_filter_msg_flag, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_kern_task_set_filter_msg_flag, "I", "");

#endif                           
