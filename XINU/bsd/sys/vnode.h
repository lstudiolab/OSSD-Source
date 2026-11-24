/*
 * Copyright (c) 2000-2017 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1989, 1993
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
 *	@(#)vnode.h	8.17 (Berkeley) 5/20/95
 */

#ifndef _VNODE_H_
#define _VNODE_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>
#ifdef KERNEL
#include <sys/kernel_types.h>
#include <sys/param.h>
#include <sys/signal.h>
#else
#include <stdint.h>
#endif             

  
                                                                   
                                                                       
                                                 
   

  
                                    
   
enum vtype      {
	       
	VNON,
	           
	VREG, VDIR, VBLK, VCHR, VLNK,
	            
	VSOCK, VFIFO, VBAD, VSTR, VCPLX
};

  
                   
                                                                    
                                               
   
enum vtagtype   {
	       
	VT_NON,
	                                                        
	VT_UFS,
	           
	VT_NFS, VT_MFS, VT_MSDOSFS, VT_LFS,
	            
	VT_LOFS, VT_FDESC, VT_PORTAL, VT_NULL, VT_UMAP,
	             
	VT_KERNFS, VT_PROCFS, VT_AFS, VT_ISOFS, VT_MOCKFS,
	             
	VT_HFS, VT_ZFS, VT_DEVFS, VT_WEBDAV, VT_UDF,
	             
	VT_AFP, VT_CDDA, VT_CIFS, VT_OTHER, VT_APFS,
	            
	VT_LOCKERFS, VT_BINDFS,
};

#define HAVE_VT_LOCKERFS 1

  
                          
   
#define VNODE_READ      0x01
#define VNODE_WRITE     0x02
#define VNODE_BLOCKMAP_NO_TRACK 0x04                                          


                             
#define PREALLOCATE             0x00000001                                         
#define ALLOCATECONTIG  0x00000002                                     
#define ALLOCATEALL             0x00000004                                        
                        
#define FREEREMAINDER   0x00000008                                    
                     
#define ALLOCATEFROMPEOF        0x00000010                                          
#define ALLOCATEFROMVOL         0x00000020                                           

  
                                                                               
   
#define VNOVAL  (-1)


#ifdef KERNEL

  
                    
   
#define IO_UNIT         0x0001                                     
#define IO_APPEND       0x0002                                   
#define IO_SYNC         0x0004                                    
#define IO_NODELOCKED   0x0008                                              
#define IO_NDELAY       0x0010                                              
#define IO_NOZEROFILL   0x0020                                                            
#ifdef XNU_KERNEL_PRIVATE
#define IO_REVOKE       IO_NOZEROFILL                                                               
#endif                         
#define IO_TAILZEROFILL 0x0040                                               
#define IO_HEADZEROFILL 0x0080                                               
#define IO_NOZEROVALID  0x0100                                              
#define IO_NOZERODIRTY  0x0200                                                 
#define IO_CLOSE        0x0400                                          
#define IO_NOCACHE      0x0800                                                                     
#define IO_RAOFF        0x1000                                                              
#define IO_DEFWRITE     0x2000                                                  
#define IO_PASSIVE      0x4000                                                                                          
#define IO_BACKGROUND IO_PASSIVE                                                                                    
                                                                 
#define IO_NOAUTH       0x8000                                        
#define IO_NODIRECT     0x10000                                                                             
#define IO_ENCRYPTED    0x20000                                                            
#define IO_RETURN_ON_THROTTLE   0x40000
#define IO_SINGLE_WRITER        0x80000
#define IO_SYSCALL_DISPATCH             0x100000                                                          
#define IO_SWAP_DISPATCH                0x200000                                                    
#define IO_SKIP_ENCRYPTION              0x400000                                                                           
#define IO_EVTONLY                      0x800000                                                                    

  
                                                        
                                                         
   
struct componentname {
	  
                        
    
	uint32_t        cn_nameiop;                           
	uint32_t        cn_flags;                              
#ifdef BSD_KERNEL_PRIVATE
	vfs_context_t   cn_context;
	struct nameidata *cn_ndp;                                      

                                             
#define cn_proc         (cn_context->vc_proc + 0)                       
#define cn_cred         (cn_context->vc_ucred + 0)                      

#else
	void * cn_reserved1;                           
	void * cn_reserved2;                           
#endif
	  
                                              
    
	char    *cn_pnbuf;                           
	int     cn_pnlen;                                       
	char    *cn_nameptr;                                   
	int     cn_namelen;                                        
	uint32_t        cn_hash;                                          
	uint32_t        cn_consume;                                       
};

  
                                              
   
#define LOOKUP          0                                     
#define CREATE          1                                    
#define DELETE          2                                    
#define RENAME          3                                    
#define OPMASK          3                               

  
                                            
   
#define FOLLOW          0x00000040                            

  
                                        
   
#define ISDOTDOT        0x00002000                                   
#define MAKEENTRY       0x00004000                                         
#define ISLASTCN        0x00008000                                         

                                                            
struct vnode_fsparam {
	struct mount * vnfs_mp;                                                           
	enum vtype      vnfs_vtype;                             
	const char * vnfs_str;                                     
	struct vnode * vnfs_dvp;                                              
	void * vnfs_fsnode;                                
	int(**vnfs_vops)(void *);                                         
	int vnfs_markroot;                                                                              
	int vnfs_marksystem;                                    
	dev_t vnfs_rdev;                                                             
	off_t vnfs_filesize;                                                     
	struct componentname * vnfs_cnp;                                         
	uint32_t vnfs_flags;                       
};

#define VNFS_NOCACHE    0x01                                               
#define VNFS_CANTCACHE  0x02                                                   
#define VNFS_ADDFSREF   0x04                                   

#define VNCREATE_FLAVOR 0
#define VCREATESIZE sizeof(struct vnode_fsparam)
#ifdef KERNEL_PRIVATE
  
                                             
   
struct vnode_trigger_param;
#define VNCREATE_TRIGGER        (('T' << 8) + ('V'))
#define VNCREATE_TRIGGER_SIZE   sizeof(struct vnode_trigger_param)
#endif                     


#ifdef KERNEL_PRIVATE
  
                                           
  
                                                                
   

   
                                    
                                                                                   
   
enum path_operation     {
	OP_LOOKUP,
	OP_MOUNT,
	OP_UNMOUNT,
	OP_STATFS,
	OP_OPEN,
	OP_LINK,
	OP_UNLINK,
	OP_RENAME,
	OP_CHDIR,
	OP_CHROOT,
	OP_MKNOD,
	OP_MKFIFO,
	OP_SYMLINK,
	OP_ACCESS,
	OP_PATHCONF,
	OP_READLINK,
	OP_GETATTR,
	OP_SETATTR,
	OP_TRUNCATE,
	OP_COPYFILE,
	OP_MKDIR,
	OP_RMDIR,
	OP_REVOKE,
	OP_EXCHANGEDATA,
	OP_SEARCHFS,
	OP_FSCTL,
	OP_GETXATTR,
	OP_SETXATTR,
	OP_REMOVEXATTR,
	OP_LISTXATTR,
	OP_MAXOP                                                       
};

   
                         
                                                
                                                                                               
                                                                  
                                                                                                   
                                                                                           
                                                                                
   
enum resolver_status {
	RESOLVER_RESOLVED,
	RESOLVER_NOCHANGE,
	RESOLVER_UNRESOLVED,
	RESOLVER_ERROR,
	RESOLVER_STOP
};

typedef uint64_t resolver_result_t;

  
                           
  
                                                                       
                                                                         
         
  
                                                                    
                                                                  
                                                               
                                                                    
                                 
  
                                                                  
                                                                    
                                                                 
                                 
   
