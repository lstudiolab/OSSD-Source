/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

#include <miscfs/mockfs/mockfs.h>
#include <miscfs/mockfs/mockfs_vnops.h>
#include <sys/ubc.h>
#include <sys/mount_internal.h>
#include <sys/vnode_internal.h>
#include <vfs/vfs_support.h>

#include <libkern/libkern.h>
#include <kern/debug.h>

  
                                                                      
   
#define VOPFUNC int (*)(void *)

  
                                                                                             
                                         
   
int mockfs_lookup(struct vnop_lookup_args * ap);
int mockfs_getattr(struct vnop_getattr_args * ap);
int mockfs_read(struct vnop_read_args * ap);
int mockfs_strategy(struct vnop_strategy_args * ap);
int mockfs_pagein(struct vnop_pagein_args * ap);
int mockfs_reclaim(__unused struct vnop_reclaim_args * ap);
int mockfs_blockmap(struct vnop_blockmap_args * ap);

  
                            
                                                                       
                                                                                            
                                                                                         
                                                                               
                                                                       
     
  
                 
                                                                                                    
                                                                                                     
                                                                                                    
                                                                                                        
                                                                                                      
                                                                                                   
  
                    
             
  
                     
            
  
                         
                
  
                                     
   
int
mockfs_lookup(struct vnop_lookup_args * ap)
{
	char held_char;
	int rvalue;
	int op;
	mockfs_fsnode_t fsnode;
	mockfs_fsnode_t target_fsnode;
	vnode_t dvp;
	vnode_t * vpp;
	vfs_context_t ctx;
	struct componentname * cnp;

	rvalue = 0;
	dvp = ap->a_dvp;
	vpp = ap->a_vpp;
	cnp = ap->a_cnp;
	ctx = ap->a_context;
	op = cnp->cn_nameiop;
	fsnode = (mockfs_fsnode_t) dvp->v_data;
	target_fsnode = NULL;

	if ((op == LOOKUP) && (fsnode->type == MOCKFS_ROOT)) {
		  
                                                                        
                                               
     

		held_char = cnp->cn_nameptr[cnp->cn_namelen];
		cnp->cn_nameptr[cnp->cn_namelen] = '\0';

		  
                                                                                           
                                                                                                
                                                                    
     
		if (!strncmp(cnp->cn_nameptr, "sbin", 5)) {
			target_fsnode = fsnode;
		} else if (!strncmp(cnp->cn_nameptr, "dev", 4)) {
			mockfs_fsnode_child_by_type(fsnode, MOCKFS_DEV, &target_fsnode);
		} else if (!strncmp(cnp->cn_nameptr, "launchd", 8)) {
			mockfs_fsnode_child_by_type(fsnode, MOCKFS_FILE, &target_fsnode);
		} else {
			rvalue = ENOENT;
		}

		cnp->cn_nameptr[cnp->cn_namelen] = held_char;

		if (target_fsnode) {
			rvalue = mockfs_fsnode_vnode(target_fsnode, vpp);
		}
	} else {
		  
                                                                                   
                           
     
		rvalue = ENOENT;
	}

	return rvalue;
}

  
                             
                                                                       
                                                                              
                                                                                                    
                                                                       
     
  
                  
                                                                                                               
                                                                                                                
                                                                                                                  
                                                                                                    
  
                                     
   
int
mockfs_getattr(struct vnop_getattr_args * ap)
{
	  
                                                                        
                                                                            
    
	vnode_t vp;
	mockfs_fsnode_t fsnode;
	struct vnode_attr * vap;

	vp = ap->a_vp;
	fsnode = (mockfs_fsnode_t)vp->v_data;
	vap = ap->a_vap;
	bzero(vap, sizeof(*vap));
	VATTR_RETURN(vap, va_nlink, 1);                                                             
	VATTR_RETURN(vap, va_mode, VREAD | VWRITE | VEXEC);
	VATTR_RETURN(vap, va_fileid, fsnode->type);
	VATTR_RETURN(vap, va_total_size, fsnode->size);
	VATTR_RETURN(vap, va_total_alloc, fsnode->size);
	VATTR_RETURN(vap, va_data_size, fsnode->size);
	VATTR_RETURN(vap, va_data_alloc, fsnode->size);

	return 0;
}

  
                          
                                                                       
                                                                              
                                                               
                                                                         
                                                                       
     
  
               
                                                                                                                    
                                                                                                                   
                                                                                                                    
                                                                                                                    
                                                                                                                  
                                                  
  
                                     
   
int
mockfs_read(struct vnop_read_args * ap)
{
	int rvalue;
	vnode_t vp;
	mockfs_fsnode_t fsnode;

	vp = ap->a_vp;
	fsnode = (mockfs_fsnode_t) vp->v_data;

	  
                                                                                           
                                    
    
	if (vp->v_type == VREG) {
		rvalue = cluster_read(vp, ap->a_uio, fsnode->size, ap->a_ioflag);
	} else {
		  
                                                             
     
		rvalue = ENOTSUP;
	}

	return rvalue;
}

  
                             
                                                                       
                                                                           
                                                                       
     
  
                  
                                                                                                                  
                                                                                                                 
                           
   
