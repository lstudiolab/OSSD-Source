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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1993
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
 *	@(#)vfs_cluster.c	8.10 (Berkeley) 3/28/95
 */

#include <sys/param.h>
#include <sys/proc_internal.h>
#include <sys/buf_internal.h>
#include <sys/mount_internal.h>
#include <sys/vnode_internal.h>
#include <sys/trace.h>
#include <kern/kalloc.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/resourcevar.h>
#include <miscfs/specfs/specdev.h>
#include <sys/uio_internal.h>
#include <libkern/libkern.h>
#include <machine/machine_routines.h>

#include <sys/ubc_internal.h>
#include <vm/vnode_pager.h>

#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <mach/vm_map.h>
#include <mach/upl.h>
#include <kern/task.h>
#include <kern/policy_internal.h>

#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_pageout.h>
#include <vm/vm_fault.h>

#include <sys/kdebug.h>
#include <libkern/OSAtomic.h>

#include <sys/sdt.h>

#include <stdbool.h>

#include <vfs/vfs_disk_conditioner.h>

#if 0
#undef KERNEL_DEBUG
#define KERNEL_DEBUG KERNEL_DEBUG_CONSTANT
#endif


#define CL_READ         0x01
#define CL_WRITE        0x02
#define CL_ASYNC        0x04
#define CL_COMMIT       0x08
#define CL_PAGEOUT      0x10
#define CL_AGE          0x20
#define CL_NOZERO       0x40
#define CL_PAGEIN       0x80
#define CL_DEV_MEMORY   0x100
#define CL_PRESERVE     0x200
#define CL_THROTTLE     0x400
#define CL_KEEPCACHED   0x800
#define CL_DIRECT_IO    0x1000
#define CL_PASSIVE      0x2000
#define CL_IOSTREAMING  0x4000
#define CL_CLOSE        0x8000
#define CL_ENCRYPTED    0x10000
#define CL_RAW_ENCRYPTED        0x20000
#define CL_NOCACHE      0x40000

#define MAX_VECTOR_UPL_ELEMENTS 8
#define MAX_VECTOR_UPL_SIZE     (2 * MAX_UPL_SIZE_BYTES)

#define CLUSTER_IO_WAITING              ((buf_t)1)

extern upl_t vector_upl_create(vm_offset_t);
extern boolean_t vector_upl_is_valid(upl_t);
extern boolean_t vector_upl_set_subupl(upl_t, upl_t, u_int32_t);
extern void vector_upl_set_pagelist(upl_t);
extern void vector_upl_set_iostate(upl_t, upl_t, vm_offset_t, u_int32_t);

struct clios {
	lck_mtx_t io_mtxp;
	u_int  io_completed;                                                      
	u_int  io_issued;                                                         
	int    io_error;                                                      
	int    io_wanted;                                                                 
};

struct cl_direct_read_lock {
	LIST_ENTRY(cl_direct_read_lock)         chain;
	int32_t                                                         ref_count;
	vnode_t                                                         vp;
	lck_rw_t                                                        rw_lock;
};

#define CL_DIRECT_READ_LOCK_BUCKETS 61

static LIST_HEAD(cl_direct_read_locks, cl_direct_read_lock)
cl_direct_read_locks[CL_DIRECT_READ_LOCK_BUCKETS];

static LCK_GRP_DECLARE(cl_mtx_grp, "cluster I/O");
static LCK_MTX_DECLARE(cl_transaction_mtxp, &cl_mtx_grp);
static LCK_SPIN_DECLARE(cl_direct_read_spin_lock, &cl_mtx_grp);

static ZONE_DECLARE(cl_rd_zone, "cluster_read",
    sizeof(struct cl_readahead), ZC_ZFREE_CLEARMEM | ZC_NOENCRYPT);

static ZONE_DECLARE(cl_wr_zone, "cluster_write",
    sizeof(struct cl_writebehind), ZC_ZFREE_CLEARMEM | ZC_NOENCRYPT);

#define IO_UNKNOWN      0
#define IO_DIRECT       1
#define IO_CONTIG       2
#define IO_COPY         3

#define PUSH_DELAY      0x01
#define PUSH_ALL        0x02
#define PUSH_SYNC       0x04


static void cluster_EOT(buf_t cbp_head, buf_t cbp_tail, int zero_offset);
static void cluster_wait_IO(buf_t cbp_head, int async);
static void cluster_complete_transaction(buf_t *cbp_head, void *callback_arg, int *retval, int flags, int needwait);

static int cluster_io_type(struct uio *uio, int *io_type, u_int32_t *io_length, u_int32_t min_length);

static int cluster_io(vnode_t vp, upl_t upl, vm_offset_t upl_offset, off_t f_offset, int non_rounded_size,
    int flags, buf_t real_bp, struct clios *iostate, int (*)(buf_t, void *), void *callback_arg);
static int cluster_iodone(buf_t bp, void *callback_arg);
static int cluster_ioerror(upl_t upl, int upl_offset, int abort_size, int error, int io_flags, vnode_t vp);
static int cluster_is_throttled(vnode_t vp);

static void cluster_iostate_wait(struct clios *iostate, u_int target, const char *wait_name);

static void cluster_syncup(vnode_t vp, off_t newEOF, int (*)(buf_t, void *), void *callback_arg, int flags);

static void cluster_read_upl_release(upl_t upl, int start_pg, int last_pg, int take_reference);
static int cluster_copy_ubc_data_internal(vnode_t vp, struct uio *uio, int *io_resid, int mark_dirty, int take_reference);

static int cluster_read_copy(vnode_t vp, struct uio *uio, u_int32_t io_req_size, off_t filesize, int flags,
    int (*)(buf_t, void *), void *callback_arg) __attribute__((noinline));
static int cluster_read_direct(vnode_t vp, struct uio *uio, off_t filesize, int *read_type, u_int32_t *read_length,
    int flags, int (*)(buf_t, void *), void *callback_arg) __attribute__((noinline));
static int cluster_read_contig(vnode_t vp, struct uio *uio, off_t filesize, int *read_type, u_int32_t *read_length,
    int (*)(buf_t, void *), void *callback_arg, int flags) __attribute__((noinline));

static int cluster_write_copy(vnode_t vp, struct uio *uio, u_int32_t io_req_size, off_t oldEOF, off_t newEOF,
    off_t headOff, off_t tailOff, int flags, int (*)(buf_t, void *), void *callback_arg) __attribute__((noinline));
static int cluster_write_direct(vnode_t vp, struct uio *uio, off_t oldEOF, off_t newEOF,
    int *write_type, u_int32_t *write_length, int flags, int (*)(buf_t, void *), void *callback_arg) __attribute__((noinline));
static int cluster_write_contig(vnode_t vp, struct uio *uio, off_t newEOF,
    int *write_type, u_int32_t *write_length, int (*)(buf_t, void *), void *callback_arg, int bflag) __attribute__((noinline));

static void cluster_update_state_internal(vnode_t vp, struct cl_extent *cl, int flags, boolean_t defer_writes, boolean_t *first_pass,
    off_t write_off, int write_cnt, off_t newEOF, int (*callback)(buf_t, void *), void *callback_arg, boolean_t vm_initiated);

static int cluster_align_phys_io(vnode_t vp, struct uio *uio, addr64_t usr_paddr, u_int32_t xsize, int flags, int (*)(buf_t, void *), void *callback_arg);

static int      cluster_read_prefetch(vnode_t vp, off_t f_offset, u_int size, off_t filesize, int (*callback)(buf_t, void *), void *callback_arg, int bflag);
static void     cluster_read_ahead(vnode_t vp, struct cl_extent *extent, off_t filesize, struct cl_readahead *ra,
    int (*callback)(buf_t, void *), void *callback_arg, int bflag);

static int      cluster_push_now(vnode_t vp, struct cl_extent *, off_t EOF, int flags, int (*)(buf_t, void *), void *callback_arg, boolean_t vm_ioitiated);

static int      cluster_try_push(struct cl_writebehind *, vnode_t vp, off_t EOF, int push_flag, int flags, int (*)(buf_t, void *),
    void *callback_arg, int *err, boolean_t vm_initiated);

static int      sparse_cluster_switch(struct cl_writebehind *, vnode_t vp, off_t EOF, int (*)(buf_t, void *), void *callback_arg, boolean_t vm_initiated);
static int      sparse_cluster_push(struct cl_writebehind *, void **cmapp, vnode_t vp, off_t EOF, int push_flag,
    int io_flags, int (*)(buf_t, void *), void *callback_arg, boolean_t vm_initiated);
static int      sparse_cluster_add(struct cl_writebehind *, void **cmapp, vnode_t vp, struct cl_extent *, off_t EOF,
    int (*)(buf_t, void *), void *callback_arg, boolean_t vm_initiated);

static kern_return_t vfs_drt_mark_pages(void **cmapp, off_t offset, u_int length, u_int *setcountp);
static kern_return_t vfs_drt_get_cluster(void **cmapp, off_t *offsetp, u_int *lengthp);
static kern_return_t vfs_drt_control(void **cmapp, int op_type);
static kern_return_t vfs_get_scmap_push_behavior_internal(void **cmapp, int *push_flag);


  
                                    
                                      
                                         
  
                                        
                                        
                                        
                           
  
                                
                                     
             
  
                                     
                                        
                                       
           
  
                                                                                          
   
int (*bootcache_contains_block)(dev_t device, u_int64_t blkno) = NULL;


  
                                         
                                   
   
#define MAX_IO_REQUEST_SIZE     (1024 * 1024 * 512)
#define MAX_IO_CONTIG_SIZE      MAX_UPL_SIZE_BYTES
#define MAX_VECTS               16
  
                                                                                     
                                                                                   
                                                                
   
#define MIN_DIRECT_WRITE_SIZE   (16384)

#define WRITE_THROTTLE          6
#define WRITE_THROTTLE_SSD      2
#define WRITE_BEHIND            1
#define WRITE_BEHIND_SSD        1

#if !defined(XNU_TARGET_OS_OSX)
#define PREFETCH                1
#define PREFETCH_SSD            1
uint32_t speculative_prefetch_max = (2048 * 1024);                                                             
uint32_t speculative_prefetch_max_iosize = (512 * 1024);                                                                 
#else                        
#define PREFETCH                3
#define PREFETCH_SSD            2
uint32_t speculative_prefetch_max = (MAX_UPL_SIZE_BYTES * 3);                                                  
uint32_t speculative_prefetch_max_iosize = (512 * 1024);                                                                        
#endif                          


#define IO_SCALE(vp, base)              (vp->v_mount->mnt_ioscale * (base))
#define MAX_CLUSTER_SIZE(vp)            (cluster_max_io_size(vp->v_mount, CL_WRITE))
#define MAX_PREFETCH(vp, size, is_ssd)  (size * IO_SCALE(vp, ((is_ssd) ? PREFETCH_SSD : PREFETCH)))

int     speculative_reads_disabled = 0;

  
                                           
                                       
                                      
   
#define THROTTLE_MAXCNT 0

uint32_t throttle_max_iosize = (128 * 1024);

#define THROTTLE_MAX_IOSIZE (throttle_max_iosize)

SYSCTL_INT(_debug, OID_AUTO, lowpri_throttle_max_iosize, CTLFLAG_RW | CTLFLAG_LOCKED, &throttle_max_iosize, 0, "");


void
cluster_init(void)
{
	for (int i = 0; i < CL_DIRECT_READ_LOCK_BUCKETS; ++i) {
		LIST_INIT(&cl_direct_read_locks[i]);
	}
}


uint32_t
cluster_max_io_size(mount_t mp, int type)
{
	uint32_t        max_io_size;
	uint32_t        segcnt;
	uint32_t        maxcnt;

	switch (type) {
	case CL_READ:
		segcnt = mp->mnt_segreadcnt;
		maxcnt = mp->mnt_maxreadcnt;
		break;
	case CL_WRITE:
		segcnt = mp->mnt_segwritecnt;
		maxcnt = mp->mnt_maxwritecnt;
		break;
	default:
		segcnt = min(mp->mnt_segreadcnt, mp->mnt_segwritecnt);
		maxcnt = min(mp->mnt_maxreadcnt, mp->mnt_maxwritecnt);
		break;
	}
	if (segcnt > (MAX_UPL_SIZE_BYTES >> PAGE_SHIFT)) {
		  
                                                             
     
		segcnt = MAX_UPL_SIZE_BYTES >> PAGE_SHIFT;
	}
	max_io_size = min((segcnt * PAGE_SIZE), maxcnt);

	if (max_io_size < MAX_UPL_TRANSFER_BYTES) {
		  
                                                        
     
		max_io_size = MAX_UPL_TRANSFER_BYTES;
	} else {
		  
                                                            
     
		max_io_size &= ~PAGE_MASK;
	}
	return max_io_size;
}




#define CLW_ALLOCATE            0x01
#define CLW_RETURNLOCKED        0x02
#define CLW_IONOCACHE           0x04
#define CLW_IOPASSIVE   0x08

  
                                               
                                
                                             
                                            
                                             
                                           
  
                                                                
                                            
                                        
                                            
                                          
                                           
                                          
                                     
   
static struct cl_readahead *
cluster_get_rap(vnode_t vp)
{
	struct ubc_info         *ubc;
	struct cl_readahead     *rap;

	ubc = vp->v_ubcinfo;

	if ((rap = ubc->cl_rahead) == NULL) {
		rap = zalloc_flags(cl_rd_zone, Z_WAITOK | Z_ZERO);
		rap->cl_lastr = -1;
		lck_mtx_init(&rap->cl_lockr, &cl_mtx_grp, LCK_ATTR_NULL);

		vnode_lock(vp);

		if (ubc->cl_rahead == NULL) {
			ubc->cl_rahead = rap;
		} else {
			lck_mtx_destroy(&rap->cl_lockr, &cl_mtx_grp);
			zfree(cl_rd_zone, rap);
			rap = ubc->cl_rahead;
		}
		vnode_unlock(vp);
	}
	if (lck_mtx_try_lock(&rap->cl_lockr) == TRUE) {
		return rap;
	}

	return (struct cl_readahead *)NULL;
}


  
                                                 
                                                               
                                             
                                            
                                             
                                           
  
                                                           
                                                           
            
   

static struct cl_writebehind *
cluster_get_wbp(vnode_t vp, int flags)
{
	struct ubc_info *ubc;
	struct cl_writebehind *wbp;

	ubc = vp->v_ubcinfo;

	if ((wbp = ubc->cl_wbehind) == NULL) {
		if (!(flags & CLW_ALLOCATE)) {
			return (struct cl_writebehind *)NULL;
		}

		wbp = zalloc_flags(cl_wr_zone, Z_WAITOK | Z_ZERO);

		lck_mtx_init(&wbp->cl_lockw, &cl_mtx_grp, LCK_ATTR_NULL);

		vnode_lock(vp);

		if (ubc->cl_wbehind == NULL) {
			ubc->cl_wbehind = wbp;
		} else {
			lck_mtx_destroy(&wbp->cl_lockw, &cl_mtx_grp);
			zfree(cl_wr_zone, wbp);
			wbp = ubc->cl_wbehind;
		}
		vnode_unlock(vp);
	}
	if (flags & CLW_RETURNLOCKED) {
		lck_mtx_lock(&wbp->cl_lockw);
	}

	return wbp;
}


static void
cluster_syncup(vnode_t vp, off_t newEOF, int (*callback)(buf_t, void *), void *callback_arg, int flags)
{
	struct cl_writebehind *wbp;

	if ((wbp = cluster_get_wbp(vp, 0)) != NULL) {
		if (wbp->cl_number) {
			lck_mtx_lock(&wbp->cl_lockw);

			cluster_try_push(wbp, vp, newEOF, PUSH_ALL | flags, 0, callback, callback_arg, NULL, FALSE);

			lck_mtx_unlock(&wbp->cl_lockw);
		}
	}
}


static int
cluster_io_present_in_BC(vnode_t vp, off_t f_offset)
{
	daddr64_t blkno;
	size_t    io_size;
	int (*bootcache_check_fn)(dev_t device, u_int64_t blkno) = bootcache_contains_block;

	if (bootcache_check_fn && vp->v_mount && vp->v_mount->mnt_devvp) {
		if (VNOP_BLOCKMAP(vp, f_offset, PAGE_SIZE, &blkno, &io_size, NULL, VNODE_READ | VNODE_BLOCKMAP_NO_TRACK, NULL)) {
			return 0;
		}

		if (io_size == 0) {
			return 0;
		}

		if (bootcache_check_fn(vp->v_mount->mnt_devvp->v_rdev, blkno)) {
			return 1;
		}
	}
	return 0;
}


static int
cluster_is_throttled(vnode_t vp)
{
	return throttle_io_will_be_throttled(-1, vp->v_mount);
}


static void
cluster_iostate_wait(struct clios *iostate, u_int target, const char *wait_name)
{
	lck_mtx_lock(&iostate->io_mtxp);

	while ((iostate->io_issued - iostate->io_completed) > target) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 95)) | DBG_FUNC_START,
		    iostate->io_issued, iostate->io_completed, target, 0, 0);

		iostate->io_wanted = 1;
		msleep((caddr_t)&iostate->io_wanted, &iostate->io_mtxp, PRIBIO + 1, wait_name, NULL);

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 95)) | DBG_FUNC_END,
		    iostate->io_issued, iostate->io_completed, target, 0, 0);
	}
	lck_mtx_unlock(&iostate->io_mtxp);
}

static void
cluster_handle_associated_upl(struct clios *iostate, upl_t upl,
    upl_offset_t upl_offset, upl_size_t size)
{
	if (!size) {
		return;
	}

	upl_t associated_upl = upl_associated_upl(upl);

	if (!associated_upl) {
		return;
	}

#if 0
	printf("1: %d %d\n", upl_offset, upl_offset + size);
#endif

	  
                                                                  
                                                                   
                                                                  
                                                                 
                                                              
                                                                  
                                                                   
                                             
   
                                                        
   
                                                 
                                                                  
                                                                  
                                                                  
                                                             
   
                                                                 
                                                                  
                                                                   
                                                                  
                                                                  
                                                                 
                                                              
                                                                
                                
    

	  
                                                                 
                                  
    
	upl_page_info_t *pl = UPL_GET_INTERNAL_PAGE_LIST(upl);
	bool is_unaligned = upl_page_get_mark(pl, 0);

	if (is_unaligned) {
		upl_page_info_t *assoc_pl = UPL_GET_INTERNAL_PAGE_LIST(associated_upl);

		upl_offset_t upl_end = upl_offset + size;
		assert(upl_end >= PAGE_SIZE);

		upl_size_t assoc_upl_size = upl_get_size(associated_upl);

		  
                                                                
                                                               
                                                                
                   
     
		assert(upl_offset);
		if (upl_offset) {
			upl_offset = trunc_page_32(upl_offset - 1);
		}

		lck_mtx_lock_spin(&iostate->io_mtxp);

		                            
		if (upl_offset
		    && !upl_page_get_mark(assoc_pl, upl_offset >> PAGE_SHIFT)) {
			  
                                                            
                           
      
			upl_page_set_mark(assoc_pl, upl_offset >> PAGE_SHIFT, true);
			upl_offset += PAGE_SIZE;
		}

		                           

		  
                                                                
                                                               
          
     
		if (upl_end > assoc_upl_size) {
			upl_end = assoc_upl_size;
		} else {
			upl_end = trunc_page_32(upl_end);
			const int last_pg = (upl_end >> PAGE_SHIFT) - 1;

			if (!upl_page_get_mark(assoc_pl, last_pg)) {
				  
                                                                  
                                        
       
				upl_page_set_mark(assoc_pl, last_pg, true);
				upl_end -= PAGE_SIZE;
			}
		}

		lck_mtx_unlock(&iostate->io_mtxp);

#if 0
		printf("2: %d %d\n", upl_offset, upl_end);
#endif

		if (upl_end <= upl_offset) {
			return;
		}

		size = upl_end - upl_offset;
	} else {
		assert(!(upl_offset & PAGE_MASK));
		assert(!(size & PAGE_MASK));
	}

	boolean_t empty;

	  
                                                      
                                                         
    
	kern_return_t kr = upl_abort_range(associated_upl, upl_offset, size,
	    UPL_ABORT_DUMP_PAGES, &empty);

	assert(!kr);

	if (!kr && empty) {
		upl_set_associated_upl(upl, NULL);
		upl_deallocate(associated_upl);
	}
}

static int
cluster_ioerror(upl_t upl, int upl_offset, int abort_size, int error, int io_flags, vnode_t vp)
{
	int upl_abort_code = 0;
	int page_in  = 0;
	int page_out = 0;

	if ((io_flags & (B_PHYS | B_CACHE)) == (B_PHYS | B_CACHE)) {
		  
                                                                     
     
		ubc_upl_commit_range(upl, upl_offset, abort_size, UPL_COMMIT_FREE_ON_EMPTY);
	} else {
		if (io_flags & B_PAGEIO) {
			if (io_flags & B_READ) {
				page_in  = 1;
			} else {
				page_out = 1;
			}
		}
		if (io_flags & B_CACHE) {
			  
                                                 
      
			upl_abort_code = UPL_ABORT_FREE_ON_EMPTY;
		} else if (((io_flags & B_READ) == 0) && ((error != ENXIO) || vnode_isswap(vp))) {
			  
                                                                    
      
			upl_abort_code = UPL_ABORT_FREE_ON_EMPTY;
		} else if (page_in) {
			upl_abort_code = UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_ERROR;
		} else {
			upl_abort_code = UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_DUMP_PAGES;
		}

		ubc_upl_abort_range(upl, upl_offset, abort_size, upl_abort_code);
	}
	return upl_abort_code;
}


static int
cluster_iodone(buf_t bp, void *callback_arg)
{
	int     b_flags;
	int     error;
	int     total_size;
	int     total_resid;
	int     upl_offset;
	int     zero_offset;
	int     pg_offset = 0;
	int     commit_size = 0;
	int     upl_flags = 0;
	int     transaction_size = 0;
	upl_t   upl;
	buf_t   cbp;
	buf_t   cbp_head;
	buf_t   cbp_next;
	buf_t   real_bp;
	vnode_t vp;
	struct  clios *iostate;
	boolean_t       transaction_complete = FALSE;

	__IGNORE_WCASTALIGN(cbp_head = (buf_t)(bp->b_trans_head));

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 20)) | DBG_FUNC_START,
	    cbp_head, bp->b_lblkno, bp->b_bcount, bp->b_flags, 0);

	if (cbp_head->b_trans_next || !(cbp_head->b_flags & B_EOT)) {
		lck_mtx_lock_spin(&cl_transaction_mtxp);

		bp->b_flags |= B_TDONE;

		for (cbp = cbp_head; cbp; cbp = cbp->b_trans_next) {
			  
                                                        
                                               
      
			if (!(cbp->b_flags & B_TDONE)) {
				KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 20)) | DBG_FUNC_END,
				    cbp_head, cbp, cbp->b_bcount, cbp->b_flags, 0);

				lck_mtx_unlock(&cl_transaction_mtxp);

				return 0;
			}

			if (cbp->b_trans_next == CLUSTER_IO_WAITING) {
				KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 20)) | DBG_FUNC_END,
				    cbp_head, cbp, cbp->b_bcount, cbp->b_flags, 0);

				lck_mtx_unlock(&cl_transaction_mtxp);
				wakeup(cbp);

				return 0;
			}

			if (cbp->b_flags & B_EOT) {
				transaction_complete = TRUE;
			}
		}
		lck_mtx_unlock(&cl_transaction_mtxp);

		if (transaction_complete == FALSE) {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 20)) | DBG_FUNC_END,
			    cbp_head, 0, 0, 0, 0);
			return 0;
		}
	}
	error       = 0;
	total_size  = 0;
	total_resid = 0;

	cbp        = cbp_head;
	vp         = cbp->b_vp;
	upl_offset = cbp->b_uploffset;
	upl        = cbp->b_upl;
	b_flags    = cbp->b_flags;
	real_bp    = cbp->b_real_bp;
	zero_offset = cbp->b_validend;
	iostate    = (struct clios *)cbp->b_iostate;

	if (real_bp) {
		real_bp->b_dev = cbp->b_dev;
	}

	while (cbp) {
		if ((cbp->b_flags & B_ERROR) && error == 0) {
			error = cbp->b_error;
		}

		total_resid += cbp->b_resid;
		total_size  += cbp->b_bcount;

		cbp_next = cbp->b_trans_next;

		if (cbp_next == NULL) {
			  
                                                 
                                                   
                                                  
                                                         
      
			transaction_size = cbp->b_uploffset + cbp->b_bcount - upl_offset;
		}

		if (cbp != cbp_head) {
			free_io_buf(cbp);
		}

		cbp = cbp_next;
	}

	if (ISSET(b_flags, B_COMMIT_UPL)) {
		cluster_handle_associated_upl(iostate,
		    cbp_head->b_upl,
		    upl_offset,
		    transaction_size);
	}

	if (error == 0 && total_resid) {
		error = EIO;
	}

	if (error == 0) {
		int     (*cliodone_func)(buf_t, void *) = (int (*)(buf_t, void *))(cbp_head->b_cliodone);

		if (cliodone_func != NULL) {
			cbp_head->b_bcount = transaction_size;

			error = (*cliodone_func)(cbp_head, callback_arg);
		}
	}
	if (zero_offset) {
		cluster_zero(upl, zero_offset, PAGE_SIZE - (zero_offset & PAGE_MASK), real_bp);
	}

	free_io_buf(cbp_head);

	if (iostate) {
		int need_wakeup = 0;

		  
                                                   
                                                    
     
		lck_mtx_lock_spin(&iostate->io_mtxp);

		if (error && iostate->io_error == 0) {
			iostate->io_error = error;
		}

		iostate->io_completed += total_size;

		if (iostate->io_wanted) {
			  
                                         
                              
      
			iostate->io_wanted = 0;
			need_wakeup = 1;
		}
		lck_mtx_unlock(&iostate->io_mtxp);

		if (need_wakeup) {
			wakeup((caddr_t)&iostate->io_wanted);
		}
	}

	if (b_flags & B_COMMIT_UPL) {
		pg_offset   = upl_offset & PAGE_MASK;
		commit_size = (pg_offset + transaction_size + (PAGE_SIZE - 1)) & ~PAGE_MASK;

		if (error) {
			upl_set_iodone_error(upl, error);

			upl_flags = cluster_ioerror(upl, upl_offset - pg_offset, commit_size, error, b_flags, vp);
		} else {
			upl_flags = UPL_COMMIT_FREE_ON_EMPTY;

			if ((b_flags & B_PHYS) && (b_flags & B_READ)) {
				upl_flags |= UPL_COMMIT_SET_DIRTY;
			}

			if (b_flags & B_AGE) {
				upl_flags |= UPL_COMMIT_INACTIVATE;
			}

			ubc_upl_commit_range(upl, upl_offset - pg_offset, commit_size, upl_flags);
		}
	}
	if (real_bp) {
		if (error) {
			real_bp->b_flags |= B_ERROR;
			real_bp->b_error = error;
		}
		real_bp->b_resid = total_resid;

		buf_biodone(real_bp);
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 20)) | DBG_FUNC_END,
	    upl, upl_offset - pg_offset, commit_size, (error << 24) | upl_flags, 0);

	return error;
}