extern resolver_result_t vfs_resolver_result(uint32_t seq, enum resolver_status stat, int aux);

  
                                                 
   
extern enum resolver_status vfs_resolver_status(resolver_result_t);
extern uint32_t vfs_resolver_sequence(resolver_result_t);
extern int vfs_resolver_auxiliary(resolver_result_t);


   
                                             
                                                                     
                                                                                              
                                                                       
                                                     
                                                                       
                                                                                          
                              
                                                                
                                          
                                                                                       
   
typedef resolver_result_t (* trigger_vnode_resolve_callback_t)(
	vnode_t                         vp,
	const struct componentname *    cnp,
	enum path_operation             pop,
	int                             flags,
	void *                          data,
	vfs_context_t                   ctx);

   
                                               
                                                                       
                                                                                              
                                                                       
                                                       
                              
                                                                
                                          
                                                                    
   
typedef resolver_result_t (* trigger_vnode_unresolve_callback_t)(
	vnode_t         vp,
	int             flags,
	void *          data,
	vfs_context_t   ctx);

   
                                           
                                                                   
                                                                                              
                                                                           
                                                    
                            
                                                                
                                          
                                               
   
typedef resolver_result_t (* trigger_vnode_rearm_callback_t)(
	vnode_t         vp,
	int             flags,
	void *          data,
	vfs_context_t   ctx);

   
                                             
                                                                     
                                                                                              
                                                       
                                                        
                                                                    
   
typedef void (* trigger_vnode_reclaim_callback_t)(
	vnode_t         vp,
	void *          data);

   
                                  
                                             
                                                                                          
                                                            
                                                  
                                                                         
   
extern int vnode_trigger_update(vnode_t vp, resolver_result_t result);

struct vnode_trigger_info {
	trigger_vnode_resolve_callback_t        vti_resolve_func;
	trigger_vnode_unresolve_callback_t      vti_unresolve_func;
	trigger_vnode_rearm_callback_t          vti_rearm_func;
	trigger_vnode_reclaim_callback_t        vti_reclaim_func;
	void *                                  vti_data;                                  
	uint32_t                                vti_flags;                                  
};

  
                                   
  
                                                                    
  
                               
  
                                               
                                                     
                                                 
                                                                   
         
   
struct vnode_trigger_param {
	struct vnode_fsparam                    vnt_params;                                  
	trigger_vnode_resolve_callback_t        vnt_resolve_func;
	trigger_vnode_unresolve_callback_t      vnt_unresolve_func;
	trigger_vnode_rearm_callback_t          vnt_rearm_func;
	trigger_vnode_reclaim_callback_t        vnt_reclaim_func;
	void *                                  vnt_data;                                  
	uint32_t                                vnt_flags;                                  
};

  
                                  
  
                  
                                                                    
  
                       
                                                                  
                                                         
  
                    
                                                                    
                             
   
#define VNT_AUTO_REARM          (1 << 0)
#define VNT_NO_DIRECT_MOUNT     (1 << 1)
#define VNT_KERN_RESOLVE        (1 << 2)
#define VNT_VALID_MASK          (VNT_AUTO_REARM | VNT_NO_DIRECT_MOUNT | \
	                         VNT_KERN_RESOLVE)

#endif                     


  
                               
  
                                                                             
                       
  
                                                                               
   

#define VATTR_INIT(v)                   do {(v)->va_supported = (v)->va_active = 0ll; (v)->va_vaflags = 0; } while(0)
#define VATTR_SET_ACTIVE(v, a)          ((v)->va_active |= VNODE_ATTR_ ## a)
#define VATTR_SET_SUPPORTED(v, a)       ((v)->va_supported |= VNODE_ATTR_ ## a)
#define VATTR_IS_SUPPORTED(v, a)        ((v)->va_supported & VNODE_ATTR_ ## a)
#define VATTR_CLEAR_ACTIVE(v, a)        ((v)->va_active &= ~VNODE_ATTR_ ## a)
#define VATTR_CLEAR_SUPPORTED(v, a)     ((v)->va_supported &= ~VNODE_ATTR_ ## a)
#define VATTR_CLEAR_SUPPORTED_ALL(v)    ((v)->va_supported = 0)
#define VATTR_IS_ACTIVE(v, a)           ((v)->va_active & VNODE_ATTR_ ## a)
#define VATTR_ALL_SUPPORTED(v)          (((v)->va_active & (v)->va_supported) == (v)->va_active)
#define VATTR_INACTIVE_SUPPORTED(v)     do {(v)->va_active &= ~(v)->va_supported; (v)->va_supported = 0;} while(0)
#define VATTR_SET(v, a, x)              do { (v)-> a = (x); VATTR_SET_ACTIVE(v, a);} while(0)
#define VATTR_WANTED(v, a)              VATTR_SET_ACTIVE(v, a)
#define VATTR_RETURN(v, a, x)           do { (v)-> a = (x); VATTR_SET_SUPPORTED(v, a);} while(0)
#define VATTR_NOT_RETURNED(v, a)        (VATTR_IS_ACTIVE(v, a) && !VATTR_IS_SUPPORTED(v, a))

  
                                                              
   
#define VATTR_IS(v, a, x)               (VATTR_IS_SUPPORTED(v, a) && (v)-> a == (x))
#define VATTR_IS_NOT(v, a, x)           (VATTR_IS_SUPPORTED(v, a) && (v)-> a != (x))

#define VNODE_ATTR_va_rdev              (1LL<< 0)                     
#define VNODE_ATTR_va_nlink             (1LL<< 1)                     
#define VNODE_ATTR_va_total_size        (1LL<< 2)                     
#define VNODE_ATTR_va_total_alloc       (1LL<< 3)                     
#define VNODE_ATTR_va_data_size         (1LL<< 4)                     
#define VNODE_ATTR_va_data_alloc        (1LL<< 5)                     
#define VNODE_ATTR_va_iosize            (1LL<< 6)                     
#define VNODE_ATTR_va_uid               (1LL<< 7)                     
#define VNODE_ATTR_va_gid               (1LL<< 8)                     
#define VNODE_ATTR_va_mode              (1LL<< 9)                     
#define VNODE_ATTR_va_flags             (1LL<<10)                     
#define VNODE_ATTR_va_acl               (1LL<<11)                     
#define VNODE_ATTR_va_create_time       (1LL<<12)                     
#define VNODE_ATTR_va_access_time       (1LL<<13)                     
#define VNODE_ATTR_va_modify_time       (1LL<<14)                     
#define VNODE_ATTR_va_change_time       (1LL<<15)                     
#define VNODE_ATTR_va_backup_time       (1LL<<16)                     
#define VNODE_ATTR_va_fileid            (1LL<<17)                     
#define VNODE_ATTR_va_linkid            (1LL<<18)                     
#define VNODE_ATTR_va_parentid          (1LL<<19)                     
#define VNODE_ATTR_va_fsid              (1LL<<20)                     
#define VNODE_ATTR_va_filerev           (1LL<<21)                     
#define VNODE_ATTR_va_gen               (1LL<<22)                     
#define VNODE_ATTR_va_encoding          (1LL<<23)                     
#define VNODE_ATTR_va_type              (1LL<<24)                     
#define VNODE_ATTR_va_name              (1LL<<25)                     
#define VNODE_ATTR_va_uuuid             (1LL<<26)                     
#define VNODE_ATTR_va_guuid             (1LL<<27)                     
#define VNODE_ATTR_va_nchildren         (1LL<<28)                     
#define VNODE_ATTR_va_dirlinkcount      (1LL<<29)                     
#define VNODE_ATTR_va_addedtime         (1LL<<30)                     
#define VNODE_ATTR_va_dataprotect_class (1LL<<31)                     
#define VNODE_ATTR_va_dataprotect_flags (1LL<<32)                      
#define VNODE_ATTR_va_document_id       (1LL<<33)                      
#define VNODE_ATTR_va_devid             (1LL<<34)                      
#define VNODE_ATTR_va_objtype           (1LL<<35)                      
#define VNODE_ATTR_va_objtag            (1LL<<36)                       
#define VNODE_ATTR_va_user_access       (1LL<<37)                       
#define VNODE_ATTR_va_finderinfo        (1LL<<38)                       
#define VNODE_ATTR_va_rsrc_length       (1LL<<39)                       
#define VNODE_ATTR_va_rsrc_alloc        (1LL<<40)                        
#define VNODE_ATTR_va_fsid64            (1LL<<41)                        
#define VNODE_ATTR_va_write_gencount    (1LL<<42)                        
#define VNODE_ATTR_va_private_size      (1LL<<43)                        
#define VNODE_ATTR_va_clone_id          (1LL<<44)                         
#define VNODE_ATTR_va_extflags          (1LL<<45)                         
#define VNODE_ATTR_va_recursive_gencount (1LL<<46)                        

