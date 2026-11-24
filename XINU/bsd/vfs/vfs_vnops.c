/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
 *	@(#)vfs_vnops.c	8.14 (Berkeley) 6/15/95
 *
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file_internal.h>
#include <sys/stat.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/mount_internal.h>
#include <sys/namei.h>
#include <sys/vnode_internal.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
                                                                      
#define ubc_setcred ubc_setcred_deprecated
#include <sys/ubc.h>
#undef ubc_setcred
int     ubc_setcred(struct vnode *, struct proc *);
#include <sys/conf.h>
#include <sys/disk.h>
#include <sys/fsevents.h>
#include <sys/kdebug.h>
#include <sys/xattr.h>
#include <sys/ubc_internal.h>
#include <sys/uio_internal.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>

#include <vm/vm_kern.h>
#include <vm/vm_map.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#include <IOKit/IOBSD.h>
#include <libkern/section_keywords.h>

static int vn_closefile(struct fileglob *fp, vfs_context_t ctx);
static int vn_ioctl(struct fileproc *fp, u_long com, caddr_t data,
    vfs_context_t ctx);
static int vn_read(struct fileproc *fp, struct uio *uio, int flags,
    vfs_context_t ctx);
static int vn_write(struct fileproc *fp, struct uio *uio, int flags,
    vfs_context_t ctx);
static int vn_select( struct fileproc *fp, int which, void * wql,
    vfs_context_t ctx);
static int vn_kqfilter(struct fileproc *fp, struct knote *kn,
    struct kevent_qos_s *kev);
static void filt_vndetach(struct knote *kn);
static int filt_vnode(struct knote *kn, long hint);
static int filt_vnode_common(struct knote *kn, struct kevent_qos_s *kev,
    vnode_t vp, long hint);
static int vn_open_auth_finish(vnode_t vp, int fmode, vfs_context_t ctx);

const struct fileops vnops = {
	.fo_type     = DTYPE_VNODE,
	.fo_read     = vn_read,
	.fo_write    = vn_write,
	.fo_ioctl    = vn_ioctl,
	.fo_select   = vn_select,
	.fo_close    = vn_closefile,
	.fo_drain    = fo_no_drain,
	.fo_kqfilter = vn_kqfilter,
};

static int filt_vntouch(struct knote *kn, struct kevent_qos_s *kev);
static int filt_vnprocess(struct knote *kn, struct kevent_qos_s*kev);

SECURITY_READ_ONLY_EARLY(struct  filterops) vnode_filtops = {
	.f_isfd = 1,
	.f_attach = NULL,
	.f_detach = filt_vndetach,
	.f_event = filt_vnode,
	.f_touch = filt_vntouch,
	.f_process = filt_vnprocess,
};

  
                                         
                                                                    
  
                                                                
   
int
vn_open(struct nameidata *ndp, int fmode, int cmode)
{
	return vn_open_modflags(ndp, &fmode, cmode);
}

int
vn_open_modflags(struct nameidata *ndp, int *fmodep, int cmode)
{
	int error;
	struct vnode_attr *vap;

	vap = kheap_alloc(KHEAP_TEMP, sizeof(struct vnode_attr), M_WAITOK);

	VATTR_INIT(vap);
	VATTR_SET(vap, va_mode, (mode_t)cmode);

	error = vn_open_auth(ndp, fmodep, vap);

	kheap_free(KHEAP_TEMP, vap, sizeof(struct vnode_attr));

	return error;
}

static int
vn_open_auth_finish(vnode_t vp, int fmode, vfs_context_t ctx)
{
	int error;

	if ((error = vnode_ref_ext(vp, fmode, 0)) != 0) {
		goto bad;
	}

	                                                    
                                                 
    
#if CONFIG_MACF
	mac_vnode_notify_open(ctx, vp, fmode);
#endif
	kauth_authorize_fileop(vfs_context_ucred(ctx), KAUTH_FILEOP_OPEN,
	    (uintptr_t)vp, 0);

	return 0;

bad:
	return error;
}

  
                                                                              
                                       
   
static int
vn_open_auth_do_create(struct nameidata *ndp, struct vnode_attr *vap, int fmode, boolean_t *did_create, boolean_t *did_open, vfs_context_t ctx)
{
	uint32_t status = 0;
	vnode_t dvp = ndp->ni_dvp;
	int batched;
	int error;
	vnode_t vp;

	batched = vnode_compound_open_available(ndp->ni_dvp);
	*did_open = FALSE;

	VATTR_SET(vap, va_type, VREG);
	if (fmode & O_EXCL) {
		vap->va_vaflags |= VA_EXCLUSIVE;
	}

#if NAMEDRSRCFORK
	if (ndp->ni_cnd.cn_flags & CN_WANTSRSRCFORK) {
		if ((error = vn_authorize_create(dvp, &ndp->ni_cnd, vap, ctx, NULL)) != 0) {
			goto out;
		}
		if ((error = vnode_makenamedstream(dvp, &ndp->ni_vp, XATTR_RESOURCEFORK_NAME, 0, ctx)) != 0) {
			goto out;
		}
		*did_create = TRUE;
	} else {
#endif
	if (!batched) {
		if ((error = vn_authorize_create(dvp, &ndp->ni_cnd, vap, ctx, NULL)) != 0) {
			goto out;
		}
	}

	error = vn_create(dvp, &ndp->ni_vp, ndp, vap, VN_CREATE_DOOPEN, fmode, &status, ctx);
	if (error != 0) {
		if (batched) {
			*did_create = (status & COMPOUND_OPEN_STATUS_DID_CREATE) ? TRUE : FALSE;
		} else {
			*did_create = FALSE;
		}

		if (error == EKEEPLOOKING) {
			if (*did_create) {
				panic("EKEEPLOOKING, but we did a create?");
			}
			if (!batched) {
				panic("EKEEPLOOKING from filesystem that doesn't support compound vnops?");
			}
			if ((ndp->ni_flag & NAMEI_CONTLOOKUP) == 0) {
				panic("EKEEPLOOKING, but continue flag not set?");
			}

			  
                                                                     
      
			return error;
		}
	} else {
		if (batched) {
			*did_create = (status & COMPOUND_OPEN_STATUS_DID_CREATE) ? 1 : 0;
			*did_open = TRUE;
		} else {
			*did_create = TRUE;
		}
	}
#if NAMEDRSRCFORK
}
#endif

	vp = ndp->ni_vp;

	if (*did_create) {
		int     update_flags = 0;

		                                                     
		if (vp->v_name == NULL) {
			update_flags |= VNODE_UPDATE_NAME;
		}
		if (vp->v_parent == NULLVP) {
			update_flags |= VNODE_UPDATE_PARENT;
		}

		if (update_flags) {
			vnode_update_identity(vp, dvp, ndp->ni_cnd.cn_nameptr, ndp->ni_cnd.cn_namelen, ndp->ni_cnd.cn_hash, update_flags);
		}

		vnode_put(dvp);
		ndp->ni_dvp = NULLVP;

#if CONFIG_FSE
		if (need_fsevent(FSE_CREATE_FILE, vp)) {
			add_fsevent(FSE_CREATE_FILE, ctx,
			    FSE_ARG_VNODE, vp,
			    FSE_ARG_DONE);
		}
#endif
	}
out:
	if (ndp->ni_dvp != NULLVP) {
		vnode_put(dvp);
		ndp->ni_dvp = NULLVP;
	}

	return error;
}

  
                                                                            
                                                       
   
#define RETRY_NO_YIELD_COUNT    5

  
                                                                          
                                                                           
                                                                          
                         
  
                                                         
           
                                                  
                                          
                                          
                                         
  
                                                        
                                           
                                            
  
                       
                    
  
                                                                      
  
                                                               
                                                                
                                                      
  
                                                               
                                                                
                                           
  
                                                                  
                                                                
                                                              
                 
  
                                                           
                                                          
                                                                   
                                                               
  
                                                                     
  
                                                                       
                                
   
int
vn_open_auth(struct nameidata *ndp, int *fmodep, struct vnode_attr *vap)
{
	struct vnode *vp;
	struct vnode *dvp;
	vfs_context_t ctx = ndp->ni_cnd.cn_context;
	int error;
	int fmode;
	uint32_t origcnflags;
	boolean_t did_create;
	boolean_t did_open;
	boolean_t need_vnop_open;
	boolean_t batched;
	boolean_t ref_failed;
	int nretries = 0;

again:
	vp = NULL;
	dvp = NULL;
	batched = FALSE;
	did_create = FALSE;
	need_vnop_open = TRUE;
	ref_failed = FALSE;
	fmode = *fmodep;
	origcnflags = ndp->ni_cnd.cn_flags;

	                                                       
	if (VATTR_IS_ACTIVE(vap, va_dataprotect_flags)
	    && ISSET(vap->va_dataprotect_flags, VA_DP_RAWENCRYPTED)) {
		fmode |= FENCRYPTED;
	}

	if ((fmode & O_NOFOLLOW_ANY) && (fmode & (O_SYMLINK | O_NOFOLLOW))) {
		error = EINVAL;
		goto out;
	}

	  
           
    
	if (fmode & O_CREAT) {
		if ((fmode & O_DIRECTORY)) {
			error = EINVAL;
			goto out;
		}
		ndp->ni_cnd.cn_nameiop = CREATE;
#if CONFIG_TRIGGERS
		ndp->ni_op = OP_LINK;
#endif
		                                                       
		ndp->ni_cnd.cn_flags &= (USEDVP | NOCROSSMOUNT);
		ndp->ni_cnd.cn_flags |= LOCKPARENT | LOCKLEAF | AUDITVNPATH1;
		ndp->ni_flag = NAMEI_COMPOUNDOPEN;
#if NAMEDRSRCFORK
		                                                
		ndp->ni_cnd.cn_flags |= CN_ALLOWRSRCFORK;
#endif
		if ((fmode & O_EXCL) == 0 && (fmode & O_NOFOLLOW) == 0 && (origcnflags & FOLLOW) != 0) {
			ndp->ni_cnd.cn_flags |= FOLLOW;
		}
		if (fmode & O_NOFOLLOW_ANY) {
			                                                      
			ndp->ni_flag |= NAMEI_NOFOLLOW_ANY;
		}

continue_create_lookup:
		if ((error = namei(ndp))) {
			goto out;
		}

		dvp = ndp->ni_dvp;
		vp = ndp->ni_vp;

		batched = vnode_compound_open_available(dvp);

		                       
		if (vp == NULL) {
			                                         
			if (vap == NULL) {
				vnode_put(dvp);
				error = EINVAL;
				goto out;
			}
			  
                                                                        
                                                                          
                                                           
      
			error = vn_open_auth_do_create(ndp, vap, fmode, &did_create, &did_open, ctx);

			dvp = ndp->ni_dvp;
			vp = ndp->ni_vp;

			  
                                                                      
                                                        
      
			if (error == EKEEPLOOKING) {
				if (!batched) {
					panic("EKEEPLOOKING from a filesystem that doesn't support compound VNOPs?");
				}
				goto continue_create_lookup;
			}

			nameidone(ndp);
			if (dvp) {
				panic("Shouldn't have a dvp here.");
			}

			if (error) {
				  
                               
       
				if ((error == EEXIST) && !(fmode & O_EXCL)) {
					if (vp) {
						vnode_put(vp);
					}
					goto again;
				}
				goto bad;
			}

			need_vnop_open = !did_open;
		} else {
			if (fmode & O_EXCL) {
				error = EEXIST;
			}

			  
                                                      
                                                              
                                                             
                        
      
			if (error == 0 && batched && (vnode_mount(dvp) == vnode_mount(vp))) {
				error = VNOP_COMPOUND_OPEN(dvp, &ndp->ni_vp, ndp, 0, fmode, NULL, NULL, ctx);

				if (error == 0) {
					vp = ndp->ni_vp;
					need_vnop_open = FALSE;
				} else if (error == EKEEPLOOKING) {
					if ((ndp->ni_flag & NAMEI_CONTLOOKUP) == 0) {
						panic("EKEEPLOOKING, but continue flag not set?");
					}
					goto continue_create_lookup;
				}
			}
			nameidone(ndp);
			vnode_put(dvp);
			ndp->ni_dvp = NULLVP;

			if (error) {
				goto bad;
			}

			fmode &= ~O_CREAT;

			                  
		}
	} else {
		  
                
     
		ndp->ni_cnd.cn_nameiop = LOOKUP;
		                                                       
		ndp->ni_cnd.cn_flags &= (USEDVP | NOCROSSMOUNT);
		ndp->ni_cnd.cn_flags |= FOLLOW | LOCKLEAF | AUDITVNPATH1 | WANTPARENT;
#if NAMEDRSRCFORK
		                                                
		ndp->ni_cnd.cn_flags |= CN_ALLOWRSRCFORK;
#endif
		if (fmode & FENCRYPTED) {
			ndp->ni_cnd.cn_flags |= CN_RAW_ENCRYPTED | CN_SKIPNAMECACHE;
		}
		ndp->ni_flag = NAMEI_COMPOUNDOPEN;

		                                         
		if (fmode & O_NOFOLLOW || fmode & O_SYMLINK || (origcnflags & FOLLOW) == 0) {
			ndp->ni_cnd.cn_flags &= ~FOLLOW;
		}
		if (fmode & O_NOFOLLOW_ANY) {
			                                                      
			ndp->ni_flag |= NAMEI_NOFOLLOW_ANY;
		}

		                                                                               
		do {
			if ((error = namei(ndp))) {
				goto out;
			}
			vp = ndp->ni_vp;
			dvp = ndp->ni_dvp;

			                                   
			batched = vnode_compound_open_available(dvp);
			if (batched && ((vp == NULLVP) || (vnode_mount(dvp) == vnode_mount(vp)))) {
				error = VNOP_COMPOUND_OPEN(dvp, &ndp->ni_vp, ndp, 0, fmode, NULL, NULL, ctx);
				vp = ndp->ni_vp;
				if (error == 0) {
					need_vnop_open = FALSE;
				} else if (error == EKEEPLOOKING) {
					if ((ndp->ni_flag & NAMEI_CONTLOOKUP) == 0) {
						panic("EKEEPLOOKING, but continue flag not set?");
					}
				}
			}
		} while (error == EKEEPLOOKING);

		nameidone(ndp);
		vnode_put(dvp);
		ndp->ni_dvp = NULLVP;

		if (error) {
			goto bad;
		}
	}

	  
                                                                 
                               
    
	if (ndp->ni_dvp != NULLVP) {
		panic("Haven't cleaned up adequately in vn_open_auth()");
	}

	  
                                                                                
                                                                              
                                                                                       
    
	if (need_vnop_open) {
		if (batched && !vnode_isvroot(vp) && !vnode_isnamedstream(vp)) {
			panic("Why am I trying to use VNOP_OPEN() on anything other than the root or a named stream?");
		}

		if (!did_create) {
			error = vn_authorize_open_existing(vp, &ndp->ni_cnd, fmode, ctx, NULL);
			if (error) {
				goto bad;
			}
		}

		if (VATTR_IS_ACTIVE(vap, va_dataprotect_flags)
		    && ISSET(vap->va_dataprotect_flags, VA_DP_RAWUNENCRYPTED)) {
			                                                                        
			boolean_t entitled = FALSE;
#if !SECURE_KERNEL
			entitled = IOTaskHasEntitlement(current_task(), "com.apple.private.security.file-unencrypt-access");
#endif
			if (!entitled) {
				error = EPERM;
				goto bad;
			}
			fmode |= FUNENCRYPTED;
		}

		error = VNOP_OPEN(vp, fmode, ctx);
		if (error) {
			goto bad;
		}
		need_vnop_open = FALSE;
	}

	                                                          
	                                                               
	                                                              
	                                                               
	                                                        
	                                                             
	                                                      
	  
	if (!did_create && (vp->v_flag & VOPENEVT) && (current_proc()->p_flag & P_CHECKOPENEVT)) {
		fmode |= O_EVTONLY;
	}

	  
                        
    
	error = vn_open_auth_finish(vp, fmode, ctx);
	if (error) {
		ref_failed = TRUE;
		goto bad;
	}

	                                                              
	if (batched || did_create) {
		fmode &= ~O_TRUNC;
	}

	*fmodep = fmode;
	return 0;

bad:
	                                                     
	if (!need_vnop_open) {
		VNOP_CLOSE(vp, fmode, ctx);
	}

	ndp->ni_vp = NULL;
	if (vp) {
#if NAMEDRSRCFORK
		                                                                       
		if ((vnode_isnamedstream(vp)) &&
		    (vp->v_parent != NULLVP) &&
		    (vnode_isshadow(vp))) {
			vnode_recycle(vp);
		}
#endif
		vnode_put(vp);
		  
                                                     
                                                     
                      
    
                                                                     
     
		if (((error == ENOENT) && (*fmodep & O_CREAT)) || (error == EREDRIVEOPEN) || ref_failed) {
			  
                                                         
                                                         
                                                       
                                                     
                                                        
                                                      
                                                      
                                                        
                                                          
     
                                                    
                                                         
                                             
      
			nretries += 1;
			if (nretries > RETRY_NO_YIELD_COUNT) {
				                                       
				tsleep(&nretries, PVFS, "vn_open_auth_retry",
				    MIN((nretries * (hz / 100)), hz));
			}
			goto again;
		}
	}

out:
	return error;
}

#if vn_access_DEPRECATED
  
                                                                           
                                                                      
                                                                     
  
                                                                       
                                                                              
                                                
   
int
vn_access(vnode_t vp, int mode, vfs_context_t context)
{
	kauth_action_t  action;

	action = 0;
	if (mode & VREAD) {
		action |= KAUTH_VNODE_READ_DATA;
	}
	if (mode & VWRITE) {
		action |= KAUTH_VNODE_WRITE_DATA;
	}
	if (mode & VEXEC) {
		action |= KAUTH_VNODE_EXECUTE;
	}

	return vnode_authorize(vp, NULL, action, context);
}
#endif                            

  
                   
   
int
vn_close(struct vnode *vp, int flags, vfs_context_t ctx)
{
	int error;
	int flusherror = 0;

#if NAMEDRSRCFORK
	                                                         
	if ((vp->v_flag & VISNAMEDSTREAM) &&
	    (vp->v_parent != NULLVP) &&
	    vnode_isshadow(vp)) {
		if (flags & FWASWRITTEN) {
			flusherror = vnode_flushnamedstream(vp->v_parent, vp, ctx);
		}
	}
#endif
	  
                                                                    
                                                                      
                                                                      
                                                                
                                                                        
                                                                       
        
    
	if (vnode_isspec(vp)) {
		(void)vnode_rele_ext(vp, flags, 0);
	}

	  
                                                             
                                                              
                                                                
                                                              
                                                             
                                                             
                    
    
	if (vp->v_tag == VT_HFS && (flags & FWRITE) && vp->v_writecount == 1) {
		VNOP_FSYNC(vp, MNT_NOWAIT, ctx);
	}

	error = VNOP_CLOSE(vp, flags, ctx);

#if CONFIG_FSE
	if (flags & FWASWRITTEN) {
		if (need_fsevent(FSE_CONTENT_MODIFIED, vp)) {
			add_fsevent(FSE_CONTENT_MODIFIED, ctx,
			    FSE_ARG_VNODE, vp,
			    FSE_ARG_DONE);
		}
	}
#endif

	if (!vnode_isspec(vp)) {
		(void)vnode_rele_ext(vp, flags, 0);
	}

	if (flusherror) {
		error = flusherror;
	}
	return error;
}

static int
vn_read_swapfile(
	struct vnode    *vp,
	uio_t           uio)
{
	int     error;
	off_t   swap_count, this_count;
	off_t   file_end, read_end;
	off_t   prev_resid;
	char    *my_swap_page;

	  
                                                 
    

	my_swap_page = NULL;
	error = 0;
	swap_count = uio_resid(uio);

	file_end = ubc_getsize(vp);
	read_end = uio->uio_offset + uio_resid(uio);
	if (uio->uio_offset >= file_end) {
		                                                   
		swap_count = 0;
	} else if (read_end > file_end) {
		                                                      
		swap_count -= (read_end - file_end);
	}

	while (swap_count > 0) {
		if (my_swap_page == NULL) {
			my_swap_page = kheap_alloc(KHEAP_TEMP, PAGE_SIZE, Z_WAITOK | Z_ZERO);
			                                                    
			my_swap_page[PAGE_SIZE - 1] = '\n';
		}
		this_count = swap_count;
		if (this_count > PAGE_SIZE) {
			this_count = PAGE_SIZE;
		}

		prev_resid = uio_resid(uio);
		error = uiomove((caddr_t) my_swap_page,
		    (int)this_count,
		    uio);
		if (error) {
			break;
		}
		swap_count -= (prev_resid - uio_resid(uio));
	}
	kheap_free(KHEAP_TEMP, my_swap_page, PAGE_SIZE);

	return error;
}
  
                                                             
   
int
vn_rdwr(
	enum uio_rw rw,
	struct vnode *vp,
	caddr_t base,
	int len,
	off_t offset,
	enum uio_seg segflg,
	int ioflg,
	kauth_cred_t cred,
	int *aresid,
	proc_t p)
{
	int64_t resid;
	int result;

	if (len < 0) {
		return EINVAL;
	}

	result = vn_rdwr_64(rw,
	    vp,
	    (uint64_t)(uintptr_t)base,
	    (int64_t)len,
	    offset,
	    segflg,
	    ioflg,
	    cred,
	    &resid,
	    p);

	                                                               
	if (aresid != NULL) {
		*aresid = (int)resid;
	}

	return result;
}


int
vn_rdwr_64(
	enum uio_rw rw,
	struct vnode *vp,
	uint64_t base,
	int64_t len,
	off_t offset,
	enum uio_seg segflg,
	int ioflg,
	kauth_cred_t cred,
	int64_t *aresid,
	proc_t p)
{
	uio_t auio;
	int spacetype;
	struct vfs_context context;
	int error = 0;
	char uio_buf[UIO_SIZEOF(1)];

	context.vc_thread = current_thread();
	context.vc_ucred = cred;

	if (UIO_SEG_IS_USER_SPACE(segflg)) {
		spacetype = proc_is64bit(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	} else {
		spacetype = UIO_SYSSPACE;
	}

	if (len < 0) {
		return EINVAL;
	}

	auio = uio_createwithbuffer(1, offset, spacetype, rw,
	    &uio_buf[0], sizeof(uio_buf));
	uio_addiov(auio, CAST_USER_ADDR_T(base), (user_size_t)len);

#if CONFIG_MACF
	         
                                         
                                                        
    
	if ((ioflg & IO_NOAUTH) == 0) {
		                               
		if (rw == UIO_READ) {
			error = mac_vnode_check_read(&context, cred, vp);
		} else {
			error = mac_vnode_check_write(&context, cred, vp);
		}
	}
#endif

	if (error == 0) {
		if (rw == UIO_READ) {
			if (vnode_isswap(vp) && ((ioflg & IO_SWAP_DISPATCH) == 0)) {
				error = vn_read_swapfile(vp, auio);
			} else {
				error = VNOP_READ(vp, auio, ioflg, &context);
			}
		} else {
			error = VNOP_WRITE(vp, auio, ioflg, &context);
		}
	}

	if (aresid) {
		*aresid = uio_resid(auio);
		assert(*aresid <= len);
	} else if (uio_resid(auio) && error == 0) {
		error = EIO;
	}
	return error;
}

static inline void
vn_offset_lock(struct fileglob *fg)
{
	lck_mtx_lock_spin(&fg->fg_lock);
	while (fg->fg_lflags & FG_OFF_LOCKED) {
		fg->fg_lflags |= FG_OFF_LOCKWANT;
		msleep(&fg->fg_lflags, &fg->fg_lock, PVFS | PSPIN,
		    "fg_offset_lock_wait", 0);
	}
	fg->fg_lflags |= FG_OFF_LOCKED;
	lck_mtx_unlock(&fg->fg_lock);
}

static inline void
vn_offset_unlock(struct fileglob *fg)
{
	int lock_wanted = 0;

	lck_mtx_lock_spin(&fg->fg_lock);
	if (fg->fg_lflags & FG_OFF_LOCKWANT) {
		lock_wanted = 1;
	}
	fg->fg_lflags &= ~(FG_OFF_LOCKED | FG_OFF_LOCKWANT);
	lck_mtx_unlock(&fg->fg_lock);
	if (lock_wanted) {
		wakeup(&fg->fg_lflags);
	}
}

  
                                 
   
static int
vn_read(struct fileproc *fp, struct uio *uio, int flags, vfs_context_t ctx)
{
	struct vnode *vp;
	int error;
	int ioflag;
	off_t read_offset;
	user_ssize_t  read_len;
	user_ssize_t  adjusted_read_len;
	user_ssize_t  clippedsize;
	bool offset_locked;

	read_len = uio_resid(uio);
	if (read_len < 0 || read_len > INT_MAX) {
		return EINVAL;
	}
	adjusted_read_len = read_len;
	clippedsize = 0;
	offset_locked = false;

	vp = (struct vnode *)fp->fp_glob->fg_data;
	if ((error = vnode_getwithref(vp))) {
		return error;
	}

#if CONFIG_MACF
	error = mac_vnode_check_read(ctx, vfs_context_ucred(ctx), vp);
	if (error) {
		(void)vnode_put(vp);
		return error;
	}
#endif

	                                                                              
	ioflag = IO_SYSCALL_DISPATCH;

	if (fp->fp_glob->fg_flag & FNONBLOCK) {
		ioflag |= IO_NDELAY;
	}
	if ((fp->fp_glob->fg_flag & FNOCACHE) || vnode_isnocache(vp)) {
		ioflag |= IO_NOCACHE;
	}
	if (fp->fp_glob->fg_flag & FENCRYPTED) {
		ioflag |= IO_ENCRYPTED;
	}
	if (fp->fp_glob->fg_flag & FUNENCRYPTED) {
		ioflag |= IO_SKIP_ENCRYPTION;
	}
	if (fp->fp_glob->fg_flag & O_EVTONLY) {
		ioflag |= IO_EVTONLY;
	}
	if (fp->fp_glob->fg_flag & FNORDAHEAD) {
		ioflag |= IO_RAOFF;
	}

	if ((flags & FOF_OFFSET) == 0) {
		if ((vnode_vtype(vp) == VREG) && !vnode_isswap(vp)) {
			vn_offset_lock(fp->fp_glob);
			offset_locked = true;
		}
		read_offset = fp->fp_glob->fg_offset;
		uio_setoffset(uio, read_offset);
	} else {
		read_offset = uio_offset(uio);
		                                                          
		if ((read_offset < 0) && (vnode_vtype(vp) != VCHR)) {
			error = EINVAL;
			goto error_out;
		}
	}

	if (read_offset == INT64_MAX) {
		                         
		error = 0;
		goto error_out;
	}

	  
                                                                  
                                      
    
	if ((read_offset >= 0) && (INT64_MAX - read_offset) < read_len) {
		  
                                                                      
                                                                    
                                                                   
                                                                 
     
		adjusted_read_len = (user_ssize_t)(INT64_MAX - read_offset);
	}

	if (adjusted_read_len < read_len) {
		uio_setresid(uio, adjusted_read_len);
		clippedsize = read_len - adjusted_read_len;
	}

	if (vnode_isswap(vp) && !(IO_SKIP_ENCRYPTION & ioflag)) {
		                                 
		error = vn_read_swapfile(vp, uio);
	} else {
		error = VNOP_READ(vp, uio, ioflag, ctx);
	}

	if (clippedsize) {
		uio_setresid(uio, (uio_resid(uio) + clippedsize));
	}

	if ((flags & FOF_OFFSET) == 0) {
		fp->fp_glob->fg_offset += read_len - uio_resid(uio);
	}

error_out:
	if (offset_locked) {
		vn_offset_unlock(fp->fp_glob);
		offset_locked = false;
	}

	(void)vnode_put(vp);
	return error;
}


  
                                  
   
static int
vn_write(struct fileproc *fp, struct uio *uio, int flags, vfs_context_t ctx)
{
	struct vnode *vp;
	int error, ioflag;
	off_t write_offset;
	off_t write_end_offset;
	user_ssize_t write_len;
	user_ssize_t adjusted_write_len;
	user_ssize_t clippedsize;
	bool offset_locked;
	proc_t p = vfs_context_proc(ctx);
	rlim_t rlim_cur_fsize = p ? proc_limitgetcur(p, RLIMIT_FSIZE, TRUE) : 0;

	write_len = uio_resid(uio);
	if (write_len < 0 || write_len > INT_MAX) {
		return EINVAL;
	}
	adjusted_write_len = write_len;
	clippedsize = 0;
	offset_locked = false;

	vp = (struct vnode *)fp->fp_glob->fg_data;
	if ((error = vnode_getwithref(vp))) {
		return error;
	}

#if CONFIG_MACF
	error = mac_vnode_check_write(ctx, vfs_context_ucred(ctx), vp);
	if (error) {
		(void)vnode_put(vp);
		return error;
	}
#endif

	  
                                                                          
                      
    
	ioflag = (IO_UNIT | IO_SYSCALL_DISPATCH);

	if (vp->v_type == VREG && (fp->fp_glob->fg_flag & O_APPEND)) {
		ioflag |= IO_APPEND;
	}
	if (fp->fp_glob->fg_flag & FNONBLOCK) {
		ioflag |= IO_NDELAY;
	}
	if ((fp->fp_glob->fg_flag & FNOCACHE) || vnode_isnocache(vp)) {
		ioflag |= IO_NOCACHE;
	}
	if (fp->fp_glob->fg_flag & FNODIRECT) {
		ioflag |= IO_NODIRECT;
	}
	if (fp->fp_glob->fg_flag & FSINGLE_WRITER) {
		ioflag |= IO_SINGLE_WRITER;
	}
	if (fp->fp_glob->fg_flag & O_EVTONLY) {
		ioflag |= IO_EVTONLY;
	}

	  
                                                                 
   
                                                                   
                                                                    
                                                                  
    
	if ((fp->fp_glob->fg_flag & (O_FSYNC | O_DSYNC)) ||
	    (vp->v_mount && (vp->v_mount->mnt_flag & MNT_SYNCHRONOUS))) {
		ioflag |= IO_SYNC;
	}

	if ((flags & FOF_OFFSET) == 0) {
		if ((vnode_vtype(vp) == VREG) && !vnode_isswap(vp)) {
			vn_offset_lock(fp->fp_glob);
			offset_locked = true;
		}
		write_offset = fp->fp_glob->fg_offset;
		uio_setoffset(uio, write_offset);
	} else {
		                                           
		ioflag &= ~IO_APPEND;
		write_offset = uio_offset(uio);
		                                                          
		if ((write_offset < 0) && (vnode_vtype(vp) != VCHR)) {
			error = EINVAL;
			goto error_out;
		}
	}

	if (write_offset == INT64_MAX) {
		                             
		error = EFBIG;
		goto error_out;
	}

	  
                                                              
                                                             
   
                                                                         
             
                                                                 
                                       
   
                                                                          
                                                             
    

	  
                                                       
                                                  
    
	if ((write_offset >= 0) && (INT64_MAX - write_offset) < write_len) {
		  
                                                                       
                                                                    
                                                                    
                                                                 
     
		adjusted_write_len = (user_ssize_t)(INT64_MAX - write_offset);
	}

	                                                    
	write_end_offset = write_offset + adjusted_write_len;

	if (p && (vp->v_type == VREG) &&
	    (rlim_cur_fsize != RLIM_INFINITY) &&
	    (rlim_cur_fsize <= INT64_MAX) &&
	    (write_end_offset > (off_t)rlim_cur_fsize)) {
		  
                                                            
                                                              
                                                                
                                                            
                                       
     
		if (write_offset >= (off_t)rlim_cur_fsize) {
			  
                                                                       
                                                                  
                                                         
     
                                        
      
			psignal(p, SIGXFSZ);
			error = EFBIG;
			goto error_out;
		}

		  
                                                                     
                                                                  
                                                     
                                           
     
		adjusted_write_len =  (user_ssize_t)((off_t)rlim_cur_fsize - write_offset);
		assert((adjusted_write_len > 0) && (adjusted_write_len < write_len));
	}

	if (adjusted_write_len < write_len) {
		uio_setresid(uio, adjusted_write_len);
		clippedsize = write_len - adjusted_write_len;
	}

	error = VNOP_WRITE(vp, uio, ioflag, ctx);

	  
                                                                  
                                               
                                                                  
                                                       
    
	if (clippedsize) {
		uio_setresid(uio, (uio_resid(uio) + clippedsize));
	}

	if ((flags & FOF_OFFSET) == 0) {
		if (ioflag & IO_APPEND) {
			fp->fp_glob->fg_offset = uio_offset(uio);
		} else {
			fp->fp_glob->fg_offset += (write_len - uio_resid(uio));
		}
		if (offset_locked) {
			vn_offset_unlock(fp->fp_glob);
			offset_locked = false;
		}
	}

	  
                                            
    
	if ((error == 0) && (vp->v_tag == VT_NFS) && (UBCINFOEXISTS(vp))) {
		  
                                                               
                                                            
                                                             
                                                             
                                                              
                               
     
		if ((flags & FOF_PCRED) == 0) {
			ubc_setthreadcred(vp, p, current_thread());
		} else {
			ubc_setcred(vp, p);
		}
	}
	(void)vnode_put(vp);
	return error;

error_out:
	if (offset_locked) {
		vn_offset_unlock(fp->fp_glob);
	}
	(void)vnode_put(vp);
	return error;
}

  
                                 
  
                       
         
          
                    
   
int
vn_stat_noauth(struct vnode *vp, void *sbptr, kauth_filesec_t *xsec, int isstat64,
    int needsrealdev, vfs_context_t ctx, struct ucred *file_cred)
{
	struct vnode_attr va;
	int error;
	u_short mode;
	kauth_filesec_t fsec;
	struct stat *sb = (struct stat *)0;                                                    
	struct stat64 * sb64 = (struct stat64 *)0;                                                 

	if (isstat64 != 0) {
		sb64 = (struct stat64 *)sbptr;
	} else {
		sb = (struct stat *)sbptr;
	}
	memset(&va, 0, sizeof(va));
	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_fsid);
	VATTR_WANTED(&va, va_fileid);
	VATTR_WANTED(&va, va_mode);
	VATTR_WANTED(&va, va_type);
	VATTR_WANTED(&va, va_nlink);
	VATTR_WANTED(&va, va_uid);
	VATTR_WANTED(&va, va_gid);
	VATTR_WANTED(&va, va_rdev);
	VATTR_WANTED(&va, va_data_size);
	VATTR_WANTED(&va, va_access_time);
	VATTR_WANTED(&va, va_modify_time);
	VATTR_WANTED(&va, va_change_time);
	VATTR_WANTED(&va, va_create_time);
	VATTR_WANTED(&va, va_flags);
	VATTR_WANTED(&va, va_gen);
	VATTR_WANTED(&va, va_iosize);
	                                                                               
	VATTR_WANTED(&va, va_total_alloc);
	if (xsec != NULL) {
		VATTR_WANTED(&va, va_uuuid);
		VATTR_WANTED(&va, va_guuid);
		VATTR_WANTED(&va, va_acl);
	}
	if (needsrealdev) {
		va.va_vaflags = VA_REALFSID;
	}
	error = vnode_getattr(vp, &va, ctx);
	if (error) {
		goto out;
	}
#if CONFIG_MACF
	  
                                                                
                                                                     
                                                                      
                                                                        
                                                                     
                                                 
    
	error = mac_vnode_check_getattr(ctx, file_cred, vp, &va);
	if (error) {
		goto out;
	}
#endif
	  
                         
    
	if (isstat64 != 0) {
		sb64->st_dev = va.va_fsid;
		sb64->st_ino = (ino64_t)va.va_fileid;
	} else {
		sb->st_dev = va.va_fsid;
		sb->st_ino = (ino_t)va.va_fileid;
	}
	mode = va.va_mode;
	switch (vp->v_type) {
	case VREG:
		mode |= S_IFREG;
		break;
	case VDIR:
		mode |= S_IFDIR;
		break;
	case VBLK:
		mode |= S_IFBLK;
		break;
	case VCHR:
		mode |= S_IFCHR;
		break;
	case VLNK:
		mode |= S_IFLNK;
		break;
	case VSOCK:
		mode |= S_IFSOCK;
		break;
	case VFIFO:
		mode |= S_IFIFO;
		break;
	default:
		error = EBADF;
		goto out;
	}
	;
	if (isstat64 != 0) {
		sb64->st_mode = mode;
		sb64->st_nlink = VATTR_IS_SUPPORTED(&va, va_nlink) ? va.va_nlink > UINT16_MAX ? UINT16_MAX : (u_int16_t)va.va_nlink : 1;
		sb64->st_uid = va.va_uid;
		sb64->st_gid = va.va_gid;
		sb64->st_rdev = va.va_rdev;
		sb64->st_size = va.va_data_size;
		sb64->st_atimespec = va.va_access_time;
		sb64->st_mtimespec = va.va_modify_time;
		sb64->st_ctimespec = va.va_change_time;
		if (VATTR_IS_SUPPORTED(&va, va_create_time)) {
			sb64->st_birthtimespec =  va.va_create_time;
		} else {
			sb64->st_birthtimespec.tv_sec = sb64->st_birthtimespec.tv_nsec = 0;
		}
		sb64->st_blksize = va.va_iosize;
		sb64->st_flags = va.va_flags;
		sb64->st_blocks = roundup(va.va_total_alloc, 512) / 512;
	} else {
		sb->st_mode = mode;
		sb->st_nlink = VATTR_IS_SUPPORTED(&va, va_nlink) ? va.va_nlink > UINT16_MAX ? UINT16_MAX : (u_int16_t)va.va_nlink : 1;
		sb->st_uid = va.va_uid;
		sb->st_gid = va.va_gid;
		sb->st_rdev = va.va_rdev;
		sb->st_size = va.va_data_size;
		sb->st_atimespec = va.va_access_time;
		sb->st_mtimespec = va.va_modify_time;
		sb->st_ctimespec = va.va_change_time;
		sb->st_blksize = va.va_iosize;
		sb->st_flags = va.va_flags;
		sb->st_blocks = roundup(va.va_total_alloc, 512) / 512;
	}

	                                                                     
	if (xsec != NULL) {
		if (!VATTR_IS_SUPPORTED(&va, va_acl) &&
		    !VATTR_IS_SUPPORTED(&va, va_uuuid) &&
		    !VATTR_IS_SUPPORTED(&va, va_guuid)) {
			*xsec = KAUTH_FILESEC_NONE;
		} else {
			if (VATTR_IS_SUPPORTED(&va, va_acl) && (va.va_acl != NULL)) {
				fsec = kauth_filesec_alloc(va.va_acl->acl_entrycount);
			} else {
				fsec = kauth_filesec_alloc(0);
			}
			if (fsec == NULL) {
				error = ENOMEM;
				goto out;
			}
			fsec->fsec_magic = KAUTH_FILESEC_MAGIC;
			if (VATTR_IS_SUPPORTED(&va, va_uuuid)) {
				fsec->fsec_owner = va.va_uuuid;
			} else {
				fsec->fsec_owner = kauth_null_guid;
			}
			if (VATTR_IS_SUPPORTED(&va, va_guuid)) {
				fsec->fsec_group = va.va_guuid;
			} else {
				fsec->fsec_group = kauth_null_guid;
			}
			if (VATTR_IS_SUPPORTED(&va, va_acl) && (va.va_acl != NULL)) {
				__nochk_bcopy(va.va_acl, &(fsec->fsec_acl), KAUTH_ACL_COPYSIZE(va.va_acl));
			} else {
				fsec->fsec_acl.acl_entrycount = KAUTH_FILESEC_NOACL;
			}
			*xsec = fsec;
		}
	}

	                                                                  
	if (va.va_gen && !vfs_context_issuser(ctx)) {
		if (isstat64 != 0) {
			sb64->st_gen = 0;
		} else {
			sb->st_gen = 0;
		}
	} else {
		if (isstat64 != 0) {
			sb64->st_gen = va.va_gen;
		} else {
			sb->st_gen = va.va_gen;
		}
	}

	error = 0;
out:
	if (VATTR_IS_SUPPORTED(&va, va_acl) && va.va_acl != NULL) {
		kauth_acl_free(va.va_acl);
	}
	return error;
}

int
vn_stat(struct vnode *vp, void *sb, kauth_filesec_t *xsec, int isstat64, int needsrealdev, vfs_context_t ctx)
{
	int error;

#if CONFIG_MACF
	error = mac_vnode_check_stat(ctx, NOCRED, vp);
	if (error) {
		return error;
	}
#endif

	               
	if ((error = vnode_authorize(vp, NULL, KAUTH_VNODE_READ_ATTRIBUTES | KAUTH_VNODE_READ_SECURITY, ctx)) != 0) {
		return error;
	}

	                 
	return vn_stat_noauth(vp, sb, xsec, isstat64, needsrealdev, ctx, NOCRED);
}


  
                                  
   
static int
vn_ioctl(struct fileproc *fp, u_long com, caddr_t data, vfs_context_t ctx)
{
	struct vnode *vp = ((struct vnode *)fp->fp_glob->fg_data);
	off_t file_size;
	int error;
	struct vnode *ttyvp;
	struct session * sessp;

	if ((error = vnode_getwithref(vp))) {
		return error;
	}

#if CONFIG_MACF
	error = mac_vnode_check_ioctl(ctx, vp, com);
	if (error) {
		goto out;
	}
#endif

	switch (vp->v_type) {
	case VREG:
	case VDIR:
		if (com == FIONREAD) {
			off_t temp_nbytes;
			if ((error = vnode_size(vp, &file_size, ctx)) != 0) {
				goto out;
			}
			temp_nbytes = file_size - fp->fp_glob->fg_offset;
			if (temp_nbytes > INT_MAX) {
				*(int *)data = INT_MAX;
			} else if (temp_nbytes < 0) {
				*(int *)data = 0;
			} else {
				*(int *)data = (int)temp_nbytes;
			}
			goto out;
		}
		if (com == FIONBIO || com == FIOASYNC) {                 
			goto out;
		}
		OS_FALLTHROUGH;

	default:
		error = ENOTTY;
		goto out;

	case VFIFO:
	case VCHR:
	case VBLK:

		if (com == TIOCREVOKE || com == TIOCREVOKECLEAR) {
			error = ENOTTY;
			goto out;
		}

		                                                          
		if (com == DKIOCSETBLOCKSIZE) {
			error = EPERM;
			goto out;
		}

		if (com == FIODTYPE) {
			if (vp->v_type == VBLK) {
				if (major(vp->v_rdev) >= nblkdev) {
					error = ENXIO;
					goto out;
				}
				*(int *)data = bdevsw[major(vp->v_rdev)].d_type;
			} else if (vp->v_type == VCHR) {
				if (major(vp->v_rdev) >= nchrdev) {
					error = ENXIO;
					goto out;
				}
				*(int *)data = cdevsw[major(vp->v_rdev)].d_type;
			} else {
				error = ENOTTY;
				goto out;
			}
			goto out;
		}
		error = VNOP_IOCTL(vp, com, data, fp->fp_glob->fg_flag, ctx);

		if (error == 0 && com == TIOCSCTTY) {
			sessp = proc_session(vfs_context_proc(ctx));

			session_lock(sessp);
			ttyvp = sessp->s_ttyvp;
			sessp->s_ttyvp = vp;
			sessp->s_ttyvid = vnode_vid(vp);
			session_unlock(sessp);
			session_rele(sessp);
		}
	}
out:
	(void)vnode_put(vp);
	return error;
}

  
                                   
   
static int
vn_select(struct fileproc *fp, int which, void *wql, __unused vfs_context_t ctx)
{
	int error;
	struct vnode * vp = (struct vnode *)fp->fp_glob->fg_data;
	struct vfs_context context;

	if ((error = vnode_getwithref(vp)) == 0) {
		context.vc_thread = current_thread();
		context.vc_ucred = fp->fp_glob->fg_cred;

#if CONFIG_MACF
		  
                                                               
                                                        
                                                     
     
		error = mac_vnode_check_select(ctx, vp, which);
		if (error == 0)
#endif
		error = VNOP_SELECT(vp, which, fp->fp_glob->fg_flag, wql, ctx);

		(void)vnode_put(vp);
	}
	return error;
}

  
                                  
   
static int
vn_closefile(struct fileglob *fg, vfs_context_t ctx)
{
	struct vnode *vp = fg->fg_data;
	int error;

	if ((error = vnode_getwithref(vp)) == 0) {
		if (FILEGLOB_DTYPE(fg) == DTYPE_VNODE &&
		    ((fg->fg_flag & FWASLOCKED) != 0 ||
		    (fg->fg_lflags & FG_HAS_OFDLOCK) != 0)) {
			struct flock lf = {
				.l_whence = SEEK_SET,
				.l_start = 0,
				.l_len = 0,
				.l_type = F_UNLCK
			};

			if ((fg->fg_flag & FWASLOCKED) != 0) {
				(void) VNOP_ADVLOCK(vp, (caddr_t)fg,
				    F_UNLCK, &lf, F_FLOCK, ctx, NULL);
			}

			if ((fg->fg_lflags & FG_HAS_OFDLOCK) != 0) {
				(void) VNOP_ADVLOCK(vp, (caddr_t)fg,
				    F_UNLCK, &lf, F_OFD_LOCK, ctx, NULL);
			}
		}
		error = vn_close(vp, fg->fg_flag, ctx);
		(void) vnode_put(vp);
	}
	return error;
}

  
                       
                    
   
int
vn_pathconf(vnode_t vp, int name, int32_t *retval, vfs_context_t ctx)
{
	int     error = 0;
	struct vfs_attr vfa;

	switch (name) {
	case _PC_EXTENDED_SECURITY_NP:
		*retval = vfs_extendedsecurity(vnode_mount(vp)) ? 1 : 0;
		break;
	case _PC_AUTH_OPAQUE_NP:
		*retval = vfs_authopaque(vnode_mount(vp));
		break;
	case _PC_2_SYMLINKS:
		*retval = 1;                                   
		break;
	case _PC_ALLOC_SIZE_MIN:
		*retval = 1;                         
		break;
	case _PC_ASYNC_IO:                                           
		*retval = 1;                                   
		break;
	case _PC_PRIO_IO:                                            
		*retval = 0;                                       
		break;
	case _PC_REC_INCR_XFER_SIZE:
		*retval = 4096;                                          
		break;
	case _PC_REC_MIN_XFER_SIZE:
		*retval = 4096;                                            
		break;
	case _PC_REC_MAX_XFER_SIZE:
		*retval = 65536;                                             
		break;
	case _PC_REC_XFER_ALIGN:
		*retval = 4096;                                         
		break;
	case _PC_SYMLINK_MAX:
		*retval = 255;                                      
		break;
	case _PC_SYNC_IO:                                             
		*retval = 0;                                       
		break;
	case _PC_XATTR_SIZE_BITS:
		                                                    
                                                          
                                                           
                                                            
                                                         
                                                           
                                                         
                                                             
                                                       
     
		memset(&vfa, 0, sizeof(vfa));
		VFSATTR_INIT(&vfa);
		VFSATTR_WANTED(&vfa, f_capabilities);
		if (vfs_getattr(vnode_mount(vp), &vfa, ctx) == 0 &&
		    (VFSATTR_IS_SUPPORTED(&vfa, f_capabilities)) &&
		    (vfa.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] & VOL_CAP_INT_EXTENDED_ATTR) &&
		    (vfa.f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] & VOL_CAP_INT_EXTENDED_ATTR)) {
			                                         
			error = VNOP_PATHCONF(vp, name, retval, ctx);
		} else {
			                                                       
                                                        
      
			*retval = AD_XATTR_SIZE_BITS;
		}
		break;
	default:
		error = VNOP_PATHCONF(vp, name, retval, ctx);
		break;
	}

	return error;
}