uint32_t
cluster_throttle_io_limit(vnode_t vp, uint32_t *limit)
{
	if (cluster_is_throttled(vp)) {
		*limit = THROTTLE_MAX_IOSIZE;
		return 1;
	}
	return 0;
}


void
cluster_zero(upl_t upl, upl_offset_t upl_offset, int size, buf_t bp)
{
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 23)) | DBG_FUNC_START,
	    upl_offset, size, bp, 0, 0);

	if (bp == NULL || bp->b_datap == 0) {
		upl_page_info_t *pl;
		addr64_t        zero_addr;

		pl = ubc_upl_pageinfo(upl);

		if (upl_device_page(pl) == TRUE) {
			zero_addr = ((addr64_t)upl_phys_page(pl, 0) << PAGE_SHIFT) + upl_offset;

			bzero_phys_nc(zero_addr, size);
		} else {
			while (size) {
				int     page_offset;
				int     page_index;
				int     zero_cnt;

				page_index  = upl_offset / PAGE_SIZE;
				page_offset = upl_offset & PAGE_MASK;

				zero_addr = ((addr64_t)upl_phys_page(pl, page_index) << PAGE_SHIFT) + page_offset;
				zero_cnt  = min(PAGE_SIZE - page_offset, size);

				bzero_phys(zero_addr, zero_cnt);

				size       -= zero_cnt;
				upl_offset += zero_cnt;
			}
		}
	} else {
		bzero((caddr_t)((vm_offset_t)bp->b_datap + upl_offset), size);
	}

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 23)) | DBG_FUNC_END,
	    upl_offset, size, 0, 0, 0);
}


static void
cluster_EOT(buf_t cbp_head, buf_t cbp_tail, int zero_offset)
{
	cbp_head->b_validend = zero_offset;
	cbp_tail->b_flags |= B_EOT;
}

static void
cluster_wait_IO(buf_t cbp_head, int async)
{
	buf_t   cbp;

	if (async) {
		  
                                                           
                                                         
                                                              
                                                              
                                                               
                                                       
                                                               
     
		bool done = true;
		buf_t last = NULL;

		lck_mtx_lock_spin(&cl_transaction_mtxp);

		for (cbp = cbp_head; cbp; last = cbp, cbp = cbp->b_trans_next) {
			if (!ISSET(cbp->b_flags, B_TDONE)) {
				done = false;
			}
		}

		if (!done) {
			last->b_trans_next = CLUSTER_IO_WAITING;

			DTRACE_IO1(wait__start, buf_t, last);
			do {
				msleep(last, &cl_transaction_mtxp, PSPIN | (PRIBIO + 1), "cluster_wait_IO", NULL);

				  
                                                   
                                                 
       
				done = true;
				for (cbp = cbp_head; cbp != CLUSTER_IO_WAITING; cbp = cbp->b_trans_next) {
					if (!ISSET(cbp->b_flags, B_TDONE)) {
						done = false;
						break;
					}
				}
			} while (!done);
			DTRACE_IO1(wait__done, buf_t, last);

			last->b_trans_next = NULL;
		}

		lck_mtx_unlock(&cl_transaction_mtxp);
	} else {          
		for (cbp = cbp_head; cbp; cbp = cbp->b_trans_next) {
			buf_biowait(cbp);
		}
	}
}

static void
cluster_complete_transaction(buf_t *cbp_head, void *callback_arg, int *retval, int flags, int needwait)
{
	buf_t   cbp;
	int     error;
	boolean_t isswapout = FALSE;

	  
                                     
                                                                       
                                                                   
    
	if (needwait) {
		for (cbp = *cbp_head; cbp; cbp = cbp->b_trans_next) {
			buf_biowait(cbp);
		}
	}
	  
                                                                
                                                             
                                                            
    
	for (cbp = *cbp_head; cbp; cbp = cbp->b_trans_next) {
		cbp->b_flags |= B_TDONE;
	}
	cbp = *cbp_head;

	if ((flags & (CL_ASYNC | CL_PAGEOUT)) == CL_PAGEOUT && vnode_isswap(cbp->b_vp)) {
		isswapout = TRUE;
	}

	error = cluster_iodone(cbp, callback_arg);

	if (!(flags & CL_ASYNC) && error && *retval == 0) {
		if (((flags & (CL_PAGEOUT | CL_KEEPCACHED)) != CL_PAGEOUT) || (error != ENXIO)) {
			*retval = error;
		} else if (isswapout == TRUE) {
			*retval = error;
		}
	}
	*cbp_head = (buf_t)NULL;
}


static int
cluster_io(vnode_t vp, upl_t upl, vm_offset_t upl_offset, off_t f_offset, int non_rounded_size,
    int flags, buf_t real_bp, struct clios *iostate, int (*callback)(buf_t, void *), void *callback_arg)
{
	buf_t   cbp;
	u_int   size;
	u_int   io_size;
	int     io_flags;
	int     bmap_flags;
	int     error = 0;
	int     retval = 0;
	buf_t   cbp_head = NULL;
	buf_t   cbp_tail = NULL;
	int     trans_count = 0;
	int     max_trans_count;
	u_int   pg_count;
	int     pg_offset;
	u_int   max_iosize;
	u_int   max_vectors;
	int     priv;
	int     zero_offset = 0;
	int     async_throttle = 0;
	mount_t mp;
	vm_offset_t upl_end_offset;
	boolean_t   need_EOT = FALSE;

	  
                                                         
    
	if (real_bp && non_rounded_size > PAGE_SIZE) {
		panic("%s(): Called with real buffer of size %d bytes which "
		    "is greater than the maximum allowed size of "
		    "%d bytes (the system PAGE_SIZE).\n",
		    __FUNCTION__, non_rounded_size, PAGE_SIZE);
	}

	mp = vp->v_mount;

	  
                                                                      
                                                                       
                                                                       
              
    
	if (mp->mnt_devblocksize > 1 && !(flags & (CL_DEV_MEMORY | CL_DIRECT_IO))) {
		  
                                                           
                                                                 
                                                                     
                                                                      
                                                                     
                                                                   
                                                                    
                                                                 
                                                                   
                                                              
     
		pg_offset = upl_offset & PAGE_MASK;

		size = (((non_rounded_size + pg_offset) + (PAGE_SIZE - 1)) & ~PAGE_MASK) - pg_offset;
	} else {
		  
                                                      
                                                    
                                      
     
		size = non_rounded_size;
	}
	upl_end_offset = upl_offset + size;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 22)) | DBG_FUNC_START, (int)f_offset, size, upl_offset, flags, 0);

	  
                                                                     
            
    
	max_trans_count = 8;
	if (flags & CL_DEV_MEMORY) {
		max_trans_count = 16;
	}

	if (flags & CL_READ) {
		io_flags = B_READ;
		bmap_flags = VNODE_READ;

		max_iosize  = mp->mnt_maxreadcnt;
		max_vectors = mp->mnt_segreadcnt;
	} else {
		io_flags = B_WRITE;
		bmap_flags = VNODE_WRITE;

		max_iosize  = mp->mnt_maxwritecnt;
		max_vectors = mp->mnt_segwritecnt;
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 22)) | DBG_FUNC_NONE, max_iosize, max_vectors, mp->mnt_devblocksize, 0, 0);

	  
                                     
                             
    
	max_iosize  &= ~PAGE_MASK;

	  
                                          
    
	if (!max_iosize) {
		max_iosize = PAGE_SIZE;
	}

	if (flags & CL_THROTTLE) {
		if (!(flags & CL_PAGEOUT) && cluster_is_throttled(vp)) {
			if (max_iosize > THROTTLE_MAX_IOSIZE) {
				max_iosize = THROTTLE_MAX_IOSIZE;
			}
			async_throttle = THROTTLE_MAXCNT;
		} else {
			if ((flags & CL_DEV_MEMORY)) {
				async_throttle = IO_SCALE(vp, VNODE_ASYNC_THROTTLE);
			} else {
				u_int max_cluster;
				u_int max_cluster_size;
				u_int scale;

				if (vp->v_mount->mnt_minsaturationbytecount) {
					max_cluster_size = vp->v_mount->mnt_minsaturationbytecount;

					scale = 1;
				} else {
					max_cluster_size = MAX_CLUSTER_SIZE(vp);

					if (disk_conditioner_mount_is_ssd(vp->v_mount)) {
						scale = WRITE_THROTTLE_SSD;
					} else {
						scale = WRITE_THROTTLE;
					}
				}
				if (max_iosize > max_cluster_size) {
					max_cluster = max_cluster_size;
				} else {
					max_cluster = max_iosize;
				}

				if (size < max_cluster) {
					max_cluster = size;
				}

				if (flags & CL_CLOSE) {
					scale += MAX_CLUSTERS;
				}

				async_throttle = min(IO_SCALE(vp, VNODE_ASYNC_THROTTLE), ((scale * max_cluster_size) / max_cluster) - 1);
			}
		}
	}
	if (flags & CL_AGE) {
		io_flags |= B_AGE;
	}
	if (flags & (CL_PAGEIN | CL_PAGEOUT)) {
		io_flags |= B_PAGEIO;
	}
	if (flags & (CL_IOSTREAMING)) {
		io_flags |= B_IOSTREAMING;
	}
	if (flags & CL_COMMIT) {
		io_flags |= B_COMMIT_UPL;
	}
	if (flags & CL_DIRECT_IO) {
		io_flags |= B_PHYS;
	}
	if (flags & (CL_PRESERVE | CL_KEEPCACHED)) {
		io_flags |= B_CACHE;
	}
	if (flags & CL_PASSIVE) {
		io_flags |= B_PASSIVE;
	}
	if (flags & CL_ENCRYPTED) {
		io_flags |= B_ENCRYPTED_IO;
	}

	if (vp->v_flag & VSYSTEM) {
		io_flags |= B_META;
	}

	if ((flags & CL_READ) && ((upl_offset + non_rounded_size) & PAGE_MASK) && (!(flags & CL_NOZERO))) {
		  
                                
                                                                        
                                                                 
                                                                    
                          
     
		zero_offset = (int)(upl_offset + non_rounded_size);
	} else if (!ISSET(flags, CL_READ) && ISSET(flags, CL_DIRECT_IO)) {
		assert(ISSET(flags, CL_COMMIT));

		                                                        

		upl_t cached_upl;

		  
                                                           
                          
     
		ubc_create_upl_kernel(vp, f_offset, non_rounded_size, &cached_upl,
		    NULL, UPL_SET_LITE, VM_KERN_MEMORY_FILE);

		  
                                                            
           
     
		upl_set_associated_upl(upl, cached_upl);

		if (upl_offset & PAGE_MASK) {
			  
                                                             
                                                           
                     
      
			upl_page_info_t *pl = UPL_GET_INTERNAL_PAGE_LIST(upl);
			upl_page_set_mark(pl, 0, true);
		}
	}

	while (size) {
		daddr64_t blkno;
		daddr64_t lblkno;
		u_int   io_size_wanted;
		size_t  io_size_tmp;

		if (size > max_iosize) {
			io_size = max_iosize;
		} else {
			io_size = size;
		}

		io_size_wanted = io_size;
		io_size_tmp = (size_t)io_size;

		if ((error = VNOP_BLOCKMAP(vp, f_offset, io_size, &blkno, &io_size_tmp, NULL, bmap_flags, NULL))) {
			break;
		}

		if (io_size_tmp > io_size_wanted) {
			io_size = io_size_wanted;
		} else {
			io_size = (u_int)io_size_tmp;
		}

		if (real_bp && (real_bp->b_blkno == real_bp->b_lblkno)) {
			real_bp->b_blkno = blkno;
		}

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 24)) | DBG_FUNC_NONE,
		    (int)f_offset, (int)(blkno >> 32), (int)blkno, io_size, 0);

		if (io_size == 0) {
			  
                                                             
                                                     
                                                        
                                                                     
                                                                         
      
			error = EINVAL;
			break;
		}
		if (!(flags & CL_READ) && blkno == -1) {
			off_t   e_offset;
			int     pageout_flags;

			if (upl_get_internal_vectorupl(upl)) {
				panic("Vector UPLs should not take this code-path\n");
			}
			  
                                 
      
			if (flags & CL_PAGEOUT) {
				  
                                         
                                             
                                                  
       
				error = EINVAL;
				break;
			}
			  
                                                    
                                                 
                                                
                                                
                                                       
     
                                                     
                                                      
                                                        
                                                     
                                                     
                             
     
                                                 
                                                    
                                                      
                                                                
     
                                                         
                                                        
                                                       
                                                                 
                                                               
                                         
      
			pageout_flags = UPL_MSYNC | UPL_VNODE_PAGER | UPL_NESTED_PAGEOUT;

			if (!(flags & CL_ASYNC)) {
				pageout_flags |= UPL_IOSYNC;
			}
			if (!(flags & CL_COMMIT)) {
				pageout_flags |= UPL_NOCOMMIT;
			}

			if (cbp_head) {
				buf_t prev_cbp;
				uint32_t   bytes_in_last_page;

				  
                                                                 
                                                                 
                                     
       
				cluster_wait_IO(cbp_head, (flags & CL_ASYNC));

				bytes_in_last_page = cbp_head->b_uploffset & PAGE_MASK;
				for (cbp = cbp_head; cbp; cbp = cbp->b_trans_next) {
					bytes_in_last_page += cbp->b_bcount;
				}
				bytes_in_last_page &= PAGE_MASK;

				while (bytes_in_last_page) {
					  
                                    
                                                                          
                                                                      
                                                                                    
                                                                              
       
                                                  
        
					for (prev_cbp = cbp = cbp_head; cbp->b_trans_next; cbp = cbp->b_trans_next) {
						prev_cbp = cbp;
					}

					if (bytes_in_last_page >= cbp->b_bcount) {
						  
                                                          
         
						bytes_in_last_page -= cbp->b_bcount;
						cbp->b_bcount = 0;

						free_io_buf(cbp);

						if (cbp == cbp_head) {
							assert(bytes_in_last_page == 0);
							  
                                                   
                                                     
          
							cbp_head = NULL;
							cbp_tail = NULL;
						} else {
							  
                                           
                              
          
							prev_cbp->b_trans_next = NULL;
							cbp_tail = prev_cbp;
						}
					} else {
						  
                                         
                                          
                                                    
                                                             
         
						cbp->b_bcount -= bytes_in_last_page;
						cbp_tail = cbp;
						bytes_in_last_page = 0;
					}
				}
				if (cbp_head) {
					  
                                                 
                                                                  
                                                            
                                                                            
        
					cluster_EOT(cbp_head, cbp_tail, 0);

					cluster_complete_transaction(&cbp_head, callback_arg, &retval, flags, 0);

					trans_count = 0;
				}
			}
			if (vnode_pageout(vp, upl, (upl_offset_t)trunc_page(upl_offset), trunc_page_64(f_offset), PAGE_SIZE, pageout_flags, NULL) != PAGER_SUCCESS) {
				error = EINVAL;
			}
			e_offset = round_page_64(f_offset + 1);
			io_size = (u_int)(e_offset - f_offset);

			f_offset   += io_size;
			upl_offset += io_size;

			if (size >= io_size) {
				size -= io_size;
			} else {
				size = 0;
			}
			  
                                                    
                                                       
                                                    
                                                             
      
			non_rounded_size -= io_size;

			if (non_rounded_size <= 0) {
				  
                                                        
                                                       
                                                    
                                                        
       
				size = 0;
			}
			if (error) {
				if (size == 0) {
					flags &= ~CL_COMMIT;
				}
				break;
			}
			continue;
		}
		lblkno = (daddr64_t)(f_offset / 0x1000);
		  
                                                                          
                                                                  
                                                                                
     
		pg_offset = upl_offset & PAGE_MASK;

		if (flags & CL_DEV_MEMORY) {
			  
                                                 
      
			pg_count = 1;
		} else {
			pg_count  = (io_size + pg_offset + (PAGE_SIZE - 1)) / PAGE_SIZE;
		}

		if ((flags & CL_READ) && blkno == -1) {
			vm_offset_t  commit_offset;
			int bytes_to_zero;
			int complete_transaction_now = 0;

			  
                                                        
                                                             
                                      
      
			if (io_size >= (u_int)non_rounded_size) {
				  
                                                                         
                                          
                                                                             
                                                                            
                                                   
                                                                          
                                                                            
       
				bytes_to_zero = non_rounded_size;
				if (!(flags & CL_NOZERO)) {
					bytes_to_zero = (int)((((upl_offset + io_size) + (PAGE_SIZE - 1)) & ~PAGE_MASK) - upl_offset);
				}

				zero_offset = 0;
			} else {
				bytes_to_zero = io_size;
			}

			pg_count = 0;

			cluster_zero(upl, (upl_offset_t)upl_offset, bytes_to_zero, real_bp);

			if (cbp_head) {
				int     pg_resid;

				  
                                              
                                                      
                                                        
                                             
       
				commit_offset = (upl_offset + (PAGE_SIZE - 1)) & ~PAGE_MASK;

				pg_resid = (int)(commit_offset - upl_offset);

				if (bytes_to_zero >= pg_resid) {
					  
                                        
                             
                                          
                                
                                         
                                           
                                        
                             
        
					if ((int)io_size >= non_rounded_size) {
						pg_count = (bytes_to_zero - pg_resid + (PAGE_SIZE - 1)) / PAGE_SIZE;
					} else {
						pg_count = (bytes_to_zero - pg_resid) / PAGE_SIZE;
					}

					complete_transaction_now = 1;
				}
			} else {
				  
                                  
                                               
                                        
                                          
                                       
                            
       
				if ((int)io_size >= non_rounded_size) {
					pg_count = (pg_offset + bytes_to_zero + (PAGE_SIZE - 1)) / PAGE_SIZE;
				} else {
					pg_count = (pg_offset + bytes_to_zero) / PAGE_SIZE;
				}

				commit_offset = upl_offset & ~PAGE_MASK;
			}

			                                                                 
			assert(!upl_associated_upl(upl));

			if ((flags & CL_COMMIT) && pg_count) {
				ubc_upl_commit_range(upl, (upl_offset_t)commit_offset,
				    pg_count * PAGE_SIZE,
				    UPL_COMMIT_CLEAR_DIRTY | UPL_COMMIT_FREE_ON_EMPTY);
			}
			upl_offset += io_size;
			f_offset   += io_size;
			size       -= io_size;

			  
                                                    
                                                       
                                                    
                                                             
      
			non_rounded_size -= io_size;

			if (non_rounded_size <= 0) {
				  
                                                        
                                                       
                                                    
                                                        
       
				size = 0;
			}
			if (cbp_head && (complete_transaction_now || size == 0)) {
				cluster_wait_IO(cbp_head, (flags & CL_ASYNC));

				cluster_EOT(cbp_head, cbp_tail, size == 0 ? zero_offset : 0);

				cluster_complete_transaction(&cbp_head, callback_arg, &retval, flags, 0);

				trans_count = 0;
			}
			continue;
		}
		if (pg_count > max_vectors) {
			if (((pg_count - max_vectors) * PAGE_SIZE) > io_size) {
				io_size = PAGE_SIZE - pg_offset;
				pg_count = 1;
			} else {
				io_size -= (pg_count - max_vectors) * PAGE_SIZE;
				pg_count = max_vectors;
			}
		}
		  
                                                               
                                                              
                                                          
                                        
    
                                                 
                                                             
     
		if (!(flags & CL_DEV_MEMORY) && trans_count >= max_trans_count &&
		    ((upl_offset + io_size) & PAGE_MASK)) {
			vm_offset_t aligned_ofs;

			aligned_ofs = (upl_offset + io_size) & ~PAGE_MASK;
			  
                                                        
                                                       
                                                           
     
                                                         
                                                         
                                                         
                                                      
      
			if (aligned_ofs > upl_offset) {
				io_size = (u_int)(aligned_ofs - upl_offset);
				pg_count--;
			}
		}

		if (!(mp->mnt_kern_flag & MNTK_VIRTUALDEV)) {
			  
                                                               
                                                               
                                                                
                                     
      
			priv = 1;
		} else if ((flags & CL_ASYNC) && !(flags & CL_PAGEOUT) && !cbp_head) {
			  
                                 
     
                                                        
                                                  
                                                    
      
			priv = 0;
		} else {
			priv = 1;
		}

		cbp = alloc_io_buf(vp, priv);

		if (flags & CL_PAGEOUT) {
			u_int i;

			  
                                                  
                                                  
      
			for (i = 0; i < (PAGE_SIZE * pg_count) / 0x1000; i++) {
				if (buf_invalblkno(vp, lblkno + i, 0) == EBUSY) {
					panic("BUSY bp found in cluster_io");
				}
			}
		}
		if (flags & CL_ASYNC) {
			if (buf_setcallback(cbp, (void *)cluster_iodone, callback_arg)) {
				panic("buf_setcallback failed\n");
			}
		}
		cbp->b_cliodone = (void *)callback;
		cbp->b_flags |= io_flags;
		if (flags & CL_NOCACHE) {
			cbp->b_attr.ba_flags |= BA_NOCACHE;
		}

		cbp->b_lblkno = lblkno;
		cbp->b_blkno  = blkno;
		cbp->b_bcount = io_size;

		if (buf_setupl(cbp, upl, (uint32_t)upl_offset)) {
			panic("buf_setupl failed\n");
		}
#if CONFIG_IOSCHED
		upl_set_blkno(upl, upl_offset, io_size, blkno);
#endif
		cbp->b_trans_next = (buf_t)NULL;

		if ((cbp->b_iostate = (void *)iostate)) {
			  
                                             
                                                      
      
			iostate->io_issued += io_size;
		}

		if (flags & CL_READ) {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 26)) | DBG_FUNC_NONE,
			    (int)cbp->b_lblkno, (int)cbp->b_blkno, upl_offset, io_size, 0);
		} else {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 27)) | DBG_FUNC_NONE,
			    (int)cbp->b_lblkno, (int)cbp->b_blkno, upl_offset, io_size, 0);
		}

		if (cbp_head) {
			cbp_tail->b_trans_next = cbp;
			cbp_tail = cbp;
		} else {
			cbp_head = cbp;
			cbp_tail = cbp;

			if ((cbp_head->b_real_bp = real_bp)) {
				real_bp = (buf_t)NULL;
			}
		}
		*(buf_t *)(&cbp->b_trans_head) = cbp_head;

		trans_count++;

		upl_offset += io_size;
		f_offset   += io_size;
		size       -= io_size;
		  
                                                   
                                                      
                                                   
                                                            
     
		non_rounded_size -= io_size;

		if (non_rounded_size <= 0) {
			  
                                                       
                                                      
                                                   
                                                       
      
			size = 0;
		}
		if (size == 0) {
			  
                                         
                                  
      
			need_EOT = TRUE;
		} else if (((flags & CL_DEV_MEMORY) || (upl_offset & PAGE_MASK) == 0) &&
		    ((flags & CL_ASYNC) || trans_count > max_trans_count)) {
			  
                                                     
                                                                    
           
                                          
                                             
             
                                     
                                                      
                                                     
                                                                          
                                         
      
			need_EOT = TRUE;
		}
		if (need_EOT == TRUE) {
			cluster_EOT(cbp_head, cbp_tail, size == 0 ? zero_offset : 0);
		}

		if (flags & CL_THROTTLE) {
			(void)vnode_waitforwrites(vp, async_throttle, 0, 0, "cluster_io");
		}

		if (!(io_flags & B_READ)) {
			vnode_startwrite(vp);
		}

		if (flags & CL_RAW_ENCRYPTED) {
			  
                                         
                                                    
      
			cbp->b_attr.ba_flags |= BA_RAW_ENCRYPTED_IO;
		}

		(void) VNOP_STRATEGY(cbp);

		if (need_EOT == TRUE) {
			if (!(flags & CL_ASYNC)) {
				cluster_complete_transaction(&cbp_head, callback_arg, &retval, flags, 1);
			}

			need_EOT = FALSE;
			trans_count = 0;
			cbp_head = NULL;
		}
	}
	if (error) {
		int abort_size;

		io_size = 0;

		if (cbp_head) {
			  
                                           
                                                
      
			cluster_wait_IO(cbp_head, (flags & CL_ASYNC));

			  
                                                   
                  
      
			upl_offset = cbp_head->b_uploffset;
		}

		if (ISSET(flags, CL_COMMIT)) {
			cluster_handle_associated_upl(iostate, upl,
			    (upl_offset_t)upl_offset,
			    (upl_size_t)(upl_end_offset - upl_offset));
		}

		                                              
		for (cbp = cbp_head; cbp;) {
			buf_t   cbp_next;

			size       += cbp->b_bcount;
			io_size    += cbp->b_bcount;

			cbp_next = cbp->b_trans_next;
			free_io_buf(cbp);
			cbp = cbp_next;
		}

		if (iostate) {
			int need_wakeup = 0;

			  
                                                
                                         
                                      
      
			lck_mtx_lock_spin(&iostate->io_mtxp);

			if (iostate->io_error == 0) {
				iostate->io_error = error;
			}
			iostate->io_issued -= io_size;

			if (iostate->io_wanted) {
				  
                                          
                               
       
				iostate->io_wanted = 0;
				need_wakeup = 1;
			}
			lck_mtx_unlock(&iostate->io_mtxp);

			if (need_wakeup) {
				wakeup((caddr_t)&iostate->io_wanted);
			}
		}

		if (flags & CL_COMMIT) {
			int     upl_flags;

			pg_offset  = upl_offset & PAGE_MASK;
			abort_size = (int)((upl_end_offset - upl_offset + PAGE_MASK) & ~PAGE_MASK);

			upl_flags = cluster_ioerror(upl, (int)(upl_offset - pg_offset),
			    abort_size, error, io_flags, vp);

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 28)) | DBG_FUNC_NONE,
			    upl, upl_offset - pg_offset, abort_size, (error << 24) | upl_flags, 0);
		}
		if (retval == 0) {
			retval = error;
		}
	} else if (cbp_head) {
		panic("%s(): cbp_head is not NULL.\n", __FUNCTION__);
	}

	if (real_bp) {
		  
                                                   
                                                 
                      
     
		if (error) {
			real_bp->b_flags |= B_ERROR;
			real_bp->b_error = error;
		}
		buf_biodone(real_bp);
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 22)) | DBG_FUNC_END, (int)f_offset, size, upl_offset, retval, 0);

	return retval;
}

