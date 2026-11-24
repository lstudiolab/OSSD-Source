/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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
 * Copyright (c) 1989, 1993, 1995
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
 *	@(#)nfs.h	8.4 (Berkeley) 5/1/95
 * FreeBSD-Id: nfs.h,v 1.32 1997/10/12 20:25:38 phk Exp $
 */

#ifndef _NFS_NFS_H_
#define _NFS_NFS_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>

#ifdef __APPLE_API_PRIVATE
  
                            
   

#define NFS_TICKINTVL   5                                                   
#define NFS_HZ          (hz / nfs_ticks)                
extern int nfs_ticks;
#define NFS_TIMEO       (1 * NFS_HZ)                                    
#define NFS_MINTIMEO    (1 * NFS_HZ)                            
#define NFS_MAXTIMEO    (60 * NFS_HZ)                                  
#define NFS_MINIDEMTIMEO (5 * NFS_HZ)                                          
#define NFS_MAXREXMIT   100                                                
#define NFS_RETRANS     10                                                  
#define NFS_TRYLATERDEL 4                                                  
#define NFS_MAXGRPS     16U                                           
#define NFS_MINATTRTIMO 5                                                   
#define NFS_MAXATTRTIMO 60
#define NFS_MINDIRATTRTIMO 5                                                          
#define NFS_MAXDIRATTRTIMO 60
#define NFS_MAXPORT     0xffff
#define NFS_IOSIZE      (1024 * 1024)                           
#define NFS_RWSIZE      32768                                                 
#define NFS_WSIZE       NFS_RWSIZE                                       
#define NFS_RSIZE       NFS_RWSIZE                                      
#define NFS_DGRAM_WSIZE 8192                                                
#define NFS_DGRAM_RSIZE 8192                                               
#define NFS_READDIRSIZE 32768                                  
#define NFS_DEFRAHEAD   16                                            
#define NFS_MAXRAHEAD   128                                           
#define NFS_DEFMAXASYNCWRITES   128                                                 
#define NFS_DEFASYNCTHREAD      16                                 
#define NFS_MAXASYNCTHREAD      64                                
#define NFS_ASYNCTHREADMAXIDLE  60                                                  
#define NFS_DEFSTATFSRATELIMIT  10                                             
#define NFS_REQUESTDELAY        10                                              
#define NFSRV_MAXWGATHERDELAY   100                                         
#ifndef NFSRV_WGATHERDELAY
#define NFSRV_WGATHERDELAY      1                                              
#endif
#define NFS_DIRBLKSIZ   4096                                               
#if defined(KERNEL) && !defined(DIRBLKSIZ)
#define DIRBLKSIZ       512                                                     
                                                                                
#endif

                                                    
#define NFS_TPRINTF_INITIAL_DELAY       5
#define NFS_TPRINTF_DELAY               30

  
           
   
#define NFS_CMPFH(n, f, s) \
	((n)->n_fhsize == (s) && !bcmp((caddr_t)(n)->n_fhp, (caddr_t)(f), (s)))
#define NFSRV_NDMAXDATA(n) \
	        (((n)->nd_vers == NFS_VER3) ? (((n)->nd_nam2) ? \
	         NFS_MAXDGRAMDATA : NFSRV_MAXDATA) : NFS_V2MAXDATA)
#define NFS_PORT_INVALID(port) \
	(((port) > NFS_MAXPORT) || ((port) < 0))

  
                                                                     
                                     
   
#ifndef IO_METASYNC
#define IO_METASYNC     0
#endif

  
                                                                          
                                                                         
                                                                       
                                                               
                                                                       
                                                                          
                   
                                                                      
   
#define NFS_NODEALLOC   1024
#define NFS_MNTALLOC    1024
#define NFS_SVCALLOC    512

#define NFS_ARGSVERSION_XDR     88                                            

#define NFS_XDRARGS_VERSION_0   0
#define NFS_MATTR_BITMAP_LEN    1                                                      
#define NFS_MFLAG_BITMAP_LEN    1                                                 

                          
#define NFS_MATTR_FLAGS                 0                                      
#define NFS_MATTR_NFS_VERSION           1                                 
#define NFS_MATTR_NFS_MINOR_VERSION     2                                       
#define NFS_MATTR_READ_SIZE             3                          
#define NFS_MATTR_WRITE_SIZE            4                           
#define NFS_MATTR_READDIR_SIZE          5                             
#define NFS_MATTR_READAHEAD             6                                  
#define NFS_MATTR_ATTRCACHE_REG_MIN     7                                         
#define NFS_MATTR_ATTRCACHE_REG_MAX     8                                         
#define NFS_MATTR_ATTRCACHE_DIR_MIN     9                                                  
#define NFS_MATTR_ATTRCACHE_DIR_MAX     10                                                 
#define NFS_MATTR_LOCK_MODE             11                                                        
#define NFS_MATTR_SECURITY              12                                       
#define NFS_MATTR_MAX_GROUP_LIST        13                                        
#define NFS_MATTR_SOCKET_TYPE           14                                                        
#define NFS_MATTR_NFS_PORT              15                                          
#define NFS_MATTR_MOUNT_PORT            16                                            
#define NFS_MATTR_REQUEST_TIMEOUT       17                                             
#define NFS_MATTR_SOFT_RETRY_COUNT      18                                                   
#define NFS_MATTR_DEAD_TIMEOUT          19                                                                
#define NFS_MATTR_FH                    20                                           
#define NFS_MATTR_FS_LOCATIONS          21                                                 
#define NFS_MATTR_MNTFLAGS              22                                   
#define NFS_MATTR_MNTFROM               23                                                   
#define NFS_MATTR_REALM                 24                                      
#define NFS_MATTR_PRINCIPAL             25                                              
#define NFS_MATTR_SVCPRINCIPAL          26                                                                  
#define NFS_MATTR_NFS_VERSION_RANGE     27                                       
#define NFS_MATTR_KERB_ETYPE            28                                              
#define NFS_MATTR_LOCAL_NFS_PORT        29                                               
#define NFS_MATTR_LOCAL_MOUNT_PORT      30                                                 
#define NFS_MATTR_SET_MOUNT_OWNER       31                                    

                     
#define NFS_MFLAG_SOFT                  0                                                       
#define NFS_MFLAG_INTR                  1                                               
#define NFS_MFLAG_RESVPORT              2                                
#define NFS_MFLAG_NOCONNECT             3                                           
#define NFS_MFLAG_DUMBTIMER             4                                           
#define NFS_MFLAG_CALLUMNT              5                                           
#define NFS_MFLAG_RDIRPLUS              6                                                             
#define NFS_MFLAG_NONEGNAMECACHE        7                                           
#define NFS_MFLAG_MUTEJUKEBOX           8                                                       
#define NFS_MFLAG_EPHEMERAL             9                                     
#define NFS_MFLAG_NOCALLBACK            10                                              
#define NFS_MFLAG_NAMEDATTR             11                                      
#define NFS_MFLAG_NOACL                 12                              
#define NFS_MFLAG_ACLONLY               13                                        
#define NFS_MFLAG_NFC                   14                            
#define NFS_MFLAG_NOQUOTA               15                                        
#define NFS_MFLAG_MNTUDP                16                                         
#define NFS_MFLAG_MNTQUICK              17                                             
                                                              
#define NFS_MFLAG_NOOPAQUE_AUTH         19                                                        

                                                      
#define PVER2MAJOR(M) ((uint32_t)(((M) >> 16) & 0xffff))
#define PVER2MINOR(m) ((uint32_t)((m) & 0xffff))
#define VER2PVER(M, m) ((uint32_t)((M) << 16) | ((m) & 0xffff))

                                     
#define NFS_LOCK_MODE_ENABLED           0                                          
#define NFS_LOCK_MODE_DISABLED          1                                                 
#define NFS_LOCK_MODE_LOCAL             2                                                  

#define NFS_STRLEN_INT(str) \
	        (int)strnlen(str, INT_MAX)
#define NFS_UIO_ADDIOV(a_uio, a_baseaddr, a_length) \
	        assert(a_length <= UINT32_MAX); uio_addiov(a_uio, a_baseaddr, (uint32_t)(a_length));
#define NFS_BZERO(off, bytes) \
	do { \
	        uint32_t bytes32 = bytes > UINT32_MAX ? UINT32_MAX : (uint32_t)(bytes); \
	        bzero(off, bytes32); \
	        if (bytes > UINT32_MAX) { \
	                bzero(off + bytes32, (uint32_t)(bytes - UINT32_MAX)); \
	        } \
	} while(0);
#define NFS_ZFREE(zone, ptr) \
	do { \
	        if ((ptr)) { \
	                zfree((zone), (ptr)); \
	                (ptr) = NULL; \
	        } \
	} while (0); \

                                                          
typedef enum  nfs_supported_kerberos_etypes {
	NFS_DES3_CBC_SHA1_KD = 16,
	NFS_AES128_CTS_HMAC_SHA1_96 = 17,
	NFS_AES256_CTS_HMAC_SHA1_96 = 18
} nfs_supported_kerberos_etypes;

                                                                         
#define NFS_MAX_ETYPES 3
struct nfs_etype {
	uint32_t count;
	uint32_t selected;                                                                                     
	nfs_supported_kerberos_etypes etypes[NFS_MAX_ETYPES];
};

  
                                   
   
#define NFS_ARGSVERSION 6                                                 
struct nfs_args {
	int             version;                                           
#ifdef KERNEL
	user32_addr_t addr;                                      
#else
	struct sockaddr *addr;                                   
#endif
	uint8_t         addrlen;                               
	int             sotype;                          
	int             proto;                            
#ifdef KERNEL
	user32_addr_t fh;                                              
#else
	u_char          *fh;                                           
#endif
	int             fhsize;                                    
	int             flags;                     
	int             wsize;                                   
	int             rsize;                                  
	int             readdirsize;                               
	int             timeo;                                          
	int             retrans;                                 
	int             maxgrouplist;                                
	int             readahead;                                    
	int             leaseterm;                                         
	int             deadthresh;                                      
#ifdef KERNEL
	user32_addr_t hostname;                    
#else
	char            *hostname;                         
#endif
	                                 
	int             acregmin;                                            
	int             acregmax;                                            
	int             acdirmin;                                       
	int             acdirmax;                                       
	                                 
	uint32_t        auth;                                          
	                                 
	uint32_t        deadtimeout;                                                       
};

                                                            
#define NFS_ARGSVERSION4_INCSIZE        (4 * sizeof(int))
#define NFS_ARGSVERSION5_INCSIZE        (sizeof(uint32_t))
#define NFS_ARGSVERSION6_INCSIZE        (sizeof(uint32_t))

#ifdef KERNEL
                                                    
                                                 
                                       
   
struct user_nfs_args {
	int             version;                                           
	user_addr_t     addr __attribute((aligned(8)));                                  
	uint8_t         addrlen;                               
	int             sotype;                          
	int             proto;                            
	user_addr_t     fh __attribute((aligned(8)));                                          
	int             fhsize;                                    
	int             flags;                     
	int             wsize;                                   
	int             rsize;                                  
	int             readdirsize;                               
	int             timeo;                                          
	int             retrans;                                 
	int             maxgrouplist;                                
	int             readahead;                                    
	int             leaseterm;                                         
	int             deadthresh;                                      
	user_addr_t     hostname __attribute((aligned(8)));                        
	                                 
	int             acregmin;                                            
	int             acregmax;                                            
	int             acdirmin;                                       
	int             acdirmax;                                       
	                                 
	uint32_t        auth;                                          
	                                 
	uint32_t        deadtimeout;                                                       
};
#endif          

  
                                   
   
#define NFSMNT_SOFT             0x00000001                                    
#define NFSMNT_WSIZE            0x00000002                      
#define NFSMNT_RSIZE            0x00000004                     
#define NFSMNT_TIMEO            0x00000008                           
#define NFSMNT_RETRANS          0x00000010                                     
#define NFSMNT_MAXGRPS          0x00000020                                  
#define NFSMNT_INT              0x00000040                                      
#define NFSMNT_NOCONN           0x00000080                                
#define NFSMNT_NONEGNAMECACHE   0x00000100                                      
#define NFSMNT_NFSV3            0x00000200                                  
#define NFSMNT_NFSV4            0x00000400                                  
#define NFSMNT_DUMBTIMR         0x00000800                                      
#define NFSMNT_DEADTIMEOUT      0x00001000                                                  
#define NFSMNT_READAHEAD        0x00002000                      
#define NFSMNT_CALLUMNT         0x00004000                                      
#define NFSMNT_RESVPORT         0x00008000                                
#define NFSMNT_RDIRPLUS         0x00010000                              
#define NFSMNT_READDIRSIZE      0x00020000                        
#define NFSMNT_NOLOCKS          0x00040000                                  
#define NFSMNT_LOCALLOCKS       0x00080000                                         
#define NFSMNT_ACREGMIN         0x00100000                                  
#define NFSMNT_ACREGMAX         0x00200000                                  
#define NFSMNT_ACDIRMIN         0x00400000                                  
#define NFSMNT_ACDIRMAX         0x00800000                                  
#define NFSMNT_SECFLAVOR        0x01000000                           
#define NFSMNT_SECSYSOK         0x02000000                                   
#define NFSMNT_MUTEJUKEBOX      0x04000000                                                  
#define NFSMNT_NOQUOTA          0x08000000                                    


  
                                         
   
#define NFS_MOUNT_INFO_VERSION  0                                              
#define NFS_MIATTR_BITMAP_LEN   1                                                   
#define NFS_MIFLAG_BITMAP_LEN   1                                              

                               
#define NFS_MIATTR_FLAGS                0                                               
#define NFS_MIATTR_ORIG_ARGS            1                                                       
#define NFS_MIATTR_CUR_ARGS             2                                      
#define NFS_MIATTR_CUR_LOC_INDEX        3                                      

                          
#define NFS_MIFLAG_DEAD         0                          
#define NFS_MIFLAG_NOTRESP      1                                   
#define NFS_MIFLAG_RECOVERY     2                              


  
                                                                 
                              
   
struct nfsd_args {
	int     sock;                                
#ifdef KERNEL
	user32_addr_t   name;                                                         
#else
	caddr_t name;                                                         
#endif
	int     namelen;                            
};

#ifdef KERNEL
                                                     
                                                 
                                        
   
struct user_nfsd_args {
	int             sock;                                
	user_addr_t     name __attribute((aligned(8)));                                                       
	int             namelen;                            
};

#endif          

  
                                    
   

                                                   
#define NFS_FH_VERSION  0x4e580000                          
struct nfs_exphandle {
	uint32_t        nxh_version;                                        
	uint32_t        nxh_fsid;                                          
	uint32_t        nxh_expid;                             
	uint16_t        nxh_flags;                                       
	uint8_t         nxh_reserved;                           
	uint32_t        nxh_fidlen;                                    
};

               
#define NXHF_INVALIDFH          0x0001                                      

#define NFS_MAX_FID_SIZE        (NFS_MAX_FH_SIZE - sizeof(struct nfs_exphandle))
#define NFSV4_MAX_FID_SIZE      (NFSV4_MAX_FH_SIZE - sizeof(struct nfs_exphandle))
#define NFSV3_MAX_FID_SIZE      (NFSV3_MAX_FH_SIZE - sizeof(struct nfs_exphandle))
#define NFSV2_MAX_FID_SIZE      (NFSV2_MAX_FH_SIZE - sizeof(struct nfs_exphandle))

                                           
                                                                            
                                                      
struct nfs_filehandle {
	uint32_t                nfh_len;                                         
	struct nfs_exphandle    nfh_xh;                            
	unsigned char           nfh_fid[NFS_MAX_FID_SIZE];              
	unsigned char           *nfh_fhp;                                   
};

  
                             
   

                                                    
#define NX_MAX_SEC_FLAVORS 5
struct nfs_sec {
	int count;
	uint32_t flavors[NX_MAX_SEC_FLAVORS];
};

struct nfs_export_net_args {
	uint32_t                nxna_flags;                       
	struct xucred           nxna_cred;                                               
	struct sockaddr_storage nxna_addr;                                         
	struct sockaddr_storage nxna_mask;                                
	struct nfs_sec          nxna_sec;                                       
};

struct nfs_export_args {
	uint32_t                nxa_fsid;                         
	uint32_t                nxa_expid;                     
#ifdef KERNEL
	user32_addr_t           nxa_fspath;                         
	user32_addr_t           nxa_exppath;                         
#else
	char                    *nxa_fspath;                        
	char                    *nxa_exppath;                        
#endif
	uint32_t                nxa_flags;                            
	uint32_t                nxa_netcount;                                  
#ifdef KERNEL
	user32_addr_t           nxa_nets;                              
#else
	struct nfs_export_net_args *nxa_nets;                          
#endif
};

#ifdef KERNEL
                                 

struct user_nfs_export_args {
	uint32_t                nxa_fsid;                         
	uint32_t                nxa_expid;                     
	user_addr_t             nxa_fspath;                         
	user_addr_t             nxa_exppath;                         
	uint32_t                nxa_flags;                            
	uint32_t                nxa_netcount;                                  
	user_addr_t             nxa_nets;                              
};

#endif             

                          
#define NXA_DELETE              0x0001                                      
#define NXA_ADD                 0x0002                                   
#define NXA_REPLACE             0x0003                                              
#define NXA_DELETE_ALL          0x0004                          
#define NXA_OFFLINE             0x0008                         
#define NXA_CHECK               0x0010                           

                         
#define NX_READONLY             0x0001                          
#define NX_DEFAULTEXPORT        0x0002                             
#define NX_MAPROOT              0x0004                                          
#define NX_MAPALL               0x0008                                         
#define NX_32BITCLIENTS         0x0020                                             
#define NX_OFFLINE              0x0040                         
#define NX_MANGLEDNAMES         0x0080                                                              

  
                                                  
   
#define NFS_EXPORT_STAT_REC_VERSION 1                                   
#define NFS_USER_STAT_REC_VERSION 1                                          

                                             
struct nfs_export_stat_desc {
	uint32_t rec_vers;                                                  
	uint64_t rec_count;                                     
}__attribute__((__packed__));

                                                          
struct nfs_export_stat_rec {
	char     path[RPCMNT_PATHLEN + 1];
	uint64_t ops;                                                               
	uint64_t bytes_read;                                              
	uint64_t bytes_written;                                            
}__attribute__((__packed__));

                                             
struct nfs_user_stat_desc {
	uint32_t rec_vers;                                               
	uint32_t rec_count;                             
}__attribute__((__packed__));

                                              
struct nfs_user_stat_user_rec {
	u_char                  rec_type;
	uid_t                   uid;
	struct sockaddr_storage sock;
	uint64_t                ops;
	uint64_t                bytes_read;
	uint64_t                bytes_written;
	time_t                  tm_start;
	time_t                  tm_last;
}__attribute__((__packed__));

                                         
struct nfs_user_stat_path_rec {
	u_char  rec_type;
	char    path[RPCMNT_PATHLEN + 1];
}__attribute__((__packed__));

                                
                                        
                  
   
#define NFS_USER_STAT_USER_REC  0
#define NFS_USER_STAT_PATH_REC  1


#ifdef KERNEL
struct nfs_exportfs;

struct nfs_export_options {
	uint32_t                nxo_flags;                          
	kauth_cred_t            nxo_cred;                              
	struct nfs_sec          nxo_sec;                                        
};

                                                                  
struct nfs_netopt {
	struct radix_node               no_rnodes[2];                        
	struct nfs_export_options       no_opt;                             
};

                                                 
  
                                                                        
                                                                   
                    
   
typedef struct nfsstatcount64 {
	uint32_t        hi;
	uint32_t        lo;
} nfsstatcount64;

struct nfs_export_stat_counters {
	struct nfsstatcount64 ops;                                                                   
	struct nfsstatcount64 bytes_read;                                                 
	struct nfsstatcount64 bytes_written;                                              
};

                                                 
#define NFSStatAdd64(PTR, VAL) \
	do { \
	        uint32_t NFSSA_OldValue = \
	        OSAddAtomic((VAL), &(PTR)->lo); \
	        if ((NFSSA_OldValue + (VAL)) < NFSSA_OldValue) \
	                OSAddAtomic(1, &(PTR)->hi); \
	} while (0)

                                                          
#define NFSRV_USER_STAT_DEF_MAX_NODES 1024                                               
#define NFSRV_USER_STAT_DEF_IDLE_SEC  7200                                                                   

                              
extern uint32_t nfsrv_user_stat_enabled;                                                     
extern uint32_t nfsrv_user_stat_node_count;                                                   
extern uint32_t nfsrv_user_stat_max_idle_sec;                                                        
extern uint32_t nfsrv_user_stat_max_nodes;                                               
extern lck_grp_t nfsrv_active_user_mutex_group;

                                                   
struct nfs_user_stat_node {
	TAILQ_ENTRY(nfs_user_stat_node) lru_link;
	LIST_ENTRY(nfs_user_stat_node)  hash_link;
	uid_t                   uid;
	struct sockaddr_storage sock;
	uint64_t                ops;
	uint64_t                bytes_read;
	uint64_t                bytes_written;
	time_t                  tm_start;
	time_t                  tm_last;
};

                                      
#define NFS_USER_STAT_HASH_SIZE 16                                
#define NFS_USER_STAT_HASH(userhashtbl, uid) \
	        &((userhashtbl)[(uid) & (NFS_USER_STAT_HASH_SIZE - 1)])

TAILQ_HEAD(nfs_user_stat_lru_head, nfs_user_stat_node);
LIST_HEAD(nfs_user_stat_hashtbl_head, nfs_user_stat_node);

                                     
                                
struct nfs_active_user_list {
	struct nfs_user_stat_lru_head           user_lru;
	struct nfs_user_stat_hashtbl_head       user_hashtbl[NFS_USER_STAT_HASH_SIZE];
	uint32_t                                node_count;
	lck_mtx_t user_mutex;
};


                                
                                              
struct nfs_export {
	LIST_ENTRY(nfs_export)          nx_next;                            
	LIST_ENTRY(nfs_export)          nx_hash;                               
	struct nfs_export               *nx_parent;                        
	uint32_t                        nx_id;                         
	uint32_t                        nx_flags;                         
	struct nfs_exportfs             *nx_fs;                                   
	char                            *nx_path;                                          
	struct nfs_filehandle           nx_fh;                                       
	struct nfs_export_options       nx_defopt;                           
	uint32_t                        nx_expcnt;                              
	struct radix_node_head          *nx_rtable[AF_MAX + 1];                                 
	struct nfs_export_stat_counters nx_stats;                                                           
	struct nfs_active_user_list     nx_user_list;                                                     
	struct timeval                  nx_exptime;                                            
};

                                   
                                                
struct nfs_exportfs {
	LIST_ENTRY(nfs_exportfs)        nxfs_next;                                     
	uint32_t                        nxfs_id;                                     
	char                            *nxfs_path;                                    
	LIST_HEAD(, nfs_export)          nxfs_exports;                                             
};

extern LIST_HEAD(nfsrv_expfs_list, nfs_exportfs) nfsrv_exports;
extern lck_rw_t nfsrv_export_rwlock;                                    
#define NFSRVEXPHASHSZ  64
#define NFSRVEXPHASHVAL(FSID, EXPID)    \
	(((FSID) >> 24) ^ ((FSID) >> 16) ^ ((FSID) >> 8) ^ (EXPID))
#define NFSRVEXPHASH(FSID, EXPID)       \
	(&nfsrv_export_hashtbl[NFSRVEXPHASHVAL((FSID),(EXPID)) & nfsrv_export_hash])
extern LIST_HEAD(nfsrv_export_hashhead, nfs_export) * nfsrv_export_hashtbl;
extern u_long nfsrv_export_hash;

#if CONFIG_FSE
  
                               
   
struct nfsrv_fmod {
	LIST_ENTRY(nfsrv_fmod)  fm_link;
	vnode_t                 fm_vp;
	struct vfs_context      fm_context;
	uint64_t                fm_deadline;
};

#define NFSRVFMODHASHSZ 128
#define NFSRVFMODHASH(vp) (((uintptr_t) vp) & nfsrv_fmod_hash)
extern LIST_HEAD(nfsrv_fmod_hashhead, nfsrv_fmod) * nfsrv_fmod_hashtbl;
extern u_long nfsrv_fmod_hash;
extern lck_mtx_t nfsrv_fmod_mutex;
extern int nfsrv_fmod_pending, nfsrv_fsevents_enabled;
#endif

extern int nfsrv_async, nfsrv_export_hash_size,
    nfsrv_reqcache_size, nfsrv_sock_max_rec_queue_length;
extern uint32_t nfsrv_gss_context_ttl;
extern struct nfsstats nfsstats;
#define NFS_UC_Q_DEBUG
#ifdef NFS_UC_Q_DEBUG
extern int nfsrv_uc_use_proxy;
extern uint32_t nfsrv_uc_queue_limit;
extern uint32_t nfsrv_uc_queue_max_seen;
extern volatile uint32_t nfsrv_uc_queue_count;
#endif

#endif          

  
                                                       
   
#ifdef NFS_NPROCS
  
                  
   
struct nfsstats {
	uint64_t        attrcache_hits;
	uint64_t        attrcache_misses;
	uint64_t        lookupcache_hits;
	uint64_t        lookupcache_misses;
	uint64_t        direofcache_hits;
	uint64_t        direofcache_misses;
	uint64_t        biocache_reads;
	uint64_t        read_bios;
	uint64_t        read_physios;
	uint64_t        biocache_writes;
	uint64_t        write_bios;
	uint64_t        write_physios;
	uint64_t        biocache_readlinks;
	uint64_t        readlink_bios;
	uint64_t        biocache_readdirs;
	uint64_t        readdir_bios;
	uint64_t        rpccnt[NFS_NPROCS];
	uint64_t        rpcretries;
	uint64_t        srvrpccnt[NFS_NPROCS];
	uint64_t        srvrpc_errs;
	uint64_t        srv_errs;
	uint64_t        rpcrequests;
	uint64_t        rpctimeouts;
	uint64_t        rpcunexpected;
	uint64_t        rpcinvalid;
	uint64_t        srvcache_inproghits;
	uint64_t        srvcache_idemdonehits;
	uint64_t        srvcache_nonidemdonehits;
	uint64_t        srvcache_misses;
	uint64_t        srvvop_writes;
	uint64_t        pageins;
	uint64_t        pageouts;
};
#endif

  
                                  
   
#define NFSSVC_NFSD     0x004
#define NFSSVC_ADDSOCK  0x008
#define NFSSVC_EXPORT   0x200

  
                                   
   
#define NFSCLNT_LOCKDANS        0x200
#define NFSCLNT_LOCKDNOTIFY     0x400
#define NFSCLNT_TESTIDMAP       0x001

#include <sys/_types/_guid_t.h>                       
#define MAXIDNAMELEN            1024
struct nfs_testmapid {
	uint32_t                ntm_lookup;                                        
	uint32_t                ntm_grpflag;                                        
	uint32_t                ntm_id;                                  
	uint32_t                pad;
	guid_t                  ntm_guid;                                                 
	char                    ntm_name[MAXIDNAMELEN];                            
};

#define NTM_ID2NAME     0
#define NTM_NAME2ID     1
#define NTM_NAME2GUID   2
#define NTM_GUID2NAME   3

  
                               
   
#define NFS_NFSSTATS        1                                    
#define NFS_EXPORTSTATS     3                                          
#define NFS_USERSTATS       4                                                      
#define NFS_USERCOUNT       5                                                   
#define NFS_MOUNTINFO       6                                                
#define NFS_NFSZEROSTATS    7                                

#ifndef NFS_WDELAYHASHSIZ
#define NFS_WDELAYHASHSIZ 16                       
#endif

#ifdef KERNEL
#include <sys/kernel_types.h>
#include <kern/thread_call.h>
#include <sys/kdebug.h>

#define NFS_KERNEL_DEBUG KERNEL_DEBUG

                               
#define FSDBG(A, B, C, D, E) \
	NFS_KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, (A))) | DBG_FUNC_NONE, \
	        (int)(B), (int)(C), (int)(D), (int)(E), 0)
