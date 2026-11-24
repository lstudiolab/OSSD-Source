/*
 * Copyright (c) 2000-2010 Apple Inc. All rights reserved.
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
/*-
 * Copyright 1997,1998 Julian Elischer.  All rights reserved.
 * julian@freebsd.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * devfs_vfsops.c
 *
 */
  
                                                                      
                                                                          
                                                                        
               
   
  
          
                                                                   
                                                                 
                                    
   


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vnode_internal.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/mount_internal.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <libkern/OSAtomic.h>
#include <atm/atm_internal.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#include "devfs.h"
#include "devfsdefs.h"

#if FDESC
#include "fdesc.h"
#endif            


static int devfs_statfs( struct mount *mp, struct vfsstatfs *sbp, vfs_context_t ctx);
static int devfs_vfs_getattr(mount_t mp, struct vfs_attr *fsap, vfs_context_t ctx);

#if CONFIG_DEV_KMEM
extern boolean_t dev_kmem_enabled;
#endif

   
                                        
                                                    
                                                      
                                        
                                                                  
                                 
   
static int
devfs_init(__unused struct vfsconf *vfsp)
{
	if (devfs_sinit()) {
		return ENOTSUP;
	}
	devfs_make_node(makedev(0, 0), DEVFS_CHAR,
	    UID_ROOT, GID_WHEEL, 0622, "console");
	devfs_make_node(makedev(2, 0), DEVFS_CHAR,
	    UID_ROOT, GID_WHEEL, 0666, "tty");
#if CONFIG_DEV_KMEM
	if (dev_kmem_enabled) {
		                                                 
		devfs_make_node(makedev(3, 1), DEVFS_CHAR,
		    UID_ROOT, GID_KMEM, 0640, "kmem");
	}
#endif
	devfs_make_node(makedev(3, 2), DEVFS_CHAR,
	    UID_ROOT, GID_WHEEL, 0666, "null");
	devfs_make_node(makedev(3, 3), DEVFS_CHAR,
	    UID_ROOT, GID_WHEEL, 0666, "zero");
	uint32_t logging_config = atm_get_diagnostic_config();

	devfs_make_node(makedev(6, 0), DEVFS_CHAR,
	    UID_ROOT, GID_WHEEL, 0600, "klog");

	if (!(logging_config & ATM_TRACE_DISABLE)) {
		devfs_make_node(makedev(7, 0), DEVFS_CHAR,
		    UID_LOGD, GID_LOGD, 0600, "oslog");
		if (cdevsw_setkqueueok(7, (&(cdevsw[7])), 0) == -1) {
			return ENOTSUP;
		}

		devfs_make_node(makedev(8, 0), DEVFS_CHAR,
		    UID_ROOT, GID_WHEEL, 0600, "oslog_stream");
		if (cdevsw_setkqueueok(8, (&(cdevsw[8])), 0) == -1) {
			return ENOTSUP;
		}
	}


#if  FDESC
	devfs_fdesc_init();
#endif

	return 0;
}

   
                                      
                                                                  
                                                           
                                                                   
                     
                                        
                       
                                                                         
                                         
                                                                            
                                         
   
         
int
devfs_mount(struct mount *mp, __unused vnode_t devvp, __unused user_addr_t data, vfs_context_t ctx)
{
	struct devfsmount *devfs_mp_p;                                 
	int error;

	   
                                                               
    
	if (mp->mnt_flag & MNT_UPDATE) {
		return 0;
	}

	                                                         
	vfs_setlocklocal(mp);

	   
                                                         
                       
                                                               
    

	MALLOC(devfs_mp_p, struct devfsmount *, sizeof(struct devfsmount),
	    M_DEVFSMNT, M_WAITOK);
	if (devfs_mp_p == NULL) {
		return ENOMEM;
	}
	bzero(devfs_mp_p, sizeof(*devfs_mp_p));
	devfs_mp_p->mount = mp;

	   
                         
    
	__IGNORE_WCASTALIGN(mp->mnt_data = (qaddr_t)devfs_mp_p);
	mp->mnt_vfsstat.f_fsid.val[0] = (int32_t)VM_KERNEL_ADDRHASH(devfs_mp_p);
	mp->mnt_vfsstat.f_fsid.val[1] = vfs_typenum(mp);
	mp->mnt_flag |= MNT_LOCAL;

	DEVFS_LOCK();
	error = dev_dup_plane(devfs_mp_p);
	DEVFS_UNLOCK();

	if (error) {
		mp->mnt_data = (qaddr_t)0;
		FREE(devfs_mp_p, M_DEVFSMNT);
		return error;
	} else {
		DEVFS_INCR_MOUNTS();
	}

	   
                                                     
                         
                                                        
                
    

	bzero(mp->mnt_vfsstat.f_mntfromname, MAXPATHLEN);
	bcopy("devfs", mp->mnt_vfsstat.f_mntfromname, 5);
	(void)devfs_statfs(mp, &mp->mnt_vfsstat, ctx);

	return 0;
}