static int
vn_kqfilter(struct fileproc *fp, struct knote *kn, struct kevent_qos_s *kev)
{
	vfs_context_t ctx = vfs_context_current();
	struct vnode *vp;
	int error = 0;
	int result = 0;

	vp = (struct vnode *)fp->fp_glob->fg_data;

	  
                                         
    
	if ((error = vget_internal(vp, 0, VNODE_NODEAD)) == 0) {
		switch (kn->kn_filter) {
		case EVFILT_READ:
		case EVFILT_WRITE:
			if (vnode_isfifo(vp)) {
				                                                
				if (!(vp->v_fifoinfo && vp->v_fifoinfo->fi_readsock)) {
					error = ENOTSUP;
				}
			} else if (!vnode_isreg(vp)) {
				if (vnode_ischr(vp)) {
					result = spec_kqfilter(vp, kn, kev);
					if ((kn->kn_flags & EV_ERROR) == 0) {
						                                 
						vnode_put(vp);
						return result;
					}
				}
				error = EINVAL;
			}
			break;
		case EVFILT_VNODE:
			break;
		default:
			error = EINVAL;
		}

		if (error == 0) {
#if CONFIG_MACF
			error = mac_vnode_check_kqfilter(ctx, fp->fp_glob->fg_cred, kn, vp);
			if (error) {
				vnode_put(vp);
				goto out;
			}
#endif

			kn->kn_hook = (void*)vp;
			kn->kn_filtid = EVFILTID_VN;

			vnode_lock(vp);
			KNOTE_ATTACH(&vp->v_knotes, kn);
			result = filt_vnode_common(kn, NULL, vp, 0);
			vnode_unlock(vp);

			  
                                                         
                        
      
			VNOP_MONITOR(vp, 0, VNODE_MONITOR_BEGIN, (void*) kn, ctx);
		}

		vnode_put(vp);
	}

out:
	if (error) {
		knote_set_error(kn, error);
	}

	return result;
}

