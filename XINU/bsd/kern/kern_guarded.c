/*
 * Copyright (c) 2018 Apple Inc. All rights reserved.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file_internal.h>
#include <kern/exc_guard.h>
#include <sys/guarded.h>
#include <kern/kalloc.h>
#include <sys/sysproto.h>
#include <sys/vnode.h>
#include <sys/vnode_internal.h>
#include <sys/uio_internal.h>
#include <sys/ubc_internal.h>
#include <vfs/vfs_support.h>
#include <security/audit/audit.h>
#include <sys/syscall.h>
#include <sys/kauth.h>
#include <sys/kdebug.h>
#include <stdbool.h>
#include <vm/vm_protos.h>
#include <libkern/section_keywords.h>
#if CONFIG_MACF && CONFIG_VNGUARD
#include <security/mac.h>
#include <security/mac_framework.h>
#include <security/mac_policy.h>
#include <pexpert/pexpert.h>
#include <sys/sysctl.h>
#include <sys/reason.h>
#endif

#define f_flag fp_glob->fg_flag
extern int dofilewrite(vfs_context_t ctx, struct fileproc *fp,
    user_addr_t bufp, user_size_t nbyte, off_t offset,
    int flags, user_ssize_t *retval );
extern int do_uiowrite(struct proc *p, struct fileproc *fp, uio_t uio, int flags, user_ssize_t *retval);

  
                                                
   

kern_return_t task_exception_notify(exception_type_t exception,
    mach_exception_data_type_t code, mach_exception_data_type_t subcode);
kern_return_t task_violated_guard(mach_exception_code_t, mach_exception_subcode_t, void *);

  
                                                                
                                                                    
                                    
  
                                                                           
                                                             
  
                                                                 
                                                               
                                       
   

struct guarded_fileproc {
	struct fileproc gf_fileproc;
	u_int           gf_attrs;
	guardid_t       gf_guard;
};

ZONE_DECLARE(gfp_zone, "guarded_fileproc",
    sizeof(struct guarded_fileproc),
    ZC_NOENCRYPT | ZC_ZFREE_CLEARMEM);

static inline struct guarded_fileproc *
FP_TO_GFP(struct fileproc *fp)
{
	struct guarded_fileproc *gfp =
	    __container_of(fp, struct guarded_fileproc, gf_fileproc);

	zone_require(gfp_zone, gfp);
	return gfp;
}

#define GFP_TO_FP(gfp)  (&(gfp)->gf_fileproc)

struct gfp_crarg {
	guardid_t gca_guard;
	u_int gca_attrs;
};

static struct fileproc *
guarded_fileproc_alloc_init(void *crarg)
{
	struct gfp_crarg *aarg = crarg;
	struct guarded_fileproc *gfp;

	gfp = zalloc_flags(gfp_zone, Z_WAITOK | Z_ZERO);

	struct fileproc *fp = &gfp->gf_fileproc;
	os_ref_init(&fp->fp_iocount, &f_refgrp);
	fp->fp_flags = FTYPE_GUARDED;

	gfp->gf_guard = aarg->gca_guard;
	gfp->gf_attrs = aarg->gca_attrs;

	return GFP_TO_FP(gfp);
}

void
guarded_fileproc_free(struct fileproc *fp)
{
	struct guarded_fileproc *gfp = FP_TO_GFP(fp);
	zfree(gfp_zone, gfp);
}

static int
fp_lookup_guarded(proc_t p, int fd, guardid_t guard,
    struct guarded_fileproc **gfpp, int locked)
{
	struct fileproc *fp;
	int error;

	if ((error = fp_lookup(p, fd, &fp, locked)) != 0) {
		return error;
	}
	if (FILEPROC_TYPE(fp) != FTYPE_GUARDED) {
		(void) fp_drop(p, fd, fp, locked);
		return EINVAL;
	}
	struct guarded_fileproc *gfp = FP_TO_GFP(fp);

	if (guard != gfp->gf_guard) {
		(void) fp_drop(p, fd, fp, locked);
		return EPERM;                                 
	}
	if (gfpp) {
		*gfpp = gfp;
	}
	return 0;
}

  
                        
  
                                       
                                                                
                         
                     
    
  
                                                                   
   

int
fp_isguarded(struct fileproc *fp, u_int attrs)
{
	if (FILEPROC_TYPE(fp) == FTYPE_GUARDED) {
		return (attrs & FP_TO_GFP(fp)->gf_attrs) == attrs;
	}
	return 0;
}

extern char *proc_name_address(void *p);

int
fp_guard_exception(proc_t p, int fd, struct fileproc *fp, u_int flavor)
{
	if (FILEPROC_TYPE(fp) != FTYPE_GUARDED) {
		panic("%s corrupt fp %p flags %x", __func__, fp, fp->fp_flags);
	}

	struct guarded_fileproc *gfp = FP_TO_GFP(fp);
	                                                
	proc_fdlock_assert(p, LCK_MTX_ASSERT_OWNED);

	mach_exception_code_t code = 0;
	EXC_GUARD_ENCODE_TYPE(code, GUARD_TYPE_FD);
	EXC_GUARD_ENCODE_FLAVOR(code, flavor);
	EXC_GUARD_ENCODE_TARGET(code, fd);
	mach_exception_subcode_t subcode = gfp->gf_guard;

	thread_t t = current_thread();
	thread_guard_violation(t, code, subcode, TRUE);
	return EPERM;
}

  
                                                                   
   
void
fd_guard_ast(
	thread_t __unused t,
	mach_exception_code_t code,
	mach_exception_subcode_t subcode)
{
	task_exception_notify(EXC_GUARD, code, subcode);
	proc_t p = current_proc();
	psignal(p, SIGKILL);
}

  
                                            
   

  
                                                       
                                                      
  
                                                               
                                                                 
  
                                                                       
                                                                  
                                                                        
                                                                    
                                    
  
                                                                
                                                          
                                                             
                           
  
                                                                   
                                                         
                        
   
int
guarded_open_np(proc_t p, struct guarded_open_np_args *uap, int32_t *retval)
{
	if ((uap->flags & O_CLOEXEC) == 0) {
		return EINVAL;
	}

#define GUARD_REQUIRED (GUARD_DUP)
#define GUARD_ALL      (GUARD_REQUIRED |        \
	                (GUARD_CLOSE | GUARD_SOCKET_IPC | GUARD_FILEPORT | GUARD_WRITE))

	if (((uap->guardflags & GUARD_REQUIRED) != GUARD_REQUIRED) ||
	    ((uap->guardflags & ~GUARD_ALL) != 0)) {
		return EINVAL;
	}

	int error;
	struct gfp_crarg crarg = {
		.gca_attrs = uap->guardflags
	};

	if ((error = copyin(uap->guard,
	    &(crarg.gca_guard), sizeof(crarg.gca_guard))) != 0) {
		return error;
	}

	  
                                                    
    
	if (crarg.gca_guard == 0) {
		return EINVAL;
	}

	struct filedesc *fdp = p->p_fd;
	struct vnode_attr va;
	struct nameidata nd;
	vfs_context_t ctx = vfs_context_current();
	int cmode;

	VATTR_INIT(&va);
	cmode = ((uap->mode & ~fdp->fd_cmask) & ALLPERMS) & ~S_ISTXT;
	VATTR_SET(&va, va_mode, cmode & ACCESSPERMS);

	NDINIT(&nd, LOOKUP, OP_OPEN, FOLLOW | AUDITVNPATH1, UIO_USERSPACE,
	    uap->path, ctx);

	return open1(ctx, &nd, uap->flags | O_CLOFORK, &va,
	           guarded_fileproc_alloc_init, &crarg, retval);
}

  
                                                                  
                                                                                
  
                                                                                         
                                                                                      
                    
   
int
guarded_open_dprotected_np(proc_t p, struct guarded_open_dprotected_np_args *uap, int32_t *retval)
{
	if ((uap->flags & O_CLOEXEC) == 0) {
		return EINVAL;
	}

	if (((uap->guardflags & GUARD_REQUIRED) != GUARD_REQUIRED) ||
	    ((uap->guardflags & ~GUARD_ALL) != 0)) {
		return EINVAL;
	}

	int error;
	struct gfp_crarg crarg = {
		.gca_attrs = uap->guardflags
	};

	if ((error = copyin(uap->guard,
	    &(crarg.gca_guard), sizeof(crarg.gca_guard))) != 0) {
		return error;
	}

	  
                                                    
    
	if (crarg.gca_guard == 0) {
		return EINVAL;
	}

	struct filedesc *fdp = p->p_fd;
	struct vnode_attr va;
	struct nameidata nd;
	vfs_context_t ctx = vfs_context_current();
	int cmode;

	VATTR_INIT(&va);
	cmode = ((uap->mode & ~fdp->fd_cmask) & ALLPERMS) & ~S_ISTXT;
	VATTR_SET(&va, va_mode, cmode & ACCESSPERMS);

	NDINIT(&nd, LOOKUP, OP_OPEN, FOLLOW | AUDITVNPATH1, UIO_USERSPACE,
	    uap->path, ctx);

	  
                                                                         
                 
                             
                                                                       
    
	if (uap->flags & O_CREAT) {
		VATTR_SET(&va, va_dataprotect_class, uap->dpclass);
	}

	if (uap->dpflags & (O_DP_GETRAWENCRYPTED | O_DP_GETRAWUNENCRYPTED)) {
		if (uap->flags & (O_RDWR | O_WRONLY)) {
			                                              
			return EINVAL;
		}
		if (uap->dpflags & O_DP_GETRAWENCRYPTED) {
			VATTR_SET(&va, va_dataprotect_flags, VA_DP_RAWENCRYPTED);
		}
		if (uap->dpflags & O_DP_GETRAWUNENCRYPTED) {
			VATTR_SET(&va, va_dataprotect_flags, VA_DP_RAWUNENCRYPTED);
		}
	}

	return open1(ctx, &nd, uap->flags | O_CLOFORK, &va,
	           guarded_fileproc_alloc_init, &crarg, retval);
}

  
                                                                   
  
                                                                  
  
                                                            
                                                                         
                        
   
int
guarded_kqueue_np(proc_t p, struct guarded_kqueue_np_args *uap, int32_t *retval)
{
	if (((uap->guardflags & GUARD_REQUIRED) != GUARD_REQUIRED) ||
	    ((uap->guardflags & ~GUARD_ALL) != 0)) {
		return EINVAL;
	}

	int error;
	struct gfp_crarg crarg = {
		.gca_attrs = uap->guardflags
	};

	if ((error = copyin(uap->guard,
	    &(crarg.gca_guard), sizeof(crarg.gca_guard))) != 0) {
		return error;
	}

	if (crarg.gca_guard == 0) {
		return EINVAL;
	}

	return kqueue_internal(p, guarded_fileproc_alloc_init, &crarg, retval);
}

  
                                                        
   
int
guarded_close_np(proc_t p, struct guarded_close_np_args *uap,
    __unused int32_t *retval)
{
	struct guarded_fileproc *gfp;
	int fd = uap->fd;
	int error;
	guardid_t uguard;

	AUDIT_SYSCLOSE(p, fd);

	if ((error = copyin(uap->guard, &uguard, sizeof(uguard))) != 0) {
		return error;
	}

	proc_fdlock(p);
	if ((error = fp_lookup_guarded(p, fd, uguard, &gfp, 1)) != 0) {
		proc_fdunlock(p);
		return error;
	}
	fp_drop(p, fd, GFP_TO_FP(gfp), 1);
	return fp_close_and_unlock(p, fd, GFP_TO_FP(gfp), 0);
}

  
      
                                                                      
                                                                 
  
                                                                       
                                                               
                                          
  
                                                                        
                                                                      
                                                                  
                                            
  
                                                                           
                                                                  
  
                                                                    
                                                               
                                                                         
                                                                           
                                                        
  
                                                                        
                                                                  
                                                                     
                                                                             
  
                                                                         
                                                                          
                                                                           
                                                                          
                                                                           
  
                                                                          
                                                                 
  
                                                                       
                                                     
  
                     
                                                                     
                                     
                                                                     
                       
  
                                                                      
                                                                 
  
                                                            
                                                                          
                                                
                                                                      
                                    
  
                                                                
   

int
change_fdguard_np(proc_t p, struct change_fdguard_np_args *uap,
    __unused int32_t *retval)
{
	struct fileproc *fp;
	int fd = uap->fd;
	int error;
	guardid_t oldg = 0, newg = 0;
	int nfdflags = 0;

	if (0 != uap->guard &&
	    0 != (error = copyin(uap->guard, &oldg, sizeof(oldg)))) {
		return error;                                 
	}
	if (0 != uap->nguard &&
	    0 != (error = copyin(uap->nguard, &newg, sizeof(newg)))) {
		return error;                             
	}
	if (0 != uap->fdflagsp &&
	    0 != (error = copyin(uap->fdflagsp, &nfdflags, sizeof(nfdflags)))) {
		return error;                               
	}
	proc_fdlock(p);
restart:
	if ((error = fp_lookup(p, fd, &fp, 1)) != 0) {
		proc_fdunlock(p);
		return error;
	}

	if (0 != uap->fdflagsp) {
		int ofdflags = FDFLAGS_GET(p, fd);
		int ofl = ((ofdflags & UF_EXCLOSE) ? FD_CLOEXEC : 0) |
		    ((ofdflags & UF_FORKCLOSE) ? FD_CLOFORK : 0);
		proc_fdunlock(p);
		if (0 != (error = copyout(&ofl, uap->fdflagsp, sizeof(ofl)))) {
			proc_fdlock(p);
			goto dropout;                                
		}
		proc_fdlock(p);
	}

	if (FILEPROC_TYPE(fp) == FTYPE_GUARDED) {
		if (0 == uap->guard || 0 == uap->guardflags) {
			error = EINVAL;                     
		} else if (0 == oldg) {
			error = EPERM;                              
		}
	} else {
		if (0 != uap->guard || 0 != uap->guardflags) {
			error = EINVAL;                                       
		}
	}

	if (0 != error) {
		goto dropout;
	}

	if (0 != uap->nguard) {
		  
                                 
     
		if (0 == newg) {
			error = EINVAL;                                 
		} else if (((uap->nguardflags & GUARD_REQUIRED) != GUARD_REQUIRED) ||
		    ((uap->nguardflags & ~GUARD_ALL) != 0)) {
			error = EINVAL;                                     
		}
		if (0 != error) {
			goto dropout;
		}

		if (FILEPROC_TYPE(fp) == FTYPE_GUARDED) {
			  
                                      
      
			struct guarded_fileproc *gfp = FP_TO_GFP(fp);

			if (oldg == gfp->gf_guard &&
			    uap->guardflags == gfp->gf_attrs) {
				  
                                             
                                                   
                                                   
                                                 
       
				if (gfp->gf_attrs & GUARD_CLOSE) {
					FDFLAGS_CLR(p, fd, UF_FORKCLOSE);
				}
				gfp->gf_guard = newg;
				gfp->gf_attrs = uap->nguardflags;
				if (gfp->gf_attrs & GUARD_CLOSE) {
					FDFLAGS_SET(p, fd, UF_FORKCLOSE);
				}
				FDFLAGS_SET(p, fd,
				    (nfdflags & FD_CLOFORK) ? UF_FORKCLOSE : 0);
				                                     
			} else {
				error = EPERM;
			}
			goto dropout;
		} else {
			  
                                                      
      
			switch (FILEGLOB_DTYPE(fp->fp_glob)) {
			case DTYPE_VNODE:
			case DTYPE_PIPE:
			case DTYPE_SOCKET:
			case DTYPE_KQUEUE:
			case DTYPE_NETPOLICY:
				break;
			default:
				error = ENOTSUP;
				goto dropout;
			}

			proc_fdunlock(p);

			struct gfp_crarg crarg = {
				.gca_guard = newg,
				.gca_attrs = uap->nguardflags
			};
			struct fileproc *nfp =
			    guarded_fileproc_alloc_init(&crarg);
			struct guarded_fileproc *gfp;

			proc_fdlock(p);

			switch (error = fp_tryswap(p, fd, nfp)) {
			case 0:                                                    
				fp = NULL;
				gfp = FP_TO_GFP(nfp);
				if (gfp->gf_attrs & GUARD_CLOSE) {
					FDFLAGS_SET(p, fd, UF_FORKCLOSE);
				}
				FDFLAGS_SET(p, fd, UF_EXCLOSE);
				(void) fp_drop(p, fd, nfp, 1);
				break;
			case EKEEPLOOKING:                                       
				(void) fp_drop(p, fd, fp, 1);
				fileproc_free(nfp);
				goto restart;
			default:
				(void) fp_drop(p, fd, fp, 1);
				fileproc_free(nfp);
				break;
			}
			proc_fdunlock(p);
			return error;
		}
	} else {
		  
                  
     
		if (FILEPROC_TYPE(fp) == FTYPE_GUARDED) {
			  
                                  
      
			struct guarded_fileproc *gfp = FP_TO_GFP(fp);

			if (0 != uap->nguardflags) {
				error = EINVAL;
				goto dropout;
			}

			if (oldg != gfp->gf_guard ||
			    uap->guardflags != gfp->gf_attrs) {
				error = EPERM;
				goto dropout;
			}

			proc_fdunlock(p);
			struct fileproc *nfp = fileproc_alloc_init(NULL);
			proc_fdlock(p);

			switch (error = fp_tryswap(p, fd, nfp)) {
			case 0:                                                 
				fp = NULL;
				FDFLAGS_CLR(p, fd, UF_FORKCLOSE | UF_EXCLOSE);
				FDFLAGS_SET(p, fd,
				    (nfdflags & FD_CLOFORK) ? UF_FORKCLOSE : 0);
				                                     
				FDFLAGS_SET(p, fd,
				    (nfdflags & FD_CLOEXEC) ? UF_EXCLOSE : 0);
				(void) fp_drop(p, fd, nfp, 1);
				break;
			case EKEEPLOOKING:                                     
				(void) fp_drop(p, fd, fp, 1);
				fileproc_free(nfp);
				goto restart;
			default:
				(void) fp_drop(p, fd, fp, 1);
				fileproc_free(nfp);
				break;
			}
			proc_fdunlock(p);
			return error;
		} else {
			  
                                            
      
			error = EINVAL;
		}
	}

dropout:
	(void) fp_drop(p, fd, fp, 1);
	proc_fdunlock(p);
	return error;
}

  
                                                                
                                                                  
  
                                            
   
int
guarded_write_np(struct proc *p, struct guarded_write_np_args *uap, user_ssize_t *retval)
{
	int error;
	int fd = uap->fd;
	guardid_t uguard;
	struct fileproc *fp;
	struct guarded_fileproc *gfp;

	AUDIT_ARG(fd, fd);

	if ((error = copyin(uap->guard, &uguard, sizeof(uguard))) != 0) {
		return error;
	}

	error = fp_lookup_guarded(p, fd, uguard, &gfp, 0);
	if (error) {
		return error;
	}

	fp = GFP_TO_FP(gfp);
	if ((fp->f_flag & FWRITE) == 0) {
		error = EBADF;
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
guarded_pwrite_np(struct proc *p, struct guarded_pwrite_np_args *uap, user_ssize_t *retval)
{
	struct fileproc *fp;
	int error;
	int fd = uap->fd;
	vnode_t vp  = (vnode_t)0;
	guardid_t uguard;
	struct guarded_fileproc *gfp;

	AUDIT_ARG(fd, fd);

	if ((error = copyin(uap->guard, &uguard, sizeof(uguard))) != 0) {
		return error;
	}

	error = fp_lookup_guarded(p, fd, uguard, &gfp, 0);
	if (error) {
		return error;
	}

	fp = GFP_TO_FP(gfp);
	if ((fp->f_flag & FWRITE) == 0) {
		error = EBADF;
	} else {
		struct vfs_context context = *vfs_context_current();
		context.vc_ucred = fp->fp_glob->fg_cred;

		if (FILEGLOB_DTYPE(fp->fp_glob) != DTYPE_VNODE) {
			error = ESPIPE;
			goto errout;
		}
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

	KERNEL_DEBUG_CONSTANT((BSDDBG_CODE(DBG_BSD_SC_EXTENDED_INFO, SYS_guarded_pwrite_np) | DBG_FUNC_NONE),
	    uap->fd, uap->nbyte, (unsigned int)((uap->offset >> 32)), (unsigned int)(uap->offset), 0);

	return error;
}

  
                                                                 
                                                                       
  
                                            
  
   
int
guarded_writev_np(struct proc *p, struct guarded_writev_np_args *uap, user_ssize_t *retval)
{
	uio_t auio = NULL;
	int error;
	struct fileproc *fp;
	struct user_iovec *iovp;
	guardid_t uguard;
	struct guarded_fileproc *gfp;

	AUDIT_ARG(fd, uap->fd);

	                                               
	if (uap->iovcnt <= 0 || uap->iovcnt > UIO_MAXIOV) {
		return EINVAL;
	}

	                                                                     
	auio = uio_create(uap->iovcnt, 0,
	    (IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32),
	    UIO_WRITE);

	                                                                      
               
    
	iovp = uio_iovsaddr(auio);
	if (iovp == NULL) {
		error = ENOMEM;
		goto ExitThisRoutine;
	}
	error = copyin_user_iovec_array(uap->iovp,
	    IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32,
	    uap->iovcnt, iovp);
	if (error) {
		goto ExitThisRoutine;
	}

	                                       
    
	error = uio_calculateresid(auio);
	if (error) {
		goto ExitThisRoutine;
	}

	if ((error = copyin(uap->guard, &uguard, sizeof(uguard))) != 0) {
		goto ExitThisRoutine;
	}

	error = fp_lookup_guarded(p, uap->fd, uguard, &gfp, 0);
	if (error) {
		goto ExitThisRoutine;
	}

	fp = GFP_TO_FP(gfp);
	if ((fp->f_flag & FWRITE) == 0) {
		error = EBADF;
	} else {
		error = do_uiowrite(p, fp, auio, 0, retval);
	}

	fp_drop(p, uap->fd, fp, 0);
ExitThisRoutine:
	if (auio != NULL) {
		uio_free(auio);
	}
	return error;
}

  
                                                                    
                                                               
  
                                                                    
                                                                    
   
int
falloc_guarded(struct proc *p, struct fileproc **fp, int *fd,
    vfs_context_t ctx, const guardid_t *guard, u_int attrs)
{
	struct gfp_crarg crarg;

	if (((attrs & GUARD_REQUIRED) != GUARD_REQUIRED) ||
	    ((attrs & ~GUARD_ALL) != 0) || (*guard == 0)) {
		return EINVAL;
	}

	bzero(&crarg, sizeof(crarg));
	crarg.gca_guard = *guard;
	crarg.gca_attrs = attrs;

	return falloc_withalloc(p, fp, fd, ctx, guarded_fileproc_alloc_init,
	           &crarg);
}

#if CONFIG_MACF && CONFIG_VNGUARD

  
                 
  
                                                                           
                                                                    
                                                                        
                                                                      
                                           
  
                                                              
                                                                 
                  
  
                                                                             
                                                                     
                                                                
  
                                                                         
             
  
                                                                               
                                                  
   

struct vng_owner;

struct vng_info {                               
	guardid_t vgi_guard;
	unsigned vgi_attrs;
	TAILQ_HEAD(, vng_owner) vgi_owners;
};

struct vng_owner {                                  
	proc_t vgo_p;
	struct fileglob *vgo_fg;
	struct vng_info *vgo_vgi;
	TAILQ_ENTRY(vng_owner) vgo_link;
};

static struct vng_info *
new_vgi(unsigned attrs, guardid_t guard)
{
	struct vng_info *vgi = kalloc(sizeof(*vgi));
	vgi->vgi_guard = guard;
	vgi->vgi_attrs = attrs;
	TAILQ_INIT(&vgi->vgi_owners);
	return vgi;
}

static struct vng_owner *
new_vgo(proc_t p, struct fileglob *fg)
{
	struct vng_owner *vgo = kalloc(sizeof(*vgo));
	memset(vgo, 0, sizeof(*vgo));
	vgo->vgo_p = p;
	vgo->vgo_fg = fg;
	return vgo;
}

static void
vgi_add_vgo(struct vng_info *vgi, struct vng_owner *vgo)
{
	vgo->vgo_vgi = vgi;
	TAILQ_INSERT_HEAD(&vgi->vgi_owners, vgo, vgo_link);
}

static boolean_t
vgi_remove_vgo(struct vng_info *vgi, struct vng_owner *vgo)
{
	TAILQ_REMOVE(&vgi->vgi_owners, vgo, vgo_link);
	vgo->vgo_vgi = NULL;
	return TAILQ_EMPTY(&vgi->vgi_owners);
}

static void
free_vgi(struct vng_info *vgi)
{
	assert(TAILQ_EMPTY(&vgi->vgi_owners));
#if DEVELOP || DEBUG
	memset(vgi, 0xbeadfade, sizeof(*vgi));
#endif
	kfree(vgi, sizeof(*vgi));
}

static void
free_vgo(struct vng_owner *vgo)
{
#if DEVELOP || DEBUG
	memset(vgo, 0x2bedf1d0, sizeof(*vgo));
#endif
	kfree(vgo, sizeof(*vgo));
}

static int label_slot;
static LCK_GRP_DECLARE(llock_grp, VNG_POLICY_NAME);
static LCK_RW_DECLARE(llock, &llock_grp);

static __inline void *
vng_lbl_get(struct label *label)
{
	lck_rw_assert(&llock, LCK_RW_ASSERT_HELD);
	void *data;
	if (NULL == label) {
		data = NULL;
	} else {
		data = (void *)mac_label_get(label, label_slot);
	}
	return data;
}

static __inline struct vng_info *
vng_lbl_get_withattr(struct label *label, unsigned attrmask)
{
	struct vng_info *vgi = vng_lbl_get(label);
	assert(NULL == vgi || (vgi->vgi_attrs & ~VNG_ALL) == 0);
	if (NULL != vgi && 0 == (vgi->vgi_attrs & attrmask)) {
		vgi = NULL;
	}
	return vgi;
}

static __inline void
vng_lbl_set(struct label *label, void *data)
{
	assert(NULL != label);
	lck_rw_assert(&llock, LCK_RW_ASSERT_EXCLUSIVE);
	mac_label_set(label, label_slot, (intptr_t)data);
}

static int
vnguard_sysc_getguardattr(proc_t p, struct vnguard_getattr *vga)
{
	const int fd = vga->vga_fd;

	if (0 == vga->vga_guard) {
		return EINVAL;
	}

	int error;
	struct fileproc *fp;
	if (0 != (error = fp_lookup(p, fd, &fp, 0))) {
		return error;
	}
	do {
		struct fileglob *fg = fp->fp_glob;
		if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
			error = EBADF;
			break;
		}
		struct vnode *vp = fg->fg_data;
		if (!vnode_isreg(vp) || NULL == vp->v_mount) {
			error = EBADF;
			break;
		}
		error = vnode_getwithref(vp);
		if (0 != error) {
			break;
		}

		vga->vga_attrs = 0;

		lck_rw_lock_shared(&llock);

		if (NULL != vp->v_label) {
			const struct vng_info *vgi = vng_lbl_get(vp->v_label);
			if (NULL != vgi) {
				if (vgi->vgi_guard != vga->vga_guard) {
					error = EPERM;
				} else {
					vga->vga_attrs = vgi->vgi_attrs;
				}
			}
		}

		lck_rw_unlock_shared(&llock);
		vnode_put(vp);
	} while (0);

	fp_drop(p, fd, fp, 0);
	return error;
}

static int
vnguard_sysc_setguard(proc_t p, const struct vnguard_set *vns)
{
	const int fd = vns->vns_fd;

	if ((vns->vns_attrs & ~VNG_ALL) != 0 ||
	    0 == vns->vns_attrs || 0 == vns->vns_guard) {
		return EINVAL;
	}

	int error;
	struct fileproc *fp;
	if (0 != (error = fp_lookup(p, fd, &fp, 0))) {
		return error;
	}
	do {
		  
                                                 
                                       
     
		if ((FREAD | FWRITE) != (fp->f_flag & (FREAD | FWRITE))) {
			error = EBADF;
			break;
		}
		struct fileglob *fg = fp->fp_glob;
		if (FILEGLOB_DTYPE(fg) != DTYPE_VNODE) {
			error = EBADF;
			break;
		}
		  
                                                      
                                                      
              
     
		if (0 == (FG_CONFINED & fg->fg_lflags)) {
			error = EBADF;
			break;
		}
		struct vnode *vp = fg->fg_data;
		if (!vnode_isreg(vp) || NULL == vp->v_mount) {
			error = EBADF;
			break;
		}
		error = vnode_getwithref(vp);
		if (0 != error) {
			break;
		}

		                                           
		struct vfs_context *ctx = vfs_context_current();
		mac_vnode_label_update(ctx, vp, NULL);

		struct vng_info *nvgi = new_vgi(vns->vns_attrs, vns->vns_guard);
		struct vng_owner *nvgo = new_vgo(p, fg);

		lck_rw_lock_exclusive(&llock);

		do {
			  
                                                  
                                         
      
			struct vng_info *vgi = vng_lbl_get(vp->v_label);
			struct vng_owner *vgo = vng_lbl_get(fg->fg_label);

			if (NULL == vgi) {
				                                          
				if (NULL != vgo) {
					panic("vnguard label on fileglob "
					    "but not vnode");
				}
				                                             
				error = vnode_ref_ext(vp, O_EVTONLY, 0);
				if (0 == error) {
					                   
					vgi_add_vgo(nvgi, nvgo);
					vng_lbl_set(vp->v_label, nvgi);
					vng_lbl_set(fg->fg_label, nvgo);
				} else {
					free_vgo(nvgo);
					free_vgi(nvgi);
				}
			} else {
				                           
				free_vgi(nvgi);
				if (vgi->vgi_guard != vns->vns_guard) {
					error = EPERM;                     
				} else if (vgi->vgi_attrs != vns->vns_attrs) {
					  
                                                          
                                                            
        
					const unsigned mask = ~(VNG_WRITE_OTHER | VNG_TRUNC_OTHER);
					if ((vgi->vgi_attrs & mask) == (vns->vns_attrs & mask)) {
						vgi->vgi_attrs &= vns->vns_attrs;
					} else {
						error = EACCES;                    
					}
				}
				if (0 != error || NULL != vgo) {
					free_vgo(nvgo);
					break;
				}
				                             
				vgi_add_vgo(vgi, nvgo);
				vng_lbl_set(fg->fg_label, nvgo);
			}
		} while (0);

		lck_rw_unlock_exclusive(&llock);
		vnode_put(vp);
	} while (0);

	fp_drop(p, fd, fp, 0);
	return error;
}

static int
vng_policy_syscall(proc_t p, int cmd, user_addr_t arg)
{
	int error = EINVAL;

	switch (cmd) {
	case VNG_SYSC_PING:
		if (0 == arg) {
			error = 0;
		}
		break;
	case VNG_SYSC_SET_GUARD: {
		struct vnguard_set vns;
		error = copyin(arg, (void *)&vns, sizeof(vns));
		if (error) {
			break;
		}
		error = vnguard_sysc_setguard(p, &vns);
		break;
	}
	case VNG_SYSC_GET_ATTR: {
		struct vnguard_getattr vga;
		error = copyin(arg, (void *)&vga, sizeof(vga));
		if (error) {
			break;
		}
		error = vnguard_sysc_getguardattr(p, &vga);
		if (error) {
			break;
		}
		error = copyout((void *)&vga, arg, sizeof(vga));
		break;
	}
	default:
		break;
	}
	return error;
}

  
                                                                   
                                                             
                                         
   
static void
vng_file_label_destroy(struct label *label)
{
	lck_rw_lock_exclusive(&llock);
	struct vng_owner *lvgo = vng_lbl_get(label);
	if (lvgo) {
		vng_lbl_set(label, 0);
		struct vng_info *vgi = lvgo->vgo_vgi;
		assert(vgi);
		if (vgi_remove_vgo(vgi, lvgo)) {
			                                 
			vgi->vgi_attrs = 0;
			struct fileglob *fg = lvgo->vgo_fg;
			assert(fg);
			if (DTYPE_VNODE == FILEGLOB_DTYPE(fg)) {
				struct vnode *vp = fg->fg_data;
				int error = vnode_getwithref(vp);
				if (0 == error) {
					vng_lbl_set(vp->v_label, 0);
					lck_rw_unlock_exclusive(&llock);
					                               
					vnode_rele_ext(vp, O_EVTONLY, 0);
					vnode_put(vp);
					free_vgi(vgi);
					free_vgo(lvgo);
					return;
				}
			}
		}
		free_vgo(lvgo);
	}
	lck_rw_unlock_exclusive(&llock);
}

static os_reason_t
vng_reason_from_pathname(const char *path, uint32_t pathlen)
{
	os_reason_t r = os_reason_create(OS_REASON_GUARD, GUARD_REASON_VNODE);
	if (NULL == r) {
		return r;
	}
	  
                                                             
    
	const uint32_t pathmax = 3 * EXIT_REASON_USER_DESC_MAX_LEN / 4;
	if (pathlen > pathmax) {
		path += (pathlen - pathmax);
		pathlen = pathmax;
	}
	uint32_t rsize = kcdata_estimate_required_buffer_size(1, pathlen);
	if (0 == os_reason_alloc_buffer(r, rsize)) {
		struct kcdata_descriptor *kcd = &r->osr_kcd_descriptor;
		mach_vm_address_t addr;
		if (kcdata_get_memory_addr(kcd,
		    EXIT_REASON_USER_DESC, pathlen, &addr) == KERN_SUCCESS) {
			kcdata_memcpy(kcd, addr, path, pathlen);
			return r;
		}
	}
	os_reason_free(r);
	return OS_REASON_NULL;
}

static int vng_policy_flags;

  
                                                                
                                                             
                                                 
   
static int
vng_guard_violation(const struct vng_info *vgi,
    unsigned opval, vnode_t vp)
{
	int retval = 0;

	if (vng_policy_flags & kVNG_POLICY_EPERM) {
		                        
		retval = EPERM;
	}

	if (vng_policy_flags & (kVNG_POLICY_LOGMSG | kVNG_POLICY_UPRINTMSG)) {
		                   
		const char *op;
		switch (opval) {
		case VNG_RENAME_FROM:
			op = "rename-from";
			break;
		case VNG_RENAME_TO:
			op = "rename-to";
			break;
		case VNG_UNLINK:
			op = "unlink";
			break;
		case VNG_LINK:
			op = "link";
			break;
		case VNG_EXCHDATA:
			op = "exchdata";
			break;
		case VNG_WRITE_OTHER:
			op = "write";
			break;
		case VNG_TRUNC_OTHER:
			op = "truncate";
			break;
		default:
			op = "(unknown)";
			break;
		}

		const char *nm = vnode_getname(vp);
		proc_t p = current_proc();
		const struct vng_owner *vgo;
		TAILQ_FOREACH(vgo, &vgi->vgi_owners, vgo_link) {
			const char fmt[] =
			    "%s[%d]: %s%s: '%s' guarded by %s[%d] (0x%llx)\n";

			if (vng_policy_flags & kVNG_POLICY_LOGMSG) {
				printf(fmt,
				    proc_name_address(p), proc_pid(p), op,
				    0 != retval ? " denied" : "",
				    NULL != nm ? nm : "(unknown)",
				    proc_name_address(vgo->vgo_p),
				    proc_pid(vgo->vgo_p), vgi->vgi_guard);
			}
			if (vng_policy_flags & kVNG_POLICY_UPRINTMSG) {
				uprintf(fmt,
				    proc_name_address(p), proc_pid(p), op,
				    0 != retval ? " denied" : "",
				    NULL != nm ? nm : "(unknown)",
				    proc_name_address(vgo->vgo_p),
				    proc_pid(vgo->vgo_p), vgi->vgi_guard);
			}
		}
		if (NULL != nm) {
			vnode_putname(nm);
		}
	}

	if (vng_policy_flags & (kVNG_POLICY_EXC | kVNG_POLICY_EXC_CORPSE)) {
		                         
		const struct vng_owner *vgo = TAILQ_FIRST(&vgi->vgi_owners);
		pid_t pid = vgo ? proc_pid(vgo->vgo_p) : 0;
		mach_exception_code_t code;
		mach_exception_subcode_t subcode;

		code = 0;
		EXC_GUARD_ENCODE_TYPE(code, GUARD_TYPE_VN);
		EXC_GUARD_ENCODE_FLAVOR(code, opval);
		EXC_GUARD_ENCODE_TARGET(code, pid);
		subcode = vgi->vgi_guard;

		lck_rw_unlock_shared(&llock);

		if (vng_policy_flags & kVNG_POLICY_EXC_CORPSE) {
			char *path;
			int len = MAXPATHLEN;

			path = zalloc(ZV_NAMEI);

			os_reason_t r = NULL;
			if (NULL != path) {
				vn_getpath(vp, path, &len);
				if (*path && len) {
					r = vng_reason_from_pathname(path, len);
				}
			}
			task_violated_guard(code, subcode, r);                
			if (NULL != r) {
				os_reason_free(r);
			}

			zfree(ZV_NAMEI, path);
		} else {
			thread_t t = current_thread();
			thread_guard_violation(t, code, subcode, TRUE);
		}

		lck_rw_lock_shared(&llock);
	} else if (vng_policy_flags & kVNG_POLICY_SIGKILL) {
		proc_t p = current_proc();
		psignal(p, SIGKILL);
	}

	return retval;
}

  
                                                  
  
                                                                   
   
void
vn_guard_ast(thread_t __unused t,
    mach_exception_data_type_t code, mach_exception_data_type_t subcode)
{
	task_exception_notify(EXC_GUARD, code, subcode);
	proc_t p = current_proc();
	psignal(p, SIGKILL);
}

  
                  
   

static int
vng_vnode_check_rename(kauth_cred_t __unused cred,
    struct vnode *__unused dvp, struct label *__unused dlabel,
    struct vnode *vp, struct label *label,
    struct componentname *__unused cnp,
    struct vnode *__unused tdvp, struct label *__unused tdlabel,
    struct vnode *tvp, struct label *tlabel,
    struct componentname *__unused tcnp)
{
	int error = 0;
	if (NULL != label || NULL != tlabel) {
		lck_rw_lock_shared(&llock);
		const struct vng_info *vgi =
		    vng_lbl_get_withattr(label, VNG_RENAME_FROM);
		if (NULL != vgi) {
			error = vng_guard_violation(vgi, VNG_RENAME_FROM, vp);
		}
		if (0 == error) {
			vgi = vng_lbl_get_withattr(tlabel, VNG_RENAME_TO);
			if (NULL != vgi) {
				error = vng_guard_violation(vgi,
				    VNG_RENAME_TO, tvp);
			}
		}
		lck_rw_unlock_shared(&llock);
	}
	return error;
}

static int
vng_vnode_check_link(kauth_cred_t __unused cred,
    struct vnode *__unused dvp, struct label *__unused dlabel,
    struct vnode *vp, struct label *label, struct componentname *__unused cnp)
{
	int error = 0;
	if (NULL != label) {
		lck_rw_lock_shared(&llock);
		const struct vng_info *vgi =
		    vng_lbl_get_withattr(label, VNG_LINK);
		if (vgi) {
			error = vng_guard_violation(vgi, VNG_LINK, vp);
		}
		lck_rw_unlock_shared(&llock);
	}
	return error;
}

static int
vng_vnode_check_unlink(kauth_cred_t __unused cred,
    struct vnode *__unused dvp, struct label *__unused dlabel,
    struct vnode *vp, struct label *label, struct componentname *__unused cnp)
{
	int error = 0;
	if (NULL != label) {
		lck_rw_lock_shared(&llock);
		const struct vng_info *vgi =
		    vng_lbl_get_withattr(label, VNG_UNLINK);
		if (vgi) {
			error = vng_guard_violation(vgi, VNG_UNLINK, vp);
		}
		lck_rw_unlock_shared(&llock);
	}
	return error;
}

  
                                                                  
   
static int
vng_vnode_check_write(kauth_cred_t __unused actv_cred,
    kauth_cred_t __unused file_cred, struct vnode *vp, struct label *label)
{
	int error = 0;
	if (NULL != label) {
		lck_rw_lock_shared(&llock);
		const struct vng_info *vgi =
		    vng_lbl_get_withattr(label, VNG_WRITE_OTHER);
		if (vgi) {
			proc_t p = current_proc();
			const struct vng_owner *vgo;
			TAILQ_FOREACH(vgo, &vgi->vgi_owners, vgo_link) {
				if (vgo->vgo_p == p) {
					goto done;
				}
			}
			error = vng_guard_violation(vgi, VNG_WRITE_OTHER, vp);
		}
done:
		lck_rw_unlock_shared(&llock);
	}
	return error;
}

  
                                                                     
   
static int
vng_vnode_check_truncate(kauth_cred_t __unused actv_cred,
    kauth_cred_t __unused file_cred, struct vnode *vp,
    struct label *label)
{
	int error = 0;
	if (NULL != label) {
		lck_rw_lock_shared(&llock);
		const struct vng_info *vgi =
		    vng_lbl_get_withattr(label, VNG_TRUNC_OTHER);
		if (vgi) {
			proc_t p = current_proc();
			const struct vng_owner *vgo;
			TAILQ_FOREACH(vgo, &vgi->vgi_owners, vgo_link) {
				if (vgo->vgo_p == p) {
					goto done;
				}
			}
			error = vng_guard_violation(vgi, VNG_TRUNC_OTHER, vp);
		}
done:
		lck_rw_unlock_shared(&llock);
	}
	return error;
}

static int
vng_vnode_check_exchangedata(kauth_cred_t __unused cred,
    struct vnode *fvp, struct label *flabel,
    struct vnode *svp, struct label *slabel)
{
	int error = 0;
	if (NULL != flabel || NULL != slabel) {
		lck_rw_lock_shared(&llock);
		const struct vng_info *vgi =
		    vng_lbl_get_withattr(flabel, VNG_EXCHDATA);
		if (NULL != vgi) {
			error = vng_guard_violation(vgi, VNG_EXCHDATA, fvp);
		}
		if (0 == error) {
			vgi = vng_lbl_get_withattr(slabel, VNG_EXCHDATA);
			if (NULL != vgi) {
				error = vng_guard_violation(vgi,
				    VNG_EXCHDATA, svp);
			}
		}
		lck_rw_unlock_shared(&llock);
	}
	return error;
}

                                                                     

static int
vng_vnode_check_open(kauth_cred_t cred,
    struct vnode *vp, struct label *label, int acc_mode)
{
	if (0 == (acc_mode & O_TRUNC)) {
		return 0;
	}
	return vng_vnode_check_truncate(cred, NULL, vp, label);
}

  
                     
   

SECURITY_READ_ONLY_EARLY(static struct mac_policy_ops) vng_policy_ops = {
	.mpo_file_label_destroy = vng_file_label_destroy,

	.mpo_vnode_check_link = vng_vnode_check_link,
	.mpo_vnode_check_unlink = vng_vnode_check_unlink,
	.mpo_vnode_check_rename = vng_vnode_check_rename,
	.mpo_vnode_check_write = vng_vnode_check_write,
	.mpo_vnode_check_truncate = vng_vnode_check_truncate,
	.mpo_vnode_check_exchangedata = vng_vnode_check_exchangedata,
	.mpo_vnode_check_open = vng_vnode_check_open,

	.mpo_policy_syscall = vng_policy_syscall,
};

static const char *vng_labelnames[] = {
	"vnguard",
};

#define ACOUNT(arr) ((unsigned)(sizeof (arr) / sizeof (arr[0])))

SECURITY_READ_ONLY_LATE(static struct mac_policy_conf) vng_policy_conf = {
	.mpc_name = VNG_POLICY_NAME,
	.mpc_fullname = "Guarded vnode policy",
	.mpc_field_off = &label_slot,
	.mpc_labelnames = vng_labelnames,
	.mpc_labelname_count = ACOUNT(vng_labelnames),
	.mpc_ops = &vng_policy_ops,
	.mpc_loadtime_flags = 0,
	.mpc_runtime_flags = 0
};

SECURITY_READ_ONLY_LATE(static mac_policy_handle_t) vng_policy_handle;

void
vnguard_policy_init(void)
{
	if (0 == PE_i_can_has_debugger(NULL)) {
		return;
	}
	vng_policy_flags = kVNG_POLICY_LOGMSG |
	    kVNG_POLICY_EXC_CORPSE | kVNG_POLICY_UPRINTMSG;
	PE_parse_boot_argn("vnguard", &vng_policy_flags, sizeof(vng_policy_flags));
	if (vng_policy_flags) {
		mac_policy_register(&vng_policy_conf, &vng_policy_handle, NULL);
	}
}

#if DEBUG || DEVELOPMENT
#include <sys/sysctl.h>

SYSCTL_DECL(_kern_vnguard);
SYSCTL_NODE(_kern, OID_AUTO, vnguard, CTLFLAG_RW | CTLFLAG_LOCKED, 0, "vnguard");
SYSCTL_INT(_kern_vnguard, OID_AUTO, flags, CTLFLAG_RW | CTLFLAG_LOCKED,
    &vng_policy_flags, 0, "vnguard policy flags");
#endif

#endif                                    
