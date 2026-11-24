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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
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
 *	@(#)buf.h	8.9 (Berkeley) 3/30/95
 */

#ifndef _SYS_BUF_INTERNAL_H_
#define _SYS_BUF_INTERNAL_H_

#include <sys/appleapiopts.h>

#ifdef KERNEL
#include <sys/queue.h>
#include <sys/errno.h>
#include <sys/vm.h>
#include <sys/cdefs.h>
#include <sys/buf.h>
#include <sys/lock.h>

#if CONFIG_PROTECT
#include <sys/cprotect.h>
#endif

#define NOLIST ((struct buf *)0x87654321)

  
                                                  
   
struct bufattr {
#if CONFIG_PROTECT
	struct cpx *ba_cpx;
	uint64_t ba_cp_file_off;
#endif
	uint64_t ba_flags;                                                           
};

  
                                                              
   
struct buf {
	LIST_ENTRY(buf) b_hash;                          
	LIST_ENTRY(buf) b_vnbufs;                                       
	TAILQ_ENTRY(buf) b_freelist;                                           
	int     b_timestamp;                                                 
	struct timeval b_timestamp_tv;                                       
	int     b_whichq;                                                        
	volatile uint32_t       b_flags;                        
	volatile uint32_t       b_lflags;                                                              
	int     b_error;                                  
	int     b_bufsize;                                          
	int     b_bcount;                                           
	int     b_resid;                                    
	dev_t   b_dev;                                                      
	uintptr_t       b_datap;                                               
	daddr64_t       b_lblkno;                                  
	daddr64_t       b_blkno;                                               
	void    (*b_iodone)(buf_t, void *);                                            
	vnode_t b_vp;                                                                        
	kauth_cred_t b_rcred;                                            
	kauth_cred_t b_wcred;                                             
	void *  b_upl;                                      
	buf_t   b_real_bp;                                                                 
	TAILQ_ENTRY(buf)        b_act;                                       
	void *  b_drvdata;                                             
	void *  b_fsprivate;                                        
	void *  b_transaction;                                   
	int     b_dirtyoff;                                                    
	int     b_dirtyend;                                                 
	int     b_validoff;                                                    
	int     b_validend;                                                 

	                                                                 
                                     
    
	uint32_t b_redundancy_flags;

	proc_t  b_proc;                                                       
#ifdef BUF_MAKE_PRIVATE
	buf_t   b_data_store;
#endif
	struct bufattr b_attr;
#ifdef JOE_DEBUG
	void *  b_owner;
	int     b_tag;
	void *  b_lastbrelse;
	void *  b_stackbrelse[6];
	void *  b_stackgetblk[6];
#endif
};

extern vm_offset_t buf_kernel_addrperm;

                                                 
#define b_uploffset  b_bufsize
#define b_orig       b_freelist.tqe_prev
#define b_shadow     b_freelist.tqe_next
#define b_shadow_ref b_validoff
#ifdef BUF_MAKE_PRIVATE
#define b_data_ref   b_validend
#endif
#define b_trans_head b_freelist.tqe_prev
#define b_trans_next b_freelist.tqe_next
#define b_iostate    b_rcred
#define b_cliodone   b_wcred

  
                                      
                                                 
   
#define BL_BUSY         0x00000001                            
#define BL_WANTED       0x00000002                                      
#define BL_IOBUF        0x00000004                                            
#define BL_WANTDEALLOC  0x00000010                                                         
#define BL_SHADOW       0x00000020
#define BL_EXTERNAL     0x00000040
#define BL_WAITSHADOW   0x00000080
#define BL_IOBUF_ALLOC  0x00000100
#define BL_WANTED_REF   0x00000200
#define BL_IOBUF_VDEV   0x00000400                                     

  
                                                 
   
#define BUF_STALE_THRESHHOLD    30                                                       
#define BUF_MAX_GC_BATCH_SIZE   64                                           

  
                                                                  
   
#define BUF_X_RDFLAGS (B_PHYS | B_RAW | B_LOCKED | B_ASYNC | B_READ | B_WRITE | B_PAGEIO |\
	               B_META | B_CLUSTER | B_DELWRI | B_FUA | B_PASSIVE | B_IOSTREAMING |\
	               B_ENCRYPTED_IO | B_STATICCONTENT)
  
                                                                                    
   
#define BUF_X_WRFLAGS (B_PHYS | B_RAW | B_LOCKED | B_ASYNC | B_READ | B_WRITE | B_PAGEIO |\
	               B_NOCACHE | B_FUA | B_PASSIVE | B_IOSTREAMING)

#if 0
                              