static void
filt_vndetach(struct knote *kn)
{
	vfs_context_t ctx = vfs_context_current();
	struct vnode *vp = (struct vnode *)kn->kn_hook;
	uint32_t vid = vnode_vid(vp);
	if (vnode_getwithvid(vp, vid)) {
		return;
	}

	vnode_lock(vp);
	KNOTE_DETACH(&vp->v_knotes, kn);
	vnode_unlock(vp);

	  
                                                                         
                                                                            
                            
    
	VNOP_MONITOR(vp, 0, VNODE_MONITOR_END, (void*)kn, ctx);
	vnode_put(vp);
}


  
                       
  
                                                                        
                                                                       
                                                         
   
static int64_t
vnode_readable_data_count(vnode_t vp, off_t current_offset, int ispoll)
{
	if (vnode_isfifo(vp)) {
#if FIFO
		int cnt;
		int err = fifo_charcount(vp, &cnt);
		if (err == 0) {
			return (int64_t)cnt;
		} else
#endif
		{
			return 0;
		}
	} else if (vnode_isreg(vp)) {
		if (ispoll) {
			return 1;
		}

		off_t amount;
		amount = vp->v_un.vu_ubcinfo->ui_size - current_offset;
		if (amount > INT64_MAX) {
			return INT64_MAX;
		} else if (amount < INT64_MIN) {
			return INT64_MIN;
		} else {
			return (int64_t)amount;
		}
	} else {
		panic("Should never have an EVFILT_READ except for reg or fifo.");
		return 0;
	}
}

  
                         
  
                                                                 
                                                                       
   
