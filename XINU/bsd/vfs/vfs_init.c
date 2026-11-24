/*
 * Copyright (c) 2000-2010 Apple Inc. All rights reserved.
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
 * This code is derived from software contributed
 * to Berkeley by John Heidemann of the UCLA Ficus project.
 *
 * Source: * @(#)i405_init.c 2.10 92/04/27 UCLA Ficus project
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
 *	@(#)vfs_init.c	8.5 (Berkeley) 5/11/95
 */
  
                                                                      
                                                                          
                                                                        
               
   


#include <sys/param.h>
#include <sys/mount_internal.h>
#include <sys/time.h>
#include <sys/vm.h>
#include <sys/vnode_internal.h>
#include <sys/stat.h>
#include <sys/namei.h>
#include <sys/ucred.h>
#include <sys/errno.h>
#include <kern/kalloc.h>
#include <sys/decmpfs.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#include <sys/kauth.h>
#endif
#if QUOTA
#include <sys/quota.h>
#endif

  
                                          
   
#if 0
#define DODEBUG(A) A
#else
#define DODEBUG(A)
#endif

ZONE_DECLARE(mount_zone, "mount", sizeof(struct mount), ZC_ZFREE_CLEARMEM);

__private_extern__ void vntblinit(void);

extern const struct vnodeopv_desc *vfs_opv_descs[];
                                       
extern struct vnodeop_desc *vfs_op_descs[];
                                     
  
                                                                
                                                                 
                                                              
                  
   
int vfs_opv_numops;

typedef int (*PFIvp)(void *);

  
                           
                                                          
   
int
vn_default_error(void)
{
	return ENOTSUP;
}

  
             
  
                                           
  
                                                                          
                                                                         
                                                                    
                                                                       
                                                                           
                                                                             
                                                                             
                                                                            
                                                                          
                                                    
   
void
vfs_opv_init(void)
{
	int i, j, k;
	int(***opv_desc_vector_p)(void *);
	int(**opv_desc_vector)(void *);
	const struct vnodeopv_entry_desc *opve_descp;

	  
                                                  
    
	for (i = 0; vfs_opv_descs[i]; i++) {
		opv_desc_vector_p = vfs_opv_descs[i]->opv_desc_vector_p;
		  
                                                  
                                         
     
		if (*opv_desc_vector_p == NULL) {
			*opv_desc_vector_p = kheap_alloc(KHEAP_DEFAULT,
			    vfs_opv_numops * sizeof(PFIvp), Z_WAITOK | Z_ZERO);
			DODEBUG(printf("vector at %x allocated\n",
			    opv_desc_vector_p));
		}
		opv_desc_vector = *opv_desc_vector_p;
		for (j = 0; vfs_opv_descs[i]->opv_desc_ops[j].opve_op; j++) {
			opve_descp = &(vfs_opv_descs[i]->opv_desc_ops[j]);

			                                             
			if (opve_descp->opve_op->vdesc_flags & VDESC_DISABLED) {
				printf("vfs_fsadd: Ignoring reference in %p to disabled operation %s.\n",
				    vfs_opv_descs[i], opve_descp->opve_op->vdesc_name);
				continue;
			}

			  
                                             
                                               
                                             
                                                 
                                                  
                                              
                                      
                                                
     
                                             
                                          
                                              
                                  
                                                 
                                   
      
			if (opve_descp->opve_op->vdesc_offset == 0 &&
			    opve_descp->opve_op !=
			    VDESC(vnop_default)) {
				printf("operation %s not listed in %s.\n",
				    opve_descp->opve_op->vdesc_name,
				    "vfs_op_descs");
				panic("vfs_opv_init: bad operation");
			}
			  
                         
      
			opv_desc_vector[opve_descp->opve_op->vdesc_offset] =
			    opve_descp->opve_impl;
		}
	}
	  
                                                  
                                                       
                                                              
    
	for (i = 0; vfs_opv_descs[i]; i++) {
		opv_desc_vector = *(vfs_opv_descs[i]->opv_desc_vector_p);
		  
                                                             
     
		if (opv_desc_vector[VOFFSET(vnop_default)] == NULL) {
			panic("vfs_opv_init: operation vector without default routine.");
		}
		for (k = 0; k < vfs_opv_numops; k++) {
			if (opv_desc_vector[k] == NULL) {
				opv_desc_vector[k] =
				    opv_desc_vector[VOFFSET(vnop_default)];
			}
		}
	}
}

  
                                             
   
void
vfs_op_init(void)
{
	int i;

	DODEBUG(printf("Vnode_interface_init.\n"));
	  
                                                
    
	for (i = 0; vfs_opv_descs[i]; i++) {
		*(vfs_opv_descs[i]->opv_desc_vector_p) = NULL;
	}
	  
                                                            
                               
    
	for (vfs_opv_numops = 0, i = 0; vfs_op_descs[i]; i++) {
		                                             
		if (vfs_op_descs[i]->vdesc_flags & VDESC_DISABLED) {
			continue;
		}
		vfs_op_descs[i]->vdesc_offset = vfs_opv_numops;
		vfs_opv_numops++;
	}
	DODEBUG(printf("vfs_opv_numops=%d\n", vfs_opv_numops));
}

  
                                                                
   
extern struct vnodeops dead_vnodeops;
extern struct vnodeops spec_vnodeops;

                              
static LCK_GRP_DECLARE(vnode_list_lck_grp, "vnode list");
static LCK_ATTR_DECLARE(vnode_list_lck_attr, 0, 0);
static LCK_SPIN_DECLARE_ATTR(vnode_list_spin_lock,
    &vnode_list_lck_grp, &vnode_list_lck_attr);