#define B_WRITE         0x00000000                                       
#define B_READ          0x00000001                        
#define B_ASYNC         0x00000002                                   
#define B_NOCACHE       0x00000004                                         
#define B_DELWRI        0x00000008                                          
#define B_LOCKED        0x00000010                                          
#define B_PHYS          0x00000020                               
#define B_CLUSTER       0x00000040                                                    
#define B_PAGEIO        0x00000080                       
#define B_META          0x00000100                                      
#define B_RAW           0x00000200                                            
#define B_FUA           0x00000400                                                  
#define B_PASSIVE       0x00000800                                                    
#define B_IOSTREAMING   0x00001000                                              
#define B_ENCRYPTED_IO  0x00004000                         
#define B_STATICCONTENT 0x00008000                                                
#endif

  
                                                        
                                                         
                     
  
                                                          
   
#define B_CACHE         0x00010000                                         
#define B_DONE          0x00020000                          
#define B_INVAL         0x00040000                                        
#define B_ERROR         0x00080000                               
#define B_EINTR         0x00100000                               
#define B_AGE           0x00200000                                            
#define B_FILTER        0x00400000                                                           
#define B_CALL          0x00800000                                                                    
#define B_EOT           0x01000000                                                                   
#define B_WASDIRTY      0x02000000                                                
#define B_HDRALLOC      0x04000000                                        
#define B_ZALLOC        0x08000000                                 
  
                                             
   
#define B_COMMIT_UPL    0x40000000                                                       
#define B_TDONE         0x80000000                                                                           

                                             
#define B_CLRBUF        0x01                                              
#define B_SYNC          0x02                                           
#define B_NOBUFF        0x04                                    

  
                                    
                                                        
   
#define BA_RAW_ENCRYPTED_IO     0x00000001
#define BA_THROTTLED_IO         0x00000002
#define BA_DELAYIDLESLEEP       0x00000004                                                            
#define BA_NOCACHE              0x00000008
#define BA_META                 0x00000010
#define BA_GREEDY_MODE          0x00000020                                                       
#define BA_QUICK_COMPLETE       0x00000040                                                                     
#define BA_PASSIVE              0x00000080

  
                                                        
                                                  
   
#define BA_IO_TIER_MASK         0x00000f00
#define BA_IO_TIER_SHIFT        8

#define BA_ISOCHRONOUS          0x00001000                                                      

#define BA_STRATEGY_TRACKED_IO  0x00002000                               
#define BA_IO_TIER_UPGRADE      0x00004000                                                   
#define BA_IO_SCHEDULED         0x00008000                                                                
#define BA_EXPEDITED_META_IO    0x00010000                                               

#define GET_BUFATTR_IO_TIER(bap)        ((bap->ba_flags & BA_IO_TIER_MASK) >> BA_IO_TIER_SHIFT)
#define SET_BUFATTR_IO_TIER(bap, tier)                                          \
do {                                                                            \
	(bap)->ba_flags &= (~BA_IO_TIER_MASK);                                  \
	(bap)->ba_flags |= (((tier) << BA_IO_TIER_SHIFT) & BA_IO_TIER_MASK);    \
} while(0)

extern int niobuf_headers;                                                                  
extern int nbuf_headers;                                                  
extern int max_nbuf_headers;                                                  
extern int nbuf_hashelements;                                                  
extern struct buf *buf_headers;                                  


  
                                         
   

enum bq_opts {
	BQ_LOCKED   = 0,                       
	BQ_LRU      = 1,                           
	BQ_AGE      = 2,               
	BQ_EMPTY    = 3,                                     
	BQ_META     = 4,                                  
	BQ_LAUNDRY  = 5,                                  
	BQUEUES     = 6                                     
};


__BEGIN_DECLS

buf_t   alloc_io_buf(vnode_t, int);
void    free_io_buf(buf_t);

int     allocbuf(struct buf *, int);
void    bufinit(void);

void    buf_list_lock(void);
void    buf_list_unlock(void);

void    cluster_init(void);

uint32_t     count_busy_buffers(void);

int buf_flushdirtyblks_skipinfo(vnode_t, int, int, const char *);
void buf_wait_for_shadow_io(vnode_t, daddr64_t);

#ifdef BUF_MAKE_PRIVATE
errno_t buf_make_private(buf_t bp);
#endif

#ifdef CONFIG_PROTECT
void buf_setcpoff(buf_t, uint64_t);
#endif

__END_DECLS


  
                                          
   
struct bufstats {
	long    bufs_incore;                              
	long    bufs_busyincore;                                    
	long    bufs_vmhits;                                         
	long    bufs_miss;                                                 
	long    bufs_sleeps;                                   
	long    bufs_eblk;                                            
	uint32_t    bufs_iobufmax;                                              
	uint32_t    bufs_iobufinuse;                                         
	long    bufs_iobufsleeps;                                 
	long    bufs_iobufinuse_vdev;                                    
                                                  
};

#endif             
#endif                   
