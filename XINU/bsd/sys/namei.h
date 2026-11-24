/*
 * Copyright (c) 2000-2014 Apple Inc. All rights reserved.
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
 * Copyright (c) 1985, 1989, 1991, 1993
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
 *	@(#)namei.h	8.4 (Berkeley) 8/20/94
 */

#ifndef _SYS_NAMEI_H_
#define _SYS_NAMEI_H_

#include <sys/appleapiopts.h>

#ifdef KERNEL
#define LOCKLEAF        0x0004                            
#define LOCKPARENT      0x0008                                  
#define WANTPARENT      0x0010                                  

#ifdef KERNEL_PRIVATE
#define CN_SECLUDE_RENAME 0x10000000                                                
#define CN_RAW_ENCRYPTED 0x80000000                                              
#endif

#endif          

#ifdef BSD_KERNEL_PRIVATE

                                              
#define NAMEDRSRCFORK           NAMEDSTREAMS


#include <sys/queue.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/filedesc.h>

#define PATHBUFLEN      256

  
                                     
   
struct nameidata {
	  
                              
    
	user_addr_t ni_dirp;                                  
	enum    uio_seg ni_segflg;                                
#if CONFIG_TRIGGERS
	enum    path_operation ni_op;                                                               
#endif                      
	  
                        
    
	struct  vnode *ni_startdir;                             
	struct  vnode *ni_rootdir;                                  
	struct  vnode *ni_usedvp;                                           
	  
                                                
    
	struct  vnode *ni_vp;                                
	struct  vnode *ni_dvp;                                               
	  
                                                    
    
	u_int   ni_pathlen;                                          
	char    *ni_next;                                              
	char    ni_pathbuf[PATHBUFLEN];
	u_long  ni_loopcnt;                                                

	struct componentname ni_cnd;
	int32_t ni_flag;
	int ni_ncgeneration;                                                                
};

#define NAMEI_CONTLOOKUP        0x002                                                                                       
#define NAMEI_TRAILINGSLASH     0x004                                                                    
#define NAMEI_UNFINISHED        0x008                                                   

  
                                                            
                                                                     
                                                        
   
#define NAMEI_COMPOUNDOPEN      0x010
#define NAMEI_COMPOUNDREMOVE    0x020
#define NAMEI_COMPOUNDMKDIR     0x040
#define NAMEI_COMPOUNDRMDIR     0x080
#define NAMEI_COMPOUNDRENAME    0x100
#define NAMEI_COMPOUND_OP_MASK (NAMEI_COMPOUNDOPEN | NAMEI_COMPOUNDREMOVE | NAMEI_COMPOUNDMKDIR | NAMEI_COMPOUNDRMDIR | NAMEI_COMPOUNDRENAME)

#define NAMEI_NOFOLLOW_ANY      0x1000                                       

#ifdef KERNEL
  
                                                           
                                                                           
   
#define NOCACHE         0x00000020                                     
#define NOFOLLOW        0x00000000                                            
                                             
#define SHAREDLEAF      0x00000080                                  
#define MODMASK         0x100000fc                                    
  
                               
  
                                                                     
                                                                     
                                                                  
                                                                 
                                        
   
#define SAVENAME        0                                                
#define NOCROSSMOUNT    0x00000100                                
#define RDONLY          0x00000200                                      
#define HASBUF          0x00000400                                    
#define DONOTAUTH       0x00000800                                     
#define SAVESTART       0x00001000                              
                                               
                                                
                                               
#define ISSYMLINK       0x00010000                                   
                                                 
                                                 
#define WILLBEDIR       0x00080000                                               
#define AUDITVNPATH1    0x00100000                                
#define AUDITVNPATH2    0x00200000                                
#define USEDVP          0x00400000                                     
#define CN_VOLFSPATH    0x00800000                                       
#define CN_FIRMLINK_NOFOLLOW    0x01000000                               
#define UNIONCREATED    0x02000000                                 
#if NAMEDRSRCFORK
#define CN_WANTSRSRCFORK 0x04000000
#define CN_ALLOWRSRCFORK 0x08000000
#endif                 
                                                         