#define VNODE_ATTR_BIT(n)       (VNODE_ATTR_ ## n)

  
                         
   
#define VNODE_ATTR_ALL          (VNODE_ATTR_BIT(va_rdev) |              \
	                        VNODE_ATTR_BIT(va_nlink) |              \
	                        VNODE_ATTR_BIT(va_total_size) |         \
	                        VNODE_ATTR_BIT(va_total_alloc) |        \
	                        VNODE_ATTR_BIT(va_data_size) |          \
	                        VNODE_ATTR_BIT(va_data_alloc) |         \
	                        VNODE_ATTR_BIT(va_iosize) |             \
	                        VNODE_ATTR_BIT(va_uid) |                \
	                        VNODE_ATTR_BIT(va_gid) |                \
	                        VNODE_ATTR_BIT(va_mode) |               \
	                        VNODE_ATTR_BIT(va_flags) |              \
	                        VNODE_ATTR_BIT(va_acl) |                \
	                        VNODE_ATTR_BIT(va_create_time) |        \
	                        VNODE_ATTR_BIT(va_access_time) |        \
	                        VNODE_ATTR_BIT(va_modify_time) |        \
	                        VNODE_ATTR_BIT(va_change_time) |        \
	                        VNODE_ATTR_BIT(va_backup_time) |        \
	                        VNODE_ATTR_BIT(va_fileid) |             \
	                        VNODE_ATTR_BIT(va_linkid) |             \
	                        VNODE_ATTR_BIT(va_parentid) |           \
	                        VNODE_ATTR_BIT(va_fsid) |               \
	                        VNODE_ATTR_BIT(va_filerev) |            \
	                        VNODE_ATTR_BIT(va_gen) |                \
	                        VNODE_ATTR_BIT(va_encoding) |           \
	                        VNODE_ATTR_BIT(va_type) |               \
	                        VNODE_ATTR_BIT(va_name) |               \
	                        VNODE_ATTR_BIT(va_uuuid) |              \
	                        VNODE_ATTR_BIT(va_guuid) |              \
	                        VNODE_ATTR_BIT(va_nchildren) |          \
	                        VNODE_ATTR_BIT(va_dirlinkcount) |       \
	                        VNODE_ATTR_BIT(va_addedtime) |          \
	                        VNODE_ATTR_BIT(va_dataprotect_class) |  \
	                        VNODE_ATTR_BIT(va_dataprotect_flags) |  \
	                        VNODE_ATTR_BIT(va_document_id) |        \
	                        VNODE_ATTR_BIT(va_devid) |              \
	                        VNODE_ATTR_BIT(va_objtype) |            \
	                        VNODE_ATTR_BIT(va_objtag) |             \
	                        VNODE_ATTR_BIT(va_user_access) |        \
	                        VNODE_ATTR_BIT(va_finderinfo) |         \
	                        VNODE_ATTR_BIT(va_rsrc_length) |        \
	                        VNODE_ATTR_BIT(va_rsrc_alloc) |         \
	                        VNODE_ATTR_BIT(va_fsid64) |             \
	                        VNODE_ATTR_BIT(va_write_gencount) |     \
	                        VNODE_ATTR_BIT(va_private_size) |       \
	                        VNODE_ATTR_BIT(va_clone_id) |           \
	                        VNODE_ATTR_BIT(va_extflags) |           \
	                        VNODE_ATTR_BIT(va_recursive_gencount))

  
                        
   
#define VNODE_ATTR_RDONLY       (VNODE_ATTR_BIT(va_rdev) |              \
	                        VNODE_ATTR_BIT(va_nlink) |              \
	                        VNODE_ATTR_BIT(va_total_size) |         \
	                        VNODE_ATTR_BIT(va_total_alloc) |        \
	                        VNODE_ATTR_BIT(va_data_alloc) |         \
	                        VNODE_ATTR_BIT(va_iosize) |             \
	                        VNODE_ATTR_BIT(va_fileid) |             \
	                        VNODE_ATTR_BIT(va_linkid) |             \
	                        VNODE_ATTR_BIT(va_parentid) |           \
	                        VNODE_ATTR_BIT(va_fsid) |               \
	                        VNODE_ATTR_BIT(va_filerev) |            \
	                        VNODE_ATTR_BIT(va_gen) |                \
	                        VNODE_ATTR_BIT(va_name) |               \
	                        VNODE_ATTR_BIT(va_type) |               \
	                        VNODE_ATTR_BIT(va_nchildren) |          \
	                        VNODE_ATTR_BIT(va_dirlinkcount) |       \
	                        VNODE_ATTR_BIT(va_devid) |              \
	                        VNODE_ATTR_BIT(va_objtype) |            \
	                        VNODE_ATTR_BIT(va_objtag) |             \
	                        VNODE_ATTR_BIT(va_user_access) |        \
	                        VNODE_ATTR_BIT(va_finderinfo) |         \
	                        VNODE_ATTR_BIT(va_rsrc_length) |        \
	                        VNODE_ATTR_BIT(va_rsrc_alloc) |         \
	                        VNODE_ATTR_BIT(va_fsid64) |             \
	                        VNODE_ATTR_BIT(va_write_gencount) |     \
	                        VNODE_ATTR_BIT(va_private_size) |       \
	                        VNODE_ATTR_BIT(va_clone_id) |           \
	                        VNODE_ATTR_BIT(va_extflags) |           \
	                        VNODE_ATTR_BIT(va_recursive_gencount))

  
                                                       
   
#define VNODE_ATTR_NEWOBJ       (VNODE_ATTR_BIT(va_rdev) |              \
	                        VNODE_ATTR_BIT(va_uid)	|               \
	                        VNODE_ATTR_BIT(va_gid) |                \
	                        VNODE_ATTR_BIT(va_mode) |               \
	                        VNODE_ATTR_BIT(va_flags) |              \
	                        VNODE_ATTR_BIT(va_acl) |                \
	                        VNODE_ATTR_BIT(va_create_time) |        \
	                        VNODE_ATTR_BIT(va_modify_time) |        \
	                        VNODE_ATTR_BIT(va_change_time) |        \
	                        VNODE_ATTR_BIT(va_encoding) |           \
	                        VNODE_ATTR_BIT(va_type) |               \
	                        VNODE_ATTR_BIT(va_uuuid) |              \
	                        VNODE_ATTR_BIT(va_guuid) |              \
	                        VNODE_ATTR_BIT(va_dataprotect_class) |  \
	                        VNODE_ATTR_BIT(va_dataprotect_flags) |  \
	                        VNODE_ATTR_BIT(va_document_id))

#include <sys/_types/_fsid_t.h>

struct vnode_attr {
	               
