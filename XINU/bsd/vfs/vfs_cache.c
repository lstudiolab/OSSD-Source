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
 * Copyright (c) 1989, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Poul-Henning Kamp of the FreeBSD Project.
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
 *
 *	@(#)vfs_cache.c	8.5 (Berkeley) 3/22/95
 */
  
                                                                      
                                                                          
                                                                        
               
   
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/mount_internal.h>
#include <sys/vnode_internal.h>
#include <miscfs/specfs/specdev.h>
#include <sys/namei.h>
#include <sys/errno.h>
#include <kern/kalloc.h>
#include <sys/kauth.h>
#include <sys/user.h>
#include <sys/paths.h>
#include <os/overflow.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

  
                                 
  
                                                         
                                                          
                                                               
                                                            
                   
  
                                                                     
                                         
  
                                                             
                                                      
                                                    
   

  
                                            
   

ZONE_DECLARE(namecache_zone, "namecache", sizeof(struct namecache), ZC_NONE);

LIST_HEAD(nchashhead, namecache) * nchashtbl;                    
u_long  nchashmask;
u_long  nchash;                                                     
long    numcache;                                                              
int     desiredNodes;
int     desiredNegNodes;
int     ncs_negtotal;
int     nc_disabled = 0;
TAILQ_HEAD(, namecache) nchead;                                              
TAILQ_HEAD(, namecache) neghead;                                                  


#if COLLECT_STATS

struct  nchstats nchstats;                                                  

#define NCHSTAT(v) {            \
	nchstats.v++;           \
}
#define NAME_CACHE_LOCK()               name_cache_lock()
#define NAME_CACHE_UNLOCK()             name_cache_unlock()
#define NAME_CACHE_LOCK_SHARED()        name_cache_lock()

#else

#define NCHSTAT(v)
#define NAME_CACHE_LOCK()               name_cache_lock()
#define NAME_CACHE_UNLOCK()             name_cache_unlock()
#define NAME_CACHE_LOCK_SHARED()        name_cache_lock_shared()

#endif


                                   
static LCK_GRP_DECLARE(namecache_lck_grp, "Name Cache");
static LCK_RW_DECLARE(namecache_rw_lock, &namecache_lck_grp);

static LCK_GRP_DECLARE(strcache_lck_grp, "String Cache");
static LCK_ATTR_DECLARE(strcache_lck_attr, 0, 0);
LCK_RW_DECLARE_ATTR(strtable_rw_lock, &strcache_lck_grp, &strcache_lck_attr);

static LCK_GRP_DECLARE(rootvnode_lck_grp, "rootvnode");
LCK_RW_DECLARE(rootvnode_rw_lock, &rootvnode_lck_grp);

#define NUM_STRCACHE_LOCKS 1024

lck_mtx_t strcache_mtx_locks[NUM_STRCACHE_LOCKS];


static vnode_t cache_lookup_locked(vnode_t dvp, struct componentname *cnp);
static const char *add_name_internal(const char *, uint32_t, u_int, boolean_t, u_int);
static void init_string_table(void);
static void cache_delete(struct namecache *, int);
static void cache_enter_locked(vnode_t dvp, vnode_t vp, struct componentname *cnp, const char *strname);
static void cache_purge_locked(vnode_t vp, kauth_cred_t *credp);

#ifdef DUMP_STRING_TABLE
  
                                            
   
void dump_string_table(void);
#endif                         

static void init_crc32(void);
static unsigned int crc32tab[256];


#define NCHHASH(dvp, hash_val) \
	(&nchashtbl[(dvp->v_id ^ (hash_val)) & nchashmask])

  
                                                                          
                                                                           
                                                               
  
                                                                    
                                                                         
                                                                             
                                              
  
                                                                               
                                                                              
                       
  
                                                                          
                                                                     
                                                                        
           
  
                                                               
  
                                                                   
                
                       
                                   
            
                                         
           
                     
                                                             
            
                                                         
    
  
   
static boolean_t
cache_check_vnode_issubdir(vnode_t vp, vnode_t dvp, boolean_t *is_subdir,
    vnode_t *next_vp)
{
	vnode_t tvp = vp;
	int defer = FALSE;

	*is_subdir = FALSE;
	*next_vp = NULLVP;
	while (1) {
		mount_t tmp;

		if (tvp == dvp) {
			*is_subdir = TRUE;
			break;
		} else if (tvp == rootvnode) {
			                        
			break;
		}

		tmp = tvp->v_mount;
		while ((tvp->v_flag & VROOT) && tmp && tmp->mnt_vnodecovered &&
		    tvp != dvp && tvp != rootvnode) {
			tvp = tmp->mnt_vnodecovered;
			tmp = tvp->v_mount;
		}

		  
                                                     
                                            
     
		if (tvp == dvp || tvp == rootvnode) {
			                        
			break;
		}

		if (!tmp) {
			defer = TRUE;
			*next_vp = NULLVP;
			break;
		}

		if ((tvp->v_flag & VISHARDLINK) || !(tvp->v_parent)) {
			defer = TRUE;
			*next_vp = tvp;
			break;
		}

		tvp = tvp->v_parent;
	}

	return defer;
}

                                                                             
#define MAX_ERROR_RETRY 3

  
                                                                           
                                                                     
                                                                       
                  
  
                                                                         
  
                                                                         
                                                                                
   
int
vnode_issubdir(vnode_t vp, vnode_t dvp, int *is_subdir, vfs_context_t ctx)
{
	vnode_t start_vp, tvp;
	vnode_t vp_with_iocount;
	int error = 0;
	char dotdotbuf[] = "..";
	int error_retry_count = 0;                                         
                                         

	*is_subdir = FALSE;
	tvp = start_vp = vp;
	  
                                                                     
                                                   
    
	vp_with_iocount = NULLVP;

	while (1) {
		boolean_t defer;
		vnode_t pvp;
		uint32_t vid;
		struct componentname cn;
		boolean_t is_subdir_locked = FALSE;

		if (tvp == dvp) {
			*is_subdir = TRUE;
			break;
		} else if (tvp == rootvnode) {
			                        
			break;
		}

		NAME_CACHE_LOCK_SHARED();

		defer = cache_check_vnode_issubdir(tvp, dvp, &is_subdir_locked,
		    &tvp);

		if (defer && tvp) {
			vid = vnode_vid(tvp);
		}

		NAME_CACHE_UNLOCK();

		if (!defer) {
			*is_subdir = is_subdir_locked;
			break;
		}

		if (!tvp) {
			if (error_retry_count++ < MAX_ERROR_RETRY) {
				tvp = vp;
				continue;
			}
			error = ENOENT;
			break;
		}

		if (tvp != start_vp) {
			if (vp_with_iocount) {
				vnode_put(vp_with_iocount);
				vp_with_iocount = NULLVP;
			}

			error = vnode_getwithvid(tvp, vid);
			if (error) {
				if (error_retry_count++ < MAX_ERROR_RETRY) {
					tvp = vp;
					error = 0;
					continue;
				}
				break;
			}

			vp_with_iocount = tvp;
		}

		bzero(&cn, sizeof(cn));
		cn.cn_nameiop = LOOKUP;
		cn.cn_flags = ISLASTCN | ISDOTDOT;
		cn.cn_context = ctx;
		cn.cn_pnbuf = &dotdotbuf[0];
		cn.cn_pnlen = sizeof(dotdotbuf);
		cn.cn_nameptr = cn.cn_pnbuf;
		cn.cn_namelen = 2;

		pvp = NULLVP;
		if ((error = VNOP_LOOKUP(tvp, &pvp, &cn, ctx))) {
			break;
		}

		if (!(tvp->v_flag & VISHARDLINK) && tvp->v_parent != pvp) {
			(void)vnode_update_identity(tvp, pvp, NULL, 0, 0,
			    VNODE_UPDATE_PARENT);
		}

		if (vp_with_iocount) {
			vnode_put(vp_with_iocount);
		}

		vp_with_iocount = tvp = pvp;
	}

	if (vp_with_iocount) {
		vnode_put(vp_with_iocount);
	}

	return error;
}

  
                                                                   
                                                                 
                                                                   
                                                                 
                                                                      
                         
  
                                                                 
                                                              
                                                                        
                                                        
  
                                                                  
                                                                     
                                                                 
                                                                   
  
                                                               
                                                           
                                                    
  
                                                    
  
                                                                   
                                                                   
                                                                     
                                                                      
                                                                     
                                                                    
                                                                     
                                                        
  
   
