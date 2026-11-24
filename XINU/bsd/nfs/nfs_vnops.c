/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
 *	@(#)nfs_vnops.c	8.16 (Berkeley) 5/27/95
 * FreeBSD-Id: nfs_vnops.c,v 1.72 1997/11/07 09:20:48 phk Exp $
 */

#include <nfs/nfs_conf.h>
#if CONFIG_NFS_CLIENT

  
                                             
   
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/resourcevar.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/mount_internal.h>
#include <sys/malloc.h>
#include <sys/kpi_mbuf.h>
#include <sys/conf.h>
#include <sys/vnode_internal.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/lockf.h>
#include <sys/ubc_internal.h>
#include <sys/attr.h>
#include <sys/signalvar.h>
#include <sys/uio_internal.h>
#include <sys/xattr.h>

#include <vfs/vfs_support.h>

#include <sys/vm.h>

#include <sys/time.h>
#include <kern/clock.h>
#include <libkern/OSAtomic.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/specfs/specdev.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <nfs/nfs_gss.h>
#include <nfs/nfsmount.h>
#include <nfs/nfs_lock.h>
#include <nfs/xdr_subs.h>
#include <nfs/nfsm_subs.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#include <vm/vm_kern.h>
#include <vm/vm_pageout.h>

#include <kern/task.h>
#include <kern/sched_prim.h>

#define NFS_VNOP_DBG(...) NFS_DBG(NFS_FAC_VNOP, 7, ## __VA_ARGS__)
#define DEFAULT_READLINK_NOCACHE 0

  
                
   
int     nfs_vnop_lookup(struct vnop_lookup_args *);
int     nfsspec_vnop_read(struct vnop_read_args *);
int     nfsspec_vnop_write(struct vnop_write_args *);
int     nfsspec_vnop_close(struct vnop_close_args *);
#if FIFO
int     nfsfifo_vnop_read(struct vnop_read_args *);
int     nfsfifo_vnop_write(struct vnop_write_args *);
int     nfsfifo_vnop_close(struct vnop_close_args *);
#endif
int     nfs_vnop_ioctl(struct vnop_ioctl_args *);
int     nfs_vnop_select(struct vnop_select_args *);
int     nfs_vnop_setattr(struct vnop_setattr_args *);
int     nfs_vnop_fsync(struct vnop_fsync_args *);
int     nfs_vnop_rename(struct vnop_rename_args *);
int     nfs_vnop_readdir(struct vnop_readdir_args *);
int     nfs_vnop_readlink(struct vnop_readlink_args *);
int     nfs_vnop_pathconf(struct vnop_pathconf_args *);
int     nfs_vnop_pagein(struct vnop_pagein_args *);
int     nfs_vnop_pageout(struct vnop_pageout_args *);
int     nfs_vnop_blktooff(struct vnop_blktooff_args *);
int     nfs_vnop_offtoblk(struct vnop_offtoblk_args *);
int     nfs_vnop_blockmap(struct vnop_blockmap_args *);
int     nfs_vnop_monitor(struct vnop_monitor_args *);

int     nfs3_vnop_create(struct vnop_create_args *);
int     nfs3_vnop_mknod(struct vnop_mknod_args *);
int     nfs3_vnop_getattr(struct vnop_getattr_args *);
int     nfs3_vnop_link(struct vnop_link_args *);
int     nfs3_vnop_mkdir(struct vnop_mkdir_args *);
int     nfs3_vnop_rmdir(struct vnop_rmdir_args *);
int     nfs3_vnop_symlink(struct vnop_symlink_args *);


vnop_t **nfsv2_vnodeop_p;
static const struct vnodeopv_entry_desc nfsv2_vnodeop_entries[] = {
	{ .opve_op = &vnop_default_desc, .opve_impl = (vnop_t *)vn_default_error },
	{ .opve_op = &vnop_lookup_desc, .opve_impl = (vnop_t *)nfs_vnop_lookup },                     
	{ .opve_op = &vnop_create_desc, .opve_impl = (vnop_t *)nfs3_vnop_create },                    
	{ .opve_op = &vnop_mknod_desc, .opve_impl = (vnop_t *)nfs3_vnop_mknod },                     
	{ .opve_op = &vnop_open_desc, .opve_impl = (vnop_t *)nfs_vnop_open },                       
	{ .opve_op = &vnop_close_desc, .opve_impl = (vnop_t *)nfs_vnop_close },                      
	{ .opve_op = &vnop_access_desc, .opve_impl = (vnop_t *)nfs_vnop_access },                     
	{ .opve_op = &vnop_getattr_desc, .opve_impl = (vnop_t *)nfs3_vnop_getattr },                   
	{ .opve_op = &vnop_setattr_desc, .opve_impl = (vnop_t *)nfs_vnop_setattr },                    
	{ .opve_op = &vnop_read_desc, .opve_impl = (vnop_t *)nfs_vnop_read },                       
	{ .opve_op = &vnop_write_desc, .opve_impl = (vnop_t *)nfs_vnop_write },                      
	{ .opve_op = &vnop_ioctl_desc, .opve_impl = (vnop_t *)nfs_vnop_ioctl },                      
	{ .opve_op = &vnop_select_desc, .opve_impl = (vnop_t *)nfs_vnop_select },                     
	{ .opve_op = &vnop_revoke_desc, .opve_impl = (vnop_t *)nfs_vnop_revoke },                     
	{ .opve_op = &vnop_mmap_desc, .opve_impl = (vnop_t *)nfs_vnop_mmap },                       
	{ .opve_op = &vnop_mmap_check_desc, .opve_impl = (vnop_t *)nfs_vnop_mmap_check },                 
	{ .opve_op = &vnop_mnomap_desc, .opve_impl = (vnop_t *)nfs_vnop_mnomap },                     
	{ .opve_op = &vnop_fsync_desc, .opve_impl = (vnop_t *)nfs_vnop_fsync },                      
	{ .opve_op = &vnop_remove_desc, .opve_impl = (vnop_t *)nfs_vnop_remove },                     
	{ .opve_op = &vnop_link_desc, .opve_impl = (vnop_t *)nfs3_vnop_link },                      
	{ .opve_op = &vnop_rename_desc, .opve_impl = (vnop_t *)nfs_vnop_rename },                     
	{ .opve_op = &vnop_mkdir_desc, .opve_impl = (vnop_t *)nfs3_vnop_mkdir },                     
	{ .opve_op = &vnop_rmdir_desc, .opve_impl = (vnop_t *)nfs3_vnop_rmdir },                     
	{ .opve_op = &vnop_symlink_desc, .opve_impl = (vnop_t *)nfs3_vnop_symlink },                   
	{ .opve_op = &vnop_readdir_desc, .opve_impl = (vnop_t *)nfs_vnop_readdir },                    
	{ .opve_op = &vnop_readlink_desc, .opve_impl = (vnop_t *)nfs_vnop_readlink },                   
	{ .opve_op = &vnop_inactive_desc, .opve_impl = (vnop_t *)nfs_vnop_inactive },                   
	{ .opve_op = &vnop_reclaim_desc, .opve_impl = (vnop_t *)nfs_vnop_reclaim },                    
	{ .opve_op = &vnop_strategy_desc, .opve_impl = (vnop_t *)err_strategy },                        
	{ .opve_op = &vnop_pathconf_desc, .opve_impl = (vnop_t *)nfs_vnop_pathconf },                   
	{ .opve_op = &vnop_advlock_desc, .opve_impl = (vnop_t *)nfs_vnop_advlock },                    
	{ .opve_op = &vnop_bwrite_desc, .opve_impl = (vnop_t *)err_bwrite },                          
	{ .opve_op = &vnop_pagein_desc, .opve_impl = (vnop_t *)nfs_vnop_pagein },                     
	{ .opve_op = &vnop_pageout_desc, .opve_impl = (vnop_t *)nfs_vnop_pageout },                    
	{ .opve_op = &vnop_copyfile_desc, .opve_impl = (vnop_t *)err_copyfile },                        
	{ .opve_op = &vnop_blktooff_desc, .opve_impl = (vnop_t *)nfs_vnop_blktooff },                   
	{ .opve_op = &vnop_offtoblk_desc, .opve_impl = (vnop_t *)nfs_vnop_offtoblk },                   
	{ .opve_op = &vnop_blockmap_desc, .opve_impl = (vnop_t *)nfs_vnop_blockmap },                   
	{ .opve_op = &vnop_monitor_desc, .opve_impl = (vnop_t *)nfs_vnop_monitor },                    
	{ .opve_op = NULL, .opve_impl = NULL }
};
const struct vnodeopv_desc nfsv2_vnodeop_opv_desc =
{ &nfsv2_vnodeop_p, nfsv2_vnodeop_entries };


#if CONFIG_NFS4
vnop_t **nfsv4_vnodeop_p;
static const struct vnodeopv_entry_desc nfsv4_vnodeop_entries[] = {
	{ &vnop_default_desc, (vnop_t *)vn_default_error },
	{ &vnop_lookup_desc, (vnop_t *)nfs_vnop_lookup },                                    
	{ &vnop_create_desc, (vnop_t *)nfs4_vnop_create },                                   
	{ &vnop_mknod_desc, (vnop_t *)nfs4_vnop_mknod },                                    
	{ &vnop_open_desc, (vnop_t *)nfs_vnop_open },                                      
	{ &vnop_close_desc, (vnop_t *)nfs_vnop_close },                                     
	{ &vnop_access_desc, (vnop_t *)nfs_vnop_access },                                    
	{ &vnop_getattr_desc, (vnop_t *)nfs4_vnop_getattr },                                  
	{ &vnop_setattr_desc, (vnop_t *)nfs_vnop_setattr },                                   
	{ &vnop_read_desc, (vnop_t *)nfs_vnop_read },                                      
	{ &vnop_write_desc, (vnop_t *)nfs_vnop_write },                                     
	{ &vnop_ioctl_desc, (vnop_t *)nfs_vnop_ioctl },                                     
	{ &vnop_select_desc, (vnop_t *)nfs_vnop_select },                                    
	{ &vnop_revoke_desc, (vnop_t *)nfs_vnop_revoke },                                    
	{ &vnop_mmap_desc, (vnop_t *)nfs_vnop_mmap },                                      
	{ &vnop_mmap_check_desc, (vnop_t *)nfs_vnop_mmap_check },                                
	{ &vnop_mnomap_desc, (vnop_t *)nfs_vnop_mnomap },                                    
	{ &vnop_fsync_desc, (vnop_t *)nfs_vnop_fsync },                                     
	{ &vnop_remove_desc, (vnop_t *)nfs_vnop_remove },                                    
	{ &vnop_link_desc, (vnop_t *)nfs4_vnop_link },                                     
	{ &vnop_rename_desc, (vnop_t *)nfs_vnop_rename },                                    
	{ &vnop_mkdir_desc, (vnop_t *)nfs4_vnop_mkdir },                                    
	{ &vnop_rmdir_desc, (vnop_t *)nfs4_vnop_rmdir },                                    
	{ &vnop_symlink_desc, (vnop_t *)nfs4_vnop_symlink },                                  
	{ &vnop_readdir_desc, (vnop_t *)nfs_vnop_readdir },                                   
	{ &vnop_readlink_desc, (vnop_t *)nfs_vnop_readlink },                                  
	{ &vnop_inactive_desc, (vnop_t *)nfs_vnop_inactive },                                  
	{ &vnop_reclaim_desc, (vnop_t *)nfs_vnop_reclaim },                                   
	{ &vnop_strategy_desc, (vnop_t *)err_strategy },                                       
	{ &vnop_pathconf_desc, (vnop_t *)nfs_vnop_pathconf },                                  
	{ &vnop_advlock_desc, (vnop_t *)nfs_vnop_advlock },                                   
	{ &vnop_bwrite_desc, (vnop_t *)err_bwrite },                                         
	{ &vnop_pagein_desc, (vnop_t *)nfs_vnop_pagein },                                    
	{ &vnop_pageout_desc, (vnop_t *)nfs_vnop_pageout },                                   
	{ &vnop_copyfile_desc, (vnop_t *)err_copyfile },                                       
	{ &vnop_blktooff_desc, (vnop_t *)nfs_vnop_blktooff },                                  
	{ &vnop_offtoblk_desc, (vnop_t *)nfs_vnop_offtoblk },                                  
	{ &vnop_blockmap_desc, (vnop_t *)nfs_vnop_blockmap },                                  
	{ &vnop_getxattr_desc, (vnop_t *)nfs4_vnop_getxattr },                                 
	{ &vnop_setxattr_desc, (vnop_t *)nfs4_vnop_setxattr },                                 
	{ &vnop_removexattr_desc, (vnop_t *)nfs4_vnop_removexattr },                              
	{ &vnop_listxattr_desc, (vnop_t *)nfs4_vnop_listxattr },                                
#if NAMEDSTREAMS
	{ &vnop_getnamedstream_desc, (vnop_t *)nfs4_vnop_getnamedstream },                           
	{ &vnop_makenamedstream_desc, (vnop_t *)nfs4_vnop_makenamedstream },                          
	{ &vnop_removenamedstream_desc, (vnop_t *)nfs4_vnop_removenamedstream },                        
#endif
	{ &vnop_monitor_desc, (vnop_t *)nfs_vnop_monitor },                                   
	{ NULL, NULL }
};
const struct vnodeopv_desc nfsv4_vnodeop_opv_desc =
{ &nfsv4_vnodeop_p, nfsv4_vnodeop_entries };
#endif

  
                           
   
vnop_t **spec_nfsv2nodeop_p;
static const struct vnodeopv_entry_desc spec_nfsv2nodeop_entries[] = {
	{ &vnop_default_desc, (vnop_t *)vn_default_error },
	{ &vnop_lookup_desc, (vnop_t *)spec_lookup },                       
	{ &vnop_create_desc, (vnop_t *)spec_create },                       
	{ &vnop_mknod_desc, (vnop_t *)spec_mknod },                        
	{ &vnop_open_desc, (vnop_t *)spec_open },                         
	{ &vnop_close_desc, (vnop_t *)nfsspec_vnop_close },                
	{ &vnop_getattr_desc, (vnop_t *)nfs3_vnop_getattr },                 
	{ &vnop_setattr_desc, (vnop_t *)nfs_vnop_setattr },                  
	{ &vnop_read_desc, (vnop_t *)nfsspec_vnop_read },                 
	{ &vnop_write_desc, (vnop_t *)nfsspec_vnop_write },                
	{ &vnop_ioctl_desc, (vnop_t *)spec_ioctl },                        
	{ &vnop_select_desc, (vnop_t *)spec_select },                       
	{ &vnop_revoke_desc, (vnop_t *)spec_revoke },                       
	{ &vnop_mmap_desc, (vnop_t *)spec_mmap },                         
	{ &vnop_fsync_desc, (vnop_t *)nfs_vnop_fsync },                    
	{ &vnop_remove_desc, (vnop_t *)spec_remove },                       
	{ &vnop_link_desc, (vnop_t *)spec_link },                         
	{ &vnop_rename_desc, (vnop_t *)spec_rename },                       
	{ &vnop_mkdir_desc, (vnop_t *)spec_mkdir },                        
	{ &vnop_rmdir_desc, (vnop_t *)spec_rmdir },                        
	{ &vnop_symlink_desc, (vnop_t *)spec_symlink },                      
	{ &vnop_readdir_desc, (vnop_t *)spec_readdir },                      
	{ &vnop_readlink_desc, (vnop_t *)spec_readlink },                     
	{ &vnop_inactive_desc, (vnop_t *)nfs_vnop_inactive },                 
	{ &vnop_reclaim_desc, (vnop_t *)nfs_vnop_reclaim },                  
	{ &vnop_strategy_desc, (vnop_t *)spec_strategy },                     
	{ &vnop_pathconf_desc, (vnop_t *)spec_pathconf },                     
	{ &vnop_advlock_desc, (vnop_t *)spec_advlock },                      
	{ &vnop_bwrite_desc, (vnop_t *)vn_bwrite },                         
	{ &vnop_pagein_desc, (vnop_t *)nfs_vnop_pagein },                   
	{ &vnop_pageout_desc, (vnop_t *)nfs_vnop_pageout },                  
	{ &vnop_blktooff_desc, (vnop_t *)nfs_vnop_blktooff },                 
	{ &vnop_offtoblk_desc, (vnop_t *)nfs_vnop_offtoblk },                 
	{ &vnop_blockmap_desc, (vnop_t *)nfs_vnop_blockmap },                 
	{ &vnop_monitor_desc, (vnop_t *)nfs_vnop_monitor },                  
	{ NULL, NULL }
};
const struct vnodeopv_desc spec_nfsv2nodeop_opv_desc =
{ &spec_nfsv2nodeop_p, spec_nfsv2nodeop_entries };
#if CONFIG_NFS4
vnop_t **spec_nfsv4nodeop_p;
static const struct vnodeopv_entry_desc spec_nfsv4nodeop_entries[] = {
	{ &vnop_default_desc, (vnop_t *)vn_default_error },
	{ &vnop_lookup_desc, (vnop_t *)spec_lookup },                       
	{ &vnop_create_desc, (vnop_t *)spec_create },                       
	{ &vnop_mknod_desc, (vnop_t *)spec_mknod },                        
	{ &vnop_open_desc, (vnop_t *)spec_open },                         
	{ &vnop_close_desc, (vnop_t *)nfsspec_vnop_close },                
	{ &vnop_getattr_desc, (vnop_t *)nfs4_vnop_getattr },                 
	{ &vnop_setattr_desc, (vnop_t *)nfs_vnop_setattr },                  
	{ &vnop_read_desc, (vnop_t *)nfsspec_vnop_read },                 
	{ &vnop_write_desc, (vnop_t *)nfsspec_vnop_write },                
	{ &vnop_ioctl_desc, (vnop_t *)spec_ioctl },                        
	{ &vnop_select_desc, (vnop_t *)spec_select },                       
	{ &vnop_revoke_desc, (vnop_t *)spec_revoke },                       
	{ &vnop_mmap_desc, (vnop_t *)spec_mmap },                         
	{ &vnop_fsync_desc, (vnop_t *)nfs_vnop_fsync },                    
	{ &vnop_remove_desc, (vnop_t *)spec_remove },                       
	{ &vnop_link_desc, (vnop_t *)spec_link },                         
	{ &vnop_rename_desc, (vnop_t *)spec_rename },                       
	{ &vnop_mkdir_desc, (vnop_t *)spec_mkdir },                        
	{ &vnop_rmdir_desc, (vnop_t *)spec_rmdir },                        
	{ &vnop_symlink_desc, (vnop_t *)spec_symlink },                      
	{ &vnop_readdir_desc, (vnop_t *)spec_readdir },                      
	{ &vnop_readlink_desc, (vnop_t *)spec_readlink },                     
	{ &vnop_inactive_desc, (vnop_t *)nfs_vnop_inactive },                 
	{ &vnop_reclaim_desc, (vnop_t *)nfs_vnop_reclaim },                  
	{ &vnop_strategy_desc, (vnop_t *)spec_strategy },                     
	{ &vnop_pathconf_desc, (vnop_t *)spec_pathconf },                     
	{ &vnop_advlock_desc, (vnop_t *)spec_advlock },                      
	{ &vnop_bwrite_desc, (vnop_t *)vn_bwrite },                         
	{ &vnop_pagein_desc, (vnop_t *)nfs_vnop_pagein },                   
	{ &vnop_pageout_desc, (vnop_t *)nfs_vnop_pageout },                  
	{ &vnop_blktooff_desc, (vnop_t *)nfs_vnop_blktooff },                 
	{ &vnop_offtoblk_desc, (vnop_t *)nfs_vnop_offtoblk },                 
	{ &vnop_blockmap_desc, (vnop_t *)nfs_vnop_blockmap },                 
	{ &vnop_getxattr_desc, (vnop_t *)nfs4_vnop_getxattr },                
	{ &vnop_setxattr_desc, (vnop_t *)nfs4_vnop_setxattr },                
	{ &vnop_removexattr_desc, (vnop_t *)nfs4_vnop_removexattr },                 
	{ &vnop_listxattr_desc, (vnop_t *)nfs4_vnop_listxattr },               
#if NAMEDSTREAMS
	{ &vnop_getnamedstream_desc, (vnop_t *)nfs4_vnop_getnamedstream },                          
	{ &vnop_makenamedstream_desc, (vnop_t *)nfs4_vnop_makenamedstream },                         
	{ &vnop_removenamedstream_desc, (vnop_t *)nfs4_vnop_removenamedstream },                       
#endif
	{ &vnop_monitor_desc, (vnop_t *)nfs_vnop_monitor },                  
	{ NULL, NULL }
};
const struct vnodeopv_desc spec_nfsv4nodeop_opv_desc =
{ &spec_nfsv4nodeop_p, spec_nfsv4nodeop_entries };
#endif                  

#if FIFO
vnop_t **fifo_nfsv2nodeop_p;
static const struct vnodeopv_entry_desc fifo_nfsv2nodeop_entries[] = {
	{ &vnop_default_desc, (vnop_t *)vn_default_error },
	{ &vnop_lookup_desc, (vnop_t *)fifo_lookup },                       
	{ &vnop_create_desc, (vnop_t *)fifo_create },                       
	{ &vnop_mknod_desc, (vnop_t *)fifo_mknod },                        
	{ &vnop_open_desc, (vnop_t *)fifo_open },                         
	{ &vnop_close_desc, (vnop_t *)nfsfifo_vnop_close },                
	{ &vnop_getattr_desc, (vnop_t *)nfs3_vnop_getattr },                 
	{ &vnop_setattr_desc, (vnop_t *)nfs_vnop_setattr },                  
	{ &vnop_read_desc, (vnop_t *)nfsfifo_vnop_read },                 
	{ &vnop_write_desc, (vnop_t *)nfsfifo_vnop_write },                
	{ &vnop_ioctl_desc, (vnop_t *)fifo_ioctl },                        
	{ &vnop_select_desc, (vnop_t *)fifo_select },                       
	{ &vnop_revoke_desc, (vnop_t *)fifo_revoke },                       
	{ &vnop_mmap_desc, (vnop_t *)fifo_mmap },                         
	{ &vnop_fsync_desc, (vnop_t *)nfs_vnop_fsync },                    
	{ &vnop_remove_desc, (vnop_t *)fifo_remove },                       
	{ &vnop_link_desc, (vnop_t *)fifo_link },                         
	{ &vnop_rename_desc, (vnop_t *)fifo_rename },                       
	{ &vnop_mkdir_desc, (vnop_t *)fifo_mkdir },                        
	{ &vnop_rmdir_desc, (vnop_t *)fifo_rmdir },                        
	{ &vnop_symlink_desc, (vnop_t *)fifo_symlink },                      
	{ &vnop_readdir_desc, (vnop_t *)fifo_readdir },                      
	{ &vnop_readlink_desc, (vnop_t *)fifo_readlink },                     
	{ &vnop_inactive_desc, (vnop_t *)nfs_vnop_inactive },                 
	{ &vnop_reclaim_desc, (vnop_t *)nfs_vnop_reclaim },                  
	{ &vnop_strategy_desc, (vnop_t *)fifo_strategy },                     
	{ &vnop_pathconf_desc, (vnop_t *)fifo_pathconf },                     
	{ &vnop_advlock_desc, (vnop_t *)fifo_advlock },                      
	{ &vnop_bwrite_desc, (vnop_t *)vn_bwrite },                         
	{ &vnop_pagein_desc, (vnop_t *)nfs_vnop_pagein },                   
	{ &vnop_pageout_desc, (vnop_t *)nfs_vnop_pageout },                  
	{ &vnop_blktooff_desc, (vnop_t *)nfs_vnop_blktooff },                 
	{ &vnop_offtoblk_desc, (vnop_t *)nfs_vnop_offtoblk },                 
	{ &vnop_blockmap_desc, (vnop_t *)nfs_vnop_blockmap },                 
	{ &vnop_monitor_desc, (vnop_t *)nfs_vnop_monitor },                  
	{ NULL, NULL }
};
const struct vnodeopv_desc fifo_nfsv2nodeop_opv_desc =
{ &fifo_nfsv2nodeop_p, fifo_nfsv2nodeop_entries };
#endif

#if CONFIG_NFS4
#if FIFO
vnop_t **fifo_nfsv4nodeop_p;
static const struct vnodeopv_entry_desc fifo_nfsv4nodeop_entries[] = {
	{ &vnop_default_desc, (vnop_t *)vn_default_error },
	{ &vnop_lookup_desc, (vnop_t *)fifo_lookup },                       
	{ &vnop_create_desc, (vnop_t *)fifo_create },                       
	{ &vnop_mknod_desc, (vnop_t *)fifo_mknod },                        
	{ &vnop_open_desc, (vnop_t *)fifo_open },                         
	{ &vnop_close_desc, (vnop_t *)nfsfifo_vnop_close },                
	{ &vnop_getattr_desc, (vnop_t *)nfs4_vnop_getattr },                 
	{ &vnop_setattr_desc, (vnop_t *)nfs_vnop_setattr },                  
	{ &vnop_read_desc, (vnop_t *)nfsfifo_vnop_read },                 
	{ &vnop_write_desc, (vnop_t *)nfsfifo_vnop_write },                
	{ &vnop_ioctl_desc, (vnop_t *)fifo_ioctl },                        
	{ &vnop_select_desc, (vnop_t *)fifo_select },                       
	{ &vnop_revoke_desc, (vnop_t *)fifo_revoke },                       
	{ &vnop_mmap_desc, (vnop_t *)fifo_mmap },                         
	{ &vnop_fsync_desc, (vnop_t *)nfs_vnop_fsync },                    
	{ &vnop_remove_desc, (vnop_t *)fifo_remove },                       
	{ &vnop_link_desc, (vnop_t *)fifo_link },                         
	{ &vnop_rename_desc, (vnop_t *)fifo_rename },                       
	{ &vnop_mkdir_desc, (vnop_t *)fifo_mkdir },                        
	{ &vnop_rmdir_desc, (vnop_t *)fifo_rmdir },                        
	{ &vnop_symlink_desc, (vnop_t *)fifo_symlink },                      
	{ &vnop_readdir_desc, (vnop_t *)fifo_readdir },                      
	{ &vnop_readlink_desc, (vnop_t *)fifo_readlink },                     
	{ &vnop_inactive_desc, (vnop_t *)nfs_vnop_inactive },                 
	{ &vnop_reclaim_desc, (vnop_t *)nfs_vnop_reclaim },                  
	{ &vnop_strategy_desc, (vnop_t *)fifo_strategy },                     
	{ &vnop_pathconf_desc, (vnop_t *)fifo_pathconf },                     
	{ &vnop_advlock_desc, (vnop_t *)fifo_advlock },                      
	{ &vnop_bwrite_desc, (vnop_t *)vn_bwrite },                         
	{ &vnop_pagein_desc, (vnop_t *)nfs_vnop_pagein },                   
	{ &vnop_pageout_desc, (vnop_t *)nfs_vnop_pageout },                  
	{ &vnop_blktooff_desc, (vnop_t *)nfs_vnop_blktooff },                 
	{ &vnop_offtoblk_desc, (vnop_t *)nfs_vnop_offtoblk },                 
	{ &vnop_blockmap_desc, (vnop_t *)nfs_vnop_blockmap },                 
	{ &vnop_getxattr_desc, (vnop_t *)nfs4_vnop_getxattr },                
	{ &vnop_setxattr_desc, (vnop_t *)nfs4_vnop_setxattr },                
	{ &vnop_removexattr_desc, (vnop_t *)nfs4_vnop_removexattr },                 
	{ &vnop_listxattr_desc, (vnop_t *)nfs4_vnop_listxattr },               
#if NAMEDSTREAMS
	{ &vnop_getnamedstream_desc, (vnop_t *)nfs4_vnop_getnamedstream },                          
	{ &vnop_makenamedstream_desc, (vnop_t *)nfs4_vnop_makenamedstream },                         
	{ &vnop_removenamedstream_desc, (vnop_t *)nfs4_vnop_removenamedstream },                       
#endif
	{ &vnop_monitor_desc, (vnop_t *)nfs_vnop_monitor },                  
	{ NULL, NULL }
};
const struct vnodeopv_desc fifo_nfsv4nodeop_opv_desc =
{ &fifo_nfsv4nodeop_p, fifo_nfsv4nodeop_entries };
#endif           
#endif                  

int     nfs_sillyrename(nfsnode_t, nfsnode_t, struct componentname *, vfs_context_t);
int     nfs_getattr_internal(nfsnode_t, struct nfs_vattr *, vfs_context_t, int);
int     nfs_refresh_fh(nfsnode_t, vfs_context_t);


ZONE_VIEW_DEFINE(ZV_NFSDIROFF, "NFSV3 diroff",
    KHEAP_ID_DATA_BUFFERS, sizeof(struct nfsdmap));

static void
nfs_dir_buf_cache_lookup_boundaries(struct nfsbuf *bp, int *sof, int *eof)
{
	if (bp) {
		struct nfs_dir_buf_header *ndbhp = (struct nfs_dir_buf_header*)bp->nb_data;
		if (sof && bp->nb_lblkno == 0) {
			*sof = 1;
		}
		if (eof && ISSET(ndbhp->ndbh_flags, NDB_EOF)) {
			*eof = 1;
		}
	}
}

  
                                                                            
                                                                   
   
void
nfs_rdirplus_update_node_attrs(nfsnode_t dnp, struct direntry *dp, fhandle_t *fhp, struct nfs_vattr *nvattrp, uint64_t *savedxidp)
{
	nfsnode_t np;
	struct componentname cn;
	int isdot = (dp->d_namlen == 1) && (dp->d_name[0] == '.');
	int isdotdot = (dp->d_namlen == 2) && (dp->d_name[0] == '.') && (dp->d_name[1] == '.');
	int should_update_fileid = nvattrp->nva_flags & NFS_FFLAG_FILEID_CONTAINS_XID;
	uint64_t xid = 0;

	if (isdot || isdotdot) {
		return;
	}

	np = NULL;
	bzero(&cn, sizeof(cn));
	cn.cn_nameptr = dp->d_name;
	cn.cn_namelen = dp->d_namlen;
	cn.cn_nameiop = LOOKUP;

	                                                               
	if (should_update_fileid) {
		xid = nvattrp->nva_fileid;
		nvattrp->nva_fileid = dp->d_fileno;
	}
	nfs_nget(NFSTOMP(dnp), dnp, &cn, fhp->fh_data, fhp->fh_len, nvattrp, savedxidp, RPCAUTH_UNKNOWN, NG_NOCREATE, &np);
	if (should_update_fileid) {
		nvattrp->nva_fileid = xid;
	}
	if (np) {
		nfs_node_unlock(np);
		vnode_put(NFSTOV(np));
	}
}

  
                                                  
                                                                      
                                                                                     
   
int
nfs_node_access_slot(nfsnode_t np, uid_t uid, int add)
{
	int slot;

	for (slot = 0; slot < NFS_ACCESS_CACHE_SIZE; slot++) {
		if (np->n_accessuid[slot] == uid) {
			break;
		}
	}
	if (slot == NFS_ACCESS_CACHE_SIZE) {
		if (!add) {
			return -1;
		}
		slot = np->n_access[NFS_ACCESS_CACHE_SIZE];
		np->n_access[NFS_ACCESS_CACHE_SIZE] = (slot + 1) % NFS_ACCESS_CACHE_SIZE;
	}
	return slot;
}

int
nfs3_access_rpc(nfsnode_t np, u_int32_t *access, int rpcflags, vfs_context_t ctx)
{
	int error = 0, lockerror = ENOENT, status = 0, slot;
	uint32_t access_result = 0;
	u_int64_t xid;
	struct nfsm_chain nmreq, nmrep;
	struct nfsmount *nmp;
	struct timeval now;
	uid_t uid;

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq, NFSX_FH(NFS_VER3) + NFSX_UNSIGNED);
	nfsm_chain_add_fh(error, &nmreq, NFS_VER3, np->n_fhp, np->n_fhsize);
	nfsm_chain_add_32(error, &nmreq, *access);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);
	error = nfs_request2(np, NULL, &nmreq, NFSPROC_ACCESS,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx),
	    NULL, rpcflags, &nmrep, &xid, &status);
	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_postop_attr_update(error, &nmrep, np, &xid);
	if (!error) {
		error = status;
	}
	nfsm_chain_get_32(error, &nmrep, access_result);
	nfsmout_if(error);

	                                                                                             
	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		error = ENXIO;
	}
	nfsmout_if(error);

#if CONFIG_NFS_GSS
	if (auth_is_kerberized(np->n_auth) || auth_is_kerberized(nmp->nm_auth)) {
		uid = nfs_cred_getasid2uid(vfs_context_ucred(ctx));
	} else {
		uid = kauth_cred_getuid(vfs_context_ucred(ctx));
	}
#else
	uid = kauth_cred_getuid(vfs_context_ucred(ctx));
#endif                     
	slot = nfs_node_access_slot(np, uid, 1);
	np->n_accessuid[slot] = uid;
	microuptime(&now);
	np->n_accessstamp[slot] = now.tv_sec;
	np->n_access[slot] = access_result;

	  
                                                        
                                                       
                                                      
                                                        
                           
    
	if ((*access & NFS_ACCESS_DELETE) &&
	    !(np->n_access[slot] & NFS_ACCESS_DELETE)) {
		np->n_access[slot] |= NFS_ACCESS_DELETE;
	}
	                                                                               
	if (nfs_access_dotzfs && (np->n_flag & NISDOTZFSCHILD)) {
		np->n_access[slot] |= (NFS_ACCESS_MODIFY | NFS_ACCESS_EXTEND | NFS_ACCESS_DELETE);
	}
	                                                     
	*access = np->n_access[slot];
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}


  
                       
                                                                   
                                                                         
                                                                          
   
