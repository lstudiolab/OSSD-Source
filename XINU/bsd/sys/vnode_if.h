/*
 * Copyright (c) 2000-2016 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved
 * Copyright (c) 1992, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS AND
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
 */
  
                                                                      
                                                                          
                                                                        
               
   

  
                                                 
                                                
  
                         
                                         
   


#ifndef _SYS_VNODE_IF_H_
#define _SYS_VNODE_IF_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>
#include <sys/kernel_types.h>
#include <sys/buf.h>
#ifdef BSD_KERNEL_PRIVATE
#include <sys/vm.h>
#endif
#include <mach/memory_object_types.h>


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

#ifdef KERNEL

extern struct vnodeop_desc vnop_default_desc;
extern struct vnodeop_desc vnop_lookup_desc;
#ifdef KERNEL_PRIVATE
extern struct vnodeop_desc vnop_compound_open_desc;
extern struct vnodeop_desc vnop_compound_remove_desc;
extern struct vnodeop_desc vnop_compound_rename_desc;
extern struct vnodeop_desc vnop_compound_mkdir_desc;
extern struct vnodeop_desc vnop_compound_rmdir_desc;
#endif                     
extern struct vnodeop_desc vnop_create_desc;
extern struct vnodeop_desc vnop_whiteout_desc;            
extern struct vnodeop_desc vnop_mknod_desc;
extern struct vnodeop_desc vnop_open_desc;
extern struct vnodeop_desc vnop_close_desc;
extern struct vnodeop_desc vnop_access_desc;
extern struct vnodeop_desc vnop_getattr_desc;
extern struct vnodeop_desc vnop_setattr_desc;
extern struct vnodeop_desc vnop_read_desc;
extern struct vnodeop_desc vnop_write_desc;
extern struct vnodeop_desc vnop_ioctl_desc;
extern struct vnodeop_desc vnop_select_desc;
extern struct vnodeop_desc vnop_exchange_desc;
extern struct vnodeop_desc vnop_revoke_desc;
extern struct vnodeop_desc vnop_mmap_check_desc;
extern struct vnodeop_desc vnop_mmap_desc;
extern struct vnodeop_desc vnop_mnomap_desc;
extern struct vnodeop_desc vnop_fsync_desc;
extern struct vnodeop_desc vnop_remove_desc;
extern struct vnodeop_desc vnop_link_desc;
extern struct vnodeop_desc vnop_rename_desc;
extern struct vnodeop_desc vnop_renamex_desc;
extern struct vnodeop_desc vnop_mkdir_desc;
extern struct vnodeop_desc vnop_rmdir_desc;
extern struct vnodeop_desc vnop_symlink_desc;
extern struct vnodeop_desc vnop_readdir_desc;
extern struct vnodeop_desc vnop_readdirattr_desc;
extern struct vnodeop_desc vnop_getattrlistbulk_desc;
extern struct vnodeop_desc vnop_readlink_desc;
extern struct vnodeop_desc vnop_inactive_desc;
extern struct vnodeop_desc vnop_reclaim_desc;
extern struct vnodeop_desc vnop_print_desc;
extern struct vnodeop_desc vnop_pathconf_desc;
extern struct vnodeop_desc vnop_advlock_desc;
extern struct vnodeop_desc vnop_truncate_desc;
extern struct vnodeop_desc vnop_allocate_desc;
extern struct vnodeop_desc vnop_pagein_desc;
extern struct vnodeop_desc vnop_pageout_desc;
extern struct vnodeop_desc vnop_searchfs_desc;
extern struct vnodeop_desc vnop_copyfile_desc;
extern struct vnodeop_desc vnop_clonefile_desc;
extern struct vnodeop_desc vnop_blktooff_desc;
extern struct vnodeop_desc vnop_offtoblk_desc;
extern struct vnodeop_desc vnop_blockmap_desc;
extern struct vnodeop_desc vnop_strategy_desc;
extern struct vnodeop_desc vnop_bwrite_desc;
#ifdef KERNEL_PRIVATE
extern struct vnodeop_desc vnop_verify_desc;
#endif

#ifdef __APPLE_API_UNSTABLE

#if NAMEDSTREAMS
extern struct vnodeop_desc vnop_getnamedstream_desc;
extern struct vnodeop_desc vnop_makenamedstream_desc;
extern struct vnodeop_desc vnop_removenamedstream_desc;
#endif

