/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
/*-
 * Copyright (c) 1994 Christopher G. Demetriou
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
 *	@(#)vfs_bio.c	8.6 (Berkeley) 1/11/94
 */

  
                   
                                                                      
                                                               
                                                
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc_internal.h>
#include <sys/buf_internal.h>
#include <sys/vnode_internal.h>
#include <sys/mount_internal.h>
#include <sys/trace.h>
#include <kern/kalloc.h>
#include <sys/resourcevar.h>
#include <miscfs/specfs/specdev.h>
#include <sys/ubc.h>
#include <sys/kauth.h>
#if DIAGNOSTIC
#include <kern/assert.h>
#endif                 
#include <kern/task.h>
#include <kern/zalloc.h>
#include <kern/locks.h>
#include <kern/thread.h>

#include <sys/fslog.h>                                
#include <sys/disk.h>                                       

#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <kern/sched_prim.h>                        

#include <vm/vm_kern.h>
#include <vm/vm_pageout.h>

#include <sys/kdebug.h>

#include <libkern/OSAtomic.h>
#include <libkern/OSDebug.h>
#include <sys/ubc_internal.h>

#include <sys/sdt.h>

int     bcleanbuf(buf_t bp, boolean_t discard);
static int      brecover_data(buf_t bp);
static boolean_t incore(vnode_t vp, daddr64_t blkno);
                         
static buf_t    getnewbuf(int slpflag, int slptimeo, int *queue);
static void     bremfree_locked(buf_t bp);
static void     buf_reassign(buf_t bp, vnode_t newvp);
static errno_t  buf_acquire_locked(buf_t bp, int flags, int slpflag, int slptimeo);
static int      buf_iterprepare(vnode_t vp, struct buflists *, int flags);
static void     buf_itercomplete(vnode_t vp, struct buflists *, int flags);
static boolean_t buffer_cache_gc(int);
static buf_t    buf_brelse_shadow(buf_t bp);
static void     buf_free_meta_store(buf_t bp);

static buf_t    buf_create_shadow_internal(buf_t bp, boolean_t force_copy,
    uintptr_t external_storage, void (*iodone)(buf_t, void *), void *arg, int priv);


int  bdwrite_internal(buf_t, int);

extern void disk_conditioner_delay(buf_t, int, int, uint64_t);

                                   
static void     bcleanbuf_thread_init(void);
static void     bcleanbuf_thread(void);

static ZONE_DECLARE(buf_hdr_zone, "buf headers", sizeof(struct buf), ZC_NONE);
static int      buf_hdr_count;


  
                                         
   
#define BUFHASH(dvp, lbn)       \
	(&bufhashtbl[((long)(dvp) / sizeof(*(dvp)) + (int)(lbn)) & bufhash])
LIST_HEAD(bufhashhdr, buf) * bufhashtbl, invalhash;
u_long  bufhash;

static buf_t    incore_locked(vnode_t vp, daddr64_t blkno, struct bufhashhdr *dp);

                                       
struct bufstats bufstats;

                                     
long nbdwrite = 0;
int blaundrycnt = 0;
static int boot_nbuf_headers = 0;

static TAILQ_HEAD(delayqueue, buf) delaybufqueue;

static TAILQ_HEAD(ioqueue, buf) iobufqueue;
static TAILQ_HEAD(bqueues, buf) bufqueues[BQUEUES];
static int needbuffer;
static int need_iobuffer;

static LCK_GRP_DECLARE(buf_mtx_grp, "buffer cache");
static LCK_ATTR_DECLARE(buf_mtx_attr, 0, 0);
static LCK_MTX_DECLARE_ATTR(iobuffer_mtxp, &buf_mtx_grp, &buf_mtx_attr);
static LCK_MTX_DECLARE_ATTR(buf_mtx, &buf_mtx_grp, &buf_mtx_attr);
static LCK_MTX_DECLARE_ATTR(buf_gc_callout, &buf_mtx_grp, &buf_mtx_attr);

static uint32_t buf_busycount;

#define FS_BUFFER_CACHE_GC_CALLOUTS_MAX_SIZE 16
typedef struct {
	void (* callout)(int, void *);
	void *context;
} fs_buffer_cache_gc_callout_t;

fs_buffer_cache_gc_callout_t fs_callouts[FS_BUFFER_CACHE_GC_CALLOUTS_MAX_SIZE] = { {NULL, NULL} };

static __inline__ int
buf_timestamp(void)
{
	struct  timeval         t;
	microuptime(&t);
	return (int)t.tv_sec;
}

  
                                       
   
#define binsheadfree(bp, dp, whichq)    do { \
	                            TAILQ_INSERT_HEAD(dp, bp, b_freelist); \
	                        } while (0)

#define binstailfree(bp, dp, whichq)    do { \
	                            TAILQ_INSERT_TAIL(dp, bp, b_freelist); \
	                        } while (0)

#define BHASHENTCHECK(bp)       \
	if ((bp)->b_hash.le_prev != (struct buf **)0xdeadbeef)  \
	        panic("%p: b_hash.le_prev is not deadbeef", (bp));

#define BLISTNONE(bp)   \
	(bp)->b_hash.le_next = (struct buf *)0; \
	(bp)->b_hash.le_prev = (struct buf **)0xdeadbeef;

  
                                       
   
#define bufinsvn(bp, dp)        LIST_INSERT_HEAD(dp, bp, b_vnbufs)
#define bufremvn(bp) {                                                  \
	LIST_REMOVE(bp, b_vnbufs);                                      \
	(bp)->b_vnbufs.le_next = NOLIST;                                \
}

  
                                               
                               
   
#define LRU_IS_STALE 120                                
#define AGE_IS_STALE 60                                 
#define META_IS_STALE 180                                    

int lru_is_stale = LRU_IS_STALE;
int age_is_stale = AGE_IS_STALE;
int meta_is_stale = META_IS_STALE;

#define MAXLAUNDRY      10

                                        
static __inline__ void
blistenterhead(struct bufhashhdr * head, buf_t bp)
{
	if ((bp->b_hash.le_next = (head)->lh_first) != NULL) {
		(head)->lh_first->b_hash.le_prev = &(bp)->b_hash.le_next;
	}
	(head)->lh_first = bp;
	bp->b_hash.le_prev = &(head)->lh_first;
	if (bp->b_hash.le_prev == (struct buf **)0xdeadbeef) {
		panic("blistenterhead: le_prev is deadbeef");
	}
}

static __inline__ void
binshash(buf_t bp, struct bufhashhdr *dp)
{
#if DIAGNOSTIC
	buf_t   nbp;
#endif                 

	BHASHENTCHECK(bp);

#if DIAGNOSTIC
	nbp = dp->lh_first;
	for (; nbp != NULL; nbp = nbp->b_hash.le_next) {
		if (nbp == bp) {
			panic("buf already in hashlist");
		}
	}
#endif                 

	blistenterhead(dp, bp);
}

static __inline__ void
bremhash(buf_t  bp)
{
	if (bp->b_hash.le_prev == (struct buf **)0xdeadbeef) {
		panic("bremhash le_prev is deadbeef");
	}
	if (bp->b_hash.le_next == bp) {
		panic("bremhash: next points to self");
	}

	if (bp->b_hash.le_next != NULL) {
		bp->b_hash.le_next->b_hash.le_prev = bp->b_hash.le_prev;
	}
	*bp->b_hash.le_prev = (bp)->b_hash.le_next;
}

  
                
   
static __inline__ void
bmovelaundry(buf_t bp)
{
	bp->b_whichq = BQ_LAUNDRY;
	bp->b_timestamp = buf_timestamp();
	binstailfree(bp, &bufqueues[BQ_LAUNDRY], BQ_LAUNDRY);
	blaundrycnt++;
}

static __inline__ void
buf_release_credentials(buf_t bp)
{
	if (IS_VALID_CRED(bp->b_rcred)) {
		kauth_cred_unref(&bp->b_rcred);
	}
	if (IS_VALID_CRED(bp->b_wcred)) {
		kauth_cred_unref(&bp->b_wcred);
	}
}


int
buf_valid(buf_t bp)
{
	if ((bp->b_flags & (B_DONE | B_DELWRI))) {
		return 1;
	}
	return 0;
}

int
buf_fromcache(buf_t bp)
{
	if ((bp->b_flags & B_CACHE)) {
		return 1;
	}
	return 0;
}

void
buf_markinvalid(buf_t bp)
{
	SET(bp->b_flags, B_INVAL);
}

void
buf_markdelayed(buf_t bp)
{
	if (!ISSET(bp->b_flags, B_DELWRI)) {
		SET(bp->b_flags, B_DELWRI);

		OSAddAtomicLong(1, &nbdwrite);
		buf_reassign(bp, bp->b_vp);
	}
	SET(bp->b_flags, B_DONE);
}

void
buf_markclean(buf_t bp)
{
	if (ISSET(bp->b_flags, B_DELWRI)) {
		CLR(bp->b_flags, B_DELWRI);

		OSAddAtomicLong(-1, &nbdwrite);
		buf_reassign(bp, bp->b_vp);
	}
}

void
buf_markeintr(buf_t bp)
{
	SET(bp->b_flags, B_EINTR);
}


void
buf_markaged(buf_t bp)
{
	SET(bp->b_flags, B_AGE);
}

int
buf_fua(buf_t bp)
{
	if ((bp->b_flags & B_FUA) == B_FUA) {
		return 1;
	}
	return 0;
}

void
buf_markfua(buf_t bp)
{
	SET(bp->b_flags, B_FUA);
}

#if CONFIG_PROTECT
cpx_t
bufattr_cpx(bufattr_t bap)
{
	return bap->ba_cpx;
}

void
bufattr_setcpx(bufattr_t bap, cpx_t cpx)
{
	bap->ba_cpx = cpx;
}

void
buf_setcpoff(buf_t bp, uint64_t foffset)
{
	bp->b_attr.ba_cp_file_off = foffset;
}

uint64_t
bufattr_cpoff(bufattr_t bap)
{
	return bap->ba_cp_file_off;
}

void
bufattr_setcpoff(bufattr_t bap, uint64_t foffset)
{
	bap->ba_cp_file_off = foffset;
}

#else                    

uint64_t
bufattr_cpoff(bufattr_t bap __unused)
{
	return 0;
}

void
bufattr_setcpoff(__unused bufattr_t bap, __unused uint64_t foffset)
{
	return;
}

struct cpx *
bufattr_cpx(__unused bufattr_t bap)
{
	return NULL;
}

void
bufattr_setcpx(__unused bufattr_t bap, __unused struct cpx *cpx)
{
}

#endif                      

bufattr_t
bufattr_alloc(void)
{
	return kheap_alloc(KHEAP_DEFAULT, sizeof(struct bufattr),
	           Z_WAITOK | Z_ZERO);
}

void
bufattr_free(bufattr_t bap)
{
	kheap_free(KHEAP_DEFAULT, bap, sizeof(struct bufattr));
}

bufattr_t
bufattr_dup(bufattr_t bap)
{
	bufattr_t new_bufattr;
	new_bufattr = kheap_alloc(KHEAP_DEFAULT, sizeof(struct bufattr),
	    Z_WAITOK);
	if (new_bufattr == NULL) {
		return NULL;
	}

	                                             
	memcpy(new_bufattr, bap, sizeof(struct bufattr));
	return new_bufattr;
}

int
bufattr_rawencrypted(bufattr_t bap)
{
	if ((bap->ba_flags & BA_RAW_ENCRYPTED_IO)) {
		return 1;
	}
	return 0;
}

int
bufattr_throttled(bufattr_t bap)
{
	return GET_BUFATTR_IO_TIER(bap);
}

int
bufattr_passive(bufattr_t bap)
{
	if ((bap->ba_flags & BA_PASSIVE)) {
		return 1;
	}
	return 0;
}

int
bufattr_nocache(bufattr_t bap)
{
	if ((bap->ba_flags & BA_NOCACHE)) {
		return 1;
	}
	return 0;
}

int
bufattr_meta(bufattr_t bap)
{
	if ((bap->ba_flags & BA_META)) {
		return 1;
	}
	return 0;
}

void
bufattr_markmeta(bufattr_t bap)
{
	SET(bap->ba_flags, BA_META);
}

int
bufattr_delayidlesleep(bufattr_t bap)
{
	if ((bap->ba_flags & BA_DELAYIDLESLEEP)) {
		return 1;
	}
	return 0;
}

bufattr_t
buf_attr(buf_t bp)
{
	return &bp->b_attr;
}

void
buf_markstatic(buf_t bp __unused)
{
	SET(bp->b_flags, B_STATICCONTENT);
}

int
buf_static(buf_t bp)
{
	if ((bp->b_flags & B_STATICCONTENT)) {
		return 1;
	}
	return 0;
}

void
bufattr_markgreedymode(bufattr_t bap)
{
	SET(bap->ba_flags, BA_GREEDY_MODE);
}

int
bufattr_greedymode(bufattr_t bap)
{
	if ((bap->ba_flags & BA_GREEDY_MODE)) {
		return 1;
	}
	return 0;
}

void
bufattr_markisochronous(bufattr_t bap)
{
	SET(bap->ba_flags, BA_ISOCHRONOUS);
}

int
bufattr_isochronous(bufattr_t bap)
{
	if ((bap->ba_flags & BA_ISOCHRONOUS)) {
		return 1;
	}
	return 0;
}

void
bufattr_markquickcomplete(bufattr_t bap)
{
	SET(bap->ba_flags, BA_QUICK_COMPLETE);
}

int
bufattr_quickcomplete(bufattr_t bap)
{
	if ((bap->ba_flags & BA_QUICK_COMPLETE)) {
		return 1;
	}
	return 0;
}

void
bufattr_markioscheduled(bufattr_t bap)
{
	SET(bap->ba_flags, BA_IO_SCHEDULED);
}


int
bufattr_ioscheduled(bufattr_t bap)
{
	if ((bap->ba_flags & BA_IO_SCHEDULED)) {
		return 1;
	}
	return 0;
}

void
bufattr_markexpeditedmeta(bufattr_t bap)
{
	SET(bap->ba_flags, BA_EXPEDITED_META_IO);
}

int
bufattr_expeditedmeta(bufattr_t bap)
{
	if ((bap->ba_flags & BA_EXPEDITED_META_IO)) {
		return 1;
	}
	return 0;
}

errno_t
buf_error(buf_t bp)
{
	return bp->b_error;
}

void
buf_seterror(buf_t bp, errno_t error)
{
	if ((bp->b_error = error)) {
		SET(bp->b_flags, B_ERROR);
	} else {
		CLR(bp->b_flags, B_ERROR);
	}
}

void
buf_setflags(buf_t bp, int32_t flags)
{
	SET(bp->b_flags, (flags & BUF_X_WRFLAGS));
}

void
buf_clearflags(buf_t bp, int32_t flags)
{
	CLR(bp->b_flags, (flags & BUF_X_WRFLAGS));
}

int32_t
buf_flags(buf_t bp)
{
	return bp->b_flags & BUF_X_RDFLAGS;
}

void
buf_reset(buf_t bp, int32_t io_flags)
{
	CLR(bp->b_flags, (B_READ | B_WRITE | B_ERROR | B_DONE | B_INVAL | B_ASYNC | B_NOCACHE | B_FUA));
	SET(bp->b_flags, (io_flags & (B_ASYNC | B_READ | B_WRITE | B_NOCACHE)));

	bp->b_error = 0;
}

uint32_t
buf_count(buf_t bp)
{
	return bp->b_bcount;
}

void
buf_setcount(buf_t bp, uint32_t bcount)
{
	bp->b_bcount = bcount;
}

uint32_t
buf_size(buf_t bp)
{
	return bp->b_bufsize;
}