int
build_path_with_parent(vnode_t first_vp, vnode_t parent_vp, char *buff, int buflen,
    int *outlen, size_t *mntpt_outlen, int flags, vfs_context_t ctx)
{
	vnode_t vp, tvp;
	vnode_t vp_with_iocount;
	vnode_t proc_root_dir_vp;
	char *end;
	char *mntpt_end;
	const char *str;
	unsigned int  len;
	int  ret = 0;
	int  fixhardlink;

	if (first_vp == NULLVP) {
		return EINVAL;
	}

	if (buflen <= 1) {
		return ENOSPC;
	}

	  
                                                   
    
	if (vfs_context_proc(ctx)->p_fd && !(flags & BUILDPATH_NO_PROCROOT)) {
		proc_root_dir_vp = vfs_context_proc(ctx)->p_fd->fd_rdir;
	} else {
		proc_root_dir_vp = NULL;
	}

	vp_with_iocount = NULLVP;
again:
	vp = first_vp;

	end = &buff[buflen - 1];
	*end = '\0';
	mntpt_end = NULL;

	  
                                                                           
                                                                
                                     
    
	if (proc_root_dir_vp == first_vp) {
		*--end = '/';
		goto out;
	}

	  
                                                 
                                                       
                                                 
   
                                                             
                                                          
                                                            
                                                       
                                                             
                                                            
                                                          
                                                        
                                                             
    
	NAME_CACHE_LOCK_SHARED();

#if CONFIG_FIRMLINKS
	if (!(flags & BUILDPATH_NO_FIRMLINK) &&
	    (vp->v_flag & VFMLINKTARGET) && vp->v_fmlink) {
		vp = vp->v_fmlink;
	}
#endif

	  
                                               
    
	while (vp && vp->v_flag & VROOT) {
		if (vp->v_mount == NULL) {
			ret = EINVAL;
			goto out_unlock;
		}
		if ((vp->v_mount->mnt_flag & MNT_ROOTFS) || (vp == proc_root_dir_vp)) {
			  
                                                    
               
      
			*--end = '/';

			goto out_unlock;
		} else {
			  
                                                         
                                                        
                               
      
#if CONFIG_FIRMLINKS
			if (!(flags & BUILDPATH_NO_FIRMLINK) &&
			    (vp->v_flag & VFMLINKTARGET) && vp->v_fmlink) {
				vp = vp->v_fmlink;
			} else
#endif
			if (flags & BUILDPATH_VOLUME_RELATIVE) {
				*--end = '/';
				goto out_unlock;
			} else {
				vp = vp->v_mount->mnt_vnodecovered;
				if (!mntpt_end && vp) {
					mntpt_end = end;
				}
			}
		}
	}

	while ((vp != NULLVP) && (vp->v_parent != vp)) {
		int  vid;

		  
                                                        
                                                        
                             
     
		fixhardlink = (vp->v_flag & VISHARDLINK) &&
		    (vp->v_mount->mnt_kern_flag & MNTK_PATH_FROM_ID) &&
		    !(flags & BUILDPATH_NO_FS_ENTER);

		if (!fixhardlink) {
			str = vp->v_name;

			if (str == NULL || *str == '\0') {
				if (vp->v_parent != NULL) {
					ret = EINVAL;
				} else {
					ret = ENOENT;
				}
				goto out_unlock;
			}
			len = (unsigned int)strlen(str);
			  
                                                                   
      
			if ((unsigned int)(end - buff) < (len + 1)) {
				ret = ENOSPC;
				goto out_unlock;
			}
			  
                              
      
			str += len;

			for (; len > 0; len--) {
				*--end = *--str;
			}
			  
                           
      
			*--end = '/';
		}

		  
                              
     
		if (((vp->v_parent != NULLVP) && !fixhardlink) ||
		    (flags & BUILDPATH_NO_FS_ENTER)) {
			  
                                                                    
                                                              
                                                       
                                                    
                                            
      
			if ((vp->v_parent == NULLVP) && (rootvnode != vp)) {
				  
                                                
                                                  
                                                  
       
				ret = ENOENT;

				                                                          
			}
			vp = vp->v_parent;

			  
                                                           
                                                            
                                                         
                            
      
			if (vp && !vnode_isdir(vp) && vp->v_parent) {
				vp = vp->v_parent;
			}
		} else {
			  
                                        
      
			struct vnode_attr  va;
			vnode_t  dvp;

			  
                                                              
      
			if (!(vp->v_mount->mnt_kern_flag & MNTK_PATH_FROM_ID)) {
				ret = ENOENT;
				goto out_unlock;
			}
			vid = vp->v_id;

			NAME_CACHE_UNLOCK();

			if (vp != first_vp && vp != parent_vp && vp != vp_with_iocount) {
				if (vp_with_iocount) {
					vnode_put(vp_with_iocount);
					vp_with_iocount = NULLVP;
				}
				if (vnode_getwithvid(vp, vid)) {
					goto again;
				}
				vp_with_iocount = vp;
			}
			VATTR_INIT(&va);
			VATTR_WANTED(&va, va_parentid);

			if (fixhardlink) {
				VATTR_WANTED(&va, va_name);
				va.va_name = zalloc(ZV_NAMEI);
			} else {
				va.va_name = NULL;
			}
			  
                                                                        
      
			ret = vnode_getattr(vp, &va, ctx);

			if (fixhardlink) {
				if ((ret == 0) && (VATTR_IS_SUPPORTED(&va, va_name))) {
					str = va.va_name;
					vnode_update_identity(vp, NULL, str, (unsigned int)strlen(str), 0, VNODE_UPDATE_NAME);
				} else if (vp->v_name) {
					str = vp->v_name;
					ret = 0;
				} else {
					ret = ENOENT;
					goto bad_news;
				}
				len = (unsigned int)strlen(str);

				  
                                       
       
				if ((unsigned int)(end - buff) < (len + 1)) {
					ret = ENOSPC;
				} else {
					                              
					str += len;

					for (; len > 0; len--) {
						*--end = *--str;
					}
					  
                             
        
					*--end = '/';
				}
bad_news:
				zfree(ZV_NAMEI, va.va_name);
			}
			if (ret || !VATTR_IS_SUPPORTED(&va, va_parentid)) {
				ret = ENOENT;
				goto out;
			}
			  
                                               
      
			if ((ret = VFS_VGET(vp->v_mount, (ino64_t)va.va_parentid, &dvp, ctx))) {
				goto out;
			}

			if (!fixhardlink && (vp->v_parent != dvp)) {
				vnode_update_identity(vp, dvp, NULL, 0, 0, VNODE_UPDATE_PARENT);
			}

			if (vp_with_iocount) {
				vnode_put(vp_with_iocount);
			}
			vp = dvp;
			vp_with_iocount = vp;

			NAME_CACHE_LOCK_SHARED();

			  
                                                           
                                                            
                                                         
                            
      
			if (vp && !vnode_isdir(vp) && vp->v_parent) {
				vp = vp->v_parent;
			}
		}

		if (vp && (flags & BUILDPATH_CHECKACCESS)) {
			vid = vp->v_id;

			NAME_CACHE_UNLOCK();

			if (vp != first_vp && vp != parent_vp && vp != vp_with_iocount) {
				if (vp_with_iocount) {
					vnode_put(vp_with_iocount);
					vp_with_iocount = NULLVP;
				}
				if (vnode_getwithvid(vp, vid)) {
					goto again;
				}
				vp_with_iocount = vp;
			}
			if ((ret = vnode_authorize(vp, NULL, KAUTH_VNODE_SEARCH, ctx))) {
				goto out;                       
			}
			NAME_CACHE_LOCK_SHARED();
		}

		  
                                                 
                                               
                                             
                 
     
		tvp = vp;

		while (tvp) {
			if (tvp == proc_root_dir_vp) {
				goto out_unlock;                                  
			}

#if CONFIG_FIRMLINKS
			if (!(flags & BUILDPATH_NO_FIRMLINK) &&
			    (tvp->v_flag & VFMLINKTARGET) && tvp->v_fmlink) {
				tvp = tvp->v_fmlink;
				break;
			}
#endif

			if (!(tvp->v_flag & VROOT) || !tvp->v_mount) {
				break;                                                    
			}
			if (flags & BUILDPATH_VOLUME_RELATIVE) {
				                                    
				tvp = NULL;
			} else {
				tvp = tvp->v_mount->mnt_vnodecovered;
				if (!mntpt_end && tvp) {
					mntpt_end = end;
				}
			}
		}
		if (tvp == NULLVP) {
			goto out_unlock;
		}
		vp = tvp;
	}
out_unlock:
	NAME_CACHE_UNLOCK();
out:
	if (vp_with_iocount) {
		vnode_put(vp_with_iocount);
	}
	  
                                                       
    
	memmove(buff, end, &buff[buflen] - end);

	  
                                          
    
	*outlen = (int)(&buff[buflen] - end);
	if (mntpt_outlen && mntpt_end) {
		*mntpt_outlen = (size_t)*outlen - (size_t)(&buff[buflen] - mntpt_end);
	}

	                                                           
                                                          
                                                              
    
	if ((ret == ENOENT) && (flags & BUILDPATH_CHECK_MOVED)) {
		ret = EAGAIN;
	}

	return ret;
}