#endif

#ifdef KERNEL_PRIVATE
  
                                                              
                                                               
                                                                
                                                                  
                                                              
                                                            
                                                                       
                                                           
                                                                   
                                                             
                                                                
                   
                                                                
                                                                  
                                                                   
                                                                    
        
   


extern int fs_buffer_cache_gc_register(void (* callout)(int, void *), void *);
extern int fs_buffer_cache_gc_unregister(void (* callout)(int, void *), void *);
#endif

__BEGIN_DECLS

struct vnop_lookup_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	vfs_context_t a_context;
};

   
                         
                                                                              
                                                                                                             
                                                                                                              
                            
                                                  
                                           
                                                                                                
                                                                    
                                                         
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_LOOKUP(vnode_t, vnode_t *, struct componentname *, vfs_context_t);
#endif                         

struct vnop_create_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
};

   
                         
                                                                        
                                                                                                                
                                                                      
                                                 
                                                            
                                                 
                                                                                                                    
                                      
                                                                   
                                                         
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_CREATE(vnode_t, vnode_t *, struct componentname *, struct vnode_attr *, vfs_context_t);
#endif                         

struct vnop_whiteout_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	struct componentname *a_cnp;
	int a_flags;
	vfs_context_t a_context;
};

   
                           
                                             
                                                                                                                     
                                                                                                                    
                                                                                                                      
                                                                                                                     
                                   
                                                                                               
                                            
                                             
                                                                                                                            
                                                                       
                                                                                                                                    
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_WHITEOUT(vnode_t, struct componentname *, int, vfs_context_t);
#endif                         

struct vnop_mknod_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
};

   
                        
                                                                 
                                                                                                                              
                                                                                                                       
                                                  
                                                             
                                                   
                                             
                                                       
                                                                   
                                                         
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_MKNOD(vnode_t, vnode_t *, struct componentname *, struct vnode_attr *, vfs_context_t);
#endif                         

struct vnop_open_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_mode;
	vfs_context_t a_context;
};

#ifdef KERNEL_PRIVATE
struct vnop_compound_open_args {
	struct vnodeop_desc *a_desc;

	vnode_t a_dvp;                                                                 
	vnode_t *a_vpp;                                              
	int a_fmode;                                           
	struct componentname *a_cnp;                                 
	struct vnode_attr *a_vap;                                                                    
	uint32_t a_flags;                                               
	uint32_t *a_status;                                                    

	vfs_context_t a_context;                                                   

	int (*a_open_create_authorizer)(                                        
		vnode_t dvp,                                                              
		struct componentname *cnp,                                     
		struct vnode_attr *vap,                                        
		vfs_context_t ctx,                                   
		void *reserved);                                       

	int (*a_open_existing_authorizer)(                                           
		vnode_t vp,                                             
		struct componentname *cnp,                                
		int fmode,                                                     
		vfs_context_t ctx,                                   
		void *reserved);                                       

	void *a_reserved;
};

             
#define COMPOUND_OPEN_STATUS_DID_CREATE 0x00000001
#endif                     

   
                       
                                                       
                                                                                  
                                                                                                     
                         
                           
                                    
                                                          
                                                         
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_OPEN(vnode_t, int, vfs_context_t);
#endif                         

#ifdef BSD_KERNEL_PRIVATE
struct nameidata;
extern int VNOP_COMPOUND_OPEN(vnode_t dvp, vnode_t *vpp, struct nameidata *ndp, int32_t flags, int32_t fmode, uint32_t *status, struct vnode_attr *vap, vfs_context_t ctx);
#endif

struct vnop_close_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_fflag;
	vfs_context_t a_context;
};

   
                        
                                                        
                                                                                  
                                                                                              
                            
                                                                                                  
                               
                                                           
                                                         
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_CLOSE(vnode_t, int, vfs_context_t);
#endif                         

struct vnop_access_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_action;
	vfs_context_t a_context;
};

   
                         
                                                                                       
                                                                                         
                                                                                                    
                                                        
                                           
                                                                                            
                                                            
                                                         
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_ACCESS(vnode_t, int, vfs_context_t);
#endif                         

struct vnop_getattr_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
};

   
                          
                                                                
                                                                                                               
                                                                                     
                                                                                                                
                                                                         
                                                
                                                                                                                                    
                                                                            
                                                                                                       
                                                                                                                                
                                    
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_GETATTR(vnode_t, struct vnode_attr *, vfs_context_t);
#endif                         