void
buf_setsize(buf_t bp, uint32_t bufsize)
{
	bp->b_bufsize = bufsize;
}

uint32_t
buf_resid(buf_t bp)
{
	return bp->b_resid;
}

void
buf_setresid(buf_t bp, uint32_t resid)
{
	bp->b_resid = resid;
}

uint32_t
buf_dirtyoff(buf_t bp)
{
	return bp->b_dirtyoff;
}

uint32_t
buf_dirtyend(buf_t bp)
{
	return bp->b_dirtyend;
}

void
buf_setdirtyoff(buf_t bp, uint32_t dirtyoff)
{
	bp->b_dirtyoff = dirtyoff;
}

void
buf_setdirtyend(buf_t bp, uint32_t dirtyend)
{
	bp->b_dirtyend = dirtyend;
}

uintptr_t
buf_dataptr(buf_t bp)
{
	return bp->b_datap;
}

void
buf_setdataptr(buf_t bp, uintptr_t data)
{
	bp->b_datap = data;
}

vnode_t
buf_vnode(buf_t bp)
{
	return bp->b_vp;
}

void
buf_setvnode(buf_t bp, vnode_t vp)
{
	bp->b_vp = vp;
}


void *
buf_callback(buf_t bp)
{
	if (!(bp->b_flags & B_CALL)) {
		return (void *) NULL;
	}

	return (void *)bp->b_iodone;
}


errno_t
buf_setcallback(buf_t bp, void (*callback)(buf_t, void *), void *transaction)
{
	assert(!ISSET(bp->b_flags, B_FILTER) && ISSET(bp->b_lflags, BL_BUSY));

	if (callback) {
		bp->b_flags |= (B_CALL | B_ASYNC);
	} else {
		bp->b_flags &= ~B_CALL;
	}
	bp->b_transaction = transaction;
	bp->b_iodone = callback;

	return 0;
}

errno_t
buf_setupl(buf_t bp, upl_t upl, uint32_t offset)
{
	if (!(bp->b_lflags & BL_IOBUF)) {
		return EINVAL;
	}

	if (upl) {
		bp->b_flags |= B_CLUSTER;
	} else {
		bp->b_flags &= ~B_CLUSTER;
	}
	bp->b_upl = upl;
	bp->b_uploffset = offset;

	return 0;
}

buf_t
buf_clone(buf_t bp, int io_offset, int io_size, void (*iodone)(buf_t, void *), void *arg)
{
	buf_t   io_bp;
	int add1, add2;

	if (io_offset < 0 || io_size < 0) {
		return NULL;
	}

	if ((unsigned)(io_offset + io_size) > (unsigned)bp->b_bcount) {
		return NULL;
	}

	if (bp->b_flags & B_CLUSTER) {
		if (io_offset && ((bp->b_uploffset + io_offset) & PAGE_MASK)) {
			return NULL;
		}

		if (os_add_overflow(io_offset, io_size, &add1) || os_add_overflow(add1, bp->b_uploffset, &add2)) {
			return NULL;
		}
		if ((add2 & PAGE_MASK) && ((uint32_t)add1 < (uint32_t)bp->b_bcount)) {
			return NULL;
		}
	}
	io_bp = alloc_io_buf(bp->b_vp, 0);

	io_bp->b_flags = bp->b_flags & (B_COMMIT_UPL | B_META | B_PAGEIO | B_CLUSTER | B_PHYS | B_RAW | B_ASYNC | B_READ | B_FUA);

	if (iodone) {
		io_bp->b_transaction = arg;
		io_bp->b_iodone = iodone;
		io_bp->b_flags |= B_CALL;
	}
	if (bp->b_flags & B_CLUSTER) {
		io_bp->b_upl = bp->b_upl;
		io_bp->b_uploffset = bp->b_uploffset + io_offset;
	} else {
		io_bp->b_datap  = (uintptr_t)(((char *)bp->b_datap) + io_offset);
	}
	io_bp->b_bcount = io_size;

	return io_bp;
}


int
buf_shadow(buf_t bp)
{
	if (bp->b_lflags & BL_SHADOW) {
		return 1;
	}
	return 0;
}


buf_t
buf_create_shadow_priv(buf_t bp, boolean_t force_copy, uintptr_t external_storage, void (*iodone)(buf_t, void *), void *arg)
{
	return buf_create_shadow_internal(bp, force_copy, external_storage, iodone, arg, 1);
}

buf_t
buf_create_shadow(buf_t bp, boolean_t force_copy, uintptr_t external_storage, void (*iodone)(buf_t, void *), void *arg)
{
	return buf_create_shadow_internal(bp, force_copy, external_storage, iodone, arg, 0);
}


static buf_t
buf_create_shadow_internal(buf_t bp, boolean_t force_copy, uintptr_t external_storage, void (*iodone)(buf_t, void *), void *arg, int priv)
{
	buf_t   io_bp;

	KERNEL_DEBUG(0xbbbbc000 | DBG_FUNC_START, bp, 0, 0, 0, 0);

	if (!(bp->b_flags & B_META) || (bp->b_lflags & BL_IOBUF)) {
		KERNEL_DEBUG(0xbbbbc000 | DBG_FUNC_END, bp, 0, 0, 0, 0);
		return NULL;
	}
#ifdef BUF_MAKE_PRIVATE
	if (bp->b_shadow_ref && bp->b_data_ref == 0 && external_storage == 0) {
		panic("buf_create_shadow: %p is in the private state (%d, %d)", bp, bp->b_shadow_ref, bp->b_data_ref);
	}
#endif
	io_bp = alloc_io_buf(bp->b_vp, priv);

	io_bp->b_flags = bp->b_flags & (B_META | B_ZALLOC | B_ASYNC | B_READ | B_FUA);
	io_bp->b_blkno = bp->b_blkno;
	io_bp->b_lblkno = bp->b_lblkno;

	if (iodone) {
		io_bp->b_transaction = arg;
		io_bp->b_iodone = iodone;
		io_bp->b_flags |= B_CALL;
	}
	if (force_copy == FALSE) {
		io_bp->b_bcount = bp->b_bcount;
		io_bp->b_bufsize = bp->b_bufsize;

		if (external_storage) {
			io_bp->b_datap = external_storage;
#ifdef BUF_MAKE_PRIVATE
			io_bp->b_data_store = NULL;
#endif
		} else {
			io_bp->b_datap = bp->b_datap;
#ifdef BUF_MAKE_PRIVATE
			io_bp->b_data_store = bp;
#endif
		}
		*(buf_t *)(&io_bp->b_orig) = bp;

		lck_mtx_lock_spin(&buf_mtx);

		io_bp->b_lflags |= BL_SHADOW;
		io_bp->b_shadow = bp->b_shadow;
		bp->b_shadow = io_bp;
		bp->b_shadow_ref++;

#ifdef BUF_MAKE_PRIVATE
		if (external_storage) {
			io_bp->b_lflags |= BL_EXTERNAL;
		} else {
			bp->b_data_ref++;
		}
#endif
		lck_mtx_unlock(&buf_mtx);
	} else {
		if (external_storage) {
#ifdef BUF_MAKE_PRIVATE
			io_bp->b_lflags |= BL_EXTERNAL;
#endif
			io_bp->b_bcount = bp->b_bcount;
			io_bp->b_bufsize = bp->b_bufsize;
			io_bp->b_datap = external_storage;
		} else {
			allocbuf(io_bp, bp->b_bcount);

			io_bp->b_lflags |= BL_IOBUF_ALLOC;
		}
		bcopy((caddr_t)bp->b_datap, (caddr_t)io_bp->b_datap, bp->b_bcount);

#ifdef BUF_MAKE_PRIVATE
		io_bp->b_data_store = NULL;
#endif
	}
	KERNEL_DEBUG(0xbbbbc000 | DBG_FUNC_END, bp, bp->b_shadow_ref, 0, io_bp, 0);

	return io_bp;
}


#ifdef BUF_MAKE_PRIVATE
errno_t
buf_make_private(buf_t bp)
{
	buf_t   ds_bp;
	buf_t   t_bp;
	struct buf my_buf;

	KERNEL_DEBUG(0xbbbbc004 | DBG_FUNC_START, bp, bp->b_shadow_ref, 0, 0, 0);

	if (bp->b_shadow_ref == 0 || bp->b_data_ref == 0 || ISSET(bp->b_lflags, BL_SHADOW)) {
		KERNEL_DEBUG(0xbbbbc004 | DBG_FUNC_END, bp, bp->b_shadow_ref, 0, EINVAL, 0);
		return EINVAL;
	}
	my_buf.b_flags = B_META;
	my_buf.b_datap = (uintptr_t)NULL;
	allocbuf(&my_buf, bp->b_bcount);

	bcopy((caddr_t)bp->b_datap, (caddr_t)my_buf.b_datap, bp->b_bcount);

	lck_mtx_lock_spin(&buf_mtx);

	for (t_bp = bp->b_shadow; t_bp; t_bp = t_bp->b_shadow) {
		if (!ISSET(bp->b_lflags, BL_EXTERNAL)) {
			break;
		}
	}
	ds_bp = t_bp;

	if (ds_bp == NULL && bp->b_data_ref) {
		panic("buf_make_private: b_data_ref != 0 && ds_bp == NULL");
	}

	if (ds_bp && (bp->b_data_ref == 0 || bp->b_shadow_ref == 0)) {
		panic("buf_make_private: ref_count == 0 && ds_bp != NULL");
	}

	if (ds_bp == NULL) {
		lck_mtx_unlock(&buf_mtx);

		buf_free_meta_store(&my_buf);

		KERNEL_DEBUG(0xbbbbc004 | DBG_FUNC_END, bp, bp->b_shadow_ref, 0, EINVAL, 0);
		return EINVAL;
	}
	for (t_bp = bp->b_shadow; t_bp; t_bp = t_bp->b_shadow) {
		if (!ISSET(t_bp->b_lflags, BL_EXTERNAL)) {
			t_bp->b_data_store = ds_bp;
		}
	}
	ds_bp->b_data_ref = bp->b_data_ref;

	bp->b_data_ref = 0;
	bp->b_datap = my_buf.b_datap;

	lck_mtx_unlock(&buf_mtx);

	KERNEL_DEBUG(0xbbbbc004 | DBG_FUNC_END, bp, bp->b_shadow_ref, 0, 0, 0);
	return 0;
}
#endif


void
buf_setfilter(buf_t bp, void (*filter)(buf_t, void *), void *transaction,
    void(**old_iodone)(buf_t, void *), void **old_transaction)
{
	assert(ISSET(bp->b_lflags, BL_BUSY));

	if (old_iodone) {
		*old_iodone = bp->b_iodone;
	}
	if (old_transaction) {
		*old_transaction = bp->b_transaction;
	}

	bp->b_transaction = transaction;
	bp->b_iodone = filter;
	if (filter) {
		bp->b_flags |= B_FILTER;
	} else {
		bp->b_flags &= ~B_FILTER;
	}
}


daddr64_t
buf_blkno(buf_t bp)
{
	return bp->b_blkno;
}

daddr64_t
buf_lblkno(buf_t bp)
{
	return bp->b_lblkno;
}

void
buf_setblkno(buf_t bp, daddr64_t blkno)
{
	bp->b_blkno = blkno;
}

void
buf_setlblkno(buf_t bp, daddr64_t lblkno)
{
	bp->b_lblkno = lblkno;
}

dev_t
buf_device(buf_t bp)
{
	return bp->b_dev;
}

errno_t
buf_setdevice(buf_t bp, vnode_t vp)
{
	if ((vp->v_type != VBLK) && (vp->v_type != VCHR)) {
		return EINVAL;
	}
	bp->b_dev = vp->v_rdev;

	return 0;
}


void *
buf_drvdata(buf_t bp)
{
	return bp->b_drvdata;
}

void
buf_setdrvdata(buf_t bp, void *drvdata)
{
	bp->b_drvdata = drvdata;
}

void *
buf_fsprivate(buf_t bp)
{
	return bp->b_fsprivate;
}

void
buf_setfsprivate(buf_t bp, void *fsprivate)
{
	bp->b_fsprivate = fsprivate;
}

kauth_cred_t
buf_rcred(buf_t bp)
{
	return bp->b_rcred;
}

kauth_cred_t
buf_wcred(buf_t bp)
{
	return bp->b_wcred;
}

void *
buf_upl(buf_t bp)
{
	return bp->b_upl;
}

uint32_t
buf_uploffset(buf_t bp)
{
	return (uint32_t)(bp->b_uploffset);
}

proc_t
buf_proc(buf_t bp)
{
	return bp->b_proc;
}


errno_t
buf_map(buf_t bp, caddr_t *io_addr)
{
	buf_t           real_bp;
	vm_offset_t     vaddr;
	kern_return_t   kret;

	if (!(bp->b_flags & B_CLUSTER)) {
		*io_addr = (caddr_t)bp->b_datap;
		return 0;
	}
	real_bp = (buf_t)(bp->b_real_bp);

	if (real_bp && real_bp->b_datap) {
		  
                                                
                                                         
                                                      
                                                         
                                  
     
		*io_addr = (caddr_t)real_bp->b_datap;
		return 0;
	}
	kret = ubc_upl_map(bp->b_upl, &vaddr);                   

	if (kret != KERN_SUCCESS) {
		*io_addr = NULL;

		return ENOMEM;
	}
	vaddr += bp->b_uploffset;

	*io_addr = (caddr_t)vaddr;

	return 0;
}

errno_t
buf_unmap(buf_t bp)
{
	buf_t           real_bp;
	kern_return_t   kret;

	if (!(bp->b_flags & B_CLUSTER)) {
		return 0;
	}
	  
                                   
    
	real_bp = (buf_t)(bp->b_real_bp);

	if (real_bp && real_bp->b_datap) {
		return 0;
	}

	if ((bp->b_lflags & BL_IOBUF) &&
	    ((bp->b_flags & (B_PAGEIO | B_READ)) != (B_PAGEIO | B_READ))) {
		  
                                             
                                                
                
    
                                          
                                      
                                          
                                          
     
		bp->b_flags |= B_AGE;
	}
	kret = ubc_upl_unmap(bp->b_upl);

	if (kret != KERN_SUCCESS) {
		return EINVAL;
	}
	return 0;
}


void
buf_clear(buf_t bp)
{
	caddr_t baddr;

	if (buf_map(bp, &baddr) == 0) {
		bzero(baddr, bp->b_bcount);
		buf_unmap(bp);
	}
	bp->b_resid = 0;
}

  
                                                         
                                                
   
