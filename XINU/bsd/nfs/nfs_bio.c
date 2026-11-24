/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
 *	@(#)nfs_bio.c	8.9 (Berkeley) 3/30/95
 * FreeBSD-Id: nfs_bio.c,v 1.44 1997/09/10 19:52:25 phk Exp $
 */

#include <nfs/nfs_conf.h>
#if CONFIG_NFS_CLIENT

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/dirent.h>
#include <sys/mount_internal.h>
#include <sys/kernel.h>
#include <sys/ubc_internal.h>
#include <sys/uio_internal.h>
#include <sys/kpi_mbuf.h>

#include <sys/vm.h>
#include <sys/vmparam.h>

#include <sys/time.h>
#include <kern/clock.h>
#include <libkern/OSAtomic.h>
#include <kern/kalloc.h>
#include <kern/thread_call.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfs_gss.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsnode.h>
#include <sys/buf_internal.h>
#include <libkern/OSAtomic.h>
#include <os/refcnt.h>

#define NFS_BIO_DBG(...) NFS_DBG(NFS_FAC_BIO, 7, ## __VA_ARGS__)

kern_return_t   thread_terminate(thread_t);          

#define NFSBUFHASH(np, lbn)     \
	(&nfsbufhashtbl[((long)(np) / sizeof(*(np)) + (int)(lbn)) & nfsbufhash])
LIST_HEAD(nfsbufhashhead, nfsbuf) * nfsbufhashtbl;
struct nfsbuffreehead nfsbuffree, nfsbuffreemeta, nfsbufdelwri;
u_long nfsbufhash;
int nfsbufcnt, nfsbufmin, nfsbufmax, nfsbufmetacnt, nfsbufmetamax;
int nfsbuffreecnt, nfsbuffreemetacnt, nfsbufdelwricnt, nfsneedbuffer;
int nfs_nbdwrite;
int nfs_buf_timer_on = 0;
thread_t nfsbufdelwrithd = NULL;

static ZONE_DECLARE(nfsbuf_zone, "NFS bio", sizeof(struct nfsbuf), ZC_NONE);

static LCK_GRP_DECLARE(nfs_buf_lck_grp, "nfs buf");
LCK_MTX_DECLARE(nfs_buf_mutex, &nfs_buf_lck_grp);

#define NFSBUF_FREE_PERIOD      30                   
#define NFSBUF_LRU_STALE        120
#define NFSBUF_META_STALE       240

                                                                                    
#define LRU_TO_FREEUP                   6
                                                                                        
#define META_TO_FREEUP                  3
                                                                     
#define TOTAL_TO_FREEUP                 (LRU_TO_FREEUP+META_TO_FREEUP)
                                                                                                             
#define LRU_FREEUP_FRAC_ON_TIMER        8
                                                                                                                 
#define META_FREEUP_FRAC_ON_TIMER       16
                                                                                                          
#define LRU_FREEUP_MIN_FRAC             4
                                                                                                              
#define META_FREEUP_MIN_FRAC            2

#define NFS_ROUND_BLOCK(p, blksize)         ((((uint64_t)(p) + blksize - 1) & ~((uint64_t)blksize - 1)) / blksize)

#define NFS_BUF_FREEUP() \
	do { \
	                                                       \
	        if (((nfsbuffreecnt > nfsbufcnt/LRU_FREEUP_MIN_FRAC) || \
	             (nfsbuffreemetacnt > nfsbufcnt/META_FREEUP_MIN_FRAC)) && \
	            ((nfsbufcnt - TOTAL_TO_FREEUP) > nfsbufmin)) \
	                nfs_buf_freeup(0); \
	} while (0)

void
nfs_buf_pgs_get_page_mask(nfsbufpgs *nfsbp, off_t page)
{
	off_t page_pos = page / NBPGS_ELEMENT_PAGES;
	off_t max_page = NBPGS_STRUCT_SIZE * 8;
	NBPGS_ERASE(nfsbp);

	if (page >= max_page) {
		nfs_buf_pgs_bit_not(nfsbp);
		return;
	}

	NBPGS_SET(nfsbp, page);
	nfsbp->pages[page_pos]--;
	for (off_t i = page_pos - 1; i >= 0; i--) {
		nfsbp->pages[i] = ~0;
	}
}

void
nfs_buf_pgs_bit_not(nfsbufpgs *nfsbp)
{
	for (uint32_t i = 0; i < NBPGS_ELEMENTS; i++) {
		nfsbp->pages[i] = ~nfsbp->pages[i];
	}
}

void
nfs_buf_pgs_bit_and(nfsbufpgs *nfsbp_src1, nfsbufpgs *nfsbp_src2, nfsbufpgs *nfsbp_dst)
{
	for (uint32_t i = 0; i < NBPGS_ELEMENTS; i++) {
		nfsbp_dst->pages[i] = nfsbp_src1->pages[i] & nfsbp_src2->pages[i];
	}
}

void
nfs_buf_pgs_set_pages_between(nfsbufpgs *nfsbp, off_t firstpg, off_t lastpg)
{
	nfsbufpgs pagemaskfirst, pagemasklast;

	nfs_buf_pgs_get_page_mask(&pagemasklast, lastpg);
	nfs_buf_pgs_get_page_mask(&pagemaskfirst, firstpg);
	nfs_buf_pgs_bit_not(&pagemaskfirst);
	nfs_buf_pgs_bit_and(&pagemaskfirst, &pagemasklast, nfsbp);
}

int
nfs_buf_pgs_is_set(nfsbufpgs *nfsbp)
{
	for (uint32_t i = 0; i < NBPGS_ELEMENTS; i++) {
		if (nfsbp->pages[i] != 0) {
			return 1;
		}
	}
	return 0;
}

  
                          
   
void
nfs_nbinit(void)
{
	nfsbufcnt = nfsbufmetacnt =
	    nfsbuffreecnt = nfsbuffreemetacnt = nfsbufdelwricnt = 0;
	nfsbufmin = 128;
	                                                                         
	nfsbufmax = (int)(sane_size >> PAGE_SHIFT) / (2 * (NFS_RWSIZE >> PAGE_SHIFT));
	nfsbufmetamax = nfsbufmax / 4;
	nfsneedbuffer = 0;
	nfs_nbdwrite = 0;

	nfsbufhashtbl = hashinit(nfsbufmax / 4, M_NFSBIO, &nfsbufhash);
	TAILQ_INIT(&nfsbuffree);
	TAILQ_INIT(&nfsbuffreemeta);
	TAILQ_INIT(&nfsbufdelwri);
}

  
                                               
   
void
nfs_buf_timer(__unused void *param0, __unused void *param1)
{
	nfs_buf_freeup(1);

	lck_mtx_lock(&nfs_buf_mutex);
	if (nfsbufcnt <= nfsbufmin) {
		nfs_buf_timer_on = 0;
		lck_mtx_unlock(&nfs_buf_mutex);
		return;
	}
	lck_mtx_unlock(&nfs_buf_mutex);

	nfs_interval_timer_start(nfs_buf_timer_call,
	    NFSBUF_FREE_PERIOD * 1000);
}

  
                                             
   
void
nfs_buf_freeup(int timer)
{
	struct nfsbuf *fbp;
	struct timeval now;
	int count;
	struct nfsbuffreehead nfsbuffreeup;

	TAILQ_INIT(&nfsbuffreeup);

	lck_mtx_lock(&nfs_buf_mutex);

	microuptime(&now);

	FSDBG(320, nfsbufcnt, nfsbuffreecnt, nfsbuffreemetacnt, 0);

	count = timer ? nfsbuffreecnt / LRU_FREEUP_FRAC_ON_TIMER : LRU_TO_FREEUP;
	while ((nfsbufcnt > nfsbufmin) && (count-- > 0)) {
		fbp = TAILQ_FIRST(&nfsbuffree);
		if (!fbp) {
			break;
		}
		if (os_ref_get_count(&fbp->nb_refs) > 1) {
			break;
		}
		if (NBUFSTAMPVALID(fbp) &&
		    (fbp->nb_timestamp + (2 * NFSBUF_LRU_STALE)) > now.tv_sec) {
			break;
		}
		nfs_buf_remfree(fbp);
		                                          
		if (fbp->nb_np) {
			if (fbp->nb_vnbufs.le_next != NFSNOLIST) {
				LIST_REMOVE(fbp, nb_vnbufs);
				fbp->nb_vnbufs.le_next = NFSNOLIST;
			}
			fbp->nb_np = NULL;
		}
		LIST_REMOVE(fbp, nb_hash);
		TAILQ_INSERT_TAIL(&nfsbuffreeup, fbp, nb_free);
		nfsbufcnt--;
	}

	count = timer ? nfsbuffreemetacnt / META_FREEUP_FRAC_ON_TIMER : META_TO_FREEUP;
	while ((nfsbufcnt > nfsbufmin) && (count-- > 0)) {
		fbp = TAILQ_FIRST(&nfsbuffreemeta);
		if (!fbp) {
			break;
		}
		if (os_ref_get_count(&fbp->nb_refs) > 1) {
			break;
		}
		if (NBUFSTAMPVALID(fbp) &&
		    (fbp->nb_timestamp + (2 * NFSBUF_META_STALE)) > now.tv_sec) {
			break;
		}
		nfs_buf_remfree(fbp);
		                                          
		if (fbp->nb_np) {
			if (fbp->nb_vnbufs.le_next != NFSNOLIST) {
				LIST_REMOVE(fbp, nb_vnbufs);
				fbp->nb_vnbufs.le_next = NFSNOLIST;
			}
			fbp->nb_np = NULL;
		}
		LIST_REMOVE(fbp, nb_hash);
		TAILQ_INSERT_TAIL(&nfsbuffreeup, fbp, nb_free);
		nfsbufcnt--;
		nfsbufmetacnt--;
	}

	FSDBG(320, nfsbufcnt, nfsbuffreecnt, nfsbuffreemetacnt, 0);
	NFSBUFCNTCHK();

	lck_mtx_unlock(&nfs_buf_mutex);

	while ((fbp = TAILQ_FIRST(&nfsbuffreeup))) {
		TAILQ_REMOVE(&nfsbuffreeup, fbp, nb_free);
		                    
		if (IS_VALID_CRED(fbp->nb_rcred)) {
			kauth_cred_unref(&fbp->nb_rcred);
		}
		if (IS_VALID_CRED(fbp->nb_wcred)) {
			kauth_cred_unref(&fbp->nb_wcred);
		}
		                                     
		if (ISSET(fbp->nb_flags, NB_META) && fbp->nb_data) {
			kheap_free(KHEAP_DATA_BUFFERS, fbp->nb_data, fbp->nb_bufsize);
		}
		NFS_ZFREE(nfsbuf_zone, fbp);
	}
}

  
                                    
                                           
   
void
nfs_buf_remfree(struct nfsbuf *bp)
{
	if (bp->nb_free.tqe_next == NFSNOLIST) {
		panic("nfsbuf not on free list");
	}
	if (ISSET(bp->nb_flags, NB_DELWRI)) {
		nfsbufdelwricnt--;
		TAILQ_REMOVE(&nfsbufdelwri, bp, nb_free);
	} else if (ISSET(bp->nb_flags, NB_META)) {
		nfsbuffreemetacnt--;
		TAILQ_REMOVE(&nfsbuffreemeta, bp, nb_free);
	} else {
		nfsbuffreecnt--;
		TAILQ_REMOVE(&nfsbuffree, bp, nb_free);
	}
	bp->nb_free.tqe_next = NFSNOLIST;
	NFSBUFCNTCHK();
}

  
                                         
   
boolean_t
nfs_buf_is_incore(nfsnode_t np, daddr64_t blkno)
{
	boolean_t rv;
	lck_mtx_lock(&nfs_buf_mutex);
	if (nfs_buf_incore(np, blkno)) {
		rv = TRUE;
	} else {
		rv = FALSE;
	}
	lck_mtx_unlock(&nfs_buf_mutex);
	return rv;
}

  
                                                                
   
struct nfsbuf *
nfs_buf_incore(nfsnode_t np, daddr64_t blkno)
{
	                       
	struct nfsbuf * bp = NFSBUFHASH(np, blkno)->lh_first;
	for (; bp != NULL; bp = bp->nb_hash.le_next) {
		if ((bp->nb_lblkno == blkno) && (bp->nb_np == np)) {
			if (!ISSET(bp->nb_flags, NB_INVAL)) {
				FSDBG(547, bp, blkno, bp->nb_flags, bp->nb_np);
				return bp;
			}
		}
	}
	return NULL;
}

  
                                   
  
                                                                
                                                              
                                                               
                                                   
   
int
nfs_buf_page_inval(vnode_t vp, off_t offset)
{
	struct nfsmount *nmp = VTONMP(vp);
	struct nfsbuf *bp;
	int error = 0;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	lck_mtx_lock(&nfs_buf_mutex);
	bp = nfs_buf_incore(VTONFS(vp), (daddr64_t)(offset / nmp->nm_biosize));
	if (!bp) {
		goto out;
	}
	FSDBG(325, bp, bp->nb_flags, bp->nb_dirtyoff, bp->nb_dirtyend);
	if (ISSET(bp->nb_lflags, NBL_BUSY)) {
		error = EBUSY;
		goto out;
	}
	  
                                                    
                                                     
                                                     
    
	if (bp->nb_dirtyend > 0) {
		off_t start = offset - NBOFF(bp);
		if ((bp->nb_dirtyend > start) &&
		    (bp->nb_dirtyoff < (start + PAGE_SIZE))) {
			  
                                             
                                                
                                              
                 
      
			error = EBUSY;
			nfs_buf_remfree(bp);
			TAILQ_INSERT_HEAD(&nfsbufdelwri, bp, nb_free);
			nfsbufdelwricnt++;
			nfs_buf_delwri_push(1);
		}
	}
out:
	lck_mtx_unlock(&nfs_buf_mutex);
	return error;
}

  
                              
                                               
   
int
nfs_buf_upl_setup(struct nfsbuf *bp)
{
	kern_return_t kret;
	upl_t upl;
	int upl_flags;

	if (ISSET(bp->nb_flags, NB_PAGELIST)) {
		return 0;
	}

	upl_flags = UPL_PRECIOUS;
	if (!ISSET(bp->nb_flags, NB_READ)) {
		  
                                                  
                               
     
		upl_flags |= UPL_WILL_MODIFY;
	}
	kret = ubc_create_upl_kernel(NFSTOV(bp->nb_np), NBOFF(bp), bp->nb_bufsize,
	    &upl, NULL, upl_flags, VM_KERN_MEMORY_FILE);
	if (kret == KERN_INVALID_ARGUMENT) {
		                                               
		bp->nb_pagelist = NULL;
		return EINVAL;
	}
	if (kret != KERN_SUCCESS) {
		printf("nfs_buf_upl_setup(): failed to get pagelist %d\n", kret);
		bp->nb_pagelist = NULL;
		return EIO;
	}

	FSDBG(538, bp, NBOFF(bp), bp->nb_bufsize, bp->nb_np);

	bp->nb_pagelist = upl;
	SET(bp->nb_flags, NB_PAGELIST);
	return 0;
}

  
                                            
                                               
   
void
nfs_buf_upl_check(struct nfsbuf *bp)
{
	upl_page_info_t *pl;
	off_t filesize, fileoffset;
	int i, npages;

	if (!ISSET(bp->nb_flags, NB_PAGELIST)) {
		return;
	}

	npages = round_page_32(bp->nb_bufsize) / PAGE_SIZE;
	filesize = ubc_getsize(NFSTOV(bp->nb_np));
	fileoffset = NBOFF(bp);
	if (fileoffset < filesize) {
		SET(bp->nb_flags, NB_CACHE);
	} else {
		CLR(bp->nb_flags, NB_CACHE);
	}

	pl = ubc_upl_pageinfo(bp->nb_pagelist);
	NBPGS_ERASE(&bp->nb_valid);
	NBPGS_ERASE(&bp->nb_dirty);

	for (i = 0; i < npages; i++, fileoffset += PAGE_SIZE_64) {
		                                                               
		if (fileoffset >= filesize) {
			break;
		}
		if (!upl_valid_page(pl, i)) {
			CLR(bp->nb_flags, NB_CACHE);
			continue;
		}
		NBPGVALID_SET(bp, i);
		if (upl_dirty_page(pl, i)) {
			NBPGDIRTY_SET(bp, i);
		}
	}
	fileoffset = NBOFF(bp);
	if (ISSET(bp->nb_flags, NB_CACHE)) {
		bp->nb_validoff = 0;
		bp->nb_validend = bp->nb_bufsize;
		if (fileoffset + bp->nb_validend > filesize) {
			bp->nb_validend = filesize - fileoffset;
		}
	} else {
		bp->nb_validoff = bp->nb_validend = -1;
	}
	FSDBG(539, bp, fileoffset, bp->nb_valid, bp->nb_dirty);
	FSDBG(539, bp->nb_validoff, bp->nb_validend, bp->nb_dirtyoff, bp->nb_dirtyend);
}

  
                                    
                                               
   
int
nfs_buf_map(struct nfsbuf *bp)
{
	kern_return_t kret;

	if (bp->nb_data) {
		return 0;
	}
	if (!ISSET(bp->nb_flags, NB_PAGELIST)) {
		return EINVAL;
	}

	kret = ubc_upl_map(bp->nb_pagelist, (vm_offset_t *)&(bp->nb_data));
	if (kret != KERN_SUCCESS) {
		panic("nfs_buf_map: ubc_upl_map() failed with (%d)", kret);
	}
	if (bp->nb_data == 0) {
		panic("ubc_upl_map mapped 0");
	}
	FSDBG(540, bp, bp->nb_flags, NBOFF(bp), bp->nb_data);
	return 0;
}

  
                                    
  
                                                                
                                                             
                                                           
                                                       
                          
   
void
nfs_buf_normalize_valid_range(nfsnode_t np, struct nfsbuf *bp)
{
	off_t pg, npg;
	                                                                
	pg = bp->nb_validoff / PAGE_SIZE;
	while (pg >= 0 && NBPGVALID(bp, pg)) {
		pg--;
	}
	bp->nb_validoff = (pg + 1) * PAGE_SIZE;
	                                                                 
	npg = bp->nb_bufsize / PAGE_SIZE;
	pg = bp->nb_validend / PAGE_SIZE;
	while (pg < npg && NBPGVALID(bp, pg)) {
		pg++;
	}
	bp->nb_validend = pg * PAGE_SIZE;
	                 
	if (NBOFF(bp) + bp->nb_validend > (off_t)np->n_size) {
		bp->nb_validend = np->n_size % bp->nb_bufsize;
	}
}

  
                                                  
                                           
   
void
nfs_buf_delwri_service(void)
{
	struct nfsbuf *bp;
	nfsnode_t np;
	int error, i = 0;

	while (i < 8 && (bp = TAILQ_FIRST(&nfsbufdelwri)) != NULL) {
		np = bp->nb_np;
		nfs_buf_remfree(bp);
		nfs_buf_refget(bp);
		while ((error = nfs_buf_acquire(bp, 0, 0, 0)) == EAGAIN) {
			;
		}
		nfs_buf_refrele(bp);
		if (error) {
			break;
		}
		if (!bp->nb_np) {
			                               
			nfs_buf_drop(bp);
			continue;
		}
		if (ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
			nfs_buf_check_write_verifier(np, bp);
		}
		if (ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
			                                      
			TAILQ_INSERT_TAIL(&nfsbufdelwri, bp, nb_free);
			nfsbufdelwricnt++;
			nfs_buf_drop(bp);
			lck_mtx_unlock(&nfs_buf_mutex);
			nfs_flushcommits(np, 1);
		} else {
			SET(bp->nb_flags, NB_ASYNC);
			lck_mtx_unlock(&nfs_buf_mutex);
			nfs_buf_write(bp);
		}
		i++;
		lck_mtx_lock(&nfs_buf_mutex);
	}
}

  
                                                       
   
void
nfs_buf_delwri_thread(__unused void *arg, __unused wait_result_t wr)
{
	struct timespec ts = { .tv_sec = 30, .tv_nsec = 0 };
	int error = 0;

	lck_mtx_lock(&nfs_buf_mutex);
	while (!error) {
		nfs_buf_delwri_service();
		error = msleep(&nfsbufdelwrithd, &nfs_buf_mutex, 0, "nfsbufdelwri", &ts);
	}
	nfsbufdelwrithd = NULL;
	lck_mtx_unlock(&nfs_buf_mutex);
	thread_terminate(nfsbufdelwrithd);
}

  
                                                  
                                                             
   
void
nfs_buf_delwri_push(int locked)
{
	if (TAILQ_EMPTY(&nfsbufdelwri)) {
		return;
	}
	if (!locked) {
		lck_mtx_lock(&nfs_buf_mutex);
	}
	                                              
	if (nfsbufdelwrithd) {
		wakeup(&nfsbufdelwrithd);
	} else if (kernel_thread_start(nfs_buf_delwri_thread, NULL, &nfsbufdelwrithd) == KERN_SUCCESS) {
		thread_deallocate(nfsbufdelwrithd);
	}
	                                                     
	if (!nfsbufdelwrithd) {
		nfs_buf_delwri_service();
	}
	if (!locked) {
		lck_mtx_unlock(&nfs_buf_mutex);
	}
}

  
                     
  
                                       
                                  
  
                                                                  
                                                                        
  
                                          
                                                           
                                                                     
                             
  
                                                        
  
                                                        
                                              
   
int
nfs_buf_get(
	nfsnode_t np,
	daddr64_t blkno,
	uint32_t size,
	thread_t thd,
	int flags,
	struct nfsbuf **bpp)
{
	vnode_t vp = NFSTOV(np);
	struct nfsmount *nmp = VTONMP(vp);
	struct nfsbuf *bp;
	uint32_t bufsize;
	int slpflag = PCATCH;
	int operation = (flags & NBLK_OPMASK);
	int error = 0;
	struct timespec ts;

	FSDBG_TOP(541, np, blkno, size, flags);
	*bpp = NULL;

	bufsize = size;
	if (bufsize > NFS_MAXBSIZE) {
		panic("nfs_buf_get: buffer larger than NFS_MAXBSIZE requested");
	}

	if (nfs_mount_gone(nmp)) {
		FSDBG_BOT(541, np, blkno, 0, ENXIO);
		return ENXIO;
	}

	if (!UBCINFOEXISTS(vp)) {
		operation = NBLK_META;
	} else if (bufsize < (uint32_t)nmp->nm_biosize) {
		                                                 
		bufsize = nmp->nm_biosize;
	}

	                                                                  
	if ((operation == NBLK_WRITE) && (nfs_nbdwrite > NFS_A_LOT_OF_DELAYED_WRITES)) {
		FSDBG_TOP(542, np, blkno, nfs_nbdwrite, NFS_A_LOT_OF_DELAYED_WRITES);

		                          
		nfs_buf_delwri_push(0);

		                                       
		tsleep(&nfs_nbdwrite, PCATCH, "nfs_nbdwrite", 1);
		FSDBG_BOT(542, np, blkno, nfs_nbdwrite, NFS_A_LOT_OF_DELAYED_WRITES);
	}

loop:
	lck_mtx_lock(&nfs_buf_mutex);

	                                                           
	while (np->n_bflag & NBINVALINPROG) {
		np->n_bflag |= NBINVALWANT;
		ts.tv_sec = 2;
		ts.tv_nsec = 0;
		msleep(&np->n_bflag, &nfs_buf_mutex, slpflag, "nfs_buf_get_invalwait", &ts);
		if ((error = nfs_sigintr(VTONMP(vp), NULL, thd, 0))) {
			lck_mtx_unlock(&nfs_buf_mutex);
			FSDBG_BOT(541, np, blkno, 0, error);
			return error;
		}
		if (np->n_bflag & NBINVALINPROG) {
			slpflag = 0;
		}
	}

	                                            
	if ((bp = nfs_buf_incore(np, blkno))) {
		                                  
		if (ISSET(bp->nb_lflags, NBL_BUSY)) {
			if (flags & NBLK_NOWAIT) {
				lck_mtx_unlock(&nfs_buf_mutex);
				FSDBG_BOT(541, np, blkno, bp, 0xbcbcbcbc);
				return 0;
			}
			FSDBG_TOP(543, np, blkno, bp, bp->nb_flags);
			SET(bp->nb_lflags, NBL_WANTED);

			ts.tv_sec = 2;
			ts.tv_nsec = 0;
			msleep(bp, &nfs_buf_mutex, slpflag | (PRIBIO + 1) | PDROP,
			    "nfsbufget", (slpflag == PCATCH) ? NULL : &ts);
			slpflag = 0;
			FSDBG_BOT(543, np, blkno, bp, bp->nb_flags);
			if ((error = nfs_sigintr(VTONMP(vp), NULL, thd, 0))) {
				FSDBG_BOT(541, np, blkno, 0, error);
				return error;
			}
			goto loop;
		}
		if (bp->nb_bufsize != bufsize) {
			panic("nfsbuf size mismatch");
		}
		SET(bp->nb_lflags, NBL_BUSY);
		SET(bp->nb_flags, NB_CACHE);
		nfs_buf_remfree(bp);
		                          
		if (ISSET(bp->nb_flags, NB_PAGELIST)) {
			panic("pagelist buffer was not busy");
		}
		goto buffer_setup;
	}

	if (flags & NBLK_ONLYVALID) {
		lck_mtx_unlock(&nfs_buf_mutex);
		FSDBG_BOT(541, np, blkno, 0, 0x0000cace);
		return 0;
	}

	  
                               
                                                  
                                              
                                 
                                     
                                              
                                                           
                                                 
                                                  
    

	if ((operation == NBLK_META) && (nfsbufmetacnt >= nfsbufmetamax)) {
		                                                             
		bp = TAILQ_FIRST(&nfsbuffreemeta);
	} else if ((nfsbufcnt > nfsbufmin) &&
	    (!TAILQ_EMPTY(&nfsbuffree) || !TAILQ_EMPTY(&nfsbuffreemeta))) {
		                                           
		struct nfsbuf *lrubp, *metabp;
		struct timeval now;
		microuptime(&now);

		                                                                
		lrubp = TAILQ_FIRST(&nfsbuffree);
		if (lrubp && (!NBUFSTAMPVALID(lrubp) ||
		    ((lrubp->nb_timestamp + NFSBUF_LRU_STALE) < now.tv_sec))) {
			bp = lrubp;
		}
		metabp = TAILQ_FIRST(&nfsbuffreemeta);
		if (!bp && metabp && (!NBUFSTAMPVALID(metabp) ||
		    ((metabp->nb_timestamp + NFSBUF_META_STALE) < now.tv_sec))) {
			bp = metabp;
		}

		if (!bp && (nfsbufcnt >= nfsbufmax)) {
			                                          
			                                              
			if (!metabp) {
				bp = lrubp;
			} else if (!lrubp) {
				bp = metabp;
			} else {
				time_t lru_stale_time, meta_stale_time;
				lru_stale_time = lrubp->nb_timestamp + NFSBUF_LRU_STALE;
				meta_stale_time = metabp->nb_timestamp + NFSBUF_META_STALE;
				if (lru_stale_time <= meta_stale_time) {
					bp = lrubp;
				} else {
					bp = metabp;
				}
			}
		}
	}

	if (bp) {
		                               
		FSDBG(544, np, blkno, bp, bp->nb_flags);
		nfs_buf_remfree(bp);
		if (ISSET(bp->nb_flags, NB_DELWRI)) {
			panic("nfs_buf_get: delwri");
		}
		SET(bp->nb_lflags, NBL_BUSY);
		                                               
		if (bp->nb_np) {
			if (bp->nb_vnbufs.le_next != NFSNOLIST) {
				LIST_REMOVE(bp, nb_vnbufs);
				bp->nb_vnbufs.le_next = NFSNOLIST;
			}
			bp->nb_np = NULL;
		}
		LIST_REMOVE(bp, nb_hash);
		                                  
		if (IS_VALID_CRED(bp->nb_rcred)) {
			kauth_cred_unref(&bp->nb_rcred);
		}
		if (IS_VALID_CRED(bp->nb_wcred)) {
			kauth_cred_unref(&bp->nb_wcred);
		}
		                                                       
		if (operation == NBLK_META) {
			if (!ISSET(bp->nb_flags, NB_META)) {
				nfsbufmetacnt++;
			}
		} else if (ISSET(bp->nb_flags, NB_META)) {
			if (bp->nb_data) {
				kheap_free(KHEAP_DATA_BUFFERS, bp->nb_data, bp->nb_bufsize);
				bp->nb_data = NULL;
			}
			nfsbufmetacnt--;
		}
		                        
		bp->nb_error = 0;
		bp->nb_validoff = bp->nb_validend = -1;
		bp->nb_dirtyoff = bp->nb_dirtyend = 0;
		NBPGS_ERASE(&bp->nb_valid);
		NBPGS_ERASE(&bp->nb_dirty);
		bp->nb_verf = 0;
	} else {
		                        
		if ((nfsbufcnt < nfsbufmax) &&
		    ((operation != NBLK_META) || (nfsbufmetacnt < nfsbufmetamax))) {
			                          
			bp = zalloc(nfsbuf_zone);
			nfsbufcnt++;

			  
                                             
                                       
      
			if (nfsbufcnt > nfsbufmin && !nfs_buf_timer_on) {
				nfs_buf_timer_on = 1;
				nfs_interval_timer_start(nfs_buf_timer_call,
				    NFSBUF_FREE_PERIOD * 1000);
			}

			if (operation == NBLK_META) {
				nfsbufmetacnt++;
			}
			NFSBUFCNTCHK();
			                 
			bzero(bp, sizeof(*bp));
			os_ref_init(&bp->nb_refs, NULL);

			bp->nb_free.tqe_next = NFSNOLIST;
			bp->nb_validoff = bp->nb_validend = -1;
			FSDBG(545, np, blkno, bp, 0);
		} else {
			                                                  
			FSDBG_TOP(546, np, blkno, nfsbufcnt, nfsbufmax);

			                          
			nfs_buf_delwri_push(1);

			nfsneedbuffer = 1;
			msleep(&nfsneedbuffer, &nfs_buf_mutex, PCATCH | PDROP, "nfsbufget", NULL);
			FSDBG_BOT(546, np, blkno, nfsbufcnt, nfsbufmax);
			if ((error = nfs_sigintr(VTONMP(vp), NULL, thd, 0))) {
				FSDBG_BOT(541, np, blkno, 0, error);
				return error;
			}
			goto loop;
		}
	}

	                   
	SET(bp->nb_lflags, NBL_BUSY);
	bp->nb_flags = 0;
	bp->nb_lblkno = blkno;
	                        
	LIST_INSERT_HEAD(NFSBUFHASH(np, blkno), bp, nb_hash);
	                                       
	bp->nb_np = np;
	LIST_INSERT_HEAD(&np->n_cleanblkhd, bp, nb_vnbufs);

buffer_setup:

	                 
	lck_mtx_unlock(&nfs_buf_mutex);

	switch (operation) {
	case NBLK_META:
		SET(bp->nb_flags, NB_META);
		if ((bp->nb_bufsize != bufsize) && bp->nb_data) {
			kheap_free(KHEAP_DATA_BUFFERS, bp->nb_data, bp->nb_bufsize);
			bp->nb_data = NULL;
			bp->nb_validoff = bp->nb_validend = -1;
			bp->nb_dirtyoff = bp->nb_dirtyend = 0;
			NBPGS_ERASE(&bp->nb_valid);
			NBPGS_ERASE(&bp->nb_dirty);
			CLR(bp->nb_flags, NB_CACHE);
		}
		if (!bp->nb_data) {
			bp->nb_data = kheap_alloc(KHEAP_DATA_BUFFERS,
			    bufsize, Z_WAITOK);
		}
		if (!bp->nb_data) {
			                                             
			                                      
			lck_mtx_lock(&nfs_buf_mutex);
			LIST_REMOVE(bp, nb_vnbufs);
			bp->nb_vnbufs.le_next = NFSNOLIST;
			bp->nb_np = NULL;
			                                                           
			NBUFSTAMPINVALIDATE(bp);
			if (bp->nb_free.tqe_next != NFSNOLIST) {
				panic("nfsbuf on freelist");
			}
			TAILQ_INSERT_HEAD(&nfsbuffree, bp, nb_free);
			nfsbuffreecnt++;
			lck_mtx_unlock(&nfs_buf_mutex);
			FSDBG_BOT(541, np, blkno, 0xb00, ENOMEM);
			return ENOMEM;
		}
		bp->nb_bufsize = bufsize;
		break;

	case NBLK_READ:
	case NBLK_WRITE:
		  
                                                           
                                             
     
		if (operation == NBLK_READ) {
			SET(bp->nb_flags, NB_READ);
		} else {
			CLR(bp->nb_flags, NB_READ);
		}
		if (bufsize < PAGE_SIZE) {
			bufsize = PAGE_SIZE;
		}
		bp->nb_bufsize = bufsize;
		bp->nb_validoff = bp->nb_validend = -1;

		if (UBCINFOEXISTS(vp)) {
			                
			if (nfs_buf_upl_setup(bp)) {
				                          
				                                    
				                                      
				lck_mtx_lock(&nfs_buf_mutex);
				LIST_REMOVE(bp, nb_vnbufs);
				bp->nb_vnbufs.le_next = NFSNOLIST;
				bp->nb_np = NULL;
				                                                           
				NBUFSTAMPINVALIDATE(bp);
				if (bp->nb_free.tqe_next != NFSNOLIST) {
					panic("nfsbuf on freelist");
				}
				TAILQ_INSERT_HEAD(&nfsbuffree, bp, nb_free);
				nfsbuffreecnt++;
				lck_mtx_unlock(&nfs_buf_mutex);
				FSDBG_BOT(541, np, blkno, 0x2bc, EIO);
				return EIO;
			}
			nfs_buf_upl_check(bp);
		}
		break;

	default:
		panic("nfs_buf_get: %d unknown operation", operation);
	}

	*bpp = bp;

	FSDBG_BOT(541, np, blkno, bp, bp->nb_flags);

	return 0;
}

void
nfs_buf_release(struct nfsbuf *bp, int freeup)
{
	nfsnode_t np = bp->nb_np;
	vnode_t vp;
	struct timeval now;
	int wakeup_needbuffer, wakeup_buffer, wakeup_nbdwrite;

	FSDBG_TOP(548, bp, NBOFF(bp), bp->nb_flags, bp->nb_data);
	FSDBG(548, bp->nb_validoff, bp->nb_validend, bp->nb_dirtyoff, bp->nb_dirtyend);
	FSDBG(548, bp->nb_valid, 0, bp->nb_dirty, 0);

	vp = np ? NFSTOV(np) : NULL;
	if (vp && UBCINFOEXISTS(vp) && bp->nb_bufsize) {
		int upl_flags, rv;
		upl_t upl;
		uint32_t i;

		if (!ISSET(bp->nb_flags, NB_PAGELIST) && !ISSET(bp->nb_flags, NB_INVAL)) {
			rv = nfs_buf_upl_setup(bp);
			if (rv) {
				printf("nfs_buf_release: upl create failed %d\n", rv);
			} else {
				nfs_buf_upl_check(bp);
			}
		}
		upl = bp->nb_pagelist;
		if (!upl) {
			goto pagelist_cleanup_done;
		}
		if (bp->nb_data) {
			if (ubc_upl_unmap(upl) != KERN_SUCCESS) {
				panic("ubc_upl_unmap failed");
			}
			bp->nb_data = NULL;
		}
		  
                                                          
                                                          
     
		if (ISSET(bp->nb_flags, NB_ERROR) || (!nfs_buf_pgs_is_set(&bp->nb_dirty) && (ISSET(bp->nb_flags, NB_INVAL) ||
		    (ISSET(bp->nb_flags, NB_NOCACHE) && !ISSET(bp->nb_flags, (NB_NEEDCOMMIT | NB_DELWRI)))))) {
			if (ISSET(bp->nb_flags, (NB_READ | NB_INVAL | NB_NOCACHE))) {
				upl_flags = UPL_ABORT_DUMP_PAGES;
			} else {
				upl_flags = 0;
			}
			ubc_upl_abort(upl, upl_flags);
			goto pagelist_cleanup_done;
		}
		for (i = 0; i <= (bp->nb_bufsize - 1) / PAGE_SIZE; i++) {
			if (!NBPGVALID(bp, i)) {
				ubc_upl_abort_range(upl,
				    i * PAGE_SIZE, PAGE_SIZE,
				    UPL_ABORT_DUMP_PAGES |
				    UPL_ABORT_FREE_ON_EMPTY);
			} else {
				if (NBPGDIRTY(bp, i)) {
					upl_flags = UPL_COMMIT_SET_DIRTY;
				} else {
					upl_flags = UPL_COMMIT_CLEAR_DIRTY;
				}

				if (!ISSET(bp->nb_flags, (NB_NEEDCOMMIT | NB_DELWRI))) {
					upl_flags |= UPL_COMMIT_CLEAR_PRECIOUS;
				}

				ubc_upl_commit_range(upl,
				    i * PAGE_SIZE, PAGE_SIZE,
				    upl_flags |
				    UPL_COMMIT_INACTIVATE |
				    UPL_COMMIT_FREE_ON_EMPTY);
			}
		}
pagelist_cleanup_done:
		                                   
		if (NBOFF(bp) + bp->nb_bufsize > (off_t)(np->n_size)) {
			off_t start, end;
			start = trunc_page_64(np->n_size) + PAGE_SIZE_64;
			end = trunc_page_64(NBOFF(bp) + bp->nb_bufsize);
			if (start < NBOFF(bp)) {
				start = NBOFF(bp);
			}
			if (end > start) {
				if ((rv = ubc_msync(vp, start, end, NULL, UBC_INVALIDATE))) {
					printf("nfs_buf_release(): ubc_msync failed!, error %d\n", rv);
				}
			}
		}
		CLR(bp->nb_flags, NB_PAGELIST);
		bp->nb_pagelist = NULL;
	}

	lck_mtx_lock(&nfs_buf_mutex);

	wakeup_needbuffer = wakeup_buffer = wakeup_nbdwrite = 0;

	                                                                  
	if (nfsneedbuffer) {
		nfsneedbuffer = 0;
		wakeup_needbuffer = 1;
	}
	                                                                     
	if (ISSET(bp->nb_lflags, NBL_WANTED)) {
		CLR(bp->nb_lflags, NBL_WANTED);
		wakeup_buffer = 1;
	}

	                                                                   
	if (ISSET(bp->nb_flags, NB_ERROR) ||
	    (ISSET(bp->nb_flags, NB_NOCACHE) && !ISSET(bp->nb_flags, (NB_NEEDCOMMIT | NB_DELWRI)))) {
		SET(bp->nb_flags, NB_INVAL);
	}

	if ((bp->nb_bufsize <= 0) || ISSET(bp->nb_flags, NB_INVAL)) {
		                                                              
		if (bp->nb_vnbufs.le_next != NFSNOLIST) {
			LIST_REMOVE(bp, nb_vnbufs);
			bp->nb_vnbufs.le_next = NFSNOLIST;
		}
		bp->nb_np = NULL;
		                                                
		                                            
		if (ISSET(bp->nb_flags, NB_DELWRI)) {
			CLR(bp->nb_flags, NB_DELWRI);
			nfs_nbdwrite--;
			NFSBUFCNTCHK();
			wakeup_nbdwrite = 1;
		}
		                                                           
		NBUFSTAMPINVALIDATE(bp);
		                                     
		if (bp->nb_free.tqe_next != NFSNOLIST) {
			panic("nfsbuf on freelist");
		}
		SET(bp->nb_flags, NB_INVAL);
		if (ISSET(bp->nb_flags, NB_META)) {
			TAILQ_INSERT_HEAD(&nfsbuffreemeta, bp, nb_free);
			nfsbuffreemetacnt++;
		} else {
			TAILQ_INSERT_HEAD(&nfsbuffree, bp, nb_free);
			nfsbuffreecnt++;
		}
	} else if (ISSET(bp->nb_flags, NB_DELWRI)) {
		                                      
		if (bp->nb_free.tqe_next != NFSNOLIST) {
			panic("nfsbuf on freelist");
		}
		TAILQ_INSERT_TAIL(&nfsbufdelwri, bp, nb_free);
		nfsbufdelwricnt++;
		freeup = 0;
	} else {
		                            
		microuptime(&now);
		bp->nb_timestamp = now.tv_sec;
		                                    
		if (bp->nb_free.tqe_next != NFSNOLIST) {
			panic("nfsbuf on freelist");
		}
		if (ISSET(bp->nb_flags, NB_META)) {
			TAILQ_INSERT_TAIL(&nfsbuffreemeta, bp, nb_free);
			nfsbuffreemetacnt++;
		} else {
			TAILQ_INSERT_TAIL(&nfsbuffree, bp, nb_free);
			nfsbuffreecnt++;
		}
	}

	NFSBUFCNTCHK();

	                        
	CLR(bp->nb_flags, (NB_ASYNC | NB_STABLE));
	CLR(bp->nb_lflags, NBL_BUSY);

	FSDBG_BOT(548, bp, NBOFF(bp), bp->nb_flags, bp->nb_data);

	lck_mtx_unlock(&nfs_buf_mutex);

	if (wakeup_needbuffer) {
		wakeup(&nfsneedbuffer);
	}
	if (wakeup_buffer) {
		wakeup(bp);
	}
	if (wakeup_nbdwrite) {
		wakeup(&nfs_nbdwrite);
	}
	if (freeup) {
		NFS_BUF_FREEUP();
	}
}

  
                                                 
                                                          
   
int
nfs_buf_iowait(struct nfsbuf *bp)
{
	FSDBG_TOP(549, bp, NBOFF(bp), bp->nb_flags, bp->nb_error);

	lck_mtx_lock(&nfs_buf_mutex);

	while (!ISSET(bp->nb_flags, NB_DONE)) {
		msleep(bp, &nfs_buf_mutex, PRIBIO + 1, "nfs_buf_iowait", NULL);
	}

	lck_mtx_unlock(&nfs_buf_mutex);

	FSDBG_BOT(549, bp, NBOFF(bp), bp->nb_flags, bp->nb_error);

	                                                 
	if (ISSET(bp->nb_flags, NB_EINTR)) {
		CLR(bp->nb_flags, NB_EINTR);
		return EINTR;
	} else if (ISSET(bp->nb_flags, NB_ERROR)) {
		return bp->nb_error ? bp->nb_error : EIO;
	}
	return 0;
}

  
                                 
   
void
nfs_buf_iodone(struct nfsbuf *bp)
{
	FSDBG_TOP(550, bp, NBOFF(bp), bp->nb_flags, bp->nb_error);

	if (ISSET(bp->nb_flags, NB_DONE)) {
		panic("nfs_buf_iodone already");
	}

	if (!ISSET(bp->nb_flags, NB_READ)) {
		CLR(bp->nb_flags, NB_WRITEINPROG);
		  
                                              
                                   
     
		vnode_writedone(NFSTOV(bp->nb_np));
		nfs_node_lock_force(bp->nb_np);
		bp->nb_np->n_numoutput--;
		nfs_node_unlock(bp->nb_np);
	}
	if (ISSET(bp->nb_flags, NB_ASYNC)) {                              
		SET(bp->nb_flags, NB_DONE);                                      
		nfs_buf_release(bp, 1);
	} else {                                                                       
		lck_mtx_lock(&nfs_buf_mutex);
		SET(bp->nb_flags, NB_DONE);                                      
		CLR(bp->nb_lflags, NBL_WANTED);
		lck_mtx_unlock(&nfs_buf_mutex);
		wakeup(bp);
	}

	FSDBG_BOT(550, bp, NBOFF(bp), bp->nb_flags, bp->nb_error);
}

void
nfs_buf_write_delayed(struct nfsbuf *bp)
{
	nfsnode_t np = bp->nb_np;

	FSDBG_TOP(551, bp, NBOFF(bp), bp->nb_flags, 0);
	FSDBG(551, bp, bp->nb_dirtyoff, bp->nb_dirtyend, bp->nb_dirty);

	  
                                         
                                    
                                                        
    
	if (!ISSET(bp->nb_flags, NB_DELWRI)) {
		SET(bp->nb_flags, NB_DELWRI);
		                        
		lck_mtx_lock(&nfs_buf_mutex);
		nfs_nbdwrite++;
		NFSBUFCNTCHK();
		if (bp->nb_vnbufs.le_next != NFSNOLIST) {
			LIST_REMOVE(bp, nb_vnbufs);
		}
		LIST_INSERT_HEAD(&np->n_dirtyblkhd, bp, nb_vnbufs);
		lck_mtx_unlock(&nfs_buf_mutex);
	}

	  
                                                            
                                  
    
	vnode_waitforwrites(NFSTOV(np), VNODE_ASYNC_THROTTLE, 0, 0, "nfs_buf_write_delayed");

	                                                                  
	nfs_node_lock_force(np);
	np->n_flag |= NMODIFIED;
	nfs_node_unlock(np);

	  
                                              
                                            
    
	if (nfs_nbdwrite < 0) {
		panic("nfs_buf_write_delayed: Negative nfs_nbdwrite");
	}
	if (nfs_nbdwrite > NFS_A_LOT_OF_DELAYED_WRITES) {
		                       
		SET(bp->nb_flags, NB_ASYNC);
		nfs_buf_write(bp);
		FSDBG_BOT(551, bp, NBOFF(bp), bp->nb_flags, bp->nb_error);
		return;
	}

	                                                                     
	SET(bp->nb_flags, NB_DONE);
	nfs_buf_release(bp, 1);
	FSDBG_BOT(551, bp, NBOFF(bp), bp->nb_flags, 0);
	return;
}

  
                                                           
                                                          
                       
   
void
nfs_buf_check_write_verifier(nfsnode_t np, struct nfsbuf *bp)
{
	struct nfsmount *nmp;

	if (!ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
		return;
	}

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return;
	}
	if (!ISSET(bp->nb_flags, NB_STALEWVERF) && (bp->nb_verf == nmp->nm_verf)) {
		return;
	}

	                                                      
	CLR(bp->nb_flags, (NB_NEEDCOMMIT | NB_STALEWVERF));
	bp->nb_verf = 0;
	nfs_node_lock_force(np);
	np->n_needcommitcnt--;
	CHECK_NEEDCOMMITCNT(np);
	nfs_node_unlock(np);
}

  
                                                                       
                                           
   
