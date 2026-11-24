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
  
                                                                         
                                                                          
                                                                        
               
   

#ifndef _SYS_MOUNT_INTERNAL_H_
#define _SYS_MOUNT_INTERNAL_H_

#include <sys/appleapiopts.h>
#ifndef KERNEL
#include <sys/ucred.h>
#else
#include <sys/kernel_types.h>
#include <sys/namei.h>
#endif
#include <sys/queue.h>
#include <sys/lock.h>
#include <net/radix.h>
#include <sys/socket.h>                             
#include <sys/vfs_context.h>                                
#include <sys/mount.h>
#include <sys/cdefs.h>
#include <sys/sysctl.h>

struct label;

#if defined(__i386__) || defined(__x86_64__)
typedef uint64_t  pending_io_t;
#define INCR_PENDING_IO(a, b) OSAddAtomic64((int64_t)(a), (int64_t *)&(b));
#else
typedef uint32_t  pending_io_t;
#define INCR_PENDING_IO(a, b) OSAddAtomic((int32_t)(a), (int32_t *)&(b));
#endif


  
                                                                      
                                                                    
                               
   
TAILQ_HEAD(vnodelst, vnode);

struct mount {
	TAILQ_ENTRY(mount)      mnt_list;                                   
	int32_t                 mnt_count;                                              
	lck_mtx_t               mnt_mlock;                                                       
	const struct vfsops     * XNU_PTRAUTH_SIGNED_PTR("mount.vfsops") mnt_op;                              
	struct vfstable         * XNU_PTRAUTH_SIGNED_PTR("mount.mnt_vtable") mnt_vtable;                                
	struct vnode            * XNU_PTRAUTH_SIGNED_PTR("mount.mnt_vnodecovered") mnt_vnodecovered;                             
	struct vnodelst         mnt_vnodelist;                                             
	struct vnodelst         mnt_workerqueue;                                           
	struct vnodelst         mnt_newvnodes;                                             
	uint32_t                mnt_flag;                              
	uint32_t                mnt_kern_flag;                                                                            
	uint32_t                mnt_compound_ops;                                              
	uint32_t                mnt_lflag;                                              
	uint32_t                mnt_maxsymlinklen;                                         
	struct vfsstatfs        mnt_vfsstat;                                               
	qaddr_t                 mnt_data;                                     
	                                                        
	uint32_t                mnt_maxreadcnt;                                           
	uint32_t                mnt_maxwritecnt;                                           
	uint32_t                mnt_segreadcnt;                                              
	uint32_t                mnt_segwritecnt;                                              
	uint32_t                mnt_maxsegreadsize;                                      
	uint32_t                mnt_maxsegwritesize;                                     
	uint32_t                mnt_alignmentmask;                                                            
	uint32_t                mnt_devblocksize;                                                 
	uint32_t                mnt_ioqueue_depth;                                                                   
	uint32_t                mnt_ioscale;                                                                                               
	uint32_t                mnt_ioflags;                                                  
	uint32_t                mnt_minsaturationbytecount;                                                                                    
	pending_io_t            mnt_pending_write_size __attribute__((aligned(sizeof(pending_io_t))));                                    
	pending_io_t            mnt_pending_read_size  __attribute__((aligned(sizeof(pending_io_t))));                                   
	struct timeval          mnt_last_write_issued_timestamp;
	struct timeval          mnt_last_write_completed_timestamp;
	int64_t                 mnt_max_swappin_available;

	lck_rw_t                mnt_rwlock;                                           
	lck_mtx_t               mnt_renamelock;                                                                          
	vnode_t                 mnt_devvp;                                                                    
	uint32_t                mnt_devbsdunit;                                                    
	uint64_t                mnt_throttle_mask;                                                                                       
	void                    *mnt_throttle_info;                                        
	int32_t                 mnt_crossref;                                                                 
	int32_t                 mnt_iterref;                                                                          
#if CONFIG_TRIGGERS
	int32_t                 mnt_numtriggers;                                                      
	vfs_trigger_callback_t  *mnt_triggercallback;
	void                    *mnt_triggerdata;
#endif
	                                                        
	uid_t                   mnt_fsowner;
	gid_t                   mnt_fsgroup;