static LCK_MTX_DECLARE_ATTR(spechash_mtx_lock,
    &vnode_list_lck_grp, &vnode_list_lck_attr);
LCK_MTX_DECLARE_ATTR(pkg_extensions_lck,
    &vnode_list_lck_grp, &vnode_list_lck_attr);

                         
static LCK_GRP_DECLARE(mnt_lck_grp, "mount");
static LCK_ATTR_DECLARE(mnt_lck_attr, 0, 0);

                              
static LCK_GRP_DECLARE(mnt_list_lck_grp, "mount list");
LCK_MTX_DECLARE(mnt_list_mtx_lock, &mnt_list_lck_grp);

struct mount * dead_mountp;

  
                                                                        
   
void
vfsinit(void)
{
	struct vfstable *vfsp;
	int i, maxtypenum;
	struct mount * mp;

	  
                              
    
	vntblinit();
	  
                                              
    
	vfs_event_init();
	  
                                   
    
	nchinit();

	  
                                  
    
	vfs_op_init();
	vfs_opv_init();                       
	  
                                                        
                                                     
    
	maxtypenum = VT_NON;
	for (vfsp = vfsconf, i = 0; i < maxvfsslots; i++, vfsp++) {
		struct vfsconf vfsc;
		if (vfsp->vfc_vfsops == (struct vfsops *)0) {
			break;
		}
		if (i) {
			vfsconf[i - 1].vfc_next = vfsp;
		}
		if (maxtypenum <= vfsp->vfc_typenum) {
			maxtypenum = vfsp->vfc_typenum + 1;
		}

		bzero(&vfsc, sizeof(struct vfsconf));
		vfsc.vfc_reserved1 = 0;
		bcopy(vfsp->vfc_name, vfsc.vfc_name, sizeof(vfsc.vfc_name));
		vfsc.vfc_typenum = vfsp->vfc_typenum;
		vfsc.vfc_refcount = vfsp->vfc_refcount;
		vfsc.vfc_flags = vfsp->vfc_flags;
		vfsc.vfc_reserved2 = 0;
		vfsc.vfc_reserved3 = 0;

		if (vfsp->vfc_vfsops->vfs_sysctl) {
			struct sysctl_oid *oidp = NULL;
			struct sysctl_oid oid = SYSCTL_STRUCT_INIT(_vfs, vfsp->vfc_typenum, , CTLTYPE_NODE | CTLFLAG_KERN | CTLFLAG_RW | CTLFLAG_LOCKED, NULL, 0, vfs_sysctl_node, "-", "");

			oidp = kheap_alloc(KHEAP_DEFAULT, sizeof(struct sysctl_oid), Z_WAITOK);
			*oidp = oid;

			                                                 
			vfsp->vfc_sysctl = oidp;
			oidp->oid_name = vfsp->vfc_name;
			sysctl_register_oid(vfsp->vfc_sysctl);
		}

		(*vfsp->vfc_vfsops->vfs_init)(&vfsc);

		numused_vfsslots++;
		numregistered_fses++;
	}
	                                 
	maxvfstypenum = maxtypenum;

	  
                                            
    
	vnode_authorize_init();

	  
                                        
    
	mp = zalloc_flags(mount_zone, Z_WAITOK | Z_ZERO);
	                                           
	mp->mnt_maxreadcnt = mp->mnt_maxwritecnt = MAXPHYS;
	mp->mnt_segreadcnt = mp->mnt_segwritecnt = 32;
	mp->mnt_maxsegreadsize = mp->mnt_maxreadcnt;
	mp->mnt_maxsegwritesize = mp->mnt_maxwritecnt;
	mp->mnt_devblocksize = DEV_BSIZE;
	mp->mnt_alignmentmask = PAGE_MASK;
	mp->mnt_ioqueue_depth = MNT_DEFAULT_IOQUEUE_DEPTH;
	mp->mnt_ioscale = 1;
	mp->mnt_ioflags = 0;
	mp->mnt_realrootvp = NULLVP;
	mp->mnt_authcache_ttl = CACHED_LOOKUP_RIGHT_TTL;

	TAILQ_INIT(&mp->mnt_vnodelist);
	TAILQ_INIT(&mp->mnt_workerqueue);
	TAILQ_INIT(&mp->mnt_newvnodes);
	mp->mnt_flag = MNT_LOCAL;
	mp->mnt_lflag = MNT_LDEAD;
	mount_lock_init(mp);

#if CONFIG_MACF
	mac_mount_label_init(mp);
	mac_mount_label_associate(vfs_context_kernel(), mp);
#endif
	dead_mountp = mp;

#if FS_COMPRESSION
	decmpfs_init();
#endif

	nspace_resolver_init();
}

