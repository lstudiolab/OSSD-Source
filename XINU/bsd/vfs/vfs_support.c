/*
 * Copyright (c) 2000-2004 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1998-1999 Apple Computer, Inc.  All rights reserved.
 *
 *  File:  vfs/vfs_support.c
 *
 *	The default VFS routines. A VFS plugin can use these
 *	functions in case it does not want to implement all. These functions
 *	take care of releasing locks and free up memory that they are
 *	supposed to.
 *
 *	nop_* routines always return 0 [success]
 *	err_* routines always return ENOTSUP
 *
 *	This file could be auto-generated from vnode_if.src. but that needs
 *	support for freeing cnp.
 *
 * HISTORY
 *  15-Jul-1998 Earsh Nandkeshwar (earsh@apple.com)
 *      Fixed up readdirattr for its existance now.
 *  18-Aug-1998 Umesh Vaishampayan (umeshv@apple.com)
 *      Created.
 */

#include <vfs/vfs_support.h>
#include <sys/kauth.h>
#include <sys/ubc.h>                               


struct vnop_create_args     
                                                
                                                 
                                                        
                                                      
                                                    
                                ;

int
nop_create(__unused struct vnop_create_args *ap)
{
#if DIAGNOSTIC
	if ((ap->a_cnp->cn_flags & HASBUF) == 0) {
		panic("nop_create: no name");
	}
#endif
	return 0;
}

int
err_create(struct vnop_create_args *ap)
{
	(void)nop_create(ap);
	return ENOTSUP;
}


struct vnop_whiteout_args     
                                                  
                                                          
                                          
                                                      
                                  ;

int
nop_whiteout(__unused struct vnop_whiteout_args *ap)
{
	return 0;
}

int
err_whiteout(__unused struct vnop_whiteout_args *ap)
{
	return ENOTSUP;
}


struct vnop_mknod_args     
                                               
                                                
                                                       
                                                     
                                                   
                               ;

int
nop_mknod(__unused struct vnop_mknod_args *ap)
{
#if DIAGNOSTIC
	if ((ap->a_cnp->cn_flags & HASBUF) == 0) {
		panic("nop_mknod: no name");
	}
#endif
	return 0;
}

int
err_mknod(struct vnop_mknod_args *ap)
{
	(void)nop_mknod(ap);
	return ENOTSUP;
}

struct vnop_open_args     
                                             
                                      
                                                  
                              ;

int
nop_open(__unused struct vnop_open_args *ap)
{
	return 0;
}

int
err_open(__unused struct vnop_open_args *ap)
{
	return ENOTSUP;
}


struct vnop_close_args     
                                              
                                        
                                                   
                               ;

int
nop_close(__unused struct vnop_close_args *ap)
{
	return 0;
}

int
err_close(__unused struct vnop_close_args *ap)
{
	return ENOTSUP;
}


struct vnop_access_args     
                                               
                                        
                                                    
                                ;

int
nop_access(__unused struct vnop_access_args *ap)
{
	return 0;
}

int
err_access(__unused struct vnop_access_args *ap)
{
	return ENOTSUP;
}


struct vnop_getattr_args     
                                                
                                                       
                                                     
                                 ;

int
nop_getattr(__unused struct vnop_getattr_args *ap)
{
	return 0;
}

int
err_getattr(__unused struct vnop_getattr_args *ap)
{
	return ENOTSUP;
}


struct vnop_setattr_args     
                                                
                                                       
                                                     
                                 ;

int
nop_setattr(__unused struct vnop_setattr_args *ap)
{
	return 0;
}

int
err_setattr(__unused struct vnop_setattr_args *ap)
{
	return ENOTSUP;
}

struct vnop_read_args      
                                             
                                            
                                       
                                                  
                              ;

int
nop_read(__unused struct vnop_read_args *ap)
{
	return 0;
}

int
err_read(__unused struct vnop_read_args *ap)
{
	return ENOTSUP;
}


struct vnop_write_args      
                                              
                                             
                                        
                                                   
                               ;

int
nop_write(__unused struct vnop_write_args *ap)
{
	return 0;
}

int
err_write(__unused struct vnop_write_args *ap)
{
	return ENOTSUP;
}


struct vnop_ioctl_args     
                                              
                                            
                                          
                                       
                                               
                                            
                               ;

int
nop_ioctl(__unused struct vnop_ioctl_args *ap)
{
	return 0;
}

int
err_ioctl(__unused struct vnop_ioctl_args *ap)
{
	return ENOTSUP;
}


struct vnop_select_args     
                                               
                                        
                                         
                                                
                                        
                                             
                                ;

int
nop_select(__unused struct vnop_select_args *ap)
{
	return 0;
}

int
err_select(__unused struct vnop_select_args *ap)
{
	return ENOTSUP;
}


struct vnop_exchange_args     
                                                  
                                                  
                                            
                                                      
                                  ;

int
nop_exchange(__unused struct vnop_exchange_args *ap)
{
	return 0;
}

int
err_exchange(__unused struct vnop_exchange_args *ap)
{
	return ENOTSUP;
}


