/*
 * Copyright (c) 2000-2012 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1990, 1993
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
 *	@(#)filedesc.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _SYS_FILEDESC_H_
#define _SYS_FILEDESC_H_

#include <sys/appleapiopts.h>

  
                                                                       
                                
  
                                                                               
                                                                              
                                                                         
                                                                          
                                                                             
                                                                            
                                                                           
                                                                         
                                                         
   
#define NDFILE          25                             
#define NDEXTENT        50                                                

#ifdef BSD_KERNEL_PRIVATE

#include <kern/locks.h>

struct klist;
struct kqwllist;

struct filedesc {
	struct  fileproc **fd_ofiles;                                       
	lck_mtx_t fd_kqhashlock;                                          
	u_long  fd_kqhashmask;                                           
	struct  kqwllist *fd_kqhash;                                        
	struct  kqworkq *fd_wqkqueue;                         
	char    *fd_ofileflags;                                          
	struct  vnode *fd_cdir;                                
	struct  vnode *fd_rdir;                             
	int     fd_nfiles;                                                  
	int     fd_lastfile;                                              
	int     fd_freefile;                                        
	mode_t  fd_cmask;                                           
	int     fd_flags;
	int     fd_knlistsize;                              
	struct  klist *fd_knlist;                                    
	u_long  fd_knhashmask;                              
	struct  klist *fd_knhash;                                           
	lck_mtx_t fd_knhashlock;                                                     
};

  
                            
   
#define FD_CHROOT       0x01                                                 
                                                                            
                                                                               

#define FD_WORKLOOP     0x02                                               
                                                                     

  
                          
   
#define UF_EXCLOSE      0x01                                    
#define UF_FORKCLOSE    0x02                                    
#define UF_RESERVED     0x04                                            
#define UF_CLOSING      0x08                                   
#define UF_RESVWAIT     0x10                                   
#define UF_INHERIT      0x20                                   

#define UF_VALID_FLAGS  \
	(UF_EXCLOSE | UF_FORKCLOSE | UF_RESERVED | UF_CLOSING |\
	UF_RESVWAIT | UF_INHERIT)

  
                                             
   
#define OFILESIZE (sizeof(struct file *) + sizeof(char))

   
                       
  
         
                                                
   
struct fdt_iterator {
	int              fdti_fd;
	struct fileproc *fdti_fp;
};

   
                     
  
         
                             
  
           
                                                                     
  
            
                                                             
  
                      
                                                                  
                                                                             
  
           
                              
                                                
   
extern struct fdt_iterator
fdt_next(proc_t p, int fd, bool only_settled);

   
                     
  
         
                               
  
           
                                                                     
  
            
                                                             
  
                      
                                                                  
                                                                             
  
           
                              
                                                
   
extern struct fdt_iterator
fdt_prev(proc_t p, int fd, bool only_settled);

   
                   
  
         
                                                                              
                         
  
            
                          
  
           
                                                                     
   
#define fdt_foreach(fp, p) \
	for (struct fdt_iterator __fdt_it = fdt_next(p, -1, true); \
	    ((fp) = __fdt_it.fdti_fp); \
	    __fdt_it = fdt_next(p, __fdt_it.fdti_fd, true))

   
                      
  
         
                                                                       
                   
   
#define fdt_foreach_fd()  __fdt_it.fdti_fd

  
                                        
   
extern int      dupfdopen(struct filedesc *fdp,
    int indx, int dfd, int mode, int error);
extern int      fdalloc(proc_t p, int want, int *result);
extern int      fdavail(proc_t p, int n);
#define         fdfile(p, fd)                                   \
	                (&(p)->p_fd->fd_ofiles[(fd)])
#define         fdflags(p, fd)                                  \
	                (&(p)->p_fd->fd_ofileflags[(fd)])

  
                              
   
#define FDFLAGS_GET(p, fd) (*fdflags(p, fd) & (UF_EXCLOSE|UF_FORKCLOSE))
#define FDFLAGS_SET(p, fd, bits) \
	   (*fdflags(p, fd) |= ((bits) & (UF_EXCLOSE|UF_FORKCLOSE)))
#define FDFLAGS_CLR(p, fd, bits) \
	   (*fdflags(p, fd) &= ~((bits) & (UF_EXCLOSE|UF_FORKCLOSE)))

extern int      falloc(proc_t p, struct fileproc **resultfp, int *resultfd, vfs_context_t ctx);

typedef struct fileproc *(*fp_allocfn_t)(void *);
extern int      falloc_withalloc(proc_t p, struct fileproc **resultfp,
    int *resultfd, vfs_context_t ctx,
    fp_allocfn_t fp_zalloc, void *crarg);

extern struct   filedesc *fdcopy(proc_t p, struct vnode *uth_cdir);
extern void     fdfree(proc_t p);
extern void     fdexec(proc_t p, short flags, int self_exec);

#endif                         

#endif                        