#define FSDBG_TOP(A, B, C, D, E) \
	NFS_KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, (A))) | DBG_FUNC_START, \
	        (int)(B), (int)(C), (int)(D), (int)(E), 0)
#define FSDBG_BOT(A, B, C, D, E) \
	NFS_KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, (A))) | DBG_FUNC_END, \
	        (int)(B), (int)(C), (int)(D), (int)(E), 0)

#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_NFSMNT);
MALLOC_DECLARE(M_NFSBIO);
MALLOC_DECLARE(M_NFSDIROFF);
MALLOC_DECLARE(M_NFSRVDESC);
MALLOC_DECLARE(M_NFSD);
MALLOC_DECLARE(M_NFSBIGFH);
#endif

struct vnode_attr; struct nameidata; struct dqblk; struct sockaddr_in;          
struct nfsbuf;
struct nfs_vattr;
struct nfs_fsattr;
struct nfsnode;
typedef struct nfsnode * nfsnode_t;
struct nfs_open_owner;
struct nfs_open_file;
struct nfs_lock_owner;
struct nfs_file_lock;
struct nfsreq;
struct nfs_rpc_record_state;
struct nfs_fs_locations;
struct nfs_location_index;
struct nfs_socket;
struct nfs_socket_search;
struct nfsrv_uc_arg;
struct direntry;

  
                                                                             
                                                                         
                                                                          
                                                                          
                                                                          
                     
   