static int
devfs_start(__unused struct mount *mp, __unused int flags, __unused vfs_context_t ctx)
{
	return 0;
}

   
                                           
   
static int
devfs_unmount( struct mount *mp, int mntflags, __unused vfs_context_t ctx)
{
	struct devfsmount *devfs_mp_p = (struct devfsmount *)mp->mnt_data;
	int flags = 0;
	int force = 0;
	int error;

	if (mntflags & MNT_FORCE) {
		flags |= FORCECLOSE;
		force = 1;
	}
	error = vflush(mp, NULLVP, flags);
	if (error && !force) {
		return error;
	}

	DEVFS_LOCK();
	devfs_free_plane(devfs_mp_p);
	DEVFS_UNLOCK();

	DEVFS_DECR_MOUNTS();

	FREE(devfs_mp_p, M_DEVFSMNT);
	mp->mnt_data = (qaddr_t)0;
	mp->mnt_flag &= ~MNT_LOCAL;

	return 0;
}

                                                   
static int
devfs_root(struct mount *mp, struct vnode **vpp, __unused vfs_context_t ctx)
{
	struct devfsmount *devfs_mp_p = (struct devfsmount *)(mp->mnt_data);
	int error;

	DEVFS_LOCK();
	                                                 
	error = devfs_dntovn(devfs_mp_p->plane_root->de_dnp, vpp, NULL);
	DEVFS_UNLOCK();

	return error;
}

static int
devfs_statfs( struct mount *mp, struct vfsstatfs *sbp, __unused vfs_context_t ctx)
{
	struct devfsmount *devfs_mp_p = (struct devfsmount *)mp->mnt_data;

	   
                            
    
	                                         
	sbp->f_flags  = 0;                       
	sbp->f_bsize  = 512;
	sbp->f_iosize = 512;
	sbp->f_blocks = (devfs_stats.mounts * sizeof(struct devfsmount)
	    + devfs_stats.nodes * sizeof(devnode_t)
	    + devfs_stats.entries * sizeof(devdirent_t)
	    + devfs_stats.stringspace
	    ) / sbp->f_bsize;
	sbp->f_bfree  = 0;
	sbp->f_bavail = 0;
	sbp->f_files  = devfs_stats.nodes;
	sbp->f_ffree  = 0;
	sbp->f_fsid.val[0] = (int32_t)VM_KERNEL_ADDRHASH(devfs_mp_p);
	sbp->f_fsid.val[1] = vfs_typenum(mp);

	return 0;
}