int
nfs_vnop_access(
	struct vnop_access_args     
                                                                
                                                 
                                                 
                                                            
                                        *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	int error = 0, slot, dorpc, rpcflags = 0;
	u_int32_t access, waccess;
	nfsnode_t np = VTONFS(vp);
	struct nfsmount *nmp;
	int nfsvers;
	struct timeval now;
	uid_t uid;

	nmp = VTONMP(vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;


	if (nfsvers == NFS_VER2 || NMFLAG(nmp, NOOPAQUE_AUTH)) {
		if ((ap->a_action & KAUTH_VNODE_WRITE_RIGHTS) &&
		    vfs_isrdonly(vnode_mount(vp))) {
			return EROFS;
		}
		return 0;
	}

	  
                                                                   
                                                                  
                                                            
                                                                  
                                                                   
                 
    

	  
                                                  
    
	access = 0;
	if (vnode_isdir(vp)) {
		               
		if (ap->a_action &
		    (KAUTH_VNODE_LIST_DIRECTORY |
		    KAUTH_VNODE_READ_EXTATTRIBUTES)) {
			access |= NFS_ACCESS_READ;
		}
		if (ap->a_action & KAUTH_VNODE_SEARCH) {
			access |= NFS_ACCESS_LOOKUP;
		}
		if (ap->a_action &
		    (KAUTH_VNODE_ADD_FILE |
		    KAUTH_VNODE_ADD_SUBDIRECTORY)) {
			access |= NFS_ACCESS_MODIFY | NFS_ACCESS_EXTEND;
		}
		if (ap->a_action & KAUTH_VNODE_DELETE_CHILD) {
			access |= NFS_ACCESS_MODIFY;
		}
	} else {
		          
		if (ap->a_action &
		    (KAUTH_VNODE_READ_DATA |
		    KAUTH_VNODE_READ_EXTATTRIBUTES)) {
			access |= NFS_ACCESS_READ;
		}
		if (ap->a_action & KAUTH_VNODE_WRITE_DATA) {
			access |= NFS_ACCESS_MODIFY | NFS_ACCESS_EXTEND;
		}
		if (ap->a_action & KAUTH_VNODE_APPEND_DATA) {
			access |= NFS_ACCESS_EXTEND;
		}
		if (ap->a_action & KAUTH_VNODE_EXECUTE) {
			access |= NFS_ACCESS_EXECUTE;
		}
	}
	            
	if (ap->a_action & KAUTH_VNODE_DELETE) {
		access |= NFS_ACCESS_DELETE;
	}
	if (ap->a_action &
	    (KAUTH_VNODE_WRITE_ATTRIBUTES |
	    KAUTH_VNODE_WRITE_EXTATTRIBUTES |
	    KAUTH_VNODE_WRITE_SECURITY)) {
		access |= NFS_ACCESS_MODIFY;
	}
	                                
	if (ap->a_action & KAUTH_VNODE_CHANGE_OWNER) {
		access |= NFS_ACCESS_MODIFY;
	}

	                                            
	if (nfs_access_cache_timeout > 0) {
		waccess = NFS_ACCESS_READ | NFS_ACCESS_MODIFY |
		    NFS_ACCESS_EXTEND | NFS_ACCESS_EXECUTE |
		    NFS_ACCESS_DELETE | NFS_ACCESS_LOOKUP;
	} else {
		waccess = access;
	}

	if ((error = nfs_node_lock(np))) {
		return error;
	}

	  
                                                             
                 
    
#if CONFIG_NFS_GSS
	if (auth_is_kerberized(np->n_auth) || auth_is_kerberized(nmp->nm_auth)) {
		uid = nfs_cred_getasid2uid(vfs_context_ucred(ctx));
	} else {
		uid = kauth_cred_getuid(vfs_context_ucred(ctx));
	}
#else
	uid = kauth_cred_getuid(vfs_context_ucred(ctx));
#endif                     
	slot = nfs_node_access_slot(np, uid, 0);
	dorpc = 1;
	if (access == 0) {
		                                                                               
		                                                 
		dorpc = 0;
		waccess = 0;
	} else if (NACCESSVALID(np, slot)) {
		microuptime(&now);
		if (((now.tv_sec < (np->n_accessstamp[slot] + nfs_access_cache_timeout)) &&
		    ((np->n_access[slot] & access) == access)) || nfs_use_cache(nmp)) {
			                                                 
			dorpc = 0;
			waccess = np->n_access[slot];
		}
	}
	nfs_node_unlock(np);
	if (dorpc) {
		                                                    
		                                                   

		  
                                                         
                                                     
     
		if (NACCESSVALID(np, slot)) {
			rpcflags |= R_SOFT;
		}

		error = nmp->nm_funcs->nf_access_rpc(np, &waccess, rpcflags, ctx);

		  
                                                           
     
		if ((error == ETIMEDOUT) && (rpcflags & R_SOFT)) {
			error = 0;
			waccess = np->n_access[slot];
		}
	}
	if (!error && ((waccess & access) != access)) {
		error = EACCES;
	}

	return error;
}


  
                    
  
                                                              
                                                                    
                                                                     
   
int
nfs_vnop_open(
	struct vnop_open_args     
                                                              
                                               
                                             
                                                          
                                      *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	nfsnode_t np = VTONFS(vp);
	struct nfsmount *nmp;
	int error, accessMode, denyMode, opened = 0;
	struct nfs_open_owner *noop = NULL;
	struct nfs_open_file *nofp = NULL;
	enum vtype vtype;

	if (!(ap->a_mode & (FREAD | FWRITE))) {
		return EINVAL;
	}

	nmp = VTONMP(vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (np->n_flag & NREVOKE) {
		return EIO;
	}

	vtype = vnode_vtype(vp);
	if ((vtype != VREG) && (vtype != VDIR) && (vtype != VLNK)) {
		return EACCES;
	}

	                                                  
	if (ISSET(np->n_flag, NUPDATESIZE)) {
		nfs_data_update_size(np, 0);
	}
	if ((error = nfs_node_lock(np))) {
		return error;
	}
	if (np->n_flag & NNEEDINVALIDATE) {
		np->n_flag &= ~NNEEDINVALIDATE;
		if (vtype == VDIR) {
			nfs_invaldir(np);
		}
		nfs_node_unlock(np);
		nfs_vinvalbuf(vp, V_SAVE | V_IGNORE_WRITEERR, ctx, 1);
		if ((error = nfs_node_lock(np))) {
			return error;
		}
	}
	if (vtype == VREG) {
		np->n_lastrahead = -1;
	}
	if (np->n_flag & NMODIFIED) {
		if (vtype == VDIR) {
			nfs_invaldir(np);
		}
		nfs_node_unlock(np);
		if ((error = nfs_vinvalbuf(vp, V_SAVE | V_IGNORE_WRITEERR, ctx, 1))) {
			return error;
		}
	} else {
		nfs_node_unlock(np);
	}

	                                                       
	if ((error = nfs_getattr(np, NULL, ctx, NGA_UNCACHED))) {
		return error;
	}

	if (vtype != VREG) {
		                                  
		lck_mtx_lock(&np->n_openlock);
		np->n_openrefcnt++;
		lck_mtx_unlock(&np->n_openlock);
		return 0;
	}

	                                                                          
	accessMode = 0;
	if (ap->a_mode & FREAD) {
		accessMode |= NFS_OPEN_SHARE_ACCESS_READ;
	}
	if (ap->a_mode & FWRITE) {
		accessMode |= NFS_OPEN_SHARE_ACCESS_WRITE;
	}
	if (ap->a_mode & O_EXLOCK) {
		denyMode = NFS_OPEN_SHARE_DENY_BOTH;
	} else if (ap->a_mode & O_SHLOCK) {
		denyMode = NFS_OPEN_SHARE_DENY_WRITE;
	} else {
		denyMode = NFS_OPEN_SHARE_DENY_NONE;
	}
	                                                             
	denyMode = NFS_OPEN_SHARE_DENY_NONE;

	noop = nfs_open_owner_find(nmp, vfs_context_ucred(ctx), 1);
	if (!noop) {
		return ENOMEM;
	}

restart:
	error = nfs_mount_state_in_use_start(nmp, vfs_context_thread(ctx));
	if (error) {
		nfs_open_owner_rele(noop);
		return error;
	}
	if (np->n_flag & NREVOKE) {
		error = EIO;
		nfs_mount_state_in_use_end(nmp, 0);
		nfs_open_owner_rele(noop);
		return error;
	}

	error = nfs_open_file_find(np, noop, &nofp, accessMode, denyMode, 1);
	if (!error && (nofp->nof_flags & NFS_OPEN_FILE_LOST)) {
		NP(np, "nfs_vnop_open: LOST %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
		error = EIO;
	}
#if CONFIG_NFS4
	if (!error && (nofp->nof_flags & NFS_OPEN_FILE_REOPEN)) {
		error = nfs4_reopen(nofp, vfs_context_thread(ctx));
		nofp = NULL;
		if (!error) {
			nfs_mount_state_in_use_end(nmp, 0);
			goto restart;
		}
	}
#endif
	if (!error) {
		error = nfs_open_file_set_busy(nofp, vfs_context_thread(ctx));
	}
	if (error) {
		nofp = NULL;
		goto out;
	}

	if (nmp->nm_vers < NFS_VER4) {
		  
                                                         
     
		nfs_open_file_add_open(nofp, accessMode, denyMode, 0);
		goto out;
	}

	  
                                                                       
                                                                   
    
	if ((nofp->nof_flags & NFS_OPEN_FILE_CREATE) &&
	    (nofp->nof_creator == current_thread()) &&
	    (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) &&
	    (denyMode == NFS_OPEN_SHARE_DENY_NONE)) {
		nofp->nof_flags &= ~NFS_OPEN_FILE_CREATE;
		nofp->nof_creator = NULL;
	} else {
#if CONFIG_NFS4
		if (!opened) {
			error = nfs4_open(np, nofp, accessMode, denyMode, ctx);
		}
#endif
		if ((error == EACCES) && (nofp->nof_flags & NFS_OPEN_FILE_CREATE) &&
		    (nofp->nof_creator == current_thread())) {
			  
                                                                      
                                                                     
                                                                     
                                                               
                                                          
                                                                     
                                                                         
      
			if (denyMode != NFS_OPEN_SHARE_DENY_NONE) {
				NP(np, "nfs_vnop_open: deny mode foregone on create, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
			}
			nofp->nof_creator = NULL;
			error = 0;
		}
		if (error) {
			goto out;
		}
		opened = 1;
		  
                                                             
                                                            
                                                     
     
		if ((nofp->nof_flags & NFS_OPEN_FILE_CREATE) &&
		    (nofp->nof_creator == current_thread())) {
			error = nfs_close(np, nofp, NFS_OPEN_SHARE_ACCESS_BOTH, NFS_OPEN_SHARE_DENY_NONE, ctx);
			if (error) {
				NP(np, "nfs_vnop_open: create close error %d, %d", error, kauth_cred_getuid(nofp->nof_owner->noo_cred));
			}
			if (!nfs_mount_state_error_should_restart(error)) {
				error = 0;
				nofp->nof_flags &= ~NFS_OPEN_FILE_CREATE;
			}
		}
	}

out:
	if (nofp) {
		nfs_open_file_clear_busy(nofp);
	}
	if (nfs_mount_state_in_use_end(nmp, error)) {
		nofp = NULL;
		goto restart;
	}
	if (error) {
		NP(np, "nfs_vnop_open: error %d, %d", error, kauth_cred_getuid(noop->noo_cred));
	}
	if (noop) {
		nfs_open_owner_rele(noop);
	}
	if (!error && vtype == VREG && (ap->a_mode & FWRITE)) {
		lck_mtx_lock(&nmp->nm_lock);
		nmp->nm_state &= ~NFSSTA_SQUISHY;
		nmp->nm_curdeadtimeout = nmp->nm_deadtimeout;
		if (nmp->nm_curdeadtimeout <= 0) {
			nmp->nm_deadto_start = 0;
		}
		nmp->nm_writers++;
		lck_mtx_unlock(&nmp->nm_lock);
	}

	return error;
}

static uint32_t
nfs_no_of_open_file_writers(nfsnode_t np)
{
	uint32_t writers = 0;
	struct nfs_open_file *nofp;

	TAILQ_FOREACH(nofp, &np->n_opens, nof_link) {
		writers += nofp->nof_w + nofp->nof_rw + nofp->nof_w_dw + nofp->nof_rw_dw +
		    nofp->nof_w_drw + nofp->nof_rw_drw + nofp->nof_d_w_dw +
		    nofp->nof_d_rw_dw + nofp->nof_d_w_drw + nofp->nof_d_rw_drw +
		    nofp->nof_d_w + nofp->nof_d_rw;
	}

	return writers;
}

  
                     
  
                                                                              
                                                                               
               
                                                                          
                                                                         
                                                                          
                                                                     
                                     
                                                                           
                                                                           
                                                                           
                                            
                                                                        
                                                         
  
                                       
                                                                          
                                                                                   
                                                  
   
int
nfs_vnop_close(
	struct vnop_close_args     
                                                               
                                                
                                               
                                                           
                                       *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	nfsnode_t np = VTONFS(vp);
	struct nfsmount *nmp;
	int error = 0, error1, nfsvers;
	int fflag = ap->a_fflag;
	enum vtype vtype;
	int accessMode, denyMode;
	struct nfs_open_owner *noop = NULL;
	struct nfs_open_file *nofp = NULL;

	nmp = VTONMP(vp);
	if (!nmp) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	vtype = vnode_vtype(vp);

	                                                        
	if (ISSET(np->n_flag, NUPDATESIZE)) {
		nfs_data_update_size(np, 0);
	}
	nfs_node_lock_force(np);
	if (np->n_flag & NNEEDINVALIDATE) {
		np->n_flag &= ~NNEEDINVALIDATE;
		nfs_node_unlock(np);
		nfs_vinvalbuf(vp, V_SAVE | V_IGNORE_WRITEERR, ctx, 1);
		nfs_node_lock_force(np);
	}
	if ((vtype == VREG) && (np->n_flag & NMODIFIED) && (fflag & FWRITE)) {
		                                                                           
		nfs_node_unlock(np);
		if (nfsvers != NFS_VER2) {
			error = nfs_flush(np, MNT_WAIT, vfs_context_thread(ctx), 0);
		} else {
			error = nfs_vinvalbuf(vp, V_SAVE, ctx, 1);
		}
		nfs_node_lock_force(np);
		NATTRINVALIDATE(np);
	}
	if (np->n_flag & NWRITEERR) {
		np->n_flag &= ~NWRITEERR;
		error = np->n_error;
	}
	nfs_node_unlock(np);

	if (vtype != VREG) {
		                                  
		lck_mtx_lock(&np->n_openlock);
		if (np->n_openrefcnt == 0) {
			if (fflag & (FREAD | FWRITE)) {
				NP(np, "nfs_vnop_close: open reference underrun");
				error = EINVAL;
			}
		} else if (fflag & (FREAD | FWRITE)) {
			np->n_openrefcnt--;
		} else {
			                                                    
			np->n_openrefcnt = 0;
		}
		lck_mtx_unlock(&np->n_openlock);
		return error;
	}
	error1 = error;

	                                                             
	accessMode = 0;
	if (fflag & FREAD) {
		accessMode |= NFS_OPEN_SHARE_ACCESS_READ;
	}
	if (fflag & FWRITE) {
		accessMode |= NFS_OPEN_SHARE_ACCESS_WRITE;
	}
                                                                                         
                        
                                        
                             
                                         
       
                                        
	                                                             
	denyMode = NFS_OPEN_SHARE_DENY_NONE;

	if (!accessMode) {
		  
                            
                                   
                                                    
     
		uint32_t writers;
		mount_t mp = vnode_mount(vp);
		int force = (!mp || vfs_isforce(mp));

		writers = nfs_no_of_open_file_writers(np);
		nfs_release_open_state_for_node(np, force);
		if (writers) {
			lck_mtx_lock(&nmp->nm_lock);
			if (writers > nmp->nm_writers) {
				NP(np, "nfs_vnop_close: number of write opens for mount underrun. Node has %d"
				    " opens for write. Mount has total of %d opens for write\n",
				    writers, nmp->nm_writers);
				nmp->nm_writers = 0;
			} else {
				nmp->nm_writers -= writers;
			}
			lck_mtx_unlock(&nmp->nm_lock);
		}

		return error;
	} else if (fflag & FWRITE) {
		lck_mtx_lock(&nmp->nm_lock);
		if (nmp->nm_writers == 0) {
			NP(np, "nfs_vnop_close: removing open writer from mount, but mount has no files open for writing");
		} else {
			nmp->nm_writers--;
		}
		lck_mtx_unlock(&nmp->nm_lock);
	}


	noop = nfs_open_owner_find(nmp, vfs_context_ucred(ctx), 0);
	if (!noop) {
		                                                     
		return EIO;
	}

restart:
	error = nfs_mount_state_in_use_start(nmp, NULL);
	if (error) {
		nfs_open_owner_rele(noop);
		return error;
	}

	error = nfs_open_file_find(np, noop, &nofp, 0, 0, 0);
#if CONFIG_NFS4
	if (!error && (nofp->nof_flags & NFS_OPEN_FILE_REOPEN)) {
		error = nfs4_reopen(nofp, NULL);
		nofp = NULL;
		if (!error) {
			nfs_mount_state_in_use_end(nmp, 0);
			goto restart;
		}
	}
#endif
	if (error) {
		NP(np, "nfs_vnop_close: no open file for owner, error %d, %d", error, kauth_cred_getuid(noop->noo_cred));
		error = EBADF;
		goto out;
	}
	error = nfs_open_file_set_busy(nofp, NULL);
	if (error) {
		nofp = NULL;
		goto out;
	}

	error = nfs_close(np, nofp, accessMode, denyMode, ctx);
	if (error) {
		NP(np, "nfs_vnop_close: close error %d, %d", error, kauth_cred_getuid(noop->noo_cred));
	}

out:
	if (nofp) {
		nfs_open_file_clear_busy(nofp);
	}
	if (nfs_mount_state_in_use_end(nmp, error)) {
		nofp = NULL;
		goto restart;
	}
	if (!error) {
		error = error1;
	}
	if (error) {
		NP(np, "nfs_vnop_close: error %d, %d", error, kauth_cred_getuid(noop->noo_cred));
	}
	if (noop) {
		nfs_open_owner_rele(noop);
	}
	return error;
}

  
                                                                               
  
                                                                                  
                                                                                 
   
int
nfs_close(
	nfsnode_t np,
	struct nfs_open_file *nofp,
	uint32_t accessMode,
	uint32_t denyMode,
	__unused vfs_context_t ctx)
{
#if CONFIG_NFS4
	struct nfs_lock_owner *nlop;
#endif
	int error = 0, changed = 0, delegated = 0, closed = 0, downgrade = 0;
	uint8_t newAccessMode, newDenyMode;

	                                             
	if (((accessMode & nofp->nof_access) != accessMode) || ((denyMode & nofp->nof_deny) != denyMode)) {
		NP(np, "nfs_close: mode mismatch %d %d, current %d %d, %d",
		    accessMode, denyMode, nofp->nof_access, nofp->nof_deny,
		    kauth_cred_getuid(nofp->nof_owner->noo_cred));
	}

	  
                                                                          
                                                                    
    
	if (denyMode == NFS_OPEN_SHARE_DENY_NONE) {
		if ((accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) &&
		    (nofp->nof_w == 0) && (nofp->nof_d_w == 0) &&
		    (nofp->nof_rw || nofp->nof_d_rw)) {
			accessMode = NFS_OPEN_SHARE_ACCESS_BOTH;
		}
	} else if (denyMode == NFS_OPEN_SHARE_DENY_WRITE) {
		if ((accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) &&
		    (nofp->nof_w_dw == 0) && (nofp->nof_d_w_dw == 0) &&
		    (nofp->nof_rw_dw || nofp->nof_d_rw_dw)) {
			accessMode = NFS_OPEN_SHARE_ACCESS_BOTH;
		}
	} else {                               
		if ((accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) &&
		    (nofp->nof_w_drw == 0) && (nofp->nof_d_w_drw == 0) &&
		    (nofp->nof_rw_drw || nofp->nof_d_rw_drw)) {
			accessMode = NFS_OPEN_SHARE_ACCESS_BOTH;
		}
	}

	nfs_open_file_remove_open_find(nofp, accessMode, denyMode, &newAccessMode, &newDenyMode, &delegated);
	if ((newAccessMode != nofp->nof_access) || (newDenyMode != nofp->nof_deny)) {
		changed = 1;
	} else {
		changed = 0;
	}

	if (NFSTONMP(np)->nm_vers < NFS_VER4) {
		                                                      
		goto v3close;
	}
#if CONFIG_NFS4
	if ((newAccessMode == 0) || (nofp->nof_opencnt == 1)) {
		  
                                                               
                                                              
     
		nfs_wait_bufs(np);
		closed = 1;
		if (!delegated && !(nofp->nof_flags & NFS_OPEN_FILE_LOST)) {
			error = nfs4_close_rpc(np, nofp, vfs_context_thread(ctx), vfs_context_ucred(ctx), 0);
		}
		if (error == NFSERR_LOCKS_HELD) {
			  
                                                                   
                                                       
      
			nlop = nfs_lock_owner_find(np, vfs_context_proc(ctx), 0);
			if (nlop) {
				nfs4_unlock_rpc(np, nlop, F_WRLCK, 0, UINT64_MAX,
				    0, vfs_context_thread(ctx), vfs_context_ucred(ctx));
				nfs_lock_owner_rele(nlop);
			}
			error = nfs4_close_rpc(np, nofp, vfs_context_thread(ctx), vfs_context_ucred(ctx), 0);
		}
	} else if (changed) {
		  
                                                                    
                                                                  
     
		if (!delegated && !(nofp->nof_flags & NFS_OPEN_FILE_LOST)) {
			downgrade = 1;
			  
                                                                              
                                                                                   
      
			if (nofp->nof_d_rw_drw || nofp->nof_d_w_drw || nofp->nof_d_r_drw ||
			    nofp->nof_d_rw_dw || nofp->nof_d_w_dw || nofp->nof_d_r_dw ||
			    nofp->nof_d_rw || nofp->nof_d_w || nofp->nof_d_r) {
				nfs4_claim_delegated_state_for_open_file(nofp, 0);
			}
			                                                          
			nfs_open_file_remove_open(nofp, accessMode, denyMode);
			error = nfs4_open_downgrade_rpc(np, nofp, ctx);
			if (error) {                                                    
				nfs_open_file_add_open(nofp, accessMode, denyMode, delegated);
			}
		}
	}
#endif
v3close:
	if (error) {
		NP(np, "nfs_close: error %d, %d", error, kauth_cred_getuid(nofp->nof_owner->noo_cred));
		return error;
	}

	if (!downgrade) {
		nfs_open_file_remove_open(nofp, accessMode, denyMode);
	}

	if (closed) {
		lck_mtx_lock(&nofp->nof_lock);
		if (nofp->nof_r || nofp->nof_d_r || nofp->nof_w || nofp->nof_d_w || nofp->nof_d_rw ||
		    (nofp->nof_rw && !((nofp->nof_flags & NFS_OPEN_FILE_CREATE) && !nofp->nof_creator && (nofp->nof_rw == 1))) ||
		    nofp->nof_r_dw || nofp->nof_d_r_dw || nofp->nof_w_dw || nofp->nof_d_w_dw ||
		    nofp->nof_rw_dw || nofp->nof_d_rw_dw || nofp->nof_r_drw || nofp->nof_d_r_drw ||
		    nofp->nof_w_drw || nofp->nof_d_w_drw || nofp->nof_rw_drw || nofp->nof_d_rw_drw) {
			NP(np, "nfs_close: unexpected count: %u.%u %u.%u %u.%u dw %u.%u %u.%u %u.%u drw %u.%u %u.%u %u.%u flags 0x%x, %d",
			    nofp->nof_r, nofp->nof_d_r, nofp->nof_w, nofp->nof_d_w,
			    nofp->nof_rw, nofp->nof_d_rw, nofp->nof_r_dw, nofp->nof_d_r_dw,
			    nofp->nof_w_dw, nofp->nof_d_w_dw, nofp->nof_rw_dw, nofp->nof_d_rw_dw,
			    nofp->nof_r_drw, nofp->nof_d_r_drw, nofp->nof_w_drw, nofp->nof_d_w_drw,
			    nofp->nof_rw_drw, nofp->nof_d_rw_drw, nofp->nof_flags,
			    kauth_cred_getuid(nofp->nof_owner->noo_cred));
		}
		                                              
		nofp->nof_access = nofp->nof_deny = 0;
		nofp->nof_mmap_access = nofp->nof_mmap_deny = 0;
		nofp->nof_r = nofp->nof_d_r = 0;
		nofp->nof_w = nofp->nof_d_w = 0;
		nofp->nof_rw = nofp->nof_d_rw = 0;
		nofp->nof_r_dw = nofp->nof_d_r_dw = 0;
		nofp->nof_w_dw = nofp->nof_d_w_dw = 0;
		nofp->nof_rw_dw = nofp->nof_d_rw_dw = 0;
		nofp->nof_r_drw = nofp->nof_d_r_drw = 0;
		nofp->nof_w_drw = nofp->nof_d_w_drw = 0;
		nofp->nof_rw_drw = nofp->nof_d_rw_drw = 0;
		nofp->nof_flags &= ~NFS_OPEN_FILE_CREATE;
		lck_mtx_unlock(&nofp->nof_lock);
		                                                                              
	}
	if (nofp->nof_flags & NFS_OPEN_FILE_LOST) {
		error = EIO;
		NP(np, "nfs_close: LOST%s, %d", !nofp->nof_opencnt ? " (last)" : "",
		    kauth_cred_getuid(nofp->nof_owner->noo_cred));
	}

	return error;
}


int
nfs3_getattr_rpc(
	nfsnode_t np,
	mount_t mp,
	u_char *fhp,
	size_t fhsize,
	int flags,
	vfs_context_t ctx,
	struct nfs_vattr *nvap,
	u_int64_t *xidp)
{
	struct nfsmount *nmp = mp ? VFSTONFS(mp) : NFSTONMP(np);
	int error = 0, status = 0, nfsvers, rpcflags = 0;
	struct nfsm_chain nmreq, nmrep;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	if (flags & NGA_MONITOR) {                                            
		rpcflags = R_RECOVER;
	}

	if (flags & NGA_SOFT) {                                                
		rpcflags |= R_SOFT;
	}

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq, NFSX_FH(nfsvers));
	if (nfsvers != NFS_VER2) {
		nfsm_chain_add_32(error, &nmreq, fhsize);
	}
	nfsm_chain_add_opaque(error, &nmreq, fhp, fhsize);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);
	error = nfs_request2(np, mp, &nmreq, NFSPROC_GETATTR,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx),
	    NULL, rpcflags, &nmrep, xidp, &status);
	if (!error) {
		error = status;
	}
	nfsmout_if(error);
	error = nfs_parsefattr(nmp, &nmrep, nfsvers, nvap);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

  
                                                                      
  
                                                                                        
  
                                                                                   
   
int
nfs_refresh_fh(nfsnode_t np, vfs_context_t ctx)
{
	vnode_t dvp, vp = NFSTOV(np);
	nfsnode_t dnp;
	const char *v_name = vnode_getname(vp);
	char *name;
	int namelen, refreshed;
	uint32_t fhsize;
	int error, wanted = 0;
	uint8_t *fhp;
	struct timespec ts = {.tv_sec = 2, .tv_nsec = 0};

	NFS_VNOP_DBG("vnode is %d\n", vnode_vtype(vp));

	dvp = vnode_parent(vp);
	if ((vnode_vtype(vp) != VREG && vnode_vtype(vp) != VLNK) ||
	    v_name == NULL || *v_name == '\0' || dvp == NULL) {
		if (v_name != NULL) {
			vnode_putname(v_name);
		}
		return ESTALE;
	}
	dnp = VTONFS(dvp);

	namelen = NFS_STRLEN_INT(v_name);
	MALLOC(name, char *, namelen + 1, M_TEMP, M_WAITOK);
	if (name == NULL) {
		vnode_putname(v_name);
		return ESTALE;
	}
	bcopy(v_name, name, namelen + 1);
	NFS_VNOP_DBG("Trying to refresh %s : %s\n", v_name, name);
	vnode_putname(v_name);

	                                           
	MALLOC(fhp, uint8_t *, NFS4_FHSIZE, M_FHANDLE, M_WAITOK);
	if (fhp == NULL) {
		FREE(name, M_TEMP);
		return ESTALE;
	}

	if ((error = nfs_node_lock(np))) {
		FREE(name, M_TEMP);
		FREE(fhp, M_FHANDLE);
		return ESTALE;
	}

	fhsize = np->n_fhsize;
	bcopy(np->n_fhp, fhp, fhsize);
	while (ISSET(np->n_flag, NREFRESH)) {
		SET(np->n_flag, NREFRESHWANT);
		NFS_VNOP_DBG("Waiting for refresh of %s\n", name);
		msleep(np, &np->n_lock, PZERO - 1, "nfsrefreshwant", &ts);
		if ((error = nfs_sigintr(NFSTONMP(np), NULL, vfs_context_thread(ctx), 0))) {
			break;
		}
	}
	refreshed = error ? 0 : !NFS_CMPFH(np, fhp, fhsize);
	SET(np->n_flag, NREFRESH);
	nfs_node_unlock(np);

	NFS_VNOP_DBG("error = %d, refreshed = %d\n", error, refreshed);
	if (error || refreshed) {
		goto nfsmout;
	}

	                                                           
	lck_mtx_lock(&np->n_openlock);
	if (np->n_openrefcnt || !TAILQ_EMPTY(&np->n_opens) || !TAILQ_EMPTY(&np->n_lock_owners)) {
		int cnt = 0;
		struct nfs_open_file *ofp;

		TAILQ_FOREACH(ofp, &np->n_opens, nof_link) {
			cnt += ofp->nof_opencnt;
		}
		if (cnt) {
			lck_mtx_unlock(&np->n_openlock);
			NFS_VNOP_DBG("Can not refresh file handle for %s with open state\n", name);
			NFS_VNOP_DBG("\topenrefcnt = %d, opens = %d lock_owners = %d\n",
			    np->n_openrefcnt, cnt, !TAILQ_EMPTY(&np->n_lock_owners));
			error = ESTALE;
			goto nfsmout;
		}
	}
	lck_mtx_unlock(&np->n_openlock);
	  
                                                            
                                                       
    

	error = nfs_node_lock(np);
	nfsmout_if(error);
	  
                                                            
                                                         
                                                             
                                                        
               
    
	if (vnode_vtype(vp) == VREG) {
		np->n_flag &= ~NNEEDINVALIDATE;
		nfs_node_unlock(np);
		error = nfs_vinvalbuf(vp, V_IGNORE_WRITEERR, ctx, 1);
		if (error) {
			NFS_VNOP_DBG("nfs_vinvalbuf returned %d\n", error);
		}
		nfsmout_if(error);
	} else {
		nfs_node_unlock(np);
	}

	NFS_VNOP_DBG("Looking up %s\n", name);
	error = nfs_lookitup(dnp, name, namelen, ctx, &np);
	if (error) {
		NFS_VNOP_DBG("nfs_lookitup returned %d\n", error);
	}

nfsmout:
	nfs_node_lock_force(np);
	wanted = ISSET(np->n_flag, NREFRESHWANT);
	CLR(np->n_flag, NREFRESH | NREFRESHWANT);
	nfs_node_unlock(np);
	if (wanted) {
		wakeup(np);
	}

	if (error == 0) {
		NFS_VNOP_DBG("%s refreshed file handle\n", name);
	}

	FREE(name, M_TEMP);
	FREE(fhp, M_FHANDLE);

	return error ? ESTALE : 0;
}

int
nfs_getattr(nfsnode_t np, struct nfs_vattr *nvap, vfs_context_t ctx, int flags)
{
	int error;

retry:
	error = nfs_getattr_internal(np, nvap, ctx, flags);
	if (error == ESTALE) {
		error = nfs_refresh_fh(np, ctx);
		if (!error) {
			goto retry;
		}
	}
	return error;
}

int
nfs_getattr_internal(nfsnode_t np, struct nfs_vattr *nvap, vfs_context_t ctx, int flags)
{
	struct nfsmount *nmp;
	int error = 0, nfsvers, inprogset = 0, wanted = 0, avoidfloods = 0;
	struct nfs_vattr *nvattr = NULL;
	struct timespec ts = { .tv_sec = 2, .tv_nsec = 0 };
	u_int64_t xid = 0;

	FSDBG_TOP(513, np->n_size, np, np->n_vattr.nva_size, np->n_flag);

	nmp = NFSTONMP(np);

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	if (!nvap) {
		MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
		nvap = nvattr;
	}
	NVATTR_INIT(nvap);

	                                           
	if (np->n_flag & (NACC | NUPD)) {
		nfs_node_lock_force(np);
		np->n_flag |= NCHG;
		nfs_node_unlock(np);
	}
	                               
	if (ISSET(np->n_flag, NUPDATESIZE)) {
		nfs_data_update_size(np, 0);
	}

	error = nfs_node_lock(np);
	nfsmout_if(error);
	if (!(flags & (NGA_UNCACHED | NGA_MONITOR)) || ((nfsvers >= NFS_VER4) && (np->n_openflags & N_DELEG_MASK))) {
		  
                                                          
                                
                               
                                  
     
		while (1) {
			error = nfs_getattrcache(np, nvap, flags);
			if (!error || (error != ENOENT)) {
				nfs_node_unlock(np);
				goto nfsmout;
			}
			error = 0;
			if (!ISSET(np->n_flag, NGETATTRINPROG)) {
				break;
			}
			if (flags & NGA_MONITOR) {
				                                             
				error = EINPROGRESS;
				nfs_node_unlock(np);
				goto nfsmout;
			}
			SET(np->n_flag, NGETATTRWANT);
			msleep(np, &np->n_lock, PZERO - 1, "nfsgetattrwant", &ts);
			if ((error = nfs_sigintr(NFSTONMP(np), NULL, vfs_context_thread(ctx), 0))) {
				nfs_node_unlock(np);
				goto nfsmout;
			}
		}
		SET(np->n_flag, NGETATTRINPROG);
		inprogset = 1;
	} else if (!ISSET(np->n_flag, NGETATTRINPROG)) {
		SET(np->n_flag, NGETATTRINPROG);
		inprogset = 1;
	} else if (flags & NGA_MONITOR) {
		                                                 
		error = EINPROGRESS;
	}
	nfs_node_unlock(np);

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		error = ENXIO;
	}
	if (error) {
		goto nfsmout;
	}

	  
                                               
                                              
                                    
    
	if (NATTRVALID(np) && nfs_use_cache(nmp)) {
		flags |= NGA_SOFT;
	}

	  
                                                                      
                                                                      
                                                               
                                           
    
	if (!(flags & NGA_ACL) && (nfsvers != NFS_VER2) && nfs_access_for_getattr && (nfs_access_cache_timeout > 0)) {
		if (nfs_attrcachetimeout(np) > 0) {
			                                                    
			u_int32_t access = NFS_ACCESS_ALL;
			int rpcflags = 0;

			                                                   
			if (flags & NGA_SOFT) {
				rpcflags |= R_SOFT;
			}

			error = nmp->nm_funcs->nf_access_rpc(np, &access, rpcflags, ctx);

			if (error == ETIMEDOUT) {
				goto returncached;
			}

			if (error) {
				goto nfsmout;
			}
			nfs_node_lock_force(np);
			error = nfs_getattrcache(np, nvap, flags);
			nfs_node_unlock(np);
			if (!error || (error != ENOENT)) {
				goto nfsmout;
			}
			                                                    
			error = 0;
		}
	}

	avoidfloods = 0;

tryagain:
	error = nmp->nm_funcs->nf_getattr_rpc(np, NULL, np->n_fhp, np->n_fhsize, flags, ctx, nvap, &xid);
	if (!error) {
		nfs_node_lock_force(np);
		error = nfs_loadattrcache(np, nvap, &xid, 0);
		nfs_node_unlock(np);
	}

	  
                                                           
    
returncached:
	if ((flags & NGA_SOFT) && (error == ETIMEDOUT)) {
		nfs_node_lock_force(np);
		error = nfs_getattrcache(np, nvap, flags);
		if (!error || (error != ENOENT)) {
			nfs_node_unlock(np);
			goto nfsmout;
		}
		nfs_node_unlock(np);
	}
	nfsmout_if(error);

	if (!xid) {                                                 
		FSDBG(513, -1, np, np->n_xid >> 32, np->n_xid);
		if (avoidfloods++ < 20) {
			goto tryagain;
		}
		                                                    
		                                            
	}
nfsmout:
	nfs_node_lock_force(np);
	if (inprogset) {
		wanted = ISSET(np->n_flag, NGETATTRWANT);
		CLR(np->n_flag, (NGETATTRINPROG | NGETATTRWANT));
	}
	if (!error) {
		                                     
		vnode_t vp = NFSTOV(np);
		enum vtype vtype = vnode_vtype(vp);
		if ((vtype == VDIR) && NFS_CHANGED_NC(nfsvers, np, nvap)) {
			FSDBG(513, -1, np, 0, np);
			np->n_flag &= ~NNEGNCENTRIES;
			cache_purge(vp);
			np->n_ncgen++;
			NFS_CHANGED_UPDATE_NC(nfsvers, np, nvap);
			NFS_VNOP_DBG("Purge directory 0x%llx\n",
			    (uint64_t)VM_KERNEL_ADDRPERM(vp));
		}
		if (NFS_CHANGED(nfsvers, np, nvap)) {
			FSDBG(513, -1, np, -1, np);
			if (vtype == VDIR) {
				NFS_VNOP_DBG("Invalidate directory 0x%llx\n",
				    (uint64_t)VM_KERNEL_ADDRPERM(vp));
				nfs_invaldir(np);
			}
			nfs_node_unlock(np);
			if (wanted) {
				wakeup(np);
			}
			error = nfs_vinvalbuf(vp, V_SAVE, ctx, 1);
			FSDBG(513, -1, np, -2, error);
			if (!error) {
				nfs_node_lock_force(np);
				NFS_CHANGED_UPDATE(nfsvers, np, nvap);
				nfs_node_unlock(np);
			}
		} else {
			nfs_node_unlock(np);
			if (wanted) {
				wakeup(np);
			}
		}
	} else {
		nfs_node_unlock(np);
		if (wanted) {
			wakeup(np);
		}
	}

	if (nvattr != NULL) {
		NVATTR_CLEANUP(nvap);
		FREE(nvattr, M_TEMP);
	} else if (!(flags & NGA_ACL)) {
		                                                             
		NFS_BITMAP_CLR(nvap->nva_bitmap, NFS_FATTR_ACL);
		if (nvap->nva_acl) {
			kauth_acl_free(nvap->nva_acl);
			nvap->nva_acl = NULL;
		}
	}
	FSDBG_BOT(513, np->n_size, error, np->n_vattr.nva_size, np->n_flag);
	return error;
}


  
                             
   

  
                                           
                                                              
                                                                   
                                                                   
                                               
   
#define NFS3_SUPPORTED_VATTRS \
	(VNODE_ATTR_va_rdev |           \
	 VNODE_ATTR_va_nlink |          \
	 VNODE_ATTR_va_data_size |      \
	 VNODE_ATTR_va_data_alloc |     \
	 VNODE_ATTR_va_uid |            \
	 VNODE_ATTR_va_gid |            \
	 VNODE_ATTR_va_mode |           \
	 VNODE_ATTR_va_modify_time |    \
	 VNODE_ATTR_va_change_time |    \
	 VNODE_ATTR_va_access_time |    \
	 VNODE_ATTR_va_fileid |         \
	 VNODE_ATTR_va_type)


int
nfs3_vnop_getattr(
	struct vnop_getattr_args     
                                                                 
                                                  
                                                              
                                                             
                                         *ap)
{
	int error;
	nfsnode_t np;
	uint64_t supported_attrs;
	struct nfs_vattr *nva;
	struct vnode_attr *vap = ap->a_vap;
	struct nfsmount *nmp;
	dev_t rdev;

	nmp = VTONMP(ap->a_vp);

	  
                                                                          
                                                                        
    
	                                                                                 
	VATTR_RETURN(vap, va_iosize, nfs_iosize);

	supported_attrs = NFS3_SUPPORTED_VATTRS;

	if ((vap->va_active & supported_attrs) == 0) {
		return 0;
	}

	if (VATTR_IS_ACTIVE(ap->a_vap, va_name)) {
		NFS_VNOP_DBG("Getting attrs for 0x%llx, vname is %s\n",
		    (uint64_t)VM_KERNEL_ADDRPERM(ap->a_vp),
		    ap->a_vp->v_name ? ap->a_vp->v_name : "empty");
	}

	  
                                                                                            
    
	if ((vap->va_active & supported_attrs) == VNODE_ATTR_va_fileid) {
		np = VTONFS(ap->a_vp);
		if (np->n_attrstamp) {
			VATTR_RETURN(vap, va_fileid, np->n_vattr.nva_fileid);
			return 0;
		}
	}

	MALLOC(nva, struct nfs_vattr *, sizeof(*nva), M_TEMP, M_WAITOK);
	error = nfs_getattr(VTONFS(ap->a_vp), nva, ap->a_context, NGA_CACHED);
	if (error) {
		goto out;
	}

	                        
	VATTR_RETURN(vap, va_type, nva->nva_type);
	VATTR_RETURN(vap, va_mode, nva->nva_mode);
	rdev = makedev(nva->nva_rawdev.specdata1, nva->nva_rawdev.specdata2);
	VATTR_RETURN(vap, va_rdev, rdev);
	VATTR_RETURN(vap, va_uid, nva->nva_uid);
	VATTR_RETURN(vap, va_gid, nva->nva_gid);
	VATTR_RETURN(vap, va_nlink, nva->nva_nlink);
	VATTR_RETURN(vap, va_fileid, nva->nva_fileid);
	VATTR_RETURN(vap, va_data_size, nva->nva_size);
	VATTR_RETURN(vap, va_data_alloc, nva->nva_bytes);
	vap->va_access_time.tv_sec = nva->nva_timesec[NFSTIME_ACCESS];
	vap->va_access_time.tv_nsec = nva->nva_timensec[NFSTIME_ACCESS];
	VATTR_SET_SUPPORTED(vap, va_access_time);
	vap->va_modify_time.tv_sec = nva->nva_timesec[NFSTIME_MODIFY];
	vap->va_modify_time.tv_nsec = nva->nva_timensec[NFSTIME_MODIFY];
	VATTR_SET_SUPPORTED(vap, va_modify_time);
	vap->va_change_time.tv_sec = nva->nva_timesec[NFSTIME_CHANGE];
	vap->va_change_time.tv_nsec = nva->nva_timensec[NFSTIME_CHANGE];
	VATTR_SET_SUPPORTED(vap, va_change_time);


	                                                                     
out:
	FREE(nva, M_TEMP);
	return error;
}

  
                    
   
int
nfs_vnop_setattr(
	struct vnop_setattr_args     
                                                                 
                                                  
                                                              
                                                             
                                         *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	nfsnode_t np = VTONFS(vp);
	struct nfsmount *nmp;
	struct vnode_attr *vap = ap->a_vap;
	int error = 0;
	int biosize, nfsvers, namedattrs;
	u_quad_t origsize, vapsize;
	struct nfs_dulookup *dul;
	nfsnode_t dnp = NULL;
	int dul_in_progress = 0;
	vnode_t dvp = NULL;
	const char *vname = NULL;
#if CONFIG_NFS4
	struct nfs_open_owner *noop = NULL;
	struct nfs_open_file *nofp = NULL;
#endif
	nmp = VTONMP(vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	namedattrs = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR);
	biosize = nmp->nm_biosize;

	                                                                     
	if (vnode_vfsisrdonly(vp)) {
		return EROFS;
	}

	origsize = np->n_size;
	if (VATTR_IS_ACTIVE(vap, va_data_size)) {
		switch (vnode_vtype(vp)) {
		case VDIR:
			return EISDIR;
		case VCHR:
		case VBLK:
		case VSOCK:
		case VFIFO:
			if (!VATTR_IS_ACTIVE(vap, va_modify_time) &&
			    !VATTR_IS_ACTIVE(vap, va_access_time) &&
			    !VATTR_IS_ACTIVE(vap, va_mode) &&
			    !VATTR_IS_ACTIVE(vap, va_uid) &&
			    !VATTR_IS_ACTIVE(vap, va_gid)) {
				return 0;
			}
			VATTR_CLEAR_ACTIVE(vap, va_data_size);
			break;
		default:
			  
                                                  
                        
      
			if (vnode_vfsisrdonly(vp)) {
				return EROFS;
			}
			FSDBG_TOP(512, np->n_size, vap->va_data_size,
			    np->n_vattr.nva_size, np->n_flag);
			                                   
			if ((error = nfs_node_lock(np))) {
				return error;
			}
			if (np->n_flag & NNEEDINVALIDATE) {
				np->n_flag &= ~NNEEDINVALIDATE;
			}
			nfs_node_unlock(np);
			                      
			error = nfs_vinvalbuf(vp, (vap->va_data_size ? V_SAVE : 0), ctx, 1);
			if (error) {
				NP(np, "nfs_setattr: nfs_vinvalbuf %d", error);
				FSDBG_BOT(512, np->n_size, vap->va_data_size, np->n_vattr.nva_size, -1);
				return error;
			}
#if CONFIG_NFS4
			if (nfsvers >= NFS_VER4) {
				                                                                      
				if (np->n_flag & NREVOKE) {
					return EIO;
				}
				noop = nfs_open_owner_find(nmp, vfs_context_ucred(ctx), 1);
				if (!noop) {
					return ENOMEM;
				}
restart:
				error = nfs_mount_state_in_use_start(nmp, vfs_context_thread(ctx));
				if (error) {
					return error;
				}
				if (np->n_flag & NREVOKE) {
					nfs_mount_state_in_use_end(nmp, 0);
					return EIO;
				}
				error = nfs_open_file_find(np, noop, &nofp, 0, 0, 1);
				if (!error && (nofp->nof_flags & NFS_OPEN_FILE_LOST)) {
					error = EIO;
				}
				if (!error && (nofp->nof_flags & NFS_OPEN_FILE_REOPEN)) {
					error = nfs4_reopen(nofp, vfs_context_thread(ctx));
					nofp = NULL;
					if (!error) {
						nfs_mount_state_in_use_end(nmp, 0);
						goto restart;
					}
				}
				if (!error) {
					error = nfs_open_file_set_busy(nofp, vfs_context_thread(ctx));
				}
				if (error) {
					nfs_mount_state_in_use_end(nmp, 0);
					nfs_open_owner_rele(noop);
					return error;
				}
				if (!(nofp->nof_access & NFS_OPEN_SHARE_ACCESS_WRITE)) {
					                                                              
					error = nfs4_open(np, nofp, NFS_OPEN_SHARE_ACCESS_WRITE, NFS_OPEN_SHARE_DENY_NONE, ctx);
					if (!error) {
						nofp->nof_flags |= NFS_OPEN_FILE_SETATTR;
					}
					if (nfs_mount_state_error_should_restart(error)) {
						nfs_open_file_clear_busy(nofp);
						nofp = NULL;
						nfs_mount_state_in_use_end(nmp, error);
						goto restart;
					}
				}
			}
#endif
			nfs_data_lock(np, NFS_DATA_LOCK_EXCLUSIVE);
			if (np->n_size > vap->va_data_size) {                 
				daddr64_t obn, bn;
				int mustwrite;
				off_t neweofoff;
				struct nfsbuf *bp;
				nfsbufpgs pagemask;

				obn = (np->n_size - 1) / biosize;
				bn = vap->va_data_size / biosize;
				for (; obn >= bn; obn--) {
					if (!nfs_buf_is_incore(np, obn)) {
						continue;
					}
					error = nfs_buf_get(np, obn, biosize, NULL, NBLK_READ, &bp);
					if (error) {
						continue;
					}
					if (obn != bn) {
						FSDBG(512, bp, bp->nb_flags, 0, obn);
						SET(bp->nb_flags, NB_INVAL);
						nfs_buf_release(bp, 1);
						continue;
					}
					mustwrite = 0;
					neweofoff = vap->va_data_size - NBOFF(bp);
					                                                 
					if ((bp->nb_dirtyend > 0) && (bp->nb_dirtyoff < neweofoff)) {
						                             
						if (bp->nb_dirtyend > neweofoff) {
							bp->nb_dirtyend = neweofoff;
							if (bp->nb_dirtyoff >= bp->nb_dirtyend) {
								bp->nb_dirtyoff = bp->nb_dirtyend = 0;
							}
						}
						if ((bp->nb_dirtyend > 0) && (bp->nb_dirtyoff < neweofoff)) {
							mustwrite++;
						}
					}
					nfs_buf_pgs_get_page_mask(&pagemask, round_page_64(neweofoff) / PAGE_SIZE);
					nfs_buf_pgs_bit_and(&bp->nb_dirty, &pagemask, &bp->nb_dirty);
					if (nfs_buf_pgs_is_set(&bp->nb_dirty)) {
						mustwrite++;
					}
					if (!mustwrite) {
						FSDBG(512, bp, bp->nb_flags, 0, obn);
						SET(bp->nb_flags, NB_INVAL);
						nfs_buf_release(bp, 1);
						continue;
					}
					                                                    
					                                                               
					                                                       
					CLR(bp->nb_flags, (NB_DONE | NB_ERROR | NB_INVAL | NB_ASYNC | NB_READ));
					SET(bp->nb_flags, NB_STABLE | NB_NOCACHE);
					if (!IS_VALID_CRED(bp->nb_wcred)) {
						kauth_cred_t cred = vfs_context_ucred(ctx);
						kauth_cred_ref(cred);
						bp->nb_wcred = cred;
					}
					error = nfs_buf_write(bp);
					                             
					if (error) {
						FSDBG(512, bp, 0xd00dee, 0xbad, error);
						nfs_node_lock_force(np);
						np->n_error = error;
						np->n_flag |= NWRITEERR;
						  
                                               
                                              
                                          
                                               
                                                
         
						NATTRINVALIDATE(np);
						nfs_node_unlock(np);
						nfs_data_unlock(np);
						nfs_vinvalbuf(vp, V_SAVE | V_IGNORE_WRITEERR, ctx, 1);
						nfs_data_lock(np, NFS_DATA_LOCK_EXCLUSIVE);
						error = 0;
					}
				}
			}
			if (vap->va_data_size != np->n_size) {
				ubc_setsize(vp, (off_t)vap->va_data_size);                 
			}
			origsize = np->n_size;
			np->n_size = np->n_vattr.nva_size = vap->va_data_size;
			nfs_node_lock_force(np);
			CLR(np->n_flag, NUPDATESIZE);
			nfs_node_unlock(np);
			FSDBG(512, np, np->n_size, np->n_vattr.nva_size, 0xf00d0001);
		}
	} else if (VATTR_IS_ACTIVE(vap, va_modify_time) ||
	    VATTR_IS_ACTIVE(vap, va_access_time) ||
	    (vap->va_vaflags & VA_UTIMES_NULL)) {
		if ((error = nfs_node_lock(np))) {
#if CONFIG_NFS4
			if (nfsvers >= NFS_VER4) {
				nfs_mount_state_in_use_end(nmp, 0);
			}
#endif
			return error;
		}
		if ((np->n_flag & NMODIFIED) && (vnode_vtype(vp) == VREG)) {
			nfs_node_unlock(np);
			error = nfs_vinvalbuf(vp, V_SAVE, ctx, 1);
			if (error == EINTR) {
#if CONFIG_NFS4
				if (nfsvers >= NFS_VER4) {
					nfs_mount_state_in_use_end(nmp, 0);
				}
#endif
				return error;
			}
		} else {
			nfs_node_unlock(np);
		}
	}

	MALLOC(dul, struct nfs_dulookup *, sizeof(*dul), M_TEMP, M_WAITOK);

	if ((VATTR_IS_ACTIVE(vap, va_mode) || VATTR_IS_ACTIVE(vap, va_uid) || VATTR_IS_ACTIVE(vap, va_gid) ||
	    VATTR_IS_ACTIVE(vap, va_acl) || VATTR_IS_ACTIVE(vap, va_uuuid) || VATTR_IS_ACTIVE(vap, va_guuid)) &&
	    !(error = nfs_node_lock(np))) {
		NACCESSINVALIDATE(np);
		nfs_node_unlock(np);
		if (!namedattrs) {
			dvp = vnode_getparent(vp);
			vname = vnode_getname(vp);
			dnp = (dvp && vname) ? VTONFS(dvp) : NULL;
			if (dnp) {
				if (nfs_node_set_busy(dnp, vfs_context_thread(ctx))) {
					vnode_put(dvp);
					vnode_putname(vname);
				} else {
					nfs_dulookup_init(dul, dnp, vname, NFS_STRLEN_INT(vname), ctx);
					nfs_dulookup_start(dul, dnp, ctx);
					dul_in_progress = 1;
				}
			} else {
				if (dvp) {
					vnode_put(dvp);
				}
				if (vname) {
					vnode_putname(vname);
				}
			}
		}
	}

	if (!error) {
		error = nmp->nm_funcs->nf_setattr_rpc(np, vap, ctx);
	}

	if (dul_in_progress) {
		nfs_dulookup_finish(dul, dnp, ctx);
		nfs_node_clear_busy(dnp);
		vnode_put(dvp);
		vnode_putname(vname);
	}

	FREE(dul, M_TEMP);
	FSDBG_BOT(512, np->n_size, vap->va_data_size, np->n_vattr.nva_size, error);
	if (VATTR_IS_ACTIVE(vap, va_data_size)) {
		if (error && (origsize != np->n_size) &&
		    ((nfsvers < NFS_VER4) || !nfs_mount_state_error_should_restart(error))) {
			                                                        
			                                                         
			int err;                                  
			np->n_size = np->n_vattr.nva_size = origsize;
			nfs_node_lock_force(np);
			CLR(np->n_flag, NUPDATESIZE);
			nfs_node_unlock(np);
			FSDBG(512, np, np->n_size, np->n_vattr.nva_size, 0xf00d0002);
			ubc_setsize(vp, (off_t)np->n_size);                      
			vapsize = vap->va_data_size;
			vap->va_data_size = origsize;
			err = nmp->nm_funcs->nf_setattr_rpc(np, vap, ctx);
			if (err) {
				NP(np, "nfs_vnop_setattr: nfs%d_setattr_rpc %d %d", nfsvers, error, err);
			}
			vap->va_data_size = vapsize;
		}
		nfs_node_lock_force(np);
		  
                                                                            
                                                                                
                                                                               
                             
     
		if (ISSET(np->n_flag, NUPDATESIZE)) {
			CLR(np->n_flag, NUPDATESIZE);
			NATTRINVALIDATE(np);
			nfs_node_unlock(np);
			nfs_getattr(np, NULL, ctx, NGA_UNCACHED);
		} else {
			nfs_node_unlock(np);
		}
		nfs_data_unlock(np);
#if CONFIG_NFS4
		if (nfsvers >= NFS_VER4) {
			if (nofp) {
				                                                            
				if (!nfs_mount_state_error_should_restart(error) &&
				    (nofp->nof_flags & NFS_OPEN_FILE_SETATTR)) {
					int err = nfs_close(np, nofp, NFS_OPEN_SHARE_ACCESS_WRITE, NFS_OPEN_SHARE_DENY_NONE, ctx);
					if (err) {
						NP(np, "nfs_vnop_setattr: close error: %d", err);
					}
					nofp->nof_flags &= ~NFS_OPEN_FILE_SETATTR;
				}
				nfs_open_file_clear_busy(nofp);
				nofp = NULL;
			}
			if (nfs_mount_state_in_use_end(nmp, error)) {
				goto restart;
			}
			nfs_open_owner_rele(noop);
		}
#endif
	}
	return error;
}

  
                         
   
int
nfs3_setattr_rpc(
	nfsnode_t np,
	struct vnode_attr *vap,
	vfs_context_t ctx)
{
	struct nfsmount *nmp = NFSTONMP(np);
	int error = 0, lockerror = ENOENT, status = 0, wccpostattr = 0, nfsvers;
	u_int64_t xid, nextxid;
	struct nfsm_chain nmreq, nmrep;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	VATTR_SET_SUPPORTED(vap, va_mode);
	VATTR_SET_SUPPORTED(vap, va_uid);
	VATTR_SET_SUPPORTED(vap, va_gid);
	VATTR_SET_SUPPORTED(vap, va_data_size);
	VATTR_SET_SUPPORTED(vap, va_access_time);
	VATTR_SET_SUPPORTED(vap, va_modify_time);


	if (VATTR_IS_ACTIVE(vap, va_flags)
	    ) {
		if (vap->va_flags) {                                        
			if (vap->va_active & ~VNODE_ATTR_va_flags) {
				return EINVAL;                                                        
			} else {
				return ENOTSUP;                                          
			}
		}
		                                           
		if (!(vap->va_active & ~VNODE_ATTR_va_flags)) {
			return 0;                                                     
		}
	}

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq,
	    NFSX_FH(nfsvers) + NFSX_SATTR(nfsvers));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	if (nfsvers == NFS_VER3) {
		if (VATTR_IS_ACTIVE(vap, va_mode)) {
			nfsm_chain_add_32(error, &nmreq, TRUE);
			nfsm_chain_add_32(error, &nmreq, vap->va_mode);
		} else {
			nfsm_chain_add_32(error, &nmreq, FALSE);
		}
		if (VATTR_IS_ACTIVE(vap, va_uid)) {
			nfsm_chain_add_32(error, &nmreq, TRUE);
			nfsm_chain_add_32(error, &nmreq, vap->va_uid);
		} else {
			nfsm_chain_add_32(error, &nmreq, FALSE);
		}
		if (VATTR_IS_ACTIVE(vap, va_gid)) {
			nfsm_chain_add_32(error, &nmreq, TRUE);
			nfsm_chain_add_32(error, &nmreq, vap->va_gid);
		} else {
			nfsm_chain_add_32(error, &nmreq, FALSE);
		}
		if (VATTR_IS_ACTIVE(vap, va_data_size)) {
			nfsm_chain_add_32(error, &nmreq, TRUE);
			nfsm_chain_add_64(error, &nmreq, vap->va_data_size);
		} else {
			nfsm_chain_add_32(error, &nmreq, FALSE);
		}
		if (vap->va_vaflags & VA_UTIMES_NULL) {
			nfsm_chain_add_32(error, &nmreq, NFS_TIME_SET_TO_SERVER);
			nfsm_chain_add_32(error, &nmreq, NFS_TIME_SET_TO_SERVER);
		} else {
			if (VATTR_IS_ACTIVE(vap, va_access_time)) {
				nfsm_chain_add_32(error, &nmreq, NFS_TIME_SET_TO_CLIENT);
				nfsm_chain_add_32(error, &nmreq, vap->va_access_time.tv_sec);
				nfsm_chain_add_32(error, &nmreq, vap->va_access_time.tv_nsec);
			} else {
				nfsm_chain_add_32(error, &nmreq, NFS_TIME_DONT_CHANGE);
			}
			if (VATTR_IS_ACTIVE(vap, va_modify_time)) {
				nfsm_chain_add_32(error, &nmreq, NFS_TIME_SET_TO_CLIENT);
				nfsm_chain_add_32(error, &nmreq, vap->va_modify_time.tv_sec);
				nfsm_chain_add_32(error, &nmreq, vap->va_modify_time.tv_nsec);
			} else {
				nfsm_chain_add_32(error, &nmreq, NFS_TIME_DONT_CHANGE);
			}
		}
		nfsm_chain_add_32(error, &nmreq, FALSE);
	} else {
		nfsm_chain_add_32(error, &nmreq, VATTR_IS_ACTIVE(vap, va_mode) ?
		    vtonfsv2_mode(vnode_vtype(NFSTOV(np)), vap->va_mode) : -1);
		nfsm_chain_add_32(error, &nmreq, VATTR_IS_ACTIVE(vap, va_uid) ?
		    vap->va_uid : (uint32_t)-1);
		nfsm_chain_add_32(error, &nmreq, VATTR_IS_ACTIVE(vap, va_gid) ?
		    vap->va_gid : (uint32_t)-1);
		nfsm_chain_add_32(error, &nmreq, VATTR_IS_ACTIVE(vap, va_data_size) ?
		    vap->va_data_size : (uint32_t)-1);
		if (VATTR_IS_ACTIVE(vap, va_access_time)) {
			nfsm_chain_add_32(error, &nmreq, vap->va_access_time.tv_sec);
			nfsm_chain_add_32(error, &nmreq, (vap->va_access_time.tv_nsec != -1) ?
			    ((uint32_t)vap->va_access_time.tv_nsec / 1000) : 0xffffffff);
		} else {
			nfsm_chain_add_32(error, &nmreq, -1);
			nfsm_chain_add_32(error, &nmreq, -1);
		}
		if (VATTR_IS_ACTIVE(vap, va_modify_time)) {
			nfsm_chain_add_32(error, &nmreq, vap->va_modify_time.tv_sec);
			nfsm_chain_add_32(error, &nmreq, (vap->va_modify_time.tv_nsec != -1) ?
			    ((uint32_t)vap->va_modify_time.tv_nsec / 1000) : 0xffffffff);
		} else {
			nfsm_chain_add_32(error, &nmreq, -1);
			nfsm_chain_add_32(error, &nmreq, -1);
		}
	}
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);
	error = nfs_request(np, NULL, &nmreq, NFSPROC_SETATTR, ctx, NULL, &nmrep, &xid, &status);
	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	if (nfsvers == NFS_VER3) {
		struct timespec premtime = { .tv_sec = 0, .tv_nsec = 0 };
		nfsm_chain_get_wcc_data(error, &nmrep, np, &premtime, &wccpostattr, &xid);
		nfsmout_if(error);
		                                                 
		if (nfstimespeccmp(&np->n_mtime, &premtime, ==)) {
			NFS_CHANGED_UPDATE(nfsvers, np, &np->n_vattr);
		}
		                                                         
		if ((vnode_vtype(NFSTOV(np)) == VDIR) &&
		    nfstimespeccmp(&np->n_ncmtime, &premtime, ==)) {
			NFS_CHANGED_UPDATE_NC(nfsvers, np, &np->n_vattr);
		}
		if (!wccpostattr) {
			NATTRINVALIDATE(np);
		}
		error = status;
	} else {
		if (!error) {
			error = status;
		}
		nfsm_chain_loadattr(error, &nmrep, np, nfsvers, &xid);
	}
	  
                                                                   
                                                                  
                                                                   
                                                                        
                                                                     
                                                                   
                    
    
	nextxid = 0;
	nfs_get_xid(&nextxid);
	if (nextxid != (xid + 1)) {
		np->n_xid = nextxid;
		NATTRINVALIDATE(np);
	}
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

  
                                         
                      
                                                            
   
int
nfs_vnop_lookup(
	struct vnop_lookup_args     
                                                                
                                                  
                                                   
                                                                
                                                            
                                        *ap)
{
	vfs_context_t ctx = ap->a_context;
	struct componentname *cnp = ap->a_cnp;
	vnode_t dvp = ap->a_dvp;
	vnode_t *vpp = ap->a_vpp;
	int flags = cnp->cn_flags;
	vnode_t newvp;
	nfsnode_t dnp, np;
	struct nfsmount *nmp;
	mount_t mp;
	int nfsvers, error, busyerror = ENOENT, isdot, isdotdot, negnamecache;
	u_int64_t xid = 0;
	struct nfs_vattr *nvattr;
	int ngflags, skipdu = 0;
	struct vnop_access_args naa;
	fhandle_t *fh;
	struct nfsreq *req;

	*vpp = NULLVP;

	dnp = VTONFS(dvp);

	fh = zalloc(nfs_fhandle_zone);
	req = zalloc_flags(nfs_req_zone, Z_WAITOK);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	NVATTR_INIT(nvattr);

	mp = vnode_mount(dvp);
	nmp = VFSTONFS(mp);
	if (nfs_mount_gone(nmp)) {
		error = ENXIO;
		goto error_return;
	}
	nfsvers = nmp->nm_vers;
	negnamecache = !NMFLAG(nmp, NONEGNAMECACHE);

	if ((error = busyerror = nfs_node_set_busy(dnp, vfs_context_thread(ctx)))) {
		goto error_return;
	}
	                                                       
	if ((error = nfs_getattr(dnp, NULL, ctx, NGA_CACHED))) {
		goto error_return;
	}

	error = cache_lookup(dvp, vpp, cnp);
	switch (error) {
	case ENOENT:
		                          
		goto error_return;
	case 0:
		                
		if ((nfsvers > NFS_VER2) && NMFLAG(nmp, RDIRPLUS)) {
			                                           
			error = nfs_dir_buf_cache_lookup(dnp, &np, cnp, ctx, 0, &skipdu);
			if (!error && np) {
				                       
				*vpp = NFSTOV(np);
				error = -1;
			} else if (skipdu) {
				                              
				error = ENOENT;
				goto error_return;
			}
		}
		if (error != -1) {                 
			break;
		}
		OS_FALLTHROUGH;
	case -1:
		                                    
		OSAddAtomic64(1, &nfsstats.lookupcache_hits);

		nfs_node_clear_busy(dnp);
		busyerror = ENOENT;

		                                
		naa.a_desc = &vnop_access_desc;
		naa.a_vp = dvp;
		naa.a_action = KAUTH_VNODE_SEARCH;
		naa.a_context = ctx;

		                                                           
		error = nfs_vnop_access(&naa);
		OS_FALLTHROUGH;
	default:
		                                        
		goto error_return;
	}

	                                                     
	isdot = isdotdot = 0;
	if (cnp->cn_nameptr[0] == '.') {
		if (cnp->cn_namelen == 1) {
			isdot = 1;
		}
		if ((cnp->cn_namelen == 2) && (cnp->cn_nameptr[1] == '.')) {
			isdotdot = 1;
		}
	}
	if (isdotdot || isdot) {
		fh->fh_len = 0;
		goto found;
	}
#if CONFIG_NFS4
	if ((nfsvers >= NFS_VER4) && (dnp->n_vattr.nva_flags & NFS_FFLAG_TRIGGER)) {
		                                                                                 
		error = ENOENT;
		goto error_return;
	}
#endif

	                                       
	nmp = VTONMP(dvp);
	if (nfs_mount_gone(nmp)) {
		error = ENXIO;
		goto error_return;
	}
	if (NFS_BITMAP_ISSET(nmp->nm_fsattr.nfsa_bitmap, NFS_FATTR_MAXNAME) &&
	    (cnp->cn_namelen > nmp->nm_fsattr.nfsa_maxname)) {
		error = ENAMETOOLONG;
		goto error_return;
	}

	error = 0;
	newvp = NULLVP;

	OSAddAtomic64(1, &nfsstats.lookupcache_misses);

	error = nmp->nm_funcs->nf_lookup_rpc_async(dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx, &req);
	nfsmout_if(error);
	error = nmp->nm_funcs->nf_lookup_rpc_async_finish(dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx, req, &xid, fh, nvattr);
	nfsmout_if(error);

	                                                                  
	isdot = NFS_CMPFH(dnp, fh->fh_data, fh->fh_len);

found:
	if (flags & ISLASTCN) {
		switch (cnp->cn_nameiop) {
		case DELETE:
			cnp->cn_flags &= ~MAKEENTRY;
			break;
		case RENAME:
			cnp->cn_flags &= ~MAKEENTRY;
			if (isdot) {
				error = EISDIR;
				goto error_return;
			}
			break;
		}
	}

	if (isdotdot) {
		newvp = vnode_getparent(dvp);
		if (!newvp) {
			error = ENOENT;
			goto error_return;
		}
	} else if (isdot) {
		error = vnode_get(dvp);
		if (error) {
			goto error_return;
		}
		newvp = dvp;
		nfs_node_lock_force(dnp);
		if (fh->fh_len && (dnp->n_xid <= xid)) {
			nfs_loadattrcache(dnp, nvattr, &xid, 0);
		}
		nfs_node_unlock(dnp);
	} else {
		ngflags = (cnp->cn_flags & MAKEENTRY) ? NG_MAKEENTRY : 0;
		error = nfs_nget(mp, dnp, cnp, fh->fh_data, fh->fh_len, nvattr, &xid, req->r_auth, ngflags, &np);
		if (error) {
			goto error_return;
		}
		newvp = NFSTOV(np);
		nfs_node_unlock(np);
	}
	*vpp = newvp;

nfsmout:
	if (error) {
		if (((cnp->cn_nameiop == CREATE) || (cnp->cn_nameiop == RENAME)) &&
		    (flags & ISLASTCN) && (error == ENOENT)) {
			if (vnode_mount(dvp) && vnode_vfsisrdonly(dvp)) {
				error = EROFS;
			} else {
				error = EJUSTRETURN;
			}
		}
	}
	if ((error == ENOENT) && (cnp->cn_flags & MAKEENTRY) &&
	    (cnp->cn_nameiop != CREATE) && negnamecache) {
		                                            
		nfs_node_lock_force(dnp);
		cache_enter(dvp, NULL, cnp);
		dnp->n_flag |= NNEGNCENTRIES;
		nfs_node_unlock(dnp);
	}
error_return:
	NVATTR_CLEANUP(nvattr);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	NFS_ZFREE(nfs_req_zone, req);
	FREE(nvattr, M_TEMP);
	if (!busyerror) {
		nfs_node_clear_busy(dnp);
	}
	if (error && *vpp) {
		vnode_put(*vpp);
		*vpp = NULLVP;
	}
	return error;
}