#define NFSINT_SIGMASK  (sigmask(SIGINT)|sigmask(SIGTERM)|sigmask(SIGKILL)| \
	                 sigmask(SIGHUP)|sigmask(SIGQUIT))

extern size_t nfs_mbuf_mhlen, nfs_mbuf_minclsize;

  
                                                                             
   
struct nfsm_chain {
	mbuf_t          nmc_mhead;                           
	mbuf_t          nmc_mcur;                         
	caddr_t         nmc_ptr;                                       
	size_t          nmc_left;                                            
	uint32_t        nmc_flags;                                     
};
#define NFSM_CHAIN_FLAG_ADD_CLUSTERS    0x1                                   

  
                                               
                                     
   
struct gss_seq {
	SLIST_ENTRY(gss_seq)    gss_seqnext;
	uint32_t                gss_seqnum;
};

   
                       
   
struct nfsreq_cbargs {
	off_t offset;
	size_t length;
	uint32_t stategenid;
};

  
                                  
   
struct nfsreq_cbinfo {
	void                    (*rcb_func)(struct nfsreq *);                                        
	struct nfsbuf           *rcb_bp;                                                   
	struct nfsreq_cbargs    rcb_args;                                                 
};

  
                                                                                 
  
                                                                                   
                                         
   
struct nfsreq_secinfo_args {
	nfsnode_t               rsia_np;                              
	const char              *rsia_name;                                        
	u_char                  *rsia_fh;                                          
	uint32_t                rsia_namelen;                                 
	uint32_t                rsia_fhsize;                              
};
#define NFSREQ_SECINFO_SET(SI, NP, FH, FHSIZE, NAME, NAMELEN) \
	do { \
	        (SI)->rsia_np = (NP); \
	        (SI)->rsia_fh = (FH); \
	        (SI)->rsia_fhsize = (FHSIZE); \
	        (SI)->rsia_name = (NAME); \
	        (SI)->rsia_namelen = (NAMELEN); \
	} while (0)

  
                                       
   