int
build_path(vnode_t first_vp, char *buff, int buflen, int *outlen, int flags, vfs_context_t ctx)
{
	return build_path_with_parent(first_vp, NULL, buff, buflen, outlen, NULL, flags, ctx);
}

  
                                       
                                         
                               
   
vnode_t
vnode_getparent(vnode_t vp)
{
	vnode_t pvp = NULLVP;
	int     pvid;

	NAME_CACHE_LOCK_SHARED();

	pvp = vp->v_parent;

	  
                                                 
                                                   
                                                
                                                             
                                                   
    
	if (pvp != NULLVP) {
		pvid = pvp->v_id;

		NAME_CACHE_UNLOCK();

		if (vnode_getwithvid(pvp, pvid) != 0) {
			pvp = NULL;
		}
	} else {
		NAME_CACHE_UNLOCK();
	}
	return pvp;
}

const char *
vnode_getname(vnode_t vp)
{
	const char *name = NULL;

	NAME_CACHE_LOCK_SHARED();

	if (vp->v_name) {
		name = vfs_addname(vp->v_name, (unsigned int)strlen(vp->v_name), 0, 0);
	}
	NAME_CACHE_UNLOCK();

	return name;
}

void
vnode_putname(const char *name)
{
	vfs_removename(name);
}

static const char unknown_vnodename[] = "(unknown vnode name)";

const char *
vnode_getname_printable(vnode_t vp)
{
	const char *name = vnode_getname(vp);
	if (name != NULL) {
		return name;
	}

	switch (vp->v_type) {
	case VCHR:
	case VBLK:
	{
		  
                                       
                                  
     
		char dev_name[64];
		(void) snprintf(dev_name, sizeof(dev_name),
		    "%c(%u, %u)", VCHR == vp->v_type ? 'c':'b',
		    major(vp->v_rdev), minor(vp->v_rdev));
		  
                                               
                                         
                                                  
                    
     
		NAME_CACHE_LOCK_SHARED();
		name = vfs_addname(dev_name, (unsigned int)strlen(dev_name), 0, 0);
		NAME_CACHE_UNLOCK();
		return name;
	}
	default:
		return unknown_vnodename;
	}
}

void
vnode_putname_printable(const char *name)
{
	if (name == unknown_vnodename) {
		return;
	}
	vnode_putname(name);
}


  
                                          
                                          
                                                 
                                        
  
                        
                                                                       
                                                                     
                                                             
  
                                                                         
   
void
vnode_update_identity(vnode_t vp, vnode_t dvp, const char *name, int name_len, uint32_t name_hashval, int flags)
{
	struct  namecache *ncp;
	vnode_t old_parentvp = NULLVP;
	int isstream = (vp->v_flag & VISNAMEDSTREAM);
	int kusecountbumped = 0;
	kauth_cred_t tcred = NULL;
	const char *vname = NULL;
	const char *tname = NULL;

	if (name_len < 0) {
		return;
	}

	if (flags & VNODE_UPDATE_PARENT) {
		if (dvp && vnode_ref(dvp) != 0) {
			dvp = NULLVP;
		}
		                                                       
		if (isstream && dvp && (dvp != vp) && (dvp != vp->v_parent) && (dvp->v_type == VREG)) {
			vnode_lock_spin(dvp);
			++dvp->v_kusecount;
			kusecountbumped = 1;
			vnode_unlock(dvp);
		}
	} else {
		dvp = NULLVP;
	}
	if ((flags & VNODE_UPDATE_NAME)) {
		if (name != vp->v_name) {
			if (name && *name) {
				if (name_len == 0) {
					name_len = (int)strlen(name);
				}
				tname = vfs_addname(name, name_len, name_hashval, 0);
			}
		} else {
			flags &= ~VNODE_UPDATE_NAME;
		}
	}
	if ((flags & (VNODE_UPDATE_PURGE | VNODE_UPDATE_PARENT | VNODE_UPDATE_CACHE | VNODE_UPDATE_NAME | VNODE_UPDATE_PURGEFIRMLINK))) {
		NAME_CACHE_LOCK();

#if CONFIG_FIRMLINKS
		if (flags & VNODE_UPDATE_PURGEFIRMLINK) {
			vnode_t old_fvp = vp->v_fmlink;
			if (old_fvp) {
				vnode_lock_spin(vp);
				vp->v_flag &= ~VFMLINKTARGET;
				vp->v_fmlink = NULLVP;
				vnode_unlock(vp);
				NAME_CACHE_UNLOCK();

				  
                                                   
                                                    
                                                 
                                                 
                  
       
				vnode_recycle(old_fvp);
				vnode_rele_internal(old_fvp, O_EVTONLY, 1, 0);

				NAME_CACHE_LOCK();
			}
		}
#endif

		if ((flags & VNODE_UPDATE_PURGE)) {
			if (vp->v_parent) {
				vp->v_parent->v_nc_generation++;
			}

			while ((ncp = LIST_FIRST(&vp->v_nclinks))) {
				cache_delete(ncp, 1);
			}

			while ((ncp = TAILQ_FIRST(&vp->v_ncchildren))) {
				cache_delete(ncp, 1);
			}

			  
                                                                                   
      
			tcred = vp->v_cred;
			vp->v_cred = NOCRED;
			vp->v_authorized_actions = 0;
			vp->v_cred_timestamp = 0;
		}
		if ((flags & VNODE_UPDATE_NAME)) {
			vname = vp->v_name;
			vp->v_name = tname;
		}
		if (flags & VNODE_UPDATE_PARENT) {
			if (dvp != vp && dvp != vp->v_parent) {
				old_parentvp = vp->v_parent;
				vp->v_parent = dvp;
				dvp = NULLVP;

				if (old_parentvp) {
					flags |= VNODE_UPDATE_CACHE;
				}
			}
		}
		if (flags & VNODE_UPDATE_CACHE) {
			while ((ncp = LIST_FIRST(&vp->v_nclinks))) {
				cache_delete(ncp, 1);
			}
		}
		NAME_CACHE_UNLOCK();

		if (vname != NULL) {
			vfs_removename(vname);
		}

		if (IS_VALID_CRED(tcred)) {
			kauth_cred_unref(&tcred);
		}
	}
	if (dvp != NULLVP) {
		                                                                  
		if (kusecountbumped) {
			vnode_lock_spin(dvp);
			if (dvp->v_kusecount > 0) {
				--dvp->v_kusecount;
			}
			vnode_unlock(dvp);
		}
		vnode_rele(dvp);
	}
	if (old_parentvp) {
		struct  uthread *ut;

		if (isstream) {
			vnode_lock_spin(old_parentvp);
			if ((old_parentvp->v_type != VDIR) && (old_parentvp->v_kusecount > 0)) {
				--old_parentvp->v_kusecount;
			}
			vnode_unlock(old_parentvp);
		}
		ut = get_bsdthread_info(current_thread());

		  
                                                   
                                                  
                                                
                                                   
                                          
     
		ut->uu_defer_reclaims = 1;
		ut->uu_vreclaims = NULLVP;

		while ((vp = old_parentvp) != NULLVP) {
			vnode_lock_spin(vp);
			vnode_rele_internal(vp, 0, 0, 1);

			  
                                                   
                                                             
                                                         
                                                        
                                                         
                                                          
                                                   
                                              
                                                      
     
                                              
                                                        
                               
                                                          
                                                     
                                                 
      
			if ((vp->v_iocount == 1) && (vp->v_usecount == 0) &&
			    ((vp->v_lflag & (VL_MARKTERM | VL_TERMINATE | VL_DEAD)) == VL_MARKTERM)) {
				  
                                                            
                                                                 
                                                         
                                                               
                                               
      
                                                            
                                                        
                                                               
       
				vnode_lock_convert(vp);

				NAME_CACHE_LOCK();
				old_parentvp = vp->v_parent;
				vp->v_parent = NULLVP;
				NAME_CACHE_UNLOCK();
			} else {
				  
                                                   
                       
       
				old_parentvp = NULLVP;
			}
			vnode_unlock(vp);
		}
		ut->uu_defer_reclaims = 0;

		while ((vp = ut->uu_vreclaims) != NULLVP) {
			ut->uu_vreclaims = vp->v_defer_reclaimlist;

			  
                                               
                                                   
      
			vnode_put(vp);
		}
	}
}