#define reset_vector_run_state()                                                                                \
	issueVectorUPL = vector_upl_offset = vector_upl_index = vector_upl_iosize = vector_upl_size = 0;

static int
vector_cluster_io(vnode_t vp, upl_t vector_upl, vm_offset_t vector_upl_offset, off_t v_upl_uio_offset, int vector_upl_iosize,
    int io_flag, buf_t real_bp, struct clios *iostate, int (*callback)(buf_t, void *), void *callback_arg)
{
	vector_upl_set_pagelist(vector_upl);

	if (io_flag & CL_READ) {
		if (vector_upl_offset == 0 && ((vector_upl_iosize & PAGE_MASK) == 0)) {
			io_flag &= ~CL_PRESERVE;                    
		} else {
			io_flag |= CL_PRESERVE;              
		}
	}
	return cluster_io(vp, vector_upl, vector_upl_offset, v_upl_uio_offset, vector_upl_iosize, io_flag, real_bp, iostate, callback, callback_arg);
}

static int
cluster_read_prefetch(vnode_t vp, off_t f_offset, u_int size, off_t filesize, int (*callback)(buf_t, void *), void *callback_arg, int bflag)
{
	int           pages_in_prefetch;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 49)) | DBG_FUNC_START,
	    (int)f_offset, size, (int)filesize, 0, 0);

	if (f_offset >= filesize) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 49)) | DBG_FUNC_END,
		    (int)f_offset, 0, 0, 0, 0);
		return 0;
	}
	if ((off_t)size > (filesize - f_offset)) {
		size = (u_int)(filesize - f_offset);
	}
	pages_in_prefetch = (size + (PAGE_SIZE - 1)) / PAGE_SIZE;

	advisory_read_ext(vp, filesize, f_offset, size, callback, callback_arg, bflag);

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 49)) | DBG_FUNC_END,
	    (int)f_offset + size, pages_in_prefetch, 0, 1, 0);

	return pages_in_prefetch;
}



static void
cluster_read_ahead(vnode_t vp, struct cl_extent *extent, off_t filesize, struct cl_readahead *rap, int (*callback)(buf_t, void *), void *callback_arg,
    int bflag)
{
	daddr64_t       r_addr;
	off_t           f_offset;
	int             size_of_prefetch;
	u_int           max_prefetch;


	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 48)) | DBG_FUNC_START,
	    (int)extent->b_addr, (int)extent->e_addr, (int)rap->cl_lastr, 0, 0);

	if (extent->b_addr == rap->cl_lastr && extent->b_addr == extent->e_addr) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 48)) | DBG_FUNC_END,
		    rap->cl_ralen, (int)rap->cl_maxra, (int)rap->cl_lastr, 0, 0);
		return;
	}
	if (rap->cl_lastr == -1 || (extent->b_addr != rap->cl_lastr && extent->b_addr != (rap->cl_lastr + 1))) {
		rap->cl_ralen = 0;
		rap->cl_maxra = 0;

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 48)) | DBG_FUNC_END,
		    rap->cl_ralen, (int)rap->cl_maxra, (int)rap->cl_lastr, 1, 0);

		return;
	}
	max_prefetch = MAX_PREFETCH(vp, cluster_max_io_size(vp->v_mount, CL_READ), disk_conditioner_mount_is_ssd(vp->v_mount));

	if (max_prefetch > speculative_prefetch_max) {
		max_prefetch = speculative_prefetch_max;
	}

	if (max_prefetch <= PAGE_SIZE) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 48)) | DBG_FUNC_END,
		    rap->cl_ralen, (int)rap->cl_maxra, (int)rap->cl_lastr, 6, 0);
		return;
	}
	if (extent->e_addr < rap->cl_maxra && rap->cl_ralen >= 4) {
		if ((rap->cl_maxra - extent->e_addr) > (rap->cl_ralen / 4)) {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 48)) | DBG_FUNC_END,
			    rap->cl_ralen, (int)rap->cl_maxra, (int)rap->cl_lastr, 2, 0);
			return;
		}
	}
	r_addr = MAX(extent->e_addr, rap->cl_maxra) + 1;
	f_offset = (off_t)(r_addr * PAGE_SIZE_64);

	size_of_prefetch = 0;

	ubc_range_op(vp, f_offset, f_offset + PAGE_SIZE_64, UPL_ROP_PRESENT, &size_of_prefetch);

	if (size_of_prefetch) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 48)) | DBG_FUNC_END,
		    rap->cl_ralen, (int)rap->cl_maxra, (int)rap->cl_lastr, 3, 0);
		return;
	}
	if (f_offset < filesize) {
		daddr64_t read_size;

		rap->cl_ralen = rap->cl_ralen ? min(max_prefetch / PAGE_SIZE, rap->cl_ralen << 1) : 1;

		read_size = (extent->e_addr + 1) - extent->b_addr;

		if (read_size > rap->cl_ralen) {
			if (read_size > max_prefetch / PAGE_SIZE) {
				rap->cl_ralen = max_prefetch / PAGE_SIZE;
			} else {
				rap->cl_ralen = (int)read_size;
			}
		}
		size_of_prefetch = cluster_read_prefetch(vp, f_offset, rap->cl_ralen * PAGE_SIZE, filesize, callback, callback_arg, bflag);

		if (size_of_prefetch) {
			rap->cl_maxra = (r_addr + size_of_prefetch) - 1;
		}
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 48)) | DBG_FUNC_END,
	    rap->cl_ralen, (int)rap->cl_maxra, (int)rap->cl_lastr, 4, 0);
}


int
cluster_pageout(vnode_t vp, upl_t upl, upl_offset_t upl_offset, off_t f_offset,
    int size, off_t filesize, int flags)
{
	return cluster_pageout_ext(vp, upl, upl_offset, f_offset, size, filesize, flags, NULL, NULL);
}


int
cluster_pageout_ext(vnode_t vp, upl_t upl, upl_offset_t upl_offset, off_t f_offset,
    int size, off_t filesize, int flags, int (*callback)(buf_t, void *), void *callback_arg)
{
	int           io_size;
	int           rounded_size;
	off_t         max_size;
	int           local_flags;

	local_flags = CL_PAGEOUT | CL_THROTTLE;

	if ((flags & UPL_IOSYNC) == 0) {
		local_flags |= CL_ASYNC;
	}
	if ((flags & UPL_NOCOMMIT) == 0) {
		local_flags |= CL_COMMIT;
	}
	if ((flags & UPL_KEEPCACHED)) {
		local_flags |= CL_KEEPCACHED;
	}
	if (flags & UPL_PAGING_ENCRYPTED) {
		local_flags |= CL_ENCRYPTED;
	}


	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 52)) | DBG_FUNC_NONE,
	    (int)f_offset, size, (int)filesize, local_flags, 0);

	  
                                                       
                                                     
                         
    
	if (size <= 0) {
		return EINVAL;
	}

	if (vp->v_mount->mnt_flag & MNT_RDONLY) {
		if (local_flags & CL_COMMIT) {
			ubc_upl_abort_range(upl, upl_offset, size, UPL_ABORT_FREE_ON_EMPTY);
		}
		return EROFS;
	}
	  
                                        
                                       
                                            
                                                       
    
	if (f_offset < 0 || f_offset >= filesize ||
	    (f_offset & PAGE_MASK_64) || (size & PAGE_MASK)) {
		if (local_flags & CL_COMMIT) {
			ubc_upl_abort_range(upl, upl_offset, size, UPL_ABORT_FREE_ON_EMPTY);
		}
		return EINVAL;
	}
	max_size = filesize - f_offset;

	if (size < max_size) {
		io_size = size;
	} else {
		io_size = (int)max_size;
	}

	rounded_size = (io_size + (PAGE_SIZE - 1)) & ~PAGE_MASK;

	if (size > rounded_size) {
		if (local_flags & CL_COMMIT) {
			ubc_upl_abort_range(upl, upl_offset + rounded_size, size - rounded_size,
			    UPL_ABORT_FREE_ON_EMPTY);
		}
	}
	return cluster_io(vp, upl, upl_offset, f_offset, io_size,
	           local_flags, (buf_t)NULL, (struct clios *)NULL, callback, callback_arg);
}


int
cluster_pagein(vnode_t vp, upl_t upl, upl_offset_t upl_offset, off_t f_offset,
    int size, off_t filesize, int flags)
{
	return cluster_pagein_ext(vp, upl, upl_offset, f_offset, size, filesize, flags, NULL, NULL);
}


int
cluster_pagein_ext(vnode_t vp, upl_t upl, upl_offset_t upl_offset, off_t f_offset,
    int size, off_t filesize, int flags, int (*callback)(buf_t, void *), void *callback_arg)
{
	u_int         io_size;
	int           rounded_size;
	off_t         max_size;
	int           retval;
	int           local_flags = 0;

	if (upl == NULL || size < 0) {
		panic("cluster_pagein: NULL upl passed in");
	}

	if ((flags & UPL_IOSYNC) == 0) {
		local_flags |= CL_ASYNC;
	}
	if ((flags & UPL_NOCOMMIT) == 0) {
		local_flags |= CL_COMMIT;
	}
	if (flags & UPL_IOSTREAMING) {
		local_flags |= CL_IOSTREAMING;
	}
	if (flags & UPL_PAGING_ENCRYPTED) {
		local_flags |= CL_ENCRYPTED;
	}


	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 56)) | DBG_FUNC_NONE,
	    (int)f_offset, size, (int)filesize, local_flags, 0);

	  
                                        
                                       
                                            
                                                       
    
	if (f_offset < 0 || f_offset >= filesize ||
	    (f_offset & PAGE_MASK_64) || (size & PAGE_MASK) || (upl_offset & PAGE_MASK)) {
		if (local_flags & CL_COMMIT) {
			ubc_upl_abort_range(upl, upl_offset, size, UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_ERROR);
		}
		return EINVAL;
	}
	max_size = filesize - f_offset;

	if (size < max_size) {
		io_size = size;
	} else {
		io_size = (int)max_size;
	}

	rounded_size = (io_size + (PAGE_SIZE - 1)) & ~PAGE_MASK;

	if (size > rounded_size && (local_flags & CL_COMMIT)) {
		ubc_upl_abort_range(upl, upl_offset + rounded_size,
		    size - rounded_size, UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_ERROR);
	}

	retval = cluster_io(vp, upl, upl_offset, f_offset, io_size,
	    local_flags | CL_READ | CL_PAGEIN, (buf_t)NULL, (struct clios *)NULL, callback, callback_arg);

	return retval;
}


int
cluster_bp(buf_t bp)
{
	return cluster_bp_ext(bp, NULL, NULL);
}


int
cluster_bp_ext(buf_t bp, int (*callback)(buf_t, void *), void *callback_arg)
{
	off_t  f_offset;
	int    flags;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 19)) | DBG_FUNC_START,
	    bp, (int)bp->b_lblkno, bp->b_bcount, bp->b_flags, 0);

	if (bp->b_flags & B_READ) {
		flags = CL_ASYNC | CL_READ;
	} else {
		flags = CL_ASYNC;
	}
	if (bp->b_flags & B_PASSIVE) {
		flags |= CL_PASSIVE;
	}

	f_offset = ubc_blktooff(bp->b_vp, bp->b_lblkno);

	return cluster_io(bp->b_vp, bp->b_upl, 0, f_offset, bp->b_bcount, flags, bp, (struct clios *)NULL, callback, callback_arg);
}



int
cluster_write(vnode_t vp, struct uio *uio, off_t oldEOF, off_t newEOF, off_t headOff, off_t tailOff, int xflags)
{
	return cluster_write_ext(vp, uio, oldEOF, newEOF, headOff, tailOff, xflags, NULL, NULL);
}


int
cluster_write_ext(vnode_t vp, struct uio *uio, off_t oldEOF, off_t newEOF, off_t headOff, off_t tailOff,
    int xflags, int (*callback)(buf_t, void *), void *callback_arg)
{
	user_ssize_t    cur_resid;
	int             retval = 0;
	int             flags;
	int             zflags;
	int             bflag;
	int             write_type = IO_COPY;
	u_int32_t       write_length;

	flags = xflags;

	if (flags & IO_PASSIVE) {
		bflag = CL_PASSIVE;
	} else {
		bflag = 0;
	}

	if (vp->v_flag & VNOCACHE_DATA) {
		flags |= IO_NOCACHE;
		bflag |= CL_NOCACHE;
	}
	if (uio == NULL) {
		  
                    
                                                                
     
		retval = cluster_write_copy(vp, NULL, (u_int32_t)0, oldEOF, newEOF, headOff, tailOff, flags, callback, callback_arg);

		return retval;
	}
	  
                                                                    
                                             
                                              
                                                                   
                                       
    
	if (((flags & (IO_NOCACHE | IO_NODIRECT)) == IO_NOCACHE) && UIO_SEG_IS_USER_SPACE(uio->uio_segflg)) {
		retval = cluster_io_type(uio, &write_type, &write_length, MIN_DIRECT_WRITE_SIZE);
	}

	if ((flags & (IO_TAILZEROFILL | IO_HEADZEROFILL)) && write_type == IO_DIRECT) {
		  
                                                    
     
		write_type = IO_COPY;
	}

	while ((cur_resid = uio_resid(uio)) && uio->uio_offset < newEOF && retval == 0) {
		switch (write_type) {
		case IO_COPY:
			  
                                              
                                                     
                                          
      
			if (cur_resid > (user_ssize_t)(MAX_IO_REQUEST_SIZE)) {
				  
                                                     
                        
      
                                                       
                                                     
                                             
       
				zflags = flags & ~IO_TAILZEROFILL;
				flags &= ~IO_HEADZEROFILL;

				write_length = MAX_IO_REQUEST_SIZE;
			} else {
				  
                                      
       
				zflags = flags;

				write_length = (u_int32_t)cur_resid;
			}
			retval = cluster_write_copy(vp, uio, write_length, oldEOF, newEOF, headOff, tailOff, zflags, callback, callback_arg);
			break;

		case IO_CONTIG:
			zflags = flags & ~(IO_TAILZEROFILL | IO_HEADZEROFILL);

			if (flags & IO_HEADZEROFILL) {
				  
                                    
       
				flags &= ~IO_HEADZEROFILL;

				retval = cluster_write_copy(vp, (struct uio *)0, (u_int32_t)0, (off_t)0, uio->uio_offset,
				    headOff, (off_t)0, zflags | IO_HEADZEROFILL | IO_SYNC, callback, callback_arg);
				if (retval) {
					break;
				}
			}
			retval = cluster_write_contig(vp, uio, newEOF, &write_type, &write_length, callback, callback_arg, bflag);

			if (retval == 0 && (flags & IO_TAILZEROFILL) && uio_resid(uio) == 0) {
				  
                                                                 
                                                        
                                                                   
                                                             
       
				retval = cluster_write_copy(vp, (struct uio *)0, (u_int32_t)0, (off_t)0, tailOff, uio->uio_offset,
				    (off_t)0, zflags | IO_HEADZEROFILL | IO_SYNC, callback, callback_arg);
			}
			break;

		case IO_DIRECT:
			  
                                                                                  
      
			retval = cluster_write_direct(vp, uio, oldEOF, newEOF, &write_type, &write_length, flags, callback, callback_arg);
			break;

		case IO_UNKNOWN:
			retval = cluster_io_type(uio, &write_type, &write_length, MIN_DIRECT_WRITE_SIZE);
			break;
		}
		  
                                                                             
                                                                                  
                                            
                                                                     
                                                                           
                                                                        
                                                                      
                                                                      
     
		if (uio->uio_offset > oldEOF) {
			oldEOF = uio->uio_offset;
		}
	}
	return retval;
}


static int
cluster_write_direct(vnode_t vp, struct uio *uio, off_t oldEOF, off_t newEOF, int *write_type, u_int32_t *write_length,
    int flags, int (*callback)(buf_t, void *), void *callback_arg)
{
	upl_t            upl;
	upl_page_info_t  *pl;
	vm_offset_t      upl_offset;
	vm_offset_t      vector_upl_offset = 0;
	u_int32_t        io_req_size;
	u_int32_t        offset_in_file;
	u_int32_t        offset_in_iovbase;
	u_int32_t        io_size;
	int              io_flag = 0;
	upl_size_t       upl_size, vector_upl_size = 0;
	vm_size_t        upl_needed_size;
	mach_msg_type_number_t  pages_in_pl;
	upl_control_flags_t upl_flags;
	kern_return_t    kret;
	mach_msg_type_number_t  i;
	int              force_data_sync;
	int              retval = 0;
	int              first_IO = 1;
	struct clios     iostate;
	user_addr_t      iov_base;
	u_int32_t        mem_alignment_mask;
	u_int32_t        devblocksize;
	u_int32_t        max_io_size;
	u_int32_t        max_upl_size;
	u_int32_t        max_vector_size;
	u_int32_t        bytes_outstanding_limit;
	boolean_t        io_throttled = FALSE;

	u_int32_t        vector_upl_iosize = 0;
	int              issueVectorUPL = 0, useVectorUPL = (uio->uio_iovcnt > 1);
	off_t            v_upl_uio_offset = 0;
	int              vector_upl_index = 0;
	upl_t            vector_upl = NULL;


	  
                                       
                                         
    
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 75)) | DBG_FUNC_START,
	    (int)uio->uio_offset, *write_length, (int)newEOF, 0, 0);

	assert(vm_map_page_shift(current_map()) >= PAGE_SHIFT);

	max_upl_size = cluster_max_io_size(vp->v_mount, CL_WRITE);

	io_flag = CL_ASYNC | CL_PRESERVE | CL_COMMIT | CL_THROTTLE | CL_DIRECT_IO;

	if (flags & IO_PASSIVE) {
		io_flag |= CL_PASSIVE;
	}

	if (flags & IO_NOCACHE) {
		io_flag |= CL_NOCACHE;
	}

	if (flags & IO_SKIP_ENCRYPTION) {
		io_flag |= CL_ENCRYPTED;
	}

	iostate.io_completed = 0;
	iostate.io_issued = 0;
	iostate.io_error = 0;
	iostate.io_wanted = 0;

	lck_mtx_init(&iostate.io_mtxp, &cl_mtx_grp, LCK_ATTR_NULL);

	mem_alignment_mask = (u_int32_t)vp->v_mount->mnt_alignmentmask;
	devblocksize = (u_int32_t)vp->v_mount->mnt_devblocksize;

	if (devblocksize == 1) {
		  
                                                  
                                                   
                                         
                                                             
                                             
                                                        
                                                          
                       
     
		devblocksize = PAGE_SIZE;
	}

next_dwrite:
	io_req_size = *write_length;
	iov_base = uio_curriovbase(uio);

	offset_in_file = (u_int32_t)uio->uio_offset & PAGE_MASK;
	offset_in_iovbase = (u_int32_t)iov_base & mem_alignment_mask;

	if (offset_in_file || offset_in_iovbase) {
		  
                                                 
                                                            
     
		goto wait_for_dwrites;
	}
	if (iov_base & (devblocksize - 1)) {
		  
                                                            
                                                     
                                                   
     
		goto wait_for_dwrites;
	}

	task_update_logical_writes(current_task(), (io_req_size & ~PAGE_MASK), TASK_WRITE_IMMEDIATE, vp);
	while (io_req_size >= PAGE_SIZE && uio->uio_offset < newEOF && retval == 0) {
		int     throttle_type;

		if ((throttle_type = cluster_is_throttled(vp))) {
			  
                                                     
                                                      
              
      
			if ((flags & IO_RETURN_ON_THROTTLE) && throttle_type == THROTTLE_NOW) {
				  
                                                      
                                                       
                                                        
                                                       
                                                   
       
				throttle_info_update_by_mount(vp->v_mount);

				io_throttled = TRUE;
				goto wait_for_dwrites;
			}
			max_vector_size = THROTTLE_MAX_IOSIZE;
			max_io_size = THROTTLE_MAX_IOSIZE;
		} else {
			max_vector_size = MAX_VECTOR_UPL_SIZE;
			max_io_size = max_upl_size;
		}

		if (first_IO) {
			cluster_syncup(vp, newEOF, callback, callback_arg, callback ? PUSH_SYNC : 0);
			first_IO = 0;
		}
		io_size  = io_req_size & ~PAGE_MASK;
		iov_base = uio_curriovbase(uio);

		if (io_size > max_io_size) {
			io_size = max_io_size;
		}

		if (useVectorUPL && (iov_base & PAGE_MASK)) {
			  
                                                  
                                                     
                        
      
			if (vector_upl_index) {
				retval = vector_cluster_io(vp, vector_upl, vector_upl_offset, v_upl_uio_offset, vector_upl_iosize, io_flag, (buf_t)NULL, &iostate, callback, callback_arg);
				reset_vector_run_state();
			}

			  
                                                                           
                                                                                       
      
		}

		upl_offset = (vm_offset_t)((u_int32_t)iov_base & PAGE_MASK);
		upl_needed_size = (upl_offset + io_size + (PAGE_SIZE - 1)) & ~PAGE_MASK;

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 76)) | DBG_FUNC_START,
		    (int)upl_offset, upl_needed_size, (int)iov_base, io_size, 0);

		vm_map_t map = UIO_SEG_IS_USER_SPACE(uio->uio_segflg) ? current_map() : kernel_map;
		for (force_data_sync = 0; force_data_sync < 3; force_data_sync++) {
			pages_in_pl = 0;
			upl_size = (upl_size_t)upl_needed_size;
			upl_flags = UPL_FILE_IO | UPL_COPYOUT_FROM | UPL_NO_SYNC |
			    UPL_CLEAN_IN_PLACE | UPL_SET_INTERNAL | UPL_SET_LITE | UPL_SET_IO_WIRE;

			kret = vm_map_get_upl(map,
			    vm_map_trunc_page(iov_base, vm_map_page_mask(map)),
			    &upl_size,
			    &upl,
			    NULL,
			    &pages_in_pl,
			    &upl_flags,
			    VM_KERN_MEMORY_FILE,
			    force_data_sync);

			if (kret != KERN_SUCCESS) {
				KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 76)) | DBG_FUNC_END,
				    0, 0, 0, kret, 0);
				  
                             
      
                                                            
                                                           
                                   
       
				goto wait_for_dwrites;
			}
			pl = UPL_GET_INTERNAL_PAGE_LIST(upl);
			pages_in_pl = upl_size / PAGE_SIZE;

			for (i = 0; i < pages_in_pl; i++) {
				if (!upl_valid_page(pl, i)) {
					break;
				}
			}
			if (i == pages_in_pl) {
				break;
			}

			  
                                           
                                              
      
			ubc_upl_abort(upl, 0);
		}
		if (force_data_sync >= 3) {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 76)) | DBG_FUNC_END,
			    i, pages_in_pl, upl_size, kret, 0);
			  
                                                        
                                                  
     
                                                           
                                                          
                                  
      
			goto wait_for_dwrites;
		}

		  
                                                             
     
		if (upl_size < upl_needed_size) {
			if (upl_size && upl_offset == 0) {
				io_size = upl_size;
			} else {
				io_size = 0;
			}
		}
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 76)) | DBG_FUNC_END,
		    (int)upl_offset, upl_size, (int)iov_base, io_size, 0);

		if (io_size == 0) {
			ubc_upl_abort(upl, 0);
			  
                                                           
                                                          
                                  
      
			goto wait_for_dwrites;
		}

		if (useVectorUPL) {
			vm_offset_t end_off = ((iov_base + io_size) & PAGE_MASK);
			if (end_off) {
				issueVectorUPL = 1;
			}
			  
                                                          
                                                           
                                                          
                         
      
		}

		  
                                                                        
                                    
                                                     
                                                     
     
		if (vp->v_mount->mnt_minsaturationbytecount) {
			bytes_outstanding_limit = vp->v_mount->mnt_minsaturationbytecount;
		} else {
			bytes_outstanding_limit = max_upl_size * IO_SCALE(vp, 2);
		}

		cluster_iostate_wait(&iostate, bytes_outstanding_limit, "cluster_write_direct");

		if (iostate.io_error) {
			  
                                                               
                                                  
                                              
                                                         
                                                          
      
			ubc_upl_abort(upl, 0);

			goto wait_for_dwrites;
		}

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 77)) | DBG_FUNC_START,
		    (int)upl_offset, (int)uio->uio_offset, io_size, io_flag, 0);

		if (!useVectorUPL) {
			retval = cluster_io(vp, upl, upl_offset, uio->uio_offset,
			    io_size, io_flag, (buf_t)NULL, &iostate, callback, callback_arg);
		} else {
			if (!vector_upl_index) {
				vector_upl = vector_upl_create(upl_offset);
				v_upl_uio_offset = uio->uio_offset;
				vector_upl_offset = upl_offset;
			}

			vector_upl_set_subupl(vector_upl, upl, upl_size);
			vector_upl_set_iostate(vector_upl, upl, vector_upl_size, upl_size);
			vector_upl_index++;
			vector_upl_iosize += io_size;
			vector_upl_size += upl_size;

			if (issueVectorUPL || vector_upl_index == MAX_VECTOR_UPL_ELEMENTS || vector_upl_size >= max_vector_size) {
				retval = vector_cluster_io(vp, vector_upl, vector_upl_offset, v_upl_uio_offset, vector_upl_iosize, io_flag, (buf_t)NULL, &iostate, callback, callback_arg);
				reset_vector_run_state();
			}
		}

		  
                                
                                        
     
		uio_update(uio, (user_size_t)io_size);

		  
                                                                      
                                                                      
                                                                     
                                                                           
                                                                        
                                                                      
                                                                      
     
		if (uio->uio_offset > oldEOF) {
			oldEOF = uio->uio_offset;
		}

		io_req_size -= io_size;

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 77)) | DBG_FUNC_END,
		    (int)upl_offset, (int)uio->uio_offset, io_req_size, retval, 0);
	}                

	if (retval == 0 && iostate.io_error == 0 && io_req_size == 0) {
		retval = cluster_io_type(uio, write_type, write_length, MIN_DIRECT_WRITE_SIZE);

		if (retval == 0 && *write_type == IO_DIRECT) {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 75)) | DBG_FUNC_NONE,
			    (int)uio->uio_offset, *write_length, (int)newEOF, 0, 0);

			goto next_dwrite;
		}
	}