struct vnop_setattr_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
};

   
                          
                                                                
                                                                                                            
                                                                                                          
                                                                                                                
                                                                                                            
                                                
                                                                                                                      
                                                                    
                                                                                  
                                                                                                        
                                                                                                                           
                                    
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_SETATTR(vnode_t, struct vnode_attr *, vfs_context_t);
#endif                         

struct vnop_read_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct uio *a_uio;
	int a_ioflag;
	vfs_context_t a_context;
};

   
                       
                                                          
                                                                                                                
                                                                                                                             
                                                                                                          
                                             
                                     
                                                                                                                     
                                                            
                                                                             
                                                                  
                                                                                                               
                                                                                                                
   
extern errno_t VNOP_READ(vnode_t vp, struct uio *uio, int ioflag, vfs_context_t ctx);

struct vnop_write_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct uio *a_uio;
	int a_ioflag;
	vfs_context_t a_context;
};

   
                        
                                                             
                                                                                               
                                                                                                                               
                                                                                                          
                                             
                                    
                                                                                                               
                                                            
                                                                             
                                                                   
                                                                                                                
                                                                                                                   
   
extern errno_t VNOP_WRITE(vnode_t vp, struct uio *uio, int ioflag, vfs_context_t ctx);

struct vnop_ioctl_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	u_long a_command;
	caddr_t a_data;
	int a_fflag;
	vfs_context_t a_context;
};

   
                        
                                                                                                                               
                                                                                                    
                                                                                                           
                                                                                                                    
                                                                                                         
                                                                                                    
                                                              
                                                  
                                                 
                                                                                                                               
                                                                                                                    
                                                 
                                                                   
                                                         
   
extern errno_t VNOP_IOCTL(vnode_t vp, u_long command, caddr_t data, int fflag, vfs_context_t ctx);

struct vnop_select_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_which;
	int a_fflags;
	void *a_wql;
	vfs_context_t a_context;
};

   
                         
                                                                                                                                                  
                                                                                                        
                                                                                                               
                                                                                                                    
                                                                                                                                  
                                                   
                                                            
                                                                                    
                                                    
                                                          
                                                                                                            
                                                                                                          
                                                                                                
                                                     
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_SELECT(vnode_t, int, int, void *, vfs_context_t);
#endif                         

struct vnop_exchange_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_fvp;
	vnode_t a_tvp;
	int a_options;
	vfs_context_t a_context;
};

   
                           
                                                                                     
                                                                                                         
                                           
                           
                            
                          
                                                                
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_EXCHANGE(vnode_t, vnode_t, int, vfs_context_t);
#endif                         

struct vnop_revoke_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_flags;
	vfs_context_t a_context;
};

   
                         
                                                                                            
                                                                                          
                                                                                              
                                        
                                  
                        
                                                          
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_REVOKE(vnode_t, int, vfs_context_t);
#endif                         

struct vnop_mmap_check_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_flags;
	vfs_context_t a_context;
};

   
                             
                                                               
                                                                          
                                                                          
                                      
                                                                     
                                                        
                                                                         
                                                                         
                                                                 
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_MMAP_CHECK(vnode_t, int, vfs_context_t);
#endif                         


struct vnop_mmap_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_fflags;
	vfs_context_t a_context;
};

   
                       
                                                               
                                                                                            
                                      
                                                                     
                                                        
                                                               
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_MMAP(vnode_t, int, vfs_context_t);
#endif                         

struct vnop_mnomap_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	vfs_context_t a_context;
};

   
                         
                                                                  
                                                                                  
                                                  
                                                          
                                    
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_MNOMAP(vnode_t, vfs_context_t);
#endif                         

struct vnop_fsync_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_waitfor;
	vfs_context_t a_context;
};

   
                        
                                                                                 
                                                                                              
                                                                                                         
                                                             
                                                         
                                              
   
extern errno_t VNOP_FSYNC(vnode_t vp, int waitfor, vfs_context_t ctx);

struct vnop_remove_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t a_vp;
	struct componentname *a_cnp;
	int a_flags;
	vfs_context_t a_context;
};

   
                         
                                                         
                                                                                                              
                                                                                                  
                                                   
                                 
                                    
                                                         
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_REMOVE(vnode_t, vnode_t, struct componentname *, int, vfs_context_t);
#endif                         

