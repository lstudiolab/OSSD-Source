/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
 * devfs_vnops.c
 */

  
          
                                                     
                                                      
                                                                   
                                                                   
                                                                     
                                         
                                                                      
                                      
                                                                   
                                              
                                                                   
                                                                             
                                                        
                                        
                                                                   
                                 
   
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/conf.h>
#include <sys/disklabel.h>
#include <sys/lock.h>
#include <sys/stat.h>
#include <sys/mount_internal.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/time.h>
#include <sys/vnode_internal.h>
#include <miscfs/specfs/specdev.h>
#include <sys/dirent.h>
#include <sys/vmmeter.h>
#include <sys/vm.h>
#include <sys/uio_internal.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#include "devfsdefs.h"
#include "devfs.h"

#if FDESC
#include "fdesc.h"
#endif            

static int              devfs_update(struct vnode *vp, struct timeval *access,
    struct timeval *modify);
void                    devfs_rele_node(devnode_t *);
static void             devfs_consider_time_update(devnode_t *dnp, uint32_t just_changed_flags);
static boolean_t        devfs_update_needed(long now_s, long last_s);
static boolean_t        devfs_is_name_protected(struct vnode *dvp, const char *name);
void                    dn_times_locked(devnode_t * dnp, struct timeval *t1, struct timeval *t2, struct timeval *t3, uint32_t just_changed_flags);
void                    dn_times_now(devnode_t *dnp, uint32_t just_changed_flags);
void                    dn_mark_for_delayed_times_update(devnode_t *dnp, uint32_t just_changed_flags);

void
dn_times_locked(devnode_t * dnp, struct timeval *t1, struct timeval *t2, struct timeval *t3, uint32_t just_changed_flags)
{
	lck_mtx_assert(&devfs_attr_mutex, LCK_MTX_ASSERT_OWNED);

	if (just_changed_flags & DEVFS_UPDATE_ACCESS) {
		dnp->dn_atime.tv_sec = t1->tv_sec;
		dnp->dn_atime.tv_nsec = t1->tv_usec * 1000;
		dnp->dn_access = 0;
	} else if (dnp->dn_access) {
		dnp->dn_atime.tv_sec = MIN(t1->tv_sec, dnp->dn_atime.tv_sec + DEVFS_LAZY_UPDATE_SECONDS);
		dnp->dn_atime.tv_nsec = t1->tv_usec * 1000;
		dnp->dn_access = 0;
	}

	if (just_changed_flags & DEVFS_UPDATE_MOD) {
		dnp->dn_mtime.tv_sec = t2->tv_sec;
		dnp->dn_mtime.tv_nsec = t2->tv_usec * 1000;
		dnp->dn_update = 0;
	} else if (dnp->dn_update) {
		dnp->dn_mtime.tv_sec = MIN(t2->tv_sec, dnp->dn_mtime.tv_sec + DEVFS_LAZY_UPDATE_SECONDS);
		dnp->dn_mtime.tv_nsec = t2->tv_usec * 1000;
		dnp->dn_update = 0;
	}

	if (just_changed_flags & DEVFS_UPDATE_CHANGE) {
		dnp->dn_ctime.tv_sec = t3->tv_sec;
		dnp->dn_ctime.tv_nsec = t3->tv_usec * 1000;
		dnp->dn_change = 0;
	} else if (dnp->dn_change) {
		dnp->dn_ctime.tv_sec = MIN(t3->tv_sec, dnp->dn_ctime.tv_sec + DEVFS_LAZY_UPDATE_SECONDS);
		dnp->dn_ctime.tv_nsec = t3->tv_usec * 1000;
		dnp->dn_change = 0;
	}
}