int nfs_readlink_nocache = DEFAULT_READLINK_NOCACHE;

  
                    
   
int
nfs_vnop_readlink(
	struct vnop_readlink_args     
                                                                  
                                                   
                                                        
                                                              
                                          *ap)
{
	vfs_context_t ctx = ap->a_context;
	nfsnode_t np = VTONFS(ap->a_vp);
	struct nfsmount *nmp;
	int error = 0, nfsvers;
	size_t buflen;
	uio_t uio = ap->a_uio;
	struct nfsbuf *bp = NULL;
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
	long timeo = 0;

	if (vnode_vtype(ap->a_vp) != VLNK) {
		return EPERM;
	}

	if (uio_resid(uio) == 0) {
		return 0;
	}
	if (uio_offset(uio) < 0) {
		return EINVAL;
	}

	nmp = VTONMP(ap->a_vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;


	                                                       
	if ((error = nfs_getattr(np, NULL, ctx, nfs_readlink_nocache ? NGA_UNCACHED : NGA_CACHED))) {
		FSDBG(531, np, 0xd1e0001, 0, error);
		return error;
	}

	if (nfs_readlink_nocache) {
		timeo = nfs_attrcachetimeout(np);
		nanouptime(&ts);
	}

retry:
	OSAddAtomic64(1, &nfsstats.biocache_readlinks);
	error = nfs_buf_get(np, 0, NFS_MAXPATHLEN, vfs_context_thread(ctx), NBLK_META, &bp);
	if (error) {
		FSDBG(531, np, 0xd1e0002, 0, error);
		return error;
	}

	if (nfs_readlink_nocache) {
		NFS_VNOP_DBG("timeo = %ld ts.tv_sec = %ld need refresh = %d cached = %d\n", timeo, ts.tv_sec,
		    (np->n_rltim.tv_sec + timeo) < ts.tv_sec || nfs_readlink_nocache > 1,
		    ISSET(bp->nb_flags, NB_CACHE) == NB_CACHE);
		                                                       
		if (ISSET(bp->nb_flags, NB_CACHE) && ((nfs_readlink_nocache > 1) || ((np->n_rltim.tv_sec + timeo) < ts.tv_sec))) {
			SET(bp->nb_flags, NB_INVAL);
			nfs_buf_release(bp, 0);
			goto retry;
		}
	}
	if (!ISSET(bp->nb_flags, NB_CACHE)) {
readagain:
		OSAddAtomic64(1, &nfsstats.readlink_bios);
		buflen = bp->nb_bufsize;
		error = nmp->nm_funcs->nf_readlink_rpc(np, bp->nb_data, &buflen, ctx);
		if (error) {
			if (error == ESTALE) {
				NFS_VNOP_DBG("Stale FH from readlink rpc\n");
				error = nfs_refresh_fh(np, ctx);
				if (error == 0) {
					goto readagain;
				}
			}
			SET(bp->nb_flags, NB_ERROR);
			bp->nb_error = error;
			NFS_VNOP_DBG("readlink failed %d\n", error);
		} else {
			bp->nb_validoff = 0;
			bp->nb_validend = buflen;
			np->n_rltim = ts;
			NFS_VNOP_DBG("readlink of %.*s\n", (int32_t)bp->nb_validend, (char *)bp->nb_data);
		}
	} else {
		NFS_VNOP_DBG("got cached link of %.*s\n", (int32_t)bp->nb_validend, (char *)bp->nb_data);
	}

	if (!error && (bp->nb_validend > 0)) {
		int validend32 = bp->nb_validend > INT_MAX ? INT_MAX : (int)bp->nb_validend;
		error = uiomove(bp->nb_data, validend32, uio);
		if (!error && bp->nb_validend > validend32) {
			error = uiomove(bp->nb_data + validend32, (int)(bp->nb_validend - validend32), uio);
		}
	}
	FSDBG(531, np, bp->nb_validend, 0, error);
	nfs_buf_release(bp, 1);
	return error;
}

  
                     
   
int
nfs3_readlink_rpc(nfsnode_t np, char *buf, size_t *buflenp, vfs_context_t ctx)
{
	struct nfsmount *nmp;
	int error = 0, lockerror = ENOENT, nfsvers, status;
	size_t len;
	u_int64_t xid;
	struct nfsm_chain nmreq, nmrep;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq, NFSX_FH(nfsvers));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);
	error = nfs_request(np, NULL, &nmreq, NFSPROC_READLINK, ctx, NULL, &nmrep, &xid, &status);
	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	if (nfsvers == NFS_VER3) {
		nfsm_chain_postop_attr_update(error, &nmrep, np, &xid);
	}
	if (!error) {
		error = status;
	}
	nfsm_chain_get_32(error, &nmrep, len);
	nfsmout_if(error);
	if ((nfsvers == NFS_VER2) && (len > *buflenp)) {
		error = EBADRPC;
		goto nfsmout;
	}
	if (len >= *buflenp) {
		if (np->n_size && (np->n_size < *buflenp)) {
			len = (size_t)np->n_size;
		} else {
			len = *buflenp - 1;
		}
	}
	nfsm_chain_get_opaque(error, &nmrep, len, buf);
	if (!error) {
		*buflenp = len;
	}
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

  
                    
              
   
int
nfs_read_rpc(nfsnode_t np, uio_t uio, vfs_context_t ctx)
{
	struct nfsmount *nmp;
	int error = 0, nfsvers, eof = 0;
	size_t nmrsize, len, retlen;
	user_ssize_t tsiz;
	off_t txoffset;
	struct nfsreq *req;
#if CONFIG_NFS4
	uint32_t stategenid = 0, restart = 0;
#endif
	FSDBG_TOP(536, np, uio_offset(uio), uio_resid(uio), 0);
	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	nmrsize = nmp->nm_rsize;

	txoffset = uio_offset(uio);
	tsiz = uio_resid(uio);
	if ((nfsvers == NFS_VER2) && ((uint64_t)(txoffset + tsiz) > 0xffffffffULL)) {
		FSDBG_BOT(536, np, uio_offset(uio), uio_resid(uio), EFBIG);
		return EFBIG;
	}

	req = zalloc_flags(nfs_req_zone, Z_WAITOK);
	while (tsiz > 0) {
		len = retlen = (tsiz > (user_ssize_t)nmrsize) ? nmrsize : (size_t)tsiz;
		FSDBG(536, np, txoffset, len, 0);
		if (np->n_flag & NREVOKE) {
			error = EIO;
			break;
		}
#if CONFIG_NFS4
		if (nmp->nm_vers >= NFS_VER4) {
			stategenid = nmp->nm_stategenid;
		}
#endif
		error = nmp->nm_funcs->nf_read_rpc_async(np, txoffset, len,
		    vfs_context_thread(ctx), vfs_context_ucred(ctx), NULL, &req);
		if (!error) {
			error = nmp->nm_funcs->nf_read_rpc_async_finish(np, req, uio, &retlen, &eof);
		}
#if CONFIG_NFS4
		if ((nmp->nm_vers >= NFS_VER4) && nfs_mount_state_error_should_restart(error) &&
		    (++restart <= nfs_mount_state_max_restarts(nmp))) {                                
			lck_mtx_lock(&nmp->nm_lock);
			if ((error != NFSERR_GRACE) && (stategenid == nmp->nm_stategenid)) {
				NP(np, "nfs_read_rpc: error %d, initiating recovery", error);
				nfs_need_recover(nmp, error);
			}
			lck_mtx_unlock(&nmp->nm_lock);
			if (np->n_flag & NREVOKE) {
				error = EIO;
			} else {
				if (error == NFSERR_GRACE) {
					tsleep(&nmp->nm_state, (PZERO - 1), "nfsgrace", 2 * hz);
				}
				if (!(error = nfs_mount_state_wait_for_recovery(nmp))) {
					continue;
				}
			}
		}
#endif
		if (error) {
			break;
		}
		txoffset += retlen;
		tsiz -= retlen;
		if (nfsvers != NFS_VER2) {
			if (eof || (retlen == 0)) {
				tsiz = 0;
			}
		} else if (retlen < len) {
			tsiz = 0;
		}
	}

	NFS_ZFREE(nfs_req_zone, req);
	FSDBG_BOT(536, np, eof, uio_resid(uio), error);
	return error;
}