struct nfsreq {
	lck_mtx_t               r_mtx;                                 
	TAILQ_ENTRY(nfsreq)     r_chain;                                 
	TAILQ_ENTRY(nfsreq)     r_achain;                                                  
	TAILQ_ENTRY(nfsreq)     r_rchain;                                                 
	TAILQ_ENTRY(nfsreq)     r_cchain;                                     
	mbuf_t                  r_mrest;                                
	mbuf_t                  r_mhead;                                  
	struct nfsm_chain       r_nmrep;                         
	nfsnode_t               r_np;                         
	struct nfsmount         *r_nmp;                              
	uint64_t                r_xid;                                  
	uint32_t                r_procnum;                                
	size_t                  r_mreqlen;                          
	int                     r_flags;                                         
	int                     r_lflags;                                                     
	int                     r_refs;                                       
	uint8_t                 r_delay;                                            
	uint32_t                r_retry;                                      
	uint8_t                 r_rexmit;                                  
	int                     r_rtt;                           
	thread_t                r_thread;                                            
	kauth_cred_t            r_cred;                                          
	time_t                  r_start;                                
	time_t                  r_lastmsg;                                
	time_t                  r_resendtime;                                          
	struct nfs_gss_clnt_ctx *r_gss_ctx;                             
	SLIST_HEAD(, gss_seq)   r_gss_seqlist;                                   
	size_t                  r_gss_argoff;                                  
	uint32_t                r_gss_arglen;                              
	uint32_t                r_auth;                                                
	uint32_t                *r_wrongsec;                                        
	int                     r_error;                           
	struct nfsreq_cbinfo    r_callback;                        
	struct nfsreq_secinfo_args r_secinfo;                     
};

  
                          
   
TAILQ_HEAD(nfs_reqqhead, nfsreq);
extern struct nfs_reqqhead nfs_reqq;
extern lck_grp_t nfs_request_grp;

#define R_XID32(x)      ((x) & 0xffffffff)

#define NFSNOLIST       ((void *)0x0badcafe)                                      
#define NFSREQNOLIST    NFSNOLIST                                                    

                             
#define R_TIMING        0x00000001                                    
#define R_CWND          0x00000002                                                      
#define R_SOFTTERM      0x00000004                                              
#define R_RESTART       0x00000008                                    
#define R_INITTED       0x00000010                                        
#define R_TPRINTFMSG    0x00000020                              
#define R_MUSTRESEND    0x00000040                               
#define R_ALLOCATED     0x00000080                                 
#define R_SENT          0x00000100                                 
#define R_WAITSENT      0x00000200                                                     
#define R_RESENDERR     0x00000400                         
#define R_JBTPRINTFMSG  0x00000800                                               
#define R_ASYNC         0x00001000                         
#define R_ASYNCWAIT     0x00002000                                             
#define R_RESENDQ       0x00004000                                              
#define R_SENDING       0x00008000                                        
#define R_SOFT          0x00010000                                                      
#define R_IOD           0x00020000                                              

#define R_NOINTR        0x20000000                                                        
#define R_RECOVER       0x40000000                                                        
#define R_SETUP         0x80000000                                               
#define R_OPTMASK       0xe0000000                                        

                              
#define RL_BUSY         0x0001                       
#define RL_WAITING      0x0002                                         
#define RL_QUEUED       0x0004                                       