wait_for_dwrites:

	if (retval == 0 && iostate.io_error == 0 && useVectorUPL && vector_upl_index) {
		retval = vector_cluster_io(vp, vector_upl, vector_upl_offset, v_upl_uio_offset, vector_upl_iosize, io_flag, (buf_t)NULL, &iostate, callback, callback_arg);
		reset_vector_run_state();
	}
	  
                                                            
                                   
    
	cluster_iostate_wait(&iostate, 0, "cluster_write_direct");

	if (iostate.io_error) {
		retval = iostate.io_error;
	}

	lck_mtx_destroy(&iostate.io_mtxp, &cl_mtx_grp);

	if (io_throttled == TRUE && retval == 0) {
		retval = EAGAIN;
	}

	if (io_req_size && retval == 0) {
		  
                                                               
                                     
    
                                                                           
                                                         
     
		if (uio->uio_offset > oldEOF) {
			oldEOF = uio->uio_offset;
		}

		retval = cluster_write_copy(vp, uio, io_req_size, oldEOF, newEOF, (off_t)0, (off_t)0, flags, callback, callback_arg);

		*write_type = IO_UNKNOWN;
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 75)) | DBG_FUNC_END,
	    (int)uio->uio_offset, io_req_size, retval, 4, 0);

	return retval;
}


static int
cluster_write_contig(vnode_t vp, struct uio *uio, off_t newEOF, int *write_type, u_int32_t *write_length,
    int (*callback)(buf_t, void *), void *callback_arg, int bflag)
{
	upl_page_info_t *pl;
	addr64_t         src_paddr = 0;
	upl_t            upl[MAX_VECTS];
	vm_offset_t      upl_offset;
	u_int32_t        tail_size = 0;
	u_int32_t        io_size;
	u_int32_t        xsize;
	upl_size_t       upl_size;
	vm_size_t        upl_needed_size;
	mach_msg_type_number_t  pages_in_pl;
	upl_control_flags_t upl_flags;
	kern_return_t    kret;
	struct clios     iostate;
	int              error  = 0;
	int              cur_upl = 0;
	int              num_upl = 0;
	int              n;
	user_addr_t      iov_base;
	u_int32_t        devblocksize;
	u_int32_t        mem_alignment_mask;

	  
                                       
                                               
                                                   
    
	cluster_syncup(vp, newEOF, callback, callback_arg, callback ? PUSH_SYNC : 0);

	devblocksize = (u_int32_t)vp->v_mount->mnt_devblocksize;
	mem_alignment_mask = (u_int32_t)vp->v_mount->mnt_alignmentmask;

	iostate.io_completed = 0;
	iostate.io_issued = 0;
	iostate.io_error = 0;
	iostate.io_wanted = 0;

	lck_mtx_init(&iostate.io_mtxp, &cl_mtx_grp, LCK_ATTR_NULL);

next_cwrite:
	io_size = *write_length;

	iov_base = uio_curriovbase(uio);

	upl_offset = (vm_offset_t)((u_int32_t)iov_base & PAGE_MASK);
	upl_needed_size = upl_offset + io_size;

	pages_in_pl = 0;
	upl_size = (upl_size_t)upl_needed_size;
	upl_flags = UPL_FILE_IO | UPL_COPYOUT_FROM | UPL_NO_SYNC |
	    UPL_CLEAN_IN_PLACE | UPL_SET_INTERNAL | UPL_SET_LITE | UPL_SET_IO_WIRE;

	vm_map_t map = UIO_SEG_IS_USER_SPACE(uio->uio_segflg) ? current_map() : kernel_map;
	kret = vm_map_get_upl(map,
	    vm_map_trunc_page(iov_base, vm_map_page_mask(map)),
	    &upl_size, &upl[cur_upl], NULL, &pages_in_pl, &upl_flags, VM_KERN_MEMORY_FILE, 0);

	if (kret != KERN_SUCCESS) {
		  
                           
     
		error = EINVAL;
		goto wait_for_cwrites;
	}
	num_upl++;

	  
                                                            
    
	if (upl_size < upl_needed_size) {
		  
                                                   
     
		error = EINVAL;
		goto wait_for_cwrites;
	}
	pl = ubc_upl_pageinfo(upl[cur_upl]);

	src_paddr = ((addr64_t)upl_phys_page(pl, 0) << PAGE_SHIFT) + (addr64_t)upl_offset;

	while (((uio->uio_offset & (devblocksize - 1)) || io_size < devblocksize) && io_size) {
		u_int32_t   head_size;

		head_size = devblocksize - (u_int32_t)(uio->uio_offset & (devblocksize - 1));

		if (head_size > io_size) {
			head_size = io_size;
		}

		error = cluster_align_phys_io(vp, uio, src_paddr, head_size, 0, callback, callback_arg);

		if (error) {
			goto wait_for_cwrites;
		}

		upl_offset += head_size;
		src_paddr  += head_size;
		io_size    -= head_size;

		iov_base   += head_size;
	}
	if ((u_int32_t)iov_base & mem_alignment_mask) {
		  
                                                
                                            
                                             
                                                
                                          
     
		error = EINVAL;
		goto wait_for_cwrites;
	}

	tail_size = io_size & (devblocksize - 1);
	io_size  -= tail_size;

	while (io_size && error == 0) {
		if (io_size > MAX_IO_CONTIG_SIZE) {
			xsize = MAX_IO_CONTIG_SIZE;
		} else {
			xsize = io_size;
		}
		  
                                                  
                                                
                                               
                                              
                                                     
                                                           
     
		cluster_iostate_wait(&iostate, MAX_IO_CONTIG_SIZE * IO_SCALE(vp, 2), "cluster_write_contig");

		if (iostate.io_error) {
			  
                                                               
                                    
                                                         
                                                          
      
			goto wait_for_cwrites;
		}
		  
                                              
     
		error = cluster_io(vp, upl[cur_upl], upl_offset, uio->uio_offset,
		    xsize, CL_DEV_MEMORY | CL_ASYNC | bflag, (buf_t)NULL, (struct clios *)&iostate, callback, callback_arg);

		if (error == 0) {
			  
                                                  
                              
      
			uio_update(uio, (user_size_t)xsize);

			upl_offset += xsize;
			src_paddr  += xsize;
			io_size    -= xsize;
		}
	}
	if (error == 0 && iostate.io_error == 0 && tail_size == 0 && num_upl < MAX_VECTS) {
		error = cluster_io_type(uio, write_type, write_length, 0);

		if (error == 0 && *write_type == IO_CONTIG) {
			cur_upl++;
			goto next_cwrite;
		}
	} else {
		*write_type = IO_UNKNOWN;
	}

wait_for_cwrites:
	  
                                                           
                                    
    
	cluster_iostate_wait(&iostate, 0, "cluster_write_contig");

	if (iostate.io_error) {
		error = iostate.io_error;
	}

	lck_mtx_destroy(&iostate.io_mtxp, &cl_mtx_grp);

	if (error == 0 && tail_size) {
		error = cluster_align_phys_io(vp, uio, src_paddr, tail_size, 0, callback, callback_arg);
	}

	for (n = 0; n < num_upl; n++) {
		  
                                                        
                                      
     
		ubc_upl_abort(upl[n], 0);
	}

	return error;
}


  
                                                                             
                                                                                     
                                                                                      
  
                                                                               
                                                                                
                                                               
  
   
static int
cluster_zero_range(upl_t upl, upl_page_info_t *pl, int flags, int io_offset, off_t zero_off, off_t upl_f_offset, int bytes_to_zero)
{
	int zero_pg_index;
	boolean_t need_cluster_zero = TRUE;

	if ((flags & (IO_NOZEROVALID | IO_NOZERODIRTY))) {
		bytes_to_zero = min(bytes_to_zero, PAGE_SIZE - (int)(zero_off & PAGE_MASK_64));
		zero_pg_index = (int)((zero_off - upl_f_offset) / PAGE_SIZE_64);

		if (upl_valid_page(pl, zero_pg_index)) {
			  
                                                   
                                                                      
      
			need_cluster_zero = FALSE;
		}
	}
	if (need_cluster_zero == TRUE) {
		cluster_zero(upl, io_offset, bytes_to_zero, NULL);
	}

	return bytes_to_zero;
}


void
cluster_update_state(vnode_t vp, vm_object_offset_t s_offset, vm_object_offset_t e_offset, boolean_t vm_initiated)
{
	struct cl_extent cl;
	boolean_t first_pass = TRUE;

	assert(s_offset < e_offset);
	assert((s_offset & PAGE_MASK_64) == 0);
	assert((e_offset & PAGE_MASK_64) == 0);

	cl.b_addr = (daddr64_t)(s_offset / PAGE_SIZE_64);
	cl.e_addr = (daddr64_t)(e_offset / PAGE_SIZE_64);

	cluster_update_state_internal(vp, &cl, 0, TRUE, &first_pass, s_offset, (int)(e_offset - s_offset),
	    vp->v_un.vu_ubcinfo->ui_size, NULL, NULL, vm_initiated);
}


static void
cluster_update_state_internal(vnode_t vp, struct cl_extent *cl, int flags, boolean_t defer_writes,
    boolean_t *first_pass, off_t write_off, int write_cnt, off_t newEOF,
    int (*callback)(buf_t, void *), void *callback_arg, boolean_t vm_initiated)
{
	struct cl_writebehind *wbp;
	int     cl_index;
	int     ret_cluster_try_push;
	u_int   max_cluster_pgcount;


	max_cluster_pgcount = MAX_CLUSTER_SIZE(vp) / PAGE_SIZE;

	  
                                         
                                               
    
	wbp = cluster_get_wbp(vp, CLW_ALLOCATE | CLW_RETURNLOCKED);

	if (wbp->cl_scmap) {
		if (!(flags & IO_NOCACHE)) {
			  
                                  
                                            
      
			sparse_cluster_add(wbp, &(wbp->cl_scmap), vp, cl, newEOF, callback, callback_arg, vm_initiated);

			lck_mtx_unlock(&wbp->cl_lockw);
			return;
		}
		  
                                                
                                                   
                                                
                                          
                                         
     
		wbp->cl_number = 0;

		sparse_cluster_push(wbp, &(wbp->cl_scmap), vp, newEOF, PUSH_ALL, 0, callback, callback_arg, vm_initiated);
		  
                                                     
                                                   
                                                  
                                                   
                                                   
     
		goto start_new_cluster;
	}
	if (*first_pass == TRUE) {
		if (write_off == wbp->cl_last_write) {
			wbp->cl_seq_written += write_cnt;
		} else {
			wbp->cl_seq_written = write_cnt;
		}

		wbp->cl_last_write = write_off + write_cnt;

		*first_pass = FALSE;
	}
	if (wbp->cl_number == 0) {
		  
                                  
     
		goto start_new_cluster;
	}

	for (cl_index = 0; cl_index < wbp->cl_number; cl_index++) {
		  
                                              
                                                
                                               
                                              
                
     
		if (cl->b_addr >= wbp->cl_clusters[cl_index].b_addr) {
			  
                                                              
      
			if (cl->e_addr <= (wbp->cl_clusters[cl_index].b_addr + max_cluster_pgcount)) {
				  
                                         
                                         
       
				if (cl->e_addr > wbp->cl_clusters[cl_index].e_addr) {
					  
                                                 
        
					wbp->cl_clusters[cl_index].e_addr = cl->e_addr;
				}
				break;
			}
			if (cl->b_addr < (wbp->cl_clusters[cl_index].b_addr + max_cluster_pgcount)) {
				  
                                                                       
                                                                     
                             
                                                  
                                                                  
                                                       
                                                                     
                                                              
       
				wbp->cl_clusters[cl_index].e_addr = wbp->cl_clusters[cl_index].b_addr + max_cluster_pgcount;

				cl->b_addr = wbp->cl_clusters[cl_index].e_addr;
			}
			  
                                                              
                                                                    
                                     
     
                                                               
                        
      
		} else {
			  
                                                                                  
      
			if ((wbp->cl_clusters[cl_index].e_addr - cl->b_addr) <= max_cluster_pgcount) {
				  
                                             
                                             
                                           
                                           
       
				wbp->cl_clusters[cl_index].b_addr = cl->b_addr;

				if (cl->e_addr > wbp->cl_clusters[cl_index].e_addr) {
					  
                                    
                                               
                                                                  
                                                                
                                      
        
					wbp->cl_clusters[cl_index].e_addr = cl->e_addr;
				}
				break;
			}
			  
                                                               
                                                    
                                                          
                                                               
                                                              
                                                
     
      
			if (cl->e_addr > wbp->cl_clusters[cl_index].e_addr - max_cluster_pgcount) {
				  
                                                          
                                                                      
                                         
       
				wbp->cl_clusters[cl_index].b_addr = wbp->cl_clusters[cl_index].e_addr - max_cluster_pgcount;

				cl->e_addr = wbp->cl_clusters[cl_index].b_addr;
			}
			  
                                               
                                                 
                                             
                          
                                                                  
      
		}
	}
	if (cl_index < wbp->cl_number) {
		  
                                            
                                       
     
		goto delay_io;
	}

	if (defer_writes == FALSE &&
	    wbp->cl_number == MAX_CLUSTERS &&
	    wbp->cl_seq_written >= (MAX_CLUSTERS * (max_cluster_pgcount * PAGE_SIZE))) {
		uint32_t        n;

		if (vp->v_mount->mnt_minsaturationbytecount) {
			n = vp->v_mount->mnt_minsaturationbytecount / MAX_CLUSTER_SIZE(vp);

			if (n > MAX_CLUSTERS) {
				n = MAX_CLUSTERS;
			}
		} else {
			n = 0;
		}

		if (n == 0) {
			if (disk_conditioner_mount_is_ssd(vp->v_mount)) {
				n = WRITE_BEHIND_SSD;
			} else {
				n = WRITE_BEHIND;
			}
		}
		while (n--) {
			cluster_try_push(wbp, vp, newEOF, 0, 0, callback, callback_arg, NULL, vm_initiated);
		}
	}
	if (wbp->cl_number < MAX_CLUSTERS) {
		  
                                          
                                          
              
     
		goto start_new_cluster;
	}
	  
                                            
                                        
                                                  
                                            
                                   
                                             
                                       
                                                   
    
	ret_cluster_try_push = 0;

	  
                                                             
    
	if (defer_writes == FALSE) {
		ret_cluster_try_push = cluster_try_push(wbp, vp, newEOF, (flags & IO_NOCACHE) ? 0 : PUSH_DELAY, 0, callback, callback_arg, NULL, vm_initiated);
	}
	  
                                                                
    
	if (ret_cluster_try_push == 0) {
		  
                                                 
                                                        
                         
     
		sparse_cluster_switch(wbp, vp, newEOF, callback, callback_arg, vm_initiated);
		sparse_cluster_add(wbp, &(wbp->cl_scmap), vp, cl, newEOF, callback, callback_arg, vm_initiated);

		lck_mtx_unlock(&wbp->cl_lockw);
		return;
	}
start_new_cluster:
	wbp->cl_clusters[wbp->cl_number].b_addr = cl->b_addr;
	wbp->cl_clusters[wbp->cl_number].e_addr = cl->e_addr;

	wbp->cl_clusters[wbp->cl_number].io_flags = 0;

	if (flags & IO_NOCACHE) {
		wbp->cl_clusters[wbp->cl_number].io_flags |= CLW_IONOCACHE;
	}

	if (flags & IO_PASSIVE) {
		wbp->cl_clusters[wbp->cl_number].io_flags |= CLW_IOPASSIVE;
	}

	wbp->cl_number++;
delay_io:
	lck_mtx_unlock(&wbp->cl_lockw);
	return;
}


static int
cluster_write_copy(vnode_t vp, struct uio *uio, u_int32_t io_req_size, off_t oldEOF, off_t newEOF, off_t headOff,
    off_t tailOff, int flags, int (*callback)(buf_t, void *), void *callback_arg)
{
	upl_page_info_t *pl;
	upl_t            upl;
	vm_offset_t      upl_offset = 0;
	vm_size_t        upl_size;
	off_t            upl_f_offset;
	int              pages_in_upl;
	int              start_offset;
	int              xfer_resid;
	int              io_size;
	int              io_offset;
	int              bytes_to_zero;
	int              bytes_to_move;
	kern_return_t    kret;
	int              retval = 0;
	int              io_resid;
	long long        total_size;
	long long        zero_cnt;
	off_t            zero_off;
	long long        zero_cnt1;
	off_t            zero_off1;
	off_t            write_off = 0;
	int              write_cnt = 0;
	boolean_t        first_pass = FALSE;
	struct cl_extent cl;
	int              bflag;
	u_int            max_io_size;

	if (uio) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 40)) | DBG_FUNC_START,
		    (int)uio->uio_offset, io_req_size, (int)oldEOF, (int)newEOF, 0);

		io_resid = io_req_size;
	} else {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 40)) | DBG_FUNC_START,
		    0, 0, (int)oldEOF, (int)newEOF, 0);

		io_resid = 0;
	}
	if (flags & IO_PASSIVE) {
		bflag = CL_PASSIVE;
	} else {
		bflag = 0;
	}
	if (flags & IO_NOCACHE) {
		bflag |= CL_NOCACHE;
	}

	if (flags & IO_SKIP_ENCRYPTION) {
		bflag |= CL_ENCRYPTED;
	}

	zero_cnt  = 0;
	zero_cnt1 = 0;
	zero_off  = 0;
	zero_off1 = 0;

	max_io_size = cluster_max_io_size(vp->v_mount, CL_WRITE);

	if (flags & IO_HEADZEROFILL) {
		  
                                                                                   
                                                                             
                                                                              
                                                                                 
                                       
     
		if (uio) {
			if (headOff < uio->uio_offset) {
				zero_cnt = uio->uio_offset - headOff;
				zero_off = headOff;
			}
		} else if (headOff < newEOF) {
			zero_cnt = newEOF - headOff;
			zero_off = headOff;
		}
	} else {
		if (uio && uio->uio_offset > oldEOF) {
			zero_off = uio->uio_offset & ~PAGE_MASK_64;

			if (zero_off >= oldEOF) {
				zero_cnt = uio->uio_offset - zero_off;

				flags |= IO_HEADZEROFILL;
			}
		}
	}
	if (flags & IO_TAILZEROFILL) {
		if (uio) {
			zero_off1 = uio->uio_offset + io_req_size;

			if (zero_off1 < tailOff) {
				zero_cnt1 = tailOff - zero_off1;
			}
		}
	} else {
		if (uio && newEOF > oldEOF) {
			zero_off1 = uio->uio_offset + io_req_size;

			if (zero_off1 == newEOF && (zero_off1 & PAGE_MASK_64)) {
				zero_cnt1 = PAGE_SIZE_64 - (zero_off1 & PAGE_MASK_64);

				flags |= IO_TAILZEROFILL;
			}
		}
	}
	if (zero_cnt == 0 && uio == (struct uio *) 0) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 40)) | DBG_FUNC_END,
		    retval, 0, 0, 0, 0);
		return 0;
	}
	if (uio) {
		write_off = uio->uio_offset;
		write_cnt = (int)uio_resid(uio);
		  
                                             
                                              
                    
     
		first_pass = TRUE;
	}
	while ((total_size = (io_resid + zero_cnt + zero_cnt1)) && retval == 0) {
		  
                                                                           
     
		if (zero_cnt) {
			start_offset = (int)(zero_off & PAGE_MASK_64);
			upl_f_offset = zero_off - start_offset;
		} else if (io_resid) {
			start_offset = (int)(uio->uio_offset & PAGE_MASK_64);
			upl_f_offset = uio->uio_offset - start_offset;
		} else {
			start_offset = (int)(zero_off1 & PAGE_MASK_64);
			upl_f_offset = zero_off1 - start_offset;
		}
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 46)) | DBG_FUNC_NONE,
		    (int)zero_off, (int)zero_cnt, (int)zero_off1, (int)zero_cnt1, 0);

		if (total_size > max_io_size) {
			total_size = max_io_size;
		}

		cl.b_addr = (daddr64_t)(upl_f_offset / PAGE_SIZE_64);

		if (uio && ((flags & (IO_SYNC | IO_HEADZEROFILL | IO_TAILZEROFILL)) == 0)) {
			  
                                          
                                                         
      
			if ((start_offset + total_size) > max_io_size) {
				total_size = max_io_size - start_offset;
			}
			xfer_resid = (int)total_size;

			retval = cluster_copy_ubc_data_internal(vp, uio, &xfer_resid, 1, 1);

			if (retval) {
				break;
			}

			io_resid    -= (total_size - xfer_resid);
			total_size   = xfer_resid;
			start_offset = (int)(uio->uio_offset & PAGE_MASK_64);
			upl_f_offset = uio->uio_offset - start_offset;

			if (total_size == 0) {
				if (start_offset) {
					  
                                                   
                                                     
                                                     
                                                  
                                                      
                        
        
					upl_f_offset += PAGE_SIZE_64;
				}
				upl_size = 0;

				goto check_cluster;
			}
		}
		  
                                                    
                                                         
                                                       
                                                     
                             
                                              
     
		upl_size = (start_offset + total_size + (PAGE_SIZE - 1)) & ~PAGE_MASK;

		if (upl_size > max_io_size) {
			upl_size = max_io_size;
		}

		pages_in_upl = (int)(upl_size / PAGE_SIZE);
		io_size      = (int)(upl_size - start_offset);

		if ((long long)io_size > total_size) {
			io_size = (int)total_size;
		}

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 41)) | DBG_FUNC_START, upl_size, io_size, total_size, 0, 0);


		  
                                            
                                                         
                                          
     
		kret = ubc_create_upl_kernel(vp,
		    upl_f_offset,
		    (int)upl_size,
		    &upl,
		    &pl,
		    UPL_SET_LITE | ((uio != NULL && (uio->uio_flags & UIO_FLAGS_IS_COMPRESSED_FILE)) ? 0 : UPL_WILL_MODIFY),
		    VM_KERN_MEMORY_FILE);
		if (kret != KERN_SUCCESS) {
			panic("cluster_write_copy: failed to get pagelist");
		}

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 41)) | DBG_FUNC_END,
		    upl, (int)upl_f_offset, start_offset, 0, 0);

		if (start_offset && upl_f_offset < oldEOF && !upl_valid_page(pl, 0)) {
			int   read_size;

			  
                                                               
                                                                
                                                            
      
			read_size = PAGE_SIZE;

			if ((upl_f_offset + read_size) > oldEOF) {
				read_size = (int)(oldEOF - upl_f_offset);
			}

			retval = cluster_io(vp, upl, 0, upl_f_offset, read_size,
			    CL_READ | bflag, (buf_t)NULL, (struct clios *)NULL, callback, callback_arg);
			if (retval) {
				  
                                                               
                                                                 
                                                                    
                                                    
       
				ubc_upl_abort_range(upl, 0, PAGE_SIZE, UPL_ABORT_DUMP_PAGES | UPL_ABORT_FREE_ON_EMPTY);

				if (upl_size > PAGE_SIZE) {
					ubc_upl_abort_range(upl, 0, (upl_size_t)upl_size,
					    UPL_ABORT_FREE_ON_EMPTY);
				}

				KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 45)) | DBG_FUNC_NONE,
				    upl, 0, 0, retval, 0);
				break;
			}
		}
		if ((start_offset == 0 || upl_size > PAGE_SIZE) && ((start_offset + io_size) & PAGE_MASK)) {
			  
                                                                         
                                                                         
                                                     
      
			upl_offset = upl_size - PAGE_SIZE;

			if ((upl_f_offset + start_offset + io_size) < oldEOF &&
			    !upl_valid_page(pl, (int)(upl_offset / PAGE_SIZE))) {
				int   read_size;

				read_size = PAGE_SIZE;

				if ((off_t)(upl_f_offset + upl_offset + read_size) > oldEOF) {
					read_size = (int)(oldEOF - (upl_f_offset + upl_offset));
				}

				retval = cluster_io(vp, upl, upl_offset, upl_f_offset + upl_offset, read_size,
				    CL_READ | bflag, (buf_t)NULL, (struct clios *)NULL, callback, callback_arg);
				if (retval) {
					  
                                                                
                                                             
                                                                
                                                               
        
					ubc_upl_abort_range(upl, (upl_offset_t)upl_offset, PAGE_SIZE, UPL_ABORT_DUMP_PAGES | UPL_ABORT_FREE_ON_EMPTY);

					if (upl_size > PAGE_SIZE) {
						ubc_upl_abort_range(upl, 0, (upl_size_t)upl_size, UPL_ABORT_FREE_ON_EMPTY);
					}

					KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 45)) | DBG_FUNC_NONE,
					    upl, 0, 0, retval, 0);
					break;
				}
			}
		}
		xfer_resid = io_size;
		io_offset = start_offset;

		while (zero_cnt && xfer_resid) {
			if (zero_cnt < (long long)xfer_resid) {
				bytes_to_zero = (int)zero_cnt;
			} else {
				bytes_to_zero = xfer_resid;
			}

			bytes_to_zero = cluster_zero_range(upl, pl, flags, io_offset, zero_off, upl_f_offset, bytes_to_zero);

			xfer_resid -= bytes_to_zero;
			zero_cnt   -= bytes_to_zero;
			zero_off   += bytes_to_zero;
			io_offset  += bytes_to_zero;
		}
		if (xfer_resid && io_resid) {
			u_int32_t  io_requested;

			bytes_to_move = min(io_resid, xfer_resid);
			io_requested = bytes_to_move;

			retval = cluster_copy_upl_data(uio, upl, io_offset, (int *)&io_requested);

			if (retval) {
				ubc_upl_abort_range(upl, 0, (upl_size_t)upl_size, UPL_ABORT_FREE_ON_EMPTY);

				KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 45)) | DBG_FUNC_NONE,
				    upl, 0, 0, retval, 0);
			} else {
				io_resid   -= bytes_to_move;
				xfer_resid -= bytes_to_move;
				io_offset  += bytes_to_move;
			}
		}
		while (xfer_resid && zero_cnt1 && retval == 0) {
			if (zero_cnt1 < (long long)xfer_resid) {
				bytes_to_zero = (int)zero_cnt1;
			} else {
				bytes_to_zero = xfer_resid;
			}

			bytes_to_zero = cluster_zero_range(upl, pl, flags, io_offset, zero_off1, upl_f_offset, bytes_to_zero);

			xfer_resid -= bytes_to_zero;
			zero_cnt1  -= bytes_to_zero;
			zero_off1  += bytes_to_zero;
			io_offset  += bytes_to_zero;
		}
		if (retval == 0) {
			int do_zeroing = 1;

			io_size += start_offset;

			                                                          
			if ((vnode_tag(vp) == VT_APFS) && (newEOF < oldEOF)) {
				do_zeroing = 0;
			}

			if (do_zeroing && (upl_f_offset + io_size) >= newEOF && (u_int)io_size < upl_size) {
				  
                                                  
                                                   
                                                                  
                                                                       
       
				cluster_zero(upl, io_size, (int)(upl_size - io_size), NULL);
			}
			  
                                                 
                                                             
                                                                
                                                                           
                                                     
                                                                              
                                                                       
                          
                                                                                      
                                                                                             
                                                                                       
                                                                                                   
                                                                                  
                                                                                                   
                                                                                                      
                                                                        
      
			ubc_upl_commit_range(upl, 0, (upl_size_t)upl_size,
			    UPL_COMMIT_SET_DIRTY | UPL_COMMIT_INACTIVATE | UPL_COMMIT_FREE_ON_EMPTY);
check_cluster:
			  
                                             
                                       
      
			cl.e_addr = (daddr64_t)((upl_f_offset + (off_t)upl_size) / PAGE_SIZE_64);

			if (flags & IO_SYNC) {
				  
                                                        
                                                   
      
                                           
      
                                                                                      
                                                                                     
                                                                                     
                                                                                             
                                                          
       
				retval = cluster_push_now(vp, &cl, newEOF, flags, callback, callback_arg, FALSE);
			} else {
				boolean_t defer_writes = FALSE;

				if (vfs_flags(vp->v_mount) & MNT_DEFWRITE) {
					defer_writes = TRUE;
				}

				cluster_update_state_internal(vp, &cl, flags, defer_writes, &first_pass,
				    write_off, write_cnt, newEOF, callback, callback_arg, FALSE);
			}
		}
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 40)) | DBG_FUNC_END, retval, 0, io_resid, 0, 0);

	return retval;
}



