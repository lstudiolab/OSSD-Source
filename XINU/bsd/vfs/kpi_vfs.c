/*
 * Copyright (c) 2000-2017 Apple Inc. All rights reserved.
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
 * Copyright (c) 1989, 1993
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
 *	@(#)kpi_vfs.c
 */
  
                                                                      
                                                                          
                                                                        
               
   

  
                                       
   


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/mount.h>
#include <sys/mount_internal.h>
#include <sys/time.h>
#include <sys/disk.h>
#include <sys/vnode_internal.h>
#include <sys/stat.h>
#include <sys/namei.h>
#include <sys/ucred.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <kern/kalloc.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/ubc.h>
#include <sys/vm.h>
#include <sys/sysctl.h>
#include <sys/filedesc.h>
#include <sys/event.h>
#include <sys/fsevents.h>
#include <sys/user.h>
#include <sys/lockf.h>
#include <sys/xattr.h>
#include <sys/kdebug.h>

#include <kern/assert.h>
#include <kern/zalloc.h>
#include <kern/task.h>
#include <kern/policy_internal.h>

#include <libkern/OSByteOrder.h>

#include <miscfs/specfs/specdev.h>

#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <mach/task.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#if NULLFS
#include <miscfs/nullfs/nullfs.h>
#endif

#include <sys/sdt.h>

#define ESUCCESS 0
#undef mount_t
#undef vnode_t

#define COMPAT_ONLY

#define NATIVE_XATTR(VP)  \
	((VP)->v_mount ? (VP)->v_mount->mnt_kern_flag & MNTK_EXTENDED_ATTRS : 0)

#if CONFIG_APPLEDOUBLE
static void xattrfile_remove(vnode_t dvp, const char *basename,
    vfs_context_t ctx, int force);
static void xattrfile_setattr(vnode_t dvp, const char * basename,
    struct vnode_attr * vap, vfs_context_t ctx);
#endif                         

extern lck_rw_t rootvnode_rw_lock;

static errno_t post_rename(vnode_t fdvp, vnode_t fvp, vnode_t tdvp, vnode_t tvp);

static ZONE_VIEW_DEFINE(ZV_VFS_CONTEXT, "vfs_context",
    KHEAP_ID_DEFAULT, sizeof(struct vfs_context));

  
                        
  
                                                                            
                                                                              
                                  
  
                                      
  
                       
  
                                                                           
                                                                            
                                                        
   
void
vnode_setneedinactive(vnode_t vp)
{
	cache_purge(vp);

	vnode_lock_spin(vp);
	vp->v_lflag |= VL_NEEDINACTIVE;
	vnode_unlock(vp);
}


                                                                            
                                                                            
                                                                            

  
                                             
   
int
VFS_MOUNT(mount_t mp, vnode_t devvp, user_addr_t data, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_mount == 0)) {
		return ENOTSUP;
	}

	if (vfs_context_is64bit(ctx)) {
		if (vfs_64bitready(mp)) {
			error = (*mp->mnt_op->vfs_mount)(mp, devvp, data, ctx);
		} else {
			error = ENOTSUP;
		}
	} else {
		error = (*mp->mnt_op->vfs_mount)(mp, devvp, data, ctx);
	}

	return error;
}

int
VFS_START(mount_t mp, int flags, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_start == 0)) {
		return ENOTSUP;
	}

	error = (*mp->mnt_op->vfs_start)(mp, flags, ctx);

	return error;
}

int
VFS_UNMOUNT(mount_t mp, int flags, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_unmount == 0)) {
		return ENOTSUP;
	}

	error = (*mp->mnt_op->vfs_unmount)(mp, flags, ctx);

	return error;
}

  
                       
                           
                     
                  
  
                                                                      
                                                                   
                                                               
                                                                    
  
                                                                   
                                                                   
                                                                   
                                                                  
                                
   
int
VFS_ROOT(mount_t mp, struct vnode  ** vpp, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_root == 0)) {
		return ENOTSUP;
	}

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}

	error = (*mp->mnt_op->vfs_root)(mp, vpp, ctx);

	return error;
}

int
VFS_QUOTACTL(mount_t mp, int cmd, uid_t uid, caddr_t datap, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_quotactl == 0)) {
		return ENOTSUP;
	}

	error = (*mp->mnt_op->vfs_quotactl)(mp, cmd, uid, datap, ctx);

	return error;
}

int
VFS_GETATTR(mount_t mp, struct vfs_attr *vfa, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_getattr == 0)) {
		return ENOTSUP;
	}

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}

	error = (*mp->mnt_op->vfs_getattr)(mp, vfa, ctx);

	return error;
}

int
VFS_SETATTR(mount_t mp, struct vfs_attr *vfa, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_setattr == 0)) {
		return ENOTSUP;
	}

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}

	error = (*mp->mnt_op->vfs_setattr)(mp, vfa, ctx);

	return error;
}

int
VFS_SYNC(mount_t mp, int flags, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_sync == 0)) {
		return ENOTSUP;
	}

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}

	error = (*mp->mnt_op->vfs_sync)(mp, flags, ctx);

	return error;
}

int
VFS_VGET(mount_t mp, ino64_t ino, struct vnode **vpp, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_vget == 0)) {
		return ENOTSUP;
	}

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}

	error = (*mp->mnt_op->vfs_vget)(mp, ino, vpp, ctx);

	return error;
}

int
VFS_FHTOVP(mount_t mp, int fhlen, unsigned char *fhp, vnode_t *vpp, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_fhtovp == 0)) {
		return ENOTSUP;
	}

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}

	error = (*mp->mnt_op->vfs_fhtovp)(mp, fhlen, fhp, vpp, ctx);

	return error;
}

int
VFS_VPTOFH(struct vnode *vp, int *fhlenp, unsigned char *fhp, vfs_context_t ctx)
{
	int error;

	if ((vp->v_mount == dead_mountp) || (vp->v_mount->mnt_op->vfs_vptofh == 0)) {
		return ENOTSUP;
	}

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}

	error = (*vp->v_mount->mnt_op->vfs_vptofh)(vp, fhlenp, fhp, ctx);

	return error;
}

int
VFS_IOCTL(struct mount *mp, u_long command, caddr_t data,
    int flags, vfs_context_t context)
{
	if (mp == dead_mountp || !mp->mnt_op->vfs_ioctl) {
		return ENOTSUP;
	}

	return mp->mnt_op->vfs_ioctl(mp, command, data, flags,
	           context ?: vfs_context_current());
}

int
VFS_VGET_SNAPDIR(mount_t mp, vnode_t *vpp, vfs_context_t ctx)
{
	int error;

	if ((mp == dead_mountp) || (mp->mnt_op->vfs_vget_snapdir == 0)) {
		return ENOTSUP;
	}

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}

	error = (*mp->mnt_op->vfs_vget_snapdir)(mp, vpp, ctx);

	return error;
}

                                                      
uint64_t
vfs_throttle_mask(mount_t mp)
{
	return mp->mnt_throttle_mask;
}

                                                      
void
vfs_name(mount_t mp, char *buffer)
{
	strncpy(buffer, mp->mnt_vtable->vfc_name, MFSNAMELEN);
}

                                              
int
vfs_typenum(mount_t mp)
{
	return mp->mnt_vtable->vfc_typenum;
}

                                                                                                           
void*
vfs_mntlabel(mount_t mp)
{
	return (void*)mp->mnt_mntlabel;
}

uint64_t
vfs_mount_id(mount_t mp)
{
	return mp->mnt_mount_id;
}

                                                                
uint64_t
vfs_flags(mount_t mp)
{
	return (uint64_t)(mp->mnt_flag & (MNT_CMDFLAGS | MNT_VISFLAGMASK));
}

                                                                    
void
vfs_setflags(mount_t mp, uint64_t flags)
{
	uint32_t lflags = (uint32_t)(flags & (MNT_CMDFLAGS | MNT_VISFLAGMASK));

	mount_lock(mp);
	mp->mnt_flag |= lflags;
	mount_unlock(mp);
}

                                                                      
void
vfs_clearflags(mount_t mp, uint64_t flags)
{
	uint32_t lflags = (uint32_t)(flags & (MNT_CMDFLAGS | MNT_VISFLAGMASK));

	mount_lock(mp);
	mp->mnt_flag &= ~lflags;
	mount_unlock(mp);
}

                                                            
int
vfs_iswriteupgrade(mount_t mp)                              
{
	return (mp->mnt_flag & MNT_RDONLY) && (mp->mnt_kern_flag & MNTK_WANTRDWR);
}


                                  
int
vfs_isrdonly(mount_t mp)
{
	return mp->mnt_flag & MNT_RDONLY;
}

                                                               
int
vfs_issynchronous(mount_t mp)
{
	return mp->mnt_flag & MNT_SYNCHRONOUS;
}

                                        
int
vfs_isrdwr(mount_t mp)
{
	return (mp->mnt_flag & MNT_RDONLY) == 0;
}


                                                  
int
vfs_isupdate(mount_t mp)
{
	return mp->mnt_flag & MNT_UPDATE;
}


                                                  
int
vfs_isreload(mount_t mp)
{
	return (mp->mnt_flag & MNT_UPDATE) && (mp->mnt_flag & MNT_RELOAD);
}

                                                                            
int
vfs_isforce(mount_t mp)
{
	if (mp->mnt_lflag & MNT_LFORCE) {
		return 1;
	} else {
		return 0;
	}
}

int
vfs_isunmount(mount_t mp)
{
	if ((mp->mnt_lflag & MNT_LUNMOUNT)) {
		return 1;
	} else {
		return 0;
	}
}

int
vfs_64bitready(mount_t mp)
{
	if ((mp->mnt_vtable->vfc_vfsflags & VFC_VFS64BITREADY)) {
		return 1;
	} else {
		return 0;
	}
}


int
vfs_authcache_ttl(mount_t mp)
{
	if ((mp->mnt_kern_flag & (MNTK_AUTH_OPAQUE | MNTK_AUTH_CACHE_TTL))) {
		return mp->mnt_authcache_ttl;
	} else {
		return CACHED_RIGHT_INFINITE_TTL;
	}
}

void
vfs_setauthcache_ttl(mount_t mp, int ttl)
{
	mount_lock(mp);
	mp->mnt_kern_flag |= MNTK_AUTH_CACHE_TTL;
	mp->mnt_authcache_ttl = ttl;
	mount_unlock(mp);
}

void
vfs_clearauthcache_ttl(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag &= ~MNTK_AUTH_CACHE_TTL;
	  
                                         
                                         
    
	mp->mnt_authcache_ttl = CACHED_LOOKUP_RIGHT_TTL;
	mount_unlock(mp);
}

int
vfs_authopaque(mount_t mp)
{
	if ((mp->mnt_kern_flag & MNTK_AUTH_OPAQUE)) {
		return 1;
	} else {
		return 0;
	}
}

int
vfs_authopaqueaccess(mount_t mp)
{
	if ((mp->mnt_kern_flag & MNTK_AUTH_OPAQUE_ACCESS)) {
		return 1;
	} else {
		return 0;
	}
}

void
vfs_setauthopaque(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag |= MNTK_AUTH_OPAQUE;
	mount_unlock(mp);
}

void
vfs_setauthopaqueaccess(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag |= MNTK_AUTH_OPAQUE_ACCESS;
	mount_unlock(mp);
}

void
vfs_clearauthopaque(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag &= ~MNTK_AUTH_OPAQUE;
	mount_unlock(mp);
}

void
vfs_clearauthopaqueaccess(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag &= ~MNTK_AUTH_OPAQUE_ACCESS;
	mount_unlock(mp);
}

void
vfs_setextendedsecurity(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag |= MNTK_EXTENDED_SECURITY;
	mount_unlock(mp);
}

void
vfs_setmntsystem(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag |= MNTK_SYSTEM;
	mount_unlock(mp);
}

void
vfs_setmntsystemdata(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag |= MNTK_SYSTEMDATA;
	mount_unlock(mp);
}

void
vfs_setmntswap(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag |= (MNTK_SYSTEM | MNTK_SWAP_MOUNT);
	mount_unlock(mp);
}

void
vfs_clearextendedsecurity(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag &= ~MNTK_EXTENDED_SECURITY;
	mount_unlock(mp);
}

void
vfs_setnoswap(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag |= MNTK_NOSWAP;
	mount_unlock(mp);
}

void
vfs_clearnoswap(mount_t mp)
{
	mount_lock(mp);
	mp->mnt_kern_flag &= ~MNTK_NOSWAP;
	mount_unlock(mp);
}

int
vfs_extendedsecurity(mount_t mp)
{
	return mp->mnt_kern_flag & MNTK_EXTENDED_SECURITY;
}

                                                           
uint32_t
vfs_maxsymlen(mount_t mp)
{
	return mp->mnt_maxsymlinklen;
}

                                               
void
vfs_setmaxsymlen(mount_t mp, uint32_t symlen)
{
	mp->mnt_maxsymlinklen = symlen;
}

boolean_t
vfs_is_basesystem(mount_t mp)
{
	return ((mp->mnt_supl_kern_flag & MNTK_SUPL_BASESYSTEM) == 0) ? false : true;
}

                                                                   
struct vfsstatfs *
vfs_statfs(mount_t mp)
{
	return &mp->mnt_vfsstat;
}

int
vfs_getattr(mount_t mp, struct vfs_attr *vfa, vfs_context_t ctx)
{
	int             error;

	if ((error = VFS_GETATTR(mp, vfa, ctx)) != 0) {
		return error;
	}

	  
                                                                       
    
	if (VFSATTR_IS_SUPPORTED(vfa, f_create_time)) {
		if (VFSATTR_IS_ACTIVE(vfa, f_modify_time) && !VFSATTR_IS_SUPPORTED(vfa, f_modify_time)) {
			VFSATTR_RETURN(vfa, f_modify_time, vfa->f_create_time);
		}
	}

	return 0;
}

int
vfs_setattr(mount_t mp, struct vfs_attr *vfa, vfs_context_t ctx)
{
	int error;

	  
                                                                              
                                                                            
                   
    
	if (vfs_isrdonly(mp) &&
	    !((strcmp(mp->mnt_vfsstat.f_fstypename, "apfs") == 0) && (vfa->f_active == VFSATTR_f_vol_name))) {
		return EROFS;
	}

	error = VFS_SETATTR(mp, vfa, ctx);

	  
                                                            
                   
    

	return error;
}

                                                      
void *
vfs_fsprivate(mount_t mp)
{
	return mp->mnt_data;
}

                                            
void
vfs_setfsprivate(mount_t mp, void *mntdata)
{
	mount_lock(mp);
	mp->mnt_data = mntdata;
	mount_unlock(mp);
}

                                                       
int
vfs_nativexattrs(mount_t mp)
{
	return mp->mnt_kern_flag & MNTK_EXTENDED_ATTRS;
}

  
                                          
                                 
   
int
vfs_devblocksize(mount_t mp)
{
	return mp->mnt_devblocksize;
}

  
                                                                       
   
vnode_t
vfs_vnodecovered(mount_t mp)
{
	vnode_t vp = mp->mnt_vnodecovered;
	if ((vp == NULL) || (vnode_getwithref(vp) != 0)) {
		return NULL;
	} else {
		return vp;
	}
}

  
                                                                                     
                                                                               
                                                                                   
                                                                             
                                                                                
                                                                                
                                   
   
vnode_t
vfs_devvp(mount_t mp)
{
	vnode_t vp = mp->mnt_devvp;

	if ((vp != NULLVP) && (vnode_get(vp) == 0)) {
		return vp;
	}

	return NULLVP;
}

  
                                                   
   
void
vfs_ioattr(mount_t mp, struct vfsioattr *ioattrp)
{
	ioattrp->io_reserved[0] = NULL;
	ioattrp->io_reserved[1] = NULL;
	if (mp == NULL) {
		ioattrp->io_maxreadcnt  = MAXPHYS;
		ioattrp->io_maxwritecnt = MAXPHYS;
		ioattrp->io_segreadcnt  = 32;
		ioattrp->io_segwritecnt = 32;
		ioattrp->io_maxsegreadsize  = MAXPHYS;
		ioattrp->io_maxsegwritesize = MAXPHYS;
		ioattrp->io_devblocksize = DEV_BSIZE;
		ioattrp->io_flags = 0;
		ioattrp->io_max_swappin_available = 0;
	} else {
		ioattrp->io_maxreadcnt  = mp->mnt_maxreadcnt;
		ioattrp->io_maxwritecnt = mp->mnt_maxwritecnt;
		ioattrp->io_segreadcnt  = mp->mnt_segreadcnt;
		ioattrp->io_segwritecnt = mp->mnt_segwritecnt;
		ioattrp->io_maxsegreadsize  = mp->mnt_maxsegreadsize;
		ioattrp->io_maxsegwritesize = mp->mnt_maxsegwritesize;
		ioattrp->io_devblocksize = mp->mnt_devblocksize;
		ioattrp->io_flags = mp->mnt_ioflags;
		ioattrp->io_max_swappin_available = mp->mnt_max_swappin_available;
	}
}


  
                                                
   
void
vfs_setioattr(mount_t mp, struct vfsioattr * ioattrp)
{
	if (mp == NULL) {
		return;
	}
	mp->mnt_maxreadcnt  = ioattrp->io_maxreadcnt;
	mp->mnt_maxwritecnt = ioattrp->io_maxwritecnt;
	mp->mnt_segreadcnt  = ioattrp->io_segreadcnt;
	mp->mnt_segwritecnt = ioattrp->io_segwritecnt;
	mp->mnt_maxsegreadsize = ioattrp->io_maxsegreadsize;
	mp->mnt_maxsegwritesize = ioattrp->io_maxsegwritesize;
	mp->mnt_devblocksize = ioattrp->io_devblocksize;
	mp->mnt_ioflags = ioattrp->io_flags;
	mp->mnt_max_swappin_available = ioattrp->io_max_swappin_available;
}

  
                                                              
                                            
                                                                   
                                                                          
   