int
mockfs_reclaim(struct vnop_reclaim_args * ap)
{
	int rvalue;
	vnode_t vp;
	mockfs_fsnode_t fsnode;

	vp = ap->a_vp;
	fsnode = (mockfs_fsnode_t) vnode_fsnode(vp);
	rvalue = mockfs_fsnode_drop_vnode(fsnode);

	return rvalue;
}

  
                              
                                                                       
                                                                  
     
  
                   
                                                                                                                    
                                                                                                                   
                        
  
                                     
   
int
mockfs_strategy(struct vnop_strategy_args * ap)
{
	int rvalue;
	vnode_t dvp;

	  
                                                                                                        
                                    
    

	dvp = vfs_devvp(buf_vnode(ap->a_bp)->v_mount);

	if (dvp) {
		rvalue = buf_strategy(dvp, ap);
		vnode_put(dvp);
	} else {
		  
                                                                    
     
		rvalue = EIO;
	}

	return rvalue;
}

  
                            
                                                                       
                                                                              
                                                                                 
                                                                               
                                                                      
                                                             
                                                                          
                                                                       
     
  
                  
                                                                                                                  
                                                                                                                         
                                                                                      
  
                                     
   
int
mockfs_pagein(struct vnop_pagein_args * ap)
{
	mockfs_fsnode_t fsnode;
	mockfs_mount_t mockfs_mnt;

	  
                                                                                                   
    
	fsnode = (mockfs_fsnode_t) ap->a_vp->v_data;
	mockfs_mnt = ((mockfs_mount_t) fsnode->mnt->mnt_data);

	  
                                                                                                                
                                
    
	if (mockfs_mnt->mockfs_memory_backed) {
		panic("mockfs_pagein called for a memory-backed device");
	}

	return cluster_pagein(ap->a_vp, ap->a_pl, ap->a_pl_offset, ap->a_f_offset, ap->a_size, fsnode->size, ap->a_flags);
}

  
                              
                                                                       
                                                                              
                                                                     
                                                                            
                                                                                                                    
                                                                                         
                                                             
                                                                                        
                                                                       
     
  
                   
                                                                                                                  
                                                                                                               
                                                                                                                   
                                                                                                                    
                                                                            
  
                                     
   
int
mockfs_blockmap(struct vnop_blockmap_args * ap)
{
	int rvalue;
	off_t foffset;
	size_t * run;
	uint32_t blksize;
	daddr64_t * bpn;
	vnode_t vp;
	mockfs_fsnode_t fsnode;

	rvalue = 0;
	foffset = ap->a_foffset;
	run = ap->a_run;
	bpn = ap->a_bpn;
	vp = ap->a_vp;
	fsnode = (mockfs_fsnode_t) vp->v_data;
	blksize = vp->v_mount->mnt_devblocksize;

	  
                                                                                                              
                                                                                                              
                              
    
	if (((mockfs_mount_t) fsnode->mnt->mnt_data)->mockfs_memory_backed) {
		printf("mockfs_blockmap called for a memory-backed device\n");
	}

	  
                                                                                                  
                                                                                                     
    
	if (vp->v_type == VREG) {
		*bpn = foffset / blksize;
		*run = fsnode->size - foffset;

		if (ap->a_size > *run) {
			                                                                                     
			panic("mockfs_blockmap was asked for a region that extended past the end of the backing device");
		}
	} else {
		rvalue = ENOTSUP;
	}

	return rvalue;
}