static int
buf_strategy_fragmented(vnode_t devvp, buf_t bp, off_t f_offset, size_t contig_bytes)
{
	vnode_t vp = buf_vnode(bp);
	buf_t   io_bp;                                                              
	int     io_direction;
	int     io_resid;
	size_t  io_contig_bytes;
	daddr64_t io_blkno;
	int     error = 0;
	int     bmap_flags;

	  
                                                        
                                        
                            
    
	io_blkno = bp->b_blkno;
	  
                                                   
                                                
    
	bp->b_blkno = bp->b_lblkno;

	  
                                         
    
	io_bp = alloc_io_buf(devvp, 0);

	io_bp->b_lblkno = bp->b_lblkno;
	io_bp->b_datap  = bp->b_datap;
	io_resid        = bp->b_bcount;
	io_direction    = bp->b_flags & B_READ;
	io_contig_bytes = contig_bytes;

	if (bp->b_flags & B_READ) {
		bmap_flags = VNODE_READ;
	} else {
		bmap_flags = VNODE_WRITE;
	}

	for (;;) {
		if (io_blkno == -1) {
			  
                                                 
      
			bzero((caddr_t)io_bp->b_datap, (int)io_contig_bytes);
		} else {
			io_bp->b_bcount  = (uint32_t)io_contig_bytes;
			io_bp->b_bufsize = (uint32_t)io_contig_bytes;
			io_bp->b_resid   = (uint32_t)io_contig_bytes;
			io_bp->b_blkno   = io_blkno;

			buf_reset(io_bp, io_direction);

			  
                                                                                                          
      

			if (!ISSET(bp->b_flags, B_READ)) {
				OSAddAtomic(1, &devvp->v_numoutput);
			}

			if ((error = VNOP_STRATEGY(io_bp))) {
				break;
			}
			if ((error = (int)buf_biowait(io_bp))) {
				break;
			}
			if (io_bp->b_resid) {
				io_resid -= (io_contig_bytes - io_bp->b_resid);
				break;
			}
		}
		if ((io_resid -= io_contig_bytes) == 0) {
			break;
		}
		f_offset       += io_contig_bytes;
		io_bp->b_datap += io_contig_bytes;

		  
                                                        
     
		if ((error = VNOP_BLOCKMAP(vp, f_offset, io_resid, &io_blkno, &io_contig_bytes, NULL, bmap_flags, NULL))) {
			break;
		}
	}
	buf_free(io_bp);

	if (error) {
		buf_seterror(bp, error);
	}
	bp->b_resid = io_resid;
	  
                            
    
	buf_biodone(bp);

	return error;
}


  
                              
                         
         
   
errno_t
buf_strategy(vnode_t devvp, void *ap)
{
	buf_t   bp = ((struct vnop_strategy_args *)ap)->a_bp;
	vnode_t vp = bp->b_vp;
	int     bmap_flags;
	errno_t error;
#if CONFIG_DTRACE
	int dtrace_io_start_flag = 0;                                          
                                                                       
                                                             
                                     

#endif

	if (vp == NULL || vp->v_type == VCHR || vp->v_type == VBLK) {
		panic("buf_strategy: b_vp == NULL || vtype == VCHR | VBLK\n");
	}
	  
                                      
                                    
                             
    
	bp->b_dev = devvp->v_rdev;

	if (bp->b_flags & B_READ) {
		bmap_flags = VNODE_READ;
	} else {
		bmap_flags = VNODE_WRITE;
	}

	if (!(bp->b_flags & B_CLUSTER)) {
		if ((bp->b_upl)) {
			  
                                           
                                           
                                         
                                        
      
			DTRACE_IO1(start, buf_t, bp);
			return cluster_bp(bp);
		}
		if (bp->b_blkno == bp->b_lblkno) {
			off_t       f_offset;
			size_t  contig_bytes;

			if ((error = VNOP_BLKTOOFF(vp, bp->b_lblkno, &f_offset))) {
				DTRACE_IO1(start, buf_t, bp);
				buf_seterror(bp, error);
				buf_biodone(bp);

				return error;
			}

			if ((error = VNOP_BLOCKMAP(vp, f_offset, bp->b_bcount, &bp->b_blkno, &contig_bytes, NULL, bmap_flags, NULL))) {
				DTRACE_IO1(start, buf_t, bp);
				buf_seterror(bp, error);
				buf_biodone(bp);

				return error;
			}

			DTRACE_IO1(start, buf_t, bp);
#if CONFIG_DTRACE
			dtrace_io_start_flag = 1;
#endif                    

			if ((bp->b_blkno == -1) || (contig_bytes == 0)) {
				                                             
				bp->b_blkno = -1;
				buf_clear(bp);
			} else if (contig_bytes < (size_t)bp->b_bcount) {
				return buf_strategy_fragmented(devvp, bp, f_offset, contig_bytes);
			}
		}

#if CONFIG_DTRACE
		if (dtrace_io_start_flag == 0) {
			DTRACE_IO1(start, buf_t, bp);
			dtrace_io_start_flag = 1;
		}
#endif                    

		if (bp->b_blkno == -1) {
			buf_biodone(bp);
			return 0;
		}
	}

#if CONFIG_DTRACE
	if (dtrace_io_start_flag == 0) {
		DTRACE_IO1(start, buf_t, bp);
	}
#endif                    

#if CONFIG_PROTECT
	                                    
	cpx_t cpx = bufattr_cpx(buf_attr(bp));
	if (cpx) {
		                                      
		if (cpx_use_offset_for_iv(cpx) && !cpx_synthetic_offset_for_iv(cpx)) {
			off_t f_offset;
			if ((error = VNOP_BLKTOOFF(bp->b_vp, bp->b_lblkno, &f_offset))) {
				return error;
			}

			  
                                                 
                                                      
                                                  
                                                 
                                                        
                                             
     
                                                  
                                                            
                                                          
                                                           
                                       
     
                                                          
                                                         
                                                           
                                          
      
			buf_setcpoff(bp, f_offset);
			CP_DEBUG((CPDBG_OFFSET_IO | DBG_FUNC_NONE), (uint32_t) f_offset, (uint32_t) bp->b_lblkno, (uint32_t) bp->b_blkno, (uint32_t) bp->b_bcount, 0);
		}
	}
#endif

	  
                                   
                                 
                                      
                                            
                                           
                          
    
	error = VOCALL(devvp->v_op, VOFFSET(vnop_strategy), ap);
	DTRACE_FSINFO(strategy, vnode_t, vp);
	return error;
}



buf_t
buf_alloc(vnode_t vp)
{
	return alloc_io_buf(vp, is_vm_privileged());
}

void
buf_free(buf_t bp)
{
	free_io_buf(bp);
}


  
                                        
                                                
                                                
                                                      
                                                                                        
   

struct buf_iterate_info_t {
	int flag;
	struct buflists *listhead;
};

void
buf_iterate(vnode_t vp, int (*callout)(buf_t, void *), int flags, void *arg)
{
	buf_t   bp;
	int     retval;
	struct  buflists local_iterblkhd;
	int     lock_flags = BAC_NOWAIT | BAC_REMOVE;
	int     notify_busy = flags & BUF_NOTIFY_BUSY;
	struct buf_iterate_info_t list[2];
	int     num_lists, i;

	if (flags & BUF_SKIP_LOCKED) {
		lock_flags |= BAC_SKIP_LOCKED;
	}
	if (flags & BUF_SKIP_NONLOCKED) {
		lock_flags |= BAC_SKIP_NONLOCKED;
	}

	if (!(flags & (BUF_SCAN_DIRTY | BUF_SCAN_CLEAN))) {
		flags |= BUF_SCAN_DIRTY;
	}

	num_lists = 0;

	if (flags & BUF_SCAN_DIRTY) {
		list[num_lists].flag = VBI_DIRTY;
		list[num_lists].listhead = &vp->v_dirtyblkhd;
		num_lists++;
	}
	if (flags & BUF_SCAN_CLEAN) {
		list[num_lists].flag = VBI_CLEAN;
		list[num_lists].listhead = &vp->v_cleanblkhd;
		num_lists++;
	}

	for (i = 0; i < num_lists; i++) {
		lck_mtx_lock(&buf_mtx);

		if (buf_iterprepare(vp, &local_iterblkhd, list[i].flag)) {
			lck_mtx_unlock(&buf_mtx);
			continue;
		}
		while (!LIST_EMPTY(&local_iterblkhd)) {
			bp = LIST_FIRST(&local_iterblkhd);
			LIST_REMOVE(bp, b_vnbufs);
			LIST_INSERT_HEAD(list[i].listhead, bp, b_vnbufs);

			if (buf_acquire_locked(bp, lock_flags, 0, 0)) {
				if (notify_busy) {
					bp = NULL;
				} else {
					continue;
				}
			}

			lck_mtx_unlock(&buf_mtx);

			retval = callout(bp, arg);

			switch (retval) {
			case BUF_RETURNED:
				if (bp) {
					buf_brelse(bp);
				}
				break;
			case BUF_CLAIMED:
				break;
			case BUF_RETURNED_DONE:
				if (bp) {
					buf_brelse(bp);
				}
				lck_mtx_lock(&buf_mtx);
				goto out;
			case BUF_CLAIMED_DONE:
				lck_mtx_lock(&buf_mtx);
				goto out;
			}
			lck_mtx_lock(&buf_mtx);
		}                                
out:
		buf_itercomplete(vp, &local_iterblkhd, list[i].flag);
		lck_mtx_unlock(&buf_mtx);
	}                    
}                  


  
                                                                
   
int
buf_invalidateblks(vnode_t vp, int flags, int slpflag, int slptimeo)
{
	buf_t   bp;
	int     aflags;
	int     error = 0;
	int     must_rescan = 1;
	struct  buflists local_iterblkhd;


	if (LIST_EMPTY(&vp->v_cleanblkhd) && LIST_EMPTY(&vp->v_dirtyblkhd)) {
		return 0;
	}

	lck_mtx_lock(&buf_mtx);

	for (;;) {
		if (must_rescan == 0) {
			  
                                                             
                                                            
                                                             
                                                           
                                                     
      
			break;
		}
		if (LIST_EMPTY(&vp->v_cleanblkhd) && LIST_EMPTY(&vp->v_dirtyblkhd)) {
			break;
		}
		must_rescan = 0;
		  
                           
     
		if (buf_iterprepare(vp, &local_iterblkhd, VBI_CLEAN)) {
			goto try_dirty_list;
		}
		while (!LIST_EMPTY(&local_iterblkhd)) {
			bp = LIST_FIRST(&local_iterblkhd);

			LIST_REMOVE(bp, b_vnbufs);
			LIST_INSERT_HEAD(&vp->v_cleanblkhd, bp, b_vnbufs);

			  
                                                                                   
      
			if ((flags & BUF_SKIP_META) && (bp->b_lblkno < 0 || ISSET(bp->b_flags, B_META))) {
				continue;
			}

			aflags = BAC_REMOVE;

			if (!(flags & BUF_INVALIDATE_LOCKED)) {
				aflags |= BAC_SKIP_LOCKED;
			}

			if ((error = (int)buf_acquire_locked(bp, aflags, slpflag, slptimeo))) {
				if (error == EDEADLK) {
					  
                                          
                                     
                               
        
					continue;
				}
				if (error == EAGAIN) {
					  
                                             
                                          
                                                   
        
					must_rescan++;
					continue;
				}
				  
                                                      
                                                                     
       
				buf_itercomplete(vp, &local_iterblkhd, VBI_CLEAN);

				lck_mtx_unlock(&buf_mtx);
				return error;
			}
			lck_mtx_unlock(&buf_mtx);

			if (bp->b_flags & B_LOCKED) {
				KERNEL_DEBUG(0xbbbbc038, bp, 0, 0, 0, 0);
			}

			CLR(bp->b_flags, B_LOCKED);
			SET(bp->b_flags, B_INVAL);
			buf_brelse(bp);

			lck_mtx_lock(&buf_mtx);

			  
                                       
                                              
                                             
                                
      
			must_rescan++;
		}
		buf_itercomplete(vp, &local_iterblkhd, VBI_CLEAN);

try_dirty_list:
		  
                              
     
		if (buf_iterprepare(vp, &local_iterblkhd, VBI_DIRTY)) {
			continue;
		}
		while (!LIST_EMPTY(&local_iterblkhd)) {
			bp = LIST_FIRST(&local_iterblkhd);

			LIST_REMOVE(bp, b_vnbufs);
			LIST_INSERT_HEAD(&vp->v_dirtyblkhd, bp, b_vnbufs);

			  
                                                                                   
      
			if ((flags & BUF_SKIP_META) && (bp->b_lblkno < 0 || ISSET(bp->b_flags, B_META))) {
				continue;
			}

			aflags = BAC_REMOVE;

			if (!(flags & BUF_INVALIDATE_LOCKED)) {
				aflags |= BAC_SKIP_LOCKED;
			}

			if ((error = (int)buf_acquire_locked(bp, aflags, slpflag, slptimeo))) {
				if (error == EDEADLK) {
					  
                                          
                                     
                               
        
					continue;
				}
				if (error == EAGAIN) {
					  
                                             
                                          
                                                   
        
					must_rescan++;
					continue;
				}
				  
                                                      
                                                                     
       
				buf_itercomplete(vp, &local_iterblkhd, VBI_DIRTY);

				lck_mtx_unlock(&buf_mtx);
				return error;
			}
			lck_mtx_unlock(&buf_mtx);

			if (bp->b_flags & B_LOCKED) {
				KERNEL_DEBUG(0xbbbbc038, bp, 0, 0, 1, 0);
			}

			CLR(bp->b_flags, B_LOCKED);
			SET(bp->b_flags, B_INVAL);

			if (ISSET(bp->b_flags, B_DELWRI) && (flags & BUF_WRITE_DATA)) {
				(void) VNOP_BWRITE(bp);
			} else {
				buf_brelse(bp);
			}

			lck_mtx_lock(&buf_mtx);
			  
                                       
                                              
                                             
                                
      
			must_rescan++;
		}
		buf_itercomplete(vp, &local_iterblkhd, VBI_DIRTY);
	}
	lck_mtx_unlock(&buf_mtx);

	return 0;
}

void
buf_flushdirtyblks(vnode_t vp, int wait, int flags, const char *msg)
{
	(void) buf_flushdirtyblks_skipinfo(vp, wait, flags, msg);
	return;
}

int
buf_flushdirtyblks_skipinfo(vnode_t vp, int wait, int flags, const char *msg)
{
	buf_t   bp;
	int     writes_issued = 0;
	errno_t error;
	int     busy = 0;
	struct  buflists local_iterblkhd;
	int     lock_flags = BAC_NOWAIT | BAC_REMOVE;
	int any_locked = 0;

	if (flags & BUF_SKIP_LOCKED) {
		lock_flags |= BAC_SKIP_LOCKED;
	}
	if (flags & BUF_SKIP_NONLOCKED) {
		lock_flags |= BAC_SKIP_NONLOCKED;
	}
loop:
	lck_mtx_lock(&buf_mtx);

	if (buf_iterprepare(vp, &local_iterblkhd, VBI_DIRTY) == 0) {
		while (!LIST_EMPTY(&local_iterblkhd)) {
			bp = LIST_FIRST(&local_iterblkhd);
			LIST_REMOVE(bp, b_vnbufs);
			LIST_INSERT_HEAD(&vp->v_dirtyblkhd, bp, b_vnbufs);

			if ((error = buf_acquire_locked(bp, lock_flags, 0, 0)) == EBUSY) {
				busy++;
			}
			if (error) {
				  
                                                             
                                                                      
                                                               
                                                                   
                                    
      
                                                                      
                                                             
       
				if (error == EDEADLK) {
					any_locked++;
				}
				continue;
			}
			lck_mtx_unlock(&buf_mtx);

			bp->b_flags &= ~B_LOCKED;

			  
                                                               
                                                           
      
			if ((bp->b_vp == vp) || (wait == 0)) {
				(void) buf_bawrite(bp);
			} else {
				(void) VNOP_BWRITE(bp);
			}
			writes_issued++;

			lck_mtx_lock(&buf_mtx);
		}
		buf_itercomplete(vp, &local_iterblkhd, VBI_DIRTY);
	}
	lck_mtx_unlock(&buf_mtx);

	if (wait) {
		(void)vnode_waitforwrites(vp, 0, 0, 0, msg);

		if (vp->v_dirtyblkhd.lh_first && busy) {
			  
                                        
                                        
                                          
                                        
                                  
                                     
                                             
                                             
                                     
      
			if (writes_issued == 0) {
				(void)tsleep((caddr_t)&vp->v_numoutput,
				    PRIBIO + 1, "vnode_flushdirtyblks", hz / 20);
			}
			writes_issued = 0;
			busy = 0;

			goto loop;
		}
	}

	return any_locked;
}


  
                              
                                            
   
