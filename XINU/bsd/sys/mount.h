/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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
 * Copyright (c) 1989, 1991, 1993
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
 *	@(#)mount.h	8.21 (Berkeley) 5/20/95
 */
  
                                                                      
                                                                          
                                                                        
               
   


#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>
#include <sys/attr.h>                                                   
#include <os/base.h>

#ifndef KERNEL
#include <stdint.h>
#include <sys/ucred.h>
#include <sys/queue.h>                                          
#include <Availability.h>
#else
#include <sys/kernel_types.h>
#include <uuid/uuid.h>
#endif

#include <sys/_types/_fsid_t.h>                          

  
                         
   

#define MFSNAMELEN      15                                                 
#define MFSTYPENAMELEN  16                                                 

#if __DARWIN_64_BIT_INO_T
#define MNAMELEN        MAXPATHLEN                                              
#else                              
#define MNAMELEN        90                                                      
#endif                            

#define MNT_EXT_ROOT_DATA_VOL      0x00000001                                            

#define __DARWIN_STRUCT_STATFS64 { \
	uint32_t	f_bsize;                                                 \
	int32_t		f_iosize;                                         \
	uint64_t	f_blocks;                                              \
	uint64_t	f_bfree;                                \
	uint64_t	f_bavail;                                                \
	uint64_t	f_files;                                              \
	uint64_t	f_ffree;                                    \
	fsid_t		f_fsid;                              \
	uid_t		f_owner;                                               \
	uint32_t	f_type;                                  \
	uint32_t	f_flags;                                           \
	uint32_t	f_fssubtype;                               \
	char		f_fstypename[MFSTYPENAMELEN];                      \
	char		f_mntonname[MAXPATHLEN];                                         \
	char		f_mntfromname[MAXPATHLEN];                               \
	uint32_t    f_flags_ext;                         \
	uint32_t	f_reserved[7];                       \
}

#if !__DARWIN_ONLY_64_BIT_INO_T

struct statfs64 __DARWIN_STRUCT_STATFS64;

#endif                                  

#if __DARWIN_64_BIT_INO_T

struct statfs __DARWIN_STRUCT_STATFS64;

#else                             

  
                                                                                     
   
struct statfs {
	short   f_otype;                                                     
	short   f_oflags;                                                     
	long    f_bsize;                                                        
	long    f_iosize;                                                
	long    f_blocks;                                                     
	long    f_bfree;                                       
	long    f_bavail;                                                       
	long    f_files;                                                     
	long    f_ffree;                                           
	fsid_t  f_fsid;                                     
	uid_t   f_owner;                                                      
	short   f_reserved1;                         
	short   f_type;                                         
	long    f_flags;                                                  
	long    f_reserved2[2];                              
	char    f_fstypename[MFSNAMELEN];                   
	char    f_mntonname[MNAMELEN];                                  
	char    f_mntfromname[MNAMELEN];                        
	char    f_reserved3;                       
	long    f_reserved4[4];                     
};

#endif                            

#pragma pack(4)

struct vfsstatfs {
	uint32_t        f_bsize;                                                
	size_t          f_iosize;                                        
	uint64_t        f_blocks;                                             
	uint64_t        f_bfree;                               
	uint64_t        f_bavail;                                               
	uint64_t        f_bused;                                                
	uint64_t        f_files;                                             
	uint64_t        f_ffree;                                   
	fsid_t          f_fsid;                             
	uid_t           f_owner;                                              
	uint64_t        f_flags;                                          
	char            f_fstypename[MFSTYPENAMELEN];                         
	char            f_mntonname[MAXPATHLEN];                                
	char            f_mntfromname[MAXPATHLEN];                        
	uint32_t        f_fssubtype;                               
	void            *f_reserved[2];                                  
};

#pragma pack()

#ifdef KERNEL
  
                                                                       
                                                                   
                      
   

#define VFSATTR_INIT(s)                 ((s)->f_supported = (s)->f_active = 0LL)
#define VFSATTR_SET_SUPPORTED(s, a)     ((s)->f_supported |= VFSATTR_ ## a)
#define VFSATTR_IS_SUPPORTED(s, a)      ((s)->f_supported & VFSATTR_ ## a)
#define VFSATTR_CLEAR_ACTIVE(s, a)      ((s)->f_active &= ~VFSATTR_ ## a)
#define VFSATTR_IS_ACTIVE(s, a)         ((s)->f_active & VFSATTR_ ## a)
#define VFSATTR_ALL_SUPPORTED(s)        (((s)->f_active & (s)->f_supported) == (s)->f_active)
#define VFSATTR_WANTED(s, a)            ((s)->f_active |= VFSATTR_ ## a)
#define VFSATTR_RETURN(s, a, x)         do { (s)-> a = (x); VFSATTR_SET_SUPPORTED(s, a);} while(0)