typedef int (*PFI)(void *);
extern int vfs_opv_numops;
errno_t
vfs_fsadd(struct vfs_fsentry *vfe, vfstable_t *handle)
{
	struct vfstable *newvfstbl = NULL;
	int     i, j;
	int(***opv_desc_vector_p)(void *);
	int(**opv_desc_vector)(void *);
	const struct vnodeopv_entry_desc        *opve_descp;
	int desccount;
	int descsize;
	PFI *descptr;

	  
                                                                     
                                                     
    

	if (vfe == (struct vfs_fsentry *)0) {
		return EINVAL;
	}

	desccount = vfe->vfe_vopcnt;
	if ((desccount <= 0) || ((desccount > 8)) || (vfe->vfe_vfsops == (struct vfsops *)NULL)
	    || (vfe->vfe_opvdescs == (struct vnodeopv_desc **)NULL)) {
		return EINVAL;
	}

	                                                  
	if ((vfe->vfe_flags &  (VFS_TBLTHREADSAFE | VFS_TBLFSNODELOCK)) == 0) {
		return EINVAL;
	}

	newvfstbl = kheap_alloc(KHEAP_TEMP, sizeof(struct vfstable),
	    Z_WAITOK | Z_ZERO);
	newvfstbl->vfc_vfsops = vfe->vfe_vfsops;
	strncpy(&newvfstbl->vfc_name[0], vfe->vfe_fsname, MFSNAMELEN);
	if ((vfe->vfe_flags & VFS_TBLNOTYPENUM)) {
		newvfstbl->vfc_typenum = maxvfstypenum++;
	} else {
		newvfstbl->vfc_typenum = vfe->vfe_fstypenum;
	}

	newvfstbl->vfc_refcount = 0;
	newvfstbl->vfc_flags = 0;
	newvfstbl->vfc_mountroot = NULL;
	newvfstbl->vfc_next = NULL;
	newvfstbl->vfc_vfsflags = 0;
	if (vfe->vfe_flags &  VFS_TBL64BITREADY) {
		newvfstbl->vfc_vfsflags |= VFC_VFS64BITREADY;
	}
	if (vfe->vfe_flags &  VFS_TBLVNOP_PAGEINV2) {
		newvfstbl->vfc_vfsflags |= VFC_VFSVNOP_PAGEINV2;
	}
	if (vfe->vfe_flags &  VFS_TBLVNOP_PAGEOUTV2) {
		newvfstbl->vfc_vfsflags |= VFC_VFSVNOP_PAGEOUTV2;
	}
	if ((vfe->vfe_flags & VFS_TBLLOCALVOL) == VFS_TBLLOCALVOL) {
		newvfstbl->vfc_flags |= MNT_LOCAL;
	}
	if ((vfe->vfe_flags & VFS_TBLLOCALVOL) && (vfe->vfe_flags & VFS_TBLGENERICMNTARGS) == 0) {
		newvfstbl->vfc_vfsflags |= VFC_VFSLOCALARGS;
	} else {
		newvfstbl->vfc_vfsflags |= VFC_VFSGENERICARGS;
	}

	if (vfe->vfe_flags &  VFS_TBLNATIVEXATTR) {
		newvfstbl->vfc_vfsflags |= VFC_VFSNATIVEXATTR;
	}
	if (vfe->vfe_flags &  VFS_TBLUNMOUNT_PREFLIGHT) {
		newvfstbl->vfc_vfsflags |= VFC_VFSPREFLIGHT;
	}
	if (vfe->vfe_flags &  VFS_TBLREADDIR_EXTENDED) {
		newvfstbl->vfc_vfsflags |= VFC_VFSREADDIR_EXTENDED;
	}
	if (vfe->vfe_flags & VFS_TBLNOMACLABEL) {
		newvfstbl->vfc_vfsflags |= VFC_VFSNOMACLABEL;
	}
	if (vfe->vfe_flags & VFS_TBLVNOP_NOUPDATEID_RENAME) {
		newvfstbl->vfc_vfsflags |= VFC_VFSVNOP_NOUPDATEID_RENAME;
	}
	if (vfe->vfe_flags & VFS_TBLVNOP_SECLUDE_RENAME) {
		newvfstbl->vfc_vfsflags |= VFC_VFSVNOP_SECLUDE_RENAME;
	}
	if (vfe->vfe_flags & VFS_TBLCANMOUNTROOT) {
		newvfstbl->vfc_vfsflags |= VFC_VFSCANMOUNTROOT;
	}

	  
                                  
                                        
   
                                                       
                                                
    
	                               

	descsize = desccount * vfs_opv_numops * sizeof(PFI);
	descptr = kheap_alloc(KHEAP_DEFAULT, descsize, Z_WAITOK | Z_ZERO);

	newvfstbl->vfc_descptr = descptr;
	newvfstbl->vfc_descsize = descsize;

	newvfstbl->vfc_sysctl = NULL;

	for (i = 0; i < desccount; i++) {
		opv_desc_vector_p = vfe->vfe_opvdescs[i]->opv_desc_vector_p;
		  
                                                                  
                                                         
     
		opv_desc_vector = descptr + i * vfs_opv_numops;
		*opv_desc_vector_p = opv_desc_vector;

		for (j = 0; vfe->vfe_opvdescs[i]->opv_desc_ops[j].opve_op; j++) {
			opve_descp = &(vfe->vfe_opvdescs[i]->opv_desc_ops[j]);

			                                             
			if (opve_descp->opve_op->vdesc_flags & VDESC_DISABLED) {
				printf("vfs_fsadd: Ignoring reference in %p to disabled operation %s.\n",
				    vfe->vfe_opvdescs[i], opve_descp->opve_op->vdesc_name);
				continue;
			}

			  
                                             
                                               
                                             
                                                 
                                                  
                                              
                                      
                                                
     
                                             
                                          
                                              
                                  
                                                 
                                   
      
			if (opve_descp->opve_op->vdesc_offset == 0 &&
			    opve_descp->opve_op != VDESC(vnop_default)) {
				printf("vfs_fsadd: operation %s not listed in %s.\n",
				    opve_descp->opve_op->vdesc_name,
				    "vfs_op_descs");
				panic("vfs_fsadd: bad operation");
			}
			  
                         
      
			opv_desc_vector[opve_descp->opve_op->vdesc_offset] =
			    opve_descp->opve_impl;
		}


		  
                                                   
                                                        
                                                               
     
		opv_desc_vector_p = vfe->vfe_opvdescs[i]->opv_desc_vector_p;

		  
                                                             
     
		opv_desc_vector = *opv_desc_vector_p;
		if (opv_desc_vector[VOFFSET(vnop_default)] == NULL) {
			panic("vfs_fsadd: operation vector without default routine.");
		}
		for (j = 0; j < vfs_opv_numops; j++) {
			if (opv_desc_vector[j] == NULL) {
				opv_desc_vector[j] =
				    opv_desc_vector[VOFFSET(vnop_default)];
			}
		}
	}                                        



	*handle = vfstable_add(newvfstbl);

	if (newvfstbl->vfc_typenum <= maxvfstypenum) {
		maxvfstypenum = newvfstbl->vfc_typenum + 1;
	}

	if (newvfstbl->vfc_vfsops->vfs_init) {
		struct vfsconf vfsc;
		bzero(&vfsc, sizeof(struct vfsconf));
		vfsc.vfc_reserved1 = 0;
		bcopy((*handle)->vfc_name, vfsc.vfc_name, sizeof(vfsc.vfc_name));
		vfsc.vfc_typenum = (*handle)->vfc_typenum;
		vfsc.vfc_refcount = (*handle)->vfc_refcount;
		vfsc.vfc_flags = (*handle)->vfc_flags;
		vfsc.vfc_reserved2 = 0;
		vfsc.vfc_reserved3 = 0;

		(*newvfstbl->vfc_vfsops->vfs_init)(&vfsc);
	}

	kheap_free(KHEAP_TEMP, newvfstbl, sizeof(struct vfstable));

	return 0;
}

  
                                      
                                                           
                        
   
errno_t
vfs_fsremove(vfstable_t handle)
{
	struct vfstable * vfstbl =  (struct vfstable *)handle;
	void *old_desc = NULL;
	size_t descsize = 0;
	errno_t err;

	                                    
	mount_list_lock();
	if (vfstbl->vfc_refcount != 0) {
		mount_list_unlock();
		return EBUSY;
	}

	  
                                                                   
                                  
    
	if (vfstbl->vfc_descptr && vfstbl->vfc_descsize) {
		old_desc = vfstbl->vfc_descptr;
		descsize = vfstbl->vfc_descsize;
	}
	err = vfstable_del(vfstbl);

	mount_list_unlock();

	                                                      
	if (err == 0) {
		kheap_free(KHEAP_DEFAULT, old_desc, descsize);
	}

	return err;
}

void
vfs_setowner(mount_t mp, uid_t uid, gid_t gid)
{
	mp->mnt_fsowner = uid;
	mp->mnt_fsgroup = gid;
}

  
                                                         
                                                                     
                                                                      
                  
   
uint64_t
vfs_idle_time(mount_t mp)
{
	if (mp->mnt_pending_write_size) {
		return 0;
	}

	struct timeval now;

	microuptime(&now);

	return (now.tv_sec
	       - mp->mnt_last_write_completed_timestamp.tv_sec) * 1000000
	       + now.tv_usec - mp->mnt_last_write_completed_timestamp.tv_usec;
}

int
vfs_context_pid(vfs_context_t ctx)
{
	return proc_pid(vfs_context_proc(ctx));
}

int
vfs_context_suser(vfs_context_t ctx)
{
	return suser(ctx->vc_ucred, NULL);
}

  
                                                                              
  
                                                                              
            
   
int
vfs_context_issignal(vfs_context_t ctx, sigset_t mask)
{
	proc_t p = vfs_context_proc(ctx);
	if (p) {
		return proc_pendingsignals(p, mask);
	}
	return 0;
}

int
vfs_context_is64bit(vfs_context_t ctx)
{
	proc_t proc = vfs_context_proc(ctx);

	if (proc) {
		return proc_is64bit(proc);
	}
	return 0;
}

boolean_t
vfs_context_can_resolve_triggers(vfs_context_t ctx)
{
	proc_t proc = vfs_context_proc(ctx);

	if (proc) {
		if (proc->p_vfs_iopolicy &
		    P_VFS_IOPOLICY_TRIGGER_RESOLVE_DISABLE) {
			return false;
		}
		return true;
	}
	return false;
}

  
                   
  
                                                                            
  
                                                 
  
                                                  
  
                                                                    
                                  
  
                                          
                                                         
                                                           
                                                      
                                                   
                                
  
                                                           
                                                            
                                                                 
   
proc_t
vfs_context_proc(vfs_context_t ctx)
{
	proc_t  proc = NULL;

	if (ctx != NULL && ctx->vc_thread != NULL) {
		proc = (proc_t)get_bsdthreadtask_info(ctx->vc_thread);
	}
	if (proc != NULL && (proc->p_fd == NULL || (proc->p_lflag & P_LVFORK))) {
		proc = NULL;
	}

	return proc == NULL ? current_proc() : proc;
}

  
                               
  
                                                                          
                                       
  
                                                 
                                            
                                                       
  
                                                       
   
kern_return_t
vfs_context_get_special_port(vfs_context_t ctx, int which, ipc_port_t *portp)
{
	task_t                  task = NULL;

	if (ctx != NULL && ctx->vc_thread != NULL) {
		task = get_threadtask(ctx->vc_thread);
	}

	return task_get_special_port(task, which, portp);
}

  
                               
  
                                                                     
                                       
  
                                                 
                                            
                                             
  
                                                                
   
kern_return_t
vfs_context_set_special_port(vfs_context_t ctx, int which, ipc_port_t port)
{
	task_t                  task = NULL;

	if (ctx != NULL && ctx->vc_thread != NULL) {
		task = get_threadtask(ctx->vc_thread);
	}

	return task_set_special_port_internal(task, which, port);
}

  
                     
  
                                                                      
  
                                                 
  
                                                      
                                  
  
                                                                      
                                                                   
                                                    
  
                                                            
                                  
   
thread_t
vfs_context_thread(vfs_context_t ctx)
{
	return ctx->vc_thread;
}


  
                  
  
                                                                        
                                      
  
                                                 
  
                                                    
                        
  
                                                                     
                                                               
                                                             
                                                 
   
vnode_t
vfs_context_cwd(vfs_context_t ctx)
{
	vnode_t cwd = NULLVP;

	if (ctx != NULL && ctx->vc_thread != NULL) {
		uthread_t uth = get_bsdthread_info(ctx->vc_thread);
		proc_t proc;

		  
                                                            
                               
     
		if ((cwd = uth->uu_cdir) == NULLVP &&
		    (proc = (proc_t)get_bsdthreadtask_info(ctx->vc_thread)) != NULL &&
		    proc->p_fd != NULL) {
			cwd = proc->p_fd->fd_cdir;
		}
	}

	return cwd;
}

  
                     
  
                                                      
  
                                                                                
  
                                      
  
                                                                      
                                                                     
                                                        
   
vfs_context_t
vfs_context_create(vfs_context_t ctx)
{
	vfs_context_t newcontext;

	newcontext = zalloc_flags(ZV_VFS_CONTEXT, Z_WAITOK | Z_ZERO);

	if (newcontext) {
		kauth_cred_t safecred;
		if (ctx) {
			newcontext->vc_thread = ctx->vc_thread;
			safecred = ctx->vc_ucred;
		} else {
			newcontext->vc_thread = current_thread();
			safecred = kauth_cred_get();
		}
		if (IS_VALID_CRED(safecred)) {
			kauth_cred_ref(safecred);
		}
		newcontext->vc_ucred = safecred;
		return newcontext;
	}
	return NULL;
}


vfs_context_t
vfs_context_current(void)
{
	vfs_context_t ctx = NULL;
	uthread_t ut = (uthread_t)get_bsdthread_info(current_thread());

	if (ut != NULL) {
		if (ut->uu_context.vc_ucred != NULL) {
			ctx = &ut->uu_context;
		}
	}

	return ctx == NULL ? vfs_context_kernel() : ctx;
}


  
                 
  
                                                                           
                                                                           
                                                                
                                                                       
                                                                          
                                                                    
  
                                                                             
                                        
  
   
static struct vfs_context kerncontext;
vfs_context_t
vfs_context_kernel(void)
{
	return &kerncontext;
}

  
                                                                             
   
void
vfs_set_context_kernel(vfs_context_t ctx)
{
	kerncontext = *ctx;
}

int
vfs_context_rele(vfs_context_t ctx)
{
	if (ctx) {
		if (IS_VALID_CRED(ctx->vc_ucred)) {
			kauth_cred_unref(&ctx->vc_ucred);
		}
		zfree(ZV_VFS_CONTEXT, ctx);
	}
	return 0;
}


kauth_cred_t
vfs_context_ucred(vfs_context_t ctx)
{
	return ctx->vc_ucred;
}

  
                                                        
   
int
vfs_context_issuser(vfs_context_t ctx)
{
	return kauth_cred_issuser(vfs_context_ucred(ctx));
}

int
vfs_context_iskernel(vfs_context_t ctx)
{
	return ctx == &kerncontext;
}

  
                                                         
                                                         
                                                             
                            
  
                                              
  
                       
                                                    
                                                          
                                                    
  
   
int
vfs_context_bind(vfs_context_t ctx)
{
	ctx->vc_thread = current_thread();
	return 0;
}

int
vfs_set_thread_fs_private(uint8_t tag, uint64_t fs_private)
{
	struct uthread *ut;

	if (tag != FS_PRIVATE_TAG_APFS) {
		return ENOTSUP;
	}

	ut = get_bsdthread_info(current_thread());
	ut->t_fs_private = fs_private;

	return 0;
}

int
vfs_get_thread_fs_private(uint8_t tag, uint64_t *fs_private)
{
	struct uthread *ut;

	if (tag != FS_PRIVATE_TAG_APFS) {
		return ENOTSUP;
	}

	ut = get_bsdthread_info(current_thread());
	*fs_private = ut->t_fs_private;

	return 0;
}

int
vfs_isswapmount(mount_t mnt)
{
	return mnt && ISSET(mnt->mnt_kern_flag, MNTK_SWAP_MOUNT) ? 1 : 0;
}

                                                          


  
                                                               
                                                                  
   
enum vtype
vnode_iftovt(int mode)
{
	return iftovt_tab[((mode) & S_IFMT) >> 12];
}

int
vnode_vttoif(enum vtype indx)
{
	return vttoif_tab[(int)(indx)];
}

int
vnode_makeimode(int indx, int mode)
{
	return (int)(VTTOIF(indx) | (mode));
}


  
                                
   

                                                                                
vnode_t
vfs_rootvnode(void)
{
	int error;

	lck_rw_lock_shared(&rootvnode_rw_lock);
	error = vnode_get(rootvnode);
	lck_rw_unlock_shared(&rootvnode_rw_lock);
	if (error) {
		return (vnode_t)0;
	} else {
		return rootvnode;
	}
}


uint32_t
vnode_vid(vnode_t vp)
{
	return (uint32_t)(vp->v_id);
}

mount_t
vnode_mount(vnode_t vp)
{
	return vp->v_mount;
}

#if CONFIG_IOSCHED
vnode_t
vnode_mountdevvp(vnode_t vp)
{
	if (vp->v_mount) {
		return vp->v_mount->mnt_devvp;
	} else {
		return (vnode_t)0;
	}
}
#endif