int
nfs3_read_rpc_async(
	nfsnode_t np,
	off_t offset,
	size_t len,
	thread_t thd,
	kauth_cred_t cred,
	struct nfsreq_cbinfo *cb,
	struct nfsreq **reqp)
{
	struct nfsmount *nmp;
	int error = 0, nfsvers;
	struct nfsm_chain nmreq;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	nfsm_chain_null(&nmreq);
	nfsm_chain_build_alloc_init(error, &nmreq, NFSX_FH(nfsvers) + 3 * NFSX_UNSIGNED);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	if (nfsvers == NFS_VER3) {
		nfsm_chain_add_64(error, &nmreq, offset);
		nfsm_chain_add_32(error, &nmreq, len);
	} else {
		nfsm_chain_add_32(error, &nmreq, offset);
		nfsm_chain_add_32(error, &nmreq, len);
		nfsm_chain_add_32(error, &nmreq, 0);
	}
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);
	error = nfs_request_async(np, NULL, &nmreq, NFSPROC_READ, thd, cred, NULL, 0, cb, reqp);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	return error;
}

int
nfs3_read_rpc_async_finish(
	nfsnode_t np,
	struct nfsreq *req,
	uio_t uio,
	size_t *lenp,
	int *eofp)
{
	int error = 0, lockerror, nfsvers, status = 0, eof = 0;
	uint32_t retlen = 0;
	uint64_t xid;
	struct nfsmount *nmp;
	struct nfsm_chain nmrep;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		nfs_request_async_cancel(req);
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	nfsm_chain_null(&nmrep);

	error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	if (error == EINPROGRESS) {                              
		return error;
	}

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	if (nfsvers == NFS_VER3) {
		nfsm_chain_postop_attr_update(error, &nmrep, np, &xid);
	}
	if (!error) {
		error = status;
	}
	if (nfsvers == NFS_VER3) {
		nfsm_chain_adv(error, &nmrep, NFSX_UNSIGNED);
		nfsm_chain_get_32(error, &nmrep, eof);
	} else {
		nfsm_chain_loadattr(error, &nmrep, np, nfsvers, &xid);
	}
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsm_chain_get_32(error, &nmrep, retlen);
	if ((nfsvers == NFS_VER2) && (retlen > *lenp)) {
		error = EBADRPC;
	}
	nfsmout_if(error);
	error = nfsm_chain_get_uio(&nmrep, MIN(retlen, *lenp), uio);
	if (eofp) {
		if (nfsvers == NFS_VER3) {
			if (!eof && !retlen) {
				eof = 1;
			}
		} else if (retlen < *lenp) {
			eof = 1;
		}
		*eofp = eof;
	}
	*lenp = MIN(retlen, *lenp);
nfsmout:
	nfsm_chain_cleanup(&nmrep);
	return error;
}

  
                 
   
int
nfs_vnop_write(
	struct vnop_write_args     
                                                               
                                                
                                                     
                                                
                                                           
                                       *ap)
{
	vfs_context_t ctx = ap->a_context;
	uio_t uio = ap->a_uio;
	vnode_t vp = ap->a_vp;
	nfsnode_t np = VTONFS(vp);
	int ioflag = ap->a_ioflag;
	struct nfsbuf *bp;
	struct nfsmount *nmp = VTONMP(vp);
	daddr64_t lbn;
	uint32_t biosize;
	int error = 0;
	off_t n, on;
	int n32;
	off_t boff, start, end;
	uio_t auio;
	char auio_buf[UIO_SIZEOF(1)];
	thread_t thd;
	kauth_cred_t cred;

	FSDBG_TOP(515, np, uio_offset(uio), uio_resid(uio), ioflag);

	if (vnode_vtype(vp) != VREG) {
		FSDBG_BOT(515, np, uio_offset(uio), uio_resid(uio), EIO);
		return EIO;
	}

	thd = vfs_context_thread(ctx);
	cred = vfs_context_ucred(ctx);

	nfs_data_lock(np, NFS_DATA_LOCK_SHARED);

	if ((error = nfs_node_lock(np))) {
		nfs_data_unlock(np);
		FSDBG_BOT(515, np, uio_offset(uio), uio_resid(uio), error);
		return error;
	}
	np->n_wrbusy++;

	if (np->n_flag & NWRITEERR) {
		error = np->n_error;
		np->n_flag &= ~NWRITEERR;
	}
	if (np->n_flag & NNEEDINVALIDATE) {
		np->n_flag &= ~NNEEDINVALIDATE;
		nfs_node_unlock(np);
		nfs_data_unlock(np);
		nfs_vinvalbuf(vp, V_SAVE | V_IGNORE_WRITEERR, ctx, 1);
		nfs_data_lock(np, NFS_DATA_LOCK_SHARED);
	} else {
		nfs_node_unlock(np);
	}
	if (error) {
		goto out;
	}

	biosize = nmp->nm_biosize;

	if (ioflag & (IO_APPEND | IO_SYNC)) {
		nfs_node_lock_force(np);
		if (np->n_flag & NMODIFIED) {
			NATTRINVALIDATE(np);
			nfs_node_unlock(np);
			nfs_data_unlock(np);
			error = nfs_vinvalbuf(vp, V_SAVE, ctx, 1);
			nfs_data_lock(np, NFS_DATA_LOCK_SHARED);
			if (error) {
				FSDBG(515, np, uio_offset(uio), 0x10bad01, error);
				goto out;
			}
		} else {
			nfs_node_unlock(np);
		}
		if (ioflag & IO_APPEND) {
			nfs_data_unlock(np);
			                                                       
			error = nfs_getattr(np, NULL, ctx, NGA_UNCACHED);
			                                                                  
			nfs_data_lock(np, NFS_DATA_LOCK_EXCLUSIVE);
			if (error) {
				FSDBG(515, np, uio_offset(uio), 0x10bad02, error);
				goto out;
			}
			uio_setoffset(uio, np->n_size);
		}
	}
	if (uio_offset(uio) < 0) {
		error = EINVAL;
		FSDBG_BOT(515, np, uio_offset(uio), 0xbad0ff, error);
		goto out;
	}
	if (uio_resid(uio) == 0) {
		goto out;
	}

	if (((uio_offset(uio) + uio_resid(uio)) > (off_t)np->n_size) && !(ioflag & IO_APPEND)) {
		  
                                                                                
     
		nfs_data_unlock(np);
		nfs_data_lock(np, NFS_DATA_LOCK_EXCLUSIVE);

		  
                                                                               
                                               
     
		struct nfsbuf *eofbp = NULL;
		daddr64_t eofbn = np->n_size / biosize;
		uint32_t eofoff = np->n_size % biosize;
		lbn = uio_offset(uio) / biosize;

		if (eofoff && (eofbn < lbn)) {
			if ((error = nfs_buf_get(np, eofbn, biosize, thd, NBLK_WRITE | NBLK_ONLYVALID, &eofbp))) {
				goto out;
			}
			np->n_size += (biosize - eofoff);
			nfs_node_lock_force(np);
			CLR(np->n_flag, NUPDATESIZE);
			np->n_flag |= NMODIFIED;
			nfs_node_unlock(np);
			FSDBG(516, np, np->n_size, np->n_vattr.nva_size, 0xf00d0001);
			ubc_setsize(vp, (off_t)np->n_size);                 
			if (eofbp) {
				  
                                                       
                                                          
                        
                                                       
                          
       
				char *d;
				int i;
				if (ioflag & IO_NOCACHE) {
					SET(eofbp->nb_flags, NB_NOCACHE);
				}
				NFS_BUF_MAP(eofbp);
				FSDBG(516, eofbp, eofoff, biosize - eofoff, 0xe0fff01e);
				d = eofbp->nb_data;
				i = eofoff / PAGE_SIZE;
				while (eofoff < biosize) {
					int poff = eofoff & PAGE_MASK;
					if (!poff || NBPGVALID(eofbp, i)) {
						bzero(d + eofoff, PAGE_SIZE - poff);
						NBPGVALID_SET(eofbp, i);
					}
					eofoff += PAGE_SIZE - poff;
					i++;
				}
				nfs_buf_release(eofbp, 1);
			}
		}
	}

	do {
		OSAddAtomic64(1, &nfsstats.biocache_writes);
		lbn = uio_offset(uio) / biosize;
		on = uio_offset(uio) % biosize;
		n = biosize - on;
		if (uio_resid(uio) < n) {
			n = uio_resid(uio);
		}
again:
		  
                                                               
                                                             
                                                          
                                               
     
		error = nfs_buf_get(np, lbn, biosize, thd, NBLK_WRITE, &bp);
		if (error) {
			goto out;
		}
		                                                              
		NFS_BUF_MAP(bp);

		if (ioflag & IO_NOCACHE) {
			SET(bp->nb_flags, NB_NOCACHE);
		}

		if (!IS_VALID_CRED(bp->nb_wcred)) {
			kauth_cred_ref(cred);
			bp->nb_wcred = cred;
		}

		  
                                                                      
                                                                       
    
                                                                  
                                                                       
                                                 
                                                                
                                                             
                                                            
                                                               
                                                   
     
		if (bp->nb_dirtyend > 0) {
			if (on > bp->nb_dirtyend || (on + n) < bp->nb_dirtyoff || nfs_buf_pgs_is_set(&bp->nb_dirty)) {
				FSDBG(515, np, uio_offset(uio), bp, 0xd15c001);
				                                         
				                                                               
				CLR(bp->nb_flags, (NB_DONE | NB_ERROR | NB_INVAL));
				SET(bp->nb_flags, (NB_ASYNC | NB_STABLE));
				error = nfs_buf_write(bp);
				if (error) {
					goto out;
				}
				goto again;
			}
		} else if (nfs_buf_pgs_is_set(&bp->nb_dirty)) {
			off_t firstpg = 0, lastpg = 0;
			nfsbufpgs pagemask, pagemaskand;
			                                    
			if (n > 0) {
				firstpg = on / PAGE_SIZE;
				lastpg = (on + n - 1) / PAGE_SIZE;
				nfs_buf_pgs_set_pages_between(&pagemask, firstpg, lastpg + 1);
			} else {
				NBPGS_ERASE(&pagemask);
			}
			                                                            
			nfs_buf_pgs_bit_not(&pagemask);
			nfs_buf_pgs_bit_and(&bp->nb_dirty, &pagemask, &pagemaskand);
			if (nfs_buf_pgs_is_set(&pagemaskand)) {
				FSDBG(515, np, uio_offset(uio), bp, 0xd15c002);
				                                         
				                                                               
				CLR(bp->nb_flags, (NB_DONE | NB_ERROR | NB_INVAL));
				SET(bp->nb_flags, (NB_ASYNC | NB_STABLE));
				error = nfs_buf_write(bp);
				if (error) {
					goto out;
				}
				goto again;
			}
			                                                  
			                                                            
			if (NBPGDIRTY(bp, firstpg) || NBPGDIRTY(bp, lastpg)) {
				FSDBG(515, np, uio_offset(uio), bp, 0xd15c003);
				bp->nb_dirtyoff = MIN(on, firstpg * PAGE_SIZE);
				if (NBPGDIRTY(bp, lastpg)) {
					bp->nb_dirtyend = (lastpg + 1) * PAGE_SIZE;
					                 
					if (NBOFF(bp) + bp->nb_dirtyend > (off_t)np->n_size) {
						bp->nb_dirtyend = np->n_size - NBOFF(bp);
						if (bp->nb_dirtyoff >= bp->nb_dirtyend) {
							bp->nb_dirtyoff = bp->nb_dirtyend = 0;
						}
					}
				} else {
					bp->nb_dirtyend = on + n;
				}
			}
		}

		  
                                                           
                                                        
                                                        
                            
     
		if ((uio_offset(uio) + n) > (off_t)np->n_size) {
			daddr64_t eofbn = np->n_size / biosize;
			int neweofoff = (uio_offset(uio) + n) % biosize;

			FSDBG(515, 0xb1ffa000, uio_offset(uio) + n, eofoff, neweofoff);

			                                                   
			                                                 
			if ((lbn == eofbn) && ISSET(bp->nb_flags, NB_CACHE)) {
				                                                 
				int endpg = ((neweofoff ? neweofoff : biosize) - 1) / PAGE_SIZE;
				nfsbufpgs pagemask, pagemaskand;
				                                                               
				nfs_buf_pgs_get_page_mask(&pagemask, endpg + 1);
				FSDBG(515, 0xb1ffa001, bp->nb_valid, pagemask, 0);
				nfs_buf_pgs_bit_and(&bp->nb_valid, &pagemask, &pagemaskand);
				if (!NBPGS_IS_EQUAL(&pagemaskand, &pagemask)) {
					                       
					if (on > bp->nb_validend) {
						for (off_t i = bp->nb_validend / PAGE_SIZE; i <= (on - 1) / PAGE_SIZE; i++) {
							NBPGVALID_SET(bp, i);
						}
						NFS_BUF_MAP(bp);
						FSDBG(516, bp, bp->nb_validend, on - bp->nb_validend, 0xf01e);
						NFS_BZERO((char *)bp->nb_data + bp->nb_validend, on - bp->nb_validend);
					}
					                                                 
					if (neweofoff) {
						NFS_BUF_MAP(bp);
						FSDBG(516, bp, neweofoff, PAGE_SIZE - (neweofoff & PAGE_MASK), 0xe0f);
						bzero((char *)bp->nb_data + neweofoff,
						    PAGE_SIZE - (neweofoff & PAGE_MASK));
					}
				}
			}
			np->n_size = uio_offset(uio) + n;
			nfs_node_lock_force(np);
			CLR(np->n_flag, NUPDATESIZE);
			np->n_flag |= NMODIFIED;
			nfs_node_unlock(np);
			FSDBG(516, np, np->n_size, np->n_vattr.nva_size, 0xf00d0001);
			ubc_setsize(vp, (off_t)np->n_size);                 
		}
		  
                                                              
                                      
     
		if (NBOFF(bp) + bp->nb_dirtyend > (off_t)np->n_size) {
			bp->nb_dirtyend = np->n_size - NBOFF(bp);
			if (bp->nb_dirtyoff >= bp->nb_dirtyend) {
				bp->nb_dirtyoff = bp->nb_dirtyend = 0;
			}
		}
		  
                                                              
                                                                
    
                                                             
                                          
    
                                                                
                                                             
                                                               
                                                                  
                                                                     
                                                                  
    
           
                                                                   
                                                                  
                                                                  
                                                             
                                          
                                                                   
                  
     
		if (!ISSET(bp->nb_flags, NB_CACHE) && (n < biosize)) {
			off_t firstpgoff, lastpgoff, firstpg, lastpg, dirtypg;
			start = end = -1;
			firstpg = on / PAGE_SIZE;
			firstpgoff = on & PAGE_MASK;
			lastpg = (on + n - 1) / PAGE_SIZE;
			lastpgoff = (on + n) & PAGE_MASK;
			if (firstpgoff && !NBPGVALID(bp, firstpg)) {
				                                      
				start = firstpg * PAGE_SIZE;
				end = start + firstpgoff;
			}
			if (lastpgoff && !NBPGVALID(bp, lastpg)) {
				                                   
				if (start < 0) {
					start = (lastpg * PAGE_SIZE) + lastpgoff;
				}
				end = (lastpg + 1) * PAGE_SIZE;
			}
			if (ISSET(bp->nb_flags, NB_NOCACHE)) {
				  
                                                              
                                                            
                                                           
                                                               
                                                                
                                                                 
                                                               
                      
                                                                
       
				if (end > start) {
					SET(bp->nb_flags, NB_STABLE);
				}
				goto skipread;
			}
			if (end > start) {
				                                                   

				                                             
				                                                      
				                                                 
				for (dirtypg = start / PAGE_SIZE; dirtypg <= (end - 1) / PAGE_SIZE; dirtypg++) {
					if (NBPGDIRTY(bp, dirtypg)) {
						break;
					}
				}

				                                           
				                                          
				if (!(start & PAGE_MASK)) {
					                                                     
					for (; start > 0; start -= PAGE_SIZE) {
						if (NBPGVALID(bp, ((start - 1) / PAGE_SIZE))) {
							break;
						}
					}
				}

				NFS_BUF_MAP(bp);
				                           
				boff = NBOFF(bp);
				auio = uio_createwithbuffer(1, 0, UIO_SYSSPACE, UIO_READ,
				    &auio_buf, sizeof(auio_buf));

				if (dirtypg <= (end - 1) / PAGE_SIZE) {
					                                                           
					                                        
					uio_reset(auio, boff + start, UIO_SYSSPACE, UIO_READ);
					NFS_UIO_ADDIOV(auio, CAST_USER_ADDR_T(bp->nb_data + start), on - start);
					error = nfs_read_rpc(np, auio, ctx);
					if (error) {
						                                                                    
						SET(bp->nb_flags, (NB_NOCACHE | NB_STABLE));
						goto skipread;
					}
					if (uio_resid(auio) > 0) {
						FSDBG(516, bp, (caddr_t)uio_curriovbase(auio) - bp->nb_data, uio_resid(auio), 0xd00dee01);
						bzero(CAST_DOWN(caddr_t, uio_curriovbase(auio)), uio_resid(auio));
					}
					if (!error) {
						                                           
						if ((bp->nb_validoff < 0) || (bp->nb_validoff > start)) {
							bp->nb_validoff = start;
						}
						if ((bp->nb_validend < 0) || (bp->nb_validend < on)) {
							bp->nb_validend = on;
						}
						if ((off_t)np->n_size > boff + bp->nb_validend) {
							bp->nb_validend = MIN(np->n_size - (boff + start), biosize);
						}
						                                                
						for (; start < on / PAGE_SIZE; start += PAGE_SIZE) {
							NBPGVALID_SET(bp, start / PAGE_SIZE);
						}
					}
					                                            
					start = on + n;
				}

				                                      
				                                      
				if (!(end & PAGE_MASK)) {
					                                             
					for (; end < biosize; end += PAGE_SIZE) {
						if (NBPGVALID(bp, end / PAGE_SIZE)) {
							break;
						}
					}
				}

				if (((boff + start) >= (off_t)np->n_size) ||
				    ((start >= on) && ((boff + on + n) >= (off_t)np->n_size))) {
					  
                                                         
                                                            
                                      
                                                    
                                                        
        
					FSDBG(516, bp, start, end - start, 0xd00dee00);
					NFS_BZERO(bp->nb_data + start, end - start);
					error = 0;
				} else {
					                                           
					uio_reset(auio, boff + start, UIO_SYSSPACE, UIO_READ);
					NFS_UIO_ADDIOV(auio, CAST_USER_ADDR_T(bp->nb_data + start), end - start);
					error = nfs_read_rpc(np, auio, ctx);
					if (error) {
						                                                                    
						SET(bp->nb_flags, (NB_NOCACHE | NB_STABLE));
						goto skipread;
					}
					if (uio_resid(auio) > 0) {
						FSDBG(516, bp, (caddr_t)uio_curriovbase(auio) - bp->nb_data, uio_resid(auio), 0xd00dee02);
						bzero(CAST_DOWN(caddr_t, uio_curriovbase(auio)), uio_resid(auio));
					}
				}
				if (!error) {
					                                           
					if ((bp->nb_validoff < 0) || (bp->nb_validoff > start)) {
						bp->nb_validoff = start;
					}
					if ((bp->nb_validend < 0) || (bp->nb_validend < end)) {
						bp->nb_validend = end;
					}
					if ((off_t)np->n_size > boff + bp->nb_validend) {
						bp->nb_validend = MIN(np->n_size - (boff + start), biosize);
					}
					                                                       
					for (; start < (off_t)trunc_page_64(on); start += PAGE_SIZE) {
						NBPGVALID_SET(bp, start / PAGE_SIZE);
					}
					                                                                  
					for (; (end - 1) > (off_t)round_page_64(on + n - 1); end -= PAGE_SIZE) {
						NBPGVALID_SET(bp, (end - 1) / PAGE_SIZE);
					}
				}
				                                                                 
			}
		}
skipread:

		if (ISSET(bp->nb_flags, NB_ERROR)) {
			error = bp->nb_error;
			nfs_buf_release(bp, 1);
			goto out;
		}

		nfs_node_lock_force(np);
		np->n_flag |= NMODIFIED;
		nfs_node_unlock(np);

		NFS_BUF_MAP(bp);
		if (n < 0) {
			error = EINVAL;
		} else {
			n32 = n > INT_MAX ? INT_MAX : (int)n;
			error = uiomove(bp->nb_data + on, n32, uio);
			if (!error && n > n32) {
				error = uiomove(bp->nb_data + on + n32, (int)(n - n32), uio);
			}
		}
		if (error) {
			SET(bp->nb_flags, NB_ERROR);
			nfs_buf_release(bp, 1);
			goto out;
		}

		                                   
		start = on & ~PAGE_MASK;
		for (; start < on + n; start += PAGE_SIZE) {
			NBPGVALID_SET(bp, start / PAGE_SIZE);
			  
                                                                 
                                                                     
                                                                    
                                                                    
                                                                     
                                                                    
                                               
      
		}
		if (bp->nb_dirtyend > 0) {
			bp->nb_dirtyoff = MIN(on, bp->nb_dirtyoff);
			bp->nb_dirtyend = MAX((on + n), bp->nb_dirtyend);
		} else {
			bp->nb_dirtyoff = on;
			bp->nb_dirtyend = on + n;
		}
		if (bp->nb_validend <= 0 || bp->nb_validend < bp->nb_dirtyoff ||
		    bp->nb_validoff > bp->nb_dirtyend) {
			bp->nb_validoff = bp->nb_dirtyoff;
			bp->nb_validend = bp->nb_dirtyend;
		} else {
			bp->nb_validoff = MIN(bp->nb_validoff, bp->nb_dirtyoff);
			bp->nb_validend = MAX(bp->nb_validend, bp->nb_dirtyend);
		}
		if (!ISSET(bp->nb_flags, NB_CACHE)) {
			nfs_buf_normalize_valid_range(np, bp);
		}

		  
                                                           
                                  
     
		if (ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
			nfs_node_lock_force(np);
			if (ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
				np->n_needcommitcnt--;
				CHECK_NEEDCOMMITCNT(np);
			}
			CLR(bp->nb_flags, NB_NEEDCOMMIT);
			nfs_node_unlock(np);
		}

		if (ioflag & IO_SYNC) {
			error = nfs_buf_write(bp);
			if (error) {
				goto out;
			}
			if (np->n_needcommitcnt >= NFS_A_LOT_OF_NEEDCOMMITS) {
				nfs_flushcommits(np, 1);
			}
		} else if (((n + on) == biosize) || (ioflag & IO_APPEND) ||
		    (ioflag & IO_NOCACHE) || ISSET(bp->nb_flags, NB_NOCACHE)) {
			SET(bp->nb_flags, NB_ASYNC);
			error = nfs_buf_write(bp);
			if (error) {
				goto out;
			}
		} else {
			                                                               
			if (!ISSET(bp->nb_flags, NB_DELWRI)) {
				proc_t p = vfs_context_proc(ctx);
				if (p && p->p_stats) {
					OSIncrementAtomicLong(&p->p_stats->p_ru.ru_oublock);
				}
			}
			nfs_buf_write_delayed(bp);
		}

	} while (uio_resid(uio) > 0 && n > 0);

out:
	nfs_node_lock_force(np);
	np->n_wrbusy--;
	if ((ioflag & IO_SYNC) && !np->n_wrbusy && !np->n_numoutput) {
		np->n_flag &= ~NMODIFIED;
	}
	nfs_node_unlock(np);
	nfs_data_unlock(np);
	FSDBG_BOT(515, np, uio_offset(uio), uio_resid(uio), error);
	return error;
}


  
                 
   
int
nfs_write_rpc(
	nfsnode_t np,
	uio_t uio,
	vfs_context_t ctx,
	int *iomodep,
	uint64_t *wverfp)
{
	return nfs_write_rpc2(np, uio, vfs_context_thread(ctx), vfs_context_ucred(ctx), iomodep, wverfp);
}

int
nfs_write_rpc2(
	nfsnode_t np,
	uio_t uio,
	thread_t thd,
	kauth_cred_t cred,
	int *iomodep,
	uint64_t *wverfp)
{
	struct nfsmount *nmp;
	int error = 0, nfsvers;
	int wverfset, commit = 0, committed;
	uint64_t wverf = 0, wverf2 = 0;
	size_t nmwsize, totalsize, tsiz, len, rlen = 0;
	struct nfsreq *req;
#if CONFIG_NFS4
	uint32_t stategenid = 0, restart = 0;
#endif
	uint32_t vrestart = 0;
	uio_t uio_save = NULL;

#if DIAGNOSTIC
	                                                                
	if (uio_iovcnt(uio) != 1) {
		panic("nfs3_write_rpc: iovcnt > 1");
	}
#endif
	FSDBG_TOP(537, np, uio_offset(uio), uio_resid(uio), *iomodep);
	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	nmwsize = nmp->nm_wsize;

	wverfset = 0;
	committed = NFS_WRITE_FILESYNC;

	totalsize = tsiz = uio_resid(uio);
	if ((nfsvers == NFS_VER2) && ((uint64_t)(uio_offset(uio) + tsiz) > 0xffffffffULL)) {
		FSDBG_BOT(537, np, uio_offset(uio), uio_resid(uio), EFBIG);
		return EFBIG;
	}

	uio_save = uio_duplicate(uio);
	if (uio_save == NULL) {
		return EIO;
	}

	req = zalloc_flags(nfs_req_zone, Z_WAITOK);
	while (tsiz > 0) {
		len = (tsiz > nmwsize) ? nmwsize : tsiz;
		FSDBG(537, np, uio_offset(uio), len, 0);
		if (np->n_flag & NREVOKE) {
			error = EIO;
			break;
		}
#if CONFIG_NFS4
		if (nmp->nm_vers >= NFS_VER4) {
			stategenid = nmp->nm_stategenid;
		}
#endif
		error = nmp->nm_funcs->nf_write_rpc_async(np, uio, len, thd, cred, *iomodep, NULL, &req);
		if (!error) {
			error = nmp->nm_funcs->nf_write_rpc_async_finish(np, req, &commit, &rlen, &wverf2);
		}
		nmp = NFSTONMP(np);
		if (nfs_mount_gone(nmp)) {
			error = ENXIO;
		}
#if CONFIG_NFS4
		if ((nmp->nm_vers >= NFS_VER4) && nfs_mount_state_error_should_restart(error) &&
		    (++restart <= nfs_mount_state_max_restarts(nmp))) {                                
			lck_mtx_lock(&nmp->nm_lock);
			if ((error != NFSERR_GRACE) && (stategenid == nmp->nm_stategenid)) {
				NP(np, "nfs_write_rpc: error %d, initiating recovery", error);
				nfs_need_recover(nmp, error);
			}
			lck_mtx_unlock(&nmp->nm_lock);
			if (np->n_flag & NREVOKE) {
				error = EIO;
			} else {
				if (error == NFSERR_GRACE) {
					tsleep(&nmp->nm_state, (PZERO - 1), "nfsgrace", 2 * hz);
				}
				if (!(error = nfs_mount_state_wait_for_recovery(nmp))) {
					continue;
				}
			}
		}
#endif
		if (error) {
			break;
		}
		if (nfsvers == NFS_VER2) {
			tsiz -= len;
			continue;
		}

		                             
		if (rlen < len) {
			                                                  
			*uio = *uio_save;
			uio_update(uio, totalsize - (tsiz - rlen));
			len = rlen;
		}

		                                         
		if (commit < committed) {
			committed = commit;
		}

		tsiz -= len;

		                          
		if (!wverfset) {
			wverf = wverf2;
			wverfset = 1;
		} else if (wverf != wverf2) {
			                                                            
			if (++vrestart > 100) {
				                                     
				error = EIO;
				break;
			}
			*uio = *uio_save;                                         
			committed = NFS_WRITE_FILESYNC;
			wverfset = 0;
			tsiz = totalsize;
		}
	}
	if (uio_save) {
		uio_free(uio_save);
	}
	if (wverfset && wverfp) {
		*wverfp = wverf;
	}
	*iomodep = committed;
	if (error) {
		uio_setresid(uio, tsiz);
	}
	NFS_ZFREE(nfs_req_zone, req);
	FSDBG_BOT(537, np, committed, uio_resid(uio), error);
	return error;
}

int
nfs3_write_rpc_async(
	nfsnode_t np,
	uio_t uio,
	size_t len,
	thread_t thd,
	kauth_cred_t cred,
	int iomode,
	struct nfsreq_cbinfo *cb,
	struct nfsreq **reqp)
{
	struct nfsmount *nmp;
	mount_t mp;
	int error = 0, nfsvers;
	struct nfsm_chain nmreq;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	                                                                
	if ((iomode != NFS_WRITE_UNSTABLE) && nfs_allow_async &&
	    ((mp = NFSTOMP(np))) && (vfs_flags(mp) & MNT_ASYNC)) {
		iomode = NFS_WRITE_UNSTABLE;
	}

	nfsm_chain_null(&nmreq);
	nfsm_chain_build_alloc_init(error, &nmreq,
	    NFSX_FH(nfsvers) + 5 * NFSX_UNSIGNED + nfsm_rndup(len));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	if (nfsvers == NFS_VER3) {
		nfsm_chain_add_64(error, &nmreq, uio_offset(uio));
		nfsm_chain_add_32(error, &nmreq, len);
		nfsm_chain_add_32(error, &nmreq, iomode);
	} else {
		nfsm_chain_add_32(error, &nmreq, 0);
		nfsm_chain_add_32(error, &nmreq, uio_offset(uio));
		nfsm_chain_add_32(error, &nmreq, 0);
	}
	nfsm_chain_add_32(error, &nmreq, len);
	nfsmout_if(error);
	error = nfsm_chain_add_uio(&nmreq, uio, len);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);
	error = nfs_request_async(np, NULL, &nmreq, NFSPROC_WRITE, thd, cred, NULL, 0, cb, reqp);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	return error;
}