#define VFSATTR_f_objcount              (1LL<<  0)
#define VFSATTR_f_filecount             (1LL<<  1)
#define VFSATTR_f_dircount              (1LL<<  2)
#define VFSATTR_f_maxobjcount           (1LL<<  3)
#define VFSATTR_f_bsize                 (1LL<< 4)
#define VFSATTR_f_iosize                (1LL<<  5)
#define VFSATTR_f_blocks                (1LL<<  6)
#define VFSATTR_f_bfree                 (1LL<<  7)
#define VFSATTR_f_bavail                (1LL<<  8)
#define VFSATTR_f_bused                 (1LL<<  9)
#define VFSATTR_f_files                 (1LL<< 10)
#define VFSATTR_f_ffree                 (1LL<< 11)
#define VFSATTR_f_fsid                  (1LL<< 12)
#define VFSATTR_f_owner                 (1LL<< 13)
#define VFSATTR_f_capabilities          (1LL<< 14)
#define VFSATTR_f_attributes            (1LL<< 15)
#define VFSATTR_f_create_time           (1LL<< 16)
#define VFSATTR_f_modify_time           (1LL<< 17)
#define VFSATTR_f_access_time           (1LL<< 18)
#define VFSATTR_f_backup_time           (1LL<< 19)
#define VFSATTR_f_fssubtype             (1LL<< 20)
#define VFSATTR_f_vol_name              (1LL<< 21)
#define VFSATTR_f_signature             (1LL<< 22)
#define VFSATTR_f_carbon_fsid           (1LL<< 23)
#define VFSATTR_f_uuid                  (1LL<< 24)
#define VFSATTR_f_quota         (1LL<< 25)
#define VFSATTR_f_reserved              (1LL<< 26)


  
                      
   
#pragma pack(4)
  
                                                       
                                                   
                                                       
                                               
   
struct vfs_attr {
	uint64_t        f_supported;
	uint64_t        f_active;

	uint64_t        f_objcount;                                                 
	uint64_t        f_filecount;                   
	uint64_t        f_dircount;                          
	uint64_t        f_maxobjcount;                                            

	uint32_t        f_bsize;                                                  
	size_t          f_iosize;                                        
	uint64_t        f_blocks;                                             
	uint64_t        f_bfree;                               
	uint64_t        f_bavail;                                               
	uint64_t        f_bused;                           
	uint64_t        f_files;                                             
	uint64_t        f_ffree;                                   
	fsid_t          f_fsid;                             
	uid_t           f_owner;                                              

	vol_capabilities_attr_t f_capabilities;
	vol_attributes_attr_t f_attributes;

	struct timespec f_create_time;                     
	struct timespec f_modify_time;                              
	struct timespec f_access_time;                           
	struct timespec f_backup_time;                        

	uint32_t        f_fssubtype;                            

	char            *f_vol_name;                     

	uint16_t        f_signature;                                                                      
	uint16_t        f_carbon_fsid;                                                  
	uuid_t          f_uuid;                                                                             
	uint64_t        f_quota;                                                    
	uint64_t        f_reserved;                                                    
};

#pragma pack()

#endif              

  
                          
  
                               
   
#define MNT_RDONLY      0x00000001                                
#define MNT_SYNCHRONOUS 0x00000002                                             
#define MNT_NOEXEC      0x00000004                                      
#define MNT_NOSUID      0x00000008                                         
#define MNT_NODEV       0x00000010                                         
#define MNT_UNION       0x00000020                                            
#define MNT_ASYNC       0x00000040                                              
#define MNT_CPROTECT    0x00000080                                                   

  
                                  
   
#define MNT_EXPORTED    0x00000100                                   

  
                                                                    
   

#define MNT_REMOVABLE   0x00000200

  
                                   
   
#define MNT_QUARANTINE  0x00000400                                      

  
                                    
   
#define MNT_LOCAL       0x00001000                                        
#define MNT_QUOTA       0x00002000                                            
#define MNT_ROOTFS      0x00004000                                          
#define MNT_DOVOLFS     0x00008000                                                                


#define MNT_DONTBROWSE  0x00100000                                                            
#define MNT_IGNORE_OWNERSHIP 0x00200000                                                                  
#define MNT_AUTOMOUNTED 0x00400000                                                 
#define MNT_JOURNALED   0x00800000                                   
#define MNT_NOUSERXATTR 0x01000000                                                
#define MNT_DEFWRITE    0x02000000                                          
#define MNT_MULTILABEL  0x04000000                                             
#define MNT_NOATIME             0x10000000                                              
#define MNT_SNAPSHOT    0x40000000                              
#define MNT_STRICTATIME 0x80000000                                                    
#ifdef BSD_KERNEL_PRIVATE
                                                             