#ifdef KERNEL_PRIVATE
struct vnop_compound_remove_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;                                                                       
	vnode_t *a_vpp;                                                                                   
	struct componentname *a_cnp;                                        
	struct vnode_attr *a_vap;                                                                         
	uint32_t a_flags;                                                   
	vfs_context_t a_context;                                           
	int (*a_remove_authorizer)(                                      
		vnode_t dvp,                                                              
		vnode_t vp,                                                 
		struct componentname *cnp,                                     
		vfs_context_t ctx,                                             
		void *reserved);                                         
	void *a_reserved;                                   
};
#endif                     

#ifdef BSD_KERNEL_PRIVATE
extern errno_t VNOP_COMPOUND_REMOVE(vnode_t, vnode_t*, struct nameidata *, int32_t flags, struct vnode_attr *vap, vfs_context_t);
#endif
struct vnop_link_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	vnode_t a_tdvp;
	struct componentname *a_cnp;
	vfs_context_t a_context;
};

   
                       
                                                                       
                                 
                              
                                                     
                                                 
                                                        
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_LINK(vnode_t, vnode_t, struct componentname *, vfs_context_t);
#endif                         

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

   
                         
                                                         
                                                                                              
                                                       
                                  
                                                 
                                                 
                                                                        
                                                 
                                                          
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_RENAME(vnode_t, vnode_t, struct componentname *, vnode_t, vnode_t, struct componentname *, vfs_context_t);
#endif                         

typedef unsigned int vfs_rename_flags_t;

                         
enum {
	VFS_RENAME_SECLUDE              = 0x00000001,
	VFS_RENAME_SWAP                 = 0x00000002,
	VFS_RENAME_EXCL                 = 0x00000004,

	  
                                                           
                                          
    
	VFS_RENAME_DATALESS             = 0x00000008,

	VFS_RENAME_FLAGS_MASK   = (VFS_RENAME_SECLUDE | VFS_RENAME_SWAP
	    | VFS_RENAME_EXCL),
};

struct vnop_renamex_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_fdvp;
	vnode_t a_fvp;
	struct componentname *a_fcnp;
	vnode_t a_tdvp;
	vnode_t a_tvp;
	struct componentname *a_tcnp;
	struct vnode_attr *a_vap;                                         
	vfs_rename_flags_t a_flags;
	vfs_context_t a_context;
};

   
                          
                                                         
                                                                                               
                                                       
                                  
                                                 
                                                 
                                                                        
                                                 
                                                  
                                                          
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_RENAMEX(vnode_t, vnode_t, struct componentname *, vnode_t, vnode_t, struct componentname *, vfs_rename_flags_t, vfs_context_t);
#endif                         

#ifdef KERNEL_PRIVATE
struct vnop_compound_rename_args {
	struct vnodeop_desc *a_desc;

	vnode_t a_fdvp;                                                     
	vnode_t *a_fvpp;                                                                   
	struct componentname *a_fcnp;                    
	struct vnode_attr *a_fvap;

	vnode_t a_tdvp;                                                   
	vnode_t *a_tvpp;                                                                        
	struct componentname *a_tcnp;                         
	struct vnode_attr *a_tvap;

	uint32_t a_flags;                                                    
	vfs_context_t a_context;                                   
	int (*a_rename_authorizer)(                                                 
		vnode_t fdvp,                                                  
		vnode_t fvp,                                                 
		struct componentname *fcnp,                                    
		vnode_t tdvp,                                                  
		vnode_t tvp,                                                                    
		struct componentname *tcnp,                                    
		vfs_context_t ctx,                                             
		void *reserved);                                         
	void *a_reserved;                                     
};
#endif                     

#ifdef XNU_KERNEL_PRIVATE
errno_t
VNOP_COMPOUND_RENAME(
	struct vnode *fdvp, struct vnode **fvpp, struct componentname *fcnp, struct vnode_attr *fvap,
	struct vnode *tdvp, struct vnode **tvpp, struct componentname *tcnp, struct vnode_attr *tvap,
	uint32_t flags, vfs_context_t ctx);
#endif                         

struct vnop_mkdir_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	struct vnode_attr *a_vap;
	vfs_context_t a_context;
};

   
                        
                                                              
                                                                                                                   
                                                          
                                                                
                                                  
                                            
                                                         
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_MKDIR(vnode_t, vnode_t *, struct componentname *, struct vnode_attr *, vfs_context_t);
#endif                         


