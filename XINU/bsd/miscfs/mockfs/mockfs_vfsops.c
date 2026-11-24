/*
 * Copyright (c) 2012 Apple Inc. All rights reserved.
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

#include <kern/assert.h>
#include <kern/debug.h>
#include <libkern/libkern.h>
#include <miscfs/mockfs/mockfs.h>
#include <miscfs/mockfs/mockfs_fsnode.h>
#include <miscfs/mockfs/mockfs_vnops.h>
#include <miscfs/specfs/specdev.h>
#include <sys/disk.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/mount_internal.h>
#include <sys/vnode_internal.h>

static LCK_GRP_DECLARE(mockfs_mtx_grp, "mockfs-mutex");

int mockfs_mountroot(mount_t mp, vnode_t rvp, __unused vfs_context_t ctx);

  
                                                          
   
int mockfs_unmount(__unused struct mount *mp, __unused int mntflags, __unused vfs_context_t ctx);
int mockfs_root(mount_t mp, vnode_t * vpp, __unused vfs_context_t ctx);
int mockfs_sync(__unused struct mount *mp, __unused int waitfor, __unused vfs_context_t ctx);
int mockfs_init(__unused struct vfsconf * vfsc);

  
                    
                                                                                                               
                                                                                                            
                                                                                                                  
                                                                                                           
                                                           
  
                                     
   
int
mockfs_mountroot(mount_t mp, vnode_t rvp, __unused vfs_context_t ctx)
{
	int rvalue = 0;
	mockfs_fsnode_t root_fsnode = NULL;
	mockfs_fsnode_t dev_fsnode = NULL;
	mockfs_fsnode_t file_fsnode = NULL;
	mockfs_mount_t mockfs_mount_data = NULL;
	dk_memdev_info_t memdev_info;

	  
                                                                                                          
                                                                
    

	  
                                                                      
    

	MALLOC(mockfs_mount_data, mockfs_mount_t, sizeof(*mockfs_mount_data), M_TEMP, M_WAITOK | M_ZERO);
	mockfs_fsnode_create(mp, MOCKFS_ROOT, &root_fsnode);
	mockfs_fsnode_create(mp, MOCKFS_DEV, &dev_fsnode);
	mockfs_fsnode_create(mp, MOCKFS_FILE, &file_fsnode);

	if (!mockfs_mount_data || !root_fsnode || !dev_fsnode || !file_fsnode) {
		rvalue = ENOMEM;
		goto done;
	}

	  
                                                                                                          
                                                                                                        
                        
    
	bzero(&memdev_info, sizeof(memdev_info));

	if (!VNOP_IOCTL(rvp, DKIOCGETMEMDEVINFO, (caddr_t)&memdev_info, 0, NULL)) {
		  
                                                                   
     
		if (!mockfs_mount_data->mockfs_physical_memory) {
			mockfs_mount_data->mockfs_memory_backed = memdev_info.mi_mdev;
			mockfs_mount_data->mockfs_physical_memory = memdev_info.mi_phys;
			mockfs_mount_data->mockfs_memdev_base = memdev_info.mi_base;
			mockfs_mount_data->mockfs_memdev_size = memdev_info.mi_size;
		}
	}

	lck_mtx_init(&mockfs_mount_data->mockfs_mnt_mtx, &mockfs_mtx_grp, LCK_ATTR_NULL);

	  
                                                                                                        
                                       
    
	if ((rvalue = mockfs_fsnode_adopt(root_fsnode, dev_fsnode))) {
		goto done;
	}

	if ((rvalue = mockfs_fsnode_adopt(root_fsnode, file_fsnode))) {
		goto done;
	}

	mockfs_mount_data->mockfs_root = root_fsnode;
	mp->mnt_data = (typeof(mp->mnt_data))mockfs_mount_data;

done:
	if (rvalue) {
		if (file_fsnode) {
			mockfs_fsnode_destroy(file_fsnode);
		}
		if (dev_fsnode) {
			mockfs_fsnode_destroy(dev_fsnode);
		}
		if (root_fsnode) {
			mockfs_fsnode_destroy(root_fsnode);
		}
		if (mockfs_mount_data) {
			lck_mtx_destroy(&mockfs_mount_data->mockfs_mnt_mtx, &mockfs_mtx_grp);
			FREE(mockfs_mount_data, M_TEMP);
		}
	}

	return rvalue;
}

  
                  
                                                                                                               
  
                                     
   
int
mockfs_unmount(struct mount *mp, int mntflags, __unused vfs_context_t ctx)
{
	int rvalue;
	int vflush_flags;
	mockfs_fsnode_t root_fsnode;
	mockfs_mount_t mockfs_mnt;

	vflush_flags = 0;
	mockfs_mnt = (mockfs_mount_t) mp->mnt_data;

	  
                                                                                                      
                                                   
    
	if (mntflags & MNT_FORCE) {
		vflush_flags |= FORCECLOSE;
	}

	rvalue = vflush(mp, NULL, vflush_flags);

	if (rvalue) {
		return rvalue;
	}

	  
                                                                                                  
                                                                                                  
                                                                                                 
                                                                                   
    
	root_fsnode = mockfs_mnt->mockfs_root;
	mockfs_mnt->mockfs_root = NULL;
	rvalue = mockfs_fsnode_destroy(root_fsnode);

	if (rvalue) {
		panic("mockfs_unmount: Failed to destroy the fsnode tree");
	}

	lck_mtx_destroy(&mockfs_mnt->mockfs_mnt_mtx, &mockfs_mtx_grp);
	FREE(mockfs_mnt, M_TEMP);
	mp->mnt_data = NULL;

	return rvalue;
}

  
               
                                                                                      
  
                                     
   
int
mockfs_root(mount_t mp, vnode_t * vpp, __unused vfs_context_t ctx)
{
	int rvalue;

	rvalue = mockfs_fsnode_vnode(((mockfs_mount_t) mp->mnt_data)->mockfs_root, vpp);
	return rvalue;
}

  
               
                                                         
  
             
   
int
mockfs_sync(__unused struct mount *mp, __unused int waitfor, __unused vfs_context_t ctx)
{
	return 0;
}

int
mockfs_init(__unused struct vfsconf * vfsc)
{
	return 0;
}

struct vfsops mockfs_vfsops = {
	.vfs_unmount = mockfs_unmount,
	.vfs_root = mockfs_root,
	.vfs_sync = mockfs_sync,
	.vfs_init = mockfs_init,
};