int
nfs3_write_rpc_async_finish(
	nfsnode_t np,
	struct nfsreq *req,
	int *iomodep,
	size_t *rlenp,
	uint64_t *wverfp)
{
	struct nfsmount *nmp;
	int error = 0, lockerror = ENOENT, nfsvers, status;
	int updatemtime = 0, wccpostattr = 0, rlen, committed = NFS_WRITE_FILESYNC;
	u_int64_t xid, wverf;
	mount_t mp;
	struct nfsm_chain nmrep;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		nfs_request_async_cancel(req);
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	nfsm_chain_null(&nmrep);

	error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	if (error == EINPROGRESS) {                              
		return error;
	}
	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		error = ENXIO;
	}
	if (!error && (lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	if (nfsvers == NFS_VER3) {
		struct timespec premtime = { .tv_sec = 0, .tv_nsec = 0 };
		nfsm_chain_get_wcc_data(error, &nmrep, np, &premtime, &wccpostattr, &xid);
		if (nfstimespeccmp(&np->n_mtime, &premtime, ==)) {
			updatemtime = 1;
		}
		if (!error) {
			error = status;
		}
		nfsm_chain_get_32(error, &nmrep, rlen);
		nfsmout_if(error);
		*rlenp = rlen;
		if (rlen <= 0) {
			error = NFSERR_IO;
		}
		nfsm_chain_get_32(error, &nmrep, committed);
		nfsm_chain_get_64(error, &nmrep, wverf);
		nfsmout_if(error);
		if (wverfp) {
			*wverfp = wverf;
		}
		lck_mtx_lock(&nmp->nm_lock);
		if (!(nmp->nm_state & NFSSTA_HASWRITEVERF)) {
			nmp->nm_verf = wverf;
			nmp->nm_state |= NFSSTA_HASWRITEVERF;
		} else if (nmp->nm_verf != wverf) {
			nmp->nm_verf = wverf;
		}
		lck_mtx_unlock(&nmp->nm_lock);
	} else {
		if (!error) {
			error = status;
		}
		nfsm_chain_loadattr(error, &nmrep, np, nfsvers, &xid);
		nfsmout_if(error);
	}
	if (updatemtime) {
		NFS_CHANGED_UPDATE(nfsvers, np, &np->n_vattr);
	}
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsm_chain_cleanup(&nmrep);
	if ((committed != NFS_WRITE_FILESYNC) && nfs_allow_async &&
	    ((mp = NFSTOMP(np))) && (vfs_flags(mp) & MNT_ASYNC)) {
		committed = NFS_WRITE_FILESYNC;
	}
	*iomodep = committed;
	return error;
}

  
                     
  
                                                                              
                                                                 
   
int
nfs3_vnop_mknod(
	struct vnop_mknod_args     
                                                               
                                                 
                                                  
                                                               
                                                            
                                                           
                                       *ap)
{
	vnode_t dvp = ap->a_dvp;
	vnode_t *vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	struct vnode_attr *vap = ap->a_vap;
	vfs_context_t ctx = ap->a_context;
	vnode_t newvp = NULL;
	nfsnode_t np = NULL;
	struct nfsmount *nmp;
	nfsnode_t dnp = VTONFS(dvp);
	struct nfs_vattr *nvattr;
	fhandle_t *fh;
	int error = 0, lockerror = ENOENT, busyerror = ENOENT, status = 0, wccpostattr = 0;
	struct timespec premtime = { .tv_sec = 0, .tv_nsec = 0 };
	u_int32_t rdev;
	u_int64_t xid = 0, dxid;
	int nfsvers, gotuid, gotgid;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq *req;

	nmp = VTONMP(dvp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	if (!VATTR_IS_ACTIVE(vap, va_type)) {
		return EINVAL;
	}
	if (vap->va_type == VCHR || vap->va_type == VBLK) {
		if (!VATTR_IS_ACTIVE(vap, va_rdev)) {
			return EINVAL;
		}
		rdev = vap->va_rdev;
	} else if (vap->va_type == VFIFO || vap->va_type == VSOCK) {
		rdev = 0xffffffff;
	} else {
		return ENOTSUP;
	}
	if ((nfsvers == NFS_VER2) && (cnp->cn_namelen > NFS_MAXNAMLEN)) {
		return ENAMETOOLONG;
	}

	nfs_avoid_needless_id_setting_on_create(dnp, vap, ctx);

	VATTR_SET_SUPPORTED(vap, va_mode);
	VATTR_SET_SUPPORTED(vap, va_uid);
	VATTR_SET_SUPPORTED(vap, va_gid);
	VATTR_SET_SUPPORTED(vap, va_data_size);
	VATTR_SET_SUPPORTED(vap, va_access_time);
	VATTR_SET_SUPPORTED(vap, va_modify_time);
	gotuid = VATTR_IS_ACTIVE(vap, va_uid);
	gotgid = VATTR_IS_ACTIVE(vap, va_gid);

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	fh = zalloc(nfs_fhandle_zone);
	req = zalloc_flags(nfs_req_zone, Z_WAITOK);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);

	nfsm_chain_build_alloc_init(error, &nmreq,
	    NFSX_FH(nfsvers) + 4 * NFSX_UNSIGNED +
	    nfsm_rndup(cnp->cn_namelen) + NFSX_SATTR(nfsvers));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	if (nfsvers == NFS_VER3) {
		nfsm_chain_add_32(error, &nmreq, vtonfs_type(vap->va_type, nfsvers));
		nfsm_chain_add_v3sattr(nmp, error, &nmreq, vap);
		if (vap->va_type == VCHR || vap->va_type == VBLK) {
			nfsm_chain_add_32(error, &nmreq, major(vap->va_rdev));
			nfsm_chain_add_32(error, &nmreq, minor(vap->va_rdev));
		}
	} else {
		nfsm_chain_add_v2sattr(error, &nmreq, vap, rdev);
	}
	nfsm_chain_build_done(error, &nmreq);
	if (!error) {
		error = busyerror = nfs_node_set_busy(dnp, vfs_context_thread(ctx));
	}
	nfsmout_if(error);

	error = nfs_request_async(dnp, NULL, &nmreq, NFSPROC_MKNOD,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), NULL, 0, NULL, &req);
	if (!error) {
		error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	}

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	                                
	dxid = xid;
	if (!error && !status) {
		if (dnp->n_flag & NNEGNCENTRIES) {
			dnp->n_flag &= ~NNEGNCENTRIES;
			cache_purge_negatives(dvp);
		}
		error = nfsm_chain_get_fh_attr(nmp, &nmrep, dnp, ctx, nfsvers, &xid, fh, nvattr);
	}
	if (nfsvers == NFS_VER3) {
		nfsm_chain_get_wcc_data(error, &nmrep, dnp, &premtime, &wccpostattr, &dxid);
	}
	if (!error) {
		error = status;
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	if (!lockerror) {
		dnp->n_flag |= NMODIFIED;
		                                                         
		if (nfstimespeccmp(&dnp->n_ncmtime, &premtime, ==)) {
			NFS_CHANGED_UPDATE_NC(nfsvers, dnp, &dnp->n_vattr);
		}
		nfs_node_unlock(dnp);
		                                                       
		nfs_getattr(dnp, NULL, ctx, wccpostattr ? NGA_CACHED : NGA_UNCACHED);
	}

	if (!error && fh->fh_len) {
		error = nfs_nget(NFSTOMP(dnp), dnp, cnp, fh->fh_data, fh->fh_len, nvattr, &xid, req->r_auth, NG_MAKEENTRY, &np);
	}
	if (!error && !np) {
		error = nfs_lookitup(dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx, &np);
	}
	if (!error && np) {
		newvp = NFSTOV(np);
	}
	if (!busyerror) {
		nfs_node_clear_busy(dnp);
	}

	if (!error && (gotuid || gotgid) &&
	    (!newvp || nfs_getattrcache(np, nvattr, 0) ||
	    (gotuid && (nvattr->nva_uid != vap->va_uid)) ||
	    (gotgid && (nvattr->nva_gid != vap->va_gid)))) {
		                                                                
		VATTR_CLEAR_SUPPORTED(vap, va_uid);
		VATTR_CLEAR_SUPPORTED(vap, va_gid);
	}
	if (error) {
		if (newvp) {
			nfs_node_unlock(np);
			vnode_put(newvp);
		}
	} else {
		*vpp = newvp;
		nfs_node_unlock(np);
	}
	NFS_ZFREE(nfs_fhandle_zone, fh);
	NFS_ZFREE(nfs_req_zone, req);
	FREE(nvattr, M_TEMP);
	return error;
}

static uint32_t create_verf;
  
                       
   
int
nfs3_vnop_create(
	struct vnop_create_args     
                                                                
                                                  
                                                   
                                                                
                                                             
                                                            
                                        *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t dvp = ap->a_dvp;
	struct vnode_attr *vap = ap->a_vap;
	struct componentname *cnp = ap->a_cnp;
	struct nfs_vattr *nvattr;
	fhandle_t *fh;
	nfsnode_t np = NULL;
	struct nfsmount *nmp;
	nfsnode_t dnp = VTONFS(dvp);
	vnode_t newvp = NULL;
	int error = 0, lockerror = ENOENT, busyerror = ENOENT, status = 0, wccpostattr = 0, fmode = 0;
	struct timespec premtime = { .tv_sec = 0, .tv_nsec = 0 };
	int nfsvers, gotuid, gotgid;
	u_int64_t xid = 0, dxid;
	uint32_t val;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq *req;
	struct nfs_dulookup *dul;
	int dul_in_progress = 0;
	int namedattrs;

	nmp = VTONMP(dvp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	namedattrs = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR);

	if ((nfsvers == NFS_VER2) && (cnp->cn_namelen > NFS_MAXNAMLEN)) {
		return ENAMETOOLONG;
	}

	nfs_avoid_needless_id_setting_on_create(dnp, vap, ctx);

	VATTR_SET_SUPPORTED(vap, va_mode);
	VATTR_SET_SUPPORTED(vap, va_uid);
	VATTR_SET_SUPPORTED(vap, va_gid);
	VATTR_SET_SUPPORTED(vap, va_data_size);
	VATTR_SET_SUPPORTED(vap, va_access_time);
	VATTR_SET_SUPPORTED(vap, va_modify_time);
	gotuid = VATTR_IS_ACTIVE(vap, va_uid);
	gotgid = VATTR_IS_ACTIVE(vap, va_gid);

	if ((vap->va_vaflags & VA_EXCLUSIVE)
	    ) {
		fmode |= O_EXCL;
		if (!VATTR_IS_ACTIVE(vap, va_access_time) || !VATTR_IS_ACTIVE(vap, va_modify_time)) {
			vap->va_vaflags |= VA_UTIMES_NULL;
		}
	}

	fh = zalloc(nfs_fhandle_zone);
	req = zalloc_flags(nfs_req_zone, Z_WAITOK);
	MALLOC(dul, struct nfs_dulookup *, sizeof(*dul), M_TEMP, M_WAITOK);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);

again:
	error = busyerror = nfs_node_set_busy(dnp, vfs_context_thread(ctx));
	if (!namedattrs) {
		nfs_dulookup_init(dul, dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx);
	}

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq,
	    NFSX_FH(nfsvers) + 2 * NFSX_UNSIGNED +
	    nfsm_rndup(cnp->cn_namelen) + NFSX_SATTR(nfsvers));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	if (nfsvers == NFS_VER3) {
		if (fmode & O_EXCL) {
			nfsm_chain_add_32(error, &nmreq, NFS_CREATE_EXCLUSIVE);
			lck_rw_lock_shared(in_ifaddr_rwlock);
			if (!TAILQ_EMPTY(&in_ifaddrhead)) {
				val = IA_SIN(in_ifaddrhead.tqh_first)->sin_addr.s_addr;
			} else {
				val = create_verf;
			}
			lck_rw_done(in_ifaddr_rwlock);
			nfsm_chain_add_32(error, &nmreq, val);
			++create_verf;
			nfsm_chain_add_32(error, &nmreq, create_verf);
		} else {
			nfsm_chain_add_32(error, &nmreq, NFS_CREATE_UNCHECKED);
			nfsm_chain_add_v3sattr(nmp, error, &nmreq, vap);
		}
	} else {
		nfsm_chain_add_v2sattr(error, &nmreq, vap, 0);
	}
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);

	error = nfs_request_async(dnp, NULL, &nmreq, NFSPROC_CREATE,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), NULL, 0, NULL, &req);
	if (!error) {
		if (!namedattrs) {
			nfs_dulookup_start(dul, dnp, ctx);
			dul_in_progress = 1;
		}
		error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	}

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	dxid = xid;
	if (!error && !status) {
		if (dnp->n_flag & NNEGNCENTRIES) {
			dnp->n_flag &= ~NNEGNCENTRIES;
			cache_purge_negatives(dvp);
		}
		error = nfsm_chain_get_fh_attr(nmp, &nmrep, dnp, ctx, nfsvers, &xid, fh, nvattr);
	}
	if (nfsvers == NFS_VER3) {
		nfsm_chain_get_wcc_data(error, &nmrep, dnp, &premtime, &wccpostattr, &dxid);
	}
	if (!error) {
		error = status;
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	if (!lockerror) {
		dnp->n_flag |= NMODIFIED;
		                                                         
		if (nfstimespeccmp(&dnp->n_ncmtime, &premtime, ==)) {
			NFS_CHANGED_UPDATE_NC(nfsvers, dnp, &dnp->n_vattr);
		}
		nfs_node_unlock(dnp);
		                                                       
		nfs_getattr(dnp, NULL, ctx, wccpostattr ? NGA_CACHED : NGA_UNCACHED);
	}

	if (!error && fh->fh_len) {
		error = nfs_nget(NFSTOMP(dnp), dnp, cnp, fh->fh_data, fh->fh_len, nvattr, &xid, req->r_auth, NG_MAKEENTRY, &np);
	}
	if (!error && !np) {
		error = nfs_lookitup(dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx, &np);
	}
	if (!error && np) {
		newvp = NFSTOV(np);
	}

	if (dul_in_progress) {
		nfs_dulookup_finish(dul, dnp, ctx);
	}
	if (!busyerror) {
		nfs_node_clear_busy(dnp);
	}

	if (error) {
		if ((nfsvers == NFS_VER3) && (fmode & O_EXCL) && (error == NFSERR_NOTSUPP)) {
			fmode &= ~O_EXCL;
			goto again;
		}
		if (newvp) {
			nfs_node_unlock(np);
			vnode_put(newvp);
		}
	} else if ((nfsvers == NFS_VER3) && (fmode & O_EXCL)) {
		nfs_node_unlock(np);
		error = nfs3_setattr_rpc(np, vap, ctx);
		if (error && (gotuid || gotgid)) {
			                                                                  
			                                          
			VATTR_CLEAR_ACTIVE(vap, va_uid);
			VATTR_CLEAR_ACTIVE(vap, va_gid);
			error = nfs3_setattr_rpc(np, vap, ctx);
		}
		if (error) {
			vnode_put(newvp);
		} else {
			nfs_node_lock_force(np);
		}
	}
	if (!error) {
		*ap->a_vpp = newvp;
	}
	if (!error && (gotuid || gotgid) &&
	    (!newvp || nfs_getattrcache(np, nvattr, 0) ||
	    (gotuid && (nvattr->nva_uid != vap->va_uid)) ||
	    (gotgid && (nvattr->nva_gid != vap->va_gid)))) {
		                                                                
		VATTR_CLEAR_SUPPORTED(vap, va_uid);
		VATTR_CLEAR_SUPPORTED(vap, va_gid);
	}
	if (!error) {
		nfs_node_unlock(np);
	}
	NFS_ZFREE(nfs_fhandle_zone, fh);
	NFS_ZFREE(nfs_req_zone, req);
	FREE(dul, M_TEMP);
	FREE(nvattr, M_TEMP);
	return error;
}

  
                       
                                                                         
                                                                         
                                   
                       
                                            
                                           
           
                      
   
int
nfs_vnop_remove(
	struct vnop_remove_args     
                                                                
                                                  
                                                 
                                                                
                                                
                                                            
                                        *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	vnode_t dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	nfsnode_t dnp = VTONFS(dvp);
	nfsnode_t np = VTONFS(vp);
	int error = 0, nfsvers, namedattrs, inuse, gotattr = 0, flushed = 0, setsize = 0;
	struct nfs_vattr *nvattr;
	struct nfsmount *nmp;
	struct nfs_dulookup *dul;

	                                               

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (vnode_isdir(vp)) {
		return EPERM;
	}

	nfsvers = nmp->nm_vers;
	namedattrs = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR);
	MALLOC(dul, struct nfs_dulookup *, sizeof(*dul), M_TEMP, M_WAITOK);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);

again_relock:
	error = nfs_node_set_busy2(dnp, np, vfs_context_thread(ctx));
	if (error) {
		goto out_free;
	}

	                                            
	lck_mtx_lock(&nfs_node_hash_mutex);
	while (np->n_hflag & NHLOCKED) {
		np->n_hflag |= NHLOCKWANT;
		msleep(np, &nfs_node_hash_mutex, PINOD, "nfs_remove", NULL);
	}
	np->n_hflag |= NHLOCKED;
	lck_mtx_unlock(&nfs_node_hash_mutex);

	if (!namedattrs) {
		nfs_dulookup_init(dul, dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx);
	}

again:
	inuse = vnode_isinuse(vp, 0);
	if ((ap->a_flags & VNODE_REMOVE_NODELETEBUSY) && inuse) {
		                                                                
		error = EBUSY;
		goto out;
	}
	if (inuse && !gotattr) {
		if (nfs_getattr(np, nvattr, ctx, NGA_CACHED)) {
			nvattr->nva_nlink = 1;
		}
		gotattr = 1;
		goto again;
	}
	if (!inuse || (np->n_sillyrename && (nvattr->nva_nlink > 1))) {
		if (!inuse && !flushed) {                                  
			                     
			lck_mtx_lock(&nfs_node_hash_mutex);
			np->n_hflag &= ~NHLOCKED;
			if (np->n_hflag & NHLOCKWANT) {
				np->n_hflag &= ~NHLOCKWANT;
				wakeup(np);
			}
			lck_mtx_unlock(&nfs_node_hash_mutex);
			nfs_node_clear_busy2(dnp, np);
			error = nfs_vinvalbuf(vp, V_SAVE, ctx, 1);
			FSDBG(260, np, np->n_size, np->n_vattr.nva_size, 0xf00d0011);
			flushed = 1;
			if (error == EINTR) {
				nfs_node_lock_force(np);
				NATTRINVALIDATE(np);
				nfs_node_unlock(np);
				goto out_free;
			}
			if (!namedattrs) {
				nfs_dulookup_finish(dul, dnp, ctx);
			}
			goto again_relock;
		}
#if CONFIG_NFS4
		if ((nmp->nm_vers >= NFS_VER4) && (np->n_openflags & N_DELEG_MASK)) {
			nfs4_delegation_return(np, 0, vfs_context_thread(ctx), vfs_context_ucred(ctx));
		}
#endif
		  
                                                            
                                                           
               
     
		nfs_name_cache_purge(dnp, np, cnp, ctx);

		if (!namedattrs) {
			nfs_dulookup_start(dul, dnp, ctx);
		}

		                
		error = nmp->nm_funcs->nf_remove_rpc(dnp, cnp->cn_nameptr, cnp->cn_namelen,
		    vfs_context_thread(ctx), vfs_context_ucred(ctx));

		  
                                                                
                                                            
                                         
                                              
     
		if (error == ENOENT) {
			error = 0;
		}

		if (!error && !inuse && !np->n_sillyrename) {
			  
                                                                  
                                                                   
                                                                   
                                      
      
			lck_mtx_lock(&nfs_node_hash_mutex);
			if (np->n_hflag & NHHASHED) {
				LIST_REMOVE(np, n_hash);
				np->n_hflag &= ~NHHASHED;
				FSDBG(266, 0, np, np->n_flag, 0xb1eb1e);
			}
			lck_mtx_unlock(&nfs_node_hash_mutex);
			                                                                     
			                                      
			nfs_node_lock_force(np);
			np->n_flag &= (NMODIFIED);
			NATTRINVALIDATE(np);
			nfs_node_unlock(np);
			vnode_recycle(vp);
			setsize = 1;
		} else {
			nfs_node_lock_force(np);
			NATTRINVALIDATE(np);
			nfs_node_unlock(np);
		}
	} else if (!np->n_sillyrename) {
		if (!namedattrs) {
			nfs_dulookup_start(dul, dnp, ctx);
		}
		error = nfs_sillyrename(dnp, np, cnp, ctx);
		nfs_node_lock_force(np);
		NATTRINVALIDATE(np);
		nfs_node_unlock(np);
	} else {
		nfs_node_lock_force(np);
		NATTRINVALIDATE(np);
		nfs_node_unlock(np);
		if (!namedattrs) {
			nfs_dulookup_start(dul, dnp, ctx);
		}
	}

	                                                       
	nfs_getattr(dnp, NULL, ctx, NGA_CACHED);
	if (!namedattrs) {
		nfs_dulookup_finish(dul, dnp, ctx);
	}
out:
	                     
	lck_mtx_lock(&nfs_node_hash_mutex);
	np->n_hflag &= ~NHLOCKED;
	if (np->n_hflag & NHLOCKWANT) {
		np->n_hflag &= ~NHLOCKWANT;
		wakeup(np);
	}
	lck_mtx_unlock(&nfs_node_hash_mutex);
	nfs_node_clear_busy2(dnp, np);
	if (setsize) {
		ubc_setsize(vp, 0);
	}
out_free:
	FREE(dul, M_TEMP);
	FREE(nvattr, M_TEMP);
	return error;
}

  
                                                                        
   
int
nfs_removeit(struct nfs_sillyrename *nsp)
{
	struct nfsmount *nmp = NFSTONMP(nsp->nsr_dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	return nmp->nm_funcs->nf_remove_rpc(nsp->nsr_dnp, nsp->nsr_name, nsp->nsr_namlen, NULL, nsp->nsr_cred);
}

  
                                                               
   
int
nfs3_remove_rpc(
	nfsnode_t dnp,
	char *name,
	int namelen,
	thread_t thd,
	kauth_cred_t cred)
{
	int error = 0, lockerror = ENOENT, status = 0, wccpostattr = 0;
	struct timespec premtime = { .tv_sec = 0, .tv_nsec = 0 };
	struct nfsmount *nmp;
	int nfsvers;
	u_int64_t xid;
	struct nfsm_chain nmreq, nmrep;

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if ((nfsvers == NFS_VER2) && (namelen > NFS_MAXNAMLEN)) {
		return ENAMETOOLONG;
	}

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq,
	    NFSX_FH(nfsvers) + NFSX_UNSIGNED + nfsm_rndup(namelen));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	nfsm_chain_add_name(error, &nmreq, name, namelen, nmp);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);

	error = nfs_request2(dnp, NULL, &nmreq, NFSPROC_REMOVE, thd, cred, NULL, 0, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	if (nfsvers == NFS_VER3) {
		nfsm_chain_get_wcc_data(error, &nmrep, dnp, &premtime, &wccpostattr, &xid);
	}
	nfsmout_if(error);
	dnp->n_flag |= NMODIFIED;
	                                                         
	if (nfstimespeccmp(&dnp->n_ncmtime, &premtime, ==)) {
		NFS_CHANGED_UPDATE_NC(nfsvers, dnp, &dnp->n_vattr);
	}
	if (!wccpostattr) {
		NATTRINVALIDATE(dnp);
	}
	if (!error) {
		error = status;
	}
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(dnp);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

  
                       
   
int
nfs_vnop_rename(
	struct vnop_rename_args      
                                                                 
                                                    
                                                   
                                                                  
                                                    
                                                   
                                                                  
                                                             
                                         *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t fdvp = ap->a_fdvp;
	vnode_t fvp = ap->a_fvp;
	vnode_t tdvp = ap->a_tdvp;
	vnode_t tvp = ap->a_tvp;
	nfsnode_t fdnp, fnp, tdnp, tnp;
	struct componentname *tcnp = ap->a_tcnp;
	struct componentname *fcnp = ap->a_fcnp;
	int error, nfsvers, inuse = 0, tvprecycle = 0, locked = 0;
	mount_t fmp, tdmp, tmp;
	struct nfs_vattr *nvattr;
	struct nfsmount *nmp;

	fdnp = VTONFS(fdvp);
	fnp = VTONFS(fvp);
	tdnp = VTONFS(tdvp);
	tnp = tvp ? VTONFS(tvp) : NULL;

	nmp = NFSTONMP(fdnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	error = nfs_node_set_busy4(fdnp, fnp, tdnp, tnp, vfs_context_thread(ctx));
	if (error) {
		return error;
	}

	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);

	if (tvp && (tvp != fvp)) {
		                                                          
		lck_mtx_lock(&nfs_node_hash_mutex);
		while (tnp->n_hflag & NHLOCKED) {
			tnp->n_hflag |= NHLOCKWANT;
			msleep(tnp, &nfs_node_hash_mutex, PINOD, "nfs_rename", NULL);
		}
		tnp->n_hflag |= NHLOCKED;
		lck_mtx_unlock(&nfs_node_hash_mutex);
		locked = 1;
	}

	                                   
	fmp = vnode_mount(fvp);
	tmp = tvp ? vnode_mount(tvp) : NULL;
	tdmp = vnode_mount(tdvp);
	if ((fmp != tdmp) || (tvp && (fmp != tmp))) {
		error = EXDEV;
		goto out;
	}

	                                                         

	  
                                                                    
                                   
                                      
                                                               
                           
    
	if (tvp && (tvp != fvp)) {
		inuse = vnode_isinuse(tvp, 0);
	}
	if (inuse && !tnp->n_sillyrename && (vnode_vtype(tvp) != VDIR)) {
		error = nfs_sillyrename(tdnp, tnp, tcnp, ctx);
		if (error) {
			                                                              
			goto out;                            
		} else {
			                           
			tvp = NULL;
		}
	}
#if CONFIG_NFS4
	else if (tvp && (nmp->nm_vers >= NFS_VER4) && (tnp->n_openflags & N_DELEG_MASK)) {
		nfs4_delegation_return(tnp, 0, vfs_context_thread(ctx), vfs_context_ucred(ctx));
	}
#endif
	error = nmp->nm_funcs->nf_rename_rpc(fdnp, fcnp->cn_nameptr, fcnp->cn_namelen,
	    tdnp, tcnp->cn_nameptr, tcnp->cn_namelen, ctx);

	  
                                                                   
    
	if (error == ENOENT) {
		error = 0;
	}

	if (tvp && (tvp != fvp) && !tnp->n_sillyrename) {
		nfs_node_lock_force(tnp);
		tvprecycle = (!error && !vnode_isinuse(tvp, 0) &&
		    (nfs_getattrcache(tnp, nvattr, 0) || (nvattr->nva_nlink == 1)));
		nfs_node_unlock(tnp);
		lck_mtx_lock(&nfs_node_hash_mutex);
		if (tvprecycle && (tnp->n_hflag & NHHASHED)) {
			  
                                                                   
                                                                   
                                      
      
			LIST_REMOVE(tnp, n_hash);
			tnp->n_hflag &= ~NHHASHED;
			FSDBG(266, 0, tnp, tnp->n_flag, 0xb1eb1e);
		}
		lck_mtx_unlock(&nfs_node_hash_mutex);
	}

	                                                            
	nfs_name_cache_purge(fdnp, fnp, fcnp, ctx);
	if (tvp) {
		nfs_name_cache_purge(tdnp, tnp, tcnp, ctx);
		if (tvprecycle) {
			                                                                     
			                                      
			nfs_node_lock_force(tnp);
			tnp->n_flag &= (NMODIFIED);
			nfs_node_unlock(tnp);
			vnode_recycle(tvp);
		}
	}
	if (!error) {
		nfs_node_lock_force(tdnp);
		if (tdnp->n_flag & NNEGNCENTRIES) {
			tdnp->n_flag &= ~NNEGNCENTRIES;
			cache_purge_negatives(tdvp);
		}
		nfs_node_unlock(tdnp);
		nfs_node_lock_force(fnp);
		cache_enter(tdvp, fvp, tcnp);
		if (tdvp != fdvp) {                                
			if (fnp->n_parent && !vnode_get(fnp->n_parent)) {
				                                
				vnode_rele(fnp->n_parent);
				vnode_put(fnp->n_parent);
			}
			fnp->n_parent = tdvp;
			if (tdvp && !vnode_get(tdvp)) {
				                           
				vnode_ref(tdvp);
				vnode_put(tdvp);
			} else {
				fnp->n_parent = NULL;
			}
		}
		nfs_node_unlock(fnp);
	}
out:
	                                                       
	nfs_getattr(fdnp, NULL, ctx, NGA_CACHED);
	nfs_getattr(tdnp, NULL, ctx, NGA_CACHED);
	if (locked) {
		                 
		lck_mtx_lock(&nfs_node_hash_mutex);
		tnp->n_hflag &= ~NHLOCKED;
		if (tnp->n_hflag & NHLOCKWANT) {
			tnp->n_hflag &= ~NHLOCKWANT;
			wakeup(tnp);
		}
		lck_mtx_unlock(&nfs_node_hash_mutex);
	}
	nfs_node_clear_busy4(fdnp, fnp, tdnp, tnp);
	FREE(nvattr, M_TEMP);
	return error;
}

  
                                                                             
   
int
nfs3_rename_rpc(
	nfsnode_t fdnp,
	char *fnameptr,
	int fnamelen,
	nfsnode_t tdnp,
	char *tnameptr,
	int tnamelen,
	vfs_context_t ctx)
{
	int error = 0, lockerror = ENOENT, status = 0, fwccpostattr = 0, twccpostattr = 0;
	struct timespec fpremtime = { .tv_sec = 0, .tv_nsec = 0 }, tpremtime = { .tv_sec = 0, .tv_nsec = 0 };
	struct nfsmount *nmp;
	int nfsvers;
	u_int64_t xid, txid;
	struct nfsm_chain nmreq, nmrep;

	nmp = NFSTONMP(fdnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if ((nfsvers == NFS_VER2) &&
	    ((fnamelen > NFS_MAXNAMLEN) || (tnamelen > NFS_MAXNAMLEN))) {
		return ENAMETOOLONG;
	}

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq,
	    (NFSX_FH(nfsvers) + NFSX_UNSIGNED) * 2 +
	    nfsm_rndup(fnamelen) + nfsm_rndup(tnamelen));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, fdnp->n_fhp, fdnp->n_fhsize);
	nfsm_chain_add_name(error, &nmreq, fnameptr, fnamelen, nmp);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, tdnp->n_fhp, tdnp->n_fhsize);
	nfsm_chain_add_name(error, &nmreq, tnameptr, tnamelen, nmp);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);

	error = nfs_request(fdnp, NULL, &nmreq, NFSPROC_RENAME, ctx, NULL, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock2(fdnp, tdnp))) {
		error = lockerror;
	}
	if (nfsvers == NFS_VER3) {
		txid = xid;
		nfsm_chain_get_wcc_data(error, &nmrep, fdnp, &fpremtime, &fwccpostattr, &xid);
		nfsm_chain_get_wcc_data(error, &nmrep, tdnp, &tpremtime, &twccpostattr, &txid);
	}
	if (!error) {
		error = status;
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	if (!lockerror) {
		fdnp->n_flag |= NMODIFIED;
		                                                         
		if (nfstimespeccmp(&fdnp->n_ncmtime, &fpremtime, ==)) {
			NFS_CHANGED_UPDATE_NC(nfsvers, fdnp, &fdnp->n_vattr);
		}
		if (!fwccpostattr) {
			NATTRINVALIDATE(fdnp);
		}
		tdnp->n_flag |= NMODIFIED;
		                                                         
		if (nfstimespeccmp(&tdnp->n_ncmtime, &tpremtime, ==)) {
			NFS_CHANGED_UPDATE_NC(nfsvers, tdnp, &tdnp->n_vattr);
		}
		if (!twccpostattr) {
			NATTRINVALIDATE(tdnp);
		}
		nfs_node_unlock2(fdnp, tdnp);
	}
	return error;
}

  
                            
   
int
nfs3_vnop_link(
	struct vnop_link_args     
                                                              
                                               
                                                 
                                                              
                                                          
                                      *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	vnode_t tdvp = ap->a_tdvp;
	struct componentname *cnp = ap->a_cnp;
	int error = 0, lockerror = ENOENT, status = 0, wccpostattr = 0, attrflag = 0;
	struct timespec premtime = { .tv_sec = 0, .tv_nsec = 0 };
	struct nfsmount *nmp;
	nfsnode_t np = VTONFS(vp);
	nfsnode_t tdnp = VTONFS(tdvp);
	int nfsvers;
	u_int64_t xid, txid;
	struct nfsm_chain nmreq, nmrep;

	if (vnode_mount(vp) != vnode_mount(tdvp)) {
		return EXDEV;
	}

	nmp = VTONMP(vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if ((nfsvers == NFS_VER2) && (cnp->cn_namelen > NFS_MAXNAMLEN)) {
		return ENAMETOOLONG;
	}

	  
                                                              
                                              
                                     
    
	nfs_flush(np, MNT_WAIT, vfs_context_thread(ctx), V_IGNORE_WRITEERR);

	error = nfs_node_set_busy2(tdnp, np, vfs_context_thread(ctx));
	if (error) {
		return error;
	}

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq,
	    NFSX_FH(nfsvers) * 2 + NFSX_UNSIGNED + nfsm_rndup(cnp->cn_namelen));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, tdnp->n_fhp, tdnp->n_fhsize);
	nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);
	error = nfs_request(np, NULL, &nmreq, NFSPROC_LINK, ctx, NULL, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock2(tdnp, np))) {
		error = lockerror;
		goto nfsmout;
	}
	if (nfsvers == NFS_VER3) {
		txid = xid;
		nfsm_chain_postop_attr_update_flag(error, &nmrep, np, attrflag, &xid);
		nfsm_chain_get_wcc_data(error, &nmrep, tdnp, &premtime, &wccpostattr, &txid);
	}
	if (!error) {
		error = status;
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	if (!lockerror) {
		if (!attrflag) {
			NATTRINVALIDATE(np);
		}
		tdnp->n_flag |= NMODIFIED;
		                                                         
		if (nfstimespeccmp(&tdnp->n_ncmtime, &premtime, ==)) {
			NFS_CHANGED_UPDATE_NC(nfsvers, tdnp, &tdnp->n_vattr);
		}
		if (!wccpostattr) {
			NATTRINVALIDATE(tdnp);
		}
		if (!error && (tdnp->n_flag & NNEGNCENTRIES)) {
			tdnp->n_flag &= ~NNEGNCENTRIES;
			cache_purge_negatives(tdvp);
		}
		nfs_node_unlock2(tdnp, np);
	}
	nfs_node_clear_busy2(tdnp, np);
	  
                                                                   
    
	if (error == EEXIST) {
		error = 0;
	}
	return error;
}

  
                                
   
int
nfs3_vnop_symlink(
	struct vnop_symlink_args     
                                                                 
                                                   
                                                    
                                                                 
                                                              
                                                    
                                                             
                                         *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t dvp = ap->a_dvp;
	struct vnode_attr *vap = ap->a_vap;
	struct componentname *cnp = ap->a_cnp;
	struct nfs_vattr *nvattr;
	fhandle_t *fh;
	int error = 0, lockerror = ENOENT, busyerror = ENOENT, status = 0, wccpostattr = 0;
	size_t slen;
	struct timespec premtime = { .tv_sec = 0, .tv_nsec = 0 };
	vnode_t newvp = NULL;
	int nfsvers, gotuid, gotgid;
	u_int64_t xid = 0, dxid;
	nfsnode_t np = NULL;
	nfsnode_t dnp = VTONFS(dvp);
	struct nfsmount *nmp;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq *req;
	struct nfs_dulookup *dul;
	int namedattrs;
	int dul_in_progress = 0;

	nmp = VTONMP(dvp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	namedattrs = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR);

	slen = strlen(ap->a_target);
	if ((nfsvers == NFS_VER2) &&
	    ((cnp->cn_namelen > NFS_MAXNAMLEN) || (slen > NFS_MAXPATHLEN))) {
		return ENAMETOOLONG;
	}

	nfs_avoid_needless_id_setting_on_create(dnp, vap, ctx);

	VATTR_SET_SUPPORTED(vap, va_mode);
	VATTR_SET_SUPPORTED(vap, va_uid);
	VATTR_SET_SUPPORTED(vap, va_gid);
	VATTR_SET_SUPPORTED(vap, va_data_size);
	VATTR_SET_SUPPORTED(vap, va_access_time);
	VATTR_SET_SUPPORTED(vap, va_modify_time);
	gotuid = VATTR_IS_ACTIVE(vap, va_uid);
	gotgid = VATTR_IS_ACTIVE(vap, va_gid);

	fh = zalloc(nfs_fhandle_zone);
	req = zalloc_flags(nfs_req_zone, Z_WAITOK);
	MALLOC(dul, struct nfs_dulookup *, sizeof(*dul), M_TEMP, M_WAITOK);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);

	error = busyerror = nfs_node_set_busy(dnp, vfs_context_thread(ctx));
	if (!namedattrs) {
		nfs_dulookup_init(dul, dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx);
	}

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq,
	    NFSX_FH(nfsvers) + 2 * NFSX_UNSIGNED +
	    nfsm_rndup(cnp->cn_namelen) + nfsm_rndup(slen) + NFSX_SATTR(nfsvers));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	if (nfsvers == NFS_VER3) {
		nfsm_chain_add_v3sattr(nmp, error, &nmreq, vap);
	}
	nfsm_chain_add_name(error, &nmreq, ap->a_target, slen, nmp);
	if (nfsvers == NFS_VER2) {
		nfsm_chain_add_v2sattr(error, &nmreq, vap, -1);
	}
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);

	error = nfs_request_async(dnp, NULL, &nmreq, NFSPROC_SYMLINK,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), NULL, 0, NULL, &req);
	if (!error) {
		if (!namedattrs) {
			nfs_dulookup_start(dul, dnp, ctx);
			dul_in_progress = 1;
		}
		error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	}

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	dxid = xid;
	if (!error && !status) {
		if (dnp->n_flag & NNEGNCENTRIES) {
			dnp->n_flag &= ~NNEGNCENTRIES;
			cache_purge_negatives(dvp);
		}
		if (nfsvers == NFS_VER3) {
			error = nfsm_chain_get_fh_attr(nmp, &nmrep, dnp, ctx, nfsvers, &xid, fh, nvattr);
		} else {
			fh->fh_len = 0;
		}
	}
	if (nfsvers == NFS_VER3) {
		nfsm_chain_get_wcc_data(error, &nmrep, dnp, &premtime, &wccpostattr, &dxid);
	}
	if (!error) {
		error = status;
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	if (!lockerror) {
		dnp->n_flag |= NMODIFIED;
		                                                         
		if (nfstimespeccmp(&dnp->n_ncmtime, &premtime, ==)) {
			NFS_CHANGED_UPDATE_NC(nfsvers, dnp, &dnp->n_vattr);
		}
		nfs_node_unlock(dnp);
		                                                       
		nfs_getattr(dnp, NULL, ctx, wccpostattr ? NGA_CACHED : NGA_UNCACHED);
	}

	if (!error && fh->fh_len) {
		error = nfs_nget(NFSTOMP(dnp), dnp, cnp, fh->fh_data, fh->fh_len, nvattr, &xid, req->r_auth, NG_MAKEENTRY, &np);
	}
	if (!error && np) {
		newvp = NFSTOV(np);
	}

	if (dul_in_progress) {
		nfs_dulookup_finish(dul, dnp, ctx);
	}

	  
                                                                     
                                                
    
	if ((error == EEXIST) || (!error && !newvp)) {
		if (newvp) {
			nfs_node_unlock(np);
			vnode_put(newvp);
			newvp = NULL;
		}
		error = nfs_lookitup(dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx, &np);
		if (!error) {
			newvp = NFSTOV(np);
			if (vnode_vtype(newvp) != VLNK) {
				error = EEXIST;
			}
		}
	}
	if (!busyerror) {
		nfs_node_clear_busy(dnp);
	}
	if (!error && (gotuid || gotgid) &&
	    (!newvp || nfs_getattrcache(np, nvattr, 0) ||
	    (gotuid && (nvattr->nva_uid != vap->va_uid)) ||
	    (gotgid && (nvattr->nva_gid != vap->va_gid)))) {
		                                                                
		VATTR_CLEAR_SUPPORTED(vap, va_uid);
		VATTR_CLEAR_SUPPORTED(vap, va_gid);
	}
	if (error) {
		if (newvp) {
			nfs_node_unlock(np);
			vnode_put(newvp);
		}
	} else {
		nfs_node_unlock(np);
		*ap->a_vpp = newvp;
	}
	NFS_ZFREE(nfs_fhandle_zone, fh);
	NFS_ZFREE(nfs_req_zone, req);
	FREE(dul, M_TEMP);
	FREE(nvattr, M_TEMP);
	return error;
}

  
                    
   
int
nfs3_vnop_mkdir(
	struct vnop_mkdir_args     
                                                               
                                                 
                                                  
                                                               
                                                            
                                                           
                                       *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t dvp = ap->a_dvp;
	struct vnode_attr *vap = ap->a_vap;
	struct componentname *cnp = ap->a_cnp;
	struct nfs_vattr *nvattr;
	nfsnode_t np = NULL;
	struct nfsmount *nmp;
	nfsnode_t dnp = VTONFS(dvp);
	vnode_t newvp = NULL;
	int error = 0, lockerror = ENOENT, busyerror = ENOENT, status = 0, wccpostattr = 0;
	struct timespec premtime = { .tv_sec = 0, .tv_nsec = 0 };
	int nfsvers, gotuid, gotgid;
	u_int64_t xid = 0, dxid;
	fhandle_t *fh;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq *req;
	struct nfs_dulookup *dul;
	int namedattrs;
	int dul_in_progress = 0;

	nmp = VTONMP(dvp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	namedattrs = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR);

	if ((nfsvers == NFS_VER2) && (cnp->cn_namelen > NFS_MAXNAMLEN)) {
		return ENAMETOOLONG;
	}

	nfs_avoid_needless_id_setting_on_create(dnp, vap, ctx);

	VATTR_SET_SUPPORTED(vap, va_mode);
	VATTR_SET_SUPPORTED(vap, va_uid);
	VATTR_SET_SUPPORTED(vap, va_gid);
	VATTR_SET_SUPPORTED(vap, va_data_size);
	VATTR_SET_SUPPORTED(vap, va_access_time);
	VATTR_SET_SUPPORTED(vap, va_modify_time);
	gotuid = VATTR_IS_ACTIVE(vap, va_uid);
	gotgid = VATTR_IS_ACTIVE(vap, va_gid);

	fh = zalloc(nfs_fhandle_zone);
	req = zalloc_flags(nfs_req_zone, Z_WAITOK);
	MALLOC(dul, struct nfs_dulookup *, sizeof(*dul), M_TEMP, M_WAITOK);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);

	error = busyerror = nfs_node_set_busy(dnp, vfs_context_thread(ctx));
	if (!namedattrs) {
		nfs_dulookup_init(dul, dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx);
	}

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq,
	    NFSX_FH(nfsvers) + NFSX_UNSIGNED +
	    nfsm_rndup(cnp->cn_namelen) + NFSX_SATTR(nfsvers));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	if (nfsvers == NFS_VER3) {
		nfsm_chain_add_v3sattr(nmp, error, &nmreq, vap);
	} else {
		nfsm_chain_add_v2sattr(error, &nmreq, vap, -1);
	}
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);

	error = nfs_request_async(dnp, NULL, &nmreq, NFSPROC_MKDIR,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), NULL, 0, NULL, &req);
	if (!error) {
		if (!namedattrs) {
			nfs_dulookup_start(dul, dnp, ctx);
			dul_in_progress = 1;
		}
		error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	}

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	dxid = xid;
	if (!error && !status) {
		if (dnp->n_flag & NNEGNCENTRIES) {
			dnp->n_flag &= ~NNEGNCENTRIES;
			cache_purge_negatives(dvp);
		}
		error = nfsm_chain_get_fh_attr(nmp, &nmrep, dnp, ctx, nfsvers, &xid, fh, nvattr);
	}
	if (nfsvers == NFS_VER3) {
		nfsm_chain_get_wcc_data(error, &nmrep, dnp, &premtime, &wccpostattr, &dxid);
	}
	if (!error) {
		error = status;
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	if (!lockerror) {
		dnp->n_flag |= NMODIFIED;
		                                                         
		if (nfstimespeccmp(&dnp->n_ncmtime, &premtime, ==)) {
			NFS_CHANGED_UPDATE_NC(nfsvers, dnp, &dnp->n_vattr);
		}
		nfs_node_unlock(dnp);
		                                                       
		nfs_getattr(dnp, NULL, ctx, wccpostattr ? NGA_CACHED : NGA_UNCACHED);
	}

	if (!error && fh->fh_len) {
		error = nfs_nget(NFSTOMP(dnp), dnp, cnp, fh->fh_data, fh->fh_len, nvattr, &xid, req->r_auth, NG_MAKEENTRY, &np);
	}
	if (!error && np) {
		newvp = NFSTOV(np);
	}

	if (dul_in_progress) {
		nfs_dulookup_finish(dul, dnp, ctx);
	}

	  
                                                                     
                                                  
    
	if ((error == EEXIST) || (!error && !newvp)) {
		if (newvp) {
			nfs_node_unlock(np);
			vnode_put(newvp);
			newvp = NULL;
		}
		error = nfs_lookitup(dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx, &np);
		if (!error) {
			newvp = NFSTOV(np);
			if (vnode_vtype(newvp) != VDIR) {
				error = EEXIST;
			}
		}
	}
	if (!busyerror) {
		nfs_node_clear_busy(dnp);
	}
	if (!error && (gotuid || gotgid) &&
	    (!newvp || nfs_getattrcache(np, nvattr, 0) ||
	    (gotuid && (nvattr->nva_uid != vap->va_uid)) ||
	    (gotgid && (nvattr->nva_gid != vap->va_gid)))) {
		                                                                
		VATTR_CLEAR_SUPPORTED(vap, va_uid);
		VATTR_CLEAR_SUPPORTED(vap, va_gid);
	}
	if (error) {
		if (newvp) {
			nfs_node_unlock(np);
			vnode_put(newvp);
		}
	} else {
		nfs_node_unlock(np);
		*ap->a_vpp = newvp;
	}
	NFS_ZFREE(nfs_fhandle_zone, fh);
	NFS_ZFREE(nfs_req_zone, req);
	FREE(dul, M_TEMP);
	FREE(nvattr, M_TEMP);
	return error;
}

  
                            
   
int
nfs3_vnop_rmdir(
	struct vnop_rmdir_args     
                                                               
                                                 
                                                
                                                               
                                                           
                                       *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	vnode_t dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	int error = 0, lockerror = ENOENT, status = 0, wccpostattr = 0;
	struct timespec premtime = { .tv_sec = 0, .tv_nsec = 0 };
	struct nfsmount *nmp;
	nfsnode_t np = VTONFS(vp);
	nfsnode_t dnp = VTONFS(dvp);
	int nfsvers;
	u_int64_t xid;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq *req;
	struct nfs_dulookup *dul;
	int namedattrs;
	int dul_in_progress = 0;

	nmp = VTONMP(vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	namedattrs = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR);

	if ((nfsvers == NFS_VER2) && (cnp->cn_namelen > NFS_MAXNAMLEN)) {
		return ENAMETOOLONG;
	}

	if ((error = nfs_node_set_busy2(dnp, np, vfs_context_thread(ctx)))) {
		return error;
	}

	req = zalloc_flags(nfs_req_zone, Z_WAITOK);
	MALLOC(dul, struct nfs_dulookup *, sizeof(*dul), M_TEMP, M_WAITOK);

	if (!namedattrs) {
		nfs_dulookup_init(dul, dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx);
	}

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq,
	    NFSX_FH(nfsvers) + NFSX_UNSIGNED + nfsm_rndup(cnp->cn_namelen));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);

	error = nfs_request_async(dnp, NULL, &nmreq, NFSPROC_RMDIR,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), NULL, 0, NULL, &req);
	if (!error) {
		if (!namedattrs) {
			nfs_dulookup_start(dul, dnp, ctx);
			dul_in_progress = 1;
		}
		error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	}

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	if (nfsvers == NFS_VER3) {
		nfsm_chain_get_wcc_data(error, &nmrep, dnp, &premtime, &wccpostattr, &xid);
	}
	if (!error) {
		error = status;
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	if (!lockerror) {
		dnp->n_flag |= NMODIFIED;
		                                                         
		if (nfstimespeccmp(&dnp->n_ncmtime, &premtime, ==)) {
			NFS_CHANGED_UPDATE_NC(nfsvers, dnp, &dnp->n_vattr);
		}
		nfs_node_unlock(dnp);
		nfs_name_cache_purge(dnp, np, cnp, ctx);
		                                                       
		nfs_getattr(dnp, NULL, ctx, wccpostattr ? NGA_CACHED : NGA_UNCACHED);
	}
	if (dul_in_progress) {
		nfs_dulookup_finish(dul, dnp, ctx);
	}
	nfs_node_clear_busy2(dnp, np);

	  
                                                                      
    
	if (error == ENOENT) {
		error = 0;
	}
	if (!error) {
		  
                                                                  
                                                                  
                                     
     
		lck_mtx_lock(&nfs_node_hash_mutex);
		if (np->n_hflag & NHHASHED) {
			LIST_REMOVE(np, n_hash);
			np->n_hflag &= ~NHHASHED;
			FSDBG(266, 0, np, np->n_flag, 0xb1eb1e);
		}
		lck_mtx_unlock(&nfs_node_hash_mutex);
	}
	NFS_ZFREE(nfs_req_zone, req);
	FREE(dul, M_TEMP);
	return error;
}

  
                   
  
                                                                      
                                                                       
                                                                     
                            
  
                                                                       
                                                                     
                                                                     
                                                                     
                                                                 
  
                                                                         
                                                            
  
                                                                      
                                                               
                                                                  
                                       
   