void
nfs_buf_refget(struct nfsbuf *bp)
{
	os_ref_retain_locked(&bp->nb_refs);
}
  
                                  
                                           
   
void
nfs_buf_refrele(struct nfsbuf *bp)
{
	(void) os_ref_release_locked(&bp->nb_refs);
}

  
                                   
                                           
   
errno_t
nfs_buf_acquire(struct nfsbuf *bp, int flags, int slpflag, int slptimeo)
{
	errno_t error;
	struct timespec ts;

	if (ISSET(bp->nb_lflags, NBL_BUSY)) {
		  
                                                 
                                               
                     
     
		if (flags & NBAC_NOWAIT) {
			return EBUSY;
		}
		SET(bp->nb_lflags, NBL_WANTED);

		ts.tv_sec = (slptimeo / 100);
		                                              
		ts.tv_nsec = (slptimeo % 100) * 10  * NSEC_PER_USEC * 1000;

		error = msleep(bp, &nfs_buf_mutex, slpflag | (PRIBIO + 1),
		    "nfs_buf_acquire", &ts);
		if (error) {
			return error;
		}
		return EAGAIN;
	}
	if (flags & NBAC_REMOVE) {
		nfs_buf_remfree(bp);
	}
	SET(bp->nb_lflags, NBL_BUSY);

	return 0;
}

  
                                          
                                           
   
