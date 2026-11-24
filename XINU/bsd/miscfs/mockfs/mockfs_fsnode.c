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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <miscfs/mockfs/mockfs.h>
#include <miscfs/mockfs/mockfs_fsnode.h>
#include <miscfs/mockfs/mockfs_vnops.h>
#include <miscfs/specfs/specdev.h>
#include <sys/disk.h>
#include <sys/mount_internal.h>
#include <sys/ubc_internal.h>
#include <sys/vnode_internal.h>
#include <vm/vm_protos.h>

#include <libkern/libkern.h>

  
                                                                                               
                                                                                                       
                                                                                                     
                    
  
                                                                                                        
                                                                   
   

                        
                                                                                                       
                                                                                             
  
                                     
   
int
mockfs_fsnode_create(mount_t mp, uint8_t type, mockfs_fsnode_t * fsnpp)
{
	int rvalue;
	uint64_t new_size;

	rvalue = 0;
	new_size = 0;

	if (!fsnpp || !mp) {
		rvalue = EINVAL;
		goto done;
	}

	switch (type) {
	case MOCKFS_ROOT:
		break;
	case MOCKFS_DEV:
		break;
	case MOCKFS_FILE:
		  
                                                                               
                                
     
		new_size = mp->mnt_devvp->v_specinfo->si_devsize;
		break;
	default:
		rvalue = EINVAL;
		goto done;
	}

	MALLOC(*fsnpp, typeof(*fsnpp), sizeof(**fsnpp), M_TEMP, M_WAITOK | M_ZERO);

	if (!*fsnpp) {
		rvalue = ENOMEM;
		goto done;
	}

	(*fsnpp)->size = new_size;
	(*fsnpp)->type = type;
	(*fsnpp)->mnt = mp;

done:
	return rvalue;
}

  
                         
                                                                                                    
                                                                                                     
  
                                     
   
int
mockfs_fsnode_destroy(mockfs_fsnode_t fsnp)
{
	int rvalue;

	rvalue = 0;

	  
                                                                                           
                                                                                   
    
	if (!fsnp || (((mockfs_mount_t)fsnp->mnt->mnt_data)->mockfs_root == fsnp)) {
		rvalue = EINVAL;
		goto done;
	}

	  
                                                                                              
                                                                        
    
	if (fsnp->vp) {
		panic("mockfs_fsnode_destroy called on node with live vnode; fsnp = %p (in case gdb is screwing with you)", fsnp);
	}

	  
                                                       
   
                                                                                              
                                                                                             
                                                                                              
                        
    
	if (fsnp->child_a) {
		if ((rvalue = mockfs_fsnode_destroy(fsnp->child_a))) {
			panic("mockfs_fsnode_destroy failed on child_a; fsnp = %p (in case gdb is screwing with you), rvalue = %d", fsnp, rvalue);
		}
	}

	if (fsnp->child_b) {
		if ((rvalue = mockfs_fsnode_destroy(fsnp->child_b))) {
			panic("mockfs_fsnode_destroy failed on child_b; fsnp = %p (in case gdb is screwing with you), rvalue = %d", fsnp, rvalue);
		}
	}

	  
                                                     
    
	if (fsnp->parent) {
		if ((rvalue = mockfs_fsnode_orphan(fsnp))) {
			panic("mockfs_fsnode_orphan failed during destroy; fsnp = %p (in case gdb is screwing with you), rvalue = %d", fsnp, rvalue);
		}
	}

	FREE(fsnp, M_TEMP);
done:
	return rvalue;
}

  
                       
                                                                                 
  
                                     
   