#endif                         

                                  
#define MNT_UNKNOWNPERMISSIONS MNT_IGNORE_OWNERSHIP


  
                                                           
                                                            
   
#define MNT_VISFLAGMASK (MNT_RDONLY	| MNT_SYNCHRONOUS | MNT_NOEXEC	| \
	                MNT_NOSUID	| MNT_NODEV	| MNT_UNION	| \
	                MNT_ASYNC	| MNT_EXPORTED	| MNT_QUARANTINE | \
	                MNT_LOCAL	| MNT_QUOTA | MNT_REMOVABLE | \
	                MNT_ROOTFS	| MNT_DOVOLFS	| MNT_DONTBROWSE | \
	                MNT_IGNORE_OWNERSHIP | MNT_AUTOMOUNTED | MNT_JOURNALED | \
	                MNT_NOUSERXATTR | MNT_DEFWRITE	| MNT_MULTILABEL | \
	                MNT_NOATIME | MNT_STRICTATIME | MNT_SNAPSHOT | MNT_CPROTECT)
  
                                              
                                      
                                                                
                                     
   
#define MNT_UPDATE      0x00010000                                            
#define MNT_NOBLOCK     0x00020000                                                 
#define MNT_RELOAD      0x00040000                                  
#define MNT_FORCE       0x00080000                                            
#define MNT_CMDFLAGS    (MNT_UPDATE|MNT_NOBLOCK|MNT_RELOAD|MNT_FORCE)



  
                              
  
                                                                   
                                                                    
   
#define VFS_GENERIC             0                                           
#define VFS_NUMMNTOPS           1                                                           
  
                                                                 
                                                                
                               
   
#define VFS_MAXTYPENUM  1                                                 
#define VFS_CONF        2                                              
                                                

  
                                            
  
                                              
   
#define MNT_WAIT        1                                                       
#define MNT_NOWAIT      2                                                  
#define MNT_DWAIT       4                                                       
#ifdef KERNEL
                       
#define MNT_VOLUME      8                                                 
#endif


#if !defined(KERNEL) && !defined(_KERN_SYS_KERNELTYPES_H_)                                     
struct mount;
typedef struct mount * mount_t;
struct vnode;
typedef struct vnode * vnode_t;
#endif

                                                   
struct vfsconf {
	uint32_t vfc_reserved1;                     
	char    vfc_name[MFSNAMELEN];                             
	int     vfc_typenum;                                                 
	int     vfc_refcount;                                            
	int     vfc_flags;                                   
	uint32_t vfc_reserved2;                     
	uint32_t vfc_reserved3;                     
};

struct vfsidctl {
	int             vc_vers;                                              
	fsid_t          vc_fsid;                                 
	void            *vc_ptr;                                        
	size_t          vc_len;                                     
	u_int32_t       vc_spare[12];                              
};


                           
#define VFS_CTL_VERS1   0x01

#ifdef KERNEL
struct user_vfsidctl {
	int                              vc_vers;                                             
	fsid_t                   vc_fsid;                                        
	user_addr_t              vc_ptr __attribute((aligned(8)));                                      
	user_size_t              vc_len;                                            
	u_int32_t                vc_spare[12];                             
};

struct user32_vfsidctl {
	int                              vc_vers;                                             
	fsid_t                   vc_fsid;                                        
	user32_addr_t    vc_ptr;                                                
	user32_size_t    vc_len;                                            
	u_int32_t                vc_spare[12];                             
};

union union_vfsidctl {                                                    
	struct user32_vfsidctl vc32;
	struct user_vfsidctl vc64;
};

#endif             

  
                                                                      
                   
   
#define VFS_CTL_OSTATFS 0x00010001                             
#define VFS_CTL_UMOUNT  0x00010002                   
#define VFS_CTL_QUERY   0x00010003                                      
#define VFS_CTL_NEWADDR 0x00010004                                    
#define VFS_CTL_TIMEO   0x00010005                                            
#define VFS_CTL_NOLOCKS 0x00010006                                
#define VFS_CTL_SADDR   0x00010007                              
#define VFS_CTL_DISC    0x00010008                               
#define VFS_CTL_SERVERINFO  0x00010009                                   
#define VFS_CTL_NSTATUS 0x0001000A                              
#define VFS_CTL_STATFS64 0x0001000B                   

#ifndef KERNEL
  
                                                                 
                                                      
   
#if __DARWIN_64_BIT_INO_T
#define VFS_CTL_STATFS  VFS_CTL_STATFS64
#else
#define VFS_CTL_STATFS  VFS_CTL_OSTATFS
#endif
#endif               