#ifdef KERNEL_PRIVATE
struct vnop_compound_mkdir_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;                                                    
	vnode_t *a_vpp;                                                             
	struct componentname *a_cnp;                                     
	struct vnode_attr *a_vap;                                
	uint32_t a_flags;                                           
	vfs_context_t a_context;                                   
#if 0
	int (*a_mkdir_authorizer)(vnode_t dvp, struct componentname *cnp, struct vnode_attr *vap, vfs_context_t ctx, void *reserved);
#endif        
	void *a_reserved;                           
};
#endif                     

#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_COMPOUND_MKDIR(vnode_t, vnode_t *, struct nameidata *, struct vnode_attr *, vfs_context_t);
#endif                         

struct vnop_rmdir_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t a_vp;
	struct componentname *a_cnp;
	vfs_context_t a_context;
};

   
                        
                                                              
                                                 
                                  
                                                            
                                                         
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_RMDIR(vnode_t, vnode_t, struct componentname *, vfs_context_t);
#endif                         

#ifdef KERNEL_PRIVATE
struct vnop_compound_rmdir_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;                                                                        
	vnode_t *a_vpp;                                                          
	struct componentname *a_cnp;                                
	struct vnode_attr *a_vap;                                                                                           
	uint32_t a_flags;                                                             
	vfs_context_t a_context;                                               
	int (*a_rmdir_authorizer)(                                          
		vnode_t dvp,                                                   
		vnode_t vp,                                                      
		struct componentname *cnp,                                     
		vfs_context_t ctx,                                             
		void *reserved);                                         
	void *a_reserved;                                   
};
#endif                     

#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_COMPOUND_RMDIR(vnode_t, vnode_t*, struct nameidata *, struct vnode_attr *vap, vfs_context_t);
#endif                         


struct vnop_symlink_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_dvp;
	vnode_t *a_vpp;
	struct componentname *a_cnp;
	struct vnode_attr *a_vap;
	char *a_target;
	vfs_context_t a_context;
};

   
                          
                                                                  
                                                                                                      
                                                                                                  
                                                   
                                                     
              
                                                
                                      
                                                                                                                  
                                                           
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_SYMLINK(vnode_t, vnode_t *, struct componentname *, struct vnode_attr *, char *, vfs_context_t);
#endif                         

  
  
                                                                 
                         
  
                                                                
                                            
  
                                                                          
                                                                  
                                                                  
                                                                    
                                                                   
                                                                    
                                                                 
                                                  
  
   

struct vnop_readdir_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct uio *a_uio;
	int a_flags;
	int *a_eofflag;
	int *a_numdirent;
	vfs_context_t a_context;
};

   
                          
                                                                       
                                                                                                               
                                       
                                     
                                                                
                                                                                                                 
                                                                                   
                                                                            
                                                           
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_READDIR(vnode_t, struct uio *, int, int *, int *, vfs_context_t);
#endif                         

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

   
                              
                                                                                     
                                                                                                                       
                                                                  
                                                                      
                                                                
                                                                 
                                                                                                                     
                                                  
                                                                                                               
                                                                 
                                                                                   
                                                                                                   
                                                               
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_READDIRATTR(vnode_t, struct attrlist *, struct uio *, uint32_t, uint32_t, uint32_t *, int *, uint32_t *, vfs_context_t);
#endif                         

struct vnop_getattrlistbulk_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct attrlist *a_alist;
	struct vnode_attr *a_vap;
	struct uio *a_uio;
	void *a_private;
	uint64_t a_options;
	int32_t *a_eofflag;
	int32_t *a_actualcount;
	vfs_context_t a_context;
};

   
                                  
                                                                                     
                                                                                                                           
                                                                  
                                                                      
                                                                
                                                                                                                                                                              
                                           
                  
                                                                                   
                                                                                                   
                                                                   
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_GETATTRLISTBULK(vnode_t, struct attrlist *, struct vnode_attr *, uio_t, void *, uint64_t, int32_t *, int32_t *, vfs_context_t);
#endif                         

struct vnop_readlink_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	struct uio *a_uio;
	vfs_context_t a_context;
};

   
                           
                                                                                           
                                                                                                                                
                                         
                                                     
                                                            
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_READLINK(vnode_t, struct uio *, vfs_context_t);
#endif                         