int
cluster_read(vnode_t vp, struct uio *uio, off_t filesize, int xflags)
{
	return cluster_read_ext(vp, uio, filesize, xflags, NULL, NULL);
}


int
cluster_read_ext(vnode_t vp, struct uio *uio, off_t filesize, int xflags, int (*callback)(buf_t, void *), void *callback_arg)
{
	int             retval = 0;
	int             flags;
	user_ssize_t    cur_resid;
	u_int32_t       io_size;
	u_int32_t       read_length = 0;
	int             read_type = IO_COPY;

	flags = xflags;

	if (vp->v_flag & VNOCACHE_DATA) {
		flags |= IO_NOCACHE;
	}
	if ((vp->v_flag & VRAOFF) || speculative_reads_disabled) {
		flags |= IO_RAOFF;
	}

	if (flags & IO_SKIP_ENCRYPTION) {
		flags |= IO_ENCRYPTED;
	}

	  
                                                                   
                         
                                              
                                                                                    
                                                                                        
            
   
                                                                   
                                       
    
	if (((flags & IO_NOCACHE) && UIO_SEG_IS_USER_SPACE(uio->uio_segflg)) || (flags & IO_ENCRYPTED)) {
		retval = cluster_io_type(uio, &read_type, &read_length, 0);
	}

	while ((cur_resid = uio_resid(uio)) && uio->uio_offset < filesize && retval == 0) {
		switch (read_type) {
		case IO_COPY:
			  
                                              
                                                     
                                          
      
			if (cur_resid > (user_ssize_t)(MAX_IO_REQUEST_SIZE)) {
				io_size = MAX_IO_REQUEST_SIZE;
			} else {
				io_size = (u_int32_t)cur_resid;
			}

			retval = cluster_read_copy(vp, uio, io_size, filesize, flags, callback, callback_arg);
			break;

		case IO_DIRECT:
			retval = cluster_read_direct(vp, uio, filesize, &read_type, &read_length, flags, callback, callback_arg);
			break;

		case IO_CONTIG:
			retval = cluster_read_contig(vp, uio, filesize, &read_type, &read_length, callback, callback_arg, flags);
			break;

		case IO_UNKNOWN:
			retval = cluster_io_type(uio, &read_type, &read_length, 0);
			break;
		}
	}
	return retval;
}



static void
cluster_read_upl_release(upl_t upl, int start_pg, int last_pg, int take_reference)
{
	int range;
	int abort_flags = UPL_ABORT_FREE_ON_EMPTY;

	if ((range = last_pg - start_pg)) {
		if (take_reference) {
			abort_flags |= UPL_ABORT_REFERENCE;
		}

		ubc_upl_abort_range(upl, start_pg * PAGE_SIZE, range * PAGE_SIZE, abort_flags);
	}
}


static int
cluster_read_copy(vnode_t vp, struct uio *uio, u_int32_t io_req_size, off_t filesize, int flags, int (*callback)(buf_t, void *), void *callback_arg)
{
	upl_page_info_t *pl;
	upl_t            upl;
	vm_offset_t      upl_offset;
	u_int32_t        upl_size;
	off_t            upl_f_offset;
	int              start_offset;
	int              start_pg;
	int              last_pg;
	int              uio_last = 0;
	int              pages_in_upl;
	off_t            max_size;
	off_t            last_ioread_offset;
	off_t            last_request_offset;
	kern_return_t    kret;
	int              error  = 0;
	int              retval = 0;
	u_int32_t        size_of_prefetch;
	u_int32_t        xsize;
	u_int32_t        io_size;
	u_int32_t        max_rd_size;
	u_int32_t        max_io_size;
	u_int32_t        max_prefetch;
	u_int            rd_ahead_enabled = 1;
	u_int            prefetch_enabled = 1;
	struct cl_readahead *   rap;
	struct clios            iostate;
	struct cl_extent        extent;
	int              bflag;
	int              take_reference = 1;
	int              policy = IOPOL_DEFAULT;
	boolean_t        iolock_inited = FALSE;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 32)) | DBG_FUNC_START,
	    (int)uio->uio_offset, io_req_size, (int)filesize, flags, 0);

	if (flags & IO_ENCRYPTED) {
		panic("encrypted blocks will hit UBC!");
	}

	policy = throttle_get_io_policy(NULL);

	if (policy == THROTTLE_LEVEL_TIER3 || policy == THROTTLE_LEVEL_TIER2 || (flags & IO_NOCACHE)) {
		take_reference = 0;
	}

	if (flags & IO_PASSIVE) {
		bflag = CL_PASSIVE;
	} else {
		bflag = 0;
	}

	if (flags & IO_NOCACHE) {
		bflag |= CL_NOCACHE;
	}

	if (flags & IO_SKIP_ENCRYPTION) {
		bflag |= CL_ENCRYPTED;
	}

	max_io_size = cluster_max_io_size(vp->v_mount, CL_READ);
	max_prefetch = MAX_PREFETCH(vp, max_io_size, disk_conditioner_mount_is_ssd(vp->v_mount));
	max_rd_size = max_prefetch;

	last_request_offset = uio->uio_offset + io_req_size;

	if (last_request_offset > filesize) {
		last_request_offset = filesize;
	}

	if ((flags & (IO_RAOFF | IO_NOCACHE)) || ((last_request_offset & ~PAGE_MASK_64) == (uio->uio_offset & ~PAGE_MASK_64))) {
		rd_ahead_enabled = 0;
		rap = NULL;
	} else {
		if (cluster_is_throttled(vp)) {
			  
                                                     
                                                      
              
      
			rd_ahead_enabled = 0;
			prefetch_enabled = 0;

			max_rd_size = THROTTLE_MAX_IOSIZE;
		}
		if ((rap = cluster_get_rap(vp)) == NULL) {
			rd_ahead_enabled = 0;
		} else {
			extent.b_addr = uio->uio_offset / PAGE_SIZE_64;
			extent.e_addr = (last_request_offset - 1) / PAGE_SIZE_64;
		}
	}
	if (rap != NULL && rap->cl_ralen && (rap->cl_lastr == extent.b_addr || (rap->cl_lastr + 1) == extent.b_addr)) {
		  
                                                                          
                                            
                                                                  
                                                              
                                                                
     
		last_ioread_offset = (rap->cl_maxra * PAGE_SIZE_64) + PAGE_SIZE_64;

		if (last_ioread_offset < uio->uio_offset) {
			last_ioread_offset = (off_t)0;
		} else if (last_ioread_offset > last_request_offset) {
			last_ioread_offset = last_request_offset;
		}
	} else {
		last_ioread_offset = (off_t)0;
	}

	while (io_req_size && uio->uio_offset < filesize && retval == 0) {
		max_size = filesize - uio->uio_offset;
		bool leftover_upl_aborted = false;

		if ((off_t)(io_req_size) < max_size) {
			io_size = io_req_size;
		} else {
			io_size = (u_int32_t)max_size;
		}

		if (!(flags & IO_NOCACHE)) {
			while (io_size) {
				u_int32_t io_resid;
				u_int32_t io_requested;

				  
                                                                      
                                                                           
                                                                         
                                                                            
                                                                                     
       
				if (last_request_offset && last_ioread_offset && (size_of_prefetch = (u_int32_t)(last_request_offset - last_ioread_offset))) {
					if ((last_ioread_offset - uio->uio_offset) <= max_rd_size && prefetch_enabled) {
						  
                                                      
                                     
                                                       
                                                      
                                     
         
						if (size_of_prefetch > max_rd_size) {
							size_of_prefetch = max_rd_size;
						}

						size_of_prefetch = cluster_read_prefetch(vp, last_ioread_offset, size_of_prefetch, filesize, callback, callback_arg, bflag);

						last_ioread_offset += (off_t)(size_of_prefetch * PAGE_SIZE);

						if (last_ioread_offset > last_request_offset) {
							last_ioread_offset = last_request_offset;
						}
					}
				}
				  
                                                           
                                                         
                                                    
       
				if (last_ioread_offset && io_size > (max_io_size / 4)) {
					io_resid = (max_io_size / 4);
				} else {
					io_resid = io_size;
				}

				io_requested = io_resid;

				retval = cluster_copy_ubc_data_internal(vp, uio, (int *)&io_resid, 0, take_reference);

				xsize = io_requested - io_resid;

				io_size -= xsize;
				io_req_size -= xsize;

				if (retval || io_resid) {
					  
                                      
                                       
                                       
        
					break;
				}

				if (rd_ahead_enabled && (io_size == 0 || last_ioread_offset == last_request_offset)) {
					  
                                                            
                                              
        
					cluster_read_ahead(vp, &extent, filesize, rap, callback, callback_arg, bflag);
				}
			}
			if (retval) {
				break;
			}
			if (io_size == 0) {
				if (rap != NULL) {
					if (extent.e_addr < rap->cl_lastr) {
						rap->cl_maxra = 0;
					}
					rap->cl_lastr = extent.e_addr;
				}
				break;
			}
			  
                                                             
                                       
      
			max_size = filesize - uio->uio_offset;
		}

		iostate.io_completed = 0;
		iostate.io_issued = 0;
		iostate.io_error = 0;
		iostate.io_wanted = 0;

		if ((flags & IO_RETURN_ON_THROTTLE)) {
			if (cluster_is_throttled(vp) == THROTTLE_NOW) {
				if (!cluster_io_present_in_BC(vp, uio->uio_offset)) {
					  
                                                       
                                                        
                                                         
                                                       
                                                    
        
					throttle_info_update_by_mount(vp->v_mount);

					retval = EAGAIN;
					break;
				}
			}
		}

		  
                                                    
                                                        
                                                       
                                                     
                             
                                              
     
		start_offset = (int)(uio->uio_offset & PAGE_MASK_64);
		upl_f_offset = uio->uio_offset - (off_t)start_offset;

		if (io_size > max_rd_size) {
			io_size = max_rd_size;
		}

		upl_size = (start_offset + io_size + (PAGE_SIZE - 1)) & ~PAGE_MASK;

		if (flags & IO_NOCACHE) {
			if (upl_size > max_io_size) {
				upl_size = max_io_size;
			}
		} else {
			if (upl_size > max_io_size / 4) {
				upl_size = max_io_size / 4;
				upl_size &= ~PAGE_MASK;

				if (upl_size == 0) {
					upl_size = PAGE_SIZE;
				}
			}
		}
		pages_in_upl = upl_size / PAGE_SIZE;

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 33)) | DBG_FUNC_START,
		    upl, (int)upl_f_offset, upl_size, start_offset, 0);

		kret = ubc_create_upl_kernel(vp,
		    upl_f_offset,
		    upl_size,
		    &upl,
		    &pl,
		    UPL_FILE_IO | UPL_SET_LITE,
		    VM_KERN_MEMORY_FILE);
		if (kret != KERN_SUCCESS) {
			panic("cluster_read_copy: failed to get pagelist");
		}

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 33)) | DBG_FUNC_END,
		    upl, (int)upl_f_offset, upl_size, start_offset, 0);

		  
                                                             
                                                          
                                                              
                                                                  
     
		for (start_pg = 0; start_pg < pages_in_upl; start_pg++) {
			if (!upl_valid_page(pl, start_pg)) {
				break;
			}
		}

		  
                                                            
                                                     
                                                              
                 
     
		for (last_pg = start_pg; last_pg < pages_in_upl; last_pg++) {
			if (upl_valid_page(pl, last_pg)) {
				break;
			}
		}

		if (start_pg < last_pg) {
			  
                                                             
                                                                 
                                                                  
                                                                 
      
			if (iolock_inited == FALSE) {
				lck_mtx_init(&iostate.io_mtxp, &cl_mtx_grp, LCK_ATTR_NULL);

				iolock_inited = TRUE;
			}
			upl_offset = start_pg * PAGE_SIZE;
			io_size    = (last_pg - start_pg) * PAGE_SIZE;

			if ((off_t)(upl_f_offset + upl_offset + io_size) > filesize) {
				io_size = (u_int32_t)(filesize - (upl_f_offset + upl_offset));
			}

			  
                                                                       
                                                                
                                                                          
                                      
      
			size_t verify_block_size = 0;
			if ((VNOP_VERIFY(vp, start_offset, NULL, 0, &verify_block_size, VNODE_VERIFY_DEFAULT, NULL) == 0)                           ) {
				for (uio_last = last_pg; uio_last < pages_in_upl; uio_last++) {
					if (!upl_valid_page(pl, uio_last)) {
						break;
					}
				}
				if (uio_last < pages_in_upl) {
					  
                                                            
                                                          
                                                        
                    
        
					ubc_upl_abort_range(upl, uio_last * PAGE_SIZE,
					    (pages_in_upl - uio_last) * PAGE_SIZE, UPL_ABORT_FREE_ON_EMPTY);
					leftover_upl_aborted = true;
				}
			}

			  
                                              
      

			error = cluster_io(vp, upl, upl_offset, upl_f_offset + upl_offset,
			    io_size, CL_READ | CL_ASYNC | bflag, (buf_t)NULL, &iostate, callback, callback_arg);

			if (rap) {
				if (extent.e_addr < rap->cl_maxra) {
					  
                                                                  
                                                                   
                                                                 
                                                         
        
					rap->cl_maxra = 0;
				}
			}
		}
		if (error == 0) {
			  
                                                                     
                                                                           
                                    
                                                             
      
			u_int  val_size;

			if (!leftover_upl_aborted) {
				for (uio_last = last_pg; uio_last < pages_in_upl; uio_last++) {
					if (!upl_valid_page(pl, uio_last)) {
						break;
					}
				}
				if (uio_last < pages_in_upl) {
					  
                                                            
                                                          
                                                        
                    
        
					ubc_upl_abort_range(upl, uio_last * PAGE_SIZE,
					    (pages_in_upl - uio_last) * PAGE_SIZE, UPL_ABORT_FREE_ON_EMPTY);
				}
			}

			  
                                                             
                                                              
                             
      
			val_size = (uio_last * PAGE_SIZE) - start_offset;

			if (val_size > max_size) {
				val_size = (u_int)max_size;
			}

			if (val_size > io_req_size) {
				val_size = io_req_size;
			}

			if ((uio->uio_offset + val_size) > last_ioread_offset) {
				last_ioread_offset = uio->uio_offset + val_size;
			}

			if ((size_of_prefetch = (u_int32_t)(last_request_offset - last_ioread_offset)) && prefetch_enabled) {
				if ((last_ioread_offset - (uio->uio_offset + val_size)) <= upl_size) {
					  
                                                                
                                               
                                                                   
                                                         
                                    
        
					if (size_of_prefetch > max_rd_size) {
						size_of_prefetch = max_rd_size;
					}

					size_of_prefetch = cluster_read_prefetch(vp, last_ioread_offset, size_of_prefetch, filesize, callback, callback_arg, bflag);

					last_ioread_offset += (off_t)(size_of_prefetch * PAGE_SIZE);

					if (last_ioread_offset > last_request_offset) {
						last_ioread_offset = last_request_offset;
					}
				}
			} else if ((uio->uio_offset + val_size) == last_request_offset) {
				  
                                                    
                                          
                                                 
                             
       
				if (rd_ahead_enabled) {
					cluster_read_ahead(vp, &extent, filesize, rap, callback, callback_arg, bflag);
				}

				if (rap != NULL) {
					if (extent.e_addr < rap->cl_lastr) {
						rap->cl_maxra = 0;
					}
					rap->cl_lastr = extent.e_addr;
				}
			}
			if (iolock_inited == TRUE) {
				cluster_iostate_wait(&iostate, 0, "cluster_read_copy");
			}

			if (iostate.io_error) {
				error = iostate.io_error;
			} else {
				u_int32_t io_requested;

				io_requested = val_size;

				retval = cluster_copy_upl_data(uio, upl, start_offset, (int *)&io_requested);

				io_req_size -= (val_size - io_requested);
			}
		} else {
			if (iolock_inited == TRUE) {
				cluster_iostate_wait(&iostate, 0, "cluster_read_copy");
			}
		}
		if (start_pg < last_pg) {
			  
                                                                   
                                                          
                                                              
                            
      
			io_size = (last_pg - start_pg) * PAGE_SIZE;

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 35)) | DBG_FUNC_START, upl, start_pg * PAGE_SIZE, io_size, error, 0);

			if (error || (flags & IO_NOCACHE)) {
				ubc_upl_abort_range(upl, start_pg * PAGE_SIZE, io_size,
				    UPL_ABORT_DUMP_PAGES | UPL_ABORT_FREE_ON_EMPTY);
			} else {
				int     commit_flags = UPL_COMMIT_CLEAR_DIRTY | UPL_COMMIT_FREE_ON_EMPTY;

				if (take_reference) {
					commit_flags |= UPL_COMMIT_INACTIVATE;
				} else {
					commit_flags |= UPL_COMMIT_SPECULATE;
				}

				ubc_upl_commit_range(upl, start_pg * PAGE_SIZE, io_size, commit_flags);
			}
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 35)) | DBG_FUNC_END, upl, start_pg * PAGE_SIZE, io_size, error, 0);
		}
		if ((last_pg - start_pg) < pages_in_upl) {
			  
                                                                  
                                                               
                 
      
			if (error) {
				if (leftover_upl_aborted) {
					ubc_upl_abort_range(upl, start_pg * PAGE_SIZE, (uio_last - start_pg) * PAGE_SIZE,
					    UPL_ABORT_FREE_ON_EMPTY);
				} else {
					ubc_upl_abort_range(upl, 0, upl_size, UPL_ABORT_FREE_ON_EMPTY);
				}
			} else {
				KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 35)) | DBG_FUNC_START,
				    upl, -1, pages_in_upl - (last_pg - start_pg), 0, 0);

				  
                                                 
                                             
       
				cluster_read_upl_release(upl, 0, start_pg, take_reference);

				  
                                                   
                                                                 
       
				cluster_read_upl_release(upl, last_pg, uio_last, take_reference);

				KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 35)) | DBG_FUNC_END, upl, -1, -1, 0, 0);
			}
		}
		if (retval == 0) {
			retval = error;
		}

		if (io_req_size) {
			if (cluster_is_throttled(vp)) {
				  
                                                      
                                                       
               
       
				rd_ahead_enabled = 0;
				prefetch_enabled = 0;
				max_rd_size = THROTTLE_MAX_IOSIZE;
			} else {
				if (max_rd_size == THROTTLE_MAX_IOSIZE) {
					  
                                     
        
					if (policy != THROTTLE_LEVEL_TIER3 && policy != THROTTLE_LEVEL_TIER2) {
						if (rap != NULL) {
							rd_ahead_enabled = 1;
						}
						prefetch_enabled = 1;
					}
					max_rd_size = max_prefetch;
					last_ioread_offset = 0;
				}
			}
		}
	}
	if (iolock_inited == TRUE) {
		  
                                          
                                          
                                            
                                        
                                              
                                                   
     
		cluster_iostate_wait(&iostate, 0, "cluster_read_copy");

		lck_mtx_destroy(&iostate.io_mtxp, &cl_mtx_grp);
	}
	if (rap != NULL) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 32)) | DBG_FUNC_END,
		    (int)uio->uio_offset, io_req_size, rap->cl_lastr, retval, 0);

		lck_mtx_unlock(&rap->cl_lockr);
	} else {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 32)) | DBG_FUNC_END,
		    (int)uio->uio_offset, io_req_size, 0, retval, 0);
	}

	return retval;
}

  
                                                                      
                                                                      
                                     
   
cl_direct_read_lock_t *
cluster_lock_direct_read(vnode_t vp, lck_rw_type_t type)
{
	struct cl_direct_read_locks *head
	        = &cl_direct_read_locks[(uintptr_t)vp / sizeof(*vp)
	    % CL_DIRECT_READ_LOCK_BUCKETS];

	struct cl_direct_read_lock *lck, *new_lck = NULL;

	for (;;) {
		lck_spin_lock(&cl_direct_read_spin_lock);

		LIST_FOREACH(lck, head, chain) {
			if (lck->vp == vp) {
				++lck->ref_count;
				lck_spin_unlock(&cl_direct_read_spin_lock);
				if (new_lck) {
					                                              
					lck_rw_destroy(&new_lck->rw_lock, &cl_mtx_grp);
					kheap_free(KHEAP_DEFAULT, new_lck, sizeof(cl_direct_read_lock_t));
				}
				lck_rw_lock(&lck->rw_lock, type);
				return lck;
			}
		}

		if (new_lck) {
			                            
			LIST_INSERT_HEAD(head, new_lck, chain);
			lck_spin_unlock(&cl_direct_read_spin_lock);
			lck_rw_lock(&new_lck->rw_lock, type);
			return new_lck;
		}

		lck_spin_unlock(&cl_direct_read_spin_lock);

		                      
		new_lck = kheap_alloc(KHEAP_DEFAULT, sizeof(cl_direct_read_lock_t),
		    Z_WAITOK);
		lck_rw_init(&new_lck->rw_lock, &cl_mtx_grp, LCK_ATTR_NULL);
		new_lck->vp = vp;
		new_lck->ref_count = 1;

		                        
	}
}

void
cluster_unlock_direct_read(cl_direct_read_lock_t *lck)
{
	lck_rw_done(&lck->rw_lock);

	lck_spin_lock(&cl_direct_read_spin_lock);
	if (lck->ref_count == 1) {
		LIST_REMOVE(lck, chain);
		lck_spin_unlock(&cl_direct_read_spin_lock);
		lck_rw_destroy(&lck->rw_lock, &cl_mtx_grp);
		kheap_free(KHEAP_DEFAULT, lck, sizeof(cl_direct_read_lock_t));
	} else {
		--lck->ref_count;
		lck_spin_unlock(&cl_direct_read_spin_lock);
	}
}

static int
cluster_read_direct(vnode_t vp, struct uio *uio, off_t filesize, int *read_type, u_int32_t *read_length,
    int flags, int (*callback)(buf_t, void *), void *callback_arg)
{
	upl_t            upl;
	upl_page_info_t  *pl;
	off_t            max_io_size;
	vm_offset_t      upl_offset, vector_upl_offset = 0;
	upl_size_t       upl_size, vector_upl_size = 0;
	vm_size_t        upl_needed_size;
	unsigned int     pages_in_pl;
	upl_control_flags_t upl_flags;
	kern_return_t    kret;
	unsigned int     i;
	int              force_data_sync;
	int              retval = 0;
	int              no_zero_fill = 0;
	int              io_flag = 0;
	int              misaligned = 0;
	struct clios     iostate;
	user_addr_t      iov_base;
	u_int32_t        io_req_size;
	u_int32_t        offset_in_file;
	u_int32_t        offset_in_iovbase;
	u_int32_t        io_size;
	u_int32_t        io_min;
	u_int32_t        xsize;
	u_int32_t        devblocksize;
	u_int32_t        mem_alignment_mask;
	u_int32_t        max_upl_size;
	u_int32_t        max_rd_size;
	u_int32_t        max_rd_ahead;
	u_int32_t        max_vector_size;
	boolean_t        io_throttled = FALSE;

	u_int32_t        vector_upl_iosize = 0;
	int              issueVectorUPL = 0, useVectorUPL = (uio->uio_iovcnt > 1);
	off_t            v_upl_uio_offset = 0;
	int              vector_upl_index = 0;
	upl_t            vector_upl = NULL;
	cl_direct_read_lock_t *lock = NULL;

	user_addr_t      orig_iov_base = 0;
	user_addr_t      last_iov_base = 0;
	user_addr_t      next_iov_base = 0;

	assert(vm_map_page_shift(current_map()) >= PAGE_SHIFT);

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 70)) | DBG_FUNC_START,
	    (int)uio->uio_offset, (int)filesize, *read_type, *read_length, 0);

	max_upl_size = cluster_max_io_size(vp->v_mount, CL_READ);

	max_rd_size = max_upl_size;
	max_rd_ahead = max_rd_size * IO_SCALE(vp, 2);

	io_flag = CL_COMMIT | CL_READ | CL_ASYNC | CL_NOZERO | CL_DIRECT_IO;

	if (flags & IO_PASSIVE) {
		io_flag |= CL_PASSIVE;
	}

	if (flags & IO_ENCRYPTED) {
		io_flag |= CL_RAW_ENCRYPTED;
	}

	if (flags & IO_NOCACHE) {
		io_flag |= CL_NOCACHE;
	}

	if (flags & IO_SKIP_ENCRYPTION) {
		io_flag |= CL_ENCRYPTED;
	}

	iostate.io_completed = 0;
	iostate.io_issued = 0;
	iostate.io_error = 0;
	iostate.io_wanted = 0;

	lck_mtx_init(&iostate.io_mtxp, &cl_mtx_grp, LCK_ATTR_NULL);

	devblocksize = (u_int32_t)vp->v_mount->mnt_devblocksize;
	mem_alignment_mask = (u_int32_t)vp->v_mount->mnt_alignmentmask;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 70)) | DBG_FUNC_NONE,
	    (int)devblocksize, (int)mem_alignment_mask, 0, 0, 0);

	if (devblocksize == 1) {
		  
                                                  
                                                   
                                         
                                                             
                                             
                                                        
                                                          
                       
     
		devblocksize = PAGE_SIZE;
	}

	orig_iov_base = uio_curriovbase(uio);
	last_iov_base = orig_iov_base;