void
nfs_buf_drop(struct nfsbuf *bp)
{
	int need_wakeup = 0;

	if (!ISSET(bp->nb_lflags, NBL_BUSY)) {
		panic("nfs_buf_drop: buffer not busy!");
	}
	if (ISSET(bp->nb_lflags, NBL_WANTED)) {
		                                                           
		need_wakeup = 1;
	}
	                        
	CLR(bp->nb_lflags, (NBL_BUSY | NBL_WANTED));

	if (need_wakeup) {
		wakeup(bp);
	}
}

  
                                                      
                                            
                                           
   
int
nfs_buf_iterprepare(nfsnode_t np, struct nfsbuflists *iterheadp, int flags)
{
	struct nfsbuflists *listheadp;

	if (flags & NBI_DIRTY) {
		listheadp = &np->n_dirtyblkhd;
	} else {
		listheadp = &np->n_cleanblkhd;
	}

	if ((flags & NBI_NOWAIT) && (np->n_bufiterflags & NBI_ITER)) {
		LIST_INIT(iterheadp);
		return EWOULDBLOCK;
	}

	while (np->n_bufiterflags & NBI_ITER) {
		np->n_bufiterflags |= NBI_ITERWANT;
		msleep(&np->n_bufiterflags, &nfs_buf_mutex, 0, "nfs_buf_iterprepare", NULL);
	}
	if (LIST_EMPTY(listheadp)) {
		LIST_INIT(iterheadp);
		return EINVAL;
	}
	np->n_bufiterflags |= NBI_ITER;

	iterheadp->lh_first = listheadp->lh_first;
	listheadp->lh_first->nb_vnbufs.le_prev = &iterheadp->lh_first;
	LIST_INIT(listheadp);

	return 0;
}

  
                                                         
                                            
                                           
   
void
nfs_buf_itercomplete(nfsnode_t np, struct nfsbuflists *iterheadp, int flags)
{
	struct nfsbuflists * listheadp;
	struct nfsbuf *bp;

	if (flags & NBI_DIRTY) {
		listheadp = &np->n_dirtyblkhd;
	} else {
		listheadp = &np->n_cleanblkhd;
	}

	while (!LIST_EMPTY(iterheadp)) {
		bp = LIST_FIRST(iterheadp);
		LIST_REMOVE(bp, nb_vnbufs);
		LIST_INSERT_HEAD(listheadp, bp, nb_vnbufs);
	}

	np->n_bufiterflags &= ~NBI_ITER;
	if (np->n_bufiterflags & NBI_ITERWANT) {
		np->n_bufiterflags &= ~NBI_ITERWANT;
		wakeup(&np->n_bufiterflags);
	}
}


  
                                 
   
int
nfs_buf_read(struct nfsbuf *bp)
{
	int error = 0;
	nfsnode_t np;
	thread_t thd;
	kauth_cred_t cred;

	np = bp->nb_np;
	cred = bp->nb_rcred;
	if (IS_VALID_CRED(cred)) {
		kauth_cred_ref(cred);
	}
	thd = ISSET(bp->nb_flags, NB_ASYNC) ? NULL : current_thread();

	                   
	if (!ISSET(bp->nb_flags, NB_READ)) {
		panic("nfs_buf_read: !NB_READ");
	}
	if (ISSET(bp->nb_flags, NB_DONE)) {
		CLR(bp->nb_flags, NB_DONE);
	}

	NFS_BUF_MAP(bp);

	OSAddAtomic64(1, &nfsstats.read_bios);

	error = nfs_buf_read_rpc(bp, thd, cred);
	  
                                                   
                                                         
    

	if (IS_VALID_CRED(cred)) {
		kauth_cred_unref(&cred);
	}
	return error;
}

  
                                 
   
void
nfs_buf_read_finish(struct nfsbuf *bp)
{
	nfsnode_t np = bp->nb_np;
	struct nfsmount *nmp;

	if (!ISSET(bp->nb_flags, NB_ERROR)) {
		                        
		bp->nb_validoff = 0;
		bp->nb_validend = bp->nb_endio;
		if (bp->nb_endio < bp->nb_bufsize) {
			  
                                                            
                                                            
                                                              
                           
                                                                
                                   
      
			off_t boff = NBOFF(bp);
			if ((off_t)np->n_size >= (boff + bp->nb_bufsize)) {
				bp->nb_validend = bp->nb_bufsize;
			} else if ((off_t)np->n_size >= boff) {
				bp->nb_validend = np->n_size - boff;
			} else {
				bp->nb_validend = 0;
			}
		}
		if ((nmp = NFSTONMP(np)) && (nmp->nm_vers == NFS_VER2) &&
		    ((NBOFF(bp) + bp->nb_validend) > 0x100000000LL)) {
			bp->nb_validend = 0x100000000LL - NBOFF(bp);
		}
		nfs_buf_pgs_get_page_mask(&bp->nb_valid, round_page_64(bp->nb_validend) / PAGE_SIZE);
		if (bp->nb_validend & PAGE_MASK) {
			                                      
			bzero(bp->nb_data + bp->nb_validend, PAGE_SIZE - (bp->nb_validend & PAGE_MASK));
		}
	}
	nfs_buf_iodone(bp);
}

  
                                            
   
int
nfs_buf_read_rpc(struct nfsbuf *bp, thread_t thd, kauth_cred_t cred)
{
	struct nfsmount *nmp;
	nfsnode_t np = bp->nb_np;
	int error = 0, nfsvers, async;
	int offset;
	uint64_t length, nrpcs;
	uint32_t nmrsize;
	size_t len;
	off_t boff;
	struct nfsreq *req;
	struct nfsreq_cbinfo cb;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		bp->nb_error = error = ENXIO;
		SET(bp->nb_flags, NB_ERROR);
		nfs_buf_iodone(bp);
		return error;
	}
	nfsvers = nmp->nm_vers;
	nmrsize = nmp->nm_rsize;

	boff = NBOFF(bp);
	offset = 0;
	length = bp->nb_bufsize;

	if (nfsvers == NFS_VER2) {
		if (boff > 0xffffffffLL) {
			bp->nb_error = error = EFBIG;
			SET(bp->nb_flags, NB_ERROR);
			nfs_buf_iodone(bp);
			return error;
		}
		if ((boff + length - 1) > 0xffffffffLL) {
			length = 0x100000000LL - boff;
		}
	}

	                                                            
	async = (bp->nb_flags & NB_ASYNC);
	cb.rcb_func = async ? nfs_buf_read_rpc_finish : NULL;
	cb.rcb_bp = bp;

	bp->nb_offio = bp->nb_endio = 0;
	bp->nb_rpcs = nrpcs = (length + nmrsize - 1) / nmrsize;
	if (async && (nrpcs > 1)) {
		SET(bp->nb_flags, NB_MULTASYNCRPC);
	} else {
		CLR(bp->nb_flags, NB_MULTASYNCRPC);
	}

	while (length > 0) {
		if (ISSET(bp->nb_flags, NB_ERROR)) {
			error = bp->nb_error;
			break;
		}
		len = (length > nmrsize) ? nmrsize : (uint32_t)length;
		cb.rcb_args.offset = offset;
		cb.rcb_args.length = len;
#if CONFIG_NFS4
		if (nmp->nm_vers >= NFS_VER4) {
			cb.rcb_args.stategenid = nmp->nm_stategenid;
		}
#endif
		req = NULL;
		error = nmp->nm_funcs->nf_read_rpc_async(np, boff + offset, len, thd, cred, &cb, &req);
		if (error) {
			break;
		}
		offset += len;
		length -= len;
		if (async) {
			continue;
		}
		nfs_buf_read_rpc_finish(req);
		if (ISSET(bp->nb_flags, NB_ERROR)) {
			error = bp->nb_error;
			break;
		}
	}

	if (length > 0) {
		  
                                                            
                                                   
     
		bp->nb_error = error;
		SET(bp->nb_flags, NB_ERROR);
		if (ISSET(bp->nb_flags, NB_MULTASYNCRPC)) {
			nrpcs = (length + nmrsize - 1) / nmrsize;
			lck_mtx_lock(&nfs_buf_mutex);
			bp->nb_rpcs -= nrpcs;
			if (bp->nb_rpcs == 0) {
				                                        
				lck_mtx_unlock(&nfs_buf_mutex);
				nfs_buf_iodone(bp);
			} else {
				                                           
				while (bp->nb_rpcs > 0) {
					msleep(&bp->nb_rpcs, &nfs_buf_mutex, 0,
					    "nfs_buf_read_rpc_cancel", NULL);
				}
				lck_mtx_unlock(&nfs_buf_mutex);
			}
		} else {
			nfs_buf_iodone(bp);
		}
	}

	return error;
}

  
                                        
   
void
nfs_buf_read_rpc_finish(struct nfsreq *req)
{
	struct nfsmount *nmp;
	size_t rlen, length;
	struct nfsreq_cbinfo cb;
	struct nfsbuf *bp;
	int error = 0, nfsvers, eof = 0, multasyncrpc, finished;
	off_t offset;
	void *wakeme = NULL;
	struct nfsreq *rreq = NULL;
	nfsnode_t np;
	thread_t thd;
	kauth_cred_t cred;
	uio_t auio;
	char uio_buf[UIO_SIZEOF(1)];

finish:
	np = req->r_np;
	thd = req->r_thread;
	cred = req->r_cred;
	if (IS_VALID_CRED(cred)) {
		kauth_cred_ref(cred);
	}
	cb = req->r_callback;
	bp = cb.rcb_bp;
	if (cb.rcb_func) {                                                                                                  
		nfs_request_ref(req, 0);
	}

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		SET(bp->nb_flags, NB_ERROR);
		bp->nb_error = error = ENXIO;
	}
	if (error || ISSET(bp->nb_flags, NB_ERROR)) {
		                  
		nfs_request_async_cancel(req);
		goto out;
	}

	nfsvers = nmp->nm_vers;
	offset = cb.rcb_args.offset;
	rlen = length = cb.rcb_args.length;

	auio = uio_createwithbuffer(1, NBOFF(bp) + offset, UIO_SYSSPACE,
	    UIO_READ, &uio_buf, sizeof(uio_buf));
	uio_addiov(auio, CAST_USER_ADDR_T(bp->nb_data + offset), length);

	                    
	error = nmp->nm_funcs->nf_read_rpc_async_finish(np, req, auio, &rlen, &eof);
	if ((error == EINPROGRESS) && cb.rcb_func) {
		                             
		if (cb.rcb_func) {
			nfs_request_rele(req);
		}
		if (IS_VALID_CRED(cred)) {
			kauth_cred_unref(&cred);
		}
		return;
	}