struct vfsquery {
	u_int32_t       vq_flags;
	u_int32_t       vq_spare[31];
};

struct vfs_server {
	int32_t  vs_minutes;                                                         
	u_int8_t vs_server_name[MAXHOSTNAMELEN * 3];                                                    
};

  
                                                   
   
struct netfs_status {
	u_int32_t       ns_status;                                                         
	char            ns_mountopts[512];                                  
	uint32_t        ns_waittime;                                           
	uint32_t        ns_threadcount;                                                      
	uint64_t        ns_threadids[0];                                              
};

                    
#define VQ_NOTRESP      0x0001                   
#define VQ_NEEDAUTH     0x0002                       
#define VQ_LOWDISK      0x0004                          
#define VQ_MOUNT        0x0008                              
#define VQ_UNMOUNT      0x0010                           
#define VQ_DEAD         0x0020                                               
#define VQ_ASSIST       0x0040                                                         
#define VQ_NOTRESPLOCK  0x0080                         
#define VQ_UPDATE       0x0100                                          
#define VQ_VERYLOWDISK  0x0200                                                     
#define VQ_SYNCEVENT    0x0400                                                                       
#define VQ_SERVEREVENT  0x0800                                          
#define VQ_QUOTA        0x1000                                 
#define VQ_NEARLOWDISK          0x2000                                                  
#define VQ_DESIRED_DISK         0x4000                              
#define VQ_FREE_SPACE_CHANGE    0x8000                                                 
#define VQ_FLAG10000    0x10000                   


#ifdef KERNEL

                                                                
struct vfsioattr {
	u_int32_t       io_maxreadcnt;                                        
	u_int32_t       io_maxwritecnt;                                        
	u_int32_t       io_segreadcnt;                                           
	u_int32_t       io_segwritecnt;                                           
	u_int32_t       io_maxsegreadsize;                                   
	u_int32_t       io_maxsegwritesize;                                  
	u_int32_t       io_devblocksize;                                              
	u_int32_t       io_flags;                                                        
	union {
		int64_t io_max_swappin_available;
		                                                   
		void *io_reserved[2];
	};
};

#define VFS_IOATTR_FLAGS_FUA                            0x00000001                                         
#define VFS_IOATTR_FLAGS_UNMAP                          0x00000002                                  
#define VFS_IOATTR_FLAGS_SWAPPIN_SUPPORTED      0x00000010                                       

  
                                      
   
#define VFS_TBLTHREADSAFE               0x0001                                                 
#define VFS_TBLFSNODELOCK               0x0002                                                 
#define VFS_TBLNOTYPENUM                0x0008
#define VFS_TBLLOCALVOL                 0x0010
#define VFS_TBL64BITREADY               0x0020
#define VFS_TBLNATIVEXATTR              0x0040
#define VFS_TBLDIRLINKS                 0x0080
#define VFS_TBLUNMOUNT_PREFLIGHT        0x0100                                                
#define VFS_TBLGENERICMNTARGS           0x0200                                             
#define VFS_TBLREADDIR_EXTENDED         0x0400                                          
#define VFS_TBLNOMACLABEL               0x1000
#define VFS_TBLVNOP_PAGEINV2            0x2000
#define VFS_TBLVNOP_PAGEOUTV2           0x4000
#define VFS_TBLVNOP_NOUPDATEID_RENAME   0x8000                                                        
#define VFS_TBLVNOP_SECLUDE_RENAME      0x10000
#define VFS_TBLCANMOUNTROOT             0x20000


struct vfs_fsentry {
	struct vfsops * vfe_vfsops;                         
	int             vfe_vopcnt;                                                                    
	struct vnodeopv_desc ** vfe_opvdescs;                        
	int                     vfe_fstypenum;                                       
	char            vfe_fsname[MFSNAMELEN];                           
	uint32_t        vfe_flags;                                               
	void *              vfe_reserv[2];                                              
};



struct vfsops {
	   
                     
                                                                            
                                                                                                    
                                                                                                        
                                               
                                                                
                                                             
                                                                     
                                                      
                                                                                                                       
                            
    
	int  (*vfs_mount)(struct mount *mp, vnode_t devvp, user_addr_t data, vfs_context_t context);

	   
                     
                                                
                                                                                                                 
                                   
                                               
                         
                                                      
                                     
    
	int  (*vfs_start)(struct mount *mp, int flags, vfs_context_t context);

	   
                       
                                                                      
                                                                                                 
                                
                                          
                                                                                                 
                                                        
                                               
    
	int  (*vfs_unmount)(struct mount *mp, int mntflags, vfs_context_t context);

	   
                    
                                                  
                                                                                                         
                           
                                               
                                           
                                                                 
                                               
    