next_dread:
	io_req_size = *read_length;
	iov_base = uio_curriovbase(uio);

	offset_in_file = (u_int32_t)uio->uio_offset & (devblocksize - 1);
	offset_in_iovbase = (u_int32_t)iov_base & mem_alignment_mask;

	if (vm_map_page_mask(current_map()) < PAGE_MASK) {
		  
               
                                                                  
                                                            
                                     
                                   
     
		if (!misaligned) {
			DEBUG4K_VFS("forcing misaligned\n");
		}
		misaligned = 1;
	}

	if (offset_in_file || offset_in_iovbase) {
		  
                                                 
                                                            
     
		misaligned = 1;
	}
	if (iov_base & (devblocksize - 1)) {
		  
                                                            
                                                     
                                                   
     
		misaligned = 1;
	}

	max_io_size = filesize - uio->uio_offset;

	  
                                                       
                                                        
                                                                    
    
	if (flags & IO_ENCRYPTED) {
		if (misaligned || (io_req_size & (devblocksize - 1))) {
			retval = EINVAL;
		}

		max_io_size = roundup(max_io_size, devblocksize);
	}

	if ((off_t)io_req_size > max_io_size) {
		io_req_size = (u_int32_t)max_io_size;
	}

	  
                                         
                                                              
    

	while (io_req_size && retval == 0) {
		u_int32_t io_start;

		if (cluster_is_throttled(vp)) {
			  
                                                     
                                                      
              
      
			max_rd_size  = THROTTLE_MAX_IOSIZE;
			max_rd_ahead = THROTTLE_MAX_IOSIZE - 1;
			max_vector_size = THROTTLE_MAX_IOSIZE;
		} else {
			max_rd_size  = max_upl_size;
			max_rd_ahead = max_rd_size * IO_SCALE(vp, 2);
			max_vector_size = MAX_VECTOR_UPL_SIZE;
		}
		io_start = io_size = io_req_size;

		  
                                              
                                                   
                                                           
                                                    
                   
    
                                            
               
     
		if ((flags & IO_ENCRYPTED) == 0) {
			retval = cluster_copy_ubc_data_internal(vp, uio, (int *)&io_size, 0, 0);
		}
		  
                                                  
                             
     
		xsize = io_start - io_size;

		io_req_size -= xsize;

		if (useVectorUPL && (xsize || (iov_base & PAGE_MASK))) {
			  
                                                                       
                   
     
                                                                        
      
			if (vector_upl_index) {
				retval = vector_cluster_io(vp, vector_upl, vector_upl_offset, v_upl_uio_offset, vector_upl_iosize, io_flag, (buf_t)NULL, &iostate, callback, callback_arg);
				reset_vector_run_state();
			}

			if (xsize) {
				useVectorUPL = 0;
			}

			  
                                                                           
                                                                                       
      
		}

		  
                                                       
    
                                                                 
                                                                     
                                     
    
     
		if (io_req_size == 0 || (misaligned)) {
			  
                                          
                                      
     
                                          
      
			break;
		}
		  
                                                       
     
		io_min = devblocksize;

		  
                                                              
                                                            
                                                            
                                                            
                                                              
                                                              
                                                               
                                                               
                                                            
                              
     
		if (io_size & (devblocksize - 1)) {
			assert(!(flags & IO_ENCRYPTED));
			  
                                                         
                                                           
      
			io_size &= ~PAGE_MASK;
			io_min = PAGE_SIZE;
		}
		if (retval || io_size < io_min) {
			  
                                                      
                                   
                                                           
                                                          
                                  
      
			goto wait_for_dreads;
		}

		  
                                                                  
                                    
     
		if ((flags & IO_ENCRYPTED) == 0) {
			if ((xsize = io_size) > max_rd_size) {
				xsize = max_rd_size;
			}

			io_size = 0;

			if (!lock) {
				  
                                                        
                                                      
                                                       
                                                     
                                                      
                                                         
                                      
       
				lock = cluster_lock_direct_read(vp, LCK_RW_TYPE_SHARED);
			}

			ubc_range_op(vp, uio->uio_offset, uio->uio_offset + xsize, UPL_ROP_ABSENT, (int *)&io_size);

			if (io_size == 0) {
				  
                                                
                                               
                                             
       
				continue;
			}
		}
		if ((flags & IO_RETURN_ON_THROTTLE)) {
			if (cluster_is_throttled(vp) == THROTTLE_NOW) {
				if (!cluster_io_present_in_BC(vp, uio->uio_offset)) {
					  
                                                       
                                                        
                                                         
                                                       
                                                    
        
					throttle_info_update_by_mount(vp->v_mount);

					io_throttled = TRUE;
					goto wait_for_dreads;
				}
			}
		}
		if (io_size > max_rd_size) {
			io_size = max_rd_size;
		}

		iov_base = uio_curriovbase(uio);

		upl_offset = (vm_offset_t)((u_int32_t)iov_base & PAGE_MASK);
		upl_needed_size = (upl_offset + io_size + (PAGE_SIZE - 1)) & ~PAGE_MASK;

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 72)) | DBG_FUNC_START,
		    (int)upl_offset, upl_needed_size, (int)iov_base, io_size, 0);

		if (upl_offset == 0 && ((io_size & PAGE_MASK) == 0)) {
			no_zero_fill = 1;
		} else {
			no_zero_fill = 0;
		}

		vm_map_t map = UIO_SEG_IS_USER_SPACE(uio->uio_segflg) ? current_map() : kernel_map;
		for (force_data_sync = 0; force_data_sync < 3; force_data_sync++) {
			pages_in_pl = 0;
			upl_size = (upl_size_t)upl_needed_size;
			upl_flags = UPL_FILE_IO | UPL_NO_SYNC | UPL_SET_INTERNAL | UPL_SET_LITE | UPL_SET_IO_WIRE;
			if (no_zero_fill) {
				upl_flags |= UPL_NOZEROFILL;
			}
			if (force_data_sync) {
				upl_flags |= UPL_FORCE_DATA_SYNC;
			}

			kret = vm_map_create_upl(map,
			    (vm_map_offset_t)(iov_base & ~((user_addr_t)PAGE_MASK)),
			    &upl_size, &upl, NULL, &pages_in_pl, &upl_flags, VM_KERN_MEMORY_FILE);

			if (kret != KERN_SUCCESS) {
				KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 72)) | DBG_FUNC_END,
				    (int)upl_offset, upl_size, io_size, kret, 0);
				  
                             
      
                                                            
                                                           
                                   
       
				goto wait_for_dreads;
			}
			pages_in_pl = upl_size / PAGE_SIZE;
			pl = UPL_GET_INTERNAL_PAGE_LIST(upl);

			for (i = 0; i < pages_in_pl; i++) {
				if (!upl_page_present(pl, i)) {
					break;
				}
			}
			if (i == pages_in_pl) {
				break;
			}

			ubc_upl_abort(upl, 0);
		}
		if (force_data_sync >= 3) {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 72)) | DBG_FUNC_END,
			    (int)upl_offset, upl_size, io_size, kret, 0);

			goto wait_for_dreads;
		}
		  
                                                             
     
		if (upl_size < upl_needed_size) {
			if (upl_size && upl_offset == 0) {
				io_size = upl_size;
			} else {
				io_size = 0;
			}
		}
		if (io_size == 0) {
			ubc_upl_abort(upl, 0);
			goto wait_for_dreads;
		}
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 72)) | DBG_FUNC_END,
		    (int)upl_offset, upl_size, io_size, kret, 0);

		if (useVectorUPL) {
			vm_offset_t end_off = ((iov_base + io_size) & PAGE_MASK);
			if (end_off) {
				issueVectorUPL = 1;
			}
			  
                                                          
                                                           
                                                          
                         
      
		}

		  
                                                  
                                    
                                                    
                                                                
     
		cluster_iostate_wait(&iostate, max_rd_ahead, "cluster_read_direct");

		if (iostate.io_error) {
			  
                                                              
                                                 
                                              
                                                    
                                       
      
			ubc_upl_abort(upl, 0);

			goto wait_for_dreads;
		}
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 73)) | DBG_FUNC_START,
		    upl, (int)upl_offset, (int)uio->uio_offset, io_size, 0);

		if (!useVectorUPL) {
			if (no_zero_fill) {
				io_flag &= ~CL_PRESERVE;
			} else {
				io_flag |= CL_PRESERVE;
			}

			retval = cluster_io(vp, upl, upl_offset, uio->uio_offset, io_size, io_flag, (buf_t)NULL, &iostate, callback, callback_arg);
		} else {
			if (!vector_upl_index) {
				vector_upl = vector_upl_create(upl_offset);
				v_upl_uio_offset = uio->uio_offset;
				vector_upl_offset = upl_offset;
			}

			vector_upl_set_subupl(vector_upl, upl, upl_size);
			vector_upl_set_iostate(vector_upl, upl, vector_upl_size, upl_size);
			vector_upl_index++;
			vector_upl_size += upl_size;
			vector_upl_iosize += io_size;

			if (issueVectorUPL || vector_upl_index == MAX_VECTOR_UPL_ELEMENTS || vector_upl_size >= max_vector_size) {
				retval = vector_cluster_io(vp, vector_upl, vector_upl_offset, v_upl_uio_offset, vector_upl_iosize, io_flag, (buf_t)NULL, &iostate, callback, callback_arg);
				reset_vector_run_state();
			}
		}
		last_iov_base = iov_base + io_size;

		if (lock) {
			                                                
			cluster_unlock_direct_read(lock);
			lock = NULL;
		}

		  
                             
     
		if ((flags & IO_ENCRYPTED) && (max_io_size < io_size)) {
			uio_update(uio, (user_size_t)max_io_size);
		} else {
			uio_update(uio, (user_size_t)io_size);
		}

		io_req_size -= io_size;

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 73)) | DBG_FUNC_END,
		    upl, (int)uio->uio_offset, io_req_size, retval, 0);
	}                

	if (retval == 0 && iostate.io_error == 0 && io_req_size == 0 && uio->uio_offset < filesize) {
		retval = cluster_io_type(uio, read_type, read_length, 0);

		if (retval == 0 && *read_type == IO_DIRECT) {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 70)) | DBG_FUNC_NONE,
			    (int)uio->uio_offset, (int)filesize, *read_type, *read_length, 0);

			goto next_dread;
		}
	}

wait_for_dreads:

	if (retval == 0 && iostate.io_error == 0 && useVectorUPL && vector_upl_index) {
		retval = vector_cluster_io(vp, vector_upl, vector_upl_offset, v_upl_uio_offset, vector_upl_iosize, io_flag, (buf_t)NULL, &iostate, callback, callback_arg);
		reset_vector_run_state();
	}

	                                                
	if (lock) {
		cluster_unlock_direct_read(lock);
	}

	  
                                                          
                                   
    
	cluster_iostate_wait(&iostate, 0, "cluster_read_direct");

	if (iostate.io_error) {
		retval = iostate.io_error;
	}

	lck_mtx_destroy(&iostate.io_mtxp, &cl_mtx_grp);

	if (io_throttled == TRUE && retval == 0) {
		retval = EAGAIN;
	}

	vm_map_offset_t current_page_size, current_page_mask;
	current_page_size = vm_map_page_size(current_map());
	current_page_mask = vm_map_page_mask(current_map());
	for (next_iov_base = orig_iov_base;
	    next_iov_base < last_iov_base;
	    next_iov_base += current_page_size) {
		  
                                                            
                                                               
                                                                   
     
		vm_pre_fault(vm_map_trunc_page(next_iov_base, current_page_mask), VM_PROT_READ);
	}

	if (io_req_size && retval == 0) {
		  
                                                               
                                     
     
		if (flags & IO_ENCRYPTED) {
			  
                                                                     
                                                                   
           
      
			retval = EFAULT;
		} else {
			retval = cluster_read_copy(vp, uio, io_req_size, filesize, flags, callback, callback_arg);
		}

		*read_type = IO_UNKNOWN;
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 70)) | DBG_FUNC_END,
	    (int)uio->uio_offset, (int)uio_resid(uio), io_req_size, retval, 0);

	return retval;
}


static int
cluster_read_contig(vnode_t vp, struct uio *uio, off_t filesize, int *read_type, u_int32_t *read_length,
    int (*callback)(buf_t, void *), void *callback_arg, int flags)
{
	upl_page_info_t *pl;
	upl_t            upl[MAX_VECTS];
	vm_offset_t      upl_offset;
	addr64_t         dst_paddr = 0;
	user_addr_t      iov_base;
	off_t            max_size;
	upl_size_t       upl_size;
	vm_size_t        upl_needed_size;
	mach_msg_type_number_t  pages_in_pl;
	upl_control_flags_t upl_flags;
	kern_return_t    kret;
	struct clios     iostate;
	int              error = 0;
	int              cur_upl = 0;
	int              num_upl = 0;
	int              n;
	u_int32_t        xsize;
	u_int32_t        io_size;
	u_int32_t        devblocksize;
	u_int32_t        mem_alignment_mask;
	u_int32_t        tail_size = 0;
	int              bflag;

	if (flags & IO_PASSIVE) {
		bflag = CL_PASSIVE;
	} else {
		bflag = 0;
	}

	if (flags & IO_NOCACHE) {
		bflag |= CL_NOCACHE;
	}

	  
                                       
                                                           
                                                                   
    
	cluster_syncup(vp, filesize, callback, callback_arg, PUSH_SYNC);

	devblocksize = (u_int32_t)vp->v_mount->mnt_devblocksize;
	mem_alignment_mask = (u_int32_t)vp->v_mount->mnt_alignmentmask;

	iostate.io_completed = 0;
	iostate.io_issued = 0;
	iostate.io_error = 0;
	iostate.io_wanted = 0;

	lck_mtx_init(&iostate.io_mtxp, &cl_mtx_grp, LCK_ATTR_NULL);

next_cread:
	io_size = *read_length;

	max_size = filesize - uio->uio_offset;

	if (io_size > max_size) {
		io_size = (u_int32_t)max_size;
	}

	iov_base = uio_curriovbase(uio);

	upl_offset = (vm_offset_t)((u_int32_t)iov_base & PAGE_MASK);
	upl_needed_size = upl_offset + io_size;

	pages_in_pl = 0;
	upl_size = (upl_size_t)upl_needed_size;
	upl_flags = UPL_FILE_IO | UPL_NO_SYNC | UPL_CLEAN_IN_PLACE | UPL_SET_INTERNAL | UPL_SET_LITE | UPL_SET_IO_WIRE;


	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 92)) | DBG_FUNC_START,
	    (int)upl_offset, (int)upl_size, (int)iov_base, io_size, 0);

	vm_map_t map = UIO_SEG_IS_USER_SPACE(uio->uio_segflg) ? current_map() : kernel_map;
	kret = vm_map_get_upl(map,
	    vm_map_trunc_page(iov_base, vm_map_page_mask(map)),
	    &upl_size, &upl[cur_upl], NULL, &pages_in_pl, &upl_flags, VM_KERN_MEMORY_FILE, 0);

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 92)) | DBG_FUNC_END,
	    (int)upl_offset, upl_size, io_size, kret, 0);

	if (kret != KERN_SUCCESS) {
		  
                           
     
		error = EINVAL;
		goto wait_for_creads;
	}
	num_upl++;

	if (upl_size < upl_needed_size) {
		  
                                   
     
		error = EINVAL;
		goto wait_for_creads;
	}
	pl = ubc_upl_pageinfo(upl[cur_upl]);

	dst_paddr = ((addr64_t)upl_phys_page(pl, 0) << PAGE_SHIFT) + (addr64_t)upl_offset;

	while (((uio->uio_offset & (devblocksize - 1)) || io_size < devblocksize) && io_size) {
		u_int32_t   head_size;

		head_size = devblocksize - (u_int32_t)(uio->uio_offset & (devblocksize - 1));

		if (head_size > io_size) {
			head_size = io_size;
		}

		error = cluster_align_phys_io(vp, uio, dst_paddr, head_size, CL_READ, callback, callback_arg);

		if (error) {
			goto wait_for_creads;
		}

		upl_offset += head_size;
		dst_paddr  += head_size;
		io_size    -= head_size;

		iov_base   += head_size;
	}
	if ((u_int32_t)iov_base & mem_alignment_mask) {
		  
                                                
                                            
                                             
                                                
                                        
     
		error = EINVAL;
		goto wait_for_creads;
	}

	tail_size = io_size & (devblocksize - 1);

	io_size  -= tail_size;

	while (io_size && error == 0) {
		if (io_size > MAX_IO_CONTIG_SIZE) {
			xsize = MAX_IO_CONTIG_SIZE;
		} else {
			xsize = io_size;
		}
		  
                                                  
                                                
                                               
                                              
                                                    
                                                           
     
		cluster_iostate_wait(&iostate, MAX_IO_CONTIG_SIZE * IO_SCALE(vp, 2), "cluster_read_contig");

		if (iostate.io_error) {
			  
                                                              
                                   
                                                    
                                       
      
			goto wait_for_creads;
		}
		error = cluster_io(vp, upl[cur_upl], upl_offset, uio->uio_offset, xsize,
		    CL_READ | CL_NOZERO | CL_DEV_MEMORY | CL_ASYNC | bflag,
		    (buf_t)NULL, &iostate, callback, callback_arg);
		  
                                                 
                             
     
		if (error == 0) {
			uio_update(uio, (user_size_t)xsize);

			dst_paddr  += xsize;
			upl_offset += xsize;
			io_size    -= xsize;
		}
	}
	if (error == 0 && iostate.io_error == 0 && tail_size == 0 && num_upl < MAX_VECTS && uio->uio_offset < filesize) {
		error = cluster_io_type(uio, read_type, read_length, 0);

		if (error == 0 && *read_type == IO_CONTIG) {
			cur_upl++;
			goto next_cread;
		}
	} else {
		*read_type = IO_UNKNOWN;
	}

wait_for_creads:
	  
                                                          
                                    
    
	cluster_iostate_wait(&iostate, 0, "cluster_read_contig");

	if (iostate.io_error) {
		error = iostate.io_error;
	}

	lck_mtx_destroy(&iostate.io_mtxp, &cl_mtx_grp);

	if (error == 0 && tail_size) {
		error = cluster_align_phys_io(vp, uio, dst_paddr, tail_size, CL_READ, callback, callback_arg);
	}

	for (n = 0; n < num_upl; n++) {
		  
                                                        
                                      
     
		ubc_upl_abort(upl[n], 0);
	}

	return error;
}


static int
cluster_io_type(struct uio *uio, int *io_type, u_int32_t *io_length, u_int32_t min_length)
{
	user_size_t      iov_len;
	user_addr_t      iov_base = 0;
	upl_t            upl;
	upl_size_t       upl_size;
	upl_control_flags_t upl_flags;
	int              retval = 0;

	  
                               
    
	uio_update(uio, (user_size_t)0);

	iov_len = uio_curriovlen(uio);

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 94)) | DBG_FUNC_START, uio, (int)iov_len, 0, 0, 0);

	if (iov_len) {
		iov_base = uio_curriovbase(uio);
		  
                                                      
                                                    
                                         
     
		if (iov_len > (user_size_t)MAX_IO_REQUEST_SIZE) {
			upl_size = MAX_IO_REQUEST_SIZE;
		} else {
			upl_size = (u_int32_t)iov_len;
		}

		upl_flags = UPL_QUERY_OBJECT_TYPE;

		vm_map_t map = UIO_SEG_IS_USER_SPACE(uio->uio_segflg) ? current_map() : kernel_map;
		if ((vm_map_get_upl(map,
		    vm_map_trunc_page(iov_base, vm_map_page_mask(map)),
		    &upl_size, &upl, NULL, NULL, &upl_flags, VM_KERN_MEMORY_FILE, 0)) != KERN_SUCCESS) {
			  
                                                         
      
			retval = EFAULT;
		}
		if (upl_size == 0) {
			retval = EFAULT;
		}

		*io_length = upl_size;

		if (upl_flags & UPL_PHYS_CONTIG) {
			*io_type = IO_CONTIG;
		} else if (iov_len >= min_length) {
			*io_type = IO_DIRECT;
		} else {
			*io_type = IO_COPY;
		}
	} else {
		  
                                    
     
		*io_length = 0;
		*io_type   = IO_UNKNOWN;
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 94)) | DBG_FUNC_END, iov_base, *io_type, *io_length, retval, 0);

	if (*io_type == IO_DIRECT &&
	    vm_map_page_shift(current_map()) < PAGE_SHIFT) {
		                                                    
		DEBUG4K_VFS("io_type IO_DIRECT -> IO_COPY\n");
		*io_type = IO_COPY;
	}

	return retval;
}


  
                                                         
                                                         
   
int
advisory_read(vnode_t vp, off_t filesize, off_t f_offset, int resid)
{
	return advisory_read_ext(vp, filesize, f_offset, resid, NULL, NULL, CL_PASSIVE);
}

int
advisory_read_ext(vnode_t vp, off_t filesize, off_t f_offset, int resid, int (*callback)(buf_t, void *), void *callback_arg, int bflag)
{
	upl_page_info_t *pl;
	upl_t            upl;
	vm_offset_t      upl_offset;
	int              upl_size;
	off_t            upl_f_offset;
	int              start_offset;
	int              start_pg;
	int              last_pg;
	int              pages_in_upl;
	off_t            max_size;
	int              io_size;
	kern_return_t    kret;
	int              retval = 0;
	int              issued_io;
	int              skip_range;
	uint32_t         max_io_size;


	if (!UBCINFOEXISTS(vp)) {
		return EINVAL;
	}

	if (f_offset < 0 || resid < 0) {
		return EINVAL;
	}

	max_io_size = cluster_max_io_size(vp->v_mount, CL_READ);

	if (disk_conditioner_mount_is_ssd(vp->v_mount)) {
		if (max_io_size > speculative_prefetch_max_iosize) {
			max_io_size = speculative_prefetch_max_iosize;
		}
	}

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 60)) | DBG_FUNC_START,
	    (int)f_offset, resid, (int)filesize, 0, 0);

	while (resid && f_offset < filesize && retval == 0) {
		  
                                                    
                                                        
                                                       
                                                     
                             
                                              
     
		start_offset = (int)(f_offset & PAGE_MASK_64);
		upl_f_offset = f_offset - (off_t)start_offset;
		max_size     = filesize - f_offset;

		if (resid < max_size) {
			io_size = resid;
		} else {
			io_size = (int)max_size;
		}

		upl_size = (start_offset + io_size + (PAGE_SIZE - 1)) & ~PAGE_MASK;
		if ((uint32_t)upl_size > max_io_size) {
			upl_size = max_io_size;
		}

		skip_range = 0;
		  
                                                                 
                                             
     
		ubc_range_op(vp, upl_f_offset, upl_f_offset + upl_size, UPL_ROP_PRESENT, &skip_range);

		if (skip_range) {
			  
                                                  
      
			io_size = skip_range - start_offset;

			f_offset += io_size;
			resid    -= io_size;

			if (skip_range == upl_size) {
				continue;
			}
			  
                                 
                                                             
                                                                       
      
			start_offset = 0;
			upl_f_offset += skip_range;
			upl_size     -= skip_range;
		}
		pages_in_upl = upl_size / PAGE_SIZE;

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 61)) | DBG_FUNC_START,
		    upl, (int)upl_f_offset, upl_size, start_offset, 0);

		kret = ubc_create_upl_kernel(vp,
		    upl_f_offset,
		    upl_size,
		    &upl,
		    &pl,
		    UPL_RET_ONLY_ABSENT | UPL_SET_LITE,
		    VM_KERN_MEMORY_FILE);
		if (kret != KERN_SUCCESS) {
			return retval;
		}
		issued_io = 0;

		  
                                                                  
                                                              
        
     
		for (last_pg = pages_in_upl - 1; last_pg >= 0; last_pg--) {
			if (upl_page_present(pl, last_pg)) {
				break;
			}
		}
		pages_in_upl = last_pg + 1;


		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 61)) | DBG_FUNC_END,
		    upl, (int)upl_f_offset, upl_size, start_offset, 0);


		for (last_pg = 0; last_pg < pages_in_upl;) {
			  
                                                              
                                                                 
                                                               
                                                                    
      
			for (start_pg = last_pg; start_pg < pages_in_upl; start_pg++) {
				if (upl_page_present(pl, start_pg)) {
					break;
				}
			}

			  
                                                               
                                                      
                                                               
                               
      
			for (last_pg = start_pg; last_pg < pages_in_upl; last_pg++) {
				if (!upl_page_present(pl, last_pg)) {
					break;
				}
			}

			if (last_pg > start_pg) {
				  
                                                    
                                                                  
                                                                   
                                                                  
       
				upl_offset = start_pg * PAGE_SIZE;
				io_size    = (last_pg - start_pg) * PAGE_SIZE;

				if ((off_t)(upl_f_offset + upl_offset + io_size) > filesize) {
					io_size = (int)(filesize - (upl_f_offset + upl_offset));
				}

				  
                                               
       
				retval = cluster_io(vp, upl, upl_offset, upl_f_offset + upl_offset, io_size,
				    CL_ASYNC | CL_READ | CL_COMMIT | CL_AGE | bflag, (buf_t)NULL, (struct clios *)NULL, callback, callback_arg);

				issued_io = 1;
			}
		}
		if (issued_io == 0) {
			ubc_upl_abort(upl, 0);
		}

		io_size = upl_size - start_offset;

		if (io_size > resid) {
			io_size = resid;
		}
		f_offset += io_size;
		resid    -= io_size;
	}

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 60)) | DBG_FUNC_END,
	    (int)f_offset, resid, retval, 0, 0);

	return retval;
}


int
cluster_push(vnode_t vp, int flags)
{
	return cluster_push_ext(vp, flags, NULL, NULL);
}