boolean_t
vnode_isonexternalstorage(vnode_t vp)
{
	if (vp) {
		if (vp->v_mount) {
			if (vp->v_mount->mnt_ioflags & MNT_IOFLAGS_PERIPHERAL_DRIVE) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

mount_t
vnode_mountedhere(vnode_t vp)
{
	mount_t mp;

	if ((vp->v_type == VDIR) && ((mp = vp->v_mountedhere) != NULL) &&
	    (mp->mnt_vnodecovered == vp)) {
		return mp;
	} else {
		return (mount_t)NULL;
	}
}

                                   
enum vtype
vnode_vtype(vnode_t vp)
{
	return vp->v_type;
}

                                             
void *
vnode_fsnode(vnode_t vp)
{
	return vp->v_data;
}

void
vnode_clearfsnode(vnode_t vp)
{
	vp->v_data = NULL;
}

dev_t
vnode_specrdev(vnode_t vp)
{
	return vp->v_rdev;
}


                        
                             
int
vnode_isvroot(vnode_t vp)
{
	return (vp->v_flag & VROOT)? 1 : 0;
}

                               
int
vnode_issystem(vnode_t vp)
{
	return (vp->v_flag & VSYSTEM)? 1 : 0;
}

                                  
int
vnode_isswap(vnode_t vp)
{
	return (vp->v_flag & VSWAP)? 1 : 0;
}

                      
int
vnode_istty(vnode_t vp)
{
	return (vp->v_flag & VISTTY) ? 1 : 0;
}

                                            
int
vnode_ismount(vnode_t vp)
{
	return (vp->v_flag & VMOUNT)? 1 : 0;
}

                                    
int
vnode_isrecycled(vnode_t vp)
{
	int ret;

	vnode_lock_spin(vp);
	ret =  (vp->v_lflag & (VL_TERMINATE | VL_DEAD))? 1 : 0;
	vnode_unlock(vp);
	return ret;
}

                                                              
                                                         
int
vnode_israge(vnode_t vp)
{
	return (vp->v_flag & VRAGE)? 1 : 0;
}

int
vnode_needssnapshots(vnode_t vp)
{
	return (vp->v_flag & VNEEDSSNAPSHOT)? 1 : 0;
}


                                                                     
int
vfs_ctx_skipatime(vfs_context_t ctx)
{
	struct uthread *ut;
	proc_t proc;
	thread_t thr;

	proc = vfs_context_proc(ctx);
	thr = vfs_context_thread(ctx);

	                                                                    
	if (thr && proc) {
		ut = get_bsdthread_info(thr);

		if (proc->p_lflag & P_LRAGE_VNODES) {
			return 1;
		}

		if (ut) {
			if (ut->uu_flag & (UT_RAGE_VNODES | UT_ATIME_UPDATE)) {
				return 1;
			}
		}

		if (proc->p_vfs_iopolicy & P_VFS_IOPOLICY_ATIME_UPDATES) {
			return 1;
		}
	}
	return 0;
}

                                                                       
int
vnode_isnocache(vnode_t vp)
{
	return (vp->v_flag & VNOCACHE_DATA)? 1 : 0;
}

  
                                                       
   
int
vnode_isnoreadahead(vnode_t vp)
{
	return (vp->v_flag & VRAOFF)? 1 : 0;
}

int
vnode_is_openevt(vnode_t vp)
{
	return (vp->v_flag & VOPENEVT)? 1 : 0;
}

                                
int
vnode_isstandard(vnode_t vp)
{
	return (vp->v_flag & VSTANDARD)? 1 : 0;
}

                                  
int
vnode_isnoflush(vnode_t vp)
{
	return (vp->v_flag & VNOFLUSH)? 1 : 0;
}

                               
int
vnode_isreg(vnode_t vp)
{
	return (vp->v_type == VREG)? 1 : 0;
}

                             
int
vnode_isdir(vnode_t vp)
{
	return (vp->v_type == VDIR)? 1 : 0;
}

                                  
int
vnode_islnk(vnode_t vp)
{
	return (vp->v_type == VLNK)? 1 : 0;
}

int
vnode_lookup_continue_needed(vnode_t vp, struct componentname *cnp)
{
	struct nameidata *ndp = cnp->cn_ndp;

	if (ndp == NULL) {
		panic("vnode_lookup_continue_needed(): cnp->cn_ndp is NULL\n");
	}

	if (vnode_isdir(vp)) {
		if (vp->v_mountedhere != NULL) {
			goto yes;
		}

#if CONFIG_TRIGGERS
		if (vp->v_resolve) {
			goto yes;
		}
#endif                      
	}


	if (vnode_islnk(vp)) {
		                                                                                                   
		if (cnp->cn_flags & FOLLOW) {
			goto yes;
		}
		if (ndp->ni_flag & NAMEI_TRAILINGSLASH) {
			goto yes;
		}
	}

	return 0;

yes:
	ndp->ni_flag |= NAMEI_CONTLOOKUP;
	return EKEEPLOOKING;
}

                         
int
vnode_isfifo(vnode_t vp)
{
	return (vp->v_type == VFIFO)? 1 : 0;
}

                                
int
vnode_isblk(vnode_t vp)
{
	return (vp->v_type == VBLK)? 1 : 0;
}

int
vnode_isspec(vnode_t vp)
{
	return ((vp->v_type == VCHR) || (vp->v_type == VBLK)) ? 1 : 0;
}

                               
int
vnode_ischr(vnode_t vp)
{
	return (vp->v_type == VCHR)? 1 : 0;
}

                          
int
vnode_issock(vnode_t vp)
{
	return (vp->v_type == VSOCK)? 1 : 0;
}

                                                                      
int
vnode_isaliased(vnode_t vp)
{
	enum vtype vt = vp->v_type;
	if (!((vt == VCHR) || (vt == VBLK))) {
		return 0;
	} else {
		return vp->v_specflags & SI_ALIASED;
	}
}

                                
int
vnode_isnamedstream(
#if NAMEDSTREAMS
	vnode_t vp
#else
	__unused vnode_t vp
#endif
	)
{
#if NAMEDSTREAMS
	return (vp->v_flag & VISNAMEDSTREAM) ? 1 : 0;
#else
	return 0;
#endif
}

int
vnode_isshadow(
#if NAMEDSTREAMS
	vnode_t vp
#else
	__unused vnode_t vp
#endif
	)
{
#if NAMEDSTREAMS
	return (vp->v_flag & VISSHADOW) ? 1 : 0;
#else
	return 0;
#endif
}

                                                      
int
vnode_hasnamedstreams(
#if NAMEDSTREAMS
	vnode_t vp
#else
	__unused vnode_t vp
#endif
	)
{
#if NAMEDSTREAMS
	return (vp->v_lflag & VL_HASSTREAMS) ? 1 : 0;
#else
	return 0;
#endif
}
                                                                                   
void
vnode_setnocache(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag |= VNOCACHE_DATA;
	vnode_unlock(vp);
}

void
vnode_clearnocache(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag &= ~VNOCACHE_DATA;
	vnode_unlock(vp);
}

void
vnode_set_openevt(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag |= VOPENEVT;
	vnode_unlock(vp);
}

void
vnode_clear_openevt(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag &= ~VOPENEVT;
	vnode_unlock(vp);
}


void
vnode_setnoreadahead(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag |= VRAOFF;
	vnode_unlock(vp);
}

void
vnode_clearnoreadahead(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag &= ~VRAOFF;
	vnode_unlock(vp);
}

int
vnode_isfastdevicecandidate(vnode_t vp)
{
	return (vp->v_flag & VFASTDEVCANDIDATE)? 1 : 0;
}

void
vnode_setfastdevicecandidate(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag |= VFASTDEVCANDIDATE;
	vnode_unlock(vp);
}

void
vnode_clearfastdevicecandidate(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag &= ~VFASTDEVCANDIDATE;
	vnode_unlock(vp);
}

int
vnode_isautocandidate(vnode_t vp)
{
	return (vp->v_flag & VAUTOCANDIDATE)? 1 : 0;
}

void
vnode_setautocandidate(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag |= VAUTOCANDIDATE;
	vnode_unlock(vp);
}

void
vnode_clearautocandidate(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag &= ~VAUTOCANDIDATE;
	vnode_unlock(vp);
}




                                                 
void
vnode_setnoflush(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag |= VNOFLUSH;
	vnode_unlock(vp);
}

void
vnode_clearnoflush(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_flag &= ~VNOFLUSH;
	vnode_unlock(vp);
}


                                                       
int
vnode_ismountedon(vnode_t vp)
{
	return (vp->v_specflags & SI_MOUNTEDON)? 1 : 0;
}

void
vnode_setmountedon(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_specflags |= SI_MOUNTEDON;
	vnode_unlock(vp);
}

void
vnode_clearmountedon(vnode_t vp)
{
	vnode_lock_spin(vp);
	vp->v_specflags &= ~SI_MOUNTEDON;
	vnode_unlock(vp);
}


void
vnode_settag(vnode_t vp, int tag)
{
	  
                                                                          
                                                     
    
	assert(tag >= SHRT_MIN && tag <= SHRT_MAX);
	vp->v_tag = (uint16_t)tag;
}

int
vnode_tag(vnode_t vp)
{
	return vp->v_tag;
}

vnode_t
vnode_parent(vnode_t vp)
{
	return vp->v_parent;
}

void
vnode_setparent(vnode_t vp, vnode_t dvp)
{
	vp->v_parent = dvp;
}

void
vnode_setname(vnode_t vp, char * name)
{
	vp->v_name = name;
}

                                                                 
void
vnode_vfsname(vnode_t vp, char * buf)
{
	strlcpy(buf, vp->v_mount->mnt_vtable->vfc_name, MFSNAMELEN);
}

                               
int
vnode_vfstypenum(vnode_t vp)
{
	return vp->v_mount->mnt_vtable->vfc_typenum;
}

int
vnode_vfs64bitready(vnode_t vp)
{
	  
                                                                                         
    
	if ((vp->v_mount != dead_mountp) && (vp->v_mount->mnt_vtable->vfc_vfsflags & VFC_VFS64BITREADY)) {
		return 1;
	} else {
		return 0;
	}
}



                                                                   
uint32_t
vnode_vfsvisflags(vnode_t vp)
{
	return vp->v_mount->mnt_flag & MNT_VISFLAGMASK;
}

                                                                            
uint32_t
vnode_vfscmdflags(vnode_t vp)
{
	return vp->v_mount->mnt_flag & MNT_CMDFLAGS;
}

                                                       
uint32_t
vnode_vfsmaxsymlen(vnode_t vp)
{
	return vp->v_mount->mnt_maxsymlinklen;
}

                                                                                 
struct vfsstatfs *
vnode_vfsstatfs(vnode_t vp)
{
	return &vp->v_mount->mnt_vfsstat;
}

                                                                                              
void *
vnode_vfsfsprivate(vnode_t vp)
{
	return vp->v_mount->mnt_data;
}

                                        
int
vnode_vfsisrdonly(vnode_t vp)
{
	return (vp->v_mount->mnt_flag & MNT_RDONLY)? 1 : 0;
}

int
vnode_compound_rename_available(vnode_t vp)
{
	return vnode_compound_op_available(vp, COMPOUND_VNOP_RENAME);
}
int
vnode_compound_rmdir_available(vnode_t vp)
{
	return vnode_compound_op_available(vp, COMPOUND_VNOP_RMDIR);
}
int
vnode_compound_mkdir_available(vnode_t vp)
{
	return vnode_compound_op_available(vp, COMPOUND_VNOP_MKDIR);
}
int
vnode_compound_remove_available(vnode_t vp)
{
	return vnode_compound_op_available(vp, COMPOUND_VNOP_REMOVE);
}
int
vnode_compound_open_available(vnode_t vp)
{
	return vnode_compound_op_available(vp, COMPOUND_VNOP_OPEN);
}

int
vnode_compound_op_available(vnode_t vp, compound_vnop_id_t opid)
{
	return (vp->v_mount->mnt_compound_ops & opid) != 0;
}

  
                                                                          
                                                                              
  
                               
   
vnode_t
current_workingdir(void)
{
	return vfs_context_cwd(vfs_context_current());
}

  
                                                                      
                                                                           
                                                            
  
                                                  
                                               
                 
                                        
                                    
  
                       
                    
  
                                                                   
                                                         
                                                                 
                                                                  
                                                                  
                                                              
                                                     
                         
  
                                                                
                                                               
  
                                                                       
                                                                
                                                                 
  
                                                                       
                                
   
static int
vnode_get_filesec(vnode_t vp, kauth_filesec_t *fsecp, vfs_context_t ctx)
{
	kauth_filesec_t fsec;
	uio_t   fsec_uio;
	size_t  fsec_size;
	size_t  xsize, rsize;
	int     error;
	uint32_t        host_fsec_magic;
	uint32_t        host_acl_entrycount;

	fsec = NULL;
	fsec_uio = NULL;

	                                
	error = vn_getxattr(vp, KAUTH_FILESEC_XATTR, NULL, &xsize, XATTR_NOSECURITY, ctx);
	if (error != 0) {
		                       
		if ((error == ENOATTR) || (error == ENOENT) || (error == EJUSTRETURN)) {
			error = 0;
		}
		                             
		goto out;
	}

	  
                                                                      
                                                                        
                                                                   
                                                          
    
	if (!KAUTH_FILESEC_VALID(xsize)) {
		KAUTH_DEBUG("    ERROR - Bogus kauth_fiilesec_t: %ld bytes", xsize);
		error = 0;
		goto out;
	}

	                                 
	fsec_size = KAUTH_FILESEC_COUNT(xsize);
	if (fsec_size > KAUTH_ACL_MAX_ENTRIES) {
		KAUTH_DEBUG("    ERROR - Bogus (too large) kauth_fiilesec_t: %ld bytes", xsize);
		error = 0;
		goto out;
	}

	                        
	if (((fsec = kauth_filesec_alloc((int)fsec_size)) == NULL) ||
	    ((fsec_uio = uio_create(1, 0, UIO_SYSSPACE, UIO_READ)) == NULL) ||
	    uio_addiov(fsec_uio, CAST_USER_ADDR_T(fsec), xsize)) {
		KAUTH_DEBUG("    ERROR - could not allocate iov to read ACL");
		error = ENOMEM;
		goto out;
	}

	                             
	rsize = xsize;
	if ((error = vn_getxattr(vp,
	    KAUTH_FILESEC_XATTR,
	    fsec_uio,
	    &rsize,
	    XATTR_NOSECURITY,
	    ctx)) != 0) {
		                                     
		if ((error == ENOATTR) || (error == ENOENT) || (error == EJUSTRETURN)) {
			error = 0;
		}
		                             
		goto out;
	}

	  
                                                                       
                                                         
    

	                                                   
	if (rsize < KAUTH_FILESEC_SIZE(0)) {
		KAUTH_DEBUG("ACL - DATA TOO SMALL (%d)", rsize);
		goto out;
	}

	                                                           
	host_fsec_magic = ntohl(KAUTH_FILESEC_MAGIC);
	if (fsec->fsec_magic != host_fsec_magic) {
		KAUTH_DEBUG("ACL - BAD MAGIC %x", host_fsec_magic);
		goto out;
	}

	                                                           
	host_acl_entrycount = ntohl(fsec->fsec_acl.acl_entrycount);
	if (host_acl_entrycount != KAUTH_FILESEC_NOACL) {
		if (host_acl_entrycount > KAUTH_ACL_MAX_ENTRIES) {
			KAUTH_DEBUG("ACL - BAD ENTRYCOUNT %x", host_acl_entrycount);
			goto out;
		}
		if (KAUTH_FILESEC_SIZE(host_acl_entrycount) > rsize) {
			KAUTH_DEBUG("ACL - BUFFER OVERFLOW (%d entries too big for %d)", host_acl_entrycount, rsize);
			goto out;
		}
	}

	kauth_filesec_acl_setendian(KAUTH_ENDIAN_HOST, fsec, NULL);

	*fsecp = fsec;
	fsec = NULL;
	error = 0;
out:
	if (fsec != NULL) {
		kauth_filesec_free(fsec);
	}
	if (fsec_uio != NULL) {
		uio_free(fsec_uio);
	}
	if (error) {
		*fsecp = NULL;
	}
	return error;
}

  
                                                                      
                                                                         
                                                                        
                                                                         
                                   
  
                                                  
                                 
                                               
                                        
                                    
  
                       
                    
  
                                                     
  
                                                                 
                                                                   
                                                                  
                                                             
                                                                  
                                                                 
                                                                 
                                                                
                                    
  
                                                                       
                                
   
static int
vnode_set_filesec(vnode_t vp, kauth_filesec_t fsec, kauth_acl_t acl, vfs_context_t ctx)
{
	uio_t           fsec_uio;
	int             error;
	uint32_t        saved_acl_copysize;

	fsec_uio = NULL;

	if ((fsec_uio = uio_create(2, 0, UIO_SYSSPACE, UIO_WRITE)) == NULL) {
		KAUTH_DEBUG("    ERROR - could not allocate iov to write ACL");
		error = ENOMEM;
		goto out;
	}
	  
                                                                    
                                                
    
	saved_acl_copysize = KAUTH_ACL_COPYSIZE(acl);

	kauth_filesec_acl_setendian(KAUTH_ENDIAN_DISK, fsec, acl);

	uio_addiov(fsec_uio, CAST_USER_ADDR_T(fsec), KAUTH_FILESEC_SIZE(0) - KAUTH_ACL_SIZE(KAUTH_FILESEC_NOACL));
	uio_addiov(fsec_uio, CAST_USER_ADDR_T(acl), saved_acl_copysize);
	error = vn_setxattr(vp,
	    KAUTH_FILESEC_XATTR,
	    fsec_uio,
	    XATTR_NOSECURITY,                                        
	    ctx);
	VFS_DEBUG(ctx, vp, "SETATTR - set ACL returning %d", error);

	kauth_filesec_acl_setendian(KAUTH_ENDIAN_HOST, fsec, acl);

out:
	if (fsec_uio != NULL) {
		uio_free(fsec_uio);
	}
	return error;
}

  
                                                      
   
void
vnode_attr_handle_mnt_ignore_ownership(struct vnode_attr *vap, mount_t mp, vfs_context_t ctx)
{
	uid_t   nuid;
	gid_t   ngid;

	if (VATTR_IS_ACTIVE(vap, va_uid)) {
		if (vfs_context_issuser(ctx) && VATTR_IS_SUPPORTED(vap, va_uid)) {
			nuid = vap->va_uid;
		} else if (mp->mnt_flag & MNT_IGNORE_OWNERSHIP) {
			nuid = mp->mnt_fsowner;
			if (nuid == KAUTH_UID_NONE) {
				nuid = 99;
			}
		} else if (VATTR_IS_SUPPORTED(vap, va_uid)) {
			nuid = vap->va_uid;
		} else {
			                                            
			nuid = mp->mnt_fsowner;
		}
		if ((nuid == 99) && !vfs_context_issuser(ctx)) {
			nuid = kauth_cred_getuid(vfs_context_ucred(ctx));
		}
		VATTR_RETURN(vap, va_uid, nuid);
	}
	if (VATTR_IS_ACTIVE(vap, va_gid)) {
		if (vfs_context_issuser(ctx) && VATTR_IS_SUPPORTED(vap, va_gid)) {
			ngid = vap->va_gid;
		} else if (mp->mnt_flag & MNT_IGNORE_OWNERSHIP) {
			ngid = mp->mnt_fsgroup;
			if (ngid == KAUTH_GID_NONE) {
				ngid = 99;
			}
		} else if (VATTR_IS_SUPPORTED(vap, va_gid)) {
			ngid = vap->va_gid;
		} else {
			                                            
			ngid = mp->mnt_fsgroup;
		}
		if ((ngid == 99) && !vfs_context_issuser(ctx)) {
			ngid = kauth_cred_getgid(vfs_context_ucred(ctx));
		}
		VATTR_RETURN(vap, va_gid, ngid);
	}
}

  
                       
                                                   
                                         
                      
                          
                            
                            
                           
   
int
vnode_getattr(vnode_t vp, struct vnode_attr *vap, vfs_context_t ctx)
{
	kauth_filesec_t fsec;
	kauth_acl_t facl;
	int     error;

	  
                                                
    
	if (vap->va_active & ~VNODE_ATTR_ALL) {
		return EINVAL;
	}

	                                                                               
	if (!vfs_extendedsecurity(vnode_mount(vp))) {
		VATTR_CLEAR_ACTIVE(vap, va_acl);
		VATTR_CLEAR_ACTIVE(vap, va_uuuid);
		VATTR_CLEAR_ACTIVE(vap, va_guuid);
	}

	  
                                                                         
                                                                     
    
	if (VATTR_IS_ACTIVE(vap, va_data_alloc) ||
	    VATTR_IS_ACTIVE(vap, va_total_size) ||
	    VATTR_IS_ACTIVE(vap, va_total_alloc)) {
		VATTR_SET_ACTIVE(vap, va_data_size);
		VATTR_SET_ACTIVE(vap, va_data_alloc);
		VATTR_SET_ACTIVE(vap, va_total_size);
		VATTR_SET_ACTIVE(vap, va_total_alloc);
	}

	vap->va_vaflags &= ~VA_USEFSID;

	error = VNOP_GETATTR(vp, vap, ctx);
	if (error) {
		KAUTH_DEBUG("ERROR - returning %d", error);
		goto out;
	}

	  
                                                                              
         
    
	if (VATTR_NOT_RETURNED(vap, va_acl) || VATTR_NOT_RETURNED(vap, va_uuuid) || VATTR_NOT_RETURNED(vap, va_guuid)) {
		fsec = NULL;

		if (XATTR_VNODE_SUPPORTED(vp)) {
			                            
			if ((error = vnode_get_filesec(vp, &fsec, ctx)) != 0) {
				goto out;
			}
		}
		                                  
		if (fsec == NULL) {
			VATTR_RETURN(vap, va_acl, NULL);
			VATTR_RETURN(vap, va_uuuid, kauth_null_guid);
			VATTR_RETURN(vap, va_guuid, kauth_null_guid);
		} else {
			                                                      
			VATTR_RETURN(vap, va_uuuid, fsec->fsec_owner);
			VATTR_RETURN(vap, va_guuid, fsec->fsec_group);

			                                                          
			if (VATTR_IS_ACTIVE(vap, va_acl)) {
				if (fsec->fsec_acl.acl_entrycount == KAUTH_FILESEC_NOACL) {
					VATTR_RETURN(vap, va_acl, NULL);
				} else {
					facl = kauth_acl_alloc(fsec->fsec_acl.acl_entrycount);
					if (facl == NULL) {
						kauth_filesec_free(fsec);
						error = ENOMEM;
						goto out;
					}
					__nochk_bcopy(&fsec->fsec_acl, facl, KAUTH_ACL_COPYSIZE(&fsec->fsec_acl));
					VATTR_RETURN(vap, va_acl, facl);
				}
			}
			kauth_filesec_free(fsec);
		}
	}
	  
                                                                        
                                                                       
                                    
    
	if (VATTR_IS_SUPPORTED(vap, va_acl) && !VATTR_IS_ACTIVE(vap, va_acl)) {
		if (vap->va_acl != NULL) {
			kauth_acl_free(vap->va_acl);
		}
		VATTR_CLEAR_SUPPORTED(vap, va_acl);
	}

#if 0                                                               
	  
                                                                   
                         
    
	if (VATTR_NOT_RETURNED(vap, va_uid) &&
	    VATTR_IS_SUPPORTED(vap, va_uuuid) &&
	    !kauth_guid_equal(&vap->va_uuuid, &kauth_null_guid)) {
		if ((error = kauth_cred_guid2uid(&vap->va_uuuid, &nuid)) == 0) {
			VATTR_RETURN(vap, va_uid, nuid);
		}
	}
	if (VATTR_NOT_RETURNED(vap, va_gid) &&
	    VATTR_IS_SUPPORTED(vap, va_guuid) &&
	    !kauth_guid_equal(&vap->va_guuid, &kauth_null_guid)) {
		if ((error = kauth_cred_guid2gid(&vap->va_guuid, &ngid)) == 0) {
			VATTR_RETURN(vap, va_gid, ngid);
		}
	}
#endif

	vnode_attr_handle_mnt_ignore_ownership(vap, vp->v_mount, ctx);

	  
                                                          
    
	if (!VATTR_IS_SUPPORTED(vap, va_iosize)) {
		assert(vp->v_mount->mnt_vfsstat.f_iosize <= UINT32_MAX);
		VATTR_RETURN(vap, va_iosize, (uint32_t)vp->v_mount->mnt_vfsstat.f_iosize);
	}

	if (!VATTR_IS_SUPPORTED(vap, va_flags)) {
		VATTR_RETURN(vap, va_flags, 0);
	}

	if (!VATTR_IS_SUPPORTED(vap, va_filerev)) {
		VATTR_RETURN(vap, va_filerev, 0);
	}

	if (!VATTR_IS_SUPPORTED(vap, va_gen)) {
		VATTR_RETURN(vap, va_gen, 0);
	}

	  
                                                                                        
    
	if (!VATTR_IS_SUPPORTED(vap, va_data_size)) {
		VATTR_RETURN(vap, va_data_size, 0);
	}

	                                                     
	if (VATTR_IS_ACTIVE(vap, va_data_alloc) ||
	    VATTR_IS_ACTIVE(vap, va_total_size) ||
	    VATTR_IS_ACTIVE(vap, va_total_alloc)) {
		                                
		if (vp->v_mount->mnt_vfsstat.f_bsize == 0) {
			if ((error = vfs_update_vfsstat(vp->v_mount, ctx, VFS_KERNEL_EVENT)) != 0) {
				goto out;
			}
		}

		                                             
		if (!VATTR_IS_SUPPORTED(vap, va_data_alloc)) {
			VATTR_RETURN(vap, va_data_alloc, roundup(vap->va_data_size, vp->v_mount->mnt_vfsstat.f_bsize));
		}

		                                             
		if (!VATTR_IS_SUPPORTED(vap, va_total_size)) {
			VATTR_RETURN(vap, va_total_size, vap->va_data_size);
		}

		                                                                                 
		if (!VATTR_IS_SUPPORTED(vap, va_total_alloc)) {
			VATTR_RETURN(vap, va_total_alloc, roundup(vap->va_total_size, vp->v_mount->mnt_vfsstat.f_bsize));
		}
	}

	  
                                                             
    
	if (!VATTR_IS_SUPPORTED(vap, va_change_time) && VATTR_IS_SUPPORTED(vap, va_modify_time)) {
		VATTR_RETURN(vap, va_change_time, vap->va_modify_time);
	}

	  
                                                                                      
                                    
    
	VATTR_RETURN(vap, va_type, vp->v_type);

	  
                                                          
    
	if (VATTR_IS_ACTIVE(vap, va_fsid) &&
	    (!VATTR_IS_SUPPORTED(vap, va_fsid) ||
	    vap->va_vaflags & VA_REALFSID || !(vap->va_vaflags & VA_USEFSID))) {
		VATTR_RETURN(vap, va_fsid, vp->v_mount->mnt_vfsstat.f_fsid.val[0]);
	}

out:
	vap->va_vaflags &= ~VA_USEFSID;

	return error;
}

  
                               
   
uint64_t
vnode_get_va_fsid(struct vnode_attr *vap)
{
	if (VATTR_IS_SUPPORTED(vap, va_fsid64)) {
		return (uint64_t)vap->va_fsid64.val[0] + ((uint64_t)vap->va_fsid64.val[1] << 32);
	}
	return vap->va_fsid;
}

  
                                                    
  
                                                      
                                             
                                        
                                    
  
                       
                    
  
                                                                      
  
                                                            
                                                           
                                                            
                                                              
                                                             
  
                                                                       
                                
   
int
vnode_setattr(vnode_t vp, struct vnode_attr *vap, vfs_context_t ctx)
{
	int     error;
#if CONFIG_FSE
	uint64_t active;
	int     is_perm_change = 0;
	int     is_stat_change = 0;
#endif

	  
                                              
    
	if (vap->va_active & ~VNODE_ATTR_ALL) {
		return EINVAL;
	}

	  
                                            
                            
    
	if (vfs_isrdonly(vp->v_mount)) {
		error = EROFS;
		goto out;
	}

#if DEVELOPMENT || DEBUG
	  
                                                          
                                            
    
#else                           

	if (vnode_isswap(vp) && (ctx != vfs_context_kernel())) {
		error = EPERM;
		goto out;
	}
#endif                           

#if NAMEDSTREAMS
	                                                              
	if ((vp->v_flag & VISNAMEDSTREAM) && (vap->va_active != VNODE_ATTR_va_data_size)) {
		error = EPERM;
		goto out;
	}
#endif
	                          
	if (VATTR_IS_ACTIVE(vap, va_data_size)) {
		switch (vp->v_type) {
		case VREG:
			                               
			break;
		case VDIR:
			                                         
			error = EISDIR;
			goto out;
		default:
			                                                                                        
			VATTR_CLEAR_ACTIVE(vap, va_data_size);
			if (vap->va_active) {
				break;
			}
			                                                                                            
			return 0;
		}
	}

	  
                                                                     
                      
    
	if (vp->v_mount->mnt_flag & MNT_IGNORE_OWNERSHIP) {
		VATTR_CLEAR_ACTIVE(vap, va_uid);
		VATTR_CLEAR_ACTIVE(vap, va_gid);
	}

	  
                                                                     
               
    
	if (!vfs_extendedsecurity(vnode_mount(vp)) &&
	    (VATTR_IS_ACTIVE(vap, va_acl) || VATTR_IS_ACTIVE(vap, va_uuuid) || VATTR_IS_ACTIVE(vap, va_guuid))) {
		KAUTH_DEBUG("SETATTR - returning ENOTSUP to request to set extended security");
		error = ENOTSUP;
		goto out;
	}

	                                                                 
	if (VATTR_IS_ACTIVE(vap, va_flags)) {
		vap->va_flags &= (SF_SUPPORTED | UF_SETTABLE);
	}

#if CONFIG_FSE
	  
                                                    
                         
    
	active = vap->va_active & ~VNODE_ATTR_RDONLY;
#endif

	error = VNOP_SETATTR(vp, vap, ctx);

	if ((error == 0) && !VATTR_ALL_SUPPORTED(vap)) {
		error = vnode_setattr_fallback(vp, vap, ctx);
	}

#if CONFIG_FSE
#define PERMISSION_BITS (VNODE_ATTR_BIT(va_uid) | VNODE_ATTR_BIT(va_uuuid) | \
	                 VNODE_ATTR_BIT(va_gid) | VNODE_ATTR_BIT(va_guuid) | \
	                 VNODE_ATTR_BIT(va_mode) | VNODE_ATTR_BIT(va_acl))

	  
                                                          
            
    
	if ((active & PERMISSION_BITS) & vap->va_supported) {
		is_perm_change = 1;
	} else {
		  
                                                      
                                                      
             
     
		active &= ~(PERMISSION_BITS |
		    VNODE_ATTR_BIT(va_access_time) |
		    VNODE_ATTR_BIT(va_backup_time));

		                                    
		if (active & vap->va_supported) {
			is_stat_change = 1;
		}
	}

	if (error == 0) {
		if (is_perm_change) {
			if (need_fsevent(FSE_CHOWN, vp)) {
				add_fsevent(FSE_CHOWN, ctx, FSE_ARG_VNODE, vp, FSE_ARG_DONE);
			}
		} else if (is_stat_change && need_fsevent(FSE_STAT_CHANGED, vp)) {
			add_fsevent(FSE_STAT_CHANGED, ctx, FSE_ARG_VNODE, vp, FSE_ARG_DONE);
		}
	}
#undef PERMISSION_BITS
#endif

out:
	return error;
}

  
                                                                           
                                                                           
                                                                          
              
  
                                                      
                                             
                                        
                                    
  
                       
                    
  
                                                                      
                                                
  
                                                            
                                                            
                                  
  
                                                                       
                                
   
int
vnode_setattr_fallback(vnode_t vp, struct vnode_attr *vap, vfs_context_t ctx)
{
	kauth_filesec_t fsec;
	kauth_acl_t facl;
	struct kauth_filesec lfsec;
	int     error;

	error = 0;

	  
                                                       
   
                                                                   
            
    
	if (VATTR_NOT_RETURNED(vap, va_acl) ||
	    VATTR_NOT_RETURNED(vap, va_uuuid) ||
	    VATTR_NOT_RETURNED(vap, va_guuid)) {
		VFS_DEBUG(ctx, vp, "SETATTR - doing filesec fallback");

		  
                                                               
                  
     
		if (!XATTR_VNODE_SUPPORTED(vp)) {
			VFS_DEBUG(ctx, vp, "SETATTR - Can't write ACL to file type %d", vnode_vtype(vp));
			error = EINVAL;
			goto out;
		}

		  
                                                              
                                                              
               
     
		fsec = NULL;
		if (!VATTR_IS_ACTIVE(vap, va_acl) ||
		    !VATTR_IS_ACTIVE(vap, va_uuuid) ||
		    !VATTR_IS_ACTIVE(vap, va_guuid)) {
			if ((error = vnode_get_filesec(vp, &fsec, ctx)) != 0) {
				KAUTH_DEBUG("SETATTR - ERROR %d fetching filesec for update", error);
				goto out;
			}
		}
		                                                   
		if (fsec == NULL) {
			KAUTH_DEBUG("SETATTR - using local filesec for new/full update");
			fsec = &lfsec;
		} else {
			KAUTH_DEBUG("SETATTR - updating existing filesec");
		}
		                  
		facl = &fsec->fsec_acl;

		                                                                
		if (fsec == &lfsec) {
			fsec->fsec_magic = KAUTH_FILESEC_MAGIC;
			fsec->fsec_owner = kauth_null_guid;
			fsec->fsec_group = kauth_null_guid;
			facl->acl_entrycount = KAUTH_FILESEC_NOACL;
			facl->acl_flags = 0;
		}

		  
                                         
     
		if (VATTR_IS_ACTIVE(vap, va_uuuid)) {
			KAUTH_DEBUG("SETATTR - updating owner UUID");
			fsec->fsec_owner = vap->va_uuuid;
			VATTR_SET_SUPPORTED(vap, va_uuuid);
		}
		if (VATTR_IS_ACTIVE(vap, va_guuid)) {
			KAUTH_DEBUG("SETATTR - updating group UUID");
			fsec->fsec_group = vap->va_guuid;
			VATTR_SET_SUPPORTED(vap, va_guuid);
		}
		if (VATTR_IS_ACTIVE(vap, va_acl)) {
			if (vap->va_acl == NULL) {
				KAUTH_DEBUG("SETATTR - removing ACL");
				facl->acl_entrycount = KAUTH_FILESEC_NOACL;
			} else {
				KAUTH_DEBUG("SETATTR - setting ACL with %d entries", vap->va_acl->acl_entrycount);
				facl = vap->va_acl;
			}
			VATTR_SET_SUPPORTED(vap, va_acl);
		}

		  
                                                           
                       
     
		if ((facl->acl_entrycount == KAUTH_FILESEC_NOACL) &&
		    kauth_guid_equal(&fsec->fsec_owner, &kauth_null_guid) &&
		    kauth_guid_equal(&fsec->fsec_group, &kauth_null_guid)) {
			error = vn_removexattr(vp, KAUTH_FILESEC_XATTR, XATTR_NOSECURITY, ctx);
			                                           
			if (error == ENOATTR) {
				error = 0;
			}
			VFS_DEBUG(ctx, vp, "SETATTR - remove filesec returning %d", error);
		} else {
			                  
			error = vnode_set_filesec(vp, fsec, facl, ctx);
			VFS_DEBUG(ctx, vp, "SETATTR - update filesec returning %d", error);
		}

		                                                    
		if (fsec != &lfsec) {
			kauth_filesec_free(fsec);
		}
	}
out:

	return error;
}

  
                                                                 
                    
   
int
vnode_notify(vnode_t vp, uint32_t events, struct vnode_attr *vap)
{
	                                                                                           
	uint32_t knote_mask = (VNODE_EVENT_WRITE | VNODE_EVENT_DELETE | VNODE_EVENT_RENAME
	    | VNODE_EVENT_LINK | VNODE_EVENT_EXTEND | VNODE_EVENT_ATTRIB);
	uint32_t dir_contents_mask = (VNODE_EVENT_DIR_CREATED | VNODE_EVENT_FILE_CREATED
	    | VNODE_EVENT_DIR_REMOVED | VNODE_EVENT_FILE_REMOVED);
	uint32_t knote_events = (events & knote_mask);

	                                                             
	if (events & VNODE_EVENT_PERMS) {
		knote_events |= NOTE_ATTRIB;
	}

	                                                            
	if ((vnode_isdir(vp)) && (events & dir_contents_mask)) {
		knote_events |= NOTE_WRITE;
	}

	if (knote_events) {
		lock_vnode_and_post(vp, knote_events);
#if CONFIG_FSE
		if (vap != NULL) {
			create_fsevent_from_kevent(vp, events, vap);
		}
#else
		(void)vap;
#endif
	}

	return 0;
}



int
vnode_isdyldsharedcache(vnode_t vp)
{
	return (vp->v_flag & VSHARED_DYLD) ? 1 : 0;
}


  
                                                               
                                            
   
int
vnode_ismonitored(vnode_t vp)
{
	return vp->v_knotes.slh_first != NULL;
}

int
vnode_getbackingvnode(vnode_t in_vp, vnode_t* out_vpp)
{
	if (out_vpp) {
		*out_vpp = NULLVP;
	}
#if NULLFS
	return nullfs_getbackingvnode(in_vp, out_vpp);
#else
#pragma unused(in_vp)
	return ENOENT;
#endif
}

  
                                                                      
                              
   
int
vfs_get_notify_attributes(struct vnode_attr *vap)
{
	VATTR_INIT(vap);
	vap->va_active = VNODE_NOTIFY_ATTRS;
	return 0;
}

#if CONFIG_TRIGGERS
int
vfs_settriggercallback(fsid_t *fsid, vfs_trigger_callback_t vtc, void *data, uint32_t flags __unused, vfs_context_t ctx)
{
	int error;
	mount_t mp;

	mp = mount_list_lookupby_fsid(fsid, 0             , 1              );
	if (mp == NULL) {
		return ENOENT;
	}

	error = vfs_busy(mp, LK_NOWAIT);
	mount_iterdrop(mp);

	if (error != 0) {
		return ENOENT;
	}

	mount_lock(mp);
	if (mp->mnt_triggercallback != NULL) {
		error = EBUSY;
		mount_unlock(mp);
		goto out;
	}

	mp->mnt_triggercallback = vtc;
	mp->mnt_triggerdata = data;
	mount_unlock(mp);

	mp->mnt_triggercallback(mp, VTC_REPLACE, data, ctx);

out:
	vfs_unbusy(mp);
	return 0;
}
#endif                      

  
                                   
   

#if 0
  
  
                              
                              
  
struct vnop_lookup_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	vfs_context_t a_context;
};
#endif       

  
                       
                                                              
                                          
                                            
                            
                      
                           
                     
                      
                       
                   
  
                                                                        
                                                                   
                                                               
                                                                    
  
                                                                   
                                                                
                                                                
   
errno_t
VNOP_LOOKUP(vnode_t dvp, vnode_t *vpp, struct componentname *cnp, vfs_context_t ctx)
{
	int _err;
	struct vnop_lookup_args a;

	a.a_desc = &vnop_lookup_desc;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_context = ctx;

	_err = (*dvp->v_op[vnop_lookup_desc.vdesc_offset])(&a);
	if (_err == 0 && *vpp) {
		DTRACE_FSINFO(lookup, vnode_t, *vpp);
	}

	return _err;
}

#if 0
struct vnop_compound_open_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	int32_t a_flags;
	int32_t a_fmode;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
	void *a_reserved;
};
#endif        