	uint64_t        va_supported;
	uint64_t        va_active;

	  
                                                        
                                                   
    
	int             va_vaflags;

	                                          
	dev_t           va_rdev;                                           
	uint64_t        va_nlink;                                              
	uint64_t        va_total_size;                                  
	uint64_t        va_total_alloc;                                   
	uint64_t        va_data_size;                                                           
	uint64_t        va_data_alloc;                                                            
	uint32_t        va_iosize;                                 

	                               
	uid_t           va_uid;                        
	gid_t           va_gid;                        
	mode_t          va_mode;                               
	uint32_t        va_flags;                       
	struct kauth_acl *va_acl;                                

	                
	struct timespec va_create_time;                       
	struct timespec va_access_time;                          
	struct timespec va_modify_time;                                     
	struct timespec va_change_time;                                   
	struct timespec va_backup_time;                          

	                     
	uint64_t        va_fileid;                                        
	uint64_t        va_linkid;                               
	uint64_t        va_parentid;                   
	uint32_t        va_fsid;                           
	uint64_t        va_filerev;                                	         
	uint32_t        va_gen;                                    	                        
                                                 
	                     
	uint32_t        va_encoding;                                  

	enum vtype      va_type;                       
	char *          va_name;                                                      
	guid_t          va_uuuid;                            
	guid_t          va_guuid;                            

	                                     
	uint64_t        va_nchildren;                                         
	uint64_t        va_dirlinkcount;                                                                 

#ifdef BSD_KERNEL_PRIVATE
	struct kauth_acl *va_base_acl;
#else
	void *          va_reserved1;
#endif                         
	struct timespec va_addedtime;                                                          

	                            
	uint32_t va_dataprotect_class;                                                        
	uint32_t va_dataprotect_flags;                                               

	                                
	uint32_t va_document_id;

	                          
	uint32_t        va_devid;                                
	uint32_t        va_objtype;                         
	uint32_t        va_objtag;                                   
	uint32_t        va_user_access;                      
	uint8_t         va_finderinfo[32];                       
	uint64_t        va_rsrc_length;                           
	uint64_t        va_rsrc_alloc;                                     
	fsid_t          va_fsid64;                                      

	uint32_t va_write_gencount;                                                             

	uint64_t va_private_size;                                                                          
	uint64_t va_clone_id;                                                                                 
	uint64_t va_extflags;                                        
	uint64_t va_recursive_gencount;                                        

	                              
};

#ifdef BSD_KERNEL_PRIVATE
  
                                 
   
#define VA_DP_RAWENCRYPTED   0x0001
#define VA_DP_RAWUNENCRYPTED 0x0002

#endif

  
                        
   
#define VA_UTIMES_NULL          0x010000                                      
#define VA_EXCLUSIVE            0x020000                                      
#define VA_NOINHERIT            0x040000                                            
#define VA_NOAUTH               0x080000
#define VA_64BITOBJIDS          0x100000                                               
#define VA_REALFSID             0x200000                              
#define VA_USEFSID              0x400000                                       

  
                                                                  
   
#define VSUID   0x800          	                              
#define VSGID   0x400          	                               
#define VSVTX   0x200          	                                      
#define VREAD   0x100          	                                      
#define VWRITE  0x080          
#define VEXEC   0x040          

  
                                                               
                                                                  
   
extern enum vtype       iftovt_tab[];
extern int              vttoif_tab[];
#define IFTOVT(mode)    (iftovt_tab[((mode) & S_IFMT) >> 12])
#define VTTOIF(indx)    (vttoif_tab[(int)(indx)])
#define MAKEIMODE(indx, mode)   (int)(VTTOIF(indx) | (mode))

  
                                    
   
#define SKIPSYSTEM      0x0001                                                  
#define FORCECLOSE      0x0002                                           
#define WRITECLOSE      0x0004                                                  
#define SKIPSWAP        0x0008                                                
#define SKIPROOT        0x0010                                                     

#define DOCLOSE         0x0008                                          

#define V_SAVE          0x0001                                          
#define V_SAVEMETA      0x0002                                                

#define REVOKEALL       0x0001                                               

                              
#define VNODE_REMOVE_NODELETEBUSY                       0x0001                                       
#define VNODE_REMOVE_SKIP_NAMESPACE_EVENT       0x0002                                         
#define VNODE_REMOVE_NO_AUDIT_PATH              0x0004                            
#define VNODE_REMOVE_DATALESS_DIR               0x0008                                                                                 

                         
#define VNODE_READDIR_EXTENDED    0x0001                                       
#define VNODE_READDIR_REQSEEKOFF  0x0002                                       
#define VNODE_READDIR_SEEKOFF32   0x0004                                                 
#define VNODE_READDIR_NAMEMAX     0x0008                                                                   

                           
#define VNODE_CLONEFILE_DEFAULT       0x0000
#define VNODE_CLONEFILE_NOOWNERCOPY   0x0001                                       


#define NULLVP  ((struct vnode *)NULL)

#ifndef BSD_KERNEL_PRIVATE
struct vnodeop_desc;
#endif

extern  int desiredvnodes;                                            


  
                                                               
   
struct vnodeopv_entry_desc {
	struct vnodeop_desc *opve_op;                                
	int (*opve_impl)(void *);                                                     
};
struct vnodeopv_desc {
	                                                     
	int(***opv_desc_vector_p)(void *);
	const struct vnodeopv_entry_desc *opv_desc_ops;                             
};

   
                              
                                                                                           
                   
   
int vn_default_error(void);

  
                       
                                                                     
   
struct vnop_generic_args {
	struct vnodeop_desc *a_desc;
	                                           
};

#include <sys/vnode_if.h>

__BEGIN_DECLS

   
                          
                                            
                                                                                                           
                                            
                                                            
                                                                                         
                                                                                     
                                                
   
errno_t vnode_create(uint32_t flavor, uint32_t size, void  *data, vnode_t *vpp);

#ifdef KERNEL_PRIVATE
   
                                
                                                   
                                                                                  
                                                                               
                                                                       
                                                                                 
                                                                                  
                                                                             
                                                                       
                                                                                     
                                                
   
errno_t vnode_create_empty(vnode_t *vpp);

   
                              
                                                               
                                                                                
                                                                                  
                                                                     
                                            
                                                            
                                                                                         
                                                                                     
                                                
   
errno_t vnode_initialize(uint32_t flavor, uint32_t size, void *data, vnode_t *vpp);
#endif                     

   
                            
                                                                
                                                                                                              
                                                                                                               
                                                                    
                                
                     
   
int     vnode_addfsref(vnode_t vp);

   
                               
                                                                          
                                                                                                                                                                            
                                  
                     
   
int     vnode_removefsref(vnode_t vp);

   
                                
                                                                            
                                                                                                                                                  
                                
                                                          
   
int     vnode_hasdirtyblks(vnode_t vp);

   
                                
                                                                    
                                                                                                                                                  
                                
                                                           
   
int     vnode_hascleanblks(vnode_t vp);

#define VNODE_ASYNC_THROTTLE    15
   
                                 
                                                                                      
                                   
                                                                      
                                      
                                                                                                
                                         
                                                           
   
int     vnode_waitforwrites(vnode_t vp, int output_target, int slpflag, int slptimeout, const char *msg);

   
                              
                                                               
                                                 
   
void    vnode_startwrite(vnode_t vp);

   
                              
                                                                
                                                                                                     
                                                 
   
void    vnode_writedone(vnode_t vp);

   
                         
                                    
                                           
                             
   
enum vtype      vnode_vtype(vnode_t vp);

   
                       
                                                                                                      
                                          
                            
   
uint32_t        vnode_vid(vnode_t vp);

   
                                       
                                                                                          
                                                                    
                                                                   
   