static int
devfs_vfs_getattr(__unused mount_t mp, struct vfs_attr *fsap, __unused vfs_context_t ctx)
{
	VFSATTR_RETURN(fsap, f_objcount, devfs_stats.nodes);
	VFSATTR_RETURN(fsap, f_maxobjcount, devfs_stats.nodes);
	VFSATTR_RETURN(fsap, f_bsize, 512);
	VFSATTR_RETURN(fsap, f_iosize, 512);
	if (VFSATTR_IS_ACTIVE(fsap, f_blocks) || VFSATTR_IS_ACTIVE(fsap, f_bused)) {
		fsap->f_blocks = (devfs_stats.mounts * sizeof(struct devfsmount)
		    + devfs_stats.nodes * sizeof(devnode_t)
		    + devfs_stats.entries * sizeof(devdirent_t)
		    + devfs_stats.stringspace
		    ) / fsap->f_bsize;
		fsap->f_bused = fsap->f_blocks;
		VFSATTR_SET_SUPPORTED(fsap, f_blocks);
		VFSATTR_SET_SUPPORTED(fsap, f_bused);
	}
	VFSATTR_RETURN(fsap, f_bfree, 0);
	VFSATTR_RETURN(fsap, f_bavail, 0);
	VFSATTR_RETURN(fsap, f_files, devfs_stats.nodes);
	VFSATTR_RETURN(fsap, f_ffree, 0);
	VFSATTR_RETURN(fsap, f_fssubtype, 0);

	if (VFSATTR_IS_ACTIVE(fsap, f_capabilities)) {
		fsap->f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] =
		    VOL_CAP_FMT_SYMBOLICLINKS |
		    VOL_CAP_FMT_HARDLINKS |
		    VOL_CAP_FMT_NO_ROOT_TIMES |
		    VOL_CAP_FMT_CASE_SENSITIVE |
		    VOL_CAP_FMT_CASE_PRESERVING |
		    VOL_CAP_FMT_FAST_STATFS |
		    VOL_CAP_FMT_2TB_FILESIZE |
		    VOL_CAP_FMT_HIDDEN_FILES;
		fsap->f_capabilities.capabilities[VOL_CAPABILITIES_INTERFACES] =
		    VOL_CAP_INT_ATTRLIST;
		fsap->f_capabilities.capabilities[VOL_CAPABILITIES_RESERVED1] = 0;
		fsap->f_capabilities.capabilities[VOL_CAPABILITIES_RESERVED2] = 0;

		fsap->f_capabilities.valid[VOL_CAPABILITIES_FORMAT] =
		    VOL_CAP_FMT_PERSISTENTOBJECTIDS |
		    VOL_CAP_FMT_SYMBOLICLINKS |
		    VOL_CAP_FMT_HARDLINKS |
		    VOL_CAP_FMT_JOURNAL |
		    VOL_CAP_FMT_JOURNAL_ACTIVE |
		    VOL_CAP_FMT_NO_ROOT_TIMES |
		    VOL_CAP_FMT_SPARSE_FILES |
		    VOL_CAP_FMT_ZERO_RUNS |
		    VOL_CAP_FMT_CASE_SENSITIVE |
		    VOL_CAP_FMT_CASE_PRESERVING |
		    VOL_CAP_FMT_FAST_STATFS |
		    VOL_CAP_FMT_2TB_FILESIZE |
		    VOL_CAP_FMT_OPENDENYMODES |
		    VOL_CAP_FMT_HIDDEN_FILES |
		    VOL_CAP_FMT_PATH_FROM_ID |
		    VOL_CAP_FMT_NO_VOLUME_SIZES;
		fsap->f_capabilities.valid[VOL_CAPABILITIES_INTERFACES] =
		    VOL_CAP_INT_SEARCHFS |
		    VOL_CAP_INT_ATTRLIST |
		    VOL_CAP_INT_NFSEXPORT |
		    VOL_CAP_INT_READDIRATTR |
		    VOL_CAP_INT_EXCHANGEDATA |
		    VOL_CAP_INT_COPYFILE |
		    VOL_CAP_INT_ALLOCATE |
		    VOL_CAP_INT_VOL_RENAME |
		    VOL_CAP_INT_ADVLOCK |
		    VOL_CAP_INT_FLOCK |
		    VOL_CAP_INT_EXTENDED_SECURITY |
		    VOL_CAP_INT_USERACCESS |
		    VOL_CAP_INT_MANLOCK |
		    VOL_CAP_INT_EXTENDED_ATTR |
		    VOL_CAP_INT_NAMEDSTREAMS;
		fsap->f_capabilities.valid[VOL_CAPABILITIES_RESERVED1] = 0;
		fsap->f_capabilities.valid[VOL_CAPABILITIES_RESERVED2] = 0;

		VFSATTR_SET_SUPPORTED(fsap, f_capabilities);
	}

	if (VFSATTR_IS_ACTIVE(fsap, f_attributes)) {
		fsap->f_attributes.validattr.commonattr =
		    ATTR_CMN_NAME | ATTR_CMN_DEVID | ATTR_CMN_FSID |
		    ATTR_CMN_OBJTYPE | ATTR_CMN_OBJTAG | ATTR_CMN_OBJID |
		    ATTR_CMN_PAROBJID |
		    ATTR_CMN_MODTIME | ATTR_CMN_CHGTIME | ATTR_CMN_ACCTIME |
		    ATTR_CMN_OWNERID | ATTR_CMN_GRPID | ATTR_CMN_ACCESSMASK |
		    ATTR_CMN_FLAGS | ATTR_CMN_USERACCESS | ATTR_CMN_FILEID;
		fsap->f_attributes.validattr.volattr =
		    ATTR_VOL_FSTYPE | ATTR_VOL_SIZE | ATTR_VOL_SPACEFREE |
		    ATTR_VOL_SPACEAVAIL | ATTR_VOL_MINALLOCATION |
		    ATTR_VOL_OBJCOUNT | ATTR_VOL_MAXOBJCOUNT |
		    ATTR_VOL_MOUNTPOINT | ATTR_VOL_MOUNTFLAGS |
		    ATTR_VOL_MOUNTEDDEVICE | ATTR_VOL_CAPABILITIES |
		    ATTR_VOL_ATTRIBUTES;
		fsap->f_attributes.validattr.dirattr =
		    ATTR_DIR_LINKCOUNT | ATTR_DIR_MOUNTSTATUS;
		fsap->f_attributes.validattr.fileattr =
		    ATTR_FILE_LINKCOUNT | ATTR_FILE_TOTALSIZE |
		    ATTR_FILE_IOBLOCKSIZE | ATTR_FILE_DEVTYPE |
		    ATTR_FILE_DATALENGTH;
		fsap->f_attributes.validattr.forkattr = 0;

		fsap->f_attributes.nativeattr.commonattr =
		    ATTR_CMN_NAME | ATTR_CMN_DEVID | ATTR_CMN_FSID |
		    ATTR_CMN_OBJTYPE | ATTR_CMN_OBJTAG | ATTR_CMN_OBJID |
		    ATTR_CMN_PAROBJID |
		    ATTR_CMN_MODTIME | ATTR_CMN_CHGTIME | ATTR_CMN_ACCTIME |
		    ATTR_CMN_OWNERID | ATTR_CMN_GRPID | ATTR_CMN_ACCESSMASK |
		    ATTR_CMN_FLAGS | ATTR_CMN_USERACCESS | ATTR_CMN_FILEID;
		fsap->f_attributes.nativeattr.volattr =
		    ATTR_VOL_FSTYPE | ATTR_VOL_SIZE | ATTR_VOL_SPACEFREE |
		    ATTR_VOL_SPACEAVAIL | ATTR_VOL_MINALLOCATION |
		    ATTR_VOL_OBJCOUNT | ATTR_VOL_MAXOBJCOUNT |
		    ATTR_VOL_MOUNTPOINT | ATTR_VOL_MOUNTFLAGS |
		    ATTR_VOL_MOUNTEDDEVICE | ATTR_VOL_CAPABILITIES |
		    ATTR_VOL_ATTRIBUTES;
		fsap->f_attributes.nativeattr.dirattr =
		    ATTR_DIR_MOUNTSTATUS;
		fsap->f_attributes.nativeattr.fileattr =
		    ATTR_FILE_LINKCOUNT | ATTR_FILE_TOTALSIZE |
		    ATTR_FILE_IOBLOCKSIZE | ATTR_FILE_DEVTYPE |
		    ATTR_FILE_DATALENGTH;
		fsap->f_attributes.nativeattr.forkattr = 0;

		VFSATTR_SET_SUPPORTED(fsap, f_attributes);
	}

	return 0;
}