int
VNOP_COMPOUND_OPEN(vnode_t dvp, vnode_t *vpp, struct nameidata *ndp, int32_t flags, int32_t fmode, uint32_t *statusp, struct vnode_attr *vap, vfs_context_t ctx)
{
	int _err;
	struct vnop_compound_open_args a;
	int did_create = 0;
	int want_create;
	uint32_t tmp_status = 0;
	struct componentname *cnp = &ndp->ni_cnd;

	want_create = (flags & O_CREAT);

	a.a_desc = &vnop_compound_open_desc;
	a.a_dvp = dvp;
	a.a_vpp = vpp;                    
	a.a_cnp = cnp;
	a.a_flags = flags;
	a.a_fmode = fmode;
	a.a_status = (statusp != NULL) ? statusp : &tmp_status;
	a.a_vap = vap;
	a.a_context = ctx;
	a.a_open_create_authorizer = vn_authorize_create;
	a.a_open_existing_authorizer = vn_authorize_open_existing;
	a.a_reserved = NULL;

	if (dvp == NULLVP) {
		panic("No dvp?");
	}
	if (want_create && !vap) {
		panic("Want create, but no vap?");
	}
	if (!want_create && vap) {
		panic("Don't want create, but have a vap?");
	}

	_err = (*dvp->v_op[vnop_compound_open_desc.vdesc_offset])(&a);
	if (want_create) {
		if (_err == 0 && *vpp) {
			DTRACE_FSINFO(compound_open, vnode_t, *vpp);
		} else {
			DTRACE_FSINFO(compound_open, vnode_t, dvp);
		}
	} else {
		DTRACE_FSINFO(compound_open, vnode_t, *vpp);
	}

	did_create = (*a.a_status & COMPOUND_OPEN_STATUS_DID_CREATE);

	if (did_create && !want_create) {
		panic("Filesystem did a create, even though none was requested?");
	}

	if (did_create) {
#if CONFIG_APPLEDOUBLE
		if (!NATIVE_XATTR(dvp)) {
			  
                                              
      
			xattrfile_remove(dvp, cnp->cn_nameptr, ctx, 0);
		}
#endif                         
		                                            
		post_event_if_success(dvp, _err, NOTE_WRITE);
	}

	lookup_compound_vnop_post_hook(_err, dvp, *vpp, ndp, did_create);
#if 0                  
	if (*vpp && _err && _err != EKEEPLOOKING) {
		vnode_put(*vpp);
		*vpp = NULLVP;
	}
#endif        

	return _err;
}