int
nfs_vnop_readdir(
	struct vnop_readdir_args     
                                                                 
                                                  
                                                       
                                                 
                                                    
                                                      
                                                             
                                         *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t dvp = ap->a_vp;
	nfsnode_t dnp = VTONFS(dvp);
	struct nfsmount *nmp;
	uio_t uio = ap->a_uio;
	int error, nfsvers, extended, numdirent, bigcookies, ptc, done;
	long attrcachetimeout;
	uint16_t i, iptc, rlen, nlen;
	uint64_t cookie, nextcookie, lbn = 0;
	struct nfsbuf *bp = NULL;
	struct nfs_dir_buf_header *ndbhp;
	struct direntry *dp, *dpptc;
	struct dirent dent;
	char *cp = NULL;
	struct timeval now;
	thread_t thd;

	nmp = VTONMP(dvp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	bigcookies = (nmp->nm_state & NFSSTA_BIGCOOKIES);
	extended = (ap->a_flags & VNODE_READDIR_EXTENDED);

	if (vnode_vtype(dvp) != VDIR) {
		return EPERM;
	}

	if (ap->a_eofflag) {
		*ap->a_eofflag = 0;
	}

	if (uio_resid(uio) == 0) {
		return 0;
	}
#if CONFIG_NFS4
	if ((nfsvers >= NFS_VER4) && (dnp->n_vattr.nva_flags & NFS_FFLAG_TRIGGER)) {
		                                                              
		return 0;
	}
#endif
	thd = vfs_context_thread(ctx);
	numdirent = done = 0;
	nextcookie = uio_offset(uio);
	ptc = bigcookies && NFS_DIR_COOKIE_POTENTIALLY_TRUNCATED(nextcookie);

	if ((error = nfs_node_lock(dnp))) {
		goto out;
	}

	if (dnp->n_flag & NNEEDINVALIDATE) {
		dnp->n_flag &= ~NNEEDINVALIDATE;
		nfs_invaldir(dnp);
		nfs_node_unlock(dnp);
		error = nfs_vinvalbuf(dvp, 0, ctx, 1);
		if (!error) {
			error = nfs_node_lock(dnp);
		}
		if (error) {
			goto out;
		}
	}

	if (dnp->n_rdirplusstamp_eof && dnp->n_rdirplusstamp_sof) {
		attrcachetimeout = nfs_attrcachetimeout(dnp);
		microuptime(&now);
		if (attrcachetimeout && (now.tv_sec - dnp->n_rdirplusstamp_sof > attrcachetimeout - 1)) {
			dnp->n_rdirplusstamp_eof = dnp->n_rdirplusstamp_sof = 0;
			nfs_invaldir(dnp);
			nfs_node_unlock(dnp);
			error = nfs_vinvalbuf(dvp, 0, ctx, 1);
			if (!error) {
				error = nfs_node_lock(dnp);
			}
			if (error) {
				goto out;
			}
		}
	}

	  
                                                               
    
	if (!nextcookie) {
		if (dnp->n_flag & NMODIFIED) {
			nfs_invaldir(dnp);
			nfs_node_unlock(dnp);
			if ((error = nfs_vinvalbuf(dvp, 0, ctx, 1))) {
				goto out;
			}
		} else {
			nfs_node_unlock(dnp);
		}
		                                                       
		if ((error = nfs_getattr(dnp, NULL, ctx, NGA_UNCACHED))) {
			goto out;
		}
	} else {
		nfs_node_unlock(dnp);
	}

	error = nfs_dir_cookie_to_lbn(dnp, nextcookie, &ptc, &lbn);
	if (error) {
		if (error < 0) {                          
			done = 1;
			error = 0;
		}
		if (ap->a_eofflag) {
			*ap->a_eofflag = 1;
		}
	}

	while (!error && !done) {
		OSAddAtomic64(1, &nfsstats.biocache_readdirs);
		cookie = nextcookie;
getbuffer:
		error = nfs_buf_get(dnp, lbn, NFS_DIRBLKSIZ, thd, NBLK_READ, &bp);
		if (error) {
			goto out;
		}
		ndbhp = (struct nfs_dir_buf_header*)bp->nb_data;
		if (!ISSET(bp->nb_flags, NB_CACHE) || !ISSET(ndbhp->ndbh_flags, NDB_FULL)) {
			if (!ISSET(bp->nb_flags, NB_CACHE)) {                            
				ndbhp->ndbh_flags = 0;
				ndbhp->ndbh_count = 0;
				ndbhp->ndbh_entry_end = sizeof(*ndbhp);
				ndbhp->ndbh_ncgen = dnp->n_ncgen;
			}
			error = nfs_buf_readdir(bp, ctx);
			if (error == NFSERR_DIRBUFDROPPED) {
				goto getbuffer;
			}
			if (error) {
				nfs_buf_release(bp, 1);
			}
			if (error && (error != ENXIO) && (error != ETIMEDOUT) && (error != EINTR) && (error != ERESTART)) {
				if (!nfs_node_lock(dnp)) {
					nfs_invaldir(dnp);
					nfs_node_unlock(dnp);
				}
				nfs_vinvalbuf(dvp, 0, ctx, 1);
				if (error == NFSERR_BAD_COOKIE) {
					error = ENOENT;
				}
			}
			if (error) {
				goto out;
			}
		}

		                               
		dp = NFS_DIR_BUF_FIRST_DIRENTRY(bp);
		i = 0;
		if ((lbn != cookie) && !(ptc && NFS_DIR_COOKIE_SAME32(lbn, cookie))) {
			dpptc = NULL;
			iptc = 0;
			for (; (i < ndbhp->ndbh_count) && (cookie != dp->d_seekoff); i++) {
				if (ptc && !dpptc && NFS_DIR_COOKIE_SAME32(cookie, dp->d_seekoff)) {
					iptc = i;
					dpptc = dp;
				}
				nextcookie = dp->d_seekoff;
				dp = NFS_DIRENTRY_NEXT(dp);
			}
			if ((i == ndbhp->ndbh_count) && dpptc) {
				i = iptc;
				dp = dpptc;
			}
			if (i < ndbhp->ndbh_count) {
				nextcookie = dp->d_seekoff;
				dp = NFS_DIRENTRY_NEXT(dp);
				i++;
			}
		}
		ptc = 0;                                                  

		                                      
		for (; i < ndbhp->ndbh_count; i++) {
			if (extended) {
				rlen = dp->d_reclen;
				cp = (char*)dp;
			} else {
				if (!cp) {
					cp = (char*)&dent;
					bzero(cp, sizeof(dent));
				}
				if (dp->d_namlen > (sizeof(dent.d_name) - 1)) {
					nlen = sizeof(dent.d_name) - 1;
				} else {
					nlen = dp->d_namlen;
				}
				rlen = NFS_DIRENT_LEN(nlen);
				dent.d_reclen = rlen;
				dent.d_ino = (ino_t)dp->d_ino;
				dent.d_type = dp->d_type;
				dent.d_namlen = (uint8_t)nlen;
				strlcpy(dent.d_name, dp->d_name, nlen + 1);
			}
			                                
			if (rlen > uio_resid(uio)) {
				done = 1;
				break;
			}
			if ((error = uiomove(cp, rlen, uio))) {
				break;
			}
			numdirent++;
			nextcookie = dp->d_seekoff;
			dp = NFS_DIRENTRY_NEXT(dp);
		}

		if (i == ndbhp->ndbh_count) {
			                                            
			lbn = nextcookie;
			                                    
			if (ISSET(ndbhp->ndbh_flags, NDB_EOF)) {
				done = 1;
				if (ap->a_eofflag) {
					*ap->a_eofflag = 1;
				}
			}
		}
		if (!error) {
			uio_setoffset(uio, nextcookie);
		}
		if (!error && !done && (nextcookie == cookie)) {
			printf("nfs readdir cookie didn't change 0x%llx, %d/%d\n", cookie, i, ndbhp->ndbh_count);
			error = EIO;
		}
		nfs_buf_release(bp, 1);
	}

	if (!error) {
		nfs_dir_cookie_cache(dnp, nextcookie, lbn);
	}

	if (ap->a_numdirent) {
		*ap->a_numdirent = numdirent;
	}
out:
	return error;
}


  
                                                                           
                                             
   
static void
nfs_invaldir_cookies(nfsnode_t dnp)
{
	if (vnode_vtype(NFSTOV(dnp)) != VDIR) {
		return;
	}
	dnp->n_eofcookie = 0;
	dnp->n_cookieverf = 0;
	if (!dnp->n_cookiecache) {
		return;
	}
	dnp->n_cookiecache->free = 0;
	dnp->n_cookiecache->mru = -1;
	memset(dnp->n_cookiecache->next, -1, NFSNUMCOOKIES);
}

void
nfs_invaldir(nfsnode_t dnp)
{

	nfs_invaldir_cookies(dnp);
}

  
                                                                          
   
uint64_t
nfs_dir_buf_freespace(struct nfsbuf *bp, int rdirplus)
{
	struct nfs_dir_buf_header *ndbhp = (struct nfs_dir_buf_header*)bp->nb_data;
	uint64_t space;

	if (!ndbhp) {
		return 0;
	}
	space = bp->nb_bufsize - ndbhp->ndbh_entry_end;
	if (rdirplus) {
		space -= ndbhp->ndbh_count * sizeof(struct nfs_vattr);
	}
	return space;
}

  
                                                               
   
void
nfs_dir_cookie_cache(nfsnode_t dnp, uint64_t cookie, uint64_t lbn)
{
	struct nfsdmap *ndcc;
	int8_t i, prev;

	if (!cookie) {
		return;
	}

	if (nfs_node_lock(dnp)) {
		return;
	}

	if (cookie == dnp->n_eofcookie) {                 
		nfs_node_unlock(dnp);
		return;
	}

	ndcc = dnp->n_cookiecache;
	if (!ndcc) {
		                                         
		ndcc = dnp->n_cookiecache = zalloc(ZV_NFSDIROFF);
		ndcc->free = 0;
		ndcc->mru = -1;
		memset(ndcc->next, -1, NFSNUMCOOKIES);
	}

	  
                                    
                                            
    
	prev = -1;
	i = ndcc->mru;
	while ((i != -1) && (cookie != ndcc->cookies[i].key)) {
		if (ndcc->next[i] == -1) {                                         
			break;
		}
		prev = i;
		i = ndcc->next[i];
	}
	if ((i != -1) && (cookie == ndcc->cookies[i].key)) {
		                                
		if (prev != -1) {
			ndcc->next[prev] = ndcc->next[i];
		} else {
			ndcc->mru = ndcc->next[i];
		}
	} else {
		                                                        
		if (ndcc->free != NFSNUMCOOKIES) {
			i = ndcc->free++;
		} else {
			ndcc->next[prev] = -1;
		}
		ndcc->cookies[i].key = cookie;
		ndcc->cookies[i].lbn = lbn;
	}
	                                       
	ndcc->next[i] = ndcc->mru;
	ndcc->mru = i;
	nfs_node_unlock(dnp);
}

  
                                                                            
                                                                              
   
int
nfs_dir_cookie_to_lbn(nfsnode_t dnp, uint64_t cookie, int *ptc, uint64_t *lbnp)
{
	struct nfsdmap *ndcc = dnp->n_cookiecache;
	int8_t eofptc, found;
	int i, iptc;
	struct nfsmount *nmp;
	struct nfsbuf *bp, *lastbp;
	struct nfsbuflists blist;
	struct direntry *dp, *dpptc;
	struct nfs_dir_buf_header *ndbhp;

	if (!cookie) {                      
		*lbnp = 0;
		*ptc = 0;
		return 0;
	}

	if (nfs_node_lock(dnp)) {
		return ENOENT;
	}

	if (cookie == dnp->n_eofcookie) {                 
		nfs_node_unlock(dnp);
		OSAddAtomic64(1, &nfsstats.direofcache_hits);
		*ptc = 0;
		return -1;
	}
	                                                          
	eofptc = *ptc ? NFS_DIR_COOKIE_SAME32(cookie, dnp->n_eofcookie) : 0;
	iptc = -1;

	                                    
	for (i = ndcc ? ndcc->mru : -1; i >= 0; i = ndcc->next[i]) {
		if (ndcc->cookies[i].key == cookie) {
			                                   
			*lbnp = ndcc->cookies[i].lbn;
			nfs_node_unlock(dnp);
			OSAddAtomic64(1, &nfsstats.direofcache_hits);
			*ptc = 0;
			return 0;
		}
		                            
		if (*ptc && (iptc == -1) && NFS_DIR_COOKIE_SAME32(ndcc->cookies[i].key, cookie)) {
			iptc = i;
		}
	}
	                           
	if (eofptc) {
		                                         
		nfs_node_unlock(dnp);
		OSAddAtomic64(1, &nfsstats.direofcache_hits);
		return -1;
	}
	if (iptc >= 0) {
		                                
		*lbnp = ndcc->cookies[iptc].lbn;
		nfs_node_unlock(dnp);
		OSAddAtomic64(1, &nfsstats.direofcache_hits);
		return 0;
	}
	nfs_node_unlock(dnp);

	  
                                                
                                                        
    
	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	dpptc = NULL;
	found = 0;

	lck_mtx_lock(&nfs_buf_mutex);
	  
                                                    
                                                                
                                                                
                                                                  
                                      
                                                                 
                                
    
	if (!nfs_buf_iterprepare(dnp, &blist, NBI_CLEAN)) {
		lastbp = NULL;
		while ((bp = LIST_FIRST(&blist))) {
			LIST_REMOVE(bp, nb_vnbufs);
			if (!lastbp) {
				LIST_INSERT_HEAD(&dnp->n_cleanblkhd, bp, nb_vnbufs);
			} else {
				LIST_INSERT_AFTER(lastbp, bp, nb_vnbufs);
			}
			lastbp = bp;
			if (found) {
				continue;
			}
			nfs_buf_refget(bp);
			if (nfs_buf_acquire(bp, NBAC_NOWAIT, 0, 0)) {
				                           
				nfs_buf_refrele(bp);
				continue;
			}
			nfs_buf_refrele(bp);

			                                    
			ndbhp = (struct nfs_dir_buf_header*)bp->nb_data;
			dp = NFS_DIR_BUF_FIRST_DIRENTRY(bp);
			dpptc = NULL;
			for (i = 0; (i < ndbhp->ndbh_count) && (cookie != dp->d_seekoff); i++) {
				if (*ptc && !dpptc && NFS_DIR_COOKIE_SAME32(cookie, dp->d_seekoff)) {
					dpptc = dp;
					iptc = i;
				}
				dp = NFS_DIRENTRY_NEXT(dp);
			}
			if ((i == ndbhp->ndbh_count) && dpptc) {
				                            
				dp = dpptc;
				i = iptc;
			} else if (i < ndbhp->ndbh_count) {
				*ptc = 0;
			}
			if (i < (ndbhp->ndbh_count - 1)) {
				                                                       
				*lbnp = bp->nb_lblkno;
				found = 1;
			} else if (i == (ndbhp->ndbh_count - 1)) {
				                                                           
				*lbnp = dp->d_seekoff;
				found = 1;
			}
			nfs_buf_drop(bp);
		}
		nfs_buf_itercomplete(dnp, &blist, NBI_CLEAN);
	}
	lck_mtx_unlock(&nfs_buf_mutex);
	if (found) {
		OSAddAtomic64(1, &nfsstats.direofcache_hits);
		return 0;
	}

	                                                        
	*lbnp = cookie;
	OSAddAtomic64(1, &nfsstats.direofcache_misses);
	return 0;
}

  
                                             
                                                                   
                                                              
   

#define NDBS_PURGE      1
#define NDBS_UPDATE     2

int
nfs_dir_buf_search(
	struct nfsbuf *bp,
	struct componentname *cnp,
	fhandle_t *fhp,
	struct nfs_vattr *nvap,
	uint64_t *xidp,
	time_t *attrstampp,
	daddr64_t *nextlbnp,
	int flags)
{
	struct direntry *dp;
	struct nfs_dir_buf_header *ndbhp;
	struct nfs_vattr *nvattrp;
	daddr64_t nextlbn = 0;
	int i, error = ESRCH;
	uint32_t fhlen;

	                                  
	ndbhp = (struct nfs_dir_buf_header*)bp->nb_data;
	dp = NFS_DIR_BUF_FIRST_DIRENTRY(bp);
	for (i = 0; i < ndbhp->ndbh_count; i++) {
		nextlbn = dp->d_seekoff;
		if ((cnp->cn_namelen == dp->d_namlen) && !strcmp(cnp->cn_nameptr, dp->d_name)) {
			fhlen = (uint8_t)dp->d_name[dp->d_namlen + 1];
			nvattrp = NFS_DIR_BUF_NVATTR(bp, i);
			if ((ndbhp->ndbh_ncgen != bp->nb_np->n_ncgen) || (fhlen == 0) ||
			    (nvattrp->nva_type == VNON) || (nvattrp->nva_fileid == 0)) {
				                        
				error = ENOENT;
				break;
			}
			if (flags == NDBS_PURGE) {
				dp->d_fileno = 0;
				bzero(nvattrp, sizeof(*nvattrp));
				error = ENOENT;
				break;
			}
			if (flags == NDBS_UPDATE) {
				                                           
				if ((fhp->fh_len == fhlen) && !bcmp(&dp->d_name[dp->d_namlen + 2], fhp->fh_data, fhlen)) {
					bcopy(nvap, nvattrp, sizeof(*nvap));
					dp->d_fileno = nvattrp->nva_fileid;
					nvattrp->nva_fileid = *xidp;
					nvap->nva_flags |= NFS_FFLAG_FILEID_CONTAINS_XID;
					*(time_t*)(&dp->d_name[dp->d_namlen + 2 + fhp->fh_len]) = *attrstampp;
				}
				error = 0;
				break;
			}
			                                            
			fhp->fh_len = fhlen;
			bcopy(&dp->d_name[dp->d_namlen + 2], fhp->fh_data, MAX(fhp->fh_len, (int)sizeof(fhp->fh_data)));
			*attrstampp = *(time_t*)(&dp->d_name[dp->d_namlen + 2 + fhp->fh_len]);
			bcopy(nvattrp, nvap, sizeof(*nvap));
			*xidp = nvap->nva_fileid;
			nvap->nva_fileid = dp->d_fileno;
			nvap->nva_flags &= ~NFS_FFLAG_FILEID_CONTAINS_XID;
			error = 0;
			break;
		}
		dp = NFS_DIRENTRY_NEXT(dp);
	}
	if (nextlbnp) {
		*nextlbnp = nextlbn;
	}
	return error;
}

  
                                           
                                                              
   
int
nfs_dir_buf_cache_lookup(nfsnode_t dnp, nfsnode_t *npp, struct componentname *cnp, vfs_context_t ctx, int purge, int *skipdu)
{
	nfsnode_t newnp;
	struct nfsmount *nmp;
	int error = 0, i, found = 0, count = 0;
	u_int64_t xid;
	struct nfs_vattr *nvattr;
	fhandle_t *fh;
	time_t attrstamp = 0;
	thread_t thd = vfs_context_thread(ctx);
	struct nfsbuf *bp, *lastbp, *foundbp;
	struct nfsbuflists blist;
	daddr64_t lbn, nextlbn;
	int dotunder = (cnp->cn_namelen > 2) && (cnp->cn_nameptr[0] == '.') && (cnp->cn_nameptr[1] == '_');
	int isdot = (cnp->cn_namelen == 1) && (cnp->cn_nameptr[0] == '.');
	int isdotdot = (cnp->cn_namelen == 2) && (cnp->cn_nameptr[0] == '.') && (cnp->cn_nameptr[1] == '.');
	int eof = 0, sof = 0, skipped = 0;

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (!purge) {
		*npp = NULL;
	}

	if (isdot || isdotdot) {
		return 0;
	}

	fh = zalloc(nfs_fhandle_zone);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);

	                                                       
	lbn = dnp->n_lastdbl;
	for (i = 0; i < 2; i++) {
		if ((error = nfs_buf_get(dnp, lbn, NFS_DIRBLKSIZ, thd, NBLK_READ | NBLK_ONLYVALID, &bp))) {
			goto out;
		}
		if (!bp) {
			skipped = 1;
			break;
		}
		count++;
		nfs_dir_buf_cache_lookup_boundaries(bp, &sof, &eof);
		error = nfs_dir_buf_search(bp, cnp, fh, nvattr, &xid, &attrstamp, &nextlbn, purge ? NDBS_PURGE : 0);
		nfs_buf_release(bp, 0);
		if (error == ESRCH) {
			error = 0;
		} else {
			found = 1;
			break;
		}
		lbn = nextlbn;
	}

	lck_mtx_lock(&nfs_buf_mutex);
	if (found) {
		dnp->n_lastdbl = lbn;
		goto done;
	}

	                                                                                                      
	if (dotunder && !found && !error && eof && sof && !skipped && skipdu) {
		*skipdu = 1;
	}

	  
                                                    
                                                                
                                                                
                                                                  
                                      
                                                                 
                                
    
	eof = sof = skipped = 0;
	if (!nfs_buf_iterprepare(dnp, &blist, NBI_CLEAN)) {
		lastbp = foundbp = NULL;
		while ((bp = LIST_FIRST(&blist))) {
			LIST_REMOVE(bp, nb_vnbufs);
			if (!lastbp) {
				LIST_INSERT_HEAD(&dnp->n_cleanblkhd, bp, nb_vnbufs);
			} else {
				LIST_INSERT_AFTER(lastbp, bp, nb_vnbufs);
			}
			lastbp = bp;
			if (error || found) {
				skipped = 1;
				continue;
			}
			if (!purge && dotunder && (count > 100)) {                                                     
				skipped = 1;
				continue;
			}
			nfs_buf_refget(bp);
			lbn = bp->nb_lblkno;
			if (nfs_buf_acquire(bp, NBAC_NOWAIT, 0, 0)) {
				                           
				nfs_buf_refrele(bp);
				skipped = 1;
				continue;
			}
			nfs_buf_refrele(bp);
			count++;
			nfs_dir_buf_cache_lookup_boundaries(bp, &sof, &eof);
			error = nfs_dir_buf_search(bp, cnp, fh, nvattr, &xid, &attrstamp, NULL, purge ? NDBS_PURGE : 0);
			if (error == ESRCH) {
				error = 0;
			} else {
				found = 1;
				foundbp = bp;
			}
			nfs_buf_drop(bp);
		}
		if (found) {
			LIST_REMOVE(foundbp, nb_vnbufs);
			LIST_INSERT_HEAD(&dnp->n_cleanblkhd, foundbp, nb_vnbufs);
			dnp->n_lastdbl = foundbp->nb_lblkno;
		}
		nfs_buf_itercomplete(dnp, &blist, NBI_CLEAN);
	}

	                                                                                                      
	if (dotunder && !found && !error && eof && sof && !skipped && skipdu) {
		*skipdu = 1;
	}

done:
	lck_mtx_unlock(&nfs_buf_mutex);

	if (!error && found && !purge) {
		error = nfs_nget(NFSTOMP(dnp), dnp, cnp, fh->fh_data,
		    fh->fh_len, nvattr, &xid, dnp->n_auth, NG_MAKEENTRY,
		    &newnp);
		if (error) {
			goto out;
		}
		newnp->n_attrstamp = attrstamp;
		*npp = newnp;
		nfs_node_unlock(newnp);
		                                                     
		if (!nfs_getattr(newnp, nvattr, ctx, NGA_CACHED) &&
		    (newnp->n_attrstamp != attrstamp)) {
			                              
			error = nfs_buf_get(dnp, lbn, NFS_DIRBLKSIZ, thd, NBLK_READ | NBLK_ONLYVALID, &bp);
			if (!error && bp) {
				attrstamp = newnp->n_attrstamp;
				xid = newnp->n_xid;
				nfs_dir_buf_search(bp, cnp, fh, nvattr, &xid, &attrstamp, NULL, NDBS_UPDATE);
				nfs_buf_release(bp, 0);
			}
			error = 0;
		}
	}

out:
	NFS_ZFREE(nfs_fhandle_zone, fh);
	FREE(nvattr, M_TEMP);
	return error;
}

  
                                               
                                                                      
   
void
nfs_name_cache_purge(nfsnode_t dnp, nfsnode_t np, struct componentname *cnp, vfs_context_t ctx)
{
	struct nfsmount *nmp = NFSTONMP(dnp);

	cache_purge(NFSTOV(np));
	if (nmp && (nmp->nm_vers > NFS_VER2) && NMFLAG(nmp, RDIRPLUS)) {
		nfs_dir_buf_cache_lookup(dnp, NULL, cnp, ctx, 1, NULL);
	}
}

  
                             
   
int
nfs3_readdir_rpc(nfsnode_t dnp, struct nfsbuf *bp, vfs_context_t ctx)
{
	struct nfsmount *nmp;
	int error = 0, lockerror, nfsvers, rdirplus, bigcookies;
	int i, status = 0, attrflag, fhflag, more_entries = 1, eof, bp_dropped = 0;
	uint32_t nmreaddirsize, nmrsize;
	uint32_t namlen, skiplen, fhlen, xlen, attrlen;
	uint64_t cookie, lastcookie, xid, savedxid, fileno, space_free, space_needed;
	struct nfsm_chain nmreq, nmrep, nmrepsave;
	fhandle_t *fh;
	struct nfs_vattr *nvattrp;
	struct nfs_dir_buf_header *ndbhp;
	struct direntry *dp;
	char *padstart;
	struct timeval now;
	uint16_t reclen;
	size_t padlen;

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	nmreaddirsize = nmp->nm_readdirsize;
	nmrsize = nmp->nm_rsize;
	bigcookies = nmp->nm_state & NFSSTA_BIGCOOKIES;
	fh = zalloc(nfs_fhandle_zone);
resend:
	rdirplus = ((nfsvers > NFS_VER2) && NMFLAG(nmp, RDIRPLUS)) ? 1 : 0;

	if ((lockerror = nfs_node_lock(dnp))) {
		NFS_ZFREE(nfs_fhandle_zone, fh);
		return lockerror;
	}

	                                                              
	ndbhp = (struct nfs_dir_buf_header*)bp->nb_data;
	dp = NFS_DIR_BUF_FIRST_DIRENTRY(bp);
	if (ndbhp->ndbh_count) {
		for (i = 0; i < ndbhp->ndbh_count - 1; i++) {
			dp = NFS_DIRENTRY_NEXT(dp);
		}
		cookie = dp->d_seekoff;
		dp = NFS_DIRENTRY_NEXT(dp);
	} else {
		cookie = bp->nb_lblkno;
		                                      
		OSAddAtomic64(1, &nfsstats.readdir_bios);
	}
	lastcookie = cookie;

	  
                                                                     
                                                                      
                                  
    
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);
	while (nfs_dir_buf_freespace(bp, rdirplus) && !(ndbhp->ndbh_flags & NDB_FULL)) {
		nfsm_chain_build_alloc_init(error, &nmreq,
		    NFSX_FH(nfsvers) + NFSX_READDIR(nfsvers) + NFSX_UNSIGNED);
		nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
		if (nfsvers == NFS_VER3) {
			                                                    
			                                                    
			nfsm_chain_add_64(error, &nmreq, cookie);
			nfsm_chain_add_64(error, &nmreq, dnp->n_cookieverf);
		} else {
			nfsm_chain_add_32(error, &nmreq, cookie);
		}
		nfsm_chain_add_32(error, &nmreq, nmreaddirsize);
		if (rdirplus) {
			nfsm_chain_add_32(error, &nmreq, nmrsize);
		}
		nfsm_chain_build_done(error, &nmreq);
		nfs_node_unlock(dnp);
		lockerror = ENOENT;
		nfsmout_if(error);

		error = nfs_request(dnp, NULL, &nmreq,
		    rdirplus ? NFSPROC_READDIRPLUS : NFSPROC_READDIR,
		    ctx, NULL, &nmrep, &xid, &status);

		if ((lockerror = nfs_node_lock(dnp))) {
			error = lockerror;
		}

		savedxid = xid;
		if (nfsvers == NFS_VER3) {
			nfsm_chain_postop_attr_update(error, &nmrep, dnp, &xid);
		}
		if (!error) {
			error = status;
		}
		if (nfsvers == NFS_VER3) {
			nfsm_chain_get_64(error, &nmrep, dnp->n_cookieverf);
		}
		nfsm_chain_get_32(error, &nmrep, more_entries);

		if (!lockerror) {
			nfs_node_unlock(dnp);
			lockerror = ENOENT;
		}
		if (error == NFSERR_NOTSUPP) {
			                                                           
			lck_mtx_lock(&nmp->nm_lock);
			NFS_BITMAP_CLR(nmp->nm_flags, NFS_MFLAG_RDIRPLUS);
			lck_mtx_unlock(&nmp->nm_lock);
			nfsm_chain_cleanup(&nmreq);
			nfsm_chain_cleanup(&nmrep);
			goto resend;
		}
		nfsmout_if(error);

		if (rdirplus) {
			microuptime(&now);
			if (lastcookie == 0) {
				dnp->n_rdirplusstamp_sof = now.tv_sec;
				dnp->n_rdirplusstamp_eof = 0;
			}
		}

		                                                           
		while (more_entries) {
			if (nfsvers == NFS_VER3) {
				nfsm_chain_get_64(error, &nmrep, fileno);
			} else {
				nfsm_chain_get_32(error, &nmrep, fileno);
			}
			nfsm_chain_get_32(error, &nmrep, namlen);
			nfsmout_if(error);
			                                                           
			if (namlen <= 0) {
				error = EBADRPC;
				goto nfsmout;
			}
			if (namlen > (sizeof(dp->d_name) - 1)) {
				skiplen = namlen - sizeof(dp->d_name) + 1;
				namlen = sizeof(dp->d_name) - 1;
			} else {
				skiplen = 0;
			}
			                                               
			fhlen = rdirplus ? (1 + dnp->n_fhsize) : 0;
			xlen = rdirplus ? (fhlen + sizeof(time_t)) : 0;
			attrlen = rdirplus ? sizeof(struct nfs_vattr) : 0;
			reclen = NFS_DIRENTRY_LEN_16(namlen + xlen);
			space_needed = reclen + attrlen;
			space_free = nfs_dir_buf_freespace(bp, rdirplus);
			if (space_needed > space_free) {
				  
                                               
                                             
                                             
                                            
                                         
       
nextbuffer:
				ndbhp->ndbh_flags |= NDB_FULL;
				nfs_buf_release(bp, 0);
				bp_dropped = 1;
				bp = NULL;
				error = nfs_buf_get(dnp, lastcookie, NFS_DIRBLKSIZ, vfs_context_thread(ctx), NBLK_READ, &bp);
				nfsmout_if(error);
				                       
				ndbhp = (struct nfs_dir_buf_header*)bp->nb_data;
				ndbhp->ndbh_flags = 0;
				ndbhp->ndbh_count = 0;
				ndbhp->ndbh_entry_end = sizeof(*ndbhp);
				ndbhp->ndbh_ncgen = dnp->n_ncgen;
				space_free = nfs_dir_buf_freespace(bp, rdirplus);
				dp = NFS_DIR_BUF_FIRST_DIRENTRY(bp);
				                                      
				OSAddAtomic64(1, &nfsstats.readdir_bios);
			}
			nmrepsave = nmrep;
			dp->d_fileno = fileno;
			dp->d_namlen = (uint16_t)namlen;
			dp->d_reclen = reclen;
			dp->d_type = DT_UNKNOWN;
			nfsm_chain_get_opaque(error, &nmrep, namlen, dp->d_name);
			nfsmout_if(error);
			dp->d_name[namlen] = '\0';
			if (skiplen) {
				nfsm_chain_adv(error, &nmrep,
				    nfsm_rndup(namlen + skiplen) - nfsm_rndup(namlen));
			}
			if (nfsvers == NFS_VER3) {
				nfsm_chain_get_64(error, &nmrep, cookie);
			} else {
				nfsm_chain_get_32(error, &nmrep, cookie);
			}
			nfsmout_if(error);
			dp->d_seekoff = cookie;
			if (!bigcookies && (cookie >> 32) && (nmp == NFSTONMP(dnp))) {
				                                                   
				lck_mtx_lock(&nmp->nm_lock);
				nmp->nm_state |= NFSSTA_BIGCOOKIES;
				lck_mtx_unlock(&nmp->nm_lock);
				bigcookies = 1;
			}
			if (rdirplus) {
				nvattrp = NFS_DIR_BUF_NVATTR(bp, ndbhp->ndbh_count);
				                          
				nfsm_chain_get_32(error, &nmrep, attrflag);
				nfsmout_if(error);
				if (attrflag) {
					                     
					error = nfs_parsefattr(nmp, &nmrep, NFS_VER3, nvattrp);
					nfsmout_if(error);
					dp->d_type = IFTODT(VTTOIF(nvattrp->nva_type));
					                                                          
					nvattrp->nva_fileid = savedxid;
					nvattrp->nva_flags |= NFS_FFLAG_FILEID_CONTAINS_XID;
				} else {
					                                 
					bzero(nvattrp, sizeof(struct nfs_vattr));
				}
				                           
				nfsm_chain_get_32(error, &nmrep, fhflag);
				nfsmout_if(error);
				if (fhflag) {
					nfsm_chain_get_fh(error, &nmrep, NFS_VER3, fh);
					nfsmout_if(error);
					fhlen = fh->fh_len + 1;
					xlen = fhlen + sizeof(time_t);
					reclen = NFS_DIRENTRY_LEN_16(namlen + xlen);
					space_needed = reclen + attrlen;
					if (space_needed > space_free) {
						                                                             
						nmrep = nmrepsave;
						goto nextbuffer;
					}
					                                          
					dp->d_name[dp->d_namlen + 1] = (unsigned char)fh->fh_len;                                                                                 
					bcopy(fh->fh_data, &dp->d_name[dp->d_namlen + 2], fh->fh_len);
				} else {
					                                  
					fh->fh_len = 0;
					fhlen = fh->fh_len + 1;
					xlen = fhlen + sizeof(time_t);
					reclen = NFS_DIRENTRY_LEN_16(namlen + xlen);
					bzero(&dp->d_name[dp->d_namlen + 1], fhlen);
				}
				*(time_t*)(&dp->d_name[dp->d_namlen + 1 + fhlen]) = now.tv_sec;
				dp->d_reclen = reclen;
				nfs_rdirplus_update_node_attrs(dnp, dp, fh, nvattrp, &savedxid);
			}
			padstart = dp->d_name + dp->d_namlen + 1 + xlen;
			ndbhp->ndbh_count++;
			lastcookie = cookie;
			                                        
			dp = NFS_DIRENTRY_NEXT(dp);
			ndbhp->ndbh_entry_end = (char*)dp - bp->nb_data;
			                            
			padlen = (char*)dp - padstart;
			if (padlen > 0) {
				bzero(padstart, padlen);
			}
			                            
			nfsm_chain_get_32(error, &nmrep, more_entries);
			nfsmout_if(error);
		}
		                                  
		nfsm_chain_get_32(error, &nmrep, eof);
		nfsmout_if(error);
		if (eof) {
			ndbhp->ndbh_flags |= (NDB_FULL | NDB_EOF);
			nfs_node_lock_force(dnp);
			dnp->n_eofcookie = lastcookie;
			if (rdirplus) {
				dnp->n_rdirplusstamp_eof = now.tv_sec;
			}
			nfs_node_unlock(dnp);
		} else {
			more_entries = 1;
		}
		if (bp_dropped) {
			nfs_buf_release(bp, 0);
			bp = NULL;
			break;
		}
		if ((lockerror = nfs_node_lock(dnp))) {
			error = lockerror;
		}
		nfsmout_if(error);
		nfsm_chain_cleanup(&nmrep);
		nfsm_chain_null(&nmreq);
	}