boolean_t vnode_isonexternalstorage(vnode_t vp);

   
                               
                                                                                     
                                                             
                                                                      
   
mount_t vnode_mountedhere(vnode_t vp);

   
                         
                                                                                 
                                            
                                
   
mount_t vnode_mount(vnode_t vp);

   
                            
                                                                                
                                                                                 
                          
   
dev_t   vnode_specrdev(vnode_t vp);

   
                          
                                                                        
                                           
                                                   
   
void *  vnode_fsnode(vnode_t vp);

   
                               
                                                                 
                                                                                                                
                                                
   
void    vnode_clearfsnode(vnode_t vp);

   
                           
                                                                 
                                
                                                             
   
int     vnode_isvroot(vnode_t vp);

   
                            
                                                               
                                
                                                                   
   
int     vnode_issystem(vnode_t vp);

   
                           
                                                                                            
                                                                                        
                                
                                                                 
   
int     vnode_ismount(vnode_t vp);

   
                         
                                                     
                                
                                                              
   
int     vnode_isreg(vnode_t vp);

   
                         
                                                  
                                
                                                              
   
int     vnode_isdir(vnode_t vp);

   
                         
                                                      
                                
                                                              
   
int     vnode_islnk(vnode_t vp);

   
                          
                                                   
                                
                                                               
   
int     vnode_isfifo(vnode_t vp);

   
                         
                                                                  
                                
                                                              
   
int     vnode_isblk(vnode_t vp);

   
                         
                                                                      
                                
                                                              
   
int     vnode_ischr(vnode_t vp);

   
                          
                                                                
                                
                                                                    
   
int     vnode_isswap(vnode_t vp);

   
                                 
                                                     
                                
                                                                
   
int     vnode_isnamedstream(vnode_t vp);

#ifdef KERNEL_PRIVATE
   
                                    
                                                                              
                                                        
                                     
                                                                 
   
errno_t vnode_setasnamedstream(vnode_t vp, vnode_t svp);

   
                                 
                                                                            
                                                              
                                                                        
                                                                 
   
errno_t vnode_setasfirmlink(vnode_t vp, vnode_t target_vp);

   
                               
                                                             
                                 
                                                                                       
                                                                 
   
errno_t vnode_getfirmlink(vnode_t vp, vnode_t *target_vp);

#endif                     

   
                               
                                                                                            
                                                                           
                                
                                                                                                  
   
int     vnode_ismountedon(vnode_t vp);

   
                                
                                                                                              
                                                                           
                                                        
   
void    vnode_setmountedon(vnode_t vp);

   
                                  
                                                                                                
                                                          
   
void    vnode_clearmountedon(vnode_t vp);

   
                              
                                                                                    
                                                                                                                  
                                
                                                                    
   
int     vnode_isrecycled(vnode_t vp);

   
                             
                                                                                                                                       
                                
                                                                          
   
int     vnode_isnocache(vnode_t vp);

   
                          
                                                        
                                
                                                                   
   
int     vnode_israge(vnode_t vp);

   
                                  
                                                                                      
                                
                                                               
   
int     vnode_needssnapshots(vnode_t vp);

   
                              
                                                                                                                          
                                           
   
void    vnode_setnocache(vnode_t vp);

   
                                
                                                                                                                                                      
                                             
   
void    vnode_clearnocache(vnode_t vp);

   
                                 
                                                                                                           
                                
                                                          
   
int     vnode_isnoreadahead(vnode_t vp);

   
                                  
                                                                                              
                                                      
   
void    vnode_setnoreadahead(vnode_t vp);

   
                                    
                                                                                                
                                            
   
void    vnode_clearnoreadahead(vnode_t vp);

   
                                         
                                                                                                    
                                               
                                                                     
   
int     vnode_isfastdevicecandidate(vnode_t vp);

   
                                          
                                                                                                
                                                                                    
                                              
   
void    vnode_setfastdevicecandidate(vnode_t vp);

   
                                            
                                                                                                                   
                                            
   
void    vnode_clearfastdevicecandidate(vnode_t vp);

   
                                   
                                                                                                                     
                                               
                                                                                    
   
int     vnode_isautocandidate(vnode_t vp);

   
                                          
                                                                                                                           
                                                                                    
                                              
   
void    vnode_setautocandidate(vnode_t vp);

   
                                      
                                                                                  
                                            
   
void    vnode_clearautocandidate(vnode_t vp);

                                                                                        

   
                          
                                                    
                                                                                                                                                                                                                                                        
                                         
   
void    vnode_settag(vnode_t vp, int tag);

   
                       
                                                      
                                                                                                                                                                                                                                                          
                                          
                    
   
int     vnode_tag(vnode_t vp);

   
                           
                                   
                                                                                                                                                                 
                                                 
                                                                                                                                                                            
                                          
                                           
   
int     vnode_getattr(vnode_t vp, struct vnode_attr *vap, vfs_context_t ctx);

  
                                               
   
extern uint64_t vnode_get_va_fsid(struct vnode_attr *vap);

   
                           
                                   
                                                                                                                                                                          
                                                
                                                                                                                                                            
                                          
                                           
   
int     vnode_setattr(vnode_t vp, struct vnode_attr *vap, vfs_context_t ctx);

   
                           
                                                     
                                                                
                                                                                                    
   
vnode_t vfs_rootvnode(void);

   
                                       
                                                      
                                                                                                                                                                                                                  
                                             
   
void    vnode_uncache_credentials(vnode_t vp);

   
                                
                                                                                     
                                                                                                                                                                                                                                                    
                                
   
void    vnode_setmultipath(vnode_t vp);

   
                                
                                                                                                          
                                                                     
                               
   
uint32_t  vnode_vfsmaxsymlen(vnode_t vp);

   
                               
                                                                                        
                                                                 
                                                                
   
int     vnode_vfsisrdonly(vnode_t vp);

   
                              
                                                                               
                                                                                                                                                          
                                                    
                                                                        
   
int     vnode_vfstypenum(vnode_t vp);

   
                           
                                                                      
                                                    
                                                                                
   
void    vnode_vfsname(vnode_t vp, char *buf);

   
                                 
                                                                                                                             
                                                    
                                                                                       
   
int     vnode_vfs64bitready(vnode_t vp);

                                                              
int     vfs_context_get_special_port(vfs_context_t, int, ipc_port_t *);
int     vfs_context_set_special_port(vfs_context_t, int, ipc_port_t);

   
                              
                                                                            
                                                        
                                                 
   
proc_t  vfs_context_proc(vfs_context_t ctx);

   
                               
                                                                 
                                                                                                                     
                                                        
                                                            
   
kauth_cred_t    vfs_context_ucred(vfs_context_t ctx);

   
                             
                                                                                    
                                                        
                       
   
int     vfs_context_pid(vfs_context_t ctx);

   
                                  
                                                                                                    
                                                                                                                
                                                        
                                        
   
int     vfs_context_issignal(vfs_context_t ctx, sigset_t mask);

   
                               
                                                                        
                                  
                                                               
   
int     vfs_context_suser(vfs_context_t ctx);

   
                                 
                                                                                
                                  
                                                                 
   
int     vfs_context_is64bit(vfs_context_t ctx);

   
                                
                                                                          
                                                                                           
                                                                               
                                                             
   
vfs_context_t vfs_context_create(vfs_context_t ctx);

   
                              
                                                                              
                                                                                          
                                  
                     
   
int vfs_context_rele(vfs_context_t ctx);

   
                                 
                                                                                                                          
                                                                                                                                                                                            
                                                                                           
   
vfs_context_t vfs_context_current(void);
#ifdef KERNEL_PRIVATE
int     vfs_context_bind(vfs_context_t);

   
                               
                                                                                       
                                                                                                                    
                                                                                                                
                                                                                         
                                              
                                                    
                                                        
   