static int
buf_iterprepare(vnode_t vp, struct buflists *iterheadp, int flags)
{
	struct buflists * listheadp;

	if (flags & VBI_DIRTY) {
		listheadp = &vp->v_dirtyblkhd;
	} else {
		listheadp = &vp->v_cleanblkhd;
	}

	while (vp->v_iterblkflags & VBI_ITER) {
		vp->v_iterblkflags |= VBI_ITERWANT;
		msleep(&vp->v_iterblkflags, &buf_mtx, 0, "buf_iterprepare", NULL);
	}
	if (LIST_EMPTY(listheadp)) {
		LIST_INIT(iterheadp);
		return EINVAL;
	}
	vp->v_iterblkflags |= VBI_ITER;

	iterheadp->lh_first = listheadp->lh_first;
	listheadp->lh_first->b_vnbufs.le_prev = &iterheadp->lh_first;
	LIST_INIT(listheadp);

	return 0;
}

  
                              
                                            
   
static void
buf_itercomplete(vnode_t vp, struct buflists *iterheadp, int flags)
{
	struct buflists * listheadp;
	buf_t bp;

	if (flags & VBI_DIRTY) {
		listheadp = &vp->v_dirtyblkhd;
	} else {
		listheadp = &vp->v_cleanblkhd;
	}

	while (!LIST_EMPTY(iterheadp)) {
		bp = LIST_FIRST(iterheadp);
		LIST_REMOVE(bp, b_vnbufs);
		LIST_INSERT_HEAD(listheadp, bp, b_vnbufs);
	}
	vp->v_iterblkflags &= ~VBI_ITER;

	if (vp->v_iterblkflags & VBI_ITERWANT) {
		vp->v_iterblkflags &= ~VBI_ITERWANT;
		wakeup(&vp->v_iterblkflags);
	}
}


static void
bremfree_locked(buf_t bp)
{
	struct bqueues *dp = NULL;
	int whichq;

	whichq = bp->b_whichq;

	if (whichq == -1) {
		if (bp->b_shadow_ref == 0) {
			panic("bremfree_locked: %p not on freelist", bp);
		}
		  
                                         
                                            
                                            
     
		return;
	}
	  
                                                            
                                                              
                                                  
   
                                                                   
    
	if (bp->b_freelist.tqe_next == NULL) {
		dp = &bufqueues[whichq];

		if (dp->tqh_last != &bp->b_freelist.tqe_next) {
			panic("bremfree: lost tail");
		}
	}
	TAILQ_REMOVE(dp, bp, b_freelist);

	if (whichq == BQ_LAUNDRY) {
		blaundrycnt--;
	}

	bp->b_whichq = -1;
	bp->b_timestamp = 0;
	bp->b_shadow = 0;
}

  
                                   
                                  
   
static void
bgetvp_locked(vnode_t vp, buf_t bp)
{
	if (bp->b_vp != vp) {
		panic("bgetvp_locked: not free");
	}

	if (vp->v_type == VBLK || vp->v_type == VCHR) {
		bp->b_dev = vp->v_rdev;
	} else {
		bp->b_dev = NODEV;
	}
	  
                                   
    
	bufinsvn(bp, &vp->v_cleanblkhd);
}

  
                                      
                                  
   
static void
brelvp_locked(buf_t bp)
{
	  
                                          
    
	if (bp->b_vnbufs.le_next != NOLIST) {
		bufremvn(bp);
	}

	bp->b_vp = (vnode_t)NULL;
}

  
                                               
                                                   
                                                       
   
static void
buf_reassign(buf_t bp, vnode_t newvp)
{
	struct buflists *listheadp;

	if (newvp == NULL) {
		printf("buf_reassign: NULL");
		return;
	}
	lck_mtx_lock_spin(&buf_mtx);

	  
                                          
    
	if (bp->b_vnbufs.le_next != NOLIST) {
		bufremvn(bp);
	}
	  
                                           
                                                
    
	if (ISSET(bp->b_flags, B_DELWRI)) {
		listheadp = &newvp->v_dirtyblkhd;
	} else {
		listheadp = &newvp->v_cleanblkhd;
	}
	bufinsvn(bp, listheadp);

	lck_mtx_unlock(&buf_mtx);
}

static __inline__ void
bufhdrinit(buf_t bp)
{
	bzero((char *)bp, sizeof *bp);
	bp->b_dev = NODEV;
	bp->b_rcred = NOCRED;
	bp->b_wcred = NOCRED;
	bp->b_vnbufs.le_next = NOLIST;
	bp->b_flags = B_INVAL;

	return;
}

  
                                                 
   
__private_extern__ void
bufinit(void)
{
	buf_t   bp;
	struct bqueues *dp;
	int     i;

	nbuf_headers = 0;
	                                                                   
	for (dp = bufqueues; dp < &bufqueues[BQUEUES]; dp++) {
		TAILQ_INIT(dp);
	}
	bufhashtbl = hashinit(nbuf_hashelements, M_CACHE, &bufhash);

	buf_busycount = 0;

	                                   
	for (i = 0; i < max_nbuf_headers; i++) {
		nbuf_headers++;
		bp = &buf_headers[i];
		bufhdrinit(bp);

		BLISTNONE(bp);
		dp = &bufqueues[BQ_EMPTY];
		bp->b_whichq = BQ_EMPTY;
		bp->b_timestamp = buf_timestamp();
		binsheadfree(bp, dp, BQ_EMPTY);
		binshash(bp, &invalhash);
	}
	boot_nbuf_headers = nbuf_headers;

	TAILQ_INIT(&iobufqueue);
	TAILQ_INIT(&delaybufqueue);

	for (; i < nbuf_headers + niobuf_headers; i++) {
		bp = &buf_headers[i];
		bufhdrinit(bp);
		bp->b_whichq = -1;
		binsheadfree(bp, &iobufqueue, -1);
	}

	  
                                                            
    
	cluster_init();

	printf("using %d buffer headers and %d cluster IO buffer headers\n",
	    nbuf_headers, niobuf_headers);

	                                  
	bcleanbuf_thread_init();

	                                                  
	if (vm_set_buffer_cleanup_callout(buffer_cache_gc) != KERN_SUCCESS) {
		panic("Couldn't register buffer cache callout for vm pressure!\n");
	}
}

  
                                  
   

#define MINMETA 512
#define MAXMETA 16384

KALLOC_HEAP_DEFINE(KHEAP_VFS_BIO, "vfs_bio", KHEAP_ID_DATA_BUFFERS);

static struct buf *
bio_doread(vnode_t vp, daddr64_t blkno, int size, kauth_cred_t cred, int async, int queuetype)
{
	buf_t   bp;

	bp = buf_getblk(vp, blkno, size, 0, 0, queuetype);

	  
                                                     
                                                                 
                                                                    
    
	if (!ISSET(bp->b_flags, (B_DONE | B_DELWRI))) {
		struct proc *p;

		p = current_proc();

		                                                     
		SET(bp->b_flags, B_READ | async);
		if (IS_VALID_CRED(cred) && !IS_VALID_CRED(bp->b_rcred)) {
			kauth_cred_ref(cred);
			bp->b_rcred = cred;
		}

		VNOP_STRATEGY(bp);

		trace(TR_BREADMISS, pack(vp, size), blkno);

		                       
		if (p && p->p_stats) {
			OSIncrementAtomicLong(&p->p_stats->p_ru.ru_inblock);                     
		}

		if (async) {
			  
                                     
                                    
                                     
                         
      
			bp = NULL;
		}
	} else if (async) {
		buf_brelse(bp);
		bp = NULL;
	}

	trace(TR_BREADHIT, pack(vp, size), blkno);

	return bp;
}

  
                                                            
                                                                         
   
static errno_t
do_breadn_for_type(vnode_t vp, daddr64_t blkno, int size, daddr64_t *rablks, int *rasizes,
    int nrablks, kauth_cred_t cred, buf_t *bpp, int queuetype)
{
	buf_t   bp;
	int     i;

	bp = *bpp = bio_doread(vp, blkno, size, cred, 0, queuetype);

	  
                                                                  
    
	for (i = 0; i < nrablks; i++) {
		                                                   
		if (incore(vp, rablks[i])) {
			continue;
		}

		                                           
		(void) bio_doread(vp, rablks[i], rasizes[i], cred, B_ASYNC, queuetype);
	}

	                                                                      
	return buf_biowait(bp);
}


  
                     
                                           
   
errno_t
buf_bread(vnode_t vp, daddr64_t blkno, int size, kauth_cred_t cred, buf_t *bpp)
{
	buf_t   bp;

	                           
	bp = *bpp = bio_doread(vp, blkno, size, cred, 0, BLK_READ);

	                                                       
	return buf_biowait(bp);
}

  
                                             
                                           
   
errno_t
buf_meta_bread(vnode_t vp, daddr64_t blkno, int size, kauth_cred_t cred, buf_t *bpp)
{
	buf_t   bp;

	                           
	bp = *bpp = bio_doread(vp, blkno, size, cred, 0, BLK_META);

	                                                       
	return buf_biowait(bp);
}

  
                                                                      
   
errno_t
buf_breadn(vnode_t vp, daddr64_t blkno, int size, daddr64_t *rablks, int *rasizes, int nrablks, kauth_cred_t cred, buf_t *bpp)
{
	return do_breadn_for_type(vp, blkno, size, rablks, rasizes, nrablks, cred, bpp, BLK_READ);
}

  
                                                                      
                               
   
errno_t
buf_meta_breadn(vnode_t vp, daddr64_t blkno, int size, daddr64_t *rablks, int *rasizes, int nrablks, kauth_cred_t cred, buf_t *bpp)
{
	return do_breadn_for_type(vp, blkno, size, rablks, rasizes, nrablks, cred, bpp, BLK_META);
}

  
                                         
   
errno_t
buf_bwrite(buf_t bp)
{
	int     sync, wasdelayed;
	errno_t rv;
	proc_t  p = current_proc();
	vnode_t vp = bp->b_vp;

	if (bp->b_datap == 0) {
		if (brecover_data(bp) == 0) {
			return 0;
		}
	}
	                                                  
	sync = !ISSET(bp->b_flags, B_ASYNC);
	wasdelayed = ISSET(bp->b_flags, B_DELWRI);
	CLR(bp->b_flags, (B_READ | B_DONE | B_ERROR | B_DELWRI));

	if (wasdelayed) {
		OSAddAtomicLong(-1, &nbdwrite);
	}

	if (!sync) {
		  
                                                           
                                                         
                                                           
                                                     
     
		if (wasdelayed) {
			buf_reassign(bp, vp);
		} else if (p && p->p_stats) {
			OSIncrementAtomicLong(&p->p_stats->p_ru.ru_oublock);                     
		}
	}
	trace(TR_BUFWRITE, pack(vp, bp->b_bcount), bp->b_lblkno);

	                                                                       

	OSAddAtomic(1, &vp->v_numoutput);

	VNOP_STRATEGY(bp);

	if (sync) {
		  
                                                     
     
		rv = buf_biowait(bp);

		  
                                                              
                                                                 
                           
     
		if (wasdelayed) {
			buf_reassign(bp, vp);
		} else if (p && p->p_stats) {
			OSIncrementAtomicLong(&p->p_stats->p_ru.ru_oublock);                     
		}

		                         
		buf_brelse(bp);

		return rv;
	} else {
		return 0;
	}
}

int
vn_bwrite(struct vnop_bwrite_args *ap)
{
	return buf_bwrite(ap->a_bp);
}

  
                 
  
                                                         
                                                          
                                                          
                            
  
                                                     
                                                      
  
                                              
  
                                                       
                                                           
                                                                  
                                                                      
                                                                         
   
int
bdwrite_internal(buf_t bp, int return_error)
{
	proc_t  p  = current_proc();
	vnode_t vp = bp->b_vp;

	  
                                         
                                    
                             
                                                         
    
	if (!ISSET(bp->b_flags, B_DELWRI)) {
		SET(bp->b_flags, B_DELWRI);
		if (p && p->p_stats) {
			OSIncrementAtomicLong(&p->p_stats->p_ru.ru_oublock);             
		}
		OSAddAtomicLong(1, &nbdwrite);
		buf_reassign(bp, vp);
	}

	  
                                                               
                                                            
                                                           
                                                           
                                                            
                                                              
                                                                   
                            
   
                                                                          
                                                              
                              
    
	if (!ISSET(bp->b_flags, B_LOCKED) && nbdwrite > ((nbuf_headers / 4) * 3)) {
		if (return_error) {
			return EAGAIN;
		}
		  
                                                             
                                   
     
		(void)vnode_waitforwrites(vp, VNODE_ASYNC_THROTTLE, 0, 0, "buf_bdwrite");

		return buf_bawrite(bp);
	}

	                                                                     
	SET(bp->b_flags, B_DONE);
	buf_brelse(bp);
	return 0;
}

errno_t
buf_bdwrite(buf_t bp)
{
	return bdwrite_internal(bp, 0);
}


  
                                                               
  
                                                        
                                                           
                                                                  
                                             
                                                                
              
   
static int
bawrite_internal(buf_t bp, int throttle)
{
	vnode_t vp = bp->b_vp;

	if (vp) {
		if (throttle) {
			  
                                                              
                                    
      
			(void)vnode_waitforwrites(vp, VNODE_ASYNC_THROTTLE, 0, 0, (const char *)"buf_bawrite");
		} else if (vp->v_numoutput >= VNODE_ASYNC_THROTTLE) {
			  
                              
                               
      
			return EWOULDBLOCK;
		}
	}
	SET(bp->b_flags, B_ASYNC);

	return VNOP_BWRITE(bp);
}

errno_t
buf_bawrite(buf_t bp)
{
	return bawrite_internal(bp, 1);
}



static void
buf_free_meta_store(buf_t bp)
{
	if (bp->b_bufsize) {
		uintptr_t datap = bp->b_datap;
		int bufsize = bp->b_bufsize;

		bp->b_datap = (uintptr_t)NULL;
		bp->b_bufsize = 0;

		  
                                                           
                               
     
		OSMemoryBarrier();

		if (ISSET(bp->b_flags, B_ZALLOC)) {
			kheap_free(KHEAP_VFS_BIO, datap, bufsize);
		} else {
			kmem_free(kernel_map, datap, bufsize);
		}
	}
}