void
vnode_list_lock(void)
{
	lck_spin_lock_grp(&vnode_list_spin_lock, &vnode_list_lck_grp);
}

void
vnode_list_unlock(void)
{
	lck_spin_unlock(&vnode_list_spin_lock);
}

void
mount_list_lock(void)
{
	lck_mtx_lock(&mnt_list_mtx_lock);
}

void
mount_list_unlock(void)
{
	lck_mtx_unlock(&mnt_list_mtx_lock);
}

void
mount_lock_init(mount_t mp)
{
	lck_mtx_init(&mp->mnt_mlock, &mnt_lck_grp, &mnt_lck_attr);
	lck_mtx_init(&mp->mnt_iter_lock, &mnt_lck_grp, &mnt_lck_attr);
	lck_mtx_init(&mp->mnt_renamelock, &mnt_lck_grp, &mnt_lck_attr);
	lck_rw_init(&mp->mnt_rwlock, &mnt_lck_grp, &mnt_lck_attr);
}

void
mount_lock_destroy(mount_t mp)
{
	lck_mtx_destroy(&mp->mnt_mlock, &mnt_lck_grp);
	lck_mtx_destroy(&mp->mnt_iter_lock, &mnt_lck_grp);
	lck_mtx_destroy(&mp->mnt_renamelock, &mnt_lck_grp);
	lck_rw_destroy(&mp->mnt_rwlock, &mnt_lck_grp);
}


  
                     
  
                                                                 
                                                      
          
  
                                           
  
                      
               
  
                                                           
                                                     
                                                     
                                                        
  
                                                           
   
struct vfstable *
vfstable_add(struct vfstable  *nvfsp)
{
	int slot;
	struct vfstable *slotp, *allocated = NULL;
	struct sysctl_oid *oidp = NULL;


	if (nvfsp->vfc_vfsops->vfs_sysctl) {
		struct sysctl_oid oid = SYSCTL_STRUCT_INIT(_vfs, nvfsp->vfc_typenum, , CTLTYPE_NODE | CTLFLAG_KERN | CTLFLAG_RW | CTLFLAG_LOCKED, NULL, 0, vfs_sysctl_node, "-", "");

		oidp = kheap_alloc(KHEAP_DEFAULT, sizeof(struct sysctl_oid), Z_WAITOK);
		*oidp = oid;
	}

	  
                                                             
                                                            
                                         
    
findslot:
	mount_list_lock();
	for (slot = 0; slot < maxvfsslots; slot++) {
		if (vfsconf[slot].vfc_vfsops == NULL) {
			break;
		}
	}
	if (slot == maxvfsslots) {
		if (allocated == NULL) {
			mount_list_unlock();
			                                               
			allocated = kheap_alloc(KHEAP_DEFAULT, sizeof(struct vfstable),
			    Z_WAITOK);
			goto findslot;
		} else {
			slotp = allocated;
		}
	} else {
		slotp = &vfsconf[slot];
	}

	  
                                                                 
                          
   
                                                          
                                                          
    
	bcopy(nvfsp, slotp, sizeof(struct vfstable));
	if (slot != 0) {
		slotp->vfc_next = vfsconf[slot - 1].vfc_next;
		vfsconf[slot - 1].vfc_next = slotp;
	} else {
		slotp->vfc_next = NULL;
	}

	if (slotp != allocated) {
		                                      
		numused_vfsslots++;
	}
	numregistered_fses++;

	if (oidp) {
		                                                       
		slotp->vfc_sysctl = oidp;
		oidp->oid_name = slotp->vfc_name;
		sysctl_register_oid(slotp->vfc_sysctl);
	}

	mount_list_unlock();

	if (allocated && allocated != slotp) {
		                                                    
		kheap_free(KHEAP_DEFAULT, allocated, sizeof(struct vfstable));
	}

	return slotp;
}

  
                     
  
                                                                  
                                                  
  
                                            
  
                      
               
  
                                                      
   
int
vfstable_del(struct vfstable  * vtbl)
{
	struct vfstable **vcpp;
	struct vfstable *vcdelp;

#if DEBUG
	lck_mtx_assert(&mnt_list_mtx_lock, LCK_MTX_ASSERT_OWNED);
#endif            

	  
                                                       
                                                           
               
    
	for (vcpp = &vfsconf; *vcpp; vcpp = &(*vcpp)->vfc_next) {
		if (*vcpp == vtbl) {
			break;
		}
	}

	if (*vcpp == NULL) {
		return ESRCH;                               
	}
	if ((*vcpp)->vfc_sysctl) {
		sysctl_unregister_oid((*vcpp)->vfc_sysctl);
		(*vcpp)->vfc_sysctl->oid_name = NULL;
		kheap_free(KHEAP_DEFAULT, (*vcpp)->vfc_sysctl, sizeof(struct sysctl_oid));
	}

	                  
	vcdelp = *vcpp;
	*vcpp = (*vcpp)->vfc_next;

	  
                                                           
                                                            
                                                                  
    
	if (vcdelp >= vfsconf && vcdelp < (vfsconf + maxvfsslots)) {           
		                                    
		bzero(vcdelp, sizeof(struct vfstable));
		numregistered_fses--;
		numused_vfsslots--;
	} else {                                                               
		  
                                                           
                                                     
                                                        
                                                      
     
		numregistered_fses--;
		mount_list_unlock();
		kheap_free(KHEAP_DEFAULT, vcdelp, sizeof(struct vfstable));
		mount_list_lock();
	}

#if DEBUG
	lck_mtx_assert(&mnt_list_mtx_lock, LCK_MTX_ASSERT_OWNED);
#endif            

	return 0;
}

void
SPECHASH_LOCK(void)
{
	lck_mtx_lock(&spechash_mtx_lock);
}

void
SPECHASH_UNLOCK(void)
{
	lck_mtx_unlock(&spechash_mtx_lock);
}