int
mockfs_fsnode_adopt(mockfs_fsnode_t parent, mockfs_fsnode_t child)
{
	int rvalue;

	rvalue = 0;

	  
                                                                    
    
	if ((!parent || !child || child->parent) && (parent != child)) {
		rvalue = EINVAL;
		goto done;
	}

	  
                                                                                                    
    
	if (parent->mnt != child->mnt) {
		rvalue = EINVAL;
		goto done;
	}

	  
                                                                                   
                                                                                                    
                                                                                                
                      
   
                                                                          
    
	if (!parent->child_a) {
		parent->child_a = child;
		child->parent = parent;
	} else if (!parent->child_b) {
		parent->child_b = child;
		child->parent = parent;
	} else {
		rvalue = ENOMEM;
	}

done:
	return rvalue;
}

  
                        
  
                                     
   
int
mockfs_fsnode_orphan(mockfs_fsnode_t fsnp)
{
	int rvalue;
	mockfs_fsnode_t parent;

	rvalue = 0;

	if (!fsnp || !fsnp->parent) {
		rvalue = EINVAL;
		goto done;
	}

	  
                                                        
    
	if (fsnp->vp) {
		panic("mockfs_fsnode_orphan called on node with live vnode; fsnp = %p (in case gdb is screwing with you)", fsnp);
	}

	parent = fsnp->parent;

	if (parent->child_a == fsnp) {
		parent->child_a = NULL;
		fsnp->parent = NULL;
	} else if (parent->child_b == fsnp) {
		parent->child_b = NULL;
		fsnp->parent = NULL;
	} else {
		panic("mockfs_fsnode_orphan insanity, fsnp->parent != parent->child; fsnp = %p (in case gdb is screwing with you)", fsnp);
	}

done:
	return rvalue;
}

  
                               
                                                                                                         
                                                                                                         
                                                      
  
                                                                                                       
                
  
                                     
   
int
mockfs_fsnode_child_by_type(mockfs_fsnode_t parent, uint8_t type, mockfs_fsnode_t * child)
{
	int rvalue;

	rvalue = 0;

	if (!parent || !child) {
		rvalue = EINVAL;
		goto done;
	}

	if ((parent->child_a) && (parent->child_a->type == type)) {
		*child = parent->child_a;
	} else if ((parent->child_b) && (parent->child_b->type == type)) {
		*child = parent->child_b;
	} else {
		rvalue = ENOENT;
	}

done:
	return rvalue;
}

  
                       
                                                                                                        
                           
  
                                     
   
int
mockfs_fsnode_vnode(mockfs_fsnode_t fsnp, vnode_t * vpp)
{
	int rvalue;
	memory_object_control_t ubc_mem_object;
	mockfs_mount_t mockfs_mnt;
	struct vnode_fsparam vnfs_param;

	if ((!fsnp) || (!vpp)) {
		rvalue = EINVAL;
		goto done;
	}

	mockfs_mnt = ((mockfs_mount_t) fsnp->mnt->mnt_data);
	lck_mtx_lock(&mockfs_mnt->mockfs_mnt_mtx);

	if (fsnp->vp) {
		  
                                                   
     
		rvalue = vnode_get(fsnp->vp);
		if (!rvalue) {
			*vpp = fsnp->vp;
		}
	} else {
		  
                                                          
     
		vnfs_param.vnfs_mp = fsnp->mnt;
		vnfs_param.vnfs_vtype = (fsnp->type == MOCKFS_FILE) ? VREG : VDIR;
		vnfs_param.vnfs_str = "mockfs";
		vnfs_param.vnfs_dvp = (fsnp->type == MOCKFS_ROOT) ? NULL : fsnp->parent->vp;
		vnfs_param.vnfs_fsnode = fsnp;
		vnfs_param.vnfs_vops = mockfs_vnodeop_p;
		vnfs_param.vnfs_markroot = (fsnp->type == MOCKFS_ROOT);
		vnfs_param.vnfs_marksystem = 0;
		vnfs_param.vnfs_rdev = 0;
		vnfs_param.vnfs_filesize = fsnp->size;
		vnfs_param.vnfs_cnp = NULL;
		vnfs_param.vnfs_flags = VNFS_CANTCACHE | VNFS_NOCACHE;
		rvalue = vnode_create(VNCREATE_FLAVOR, VCREATESIZE, &vnfs_param, &fsnp->vp);

		if ((!rvalue) && (fsnp->type == MOCKFS_FILE) && (mockfs_mnt->mockfs_memory_backed)) {
			  
                                                                                   
      
			ubc_mem_object = ubc_getobject(fsnp->vp, 0);

			if (!ubc_mem_object) {
				panic("mockfs_fsvnode failed to get ubc_mem_object for a new vnode");
			}

			rvalue = pager_map_to_phys_contiguous(ubc_mem_object, 0, (mockfs_mnt->mockfs_memdev_base << PAGE_SHIFT), fsnp->size);

			if (rvalue) {
				panic("mockfs_fsnode_vnode failed to create fictitious pages for a memory-backed device; rvalue = %d", rvalue);
			}
		}

		if (!rvalue) {
			*vpp = fsnp->vp;
		}
	}

	lck_mtx_unlock(&mockfs_mnt->mockfs_mnt_mtx);

done:
	return rvalue;
}

  
                       
                                                                                              
                                                                                                      
                                         
  
                                     
   
int
mockfs_fsnode_drop_vnode(mockfs_fsnode_t fsnp)
{
	int rvalue;
	mockfs_mount_t mockfs_mnt;
	vnode_t vp;

	rvalue = 0;

	if (!fsnp) {
		rvalue = EINVAL;
		goto done;
	}

	mockfs_mnt = ((mockfs_mount_t) fsnp->mnt->mnt_data);
	lck_mtx_lock(&mockfs_mnt->mockfs_mnt_mtx);

	if (!(fsnp->vp)) {
		panic("mock_fsnode_drop_vnode: target fsnode does not have an associated vnode");
	}

	vp = fsnp->vp;
	fsnp->vp = NULL;
	vnode_clearfsnode(vp);

	lck_mtx_unlock(&mockfs_mnt->mockfs_mnt_mtx);
done:
	return rvalue;
}