nfsmout:
	if (bp_dropped && bp) {
		nfs_buf_release(bp, 0);
	}
	if (!lockerror) {
		nfs_node_unlock(dnp);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	return bp_dropped ? NFSERR_DIRBUFDROPPED : error;
}

  
                                                                           
                                                                            
                                                                          
                                                                            
                                                                          
                             
   

                                                                
                                                               
#define NFS_SILLYNAME_FORMAT ".nfs.%08x.%04x"
                                           
static uint32_t nfs_sillyrename_number = 0x20051025;

int
nfs_sillyrename(
	nfsnode_t dnp,
	nfsnode_t np,
	struct componentname *cnp,
	vfs_context_t ctx)
{
	struct nfs_sillyrename *nsp;
	int error;
	pid_t pid;
	kauth_cred_t cred;
	uint32_t num;
	struct nfsmount *nmp;

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	nfs_name_cache_purge(dnp, np, cnp, ctx);

	MALLOC(nsp, struct nfs_sillyrename *,
	    sizeof(struct nfs_sillyrename), M_TEMP, M_WAITOK);
	if (!nsp) {
		return ENOMEM;
	}
	cred = vfs_context_ucred(ctx);
	kauth_cred_ref(cred);
	nsp->nsr_cred = cred;
	nsp->nsr_dnp = dnp;
	error = vnode_ref(NFSTOV(dnp));
	if (error) {
		goto bad_norele;
	}

	                                 
	pid = vfs_context_pid(ctx);
	num = OSAddAtomic(1, &nfs_sillyrename_number);
	nsp->nsr_namlen = snprintf(nsp->nsr_name, sizeof(nsp->nsr_name),
	    NFS_SILLYNAME_FORMAT, num, (pid & 0xffff));
	if (nsp->nsr_namlen >= (int)sizeof(nsp->nsr_name)) {
		nsp->nsr_namlen = sizeof(nsp->nsr_name) - 1;
	}

	                                                     
	while (nfs_lookitup(dnp, nsp->nsr_name, nsp->nsr_namlen, ctx, NULL) == 0) {
		num = OSAddAtomic(1, &nfs_sillyrename_number);
		nsp->nsr_namlen = snprintf(nsp->nsr_name, sizeof(nsp->nsr_name),
		    NFS_SILLYNAME_FORMAT, num, (pid & 0xffff));
		if (nsp->nsr_namlen >= (int)sizeof(nsp->nsr_name)) {
			nsp->nsr_namlen = sizeof(nsp->nsr_name) - 1;
		}
	}

	                        
	error = nmp->nm_funcs->nf_rename_rpc(dnp, cnp->cn_nameptr, cnp->cn_namelen,
	    dnp, nsp->nsr_name, nsp->nsr_namlen, ctx);

	                                                                     
	if (error == ENOENT) {
		error = 0;
	}
	if (!error) {
		nfs_node_lock_force(dnp);
		if (dnp->n_flag & NNEGNCENTRIES) {
			dnp->n_flag &= ~NNEGNCENTRIES;
			cache_purge_negatives(NFSTOV(dnp));
		}
		nfs_node_unlock(dnp);
	}
	FSDBG(267, dnp, np, num, error);
	if (error) {
		goto bad;
	}
	error = nfs_lookitup(dnp, nsp->nsr_name, nsp->nsr_namlen, ctx, &np);
	nfs_node_lock_force(np);
	np->n_sillyrename = nsp;
	nfs_node_unlock(np);
	return 0;
bad:
	vnode_rele(NFSTOV(dnp));
bad_norele:
	nsp->nsr_cred = NOCRED;
	kauth_cred_unref(&cred);
	FREE(nsp, M_TEMP);
	return error;
}

int
nfs3_lookup_rpc_async(
	nfsnode_t dnp,
	char *name,
	int namelen,
	vfs_context_t ctx,
	struct nfsreq **reqp)
{
	struct nfsmount *nmp;
	struct nfsm_chain nmreq;
	int error = 0, nfsvers;

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	nfsm_chain_null(&nmreq);

	nfsm_chain_build_alloc_init(error, &nmreq,
	    NFSX_FH(nfsvers) + NFSX_UNSIGNED + nfsm_rndup(namelen));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	nfsm_chain_add_name(error, &nmreq, name, namelen, nmp);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);
	error = nfs_request_async(dnp, NULL, &nmreq, NFSPROC_LOOKUP,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), NULL, 0, NULL, reqp);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	return error;
}

int
nfs3_lookup_rpc_async_finish(
	nfsnode_t dnp,
	__unused char *name,
	__unused int namelen,
	vfs_context_t ctx,
	struct nfsreq *req,
	u_int64_t *xidp,
	fhandle_t *fhp,
	struct nfs_vattr *nvap)
{
	int error = 0, lockerror = ENOENT, status = 0, nfsvers, attrflag;
	u_int64_t xid;
	struct nfsmount *nmp;
	struct nfsm_chain nmrep;

	nmp = NFSTONMP(dnp);
	if (nmp == NULL) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	nfsm_chain_null(&nmrep);

	error = nfs_request_async_finish(req, &nmrep, xidp, &status);

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	xid = *xidp;
	if (error || status) {
		if (nfsvers == NFS_VER3) {
			nfsm_chain_postop_attr_update(error, &nmrep, dnp, &xid);
		}
		if (!error) {
			error = status;
		}
		goto nfsmout;
	}

	nfsmout_if(error || !fhp || !nvap);

	                         
	nfsm_chain_get_fh(error, &nmrep, nfsvers, fhp);

	                        
	if (nfsvers == NFS_VER3) {
		nfsm_chain_postop_attr_get(nmp, error, &nmrep, attrflag, nvap);
		nfsm_chain_postop_attr_update(error, &nmrep, dnp, &xid);
		if (!error && !attrflag) {
			error = nfs3_getattr_rpc(NULL, NFSTOMP(dnp), fhp->fh_data, fhp->fh_len, 0, ctx, nvap, xidp);
		}
	} else {
		error = nfs_parsefattr(nmp, &nmrep, nfsvers, nvap);
	}
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(dnp);
	}
	nfsm_chain_cleanup(&nmrep);
	return error;
}

  
                                                                      
                                                      
                                     
                                                                       
                
                                                       
   
int
nfs_lookitup(
	nfsnode_t dnp,
	char *name,
	int namelen,
	vfs_context_t ctx,
	nfsnode_t *npp)
{
	int error = 0;
	nfsnode_t np, newnp = NULL;
	u_int64_t xid;
	fhandle_t *fh;
	struct nfsmount *nmp;
	struct nfs_vattr *nvattr;
	struct nfsreq *req;

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (NFS_BITMAP_ISSET(nmp->nm_fsattr.nfsa_bitmap, NFS_FATTR_MAXNAME) &&
	    (namelen > nmp->nm_fsattr.nfsa_maxname)) {
		return ENAMETOOLONG;
	}

	fh = zalloc(nfs_fhandle_zone);
	req = zalloc_flags(nfs_req_zone, Z_WAITOK);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	NVATTR_INIT(nvattr);

	                             
	if ((name[0] == '.') && (namelen == 1)) {
		                                     
		fh->fh_len = 0;
		newnp = dnp;
		goto nfsmout;
	}

	error = nmp->nm_funcs->nf_lookup_rpc_async(dnp, name, namelen, ctx, &req);
	nfsmout_if(error);
	error = nmp->nm_funcs->nf_lookup_rpc_async_finish(dnp, name, namelen, ctx, req, &xid, fh, nvattr);
	nfsmout_if(!npp || error);

	if (*npp) {
		np = *npp;
		if (fh->fh_len != np->n_fhsize) {
			u_char *oldbuf = (np->n_fhsize > NFS_SMALLFH) ? np->n_fhp : NULL;
			if (fh->fh_len > NFS_SMALLFH) {
				MALLOC(np->n_fhp, u_char *, fh->fh_len, M_NFSBIGFH, M_WAITOK);
				if (!np->n_fhp) {
					np->n_fhp = oldbuf;
					error = ENOMEM;
					goto nfsmout;
				}
			} else {
				np->n_fhp = &np->n_fh[0];
			}
			if (oldbuf) {
				FREE(oldbuf, M_NFSBIGFH);
			}
		}
		bcopy(fh->fh_data, np->n_fhp, fh->fh_len);
		np->n_fhsize = fh->fh_len;
		nfs_node_lock_force(np);
		error = nfs_loadattrcache(np, nvattr, &xid, 0);
		nfs_node_unlock(np);
		nfsmout_if(error);
		newnp = np;
	} else if (NFS_CMPFH(dnp, fh->fh_data, fh->fh_len)) {
		nfs_node_lock_force(dnp);
		if (dnp->n_xid <= xid) {
			error = nfs_loadattrcache(dnp, nvattr, &xid, 0);
		}
		nfs_node_unlock(dnp);
		nfsmout_if(error);
		newnp = dnp;
	} else {
		struct componentname cn, *cnp = &cn;
		bzero(cnp, sizeof(*cnp));
		cnp->cn_nameptr = name;
		cnp->cn_namelen = namelen;
		error = nfs_nget(NFSTOMP(dnp), dnp, cnp, fh->fh_data, fh->fh_len,
		    nvattr, &xid, req->r_auth, NG_MAKEENTRY, &np);
		nfsmout_if(error);
		newnp = np;
	}

nfsmout:
	if (npp && !*npp && !error) {
		*npp = newnp;
	}
	NVATTR_CLEANUP(nvattr);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	NFS_ZFREE(nfs_req_zone, req);
	FREE(nvattr, M_TEMP);
	return error;
}

  
                                                              
                            
   
void
nfs_dulookup_init(struct nfs_dulookup *dulp, nfsnode_t dnp, const char *name, int namelen, vfs_context_t ctx)
{
	int error, du_namelen;
	vnode_t du_vp;
	struct nfsmount *nmp = NFSTONMP(dnp);

	                                     
	dulp->du_flags = 0;
	bzero(&dulp->du_cn, sizeof(dulp->du_cn));
	du_namelen = namelen + 2;
	if (!nmp || NMFLAG(nmp, NONEGNAMECACHE)) {
		return;
	}
	if ((namelen >= 2) && (name[0] == '.') && (name[1] == '_')) {
		return;
	}
	if (du_namelen >= (int)sizeof(dulp->du_smallname)) {
		MALLOC(dulp->du_cn.cn_nameptr, char *, du_namelen + 1, M_TEMP, M_WAITOK);
	} else {
		dulp->du_cn.cn_nameptr = dulp->du_smallname;
	}
	if (!dulp->du_cn.cn_nameptr) {
		return;
	}
	dulp->du_cn.cn_namelen = du_namelen;
	snprintf(dulp->du_cn.cn_nameptr, du_namelen + 1, "._%s", name);
	dulp->du_cn.cn_nameptr[du_namelen] = '\0';
	dulp->du_cn.cn_nameiop = LOOKUP;
	dulp->du_cn.cn_flags = MAKEENTRY;

	error = cache_lookup(NFSTOV(dnp), &du_vp, &dulp->du_cn);
	if (error == -1) {
		vnode_put(du_vp);
	} else if (!error) {
		nmp = NFSTONMP(dnp);
		if (nmp && (nmp->nm_vers > NFS_VER2) && NMFLAG(nmp, RDIRPLUS)) {
			                                           
			nfsnode_t du_np = NULL;
			if (!nfs_dir_buf_cache_lookup(dnp, &du_np, &dulp->du_cn, ctx, 0, NULL) && du_np) {
				                       
				du_vp = NFSTOV(du_np);
				vnode_put(du_vp);
				error = -1;
			}
		}
		if (!error) {
			dulp->du_flags |= NFS_DULOOKUP_DOIT;
		}
	}
}

  
                                          
   
void
nfs_dulookup_start(struct nfs_dulookup *dulp, nfsnode_t dnp, vfs_context_t ctx)
{
	struct nfsmount *nmp = NFSTONMP(dnp);
	struct nfsreq *req = &dulp->du_req;

	if (!nmp || !(dulp->du_flags & NFS_DULOOKUP_DOIT) || (dulp->du_flags & NFS_DULOOKUP_INPROG)) {
		return;
	}
	if (!nmp->nm_funcs->nf_lookup_rpc_async(dnp, dulp->du_cn.cn_nameptr,
	    dulp->du_cn.cn_namelen, ctx, &req)) {
		dulp->du_flags |= NFS_DULOOKUP_INPROG;
	}
}

  
                                                                      
   
void
nfs_dulookup_finish(struct nfs_dulookup *dulp, nfsnode_t dnp, vfs_context_t ctx)
{
	struct nfsmount *nmp = NFSTONMP(dnp);
	int error;
	nfsnode_t du_np;
	u_int64_t xid;
	fhandle_t *fh;
	struct nfs_vattr *nvattr;

	if (!nmp || !(dulp->du_flags & NFS_DULOOKUP_INPROG)) {
		goto out;
	}

	fh = zalloc(nfs_fhandle_zone);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	NVATTR_INIT(nvattr);
	error = nmp->nm_funcs->nf_lookup_rpc_async_finish(dnp, dulp->du_cn.cn_nameptr,
	    dulp->du_cn.cn_namelen, ctx, &dulp->du_req, &xid, fh, nvattr);
	dulp->du_flags &= ~NFS_DULOOKUP_INPROG;
	if (error == ENOENT) {
		                                            
		nfs_node_lock_force(dnp);
		cache_enter(NFSTOV(dnp), NULL, &dulp->du_cn);
		dnp->n_flag |= NNEGNCENTRIES;
		nfs_node_unlock(dnp);
	} else if (!error) {
		error = nfs_nget(NFSTOMP(dnp), dnp, &dulp->du_cn, fh->fh_data, fh->fh_len,
		    nvattr, &xid, dulp->du_req.r_auth, NG_MAKEENTRY, &du_np);
		if (!error) {
			nfs_node_unlock(du_np);
			vnode_put(NFSTOV(du_np));
		}
	}
	NVATTR_CLEANUP(nvattr);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	FREE(nvattr, M_TEMP);
out:
	if (dulp->du_flags & NFS_DULOOKUP_INPROG) {
		nfs_request_async_cancel(&dulp->du_req);
	}
	if (dulp->du_cn.cn_nameptr && (dulp->du_cn.cn_nameptr != dulp->du_smallname)) {
		FREE(dulp->du_cn.cn_nameptr, M_TEMP);
	}
}


  
                           
   
int
nfs3_commit_rpc(
	nfsnode_t np,
	uint64_t offset,
	uint64_t count,
	kauth_cred_t cred,
	uint64_t wverf)
{
	struct nfsmount *nmp;
	int error = 0, lockerror, status = 0, wccpostattr = 0, nfsvers;
	struct timespec premtime = { .tv_sec = 0, .tv_nsec = 0 };
	u_int64_t xid, newwverf;
	uint32_t count32;
	struct nfsm_chain nmreq, nmrep;

	nmp = NFSTONMP(np);
	FSDBG(521, np, offset, count, nmp ? nmp->nm_state : 0);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (!(nmp->nm_state & NFSSTA_HASWRITEVERF)) {
		return 0;
	}
	nfsvers = nmp->nm_vers;
	count32 = count > UINT32_MAX ? 0 : (uint32_t)count;

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	nfsm_chain_build_alloc_init(error, &nmreq, NFSX_FH(NFS_VER3));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	nfsm_chain_add_64(error, &nmreq, offset);
	nfsm_chain_add_32(error, &nmreq, count32);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);
	error = nfs_request2(np, NULL, &nmreq, NFSPROC_COMMIT,
	    current_thread(), cred, NULL, 0, &nmrep, &xid, &status);
	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	                                                  
	nfsm_chain_get_wcc_data(error, &nmrep, np, &premtime, &wccpostattr, &xid);
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	if (!error) {
		error = status;
	}
	nfsm_chain_get_64(error, &nmrep, newwverf);
	nfsmout_if(error);
	lck_mtx_lock(&nmp->nm_lock);
	if (nmp->nm_verf != newwverf) {
		nmp->nm_verf = newwverf;
	}
	if (wverf != newwverf) {
		error = NFSERR_STALEWRITEVERF;
	}
	lck_mtx_unlock(&nmp->nm_lock);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}


int
nfs_vnop_blockmap(
	__unused struct vnop_blockmap_args     
                                                                           
                                                            
                                                               
                                                             
                                                                
                                                             
                                                            
                                                           
                                                   *ap)
{
	return ENOTSUP;
}


  
                                         
   
              