static buf_t
buf_brelse_shadow(buf_t bp)
{
	buf_t   bp_head;
	buf_t   bp_temp;
	buf_t   bp_return = NULL;
#ifdef BUF_MAKE_PRIVATE
	buf_t   bp_data;
	int     data_ref = 0;
#endif
	int need_wakeup = 0;

	lck_mtx_lock_spin(&buf_mtx);

	__IGNORE_WCASTALIGN(bp_head = (buf_t)bp->b_orig);

	if (bp_head->b_whichq != -1) {
		panic("buf_brelse_shadow: bp_head on freelist %d\n", bp_head->b_whichq);
	}

#ifdef BUF_MAKE_PRIVATE
	if (bp_data = bp->b_data_store) {
		bp_data->b_data_ref--;
		  
                                                   
                                                      
                                               
     
		data_ref = bp_data->b_data_ref;
	}
#endif
	KERNEL_DEBUG(0xbbbbc008 | DBG_FUNC_START, bp, bp_head, bp_head->b_shadow_ref, 0, 0);

	bp_head->b_shadow_ref--;

	for (bp_temp = bp_head; bp_temp && bp != bp_temp->b_shadow; bp_temp = bp_temp->b_shadow) {
		;
	}

	if (bp_temp == NULL) {
		panic("buf_brelse_shadow: bp not on list %p", bp_head);
	}

	bp_temp->b_shadow = bp_temp->b_shadow->b_shadow;

#ifdef BUF_MAKE_PRIVATE
	  
                                                                  
                                                                 
                                                            
    
	if (bp == bp_data && data_ref) {
		if ((bp_data = bp_head->b_shadow) == NULL) {
			panic("buf_brelse_shadow: data_ref mismatch bp(%p)", bp);
		}

		for (bp_temp = bp_data; bp_temp; bp_temp = bp_temp->b_shadow) {
			bp_temp->b_data_store = bp_data;
		}
		bp_data->b_data_ref = data_ref;
	}
#endif
	if (bp_head->b_shadow_ref == 0 && bp_head->b_shadow) {
		panic("buf_relse_shadow: b_shadow != NULL && b_shadow_ref == 0  bp(%p)", bp);
	}
	if (bp_head->b_shadow_ref && bp_head->b_shadow == 0) {
		panic("buf_relse_shadow: b_shadow == NULL && b_shadow_ref != 0  bp(%p)", bp);
	}

	if (bp_head->b_shadow_ref == 0) {
		if (!ISSET(bp_head->b_lflags, BL_BUSY)) {
			CLR(bp_head->b_flags, B_AGE);
			bp_head->b_timestamp = buf_timestamp();

			if (ISSET(bp_head->b_flags, B_LOCKED)) {
				bp_head->b_whichq = BQ_LOCKED;
				binstailfree(bp_head, &bufqueues[BQ_LOCKED], BQ_LOCKED);
			} else {
				bp_head->b_whichq = BQ_META;
				binstailfree(bp_head, &bufqueues[BQ_META], BQ_META);
			}
		} else if (ISSET(bp_head->b_lflags, BL_WAITSHADOW)) {
			CLR(bp_head->b_lflags, BL_WAITSHADOW);

			bp_return = bp_head;
		}
		if (ISSET(bp_head->b_lflags, BL_WANTED_REF)) {
			CLR(bp_head->b_lflags, BL_WANTED_REF);
			need_wakeup = 1;
		}
	}
	lck_mtx_unlock(&buf_mtx);

	if (need_wakeup) {
		wakeup(bp_head);
	}

#ifdef BUF_MAKE_PRIVATE
	if (bp == bp_data && data_ref == 0) {
		buf_free_meta_store(bp);
	}

	bp->b_data_store = NULL;
#endif
	KERNEL_DEBUG(0xbbbbc008 | DBG_FUNC_END, bp, 0, 0, 0, 0);

	return bp_return;
}


  
                                         
                             
   
void
buf_brelse(buf_t bp)
{
	struct bqueues *bufq;
	int    whichq;
	upl_t   upl;
	int need_wakeup = 0;
	int need_bp_wakeup = 0;


	if (bp->b_whichq != -1 || !(bp->b_lflags & BL_BUSY)) {
		panic("buf_brelse: bad buffer = %p\n", bp);
	}

#ifdef JOE_DEBUG
	(void) OSBacktrace(&bp->b_stackbrelse[0], 6);

	bp->b_lastbrelse = current_thread();
	bp->b_tag = 0;
#endif
	if (bp->b_lflags & BL_IOBUF) {
		buf_t   shadow_master_bp = NULL;

		if (ISSET(bp->b_lflags, BL_SHADOW)) {
			shadow_master_bp = buf_brelse_shadow(bp);
		} else if (ISSET(bp->b_lflags, BL_IOBUF_ALLOC)) {
			buf_free_meta_store(bp);
		}
		free_io_buf(bp);

		if (shadow_master_bp) {
			bp = shadow_master_bp;
			goto finish_shadow_master;
		}
		return;
	}

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 388)) | DBG_FUNC_START,
	    bp->b_lblkno * PAGE_SIZE, bp, bp->b_datap,
	    bp->b_flags, 0);

	trace(TR_BRELSE, pack(bp->b_vp, bp->b_bufsize), bp->b_lblkno);

	  
                                                            
                                                          
                
   
                                        
    
	if (ISSET(bp->b_flags, B_META) && ISSET(bp->b_flags, B_INVAL)) {
		if (ISSET(bp->b_flags, B_FILTER)) {                                 
			void    (*iodone_func)(struct buf *, void *) = bp->b_iodone;
			void    *arg = bp->b_transaction;

			CLR(bp->b_flags, B_FILTER);                                
			bp->b_iodone = NULL;
			bp->b_transaction = NULL;

			if (iodone_func == NULL) {
				panic("brelse: bp @ %p has NULL b_iodone!\n", bp);
			}
			(*iodone_func)(bp, arg);
		}
	}
	  
                                      
    
	upl = bp->b_upl;

	if (!ISSET(bp->b_flags, B_META) && UBCINFOEXISTS(bp->b_vp) && bp->b_bufsize) {
		kern_return_t kret;
		int           upl_flags;

		if (upl == NULL) {
			if (!ISSET(bp->b_flags, B_INVAL)) {
				kret = ubc_create_upl_kernel(bp->b_vp,
				    ubc_blktooff(bp->b_vp, bp->b_lblkno),
				    bp->b_bufsize,
				    &upl,
				    NULL,
				    UPL_PRECIOUS,
				    VM_KERN_MEMORY_FILE);

				if (kret != KERN_SUCCESS) {
					panic("brelse: Failed to create UPL");
				}
#if  UPL_DEBUG
				upl_ubc_alias_set(upl, (uintptr_t) bp, (uintptr_t) 5);
#endif                
			}
		} else {
			if (bp->b_datap) {
				kret = ubc_upl_unmap(upl);

				if (kret != KERN_SUCCESS) {
					panic("ubc_upl_unmap failed");
				}
				bp->b_datap = (uintptr_t)NULL;
			}
		}
		if (upl) {
			if (bp->b_flags & (B_ERROR | B_INVAL)) {
				if (bp->b_flags & (B_READ | B_INVAL)) {
					upl_flags = UPL_ABORT_DUMP_PAGES;
				} else {
					upl_flags = 0;
				}

				ubc_upl_abort(upl, upl_flags);
			} else {
				if (ISSET(bp->b_flags, B_DELWRI | B_WASDIRTY)) {
					upl_flags = UPL_COMMIT_SET_DIRTY;
				} else {
					upl_flags = UPL_COMMIT_CLEAR_DIRTY;
				}

				ubc_upl_commit_range(upl, 0, bp->b_bufsize, upl_flags |
				    UPL_COMMIT_INACTIVATE | UPL_COMMIT_FREE_ON_EMPTY);
			}
			bp->b_upl = NULL;
		}
	} else {
		if ((upl)) {
			panic("brelse: UPL set for non VREG; vp=%p", bp->b_vp);
		}
	}

	  
                                                           
    
	if (ISSET(bp->b_flags, (B_LOCKED | B_ERROR)) == (B_LOCKED | B_ERROR)) {
		CLR(bp->b_flags, B_ERROR);
	}
	  
                                                        
    
	if (ISSET(bp->b_flags, (B_NOCACHE | B_ERROR))) {
		SET(bp->b_flags, B_INVAL);
	}

	if ((bp->b_bufsize <= 0) ||
	    ISSET(bp->b_flags, B_INVAL) ||
	    (ISSET(bp->b_lflags, BL_WANTDEALLOC) && !ISSET(bp->b_flags, B_DELWRI))) {
		boolean_t       delayed_buf_free_meta_store = FALSE;

		  
                                                            
                                       
                                                    
     
		if (ISSET(bp->b_flags, B_DELWRI)) {
			OSAddAtomicLong(-1, &nbdwrite);
		}

		if (ISSET(bp->b_flags, B_META)) {
			if (bp->b_shadow_ref) {
				delayed_buf_free_meta_store = TRUE;
			} else {
				buf_free_meta_store(bp);
			}
		}
		  
                                         
     
		buf_release_credentials(bp);

		lck_mtx_lock_spin(&buf_mtx);

		if (bp->b_shadow_ref) {
			SET(bp->b_lflags, BL_WAITSHADOW);

			lck_mtx_unlock(&buf_mtx);

			return;
		}
		if (delayed_buf_free_meta_store == TRUE) {
			lck_mtx_unlock(&buf_mtx);
finish_shadow_master:
			buf_free_meta_store(bp);

			lck_mtx_lock_spin(&buf_mtx);
		}
		CLR(bp->b_flags, (B_META | B_ZALLOC | B_DELWRI | B_LOCKED | B_AGE | B_ASYNC | B_NOCACHE | B_FUA));

		if (bp->b_vp) {
			brelvp_locked(bp);
		}

		bremhash(bp);
		BLISTNONE(bp);
		binshash(bp, &invalhash);

		bp->b_whichq = BQ_EMPTY;
		binsheadfree(bp, &bufqueues[BQ_EMPTY], BQ_EMPTY);
	} else {
		  
                                                             
                                                               
     
		if (ISSET(bp->b_flags, B_LOCKED)) {
			whichq = BQ_LOCKED;                                 
		} else if (ISSET(bp->b_flags, B_META)) {
			whichq = BQ_META;                              
		} else if (ISSET(bp->b_flags, B_AGE)) {
			whichq = BQ_AGE;                                          
		} else {
			whichq = BQ_LRU;                                
		}
		bufq = &bufqueues[whichq];

		bp->b_timestamp = buf_timestamp();

		lck_mtx_lock_spin(&buf_mtx);

		  
                                                           
                                                                 
                                                                
                                                                  
                                                                   
                                                                
                                                                 
     
		if (bp->b_shadow_ref == 0) {
			CLR(bp->b_flags, (B_AGE | B_ASYNC | B_NOCACHE));
			bp->b_whichq = whichq;
			binstailfree(bp, bufq, whichq);
		} else {
			  
                                             
                                            
                                             
                    
      
			CLR(bp->b_flags, (B_ASYNC | B_NOCACHE));
		}
	}
	if (needbuffer) {
		  
                           
                                                
                                              
                    
     
		needbuffer = 0;
		need_wakeup = 1;
	}
	if (ISSET(bp->b_lflags, BL_WANTED)) {
		  
                                           
                                            
     
		need_bp_wakeup = 1;
	}
	  
                      
    
	CLR(bp->b_lflags, (BL_BUSY | BL_WANTED));
	buf_busycount--;

	lck_mtx_unlock(&buf_mtx);

	if (need_wakeup) {
		  
                                                                 
     
		wakeup(&needbuffer);
	}
	if (need_bp_wakeup) {
		  
                                                                    
     
		wakeup(bp);
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 388)) | DBG_FUNC_END,
	    bp, bp->b_datap, bp->b_flags, 0, 0);
}

  
                                        
                                                                    
                                                                        
                                                                    
               
   
static boolean_t
incore(vnode_t vp, daddr64_t blkno)
{
	boolean_t retval;
	struct  bufhashhdr *dp;

	dp = BUFHASH(vp, blkno);

	lck_mtx_lock_spin(&buf_mtx);

	if (incore_locked(vp, blkno, dp)) {
		retval = TRUE;
	} else {
		retval = FALSE;
	}
	lck_mtx_unlock(&buf_mtx);

	return retval;
}


static buf_t
incore_locked(vnode_t vp, daddr64_t blkno, struct bufhashhdr *dp)
{
	struct buf *bp;

	                       
	for (bp = dp->lh_first; bp != NULL; bp = bp->b_hash.le_next) {
		if (bp->b_lblkno == blkno && bp->b_vp == vp &&
		    !ISSET(bp->b_flags, B_INVAL)) {
			return bp;
		}
	}
	return NULL;
}


void
buf_wait_for_shadow_io(vnode_t vp, daddr64_t blkno)
{
	buf_t bp;
	struct  bufhashhdr *dp;

	dp = BUFHASH(vp, blkno);

	lck_mtx_lock_spin(&buf_mtx);

	for (;;) {
		if ((bp = incore_locked(vp, blkno, dp)) == NULL) {
			break;
		}

		if (bp->b_shadow_ref == 0) {
			break;
		}

		SET(bp->b_lflags, BL_WANTED_REF);

		(void) msleep(bp, &buf_mtx, PSPIN | (PRIBIO + 1), "buf_wait_for_shadow", NULL);
	}
	lck_mtx_unlock(&buf_mtx);
}

                                                                            
  
                                                        
                                                        
                                                          
                                                         
                                                          
                                        
   
buf_t
buf_getblk(vnode_t vp, daddr64_t blkno, int size, int slpflag, int slptimeo, int operation)
{
	buf_t bp;
	int   err;
	upl_t upl;
	upl_page_info_t *pl;
	kern_return_t kret;
	int ret_only_valid;
	struct timespec ts;
	int upl_flags;
	struct  bufhashhdr *dp;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 386)) | DBG_FUNC_START,
	    (uintptr_t)(blkno * PAGE_SIZE), size, operation, 0, 0);

	ret_only_valid = operation & BLK_ONLYVALID;
	operation &= ~BLK_ONLYVALID;
	dp = BUFHASH(vp, blkno);
