/*
 * Copyright (c) 2007-2015 Apple Inc. All rights reserved.
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

#ifndef _NFS_NFS_GSS_H_
#define _NFS_NFS_GSS_H_

#include "gss/gss_krb5_mech.h"
#include <gssd/gssd_mach.h>
#include <sys/param.h>
#include <nfs/nfs_ioctl.h>

#define RPCSEC_GSS                      6
#define RPCSEC_GSS_VERS_1               1

enum rpcsec_gss_proc {
	RPCSEC_GSS_DATA                 = 0,
	RPCSEC_GSS_INIT                 = 1,
	RPCSEC_GSS_CONTINUE_INIT        = 2,
	RPCSEC_GSS_DESTROY              = 3
};

enum rpcsec_gss_service {
	RPCSEC_GSS_SVC_NONE             = 1,               
	RPCSEC_GSS_SVC_INTEGRITY        = 2,                
	RPCSEC_GSS_SVC_PRIVACY          = 3,                
};

                      
extern u_char krb5_mech_oid[11];


  
                                                               
                                                              
  
                                                               
                                                        
   

#define GSS_MAX_CTX_HANDLE_LEN          256
#define GSS_MAX_TOKEN_LEN               64*1024

  
                                         
   
#define GSS_MAX_MIC_LEN                 2048

#define GSS_MAXSEQ                      0x80000000                                    
#define GSS_SVC_MAXCONTEXTS             500000                                   
#define GSS_SVC_SEQWINDOW               256                                        
#define GSS_CLNT_SEQLISTMAX             32                                               

#define MAX_SKEYLEN     32
#define MAX_LUCIDLEN    (sizeof (lucid_context) + MAX_SKEYLEN)
#define GSS_MAX_NEG_CACHE_ENTRIES 16
#define GSS_NEG_CACHE_TO 3
#define GSS_PRINT_DELAY  (8 * 3600)                                                       

  
                                              
   
struct nfs_gss_clnt_ctx {
	lck_mtx_t               gss_clnt_mtx;
	thread_t                gss_clnt_thread;                                  
	TAILQ_ENTRY(nfs_gss_clnt_ctx)   gss_clnt_entries;
	uint32_t                gss_clnt_flags;                                 
	int32_t                 gss_clnt_refcnt;                          
	kauth_cred_t            gss_clnt_cred;                                  
	uint8_t                 *gss_clnt_principal;                                           
	size_t                  gss_clnt_prinlen;                             
	gssd_nametype           gss_clnt_prinnt;                                 
	char                    *gss_clnt_display;                                  
	uint32_t                gss_clnt_proc;                                      
	uint32_t                gss_clnt_seqnum;                              
	uint32_t                gss_clnt_service;                                        
	uint8_t                 *gss_clnt_handle;                                   
	uint32_t                gss_clnt_handle_len;                                  
	time_t                  gss_clnt_nctime;                                                     
	uint32_t                gss_clnt_seqwin;                                  
	uint32_t                *gss_clnt_seqbits;                                           
	mach_port_t             gss_clnt_mport;                                     
	uint32_t                gss_clnt_verflen;                                         
	uint8_t                 *gss_clnt_verf;                                    
	uint8_t                 *gss_clnt_svcname;                                              
	size_t                  gss_clnt_svcnamlen;                           
	gssd_nametype           gss_clnt_svcnt;                             
	gssd_cred               gss_clnt_cred_handle;                                  
	gssd_ctx                gss_clnt_context;                                         
	gss_ctx_id_t            gss_clnt_ctx_id;                                 
	uint8_t                 *gss_clnt_token;                                                
	uint32_t                gss_clnt_tokenlen;                        
	uint32_t                gss_clnt_gssd_flags;                                
	uint32_t                gss_clnt_major;                                                
	uint32_t                gss_clnt_minor;                                                
	time_t                  gss_clnt_ptime;                                               
};

  
                 
   
#define GSS_CTX_COMPLETE        0x00000001                            
#define GSS_CTX_INVAL           0x00000002                           
#define GSS_CTX_STICKY          0x00000004                                     
#define GSS_NEEDSEQ             0x00000008                               
#define GSS_NEEDCTX             0x00000010                         
#define GSS_CTX_DESTROY         0x00000020                                                
#define GSS_CTX_USECOUNT        0x00000040                                                  

  
                                              
   
struct nfs_gss_svc_ctx {
	lck_mtx_t               gss_svc_mtx;
	LIST_ENTRY(nfs_gss_svc_ctx)     gss_svc_entries;
	uint32_t                gss_svc_handle;                                               
	uint32_t                gss_svc_refcnt;                           
	uint32_t                gss_svc_proc;                                        
	uid_t                   gss_svc_uid;                               
	gid_t                   gss_svc_gids[NGROUPS];                      
	uint32_t                gss_svc_ngroups;                        
	uint64_t                gss_svc_incarnation;                                               
	uint32_t                gss_svc_seqmax;                                           
	uint32_t                gss_svc_seqwin;                                      
	uint32_t                *gss_svc_seqbits;                                     
	gssd_cred               gss_svc_cred_handle;                                   
	gssd_ctx                gss_svc_context;                                          
	gss_ctx_id_t            gss_svc_ctx_id;                                  
	u_char                  *gss_svc_token;                                                 
	uint32_t                gss_svc_tokenlen;                         
	uint32_t                gss_svc_major;                                       
	uint32_t                gss_svc_minor;                                       
};

#define SVC_CTX_HASHSZ  64
#define SVC_CTX_HASH(handle)    ((handle) % SVC_CTX_HASHSZ)
LIST_HEAD(nfs_gss_svc_ctx_hashhead, nfs_gss_svc_ctx);

  
                                                   
   
#define win_getbit(bits, bit)      ((bits[(bit) / 32] &   (1 << (bit) % 32)) != 0)
#define win_setbit(bits, bit)   do { bits[(bit) / 32] |=  (1 << (bit) % 32); } while (0)
#define win_resetbit(bits, bit) do { bits[(bit) / 32] &= ~(1 << (bit) % 32); } while (0)

  
                             
   
#define GSS_CTX_PEND            5                         
#define GSS_CTX_EXPIRE          (8 * 3600)                
#define GSS_CTX_TTL_MIN         1                         
#define GSS_TIMER_PERIOD        300                       
#define MSECS_PER_SEC           1000

#define auth_is_kerberized(auth) \
	(auth == RPCAUTH_KRB5 || \
	 auth == RPCAUTH_KRB5I || \
	 auth == RPCAUTH_KRB5P)

__BEGIN_DECLS

void    nfs_gss_init(void);
uid_t   nfs_cred_getasid2uid(kauth_cred_t);
int     nfs_gss_clnt_cred_put(struct nfsreq *, struct nfsm_chain *, mbuf_t);
int     nfs_gss_clnt_verf_get(struct nfsreq *, struct nfsm_chain *,
    uint32_t, uint32_t, uint32_t *);
void    nfs_gss_clnt_rpcdone(struct nfsreq *);
int     nfs_gss_clnt_args_restore(struct nfsreq *);
int     nfs_gss_clnt_ctx_renew(struct nfsreq *);
void    nfs_gss_clnt_ctx_ref(struct nfsreq *, struct nfs_gss_clnt_ctx *);
void    nfs_gss_clnt_ctx_unref(struct nfsreq *);
void    nfs_gss_clnt_ctx_unmount(struct nfsmount *);
int     nfs_gss_clnt_ctx_remove(struct nfsmount *, kauth_cred_t);
int     nfs_gss_clnt_ctx_set_principal(struct nfsmount *, vfs_context_t, uint8_t *, size_t, uint32_t);
int     nfs_gss_clnt_ctx_get_principal(struct nfsmount *, vfs_context_t, struct user_nfs_gss_principal *);
int     nfs_gss_svc_cred_get(struct nfsrv_descript *, struct nfsm_chain *);
int     nfs_gss_svc_verf_put(struct nfsrv_descript *, struct nfsm_chain *);
int     nfs_gss_svc_ctx_init(struct nfsrv_descript *, struct nfsrv_sock *, mbuf_t *);
int     nfs_gss_svc_prepare_reply(struct nfsrv_descript *, struct nfsm_chain *);
int     nfs_gss_svc_protect_reply(struct nfsrv_descript *, mbuf_t);
void    nfs_gss_svc_ctx_deref(struct nfs_gss_svc_ctx *);
void    nfs_gss_svc_cleanup(void);

__END_DECLS
#endif                      