#if CONFIG_NFS4
	if ((nmp->nm_vers >= NFS_VER4) && nfs_mount_state_error_should_restart(error) && !ISSET(bp->nb_flags, NB_ERROR)) {
		lck_mtx_lock(&nmp->nm_lock);
		if ((error != NFSERR_OLD_STATEID) && (error != NFSERR_GRACE) && (cb.rcb_args.stategenid == nmp->nm_stategenid)) {
			NP(np, "nfs_buf_read_rpc_finish: error %d @ 0x%llx, 0x%x 0x%x, initiating recovery",
			    error, NBOFF(bp) + offset, cb.rcb_args.stategenid, nmp->nm_stategenid);
			nfs_need_recover(nmp, error);
		}
		lck_mtx_unlock(&nmp->nm_lock);
		if (np->n_flag & NREVOKE) {
			error = EIO;
		} else {
			if (error == NFSERR_GRACE) {
				if (cb.rcb_func) {
					  
                                                                
                                                             
        
					struct timeval now;
					if (req->r_nmrep.nmc_mhead) {
						mbuf_freem(req->r_nmrep.nmc_mhead);
						req->r_nmrep.nmc_mhead = NULL;
					}
					req->r_error = 0;
					microuptime(&now);
					lck_mtx_lock(&req->r_mtx);
					req->r_resendtime = now.tv_sec + 2;
					req->r_xid = 0;                                 
					req->r_flags |= R_RESTART;
					req->r_start = 0;
					nfs_asyncio_resend(req);
					lck_mtx_unlock(&req->r_mtx);
					if (IS_VALID_CRED(cred)) {
						kauth_cred_unref(&cred);
					}
					                                                                      
					return;
				}
				                                                      
				tsleep(&nmp->nm_state, (PZERO - 1), "nfsgrace", 2 * hz);
			}
			if (!(error = nfs_mount_state_wait_for_recovery(nmp))) {
				rlen = 0;
				goto readagain;
			}
		}
	}
#endif
	if (error) {
		SET(bp->nb_flags, NB_ERROR);
		bp->nb_error = error;
		goto out;
	}

	if ((rlen > 0) && (bp->nb_endio < (offset + (int)rlen))) {
		bp->nb_endio = offset + rlen;
	}

	if ((nfsvers == NFS_VER2) || eof || (rlen == 0)) {
		                                             
		off_t rpcrem, eofrem, rem;
		rpcrem = (length - rlen);
		eofrem = np->n_size - (NBOFF(bp) + offset + rlen);
		rem = (rpcrem < eofrem) ? rpcrem : eofrem;
		if (rem > 0) {
			NFS_BZERO(bp->nb_data + offset + rlen, rem);
		}
	} else if ((rlen < length) && !ISSET(bp->nb_flags, NB_ERROR)) {
		  
               
    
                                                      
                                                              
                                                       
     
#if CONFIG_NFS4
readagain:
#endif
		offset += rlen;
		length -= rlen;
		cb.rcb_args.offset = offset;
		cb.rcb_args.length = length;
#if CONFIG_NFS4
		if (nmp->nm_vers >= NFS_VER4) {
			cb.rcb_args.stategenid = nmp->nm_stategenid;
		}
#endif
		error = nmp->nm_funcs->nf_read_rpc_async(np, NBOFF(bp) + offset, length, thd, cred, &cb, &rreq);
		if (!error) {
			if (IS_VALID_CRED(cred)) {
				kauth_cred_unref(&cred);
			}
			if (!cb.rcb_func) {
				                                                         
				req = rreq;
				rreq = NULL;
				goto finish;
			}
			nfs_request_rele(req);
			  
                      
                                         
                                               
      
			return;
		}
		SET(bp->nb_flags, NB_ERROR);
		bp->nb_error = error;
	}

out:
	if (cb.rcb_func) {
		nfs_request_rele(req);
	}
	if (IS_VALID_CRED(cred)) {
		kauth_cred_unref(&cred);
	}

	  
                                             
                                             
   
                                                          
                                                          
                                               
    

	multasyncrpc = ISSET(bp->nb_flags, NB_MULTASYNCRPC);
	if (multasyncrpc) {
		lck_mtx_lock(&nfs_buf_mutex);
	}

	bp->nb_rpcs--;
	finished = (bp->nb_rpcs == 0);

	if (multasyncrpc) {
		lck_mtx_unlock(&nfs_buf_mutex);
	}

	if (finished) {
		if (multasyncrpc) {
			wakeme = &bp->nb_rpcs;
		}
		nfs_buf_read_finish(bp);
		if (wakeme) {
			wakeup(wakeme);
		}
	}
}

  
                       
                                                   
   