start:
	lck_mtx_lock_spin(&buf_mtx);

	if ((bp = incore_locked(vp, blkno, dp))) {
		  
                              
     
		if (ISSET(bp->b_lflags, BL_BUSY)) {
			  
                 
      
			switch (operation) {
			case BLK_READ:
			case BLK_WRITE:
			case BLK_META:
				SET(bp->b_lflags, BL_WANTED);
				bufstats.bufs_busyincore++;

				  
                                                     
                               
       
				ts.tv_sec = (slptimeo / 1000);
				ts.tv_nsec = (slptimeo % 1000) * 10  * NSEC_PER_USEC * 1000;

				KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 396)) | DBG_FUNC_NONE,
				    (uintptr_t)blkno, size, operation, 0, 0);

				err = msleep(bp, &buf_mtx, slpflag | PDROP | (PRIBIO + 1), "buf_getblk", &ts);

				  
                                                  
                                            
       
				if (err && ((slpflag & PCATCH) || ((err == EWOULDBLOCK) && slptimeo))) {
					return NULL;
				}
				goto start;
			              

			default:
				  
                                  
       
				panic("getblk: paging or unknown operation for incore busy buffer - %x\n", operation);
				              
				break;
			}
		} else {
			int clear_bdone;

			  
                                 
      
			SET(bp->b_lflags, BL_BUSY);
			SET(bp->b_flags, B_CACHE);
			buf_busycount++;

			bremfree_locked(bp);
			bufstats.bufs_incore++;

			lck_mtx_unlock(&buf_mtx);
#ifdef JOE_DEBUG
			bp->b_owner = current_thread();
			bp->b_tag   = 1;
#endif
			if ((bp->b_upl)) {
				panic("buffer has UPL, but not marked BUSY: %p", bp);
			}

			clear_bdone = FALSE;
			if (!ret_only_valid) {
				  
                                                  
                                                 
                                                     
                          
      
                                                      
                                                       
                                                                
                                                 
       
				if (operation == BLK_META && bp->b_bcount < (uint32_t)size) {
					  
                                                          
                                                              
                                 
        
					if (ISSET(bp->b_flags, B_DELWRI)) {
						CLR(bp->b_flags, B_CACHE);
						buf_bwrite(bp);
						goto start;
					}
					  
                                          
                                            
                                          
        
					clear_bdone = TRUE;
				}

				if (bp->b_bufsize != (uint32_t)size) {
					allocbuf(bp, size);
				}
			}

			upl_flags = 0;
			switch (operation) {
			case BLK_WRITE:
				  
                                                
                                               
                                   
       
				upl_flags |= UPL_WILL_MODIFY;
				OS_FALLTHROUGH;
			case BLK_READ:
				upl_flags |= UPL_PRECIOUS;
				if (UBCINFOEXISTS(bp->b_vp) && bp->b_bufsize) {
					kret = ubc_create_upl_kernel(vp,
					    ubc_blktooff(vp, bp->b_lblkno),
					    bp->b_bufsize,
					    &upl,
					    &pl,
					    upl_flags,
					    VM_KERN_MEMORY_FILE);
					if (kret != KERN_SUCCESS) {
						panic("Failed to create UPL");
					}

					bp->b_upl = upl;

					if (upl_valid_page(pl, 0)) {
						if (upl_dirty_page(pl, 0)) {
							SET(bp->b_flags, B_WASDIRTY);
						} else {
							CLR(bp->b_flags, B_WASDIRTY);
						}
					} else {
						CLR(bp->b_flags, (B_DONE | B_CACHE | B_WASDIRTY | B_DELWRI));
					}

					kret = ubc_upl_map(upl, (vm_offset_t*)&(bp->b_datap));

					if (kret != KERN_SUCCESS) {
						panic("getblk: ubc_upl_map() failed with (%d)", kret);
					}
				}
				break;

			case BLK_META:
				  
                                                 
                                    
       
				break;

			default:
				panic("getblk: paging or unknown operation for incore buffer- %d\n", operation);
				              
				break;
			}

			if (clear_bdone) {
				CLR(bp->b_flags, B_DONE);
			}
		}
	} else {                   
		int queue = BQ_EMPTY;                               

		if (ret_only_valid) {
			lck_mtx_unlock(&buf_mtx);
			return NULL;
		}
		if ((vnode_isreg(vp) == 0) || (UBCINFOEXISTS(vp) == 0)                                 ) {
			operation = BLK_META;
		}

		if ((bp = getnewbuf(slpflag, slptimeo, &queue)) == NULL) {
			goto start;
		}

		  
                                                             
                                                       
                                                          
                                                          
                                                 
     
		if (incore_locked(vp, blkno, dp)) {
			SET(bp->b_flags, B_INVAL);
			binshash(bp, &invalhash);

			lck_mtx_unlock(&buf_mtx);

			buf_brelse(bp);
			goto start;
		}
		  
                                                      
                               
     

		  
                                           
                                                            
     
		if (operation == BLK_META) {
			SET(bp->b_flags, B_META);
		}

		bp->b_blkno = bp->b_lblkno = blkno;
		bp->b_vp = vp;

		  
                                                    
     
		binshash(bp, BUFHASH(vp, blkno));

		bgetvp_locked(vp, bp);

		lck_mtx_unlock(&buf_mtx);

		allocbuf(bp, size);

		upl_flags = 0;
		switch (operation) {
		case BLK_META:
			  
                               
     
                                             
                                              
                                               
                                           
                              
      
			OSAddAtomicLong(1, &bufstats.bufs_miss);
			break;

		case BLK_WRITE:
			  
                                                    
                                                     
                      
      
			upl_flags |= UPL_WILL_MODIFY;
			OS_FALLTHROUGH;
		case BLK_READ:
		{     off_t   f_offset;
		      size_t  contig_bytes;
		      int     bmap_flags;

#if DEVELOPMENT || DEBUG
			  
                                                                     
                        
      
		      const char* excldfs[] = {"hfs", "afpfs", "smbfs", "acfs",
			                       "exfat", "msdos", "webdav", NULL};

		      for (int i = 0; excldfs[i] != NULL; i++) {
			      if (vp->v_mount &&
			          !strcmp(vp->v_mount->mnt_vfsstat.f_fstypename,
			          excldfs[i])) {
				      panic("%s %s calls buf_getblk",
				          excldfs[i],
				          operation == BLK_READ ? "BLK_READ" : "BLK_WRITE");
			      }
		      }
#endif

		      if ((bp->b_upl)) {
			      panic("bp already has UPL: %p", bp);
		      }

		      f_offset = ubc_blktooff(vp, blkno);

		      upl_flags |= UPL_PRECIOUS;
		      kret = ubc_create_upl_kernel(vp,
			  f_offset,
			  bp->b_bufsize,
			  &upl,
			  &pl,
			  upl_flags,
			  VM_KERN_MEMORY_FILE);

		      if (kret != KERN_SUCCESS) {
			      panic("Failed to create UPL");
		      }
#if  UPL_DEBUG
		      upl_ubc_alias_set(upl, (uintptr_t) bp, (uintptr_t) 4);
#endif                
		      bp->b_upl = upl;

		      if (upl_valid_page(pl, 0)) {
			      if (operation == BLK_READ) {
				      bmap_flags = VNODE_READ;
			      } else {
				      bmap_flags = VNODE_WRITE;
			      }

			      SET(bp->b_flags, B_CACHE | B_DONE);

			      OSAddAtomicLong(1, &bufstats.bufs_vmhits);

			      bp->b_validoff = 0;
			      bp->b_dirtyoff = 0;

			      if (upl_dirty_page(pl, 0)) {
				                         
				      SET(bp->b_flags, B_WASDIRTY);

				      bp->b_validend = bp->b_bcount;
				      bp->b_dirtyend = bp->b_bcount;
			      } else {
				                         
				      bp->b_validend = bp->b_bcount;
				      bp->b_dirtyend = 0;
			      }
			        
                                                                     
                          
            
			      if (VNOP_BLOCKMAP(vp, f_offset, bp->b_bcount, &bp->b_blkno, &contig_bytes, NULL, bmap_flags, NULL)) {
				      panic("getblk: VNOP_BLOCKMAP failed");
			      }
			        
                                                    
                                                      
                                                          
                                
            
			      if ((uint32_t)contig_bytes < bp->b_bcount) {
				      bp->b_blkno = bp->b_lblkno;
			      }
		      } else {
			      OSAddAtomicLong(1, &bufstats.bufs_miss);
		      }
		      kret = ubc_upl_map(upl, (vm_offset_t *)&(bp->b_datap));

		      if (kret != KERN_SUCCESS) {
			      panic("getblk: ubc_upl_map() failed with (%d)", kret);
		      }
		      break;}                
		default:
			panic("getblk: paging or unknown operation - %x", operation);
			              
			break;
		}              
	}                    

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 386)) | DBG_FUNC_END,
	    bp, bp->b_datap, bp->b_flags, 3, 0);

#ifdef JOE_DEBUG
	(void) OSBacktrace(&bp->b_stackgetblk[0], 6);
#endif
	return bp;
}

  
                                                    
   
buf_t
buf_geteblk(int size)
{
	buf_t   bp = NULL;
	int queue = BQ_EMPTY;

	do {
		lck_mtx_lock_spin(&buf_mtx);

		bp = getnewbuf(0, 0, &queue);
	} while (bp == NULL);

	SET(bp->b_flags, (B_META | B_INVAL));

#if DIAGNOSTIC
	assert(queue == BQ_EMPTY);
#endif                 
	                                                           

	binshash(bp, &invalhash);
	bufstats.bufs_eblk++;

	lck_mtx_unlock(&buf_mtx);

	allocbuf(bp, size);

	return bp;
}

uint32_t
buf_redundancy_flags(buf_t bp)
{
	return bp->b_redundancy_flags;
}

void
buf_set_redundancy_flags(buf_t bp, uint32_t flags)
{
	SET(bp->b_redundancy_flags, flags);
}

void
buf_clear_redundancy_flags(buf_t bp, uint32_t flags)
{
	CLR(bp->b_redundancy_flags, flags);
}



static void *
recycle_buf_from_pool(int nsize)
{
	buf_t   bp;
	void    *ptr = NULL;

	lck_mtx_lock_spin(&buf_mtx);

	TAILQ_FOREACH(bp, &bufqueues[BQ_META], b_freelist) {
		if (ISSET(bp->b_flags, B_DELWRI) || bp->b_bufsize != (uint32_t)nsize) {
			continue;
		}
		ptr = (void *)bp->b_datap;
		bp->b_bufsize = 0;

		bcleanbuf(bp, TRUE);
		break;
	}
	lck_mtx_unlock(&buf_mtx);

	return ptr;
}



int zalloc_nopagewait_failed = 0;
int recycle_buf_failed = 0;

static void *
grab_memory_for_meta_buf(int nsize)
{
	void *ptr;
	boolean_t was_vmpriv;


	  
                                          
                                         
                                          
                                      
                      
    
	was_vmpriv = set_vm_privilege(FALSE);

	ptr = kheap_alloc(KHEAP_VFS_BIO, nsize, Z_NOPAGEWAIT);

	if (was_vmpriv == TRUE) {
		set_vm_privilege(TRUE);
	}

	if (ptr == NULL) {
		zalloc_nopagewait_failed++;

		ptr = recycle_buf_from_pool(nsize);

		if (ptr == NULL) {
			recycle_buf_failed++;

			if (was_vmpriv == FALSE) {
				set_vm_privilege(TRUE);
			}

			ptr = kheap_alloc(KHEAP_VFS_BIO, nsize, Z_WAITOK);

			if (was_vmpriv == FALSE) {
				set_vm_privilege(FALSE);
			}
		}
	}
	return ptr;
}

  
                                                              
                                                      
                                               
                                                           
  
                                                           
                                                              
                                                             
                              
   

int
allocbuf(buf_t bp, int size)
{
	vm_size_t desired_size;

	desired_size = roundup(size, CLBYTES);

	if (desired_size < PAGE_SIZE) {
		desired_size = PAGE_SIZE;
	}
	if (desired_size > MAXBSIZE) {
		panic("allocbuf: buffer larger than MAXBSIZE requested");
	}

	if (ISSET(bp->b_flags, B_META)) {
		int    nsize = roundup(size, MINMETA);

		if (bp->b_datap) {
			void *elem = (void *)bp->b_datap;

			if (ISSET(bp->b_flags, B_ZALLOC)) {
				if (bp->b_bufsize < (uint32_t)nsize) {
					                                 

					if (nsize <= MAXMETA) {
						desired_size = nsize;

						                              
						*(void **)(&bp->b_datap) = grab_memory_for_meta_buf(nsize);
					} else {
						bp->b_datap = (uintptr_t)NULL;
						kmem_alloc_kobject(kernel_map, (vm_offset_t *)&bp->b_datap, desired_size, VM_KERN_MEMORY_FILE);
						CLR(bp->b_flags, B_ZALLOC);
					}
					bcopy(elem, (caddr_t)bp->b_datap, bp->b_bufsize);
					kheap_free(KHEAP_VFS_BIO, elem, bp->b_bufsize);
				} else {
					desired_size = bp->b_bufsize;
				}
			} else {
				if ((vm_size_t)bp->b_bufsize < desired_size) {
					                                 
					bp->b_datap = (uintptr_t)NULL;
					kmem_alloc_kobject(kernel_map, (vm_offset_t *)&bp->b_datap, desired_size, VM_KERN_MEMORY_FILE);
					bcopy(elem, (caddr_t)bp->b_datap, bp->b_bufsize);
					kmem_free(kernel_map, (vm_offset_t)elem, bp->b_bufsize);
				} else {
					desired_size = bp->b_bufsize;
				}
			}
		} else {
			                    
			if (nsize <= MAXMETA) {
				desired_size = nsize;

				                              
				*(void **)(&bp->b_datap) = grab_memory_for_meta_buf(nsize);
				SET(bp->b_flags, B_ZALLOC);
			} else {
				kmem_alloc_kobject(kernel_map, (vm_offset_t *)&bp->b_datap, desired_size, VM_KERN_MEMORY_FILE);
			}
		}

		if (bp->b_datap == 0) {
			panic("allocbuf: NULL b_datap");
		}
	}
	bp->b_bufsize = (uint32_t)desired_size;
	bp->b_bcount = size;

	return 0;
}

  
                                               
  
                                                                              
                                                                          
                                                           
                            
                                                       
                                                      
                                                                        
                              
                                                                   
                          
                                                                    
                                                                   
                         
  
                             
                                                   
                                                         
   

static buf_t
getnewbuf(int slpflag, int slptimeo, int * queue)
{
	buf_t   bp;
	buf_t   lru_bp;
	buf_t   age_bp;
	buf_t   meta_bp;
	int     age_time, lru_time, bp_time, meta_time;
	int     req = *queue;                             
	struct timespec ts;

start:
	  
                                    
    
	if ((*queue >= BQUEUES) || (*queue < 0)
	    || (*queue == BQ_LAUNDRY) || (*queue == BQ_LOCKED)) {
		*queue = BQ_EMPTY;
	}


	if (*queue == BQ_EMPTY && (bp = bufqueues[*queue].tqh_first)) {
		goto found;
	}

	  
                                                                      
    
	if (nbuf_headers < max_nbuf_headers) {
		  
                                 
                               
                             
     
		nbuf_headers++;
		goto add_newbufs;
	}
	                                       
	bp = bufqueues[*queue].tqh_first;
	if (bp) {
		goto found;
	}

	                                   
	age_bp = bufqueues[BQ_AGE].tqh_first;
	lru_bp = bufqueues[BQ_LRU].tqh_first;
	meta_bp = bufqueues[BQ_META].tqh_first;

	if (!age_bp && !lru_bp && !meta_bp) {
		  
                                            
                             
     
		bp = bufqueues[BQ_EMPTY].tqh_first;
		if (bp) {
			*queue = BQ_EMPTY;
			goto found;
		}
		  
                                             
                                              
                                      
     

add_newbufs:
		lck_mtx_unlock(&buf_mtx);

		                                          
		bp = (struct buf *)zalloc(buf_hdr_zone);

		if (bp) {
			bufhdrinit(bp);
			bp->b_whichq = BQ_EMPTY;
			bp->b_timestamp = buf_timestamp();
			BLISTNONE(bp);
			SET(bp->b_flags, B_HDRALLOC);
			*queue = BQ_EMPTY;
		}
		lck_mtx_lock_spin(&buf_mtx);

		if (bp) {
			binshash(bp, &invalhash);
			binsheadfree(bp, &bufqueues[BQ_EMPTY], BQ_EMPTY);
			buf_hdr_count++;
			goto found;
		}
		                                         
		nbuf_headers--;

		bufstats.bufs_sleeps++;

		                                        
		needbuffer = 1;
		                     
		ts.tv_sec = (slptimeo / 1000);
		                                              
		ts.tv_nsec = (slptimeo % 1000) * NSEC_PER_USEC * 1000 * 10;

		msleep(&needbuffer, &buf_mtx, slpflag | PDROP | (PRIBIO + 1), "getnewbuf", &ts);
		return NULL;
	}

	                                                   
	bp = NULL;
	*queue = -1;

	                                           
	if (!age_bp) {
		bp = lru_bp;
		*queue = BQ_LRU;
	} else if (!lru_bp) {
		bp = age_bp;
		*queue = BQ_AGE;
	} else {                                           
		int             t = buf_timestamp();

		age_time = t - age_bp->b_timestamp;
		lru_time = t - lru_bp->b_timestamp;
		if ((age_time < 0) || (lru_time < 0)) {                         
			bp = age_bp;
			*queue = BQ_AGE;
			  
                                                      
                                                
      
		} else {
			if ((lru_time >= lru_is_stale) && (age_time < age_is_stale)) {
				bp = lru_bp;
				*queue = BQ_LRU;
			} else {
				bp = age_bp;
				*queue = BQ_AGE;
			}
		}
	}

	if (!bp) {                                
		bp = meta_bp;
		*queue = BQ_META;
	} else if (meta_bp) {
		int             t = buf_timestamp();

		bp_time = t - bp->b_timestamp;
		meta_time = t - meta_bp->b_timestamp;

		if (!(bp_time < 0) && !(meta_time < 0)) {
			                            
			int bp_is_stale;
			bp_is_stale = (*queue == BQ_LRU) ?
			    lru_is_stale : age_is_stale;

			if ((meta_time >= meta_is_stale) &&
			    (bp_time < bp_is_stale)) {
				bp = meta_bp;
				*queue = BQ_META;
			}
		}
	}
found:
	if (ISSET(bp->b_flags, B_LOCKED) || ISSET(bp->b_lflags, BL_BUSY)) {
		panic("getnewbuf: bp @ %p is LOCKED or BUSY! (flags 0x%x)\n", bp, bp->b_flags);
	}

	              
	if (bcleanbuf(bp, FALSE)) {
		  
                                                  
     
		*queue = req;
		goto start;
	}
	return bp;
}


  
                  
                                       
                                                  
                                
  
                             
                              
   
int
bcleanbuf(buf_t bp, boolean_t discard)
{
	                           
	bremfree_locked(bp);

#ifdef JOE_DEBUG
	bp->b_owner = current_thread();
	bp->b_tag   = 2;
#endif
	  
                                                          
                                         
    
	if (ISSET(bp->b_flags, B_DELWRI)) {
		if (discard) {
			SET(bp->b_lflags, BL_WANTDEALLOC);
		}

		bmovelaundry(bp);

		lck_mtx_unlock(&buf_mtx);

		wakeup(&bufqueues[BQ_LAUNDRY]);
		  
                                
     
		(void)thread_block(THREAD_CONTINUE_NULL);

		lck_mtx_lock_spin(&buf_mtx);

		return 1;
	}
#ifdef JOE_DEBUG
	bp->b_owner = current_thread();
	bp->b_tag   = 8;
#endif
	  
                                                     
    
	SET(bp->b_lflags, BL_BUSY);
	buf_busycount++;

	bremhash(bp);

	  
                                                    
    
	if (bp->b_vp) {
		brelvp_locked(bp);
	}

	lck_mtx_unlock(&buf_mtx);

	BLISTNONE(bp);

	if (ISSET(bp->b_flags, B_META)) {
		buf_free_meta_store(bp);
	}

	trace(TR_BRELSE, pack(bp->b_vp, bp->b_bufsize), bp->b_lblkno);

	buf_release_credentials(bp);

	                                                 
	if (discard) {
		lck_mtx_lock_spin(&buf_mtx);
		CLR(bp->b_flags, (B_META | B_ZALLOC | B_DELWRI | B_LOCKED | B_AGE | B_ASYNC | B_NOCACHE | B_FUA));
		bp->b_whichq = BQ_EMPTY;
		binshash(bp, &invalhash);
		binsheadfree(bp, &bufqueues[BQ_EMPTY], BQ_EMPTY);
		CLR(bp->b_lflags, BL_BUSY);
		buf_busycount--;
	} else {
		                                                    
		bp->b_bufsize = 0;
		bp->b_datap = (uintptr_t)NULL;
		bp->b_upl = (void *)NULL;
		bp->b_fsprivate = (void *)NULL;
		  
                                              
                                       
                                              
                             
     
#ifdef JOE_DEBUG
		bp->b_owner = current_thread();
		bp->b_tag   = 3;
#endif
		bp->b_lflags = BL_BUSY;
		bp->b_flags = (bp->b_flags & B_HDRALLOC);
		bp->b_redundancy_flags = 0;
		bp->b_dev = NODEV;
		bp->b_blkno = bp->b_lblkno = 0;
		bp->b_iodone = NULL;
		bp->b_error = 0;
		bp->b_resid = 0;
		bp->b_bcount = 0;
		bp->b_dirtyoff = bp->b_dirtyend = 0;
		bp->b_validoff = bp->b_validend = 0;
		bzero(&bp->b_attr, sizeof(struct bufattr));

		lck_mtx_lock_spin(&buf_mtx);
	}
	return 0;
}