struct vnop_inactive_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	vfs_context_t a_context;
};

   
                           
                                                                                                            
                                                                                                                            
                                                                                                                   
                                                  
                                                                                             
                                                                                                  
                                              
                                                            
                                                                                     
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_INACTIVE(vnode_t, vfs_context_t);
#endif                         

struct vnop_reclaim_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	vfs_context_t a_context;
};

   
                          
                                                                
                                                                                             
                                                                                                    
                                                                                                  
                                                                                                       
                                                                                                   
                                                                                                  
                
                                   
                                                   
                                                                                        
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_RECLAIM(vnode_t, vfs_context_t);
#endif                         

struct vnop_pathconf_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	int a_name;
	int32_t *a_retval;
	vfs_context_t a_context;
};

   
                           
                                                     
                                                  
                                                                                             
                                                                                 
                                                    
                                                            
                                            
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_PATHCONF(vnode_t, int, int32_t *, vfs_context_t);
#endif                         

struct vnop_advlock_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	caddr_t a_id;
	int a_op;
	struct flock *a_fl;
	int a_flags;
	vfs_context_t a_context;
	struct timespec *a_timeout;
};

   
                          
                                                             
                                                                                         
                                                                                                         
                                                                                                             
                                                             
                                          
                                                                      
                                                                                       
                                                                                           
                                                                              
                                                                                            
                                                                                                           
                                                           
                                                                    
                                                                   
                                            
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_ADVLOCK(vnode_t, caddr_t, int, struct flock *, int, vfs_context_t, struct timespec *);
#endif                         

struct vnop_allocate_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	off_t a_length;
	u_int32_t a_flags;
	off_t *a_bytesallocated;
	off_t a_offset;
	vfs_context_t a_context;
};

   
                           
                                            
                                                                                
                                                                                  
                                                                                               
                                                                                                  
                                              
                                                       
                                                   
                
                                                   
                                               
                                                                     
                                                              
                                                     
                                                      
                                                                                              
                                 
                                                     
                                                              
                                            
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_ALLOCATE(vnode_t, off_t, u_int32_t, off_t *, off_t, vfs_context_t);
#endif                         

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

   
                         
                                         
                                                                                              
                                                                                                          
                                                                                                          
                                                  
                                                          
                                                                  
                                                                  
                                                     
                                                                                                      
                                                                                             
                                                                       
                                                          
                                            
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_PAGEIN(vnode_t, upl_t, upl_offset_t, off_t, size_t, int, vfs_context_t);
#endif                         

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

   
                          
                                                         
                                                                                                         
                                                                                                        
                                                                                                 
                                                         
                                                   
                                                                                                        
                                                                                                               
                                                                                                             
                                                                          
                                                                                                            
                                                                                                                
                                                                                                          
                                                                                                    
                                                                                                     
                                                                                                          
                                                                                    
                                                                                                      
                                                                                             
                                                                       
                                                           
                                            
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_PAGEOUT(vnode_t, upl_t, upl_offset_t, off_t, size_t, int, vfs_context_t);
#endif                         

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

  
                           
                                                                                                            
                                                                                                            
                                                                                                                 
                                                                                                             
                                                                                                              
                                                                                                                
                                                                                                                  
                                                                                    
                                                                                     
                                                                                                                    
                                                                                                                      
                                                                       
                                                                                                      
                                                                                                                 
                                                                                        
                                                                                                                   
                                                                                                                    
                                                                                                                     
                   
                                          
                                                                  
                                                                                                                  
                                                                                                                            
                                                                   
                                                                           
                                                                                                                 
   

#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_SEARCHFS(vnode_t, void *, void *, struct attrlist *, uint32_t, struct timeval *, struct attrlist *, uint32_t *, uint32_t, uint32_t, struct uio *, struct searchstate *, vfs_context_t);
#endif                         

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

#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_COPYFILE(vnode_t, vnode_t, vnode_t, struct componentname *, int, int, vfs_context_t);
#endif                         

