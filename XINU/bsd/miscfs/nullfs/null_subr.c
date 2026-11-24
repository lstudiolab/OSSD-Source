/*
 * Copyright (c) 2016 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */

/*-
 * Portions Copyright (c) 1992, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *  @(#)null_subr.c 8.7 (Berkeley) 5/14/95
 *
 * $FreeBSD$
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/vnode.h>

#include "nullfs.h"

  
                    
                                                        
                                                    
                                                      
                                               
   

#define NULL_HASH_SIZE (desiredvnodes / 10)

                                                                              
                   
#define NULL_NHASH(vp) (&null_node_hashtbl[((((uintptr_t)vp) >> vnsz2log) + (uintptr_t)vnode_mount(vp)) & null_hash_mask])

static LIST_HEAD(null_node_hashhead, null_node) * null_node_hashtbl;
static LCK_GRP_DECLARE(null_hashlck_grp, "com.apple.filesystems.nullfs");
static LCK_MTX_DECLARE(null_hashmtx, &null_hashlck_grp);
static u_long null_hash_mask;

                                                                               
          
                                                                             
          
                                                 
static int vnsz2log = 9;

static int null_hashins(struct mount *, struct null_node *, struct vnode **);

void
nullfs_init_lck(lck_mtx_t * lck)
{
	lck_mtx_init(lck, &null_hashlck_grp, LCK_ATTR_NULL);
}

void
nullfs_destroy_lck(lck_mtx_t * lck)
{
	lck_mtx_destroy(lck, &null_hashlck_grp);
}

  
                           
   
int
nullfs_init(__unused struct vfsconf * vfsp)
{
	NULLFSDEBUG("%s\n", __FUNCTION__);
	null_node_hashtbl = hashinit(NULL_HASH_SIZE, M_TEMP, &null_hash_mask);
	NULLFSDEBUG("%s finished\n", __FUNCTION__);
	return 0;
}

int
nullfs_uninit(void)
{
	                                                                     
                           
	hashdestroy(null_node_hashtbl, M_TEMP, null_hash_mask);
	return 0;
}

  
                                                                                       
                                                                                                
   
int
null_hashget(struct mount * mp, struct vnode * lowervp, struct vnode ** vpp)
{
	struct null_node_hashhead * hd = NULL;
	struct null_node * a = NULL;
	struct vnode * vp = NULL;
	uint32_t vp_vid = 0;
	int error = ENOENT;

	  
                                                        
                                                               
                                                                
                                                                  
    
	hd = NULL_NHASH(lowervp);
	                                                           
	lck_mtx_lock(&null_hashmtx);
	LIST_FOREACH(a, hd, null_hash)
	{
		if (a->null_lowervp == lowervp && vnode_mount(NULLTOV(a)) == mp) {
			vp = NULLTOV(a);
			if (a->null_lowervid != vnode_vid(lowervp)) {
				                      
				error = EIO;
				vp = NULL;
			} else {
				vp_vid = a->null_myvid;
			}
			                                                                                                 
			break;
		}
	}
	lck_mtx_unlock(&null_hashmtx);
	if (vp != NULL) {
		error = vnode_getwithvid(vp, vp_vid);
		if (error == 0) {
			*vpp = vp;
		}
	}
	return error;
}

  
                                                                         
              
   
static int
null_hashins(struct mount * mp, struct null_node * xp, struct vnode ** vpp)
{
	struct null_node_hashhead * hd = NULL;
	struct null_node * oxp = NULL;
	struct vnode * ovp = NULL;
	uint32_t oxp_vid = 0;
	int error = 0;

	hd = NULL_NHASH(xp->null_lowervp);
	lck_mtx_lock(&null_hashmtx);
	LIST_FOREACH(oxp, hd, null_hash)
	{
		if (oxp->null_lowervp == xp->null_lowervp && vnode_mount(NULLTOV(oxp)) == mp) {
			  
                                                
                
      
			ovp = NULLTOV(oxp);
			if (oxp->null_lowervid != vnode_vid(oxp->null_lowervp)) {
				                                                                       
                         
                                                                           
                                       
                       
				error = EIO;
				ovp = NULL;
			} else {
				oxp_vid = oxp->null_myvid;
			}
			goto end;
		}
	}
	                                                                             
                              
	LIST_INSERT_HEAD(hd, xp, null_hash);
	xp->null_flags |= NULL_FLAG_HASHED;
end:
	lck_mtx_unlock(&null_hashmtx);
	if (ovp != NULL) {
		                                                                
		error = vnode_getwithvid(ovp, oxp_vid);
		if (error == 0) {
			*vpp = ovp;
		}
	}
	return error;
}

  
                         
   
void
null_hashrem(struct null_node * xp)
{
	lck_mtx_lock(&null_hashmtx);
	LIST_REMOVE(xp, null_hash);
	lck_mtx_unlock(&null_hashmtx);
}

static struct null_node *
null_nodecreate(struct vnode * lowervp)
{
	struct null_node * xp;

	MALLOC(xp, struct null_node *, sizeof(struct null_node), M_TEMP, M_WAITOK | M_ZERO);
	if (xp != NULL) {
		if (lowervp) {
			xp->null_lowervp  = lowervp;
			xp->null_lowervid = vnode_vid(lowervp);
		}
	}
	return xp;
}

                                                                   
int
null_getnewvnode(
	struct mount * mp, struct vnode * lowervp, struct vnode * dvp, struct vnode ** vpp, struct componentname * cnp, int root)
{
	struct vnode_fsparam vnfs_param;
	int error             = 0;
	enum vtype type       = VDIR;
	struct null_node * xp = null_nodecreate(lowervp);

	if (xp == NULL) {
		return ENOMEM;
	}

	if (lowervp) {
		type = vnode_vtype(lowervp);
	}

	vnfs_param.vnfs_mp         = mp;
	vnfs_param.vnfs_vtype      = type;
	vnfs_param.vnfs_str        = "nullfs";
	vnfs_param.vnfs_dvp        = dvp;
	vnfs_param.vnfs_fsnode     = (void *)xp;
	vnfs_param.vnfs_vops       = nullfs_vnodeop_p;
	vnfs_param.vnfs_markroot   = root;
	vnfs_param.vnfs_marksystem = 0;
	vnfs_param.vnfs_rdev       = 0;
	vnfs_param.vnfs_filesize   = 0;                                                                     
	vnfs_param.vnfs_cnp        = cnp;
	vnfs_param.vnfs_flags      = VNFS_ADDFSREF;

	error = vnode_create(VNCREATE_FLAVOR, VCREATESIZE, &vnfs_param, vpp);
	if (error == 0) {
		xp->null_vnode = *vpp;
		xp->null_myvid = vnode_vid(*vpp);
		vnode_settag(*vpp, VT_NULL);
	} else {
		FREE(xp, M_TEMP);
	}
	return error;
}

  
                                          
                                                     
  
                                                              
   
int
null_nodeget(
	struct mount * mp, struct vnode * lowervp, struct vnode * dvp, struct vnode ** vpp, struct componentname * cnp, int root)
{
	struct vnode * vp;
	int error;

	                              
	error = null_hashget(mp, lowervp, vpp);
	                                                                              
                
	if (error != ENOENT) {
		                                                                         
                                
		                                                         
                                                                             
		return error;
	}

	  
                                                                 
                                                    
    
	error = vnode_ref(lowervp);                                                                    
	if (error) {
		                                                                                  
		return error;
	}

	error = null_getnewvnode(mp, lowervp, dvp, &vp, cnp, root);

	if (error) {
		vnode_rele(lowervp);
		return error;
	}

	  
                                                                 
                                        
    
	error = null_hashins(mp, VTONULL(vp), vpp);
	if (error || *vpp != NULL) {
		                                                                   
		vnode_recycle(vp);
		vnode_put(vp);
		                                                             
		return error;
	}

	                                             
	*vpp = vp;

	return 0;
}