int
cluster_push_ext(vnode_t vp, int flags, int (*callback)(buf_t, void *), void *callback_arg)
{
	return cluster_push_err(vp, flags, callback, callback_arg, NULL);
}

                                                                     
int
cluster_push_err(vnode_t vp, int flags, int (*callback)(buf_t, void *), void *callback_arg, int *err)
{
	int     retval;
	int     my_sparse_wait = 0;
	struct  cl_writebehind *wbp;
	int     local_err = 0;

	if (err) {
		*err = 0;
	}

	if (!UBCINFOEXISTS(vp)) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 53)) | DBG_FUNC_NONE, kdebug_vnode(vp), flags, 0, -1, 0);
		return 0;
	}
	                                     
	if (((unsigned int)vfs_flags(vp->v_mount) & MNT_DEFWRITE) && (flags & IO_DEFWRITE)) {
		return 0;
	}
	if ((wbp = cluster_get_wbp(vp, CLW_RETURNLOCKED)) == NULL) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 53)) | DBG_FUNC_NONE, kdebug_vnode(vp), flags, 0, -2, 0);
		return 0;
	}
	if (!ISSET(flags, IO_SYNC) && wbp->cl_number == 0 && wbp->cl_scmap == NULL) {
		lck_mtx_unlock(&wbp->cl_lockw);

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 53)) | DBG_FUNC_NONE, kdebug_vnode(vp), flags, 0, -3, 0);
		return 0;
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 53)) | DBG_FUNC_START,
	    wbp->cl_scmap, wbp->cl_number, flags, 0, 0);

	  
                                                                          
                                                   
                                                                       
                                                                        
                          
    
	while (wbp->cl_sparse_wait) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 97)) | DBG_FUNC_START, kdebug_vnode(vp), 0, 0, 0, 0);

		msleep((caddr_t)&wbp->cl_sparse_wait, &wbp->cl_lockw, PRIBIO + 1, "cluster_push_ext", NULL);

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 97)) | DBG_FUNC_END, kdebug_vnode(vp), 0, 0, 0, 0);
	}
	if (flags & IO_SYNC) {
		my_sparse_wait = 1;
		wbp->cl_sparse_wait = 1;

		  
                                                                            
                                                                         
                                                                           
                                                                     
     
		while (wbp->cl_sparse_pushes) {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 98)) | DBG_FUNC_START, kdebug_vnode(vp), 0, 0, 0, 0);

			msleep((caddr_t)&wbp->cl_sparse_pushes, &wbp->cl_lockw, PRIBIO + 1, "cluster_push_ext", NULL);

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 98)) | DBG_FUNC_END, kdebug_vnode(vp), 0, 0, 0, 0);
		}
	}
	if (wbp->cl_scmap) {
		void    *scmap;

		if (wbp->cl_sparse_pushes < SPARSE_PUSH_LIMIT) {
			scmap = wbp->cl_scmap;
			wbp->cl_scmap = NULL;

			wbp->cl_sparse_pushes++;

			lck_mtx_unlock(&wbp->cl_lockw);

			retval = sparse_cluster_push(wbp, &scmap, vp, ubc_getsize(vp), PUSH_ALL, flags, callback, callback_arg, FALSE);

			lck_mtx_lock(&wbp->cl_lockw);

			wbp->cl_sparse_pushes--;

			if (retval) {
				if (wbp->cl_scmap != NULL) {
					panic("cluster_push_err: Expected NULL cl_scmap\n");
				}

				wbp->cl_scmap = scmap;
			}

			if (wbp->cl_sparse_wait && wbp->cl_sparse_pushes == 0) {
				wakeup((caddr_t)&wbp->cl_sparse_pushes);
			}
		} else {
			retval = sparse_cluster_push(wbp, &(wbp->cl_scmap), vp, ubc_getsize(vp), PUSH_ALL, flags, callback, callback_arg, FALSE);
		}

		local_err = retval;

		if (err) {
			*err = retval;
		}
		retval = 1;
	} else {
		retval = cluster_try_push(wbp, vp, ubc_getsize(vp), PUSH_ALL, flags, callback, callback_arg, &local_err, FALSE);
		if (err) {
			*err = local_err;
		}
	}
	lck_mtx_unlock(&wbp->cl_lockw);

	if (flags & IO_SYNC) {
		(void)vnode_waitforwrites(vp, 0, 0, 0, "cluster_push");
	}

	if (my_sparse_wait) {
		  
                                             
                                               
                     
     
		lck_mtx_lock(&wbp->cl_lockw);

		wbp->cl_sparse_wait = 0;
		wakeup((caddr_t)&wbp->cl_sparse_wait);

		lck_mtx_unlock(&wbp->cl_lockw);
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 53)) | DBG_FUNC_END,
	    wbp->cl_scmap, wbp->cl_number, retval, local_err, 0);

	return retval;
}


__private_extern__ void
cluster_release(struct ubc_info *ubc)
{
	struct cl_writebehind *wbp;
	struct cl_readahead   *rap;

	if ((wbp = ubc->cl_wbehind)) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 81)) | DBG_FUNC_START, ubc, wbp->cl_scmap, 0, 0, 0);

		if (wbp->cl_scmap) {
			vfs_drt_control(&(wbp->cl_scmap), 0);
		}
		lck_mtx_destroy(&wbp->cl_lockw, &cl_mtx_grp);
		zfree(cl_wr_zone, wbp);
		ubc->cl_wbehind = NULL;
	} else {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 81)) | DBG_FUNC_START, ubc, 0, 0, 0, 0);
	}

	if ((rap = ubc->cl_rahead)) {
		lck_mtx_destroy(&rap->cl_lockr, &cl_mtx_grp);
		zfree(cl_rd_zone, rap);
		ubc->cl_rahead  = NULL;
	}

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 81)) | DBG_FUNC_END, ubc, rap, wbp, 0, 0);
}


static int
cluster_try_push(struct cl_writebehind *wbp, vnode_t vp, off_t EOF, int push_flag, int io_flags, int (*callback)(buf_t, void *), void *callback_arg, int *err, boolean_t vm_initiated)
{
	int cl_index;
	int cl_index1;
	int min_index;
	int cl_len;
	int cl_pushed = 0;
	struct cl_wextent l_clusters[MAX_CLUSTERS];
	u_int  max_cluster_pgcount;
	int error = 0;

	max_cluster_pgcount = MAX_CLUSTER_SIZE(vp) / PAGE_SIZE;
	  
                                           
                          
    
	if (wbp->cl_number == 0) {
		  
                        
                                 
     
		return MAX_CLUSTERS;
	}

	  
                                              
                                                     
                
    
	for (cl_index = 0; cl_index < wbp->cl_number; cl_index++) {
		for (min_index = -1, cl_index1 = 0; cl_index1 < wbp->cl_number; cl_index1++) {
			if (wbp->cl_clusters[cl_index1].b_addr == wbp->cl_clusters[cl_index1].e_addr) {
				continue;
			}
			if (min_index == -1) {
				min_index = cl_index1;
			} else if (wbp->cl_clusters[cl_index1].b_addr < wbp->cl_clusters[min_index].b_addr) {
				min_index = cl_index1;
			}
		}
		if (min_index == -1) {
			break;
		}

		l_clusters[cl_index].b_addr = wbp->cl_clusters[min_index].b_addr;
		l_clusters[cl_index].e_addr = wbp->cl_clusters[min_index].e_addr;
		l_clusters[cl_index].io_flags = wbp->cl_clusters[min_index].io_flags;

		wbp->cl_clusters[min_index].b_addr = wbp->cl_clusters[min_index].e_addr;
	}
	wbp->cl_number = 0;

	cl_len = cl_index;

	                                                                    
	if (((push_flag & PUSH_DELAY) && cl_len == MAX_CLUSTERS) &&
	    !(vp->v_mount->mnt_kern_flag & MNTK_VIRTUALDEV)) {
		int   i;

		  
                                                               
                                                            
                                                                            
                                               
    
                                                                                                            
                                                    
    
                                                                                                     
                                                                                              
                                                                                        
                                                                                                     
                   
    
                                                                                     
                                                                                            
                                                                                                                       
     
		for (i = 0; i < MAX_CLUSTERS - 1; i++) {
			if ((l_clusters[i].e_addr - l_clusters[i].b_addr) != max_cluster_pgcount) {
				goto dont_try;
			}
			if (l_clusters[i].e_addr != l_clusters[i + 1].b_addr) {
				goto dont_try;
			}
		}
	}
	if (vm_initiated == TRUE) {
		lck_mtx_unlock(&wbp->cl_lockw);
	}

	for (cl_index = 0; cl_index < cl_len; cl_index++) {
		int     flags;
		struct  cl_extent cl;
		int retval;

		flags = io_flags & (IO_PASSIVE | IO_CLOSE);

		  
                                        
     
		if (l_clusters[cl_index].io_flags & CLW_IONOCACHE) {
			flags |= IO_NOCACHE;
		}

		if (l_clusters[cl_index].io_flags & CLW_IOPASSIVE) {
			flags |= IO_PASSIVE;
		}

		if (push_flag & PUSH_SYNC) {
			flags |= IO_SYNC;
		}

		cl.b_addr = l_clusters[cl_index].b_addr;
		cl.e_addr = l_clusters[cl_index].e_addr;

		retval = cluster_push_now(vp, &cl, EOF, flags, callback, callback_arg, vm_initiated);

		if (retval == 0) {
			cl_pushed++;

			l_clusters[cl_index].b_addr = 0;
			l_clusters[cl_index].e_addr = 0;
		} else if (error == 0) {
			error = retval;
		}

		if (!(push_flag & PUSH_ALL)) {
			break;
		}
	}
	if (vm_initiated == TRUE) {
		lck_mtx_lock(&wbp->cl_lockw);
	}

	if (err) {
		*err = error;
	}

dont_try:
	if (cl_len > cl_pushed) {
		  
                                           
                                                
     
		if ((MAX_CLUSTERS - wbp->cl_number) < (cl_len - cl_pushed)) {
			  
                                                            
                                                               
                                      
                                                              
                                                                  
     
                                           
      
			sparse_cluster_switch(wbp, vp, EOF, callback, callback_arg, vm_initiated);

			for (cl_index = 0, cl_index1 = 0; cl_index < cl_len; cl_index++) {
				if (l_clusters[cl_index].b_addr == l_clusters[cl_index].e_addr) {
					continue;
				}
				wbp->cl_clusters[cl_index1].b_addr = l_clusters[cl_index].b_addr;
				wbp->cl_clusters[cl_index1].e_addr = l_clusters[cl_index].e_addr;
				wbp->cl_clusters[cl_index1].io_flags = l_clusters[cl_index].io_flags;

				cl_index1++;
			}
			  
                              
      
			wbp->cl_number = cl_index1;

			  
                                                                
                                        
      
			sparse_cluster_switch(wbp, vp, EOF, callback, callback_arg, vm_initiated);
		} else {
			  
                                                   
                                                  
                                   
      
			for (cl_index = 0, cl_index1 = wbp->cl_number; cl_index < cl_len; cl_index++) {
				if (l_clusters[cl_index].b_addr == l_clusters[cl_index].e_addr) {
					continue;
				}

				wbp->cl_clusters[cl_index1].b_addr = l_clusters[cl_index].b_addr;
				wbp->cl_clusters[cl_index1].e_addr = l_clusters[cl_index].e_addr;
				wbp->cl_clusters[cl_index1].io_flags = l_clusters[cl_index].io_flags;

				cl_index1++;
			}
			  
                              
      
			wbp->cl_number = cl_index1;
		}
	}
	return MAX_CLUSTERS - wbp->cl_number;
}



static int
cluster_push_now(vnode_t vp, struct cl_extent *cl, off_t EOF, int flags,
    int (*callback)(buf_t, void *), void *callback_arg, boolean_t vm_initiated)
{
	upl_page_info_t *pl;
	upl_t            upl;
	vm_offset_t      upl_offset;
	int              upl_size;
	off_t            upl_f_offset;
	int              pages_in_upl;
	int              start_pg;
	int              last_pg;
	int              io_size;
	int              io_flags;
	int              upl_flags;
	int              bflag;
	int              size;
	int              error = 0;
	int              retval;
	kern_return_t    kret;

	if (flags & IO_PASSIVE) {
		bflag = CL_PASSIVE;
	} else {
		bflag = 0;
	}

	if (flags & IO_SKIP_ENCRYPTION) {
		bflag |= CL_ENCRYPTED;
	}

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 51)) | DBG_FUNC_START,
	    (int)cl->b_addr, (int)cl->e_addr, (int)EOF, flags, 0);

	if ((pages_in_upl = (int)(cl->e_addr - cl->b_addr)) == 0) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 51)) | DBG_FUNC_END, 1, 0, 0, 0, 0);

		return 0;
	}
	upl_size = pages_in_upl * PAGE_SIZE;
	upl_f_offset = (off_t)(cl->b_addr * PAGE_SIZE_64);

	if (upl_f_offset + upl_size >= EOF) {
		if (upl_f_offset >= EOF) {
			  
                                             
                                                       
                        
      
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 51)) | DBG_FUNC_END, 1, 1, 0, 0, 0);

			return 0;
		}
		size = (int)(EOF - upl_f_offset);

		upl_size = (size + (PAGE_SIZE - 1)) & ~PAGE_MASK;
		pages_in_upl = upl_size / PAGE_SIZE;
	} else {
		size = upl_size;
	}


	if (vm_initiated) {
		vnode_pageout(vp, NULL, (upl_offset_t)0, upl_f_offset, (upl_size_t)upl_size,
		    UPL_MSYNC | UPL_VNODE_PAGER | UPL_KEEPCACHED, &error);

		return error;
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 41)) | DBG_FUNC_START, upl_size, size, 0, 0, 0);

	  
                                                                                                  
   
                                                                                             
                                                                                                               
                                                                                                                    
                                                                                                                  
                                                                                                  
   
                                                                                                           
    

	if ((vp->v_flag & VNOCACHE_DATA) || (flags & IO_NOCACHE)) {
		upl_flags = UPL_COPYOUT_FROM | UPL_RET_ONLY_DIRTY | UPL_SET_LITE | UPL_WILL_BE_DUMPED;
	} else {
		upl_flags = UPL_COPYOUT_FROM | UPL_RET_ONLY_DIRTY | UPL_SET_LITE;
	}

	kret = ubc_create_upl_kernel(vp,
	    upl_f_offset,
	    upl_size,
	    &upl,
	    &pl,
	    upl_flags,
	    VM_KERN_MEMORY_FILE);
	if (kret != KERN_SUCCESS) {
		panic("cluster_push: failed to get pagelist");
	}

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 41)) | DBG_FUNC_END, upl, upl_f_offset, 0, 0, 0);

	  
                                                
                                                                
                                                               
                                                                 
                                                                      
                                             
    
	for (last_pg = pages_in_upl - 1; last_pg >= 0; last_pg--) {
		if (upl_page_present(pl, last_pg)) {
			break;
		}
	}
	pages_in_upl = last_pg + 1;

	if (pages_in_upl == 0) {
		ubc_upl_abort(upl, 0);

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 51)) | DBG_FUNC_END, 1, 2, 0, 0, 0);
		return 0;
	}

	for (last_pg = 0; last_pg < pages_in_upl;) {
		  
                                        
                                           
                         
     
		for (start_pg = last_pg; start_pg < pages_in_upl; start_pg++) {
			if (upl_dirty_page(pl, start_pg)) {
				break;
			}
			if (upl_page_present(pl, start_pg)) {
				  
                                                            
                                                         
                                          
       
				ubc_upl_abort_range(upl, start_pg * PAGE_SIZE, PAGE_SIZE, UPL_ABORT_FREE_ON_EMPTY);
			}
		}
		if (start_pg >= pages_in_upl) {
			  
                                         
      
			break;
		}
		if (start_pg > last_pg) {
			  
                                       
      
			size -= ((start_pg - last_pg) * PAGE_SIZE);
		}

		  
                                         
     
		for (last_pg = start_pg; last_pg < pages_in_upl; last_pg++) {
			if (!upl_dirty_page(pl, last_pg)) {
				break;
			}
		}
		upl_offset = start_pg * PAGE_SIZE;

		io_size = min(size, (last_pg - start_pg) * PAGE_SIZE);

		io_flags = CL_THROTTLE | CL_COMMIT | CL_AGE | bflag;

		if (!(flags & IO_SYNC)) {
			io_flags |= CL_ASYNC;
		}

		if (flags & IO_CLOSE) {
			io_flags |= CL_CLOSE;
		}

		if (flags & IO_NOCACHE) {
			io_flags |= CL_NOCACHE;
		}

		retval = cluster_io(vp, upl, upl_offset, upl_f_offset + upl_offset, io_size,
		    io_flags, (buf_t)NULL, (struct clios *)NULL, callback, callback_arg);

		if (error == 0 && retval) {
			error = retval;
		}

		size -= io_size;
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 51)) | DBG_FUNC_END, 1, 3, error, 0, 0);

	return error;
}


  
                                                                  
   
static int
sparse_cluster_switch(struct cl_writebehind *wbp, vnode_t vp, off_t EOF, int (*callback)(buf_t, void *), void *callback_arg, boolean_t vm_initiated)
{
	int     cl_index;
	int     error;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 78)) | DBG_FUNC_START, kdebug_vnode(vp), wbp->cl_scmap, wbp->cl_number, 0, 0);

	for (cl_index = 0; cl_index < wbp->cl_number; cl_index++) {
		int       flags;
		struct cl_extent cl;

		for (cl.b_addr = wbp->cl_clusters[cl_index].b_addr; cl.b_addr < wbp->cl_clusters[cl_index].e_addr; cl.b_addr++) {
			if (ubc_page_op(vp, (off_t)(cl.b_addr * PAGE_SIZE_64), 0, NULL, &flags) == KERN_SUCCESS) {
				if (flags & UPL_POP_DIRTY) {
					cl.e_addr = cl.b_addr + 1;

					error = sparse_cluster_add(wbp, &(wbp->cl_scmap), vp, &cl, EOF, callback, callback_arg, vm_initiated);

					if (error) {
						break;
					}
				}
			}
		}
	}
	wbp->cl_number -= cl_index;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 78)) | DBG_FUNC_END, kdebug_vnode(vp), wbp->cl_scmap, wbp->cl_number, error, 0);

	return error;
}


  
                                                                                     
                                                                                                 
                                                                                 
   
static int
sparse_cluster_push(struct cl_writebehind *wbp, void **scmap, vnode_t vp, off_t EOF, int push_flag,
    int io_flags, int (*callback)(buf_t, void *), void *callback_arg, boolean_t vm_initiated)
{
	struct cl_extent cl;
	off_t           offset;
	u_int           length;
	void            *l_scmap;
	int error = 0;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 79)) | DBG_FUNC_START, kdebug_vnode(vp), (*scmap), 0, push_flag, 0);

	if (push_flag & PUSH_ALL) {
		vfs_drt_control(scmap, 1);
	}

	l_scmap = *scmap;

	for (;;) {
		int retval;

		if (vfs_drt_get_cluster(scmap, &offset, &length) != KERN_SUCCESS) {
			break;
		}

		if (vm_initiated == TRUE) {
			lck_mtx_unlock(&wbp->cl_lockw);
		}

		cl.b_addr = (daddr64_t)(offset / PAGE_SIZE_64);
		cl.e_addr = (daddr64_t)((offset + length) / PAGE_SIZE_64);

		retval = cluster_push_now(vp, &cl, EOF, io_flags, callback, callback_arg, vm_initiated);
		if (error == 0 && retval) {
			error = retval;
		}

		if (vm_initiated == TRUE) {
			lck_mtx_lock(&wbp->cl_lockw);

			if (*scmap != l_scmap) {
				break;
			}
		}

		if (error) {
			if (vfs_drt_mark_pages(scmap, offset, length, NULL) != KERN_SUCCESS) {
				panic("Failed to restore dirty state on failure\n");
			}

			break;
		}

		if (!(push_flag & PUSH_ALL)) {
			break;
		}
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 79)) | DBG_FUNC_END, kdebug_vnode(vp), (*scmap), error, 0, 0);

	return error;
}


  
                                                               
   
static int
sparse_cluster_add(struct cl_writebehind *wbp, void **scmap, vnode_t vp, struct cl_extent *cl, off_t EOF,
    int (*callback)(buf_t, void *), void *callback_arg, boolean_t vm_initiated)
{
	u_int   new_dirty;
	u_int   length;
	off_t   offset;
	int     error;
	int     push_flag = 0;                             

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 80)) | DBG_FUNC_START, (*scmap), 0, cl->b_addr, (int)cl->e_addr, 0);

	offset = (off_t)(cl->b_addr * PAGE_SIZE_64);
	length = ((u_int)(cl->e_addr - cl->b_addr)) * PAGE_SIZE;

	while (vfs_drt_mark_pages(scmap, offset, length, &new_dirty) != KERN_SUCCESS) {
		  
                            
                                   
                                      
     

		if (vfs_get_scmap_push_behavior_internal(scmap, &push_flag)) {
			push_flag = 0;
		}

		error = sparse_cluster_push(wbp, scmap, vp, EOF, push_flag, 0, callback, callback_arg, vm_initiated);

		if (error) {
			break;
		}

		offset += (new_dirty * PAGE_SIZE_64);
		length -= (new_dirty * PAGE_SIZE);
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 80)) | DBG_FUNC_END, kdebug_vnode(vp), (*scmap), error, 0, 0);

	return error;
}


static int
cluster_align_phys_io(vnode_t vp, struct uio *uio, addr64_t usr_paddr, u_int32_t xsize, int flags, int (*callback)(buf_t, void *), void *callback_arg)
{
	upl_page_info_t  *pl;
	upl_t            upl;
	addr64_t         ubc_paddr;
	kern_return_t    kret;
	int              error = 0;
	int              did_read = 0;
	int              abort_flags;
	int              upl_flags;
	int              bflag;

	if (flags & IO_PASSIVE) {
		bflag = CL_PASSIVE;
	} else {
		bflag = 0;
	}

	if (flags & IO_NOCACHE) {
		bflag |= CL_NOCACHE;
	}

	upl_flags = UPL_SET_LITE;

	if (!(flags & CL_READ)) {
		  
                                                   
                                                    
                     
     
		upl_flags |= UPL_WILL_MODIFY;
	} else {
		  
                                               
                                              
                                    
     
		upl_flags |= UPL_FILE_IO;
	}
	kret = ubc_create_upl_kernel(vp,
	    uio->uio_offset & ~PAGE_MASK_64,
	    PAGE_SIZE,
	    &upl,
	    &pl,
	    upl_flags,
	    VM_KERN_MEMORY_FILE);

	if (kret != KERN_SUCCESS) {
		return EINVAL;
	}

	if (!upl_valid_page(pl, 0)) {
		  
                                           
     
		error = cluster_io(vp, upl, 0, uio->uio_offset & ~PAGE_MASK_64, PAGE_SIZE,
		    CL_READ | bflag, (buf_t)NULL, (struct clios *)NULL, callback, callback_arg);
		if (error) {
			ubc_upl_abort_range(upl, 0, PAGE_SIZE, UPL_ABORT_DUMP_PAGES | UPL_ABORT_FREE_ON_EMPTY);

			return error;
		}
		did_read = 1;
	}
	ubc_paddr = ((addr64_t)upl_phys_page(pl, 0) << PAGE_SHIFT) + (addr64_t)(uio->uio_offset & PAGE_MASK_64);

  
                                                                                 
                                                                                 
                                                                                   
                                                        
   
	if (flags & CL_READ) {
                                                                                                                                  
		copypv(ubc_paddr, usr_paddr, xsize, 2 |        1 |        4);                                                                    
	} else {
                                                                                                                             
		copypv(usr_paddr, ubc_paddr, xsize, 2 |        1 |        8);                                                               
	}
	if (!(flags & CL_READ) || (upl_valid_page(pl, 0) && upl_dirty_page(pl, 0))) {
		  
                                            
     
		error = cluster_io(vp, upl, 0, uio->uio_offset & ~PAGE_MASK_64, PAGE_SIZE,
		    bflag, (buf_t)NULL, (struct clios *)NULL, callback, callback_arg);
	}
	if (error == 0) {
		uio_update(uio, (user_size_t)xsize);
	}

	if (did_read) {
		abort_flags = UPL_ABORT_FREE_ON_EMPTY;
	} else {
		abort_flags = UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_DUMP_PAGES;
	}

	ubc_upl_abort_range(upl, 0, PAGE_SIZE, abort_flags);

	return error;
}

int
cluster_copy_upl_data(struct uio *uio, upl_t upl, int upl_offset, int *io_resid)
{
	int       pg_offset;
	int       pg_index;
	int       csize;
	int       segflg;
	int       retval = 0;
	int       xsize;
	upl_page_info_t *pl;
	int       dirty_count;

	xsize = *io_resid;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 34)) | DBG_FUNC_START,
	    (int)uio->uio_offset, upl_offset, xsize, 0, 0);

	segflg = uio->uio_segflg;

	switch (segflg) {
	case UIO_USERSPACE32:
	case UIO_USERISPACE32:
		uio->uio_segflg = UIO_PHYS_USERSPACE32;
		break;

	case UIO_USERSPACE:
	case UIO_USERISPACE:
		uio->uio_segflg = UIO_PHYS_USERSPACE;
		break;

	case UIO_USERSPACE64:
	case UIO_USERISPACE64:
		uio->uio_segflg = UIO_PHYS_USERSPACE64;
		break;

	case UIO_SYSSPACE:
		uio->uio_segflg = UIO_PHYS_SYSSPACE;
		break;
	}
	pl = ubc_upl_pageinfo(upl);

	pg_index  = upl_offset / PAGE_SIZE;
	pg_offset = upl_offset & PAGE_MASK;
	csize     = min(PAGE_SIZE - pg_offset, xsize);

	dirty_count = 0;
	while (xsize && retval == 0) {
		addr64_t  paddr;

		paddr = ((addr64_t)upl_phys_page(pl, pg_index) << PAGE_SHIFT) + pg_offset;
		if ((uio->uio_rw == UIO_WRITE) && (upl_dirty_page(pl, pg_index) == FALSE)) {
			dirty_count++;
		}

		retval = uiomove64(paddr, csize, uio);

		pg_index += 1;
		pg_offset = 0;
		xsize    -= csize;
		csize     = min(PAGE_SIZE, xsize);
	}
	*io_resid = xsize;

	uio->uio_segflg = segflg;

	task_update_logical_writes(current_task(), (dirty_count * PAGE_SIZE), TASK_WRITE_DEFERRED, upl_lookup_vnode(upl));
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 34)) | DBG_FUNC_END,
	    (int)uio->uio_offset, xsize, retval, segflg, 0);

	return retval;
}


int
cluster_copy_ubc_data(vnode_t vp, struct uio *uio, int *io_resid, int mark_dirty)
{
	return cluster_copy_ubc_data_internal(vp, uio, io_resid, mark_dirty, 1);
}