errno_t
buf_invalblkno(vnode_t vp, daddr64_t lblkno, int flags)
{
	buf_t   bp;
	errno_t error;
	struct bufhashhdr *dp;

	dp = BUFHASH(vp, lblkno);

relook:
	lck_mtx_lock_spin(&buf_mtx);

	if ((bp = incore_locked(vp, lblkno, dp)) == (struct buf *)0) {
		lck_mtx_unlock(&buf_mtx);
		return 0;
	}
	if (ISSET(bp->b_lflags, BL_BUSY)) {
		if (!ISSET(flags, BUF_WAIT)) {
			lck_mtx_unlock(&buf_mtx);
			return EBUSY;
		}
		SET(bp->b_lflags, BL_WANTED);

		error = msleep((caddr_t)bp, &buf_mtx, PDROP | (PRIBIO + 1), "buf_invalblkno", NULL);

		if (error) {
			return error;
		}
		goto relook;
	}
	bremfree_locked(bp);
	SET(bp->b_lflags, BL_BUSY);
	SET(bp->b_flags, B_INVAL);
	buf_busycount++;
#ifdef JOE_DEBUG
	bp->b_owner = current_thread();
	bp->b_tag   = 4;
#endif
	lck_mtx_unlock(&buf_mtx);
	buf_brelse(bp);

	return 0;
}


void
buf_drop(buf_t bp)
{
	int need_wakeup = 0;

	lck_mtx_lock_spin(&buf_mtx);

	if (ISSET(bp->b_lflags, BL_WANTED)) {
		  
                                           
                                            
     
		need_wakeup = 1;
	}
#ifdef JOE_DEBUG
	bp->b_owner = current_thread();
	bp->b_tag   = 9;
#endif
	  
                      
    
	CLR(bp->b_lflags, (BL_BUSY | BL_WANTED));
	buf_busycount--;

	lck_mtx_unlock(&buf_mtx);

	if (need_wakeup) {
		  
                                                                    
     
		wakeup(bp);
	}
}


errno_t
buf_acquire(buf_t bp, int flags, int slpflag, int slptimeo)
{
	errno_t error;

	lck_mtx_lock_spin(&buf_mtx);

	error = buf_acquire_locked(bp, flags, slpflag, slptimeo);

	lck_mtx_unlock(&buf_mtx);

	return error;
}


static errno_t
buf_acquire_locked(buf_t bp, int flags, int slpflag, int slptimeo)
{
	errno_t error;
	struct timespec ts;

	if (ISSET(bp->b_flags, B_LOCKED)) {
		if ((flags & BAC_SKIP_LOCKED)) {
			return EDEADLK;
		}
	} else {
		if ((flags & BAC_SKIP_NONLOCKED)) {
			return EDEADLK;
		}
	}
	if (ISSET(bp->b_lflags, BL_BUSY)) {
		  
                                                 
                                   
                                 
     
		if (flags & BAC_NOWAIT) {
			return EBUSY;
		}
		SET(bp->b_lflags, BL_WANTED);

		                                              
		ts.tv_sec = (slptimeo / 100);
		ts.tv_nsec = (slptimeo % 100) * 10  * NSEC_PER_USEC * 1000;
		error = msleep((caddr_t)bp, &buf_mtx, slpflag | (PRIBIO + 1), "buf_acquire", &ts);

		if (error) {
			return error;
		}
		return EAGAIN;
	}
	if (flags & BAC_REMOVE) {
		bremfree_locked(bp);
	}
	SET(bp->b_lflags, BL_BUSY);
	buf_busycount++;

#ifdef JOE_DEBUG
	bp->b_owner = current_thread();
	bp->b_tag   = 5;
#endif
	return 0;
}


  
                                                 
                                                          
   
errno_t
buf_biowait(buf_t bp)
{
	while (!ISSET(bp->b_flags, B_DONE)) {
		lck_mtx_lock_spin(&buf_mtx);

		if (!ISSET(bp->b_flags, B_DONE)) {
			DTRACE_IO1(wait__start, buf_t, bp);
			(void) msleep(bp, &buf_mtx, PDROP | (PRIBIO + 1), "buf_biowait", NULL);
			DTRACE_IO1(wait__done, buf_t, bp);
		} else {
			lck_mtx_unlock(&buf_mtx);
		}
	}
	                                                                
	if (ISSET(bp->b_flags, B_EINTR)) {
		CLR(bp->b_flags, B_EINTR);
		return EINTR;
	} else if (ISSET(bp->b_flags, B_ERROR)) {
		return bp->b_error ? bp->b_error : EIO;
	} else {
		return 0;
	}
}


  
                                 
  
                                                     
                                                      
  
                                    
                                                               
                                                                 
                                                                    
  
                                                                 
                                                                    
                                                                   
                                                                    
   

void
buf_biodone(buf_t bp)
{
	mount_t mp;
	struct bufattr *bap;
	struct timeval real_elapsed;
	uint64_t real_elapsed_usec = 0;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 387)) | DBG_FUNC_START,
	    bp, bp->b_datap, bp->b_flags, 0, 0);

	if (ISSET(bp->b_flags, B_DONE)) {
		panic("biodone already");
	}

	bap = &bp->b_attr;

	if (bp->b_vp && bp->b_vp->v_mount) {
		mp = bp->b_vp->v_mount;
	} else {
		mp = NULL;
	}

	if (ISSET(bp->b_flags, B_ERROR)) {
		if (mp && (MNT_ROOTFS & mp->mnt_flag)) {
			dk_error_description_t desc;
			bzero(&desc, sizeof(desc));
			desc.description      = panic_disk_error_description;
			desc.description_size = panic_disk_error_description_size;
			VNOP_IOCTL(mp->mnt_devvp, DKIOCGETERRORDESCRIPTION, (caddr_t)&desc, 0, vfs_context_kernel());
		}
	}

	if (mp && (bp->b_flags & B_READ) == 0) {
		update_last_io_time(mp);
		INCR_PENDING_IO(-(pending_io_t)buf_count(bp), mp->mnt_pending_write_size);
	} else if (mp) {
		INCR_PENDING_IO(-(pending_io_t)buf_count(bp), mp->mnt_pending_read_size);
	}

	throttle_info_end_io(bp);

	if (kdebug_enable) {
		int code    = DKIO_DONE;
		int io_tier = GET_BUFATTR_IO_TIER(bap);

		if (bp->b_flags & B_READ) {
			code |= DKIO_READ;
		}
		if (bp->b_flags & B_ASYNC) {
			code |= DKIO_ASYNC;
		}

		if (bp->b_flags & B_META) {
			code |= DKIO_META;
		} else if (bp->b_flags & B_PAGEIO) {
			code |= DKIO_PAGING;
		}

		if (io_tier != 0) {
			code |= DKIO_THROTTLE;
		}

		code |= ((io_tier << DKIO_TIER_SHIFT) & DKIO_TIER_MASK);

		if (bp->b_flags & B_PASSIVE) {
			code |= DKIO_PASSIVE;
		}

		if (bap->ba_flags & BA_NOCACHE) {
			code |= DKIO_NOCACHE;
		}

		if (bap->ba_flags & BA_IO_TIER_UPGRADE) {
			code |= DKIO_TIER_UPGRADE;
		}

		KDBG_RELEASE_NOPROCFILT(FSDBG_CODE(DBG_DKRW, code),
		    buf_kernel_addrperm_addr(bp),
		    (uintptr_t)VM_KERNEL_ADDRPERM(bp->b_vp), bp->b_resid,
		    bp->b_error);
	}

	microuptime(&real_elapsed);
	timevalsub(&real_elapsed, &bp->b_timestamp_tv);
	real_elapsed_usec = real_elapsed.tv_sec * USEC_PER_SEC + real_elapsed.tv_usec;
	disk_conditioner_delay(bp, 1, bp->b_bcount, real_elapsed_usec);

	  
                                  
                                      
                                              
              
    
	CLR(bp->b_flags, (B_WASDIRTY | B_PASSIVE));
	CLR(bap->ba_flags, (BA_META | BA_NOCACHE | BA_DELAYIDLESLEEP | BA_IO_TIER_UPGRADE));

	SET_BUFATTR_IO_TIER(bap, 0);

	DTRACE_IO1(done, buf_t, bp);

	if (!ISSET(bp->b_flags, B_READ) && !ISSET(bp->b_flags, B_RAW)) {
		  
                                 
                                   
             
     
		vnode_writedone(bp->b_vp);
	}

	if (ISSET(bp->b_flags, (B_CALL | B_FILTER))) {                              
		void    (*iodone_func)(struct buf *, void *) = bp->b_iodone;
		void    *arg = bp->b_transaction;
		int     callout = ISSET(bp->b_flags, B_CALL);

		if (iodone_func == NULL) {
			panic("biodone: bp @ %p has NULL b_iodone!\n", bp);
		}

		CLR(bp->b_flags, (B_CALL | B_FILTER));                                         
		bp->b_iodone = NULL;
		bp->b_transaction = NULL;

		if (callout) {
			SET(bp->b_flags, B_DONE);                                
		}
		(*iodone_func)(bp, arg);

		if (callout) {
			  
                                              
                                                                  
      
			goto biodone_done;
		}
		  
                                                  
                                             
                                            
                                                    
                               
     
	}
	if (ISSET(bp->b_flags, B_ASYNC)) {                                
		SET(bp->b_flags, B_DONE);                                

		buf_brelse(bp);
	} else {                                                               
		  
                                      
                                                    
                                       
                                        
                      
                                             
                                   
                                             
                                              
                                              
                                              
                                   
     
		lck_mtx_lock_spin(&buf_mtx);

		CLR(bp->b_lflags, BL_WANTED);
		SET(bp->b_flags, B_DONE);                                        

		lck_mtx_unlock(&buf_mtx);

		wakeup(bp);
	}
biodone_done:
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 387)) | DBG_FUNC_END,
	    (uintptr_t)bp, (uintptr_t)bp->b_datap, bp->b_flags, 0, 0);
}

  
                          
   
vm_offset_t
buf_kernel_addrperm_addr(void * addr)
{
	if ((vm_offset_t)addr == 0) {
		return 0;
	} else {
		return (vm_offset_t)addr + buf_kernel_addrperm;
	}
}

  
                                                   
   
int
count_lock_queue(void)
{
	buf_t   bp;
	int     n = 0;

	lck_mtx_lock_spin(&buf_mtx);

	for (bp = bufqueues[BQ_LOCKED].tqh_first; bp;
	    bp = bp->b_freelist.tqe_next) {
		n++;
	}
	lck_mtx_unlock(&buf_mtx);

	return n;
}

  
                                                                  
                                                                         
   
uint32_t
count_busy_buffers(void)
{
	return buf_busycount + bufstats.bufs_iobufinuse;
}

#if DIAGNOSTIC
  
                                                                     
                                                                     
                                  
   
void
vfs_bufstats()
{
	int i, j, count;
	struct buf *bp;
	struct bqueues *dp;
	int counts[MAXBSIZE / CLBYTES + 1];
	static char *bname[BQUEUES] =
	{ "LOCKED", "LRU", "AGE", "EMPTY", "META", "LAUNDRY" };

	for (dp = bufqueues, i = 0; dp < &bufqueues[BQUEUES]; dp++, i++) {
		count = 0;
		for (j = 0; j <= MAXBSIZE / CLBYTES; j++) {
			counts[j] = 0;
		}

		lck_mtx_lock(&buf_mtx);

		for (bp = dp->tqh_first; bp; bp = bp->b_freelist.tqe_next) {
			counts[bp->b_bufsize / CLBYTES]++;
			count++;
		}
		lck_mtx_unlock(&buf_mtx);

		printf("%s: total-%d", bname[i], count);
		for (j = 0; j <= MAXBSIZE / CLBYTES; j++) {
			if (counts[j] != 0) {
				printf(", %d-%d", j * CLBYTES, counts[j]);
			}
		}
		printf("\n");
	}
}
#endif                 

#define NRESERVEDIOBUFS 128

#define MNT_VIRTUALDEV_MAX_IOBUFS 16
#define VIRTUALDEV_MAX_IOBUFS ((40*niobuf_headers)/100)