int
nfs_buf_readahead(nfsnode_t np, int ioflag, daddr64_t *rabnp, daddr64_t lastrabn, thread_t thd, kauth_cred_t cred)
{
	struct nfsmount *nmp = NFSTONMP(np);
	struct nfsbuf *bp;
	int error = 0;
	uint32_t nra;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (nmp->nm_readahead <= 0) {
		return 0;
	}
	if (*rabnp > lastrabn) {
		return 0;
	}

	for (nra = 0; (nra < nmp->nm_readahead) && (*rabnp <= lastrabn); nra++, *rabnp = *rabnp + 1) {
		                                         
		if ((*rabnp * nmp->nm_biosize) >= (off_t)np->n_size) {
			                                            
			*rabnp = lastrabn;
			break;
		}
		error = nfs_buf_get(np, *rabnp, nmp->nm_biosize, thd, NBLK_READ | NBLK_NOWAIT, &bp);
		if (error) {
			break;
		}
		nfs_node_lock_force(np);
		np->n_lastrahead = *rabnp;
		nfs_node_unlock(np);
		if (!bp) {
			continue;
		}
		if ((ioflag & IO_NOCACHE) && ISSET(bp->nb_flags, NB_CACHE) &&
		    !nfs_buf_pgs_is_set(&bp->nb_dirty) && !ISSET(bp->nb_flags, (NB_DELWRI | NB_NCRDAHEAD))) {
			CLR(bp->nb_flags, NB_CACHE);
			NBPGS_ERASE(&bp->nb_valid);
			bp->nb_validoff = bp->nb_validend = -1;
		}
		if ((bp->nb_dirtyend <= 0) && !nfs_buf_pgs_is_set(&bp->nb_dirty) &&
		    !ISSET(bp->nb_flags, (NB_CACHE | NB_DELWRI))) {
			SET(bp->nb_flags, (NB_READ | NB_ASYNC));
			if (ioflag & IO_NOCACHE) {
				SET(bp->nb_flags, NB_NCRDAHEAD);
			}
			if (!IS_VALID_CRED(bp->nb_rcred) && IS_VALID_CRED(cred)) {
				kauth_cred_ref(cred);
				bp->nb_rcred = cred;
			}
			if ((error = nfs_buf_read(bp))) {
				break;
			}
			continue;
		}
		nfs_buf_release(bp, 1);
	}
	return error;
}

  
                                    
   
int
nfs_bioread(nfsnode_t np, uio_t uio, int ioflag, vfs_context_t ctx)
{
	vnode_t vp = NFSTOV(np);
	struct nfsbuf *bp = NULL;
	struct nfsmount *nmp = VTONMP(vp);
	daddr64_t lbn, rabn = 0, lastrabn, maxrabn = -1;
	off_t diff, on = 0, n = 0;
	int error = 0, n32;
	int nfsvers, biosize, modified, readaheads = 0;
	thread_t thd;
	kauth_cred_t cred;
	int64_t io_resid;

	FSDBG_TOP(514, np, uio_offset(uio), uio_resid(uio), ioflag);

	nfsvers = nmp->nm_vers;
	biosize = nmp->nm_biosize;
	thd = vfs_context_thread(ctx);
	cred = vfs_context_ucred(ctx);

	if (vnode_vtype(vp) != VREG) {
		printf("nfs_bioread: type %x unexpected\n", vnode_vtype(vp));
		FSDBG_BOT(514, np, 0xd1e0016, 0, EINVAL);
		return EINVAL;
	}

	  
                                                                    
                                                                    
                                                      
   
                                                               
                                                                 
                                                                  
                                 
                                                           
                                                             
                                                                 
                                                           
    

	if (ISSET(np->n_flag, NUPDATESIZE)) {
		nfs_data_update_size(np, 0);
	}

	if ((error = nfs_node_lock(np))) {
		FSDBG_BOT(514, np, 0xd1e0222, 0, error);
		return error;
	}

	if (np->n_flag & NNEEDINVALIDATE) {
		np->n_flag &= ~NNEEDINVALIDATE;
		nfs_node_unlock(np);
		error = nfs_vinvalbuf(vp, V_SAVE | V_IGNORE_WRITEERR, ctx, 1);
		if (!error) {
			error = nfs_node_lock(np);
		}
		if (error) {
			FSDBG_BOT(514, np, 0xd1e0322, 0, error);
			return error;
		}
	}

	modified = (np->n_flag & NMODIFIED);
	nfs_node_unlock(np);
	                                                       
	error = nfs_getattr(np, NULL, ctx, modified ? NGA_UNCACHED : NGA_CACHED);
	if (error) {
		FSDBG_BOT(514, np, 0xd1e0004, 0, error);
		return error;
	}

	if (uio_resid(uio) == 0) {
		FSDBG_BOT(514, np, 0xd1e0001, 0, 0);
		return 0;
	}
	if (uio_offset(uio) < 0) {
		FSDBG_BOT(514, np, 0xd1e0002, 0, EINVAL);
		return EINVAL;
	}

	  
                                               
                                             
                       
               
    
	if (nmp->nm_readahead > 0) {
		off_t end = uio_offset(uio) + uio_resid(uio);
		if (end > (off_t)np->n_size) {
			end = np->n_size;
		}
		rabn = uio_offset(uio) / biosize;
		maxrabn = (end - 1) / biosize;
		nfs_node_lock_force(np);
		if (!(ioflag & IO_NOCACHE) &&
		    (!rabn || (rabn == np->n_lastread) || (rabn == (np->n_lastread + 1)))) {
			maxrabn += nmp->nm_readahead;
			if ((maxrabn * biosize) >= (off_t)np->n_size) {
				maxrabn = ((off_t)np->n_size - 1) / biosize;
			}
		}
		if (maxrabn < np->n_lastrahead) {
			np->n_lastrahead = -1;
		}
		if (rabn < np->n_lastrahead) {
			rabn = np->n_lastrahead + 1;
		}
		nfs_node_unlock(np);
	} else {
		rabn = maxrabn = 0;
	}

	do {
		nfs_data_lock(np, NFS_DATA_LOCK_SHARED);
		lbn = uio_offset(uio) / biosize;

		  
                                                                   
                                                                    
                                                 
     
		if ((!(ioflag & IO_NOCACHE) || !readaheads) &&
		    ((uio->uio_segflg == UIO_USERSPACE32 ||
		    uio->uio_segflg == UIO_USERSPACE64 ||
		    uio->uio_segflg == UIO_USERSPACE))) {
			io_resid = uio_resid(uio);
			diff = np->n_size - uio_offset(uio);
			if (diff < io_resid) {
				io_resid = diff;
			}
			if (io_resid > 0) {
				int count = (io_resid > INT_MAX) ? INT_MAX : (int)io_resid;
				error = cluster_copy_ubc_data(vp, uio, &count, 0);
				if (error) {
					nfs_data_unlock(np);
					FSDBG_BOT(514, np, uio_offset(uio), 0xcacefeed, error);
					return error;
				}
			}
			                                                           
			if (lbn != (uio_offset(uio) / biosize)) {
				OSAddAtomic64(NFS_ROUND_BLOCK(uio_offset(uio), biosize) - lbn, &nfsstats.biocache_reads);
				FSDBG(514, np, 0xcacefeed, uio_offset(uio), error);
			}
		}

		lbn = uio_offset(uio) / biosize;
		on = uio_offset(uio) % biosize;
		nfs_node_lock_force(np);
		np->n_lastread = (uio_offset(uio) - 1) / biosize;
		nfs_node_unlock(np);

		if ((uio_resid(uio) <= 0) || (uio_offset(uio) >= (off_t)np->n_size)) {
			nfs_data_unlock(np);
			FSDBG_BOT(514, np, uio_offset(uio), uio_resid(uio), 0xaaaaaaaa);
			return 0;
		}

		                                                 
		if (rabn < lbn) {
			rabn = lbn;
		}
		lastrabn = MIN(maxrabn, lbn + nmp->nm_readahead);
		if (rabn <= lastrabn) {                       
			error = nfs_buf_readahead(np, ioflag, &rabn, lastrabn, thd, cred);
			if (error) {
				nfs_data_unlock(np);
				FSDBG_BOT(514, np, 0xd1e000b, 1, error);
				return error;
			}
			readaheads = 1;
			OSAddAtomic64(rabn - lbn, &nfsstats.biocache_reads);
		} else {
			OSAddAtomic64(1, &nfsstats.biocache_reads);
		}

		  
                                                           
                                         
                                                     
                 
     
again:
		io_resid = uio_resid(uio);
		n = (io_resid > (biosize - on)) ? (biosize - on) : io_resid;
		diff = np->n_size - uio_offset(uio);
		if (diff < n) {
			n = diff;
		}

		error = nfs_buf_get(np, lbn, biosize, thd, NBLK_READ, &bp);
		if (error) {
			nfs_data_unlock(np);
			FSDBG_BOT(514, np, 0xd1e000c, 0, error);
			return error;
		}

		if ((ioflag & IO_NOCACHE) && ISSET(bp->nb_flags, NB_CACHE)) {
			  
                                       
                                     
                                                
                                          
      
			if (nfs_buf_pgs_is_set(&bp->nb_dirty) || (bp->nb_dirtyend > 0)) {
				                                           
				SET(bp->nb_flags, NB_NOCACHE);
				goto flushbuffer;
			}
			if (ISSET(bp->nb_flags, NB_NCRDAHEAD)) {
				CLR(bp->nb_flags, NB_NCRDAHEAD);
				SET(bp->nb_flags, NB_NOCACHE);
			}
		}

		                               
		if (nfs_buf_pgs_is_set(&bp->nb_valid)) {
			                                                      
			off_t pg, firstpg, lastpg, dirtypg;
			dirtypg = firstpg = lastpg = -1;
			pg = on / PAGE_SIZE;
			while (pg <= (on + n - 1) / PAGE_SIZE) {
				if (!NBPGVALID(bp, pg)) {
					if (firstpg < 0) {
						firstpg = pg;
					}
					lastpg = pg;
				} else if (firstpg >= 0 && dirtypg < 0 && NBPGDIRTY(bp, pg)) {
					dirtypg = pg;
				}
				pg++;
			}

			                                                  
			if (firstpg < 0) {
				if (bp->nb_validoff < 0) {
					                                  
					                                     
					bp->nb_validoff = trunc_page_64(on);
					bp->nb_validend = round_page_64(on + n);
					nfs_buf_normalize_valid_range(np, bp);
				}
				goto buffer_ready;
			}

			                                               
			if (((dirtypg > firstpg) && (dirtypg < lastpg)) ||
			    (((firstpg * PAGE_SIZE) < bp->nb_dirtyend) && (((lastpg + 1) * PAGE_SIZE) > bp->nb_dirtyoff))) {
				                                                                
				                                           
flushbuffer:
				CLR(bp->nb_flags, (NB_DONE | NB_ERROR | NB_INVAL));
				SET(bp->nb_flags, NB_ASYNC);
				if (!IS_VALID_CRED(bp->nb_wcred)) {
					kauth_cred_ref(cred);
					bp->nb_wcred = cred;
				}
				error = nfs_buf_write(bp);
				if (error) {
					nfs_data_unlock(np);
					FSDBG_BOT(514, np, 0xd1e000d, 0, error);
					return error;
				}
				goto again;
			}
			if (!nfs_buf_pgs_is_set(&bp->nb_dirty) && bp->nb_dirtyend <= 0 &&
			    (lastpg - firstpg + 1) > (biosize / PAGE_SIZE) / 2) {
				                                                          
				                                                        
				NBPGS_ERASE(&bp->nb_valid);
			} else {
				                            
				uio_t auio;
				char uio_buf[UIO_SIZEOF(1)];

				NFS_BUF_MAP(bp);
				auio = uio_createwithbuffer(1, (NBOFF(bp) + firstpg * PAGE_SIZE_64),
				    UIO_SYSSPACE, UIO_READ, &uio_buf[0], sizeof(uio_buf));
				if (!auio) {
					error = ENOMEM;
				} else {
					NFS_UIO_ADDIOV(auio, CAST_USER_ADDR_T(bp->nb_data + (firstpg * PAGE_SIZE)),
					    ((lastpg - firstpg + 1) * PAGE_SIZE));
					error = nfs_read_rpc(np, auio, ctx);
				}
				if (error) {
					if (ioflag & IO_NOCACHE) {
						SET(bp->nb_flags, NB_NOCACHE);
					}
					nfs_buf_release(bp, 1);
					nfs_data_unlock(np);
					FSDBG_BOT(514, np, 0xd1e000e, 0, error);
					return error;
				}
				                                                               
				bp->nb_validoff = trunc_page_64(on);
				bp->nb_validend = round_page_64(on + n);
				nfs_buf_normalize_valid_range(np, bp);
				if (uio_resid(auio) > 0) {
					                                       
					                                   
					bzero(CAST_DOWN(caddr_t, uio_curriovbase(auio)), uio_resid(auio));
				}
				                                                 
				for (pg = firstpg; pg <= lastpg; pg++) {
					NBPGVALID_SET(bp, pg);
				}
			}
		}
		                                                 
		if (!nfs_buf_pgs_is_set(&bp->nb_valid)) {
			if (!IS_VALID_CRED(bp->nb_rcred) && IS_VALID_CRED(cred)) {
				kauth_cred_ref(cred);
				bp->nb_rcred = cred;
			}
			SET(bp->nb_flags, NB_READ);
			CLR(bp->nb_flags, (NB_DONE | NB_ERROR | NB_INVAL));
			error = nfs_buf_read(bp);
			if (ioflag & IO_NOCACHE) {
				SET(bp->nb_flags, NB_NOCACHE);
			}
			if (error) {
				nfs_data_unlock(np);
				nfs_buf_release(bp, 1);
				FSDBG_BOT(514, np, 0xd1e000f, 0, error);
				return error;
			}
		}
buffer_ready:
		                                                      
		if (bp->nb_validend > 0) {
			diff = (on >= bp->nb_validend) ? 0 : (bp->nb_validend - on);
			if (diff < n) {
				n = diff;
			}
		}
		if (n > 0) {
			NFS_BUF_MAP(bp);
			n32 = n > INT_MAX ? INT_MAX : (int)n;
			error = uiomove(bp->nb_data + on, n32, uio);
			if (!error && n > n32) {
				error = uiomove(bp->nb_data + on + n32, (int)(n - n32), uio);
			}
		}


		nfs_buf_release(bp, 1);
		nfs_data_unlock(np);
		nfs_node_lock_force(np);
		np->n_lastread = (uio_offset(uio) - 1) / biosize;
		nfs_node_unlock(np);
	} while (error == 0 && uio_resid(uio) > 0 && n > 0);
	FSDBG_BOT(514, np, uio_offset(uio), uio_resid(uio), error);
	return error;
}

  
                                                   
   
int
nfs_async_write_start(struct nfsmount *nmp)
{
	int error = 0, slpflag = NMFLAG(nmp, INTR) ? PCATCH : 0;
	struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };

	if (nfs_max_async_writes <= 0) {
		return 0;
	}
	lck_mtx_lock(&nmp->nm_lock);
	while ((nfs_max_async_writes > 0) && (nmp->nm_asyncwrites >= nfs_max_async_writes)) {
		if ((error = nfs_sigintr(nmp, NULL, current_thread(), 1))) {
			break;
		}
		msleep(&nmp->nm_asyncwrites, &nmp->nm_lock, slpflag | (PZERO - 1), "nfsasyncwrites", &ts);
		slpflag = 0;
	}
	if (!error) {
		nmp->nm_asyncwrites++;
	}
	lck_mtx_unlock(&nmp->nm_lock);
	return error;
}
void
nfs_async_write_done(struct nfsmount *nmp)
{
	if (nmp->nm_asyncwrites <= 0) {
		return;
	}
	lck_mtx_lock(&nmp->nm_lock);
	if (nmp->nm_asyncwrites-- >= nfs_max_async_writes) {
		wakeup(&nmp->nm_asyncwrites);
	}
	lck_mtx_unlock(&nmp->nm_lock);
}

  
                                         
  
                               
                             
                                             
                    
  
                                                              
                                               
   
int
nfs_buf_write(struct nfsbuf *bp)
{
	int error = 0, oldflags, async;
	nfsnode_t np;
	thread_t thd;
	kauth_cred_t cred;
	proc_t p = current_proc();
	int iomode;
	off_t doff, dend, firstpg, lastpg;

	FSDBG_TOP(553, bp, NBOFF(bp), bp->nb_flags, 0);

	if (!ISSET(bp->nb_lflags, NBL_BUSY)) {
		panic("nfs_buf_write: buffer is not busy???");
	}

	np = bp->nb_np;
	async = ISSET(bp->nb_flags, NB_ASYNC);
	oldflags = bp->nb_flags;

	CLR(bp->nb_flags, (NB_READ | NB_DONE | NB_ERROR | NB_DELWRI));
	if (ISSET(oldflags, NB_DELWRI)) {
		lck_mtx_lock(&nfs_buf_mutex);
		nfs_nbdwrite--;
		NFSBUFCNTCHK();
		lck_mtx_unlock(&nfs_buf_mutex);
		wakeup(&nfs_nbdwrite);
	}

	                        
	if (ISSET(oldflags, (NB_ASYNC | NB_DELWRI))) {
		lck_mtx_lock(&nfs_buf_mutex);
		if (bp->nb_vnbufs.le_next != NFSNOLIST) {
			LIST_REMOVE(bp, nb_vnbufs);
		}
		LIST_INSERT_HEAD(&np->n_cleanblkhd, bp, nb_vnbufs);
		lck_mtx_unlock(&nfs_buf_mutex);
	}
	nfs_node_lock_force(np);
	np->n_numoutput++;
	nfs_node_unlock(np);
	vnode_startwrite(NFSTOV(np));

	if (p && p->p_stats) {
		OSIncrementAtomicLong(&p->p_stats->p_ru.ru_oublock);
	}

	cred = bp->nb_wcred;
	if (!IS_VALID_CRED(cred) && ISSET(bp->nb_flags, NB_READ)) {
		cred = bp->nb_rcred;                                       
	}
	if (IS_VALID_CRED(cred)) {
		kauth_cred_ref(cred);
	}
	thd = async ? NULL : current_thread();

	                                                                  
	if (!ISSET(bp->nb_flags, NB_META)) {
		if (UBCINFOEXISTS(NFSTOV(np))) {
			if (!ISSET(bp->nb_flags, NB_PAGELIST)) {
				error = nfs_buf_upl_setup(bp);
				if (error) {
					printf("nfs_buf_write: upl create failed %d\n", error);
					SET(bp->nb_flags, NB_ERROR);
					bp->nb_error = error = EIO;
					nfs_buf_iodone(bp);
					goto out;
				}
				nfs_buf_upl_check(bp);
			}
		} else {
			                                                            
			printf("nfs_buf_write: ubcinfo already gone\n");
			SET(bp->nb_flags, NB_ERROR);
			bp->nb_error = error = EIO;
			nfs_buf_iodone(bp);
			goto out;
		}
	}

	                                                             
	if (ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
		nfs_buf_check_write_verifier(np, bp);
	}
	if (ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
		struct nfsmount *nmp = NFSTONMP(np);
		if (nfs_mount_gone(nmp)) {
			SET(bp->nb_flags, NB_ERROR);
			bp->nb_error = error = EIO;
			nfs_buf_iodone(bp);
			goto out;
		}
		SET(bp->nb_flags, NB_WRITEINPROG);
		error = nmp->nm_funcs->nf_commit_rpc(np, NBOFF(bp) + bp->nb_dirtyoff,
		    bp->nb_dirtyend - bp->nb_dirtyoff, bp->nb_wcred, bp->nb_verf);
		CLR(bp->nb_flags, NB_WRITEINPROG);
		if (error) {
			if (error != NFSERR_STALEWRITEVERF) {
				SET(bp->nb_flags, NB_ERROR);
				bp->nb_error = error;
			}
			nfs_buf_iodone(bp);
			goto out;
		}
		bp->nb_dirtyoff = bp->nb_dirtyend = 0;
		CLR(bp->nb_flags, NB_NEEDCOMMIT);
		nfs_node_lock_force(np);
		np->n_needcommitcnt--;
		CHECK_NEEDCOMMITCNT(np);
		nfs_node_unlock(np);
	}
	if (!error && (bp->nb_dirtyend > 0)) {
		                                  
		if (NBOFF(bp) + bp->nb_dirtyend > (off_t) np->n_size) {
			bp->nb_dirtyend = np->n_size - NBOFF(bp);
			if (bp->nb_dirtyoff >= bp->nb_dirtyend) {
				bp->nb_dirtyoff = bp->nb_dirtyend = 0;
			}
		}
	}
	if (!error && (bp->nb_dirtyend > 0)) {
		                                                        
		nfsbufpgs pagemask, pagemaskand;
		NFS_BUF_MAP(bp);

		doff = bp->nb_dirtyoff;
		dend = bp->nb_dirtyend;

		                                                       
		if (NBPGDIRTY(bp, doff / PAGE_SIZE)) {
			doff -= doff & PAGE_MASK;
		}
		                                                                
		if (!(doff & PAGE_MASK)) {
			while ((doff > 0) && NBPGDIRTY(bp, (doff - 1) / PAGE_SIZE)) {
				doff -= PAGE_SIZE;
			}
		}
		                                                            
		if ((dend & PAGE_MASK) && NBPGDIRTY(bp, dend / PAGE_SIZE)) {
			dend = round_page_64(dend);
		}
		                                                               
		if (!(dend & PAGE_MASK)) {
			while ((dend < (int)bp->nb_bufsize) && NBPGDIRTY(bp, dend / PAGE_SIZE)) {
				dend += PAGE_SIZE;
			}
		}
		                                           
		if ((NBOFF(bp) + dend) > (off_t) np->n_size) {
			dend = np->n_size - NBOFF(bp);
		}
		                                                     
		if (dend > doff) {
			firstpg = doff / PAGE_SIZE;
			lastpg = (dend - 1) / PAGE_SIZE;
			                                        
			nfs_buf_pgs_set_pages_between(&pagemask, firstpg, lastpg + 1);
		} else {
			NBPGS_ERASE(&pagemask);
		}

		  
                                                                  
                                                                
                                                    
     
		nfs_buf_pgs_bit_not(&pagemask);
		nfs_buf_pgs_bit_and(&bp->nb_dirty, &pagemask, &pagemaskand);
		if (nfs_buf_pgs_is_set(&pagemaskand)) {
			iomode = NFS_WRITE_FILESYNC;
		} else if ((bp->nb_flags & (NB_ASYNC | NB_NEEDCOMMIT | NB_STABLE)) == NB_ASYNC) {
			iomode = NFS_WRITE_UNSTABLE;
		} else {
			iomode = NFS_WRITE_FILESYNC;
		}

		                                            
		bp->nb_offio = doff;
		bp->nb_endio = dend;

		OSAddAtomic64(1, &nfsstats.write_bios);

		SET(bp->nb_flags, NB_WRITEINPROG);
		error = nfs_buf_write_rpc(bp, iomode, thd, cred);
		  
                                                    
                                                    
                                                      
                      
     
	} else {
		if (!error && nfs_buf_pgs_is_set(&bp->nb_dirty)) {                                
			error = nfs_buf_write_dirty_pages(bp, thd, cred);
		}
		nfs_buf_iodone(bp);
	}
	                                                  
out:
	if (!async) {
		error = nfs_buf_iowait(bp);
		                        
		if (oldflags & NB_DELWRI) {
			lck_mtx_lock(&nfs_buf_mutex);
			if (bp->nb_vnbufs.le_next != NFSNOLIST) {
				LIST_REMOVE(bp, nb_vnbufs);
			}
			LIST_INSERT_HEAD(&np->n_cleanblkhd, bp, nb_vnbufs);
			lck_mtx_unlock(&nfs_buf_mutex);
		}
		FSDBG_BOT(553, bp, NBOFF(bp), bp->nb_flags, error);
		nfs_buf_release(bp, 1);
		                                                 
		if ((np->n_flag & NNEEDINVALIDATE) &&
		    !(np->n_bflag & (NBINVALINPROG | NBFLUSHINPROG))) {
			int invalidate = 0;
			nfs_node_lock_force(np);
			if (np->n_flag & NNEEDINVALIDATE) {
				invalidate = 1;
				np->n_flag &= ~NNEEDINVALIDATE;
			}
			nfs_node_unlock(np);
			if (invalidate) {
				  
                                             
                                            
                                        
                                             
                                              
      
                                                   
                                                     
                            
       
				nfs_vinvalbuf2(NFSTOV(np), V_SAVE | V_IGNORE_WRITEERR, thd, cred, 1);
			}
		}
	}

	if (IS_VALID_CRED(cred)) {
		kauth_cred_unref(&cred);
	}
	return error;
}

  
                                 
   
void
nfs_buf_write_finish(struct nfsbuf *bp, thread_t thd, kauth_cred_t cred)
{
	nfsnode_t np = bp->nb_np;
	int error = (bp->nb_flags & NB_ERROR) ? bp->nb_error : 0;
	off_t firstpg, lastpg;

	if ((error == EINTR) || (error == ERESTART)) {
		CLR(bp->nb_flags, NB_ERROR);
		SET(bp->nb_flags, NB_EINTR);
	}

	if (!error) {
		nfsbufpgs pagemask;
		                                                     
		if (bp->nb_endio > bp->nb_offio) {
			firstpg = bp->nb_offio / PAGE_SIZE;
			lastpg = (bp->nb_endio - 1) / PAGE_SIZE;
			                                                
			nfs_buf_pgs_set_pages_between(&pagemask, firstpg, lastpg + 1);
		} else {
			NBPGS_ERASE(&pagemask);
		}
		                                              
		nfs_buf_pgs_bit_not(&pagemask);
		nfs_buf_pgs_bit_and(&bp->nb_dirty, &pagemask, &bp->nb_dirty);
	}

	                             
	if (!error && (bp->nb_commitlevel == NFS_WRITE_UNSTABLE)) {
		if (!ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
			nfs_node_lock_force(np);
			np->n_needcommitcnt++;
			nfs_node_unlock(np);
			SET(bp->nb_flags, NB_NEEDCOMMIT);
		}
		                                                                    
		bp->nb_dirtyoff = bp->nb_offio;
		bp->nb_dirtyend = bp->nb_endio;
	} else if (ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
		nfs_node_lock_force(np);
		np->n_needcommitcnt--;
		CHECK_NEEDCOMMITCNT(np);
		nfs_node_unlock(np);
		CLR(bp->nb_flags, NB_NEEDCOMMIT);
	}

	CLR(bp->nb_flags, NB_WRITEINPROG);

	  
                                                                     
                                                                      
                                                                   
   
                                                                     
                                                                     
                                                     
    
	if ((error == EINTR) || (error == ERESTART) || (!error && (bp->nb_flags & NB_NEEDCOMMIT))) {
		CLR(bp->nb_flags, NB_INVAL);
		if (!ISSET(bp->nb_flags, NB_DELWRI)) {
			SET(bp->nb_flags, NB_DELWRI);
			lck_mtx_lock(&nfs_buf_mutex);
			nfs_nbdwrite++;
			NFSBUFCNTCHK();
			lck_mtx_unlock(&nfs_buf_mutex);
		}
		  
                                                                    
                                                                   
     
		if (ISSET(bp->nb_flags, NB_ASYNC)) {
			                        
			lck_mtx_lock(&nfs_buf_mutex);
			if (bp->nb_vnbufs.le_next != NFSNOLIST) {
				LIST_REMOVE(bp, nb_vnbufs);
			}
			LIST_INSERT_HEAD(&np->n_dirtyblkhd, bp, nb_vnbufs);
			lck_mtx_unlock(&nfs_buf_mutex);
		}
	} else {
		                                                        
		if (error) {
			  
                                                       
                                                          
                                                        
                                          
     
                                                    
                                                       
                 
      
			nfs_node_lock_force(np);
			np->n_error = error;
			np->n_flag |= (NWRITEERR | NNEEDINVALIDATE);
			NATTRINVALIDATE(np);
			nfs_node_unlock(np);
		}
		                           
		bp->nb_dirtyoff = bp->nb_dirtyend = 0;
	}

	if (!error && nfs_buf_pgs_is_set(&bp->nb_dirty)) {
		nfs_buf_write_dirty_pages(bp, thd, cred);
	}
	nfs_buf_iodone(bp);
}

  
                                               
  
                                                         
                                                        
                          
   
int
nfs_buf_write_dirty_pages(struct nfsbuf *bp, thread_t thd, kauth_cred_t cred)
{
	nfsnode_t np = bp->nb_np;
	struct nfsmount *nmp = NFSTONMP(np);
	int error = 0, commit, iomode, iomode2, len, pg, count, npages, off;
	nfsbufpgs dirty;
	uint64_t wverf;
	uio_t auio;
	char uio_buf[UIO_SIZEOF(1)];

	if (!nfs_buf_pgs_is_set(&bp->nb_dirty)) {
		return 0;
	}

	                                                              
	OSAddAtomic64(1, &nfsstats.write_bios);
	NFS_BUF_MAP(bp);
	SET(bp->nb_flags, NB_WRITEINPROG);
	npages = bp->nb_bufsize / PAGE_SIZE;
	iomode = NFS_WRITE_UNSTABLE;

	auio = uio_createwithbuffer(1, 0, UIO_SYSSPACE, UIO_WRITE,
	    &uio_buf, sizeof(uio_buf));

again:
	NBPGS_COPY(&dirty, &bp->nb_dirty);
	wverf = bp->nb_verf;
	commit = NFS_WRITE_FILESYNC;
	for (pg = 0; pg < npages; pg++) {
		if (!NBPGDIRTY(bp, pg)) {
			continue;
		}
		count = 1;
		while (((pg + count) < npages) && NBPGDIRTY(bp, pg + count)) {
			count++;
		}
		                                             
		off = pg * PAGE_SIZE;
		len = count * PAGE_SIZE;
		                        
		if (NBOFF(bp) + off + len > (off_t) np->n_size) {
			len -= (NBOFF(bp) + off + len) - np->n_size;
		}
		if (len > 0) {
			iomode2 = iomode;
			uio_reset(auio, NBOFF(bp) + off, UIO_SYSSPACE, UIO_WRITE);
			uio_addiov(auio, CAST_USER_ADDR_T(bp->nb_data + off), len);
			error = nfs_write_rpc2(np, auio, thd, cred, &iomode2, &bp->nb_verf);
			if (error) {
				break;
			}
			if (iomode2 < commit) {                                                   
				commit = iomode2;
			}
			if ((commit != NFS_WRITE_FILESYNC) && (wverf != bp->nb_verf)) {
				                                                    
				iomode = NFS_WRITE_FILESYNC;
				goto again;
			}
		}
		                      
		while (count--) {
			NBPGS_UNSET(&dirty, pg);
			if (count) {                            
				pg++;
			}
		}
	}
	CLR(bp->nb_flags, NB_WRITEINPROG);

	if (!error && (commit != NFS_WRITE_FILESYNC)) {
		error = nmp->nm_funcs->nf_commit_rpc(np, NBOFF(bp), bp->nb_bufsize, cred, wverf);
		if (error == NFSERR_STALEWRITEVERF) {
			                                                            
			iomode = NFS_WRITE_FILESYNC;
			goto again;
		}
	}
	if (!error) {
		NBPGS_COPY(&bp->nb_dirty, &dirty);
	} else {
		SET(bp->nb_flags, NB_ERROR);
		bp->nb_error = error;
	}
	return error;
}

  
                                             
   
int
nfs_buf_write_rpc(struct nfsbuf *bp, int iomode, thread_t thd, kauth_cred_t cred)
{
	struct nfsmount *nmp;
	nfsnode_t np = bp->nb_np;
	int error = 0, nfsvers, async;
	int64_t nrpcs;
	size_t len;
	uint32_t nmwsize;
	struct nfsreq *req;
	struct nfsreq_cbinfo cb;
	uio_t auio;
	char uio_buf[UIO_SIZEOF(1)];
	off_t offset, length;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		bp->nb_error = error = ENXIO;
		SET(bp->nb_flags, NB_ERROR);
		nfs_buf_iodone(bp);
		return error;
	}
	nfsvers = nmp->nm_vers;
	nmwsize = nmp->nm_wsize;

	offset = bp->nb_offio;
	length = bp->nb_endio - bp->nb_offio;

	                                                            
	async = (bp->nb_flags & NB_ASYNC) && (NFSIOD_MAX > 0);
	bp->nb_commitlevel = NFS_WRITE_FILESYNC;
	cb.rcb_func = async ? nfs_buf_write_rpc_finish : NULL;
	cb.rcb_bp = bp;

	if ((nfsvers == NFS_VER2) && ((NBOFF(bp) + bp->nb_endio) > 0xffffffffLL)) {
		bp->nb_error = error = EFBIG;
		SET(bp->nb_flags, NB_ERROR);
		nfs_buf_iodone(bp);
		return error;
	}

	if (length == 0) {
		                               
#if DEVELOPMENT
		printf("nfs_buf_write_rpc: Got request with zero length. np %p, bp %p, offset %lld\n", np, bp, offset);
#else
		printf("nfs_buf_write_rpc: Got request with zero length.\n");
#endif                  
		nfs_buf_iodone(bp);
		return 0;
	}

	auio = uio_createwithbuffer(1, NBOFF(bp) + offset, UIO_SYSSPACE,
	    UIO_WRITE, &uio_buf, sizeof(uio_buf));
	NFS_UIO_ADDIOV(auio, CAST_USER_ADDR_T(bp->nb_data + offset), length);

	bp->nb_rpcs = nrpcs = (length + nmwsize - 1) / nmwsize;
	if (async && (nrpcs > 1)) {
		SET(bp->nb_flags, NB_MULTASYNCRPC);
	} else {
		CLR(bp->nb_flags, NB_MULTASYNCRPC);
	}

	while (length > 0) {
		if (ISSET(bp->nb_flags, NB_ERROR)) {
			error = bp->nb_error;
			break;
		}
		len = (length > nmwsize) ? nmwsize : (uint32_t)length;
		cb.rcb_args.offset = offset;
		cb.rcb_args.length = len;
#if CONFIG_NFS4
		if (nmp->nm_vers >= NFS_VER4) {
			cb.rcb_args.stategenid = nmp->nm_stategenid;
		}
#endif
		if (async && ((error = nfs_async_write_start(nmp)))) {
			break;
		}
		req = NULL;
		error = nmp->nm_funcs->nf_write_rpc_async(np, auio, len, thd, cred,
		    iomode, &cb, &req);
		if (error) {
			if (async) {
				nfs_async_write_done(nmp);
			}
			break;
		}
		offset += len;
		length -= len;
		if (async) {
			continue;
		}
		nfs_buf_write_rpc_finish(req);
	}

	if (length > 0) {
		  
                                                          
                                                   
     
		bp->nb_error = error;
		SET(bp->nb_flags, NB_ERROR);
		if (ISSET(bp->nb_flags, NB_MULTASYNCRPC)) {
			nrpcs = (length + nmwsize - 1) / nmwsize;
			lck_mtx_lock(&nfs_buf_mutex);
			bp->nb_rpcs -= nrpcs;
			if (bp->nb_rpcs == 0) {
				                                        
				lck_mtx_unlock(&nfs_buf_mutex);
				nfs_buf_write_finish(bp, thd, cred);
			} else {
				                                           
				while (bp->nb_rpcs > 0) {
					msleep(&bp->nb_rpcs, &nfs_buf_mutex, 0,
					    "nfs_buf_write_rpc_cancel", NULL);
				}
				lck_mtx_unlock(&nfs_buf_mutex);
			}
		} else {
			nfs_buf_write_finish(bp, thd, cred);
		}
		                                                     
		if (!ISSET(bp->nb_flags, NB_ERROR)) {
			error = 0;
		}
	}

	return error;
}

  
                                         
   
void
nfs_buf_write_rpc_finish(struct nfsreq *req)
{
	int error = 0, nfsvers, multasyncrpc, finished;
	int committed = NFS_WRITE_FILESYNC;
	uint64_t wverf = 0;
	off_t offset;
	size_t rlen, length;
	void *wakeme = NULL;
	struct nfsreq_cbinfo cb;
	struct nfsreq *wreq = NULL;
	struct nfsbuf *bp;
	struct nfsmount *nmp;
	nfsnode_t np;
	thread_t thd;
	kauth_cred_t cred;
	uio_t auio;
	char uio_buf[UIO_SIZEOF(1)];

finish:
	np = req->r_np;
	thd = req->r_thread;
	cred = req->r_cred;
	if (IS_VALID_CRED(cred)) {
		kauth_cred_ref(cred);
	}
	cb = req->r_callback;
	bp = cb.rcb_bp;
	if (cb.rcb_func) {                                                                                                  
		nfs_request_ref(req, 0);
	}

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		SET(bp->nb_flags, NB_ERROR);
		bp->nb_error = error = ENXIO;
	}
	if (error || ISSET(bp->nb_flags, NB_ERROR)) {
		                  
		nfs_request_async_cancel(req);
		goto out;
	}
	nfsvers = nmp->nm_vers;

	offset = cb.rcb_args.offset;
	rlen = length = cb.rcb_args.length;

	                    
	error = nmp->nm_funcs->nf_write_rpc_async_finish(np, req, &committed, &rlen, &wverf);
	if ((error == EINPROGRESS) && cb.rcb_func) {
		                             
		if (cb.rcb_func) {
			nfs_request_rele(req);
		}
		if (IS_VALID_CRED(cred)) {
			kauth_cred_unref(&cred);
		}
		return;
	}