struct vnop_revoke_args     
                                               
                                        
                                                    
                                ;

int
nop_revoke(struct vnop_revoke_args *ap)
{
	return vn_revoke(ap->a_vp, ap->a_flags, ap->a_context);
}

int
err_revoke(struct vnop_revoke_args *ap)
{
	(void)nop_revoke(ap);
	return ENOTSUP;
}


struct vnop_mmap_args     
                                             
                                       
                                              
                                           
                              ;

int
nop_mmap(__unused struct vnop_mmap_args *ap)
{
	return 0;
}

int
err_mmap(__unused struct vnop_mmap_args *ap)
{
	return ENOTSUP;
}


struct vnop_fsync_args     
                                              
                                         
                                                   
                               ;

int
nop_fsync(__unused struct vnop_fsync_args *ap)
{
	return 0;
}

int
err_fsync(__unused struct vnop_fsync_args *ap)
{
	return ENOTSUP;
}


struct vnop_remove_args     
                                                
                                               
                                                        
                                        
                                                    
                                ;

int
nop_remove(__unused struct vnop_remove_args *ap)
{
	return 0;
}

int
err_remove(struct vnop_remove_args *ap)
{
	(void)nop_remove(ap);
	return ENOTSUP;
}


struct vnop_link_args     
                                             
                                               
                                                      
                                                  
                              ;

int
nop_link(__unused struct vnop_link_args *ap)
{
	return 0;
}

int
err_link(struct vnop_link_args *ap)
{
	(void)nop_link(ap);
	return ENOTSUP;
}


struct vnop_rename_args     
                                                 
                                                
                                                         
                                                 
                                                
                                                         
                                                    
                                ;

int
nop_rename(__unused struct vnop_rename_args *ap)
{
	return 0;
}

int
err_rename(struct vnop_rename_args *ap)
{
	(void)nop_rename(ap);
	return ENOTSUP;
}


struct vnop_mkdir_args     
                                               
                                                
                                                       
                                                     
                                                   
                               ;

int
nop_mkdir(__unused struct vnop_mkdir_args *ap)
{
	return 0;
}

int
err_mkdir(__unused struct vnop_mkdir_args *ap)
{
	return ENOTSUP;
}


struct vnop_rmdir_args     
                                               
                                              
                                                       
                                                   
                               ;

int
nop_rmdir(__unused struct vnop_rmdir_args *ap)
{
	return 0;
}

int
err_rmdir(struct vnop_rmdir_args *ap)
{
	(void)nop_rmdir(ap);
	return ENOTSUP;
}


struct vnop_symlink_args     
                                                 
                                                  
                                                         
                                                       
                                            
                                                     
                                 ;

int
nop_symlink(__unused struct vnop_symlink_args *ap)
{
#if DIAGNOSTIC
	if ((ap->a_cnp->cn_flags & HASBUF) == 0) {
		panic("nop_symlink: no name");
	}
#endif
	return 0;
}

int
err_symlink(struct vnop_symlink_args *ap)
{
	(void)nop_symlink(ap);
	return ENOTSUP;
}


struct vnop_readdir_args     
                                          
                                               
                                         
                                            
                                              
                                                     
                                 ;

int
nop_readdir(__unused struct vnop_readdir_args *ap)
{
	return 0;
}

int
err_readdir(__unused struct vnop_readdir_args *ap)
{
	return ENOTSUP;
}


struct vnop_readdirattr_args     
                                                             
                                              
                                                          
                                                   
                                                   
                                                  
                                                    
                                                
                                                       
                                                         
                                     ;

int
nop_readdirattr(struct vnop_readdirattr_args *ap)
{
	*(ap->a_actualcount) = 0;
	*(ap->a_eofflag) = 0;
	return 0;
}

int
err_readdirattr(struct vnop_readdirattr_args *ap)
{
	(void)nop_readdirattr(ap);
	return ENOTSUP;
}


struct vnop_readlink_args     
                                               
                                              
                                                      
                                  ;

int
nop_readlink(__unused struct vnop_readlink_args *ap)
{
	return 0;
}

int
err_readlink(__unused struct vnop_readlink_args *ap)
{
	return ENOTSUP;
}


struct vnop_inactive_args     
                                                 
                                                      
                                  ;

int
nop_inactive(__unused struct vnop_inactive_args *ap)
{
	return 0;
}

int
err_inactive(struct vnop_inactive_args *ap)
{
	(void)nop_inactive(ap);
	return ENOTSUP;
}


struct vnop_reclaim_args     
                                                
                                                     
                                 ;

int
nop_reclaim(__unused struct vnop_reclaim_args *ap)
{
	return 0;
}

int
err_reclaim(__unused struct vnop_reclaim_args *ap)
{
	return ENOTSUP;
}


struct vnop_strategy_args     
                                               
                                  ;

int
nop_strategy(__unused struct vnop_strategy_args *ap)
{
	return 0;
}

int
err_strategy(__unused struct vnop_strategy_args *ap)
{
	return ENOTSUP;
}