	struct label            * XNU_PTRAUTH_SIGNED_PTR("mount.mnt_mntlabel") mnt_mntlabel;                         
	struct label            * XNU_PTRAUTH_SIGNED_PTR("mount.mnt_fslabel") mnt_fslabel;                                

	  
                                            
                                            
                                  
                                          
                                       
                                             
                                                
   
                                                
                                             
                                                 
                          
    
	vnode_t                 mnt_realrootvp;
	uint32_t                mnt_realrootvp_vid;
	  
                                       
                                    
                                   
    
	uint32_t                mnt_generation;
	  
                                   
                                    
                                                           
                                                       
                                                   
                                                
                                      
    
	int                     mnt_authcache_ttl;
	char                    fstypename_override[MFSTYPENAMELEN];

	uint32_t                mnt_iobufinuse;

	void                    *mnt_disk_conditioner_info;

	lck_mtx_t               mnt_iter_lock;                                                           

	uint64_t                mnt_mount_id;                                                
	uint32_t                mnt_supl_kern_flag;                                                   
};

  
                                                  
                                                 
   
#define CACHED_LOOKUP_RIGHT_TTL         2

  
          
   
#define MNT_IOFLAGS_FUA_SUPPORTED       0x00000001
#define MNT_IOFLAGS_UNMAP_SUPPORTED     0x00000002
#define MNT_IOFLAGS_IOSCHED_SUPPORTED   0x00000004
#define MNT_IOFLAGS_CSUNMAP_SUPPORTED   0x00000008
#define MNT_IOFLAGS_SWAPPIN_SUPPORTED   0x00000010
#define MNT_IOFLAGS_FUSION_DRIVE        0x00000020
#define MNT_IOFLAGS_PERIPHERAL_DRIVE    0x00000040                                                                  

  
                                                  
   
#define MNT_DEFAULT_IOQUEUE_DEPTH       32

  
                                                
   
#define MNT_IOSCALE(ioqueue_depth)      ((ioqueue_depth + (MNT_DEFAULT_IOQUEUE_DEPTH - 1)) / MNT_DEFAULT_IOQUEUE_DEPTH)

                                            
extern struct mount * dead_mountp;

  
                                                             
  
                                                                        
                                                                     
                
  
                                                              
                                                            
                       
   
#define MNTK_SYSTEM             0x00000040                                                                       
#define MNTK_NOSWAP             0x00000080                                                   
#define MNTK_SWAP_MOUNT         0x00000100                                         
#define MNTK_DENY_READDIREXT    0x00000200                                                         
#define MNTK_PERMIT_UNMOUNT     0x00000400                                                                                          
#define MNTK_TYPENAME_OVERRIDE  0x00000800                                                
#define MNTK_KERNEL_MOUNT       0x00001000                                       
#ifdef CONFIG_IMGSRC_ACCESS
#define MNTK_HAS_MOVED          0x00002000
#endif                           
#define MNTK_BACKS_ROOT         0x00004000                                                                                                       
#define MNTK_AUTH_CACHE_TTL     0x00008000                                                          
#define MNTK_PATH_FROM_ID       0x00010000                                                           
#define MNTK_UNMOUNT_PREFLIGHT  0x00020000                                                                    
#define MNTK_NAMED_STREAMS      0x00040000                                                            
#define MNTK_EXTENDED_ATTRS     0x00080000                                                                  
#define MNTK_LOCK_LOCAL         0x00100000                                                         
#define MNTK_VIRTUALDEV         0x00200000                                                         
#define MNTK_ROOTDEV            0x00400000                                                                                              
#define MNTK_SSD                0x00800000                                                           
#define MNTK_UNMOUNT            0x01000000                               
#define MNTK_MWAIT              0x02000000                                                                     
#define MNTK_WANTRDWR           0x04000000                                           
#define MNTK_SYSTEMDATA         0x08000000                                                                                          
#define MNTK_DIR_HARDLINKS      0x10000000                                                             
#define MNTK_AUTH_OPAQUE        0x20000000                                                        
#define MNTK_AUTH_OPAQUE_ACCESS 0x40000000                                                   
#define MNTK_EXTENDED_SECURITY  0x80000000                                       


  
                                                                      
  
                                                                                     
                                                  
   