int(**mockfs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc mockfs_vnodeop_entries[] = {
	{ .opve_op = &vnop_default_desc, .opve_impl = (VOPFUNC) vn_default_error },              
	{ .opve_op = &vnop_lookup_desc, .opve_impl = (VOPFUNC) mockfs_lookup },             
	{ .opve_op = &vnop_create_desc, .opve_impl = (VOPFUNC) err_create },            
	{ .opve_op = &vnop_open_desc, .opve_impl = (VOPFUNC) err_open },           
	{ .opve_op = &vnop_mknod_desc, .opve_impl = (VOPFUNC) err_mknod },            
	{ .opve_op = &vnop_close_desc, .opve_impl = (VOPFUNC) err_close },            
	{ .opve_op = &vnop_access_desc, .opve_impl = (VOPFUNC) err_access },             
	{ .opve_op = &vnop_getattr_desc, .opve_impl = (VOPFUNC) mockfs_getattr },              
	{ .opve_op = &vnop_setattr_desc, .opve_impl = (VOPFUNC) err_setattr },              
	{ .opve_op = &vnop_read_desc, .opve_impl = (VOPFUNC) mockfs_read },           
	{ .opve_op = &vnop_write_desc, .opve_impl = (VOPFUNC) err_write },            
	{ .opve_op = &vnop_ioctl_desc, .opve_impl = (VOPFUNC) err_ioctl },            
	{ .opve_op = &vnop_select_desc, .opve_impl = (VOPFUNC) err_select },             
	{ .opve_op = &vnop_mmap_desc, .opve_impl = (VOPFUNC) err_mmap },           
	{ .opve_op = &vnop_fsync_desc, .opve_impl = (VOPFUNC) nop_fsync },            
	{ .opve_op = &vnop_remove_desc, .opve_impl = (VOPFUNC) err_remove },             
	{ .opve_op = &vnop_link_desc, .opve_impl = (VOPFUNC) err_link },           
	{ .opve_op = &vnop_rename_desc, .opve_impl = (VOPFUNC) err_rename },             
	{ .opve_op = &vnop_mkdir_desc, .opve_impl = (VOPFUNC) err_mkdir },            
	{ .opve_op = &vnop_rmdir_desc, .opve_impl = (VOPFUNC) err_rmdir },            
	{ .opve_op = &vnop_symlink_desc, .opve_impl = (VOPFUNC) err_symlink },              
	{ .opve_op = &vnop_readdir_desc, .opve_impl = (VOPFUNC) err_readdir },              
	{ .opve_op = &vnop_readlink_desc, .opve_impl = (VOPFUNC) err_readlink },               
	{ .opve_op = &vnop_inactive_desc, .opve_impl = (VOPFUNC) err_inactive },               
	{ .opve_op = &vnop_reclaim_desc, .opve_impl = (VOPFUNC) mockfs_reclaim },              
	{ .opve_op = &vnop_strategy_desc, .opve_impl = (VOPFUNC) mockfs_strategy },               
	{ .opve_op = &vnop_pathconf_desc, .opve_impl = (VOPFUNC) err_pathconf },               
	{ .opve_op = &vnop_advlock_desc, .opve_impl = (VOPFUNC) err_advlock },              
	{ .opve_op = &vnop_bwrite_desc, .opve_impl = (VOPFUNC) err_bwrite },             
	{ .opve_op = &vnop_pagein_desc, .opve_impl = (VOPFUNC) mockfs_pagein },             
	{ .opve_op = &vnop_pageout_desc, .opve_impl = (VOPFUNC) err_pageout },              
	{ .opve_op = &vnop_copyfile_desc, .opve_impl = (VOPFUNC) err_copyfile },               
	{ .opve_op = &vnop_blktooff_desc, .opve_impl = (VOPFUNC) err_blktooff },               
	{ .opve_op = &vnop_offtoblk_desc, .opve_impl = (VOPFUNC) err_offtoblk },               
	{ .opve_op = &vnop_blockmap_desc, .opve_impl = (VOPFUNC) mockfs_blockmap },               
	{ .opve_op = (struct vnodeop_desc *) NULL, .opve_impl = (VOPFUNC) NULL }
};

const struct vnodeopv_desc mockfs_vnodeop_opv_desc = {
	.opv_desc_vector_p = &mockfs_vnodeop_p,
	.opv_desc_ops = mockfs_vnodeop_entries
};