int     vfs_ctx_skipatime(vfs_context_t ctx);

#endif

                                                                   
#define FS_PRIVATE_TAG_APFS (1)

   
                                       
                                                               
                                                                                                   
                                                                                                          
                                             
                                          
                                                                          
   
int vfs_set_thread_fs_private(uint8_t tag, uint64_t fs_private);

   
                                       
                                                                  
                                                                                         
                                             
                                                  
                                                                          
   
int vfs_get_thread_fs_private(uint8_t tag, uint64_t *fs_private);

   
                    
                                                         
                                             
                                                                                        
                               
                                                                                                                     
                                                                                                                            
                                                                                                                                                          
                                             
                                              
                                       
                                              
                                            
                                                 
                                                                                
   
int     vflush(struct mount *mp, struct vnode *skipvp, int flags);

   
                       
                                              
                                                                                                     
                                                                                                                           
                                                                                               
  
                                                                                                                                                         
                                                                                                                                
                                                                                                                             
                                                                                                                                    
                                                                                                                
                                                                                            
                                            
  
                                                                                         
   
int     vnode_get(vnode_t);

   
                              
                                                                                                                            
                                                                                                            
                                                                                                                                                
                                                                                                                                                 
                                                                                                                                                          
                                                      
  
                                                                                                                                                                 
                                                                                                                                
                                                                  
                                                                                                                           
   
int     vnode_getwithvid(vnode_t, uint32_t);

#ifdef BSD_KERNEL_PRIVATE
int vnode_getwithvid_drainok(vnode_t, uint32_t);
#endif                         

   
                              
                                                                                                 
                                                                                                            
                                                                                                                                     
                                                                                                                
                                                                                                                                
                                                                                                                                                                
                                                                                                                                
                                                                  
                                                                                                                           
   
int     vnode_getwithref(vnode_t vp);

   
                       
                                               
                                                                                                                           
                                                                                                                       
                                              
                     
   
int     vnode_put(vnode_t vp);

   
                       
                                                
                                                                                                                                   
                                                                                                                                   
                                                                                                                                    
                                                                                                                                 
                                                                                                                                       
                                                                  
                                                                                                                                         
   
int     vnode_ref(vnode_t vp);

   
                        
                                                
                                                                                                                
                                                                                                                   
                                                                                        
                                               
   
void    vnode_rele(vnode_t vp);

   
                           
                                                                                                                   
                                                                                                                         
                                                                                                                      
                                                                                                                                      
                                          
                                                                                                                                          
                                                                         
                                                  
                                                                  
   
int     vnode_isinuse(vnode_t vp, int refcnt);

   
                           
                                                                   
                                                                                        
                                                                                              
                                   
                                                                                                                              
   
int     vnode_recycle(vnode_t vp);

#ifdef KERNEL_PRIVATE

#define VNODE_EVENT_DELETE              0x00000001                            
#define VNODE_EVENT_WRITE               0x00000002                                              
#define VNODE_EVENT_EXTEND              0x00000004                              
#define VNODE_EVENT_ATTRIB              0x00000008                                                                               
#define VNODE_EVENT_LINK                0x00000010                              
#define VNODE_EVENT_RENAME              0x00000020                             
#define VNODE_EVENT_PERMS               0x00000040                                                         
#define VNODE_EVENT_FILE_CREATED        0x00000080                                                            
#define VNODE_EVENT_DIR_CREATED         0x00000100                                                                          
#define VNODE_EVENT_FILE_REMOVED        0x00000200                                                                   
#define VNODE_EVENT_DIR_REMOVED         0x00000400                                                                   

#ifdef BSD_KERNEL_PRIVATE
#define VNODE_NOTIFY_ATTRS              (VNODE_ATTR_BIT(va_fsid) | \
	                                VNODE_ATTR_BIT(va_fileid)| \
	                                VNODE_ATTR_BIT(va_mode)  | \
	                                VNODE_ATTR_BIT(va_uid)   | \
	                                VNODE_ATTR_BIT(va_gid)   | \
	                                VNODE_ATTR_BIT(va_dirlinkcount) | \
	                                VNODE_ATTR_BIT(va_nlink))



#endif                          

   
                               
                                                                                           
                     
                               
                                                
                                                        
   
int     vnode_ismonitored(vnode_t vp);


   
                                     
                                                               
                               
                                                
                                                                
   
int     vnode_isdyldsharedcache(vnode_t vp);


   
                                 
                                                                   
                                                                                                                     
                                                      
                                      
                                                                                            
                             
                                                                                                      
   
int     vn_authorize_unlink(vnode_t dvp, vnode_t vp, struct componentname *cnp, vfs_context_t ctx, void *reserved);


   
                                
                                                                  
                                                                                                                    
                                                           
                                      
                                                                                            
                             
                                                                                                     
   
int     vn_authorize_rmdir(vnode_t dvp, vnode_t vp, struct componentname *cnp, vfs_context_t ctx, void *reserved);

   
                                
                                                                             
                                                                                                                             
                                                                                                                   
                                                                            
                                     
                                                 
                                                                                                                             
                                
                                      
   
int     vn_getpath_fsenter(struct vnode *vp, char *pathbuf, int *len);

   
                                    
                                                                            
                                                                                                                             
                                                                                                                         
                                     
                                                 
                                                                                                                             
                                
                                      
   
int     vn_getpath_no_firmlink(struct vnode *vp, char *pathbuf, int *len);

   
                                            
                                                                                                                         
                                                                                                             
                                                                                                              
                                                          
                                                                       
                                                                    
                                                 
                                                                                                                             
                                
                                      
   
int     vn_getpath_fsenter_with_parent(struct vnode *dvp, struct vnode *vp, char *pathbuf, int *len);

   
                            
                                                                                                           
                                                                                                                             
                                                                                                                   
                                                                            
                                     
                                                                                     
                                                 
                                                                                                                             
                                
                                                
                                      
   
int     vn_getpath_ext(struct vnode *vp, struct vnode *dvp, char *pathbuf, int *len, int flags);

   
                                        
                                                                                                           
                                                                                                                             
                                                                                                                   
                                                                            
                                     
                                                                                     
                                                 
                                                                                                                             
                                
                                                                                                                                         
                                
                                                
                                      
   
int     vn_getpath_ext_with_mntlen(struct vnode *vp, struct vnode *dvp, char *pathbuf, size_t *len, size_t *mntlen, int flags);

                                        
#define VN_GETPATH_FSENTER              0x0001                              
#define VN_GETPATH_NO_FIRMLINK          0x0002
#define VN_GETPATH_VOLUME_RELATIVE      0x0004                                          
#define VN_GETPATH_NO_PROCROOT          0x0008                                               

#endif                     

#define VNODE_UPDATE_PARENT     0x01
#define VNODE_UPDATE_NAMEDSTREAM_PARENT VNODE_UPDATE_PARENT
#define VNODE_UPDATE_NAME       0x02
#define VNODE_UPDATE_CACHE      0x04
#define VNODE_UPDATE_PURGE      0x08
#ifdef BSD_KERNEL_PRIVATE
#define VNODE_UPDATE_PURGEFIRMLINK      0x10
#endif
   
                                   
                                                              
                                                                                                                                             
                                                                                                                                            
                        
                                                    
                                                                         
                                                                                                                                                  
                                                                                                            
                                                                                                         
                                                                                                          
                                                                                                                                       
                                                                                                                 
   
void    vnode_update_identity(vnode_t vp, vnode_t dvp, const char *name, int name_len, uint32_t name_hashval, int flags);

   
                       
                                                              
                                                                                                                                
                                                 
                                                   
                                           
   
