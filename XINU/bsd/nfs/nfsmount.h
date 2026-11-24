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
 *	@(#)nfsmount.h	8.3 (Berkeley) 3/30/95
 * FreeBSD-Id: nfsmount.h,v 1.13 1997/08/16 19:16:05 wollman Exp $
 */


#ifndef _NFS_NFSMOUNT_H_
#define _NFS_NFSMOUNT_H_

#include <sys/appleapiopts.h>

#ifdef __APPLE_API_PRIVATE

  
                                   
   
struct nfs_fsattr {
	uint32_t        nfsa_flags;                                    
	uint32_t        nfsa_lease;                                        
	int32_t         nfsa_maxname;                                      
	uint32_t        nfsa_maxlink;                                
	uint32_t        nfsa_bsize;                             
	uint32_t        nfsa_pad;                           
	uint64_t        nfsa_maxfilesize;                              
	uint32_t        nfsa_maxread;                                  
	uint32_t        nfsa_maxwrite;                                  
	uint64_t        nfsa_files_avail;                                 
	uint64_t        nfsa_files_free;                             
	uint64_t        nfsa_files_total;                             
	uint64_t        nfsa_space_avail;                                 
	uint64_t        nfsa_space_free;                             
	uint64_t        nfsa_space_total;                             
	uint32_t        nfsa_supp_attr[NFS_ATTR_BITMAP_LEN];                                               
	uint32_t        nfsa_bitmap[NFS_ATTR_BITMAP_LEN];                       
};
#define NFS_FSFLAG_LINK                 0x00000001
#define NFS_FSFLAG_SYMLINK              0x00000002
#define NFS_FSFLAG_UNIQUE_FH            0x00000004
#define NFS_FSFLAG_ACL                  0x00000008
#define NFS_FSFLAG_SET_TIME             0x00000010
#define NFS_FSFLAG_CASE_INSENSITIVE     0x00000020
#define NFS_FSFLAG_CASE_PRESERVING      0x00000040
#define NFS_FSFLAG_CHOWN_RESTRICTED     0x00000080
#define NFS_FSFLAG_HOMOGENEOUS          0x00000100
#define NFS_FSFLAG_NO_TRUNC             0x00000200
#define NFS_FSFLAG_NAMED_ATTR           0x00000400
#define NFS_FSFLAG_FHTYPE_MASK          0xFF000000
#define NFS_FSFLAG_FHTYPE_SHIFT         24

  
                                      
   
struct nfs_fs_server {
	char *                  ns_name;                                    
	char **                 ns_addresses;                                              
	uint32_t                ns_addrcount;                               
};
struct nfs_fs_path {
	char **                 np_components;                                           
	uint32_t                np_compcount;                                     
};
struct nfs_fs_location {
	struct nfs_fs_server ** nl_servers;                                           
	struct nfs_fs_path      nl_path;                                      
	uint32_t                nl_servcount;                             
};

struct nfs_location_index {
	uint8_t                 nli_flags;                              
	uint8_t                 nli_loc;                                    
	uint8_t                 nli_serv;                                 
	uint8_t                 nli_addr;                                  
};
#define NLI_VALID       0x01                        

struct nfs_fs_locations {
	struct nfs_fs_path      nl_root;                                                            
	uint32_t                nl_numlocs;                                 
	struct nfs_location_index nl_current;                                                         
	struct nfs_fs_location **nl_locations;                                     
};

  
                                  
   
struct nfs_rpc_record_state {
	mbuf_t                  nrrs_m;                                               
	mbuf_t                  nrrs_mlast;
	uint16_t                nrrs_lastfrag;                                       
	uint16_t                nrrs_markerleft;                                    
	uint32_t                nrrs_fragleft;                                        
	uint32_t                nrrs_reclen;                                      
};

  
                        
   
struct nfs_socket {
	lck_mtx_t               nso_lock;                                    
	TAILQ_ENTRY(nfs_socket) nso_link;                                    
	struct sockaddr *       nso_saddr;                                  
	struct sockaddr *       nso_saddr2;                                            
	void *                  nso_wake;                                       
	time_t                  nso_timestamp;
	time_t                  nso_reqtimestamp;                              
	socket_t                nso_so;                             
	uint8_t                 nso_sotype;                                 
	uint16_t                nso_flags;                               
	struct nfs_location_index nso_location;                             
	uint32_t                nso_protocol;                             
	uint32_t                nso_version;                                      
	uint32_t                nso_pingxid;                                              
	uint32_t                nso_nfs_min_vers;                                                       
	uint32_t                nso_nfs_max_vers;                                                       
	int                     nso_error;                                      
	struct nfs_rpc_record_state nso_rrs;                                                
};

TAILQ_HEAD(nfssocketlist, nfs_socket);

               
#define NSO_UPCALL              0x0001                                                 
#define NSO_DEAD                0x0002                                      
#define NSO_CONNECTING          0x0004                                                 
#define NSO_CONNECTED           0x0008                                                  
#define NSO_PINGING             0x0010                                              
#define NSO_VERIFIED            0x0020                                                 
#define NSO_DISCONNECTING       0x0040                                                    

                                     
struct nfs_socket_search {
	struct nfs_location_index nss_startloc;                                      
	struct nfs_location_index nss_nextloc;                                   
	struct nfssocketlist    nss_socklist;                                       
	time_t                  nss_timestamp;                                 
	time_t                  nss_last;                                             
	struct nfs_socket *     nss_sock;                                 
	uint8_t                 nss_sotype;                          
	uint8_t                 nss_sockcnt;                                     
	in_port_t               nss_port;                                         
	uint32_t                nss_protocol;                             
	uint32_t                nss_version;                                      
	uint32_t                nss_flags;                               
	int                     nss_addrcnt;                                                 
	int                     nss_timeo;                                                   
	int                     nss_error;                                                  
};
               
#define NSS_VERBOSE             0x00000001                                                      
#define NSS_WARNED              0x00000002                                                                     
#define NSS_FALLBACK2PMAP       0x00000004                                                                                   

  
                                                            
   
struct nfs_funcs {
	int     (*nf_mount)(struct nfsmount *, vfs_context_t, nfsnode_t *);
	int     (*nf_update_statfs)(struct nfsmount *, vfs_context_t);
	int     (*nf_getquota)(struct nfsmount *, vfs_context_t, uid_t, int, struct dqblk *);
	int     (*nf_access_rpc)(nfsnode_t, u_int32_t *, int, vfs_context_t);
	int     (*nf_getattr_rpc)(nfsnode_t, mount_t, u_char *, size_t, int, vfs_context_t, struct nfs_vattr *, u_int64_t *);
	int     (*nf_setattr_rpc)(nfsnode_t, struct vnode_attr *, vfs_context_t);
	int     (*nf_read_rpc_async)(nfsnode_t, off_t, size_t, thread_t, kauth_cred_t, struct nfsreq_cbinfo *, struct nfsreq **);
	int     (*nf_read_rpc_async_finish)(nfsnode_t, struct nfsreq *, uio_t, size_t *, int *);
	int     (*nf_readlink_rpc)(nfsnode_t, char *, size_t *, vfs_context_t);
	int     (*nf_write_rpc_async)(nfsnode_t, uio_t, size_t, thread_t, kauth_cred_t, int, struct nfsreq_cbinfo *, struct nfsreq **);
	int     (*nf_write_rpc_async_finish)(nfsnode_t, struct nfsreq *, int *, size_t *, uint64_t *);
	int     (*nf_commit_rpc)(nfsnode_t, uint64_t, uint64_t, kauth_cred_t, uint64_t);
	int     (*nf_lookup_rpc_async)(nfsnode_t, char *, int, vfs_context_t, struct nfsreq **);
	int     (*nf_lookup_rpc_async_finish)(nfsnode_t, char *, int, vfs_context_t, struct nfsreq *, u_int64_t *, fhandle_t *, struct nfs_vattr *);
	int     (*nf_remove_rpc)(nfsnode_t, char *, int, thread_t, kauth_cred_t);
	int     (*nf_rename_rpc)(nfsnode_t, char *, int, nfsnode_t, char *, int, vfs_context_t);
	int     (*nf_setlock_rpc)(nfsnode_t, struct nfs_open_file *, struct nfs_file_lock *, int, int, thread_t, kauth_cred_t);
	int     (*nf_unlock_rpc)(nfsnode_t, struct nfs_lock_owner *, int, uint64_t, uint64_t, int, thread_t, kauth_cred_t);
	int     (*nf_getlock_rpc)(nfsnode_t, struct nfs_lock_owner *, struct flock *, uint64_t, uint64_t, vfs_context_t);
};

  
                                        
   
struct nfs_client_id {
	TAILQ_ENTRY(nfs_client_id)      nci_link;                               
	char                            *nci_id;                              
	long                            nci_idlen;                                      
};
TAILQ_HEAD(nfsclientidlist, nfs_client_id);
extern struct nfsclientidlist nfsclientids;

  
                   
                                    
                                            
   
struct nfsmount {
	lck_mtx_t nm_lock;                                  
	char *  nm_args;                                          
	uint32_t nm_mattrs[NFS_MATTR_BITMAP_LEN];                                     
	uint32_t nm_mflags_mask[NFS_MFLAG_BITMAP_LEN];                                     
	uint32_t nm_mflags[NFS_MFLAG_BITMAP_LEN];                                
	uint32_t nm_flags[NFS_MFLAG_BITMAP_LEN];                                               
	char *  nm_realm;                                          
	char *  nm_principal;                                                      
	char *  nm_sprinc;                                                    
	int     nm_ref;                                                    
	int     nm_state;                                         
	int     nm_vers;                                 
	uint32_t nm_minor_vers;                                     
	uint32_t nm_min_vers;                                              
	uint32_t nm_max_vers;                                              
	const struct nfs_funcs *nm_funcs;                                
	kauth_cred_t nm_mcred;                                             
	mount_t nm_mountp;                                                     
	nfsnode_t nm_dnp;                                                   
	struct nfs_fs_locations nm_locations;                            
	uint32_t nm_numgrps;                                         
	TAILQ_HEAD(, nfs_gss_clnt_ctx) nm_gsscl;                               
	uint32_t nm_ncentries;                                                  
	int     nm_timeo;                                                   
	int     nm_retry;                                
	uint32_t nm_rsize;                                        
	uint32_t nm_wsize;                                         
	uint32_t nm_biosize;                                 
	uint32_t nm_readdirsize;                                   
	uint32_t nm_readahead;                                           
	time_t   nm_acregmin;                                                
	time_t   nm_acregmax;                                                
	time_t   nm_acdirmin;                                           
	time_t   nm_acdirmax;                                           
	uint32_t nm_auth;                                                         
	uint32_t nm_writers;                                                  
	uint32_t nm_mappers;                                                   
	struct nfs_sec nm_sec;                                                     
	struct nfs_sec nm_servsec;                                                          
	struct nfs_etype nm_etype;                                                                       
	fhandle_t *nm_fh;                                        
	uint32_t  nm_lockmode;                                          
	                
	time_t nm_fsattrstamp;                                      
	struct nfs_fsattr nm_fsattr;                                
	uint64_t nm_verf;                                         
	union {
		struct {                                           
			TAILQ_ENTRY(nfsmount) ldlink;                                               
			int udp_sent;                               
			int udp_cwnd;                                      
			struct nfs_reqqhead udp_cwndq;                               
			struct sockaddr *rqsaddr;                                  
			uint64_t rqsaddrstamp;                                         
		} v3;
		struct {                                        
			struct nfs_client_id *longid;                           
			uint64_t mounttime;                                 
			uint64_t clientid;                            
			thread_call_t renew_timer;                       
			nfs_fsid fsid;                          
			TAILQ_HEAD(, nfsnode) delegations;                                     
			TAILQ_HEAD(, nfsnode) dreturnq;                                               
			TAILQ_ENTRY(nfsmount) cblink;                                               
			uint32_t cbid;                                   
			uint32_t cbrefs;                                   
		} v4;
	} nm_un;
	                  
	TAILQ_HEAD(, nfs_open_owner) nm_open_owners;                          
	uint32_t nm_stateinuse;                                   
	uint32_t nm_stategenid;                                       
	time_t  nm_recover_start;                               
	LIST_HEAD(, nfsnode) nm_monlist;                                    
	                     
	struct nfs_reqqhead nm_resendq;                             
	struct nfs_reqqhead nm_iodq;                                 
	struct nfsiod *nm_niod;                                           
	TAILQ_ENTRY(nfsmount) nm_iodlink;                                      
	int     nm_asyncwrites;                                           
	                  
	uint8_t nm_sofamily;                                                       
	uint8_t nm_sotype;                                              
	in_port_t       nm_nfsport;                            
	in_port_t       nm_mountport;                                    
	char    *nm_nfs_localport;                                              
	char    *nm_mount_localport;                                               
	struct nfs_socket_search *nm_nss;                                      
	struct nfs_socket *nm_nso;                          
	struct sockaddr *nm_saddr;                             
	u_short nm_sockflags;                                   
	time_t  nm_deadto_start;                                     
	time_t  nm_reconnect_start;                               
	int     nm_tprintf_initial_delay;                                      
	int     nm_tprintf_delay;                                        
	int     nm_deadtimeout;                                                                           
	int     nm_curdeadtimeout;                                                                      
	int     nm_srtt[4];                                  
	int     nm_sdrtt[4];
	int     nm_timeouts;                                  
	int     nm_jbreqs;                                             
	int     nm_mounterror;                                       
	TAILQ_ENTRY(nfsmount) nm_pokeq;                             
	thread_t nm_sockthd;                                              
};

                                            
#define NMFLAG(NMP, F)          NFS_BITMAP_ISSET((NMP)->nm_flags, NFS_MFLAG_ ## F)
                                                           
#define NM_OMATTR_GIVEN(NMP, F) NFS_BITMAP_ISSET((NMP)->nm_mattrs, NFS_MATTR_ ## F)
#define NM_OMFLAG_GIVEN(NMP, F) NFS_BITMAP_ISSET((NMP)->nm_mflags_mask, NFS_MFLAG_ ## F)
#define NM_OMFLAG(NMP, F)       NFS_BITMAP_ISSET((NMP)->nm_mflags, NFS_MFLAG_ ## F)

  
                                   
   
#define NFSSTA_MOUNT_THREAD     0x00000040                                        
#define NFSSTA_MONITOR_SCAN     0x00000080                                           
#define NFSSTA_UNMOUNTING       0x00000100                                         
#define NFSSTA_NEEDSECINFO      0x00000200                                   
#define NFSSTA_CLIENTID         0x00000400                                
#define NFSSTA_BIGCOOKIES       0x00000800                                    
#define NFSSTA_JUKEBOXTIMEO     0x00001000                                     
#define NFSSTA_LOCKTIMEO        0x00002000                                      
#define NFSSTA_MOUNTED          0x00004000                          
#define NFSSTA_LOCKSWORK        0x00008000                             
#define NFSSTA_TIMEO            0x00010000                              
#define NFSSTA_FORCE            0x00020000                               
#define NFSSTA_HASWRITEVERF     0x00040000                                 
#define NFSSTA_GOTPATHCONF      0x00080000                                
#define NFSSTA_GOTFSINFO        0x00100000                         
#define NFSSTA_WANTRQUOTA       0x00200000                           
#define NFSSTA_RQUOTAINPROG     0x00400000                              
#define NFSSTA_SENDING          0x00800000                         
#define NFSSTA_SNDLOCK          0x01000000                        
#define NFSSTA_WANTSND          0x02000000                  
#define NFSSTA_DEAD             0x04000000                     
#define NFSSTA_RECOVER          0x08000000                                         
#define NFSSTA_RECOVER_EXPIRED  0x10000000                           
#define NFSSTA_REVOKE           0x20000000                                      
#define NFSSTA_SQUISHY          0x40000000                                           
#define NFSSTA_MOUNT_DRAIN      0x80000000                                    

                            
#define NMSOCK_READY            0x0001                               
#define NMSOCK_CONNECTING       0x0002                                   
#define NMSOCK_SETUP            0x0004                                         
#define NMSOCK_UNMOUNT          0x0008                                          
#define NMSOCK_HASCONNECTED     0x0010                                   
#define NMSOCK_POKE             0x0020                                
#define NMSOCK_DISCONNECTING    0x0080                                    

                                         
#define nm_ldlink       nm_un.v3.ldlink
#define nm_sent         nm_un.v3.udp_sent
#define nm_cwnd         nm_un.v3.udp_cwnd
#define nm_cwndq        nm_un.v3.udp_cwndq
#define nm_rqproto      nm_un.v3.rqproto
#define nm_rqsaddr      nm_un.v3.rqsaddr
#define nm_rqsaddrstamp nm_un.v3.rqsaddrstamp
#define nm_longid       nm_un.v4.longid
#define nm_clientid     nm_un.v4.clientid
#define nm_mounttime    nm_un.v4.mounttime
#define nm_fsid         nm_un.v4.fsid
#define nm_renew_timer  nm_un.v4.renew_timer
#define nm_cbid         nm_un.v4.cbid
#define nm_cblink       nm_un.v4.cblink
#define nm_cbrefs       nm_un.v4.cbrefs
#define nm_delegations  nm_un.v4.delegations
#define nm_dreturnq     nm_un.v4.dreturnq

#if defined(KERNEL)
  
                                                             
   
#define VFSTONFS(mp)    ((mp) ? ((struct nfsmount *)vfs_fsprivate(mp)) : NULL)
#define VTONMP(vp)      VFSTONFS(vnode_mount(vp))
#define NFSTONMP(np)    VTONMP(NFSTOV(np))
#define NFSTOMP(np)     (vnode_mount(NFSTOV(np)))

#endif             

#endif                          
#endif                       