extern u_int64_t nfs_xid, nfs_xidwrap;
extern int nfs_iosize, nfs_allow_async, nfs_statfs_rate_limit;
extern int nfs_access_cache_timeout, nfs_access_delete, nfs_access_dotzfs, nfs_access_for_getattr;
extern int nfs_lockd_mounts, nfs_lockd_request_sent;
extern int nfs_tprintf_initial_delay, nfs_tprintf_delay;
extern int nfsiod_thread_count, nfsiod_thread_max, nfs_max_async_writes;
extern int nfs_idmap_ctrl, nfs_callback_port;
extern int nfs_is_mobile, nfs_readlink_nocache, nfs_root_steals_ctx;
extern uint32_t nfs_tcp_sockbuf;
extern uint32_t nfs_squishy_flags;
extern uint32_t nfs_debug_ctl;

                              
#define NFS_IDMAP_CTRL_USE_IDMAP_SERVICE                0x00000001                                 
#define NFS_IDMAP_CTRL_FALLBACK_NO_COMMON_IDS           0x00000002                                                                     
#define NFS_IDMAP_CTRL_LOG_FAILED_MAPPINGS              0x00000020                                     
#define NFS_IDMAP_CTRL_LOG_SUCCESSFUL_MAPPINGS          0x00000040                                         

#define NFSIOD_MAX      (MIN(nfsiod_thread_max, NFS_MAXASYNCTHREAD))

struct nfs_dulookup {
	int du_flags;                                           
#define NFS_DULOOKUP_DOIT       0x1
#define NFS_DULOOKUP_INPROG     0x2
	struct componentname du_cn;                                  
	struct nfsreq du_req;                                       
	char du_smallname[48];                                      
};

  
                                                             
                                   
   
struct nfsrv_sock {
	TAILQ_ENTRY(nfsrv_sock) ns_chain;                                     
	TAILQ_ENTRY(nfsrv_sock) ns_svcq;                                               
	TAILQ_ENTRY(nfsrv_sock) ns_wgq;                                                          
	struct nfsrv_uc_arg *ns_ua;                                           
	lck_rw_t        ns_rwlock;                                        
	socket_t        ns_so;
	mbuf_t          ns_nam;
	mbuf_t          ns_raw;
	mbuf_t          ns_rawend;
	mbuf_t          ns_rec;
	mbuf_t          ns_recend;
	mbuf_t          ns_frag;
	int             ns_flag;
	int             ns_sotype;
	size_t          ns_cc;
	size_t          ns_reclen;
	int             ns_reccnt;
	int             ns_sobufsize;
	u_int32_t       ns_sref;
	time_t          ns_timestamp;                                 
	lck_mtx_t       ns_wgmutex;                                                
	time_t          ns_wgtime;                                              
	LIST_HEAD(, nfsrv_descript) ns_tq;                              
	LIST_HEAD(nfsrv_wg_delayhash, nfsrv_descript) ns_wdelayhashtbl[NFS_WDELAYHASHSIZ];
};

                        
#define SLP_VALID       0x0001                     
#define SLP_DOREC       0x0002                                            
#define SLP_NEEDQ       0x0004                                         
#define SLP_DISCONN     0x0008                                
#define SLP_GETSTREAM   0x0010                                     
#define SLP_LASTFRAG    0x0020                                     
#define SLP_DOWRITES    0x0040                                              
#define SLP_WORKTODO    0x004e                                     
#define SLP_ALLFLAGS    0x007f
#define SLP_WAITQ       0x4000                                    
#define SLP_WORKQ       0x8000                                    
#define SLP_QUEUED      0xc000                             

#define SLPNOLIST ((struct nfsrv_sock *)0xdeadbeef)                                                                  

extern struct nfsrv_sock *nfsrv_udpsock, *nfsrv_udp6sock;

  
                                  
  
                                                  
                                                                        
                                                                                    
                                                                  
   
extern TAILQ_HEAD(nfsrv_sockhead, nfsrv_sock) nfsrv_socklist, nfsrv_sockwg,
nfsrv_sockwait, nfsrv_sockwork;

                                  
extern lck_grp_t nfsrv_slp_rwlock_group;
extern lck_grp_t nfsrv_slp_mutex_group;

  
                                                      
   
struct nfsd {
	TAILQ_ENTRY(nfsd)       nfsd_chain;                             
	TAILQ_ENTRY(nfsd)       nfsd_queue;                                 
	int                     nfsd_flag;                       
	struct nfsrv_sock       *nfsd_slp;                          
	struct nfsrv_descript   *nfsd_nd;                                      
};

                          
#define NFSD_WAITING    0x01
#define NFSD_REQINPROG  0x02

  
                                                                    
                                                                       
   
struct nfsrv_descript {
	time_t                  nd_time;                                   
	off_t                   nd_off;                                
	off_t                   nd_eoff;                                 
	LIST_ENTRY(nfsrv_descript) nd_hash;                    
	LIST_ENTRY(nfsrv_descript) nd_tq;                           
	LIST_HEAD(, nfsrv_descript) nd_coalesce;                       
	struct nfsm_chain       nd_nmreq;                               
	mbuf_t                  nd_mrep;                                  
	mbuf_t                  nd_nam;                              
	mbuf_t                  nd_nam2;                                
	u_int32_t               nd_procnum;                
	int                     nd_stable;                        
	int                     nd_vers;                         
	int                     nd_len;                                   
	int                     nd_repstat;                       
	u_int32_t               nd_retxid;                     
	struct timeval          nd_starttime;                           
	struct nfs_filehandle   nd_fh;                           
	uint32_t                nd_sec;                              
	struct nfs_gss_svc_ctx  *nd_gss_context;                        
	uint32_t                nd_gss_seqnum;                          
	mbuf_t                  nd_gss_mb;                                   
	kauth_cred_t            nd_cr;                           
};

extern TAILQ_HEAD(nfsd_head, nfsd) nfsd_head, nfsd_queue;

typedef int (*nfsrv_proc_t)(struct nfsrv_descript *, struct nfsrv_sock *,
    vfs_context_t, mbuf_t *);

                          
extern lck_mtx_t nfsd_mutex;
extern int nfsd_thread_count, nfsd_thread_max;

                        
extern lck_mtx_t nfs_request_mutex;
extern int nfs_request_timer_on;

                                  
extern lck_mtx_t nfs_global_mutex;

#if CONFIG_NFS4
                            
extern int nfs4_callback_timer_on;
extern in_port_t nfs4_cb_port, nfs4_cb_port6;

                                           
extern char nfs4_default_domain[MAXPATHLEN];
                                
extern thread_call_t    nfs4_callback_timer_call;
#endif

                               
extern thread_call_t    nfs_request_timer_call;
extern thread_call_t    nfs_buf_timer_call;
extern thread_call_t    nfsrv_idlesock_timer_call;
#if CONFIG_FSE
extern thread_call_t    nfsrv_fmod_timer_call;
#endif

__BEGIN_DECLS

nfstype vtonfs_type(enum vtype, int);
enum vtype nfstov_type(nfstype, int);
int     vtonfsv2_mode(enum vtype, mode_t);

void    nfs_mbuf_init(void);

void    nfs_nhinit_finish(void);
u_long  nfs_hash(u_char *, int);

#if CONFIG_NFS4
int     nfs4_init_clientid(struct nfsmount *);
int     nfs4_setclientid(struct nfsmount *);
int     nfs4_renew(struct nfsmount *, int);
void    nfs4_renew_timer(void *, void *);
void    nfs4_mount_callback_setup(struct nfsmount *);
void    nfs4_mount_callback_shutdown(struct nfsmount *);
void    nfs4_cb_accept(socket_t, void *, int);
void    nfs4_cb_rcv(socket_t, void *, int);
void    nfs4_callback_timer(void *, void *);
int     nfs4_secinfo_rpc(struct nfsmount *, struct nfsreq_secinfo_args *, kauth_cred_t, uint32_t *, int *);
int     nfs4_get_fs_locations(struct nfsmount *, nfsnode_t, u_char *, int, const char *, vfs_context_t, struct nfs_fs_locations *);
void    nfs4_default_attrs_for_referral_trigger(nfsnode_t, char *, int, struct nfs_vattr *, fhandle_t *);
#endif

void    nfs_fs_locations_cleanup(struct nfs_fs_locations *);

int     nfs_sockaddr_cmp(struct sockaddr *, struct sockaddr *);
int     nfs_connect(struct nfsmount *, int, int);
void    nfs_disconnect(struct nfsmount *);
void    nfs_need_reconnect(struct nfsmount *);
void    nfs_mount_sock_thread_wake(struct nfsmount *);
int     nfs_mount_check_dead_timeout(struct nfsmount *);
int     nfs_mount_gone(struct nfsmount *);
void    nfs_mount_rele(struct nfsmount *);
void    nfs_mount_zombie(struct nfsmount *, int);
void    nfs_mount_make_zombie(struct nfsmount *);

void    nfs_rpc_record_state_init(struct nfs_rpc_record_state *);
void    nfs_rpc_record_state_cleanup(struct nfs_rpc_record_state *);
int     nfs_rpc_record_read(socket_t, struct nfs_rpc_record_state *, int, int *, mbuf_t *);

int     nfs_getattr(nfsnode_t, struct nfs_vattr *, vfs_context_t, int);
int     nfs_getattrcache(nfsnode_t, struct nfs_vattr *, int);
int     nfs_loadattrcache(nfsnode_t, struct nfs_vattr *, u_int64_t *, int);
long    nfs_attrcachetimeout(nfsnode_t);

