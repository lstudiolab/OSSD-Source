/*
 * Copyright (c) 2000-2016 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995, 1997 Apple Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)file.h	8.3 (Berkeley) 1/9/95
 */

#ifndef _SYS_FILE_INTERNAL_H_
#define _SYS_FILE_INTERNAL_H_

#include <sys/appleapiopts.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>

#ifdef XNU_KERNEL_PRIVATE
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/cdefs.h>
#include <sys/lock.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/guarded.h>
#include <os/refcnt.h>

__BEGIN_DECLS

#pragma GCC visibility push(hidden)

struct proc;
struct uio;
struct knote;
struct kevent_qos_s;
struct file;
#ifndef _KAUTH_CRED_T
#define _KAUTH_CRED_T
typedef struct ucred *kauth_cred_t;
typedef struct posix_cred *posix_cred_t;
#endif                      

__options_decl(fileproc_vflags_t, unsigned int, {
	FPV_NONE        = 0,
	FPV_DRAIN       = 0x01,
});

  
                           
                                                   
   
struct fileproc {
	unsigned int fp_flags;
	_Atomic fileproc_vflags_t fp_vflags;
	os_refcnt_t fp_iocount;
	struct fileglob * fp_glob;
	void *fp_wset;
};

#define FILEPROC_NULL (struct fileproc *)0

#define FP_INSELECT     0x0004
#define FP_AIOISSUED    0x0080
#define FP_SELCONFLICT  0x0200                                           

                                                     

#define _FP_TYPESHIFT   24
#define FP_TYPEMASK     (0x7 << _FP_TYPESHIFT)                             

#define FILEPROC_TYPE(fp)       ((fp)->fp_flags & FP_TYPEMASK)

typedef enum {
	FTYPE_SIMPLE    = 0,
	FTYPE_GUARDED   = (1 << _FP_TYPESHIFT)
} fileproc_type_t;

                
typedef enum {
	DTYPE_VNODE     = 1,              
	DTYPE_SOCKET,                                        
	DTYPE_PSXSHM,                                    
	DTYPE_PSXSEM,                                 
	DTYPE_KQUEUE,                       
	DTYPE_PIPE,                       
	DTYPE_FSEVENTS,                       
	DTYPE_ATALK,                            
	DTYPE_NETPOLICY,                               
} file_type_t;

                           
                            
#define FG_INSMSGQ      0x02                                       
#define FG_WINSMSGQ     0x04                                            
#define FG_RMMSGQ       0x08                                                     
#define FG_WRMMSGQ      0x10                                                            
#define FG_PORTMADE     0x20                                                            
#define FG_NOSIGPIPE    0x40                                                 
#define FG_OFF_LOCKED   0x80                                                         
#define FG_OFF_LOCKWANT 0x100                                       
#define FG_CONFINED     0x200                                                
#define FG_HAS_OFDLOCK  0x400                                   

struct fileops {
	file_type_t     fo_type;                             
	int (*fo_read)      (struct fileproc *fp, struct uio *uio,
	    int flags, vfs_context_t ctx);
	int (*fo_write)     (struct fileproc *fp, struct uio *uio,
	    int flags, vfs_context_t ctx);
#define FOF_OFFSET      0x00000001                                       
#define FOF_PCRED       0x00000002                                              
	int (*fo_ioctl)(struct fileproc *fp, u_long com,
	    caddr_t data, vfs_context_t ctx);
	int (*fo_select)    (struct fileproc *fp, int which,
	    void *wql, vfs_context_t ctx);
	int (*fo_close)     (struct fileglob *fg, vfs_context_t ctx);
	int (*fo_kqfilter)  (struct fileproc *fp, struct knote *, struct kevent_qos_s *);
	int (*fo_drain)     (struct fileproc *fp, vfs_context_t ctx);
};

struct fileglob {
	LIST_ENTRY(fileglob) f_msglist;                                         
	uint32_t             fg_flag;                                 
	os_ref_atomic_t      fg_count;                           
	uint32_t             fg_msgcount;                                      
	int32_t              fg_lflags;                            
	kauth_cred_t         fg_cred;                                                   
	const struct fileops *fg_ops;
	off_t                fg_offset;
	void                *fg_data;                                                
	void                *fg_vn_data;                                                 
	lck_mtx_t            fg_lock;
#if CONFIG_MACF
	struct label        *fg_label;                                          
#endif
};

extern int maxfiles;                                                              
extern int nfiles;                                                       
extern int maxfilesperproc;
os_refgrp_decl_extern(f_refgrp);                                            

#define FILEGLOB_DTYPE(fg)              ((const file_type_t)((fg)->fg_ops->fo_type))

#pragma mark files (struct fileglob)

   
                   
  
         
                                                  
  
               
                                                                     
                                                    
  
              
                                                      
  
            
                     
   