int
nfs_vnop_fsync(
	struct vnop_fsync_args     
                                                               
                                                
                                                 
                                                           
                                       *ap)
{
	return nfs_flush(VTONFS(ap->a_vp), ap->a_waitfor, vfs_context_thread(ap->a_context), 0);
}


  
                          
   
int
nfs3_pathconf_rpc(
	nfsnode_t np,
	struct nfs_fsattr *nfsap,
	vfs_context_t ctx)
{
	u_int64_t xid;
	int error = 0, lockerror, status = 0, nfsvers;
	struct nfsm_chain nmreq, nmrep;
	struct nfsmount *nmp = NFSTONMP(np);
	uint32_t val = 0;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                                     
	nfsm_chain_build_alloc_init(error, &nmreq, NFSX_FH(NFS_VER3));
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	nfsm_chain_build_done(error, &nmreq);
	nfsmout_if(error);
	error = nfs_request(np, NULL, &nmreq, NFSPROC_PATHCONF, ctx, NULL, &nmrep, &xid, &status);
	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_postop_attr_update(error, &nmrep, np, &xid);
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	if (!error) {
		error = status;
	}
	nfsm_chain_get_32(error, &nmrep, nfsap->nfsa_maxlink);
	nfsm_chain_get_32(error, &nmrep, nfsap->nfsa_maxname);
	nfsap->nfsa_flags &= ~(NFS_FSFLAG_NO_TRUNC | NFS_FSFLAG_CHOWN_RESTRICTED | NFS_FSFLAG_CASE_INSENSITIVE | NFS_FSFLAG_CASE_PRESERVING);
	nfsm_chain_get_32(error, &nmrep, val);
	if (val) {
		nfsap->nfsa_flags |= NFS_FSFLAG_NO_TRUNC;
	}
	nfsm_chain_get_32(error, &nmrep, val);
	if (val) {
		nfsap->nfsa_flags |= NFS_FSFLAG_CHOWN_RESTRICTED;
	}
	nfsm_chain_get_32(error, &nmrep, val);
	if (val) {
		nfsap->nfsa_flags |= NFS_FSFLAG_CASE_INSENSITIVE;
	}
	nfsm_chain_get_32(error, &nmrep, val);
	if (val) {
		nfsap->nfsa_flags |= NFS_FSFLAG_CASE_PRESERVING;
	}
	NFS_BITMAP_SET(nfsap->nfsa_bitmap, NFS_FATTR_MAXLINK);
	NFS_BITMAP_SET(nfsap->nfsa_bitmap, NFS_FATTR_MAXNAME);
	NFS_BITMAP_SET(nfsap->nfsa_bitmap, NFS_FATTR_NO_TRUNC);
	NFS_BITMAP_SET(nfsap->nfsa_bitmap, NFS_FATTR_CHOWN_RESTRICTED);
	NFS_BITMAP_SET(nfsap->nfsa_bitmap, NFS_FATTR_CASE_INSENSITIVE);
	NFS_BITMAP_SET(nfsap->nfsa_bitmap, NFS_FATTR_CASE_PRESERVING);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

                                        
void
nfs3_pathconf_cache(struct nfsmount *nmp, struct nfs_fsattr *nfsap)
{
	nmp->nm_fsattr.nfsa_maxlink = nfsap->nfsa_maxlink;
	nmp->nm_fsattr.nfsa_maxname = nfsap->nfsa_maxname;
	nmp->nm_fsattr.nfsa_flags &= ~(NFS_FSFLAG_NO_TRUNC | NFS_FSFLAG_CHOWN_RESTRICTED | NFS_FSFLAG_CASE_INSENSITIVE | NFS_FSFLAG_CASE_PRESERVING);
	nmp->nm_fsattr.nfsa_flags |= nfsap->nfsa_flags & NFS_FSFLAG_NO_TRUNC;
	nmp->nm_fsattr.nfsa_flags |= nfsap->nfsa_flags & NFS_FSFLAG_CHOWN_RESTRICTED;
	nmp->nm_fsattr.nfsa_flags |= nfsap->nfsa_flags & NFS_FSFLAG_CASE_INSENSITIVE;
	nmp->nm_fsattr.nfsa_flags |= nfsap->nfsa_flags & NFS_FSFLAG_CASE_PRESERVING;
	NFS_BITMAP_SET(nmp->nm_fsattr.nfsa_bitmap, NFS_FATTR_MAXLINK);
	NFS_BITMAP_SET(nmp->nm_fsattr.nfsa_bitmap, NFS_FATTR_MAXNAME);
	NFS_BITMAP_SET(nmp->nm_fsattr.nfsa_bitmap, NFS_FATTR_NO_TRUNC);
	NFS_BITMAP_SET(nmp->nm_fsattr.nfsa_bitmap, NFS_FATTR_CHOWN_RESTRICTED);
	NFS_BITMAP_SET(nmp->nm_fsattr.nfsa_bitmap, NFS_FATTR_CASE_INSENSITIVE);
	NFS_BITMAP_SET(nmp->nm_fsattr.nfsa_bitmap, NFS_FATTR_CASE_PRESERVING);
	nmp->nm_state |= NFSSTA_GOTPATHCONF;
}

  
                                                       
  
                                                                  
          
   
              
int
nfs_vnop_pathconf(
	struct vnop_pathconf_args     
                                                                  
                                                   
                                                 
                                                        
                                                              
                                          *ap)
{
	vnode_t vp = ap->a_vp;
	nfsnode_t np = VTONFS(vp);
	struct nfsmount *nmp;
	struct nfs_fsattr nfsa, *nfsap;
	int error = 0;
	uint64_t maxFileSize;
	uint nbits;

	nmp = VTONMP(vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	switch (ap->a_name) {
	case _PC_LINK_MAX:
	case _PC_NAME_MAX:
	case _PC_CHOWN_RESTRICTED:
	case _PC_NO_TRUNC:
	case _PC_CASE_SENSITIVE:
	case _PC_CASE_PRESERVING:
		break;
	case _PC_FILESIZEBITS:
		if (nmp->nm_vers == NFS_VER2) {
			*ap->a_retval = 32;
			return 0;
		}
		break;
	case _PC_XATTR_SIZE_BITS:
		                                    
		if (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR) {
			break;           
		}
		                                   
		return EINVAL;
	default:
		                                                              
		return EINVAL;
	}

	if (nmp->nm_vers == NFS_VER2) {
		return EINVAL;
	}

	lck_mtx_lock(&nmp->nm_lock);
	if (nmp->nm_vers == NFS_VER3) {
		if (!(nmp->nm_state & NFSSTA_GOTPATHCONF) || (!(nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_HOMOGENEOUS) && nmp->nm_dnp != np)) {
			                                                                                                                    
			lck_mtx_unlock(&nmp->nm_lock);
			NFS_CLEAR_ATTRIBUTES(nfsa.nfsa_bitmap);
			error = nfs3_pathconf_rpc(np, &nfsa, ap->a_context);
			if (error) {
				return error;
			}
			nmp = VTONMP(vp);
			if (nfs_mount_gone(nmp)) {
				return ENXIO;
			}
			lck_mtx_lock(&nmp->nm_lock);
			if (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_HOMOGENEOUS) {
				                                            
				                                    
				nfs3_pathconf_cache(nmp, &nfsa);
			}
			nfsap = &nfsa;
		} else {
			nfsap = &nmp->nm_fsattr;
		}
	}
#if CONFIG_NFS4
	else if (!(nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_HOMOGENEOUS)) {
		                             
		lck_mtx_unlock(&nmp->nm_lock);
		NFS_CLEAR_ATTRIBUTES(nfsa.nfsa_bitmap);
		error = nfs4_pathconf_rpc(np, &nfsa, ap->a_context);
		if (error) {
			return error;
		}
		nmp = VTONMP(vp);
		if (nfs_mount_gone(nmp)) {
			return ENXIO;
		}
		lck_mtx_lock(&nmp->nm_lock);
		nfsap = &nfsa;
	}
#endif
	else {
		nfsap = &nmp->nm_fsattr;
	}
	switch (ap->a_name) {
	case _PC_LINK_MAX:
		if (NFS_BITMAP_ISSET(nfsap->nfsa_bitmap, NFS_FATTR_MAXLINK)) {
			*ap->a_retval = nfsap->nfsa_maxlink;
#if CONFIG_NFS4
		} else if ((nmp->nm_vers == NFS_VER4) && NFS_BITMAP_ISSET(np->n_vattr.nva_bitmap, NFS_FATTR_MAXLINK)) {
			*ap->a_retval = np->n_vattr.nva_maxlink;
#endif
		} else {
			error = EINVAL;
		}
		break;
	case _PC_NAME_MAX:
		if (NFS_BITMAP_ISSET(nfsap->nfsa_bitmap, NFS_FATTR_MAXNAME)) {
			*ap->a_retval = nfsap->nfsa_maxname;
		} else {
			error = EINVAL;
		}
		break;
	case _PC_CHOWN_RESTRICTED:
		if (NFS_BITMAP_ISSET(nfsap->nfsa_bitmap, NFS_FATTR_CHOWN_RESTRICTED)) {
			*ap->a_retval = (nfsap->nfsa_flags & NFS_FSFLAG_CHOWN_RESTRICTED) ? 200112                               : 0;
		} else {
			error = EINVAL;
		}
		break;
	case _PC_NO_TRUNC:
		if (NFS_BITMAP_ISSET(nfsap->nfsa_bitmap, NFS_FATTR_NO_TRUNC)) {
			*ap->a_retval = (nfsap->nfsa_flags & NFS_FSFLAG_NO_TRUNC) ? 200112                       : 0;
		} else {
			error = EINVAL;
		}
		break;
	case _PC_CASE_SENSITIVE:
		if (NFS_BITMAP_ISSET(nfsap->nfsa_bitmap, NFS_FATTR_CASE_INSENSITIVE)) {
			*ap->a_retval = (nfsap->nfsa_flags & NFS_FSFLAG_CASE_INSENSITIVE) ? 0 : 1;
		} else {
			error = EINVAL;
		}
		break;
	case _PC_CASE_PRESERVING:
		if (NFS_BITMAP_ISSET(nfsap->nfsa_bitmap, NFS_FATTR_CASE_PRESERVING)) {
			*ap->a_retval = (nfsap->nfsa_flags & NFS_FSFLAG_CASE_PRESERVING) ? 1 : 0;
		} else {
			error = EINVAL;
		}
		break;
	case _PC_XATTR_SIZE_BITS:                                                      
	case _PC_FILESIZEBITS:
		if (!NFS_BITMAP_ISSET(nfsap->nfsa_bitmap, NFS_FATTR_MAXFILESIZE)) {
			*ap->a_retval = 64;
			error = 0;
			break;
		}
		maxFileSize = nfsap->nfsa_maxfilesize;
		nbits = 1;
		if (maxFileSize & 0xffffffff00000000ULL) {
			nbits += 32;
			maxFileSize >>= 32;
		}
		if (maxFileSize & 0xffff0000) {
			nbits += 16;
			maxFileSize >>= 16;
		}
		if (maxFileSize & 0xff00) {
			nbits += 8;
			maxFileSize >>= 8;
		}
		if (maxFileSize & 0xf0) {
			nbits += 4;
			maxFileSize >>= 4;
		}
		if (maxFileSize & 0xc) {
			nbits += 2;
			maxFileSize >>= 2;
		}
		if (maxFileSize & 0x2) {
			nbits += 1;
		}
		*ap->a_retval = nbits;
		break;
	default:
		error = EINVAL;
	}

	lck_mtx_unlock(&nmp->nm_lock);

	return error;
}

  
                                    
   
int
nfsspec_vnop_read(
	struct vnop_read_args     
                                                              
                                               
                                                    
                                               
                                                          
                                      *ap)
{
	nfsnode_t np = VTONFS(ap->a_vp);
	struct timespec now;
	int error;

	  
                    
    
	if ((error = nfs_node_lock(np))) {
		return error;
	}
	np->n_flag |= NACC;
	nanotime(&now);
	np->n_atim.tv_sec = now.tv_sec;
	np->n_atim.tv_nsec = now.tv_nsec;
	nfs_node_unlock(np);
	return VOCALL(spec_vnodeop_p, VOFFSET(vnop_read), ap);
}

  
                                     
   
int
nfsspec_vnop_write(
	struct vnop_write_args     
                                                               
                                                
                                                     
                                                
                                                           
                                       *ap)
{
	nfsnode_t np = VTONFS(ap->a_vp);
	struct timespec now;
	int error;

	  
                    
    
	if ((error = nfs_node_lock(np))) {
		return error;
	}
	np->n_flag |= NUPD;
	nanotime(&now);
	np->n_mtim.tv_sec = now.tv_sec;
	np->n_mtim.tv_nsec = now.tv_nsec;
	nfs_node_unlock(np);
	return VOCALL(spec_vnodeop_p, VOFFSET(vnop_write), ap);
}

  
                                     
  
                                                        
   
int
nfsspec_vnop_close(
	struct vnop_close_args     
                                                               
                                                
                                               
                                                           
                                       *ap)
{
	vnode_t vp = ap->a_vp;
	nfsnode_t np = VTONFS(vp);
	struct vnode_attr vattr;
	mount_t mp;
	int error;

	if ((error = nfs_node_lock(np))) {
		return error;
	}
	if (np->n_flag & (NACC | NUPD)) {
		np->n_flag |= NCHG;
		if (!vnode_isinuse(vp, 0) && (mp = vnode_mount(vp)) && !vfs_isrdonly(mp)) {
			VATTR_INIT(&vattr);
			if (np->n_flag & NACC) {
				vattr.va_access_time = np->n_atim;
				VATTR_SET_ACTIVE(&vattr, va_access_time);
			}
			if (np->n_flag & NUPD) {
				vattr.va_modify_time = np->n_mtim;
				VATTR_SET_ACTIVE(&vattr, va_modify_time);
			}
			nfs_node_unlock(np);
			vnode_setattr(vp, &vattr, ap->a_context);
		} else {
			nfs_node_unlock(np);
		}
	} else {
		nfs_node_unlock(np);
	}
	return VOCALL(spec_vnodeop_p, VOFFSET(vnop_close), ap);
}

#if FIFO
extern vnop_t **fifo_vnodeop_p;

  
                          
   
int
nfsfifo_vnop_read(
	struct vnop_read_args     
                                                              
                                               
                                                    
                                               
                                                          
                                      *ap)
{
	nfsnode_t np = VTONFS(ap->a_vp);
	struct timespec now;
	int error;

	  
                    
    
	if ((error = nfs_node_lock(np))) {
		return error;
	}
	np->n_flag |= NACC;
	nanotime(&now);
	np->n_atim.tv_sec = now.tv_sec;
	np->n_atim.tv_nsec = now.tv_nsec;
	nfs_node_unlock(np);
	return VOCALL(fifo_vnodeop_p, VOFFSET(vnop_read), ap);
}

  
                           
   
int
nfsfifo_vnop_write(
	struct vnop_write_args     
                                                               
                                                
                                                     
                                                
                                                           
                                       *ap)
{
	nfsnode_t np = VTONFS(ap->a_vp);
	struct timespec now;
	int error;

	  
                    
    
	if ((error = nfs_node_lock(np))) {
		return error;
	}
	np->n_flag |= NUPD;
	nanotime(&now);
	np->n_mtim.tv_sec = now.tv_sec;
	np->n_mtim.tv_nsec = now.tv_nsec;
	nfs_node_unlock(np);
	return VOCALL(fifo_vnodeop_p, VOFFSET(vnop_write), ap);
}

  
                           
  
                                                      
   
int
nfsfifo_vnop_close(
	struct vnop_close_args     
                                                               
                                                
                                               
                                                           
                                       *ap)
{
	vnode_t vp = ap->a_vp;
	nfsnode_t np = VTONFS(vp);
	struct vnode_attr vattr;
	struct timespec now;
	mount_t mp;
	int error;

	if ((error = nfs_node_lock(np))) {
		return error;
	}
	if (np->n_flag & (NACC | NUPD)) {
		nanotime(&now);
		if (np->n_flag & NACC) {
			np->n_atim.tv_sec = now.tv_sec;
			np->n_atim.tv_nsec = now.tv_nsec;
		}
		if (np->n_flag & NUPD) {
			np->n_mtim.tv_sec = now.tv_sec;
			np->n_mtim.tv_nsec = now.tv_nsec;
		}
		np->n_flag |= NCHG;
		if (!vnode_isinuse(vp, 1) && (mp = vnode_mount(vp)) && !vfs_isrdonly(mp)) {
			VATTR_INIT(&vattr);
			if (np->n_flag & NACC) {
				vattr.va_access_time = np->n_atim;
				VATTR_SET_ACTIVE(&vattr, va_access_time);
			}
			if (np->n_flag & NUPD) {
				vattr.va_modify_time = np->n_mtim;
				VATTR_SET_ACTIVE(&vattr, va_modify_time);
			}
			nfs_node_unlock(np);
			vnode_setattr(vp, &vattr, ap->a_context);
		} else {
			nfs_node_unlock(np);
		}
	} else {
		nfs_node_unlock(np);
	}
	return VOCALL(fifo_vnodeop_p, VOFFSET(vnop_close), ap);
}
#endif           

            
int
nfs_vnop_ioctl(
	struct vnop_ioctl_args     
                                                               
                                                
                                                       
                                                  
                                               
                                                           
                                       *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	struct nfsmount *mp = VTONMP(vp);
	int error = ENOTTY;
#if CONFIG_NFS_GSS
	struct user_nfs_gss_principal gprinc = {};
	size_t len;
#endif

	if (mp == NULL) {
		return ENXIO;
	}
	switch (ap->a_command) {
	case F_FULLFSYNC:
		if (vnode_vfsisrdonly(vp)) {
			return EROFS;
		}
		error = nfs_flush(VTONFS(vp), MNT_WAIT, vfs_context_thread(ctx), 0);
		break;
#if CONFIG_NFS_GSS
	case NFS_IOC_DESTROY_CRED:
		if (!auth_is_kerberized(mp->nm_auth)) {
			return ENOTSUP;
		}
		if ((error = nfs_gss_clnt_ctx_remove(mp, vfs_context_ucred(ctx))) == ENOENT) {
			error = 0;
		}
		break;
	case NFS_IOC_SET_CRED:
	case NFS_IOC_SET_CRED64:
		if (!auth_is_kerberized(mp->nm_auth)) {
			return ENOTSUP;
		}
		if ((ap->a_command == NFS_IOC_SET_CRED && vfs_context_is64bit(ctx)) ||
		    (ap->a_command == NFS_IOC_SET_CRED64 && !vfs_context_is64bit(ctx))) {
			return EINVAL;
		}
		if (vfs_context_is64bit(ctx)) {
			gprinc = *(struct user_nfs_gss_principal *)ap->a_data;
		} else {
			struct nfs_gss_principal *tp;
			tp = (struct nfs_gss_principal *)ap->a_data;
			gprinc.princlen = tp->princlen;
			gprinc.nametype = tp->nametype;
			gprinc.principal = CAST_USER_ADDR_T(tp->principal);
		}
		NFS_DBG(NFS_FAC_GSS, 7, "Enter NFS_FSCTL_SET_CRED (64-bit=%d): principal length %zu name type %d usr pointer 0x%llx\n", vfs_context_is64bit(ctx), gprinc.princlen, gprinc.nametype, gprinc.principal);
		if (gprinc.princlen > MAXPATHLEN) {
			return EINVAL;
		}
		uint8_t *p;
		MALLOC(p, uint8_t *, gprinc.princlen + 1, M_TEMP, M_WAITOK | M_ZERO);
		if (p == NULL) {
			return ENOMEM;
		}
		assert((user_addr_t)gprinc.principal == gprinc.principal);
		error = copyin((user_addr_t)gprinc.principal, p, gprinc.princlen);
		if (error) {
			NFS_DBG(NFS_FAC_GSS, 7, "NFS_FSCTL_SET_CRED could not copy in princiapl data of len %zu: %d\n",
			    gprinc.princlen, error);
			FREE(p, M_TEMP);
			return error;
		}
		NFS_DBG(NFS_FAC_GSS, 7, "Seting credential to principal %s\n", p);
		error = nfs_gss_clnt_ctx_set_principal(mp, ctx, p, gprinc.princlen, gprinc.nametype);
		NFS_DBG(NFS_FAC_GSS, 7, "Seting credential to principal %s returned %d\n", p, error);
		FREE(p, M_TEMP);
		break;
	case NFS_IOC_GET_CRED:
	case NFS_IOC_GET_CRED64:
		if (!auth_is_kerberized(mp->nm_auth)) {
			return ENOTSUP;
		}
		if ((ap->a_command == NFS_IOC_GET_CRED && vfs_context_is64bit(ctx)) ||
		    (ap->a_command == NFS_IOC_GET_CRED64 && !vfs_context_is64bit(ctx))) {
			return EINVAL;
		}
		error = nfs_gss_clnt_ctx_get_principal(mp, ctx, &gprinc);
		if (error) {
			break;
		}
		if (vfs_context_is64bit(ctx)) {
			struct user_nfs_gss_principal *upp = (struct user_nfs_gss_principal *)ap->a_data;
			len = upp->princlen;
			if (gprinc.princlen < len) {
				len = gprinc.princlen;
			}
			upp->princlen = gprinc.princlen;
			upp->nametype = gprinc.nametype;
			upp->flags = gprinc.flags;
			if (gprinc.principal) {
				assert((user_addr_t)upp->principal == upp->principal);
				error = copyout((void *)gprinc.principal, (user_addr_t)upp->principal, len);
			} else {
				upp->principal = USER_ADDR_NULL;
			}
		} else {
			struct nfs_gss_principal *u32pp = (struct nfs_gss_principal *)ap->a_data;
			len = u32pp->princlen;
			if (gprinc.princlen < len) {
				len = gprinc.princlen;
			}
			u32pp->princlen = gprinc.princlen;
			u32pp->nametype = gprinc.nametype;
			u32pp->flags = gprinc.flags;
			if (gprinc.principal) {
				error = copyout((void *)gprinc.principal, u32pp->principal, len);
			} else {
				u32pp->principal = (user32_addr_t)0;
			}
		}
		if (error) {
			NFS_DBG(NFS_FAC_GSS, 7, "NFS_FSCTL_GET_CRED could not copy out princiapl data of len %zu: %d\n",
			    gprinc.princlen, error);
		}
		if (gprinc.principal) {
			void *ptr = (void *)gprinc.principal;
			gprinc.principal = 0;
			FREE(ptr, M_TEMP);
		}
#endif                     
	}

	return error;
}

            
int
nfs_vnop_select(
	__unused struct vnop_select_args     
                                                                         
                                                          
                                                         
                                                          
                                                         
                                                                     
                                                 *ap)
{
	  
                                                                   
    
	return 1;
}

  
                                
  
                                                           
   
int
nfs_vnop_pagein(
	struct vnop_pagein_args     
                                                                
                                                 
                                               
                                                            
                                                     
                                                  
                                                
                                                            
                                        *ap)
{
	vnode_t vp = ap->a_vp;
	upl_t pl = ap->a_pl;
	upl_size_t size = (upl_size_t)ap->a_size;
	off_t f_offset = ap->a_f_offset;
	upl_offset_t pl_offset = ap->a_pl_offset;
	int flags = ap->a_flags;
	thread_t thd;
	kauth_cred_t cred;
	nfsnode_t np = VTONFS(vp);
	size_t nmrsize, iosize, txsize, rxsize, retsize;
	off_t txoffset;
	struct nfsmount *nmp;
	int error = 0;
	vm_offset_t ioaddr, rxaddr;
	uio_t uio;
	char uio_buf[UIO_SIZEOF(1)];
	int nofreeupl = flags & UPL_NOCOMMIT;
	upl_page_info_t *plinfo;
#define MAXPAGINGREQS   16                                                   
	struct nfsreq *req[MAXPAGINGREQS];
	int nextsend, nextwait;
#if CONFIG_NFS4
	uint32_t stategenid = 0;
#endif
	uint32_t restart = 0;
	kern_return_t kret;

	FSDBG(322, np, f_offset, size, flags);
	if (pl == (upl_t)NULL) {
		panic("nfs_pagein: no upl");
	}

	if (size <= 0) {
		printf("nfs_pagein: invalid size %u", size);
		if (!nofreeupl) {
			(void) ubc_upl_abort_range(pl, pl_offset, size, 0);
		}
		return EINVAL;
	}
	if (f_offset < 0 || f_offset >= (off_t)np->n_size || (f_offset & PAGE_MASK_64)) {
		if (!nofreeupl) {
			ubc_upl_abort_range(pl, pl_offset, size,
			    UPL_ABORT_ERROR | UPL_ABORT_FREE_ON_EMPTY);
		}
		return EINVAL;
	}

	thd = vfs_context_thread(ap->a_context);
	cred = ubc_getcred(vp);
	if (!IS_VALID_CRED(cred)) {
		cred = vfs_context_ucred(ap->a_context);
	}

	uio = uio_createwithbuffer(1, f_offset, UIO_SYSSPACE, UIO_READ,
	    &uio_buf, sizeof(uio_buf));

	nmp = VTONMP(vp);
	if (nfs_mount_gone(nmp)) {
		if (!nofreeupl) {
			ubc_upl_abort_range(pl, pl_offset, size,
			    UPL_ABORT_ERROR | UPL_ABORT_FREE_ON_EMPTY);
		}
		return ENXIO;
	}
	nmrsize = nmp->nm_rsize;

	plinfo = ubc_upl_pageinfo(pl);
	kret = ubc_upl_map(pl, &ioaddr);
	if (kret != KERN_SUCCESS) {
		panic("nfs_vnop_pagein: ubc_upl_map() failed with (%d)", kret);
	}
	ioaddr += pl_offset;

tryagain:
#if CONFIG_NFS4
	if (nmp->nm_vers >= NFS_VER4) {
		stategenid = nmp->nm_stategenid;
	}
#endif
	txsize = rxsize = size;
	txoffset = f_offset;
	rxaddr = ioaddr;

	bzero(req, sizeof(req));
	nextsend = nextwait = 0;
	do {
		if (np->n_flag & NREVOKE) {
			error = EIO;
			break;
		}
		                                                             
		while ((txsize > 0) && (req[nextsend] == NULL)) {
			iosize = MIN(nmrsize, txsize);
			if ((error = nmp->nm_funcs->nf_read_rpc_async(np, txoffset, iosize, thd, cred, NULL, &req[nextsend]))) {
				req[nextsend] = NULL;
				break;
			}
			txoffset += iosize;
			txsize -= iosize;
			nextsend = (nextsend + 1) % MAXPAGINGREQS;
		}
		                                                                  
		while ((rxsize > 0) && req[nextwait]) {
			iosize = retsize = MIN(nmrsize, rxsize);
			uio_reset(uio, uio_offset(uio), UIO_SYSSPACE, UIO_READ);
			uio_addiov(uio, CAST_USER_ADDR_T(rxaddr), iosize);
			FSDBG(322, uio_offset(uio), uio_resid(uio), rxaddr, rxsize);
#if UPL_DEBUG
			upl_ubc_alias_set(pl, (uintptr_t) current_thread(), (uintptr_t) 2);
#endif                
			OSAddAtomic64(1, &nfsstats.pageins);
			error = nmp->nm_funcs->nf_read_rpc_async_finish(np, req[nextwait], uio, &retsize, NULL);
			req[nextwait] = NULL;
			nextwait = (nextwait + 1) % MAXPAGINGREQS;
#if CONFIG_NFS4
			if ((nmp->nm_vers >= NFS_VER4) && nfs_mount_state_error_should_restart(error)) {
				lck_mtx_lock(&nmp->nm_lock);
				if ((error != NFSERR_GRACE) && (stategenid == nmp->nm_stategenid)) {
					NP(np, "nfs_vnop_pagein: error %d, initiating recovery", error);
					nfs_need_recover(nmp, error);
				}
				lck_mtx_unlock(&nmp->nm_lock);
				restart++;
				goto cancel;
			}
#endif
			if (error) {
				FSDBG(322, uio_offset(uio), uio_resid(uio), error, -1);
				break;
			}
			if (retsize < iosize) {
				                                                
				size_t zcnt = iosize - retsize;
				bzero((char *)rxaddr + retsize, zcnt);
				FSDBG(324, uio_offset(uio), retsize, zcnt, rxaddr);
				uio_update(uio, zcnt);
			}
			rxaddr += iosize;
			rxsize -= iosize;
			if (txsize) {
				break;
			}
		}
	} while (!error && (txsize || rxsize));

	restart = 0;

	if (error) {
#if CONFIG_NFS4
cancel:
#endif
		                                     
		while (req[nextwait]) {
			nfs_request_async_cancel(req[nextwait]);
			req[nextwait] = NULL;
			nextwait = (nextwait + 1) % MAXPAGINGREQS;
		}
		if (np->n_flag & NREVOKE) {
			error = EIO;
		} else if (restart) {
			if (restart <= nfs_mount_state_max_restarts(nmp)) {                                
				if (error == NFSERR_GRACE) {
					tsleep(&nmp->nm_state, (PZERO - 1), "nfsgrace", 2 * hz);
				}
				if (!(error = nfs_mount_state_wait_for_recovery(nmp))) {
					goto tryagain;
				}
			} else {
				NP(np, "nfs_pagein: too many restarts, aborting");
			}
		}
	}

	ubc_upl_unmap(pl);

	if (!nofreeupl) {
		if (error) {
			  
                                                                                     
                                                                                
      
			if (error != EAGAIN && error != EPERM) {
				ubc_upl_abort_range(pl, pl_offset, size,
				    UPL_ABORT_ERROR |
				    UPL_ABORT_FREE_ON_EMPTY);
			}
		} else {
			ubc_upl_commit_range(pl, pl_offset, size,
			    UPL_COMMIT_CLEAR_DIRTY |
			    UPL_COMMIT_FREE_ON_EMPTY);
		}
	}
	return error;
}


  
                                                                            
                                                      
                                                                         
                                                                         
             
   
char nfs_pageouterrorhandler(int);
enum actiontype {NOACTION, DUMP, DUMPANDLOG, RETRY, SEVER};
#define NFS_ELAST 88
static u_char errorcount[NFS_ELAST + 1];                                       
static const char errortooutcome[NFS_ELAST + 1] = {
	NOACTION,
	DUMP,                                                        
	DUMP,                                                           
	DUMPANDLOG,                                          
	RETRY,                                                              
	DUMP,                                                  
	DUMP,                                                      
	DUMPANDLOG,                                                 
	DUMPANDLOG,                                              
	DUMPANDLOG,                                              
	DUMPANDLOG,                                               
	DUMPANDLOG,                                                                    
	RETRY,                                                        
	DUMP,                                                    
	DUMPANDLOG,                                        
	DUMPANDLOG,                                                           
	RETRY,                                            
	DUMP,                                              
	DUMP,                                                   
	DUMP,                                                                    
	DUMP,                                                   
	DUMP,                                                       
	DUMP,                                                   
	DUMPANDLOG,                                                          
	DUMPANDLOG,                                                
	DUMPANDLOG,                                                           
	DUMPANDLOG,                                                    
	DUMP,                                                
	DUMP,                                                          
	DUMPANDLOG,                                         
	DUMP,                                                       
	DUMP,                                                 
	RETRY,                                            
	                   
	DUMPANDLOG,                                                              
	DUMPANDLOG,                                               
	RETRY,                                                                              
	DUMPANDLOG,                                                            
	DUMPANDLOG,                                                              
	                                             
	DUMPANDLOG,                                                              
	DUMPANDLOG,                                                                
	DUMPANDLOG,                                                 
	DUMPANDLOG,                                                                
	DUMPANDLOG,                                                         
	DUMPANDLOG,                                                            
	DUMPANDLOG,                                                               
	DUMPANDLOG,                                                       
	DUMPANDLOG,                                                                 
	DUMPANDLOG,                                                                                   
	DUMPANDLOG,                                                        
	DUMPANDLOG,                                                                  
	                                                
	RETRY,                                                     
	RETRY,                                                              
	RETRY,                                                                         
	RETRY,                                                                         
	RETRY,                                                               
	RETRY,                                                              
	RETRY,                                                                
	RETRY,                                                             
	RETRY,                                                                      
	RETRY,                                                                          
	RETRY,                                                         
	RETRY,                                                           

	DUMPANDLOG,                                                               
	DUMP,                                                            
	RETRY,                                                  
	RETRY,                                                         
	DUMP,                                                          
	                   
	DUMPANDLOG,                                                  
	DUMPANDLOG,                                             
	DUMPANDLOG,                                                  
	                         
	DUMP,                                                          
	DUMP,                                                                       
	DUMPANDLOG,                                                 
	DUMPANDLOG,                                                     
	DUMPANDLOG,                                                       
	DUMPANDLOG,                                                         
	DUMPANDLOG,                                                             

	DUMPANDLOG,                                                 
	DUMPANDLOG,                                                       
	DUMPANDLOG,                                                                
	DUMPANDLOG,                                                  
	DUMPANDLOG,                                                   
	                               
	DUMPANDLOG,                                                   
	DUMPANDLOG,                                                            
	DUMPANDLOG,                                                                          
	                            
	DUMPANDLOG,                                               
	DUMPANDLOG,                                                           
	DUMPANDLOG,                                                                 
	DUMPANDLOG,                                                     
};

char
nfs_pageouterrorhandler(int error)
{
	if (error > NFS_ELAST) {
		return DUMP;
	} else {
		return errortooutcome[error];
	}
}


  
                                 
  
                                                           
                                                  
   
int
nfs_vnop_pageout(
	struct vnop_pageout_args     
                                                                 
                                                  
                                                
                                                             
                                                      
                                                   
                                                 
                                                             
                                         *ap)
{
	vnode_t vp = ap->a_vp;
	upl_t pl = ap->a_pl;
	upl_size_t size = (upl_size_t)ap->a_size;
	off_t f_offset = ap->a_f_offset;
	upl_offset_t pl_offset = ap->a_pl_offset;
	upl_offset_t pgsize;
	int flags = ap->a_flags;
	nfsnode_t np = VTONFS(vp);
	thread_t thd;
	kauth_cred_t cred;
	struct nfsbuf *bp;
	struct nfsmount *nmp = VTONMP(vp);
	daddr64_t lbn;
	int error = 0, iomode;
	off_t off, txoffset, rxoffset;
	vm_offset_t ioaddr, txaddr, rxaddr;
	uio_t auio;
	char uio_buf[UIO_SIZEOF(1)];
	int nofreeupl = flags & UPL_NOCOMMIT;
	size_t nmwsize, biosize, iosize, remsize;
	struct nfsreq *req[MAXPAGINGREQS];
	int nextsend, nextwait, wverfset, commit;
	uint64_t wverf, wverf2, xsize, txsize, rxsize;
#if CONFIG_NFS4
	uint32_t stategenid = 0;
#endif
	uint32_t vrestart = 0, restart = 0, vrestarts = 0, restarts = 0;
	kern_return_t kret;

	FSDBG(323, f_offset, size, pl, pl_offset);

	if (pl == (upl_t)NULL) {
		panic("nfs_pageout: no upl");
	}

	if (size <= 0) {
		printf("nfs_pageout: invalid size %u", size);
		if (!nofreeupl) {
			ubc_upl_abort_range(pl, pl_offset, size, 0);
		}
		return EINVAL;
	}

	if (!nmp) {
		if (!nofreeupl) {
			ubc_upl_abort(pl, UPL_ABORT_DUMP_PAGES | UPL_ABORT_FREE_ON_EMPTY);
		}
		return ENXIO;
	}
	biosize = nmp->nm_biosize;
	nmwsize = nmp->nm_wsize;

	nfs_data_lock_noupdate(np, NFS_DATA_LOCK_SHARED);

	  
                                              
                                                         
    
	for (iosize = 0; iosize < size; iosize += xsize) {
		off = f_offset + iosize;
		                                                     
		xsize = biosize - (off % biosize);
		if (off + (off_t)xsize > f_offset + (off_t)size) {
			xsize = f_offset + size - off;
		}
		lbn = (daddr64_t)(off / biosize);
		lck_mtx_lock(&nfs_buf_mutex);
		if ((bp = nfs_buf_incore(np, lbn))) {
			FSDBG(323, off, bp, bp->nb_lflags, bp->nb_flags);
			if (nfs_buf_acquire(bp, NBAC_NOWAIT, 0, 0)) {
				lck_mtx_unlock(&nfs_buf_mutex);
				nfs_data_unlock_noupdate(np);
				                                        
				if (!nofreeupl) {
					ubc_upl_abort_range(pl, pl_offset, size, 0);
				}
				return EBUSY;
			}
			if (bp->nb_dirtyend > 0) {
				  
                                                    
                                                     
      
                                                     
                                                    
                                                     
      
                                                  
                                                  
                                                   
                                                  
                                                    
                                                    
       
				off_t boff, start, end;
				boff = NBOFF(bp);
				start = off;
				end = off + xsize;
				                     
				if (end > (off_t)np->n_size) {
					end = np->n_size;
				}
				start -= boff;
				end -= boff;
				if ((bp->nb_dirtyoff < start) &&
				    (bp->nb_dirtyend > end)) {
					  
                                                  
       
                                                   
                                                  
                                                
                   
        
					FSDBG(323, np, bp, 0xd00deebc, EBUSY);
					nfs_buf_remfree(bp);
					TAILQ_INSERT_HEAD(&nfsbufdelwri, bp, nb_free);
					nfsbufdelwricnt++;
					nfs_buf_drop(bp);
					nfs_buf_delwri_push(1);
					lck_mtx_unlock(&nfs_buf_mutex);
					nfs_data_unlock_noupdate(np);
					if (!nofreeupl) {
						ubc_upl_abort_range(pl, pl_offset, size, 0);
					}
					return EBUSY;
				}
				if ((bp->nb_dirtyoff < start) ||
				    (bp->nb_dirtyend > end)) {
					                                     
					if (bp->nb_dirtyoff < start) {
						bp->nb_dirtyend = MIN(bp->nb_dirtyend, start);
					}
					if (bp->nb_dirtyend > end) {
						bp->nb_dirtyoff = MAX(bp->nb_dirtyoff, end);
					}
					FSDBG(323, bp, bp->nb_dirtyoff, bp->nb_dirtyend, 0xd00dee00);
					                                    
					nfs_buf_drop(bp);
					lck_mtx_unlock(&nfs_buf_mutex);
					continue;
				}
			}
			nfs_buf_remfree(bp);
			lck_mtx_unlock(&nfs_buf_mutex);
			SET(bp->nb_flags, NB_INVAL);
			nfs_node_lock_force(np);
			if (ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
				CLR(bp->nb_flags, NB_NEEDCOMMIT);
				np->n_needcommitcnt--;
				CHECK_NEEDCOMMITCNT(np);
			}
			nfs_node_unlock(np);
			nfs_buf_release(bp, 1);
		} else {
			lck_mtx_unlock(&nfs_buf_mutex);
		}
	}

	thd = vfs_context_thread(ap->a_context);
	cred = ubc_getcred(vp);
	if (!IS_VALID_CRED(cred)) {
		cred = vfs_context_ucred(ap->a_context);
	}

	nfs_node_lock_force(np);
	if (np->n_flag & NWRITEERR) {
		error = np->n_error;
		nfs_node_unlock(np);
		nfs_data_unlock_noupdate(np);
		if (!nofreeupl) {
			ubc_upl_abort_range(pl, pl_offset, size,
			    UPL_ABORT_FREE_ON_EMPTY);
		}
		return error;
	}
	nfs_node_unlock(np);

	if (f_offset < 0 || f_offset >= (off_t)np->n_size ||
	    f_offset & PAGE_MASK_64 || size & PAGE_MASK_64) {
		nfs_data_unlock_noupdate(np);
		if (!nofreeupl) {
			ubc_upl_abort_range(pl, pl_offset, size,
			    UPL_ABORT_FREE_ON_EMPTY);
		}
		return EINVAL;
	}

	kret = ubc_upl_map(pl, &ioaddr);
	if (kret != KERN_SUCCESS) {
		panic("nfs_vnop_pageout: ubc_upl_map() failed with (%d)", kret);
	}
	ioaddr += pl_offset;

	if ((u_quad_t)f_offset + size > np->n_size) {
		xsize = np->n_size - f_offset;
	} else {
		xsize = size;
	}

	pgsize = (upl_offset_t)round_page_64(xsize);
	if ((size > pgsize) && !nofreeupl) {
		ubc_upl_abort_range(pl, pl_offset + pgsize, size - pgsize,
		    UPL_ABORT_FREE_ON_EMPTY);
	}

	  
                                        
                                        
                                     
    
	if ((u_quad_t)f_offset < np->n_size && (u_quad_t)f_offset + size > np->n_size) {
		uint64_t io = np->n_size - f_offset;
		NFS_BZERO((caddr_t)(ioaddr + io), size - io);
		FSDBG(321, np->n_size, f_offset, f_offset + io, size - io);
	}
	nfs_data_unlock_noupdate(np);

	auio = uio_createwithbuffer(1, 0, UIO_SYSSPACE, UIO_WRITE,
	    &uio_buf, sizeof(uio_buf));

tryagain:
#if CONFIG_NFS4
	if (nmp->nm_vers >= NFS_VER4) {
		stategenid = nmp->nm_stategenid;
	}
#endif
	wverf = wverf2 = wverfset = 0;
	txsize = rxsize = xsize;
	txoffset = rxoffset = f_offset;
	txaddr = rxaddr = ioaddr;
	commit = NFS_WRITE_FILESYNC;

	bzero(req, sizeof(req));
	nextsend = nextwait = 0;
	do {
		if (np->n_flag & NREVOKE) {
			error = EIO;
			break;
		}
		                                                             
		while ((txsize > 0) && (req[nextsend] == NULL)) {
			iosize = (size_t)MIN(nmwsize, txsize);
			uio_reset(auio, txoffset, UIO_SYSSPACE, UIO_WRITE);
			uio_addiov(auio, CAST_USER_ADDR_T(txaddr), iosize);
			FSDBG(323, uio_offset(auio), iosize, txaddr, txsize);
			OSAddAtomic64(1, &nfsstats.pageouts);
			nfs_node_lock_force(np);
			np->n_numoutput++;
			nfs_node_unlock(np);
			vnode_startwrite(vp);
			iomode = NFS_WRITE_UNSTABLE;
			if ((error = nmp->nm_funcs->nf_write_rpc_async(np, auio, iosize, thd, cred, iomode, NULL, &req[nextsend]))) {
				req[nextsend] = NULL;
				vnode_writedone(vp);
				nfs_node_lock_force(np);
				np->n_numoutput--;
				nfs_node_unlock(np);
				break;
			}
			txaddr += iosize;
			txoffset += iosize;
			txsize -= iosize;
			nextsend = (nextsend + 1) % MAXPAGINGREQS;
		}
		                                                                  
		while ((rxsize > 0) && req[nextwait]) {
			iosize = remsize = (size_t)MIN(nmwsize, rxsize);
			error = nmp->nm_funcs->nf_write_rpc_async_finish(np, req[nextwait], &iomode, &iosize, &wverf2);
			req[nextwait] = NULL;
			nextwait = (nextwait + 1) % MAXPAGINGREQS;
			vnode_writedone(vp);
			nfs_node_lock_force(np);
			np->n_numoutput--;
			nfs_node_unlock(np);
#if CONFIG_NFS4
			if ((nmp->nm_vers >= NFS_VER4) && nfs_mount_state_error_should_restart(error)) {
				lck_mtx_lock(&nmp->nm_lock);
				if ((error != NFSERR_GRACE) && (stategenid == nmp->nm_stategenid)) {
					NP(np, "nfs_vnop_pageout: error %d, initiating recovery", error);
					nfs_need_recover(nmp, error);
				}
				lck_mtx_unlock(&nmp->nm_lock);
				restart = 1;
				goto cancel;
			}
#endif
			if (error) {
				FSDBG(323, rxoffset, rxsize, error, -1);
				break;
			}
			if (!wverfset) {
				wverf = wverf2;
				wverfset = 1;
			} else if (wverf != wverf2) {
				                                                            
				vrestart = 1;
				goto cancel;
			}
			                                                  
			if (iomode < commit) {
				commit = iomode;
			}
			rxaddr += iosize;
			rxoffset += iosize;
			rxsize -= iosize;
			remsize -= iosize;
			if (remsize > 0) {
				                                       
				iosize = remsize;
				uio_reset(auio, rxoffset, UIO_SYSSPACE, UIO_WRITE);
				uio_addiov(auio, CAST_USER_ADDR_T(rxaddr), remsize);
				iomode = NFS_WRITE_UNSTABLE;
				error = nfs_write_rpc2(np, auio, thd, cred, &iomode, &wverf2);
#if CONFIG_NFS4
				if ((nmp->nm_vers >= NFS_VER4) && nfs_mount_state_error_should_restart(error)) {
					NP(np, "nfs_vnop_pageout: restart: error %d", error);
					lck_mtx_lock(&nmp->nm_lock);
					if ((error != NFSERR_GRACE) && (stategenid == nmp->nm_stategenid)) {
						NP(np, "nfs_vnop_pageout: error %d, initiating recovery", error);
						nfs_need_recover(nmp, error);
					}
					lck_mtx_unlock(&nmp->nm_lock);
					restart = 1;
					goto cancel;
				}
#endif
				if (error) {
					FSDBG(323, rxoffset, rxsize, error, -1);
					break;
				}
				if (wverf != wverf2) {
					                                                            
					vrestart = 1;
					goto cancel;
				}
				if (iomode < commit) {
					commit = iomode;
				}
				rxaddr += iosize;
				rxoffset += iosize;
				rxsize -= iosize;
			}
			if (txsize) {
				break;
			}
		}
	} while (!error && (txsize || rxsize));

	vrestart = 0;

	if (!error && (commit != NFS_WRITE_FILESYNC)) {
		error = nmp->nm_funcs->nf_commit_rpc(np, f_offset, xsize, cred, wverf);
		if (error == NFSERR_STALEWRITEVERF) {
			vrestart = 1;
			error = EIO;
		}
	}

	if (error) {
cancel:
		                                     
		while (req[nextwait]) {
			nfs_request_async_cancel(req[nextwait]);
			req[nextwait] = NULL;
			nextwait = (nextwait + 1) % MAXPAGINGREQS;
			vnode_writedone(vp);
			nfs_node_lock_force(np);
			np->n_numoutput--;
			nfs_node_unlock(np);
		}
		if (np->n_flag & NREVOKE) {
			error = EIO;
		} else {
			if (vrestart) {
				if (++vrestarts <= 100) {                                
					goto tryagain;
				}
				NP(np, "nfs_pageout: too many restarts, aborting");
				FSDBG(323, f_offset, xsize, ERESTART, -1);
			}
			if (restart) {
				if (restarts <= nfs_mount_state_max_restarts(nmp)) {                                
					if (error == NFSERR_GRACE) {
						tsleep(&nmp->nm_state, (PZERO - 1), "nfsgrace", 2 * hz);
					}
					if (!(error = nfs_mount_state_wait_for_recovery(nmp))) {
						goto tryagain;
					}
				} else {
					NP(np, "nfs_pageout: too many restarts, aborting");
					FSDBG(323, f_offset, xsize, ERESTART, -1);
				}
			}
		}
	}

	ubc_upl_unmap(pl);

	  
                                                                        
                                                                    
                                                               
                                                                    
                                                                      
                                                                 
                                                                         
                                                                    
                                                                   
                                                                     
                                                                       
                                                                     
                                                              
                                                                           
                                                                      
                                                                    
                                                                      
                                                                      
                                                                     
                                                                       
                                                                         
    
	if (!nofreeupl) {                                                       
		if (error) {
			int abortflags = 0;
			char action = nfs_pageouterrorhandler(error);

			switch (action) {
			case DUMP:
				abortflags = UPL_ABORT_DUMP_PAGES | UPL_ABORT_FREE_ON_EMPTY;
				break;
			case DUMPANDLOG:
				abortflags = UPL_ABORT_DUMP_PAGES | UPL_ABORT_FREE_ON_EMPTY;
				if (error <= NFS_ELAST) {
					if ((errorcount[error] % 100) == 0) {
						NP(np, "nfs_pageout: unexpected error %d. dumping vm page", error);
					}
					errorcount[error]++;
				}
				break;
			case RETRY:
				abortflags = UPL_ABORT_FREE_ON_EMPTY;
				break;
			case SEVER:                              
			default:
				NP(np, "nfs_pageout: action %d not expected", action);
				break;
			}

			ubc_upl_abort_range(pl, pl_offset, pgsize, abortflags);
			                                     
		} else {
			ubc_upl_commit_range(pl, pl_offset, pgsize,
			    UPL_COMMIT_CLEAR_DIRTY |
			    UPL_COMMIT_FREE_ON_EMPTY);
		}
	}
	return error;
}

                                                               
int
nfs_vnop_blktooff(
	struct vnop_blktooff_args     
                                                                  
                                                   
                                                         
                                                      
                                          *ap)
{
	int biosize;
	vnode_t vp = ap->a_vp;
	struct nfsmount *nmp = VTONMP(vp);

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	biosize = nmp->nm_biosize;

	*ap->a_offset = (off_t)(ap->a_lblkno * biosize);

	return 0;
}

int
nfs_vnop_offtoblk(
	struct vnop_offtoblk_args     
                                                                  
                                                   
                                                     
                                                          
                                          *ap)
{
	int biosize;
	vnode_t vp = ap->a_vp;
	struct nfsmount *nmp = VTONMP(vp);

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	biosize = nmp->nm_biosize;

	*ap->a_lblkno = (daddr64_t)(ap->a_offset / biosize);

	return 0;
}

  
                          
   
int
nfs_vnop_monitor(
	struct vnop_monitor_args     
                                                                 
                                                  
                                                       
                                                      
                                                    
                                                             
                                         *ap)
{
	nfsnode_t np = VTONFS(ap->a_vp);
	struct nfsmount *nmp = VTONMP(ap->a_vp);
	int error = 0;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	                                                                
	lck_mtx_lock(&nmp->nm_lock);
	if (vnode_ismonitored(ap->a_vp)) {
		                                                                           
		if (np->n_monlink.le_next == NFSNOLIST) {
			LIST_INSERT_HEAD(&nmp->nm_monlist, np, n_monlink);
			nfs_mount_sock_thread_wake(nmp);
		}
	} else {
		                                                                               
		                                                         
		while (np->n_mflag & NMMONSCANINPROG) {
			struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
			np->n_mflag |= NMMONSCANWANT;
			msleep(&np->n_mflag, &nmp->nm_lock, PZERO - 1, "nfswaitmonscan", &ts);
		}
		if (np->n_monlink.le_next != NFSNOLIST) {
			LIST_REMOVE(np, n_monlink);
			np->n_monlink.le_next = NFSNOLIST;
		}
	}
	lck_mtx_unlock(&nmp->nm_lock);

	return error;
}

  
                                                  
   
void
nfs_vnode_notify(nfsnode_t np, uint32_t events)
{
	struct nfsmount *nmp = NFSTONMP(np);
	struct nfs_vattr *nvattr;
	struct vnode_attr vattr, *vap = NULL;
	struct timeval now;

	microuptime(&now);
	if ((np->n_evtstamp == now.tv_sec) || !nmp) {
		                               
		np->n_events |= events;
		return;
	}
	events |= np->n_events;
	np->n_events = 0;
	np->n_evtstamp = now.tv_sec;
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);

	vfs_get_notify_attributes(&vattr);
	if (!nfs_getattrcache(np, nvattr, 0)) {
		vap = &vattr;
		VATTR_INIT(vap);

		VATTR_RETURN(vap, va_fsid, vfs_statfs(nmp->nm_mountp)->f_fsid.val[0]);
		VATTR_RETURN(vap, va_fileid, nvattr->nva_fileid);
		VATTR_RETURN(vap, va_mode, nvattr->nva_mode);
		VATTR_RETURN(vap, va_uid, nvattr->nva_uid);
		VATTR_RETURN(vap, va_gid, nvattr->nva_gid);
		VATTR_RETURN(vap, va_nlink, nvattr->nva_nlink);
	}
	vnode_notify(NFSTOV(np), events, vap);
	FREE(nvattr, M_TEMP);
}

#endif                        
