/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

#include "bindfs.h"

  
                    
                                                        
                                                    
                                                      
                                               
   

#define BIND_HASH_SIZE (desiredvnodes / 10)

                                                                              
                   
#define BIND_NHASH(vp) (&bind_node_hashtbl[((((uintptr_t)vp) >> vnsz2log) + (uintptr_t)vnode_mount(vp)) & bind_hash_mask])

static LIST_HEAD(bind_node_hashhead, bind_node) * bind_node_hashtbl;
static LCK_GRP_DECLARE(bind_hashlck_grp, "com.apple.filesystems.bindfs");
static LCK_MTX_DECLARE(bind_hashmtx, &bind_hashlck_grp);
static u_long bind_hash_mask;

                                                                            
                                                 
static int vnsz2log = 9;

static int bind_hashins(struct mount *, struct bind_node *, struct vnode **);

  
                           
   
int
bindfs_init(__unused struct vfsconf * vfsp)
{
	BINDFSDEBUG("%s\n", __FUNCTION__);

	bind_node_hashtbl = hashinit(BIND_HASH_SIZE, M_TEMP, &bind_hash_mask);
	if (bind_node_hashtbl == NULL) {
		goto error;
	}

	BINDFSDEBUG("%s finished\n", __FUNCTION__);
	return 0;
error:
	printf("BINDFS: failed to initialize globals\n");
	return KERN_FAILURE;
}

int
bindfs_destroy(void)
{
	                                                                     
                           
	hashdestroy(bind_node_hashtbl, M_TEMP, bind_hash_mask);
	return 0;
}

  
                                                                                       
                                                                                                
   
int
bind_hashget(struct mount * mp, struct vnode * lowervp, struct vnode ** vpp)
{
	struct bind_node_hashhead * hd;
	struct bind_node * a;
	struct vnode * vp = NULL;
	uint32_t vp_vid = 0;
	int error = ENOENT;

	  
                                                        
                                                               
                                                                
                                                                  
    
	hd = BIND_NHASH(lowervp);
	lck_mtx_lock(&bind_hashmtx);
	LIST_FOREACH(a, hd, bind_hash)
	{
		if (a->bind_lowervp == lowervp && vnode_mount(BINDTOV(a)) == mp) {
			vp = BINDTOV(a);
			if (a->bind_lowervid != vnode_vid(lowervp)) {
				                      
				error = EIO;
				vp = NULL;
			} else {
				vp_vid = a->bind_myvid;
			}
			break;
		}
	}
	lck_mtx_unlock(&bind_hashmtx);

	if (vp != NULL) {
		error = vnode_getwithvid(vp, vp_vid);
		if (error == 0) {
			*vpp = vp;
		}
	}
	return error;
}

  
                                                                         
              
                                                                           
                                                                                    
                                                                          
                                                                                 
   
static int
bind_hashins(struct mount * mp, struct bind_node * xp, struct vnode ** vpp)
{
	struct bind_node_hashhead * hd;
	struct bind_node * oxp;
	struct vnode * ovp = NULL;
	uint32_t oxp_vid = 0;
	int error = 0;

	hd = BIND_NHASH(xp->bind_lowervp);
	lck_mtx_lock(&bind_hashmtx);
	LIST_FOREACH(oxp, hd, bind_hash)
	{
		if (oxp->bind_lowervp == xp->bind_lowervp && vnode_mount(BINDTOV(oxp)) == mp) {
			ovp = BINDTOV(oxp);
			if (oxp->bind_lowervid != vnode_vid(oxp->bind_lowervp)) {
				                                                                        
                         
                                                                           
                                       
                       
				error = EIO;
				ovp = NULL;
			} else {
				oxp_vid = oxp->bind_myvid;
			}
			goto end;
		}
	}
	                                                                             
                                    
	LIST_INSERT_HEAD(hd, xp, bind_hash);
	xp->bind_flags |= BIND_FLAG_HASHED;
end:
	lck_mtx_unlock(&bind_hashmtx);
	if (ovp != NULL) {
		                                                                
		error = vnode_getwithvid(ovp, oxp_vid);
		if (error == 0) {
			*vpp = ovp;
		}
	}
	return error;
}

  
                         
   
void
bind_hashrem(struct bind_node * xp)
{
	if (xp->bind_flags & BIND_FLAG_HASHED) {
		lck_mtx_lock(&bind_hashmtx);
		LIST_REMOVE(xp, bind_hash);
		lck_mtx_unlock(&bind_hashmtx);
	}
}

static struct bind_node *
bind_nodecreate(struct vnode * lowervp)
{
	struct bind_node * xp;

	MALLOC(xp, struct bind_node *, sizeof(struct bind_node), M_TEMP, M_WAITOK | M_ZERO);
	if (xp != NULL) {
		if (lowervp) {
			xp->bind_lowervp  = lowervp;
			xp->bind_lowervid = vnode_vid(lowervp);
		}
	}
	return xp;
}

                                                                   
int
bind_getnewvnode(
	struct mount * mp, struct vnode * lowervp, struct vnode * dvp, struct vnode ** vpp, struct componentname * cnp, int root)
{
	struct vnode_fsparam vnfs_param;
	int error             = 0;
	enum vtype type       = VDIR;
	struct bind_node * xp = bind_nodecreate(lowervp);

	if (xp == NULL) {
		return ENOMEM;
	}

	if (lowervp) {
		type = vnode_vtype(lowervp);
	}

	vnfs_param.vnfs_mp         = mp;
	vnfs_param.vnfs_vtype      = type;
	vnfs_param.vnfs_str        = "bindfs";
	vnfs_param.vnfs_dvp        = dvp;
	vnfs_param.vnfs_fsnode     = (void *)xp;
	vnfs_param.vnfs_vops       = bindfs_vnodeop_p;
	vnfs_param.vnfs_markroot   = root;
	vnfs_param.vnfs_marksystem = 0;
	vnfs_param.vnfs_rdev       = 0;
	vnfs_param.vnfs_filesize   = 0;                                                                     
	vnfs_param.vnfs_cnp        = cnp;
	vnfs_param.vnfs_flags      = VNFS_ADDFSREF;

	error = vnode_create(VNCREATE_FLAVOR, VCREATESIZE, &vnfs_param, vpp);
	if (error == 0) {
		xp->bind_vnode = *vpp;
		xp->bind_myvid = vnode_vid(*vpp);
		vnode_settag(*vpp, VT_BINDFS);
	} else {
		FREE(xp, M_TEMP);
	}
	return error;
}

  
                                          
                                                     
  
                                                              
   
int
bind_nodeget(
	struct mount * mp, struct vnode * lowervp, struct vnode * dvp, struct vnode ** vpp, struct componentname * cnp, int root)
{
	struct vnode * vp;
	int error;

	                              
	error = bind_hashget(mp, lowervp, vpp);
	                                                                              
                
	if (error != ENOENT) {
		                                                                         
                                
		                                                         
                                                                             
		return error;
	}

	  
                                                                 
                                                    
    
	error = vnode_ref(lowervp);                                                                    
	if (error) {
		                                                                                  
		return error;
	}

	error = bind_getnewvnode(mp, lowervp, dvp, &vp, cnp, root);

	if (error) {
		vnode_rele(lowervp);
		return error;
	}

	  
                                                                 
                                        
    
	error = bind_hashins(mp, VTOBIND(vp), vpp);
	if (error || *vpp != NULL) {
		                                                                   
		vnode_recycle(vp);
		vnode_put(vp);
		                                                             
		return error;
	}

	                                             
	*vpp = vp;

	return 0;
}