int     nfs_buf_page_inval(vnode_t vp, off_t offset);
int     nfs_vinvalbuf(vnode_t, int, vfs_context_t, int);
int     nfs_vinvalbuf2(vnode_t, int, thread_t, kauth_cred_t, int);
int     nfs_vinvalbuf_internal(nfsnode_t, int, thread_t, kauth_cred_t, int, int);
void    nfs_wait_bufs(nfsnode_t);

int     nfs_request_create(nfsnode_t, mount_t, struct nfsm_chain *, int, thread_t, kauth_cred_t, struct nfsreq **);
void    nfs_request_destroy(struct nfsreq *);
void    nfs_request_ref(struct nfsreq *, int);
void    nfs_request_rele(struct nfsreq *);
int     nfs_request_add_header(struct nfsreq *);
int     nfs_request_send(struct nfsreq *, int);
void    nfs_request_wait(struct nfsreq *);
int     nfs_request_finish(struct nfsreq *, struct nfsm_chain *, int *);
int     nfs_request(nfsnode_t, mount_t, struct nfsm_chain *, int, vfs_context_t, struct nfsreq_secinfo_args *, struct nfsm_chain *, u_int64_t *, int *);
int     nfs_request2(nfsnode_t, mount_t, struct nfsm_chain *, int, thread_t, kauth_cred_t, struct nfsreq_secinfo_args *, int, struct nfsm_chain *, u_int64_t *, int *);
int     nfs_request_gss(mount_t, struct nfsm_chain *, thread_t, kauth_cred_t, int, struct nfs_gss_clnt_ctx *, struct nfsm_chain *, int *);
int     nfs_request_async(nfsnode_t, mount_t, struct nfsm_chain *, int, thread_t, kauth_cred_t, struct nfsreq_secinfo_args *, int, struct nfsreq_cbinfo *, struct nfsreq **);
int     nfs_request_async_finish(struct nfsreq *, struct nfsm_chain *, u_int64_t *, int *);
void    nfs_request_async_cancel(struct nfsreq *);
void    nfs_request_timer(void *, void *);
int     nfs_request_using_gss(struct nfsreq *);
void    nfs_get_xid(uint64_t *);
int     nfs_sigintr(struct nfsmount *, struct nfsreq *, thread_t, int);
int     nfs_noremotehang(thread_t);

int     nfs_send(struct nfsreq *, int);
int     nfs_sndlock(struct nfsreq *);
void    nfs_sndunlock(struct nfsreq *);

int     nfs_uaddr2sockaddr(const char *, struct sockaddr *);

int     nfs_aux_request(struct nfsmount *, thread_t, struct sockaddr *, socket_t, int, mbuf_t, uint32_t, int, int, struct nfsm_chain *);
int     nfs_portmap_lookup(struct nfsmount *, vfs_context_t, struct sockaddr *, socket_t, uint32_t, uint32_t, uint32_t, int);

void    nfs_location_next(struct nfs_fs_locations *, struct nfs_location_index *);
int     nfs_location_index_cmp(struct nfs_location_index *, struct nfs_location_index *);
void    nfs_location_mntfromname(struct nfs_fs_locations *, struct nfs_location_index, char *, size_t, int);
int     nfs_socket_create(struct nfsmount *, struct sockaddr *, uint8_t, in_port_t, uint32_t, uint32_t, int, struct nfs_socket **);
void    nfs_socket_destroy(struct nfs_socket *);
void    nfs_socket_options(struct nfsmount *, struct nfs_socket *);
void    nfs_connect_upcall(socket_t, void *, int);
int     nfs_connect_error_class(int);
int     nfs_connect_search_loop(struct nfsmount *, struct nfs_socket_search *);
void    nfs_socket_search_update_error(struct nfs_socket_search *, int);
void    nfs_socket_search_cleanup(struct nfs_socket_search *);
void    nfs_mount_connect_thread(void *, __unused wait_result_t);

int     nfs_lookitup(nfsnode_t, char *, int, vfs_context_t, nfsnode_t *);
void    nfs_dulookup_init(struct nfs_dulookup *, nfsnode_t, const char *, int, vfs_context_t);
void    nfs_dulookup_start(struct nfs_dulookup *, nfsnode_t, vfs_context_t);
void    nfs_dulookup_finish(struct nfs_dulookup *, nfsnode_t, vfs_context_t);
int     nfs_dir_buf_cache_lookup(nfsnode_t, nfsnode_t *, struct componentname *, vfs_context_t, int, int *);
int     nfs_dir_buf_search(struct nfsbuf *, struct componentname *, fhandle_t *, struct nfs_vattr *, uint64_t *, time_t *, daddr64_t *, int);
void    nfs_name_cache_purge(nfsnode_t, nfsnode_t, struct componentname *, vfs_context_t);

#if CONFIG_NFS4
uint32_t nfs4_ace_nfstype_to_vfstype(uint32_t, int *);
uint32_t nfs4_ace_vfstype_to_nfstype(uint32_t, int *);
uint32_t nfs4_ace_nfsflags_to_vfsflags(uint32_t);
uint32_t nfs4_ace_vfsflags_to_nfsflags(uint32_t);
uint32_t nfs4_ace_nfsmask_to_vfsrights(uint32_t);
uint32_t nfs4_ace_vfsrights_to_nfsmask(uint32_t);
int nfs4_id2guid(char *, guid_t *, int);
int nfs4_guid2id(guid_t *, char *, size_t *, int);

int     nfs4_parsefattr(struct nfsm_chain *, struct nfs_fsattr *, struct nfs_vattr *, fhandle_t *, struct dqblk *, struct nfs_fs_locations *);
#endif

int     nfs_parsefattr(struct nfsmount *nmp, struct nfsm_chain *, int,
    struct nfs_vattr *);
void    nfs_vattr_set_supported(uint32_t *, struct vnode_attr *);
void    nfs_vattr_set_bitmap(struct nfsmount *, uint32_t *, struct vnode_attr *);
void    nfs3_pathconf_cache(struct nfsmount *, struct nfs_fsattr *);
int     nfs3_check_lockmode(struct nfsmount *, struct sockaddr *, int, int);
int     nfs3_mount_rpc(struct nfsmount *, struct sockaddr *, int, int, char *, vfs_context_t, int, fhandle_t *, struct nfs_sec *);
void    nfs3_umount_rpc(struct nfsmount *, vfs_context_t, int);
void    nfs_rdirplus_update_node_attrs(nfsnode_t, struct direntry *, fhandle_t *, struct nfs_vattr *, uint64_t *);
int     nfs_node_access_slot(nfsnode_t, uid_t, int);
void    nfs_vnode_notify(nfsnode_t, uint32_t);

void    nfs_avoid_needless_id_setting_on_create(nfsnode_t, struct vnode_attr *, vfs_context_t);
int     nfs_open_state_set_busy(nfsnode_t, thread_t);
void    nfs_open_state_clear_busy(nfsnode_t);
struct nfs_open_owner *nfs_open_owner_find(struct nfsmount *, kauth_cred_t, int);
void    nfs_open_owner_destroy(struct nfs_open_owner *);
void    nfs_open_owner_ref(struct nfs_open_owner *);
void    nfs_open_owner_rele(struct nfs_open_owner *);
int     nfs_open_owner_set_busy(struct nfs_open_owner *, thread_t);
void    nfs_open_owner_clear_busy(struct nfs_open_owner *);
void    nfs_owner_seqid_increment(struct nfs_open_owner *, struct nfs_lock_owner *, int);
int     nfs_open_file_find(nfsnode_t, struct nfs_open_owner *, struct nfs_open_file **, uint32_t, uint32_t, int);
int     nfs_open_file_find_internal(nfsnode_t, struct nfs_open_owner *, struct nfs_open_file **, uint32_t, uint32_t, int);
void    nfs_open_file_destroy(struct nfs_open_file *);
int     nfs_open_file_set_busy(struct nfs_open_file *, thread_t);
void    nfs_open_file_clear_busy(struct nfs_open_file *);
void    nfs_open_file_add_open(struct nfs_open_file *, uint32_t, uint32_t, int);
void    nfs_open_file_remove_open_find(struct nfs_open_file *, uint32_t, uint32_t, uint8_t *, uint8_t *, int *);
void    nfs_open_file_remove_open(struct nfs_open_file *, uint32_t, uint32_t);
void    nfs_get_stateid(nfsnode_t, thread_t, kauth_cred_t, nfs_stateid *);
int     nfs_check_for_locks(struct nfs_open_owner *, struct nfs_open_file *);
int     nfs_close(nfsnode_t, struct nfs_open_file *, uint32_t, uint32_t, vfs_context_t);