#define MNTK_SUPL_BASESYSTEM    0x00000001


  
                                              
   
#define MNT_LNOTRESP            0x00000001                                
#define MNT_LUNMOUNT            0x00000002                            
#define MNT_LFORCE              0x00000004                                   
#define MNT_LDRAIN              0x00000008                          
#define MNT_LITER               0x00000010                              
#define MNT_LNEWVN              0x00000020                                        
#define MNT_LWAIT               0x00000040                               
#define MNT_LMOUNT              0x00000080                                 
#define MNT_LDEAD               0x00000100                                  
#define MNT_LNOSUB              0x00000200                                   


  
                      
   
#define NFS_MAX_FH_SIZE         NFSV4_MAX_FH_SIZE
#define NFSV4_MAX_FH_SIZE       128
#define NFSV3_MAX_FH_SIZE       64
#define NFSV2_MAX_FH_SIZE       32

#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_FHANDLE);
#endif

struct fhandle {
	unsigned int    fh_len;                                                    
	unsigned char   fh_data[NFS_MAX_FH_SIZE];                              
};
typedef struct fhandle  fhandle_t;



  
                                                                     
                                                                    
                                                   
   
struct vfstable {
	const struct vfsops *vfc_vfsops;                                  
	char    vfc_name[MFSNAMELEN];                             
	int     vfc_typenum;                                                 
	int     vfc_refcount;                                            
	int     vfc_flags;                                   
	int     (*vfc_mountroot)(mount_t, vnode_t, vfs_context_t);                                             
	struct  vfstable *vfc_next;                       
	int32_t vfc_reserved1;
	int32_t vfc_reserved2;
	int             vfc_vfsflags;                           
	void *          vfc_descptr;                                      
	int                     vfc_descsize;                                      
	struct sysctl_oid       *vfc_sysctl;                                            
};

                   
#define VFC_VFSLOCALARGS        0x002
#define VFC_VFSGENERICARGS      0x004
#define VFC_VFSNATIVEXATTR      0x010
#define VFC_VFSCANMOUNTROOT 0x020
#define VFC_VFSPREFLIGHT        0x040
#define VFC_VFSREADDIR_EXTENDED 0x080
#define VFC_VFS64BITREADY       0x100
#define VFC_VFSNOMACLABEL       0x1000
#define VFC_VFSVNOP_PAGEINV2    0x2000
#define VFC_VFSVNOP_PAGEOUTV2   0x4000
#define VFC_VFSVNOP_NOUPDATEID_RENAME   0x8000
#define VFC_VFSVNOP_SECLUDE_RENAME      0x10000

extern int maxvfstypenum;                                                    
extern struct vfstable  *vfsconf;                                             
extern const int maxvfsslots;                                                                        
extern int numused_vfsslots;                                                           
extern int numregistered_fses;                                              

                                       
struct vfstable *       vfstable_add(struct     vfstable *);
int     vfstable_del(struct vfstable *);


struct vfsmount_args {
	union {
		struct {
			char * mnt_fspec;
			void * mnt_fsdata;
		} mnt_localfs_args;
		struct {
			void *  mnt_fsdata;                              
		} mnt_remotefs_args;
	} mountfs_args;
};


  
                                           
                                                            
   
struct user64_statfs {
	short           f_otype;                                                     
	short           f_oflags;                                                     
	user64_long_t   f_bsize;                                                        
	user64_long_t   f_iosize;                                                
	user64_long_t   f_blocks;                                                     
	user64_long_t   f_bfree;                                       
	user64_long_t   f_bavail;                                                       
	user64_long_t   f_files;                                                     
	user64_long_t   f_ffree;                                           
	fsid_t          f_fsid;                                     
	uid_t           f_owner;                                                      
	short           f_reserved1;                                 
	short           f_type;                                         
	user64_long_t   f_flags;                                                  
	user64_long_t   f_reserved2[2];                                      
	char            f_fstypename[MFSNAMELEN];                   
	char            f_mntonname[MNAMELEN];                                  
	char            f_mntfromname[MNAMELEN];                        
	char            f_reserved3;                       
	user64_long_t   f_reserved4[4];                     
};

  
                                            
                                                            
   