void
dn_mark_for_delayed_times_update(devnode_t *dnp, uint32_t just_changed_flags)
{
	if (just_changed_flags & DEVFS_UPDATE_CHANGE) {
		dnp->dn_change = 1;
	}
	if (just_changed_flags & DEVFS_UPDATE_ACCESS) {
		dnp->dn_access = 1;
	}
	if (just_changed_flags & DEVFS_UPDATE_MOD) {
		dnp->dn_update = 1;
	}
}

  
                                                                             
   
void
dn_times_now(devnode_t * dnp, uint32_t just_changed_flags)
{
	struct timeval now;

	DEVFS_ATTR_LOCK_SPIN();
	microtime(&now);
	dn_times_locked(dnp, &now, &now, &now, just_changed_flags);
	DEVFS_ATTR_UNLOCK();
}

  
                                                       
                                                            
                                                           
   
static boolean_t
devfs_is_name_protected(struct vnode *dvp, const char *name)
{
	  
                                                                  
                            
    
	if (!vnode_isvroot(dvp)) {
		return FALSE;
	}

	if ((strcmp("console", name) == 0) ||
	    (strcmp("tty", name) == 0) ||
	    (strcmp("null", name) == 0) ||
	    (strcmp("zero", name) == 0) ||
	    (strcmp("klog", name) == 0)) {
		return TRUE;
	}

	return FALSE;
}


  
                                                                     
                                                         
                                                                   
                                                                        
  
                                                                      
                                                                     
                                                                     
                                                                       
                                                                      
                                                                          
                                                                        
                                                                         
                                                                             
                            
  
                                   
  
                                   
                                                                      
                                                           
                                                        
                                                     
            
                                        
                    
         
                                                                     
                                                                       
                                          
                                                                   
                                    
                                                                           
  
                                                                          
   
static int
devfs_lookup(struct vnop_lookup_args *ap)
                           
                                                  
                                                       
                                                       
                                 
        
{
	struct componentname *cnp = ap->a_cnp;
	vfs_context_t ctx = cnp->cn_context;
	struct proc *p = vfs_context_proc(ctx);
	struct vnode *dir_vnode = ap->a_dvp;
	struct vnode **result_vnode = ap->a_vpp;
	devnode_t *   dir_node;                                           
	devnode_t *   node = NULL;                                          
	devdirent_t * nodename;
	int flags = cnp->cn_flags;
	int op = cnp->cn_nameiop;                                              
	int wantparent = flags & (LOCKPARENT | WANTPARENT);
	int error = 0;
	char    heldchar;                                                     

retry:

	*result_vnode = NULL;                             

	                                                                                  
	dir_node = VTODN(dir_vnode);

	  
                                                   
    
	if (dir_node->dn_type != DEV_DIR) {
		return ENOTDIR;
	}

	DEVFS_LOCK();
	  
                                          
    
	heldchar = cnp->cn_nameptr[cnp->cn_namelen];
	cnp->cn_nameptr[cnp->cn_namelen] = '\0';

	nodename = dev_findname(dir_node, cnp->cn_nameptr);
	  
                           
    
	cnp->cn_nameptr[cnp->cn_namelen] = heldchar;

	if (nodename) {
		                  
		node = nodename->de_dnp;

		                                                     
                                                           
                                                         
                                                         
     
		error = devfs_dntovn(node, result_vnode, p);
	}
	DEVFS_UNLOCK();

	if (error) {
		if (error == EAGAIN) {
			goto retry;
		}
		return error;
	}
	if (!nodename) {
		  
                                                  
                                                   
                                                  
    
                                                          
                                                           
                                
     
		if (!(flags & ISLASTCN) || !(op == CREATE || op == RENAME)) {
			return ENOENT;
		}
		  
                                                 
                                                 
                                                    
                                                       
                                                    
                                                        
    
                                                 
                                
     
		return EJUSTRETURN;
	}
	  
                                                               
                                                             
    

	  
                                               
                                                
                                                    
                                                          
                                                 
    
	if (op == DELETE && (flags & ISLASTCN)) {
		  
                                                           
     
		if (dir_node == node) {
			if (*result_vnode) {
				vnode_put(*result_vnode);
				*result_vnode = NULL;
			}
			if (((error = vnode_get(dir_vnode)) == 0)) {
				*result_vnode = dir_vnode;
			}
			return error;
		}
		return 0;
	}

	  
                                                   
                                                         
                                                     
                                     
    
	if (op == RENAME && wantparent && (flags & ISLASTCN)) {
		  
                                       
                                              
     
		if (dir_node == node) {
			error = EISDIR;
			goto drop_ref;
		}
		return 0;
	}

	  
                                                                   
                                                             
                                                             
                                                             
                         
                                                             
                                                             
                                                             
                                                            
                                                            
                                                                
                                                              
                                                                
                                                                  
                                                              
                                                                 
                                                              
                                                    
    
	if ((flags & ISDOTDOT) == 0 && dir_node == node) {
		if (*result_vnode) {
			vnode_put(*result_vnode);
			*result_vnode = NULL;
		}
		if ((error = vnode_get(dir_vnode))) {
			return error;
		}
		*result_vnode = dir_vnode;
	}
	return 0;

drop_ref:
	if (*result_vnode) {
		vnode_put(*result_vnode);
		*result_vnode = NULL;
	}
	return error;
}

static int
devfs_getattr(struct vnop_getattr_args *ap)
                            
                            
                                  
                             
                          
        
{
	struct vnode *vp = ap->a_vp;
	struct vnode_attr *vap = ap->a_vap;
	devnode_t *     file_node;
	struct timeval now;


	DEVFS_LOCK();
	file_node = VTODN(vp);

	VATTR_RETURN(vap, va_mode, file_node->dn_mode);

	  
                                                              
                                                             
                                                                     
    
	switch (file_node->dn_type) {
	case    DEV_DIR:
#if FDESC
	case    DEV_DEVFD:                            
#endif            
		VATTR_RETURN(vap, va_rdev, 0);
		vap->va_mode |= (S_IFDIR);
		break;
	case    DEV_CDEV:
		VATTR_RETURN(vap, va_rdev, vp->v_rdev);
		vap->va_mode |= (S_IFCHR);
		break;
	case    DEV_BDEV:
		VATTR_RETURN(vap, va_rdev, vp->v_rdev);
		vap->va_mode |= (S_IFBLK);
		break;
	case    DEV_SLNK:
		VATTR_RETURN(vap, va_rdev, 0);
		vap->va_mode |= (S_IFLNK);
		break;
	default:
		VATTR_RETURN(vap, va_rdev, 0);                          
	}
	VATTR_RETURN(vap, va_type, vp->v_type);
	VATTR_RETURN(vap, va_nlink, file_node->dn_links);
	VATTR_RETURN(vap, va_uid, file_node->dn_uid);
	VATTR_RETURN(vap, va_gid, file_node->dn_gid);
	VATTR_RETURN(vap, va_fsid, (uint32_t)VM_KERNEL_ADDRHASH(file_node->dn_dvm));
	VATTR_RETURN(vap, va_fileid, (uintptr_t)file_node->dn_ino);
	VATTR_RETURN(vap, va_data_size, file_node->dn_len);

	                                              
	if (vp->v_type == VBLK) {
		VATTR_RETURN(vap, va_iosize, BLKDEV_IOSIZE);
	} else if (vp->v_type == VCHR) {
		VATTR_RETURN(vap, va_iosize, MAXPHYSIO);
	} else {
		VATTR_RETURN(vap, va_iosize, (uint32_t)vp->v_mount->mnt_vfsstat.f_iosize);
	}


	DEVFS_ATTR_LOCK_SPIN();

	microtime(&now);
	dn_times_locked(file_node, &now, &now, &now, 0);

	                                                   
	if (file_node->dn_ctime.tv_sec == 0) {
		file_node->dn_ctime.tv_sec = boottime_sec();
		file_node->dn_ctime.tv_nsec = 0;
	}
	if (file_node->dn_mtime.tv_sec == 0) {
		file_node->dn_mtime = file_node->dn_ctime;
	}
	if (file_node->dn_atime.tv_sec == 0) {
		file_node->dn_atime = file_node->dn_ctime;
	}
	VATTR_RETURN(vap, va_change_time, file_node->dn_ctime);
	VATTR_RETURN(vap, va_modify_time, file_node->dn_mtime);
	VATTR_RETURN(vap, va_access_time, file_node->dn_atime);

	DEVFS_ATTR_UNLOCK();

	VATTR_RETURN(vap, va_gen, 0);
	VATTR_RETURN(vap, va_filerev, 0);
	VATTR_RETURN(vap, va_acl, NULL);

	                                                
	if (vnode_isvroot(vp)) {
		VATTR_RETURN(vap, va_flags, UF_HIDDEN);
	} else {
		VATTR_RETURN(vap, va_flags, 0);
	}

	DEVFS_UNLOCK();

	return 0;
}