	int  (*vfs_root)(struct mount *mp, struct vnode **vpp, vfs_context_t context);

	   
                        
                                              
                                                    
                                                    
                                                   
                                                   
                                                                
                                               
    
	int  (*vfs_quotactl)(struct mount *mp, int cmds, uid_t uid, caddr_t arg, vfs_context_t context);

	   
                       
                                         
                                                                                                  
                                                 
                                                                                                         
                                                
                                                                          
                                               
    
	int  (*vfs_getattr)(struct mount *mp, struct vfs_attr *, vfs_context_t context);
                                                                                        

	   
                    
                                                          
                                                                                              
                                  
                                                                                                                                                     
                                                     
                                               
    
	int  (*vfs_sync)(struct mount *mp, int waitfor, vfs_context_t context);

	   
                    
                                                     
                                                                                                                         
                                       
                                                           
                                                                     
                                      
                                               
    
	int  (*vfs_vget)(struct mount *mp, ino64_t ino, struct vnode **vpp, vfs_context_t context);

	   
                      
                                                            
                                                                                                                     
                                                                                                                    
                                           
                                                          
                                                                           
                                  
                                      
                                                                                 
                                               
    
	int  (*vfs_fhtovp)(struct mount *mp, int fhlen, unsigned char *fhp, struct vnode **vpp,
	    vfs_context_t context);

	   
                      
                                                                
                                                                    
                                                                                            
                                                                
                                                                              
                                               
    
	int  (*vfs_vptofh)(struct vnode *vp, int *fhlen, unsigned char *fhp, vfs_context_t context);

	   
                    
                                                                 
                                                                                            
                                                                                       
                                                                                                      
                       
                                                                                                        
                                                                                                      
               
                                               
    
	int  (*vfs_init)(struct vfsconf *);

	   
                      
                                                                       
                                                                                        
                                                 
                                                                                       
                                          
                                                                  
                                               
    
	int  (*vfs_sysctl)(int *, u_int, user_addr_t, size_t *, user_addr_t, size_t, vfs_context_t context);

	   
                       
                                         
                                                                                                   
                                                
                                                                                                           
                                             
                                                                           
                                               
    
	int  (*vfs_setattr)(struct mount *mp, struct vfs_attr *, vfs_context_t context);

	   
                     
                                              
                                                                             
                                                   
                                                                         
                                                            
                                                                             
                                                                               
                                                                           
                                                                  
                                                      
                                                                    
                                               
    
	int  (*vfs_ioctl)(struct mount *mp, u_long command, caddr_t data,
	    int flags, vfs_context_t context);

	   
                            
                                                                        
                                                                                                         
                           
                                               
                                                         
                                                                               
                                               
    
	int  (*vfs_vget_snapdir)(struct mount *mp, struct vnode **vpp, vfs_context_t context);
	void *vfs_reserved5;
	void *vfs_reserved4;
	void *vfs_reserved3;
	void *vfs_reserved2;
	void *vfs_reserved1;
};

#ifdef KERNEL

  
                                                                               
                                                                       
   
struct fs_snapshot_mount_args {
	mount_t sm_mp;
	struct componentname *sm_cnp;
};

#define VFSIOC_MOUNT_SNAPSHOT  _IOW('V', 1, struct fs_snapshot_mount_args)

struct fs_snapshot_revert_args {
	struct componentname *sr_cnp;
};
#define VFSIOC_REVERT_SNAPSHOT  _IOW('V', 2, struct fs_snapshot_revert_args)

struct fs_snapshot_root_args {
	struct componentname *sr_cnp;
};
#define VFSIOC_ROOT_SNAPSHOT  _IOW('V', 3, struct fs_snapshot_root_args)

typedef struct fs_role_mount_args {
	mount_t root_mp;
	uint32_t mount_role;
} fs_role_mount_args_t;

OS_ENUM(vfs_roles, uint32_t,
    VFS_SYSTEM_ROLE = 1,
    VFS_RECOVERY_ROLE = 4,
    VFS_VM_ROLE = 8,
    VFS_PREBOOT_ROLE = 16,
    VFS_DATA_ROLE = 64);

#define VFSIOC_MOUNT_BYROLE  _IOW('V', 4, fs_role_mount_args_t)

                                                                
                    
#define VFSIOC_MOUNT_BYROLE_has_recovery 1

#endif             

  
                                
   
#ifdef PRIVATE
#define VFS_ITERATE_TAIL_FIRST  (1 << 0)
#define VFS_ITERATE_CB_DROPREF  (1 << 1)                                         
#define VFS_ITERATE_NOSKIP_UNMOUNT  (1 << 2)                                              
                                                                             
                                                                                   
#endif              

  
                              
   
#define VFS_RETURNED            0                                                      
#define VFS_RETURNED_DONE       1                                                                           
#define VFS_CLAIMED             2                                 
#define VFS_CLAIMED_DONE        3                                                      