#if 0
struct vnop_create_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_CREATE(vnode_t dvp, vnode_t * vpp, struct componentname * cnp, struct vnode_attr * vap, vfs_context_t ctx)
{
	int _err;
	struct vnop_create_args a;

	a.a_desc = &vnop_create_desc;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;
	a.a_context = ctx;

	_err = (*dvp->v_op[vnop_create_desc.vdesc_offset])(&a);
	if (_err == 0 && *vpp) {
		DTRACE_FSINFO(create, vnode_t, *vpp);
	}

#if CONFIG_APPLEDOUBLE
	if (_err == 0 && !NATIVE_XATTR(dvp)) {
		  
                                             
     
		xattrfile_remove(dvp, cnp->cn_nameptr, ctx, 0);
	}
#endif                         

	post_event_if_success(dvp, _err, NOTE_WRITE);

	return _err;
}

#if 0
  
  
                              
                              
                              
  
  
struct vnop_whiteout_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	struct componentname *a_cnp;
	int a_flags;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_WHITEOUT(__unused vnode_t dvp, __unused struct componentname *cnp,
    __unused int flags, __unused vfs_context_t ctx)
{
	return ENOTSUP;                      
}

#if 0
  
  
                              
                              
  
  
struct vnop_mknod_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_MKNOD(vnode_t dvp, vnode_t * vpp, struct componentname * cnp, struct vnode_attr * vap, vfs_context_t ctx)
{
	int _err;
	struct vnop_mknod_args a;

	a.a_desc = &vnop_mknod_desc;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;
	a.a_context = ctx;

	_err = (*dvp->v_op[vnop_mknod_desc.vdesc_offset])(&a);
	if (_err == 0 && *vpp) {
		DTRACE_FSINFO(mknod, vnode_t, *vpp);
	}

	post_event_if_success(dvp, _err, NOTE_WRITE);

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_open_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_mode;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_OPEN(vnode_t vp, int mode, vfs_context_t ctx)
{
	int _err;
	struct vnop_open_args a;

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}
	a.a_desc = &vnop_open_desc;
	a.a_vp = vp;
	a.a_mode = mode;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_open_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(open, vnode_t, vp);

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_close_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_fflag;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_CLOSE(vnode_t vp, int fflag, vfs_context_t ctx)
{
	int _err;
	struct vnop_close_args a;

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}
	a.a_desc = &vnop_close_desc;
	a.a_vp = vp;
	a.a_fflag = fflag;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_close_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(close, vnode_t, vp);

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_access_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_action;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_ACCESS(vnode_t vp, int action, vfs_context_t ctx)
{
	int _err;
	struct vnop_access_args a;

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}
	a.a_desc = &vnop_access_desc;
	a.a_vp = vp;
	a.a_action = action;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_access_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(access, vnode_t, vp);

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_getattr_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_GETATTR(vnode_t vp, struct vnode_attr * vap, vfs_context_t ctx)
{
	int _err;
	struct vnop_getattr_args a;

	a.a_desc = &vnop_getattr_desc;
	a.a_vp = vp;
	a.a_vap = vap;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_getattr_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(getattr, vnode_t, vp);

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_setattr_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_SETATTR(vnode_t vp, struct vnode_attr * vap, vfs_context_t ctx)
{
	int _err;
	struct vnop_setattr_args a;

	a.a_desc = &vnop_setattr_desc;
	a.a_vp = vp;
	a.a_vap = vap;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_setattr_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(setattr, vnode_t, vp);

#if CONFIG_APPLEDOUBLE
	  
                                                         
    
	if (_err == 0 && !NATIVE_XATTR(vp)) {
		struct vnode_attr va;
		int change = 0;

		VATTR_INIT(&va);
		if (VATTR_IS_ACTIVE(vap, va_uid)) {
			VATTR_SET(&va, va_uid, vap->va_uid);
			change = 1;
		}
		if (VATTR_IS_ACTIVE(vap, va_gid)) {
			VATTR_SET(&va, va_gid, vap->va_gid);
			change = 1;
		}
		if (VATTR_IS_ACTIVE(vap, va_mode)) {
			VATTR_SET(&va, va_mode, vap->va_mode);
			change = 1;
		}
		if (change) {
			vnode_t dvp;
			const char   *vname;

			dvp = vnode_getparent(vp);
			vname = vnode_getname(vp);

			xattrfile_setattr(dvp, vname, &va, ctx);
			if (dvp != NULLVP) {
				vnode_put(dvp);
			}
			if (vname != NULL) {
				vnode_putname(vname);
			}
		}
	}
#endif                         

	  
                                                                       
                                                                      
         
    
	if (_err == 0 && (
		    VATTR_IS_SUPPORTED(vap, va_mode) ||
		    VATTR_IS_SUPPORTED(vap, va_uid) ||
		    VATTR_IS_SUPPORTED(vap, va_gid) ||
		    VATTR_IS_SUPPORTED(vap, va_flags) ||
		    VATTR_IS_SUPPORTED(vap, va_acl) ||
		    VATTR_IS_SUPPORTED(vap, va_uuuid) ||
		    VATTR_IS_SUPPORTED(vap, va_guuid))) {
		vnode_uncache_authorized_action(vp, KAUTH_INVALIDATE_CACHED_RIGHTS);

#if NAMEDSTREAMS
		if (vfs_authopaque(vp->v_mount) && vnode_hasnamedstreams(vp)) {
			vnode_t svp;
			if (vnode_getnamedstream(vp, &svp, XATTR_RESOURCEFORK_NAME, NS_OPEN, 0, ctx) == 0) {
				vnode_uncache_authorized_action(svp, KAUTH_INVALIDATE_CACHED_RIGHTS);
				vnode_put(svp);
			}
		}
#endif                   
	}


	post_event_if_success(vp, _err, NOTE_ATTRIB);

	return _err;
}


#if 0
  
  
                              
  
  
struct vnop_read_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct uio *a_uio;
	int a_ioflag;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_READ(vnode_t vp, struct uio * uio, int ioflag, vfs_context_t ctx)
{
	int _err;
	struct vnop_read_args a;
#if CONFIG_DTRACE
	user_ssize_t resid = uio_resid(uio);
#endif

	if (ctx == NULL) {
		return EINVAL;
	}

	a.a_desc = &vnop_read_desc;
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_ioflag = ioflag;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_read_desc.vdesc_offset])(&a);
	DTRACE_FSINFO_IO(read,
	    vnode_t, vp, user_ssize_t, (resid - uio_resid(uio)));

	return _err;
}


#if 0
  
  
                              
  
  
struct vnop_write_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct uio *a_uio;
	int a_ioflag;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_WRITE(vnode_t vp, struct uio * uio, int ioflag, vfs_context_t ctx)
{
	struct vnop_write_args a;
	int _err;
#if CONFIG_DTRACE
	user_ssize_t resid = uio_resid(uio);
#endif

	if (ctx == NULL) {
		return EINVAL;
	}

	a.a_desc = &vnop_write_desc;
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_ioflag = ioflag;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_write_desc.vdesc_offset])(&a);
	DTRACE_FSINFO_IO(write,
	    vnode_t, vp, user_ssize_t, (resid - uio_resid(uio)));

	post_event_if_success(vp, _err, NOTE_WRITE);

	return _err;
}


#if 0
  
  
                              
  
  
struct vnop_ioctl_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	u_long a_command;
	caddr_t a_data;
	int a_fflag;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_IOCTL(vnode_t vp, u_long command, caddr_t data, int fflag, vfs_context_t ctx)
{
	int _err;
	struct vnop_ioctl_args a;

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}

	  
                                                                       
   
                                                                           
                                                                          
                                                                          
                                                                      
                                                                    
                                                                       
                                            
   
                                                                          
                                                      
    
	if (vfs_context_is64bit(ctx) && !(vnode_ischr(vp) || vnode_isblk(vp))) {
		if (data != NULL && !vnode_vfs64bitready(vp)) {
			return ENOTTY;
		}
	}

	if ((command == DKIOCISSOLIDSTATE) && (vp == rootvp) && rootvp_is_ssd && data) {
		*data = 1;
		return 0;
	}

	a.a_desc = &vnop_ioctl_desc;
	a.a_vp = vp;
	a.a_command = command;
	a.a_data = data;
	a.a_fflag = fflag;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_ioctl_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(ioctl, vnode_t, vp);

	return _err;
}


#if 0
  
  
                              
  
  
struct vnop_select_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_which;
	int a_fflags;
	void *a_wql;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_SELECT(vnode_t vp, int which, int fflags, void * wql, vfs_context_t ctx)
{
	int _err;
	struct vnop_select_args a;

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}
	a.a_desc = &vnop_select_desc;
	a.a_vp = vp;
	a.a_which = which;
	a.a_fflags = fflags;
	a.a_context = ctx;
	a.a_wql = wql;

	_err = (*vp->v_op[vnop_select_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(select, vnode_t, vp);

	return _err;
}


#if 0
  
  
                              
                              
  
  
struct vnop_exchange_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_fvp;
	vnode_t a_tvp;
	int a_options;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_EXCHANGE(vnode_t fvp, vnode_t tvp, int options, vfs_context_t ctx)
{
	int _err;
	struct vnop_exchange_args a;

	a.a_desc = &vnop_exchange_desc;
	a.a_fvp = fvp;
	a.a_tvp = tvp;
	a.a_options = options;
	a.a_context = ctx;

	_err = (*fvp->v_op[vnop_exchange_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(exchange, vnode_t, fvp);

	                                                                        
	post_event_if_success(fvp, _err, NOTE_ATTRIB);
	post_event_if_success(tvp, _err, NOTE_ATTRIB);

	return _err;
}


#if 0
  
  
                              
  
  
struct vnop_revoke_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_flags;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_REVOKE(vnode_t vp, int flags, vfs_context_t ctx)
{
	struct vnop_revoke_args a;
	int _err;

	a.a_desc = &vnop_revoke_desc;
	a.a_vp = vp;
	a.a_flags = flags;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_revoke_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(revoke, vnode_t, vp);

	return _err;
}


#if 0
  
  
                        
  
  
struct vnop_mmap_check_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_flags;
	vfs_context_t a_context;
};
#endif        
errno_t
VNOP_MMAP_CHECK(vnode_t vp, int flags, vfs_context_t ctx)
{
	int _err;
	struct vnop_mmap_check_args a;

	a.a_desc = &vnop_mmap_check_desc;
	a.a_vp = vp;
	a.a_flags = flags;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_mmap_check_desc.vdesc_offset])(&a);
	if (_err == ENOTSUP) {
		_err = 0;
	}
	DTRACE_FSINFO(mmap_check, vnode_t, vp);

	return _err;
}

#if 0
  
  
                  
  
  
struct vnop_mmap_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_fflags;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_MMAP(vnode_t vp, int fflags, vfs_context_t ctx)
{
	int _err;
	struct vnop_mmap_args a;

	a.a_desc = &vnop_mmap_desc;
	a.a_vp = vp;
	a.a_fflags = fflags;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_mmap_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(mmap, vnode_t, vp);

	return _err;
}


#if 0
  
  
                    
  
  
struct vnop_mnomap_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_MNOMAP(vnode_t vp, vfs_context_t ctx)
{
	int _err;
	struct vnop_mnomap_args a;

	a.a_desc = &vnop_mnomap_desc;
	a.a_vp = vp;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_mnomap_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(mnomap, vnode_t, vp);

	return _err;
}


#if 0
  
  
                              
  
  
struct vnop_fsync_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_waitfor;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_FSYNC(vnode_t vp, int waitfor, vfs_context_t ctx)
{
	struct vnop_fsync_args a;
	int _err;

	a.a_desc = &vnop_fsync_desc;
	a.a_vp = vp;
	a.a_waitfor = waitfor;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_fsync_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(fsync, vnode_t, vp);

	return _err;
}