static intptr_t
vnode_writable_space_count(vnode_t vp)
{
	if (vnode_isfifo(vp)) {
#if FIFO
		long spc;
		int err = fifo_freespace(vp, &spc);
		if (err == 0) {
			return (intptr_t)spc;
		} else
#endif
		{
			return (intptr_t)0;
		}
	} else if (vnode_isreg(vp)) {
		return (intptr_t)1;
	} else {
		panic("Should never have an EVFILT_READ except for reg or fifo.");
		return 0;
	}
}

  
                                                
  
                          
                                                                           
                                                                   
                                              
                                                           
   
static int
filt_vnode_common(struct knote *kn, struct kevent_qos_s *kev, vnode_t vp, long hint)
{
	int activate = 0;
	int64_t data = 0;

	lck_mtx_assert(&vp->v_lock, LCK_MTX_ASSERT_OWNED);

	                                                                     
	if (NOTE_REVOKE == hint) {
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);
		activate = 1;

		if ((kn->kn_filter == EVFILT_VNODE) && (kn->kn_sfflags & NOTE_REVOKE)) {
			kn->kn_fflags |= NOTE_REVOKE;
		}
	} else {
		switch (kn->kn_filter) {
		case EVFILT_READ:
			data = vnode_readable_data_count(vp, kn->kn_fp->fp_glob->fg_offset, (kn->kn_flags & EV_POLL));
			activate = (data != 0);
			break;
		case EVFILT_WRITE:
			data = vnode_writable_space_count(vp);
			activate = (data != 0);
			break;
		case EVFILT_VNODE:
			                                                     
			if (kn->kn_sfflags & hint) {
				kn->kn_fflags |= hint;                                       
			}
			activate = (kn->kn_fflags != 0);
			break;
		default:
			panic("Invalid knote filter on a vnode!\n");
		}
	}

	if (kev && activate) {
		knote_fill_kevent(kn, kev, data);
	}

	return activate;
}