struct user32_statfs {
	short           f_otype;                                                     
	short           f_oflags;                                                     
	user32_long_t   f_bsize;                                                        
	user32_long_t   f_iosize;                                                
	user32_long_t   f_blocks;                                                     
	user32_long_t   f_bfree;                                       
	user32_long_t   f_bavail;                                                       
	user32_long_t   f_files;                                                     
	user32_long_t   f_ffree;                                           
	fsid_t          f_fsid;                                     
	uid_t           f_owner;                                                      
	short           f_reserved1;                         
	short           f_type;                                         
	user32_long_t       f_flags;                                              
	user32_long_t f_reserved2[2];                                
	char            f_fstypename[MFSNAMELEN];                   
	char            f_mntonname[MNAMELEN];                                  
	char            f_mntfromname[MNAMELEN];                        
	char            f_reserved3;                       
	user32_long_t   f_reserved4[4];                     
};

  
                                                                                                                      
                                                                                              
                                                                                                       
   
#define LOWPRI_MAX_NUM_DEV 64

__BEGIN_DECLS

extern uint32_t mount_generation;
extern TAILQ_HEAD(mntlist, mount) mountlist;
void mount_list_lock(void);
void mount_list_unlock(void);
void mount_lock_init(mount_t);
void mount_lock_destroy(mount_t);
void mount_lock(mount_t);
void mount_lock_spin(mount_t);
void mount_unlock(mount_t);
void mount_iterate_lock(mount_t);
void mount_iterate_unlock(mount_t);
void mount_lock_renames(mount_t);
void mount_unlock_renames(mount_t);
void mount_ref(mount_t, int);
void mount_drop(mount_t, int);
int  mount_refdrain(mount_t);

                                                        
errno_t vfs_rootmountalloc(const char *, const char *, mount_t *mpp);

int vfs_mount_recovery(void);

typedef uint32_t vfs_switch_root_flags_t;
#define VFSSR_VIRTUALDEV_PROHIBITED     0x01                                                            
                                                                                                      
                                                                                            
                                                                                                         
                                                                                                     
                                                                                                     

int vfs_switch_root(const char *, const char *, vfs_switch_root_flags_t);

int     vfs_mountroot(void);
void    vfs_unmountall(int only_non_system);
int     safedounmount(struct mount *, int, vfs_context_t);
int     dounmount(struct mount *, int, int, vfs_context_t);
void    dounmount_submounts(struct mount *, int, vfs_context_t);

                      
void  mount_dropcrossref(mount_t, vnode_t, int);
mount_t mount_lookupby_volfsid(int, int);
mount_t mount_list_lookupby_fsid(fsid_t *, int, int);
int  mount_list_add(mount_t);
void mount_list_remove(mount_t);
int  mount_iterref(mount_t, int);
int  mount_isdrained(mount_t, int);
void mount_iterdrop(mount_t);
void mount_iterdrain(mount_t);
void mount_iterreset(mount_t);

                                                                                        
                     
#define KERNEL_MOUNT_NOAUTH             0x01                                                              
#define KERNEL_MOUNT_PERMIT_UNMOUNT     0x02                                                                                
                                   
#define KERNEL_MOUNT_SNAPSHOT           0x04                          
#define KERNEL_MOUNT_DATAVOL            0x08                            
#define KERNEL_MOUNT_VMVOL              0x10                          
#define KERNEL_MOUNT_PREBOOTVOL         0x20                               
#define KERNEL_MOUNT_RECOVERYVOL        0x40                                
#define KERNEL_MOUNT_BASESYSTEMROOT     0x80                                                                                             
#define KERNEL_MOUNT_DEVFS             0x100                                    

                                                                          
#define KERNEL_MOUNT_VOLBYROLE_MASK (KERNEL_MOUNT_DATAVOL | KERNEL_MOUNT_VMVOL | KERNEL_MOUNT_PREBOOTVOL | KERNEL_MOUNT_RECOVERYVOL)


#if NFSCLIENT || DEVFS || ROUTEFS
  
                                                                                              
                                                          
   
int kernel_mount(char *, vnode_t, vnode_t, const char *, void *, size_t, int, uint32_t, vfs_context_t);
boolean_t vfs_iskernelmount(mount_t);
#endif

                                                

int  throttle_get_io_policy(struct uthread **ut);
int  throttle_get_passive_io_policy(struct uthread **ut);
void *throttle_info_update_by_mount(mount_t mp);
void rethrottle_thread(uthread_t ut);


                                   
                                              
extern int num_trailing_0(uint64_t n);

               
extern int sync_timeout_seconds;

extern zone_t mount_zone;

__END_DECLS

#endif                              