__BEGIN_DECLS
#ifdef BSD_KERNEL_PRIVATE
extern int VFS_MOUNT(mount_t, vnode_t, user_addr_t, vfs_context_t);
extern int VFS_START(mount_t, int, vfs_context_t);
extern int VFS_UNMOUNT(mount_t, int, vfs_context_t);
extern int VFS_ROOT(mount_t, vnode_t *, vfs_context_t);
extern int VFS_QUOTACTL(mount_t, int, uid_t, caddr_t, vfs_context_t);
extern int VFS_GETATTR(mount_t, struct vfs_attr *, vfs_context_t);
extern int VFS_SETATTR(mount_t, struct vfs_attr *, vfs_context_t);
extern int VFS_SYNC(mount_t, int, vfs_context_t);
extern int VFS_VGET(mount_t, ino64_t, vnode_t *, vfs_context_t);
extern int VFS_FHTOVP(mount_t, int, unsigned char *, vnode_t *, vfs_context_t);
extern int VFS_VPTOFH(vnode_t, int *, unsigned char *, vfs_context_t);
extern int VFS_IOCTL(mount_t mp, u_long command, caddr_t data,
    int flags, vfs_context_t context);
extern int VFS_VGET_SNAPDIR(mount_t, vnode_t *, vfs_context_t);
#endif                         
  
                                         
   

   
                       
                                             
                                                                                    
                                                                                               
                                                                                    
                                                                     
                                              
   
int vfs_fsadd(struct vfs_fsentry *vfe, vfstable_t *handle);

   
                          
                                               
                                                                                      
                                                         
                                              
   
int vfs_fsremove(vfstable_t handle);

   
                         
                                                                                          
                        
                                                                                                                                            
                                                                                                                      
                                                  
                                              
   
int     vfs_iterate(int flags, int (*callout)(struct mount *, void *), void *arg);

   
                                    
                                                                            
                                                                             
                                                            
                                              
   
int     vfs_init_io_attributes(vnode_t devvp, mount_t mp);

   
                       
                                   
                                                                                        
                                        
                  
   
uint64_t vfs_flags(mount_t mp);

   
                          
                                   
                                                                                                         
                                                      
                                       
                                                                                                     
   
void    vfs_setflags(mount_t mp, uint64_t flags);

   
                            
                                     
                                                                                                                      
                                       
                                                                                                       
   
void    vfs_clearflags(mount_t mp, uint64_t flags);

   
                               
                                                                      
                            
                                                          
   
int     vfs_issynchronous(mount_t mp);

   
                                
                                                                                                   
                  
                            
                                                                                              
   
int     vfs_iswriteupgrade(mount_t mp);

   
                          
                                                         
                            
                                                                  
   
int     vfs_isupdate(mount_t mp);

   
                          
                                                                                                 
                                                                              
                            
                                                                      
   
int     vfs_isreload(mount_t mp);

   
                         
                                                           
                                                        
                            
                                                                           
   
int     vfs_isforce(mount_t mp);

   
                           
                                                     
                                                                                                
                                                                                            
                            
                                                            
   
int     vfs_isunmount(mount_t mp);

   
                          
                                                             
                            
                                                               
   
int     vfs_isrdonly(mount_t mp);

   
                        
                                                                       
                            
                                                                
   
int     vfs_isrdwr(mount_t mp);

   
                            
                                                                                 
                            
                                                                               
   
int     vfs_authopaque(mount_t mp);

   
                                  
                                                                                            
                            
                                                                 
   
int     vfs_authopaqueaccess(mount_t mp);

   
                               
                                                                                      
                            
   
void    vfs_setauthopaque(mount_t mp);

   
                                     
                                                                     
                            
   
void    vfs_setauthopaqueaccess(mount_t mp);

   
                                 
                                                                             
                            
   
void    vfs_clearauthopaque(mount_t mp);

   
                                 
                                                                         
                            
   
void    vfs_clearauthopaqueaccess(mount_t mp);

   
                                     
                                                                                         
                                                                                  
                            
   
void    vfs_setextendedsecurity(mount_t mp);

   
                                       
                                                                                             
                                                                                  
                            
   
void    vfs_clearextendedsecurity(mount_t mp);

   
                           
                                                            
                            
   
#ifdef KERNEL_PRIVATE
void    vfs_setnoswap(mount_t mp);
#endif

   
                             
                                                               
                            
   
#ifdef KERNEL_PRIVATE
void    vfs_clearnoswap(mount_t mp);
#endif

   
                              
                                                                            
                                                                                                     
                            
   