#if CONFIG_NFS4
	if ((nmp->nm_vers >= NFS_VER4) && nfs_mount_state_error_should_restart(error) && !ISSET(bp->nb_flags, NB_ERROR)) {
		lck_mtx_lock(&nmp->nm_lock);
		if ((error != NFSERR_OLD_STATEID) && (error != NFSERR_GRACE) && (cb.rcb_args.stategenid == nmp->nm_stategenid)) {
			NP(np, "nfs_buf_write_rpc_finish: error %d @ 0x%llx, 0x%x 0x%x, initiating recovery",
			    error, NBOFF(bp) + offset, cb.rcb_args.stategenid, nmp->nm_stategenid);
			nfs_need_recover(nmp, error);
		}
		lck_mtx_unlock(&nmp->nm_lock);
		if (np->n_flag & NREVOKE) {
			error = EIO;
		} else {
			if (error == NFSERR_GRACE) {
				if (cb.rcb_func) {
					  
                                                                
                                                             
        
					struct timeval now;
					if (req->r_nmrep.nmc_mhead) {
						mbuf_freem(req->r_nmrep.nmc_mhead);
						req->r_nmrep.nmc_mhead = NULL;
					}
					req->r_error = 0;
					microuptime(&now);
					lck_mtx_lock(&req->r_mtx);
					req->r_resendtime = now.tv_sec + 2;
					req->r_xid = 0;                                 
					req->r_flags |= R_RESTART;
					req->r_start = 0;
					nfs_asyncio_resend(req);
					lck_mtx_unlock(&req->r_mtx);
					if (IS_VALID_CRED(cred)) {
						kauth_cred_unref(&cred);
					}
					                                                                      
					return;
				}
				                                                      
				tsleep(&nmp->nm_state, (PZERO - 1), "nfsgrace", 2 * hz);
			}
			if (!(error = nfs_mount_state_wait_for_recovery(nmp))) {
				rlen = 0;
				goto writeagain;
			}
		}
	}