#if CONFIG_FIRMLINKS
errno_t
vnode_setasfirmlink(vnode_t vp, vnode_t target_vp)
{
	int error = 0;
	vnode_t old_target_vp = NULLVP;
	vnode_t old_target_vp_v_fmlink = NULLVP;
	kauth_cred_t target_vp_cred = NULL;
	kauth_cred_t old_target_vp_cred = NULL;

	if (!vp) {
		return EINVAL;
	}

	if (target_vp) {
		if (vp->v_fmlink == target_vp) {                                                      
			return 0;
		}

		  
                                                         
                                 
     
		if ((error = vnode_ref_ext(target_vp, O_EVTONLY, VNODE_REF_FORCE))) {
			return error;
		}

		if ((error = vnode_ref_ext(vp, O_EVTONLY, VNODE_REF_FORCE))) {
			vnode_rele_ext(target_vp, O_EVTONLY, 1);
			return error;
		}
	}

	NAME_CACHE_LOCK();

	old_target_vp = vp->v_fmlink;
	if (target_vp && (target_vp == old_target_vp)) {
		NAME_CACHE_UNLOCK();
		return 0;
	}
	vp->v_fmlink = target_vp;

	vnode_lock_spin(vp);
	vp->v_flag &= ~VFMLINKTARGET;
	vnode_unlock(vp);

	if (target_vp) {
		target_vp->v_fmlink = vp;
		vnode_lock_spin(target_vp);
		target_vp->v_flag |= VFMLINKTARGET;
		vnode_unlock(target_vp);
		cache_purge_locked(vp, &target_vp_cred);
	}

	if (old_target_vp) {
		old_target_vp_v_fmlink = old_target_vp->v_fmlink;
		old_target_vp->v_fmlink = NULLVP;
		vnode_lock_spin(old_target_vp);
		old_target_vp->v_flag &= ~VFMLINKTARGET;
		vnode_unlock(old_target_vp);
		cache_purge_locked(vp, &old_target_vp_cred);
	}

	NAME_CACHE_UNLOCK();

	if (target_vp_cred && IS_VALID_CRED(target_vp_cred)) {
		kauth_cred_unref(&target_vp_cred);
	}

	if (old_target_vp) {
		if (old_target_vp_cred && IS_VALID_CRED(old_target_vp_cred)) {
			kauth_cred_unref(&old_target_vp_cred);
		}

		vnode_rele_ext(old_target_vp, O_EVTONLY, 1);
		if (old_target_vp_v_fmlink) {
			vnode_rele_ext(old_target_vp_v_fmlink, O_EVTONLY, 1);
		}
	}

	return 0;
}

errno_t
vnode_getfirmlink(vnode_t vp, vnode_t *target_vp)
{
	int error;

	if (!vp->v_fmlink) {
		return ENODEV;
	}

	NAME_CACHE_LOCK_SHARED();
	if (vp->v_fmlink && !(vp->v_flag & VFMLINKTARGET) &&
	    (vnode_get(vp->v_fmlink) == 0)) {
		vnode_t tvp = vp->v_fmlink;

		vnode_lock_spin(tvp);
		if (tvp->v_lflag & (VL_TERMINATE | VL_DEAD)) {
			vnode_unlock(tvp);
			NAME_CACHE_UNLOCK();
			vnode_put(tvp);
			return ENOENT;
		}
		if (!(tvp->v_flag & VFMLINKTARGET)) {
			panic("firmlink target for vnode %p does not have flag set", vp);
		}
		vnode_unlock(tvp);
		*target_vp = tvp;
		error = 0;
	} else {
		*target_vp = NULLVP;
		error = ENODEV;
	}
	NAME_CACHE_UNLOCK();
	return error;
}

#else                       

errno_t
vnode_setasfirmlink(__unused vnode_t vp, __unused vnode_t src_vp)
{
	return ENOTSUP;
}

errno_t
vnode_getfirmlink(__unused vnode_t vp, __unused vnode_t *target_vp)
{
	return ENOTSUP;
}

#endif

  
                                                                     
                                                                    
                                
  
                                                                     
                                                                  
                                                                    
   
void
vnode_setmultipath(vnode_t vp)
{
	vnode_lock_spin(vp);

	  
                                                                
                                                                    
                                                                  
                                                           
   
                                                                 
                                                                 
                               
    

	vp->v_flag |= VISHARDLINK;

	vnode_unlock(vp);
}



  
                          
   
void
vnode_uncache_credentials(vnode_t vp)
{
	vnode_uncache_authorized_action(vp, KAUTH_INVALIDATE_CACHED_RIGHTS);
}


  
                                                                         
                                                                                
                                                                              
                                                                                     
                                                                                    
                                                                                  
                                                                                
                                                     
   

void
vnode_uncache_authorized_action(vnode_t vp, kauth_action_t action)
{
	kauth_cred_t tcred = NOCRED;

	NAME_CACHE_LOCK();

	vp->v_authorized_actions &= ~action;

	if (action == KAUTH_INVALIDATE_CACHED_RIGHTS &&
	    IS_VALID_CRED(vp->v_cred)) {
		  
                                                                                  
     
		tcred = vp->v_cred;
		vp->v_cred = NOCRED;
	}
	NAME_CACHE_UNLOCK();

	if (tcred != NOCRED) {
		kauth_cred_unref(&tcred);
	}
}


extern int bootarg_vnode_cache_defeat;                                    

boolean_t
vnode_cache_is_authorized(vnode_t vp, vfs_context_t ctx, kauth_action_t action)
{
	kauth_cred_t    ucred;
	boolean_t       retval = FALSE;

	                                            
	if (bootarg_vnode_cache_defeat) {
		return FALSE;
	}

	if ((vp->v_mount->mnt_kern_flag & (MNTK_AUTH_OPAQUE | MNTK_AUTH_CACHE_TTL))) {
		  
                                                           
                                                         
     
		if (vp->v_mount->mnt_authcache_ttl) {
			if (!(vp->v_mount->mnt_kern_flag & MNTK_AUTH_CACHE_TTL)) {
				  
                                                                             
                                                                       
                                                                    
       
				if (action != KAUTH_VNODE_SEARCH || vp->v_type != VDIR) {
					vp = NULLVP;
				}
			}
			if (vp != NULLVP && vnode_cache_is_stale(vp) == TRUE) {
				vnode_uncache_authorized_action(vp, vp->v_authorized_actions);
				vp = NULLVP;
			}
		} else {
			vp = NULLVP;
		}
	}
	if (vp != NULLVP) {
		ucred = vfs_context_ucred(ctx);

		NAME_CACHE_LOCK_SHARED();

		if (vp->v_cred == ucred && (vp->v_authorized_actions & action) == action) {
			retval = TRUE;
		}

		NAME_CACHE_UNLOCK();
	}
	return retval;
}


void
vnode_cache_authorized_action(vnode_t vp, vfs_context_t ctx, kauth_action_t action)
{
	kauth_cred_t tcred = NOCRED;
	kauth_cred_t ucred;
	struct timeval tv;
	boolean_t ttl_active = FALSE;

	ucred = vfs_context_ucred(ctx);

	if (!IS_VALID_CRED(ucred) || action == 0) {
		return;
	}

	if ((vp->v_mount->mnt_kern_flag & (MNTK_AUTH_OPAQUE | MNTK_AUTH_CACHE_TTL))) {
		  
                                                           
                                                         
     
		if (vp->v_mount->mnt_authcache_ttl == 0) {
			return;
		}

		if (!(vp->v_mount->mnt_kern_flag & MNTK_AUTH_CACHE_TTL)) {
			  
                                                     
                                  
                                              
      
			if ((action & ~KAUTH_VNODE_SEARCH) || vp->v_type != VDIR) {
				return;
			}
		}
		ttl_active = TRUE;

		microuptime(&tv);
	}
	NAME_CACHE_LOCK();

	if (vp->v_cred != ucred) {
		kauth_cred_ref(ucred);
		  
                                                                                  
     
		tcred = vp->v_cred;
		vp->v_cred = ucred;
		vp->v_authorized_actions = 0;
	}
	if (ttl_active == TRUE && vp->v_authorized_actions == 0) {
		  
                                     
                                                  
                                                  
                                               
                                                
                   
     
		vp->v_cred_timestamp = (int)tv.tv_sec;
	}
	vp->v_authorized_actions |= action;

	NAME_CACHE_UNLOCK();

	if (IS_VALID_CRED(tcred)) {
		kauth_cred_unref(&tcred);
	}
}