void    vfs_setlocklocal(mount_t mp);

   
                               
                                                                                                       
                                                                                                
                                                                                                            
                                         
                                                      
                                                                                                           
   
int     vfs_authcache_ttl(mount_t mp);

   
                                  
                                                                                                                            
                                                                                                
                                                                                                            
                                         
                                                    
   
void    vfs_setauthcache_ttl(mount_t mp, int ttl);

   
                                    
                                                                                                                        
                                                                                                                         
                                                      
   
void    vfs_clearauthcache_ttl(mount_t mp);

  
                                            
                                                 
                                         
                                                         
   
#define CACHED_RIGHT_INFINITE_TTL       ~0

   
                           
                                                                        
                                                         
                               
   
uint32_t vfs_maxsymlen(mount_t mp);

   
                              
                                                                        
                                                       
                                
   
void    vfs_setmaxsymlen(mount_t mp, uint32_t symlen);

   
                           
                                                
                                                                                                                         
                                    
                                                  
                         
   
void *  vfs_fsprivate(mount_t mp);

   
                              
                                                
                                                                                                                         
                                    
                                                  
   
void    vfs_setfsprivate(mount_t mp, void *mntdata);

   
                        
                                                      
                                                                                                                         
                                                                                                                        
                                                                      
                                                       
                                 
   
struct vfsstatfs *      vfs_statfs(mount_t mp);
#define VFS_USER_EVENT          0
#define VFS_KERNEL_EVENT        1

   
                                
                                                                                     
                                                                                                                         
                                                                                                              
                                                                  
                                                                           
                                                                                                                        
                                                                                                
                                                                                                            
                        
   
int     vfs_update_vfsstat(mount_t mp, vfs_context_t ctx, int eventtype);

   
                         
                                                   
                                                                                                                  
                                                           
                                                 
                        
   
int     vfs_typenum(mount_t mp);

   
                      
                                                 
                                                                                                             
                                                  
                                          
                                                                             
   
void    vfs_name(mount_t mp, char *buffer);

   
                              
                                                                  
                                                
                       
   
int     vfs_devblocksize(mount_t mp);

   
                        
                                                             
                                                                                                   
                                           
   
void    vfs_ioattr(mount_t mp, struct vfsioattr *ioattrp);

   
                           
                                                             
                                                
                                                                                     
   
void    vfs_setioattr(mount_t mp, struct vfsioattr *ioattrp);

   
                            
                                                                                                                              
                            
                                                                   
   
int     vfs_64bitready(mount_t mp);


#define LK_NOWAIT 1
   
                      
                                  
                                                                                                                          
                                            
                                                                                                            
                                                                                                                            
                                                                                                
                            
                                                                          
                                                                                        
   
int     vfs_busy(mount_t mp, int flags);

   
                        
                                                                     
                                                                                                            
                              
   
void    vfs_unbusy(mount_t mp);

   
                            
                                                                                              
                                                                                                   
                                                                               
                                     
   
void    vfs_getnewfsid(struct mount *mp);

   
                        
                                                               
                                         
                                                                                         
   
mount_t vfs_getvfs(fsid_t *fsid);

   
                           
                                                                                
                                                                                           
                                                                                                 
                                                                                           
                                
                                                                                
   
int     vfs_mountedon(struct vnode *vp);

   
                               
                                                     
                                                                                   
                                                                                                        
                                 
                                                                       
                                               
   
int     vfs_unmountbyfsid(fsid_t *fsid, int flags, vfs_context_t ctx);

   
                              
                                                                    
                       
                                
                       
   
void    vfs_event_signal(fsid_t *fsid, u_int32_t event, intptr_t data);

   
                            
                                                          
   
void    vfs_event_init(void);                                    

   
                                          
                                                                    
                                                                    
   
void vfs_set_root_unmounted_cleanly(void);

#ifdef KERNEL_PRIVATE
int     vfs_getbyid(fsid_t *fsid, ino64_t ino, vnode_t *vpp, vfs_context_t ctx);
int     vfs_getattr(mount_t mp, struct vfs_attr *vfa, vfs_context_t ctx);
int     vfs_setattr(mount_t mp, struct vfs_attr *vfa, vfs_context_t ctx);
int     vfs_extendedsecurity(mount_t);
mount_t vfs_getvfs_by_mntonname(char *);
vnode_t vfs_vnodecovered(mount_t mp);                                                                           
vnode_t vfs_devvp(mount_t mp);                                                   
int vfs_nativexattrs(mount_t mp);                                                   
void *  vfs_mntlabel(mount_t mp);                                                                                                            
void    vfs_setcompoundopen(mount_t mp);
uint64_t vfs_throttle_mask(mount_t mp);
int vfs_isswapmount(mount_t mp);
int     vfs_context_dataless_materialization_is_prevented(vfs_context_t);
boolean_t vfs_context_is_dataless_manipulator(vfs_context_t);
boolean_t vfs_context_can_resolve_triggers(vfs_context_t);
void    vfs_setmntsystem(mount_t mp);
void    vfs_setmntsystemdata(mount_t mp);
void    vfs_setmntswap(mount_t mp);
boolean_t vfs_is_basesystem(mount_t mp);