static int
devfs_sync(__unused struct mount *mp, __unused int waitfor, __unused vfs_context_t ctx)
{
	return 0;
}


static int
devfs_vget(__unused struct mount *mp, __unused ino64_t ino, __unused struct vnode **vpp, __unused vfs_context_t ctx)
{
	return ENOTSUP;
}

                                                              
                                                              
                              
   

static int
devfs_fhtovp(__unused struct mount *mp, __unused int fhlen, __unused unsigned char *fhp, __unused struct vnode **vpp, __unused vfs_context_t ctx)
{
	return EINVAL;
}


static int
devfs_vptofh(__unused struct vnode *vp, __unused int *fhlenp, __unused unsigned char *fhp, __unused vfs_context_t ctx)
{
	return EINVAL;
}

static int
devfs_sysctl(__unused int *name, __unused u_int namelen, __unused user_addr_t oldp,
    __unused size_t *oldlenp, __unused user_addr_t newp,
    __unused size_t newlen, __unused vfs_context_t ctx)
{
	return ENOTSUP;
}

#include <sys/namei.h>

  
                               
           
                                                                 
   
int
devfs_kernel_mount(char * mntname)
{
	int error;
	vfs_context_t ctx = vfs_context_kernel();
	char fsname[] = "devfs";

	error = kernel_mount(fsname, NULLVP, NULLVP, mntname, NULL, 0, MNT_DONTBROWSE, KERNEL_MOUNT_NOAUTH | KERNEL_MOUNT_DEVFS, ctx);
	if (error) {
		printf("devfs_kernel_mount: kernel_mount failed: %d\n", error);
		return error;
	}

	return 0;
}

const struct vfsops devfs_vfsops = {
	.vfs_mount   = devfs_mount,
	.vfs_start   = devfs_start,
	.vfs_unmount = devfs_unmount,
	.vfs_root    = devfs_root,
	.vfs_getattr = devfs_vfs_getattr,
	.vfs_sync    = devfs_sync,
	.vfs_vget    = devfs_vget,
	.vfs_fhtovp  = devfs_fhtovp,
	.vfs_vptofh  = devfs_vptofh,
	.vfs_init    = devfs_init,
	.vfs_sysctl  = devfs_sysctl,
	                                                 
};