static int
cluster_copy_ubc_data_internal(vnode_t vp, struct uio *uio, int *io_resid, int mark_dirty, int take_reference)
{
	int       segflg;
	int       io_size;
	int       xsize;
	int       start_offset;
	int       retval = 0;
	memory_object_control_t  control;

	io_size = *io_resid;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 34)) | DBG_FUNC_START,
	    (int)uio->uio_offset, io_size, mark_dirty, take_reference, 0);

	control = ubc_getobject(vp, UBC_FLAGS_NONE);

	if (control == MEMORY_OBJECT_CONTROL_NULL) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 34)) | DBG_FUNC_END,
		    (int)uio->uio_offset, io_size, retval, 3, 0);

		return 0;
	}
	segflg = uio->uio_segflg;

	switch (segflg) {
	case UIO_USERSPACE32:
	case UIO_USERISPACE32:
		uio->uio_segflg = UIO_PHYS_USERSPACE32;
		break;

	case UIO_USERSPACE64:
	case UIO_USERISPACE64:
		uio->uio_segflg = UIO_PHYS_USERSPACE64;
		break;

	case UIO_USERSPACE:
	case UIO_USERISPACE:
		uio->uio_segflg = UIO_PHYS_USERSPACE;
		break;

	case UIO_SYSSPACE:
		uio->uio_segflg = UIO_PHYS_SYSSPACE;
		break;
	}

	if ((io_size = *io_resid)) {
		start_offset = (int)(uio->uio_offset & PAGE_MASK_64);
		xsize = (int)uio_resid(uio);

		retval = memory_object_control_uiomove(control, uio->uio_offset - start_offset, uio,
		    start_offset, io_size, mark_dirty, take_reference);
		xsize -= uio_resid(uio);
		io_size -= xsize;
	}
	uio->uio_segflg = segflg;
	*io_resid       = io_size;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 34)) | DBG_FUNC_END,
	    (int)uio->uio_offset, io_size, retval, 0x80000000 | segflg, 0);

	return retval;
}


int
is_file_clean(vnode_t vp, off_t filesize)
{
	off_t f_offset;
	int   flags;
	int   total_dirty = 0;

	for (f_offset = 0; f_offset < filesize; f_offset += PAGE_SIZE_64) {
		if (ubc_page_op(vp, f_offset, 0, NULL, &flags) == KERN_SUCCESS) {
			if (flags & UPL_POP_DIRTY) {
				total_dirty++;
			}
		}
	}
	if (total_dirty) {
		return EINVAL;
	}

	return 0;
}



  
                                              
  
                                                                         
                                                                           
                                                           
  
                                                               
  
                                                                      
                               
   

  
                                                                     
                                                                   
                        
   
#define DRT_BITVECTOR_PAGES             ((1024 * 256) / PAGE_SIZE)

  
                        
  
                                                        
                                                                     
   
#define DRT_ADDRESS_MASK                (~((DRT_BITVECTOR_PAGES * PAGE_SIZE) - 1))
#define DRT_ALIGN_ADDRESS(addr)         ((addr) & DRT_ADDRESS_MASK)

  
                                    
  
                                                                   
         
  
                                                              
                                                             
                                                      
   
#define DRT_HASH_GET_ADDRESS(scm, i)    ((scm)->scm_hashtable[(i)].dhe_control & DRT_ADDRESS_MASK)
#define DRT_HASH_SET_ADDRESS(scm, i, a)                                                                 \
	do {                                                                                            \
	        (scm)->scm_hashtable[(i)].dhe_control =                                                 \
	            ((scm)->scm_hashtable[(i)].dhe_control & ~DRT_ADDRESS_MASK) | DRT_ALIGN_ADDRESS(a); \
	} while (0)
#define DRT_HASH_COUNT_MASK             0x1ff
#define DRT_HASH_GET_COUNT(scm, i)      ((scm)->scm_hashtable[(i)].dhe_control & DRT_HASH_COUNT_MASK)
#define DRT_HASH_SET_COUNT(scm, i, c)                                                                                   \
	do {                                                                                                            \
	        (scm)->scm_hashtable[(i)].dhe_control =                                                                 \
	            ((scm)->scm_hashtable[(i)].dhe_control & ~DRT_HASH_COUNT_MASK) | ((c) & DRT_HASH_COUNT_MASK);       \
	} while (0)
#define DRT_HASH_CLEAR(scm, i)                                                                                          \
	do {                                                                                                            \
	        (scm)->scm_hashtable[(i)].dhe_control =	0;                                                              \
	} while (0)
#define DRT_HASH_VACATE(scm, i)         DRT_HASH_SET_COUNT((scm), (i), DRT_HASH_COUNT_MASK)
#define DRT_HASH_VACANT(scm, i)         (DRT_HASH_GET_COUNT((scm), (i)) == DRT_HASH_COUNT_MASK)
#define DRT_HASH_COPY(oscm, oi, scm, i)                                                                 \
	do {                                                                                            \
	        (scm)->scm_hashtable[(i)].dhe_control = (oscm)->scm_hashtable[(oi)].dhe_control;        \
	        DRT_BITVECTOR_COPY(oscm, oi, scm, i);                                                   \
	} while(0);


#if !defined(XNU_TARGET_OS_OSX)
  
                     
  
                                                               
                                                                
                                                             
                                                     
  
                                                           
  
                                                                       
                                                                         
                                                                           
   

#define DRT_HASH_SMALL_MODULUS  251
#define DRT_HASH_LARGE_MODULUS  2039
#define DRT_HASH_XLARGE_MODULUS  8179

  
                                                                       
  
                                                                       
                                                 
   
#define DRT_HASH_LARGE_MEMORY_REQUIRED  (1024LL * 1024LL * 1024LL)                
#define DRT_HASH_XLARGE_MEMORY_REQUIRED  (8 * 1024LL * 1024LL * 1024LL)            

#define DRT_SMALL_ALLOCATION    4096                        
#define DRT_LARGE_ALLOCATION    32768                        
#define DRT_XLARGE_ALLOCATION    131072                       

#else                        
  
                     
  
                                                               
                                                                
                                                             
                                                     
  
                                                           
  
                                                                         
                                                                          
                                                                            
   

#define DRT_HASH_SMALL_MODULUS  1019
#define DRT_HASH_LARGE_MODULUS  8179
#define DRT_HASH_XLARGE_MODULUS  32749

  
                                                                       
  
                                                                       
                                                 
   
#define DRT_HASH_LARGE_MEMORY_REQUIRED  (4 * 1024LL * 1024LL * 1024LL)            
#define DRT_HASH_XLARGE_MEMORY_REQUIRED  (32 * 1024LL * 1024LL * 1024LL)             

#define DRT_SMALL_ALLOCATION    16384                       
#define DRT_LARGE_ALLOCATION    131072                       
#define DRT_XLARGE_ALLOCATION   524288                       

#endif                          

                                                                               

  
                   
   
struct vfs_drt_hashentry {
	u_int64_t       dhe_control;
  
                                                                         
                                                               
                                              
                                                                        
                                                           
   
#define MAX_DRT_BITVECTOR_PAGES (1024 * 256)/( 4 * 1024)
	u_int32_t       dhe_bitvector[MAX_DRT_BITVECTOR_PAGES / 32];
};

  
                                
  
                                     
   

#define DRT_HASH_SET_BIT(scm, i, bit)                           \
	(scm)->scm_hashtable[(i)].dhe_bitvector[(bit) / 32] |= (1 << ((bit) % 32))

#define DRT_HASH_CLEAR_BIT(scm, i, bit)                         \
	(scm)->scm_hashtable[(i)].dhe_bitvector[(bit) / 32] &= ~(1 << ((bit) % 32))

#define DRT_HASH_TEST_BIT(scm, i, bit)                          \
	((scm)->scm_hashtable[(i)].dhe_bitvector[(bit) / 32] & (1 << ((bit) % 32)))

#define DRT_BITVECTOR_CLEAR(scm, i)                             \
	bzero(&(scm)->scm_hashtable[(i)].dhe_bitvector[0], (MAX_DRT_BITVECTOR_PAGES / 32) * sizeof(u_int32_t))

#define DRT_BITVECTOR_COPY(oscm, oi, scm, i)                    \
	bcopy(&(oscm)->scm_hashtable[(oi)].dhe_bitvector[0],    \
	    &(scm)->scm_hashtable[(i)].dhe_bitvector[0],        \
	    (MAX_DRT_BITVECTOR_PAGES / 32) * sizeof(u_int32_t))

  
                                   
  
                                                                
  
                                                                         
                                                  
   

struct vfs_drt_clustermap {
	u_int32_t               scm_magic;                            
#define DRT_SCM_MAGIC           0x12020003
	u_int32_t               scm_modulus;                           
	u_int32_t               scm_buckets;                                    
	u_int32_t               scm_lastclean;                             
	u_int32_t               scm_iskips;                               

	struct vfs_drt_hashentry scm_hashtable[0];
};


#define DRT_HASH(scm, addr)             ((addr) % (scm)->scm_modulus)
#define DRT_HASH_NEXT(scm, addr)        (((addr) + 1) % (scm)->scm_modulus)

  
                                 
   
#define DRT_DEBUG_EMPTYFREE     (FSDBG_CODE(DBG_FSRW, 82))          
#define DRT_DEBUG_RETCLUSTER    (FSDBG_CODE(DBG_FSRW, 83))                     
#define DRT_DEBUG_ALLOC         (FSDBG_CODE(DBG_FSRW, 84))                
#define DRT_DEBUG_INSERT        (FSDBG_CODE(DBG_FSRW, 85))                    
#define DRT_DEBUG_MARK          (FSDBG_CODE(DBG_FSRW, 86))                   
                                                               
                                                                            
                                                                                  
                                                                                   
                                                                                   
#define DRT_DEBUG_6             (FSDBG_CODE(DBG_FSRW, 87))
#define DRT_DEBUG_SCMDATA       (FSDBG_CODE(DBG_FSRW, 88))                     
                                                                           


static kern_return_t    vfs_drt_alloc_map(struct vfs_drt_clustermap **cmapp);
static kern_return_t    vfs_drt_free_map(struct vfs_drt_clustermap *cmap);
static kern_return_t    vfs_drt_search_index(struct vfs_drt_clustermap *cmap,
    u_int64_t offset, int *indexp);
static kern_return_t    vfs_drt_get_index(struct vfs_drt_clustermap **cmapp,
    u_int64_t offset,
    int *indexp,
    int recursed);
static kern_return_t    vfs_drt_do_mark_pages(
	void            **cmapp,
	u_int64_t       offset,
	u_int           length,
	u_int           *setcountp,
	int             dirty);
static void             vfs_drt_trace(
	struct vfs_drt_clustermap *cmap,
	int code,
	int arg1,
	int arg2,
	int arg3,
	int arg4);


  
                                                
  
                                                              
  
                                                                  
                                         
   
static kern_return_t
vfs_drt_alloc_map(struct vfs_drt_clustermap **cmapp)
{
	struct vfs_drt_clustermap *cmap = NULL, *ocmap = NULL;
	kern_return_t   kret = KERN_SUCCESS;
	u_int64_t       offset = 0;
	u_int32_t       i = 0;
	int             modulus_size = 0, map_size = 0, active_buckets = 0, index = 0, copycount = 0;

	ocmap = NULL;
	if (cmapp != NULL) {
		ocmap = *cmapp;
	}

	  
                                      
    
	if (ocmap == NULL) {
		modulus_size = DRT_HASH_SMALL_MODULUS;
		map_size = DRT_SMALL_ALLOCATION;
	} else {
		                                                       
		active_buckets = 0;
		for (i = 0; i < ocmap->scm_modulus; i++) {
			if (!DRT_HASH_VACANT(ocmap, i) &&
			    (DRT_HASH_GET_COUNT(ocmap, i) != 0)) {
				active_buckets++;
			}
		}
		  
                                                            
                                                 
     
		if (ocmap->scm_modulus == DRT_HASH_SMALL_MODULUS) {
			  
                                                      
                                     
      
			if ((active_buckets > (DRT_HASH_SMALL_MODULUS - 5)) &&
			    (max_mem >= DRT_HASH_LARGE_MEMORY_REQUIRED)) {
				modulus_size = DRT_HASH_LARGE_MODULUS;
				map_size = DRT_LARGE_ALLOCATION;
			} else {
				modulus_size = DRT_HASH_SMALL_MODULUS;
				map_size = DRT_SMALL_ALLOCATION;
			}
		} else if (ocmap->scm_modulus == DRT_HASH_LARGE_MODULUS) {
			if ((active_buckets > (DRT_HASH_LARGE_MODULUS - 5)) &&
			    (max_mem >= DRT_HASH_XLARGE_MEMORY_REQUIRED)) {
				modulus_size = DRT_HASH_XLARGE_MODULUS;
				map_size = DRT_XLARGE_ALLOCATION;
			} else {
				  
                                                  
                                                   
                                                     
                        
       
				return KERN_SUCCESS;
			}
		} else {
			                                      
			modulus_size = DRT_HASH_XLARGE_MODULUS;
			map_size = DRT_XLARGE_ALLOCATION;

			  
                                             
                                             
                                          
                       
      
			if (active_buckets >= DRT_HASH_XLARGE_MODULUS) {
				return KERN_SUCCESS;
			}
		}
	}

	  
                                        
    

	kret = kmem_alloc(kernel_map, (vm_offset_t *)&cmap, map_size, VM_KERN_MEMORY_FILE);
	if (kret != KERN_SUCCESS) {
		return kret;
	}
	cmap->scm_magic = DRT_SCM_MAGIC;
	cmap->scm_modulus = modulus_size;
	cmap->scm_buckets = 0;
	cmap->scm_lastclean = 0;
	cmap->scm_iskips = 0;
	for (i = 0; i < cmap->scm_modulus; i++) {
		DRT_HASH_CLEAR(cmap, i);
		DRT_HASH_VACATE(cmap, i);
		DRT_BITVECTOR_CLEAR(cmap, i);
	}

	  
                                                                    
    
	copycount = 0;
	if (ocmap != NULL) {
		for (i = 0; i < ocmap->scm_modulus; i++) {
			                        
			if (DRT_HASH_VACANT(ocmap, i) ||
			    (DRT_HASH_GET_COUNT(ocmap, i) == 0)) {
				continue;
			}
			                   
			offset = DRT_HASH_GET_ADDRESS(ocmap, i);
			kret = vfs_drt_get_index(&cmap, offset, &index, 1);
			if (kret != KERN_SUCCESS) {
				                                          
				panic("vfs_drt: new cluster map mysteriously too small");
				index = 0;
			}
			          
			DRT_HASH_COPY(ocmap, i, cmap, index);
			copycount++;
		}
	}

	                         
	vfs_drt_trace(cmap, DRT_DEBUG_ALLOC, copycount, 0, 0, 0);

	  
                                                         
                                                       
                
    
	*cmapp = cmap;
	if (ocmap != NULL) {
		                                  
		vfs_drt_trace(ocmap, DRT_DEBUG_SCMDATA,
		    ocmap->scm_modulus,
		    ocmap->scm_buckets,
		    ocmap->scm_lastclean,
		    ocmap->scm_iskips);

		vfs_drt_free_map(ocmap);
	}
	return KERN_SUCCESS;
}


  
                             
   
static kern_return_t
vfs_drt_free_map(struct vfs_drt_clustermap *cmap)
{
	vm_size_t map_size = 0;

	if (cmap->scm_modulus == DRT_HASH_SMALL_MODULUS) {
		map_size = DRT_SMALL_ALLOCATION;
	} else if (cmap->scm_modulus == DRT_HASH_LARGE_MODULUS) {
		map_size = DRT_LARGE_ALLOCATION;
	} else if (cmap->scm_modulus == DRT_HASH_XLARGE_MODULUS) {
		map_size = DRT_XLARGE_ALLOCATION;
	} else {
		panic("vfs_drt_free_map: Invalid modulus %d\n", cmap->scm_modulus);
	}

	kmem_free(kernel_map, (vm_offset_t)cmap, map_size);
	return KERN_SUCCESS;
}


  
                                                                                  
   
static kern_return_t
vfs_drt_search_index(struct vfs_drt_clustermap *cmap, u_int64_t offset, int *indexp)
{
	int             index;
	u_int32_t       i;

	offset = DRT_ALIGN_ADDRESS(offset);
	index = DRT_HASH(cmap, offset);

	                            
	for (i = 0; i < cmap->scm_modulus; i++) {
		  
                                        
     
		if (DRT_HASH_VACANT(cmap, index)) {
			break;
		}

		  
                                                        
     
		if (DRT_HASH_GET_ADDRESS(cmap, index) == offset) {
			*indexp = index;
			return KERN_SUCCESS;
		}

		  
                                      
     
		index = DRT_HASH_NEXT(cmap, index);
	}
	  
                   
    
	return KERN_FAILURE;
}

  
                                                                            
                                                                           
                                                                            
                                                                             
   
static kern_return_t
vfs_drt_get_index(struct vfs_drt_clustermap **cmapp, u_int64_t offset, int *indexp, int recursed)
{
	struct vfs_drt_clustermap *cmap;
	kern_return_t   kret;
	u_int32_t       index;
	u_int32_t       i;

	cmap = *cmapp;

	                                
	kret = vfs_drt_search_index(cmap, offset, indexp);
	if (kret == KERN_SUCCESS) {
		return kret;
	}

	                               
	offset = DRT_ALIGN_ADDRESS(offset);
	index = DRT_HASH(cmap, offset);

	                                                            
	for (i = 0; i < cmap->scm_modulus; i++) {
		                  
		if (DRT_HASH_VACANT(cmap, index) || DRT_HASH_GET_COUNT(cmap, index) == 0) {
			cmap->scm_buckets++;
			if (index < cmap->scm_lastclean) {
				cmap->scm_lastclean = index;
			}
			DRT_HASH_SET_ADDRESS(cmap, index, offset);
			DRT_HASH_SET_COUNT(cmap, index, 0);
			DRT_BITVECTOR_CLEAR(cmap, index);
			*indexp = index;
			vfs_drt_trace(cmap, DRT_DEBUG_INSERT, (int)offset, i, 0, 0);
			return KERN_SUCCESS;
		}
		cmap->scm_iskips += i;
		index = DRT_HASH_NEXT(cmap, index);
	}

	  
                                                                     
                                                     
    
	if (recursed) {
		return KERN_FAILURE;
	}
	kret = vfs_drt_alloc_map(cmapp);
	if (kret == KERN_SUCCESS) {
		                             
		kret = vfs_drt_get_index(cmapp, offset, indexp, 1);
	}
	return kret;
}

  
                                     
  
                                                
   
static kern_return_t
vfs_drt_do_mark_pages(
	void            **private,
	u_int64_t       offset,
	u_int           length,
	u_int           *setcountp,
	int             dirty)
{
	struct vfs_drt_clustermap *cmap, **cmapp;
	kern_return_t   kret;
	int             i, index, pgoff, pgcount, setcount, ecount;

	cmapp = (struct vfs_drt_clustermap **)private;
	cmap = *cmapp;

	vfs_drt_trace(cmap, DRT_DEBUG_MARK | DBG_FUNC_START, (int)offset, (int)length, dirty, 0);

	if (setcountp != NULL) {
		*setcountp = 0;
	}

	                                                         
	if (cmap == NULL) {
		                                      
		if (!dirty) {
			vfs_drt_trace(cmap, DRT_DEBUG_MARK | DBG_FUNC_END, 1, 0, 0, 0);
			return KERN_SUCCESS;
		}
		kret = vfs_drt_alloc_map(cmapp);
		if (kret != KERN_SUCCESS) {
			vfs_drt_trace(cmap, DRT_DEBUG_MARK | DBG_FUNC_END, 2, 0, 0, 0);
			return kret;
		}
	}
	setcount = 0;

	  
                                          
    
	while (length > 0) {
		  
                                             
    
                                                               
                              
     
		kret = vfs_drt_get_index(cmapp, offset, &index, 0);
		cmap = *cmapp;                         
		                                          
		if (kret != KERN_SUCCESS) {
			if (setcountp != NULL) {
				*setcountp = setcount;
			}
			vfs_drt_trace(cmap, DRT_DEBUG_MARK | DBG_FUNC_END, 3, (int)length, 0, 0);

			return kret;
		}

		  
                                                    
                     
     
		pgoff = (int)((offset - DRT_ALIGN_ADDRESS(offset)) / PAGE_SIZE);
		pgcount = min((length / PAGE_SIZE), (DRT_BITVECTOR_PAGES - pgoff));

		  
                                                 
     
		ecount = DRT_HASH_GET_COUNT(cmap, index);
		for (i = 0; i < pgcount; i++) {
			if (dirty) {
				if (!DRT_HASH_TEST_BIT(cmap, index, pgoff + i)) {
					if (ecount >= DRT_BITVECTOR_PAGES) {
						panic("ecount >= DRT_BITVECTOR_PAGES, cmap = %p, index = %d, bit = %d", cmap, index, pgoff + i);
					}
					DRT_HASH_SET_BIT(cmap, index, pgoff + i);
					ecount++;
					setcount++;
				}
			} else {
				if (DRT_HASH_TEST_BIT(cmap, index, pgoff + i)) {
					if (ecount <= 0) {
						panic("ecount <= 0, cmap = %p, index = %d, bit = %d", cmap, index, pgoff + i);
					}
					assert(ecount > 0);
					DRT_HASH_CLEAR_BIT(cmap, index, pgoff + i);
					ecount--;
					setcount++;
				}
			}
		}
		DRT_HASH_SET_COUNT(cmap, index, ecount);

		offset += pgcount * PAGE_SIZE;
		length -= pgcount * PAGE_SIZE;
	}
	if (setcountp != NULL) {
		*setcountp = setcount;
	}

	vfs_drt_trace(cmap, DRT_DEBUG_MARK | DBG_FUNC_END, 0, setcount, 0, 0);

	return KERN_SUCCESS;
}

  
                                      
  
                              
  
        
                                                                
                                                            
  
       
                              
  
         
                                                                     
                
  
         
                                                                      
  
            
                                                              
  
                                                                  
   
static kern_return_t
vfs_drt_mark_pages(void **cmapp, off_t offset, u_int length, u_int *setcountp)
{
	                                          
	return vfs_drt_do_mark_pages(cmapp, offset, length, setcountp, 1);
}

#if 0
static kern_return_t
vfs_drt_unmark_pages(void **cmapp, off_t offset, u_int length)
{
	return vfs_drt_do_mark_pages(cmapp, offset, length, NULL, 0);
}
#endif

  
                                
  
                              
  
        
                                                                     
                                            
  
          
                                                                          
  
          
                                                             
  
                                                                              
                                                                             
                                                               
  
   
static kern_return_t
vfs_drt_get_cluster(void **cmapp, off_t *offsetp, u_int *lengthp)
{
	struct vfs_drt_clustermap *cmap;
	u_int64_t       offset;
	u_int           length;
	u_int32_t       j;
	int             index, i, fs, ls;

	            
	if ((cmapp == NULL) || (*cmapp == NULL)) {
		return KERN_FAILURE;
	}
	cmap = *cmapp;

	                        
	for (offset = 0, j = 0; j < cmap->scm_modulus; offset += (DRT_BITVECTOR_PAGES * PAGE_SIZE), j++) {
		index = DRT_HASH(cmap, offset);

		if (DRT_HASH_VACANT(cmap, index) || (DRT_HASH_GET_COUNT(cmap, index) == 0)) {
			continue;
		}

		                                            
		fs = -1;

		for (i = 0; i < DRT_BITVECTOR_PAGES; i++) {
			if (DRT_HASH_TEST_BIT(cmap, index, i)) {
				fs = i;
				break;
			}
		}
		if (fs == -1) {
			                               
			panic("vfs_drt: entry summary count > 0 but no bits set in map, cmap = %p, index = %d, count = %lld",
			    cmap, index, DRT_HASH_GET_COUNT(cmap, index));
		}
		for (ls = 0; i < DRT_BITVECTOR_PAGES; i++, ls++) {
			if (!DRT_HASH_TEST_BIT(cmap, index, i)) {
				break;
			}
		}

		                                                 
		offset = DRT_HASH_GET_ADDRESS(cmap, index) + (PAGE_SIZE * fs);
		length = ls * PAGE_SIZE;
		vfs_drt_do_mark_pages(cmapp, offset, length, NULL, 0);
		cmap->scm_lastclean = index;

		                       
		*offsetp = (off_t)offset;
		*lengthp = length;

		vfs_drt_trace(cmap, DRT_DEBUG_RETCLUSTER, (int)offset, (int)length, 0, 0);
		return KERN_SUCCESS;
	}
	  
                                                 
                                    
                
    
	vfs_drt_trace(cmap, DRT_DEBUG_SCMDATA,
	    cmap->scm_modulus,
	    cmap->scm_buckets,
	    cmap->scm_lastclean,
	    cmap->scm_iskips);

	vfs_drt_free_map(cmap);
	*cmapp = NULL;

	return KERN_FAILURE;
}


static kern_return_t
vfs_drt_control(void **cmapp, int op_type)
{
	struct vfs_drt_clustermap *cmap;

	            
	if ((cmapp == NULL) || (*cmapp == NULL)) {
		return KERN_FAILURE;
	}
	cmap = *cmapp;

	switch (op_type) {
	case 0:
		                                  
		vfs_drt_trace(cmap, DRT_DEBUG_SCMDATA,
		    cmap->scm_modulus,
		    cmap->scm_buckets,
		    cmap->scm_lastclean,
		    cmap->scm_iskips);

		vfs_drt_free_map(cmap);
		*cmapp = NULL;
		break;

	case 1:
		cmap->scm_lastclean = 0;
		break;
	}
	return KERN_SUCCESS;
}



  
                                                                      
                                        
   
#if KDEBUG
static void
vfs_drt_trace(__unused struct vfs_drt_clustermap *cmap, int code, int arg1, int arg2, int arg3, int arg4)
{
	KERNEL_DEBUG(code, arg1, arg2, arg3, arg4, 0);
}
#else
static void
vfs_drt_trace(__unused struct vfs_drt_clustermap *cmap, __unused int code,
    __unused int arg1, __unused int arg2, __unused int arg3,
    __unused int arg4)
{
}
#endif

#if 0
  
                                                             
                                        
   
static void
vfs_drt_sanity(struct vfs_drt_clustermap *cmap)
{
	int index, i;
	int bits_on;

	for (index = 0; index < cmap->scm_modulus; index++) {
		if (DRT_HASH_VACANT(cmap, index)) {
			continue;
		}

		for (bits_on = 0, i = 0; i < DRT_BITVECTOR_PAGES; i++) {
			if (DRT_HASH_TEST_BIT(cmap, index, i)) {
				bits_on++;
			}
		}
		if (bits_on != DRT_HASH_GET_COUNT(cmap, index)) {
			panic("bits_on = %d,  index = %d\n", bits_on, index);
		}
	}
}
#endif

  
                           
   
static kern_return_t
vfs_get_scmap_push_behavior_internal(void **cmapp, int *push_flag)
{
	struct vfs_drt_clustermap *cmap;

	            
	if ((cmapp == NULL) || (*cmapp == NULL) || (push_flag == NULL)) {
		return KERN_FAILURE;
	}
	cmap = *cmapp;

	if (cmap->scm_modulus == DRT_HASH_XLARGE_MODULUS) {
		  
                                             
                                              
                                              
                                            
                                            
                   
     
		*push_flag = PUSH_ALL;
	}
	return KERN_SUCCESS;
}