void    nfs_release_open_state_for_node(nfsnode_t, int);
void    nfs_revoke_open_state_for_node(nfsnode_t);
struct nfs_lock_owner *nfs_lock_owner_find(nfsnode_t, proc_t, int);
void    nfs_lock_owner_destroy(struct nfs_lock_owner *);
void    nfs_lock_owner_ref(struct nfs_lock_owner *);
void    nfs_lock_owner_rele(struct nfs_lock_owner *);
int     nfs_lock_owner_set_busy(struct nfs_lock_owner *, thread_t);
void    nfs_lock_owner_clear_busy(struct nfs_lock_owner *);
void    nfs_lock_owner_insert_held_lock(struct nfs_lock_owner *, struct nfs_file_lock *);
struct nfs_file_lock *nfs_file_lock_alloc(struct nfs_lock_owner *);
void    nfs_file_lock_destroy(struct nfs_file_lock *);
int     nfs_file_lock_conflict(struct nfs_file_lock *, struct nfs_file_lock *, int *);
int     nfs_unlock_rpc(nfsnode_t, struct nfs_lock_owner *, int, uint64_t, uint64_t, thread_t, kauth_cred_t, int);
int     nfs_advlock_getlock(nfsnode_t, struct nfs_lock_owner *, struct flock *, uint64_t, uint64_t, vfs_context_t);
int     nfs_advlock_setlock(nfsnode_t, struct nfs_open_file *, struct nfs_lock_owner *, int, uint64_t, uint64_t, int, short, vfs_context_t);
int     nfs_advlock_unlock(nfsnode_t, struct nfs_open_file *, struct nfs_lock_owner *, uint64_t, uint64_t, int, vfs_context_t);

#if CONFIG_NFS4
int     nfs4_create_rpc(vfs_context_t, nfsnode_t, struct componentname *, struct vnode_attr *, int, char *, nfsnode_t *);
int     nfs4_open(nfsnode_t, struct nfs_open_file *, uint32_t, uint32_t, vfs_context_t);
int     nfs4_open_delegated(nfsnode_t, struct nfs_open_file *, uint32_t, uint32_t, vfs_context_t);
int     nfs4_reopen(struct nfs_open_file *, thread_t);
int     nfs4_open_rpc(struct nfs_open_file *, vfs_context_t, struct componentname *, struct vnode_attr *, vnode_t, vnode_t *, int, int, int);
int     nfs4_open_rpc_internal(struct nfs_open_file *, vfs_context_t, thread_t, kauth_cred_t, struct componentname *, struct vnode_attr *, vnode_t, vnode_t *, int, int, int);
int     nfs4_open_confirm_rpc(struct nfsmount *, nfsnode_t, u_char *, int, struct nfs_open_owner *, nfs_stateid *, thread_t, kauth_cred_t, struct nfs_vattr *, uint64_t *);
int     nfs4_open_reopen_rpc(struct nfs_open_file *, thread_t, kauth_cred_t, struct componentname *, vnode_t, vnode_t *, int, int);
int     nfs4_open_reclaim_rpc(struct nfs_open_file *, int, int);
int     nfs4_claim_delegated_open_rpc(struct nfs_open_file *, int, int, int);
int     nfs4_claim_delegated_state_for_open_file(struct nfs_open_file *, int);
int     nfs4_claim_delegated_state_for_node(nfsnode_t, int);
int     nfs4_open_downgrade_rpc(nfsnode_t, struct nfs_open_file *, vfs_context_t);
int     nfs4_close_rpc(nfsnode_t, struct nfs_open_file *, thread_t, kauth_cred_t, int);
void    nfs4_delegation_return_enqueue(nfsnode_t);
int     nfs4_delegation_return(nfsnode_t, int, thread_t, kauth_cred_t);
int     nfs4_lock_rpc(nfsnode_t, struct nfs_open_file *, struct nfs_file_lock *, int, int, thread_t, kauth_cred_t);
int     nfs4_delegreturn_rpc(struct nfsmount *, u_char *, int, struct nfs_stateid *, int, thread_t, kauth_cred_t);

nfsnode_t nfs4_named_attr_dir_get(nfsnode_t, int, vfs_context_t);
int     nfs4_named_attr_get(nfsnode_t, struct componentname *, uint32_t, int, vfs_context_t, nfsnode_t *, struct nfs_open_file **);
int     nfs4_named_attr_remove(nfsnode_t, nfsnode_t, const char *, vfs_context_t);
#endif

int     nfs_mount_state_in_use_start(struct nfsmount *, thread_t);
int     nfs_mount_state_in_use_end(struct nfsmount *, int);
int     nfs_mount_state_error_should_restart(int);
int     nfs_mount_state_error_delegation_lost(int);
uint    nfs_mount_state_max_restarts(struct nfsmount *);
int     nfs_mount_state_wait_for_recovery(struct nfsmount *);
void    nfs_need_recover(struct nfsmount *nmp, int error);
void    nfs_recover(struct nfsmount *);

int     nfs_vnop_access(struct vnop_access_args *);
int     nfs_vnop_remove(struct vnop_remove_args *);
int     nfs_vnop_read(struct vnop_read_args *);
int     nfs_vnop_write(struct vnop_write_args *);
int     nfs_vnop_open(struct vnop_open_args *);
int     nfs_vnop_close(struct vnop_close_args *);
int     nfs_vnop_advlock(struct vnop_advlock_args *);
int     nfs_vnop_mmap(struct vnop_mmap_args *);
int     nfs_vnop_mmap_check(struct vnop_mmap_check_args *ap);
int     nfs_vnop_mnomap(struct vnop_mnomap_args *);

#if CONFIG_NFS4
int     nfs4_vnop_create(struct vnop_create_args *);
int     nfs4_vnop_mknod(struct vnop_mknod_args *);
int     nfs4_vnop_close(struct vnop_close_args *);
int     nfs4_vnop_getattr(struct vnop_getattr_args *);
int     nfs4_vnop_link(struct vnop_link_args *);
int     nfs4_vnop_mkdir(struct vnop_mkdir_args *);
int     nfs4_vnop_rmdir(struct vnop_rmdir_args *);
int     nfs4_vnop_symlink(struct vnop_symlink_args *);
int     nfs4_vnop_getxattr(struct vnop_getxattr_args *);
int     nfs4_vnop_setxattr(struct vnop_setxattr_args *);
int     nfs4_vnop_removexattr(struct vnop_removexattr_args *);
int     nfs4_vnop_listxattr(struct vnop_listxattr_args *);
#if NAMEDSTREAMS
int     nfs4_vnop_getnamedstream(struct vnop_getnamedstream_args *);
int     nfs4_vnop_makenamedstream(struct vnop_makenamedstream_args *);
int     nfs4_vnop_removenamedstream(struct vnop_removenamedstream_args *);
#endif

int     nfs4_access_rpc(nfsnode_t, u_int32_t *, int, vfs_context_t);
int     nfs4_getattr_rpc(nfsnode_t, mount_t, u_char *, size_t, int, vfs_context_t, struct nfs_vattr *, u_int64_t *);
int     nfs4_setattr_rpc(nfsnode_t, struct vnode_attr *, vfs_context_t);
int     nfs4_read_rpc_async(nfsnode_t, off_t, size_t, thread_t, kauth_cred_t, struct nfsreq_cbinfo *, struct nfsreq **);
int     nfs4_read_rpc_async_finish(nfsnode_t, struct nfsreq *, uio_t, size_t *, int *);
int     nfs4_write_rpc_async(nfsnode_t, uio_t, size_t, thread_t, kauth_cred_t, int, struct nfsreq_cbinfo *, struct nfsreq **);
int     nfs4_write_rpc_async_finish(nfsnode_t, struct nfsreq *, int *, size_t *, uint64_t *);
int     nfs4_readdir_rpc(nfsnode_t, struct nfsbuf *, vfs_context_t);
int     nfs4_readlink_rpc(nfsnode_t, char *, size_t *, vfs_context_t);
int     nfs4_commit_rpc(nfsnode_t, uint64_t, uint64_t, kauth_cred_t, uint64_t);
int     nfs4_lookup_rpc_async(nfsnode_t, char *, int, vfs_context_t, struct nfsreq **);
int     nfs4_lookup_rpc_async_finish(nfsnode_t, char *, int, vfs_context_t, struct nfsreq *, u_int64_t *, fhandle_t *, struct nfs_vattr *);
int     nfs4_remove_rpc(nfsnode_t, char *, int, thread_t, kauth_cred_t);
int     nfs4_rename_rpc(nfsnode_t, char *, int, nfsnode_t, char *, int, vfs_context_t);
int     nfs4_pathconf_rpc(nfsnode_t, struct nfs_fsattr *, vfs_context_t);
int     nfs4_setlock_rpc(nfsnode_t, struct nfs_open_file *, struct nfs_file_lock *, int, int, thread_t, kauth_cred_t);
int     nfs4_unlock_rpc(nfsnode_t, struct nfs_lock_owner *, int, uint64_t, uint64_t, int, thread_t, kauth_cred_t);
int     nfs4_getlock_rpc(nfsnode_t, struct nfs_lock_owner *, struct flock *, uint64_t, uint64_t, vfs_context_t);
#endif