void
fg_ref(proc_t proc, struct fileglob *fg);

   
                         
  
         
                                                                        
  
            
                                             
   
void
fg_drop_live(struct fileglob *fg);

   
                    
  
         
                                                
  
              
                                                      
  
           
                                  
                                                          
  
            
                         
  
           
                     
                                                            
   
int
fg_drop(proc_t p, struct fileglob *fg);

   
                        
  
         
                                                          
   
bool
fg_sendable(struct fileglob *fg);

#pragma mark file descriptor entries (struct fileproc)

   
                         
  
         
                                                                           
                                                  
  
           
                                 
  
            
                                       
  
               
                          
  
             
                                                                          
  
             
                                                 
  
           
                                       
                                   
                                                                      
   
extern int
fp_get_ftype(proc_t p, int fd, file_type_t ftype, int err, struct fileproc **fpp);

   
                                
  
         
                                                                 
                                                              
  
               
                                                                       
                                                           
  
                                                               
                                                               
                              
  
           
                                 
  
            
                                       
  
           
                            
                           
   
extern struct fileproc *
fp_get_noref_locked(proc_t p, int fd);

   
                                             
  
         
                                                                        
           
  
              
                                                                              
                                               
  
                                                                         
                                                                     
                       
  
                                                                          
                          
  
           
                                                                       
                                                                  
                         
   
extern struct fileproc *
fp_get_noref_locked_with_iocount(proc_t p, int fd);

   
                                
  
         
                                         
  
               
                                                         
                                                        
                                          
  
                                              
  
           
                                 
  
            
                                          
  
            
                                            
  
           
                       
                                   
                                                               
   
extern int
fp_close_and_unlock(proc_t p, int fd, struct fileproc *fp, int flags);

                                    
int fo_read(struct fileproc *fp, struct uio *uio, int flags, vfs_context_t ctx);
int fo_write(struct fileproc *fp, struct uio *uio, int flags,
    vfs_context_t ctx);
int fo_ioctl(struct fileproc *fp, u_long com, caddr_t data, vfs_context_t ctx);
int fo_select(struct fileproc *fp, int which, void *wql, vfs_context_t ctx);
int fo_close(struct fileglob *fg, vfs_context_t ctx);
int fo_drain(struct fileproc *fp, vfs_context_t ctx);
int fo_kqfilter(struct fileproc *fp, struct knote *kn, struct kevent_qos_s *kev);

                                              
int fo_no_read(struct fileproc *fp, struct uio *uio, int flags, vfs_context_t ctx);
int fo_no_write(struct fileproc *fp, struct uio *uio, int flags,
    vfs_context_t ctx);
int fo_no_ioctl(struct fileproc *fp, u_long com, caddr_t data, vfs_context_t ctx);
int fo_no_select(struct fileproc *fp, int which, void *wql, vfs_context_t ctx);
int fo_no_drain(struct fileproc *fp, vfs_context_t ctx);
int fo_no_kqfilter(struct fileproc *, struct knote *, struct kevent_qos_s *kev);

int fp_tryswap(proc_t, int fd, struct fileproc *nfp);
int fp_drop(struct proc *p, int fd, struct fileproc *fp, int locked);
void fp_free(struct proc * p, int fd, struct fileproc * fp);
int fp_lookup(struct proc *p, int fd, struct fileproc **resultfp, int locked);
int fp_isguarded(struct fileproc *fp, u_int attribs);
int fp_guard_exception(proc_t p, int fd, struct fileproc *fp, u_int attribs);
struct nameidata;
struct vnode_attr;
int open1(vfs_context_t ctx, struct nameidata *ndp, int uflags,
    struct vnode_attr *vap, fp_allocfn_t fp_zalloc, void *cra,
    int32_t *retval);
int chdir_internal(proc_t p, vfs_context_t ctx, struct nameidata *ndp, int per_thread);
int kqueue_internal(struct proc *p, fp_allocfn_t, void *cra, int32_t *retval);
void procfdtbl_releasefd(struct proc * p, int fd, struct fileproc * fp);
extern struct fileproc *fileproc_alloc_init(void *crargs);
extern void fileproc_free(struct fileproc *fp);
extern void guarded_fileproc_free(struct fileproc *fp);
extern void fg_vn_data_free(void *fgvndata);
extern int nameiat(struct nameidata *ndp, int dirfd);
extern int falloc_guarded(struct proc *p, struct fileproc **fp, int *fd,
    vfs_context_t ctx, const guardid_t *guard, u_int attrs);
extern void fileproc_modify_vflags(struct fileproc *fp, fileproc_vflags_t vflags, boolean_t clearflags);
fileproc_vflags_t fileproc_get_vflags(struct fileproc *fp);

#pragma mark internal version of syscalls

int fileport_makefd(proc_t p, ipc_port_t port, int uf_flags, int *fd);
int dup2(proc_t p, int from, int to, int *fd);
int close_nocancel(proc_t p, int fd);

#pragma GCC visibility pop

__END_DECLS

#endif                         

#endif                             