int     vn_bwrite(struct vnop_bwrite_args *ap);

   
                             
                                                        
                                                                                        
                                                 
                                           
                                                                                        
                                                      
                                                                                                        
   
int     vnode_authorize(vnode_t vp, vnode_t dvp, kauth_action_t action, vfs_context_t ctx);

#ifdef KERNEL_PRIVATE
   
                                       
                                                                                                                              
                                                                                                
                         
                                                                             
                                                                        
                                                                                        
                                                      
                                                                                                                                          
   
#define VNODE_ATTR_AUTHORIZE_AVAILABLE          0x01
int     vnode_attr_authorize_init(struct vnode_attr *vap, struct vnode_attr *dvap, kauth_action_t action, vfs_context_t ctx);

   
                                  
                                                                                             
                                                                                                                            
                                                                             
                                                                        
                                                                           
                                                                                        
                                                      
                                                                                                        
   
int     vnode_attr_authorize(struct vnode_attr *vap, struct vnode_attr *dvap, mount_t mp, kauth_action_t action, vfs_context_t ctx);
#endif                     

   
                            
                                                                                                                                   
                                                                                                                                       
                                                                                                                                
                        
                                                     
                                                                                                  
                                                              
                                                      
                                                                                
   
int     vnode_authattr(vnode_t vp, struct vnode_attr *vap, kauth_action_t *actionp, vfs_context_t ctx);

   
                                
                                                                                                   
                                                                                                                                            
                                                          
                                                          
                                                                                                  
                                                                                         
                                                      
                                                                                                  
   
int     vnode_authattr_new(vnode_t dvp, struct vnode_attr *vap, int noauth, vfs_context_t ctx);

   
                         
                                                       
                                                                                                                                                              
                                                                                  
                                 
                                                                                    
                                                           
                                                          
   
errno_t vnode_close(vnode_t vp, int flags, vfs_context_t ctx);

   
                        
                                            
                                                                                                                             
                                                                                                                 
                                             
                                                                         
                                                                                                                             
                                
                                           
   
int vn_getpath(struct vnode *vp, char *pathbuf, int *len);

   
                          
                                            
                                                      
                                                                   
                                                                       
                                                                                         
                                                                                        
                                                                                         
                                    
                                              
   
int     vnode_notify(vnode_t vp, uint32_t events, struct vnode_attr *vap);

   
                                       
                                                                                                   
                                                                           
                                                
                                                                         
   
int     vfs_get_notify_attributes(struct vnode_attr *vap);

  
                                            
   
#define VNODE_LOOKUP_NOFOLLOW           0x01
#define VNODE_LOOKUP_NOCROSSMOUNT       0x02
#define VNODE_LOOKUP_CROSSMOUNTNOWAIT   0x04
   
                          
                                          
                                                                                                  
                                                                                                  
                                
                                                                                                                            
                                                   
   
errno_t vnode_lookup(const char *path, int flags, vnode_t *vpp, vfs_context_t ctx);

#ifdef KERNEL_PRIVATE
   
                                                                                     
                                          
                                                                                                  
                                                                                                  
                                
                                                                                                                            
                                                                                                                             
                                 
                                                   
   
errno_t vnode_lookupat(const char *path, int flags, vnode_t *vpp, vfs_context_t ctx, vnode_t start_dvp);
#endif

   
                        
                                                                                      
                                                                                               
                                                                    
                                
                                                                
                                                                            
                                        
                                     
                                                             
                                           
   
errno_t vnode_open(const char *path, int fmode, int cmode, int flags, vnode_t *vpp, vfs_context_t ctx);

  
                            
   

   
                           
                                                                                  
                                               
                
                                                    
              
                                                                            
                             
                                    
                             
                               
                                          
                                          
                                     
  
                                                  
                                                                              
                                                                                                                          
                                                                                                   
   
int     vnode_iterate(struct mount *mp, int flags, int (*callout)(struct vnode *, void *), void *arg);

  
                                  
   
#define VNODE_RELOAD                    0x01
#define VNODE_WAIT                              0x02
#define VNODE_WRITEABLE                 0x04
#define VNODE_WITHID                    0x08
#define VNODE_NOLOCK_INTERNAL   0x10
#define VNODE_NODEAD                    0x20
#define VNODE_NOSUSPEND                 0x40
#define VNODE_ITERATE_ALL               0x80
#define VNODE_ITERATE_ACTIVE    0x100
#define VNODE_ITERATE_INACTIVE  0x200
#ifdef BSD_KERNEL_PRIVATE
#define VNODE_ALWAYS            0x400
#define VNODE_DRAINO            0x800
#define VNODE_PAGER             0x1000
#endif                         

  
                              
   
#define VNODE_RETURNED          0                                                      
#define VNODE_RETURNED_DONE     1                                                                           
#define VNODE_CLAIMED           2                                 
#define VNODE_CLAIMED_DONE      3                                                      

   
                       
                                                   
                                                                                                                         
                                                                                                                    
                                                                                           
                                  
                        
                                                           
                     
   
int     vn_revoke(vnode_t vp, int flags, vfs_context_t ctx);

                                   
   
                          
                                                                           
                                                                                                                                     
                                                                                                                                         
                                                                                          
                                                      
                                             
                                                                               
                                                                                                                        
                                                                                                                                                
   
int     cache_lookup(vnode_t dvp, vnode_t *vpp, struct componentname *cnp);

   
                         
                                                            
                                                                                                          
                                                           
                                             
                                                                                                 
                                                                                 
                                                                               
   
void    cache_enter(vnode_t dvp, vnode_t vp, struct componentname *cnp);

   
                         
                                                                     
                                                                                                          
                                                                                     
                                 
   
void    cache_purge(vnode_t vp);

   
                                   
                                                                                    
                                                                                             
                                             
                                                         
   
void    cache_purge_negatives(vnode_t vp);


  
                                                               
                                                                
                                                
   
   
                         
                        
                                                                                                    
   
const char *vfs_addname(const char *name, uint32_t len, uint32_t nc_hash, uint32_t flags);

   
                            
                        
                                                                                                    
   
int   vfs_removename(const char *name);

   
                    
                                                                                                                              
                                                                                                               
                                                                                                                             
                                                     
                                                  
                                
   
int     vcount(vnode_t vp);

   
                                   
                                                                     
                                                                                      
                     
                              
                                       
                                                                                                                    
                                         
                                                                                                                      
                              
   
int vn_path_package_check(vnode_t vp, char *path, int pathlen, int *component);

#ifdef KERNEL_PRIVATE
   
                                            
                                                                                
                                  
                                   
                                              
   
int vn_searchfs_inappropriate_name(const char *name, int len);
#endif

   
                     
                                           
                                                                                                            
                              
                                                         
                                                
                                                                               
                                
                                                               
                                                                                                                                                    
                                                                                                                                
                                                                
                                                                         
                                                                                                       
                                    
                                                                                                                                   
   
int     vn_rdwr(enum uio_rw rw, struct vnode *vp, caddr_t base, int len, off_t offset, enum uio_seg segflg, int ioflg, kauth_cred_t cred, int *aresid, proc_t p);

   
                           
                                                             
                                                                                                                               
                                                                                                                                 
                                           
                                             
   
const char      *vnode_getname(vnode_t vp);

   
                           
                                                               
                                                                           
                                  
   
void    vnode_putname(const char *name);

   
                             
                                                      
                                                                                                                
                                                                                                                 
                                             
                                             
                                           
   
vnode_t vnode_getparent(vnode_t vp);

   
                            
                                                                                                   
                                                                                                                                  
                                        
                                               
   
int     vnode_setdirty(vnode_t vp);

   
                              
                                                                                    
                                                                                                                                   
                                               
                                               
   
int     vnode_cleardirty(vnode_t vp);

   
                           
                                                   
                                                                                                                                   
                                
                                                        
   
int     vnode_isdirty(vnode_t vp);

