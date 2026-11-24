/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */

/*-
 * Portions Copyright (c) 1992, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * John Heidemann of the UCLA Ficus project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *  @(#)null_vnops.c    8.6 (Berkeley) 5/27/95
 *
 * Ancestors:
 *  @(#)lofs_vnops.c    1.2 (Berkeley) 6/18/92
 *  ...and...
 *  @(#)null_vnodeops.c 1.20 92/07/07 UCLA Ficus project
 *
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/mount_internal.h>
#include <sys/namei.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>
#include <sys/xattr.h>
#include <sys/ubc.h>
#include <sys/types.h>
#include <sys/dirent.h>
#include <sys/kauth.h>

#include "nullfs.h"

#define NULL_ROOT_INO 2
#define NULL_SECOND_INO 3
#define NULL_THIRD_INO 4

vop_t * nullfs_vnodeop_p = NULL;

                                                                 
static int
nullfs_isspecialvp(struct vnode * vp)
{
	struct null_mount * null_mp;

	null_mp = MOUNTTONULLMOUNT(vnode_mount(vp));

	                                                                            
                                         
	if (vp && (vp == null_mp->nullm_rootvp || vp == null_mp->nullm_secondvp)) {
		return 1;
	}
	return 0;
}

                                                      
static int
nullfs_checkspecialvp(struct vnode* vp)
{
	int result = 0;
	struct null_mount * null_mp;

	null_mp = MOUNTTONULLMOUNT(vnode_mount(vp));

	lck_mtx_lock(&null_mp->nullm_lock);
	result = (nullfs_isspecialvp(vp));
	lck_mtx_unlock(&null_mp->nullm_lock);

	return result;
}

vfs_context_t
nullfs_get_patched_context(struct null_mount * null_mp, vfs_context_t ctx)
{
	struct vfs_context* ectx = ctx;
	if ((null_mp->nullm_flags & NULLM_UNVEIL) == NULLM_UNVEIL) {
		ectx = vfs_context_create(ctx);
		ectx->vc_ucred = kauth_cred_setuidgid(ectx->vc_ucred, null_mp->uid, null_mp->gid);
	}
	return ectx;
}

void
nullfs_cleanup_patched_context(struct null_mount * null_mp, vfs_context_t ctx)
{
	if ((null_mp->nullm_flags & NULLM_UNVEIL) == NULLM_UNVEIL) {
		vfs_context_rele(ctx);
	}
}

static int
nullfs_default(__unused struct vnop_generic_args * args)
{
	NULLFSDEBUG("%s (default)\n", ((struct vnodeop_desc_fake *)args->a_desc)->vdesc_name);
	return ENOTSUP;
}

static int
nullfs_special_getattr(struct vnop_getattr_args * args)
{
	mount_t mp                  = vnode_mount(args->a_vp);
	struct null_mount * null_mp = MOUNTTONULLMOUNT(mp);

	ino_t ino = NULL_ROOT_INO;
	struct vnode_attr covered_rootattr;
	vnode_t checkvp = null_mp->nullm_lowerrootvp;
	vfs_context_t ectx = nullfs_get_patched_context(null_mp, args->a_context);

	VATTR_INIT(&covered_rootattr);
	VATTR_WANTED(&covered_rootattr, va_uid);
	VATTR_WANTED(&covered_rootattr, va_gid);
	VATTR_WANTED(&covered_rootattr, va_create_time);
	VATTR_WANTED(&covered_rootattr, va_modify_time);
	VATTR_WANTED(&covered_rootattr, va_access_time);

	                                                                             
                                                     
	if (vnode_getwithvid(checkvp, null_mp->nullm_lowerrootvid)) {
		checkvp = vfs_vnodecovered(mp);
		if (checkvp == NULL) {
			nullfs_cleanup_patched_context(null_mp, ectx);
			return EIO;
		}
	}

	int error = vnode_getattr(checkvp, &covered_rootattr, ectx);

	vnode_put(checkvp);
	if (error) {
		                                                                            
                         
		nullfs_cleanup_patched_context(null_mp, ectx);
		return error;
	}

	                                                               
	if (args->a_vp == null_mp->nullm_secondvp) {
		ino = NULL_SECOND_INO;
	}

	VATTR_RETURN(args->a_vap, va_type, vnode_vtype(args->a_vp));
	VATTR_RETURN(args->a_vap, va_rdev, 0);
	VATTR_RETURN(args->a_vap, va_nlink, 3);                                            
	VATTR_RETURN(args->a_vap, va_total_size, 0);                     

	VATTR_RETURN(args->a_vap, va_data_size, 0);                     
	VATTR_RETURN(args->a_vap, va_data_alloc, 0);
	VATTR_RETURN(args->a_vap, va_iosize, vfs_statfs(mp)->f_iosize);
	VATTR_RETURN(args->a_vap, va_fileid, ino);
	VATTR_RETURN(args->a_vap, va_linkid, ino);
	if (VATTR_IS_ACTIVE(args->a_vap, va_fsid)) {
		VATTR_RETURN(args->a_vap, va_fsid, vfs_statfs(mp)->f_fsid.val[0]);                                      
	}
	if (VATTR_IS_ACTIVE(args->a_vap, va_fsid64)) {
		VATTR_RETURN(args->a_vap, va_fsid64, vfs_statfs(mp)->f_fsid);
	}
	VATTR_RETURN(args->a_vap, va_filerev, 0);
	VATTR_RETURN(args->a_vap, va_gen, 0);
	VATTR_RETURN(args->a_vap, va_flags, UF_HIDDEN);                                               
                                                                                                       

	if (ino == NULL_SECOND_INO) {
		VATTR_RETURN(args->a_vap, va_parentid, NULL_ROOT_INO);                             
                                                                                      
                                                                                      
                                                                                     
                                                                 
	}

	if (VATTR_IS_ACTIVE(args->a_vap, va_mode)) {
		                      
		VATTR_RETURN(args->a_vap, va_mode, S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	}
	if (VATTR_IS_ACTIVE(args->a_vap, va_uid)) {
		VATTR_RETURN(args->a_vap, va_uid, covered_rootattr.va_uid);
	}
	if (VATTR_IS_ACTIVE(args->a_vap, va_gid)) {
		VATTR_RETURN(args->a_vap, va_gid, covered_rootattr.va_gid);
	}

	if (VATTR_IS_ACTIVE(args->a_vap, va_create_time)) {
		VATTR_SET_SUPPORTED(args->a_vap, va_create_time);
		args->a_vap->va_create_time.tv_sec  = covered_rootattr.va_create_time.tv_sec;
		args->a_vap->va_create_time.tv_nsec = covered_rootattr.va_create_time.tv_nsec;
	}
	if (VATTR_IS_ACTIVE(args->a_vap, va_modify_time)) {
		VATTR_SET_SUPPORTED(args->a_vap, va_modify_time);
		args->a_vap->va_modify_time.tv_sec  = covered_rootattr.va_modify_time.tv_sec;
		args->a_vap->va_modify_time.tv_nsec = covered_rootattr.va_modify_time.tv_nsec;
	}
	if (VATTR_IS_ACTIVE(args->a_vap, va_access_time)) {
		VATTR_SET_SUPPORTED(args->a_vap, va_access_time);
		args->a_vap->va_modify_time.tv_sec  = covered_rootattr.va_access_time.tv_sec;
		args->a_vap->va_modify_time.tv_nsec = covered_rootattr.va_access_time.tv_nsec;
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return 0;
}

static int
nullfs_getattr(struct vnop_getattr_args * args)
{
	int error;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(args->a_vp));
	kauth_cred_t cred = vfs_context_ucred(args->a_context);
	NULLFSDEBUG("%s %p\n", __FUNCTION__, args->a_vp);

	lck_mtx_lock(&null_mp->nullm_lock);
	if (nullfs_isspecialvp(args->a_vp)) {
		error = nullfs_special_getattr(args);
		lck_mtx_unlock(&null_mp->nullm_lock);
		return error;
	}
	lck_mtx_unlock(&null_mp->nullm_lock);

	                                                                     
	struct vnode * lowervp = NULLVPTOLOWERVP(args->a_vp);
	vfs_context_t ectx = nullfs_get_patched_context(null_mp, args->a_context);
	error = vnode_getwithref(lowervp);

	if (error == 0) {
		error = VNOP_GETATTR(lowervp, args->a_vap, ectx);
		vnode_put(lowervp);

		if (error == 0) {
			                                                    
			if (VATTR_IS_ACTIVE(args->a_vap, va_fsid)) {
				VATTR_RETURN(args->a_vap, va_fsid, vfs_statfs(vnode_mount(args->a_vp))->f_fsid.val[0]);
			}
			if (VATTR_IS_ACTIVE(args->a_vap, va_fsid64)) {
				VATTR_RETURN(args->a_vap, va_fsid64, vfs_statfs(vnode_mount(args->a_vp))->f_fsid);
			}

			                            
			if ((null_mp->nullm_flags & NULLM_UNVEIL) == NULLM_UNVEIL) {
				if (VATTR_IS_ACTIVE(args->a_vap, va_mode)) {
					mode_t mode = args->a_vap->va_mode;                                                      

					                           
					                  
					if (vnode_authorize(lowervp, NULL, KAUTH_VNODE_GENERIC_READ_BITS, ectx) == 0) {
						mode |= S_IRUSR;
					} else {
						mode &= ~S_IRUSR;
					}

					             
					                                     
					if (vnode_authorize(lowervp, NULL, KAUTH_VNODE_GENERIC_EXECUTE_BITS, ectx) == 0) {
						mode |= S_IXUSR;
					} else {
						mode &= ~S_IXUSR;
					}

					NULLFSDEBUG("Settings bits to %d\n", mode);
					VATTR_RETURN(args->a_vap, va_mode, mode);
				}
				if (VATTR_IS_ACTIVE(args->a_vap, va_uid)) {
					VATTR_RETURN(args->a_vap, va_uid, kauth_cred_getuid(cred));
				}
				if (VATTR_IS_ACTIVE(args->a_vap, va_gid)) {
					VATTR_RETURN(args->a_vap, va_gid, kauth_cred_getgid(cred));
				}
			}
		}
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

static int
nullfs_open(struct vnop_open_args * args)
{
	int error;
	struct vnode *vp, *lvp;
	mount_t mp                  = vnode_mount(args->a_vp);
	struct null_mount * null_mp = MOUNTTONULLMOUNT(mp);
	NULLFSDEBUG("%s %p\n", __FUNCTION__, args->a_vp);

	if (nullfs_checkspecialvp(args->a_vp)) {
		return 0;                           
	}

	vfs_context_t ectx = nullfs_get_patched_context(null_mp, args->a_context);
	vp    = args->a_vp;
	lvp   = NULLVPTOLOWERVP(vp);
	error = vnode_getwithref(lvp);
	if (error == 0) {
		error = VNOP_OPEN(lvp, args->a_mode, ectx);
		vnode_put(lvp);
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

static int
nullfs_close(struct vnop_close_args * args)
{
	int error;
	struct vnode *vp, *lvp;
	mount_t mp                  = vnode_mount(args->a_vp);
	struct null_mount * null_mp = MOUNTTONULLMOUNT(mp);

	NULLFSDEBUG("%s %p\n", __FUNCTION__, args->a_vp);

	if (nullfs_checkspecialvp(args->a_vp)) {
		return 0;                           
	}

	vfs_context_t ectx = nullfs_get_patched_context(null_mp, args->a_context);
	vp  = args->a_vp;
	lvp = NULLVPTOLOWERVP(vp);

	error = vnode_getwithref(lvp);
	if (error == 0) {
		error = VNOP_CLOSE(lvp, args->a_fflag, ectx);
		vnode_put(lvp);
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

                                                       
  
                                                                  
  
                                                                
static int
null_get_lowerparent(vnode_t lvp, vnode_t * dvpp, vfs_context_t ctx)
{
	int error = 0;
	struct vnode_attr va;
	mount_t mp  = vnode_mount(lvp);
	vnode_t dvp = vnode_parent(lvp);

	if (dvp) {
		error = vnode_get(dvp);
		goto end;
	}

	error = ENOENT;
	if (!(mp->mnt_kern_flag & MNTK_PATH_FROM_ID)) {
		goto end;
	}

	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_parentid);

	error = vnode_getattr(lvp, &va, ctx);

	if (error || !VATTR_IS_SUPPORTED(&va, va_parentid)) {
		if (!error) {
			error = ENOTSUP;
		}
		goto end;
	}

	error = VFS_VGET(mp, (ino64_t)va.va_parentid, &dvp, ctx);

end:
	if (error == 0) {
		*dvpp = dvp;
	}
	return error;
}

                                                                 
static int
null_special_lookup(struct vnop_lookup_args * ap)
{
	struct componentname * cnp  = ap->a_cnp;
	struct vnode * dvp          = ap->a_dvp;
	struct vnode * ldvp         = NULL;
	struct vnode * lvp          = NULL;
	struct vnode * vp           = NULL;
	struct mount * mp           = vnode_mount(dvp);
	struct null_mount * null_mp = MOUNTTONULLMOUNT(mp);
	int error                   = ENOENT;
	vfs_context_t ectx     = nullfs_get_patched_context(null_mp, ap->a_context);

	if (dvp == null_mp->nullm_rootvp) {
		                     
		if (cnp->cn_nameptr[0] == '.') {
			if (cnp->cn_namelen == 1 || (cnp->cn_namelen == 2 && cnp->cn_nameptr[1] == '.')) {
				                                                          
				vp    = dvp;
				error = vnode_get(vp);
				goto end;
			}
		}

		                                                                       
                                             
		if (cnp->cn_namelen == 1 &&
		    (cnp->cn_nameptr[0] == 'd' || (null_mp->nullm_flags & NULLM_CASEINSENSITIVE ? cnp->cn_nameptr[0] == 'D' : 0))) {
			error = 0;
			if (null_mp->nullm_secondvp == NULL) {
				error = null_getnewvnode(mp, NULL, dvp, &vp, cnp, 0);
				if (error) {
					goto end;
				}

				null_mp->nullm_secondvp = vp;
			} else {
				vp    = null_mp->nullm_secondvp;
				error = vnode_get(vp);
			}
		}
	} else if (dvp == null_mp->nullm_secondvp) {
		                     
		if (cnp->cn_nameptr[0] == '.') {
			if (cnp->cn_namelen == 1) {
				vp    = dvp;
				error = vnode_get(vp);
				goto end;
			} else if (cnp->cn_namelen == 2 && cnp->cn_nameptr[1] == '.') {
				                                
				vp    = null_mp->nullm_rootvp;
				error = vnode_get(vp);
				goto end;
			}
		}
		                                                                           
                 
		                                                                      
                                                                              
		error = vnode_getwithvid(null_mp->nullm_lowerrootvp, null_mp->nullm_lowerrootvid);
		if (error) {
			goto end;
		}

		                                                                            
                   
                                            
                                                                            
                                            
                                                                               
                       
                                          
     
		error = null_get_lowerparent(null_mp->nullm_lowerrootvp, &ldvp, ectx);

		if (error == 0) {
			error = VNOP_LOOKUP(ldvp, &lvp, cnp, ectx);
			vnode_put(ldvp);

			if (error == 0) {
				if (lvp == null_mp->nullm_lowerrootvp) {
					                                                                 
                          
					error = null_nodeget(mp, lvp, dvp, &vp, cnp, 0);

					if (error == 0 && null_mp->nullm_thirdcovervp == NULL) {
						                                                
						null_mp->nullm_thirdcovervp = vp;
					}
				} else {
					error = ENOENT;
				}
				vnode_put(lvp);
			}
		}
		vnode_put(null_mp->nullm_lowerrootvp);
	}

end:
	nullfs_cleanup_patched_context(null_mp, ectx);
	if (error == 0) {
		*ap->a_vpp = vp;
	}
	return error;
}

  
                                                                    
                                                                     
                                      
   
static int
null_lookup(struct vnop_lookup_args * ap)
{
	struct componentname * cnp = ap->a_cnp;
	struct vnode * dvp         = ap->a_dvp;
	struct vnode *vp, *ldvp, *lvp;
	struct mount * mp;
	struct null_mount * null_mp;
	int error;
	vfs_context_t ectx;

	NULLFSDEBUG("%s parent: %p component: %.*s\n", __FUNCTION__, ap->a_dvp, cnp->cn_namelen, cnp->cn_nameptr);

	mp = vnode_mount(dvp);
	                                                                        
	if (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME || cnp->cn_nameiop == CREATE) {
		return EROFS;
	}
	null_mp = MOUNTTONULLMOUNT(mp);


	lck_mtx_lock(&null_mp->nullm_lock);
	if (nullfs_isspecialvp(dvp)) {
		error = null_special_lookup(ap);
		lck_mtx_unlock(&null_mp->nullm_lock);
		return error;
	}
	lck_mtx_unlock(&null_mp->nullm_lock);

	                    
	if (cnp->cn_nameptr[0] == '.') {
		if (cnp->cn_namelen == 1) {
			vp = dvp;
		} else if (cnp->cn_namelen == 2 && cnp->cn_nameptr[1] == '.') {
			                                                            
			vp = vnode_parent(dvp);
		} else {
			goto notdot;
		}

		error = vp ? vnode_get(vp) : ENOENT;

		if (error == 0) {
			*ap->a_vpp = vp;
		}

		return error;
	}

notdot:
	ectx = nullfs_get_patched_context(null_mp, ap->a_context);
	ldvp = NULLVPTOLOWERVP(dvp);
	vp = lvp = NULL;

	  
                                                             
                            
    
	error = vnode_getwithref(ldvp);
	if (error) {
		nullfs_cleanup_patched_context(null_mp, ectx);
		return error;
	}

	error = VNOP_LOOKUP(ldvp, &lvp, cnp, ectx);

	vnode_put(ldvp);

	if ((error == 0 || error == EJUSTRETURN) && lvp != NULL) {
		if (ldvp == lvp) {
			vp    = dvp;
			error = vnode_get(vp);
		} else {
			error = null_nodeget(mp, lvp, dvp, &vp, cnp, 0);
		}
		if (error == 0) {
			*ap->a_vpp = vp;
		}
		                                                      
		if (lvp != NULL) {
			vnode_put(lvp);
		}
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

  
                                        
   
static int
null_inactive(__unused struct vnop_inactive_args * ap)
{
	NULLFSDEBUG("%s %p\n", __FUNCTION__, ap->a_vp);

	return 0;
}

static int
null_reclaim(struct vnop_reclaim_args * ap)
{
	struct vnode * vp;
	struct null_node * xp;
	struct vnode * lowervp;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(ap->a_vp));

	NULLFSDEBUG("%s %p\n", __FUNCTION__, ap->a_vp);

	vp = ap->a_vp;

	xp      = VTONULL(vp);
	lowervp = xp->null_lowervp;

	lck_mtx_lock(&null_mp->nullm_lock);

	vnode_removefsref(vp);

	if (lowervp != NULL) {
		                                                                          
                  
		if (xp->null_flags & NULL_FLAG_HASHED) {
			                                                                         
                     
                                                                          
                    
			null_hashrem(xp);
		}
		vnode_getwithref(lowervp);
		vnode_rele(lowervp);
		vnode_put(lowervp);
	}

	if (vp == null_mp->nullm_rootvp) {
		null_mp->nullm_rootvp = NULL;
	} else if (vp == null_mp->nullm_secondvp) {
		null_mp->nullm_secondvp = NULL;
	} else if (vp == null_mp->nullm_thirdcovervp) {
		null_mp->nullm_thirdcovervp = NULL;
	}

	lck_mtx_unlock(&null_mp->nullm_lock);

	cache_purge(vp);
	vnode_clearfsnode(vp);

	FREE(xp, M_TEMP);

	return 0;
}

#define DIRENT_SZ(dp) ((sizeof(struct dirent) - NAME_MAX) + (((dp)->d_namlen + 1 + 3) & ~3))

static int
store_entry_special(ino_t ino, const char * name, struct uio * uio)
{
	struct dirent e;
	size_t namelen = strlen(name);
	int error      = EINVAL;

	if (namelen + 1 <= NAME_MAX) {
		memset(&e, 0, sizeof(e));

		e.d_ino  = ino;
		e.d_type = DT_DIR;

		e.d_namlen = namelen;                        
		e.d_reclen = DIRENT_SZ(&e);
		if (uio_resid(uio) >= e.d_reclen) {
			strlcpy(e.d_name, name, NAME_MAX);
			error = uiomove((caddr_t)&e, e.d_reclen, uio);
		} else {
			error = EMSGSIZE;
		}
	}
	return error;
}

static int
nullfs_special_readdir(struct vnop_readdir_args * ap)
{
	struct vnode * vp           = ap->a_vp;
	struct uio * uio            = ap->a_uio;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(vp));
	off_t offset                = uio_offset(uio);
	int error                   = ERANGE;
	int items                   = 0;
	ino_t ino                   = 0;
	const char * name           = NULL;

	if (ap->a_flags & (VNODE_READDIR_EXTENDED | VNODE_READDIR_REQSEEKOFF)) {
		return EINVAL;
	}

	if (offset == 0) {
		            
		if (vp == null_mp->nullm_rootvp) {
			ino = NULL_ROOT_INO;
		} else {                                                                 
			ino = NULL_SECOND_INO;
		}
		error = store_entry_special(ino, ".", uio);
		if (error) {
			goto out;
		}
		offset++;
		items++;
	}
	if (offset == 1) {
		             
		                                                                
		ino = NULL_ROOT_INO;

		error = store_entry_special(ino, "..", uio);
		if (error) {
			goto out;
		}
		offset++;
		items++;
	}
	if (offset == 2) {
		                        
		if (vp == null_mp->nullm_rootvp) {
			ino  = NULL_SECOND_INO;
			name = "d";
		} else {                                                                 
			ino = NULL_THIRD_INO;
			if (vnode_getwithvid(null_mp->nullm_lowerrootvp, null_mp->nullm_lowerrootvid)) {
				                                                                        
                                      
                                                
				error = 0;
				goto out;
			}
			name = vnode_getname_printable(null_mp->nullm_lowerrootvp);
		}
		error = store_entry_special(ino, name, uio);

		if (ino == NULL_THIRD_INO) {
			vnode_putname_printable(name);
			vnode_put(null_mp->nullm_lowerrootvp);
		}

		if (error) {
			goto out;
		}
		offset++;
		items++;
	}

out:
	if (error == EMSGSIZE) {
		error = 0;                                                                
                                                                           
	}
	uio_setoffset(uio, offset);
	if (ap->a_numdirent) {
		*ap->a_numdirent = items;
	}
	return error;
}

static int
nullfs_readdir(struct vnop_readdir_args * ap)
{
	struct vnode *vp, *lvp;
	int error;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(ap->a_vp));

	NULLFSDEBUG("%s %p\n", __FUNCTION__, ap->a_vp);
	                                                                             
    

	lck_mtx_lock(&null_mp->nullm_lock);
	if (nullfs_isspecialvp(ap->a_vp)) {
		error = nullfs_special_readdir(ap);
		lck_mtx_unlock(&null_mp->nullm_lock);
		return error;
	}
	lck_mtx_unlock(&null_mp->nullm_lock);

	vfs_context_t ectx = nullfs_get_patched_context(null_mp, ap->a_context);
	vp    = ap->a_vp;
	lvp   = NULLVPTOLOWERVP(vp);
	error = vnode_getwithref(lvp);
	if (error == 0) {
		error = VNOP_READDIR(lvp, ap->a_uio, ap->a_flags, ap->a_eofflag, ap->a_numdirent, ectx);
		vnode_put(lvp);
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

static int
nullfs_readlink(struct vnop_readlink_args * ap)
{
	NULLFSDEBUG("%s %p\n", __FUNCTION__, ap->a_vp);
	int error;
	struct vnode *vp, *lvp;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(ap->a_vp));

	if (nullfs_checkspecialvp(ap->a_vp)) {
		return ENOTSUP;                                      
	}

	vfs_context_t ectx = nullfs_get_patched_context(null_mp, ap->a_context);
	vp  = ap->a_vp;
	lvp = NULLVPTOLOWERVP(vp);

	error = vnode_getwithref(lvp);
	if (error == 0) {
		error = VNOP_READLINK(lvp, ap->a_uio, ectx);
		vnode_put(lvp);

		if (error) {
			NULLFSDEBUG("readlink failed: %d\n", error);
		}
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

static int
nullfs_pathconf(__unused struct vnop_pathconf_args * args)
{
	NULLFSDEBUG("%s %p\n", __FUNCTION__, args->a_vp);
	return EINVAL;
}

static int
nullfs_fsync(__unused struct vnop_fsync_args * args)
{
	NULLFSDEBUG("%s %p\n", __FUNCTION__, args->a_vp);
	return 0;
}

static int
nullfs_mmap(struct vnop_mmap_args * args)
{
	int error;
	struct vnode *vp, *lvp;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(args->a_vp));

	NULLFSDEBUG("%s %p\n", __FUNCTION__, args->a_vp);

	if (nullfs_checkspecialvp(args->a_vp)) {
		return 0;                           
	}

	vfs_context_t ectx = nullfs_get_patched_context(null_mp, args->a_context);
	vp    = args->a_vp;
	lvp   = NULLVPTOLOWERVP(vp);
	error = vnode_getwithref(lvp);
	if (error == 0) {
		error = VNOP_MMAP(lvp, args->a_fflags, ectx);
		vnode_put(lvp);
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

static int
nullfs_mnomap(struct vnop_mnomap_args * args)
{
	int error;
	struct vnode *vp, *lvp;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(args->a_vp));

	NULLFSDEBUG("%s %p\n", __FUNCTION__, args->a_vp);

	if (nullfs_checkspecialvp(args->a_vp)) {
		return 0;                           
	}

	vfs_context_t ectx = nullfs_get_patched_context(null_mp, args->a_context);
	vp    = args->a_vp;
	lvp   = NULLVPTOLOWERVP(vp);
	error = vnode_getwithref(lvp);
	if (error == 0) {
		error = VNOP_MNOMAP(lvp, ectx);
		vnode_put(lvp);
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

static int
nullfs_getxattr(struct vnop_getxattr_args * args)
{
	int error;
	struct vnode *vp, *lvp;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(args->a_vp));

	NULLFSDEBUG("%s %p\n", __FUNCTION__, args->a_vp);

	if (nullfs_checkspecialvp(args->a_vp)) {
		return ENOATTR;                                      
	}

	vfs_context_t ectx = nullfs_get_patched_context(null_mp, args->a_context);
	vp    = args->a_vp;
	lvp   = NULLVPTOLOWERVP(vp);
	error = vnode_getwithref(lvp);
	if (error == 0) {
		error = VNOP_GETXATTR(lvp, args->a_name, args->a_uio, args->a_size, args->a_options, ectx);
		vnode_put(lvp);
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

static int
nullfs_listxattr(struct vnop_listxattr_args * args)
{
	int error;
	struct vnode *vp, *lvp;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(args->a_vp));

	NULLFSDEBUG("%s %p\n", __FUNCTION__, args->a_vp);

	if (nullfs_checkspecialvp(args->a_vp)) {
		return 0;                                      
	}

	vfs_context_t ectx = nullfs_get_patched_context(null_mp, args->a_context);
	vp    = args->a_vp;
	lvp   = NULLVPTOLOWERVP(vp);
	error = vnode_getwithref(lvp);
	if (error == 0) {
		error = VNOP_LISTXATTR(lvp, args->a_uio, args->a_size, args->a_options, ectx);
		vnode_put(lvp);
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

                         
static int
nullfs_pagein(struct vnop_pagein_args * ap)
{
	int error = EIO;
	struct vnode *vp, *lvp;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(ap->a_vp));
	NULLFSDEBUG("%s %p\n", __FUNCTION__, ap->a_vp);

	vp  = ap->a_vp;
	lvp = NULLVPTOLOWERVP(vp);

	if (vnode_vtype(vp) != VREG) {
		return ENOTSUP;
	}

	vfs_context_t ectx = nullfs_get_patched_context(null_mp, ap->a_context);
	  
                                    
    
	if (vnode_getwithvid(lvp, NULLVPTOLOWERVID(vp)) == 0) {
		vm_offset_t ioaddr;
		uio_t auio;
		kern_return_t kret;
		off_t bytes_to_commit;
		off_t lowersize;
		upl_t upl      = ap->a_pl;
		user_ssize_t bytes_remaining = 0;

		auio = uio_create(1, ap->a_f_offset, UIO_SYSSPACE, UIO_READ);
		if (auio == NULL) {
			error = EIO;
			goto exit_no_unmap;
		}

		kret = ubc_upl_map(upl, &ioaddr);
		if (KERN_SUCCESS != kret) {
			panic("nullfs_pagein: ubc_upl_map() failed with (%d)", kret);
		}

		ioaddr += ap->a_pl_offset;

		error = uio_addiov(auio, (user_addr_t)ioaddr, ap->a_size);
		if (error) {
			goto exit;
		}

		lowersize = ubc_getsize(lvp);
		if (lowersize != ubc_getsize(vp)) {
			(void)ubc_setsize(vp, lowersize);                                           
		}

		error = VNOP_READ(lvp, auio, ((ap->a_flags & UPL_IOSYNC) ? IO_SYNC : 0), ectx);

		bytes_remaining = uio_resid(auio);
		if (bytes_remaining > 0 && bytes_remaining <= (user_ssize_t)ap->a_size) {
			                                                
			bzero((void*)((uintptr_t)(ioaddr + ap->a_size - bytes_remaining)), (size_t) bytes_remaining);
		}

exit:
		kret = ubc_upl_unmap(upl);
		if (KERN_SUCCESS != kret) {
			panic("nullfs_pagein: ubc_upl_unmap() failed with (%d)", kret);
		}

		if (auio != NULL) {
			uio_free(auio);
		}

exit_no_unmap:
		if ((ap->a_flags & UPL_NOCOMMIT) == 0) {
			if (!error && (bytes_remaining >= 0) && (bytes_remaining <= (user_ssize_t)ap->a_size)) {
				                                                
				bytes_to_commit = ap->a_size - bytes_remaining;
				if (bytes_to_commit) {
					                                                                                                              
					if (bytes_to_commit & PAGE_MASK) {
						bytes_to_commit = (bytes_to_commit & (~PAGE_MASK)) + (PAGE_MASK + 1);
						assert(bytes_to_commit <= (off_t)ap->a_size);

						bytes_remaining = ap->a_size - bytes_to_commit;
					}
					ubc_upl_commit_range(upl, ap->a_pl_offset, (upl_size_t)bytes_to_commit, UPL_COMMIT_FREE_ON_EMPTY);
				}

				                               
				if (bytes_remaining) {
					ubc_upl_abort_range(upl, ap->a_pl_offset + bytes_to_commit, (upl_size_t)bytes_remaining, UPL_ABORT_ERROR | UPL_ABORT_FREE_ON_EMPTY);
				}
			} else {
				ubc_upl_abort_range(upl, ap->a_pl_offset, (upl_size_t)ap->a_size, UPL_ABORT_ERROR | UPL_ABORT_FREE_ON_EMPTY);
			}
		}
		vnode_put(lvp);
	} else if ((ap->a_flags & UPL_NOCOMMIT) == 0) {
		ubc_upl_abort_range(ap->a_pl, ap->a_pl_offset, (upl_size_t)ap->a_size, UPL_ABORT_ERROR | UPL_ABORT_FREE_ON_EMPTY);
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

static int
nullfs_read(struct vnop_read_args * ap)
{
	int error = EIO;

	struct vnode *vp, *lvp;
	struct null_mount * null_mp = MOUNTTONULLMOUNT(vnode_mount(ap->a_vp));
	NULLFSDEBUG("%s %p\n", __FUNCTION__, ap->a_vp);

	if (nullfs_checkspecialvp(ap->a_vp)) {
		return ENOTSUP;                                       
	}

	vfs_context_t ectx = nullfs_get_patched_context(null_mp, ap->a_context);
	vp  = ap->a_vp;
	lvp = NULLVPTOLOWERVP(vp);

	  
                            
    
	if (vnode_getwithvid(lvp, NULLVPTOLOWERVID(vp)) == 0) {
		if (!vnode_isreg(lvp) && !vnode_islnk(lvp)) {
			error = EPERM;
			goto end;
		}

		if (uio_resid(ap->a_uio) == 0) {
			error = 0;
			goto end;
		}

		  
                                         
     

		error = VNOP_READ(lvp, ap->a_uio, ap->a_ioflag, ectx);
		if (error) {
			NULLFSDEBUG("VNOP_READ failed: %d\n", error);
		}
end:
		vnode_put(lvp);
	}

	nullfs_cleanup_patched_context(null_mp, ectx);
	return error;
}

  
                             
   

static const struct vnodeopv_entry_desc nullfs_vnodeop_entries[] = {
	{.opve_op = &vnop_default_desc, .opve_impl = (vop_t)nullfs_default}, {.opve_op = &vnop_getattr_desc, .opve_impl = (vop_t)nullfs_getattr},
	{.opve_op = &vnop_open_desc, .opve_impl = (vop_t)nullfs_open}, {.opve_op = &vnop_close_desc, .opve_impl = (vop_t)nullfs_close},
	{.opve_op = &vnop_inactive_desc, .opve_impl = (vop_t)null_inactive}, {.opve_op = &vnop_reclaim_desc, .opve_impl = (vop_t)null_reclaim},
	{.opve_op = &vnop_lookup_desc, .opve_impl = (vop_t)null_lookup}, {.opve_op = &vnop_readdir_desc, .opve_impl = (vop_t)nullfs_readdir},
	{.opve_op = &vnop_readlink_desc, .opve_impl = (vop_t)nullfs_readlink}, {.opve_op = &vnop_pathconf_desc, .opve_impl = (vop_t)nullfs_pathconf},
	{.opve_op = &vnop_fsync_desc, .opve_impl = (vop_t)nullfs_fsync}, {.opve_op = &vnop_mmap_desc, .opve_impl = (vop_t)nullfs_mmap},
	{.opve_op = &vnop_mnomap_desc, .opve_impl = (vop_t)nullfs_mnomap}, {.opve_op = &vnop_getxattr_desc, .opve_impl = (vop_t)nullfs_getxattr},
	{.opve_op = &vnop_pagein_desc, .opve_impl = (vop_t)nullfs_pagein}, {.opve_op = &vnop_read_desc, .opve_impl = (vop_t)nullfs_read},
	{.opve_op = &vnop_listxattr_desc, .opve_impl = (vop_t)nullfs_listxattr}, {.opve_op = NULL, .opve_impl = NULL},
};

const struct vnodeopv_desc nullfs_vnodeop_opv_desc = {.opv_desc_vector_p = &nullfs_vnodeop_p, .opv_desc_ops = nullfs_vnodeop_entries};

                                 

int
nullfs_getbackingvnode(vnode_t in_vp, vnode_t* out_vpp)
{
	int result = EINVAL;

	if (out_vpp == NULL || in_vp == NULL) {
		goto end;
	}

	struct vfsstatfs * sp   = NULL;
	mount_t mp = vnode_mount(in_vp);

	sp = vfs_statfs(mp);
	                                                                
	if (strcmp(sp->f_fstypename, "nullfs") != 0 || nullfs_checkspecialvp(in_vp)) {
		*out_vpp = NULLVP;
		result = ENOENT;
		goto end;
	}

	vnode_t lvp = NULLVPTOLOWERVP(in_vp);
	if ((result = vnode_getwithvid(lvp, NULLVPTOLOWERVID(in_vp)))) {
		goto end;
	}

	*out_vpp = lvp;

end:
	return result;
}