boolean_t
vnode_cache_is_stale(vnode_t vp)
{
	struct timeval  tv;
	boolean_t       retval;

	microuptime(&tv);

	if ((tv.tv_sec - vp->v_cred_timestamp) > vp->v_mount->mnt_authcache_ttl) {
		retval = TRUE;
	} else {
		retval = FALSE;
	}

	return retval;
}



  
                       
                                                                                              
                                                                                                                    
   
int
cache_lookup_path(struct nameidata *ndp, struct componentname *cnp, vnode_t dp,
    vfs_context_t ctx, int *dp_authorized, vnode_t last_dp)
{
	char            *cp;                                                
	int             vid;
	int             vvid = 0;                                      
	vnode_t         vp = NULLVP;
	vnode_t         tdp = NULLVP;
	kauth_cred_t    ucred;
	boolean_t       ttl_enabled = FALSE;
	struct timeval  tv;
	mount_t         mp;
	unsigned int    hash;
	int             error = 0;
	boolean_t       dotdotchecked = FALSE;

#if CONFIG_TRIGGERS
	vnode_t         trigger_vp;
#endif                      

	ucred = vfs_context_ucred(ctx);
	ndp->ni_flag &= ~(NAMEI_TRAILINGSLASH);

	NAME_CACHE_LOCK_SHARED();

	if (dp->v_mount && (dp->v_mount->mnt_kern_flag & (MNTK_AUTH_OPAQUE | MNTK_AUTH_CACHE_TTL))) {
		ttl_enabled = TRUE;
		microuptime(&tv);
	}
	for (;;) {
		  
                        
    
                                                 
                                                              
                                                    
     
		hash = 0;
		cp = cnp->cn_nameptr;

		while (*cp && (*cp != '/')) {
			hash = crc32tab[((hash >> 24) ^ (unsigned char)*cp++)] ^ hash << 8;
		}
		  
                                                
                                           
                                              
     
		if (hash == 0) {
			hash = 1;
		}
		cnp->cn_hash = hash;
		cnp->cn_namelen = (int)(cp - cnp->cn_nameptr);

		ndp->ni_pathlen -= cnp->cn_namelen;
		ndp->ni_next = cp;

		  
                                                                    
                                                                    
                                                                    
                                                                  
                                                                      
     
		while (*cp == '/' && (cp[1] == '/' || cp[1] == '\0')) {
			cp++;
			ndp->ni_pathlen--;

			if (*cp == '\0') {
				ndp->ni_flag |= NAMEI_TRAILINGSLASH;
				*ndp->ni_next = '\0';
			}
		}
		ndp->ni_next = cp;

		cnp->cn_flags &= ~(MAKEENTRY | ISLASTCN | ISDOTDOT);

		if (*cp == '\0') {
			cnp->cn_flags |= ISLASTCN;
		}

		if (cnp->cn_namelen == 2 && cnp->cn_nameptr[1] == '.' && cnp->cn_nameptr[0] == '.') {
			cnp->cn_flags |= ISDOTDOT;
		}

		*dp_authorized = 0;
#if NAMEDRSRCFORK
		  
                                                  
    
                                                            
     
		if ((ndp->ni_pathlen == sizeof(_PATH_RSRCFORKSPEC)) &&
		    (cp[1] == '.' && cp[2] == '.') &&
		    bcmp(cp, _PATH_RSRCFORKSPEC, sizeof(_PATH_RSRCFORKSPEC)) == 0) {
			                                                                
			if ((dp->v_mount != NULL) &&
			    (dp->v_mount->mnt_flag & MNT_DOVOLFS) &&
			    (dp->v_mount->mnt_kern_flag & MNTK_NAMED_STREAMS) == 0) {
				goto skiprsrcfork;
			}
			cnp->cn_flags |= CN_WANTSRSRCFORK;
			cnp->cn_flags |= ISLASTCN;
			ndp->ni_next[0] = '\0';
			ndp->ni_pathlen = 1;
		}
skiprsrcfork:
#endif

#if CONFIG_MACF

		  
                                                          
                                                    
                                               
                                                      
                             
     
		if (!(cnp->cn_flags & DONOTAUTH)) {
			error = mac_vnode_check_lookup(ctx, dp, cnp);
			if (error) {
				NAME_CACHE_UNLOCK();
				goto errorout;
			}
		}
#endif          
		if (ttl_enabled &&
		    (dp->v_mount->mnt_authcache_ttl == 0 ||
		    ((tv.tv_sec - dp->v_cred_timestamp) > dp->v_mount->mnt_authcache_ttl))) {
			break;
		}

		  
                                              
    
                                                                 
                                                           
                                                              
                                                               
                                                                  
                                   
    
                                                            
                                        
     
		if ((dp->v_cred != ucred || !(dp->v_authorized_actions & KAUTH_VNODE_SEARCH)) &&
		    !(dp->v_authorized_actions & KAUTH_VNODE_SEARCHBYANYONE) &&
		    (ttl_enabled || !vfs_context_issuser(ctx))) {
			break;
		}

		  
                                                              
                                                           
                                                             
                                                                    
     
		*dp_authorized = 1;

		if ((cnp->cn_flags & (ISLASTCN | ISDOTDOT))) {
			  
                                                                      
                                                                  
                                                                  
                                                                      
                                       
      
#if CONFIG_FIRMLINKS
			if (cnp->cn_flags & ISDOTDOT && dp->v_fmlink && (dp->v_flag & VFMLINKTARGET)) {
				dp = dp->v_fmlink;
			}
#endif
			if (cnp->cn_nameiop != LOOKUP) {
				break;
			}
			if (cnp->cn_flags & LOCKPARENT) {
				break;
			}
			if (cnp->cn_flags & NOCACHE) {
				break;
			}

			if (cnp->cn_flags & ISDOTDOT) {
				  
                                         
                                     
       
				if ((dp->v_flag & VISHARDLINK)) {
					break;
				}
				  
                                     
                                      
                                        
                    
       
				if ((dp->v_flag & VROOT) ||
				    dp == ndp->ni_rootdir ||
				    dp->v_parent == NULLVP) {
					break;
				}
			}
		}

		if ((cnp->cn_flags & CN_SKIPNAMECACHE)) {
			  
                                               
                            
      
			break;
		}

		  
                                                        
                                        
     
		if (cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.') {
			vp = dp;
		} else if ((cnp->cn_flags & ISDOTDOT)) {
			  
                                                        
                                                        
                                                      
                                                     
                                                         
                               
     
                                                   
                                                        
                                                         
                                                 
                                                       
                                                       
      
			if (!dotdotchecked && (ndp->ni_rootdir != rootvnode)) {
				vnode_t tvp = dp;
				boolean_t defer = FALSE;
				boolean_t is_subdir = FALSE;

				defer = cache_check_vnode_issubdir(tvp,
				    ndp->ni_rootdir, &is_subdir, &tvp);

				if (defer) {
					                         
					break;
				} else if (!is_subdir) {
					  
                                            
                                        
                                             
                           
        
					vp = ndp->ni_rootdir;
				} else {
					  
                                        
                        
        
					vp = dp->v_parent;
				}
				dotdotchecked = TRUE;
			} else {
				vp = dp->v_parent;
			}
		} else {
			if ((vp = cache_lookup_locked(dp, cnp)) == NULLVP) {
				break;
			}

			if ((vp->v_flag & VISHARDLINK)) {
				  
                                                        
       
				vp = NULL;
				break;
			}
		}
		if ((cnp->cn_flags & ISLASTCN)) {
			break;
		}

		if (vp->v_type != VDIR) {
			if (vp->v_type != VLNK) {
				vp = NULL;
			}
			break;
		}

		if ((mp = vp->v_mountedhere) && ((cnp->cn_flags & NOCROSSMOUNT) == 0)) {
			vnode_t tmp_vp = mp->mnt_realrootvp;
			if (tmp_vp == NULLVP || mp->mnt_generation != mount_generation ||
			    mp->mnt_realrootvp_vid != tmp_vp->v_id) {
				break;
			}
			vp = tmp_vp;
		}

#if CONFIG_TRIGGERS
		  
                                                                
                                                             
                                                           
     
		if (vp->v_resolve) {
			break;
		}
#endif                      


		dp = vp;
		vp = NULLVP;

		cnp->cn_nameptr = ndp->ni_next + 1;
		ndp->ni_pathlen--;
		while (*cnp->cn_nameptr == '/') {
			cnp->cn_nameptr++;
			ndp->ni_pathlen--;
		}
	}
	if (vp != NULLVP) {
		vvid = vp->v_id;
	}
	vid = dp->v_id;

	NAME_CACHE_UNLOCK();

	if ((vp != NULLVP) && (vp->v_type != VLNK) &&
	    ((cnp->cn_flags & (ISLASTCN | LOCKPARENT | WANTPARENT | SAVESTART)) == ISLASTCN)) {
		  
                                                          
                                                         
                                                          
                                                         
                                                       
                                                      
                         
     
		tdp = dp;
		dp = NULLVP;
	} else {
need_dp:
		  
                                           
                                                        
                                                         
                                                               
     
		if (last_dp != dp) {
			if (dp == ndp->ni_usedvp) {
				  
                                                         
                                                          
                                                           
                                                            
                                                            
                                                               
                                                                 
                                                    
       
				vnode_get(dp);
			} else if ((error = vnode_getwithvid_drainok(dp, vid))) {
				  
                                  
                                   
                                   
                        
      
                                                       
                                                           
                                                    
                                                            
                                                          
                                                                 
                                                                
                                                                  
                               
       
				if (error == ENODEV) {
					error = ENOENT;
				} else {
					error = ERECYCLE;
				}
				goto errorout;
			}
		}
	}
	if (vp != NULLVP) {
		if ((vnode_getwithvid_drainok(vp, vvid))) {
			vp = NULLVP;

			  
                                             
                                                
                                                 
                                                  
                                             
                     
      
			if (dp == NULLVP) {
				dp = tdp;
				goto need_dp;
			}
		}
	}

	ndp->ni_dvp = dp;
	ndp->ni_vp  = vp;

#if CONFIG_TRIGGERS
	trigger_vp = vp ? vp : dp;
	if ((error == 0) && (trigger_vp != NULLVP) && vnode_isdir(trigger_vp)) {
		error = vnode_trigger_resolve(trigger_vp, ndp, ctx);
		if (error) {
			if (vp) {
				vnode_put(vp);
			}
			if (dp) {
				vnode_put(dp);
			}
			goto errorout;
		}
	}
#endif                      

errorout:
	  
                                                                                
                                                                                      
                                                                                
                                                                                  
                                                                                       
                                                                                  
                                                         
    
	if ((last_dp != NULLVP) && (last_dp != ndp->ni_dvp)) {
		vnode_put(last_dp);
	}

	                                                                  
	return error;
}