int     nfs_read_rpc(nfsnode_t, uio_t, vfs_context_t);
int     nfs_write_rpc(nfsnode_t, uio_t, vfs_context_t, int *, uint64_t *);
int     nfs_write_rpc2(nfsnode_t, uio_t, thread_t, kauth_cred_t, int *, uint64_t *);

int     nfs3_access_rpc(nfsnode_t, u_int32_t *, int, vfs_context_t);
int     nfs3_getattr_rpc(nfsnode_t, mount_t, u_char *, size_t, int, vfs_context_t, struct nfs_vattr *, u_int64_t *);
int     nfs3_setattr_rpc(nfsnode_t, struct vnode_attr *, vfs_context_t);
int     nfs3_read_rpc_async(nfsnode_t, off_t, size_t, thread_t, kauth_cred_t, struct nfsreq_cbinfo *, struct nfsreq **);
int     nfs3_read_rpc_async_finish(nfsnode_t, struct nfsreq *, uio_t, size_t *, int *);
int     nfs3_write_rpc_async(nfsnode_t, uio_t, size_t, thread_t, kauth_cred_t, int, struct nfsreq_cbinfo *, struct nfsreq **);
int     nfs3_write_rpc_async_finish(nfsnode_t, struct nfsreq *, int *, size_t *, uint64_t *);
int     nfs3_readdir_rpc(nfsnode_t, struct nfsbuf *, vfs_context_t);
int     nfs3_readlink_rpc(nfsnode_t, char *, size_t *, vfs_context_t);
int     nfs3_commit_rpc(nfsnode_t, uint64_t, uint64_t, kauth_cred_t, uint64_t);
int     nfs3_lookup_rpc_async(nfsnode_t, char *, int, vfs_context_t, struct nfsreq **);
int     nfs3_lookup_rpc_async_finish(nfsnode_t, char *, int, vfs_context_t, struct nfsreq *, u_int64_t *, fhandle_t *, struct nfs_vattr *);
int     nfs3_remove_rpc(nfsnode_t, char *, int, thread_t, kauth_cred_t);
int     nfs3_rename_rpc(nfsnode_t, char *, int, nfsnode_t, char *, int, vfs_context_t);
int     nfs3_pathconf_rpc(nfsnode_t, struct nfs_fsattr *, vfs_context_t);
int     nfs3_setlock_rpc(nfsnode_t, struct nfs_open_file *, struct nfs_file_lock *, int, int, thread_t, kauth_cred_t);
int     nfs3_unlock_rpc(nfsnode_t, struct nfs_lock_owner *, int, uint64_t, uint64_t, int, thread_t, kauth_cred_t);
int     nfs3_getlock_rpc(nfsnode_t, struct nfs_lock_owner *, struct flock *, uint64_t, uint64_t, vfs_context_t);

void    nfsrv_active_user_list_reclaim(void);
void    nfsrv_cleancache(void);
void    nfsrv_cleanup(void);
int     nfsrv_credcheck(struct nfsrv_descript *, vfs_context_t, struct nfs_export *,
    struct nfs_export_options *);
void    nfsrv_idlesock_timer(void *, void *);
int     nfsrv_dorec(struct nfsrv_sock *, struct nfsd *, struct nfsrv_descript **);
int     nfsrv_errmap(struct nfsrv_descript *, int);
int     nfsrv_export(struct user_nfs_export_args *, vfs_context_t);
int     nfsrv_fhmatch(struct nfs_filehandle *, struct nfs_filehandle *);
int     nfsrv_fhtovp(struct nfs_filehandle *, struct nfsrv_descript *, vnode_t *,
    struct nfs_export **, struct nfs_export_options **);
int     nfsrv_check_exports_allow_address(mbuf_t);
#if CONFIG_FSE
void    nfsrv_fmod_timer(void *, void *);
#endif
int     nfsrv_getcache(struct nfsrv_descript *, struct nfsrv_sock *, mbuf_t *);
void    nfsrv_group_sort(gid_t *, int);
void    nfsrv_init(void);
void    nfsrv_initcache(void);
int     nfsrv_is_initialized(void);
int     nfsrv_namei(struct nfsrv_descript *, vfs_context_t, struct nameidata *,
    struct nfs_filehandle *, vnode_t *,
    struct nfs_export **, struct nfs_export_options **);
void    nfsrv_rcv(socket_t, void *, int);
void    nfsrv_rcv_locked(socket_t, struct nfsrv_sock *, int);
int     nfsrv_rephead(struct nfsrv_descript *, struct nfsrv_sock *, struct nfsm_chain *, size_t);
int     nfsrv_send(struct nfsrv_sock *, mbuf_t, mbuf_t);
void    nfsrv_updatecache(struct nfsrv_descript *, int, mbuf_t);
void    nfsrv_update_user_stat(struct nfs_export *, struct nfsrv_descript *, uid_t, u_int, u_int, u_int);
int     nfsrv_vptofh(struct nfs_export *, int, struct nfs_filehandle *,
    vnode_t, vfs_context_t, struct nfs_filehandle *);
void    nfsrv_wakenfsd(struct nfsrv_sock *);
void    nfsrv_wg_timer(void *, void *);
int     nfsrv_writegather(struct nfsrv_descript **, struct nfsrv_sock *,
    vfs_context_t, mbuf_t *);

int     nfsrv_access(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_commit(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_create(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_fsinfo(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_getattr(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_link(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_lookup(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_mkdir(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_mknod(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_noop(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_null(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_pathconf(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_read(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_readdir(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_readdirplus(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_readlink(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_remove(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_rename(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_rmdir(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_setattr(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_statfs(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_symlink(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);
int     nfsrv_write(struct nfsrv_descript *, struct nfsrv_sock *, vfs_context_t, mbuf_t *);

void    nfs_interval_timer_start(thread_call_t, time_t);
int     nfs_use_cache(struct nfsmount *);
void    nfs_up(struct nfsmount *, thread_t, int, const char *);
void    nfs_down(struct nfsmount *, thread_t, int, int, const char *, int);
int     nfs_msg(thread_t, const char *, const char *, int);

int     nfs_mountroot(void);
struct nfs_diskless;
int     nfs_boot_init(struct nfs_diskless *);
int     nfs_boot_getfh(struct nfs_diskless *, int, int);

#if CONFIG_TRIGGERS
resolver_result_t nfs_mirror_mount_trigger_resolve(vnode_t, const struct componentname *, enum path_operation, int, void *, vfs_context_t);
resolver_result_t nfs_mirror_mount_trigger_unresolve(vnode_t, int, void *, vfs_context_t);
resolver_result_t nfs_mirror_mount_trigger_rearm(vnode_t, int, void *, vfs_context_t);
int     nfs_mirror_mount_domount(vnode_t, vnode_t, vfs_context_t);
void    nfs_ephemeral_mount_harvester_start(void);
void    nfs_ephemeral_mount_harvester(__unused void *arg, __unused wait_result_t wr);
#endif

                              
void nfsrv_uc_init(void);
void nfsrv_uc_cleanup(void);
void nfsrv_uc_addsock(struct nfsrv_sock *, int);
void nfsrv_uc_dequeue(struct nfsrv_sock *);

                   
#define NFS_DEBUG_LEVEL   (nfs_debug_ctl & 0xf)
#define NFS_DEBUG_FACILITY ((nfs_debug_ctl >> 4) & 0xfff)
#define NFS_DEBUG_FLAGS ((nfs_debug_ctl >> 16) & 0xf)
#define NFS_DEBUG_VALUE ((nfs_debug_ctl >> 20) & 0xfff)
#define NFS_FAC_SOCK    0x001
#define NFS_FAC_STATE   0x002
#define NFS_FAC_NODE    0x004
#define NFS_FAC_VNOP    0x008
#define NFS_FAC_BIO     0x010
#define NFS_FAC_GSS     0x020
#define NFS_FAC_VFS     0x040
#define NFS_FAC_SRV     0x080

#define NFS_IS_DBG(fac, lev) \
	(__builtin_expect((NFS_DEBUG_FACILITY & (fac)) && ((lev) <= NFS_DEBUG_LEVEL), 0))
#define NFS_DBG(fac, lev, fmt, ...)  nfs_printf((fac), (lev), "%s: %d: " fmt, __func__, __LINE__, ## __VA_ARGS__)

void nfs_printf(unsigned int, unsigned int, const char *, ...) __printflike(3, 4);
void nfs_dump_mbuf(const char *, int, const char *, mbuf_t);
int  nfs_mountopts(struct nfsmount *, char *, int);

#if XNU_KERNEL_PRIVATE
#include <kern/kalloc.h>

ZONE_VIEW_DECLARE(ZV_NFSDIROFF);
extern zone_t nfs_buf_zone;
extern zone_t nfsrv_descript_zone;
extern zone_t nfsnode_zone;
extern zone_t nfs_fhandle_zone;
extern zone_t nfs_req_zone;
extern zone_t nfsrv_descript_zone;
extern zone_t nfsmnt_zone;

#endif                         
__END_DECLS

#endif              
#endif                          

#endif