struct vnop_pathconf_args     
                                                 
                                         
                                                
                                                      
                                  ;

int
nop_pathconf(__unused struct vnop_pathconf_args *ap)
{
	return 0;
}

int
err_pathconf(__unused struct vnop_pathconf_args *ap)
{
	return ENOTSUP;
}


struct vnop_advlock_args     
                                                
                                          
                                      
                                                
                                         
                                                     
                                 ;

int
nop_advlock(__unused struct vnop_advlock_args *ap)
{
	return 0;
}

int
err_advlock(__unused struct vnop_advlock_args *ap)
{
	return ENOTSUP;
}



struct vnop_allocate_args     
                                                 
                                             
                                                
                                                      
                                             
                                                      
                                  ;

int
nop_allocate(struct vnop_allocate_args *ap)
{
	*(ap->a_bytesallocated) = 0;
	return 0;
}

int
err_allocate(struct vnop_allocate_args *ap)
{
	(void)nop_allocate(ap);
	return ENOTSUP;
}

struct vnop_bwrite_args     
                                             
                                ;

int
nop_bwrite(struct vnop_bwrite_args *ap)
{
	return (int)buf_bwrite(ap->a_bp);
}

int
err_bwrite(__unused struct vnop_bwrite_args *ap)
{
	return ENOTSUP;
}


struct vnop_pagein_args     
                                                      
                                        
                                                    
                                             
                                           
                                        
                                                    
                                ;

int
nop_pagein(struct vnop_pagein_args *ap)
{
	if (!(ap->a_flags & UPL_NOCOMMIT)) {
		assert(ap->a_size <= UINT32_MAX);
		ubc_upl_abort_range(ap->a_pl, ap->a_pl_offset, (upl_size_t)ap->a_size, UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_ERROR);
	}
	return EINVAL;
}

int
err_pagein(struct vnop_pagein_args *ap)
{
	if (!(ap->a_flags & UPL_NOCOMMIT)) {
		assert(ap->a_size <= UINT32_MAX);
		ubc_upl_abort_range(ap->a_pl, ap->a_pl_offset, (upl_size_t)ap->a_size, UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_ERROR);
	}
	return ENOTSUP;
}


struct vnop_pageout_args     
                                                      
                                         
                                                     
                                              
                                            
                                         
                                                     
                                 ;

int
nop_pageout(struct vnop_pageout_args *ap)
{
	if (!(ap->a_flags & UPL_NOCOMMIT)) {
		assert(ap->a_size <= UINT32_MAX);
		ubc_upl_abort_range(ap->a_pl, ap->a_pl_offset, (upl_size_t)ap->a_size, UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_ERROR);
	}
	return EINVAL;
}

int
err_pageout(struct vnop_pageout_args *ap)
{
	if (!(ap->a_flags & UPL_NOCOMMIT)) {
		assert(ap->a_size <= UINT32_MAX);
		ubc_upl_abort_range(ap->a_pl, ap->a_pl_offset, (upl_size_t)ap->a_size, UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_ERROR);
	}
	return ENOTSUP;
}


struct vnop_searchfs     
                                            
                                               
                                               
                                                        
                                             
                                                     
                                                        
                                              
                                             
                                          
                                           
                                                           
                                                 
                             ;

int
nop_searchfs(struct vnop_searchfs_args *ap)
{
	*(ap->a_nummatches) = 0;
	return 0;
}

int
err_searchfs(struct vnop_searchfs_args *ap)
{
	(void)nop_searchfs(ap);
	return ENOTSUP;
}

struct vnop_copyfile_args    
                                                          
                                                  
                                                   
                                                  
                                                           
                                          
                                 ;

int
nop_copyfile(__unused struct vnop_copyfile_args *ap)
{
	return 0;
}


int
err_copyfile(struct vnop_copyfile_args *ap)
{
	(void)nop_copyfile(ap);
	return ENOTSUP;
}


struct vnop_blktooff_args     
                                                 
                                                 
                                              
                                  ;

int
nop_blktooff(struct vnop_blktooff_args *ap)
{
	*ap->a_offset = (off_t)-1;                   
	return 0;
}

int
err_blktooff(struct vnop_blktooff_args *ap)
{
	(void)nop_blktooff(ap);
	return ENOTSUP;
}

struct vnop_offtoblk_args     
                                                 
                                             
                                                  
                                  ;

int
nop_offtoblk(struct vnop_offtoblk_args *ap)
{
	*ap->a_lblkno = (daddr64_t)-1;               
	return 0;
}

int
err_offtoblk(struct vnop_offtoblk_args *ap)
{
	(void)nop_offtoblk(ap);
	return ENOTSUP;
}

struct vnop_blockmap_args     
                                                 
                                              
                                            
                                               
                                            
                                           
                                          
                                  ;

int
nop_blockmap(__unused struct vnop_blockmap_args *ap)
{
	return 0;
}

int
err_blockmap(__unused struct vnop_blockmap_args *ap)
{
	return ENOTSUP;
}