#define CN_NBMOUNTLOOK  0x20000000                                           
#ifdef BSD_KERNEL_PRIVATE
#define CN_SKIPNAMECACHE        0x40000000                                                                         
#endif
                                                        

  
                                            
   

#if CONFIG_TRIGGERS
                                                                      

#define NDINIT(ndp, op, pop, flags, segflg, namep, ctx) { \
	(ndp)->ni_cnd.cn_nameiop = op; \
	(ndp)->ni_op = pop; \
	(ndp)->ni_cnd.cn_flags = flags; \
	if ((segflg) == UIO_USERSPACE) { \
	        (ndp)->ni_segflg = ((IS_64BIT_PROCESS(vfs_context_proc(ctx))) ? UIO_USERSPACE64 : UIO_USERSPACE32); \
	} \
	else { \
	        (ndp)->ni_segflg = segflg; \
	} \
	(ndp)->ni_dirp = namep; \
	(ndp)->ni_cnd.cn_context = ctx; \
	(ndp)->ni_flag = 0; \
	(ndp)->ni_cnd.cn_ndp = (ndp); \
}
#else
#define NDINIT(ndp, op, _unused_, flags, segflg, namep, ctx) { \
	(ndp)->ni_cnd.cn_nameiop = op; \
	(ndp)->ni_cnd.cn_flags = flags; \
	if ((segflg) == UIO_USERSPACE) { \
	        (ndp)->ni_segflg = ((IS_64BIT_PROCESS(vfs_context_proc(ctx))) ? UIO_USERSPACE64 : UIO_USERSPACE32); \
	} \
	else { \
	        (ndp)->ni_segflg = segflg; \
	} \
	(ndp)->ni_dirp = namep; \
	(ndp)->ni_cnd.cn_context = ctx; \
	(ndp)->ni_flag = 0; \
	(ndp)->ni_cnd.cn_ndp = (ndp); \
}
#endif                      

#endif             

  
                                                               
                            
   
struct  namecache {
	TAILQ_ENTRY(namecache)  nc_entry;                                 
	TAILQ_ENTRY(namecache)  nc_child;                                                     
	union {
		LIST_ENTRY(namecache)  nc_link;                                      
		TAILQ_ENTRY(namecache) nc_negentry;                                      
	} nc_un;
	LIST_ENTRY(namecache)   nc_hash;                        
	vnode_t                 nc_dvp;                                      
	vnode_t                 nc_vp;                                        
	unsigned int            nc_hashval;                                
	const char              *nc_name;                                                    
};


#ifdef KERNEL

int     namei(struct nameidata *ndp);
void    nameidone(struct nameidata *);
int     lookup(struct nameidata *ndp);
int     relookup(struct vnode *dvp, struct vnode **vpp,
    struct componentname *cnp);
int     lookup_traverse_union(vnode_t dvp, vnode_t *new_dvp, vfs_context_t ctx);
void    lookup_compound_vnop_post_hook(int error, vnode_t dvp, vnode_t vp, struct nameidata *ndp, int did_create);
void    kdebug_lookup(struct vnode *dp, struct componentname *cnp);

  
                                
   
void    cache_purgevfs(mount_t mp);
int             cache_lookup_path(struct nameidata *ndp, struct componentname *cnp, vnode_t dp,
    vfs_context_t context, int *dp_authorized, vnode_t last_dp);

void            vnode_cache_authorized_action(vnode_t vp, vfs_context_t context, kauth_action_t action);
void            vnode_uncache_authorized_action(vnode_t vp, kauth_action_t action);
boolean_t       vnode_cache_is_stale(vnode_t vp);
boolean_t       vnode_cache_is_authorized(vnode_t vp, vfs_context_t context, kauth_action_t action);
int             lookup_validate_creation_path(struct nameidata *ndp);
int             namei_compound_available(vnode_t dp, struct nameidata *ndp);

#endif             

  
                                       
   
struct  nchstats {
	long    ncs_goodhits;                                            
	long    ncs_neghits;                                               
	long    ncs_badhits;                                   
	long    ncs_miss;                           
	long    ncs_pass2;                                                
	long    ncs_2passes;                                               
	long    ncs_stolen;
	long    ncs_enters;
	long    ncs_deletes;
	long    ncs_badvid;
};
#endif                         

#endif                     