OS_ENUM(bsd_bootfail_mode, uint32_t,
    BSD_BOOTFAIL_SEAL_BROKEN = 1,
    BSD_BOOTFAIL_MEDIA_MISSING = 2);

boolean_t bsd_boot_to_recovery(bsd_bootfail_mode_t mode, uuid_t volume_uuid, boolean_t reboot);

struct vnode_trigger_info;

   
                            
                                                                                
                                                                             
                                                                              
                        
                                                                                
                                                                             
                                  
                                     
   
int     vfs_addtrigger(mount_t mp, const char *relpath, struct vnode_trigger_info *vtip, vfs_context_t ctx);


   
                                   
                                                                                        
                                                                                  
                                                                      
                                                                                 
   
typedef enum {
	VTC_REPLACE,
	VTC_RELEASE
} vfs_trigger_callback_op_t;

   
                                   
                                                                                
                         
                                                       
                                                       
                                                          
                                                                   
   
typedef void vfs_trigger_callback_t(mount_t mp, vfs_trigger_callback_op_t op, void *data, vfs_context_t ctx);

   
                                    
                                                                                 
                                                                                  
                                                                             
                                                                                 
                                                                               
                                                                           
                                                                                  
                                                                                
                                                                                  
                                                                                 
                                                                                
                                                                        
                                                                                  
                                                                            
                                                          
                                                                                  
                                                                               
                                                                                
                                  
                                                
                                
                                                         
                                  
                                     
                                                                          
   
int     vfs_settriggercallback(fsid_t *fsid, vfs_trigger_callback_t vtc, void *data, uint32_t flags, vfs_context_t ctx);

                                                                      
void mount_set_noreaddirext(mount_t);

   
                              
                                                                                
                            
   
void vfs_get_statfs64(struct mount *mp, struct statfs64 *sfs);

   
                         
                                                                        
                                                               
                                                      
   
uint64_t vfs_mount_id(mount_t mp);

#endif                      
__END_DECLS

#endif             

#ifndef KERNEL

  
                      
   
#define NFS_MAX_FH_SIZE         NFSV4_MAX_FH_SIZE
#define NFSV4_MAX_FH_SIZE       128
#define NFSV3_MAX_FH_SIZE       64
#define NFSV2_MAX_FH_SIZE       32
struct fhandle {
	unsigned int    fh_len;                                                    
	unsigned char   fh_data[NFS_MAX_FH_SIZE];                              
};
typedef struct fhandle  fhandle_t;


__BEGIN_DECLS
int     fhopen(const struct fhandle *, int);
int     fstatfs(int, struct statfs *) __DARWIN_INODE64(fstatfs);
#if !__DARWIN_ONLY_64_BIT_INO_T
int     fstatfs64(int, struct statfs64 *) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);
#endif                                  
int     getfh(const char *, fhandle_t *);
int     getfsstat(struct statfs *, int, int) __DARWIN_INODE64(getfsstat);
#if !__DARWIN_ONLY_64_BIT_INO_T
int     getfsstat64(struct statfs64 *, int, int) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);
#endif                                  
int     getmntinfo(struct statfs **, int) __DARWIN_INODE64(getmntinfo);
int     getmntinfo_r_np(struct statfs **, int) __DARWIN_INODE64(getmntinfo_r_np)
__OSX_AVAILABLE(10.13) __IOS_AVAILABLE(11.0)
__TVOS_AVAILABLE(11.0) __WATCHOS_AVAILABLE(4.0);
#if !__DARWIN_ONLY_64_BIT_INO_T
int     getmntinfo64(struct statfs64 **, int) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);
#endif                                  
int     mount(const char *, const char *, int, void *);
int     fmount(const char *, int, int, void *) __OSX_AVAILABLE(10.13) __IOS_AVAILABLE(11.0) __TVOS_AVAILABLE(11.0) __WATCHOS_AVAILABLE(4.0);
int     statfs(const char *, struct statfs *) __DARWIN_INODE64(statfs);
#if !__DARWIN_ONLY_64_BIT_INO_T
int     statfs64(const char *, struct statfs64 *) __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);
#endif                                  
int     unmount(const char *, int);
int     getvfsbyname(const char *, struct vfsconf *);
#if PRIVATE
int     pivot_root(const char *, const char *) __OSX_AVAILABLE(10.16);
#endif
__END_DECLS

#endif             
#endif                     