#ifdef KERNEL_PRIVATE
   
                                          
                                                                                             
                                                                                         
                                                                                              
                        
                                                              
                                                              
                                                                                                  
   
int vnode_lookup_continue_needed(vnode_t vp, struct componentname *cnp);

   
                         
                                                                   
                               
                                                                                        
   
int vnode_istty(vnode_t vp);

   
                    
                                              
                                                                    
   
int bdevvp(dev_t dev, struct vnode **vpp);

  
                             
                                                
                                                                    
   
int vnode_getfromfd(vfs_context_t ctx, int fd, vnode_t *vpp);

#endif                     

#ifdef BSD_KERNEL_PRIVATE
                                          
struct stat;
int     vn_stat(struct vnode *vp, void * sb, kauth_filesec_t *xsec, int isstat64, int needsrealdev,
    vfs_context_t ctx);
int     vn_stat_noauth(struct vnode *vp, void * sb, kauth_filesec_t *xsec, int isstat64, int needsrealdev,
    vfs_context_t ctx, struct ucred *file_cred);
int     vaccess(mode_t file_mode, uid_t uid, gid_t gid,
    mode_t acc_mode, kauth_cred_t cred);
int     check_mountedon(dev_t dev, enum vtype type, int  *errorp);
int vn_getcdhash(struct vnode *vp, off_t offset, unsigned char *cdhash);
void    vnode_reclaim(vnode_t);
vnode_t current_workingdir(void);
void    *vnode_vfsfsprivate(vnode_t);
struct vfsstatfs *vnode_vfsstatfs(vnode_t);
uint32_t vnode_vfsvisflags(vnode_t);
uint32_t vnode_vfscmdflags(vnode_t);
int     vnode_is_openevt(vnode_t);
void    vnode_set_openevt(vnode_t);
void    vnode_clear_openevt(vnode_t);
int     vnode_isstandard(vnode_t);
int     vnode_makeimode(int, int);
enum vtype      vnode_iftovt(int);
int     vnode_vttoif(enum vtype);
int     vnode_isshadow(vnode_t);
boolean_t vnode_on_reliable_media(vnode_t);
  
                                                                      
                                                                    
                                                     
   
vnode_t vnode_parent(vnode_t);
void vnode_setparent(vnode_t, vnode_t);
void vnode_setname(vnode_t, char *);
                                                                  
thread_t vfs_context_thread(vfs_context_t);
#if CONFIG_IOSCHED
vnode_t vnode_mountdevvp(vnode_t);
#endif
#endif                         

#ifdef KERNEL_PRIVATE
   
                                     
                                                       
                                                                                                                                                                                                                                                                                                                                                                   
                                           
                               
   
const char *vnode_getname_printable(vnode_t vp);
   
                                     
                                                                                                                              
                                  
   
void vnode_putname_printable(const char *name);
#endif                  

   
                                   
                                                                                            
                                                                                                                                                             
                                            
                                                                    
                                                                                              
   
int vnode_getbackingvnode(vnode_t in_vp, vnode_t* out_vpp);

  
                                                                          
   

   
                                           
                                                                       
                                                                                                                                                                          
                                                                                              
                                                   
                                               
                                                                                                                                                                                                                                                                                
                                                                                                                                                 
                                     
                  
   
errno_t vfs_setup_vattr_from_attrlist(struct attrlist *alp, struct vnode_attr *vap, enum vtype obj_vtype, ssize_t *attr_fixed_sizep, vfs_context_t ctx);

   
                           
                                                                                             
                                                                                                                                               
                                                                                                                                                                                         
                                                     
                                                  
                                                                               
                                                                                                                                                                                                       
                                  
                                     
                  
   
errno_t vfs_attr_pack(vnode_t vp, uio_t uio, struct attrlist *alp, uint64_t options, struct vnode_attr *vap, void *fndesc, vfs_context_t ctx);

   
                              
                                                                                             
                                                                                                                                               
                                                                                           
                                                                                                                                                                                         
                                                     
                                                  
                                                                               
                                                                                                                                                                                                       
                                  
                                     
                  
   
errno_t vfs_attr_pack_ext(mount_t mp, vnode_t vp, uio_t uio, struct attrlist *alp, uint64_t options, struct vnode_attr *vap, void *fndesc, vfs_context_t ctx);

#ifdef KERNEL_PRIVATE

                                                                        
vm_offset_t kdebug_vnode(vnode_t vp);
int vn_pathconf(vnode_t, int, int32_t *, vfs_context_t);
int vnode_should_flush_after_write(vnode_t vp, int ioflag);
void vfs_setowner(mount_t mp, uid_t uid, gid_t gid);
uint64_t vfs_idle_time(mount_t mp);
                                    
#ifndef vnode_usecount
int vnode_usecount(vnode_t vp);
#endif
int vnode_iocount(vnode_t vp);
void vnode_rele_ext(vnode_t, int, int);
int is_package_name(const char *name, int len);
int     vfs_context_issuser(vfs_context_t);
int vfs_context_iskernel(vfs_context_t);
vfs_context_t vfs_context_kernel(void);                                         
#ifdef XNU_KERNEL_PRIVATE
void vfs_set_context_kernel(vfs_context_t);                                     
#endif                         
vnode_t vfs_context_cwd(vfs_context_t);
vnode_t vfs_context_get_cwd(vfs_context_t);                           
int vnode_isnoflush(vnode_t);
void vnode_setnoflush(vnode_t);
void vnode_clearnoflush(vnode_t);
#if CONFIG_IO_COMPRESSION_STATS
void vnode_iocs_record_and_free(vnode_t);
#endif                                  

#define BUILDPATH_NO_FS_ENTER     0x1                                                 
#define BUILDPATH_CHECKACCESS     0x2                                          
#define BUILDPATH_CHECK_MOVED     0x4                                                        
#define BUILDPATH_VOLUME_RELATIVE 0x8                                                      
#define BUILDPATH_NO_FIRMLINK     0x10                                 
#define BUILDPATH_NO_PROCROOT     0x20                                                                

int     build_path(vnode_t first_vp, char *buff, int buflen, int *outlen, int flags, vfs_context_t ctx);

int vnode_issubdir(vnode_t vp, vnode_t dvp, int *is_subdir, vfs_context_t ctx);

#endif                  

__END_DECLS

#endif             

  
                                                 
   

#define IOCS_BUFFER_NUM_SIZE_BUCKETS         10
#define IOCS_BUFFER_MAX_BUCKET               9
#define IOCS_BUFFER_NUM_COMPRESSION_BUCKETS  7
#define IOCS_BLOCK_NUM_SIZE_BUCKETS          16

struct io_compression_stats {
	uint64_t uncompressed_size;
	uint64_t compressed_size;
	uint32_t buffer_size_compression_dist[IOCS_BUFFER_NUM_SIZE_BUCKETS][IOCS_BUFFER_NUM_COMPRESSION_BUCKETS];
	uint32_t block_compressed_size_dist[IOCS_BLOCK_NUM_SIZE_BUCKETS];
};
typedef struct io_compression_stats *io_compression_stats_t;

#define IOCS_SBE_PATH_LEN             128
#define IOCS_PATH_START_BYTES_TO_COPY 108
#define IOCS_PATH_END_BYTES_TO_COPY   20                                

#define IOCS_SYSCTL_LIVE                  0x00000001
#define IOCS_SYSCTL_STORE_BUFFER_RD_ONLY  0x00000002
#define IOCS_SYSCTL_STORE_BUFFER_MARK     0x00000004

struct iocs_store_buffer_entry {
	char     path_name[IOCS_SBE_PATH_LEN];
	struct io_compression_stats iocs;
};

#endif                 