static int
filt_vnode(struct knote *kn, long hint)
{
	vnode_t vp = (struct vnode *)kn->kn_hook;

	return filt_vnode_common(kn, NULL, vp, hint);
}

static int
filt_vntouch(struct knote *kn, struct kevent_qos_s *kev)
{
	vnode_t vp = (struct vnode *)kn->kn_hook;
	uint32_t vid = vnode_vid(vp);
	int activate;
	int hint = 0;

	vnode_lock(vp);
	if (vnode_getiocount(vp, vid, VNODE_NODEAD | VNODE_WITHID) != 0) {
		                 
		hint = NOTE_REVOKE;
	}

	                                  
	kn->kn_sfflags = kev->fflags;

	activate = filt_vnode_common(kn, NULL, vp, hint);

	if (hint == 0) {
		vnode_put_locked(vp);
	}
	vnode_unlock(vp);

	return activate;
}

static int
filt_vnprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	vnode_t vp = (struct vnode *)kn->kn_hook;
	uint32_t vid = vnode_vid(vp);
	int activate;
	int hint = 0;

	vnode_lock(vp);
	if (vnode_getiocount(vp, vid, VNODE_NODEAD | VNODE_WITHID) != 0) {
		                 
		hint = NOTE_REVOKE;
	}
	activate = filt_vnode_common(kn, kev, vp, hint);

	                                                
	if (hint == 0) {
		vnode_put_locked(vp);
	}
	vnode_unlock(vp);

	return activate;
}