static int
devfs_setattr(struct vnop_setattr_args *ap)
                             
                       
                             
                            
        
{
	struct vnode *vp = ap->a_vp;
	struct vnode_attr *vap = ap->a_vap;
	int error = 0;
	devnode_t *     file_node;
	struct timeval atimeval, mtimeval;

	DEVFS_LOCK();

	file_node = VTODN(vp);
	  
                                            
    
	if (VATTR_IS_ACTIVE(vap, va_access_time) || VATTR_IS_ACTIVE(vap, va_modify_time)) {
		if (VATTR_IS_ACTIVE(vap, va_access_time)) {
			file_node->dn_access = 1;
		}
		if (VATTR_IS_ACTIVE(vap, va_modify_time)) {
			file_node->dn_change = 1;
			file_node->dn_update = 1;
		}
		atimeval.tv_sec = vap->va_access_time.tv_sec;
		atimeval.tv_usec = (suseconds_t)(vap->va_access_time.tv_nsec / 1000);
		mtimeval.tv_sec = vap->va_modify_time.tv_sec;
		mtimeval.tv_usec = (suseconds_t)(vap->va_modify_time.tv_nsec / 1000);

		if ((error = devfs_update(vp, &atimeval, &mtimeval))) {
			goto exit;
		}
	}
	VATTR_SET_SUPPORTED(vap, va_access_time);
	VATTR_SET_SUPPORTED(vap, va_change_time);

	  
                           
    
	if (VATTR_IS_ACTIVE(vap, va_mode)) {
		file_node->dn_mode &= ~07777;
		file_node->dn_mode |= vap->va_mode & 07777;
	}
	VATTR_SET_SUPPORTED(vap, va_mode);

	  
                     
    
	if (VATTR_IS_ACTIVE(vap, va_uid)) {
		file_node->dn_uid = vap->va_uid;
	}
	VATTR_SET_SUPPORTED(vap, va_uid);

	  
                     
    
	if (VATTR_IS_ACTIVE(vap, va_gid)) {
		file_node->dn_gid = vap->va_gid;
	}
	VATTR_SET_SUPPORTED(vap, va_gid);
exit:
	DEVFS_UNLOCK();

	return error;
}

#if CONFIG_MACF
static int
devfs_setlabel(struct vnop_setlabel_args *ap)
                              
                                             
                                    
                                    
                                 
             
{
	struct vnode *vp;
	struct devnode *de;

	vp = ap->a_vp;
	de = VTODN(vp);

	mac_vnode_label_update(ap->a_context, vp, ap->a_vl);
	mac_devfs_label_update(vp->v_mount, de, vp);

	return 0;
}
#endif

static int
devfs_read(struct vnop_read_args *ap)
                          
                            
                           
                       
                                 
        
{
	devnode_t * dn_p = VTODN(ap->a_vp);

	switch (ap->a_vp->v_type) {
	case VDIR: {
		dn_p->dn_access = 1;

		return VNOP_READDIR(ap->a_vp, ap->a_uio, 0, NULL, NULL, ap->a_context);
	}
	default: {
		printf("devfs_read(): bad file type %d", ap->a_vp->v_type);
		return EINVAL;
	}
	}
}

static int
devfs_close(struct vnop_close_args *ap)
                           
                            
                      
                                 
        
{
	struct vnode *          vp = ap->a_vp;
	devnode_t *     dnp;

	if (vnode_isinuse(vp, 1)) {
		DEVFS_LOCK();
		dnp = VTODN(vp);
		if (dnp) {
			dn_times_now(dnp, 0);
		}
		DEVFS_UNLOCK();
	}
	return 0;
}

static int
devfsspec_close(struct vnop_close_args *ap)
                           
                            
                      
                                 
        
{
	struct vnode *          vp = ap->a_vp;
	devnode_t *     dnp;

	if (vnode_isinuse(vp, 0)) {
		DEVFS_LOCK();
		dnp = VTODN(vp);
		if (dnp) {
			dn_times_now(dnp, 0);
		}
		DEVFS_UNLOCK();
	}

	return VOCALL(spec_vnodeop_p, VOFFSET(vnop_close), ap);
}

static boolean_t
devfs_update_needed(long now_s, long last_s)
{
	if (now_s > last_s) {
		if (now_s - last_s >= DEVFS_LAZY_UPDATE_SECONDS) {
			return TRUE;
		}
	}

	return FALSE;
}

  
                                                                        
                                                                        
                                       
   
static void
devfs_consider_time_update(devnode_t *dnp, uint32_t just_changed_flags)
{
	struct timeval          now;
	long now_s;

	microtime(&now);
	now_s = now.tv_sec;

	if (dnp->dn_change || (just_changed_flags & DEVFS_UPDATE_CHANGE)) {
		if (devfs_update_needed(now_s, dnp->dn_ctime.tv_sec)) {
			dn_times_now(dnp, just_changed_flags);
			return;
		}
	}
	if (dnp->dn_access || (just_changed_flags & DEVFS_UPDATE_ACCESS)) {
		if (devfs_update_needed(now_s, dnp->dn_atime.tv_sec)) {
			dn_times_now(dnp, just_changed_flags);
			return;
		}
	}
	if (dnp->dn_update || (just_changed_flags & DEVFS_UPDATE_MOD)) {
		if (devfs_update_needed(now_s, dnp->dn_mtime.tv_sec)) {
			dn_times_now(dnp, just_changed_flags);
			return;
		}
	}

	                                                         
	dn_mark_for_delayed_times_update(dnp, just_changed_flags);

	return;
}