#endif
	if (error) {
		SET(bp->nb_flags, NB_ERROR);
		bp->nb_error = error;
	}
	if (error || (nfsvers == NFS_VER2)) {
		goto out;
	}
	if (rlen <= 0) {
		SET(bp->nb_flags, NB_ERROR);
		bp->nb_error = error = EIO;
		goto out;
	}

	                                       
	if (committed < bp->nb_commitlevel) {
		bp->nb_commitlevel = committed;
	}

	                              
	if (!bp->nb_verf) {
		bp->nb_verf = wverf;
	} else if (bp->nb_verf != wverf) {
		                                                           
		bp->nb_flags |= NB_STALEWVERF;
		bp->nb_commitlevel = NFS_WRITE_UNSTABLE;
		bp->nb_verf = wverf;
	}

	if (!ISSET(bp->nb_flags, NB_STALEWVERF) && rlen > 0 && (bp->nb_offio < (offset + (int)rlen))) {
		bp->nb_offio = offset + rlen;
	}

	  
                           
   
                                                    
                                                   
                                                             
    
	if ((rlen < length) && !(bp->nb_flags & (NB_STALEWVERF | NB_ERROR))) {
#if CONFIG_NFS4
writeagain:
#endif
		offset += rlen;
		length -= rlen;

		auio = uio_createwithbuffer(1, NBOFF(bp) + offset, UIO_SYSSPACE,
		    UIO_WRITE, &uio_buf, sizeof(uio_buf));
		uio_addiov(auio, CAST_USER_ADDR_T(bp->nb_data + offset), length);

		cb.rcb_args.offset = offset;
		cb.rcb_args.length = length;
#if CONFIG_NFS4
		if (nmp->nm_vers >= NFS_VER4) {
			cb.rcb_args.stategenid = nmp->nm_stategenid;
		}
#endif
		                                                      
		error = nmp->nm_funcs->nf_write_rpc_async(np, auio, length, thd, cred,
		    NFS_WRITE_FILESYNC, &cb, &wreq);
		if (!error) {
			if (IS_VALID_CRED(cred)) {
				kauth_cred_unref(&cred);
			}
			if (!cb.rcb_func) {
				                                                         
				req = wreq;
				wreq = NULL;
				goto finish;
			}
			nfs_request_rele(req);
			  
                      
                                         
                                               
      
			return;
		}
		SET(bp->nb_flags, NB_ERROR);
		bp->nb_error = error;
	}

out:
	if (cb.rcb_func) {
		nfs_async_write_done(nmp);
		nfs_request_rele(req);
	}
	  
                                             
                                              
   
                                                          
                                                          
                                               
    
	multasyncrpc = ISSET(bp->nb_flags, NB_MULTASYNCRPC);
	if (multasyncrpc) {
		lck_mtx_lock(&nfs_buf_mutex);
	}

	bp->nb_rpcs--;
	finished = (bp->nb_rpcs == 0);

	if (multasyncrpc) {
		lck_mtx_unlock(&nfs_buf_mutex);
	}

	if (finished) {
		if (multasyncrpc) {
			wakeme = &bp->nb_rpcs;
		}
		nfs_buf_write_finish(bp, thd, cred);
		if (wakeme) {
			wakeup(wakeme);
		}
	}

	if (IS_VALID_CRED(cred)) {
		kauth_cred_unref(&cred);
	}

	if (cb.rcb_func && np->n_needcommitcnt >= NFS_A_LOT_OF_NEEDCOMMITS) {
		nfs_flushcommits(np, 1);
	}
}

  
                                                           
   
int
nfs_flushcommits(nfsnode_t np, int nowait)
{
	struct nfsmount *nmp;
	struct nfsbuf *bp, *prevlbp, *lbp;
	struct nfsbuflists blist, commitlist;
	int error = 0, retv, wcred_set, flags;
	u_quad_t off, endoff, toff;
	uint64_t wverf, count;
	kauth_cred_t wcred = NULL;
	nfsbufpgs dirty;

	FSDBG_TOP(557, np, 0, 0, 0);

	  
                                                                           
                                                                      
                                                                          
                               
    
	if (!LIST_EMPTY(&np->n_dirtyblkhd)) {
		error = nfs_node_lock(np);
		if (error) {
			goto done;
		}
		np->n_flag |= NMODIFIED;
		nfs_node_unlock(np);
	}

	off = (u_quad_t)-1;
	endoff = 0;
	wcred_set = 0;
	LIST_INIT(&commitlist);

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		error = ENXIO;
		goto done;
	}
	if (nmp->nm_vers == NFS_VER2) {
		error = EINVAL;
		goto done;
	}

	flags = NBI_DIRTY;
	if (nowait) {
		flags |= NBI_NOWAIT;
	}
	lck_mtx_lock(&nfs_buf_mutex);
	wverf = nmp->nm_verf;
	if (!nfs_buf_iterprepare(np, &blist, flags)) {
		while ((bp = LIST_FIRST(&blist))) {
			LIST_REMOVE(bp, nb_vnbufs);
			LIST_INSERT_HEAD(&np->n_dirtyblkhd, bp, nb_vnbufs);
			error = nfs_buf_acquire(bp, NBAC_NOWAIT, 0, 0);
			if (error) {
				continue;
			}
			if (ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
				nfs_buf_check_write_verifier(np, bp);
			}
			if (((bp->nb_flags & (NB_DELWRI | NB_NEEDCOMMIT)) != (NB_DELWRI | NB_NEEDCOMMIT)) ||
			    (bp->nb_verf != wverf)) {
				nfs_buf_drop(bp);
				continue;
			}
			nfs_buf_remfree(bp);

			                                                  

			FSDBG(557, bp, bp->nb_flags, bp->nb_valid, bp->nb_dirty);
			FSDBG(557, bp->nb_validoff, bp->nb_validend,
			    bp->nb_dirtyoff, bp->nb_dirtyend);

			  
                                                     
                                                   
     
                                                            
                                                              
      
			if (wcred_set == 0) {
				wcred = bp->nb_wcred;
				if (!IS_VALID_CRED(wcred)) {
					panic("nfs: needcommit w/out wcred");
				}
				wcred_set = 1;
			} else if ((wcred_set == 1) && wcred != bp->nb_wcred) {
				wcred_set = -1;
			}
			SET(bp->nb_flags, NB_WRITEINPROG);

			  
                                                               
                                                                   
                                                               
      
			prevlbp = NULL;
			LIST_FOREACH(lbp, &commitlist, nb_vnbufs) {
				if (bp->nb_lblkno < lbp->nb_lblkno) {
					break;
				}
				prevlbp = lbp;
			}
			LIST_REMOVE(bp, nb_vnbufs);
			if (prevlbp) {
				LIST_INSERT_AFTER(prevlbp, bp, nb_vnbufs);
			} else {
				LIST_INSERT_HEAD(&commitlist, bp, nb_vnbufs);
			}

			                                    
			toff = NBOFF(bp) + bp->nb_dirtyoff;
			if (toff < off) {
				off = toff;
			}
			toff += (u_quad_t)(bp->nb_dirtyend - bp->nb_dirtyoff);
			if (toff > endoff) {
				endoff = toff;
			}
		}
		nfs_buf_itercomplete(np, &blist, NBI_DIRTY);
	}
	lck_mtx_unlock(&nfs_buf_mutex);

	if (LIST_EMPTY(&commitlist)) {
		error = ENOBUFS;
		goto done;
	}

	  
                                                                     
                      
   
                                                                      
                                                                     
                                                                       
                                                                  
                      
    
	LIST_FOREACH(bp, &commitlist, nb_vnbufs) {
		if (!ISSET(bp->nb_flags, NB_PAGELIST)) {
			retv = nfs_buf_upl_setup(bp);
			if (retv) {
				                                                                        
				printf("nfs_flushcommits: upl create failed %d\n", retv);
				NBPGS_ERASE(&bp->nb_valid);
				NBPGS_ERASE(&bp->nb_dirty);
			}
		}
		nfs_buf_upl_check(bp);
	}

	  
                                           
                                                            
                                                       
               
    
	if (wcred_set == 1) {
		  
                                                           
                                                                
     
		if ((endoff - off) > 0xffffffff) {
			count = 0;
		} else {
			count = (endoff - off);
		}
		retv = nmp->nm_funcs->nf_commit_rpc(np, off, count, wcred, wverf);
	} else {
		retv = 0;
		LIST_FOREACH(bp, &commitlist, nb_vnbufs) {
			toff = NBOFF(bp) + bp->nb_dirtyoff;
			count = bp->nb_dirtyend - bp->nb_dirtyoff;
			retv = nmp->nm_funcs->nf_commit_rpc(np, toff, count, bp->nb_wcred, wverf);
			if (retv) {
				break;
			}
		}
	}

	  
                                                    
                                                 
              
    
	while ((bp = LIST_FIRST(&commitlist))) {
		LIST_REMOVE(bp, nb_vnbufs);
		FSDBG(557, bp, retv, bp->nb_flags, bp->nb_dirty);
		nfs_node_lock_force(np);
		CLR(bp->nb_flags, (NB_NEEDCOMMIT | NB_WRITEINPROG));
		np->n_needcommitcnt--;
		CHECK_NEEDCOMMITCNT(np);
		nfs_node_unlock(np);

		if (retv) {
			                             
			lck_mtx_lock(&nfs_buf_mutex);
			LIST_INSERT_HEAD(&np->n_dirtyblkhd, bp, nb_vnbufs);
			lck_mtx_unlock(&nfs_buf_mutex);
			nfs_buf_release(bp, 1);
			continue;
		}

		nfs_node_lock_force(np);
		np->n_numoutput++;
		nfs_node_unlock(np);
		vnode_startwrite(NFSTOV(np));
		if (ISSET(bp->nb_flags, NB_DELWRI)) {
			lck_mtx_lock(&nfs_buf_mutex);
			nfs_nbdwrite--;
			NFSBUFCNTCHK();
			lck_mtx_unlock(&nfs_buf_mutex);
			wakeup(&nfs_nbdwrite);
		}
		CLR(bp->nb_flags, (NB_READ | NB_DONE | NB_ERROR | NB_DELWRI));
		                                                         
		                                                               
		NBPGS_COPY(&dirty, &bp->nb_dirty);
		if (!nfs_buf_pgs_is_set(&dirty)) {
			SET(bp->nb_flags, NB_ASYNC);
		} else {
			CLR(bp->nb_flags, NB_ASYNC);
		}

		                        
		lck_mtx_lock(&nfs_buf_mutex);
		LIST_INSERT_HEAD(&np->n_cleanblkhd, bp, nb_vnbufs);
		lck_mtx_unlock(&nfs_buf_mutex);

		bp->nb_dirtyoff = bp->nb_dirtyend = 0;

		nfs_buf_iodone(bp);
		if (nfs_buf_pgs_is_set(&dirty)) {
			                                                
			CLR(bp->nb_flags, NB_DONE);
			nfs_buf_write_delayed(bp);
		}
	}

done:
	FSDBG_BOT(557, np, 0, 0, error);
	return error;
}

  
                                                
                                                             
                             
   
int
nfs_flush(nfsnode_t np, int waitfor, thread_t thd, int ignore_writeerr)
{
	struct nfsbuf *bp;
	struct nfsbuflists blist;
	struct nfsmount *nmp = NFSTONMP(np);
	int error = 0, error2, slptimeo = 0, slpflag = 0;
	int nfsvers, flags, passone = 1;

	FSDBG_TOP(517, np, waitfor, ignore_writeerr, 0);

	if (nfs_mount_gone(nmp)) {
		error = ENXIO;
		goto out;
	}
	nfsvers = nmp->nm_vers;
	if (NMFLAG(nmp, INTR)) {
		slpflag = PCATCH;
	}

	if (!LIST_EMPTY(&np->n_dirtyblkhd)) {
		nfs_node_lock_force(np);
		np->n_flag |= NMODIFIED;
		nfs_node_unlock(np);
	}

	lck_mtx_lock(&nfs_buf_mutex);
	while (np->n_bflag & NBFLUSHINPROG) {
		np->n_bflag |= NBFLUSHWANT;
		error = msleep(&np->n_bflag, &nfs_buf_mutex, slpflag, "nfs_flush", NULL);
		if ((error && (error != EWOULDBLOCK)) ||
		    ((error = nfs_sigintr(NFSTONMP(np), NULL, thd, 0)))) {
			lck_mtx_unlock(&nfs_buf_mutex);
			goto out;
		}
	}
	np->n_bflag |= NBFLUSHINPROG;

	  
                                                         
                                                                
                                                                  
                                                                  
                                                         
    
again:
	FSDBG(518, LIST_FIRST(&np->n_dirtyblkhd), np->n_flag, 0, 0);
	if (!NFSTONMP(np)) {
		lck_mtx_unlock(&nfs_buf_mutex);
		error = ENXIO;
		goto done;
	}

	                                              
	if (!nfs_buf_iterprepare(np, &blist, NBI_DIRTY)) {
		while ((bp = LIST_FIRST(&blist))) {
			LIST_REMOVE(bp, nb_vnbufs);
			LIST_INSERT_HEAD(&np->n_dirtyblkhd, bp, nb_vnbufs);
			flags = (passone || !(waitfor == MNT_WAIT || waitfor == MNT_DWAIT)) ? NBAC_NOWAIT : 0;
			if (flags != NBAC_NOWAIT) {
				nfs_buf_refget(bp);
			}
			while ((error = nfs_buf_acquire(bp, flags, slpflag, slptimeo))) {
				FSDBG(524, bp, flags, bp->nb_lflags, bp->nb_flags);
				if (error == EBUSY) {
					break;
				}
				if (error) {
					error2 = nfs_sigintr(NFSTONMP(np), NULL, thd, 0);
					if (error2) {
						if (flags != NBAC_NOWAIT) {
							nfs_buf_refrele(bp);
						}
						nfs_buf_itercomplete(np, &blist, NBI_DIRTY);
						lck_mtx_unlock(&nfs_buf_mutex);
						error = error2;
						goto done;
					}
					if (slpflag == PCATCH) {
						slpflag = 0;
						slptimeo = 2 * hz;
					}
				}
			}
			if (flags != NBAC_NOWAIT) {
				nfs_buf_refrele(bp);
			}
			if (error == EBUSY) {
				continue;
			}
			if (!bp->nb_np) {
				                               
				nfs_buf_drop(bp);
				continue;
			}
			if (ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
				nfs_buf_check_write_verifier(np, bp);
			}
			if (!ISSET(bp->nb_flags, NB_DELWRI)) {
				                               
				nfs_buf_drop(bp);
				continue;
			}
			FSDBG(525, bp, passone, bp->nb_lflags, bp->nb_flags);
			if ((passone || !(waitfor == MNT_WAIT || waitfor == MNT_DWAIT)) &&
			    ISSET(bp->nb_flags, NB_NEEDCOMMIT)) {
				nfs_buf_drop(bp);
				continue;
			}
			nfs_buf_remfree(bp);
			lck_mtx_unlock(&nfs_buf_mutex);
			if (ISSET(bp->nb_flags, NB_ERROR)) {
				nfs_node_lock_force(np);
				np->n_error = bp->nb_error ? bp->nb_error : EIO;
				np->n_flag |= NWRITEERR;
				nfs_node_unlock(np);
				nfs_buf_release(bp, 1);
				lck_mtx_lock(&nfs_buf_mutex);
				continue;
			}
			SET(bp->nb_flags, NB_ASYNC);
			if (!passone) {
				                                                  
				SET(bp->nb_flags, NB_STABLE);
			}
			nfs_buf_write(bp);
			lck_mtx_lock(&nfs_buf_mutex);
		}
		nfs_buf_itercomplete(np, &blist, NBI_DIRTY);
	}
	lck_mtx_unlock(&nfs_buf_mutex);

	if (waitfor == MNT_WAIT || waitfor == MNT_DWAIT) {
		while ((error = vnode_waitforwrites(NFSTOV(np), 0, slpflag, slptimeo, "nfsflush"))) {
			error2 = nfs_sigintr(NFSTONMP(np), NULL, thd, 0);
			if (error2) {
				error = error2;
				goto done;
			}
			if (slpflag == PCATCH) {
				slpflag = 0;
				slptimeo = 2 * hz;
			}
		}
	}

	if (nfsvers != NFS_VER2) {
		                                                            
		                                                                
		while (np->n_needcommitcnt) {
			if (nfs_flushcommits(np, 0)) {
				break;
			}
		}
	}

	if (passone) {
		passone = 0;
		if (!LIST_EMPTY(&np->n_dirtyblkhd)) {
			nfs_node_lock_force(np);
			np->n_flag |= NMODIFIED;
			nfs_node_unlock(np);
		}
		lck_mtx_lock(&nfs_buf_mutex);
		goto again;
	}

	if (waitfor == MNT_WAIT || waitfor == MNT_DWAIT) {
		if (!LIST_EMPTY(&np->n_dirtyblkhd)) {
			nfs_node_lock_force(np);
			np->n_flag |= NMODIFIED;
			nfs_node_unlock(np);
		}
		lck_mtx_lock(&nfs_buf_mutex);
		if (!LIST_EMPTY(&np->n_dirtyblkhd)) {
			goto again;
		}
		lck_mtx_unlock(&nfs_buf_mutex);
		nfs_node_lock_force(np);
		  
                                                                
                                                                
                                                                
                                                               
                                                                 
                                                               
                                                              
                                                                     
                                                                 
                                                                      
                                                     
     
		if (!np->n_wrbusy && !np->n_numoutput) {
			np->n_flag &= ~NMODIFIED;
			NATTRINVALIDATE(np);
			nfs_get_xid(&np->n_xid);
		}
	} else {
		nfs_node_lock_force(np);
	}

	FSDBG(526, np->n_flag, np->n_error, 0, 0);
	if (!ignore_writeerr && (np->n_flag & NWRITEERR)) {
		error = np->n_error;
		np->n_flag &= ~NWRITEERR;
	}
	nfs_node_unlock(np);
done:
	lck_mtx_lock(&nfs_buf_mutex);
	flags = np->n_bflag;
	np->n_bflag &= ~(NBFLUSHINPROG | NBFLUSHWANT);
	lck_mtx_unlock(&nfs_buf_mutex);
	if (flags & NBFLUSHWANT) {
		wakeup(&np->n_bflag);
	}
out:
	FSDBG_BOT(517, np, error, ignore_writeerr, 0);
	return error;
}

  
                                                                
                                            
   