buf_t
alloc_io_buf(vnode_t vp, int priv)
{
	buf_t   bp;
	mount_t mp = NULL;
	int alloc_for_virtualdev = FALSE;

	lck_mtx_lock_spin(&iobuffer_mtxp);

	  
                                                                        
   
                                                       
                                                                       
                                        
                                                               
                                                
    
	if (vp && ((mp = vp->v_mount)) && mp != dead_mountp &&
	    mp->mnt_kern_flag & MNTK_VIRTUALDEV) {
		alloc_for_virtualdev = TRUE;
		while ((!priv && mp->mnt_iobufinuse > MNT_VIRTUALDEV_MAX_IOBUFS) ||
		    bufstats.bufs_iobufinuse_vdev > VIRTUALDEV_MAX_IOBUFS) {
			bufstats.bufs_iobufsleeps++;

			need_iobuffer = 1;
			(void)msleep(&need_iobuffer, &iobuffer_mtxp,
			    PSPIN | (PRIBIO + 1), (const char *)"alloc_io_buf (1)",
			    NULL);
		}
	}

	while ((((uint32_t)(niobuf_headers - NRESERVEDIOBUFS) < bufstats.bufs_iobufinuse) && !priv) ||
	    (bp = iobufqueue.tqh_first) == NULL) {
		bufstats.bufs_iobufsleeps++;

		need_iobuffer = 1;
		(void)msleep(&need_iobuffer, &iobuffer_mtxp, PSPIN | (PRIBIO + 1),
		    (const char *)"alloc_io_buf (2)", NULL);
	}
	TAILQ_REMOVE(&iobufqueue, bp, b_freelist);

	bufstats.bufs_iobufinuse++;
	if (bufstats.bufs_iobufinuse > bufstats.bufs_iobufmax) {
		bufstats.bufs_iobufmax = bufstats.bufs_iobufinuse;
	}

	if (alloc_for_virtualdev) {
		mp->mnt_iobufinuse++;
		bufstats.bufs_iobufinuse_vdev++;
	}

	lck_mtx_unlock(&iobuffer_mtxp);

	  
                             
                                                    
                                                    
                                                         
    
	bp->b_timestamp = 0;
	bp->b_proc = NULL;

	bp->b_datap = 0;
	bp->b_flags = 0;
	bp->b_lflags = BL_BUSY | BL_IOBUF;
	if (alloc_for_virtualdev) {
		bp->b_lflags |= BL_IOBUF_VDEV;
	}
	bp->b_redundancy_flags = 0;
	bp->b_blkno = bp->b_lblkno = 0;
#ifdef JOE_DEBUG
	bp->b_owner = current_thread();
	bp->b_tag   = 6;
#endif
	bp->b_iodone = NULL;
	bp->b_error = 0;
	bp->b_resid = 0;
	bp->b_bcount = 0;
	bp->b_bufsize = 0;
	bp->b_upl = NULL;
	bp->b_fsprivate = (void *)NULL;
	bp->b_vp = vp;
	bzero(&bp->b_attr, sizeof(struct bufattr));

	if (vp && (vp->v_type == VBLK || vp->v_type == VCHR)) {
		bp->b_dev = vp->v_rdev;
	} else {
		bp->b_dev = NODEV;
	}

	return bp;
}


void
free_io_buf(buf_t bp)
{
	int need_wakeup = 0;
	int free_for_virtualdev = FALSE;
	mount_t mp = NULL;

	                                      
	if (bp->b_lflags & BL_IOBUF_VDEV) {
		free_for_virtualdev = TRUE;
		if (bp->b_vp) {
			mp = bp->b_vp->v_mount;
		}
	}

	  
                                                 
    
	bp->b_vp = NULL;
	bp->b_flags = B_INVAL;

	                                                                        
	bzero(&bp->b_attr, sizeof(struct bufattr));

	lck_mtx_lock_spin(&iobuffer_mtxp);

	binsheadfree(bp, &iobufqueue, -1);

	if (need_iobuffer) {
		  
                                                                 
    
                                                                    
                                                                   
                                                                     
                                                                          
                                          
     
		need_iobuffer = 0;
		need_wakeup = 1;
	}
	if (bufstats.bufs_iobufinuse <= 0) {
		panic("free_io_buf: bp(%p) - bufstats.bufs_iobufinuse < 0", bp);
	}

	bufstats.bufs_iobufinuse--;

	if (free_for_virtualdev) {
		bufstats.bufs_iobufinuse_vdev--;
		if (mp && mp != dead_mountp) {
			mp->mnt_iobufinuse--;
		}
	}

	lck_mtx_unlock(&iobuffer_mtxp);

	if (need_wakeup) {
		wakeup(&need_iobuffer);
	}
}


void
buf_list_lock(void)
{
	lck_mtx_lock_spin(&buf_mtx);
}

void
buf_list_unlock(void)
{
	lck_mtx_unlock(&buf_mtx);
}

  
                                                      
                                                        
                                                                  
   


static void
bcleanbuf_thread_init(void)
{
	thread_t        thread = THREAD_NULL;

	                          
	kernel_thread_start((thread_continue_t)bcleanbuf_thread, NULL, &thread);
	thread_deallocate(thread);
}

typedef int (*bcleanbufcontinuation)(int);

__attribute__((noreturn))
static void
bcleanbuf_thread(void)
{
	struct buf *bp;
	int error = 0;
	int loopcnt = 0;

	for (;;) {
		lck_mtx_lock_spin(&buf_mtx);

		while ((bp = TAILQ_FIRST(&bufqueues[BQ_LAUNDRY])) == NULL) {
			(void)msleep0(&bufqueues[BQ_LAUNDRY], &buf_mtx, PRIBIO | PDROP, "blaundry", 0, (bcleanbufcontinuation)bcleanbuf_thread);
		}

		  
                          
     
		bremfree_locked(bp);

		  
                                         
     
		SET(bp->b_lflags, BL_BUSY);
		buf_busycount++;

#ifdef JOE_DEBUG
		bp->b_owner = current_thread();
		bp->b_tag   = 10;
#endif

		lck_mtx_unlock(&buf_mtx);
		  
              
     
		error = bawrite_internal(bp, 0);

		if (error) {
			bp->b_whichq = BQ_LAUNDRY;
			bp->b_timestamp = buf_timestamp();

			lck_mtx_lock_spin(&buf_mtx);

			binstailfree(bp, &bufqueues[BQ_LAUNDRY], BQ_LAUNDRY);
			blaundrycnt++;

			                                                     
			CLR(bp->b_lflags, BL_BUSY);
			buf_busycount--;
#ifdef JOE_DEBUG
			bp->b_owner = current_thread();
			bp->b_tag   = 11;
#endif

			lck_mtx_unlock(&buf_mtx);

			if (loopcnt > MAXLAUNDRY) {
				  
                                                                        
                                                                            
                
       
				(void)tsleep((void *)&bufqueues[BQ_LAUNDRY], PRIBIO, "blaundry", 1);
				loopcnt = 0;
			} else {
				                                        
				(void)thread_block(THREAD_CONTINUE_NULL);
				loopcnt++;
			}
		}
	}
}


static int
brecover_data(buf_t bp)
{
	int     upl_offset;
	upl_t   upl;
	upl_page_info_t *pl;
	kern_return_t kret;
	vnode_t vp = bp->b_vp;
	int upl_flags;


	if (!UBCINFOEXISTS(vp) || bp->b_bufsize == 0) {
		goto dump_buffer;
	}

	upl_flags = UPL_PRECIOUS;
	if (!(buf_flags(bp) & B_READ)) {
		  
                                                   
                                                          
               
     
		upl_flags |= UPL_WILL_MODIFY;
	}

	kret = ubc_create_upl_kernel(vp,
	    ubc_blktooff(vp, bp->b_lblkno),
	    bp->b_bufsize,
	    &upl,
	    &pl,
	    upl_flags,
	    VM_KERN_MEMORY_FILE);
	if (kret != KERN_SUCCESS) {
		panic("Failed to create UPL");
	}

	for (upl_offset = 0; (uint32_t)upl_offset < bp->b_bufsize; upl_offset += PAGE_SIZE) {
		if (!upl_valid_page(pl, upl_offset / PAGE_SIZE) || !upl_dirty_page(pl, upl_offset / PAGE_SIZE)) {
			ubc_upl_abort(upl, 0);
			goto dump_buffer;
		}
	}
	bp->b_upl = upl;

	kret = ubc_upl_map(upl, (vm_offset_t *)&(bp->b_datap));

	if (kret != KERN_SUCCESS) {
		panic("getblk: ubc_upl_map() failed with (%d)", kret);
	}
	return 1;

dump_buffer:
	bp->b_bufsize = 0;
	SET(bp->b_flags, B_INVAL);
	buf_brelse(bp);

	return 0;
}

int
fs_buffer_cache_gc_register(void (* callout)(int, void *), void *context)
{
	lck_mtx_lock(&buf_gc_callout);
	for (int i = 0; i < FS_BUFFER_CACHE_GC_CALLOUTS_MAX_SIZE; i++) {
		if (fs_callouts[i].callout == NULL) {
			fs_callouts[i].callout = callout;
			fs_callouts[i].context = context;
			lck_mtx_unlock(&buf_gc_callout);
			return 0;
		}
	}

	lck_mtx_unlock(&buf_gc_callout);
	return ENOMEM;
}

int
fs_buffer_cache_gc_unregister(void (* callout)(int, void *), void *context)
{
	lck_mtx_lock(&buf_gc_callout);
	for (int i = 0; i < FS_BUFFER_CACHE_GC_CALLOUTS_MAX_SIZE; i++) {
		if (fs_callouts[i].callout == callout &&
		    fs_callouts[i].context == context) {
			fs_callouts[i].callout = NULL;
			fs_callouts[i].context = NULL;
		}
	}
	lck_mtx_unlock(&buf_gc_callout);
	return 0;
}

static void
fs_buffer_cache_gc_dispatch_callouts(int all)
{
	lck_mtx_lock(&buf_gc_callout);
	for (int i = 0; i < FS_BUFFER_CACHE_GC_CALLOUTS_MAX_SIZE; i++) {
		if (fs_callouts[i].callout != NULL) {
			fs_callouts[i].callout(all, fs_callouts[i].context);
		}
	}
	lck_mtx_unlock(&buf_gc_callout);
}

static boolean_t
buffer_cache_gc(int all)
{
	buf_t bp;
	boolean_t did_large_zfree = FALSE;
	boolean_t need_wakeup = FALSE;
	int now = buf_timestamp();
	uint32_t found = 0;
	struct bqueues privq;
	int thresh_hold = BUF_STALE_THRESHHOLD;

	if (all) {
		thresh_hold = 0;
	}
	  
                                                                     
                                                                       
                                                                      
                                                                        
                                                                        
                                                                     
                                    
    
	lck_mtx_lock(&buf_mtx);

	do {
		found = 0;
		TAILQ_INIT(&privq);
		need_wakeup = FALSE;

		while (((bp = TAILQ_FIRST(&bufqueues[BQ_META]))) &&
		    (now > bp->b_timestamp) &&
		    (now - bp->b_timestamp > thresh_hold) &&
		    (found < BUF_MAX_GC_BATCH_SIZE)) {
			                           
			bremfree_locked(bp);
			found++;

#ifdef JOE_DEBUG
			bp->b_owner = current_thread();
			bp->b_tag   = 12;
#endif

			                                                               
			if (ISSET(bp->b_flags, B_DELWRI)) {
				SET(bp->b_lflags, BL_WANTDEALLOC);

				bmovelaundry(bp);
				need_wakeup = TRUE;

				continue;
			}

			  
                                                                  
                                        
      
			SET(bp->b_lflags, BL_BUSY);
			buf_busycount++;

			  
                                              
      
			bremhash(bp);
			if (bp->b_vp) {
				brelvp_locked(bp);
			}

			TAILQ_INSERT_TAIL(&privq, bp, b_freelist);
		}

		if (found == 0) {
			break;
		}

		                                    
		lck_mtx_unlock(&buf_mtx);

		                                             
		if (need_wakeup) {
			wakeup(&bufqueues[BQ_LAUNDRY]);
			(void)thread_block(THREAD_CONTINUE_NULL);
		}

		                                           
		TAILQ_FOREACH(bp, &privq, b_freelist) {
			                                                                   
			if ((ISSET(bp->b_flags, B_ZALLOC)) && (buf_size(bp) >= PAGE_SIZE)) {
				did_large_zfree = TRUE;
			}

			trace(TR_BRELSE, pack(bp->b_vp, bp->b_bufsize), bp->b_lblkno);

			                  
			buf_free_meta_store(bp);

			                         
			buf_release_credentials(bp);

			                                       
			CLR(bp->b_flags, (B_META | B_ZALLOC | B_DELWRI | B_LOCKED
			    | B_AGE | B_ASYNC | B_NOCACHE | B_FUA));
			bp->b_whichq = BQ_EMPTY;
			BLISTNONE(bp);
		}
		lck_mtx_lock(&buf_mtx);

		                                                                   
		TAILQ_FOREACH(bp, &privq, b_freelist) {
			binshash(bp, &invalhash);
			CLR(bp->b_lflags, BL_BUSY);
			buf_busycount--;

#ifdef JOE_DEBUG
			if (bp->b_owner != current_thread()) {
				panic("Buffer stolen from buffer_cache_gc()");
			}
			bp->b_owner = current_thread();
			bp->b_tag   = 13;
#endif
		}

		                                               
		TAILQ_CONCAT(&bufqueues[BQ_EMPTY], &privq, b_freelist);
	} while (all && (found == BUF_MAX_GC_BATCH_SIZE));

	lck_mtx_unlock(&buf_mtx);

	fs_buffer_cache_gc_dispatch_callouts(all);

	return did_large_zfree;
}


  
                   
   

#if FLUSH_QUEUES

#define NFLUSH 32

static int
bp_cmp(void *a, void *b)
{
	buf_t *bp_a = *(buf_t **)a,
	    *bp_b = *(buf_t **)b;
	daddr64_t res;

	                                           
	                               
	  
	res = (bp_a->b_blkno - bp_b->b_blkno);

	return (int)res;
}


int
bflushq(int whichq, mount_t mp)
{
	buf_t   bp, next;
	int     i, buf_count;
	int     total_writes = 0;
	static buf_t flush_table[NFLUSH];

	if (whichq < 0 || whichq >= BQUEUES) {
		return 0;
	}

restart:
	lck_mtx_lock(&buf_mtx);

	bp = TAILQ_FIRST(&bufqueues[whichq]);

	for (buf_count = 0; bp; bp = next) {
		next = bp->b_freelist.tqe_next;

		if (bp->b_vp == NULL || bp->b_vp->v_mount != mp) {
			continue;
		}

		if (ISSET(bp->b_flags, B_DELWRI) && !ISSET(bp->b_lflags, BL_BUSY)) {
			bremfree_locked(bp);
#ifdef JOE_DEBUG
			bp->b_owner = current_thread();
			bp->b_tag   = 7;
#endif
			SET(bp->b_lflags, BL_BUSY);
			buf_busycount++;

			flush_table[buf_count] = bp;
			buf_count++;
			total_writes++;

			if (buf_count >= NFLUSH) {
				lck_mtx_unlock(&buf_mtx);

				qsort(flush_table, buf_count, sizeof(struct buf *), bp_cmp);

				for (i = 0; i < buf_count; i++) {
					buf_bawrite(flush_table[i]);
				}
				goto restart;
			}
		}
	}
	lck_mtx_unlock(&buf_mtx);

	if (buf_count > 0) {
		qsort(flush_table, buf_count, sizeof(struct buf *), bp_cmp);

		for (i = 0; i < buf_count; i++) {
			buf_bawrite(flush_table[i]);
		}
	}

	return total_writes;
}
#endif
