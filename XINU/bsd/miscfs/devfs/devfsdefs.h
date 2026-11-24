/*
 * Copyright (c) 2000-2002 Apple Computer, Inc. All rights reserved.
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
 * devfsdefs.h
 */
  
                                                                         
                                                                          
                                                                        
               
   

  
          
                                                   
                             
                                     
                                                        
   

#ifndef __DEVFS_DEVFSDEFS_H__
#define __DEVFS_DEVFSDEFS_H__

#include  <sys/appleapiopts.h>

__BEGIN_DECLS
#ifdef __APPLE_API_PRIVATE
#define DEVMAXNAMESIZE  32                       
#define DEVMAXPATHSIZE  128                      

typedef enum {
	DEV_DIR,
	DEV_BDEV,
	DEV_CDEV,
	DEV_SLNK,
#if FDESC
	DEV_DEVFD
#endif            
} devfstype_t;

extern int(**devfs_vnodeop_p)(void *);                                     
extern int(**devfs_spec_vnodeop_p)(void *);                                     
extern const struct vfsops devfs_vfsops;

typedef struct devnode          devnode_t;
typedef struct devdirent        devdirent_t;
typedef union devnode_type      devnode_type_t;

struct devfs_stats {
	int                 nodes;
	int                 entries;
	int                 mounts;
	int                 stringspace;
};

union devnode_type {
	dev_t               dev;
	struct {
		devdirent_t *   dirlist;
		devdirent_t * * dirlast;
		devnode_t *     parent;
		devdirent_t *   myname;                     
		int             entrycount;
	}Dir;
	struct {
		char *          name;                                  
		size_t          namelen;
	}Slnk;
};

#define DEV_MAX_VNODE_RETRY  8                                                 
                                                                   
struct devnode {
	devfstype_t         dn_type;
	  
                                                                 
                                                              
                                                                   
                                                                 
                                                                 
                                                                  
                                                                  
                                                                
                                  
    
	os_ref_atomic_t     dn_refcount;
	u_short             dn_mode;
	uid_t               dn_uid;
	gid_t               dn_gid;
	struct timespec     dn_atime;                         
	struct timespec     dn_mtime;                               
	struct timespec     dn_ctime;                       
	int(***dn_ops)(void *);                                           
	int                 dn_links;                                              
	struct devfsmount * dn_dvm;                                           
	struct vnode *      dn_vn;                                               
	int                 dn_len;                                            
	devdirent_t *       dn_linklist;                                             
	devnode_t *         dn_nextsibling;                                  
	devnode_t * *       dn_prevsiblingp;                               
	devnode_type_t      dn_typeinfo;
	int                 dn_change;
	int                 dn_update;
	int                 dn_access;
	int                 dn_lflags;
	ino_t               dn_ino;
	int                 (*dn_clone)(dev_t dev, int action);                 
	struct label *      dn_label;                       
};

#define DN_DELETE               0x02
#define DN_CREATE               0x04
#define DN_CREATEWAIT           0x08


struct devdirent {
	                                                              
	char                de_name[DEVMAXNAMESIZE];
	devnode_t *         de_dnp;                                        
	devnode_t *         de_parent;                                           
	devdirent_t *       de_next;                                       
	devdirent_t *       *de_prevp;                                                 
	devdirent_t *       de_nextlink;                                
	devdirent_t *       *de_prevlinkp;                                             
};

extern devdirent_t *            dev_root;
extern struct devfs_stats       devfs_stats;
extern lck_mtx_t                devfs_mutex;
extern lck_mtx_t                devfs_attr_mutex;

  
                         
                                                             
                     
                                                          
                                          
   

  
                                                                      
                                     
   
struct devfsmount {
	struct mount *      mount;                                  
	devdirent_t *       plane_root;                              
};

  
                                                     
   
#include <sys/lock.h>
#include <miscfs/devfs/devfs_proto.h>
#include <libkern/OSAtomic.h>                                           

                             

          
#define M_DEVFSNAME     M_DEVFS
#define M_DEVFSNODE     M_DEVFS
#define M_DEVFSMNT      M_DEVFS

#define VTODN(vp)       ((devnode_t *)(vp)->v_data)

#define DEVFS_LOCK()    lck_mtx_lock(&devfs_mutex)
#define DEVFS_UNLOCK()  lck_mtx_unlock(&devfs_mutex)

#define DEVFS_ATTR_LOCK_SPIN()  lck_mtx_lock_spin(&devfs_attr_mutex);
#define DEVFS_ATTR_UNLOCK()     lck_mtx_unlock(&devfs_attr_mutex);

  
                                                                        
   
static __inline__ void
DEVFS_INCR_ENTRIES(void)
{
	OSAddAtomic(1, &devfs_stats.entries);
}

static __inline__ void
DEVFS_DECR_ENTRIES(void)
{
	OSAddAtomic(-1, &devfs_stats.entries);
}

static __inline__ void
DEVFS_INCR_NODES(void)
{
	OSAddAtomic(1, &devfs_stats.nodes);
}

static __inline__ void
DEVFS_DECR_NODES(void)
{
	OSAddAtomic(-1, &devfs_stats.nodes);
}

static __inline__ void
DEVFS_INCR_MOUNTS(void)
{
	OSAddAtomic(1, &devfs_stats.mounts);
}

static __inline__ void
DEVFS_DECR_MOUNTS(void)
{
	OSAddAtomic(-1, &devfs_stats.mounts);
}

static __inline__ void
DEVFS_INCR_STRINGSPACE(int space)
{
	OSAddAtomic(space, &devfs_stats.stringspace);
}

static __inline__ void
DEVFS_DECR_STRINGSPACE(int space)
{
	OSAddAtomic(-space, &devfs_stats.stringspace);
}

  
                                                                     
                                                                      
                                                       
  
                                                                           
                                                                                  
                                                                          
                                                                                   
                                                                                      
                                 
   
#define DEVFS_LAZY_UPDATE_SECONDS       1

#define DEVFS_UPDATE_CHANGE             0x1
#define DEVFS_UPDATE_MOD                0x2
#define DEVFS_UPDATE_ACCESS             0x4

static __inline__ void
dn_copy_times(devnode_t * target, devnode_t * source)
{
	DEVFS_ATTR_LOCK_SPIN();
	target->dn_atime = source->dn_atime;
	target->dn_mtime = source->dn_mtime;
	target->dn_ctime = source->dn_ctime;
	DEVFS_ATTR_UNLOCK();
	return;
}

#ifdef BSD_KERNEL_PRIVATE
int     devfs_make_symlink(devnode_t *dir_p, char *name, mode_t mode, char *target, devdirent_t **newent);
#endif                         

#endif                          

__END_DECLS

#endif                            