static vnode_t
cache_lookup_locked(vnode_t dvp, struct componentname *cnp)
{
	struct namecache *ncp;
	struct nchashhead *ncpp;
	long namelen = cnp->cn_namelen;
	unsigned int hashval = cnp->cn_hash;

	if (nc_disabled) {
		return NULL;
	}

	ncpp = NCHHASH(dvp, cnp->cn_hash);
	LIST_FOREACH(ncp, ncpp, nc_hash) {
		if ((ncp->nc_dvp == dvp) && (ncp->nc_hashval == hashval)) {
			if (strncmp(ncp->nc_name, cnp->cn_nameptr, namelen) == 0 && ncp->nc_name[namelen] == 0) {
				break;
			}
		}
	}
	if (ncp == 0) {
		  
                               
     
		NCHSTAT(ncs_miss);
		return NULL;
	}
	NCHSTAT(ncs_goodhits);

	return ncp->nc_vp;
}


unsigned int hash_string(const char *cp, int len);
  
                                                          
                                
  
unsigned int
hash_string(const char *cp, int len)
{
	unsigned hash = 0;

	if (len) {
		while (len--) {
			hash = crc32tab[((hash >> 24) ^ (unsigned char)*cp++)] ^ hash << 8;
		}
	} else {
		while (*cp != '\0') {
			hash = crc32tab[((hash >> 24) ^ (unsigned char)*cp++)] ^ hash << 8;
		}
	}
	  
                                               
                                          
                                             
    
	if (hash == 0) {
		hash = 1;
	}
	return hash;
}


  
                               
  
                                                                    
                                                                   
                                  
  
                                                                 
                                                                    
                                                                 
                                                                  
                                                                     
                                       
   

int
cache_lookup(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp)
{
	struct namecache *ncp;
	struct nchashhead *ncpp;
	long namelen = cnp->cn_namelen;
	unsigned int hashval;
	boolean_t       have_exclusive = FALSE;
	uint32_t vid;
	vnode_t  vp;

	if (cnp->cn_hash == 0) {
		cnp->cn_hash = hash_string(cnp->cn_nameptr, cnp->cn_namelen);
	}
	hashval = cnp->cn_hash;

	if (nc_disabled) {
		return 0;
	}

	NAME_CACHE_LOCK_SHARED();

relook:
	ncpp = NCHHASH(dvp, cnp->cn_hash);
	LIST_FOREACH(ncp, ncpp, nc_hash) {
		if ((ncp->nc_dvp == dvp) && (ncp->nc_hashval == hashval)) {
			if (strncmp(ncp->nc_name, cnp->cn_nameptr, namelen) == 0 && ncp->nc_name[namelen] == 0) {
				break;
			}
		}
	}
	                                
	if (ncp == 0) {
		NCHSTAT(ncs_miss);
		NAME_CACHE_UNLOCK();
		return 0;
	}

	                                                
	if ((cnp->cn_flags & MAKEENTRY) == 0) {
		if (have_exclusive == TRUE) {
			NCHSTAT(ncs_badhits);
			cache_delete(ncp, 1);
			NAME_CACHE_UNLOCK();
			return 0;
		}
		NAME_CACHE_UNLOCK();
		NAME_CACHE_LOCK();
		have_exclusive = TRUE;
		goto relook;
	}
	vp = ncp->nc_vp;

	                                                   
	if (vp) {
		NCHSTAT(ncs_goodhits);

		vid = vp->v_id;
		NAME_CACHE_UNLOCK();

		if (vnode_getwithvid(vp, vid)) {
#if COLLECT_STATS
			NAME_CACHE_LOCK();
			NCHSTAT(ncs_badvid);
			NAME_CACHE_UNLOCK();
#endif
			return 0;
		}
		*vpp = vp;
		return -1;
	}

	                                                                
	if (cnp->cn_nameiop == CREATE || cnp->cn_nameiop == RENAME) {
		if (have_exclusive == TRUE) {
			NCHSTAT(ncs_badhits);
			cache_delete(ncp, 1);
			NAME_CACHE_UNLOCK();
			return 0;
		}
		NAME_CACHE_UNLOCK();
		NAME_CACHE_LOCK();
		have_exclusive = TRUE;
		goto relook;
	}

	  
                                                                      
    
	NCHSTAT(ncs_neghits);

	NAME_CACHE_UNLOCK();
	return ENOENT;
}

const char *
cache_enter_create(vnode_t dvp, vnode_t vp, struct componentname *cnp)
{
	const char *strname;

	if (cnp->cn_hash == 0) {
		cnp->cn_hash = hash_string(cnp->cn_nameptr, cnp->cn_namelen);
	}

	  
                                           
                                             
                                                                     
    
	strname = add_name_internal(cnp->cn_nameptr, cnp->cn_namelen, cnp->cn_hash, TRUE, 0);

	NAME_CACHE_LOCK();

	cache_enter_locked(dvp, vp, cnp, strname);

	NAME_CACHE_UNLOCK();

	return strname;
}


  
                               
                                          
                                               
                                             
                                               
                                     
   
void
cache_enter_with_gen(struct vnode *dvp, struct vnode *vp, struct componentname *cnp, int gen)
{
	if (cnp->cn_hash == 0) {
		cnp->cn_hash = hash_string(cnp->cn_nameptr, cnp->cn_namelen);
	}

	NAME_CACHE_LOCK();

	if (dvp->v_nc_generation == gen) {
		(void)cache_enter_locked(dvp, vp, cnp, NULL);
	}

	NAME_CACHE_UNLOCK();
}


  
                             
   
void
cache_enter(struct vnode *dvp, struct vnode *vp, struct componentname *cnp)
{
	const char *strname;

	if (cnp->cn_hash == 0) {
		cnp->cn_hash = hash_string(cnp->cn_nameptr, cnp->cn_namelen);
	}

	  
                                          
                                         
    
	strname = add_name_internal(cnp->cn_nameptr, cnp->cn_namelen, cnp->cn_hash, FALSE, 0);

	NAME_CACHE_LOCK();

	cache_enter_locked(dvp, vp, cnp, strname);

	NAME_CACHE_UNLOCK();
}


