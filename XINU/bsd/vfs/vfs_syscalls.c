/*
 * Copyright (c) 1995-2020 Apple Inc. All rights reserved.
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
 *	@(#)vfs_syscalls.c	8.41 (Berkeley) 6/15/95
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file_internal.h>
#include <sys/stat.h>
#include <sys/vnode_internal.h>
#include <sys/mount_internal.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/uio_internal.h>
#include <kern/kalloc.h>
#include <sys/mman.h>
#include <sys/dirent.h>
#include <sys/attr.h>
#include <sys/sysctl.h>
#include <sys/ubc.h>
#include <sys/quota.h>
#include <sys/kdebug.h>
#include <sys/fsevents.h>
#include <sys/imgsrc.h>
#include <sys/sysproto.h>
#include <sys/sysctl.h>
#include <sys/xattr.h>
#include <sys/fcntl.h>
#include <sys/fsctl.h>
#include <sys/ubc_internal.h>
#include <sys/disk.h>
#include <sys/content_protection.h>
#include <sys/clonefile.h>
#include <sys/snapshot.h>
#include <sys/priv.h>
#include <sys/fsgetpath.h>
#include <machine/cons.h>
#include <machine/limits.h>
#include <miscfs/specfs/specdev.h>

#include <vfs/vfs_disk_conditioner.h>

#include <security/audit/audit.h>
#include <bsm/audit_kevents.h>

#include <mach/mach_types.h>
#include <kern/kern_types.h>
#include <kern/kalloc.h>
#include <kern/task.h>

#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>

#include <libkern/OSAtomic.h>
#include <os/atomic_private.h>
#include <pexpert/pexpert.h>
#include <IOKit/IOBSD.h>

                    
#include <kern/host.h>
#include <kern/ipc_misc.h>
#include <mach/host_priv.h>
#include <mach/vfs_nspace.h>
#include <os/log.h>

#include <nfs/nfs_conf.h>

#if ROUTEFS
#include <miscfs/routefs/routefs.h>
#endif              

#if CONFIG_MACF
#include <security/mac.h>
#include <security/mac_framework.h>
#endif

#if CONFIG_FSE
#define GET_PATH(x) \
	((x) = get_pathbuff())
#define RELEASE_PATH(x) \
	release_pathbuff(x)
#else
#define GET_PATH(x)     \
	((x) = zalloc(ZV_NAMEI))
#define RELEASE_PATH(x) \
	zfree(ZV_NAMEI, x)
#endif                 

#ifndef HFS_GET_BOOT_INFO
#define HFS_GET_BOOT_INFO   (FCNTL_FS_SPECIFIC_BASE + 0x00004)
#endif

#ifndef HFS_SET_BOOT_INFO
#define HFS_SET_BOOT_INFO   (FCNTL_FS_SPECIFIC_BASE + 0x00005)
#endif

#ifndef APFSIOC_REVERT_TO_SNAPSHOT
#define APFSIOC_REVERT_TO_SNAPSHOT  _IOW('J', 1, u_int64_t)
#endif

  
                                                          
                                          
   
#define KM_FD_VN_DATA KHEAP_DEFAULT

extern void disk_conditioner_unmount(mount_t mp);

                                    
struct cdirargs {
	vnode_t olddp;
	vnode_t newdp;
};
                                       
static int checkdirs_callback(proc_t p, void * arg);

static int change_dir(struct nameidata *ndp, vfs_context_t ctx);
static int checkdirs(vnode_t olddp, vfs_context_t ctx);
void enablequotas(struct mount *mp, vfs_context_t ctx);
static int getfsstat_callback(mount_t mp, void * arg);
static int getutimes(user_addr_t usrtvp, struct timespec *tsp);
static int setutimes(vfs_context_t ctx, vnode_t vp, const struct timespec *ts, int nullflag);
static int sync_callback(mount_t, void *);
static int munge_statfs(struct mount *mp, struct vfsstatfs *sfsp,
    user_addr_t bufp, int *sizep, boolean_t is_64_bit,
    boolean_t partial_copy);
static int fsync_common(proc_t p, struct fsync_args *uap, int flags);
static int mount_common(char *fstypename, vnode_t pvp, vnode_t vp,
    struct componentname *cnp, user_addr_t fsmountargs,
    int flags, uint32_t internal_flags, char *labelstr, boolean_t kernelmount,
    vfs_context_t ctx);
void vfs_notify_mount(vnode_t pdvp);

int prepare_coveredvp(vnode_t vp, vfs_context_t ctx, struct componentname *cnp, const char *fsname, boolean_t skip_auth);

struct fd_vn_data * fg_vn_data_alloc(void);

  
                                                                           
                                                                     
                                                                           
                                                                            
                                                                                
                                                                               
                                                                                
                                                                  
   
#define MAX_AUTHORIZE_ENOENT_RETRIES 1024

                                                        
#define MAX_RENAME_ERECYCLE_RETRIES 1024

static int rmdirat_internal(vfs_context_t, int, user_addr_t, enum uio_seg,
    int unlink_flags);

static int fsgetpath_internal(vfs_context_t, int, uint64_t, vm_size_t, caddr_t, uint32_t options, int *);

#ifdef CONFIG_IMGSRC_ACCESS
static int authorize_devpath_and_update_mntfromname(mount_t mp, user_addr_t devpath, vnode_t *devvpp, vfs_context_t ctx);
static int place_mount_and_checkdirs(mount_t mp, vnode_t vp, vfs_context_t ctx);
static void undo_place_on_covered_vp(mount_t mp, vnode_t vp);
static int mount_begin_update(mount_t mp, vfs_context_t ctx, int flags);
static void mount_end_update(mount_t mp);
static int relocate_imageboot_source(vnode_t pvp, vnode_t vp, struct componentname *cnp, const char *fsname, vfs_context_t ctx, boolean_t is64bit, user_addr_t fsmountargs, boolean_t by_index);
#endif                           

#if CONFIG_LOCKERBOOT
int mount_locker_protoboot(const char *fsname, const char *mntpoint,
    const char *pbdevpath);
#endif

                    
#if CONFIG_MNT_ROOTSNAP
static int __attribute__ ((noinline)) snapshot_root(int dirfd, user_addr_t name, uint32_t flags, vfs_context_t ctx);
#else
static int __attribute__ ((noinline)) snapshot_root(int dirfd, user_addr_t name, uint32_t flags, vfs_context_t ctx) __attribute__((unused));
#endif

__private_extern__
int sync_internal(void);

__private_extern__
int unlink1(vfs_context_t, vnode_t, user_addr_t, enum uio_seg, int);

static LCK_GRP_DECLARE(fd_vn_lck_grp, "fd_vnode_data");
static LCK_ATTR_DECLARE(fd_vn_lck_attr, 0, 0);

                         
static LCK_GRP_DECLARE(sync_mtx_lck_grp, "sync thread");
static LCK_MTX_DECLARE(sync_mtx_lck, &sync_mtx_lck_grp);

extern lck_rw_t rootvnode_rw_lock;

  
                                                            
                                                           
                                                
   
uint32_t mount_generation = 0;

                                                   
unsigned int vfs_nummntops = 0;

                                           
static _Atomic uint64_t mount_unique_id = 1;

extern const struct fileops vnops;
#if CONFIG_APPLEDOUBLE
extern errno_t rmdir_remove_orphaned_appleDouble(vnode_t, vfs_context_t, int *);
#endif                         

  
                                   
   

#if CONFIG_NFS_CLIENT || DEVFS || ROUTEFS
  
                                                          
   
__private_extern__
boolean_t
vfs_iskernelmount(mount_t mp)
{
	return (mp->mnt_kern_flag & MNTK_KERNEL_MOUNT) ? TRUE : FALSE;
}

__private_extern__
int
kernel_mount(char *fstype, vnode_t pvp, vnode_t vp, const char *path,
    void *data, __unused size_t datalen, int syscall_flags, uint32_t kern_flags, vfs_context_t ctx)
{
	struct nameidata nd;
	boolean_t did_namei;
	int error;

	NDINIT(&nd, LOOKUP, OP_MOUNT, FOLLOW | AUDITVNPATH1 | WANTPARENT,
	    UIO_SYSSPACE, CAST_USER_ADDR_T(path), ctx);

	  
                                                    
    
	if (vp == NULLVP) {
		error = namei(&nd);
		if (error) {
			if (kern_flags & (KERNEL_MOUNT_SNAPSHOT | KERNEL_MOUNT_VOLBYROLE_MASK)) {
				printf("failed to locate mount-on path: %s ", path);
			}
			return error;
		}
		vp = nd.ni_vp;
		pvp = nd.ni_dvp;
		did_namei = TRUE;
	} else {
		char *pnbuf = CAST_DOWN(char *, path);

		nd.ni_cnd.cn_pnbuf = pnbuf;
		nd.ni_cnd.cn_pnlen = (int)(strlen(pnbuf) + 1);
		did_namei = FALSE;
	}

	error = mount_common(fstype, pvp, vp, &nd.ni_cnd, CAST_USER_ADDR_T(data),
	    syscall_flags, kern_flags, NULL, TRUE, ctx);

	if (did_namei) {
		vnode_put(vp);
		vnode_put(pvp);
		nameidone(&nd);
	}

	return error;
}
#endif                                 

  
                       
   
              
int
mount(proc_t p, struct mount_args *uap, __unused int32_t *retval)
{
	struct __mac_mount_args muap;

	muap.type = uap->type;
	muap.path = uap->path;
	muap.flags = uap->flags;
	muap.data = uap->data;
	muap.mac_p = USER_ADDR_NULL;
	return __mac_mount(p, &muap, retval);
}

int
fmount(__unused proc_t p, struct fmount_args *uap, __unused int32_t *retval)
{
	struct componentname    cn;
	vfs_context_t           ctx = vfs_context_current();
	size_t                  dummy = 0;
	int                     error;
	int                     flags = uap->flags;
	char                    fstypename[MFSNAMELEN];
	char                    *labelstr = NULL;                                                                  
	vnode_t                 pvp;
	vnode_t                 vp;

	AUDIT_ARG(fd, uap->fd);
	AUDIT_ARG(fflags, flags);
	                                                 

	                            
	if (flags & (MNT_IMGSRC_BY_INDEX | MNT_ROOTFS)) {
		return ENOTSUP;
	}

	if (flags & MNT_UNION) {
		return EPERM;
	}

	error = copyinstr(uap->type, fstypename, MFSNAMELEN, &dummy);
	if (error) {
		return error;
	}

	if ((error = file_vnode(uap->fd, &vp)) != 0) {
		return error;
	}

	if ((error = vnode_getwithref(vp)) != 0) {
		file_drop(uap->fd);
		return error;
	}

	pvp = vnode_getparent(vp);
	if (pvp == NULL) {
		vnode_put(vp);
		file_drop(uap->fd);
		return EINVAL;
	}

	memset(&cn, 0, sizeof(struct componentname));
	cn.cn_pnbuf = zalloc_flags(ZV_NAMEI, Z_WAITOK);
	cn.cn_pnlen = MAXPATHLEN;

	if ((error = vn_getpath(vp, cn.cn_pnbuf, &cn.cn_pnlen)) != 0) {
		zfree(ZV_NAMEI, cn.cn_pnbuf);
		vnode_put(pvp);
		vnode_put(vp);
		file_drop(uap->fd);
		return error;
	}

	error = mount_common(fstypename, pvp, vp, &cn, uap->data, flags, 0, labelstr, FALSE, ctx);

	zfree(ZV_NAMEI, cn.cn_pnbuf);
	vnode_put(pvp);
	vnode_put(vp);
	file_drop(uap->fd);

	return error;
}

void
vfs_notify_mount(vnode_t pdvp)
{
	vfs_event_signal(NULL, VQ_MOUNT, (intptr_t)NULL);
	lock_vnode_and_post(pdvp, NOTE_WRITE);
}

  
               
                                                              
                                             
  
                                                                       
                                                                               
                                                    
  
                                                          
                                                        
                                                          
                                                   
                                                      
  
  
                                                  
                                                      
   
boolean_t root_fs_upgrade_try = FALSE;

int
__mac_mount(struct proc *p, register struct __mac_mount_args *uap, __unused int32_t *retval)
{
	vnode_t pvp = NULL;
	vnode_t vp = NULL;
	int need_nameidone = 0;
	vfs_context_t ctx = vfs_context_current();
	char fstypename[MFSNAMELEN];
	struct nameidata nd;
	size_t dummy = 0;
	char *labelstr = NULL;
	size_t labelsz = 0;
	int flags = uap->flags;
	int error;
#if CONFIG_IMGSRC_ACCESS || CONFIG_MACF
	boolean_t is_64bit = IS_64BIT_PROCESS(p);
#else
#pragma unused(p)
#endif
	  
                                        
    
	error = copyinstr(uap->type, fstypename, MFSNAMELEN, &dummy);
	if (error) {
		return error;
	}

	  
                               
    
	NDINIT(&nd, LOOKUP, OP_MOUNT, FOLLOW | AUDITVNPATH1 | WANTPARENT,
	    UIO_USERSPACE, uap->path, ctx);
	error = namei(&nd);
	if (error) {
		goto out;
	}
	need_nameidone = 1;
	vp = nd.ni_vp;
	pvp = nd.ni_dvp;

#ifdef CONFIG_IMGSRC_ACCESS
	                                                                   
	if (flags == MNT_IMGSRC_BY_INDEX) {
		error = relocate_imageboot_source(pvp, vp, &nd.ni_cnd, fstypename,
		    ctx, is_64bit, uap->data, (flags == MNT_IMGSRC_BY_INDEX));
		goto out;
	}
#endif                           

#if CONFIG_MACF
	  
                                                 
    
	if (uap->mac_p != USER_ADDR_NULL) {
		struct user_mac mac;
		size_t ulen = 0;

		if (is_64bit) {
			struct user64_mac mac64;
			error = copyin(uap->mac_p, &mac64, sizeof(mac64));
			mac.m_buflen = (user_size_t)mac64.m_buflen;
			mac.m_string = (user_addr_t)mac64.m_string;
		} else {
			struct user32_mac mac32;
			error = copyin(uap->mac_p, &mac32, sizeof(mac32));
			mac.m_buflen = mac32.m_buflen;
			mac.m_string = mac32.m_string;
		}
		if (error) {
			goto out;
		}
		if ((mac.m_buflen > MAC_MAX_LABEL_BUF_LEN) ||
		    (mac.m_buflen < 2)) {
			error = EINVAL;
			goto out;
		}
		labelsz = mac.m_buflen;
		labelstr = kheap_alloc(KHEAP_TEMP, labelsz, Z_WAITOK);
		error = copyinstr(mac.m_string, labelstr, mac.m_buflen, &ulen);
		if (error) {
			goto out;
		}
		AUDIT_ARG(mac_string, labelstr);
	}
#endif                  

	AUDIT_ARG(fflags, flags);

#if SECURE_KERNEL
	if (flags & MNT_UNION) {
		                                        
		error = EPERM;
		goto out;
	}
#endif

	if ((vp->v_flag & VROOT) &&
	    (vp->v_mount->mnt_flag & MNT_ROOTFS)) {
		if (!(flags & MNT_UNION)) {
			flags |= MNT_UPDATE;
		} else {
			  
                                                 
                              
                                                       
                                                        
                                                       
                                          
      
			flags = (flags & ~(MNT_UPDATE));
		}

#if SECURE_KERNEL
		if ((flags & MNT_RDONLY) == 0) {
			                                                        
			error = EPERM;
			goto out;
		}
#endif
		  
                                                           
                                                            
                                                        
                         
     
#if CHECK_CS_VALIDATION_BITMAP
		if ((flags & MNT_RDONLY) == 0) {
			root_fs_upgrade_try = TRUE;
		}
#endif
	}

	error = mount_common(fstypename, pvp, vp, &nd.ni_cnd, uap->data, flags, 0,
	    labelstr, FALSE, ctx);

out:

#if CONFIG_MACF
	kheap_free(KHEAP_DEFAULT, labelstr, labelsz);
#endif                  

	if (vp) {
		vnode_put(vp);
	}
	if (pvp) {
		vnode_put(pvp);
	}
	if (need_nameidone) {
		nameidone(&nd);
	}

	return error;
}

  
                                                        
  
             
                                                  
                                
                     
                                                  
                             
                                         
                               
                                                                
                         
   
static int
mount_common(char *fstypename, vnode_t pvp, vnode_t vp,
    struct componentname *cnp, user_addr_t fsmountargs, int flags, uint32_t internal_flags,
    char *labelstr, boolean_t kernelmount, vfs_context_t ctx)
{
#if !CONFIG_MACF
#pragma unused(labelstr)
#endif
	struct vnode *devvp = NULLVP;
	struct vnode *device_vnode = NULLVP;
#if CONFIG_MACF
	struct vnode *rvp;
#endif
	struct mount *mp;
	struct vfstable *vfsp = (struct vfstable *)0;
	struct proc *p = vfs_context_proc(ctx);
	int error, flag = 0;
	bool flag_set = false;
	user_addr_t devpath = USER_ADDR_NULL;
	int ronly = 0;
	int mntalloc = 0;
	boolean_t vfsp_ref = FALSE;
	boolean_t is_rwlock_locked = FALSE;
	boolean_t did_rele = FALSE;
	boolean_t have_usecount = FALSE;
	boolean_t did_set_lmount = FALSE;

#if CONFIG_ROSV_STARTUP || CONFIG_MOUNT_VM || CONFIG_BASESYSTEMROOT
	                                            
	uint32_t checkflags = (internal_flags & (KERNEL_MOUNT_VOLBYROLE_MASK | KERNEL_MOUNT_BASESYSTEMROOT));
	int bitcount = 0;
	while (checkflags != 0) {
		checkflags &= (checkflags - 1);
		bitcount++;
	}

	if (bitcount > 1) {
		                                                     
		error = EINVAL;
		goto out1;
	}
#endif

	  
                                           
    
	if (flags & MNT_UPDATE) {
		if ((vp->v_flag & VROOT) == 0) {
			error = EINVAL;
			goto out1;
		}
		mp = vp->v_mount;

		                                                   
		mount_lock_spin(mp);
		if (mp->mnt_lflag & (MNT_LUNMOUNT | MNT_LMOUNT)) {
			mount_unlock(mp);
			error = EBUSY;
			goto out1;
		}
		mp->mnt_lflag |= MNT_LMOUNT;
		did_set_lmount = TRUE;
		mount_unlock(mp);
		lck_rw_lock_exclusive(&mp->mnt_rwlock);
		is_rwlock_locked = TRUE;
		  
                                                      
                                    
     
		if ((flags & MNT_RELOAD) &&
		    ((mp->mnt_flag & MNT_RDONLY) == 0)) {
			error = ENOTSUP;
			goto out1;
		}

		  
                                                            
                            
     
		if ((mp->mnt_flag & MNT_CPROTECT) &&
		    ((flags & MNT_CPROTECT) == 0)) {
			error = EINVAL;
			goto out1;
		}

		  
                                                                    
                                                               
                                   
     
		if ((mp->mnt_flag & MNT_REMOVABLE) &&
		    ((flags & MNT_REMOVABLE) == 0)) {
			flags |= MNT_REMOVABLE;
		}

		                                               
		if ((mp->mnt_kern_flag & MNTK_BACKS_ROOT) &&
		    (!vfs_isrdonly(mp)) && (flags & MNT_RDONLY)) {
			error = ENOTSUP;
			goto out1;
		}

		  
                                                          
                            
     
		if (mp->mnt_vfsstat.f_owner != kauth_cred_getuid(vfs_context_ucred(ctx)) &&
		    (error = suser(vfs_context_ucred(ctx), &p->p_acflag))) {
			goto out1;
		}
#if CONFIG_MACF
		error = mac_mount_check_remount(ctx, mp);
		if (error != 0) {
			goto out1;
		}
#endif
		  
                                                                   
                                                         
     
		if ((!kernelmount) && suser(vfs_context_ucred(ctx), NULL)) {
			flags |= MNT_NOSUID | MNT_NODEV;
			if (mp->mnt_flag & MNT_NOEXEC) {
				flags |= MNT_NOEXEC;
			}
		}
		flag = mp->mnt_flag;
		flag_set = true;



		mp->mnt_flag |= flags & (MNT_RELOAD | MNT_FORCE | MNT_UPDATE);

		vfsp = mp->mnt_vtable;
		goto update;
	}              

	  
                                                                      
                                                    
    
	if ((!kernelmount) && suser(vfs_context_ucred(ctx), NULL)) {
		flags |= MNT_NOSUID | MNT_NODEV;
		if (vp->v_mount->mnt_flag & MNT_NOEXEC) {
			flags |= MNT_NOEXEC;
		}
	}

	                                                                     
	AUDIT_ARG(text, fstypename);
	mount_list_lock();
	for (vfsp = vfsconf; vfsp; vfsp = vfsp->vfc_next) {
		if (!strncmp(vfsp->vfc_name, fstypename, MFSNAMELEN)) {
			vfsp->vfc_refcount++;
			vfsp_ref = TRUE;
			break;
		}
	}
	mount_list_unlock();
	if (vfsp == NULL) {
		error = ENODEV;
		goto out1;
	}

	  
                                                                  
                                                               
    
	if (kernelmount && (vfsp->vfc_vfsflags & VFC_VFSLOCALARGS) &&
	    ((internal_flags & KERNEL_MOUNT_VOLBYROLE_MASK) == 0) &&
	    ((internal_flags & KERNEL_MOUNT_BASESYSTEMROOT) == 0)) {
		error = EINVAL;                           
		goto out1;
	}

	error = prepare_coveredvp(vp, ctx, cnp, fstypename, ((internal_flags & KERNEL_MOUNT_NOAUTH) != 0));
	if (error != 0) {
		goto out1;
	}

	  
                                                    
    
	mp = zalloc_flags(mount_zone, Z_WAITOK | Z_ZERO);
	mntalloc = 1;

	                                           
	mp->mnt_maxreadcnt = mp->mnt_maxwritecnt = MAXPHYS;
	mp->mnt_segreadcnt = mp->mnt_segwritecnt = 32;
	mp->mnt_maxsegreadsize = mp->mnt_maxreadcnt;
	mp->mnt_maxsegwritesize = mp->mnt_maxwritecnt;
	mp->mnt_devblocksize = DEV_BSIZE;
	mp->mnt_alignmentmask = PAGE_MASK;
	mp->mnt_ioqueue_depth = MNT_DEFAULT_IOQUEUE_DEPTH;
	mp->mnt_ioscale = 1;
	mp->mnt_ioflags = 0;
	mp->mnt_realrootvp = NULLVP;
	mp->mnt_authcache_ttl = CACHED_LOOKUP_RIGHT_TTL;

	mp->mnt_lflag |= MNT_LMOUNT;
	did_set_lmount = TRUE;

	TAILQ_INIT(&mp->mnt_vnodelist);
	TAILQ_INIT(&mp->mnt_workerqueue);
	TAILQ_INIT(&mp->mnt_newvnodes);
	mount_lock_init(mp);
	lck_rw_lock_exclusive(&mp->mnt_rwlock);
	is_rwlock_locked = TRUE;
	mp->mnt_op = vfsp->vfc_vfsops;
	mp->mnt_vtable = vfsp;
	                                          
	mp->mnt_flag |= vfsp->vfc_flags & MNT_VISFLAGMASK;
	strlcpy(mp->mnt_vfsstat.f_fstypename, vfsp->vfc_name, MFSTYPENAMELEN);
	do {
		int pathlen = MAXPATHLEN;

		if (vn_getpath_ext(vp, pvp, mp->mnt_vfsstat.f_mntonname, &pathlen, VN_GETPATH_FSENTER)) {
			strlcpy(mp->mnt_vfsstat.f_mntonname, cnp->cn_pnbuf, MAXPATHLEN);
		}
	} while (0);
	mp->mnt_vnodecovered = vp;
	mp->mnt_vfsstat.f_owner = kauth_cred_getuid(vfs_context_ucred(ctx));
	mp->mnt_throttle_mask = LOWPRI_MAX_NUM_DEV - 1;
	mp->mnt_devbsdunit = 0;
	mp->mnt_mount_id = os_atomic_inc_orig(&mount_unique_id, relaxed);

	                                                                                      
	vfs_setowner(mp, KAUTH_UID_NONE, KAUTH_GID_NONE);

#if CONFIG_NFS_CLIENT || DEVFS || ROUTEFS
	if (kernelmount) {
		mp->mnt_kern_flag |= MNTK_KERNEL_MOUNT;
	}
	if ((internal_flags & KERNEL_MOUNT_PERMIT_UNMOUNT) != 0) {
		mp->mnt_kern_flag |= MNTK_PERMIT_UNMOUNT;
	}
#endif                                 

	if (KERNEL_MOUNT_DEVFS & internal_flags) {
		                       
		mp->mnt_kern_flag |= MNTK_SYSTEM;
	}

update:

	  
                              
    
	if (flags & MNT_RDONLY) {
		mp->mnt_flag |= MNT_RDONLY;
	} else if (mp->mnt_flag & MNT_RDONLY) {
		                                                    
		                                         
		if (mp->mnt_kern_flag & MNTK_TYPENAME_OVERRIDE) {
			error = EPERM;
			goto out1;
		}
		mp->mnt_kern_flag |= MNTK_WANTRDWR;
	}
	mp->mnt_flag &= ~(MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
	    MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC |
	    MNT_UNKNOWNPERMISSIONS | MNT_DONTBROWSE |
	    MNT_AUTOMOUNTED | MNT_DEFWRITE | MNT_NOATIME | MNT_STRICTATIME |
	    MNT_QUARANTINE | MNT_CPROTECT);

#if SECURE_KERNEL
#if !CONFIG_MNT_SUID
	  
                                                                      
                                                                             
                                   
    
	mp->mnt_flag |= (MNT_NOSUID);
#endif
#endif

	mp->mnt_flag |= flags & (MNT_NOSUID | MNT_NOEXEC | MNT_NODEV |
	    MNT_SYNCHRONOUS | MNT_UNION | MNT_ASYNC |
	    MNT_UNKNOWNPERMISSIONS | MNT_DONTBROWSE |
	    MNT_AUTOMOUNTED | MNT_DEFWRITE | MNT_NOATIME | MNT_STRICTATIME |
	    MNT_QUARANTINE | MNT_CPROTECT);

#if CONFIG_MACF
	if (flags & MNT_MULTILABEL) {
		if (vfsp->vfc_vfsflags & VFC_VFSNOMACLABEL) {
			error = EINVAL;
			goto out1;
		}
		mp->mnt_flag |= MNT_MULTILABEL;
	}
#endif
	  
                                                            
   
                                                                    
                                                       
   
                                                                 
                                                           
                                                                    
                                                                     
                
    
	if (vfsp->vfc_vfsflags & VFC_VFSLOCALARGS &&
	    !(internal_flags & (KERNEL_MOUNT_SNAPSHOT | KERNEL_MOUNT_VOLBYROLE_MASK))) {
		boolean_t do_copyin_devpath = true;
#if CONFIG_BASESYSTEMROOT
		if (internal_flags & KERNEL_MOUNT_BASESYSTEMROOT) {
			                                                                    
			                                                                    
			                                                                
			                                                                
			                                                            
			                           
			                                                                  
			                                                               
			_Static_assert(sizeof(char **) == sizeof(fsmountargs), "fsmountargs should fit a (kernel) address");
			char **devnamepp = (char **)fsmountargs;
			char *devnamep = *devnamepp;
			devpath = CAST_USER_ADDR_T(devnamep);
			do_copyin_devpath = false;
			fsmountargs = USER_ADDR_NULL;

			                                                                      
			mp->mnt_supl_kern_flag |= MNTK_SUPL_BASESYSTEM;
		}
#endif                         

		if (do_copyin_devpath) {
			if (vfs_context_is64bit(ctx)) {
				if ((error = copyin(fsmountargs, (caddr_t)&devpath, sizeof(devpath)))) {
					goto out1;
				}
				fsmountargs += sizeof(devpath);
			} else {
				user32_addr_t tmp;
				if ((error = copyin(fsmountargs, (caddr_t)&tmp, sizeof(tmp)))) {
					goto out1;
				}
				                          
				devpath = CAST_USER_ADDR_T(tmp);
				fsmountargs += sizeof(tmp);
			}
		}

		                                              
		if ((devpath)) {
			struct nameidata nd;

			enum uio_seg seg = UIO_USERSPACE;
#if CONFIG_BASESYSTEMROOT
			if (internal_flags & KERNEL_MOUNT_BASESYSTEMROOT) {
				seg = UIO_SYSSPACE;
			}
#endif                         

			NDINIT(&nd, LOOKUP, OP_MOUNT, FOLLOW, seg, devpath, ctx);
			if ((error = namei(&nd))) {
				goto out1;
			}

			strlcpy(mp->mnt_vfsstat.f_mntfromname, nd.ni_cnd.cn_pnbuf, MAXPATHLEN);
			devvp = nd.ni_vp;

			nameidone(&nd);

			if (devvp->v_type != VBLK) {
				error = ENOTBLK;
				goto out2;
			}
			if (major(devvp->v_rdev) >= nblkdev) {
				error = ENXIO;
				goto out2;
			}
			  
                                                               
                                
      
			if (suser(vfs_context_ucred(ctx), NULL) != 0) {
				mode_t accessmode = KAUTH_VNODE_READ_DATA;

				if ((mp->mnt_flag & MNT_RDONLY) == 0) {
					accessmode |= KAUTH_VNODE_WRITE_DATA;
				}
				if ((error = vnode_authorize(devvp, NULL, accessmode, ctx)) != 0) {
					goto out2;
				}
			}
		}
		                                               
		if (devpath && ((flags & MNT_UPDATE) == 0)) {
			if ((error = vnode_ref(devvp))) {
				goto out2;
			}
			  
                                                  
                                                            
                                                                    
                                                              
      
			if ((error = vfs_mountedon(devvp))) {
				goto out3;
			}

			if (vcount(devvp) > 1 && !(vfs_flags(mp) & MNT_ROOTFS)) {
				error = EBUSY;
				goto out3;
			}
			if ((error = VNOP_FSYNC(devvp, MNT_WAIT, ctx))) {
				error = ENOTBLK;
				goto out3;
			}
			if ((error = buf_invalidateblks(devvp, BUF_WRITE_DATA, 0, 0))) {
				goto out3;
			}

			ronly = (mp->mnt_flag & MNT_RDONLY) != 0;
#if CONFIG_MACF
			error = mac_vnode_check_open(ctx,
			    devvp,
			    ronly ? FREAD : FREAD | FWRITE);
			if (error) {
				goto out3;
			}
#endif          
			if ((error = VNOP_OPEN(devvp, ronly ? FREAD : FREAD | FWRITE, ctx))) {
				goto out3;
			}

			mp->mnt_devvp = devvp;
			device_vnode = devvp;
		} else if ((mp->mnt_flag & MNT_RDONLY) &&
		    (mp->mnt_kern_flag & MNTK_WANTRDWR) &&
		    (device_vnode = mp->mnt_devvp)) {
			dev_t dev;
			int maj;
			  
                                                       
                                                        
      
			vnode_getalways(device_vnode);

			if (suser(vfs_context_ucred(ctx), NULL) &&
			    (error = vnode_authorize(device_vnode, NULL,
			    KAUTH_VNODE_READ_DATA | KAUTH_VNODE_WRITE_DATA,
			    ctx)) != 0) {
				vnode_put(device_vnode);
				goto out2;
			}

			                                          
			dev = (dev_t)device_vnode->v_rdev;
			maj = major(dev);

			if ((u_int)maj >= (u_int)nblkdev) {
				panic("Volume mounted on a device with invalid major number.");
			}

			error = bdevsw[maj].d_open(dev, FREAD | FWRITE, S_IFBLK, p);
			vnode_put(device_vnode);
			device_vnode = NULLVP;
			if (error != 0) {
				goto out2;
			}
		}
	}                                        

#if CONFIG_MACF
	if ((flags & MNT_UPDATE) == 0) {
		mac_mount_label_init(mp);
		mac_mount_label_associate(ctx, mp);
	}
	if (labelstr) {
		if ((flags & MNT_UPDATE) != 0) {
			error = mac_mount_check_label_update(ctx, mp);
			if (error != 0) {
				goto out3;
			}
		}
	}
#endif
	  
                                                                  
                                                    
    
	if (internal_flags & KERNEL_MOUNT_SNAPSHOT) {
		error = VFS_IOCTL(mp, VFSIOC_MOUNT_SNAPSHOT,
		    (caddr_t)fsmountargs, 0, ctx);
	} else if (internal_flags & KERNEL_MOUNT_DATAVOL) {
#if CONFIG_ROSV_STARTUP
		struct mount *origin_mp = (struct mount*)fsmountargs;
		fs_role_mount_args_t frma = {origin_mp, VFS_DATA_ROLE};
		error = VFS_IOCTL(mp, VFSIOC_MOUNT_BYROLE, (caddr_t)&frma, 0, ctx);
		if (error) {
			printf("MOUNT-BY-ROLE (%d) failed! (%d)", VFS_DATA_ROLE, error);
		} else {
			                                               
			mp->mnt_kern_flag |= MNTK_SYSTEM;

			                                                    
			struct vnode *mp_devvp = NULL;
			if (mp->mnt_vfsstat.f_mntfromname[0] != 0) {
				errno_t lerr = vnode_lookup(mp->mnt_vfsstat.f_mntfromname,
				    0, &mp_devvp, vfs_context_kernel());
				if (!lerr) {
					mp->mnt_devvp = mp_devvp;
					                                                
					vnode_put(mp_devvp);
					                                                         
					                                                                       
					                                                                     
					                                            
					device_vnode = mp_devvp;
				}
			}
		}
#else
		error = EINVAL;
#endif
	} else if (internal_flags & KERNEL_MOUNT_VMVOL) {
#if CONFIG_MOUNT_VM
		struct mount *origin_mp = (struct mount*)fsmountargs;
		fs_role_mount_args_t frma = {origin_mp, VFS_VM_ROLE};
		error = VFS_IOCTL(mp, VFSIOC_MOUNT_BYROLE, (caddr_t)&frma, 0, ctx);
		if (error) {
			printf("MOUNT-BY-ROLE (%d) failed! (%d)", VFS_VM_ROLE, error);
		} else {
			                                                                
			mp->mnt_kern_flag |= (MNTK_SYSTEM | MNTK_SWAP_MOUNT);
			                                                    
			struct vnode *mp_devvp = NULL;
			if (mp->mnt_vfsstat.f_mntfromname[0] != 0) {
				errno_t lerr = vnode_lookup(mp->mnt_vfsstat.f_mntfromname,
				    0, &mp_devvp, vfs_context_kernel());
				if (!lerr) {
					mp->mnt_devvp = mp_devvp;
					                                                
					vnode_put(mp_devvp);

					                                                         
					                                                                     
					                                            
					device_vnode = mp_devvp;
				}
			}
		}
#else
		error = EINVAL;
#endif
	} else if ((internal_flags & KERNEL_MOUNT_PREBOOTVOL) || (internal_flags & KERNEL_MOUNT_RECOVERYVOL)) {
#if CONFIG_MOUNT_PREBOOTRECOVERY
		struct mount *origin_mp = (struct mount*)fsmountargs;
		uint32_t mount_role = 0;
		if (internal_flags & KERNEL_MOUNT_PREBOOTVOL) {
			mount_role = VFS_PREBOOT_ROLE;
		} else if (internal_flags & KERNEL_MOUNT_RECOVERYVOL) {
			mount_role = VFS_RECOVERY_ROLE;
		}

		if (mount_role != 0) {
			fs_role_mount_args_t frma = {origin_mp, mount_role};
			error = VFS_IOCTL(mp, VFSIOC_MOUNT_BYROLE, (caddr_t)&frma, 0, ctx);
			if (error) {
				printf("MOUNT-BY-ROLE (%d) failed! (%d)", mount_role, error);
			} else {
				                                                                           
				                                               
				                                   
				                                                    
				struct vnode *mp_devvp = NULL;
				if (mp->mnt_vfsstat.f_mntfromname[0] != 0) {
					errno_t lerr = vnode_lookup(mp->mnt_vfsstat.f_mntfromname,
					    0, &mp_devvp, vfs_context_kernel());
					if (!lerr) {
						mp->mnt_devvp = mp_devvp;
						                                                
						vnode_put(mp_devvp);

						                                                         
						                                                                     
						                                            
						device_vnode = mp_devvp;
					}
				}
			}
		} else {
			printf("MOUNT-BY-ROLE (%d) failed - ROLE UNRECOGNIZED! (%d)", mount_role, error);
			error = EINVAL;
		}
#else
		error = EINVAL;
#endif
	} else {
		error = VFS_MOUNT(mp, device_vnode, fsmountargs, ctx);
	}

	if (flags & MNT_UPDATE) {
		if (mp->mnt_kern_flag & MNTK_WANTRDWR) {
			mp->mnt_flag &= ~MNT_RDONLY;
		}
		mp->mnt_flag &= ~
		    (MNT_UPDATE | MNT_RELOAD | MNT_FORCE);
		mp->mnt_kern_flag &= ~MNTK_WANTRDWR;
		if (error) {
			mp->mnt_flag = flag;                          
		}
		vfs_event_signal(NULL, VQ_UPDATE, (intptr_t)NULL);
		lck_rw_done(&mp->mnt_rwlock);
		is_rwlock_locked = FALSE;
		if (!error) {
			enablequotas(mp, ctx);
		}
		goto exit;
	}

	  
                                                        
    
	if (error == 0) {
		struct vfs_attr vfsattr;
#if CONFIG_MACF
		error = mac_mount_check_mount_late(ctx, mp);
		if (error != 0) {
			goto out4;
		}

		if (vfs_flags(mp) & MNT_MULTILABEL) {
			error = VFS_ROOT(mp, &rvp, ctx);
			if (error) {
				printf("%s() VFS_ROOT returned %d\n", __func__, error);
				goto out4;
			}
			error = vnode_label(mp, NULL, rvp, NULL, 0, ctx);
			  
                                         
      
			vnode_put(rvp);

			if (error) {
				goto out4;
			}
		}
#endif           

		vnode_lock_spin(vp);
		CLR(vp->v_flag, VMOUNT);
		vp->v_mountedhere = mp;
		vnode_unlock(vp);

		  
                                                
                                                     
                                               
                                      
                                                      
                      
     
		name_cache_lock();
		mount_generation++;
		name_cache_unlock();

		error = vnode_ref(vp);
		if (error != 0) {
			goto out4;
		}

		have_usecount = TRUE;

		error = checkdirs(vp, ctx);
		if (error != 0) {
			                                                            
			goto out4;
		}
		  
                                                         
                            
     
		(void)VFS_START(mp, 0, ctx);

		if (mount_list_add(mp) != 0) {
			  
                                                  
                                                 
      
			error = EBUSY;
			goto out4;
		}
		lck_rw_done(&mp->mnt_rwlock);
		is_rwlock_locked = FALSE;

		                                                                      
		                                                                           
		VFSATTR_INIT(&vfsattr);
		VFSATTR_WANTED(&vfsattr, f_capabilities);
		if (strncmp(mp->mnt_vfsstat.f_fstypename, "webdav", sizeof("webdav")) != 0 &&
		    vfs_getattr(mp, &vfsattr, ctx) == 0 &&
		    VFSATTR_IS_SUPPORTED(&vfsattr, f_capabilities)) {
			if ((vfsattr.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] & VOL_CAP_INT_EXTENDED_ATTR) &&
			    (vfsattr.f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] & VOL_CAP_INT_EXTENDED_ATTR)) {
				mp->mnt_kern_flag |= MNTK_EXTENDED_ATTRS;
			}
#if NAMEDSTREAMS
			if ((vfsattr.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] & VOL_CAP_INT_NAMEDSTREAMS) &&
			    (vfsattr.f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] & VOL_CAP_INT_NAMEDSTREAMS)) {
				mp->mnt_kern_flag |= MNTK_NAMED_STREAMS;
			}
#endif
			                                                              
			if ((vfsattr.f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_PATH_FROM_ID) &&
			    (vfsattr.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_PATH_FROM_ID)) {
				mp->mnt_kern_flag |= MNTK_PATH_FROM_ID;
			} else if (mp->mnt_flag & MNT_DOVOLFS) {
				                                                                
				mp->mnt_kern_flag |= MNTK_PATH_FROM_ID;
			}

			if ((vfsattr.f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_DIR_HARDLINKS) &&
			    (vfsattr.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_DIR_HARDLINKS)) {
				mp->mnt_kern_flag |= MNTK_DIR_HARDLINKS;
			}
		}
		if (mp->mnt_vtable->vfc_vfsflags & VFC_VFSNATIVEXATTR) {
			mp->mnt_kern_flag |= MNTK_EXTENDED_ATTRS;
		}
		if (mp->mnt_vtable->vfc_vfsflags & VFC_VFSPREFLIGHT) {
			mp->mnt_kern_flag |= MNTK_UNMOUNT_PREFLIGHT;
		}
		                                    
		OSAddAtomic(1, &vfs_nummntops);
		enablequotas(mp, ctx);

		if (device_vnode) {
			device_vnode->v_specflags |= SI_MOUNTEDON;

			  
                                                                    
                                                               
                                                                
                                                                 
      
			vfs_init_io_attributes(device_vnode, mp);
		}

		                                                   
		vfs_notify_mount(pvp);
		IOBSDMountChange(mp, kIOMountChangeMount);
	} else {
		                                                                                          
		if (mp->mnt_vnodelist.tqh_first != NULL) {
			panic("mount_common(): mount of %s filesystem failed with %d, but vnode list is not empty.",
			    mp->mnt_vtable->vfc_name, error);
		}

		vnode_lock_spin(vp);
		CLR(vp->v_flag, VMOUNT);
		vnode_unlock(vp);
		mount_list_lock();
		mp->mnt_vtable->vfc_refcount--;
		mount_list_unlock();

		if (device_vnode) {
			vnode_rele(device_vnode);
			VNOP_CLOSE(device_vnode, ronly ? FREAD : FREAD | FWRITE, ctx);
		}
		lck_rw_done(&mp->mnt_rwlock);
		is_rwlock_locked = FALSE;

		  
                                                                      
                                                                    
                                                                       
                               
     
		mount_lock_destroy(mp);
#if CONFIG_MACF
		mac_mount_label_destroy(mp);
#endif
		zfree(mount_zone, mp);
		did_set_lmount = false;
	}
exit:
	  
                                                    
    
	if (devpath && devvp) {
		vnode_put(devvp);
	}

	if (did_set_lmount) {
		mount_lock_spin(mp);
		mp->mnt_lflag &= ~MNT_LMOUNT;
		mount_unlock(mp);
	}

	return error;

                           
out4:
	(void)VFS_UNMOUNT(mp, MNT_FORCE, ctx);

	  
                                                   
                                                  
                                      
    
	mount_lock_spin(mp);
	mp->mnt_lflag |= MNT_LDEAD;
	mount_unlock(mp);

	if (device_vnode != NULLVP) {
		vnode_rele(device_vnode);
		VNOP_CLOSE(device_vnode, mp->mnt_flag & MNT_RDONLY ? FREAD : FREAD | FWRITE,
		    ctx);
		did_rele = TRUE;
	}

	vnode_lock_spin(vp);

	mp->mnt_crossref++;
	vp->v_mountedhere = (mount_t) 0;

	vnode_unlock(vp);

	if (have_usecount) {
		vnode_rele(vp);
	}
out3:
	if (devpath && ((flags & MNT_UPDATE) == 0) && (!did_rele)) {
		vnode_rele(devvp);
	}
out2:
	if (devpath && devvp) {
		vnode_put(devvp);
	}
out1:
	                                               
	if (is_rwlock_locked == TRUE) {
		if (flag_set) {
			mp->mnt_flag = flag;                              
		}
		lck_rw_done(&mp->mnt_rwlock);
	}

	if (did_set_lmount) {
		mount_lock_spin(mp);
		mp->mnt_lflag &= ~MNT_LMOUNT;
		mount_unlock(mp);
	}

	if (mntalloc) {
		if (mp->mnt_crossref) {
			mount_dropcrossref(mp, vp, 0);
		} else {
			mount_lock_destroy(mp);
#if CONFIG_MACF
			mac_mount_label_destroy(mp);
#endif
			zfree(mount_zone, mp);
		}
	}
	if (vfsp_ref) {
		mount_list_lock();
		vfsp->vfc_refcount--;
		mount_list_unlock();
	}

	return error;
}

  
                                                          
                 
   
int
prepare_coveredvp(vnode_t vp, vfs_context_t ctx, struct componentname *cnp, const char *fsname, boolean_t skip_auth)
{
#if !CONFIG_MACF
#pragma unused(cnp,fsname)
#endif
	struct vnode_attr va;
	int error;

	if (!skip_auth) {
		  
                                                                
                                           
     
		VATTR_INIT(&va);
		VATTR_WANTED(&va, va_uid);
		if ((error = vnode_getattr(vp, &va, ctx)) ||
		    (va.va_uid != kauth_cred_getuid(vfs_context_ucred(ctx)) &&
		    (!vfs_context_issuser(ctx)))) {
			error = EPERM;
			goto out;
		}
	}

	if ((error = VNOP_FSYNC(vp, MNT_WAIT, ctx))) {
		goto out;
	}

	if ((error = buf_invalidateblks(vp, BUF_WRITE_DATA, 0, 0))) {
		goto out;
	}

	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}

	if (ISSET(vp->v_flag, VMOUNT) && (vp->v_mountedhere != NULL)) {
		error = EBUSY;
		goto out;
	}

#if CONFIG_MACF
	error = mac_mount_check_mount(ctx, vp,
	    cnp, fsname);
	if (error != 0) {
		goto out;
	}
#endif

	vnode_lock_spin(vp);
	SET(vp->v_flag, VMOUNT);
	vnode_unlock(vp);

out:
	return error;
}

#if CONFIG_IMGSRC_ACCESS

#define DEBUG_IMGSRC 0

#if DEBUG_IMGSRC
#define IMGSRC_DEBUG(args...) printf("imgsrc: " args)
#else
#define IMGSRC_DEBUG(args...) do { } while(0)
#endif

static int
authorize_devpath_and_update_mntfromname(mount_t mp, user_addr_t devpath, vnode_t *devvpp, vfs_context_t ctx)
{
	struct nameidata nd;
	vnode_t vp, realdevvp;
	mode_t accessmode;
	int error;
	enum uio_seg uio = UIO_USERSPACE;

	if (ctx == vfs_context_kernel()) {
		uio = UIO_SYSSPACE;
	}

	NDINIT(&nd, LOOKUP, OP_LOOKUP, FOLLOW, uio, devpath, ctx);
	if ((error = namei(&nd))) {
		IMGSRC_DEBUG("namei() failed with %d\n", error);
		return error;
	}

	vp = nd.ni_vp;

	if (!vnode_isblk(vp)) {
		IMGSRC_DEBUG("Not block device.\n");
		error = ENOTBLK;
		goto out;
	}

	realdevvp = mp->mnt_devvp;
	if (realdevvp == NULLVP) {
		IMGSRC_DEBUG("No device backs the mount.\n");
		error = ENXIO;
		goto out;
	}

	error = vnode_getwithref(realdevvp);
	if (error != 0) {
		IMGSRC_DEBUG("Coudn't get iocount on device.\n");
		goto out;
	}

	if (vnode_specrdev(vp) != vnode_specrdev(realdevvp)) {
		IMGSRC_DEBUG("Wrong dev_t.\n");
		error = ENXIO;
		goto out1;
	}

	strlcpy(mp->mnt_vfsstat.f_mntfromname, nd.ni_cnd.cn_pnbuf, MAXPATHLEN);

	  
                                                             
                              
    
	if (!vfs_context_issuser(ctx)) {
		accessmode = KAUTH_VNODE_READ_DATA;
		if ((mp->mnt_flag & MNT_RDONLY) == 0) {
			accessmode |= KAUTH_VNODE_WRITE_DATA;
		}
		if ((error = vnode_authorize(vp, NULL, accessmode, ctx)) != 0) {
			IMGSRC_DEBUG("Access denied.\n");
			goto out1;
		}
	}

	*devvpp = vp;

out1:
	vnode_put(realdevvp);

out:
	nameidone(&nd);

	if (error) {
		vnode_put(vp);
	}

	return error;
}

  
                                                                        
                       
   
static int
place_mount_and_checkdirs(mount_t mp, vnode_t vp, vfs_context_t ctx)
{
	int error;

	mp->mnt_vnodecovered = vp;                                                     

	IMGSRC_DEBUG("placing: fsname = %s, vp = %s\n",
	    mp->mnt_vtable->vfc_name, vnode_getname(vp));

	vnode_lock_spin(vp);
	CLR(vp->v_flag, VMOUNT);
	vp->v_mountedhere = mp;
	vnode_unlock(vp);

	  
                                               
                                                    
                                              
                                     
                                                     
                     
    
	name_cache_lock();
	mount_generation++;
	name_cache_unlock();

	error = vnode_ref(vp);
	if (error != 0) {
		goto out;
	}

	error = checkdirs(vp, ctx);
	if (error != 0) {
		                                                            
		vnode_rele(vp);
		goto out;
	}

out:
	if (error != 0) {
		mp->mnt_vnodecovered = NULLVP;
	}
	return error;
}

static void
undo_place_on_covered_vp(mount_t mp, vnode_t vp)
{
	vnode_rele(vp);
	vnode_lock_spin(vp);
	vp->v_mountedhere = (mount_t)NULL;
	vnode_unlock(vp);

	mp->mnt_vnodecovered = NULLVP;
}

static int
mount_begin_update(mount_t mp, vfs_context_t ctx, int flags)
{
	int error;

	                                      
	mount_lock_spin(mp);
	if (mp->mnt_lflag & (MNT_LUNMOUNT | MNT_LMOUNT)) {
		mount_unlock(mp);
		return EBUSY;
	}
	mount_unlock(mp);
	lck_rw_lock_exclusive(&mp->mnt_rwlock);

	  
                                                     
                                   
    
	if ((flags & MNT_RELOAD) &&
	    ((mp->mnt_flag & MNT_RDONLY) == 0)) {
		error = ENOTSUP;
		goto out;
	}

	  
                                                         
                           
    
	if (mp->mnt_vfsstat.f_owner != kauth_cred_getuid(vfs_context_ucred(ctx)) &&
	    (!vfs_context_issuser(ctx))) {
		error = EPERM;
		goto out;
	}
#if CONFIG_MACF
	error = mac_mount_check_remount(ctx, mp);
	if (error != 0) {
		goto out;
	}
#endif

out:
	if (error) {
		lck_rw_done(&mp->mnt_rwlock);
	}

	return error;
}

static void
mount_end_update(mount_t mp)
{
	lck_rw_done(&mp->mnt_rwlock);
}

static int
get_imgsrc_rootvnode(uint32_t height, vnode_t *rvpp)
{
	vnode_t vp;

	if (height >= MAX_IMAGEBOOT_NESTING) {
		return EINVAL;
	}

	vp = imgsrc_rootvnodes[height];
	if ((vp != NULLVP) && (vnode_get(vp) == 0)) {
		*rvpp = vp;
		return 0;
	} else {
		return ENOENT;
	}
}

static int
relocate_imageboot_source(vnode_t pvp, vnode_t vp,
    struct componentname *cnp, const char *fsname, vfs_context_t ctx,
    boolean_t is64bit, user_addr_t fsmountargs, boolean_t by_index)
{
	int error;
	mount_t mp;
	boolean_t placed = FALSE;
	struct vfstable *vfsp;
	user_addr_t devpath;
	char *old_mntonname;
	vnode_t rvp;
	vnode_t devvp;
	uint32_t height;
	uint32_t flags;

	                                             
	if (imgsrc_rootvnodes[0] == NULLVP) {
		return EINVAL;
	}

	                           
	if (!vfs_context_issuser(ctx)) {
		return EPERM;
	}

	IMGSRC_DEBUG("looking for root vnode.\n");

	  
                                              
    
	if (by_index) {
		if (is64bit) {
			struct user64_mnt_imgsrc_args mia64;
			error = copyin(fsmountargs, &mia64, sizeof(mia64));
			if (error != 0) {
				IMGSRC_DEBUG("Failed to copy in arguments.\n");
				return error;
			}

			height = mia64.mi_height;
			flags = mia64.mi_flags;
			devpath = (user_addr_t)mia64.mi_devpath;
		} else {
			struct user32_mnt_imgsrc_args mia32;
			error = copyin(fsmountargs, &mia32, sizeof(mia32));
			if (error != 0) {
				IMGSRC_DEBUG("Failed to copy in arguments.\n");
				return error;
			}

			height = mia32.mi_height;
			flags = mia32.mi_flags;
			devpath = mia32.mi_devpath;
		}
	} else {
		  
                                                            
     
		if (is64bit) {
			if ((error = copyin(fsmountargs, (caddr_t)&devpath, sizeof(devpath)))) {
				return error;
			}
		} else {
			user32_addr_t tmp;
			if ((error = copyin(fsmountargs, (caddr_t)&tmp, sizeof(tmp)))) {
				return error;
			}

			                          
			devpath = CAST_USER_ADDR_T(tmp);
		}

		height = 0;
		flags = 0;
	}

	if (flags != 0) {
		IMGSRC_DEBUG("%s: Got nonzero flags.\n", __FUNCTION__);
		return EINVAL;
	}

	error = get_imgsrc_rootvnode(height, &rvp);
	if (error != 0) {
		IMGSRC_DEBUG("getting old root vnode failed with %d\n", error);
		return error;
	}

	IMGSRC_DEBUG("got old root vnode\n");

	old_mntonname = zalloc_flags(ZV_NAMEI, Z_WAITOK);

	                        
	mp = vnode_mount(rvp);
	if ((mp->mnt_kern_flag & MNTK_HAS_MOVED) == MNTK_HAS_MOVED) {
		IMGSRC_DEBUG("Already moved.\n");
		error = EBUSY;
		goto out0;
	}

	IMGSRC_DEBUG("moving rvp: fsname = %s\n", mp->mnt_vtable->vfc_name);
	IMGSRC_DEBUG("Starting updated.\n");

	                                                           
	error = mount_begin_update(mp, ctx, 0);
	if (error != 0) {
		IMGSRC_DEBUG("Starting updated failed with %d\n", error);
		goto out0;
	}

	  
                                                             
                                 
    
	if ((mp->mnt_kern_flag & MNTK_HAS_MOVED) == MNTK_HAS_MOVED) {
		IMGSRC_DEBUG("Already moved [2]\n");
		goto out1;
	}

	IMGSRC_DEBUG("Preparing coveredvp.\n");

	                                                                             
	error = prepare_coveredvp(vp, ctx, cnp, fsname, FALSE);
	if (error != 0) {
		IMGSRC_DEBUG("Preparing coveredvp failed with %d.\n", error);
		goto out1;
	}

	IMGSRC_DEBUG("Covered vp OK.\n");

	                                               
	vfsp = mp->mnt_vtable;
	if (strncmp(vfsp->vfc_name, fsname, MFSNAMELEN) != 0) {
		IMGSRC_DEBUG("Wrong fs name: actual = %s, expected = %s\n",
		    vfsp->vfc_name, fsname);
		error = EINVAL;
		goto out2;
	}

	                                                                              
	if (vfsp->vfc_vfsflags & VFC_VFSLOCALARGS) {
		IMGSRC_DEBUG("Local, doing device validation.\n");

		if (devpath != USER_ADDR_NULL) {
			error = authorize_devpath_and_update_mntfromname(mp, devpath, &devvp, ctx);
			if (error) {
				IMGSRC_DEBUG("authorize_devpath_and_update_mntfromname() failed.\n");
				goto out2;
			}

			vnode_put(devvp);
		}
	}

	  
                                                               
                                                   
    

	IMGSRC_DEBUG("About to call place_mount_and_checkdirs().\n");
	error = place_mount_and_checkdirs(mp, vp, ctx);
	if (error != 0) {
		goto out2;
	}

	placed = TRUE;

	strlcpy(old_mntonname, mp->mnt_vfsstat.f_mntonname, MAXPATHLEN);
	strlcpy(mp->mnt_vfsstat.f_mntonname, cnp->cn_pnbuf, MAXPATHLEN);

	                         
	mount_lock(mp);
	mp->mnt_kern_flag |= MNTK_HAS_MOVED;
	mount_unlock(mp);

	                                                        
	if (mount_list_add(mp) != 0) {
		  
                                                 
                                                
     
		error = EBUSY;
		goto out3;
	}

	mount_end_update(mp);
	vnode_put(rvp);
	zfree(ZV_NAMEI, old_mntonname);

	vfs_notify_mount(pvp);

	return 0;
out3:
	strlcpy(mp->mnt_vfsstat.f_mntonname, old_mntonname, MAXPATHLEN);

	mount_lock(mp);
	mp->mnt_kern_flag &= ~(MNTK_HAS_MOVED);
	mount_unlock(mp);

out2:
	  
                                              
                                            
    
	if (placed) {
		                                                 
		undo_place_on_covered_vp(mp, vp);
	} else {
		vnode_lock_spin(vp);
		CLR(vp->v_flag, VMOUNT);
		vnode_unlock(vp);
	}
out1:
	mount_end_update(mp);

out0:
	vnode_put(rvp);
	zfree(ZV_NAMEI, old_mntonname);
	return error;
}

#if CONFIG_LOCKERBOOT
__private_extern__
int
mount_locker_protoboot(const char *fsname, const char *mntpoint,
    const char *pbdevpath)
{
	int error = -1;
	struct nameidata nd;
	boolean_t cleanup_nd = FALSE;
	vfs_context_t ctx = vfs_context_kernel();
	boolean_t is64 = TRUE;
	boolean_t by_index = TRUE;
	struct user64_mnt_imgsrc_args mia64 = {
		.mi_height = 0,
		.mi_flags = 0,
		.mi_devpath = CAST_USER_ADDR_T(pbdevpath),
	};
	user_addr_t mia64addr = CAST_USER_ADDR_T(&mia64);

	NDINIT(&nd, LOOKUP, OP_MOUNT, FOLLOW | AUDITVNPATH1 | WANTPARENT,
	    UIO_SYSSPACE, CAST_USER_ADDR_T(mntpoint), ctx);
	error = namei(&nd);
	if (error) {
		IMGSRC_DEBUG("namei: %d\n", error);
		goto out;
	}

	cleanup_nd = TRUE;
	error = relocate_imageboot_source(nd.ni_dvp, nd.ni_vp,
	    &nd.ni_cnd, fsname, ctx, is64, mia64addr, by_index);

out:
	if (cleanup_nd) {
		int stashed = error;

		error = vnode_put(nd.ni_vp);
		if (error) {
			panic("vnode_put() returned non-zero: %d", error);
		}

		if (nd.ni_dvp) {
			error = vnode_put(nd.ni_dvp);
			if (error) {
				panic("vnode_put() returned non-zero: %d", error);
			}
		}
		nameidone(&nd);

		error = stashed;
	}
	return error;
}
#endif                        
#endif                           

void
enablequotas(struct mount *mp, vfs_context_t ctx)
{
	struct nameidata qnd;
	int type;
	char qfpath[MAXPATHLEN];
	const char *qfname = QUOTAFILENAME;
	const char *qfopsname = QUOTAOPSNAME;
	const char *qfextension[] = INITQFNAMES;

	                                                          
	if (strncmp(mp->mnt_vfsstat.f_fstypename, "hfs", sizeof("hfs")) != 0) {
		return;
	}
	  
                                               
                                                                  
    
	for (type = 0; type < MAXQUOTAS; type++) {
		snprintf(qfpath, sizeof(qfpath), "%s/%s.%s", mp->mnt_vfsstat.f_mntonname, qfopsname, qfextension[type]);
		NDINIT(&qnd, LOOKUP, OP_MOUNT, FOLLOW, UIO_SYSSPACE,
		    CAST_USER_ADDR_T(qfpath), ctx);
		if (namei(&qnd) != 0) {
			continue;                                                             
		}
		vnode_put(qnd.ni_vp);
		nameidone(&qnd);
		snprintf(qfpath, sizeof(qfpath), "%s/%s.%s", mp->mnt_vfsstat.f_mntonname, qfname, qfextension[type]);

		(void) VFS_QUOTACTL(mp, QCMD(Q_QUOTAON, type), 0, qfpath, ctx);
	}
	return;
}


static int
checkdirs_callback(proc_t p, void * arg)
{
	struct cdirargs * cdrp = (struct cdirargs *)arg;
	vnode_t olddp = cdrp->olddp;
	vnode_t newdp = cdrp->newdp;
	struct filedesc *fdp;
	vnode_t new_cvp = newdp;
	vnode_t new_rvp = newdp;
	vnode_t old_cvp = NULL;
	vnode_t old_rvp = NULL;

	  
                                                                     
                                                                    
                            
    

	  
                                                                  
                                                  
    
	proc_fdlock(p);
	fdp = p->p_fd;
	if (fdp == NULL ||
	    (fdp->fd_cdir != olddp && fdp->fd_rdir != olddp)) {
		proc_fdunlock(p);
		return PROC_RETURNED;
	}
	proc_fdunlock(p);

	  
                                                           
                                                               
                          
    
	if (vnode_ref(newdp) != 0) {
		return PROC_RETURNED;
	}
	if (vnode_ref(newdp) != 0) {
		vnode_rele(newdp);
		return PROC_RETURNED;
	}

	proc_dirs_lock_exclusive(p);
	  
                                                             
                                       
    
	proc_fdlock(p);
	fdp = p->p_fd;
	if (fdp != NULL) {
		if (fdp->fd_cdir == olddp) {
			old_cvp = olddp;
			fdp->fd_cdir = newdp;
			new_cvp = NULL;
		}
		if (fdp->fd_rdir == olddp) {
			old_rvp = olddp;
			fdp->fd_rdir = newdp;
			new_rvp = NULL;
		}
	}
	proc_fdunlock(p);
	proc_dirs_unlock_exclusive(p);

	  
                                                        
    
	if (old_cvp != NULL) {
		vnode_rele(old_cvp);
	}
	if (old_rvp != NULL) {
		vnode_rele(old_rvp);
	}
	if (new_cvp != NULL) {
		vnode_rele(new_cvp);
	}
	if (new_rvp != NULL) {
		vnode_rele(new_rvp);
	}

	return PROC_RETURNED;
}



  
                                                                 
                                                                
                                                         
   
static int
checkdirs(vnode_t olddp, vfs_context_t ctx)
{
	vnode_t newdp;
	vnode_t tvp;
	int err;
	struct cdirargs cdr;

	if (olddp->v_usecount == 1) {
		return 0;
	}
	err = VFS_ROOT(olddp->v_mountedhere, &newdp, ctx);

	if (err != 0) {
#if DIAGNOSTIC
		panic("mount: lost mount: error %d", err);
#endif
		return err;
	}

	cdr.olddp = olddp;
	cdr.newdp = newdp;
	                                                                                  
	proc_iterate(PROC_ALLPROCLIST | PROC_NOWAITTRANS, checkdirs_callback, (void *)&cdr, NULL, NULL);

	if (rootvnode == olddp) {
		vnode_ref(newdp);
		lck_rw_lock_exclusive(&rootvnode_rw_lock);
		tvp = rootvnode;
		rootvnode = newdp;
		lck_rw_unlock_exclusive(&rootvnode_rw_lock);
		vnode_rele(tvp);
	}

	vnode_put(newdp);
	return 0;
}

  
                         
  
                                                                  
                                
   
              
int
unmount(__unused proc_t p, struct unmount_args *uap, __unused int32_t *retval)
{
	vnode_t vp;
	struct mount *mp;
	int error;
	struct nameidata nd;
	vfs_context_t ctx = vfs_context_current();

	NDINIT(&nd, LOOKUP, OP_UNMOUNT, FOLLOW | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);
	error = namei(&nd);
	if (error) {
		return error;
	}
	vp = nd.ni_vp;
	mp = vp->v_mount;
	nameidone(&nd);

#if CONFIG_MACF
	error = mac_mount_check_umount(ctx, mp);
	if (error != 0) {
		vnode_put(vp);
		return error;
	}
#endif
	  
                                      
    
	if ((vp->v_flag & VROOT) == 0) {
		vnode_put(vp);
		return EINVAL;
	}
	mount_ref(mp, 0);
	vnode_put(vp);
	                                          
	return safedounmount(mp, uap->flags, ctx);
}

int
vfs_unmountbyfsid(fsid_t *fsid, int flags, vfs_context_t ctx)
{
	mount_t mp;

	mp = mount_list_lookupby_fsid(fsid, 0, 1);
	if (mp == (mount_t)0) {
		return ENOENT;
	}
	mount_ref(mp, 0);
	mount_iterdrop(mp);
	                                          
	return safedounmount(mp, flags, ctx);
}

#define ROLE_ACCOUNT_UNMOUNT_ENTITLEMENT        \
	"com.apple.private.vfs.role-account-unmount"

  
                                                                  
                                                                        
   
int
safedounmount(struct mount *mp, int flags, vfs_context_t ctx)
{
	int error;
	proc_t p = vfs_context_proc(ctx);

	  
                                                        
                                                      
    
	if ((mp->mnt_kern_flag & MNT_LNOTRESP) &&
	    (flags & MNT_NOBLOCK) && ((flags & MNT_FORCE) == 0)) {
		error = EBUSY;
		goto out;
	}

	  
                                    
                                                                              
                                                                           
                      
                                                                           
              
    
	if (!IOTaskHasEntitlement(current_task(), ROLE_ACCOUNT_UNMOUNT_ENTITLEMENT) &&
	    (!(((mp->mnt_kern_flag & MNTK_PERMIT_UNMOUNT) != 0) && ((flags & MNT_FORCE) == 0)))) {
		  
                                                          
                                          
     
		if ((mp->mnt_vfsstat.f_owner != kauth_cred_getuid(kauth_cred_get())) &&
		    (error = suser(kauth_cred_get(), &p->p_acflag))) {
			goto out;
		}
	}
	  
                                                                 
                                                                        
    
	if ((mp->mnt_flag & MNT_ROOTFS) || (mp->mnt_kern_flag & MNTK_SYSTEM)) {
		if (!(mp->mnt_flag & MNT_ROOTFS)) {
			printf("attempt to unmount a system mount (%s), will return EBUSY\n",
			    mp->mnt_vfsstat.f_mntonname);
		}
		error = EBUSY;                                                      
		goto out;
	}

	  
                                                              
                                            
    
	if (mp->mnt_kern_flag & MNTK_BACKS_ROOT) {
		error = EBUSY;
		goto out;
	}

	return dounmount(mp, flags, 1, ctx);

out:
	mount_drop(mp, 0);
	return error;
}

  
                                     
   
int
dounmount(struct mount *mp, int flags, int withref, vfs_context_t ctx)
{
	vnode_t coveredvp = (vnode_t)0;
	int error;
	int needwakeup = 0;
	int forcedunmount = 0;
	int lflags = 0;
	struct vnode *devvp = NULLVP;
#if CONFIG_TRIGGERS
	proc_t p = vfs_context_proc(ctx);
	int did_vflush = 0;
	int pflags_save = 0;
#endif                      

#if CONFIG_FSE
	if (!(flags & MNT_FORCE)) {
		fsevent_unmount(mp, ctx);                          
	}
#endif

	mount_lock(mp);

	  
                                                        
                                          
    
	if (mp->mnt_lflag & (MNT_LUNMOUNT | MNT_LMOUNT)) {
		if (withref != 0) {
			mount_drop(mp, 1);
		}
		mount_unlock(mp);
		return EBUSY;
	}

	if (flags & MNT_FORCE) {
		forcedunmount = 1;
		mp->mnt_lflag |= MNT_LFORCE;
	}

#if CONFIG_TRIGGERS
	if (flags & MNT_NOBLOCK && p != kernproc) {
		pflags_save = OSBitOrAtomic(P_NOREMOTEHANG, &p->p_flag);
	}
#endif

	mp->mnt_kern_flag |= MNTK_UNMOUNT;
	mp->mnt_lflag |= MNT_LUNMOUNT;
	mp->mnt_flag &= ~MNT_ASYNC;
	  
                                          
                                        
                                            
                                    
    
	mp->mnt_realrootvp = NULLVP;
	mount_unlock(mp);

	if (forcedunmount && (flags & MNT_LNOSUB) == 0) {
		  
                                                 
                                                     
                      
     
		(void) dounmount_submounts(mp, flags | MNT_LNOSUB, ctx);
	}

	  
                                               
                                                    
                                              
                                     
                                                     
                     
    
	name_cache_lock();
	mount_generation++;
	name_cache_unlock();


	lck_rw_lock_exclusive(&mp->mnt_rwlock);
	if (withref != 0) {
		mount_drop(mp, 0);
	}
	error = 0;
	if (forcedunmount == 0) {
		ubc_umount(mp);                            
		if ((mp->mnt_flag & MNT_RDONLY) == 0) {
			error = VFS_SYNC(mp, MNT_WAIT, ctx);
			if (error) {
				mount_lock(mp);
				mp->mnt_kern_flag &= ~MNTK_UNMOUNT;
				mp->mnt_lflag &= ~MNT_LUNMOUNT;
				mp->mnt_lflag &= ~MNT_LFORCE;
				goto out;
			}
		}
	}

	IOBSDMountChange(mp, kIOMountChangeUnmount);

#if CONFIG_TRIGGERS
	vfs_nested_trigger_unmounts(mp, flags, ctx);
	did_vflush = 1;
#endif
	if (forcedunmount) {
		lflags |= FORCECLOSE;
	}
	error = vflush(mp, NULLVP, SKIPSWAP | SKIPSYSTEM  | SKIPROOT | lflags);
	if ((forcedunmount == 0) && error) {
		mount_lock(mp);
		mp->mnt_kern_flag &= ~MNTK_UNMOUNT;
		mp->mnt_lflag &= ~MNT_LUNMOUNT;
		mp->mnt_lflag &= ~MNT_LFORCE;
		goto out;
	}

	                                                                  
	mount_iterdrain(mp);

	error = VFS_UNMOUNT(mp, flags, ctx);
	if (error) {
		mount_iterreset(mp);
		mount_lock(mp);
		mp->mnt_kern_flag &= ~MNTK_UNMOUNT;
		mp->mnt_lflag &= ~MNT_LUNMOUNT;
		mp->mnt_lflag &= ~MNT_LFORCE;
		goto out;
	}

	                                    
	if (!error) {
		OSAddAtomic(1, &vfs_nummntops);
	}

	if (mp->mnt_devvp && mp->mnt_vtable->vfc_vfsflags & VFC_VFSLOCALARGS) {
		                                                             
		devvp = mp->mnt_devvp;
		vnode_getalways(devvp);
		vnode_rele(devvp);
		VNOP_CLOSE(devvp, mp->mnt_flag & MNT_RDONLY ? FREAD : FREAD | FWRITE,
		    ctx);
		vnode_clearmountedon(devvp);
		vnode_put(devvp);
	}
	lck_rw_done(&mp->mnt_rwlock);
	mount_list_remove(mp);
	lck_rw_lock_exclusive(&mp->mnt_rwlock);

	                                                                  
	if ((coveredvp = mp->mnt_vnodecovered) != NULLVP) {
		  
                                                               
                                                              
                                                            
                                                              
                                                                  
     
		vnode_getalways(coveredvp);
		vnode_lock_spin(coveredvp);

		mp->mnt_crossref++;
		coveredvp->v_mountedhere = (struct mount *)0;
		CLR(coveredvp->v_flag, VMOUNT);

		vnode_unlock(coveredvp);
		vnode_put(coveredvp);
	}

	mount_list_lock();
	mp->mnt_vtable->vfc_refcount--;
	mount_list_unlock();

	cache_purgevfs(mp);                                                 
	vfs_event_signal(NULL, VQ_UNMOUNT, (intptr_t)NULL);
	mount_lock(mp);
	mp->mnt_lflag |= MNT_LDEAD;

	if (mp->mnt_lflag & MNT_LWAIT) {
		  
                       
                                       
                                   
                                         
                                      
     
		mp->mnt_lflag &= ~MNT_LWAIT;
		wakeup((caddr_t)mp);
	}
	mount_refdrain(mp);

	                                                         
	disk_conditioner_unmount(mp);

out:
	if (mp->mnt_lflag & MNT_LWAIT) {
		mp->mnt_lflag &= ~MNT_LWAIT;
		needwakeup = 1;
	}

#if CONFIG_TRIGGERS
	if (flags & MNT_NOBLOCK && p != kernproc) {
		                                                   
		if ((pflags_save & P_NOREMOTEHANG) == 0) {
			OSBitAndAtomic(~((uint32_t) P_NOREMOTEHANG), &p->p_flag);
		}
	}

	  
                                                                   
                                                                     
                 
    
	if (mp->mnt_triggercallback != NULL) {
		mount_unlock(mp);
		if (error == 0) {
			mp->mnt_triggercallback(mp, VTC_RELEASE, mp->mnt_triggerdata, ctx);
		} else if (did_vflush) {
			mp->mnt_triggercallback(mp, VTC_REPLACE, mp->mnt_triggerdata, ctx);
		}
	} else {
		mount_unlock(mp);
	}
#else
	mount_unlock(mp);
#endif                      

	lck_rw_done(&mp->mnt_rwlock);

	if (needwakeup) {
		wakeup((caddr_t)mp);
	}

	if (!error) {
		if ((coveredvp != NULLVP)) {
			vnode_t pvp = NULLVP;

			  
                                                         
                                                         
                                                         
                                                           
                                                  
                                             
      
			vnode_getalways(coveredvp);

			mount_dropcrossref(mp, coveredvp, 0);
			  
                                                      
                                                        
                                                        
                                                          
          
      
			if (!vnode_isrecycled(coveredvp)) {
				pvp = vnode_getparent(coveredvp);
#if CONFIG_TRIGGERS
				if (coveredvp->v_resolve) {
					vnode_trigger_rearm(coveredvp, ctx);
				}
#endif
			}

			vnode_rele(coveredvp);
			vnode_put(coveredvp);
			coveredvp = NULLVP;

			if (pvp) {
				lock_vnode_and_post(pvp, NOTE_WRITE);
				vnode_put(pvp);
			}
		} else if (mp->mnt_flag & MNT_ROOTFS) {
			mount_lock_destroy(mp);
#if CONFIG_MACF
			mac_mount_label_destroy(mp);
#endif
			zfree(mount_zone, mp);
		} else {
			panic("dounmount: no coveredvp");
		}
	}
	return error;
}

  
                                         
   
void
dounmount_submounts(struct mount *mp, int flags, vfs_context_t ctx)
{
	mount_t smp;
	fsid_t *fsids, fsid;
	int fsids_sz;
	int count = 0, i, m = 0;
	vnode_t vp;

	mount_list_lock();

	                                            
	TAILQ_FOREACH(smp, &mountlist, mnt_list)
	count++;
	fsids_sz = count * sizeof(fsid_t);
	fsids = kheap_alloc(KHEAP_TEMP, fsids_sz, Z_NOWAIT);
	if (fsids == NULL) {
		mount_list_unlock();
		goto out;
	}
	fsids[0] = mp->mnt_vfsstat.f_fsid;                       

	  
                                       
                                                                    
                                  
                                                             
                                                                      
    
	for (smp = TAILQ_NEXT(mp, mnt_list); smp; smp = TAILQ_NEXT(smp, mnt_list)) {
		vp = smp->mnt_vnodecovered;
		if (vp == NULL) {
			continue;
		}
		fsid = vnode_mount(vp)->mnt_vfsstat.f_fsid;                       
		for (i = 0; i <= m; i++) {
			if (fsids[i].val[0] == fsid.val[0] &&
			    fsids[i].val[1] == fsid.val[1]) {
				fsids[++m] = smp->mnt_vfsstat.f_fsid;
				break;
			}
		}
	}
	mount_list_unlock();

	                                                         
	for (i = m; i > 0; i--) {
		smp = mount_list_lookupby_fsid(&fsids[i], 0, 1);
		if (smp) {
			mount_ref(smp, 0);
			mount_iterdrop(smp);
			(void) dounmount(smp, flags, 1, ctx);
		}
	}
out:
	kheap_free(KHEAP_TEMP, fsids, fsids_sz);
}

void
mount_dropcrossref(mount_t mp, vnode_t dp, int need_put)
{
	vnode_lock(dp);
	mp->mnt_crossref--;

	if (mp->mnt_crossref < 0) {
		panic("mount cross refs -ve");
	}

	if ((mp != dp->v_mountedhere) && (mp->mnt_crossref == 0)) {
		if (need_put) {
			vnode_put_locked(dp);
		}
		vnode_unlock(dp);

		mount_lock_destroy(mp);
#if CONFIG_MACF
		mac_mount_label_destroy(mp);
#endif
		zfree(mount_zone, mp);
		return;
	}
	if (need_put) {
		vnode_put_locked(dp);
	}
	vnode_unlock(dp);
}


  
                                
   
#if DIAGNOSTIC
int syncprt = 0;
#endif

int print_vmpage_stat = 0;

  
                                                                 
                                                      
  
                                                                          
                                        
  
                                                                            
                                                                            
                                                                          
                 
  
                         
   
static int
sync_callback(mount_t mp, void *arg)
{
	if ((mp->mnt_flag & MNT_RDONLY) == 0) {
		int asyncflag = mp->mnt_flag & MNT_ASYNC;
		unsigned waitfor = MNT_NOWAIT;

		if (arg) {
			waitfor = *(uint32_t*)arg;
		}

		                                                                                     
		if (waitfor != MNT_WAIT &&
		    waitfor != (MNT_WAIT | MNT_VOLUME) &&
		    waitfor != MNT_NOWAIT &&
		    waitfor != (MNT_NOWAIT | MNT_VOLUME) &&
		    waitfor != MNT_DWAIT &&
		    waitfor != (MNT_DWAIT | MNT_VOLUME)) {
			panic("Passed inappropriate waitfor %u to "
			    "sync_callback()", waitfor);
		}

		mp->mnt_flag &= ~MNT_ASYNC;
		(void)VFS_SYNC(mp, waitfor, vfs_context_kernel());
		if (asyncflag) {
			mp->mnt_flag |= MNT_ASYNC;
		}
	}

	return VFS_RETURNED;
}

              
int
sync(__unused proc_t p, __unused struct sync_args *uap, __unused int32_t *retval)
{
	vfs_iterate(LK_NOWAIT, sync_callback, NULL);

	if (print_vmpage_stat) {
		vm_countdirtypages();
	}

#if DIAGNOSTIC
	if (syncprt) {
		vfs_bufstats();
	}
#endif                 
	return 0;
}

typedef enum {
	SYNC_ALL = 0,
	SYNC_ONLY_RELIABLE_MEDIA = 1,
	SYNC_ONLY_UNRELIABLE_MEDIA = 2
} sync_type_t;

static int
sync_internal_callback(mount_t mp, void *arg)
{
	if (arg) {
		int is_reliable = !(mp->mnt_kern_flag & MNTK_VIRTUALDEV) &&
		    (mp->mnt_flag & MNT_LOCAL);
		sync_type_t sync_type = *((sync_type_t *)arg);

		if ((sync_type == SYNC_ONLY_RELIABLE_MEDIA) && !is_reliable) {
			return VFS_RETURNED;
		} else if ((sync_type == SYNC_ONLY_UNRELIABLE_MEDIA) && is_reliable) {
			return VFS_RETURNED;
		}
	}

	(void)sync_callback(mp, NULL);

	return VFS_RETURNED;
}

int sync_thread_state = 0;
int sync_timeout_seconds = 5;

#define SYNC_THREAD_RUN       0x0001
#define SYNC_THREAD_RUNNING   0x0002

#if CONFIG_PHYS_WRITE_ACCT
thread_t pm_sync_thread;
#endif                             

static void
sync_thread(__unused void *arg, __unused wait_result_t wr)
{
	sync_type_t sync_type;
#if CONFIG_PHYS_WRITE_ACCT
	pm_sync_thread = current_thread();
#endif                             

	lck_mtx_lock(&sync_mtx_lck);
	while (sync_thread_state & SYNC_THREAD_RUN) {
		sync_thread_state &= ~SYNC_THREAD_RUN;
		lck_mtx_unlock(&sync_mtx_lck);

		sync_type = SYNC_ONLY_RELIABLE_MEDIA;
		vfs_iterate(LK_NOWAIT, sync_internal_callback, &sync_type);
		sync_type = SYNC_ONLY_UNRELIABLE_MEDIA;
		vfs_iterate(LK_NOWAIT, sync_internal_callback, &sync_type);

		lck_mtx_lock(&sync_mtx_lck);
	}
	  
                                                                        
                                                              
                                                                      
                                  
    
	wakeup(&sync_thread_state);
	sync_thread_state &= ~SYNC_THREAD_RUNNING;
#if CONFIG_PHYS_WRITE_ACCT
	pm_sync_thread = NULL;
#endif                             
	lck_mtx_unlock(&sync_mtx_lck);

	if (print_vmpage_stat) {
		vm_countdirtypages();
	}

#if DIAGNOSTIC
	if (syncprt) {
		vfs_bufstats();
	}
#endif                 
}

struct timeval sync_timeout_last_print = {.tv_sec = 0, .tv_usec = 0};

  
                                                  
                                                            
   
__private_extern__ int
sync_internal(void)
{
	thread_t thd;
	int error;
	int thread_created = FALSE;
	struct timespec ts = {.tv_sec = sync_timeout_seconds, .tv_nsec = 0};

	lck_mtx_lock(&sync_mtx_lck);
	sync_thread_state |= SYNC_THREAD_RUN;
	if (!(sync_thread_state & SYNC_THREAD_RUNNING)) {
		int kr;

		sync_thread_state |= SYNC_THREAD_RUNNING;
		kr = kernel_thread_start(sync_thread, NULL, &thd);
		if (kr != KERN_SUCCESS) {
			sync_thread_state &= ~SYNC_THREAD_RUNNING;
			lck_mtx_unlock(&sync_mtx_lck);
			printf("sync_thread failed\n");
			return 0;
		}
		thread_created = TRUE;
	}

	error = msleep((caddr_t)&sync_thread_state, &sync_mtx_lck,
	    (PVFS | PDROP | PCATCH), "sync_thread", &ts);
	if (error) {
		struct timeval now;

		microtime(&now);
		if (now.tv_sec - sync_timeout_last_print.tv_sec > 120) {
			printf("sync timed out: %d sec\n", sync_timeout_seconds);
			sync_timeout_last_print.tv_sec = now.tv_sec;
		}
	}

	if (thread_created) {
		thread_deallocate(thd);
	}

	return 0;
}                                

  
                            
   
#if QUOTA
int
quotactl(proc_t p, struct quotactl_args *uap, __unused int32_t *retval)
{
	struct mount *mp;
	int error, quota_cmd, quota_status = 0;
	caddr_t datap;
	size_t fnamelen;
	struct nameidata nd;
	vfs_context_t ctx = vfs_context_current();
	struct dqblk my_dqblk = {};

	AUDIT_ARG(uid, uap->uid);
	AUDIT_ARG(cmd, uap->cmd);
	NDINIT(&nd, LOOKUP, OP_LOOKUP, FOLLOW | AUDITVNPATH1, UIO_USERSPACE,
	    uap->path, ctx);
	error = namei(&nd);
	if (error) {
		return error;
	}
	mp = nd.ni_vp->v_mount;
	mount_ref(mp, 0);
	vnode_put(nd.ni_vp);
	nameidone(&nd);

#if CONFIG_MACF
	error = mac_mount_check_quotactl(ctx, mp, uap->cmd, uap->uid);
	if (error != 0) {
		goto out;
	}
#endif

	                                                      
	quota_cmd = uap->cmd >> SUBCMDSHIFT;

	switch (quota_cmd) {
	case Q_QUOTAON:
		                                                             
		fnamelen = MAXPATHLEN;
		datap = zalloc(ZV_NAMEI);
		error = copyinstr(uap->arg, datap, MAXPATHLEN, &fnamelen);
		break;
	case Q_GETQUOTA:
		                                                 
		datap = (caddr_t) &my_dqblk;
		break;
	case Q_SETQUOTA:
	case Q_SETUSE:
		                                                 
		datap = (caddr_t) &my_dqblk;
		if (proc_is64bit(p)) {
			struct user_dqblk       my_dqblk64;
			error = copyin(uap->arg, (caddr_t)&my_dqblk64, sizeof(my_dqblk64));
			if (error == 0) {
				munge_dqblk(&my_dqblk, &my_dqblk64, FALSE);
			}
		} else {
			error = copyin(uap->arg, (caddr_t)&my_dqblk, sizeof(my_dqblk));
		}
		break;
	case Q_QUOTASTAT:
		                                         
		datap = (caddr_t) &quota_status;
		break;
	default:
		datap = NULL;
		break;
	}             

	if (error == 0) {
		error = VFS_QUOTACTL(mp, uap->cmd, uap->uid, datap, ctx);
	}

	switch (quota_cmd) {
	case Q_QUOTAON:
		if (datap != NULL) {
			zfree(ZV_NAMEI, datap);
		}
		break;
	case Q_GETQUOTA:
		                                                                       
		if (error == 0) {
			if (proc_is64bit(p)) {
				struct user_dqblk       my_dqblk64;

				memset(&my_dqblk64, 0, sizeof(my_dqblk64));
				munge_dqblk(&my_dqblk, &my_dqblk64, TRUE);
				error = copyout((caddr_t)&my_dqblk64, uap->arg, sizeof(my_dqblk64));
			} else {
				error = copyout(datap, uap->arg, sizeof(struct dqblk));
			}
		}
		break;
	case Q_QUOTASTAT:
		                                         
		if (error == 0) {
			error = copyout(datap, uap->arg, sizeof(quota_status));
		}
		break;
	default:
		break;
	}             

out:
	mount_drop(mp, 0);
	return error;
}
#else
int
quotactl(__unused proc_t p, __unused struct quotactl_args *uap, __unused int32_t *retval)
{
	return EOPNOTSUPP;
}
#endif            

  
                             
  
                       
            
                         
                      
   
              
int
statfs(__unused proc_t p, struct statfs_args *uap, __unused int32_t *retval)
{
	struct mount *mp;
	struct vfsstatfs *sp;
	int error;
	struct nameidata nd;
	vfs_context_t ctx = vfs_context_current();
	vnode_t vp;

	NDINIT(&nd, LOOKUP, OP_STATFS, FOLLOW | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);
	error = namei(&nd);
	if (error != 0) {
		return error;
	}
	vp = nd.ni_vp;
	mp = vp->v_mount;
	sp = &mp->mnt_vfsstat;
	nameidone(&nd);

#if CONFIG_MACF
	error = mac_mount_check_stat(ctx, mp);
	if (error != 0) {
		vnode_put(vp);
		return error;
	}
#endif

	error = vfs_update_vfsstat(mp, ctx, VFS_USER_EVENT);
	if (error != 0) {
		vnode_put(vp);
		return error;
	}

	error = munge_statfs(mp, sp, uap->buf, NULL, IS_64BIT_PROCESS(p), TRUE);
	vnode_put(vp);
	return error;
}

  
                             
   
              
int
fstatfs(__unused proc_t p, struct fstatfs_args *uap, __unused int32_t *retval)
{
	vnode_t vp;
	struct mount *mp;
	struct vfsstatfs *sp;
	int error;

	AUDIT_ARG(fd, uap->fd);

	if ((error = file_vnode(uap->fd, &vp))) {
		return error;
	}

	error = vnode_getwithref(vp);
	if (error) {
		file_drop(uap->fd);
		return error;
	}

	AUDIT_ARG(vnpath_withref, vp, ARG_VNODE1);

	mp = vp->v_mount;
	if (!mp) {
		error = EBADF;
		goto out;
	}

#if CONFIG_MACF
	error = mac_mount_check_stat(vfs_context_current(), mp);
	if (error != 0) {
		goto out;
	}
#endif

	sp = &mp->mnt_vfsstat;
	if ((error = vfs_update_vfsstat(mp, vfs_context_current(), VFS_USER_EVENT)) != 0) {
		goto out;
	}

	error = munge_statfs(mp, sp, uap->buf, NULL, IS_64BIT_PROCESS(p), TRUE);

out:
	file_drop(uap->fd);
	vnode_put(vp);

	return error;
}

void
vfs_get_statfs64(struct mount *mp, struct statfs64 *sfs)
{
	struct vfsstatfs *vsfs = &mp->mnt_vfsstat;

	bzero(sfs, sizeof(*sfs));

	sfs->f_bsize = vsfs->f_bsize;
	sfs->f_iosize = (int32_t)vsfs->f_iosize;
	sfs->f_blocks = vsfs->f_blocks;
	sfs->f_bfree = vsfs->f_bfree;
	sfs->f_bavail = vsfs->f_bavail;
	sfs->f_files = vsfs->f_files;
	sfs->f_ffree = vsfs->f_ffree;
	sfs->f_fsid = vsfs->f_fsid;
	sfs->f_owner = vsfs->f_owner;
	sfs->f_type = mp->mnt_vtable->vfc_typenum;
	sfs->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	sfs->f_fssubtype = vsfs->f_fssubtype;
	sfs->f_flags_ext = (mp->mnt_kern_flag & MNTK_SYSTEMDATA) ? MNT_EXT_ROOT_DATA_VOL : 0;
	if (mp->mnt_kern_flag & MNTK_TYPENAME_OVERRIDE) {
		strlcpy(&sfs->f_fstypename[0], &mp->fstypename_override[0], MFSTYPENAMELEN);
	} else {
		strlcpy(&sfs->f_fstypename[0], &vsfs->f_fstypename[0], MFSTYPENAMELEN);
	}
	strlcpy(&sfs->f_mntonname[0], &vsfs->f_mntonname[0], MAXPATHLEN);
	strlcpy(&sfs->f_mntfromname[0], &vsfs->f_mntfromname[0], MAXPATHLEN);
}

  
                                            
   
int
statfs64(__unused struct proc *p, struct statfs64_args *uap, __unused int32_t *retval)
{
	struct mount *mp;
	int error;
	struct nameidata *ndp;
	struct statfs64 *sfsp;
	vfs_context_t ctxp = vfs_context_current();
	vnode_t vp;
	union {
		struct nameidata nd;
		struct statfs64 sfs;
	} *__nameidata_statfs64;

	__nameidata_statfs64 = kheap_alloc(KHEAP_TEMP, sizeof(*__nameidata_statfs64),
	    Z_WAITOK);
	ndp = &__nameidata_statfs64->nd;

	NDINIT(ndp, LOOKUP, OP_STATFS, FOLLOW | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctxp);
	error = namei(ndp);
	if (error != 0) {
		goto out;
	}
	vp = ndp->ni_vp;
	mp = vp->v_mount;
	nameidone(ndp);

#if CONFIG_MACF
	error = mac_mount_check_stat(ctxp, mp);
	if (error != 0) {
		vnode_put(vp);
		goto out;
	}
#endif

	error = vfs_update_vfsstat(mp, ctxp, VFS_USER_EVENT);
	if (error != 0) {
		vnode_put(vp);
		goto out;
	}

	sfsp = &__nameidata_statfs64->sfs;
	vfs_get_statfs64(mp, sfsp);
	if ((mp->mnt_kern_flag & MNTK_SYSTEMDATA) &&
	    (p->p_vfs_iopolicy & P_VFS_IOPOLICY_STATFS_NO_DATA_VOLUME)) {
		                                                                         
		strlcpy(&sfsp->f_mntonname[0], "/", sizeof("/"));
	}
	error = copyout(sfsp, uap->buf, sizeof(*sfsp));
	vnode_put(vp);

out:
	kheap_free(KHEAP_TEMP, __nameidata_statfs64, sizeof(*__nameidata_statfs64));

	return error;
}

  
                                            
   
int
fstatfs64(__unused struct proc *p, struct fstatfs64_args *uap, __unused int32_t *retval)
{
	struct vnode *vp;
	struct mount *mp;
	struct statfs64 sfs;
	int error;

	AUDIT_ARG(fd, uap->fd);

	if ((error = file_vnode(uap->fd, &vp))) {
		return error;
	}

	error = vnode_getwithref(vp);
	if (error) {
		file_drop(uap->fd);
		return error;
	}

	AUDIT_ARG(vnpath_withref, vp, ARG_VNODE1);

	mp = vp->v_mount;
	if (!mp) {
		error = EBADF;
		goto out;
	}

#if CONFIG_MACF
	error = mac_mount_check_stat(vfs_context_current(), mp);
	if (error != 0) {
		goto out;
	}
#endif

	if ((error = vfs_update_vfsstat(mp, vfs_context_current(), VFS_USER_EVENT)) != 0) {
		goto out;
	}

	vfs_get_statfs64(mp, &sfs);
	if ((mp->mnt_kern_flag & MNTK_SYSTEMDATA) &&
	    (p->p_vfs_iopolicy & P_VFS_IOPOLICY_STATFS_NO_DATA_VOLUME)) {
		                                                                         
		strlcpy(&sfs.f_mntonname[0], "/", sizeof("/"));
	}
	error = copyout(&sfs, uap->buf, sizeof(sfs));

out:
	file_drop(uap->fd);
	vnode_put(vp);

	return error;
}

struct getfsstat_struct {
	user_addr_t     sfsp;
	user_addr_t     *mp;
	int             count;
	int             maxcount;
	int             flags;
	int             error;
};


static int
getfsstat_callback(mount_t mp, void * arg)
{
	struct getfsstat_struct *fstp = (struct getfsstat_struct *)arg;
	struct vfsstatfs *sp;
	int error, my_size;
	vfs_context_t ctx = vfs_context_current();

	if (fstp->sfsp && fstp->count < fstp->maxcount) {
#if CONFIG_MACF
		error = mac_mount_check_stat(ctx, mp);
		if (error != 0) {
			fstp->error = error;
			return VFS_RETURNED_DONE;
		}
#endif
		sp = &mp->mnt_vfsstat;
		  
                                                   
                                                           
     
		if ((mp->mnt_lflag & MNT_LDEAD) ||
		    (((fstp->flags & MNT_NOWAIT) == 0 || (fstp->flags & (MNT_WAIT | MNT_DWAIT))) &&
		    (!(mp->mnt_lflag & MNT_LUNMOUNT)) &&
		    (error = vfs_update_vfsstat(mp, ctx, VFS_USER_EVENT)))) {
			KAUTH_DEBUG("vfs_update_vfsstat returned %d", error);
			return VFS_RETURNED;
		}

		  
                                                 
     
		error = munge_statfs(mp, sp, fstp->sfsp, &my_size, IS_64BIT_PROCESS(vfs_context_proc(ctx)), FALSE);
		if (error) {
			fstp->error = error;
			return VFS_RETURNED_DONE;
		}
		fstp->sfsp += my_size;

		if (fstp->mp) {
#if CONFIG_MACF
			error = mac_mount_label_get(mp, *fstp->mp);
			if (error) {
				fstp->error = error;
				return VFS_RETURNED_DONE;
			}
#endif
			fstp->mp++;
		}
	}
	fstp->count++;
	return VFS_RETURNED;
}

  
                                     
   
int
getfsstat(__unused proc_t p, struct getfsstat_args *uap, int *retval)
{
	struct __mac_getfsstat_args muap;

	muap.buf = uap->buf;
	muap.bufsize = uap->bufsize;
	muap.mac = USER_ADDR_NULL;
	muap.macsize = 0;
	muap.flags = uap->flags;

	return __mac_getfsstat(p, &muap, retval);
}

  
                                                          
  
                                                    
                                                                               
                                                                                    
  
                                                      
                                                        
                                                                                    
                                                   
                                                            
  
  
                                                  
                                                      
  
   
int
__mac_getfsstat(__unused proc_t p, struct __mac_getfsstat_args *uap, int *retval)
{
	user_addr_t sfsp;
	user_addr_t *mp;
	size_t count, maxcount, bufsize, macsize;
	struct getfsstat_struct fst;

	if ((unsigned)uap->bufsize > INT_MAX || (unsigned)uap->macsize > INT_MAX) {
		return EINVAL;
	}

	bufsize = (size_t) uap->bufsize;
	macsize = (size_t) uap->macsize;

	if (IS_64BIT_PROCESS(p)) {
		maxcount = bufsize / sizeof(struct user64_statfs);
	} else {
		maxcount = bufsize / sizeof(struct user32_statfs);
	}
	sfsp = uap->buf;
	count = 0;

	mp = NULL;

#if CONFIG_MACF
	if (uap->mac != USER_ADDR_NULL) {
		u_int32_t *mp0;
		int error;
		unsigned int i;

		count = (macsize / (IS_64BIT_PROCESS(p) ? 8 : 4));
		if (count != maxcount) {
			return EINVAL;
		}

		                       
		mp0 = kheap_alloc(KHEAP_TEMP, macsize, Z_WAITOK);
		if (mp0 == NULL) {
			return ENOMEM;
		}

		error = copyin(uap->mac, mp0, macsize);
		if (error) {
			kheap_free(KHEAP_TEMP, mp0, macsize);
			return error;
		}

		                                          
		mp = kheap_alloc(KHEAP_TEMP, count * sizeof(user_addr_t), Z_WAITOK);
		if (mp == NULL) {
			kheap_free(KHEAP_TEMP, mp0, macsize);
			return ENOMEM;
		}

		for (i = 0; i < count; i++) {
			if (IS_64BIT_PROCESS(p)) {
				mp[i] = ((user_addr_t *)mp0)[i];
			} else {
				mp[i] = (user_addr_t)mp0[i];
			}
		}
		kheap_free(KHEAP_TEMP, mp0, macsize);
	}
#endif


	fst.sfsp = sfsp;
	fst.mp = mp;
	fst.flags = uap->flags;
	fst.count = 0;
	fst.error = 0;
	fst.maxcount = (int)maxcount;


	vfs_iterate(VFS_ITERATE_NOSKIP_UNMOUNT, getfsstat_callback, &fst);

	if (mp) {
		kheap_free(KHEAP_TEMP, mp, count * sizeof(user_addr_t));
	}

	if (fst.error) {
		KAUTH_DEBUG("ERROR - %s gets %d", p->p_comm, fst.error);
		return fst.error;
	}

	if (fst.sfsp && fst.count > fst.maxcount) {
		*retval = fst.maxcount;
	} else {
		*retval = fst.count;
	}
	return 0;
}

static int
getfsstat64_callback(mount_t mp, void * arg)
{
	struct getfsstat_struct *fstp = (struct getfsstat_struct *)arg;
	struct vfsstatfs *sp;
	struct statfs64 sfs;
	int error;

	if (fstp->sfsp && fstp->count < fstp->maxcount) {
#if CONFIG_MACF
		error = mac_mount_check_stat(vfs_context_current(), mp);
		if (error != 0) {
			fstp->error = error;
			return VFS_RETURNED_DONE;
		}
#endif
		sp = &mp->mnt_vfsstat;
		  
                                                          
                                          
    
                                                        
                                                       
               
     
		if ((mp->mnt_lflag & MNT_LDEAD) ||
		    ((((fstp->flags & MNT_NOWAIT) == 0) || (fstp->flags & (MNT_WAIT | MNT_DWAIT))) &&
		    (!(mp->mnt_lflag & MNT_LUNMOUNT)) &&
		    (error = vfs_update_vfsstat(mp, vfs_context_current(), VFS_USER_EVENT)))) {
			KAUTH_DEBUG("vfs_update_vfsstat returned %d", error);
			return VFS_RETURNED;
		}

		vfs_get_statfs64(mp, &sfs);
		error = copyout(&sfs, fstp->sfsp, sizeof(sfs));
		if (error) {
			fstp->error = error;
			return VFS_RETURNED_DONE;
		}
		fstp->sfsp += sizeof(sfs);
	}
	fstp->count++;
	return VFS_RETURNED;
}

  
                                                     
   
int
getfsstat64(__unused proc_t p, struct getfsstat64_args *uap, int *retval)
{
	user_addr_t sfsp;
	int count, maxcount;
	struct getfsstat_struct fst;

	maxcount = uap->bufsize / sizeof(struct statfs64);

	sfsp = uap->buf;
	count = 0;

	fst.sfsp = sfsp;
	fst.flags = uap->flags;
	fst.count = 0;
	fst.error = 0;
	fst.maxcount = maxcount;

	vfs_iterate(VFS_ITERATE_NOSKIP_UNMOUNT, getfsstat64_callback, &fst);

	if (fst.error) {
		KAUTH_DEBUG("ERROR - %s gets %d", p->p_comm, fst.error);
		return fst.error;
	}

	if (fst.sfsp && fst.count > fst.maxcount) {
		*retval = fst.maxcount;
	} else {
		*retval = fst.count;
	}

	return 0;
}

  
                                                             
           
  
        
                              
                                                    
                                                    
  
                                                              
                                 
  
   
int
vnode_getfromfd(vfs_context_t ctx, int fd, vnode_t *vpp)
{
	int error;
	vnode_t vp;
	struct fileproc *fp;
	proc_t p = vfs_context_proc(ctx);

	*vpp =  NULLVP;

	error = fp_getfvp(p, fd, &fp, &vp);
	if (error) {
		return error;
	}

	error = vnode_getwithref(vp);
	if (error) {
		(void)fp_drop(p, fd, fp, 0);
		return error;
	}

	(void)fp_drop(p, fd, fp, 0);
	*vpp = vp;
	return error;
}

  
                                                                 
                                           
  
                                                                 
                                                                       
                                                 
   
int
nameiat(struct nameidata *ndp, int dirfd)
{
	if ((dirfd != AT_FDCWD) &&
	    !(ndp->ni_flag & NAMEI_CONTLOOKUP) &&
	    !(ndp->ni_cnd.cn_flags & USEDVP)) {
		int error = 0;
		char c;

		if (UIO_SEG_IS_USER_SPACE(ndp->ni_segflg)) {
			error = copyin(ndp->ni_dirp, &c, sizeof(char));
			if (error) {
				return error;
			}
		} else {
			c = *((char *)(ndp->ni_dirp));
		}

		if (c != '/') {
			vnode_t dvp_at;

			error = vnode_getfromfd(ndp->ni_cnd.cn_context, dirfd,
			    &dvp_at);
			if (error) {
				return error;
			}

			if (vnode_vtype(dvp_at) != VDIR) {
				vnode_put(dvp_at);
				return ENOTDIR;
			}

			ndp->ni_dvp = dvp_at;
			ndp->ni_cnd.cn_flags |= USEDVP;
			error = namei(ndp);
			ndp->ni_cnd.cn_flags &= ~USEDVP;
			vnode_put(dvp_at);
			return error;
		}
	}

	return namei(ndp);
}

  
                                                               
   
              
static int
common_fchdir(proc_t p, struct fchdir_args *uap, int per_thread)
{
	struct filedesc *fdp = p->p_fd;
	vnode_t vp;
	vnode_t tdp;
	vnode_t tvp;
	struct mount *mp;
	int error, should_put = 1;
	vfs_context_t ctx = vfs_context_current();

	AUDIT_ARG(fd, uap->fd);
	if (per_thread && uap->fd == -1) {
		  
                                                                 
                                                               
                                                                 
                                   
     
		thread_t th = vfs_context_thread(ctx);
		if (th) {
			uthread_t uth = get_bsdthread_info(th);
			tvp = uth->uu_cdir;
			uth->uu_cdir = NULLVP;
			if (tvp != NULLVP) {
				vnode_rele(tvp);
				return 0;
			}
		}
		return EBADF;
	}

	if ((error = file_vnode(uap->fd, &vp))) {
		return error;
	}
	if ((error = vnode_getwithref(vp))) {
		file_drop(uap->fd);
		return error;
	}

	AUDIT_ARG(vnpath, vp, ARG_VNODE1);

	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}

#if CONFIG_MACF
	error = mac_vnode_check_chdir(ctx, vp);
	if (error) {
		goto out;
	}
#endif
	error = vnode_authorize(vp, NULL, KAUTH_VNODE_SEARCH, ctx);
	if (error) {
		goto out;
	}

	while (!error && (mp = vp->v_mountedhere) != NULL) {
		if (vfs_busy(mp, LK_NOWAIT)) {
			error = EACCES;
			goto out;
		}
		error = VFS_ROOT(mp, &tdp, ctx);
		vfs_unbusy(mp);
		if (error) {
			break;
		}
		vnode_put(vp);
		vp = tdp;
	}
	if (error) {
		goto out;
	}
	if ((error = vnode_ref(vp))) {
		goto out;
	}
	vnode_put(vp);
	should_put = 0;

	if (per_thread) {
		thread_t th = vfs_context_thread(ctx);
		if (th) {
			uthread_t uth = get_bsdthread_info(th);
			tvp = uth->uu_cdir;
			uth->uu_cdir = vp;
			OSBitOrAtomic(P_THCWD, &p->p_flag);
		} else {
			vnode_rele(vp);
			error = ENOENT;
			goto out;
		}
	} else {
		proc_dirs_lock_exclusive(p);
		proc_fdlock(p);
		tvp = fdp->fd_cdir;
		fdp->fd_cdir = vp;
		proc_fdunlock(p);
		proc_dirs_unlock_exclusive(p);
	}

	if (tvp) {
		vnode_rele(tvp);
	}

out:
	if (should_put) {
		vnode_put(vp);
	}
	file_drop(uap->fd);

	return error;
}

int
fchdir(proc_t p, struct fchdir_args *uap, __unused int32_t *retval)
{
	return common_fchdir(p, uap, 0);
}

int
__pthread_fchdir(proc_t p, struct __pthread_fchdir_args *uap, __unused int32_t *retval)
{
	return common_fchdir(p, (void *)uap, 1);
}


  
                                          
  
                       
                     
                 
                                              
   
              
int
chdir_internal(proc_t p, vfs_context_t ctx, struct nameidata *ndp, int per_thread)
{
	struct filedesc *fdp = p->p_fd;
	int error;
	vnode_t tvp;

	error = change_dir(ndp, ctx);
	if (error) {
		return error;
	}
	if ((error = vnode_ref(ndp->ni_vp))) {
		vnode_put(ndp->ni_vp);
		return error;
	}
	  
                                               
    
	vnode_put(ndp->ni_vp);

	if (per_thread) {
		thread_t th = vfs_context_thread(ctx);
		if (th) {
			uthread_t uth = get_bsdthread_info(th);
			tvp = uth->uu_cdir;
			uth->uu_cdir = ndp->ni_vp;
			OSBitOrAtomic(P_THCWD, &p->p_flag);
		} else {
			vnode_rele(ndp->ni_vp);
			return ENOENT;
		}
	} else {
		proc_dirs_lock_exclusive(p);
		proc_fdlock(p);
		tvp = fdp->fd_cdir;
		fdp->fd_cdir = ndp->ni_vp;
		proc_fdunlock(p);
		proc_dirs_unlock_exclusive(p);
	}

	if (tvp) {
		vnode_rele(tvp);
	}

	return 0;
}


  
                                          
  
                       
                         
                                                   
                     
   
              
static int
common_chdir(proc_t p, struct chdir_args *uap, int per_thread)
{
	struct nameidata nd;
	vfs_context_t ctx = vfs_context_current();

	NDINIT(&nd, LOOKUP, OP_CHDIR, FOLLOW | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);

	return chdir_internal(p, ctx, &nd, per_thread);
}


  
        
  
                                                                
  
                                                   
                                                            
                                 
  
                                                
  
                       
                                     
                                                              
                                 
  
   
int
chdir(proc_t p, struct chdir_args *uap, __unused int32_t *retval)
{
	return common_chdir(p, (void *)uap, 0);
}

  
                  
  
                                                             
  
                                                   
                                                            
                                 
  
                                                
  
                       
                                     
                                                  
                     
  
   
int
__pthread_chdir(proc_t p, struct __pthread_chdir_args *uap, __unused int32_t *retval)
{
	return common_chdir(p, (void *)uap, 1);
}


  
                                           
   
              
int
chroot(proc_t p, struct chroot_args *uap, __unused int32_t *retval)
{
	struct filedesc *fdp = p->p_fd;
	int error;
	struct nameidata nd;
	vnode_t tvp;
	vfs_context_t ctx = vfs_context_current();

	if ((error = suser(kauth_cred_get(), &p->p_acflag))) {
		return error;
	}

	NDINIT(&nd, LOOKUP, OP_CHROOT, FOLLOW | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);
	error = change_dir(&nd, ctx);
	if (error) {
		return error;
	}

#if CONFIG_MACF
	error = mac_vnode_check_chroot(ctx, nd.ni_vp,
	    &nd.ni_cnd);
	if (error) {
		vnode_put(nd.ni_vp);
		return error;
	}
#endif

	if ((error = vnode_ref(nd.ni_vp))) {
		vnode_put(nd.ni_vp);
		return error;
	}
	vnode_put(nd.ni_vp);

	  
                                                                      
                                                                      
                                                                     
              
    
	                                            
	proc_dirs_lock_exclusive(p);
	                                                                       
	proc_fdlock(p);
	tvp = fdp->fd_rdir;
	fdp->fd_rdir = nd.ni_vp;
	fdp->fd_flags |= FD_CHROOT;
	proc_fdunlock(p);
	proc_dirs_unlock_exclusive(p);

	if (tvp != NULL) {
		vnode_rele(tvp);
	}

	return 0;
}

#define PATHSTATICBUFLEN 256
#define PIVOT_ROOT_ENTITLEMENT              \
       "com.apple.private.vfs.pivot-root"

#if defined(XNU_TARGET_OS_OSX)
int
pivot_root(proc_t p, struct pivot_root_args *uap, __unused int *retval)
{
	int error;
	char new_rootfs_path_before[PATHSTATICBUFLEN] = {0};
	char old_rootfs_path_after[PATHSTATICBUFLEN] = {0};
	char *new_rootfs_path_before_buf = NULL;
	char *old_rootfs_path_after_buf = NULL;
	char *incoming = NULL;
	char *outgoing = NULL;
	vnode_t incoming_rootvp = NULLVP;
	size_t bytes_copied;

	  
                                        
                                 
    
	if ((error = suser(kauth_cred_get(), &p->p_acflag))) {
		return error;
	}

	  
                                               
                        
    
	if ((p->p_pid != 1) || !IOTaskHasEntitlement(current_task(), PIVOT_ROOT_ENTITLEMENT)) {
		return EPERM;
	}

	error = copyinstr(uap->new_rootfs_path_before, &new_rootfs_path_before[0], PATHSTATICBUFLEN, &bytes_copied);
	if (error == ENAMETOOLONG) {
		new_rootfs_path_before_buf = zalloc_flags(ZV_NAMEI, Z_WAITOK);
		error = copyinstr(uap->new_rootfs_path_before, new_rootfs_path_before_buf, MAXPATHLEN, &bytes_copied);
	}

	if (error) {
		goto out;
	}

	error = copyinstr(uap->old_rootfs_path_after, &old_rootfs_path_after[0], PATHSTATICBUFLEN, &bytes_copied);
	if (error == ENAMETOOLONG) {
		old_rootfs_path_after_buf = zalloc_flags(ZV_NAMEI, Z_WAITOK);
		error = copyinstr(uap->old_rootfs_path_after, old_rootfs_path_after_buf, MAXPATHLEN, &bytes_copied);
	}
	if (error) {
		goto out;
	}

	if (new_rootfs_path_before_buf) {
		incoming = new_rootfs_path_before_buf;
	} else {
		incoming = &new_rootfs_path_before[0];
	}

	if (old_rootfs_path_after_buf) {
		outgoing = old_rootfs_path_after_buf;
	} else {
		outgoing = &old_rootfs_path_after[0];
	}

	  
                                                                              
                                                 
    
	error = vnode_lookup(incoming, 0, &incoming_rootvp, vfs_context_kernel());
	if (error) {
		goto out;
	}
	error = VNOP_IOCTL(incoming_rootvp, FSIOC_KERNEL_ROOTAUTH, NULL, 0, vfs_context_kernel());
	if (error) {
		goto out;
	}

	error = vfs_switch_root(incoming, outgoing, VFSSR_VIRTUALDEV_PROHIBITED);

out:
	if (incoming_rootvp != NULLVP) {
		vnode_put(incoming_rootvp);
		incoming_rootvp = NULLVP;
	}

	if (old_rootfs_path_after_buf) {
		zfree(ZV_NAMEI, old_rootfs_path_after_buf);
	}

	if (new_rootfs_path_before_buf) {
		zfree(ZV_NAMEI, new_rootfs_path_before_buf);
	}

	return error;
}
#else
int
pivot_root(proc_t p, __unused struct pivot_root_args *uap, int *retval)
{
	return nosys(p, NULL, retval);
}
#endif                        

  
                                       
  
                       
                             
                                          
                                                             
   
static int
change_dir(struct nameidata *ndp, vfs_context_t ctx)
{
	vnode_t vp;
	int error;

	if ((error = namei(ndp))) {
		return error;
	}
	nameidone(ndp);
	vp = ndp->ni_vp;

	if (vp->v_type != VDIR) {
		vnode_put(vp);
		return ENOTDIR;
	}

#if CONFIG_MACF
	error = mac_vnode_check_chdir(ctx, vp);
	if (error) {
		vnode_put(vp);
		return error;
	}
#endif

	error = vnode_authorize(vp, NULL, KAUTH_VNODE_SEARCH, ctx);
	if (error) {
		vnode_put(vp);
		return error;
	}

	return error;
}

  
                                                                       
   
struct fd_vn_data *
fg_vn_data_alloc(void)
{
	struct fd_vn_data *fvdata;

	                                
	fvdata = kheap_alloc(KM_FD_VN_DATA, sizeof(struct fd_vn_data),
	    Z_WAITOK | Z_ZERO);
	lck_mtx_init(&fvdata->fv_lock, &fd_vn_lck_grp, &fd_vn_lck_attr);
	return fvdata;
}

  
                                                                       
   
void
fg_vn_data_free(void *fgvndata)
{
	struct fd_vn_data *fvdata = (struct fd_vn_data *)fgvndata;

	kheap_free(KHEAP_DATA_BUFFERS, fvdata->fv_buf, fvdata->fv_bufallocsiz);
	lck_mtx_destroy(&fvdata->fv_lock, &fd_vn_lck_grp);
	kheap_free(KM_FD_VN_DATA, fvdata, sizeof(struct fd_vn_data));
}

  
                                                      
                                           
  
                       
          
         
                
                
                
                   
                
                   
                    
  
                                 
   
int
open1(vfs_context_t ctx, struct nameidata *ndp, int uflags,
    struct vnode_attr *vap, fp_allocfn_t fp_zalloc, void *cra,
    int32_t *retval)
{
	proc_t p = vfs_context_proc(ctx);
	uthread_t uu = get_bsdthread_info(vfs_context_thread(ctx));
	struct fileproc *fp;
	vnode_t vp;
	int flags, oflags;
	int type, indx, error;
	struct vfs_context context;

	oflags = uflags;

	if ((oflags & O_ACCMODE) == O_ACCMODE) {
		return EINVAL;
	}

	flags = FFLAGS(uflags);
	CLR(flags, FENCRYPTED);
	CLR(flags, FUNENCRYPTED);

	AUDIT_ARG(fflags, oflags);
	AUDIT_ARG(mode, vap->va_mode);

	if ((error = falloc_withalloc(p,
	    &fp, &indx, ctx, fp_zalloc, cra)) != 0) {
		return error;
	}
	uu->uu_dupfd = -indx - 1;

	if ((error = vn_open_auth(ndp, &flags, vap))) {
		if ((error == ENODEV || error == ENXIO) && (uu->uu_dupfd >= 0)) {                            
			if ((error = dupfdopen(p->p_fd, indx, uu->uu_dupfd, flags, error)) == 0) {
				fp_drop(p, indx, NULL, 0);
				*retval = indx;
				return 0;
			}
		}
		if (error == ERESTART) {
			error = EINTR;
		}
		fp_free(p, indx, fp);
		return error;
	}
	uu->uu_dupfd = 0;
	vp = ndp->ni_vp;

	fp->fp_glob->fg_flag = flags & (FMASK | O_EVTONLY | FENCRYPTED | FUNENCRYPTED);
	fp->fp_glob->fg_ops = &vnops;
	fp->fp_glob->fg_data = (caddr_t)vp;

	if (flags & (O_EXLOCK | O_SHLOCK)) {
		struct flock lf = {
			.l_whence = SEEK_SET,
		};

		if (flags & O_EXLOCK) {
			lf.l_type = F_WRLCK;
		} else {
			lf.l_type = F_RDLCK;
		}
		type = F_FLOCK;
		if ((flags & FNONBLOCK) == 0) {
			type |= F_WAIT;
		}
#if CONFIG_MACF
		error = mac_file_check_lock(vfs_context_ucred(ctx), fp->fp_glob,
		    F_SETLK, &lf);
		if (error) {
			goto bad;
		}
#endif
		if ((error = VNOP_ADVLOCK(vp, (caddr_t)fp->fp_glob, F_SETLK, &lf, type, ctx, NULL))) {
			goto bad;
		}
		fp->fp_glob->fg_flag |= FWASLOCKED;
	}

	                                                   
	if ((flags & O_TRUNC) && ((error = vnode_setsize(vp, (off_t)0, 0, ctx)) != 0)) {
		goto bad;
	}

	  
                                                                  
    
	if (vnode_vtype(vp) == VDIR) {
		fp->fp_glob->fg_vn_data = fg_vn_data_alloc();
	} else {
		fp->fp_glob->fg_vn_data = NULL;
	}

	vnode_put(vp);

	  
                                                                    
                                                        
    
	if (vnode_istty(vp) && !(p->p_flag & P_CONTROLT) &&
	    !(flags & O_NOCTTY)) {
		int tmp = 0;

		(void)(*fp->fp_glob->fg_ops->fo_ioctl)(fp, (int)TIOCSCTTY,
		    (caddr_t)&tmp, ctx);
	}

	proc_fdlock(p);
	if (flags & O_CLOEXEC) {
		*fdflags(p, indx) |= UF_EXCLOSE;
	}
	if (flags & O_CLOFORK) {
		*fdflags(p, indx) |= UF_FORKCLOSE;
	}
	procfdtbl_releasefd(p, indx, NULL);

#if CONFIG_SECLUDED_MEMORY
	if (secluded_for_filecache &&
	    FILEGLOB_DTYPE(fp->fp_glob) == DTYPE_VNODE &&
	    vnode_vtype(vp) == VREG) {
		memory_object_control_t moc;

		moc = ubc_getobject(vp, UBC_FLAGS_NONE);

		if (moc == MEMORY_OBJECT_CONTROL_NULL) {
			                      
		} else if (fp->fp_glob->fg_flag & FWRITE) {
			                                                        
			memory_object_mark_eligible_for_secluded(moc,
			    FALSE);
		} else if (secluded_for_filecache == 1) {
			char pathname[32] = { 0, };
			size_t copied;
			                                                     
			if (UIO_SEG_IS_USER_SPACE(ndp->ni_segflg)) {
				(void)copyinstr(ndp->ni_dirp,
				    pathname,
				    sizeof(pathname),
				    &copied);
			} else {
				copystr(CAST_DOWN(void *, ndp->ni_dirp),
				    pathname,
				    sizeof(pathname),
				    &copied);
			}
			pathname[sizeof(pathname) - 1] = '\0';
			if (strncmp(pathname,
			    "/Applications/",
			    strlen("/Applications/")) == 0 &&
			    strncmp(pathname,
			    "/Applications/Camera.app/",
			    strlen("/Applications/Camera.app/")) != 0) {
				  
                   
                                
                                               
                                
       
				memory_object_mark_eligible_for_secluded(moc,
				    TRUE);
			}
		} else if (secluded_for_filecache == 2) {
#if __arm64__
#define DYLD_SHARED_CACHE_NAME "dyld_shared_cache_arm64"
#elif __arm__
#define DYLD_SHARED_CACHE_NAME "dyld_shared_cache_armv7"
#else
                        
#endif
			size_t len = strlen(vp->v_name);
			if (!strncmp(vp->v_name, DYLD_SHARED_CACHE_NAME, len) ||
			    !strncmp(vp->v_name, "dyld", len) ||
			    !strncmp(vp->v_name, "launchd", len) ||
			    !strncmp(vp->v_name, "Camera", len) ||
			    !strncmp(vp->v_name, "mediaserverd", len) ||
			    !strncmp(vp->v_name, "SpringBoard", len) ||
			    !strncmp(vp->v_name, "backboardd", len)) {
				  
                                               
                                                
                                                  
       
				memory_object_mark_eligible_for_secluded(moc,
				    FALSE);
			}
		}
	}
#endif                             

	fp_drop(p, indx, fp, 1);
	proc_fdunlock(p);

	*retval = indx;

	return 0;
bad:
	context = *vfs_context_current();
	context.vc_ucred = fp->fp_glob->fg_cred;

	if ((fp->fp_glob->fg_flag & FWASLOCKED) &&
	    (FILEGLOB_DTYPE(fp->fp_glob) == DTYPE_VNODE)) {
		struct flock lf = {
			.l_whence = SEEK_SET,
			.l_type = F_UNLCK,
		};

		(void)VNOP_ADVLOCK(
			vp, (caddr_t)fp->fp_glob, F_UNLCK, &lf, F_FLOCK, ctx, NULL);
	}

	vn_close(vp, fp->fp_glob->fg_flag, &context);
	vnode_put(vp);
	fp_free(p, indx, fp);

	return error;
}

  
                                                                  
                                                                 
                                                                   
                                                                    
                  
   
static int
open1at(vfs_context_t ctx, struct nameidata *ndp, int uflags,
    struct vnode_attr *vap, fp_allocfn_t fp_zalloc, void *cra, int32_t *retval,
    int dirfd)
{
	if ((dirfd != AT_FDCWD) && !(ndp->ni_cnd.cn_flags & USEDVP)) {
		int error;
		char c;

		if (UIO_SEG_IS_USER_SPACE(ndp->ni_segflg)) {
			error = copyin(ndp->ni_dirp, &c, sizeof(char));
			if (error) {
				return error;
			}
		} else {
			c = *((char *)(ndp->ni_dirp));
		}

		if (c != '/') {
			vnode_t dvp_at;

			error = vnode_getfromfd(ndp->ni_cnd.cn_context, dirfd,
			    &dvp_at);
			if (error) {
				return error;
			}

			if (vnode_vtype(dvp_at) != VDIR) {
				vnode_put(dvp_at);
				return ENOTDIR;
			}

			ndp->ni_dvp = dvp_at;
			ndp->ni_cnd.cn_flags |= USEDVP;
			error = open1(ctx, ndp, uflags, vap, fp_zalloc, cra,
			    retval);
			vnode_put(dvp_at);
			return error;
		}
	}

	return open1(ctx, ndp, uflags, vap, fp_zalloc, cra, retval);
}

  
                                                                                                                 
  
                                              
                                              
                                              
                                        
  
                                                     
                                             
                                     
                                     
                                                      
                                           
  
                       
                    
  
                                                                     
  
                                                                       
                                
   
int
open_extended(proc_t p, struct open_extended_args *uap, int32_t *retval)
{
	struct filedesc *fdp = p->p_fd;
	int ciferror;
	kauth_filesec_t xsecdst;
	struct vnode_attr va;
	struct nameidata nd;
	int cmode;

	AUDIT_ARG(owner, uap->uid, uap->gid);

	xsecdst = NULL;
	if ((uap->xsecurity != USER_ADDR_NULL) &&
	    ((ciferror = kauth_copyinfilesec(uap->xsecurity, &xsecdst)) != 0)) {
		return ciferror;
	}

	VATTR_INIT(&va);
	cmode = ((uap->mode & ~fdp->fd_cmask) & ALLPERMS) & ~S_ISTXT;
	VATTR_SET(&va, va_mode, cmode & ACCESSPERMS);
	if (uap->uid != KAUTH_UID_NONE) {
		VATTR_SET(&va, va_uid, uap->uid);
	}
	if (uap->gid != KAUTH_GID_NONE) {
		VATTR_SET(&va, va_gid, uap->gid);
	}
	if (xsecdst != NULL) {
		VATTR_SET(&va, va_acl, &xsecdst->fsec_acl);
	}

	NDINIT(&nd, LOOKUP, OP_OPEN, FOLLOW | AUDITVNPATH1, UIO_USERSPACE,
	    uap->path, vfs_context_current());

	ciferror = open1(vfs_context_current(), &nd, uap->flags, &va,
	    fileproc_alloc_init, NULL, retval);
	if (xsecdst != NULL) {
		kauth_filesec_free(xsecdst);
	}

	return ciferror;
}

  
                                                               
  
                                                                                        
   
int
open_dprotected_np(__unused proc_t p, struct open_dprotected_np_args *uap, int32_t *retval)
{
	int flags = uap->flags;
	int class = uap->class;
	int dpflags = uap->dpflags;

	  
                                          
                                                         
    
	struct filedesc *fdp = p->p_fd;
	struct vnode_attr va;
	struct nameidata nd;
	int cmode;
	int error;

	VATTR_INIT(&va);
	                                                 
	cmode = ((uap->mode & ~fdp->fd_cmask) & ALLPERMS) & ~S_ISTXT;
	VATTR_SET(&va, va_mode, cmode & ACCESSPERMS);

	NDINIT(&nd, LOOKUP, OP_OPEN, FOLLOW | AUDITVNPATH1, UIO_USERSPACE,
	    uap->path, vfs_context_current());

	  
                                                              
                 
                             
                                                                       
    
	if (flags & O_CREAT) {
		                                                                                   
		if (class != PROTECTION_CLASS_DEFAULT) {
			  
                                                                      
                                      
      
			VATTR_SET(&va, va_dataprotect_class, class);
		}
	}

	if (dpflags & (O_DP_GETRAWENCRYPTED | O_DP_GETRAWUNENCRYPTED)) {
		if (flags & (O_RDWR | O_WRONLY)) {
			                                              
			return EINVAL;
		}
		if (uap->dpflags & O_DP_GETRAWENCRYPTED) {
			VATTR_SET(&va, va_dataprotect_flags, VA_DP_RAWENCRYPTED);
		}
		if (uap->dpflags & O_DP_GETRAWUNENCRYPTED) {
			VATTR_SET(&va, va_dataprotect_flags, VA_DP_RAWUNENCRYPTED);
		}
	}

	error = open1(vfs_context_current(), &nd, uap->flags, &va,
	    fileproc_alloc_init, NULL, retval);

	return error;
}

static int
openat_internal(vfs_context_t ctx, user_addr_t path, int flags, int mode,
    int fd, enum uio_seg segflg, int *retval)
{
	struct filedesc *fdp = (vfs_context_proc(ctx))->p_fd;
	struct {
		struct vnode_attr va;
		struct nameidata nd;
	} *__open_data;
	struct vnode_attr *vap;
	struct nameidata *ndp;
	int cmode;
	int error;

	__open_data = kheap_alloc(KHEAP_TEMP, sizeof(*__open_data), Z_WAITOK);
	vap = &__open_data->va;
	ndp = &__open_data->nd;

	VATTR_INIT(vap);
	                                                 
	cmode = ((mode & ~fdp->fd_cmask) & ALLPERMS) & ~S_ISTXT;
	VATTR_SET(vap, va_mode, cmode & ACCESSPERMS);

	NDINIT(ndp, LOOKUP, OP_OPEN, FOLLOW | AUDITVNPATH1,
	    segflg, path, ctx);

	error = open1at(ctx, ndp, flags, vap, fileproc_alloc_init, NULL,
	    retval, fd);

	kheap_free(KHEAP_TEMP, __open_data, sizeof(*__open_data));

	return error;
}

int
open(proc_t p, struct open_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return open_nocancel(p, (struct open_nocancel_args *)uap, retval);
}

int
open_nocancel(__unused proc_t p, struct open_nocancel_args *uap,
    int32_t *retval)
{
	return openat_internal(vfs_context_current(), uap->path, uap->flags,
	           uap->mode, AT_FDCWD, UIO_USERSPACE, retval);
}

int
openat_nocancel(__unused proc_t p, struct openat_nocancel_args *uap,
    int32_t *retval)
{
	return openat_internal(vfs_context_current(), uap->path, uap->flags,
	           uap->mode, uap->fd, UIO_USERSPACE, retval);
}

int
openat(proc_t p, struct openat_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return openat_nocancel(p, (struct openat_nocancel_args *)uap, retval);
}

  
                                                                              
                                                                  
                                                                         
  
                                              
                                              
                                              
                                        
  
                                                     
  
                                       
                                               
                                              
  
                       
                    
  
  
                                                                       
                                
   
int
openbyid_np(__unused proc_t p, struct openbyid_np_args *uap, int *retval)
{
	fsid_t fsid;
	uint64_t objid;
	int error;
	char *buf = NULL;
	int buflen = MAXPATHLEN;
	int pathlen = 0;
	vfs_context_t ctx = vfs_context_current();

	if ((error = priv_check_cred(vfs_context_ucred(ctx), PRIV_VFS_OPEN_BY_ID, 0))) {
		return error;
	}

	if ((error = copyin(uap->fsid, (caddr_t)&fsid, sizeof(fsid)))) {
		return error;
	}

	                                                                     
	if ((error = copyin(uap->objid, (caddr_t)&objid, sizeof(uint64_t)))) {
		return error;
	}

	AUDIT_ARG(value32, fsid.val[0]);
	AUDIT_ARG(value64, objid);

	                                 
	do {
		buf = kheap_alloc(KHEAP_TEMP, buflen + 1, Z_WAITOK);
		if (buf == NULL) {
			return ENOMEM;
		}

		error = fsgetpath_internal( ctx, fsid.val[0], objid, buflen,
		    buf, FSOPT_ISREALFSID, &pathlen);

		if (error) {
			kheap_free(KHEAP_TEMP, buf, buflen + 1);
			buf = NULL;
		}
	} while (error == ENOSPC && (buflen += MAXPATHLEN));

	if (error) {
		return error;
	}

	buf[pathlen] = 0;

	error = openat_internal(
		ctx, (user_addr_t)buf, uap->oflags, 0, AT_FDCWD, UIO_SYSSPACE, retval);

	kheap_free(KHEAP_TEMP, buf, buflen + 1);

	return error;
}


  
                         
   
static int mkfifo1(vfs_context_t ctx, user_addr_t upath, struct vnode_attr *vap);

int
mknod(proc_t p, struct mknod_args *uap, __unused int32_t *retval)
{
	struct vnode_attr va;
	vfs_context_t ctx = vfs_context_current();
	int error;
	struct nameidata nd;
	vnode_t vp, dvp;

	VATTR_INIT(&va);
	VATTR_SET(&va, va_mode, (uap->mode & ALLPERMS) & ~p->p_fd->fd_cmask);
	VATTR_SET(&va, va_rdev, uap->dev);

	                                                         
	if ((uap->mode & S_IFMT) == S_IFIFO) {
		return mkfifo1(ctx, uap->path, &va);
	}

	AUDIT_ARG(mode, (mode_t)uap->mode);
	AUDIT_ARG(value32, uap->dev);

	if ((error = suser(vfs_context_ucred(ctx), &p->p_acflag))) {
		return error;
	}
	NDINIT(&nd, CREATE, OP_MKNOD, LOCKPARENT | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);
	error = namei(&nd);
	if (error) {
		return error;
	}
	dvp = nd.ni_dvp;
	vp = nd.ni_vp;

	if (vp != NULL) {
		error = EEXIST;
		goto out;
	}

	switch (uap->mode & S_IFMT) {
	case S_IFCHR:
		VATTR_SET(&va, va_type, VCHR);
		break;
	case S_IFBLK:
		VATTR_SET(&va, va_type, VBLK);
		break;
	default:
		error = EINVAL;
		goto out;
	}

#if CONFIG_MACF
	error = mac_vnode_check_create(ctx,
	    nd.ni_dvp, &nd.ni_cnd, &va);
	if (error) {
		goto out;
	}
#endif

	if ((error = vnode_authorize(dvp, NULL, KAUTH_VNODE_ADD_FILE, ctx)) != 0) {
		goto out;
	}

	if ((error = vn_create(dvp, &vp, &nd, &va, 0, 0, NULL, ctx)) != 0) {
		goto out;
	}

	if (vp) {
		int     update_flags = 0;

		                                                     
		if (vp->v_name == NULL) {
			update_flags |= VNODE_UPDATE_NAME;
		}
		if (vp->v_parent == NULLVP) {
			update_flags |= VNODE_UPDATE_PARENT;
		}

		if (update_flags) {
			vnode_update_identity(vp, dvp, nd.ni_cnd.cn_nameptr, nd.ni_cnd.cn_namelen, nd.ni_cnd.cn_hash, update_flags);
		}

#if CONFIG_FSE
		add_fsevent(FSE_CREATE_FILE, ctx,
		    FSE_ARG_VNODE, vp,
		    FSE_ARG_DONE);
#endif
	}

out:
	  
                                                    
                                                           
    
	nameidone(&nd);

	if (vp) {
		vnode_put(vp);
	}
	vnode_put(dvp);

	return error;
}

  
                       
  
                       
          
            
                      
                
   
static int
mkfifo1(vfs_context_t ctx, user_addr_t upath, struct vnode_attr *vap)
{
	vnode_t vp, dvp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, CREATE, OP_MKFIFO, LOCKPARENT | AUDITVNPATH1,
	    UIO_USERSPACE, upath, ctx);
	error = namei(&nd);
	if (error) {
		return error;
	}
	dvp = nd.ni_dvp;
	vp = nd.ni_vp;

	                                                          
	if (vp != NULL) {
		error = EEXIST;
		goto out;
	}
	VATTR_SET(vap, va_type, VFIFO);

	if ((error = vn_authorize_create(dvp, &nd.ni_cnd, vap, ctx, NULL)) != 0) {
		goto out;
	}

	error = vn_create(dvp, &vp, &nd, vap, 0, 0, NULL, ctx);
out:
	  
                                                    
                                                           
    
	nameidone(&nd);

	if (vp) {
		vnode_put(vp);
	}
	vnode_put(dvp);

	return error;
}


  
                                                                                                         
  
                                              
                                              
                      
  
                                                       
                        
                        
                                                  
                                           
  
                       
                    
  
                                                                     
  
                                                                       
                                
   
int
mkfifo_extended(proc_t p, struct mkfifo_extended_args *uap, __unused int32_t *retval)
{
	int ciferror;
	kauth_filesec_t xsecdst;
	struct vnode_attr va;

	AUDIT_ARG(owner, uap->uid, uap->gid);

	xsecdst = KAUTH_FILESEC_NONE;
	if (uap->xsecurity != USER_ADDR_NULL) {
		if ((ciferror = kauth_copyinfilesec(uap->xsecurity, &xsecdst)) != 0) {
			return ciferror;
		}
	}

	VATTR_INIT(&va);
	VATTR_SET(&va, va_mode, (uap->mode & ALLPERMS) & ~p->p_fd->fd_cmask);
	if (uap->uid != KAUTH_UID_NONE) {
		VATTR_SET(&va, va_uid, uap->uid);
	}
	if (uap->gid != KAUTH_GID_NONE) {
		VATTR_SET(&va, va_gid, uap->gid);
	}
	if (xsecdst != KAUTH_FILESEC_NONE) {
		VATTR_SET(&va, va_acl, &xsecdst->fsec_acl);
	}

	ciferror = mkfifo1(vfs_context_current(), uap->path, &va);

	if (xsecdst != KAUTH_FILESEC_NONE) {
		kauth_filesec_free(xsecdst);
	}
	return ciferror;
}

              
int
mkfifo(proc_t p, struct mkfifo_args *uap, __unused int32_t *retval)
{
	struct vnode_attr va;

	VATTR_INIT(&va);
	VATTR_SET(&va, va_mode, (uap->mode & ALLPERMS) & ~p->p_fd->fd_cmask);

	return mkfifo1(vfs_context_current(), uap->path, &va);
}

extern int safe_getpath_new(struct vnode *dvp, char *leafname, char *path, int _len, int *truncated_path, int firmlink);
extern int safe_getpath(struct vnode *dvp, char *leafname, char *path, int _len, int *truncated_path);
extern int safe_getpath_no_firmlink(struct vnode *dvp, char *leafname, char *path, int _len, int *truncated_path);

int
safe_getpath_new(struct vnode *dvp, char *leafname, char *path, int _len, int *truncated_path, int firmlink)
{
	int ret, len = _len;

	*truncated_path = 0;

	if (firmlink) {
		ret = vn_getpath(dvp, path, &len);
	} else {
		ret = vn_getpath_no_firmlink(dvp, path, &len);
	}
	if (ret == 0 && len < (MAXPATHLEN - 1)) {
		if (leafname) {
			path[len - 1] = '/';
			len += strlcpy(&path[len], leafname, MAXPATHLEN - len) + 1;
			if (len > MAXPATHLEN) {
				char *ptr;

				                            
				*truncated_path = 1;
				ptr = strrchr(path, '/');
				if (ptr) {
					*ptr = '\0';                                                         
				}
				len = (int)strlen(path) + 1;
			}
		}
	} else if (ret == 0) {
		*truncated_path = 1;
	} else if (ret != 0) {
		struct vnode *mydvp = dvp;

		if (ret != ENOSPC) {
			printf("safe_getpath: failed to get the path for vp %p (%s) : err %d\n",
			    dvp, dvp->v_name ? dvp->v_name : "no-name", ret);
		}
		*truncated_path = 1;

		do {
			if (mydvp->v_parent != NULL) {
				mydvp = mydvp->v_parent;
			} else if (mydvp->v_mount) {
				strlcpy(path, mydvp->v_mount->mnt_vfsstat.f_mntonname, _len);
				break;
			} else {
				                                                                           
				strlcpy(path, "/", _len);
				len = 2;
				mydvp = NULL;
			}

			if (mydvp == NULL) {
				break;
			}

			len = _len;
			if (firmlink) {
				ret = vn_getpath(mydvp, path, &len);
			} else {
				ret = vn_getpath_no_firmlink(mydvp, path, &len);
			}
		} while (ret == ENOSPC);
	}

	return len;
}

int
safe_getpath(struct vnode *dvp, char *leafname, char *path, int _len, int *truncated_path)
{
	return safe_getpath_new(dvp, leafname, path, _len, truncated_path, 1);
}

int
safe_getpath_no_firmlink(struct vnode *dvp, char *leafname, char *path, int _len, int *truncated_path)
{
	return safe_getpath_new(dvp, leafname, path, _len, truncated_path, 0);
}

  
                         
  
                       
         
          
         
            
                      
                
   
              
static int
linkat_internal(vfs_context_t ctx, int fd1, user_addr_t path, int fd2,
    user_addr_t link, int flag, enum uio_seg segflg)
{
	vnode_t vp, pvp, dvp, lvp;
	struct nameidata nd;
	int follow;
	int error;
#if CONFIG_FSE
	fse_info finfo;
#endif
	int need_event, has_listeners, need_kpath2;
	char *target_path = NULL;
	int truncated = 0;

	vp = dvp = lvp = NULLVP;

	                                          
	follow = (flag & AT_SYMLINK_FOLLOW) ? FOLLOW : NOFOLLOW;
	NDINIT(&nd, LOOKUP, OP_LOOKUP, AUDITVNPATH1 | follow,
	    segflg, path, ctx);

	error = nameiat(&nd, fd1);
	if (error) {
		return error;
	}
	vp = nd.ni_vp;

	nameidone(&nd);

	  
                                                      
                                                        
    
	if (vp->v_type == VDIR) {
		if (!ISSET(vp->v_mount->mnt_kern_flag, MNTK_DIR_HARDLINKS)) {
			error = EPERM;              
			goto out;
		}

		                                                
		if (!kauth_cred_issuser(vfs_context_ucred(ctx))) {
			struct vnode_attr dva;

			VATTR_INIT(&dva);
			VATTR_WANTED(&dva, va_uid);
			if (vnode_getattr(vp, &dva, ctx) != 0 ||
			    !VATTR_IS_SUPPORTED(&dva, va_uid) ||
			    (dva.va_uid != kauth_cred_getuid(vfs_context_ucred(ctx)))) {
				error = EACCES;
				goto out;
			}
		}
	}

	                            
#if CONFIG_TRIGGERS
	nd.ni_op = OP_LINK;
#endif
	nd.ni_cnd.cn_nameiop = CREATE;
	nd.ni_cnd.cn_flags = LOCKPARENT | AUDITVNPATH2 | CN_NBMOUNTLOOK;
	nd.ni_dirp = link;
	error = nameiat(&nd, fd2);
	if (error != 0) {
		goto out;
	}
	dvp = nd.ni_dvp;
	lvp = nd.ni_vp;

#if CONFIG_MACF
	if ((error = mac_vnode_check_link(ctx, dvp, vp, &nd.ni_cnd)) != 0) {
		goto out2;
	}
#endif

	                                                                        
	if ((error = vnode_authorize(vp, NULL, KAUTH_VNODE_LINKTARGET, ctx)) != 0) {
		goto out2;
	}

	                                
	if (lvp != NULLVP) {
		error = EEXIST;
		goto out2;
	}
	                                    
	if (vnode_mount(vp) != vnode_mount(dvp)) {
		error = EXDEV;
		goto out2;
	}

	                                           
	if ((error = vnode_authorize(dvp, NULL, KAUTH_VNODE_ADD_FILE, ctx)) != 0) {
		goto out2;
	}

	                               
	error = VNOP_LINK(vp, dvp, &nd.ni_cnd, ctx);
	if (error) {
		goto out2;
	}

#if CONFIG_MACF
	(void)mac_vnode_notify_link(ctx, vp, dvp, &nd.ni_cnd);
#endif

#if CONFIG_FSE
	need_event = need_fsevent(FSE_CREATE_FILE, dvp);
#else
	need_event = 0;
#endif
	has_listeners = kauth_authorize_fileop_has_listeners();

	need_kpath2 = 0;
#if CONFIG_AUDIT
	if (AUDIT_RECORD_EXISTS()) {
		need_kpath2 = 1;
	}
#endif

	if (need_event || has_listeners || need_kpath2) {
		char *link_to_path = NULL;
		int len, link_name_len;

		                                         
		GET_PATH(target_path);

		len = safe_getpath(dvp, nd.ni_cnd.cn_nameptr, target_path, MAXPATHLEN, &truncated);

		AUDIT_ARG(kpath, target_path, ARG_KPATH2);

		if (has_listeners) {
			                                              
			GET_PATH(link_to_path);

			link_name_len = MAXPATHLEN;
			if (vn_getpath(vp, link_to_path, &link_name_len) == 0) {
				  
                                                          
                                                    
       
				kauth_authorize_fileop(vfs_context_ucred(ctx), KAUTH_FILEOP_LINK,
				    (uintptr_t)link_to_path,
				    (uintptr_t)target_path);
			}
			if (link_to_path != NULL) {
				RELEASE_PATH(link_to_path);
			}
		}
#if CONFIG_FSE
		if (need_event) {
			                       
			if (get_fse_info(vp, &finfo, ctx) == 0) {
				if (truncated) {
					finfo.mode |= FSE_TRUNCATED_PATH;
				}

				                                                
				add_fsevent(FSE_CREATE_FILE, ctx,
				    FSE_ARG_STRING, len, target_path,
				    FSE_ARG_FINFO, &finfo,
				    FSE_ARG_DONE);
			}

			pvp = vp->v_parent;
			                                      
			if (pvp && pvp != dvp) {
				error = vnode_get(pvp);
				if (error) {
					pvp = NULLVP;
					error = 0;
				}
			}
			if (pvp) {
				add_fsevent(FSE_STAT_CHANGED, ctx,
				    FSE_ARG_VNODE, pvp, FSE_ARG_DONE);
			}
			if (pvp && pvp != dvp) {
				vnode_put(pvp);
			}
		}
#endif
	}
out2:
	  
                                                    
                                                           
    
	nameidone(&nd);
	if (target_path != NULL) {
		RELEASE_PATH(target_path);
	}
out:
	if (lvp) {
		vnode_put(lvp);
	}
	if (dvp) {
		vnode_put(dvp);
	}
	vnode_put(vp);
	return error;
}

int
link(__unused proc_t p, struct link_args *uap, __unused int32_t *retval)
{
	return linkat_internal(vfs_context_current(), AT_FDCWD, uap->path,
	           AT_FDCWD, uap->link, AT_SYMLINK_FOLLOW, UIO_USERSPACE);
}

int
linkat(__unused proc_t p, struct linkat_args *uap, __unused int32_t *retval)
{
	if (uap->flag & ~AT_SYMLINK_FOLLOW) {
		return EINVAL;
	}

	return linkat_internal(vfs_context_current(), uap->fd1, uap->path,
	           uap->fd2, uap->link, uap->flag, UIO_USERSPACE);
}

  
                        
  
                                            
   
              
static int
symlinkat_internal(vfs_context_t ctx, user_addr_t path_data, int fd,
    user_addr_t link, enum uio_seg segflg)
{
	struct vnode_attr va;
	char *path;
	int error;
	struct nameidata nd;
	vnode_t vp, dvp;
	size_t dummy = 0;
	proc_t p;

	error = 0;
	if (UIO_SEG_IS_USER_SPACE(segflg)) {
		path = zalloc(ZV_NAMEI);
		error = copyinstr(path_data, path, MAXPATHLEN, &dummy);
	} else {
		path = (char *)path_data;
	}
	if (error) {
		goto out;
	}
	AUDIT_ARG(text, path);                               

	NDINIT(&nd, CREATE, OP_SYMLINK, LOCKPARENT | AUDITVNPATH1,
	    segflg, link, ctx);

	error = nameiat(&nd, fd);
	if (error) {
		goto out;
	}
	dvp = nd.ni_dvp;
	vp = nd.ni_vp;

	p = vfs_context_proc(ctx);
	VATTR_INIT(&va);
	VATTR_SET(&va, va_type, VLNK);
	VATTR_SET(&va, va_mode, ACCESSPERMS & ~p->p_fd->fd_cmask);

#if CONFIG_MACF
	error = mac_vnode_check_create(ctx,
	    dvp, &nd.ni_cnd, &va);
#endif
	if (error != 0) {
		goto skipit;
	}

	if (vp != NULL) {
		error = EEXIST;
		goto skipit;
	}

	               
	if (error == 0) {
		error = vnode_authorize(dvp, NULL, KAUTH_VNODE_ADD_FILE, ctx);
	}
	                                 
	if (error == 0) {
		error = vnode_authattr_new(dvp, &va, 0, ctx);
	}
	if (error == 0) {
		error = VNOP_SYMLINK(dvp, &vp, &nd.ni_cnd, &va, path, ctx);
	}

	                                    
	if (error == 0 && vp) {
		error = vnode_setattr_fallback(vp, &va, ctx);
	}

#if CONFIG_MACF
	if (error == 0 && vp) {
		error = vnode_label(vnode_mount(vp), dvp, vp, &nd.ni_cnd, VNODE_LABEL_CREATE, ctx);
	}
#endif

	if (error == 0) {
		int     update_flags = 0;

		                                                         
		if (vp == NULL) {
			nd.ni_cnd.cn_nameiop = LOOKUP;
#if CONFIG_TRIGGERS
			nd.ni_op = OP_LOOKUP;
#endif
			nd.ni_cnd.cn_flags = 0;
			error = nameiat(&nd, fd);
			vp = nd.ni_vp;

			if (vp == NULL) {
				goto skipit;
			}
		}

#if 0                                                          
		                                                      
                                                  
     
		if (kauth_authorize_fileop_has_listeners() &&
		    namei(&nd) == 0) {
			char *new_link_path = NULL;
			int             len;

			                                         
			new_link_path = get_pathbuff();
			len = MAXPATHLEN;
			vn_getpath(dvp, new_link_path, &len);
			if ((len + 1 + nd.ni_cnd.cn_namelen + 1) < MAXPATHLEN) {
				new_link_path[len - 1] = '/';
				strlcpy(&new_link_path[len], nd.ni_cnd.cn_nameptr, MAXPATHLEN - len);
			}

			kauth_authorize_fileop(vfs_context_ucred(ctx), KAUTH_FILEOP_SYMLINK,
			    (uintptr_t)path, (uintptr_t)new_link_path);
			if (new_link_path != NULL) {
				release_pathbuff(new_link_path);
			}
		}
#endif
		                                                     
		if (vp->v_name == NULL) {
			update_flags |= VNODE_UPDATE_NAME;
		}
		if (vp->v_parent == NULLVP) {
			update_flags |= VNODE_UPDATE_PARENT;
		}

		if (update_flags) {
			vnode_update_identity(vp, dvp, nd.ni_cnd.cn_nameptr, nd.ni_cnd.cn_namelen, nd.ni_cnd.cn_hash, update_flags);
		}

#if CONFIG_FSE
		add_fsevent(FSE_CREATE_FILE, ctx,
		    FSE_ARG_VNODE, vp,
		    FSE_ARG_DONE);
#endif
	}

skipit:
	  
                                                    
                                                           
    
	nameidone(&nd);

	if (vp) {
		vnode_put(vp);
	}
	vnode_put(dvp);
out:
	if (path && (path != (char *)path_data)) {
		zfree(ZV_NAMEI, path);
	}

	return error;
}

int
symlink(__unused proc_t p, struct symlink_args *uap, __unused int32_t *retval)
{
	return symlinkat_internal(vfs_context_current(), uap->path, AT_FDCWD,
	           uap->link, UIO_USERSPACE);
}

int
symlinkat(__unused proc_t p, struct symlinkat_args *uap,
    __unused int32_t *retval)
{
	return symlinkat_internal(vfs_context_current(), uap->path1, uap->fd,
	           uap->path2, UIO_USERSPACE);
}

  
                                         
                       
   
int
undelete(__unused proc_t p, __unused struct undelete_args *uap, __unused int32_t *retval)
{
	return ENOTSUP;
}

  
                                     
   
              
static int
unlinkat_internal(vfs_context_t ctx, int fd, vnode_t start_dvp,
    user_addr_t path_arg, enum uio_seg segflg, int unlink_flags)
{
	struct nameidata nd;
	vnode_t vp, dvp;
	int error;
	struct componentname *cnp;
	char  *path = NULL;
	char  *no_firmlink_path = NULL;
	int  len_path = 0;
	int  len_no_firmlink_path = 0;
#if CONFIG_FSE
	fse_info  finfo;
	struct vnode_attr va;
#endif
	int flags;
	int need_event;
	int has_listeners;
	int truncated_path;
	int truncated_no_firmlink_path;
	int batched;
	struct vnode_attr *vap;
	int do_retry;
	int retry_count = 0;
	int cn_flags;

	cn_flags = LOCKPARENT;
	if (!(unlink_flags & VNODE_REMOVE_NO_AUDIT_PATH)) {
		cn_flags |= AUDITVNPATH1;
	}
	                                                           
	if (start_dvp) {
		cn_flags |= USEDVP;
	}

#if NAMEDRSRCFORK
	                                                                 
	cn_flags |= CN_ALLOWRSRCFORK;
#endif

retry:
	do_retry = 0;
	flags = 0;
	need_event = 0;
	has_listeners = 0;
	truncated_path = 0;
	truncated_no_firmlink_path = 0;
	vap = NULL;

	NDINIT(&nd, DELETE, OP_UNLINK, cn_flags, segflg, path_arg, ctx);

	nd.ni_dvp = start_dvp;
	nd.ni_flag |= NAMEI_COMPOUNDREMOVE;
	cnp = &nd.ni_cnd;

continue_lookup:
	error = nameiat(&nd, fd);
	if (error) {
		return error;
	}

	dvp = nd.ni_dvp;
	vp = nd.ni_vp;


	                                                                
	if (unlink_flags & VNODE_REMOVE_NODELETEBUSY) {
		flags |= VNODE_REMOVE_NODELETEBUSY;
	}

	                                            
	if (unlink_flags & VNODE_REMOVE_SKIP_NAMESPACE_EVENT) {
		flags |= VNODE_REMOVE_SKIP_NAMESPACE_EVENT;
	}

	if (vp) {
		batched = vnode_compound_remove_available(vp);
		  
                                                        
     
		if ((vp->v_flag & VROOT) || (dvp->v_mount != vp->v_mount)) {
			error = EBUSY;
			goto out;
		}

#if DEVELOPMENT || DEBUG
		  
                                                           
                                             
     
#else                           

		if (vnode_isswap(vp) && (ctx != vfs_context_kernel())) {
			error = EPERM;
			goto out;
		}
#endif                           

		if (!batched) {
			error = vn_authorize_unlink(dvp, vp, cnp, ctx, NULL);
			if (error) {
				if (error == ENOENT) {
					if (retry_count < MAX_AUTHORIZE_ENOENT_RETRIES) {
						do_retry = 1;
						retry_count++;
					}
				}
				goto out;
			}
		}
	} else {
		batched = 1;

		if (!vnode_compound_remove_available(dvp)) {
			panic("No vp, but no compound remove?");
		}
	}

#if CONFIG_FSE
	need_event = need_fsevent(FSE_DELETE, dvp);
	if (need_event) {
		if (!batched) {
			if ((vp->v_flag & VISHARDLINK) == 0) {
				                                                
				get_fse_info(vp, &finfo, ctx);
			}
		} else {
			error = vfs_get_notify_attributes(&va);
			if (error) {
				goto out;
			}

			vap = &va;
		}
	}
#endif
	has_listeners = kauth_authorize_fileop_has_listeners();
	if (need_event || has_listeners) {
		if (path == NULL) {
			GET_PATH(path);
		}
		len_path = safe_getpath(dvp, nd.ni_cnd.cn_nameptr, path, MAXPATHLEN, &truncated_path);
		if (no_firmlink_path == NULL) {
			GET_PATH(no_firmlink_path);
		}
		len_no_firmlink_path = safe_getpath_no_firmlink(dvp, nd.ni_cnd.cn_nameptr, no_firmlink_path, MAXPATHLEN, &truncated_no_firmlink_path);
	}

#if NAMEDRSRCFORK
	if (nd.ni_cnd.cn_flags & CN_WANTSRSRCFORK) {
		error = vnode_removenamedstream(dvp, vp, XATTR_RESOURCEFORK_NAME, 0, ctx);
	} else
#endif
	{
		error = vn_remove(dvp, &nd.ni_vp, &nd, flags, vap, ctx);
		vp = nd.ni_vp;
		if (error == EKEEPLOOKING) {
			if (!batched) {
				panic("EKEEPLOOKING, but not a filesystem that supports compound VNOPs?");
			}

			if ((nd.ni_flag & NAMEI_CONTLOOKUP) == 0) {
				panic("EKEEPLOOKING, but continue flag not set?");
			}

			if (vnode_isdir(vp)) {
				error = EISDIR;
				goto out;
			}
			goto continue_lookup;
		} else if (error == ENOENT && batched) {
			if (retry_count < MAX_AUTHORIZE_ENOENT_RETRIES) {
				  
                                                         
                                                       
                                                   
       
				do_retry = 1;
				retry_count += 1;
				goto out;
			}
		}
	}

	  
                                                       
                                                 
    
	if (!error) {
		if (has_listeners) {
			kauth_authorize_fileop(vfs_context_ucred(ctx),
			    KAUTH_FILEOP_DELETE,
			    (uintptr_t)vp,
			    (uintptr_t)path);
		}

		if (vp->v_flag & VISHARDLINK) {
			  
			                                                      
			                                                     
			                                                      
			                                                      
			                                                      
			  
			vnode_update_identity(vp, NULL, NULL, 0, 0, VNODE_UPDATE_PARENT);
		}

#if CONFIG_FSE
		if (need_event) {
			if (vp->v_flag & VISHARDLINK) {
				get_fse_info(vp, &finfo, ctx);
			} else if (vap) {
				vnode_get_fse_info_from_vap(vp, &finfo, vap);
			}
			if (truncated_path) {
				finfo.mode |= FSE_TRUNCATED_PATH;
			}
			add_fsevent(FSE_DELETE, ctx,
			    FSE_ARG_STRING, len_no_firmlink_path, no_firmlink_path,
			    FSE_ARG_FINFO, &finfo,
			    FSE_ARG_DONE);
		}
#endif
	}

out:
	if (path != NULL) {
		RELEASE_PATH(path);
		path = NULL;
	}

	if (no_firmlink_path != NULL) {
		RELEASE_PATH(no_firmlink_path);
		no_firmlink_path = NULL;
	}
#if NAMEDRSRCFORK
	                                                                
                                                       
    
	if (vp && (vnode_isnamedstream(vp)) &&
	    (vp->v_parent != NULLVP) &&
	    vnode_isshadow(vp)) {
		vnode_recycle(vp);
	}
#endif
	  
                                                    
                                                           
    
	nameidone(&nd);
	vnode_put(dvp);
	if (vp) {
		vnode_put(vp);
	}

	if (do_retry) {
		goto retry;
	}

	return error;
}

int
unlink1(vfs_context_t ctx, vnode_t start_dvp, user_addr_t path_arg,
    enum uio_seg segflg, int unlink_flags)
{
	return unlinkat_internal(ctx, AT_FDCWD, start_dvp, path_arg, segflg,
	           unlink_flags);
}

  
                                                            
   
int
delete(__unused proc_t p, struct delete_args *uap, __unused int32_t *retval)
{
	return unlinkat_internal(vfs_context_current(), AT_FDCWD, NULLVP,
	           uap->path, UIO_USERSPACE, VNODE_REMOVE_NODELETEBUSY);
}

  
                                                           
   
int
unlink(__unused proc_t p, struct unlink_args *uap, __unused int32_t *retval)
{
	return unlinkat_internal(vfs_context_current(), AT_FDCWD, NULLVP,
	           uap->path, UIO_USERSPACE, 0);
}

int
unlinkat(__unused proc_t p, struct unlinkat_args *uap, __unused int32_t *retval)
{
	if (uap->flag & ~(AT_REMOVEDIR | AT_REMOVEDIR_DATALESS)) {
		return EINVAL;
	}

	if (uap->flag & (AT_REMOVEDIR | AT_REMOVEDIR_DATALESS)) {
		int unlink_flags = 0;

		if (uap->flag & AT_REMOVEDIR_DATALESS) {
			unlink_flags |= VNODE_REMOVE_DATALESS_DIR;
		}
		return rmdirat_internal(vfs_context_current(), uap->fd,
		           uap->path, UIO_USERSPACE, unlink_flags);
	} else {
		return unlinkat_internal(vfs_context_current(), uap->fd,
		           NULLVP, uap->path, UIO_USERSPACE, 0);
	}
}

  
                                     
   
int
lseek(proc_t p, struct lseek_args *uap, off_t *retval)
{
	struct fileproc *fp;
	vnode_t vp;
	struct vfs_context *ctx;
	off_t offset = uap->offset, file_size;
	int error;

	if ((error = fp_getfvp(p, uap->fd, &fp, &vp))) {
		if (error == ENOTSUP) {
			return ESPIPE;
		}
		return error;
	}
	if (vnode_isfifo(vp)) {
		file_drop(uap->fd);
		return ESPIPE;
	}


	ctx = vfs_context_current();
#if CONFIG_MACF
	if (uap->whence == L_INCR && uap->offset == 0) {
		error = mac_file_check_get_offset(vfs_context_ucred(ctx),
		    fp->fp_glob);
	} else {
		error = mac_file_check_change_offset(vfs_context_ucred(ctx),
		    fp->fp_glob);
	}
	if (error) {
		file_drop(uap->fd);
		return error;
	}
#endif
	if ((error = vnode_getwithref(vp))) {
		file_drop(uap->fd);
		return error;
	}

	switch (uap->whence) {
	case L_INCR:
		offset += fp->fp_glob->fg_offset;
		break;
	case L_XTND:
		if ((error = vnode_size(vp, &file_size, ctx)) != 0) {
			break;
		}
		offset += file_size;
		break;
	case L_SET:
		break;
	case SEEK_HOLE:
		error = VNOP_IOCTL(vp, FSIOC_FIOSEEKHOLE, (caddr_t)&offset, 0, ctx);
		break;
	case SEEK_DATA:
		error = VNOP_IOCTL(vp, FSIOC_FIOSEEKDATA, (caddr_t)&offset, 0, ctx);
		break;
	default:
		error = EINVAL;
	}
	if (error == 0) {
		if (uap->offset > 0 && offset < 0) {
			                                             
			error = EOVERFLOW;
		} else {
			  
                                                      
                                                      
             
      
			if (offset < 0 && vp->v_type != VCHR) {
				                                            
				error = EINVAL;
			} else {
				             
				fp->fp_glob->fg_offset = offset;
				*retval = fp->fp_glob->fg_offset;
			}
		}
	}

	  
                                                                 
                                                    
    
	post_event_if_success(vp, error, NOTE_NONE);
	(void)vnode_put(vp);
	file_drop(uap->fd);
	return error;
}


  
                            
  
                       
                       
   
static int
access1(vnode_t vp, vnode_t dvp, int uflags, vfs_context_t ctx)
{
	kauth_action_t action;
	int error;

	  
                                                              
                                         
    
	if (!(uflags & _ACCESS_EXTENDED_MASK)) {
		action = 0;
		if (uflags & R_OK) {
			action |= KAUTH_VNODE_READ_DATA;                                            
		}
		if (uflags & W_OK) {
			if (vnode_isdir(vp)) {
				action |= KAUTH_VNODE_ADD_FILE |
				    KAUTH_VNODE_ADD_SUBDIRECTORY;
				                                       
			} else {
				action |= KAUTH_VNODE_WRITE_DATA;
			}
		}
		if (uflags & X_OK) {
			if (vnode_isdir(vp)) {
				action |= KAUTH_VNODE_SEARCH;
			} else {
				action |= KAUTH_VNODE_EXECUTE;
			}
		}
	} else {
		                                            
		action = uflags >> 8;
	}

#if CONFIG_MACF
	error = mac_vnode_check_access(ctx, vp, uflags);
	if (error) {
		return error;
	}
#endif          

	                                                
	if (action != 0) {
		error = vnode_authorize(vp, dvp, action | KAUTH_VNODE_ACCESS, ctx);
	} else {
		error = 0;
	}

	return error;
}



  
                                                     
  
                                                            
                                                                       
                                                                         
                                                               
                                             
                    
                                               
  
                       
                                
                              
                             
                                        
                                           
                                                  
                                    
                                  
             
            
  
                    
                                         
  
                                                                      
                                                                   
           
  
                                 
        
                                 
                       
  
                                                                 
                                                                 
                                                               
                      
  
                                                         
          
                                      
  
                                                               
                                                                 
  
                                                  
   
int
access_extended(__unused proc_t p, struct access_extended_args *uap, __unused int32_t *retval)
{
	struct accessx_descriptor *input = NULL;
	errno_t *result = NULL;
	errno_t error = 0;
	int wantdelete = 0;
	size_t desc_max, desc_actual;
	unsigned int i, j;
	struct vfs_context context;
	struct nameidata nd;
	int niopts;
	vnode_t vp = NULL;
	vnode_t dvp = NULL;
#define ACCESSX_MAX_DESCR_ON_STACK 10
	struct accessx_descriptor stack_input[ACCESSX_MAX_DESCR_ON_STACK];

	context.vc_ucred = NULL;

	  
                                                                       
                                                                  
                                  
   
                                                                     
                                                                   
                                                   
                                                    
    
	if (uap->size > ACCESSX_MAX_TABLESIZE) {
		return ENOMEM;
	}
	if (uap->size < (sizeof(struct accessx_descriptor) + 2)) {
		return EINVAL;
	}
	if (uap->size <= sizeof(stack_input)) {
		input = stack_input;
	} else {
		input = kheap_alloc(KHEAP_DATA_BUFFERS, uap->size, Z_WAITOK);
		if (input == NULL) {
			error = ENOMEM;
			goto out;
		}
	}
	error = copyin(uap->entries, input, uap->size);
	if (error) {
		goto out;
	}

	AUDIT_ARG(opaque, input, uap->size);

	  
                                                                      
                                                                    
                 
    
	((char *)input)[uap->size - 1] = 0;

	  
                                                                     
                                                                 
                                             
    
	context.vc_ucred = kauth_cred_copy_real(kauth_cred_get());
	context.vc_thread = current_thread();

	  
                                                                    
                                                                     
                                  
    
	desc_max = (uap->size - 2) / sizeof(struct accessx_descriptor);
	desc_actual = desc_max;
	for (i = 0; i < desc_actual; i++) {
		  
                                                          
                                                            
                                                                
                 
     
		j = input[i].ad_name_offset / sizeof(struct accessx_descriptor);

		  
                                                                 
                                                     
                                                            
                                                              
     
		if (j > desc_max || (j != 0 && j <= i)) {
			error = EINVAL;
			goto out;
		}

		                                                                                    
		if (input[i].ad_name_offset >= uap->size) {
			error = EINVAL;
			goto out;
		}

		  
                                                               
                                                              
                               
     
		if (j == 0) {
			                                           
			if (i == 0) {
				error = EINVAL;
				goto out;
			}
			continue;
		}

		  
                                                              
                                                           
                                                               
                                                             
                                              
     
		if (j < desc_actual) {
			desc_actual = j;
		}
	}

	  
                                                                       
                                                                      
                                         
    
	if (desc_actual > ACCESSX_MAX_DESCRIPTORS) {
		error = ENOMEM;
		goto out;
	}
	result = kheap_alloc(KHEAP_DATA_BUFFERS, desc_actual * sizeof(errno_t),
	    Z_WAITOK | Z_ZERO);
	if (result == NULL) {
		error = ENOMEM;
		goto out;
	}

	  
                                                                   
                                          
    
	error = 0;
	for (i = 0; i < desc_actual; i++) {
		  
                                                         
                                                            
                     
     
		if (input[i].ad_name_offset != 0) {
			                        
			if (vp) {
				vnode_put(vp);
				vp = NULL;
			}
			if (dvp) {
				vnode_put(dvp);
				dvp = NULL;
			}

			  
                                                      
                                                       
                                                    
                                                     
                                         
      
			wantdelete = input[i].ad_flags & _DELETE_OK;
			for (j = i + 1; (j < desc_actual) && (input[j].ad_name_offset == 0); j++) {
				if (input[j].ad_flags & _DELETE_OK) {
					wantdelete = 1;
				}
			}

			niopts = FOLLOW | AUDITVNPATH1;

			                                                       
			if (wantdelete) {
				niopts |= WANTPARENT;
			}

			                   
			NDINIT(&nd, LOOKUP, OP_ACCESS, niopts, UIO_SYSSPACE,
			    CAST_USER_ADDR_T(((const char *)input) + input[i].ad_name_offset),
			    &context);
			error = namei(&nd);
			if (!error) {
				vp = nd.ni_vp;
				if (wantdelete) {
					dvp = nd.ni_dvp;
				}
			}
			nameidone(&nd);
		}

		  
                          
     
		switch (error) {
		case ENOENT:
		case EACCES:
		case EPERM:
		case ENOTDIR:
			result[i] = error;
			break;
		case 0:
			                           
			result[i] = access1(vp, dvp, input[i].ad_flags, &context);
			break;
		default:
			                        

			goto out;
		}
	}

	AUDIT_ARG(data, result, sizeof(errno_t), desc_actual);

	                      
	error = copyout(result, uap->results, desc_actual * sizeof(errno_t));

out:
	if (input && input != stack_input) {
		kheap_free(KHEAP_DATA_BUFFERS, input, uap->size);
	}
	if (result) {
		kheap_free(KHEAP_DATA_BUFFERS, result, desc_actual * sizeof(errno_t));
	}
	if (vp) {
		vnode_put(vp);
	}
	if (dvp) {
		vnode_put(dvp);
	}
	if (IS_VALID_CRED(context.vc_ucred)) {
		kauth_cred_unref(&context.vc_ucred);
	}
	return error;
}


  
                       
                             
                                        
                                           
                                                  
                                    
                                  
             
            
   
static int
faccessat_internal(vfs_context_t ctx, int fd, user_addr_t path, int amode,
    int flag, enum uio_seg segflg)
{
	int error;
	struct nameidata nd;
	int niopts;
	struct vfs_context context;
#if NAMEDRSRCFORK
	int is_namedstream = 0;
#endif

	  
                                                                       
                                                                       
                                                               
                                 
    
	if (!(flag & AT_EACCESS)) {
		context.vc_ucred = kauth_cred_copy_real(kauth_cred_get());
	} else {
		context.vc_ucred = ctx->vc_ucred;
	}
	context.vc_thread = ctx->vc_thread;


	niopts = (flag & AT_SYMLINK_NOFOLLOW ? NOFOLLOW : FOLLOW) | AUDITVNPATH1;
	                                                       
	if (amode & _DELETE_OK) {
		niopts |= WANTPARENT;
	}
	NDINIT(&nd, LOOKUP, OP_ACCESS, niopts, segflg,
	    path, &context);

#if NAMEDRSRCFORK
	                                                        
	if (amode == F_OK) {
		nd.ni_cnd.cn_flags |= CN_ALLOWRSRCFORK;
	}
#endif
	error = nameiat(&nd, fd);
	if (error) {
		goto out;
	}

#if NAMEDRSRCFORK
	                                                    
                                                   
                
    
	if (vnode_isnamedstream(nd.ni_vp) &&
	    (nd.ni_vp->v_parent != NULLVP) &&
	    vnode_isshadow(nd.ni_vp)) {
		is_namedstream = 1;
		vnode_ref(nd.ni_vp);
	}
#endif

	error = access1(nd.ni_vp, nd.ni_dvp, amode, &context);

#if NAMEDRSRCFORK
	if (is_namedstream) {
		vnode_rele(nd.ni_vp);
	}
#endif

	vnode_put(nd.ni_vp);
	if (amode & _DELETE_OK) {
		vnode_put(nd.ni_dvp);
	}
	nameidone(&nd);

out:
	if (!(flag & AT_EACCESS)) {
		kauth_cred_unref(&context.vc_ucred);
	}
	return error;
}

int
access(__unused proc_t p, struct access_args *uap, __unused int32_t *retval)
{
	return faccessat_internal(vfs_context_current(), AT_FDCWD,
	           uap->path, uap->flags, 0, UIO_USERSPACE);
}

int
faccessat(__unused proc_t p, struct faccessat_args *uap,
    __unused int32_t *retval)
{
	if (uap->flag & ~(AT_EACCESS | AT_SYMLINK_NOFOLLOW)) {
		return EINVAL;
	}

	return faccessat_internal(vfs_context_current(), uap->fd,
	           uap->path, uap->amode, uap->flag, UIO_USERSPACE);
}

  
                       
          
                 
            
              
   
static int
fstatat_internal(vfs_context_t ctx, user_addr_t path, user_addr_t ub,
    user_addr_t xsecurity, user_addr_t xsecurity_size, int isstat64,
    enum uio_seg segflg, int fd, int flag)
{
	struct nameidata nd;
	int follow;
	union {
		struct stat sb;
		struct stat64 sb64;
	} source = {};
	union {
		struct user64_stat user64_sb;
		struct user32_stat user32_sb;
		struct user64_stat64 user64_sb64;
		struct user32_stat64 user32_sb64;
	} dest = {};
	caddr_t sbp;
	int error, my_size;
	kauth_filesec_t fsec;
	size_t xsecurity_bufsize;
	void * statptr;
	struct fileproc *fp = NULL;
	int needsrealdev = 0;

	follow = (flag & AT_SYMLINK_NOFOLLOW) ? NOFOLLOW : FOLLOW;
	NDINIT(&nd, LOOKUP, OP_GETATTR, follow | AUDITVNPATH1,
	    segflg, path, ctx);

#if NAMEDRSRCFORK
	int is_namedstream = 0;
	                                                
	nd.ni_cnd.cn_flags |= CN_ALLOWRSRCFORK;
#endif

	if (flag & AT_FDONLY) {
		vnode_t fvp;

		error = fp_getfvp(vfs_context_proc(ctx), fd, &fp, &fvp);
		if (error) {
			return error;
		}
		if ((error = vnode_getwithref(fvp))) {
			file_drop(fd);
			return error;
		}
		nd.ni_vp = fvp;
	} else {
		error = nameiat(&nd, fd);
		if (error) {
			return error;
		}
	}
	fsec = KAUTH_FILESEC_NONE;

	statptr = (void *)&source;

#if NAMEDRSRCFORK
	                                                    
                                                   
                
    
	if (vnode_isnamedstream(nd.ni_vp) &&
	    (nd.ni_vp->v_parent != NULLVP) &&
	    vnode_isshadow(nd.ni_vp)) {
		is_namedstream = 1;
		vnode_ref(nd.ni_vp);
	}
#endif

	needsrealdev = flag & AT_REALDEV ? 1 : 0;
	if (fp && (xsecurity == USER_ADDR_NULL)) {
		  
                                                
                                                     
                                                      
     
		error = vn_stat_noauth(nd.ni_vp, statptr, NULL, isstat64, needsrealdev, ctx,
		    fp->fp_glob->fg_cred);
	} else {
		error = vn_stat(nd.ni_vp, statptr, (xsecurity != USER_ADDR_NULL ? &fsec : NULL),
		    isstat64, needsrealdev, ctx);
	}

#if NAMEDRSRCFORK
	if (is_namedstream) {
		vnode_rele(nd.ni_vp);
	}
#endif
	vnode_put(nd.ni_vp);
	nameidone(&nd);
	if (fp) {
		file_drop(fd);
		fp = NULL;
	}

	if (error) {
		return error;
	}
	                      
	if (isstat64 != 0) {
		source.sb64.st_lspare = 0;
		source.sb64.st_qspare[0] = 0LL;
		source.sb64.st_qspare[1] = 0LL;
		if (IS_64BIT_PROCESS(vfs_context_proc(ctx))) {
			munge_user64_stat64(&source.sb64, &dest.user64_sb64);
			my_size = sizeof(dest.user64_sb64);
			sbp = (caddr_t)&dest.user64_sb64;
		} else {
			munge_user32_stat64(&source.sb64, &dest.user32_sb64);
			my_size = sizeof(dest.user32_sb64);
			sbp = (caddr_t)&dest.user32_sb64;
		}
		  
                                                                       
     
		if ((source.sb64.st_nlink == 0) && S_ISREG(source.sb64.st_mode)) {
			source.sb64.st_nlink = 1;
		}
	} else {
		source.sb.st_lspare = 0;
		source.sb.st_qspare[0] = 0LL;
		source.sb.st_qspare[1] = 0LL;
		if (IS_64BIT_PROCESS(vfs_context_proc(ctx))) {
			munge_user64_stat(&source.sb, &dest.user64_sb);
			my_size = sizeof(dest.user64_sb);
			sbp = (caddr_t)&dest.user64_sb;
		} else {
			munge_user32_stat(&source.sb, &dest.user32_sb);
			my_size = sizeof(dest.user32_sb);
			sbp = (caddr_t)&dest.user32_sb;
		}

		  
                                                                       
     
		if ((source.sb.st_nlink == 0) && S_ISREG(source.sb.st_mode)) {
			source.sb.st_nlink = 1;
		}
	}
	if ((error = copyout(sbp, ub, my_size)) != 0) {
		goto out;
	}

	                                                 
	if (xsecurity != USER_ADDR_NULL) {
		                     
		if (fsec == KAUTH_FILESEC_NONE) {
			if (susize(xsecurity_size, 0) != 0) {
				error = EFAULT;
				goto out;
			}
		} else {
			                               
			xsecurity_bufsize = fusize(xsecurity_size);

			                                   
			if (susize(xsecurity_size, KAUTH_FILESEC_COPYSIZE(fsec)) != 0) {
				error = EFAULT;
				goto out;
			}

			                                                        
			if (xsecurity_bufsize >= KAUTH_FILESEC_COPYSIZE(fsec)) {
				error = copyout(fsec, xsecurity, KAUTH_FILESEC_COPYSIZE(fsec));
			}
		}
	}
out:
	if (fsec != KAUTH_FILESEC_NONE) {
		kauth_filesec_free(fsec);
	}
	return error;
}

  
                                                                
  
                                                   
                                                                              
                                                   
  
                                                                         
                                                                              
                                                                        
                                                     
  
                                                 
                                                     
  
   
int
stat_extended(__unused proc_t p, struct stat_extended_args *uap,
    __unused int32_t *retval)
{
	return fstatat_internal(vfs_context_current(), uap->path, uap->ub,
	           uap->xsecurity, uap->xsecurity_size, 0, UIO_USERSPACE, AT_FDCWD,
	           0);
}

  
                       
                                                              
   
int
stat(__unused proc_t p, struct stat_args *uap, __unused int32_t *retval)
{
	return fstatat_internal(vfs_context_current(), uap->path, uap->ub,
	           0, 0, 0, UIO_USERSPACE, AT_FDCWD, 0);
}

int
stat64(__unused proc_t p, struct stat64_args *uap, __unused int32_t *retval)
{
	return fstatat_internal(vfs_context_current(), uap->path, uap->ub,
	           0, 0, 1, UIO_USERSPACE, AT_FDCWD, 0);
}

  
                                                                                                  
  
                                                   
                                                                              
                                                   
  
                                                                         
                                                                              
                                                                        
                                                     
  
                                                 
                                                     
  
   
int
stat64_extended(__unused proc_t p, struct stat64_extended_args *uap, __unused int32_t *retval)
{
	return fstatat_internal(vfs_context_current(), uap->path, uap->ub,
	           uap->xsecurity, uap->xsecurity_size, 1, UIO_USERSPACE, AT_FDCWD,
	           0);
}

  
                                                                                        
  
                                                   
                                                                              
                                                   
  
                                                                         
                                                                              
                                                                        
                                                     
  
                                                 
                                                     
  
   
int
lstat_extended(__unused proc_t p, struct lstat_extended_args *uap, __unused int32_t *retval)
{
	return fstatat_internal(vfs_context_current(), uap->path, uap->ub,
	           uap->xsecurity, uap->xsecurity_size, 0, UIO_USERSPACE, AT_FDCWD,
	           AT_SYMLINK_NOFOLLOW);
}

  
                                                       
   
int
lstat(__unused proc_t p, struct lstat_args *uap, __unused int32_t *retval)
{
	return fstatat_internal(vfs_context_current(), uap->path, uap->ub,
	           0, 0, 0, UIO_USERSPACE, AT_FDCWD, AT_SYMLINK_NOFOLLOW);
}

int
lstat64(__unused proc_t p, struct lstat64_args *uap, __unused int32_t *retval)
{
	return fstatat_internal(vfs_context_current(), uap->path, uap->ub,
	           0, 0, 1, UIO_USERSPACE, AT_FDCWD, AT_SYMLINK_NOFOLLOW);
}

  
                                                                              
                                              
  
                                                   
                                                                              
                                                   
  
                                                                         
                                                                              
                                                                        
                                                     
  
                                                 
                                                     
  
   
int
lstat64_extended(__unused proc_t p, struct lstat64_extended_args *uap, __unused int32_t *retval)
{
	return fstatat_internal(vfs_context_current(), uap->path, uap->ub,
	           uap->xsecurity, uap->xsecurity_size, 1, UIO_USERSPACE, AT_FDCWD,
	           AT_SYMLINK_NOFOLLOW);
}

int
fstatat(__unused proc_t p, struct fstatat_args *uap, __unused int32_t *retval)
{
	if (uap->flag & ~(AT_SYMLINK_NOFOLLOW | AT_REALDEV | AT_FDONLY)) {
		return EINVAL;
	}

	return fstatat_internal(vfs_context_current(), uap->path, uap->ub,
	           0, 0, 0, UIO_USERSPACE, uap->fd, uap->flag);
}

int
fstatat64(__unused proc_t p, struct fstatat64_args *uap,
    __unused int32_t *retval)
{
	if (uap->flag & ~(AT_SYMLINK_NOFOLLOW | AT_REALDEV | AT_FDONLY)) {
		return EINVAL;
	}

	return fstatat_internal(vfs_context_current(), uap->path, uap->ub,
	           0, 0, 1, UIO_USERSPACE, uap->fd, uap->flag);
}

  
                                       
  
                       
            
                  
  
                                                             
                                                              
                                                               
                                
  
                                                                          
                                                            
   
              
int
pathconf(__unused proc_t p, struct pathconf_args *uap, int32_t *retval)
{
	int error;
	struct nameidata nd;
	vfs_context_t ctx = vfs_context_current();

	NDINIT(&nd, LOOKUP, OP_PATHCONF, FOLLOW | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);
	error = namei(&nd);
	if (error) {
		return error;
	}

	error = vn_pathconf(nd.ni_vp, uap->name, retval, ctx);

	vnode_put(nd.ni_vp);
	nameidone(&nd);
	return error;
}

  
                                         
   
              
static int
readlinkat_internal(vfs_context_t ctx, int fd, user_addr_t path,
    enum uio_seg seg, user_addr_t buf, size_t bufsize, enum uio_seg bufseg,
    int *retval)
{
	vnode_t vp;
	uio_t auio;
	int error;
	struct nameidata nd;
	char uio_buf[UIO_SIZEOF(1)];

	if (bufsize > INT32_MAX) {
		return EINVAL;
	}

	NDINIT(&nd, LOOKUP, OP_READLINK, NOFOLLOW | AUDITVNPATH1,
	    seg, path, ctx);

	error = nameiat(&nd, fd);
	if (error) {
		return error;
	}
	vp = nd.ni_vp;

	nameidone(&nd);

	auio = uio_createwithbuffer(1, 0, bufseg, UIO_READ,
	    &uio_buf[0], sizeof(uio_buf));
	uio_addiov(auio, buf, bufsize);
	if (vp->v_type != VLNK) {
		error = EINVAL;
	} else {
#if CONFIG_MACF
		error = mac_vnode_check_readlink(ctx, vp);
#endif
		if (error == 0) {
			error = vnode_authorize(vp, NULL, KAUTH_VNODE_READ_DATA,
			    ctx);
		}
		if (error == 0) {
			error = VNOP_READLINK(vp, auio, ctx);
		}
	}
	vnode_put(vp);

	*retval = (int)(bufsize - uio_resid(auio));
	return error;
}

int
readlink(proc_t p, struct readlink_args *uap, int32_t *retval)
{
	enum uio_seg procseg;

	procseg = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	return readlinkat_internal(vfs_context_current(), AT_FDCWD,
	           CAST_USER_ADDR_T(uap->path), procseg, CAST_USER_ADDR_T(uap->buf),
	           uap->count, procseg, retval);
}

int
readlinkat(proc_t p, struct readlinkat_args *uap, int32_t *retval)
{
	enum uio_seg procseg;

	procseg = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	return readlinkat_internal(vfs_context_current(), uap->fd, uap->path,
	           procseg, uap->buf, uap->bufsize, procseg, retval);
}

  
                                           
   
static int
chflags0(vnode_t vp, struct vnode_attr *va,
    int (*setattr)(vnode_t, void *, vfs_context_t),
    void *arg, vfs_context_t ctx)
{
	kauth_action_t action = 0;
	int error;

#if CONFIG_MACF
	error = mac_vnode_check_setflags(ctx, vp, va->va_flags);
	if (error) {
		goto out;
	}
#endif

	                                                   
	if ((error = vnode_authattr(vp, va, &action, ctx)) != 0) {
		goto out;
	}
	  
                                                                               
                                                                         
                          
    
	if (action && ((error = vnode_authorize(vp, NULL, action | KAUTH_VNODE_NOIMMUTABLE, ctx)) != 0)) {
		goto out;
	}
	error = (*setattr)(vp, arg, ctx);

#if CONFIG_MACF
	if (error == 0) {
		mac_vnode_notify_setflags(ctx, vp, va->va_flags);
	}
#endif

out:
	return error;
}

  
                     
  
                                   
   
static int
chflags1(vnode_t vp, int flags, vfs_context_t ctx)
{
	struct vnode_attr va;
	int error;

	VATTR_INIT(&va);
	VATTR_SET(&va, va_flags, flags);

	error = chflags0(vp, &va, (void *)vnode_setattr, &va, ctx);
	vnode_put(vp);

	if ((error == 0) && !VATTR_IS_SUPPORTED(&va, va_flags)) {
		error = ENOTSUP;
	}

	return error;
}

  
                                            
   
              
int
chflags(__unused proc_t p, struct chflags_args *uap, __unused int32_t *retval)
{
	vnode_t vp;
	vfs_context_t ctx = vfs_context_current();
	int error;
	struct nameidata nd;

	AUDIT_ARG(fflags, uap->flags);
	NDINIT(&nd, LOOKUP, OP_SETATTR, FOLLOW | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);
	error = namei(&nd);
	if (error) {
		return error;
	}
	vp = nd.ni_vp;
	nameidone(&nd);

	                                                                
	error = chflags1(vp, uap->flags, ctx);

	return error;
}

  
                                                  
   
              
int
fchflags(__unused proc_t p, struct fchflags_args *uap, __unused int32_t *retval)
{
	vnode_t vp;
	int error;

	AUDIT_ARG(fd, uap->fd);
	AUDIT_ARG(fflags, uap->flags);
	if ((error = file_vnode(uap->fd, &vp))) {
		return error;
	}

	if ((error = vnode_getwithref(vp))) {
		file_drop(uap->fd);
		return error;
	}

	AUDIT_ARG(vnpath, vp, ARG_VNODE1);

	                                                                
	error = chflags1(vp, uap->flags, vfs_context_current());

	file_drop(uap->fd);
	return error;
}

  
                                                      
  
                       
                                   
                                                           
                                                             
                                                         
  
                                                                        
                                              
   
static int
chmod_vnode(vfs_context_t ctx, vnode_t vp, struct vnode_attr *vap)
{
	kauth_action_t action;
	int error;

	AUDIT_ARG(mode, vap->va_mode);
	                        

#if NAMEDSTREAMS
	                                                     
	if (vp->v_flag & VISNAMEDSTREAM) {
		return EPERM;
	}
#endif

#if CONFIG_MACF
	if (VATTR_IS_ACTIVE(vap, va_mode) &&
	    (error = mac_vnode_check_setmode(ctx, vp, (mode_t)vap->va_mode)) != 0) {
		return error;
	}

	if (VATTR_IS_ACTIVE(vap, va_uid) || VATTR_IS_ACTIVE(vap, va_gid)) {
		if ((error = mac_vnode_check_setowner(ctx, vp,
		    VATTR_IS_ACTIVE(vap, va_uid) ? vap->va_uid : -1,
		    VATTR_IS_ACTIVE(vap, va_gid) ? vap->va_gid : -1))) {
			return error;
		}
	}

	if (VATTR_IS_ACTIVE(vap, va_acl) &&
	    (error = mac_vnode_check_setacl(ctx, vp, vap->va_acl))) {
		return error;
	}
#endif

	                                                                           
	if (((error = vnode_authattr(vp, vap, &action, ctx)) != 0) ||
	    ((error = vnode_authorize(vp, NULL, action, ctx)) != 0)) {
		if (error == EACCES) {
			error = EPERM;
		}
		return error;
	}

	if ((error = vnode_setattr(vp, vap, ctx)) != 0) {
		return error;
	}

#if CONFIG_MACF
	if (VATTR_IS_ACTIVE(vap, va_mode)) {
		mac_vnode_notify_setmode(ctx, vp, (mode_t)vap->va_mode);
	}

	if (VATTR_IS_ACTIVE(vap, va_uid) || VATTR_IS_ACTIVE(vap, va_gid)) {
		mac_vnode_notify_setowner(ctx, vp,
		    VATTR_IS_ACTIVE(vap, va_uid) ? vap->va_uid : -1,
		    VATTR_IS_ACTIVE(vap, va_gid) ? vap->va_gid : -1);
	}

	if (VATTR_IS_ACTIVE(vap, va_acl)) {
		mac_vnode_notify_setacl(ctx, vp, vap->va_acl);
	}
#endif

	return error;
}


  
                                           
  
                       
                                          
                                                      
   
static int
chmodat(vfs_context_t ctx, user_addr_t path, struct vnode_attr *vap,
    int fd, int flag, enum uio_seg segflg)
{
	struct nameidata nd;
	int follow, error;

	follow = (flag & AT_SYMLINK_NOFOLLOW) ? NOFOLLOW : FOLLOW;
	NDINIT(&nd, LOOKUP, OP_SETATTR, follow | AUDITVNPATH1,
	    segflg, path, ctx);
	if ((error = nameiat(&nd, fd))) {
		return error;
	}
	error = chmod_vnode(ctx, nd.ni_vp, vap);
	vnode_put(nd.ni_vp);
	nameidone(&nd);
	return error;
}

  
                                                                             
                                                     
  
                                              
                                              
                      
  
                                                        
                        
                        
                                                 
                                          
  
                       
                    
  
                                                                     
  
                                                                       
                                
   
int
chmod_extended(__unused proc_t p, struct chmod_extended_args *uap, __unused int32_t *retval)
{
	int error;
	struct vnode_attr va;
	kauth_filesec_t xsecdst;

	AUDIT_ARG(owner, uap->uid, uap->gid);

	VATTR_INIT(&va);
	if (uap->mode != -1) {
		VATTR_SET(&va, va_mode, uap->mode & ALLPERMS);
	}
	if (uap->uid != KAUTH_UID_NONE) {
		VATTR_SET(&va, va_uid, uap->uid);
	}
	if (uap->gid != KAUTH_GID_NONE) {
		VATTR_SET(&va, va_gid, uap->gid);
	}

	xsecdst = NULL;
	switch (uap->xsecurity) {
	                             
	case CAST_USER_ADDR_T((void *)1):                                
		VATTR_SET(&va, va_acl, NULL);
		break;
	                   
	case USER_ADDR_NULL:
		break;
	default:
		if ((error = kauth_copyinfilesec(uap->xsecurity, &xsecdst)) != 0) {
			return error;
		}
		VATTR_SET(&va, va_acl, &xsecdst->fsec_acl);
		KAUTH_DEBUG("CHMOD - setting ACL with %d entries", va.va_acl->acl_entrycount);
	}

	error = chmodat(vfs_context_current(), uap->path, &va, AT_FDCWD, 0,
	    UIO_USERSPACE);

	if (xsecdst != NULL) {
		kauth_filesec_free(xsecdst);
	}
	return error;
}

  
                       
                                              
   
static int
fchmodat_internal(vfs_context_t ctx, user_addr_t path, int mode, int fd,
    int flag, enum uio_seg segflg)
{
	struct vnode_attr va;

	VATTR_INIT(&va);
	VATTR_SET(&va, va_mode, mode & ALLPERMS);

	return chmodat(ctx, path, &va, fd, flag, segflg);
}

int
chmod(__unused proc_t p, struct chmod_args *uap, __unused int32_t *retval)
{
	return fchmodat_internal(vfs_context_current(), uap->path, uap->mode,
	           AT_FDCWD, 0, UIO_USERSPACE);
}

int
fchmodat(__unused proc_t p, struct fchmodat_args *uap, __unused int32_t *retval)
{
	if (uap->flag & ~AT_SYMLINK_NOFOLLOW) {
		return EINVAL;
	}

	return fchmodat_internal(vfs_context_current(), uap->path, uap->mode,
	           uap->fd, uap->flag, UIO_USERSPACE);
}

  
                                                 
   
static int
fchmod1(__unused proc_t p, int fd, struct vnode_attr *vap)
{
	vnode_t vp;
	int error;

	AUDIT_ARG(fd, fd);

	if ((error = file_vnode(fd, &vp)) != 0) {
		return error;
	}
	if ((error = vnode_getwithref(vp)) != 0) {
		file_drop(fd);
		return error;
	}
	AUDIT_ARG(vnpath, vp, ARG_VNODE1);

	error = chmod_vnode(vfs_context_current(), vp, vap);
	(void)vnode_put(vp);
	file_drop(fd);

	return error;
}

  
                                                                       
                                                              
  
                                                                                
                                                                              
                                                   
  
                                                                            
                                                    
                                                    
                                                                
                                                                                
  
                                                 
                                                     
  
   
int
fchmod_extended(proc_t p, struct fchmod_extended_args *uap, __unused int32_t *retval)
{
	int error;
	struct vnode_attr va;
	kauth_filesec_t xsecdst;

	AUDIT_ARG(owner, uap->uid, uap->gid);

	VATTR_INIT(&va);
	if (uap->mode != -1) {
		VATTR_SET(&va, va_mode, uap->mode & ALLPERMS);
	}
	if (uap->uid != KAUTH_UID_NONE) {
		VATTR_SET(&va, va_uid, uap->uid);
	}
	if (uap->gid != KAUTH_GID_NONE) {
		VATTR_SET(&va, va_gid, uap->gid);
	}

	xsecdst = NULL;
	switch (uap->xsecurity) {
	case USER_ADDR_NULL:
		VATTR_SET(&va, va_acl, NULL);
		break;
	case CAST_USER_ADDR_T((void *)1):                                
		VATTR_SET(&va, va_acl, NULL);
		break;
	                   
	case CAST_USER_ADDR_T(-1):
		break;
	default:
		if ((error = kauth_copyinfilesec(uap->xsecurity, &xsecdst)) != 0) {
			return error;
		}
		VATTR_SET(&va, va_acl, &xsecdst->fsec_acl);
	}

	error = fchmod1(p, uap->fd, &va);


	switch (uap->xsecurity) {
	case USER_ADDR_NULL:
	case CAST_USER_ADDR_T(-1):
		break;
	default:
		if (xsecdst != NULL) {
			kauth_filesec_free(xsecdst);
		}
	}
	return error;
}

int
fchmod(proc_t p, struct fchmod_args *uap, __unused int32_t *retval)
{
	struct vnode_attr va;

	VATTR_INIT(&va);
	VATTR_SET(&va, va_mode, uap->mode & ALLPERMS);

	return fchmod1(p, uap->fd, &va);
}


  
                                   
   
              
static int
fchownat_internal(vfs_context_t ctx, int fd, user_addr_t path, uid_t uid,
    gid_t gid, int flag, enum uio_seg segflg)
{
	vnode_t vp;
	struct vnode_attr va;
	int error;
	struct nameidata nd;
	int follow;
	kauth_action_t action;

	AUDIT_ARG(owner, uid, gid);

	follow = (flag & AT_SYMLINK_NOFOLLOW) ? NOFOLLOW : FOLLOW;
	NDINIT(&nd, LOOKUP, OP_SETATTR, follow | AUDITVNPATH1, segflg,
	    path, ctx);
	error = nameiat(&nd, fd);
	if (error) {
		return error;
	}
	vp = nd.ni_vp;

	nameidone(&nd);

	VATTR_INIT(&va);
	if (uid != (uid_t)VNOVAL) {
		VATTR_SET(&va, va_uid, uid);
	}
	if (gid != (gid_t)VNOVAL) {
		VATTR_SET(&va, va_gid, gid);
	}

#if CONFIG_MACF
	error = mac_vnode_check_setowner(ctx, vp, uid, gid);
	if (error) {
		goto out;
	}
#endif

	                                               
	if ((error = vnode_authattr(vp, &va, &action, ctx)) != 0) {
		goto out;
	}
	if (action && ((error = vnode_authorize(vp, NULL, action, ctx)) != 0)) {
		goto out;
	}
	error = vnode_setattr(vp, &va, ctx);

#if CONFIG_MACF
	if (error == 0) {
		mac_vnode_notify_setowner(ctx, vp, uid, gid);
	}
#endif

out:
	  
                                                                   
                                                         
    
	if (error == EACCES) {
		error = EPERM;
	}

	vnode_put(vp);
	return error;
}

int
chown(__unused proc_t p, struct chown_args *uap, __unused int32_t *retval)
{
	return fchownat_internal(vfs_context_current(), AT_FDCWD, uap->path,
	           uap->uid, uap->gid, 0, UIO_USERSPACE);
}

int
lchown(__unused proc_t p, struct lchown_args *uap, __unused int32_t *retval)
{
	return fchownat_internal(vfs_context_current(), AT_FDCWD, uap->path,
	           uap->owner, uap->group, AT_SYMLINK_NOFOLLOW, UIO_USERSPACE);
}

int
fchownat(__unused proc_t p, struct fchownat_args *uap, __unused int32_t *retval)
{
	if (uap->flag & ~AT_SYMLINK_NOFOLLOW) {
		return EINVAL;
	}

	return fchownat_internal(vfs_context_current(), uap->fd, uap->path,
	           uap->uid, uap->gid, uap->flag, UIO_USERSPACE);
}

  
                                         
   
              
int
fchown(__unused proc_t p, struct fchown_args *uap, __unused int32_t *retval)
{
	struct vnode_attr va;
	vfs_context_t ctx = vfs_context_current();
	vnode_t vp;
	int error;
	kauth_action_t action;

	AUDIT_ARG(owner, uap->uid, uap->gid);
	AUDIT_ARG(fd, uap->fd);

	if ((error = file_vnode(uap->fd, &vp))) {
		return error;
	}

	if ((error = vnode_getwithref(vp))) {
		file_drop(uap->fd);
		return error;
	}
	AUDIT_ARG(vnpath, vp, ARG_VNODE1);

	VATTR_INIT(&va);
	if (uap->uid != VNOVAL) {
		VATTR_SET(&va, va_uid, uap->uid);
	}
	if (uap->gid != VNOVAL) {
		VATTR_SET(&va, va_gid, uap->gid);
	}

#if NAMEDSTREAMS
	                                                     
	if (vp->v_flag & VISNAMEDSTREAM) {
		error = EPERM;
		goto out;
	}
#endif

#if CONFIG_MACF
	error = mac_vnode_check_setowner(ctx, vp, uap->uid, uap->gid);
	if (error) {
		goto out;
	}
#endif

	                                               
	if ((error = vnode_authattr(vp, &va, &action, ctx)) != 0) {
		goto out;
	}
	if (action && ((error = vnode_authorize(vp, NULL, action, ctx)) != 0)) {
		if (error == EACCES) {
			error = EPERM;
		}
		goto out;
	}
	error = vnode_setattr(vp, &va, ctx);

#if CONFIG_MACF
	if (error == 0) {
		mac_vnode_notify_setowner(ctx, vp, uap->uid, uap->gid);
	}
#endif

out:
	(void)vnode_put(vp);
	file_drop(uap->fd);
	return error;
}

static int
getutimes(user_addr_t usrtvp, struct timespec *tsp)
{
	int error;

	if (usrtvp == USER_ADDR_NULL) {
		struct timeval old_tv;
		                                                 
		microtime(&old_tv);
		TIMEVAL_TO_TIMESPEC(&old_tv, &tsp[0]);
		tsp[1] = tsp[0];
	} else {
		if (IS_64BIT_PROCESS(current_proc())) {
			struct user64_timeval tv[2];
			error = copyin(usrtvp, (void *)tv, sizeof(tv));
			if (error) {
				return error;
			}
			TIMEVAL64_TO_TIMESPEC(&tv[0], &tsp[0]);
			TIMEVAL64_TO_TIMESPEC(&tv[1], &tsp[1]);
		} else {
			struct user32_timeval tv[2];
			error = copyin(usrtvp, (void *)tv, sizeof(tv));
			if (error) {
				return error;
			}
			TIMEVAL_TO_TIMESPEC(&tv[0], &tsp[0]);
			TIMEVAL_TO_TIMESPEC(&tv[1], &tsp[1]);
		}
	}
	return 0;
}

static int
setutimes(vfs_context_t ctx, vnode_t vp, const struct timespec *ts,
    int nullflag)
{
	int error;
	struct vnode_attr va;
	kauth_action_t action;

	AUDIT_ARG(vnpath, vp, ARG_VNODE1);

	VATTR_INIT(&va);
	VATTR_SET(&va, va_access_time, ts[0]);
	VATTR_SET(&va, va_modify_time, ts[1]);
	if (nullflag) {
		va.va_vaflags |= VA_UTIMES_NULL;
	}

#if NAMEDSTREAMS
	                                                      
	if (vp->v_flag & VISNAMEDSTREAM) {
		error = EPERM;
		goto out;
	}
#endif

#if CONFIG_MACF
	error = mac_vnode_check_setutimes(ctx, vp, ts[0], ts[1]);
	if (error) {
		goto out;
	}
#endif
	if ((error = vnode_authattr(vp, &va, &action, ctx)) != 0) {
		if (!nullflag && error == EACCES) {
			error = EPERM;
		}
		goto out;
	}

	                                                        
	if ((action != 0) && ((error = vnode_authorize(vp, NULL, action, ctx)) != 0)) {
		if (!nullflag && error == EACCES) {
			error = EPERM;
		}
		goto out;
	}
	error = vnode_setattr(vp, &va, ctx);

#if CONFIG_MACF
	if (error == 0) {
		mac_vnode_notify_setutimes(ctx, vp, ts[0], ts[1]);
	}
#endif

out:
	return error;
}

  
                                                   
   
              
int
utimes(__unused proc_t p, struct utimes_args *uap, __unused int32_t *retval)
{
	struct timespec ts[2];
	user_addr_t usrtvp;
	int error;
	struct nameidata nd;
	vfs_context_t ctx = vfs_context_current();

	  
                                                             
                                                      
    
	NDINIT(&nd, LOOKUP, OP_SETATTR, FOLLOW | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);
	error = namei(&nd);
	if (error) {
		return error;
	}
	nameidone(&nd);

	  
                                                                        
                             
    
	usrtvp = uap->tptr;
	if ((error = getutimes(usrtvp, ts)) != 0) {
		goto out;
	}

	error = setutimes(ctx, nd.ni_vp, ts, usrtvp == USER_ADDR_NULL);

out:
	vnode_put(nd.ni_vp);
	return error;
}

  
                                                   
   
              
int
futimes(__unused proc_t p, struct futimes_args *uap, __unused int32_t *retval)
{
	struct timespec ts[2];
	vnode_t vp;
	user_addr_t usrtvp;
	int error;

	AUDIT_ARG(fd, uap->fd);
	usrtvp = uap->tptr;
	if ((error = getutimes(usrtvp, ts)) != 0) {
		return error;
	}
	if ((error = file_vnode(uap->fd, &vp)) != 0) {
		return error;
	}
	if ((error = vnode_getwithref(vp))) {
		file_drop(uap->fd);
		return error;
	}

	error =  setutimes(vfs_context_current(), vp, ts, usrtvp == 0);
	vnode_put(vp);
	file_drop(uap->fd);
	return error;
}

  
                                       
   
              
int
truncate(proc_t p, struct truncate_args *uap, __unused int32_t *retval)
{
	vnode_t vp;
	struct vnode_attr va;
	vfs_context_t ctx = vfs_context_current();
	int error;
	struct nameidata nd;
	kauth_action_t action;
	rlim_t fsize_limit;

	if (uap->length < 0) {
		return EINVAL;
	}

	fsize_limit = proc_limitgetcur(p, RLIMIT_FSIZE, TRUE);
	if ((rlim_t)uap->length > fsize_limit) {
		psignal(p, SIGXFSZ);
		return EFBIG;
	}

	NDINIT(&nd, LOOKUP, OP_TRUNCATE, FOLLOW | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);
	if ((error = namei(&nd))) {
		return error;
	}
	vp = nd.ni_vp;

	nameidone(&nd);

	VATTR_INIT(&va);
	VATTR_SET(&va, va_data_size, uap->length);

#if CONFIG_MACF
	error = mac_vnode_check_truncate(ctx, NOCRED, vp);
	if (error) {
		goto out;
	}
#endif

	if ((error = vnode_authattr(vp, &va, &action, ctx)) != 0) {
		goto out;
	}
	if ((action != 0) && ((error = vnode_authorize(vp, NULL, action, ctx)) != 0)) {
		goto out;
	}
	error = vnode_setattr(vp, &va, ctx);

#if CONFIG_MACF
	if (error == 0) {
		mac_vnode_notify_truncate(ctx, NOCRED, vp);
	}
#endif

out:
	vnode_put(vp);
	return error;
}

  
                                           
   
              
int
ftruncate(proc_t p, struct ftruncate_args *uap, int32_t *retval)
{
	vfs_context_t ctx = vfs_context_current();
	struct vnode_attr va;
	vnode_t vp;
	struct fileproc *fp;
	int error;
	int fd = uap->fd;
	rlim_t fsize_limit;

	AUDIT_ARG(fd, uap->fd);
	if (uap->length < 0) {
		return EINVAL;
	}

	fsize_limit = proc_limitgetcur(p, RLIMIT_FSIZE, TRUE);
	if ((rlim_t)uap->length > fsize_limit) {
		psignal(p, SIGXFSZ);
		return EFBIG;
	}

	if ((error = fp_lookup(p, fd, &fp, 0))) {
		return error;
	}

	switch (FILEGLOB_DTYPE(fp->fp_glob)) {
	case DTYPE_PSXSHM:
		error = pshm_truncate(p, fp, uap->fd, uap->length, retval);
		goto out;
	case DTYPE_VNODE:
		break;
	default:
		error = EINVAL;
		goto out;
	}

	vp = (vnode_t)fp->fp_glob->fg_data;

	if ((fp->fp_glob->fg_flag & FWRITE) == 0) {
		AUDIT_ARG(vnpath_withref, vp, ARG_VNODE1);
		error = EINVAL;
		goto out;
	}

	if ((error = vnode_getwithref(vp)) != 0) {
		goto out;
	}

	AUDIT_ARG(vnpath, vp, ARG_VNODE1);

#if CONFIG_MACF
	error = mac_vnode_check_truncate(ctx,
	    fp->fp_glob->fg_cred, vp);
	if (error) {
		(void)vnode_put(vp);
		goto out;
	}
#endif
	VATTR_INIT(&va);
	VATTR_SET(&va, va_data_size, uap->length);
	error = vnode_setattr(vp, &va, ctx);

#if CONFIG_MACF
	if (error == 0) {
		mac_vnode_notify_truncate(ctx, fp->fp_glob->fg_cred, vp);
	}
#endif

	(void)vnode_put(vp);
out:
	file_drop(fd);
	return error;
}


  
                                                                      
   
              
int
fsync(proc_t p, struct fsync_args *uap, __unused int32_t *retval)
{
	__pthread_testcancel(1);
	return fsync_common(p, uap, MNT_WAIT);
}


  
                                                                      
  
                                                                  
                               
   
              
int
fsync_nocancel(proc_t p, struct fsync_nocancel_args *uap, __unused int32_t *retval)
{
	return fsync_common(p, (struct fsync_args *)uap, MNT_WAIT);
}


  
                                                                      
   
              
int
fdatasync(proc_t p, struct fdatasync_args *uap, __unused int32_t *retval)
{
	__pthread_testcancel(1);
	return fsync_common(p, (struct fsync_args *)uap, MNT_DWAIT);
}


  
               
  
                                                                               
                                                                             
  
                                                                          
                                                                       
                                                                           
                                                                        
                                                                           
           
  
                               
                                            
                                     
  
                          
                                         
                                                   
                                
  
                                                                      
                                                       
   
static int
fsync_common(proc_t p, struct fsync_args *uap, int flags)
{
	vnode_t vp;
	struct fileproc *fp;
	vfs_context_t ctx = vfs_context_current();
	int error;

	AUDIT_ARG(fd, uap->fd);

	if ((error = fp_getfvp(p, uap->fd, &fp, &vp))) {
		return error;
	}
	if ((error = vnode_getwithref(vp))) {
		file_drop(uap->fd);
		return error;
	}

	AUDIT_ARG(vnpath, vp, ARG_VNODE1);

	error = VNOP_FSYNC(vp, flags, ctx);

#if NAMEDRSRCFORK
	                                                  
	if ((error == 0) &&
	    (vp->v_flag & VISNAMEDSTREAM) &&
	    (vp->v_parent != NULLVP) &&
	    vnode_isshadow(vp) &&
	    (fp->fp_glob->fg_flag & FWASWRITTEN)) {
		(void) vnode_flushnamedstream(vp->v_parent, vp, ctx);
	}
#endif

	(void)vnode_put(vp);
	file_drop(uap->fd);
	return error;
}

  
                                                                    
                  
  
                                                                           
                                     
   
              
int
copyfile(__unused proc_t p, struct copyfile_args *uap, __unused int32_t *retval)
{
	vnode_t tvp, fvp, tdvp, sdvp;
	struct nameidata fromnd, tond;
	int error;
	vfs_context_t ctx = vfs_context_current();
#if CONFIG_MACF
	struct filedesc *fdp = (vfs_context_proc(ctx))->p_fd;
	struct vnode_attr va;
#endif

	                                     

	if (uap->flags & ~CPF_MASK) {
		return EINVAL;
	}

	NDINIT(&fromnd, LOOKUP, OP_COPYFILE, AUDITVNPATH1,
	    UIO_USERSPACE, uap->from, ctx);
	if ((error = namei(&fromnd))) {
		return error;
	}
	fvp = fromnd.ni_vp;

	NDINIT(&tond, CREATE, OP_LINK,
	    LOCKPARENT | LOCKLEAF | NOCACHE | SAVESTART | AUDITVNPATH2 | CN_NBMOUNTLOOK,
	    UIO_USERSPACE, uap->to, ctx);
	if ((error = namei(&tond))) {
		goto out1;
	}
	tdvp = tond.ni_dvp;
	tvp = tond.ni_vp;

	if (tvp != NULL) {
		if (!(uap->flags & CPF_OVERWRITE)) {
			error = EEXIST;
			goto out;
		}
	}

	if (fvp->v_type == VDIR || (tvp && tvp->v_type == VDIR)) {
		error = EISDIR;
		goto out;
	}

	                                             
	if ((error = vn_authorize_open_existing(fvp, &fromnd.ni_cnd, FREAD, ctx,
	    NULL))) {
		goto out;
	}

	if (tvp) {
		  
                                                              
                                                               
                                                              
                                            
     
		error = vn_authorize_unlink(tdvp, tvp, &tond.ni_cnd, ctx, NULL);
		if (error && error != ENOENT) {
			goto out;
		}
		error = 0;
	}

#if CONFIG_MACF
	VATTR_INIT(&va);
	VATTR_SET(&va, va_type, fvp->v_type);
	                                                 
	VATTR_SET(&va, va_mode,
	    ((((uap->mode & ~fdp->fd_cmask) & ALLPERMS) & ~S_ISTXT) & ACCESSPERMS));
	error = mac_vnode_check_create(ctx, tdvp, &tond.ni_cnd, &va);
	if (error) {
		goto out;
	}
#endif                  

	if ((error = vnode_authorize(tdvp, NULL, KAUTH_VNODE_ADD_FILE, ctx)) != 0) {
		goto out;
	}

	if (fvp == tdvp) {
		error = EINVAL;
	}
	  
                                                         
                                                   
                                                
    
	if (fvp == tvp) {
		error = -1;
	}
	if (!error) {
		error = VNOP_COPYFILE(fvp, tdvp, tvp, &tond.ni_cnd, uap->mode, uap->flags, ctx);
	}
out:
	sdvp = tond.ni_startdir;
	  
                                                     
                                                            
    
	nameidone(&tond);

	if (tvp) {
		vnode_put(tvp);
	}
	vnode_put(tdvp);
	vnode_put(sdvp);
out1:
	vnode_put(fvp);

	nameidone(&fromnd);

	if (error == -1) {
		return 0;
	}
	return error;
}

#define CLONE_SNAPSHOT_FALLBACKS_ENABLED 1

  
                                                                         
                                         
   
static int
clonefile_internal(vnode_t fvp, boolean_t data_read_authorised, int dst_dirfd,
    user_addr_t dst, uint32_t flags, vfs_context_t ctx)
{
	vnode_t tvp, tdvp;
	struct nameidata tond;
	int error;
	int follow;
	boolean_t free_src_acl;
	boolean_t attr_cleanup;
	enum vtype v_type;
	kauth_action_t action;
	struct componentname *cnp;
	uint32_t defaulted;
	struct vnode_attr va;
	struct vnode_attr nva;
	uint32_t vnop_flags;

	v_type = vnode_vtype(fvp);
	switch (v_type) {
	case VLNK:
	              
	case VREG:
		action = KAUTH_VNODE_ADD_FILE;
		break;
	case VDIR:
		if (vnode_isvroot(fvp) || vnode_ismount(fvp) ||
		    fvp->v_mountedhere) {
			return EINVAL;
		}
		action = KAUTH_VNODE_ADD_SUBDIRECTORY;
		break;
	default:
		return EINVAL;
	}

	AUDIT_ARG(fd2, dst_dirfd);
	AUDIT_ARG(value32, flags);

	follow = (flags & CLONE_NOFOLLOW) ? NOFOLLOW : FOLLOW;
	NDINIT(&tond, CREATE, OP_LINK, follow | WANTPARENT | AUDITVNPATH2,
	    UIO_USERSPACE, dst, ctx);
	if ((error = nameiat(&tond, dst_dirfd))) {
		return error;
	}
	cnp = &tond.ni_cnd;
	tdvp = tond.ni_dvp;
	tvp = tond.ni_vp;

	free_src_acl = FALSE;
	attr_cleanup = FALSE;

	if (tvp != NULL) {
		error = EEXIST;
		goto out;
	}

	if (vnode_mount(tdvp) != vnode_mount(fvp)) {
		error = EXDEV;
		goto out;
	}

#if CONFIG_MACF
	if ((error = mac_vnode_check_clone(ctx, tdvp, fvp, cnp))) {
		goto out;
	}
#endif
	if ((error = vnode_authorize(tdvp, NULL, action, ctx))) {
		goto out;
	}

	action = KAUTH_VNODE_GENERIC_READ_BITS;
	if (data_read_authorised) {
		action &= ~KAUTH_VNODE_READ_DATA;
	}
	if ((error = vnode_authorize(fvp, NULL, action, ctx))) {
		goto out;
	}

	  
                                                                         
                                                                      
                                            
    
	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_uid);
	VATTR_WANTED(&va, va_gid);
	VATTR_WANTED(&va, va_mode);
	VATTR_WANTED(&va, va_flags);

	if ((error = vnode_getattr(fvp, &va, ctx)) != 0) {
		goto out;
	}

	VATTR_INIT(&nva);
	VATTR_SET(&nva, va_type, v_type);
	if (VATTR_IS_SUPPORTED(&va, va_acl) && va.va_acl != NULL) {
		VATTR_SET(&nva, va_acl, va.va_acl);
		free_src_acl = TRUE;
	}

	                                             
	if (v_type == VLNK) {
		error = vnode_authattr_new(tdvp, &nva, 0, ctx);
	} else {
		error = vn_attribute_prepare(tdvp, &nva, &defaulted, ctx);
		if (error) {
			goto out;
		}
		attr_cleanup = TRUE;
	}

	vnop_flags = VNODE_CLONEFILE_DEFAULT;
	  
                                                         
                                                            
                                                                 
                                                              
                        
    
	if (!(flags & CLONE_NOOWNERCOPY) && vfs_context_issuser(ctx)) {
		if (VATTR_IS_SUPPORTED(&va, va_uid)) {
			VATTR_SET(&nva, va_uid, va.va_uid);
		}
		if (VATTR_IS_SUPPORTED(&va, va_gid)) {
			VATTR_SET(&nva, va_gid, va.va_gid);
		}
	} else {
		vnop_flags |= VNODE_CLONEFILE_NOOWNERCOPY;
	}

	if (VATTR_IS_SUPPORTED(&va, va_mode)) {
		VATTR_SET(&nva, va_mode, va.va_mode);
	}
	if (VATTR_IS_SUPPORTED(&va, va_flags)) {
		VATTR_SET(&nva, va_flags,
		    ((va.va_flags & ~(UF_DATAVAULT | SF_RESTRICTED)) |                           
		    (nva.va_flags & (UF_DATAVAULT | SF_RESTRICTED))));
	}

	error = VNOP_CLONEFILE(fvp, tdvp, &tvp, cnp, &nva, vnop_flags, ctx);

	if (!error && tvp) {
		int     update_flags = 0;
#if CONFIG_FSE
		int fsevent;
#endif                 

		  
                                                               
                                 
     
		if (!VATTR_ALL_SUPPORTED(&nva)) {
			(void)vnode_setattr_fallback(tvp, &nva, ctx);
		}

#if CONFIG_MACF
		(void)vnode_label(vnode_mount(tvp), tdvp, tvp, cnp,
		    VNODE_LABEL_CREATE, ctx);
#endif

		                                                     
		if (tvp->v_name == NULL) {
			update_flags |= VNODE_UPDATE_NAME;
		}
		if (tvp->v_parent == NULLVP) {
			update_flags |= VNODE_UPDATE_PARENT;
		}

		if (update_flags) {
			(void)vnode_update_identity(tvp, tdvp, cnp->cn_nameptr,
			    cnp->cn_namelen, cnp->cn_hash, update_flags);
		}

#if CONFIG_FSE
		switch (vnode_vtype(tvp)) {
		case VLNK:
		              
		case VREG:
			fsevent = FSE_CREATE_FILE;
			break;
		case VDIR:
			fsevent = FSE_CREATE_DIR;
			break;
		default:
			goto out;
		}

		if (need_fsevent(fsevent, tvp)) {
			  
                                                           
                                                                        
                                                                 
                                                                    
                                                                          
                     
                                  
      
			add_fsevent(FSE_CLONE, ctx, FSE_ARG_VNODE, fvp, FSE_ARG_VNODE, tvp,
			    FSE_ARG_DONE);
			add_fsevent(fsevent, ctx, FSE_ARG_VNODE, tvp,
			    FSE_ARG_DONE);
		}
#endif                 
	}

out:
	if (attr_cleanup) {
		vn_attribute_cleanup(&nva, defaulted);
	}
	if (free_src_acl && va.va_acl) {
		kauth_acl_free(va.va_acl);
	}
	nameidone(&tond);
	if (tvp) {
		vnode_put(tvp);
	}
	vnode_put(tdvp);
	return error;
}

  
                                                     
   
              
int
clonefileat(__unused proc_t p, struct clonefileat_args *uap,
    __unused int32_t *retval)
{
	vnode_t fvp;
	struct nameidata fromnd;
	int follow;
	int error;
	vfs_context_t ctx = vfs_context_current();

	                                     
	if (uap->flags & ~(CLONE_NOFOLLOW | CLONE_NOOWNERCOPY)) {
		return EINVAL;
	}

	AUDIT_ARG(fd, uap->src_dirfd);

	follow = (uap->flags & CLONE_NOFOLLOW) ? NOFOLLOW : FOLLOW;
	NDINIT(&fromnd, LOOKUP, OP_COPYFILE, follow | AUDITVNPATH1,
	    UIO_USERSPACE, uap->src, ctx);
	if ((error = nameiat(&fromnd, uap->src_dirfd))) {
		return error;
	}

	fvp = fromnd.ni_vp;
	nameidone(&fromnd);

	error = clonefile_internal(fvp, FALSE, uap->dst_dirfd, uap->dst,
	    uap->flags, ctx);

	vnode_put(fvp);
	return error;
}

int
fclonefileat(__unused proc_t p, struct fclonefileat_args *uap,
    __unused int32_t *retval)
{
	vnode_t fvp;
	struct fileproc *fp;
	int error;
	vfs_context_t ctx = vfs_context_current();

	                                     
	if (uap->flags & ~(CLONE_NOFOLLOW | CLONE_NOOWNERCOPY)) {
		return EINVAL;
	}

	AUDIT_ARG(fd, uap->src_fd);
	error = fp_getfvp(p, uap->src_fd, &fp, &fvp);
	if (error) {
		return error;
	}

	if ((fp->fp_glob->fg_flag & FREAD) == 0) {
		AUDIT_ARG(vnpath_withref, fvp, ARG_VNODE1);
		error = EBADF;
		goto out;
	}

	if ((error = vnode_getwithref(fvp))) {
		goto out;
	}

	AUDIT_ARG(vnpath, fvp, ARG_VNODE1);

	error = clonefile_internal(fvp, TRUE, uap->dst_dirfd, uap->dst,
	    uap->flags, ctx);

	vnode_put(fvp);
out:
	file_drop(uap->src_fd);
	return error;
}

static int
rename_submounts_callback(mount_t mp, void *arg)
{
	int error = 0;
	mount_t pmp = (mount_t)arg;
	int prefix_len = (int)strlen(pmp->mnt_vfsstat.f_mntonname);

	if (strncmp(mp->mnt_vfsstat.f_mntonname, pmp->mnt_vfsstat.f_mntonname, prefix_len) != 0) {
		return 0;
	}

	if (mp->mnt_vfsstat.f_mntonname[prefix_len] != '/') {
		return 0;
	}

	if ((error = vfs_busy(mp, LK_NOWAIT))) {
		printf("vfs_busy failed with %d for %s\n", error, mp->mnt_vfsstat.f_mntonname);
		return -1;
	}

	int pathlen = MAXPATHLEN;
	if ((error = vn_getpath_ext(mp->mnt_vnodecovered, NULL, mp->mnt_vfsstat.f_mntonname, &pathlen, VN_GETPATH_FSENTER))) {
		printf("vn_getpath_ext failed with %d for mnt_vnodecovered of %s\n", error, mp->mnt_vfsstat.f_mntonname);
	}

	vfs_unbusy(mp);

	return error;
}

  
                                                                         
                                                                           
   
              
static int
renameat_internal(vfs_context_t ctx, int fromfd, user_addr_t from,
    int tofd, user_addr_t to, int segflg, vfs_rename_flags_t flags)
{
	if (flags & ~VFS_RENAME_FLAGS_MASK) {
		return EINVAL;
	}

	if (ISSET(flags, VFS_RENAME_SWAP) && ISSET(flags, VFS_RENAME_EXCL)) {
		return EINVAL;
	}

	vnode_t tvp, tdvp;
	vnode_t fvp, fdvp;
	vnode_t mnt_fvp;
	struct nameidata *fromnd, *tond;
	int error;
	int do_retry;
	int retry_count;
	int mntrename;
	int need_event;
	int need_kpath2;
	int has_listeners;
	const char *oname = NULL;
	char *from_name = NULL, *to_name = NULL;
	char *from_name_no_firmlink = NULL, *to_name_no_firmlink = NULL;
	int from_len = 0, to_len = 0;
	int from_len_no_firmlink = 0, to_len_no_firmlink = 0;
	int holding_mntlock;
	int vn_authorize_skipped;
	mount_t locked_mp = NULL;
	vnode_t oparent = NULLVP;
#if CONFIG_FSE
	fse_info from_finfo, to_finfo;
#endif
	int from_truncated = 0, to_truncated = 0;
	int from_truncated_no_firmlink = 0, to_truncated_no_firmlink = 0;
	int batched = 0;
	struct vnode_attr *fvap, *tvap;
	int continuing = 0;
	                                                                      
	struct {
		struct nameidata from_node, to_node;
		struct vnode_attr fv_attr, tv_attr;
	} * __rename_data;
	__rename_data = kheap_alloc(KHEAP_TEMP, sizeof(*__rename_data), Z_WAITOK);
	fromnd = &__rename_data->from_node;
	tond = &__rename_data->to_node;

	holding_mntlock = 0;
	do_retry = 0;
	retry_count = 0;
retry:
	fvp = tvp = NULL;
	fdvp = tdvp = NULL;
	fvap = tvap = NULL;
	mnt_fvp = NULLVP;
	mntrename = FALSE;
	vn_authorize_skipped = FALSE;

	NDINIT(fromnd, DELETE, OP_UNLINK, WANTPARENT | AUDITVNPATH1,
	    segflg, from, ctx);
	fromnd->ni_flag = NAMEI_COMPOUNDRENAME;

	NDINIT(tond, RENAME, OP_RENAME, WANTPARENT | AUDITVNPATH2 | CN_NBMOUNTLOOK,
	    segflg, to, ctx);
	tond->ni_flag = NAMEI_COMPOUNDRENAME;

continue_lookup:
	if ((fromnd->ni_flag & NAMEI_CONTLOOKUP) != 0 || !continuing) {
		if ((error = nameiat(fromnd, fromfd))) {
			goto out1;
		}
		fdvp = fromnd->ni_dvp;
		fvp  = fromnd->ni_vp;

		if (fvp && fvp->v_type == VDIR) {
			tond->ni_cnd.cn_flags |= WILLBEDIR;
		}
	}

	if ((tond->ni_flag & NAMEI_CONTLOOKUP) != 0 || !continuing) {
		if ((error = nameiat(tond, tofd))) {
			  
                                                        
      
			if (error == EISDIR && fvp->v_type == VDIR) {
				error = EINVAL;
			}
			goto out1;
		}
		tdvp = tond->ni_dvp;
		tvp  = tond->ni_vp;
	}

#if DEVELOPMENT || DEBUG
	  
                                                          
                                            
    
#else                           

	if (fromnd->ni_vp && vnode_isswap(fromnd->ni_vp) && (ctx != vfs_context_kernel())) {
		error = EPERM;
		goto out1;
	}

	if (tond->ni_vp && vnode_isswap(tond->ni_vp) && (ctx != vfs_context_kernel())) {
		error = EPERM;
		goto out1;
	}
#endif                           

	if (!tvp && ISSET(flags, VFS_RENAME_SWAP)) {
		error = ENOENT;
		goto out1;
	}

	if (tvp && ISSET(flags, VFS_RENAME_EXCL)) {
		int32_t pval = 0;
		int err = 0;

		  
                                                                         
                                                                      
                                                  
                                                  
                                            
                                                                       
     
		if (fvp != tvp || fdvp != tdvp) {
			error = EEXIST;
			goto out1;
		}

		  
                                                            
                                                 
     
		err = VNOP_PATHCONF(tvp, _PC_CASE_SENSITIVE, &pval, ctx);
		if (err != 0 || pval != 0) {
			error = EEXIST;
			goto out1;
		}
	}

	batched = vnode_compound_rename_available(fdvp);

#if CONFIG_FSE
	need_event = need_fsevent(FSE_RENAME, fdvp);
	if (need_event) {
		if (fvp) {
			get_fse_info(fvp, &from_finfo, ctx);
		} else {
			error = vfs_get_notify_attributes(&__rename_data->fv_attr);
			if (error) {
				goto out1;
			}

			fvap = &__rename_data->fv_attr;
		}

		if (tvp) {
			get_fse_info(tvp, &to_finfo, ctx);
		} else if (batched) {
			error = vfs_get_notify_attributes(&__rename_data->tv_attr);
			if (error) {
				goto out1;
			}

			tvap = &__rename_data->tv_attr;
		}
	}
#else
	need_event = 0;
#endif                 

	has_listeners = kauth_authorize_fileop_has_listeners();

	need_kpath2 = 0;
#if CONFIG_AUDIT
	if (AUDIT_RECORD_EXISTS()) {
		need_kpath2 = 1;
	}
#endif

	if (need_event || has_listeners) {
		if (from_name == NULL) {
			GET_PATH(from_name);
		}

		from_len = safe_getpath(fdvp, fromnd->ni_cnd.cn_nameptr, from_name, MAXPATHLEN, &from_truncated);

		if (from_name_no_firmlink == NULL) {
			GET_PATH(from_name_no_firmlink);
		}

		from_len_no_firmlink = safe_getpath_no_firmlink(fdvp, fromnd->ni_cnd.cn_nameptr, from_name_no_firmlink, MAXPATHLEN, &from_truncated_no_firmlink);
	}

	if (need_event || need_kpath2 || has_listeners) {
		if (to_name == NULL) {
			GET_PATH(to_name);
		}

		to_len = safe_getpath(tdvp, tond->ni_cnd.cn_nameptr, to_name, MAXPATHLEN, &to_truncated);

		if (to_name_no_firmlink == NULL) {
			GET_PATH(to_name_no_firmlink);
		}

		to_len_no_firmlink = safe_getpath_no_firmlink(tdvp, tond->ni_cnd.cn_nameptr, to_name_no_firmlink, MAXPATHLEN, &to_truncated_no_firmlink);
		if (to_name && need_kpath2) {
			AUDIT_ARG(kpath, to_name, ARG_KPATH2);
		}
	}
	if (!fvp) {
		  
                                                        
                                                                                                                       
                                                     
                                                                                                                                               
                                                                              
                                                                                                                                          
    
                                                                                
     
		if (fdvp->v_mount != tdvp->v_mount) {
			error = EXDEV;
			goto out1;
		}
		goto skipped_lookup;
	}

	  
                                                            
                                                          
                                                
   
                          
    
	if (fvp == tvp) {
		int pathconf_val;

		  
                                                          
                                                         
     
		if (VNOP_PATHCONF(fvp, _PC_CASE_SENSITIVE, &pathconf_val, ctx) != 0 ||
		    pathconf_val != 0) {
			vn_authorize_skipped = TRUE;
			goto out1;
		}
	}

	  
                                       
                           
                                                        
                                    
                                                                       
   
                                                                 
                              
   
                                                               
                                                             
                                                                 
                                                                   
                     
    
	if ((fvp->v_flag & VROOT) &&
	    (fvp->v_type == VDIR) &&
	    (tvp == NULL) &&
	    (fvp->v_mountedhere == NULL) &&
	    (fdvp == tdvp) &&
	    ((fvp->v_mount->mnt_flag & (MNT_UNION | MNT_ROOTFS)) == 0) &&
	    ((fvp->v_mount->mnt_kern_flag & MNTK_SYSTEM) == 0) &&
	    (fvp->v_mount->mnt_vnodecovered != NULLVP)) {
		vnode_t coveredvp;

		                                     
		coveredvp = fvp->v_mount->mnt_vnodecovered;
		if ((vnode_getwithref(coveredvp))) {
			error = ENOENT;
			goto out1;
		}
		  
                                                                         
           
     
		mnt_fvp = fvp;

		fvp = coveredvp;
		mntrename = TRUE;
	}
	  
                                  
    
	if ((fvp->v_mount != tdvp->v_mount) ||
	    (tvp && (fvp->v_mount != tvp->v_mount))) {
		error = EXDEV;
		goto out1;
	}

	  
                                                         
                                                     
                                                      
                                                       
                                                       
                                                            
   
                                                                        
                                                                     
                                                                   
                                                                      
                                                                         
   
                                                                  
                                                                       
             
   
                                                              
    
	if (fvp == tvp && fdvp == tdvp) {
		if (fromnd->ni_cnd.cn_namelen == tond->ni_cnd.cn_namelen &&
		    !bcmp(fromnd->ni_cnd.cn_nameptr, tond->ni_cnd.cn_nameptr,
		    fromnd->ni_cnd.cn_namelen)) {
			vn_authorize_skipped = TRUE;
			goto out1;
		}
	}

	if (holding_mntlock && fvp->v_mount != locked_mp) {
		  
                                       
                                           
                                           
     
		mount_unlock_renames(locked_mp);
		mount_drop(locked_mp, 0);
		holding_mntlock = 0;
	}
	if (tdvp != fdvp && fvp->v_type == VDIR) {
		  
                                    
                                      
                                   
                 
                                    
                                  
                                  
                                  
                                    
     
		if (!holding_mntlock) {
			  
                                 
                               
                                   
                                      
                  
      
			locked_mp = fvp->v_mount;
			mount_ref(locked_mp, 0);

			  
                                                      
                                                             
      
			nameidone(tond);

			if (tvp) {
				vnode_put(tvp);
			}
			vnode_put(tdvp);

			  
                                                       
                                                             
      
			nameidone(fromnd);

			vnode_put(fvp);
			vnode_put(fdvp);

			if (mnt_fvp != NULLVP) {
				vnode_put(mnt_fvp);
			}

			mount_lock_renames(locked_mp);
			holding_mntlock = 1;

			goto retry;
		}
	} else {
		  
                                         
                                         
                                                 
                                              
                                                 
                                               
                                                 
                    
     
		if (holding_mntlock) {
			mount_unlock_renames(locked_mp);
			mount_drop(locked_mp, 0);
			holding_mntlock = 0;
		}
	}

	if (!batched) {
		error = vn_authorize_renamex_with_paths(fdvp, mntrename ? mnt_fvp : fvp,
		    &fromnd->ni_cnd, from_name, tdvp, tvp, &tond->ni_cnd, to_name, ctx,
		    flags, NULL);
		if (error) {
			if (error == ENOENT) {
				if (retry_count < MAX_AUTHORIZE_ENOENT_RETRIES) {
					  
                                                          
                                                                
                          
        
					do_retry = 1;
					retry_count += 1;
				}
			}
			goto out1;
		}
	}

	                                                            
	if (mnt_fvp != NULLVP) {
		vnode_put(mnt_fvp);
		mnt_fvp = NULLVP;
	}

	                                                             
	oname   = fvp->v_name;
	oparent = fvp->v_parent;

skipped_lookup:
	error = vn_rename(fdvp, &fvp, &fromnd->ni_cnd, fvap,
	    tdvp, &tvp, &tond->ni_cnd, tvap,
	    flags, ctx);

	if (holding_mntlock) {
		  
                                  
             
     
		mount_unlock_renames(locked_mp);
		mount_drop(locked_mp, 0);
		holding_mntlock = 0;
	}
	if (error) {
		if (error == EDATALESS) {
			  
                                                   
                                                    
                                         
      
			if (flags & VFS_RENAME_DATALESS) {
				error = EIO;
				goto out1;
			}

			  
                                                     
                                                       
                                                      
                                                    
                      
     
                                                    
                                                      
      
			error = vnode_materialize_dataless_file(fvp,
			    NAMESPACE_HANDLER_RENAME_OP);

			if (error == 0) {
				  
                                               
                                               
                      
       
				flags |= VFS_RENAME_DATALESS;
				do_retry = 1;
			}
			goto out1;
		}
		if (error == EKEEPLOOKING) {
			if ((fromnd->ni_flag & NAMEI_CONTLOOKUP) == 0) {
				if ((tond->ni_flag & NAMEI_CONTLOOKUP) == 0) {
					panic("EKEEPLOOKING without NAMEI_CONTLOOKUP on either ndp?");
				}
			}

			fromnd->ni_vp = fvp;
			tond->ni_vp = tvp;

			goto continue_lookup;
		}

		  
                                                                     
                                                                   
                                                                          
                                                                        
                                                                         
     
		if (error == ERECYCLE) {
			if (retry_count < MAX_RENAME_ERECYCLE_RETRIES) {
				do_retry = 1;
				retry_count += 1;
			} else {
				printf("rename retry limit due to ERECYCLE reached\n");
				error = ENOENT;
			}
		}

		  
                                                              
                                                               
                               
     
		if (batched && error == ENOENT) {
			if (retry_count < MAX_AUTHORIZE_ENOENT_RETRIES) {
				do_retry = 1;
				retry_count += 1;
			}
		}

		goto out1;
	}

	                                                      
                                                 
    
	kauth_authorize_fileop(vfs_context_ucred(ctx),
	    KAUTH_FILEOP_RENAME,
	    (uintptr_t)from_name, (uintptr_t)to_name);
	if (flags & VFS_RENAME_SWAP) {
		kauth_authorize_fileop(vfs_context_ucred(ctx),
		    KAUTH_FILEOP_RENAME,
		    (uintptr_t)to_name, (uintptr_t)from_name);
	}

#if CONFIG_FSE
	if (from_name != NULL && to_name != NULL) {
		if (from_truncated || to_truncated) {
			                                                                       
			from_finfo.mode |= FSE_TRUNCATED_PATH;
		}

		if (tvap && tvp) {
			vnode_get_fse_info_from_vap(tvp, &to_finfo, tvap);
		}
		if (fvap) {
			vnode_get_fse_info_from_vap(fvp, &from_finfo, fvap);
		}

		if (tvp) {
			add_fsevent(FSE_RENAME, ctx,
			    FSE_ARG_STRING, from_len_no_firmlink, from_name_no_firmlink,
			    FSE_ARG_FINFO, &from_finfo,
			    FSE_ARG_STRING, to_len_no_firmlink, to_name_no_firmlink,
			    FSE_ARG_FINFO, &to_finfo,
			    FSE_ARG_DONE);
			if (flags & VFS_RENAME_SWAP) {
				  
                                                   
                                                          
                                                        
           
       
				add_fsevent(FSE_RENAME, ctx,
				    FSE_ARG_STRING, to_len_no_firmlink, to_name_no_firmlink,
				    FSE_ARG_FINFO, &to_finfo,
				    FSE_ARG_STRING, from_len_no_firmlink, from_name_no_firmlink,
				    FSE_ARG_FINFO, &from_finfo,
				    FSE_ARG_DONE);
			}
		} else {
			add_fsevent(FSE_RENAME, ctx,
			    FSE_ARG_STRING, from_len_no_firmlink, from_name_no_firmlink,
			    FSE_ARG_FINFO, &from_finfo,
			    FSE_ARG_STRING, to_len_no_firmlink, to_name_no_firmlink,
			    FSE_ARG_DONE);
		}
	}
#endif                 

	  
                                        
    
	if (mntrename) {
		char *cp, *pathend, *mpname;
		char * tobuf;
		struct mount *mp;
		int maxlen;
		size_t len = 0;

		mp = fvp->v_mountedhere;

		if (vfs_busy(mp, LK_NOWAIT)) {
			error = EBUSY;
			goto out1;
		}
		tobuf = zalloc(ZV_NAMEI);

		if (UIO_SEG_IS_USER_SPACE(segflg)) {
			error = copyinstr(to, tobuf, MAXPATHLEN, &len);
		} else {
			error = copystr((void *)to, tobuf, MAXPATHLEN, &len);
		}
		if (!error) {
			                                     
			pathend = &mp->mnt_vfsstat.f_mntonname[0];
			for (cp = pathend; *cp != '\0'; ++cp) {
				if (*cp == '/') {
					pathend = cp + 1;
				}
			}
			                                        
			for (mpname = cp = tobuf; *cp != '\0'; ++cp) {
				if (*cp == '/') {
					mpname = cp + 1;
				}
			}

			                                      
			vfs_iterate(0, rename_submounts_callback, (void *)mp);

			                           
			maxlen = MAXPATHLEN - (int)(pathend - mp->mnt_vfsstat.f_mntonname);
			bzero(pathend, maxlen);

			strlcpy(pathend, mpname, maxlen);
		}
		zfree(ZV_NAMEI, tobuf);

		vfs_unbusy(mp);

		vfs_event_signal(NULL, VQ_UPDATE, (intptr_t)NULL);
	}
	  
                                                      
                                                       
                                                        
              
   
                                                                  
    
	if (batched || (oname == fvp->v_name && oparent == fvp->v_parent)) {
		int update_flags;

		update_flags = VNODE_UPDATE_NAME;

		if (fdvp != tdvp) {
			update_flags |= VNODE_UPDATE_PARENT;
		}

		vnode_update_identity(fvp, tdvp, tond->ni_cnd.cn_nameptr, tond->ni_cnd.cn_namelen, tond->ni_cnd.cn_hash, update_flags);
	}
out1:
	  
                                                                      
                                                      
    
	if (vn_authorize_skipped && error == 0) {
		error = vn_authorize_renamex_with_paths(fdvp, fvp,
		    &fromnd->ni_cnd, from_name, tdvp, tvp, &tond->ni_cnd, to_name, ctx,
		    flags, NULL);
		if (error && error == ENOENT) {
			if (retry_count < MAX_AUTHORIZE_ENOENT_RETRIES) {
				do_retry = 1;
				retry_count += 1;
			}
		}
	}
	if (to_name != NULL) {
		RELEASE_PATH(to_name);
		to_name = NULL;
	}
	if (to_name_no_firmlink != NULL) {
		RELEASE_PATH(to_name_no_firmlink);
		to_name_no_firmlink = NULL;
	}
	if (from_name != NULL) {
		RELEASE_PATH(from_name);
		from_name = NULL;
	}
	if (from_name_no_firmlink != NULL) {
		RELEASE_PATH(from_name_no_firmlink);
		from_name_no_firmlink = NULL;
	}
	if (holding_mntlock) {
		mount_unlock_renames(locked_mp);
		mount_drop(locked_mp, 0);
		holding_mntlock = 0;
	}
	if (tdvp) {
		  
                                                      
                                                             
     
		nameidone(tond);

		if (tvp) {
			vnode_put(tvp);
		}
		vnode_put(tdvp);
	}
	if (fdvp) {
		  
                                                      
                                                             
     
		nameidone(fromnd);

		if (fvp) {
			vnode_put(fvp);
		}
		vnode_put(fdvp);
	}
	if (mnt_fvp != NULLVP) {
		vnode_put(mnt_fvp);
	}
	  
                                                                   
                                  
    
	if (do_retry) {
		do_retry = 0;
		goto retry;
	}

	kheap_free(KHEAP_TEMP, __rename_data, sizeof(*__rename_data));
	return error;
}

int
rename(__unused proc_t p, struct rename_args *uap, __unused int32_t *retval)
{
	return renameat_internal(vfs_context_current(), AT_FDCWD, uap->from,
	           AT_FDCWD, uap->to, UIO_USERSPACE, 0);
}

int
renameatx_np(__unused proc_t p, struct renameatx_np_args *uap, __unused int32_t *retval)
{
	return renameat_internal(
		vfs_context_current(),
		uap->fromfd, uap->from,
		uap->tofd, uap->to,
		UIO_USERSPACE, uap->flags);
}

int
renameat(__unused proc_t p, struct renameat_args *uap, __unused int32_t *retval)
{
	return renameat_internal(vfs_context_current(), uap->fromfd, uap->from,
	           uap->tofd, uap->to, UIO_USERSPACE, 0);
}

  
                         
  
                       
          
            
                      
                
   
              
static int
mkdir1at(vfs_context_t ctx, user_addr_t path, struct vnode_attr *vap, int fd,
    enum uio_seg segflg)
{
	vnode_t vp, dvp;
	int error;
	int update_flags = 0;
	int batched;
	struct nameidata nd;

	AUDIT_ARG(mode, vap->va_mode);
	NDINIT(&nd, CREATE, OP_MKDIR, LOCKPARENT | AUDITVNPATH1, segflg,
	    path, ctx);
	nd.ni_cnd.cn_flags |= WILLBEDIR;
	nd.ni_flag = NAMEI_COMPOUNDMKDIR;

continue_lookup:
	error = nameiat(&nd, fd);
	if (error) {
		return error;
	}
	dvp = nd.ni_dvp;
	vp = nd.ni_vp;

	if (vp != NULL) {
		error = EEXIST;
		goto out;
	}

	batched = vnode_compound_mkdir_available(dvp);

	VATTR_SET(vap, va_type, VDIR);

	  
       
                                                                                    
                                                                                     
                                                                                      
                                                                                      
    
	if ((error = vn_authorize_mkdir(dvp, &nd.ni_cnd, vap, ctx, NULL)) != 0) {
		if (error == EACCES || error == EPERM) {
			int error2;

			nameidone(&nd);
			vnode_put(dvp);
			dvp = NULLVP;

			  
                                                                             
                                               
      
			NDINIT(&nd, LOOKUP, OP_MKDIR, AUDITVNPATH1, segflg,
			    path, ctx);
			error2 = nameiat(&nd, fd);
			if (error2) {
				goto out;
			} else {
				vp = nd.ni_vp;
				error = EEXIST;
				goto out;
			}
		}

		goto out;
	}

	  
                      
    
	if ((error = vn_create(dvp, &vp, &nd, vap, 0, 0, NULL, ctx)) != 0) {
		if (error == EKEEPLOOKING) {
			nd.ni_vp = vp;
			goto continue_lookup;
		}

		goto out;
	}

	                                                     
	if (vp->v_name == NULL) {
		update_flags |= VNODE_UPDATE_NAME;
	}
	if (vp->v_parent == NULLVP) {
		update_flags |= VNODE_UPDATE_PARENT;
	}

	if (update_flags) {
		vnode_update_identity(vp, dvp, nd.ni_cnd.cn_nameptr, nd.ni_cnd.cn_namelen, nd.ni_cnd.cn_hash, update_flags);
	}

#if CONFIG_FSE
	add_fsevent(FSE_CREATE_DIR, ctx, FSE_ARG_VNODE, vp, FSE_ARG_DONE);
#endif

out:
	  
                                                    
                                                           
    
	nameidone(&nd);

	if (vp) {
		vnode_put(vp);
	}
	if (dvp) {
		vnode_put(dvp);
	}

	return error;
}

  
                                                                    
  
                                                                                    
                                                                              
                                                   
  
                                                                     
                                                                   
                                                    
  
                                                 
                                                     
  
   
int
mkdir_extended(proc_t p, struct mkdir_extended_args *uap, __unused int32_t *retval)
{
	int ciferror;
	kauth_filesec_t xsecdst;
	struct vnode_attr va;

	AUDIT_ARG(owner, uap->uid, uap->gid);

	xsecdst = NULL;
	if ((uap->xsecurity != USER_ADDR_NULL) &&
	    ((ciferror = kauth_copyinfilesec(uap->xsecurity, &xsecdst)) != 0)) {
		return ciferror;
	}

	VATTR_INIT(&va);
	VATTR_SET(&va, va_mode, (uap->mode & ACCESSPERMS) & ~p->p_fd->fd_cmask);
	if (xsecdst != NULL) {
		VATTR_SET(&va, va_acl, &xsecdst->fsec_acl);
	}

	ciferror = mkdir1at(vfs_context_current(), uap->path, &va, AT_FDCWD,
	    UIO_USERSPACE);
	if (xsecdst != NULL) {
		kauth_filesec_free(xsecdst);
	}
	return ciferror;
}

int
mkdir(proc_t p, struct mkdir_args *uap, __unused int32_t *retval)
{
	struct vnode_attr va;

	VATTR_INIT(&va);
	VATTR_SET(&va, va_mode, (uap->mode & ACCESSPERMS) & ~p->p_fd->fd_cmask);

	return mkdir1at(vfs_context_current(), uap->path, &va, AT_FDCWD,
	           UIO_USERSPACE);
}

int
mkdirat(proc_t p, struct mkdirat_args *uap, __unused int32_t *retval)
{
	struct vnode_attr va;

	VATTR_INIT(&va);
	VATTR_SET(&va, va_mode, (uap->mode & ACCESSPERMS) & ~p->p_fd->fd_cmask);

	return mkdir1at(vfs_context_current(), uap->path, &va, uap->fd,
	           UIO_USERSPACE);
}

static int
rmdirat_internal(vfs_context_t ctx, int fd, user_addr_t dirpath,
    enum uio_seg segflg, int unlink_flags)
{
	vnode_t vp, dvp;
	int error;
	struct nameidata nd;
	char     *path = NULL;
	char     *no_firmlink_path = NULL;
	int       len_path = 0;
	int       len_no_firmlink_path = 0;
	int has_listeners = 0;
	int need_event = 0;
	int truncated_path = 0;
	int truncated_no_firmlink_path = 0;
#if CONFIG_FSE
	struct vnode_attr va;
#endif                 
	struct vnode_attr *vap = NULL;
	int restart_count = 0;
	int batched;

	int restart_flag;

	  
                                                                   
                                                                    
                                          
    
	do {
		NDINIT(&nd, DELETE, OP_RMDIR, LOCKPARENT | AUDITVNPATH1,
		    segflg, dirpath, ctx);
		nd.ni_flag = NAMEI_COMPOUNDRMDIR;
continue_lookup:
		restart_flag = 0;
		vap = NULL;

		error = nameiat(&nd, fd);
		if (error) {
			return error;
		}

		dvp = nd.ni_dvp;
		vp = nd.ni_vp;

		if (vp) {
			batched = vnode_compound_rmdir_available(vp);

			if (vp->v_flag & VROOT) {
				  
                                                          
       
				error = EBUSY;
				goto out;
			}

#if DEVELOPMENT || DEBUG
			  
                                                            
                                              
      
#else                           

			if (vnode_isswap(vp) && (ctx != vfs_context_kernel())) {
				error = EPERM;
				goto out;
			}
#endif                           

			  
                                                        
                                                              
                                                              
                                                                
                                                                    
                                                              
                                                                 
              
      

			if (!batched) {
				error = vn_authorize_rmdir(dvp, vp, &nd.ni_cnd, ctx, NULL);
				if (error) {
					if (error == ENOENT) {
						if (restart_count < MAX_AUTHORIZE_ENOENT_RETRIES) {
							restart_flag = 1;
							restart_count += 1;
						}
					}
					goto out;
				}
			}
		} else {
			batched = 1;

			if (!vnode_compound_rmdir_available(dvp)) {
				panic("No error, but no compound rmdir?");
			}
		}

#if CONFIG_FSE
		fse_info  finfo = {0};

		need_event = need_fsevent(FSE_DELETE, dvp);
		if (need_event) {
			if (!batched) {
				get_fse_info(vp, &finfo, ctx);
			} else {
				error = vfs_get_notify_attributes(&va);
				if (error) {
					goto out;
				}

				vap = &va;
			}
		}
#endif
		has_listeners = kauth_authorize_fileop_has_listeners();
		if (need_event || has_listeners) {
			if (path == NULL) {
				GET_PATH(path);
			}

			len_path = safe_getpath(dvp, nd.ni_cnd.cn_nameptr, path, MAXPATHLEN, &truncated_path);

			if (no_firmlink_path == NULL) {
				GET_PATH(no_firmlink_path);
			}

			len_no_firmlink_path = safe_getpath_no_firmlink(dvp, nd.ni_cnd.cn_nameptr, no_firmlink_path, MAXPATHLEN, &truncated_no_firmlink_path);
#if CONFIG_FSE
			if (truncated_no_firmlink_path) {
				finfo.mode |= FSE_TRUNCATED_PATH;
			}
#endif
		}

		error = vn_rmdir(dvp, &vp, &nd, vap, ctx);
		nd.ni_vp = vp;
		if (vp == NULLVP) {
			                           
			goto out;
		}

		if (error == EKEEPLOOKING) {
			goto continue_lookup;
		} else if (batched && error == ENOENT) {
			if (restart_count < MAX_AUTHORIZE_ENOENT_RETRIES) {
				  
                                                     
                                                            
                          
       
				restart_flag = 1;
				restart_count += 1;
				goto out;
			}
		}

		  
                                               
                                              
                                              
                                             
                                            
                   
     
		if (error == ENOTEMPTY &&
		    (unlink_flags & VNODE_REMOVE_DATALESS_DIR) != 0) {
			  
                                                 
            
      
			if (vn_remove(dvp, &vp, &nd,
			    VNODE_REMOVE_DATALESS_DIR, vap, ctx) == 0) {
				error = 0;
			}
		}

#if CONFIG_APPLEDOUBLE
		  
                                                
                                                    
                                                            
                    
     
		if (error == ENOTEMPTY) {
			int ad_error = rmdir_remove_orphaned_appleDouble(vp, ctx, &restart_flag);
			if (ad_error == EBUSY) {
				error = ad_error;
				goto out;
			}


			  
                                                                
      
			if (!ad_error) {
				error = vn_rmdir(dvp, &vp, &nd, vap, ctx);
			}
		}
#endif                         
		  
                                                        
                                                  
     
		if (!error) {
			if (has_listeners) {
				kauth_authorize_fileop(vfs_context_ucred(ctx),
				    KAUTH_FILEOP_DELETE,
				    (uintptr_t)vp,
				    (uintptr_t)path);
			}

			if (vp->v_flag & VISHARDLINK) {
				                                                   
				                                               
				vnode_update_identity(vp, NULL, NULL, 0, 0, VNODE_UPDATE_PARENT);
			}

#if CONFIG_FSE
			if (need_event) {
				if (vap) {
					vnode_get_fse_info_from_vap(vp, &finfo, vap);
				}
				add_fsevent(FSE_DELETE, ctx,
				    FSE_ARG_STRING, len_no_firmlink_path, no_firmlink_path,
				    FSE_ARG_FINFO, &finfo,
				    FSE_ARG_DONE);
			}
#endif
		}

out:
		if (path != NULL) {
			RELEASE_PATH(path);
			path = NULL;
		}

		if (no_firmlink_path != NULL) {
			RELEASE_PATH(no_firmlink_path);
			no_firmlink_path = NULL;
		}

		  
                                                     
                                                            
     
		nameidone(&nd);
		vnode_put(dvp);

		if (vp) {
			vnode_put(vp);
		}

		if (restart_flag == 0) {
			wakeup_one((caddr_t)vp);
			return error;
		}
		tsleep(vp, PVFS, "rm AD", 1);
	} while (restart_flag != 0);

	return error;
}

  
                           
   
              
int
rmdir(__unused proc_t p, struct rmdir_args *uap, __unused int32_t *retval)
{
	return rmdirat_internal(vfs_context_current(), AT_FDCWD,
	           CAST_USER_ADDR_T(uap->path), UIO_USERSPACE, 0);
}

                                                    
#define DIRENT64_LEN(namlen) \
	((sizeof(struct direntry) + (namlen) - (MAXPATHLEN-1) + 7) & ~7)

                                                  
#define DIRENT_LEN(namelen) \
	((sizeof(struct dirent) + (namelen + 1) - (__DARWIN_MAXNAMLEN + 1) + 3) & ~3)

                                
#define DIRENT_END(dep) \
	(((char *)(dep)) + (dep)->d_reclen - 1)

errno_t
vnode_readdir64(struct vnode *vp, struct uio *uio, int flags, int *eofflag,
    int *numdirent, vfs_context_t ctxp)
{
	                                                          
	if ((vp->v_mount->mnt_vtable->vfc_vfsflags & VFC_VFSREADDIR_EXTENDED) &&
	    ((vp->v_mount->mnt_kern_flag & MNTK_DENY_READDIREXT) == 0)) {
		return VNOP_READDIR(vp, uio, flags, eofflag, numdirent, ctxp);
	} else {
		size_t bufsize;
		void * bufptr;
		uio_t auio;
		struct direntry *entry64;
		struct dirent *dep;
		size_t bytesread;
		int error;

		  
                                                           
                                                               
                                                         
    
                                                                
                                                              
                                                              
                                                                
                                                               
                                                        
    
                                                         
                                                             
                                                           
                                                  
     
		bufsize = 3 * MIN((user_size_t)uio_resid(uio), 87371u) / 8;
		bufptr = kheap_alloc(KHEAP_DATA_BUFFERS, bufsize, Z_WAITOK);
		if (bufptr == NULL) {
			return ENOMEM;
		}

		auio = uio_create(1, 0, UIO_SYSSPACE, UIO_READ);
		uio_addiov(auio, (uintptr_t)bufptr, bufsize);
		auio->uio_offset = uio->uio_offset;

		error = VNOP_READDIR(vp, auio, 0, eofflag, numdirent, ctxp);

		dep = (struct dirent *)bufptr;
		bytesread = bufsize - uio_resid(auio);

		entry64 = kheap_alloc(KHEAP_TEMP, sizeof(struct direntry), Z_WAITOK);
		  
                                                                
     
		while (error == 0 && (char *)dep < ((char *)bufptr + bytesread)) {
			                                                                          
			if ((char*)dep + offsetof(struct dirent, d_name) > ((char *)bufptr + bytesread) ||
			                                                                         
			    DIRENT_END(dep) > ((char *)bufptr + bytesread) ||
			                                                                                 
			    DIRENT_LEN(dep->d_namlen) > dep->d_reclen) {
				printf("%s: %s: Bad dirent recived from directory %s\n", __func__,
				    vp->v_mount->mnt_vfsstat.f_mntonname,
				    vp->v_name ? vp->v_name : "<unknown>");
				error = EIO;
				break;
			}

			size_t  enbufsize = DIRENT64_LEN(dep->d_namlen);

			bzero(entry64, enbufsize);
			                                     
			entry64->d_ino = dep->d_ino;
			entry64->d_seekoff = 0;
			entry64->d_reclen = (uint16_t)enbufsize;
			entry64->d_namlen = dep->d_namlen;
			entry64->d_type = dep->d_type;
			bcopy(dep->d_name, entry64->d_name, dep->d_namlen + 1);

			                         
			dep = (struct dirent *)((char *)dep + dep->d_reclen);

			                                    
			error = uiomove((caddr_t)entry64, entry64->d_reclen, uio);
		}

		                                                                       
		if (error == 0) {
			uio->uio_offset = auio->uio_offset;
		}
		uio_free(auio);
		kheap_free(KHEAP_DATA_BUFFERS, bufptr, bufsize);
		kheap_free(KHEAP_TEMP, entry64, sizeof(struct direntry));
		return error;
	}
}

#define GETDIRENTRIES_MAXBUFSIZE        (128 * 1024 * 1024U)

  
                                                                         
   
static int
getdirentries_common(int fd, user_addr_t bufp, user_size_t bufsize, ssize_t *bytesread,
    off_t *offset, int *eofflag, int flags)
{
	vnode_t vp;
	struct vfs_context context = *vfs_context_current();                    
	struct fileproc *fp;
	uio_t auio;
	int spacetype = proc_is64bit(vfs_context_proc(&context)) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	off_t loff;
	int error, numdirent;
	char uio_buf[UIO_SIZEOF(1)];

	error = fp_getfvp(vfs_context_proc(&context), fd, &fp, &vp);
	if (error) {
		return error;
	}
	if ((fp->fp_glob->fg_flag & FREAD) == 0) {
		AUDIT_ARG(vnpath_withref, vp, ARG_VNODE1);
		error = EBADF;
		goto out;
	}

	if (bufsize > GETDIRENTRIES_MAXBUFSIZE) {
		bufsize = GETDIRENTRIES_MAXBUFSIZE;
	}

#if CONFIG_MACF
	error = mac_file_check_change_offset(vfs_context_ucred(&context), fp->fp_glob);
	if (error) {
		goto out;
	}
#endif
	if ((error = vnode_getwithref(vp))) {
		goto out;
	}
	AUDIT_ARG(vnpath, vp, ARG_VNODE1);

unionread:
	if (vp->v_type != VDIR) {
		(void)vnode_put(vp);
		error = EINVAL;
		goto out;
	}

#if CONFIG_MACF
	error = mac_vnode_check_readdir(&context, vp);
	if (error != 0) {
		(void)vnode_put(vp);
		goto out;
	}
#endif          

	loff = fp->fp_glob->fg_offset;
	auio = uio_createwithbuffer(1, loff, spacetype, UIO_READ, &uio_buf[0], sizeof(uio_buf));
	uio_addiov(auio, bufp, bufsize);

	if (flags & VNODE_READDIR_EXTENDED) {
		error = vnode_readdir64(vp, auio, flags, eofflag, &numdirent, &context);
		fp->fp_glob->fg_offset = uio_offset(auio);
	} else {
		error = VNOP_READDIR(vp, auio, 0, eofflag, &numdirent, &context);
		fp->fp_glob->fg_offset = uio_offset(auio);
	}
	if (error) {
		(void)vnode_put(vp);
		goto out;
	}

	if ((user_ssize_t)bufsize == uio_resid(auio)) {
		if ((vp->v_mount->mnt_flag & MNT_UNION)) {
			struct vnode *tvp = vp;
			if (lookup_traverse_union(tvp, &vp, &context) == 0) {
				vnode_ref(vp);
				fp->fp_glob->fg_data = (caddr_t) vp;
				fp->fp_glob->fg_offset = 0;
				vnode_rele(tvp);
				vnode_put(tvp);
				goto unionread;
			}
			vp = tvp;
		}
	}

	vnode_put(vp);
	if (offset) {
		*offset = loff;
	}

	*bytesread = bufsize - uio_resid(auio);
out:
	file_drop(fd);
	return error;
}


int
getdirentries(__unused struct proc *p, struct getdirentries_args *uap, int32_t *retval)
{
	off_t offset;
	ssize_t bytesread;
	int error, eofflag;

	AUDIT_ARG(fd, uap->fd);
	error = getdirentries_common(uap->fd, uap->buf, uap->count,
	    &bytesread, &offset, &eofflag, 0);

	if (error == 0) {
		if (proc_is64bit(p)) {
			user64_long_t base = (user64_long_t)offset;
			error = copyout((caddr_t)&base, uap->basep, sizeof(user64_long_t));
		} else {
			user32_long_t base = (user32_long_t)offset;
			error = copyout((caddr_t)&base, uap->basep, sizeof(user32_long_t));
		}
		*retval = (int)bytesread;
	}
	return error;
}

int
getdirentries64(__unused struct proc *p, struct getdirentries64_args *uap, user_ssize_t *retval)
{
	off_t offset;
	ssize_t bytesread;
	int error, eofflag;
	user_size_t bufsize;

	AUDIT_ARG(fd, uap->fd);

	  
                                                                     
                                                                  
                                                                            
    
	if (uap->bufsize >= GETDIRENTRIES64_EXTENDED_BUFSIZE) {
		bufsize = uap->bufsize - sizeof(getdirentries64_flags_t);
	} else {
		bufsize = uap->bufsize;
	}

	error = getdirentries_common(uap->fd, uap->buf, bufsize,
	    &bytesread, &offset, &eofflag, VNODE_READDIR_EXTENDED);

	if (error == 0) {
		*retval = bytesread;
		error = copyout((caddr_t)&offset, uap->position, sizeof(off_t));

		if (error == 0 && uap->bufsize >= GETDIRENTRIES64_EXTENDED_BUFSIZE) {
			getdirentries64_flags_t flags = 0;
			if (eofflag) {
				flags |= GETDIRENTRIES64_EOF;
			}
			error = copyout(&flags, (user_addr_t)uap->buf + bufsize,
			    sizeof(flags));
		}
	}
	return error;
}


  
                                                      
                          
   
#define UMASK_NOXSECURITY        (void *)1                                          
static int
umask1(proc_t p, int newmask, __unused kauth_filesec_t fsec, int32_t *retval)
{
	struct filedesc *fdp;

	AUDIT_ARG(mask, newmask);
	proc_fdlock(p);
	fdp = p->p_fd;
	*retval = fdp->fd_cmask;
	fdp->fd_cmask = newmask & ALLPERMS;
	proc_fdunlock(p);
	return 0;
}

  
                                                                                                    
  
                                                                             
                                                                              
                                                                            
  
                                                      
                                                    
  
                                                 
                                                     
  
   
int
umask_extended(proc_t p, struct umask_extended_args *uap, int32_t *retval)
{
	int ciferror;
	kauth_filesec_t xsecdst;

	xsecdst = KAUTH_FILESEC_NONE;
	if (uap->xsecurity != USER_ADDR_NULL) {
		if ((ciferror = kauth_copyinfilesec(uap->xsecurity, &xsecdst)) != 0) {
			return ciferror;
		}
	} else {
		xsecdst = KAUTH_FILESEC_NONE;
	}

	ciferror = umask1(p, uap->newmask, xsecdst, retval);

	if (xsecdst != KAUTH_FILESEC_NONE) {
		kauth_filesec_free(xsecdst);
	}
	return ciferror;
}

int
umask(proc_t p, struct umask_args *uap, int32_t *retval)
{
	return umask1(p, uap->newmask, UMASK_NOXSECURITY, retval);
}

  
                                                               
                   
   
              
int
revoke(proc_t p, struct revoke_args *uap, __unused int32_t *retval)
{
	vnode_t vp;
	struct vnode_attr va;
	vfs_context_t ctx = vfs_context_current();
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, OP_REVOKE, FOLLOW | AUDITVNPATH1, UIO_USERSPACE,
	    uap->path, ctx);
	error = namei(&nd);
	if (error) {
		return error;
	}
	vp = nd.ni_vp;

	nameidone(&nd);

	if (!(vnode_ischr(vp) || vnode_isblk(vp))) {
		error = ENOTSUP;
		goto out;
	}

	if (vnode_isblk(vp) && vnode_ismountedon(vp)) {
		error = EBUSY;
		goto out;
	}

#if CONFIG_MACF
	error = mac_vnode_check_revoke(ctx, vp);
	if (error) {
		goto out;
	}
#endif

	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_uid);
	if ((error = vnode_getattr(vp, &va, ctx))) {
		goto out;
	}
	if (kauth_cred_getuid(vfs_context_ucred(ctx)) != va.va_uid &&
	    (error = suser(vfs_context_ucred(ctx), &p->p_acflag))) {
		goto out;
	}
	if (vp->v_usecount > 0 || (vnode_isaliased(vp))) {
		VNOP_REVOKE(vp, REVOKEALL, ctx);
	}
out:
	vnode_put(vp);
	return error;
}


  
                                      
                                                               
                                                           
   


  
                                                                           
                 
   
              
int
getdirentriesattr(proc_t p, struct getdirentriesattr_args *uap, int32_t *retval)
{
	vnode_t vp;
	struct fileproc *fp;
	uio_t auio = NULL;
	int spacetype = proc_is64bit(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	uint32_t count = 0, savecount = 0;
	uint32_t newstate = 0;
	int error, eofflag;
	off_t loff = 0;
	struct attrlist attributelist;
	vfs_context_t ctx = vfs_context_current();
	int fd = uap->fd;
	char uio_buf[UIO_SIZEOF(1)];
	kauth_action_t action;

	AUDIT_ARG(fd, fd);

	                                          
	if ((error = copyin(uap->alist, (caddr_t)&attributelist, sizeof(attributelist)))) {
		return error;
	}
	if ((error = copyin(uap->count, (caddr_t)&count, sizeof(count)))) {
		return error;
	}
	savecount = count;
	if ((error = fp_getfvp(p, fd, &fp, &vp))) {
		return error;
	}
	if ((fp->fp_glob->fg_flag & FREAD) == 0) {
		AUDIT_ARG(vnpath_withref, vp, ARG_VNODE1);
		error = EBADF;
		goto out;
	}


#if CONFIG_MACF
	error = mac_file_check_change_offset(vfs_context_ucred(ctx),
	    fp->fp_glob);
	if (error) {
		goto out;
	}
#endif


	if ((error = vnode_getwithref(vp))) {
		goto out;
	}

	AUDIT_ARG(vnpath, vp, ARG_VNODE1);

unionread:
	if (vp->v_type != VDIR) {
		(void)vnode_put(vp);
		error = EINVAL;
		goto out;
	}

#if CONFIG_MACF
	error = mac_vnode_check_readdir(ctx, vp);
	if (error != 0) {
		(void)vnode_put(vp);
		goto out;
	}
#endif          

	                                                                         
	loff = fp->fp_glob->fg_offset;
	auio = uio_createwithbuffer(1, loff, spacetype, UIO_READ, &uio_buf[0], sizeof(uio_buf));
	uio_addiov(auio, uap->buffer, uap->buffersize);

	  
                                                                       
                                                                       
                             
    
	action = KAUTH_VNODE_LIST_DIRECTORY;
	if ((attributelist.commonattr & ~ATTR_CMN_NAME) ||
	    attributelist.fileattr || attributelist.dirattr) {
		action |= KAUTH_VNODE_SEARCH;
	}

	if ((error = vnode_authorize(vp, NULL, action, ctx)) == 0) {
		                                                            
                                                

		error = VNOP_READDIRATTR(vp, &attributelist, auio, count,
		    (uint32_t)uap->options, &newstate, &eofflag, &count, ctx);
	}

	if (error) {
		(void) vnode_put(vp);
		goto out;
	}

	  
                                                               
                                                                  
                                                                      
                                                                
               
    
	if (eofflag && vp->v_mount->mnt_flag & MNT_UNION) {
		if (uio_resid(auio) < (user_ssize_t) uap->buffersize) {                    
			eofflag = 0;
		} else {                                                               
			struct vnode *tvp = vp;
			if (lookup_traverse_union(tvp, &vp, ctx) == 0) {
				vnode_ref_ext(vp, fp->fp_glob->fg_flag & O_EVTONLY, 0);
				fp->fp_glob->fg_data = (caddr_t) vp;
				fp->fp_glob->fg_offset = 0;                           
				count = savecount;
				vnode_rele_internal(tvp, fp->fp_glob->fg_flag & O_EVTONLY, 0, 0);
				vnode_put(tvp);
				goto unionread;
			}
			vp = tvp;
		}
	}

	(void)vnode_put(vp);

	if (error) {
		goto out;
	}
	fp->fp_glob->fg_offset = uio_offset(auio);                                                 

	if ((error = copyout((caddr_t) &count, uap->count, sizeof(count)))) {
		goto out;
	}
	if ((error = copyout((caddr_t) &newstate, uap->newstate, sizeof(newstate)))) {
		goto out;
	}
	if ((error = copyout((caddr_t) &loff, uap->basep, sizeof(loff)))) {
		goto out;
	}

	*retval = eofflag;                                
	error = 0;
out:
	file_drop(fd);
	return error;                                                    
}                                           

  
                                  
   

              
int
exchangedata(__unused proc_t p, struct exchangedata_args *uap, __unused int32_t *retval)
{
	struct nameidata fnd, snd;
	vfs_context_t ctx = vfs_context_current();
	vnode_t fvp;
	vnode_t svp;
	int error;
	u_int32_t nameiflags;
	char *fpath = NULL;
	char *spath = NULL;
	int   flen = 0, slen = 0;
	int from_truncated = 0, to_truncated = 0;
#if CONFIG_FSE
	fse_info f_finfo, s_finfo;
#endif

	nameiflags = 0;
	if ((uap->options & FSOPT_NOFOLLOW) == 0) {
		nameiflags |= FOLLOW;
	}

	NDINIT(&fnd, LOOKUP, OP_EXCHANGEDATA, nameiflags | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path1, ctx);

	error = namei(&fnd);
	if (error) {
		goto out2;
	}

	nameidone(&fnd);
	fvp = fnd.ni_vp;

	NDINIT(&snd, LOOKUP, OP_EXCHANGEDATA, CN_NBMOUNTLOOK | nameiflags | AUDITVNPATH2,
	    UIO_USERSPACE, uap->path2, ctx);

	error = namei(&snd);
	if (error) {
		vnode_put(fvp);
		goto out2;
	}
	nameidone(&snd);
	svp = snd.ni_vp;

	  
                                                    
    
	if (svp == fvp) {
		error = EINVAL;
		goto out;
	}

	  
                                                          
    
	if (svp->v_mount != fvp->v_mount) {
		error = EXDEV;
		goto out;
	}

	                                           
	if ((vnode_isreg(fvp) == 0) || (vnode_isreg(svp) == 0)) {
		error = EINVAL;
		goto out;
	}

#if CONFIG_MACF
	error = mac_vnode_check_exchangedata(ctx,
	    fvp, svp);
	if (error) {
		goto out;
	}
#endif
	if (((error = vnode_authorize(fvp, NULL, KAUTH_VNODE_READ_DATA | KAUTH_VNODE_WRITE_DATA, ctx)) != 0) ||
	    ((error = vnode_authorize(svp, NULL, KAUTH_VNODE_READ_DATA | KAUTH_VNODE_WRITE_DATA, ctx)) != 0)) {
		goto out;
	}

	if (
#if CONFIG_FSE
		need_fsevent(FSE_EXCHANGE, fvp) ||
#endif
		kauth_authorize_fileop_has_listeners()) {
		GET_PATH(fpath);
		GET_PATH(spath);

		flen = safe_getpath(fvp, NULL, fpath, MAXPATHLEN, &from_truncated);
		slen = safe_getpath(svp, NULL, spath, MAXPATHLEN, &to_truncated);

#if CONFIG_FSE
		get_fse_info(fvp, &f_finfo, ctx);
		get_fse_info(svp, &s_finfo, ctx);
		if (from_truncated || to_truncated) {
			                                                                    
			f_finfo.mode |= FSE_TRUNCATED_PATH;
		}
#endif
	}
	                       
	error = VNOP_EXCHANGE(fvp, svp, 0, ctx);

	if (error == 0) {
		const char *tmpname;

		if (fpath != NULL && spath != NULL) {
			                                                            
                                                   
      
			kauth_authorize_fileop(vfs_context_ucred(ctx), KAUTH_FILEOP_EXCHANGE,
			    (uintptr_t)fpath, (uintptr_t)spath);
		}
		name_cache_lock();

		tmpname     = fvp->v_name;
		fvp->v_name = svp->v_name;
		svp->v_name = tmpname;

		if (fvp->v_parent != svp->v_parent) {
			vnode_t tmp;

			tmp           = fvp->v_parent;
			fvp->v_parent = svp->v_parent;
			svp->v_parent = tmp;
		}
		name_cache_unlock();

#if CONFIG_FSE
		if (fpath != NULL && spath != NULL) {
			add_fsevent(FSE_EXCHANGE, ctx,
			    FSE_ARG_STRING, flen, fpath,
			    FSE_ARG_FINFO, &f_finfo,
			    FSE_ARG_STRING, slen, spath,
			    FSE_ARG_FINFO, &s_finfo,
			    FSE_ARG_DONE);
		}
#endif
	}

out:
	if (fpath != NULL) {
		RELEASE_PATH(fpath);
	}
	if (spath != NULL) {
		RELEASE_PATH(spath);
	}
	vnode_put(svp);
	vnode_put(fvp);
out2:
	return error;
}

  
                                                                      
   
uint32_t freespace_mb(vnode_t vp);

uint32_t
freespace_mb(vnode_t vp)
{
	vfs_update_vfsstat(vp->v_mount, vfs_context_current(), VFS_USER_EVENT);
	return (uint32_t)(((uint64_t)vp->v_mount->mnt_vfsstat.f_bavail *
	       vp->v_mount->mnt_vfsstat.f_bsize) >> 20);
}

#if CONFIG_SEARCHFS

              

int
searchfs(proc_t p, struct searchfs_args *uap, __unused int32_t *retval)
{
	vnode_t vp, tvp;
	int i, error = 0;
	int fserror = 0;
	struct nameidata nd;
	struct user64_fssearchblock searchblock;
	struct searchstate *state;
	struct attrlist *returnattrs;
	struct timeval timelimit;
	void *searchparams1, *searchparams2;
	uio_t auio = NULL;
	int spacetype = proc_is64bit(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	uint32_t nummatches;
	size_t mallocsize;
	uint32_t nameiflags;
	vfs_context_t ctx = vfs_context_current();
	char uio_buf[UIO_SIZEOF(1)];

	                                                     
	if (IS_64BIT_PROCESS(p)) {
		error = copyin(uap->searchblock, (caddr_t) &searchblock, sizeof(searchblock));
		timelimit.tv_sec = searchblock.timelimit.tv_sec;
		timelimit.tv_usec = searchblock.timelimit.tv_usec;
	} else {
		struct user32_fssearchblock tmp_searchblock;

		error = copyin(uap->searchblock, (caddr_t) &tmp_searchblock, sizeof(tmp_searchblock));
		                            
		searchblock.returnattrs = CAST_USER_ADDR_T(tmp_searchblock.returnattrs);
		searchblock.returnbuffer = CAST_USER_ADDR_T(tmp_searchblock.returnbuffer);
		searchblock.returnbuffersize = tmp_searchblock.returnbuffersize;
		searchblock.maxmatches = tmp_searchblock.maxmatches;
		  
                                                                                     
                                                                    
     
		timelimit.tv_sec = (__darwin_time_t) tmp_searchblock.timelimit.tv_sec;
		timelimit.tv_usec = (__darwin_useconds_t) tmp_searchblock.timelimit.tv_usec;
		searchblock.searchparams1 = CAST_USER_ADDR_T(tmp_searchblock.searchparams1);
		searchblock.sizeofsearchparams1 = tmp_searchblock.sizeofsearchparams1;
		searchblock.searchparams2 = CAST_USER_ADDR_T(tmp_searchblock.searchparams2);
		searchblock.sizeofsearchparams2 = tmp_searchblock.sizeofsearchparams2;
		searchblock.searchattrs = tmp_searchblock.searchattrs;
	}
	if (error) {
		return error;
	}

	                                                                    
    
	if (searchblock.sizeofsearchparams1 > SEARCHFS_MAX_SEARCHPARMS ||
	    searchblock.sizeofsearchparams2 > SEARCHFS_MAX_SEARCHPARMS) {
		return EINVAL;
	}

	                                                                                                        
	                                                                                                        
	                                                                                                        
	                                                                                                        
	                      
	                                                                                                        
	                                                                                                        
	                                                                                 

	mallocsize = searchblock.sizeofsearchparams1 + searchblock.sizeofsearchparams2 +
	    sizeof(struct attrlist) + sizeof(struct searchstate) + (2 * sizeof(uint32_t));

	searchparams1 = kheap_alloc(KHEAP_DATA_BUFFERS, mallocsize, Z_WAITOK);

	                                                                                        

	searchparams2 = (void *) (((caddr_t) searchparams1) + searchblock.sizeofsearchparams1);
	returnattrs = (struct attrlist *) (((caddr_t) searchparams2) + searchblock.sizeofsearchparams2);
	state = (struct searchstate *) (((caddr_t) returnattrs) + sizeof(struct attrlist));

	                                                      

	if ((error = copyin(searchblock.searchparams1, searchparams1, searchblock.sizeofsearchparams1))) {
		goto freeandexit;
	}

	if ((error = copyin(searchblock.searchparams2, searchparams2, searchblock.sizeofsearchparams2))) {
		goto freeandexit;
	}

	if ((error = copyin(searchblock.returnattrs, (caddr_t) returnattrs, sizeof(struct attrlist)))) {
		goto freeandexit;
	}

	if ((error = copyin(uap->state, (caddr_t) state, sizeof(struct searchstate)))) {
		goto freeandexit;
	}

	  
                                                 
                                                 
                                   
    
	if (uap->options & SRCHFS_START) {
		state->ss_union_layer = 0;
	} else {
		uap->options |= state->ss_union_flags;
	}
	state->ss_union_flags = 0;

	  
                                                                                          
                                                                                            
                                                                                              
                                                                                     
                                                                                    
    

	if (searchblock.searchattrs.commonattr & ATTR_CMN_NAME) {
		attrreference_t* string_ref;
		u_int32_t* start_length;
		user64_size_t param_length;

		                            
		param_length = searchblock.sizeofsearchparams1;
		                                                       
		start_length = (u_int32_t*) searchparams1;
		start_length = start_length + 1;
		string_ref = (attrreference_t*) start_length;

		                                                   
		if (string_ref->attr_dataoffset < 0) {
			error = EINVAL;
			goto freeandexit;
		}
		if (string_ref->attr_length > MAXPATHLEN) {
			error = EINVAL;
			goto freeandexit;
		}

		                                                  
		if (((char*) string_ref + string_ref->attr_dataoffset) < (char*) string_ref) {
			error = EINVAL;
			goto freeandexit;
		}

		if (((char*) string_ref + string_ref->attr_dataoffset) > ((char*)searchparams1 + param_length)) {
			error = EINVAL;
			goto freeandexit;
		}
		if (((char*)string_ref + string_ref->attr_dataoffset + string_ref->attr_length) > ((char*)searchparams1 + param_length)) {
			error = EINVAL;
			goto freeandexit;
		}
	}

	                                                                         
	auio = uio_createwithbuffer(1, 0, spacetype, UIO_READ, &uio_buf[0], sizeof(uio_buf));
	uio_addiov(auio, searchblock.returnbuffer, searchblock.returnbuffersize);

	nameiflags = 0;
	if ((uap->options & FSOPT_NOFOLLOW) == 0) {
		nameiflags |= FOLLOW;
	}
	NDINIT(&nd, LOOKUP, OP_SEARCHFS, nameiflags | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);

	error = namei(&nd);
	if (error) {
		goto freeandexit;
	}
	vp = nd.ni_vp;
	nameidone(&nd);

	  
                                           
    
	error = VFS_ROOT(vnode_mount(vp), &tvp, ctx);
	vnode_put(vp);
	if (error) {
		goto freeandexit;
	}
	vp = tvp;

	  
                                                
                                                   
                                                    
                   
    
	for (i = 0; i < (int) state->ss_union_layer; i++) {
		if ((vp->v_mount->mnt_flag & MNT_UNION) == 0) {
			break;
		}
		tvp = vp;
		vp = vp->v_mount->mnt_vnodecovered;
		if (vp == NULL) {
			vnode_put(tvp);
			error = ENOENT;
			goto freeandexit;
		}
		error = vnode_getwithref(vp);
		vnode_put(tvp);
		if (error) {
			goto freeandexit;
		}
	}

#if CONFIG_MACF
	error = mac_vnode_check_searchfs(ctx, vp, returnattrs, &searchblock.searchattrs);
	if (error) {
		vnode_put(vp);
		goto freeandexit;
	}
#endif


	  
                                                                           
                                                                      
    
	if (searchblock.maxmatches == 0) {
		nummatches = 0;
		goto saveandexit;
	}

	  
                                                                 
   
                                                                   
                                                                  
                                   
    

	fserror = VNOP_SEARCHFS(vp,
	    searchparams1,
	    searchparams2,
	    &searchblock.searchattrs,
	    (uint32_t)searchblock.maxmatches,
	    &timelimit,
	    returnattrs,
	    &nummatches,
	    (uint32_t)uap->scriptcode,
	    (uint32_t)uap->options,
	    auio,
	    (struct searchstate *) &state->ss_fsstate,
	    ctx);

	  
                                                    
                                        
    
	if ((vp->v_mount->mnt_flag & MNT_UNION) && fserror == 0) {
		state->ss_union_flags = SRCHFS_START;
		state->ss_union_layer++;                                 
		fserror = EAGAIN;
	}

saveandexit:

	vnode_put(vp);

	                                                                                       
                                                                                        

	if ((error = copyout((caddr_t) state, uap->state, sizeof(struct searchstate))) != 0) {
		goto freeandexit;
	}

	if ((error = suulong(uap->nummatches, (uint64_t)nummatches)) != 0) {
		goto freeandexit;
	}

	error = fserror;

freeandexit:

	kheap_free(KHEAP_DATA_BUFFERS, searchparams1, mallocsize);

	return error;
}                                  

#else                      

int
searchfs(__unused proc_t p, __unused struct searchfs_args *uap, __unused int32_t *retval)
{
	return ENOTSUP;
}

#endif                      


#if CONFIG_DATALESS_FILES

  
                                               
  
                                                                      
                                                                        
                                                          
  
                                                                       
                                                                   
  
                                                                         
                                                                         
                                      
  
                                                                         
                                                                       
                                        
  
                                                                            
                                                                           
                                                                    
                                                                      
                                                                          
                                                                     
                                                                       
                                      
  
                                                                            
                                                                         
                                                                             
  
                                                                            
                                                                      
                                                                        
                                                                           
                                                                        
                                                                        
                                                                   
                                                            
  
                                                                             
                                                                        
                                                                           
                                                                             
                                                                            
                                                                      
                                                                            
                                                                          
                             
  
                                                                           
                                                                            
                                                
   

struct nspace_resolver_request {
	LIST_ENTRY(nspace_resolver_request) r_hashlink;
	vnode_t         r_vp;
	uint32_t        r_req_id;
	int             r_resolver_error;
	int             r_flags;
};

#define RRF_COMPLETE    0x0001

static uint32_t
next_nspace_req_id(void)
{
	static uint32_t next_req_id;

	return OSAddAtomic(1, &next_req_id);
}

#define NSPACE_RESOLVER_REQ_HASHSIZE    32                    
#define NSPACE_RESOLVER_MAX_OUTSTANDING 256                   

static LIST_HEAD(nspace_resolver_requesthead,
    nspace_resolver_request) * nspace_resolver_request_hashtbl;
static u_long nspace_resolver_request_hashmask;
static u_int nspace_resolver_request_count;
static bool nspace_resolver_request_wait_slot;
static LCK_GRP_DECLARE(nspace_resolver_request_lck_grp, "file namespace resolver");
static LCK_MTX_DECLARE(nspace_resolver_request_hash_mutex,
    &nspace_resolver_request_lck_grp);

#define NSPACE_REQ_LOCK() \
	lck_mtx_lock(&nspace_resolver_request_hash_mutex)
#define NSPACE_REQ_UNLOCK() \
	lck_mtx_unlock(&nspace_resolver_request_hash_mutex)

#define NSPACE_RESOLVER_HASH(req_id)    \
	(&nspace_resolver_request_hashtbl[(req_id) & \
	 nspace_resolver_request_hashmask])

static struct nspace_resolver_request *
nspace_resolver_req_lookup(uint32_t req_id)
{
	struct nspace_resolver_requesthead *bucket;
	struct nspace_resolver_request *req;

	bucket = NSPACE_RESOLVER_HASH(req_id);
	LIST_FOREACH(req, bucket, r_hashlink) {
		if (req->r_req_id == req_id) {
			return req;
		}
	}

	return NULL;
}

static int
nspace_resolver_req_add(struct nspace_resolver_request *req)
{
	struct nspace_resolver_requesthead *bucket;
	int error;

	while (nspace_resolver_request_count >=
	    NSPACE_RESOLVER_MAX_OUTSTANDING) {
		nspace_resolver_request_wait_slot = true;
		error = msleep(&nspace_resolver_request_count,
		    &nspace_resolver_request_hash_mutex,
		    PVFS | PCATCH, "nspacerq", NULL);
		if (error) {
			return error;
		}
	}

	bucket = NSPACE_RESOLVER_HASH(req->r_req_id);
#if DIAGNOSTIC
	assert(nspace_resolver_req_lookup(req->r_req_id) == NULL);
#endif                 
	LIST_INSERT_HEAD(bucket, req, r_hashlink);
	nspace_resolver_request_count++;

	return 0;
}

static void
nspace_resolver_req_remove(struct nspace_resolver_request *req)
{
	struct nspace_resolver_requesthead *bucket;

	bucket = NSPACE_RESOLVER_HASH(req->r_req_id);
#if DIAGNOSTIC
	assert(nspace_resolver_req_lookup(req->r_req_id) != NULL);
#endif                 
	LIST_REMOVE(req, r_hashlink);
	nspace_resolver_request_count--;

	if (nspace_resolver_request_wait_slot) {
		nspace_resolver_request_wait_slot = false;
		wakeup(&nspace_resolver_request_count);
	}
}

static void
nspace_resolver_req_cancel(uint32_t req_id)
{
	kern_return_t kr;
	mach_port_t mp;

	                                                         
	                                           

	kr = host_get_filecoordinationd_port(host_priv_self(), &mp);
	if (kr != KERN_SUCCESS || !IPC_PORT_VALID(mp)) {
		return;
	}

	kr = send_nspace_resolve_cancel(mp, req_id);
	if (kr != KERN_SUCCESS) {
		os_log_error(OS_LOG_DEFAULT,
		    "NSPACE send_nspace_resolve_cancel failure: %d", kr);
	}

	ipc_port_release_send(mp);
}

static int
nspace_resolver_req_wait(struct nspace_resolver_request *req)
{
	bool send_cancel_message = false;
	int error;

	NSPACE_REQ_LOCK();

	while ((req->r_flags & RRF_COMPLETE) == 0) {
		error = msleep(req, &nspace_resolver_request_hash_mutex,
		    PVFS | PCATCH, "nspace", NULL);
		if (error && error != ERESTART) {
			req->r_resolver_error = (error == EINTR) ? EINTR :
			    ETIMEDOUT;
			send_cancel_message = true;
			break;
		}
	}

	nspace_resolver_req_remove(req);

	NSPACE_REQ_UNLOCK();

	if (send_cancel_message) {
		nspace_resolver_req_cancel(req->r_req_id);
	}

	return req->r_resolver_error;
}

static void
nspace_resolver_req_mark_complete(
	struct nspace_resolver_request *req,
	int resolver_error)
{
	req->r_resolver_error = resolver_error;
	req->r_flags |= RRF_COMPLETE;
	wakeup(req);
}

static void
nspace_resolver_req_completed(uint32_t req_id, int resolver_error, uint64_t orig_gencount)
{
	struct nspace_resolver_request *req;

	NSPACE_REQ_LOCK();

	                                                            
	                                                            
	                                                

	req = nspace_resolver_req_lookup(req_id);
	if (req) {
		mount_t locked_mp = NULL;

		locked_mp = req->r_vp->v_mount;
		mount_ref(locked_mp, 0);
		mount_lock_renames(locked_mp);

		  
		                                                                  
		                                                                    
		                                                  
		  
		                                                                  
		                                                                
		  
		if (resolver_error == 0 &&
		    orig_gencount != 0 &&
		    vnode_getwithref(req->r_vp) == 0) {
			struct vnode_attr va;
			uint64_t cur_gencount;

			VATTR_INIT(&va);
			VATTR_WANTED(&va, va_recursive_gencount);

			if (vnode_getattr(req->r_vp, &va, vfs_context_kernel()) == 0) {
				cur_gencount = va.va_recursive_gencount;
			} else {
				cur_gencount = 0;
			}

			if (resolver_error == 0 && cur_gencount && orig_gencount && cur_gencount != orig_gencount) {
				printf("nspace.complete: gencount changed! (orig %llu cur %llu)\n", orig_gencount, cur_gencount);

				                                                               
				                                
				resolver_error = EBUSY;

				                                                                 
				                                                                  
				                  
			}

			vnode_put(req->r_vp);
		}

		mount_unlock_renames(locked_mp);
		mount_drop(locked_mp, 0);

		nspace_resolver_req_mark_complete(req, resolver_error);
	}

	NSPACE_REQ_UNLOCK();

	return;
}

static struct proc *nspace_resolver_proc;

static int
nspace_resolver_get_proc_state(struct proc *p, int *is_resolver)
{
	*is_resolver = ((p->p_lflag & P_LNSPACE_RESOLVER) &&
	    p == nspace_resolver_proc) ? 1 : 0;
	return 0;
}

static int
nspace_resolver_set_proc_state(struct proc *p, int is_resolver)
{
	vfs_context_t ctx = vfs_context_current();
	int error = 0;

	  
	                                                            
	                                                              
	                            
	  
	if (!vfs_context_issuser(ctx)) {
		return EPERM;
	}

	error = priv_check_cred(vfs_context_ucred(ctx),
	    PRIV_VFS_DATALESS_RESOLVER, 0);
	if (error) {
		return error;
	}

	if (is_resolver) {
		NSPACE_REQ_LOCK();

		if (nspace_resolver_proc == NULL) {
			proc_lock(p);
			p->p_lflag |= P_LNSPACE_RESOLVER;
			proc_unlock(p);
			nspace_resolver_proc = p;
		} else {
			error = EBUSY;
		}

		NSPACE_REQ_UNLOCK();
	} else {
		                                             
		                                                
		                                              
		          
		nspace_resolver_exited(p);
	}

	return error;
}

static int
nspace_materialization_get_proc_state(struct proc *p, int *is_prevented)
{
	if ((p->p_lflag & P_LNSPACE_RESOLVER) != 0 ||
	    (p->p_vfs_iopolicy &
	    P_VFS_IOPOLICY_MATERIALIZE_DATALESS_FILES) == 0) {
		*is_prevented = 1;
	} else {
		*is_prevented = 0;
	}
	return 0;
}

static int
nspace_materialization_set_proc_state(struct proc *p, int is_prevented)
{
	if (p->p_lflag & P_LNSPACE_RESOLVER) {
		return is_prevented ? 0 : EBUSY;
	}

	if (is_prevented) {
		OSBitAndAtomic16(~((uint16_t)P_VFS_IOPOLICY_MATERIALIZE_DATALESS_FILES), &p->p_vfs_iopolicy);
	} else {
		OSBitOrAtomic16((uint16_t)P_VFS_IOPOLICY_MATERIALIZE_DATALESS_FILES, &p->p_vfs_iopolicy);
	}
	return 0;
}

static int
nspace_materialization_get_thread_state(int *is_prevented)
{
	uthread_t ut = get_bsdthread_info(current_thread());

	*is_prevented = (ut->uu_flag & UT_NSPACE_NODATALESSFAULTS) ? 1 : 0;
	return 0;
}

static int
nspace_materialization_set_thread_state(int is_prevented)
{
	uthread_t ut = get_bsdthread_info(current_thread());

	if (is_prevented) {
		ut->uu_flag |= UT_NSPACE_NODATALESSFAULTS;
	} else {
		ut->uu_flag &= ~UT_NSPACE_NODATALESSFAULTS;
	}
	return 0;
}

                           
SYSCTL_NODE(_vfs, OID_AUTO, nspace, CTLFLAG_RW | CTLFLAG_LOCKED, NULL, "vfs nspace hinge");

static int
sysctl_nspace_resolver(__unused struct sysctl_oid *oidp,
    __unused void *arg1, __unused int arg2, struct sysctl_req *req)
{
	struct proc *p = req->p;
	int new_value, old_value, changed = 0;
	int error;

	error = nspace_resolver_get_proc_state(p, &old_value);
	if (error) {
		return error;
	}

	error = sysctl_io_number(req, old_value, sizeof(int), &new_value,
	    &changed);
	if (error == 0 && changed) {
		error = nspace_resolver_set_proc_state(p, new_value);
	}
	return error;
}

                                                         
SYSCTL_PROC(_vfs_nspace, OID_AUTO, resolver,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_LOCKED,
    0, 0, sysctl_nspace_resolver, "I", "");

static int
sysctl_nspace_prevent_materialization(__unused struct sysctl_oid *oidp,
    __unused void *arg1, __unused int arg2, struct sysctl_req *req)
{
	struct proc *p = req->p;
	int new_value, old_value, changed = 0;
	int error;

	error = nspace_materialization_get_proc_state(p, &old_value);
	if (error) {
		return error;
	}

	error = sysctl_io_number(req, old_value, sizeof(int), &new_value,
	    &changed);
	if (error == 0 && changed) {
		error = nspace_materialization_set_proc_state(p, new_value);
	}
	return error;
}

                                                                        
SYSCTL_PROC(_vfs_nspace, OID_AUTO, prevent_materialization,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_LOCKED,
    0, 0, sysctl_nspace_prevent_materialization, "I", "");

static int
sysctl_nspace_thread_prevent_materialization(__unused struct sysctl_oid *oidp,
    __unused void *arg1, __unused int arg2, struct sysctl_req *req)
{
	int new_value, old_value, changed = 0;
	int error;

	error = nspace_materialization_get_thread_state(&old_value);
	if (error) {
		return error;
	}

	error = sysctl_io_number(req, old_value, sizeof(int), &new_value,
	    &changed);
	if (error == 0 && changed) {
		error = nspace_materialization_set_thread_state(new_value);
	}
	return error;
}

                                                                       
SYSCTL_PROC(_vfs_nspace, OID_AUTO, thread_prevent_materialization,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_LOCKED,
    0, 0, sysctl_nspace_thread_prevent_materialization, "I", "");

static int
sysctl_nspace_complete(__unused struct sysctl_oid *oidp, __unused void *arg1,
    __unused int arg2, struct sysctl_req *req)
{
	struct proc *p = req->p;
	uint32_t req_status[2] = { 0, 0 };
	uint64_t gencount = 0;
	int error, is_resolver, changed = 0, gencount_changed;

	error = nspace_resolver_get_proc_state(p, &is_resolver);
	if (error) {
		return error;
	}

	if (!is_resolver) {
		return EPERM;
	}

	error = sysctl_io_opaque(req, req_status, sizeof(req_status),
	    &changed);
	if (error) {
		return error;
	}

	                                    
	error = sysctl_io_opaque(req, &gencount, sizeof(gencount),
	    &gencount_changed);
	if (error) {
		gencount = 0;
		                                                        
		error = 0;
	}

	  
                               
   
                              
    
	if (error == 0 && changed) {
		nspace_resolver_req_completed(req_status[0],
		    (int)req_status[1], gencount);
	}
	return error;
}

                                           
SYSCTL_PROC(_vfs_nspace, OID_AUTO, complete,
    CTLTYPE_OPAQUE | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_LOCKED,
    0, 0, sysctl_nspace_complete, "-", "");

#endif                            

#if CONFIG_DATALESS_FILES
#define __no_dataless_unused                 
#else
#define __no_dataless_unused    __unused
#endif

int
vfs_context_dataless_materialization_is_prevented(
	vfs_context_t const ctx __no_dataless_unused)
{
#if CONFIG_DATALESS_FILES
	proc_t const p = vfs_context_proc(ctx);
	thread_t const t = vfs_context_thread(ctx);
	uthread_t const ut = t ? get_bsdthread_info(t) : NULL;

	  
                                                                  
                                        
    
	if (ctx == vfs_context_kernel()) {
		return EDEADLK;
	}

	  
                                                             
                                                           
                                                             
                           
    
	if (vfs_context_is_dataless_manipulator(ctx)) {
		return EJUSTRETURN;
	}

	  
                                                                 
                                                                
                                                                     
    
	if (ut != NULL) {
		if (ut->uu_flag & UT_NSPACE_NODATALESSFAULTS) {
			return EDEADLK;
		}
		if (ut->uu_flag & UT_NSPACE_FORCEDATALESSFAULTS) {
			return 0;
		}
	}

	  
                                                           
                                                 
    
	if (p->p_vfs_iopolicy & P_VFS_IOPOLICY_MATERIALIZE_DATALESS_FILES) {
		return 0;
	}
#endif                            

	  
                                                              
                                                    
    
	return EDEADLK;
}

void
nspace_resolver_init(void)
{
#if CONFIG_DATALESS_FILES
	nspace_resolver_request_hashtbl =
	    hashinit(NSPACE_RESOLVER_REQ_HASHSIZE,
	    M_VNODE          , &nspace_resolver_request_hashmask);
#endif                            
}

void
nspace_resolver_exited(struct proc *p __no_dataless_unused)
{
#if CONFIG_DATALESS_FILES
	struct nspace_resolver_requesthead *bucket;
	struct nspace_resolver_request *req;
	u_long idx;

	NSPACE_REQ_LOCK();

	if ((p->p_lflag & P_LNSPACE_RESOLVER) &&
	    p == nspace_resolver_proc) {
		for (idx = 0; idx <= nspace_resolver_request_hashmask; idx++) {
			bucket = &nspace_resolver_request_hashtbl[idx];
			LIST_FOREACH(req, bucket, r_hashlink) {
				nspace_resolver_req_mark_complete(req,
				    ETIMEDOUT);
			}
		}
		nspace_resolver_proc = NULL;
	}

	NSPACE_REQ_UNLOCK();
#endif                            
}

int
resolve_nspace_item(struct vnode *vp, uint64_t op)
{
	return resolve_nspace_item_ext(vp, op, NULL);
}

#define DATALESS_RESOLVER_ENTITLEMENT     \
	"com.apple.private.vfs.dataless-resolver"
#define DATALESS_MANIPULATION_ENTITLEMENT \
	"com.apple.private.vfs.dataless-manipulation"

  
                                                                       
                             
  
                                                                 
                                             
   
boolean_t
vfs_context_is_dataless_manipulator(vfs_context_t ctx __unused)
{
#if CONFIG_DATALESS_FILES
	assert(ctx->vc_thread == current_thread());
	task_t const task = current_task();
	return IOTaskHasEntitlement(task, DATALESS_MANIPULATION_ENTITLEMENT) ||
	       IOTaskHasEntitlement(task, DATALESS_RESOLVER_ENTITLEMENT);
#else
	return false;
#endif                            
}

int
resolve_nspace_item_ext(
	struct vnode *vp __no_dataless_unused,
	uint64_t op __no_dataless_unused,
	void *arg __unused)
{
#if CONFIG_DATALESS_FILES
	int error;
	mach_port_t mp;
	char *path = NULL;
	int path_len;
	kern_return_t kr;
	struct nspace_resolver_request req;

	                                                                          
	if (vp->v_type != VREG && vp->v_type != VDIR && vp->v_type != VLNK) {
		return EFTYPE;
	}

	  
	                                                    
	                                                     
	                                                     
	                                                    
	                                                      
	                                                   
	                                                    
	                                                   
	                                                 
	  
	if (op & NAMESPACE_HANDLER_SNAPSHOT_EVENT) {
		os_log_debug(OS_LOG_DEFAULT, "NSPACE SNAPSHOT not handled");
		return ENOTSUP;
	}

	error = vfs_context_dataless_materialization_is_prevented(
		vfs_context_current());
	if (error) {
		os_log_debug(OS_LOG_DEFAULT,
		    "NSPACE process/thread is decorated as no-materialization");
		return error;
	}

	kr = host_get_filecoordinationd_port(host_priv_self(), &mp);
	if (kr != KERN_SUCCESS || !IPC_PORT_VALID(mp)) {
		os_log_error(OS_LOG_DEFAULT, "NSPACE no port");
		                                                     
		                
		return ETIMEDOUT;
	}

	path = zalloc(ZV_NAMEI);
	path_len = MAXPATHLEN;

	error = vn_getpath(vp, path, &path_len);
	if (error == 0) {
		int xxx_rdar44371223;                    
		req.r_req_id = next_nspace_req_id();
		req.r_resolver_error = 0;
		req.r_flags = 0;

		if ((error = vnode_ref(vp)) == 0) {                                                    
			req.r_vp = vp;
		} else {
			goto out_release_port;
		}

		NSPACE_REQ_LOCK();
		error = nspace_resolver_req_add(&req);
		NSPACE_REQ_UNLOCK();
		if (error) {
			vnode_rele(req.r_vp);
			goto out_release_port;
		}

		os_log_debug(OS_LOG_DEFAULT, "NSPACE resolve_path call");
		kr = send_nspace_resolve_path(mp, req.r_req_id,
		    current_proc()->p_pid, (uint32_t)(op & 0xffffffff),
		    path, &xxx_rdar44371223);
		if (kr != KERN_SUCCESS) {
			                                              
			                            
			os_log_error(OS_LOG_DEFAULT,
			    "NSPACE resolve_path failure: %d", kr);
			error = ETIMEDOUT;

			NSPACE_REQ_LOCK();
			nspace_resolver_req_remove(&req);
			NSPACE_REQ_UNLOCK();
			vnode_rele(req.r_vp);
			goto out_release_port;
		}

		                                                  
		                                 
		zfree(ZV_NAMEI, path);
		path = NULL;

		                                              
		                                           
		                                                  
		                         
		error = nspace_resolver_req_wait(&req);

		vnode_rele(req.r_vp);
	}

out_release_port:
	if (path != NULL) {
		zfree(ZV_NAMEI, path);
	}
	ipc_port_release_send(mp);

	return error;
#else
	return ENOTSUP;
#endif                            
}

int
nspace_snapshot_event(__unused vnode_t vp, __unused  time_t ctime,
    __unused uint64_t op_type, __unused void *arg)
{
	return 0;
}

#if 0
static int
build_volfs_path(struct vnode *vp, char *path, int *len)
{
	struct vnode_attr va;
	int ret;

	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_fsid);
	VATTR_WANTED(&va, va_fileid);

	if (vnode_getattr(vp, &va, vfs_context_kernel()) != 0) {
		*len = snprintf(path, *len, "/non/existent/path/because/vnode_getattr/failed") + 1;
		ret = -1;
	} else {
		*len = snprintf(path, *len, "/.vol/%d/%lld", (dev_t)va.va_fsid, va.va_fileid) + 1;
		ret = 0;
	}

	return ret;
}
#endif

static unsigned long
fsctl_bogus_command_compat(unsigned long cmd)
{
	switch (cmd) {
	case IOCBASECMD(FSIOC_SYNC_VOLUME):
		return FSIOC_SYNC_VOLUME;
	case IOCBASECMD(FSIOC_ROUTEFS_SETROUTEID):
		return FSIOC_ROUTEFS_SETROUTEID;
	case IOCBASECMD(FSIOC_SET_PACKAGE_EXTS):
		return FSIOC_SET_PACKAGE_EXTS;
	case IOCBASECMD(FSIOC_SET_FSTYPENAME_OVERRIDE):
		return FSIOC_SET_FSTYPENAME_OVERRIDE;
	case IOCBASECMD(DISK_CONDITIONER_IOC_GET):
		return DISK_CONDITIONER_IOC_GET;
	case IOCBASECMD(DISK_CONDITIONER_IOC_SET):
		return DISK_CONDITIONER_IOC_SET;
	case IOCBASECMD(FSIOC_FIOSEEKHOLE):
		return FSIOC_FIOSEEKHOLE;
	case IOCBASECMD(FSIOC_FIOSEEKDATA):
		return FSIOC_FIOSEEKDATA;
	case IOCBASECMD(SPOTLIGHT_IOC_GET_MOUNT_TIME):
		return SPOTLIGHT_IOC_GET_MOUNT_TIME;
	case IOCBASECMD(SPOTLIGHT_IOC_GET_LAST_MTIME):
		return SPOTLIGHT_IOC_GET_LAST_MTIME;
	}

	return cmd;
}

static int
cas_bsdflags_setattr(vnode_t vp, void *arg, vfs_context_t ctx)
{
	return VNOP_IOCTL(vp, FSIOC_CAS_BSDFLAGS, arg, FWRITE, ctx);
}

static int __attribute__((noinline))
handle_sync_volume(vnode_t vp, vnode_t *arg_vp, caddr_t data, vfs_context_t ctx)
{
	struct vfs_attr vfa;
	mount_t mp = vp->v_mount;
	unsigned arg;
	int error;

	                                               
	uint32_t vvid = vp->v_id;

	  
                                                             
                                                              
                                                                
    
	error = mount_iterref(mp, 0);
	if (error) {
		return error;
	}
	vnode_put(vp);

	arg = MNT_NOWAIT;
	if (*(uint32_t*)data & FSCTL_SYNC_WAIT) {
		arg = MNT_WAIT;
	}

	  
                                                       
                                                           
                                                    
    
	VFSATTR_INIT(&vfa);
	VFSATTR_WANTED(&vfa, f_capabilities);
	if ((vfs_getattr(mp, &vfa, vfs_context_current()) == 0) &&
	    VFSATTR_IS_SUPPORTED(&vfa, f_capabilities) &&
	    ((vfa.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_SHARED_SPACE)) &&
	    ((vfa.f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_SHARED_SPACE))) {
		arg |= MNT_VOLUME;
	}

	                                    
	(void)sync_callback(mp, &arg);

	  
                                                                    
                                   
    
	mount_iterdrop(mp);

	if (arg & FSCTL_SYNC_FULLSYNC) {
		                                                         
		error = vnode_getwithvid(vp, vvid);
		if (error == 0) {
			error = VNOP_IOCTL(vp, F_FULLFSYNC, (caddr_t)NULL, 0, ctx);
			vnode_put(vp);
		}
	}
	                                                  
	*arg_vp = NULL;
	return error;
}

#if ROUTEFS
static int __attribute__((noinline))
handle_routes(user_addr_t udata)
{
	char routepath[MAXPATHLEN];
	size_t len = 0;
	int error;

	if ((error = suser(kauth_cred_get(), &(current_proc()->p_acflag)))) {
		return error;
	}
	bzero(routepath, MAXPATHLEN);
	error = copyinstr(udata, &routepath[0], MAXPATHLEN, &len);
	if (error) {
		return error;
	}
	error = routefs_kernel_mount(routepath);
	return error;
}
#endif

static int __attribute__((noinline))
handle_flags(vnode_t vp, caddr_t data, vfs_context_t ctx)
{
	struct fsioc_cas_bsdflags *cas = (struct fsioc_cas_bsdflags *)data;
	struct vnode_attr va;
	int error;

	VATTR_INIT(&va);
	VATTR_SET(&va, va_flags, cas->new_flags);

	error = chflags0(vp, &va, cas_bsdflags_setattr, cas, ctx);
	return error;
}

static int __attribute__((noinline))
handle_auth(vnode_t vp, u_long cmd, caddr_t data, u_long options, vfs_context_t ctx)
{
	struct mount *mp = NULL;
	errno_t rootauth = 0;

	mp = vp->v_mount;

	  
                                                           
                                                       
                                                      
    
	rootauth = VNOP_IOCTL(vp, cmd, data, (int)options, ctx);

	return rootauth;
}

  
                                           
   
              
static int
fsctl_internal(proc_t p, vnode_t *arg_vp, u_long cmd, user_addr_t udata, u_long options, vfs_context_t ctx)
{
	int error = 0;
	boolean_t is64bit;
	u_int size;
#define STK_PARAMS 128
	char stkbuf[STK_PARAMS] = {0};
	caddr_t data, memp;
	vnode_t vp = *arg_vp;

	if (vp->v_type == VCHR || vp->v_type == VBLK) {
		return ENOTTY;
	}

	cmd = fsctl_bogus_command_compat(cmd);

	size = IOCPARM_LEN(cmd);
	if (size > IOCPARM_MAX) {
		return EINVAL;
	}

	is64bit = proc_is64bit(p);

	memp = NULL;

	if (size > sizeof(stkbuf)) {
		if ((memp = (caddr_t)kheap_alloc(KHEAP_TEMP, size, Z_WAITOK)) == 0) {
			return ENOMEM;
		}
		data = memp;
	} else {
		data = &stkbuf[0];
	};

	if (cmd & IOC_IN) {
		if (size) {
			error = copyin(udata, data, size);
			if (error) {
				if (memp) {
					kheap_free(KHEAP_TEMP, memp, size);
				}
				return error;
			}
		} else {
			if (is64bit) {
				*(user_addr_t *)data = udata;
			} else {
				*(uint32_t *)data = (uint32_t)udata;
			}
		};
	} else if ((cmd & IOC_OUT) && size) {
		  
                                       
                                       
     
		bzero(data, size);
	} else if (cmd & IOC_VOID) {
		if (is64bit) {
			*(user_addr_t *)data = udata;
		} else {
			*(uint32_t *)data = (uint32_t)udata;
		}
	}

	                                            
	switch (cmd) {
	case FSIOC_SYNC_VOLUME:
		error = handle_sync_volume(vp, arg_vp, data, ctx);
		break;

	case FSIOC_ROUTEFS_SETROUTEID:
#if ROUTEFS
		error = handle_routes(udata);
#endif
		break;

	case FSIOC_SET_PACKAGE_EXTS: {
		user_addr_t ext_strings;
		uint32_t    num_entries;
		uint32_t    max_width;

		if ((error = priv_check_cred(kauth_cred_get(), PRIV_PACKAGE_EXTENSIONS, 0))) {
			break;
		}

		if ((is64bit && size != sizeof(user64_package_ext_info))
		    || (is64bit == 0 && size != sizeof(user32_package_ext_info))) {
			                                                     
			                                                       
			               
			error = EINVAL;
			break;
		}

		if (is64bit) {
			if (sizeof(user64_addr_t) > sizeof(user_addr_t)) {
				assert(((user64_package_ext_info *)data)->strings <= UINT32_MAX);
			}
			ext_strings = (user_addr_t)((user64_package_ext_info *)data)->strings;
			num_entries = ((user64_package_ext_info *)data)->num_entries;
			max_width   = ((user64_package_ext_info *)data)->max_width;
		} else {
			ext_strings = CAST_USER_ADDR_T(((user32_package_ext_info *)data)->strings);
			num_entries = ((user32_package_ext_info *)data)->num_entries;
			max_width   = ((user32_package_ext_info *)data)->max_width;
		}
		error = set_package_extensions_table(ext_strings, num_entries, max_width);
	}
	break;

	case FSIOC_SET_FSTYPENAME_OVERRIDE:
	{
		if ((error = suser(kauth_cred_get(), &(current_proc()->p_acflag)))) {
			break;
		}
		if (vp->v_mount) {
			mount_lock(vp->v_mount);
			if (data[0] != 0) {
				int i;
				for (i = 0; i < MFSTYPENAMELEN; i++) {
					if (!data[i]) {
						goto continue_copy;
					}
				}
				  
                                                                 
                                                          
                                                               
                       
       
				error = EINVAL;
				goto unlock;
continue_copy:
				strlcpy(&vp->v_mount->fstypename_override[0], data, MFSTYPENAMELEN);
				vp->v_mount->mnt_kern_flag |= MNTK_TYPENAME_OVERRIDE;
				if (vfs_isrdonly(vp->v_mount) && strcmp(vp->v_mount->fstypename_override, "mtmfs") == 0) {
					vp->v_mount->mnt_kern_flag |= MNTK_EXTENDED_SECURITY;
					vp->v_mount->mnt_kern_flag &= ~MNTK_AUTH_OPAQUE;
				}
			} else {
				if (strcmp(vp->v_mount->fstypename_override, "mtmfs") == 0) {
					vp->v_mount->mnt_kern_flag &= ~MNTK_EXTENDED_SECURITY;
				}
				vp->v_mount->mnt_kern_flag &= ~MNTK_TYPENAME_OVERRIDE;
				vp->v_mount->fstypename_override[0] = '\0';
			}
unlock:
			mount_unlock(vp->v_mount);
		}
	}
	break;

	case DISK_CONDITIONER_IOC_GET: {
		error = disk_conditioner_get_info(vp->v_mount, (disk_conditioner_info *)data);
	}
	break;

	case DISK_CONDITIONER_IOC_SET: {
		error = disk_conditioner_set_info(vp->v_mount, (disk_conditioner_info *)data);
	}
	break;

	case FSIOC_CAS_BSDFLAGS:
		error = handle_flags(vp, data, ctx);
		break;

	case FSIOC_FD_ONLY_OPEN_ONCE: {
		error = 0;
		if (vnode_usecount(vp) > 1) {
			vnode_lock_spin(vp);
			if (vp->v_lflag & VL_HASSTREAMS) {
				if (vnode_isinuse_locked(vp, 1, 1)) {
					error = EBUSY;
				}
			} else if (vnode_usecount(vp) > 1) {
				error = EBUSY;
			}
			vnode_unlock(vp);
		}
	}
	break;

	case FSIOC_EVAL_ROOTAUTH:
		error = handle_auth(vp, cmd, data, options, ctx);
		break;

	default: {
		                                                         
		switch (cmd) {
		case F_PUNCHHOLE:
		case F_TRIM_ACTIVE_FILE:
		case F_RDADVISE:
		case F_TRANSCODEKEY:
		case F_GETPROTECTIONLEVEL:
		case F_GETDEFAULTPROTLEVEL:
		case F_MAKECOMPRESSED:
		case F_SET_GREEDY_MODE:
		case F_SETSTATICCONTENT:
		case F_SETIOTYPE:
		case F_SETBACKINGSTORE:
		case F_GETPATH_MTMINFO:
		case APFSIOC_REVERT_TO_SNAPSHOT:
		case FSIOC_FIOSEEKHOLE:
		case FSIOC_FIOSEEKDATA:
		case HFS_GET_BOOT_INFO:
		case HFS_SET_BOOT_INFO:
		case FIOPINSWAP:
		case F_CHKCLEAN:
		case F_FULLFSYNC:
		case F_BARRIERFSYNC:
		case F_FREEZE_FS:
		case F_THAW_FS:
		case FSIOC_KERNEL_ROOTAUTH:
			error = EINVAL;
			goto outdrop;
		}
		                                         
		error = VNOP_IOCTL(vp, cmd, data, (int)options, ctx);
	}
	}                      

	  
                                                 
                                  
    
	if (error == 0 && (cmd & IOC_OUT) && size) {
		error = copyout(data, udata, size);
	}

outdrop:
	if (memp) {
		kheap_free(KHEAP_TEMP, memp, size);
	}

	return error;
}

              
int
fsctl(proc_t p, struct fsctl_args *uap, __unused int32_t *retval)
{
	int error;
	struct nameidata nd;
	uint32_t nameiflags;
	vnode_t vp = NULL;
	vfs_context_t ctx = vfs_context_current();

	AUDIT_ARG(cmd, (int)uap->cmd);
	AUDIT_ARG(value32, uap->options);
	                                                         
	nameiflags = 0;
	  
	                                                                      
	                                                                        
	                                                                       
	                               
	  
	if (uap->cmd == FSIOC_FD_ONLY_OPEN_ONCE) {
		return EINVAL;
	}
	if ((uap->options & FSOPT_NOFOLLOW) == 0) {
		nameiflags |= FOLLOW;
	}
	if (uap->cmd == FSIOC_FIRMLINK_CTL) {
		nameiflags |= (CN_FIRMLINK_NOFOLLOW | NOCACHE);
	}
	NDINIT(&nd, LOOKUP, OP_FSCTL, nameiflags | AUDITVNPATH1,
	    UIO_USERSPACE, uap->path, ctx);
	if ((error = namei(&nd))) {
		goto done;
	}
	vp = nd.ni_vp;
	nameidone(&nd);

#if CONFIG_MACF
	error = mac_mount_check_fsctl(ctx, vnode_mount(vp), uap->cmd);
	if (error) {
		goto done;
	}
#endif

	error = fsctl_internal(p, &vp, uap->cmd, (user_addr_t)uap->data, uap->options, ctx);

done:
	if (vp) {
		vnode_put(vp);
	}
	return error;
}
              
int
ffsctl(proc_t p, struct ffsctl_args *uap, __unused int32_t *retval)
{
	int error;
	vnode_t vp = NULL;
	vfs_context_t ctx = vfs_context_current();
	int fd = -1;

	AUDIT_ARG(fd, uap->fd);
	AUDIT_ARG(cmd, (int)uap->cmd);
	AUDIT_ARG(value32, uap->options);

	                                                         
	if ((error = file_vnode(uap->fd, &vp))) {
		return error;
	}
	fd = uap->fd;
	if ((error = vnode_getwithref(vp))) {
		file_drop(fd);
		return error;
	}

#if CONFIG_MACF
	if ((error = mac_mount_check_fsctl(ctx, vnode_mount(vp), uap->cmd))) {
		file_drop(fd);
		vnode_put(vp);
		return error;
	}
#endif

	error = fsctl_internal(p, &vp, uap->cmd, (user_addr_t)uap->data, uap->options, ctx);

	file_drop(fd);

	                                                                       
	if (vp) {
		vnode_put(vp);
	}

	return error;
}
                              

#define FILESEC_ACCESS_ENTITLEMENT              \
	"com.apple.private.vfs.filesec-access"

static int
xattr_entitlement_check(const char *attrname, vfs_context_t ctx, bool setting)
{
	if (strcmp(attrname, KAUTH_FILESEC_XATTR) == 0) {
		  
                                                         
                                                     
     
		if ((!setting && vfs_context_issuser(ctx)) ||
		    IOTaskHasEntitlement(current_task(),
		    FILESEC_ACCESS_ENTITLEMENT)) {
			return 0;
		}
	}

	return EPERM;
}

  
                                               
   
int
getxattr(proc_t p, struct getxattr_args *uap, user_ssize_t *retval)
{
	vnode_t vp;
	struct nameidata nd;
	char attrname[XATTR_MAXNAMELEN + 1];
	vfs_context_t ctx = vfs_context_current();
	uio_t auio = NULL;
	int spacetype = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	size_t attrsize = 0;
	size_t namelen;
	u_int32_t nameiflags;
	int error;
	char uio_buf[UIO_SIZEOF(1)];

	if (uap->options & (XATTR_NOSECURITY | XATTR_NODEFAULT)) {
		return EINVAL;
	}

	nameiflags = (uap->options & XATTR_NOFOLLOW) ? 0 : FOLLOW;
	NDINIT(&nd, LOOKUP, OP_GETXATTR, nameiflags, spacetype, uap->path, ctx);
	if ((error = namei(&nd))) {
		return error;
	}
	vp = nd.ni_vp;
	nameidone(&nd);

	error = copyinstr(uap->attrname, attrname, sizeof(attrname), &namelen);
	if (error != 0) {
		goto out;
	}
	if (xattr_protected(attrname) &&
	    (error = xattr_entitlement_check(attrname, ctx, false)) != 0) {
		goto out;
	}
	  
                                                           
                                                                 
                                                              
                                                                     
                                                                  
                                                                    
                                                                  
                                                                 
                                                                    
                                                               
                                                                    
                                                                
                                                                     
                                                                    
                     
   
                                                                  
                                                   
                                                       
    
	if (uap->size == 0xffffffff || uap->size == (size_t)-1) {
		goto no_uio;
	}

	if (uap->value) {
		if (uap->size > (size_t)XATTR_MAXSIZE) {
			uap->size = XATTR_MAXSIZE;
		}

		auio = uio_createwithbuffer(1, uap->position, spacetype, UIO_READ,
		    &uio_buf[0], sizeof(uio_buf));
		uio_addiov(auio, uap->value, uap->size);
	}
no_uio:
	error = vn_getxattr(vp, attrname, auio, &attrsize, uap->options, ctx);
out:
	vnode_put(vp);

	if (auio) {
		*retval = uap->size - uio_resid(auio);
	} else {
		*retval = (user_ssize_t)attrsize;
	}

	return error;
}

  
                                              
   
int
fgetxattr(proc_t p, struct fgetxattr_args *uap, user_ssize_t *retval)
{
	vnode_t vp;
	char attrname[XATTR_MAXNAMELEN + 1];
	vfs_context_t ctx = vfs_context_current();
	uio_t auio = NULL;
	int spacetype = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	size_t attrsize = 0;
	size_t namelen;
	int error;
	char uio_buf[UIO_SIZEOF(1)];

	if (uap->options & (XATTR_NOFOLLOW | XATTR_NOSECURITY | XATTR_NODEFAULT)) {
		return EINVAL;
	}

	if ((error = file_vnode(uap->fd, &vp))) {
		return error;
	}
	if ((error = vnode_getwithref(vp))) {
		file_drop(uap->fd);
		return error;
	}
	error = copyinstr(uap->attrname, attrname, sizeof(attrname), &namelen);
	if (error != 0) {
		goto out;
	}
	if (xattr_protected(attrname) &&
	    (error = xattr_entitlement_check(attrname, ctx, false)) != 0) {
		goto out;
	}
	if (uap->value && uap->size > 0) {
		auio = uio_createwithbuffer(1, uap->position, spacetype, UIO_READ,
		    &uio_buf[0], sizeof(uio_buf));
		uio_addiov(auio, uap->value, uap->size);
	}

	error = vn_getxattr(vp, attrname, auio, &attrsize, uap->options, vfs_context_current());
out:
	(void)vnode_put(vp);
	file_drop(uap->fd);

	if (auio) {
		*retval = uap->size - uio_resid(auio);
	} else {
		*retval = (user_ssize_t)attrsize;
	}
	return error;
}

  
                                         
   
int
setxattr(proc_t p, struct setxattr_args *uap, int *retval)
{
	vnode_t vp;
	struct nameidata nd;
	char attrname[XATTR_MAXNAMELEN + 1];
	vfs_context_t ctx = vfs_context_current();
	uio_t auio = NULL;
	int spacetype = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	size_t namelen;
	u_int32_t nameiflags;
	int error;
	char uio_buf[UIO_SIZEOF(1)];

	if (uap->options & (XATTR_NOSECURITY | XATTR_NODEFAULT)) {
		return EINVAL;
	}

	error = copyinstr(uap->attrname, attrname, sizeof(attrname), &namelen);
	if (error != 0) {
		if (error == EPERM) {
			                                                                
			return ENAMETOOLONG;
		}
		                                                                             
		return error;
	}
	if (xattr_protected(attrname) &&
	    (error = xattr_entitlement_check(attrname, ctx, true)) != 0) {
		return error;
	}
	if (uap->size != 0 && uap->value == 0) {
		return EINVAL;
	}
	if (uap->size > INT_MAX) {
		return E2BIG;
	}

	nameiflags = (uap->options & XATTR_NOFOLLOW) ? 0 : FOLLOW;
	NDINIT(&nd, LOOKUP, OP_SETXATTR, nameiflags, spacetype, uap->path, ctx);
	if ((error = namei(&nd))) {
		return error;
	}
	vp = nd.ni_vp;
	nameidone(&nd);

	auio = uio_createwithbuffer(1, uap->position, spacetype, UIO_WRITE,
	    &uio_buf[0], sizeof(uio_buf));
	uio_addiov(auio, uap->value, uap->size);

	error = vn_setxattr(vp, attrname, auio, uap->options, ctx);
#if CONFIG_FSE
	if (error == 0) {
		add_fsevent(FSE_XATTR_MODIFIED, ctx,
		    FSE_ARG_VNODE, vp,
		    FSE_ARG_DONE);
	}
#endif
	vnode_put(vp);
	*retval = 0;
	return error;
}

  
                                         
   
int
fsetxattr(proc_t p, struct fsetxattr_args *uap, int *retval)
{
	vnode_t vp;
	char attrname[XATTR_MAXNAMELEN + 1];
	vfs_context_t ctx = vfs_context_current();
	uio_t auio = NULL;
	int spacetype = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	size_t namelen;
	int error;
	char uio_buf[UIO_SIZEOF(1)];

	if (uap->options & (XATTR_NOFOLLOW | XATTR_NOSECURITY | XATTR_NODEFAULT)) {
		return EINVAL;
	}

	error = copyinstr(uap->attrname, attrname, sizeof(attrname), &namelen);
	if (error != 0) {
		if (error == EPERM) {
			                                                                
			return ENAMETOOLONG;
		}
		                                                                             
		return error;
	}
	if (xattr_protected(attrname) &&
	    (error = xattr_entitlement_check(attrname, ctx, true)) != 0) {
		return error;
	}
	if (uap->size != 0 && uap->value == 0) {
		return EINVAL;
	}
	if (uap->size > INT_MAX) {
		return E2BIG;
	}
	if ((error = file_vnode(uap->fd, &vp))) {
		return error;
	}
	if ((error = vnode_getwithref(vp))) {
		file_drop(uap->fd);
		return error;
	}
	auio = uio_createwithbuffer(1, uap->position, spacetype, UIO_WRITE,
	    &uio_buf[0], sizeof(uio_buf));
	uio_addiov(auio, uap->value, uap->size);

	error = vn_setxattr(vp, attrname, auio, uap->options, vfs_context_current());
#if CONFIG_FSE
	if (error == 0) {
		add_fsevent(FSE_XATTR_MODIFIED, ctx,
		    FSE_ARG_VNODE, vp,
		    FSE_ARG_DONE);
	}
#endif
	vnode_put(vp);
	file_drop(uap->fd);
	*retval = 0;
	return error;
}

  
                                
                             
   
int
removexattr(proc_t p, struct removexattr_args *uap, int *retval)
{
	vnode_t vp;
	struct nameidata nd;
	char attrname[XATTR_MAXNAMELEN + 1];
	int spacetype = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	vfs_context_t ctx = vfs_context_current();
	size_t namelen;
	u_int32_t nameiflags;
	int error;

	if (uap->options & (XATTR_NOSECURITY | XATTR_NODEFAULT)) {
		return EINVAL;
	}

	error = copyinstr(uap->attrname, attrname, sizeof(attrname), &namelen);
	if (error != 0) {
		return error;
	}
	if (xattr_protected(attrname)) {
		return EPERM;
	}
	nameiflags = (uap->options & XATTR_NOFOLLOW) ? 0 : FOLLOW;
	NDINIT(&nd, LOOKUP, OP_REMOVEXATTR, nameiflags, spacetype, uap->path, ctx);
	if ((error = namei(&nd))) {
		return error;
	}
	vp = nd.ni_vp;
	nameidone(&nd);

	error = vn_removexattr(vp, attrname, uap->options, ctx);
#if CONFIG_FSE
	if (error == 0) {
		add_fsevent(FSE_XATTR_REMOVED, ctx,
		    FSE_ARG_VNODE, vp,
		    FSE_ARG_DONE);
	}
#endif
	vnode_put(vp);
	*retval = 0;
	return error;
}

  
                                
                             
   
int
fremovexattr(__unused proc_t p, struct fremovexattr_args *uap, int *retval)
{
	vnode_t vp;
	char attrname[XATTR_MAXNAMELEN + 1];
	size_t namelen;
	int error;
#if CONFIG_FSE
	vfs_context_t ctx = vfs_context_current();
#endif

	if (uap->options & (XATTR_NOFOLLOW | XATTR_NOSECURITY | XATTR_NODEFAULT)) {
		return EINVAL;
	}

	error = copyinstr(uap->attrname, attrname, sizeof(attrname), &namelen);
	if (error != 0) {
		return error;
	}
	if (xattr_protected(attrname)) {
		return EPERM;
	}
	if ((error = file_vnode(uap->fd, &vp))) {
		return error;
	}
	if ((error = vnode_getwithref(vp))) {
		file_drop(uap->fd);
		return error;
	}

	error = vn_removexattr(vp, attrname, uap->options, vfs_context_current());
#if CONFIG_FSE
	if (error == 0) {
		add_fsevent(FSE_XATTR_REMOVED, ctx,
		    FSE_ARG_VNODE, vp,
		    FSE_ARG_DONE);
	}
#endif
	vnode_put(vp);
	file_drop(uap->fd);
	*retval = 0;
	return error;
}

  
                                                 
                             
   
int
listxattr(proc_t p, struct listxattr_args *uap, user_ssize_t *retval)
{
	vnode_t vp;
	struct nameidata nd;
	vfs_context_t ctx = vfs_context_current();
	uio_t auio = NULL;
	int spacetype = IS_64BIT_PROCESS(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	size_t attrsize = 0;
	u_int32_t nameiflags;
	int error;
	char uio_buf[UIO_SIZEOF(1)];

	if (uap->options & (XATTR_NOSECURITY | XATTR_NODEFAULT)) {
		return EINVAL;
	}

	nameiflags = (uap->options & XATTR_NOFOLLOW) ? 0 : FOLLOW;
	NDINIT(&nd, LOOKUP, OP_LISTXATTR, nameiflags, spacetype, uap->path, ctx);
	if ((error = namei(&nd))) {
		return error;
	}
	vp = nd.ni_vp;
	nameidone(&nd);
	if (uap->namebuf != 0 && uap->bufsize > 0) {
		auio = uio_createwithbuffer(1, 0, spacetype, UIO_READ,
		    &uio_buf[0], sizeof(uio_buf));
		uio_addiov(auio, uap->namebuf, uap->bufsize);
	}

	error = vn_listxattr(vp, auio, &attrsize, uap->options, ctx);

	vnode_put(vp);
	if (auio) {
		*retval = (user_ssize_t)uap->bufsize - uio_resid(auio);
	} else {
		*retval = (user_ssize_t)attrsize;
	}
	return error;
}

  
                                                 
                             
   
int
flistxattr(proc_t p, struct flistxattr_args *uap, user_ssize_t *retval)
{
	vnode_t vp;
	uio_t auio = NULL;
	int spacetype = proc_is64bit(p) ? UIO_USERSPACE64 : UIO_USERSPACE32;
	size_t attrsize = 0;
	int error;
	char uio_buf[UIO_SIZEOF(1)];

	if (uap->options & (XATTR_NOFOLLOW | XATTR_NOSECURITY | XATTR_NODEFAULT)) {
		return EINVAL;
	}

	if ((error = file_vnode(uap->fd, &vp))) {
		return error;
	}
	if ((error = vnode_getwithref(vp))) {
		file_drop(uap->fd);
		return error;
	}
	if (uap->namebuf != 0 && uap->bufsize > 0) {
		auio = uio_createwithbuffer(1, 0, spacetype,
		    UIO_READ, &uio_buf[0], sizeof(uio_buf));
		uio_addiov(auio, uap->namebuf, uap->bufsize);
	}

	error = vn_listxattr(vp, auio, &attrsize, uap->options, vfs_context_current());

	vnode_put(vp);
	file_drop(uap->fd);
	if (auio) {
		*retval = (user_ssize_t)uap->bufsize - uio_resid(auio);
	} else {
		*retval = (user_ssize_t)attrsize;
	}
	return error;
}

static int
fsgetpath_internal(vfs_context_t ctx, int volfs_id, uint64_t objid,
    vm_size_t bufsize, caddr_t buf, uint32_t options, int *pathlen)
{
	int error;
	struct mount *mp = NULL;
	vnode_t vp;
	int length;
	int bpflags;
	                                                 
	unsigned int retries = 0x10;

	if (bufsize > PAGE_SIZE) {
		return EINVAL;
	}

	if (buf == NULL) {
		return ENOMEM;
	}

retry:
	if ((mp = mount_lookupby_volfsid(volfs_id, 1)) == NULL) {
		error = ENOTSUP;                          
		return ENOTSUP;
	}

unionget:
	if (objid == 2) {
		struct vfs_attr vfsattr;
		int use_vfs_root = TRUE;

		VFSATTR_INIT(&vfsattr);
		VFSATTR_WANTED(&vfsattr, f_capabilities);
		if (!(options & FSOPT_ISREALFSID) &&
		    vfs_getattr(mp, &vfsattr, vfs_context_kernel()) == 0 &&
		    VFSATTR_IS_SUPPORTED(&vfsattr, f_capabilities)) {
			if ((vfsattr.f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_VOL_GROUPS) &&
			    (vfsattr.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_VOL_GROUPS)) {
				use_vfs_root = FALSE;
			}
		}

		if (use_vfs_root) {
			error = VFS_ROOT(mp, &vp, ctx);
		} else {
			error = VFS_VGET(mp, objid, &vp, ctx);
		}
	} else {
		error = VFS_VGET(mp, (ino64_t)objid, &vp, ctx);
	}

	if (error == ENOENT && (mp->mnt_flag & MNT_UNION)) {
		  
                                                   
                                                   
                       
     
		struct mount *tmp = mp;
		mp = vnode_mount(tmp->mnt_vnodecovered);
		vfs_unbusy(tmp);
		if (vfs_busy(mp, LK_NOWAIT) == 0) {
			goto unionget;
		}
	} else {
		vfs_unbusy(mp);
	}

	if (error) {
		return error;
	}

#if CONFIG_MACF
	error = mac_vnode_check_fsgetpath(ctx, vp);
	if (error) {
		vnode_put(vp);
		return error;
	}
#endif

	                                             
	bpflags = vfs_context_suser(ctx) ? BUILDPATH_CHECKACCESS : 0;
	if (options & FSOPT_NOFIRMLINKPATH) {
		bpflags |= BUILDPATH_NO_FIRMLINK;
	}
	bpflags |= BUILDPATH_CHECK_MOVED;
	error = build_path(vp, buf, (int)bufsize, &length, bpflags, ctx);
	vnode_put(vp);

	if (error) {
		                                                              
		if (error == EAGAIN) {
			--retries;
			if (retries > 0) {
				goto retry;
			}

			error = ENOENT;
		}
		goto out;
	}

	AUDIT_ARG(text, buf);

	if (kdebug_debugid_enabled(VFS_LOOKUP) && length > 0) {
		unsigned long path_words[NUMPARMS];
		size_t path_len = sizeof(path_words);

		if ((size_t)length < path_len) {
			memcpy((char *)path_words, buf, length);
			memset((char *)path_words + length, 0, path_len - length);

			path_len = length;
		} else {
			memcpy((char *)path_words, buf + (length - path_len), path_len);
		}

		kdebug_vfs_lookup(path_words, (int)path_len, vp,
		    KDBG_VFS_LOOKUP_FLAG_LOOKUP);
	}

	*pathlen = length;                                 

out:
	return error;
}

  
                                                          
   
static int
fsgetpath_extended(user_addr_t buf, user_size_t bufsize, user_addr_t user_fsid, uint64_t objid,
    uint32_t options, user_ssize_t *retval)
{
	vfs_context_t ctx = vfs_context_current();
	fsid_t fsid;
	char *realpath;
	int length;
	int error;

	if (options & ~(FSOPT_NOFIRMLINKPATH | FSOPT_ISREALFSID)) {
		return EINVAL;
	}

	if ((error = copyin(user_fsid, (caddr_t)&fsid, sizeof(fsid)))) {
		return error;
	}
	AUDIT_ARG(value32, fsid.val[0]);
	AUDIT_ARG(value64, objid);
	                                          

	if (bufsize > PAGE_SIZE || bufsize <= 0) {
		return EINVAL;
	}
	realpath = kheap_alloc(KHEAP_TEMP, bufsize, Z_WAITOK | Z_ZERO);
	if (realpath == NULL) {
		return ENOMEM;
	}

	error = fsgetpath_internal(ctx, fsid.val[0], objid, bufsize, realpath,
	    options, &length);

	if (error) {
		goto out;
	}

	error = copyout((caddr_t)realpath, buf, length);

	*retval = (user_ssize_t)length;                                 
out:
	kheap_free(KHEAP_TEMP, realpath, bufsize);
	return error;
}

int
fsgetpath(__unused proc_t p, struct fsgetpath_args *uap, user_ssize_t *retval)
{
	return fsgetpath_extended(uap->buf, uap->bufsize, uap->fsid, uap->objid,
	           0, retval);
}

int
fsgetpath_ext(__unused proc_t p, struct fsgetpath_ext_args *uap, user_ssize_t *retval)
{
	return fsgetpath_extended(uap->buf, uap->bufsize, uap->fsid, uap->objid,
	           uap->options, retval);
}

  
                                                                      
                 
  
                       
          
   
static int
munge_statfs(struct mount *mp, struct vfsstatfs *sfsp,
    user_addr_t bufp, int *sizep, boolean_t is_64_bit,
    boolean_t partial_copy)
{
	int             error;
	int             my_size, copy_size;

	if (is_64_bit) {
		struct user64_statfs sfs;
		my_size = copy_size = sizeof(sfs);
		bzero(&sfs, my_size);
		sfs.f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
		sfs.f_type = (short)mp->mnt_vtable->vfc_typenum;
		sfs.f_reserved1 = (short)sfsp->f_fssubtype;
		sfs.f_bsize = (user64_long_t)sfsp->f_bsize;
		sfs.f_iosize = (user64_long_t)sfsp->f_iosize;
		sfs.f_blocks = (user64_long_t)sfsp->f_blocks;
		sfs.f_bfree = (user64_long_t)sfsp->f_bfree;
		sfs.f_bavail = (user64_long_t)sfsp->f_bavail;
		sfs.f_files = (user64_long_t)sfsp->f_files;
		sfs.f_ffree = (user64_long_t)sfsp->f_ffree;
		sfs.f_fsid = sfsp->f_fsid;
		sfs.f_owner = sfsp->f_owner;
		if (mp->mnt_kern_flag & MNTK_TYPENAME_OVERRIDE) {
			strlcpy(&sfs.f_fstypename[0], &mp->fstypename_override[0], MFSNAMELEN);
		} else {
			strlcpy(&sfs.f_fstypename[0], &sfsp->f_fstypename[0], MFSNAMELEN);
		}
		strlcpy(&sfs.f_mntonname[0], &sfsp->f_mntonname[0], MNAMELEN);
		strlcpy(&sfs.f_mntfromname[0], &sfsp->f_mntfromname[0], MNAMELEN);

		if (partial_copy) {
			copy_size -= (sizeof(sfs.f_reserved3) + sizeof(sfs.f_reserved4));
		}
		error = copyout((caddr_t)&sfs, bufp, copy_size);
	} else {
		struct user32_statfs sfs;

		my_size = copy_size = sizeof(sfs);
		bzero(&sfs, my_size);

		sfs.f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
		sfs.f_type = (short)mp->mnt_vtable->vfc_typenum;
		sfs.f_reserved1 = (short)sfsp->f_fssubtype;

		  
                                                                                  
                                                                                     
                                                   
     
		if ((sfsp->f_blocks > INT_MAX)
		                                                               
                                                                 
                                                  
                                                                 
                                                           
                                                              
                                                              
                                                      
         
		    && (sfsp->f_blocks != 0xffffffffffffffffULL)
		    && (sfsp->f_bfree != 0xffffffffffffffffULL)
		    && (sfsp->f_bavail != 0xffffffffffffffffULL)) {
			int             shift;

			  
                                                                            
                                                                        
                                                                         
                                
     
                                                                          
                                 
      
			for (shift = 0; shift < 32; shift++) {
				if ((sfsp->f_blocks >> shift) <= INT_MAX) {
					break;
				}
				if ((sfsp->f_bsize << (shift + 1)) > INT_MAX) {
					break;
				}
			}
#define __SHIFT_OR_CLIP(x, s)   ((((x) >> (s)) > INT_MAX) ? INT_MAX : ((x) >> (s)))
			sfs.f_blocks = (user32_long_t)__SHIFT_OR_CLIP(sfsp->f_blocks, shift);
			sfs.f_bfree = (user32_long_t)__SHIFT_OR_CLIP(sfsp->f_bfree, shift);
			sfs.f_bavail = (user32_long_t)__SHIFT_OR_CLIP(sfsp->f_bavail, shift);
#undef __SHIFT_OR_CLIP
			sfs.f_bsize = (user32_long_t)(sfsp->f_bsize << shift);
			sfs.f_iosize = (int)lmax(sfsp->f_iosize, sfsp->f_bsize);
		} else {
			                                                        
			sfs.f_bsize = (user32_long_t)sfsp->f_bsize;
			sfs.f_iosize = (user32_long_t)sfsp->f_iosize;
			sfs.f_blocks = (user32_long_t)sfsp->f_blocks;
			sfs.f_bfree = (user32_long_t)sfsp->f_bfree;
			sfs.f_bavail = (user32_long_t)sfsp->f_bavail;
		}
		sfs.f_files = (user32_long_t)sfsp->f_files;
		sfs.f_ffree = (user32_long_t)sfsp->f_ffree;
		sfs.f_fsid = sfsp->f_fsid;
		sfs.f_owner = sfsp->f_owner;
		if (mp->mnt_kern_flag & MNTK_TYPENAME_OVERRIDE) {
			strlcpy(&sfs.f_fstypename[0], &mp->fstypename_override[0], MFSNAMELEN);
		} else {
			strlcpy(&sfs.f_fstypename[0], &sfsp->f_fstypename[0], MFSNAMELEN);
		}
		strlcpy(&sfs.f_mntonname[0], &sfsp->f_mntonname[0], MNAMELEN);
		strlcpy(&sfs.f_mntfromname[0], &sfsp->f_mntfromname[0], MNAMELEN);

		if (partial_copy) {
			copy_size -= (sizeof(sfs.f_reserved3) + sizeof(sfs.f_reserved4));
		}
		error = copyout((caddr_t)&sfs, bufp, copy_size);
	}

	if (sizep != NULL) {
		*sizep = my_size;
	}
	return error;
}

  
                                                
   
void
munge_user64_stat(struct stat *sbp, struct user64_stat *usbp)
{
	bzero(usbp, sizeof(*usbp));

	usbp->st_dev = sbp->st_dev;
	usbp->st_ino = sbp->st_ino;
	usbp->st_mode = sbp->st_mode;
	usbp->st_nlink = sbp->st_nlink;
	usbp->st_uid = sbp->st_uid;
	usbp->st_gid = sbp->st_gid;
	usbp->st_rdev = sbp->st_rdev;
#ifndef _POSIX_C_SOURCE
	usbp->st_atimespec.tv_sec = sbp->st_atimespec.tv_sec;
	usbp->st_atimespec.tv_nsec = sbp->st_atimespec.tv_nsec;
	usbp->st_mtimespec.tv_sec = sbp->st_mtimespec.tv_sec;
	usbp->st_mtimespec.tv_nsec = sbp->st_mtimespec.tv_nsec;
	usbp->st_ctimespec.tv_sec = sbp->st_ctimespec.tv_sec;
	usbp->st_ctimespec.tv_nsec = sbp->st_ctimespec.tv_nsec;
#else
	usbp->st_atime = sbp->st_atime;
	usbp->st_atimensec = sbp->st_atimensec;
	usbp->st_mtime = sbp->st_mtime;
	usbp->st_mtimensec = sbp->st_mtimensec;
	usbp->st_ctime = sbp->st_ctime;
	usbp->st_ctimensec = sbp->st_ctimensec;
#endif
	usbp->st_size = sbp->st_size;
	usbp->st_blocks = sbp->st_blocks;
	usbp->st_blksize = sbp->st_blksize;
	usbp->st_flags = sbp->st_flags;
	usbp->st_gen = sbp->st_gen;
	usbp->st_lspare = sbp->st_lspare;
	usbp->st_qspare[0] = sbp->st_qspare[0];
	usbp->st_qspare[1] = sbp->st_qspare[1];
}

void
munge_user32_stat(struct stat *sbp, struct user32_stat *usbp)
{
	bzero(usbp, sizeof(*usbp));

	usbp->st_dev = sbp->st_dev;
	usbp->st_ino = sbp->st_ino;
	usbp->st_mode = sbp->st_mode;
	usbp->st_nlink = sbp->st_nlink;
	usbp->st_uid = sbp->st_uid;
	usbp->st_gid = sbp->st_gid;
	usbp->st_rdev = sbp->st_rdev;
#ifndef _POSIX_C_SOURCE
	usbp->st_atimespec.tv_sec = (user32_time_t)sbp->st_atimespec.tv_sec;
	usbp->st_atimespec.tv_nsec = (user32_long_t)sbp->st_atimespec.tv_nsec;
	usbp->st_mtimespec.tv_sec = (user32_time_t)sbp->st_mtimespec.tv_sec;
	usbp->st_mtimespec.tv_nsec = (user32_long_t)sbp->st_mtimespec.tv_nsec;
	usbp->st_ctimespec.tv_sec = (user32_time_t)sbp->st_ctimespec.tv_sec;
	usbp->st_ctimespec.tv_nsec = (user32_long_t)sbp->st_ctimespec.tv_nsec;
#else
	usbp->st_atime = sbp->st_atime;
	usbp->st_atimensec = sbp->st_atimensec;
	usbp->st_mtime = sbp->st_mtime;
	usbp->st_mtimensec = sbp->st_mtimensec;
	usbp->st_ctime = sbp->st_ctime;
	usbp->st_ctimensec = sbp->st_ctimensec;
#endif
	usbp->st_size = sbp->st_size;
	usbp->st_blocks = sbp->st_blocks;
	usbp->st_blksize = sbp->st_blksize;
	usbp->st_flags = sbp->st_flags;
	usbp->st_gen = sbp->st_gen;
	usbp->st_lspare = sbp->st_lspare;
	usbp->st_qspare[0] = sbp->st_qspare[0];
	usbp->st_qspare[1] = sbp->st_qspare[1];
}

  
                                                    
   
void
munge_user64_stat64(struct stat64 *sbp, struct user64_stat64 *usbp)
{
	bzero(usbp, sizeof(*usbp));

	usbp->st_dev = sbp->st_dev;
	usbp->st_ino = sbp->st_ino;
	usbp->st_mode = sbp->st_mode;
	usbp->st_nlink = sbp->st_nlink;
	usbp->st_uid = sbp->st_uid;
	usbp->st_gid = sbp->st_gid;
	usbp->st_rdev = sbp->st_rdev;
#ifndef _POSIX_C_SOURCE
	usbp->st_atimespec.tv_sec = sbp->st_atimespec.tv_sec;
	usbp->st_atimespec.tv_nsec = sbp->st_atimespec.tv_nsec;
	usbp->st_mtimespec.tv_sec = sbp->st_mtimespec.tv_sec;
	usbp->st_mtimespec.tv_nsec = sbp->st_mtimespec.tv_nsec;
	usbp->st_ctimespec.tv_sec = sbp->st_ctimespec.tv_sec;
	usbp->st_ctimespec.tv_nsec = sbp->st_ctimespec.tv_nsec;
	usbp->st_birthtimespec.tv_sec = sbp->st_birthtimespec.tv_sec;
	usbp->st_birthtimespec.tv_nsec = sbp->st_birthtimespec.tv_nsec;
#else
	usbp->st_atime = sbp->st_atime;
	usbp->st_atimensec = sbp->st_atimensec;
	usbp->st_mtime = sbp->st_mtime;
	usbp->st_mtimensec = sbp->st_mtimensec;
	usbp->st_ctime = sbp->st_ctime;
	usbp->st_ctimensec = sbp->st_ctimensec;
	usbp->st_birthtime = sbp->st_birthtime;
	usbp->st_birthtimensec = sbp->st_birthtimensec;
#endif
	usbp->st_size = sbp->st_size;
	usbp->st_blocks = sbp->st_blocks;
	usbp->st_blksize = sbp->st_blksize;
	usbp->st_flags = sbp->st_flags;
	usbp->st_gen = sbp->st_gen;
	usbp->st_lspare = sbp->st_lspare;
	usbp->st_qspare[0] = sbp->st_qspare[0];
	usbp->st_qspare[1] = sbp->st_qspare[1];
}

void
munge_user32_stat64(struct stat64 *sbp, struct user32_stat64 *usbp)
{
	bzero(usbp, sizeof(*usbp));

	usbp->st_dev = sbp->st_dev;
	usbp->st_ino = sbp->st_ino;
	usbp->st_mode = sbp->st_mode;
	usbp->st_nlink = sbp->st_nlink;
	usbp->st_uid = sbp->st_uid;
	usbp->st_gid = sbp->st_gid;
	usbp->st_rdev = sbp->st_rdev;
#ifndef _POSIX_C_SOURCE
	usbp->st_atimespec.tv_sec = (user32_time_t)sbp->st_atimespec.tv_sec;
	usbp->st_atimespec.tv_nsec = (user32_long_t)sbp->st_atimespec.tv_nsec;
	usbp->st_mtimespec.tv_sec = (user32_time_t)sbp->st_mtimespec.tv_sec;
	usbp->st_mtimespec.tv_nsec = (user32_long_t)sbp->st_mtimespec.tv_nsec;
	usbp->st_ctimespec.tv_sec = (user32_time_t)sbp->st_ctimespec.tv_sec;
	usbp->st_ctimespec.tv_nsec = (user32_long_t)sbp->st_ctimespec.tv_nsec;
	usbp->st_birthtimespec.tv_sec = (user32_time_t)sbp->st_birthtimespec.tv_sec;
	usbp->st_birthtimespec.tv_nsec = (user32_long_t)sbp->st_birthtimespec.tv_nsec;
#else
	usbp->st_atime = sbp->st_atime;
	usbp->st_atimensec = sbp->st_atimensec;
	usbp->st_mtime = sbp->st_mtime;
	usbp->st_mtimensec = sbp->st_mtimensec;
	usbp->st_ctime = sbp->st_ctime;
	usbp->st_ctimensec = sbp->st_ctimensec;
	usbp->st_birthtime = sbp->st_birthtime;
	usbp->st_birthtimensec = sbp->st_birthtimensec;
#endif
	usbp->st_size = sbp->st_size;
	usbp->st_blocks = sbp->st_blocks;
	usbp->st_blksize = sbp->st_blksize;
	usbp->st_flags = sbp->st_flags;
	usbp->st_gen = sbp->st_gen;
	usbp->st_lspare = sbp->st_lspare;
	usbp->st_qspare[0] = sbp->st_qspare[0];
	usbp->st_qspare[1] = sbp->st_qspare[1];
}

  
                                                
   
static int
vnode_purge_callback(struct vnode *vp, __unused void *cargs)
{
	ubc_msync(vp, (off_t)0, ubc_getsize(vp), NULL                       , UBC_PUSHALL | UBC_INVALIDATE);

	return VNODE_RETURNED;
}

static int
vfs_purge_callback(mount_t mp, __unused void * arg)
{
	vnode_iterate(mp, VNODE_WAIT | VNODE_ITERATE_ALL, vnode_purge_callback, NULL);

	return VFS_RETURNED;
}

int
vfs_purge(__unused struct proc *p, __unused struct vfs_purge_args *uap, __unused int32_t *retval)
{
	if (!kauth_cred_issuser(kauth_cred_get())) {
		return EPERM;
	}

	vfs_iterate(0            , vfs_purge_callback, NULL);

	return 0;
}

  
                                                                  
                                                                  
                    
   
int
vnode_get_snapdir(vnode_t rvp, vnode_t *sdvpp, vfs_context_t ctx)
{
	return VFS_VGET_SNAPDIR(vnode_mount(rvp), sdvpp, ctx);
}

  
                          
  
                                                                       
                            
  
                                                             
  
                                                                         
              
   
static int
vnode_get_snapshot(int dirfd, vnode_t *rvpp, vnode_t *sdvpp,
    user_addr_t name, struct nameidata *ndp, int32_t op,
#if !CONFIG_TRIGGERS
    __unused
#endif
    enum path_operation pathop,
    vfs_context_t ctx)
{
	int error, i;
	caddr_t name_buf;
	size_t name_len;
	struct vfs_attr vfa;

	*sdvpp = NULLVP;
	*rvpp = NULLVP;

	error = vnode_getfromfd(ctx, dirfd, rvpp);
	if (error) {
		return error;
	}

	if (!vnode_isvroot(*rvpp)) {
		error = EINVAL;
		goto out;
	}

	                                                 
	VFSATTR_INIT(&vfa);
	VFSATTR_WANTED(&vfa, f_capabilities);
	if ((vfs_getattr(vnode_mount(*rvpp), &vfa, ctx) != 0) ||
	    !VFSATTR_IS_SUPPORTED(&vfa, f_capabilities) ||
	    !((vfa.f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] &
	    VOL_CAP_INT_SNAPSHOT)) ||
	    !((vfa.f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] &
	    VOL_CAP_INT_SNAPSHOT))) {
		error = ENOTSUP;
		goto out;
	}

	error = vnode_get_snapdir(*rvpp, sdvpp, ctx);
	if (error) {
		goto out;
	}

	name_buf = zalloc_flags(ZV_NAMEI, Z_WAITOK);
	error = copyinstr(name, name_buf, MAXPATHLEN, &name_len);
	if (error) {
		goto out1;
	}

	  
                                                                         
                                                                   
    
	if ((name_len == 1) || (name_len == 2 && name_buf[0] == '.') ||
	    (name_len == 3 && name_buf[0] == '.' && name_buf[1] == '.')) {
		error = EINVAL;
		goto out1;
	}
	for (i = 0; i < (int)name_len && name_buf[i] != '/'; i++) {
		;
	}
	if (i < (int)name_len) {
		error = EINVAL;
		goto out1;
	}

#if CONFIG_MACF
	if (op == CREATE) {
		error = mac_mount_check_snapshot_create(ctx, vnode_mount(*rvpp),
		    name_buf);
	} else if (op == DELETE) {
		error = mac_mount_check_snapshot_delete(ctx, vnode_mount(*rvpp),
		    name_buf);
	}
	if (error) {
		goto out1;
	}
#endif

	                                              
	NDINIT(ndp, op, pathop, USEDVP | NOCACHE | AUDITVNPATH1,
	    UIO_SYSSPACE, CAST_USER_ADDR_T(name_buf), ctx);
	ndp->ni_dvp = *sdvpp;

	error = namei(ndp);
out1:
	zfree(ZV_NAMEI, name_buf);
out:
	if (error) {
		if (*sdvpp) {
			vnode_put(*sdvpp);
			*sdvpp = NULLVP;
		}
		if (*rvpp) {
			vnode_put(*rvpp);
			*rvpp = NULLVP;
		}
	}
	return error;
}

  
                                                            
  
                                                                     
                                                                        
                          
  
                
  
                                                         
                                  
  
                                                                          
        
   
static int __attribute__((noinline))
snapshot_create(int dirfd, user_addr_t name, __unused uint32_t flags,
    vfs_context_t ctx)
{
	vnode_t rvp, snapdvp;
	int error;
	struct nameidata *ndp;

	ndp = kheap_alloc(KHEAP_TEMP, sizeof(*ndp), Z_WAITOK);

	error = vnode_get_snapshot(dirfd, &rvp, &snapdvp, name, ndp, CREATE,
	    OP_LINK, ctx);
	if (error) {
		goto out;
	}

	if (ndp->ni_vp) {
		vnode_put(ndp->ni_vp);
		error = EEXIST;
	} else {
		struct vnode_attr *vap;
		vnode_t vp = NULLVP;

		vap = kheap_alloc(KHEAP_TEMP, sizeof(*vap), Z_WAITOK);

		VATTR_INIT(vap);
		VATTR_SET(vap, va_type, VREG);
		VATTR_SET(vap, va_mode, 0);

		error = vn_create(snapdvp, &vp, ndp, vap,
		    VN_CREATE_NOAUTH  | VN_CREATE_NOINHERIT, 0, NULL, ctx);
		if (!error && vp) {
			vnode_put(vp);
		}

		kheap_free(KHEAP_TEMP, vap, sizeof(*vap));
	}

	nameidone(ndp);
	vnode_put(snapdvp);
	vnode_put(rvp);
out:
	kheap_free(KHEAP_TEMP, ndp, sizeof(*ndp));

	return error;
}

  
                               
  
                                                                        
                       
   
static int __attribute__((noinline))
snapshot_delete(int dirfd, user_addr_t name, __unused uint32_t flags,
    vfs_context_t ctx)
{
	vnode_t rvp, snapdvp;
	int error;
	struct nameidata *ndp;

	ndp = kheap_alloc(KHEAP_TEMP, sizeof(*ndp), Z_WAITOK);

	error = vnode_get_snapshot(dirfd, &rvp, &snapdvp, name, ndp, DELETE,
	    OP_UNLINK, ctx);
	if (error) {
		goto out;
	}

	error = VNOP_REMOVE(snapdvp, ndp->ni_vp, &ndp->ni_cnd,
	    VNODE_REMOVE_SKIP_NAMESPACE_EVENT, ctx);

	vnode_put(ndp->ni_vp);
	nameidone(ndp);
	vnode_put(snapdvp);
	vnode_put(rvp);
out:
	kheap_free(KHEAP_TEMP, ndp, sizeof(*ndp));

	return error;
}

  
                                    
  
                                                                      
   
static int __attribute__((noinline))
snapshot_revert(int dirfd, user_addr_t name, __unused uint32_t flags,
    vfs_context_t ctx)
{
	int error;
	vnode_t rvp;
	mount_t mp;
	struct fs_snapshot_revert_args revert_data;
	struct componentname cnp;
	caddr_t name_buf;
	size_t name_len;

	error = vnode_getfromfd(ctx, dirfd, &rvp);
	if (error) {
		return error;
	}
	mp = vnode_mount(rvp);

	name_buf = zalloc_flags(ZV_NAMEI, Z_WAITOK);
	error = copyinstr(name, name_buf, MAXPATHLEN, &name_len);
	if (error) {
		zfree(ZV_NAMEI, name_buf);
		vnode_put(rvp);
		return error;
	}

#if CONFIG_MACF
	error = mac_mount_check_snapshot_revert(ctx, mp, name_buf);
	if (error) {
		zfree(ZV_NAMEI, name_buf);
		vnode_put(rvp);
		return error;
	}
#endif

	  
                                                        
                                                                
    
	error = mount_iterref(mp, 0);
	vnode_put(rvp);
	if (error) {
		zfree(ZV_NAMEI, name_buf);
		return error;
	}

	memset(&cnp, 0, sizeof(cnp));
	cnp.cn_pnbuf = (char *)name_buf;
	cnp.cn_nameiop = LOOKUP;
	cnp.cn_flags = ISLASTCN | HASBUF;
	cnp.cn_pnlen = MAXPATHLEN;
	cnp.cn_nameptr = cnp.cn_pnbuf;
	cnp.cn_namelen = (int)name_len;
	revert_data.sr_cnp = &cnp;

	error = VFS_IOCTL(mp, VFSIOC_REVERT_SNAPSHOT, (caddr_t)&revert_data, 0, ctx);
	mount_iterdrop(mp);
	zfree(ZV_NAMEI, name_buf);

	if (error) {
		                                                        

		vnode_t snapdvp;
		struct nameidata namend;

		error = vnode_get_snapshot(dirfd, &rvp, &snapdvp, name, &namend, LOOKUP,
		    OP_LOOKUP, ctx);
		if (error) {
			return error;
		}


		error = VNOP_IOCTL(namend.ni_vp, APFSIOC_REVERT_TO_SNAPSHOT, (caddr_t) NULL,
		    0, ctx);

		vnode_put(namend.ni_vp);
		nameidone(&namend);
		vnode_put(snapdvp);
		vnode_put(rvp);
	}

	return error;
}

  
                               
  
                                                                        
                                                                       
                                                                          
                                                                             
   
static int __attribute__((noinline))
snapshot_rename(int dirfd, user_addr_t old, user_addr_t new,
    __unused uint32_t flags, vfs_context_t ctx)
{
	vnode_t rvp, snapdvp;
	int error, i;
	caddr_t newname_buf;
	size_t name_len;
	vnode_t fvp;
	struct nameidata *fromnd, *tond;
	                                                                      
	struct {
		struct nameidata from_node;
		struct nameidata to_node;
	} * __rename_data;

	__rename_data = kheap_alloc(KHEAP_TEMP, sizeof(*__rename_data), Z_WAITOK);
	fromnd = &__rename_data->from_node;
	tond = &__rename_data->to_node;

	error = vnode_get_snapshot(dirfd, &rvp, &snapdvp, old, fromnd, DELETE,
	    OP_UNLINK, ctx);
	if (error) {
		goto out;
	}
	fvp  = fromnd->ni_vp;

	newname_buf = zalloc_flags(ZV_NAMEI, Z_WAITOK);
	error = copyinstr(new, newname_buf, MAXPATHLEN, &name_len);
	if (error) {
		goto out1;
	}

	  
                                                                    
            
                                                                   
   
                                                                    
                    
    
	if ((name_len == 1) || (name_len == 2 && newname_buf[0] == '.') ||
	    (name_len == 3 && newname_buf[0] == '.' && newname_buf[1] == '.')) {
		error = EINVAL;
		goto out1;
	}
	for (i = 0; i < (int)name_len && newname_buf[i] != '/'; i++) {
		;
	}
	if (i < (int)name_len) {
		error = EINVAL;
		goto out1;
	}

#if CONFIG_MACF
	error = mac_mount_check_snapshot_create(ctx, vnode_mount(rvp),
	    newname_buf);
	if (error) {
		goto out1;
	}
#endif

	NDINIT(tond, RENAME, OP_RENAME, USEDVP | NOCACHE | AUDITVNPATH2,
	    UIO_SYSSPACE, CAST_USER_ADDR_T(newname_buf), ctx);
	tond->ni_dvp = snapdvp;

	error = namei(tond);
	if (error) {
		goto out2;
	} else if (tond->ni_vp) {
		  
                                                                
                                         
     
		vnode_put(tond->ni_vp);
		error = EEXIST;
		goto out2;
	}

	error = VNOP_RENAME(snapdvp, fvp, &fromnd->ni_cnd, snapdvp, NULLVP,
	    &tond->ni_cnd, ctx);

out2:
	nameidone(tond);
out1:
	zfree(ZV_NAMEI, newname_buf);
	vnode_put(fvp);
	vnode_put(snapdvp);
	vnode_put(rvp);
	nameidone(fromnd);
out:
	kheap_free(KHEAP_TEMP, __rename_data, sizeof(*__rename_data));
	return error;
}

  
                              
  
                                                                        
                      
   
static int __attribute__((noinline))
snapshot_mount(int dirfd, user_addr_t name, user_addr_t directory,
    __unused user_addr_t mnt_data, __unused uint32_t flags, vfs_context_t ctx)
{
	mount_t mp;
	vnode_t rvp, snapdvp, snapvp, vp, pvp;
	struct fs_snapshot_mount_args smnt_data;
	int error;
	struct nameidata *snapndp, *dirndp;
	                                                                      
	struct {
		struct nameidata snapnd;
		struct nameidata dirnd;
	} * __snapshot_mount_data;

	__snapshot_mount_data = kheap_alloc(KHEAP_TEMP,
	    sizeof(*__snapshot_mount_data), Z_WAITOK);
	snapndp = &__snapshot_mount_data->snapnd;
	dirndp = &__snapshot_mount_data->dirnd;

	error = vnode_get_snapshot(dirfd, &rvp, &snapdvp, name, snapndp, LOOKUP,
	    OP_LOOKUP, ctx);
	if (error) {
		goto out;
	}

	snapvp  = snapndp->ni_vp;
	if (!vnode_mount(rvp) || (vnode_mount(rvp) == dead_mountp)) {
		error = EIO;
		goto out1;
	}

	                                 
	NDINIT(dirndp, LOOKUP, OP_MOUNT, FOLLOW | AUDITVNPATH1 | WANTPARENT,
	    UIO_USERSPACE, directory, ctx);
	error = namei(dirndp);
	if (error) {
		goto out1;
	}

	vp = dirndp->ni_vp;
	pvp = dirndp->ni_dvp;
	mp = vnode_mount(rvp);

	if ((vp->v_flag & VROOT) && (vp->v_mount->mnt_flag & MNT_ROOTFS)) {
		error = EINVAL;
		goto out2;
	}

#if CONFIG_MACF
	error = mac_mount_check_snapshot_mount(ctx, rvp, vp, &dirndp->ni_cnd, snapndp->ni_cnd.cn_nameptr,
	    mp->mnt_vfsstat.f_fstypename);
	if (error) {
		goto out2;
	}
#endif

	smnt_data.sm_mp  = mp;
	smnt_data.sm_cnp = &snapndp->ni_cnd;
	error = mount_common(mp->mnt_vfsstat.f_fstypename, pvp, vp,
	    &dirndp->ni_cnd, CAST_USER_ADDR_T(&smnt_data), flags & MNT_DONTBROWSE,
	    KERNEL_MOUNT_SNAPSHOT, NULL, FALSE, ctx);

out2:
	vnode_put(vp);
	vnode_put(pvp);
	nameidone(dirndp);
out1:
	vnode_put(snapvp);
	vnode_put(snapdvp);
	vnode_put(rvp);
	nameidone(snapndp);
out:
	kheap_free(KHEAP_TEMP, __snapshot_mount_data,
	    sizeof(*__snapshot_mount_data));
	return error;
}

  
                                         
  
                                                                     
   
static int __attribute__((noinline))
snapshot_root(int dirfd, user_addr_t name, __unused uint32_t flags,
    vfs_context_t ctx)
{
	int error;
	vnode_t rvp;
	mount_t mp;
	struct fs_snapshot_root_args root_data;
	struct componentname cnp;
	caddr_t name_buf;
	size_t name_len;

	error = vnode_getfromfd(ctx, dirfd, &rvp);
	if (error) {
		return error;
	}
	mp = vnode_mount(rvp);

	name_buf = zalloc_flags(ZV_NAMEI, Z_WAITOK);
	error = copyinstr(name, name_buf, MAXPATHLEN, &name_len);
	if (error) {
		zfree(ZV_NAMEI, name_buf);
		vnode_put(rvp);
		return error;
	}

	                   

	  
                                                        
                                                              
    
	error = mount_iterref(mp, 0);
	vnode_put(rvp);
	if (error) {
		zfree(ZV_NAMEI, name_buf);
		return error;
	}

	memset(&cnp, 0, sizeof(cnp));
	cnp.cn_pnbuf = (char *)name_buf;
	cnp.cn_nameiop = LOOKUP;
	cnp.cn_flags = ISLASTCN | HASBUF;
	cnp.cn_pnlen = MAXPATHLEN;
	cnp.cn_nameptr = cnp.cn_pnbuf;
	cnp.cn_namelen = (int)name_len;
	root_data.sr_cnp = &cnp;

	error = VFS_IOCTL(mp, VFSIOC_ROOT_SNAPSHOT, (caddr_t)&root_data, 0, ctx);

	mount_iterdrop(mp);
	zfree(ZV_NAMEI, name_buf);

	return error;
}

  
                                    
   
int
fs_snapshot(__unused proc_t p, struct fs_snapshot_args *uap,
    __unused int32_t *retval)
{
	int error;
	vfs_context_t ctx = vfs_context_current();

	AUDIT_ARG(fd, uap->dirfd);
	AUDIT_ARG(value32, uap->op);

	error = priv_check_cred(vfs_context_ucred(ctx), PRIV_VFS_SNAPSHOT, 0);
	if (error) {
		return error;
	}

	  
                                                                    
                                       
    
	if (uap->op != SNAPSHOT_OP_MOUNT) {
		vnode_t dvp = NULLVP;
		vnode_t devvp = NULLVP;
		mount_t mp;

		error = vnode_getfromfd(ctx, uap->dirfd, &dvp);
		if (error) {
			return error;
		}
		mp = vnode_mount(dvp);
		devvp = mp->mnt_devvp;

		                             
		if (devvp == NULLVP) {
			error = vnode_lookup(mp->mnt_vfsstat.f_mntfromname, 0, &devvp, ctx);
			                                          
			if (error == ENOENT) {
				error = ENXIO;
			}
		} else {
			error = vnode_getwithref(devvp);
		}

		if (error) {
			vnode_put(dvp);
			return error;
		}

		if ((vfs_context_issuser(ctx) == 0) &&
		    (vnode_authorize(devvp, NULL, KAUTH_VNODE_WRITE_DATA, ctx) != 0)) {
			error = EPERM;
		}
		vnode_put(dvp);
		vnode_put(devvp);

		if (error) {
			return error;
		}
	}

	switch (uap->op) {
	case SNAPSHOT_OP_CREATE:
		error = snapshot_create(uap->dirfd, uap->name1, uap->flags, ctx);
		break;
	case SNAPSHOT_OP_DELETE:
		error = snapshot_delete(uap->dirfd, uap->name1, uap->flags, ctx);
		break;
	case SNAPSHOT_OP_RENAME:
		error = snapshot_rename(uap->dirfd, uap->name1, uap->name2,
		    uap->flags, ctx);
		break;
	case SNAPSHOT_OP_MOUNT:
		error = snapshot_mount(uap->dirfd, uap->name1, uap->name2,
		    uap->data, uap->flags, ctx);
		break;
	case SNAPSHOT_OP_REVERT:
		error = snapshot_revert(uap->dirfd, uap->name1, uap->flags, ctx);
		break;
#if CONFIG_MNT_ROOTSNAP
	case SNAPSHOT_OP_ROOT:
		error = snapshot_root(uap->dirfd, uap->name1, uap->flags, ctx);
		break;
#endif                          
	default:
		error = ENOSYS;
	}

	return error;
}