typedef enum dir_clone_authorizer_op {
	OP_AUTHORIZE = 0,                                                
	OP_VATTR_SETUP = 1,                                                                      
	OP_VATTR_CLEANUP = 2                                                                                       
} dir_clone_authorizer_op_t;

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
		uint32_t flags,                                                        
		vfs_context_t ctx,                                             
		void *reserved);                                         
	void *a_reserved;                                     
};

   
                            
                                                                                                                
                                                                                                                
                                                  
                                                             
                                                      
                                                                                                                    
                                                                                          
                          
                                                                       
                                                                   
                                                         
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_CLONEFILE(vnode_t, vnode_t, vnode_t *, struct componentname *, struct vnode_attr *, uint32_t, vfs_context_t);
#endif                         

struct vnop_getxattr_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	const char * a_name;
	uio_t a_uio;
	size_t *a_size;
	int a_options;
	vfs_context_t a_context;
};
extern struct vnodeop_desc vnop_getxattr_desc;

   
                           
                                           
                                                       
                                          
                                                           
                                                            
                                                              
                                                            
                                            
   
extern errno_t VNOP_GETXATTR(vnode_t vp, const char *name, uio_t uio, size_t *size, int options, vfs_context_t ctx);

struct vnop_setxattr_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	const char * a_name;
	uio_t a_uio;
	int a_options;
	vfs_context_t a_context;
};
extern struct vnodeop_desc vnop_setxattr_desc;

   
                           
                                           
                                                       
                                          
                                                      
                                                                                                       
                                                     
                                                            
                                            
   
extern errno_t VNOP_SETXATTR(vnode_t vp, const char *name, uio_t uio, int options, vfs_context_t ctx);

struct vnop_removexattr_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	const char * a_name;
	int a_options;
	vfs_context_t a_context;
};
extern struct vnodeop_desc vnop_removexattr_desc;

   
                              
                                              
                                                                 
                                          
                                                              
                                                                    
                                            
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_REMOVEXATTR(vnode_t, const char *, int, vfs_context_t);
#endif                         

struct vnop_listxattr_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	uio_t a_uio;
	size_t *a_size;
	int a_options;
	vfs_context_t a_context;
};
extern struct vnodeop_desc vnop_listxattr_desc;

   
                            
                                           
                                                                                          
                                                                                         
                                     
                                                                 
                                                               
                                                                  
                                                              
                                                                  
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_LISTXATTR(vnode_t, uio_t, size_t *, int, vfs_context_t);
#endif                         

struct vnop_blktooff_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	daddr64_t a_lblkno;
	off_t *a_offset;
};

   
                           
                                                                                           
                                                                                                
                                                                                                     
                                                                          
                                                           
                                              
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_BLKTOOFF(vnode_t, daddr64_t, off_t *);
#endif                         

struct vnop_offtoblk_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	off_t a_offset;
	daddr64_t *a_lblkno;
};

   
                           
                                                                                           
                                                                                 
                                         
                                                                     
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_OFFTOBLK(vnode_t, off_t, daddr64_t *);
#endif                         

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

   
                           
                                                                                                     
                                                                                                          
                                        
                                                             
                                                            
                               
                                                                                    
                                                                                             
                        
                                 
                                                                                        
                                                                                         
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_BLOCKMAP(vnode_t, off_t, size_t, daddr64_t *, size_t *, void *,
    int, vfs_context_t);
#endif                         

struct vnop_strategy_args {
	struct vnodeop_desc *a_desc;
	struct buf *a_bp;
};

   
                           
                                                           
                                                                                                                       
                                                                                                                           
                                                       
                                                                                                                                 
                                              
   
extern errno_t VNOP_STRATEGY(struct buf *bp);

struct vnop_bwrite_args {
	struct vnodeop_desc *a_desc;
	buf_t a_bp;
};

   
                         
                                              
                                                                                                                              
                                                                                                                            
                                                                                                                           
                                  
                                              
   
extern errno_t VNOP_BWRITE(buf_t bp);

struct vnop_kqfilt_add_args {
	struct vnodeop_desc *a_desc;
	struct vnode *a_vp;
	struct knote *a_kn;
	vfs_context_t a_context;
};
extern struct vnodeop_desc vnop_kqfilt_add_desc;

#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_KQFILT_ADD(vnode_t, struct knote *, vfs_context_t);
#endif                         

struct vnop_kqfilt_remove_args {
	struct vnodeop_desc *a_desc;
	struct vnode *a_vp;
	uintptr_t a_ident;
	vfs_context_t a_context;
};
extern struct vnodeop_desc vnop_kqfilt_remove_desc;

#ifdef XNU_KERNEL_PRIVATE
errno_t VNOP_KQFILT_REMOVE(vnode_t, uintptr_t, vfs_context_t);
#endif                         