static void
cache_enter_locked(struct vnode *dvp, struct vnode *vp, struct componentname *cnp, const char *strname)
{
	struct namecache *ncp, *negp;
	struct nchashhead *ncpp;

	if (nc_disabled) {
		return;
	}

	  
                                              
    
	if ((vp != NULLVP) && (LIST_FIRST(&vp->v_nclinks))) {
		  
                                   
                                       
     
		if (strname != NULL) {
			vfs_removename(strname);
		}
		return;
	}
	  
                                                           
                                                           
                                                      
    
	if (numcache < desiredNodes &&
	    ((ncp = nchead.tqh_first) == NULL ||
	    ncp->nc_hash.le_prev != 0)) {
		  
                            
     
		ncp = zalloc(namecache_zone);
		numcache++;
	} else {
		  
                       
     
		ncp = TAILQ_FIRST(&nchead);
		TAILQ_REMOVE(&nchead, ncp, nc_entry);

		if (ncp->nc_hash.le_prev != 0) {
			  
                                
                                  
      
			NCHSTAT(ncs_stolen);
			cache_delete(ncp, 0);
		}
	}
	NCHSTAT(ncs_enters);

	  
                                                                       
    
	ncp->nc_vp = vp;
	ncp->nc_dvp = dvp;
	ncp->nc_hashval = cnp->cn_hash;

	if (strname == NULL) {
		ncp->nc_name = add_name_internal(cnp->cn_nameptr, cnp->cn_namelen, cnp->cn_hash, FALSE, 0);
	} else {
		ncp->nc_name = strname;
	}

	  
	                                                             
	                                                               
	                                                            
	                                                               
	                                                             
	                                                                                                                                 
	  
	const char *vn_name = vp ? vp->v_name : NULL;
	unsigned int len = vn_name ? (unsigned int)strlen(vn_name) : 0;
	if (vn_name && ncp && ncp->nc_name && strncmp(ncp->nc_name, vn_name, len) != 0) {
		unsigned int hash = hash_string(vn_name, len);

		vfs_removename(ncp->nc_name);
		ncp->nc_name = add_name_internal(vn_name, len, hash, FALSE, 0);
		ncp->nc_hashval = hash;
	}

	  
                                         
                                       
    
	TAILQ_INSERT_TAIL(&nchead, ncp, nc_entry);

	ncpp = NCHHASH(dvp, cnp->cn_hash);
#if DIAGNOSTIC
	{
		struct namecache *p;

		for (p = ncpp->lh_first; p != 0; p = p->nc_hash.le_next) {
			if (p == ncp) {
				panic("cache_enter: duplicate");
			}
		}
	}
#endif
	  
                                            
    
	LIST_INSERT_HEAD(ncpp, ncp, nc_hash);

	if (vp) {
		  
                                          
                     
     
		LIST_INSERT_HEAD(&vp->v_nclinks, ncp, nc_un.nc_link);
	} else {
		  
                                                
                                         
     
		TAILQ_INSERT_TAIL(&neghead, ncp, nc_un.nc_negentry);

		ncs_negtotal++;

		if (ncs_negtotal > desiredNegNodes) {
			  
                                        
                                       
                
      
			negp = TAILQ_FIRST(&neghead);
			cache_delete(negp, 1);
		}
	}
	  
                                                 
                       
    
	if (vp) {
		TAILQ_INSERT_TAIL(&dvp->v_ncchildren, ncp, nc_child);
	} else {
		TAILQ_INSERT_HEAD(&dvp->v_ncchildren, ncp, nc_child);
	}
}


  
                                     
   
static void
init_crc32(void)
{
	  
                                       
                                                    
                                                     
    
	unsigned int crc32_polynomial = 0x04c11db7;
	unsigned int i, j;

	  
                                                                       
    
	for (i = 0; i < 256; i++) {
		unsigned int crc_rem = i << 24;

		for (j = 0; j < 8; j++) {
			if (crc_rem & 0x80000000) {
				crc_rem = (crc_rem << 1) ^ crc32_polynomial;
			} else {
				crc_rem = (crc_rem << 1);
			}
		}
		crc32tab[i] = crc_rem;
	}
}


  
                                                                 
   
void
nchinit(void)
{
	desiredNegNodes = (desiredvnodes / 10);
	desiredNodes = desiredvnodes + desiredNegNodes;

	TAILQ_INIT(&nchead);
	TAILQ_INIT(&neghead);

	init_crc32();

	nchashtbl = hashinit(MAX(CONFIG_NC_HASH, (2 * desiredNodes)), M_CACHE, &nchash);
	nchashmask = nchash;
	nchash++;

	init_string_table();

	for (int i = 0; i < NUM_STRCACHE_LOCKS; i++) {
		lck_mtx_init(&strcache_mtx_locks[i], &strcache_lck_grp, &strcache_lck_attr);
	}
}

void
name_cache_lock_shared(void)
{
	lck_rw_lock_shared(&namecache_rw_lock);
}

void
name_cache_lock(void)
{
	lck_rw_lock_exclusive(&namecache_rw_lock);
}

void
name_cache_unlock(void)
{
	lck_rw_done(&namecache_rw_lock);
}


int
resize_namecache(int newsize)
{
	struct nchashhead   *new_table;
	struct nchashhead   *old_table;
	struct nchashhead   *old_head, *head;
	struct namecache    *entry, *next;
	uint32_t            i, hashval;
	int                 dNodes, dNegNodes, nelements;
	u_long              new_size, old_size;

	if (newsize < 0) {
		return EINVAL;
	}

	dNegNodes = (newsize / 10);
	dNodes = newsize + dNegNodes;
	                                 
	if (dNodes <= desiredNodes) {
		return 0;
	}

	if (os_mul_overflow(dNodes, 2, &nelements)) {
		return EINVAL;
	}

	new_table = hashinit(nelements, M_CACHE, &nchashmask);
	new_size  = nchashmask + 1;

	if (new_table == NULL) {
		return ENOMEM;
	}

	NAME_CACHE_LOCK();
	                 
	old_table = nchashtbl;
	nchashtbl = new_table;
	old_size  = nchash;
	nchash    = new_size;

	                                                     
	                
	  
	for (i = 0; i < old_size; i++) {
		old_head = &old_table[i];
		for (entry = old_head->lh_first; entry != NULL; entry = next) {
			  
			                                                        
			                                                     
			                                                 
			hashval = hash_string(entry->nc_name, 0);
			entry->nc_hashval = hashval;
			head = NCHHASH(entry->nc_dvp, hashval);

			next = entry->nc_hash.le_next;
			LIST_INSERT_HEAD(head, entry, nc_hash);
		}
	}
	desiredNodes = dNodes;
	desiredNegNodes = dNegNodes;

	NAME_CACHE_UNLOCK();
	FREE(old_table, M_CACHE);

	return 0;
}

static void
cache_delete(struct namecache *ncp, int free_entry)
{
	NCHSTAT(ncs_deletes);

	if (ncp->nc_vp) {
		LIST_REMOVE(ncp, nc_un.nc_link);
	} else {
		TAILQ_REMOVE(&neghead, ncp, nc_un.nc_negentry);
		ncs_negtotal--;
	}
	TAILQ_REMOVE(&(ncp->nc_dvp->v_ncchildren), ncp, nc_child);

	LIST_REMOVE(ncp, nc_hash);
	  
                                  
                                
                                 
                
    
	ncp->nc_hash.le_prev = NULL;

	vfs_removename(ncp->nc_name);
	ncp->nc_name = NULL;
	if (free_entry) {
		TAILQ_REMOVE(&nchead, ncp, nc_entry);
		zfree(namecache_zone, ncp);
		numcache--;
	}
}


  
                                      
                                      
   
static void
cache_purge_locked(vnode_t vp, kauth_cred_t *credp)
{
	struct namecache *ncp;

	*credp = NULL;
	if ((LIST_FIRST(&vp->v_nclinks) == NULL) &&
	    (TAILQ_FIRST(&vp->v_ncchildren) == NULL) &&
	    (vp->v_cred == NOCRED) &&
	    (vp->v_parent == NULLVP)) {
		return;
	}

	if (vp->v_parent) {
		vp->v_parent->v_nc_generation++;
	}

	while ((ncp = LIST_FIRST(&vp->v_nclinks))) {
		cache_delete(ncp, 1);
	}

	while ((ncp = TAILQ_FIRST(&vp->v_ncchildren))) {
		cache_delete(ncp, 1);
	}

	  
                                                                                 
    
	*credp = vp->v_cred;
	vp->v_cred = NOCRED;
	vp->v_authorized_actions = 0;
}