static int
devfsspec_read(struct vnop_read_args *ap)
                          
                            
                           
                       
                             
        
{
	devnode_t *     dnp = VTODN(ap->a_vp);

	devfs_consider_time_update(dnp, DEVFS_UPDATE_ACCESS);

	return VOCALL(spec_vnodeop_p, VOFFSET(vnop_read), ap);
}

static int
devfsspec_write(struct vnop_write_args *ap)
                            
                            
                           
                       
                                 
        
{
	devnode_t *     dnp = VTODN(ap->a_vp);

	devfs_consider_time_update(dnp, DEVFS_UPDATE_CHANGE | DEVFS_UPDATE_MOD);

	return VOCALL(spec_vnodeop_p, VOFFSET(vnop_write), ap);
}

  
                                      
   
static int
devfs_write(struct vnop_write_args *ap)
                            
                            
                           
                       
                             
        
{
	switch (ap->a_vp->v_type) {
	case VDIR:
		return EISDIR;
	default:
		printf("devfs_write(): bad file type %d", ap->a_vp->v_type);
		return EINVAL;
	}
}

  
                                                                      
                         
   
static int
devfs_vnop_remove(struct vnop_remove_args *ap)
                             
                             
                            
                                     
        
{
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	devnode_t *  tp;
	devnode_t *  tdp;
	devdirent_t * tnp;
	int doingdirectory = 0;
	int error = 0;

	  
                                                   
                                                    
                     
    

	DEVFS_LOCK();

	tp = VTODN(vp);
	tdp = VTODN(dvp);


	tnp = dev_findname(tdp, cnp->cn_nameptr);

	if (tnp == NULL) {
		error = ENOENT;
		goto abort;
	}

	  
                                               
    
	if (devfs_is_name_protected(dvp, cnp->cn_nameptr)) {
		error = EINVAL;
		goto abort;
	}

	  
                                                   
    
	if ((tp->dn_type) == DEV_DIR) {
		  
                                                             
     
		if ((cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.')
		    || (cnp->cn_flags & ISDOTDOT)) {
			error = EINVAL;
			goto abort;
		}
		doingdirectory++;
	}

	                                    
                                    
                                     
	devfs_consider_time_update(tdp, DEVFS_UPDATE_CHANGE | DEVFS_UPDATE_MOD);

	  
                                                         
                                                        
                                                
    
	if ((doingdirectory) && (tp->dn_links > 2)) {
		error = ENOTEMPTY;
		goto abort;
	}
	dev_free_name(tnp);
abort:
	DEVFS_UNLOCK();

	return error;
}

  
   
static int
devfs_link(struct vnop_link_args *ap)
                          
                              
                            
                                     
                                 
        
{
	struct vnode *vp = ap->a_vp;
	struct vnode *tdvp = ap->a_tdvp;
	struct componentname *cnp = ap->a_cnp;
	devnode_t * fp;
	devnode_t * tdp;
	devdirent_t * tnp;
	int error = 0;

	  
                                                    
    
	if (cnp->cn_namelen > DEVMAXNAMESIZE) {
		error = ENAMETOOLONG;
		goto out1;
	}

	  
                                                  
                                                     
                                                    
                     
    
	                                                                                 
	tdp = VTODN(tdvp);

	if (tdvp->v_mount != vp->v_mount) {
		return EXDEV;
	}
	DEVFS_LOCK();

	fp = VTODN(vp);

	                                    
                                    
                                     
	dn_times_now(fp, DEVFS_UPDATE_CHANGE);

	if (!error) {
		error = dev_add_name(cnp->cn_nameptr, tdp, NULL, fp, &tnp);
	}
out1:
	DEVFS_UNLOCK();

	return error;
}

  
                                                        
                             
                 
                 
                      
                 
                      
  
                                             
                                
                                                     
  
  
                      
  
                                                             
                                                              
                                                              
                                                   
                                                                 
                      
                                                           
                                                           
                                                               
                
   
static int
devfs_rename(struct vnop_rename_args *ap)
                            
                              
                             
                                      
                              
                             
                                      
                                 
        
{
	struct vnode *tvp = ap->a_tvp;
	struct vnode *tdvp = ap->a_tdvp;
	struct vnode *fvp = ap->a_fvp;
	struct vnode *fdvp = ap->a_fdvp;
	struct componentname *tcnp = ap->a_tcnp;
	struct componentname *fcnp = ap->a_fcnp;
	devnode_t *fp, *fdp, *tp, *tdp;
	devdirent_t *fnp, *tnp;
	int doingdirectory = 0;
	int error = 0;

	DEVFS_LOCK();
	  
                                                    
    
	if (tcnp->cn_namelen > DEVMAXNAMESIZE) {
		error = ENAMETOOLONG;
		goto out;
	}

	  
                                                     
                                                    
                     
    
	tdp = VTODN(tdvp);
	fdp = VTODN(fdvp);
	fp = VTODN(fvp);

	fnp = dev_findname(fdp, fcnp->cn_nameptr);

	if (fnp == NULL) {
		error = ENOENT;
		goto out;
	}
	tp = NULL;
	tnp = NULL;

	if (tvp) {
		tnp = dev_findname(tdp, tcnp->cn_nameptr);

		if (tnp == NULL) {
			error = ENOENT;
			goto out;
		}
		tp = VTODN(tvp);
	}

	  
                                                   
    
	if ((fp->dn_type) == DEV_DIR) {
		  
                                                             
     
		if ((fcnp->cn_namelen == 1 && fcnp->cn_nameptr[0] == '.')
		    || (fcnp->cn_flags & ISDOTDOT)
		    || (tcnp->cn_namelen == 1 && tcnp->cn_nameptr[0] == '.')
		    || (tcnp->cn_flags & ISDOTDOT)
		    || (tdp == fp)) {
			error = EINVAL;
			goto out;
		}
		doingdirectory++;
	}

	  
                                               
    
	if (devfs_is_name_protected(fdvp, fcnp->cn_nameptr) ||
	    devfs_is_name_protected(tdvp, tcnp->cn_nameptr)) {
		error = EINVAL;
		goto out;
	}

	  
                                                        
                                                        
                                                       
                                                      
                                                        
                                 
    
	if (doingdirectory && (tdp != fdp)) {
		devnode_t * tmp, *ntmp;
		tmp = tdp;
		do {
			if (tmp == fp) {
				                                    
				error = EINVAL;
				goto out;
			}
			ntmp = tmp;
		} while ((tmp = tmp->dn_typeinfo.Dir.parent) != ntmp);
	}

	                                    
                                    
                                     
	dn_times_now(fp, DEVFS_UPDATE_CHANGE);

	  
                                       
    
	if (fvp == tvp) {
		if (fvp->v_type == VDIR) {
			error = EINVAL;
			goto out;
		}
		                                     
		dev_free_name(fnp);

		DEVFS_UNLOCK();
		return 0;
	}
	  
                                               
                                            
                                       
    
	fp->dn_links++;
	  
                                                                   
                                      
    
	if (tp) {
		  
                                                          
                                                         
                                                 
     
		if ((doingdirectory) && (tp->dn_links > 2)) {
			error = ENOTEMPTY;
			goto bad;
		}
		dev_free_name(tnp);
		tp = NULL;
	}
	dev_add_name(tcnp->cn_nameptr, tdp, NULL, fp, &tnp);
	fnp->de_dnp = NULL;
	fp->dn_links--;                            

	dev_free_name(fnp);
bad:
	fp->dn_links--;                          
out:
	DEVFS_UNLOCK();
	return error;
}

static int
devfs_mkdir(struct vnop_mkdir_args *ap)
                          
                             
                              
                                     
                                  
                                 
        
{
	struct componentname * cnp = ap->a_cnp;
	vfs_context_t ctx = cnp->cn_context;
	struct proc *p = vfs_context_proc(ctx);
	int error = 0;
	devnode_t * dir_p;
	devdirent_t * nm_p;
	devnode_t * dev_p;
	struct vnode_attr *     vap = ap->a_vap;
	struct vnode * * vpp = ap->a_vpp;

	DEVFS_LOCK();

	dir_p = VTODN(ap->a_dvp);
	error = dev_add_entry(cnp->cn_nameptr, dir_p, DEV_DIR,
	    NULL, NULL, NULL, &nm_p);
	if (error) {
		goto failure;
	}
	dev_p = nm_p->de_dnp;
	dev_p->dn_uid = dir_p->dn_uid;
	dev_p->dn_gid = dir_p->dn_gid;
	dev_p->dn_mode = vap->va_mode;
	dn_copy_times(dev_p, dir_p);

	error = devfs_dntovn(dev_p, vpp, p);
failure:
	DEVFS_UNLOCK();

	return error;
}

  
                                                                          
                                                                       
                                                                        
   
static int
devfs_rmdir(struct vnop_rmdir_args *ap)
                           
                             
                            
                                     
                                 
        
{
	struct vnop_remove_args ra;

	ra.a_dvp = ap->a_dvp;
	ra.a_vp = ap->a_vp;
	ra.a_cnp = ap->a_cnp;
	ra.a_flags = 0;                  
	ra.a_context = ap->a_context;

	return devfs_vnop_remove(&ra);
}


static int
devfs_symlink(struct vnop_symlink_args *ap)
                            
                             
                              
                                     
                                  
                        
                                 
        
{
	int error;
	devdirent_t *newent;

	DEVFS_LOCK();
	error = devfs_make_symlink(VTODN(ap->a_dvp), ap->a_cnp->cn_nameptr, ap->a_vap->va_mode, ap->a_target, &newent);

	if (error == 0) {
		error = devfs_dntovn(newent->de_dnp, ap->a_vpp, vfs_context_proc(ap->a_context));
	}

	DEVFS_UNLOCK();

	return error;
}

                              
int
devfs_make_symlink(devnode_t *dir_p, char *name, mode_t mode, char *target, devdirent_t **newent)
{
	int error = 0;
	devnode_type_t typeinfo;
	devdirent_t * nm_p;
	devnode_t * dev_p;

	typeinfo.Slnk.name = target;
	typeinfo.Slnk.namelen = strlen(target);

	error = dev_add_entry(name, dir_p, DEV_SLNK,
	    &typeinfo, NULL, NULL, &nm_p);
	if (error) {
		goto failure;
	}
	dev_p = nm_p->de_dnp;
	dev_p->dn_uid = dir_p->dn_uid;
	dev_p->dn_gid = dir_p->dn_gid;
	dev_p->dn_mode = mode;
	dn_copy_times(dev_p, dir_p);

	if (newent) {
		*newent = nm_p;
	}

failure:

	return error;
}

  
                   
   
static int
devfs_mknod(struct vnop_mknod_args *ap)
                           
                             
                              
                                     
                                  
                                 
        
{
	struct componentname * cnp = ap->a_cnp;
	vfs_context_t ctx = cnp->cn_context;
	struct proc *p = vfs_context_proc(ctx);
	devnode_t *     dev_p;
	devdirent_t *   devent;
	devnode_t *     dir_p;                                    
	struct vnode *  dvp = ap->a_dvp;
	int             error = 0;
	devnode_type_t  typeinfo;
	struct vnode_attr *     vap = ap->a_vap;
	struct vnode ** vpp = ap->a_vpp;

	*vpp = NULL;
	if (!(vap->va_type == VBLK) && !(vap->va_type == VCHR)) {
		return EINVAL;                                          
	}
	typeinfo.dev = vap->va_rdev;

	DEVFS_LOCK();

	dir_p = VTODN(dvp);

	error = dev_add_entry(cnp->cn_nameptr, dir_p,
	    (vap->va_type == VBLK) ? DEV_BDEV : DEV_CDEV,
	    &typeinfo, NULL, NULL, &devent);
	if (error) {
		goto failure;
	}
	dev_p = devent->de_dnp;
	error = devfs_dntovn(dev_p, vpp, p);
	if (error) {
		goto failure;
	}
	dev_p->dn_uid = vap->va_uid;
	dev_p->dn_gid = vap->va_gid;
	dev_p->dn_mode = vap->va_mode;
	VATTR_SET_SUPPORTED(vap, va_uid);
	VATTR_SET_SUPPORTED(vap, va_gid);
	VATTR_SET_SUPPORTED(vap, va_mode);
failure:
	DEVFS_UNLOCK();

	return error;
}

  
                       
   
static int
devfs_readdir(struct vnop_readdir_args *ap)
                            
                            
                           
                     
                        
                          
                                 
        
{
	struct vnode *vp = ap->a_vp;
	struct uio *uio = ap->a_uio;
	struct dirent dirent;
	devnode_t * dir_node;
	devdirent_t *   name_node;
	const char *name;
	int error = 0;
	int reclen;
	int nodenumber;
	off_t startpos, pos;

	if (ap->a_flags & (VNODE_READDIR_EXTENDED | VNODE_READDIR_REQSEEKOFF)) {
		return EINVAL;
	}

	                         
	dir_node = VTODN(vp);
	if (dir_node->dn_type != DEV_DIR) {
		return ENOTDIR;
	}
	pos = 0;
	startpos = uio->uio_offset;

	DEVFS_LOCK();

	name_node = dir_node->dn_typeinfo.Dir.dirlist;
	nodenumber = 0;

	while ((name_node || (nodenumber < 2)) && (uio_resid(uio) > 0)) {
		switch (nodenumber) {
		case    0:
			dirent.d_fileno = dir_node->dn_ino;
			name = ".";
			dirent.d_namlen = 1;
			dirent.d_type = DT_DIR;
			break;
		case    1:
			if (dir_node->dn_typeinfo.Dir.parent) {
				dirent.d_fileno = dir_node->dn_typeinfo.Dir.parent->dn_ino;
			} else {
				dirent.d_fileno = dir_node->dn_ino;
			}
			name = "..";
			dirent.d_namlen = 2;
			dirent.d_type = DT_DIR;
			break;
		default:
			dirent.d_fileno = name_node->de_dnp->dn_ino;
			dirent.d_namlen = (__uint8_t) strlen(name_node->de_name);
			name = name_node->de_name;
			switch (name_node->de_dnp->dn_type) {
			case DEV_BDEV:
				dirent.d_type = DT_BLK;
				break;
			case DEV_CDEV:
				dirent.d_type = DT_CHR;
				break;
			case DEV_DIR:
				dirent.d_type = DT_DIR;
				break;
			case DEV_SLNK:
				dirent.d_type = DT_LNK;
				break;
			default:
				dirent.d_type = DT_UNKNOWN;
			}
		}
#define GENERIC_DIRSIZ(dp) \
    ((sizeof (struct dirent) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))

		reclen = dirent.d_reclen = GENERIC_DIRSIZ(&dirent);

		if (pos >= startpos) {                                   
			if (uio_resid(uio) < reclen) {                   
				break;
			}
			strlcpy(dirent.d_name, name, DEVMAXNAMESIZE);
			if ((error = uiomove((caddr_t)&dirent,
			    dirent.d_reclen, uio)) != 0) {
				break;
			}
		}
		pos += reclen;
		if ((nodenumber > 1) && name_node) {
			name_node = name_node->de_next;
		}
		nodenumber++;
	}
	DEVFS_UNLOCK();
	uio->uio_offset = pos;

	devfs_consider_time_update(dir_node, DEVFS_UPDATE_ACCESS);

	return error;
}


  
   
static int
devfs_readlink(struct vnop_readlink_args *ap)
                             
                            
                           
                                 
        
{
	struct vnode *vp = ap->a_vp;
	struct uio *uio = ap->a_uio;
	devnode_t * lnk_node;
	int error = 0;

	                         
	lnk_node = VTODN(vp);

	if (lnk_node->dn_type != DEV_SLNK) {
		error = EINVAL;
		goto out;
	}
	error = uiomove(lnk_node->dn_typeinfo.Slnk.name,
	    (int)lnk_node->dn_typeinfo.Slnk.namelen, uio);
out:
	return error;
}

static int
devfs_reclaim(struct vnop_reclaim_args *ap)
                            
                            
        
{
	struct vnode *      vp = ap->a_vp;
	devnode_t *         dnp;

	DEVFS_LOCK();

	dnp = VTODN(vp);

	if (dnp) {
		                                                                
		dnp->dn_vn = NULL;
		vnode_clearfsnode(vp);

		                                                          
		devfs_rele_node(dnp);
	}
	DEVFS_UNLOCK();

	return 0;
}


  
                                       
   
static int
devs_vnop_pathconf(
	struct vnop_pathconf_args     
                                                         
                                                 
                                                    
                                                              
                                          *ap)
{
	switch (ap->a_name) {
	case _PC_LINK_MAX:
		                                                           
		*ap->a_retval = 32767;
		break;
	case _PC_NAME_MAX:
		*ap->a_retval = DEVMAXNAMESIZE - 1;                       
		break;
	case _PC_PATH_MAX:
		*ap->a_retval = DEVMAXPATHSIZE - 1;                            
		break;
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 200112;                                      
		break;
	case _PC_NO_TRUNC:
		*ap->a_retval = 0;
		break;
	case _PC_CASE_SENSITIVE:
		*ap->a_retval = 1;
		break;
	case _PC_CASE_PRESERVING:
		*ap->a_retval = 1;
		break;
	default:
		return EINVAL;
	}

	return 0;
}



                                                                            
              
                                                                            

  
  
                              
                       
                            
    
   

static int
devfs_inactive(__unused struct vnop_inactive_args *ap)
{
	vnode_t vp = ap->a_vp;
	devnode_t *dnp = VTODN(vp);

	  
                                                     
                         
    
	if (dnp->dn_clone != NULL) {
		vnode_recycle(vp);
	}

	return 0;
}

  
                              
   
static int
devfs_update(struct vnode *vp, struct timeval *access, struct timeval *modify)
{
	devnode_t * ip;
	struct timeval now;

	ip = VTODN(vp);
	if (vp->v_mount->mnt_flag & MNT_RDONLY) {
		ip->dn_access = 0;
		ip->dn_change = 0;
		ip->dn_update = 0;

		return 0;
	}

	DEVFS_ATTR_LOCK_SPIN();
	microtime(&now);
	dn_times_locked(ip, access, modify, &now, DEVFS_UPDATE_ACCESS | DEVFS_UPDATE_MOD);
	DEVFS_ATTR_UNLOCK();

	return 0;
}

#define VOPFUNC int (*)(void *)

                                                            
int(**devfs_vnodeop_p)(void *);
const static struct vnodeopv_entry_desc devfs_vnodeop_entries[] = {
	{ .opve_op = &vnop_default_desc, .opve_impl = (VOPFUNC)vn_default_error },
	{ .opve_op = &vnop_lookup_desc, .opve_impl = (VOPFUNC)devfs_lookup },                       
	{ .opve_op = &vnop_create_desc, .opve_impl = (VOPFUNC)err_create },                         
	{ .opve_op = &vnop_whiteout_desc, .opve_impl = (VOPFUNC)err_whiteout },                       
	{ .opve_op = &vnop_mknod_desc, .opve_impl = (VOPFUNC)devfs_mknod },                        
	{ .opve_op = &vnop_open_desc, .opve_impl = (VOPFUNC)nop_open },                           
	{ .opve_op = &vnop_close_desc, .opve_impl = (VOPFUNC)devfs_close },                        
	{ .opve_op = &vnop_getattr_desc, .opve_impl = (VOPFUNC)devfs_getattr },                      
	{ .opve_op = &vnop_setattr_desc, .opve_impl = (VOPFUNC)devfs_setattr },                      
	{ .opve_op = &vnop_read_desc, .opve_impl = (VOPFUNC)devfs_read },                         
	{ .opve_op = &vnop_write_desc, .opve_impl = (VOPFUNC)devfs_write },                        
	{ .opve_op = &vnop_ioctl_desc, .opve_impl = (VOPFUNC)err_ioctl },                          
	{ .opve_op = &vnop_select_desc, .opve_impl = (VOPFUNC)err_select },                         
	{ .opve_op = &vnop_revoke_desc, .opve_impl = (VOPFUNC)err_revoke },                         
	{ .opve_op = &vnop_mmap_desc, .opve_impl = (VOPFUNC)err_mmap },                           
	{ .opve_op = &vnop_fsync_desc, .opve_impl = (VOPFUNC)nop_fsync },                          
	{ .opve_op = &vnop_remove_desc, .opve_impl = (VOPFUNC)devfs_vnop_remove },                  
	{ .opve_op = &vnop_link_desc, .opve_impl = (VOPFUNC)devfs_link },                         
	{ .opve_op = &vnop_rename_desc, .opve_impl = (VOPFUNC)devfs_rename },                       
	{ .opve_op = &vnop_mkdir_desc, .opve_impl = (VOPFUNC)devfs_mkdir },                        
	{ .opve_op = &vnop_rmdir_desc, .opve_impl = (VOPFUNC)devfs_rmdir },                        
	{ .opve_op = &vnop_symlink_desc, .opve_impl = (VOPFUNC)devfs_symlink },                      
	{ .opve_op = &vnop_readdir_desc, .opve_impl = (VOPFUNC)devfs_readdir },                      
	{ .opve_op = &vnop_readlink_desc, .opve_impl = (VOPFUNC)devfs_readlink },                     
	{ .opve_op = &vnop_inactive_desc, .opve_impl = (VOPFUNC)devfs_inactive },                     
	{ .opve_op = &vnop_reclaim_desc, .opve_impl = (VOPFUNC)devfs_reclaim },                      
	{ .opve_op = &vnop_strategy_desc, .opve_impl = (VOPFUNC)err_strategy },                       
	{ .opve_op = &vnop_pathconf_desc, .opve_impl = (VOPFUNC)devs_vnop_pathconf },                 
	{ .opve_op = &vnop_advlock_desc, .opve_impl = (VOPFUNC)err_advlock },                        
	{ .opve_op = &vnop_bwrite_desc, .opve_impl = (VOPFUNC)err_bwrite },
	{ .opve_op = &vnop_pagein_desc, .opve_impl = (VOPFUNC)err_pagein },                         
	{ .opve_op = &vnop_pageout_desc, .opve_impl = (VOPFUNC)err_pageout },                        
	{ .opve_op = &vnop_copyfile_desc, .opve_impl = (VOPFUNC)err_copyfile },                       
	{ .opve_op = &vnop_blktooff_desc, .opve_impl = (VOPFUNC)err_blktooff },                       
	{ .opve_op = &vnop_offtoblk_desc, .opve_impl = (VOPFUNC)err_offtoblk },                       
	{ .opve_op = &vnop_blockmap_desc, .opve_impl = (VOPFUNC)err_blockmap },                       
#if CONFIG_MACF
	{ .opve_op = &vnop_setlabel_desc, .opve_impl = (VOPFUNC)devfs_setlabel },                     
#endif
	{ .opve_op = (struct vnodeop_desc*)NULL, .opve_impl = (int (*)(void *))NULL }
};
const struct vnodeopv_desc devfs_vnodeop_opv_desc =
{ .opv_desc_vector_p = &devfs_vnodeop_p, .opv_desc_ops = devfs_vnodeop_entries };

                                                    
int(**devfs_spec_vnodeop_p)(void *);
const static struct vnodeopv_entry_desc devfs_spec_vnodeop_entries[] = {
	{ .opve_op = &vnop_default_desc, .opve_impl = (VOPFUNC)vn_default_error },
	{ .opve_op = &vnop_lookup_desc, .opve_impl = (VOPFUNC)spec_lookup },                        
	{ .opve_op = &vnop_create_desc, .opve_impl = (VOPFUNC)spec_create },                        
	{ .opve_op = &vnop_mknod_desc, .opve_impl = (VOPFUNC)spec_mknod },                         
	{ .opve_op = &vnop_open_desc, .opve_impl = (VOPFUNC)spec_open },                                  
	{ .opve_op = &vnop_close_desc, .opve_impl = (VOPFUNC)devfsspec_close },                    
	{ .opve_op = &vnop_getattr_desc, .opve_impl = (VOPFUNC)devfs_getattr },                      
	{ .opve_op = &vnop_setattr_desc, .opve_impl = (VOPFUNC)devfs_setattr },                      
	{ .opve_op = &vnop_read_desc, .opve_impl = (VOPFUNC)devfsspec_read },                     
	{ .opve_op = &vnop_write_desc, .opve_impl = (VOPFUNC)devfsspec_write },                    
	{ .opve_op = &vnop_ioctl_desc, .opve_impl = (VOPFUNC)spec_ioctl },                         
	{ .opve_op = &vnop_select_desc, .opve_impl = (VOPFUNC)spec_select },                        
	{ .opve_op = &vnop_revoke_desc, .opve_impl = (VOPFUNC)spec_revoke },                        
	{ .opve_op = &vnop_mmap_desc, .opve_impl = (VOPFUNC)spec_mmap },                                  
	{ .opve_op = &vnop_fsync_desc, .opve_impl = (VOPFUNC)spec_fsync },                         
	{ .opve_op = &vnop_remove_desc, .opve_impl = (VOPFUNC)devfs_vnop_remove },                  
	{ .opve_op = &vnop_link_desc, .opve_impl = (VOPFUNC)devfs_link },                         
	{ .opve_op = &vnop_rename_desc, .opve_impl = (VOPFUNC)spec_rename },                        
	{ .opve_op = &vnop_mkdir_desc, .opve_impl = (VOPFUNC)spec_mkdir },                         
	{ .opve_op = &vnop_rmdir_desc, .opve_impl = (VOPFUNC)spec_rmdir },                         
	{ .opve_op = &vnop_symlink_desc, .opve_impl = (VOPFUNC)spec_symlink },                       
	{ .opve_op = &vnop_readdir_desc, .opve_impl = (VOPFUNC)spec_readdir },                       
	{ .opve_op = &vnop_readlink_desc, .opve_impl = (VOPFUNC)spec_readlink },                              
	{ .opve_op = &vnop_inactive_desc, .opve_impl = (VOPFUNC)devfs_inactive },                     
	{ .opve_op = &vnop_reclaim_desc, .opve_impl = (VOPFUNC)devfs_reclaim },                      
	{ .opve_op = &vnop_strategy_desc, .opve_impl = (VOPFUNC)spec_strategy },                              
	{ .opve_op = &vnop_pathconf_desc, .opve_impl = (VOPFUNC)spec_pathconf },                              
	{ .opve_op = &vnop_advlock_desc, .opve_impl = (VOPFUNC)spec_advlock },                       
	{ .opve_op = &vnop_bwrite_desc, .opve_impl = (VOPFUNC)vn_bwrite },
	{ .opve_op = &vnop_pagein_desc, .opve_impl = (VOPFUNC)err_pagein },                         
	{ .opve_op = &vnop_pageout_desc, .opve_impl = (VOPFUNC)err_pageout },                        
	{ .opve_op = &vnop_copyfile_desc, .opve_impl = (VOPFUNC)err_copyfile },                       
	{ .opve_op = &vnop_blktooff_desc, .opve_impl = (VOPFUNC)spec_blktooff },                      
	{ .opve_op = &vnop_blktooff_desc, .opve_impl = (VOPFUNC)spec_offtoblk  },                        
	{ .opve_op = &vnop_blockmap_desc, .opve_impl = (VOPFUNC)spec_blockmap },                      
#if CONFIG_MACF
	{ .opve_op = &vnop_setlabel_desc, .opve_impl = (VOPFUNC)devfs_setlabel },                     
#endif
	{ .opve_op = (struct vnodeop_desc*)NULL, .opve_impl = (int (*)(void *))NULL }
};
const struct vnodeopv_desc devfs_spec_vnodeop_opv_desc =
{ .opv_desc_vector_p = &devfs_spec_vnodeop_p, .opv_desc_ops = devfs_spec_vnodeop_entries };


#if FDESC
int(**devfs_devfd_vnodeop_p)(void*);
const static struct vnodeopv_entry_desc devfs_devfd_vnodeop_entries[] = {
	{ .opve_op = &vnop_default_desc, .opve_impl = (VOPFUNC)vn_default_error },
	{ .opve_op = &vnop_lookup_desc, .opve_impl = (VOPFUNC)devfs_devfd_lookup},                  
	{ .opve_op = &vnop_open_desc, .opve_impl = (VOPFUNC)nop_open },                           
	{ .opve_op = &vnop_close_desc, .opve_impl = (VOPFUNC)devfs_close },                        
	{ .opve_op = &vnop_getattr_desc, .opve_impl = (VOPFUNC)devfs_getattr },                      
	{ .opve_op = &vnop_setattr_desc, .opve_impl = (VOPFUNC)devfs_setattr },                      
	{ .opve_op = &vnop_revoke_desc, .opve_impl = (VOPFUNC)err_revoke },                         
	{ .opve_op = &vnop_fsync_desc, .opve_impl = (VOPFUNC)nop_fsync },                          
	{ .opve_op = &vnop_readdir_desc, .opve_impl = (VOPFUNC)devfs_devfd_readdir},                         
	{ .opve_op = &vnop_inactive_desc, .opve_impl = (VOPFUNC)devfs_inactive },                     
	{ .opve_op = &vnop_reclaim_desc, .opve_impl = (VOPFUNC)devfs_reclaim },                      
	{ .opve_op = &vnop_pathconf_desc, .opve_impl = (VOPFUNC)devs_vnop_pathconf },                 
#if CONFIG_MACF
	{ .opve_op = &vnop_setlabel_desc, .opve_impl = (VOPFUNC)devfs_setlabel },                     
#endif
	{ .opve_op = (struct vnodeop_desc*)NULL, .opve_impl = (int (*)(void *))NULL }
};
const struct vnodeopv_desc devfs_devfd_vnodeop_opv_desc =
{ .opv_desc_vector_p = &devfs_devfd_vnodeop_p, .opv_desc_ops = devfs_devfd_vnodeop_entries};
#endif            