#ifdef KERNEL_PRIVATE
#define VNODE_MONITOR_BEGIN     0x01
#define VNODE_MONITOR_END       0x02
#define VNODE_MONITOR_UPDATE    0x04
struct vnop_monitor_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	uint32_t a_events;
	uint32_t a_flags;
	void *a_handle;
	vfs_context_t a_context;
};
extern struct vnodeop_desc vnop_monitor_desc;
#endif                     

#ifdef XNU_KERNEL_PRIVATE
   
                          
                                                                                         
                                                      
                                                                 
                                                                                                  
                                                                                                               
                                                                                                       
                                                                           
                                                                                                                        
                                                                                               
                                                                                                  
            
                                                                                                       
                                                                                     
                                                                                                    
                                                                                                  
                                                                           
                                                                                               
                                                                                                   
                                                              
   
errno_t VNOP_MONITOR(vnode_t vp, uint32_t events, uint32_t flags, void *handle, vfs_context_t ctx);
#endif                         

struct label;
struct vnop_setlabel_args {
	struct vnodeop_desc *a_desc;
	struct vnode *a_vp;
	struct label *a_vl;
	vfs_context_t a_context;
};
extern struct vnodeop_desc vnop_setlabel_desc;

   
                           
                                                 
                                 
                                   
                                                        
                                              
   
#ifdef XNU_KERNEL_PRIVATE
errno_t VNOP_SETLABEL(vnode_t, struct label *, vfs_context_t);
#endif                         

#ifdef __APPLE_API_UNSTABLE

#if NAMEDSTREAMS

enum nsoperation        { NS_OPEN, NS_CREATE, NS_DELETE };

                                           
#define NS_GETRAWENCRYPTED 0x00000001

struct vnop_getnamedstream_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	vnode_t *a_svpp;
	const char *a_name;
	enum nsoperation a_operation;
	int a_flags;
	vfs_context_t a_context;
};

   
                                 
                                                        
                                                                                              
                                                                                                      
                              
                                                        
                                                                
                                                                            
                                                                                                         
                                                                                                            
                                  
                                                                                                               
                                                                
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_GETNAMEDSTREAM(vnode_t, vnode_t *, const char *, enum nsoperation, int flags, vfs_context_t);
#endif                         

struct vnop_makenamedstream_args {
	struct vnodeop_desc *a_desc;
	vnode_t *a_svpp;
	vnode_t a_vp;
	const char *a_name;
	int a_flags;
	vfs_context_t a_context;
};

   
                                  
                                                           
                                                                                                          
                                                                                          
                              
                                                        
                                                                
                                                                            
                                  
                                                             
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_MAKENAMEDSTREAM(vnode_t, vnode_t *, const char *, int flags, vfs_context_t);
#endif                         

struct vnop_removenamedstream_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	vnode_t a_svp;
	const char *a_name;
	int a_flags;
	vfs_context_t a_context;
};

   
                                    
                                                           
                                                                                                       
                              
                                                          
                                        
                                                                            
                                  
                                                             
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_REMOVENAMEDSTREAM(vnode_t, vnode_t, const char *, int flags, vfs_context_t);
#endif                         

#endif                

__options_decl(vnode_verify_flags_t, uint32_t, {
	VNODE_VERIFY_DEFAULT = 0,
});

#define VNODE_VERIFY_DEFAULT VNODE_VERIFY_DEFAULT

struct vnop_verify_args {
	struct vnodeop_desc *a_desc;
	vnode_t a_vp;
	off_t a_foffset;
	uint8_t  *a_buf;
	size_t a_bufsize;
	size_t *a_verifyblksize;
	vnode_verify_flags_t a_flags;
	vfs_context_t a_context;
};

   
                         
                                                                          
                                                                                                            
                                                                                                          
                                                                                                  
                                                         
                                                                           
                                                                                                                
                    
                                                      
                                                                                                                              
                                                                                                                        
                                
                                                                                       
                                              
   
#ifdef XNU_KERNEL_PRIVATE
extern errno_t VNOP_VERIFY(vnode_t, off_t, uint8_t *, size_t, size_t *, vnode_verify_flags_t, vfs_context_t);
#endif                         

#endif                                 

__END_DECLS

#endif             

#pragma clang diagnostic pop                                                         
#endif                        
