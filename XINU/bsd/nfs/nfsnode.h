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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
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
 *	@(#)nfsnode.h	8.9 (Berkeley) 5/14/95
 * FreeBSD-Id: nfsnode.h,v 1.24 1997/10/28 14:06:25 bde Exp $
 */


#ifndef _NFS_NFSNODE_H_
#define _NFS_NFSNODE_H_

#include <sys/appleapiopts.h>

#ifdef __APPLE_API_PRIVATE
#ifndef _NFS_NFS_H_
#include <nfs/nfs.h>
#endif
#include <sys/kauth.h>
#include <os/refcnt.h>

  
                                                                   
                                        
   
struct nfs_sillyrename {
	kauth_cred_t    nsr_cred;
	struct nfsnode  *nsr_dnp;
	int             nsr_namlen;
	char            nsr_name[20];
};

  
                       
   
typedef struct {
	uint64_t               pages[8];
} nfsbufpgs;

  
                                                    
   
struct nfsbuf {
	LIST_ENTRY(nfsbuf)      nb_hash;                        
	LIST_ENTRY(nfsbuf)      nb_vnbufs;                                  
	TAILQ_ENTRY(nfsbuf)     nb_free;                                               
	os_refcnt_t             nb_refs;                                     
	daddr64_t               nb_lblkno;                                 
	uint64_t                nb_verf;                               
	time_t                  nb_timestamp;                         
	nfsbufpgs               nb_valid;                               
	nfsbufpgs               nb_dirty;                               
	caddr_t                 nb_data;                           
	nfsnode_t               nb_np;                                         
	kauth_cred_t            nb_rcred;                                       
	kauth_cred_t            nb_wcred;                                        
	void *                  nb_pagelist;             
	volatile uint32_t       nb_flags;                        
	volatile uint32_t       nb_lflags;                        
	uint32_t                nb_bufsize;                      
	int                     nb_error;                         
	int                     nb_commitlevel;                                
	off_t                   nb_validoff;                                           
	off_t                   nb_validend;                                        
	off_t                   nb_dirtyoff;                                           
	off_t                   nb_dirtyend;                                        
	off_t                   nb_offio;                                            
	off_t                   nb_endio;                                         
	uint64_t                nb_rpcs;                                                      
};

#define NFS_MAXBSIZE    (8 * 64 * PAGE_SIZE)                                                  

#define NFS_A_LOT_OF_NEEDCOMMITS        256                                                              
#define NFS_A_LOT_OF_DELAYED_WRITES     MAX(nfsbufcnt/8,512)                                         

  
                                      
                                                       
   
#define NBL_BUSY        0x00000001                            
#define NBL_WANTED      0x00000002                                      

  
                                                              
                                                
                                                       
   
#define NB_STALEWVERF   0x00000001                                        
#define NB_NEEDCOMMIT   0x00000002                                        
#define NB_ASYNC        0x00000004                                   
#define NB_CACHE        0x00000020                                          
#define NB_STABLE       0x00000040                                       
#define NB_DELWRI       0x00000080                                                          
#define NB_DONE         0x00000200                          
#define NB_EINTR        0x00000400                               
#define NB_ERROR        0x00000800                               
#define NB_INVAL        0x00002000                                        
#define NB_NCRDAHEAD    0x00004000                                    
#define NB_NOCACHE      0x00008000                                         
#define NB_WRITE        0x00000000                                       
#define NB_READ         0x00100000                        
#define NB_MULTASYNCRPC 0x00200000                                                 
#define NB_PAGELIST     0x00400000                                          
#define NB_WRITEINPROG  0x01000000                              
#define NB_META         0x40000000                                      

                                               
#define NBLK_READ       0x00000001                           
#define NBLK_WRITE      0x00000002                            
#define NBLK_META       0x00000004                               
#define NBLK_OPMASK     0x00000007                          
                                  
#define NBLK_NOWAIT     0x40000000                                     
#define NBLK_ONLYVALID  0x80000000                                     

                                               
#define NBI_ITER                0x01                               
#define NBI_ITERWANT            0x02                            
#define NBI_CLEAN               0x04                                  
#define NBI_DIRTY               0x08                                  
#define NBI_NOWAIT              0x10                                 

                               
#define NBAC_NOWAIT             0x01                                      
#define NBAC_REMOVE             0x02                                                       

                          
#define NBPGS_STRUCT_SIZE               sizeof(nfsbufpgs)
#define NBPGS_ELEMENT_SIZE              sizeof(((nfsbufpgs *)0)->pages[0])
#define NBPGS_ELEMENT_PAGES             (8 * NBPGS_ELEMENT_SIZE)
#define NBPGS_ELEMENTS                  (NBPGS_STRUCT_SIZE / NBPGS_ELEMENT_SIZE)
#define NBPGS_ERASE(NFSBP)              bzero((NFSBP), NBPGS_STRUCT_SIZE)
#define NBPGS_COPY(NFSBPDST, NFSBPSRC)  memcpy((NFSBPDST), (NFSBPSRC), NBPGS_STRUCT_SIZE)
#define NBPGS_IS_EQUAL(NFSBP1, NFSBP2)  (memcmp((NFSBP1), (NFSBP2), NBPGS_STRUCT_SIZE) == 0)
#define NBPGS_GET(NFSBP, P)             ((NFSBP)->pages[((P)/NBPGS_ELEMENT_PAGES)] & (1LLU << ((P) % NBPGS_ELEMENT_PAGES)))
#define NBPGS_SET(NFSBP, P)             ((NFSBP)->pages[((P)/NBPGS_ELEMENT_PAGES)] |= (1LLU << ((P) % NBPGS_ELEMENT_PAGES)))
#define NBPGS_UNSET(NFSBP, P)           ((NFSBP)->pages[((P)/NBPGS_ELEMENT_PAGES)] &= ~(1LLU << ((P) % NBPGS_ELEMENT_PAGES)))

                                 
#define NBOFF(BP)                       ((off_t)(BP)->nb_lblkno * (off_t)(BP)->nb_bufsize)
#define NBPGVALID(BP, P)                NBPGS_GET(&((BP)->nb_valid), (P))
#define NBPGDIRTY(BP, P)                NBPGS_GET(&((BP)->nb_dirty), (P))
#define NBPGVALID_SET(BP, P)            NBPGS_SET(&((BP)->nb_valid), (P))
#define NBPGDIRTY_SET(BP, P)            NBPGS_SET(&((BP)->nb_dirty), (P))

#define NBUFSTAMPVALID(BP)              ((BP)->nb_timestamp != ~0)
#define NBUFSTAMPINVALIDATE(BP)         ((BP)->nb_timestamp = ~0)

#define NFS_BUF_MAP(BP) \
	do { \
	        if (!(BP)->nb_data && nfs_buf_map(BP)) \
	                panic("nfs_buf_map failed"); \
	} while (0)

LIST_HEAD(nfsbuflists, nfsbuf);
TAILQ_HEAD(nfsbuffreehead, nfsbuf);

extern lck_mtx_t nfs_buf_mutex;
extern int nfsbufcnt, nfsbufmin, nfsbufmax, nfsbufmetacnt, nfsbufmetamax;
extern int nfsbuffreecnt, nfsbuffreemetacnt, nfsbufdelwricnt, nfsneedbuffer;
extern int nfs_nbdwrite;
extern struct nfsbuffreehead nfsbuffree, nfsbufdelwri;

#ifdef NFSBUFDEBUG
#define NFSBUFCNTCHK() \
	do { \
	if (	(nfsbufcnt < 0) || \
	        (nfsbufcnt > nfsbufmax) || \
	        (nfsbufmetacnt < 0) || \
	        (nfsbufmetacnt > nfsbufmetamax) || \
	        (nfsbufmetacnt > nfsbufcnt) || \
	        (nfsbuffreecnt < 0) || \
	        (nfsbuffreecnt > nfsbufmax) || \
	        (nfsbuffreecnt > nfsbufcnt) || \
	        (nfsbuffreemetacnt < 0) || \
	        (nfsbuffreemetacnt > nfsbufmax) || \
	        (nfsbuffreemetacnt > nfsbufcnt) || \
	        (nfsbuffreemetacnt > nfsbufmetamax) || \
	        (nfsbuffreemetacnt > nfsbufmetacnt) || \
	        (nfsbufdelwricnt < 0) || \
	        (nfsbufdelwricnt > nfsbufmax) || \
	        (nfsbufdelwricnt > nfsbufcnt) || \
	        (nfs_nbdwrite < 0) || \
	        (nfs_nbdwrite > nfsbufcnt) || \
	        0) \
	        panic("nfsbuf count error: max %d meta %d cnt %d meta %d free %d meta %d delwr %d bdw %d\n", \
	                nfsbufmax, nfsbufmetamax, nfsbufcnt, nfsbufmetacnt, nfsbuffreecnt, nfsbuffreemetacnt, \
	                nfsbufdelwricnt, nfs_nbdwrite); \
	} while (0)
#else
#define NFSBUFCNTCHK()
#endif

  
                       
  
                                           
  
                   
                                         
                                                             
                                     
                      
                                                   
                                                          
   
struct nfs_dir_buf_header {
	uint16_t        ndbh_flags;                            
	uint16_t        ndbh_count;                       
	off_t           ndbh_entry_end;                                  
	uint32_t        ndbh_ncgen;                                 
	uint32_t        ndbh_pad;                     
};
                
#define NDB_FULL        0x0001                              
#define NDB_EOF         0x0002                           
#define NDB_PLUS        0x0004                                     

#define NFS_DIR_BUF_FIRST_DIRENTRY(BP) \
	((struct direntry*)((char*)((BP)->nb_data) + sizeof(*ndbhp)))
#define NFS_DIR_BUF_NVATTR(BP, IDX) \
	(&((struct nfs_vattr*)((char*)((BP)->nb_data) + (BP)->nb_bufsize))[-((IDX)+1)])
#define NFS_DIRENTRY_LEN(namlen) \
	((sizeof(struct direntry) + (namlen) - (MAXPATHLEN-1) + 7) & ~7)
#define NFS_DIRENTRY_LEN_16(namlen) \
	        (uint16_t)(NFS_DIRENTRY_LEN(namlen)); assert((namlen) <= UINT16_MAX);
#define NFS_DIRENT_LEN(namlen) \
	((sizeof(struct dirent) - (NAME_MAX+1)) + (((namlen) + 1 + 3) &~ 3))
#define NFS_DIRENTRY_NEXT(DP) \
	((struct direntry*)((char*)(DP) + (DP)->d_reclen))
#define NFS_DIR_COOKIE_POTENTIALLY_TRUNCATED(C) \
	((C) && ((((C) >> 32) == 0) || (((C) & 0x80000000ULL) && (((C) >> 32) == 0xffffffff))))
#define NFS_DIR_COOKIE_SAME32(C1, C2) \
	(((C1) & 0xffffffffULL) == ((C2) & 0xffffffffULL))

  
                             
  
                                                                
                                                                      
                                                                      
                                                                       
                                                                
   
#define NFSNUMCOOKIES           14
struct nfsdmap {
	int8_t          free;                                         
	int8_t          mru;                                          
	int8_t          next[NFSNUMCOOKIES];                        
	struct {
		uint64_t    key;                            
		uint64_t    lbn;                                   
	} cookies[NFSNUMCOOKIES];                                     
};

  
                                
   
#define NFSTIME_ACCESS  0                                
#define NFSTIME_MODIFY  1                                      
#define NFSTIME_CHANGE  2                              
#define NFSTIME_CREATE  3                              
#define NFSTIME_BACKUP  4                                
#define NFSTIME_ADDED   5                                    
#define NFSTIME_COUNT   6

#define NFS_COMPARE_MTIME(TVP, NVAP, CMP) \
	(((TVP)->tv_sec == (NVAP)->nva_timesec[NFSTIME_MODIFY]) ?       \
	 ((TVP)->tv_nsec CMP (NVAP)->nva_timensec[NFSTIME_MODIFY]) :    \
	 ((TVP)->tv_sec CMP (NVAP)->nva_timesec[NFSTIME_MODIFY]))
#define NFS_COPY_TIME(TVP, NVAP, WHICH) \
	do { \
	(TVP)->tv_sec = (NVAP)->nva_timesec[NFSTIME_##WHICH]; \
	(TVP)->tv_nsec = (NVAP)->nva_timensec[NFSTIME_##WHICH]; \
	} while (0)

struct nfs_vattr {
	enum vtype      nva_type;                                    
	mode_t          nva_mode;                                          
	uid_t           nva_uid;                           
	gid_t           nva_gid;                            
	guid_t          nva_uuuid;                           
	guid_t          nva_guuid;                            
	kauth_acl_t     nva_acl;                                 
	nfs_specdata    nva_rawdev;                                             
	uint32_t        nva_flags;                                  
	uint32_t        nva_maxlink;                                 
	uint64_t        nva_nlink;                                        
	uint64_t        nva_fileid;                  
	nfs_fsid        nva_fsid;                           
	uint64_t        nva_size;                               
	uint64_t        nva_bytes;                                            
	uint64_t        nva_change;                           
	time_t          nva_timesec[NFSTIME_COUNT];
	long            nva_timensec[NFSTIME_COUNT];
	uint32_t        nva_bitmap[NFS_ATTR_BITMAP_LEN];                                

	                 
	uint32_t        nva_bsd_flags;                 
	uint64_t        nva_parentid;                       
	uint64_t        nva_allocsize;                              
};

               
#define NFS_FFLAG_ARCHIVED              0x0001
#define NFS_FFLAG_HIDDEN                0x0002
#define NFS_FFLAG_HAS_NAMED_ATTRS       0x0004                                 
#define NFS_FFLAG_TRIGGER               0x0008                                            
#define NFS_FFLAG_TRIGGER_REFERRAL      0x0010                             
#define NFS_FFLAG_PARTIAL_WRITE         0x0020                                          
#define NFS_FFLAG_FILEID_CONTAINS_XID   0x0040                                                                 
#define NFS_FFLAG_IS_ATTR               0x8000                                                
                
#define NFS_FFLAG_FPNFS_BSD_FLAGS   0x01000000
#define NFS_FFLAG_FPNFS_PARENTID    0x02000000
#define NFS_FFLAG_FPNFS_ADDEDTIME   0x04000000

                             
#define NGA_CACHED      0x0001                                              
#define NGA_UNCACHED    0x0002                            
#define NGA_ACL         0x0004                 
#define NGA_MONITOR     0x0008                                      
#define NGA_SOFT        0x0010                                         

                                                          
#define NVATTR_INIT(NVAP) \
	do { \
	        NFS_CLEAR_ATTRIBUTES((NVAP)->nva_bitmap); \
	        (NVAP)->nva_flags = 0; \
	        (NVAP)->nva_acl = NULL; \
	} while (0)
#define NVATTR_CLEANUP(NVAP) \
	do { \
	        NFS_CLEAR_ATTRIBUTES((NVAP)->nva_bitmap); \
	        if ((NVAP)->nva_acl) { \
	                kauth_acl_free((NVAP)->nva_acl); \
	                (NVAP)->nva_acl = NULL; \
	        } \
	} while (0)

  
                                    
  
                                                                              
                                                             
  
                                               
                                           
   
#define NFS_CHANGED(VERS, NP, NVAP) \
	        (((VERS) >= NFS_VER4) ? \
	                ((NP)->n_change != (NVAP)->nva_change) : \
	                NFS_COMPARE_MTIME(&(NP)->n_mtime, (NVAP), !=))
#define NFS_CHANGED_NC(VERS, NP, NVAP) \
	        (((VERS) >= NFS_VER4) ? \
	                ((NP)->n_ncchange != (NVAP)->nva_change) : \
	                NFS_COMPARE_MTIME(&(NP)->n_ncmtime, (NVAP), !=))
#define NFS_CHANGED_UPDATE(VERS, NP, NVAP) \
	do { \
	        if ((VERS) >= NFS_VER4) \
	                (NP)->n_change = (NVAP)->nva_change; \
	        else \
	                NFS_COPY_TIME(&(NP)->n_mtime, (NVAP), MODIFY); \
	} while (0)
#define NFS_CHANGED_UPDATE_NC(VERS, NP, NVAP) \
	do { \
	        if ((VERS) >= NFS_VER4) \
	                (NP)->n_ncchange = (NVAP)->nva_change; \
	        else \
	                NFS_COPY_TIME(&(NP)->n_ncmtime, (NVAP), MODIFY); \
	} while (0)


extern lck_grp_t nfs_open_grp;
extern uint32_t nfs_open_owner_seqnum, nfs_lock_owner_seqnum;

  
                                                      
   
struct nfs_open_owner {
	TAILQ_ENTRY(nfs_open_owner)     noo_link;                                           
	lck_mtx_t                       noo_lock;                        
	struct nfsmount *               noo_mount;                     
	os_refcnt_t                     noo_refcnt;                                   
	uint32_t                        noo_flags;                     
	kauth_cred_t                    noo_cred;                                      
	uint32_t                        noo_name;                                 
	uint32_t                        noo_seqid;                                   
	TAILQ_HEAD(, nfs_open_file)      noo_opens;                              
};
               
#define NFS_OPEN_OWNER_LINK     0x1                                              
#define NFS_OPEN_OWNER_BUSY     0x2                                                     
#define NFS_OPEN_OWNER_WANT     0x4                                          

  
                                                           
   
struct nfs_open_file {
	lck_mtx_t                       nof_lock;                                    
	TAILQ_ENTRY(nfs_open_file)      nof_link;                                       
	TAILQ_ENTRY(nfs_open_file)      nof_oolink;                                                  
	struct nfs_open_owner *         nof_owner;                              
	nfsnode_t                       nof_np;                                               
	nfs_stateid                     nof_stateid;                              
	thread_t                        nof_creator;                                          
	uint32_t                        nof_opencnt;                                 
	uint16_t                        nof_flags;                             
	uint8_t                         nof_access:4;                                          
	uint8_t                         nof_deny:4;                                          
	uint8_t                         nof_mmap_access:4;                                 
	uint8_t                         nof_mmap_deny:4;                                 
	                                                  
	uint32_t                        nof_r;                                              
	uint32_t                        nof_w;                                               
	uint32_t                        nof_rw;                                                   
	uint32_t                        nof_r_dw;                                          
	                                                                      
	uint32_t                        nof_w_dw:2;                                                 
	uint32_t                        nof_rw_dw:2;                                                     
	uint32_t                        nof_r_drw:2;                                                    
	uint32_t                        nof_w_drw:2;                                                     
	uint32_t                        nof_rw_drw:2;                                                         
	                                                            
	uint32_t                        nof_d_w_dw:2;                                               
	uint32_t                        nof_d_rw_dw:2;                                                   
	uint32_t                        nof_d_r_drw:2;                                                  
	uint32_t                        nof_d_w_drw:2;                                                   
	uint32_t                        nof_d_rw_drw:2;                                                       
	uint32_t                        nof_d_r;                                            
	uint32_t                        nof_d_w;                                             
	uint32_t                        nof_d_rw;                                                 
	uint32_t                        nof_d_r_dw;                                        
};
               
#define NFS_OPEN_FILE_BUSY      0x0001                                                  
#define NFS_OPEN_FILE_WANT      0x0002                                       
#define NFS_OPEN_FILE_CREATE    0x0004                                            
#define NFS_OPEN_FILE_NEEDCLOSE 0x0008                                                                   
#define NFS_OPEN_FILE_SETATTR   0x0020                                                 
#define NFS_OPEN_FILE_POSIXLOCK 0x0040                                               
#define NFS_OPEN_FILE_LOST      0x0080                                
#define NFS_OPEN_FILE_REOPEN    0x0100                                 
#define NFS_OPEN_FILE_REOPENING 0x0200                              

struct nfs_lock_owner;
  
                
  
                                                
                                                  
   
struct nfs_file_lock {
	TAILQ_ENTRY(nfs_file_lock)      nfl_link;                                     
	TAILQ_ENTRY(nfs_file_lock)      nfl_lolink;                                       
	struct nfs_lock_owner *         nfl_owner;                                           
	uint64_t                        nfl_start;                           
	uint64_t                        nfl_end;                                       
	uint32_t                        nfl_blockcnt;                                     
	uint16_t                        nfl_flags;                     
	uint16_t                        nfl_type;                                  
};
               
#define NFS_FILE_LOCK_ALLOC             0x01                            
#define NFS_FILE_LOCK_STYLE_POSIX       0x02                                  
#define NFS_FILE_LOCK_STYLE_FLOCK       0x04                             
#define NFS_FILE_LOCK_STYLE_MASK        0x06                         
#define NFS_FILE_LOCK_WAIT              0x08                                        
#define NFS_FILE_LOCK_BLOCKED           0x10                            
#define NFS_FILE_LOCK_DEAD              0x20                                         
#define NFS_FILE_LOCK_DELEGATED         0x40                                      

TAILQ_HEAD(nfs_file_lock_queue, nfs_file_lock);

  
                                                      
                                                        
                                                          
   
#define NFS_FLOCK_LENGTH(S, E)  (((E) == UINT64_MAX) ? 0 : ((E) - (S) + 1))
#define NFS_LOCK_LENGTH(S, E)   (((E) == UINT64_MAX) ? UINT64_MAX : ((E) - (S) + 1))

  
                                                                      
  
                                          
  
                                                                     
                                                                     
                                                                   
                                                   
   
struct nfs_lock_owner {
	lck_mtx_t                       nlo_lock;                        
	TAILQ_ENTRY(nfs_lock_owner)     nlo_link;                                             
	struct nfs_open_owner *         nlo_open_owner;                               
	struct nfs_file_lock_queue      nlo_locks;                              
	struct nfs_file_lock            nlo_alock;                                                
	struct timeval                  nlo_pid_start;                                
	pid_t                           nlo_pid;                                    
	os_refcnt_t                     nlo_refcnt;                                   
	uint32_t                        nlo_flags;                     
	uint32_t                        nlo_name;                                 
	uint32_t                        nlo_seqid;                                   
	uint32_t                        nlo_stategenid;                                
	nfs_stateid                     nlo_stateid;                      
};
               
#define NFS_LOCK_OWNER_LINK     0x1                                              
#define NFS_LOCK_OWNER_BUSY     0x2                                                     
#define NFS_LOCK_OWNER_WANT     0x4                                          

  
                                                 
                                                
                                                              
                                                                             
                                                      
   

#define NFS_ACCESS_CACHE_SIZE   3

struct nfsnode {
	lck_mtx_t               n_lock;                            
	lck_rw_t                n_datalock;                             
	void                    *n_datalockowner;                                          
	LIST_ENTRY(nfsnode)     n_hash;                         
	LIST_ENTRY(nfsnode)     n_monlink;                                   
	u_quad_t                n_size;                                   
	u_quad_t                n_newsize;                                             
	u_int64_t               n_xid;                                    
	struct nfs_vattr        n_vattr;                                   
	time_t                  n_attrstamp;                               
	time_t                  n_aclstamp;                              
	time_t                  n_evtstamp;                                     
	uint32_t                n_events;                                 
	uint32_t                n_access[NFS_ACCESS_CACHE_SIZE + 1];                        
	uid_t                   n_accessuid[NFS_ACCESS_CACHE_SIZE];                                    
	time_t                  n_accessstamp[NFS_ACCESS_CACHE_SIZE];                               
	time_t                  n_rdirplusstamp_sof;                                
	time_t                  n_rdirplusstamp_eof;                                
	union {
		struct {
			struct timespec n3_mtime;                        
			struct timespec n3_ncmtime;                             
		} v3;
		struct {
			uint64_t        n4_change;                           
			uint64_t        n4_ncchange;                                
			u_char          *n4_attrdirfh;                                  
			struct timeval  n4_lastio;                                     
		} v4;
	} n_un4;
	vnode_t                 n_parent;                               
	u_char                  *n_fhp;                              
	vnode_t                 n_vnode;                              
	mount_t                 n_mount;                                       
	int                     n_error;                                    
	union {
		struct timespec ns_atim;                                
		struct timespec nl_rltim;                                  
		daddr64_t       nf_lastread;                                               
		uint64_t        nd_cookieverf;                                  
	} n_un1;
	union {
		struct timespec ns_mtim;                                
		daddr64_t       nf_lastrahead;                              
		uint64_t        nd_eofcookie;                              
	} n_un2;
	union {
		struct nfs_sillyrename *nf_silly;                                
		struct nfsdmap *nd_cookiecache;                       
	} n_un3;
	uint32_t                n_flag;                         
	uint32_t                n_fhsize;                                 
	u_short                 n_hflag;                             
	u_short                 n_bflag;                               
	u_short                 n_mflag;                              
	u_char                  n_fh[NFS_SMALLFH];                       
	uint32_t                n_auth;                                                 
	struct nfsbuflists      n_cleanblkhd;                             
	struct nfsbuflists      n_dirtyblkhd;                             
	union {
		int             nf_wrbusy;                                    
		uint32_t        nd_ncgen;                                       
	} n_un5;
	union {
		int             nf_needcommitcnt;                                 
		daddr64_t       nd_lastdbl;                                     
	} n_un6;
	int                     n_bufiterflags;                         
	union {
		int             nf_numoutput;                               
		int             nd_trigseq;                             
	} n_un7;
	                
	lck_mtx_t               n_openlock;                             
	uint32_t                n_openflags;                          
	uint32_t                n_openrefcnt;                         
	TAILQ_HEAD(, nfs_open_file) n_opens;                             
	                
	TAILQ_HEAD(, nfs_lock_owner) n_lock_owners;                          
	struct nfs_file_lock_queue n_locks;                        
	                      
	nfs_stateid             n_dstateid;                             
	TAILQ_ENTRY(nfsnode)    n_dlink;                                  
	TAILQ_ENTRY(nfsnode)    n_dreturn;                                       
	struct kauth_ace        n_dace;                             
};

#define NFS_DATA_LOCK_SHARED    1
#define NFS_DATA_LOCK_EXCLUSIVE 2

#define nfstimespeccmp(tvp, uvp, cmp)           \
	(((tvp)->tv_sec == (uvp)->tv_sec) ?     \
	 ((tvp)->tv_nsec cmp (uvp)->tv_nsec) :  \
	 ((tvp)->tv_sec cmp (uvp)->tv_sec))

#define CHECK_NEEDCOMMITCNT(np) \
	do { \
	        if ((np)->n_needcommitcnt < 0) { \
	                printf("nfs: n_needcommitcnt negative\n"); \
	                (np)->n_needcommitcnt = 0; \
	        } \
	} while (0)

#define n_atim                  n_un1.ns_atim
#define n_mtim                  n_un2.ns_mtim
#define n_rltim                 n_un1.nl_rltim
#define n_lastread              n_un1.nf_lastread
#define n_lastrahead            n_un2.nf_lastrahead
#define n_sillyrename           n_un3.nf_silly
#define n_wrbusy                n_un5.nf_wrbusy
#define n_needcommitcnt         n_un6.nf_needcommitcnt
#define n_numoutput             n_un7.nf_numoutput
#define n_cookieverf            n_un1.nd_cookieverf
#define n_eofcookie             n_un2.nd_eofcookie
#define n_cookiecache           n_un3.nd_cookiecache
#define n_ncgen                 n_un5.nd_ncgen
#define n_lastdbl               n_un6.nd_lastdbl
#define n_trigseq               n_un7.nd_trigseq
#define n_mtime                 n_un4.v3.n3_mtime
#define n_ncmtime               n_un4.v3.n3_ncmtime
#define n_change                n_un4.v4.n4_change
#define n_ncchange              n_un4.v4.n4_ncchange
#define n_attrdirfh             n_un4.v4.n4_attrdirfh
#define n_lastio                n_un4.v4.n4_lastio

  
                   
   
#define NUPDATESIZE     0x00001                                  
#define NREVOKE         0x00002                   
#define NMODIFIED       0x00004                                          
#define NWRITEERR       0x00008                                           
#define NNEEDINVALIDATE 0x00010                               
#define NGETATTRINPROG  0x00020                              
#define NGETATTRWANT    0x00040                              
#define NACC            0x00100                            
#define NUPD            0x00200                           
#define NCHG            0x00400                                 
#define NNEGNCENTRIES   0x00800                                                
#define NBUSY           0x01000                   
#define NBUSYWANT       0x02000                           
#define NISDOTZFS       0x04000                         
#define NISDOTZFSCHILD  0x08000                                    
#define NISMAPPED       0x10000                        
#define NREFRESH        0x20000                                      
#define NREFRESHWANT    0x40000                                     
#define NDISARMTRIGGER  0x80000                                         

  
                    
                                         
   
#define NHHASHED        0x0001                             
#define NHINIT          0x0002                                 
#define NHLOCKED        0x0004                                             
#define NHLOCKWANT      0x0008                             

  
                    
                                   
   
#define NBFLUSHINPROG   0x0001                                           
#define NBFLUSHWANT     0x0002                                           
#define NBINVALINPROG   0x0004                                               
#define NBINVALWANT     0x0008                                               

  
                    
                                        
   
#define NMMONSCANINPROG 0x0001                                                       
#define NMMONSCANWANT   0x0002                                                

  
              
                                
   
#define N_OPENBUSY              0x0001                                          
#define N_OPENWANT              0x0002                                  
#define N_DELEG_READ            0x0004                                 
#define N_DELEG_WRITE           0x0008                                  
#define N_DELEG_MASK            0x000c                       
#define N_DELEG_RETURN          0x0010                                    
#define N_DELEG_RETURNING       0x0020                                 

                                            
#define NATTRVALID(np)          ((np)->n_attrstamp != ~0)
#define NATTRINVALIDATE(np)     ((np)->n_attrstamp = ~0)
#define NACCESSVALID(np, slot)  (((slot) >= 0) && ((slot) < NFS_ACCESS_CACHE_SIZE) && ((np)->n_accessstamp[(slot)] != ~0))
#define NACCESSINVALIDATE(np) \
	do { \
	        int __i; \
	        for (__i=0; __i < NFS_ACCESS_CACHE_SIZE; __i++) \
	                (np)->n_accessstamp[__i] = ~0; \
	        (np)->n_access[NFS_ACCESS_CACHE_SIZE] = 0; \
	} while (0)
#define NACLVALID(np)           ((np)->n_aclstamp != ~0)
#define NACLINVALIDATE(np)      ((np)->n_aclstamp = ~0)

  
                                                 
   
#define V_IGNORE_WRITEERR       0x8000

  
                       
   
#define NG_MARKROOT     0x0001                                
#define NG_MAKEENTRY    0x0002                                      
#define NG_NOCREATE     0x0004                                                    

  
                                                      
   
#define VTONFS(vp)      ((nfsnode_t)vnode_fsnode(vp))
#define NFSTOV(np)      ((np)->n_vnode)

                              
extern lck_mtx_t nfs_node_hash_mutex;

  
                                                        
   
#define NP(NP, FMT, ...) \
	do { \
	        const char *__vname = (NP) ? vnode_getname(NFSTOV(NP)) : NULL; \
	        printf(FMT " %s\n", ##__VA_ARGS__, __vname ? __vname : "???"); \
	        if (__vname) vnode_putname(__vname); \
	} while (0)

  
                    
   
struct nfsiod {
	TAILQ_ENTRY(nfsiod)     niod_link;                           
	struct nfsmount *       niod_nmp;                                        
};
TAILQ_HEAD(nfsiodlist, nfsiod);
TAILQ_HEAD(nfsiodmountlist, nfsmount);
extern struct nfsiodlist nfsiodfree, nfsiodwork;
extern struct nfsiodmountlist nfsiodmounts;
extern lck_mtx_t nfsiod_mutex;

#if defined(KERNEL)

typedef int     vnop_t(void *);
extern  vnop_t  **fifo_nfsv2nodeop_p;
extern  vnop_t  **nfsv2_vnodeop_p;
extern  vnop_t  **fpnfs_vnodeop_p;
extern  vnop_t  **spec_nfsv2nodeop_p;
#if CONFIG_NFS4
extern  vnop_t  **fifo_nfsv4nodeop_p;
extern  vnop_t  **nfsv4_vnodeop_p;
extern  vnop_t  **spec_nfsv4nodeop_p;
#endif
  
                                      
   
#define nfs_vnop_revoke nop_revoke
int     nfs_vnop_inactive(struct vnop_inactive_args *);
int     nfs_vnop_reclaim(struct vnop_reclaim_args *);

int nfs_node_lock(nfsnode_t);
int nfs_node_lock_internal(nfsnode_t, int);
void nfs_node_lock_force(nfsnode_t);
void nfs_node_unlock(nfsnode_t);
int nfs_node_lock2(nfsnode_t, nfsnode_t);
void nfs_node_unlock2(nfsnode_t, nfsnode_t);
int nfs_node_set_busy(nfsnode_t, thread_t);
int nfs_node_set_busy2(nfsnode_t, nfsnode_t, thread_t);
int nfs_node_set_busy4(nfsnode_t, nfsnode_t, nfsnode_t, nfsnode_t, thread_t);
void nfs_node_clear_busy(nfsnode_t);
void nfs_node_clear_busy2(nfsnode_t, nfsnode_t);
void nfs_node_clear_busy4(nfsnode_t, nfsnode_t, nfsnode_t, nfsnode_t);
void nfs_data_lock(nfsnode_t, int);
void nfs_data_lock_noupdate(nfsnode_t, int);
void nfs_data_lock_internal(nfsnode_t, int, int);
void nfs_data_unlock(nfsnode_t);
void nfs_data_unlock_noupdate(nfsnode_t);
void nfs_data_unlock_internal(nfsnode_t, int);
void nfs_data_update_size(nfsnode_t, int);

                 
int nfs_removeit(struct nfs_sillyrename *);
int nfs_nget(mount_t, nfsnode_t, struct componentname *, u_char *, uint32_t, struct nfs_vattr *, u_int64_t *, uint32_t, int, nfsnode_t*);
int nfs_mount_is_dirty(mount_t);
void nfs_dir_cookie_cache(nfsnode_t, uint64_t, uint64_t);
int nfs_dir_cookie_to_lbn(nfsnode_t, uint64_t, int *, uint64_t *);
void nfs_invaldir(nfsnode_t);
uint64_t nfs_dir_buf_freespace(struct nfsbuf *, int);

                      
void nfs_nbinit(void);
void nfs_buf_timer(void *, void *);
void nfs_buf_remfree(struct nfsbuf *);
boolean_t nfs_buf_is_incore(nfsnode_t, daddr64_t);
struct nfsbuf * nfs_buf_incore(nfsnode_t, daddr64_t);
int nfs_buf_get(nfsnode_t, daddr64_t, uint32_t, thread_t, int, struct nfsbuf **);
int nfs_buf_upl_setup(struct nfsbuf *bp);
void nfs_buf_upl_check(struct nfsbuf *bp);
void nfs_buf_normalize_valid_range(nfsnode_t, struct nfsbuf *);
int nfs_buf_map(struct nfsbuf *);
void nfs_buf_release(struct nfsbuf *, int);
int nfs_buf_iowait(struct nfsbuf *);
void nfs_buf_iodone(struct nfsbuf *);
void nfs_buf_write_delayed(struct nfsbuf *);
void nfs_buf_check_write_verifier(nfsnode_t, struct nfsbuf *);
void nfs_buf_freeup(int);
void nfs_buf_refget(struct nfsbuf *bp);
void nfs_buf_refrele(struct nfsbuf *bp);
void nfs_buf_drop(struct nfsbuf *);
errno_t nfs_buf_acquire(struct nfsbuf *, int, int, int);
int nfs_buf_iterprepare(nfsnode_t, struct nfsbuflists *, int);
void nfs_buf_itercomplete(nfsnode_t, struct nfsbuflists *, int);

void nfs_buf_pgs_get_page_mask(nfsbufpgs *, off_t);
void nfs_buf_pgs_bit_not(nfsbufpgs *);
void nfs_buf_pgs_bit_and(nfsbufpgs *, nfsbufpgs *, nfsbufpgs *);
void nfs_buf_pgs_set_pages_between(nfsbufpgs *, off_t, off_t);
int nfs_buf_pgs_is_set(nfsbufpgs *);

int nfs_bioread(nfsnode_t, uio_t, int, vfs_context_t);
int nfs_buf_readahead(nfsnode_t, int, daddr64_t *, daddr64_t, thread_t, kauth_cred_t);
int nfs_buf_readdir(struct nfsbuf *, vfs_context_t);
int nfs_buf_read(struct nfsbuf *);
void nfs_buf_read_finish(struct nfsbuf *);
int nfs_buf_read_rpc(struct nfsbuf *, thread_t, kauth_cred_t);
void nfs_buf_read_rpc_finish(struct nfsreq *);
int nfs_buf_write(struct nfsbuf *);
void nfs_buf_write_finish(struct nfsbuf *, thread_t, kauth_cred_t);
int nfs_buf_write_rpc(struct nfsbuf *, int, thread_t, kauth_cred_t);
void nfs_buf_write_rpc_finish(struct nfsreq *);
int nfs_buf_write_dirty_pages(struct nfsbuf *, thread_t, kauth_cred_t);

int nfs_flushcommits(nfsnode_t, int);
int nfs_flush(nfsnode_t, int, thread_t, int);
void nfs_buf_delwri_push(int);
void nfs_buf_delwri_service(void);
void nfs_buf_delwri_thread(void *, wait_result_t);

int nfsiod_start(void);
void nfsiod_terminate(struct nfsiod *);
void nfsiod_thread(void);
int nfsiod_continue(int);
void nfs_asyncio_finish(struct nfsreq *);
void nfs_asyncio_resend(struct nfsreq *);
int nfs_async_write_start(struct nfsmount *);
void nfs_async_write_done(struct nfsmount *);

#endif             

#endif                          
#endif                      