#if 0
  
  
                              
                              
  
  
struct vnop_remove_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t a_vp;
	struct componentname *a_cnp;
	int a_flags;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_REMOVE(vnode_t dvp, vnode_t vp, struct componentname * cnp, int flags, vfs_context_t ctx)
{
	int _err;
	struct vnop_remove_args a;

	a.a_desc = &vnop_remove_desc;
	a.a_dvp = dvp;
	a.a_vp = vp;
	a.a_cnp = cnp;
	a.a_flags = flags;
	a.a_context = ctx;

	_err = (*dvp->v_op[vnop_remove_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(remove, vnode_t, vp);

	if (_err == 0) {
		vnode_setneedinactive(vp);
#if CONFIG_APPLEDOUBLE
		if (!(NATIVE_XATTR(dvp))) {
			  
                                                                          
      
			xattrfile_remove(dvp, cnp->cn_nameptr, ctx, 1);
		}
#endif                         
	}

	post_event_if_success(vp, _err, NOTE_DELETE | NOTE_LINK);
	post_event_if_success(dvp, _err, NOTE_WRITE);

	return _err;
}

int
VNOP_COMPOUND_REMOVE(vnode_t dvp, vnode_t *vpp, struct nameidata *ndp, int32_t flags, struct vnode_attr *vap, vfs_context_t ctx)
{
	int _err;
	struct vnop_compound_remove_args a;
	int no_vp = (*vpp == NULLVP);

	a.a_desc = &vnop_compound_remove_desc;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = &ndp->ni_cnd;
	a.a_flags = flags;
	a.a_vap = vap;
	a.a_context = ctx;
	a.a_remove_authorizer = vn_authorize_unlink;

	_err = (*dvp->v_op[vnop_compound_remove_desc.vdesc_offset])(&a);
	if (_err == 0 && *vpp) {
		DTRACE_FSINFO(compound_remove, vnode_t, *vpp);
	} else {
		DTRACE_FSINFO(compound_remove, vnode_t, dvp);
	}
	if (_err == 0) {
		vnode_setneedinactive(*vpp);
#if CONFIG_APPLEDOUBLE
		if (!(NATIVE_XATTR(dvp))) {
			  
                                                                          
      
			xattrfile_remove(dvp, ndp->ni_cnd.cn_nameptr, ctx, 1);
		}
#endif                         
	}

	post_event_if_success(*vpp, _err, NOTE_DELETE | NOTE_LINK);
	post_event_if_success(dvp, _err, NOTE_WRITE);

	if (no_vp) {
		lookup_compound_vnop_post_hook(_err, dvp, *vpp, ndp, 0);
		if (*vpp && _err && _err != EKEEPLOOKING) {
			vnode_put(*vpp);
			*vpp = NULLVP;
		}
	}

	                                                        

	return _err;
}

#if 0
  
  
                              
                              
  
  
struct vnop_link_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	vnode_t a_tdvp;
	struct componentname *a_cnp;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_LINK(vnode_t vp, vnode_t tdvp, struct componentname * cnp, vfs_context_t ctx)
{
	int _err;
	struct vnop_link_args a;

#if CONFIG_APPLEDOUBLE
	  
                                                         
                                                           
    
	if (!NATIVE_XATTR(tdvp) && (vp->v_type == VREG)) {
		const char   *vname;

		vname = vnode_getname(vp);
		if (vname != NULL) {
			_err = 0;
			if (vname[0] == '.' && vname[1] == '_' && vname[2] != '\0') {
				_err = EPERM;
			}
			vnode_putname(vname);
			if (_err) {
				return _err;
			}
		}
	}
#endif                         

	a.a_desc = &vnop_link_desc;
	a.a_vp = vp;
	a.a_tdvp = tdvp;
	a.a_cnp = cnp;
	a.a_context = ctx;

	_err = (*tdvp->v_op[vnop_link_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(link, vnode_t, vp);

	post_event_if_success(vp, _err, NOTE_LINK);
	post_event_if_success(tdvp, _err, NOTE_WRITE);

	return _err;
}

errno_t
vn_rename(struct vnode *fdvp, struct vnode **fvpp, struct componentname *fcnp, struct vnode_attr *fvap,
    struct vnode *tdvp, struct vnode **tvpp, struct componentname *tcnp, struct vnode_attr *tvap,
    vfs_rename_flags_t flags, vfs_context_t ctx)
{
	int _err;
	struct nameidata *fromnd = NULL;
	struct nameidata *tond = NULL;
#if CONFIG_APPLEDOUBLE
	vnode_t src_attr_vp = NULLVP;
	vnode_t dst_attr_vp = NULLVP;
	char smallname1[48];
	char smallname2[48];
	char *xfromname = NULL;
	char *xtoname = NULL;
#endif                         
	int batched;
	uint32_t tdfflags;                                    

	batched = vnode_compound_rename_available(fdvp);

	if (!batched) {
		if (*fvpp == NULLVP) {
			panic("Not batched, and no fvp?");
		}
	}

#if CONFIG_APPLEDOUBLE
	  
                                                                           
                                                                          
                                                                        
                                                                           
                             
    

	size_t xfromname_len = 0;
	size_t xtoname_len = 0;
	if (!NATIVE_XATTR(fdvp) &&
	    !(fcnp->cn_nameptr[0] == '.' && fcnp->cn_nameptr[1] == '_')) {
		int error;

		                                     
		xfromname_len = fcnp->cn_namelen + 3;
		if (xfromname_len > sizeof(smallname1)) {
			xfromname = kheap_alloc(KHEAP_TEMP, xfromname_len, Z_WAITOK);
		} else {
			xfromname = &smallname1[0];
		}
		strlcpy(xfromname, "._", xfromname_len);
		strlcat(xfromname, fcnp->cn_nameptr, xfromname_len);

		                                          
		xtoname_len = tcnp->cn_namelen + 3;
		if (xtoname_len > sizeof(smallname2)) {
			xtoname = kheap_alloc(KHEAP_TEMP, xtoname_len, Z_WAITOK);
		} else {
			xtoname = &smallname2[0];
		}
		strlcpy(xtoname, "._", xtoname_len);
		strlcat(xtoname, tcnp->cn_nameptr, xtoname_len);

		  
                                                                   
                                                                                  
                                                                                 
                                   
     
		fromnd = kheap_alloc(KHEAP_TEMP, sizeof(struct nameidata), Z_WAITOK);
		NDINIT(fromnd, RENAME, OP_RENAME, NOFOLLOW | USEDVP | CN_NBMOUNTLOOK,
		    UIO_SYSSPACE, CAST_USER_ADDR_T(xfromname), ctx);
		fromnd->ni_dvp = fdvp;
		error = namei(fromnd);

		  
                                                            
                                        
     

		if (error == 0) {
			if (fromnd->ni_vp) {
				                                                                    
				src_attr_vp = fromnd->ni_vp;

				if (fromnd->ni_vp->v_type != VREG) {
					src_attr_vp = NULLVP;
					vnode_put(fromnd->ni_vp);
				}
			}
			  
                                                                                  
                                                                                   
                                                                                  
      
			if (src_attr_vp == NULLVP) {
				nameidone(fromnd);
			}
		}
	}
#endif                         

	if (batched) {
		_err = VNOP_COMPOUND_RENAME(fdvp, fvpp, fcnp, fvap, tdvp, tvpp, tcnp, tvap, flags, ctx);
		if (_err != 0) {
			printf("VNOP_COMPOUND_RENAME() returned %d\n", _err);
		}
	} else {
		if (flags) {
			_err = VNOP_RENAMEX(fdvp, *fvpp, fcnp, tdvp, *tvpp, tcnp, flags, ctx);
			if (_err == ENOTSUP && flags == VFS_RENAME_SECLUDE) {
				            
				if ((*fvpp)->v_mount->mnt_vtable->vfc_vfsflags & VFC_VFSVNOP_SECLUDE_RENAME) {
					fcnp->cn_flags |= CN_SECLUDE_RENAME;
					_err = VNOP_RENAME(fdvp, *fvpp, fcnp, tdvp, *tvpp, tcnp, ctx);
				}
			}
		} else {
			_err = VNOP_RENAME(fdvp, *fvpp, fcnp, tdvp, *tvpp, tcnp, ctx);
		}
	}

	  
                                                   
                                              
    
	if (_err == 0) {
		_err = vnode_flags(tdvp, &tdfflags, ctx);
		if (_err == 0) {
			uint32_t inherit_flags = tdfflags & (UF_DATAVAULT | SF_RESTRICTED);
			if (inherit_flags) {
				uint32_t fflags;
				_err = vnode_flags(*fvpp, &fflags, ctx);
				if (_err == 0 && fflags != (fflags | inherit_flags)) {
					struct vnode_attr va;
					VATTR_INIT(&va);
					VATTR_SET(&va, va_flags, fflags | inherit_flags);
					_err = vnode_setattr(*fvpp, &va, ctx);
				}
			}
		}
	}

#if CONFIG_MACF
	if (_err == 0) {
		mac_vnode_notify_rename(ctx, *fvpp, tdvp, tcnp);
		if (flags & VFS_RENAME_SWAP) {
			mac_vnode_notify_rename(ctx, *tvpp, fdvp, fcnp);
		}
	}
#endif

#if CONFIG_APPLEDOUBLE
	  
                                                                        
    
	if (_err == 0 && !NATIVE_XATTR(fdvp) && xfromname != NULL) {
		int error = 0;

		  
                                          
                                                                                
                                  
     
		tond = kheap_alloc(KHEAP_TEMP, sizeof(struct nameidata), Z_WAITOK);
		NDINIT(tond, RENAME, OP_RENAME,
		    NOCACHE | NOFOLLOW | USEDVP | CN_NBMOUNTLOOK, UIO_SYSSPACE,
		    CAST_USER_ADDR_T(xtoname), ctx);
		tond->ni_dvp = tdvp;
		error = namei(tond);

		if (error) {
			goto ad_error;
		}

		if (tond->ni_vp) {
			dst_attr_vp = tond->ni_vp;
		}

		if (src_attr_vp) {
			const char *old_name = src_attr_vp->v_name;
			vnode_t old_parent = src_attr_vp->v_parent;

			if (batched) {
				error = VNOP_COMPOUND_RENAME(fdvp, &src_attr_vp, &fromnd->ni_cnd, NULL,
				    tdvp, &dst_attr_vp, &tond->ni_cnd, NULL,
				    0, ctx);
			} else {
				error = VNOP_RENAME(fdvp, src_attr_vp, &fromnd->ni_cnd,
				    tdvp, dst_attr_vp, &tond->ni_cnd, ctx);
			}

			if (error == 0 && old_name == src_attr_vp->v_name &&
			    old_parent == src_attr_vp->v_parent) {
				int update_flags = VNODE_UPDATE_NAME;

				if (fdvp != tdvp) {
					update_flags |= VNODE_UPDATE_PARENT;
				}

				if ((src_attr_vp->v_mount->mnt_vtable->vfc_vfsflags & VFC_VFSVNOP_NOUPDATEID_RENAME) == 0) {
					vnode_update_identity(src_attr_vp, tdvp,
					    tond->ni_cnd.cn_nameptr,
					    tond->ni_cnd.cn_namelen,
					    tond->ni_cnd.cn_hash,
					    update_flags);
				}
			}

			                                                 
                                                                        
                                                                         
                                   
      
			post_event_if_success(src_attr_vp, error, NOTE_RENAME);
			if (dst_attr_vp) {
				post_event_if_success(dst_attr_vp, error, NOTE_DELETE);
			}
		} else if (dst_attr_vp) {
			  
                                                                      
                                             
                                                      
      

			struct vnop_remove_args args;

			args.a_desc    = &vnop_remove_desc;
			args.a_dvp     = tdvp;
			args.a_vp      = dst_attr_vp;
			args.a_cnp     = &tond->ni_cnd;
			args.a_context = ctx;

			if (error == 0) {
				error = (*tdvp->v_op[vnop_remove_desc.vdesc_offset])(&args);

				if (error == 0) {
					vnode_setneedinactive(dst_attr_vp);
				}
			}

			                                                                    
                                                                            
                                                 
      
			post_event_if_success(dst_attr_vp, error, NOTE_DELETE);
		}
	}
ad_error:
	if (src_attr_vp) {
		vnode_put(src_attr_vp);
		nameidone(fromnd);
	}
	if (dst_attr_vp) {
		vnode_put(dst_attr_vp);
		nameidone(tond);
	}
	if (xfromname && xfromname != &smallname1[0]) {
		kheap_free(KHEAP_TEMP, xfromname, xfromname_len);
	}
	if (xtoname && xtoname != &smallname2[0]) {
		kheap_free(KHEAP_TEMP, xtoname, xtoname_len);
	}
#endif                         
	kheap_free(KHEAP_TEMP, fromnd, sizeof(struct nameidata));
	kheap_free(KHEAP_TEMP, tond, sizeof(struct nameidata));
	return _err;
}


#if 0
  
  
                              
                              
                              
                              
  
  
struct vnop_rename_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_fdvp;
	vnode_t a_fvp;
	struct componentname *a_fcnp;
	vnode_t a_tdvp;
	vnode_t a_tvp;
	struct componentname *a_tcnp;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_RENAME(struct vnode *fdvp, struct vnode *fvp, struct componentname *fcnp,
    struct vnode *tdvp, struct vnode *tvp, struct componentname *tcnp,
    vfs_context_t ctx)
{
	int _err = 0;
	struct vnop_rename_args a;

	a.a_desc = &vnop_rename_desc;
	a.a_fdvp = fdvp;
	a.a_fvp = fvp;
	a.a_fcnp = fcnp;
	a.a_tdvp = tdvp;
	a.a_tvp = tvp;
	a.a_tcnp = tcnp;
	a.a_context = ctx;

	                                     
	_err = (*fdvp->v_op[vnop_rename_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(rename, vnode_t, fdvp);

	if (_err) {
		return _err;
	}

	return post_rename(fdvp, fvp, tdvp, tvp);
}

static errno_t
post_rename(vnode_t fdvp, vnode_t fvp, vnode_t tdvp, vnode_t tvp)
{
	if (tvp && tvp != fvp) {
		vnode_setneedinactive(tvp);
	}

	                                                                                    
	int events = NOTE_WRITE;
	if (vnode_isdir(fvp)) {
		                                                               
                                                   
                                              
     
		if (((fdvp != tdvp) && (!tvp)) || ((fdvp == tdvp) && (tvp))) {
			events |= NOTE_LINK;
		}
	}

	lock_vnode_and_post(fdvp, events);
	if (fdvp != tdvp) {
		lock_vnode_and_post(tdvp, events);
	}

	                                                            
	if (tvp) {
		lock_vnode_and_post(tvp, NOTE_DELETE);
	}

	lock_vnode_and_post(fvp, NOTE_RENAME);

	return 0;
}

#if 0
  
  
                              
                              
                              
                              
  
  
struct vnop_renamex_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_fdvp;
	vnode_t a_fvp;
	struct componentname *a_fcnp;
	vnode_t a_tdvp;
	vnode_t a_tvp;
	struct componentname *a_tcnp;
	vfs_rename_flags_t a_flags;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_RENAMEX(struct vnode *fdvp, struct vnode *fvp, struct componentname *fcnp,
    struct vnode *tdvp, struct vnode *tvp, struct componentname *tcnp,
    vfs_rename_flags_t flags, vfs_context_t ctx)
{
	int _err = 0;
	struct vnop_renamex_args a;

	a.a_desc = &vnop_renamex_desc;
	a.a_fdvp = fdvp;
	a.a_fvp = fvp;
	a.a_fcnp = fcnp;
	a.a_tdvp = tdvp;
	a.a_tvp = tvp;
	a.a_tcnp = tcnp;
	a.a_flags = flags;
	a.a_context = ctx;

	                                     
	_err = (*fdvp->v_op[vnop_renamex_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(renamex, vnode_t, fdvp);

	if (_err) {
		return _err;
	}

	return post_rename(fdvp, fvp, tdvp, tvp);
}


int
VNOP_COMPOUND_RENAME(
	struct vnode *fdvp, struct vnode **fvpp, struct componentname *fcnp, struct vnode_attr *fvap,
	struct vnode *tdvp, struct vnode **tvpp, struct componentname *tcnp, struct vnode_attr *tvap,
	uint32_t flags, vfs_context_t ctx)
{
	int _err = 0;
	int events;
	struct vnop_compound_rename_args a;
	int no_fvp, no_tvp;

	no_fvp = (*fvpp) == NULLVP;
	no_tvp = (*tvpp) == NULLVP;

	a.a_desc = &vnop_compound_rename_desc;

	a.a_fdvp = fdvp;
	a.a_fvpp = fvpp;
	a.a_fcnp = fcnp;
	a.a_fvap = fvap;

	a.a_tdvp = tdvp;
	a.a_tvpp = tvpp;
	a.a_tcnp = tcnp;
	a.a_tvap = tvap;

	a.a_flags = flags;
	a.a_context = ctx;
	a.a_rename_authorizer = vn_authorize_rename;
	a.a_reserved = NULL;

	                                     
	_err = (*fdvp->v_op[vnop_compound_rename_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(compound_rename, vnode_t, fdvp);

	if (_err == 0) {
		if (*tvpp && *tvpp != *fvpp) {
			vnode_setneedinactive(*tvpp);
		}
	}

	                                                                                    
	if (_err == 0 && *fvpp != *tvpp) {
		if (!*fvpp) {
			panic("No fvpp after compound rename?");
		}

		events = NOTE_WRITE;
		if (vnode_isdir(*fvpp)) {
			                                                               
                                                    
                                               
      
			if (((fdvp != tdvp) && (!*tvpp)) || ((fdvp == tdvp) && (*tvpp))) {
				events |= NOTE_LINK;
			}
		}

		lock_vnode_and_post(fdvp, events);
		if (fdvp != tdvp) {
			lock_vnode_and_post(tdvp, events);
		}

		                                                            
		if (*tvpp) {
			lock_vnode_and_post(*tvpp, NOTE_DELETE);
		}

		lock_vnode_and_post(*fvpp, NOTE_RENAME);
	}

	if (no_fvp) {
		lookup_compound_vnop_post_hook(_err, fdvp, *fvpp, fcnp->cn_ndp, 0);
	}
	if (no_tvp && *tvpp != NULLVP) {
		lookup_compound_vnop_post_hook(_err, tdvp, *tvpp, tcnp->cn_ndp, 0);
	}

	if (_err && _err != EKEEPLOOKING) {
		if (*fvpp) {
			vnode_put(*fvpp);
			*fvpp = NULLVP;
		}
		if (*tvpp) {
			vnode_put(*tvpp);
			*tvpp = NULLVP;
		}
	}

	return _err;
}

int
vn_mkdir(struct vnode *dvp, struct vnode **vpp, struct nameidata *ndp,
    struct vnode_attr *vap, vfs_context_t ctx)
{
	if (ndp->ni_cnd.cn_nameiop != CREATE) {
		panic("Non-CREATE nameiop in vn_mkdir()?");
	}

	if (vnode_compound_mkdir_available(dvp)) {
		return VNOP_COMPOUND_MKDIR(dvp, vpp, ndp, vap, ctx);
	} else {
		return VNOP_MKDIR(dvp, vpp, &ndp->ni_cnd, vap, ctx);
	}
}

#if 0
  
  
                              
                              
  
  
struct vnop_mkdir_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_MKDIR(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp,
    struct vnode_attr *vap, vfs_context_t ctx)
{
	int _err;
	struct vnop_mkdir_args a;

	a.a_desc = &vnop_mkdir_desc;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;
	a.a_context = ctx;

	_err = (*dvp->v_op[vnop_mkdir_desc.vdesc_offset])(&a);
	if (_err == 0 && *vpp) {
		DTRACE_FSINFO(mkdir, vnode_t, *vpp);
	}
#if CONFIG_APPLEDOUBLE
	if (_err == 0 && !NATIVE_XATTR(dvp)) {
		  
                                             
     
		xattrfile_remove(dvp, cnp->cn_nameptr, ctx, 0);
	}
#endif                         

	post_event_if_success(dvp, _err, NOTE_LINK | NOTE_WRITE);

	return _err;
}

int
VNOP_COMPOUND_MKDIR(struct vnode *dvp, struct vnode **vpp, struct nameidata *ndp,
    struct vnode_attr *vap, vfs_context_t ctx)
{
	int _err;
	struct vnop_compound_mkdir_args a;

	a.a_desc = &vnop_compound_mkdir_desc;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = &ndp->ni_cnd;
	a.a_vap = vap;
	a.a_flags = 0;
	a.a_context = ctx;
#if 0
	a.a_mkdir_authorizer = vn_authorize_mkdir;
#endif        
	a.a_reserved = NULL;

	_err = (*dvp->v_op[vnop_compound_mkdir_desc.vdesc_offset])(&a);
	if (_err == 0 && *vpp) {
		DTRACE_FSINFO(compound_mkdir, vnode_t, *vpp);
	}
#if CONFIG_APPLEDOUBLE
	if (_err == 0 && !NATIVE_XATTR(dvp)) {
		  
                                             
     
		xattrfile_remove(dvp, ndp->ni_cnd.cn_nameptr, ctx, 0);
	}
#endif                         

	post_event_if_success(dvp, _err, NOTE_LINK | NOTE_WRITE);

	lookup_compound_vnop_post_hook(_err, dvp, *vpp, ndp, (_err == 0));
	if (*vpp && _err && _err != EKEEPLOOKING) {
		vnode_put(*vpp);
		*vpp = NULLVP;
	}

	return _err;
}

int
vn_rmdir(vnode_t dvp, vnode_t *vpp, struct nameidata *ndp, struct vnode_attr *vap, vfs_context_t ctx)
{
	if (vnode_compound_rmdir_available(dvp)) {
		return VNOP_COMPOUND_RMDIR(dvp, vpp, ndp, vap, ctx);
	} else {
		if (*vpp == NULLVP) {
			panic("NULL vp, but not a compound VNOP?");
		}
		if (vap != NULL) {
			panic("Non-NULL vap, but not a compound VNOP?");
		}
		return VNOP_RMDIR(dvp, *vpp, &ndp->ni_cnd, ctx);
	}
}

#if 0
  
  
                              
                              
  
  
struct vnop_rmdir_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t a_vp;
	struct componentname *a_cnp;
	vfs_context_t a_context;
};

#endif       
errno_t
VNOP_RMDIR(struct vnode *dvp, struct vnode *vp, struct componentname *cnp, vfs_context_t ctx)
{
	int _err;
	struct vnop_rmdir_args a;

	a.a_desc = &vnop_rmdir_desc;
	a.a_dvp = dvp;
	a.a_vp = vp;
	a.a_cnp = cnp;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_rmdir_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(rmdir, vnode_t, vp);

	if (_err == 0) {
		vnode_setneedinactive(vp);
#if CONFIG_APPLEDOUBLE
		if (!(NATIVE_XATTR(dvp))) {
			  
                                                                          
      
			xattrfile_remove(dvp, cnp->cn_nameptr, ctx, 1);
		}
#endif
	}

	                                                                   
	post_event_if_success(vp, _err, NOTE_DELETE | NOTE_LINK);
	post_event_if_success(dvp, _err, NOTE_LINK | NOTE_WRITE);

	return _err;
}

int
VNOP_COMPOUND_RMDIR(struct vnode *dvp, struct vnode **vpp, struct nameidata *ndp,
    struct vnode_attr *vap, vfs_context_t ctx)
{
	int _err;
	struct vnop_compound_rmdir_args a;
	int no_vp;

	a.a_desc = &vnop_mkdir_desc;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = &ndp->ni_cnd;
	a.a_vap = vap;
	a.a_flags = 0;
	a.a_context = ctx;
	a.a_rmdir_authorizer = vn_authorize_rmdir;
	a.a_reserved = NULL;

	no_vp = (*vpp == NULLVP);

	_err = (*dvp->v_op[vnop_compound_rmdir_desc.vdesc_offset])(&a);
	if (_err == 0 && *vpp) {
		DTRACE_FSINFO(compound_rmdir, vnode_t, *vpp);
	}
#if CONFIG_APPLEDOUBLE
	if (_err == 0 && !NATIVE_XATTR(dvp)) {
		  
                                             
     
		xattrfile_remove(dvp, ndp->ni_cnd.cn_nameptr, ctx, 0);
	}
#endif

	if (*vpp) {
		post_event_if_success(*vpp, _err, NOTE_DELETE | NOTE_LINK);
	}
	post_event_if_success(dvp, _err, NOTE_LINK | NOTE_WRITE);

	if (no_vp) {
		lookup_compound_vnop_post_hook(_err, dvp, *vpp, ndp, 0);

#if 0                                                   
		if (*vpp && _err && _err != EKEEPLOOKING) {
			vnode_put(*vpp);
			*vpp = NULLVP;
		}
#endif         
	}

	return _err;
}

#if CONFIG_APPLEDOUBLE
  
                               
   
#define AD_STALE_SECS  (180)
static void
xattrfile_remove(vnode_t dvp, const char * basename, vfs_context_t ctx, int force)
{
	vnode_t xvp;
	struct nameidata nd;
	char smallname[64];
	char *filename = NULL;
	size_t alloc_len;
	size_t copy_len;

	if ((basename == NULL) || (basename[0] == '\0') ||
	    (basename[0] == '.' && basename[1] == '_')) {
		return;
	}
	filename = &smallname[0];
	alloc_len = snprintf(filename, sizeof(smallname), "._%s", basename);
	if (alloc_len >= sizeof(smallname)) {
		alloc_len++;                                            
		filename = kheap_alloc(KHEAP_TEMP, alloc_len, Z_WAITOK);
		copy_len = snprintf(filename, alloc_len, "._%s", basename);
	}
	NDINIT(&nd, DELETE, OP_UNLINK, WANTPARENT | LOCKLEAF | NOFOLLOW | USEDVP, UIO_SYSSPACE,
	    CAST_USER_ADDR_T(filename), ctx);
	nd.ni_dvp = dvp;
	if (namei(&nd) != 0) {
		goto out2;
	}

	xvp = nd.ni_vp;
	nameidone(&nd);
	if (xvp->v_type != VREG) {
		goto out1;
	}

	  
                                                      
                                                  
   
    
	if (!force) {
		struct vnode_attr va;

		VATTR_INIT(&va);
		VATTR_WANTED(&va, va_data_size);
		VATTR_WANTED(&va, va_modify_time);
		if (VNOP_GETATTR(xvp, &va, ctx) == 0 &&
		    VATTR_IS_SUPPORTED(&va, va_data_size) &&
		    VATTR_IS_SUPPORTED(&va, va_modify_time) &&
		    va.va_data_size != 0) {
			struct timeval tv;

			microtime(&tv);
			if ((tv.tv_sec > va.va_modify_time.tv_sec) &&
			    (tv.tv_sec - va.va_modify_time.tv_sec) > AD_STALE_SECS) {
				force = 1;                     
			}
		}
	}
	if (force) {
		int  error;

		error = VNOP_REMOVE(dvp, xvp, &nd.ni_cnd, 0, ctx);
		if (error == 0) {
			vnode_setneedinactive(xvp);
		}

		post_event_if_success(xvp, error, NOTE_DELETE);
		post_event_if_success(dvp, error, NOTE_WRITE);
	}

out1:
	vnode_put(dvp);
	vnode_put(xvp);
out2:
	if (filename && filename != &smallname[0]) {
		kheap_free(KHEAP_TEMP, filename, alloc_len);
	}
}

  
                                              
   
static void
xattrfile_setattr(vnode_t dvp, const char * basename, struct vnode_attr * vap,
    vfs_context_t ctx)
{
	vnode_t xvp;
	struct nameidata nd;
	char smallname[64];
	char *filename = NULL;
	size_t alloc_len;
	size_t copy_len;

	if ((dvp == NULLVP) ||
	    (basename == NULL) || (basename[0] == '\0') ||
	    (basename[0] == '.' && basename[1] == '_')) {
		return;
	}
	filename = &smallname[0];
	alloc_len = snprintf(filename, sizeof(smallname), "._%s", basename);
	if (alloc_len >= sizeof(smallname)) {
		alloc_len++;                                            
		filename = kheap_alloc(KHEAP_TEMP, alloc_len, Z_WAITOK);
		copy_len = snprintf(filename, alloc_len, "._%s", basename);
	}
	NDINIT(&nd, LOOKUP, OP_SETATTR, NOFOLLOW | USEDVP, UIO_SYSSPACE,
	    CAST_USER_ADDR_T(filename), ctx);
	nd.ni_dvp = dvp;
	if (namei(&nd) != 0) {
		goto out2;
	}

	xvp = nd.ni_vp;
	nameidone(&nd);

	if (xvp->v_type == VREG) {
		struct vnop_setattr_args a;

		a.a_desc = &vnop_setattr_desc;
		a.a_vp = xvp;
		a.a_vap = vap;
		a.a_context = ctx;

		(void) (*xvp->v_op[vnop_setattr_desc.vdesc_offset])(&a);
	}

	vnode_put(xvp);
out2:
	if (filename && filename != &smallname[0]) {
		kheap_free(KHEAP_TEMP, filename, alloc_len);
	}
}
#endif                         

 #if 0
  
  
                              
                              
  
  
struct vnop_symlink_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	struct vnode_attr *a_vap;
	char *a_target;
	vfs_context_t a_context;
};

#endif       
errno_t
VNOP_SYMLINK(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp,
    struct vnode_attr *vap, char *target, vfs_context_t ctx)
{
	int _err;
	struct vnop_symlink_args a;

	a.a_desc = &vnop_symlink_desc;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;
	a.a_target = target;
	a.a_context = ctx;

	_err = (*dvp->v_op[vnop_symlink_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(symlink, vnode_t, dvp);
#if CONFIG_APPLEDOUBLE
	if (_err == 0 && !NATIVE_XATTR(dvp)) {
		  
                                                                   
     
		xattrfile_remove(dvp, cnp->cn_nameptr, ctx, 0);
	}
#endif                         

	post_event_if_success(dvp, _err, NOTE_WRITE);

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_readdir_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct uio *a_uio;
	int a_flags;
	int *a_eofflag;
	int *a_numdirent;
	vfs_context_t a_context;
};

#endif       
errno_t
VNOP_READDIR(struct vnode *vp, struct uio *uio, int flags, int *eofflag,
    int *numdirent, vfs_context_t ctx)
{
	int _err;
	struct vnop_readdir_args a;
#if CONFIG_DTRACE
	user_ssize_t resid = uio_resid(uio);
#endif

	a.a_desc = &vnop_readdir_desc;
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_flags = flags;
	a.a_eofflag = eofflag;
	a.a_numdirent = numdirent;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_readdir_desc.vdesc_offset])(&a);
	DTRACE_FSINFO_IO(readdir,
	    vnode_t, vp, user_ssize_t, (resid - uio_resid(uio)));

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_readdirattr_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct attrlist *a_alist;
	struct uio *a_uio;
	uint32_t a_maxcount;
	uint32_t a_options;
	uint32_t *a_newstate;
	int *a_eofflag;
	uint32_t *a_actualcount;
	vfs_context_t a_context;
};

#endif       
errno_t
VNOP_READDIRATTR(struct vnode *vp, struct attrlist *alist, struct uio *uio, uint32_t maxcount,
    uint32_t options, uint32_t *newstate, int *eofflag, uint32_t *actualcount, vfs_context_t ctx)
{
	int _err;
	struct vnop_readdirattr_args a;
#if CONFIG_DTRACE
	user_ssize_t resid = uio_resid(uio);
#endif

	a.a_desc = &vnop_readdirattr_desc;
	a.a_vp = vp;
	a.a_alist = alist;
	a.a_uio = uio;
	a.a_maxcount = maxcount;
	a.a_options = options;
	a.a_newstate = newstate;
	a.a_eofflag = eofflag;
	a.a_actualcount = actualcount;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_readdirattr_desc.vdesc_offset])(&a);
	DTRACE_FSINFO_IO(readdirattr,
	    vnode_t, vp, user_ssize_t, (resid - uio_resid(uio)));

	return _err;
}

#if 0
struct vnop_getttrlistbulk_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct attrlist *a_alist;
	struct vnode_attr *a_vap;
	struct uio *a_uio;
	void *a_private
	uint64_t a_options;
	int *a_eofflag;
	uint32_t *a_actualcount;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_GETATTRLISTBULK(struct vnode *vp, struct attrlist *alist,
    struct vnode_attr *vap, struct uio *uio, void *private, uint64_t options,
    int32_t *eofflag, int32_t *actualcount, vfs_context_t ctx)
{
	int _err;
	struct vnop_getattrlistbulk_args a;
#if CONFIG_DTRACE
	user_ssize_t resid = uio_resid(uio);
#endif

	a.a_desc = &vnop_getattrlistbulk_desc;
	a.a_vp = vp;
	a.a_alist = alist;
	a.a_vap = vap;
	a.a_uio = uio;
	a.a_private = private;
	a.a_options = options;
	a.a_eofflag = eofflag;
	a.a_actualcount = actualcount;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_getattrlistbulk_desc.vdesc_offset])(&a);
	DTRACE_FSINFO_IO(getattrlistbulk,
	    vnode_t, vp, user_ssize_t, (resid - uio_resid(uio)));

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_readlink_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct uio *a_uio;
	vfs_context_t a_context;
};
#endif        

  
                       
                                                              
                                          
                                            
                         
                      
  
                                                                    
                                                                 
                                                                  
                                                            
                
  
                                                                   
                                                            
                                                                 
             
   
errno_t
VNOP_READLINK(struct vnode *vp, struct uio *uio, vfs_context_t ctx)
{
	int _err;
	struct vnop_readlink_args a;
#if CONFIG_DTRACE
	user_ssize_t resid = uio_resid(uio);
#endif
	a.a_desc = &vnop_readlink_desc;
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_readlink_desc.vdesc_offset])(&a);
	DTRACE_FSINFO_IO(readlink,
	    vnode_t, vp, user_ssize_t, (resid - uio_resid(uio)));

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_inactive_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_INACTIVE(struct vnode *vp, vfs_context_t ctx)
{
	int _err;
	struct vnop_inactive_args a;

	a.a_desc = &vnop_inactive_desc;
	a.a_vp = vp;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_inactive_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(inactive, vnode_t, vp);

#if NAMEDSTREAMS
	                                                                  
                                                                   
                                                                   
                                                   
    
	if (vnode_isnamedstream(vp) &&
	    (vp->v_parent != NULLVP) &&
	    vnode_isshadow(vp) &&
	    ((vp->v_lflag & VL_TERMINATE) == 0)) {
		vnode_recycle(vp);
	}
#endif

	return _err;
}


#if 0
  
  
                              
  
  
struct vnop_reclaim_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_RECLAIM(struct vnode *vp, vfs_context_t ctx)
{
	int _err;
	struct vnop_reclaim_args a;

	a.a_desc = &vnop_reclaim_desc;
	a.a_vp = vp;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_reclaim_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(reclaim, vnode_t, vp);

	return _err;
}


  
                       
                                                              
                                          
                                            
                                                            
   
#if 0
  
  
                              
  
  
struct vnop_pathconf_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_name;
	int32_t *a_retval;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_PATHCONF(struct vnode *vp, int name, int32_t *retval, vfs_context_t ctx)
{
	int _err;
	struct vnop_pathconf_args a;

	a.a_desc = &vnop_pathconf_desc;
	a.a_vp = vp;
	a.a_name = name;
	a.a_retval = retval;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_pathconf_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(pathconf, vnode_t, vp);

	return _err;
}

  
                       
                      
                 
                          
  
                                                                     
                                                               
                                                              
                                                    
   
#if 0
  
  
                              
  
  
struct vnop_advlock_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	caddr_t a_id;
	int a_op;
	struct flock *a_fl;
	int a_flags;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_ADVLOCK(struct vnode *vp, caddr_t id, int op, struct flock *fl, int flags, vfs_context_t ctx, struct timespec *timeout)
{
	int _err;
	struct vnop_advlock_args a;

	a.a_desc = &vnop_advlock_desc;
	a.a_vp = vp;
	a.a_id = id;
	a.a_op = op;
	a.a_fl = fl;
	a.a_flags = flags;
	a.a_context = ctx;
	a.a_timeout = timeout;

	                                                      
	if (vnode_isfifo(vp)) {
		_err = err_advlock(&a);
	} else {
		if ((vp->v_flag & VLOCKLOCAL)) {
			                                         
			_err = lf_advlock(&a);
		} else if (flags & F_OFD_LOCK) {
			                                                  
			_err = err_advlock(&a);
		} else {
			                                                    
			_err = (*vp->v_op[vnop_advlock_desc.vdesc_offset])(&a);
		}
		DTRACE_FSINFO(advlock, vnode_t, vp);
		if (op == F_UNLCK &&
		    (flags & (F_FLOCK | F_OFD_LOCK)) != 0) {
			post_event_if_success(vp, _err, NOTE_FUNLOCK);
		}
	}

	return _err;
}



#if 0
  
  
                              
  
  
struct vnop_allocate_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	off_t a_length;
	u_int32_t a_flags;
	off_t *a_bytesallocated;
	off_t a_offset;
	vfs_context_t a_context;
};

#endif       
errno_t
VNOP_ALLOCATE(struct vnode *vp, off_t length, u_int32_t flags, off_t *bytesallocated, off_t offset, vfs_context_t ctx)
{
	int _err;
	struct vnop_allocate_args a;

	a.a_desc = &vnop_allocate_desc;
	a.a_vp = vp;
	a.a_length = length;
	a.a_flags = flags;
	a.a_bytesallocated = bytesallocated;
	a.a_offset = offset;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_allocate_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(allocate, vnode_t, vp);
#if CONFIG_FSE
	if (_err == 0) {
		add_fsevent(FSE_STAT_CHANGED, ctx, FSE_ARG_VNODE, vp, FSE_ARG_DONE);
	}
#endif

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_pagein_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	upl_t a_pl;
	upl_offset_t a_pl_offset;
	off_t a_f_offset;
	size_t a_size;
	int a_flags;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_PAGEIN(struct vnode *vp, upl_t pl, upl_offset_t pl_offset, off_t f_offset, size_t size, int flags, vfs_context_t ctx)
{
	int _err;
	struct vnop_pagein_args a;

	a.a_desc = &vnop_pagein_desc;
	a.a_vp = vp;
	a.a_pl = pl;
	a.a_pl_offset = pl_offset;
	a.a_f_offset = f_offset;
	a.a_size = size;
	a.a_flags = flags;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_pagein_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(pagein, vnode_t, vp);

	return _err;
}

#if 0
  
  
                              
  
  
struct vnop_pageout_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	upl_t a_pl;
	upl_offset_t a_pl_offset;
	off_t a_f_offset;
	size_t a_size;
	int a_flags;
	vfs_context_t a_context;
};

#endif       
errno_t
VNOP_PAGEOUT(struct vnode *vp, upl_t pl, upl_offset_t pl_offset, off_t f_offset, size_t size, int flags, vfs_context_t ctx)
{
	int _err;
	struct vnop_pageout_args a;

	a.a_desc = &vnop_pageout_desc;
	a.a_vp = vp;
	a.a_pl = pl;
	a.a_pl_offset = pl_offset;
	a.a_f_offset = f_offset;
	a.a_size = size;
	a.a_flags = flags;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_pageout_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(pageout, vnode_t, vp);

	post_event_if_success(vp, _err, NOTE_WRITE);

	return _err;
}

int
vn_remove(vnode_t dvp, vnode_t *vpp, struct nameidata *ndp, int32_t flags, struct vnode_attr *vap, vfs_context_t ctx)
{
	if (vnode_compound_remove_available(dvp)) {
		return VNOP_COMPOUND_REMOVE(dvp, vpp, ndp, flags, vap, ctx);
	} else {
		return VNOP_REMOVE(dvp, *vpp, &ndp->ni_cnd, flags, ctx);
	}
}

#if CONFIG_SEARCHFS

#if 0
  
  
                              
  
  
struct vnop_searchfs_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	void *a_searchparams1;
	void *a_searchparams2;
	struct attrlist *a_searchattrs;
	uint32_t a_maxmatches;
	struct timeval *a_timelimit;
	struct attrlist *a_returnattrs;
	uint32_t *a_nummatches;
	uint32_t a_scriptcode;
	uint32_t a_options;
	struct uio *a_uio;
	struct searchstate *a_searchstate;
	vfs_context_t a_context;
};

#endif       
errno_t
VNOP_SEARCHFS(struct vnode *vp, void *searchparams1, void *searchparams2, struct attrlist *searchattrs, uint32_t maxmatches, struct timeval *timelimit, struct attrlist *returnattrs, uint32_t *nummatches, uint32_t scriptcode, uint32_t options, struct uio *uio, struct searchstate *searchstate, vfs_context_t ctx)
{
	int _err;
	struct vnop_searchfs_args a;

	a.a_desc = &vnop_searchfs_desc;
	a.a_vp = vp;
	a.a_searchparams1 = searchparams1;
	a.a_searchparams2 = searchparams2;
	a.a_searchattrs = searchattrs;
	a.a_maxmatches = maxmatches;
	a.a_timelimit = timelimit;
	a.a_returnattrs = returnattrs;
	a.a_nummatches = nummatches;
	a.a_scriptcode = scriptcode;
	a.a_options = options;
	a.a_uio = uio;
	a.a_searchstate = searchstate;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_searchfs_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(searchfs, vnode_t, vp);

	return _err;
}
#endif                      

#if 0
  
  
                      
                       
                      
  
  
struct vnop_copyfile_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_fvp;
	vnode_t a_tdvp;
	vnode_t a_tvp;
	struct componentname *a_tcnp;
	int a_mode;
	int a_flags;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_COPYFILE(struct vnode *fvp, struct vnode *tdvp, struct vnode *tvp, struct componentname *tcnp,
    int mode, int flags, vfs_context_t ctx)
{
	int _err;
	struct vnop_copyfile_args a;
	a.a_desc = &vnop_copyfile_desc;
	a.a_fvp = fvp;
	a.a_tdvp = tdvp;
	a.a_tvp = tvp;
	a.a_tcnp = tcnp;
	a.a_mode = mode;
	a.a_flags = flags;
	a.a_context = ctx;
	_err = (*fvp->v_op[vnop_copyfile_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(copyfile, vnode_t, fvp);
	return _err;
}

#if 0
struct vnop_clonefile_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_fvp;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	struct vnode_attr *a_vap;
	uint32_t a_flags;
	vfs_context_t a_context;
	int (*a_dir_clone_authorizer)(                              
		struct vnode_attr *vap,                                         
		kauth_action_t action,                                                             
		struct vnode_attr *dvap,                                          
		vnode_t sdvp,                                                        
		mount_t mp,                                        
		dir_clone_authorizer_op_t vattr_op,                                                                              
		uint32_t flags;                                                  
		vfs_context_t ctx,                                             
		void *reserved);                                         
	void *a_reserved;                                     
};
#endif        

errno_t
VNOP_CLONEFILE(vnode_t fvp, vnode_t dvp, vnode_t *vpp,
    struct componentname *cnp, struct vnode_attr *vap, uint32_t flags,
    vfs_context_t ctx)
{
	int _err;
	struct vnop_clonefile_args a;
	a.a_desc = &vnop_clonefile_desc;
	a.a_fvp = fvp;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;
	a.a_flags = flags;
	a.a_context = ctx;

	if (vnode_vtype(fvp) == VDIR) {
		a.a_dir_clone_authorizer = vnode_attr_authorize_dir_clone;
	} else {
		a.a_dir_clone_authorizer = NULL;
	}

	_err = (*dvp->v_op[vnop_clonefile_desc.vdesc_offset])(&a);

	if (_err == 0 && *vpp) {
		DTRACE_FSINFO(clonefile, vnode_t, *vpp);
		if (kdebug_enable) {
			kdebug_lookup(*vpp, cnp);
		}
	}

	post_event_if_success(dvp, _err, NOTE_WRITE);

	return _err;
}

errno_t
VNOP_GETXATTR(vnode_t vp, const char *name, uio_t uio, size_t *size, int options, vfs_context_t ctx)
{
	struct vnop_getxattr_args a;
	int error;

	a.a_desc = &vnop_getxattr_desc;
	a.a_vp = vp;
	a.a_name = name;
	a.a_uio = uio;
	a.a_size = size;
	a.a_options = options;
	a.a_context = ctx;

	error = (*vp->v_op[vnop_getxattr_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(getxattr, vnode_t, vp);

	return error;
}

errno_t
VNOP_SETXATTR(vnode_t vp, const char *name, uio_t uio, int options, vfs_context_t ctx)
{
	struct vnop_setxattr_args a;
	int error;

	a.a_desc = &vnop_setxattr_desc;
	a.a_vp = vp;
	a.a_name = name;
	a.a_uio = uio;
	a.a_options = options;
	a.a_context = ctx;

	error = (*vp->v_op[vnop_setxattr_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(setxattr, vnode_t, vp);

	if (error == 0) {
		vnode_uncache_authorized_action(vp, KAUTH_INVALIDATE_CACHED_RIGHTS);
	}

	post_event_if_success(vp, error, NOTE_ATTRIB);

	return error;
}

errno_t
VNOP_REMOVEXATTR(vnode_t vp, const char *name, int options, vfs_context_t ctx)
{
	struct vnop_removexattr_args a;
	int error;

	a.a_desc = &vnop_removexattr_desc;
	a.a_vp = vp;
	a.a_name = name;
	a.a_options = options;
	a.a_context = ctx;

	error = (*vp->v_op[vnop_removexattr_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(removexattr, vnode_t, vp);

	post_event_if_success(vp, error, NOTE_ATTRIB);

	return error;
}

errno_t
VNOP_LISTXATTR(vnode_t vp, uio_t uio, size_t *size, int options, vfs_context_t ctx)
{
	struct vnop_listxattr_args a;
	int error;

	a.a_desc = &vnop_listxattr_desc;
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_size = size;
	a.a_options = options;
	a.a_context = ctx;

	error = (*vp->v_op[vnop_listxattr_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(listxattr, vnode_t, vp);

	return error;
}


#if 0
  
  
                     
  
  
struct vnop_blktooff_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	daddr64_t a_lblkno;
	off_t *a_offset;
};
#endif       
errno_t
VNOP_BLKTOOFF(struct vnode *vp, daddr64_t lblkno, off_t *offset)
{
	int _err;
	struct vnop_blktooff_args a;

	a.a_desc = &vnop_blktooff_desc;
	a.a_vp = vp;
	a.a_lblkno = lblkno;
	a.a_offset = offset;

	_err = (*vp->v_op[vnop_blktooff_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(blktooff, vnode_t, vp);

	return _err;
}

#if 0
  
  
                     
  
  
struct vnop_offtoblk_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	off_t a_offset;
	daddr64_t *a_lblkno;
};
#endif       
errno_t
VNOP_OFFTOBLK(struct vnode *vp, off_t offset, daddr64_t *lblkno)
{
	int _err;
	struct vnop_offtoblk_args a;

	a.a_desc = &vnop_offtoblk_desc;
	a.a_vp = vp;
	a.a_offset = offset;
	a.a_lblkno = lblkno;

	_err = (*vp->v_op[vnop_offtoblk_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(offtoblk, vnode_t, vp);

	return _err;
}

#if 0
  
  
               
  
  
struct vnop_verify_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	off_t a_foffset;
	char *a_buf;
	size_t a_bufsize;
	size_t *a_verifyblksize;
	int a_flags;
	vfs_context_t a_context;
};
#endif

errno_t
VNOP_VERIFY(struct vnode *vp, off_t foffset, uint8_t *buf, size_t bufsize,
    size_t *verify_block_size, vnode_verify_flags_t flags, vfs_context_t ctx)
{
	int _err;
	struct vnop_verify_args a;

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}
	a.a_desc = &vnop_verify_desc;
	a.a_vp = vp;
	a.a_foffset = foffset;
	a.a_buf = buf;
	a.a_bufsize = bufsize;
	a.a_verifyblksize = verify_block_size;
	a.a_flags = flags;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_verify_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(verify, vnode_t, vp);

	                                                                  
	if (_err == ENOTSUP) {
		if (!buf && verify_block_size) {
			*verify_block_size = 0;
		}

		_err = 0;
	}

	return _err;
}

#if 0
  
  
                     
  
  
struct vnop_blockmap_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	off_t a_foffset;
	size_t a_size;
	daddr64_t *a_bpn;
	size_t *a_run;
	void *a_poff;
	int a_flags;
	vfs_context_t a_context;
};
#endif       
errno_t
VNOP_BLOCKMAP(struct vnode *vp, off_t foffset, size_t size, daddr64_t *bpn, size_t *run, void *poff, int flags, vfs_context_t ctx)
{
	int _err;
	struct vnop_blockmap_args a;
	size_t localrun = 0;

	if (ctx == NULL) {
		ctx = vfs_context_current();
	}
	a.a_desc = &vnop_blockmap_desc;
	a.a_vp = vp;
	a.a_foffset = foffset;
	a.a_size = size;
	a.a_bpn = bpn;
	a.a_run = &localrun;
	a.a_poff = poff;
	a.a_flags = flags;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_blockmap_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(blockmap, vnode_t, vp);

	  
                                                                       
                                                               
                                                                        
                                                     
    
	if (_err == 0) {
		if (localrun > size) {
			localrun = size;
		}

		if (run) {
			*run = localrun;
		}
	}

	return _err;
}

#if 0
struct vnop_strategy_args {
	struct vnodeop_desc *a_desc;
	struct buf *a_bp;
};

#endif       
errno_t
VNOP_STRATEGY(struct buf *bp)
{
	int _err;
	struct vnop_strategy_args a;
	vnode_t vp = buf_vnode(bp);
	a.a_desc = &vnop_strategy_desc;
	a.a_bp = bp;
	_err = (*vp->v_op[vnop_strategy_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(strategy, vnode_t, vp);
	return _err;
}

#if 0
struct vnop_bwrite_args {
	struct vnodeop_desc *a_desc;
	buf_t a_bp;
};
#endif       
errno_t
VNOP_BWRITE(struct buf *bp)
{
	int _err;
	struct vnop_bwrite_args a;
	vnode_t vp = buf_vnode(bp);
	a.a_desc = &vnop_bwrite_desc;
	a.a_bp = bp;
	_err = (*vp->v_op[vnop_bwrite_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(bwrite, vnode_t, vp);
	return _err;
}

#if 0
struct vnop_kqfilt_add_args {
	struct vnodeop_desc *a_desc;
	struct vnode *a_vp;
	struct knote *a_kn;
	vfs_context_t a_context;
};
#endif
errno_t
VNOP_KQFILT_ADD(struct vnode *vp, struct knote *kn, vfs_context_t ctx)
{
	int _err;
	struct vnop_kqfilt_add_args a;

	a.a_desc = VDESC(vnop_kqfilt_add);
	a.a_vp = vp;
	a.a_kn = kn;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_kqfilt_add_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(kqfilt_add, vnode_t, vp);

	return _err;
}

#if 0
struct vnop_kqfilt_remove_args {
	struct vnodeop_desc *a_desc;
	struct vnode *a_vp;
	uintptr_t a_ident;
	vfs_context_t a_context;
};
#endif
errno_t
VNOP_KQFILT_REMOVE(struct vnode *vp, uintptr_t ident, vfs_context_t ctx)
{
	int _err;
	struct vnop_kqfilt_remove_args a;

	a.a_desc = VDESC(vnop_kqfilt_remove);
	a.a_vp = vp;
	a.a_ident = ident;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_kqfilt_remove_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(kqfilt_remove, vnode_t, vp);

	return _err;
}

errno_t
VNOP_MONITOR(vnode_t vp, uint32_t events, uint32_t flags, void *handle, vfs_context_t ctx)
{
	int _err;
	struct vnop_monitor_args a;

	a.a_desc = VDESC(vnop_monitor);
	a.a_vp = vp;
	a.a_events = events;
	a.a_flags = flags;
	a.a_handle = handle;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_monitor_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(monitor, vnode_t, vp);

	return _err;
}

#if 0
struct vnop_setlabel_args {
	struct vnodeop_desc *a_desc;
	struct vnode *a_vp;
	struct label *a_vl;
	vfs_context_t a_context;
};
#endif
errno_t
VNOP_SETLABEL(struct vnode *vp, struct label *label, vfs_context_t ctx)
{
	int _err;
	struct vnop_setlabel_args a;

	a.a_desc = VDESC(vnop_setlabel);
	a.a_vp = vp;
	a.a_vl = label;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_setlabel_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(setlabel, vnode_t, vp);

	return _err;
}


#if NAMEDSTREAMS
  
                       
   
errno_t
VNOP_GETNAMEDSTREAM(vnode_t vp, vnode_t *svpp, const char *name, enum nsoperation operation, int flags, vfs_context_t ctx)
{
	int _err;
	struct vnop_getnamedstream_args a;

	a.a_desc = &vnop_getnamedstream_desc;
	a.a_vp = vp;
	a.a_svpp = svpp;
	a.a_name = name;
	a.a_operation = operation;
	a.a_flags = flags;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_getnamedstream_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(getnamedstream, vnode_t, vp);
	return _err;
}

  
                          
   
errno_t
VNOP_MAKENAMEDSTREAM(vnode_t vp, vnode_t *svpp, const char *name, int flags, vfs_context_t ctx)
{
	int _err;
	struct vnop_makenamedstream_args a;

	a.a_desc = &vnop_makenamedstream_desc;
	a.a_vp = vp;
	a.a_svpp = svpp;
	a.a_name = name;
	a.a_flags = flags;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_makenamedstream_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(makenamedstream, vnode_t, vp);
	return _err;
}


  
                          
   
errno_t
VNOP_REMOVENAMEDSTREAM(vnode_t vp, vnode_t svp, const char *name, int flags, vfs_context_t ctx)
{
	int _err;
	struct vnop_removenamedstream_args a;

	a.a_desc = &vnop_removenamedstream_desc;
	a.a_vp = vp;
	a.a_svp = svp;
	a.a_name = name;
	a.a_flags = flags;
	a.a_context = ctx;

	_err = (*vp->v_op[vnop_removenamedstream_desc.vdesc_offset])(&a);
	DTRACE_FSINFO(removenamedstream, vnode_t, vp);
	return _err;
}
#endif
