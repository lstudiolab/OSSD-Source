/*
 * Copyright (c) 2000-2015 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)vfs_lookup.c	8.10 (Berkeley) 5/27/95
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslimits.h>
#include <sys/time.h>
#include <sys/namei.h>
#include <sys/vm.h>
#include <sys/vnode_internal.h>
#include <sys/mount_internal.h>
#include <sys/errno.h>
#include <kern/kalloc.h>
#include <sys/filedesc.h>
#include <sys/proc_internal.h>
#include <sys/kdebug.h>
#include <sys/unistd.h>                               
#include <sys/uio_internal.h>
#include <sys/kauth.h>
#include <kern/zalloc.h>
#include <security/audit/audit.h>
#if CONFIG_MACF
#include <security/mac_framework.h>
#endif
#include <os/atomic_private.h>

#include <sys/paths.h>

#if NAMEDRSRCFORK
#include <sys/xattr.h>
#endif
  
                                         
                        
   
#define VOLFS_MIN_PATH_LEN  9


#if CONFIG_VOLFS
static int vfs_getrealpath(const char * path, char * realpath, size_t bufsize, vfs_context_t ctx);
#define MAX_VOLFS_RESTARTS 5
#endif

static int              lookup_traverse_mountpoints(struct nameidata *ndp, struct componentname *cnp, vnode_t dp, int vbusyflags, vfs_context_t ctx);
static int              lookup_handle_symlink(struct nameidata *ndp, vnode_t *new_dp, bool* dp_has_iocount, vfs_context_t ctx);
static int              lookup_authorize_search(vnode_t dp, struct componentname *cnp, int dp_authorized_in_cache, vfs_context_t ctx);
static void             lookup_consider_update_cache(vnode_t dvp, vnode_t vp, struct componentname *cnp, int nc_generation);
static int              lookup_handle_found_vnode(struct nameidata *ndp, struct componentname *cnp, int rdonly,
    int vbusyflags, int *keep_going, int nc_generation,
    int wantparent, int atroot, vfs_context_t ctx);
static int              lookup_handle_emptyname(struct nameidata *ndp, struct componentname *cnp, int wantparent);

#if NAMEDRSRCFORK
static int              lookup_handle_rsrc_fork(vnode_t dp, struct nameidata *ndp, struct componentname *cnp, int wantparent, vfs_context_t ctx);
#endif

extern lck_rw_t rootvnode_rw_lock;

  
                                                       
  
                                                                
                                                              
                                                            
                                  
  
                                                                
                         
  
                            
  
               
                         
                            
                               
                                                         
    
  
                       
                                      
                                             
                                   
                                
                                            
                                     
                
                 
                
                                                                     
                                                              
               
                     
   
int
namei(struct nameidata *ndp)
{
	struct filedesc *fdp;                                         
	struct vnode *dp;                                           
	struct vnode *usedvp = ndp->ni_dvp;                                                    
                                                                                                         
	uint32_t cnpflags = ndp->ni_cnd.cn_flags;                                                  
	int error;
	struct componentname *cnp = &ndp->ni_cnd;
	vfs_context_t ctx = cnp->cn_context;
	proc_t p = vfs_context_proc(ctx);
#if CONFIG_AUDIT
                                   
	uthread_t ut = (struct uthread *)get_bsdthread_info(current_thread());
#endif

#if CONFIG_VOLFS
	int volfs_restarts = 0;
#endif
	size_t bytes_copied = 0;
	vnode_t rootdir_with_usecount = NULLVP;
	vnode_t startdir_with_usecount = NULLVP;
	vnode_t usedvp_dp = NULLVP;
	int32_t old_count = 0;
	bool dp_has_iocount = false;

	fdp = p->p_fd;

#if DIAGNOSTIC
	if (!vfs_context_ucred(ctx) || !p) {
		panic("namei: bad cred/proc");
	}
	if (cnp->cn_nameiop & (~OPMASK)) {
		panic("namei: nameiop contaminated with flags");
	}
	if (cnp->cn_flags & OPMASK) {
		panic("namei: flags contaminated with nameiops");
	}
#endif

	  
                                                                  
                                                              
    
	if (ndp->ni_flag & NAMEI_CONTLOOKUP) {
		int rdonly, vbusyflags, keep_going, wantparent;

		rdonly = cnp->cn_flags & RDONLY;
		vbusyflags = ((cnp->cn_flags & CN_NBMOUNTLOOK) != 0) ? LK_NOWAIT : 0;
		keep_going = 0;
		wantparent = cnp->cn_flags & (LOCKPARENT | WANTPARENT);

		ndp->ni_flag &= ~(NAMEI_CONTLOOKUP);

		error = lookup_handle_found_vnode(ndp, &ndp->ni_cnd, rdonly, vbusyflags,
		    &keep_going, ndp->ni_ncgeneration, wantparent, 0, ctx);
		if (error) {
			goto out_drop;
		}
		if (keep_going) {
			if ((cnp->cn_flags & ISSYMLINK) == 0) {
				panic("We need to keep going on a continued lookup, but for vp type %d (tag %d)\n", ndp->ni_vp->v_type, ndp->ni_vp->v_tag);
			}
			goto continue_symlink;
		}

		return 0;
	}

vnode_recycled:

	  
                                                            
                         
    
	if ((cnp->cn_flags & HASBUF) == 0) {
		cnp->cn_pnbuf = ndp->ni_pathbuf;
		cnp->cn_pnlen = PATHBUFLEN;
	}
#if LP64_DEBUG
	if ((UIO_SEG_IS_USER_SPACE(ndp->ni_segflg) == 0)
	    && (ndp->ni_segflg != UIO_SYSSPACE)
	    && (ndp->ni_segflg != UIO_SYSSPACE32)) {
		panic("%s :%d - invalid ni_segflg\n", __FILE__, __LINE__);
	}
#endif                 

retry_copy:
	if (UIO_SEG_IS_USER_SPACE(ndp->ni_segflg)) {
		error = copyinstr(ndp->ni_dirp, cnp->cn_pnbuf,
		    cnp->cn_pnlen, &bytes_copied);
	} else {
		error = copystr(CAST_DOWN(void *, ndp->ni_dirp), cnp->cn_pnbuf,
		    cnp->cn_pnlen, &bytes_copied);
	}
	if (error == ENAMETOOLONG && !(cnp->cn_flags & HASBUF)) {
		cnp->cn_pnbuf = zalloc(ZV_NAMEI);
		cnp->cn_flags |= HASBUF;
		cnp->cn_pnlen = MAXPATHLEN;
		bytes_copied = 0;

		goto retry_copy;
	}
	if (error) {
		goto error_out;
	}
	assert(bytes_copied <= MAXPATHLEN);
	ndp->ni_pathlen = (u_int)bytes_copied;
	bytes_copied = 0;

	  
                                                        
                                                          
                                                       
                                                        
                                                
    
	if (proc_is_forcing_hfs_case_sensitivity(p)) {
		cnp->cn_flags |= CN_SKIPNAMECACHE;
	}

#if CONFIG_VOLFS
	  
                                           
   
                                                             
                                                       
    
	if (ndp->ni_pathlen >= VOLFS_MIN_PATH_LEN &&
	    cnp->cn_pnbuf[0] == '/' &&
	    cnp->cn_pnbuf[1] == '.' &&
	    cnp->cn_pnbuf[2] == 'v' &&
	    cnp->cn_pnbuf[3] == 'o' &&
	    cnp->cn_pnbuf[4] == 'l' &&
	    cnp->cn_pnbuf[5] == '/') {
		char * realpath;
		int realpath_err;
		                                                       
		realpath = zalloc(ZV_NAMEI);
		  
                                                                   
                                                                          
                                                                                  
                                                                                  
     
		if ((realpath_err = vfs_getrealpath(&cnp->cn_pnbuf[6], realpath, MAXPATHLEN, ctx))) {
			zfree(ZV_NAMEI, realpath);
			if (realpath_err == ENOSPC || realpath_err == ENAMETOOLONG) {
				error = ENAMETOOLONG;
				goto error_out;
			}
		} else {
			size_t tmp_len;
			if (cnp->cn_flags & HASBUF) {
				zfree(ZV_NAMEI, cnp->cn_pnbuf);
			}
			cnp->cn_pnbuf = realpath;
			cnp->cn_pnlen = MAXPATHLEN;
			tmp_len = strlen(realpath) + 1;
			assert(tmp_len <= UINT_MAX);
			ndp->ni_pathlen = (u_int)tmp_len;
			cnp->cn_flags |= HASBUF | CN_VOLFSPATH;
		}
	}
#endif                   

#if CONFIG_AUDIT
	                                                                    
	if (cnp->cn_flags & AUDITVNPATH1) {
		AUDIT_ARG(upath, ut->uu_cdir, cnp->cn_pnbuf, ARG_UPATH1);
	}
	if (cnp->cn_flags & AUDITVNPATH2) {
		AUDIT_ARG(upath, ut->uu_cdir, cnp->cn_pnbuf, ARG_UPATH2);
	}
#endif                   

	  
                                
    
	if (*cnp->cn_pnbuf == '\0') {
		error = ENOENT;
		goto error_out;
	}
	if (ndp->ni_flag & NAMEI_NOFOLLOW_ANY) {
		ndp->ni_loopcnt = MAXSYMLINKS;
	} else {
		ndp->ni_loopcnt = 0;
	}

	  
                                                     
    
	proc_dirs_lock_shared(p);
	lck_rw_lock_shared(&rootvnode_rw_lock);

	if (!(fdp->fd_flags & FD_CHROOT)) {
		ndp->ni_rootdir = rootvnode;
	} else {
		ndp->ni_rootdir = fdp->fd_rdir;
	}

	if (!ndp->ni_rootdir) {
		if (!(fdp->fd_flags & FD_CHROOT)) {
			printf("rootvnode is not set\n");
		} else {
			                            
			printf("fdp->fd_rdir is not set\n");
		}
		lck_rw_unlock_shared(&rootvnode_rw_lock);
		proc_dirs_unlock_shared(p);
		error = ENOENT;
		goto error_out;
	}

	cnp->cn_nameptr = cnp->cn_pnbuf;

	ndp->ni_usedvp = NULLVP;

	if (*(cnp->cn_nameptr) == '/') {
		while (*(cnp->cn_nameptr) == '/') {
			cnp->cn_nameptr++;
			ndp->ni_pathlen--;
		}
		dp = ndp->ni_rootdir;
	} else if (cnp->cn_flags & USEDVP) {
		dp = ndp->ni_dvp;
		ndp->ni_usedvp = dp;
		usedvp_dp = dp;
	} else {
		dp = vfs_context_cwd(ctx);
	}

	if (dp == NULLVP || (dp->v_lflag & VL_DEAD)) {
		dp = NULLVP;
		lck_rw_unlock_shared(&rootvnode_rw_lock);
		proc_dirs_unlock_shared(p);
		error = ENOENT;
		goto error_out;
	}

	  
                                                                          
                                                                           
                                                                        
                                                                            
                                                                        
                                                       
   
                                                                                
                                                                          
                                                                           
                                                                       
                                                                       
                                                                           
                                                                              
              
    
	rootdir_with_usecount = ndp->ni_rootdir;
	old_count = os_atomic_inc_orig(&rootdir_with_usecount->v_usecount, relaxed);
	if (old_count < 1) {
		panic("(1) invalid pre-increment usecount (%d) for rootdir vnode %p",
		    old_count, rootdir_with_usecount);
	} else if (old_count == INT32_MAX) {
		panic("(1) usecount overflow for vnode %p", rootdir_with_usecount);
	}

	if ((dp != rootdir_with_usecount) && (dp != usedvp_dp)) {
		old_count = os_atomic_inc_orig(&dp->v_usecount, relaxed);
		if (old_count < 1) {
			panic("(2) invalid pre-increment usecount (%d) for vnode %p", old_count, dp);
		} else if (old_count == INT32_MAX) {
			panic("(2) usecount overflow for vnode %p", dp);
		}
		startdir_with_usecount = dp;
	}

	                                                      
	lck_rw_unlock_shared(&rootvnode_rw_lock);
	proc_dirs_unlock_shared(p);

	ndp->ni_dvp = NULLVP;
	ndp->ni_vp  = NULLVP;

	for (;;) {
#if CONFIG_MACF
		  
                                                     
                                                 
                                                      
                                              
                                                      
                                                  
     
		error = mac_vnode_check_lookup_preflight(ctx, dp, cnp->cn_nameptr, cnp->cn_namelen);
		if (error) {
			goto error_out;
		}
#endif
		ndp->ni_startdir = dp;
		dp = NULLVP;

		if ((error = lookup(ndp))) {
			goto error_out;
		}

		  
                            
     
		if ((cnp->cn_flags & ISSYMLINK) == 0) {
			if (startdir_with_usecount) {
				vnode_rele(startdir_with_usecount);
				startdir_with_usecount = NULLVP;
			}
			if (rootdir_with_usecount) {
				lck_rw_lock_shared(&rootvnode_rw_lock);
				if (rootdir_with_usecount == rootvnode) {
					old_count = os_atomic_dec_orig(&rootdir_with_usecount->v_usecount, relaxed);
					if (old_count < 2) {
						  
                                                                           
         
						panic("(3) Unexpected pre-decrement value (%d) of usecount for rootvnode %p",
						    old_count, rootdir_with_usecount);
					}
					rootdir_with_usecount = NULLVP;
				}
				lck_rw_unlock_shared(&rootvnode_rw_lock);
				if (rootdir_with_usecount) {
					vnode_rele(rootdir_with_usecount);
					rootdir_with_usecount = NULLVP;
				}
			}

			return 0;
		}

continue_symlink:
		                                                        
		error = lookup_handle_symlink(ndp, &dp, &dp_has_iocount, ctx);
		if (error != 0) {
			break;
		}
		if (dp_has_iocount) {
			if ((dp != rootdir_with_usecount) && (dp != startdir_with_usecount) &&
			    (dp != usedvp_dp)) {
				if (startdir_with_usecount) {
					vnode_rele(startdir_with_usecount);
				}
				vnode_ref_ext(dp, 0, VNODE_REF_FORCE);
				startdir_with_usecount = dp;
			}
			vnode_put(dp);
			dp_has_iocount = false;
		}
	}
	  
                                                    
                                               
                                               
                            
    
out_drop:
	if (ndp->ni_dvp) {
		vnode_put(ndp->ni_dvp);
	}
	if (ndp->ni_vp) {
		vnode_put(ndp->ni_vp);
	}
error_out:
	if (startdir_with_usecount) {
		vnode_rele(startdir_with_usecount);
		startdir_with_usecount = NULLVP;
	}
	if (rootdir_with_usecount) {
		lck_rw_lock_shared(&rootvnode_rw_lock);
		if (rootdir_with_usecount == rootvnode) {
			old_count = os_atomic_dec_orig(&rootdir_with_usecount->v_usecount, relaxed);
			if (old_count < 2) {
				  
                                                                         
       
				panic("(4) Unexpected pre-decrement value (%d) of usecount for rootvnode %p",
				    old_count, rootdir_with_usecount);
			}
			lck_rw_unlock_shared(&rootvnode_rw_lock);
		} else {
			lck_rw_unlock_shared(&rootvnode_rw_lock);
			vnode_rele(rootdir_with_usecount);
		}
		rootdir_with_usecount = NULLVP;
	}

	if ((cnp->cn_flags & HASBUF)) {
		cnp->cn_flags &= ~HASBUF;
		zfree(ZV_NAMEI, cnp->cn_pnbuf);
	}
	cnp->cn_pnbuf = NULL;
	ndp->ni_vp = NULLVP;
	ndp->ni_dvp = NULLVP;

#if CONFIG_VOLFS
	  
                            
   
                                                                
                                                                       
                                                                         
                                                                   
                                                                        
                                          
   
                                                                           
                                                                              
                                                                        
                                                                            
                                       
   
                                                                    
                                                                           
                                                            
   
                                                        
    
	if ((error == ENOENT) && (ndp->ni_cnd.cn_flags & CN_VOLFSPATH)) {
		if (volfs_restarts < MAX_VOLFS_RESTARTS) {
			volfs_restarts++;
			goto vnode_recycled;
		}
	}
#endif

	if (error == ERECYCLE) {
		                                                                
                                                                    
		ndp->ni_cnd.cn_flags = cnpflags;
		ndp->ni_dvp = usedvp;
		goto vnode_recycled;
	}


	return error;
}

int
namei_compound_available(vnode_t dp, struct nameidata *ndp)
{
	if ((ndp->ni_flag & NAMEI_COMPOUNDOPEN) != 0) {
		return vnode_compound_open_available(dp);
	}

	return 0;
}

static int
lookup_authorize_search(vnode_t dp, struct componentname *cnp, int dp_authorized_in_cache, vfs_context_t ctx)
{
#if !CONFIG_MACF
#pragma unused(cnp)
#endif

	int error;

	if (!dp_authorized_in_cache) {
		error = vnode_authorize(dp, NULL, KAUTH_VNODE_SEARCH, ctx);
		if (error) {
			return error;
		}
	}
#if CONFIG_MACF
	error = mac_vnode_check_lookup(ctx, dp, cnp);
	if (error) {
		return error;
	}
#endif                  

	return 0;
}

static void
lookup_consider_update_cache(vnode_t dvp, vnode_t vp, struct componentname *cnp, int nc_generation)
{
	int isdot_or_dotdot;
	isdot_or_dotdot = (cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.') || (cnp->cn_flags & ISDOTDOT);

	if (vp->v_name == NULL || vp->v_parent == NULLVP) {
		int  update_flags = 0;

		if (isdot_or_dotdot == 0) {
			if (vp->v_name == NULL) {
				update_flags |= VNODE_UPDATE_NAME;
			}
			if (dvp != NULLVP && vp->v_parent == NULLVP) {
				update_flags |= VNODE_UPDATE_PARENT;
			}

			if (update_flags) {
				vnode_update_identity(vp, dvp, cnp->cn_nameptr, cnp->cn_namelen, cnp->cn_hash, update_flags);
			}
		}
	}
	if ((cnp->cn_flags & MAKEENTRY) && (vp->v_flag & VNCACHEABLE) && LIST_FIRST(&vp->v_nclinks) == NULL) {
		  
                                        
                                         
                                          
                                          
                    
                                            
                                        
                                                     
                                                    
                                                    
                                                      
     
		if (dvp != NULLVP && (nc_generation == dvp->v_nc_generation) && (!isdot_or_dotdot)) {
			cache_enter_with_gen(dvp, vp, cnp, nc_generation);
		}
	}
}

#if NAMEDRSRCFORK
  
                                                                                               
                                                                                                        
                         
   
static int
lookup_handle_rsrc_fork(vnode_t dp, struct nameidata *ndp, struct componentname *cnp, int wantparent, vfs_context_t ctx)
{
	vnode_t svp = NULLVP;
	enum nsoperation nsop;
	int nsflags;
	int error;

	if (dp->v_type != VREG) {
		error = ENOENT;
		goto out;
	}
	switch (cnp->cn_nameiop) {
	case DELETE:
		if (cnp->cn_flags & CN_ALLOWRSRCFORK) {
			nsop = NS_DELETE;
		} else {
			error = EPERM;
			goto out;
		}
		break;
	case CREATE:
		if (cnp->cn_flags & CN_ALLOWRSRCFORK) {
			nsop = NS_CREATE;
		} else {
			error = EPERM;
			goto out;
		}
		break;
	case LOOKUP:
		                                                             
		if (cnp->cn_flags & CN_ALLOWRSRCFORK) {
			nsop = NS_OPEN;
		} else {
			error = EPERM;
			goto out;
		}
		break;
	default:
		error = EPERM;
		goto out;
	}

	nsflags = 0;
	if (cnp->cn_flags & CN_RAW_ENCRYPTED) {
		nsflags |= NS_GETRAWENCRYPTED;
	}

	                                                
	error = vnode_getnamedstream(dp, &svp, XATTR_RESOURCEFORK_NAME, nsop, nsflags, ctx);

	                                                            
	if (error == ENOATTR || error == ENOENT) {
		error = (nsop == NS_CREATE) ? 0 : ENOENT;
	}
	if (error) {
		goto out;
	}
	                                             
	if (wantparent) {
		if (ndp->ni_dvp) {
			vnode_put(ndp->ni_dvp);
		}
		ndp->ni_dvp = dp;
	} else {
		vnode_put(dp);
	}
	ndp->ni_vp = svp;                                  

	                                                         
	if (ndp->ni_pathlen == 1 && ndp->ni_next[0] == '\0') {
		  
                                                              
                                                              
                                                                  
                                                              
                                                              
                   
     
		strncpy(ndp->ni_next, _PATH_RSRCFORKSPEC, sizeof(_PATH_RSRCFORKSPEC));
		cnp->cn_nameptr = ndp->ni_next + 1;
		cnp->cn_namelen = sizeof(_PATH_RSRCFORKSPEC) - 1;
		ndp->ni_next += cnp->cn_namelen;
		if (ndp->ni_next[0] != '\0') {
			panic("Incorrect termination of path in %s", __FUNCTION__);
		}
	}
	cnp->cn_flags  &= ~MAKEENTRY;

	return 0;
out:
	return error;
}
#endif                    

  
               
                                                                                           
                                                                                   
  
                
                                                                              
                                                                          
                                                                                 
                
                                                                                                      
                                                                                         
                
   
static int
lookup_handle_found_vnode(struct nameidata *ndp, struct componentname *cnp, int rdonly,
    int vbusyflags, int *keep_going, int nc_generation,
    int wantparent, int atroot, vfs_context_t ctx)
{
	vnode_t dp;
	int error;
	char *cp;

	dp = ndp->ni_vp;
	*keep_going = 0;

	if (ndp->ni_vp == NULLVP) {
		panic("NULL ni_vp in %s\n", __FUNCTION__);
	}

	if (atroot) {
		goto nextname;
	}

	  
                                                           
                              
    
	if (cnp->cn_consume > 0) {
		cnp->cn_nameptr += cnp->cn_consume;
		ndp->ni_next += cnp->cn_consume;
		ndp->ni_pathlen -= cnp->cn_consume;
		cnp->cn_consume = 0;
	} else {
		lookup_consider_update_cache(ndp->ni_dvp, dp, cnp, nc_generation);
	}

	  
                                                    
                                                   
                       
    
	error = lookup_traverse_mountpoints(ndp, cnp, dp, vbusyflags, ctx);
	dp = ndp->ni_vp;
	if (error) {
		goto out;
	}

#if CONFIG_MACF
	if (vfs_flags(vnode_mount(dp)) & MNT_MULTILABEL) {
		error = vnode_label(vnode_mount(dp), NULL, dp, NULL, 0, ctx);
		if (error) {
			goto out;
		}
	}
#endif

	  
                           
    
	if ((dp->v_type == VLNK) &&
	    ((cnp->cn_flags & FOLLOW) || (ndp->ni_flag & NAMEI_TRAILINGSLASH) || *ndp->ni_next == '/')) {
		cnp->cn_flags |= ISSYMLINK;
		*keep_going = 1;
		return 0;
	}

	  
                                     
    
	if ((ndp->ni_flag & NAMEI_TRAILINGSLASH)) {
		if (dp->v_type != VDIR) {
			error = ENOTDIR;
			goto out;
		}
		ndp->ni_flag &= ~(NAMEI_TRAILINGSLASH);
	}

#if NAMEDSTREAMS
	  
                                                                           
                                                                             
                                      
    
	if (vnode_isshadow(dp)) {
		error = ENOENT;
		goto out;
	}
#endif

nextname:
	  
                                           
                                            
   
                                                  
    
	if (*ndp->ni_next == '/') {
		cnp->cn_nameptr = ndp->ni_next + 1;
		ndp->ni_pathlen--;
		while (*cnp->cn_nameptr == '/') {
			cnp->cn_nameptr++;
			ndp->ni_pathlen--;
		}

		cp = cnp->cn_nameptr;
		vnode_put(ndp->ni_dvp);
		ndp->ni_dvp = NULLVP;

		if (*cp == '\0') {
			goto emptyname;
		}

		*keep_going = 1;
		return 0;
	}

	  
                                                                
    
	if (rdonly &&
	    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
		error = EROFS;
		goto out;
	}

	                                               
	if (cnp->cn_flags & SAVESTART) {
		  
                                          
                                               
                                             
                                         
     
		if ((vnode_get(ndp->ni_dvp))) {
			error = ENOENT;
			goto out;
		}
		ndp->ni_startdir = ndp->ni_dvp;
	}
	if (!wantparent && ndp->ni_dvp) {
		vnode_put(ndp->ni_dvp);
		ndp->ni_dvp = NULLVP;
	}

	if (cnp->cn_flags & AUDITVNPATH1) {
		AUDIT_ARG(vnpath, dp, ARG_VNODE1);
	} else if (cnp->cn_flags & AUDITVNPATH2) {
		AUDIT_ARG(vnpath, dp, ARG_VNODE2);
	}

#if NAMEDRSRCFORK
	  
                                   
    
	if ((cnp->cn_flags & CN_WANTSRSRCFORK) && (dp != NULLVP)) {
		error = lookup_handle_rsrc_fork(dp, ndp, cnp, wantparent, ctx);
		if (error != 0) {
			goto out;
		}

		dp = ndp->ni_vp;
	}
#endif
	if (kdebug_enable) {
		kdebug_lookup(ndp->ni_vp, cnp);
	}

	return 0;

emptyname:
	error = lookup_handle_emptyname(ndp, cnp, wantparent);
	if (error != 0) {
		goto out;
	}

	return 0;
out:
	return error;
}

  
                                                                                          
   
static int
lookup_handle_emptyname(struct nameidata *ndp, struct componentname *cnp, int wantparent)
{
	vnode_t dp;
	int error = 0;

	dp = ndp->ni_vp;
	cnp->cn_namelen = 0;
	  
                                                      
                                                     
    
	if (dp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}
	if (cnp->cn_nameiop != LOOKUP) {
		error = EISDIR;
		goto out;
	}
	if (wantparent) {
		  
                                          
                                     
                                       
                                        
     
		if ((vnode_get(dp))) {
			error = ENOENT;
			goto out;
		}
		ndp->ni_dvp = dp;
	}
	cnp->cn_flags &= ~ISDOTDOT;
	cnp->cn_flags |= ISLASTCN;
	ndp->ni_next = cnp->cn_nameptr;
	ndp->ni_vp = dp;

	if (cnp->cn_flags & AUDITVNPATH1) {
		AUDIT_ARG(vnpath, dp, ARG_VNODE1);
	} else if (cnp->cn_flags & AUDITVNPATH2) {
		AUDIT_ARG(vnpath, dp, ARG_VNODE2);
	}
	if (cnp->cn_flags & SAVESTART) {
		panic("lookup: SAVESTART");
	}

	return 0;
out:
	return error;
}
  
                     
                                                         
  
                                                                    
                                                                    
                                                                        
                                                                   
                                                       
  
                                                                      
                                                                     
                                                                     
                                                                       
                                                                         
                                                                        
                                                                        
                                                                        
                                                                    
                                                                            
                                                                             
  
                             
  
           
                                                 
                                                   
                                                                      
                                                   
                                                                         
                                                                
                                                           
                            
                                                       
                                                     
                                                        
                                                          
  
                       
                                      
                               
                             
                                          
                                    
                                                                                             
                         
                          
                         
                       
                                                
                                                          
                   
                    
                   
                
   
int
lookup(struct nameidata *ndp)
{
	char    *cp;                                                
	vnode_t         tdp;                          
	vnode_t         dp;                                                 
	int docache = 1;                                                      
	int wantparent;                                                         
	int rdonly;                                                    
	int dp_authorized = 0;
	int error = 0;
	struct componentname *cnp = &ndp->ni_cnd;
	vfs_context_t ctx = cnp->cn_context;
	int vbusyflags = 0;
	int nc_generation = 0;
	vnode_t last_dp = NULLVP;
	int keep_going;
	int atroot;

	  
                                              
    
	if (cnp->cn_flags & NOCACHE) {
		docache = 0;
	}
	wantparent = cnp->cn_flags & (LOCKPARENT | WANTPARENT);
	rdonly = cnp->cn_flags & RDONLY;
	cnp->cn_flags &= ~ISSYMLINK;
	cnp->cn_consume = 0;

	dp = ndp->ni_startdir;
	ndp->ni_startdir = NULLVP;

	if ((cnp->cn_flags & CN_NBMOUNTLOOK) != 0) {
		vbusyflags = LK_NOWAIT;
	}
	cp = cnp->cn_nameptr;

	if (*cp == '\0') {
		if ((vnode_getwithref(dp))) {
			dp = NULLVP;
			error = ENOENT;
			goto bad;
		}
		ndp->ni_vp = dp;
		error = lookup_handle_emptyname(ndp, cnp, wantparent);
		if (error) {
			goto bad;
		}

		return 0;
	}
dirloop:
	atroot = 0;
	ndp->ni_vp = NULLVP;

	if ((error = cache_lookup_path(ndp, cnp, dp, ctx, &dp_authorized, last_dp))) {
		dp = NULLVP;
		goto bad;
	}
	if ((cnp->cn_flags & ISLASTCN)) {
		if (docache) {
			cnp->cn_flags |= MAKEENTRY;
		}
	} else {
		cnp->cn_flags |= MAKEENTRY;
	}

	dp = ndp->ni_dvp;

	if (ndp->ni_vp != NULLVP) {
		  
                                                      
                                                 
                                        
    
                                                       
                                                         
                                                         
                                                          
     
		if (dp != NULLVP) {
			nc_generation = dp->v_nc_generation - 1;
		}

		goto returned_from_lookup_path;
	}

	  
                                   
                                               
                                    
                                       
                                             
                                           
                                                
                                   
    
	if ((cnp->cn_flags & ISDOTDOT)) {
		  
                                                         
                                                       
                    
     
		if (ndp->ni_rootdir && (ndp->ni_rootdir != rootvnode) &&
		    dp != ndp->ni_rootdir) {
			int sdir_error;
			int is_subdir = FALSE;

			sdir_error = vnode_issubdir(dp, ndp->ni_rootdir,
			    &is_subdir, vfs_context_kernel());

			  
                                                         
                                                           
              
      
			if (!sdir_error && !is_subdir) {
				vnode_put(dp);
				dp = ndp->ni_rootdir;
				  
                                                    
                                                
                                                 
                                  
       
				if (vnode_get(dp)) {
					error = ENOENT;
					goto bad;
				}
			}
		}

		for (;;) {
			if (dp == ndp->ni_rootdir || dp == rootvnode) {
				ndp->ni_dvp = dp;
				ndp->ni_vp = dp;
				  
                               
                                              
                                            
                               
                                                 
                                      
       
				if ((vnode_get(dp))) {
					error = ENOENT;
					goto bad;
				}
				atroot = 1;
				goto returned_from_lookup_path;
			}
			if ((dp->v_flag & VROOT) == 0 ||
			    (cnp->cn_flags & NOCROSSMOUNT)) {
				break;
			}
			if (dp->v_mount == NULL) {                         
				error = EBADF;
				goto bad;
			}
			tdp = dp;
			dp = tdp->v_mount->mnt_vnodecovered;

			if ((vnode_getwithref(dp))) {
				vnode_put(tdp);
				dp = NULLVP;
				error = ENOENT;
				goto bad;
			}

			vnode_put(tdp);

			ndp->ni_dvp = dp;
			dp_authorized = 0;
		}
	}

	  
                                                                        
    
unionlookup:
	ndp->ni_vp = NULLVP;

	if (dp->v_type != VDIR) {
		error = ENOTDIR;
		goto lookup_error;
	}
	if ((cnp->cn_flags & DONOTAUTH) != DONOTAUTH) {
		error = lookup_authorize_search(dp, cnp, dp_authorized, ctx);
		if (error) {
			goto lookup_error;
		}
	}

	  
                                                                      
                                                                 
    
#if NAMEDRSRCFORK
	if ((cnp->cn_flags & ISLASTCN) && namei_compound_available(dp, ndp) && !(cnp->cn_flags & CN_WANTSRSRCFORK)) {
#else
	if ((cnp->cn_flags & ISLASTCN) && namei_compound_available(dp, ndp)) {
#endif                    
		ndp->ni_flag |= NAMEI_UNFINISHED;
		ndp->ni_ncgeneration = dp->v_nc_generation;
		return 0;
	}

	nc_generation = dp->v_nc_generation;

	  
         
                                                                             
                                                                                        
                                                                                            
                                                                            
                                                                                  
                                                                               
                                                                 
   
                                                                                       
                                                                                      
                                                                 
   
            
                    
   
                                                                                   
                                                                                
                                                                                  
                                                                          
    

	error = VNOP_LOOKUP(dp, &ndp->ni_vp, cnp, ctx);

	if (error) {
lookup_error:
		if ((error == ENOENT) &&
		    (dp->v_mount != NULL) &&
		    (dp->v_mount->mnt_flag & MNT_UNION)) {
			tdp = dp;
			error = lookup_traverse_union(tdp, &dp, ctx);
			vnode_put(tdp);
			if (error) {
				dp = NULLVP;
				goto bad;
			}

			ndp->ni_dvp = dp;
			dp_authorized = 0;
			goto unionlookup;
		}

		if (error != EJUSTRETURN) {
			goto bad;
		}

		if (ndp->ni_vp != NULLVP) {
			panic("leaf should be empty");
		}

#if NAMEDRSRCFORK
		  
                                                
    
                                                      
                                                             
                                                             
                        
    
                                                          
                                                        
                                                       
     
		if (cnp->cn_flags & CN_WANTSRSRCFORK) {
			error = ENOENT;
			goto bad;
		}
#endif

		error = lookup_validate_creation_path(ndp);
		if (error) {
			goto bad;
		}
		  
                                                         
                                                      
                                               
     
		if (cnp->cn_flags & SAVESTART) {
			if ((vnode_get(ndp->ni_dvp))) {
				error = ENOENT;
				goto bad;
			}
			ndp->ni_startdir = ndp->ni_dvp;
		}
		if (!wantparent) {
			vnode_put(ndp->ni_dvp);
		}

		if (kdebug_enable) {
			kdebug_lookup(ndp->ni_dvp, cnp);
		}
		return 0;
	}
returned_from_lookup_path:
	                                                               
	error = lookup_handle_found_vnode(ndp, cnp, rdonly, vbusyflags, &keep_going, nc_generation, wantparent, atroot, ctx);
	if (error != 0) {
		goto bad2;
	}

	if (keep_going) {
		dp = ndp->ni_vp;

		                                  
		if ((dp->v_type == VLNK) &&
		    ((cnp->cn_flags & FOLLOW) || (ndp->ni_flag & NAMEI_TRAILINGSLASH) || *ndp->ni_next == '/')) {
			return 0;
		}

		  
                                             
                                                                   
                                                                  
                                                            
     
		last_dp = dp;

		goto dirloop;
	}

	return 0;
bad2:
	if (ndp->ni_dvp) {
		vnode_put(ndp->ni_dvp);
	}

	vnode_put(ndp->ni_vp);
	ndp->ni_vp = NULLVP;

	if (kdebug_enable) {
		kdebug_lookup(dp, cnp);
	}
	return error;

bad:
	if (dp) {
		vnode_put(dp);
	}
	ndp->ni_vp = NULLVP;

	if (kdebug_enable) {
		kdebug_lookup(dp, cnp);
	}
	return error;
}

  
                                                             
                                 
   
int
lookup_traverse_union(vnode_t dvp, vnode_t *new_dvp, vfs_context_t ctx)
{
	char *path = NULL, *pp;
	const char *name, *np;
	size_t len;
	int error = 0;
	struct nameidata nd;
	vnode_t vp = dvp;

	*new_dvp = NULL;

	if (vp && vp->v_flag & VROOT) {
		*new_dvp = vp->v_mount->mnt_vnodecovered;
		if (vnode_getwithref(*new_dvp)) {
			return ENOENT;
		}
		return 0;
	}

	path = (char *) zalloc(ZV_NAMEI);
	if (path == NULL) {
		error = ENOMEM;
		goto done;
	}

	  
                                                
                                                    
                                                          
    
	pp = path + (MAXPATHLEN - 1);
	*pp = '\0';

	while (1) {
		name = vnode_getname(vp);
		if (name == NULL) {
			printf("lookup_traverse_union: null parent name: .%s\n", pp);
			error = ENOENT;
			goto done;
		}
		len = strlen(name);
		if ((len + 1) > (size_t)(pp - path)) {                                         
			error = ENAMETOOLONG;
			vnode_putname(name);
			goto done;
		}
		for (np = name + len; len > 0; len--) {                       
			*--pp = *--np;
		}
		vnode_putname(name);
		vp = vp->v_parent;
		if (vp == NULLVP || vp->v_flag & VROOT) {
			break;
		}
		*--pp = '/';
	}

	                                               
	NDINIT(&nd, LOOKUP, OP_LOOKUP, USEDVP, UIO_SYSSPACE, CAST_USER_ADDR_T(pp), ctx);
	nd.ni_dvp = dvp->v_mount->mnt_vnodecovered;
	error = namei(&nd);
	if (error == 0) {
		*new_dvp = nd.ni_vp;
	}
	nameidone(&nd);
done:
	if (path) {
		zfree(ZV_NAMEI, path);
	}
	return error;
}

int
lookup_validate_creation_path(struct nameidata *ndp)
{
	struct componentname *cnp = &ndp->ni_cnd;

	  
                                                         
                                
    
	if (cnp->cn_flags & RDONLY) {
		return EROFS;
	}
	if ((cnp->cn_flags & ISLASTCN) && (ndp->ni_flag & NAMEI_TRAILINGSLASH) && !(cnp->cn_flags & WILLBEDIR)) {
		return ENOENT;
	}

	return 0;
}

  
                                                                              
   
static int
lookup_traverse_mountpoints(struct nameidata *ndp, struct componentname *cnp, vnode_t dp,
    int vbusyflags, vfs_context_t ctx)
{
	mount_t mp;
	vnode_t tdp;
	int error = 0;
	uint32_t depth = 0;
	vnode_t mounted_on_dp;
	int current_mount_generation = 0;
#if CONFIG_TRIGGERS
	vnode_t triggered_dp = NULLVP;
	int retry_cnt = 0;
#define MAX_TRIGGER_RETRIES 1
#endif

	if (dp->v_type != VDIR || cnp->cn_flags & NOCROSSMOUNT) {
		return 0;
	}

	mounted_on_dp = dp;
#if CONFIG_TRIGGERS
restart:
#endif
	current_mount_generation = mount_generation;

	while (dp->v_mountedhere) {
		vnode_lock_spin(dp);
		if ((mp = dp->v_mountedhere)) {
			mp->mnt_crossref++;
			vnode_unlock(dp);
		} else {
			vnode_unlock(dp);
			break;
		}

		if (ISSET(mp->mnt_lflag, MNT_LFORCE)) {
			mount_dropcrossref(mp, dp, 0);
			break;                                         
		}


		if (vfs_busy(mp, vbusyflags)) {
			mount_dropcrossref(mp, dp, 0);
			if (vbusyflags == LK_NOWAIT) {
				error = ENOENT;
				goto out;
			}

			continue;
		}

		error = VFS_ROOT(mp, &tdp, ctx);

		mount_dropcrossref(mp, dp, 0);
		vfs_unbusy(mp);

		if (error) {
			goto out;
		}

		vnode_put(dp);
		ndp->ni_vp = dp = tdp;
		if (dp->v_type != VDIR) {
#if DEVELOPMENT || DEBUG
			panic("%s : Root of filesystem not a directory\n",
			    __FUNCTION__);
#else
			break;
#endif
		}
		depth++;
	}

#if CONFIG_TRIGGERS
	  
                                                                   
                                                                   
                                                                      
                                                                
                                                           
    
	if (dp->v_resolve && retry_cnt < MAX_TRIGGER_RETRIES) {
		error = vnode_trigger_resolve(dp, ndp, ctx);
		if (error) {
			goto out;
		}
		if (dp == triggered_dp) {
			retry_cnt += 1;
		} else {
			retry_cnt = 0;
		}
		triggered_dp = dp;
		goto restart;
	}
#endif                      

	if (depth) {
		mp = mounted_on_dp->v_mountedhere;

		if (mp) {
			mount_lock_spin(mp);
			mp->mnt_realrootvp_vid = dp->v_id;
			mp->mnt_realrootvp = dp;
			mp->mnt_generation = current_mount_generation;
			mount_unlock(mp);
		}
	}

	return 0;

out:
	return error;
}

  
                                                                             
                                                                                   
   
static int
lookup_handle_symlink(struct nameidata *ndp, vnode_t *new_dp, bool *new_dp_has_iocount, vfs_context_t ctx)
{
	int error;
	char *cp;                                                   
	uio_t auio;
	union {
		union {
			struct user_iovec s_uiovec;
			struct kern_iovec s_kiovec;
		} u_iovec;
		struct uio s_uio;
		char uio_buf[UIO_SIZEOF(1)];
	} u_uio_buf;                                                
	int need_newpathbuf;
	u_int linklen;
	struct componentname *cnp = &ndp->ni_cnd;
	vnode_t dp;
	char *tmppn;
	u_int rsrclen = (cnp->cn_flags & CN_WANTSRSRCFORK) ? sizeof(_PATH_RSRCFORKSPEC) : 0;
	bool dp_has_iocount = false;

	if (ndp->ni_loopcnt++ >= MAXSYMLINKS) {
		return ELOOP;
	}
#if CONFIG_MACF
	if ((error = mac_vnode_check_readlink(ctx, ndp->ni_vp)) != 0) {
		return error;
	}
#endif          
	if (ndp->ni_pathlen > 1 || !(cnp->cn_flags & HASBUF)) {
		need_newpathbuf = 1;
	} else {
		need_newpathbuf = 0;
	}

	if (need_newpathbuf) {
		cp = zalloc(ZV_NAMEI);
	} else {
		cp = cnp->cn_pnbuf;
	}
	auio = uio_createwithbuffer(1, 0, UIO_SYSSPACE, UIO_READ,
	    &u_uio_buf.uio_buf[0], sizeof(u_uio_buf.uio_buf));

	uio_addiov(auio, CAST_USER_ADDR_T(cp), MAXPATHLEN);

	error = VNOP_READLINK(ndp->ni_vp, auio, ctx);
	if (error) {
		if (need_newpathbuf) {
			zfree(ZV_NAMEI, cp);
		}
		return error;
	}

	  
                                                         
                                                       
                 
    
	linklen = MAXPATHLEN - (u_int)uio_resid(auio);
	if (linklen + ndp->ni_pathlen + rsrclen > MAXPATHLEN) {
		if (need_newpathbuf) {
			zfree(ZV_NAMEI, cp);
		}

		return ENAMETOOLONG;
	}
	if (need_newpathbuf) {
		tmppn = cnp->cn_pnbuf;
		bcopy(ndp->ni_next, cp + linklen, ndp->ni_pathlen);
		cnp->cn_pnbuf = cp;
		cnp->cn_pnlen = MAXPATHLEN;

		if ((cnp->cn_flags & HASBUF)) {
			zfree(ZV_NAMEI, tmppn);
		} else {
			cnp->cn_flags |= HASBUF;
		}
	} else {
		cnp->cn_pnbuf[linklen] = '\0';
	}

	ndp->ni_pathlen += linklen;
	cnp->cn_nameptr = cnp->cn_pnbuf;

	  
                                 
                      
    
	dp = ndp->ni_dvp;

	  
                                              
                                               
    
	vnode_put(ndp->ni_vp);
	ndp->ni_vp = NULLVP;
	ndp->ni_dvp = NULLVP;

	  
                                                  
    
	if (*(cnp->cn_nameptr) == '/') {
		vnode_put(dp);                                      
		while (*(cnp->cn_nameptr) == '/') {
			cnp->cn_nameptr++;
			ndp->ni_pathlen--;
		}
		if ((dp = ndp->ni_rootdir) == NULLVP) {
			return ENOENT;
		}
	} else {
		dp_has_iocount = true;
	}

	*new_dp = dp;
	*new_dp_has_iocount = dp_has_iocount;

	return 0;
}

  
                                          
                                         
   
int
relookup(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp)
{
	struct vnode *dp = NULL;                                                    
	int wantparent;                                                         
	int rdonly;                                                    
	int error = 0;
#ifdef NAMEI_DIAGNOSTIC
	int i, newhash;                                             
	char *cp;                                                      
#endif
	vfs_context_t ctx = cnp->cn_context;;

	  
                                              
    
	wantparent = cnp->cn_flags & (LOCKPARENT | WANTPARENT);
	rdonly = cnp->cn_flags & RDONLY;
	cnp->cn_flags &= ~ISSYMLINK;

	if (cnp->cn_flags & NOCACHE) {
		cnp->cn_flags &= ~MAKEENTRY;
	} else {
		cnp->cn_flags |= MAKEENTRY;
	}

	dp = dvp;

	  
                                            
                                                
                          
    
	if (cnp->cn_nameptr[0] == '\0') {
		if (cnp->cn_nameiop != LOOKUP || wantparent) {
			error = EISDIR;
			goto bad;
		}
		if (dp->v_type != VDIR) {
			error = ENOTDIR;
			goto bad;
		}
		if ((vnode_get(dp))) {
			error = ENOENT;
			goto bad;
		}
		*vpp = dp;

		if (cnp->cn_flags & SAVESTART) {
			panic("lookup: SAVESTART");
		}
		return 0;
	}
	  
                                                                        
    
	if ((error = VNOP_LOOKUP(dp, vpp, cnp, ctx))) {
		if (error != EJUSTRETURN) {
			goto bad;
		}
#if DIAGNOSTIC
		if (*vpp != NULL) {
			panic("leaf should be empty");
		}
#endif
		  
                                                          
                                 
     
		if (rdonly) {
			error = EROFS;
			goto bad;
		}
		  
                                                         
                                                      
                                                      
     
		return 0;
	}
	dp = *vpp;

#if DIAGNOSTIC
	  
                           
    
	if (dp->v_type == VLNK && (cnp->cn_flags & FOLLOW)) {
		panic("relookup: symlink found.\n");
	}
#endif

	  
                                                                
    
	if (rdonly &&
	    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
		error = EROFS;
		goto bad2;
	}
	                                     

	return 0;

bad2:
	vnode_put(dp);
bad:
	*vpp = NULL;

	return error;
}

  
                       
   
void
nameidone(struct nameidata *ndp)
{
	if (ndp->ni_cnd.cn_flags & HASBUF) {
		char *tmp = ndp->ni_cnd.cn_pnbuf;

		ndp->ni_cnd.cn_pnbuf = NULL;
		ndp->ni_cnd.cn_flags &= ~HASBUF;
		zfree(ZV_NAMEI, tmp);
	}
}


  
                                                                              
                                                                           
                                                                             
                                                                             
                                           
  
                                                                                
                                                                              
                                                                              
                                                
  
                                                                           
                                                                        
                                                                             
                                                                                
                                                                               
                                                                             
                                                             
  
                                                                               
                                                                  
   
#if (KDEBUG_LEVEL >= KDEBUG_LEVEL_IST)

void
kdebug_vfs_lookup(unsigned long *path_words, int path_len, void *vnp,
    uint32_t flags)
{
	bool noprocfilt = flags & KDBG_VFS_LOOKUP_FLAG_NOPROCFILT;

	assert(path_len >= 0);

	int code = ((flags & KDBG_VFS_LOOKUP_FLAG_LOOKUP) ? VFS_LOOKUP :
	    VFS_LOOKUP_DONE) | DBG_FUNC_START;

	if (path_len <= (3 * (int)sizeof(long))) {
		code |= DBG_FUNC_END;
	}

	if (noprocfilt) {
		KDBG_RELEASE_NOPROCFILT(code, kdebug_vnode(vnp), path_words[0],
		    path_words[1], path_words[2]);
	} else {
		KDBG_RELEASE(code, kdebug_vnode(vnp), path_words[0], path_words[1],
		    path_words[2]);
	}

	code &= ~DBG_FUNC_START;

	for (int i = 3; i * (int)sizeof(long) < path_len; i += 4) {
		if ((i + 4) * (int)sizeof(long) >= path_len) {
			code |= DBG_FUNC_END;
		}

		if (noprocfilt) {
			KDBG_RELEASE_NOPROCFILT(code, path_words[i], path_words[i + 1],
			    path_words[i + 2], path_words[i + 3]);
		} else {
			KDBG_RELEASE(code, path_words[i], path_words[i + 1],
			    path_words[i + 2], path_words[i + 3]);
		}
	}
}

void
kdebug_lookup_gen_events(long *path_words, int path_len, void *vnp, bool lookup)
{
	assert(path_len >= 0);
	kdebug_vfs_lookup((unsigned long *)path_words, path_len, vnp,
	    lookup ? KDBG_VFS_LOOKUP_FLAG_LOOKUP : 0);
}

void
kdebug_lookup(vnode_t vnp, struct componentname *cnp)
{
	unsigned long path_words[NUMPARMS];

	  
                                                                  
    
	char *path_end = cnp->cn_nameptr + cnp->cn_namelen;
	size_t path_len = MIN(path_end - cnp->cn_pnbuf,
	    (ssize_t)sizeof(path_words));
	assert(path_len >= 0);
	char *path_trunc = path_end - path_len;

	memcpy(path_words, path_trunc, path_len);

	  
                         
    
	if (path_len < (ssize_t)sizeof(path_words)) {
		bool complete_str = *(cnp->cn_nameptr + cnp->cn_namelen) == '\0';
		memset((char *)path_words + path_len, complete_str ? '\0' : '>',
		    sizeof(path_words) - path_len);
	}
	kdebug_vfs_lookup(path_words, (int)path_len, vnp, KDBG_VFS_LOOKUP_FLAG_LOOKUP);
}

#else                                         

void
kdebug_vfs_lookup(long *dbg_parms __unused, int dbg_namelen __unused,
    void *dp __unused, __unused uint32_t flags)
{
}

static void
kdebug_lookup(struct vnode *dp __unused, struct componentname *cnp __unused)
{
}
#endif                                         

int
vfs_getbyid(fsid_t *fsid, ino64_t ino, vnode_t *vpp, vfs_context_t ctx)
{
	mount_t mp;
	int error;

	mp = mount_lookupby_volfsid(fsid->val[0], 1);
	if (mp == NULL) {
		return EINVAL;
	}

	                           
	if (ino == 2) {
		error = VFS_ROOT(mp, vpp, ctx);
	} else {
		error = VFS_VGET(mp, ino, vpp, ctx);
	}

	vfs_unbusy(mp);
	return error;
}
  
                                                       
  
                               
  
          
          
               
                      
  
         
                                          
                                                       
                              
                                  
   
#if CONFIG_VOLFS
static int
vfs_getrealpath(const char * path, char * realpath, size_t bufsize, vfs_context_t ctx)
{
	vnode_t vp;
	struct mount *mp = NULL;
	char  *str;
	char ch;
	unsigned long id;
	ino64_t ino;
	int error;
	int length;

	                                                        
	id = strtoul(path, &str, 10);
	if (id == 0 || str[0] != '/') {
		return EINVAL;
	}
	while (*str == '/') {
		str++;
	}
	ch = *str;

	if (id > INT_MAX) {
		return ENOENT;
	}
	mp = mount_lookupby_volfsid((int)id, 1);
	if (mp == NULL) {
		return EINVAL;                          
	}
	                                               
	if (ch == '@' && str[1] == '\0') {
		ino = 2;
		str++;
	} else {
		                                                 
		ino = strtouq(str, &str, 10);
	}

	                           
	if (ino == 2) {
		struct vfs_attr vfsattr;
		int use_vfs_root = TRUE;

		VFSATTR_INIT(&vfsattr);
		VFSATTR_WANTED(&vfsattr, f_capabilities);
		if (vfs_getattr(mp, &vfsattr, vfs_context_kernel()) == 0 &&
		    VFSATTR_IS_SUPPORTED(&vfsattr, f_capabilities)) {
			if ((vfsattr.f_capabilities.capabilities[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_VOL_GROUPS) &&
			    (vfsattr.f_capabilities.valid[VOL_CAPABILITIES_FORMAT] & VOL_CAP_FMT_VOL_GROUPS)) {
				use_vfs_root = FALSE;
			}
		}

		if (use_vfs_root) {
			error = VFS_ROOT(mp, &vp, ctx);
		} else {
			error = VFS_VGET(mp, ino, &vp, ctx);
		}
	} else {
		error = VFS_VGET(mp, ino, &vp, ctx);
	}
	vfs_unbusy(mp);
	if (error) {
		goto out;
	}
	realpath[0] = '\0';

	                                          
	error = build_path(vp, realpath, (int)bufsize, &length, 0, ctx);
	vnode_put(vp);

	if (error == 0 && *str != '\0') {
		size_t attempt = strlcat(realpath, str, MAXPATHLEN);
		if (attempt > MAXPATHLEN) {
			error = ENAMETOOLONG;
		}
	}
out:
	return error;
}
#endif

void
lookup_compound_vnop_post_hook(int error, vnode_t dvp, vnode_t vp, struct nameidata *ndp, int did_create)
{
	if (error == 0 && vp == NULLVP) {
		panic("NULL vp with error == 0.\n");
	}

	  
                                                                      
                                                                                 
                               
    
	if ((ndp->ni_flag & NAMEI_COMPOUND_OP_MASK) == 0) {
		return;
	}

	  
                                                       
                                            
    
	if (error == EKEEPLOOKING) {
		return;
	}

	  
                                                                
                                                       
    
	if ((vp != NULLVP) && !did_create) {
		  
                                                                       
                                                              
     
		if ((error != 0) || (ndp->ni_cnd.cn_flags & MAKEENTRY)) {
			lookup_consider_update_cache(dvp, vp, &ndp->ni_cnd, ndp->ni_ncgeneration);
		}
		if (ndp->ni_cnd.cn_flags & AUDITVNPATH1) {
			AUDIT_ARG(vnpath, vp, ARG_VNODE1);
		} else if (ndp->ni_cnd.cn_flags & AUDITVNPATH2) {
			AUDIT_ARG(vnpath, vp, ARG_VNODE2);
		}
	}

	  
                                                                       
                                                                                    
                                         
   
                          
                                                                        
                                                              
    
	if (kdebug_enable) {
		kdebug_lookup(vp ? vp : dvp, &ndp->ni_cnd);
	}
}