int
nfs_vinvalbuf_internal(
	nfsnode_t np,
	int flags,
	thread_t thd,
	kauth_cred_t cred,
	int slpflag,
	int slptimeo)
{
	struct nfsbuf *bp;
	struct nfsbuflists blist;
	int list, error = 0;

	if (flags & V_SAVE) {
		if ((error = nfs_flush(np, MNT_WAIT, thd, (flags & V_IGNORE_WRITEERR)))) {
			return error;
		}
	}

	lck_mtx_lock(&nfs_buf_mutex);
	for (;;) {
		list = NBI_CLEAN;
		if (nfs_buf_iterprepare(np, &blist, list)) {
			list = NBI_DIRTY;
			if (nfs_buf_iterprepare(np, &blist, list)) {
				break;
			}
		}
		while ((bp = LIST_FIRST(&blist))) {
			LIST_REMOVE(bp, nb_vnbufs);
			if (list == NBI_CLEAN) {
				LIST_INSERT_HEAD(&np->n_cleanblkhd, bp, nb_vnbufs);
			} else {
				LIST_INSERT_HEAD(&np->n_dirtyblkhd, bp, nb_vnbufs);
			}
			nfs_buf_refget(bp);
			while ((error = nfs_buf_acquire(bp, NBAC_REMOVE, slpflag, slptimeo))) {
				FSDBG(556, np, bp, NBOFF(bp), bp->nb_flags);
				if (error != EAGAIN) {
					FSDBG(554, np, bp, -1, error);
					nfs_buf_refrele(bp);
					nfs_buf_itercomplete(np, &blist, list);
					lck_mtx_unlock(&nfs_buf_mutex);
					return error;
				}
			}
			nfs_buf_refrele(bp);
			FSDBG(554, np, bp, NBOFF(bp), bp->nb_flags);
			lck_mtx_unlock(&nfs_buf_mutex);
			if ((flags & V_SAVE) && UBCINFOEXISTS(NFSTOV(np)) && bp->nb_np &&
			    (NBOFF(bp) < (off_t)np->n_size)) {
				                                         
				                                           
				nfsbufpgs pagemask;
				int mustwrite = 0;
				off_t end = (NBOFF(bp) + bp->nb_bufsize > (off_t)np->n_size) ?
				    (np->n_size - NBOFF(bp)) : bp->nb_bufsize;
				if (!ISSET(bp->nb_flags, NB_PAGELIST)) {
					error = nfs_buf_upl_setup(bp);
					if (error == EINVAL) {
						                                    
						                                   
						                              
					} else if (error) {
						printf("nfs_vinvalbuf: upl setup failed %d\n", error);
					}
					NBPGS_ERASE(&bp->nb_valid);
					NBPGS_ERASE(&bp->nb_dirty);
				}
				nfs_buf_upl_check(bp);
				                                             
				if ((bp->nb_dirtyend > 0) && (bp->nb_dirtyoff < end)) {
					                             
					if (bp->nb_dirtyend > end) {
						bp->nb_dirtyend = end;
						if (bp->nb_dirtyoff >= bp->nb_dirtyend) {
							bp->nb_dirtyoff = bp->nb_dirtyend = 0;
						}
					}
					if ((bp->nb_dirtyend > 0) && (bp->nb_dirtyoff < end)) {
						mustwrite++;
					}
				}
				nfs_buf_pgs_get_page_mask(&pagemask, round_page_64(end) / PAGE_SIZE);
				nfs_buf_pgs_bit_and(&bp->nb_dirty, &pagemask, &bp->nb_dirty);
				if (nfs_buf_pgs_is_set(&bp->nb_dirty)) {
					mustwrite++;
				}
				                                                            
				if (mustwrite && !IS_VALID_CRED(bp->nb_wcred) && !IS_VALID_CRED(cred)) {
					printf("nfs_vinvalbuf: found dirty buffer with no write creds\n");
					mustwrite = 0;
				}
				if (mustwrite) {
					FSDBG(554, np, bp, 0xd00dee, bp->nb_flags);
					if (!ISSET(bp->nb_flags, NB_PAGELIST)) {
						panic("nfs_vinvalbuf: dirty buffer without upl");
					}
					                                                    
					                                                               
					                                                       
					CLR(bp->nb_flags, (NB_DONE | NB_ERROR | NB_INVAL | NB_ASYNC));
					SET(bp->nb_flags, NB_STABLE | NB_NOCACHE);
					if (!IS_VALID_CRED(bp->nb_wcred)) {
						kauth_cred_ref(cred);
						bp->nb_wcred = cred;
					}
					error = nfs_buf_write(bp);
					                             
					if (error) {
						FSDBG(554, bp, 0xd00dee, 0xbad, error);
						nfs_node_lock_force(np);
						if ((error != EINTR) && (error != ERESTART)) {
							np->n_error = error;
							np->n_flag |= NWRITEERR;
						}
						  
                                               
                                              
                                               
                                                
         
						NATTRINVALIDATE(np);
						nfs_node_unlock(np);
						if ((error == EINTR) || (error == ERESTART)) {
							  
                                                
                                               
                                                 
          
							lck_mtx_lock(&nfs_buf_mutex);
							nfs_buf_itercomplete(np, &blist, list);
							lck_mtx_unlock(&nfs_buf_mutex);
							return error;
						}
						error = 0;
					}
					lck_mtx_lock(&nfs_buf_mutex);
					continue;
				}
			}
			SET(bp->nb_flags, NB_INVAL);
			                                            
			nfs_buf_release(bp, 0);
			lck_mtx_lock(&nfs_buf_mutex);
		}
		nfs_buf_itercomplete(np, &blist, list);
	}
	if (!LIST_EMPTY(&(np)->n_dirtyblkhd) || !LIST_EMPTY(&(np)->n_cleanblkhd)) {
		panic("nfs_vinvalbuf: flush/inval failed");
	}
	lck_mtx_unlock(&nfs_buf_mutex);
	nfs_node_lock_force(np);
	if (!(flags & V_SAVE)) {
		np->n_flag &= ~NMODIFIED;
	}
	if (vnode_vtype(NFSTOV(np)) == VREG) {
		np->n_lastrahead = -1;
	}
	nfs_node_unlock(np);
	NFS_BUF_FREEUP();
	return 0;
}


  
                                                                        
                                             
   
int
nfs_vinvalbuf(vnode_t vp, int flags, vfs_context_t ctx, int intrflg)
{
	return nfs_vinvalbuf2(vp, flags, vfs_context_thread(ctx), vfs_context_ucred(ctx), intrflg);
}

int
nfs_vinvalbuf2(vnode_t vp, int flags, thread_t thd, kauth_cred_t cred, int intrflg)
{
	nfsnode_t np = VTONFS(vp);
	struct nfsmount *nmp = VTONMP(vp);
	int error, slpflag, slptimeo, nflags, retry = 0;
	int ubcflags = UBC_PUSHALL | UBC_SYNC | UBC_INVALIDATE;
	struct timespec ts = { .tv_sec = 2, .tv_nsec = 0 };
	off_t size;

	FSDBG_TOP(554, np, flags, intrflg, 0);

	if (nmp && !NMFLAG(nmp, INTR)) {
		intrflg = 0;
	}
	if (intrflg) {
		slpflag = PCATCH;
		slptimeo = 2 * hz;
	} else {
		slpflag = 0;
		slptimeo = 0;
	}

	                                                                  
	lck_mtx_lock(&nfs_buf_mutex);
	while (np->n_bflag & NBINVALINPROG) {
		np->n_bflag |= NBINVALWANT;
		msleep(&np->n_bflag, &nfs_buf_mutex, slpflag, "nfs_vinvalbuf", &ts);
		if ((error = nfs_sigintr(VTONMP(vp), NULL, thd, 0))) {
			lck_mtx_unlock(&nfs_buf_mutex);
			return error;
		}
		if (np->n_bflag & NBINVALINPROG) {
			slpflag = 0;
		}
	}
	np->n_bflag |= NBINVALINPROG;
	lck_mtx_unlock(&nfs_buf_mutex);

	                              
again:
	                                                                                        
	if (nfs_mount_gone(nmp)) {
		flags &= ~V_SAVE;
	}

	error = nfs_vinvalbuf_internal(np, flags, thd, cred, slpflag, 0);
	while (error) {
		FSDBG(554, np, 0, 0, error);
		if ((error = nfs_sigintr(VTONMP(vp), NULL, thd, 0))) {
			goto done;
		}
		error = nfs_vinvalbuf_internal(np, flags, thd, cred, 0, slptimeo);
	}

	                                                                                        
	if (nfs_mount_gone(nmp)) {
		ubcflags &= ~UBC_PUSHALL;
	}

	                                  
	if (UBCINFOEXISTS(vp) && (size = ubc_getsize(vp))) {
		if ((error = ubc_msync(vp, 0, size, NULL, ubcflags))) {
			if (error == EINVAL) {
				panic("nfs_vinvalbuf(): ubc_msync failed!, error %d", error);
			}
			if (retry++ < 10) {                                     
				if (retry > 1 || error == ENXIO) {
					ubcflags &= ~UBC_PUSHALL;
				}
				goto again;
			}
			             
			printf("nfs_vinvalbuf(): ubc_msync failed!, error %d\n", error);
		}
	}
done:
	lck_mtx_lock(&nfs_buf_mutex);
	nflags = np->n_bflag;
	np->n_bflag &= ~(NBINVALINPROG | NBINVALWANT);
	lck_mtx_unlock(&nfs_buf_mutex);
	if (nflags & NBINVALWANT) {
		wakeup(&np->n_bflag);
	}

	FSDBG_BOT(554, np, flags, intrflg, error);
	return error;
}

  
                                         
   
void
nfs_wait_bufs(nfsnode_t np)
{
	struct nfsbuf *bp;
	struct nfsbuflists blist;
	int error = 0;

	lck_mtx_lock(&nfs_buf_mutex);
	if (!nfs_buf_iterprepare(np, &blist, NBI_CLEAN)) {
		while ((bp = LIST_FIRST(&blist))) {
			LIST_REMOVE(bp, nb_vnbufs);
			LIST_INSERT_HEAD(&np->n_cleanblkhd, bp, nb_vnbufs);
			nfs_buf_refget(bp);
			while ((error = nfs_buf_acquire(bp, 0, 0, 0))) {
				if (error != EAGAIN) {
					nfs_buf_refrele(bp);
					nfs_buf_itercomplete(np, &blist, NBI_CLEAN);
					lck_mtx_unlock(&nfs_buf_mutex);
					return;
				}
			}
			nfs_buf_refrele(bp);
			nfs_buf_drop(bp);
		}
		nfs_buf_itercomplete(np, &blist, NBI_CLEAN);
	}
	if (!nfs_buf_iterprepare(np, &blist, NBI_DIRTY)) {
		while ((bp = LIST_FIRST(&blist))) {
			LIST_REMOVE(bp, nb_vnbufs);
			LIST_INSERT_HEAD(&np->n_dirtyblkhd, bp, nb_vnbufs);
			nfs_buf_refget(bp);
			while ((error = nfs_buf_acquire(bp, 0, 0, 0))) {
				if (error != EAGAIN) {
					nfs_buf_refrele(bp);
					nfs_buf_itercomplete(np, &blist, NBI_DIRTY);
					lck_mtx_unlock(&nfs_buf_mutex);
					return;
				}
			}
			nfs_buf_refrele(bp);
			nfs_buf_drop(bp);
		}
		nfs_buf_itercomplete(np, &blist, NBI_DIRTY);
	}
	lck_mtx_unlock(&nfs_buf_mutex);
}


  
                                                                   
                                       
   
void
nfs_asyncio_finish(struct nfsreq *req)
{
	struct nfsmount *nmp;
	struct nfsiod *niod;
	int started = 0;

	FSDBG_TOP(552, nmp, 0, 0, 0);
again:
	nmp = req->r_nmp;

	if (nmp == NULL) {
		return;
	}

	lck_mtx_lock(&nfsiod_mutex);
	niod = nmp->nm_niod;

	                                                 
	if (!niod) {
		niod = TAILQ_FIRST(&nfsiodfree);
		if (niod) {
			TAILQ_REMOVE(&nfsiodfree, niod, niod_link);
			TAILQ_INSERT_TAIL(&nfsiodwork, niod, niod_link);
			niod->niod_nmp = nmp;
		} else if (((nfsiod_thread_count < NFSIOD_MAX) || (nfsiod_thread_count <= 0)) && (started < 4)) {
			  
                                
                                                
                                       
      
			lck_mtx_unlock(&nfsiod_mutex);
			started++;
			if (!nfsiod_start()) {
				goto again;
			}
			lck_mtx_lock(&nfsiod_mutex);
		}
	}

	  
                                                                      
                                                                         
                                                                       
                                                           
    
	lck_mtx_lock(&req->r_mtx);
	if (req->r_flags & R_RESENDQ) {
		lck_mtx_lock(&nmp->nm_lock);
		if ((req->r_flags & R_RESENDQ) && req->r_rchain.tqe_next != NFSREQNOLIST) {
			NFS_BIO_DBG("Proccessing async request on resendq. Removing");
			TAILQ_REMOVE(&nmp->nm_resendq, req, r_rchain);
			req->r_flags &= ~R_RESENDQ;
			req->r_rchain.tqe_next = NFSREQNOLIST;
			assert(req->r_refs > 1);
			                              
			req->r_refs--;
		}
		lck_mtx_unlock(&nmp->nm_lock);
	}
	lck_mtx_unlock(&req->r_mtx);

	if (req->r_achain.tqe_next == NFSREQNOLIST) {
		TAILQ_INSERT_TAIL(&nmp->nm_iodq, req, r_achain);
	}

	                                                                   
	if (!nmp->nm_niod) {
		if (niod) {                                         
			nmp->nm_niod = niod;
			lck_mtx_unlock(&nfsiod_mutex);
			wakeup(niod);
		} else if (nfsiod_thread_count > 0) {
			                                                       
			if (nmp->nm_iodlink.tqe_next == NFSNOLIST) {
				TAILQ_INSERT_TAIL(&nfsiodmounts, nmp, nm_iodlink);
			}
			lck_mtx_unlock(&nfsiod_mutex);
		} else {
			printf("nfs_asyncio(): no nfsiods? %d %d (%d)\n", nfsiod_thread_count, NFSIOD_MAX, started);
			lck_mtx_unlock(&nfsiod_mutex);
			                                                  
			started = 0;
			goto again;
		}
	} else {
		lck_mtx_unlock(&nfsiod_mutex);
	}

	FSDBG_BOT(552, nmp, 0, 0, 0);
}

  
                                        
                                         
   
void
nfs_asyncio_resend(struct nfsreq *req)
{
	struct nfsmount *nmp = req->r_nmp;

	if (nfs_mount_gone(nmp)) {
		return;
	}

#if CONFIG_NFS_GSS
	nfs_gss_clnt_rpcdone(req);
#endif
	lck_mtx_lock(&nmp->nm_lock);
	if (!(req->r_flags & R_RESENDQ)) {
		TAILQ_INSERT_TAIL(&nmp->nm_resendq, req, r_rchain);
		req->r_flags |= R_RESENDQ;
		  
                                                            
                                                       
     
		nfs_request_ref(req, 1);
	}
	nfs_mount_sock_thread_wake(nmp);
	lck_mtx_unlock(&nmp->nm_lock);
}

  
                                     
  
                                             
                                                                  
   
int
nfs_buf_readdir(struct nfsbuf *bp, vfs_context_t ctx)
{
	nfsnode_t np = bp->nb_np;
	struct nfsmount *nmp = NFSTONMP(np);
	int error = 0;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (nmp->nm_vers < NFS_VER4) {
		error = nfs3_readdir_rpc(np, bp, ctx);
	}
#if CONFIG_NFS4
	else {
		error = nfs4_readdir_rpc(np, bp, ctx);
	}
#endif
	if (error && (error != NFSERR_DIRBUFDROPPED)) {
		SET(bp->nb_flags, NB_ERROR);
		bp->nb_error = error;
	}
	return error;
}

#endif                        