void
cache_purge(vnode_t vp)
{
	kauth_cred_t tcred = NULL;

	if ((LIST_FIRST(&vp->v_nclinks) == NULL) &&
	    (TAILQ_FIRST(&vp->v_ncchildren) == NULL) &&
	    (vp->v_cred == NOCRED) &&
	    (vp->v_parent == NULLVP)) {
		return;
	}

	NAME_CACHE_LOCK();

	cache_purge_locked(vp, &tcred);

	NAME_CACHE_UNLOCK();

	if (tcred && IS_VALID_CRED(tcred)) {
		kauth_cred_unref(&tcred);
	}
}

  
                                                            
                                                            
                                                         
                                                          
                                                       
   
void
cache_purge_negatives(vnode_t vp)
{
	struct namecache *ncp, *next_ncp;

	NAME_CACHE_LOCK();

	TAILQ_FOREACH_SAFE(ncp, &vp->v_ncchildren, nc_child, next_ncp) {
		if (ncp->nc_vp) {
			break;
		}

		cache_delete(ncp, 1);
	}

	NAME_CACHE_UNLOCK();
}

  
                                                         
  
                                                                  
                            
   
void
cache_purgevfs(struct mount *mp)
{
	struct nchashhead *ncpp;
	struct namecache *ncp;

	NAME_CACHE_LOCK();
	                                             
	for (ncpp = &nchashtbl[nchash - 1]; ncpp >= nchashtbl; ncpp--) {
restart:
		for (ncp = ncpp->lh_first; ncp != 0; ncp = ncp->nc_hash.le_next) {
			if (ncp->nc_dvp->v_mount == mp) {
				cache_delete(ncp, 0);
				goto restart;
			}
		}
	}
	NAME_CACHE_UNLOCK();
}



  
                      
  
static LIST_HEAD(stringhead, string_t) * string_ref_table;
static u_long   string_table_mask;
static uint32_t filled_buckets = 0;


typedef struct string_t {
	LIST_ENTRY(string_t)  hash_chain;
	const char *str;
	uint32_t              refcount;
} string_t;


static void
resize_string_ref_table(void)
{
	struct stringhead *new_table;
	struct stringhead *old_table;
	struct stringhead *old_head, *head;
	string_t          *entry, *next;
	uint32_t           i, hashval;
	u_long             new_mask, old_mask;

	  
                                           
                                                 
                                              
                                                  
                        
    
	lck_rw_lock_exclusive(&strtable_rw_lock);

	if (4 * filled_buckets < ((string_table_mask + 1) * 3)) {
		lck_rw_done(&strtable_rw_lock);
		return;
	}
	assert(string_table_mask < INT32_MAX);
	new_table = hashinit((int)(string_table_mask + 1) * 2, M_CACHE, &new_mask);

	if (new_table == NULL) {
		printf("failed to resize the hash table.\n");
		lck_rw_done(&strtable_rw_lock);
		return;
	}

	                 
	old_table         = string_ref_table;
	string_ref_table  = new_table;
	old_mask          = string_table_mask;
	string_table_mask = new_mask;
	filled_buckets    = 0;

	                                                     
	                
	  
	for (i = 0; i <= old_mask; i++) {
		old_head = &old_table[i];
		for (entry = old_head->lh_first; entry != NULL; entry = next) {
			hashval = hash_string((const char *)entry->str, 0);
			head = &string_ref_table[hashval & string_table_mask];
			if (head->lh_first == NULL) {
				filled_buckets++;
			}
			next = entry->hash_chain.le_next;
			LIST_INSERT_HEAD(head, entry, hash_chain);
		}
	}
	lck_rw_done(&strtable_rw_lock);

	FREE(old_table, M_CACHE);
}


static void
init_string_table(void)
{
	string_ref_table = hashinit(CONFIG_VFS_NAMES, M_CACHE, &string_table_mask);
}


const char *
vfs_addname(const char *name, uint32_t len, u_int hashval, u_int flags)
{
	return add_name_internal(name, len, hashval, FALSE, flags);
}


static const char *
add_name_internal(const char *name, uint32_t len, u_int hashval, boolean_t need_extra_ref, __unused u_int flags)
{
	struct stringhead *head;
	string_t          *entry;
	uint32_t          chain_len = 0;
	uint32_t          hash_index;
	uint32_t          lock_index;
	char              *ptr;

	if (len > MAXPATHLEN) {
		len = MAXPATHLEN;
	}

	  
                                                          
                                                        
                  
    
	if (len > 0 && name[len - 1] == '\0') {
		len--;
	}
	if (hashval == 0) {
		hashval = hash_string(name, len);
	}

	  
                                                   
                                                 
                                   
    
	lck_rw_lock_shared(&strtable_rw_lock);

	  
                                                   
    
	if (4 * filled_buckets >= ((string_table_mask + 1) * 3)) {
		lck_rw_done(&strtable_rw_lock);

		resize_string_ref_table();

		lck_rw_lock_shared(&strtable_rw_lock);
	}
	hash_index = hashval & string_table_mask;
	lock_index = hash_index % NUM_STRCACHE_LOCKS;

	head = &string_ref_table[hash_index];

	lck_mtx_lock_spin(&strcache_mtx_locks[lock_index]);

	for (entry = head->lh_first; entry != NULL; chain_len++, entry = entry->hash_chain.le_next) {
		if (strncmp(entry->str, name, len) == 0 && entry->str[len] == 0) {
			entry->refcount++;
			break;
		}
	}
	if (entry == NULL) {
		lck_mtx_convert_spin(&strcache_mtx_locks[lock_index]);
		  
                                       
     
		entry = kheap_alloc(KHEAP_DEFAULT, sizeof(string_t) + len + 1, Z_WAITOK);

		if (head->lh_first == NULL) {
			OSAddAtomic(1, &filled_buckets);
		}
		ptr = (char *)((char *)entry + sizeof(string_t));
		strncpy(ptr, name, len);
		ptr[len] = '\0';
		entry->str = ptr;
		entry->refcount = 1;
		LIST_INSERT_HEAD(head, entry, hash_chain);
	}
	if (need_extra_ref == TRUE) {
		entry->refcount++;
	}

	lck_mtx_unlock(&strcache_mtx_locks[lock_index]);
	lck_rw_done(&strtable_rw_lock);

	return (const char *)entry->str;
}


int
vfs_removename(const char *nameref)
{
	struct stringhead *head;
	string_t          *entry;
	uint32_t           hashval;
	uint32_t           hash_index;
	uint32_t           lock_index;
	int                retval = ENOENT;

	hashval = hash_string(nameref, 0);

	  
                                                   
                                                 
                                   
    
	lck_rw_lock_shared(&strtable_rw_lock);
	  
                                               
                                            
                         
    
	hash_index = hashval & string_table_mask;
	lock_index = hash_index % NUM_STRCACHE_LOCKS;

	head = &string_ref_table[hash_index];

	lck_mtx_lock_spin(&strcache_mtx_locks[lock_index]);

	for (entry = head->lh_first; entry != NULL; entry = entry->hash_chain.le_next) {
		if (entry->str == nameref) {
			entry->refcount--;

			if (entry->refcount == 0) {
				LIST_REMOVE(entry, hash_chain);

				if (head->lh_first == NULL) {
					OSAddAtomic(-1, &filled_buckets);
				}
			} else {
				entry = NULL;
			}
			retval = 0;
			break;
		}
	}
	lck_mtx_unlock(&strcache_mtx_locks[lock_index]);
	lck_rw_done(&strtable_rw_lock);

	kheap_free_addr(KHEAP_DEFAULT, entry);

	return retval;
}


#ifdef DUMP_STRING_TABLE
void
dump_string_table(void)
{
	struct stringhead *head;
	string_t          *entry;
	u_long            i;

	lck_rw_lock_shared(&strtable_rw_lock);

	for (i = 0; i <= string_table_mask; i++) {
		head = &string_ref_table[i];
		for (entry = head->lh_first; entry != NULL; entry = entry->hash_chain.le_next) {
			printf("%6d - %s\n", entry->refcount, entry->str);
		}
	}
	lck_rw_done(&strtable_rw_lock);
}
#endif                         
