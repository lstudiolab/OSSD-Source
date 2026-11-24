/*
 * Copyright (c) 2007-2020 Apple Inc. All rights reserved.
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

#include <nfs/nfs_conf.h>
#if CONFIG_NFS

              
                                                                               
                                                                            
                                                         
  
                                                                               
                                                                               
                                                                                     
                                                                               
                                                                                
                                                                                  
                                                                             
                             
  
                                                                               
                                                                           
                                                                               
                                                                            
                                                                                   
                                                                              
                                                                              
                                                                   
  
                                                                          
                                                                            
                                                                              
                                                                      
                                                                            
                                                                            
                                           
  
                                                                              
                                                                               
                                                                       
                                                                              
                                                                            
                                                                            
                                                                               
                                                                             
  
                                                                           
                                                                        
                                                                             
                                                                           
            
   

#include <stdint.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/mount_internal.h>
#include <sys/vnode.h>
#include <sys/ubc.h>
#include <sys/malloc.h>
#include <sys/kpi_mbuf.h>
#include <sys/ucred.h>

#include <kern/host.h>
#include <kern/task.h>
#include <libkern/libkern.h>

#include <mach/task.h>
#include <mach/host_special_ports.h>
#include <mach/host_priv.h>
#include <mach/thread_act.h>
#include <mach/mig_errors.h>
#include <mach/vm_map.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <gssd/gssd_mach.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <nfs/nfs_gss.h>
#include <nfs/nfsmount.h>
#include <nfs/xdr_subs.h>
#include <nfs/nfsm_subs.h>
#include <nfs/nfs_gss.h>
#include <mach_assert.h>
#include <kern/assert.h>

#define ASSERT(EX) assert(EX)

#define NFS_GSS_MACH_MAX_RETRIES 3

#define NFS_GSS_DBG(...) NFS_DBG(NFS_FAC_GSS, 7, ## __VA_ARGS__)
#define NFS_GSS_ISDBG  (NFS_DEBUG_FACILITY &  NFS_FAC_GSS)


#if CONFIG_NFS_SERVER
u_long nfs_gss_svc_ctx_hash;
struct nfs_gss_svc_ctx_hashhead *nfs_gss_svc_ctx_hashtbl;
static LCK_GRP_DECLARE(nfs_gss_svc_grp, "rpcsec_gss_svc");
static LCK_MTX_DECLARE(nfs_gss_svc_ctx_mutex, &nfs_gss_svc_grp);
uint32_t nfsrv_gss_context_ttl = GSS_CTX_EXPIRE;
#define GSS_SVC_CTX_TTL ((uint64_t)max(2*GSS_CTX_PEND, nfsrv_gss_context_ttl) * NSEC_PER_SEC)
#endif                        

#if CONFIG_NFS_CLIENT
LCK_GRP_DECLARE(nfs_gss_clnt_grp, "rpcsec_gss_clnt");
#endif                        

#define KRB5_MAX_MIC_SIZE 128
uint8_t krb5_mech_oid[11] = { 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x12, 0x01, 0x02, 0x02 };
static uint8_t xdrpad[] = { 0x00, 0x00, 0x00, 0x00};

#if CONFIG_NFS_CLIENT
static int      nfs_gss_clnt_ctx_find(struct nfsreq *);
static int      nfs_gss_clnt_ctx_init(struct nfsreq *, struct nfs_gss_clnt_ctx *);
static int      nfs_gss_clnt_ctx_init_retry(struct nfsreq *, struct nfs_gss_clnt_ctx *);
static int      nfs_gss_clnt_ctx_callserver(struct nfsreq *, struct nfs_gss_clnt_ctx *);
static uint8_t  *nfs_gss_clnt_svcname(struct nfsmount *, gssd_nametype *, size_t *);
static int      nfs_gss_clnt_gssd_upcall(struct nfsreq *, struct nfs_gss_clnt_ctx *, uint32_t);
void            nfs_gss_clnt_ctx_neg_cache_reap(struct nfsmount *);
static void     nfs_gss_clnt_ctx_clean(struct nfs_gss_clnt_ctx *);
static int      nfs_gss_clnt_ctx_copy(struct nfs_gss_clnt_ctx *, struct nfs_gss_clnt_ctx **);
static void     nfs_gss_clnt_ctx_destroy(struct nfs_gss_clnt_ctx *);
static void     nfs_gss_clnt_log_error(struct nfsreq *, struct nfs_gss_clnt_ctx *, uint32_t, uint32_t);
#endif                        

#if CONFIG_NFS_SERVER
static struct nfs_gss_svc_ctx *nfs_gss_svc_ctx_find(uint32_t);
static void     nfs_gss_svc_ctx_insert(struct nfs_gss_svc_ctx *);
static void     nfs_gss_svc_ctx_timer(void *, void *);
static int      nfs_gss_svc_gssd_upcall(struct nfs_gss_svc_ctx *);
static int      nfs_gss_svc_seqnum_valid(struct nfs_gss_svc_ctx *, uint32_t);

                                      
static void     nfs_gss_nfsm_chain(struct nfsm_chain *, mbuf_t);
#endif                        

static void     host_release_special_port(mach_port_t);
static mach_port_t host_copy_special_port(mach_port_t);
static void     nfs_gss_mach_alloc_buffer(u_char *, size_t, vm_map_copy_t *);
static int      nfs_gss_mach_vmcopyout(vm_map_copy_t, uint32_t, u_char *);

static int      nfs_gss_mchain_length(mbuf_t);
static int      nfs_gss_append_chain(struct nfsm_chain *, mbuf_t);

#if CONFIG_NFS_SERVER
thread_call_t nfs_gss_svc_ctx_timer_call;
int nfs_gss_timer_on = 0;
uint32_t nfs_gss_ctx_count = 0;
const uint32_t nfs_gss_ctx_max = GSS_SVC_MAXCONTEXTS;
#endif                        

  
                                 
   
void
nfs_gss_init(void)
{
#if CONFIG_NFS_SERVER
	nfs_gss_svc_ctx_hashtbl = hashinit(SVC_CTX_HASHSZ, M_TEMP, &nfs_gss_svc_ctx_hash);

	nfs_gss_svc_ctx_timer_call = thread_call_allocate(nfs_gss_svc_ctx_timer, NULL);
#endif                        
}

  
                                     
   

static errno_t
rpc_gss_prepend_32(mbuf_t *mb, uint32_t value)
{
	int error;
	uint32_t *data;

#if 0
	data = mbuf_data(*mb);
	  
                                                
                                                         
              
    
	if ((uintptr_t)data & 0x3) {
		mbuf_t nmb;

		error = mbuf_get(MBUF_WAITOK, MBUF_TYPE_DATA, &nmb);
		if (error) {
			return error;
		}
		mbuf_setnext(nmb, *mb);
		*mb = nmb;
	}
#endif
	error = mbuf_prepend(mb, sizeof(uint32_t), MBUF_WAITOK);
	if (error) {
		return error;
	}

	data = mbuf_data(*mb);
	*data = txdr_unsigned(value);

	return 0;
}

  
                                                                  
                                                
  
                                                                 
                                                           
   

static errno_t
rpc_gss_data_create(mbuf_t *mbp_head, uint32_t seqnum)
{
	int error;
	mbuf_t mb;
	struct nfsm_chain nmc;
	struct nfsm_chain *nmcp = &nmc;
	uint8_t *data;

	error = mbuf_get(MBUF_WAITOK, MBUF_TYPE_DATA, &mb);
	if (error) {
		return error;
	}
	data = mbuf_data(mb);
#if 0
	                                  
	len = mbuf_maxlen(mb);
	len = (len & ~0x3) - NFSX_UNSIGNED;
	printf("%s: data = %p, len = %d\n", __func__, data, (int)len);
	error = mbuf_setdata(mb, data + len, 0);
	if (error || mbuf_trailingspace(mb)) {
		printf("%s: data = %p trailingspace = %d error = %d\n", __func__, mbuf_data(mb), (int)mbuf_trailingspace(mb), error);
	}
#endif
	                                     
	error = mbuf_setdata(mb, data + 16 * sizeof(uint32_t), 0);
	nfsm_chain_init(nmcp, mb);
	nfsm_chain_add_32(error, nmcp, seqnum);
	nfsm_chain_build_done(error, nmcp);
	if (error) {
		return EINVAL;
	}
	mbuf_setnext(nmcp->nmc_mcur, *mbp_head);
	*mbp_head = nmcp->nmc_mhead;

	return 0;
}

  
                                                                         
                                                                                     
                                                                                     
                         
                                        
   

static errno_t
rpc_gss_integ_data_create(gss_ctx_id_t ctx, mbuf_t *mb_head, uint32_t seqnum, uint32_t *len)
{
	uint32_t error;
	uint32_t major;
	uint32_t length;
	gss_buffer_desc mic;
	struct nfsm_chain nmc;

	                                      
	length = nfs_gss_mchain_length(*mb_head);
	if (len) {
		*len = length;
	}
	error = rpc_gss_data_create(mb_head, seqnum);
	if (error) {
		return error;
	}

	  
                                            
    
	length += NFSX_UNSIGNED;                                             
	major = gss_krb5_get_mic_mbuf(&error, ctx, 0, *mb_head, 0, length, &mic);
	if (major != GSS_S_COMPLETE) {
		printf("gss_krb5_get_mic_mbuf failed %d\n", error);
		return error;
	}

	error = rpc_gss_prepend_32(mb_head, length);
	if (error) {
		return error;
	}

	nfsm_chain_dissect_init(error, &nmc, *mb_head);
	                                                                                                        
	nfsm_chain_adv(error, &nmc, length + NFSX_UNSIGNED);
	nfsm_chain_finish_mbuf(error, &nmc);                                         
	nfsm_chain_add_32(error, &nmc, mic.length);
	nfsm_chain_add_opaque(error, &nmc, mic.value, mic.length);
	nfsm_chain_build_done(error, &nmc);
	gss_release_buffer(NULL, &mic);

                                                               
	assert(nmc.nmc_mhead == *mb_head);

	return error;
}

  
                                                                                         
                                                                         
                                            
                                        
   
static errno_t
rpc_gss_priv_data_create(gss_ctx_id_t ctx, mbuf_t *mb_head, uint32_t seqnum, uint32_t *len)
{
	uint32_t error;
	uint32_t major;
	struct nfsm_chain nmc;
	uint32_t pad;
	uint32_t length;

	error = rpc_gss_data_create(mb_head, seqnum);
	if (error) {
		return error;
	}

	length = nfs_gss_mchain_length(*mb_head);
	major = gss_krb5_wrap_mbuf(&error, ctx, 1, 0, mb_head, 0, length, NULL);
	if (major != GSS_S_COMPLETE) {
		return error;
	}

	length = nfs_gss_mchain_length(*mb_head);
	if (len) {
		*len = length;
	}
	pad = nfsm_pad(length);

	                                                        
	error = rpc_gss_prepend_32(mb_head, length);

	if (error) {
		return error;
	}
	if (pad) {
		nfsm_chain_dissect_init(error, &nmc, *mb_head);
		                                                        
		nfsm_chain_adv(error, &nmc, NFSX_UNSIGNED + length);
		nfsm_chain_finish_mbuf(error, &nmc);
		nfsm_chain_add_opaque_nopad(error, &nmc, xdrpad, pad);
		nfsm_chain_build_done(error, &nmc);
	}

	return error;
}

#if CONFIG_NFS_CLIENT

  
                                                                       
                                                                   
                                                                         
                                              
  
                                                                            
                                                                       
  
   
static errno_t
rpc_gss_integ_data_restore(gss_ctx_id_t ctx __unused, mbuf_t *mb_head, size_t len)
{
	mbuf_t mb = *mb_head;
	mbuf_t tail = NULL, next;

	                                              
	mbuf_adj(mb, 2 * NFSX_UNSIGNED);

	                                 
	for (; mb; mb = next) {
		next = mbuf_next(mb);
		if (mbuf_len(mb) == 0) {
			mbuf_free(mb);
		} else {
			break;
		}
	}
	*mb_head = mb;

	for (; mb && len; mb = mbuf_next(mb)) {
		tail = mb;
		if (mbuf_len(mb) <= len) {
			len -= mbuf_len(mb);
		} else {
			return EBADRPC;
		}
	}
	                  
	if (tail) {
		mbuf_setnext(tail, NULL);
		mbuf_freem(mb);
	}

	return 0;
}

  
                                                                      
                                                  
  
                                                                    
                                                  
   
static errno_t
rpc_gss_priv_data_restore(gss_ctx_id_t ctx, mbuf_t *mb_head, size_t len)
{
	uint32_t major, error;
	mbuf_t mb = *mb_head, next;
	size_t plen, length;
	gss_qop_t qop = GSS_C_QOP_REVERSE;

	                               
	mbuf_adj(mb, NFSX_UNSIGNED);
	                                 
	plen = nfsm_pad(len);
	if (plen) {
		mbuf_t tail = NULL;

		for (length = 0; length < len && mb; mb = mbuf_next(mb)) {
			tail = mb;
			length += mbuf_len(mb);
		}
		if ((length != len) || (mb == NULL) || (tail == NULL)) {
			return EBADRPC;
		}

		mbuf_freem(mb);
		mbuf_setnext(tail, NULL);
	}

	major = gss_krb5_unwrap_mbuf(&error, ctx, mb_head, 0, len, NULL, &qop);
	if (major != GSS_S_COMPLETE) {
		printf("gss_krb5_unwrap_mbuf failed. major = %d minor = %d\n", (int)major, error);
		return error;
	}
	mb = *mb_head;

	                             
	mbuf_adj(mb, NFSX_UNSIGNED);
	assert(mbuf_len(mb) == 0);

	                             
	for (mb = *mb_head; mb; mb = next) {
		next = mbuf_next(mb);
		if (mbuf_len(mb) == 0) {
			mbuf_free(mb);
		} else {
			break;
		}
	}
	*mb_head = mb;

	return 0;
}

  
                                          
  
                                       
                                           
  
                                                 
                                        
  
                                                    
                                                            
   

#define kauth_cred_getasid(cred) ((cred)->cr_audit.as_aia_p->ai_asid)
#define kauth_cred_getauid(cred) ((cred)->cr_audit.as_aia_p->ai_auid)

#define SAFE_CAST_INTTYPE( type, intval ) \
	( (type)(intval)/(sizeof(type) < sizeof(intval) ? 0 : 1) )

uid_t
nfs_cred_getasid2uid(kauth_cred_t cred)
{
	uid_t result = SAFE_CAST_INTTYPE(uid_t, kauth_cred_getasid(cred));
	return result;
}

  
            
   
static void
nfs_gss_clnt_ctx_dump(struct nfsmount *nmp)
{
	struct nfs_gss_clnt_ctx *cp;

	lck_mtx_lock(&nmp->nm_lock);
	NFS_GSS_DBG("Enter\n");
	TAILQ_FOREACH(cp, &nmp->nm_gsscl, gss_clnt_entries) {
		lck_mtx_lock(&cp->gss_clnt_mtx);
		printf("context %d/%d: refcnt = %d, flags = %x\n",
		    kauth_cred_getasid(cp->gss_clnt_cred),
		    kauth_cred_getauid(cp->gss_clnt_cred),
		    cp->gss_clnt_refcnt, cp->gss_clnt_flags);
		lck_mtx_unlock(&cp->gss_clnt_mtx);
	}
	NFS_GSS_DBG("Exit\n");
	lck_mtx_unlock(&nmp->nm_lock);
}

static char *
nfs_gss_clnt_ctx_name(struct nfsmount *nmp, struct nfs_gss_clnt_ctx *cp, char *buf, int len)
{
	char *np;
	size_t nlen;
	const char *server = "";

	if (nmp && nmp->nm_mountp) {
		server = vfs_statfs(nmp->nm_mountp)->f_mntfromname;
	}

	if (cp == NULL) {
		snprintf(buf, len, "[%s] NULL context", server);
		return buf;
	}

	if (cp->gss_clnt_principal && !cp->gss_clnt_display) {
		np = (char *)cp->gss_clnt_principal;
		nlen = cp->gss_clnt_prinlen;
	} else {
		np = cp->gss_clnt_display;
		nlen = np ? strlen(cp->gss_clnt_display) : 0;
	}
	if (nlen) {
		snprintf(buf, len, "[%s] %.*s %d/%d %s", server, nlen > INT_MAX ? INT_MAX : (int)nlen, np,
		    kauth_cred_getasid(cp->gss_clnt_cred),
		    kauth_cred_getuid(cp->gss_clnt_cred),
		    cp->gss_clnt_principal ? "" : "[from default cred] ");
	} else {
		snprintf(buf, len, "[%s] using default %d/%d ", server,
		    kauth_cred_getasid(cp->gss_clnt_cred),
		    kauth_cred_getuid(cp->gss_clnt_cred));
	}
	return buf;
}

#define NFS_CTXBUFSZ 80
#define NFS_GSS_CTX(req, cp) nfs_gss_clnt_ctx_name((req)->r_nmp, cp ? cp : (req)->r_gss_ctx, CTXBUF, sizeof(CTXBUF))

#define NFS_GSS_CLNT_CTX_DUMP(nmp)              \
	do {                  \
	        if (NFS_GSS_ISDBG && (NFS_DEBUG_FLAGS & 0x2))   \
	                nfs_gss_clnt_ctx_dump((nmp));   \
	} while (0)

static int
nfs_gss_clnt_ctx_cred_match(kauth_cred_t cred1, kauth_cred_t cred2)
{
	if (kauth_cred_getasid(cred1) == kauth_cred_getasid(cred2)) {
		return 1;
	}
	return 0;
}

  
                                                     
                                                    
                                                     
                                                        
                                                            
  
                                                           
                                                                
                                                     
   

static int
nfs_gss_clnt_mnt_ref(struct nfsmount *nmp)
{
	int error;
	vnode_t rvp;

	if (nmp == NULL ||
	    !(vfs_flags(nmp->nm_mountp) & MNT_AUTOMOUNTED)) {
		return EINVAL;
	}

	error = VFS_ROOT(nmp->nm_mountp, &rvp, NULL);
	if (!error) {
		error = vnode_ref(rvp);
		vnode_put(rvp);
	}

	return error;
}

  
                                       
   

static int
nfs_gss_clnt_mnt_rele(struct nfsmount *nmp)
{
	int error;
	vnode_t rvp;

	if (nmp == NULL ||
	    !(vfs_flags(nmp->nm_mountp) & MNT_AUTOMOUNTED)) {
		return EINVAL;
	}

	error = VFS_ROOT(nmp->nm_mountp, &rvp, NULL);
	if (!error) {
		vnode_rele(rvp);
		vnode_put(rvp);
	}

	return error;
}

int nfs_root_steals_ctx = 0;

static int
nfs_gss_clnt_ctx_find_principal(struct nfsreq *req, uint8_t *principal, size_t plen, uint32_t nt)
{
	struct nfsmount *nmp = req->r_nmp;
	struct nfs_gss_clnt_ctx *cp, *tcp;
	struct nfsreq *treq;
	int error = 0;
	struct timeval now;
	char CTXBUF[NFS_CTXBUFSZ];

	treq = zalloc_flags(nfs_req_zone, Z_WAITOK | Z_ZERO);
	treq->r_nmp = nmp;

	microuptime(&now);
	lck_mtx_lock(&nmp->nm_lock);
	TAILQ_FOREACH_SAFE(cp, &nmp->nm_gsscl, gss_clnt_entries, tcp) {
		lck_mtx_lock(&cp->gss_clnt_mtx);
		if (cp->gss_clnt_flags & GSS_CTX_DESTROY) {
			NFS_GSS_DBG("Found destroyed context %s refcnt = %d continuing\n",
			    NFS_GSS_CTX(req, cp),
			    cp->gss_clnt_refcnt);
			lck_mtx_unlock(&cp->gss_clnt_mtx);
			continue;
		}
		if (nfs_gss_clnt_ctx_cred_match(cp->gss_clnt_cred, req->r_cred)) {
			if (nmp->nm_gsscl.tqh_first != cp) {
				TAILQ_REMOVE(&nmp->nm_gsscl, cp, gss_clnt_entries);
				TAILQ_INSERT_HEAD(&nmp->nm_gsscl, cp, gss_clnt_entries);
			}
			if (principal) {
				  
                                                                     
                          
       
				if (cp->gss_clnt_prinlen != plen || cp->gss_clnt_prinnt != nt ||
				    bcmp(cp->gss_clnt_principal, principal, plen) != 0) {
					cp->gss_clnt_flags |= (GSS_CTX_INVAL | GSS_CTX_DESTROY);
					cp->gss_clnt_refcnt++;
					lck_mtx_unlock(&cp->gss_clnt_mtx);
					NFS_GSS_DBG("Marking %s for deletion because %s does not match\n",
					    NFS_GSS_CTX(req, cp), principal);
					NFS_GSS_DBG("len = (%zu,%zu), nt = (%d,%d)\n", cp->gss_clnt_prinlen, plen,
					    cp->gss_clnt_prinnt, nt);
					treq->r_gss_ctx  = cp;
					cp = NULL;
					break;
				}
			}
			if (cp->gss_clnt_flags & GSS_CTX_INVAL) {
				  
                                                      
                                                       
                                                            
       
				if (cp->gss_clnt_nctime + GSS_NEG_CACHE_TO >= now.tv_sec || cp->gss_clnt_nctime == 0) {
					NFS_GSS_DBG("Context %s (refcnt = %d) not expired returning EAUTH nctime = %ld now = %ld\n",
					    NFS_GSS_CTX(req, cp), cp->gss_clnt_refcnt, cp->gss_clnt_nctime, now.tv_sec);
					lck_mtx_unlock(&cp->gss_clnt_mtx);
					lck_mtx_unlock(&nmp->nm_lock);
					NFS_ZFREE(nfs_req_zone, treq);
					return NFSERR_EAUTH;
				}
				if (cp->gss_clnt_refcnt) {
					struct nfs_gss_clnt_ctx *ncp;
					  
                                                                         
                                                                     
                                    
        
					cp->gss_clnt_flags |= GSS_CTX_DESTROY;
					NFS_GSS_DBG("Context %s has expired but we still have %d references\n",
					    NFS_GSS_CTX(req, cp), cp->gss_clnt_refcnt);
					error = nfs_gss_clnt_ctx_copy(cp, &ncp);
					lck_mtx_unlock(&cp->gss_clnt_mtx);
					if (error) {
						lck_mtx_unlock(&nmp->nm_lock);
						NFS_ZFREE(nfs_req_zone, treq);
						return error;
					}
					cp = ncp;
					break;
				} else {
					if (cp->gss_clnt_nctime) {
						nmp->nm_ncentries--;
					}
					lck_mtx_unlock(&cp->gss_clnt_mtx);
					TAILQ_REMOVE(&nmp->nm_gsscl, cp, gss_clnt_entries);
					break;
				}
			}
			                                     
			cp->gss_clnt_refcnt++;
			req->r_gss_ctx = cp;
			lck_mtx_unlock(&cp->gss_clnt_mtx);
			lck_mtx_unlock(&nmp->nm_lock);
			NFS_ZFREE(nfs_req_zone, treq);
			return 0;
		}
		lck_mtx_unlock(&cp->gss_clnt_mtx);
	}

	if (!cp && nfs_root_steals_ctx && principal == NULL && kauth_cred_getuid(req->r_cred) == 0) {
		  
                                                      
                                         
                                                  
                                                   
                                  
     
		TAILQ_FOREACH(cp, &nmp->nm_gsscl, gss_clnt_entries) {
			if (!(cp->gss_clnt_flags & (GSS_CTX_INVAL | GSS_CTX_DESTROY))) {
				nfs_gss_clnt_ctx_ref(req, cp);
				lck_mtx_unlock(&nmp->nm_lock);
				NFS_GSS_DBG("Root stole context %s\n", NFS_GSS_CTX(req, NULL));
				NFS_ZFREE(nfs_req_zone, treq);
				return 0;
			}
		}
	}

	NFS_GSS_DBG("Context %s%sfound in Neg Cache @  %ld\n",
	    NFS_GSS_CTX(req, cp),
	    cp == NULL ? " not " : "",
	    cp == NULL ? 0L : cp->gss_clnt_nctime);

	  
                                    
    

	if (cp == NULL) {
		MALLOC(cp, struct nfs_gss_clnt_ctx *, sizeof(*cp), M_TEMP, M_WAITOK | M_ZERO);
		if (cp == NULL) {
			lck_mtx_unlock(&nmp->nm_lock);
			NFS_ZFREE(nfs_req_zone, treq);
			return ENOMEM;
		}
		cp->gss_clnt_cred = req->r_cred;
		kauth_cred_ref(cp->gss_clnt_cred);
		lck_mtx_init(&cp->gss_clnt_mtx, &nfs_gss_clnt_grp, LCK_ATTR_NULL);
		cp->gss_clnt_ptime = now.tv_sec - GSS_PRINT_DELAY;
		if (principal) {
			MALLOC(cp->gss_clnt_principal, uint8_t *, plen + 1, M_TEMP, M_WAITOK | M_ZERO);
			memcpy(cp->gss_clnt_principal, principal, plen);
			cp->gss_clnt_prinlen = plen;
			cp->gss_clnt_prinnt = nt;
			cp->gss_clnt_flags |= GSS_CTX_STICKY;
			if (!nfs_gss_clnt_mnt_ref(nmp)) {
				cp->gss_clnt_flags |= GSS_CTX_USECOUNT;
			}
		}
	} else {
		uint32_t oldflags = cp->gss_clnt_flags;
		nfs_gss_clnt_ctx_clean(cp);
		if (principal) {
			  
                                                          
                                                            
                                                       
                                                          
                                                           
                                                          
                                                                 
                                                        
                                                             
                                          
      
			cp->gss_clnt_flags |= GSS_CTX_STICKY;

			  
                                                                          
                                                                          
      
			if ((oldflags & GSS_CTX_USECOUNT) || !nfs_gss_clnt_mnt_ref(nmp)) {
				cp->gss_clnt_flags |= GSS_CTX_USECOUNT;
			}
		}
	}

	cp->gss_clnt_thread = current_thread();
	nfs_gss_clnt_ctx_ref(req, cp);
	TAILQ_INSERT_HEAD(&nmp->nm_gsscl, cp, gss_clnt_entries);
	lck_mtx_unlock(&nmp->nm_lock);

	error = nfs_gss_clnt_ctx_init_retry(req, cp);                          
	if (error) {
		NFS_GSS_DBG("nfs_gss_clnt_ctx_init_retry returned %d for %s\n", error, NFS_GSS_CTX(req, cp));
		nfs_gss_clnt_ctx_unref(req);
	}

	                                                                   
	nfs_gss_clnt_ctx_unref(treq);
	NFS_ZFREE(nfs_req_zone, treq);
	return error;
}

static int
nfs_gss_clnt_ctx_find(struct nfsreq *req)
{
	return nfs_gss_clnt_ctx_find_principal(req, NULL, 0, 0);
}

  
                                                       
                                                       
                                                         
                     
   

int
nfs_gss_clnt_cred_put(struct nfsreq *req, struct nfsm_chain *nmc, mbuf_t args)
{
	struct nfs_gss_clnt_ctx *cp;
	uint32_t seqnum = 0;
	uint32_t major;
	uint32_t error = 0;
	int slpflag, recordmark = 0, offset;
	struct gss_seq *gsp;
	gss_buffer_desc mic;

	slpflag = (PZERO - 1);
	if (req->r_nmp) {
		slpflag |= (NMFLAG(req->r_nmp, INTR) && req->r_thread && !(req->r_flags & R_NOINTR)) ? PCATCH : 0;
		recordmark = (req->r_nmp->nm_sotype == SOCK_STREAM);
	}

retry:
	if (req->r_gss_ctx == NULL) {
		  
                                    
                                     
                
     
		error = nfs_gss_clnt_ctx_find(req);
		if (error) {
			return error;
		}
	}
	cp = req->r_gss_ctx;

	  
                                                            
                                                           
                                                          
            
    
	lck_mtx_lock(&cp->gss_clnt_mtx);
	if (cp->gss_clnt_thread && cp->gss_clnt_thread != current_thread()) {
		cp->gss_clnt_flags |= GSS_NEEDCTX;
		msleep(cp, &cp->gss_clnt_mtx, slpflag | PDROP, "ctxwait", NULL);
		slpflag &= ~PCATCH;
		if ((error = nfs_sigintr(req->r_nmp, req, req->r_thread, 0))) {
			return error;
		}
		nfs_gss_clnt_ctx_unref(req);
		goto retry;
	}
	lck_mtx_unlock(&cp->gss_clnt_mtx);

	if (cp->gss_clnt_flags & GSS_CTX_COMPLETE) {
		  
                                            
                                                                
                                                            
                                                             
                
     
		lck_mtx_lock(&cp->gss_clnt_mtx);
		while (win_getbit(cp->gss_clnt_seqbits,
		    ((cp->gss_clnt_seqnum - cp->gss_clnt_seqwin) + 1) % cp->gss_clnt_seqwin)) {
			cp->gss_clnt_flags |= GSS_NEEDSEQ;
			msleep(cp, &cp->gss_clnt_mtx, slpflag | PDROP, "seqwin", NULL);
			slpflag &= ~PCATCH;
			if ((error = nfs_sigintr(req->r_nmp, req, req->r_thread, 0))) {
				return error;
			}
			lck_mtx_lock(&cp->gss_clnt_mtx);
			if (cp->gss_clnt_flags & GSS_CTX_INVAL) {
				                                         
				lck_mtx_unlock(&cp->gss_clnt_mtx);
				nfs_gss_clnt_ctx_unref(req);
				goto retry;
			}
		}
		seqnum = ++cp->gss_clnt_seqnum;
		win_setbit(cp->gss_clnt_seqbits, seqnum % cp->gss_clnt_seqwin);
		lck_mtx_unlock(&cp->gss_clnt_mtx);

		MALLOC(gsp, struct gss_seq *, sizeof(*gsp), M_TEMP, M_WAITOK | M_ZERO);
		if (gsp == NULL) {
			return ENOMEM;
		}
		gsp->gss_seqnum = seqnum;
		SLIST_INSERT_HEAD(&req->r_gss_seqlist, gsp, gss_seqnext);
	}

	                           
	nfsm_chain_add_32(error, nmc, RPCSEC_GSS);
	nfsm_chain_add_32(error, nmc, 5 * NFSX_UNSIGNED + cp->gss_clnt_handle_len);
	nfsm_chain_add_32(error, nmc, RPCSEC_GSS_VERS_1);
	nfsm_chain_add_32(error, nmc, cp->gss_clnt_proc);
	nfsm_chain_add_32(error, nmc, seqnum);
	nfsm_chain_add_32(error, nmc, cp->gss_clnt_service);
	nfsm_chain_add_32(error, nmc, cp->gss_clnt_handle_len);
	if (cp->gss_clnt_handle_len > 0) {
		if (cp->gss_clnt_handle == NULL) {
			return EBADRPC;
		}
		nfsm_chain_add_opaque(error, nmc, cp->gss_clnt_handle, cp->gss_clnt_handle_len);
	}
	if (error) {
		return error;
	}
	  
                        
    
	if (cp->gss_clnt_proc == RPCSEC_GSS_INIT ||
	    cp->gss_clnt_proc == RPCSEC_GSS_CONTINUE_INIT) {
		  
                                          
                              
     
		nfsm_chain_add_32(error, nmc, RPCAUTH_NULL);             
		nfsm_chain_add_32(error, nmc, 0);                        
		nfsm_chain_build_done(error, nmc);
		if (!error) {
			nfs_gss_append_chain(nmc, args);
		}
		return error;
	}

	offset = recordmark ? NFSX_UNSIGNED : 0;               
	nfsm_chain_build_done(error, nmc);

	major = gss_krb5_get_mic_mbuf((uint32_t *)&error, cp->gss_clnt_ctx_id, 0, nmc->nmc_mhead, offset, 0, &mic);
	if (major != GSS_S_COMPLETE) {
		printf("gss_krb5_get_mic_buf failed %d\n", error);
		return error;
	}

	nfsm_chain_add_32(error, nmc, RPCSEC_GSS);               
	nfsm_chain_add_32(error, nmc, mic.length);                       
	nfsm_chain_add_opaque(error, nmc, mic.value, mic.length);
	(void)gss_release_buffer(NULL, &mic);
	nfsm_chain_build_done(error, nmc);
	if (error) {
		return error;
	}

	  
                                                                 
                              
    
	switch (cp->gss_clnt_service) {
	case RPCSEC_GSS_SVC_NONE:
		if (args) {
			nfs_gss_append_chain(nmc, args);
		}
		break;
	case RPCSEC_GSS_SVC_INTEGRITY:
		  
                                                                    
                                                             
     
		                                                                                                 
		assert(req->r_mrest == args);
		nfsm_chain_finish_mbuf(error, nmc);
		if (error) {
			return error;
		}
		error = rpc_gss_integ_data_create(cp->gss_clnt_ctx_id, &args, seqnum, &req->r_gss_arglen);
		if (error) {
			break;
		}
		req->r_mrest = args;
		req->r_gss_argoff = nfsm_chain_offset(nmc);
		nfs_gss_append_chain(nmc, args);
		break;
	case RPCSEC_GSS_SVC_PRIVACY:
		  
                                                                          
                                                        
     
		                                                                                                
		assert(req->r_mrest == args);
		nfsm_chain_finish_mbuf(error, nmc);
		if (error) {
			return error;
		}
		error = rpc_gss_priv_data_create(cp->gss_clnt_ctx_id, &args, seqnum, &req->r_gss_arglen);
		if (error) {
			break;
		}
		req->r_mrest = args;
		req->r_gss_argoff = nfsm_chain_offset(nmc);
		nfs_gss_append_chain(nmc, args);
		break;
	default:
		return EINVAL;
	}

	return error;
}

  
                                                         
                                                         
                                                          
                                                         
                                                          
                 
   
int
nfs_gss_clnt_verf_get(
	struct nfsreq *req,
	struct nfsm_chain *nmc,
	uint32_t verftype,
	uint32_t verflen,
	uint32_t *accepted_statusp)
{
	gss_buffer_desc cksum;
	uint32_t seqnum = 0;
	uint32_t major;
	struct nfs_gss_clnt_ctx *cp = req->r_gss_ctx;
	struct nfsm_chain nmc_tmp;
	struct gss_seq *gsp;
	uint32_t reslen;
	int error = 0;
	mbuf_t results_mbuf, prev_mbuf, pad_mbuf;
	size_t ressize, offset;

	reslen = 0;
	*accepted_statusp = 0;

	if (cp == NULL) {
		return NFSERR_EAUTH;
	}
	  
                                                      
                                                
                                                   
                                                    
                                                     
    
	if (verftype != RPCSEC_GSS) {
		if (verftype != RPCAUTH_NULL) {
			return NFSERR_EAUTH;
		}
		if (cp->gss_clnt_flags & GSS_CTX_COMPLETE) {
			return NFSERR_EAUTH;
		}
		if (verflen > 0) {
			nfsm_chain_adv(error, nmc, nfsm_rndup(verflen));
		}
		nfsm_chain_get_32(error, nmc, *accepted_statusp);
		return error;
	}

	  
                                                 
                                               
                                                 
                                                   
                                                   
                                                  
                                                      
                            
    
	if (!(cp->gss_clnt_flags & GSS_CTX_COMPLETE)) {
		if (verflen > KRB5_MAX_MIC_SIZE) {
			return EBADRPC;
		}
		MALLOC(cp->gss_clnt_verf, u_char *, verflen, M_TEMP, M_WAITOK | M_ZERO);
		if (cp->gss_clnt_verf == NULL) {
			return ENOMEM;
		}
		cp->gss_clnt_verflen = verflen;
		nfsm_chain_get_opaque(error, nmc, verflen, cp->gss_clnt_verf);
		nfsm_chain_get_32(error, nmc, *accepted_statusp);
		return error;
	}

	if (verflen > KRB5_MAX_MIC_SIZE) {
		return EBADRPC;
	}
	cksum.length = verflen;
	MALLOC(cksum.value, void *, verflen, M_TEMP, M_WAITOK);

	  
                   
    
	nfsm_chain_get_opaque(error, nmc, verflen, cksum.value);
	if (error) {
		FREE(cksum.value, M_TEMP);
		goto nfsmout;
	}

	  
                                                                
                                                             
                                                   
    
	SLIST_FOREACH(gsp, &req->r_gss_seqlist, gss_seqnext) {
		gss_buffer_desc seqnum_buf;
		uint32_t network_seqnum = htonl(gsp->gss_seqnum);

		seqnum_buf.length = sizeof(network_seqnum);
		seqnum_buf.value = &network_seqnum;
		major = gss_krb5_verify_mic(NULL, cp->gss_clnt_ctx_id, &seqnum_buf, &cksum, NULL);
		if (major == GSS_S_COMPLETE) {
			break;
		}
	}
	FREE(cksum.value, M_TEMP);
	if (gsp == NULL) {
		return NFSERR_EAUTH;
	}

	  
                               
    
	nfsm_chain_get_32(error, nmc, *accepted_statusp);
	if (*accepted_statusp != RPC_SUCCESS) {
		return 0;
	}

	  
                                                             
                              
    
	switch (cp->gss_clnt_service) {
	case RPCSEC_GSS_SVC_NONE:
		                   
		break;
	case RPCSEC_GSS_SVC_INTEGRITY:
		  
                                                                  
    
                                            
                                
                               
                               
                                   
     

		nfsm_chain_get_32(error, nmc, reslen);                              
		if (reslen > NFS_MAXPACKET) {
			error = EBADRPC;
			goto nfsmout;
		}

		                               
		nmc_tmp = *nmc;
		nfsm_chain_adv(error, &nmc_tmp, reslen);                                
		nfsm_chain_get_32(error, &nmc_tmp, cksum.length);
		if (cksum.length > KRB5_MAX_MIC_SIZE) {
			error = EBADRPC;
			goto nfsmout;
		}
		MALLOC(cksum.value, void *, cksum.length, M_TEMP, M_WAITOK);
		nfsm_chain_get_opaque(error, &nmc_tmp, cksum.length, cksum.value);
		                          

		                     
		offset = nfsm_chain_offset(nmc);
		major = gss_krb5_verify_mic_mbuf((uint32_t *)&error, cp->gss_clnt_ctx_id, nmc->nmc_mhead, offset, reslen, &cksum, NULL);
		FREE(cksum.value, M_TEMP);
		if (major != GSS_S_COMPLETE) {
			printf("client results: gss_krb5_verify_mic_mbuf failed %d\n", error);
			error = EBADRPC;
			goto nfsmout;
		}

		  
                                                     
                                       
     
		nfsm_chain_get_32(error, nmc, seqnum);
		if (gsp->gss_seqnum != seqnum) {
			error = EBADRPC;
			goto nfsmout;
		}
#if 0
		SLIST_FOREACH(gsp, &req->r_gss_seqlist, gss_seqnext) {
			if (seqnum == gsp->gss_seqnum) {
				break;
			}
		}
		if (gsp == NULL) {
			error = EBADRPC;
			goto nfsmout;
		}
#endif
		break;
	case RPCSEC_GSS_SVC_PRIVACY:
		  
                                                  
    
                                       
                           
                 
     
		prev_mbuf = nmc->nmc_mcur;
		nfsm_chain_get_32(error, nmc, reslen);                              
		if (reslen == 0 || reslen > NFS_MAXPACKET) {
			error = EBADRPC;
			goto nfsmout;
		}

		                                                                                   
		offset = nmc->nmc_ptr - (caddr_t)mbuf_data(nmc->nmc_mcur);

		                              
		ressize = reslen;
		error = gss_normalize_mbuf(nmc->nmc_mcur, offset, &ressize, &results_mbuf, &pad_mbuf, 0);
		if (error) {
			goto nfsmout;
		}

		if (pad_mbuf) {
			assert(nfsm_pad(reslen) == mbuf_len(pad_mbuf));
			mbuf_free(pad_mbuf);
		}

		major = gss_krb5_unwrap_mbuf((uint32_t *)&error, cp->gss_clnt_ctx_id, &results_mbuf, 0, ressize, NULL, NULL);
		if (major) {
			printf("%s unwraped failed %d\n", __func__, error);
			goto nfsmout;
		}

		                                                               
		mbuf_setnext(prev_mbuf, results_mbuf);
		nmc->nmc_mcur = results_mbuf;
		nmc->nmc_ptr = mbuf_data(results_mbuf);
		nmc->nmc_left = mbuf_len(results_mbuf);

		  
                                                     
                                      
     
		nfsm_chain_get_32(error, nmc, seqnum);
		if (gsp->gss_seqnum != seqnum) {
			printf("%s bad seqnum\n", __func__);
			error = EBADRPC;
			goto nfsmout;
		}
#if 0
		SLIST_FOREACH(gsp, &req->r_gss_seqlist, gss_seqnext) {
			if (seqnum == gsp->gss_seqnum) {
				break;
			}
		}
		if (gsp == NULL) {
			error = EBADRPC;
			goto nfsmout;
		}
#endif
		break;
	}
nfsmout:
	return error;
}

  
                                                              
                                                      
  
                                                              
                                                                    
                                                                      
              
  
                                                              
                                                           
                                                   
   
int
nfs_gss_clnt_args_restore(struct nfsreq *req)
{
	struct nfs_gss_clnt_ctx *cp = req->r_gss_ctx;
	struct nfsm_chain mchain, *nmc = &mchain;
	int error = 0, merr;

	if (cp == NULL) {
		return NFSERR_EAUTH;
	}

	if ((cp->gss_clnt_flags & GSS_CTX_COMPLETE) == 0) {
		return ENEEDAUTH;
	}

	                                     
	if (cp->gss_clnt_service == RPCSEC_GSS_SVC_NONE) {
		return 0;
	}

	nfsm_chain_dissect_init(error, nmc, req->r_mhead);                            
	nfsm_chain_adv(error, nmc, req->r_gss_argoff);                            
	if (error) {
		return error;
	}

	if (cp->gss_clnt_service == RPCSEC_GSS_SVC_INTEGRITY) {
		error = rpc_gss_integ_data_restore(cp->gss_clnt_ctx_id, &req->r_mrest, req->r_gss_arglen);
	} else {
		error = rpc_gss_priv_data_restore(cp->gss_clnt_ctx_id, &req->r_mrest, req->r_gss_arglen);
	}

	merr = mbuf_setnext(nmc->nmc_mcur, req->r_mrest);                             
	assert(merr == 0);

	return error ? error : merr;
}

  
                                                      
                                                                       
                                                                          
                                                                            
                                                                        
                                                                           
                                                                           
                             
   
static int
nfs_gss_clnt_ctx_init(struct nfsreq *req, struct nfs_gss_clnt_ctx *cp)
{
	struct nfsmount *nmp = req->r_nmp;
	gss_buffer_desc cksum, window;
	uint32_t network_seqnum;
	int client_complete = 0;
	int server_complete = 0;
	int error = 0;
	int retrycnt = 0;
	uint32_t major;

	                                     

	if (cp->gss_clnt_svcname == NULL) {
		cp->gss_clnt_svcname = nfs_gss_clnt_svcname(nmp, &cp->gss_clnt_svcnt, &cp->gss_clnt_svcnamlen);
		if (cp->gss_clnt_svcname == NULL) {
			error = NFSERR_EAUTH;
			goto nfsmout;
		}
	}

	cp->gss_clnt_proc = RPCSEC_GSS_INIT;

	cp->gss_clnt_service =
	    req->r_auth == RPCAUTH_KRB5  ? RPCSEC_GSS_SVC_NONE :
	    req->r_auth == RPCAUTH_KRB5I ? RPCSEC_GSS_SVC_INTEGRITY :
	    req->r_auth == RPCAUTH_KRB5P ? RPCSEC_GSS_SVC_PRIVACY : 0;

	  
                                                        
                                                            
                                                             
    
	for (;;) {
retry:
		                                                    
		error = nfs_gss_clnt_gssd_upcall(req, cp, retrycnt);
		if (error) {
			goto nfsmout;
		}

		if (cp->gss_clnt_major == GSS_S_COMPLETE) {
			client_complete = 1;
			NFS_GSS_DBG("Client complete\n");
			if (server_complete) {
				break;
			}
		} else if (cp->gss_clnt_major != GSS_S_CONTINUE_NEEDED) {
			  
                                                            
                                                           
                                                           
                                                            
                                                       
                     
      
			retrycnt++;
			cp->gss_clnt_gssd_flags |= GSSD_RESTART;
			NFS_GSS_DBG("Retrying major = %x minor = %d\n", cp->gss_clnt_major, (int)cp->gss_clnt_minor);
			goto retry;
		}

		  
                                  
     
		error = nfs_gss_clnt_ctx_callserver(req, cp);
		if (error) {
			if (error == ENEEDAUTH &&
			    (cp->gss_clnt_proc == RPCSEC_GSS_INIT ||
			    cp->gss_clnt_proc == RPCSEC_GSS_CONTINUE_INIT)) {
				  
                                                   
                                                        
                                                          
                                                       
                                                
                                                          
                   
       
				retrycnt++;
				cp->gss_clnt_gssd_flags |= GSSD_RESTART;
				NFS_GSS_DBG("Retrying major = %x minor = %d\n", cp->gss_clnt_major, (int)cp->gss_clnt_minor);
				goto retry;
			}
			goto nfsmout;
		}
		if (cp->gss_clnt_major == GSS_S_COMPLETE) {
			NFS_GSS_DBG("Server complete\n");
			server_complete = 1;
			if (client_complete) {
				break;
			}
		} else if (cp->gss_clnt_major == GSS_S_CONTINUE_NEEDED) {
			cp->gss_clnt_proc = RPCSEC_GSS_CONTINUE_INIT;
		} else {
			                                               
			retrycnt++;
			cp->gss_clnt_gssd_flags |= GSSD_RESTART;
			NFS_GSS_DBG("Retrying major = %x minor = %d\n", cp->gss_clnt_major, (int)cp->gss_clnt_minor);
		}
	}

	  
                                                      
    
	lck_mtx_lock(&cp->gss_clnt_mtx);
	cp->gss_clnt_flags |= GSS_CTX_COMPLETE;
	lck_mtx_unlock(&cp->gss_clnt_mtx);
	cp->gss_clnt_proc = RPCSEC_GSS_DATA;

	network_seqnum = htonl(cp->gss_clnt_seqwin);
	window.length = sizeof(cp->gss_clnt_seqwin);
	window.value = &network_seqnum;
	cksum.value = cp->gss_clnt_verf;
	cksum.length = cp->gss_clnt_verflen;
	major = gss_krb5_verify_mic((uint32_t *)&error, cp->gss_clnt_ctx_id, &window, &cksum, NULL);
	cp->gss_clnt_verflen = 0;
	FREE(cp->gss_clnt_verf, M_TEMP);
	cp->gss_clnt_verf = NULL;
	if (major != GSS_S_COMPLETE) {
		printf("%s: could not verify window\n", __func__);
		error = NFSERR_EAUTH;
		goto nfsmout;
	}

	  
                                                       
                                                            
                                                             
                        
    
	cp->gss_clnt_seqnum = (random() & 0xffff) + cp->gss_clnt_seqwin;

	  
                                                     
                                                  
    
	MALLOC(cp->gss_clnt_seqbits, uint32_t *,
	    nfsm_rndup((cp->gss_clnt_seqwin + 7) / 8), M_TEMP, M_WAITOK | M_ZERO);
	if (cp->gss_clnt_seqbits == NULL) {
		error = NFSERR_EAUTH;
	}

nfsmout:
	  
                                                        
                                                             
                               
    
	if (error == ENEEDAUTH) {
		NFS_GSS_DBG("Returning ENEEDAUTH\n");
		return error;
	}

	  
                                                 
                                               
                  
    
	lck_mtx_lock(&cp->gss_clnt_mtx);
	if (error) {
		cp->gss_clnt_flags |= GSS_CTX_INVAL;
	}

	  
                                               
    
	cp->gss_clnt_thread = NULL;
	if (cp->gss_clnt_flags & GSS_NEEDCTX) {
		cp->gss_clnt_flags &= ~GSS_NEEDCTX;
		wakeup(cp);
	}
	lck_mtx_unlock(&cp->gss_clnt_mtx);

	NFS_GSS_DBG("Returning error = %d\n", error);
	return error;
}

  
                                                                       
                                                                       
                                                                    
                                                                      
                                                                     
                          
   
static int
nfs_gss_clnt_ctx_init_retry(struct nfsreq *req, struct nfs_gss_clnt_ctx *cp)
{
	struct nfsmount *nmp = req->r_nmp;
	struct timeval now;
	time_t waituntil;
	int error, slpflag;
	int retries = 0;
	int timeo = NFS_TRYLATERDEL;

	if (nfs_mount_gone(nmp)) {
		error = ENXIO;
		goto bad;
	}

	                                                                 
	slpflag = (NMFLAG(nmp, INTR) && !(req->r_flags & R_NOINTR)) ? PCATCH : 0;

	while ((error = nfs_gss_clnt_ctx_init(req, cp)) == ENEEDAUTH) {
		microuptime(&now);
		waituntil = now.tv_sec + timeo;
		while (now.tv_sec < waituntil) {
			tsleep(NULL, PSOCK | slpflag, "nfs_gss_clnt_ctx_init_retry", hz);
			slpflag = 0;
			error = nfs_sigintr(req->r_nmp, req, current_thread(), 0);
			if (error) {
				goto bad;
			}
			microuptime(&now);
		}

		retries++;
		                                                     
		if ((NMFLAG(nmp, SOFT) || (req->r_flags & R_SOFT)) && (retries > nmp->nm_retry)) {
			error = ETIMEDOUT;
			goto bad;
		}
		timeo *= 2;
		if (timeo > 60) {
			timeo = 60;
		}
	}

	if (error == 0) {
		return 0;                 
	}
bad:
	  
                           
    
	lck_mtx_lock(&cp->gss_clnt_mtx);
	cp->gss_clnt_flags |= GSS_CTX_INVAL;

	  
                                               
    
	cp->gss_clnt_thread = NULL;
	if (cp->gss_clnt_flags & GSS_NEEDCTX) {
		cp->gss_clnt_flags &= ~GSS_NEEDCTX;
		wakeup(cp);
	}
	lck_mtx_unlock(&cp->gss_clnt_mtx);

	return error;
}

  
                                                                
                                                                   
                                                                    
                           
   
static int
nfs_gss_clnt_ctx_callserver(struct nfsreq *req, struct nfs_gss_clnt_ctx *cp)
{
	struct nfsm_chain nmreq, nmrep;
	int error = 0, status;
	uint32_t major = cp->gss_clnt_major, minor = cp->gss_clnt_minor;
	int sz;

	if (nfs_mount_gone(req->r_nmp)) {
		return ENXIO;
	}
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);
	sz = NFSX_UNSIGNED + nfsm_rndup(cp->gss_clnt_tokenlen);
	nfsm_chain_build_alloc_init(error, &nmreq, sz);
	nfsm_chain_add_32(error, &nmreq, cp->gss_clnt_tokenlen);
	if (cp->gss_clnt_tokenlen > 0) {
		nfsm_chain_add_opaque(error, &nmreq, cp->gss_clnt_token, cp->gss_clnt_tokenlen);
	}
	nfsm_chain_build_done(error, &nmreq);
	if (error) {
		goto nfsmout;
	}

	                     
	error = nfs_request_gss(req->r_nmp->nm_mountp, &nmreq, req->r_thread, req->r_cred,
	    (req->r_flags & R_OPTMASK), cp, &nmrep, &status);
	if (cp->gss_clnt_token != NULL) {
		FREE(cp->gss_clnt_token, M_TEMP);
		cp->gss_clnt_token = NULL;
	}
	if (!error) {
		error = status;
	}
	if (error) {
		goto nfsmout;
	}

	                            

	nfsm_chain_get_32(error, &nmrep, cp->gss_clnt_handle_len);
	if (cp->gss_clnt_handle != NULL) {
		FREE(cp->gss_clnt_handle, M_TEMP);
		cp->gss_clnt_handle = NULL;
	}
	if (cp->gss_clnt_handle_len > 0 && cp->gss_clnt_handle_len < GSS_MAX_CTX_HANDLE_LEN) {
		MALLOC(cp->gss_clnt_handle, u_char *, cp->gss_clnt_handle_len, M_TEMP, M_WAITOK);
		if (cp->gss_clnt_handle == NULL) {
			error = ENOMEM;
			goto nfsmout;
		}
		nfsm_chain_get_opaque(error, &nmrep, cp->gss_clnt_handle_len, cp->gss_clnt_handle);
	} else {
		error = EBADRPC;
	}
	nfsm_chain_get_32(error, &nmrep, cp->gss_clnt_major);
	nfsm_chain_get_32(error, &nmrep, cp->gss_clnt_minor);
	nfsm_chain_get_32(error, &nmrep, cp->gss_clnt_seqwin);
	nfsm_chain_get_32(error, &nmrep, cp->gss_clnt_tokenlen);
	if (error) {
		goto nfsmout;
	}
	if (cp->gss_clnt_tokenlen > 0 && cp->gss_clnt_tokenlen < GSS_MAX_TOKEN_LEN) {
		MALLOC(cp->gss_clnt_token, u_char *, cp->gss_clnt_tokenlen, M_TEMP, M_WAITOK);
		if (cp->gss_clnt_token == NULL) {
			error = ENOMEM;
			goto nfsmout;
		}
		nfsm_chain_get_opaque(error, &nmrep, cp->gss_clnt_tokenlen, cp->gss_clnt_token);
	} else {
		error = EBADRPC;
	}

	  
                                                                
    
	if (cp->gss_clnt_major != GSS_S_COMPLETE &&
	    cp->gss_clnt_major != GSS_S_CONTINUE_NEEDED) {
		printf("nfs_gss_clnt_ctx_callserver: gss_clnt_major = %d\n", cp->gss_clnt_major);
		nfs_gss_clnt_log_error(req, cp, major, minor);
	}

nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	return error;
}

  
                                                                             
                                                                             
                                                                             
                                                                        
                                                                  
                                                                              
                                                                            
                                                     
  
   

static uint8_t *
nfs_gss_clnt_svcname(struct nfsmount *nmp, gssd_nametype *nt, size_t *len)
{
	char *svcname, *d, *server;
	int lindx, sindx;

	if (nfs_mount_gone(nmp)) {
		return NULL;
	}

	if (nmp->nm_sprinc) {
		*len = strlen(nmp->nm_sprinc) + 1;
		MALLOC(svcname, char *, *len, M_TEMP, M_WAITOK);
		*nt = GSSD_HOSTBASED;
		if (svcname == NULL) {
			return NULL;
		}
		strlcpy(svcname, nmp->nm_sprinc, *len);

		return (uint8_t *)svcname;
	}

	*nt = GSSD_HOSTBASED;
	if (nmp->nm_locations.nl_numlocs && !(NFS_GSS_ISDBG && (NFS_DEBUG_FLAGS & 0x1))) {
		lindx = nmp->nm_locations.nl_current.nli_loc;
		sindx = nmp->nm_locations.nl_current.nli_serv;
		server = nmp->nm_locations.nl_locations[lindx]->nl_servers[sindx]->ns_name;
		*len = (uint32_t)strlen(server);
	} else {
		                                                       
		server = vfs_statfs(nmp->nm_mountp)->f_mntfromname;
		NFS_GSS_DBG("nfs getting gss svcname from %s\n", server);
		d = strchr(server, ':');
		*len = (uint32_t)(d ? (d - server) : strlen(server));
	}

	*len +=  5;                       
	MALLOC(svcname, char *, *len, M_TEMP, M_WAITOK);
	strlcpy(svcname, "nfs", *len);
	strlcat(svcname, "@", *len);
	strlcat(svcname, server, *len);
	NFS_GSS_DBG("nfs svcname = %s\n", svcname);

	return (uint8_t *)svcname;
}

  
                                   
                                                                     
                                                                               
                                                
   

static mach_port_t
nfs_gss_clnt_get_upcall_port(kauth_cred_t credp)
{
	mach_port_t gssd_host_port, uc_port = IPC_PORT_NULL;
	kern_return_t kr;
	au_asid_t asid;
	uid_t uid;

	kr = host_get_gssd_port(host_priv_self(), &gssd_host_port);
	if (kr != KERN_SUCCESS) {
		printf("nfs_gss_get_upcall_port: can't get gssd port, status %x (%d)\n", kr, kr);
		return IPC_PORT_NULL;
	}
	if (!IPC_PORT_VALID(gssd_host_port)) {
		printf("nfs_gss_get_upcall_port: gssd port not valid\n");
		return IPC_PORT_NULL;
	}

	asid = kauth_cred_getasid(credp);
	uid = kauth_cred_getauid(credp);
	if (uid == AU_DEFAUDITID) {
		uid = kauth_cred_getuid(credp);
	}
	kr = mach_gss_lookup(gssd_host_port, uid, asid, &uc_port);
	if (kr != KERN_SUCCESS) {
		printf("nfs_gss_clnt_get_upcall_port: mach_gssd_lookup failed: status %x (%d)\n", kr, kr);
	}
	host_release_special_port(gssd_host_port);

	return uc_port;
}


static void
nfs_gss_clnt_log_error(struct nfsreq *req, struct nfs_gss_clnt_ctx *cp, uint32_t major, uint32_t minor)
{
#define GETMAJERROR(x) (((x) >> GSS_C_ROUTINE_ERROR_OFFSET) & GSS_C_ROUTINE_ERROR_MASK)
	struct nfsmount *nmp = req->r_nmp;
	char who[] = "client";
	uint32_t gss_error = GETMAJERROR(cp->gss_clnt_major);
	const char *procn = "unkown";
	proc_t proc;
	pid_t pid = -1;
	struct timeval now;

	if (req->r_thread) {
		proc = (proc_t)get_bsdthreadtask_info(req->r_thread);
		if (proc != NULL && (proc->p_fd == NULL || (proc->p_lflag & P_LVFORK))) {
			proc = NULL;
		}
		if (proc) {
			if (*proc->p_comm) {
				procn = proc->p_comm;
			}
			pid = proc->p_pid;
		}
	} else {
		procn = "kernproc";
		pid = 0;
	}

	microuptime(&now);
	if ((cp->gss_clnt_major != major || cp->gss_clnt_minor != minor ||
	    cp->gss_clnt_ptime + GSS_PRINT_DELAY < now.tv_sec) &&
	    (nmp->nm_state & NFSSTA_MOUNTED)) {
		  
                                                                 
                    
     
		if (cp->gss_clnt_minor && cp->gss_clnt_minor != minor) {
			(void) mach_gss_log_error(
				cp->gss_clnt_mport,
				vfs_statfs(nmp->nm_mountp)->f_mntfromname,
				kauth_cred_getuid(cp->gss_clnt_cred),
				who,
				cp->gss_clnt_major,
				cp->gss_clnt_minor);
		}
		gss_error = gss_error ? gss_error : cp->gss_clnt_major;

		  
                                                                                                               
     
		printf("NFS: gssd auth failure by %s on audit session %d uid %d proc %s/%d for mount %s. Error: major = %d minor = %d\n",
		    cp->gss_clnt_display ? cp->gss_clnt_display : who, kauth_cred_getasid(req->r_cred), kauth_cred_getuid(req->r_cred),
		    procn, pid, vfs_statfs(nmp->nm_mountp)->f_mntfromname, gss_error, (int32_t)cp->gss_clnt_minor);
		cp->gss_clnt_ptime = now.tv_sec;
		switch (gss_error) {
		case 7: printf("NFS: gssd does not have credentials for session %d/%d, (kinit)?\n",
			    kauth_cred_getasid(req->r_cred), kauth_cred_getauid(req->r_cred));
			break;
		case 11: printf("NFS: gssd has expired credentals for session %d/%d, (kinit)?\n",
			    kauth_cred_getasid(req->r_cred), kauth_cred_getauid(req->r_cred));
			break;
		}
	} else {
		NFS_GSS_DBG("NFS: gssd auth failure by %s on audit session %d uid %d proc %s/%d for mount %s. Error: major = %d minor = %d\n",
		    cp->gss_clnt_display ? cp->gss_clnt_display : who, kauth_cred_getasid(req->r_cred), kauth_cred_getuid(req->r_cred),
		    procn, pid, vfs_statfs(nmp->nm_mountp)->f_mntfromname, gss_error, (int32_t)cp->gss_clnt_minor);
	}
}

  
                                            
                                                
                                                 
                                                 
                                                   
   
static int
nfs_gss_clnt_gssd_upcall(struct nfsreq *req, struct nfs_gss_clnt_ctx *cp, uint32_t retrycnt)
{
	kern_return_t kr;
	gssd_byte_buffer octx = NULL;
	uint32_t lucidlen = 0;
	void *lucid_ctx_buffer;
	int retry_cnt = 0;
	vm_map_copy_t itoken = NULL;
	gssd_byte_buffer otoken = NULL;
	mach_msg_type_number_t otokenlen;
	int error = 0;
	uint8_t *principal = NULL;
	size_t plen = 0;
	int32_t nt = GSSD_STRING_NAME;
	vm_map_copy_t pname = NULL;
	vm_map_copy_t svcname = NULL;
	char display_name[MAX_DISPLAY_STR] = "";
	uint32_t ret_flags;
	struct nfsmount *nmp = req->r_nmp;
	uint32_t major = cp->gss_clnt_major, minor = cp->gss_clnt_minor;
	uint32_t selected = (uint32_t)-1;
	struct nfs_etype etype;

	if (nmp == NULL || vfs_isforce(nmp->nm_mountp) || (nmp->nm_state & (NFSSTA_FORCE | NFSSTA_DEAD))) {
		return ENXIO;
	}

	if (cp->gss_clnt_gssd_flags & GSSD_RESTART) {
		if (cp->gss_clnt_token) {
			FREE(cp->gss_clnt_token, M_TEMP);
		}
		cp->gss_clnt_token = NULL;
		cp->gss_clnt_tokenlen = 0;
		cp->gss_clnt_proc = RPCSEC_GSS_INIT;
		                                              
		cp->gss_clnt_handle_len = 0;
		if (cp->gss_clnt_handle != NULL) {
			FREE(cp->gss_clnt_handle, M_TEMP);
			cp->gss_clnt_handle = NULL;
		}
	}

	NFS_GSS_DBG("Retrycnt = %d nm_etype.count = %d\n", retrycnt, nmp->nm_etype.count);
	if (retrycnt >= nmp->nm_etype.count) {
		return EACCES;
	}

	                                                            
	etype = nmp->nm_etype;

	  
                                                                  
                                                                
                            
    
	if (etype.selected < etype.count) {
		etype.etypes[0] = nmp->nm_etype.etypes[etype.selected];
		for (uint32_t i = 0; i < etype.selected; i++) {
			etype.etypes[i + 1] = nmp->nm_etype.etypes[i];
		}
		for (uint32_t i = etype.selected + 1; i < etype.count; i++) {
			etype.etypes[i] = nmp->nm_etype.etypes[i];
		}
	}

	                                              
	for (uint32_t i = retrycnt; i < etype.count; i++) {
		etype.etypes[i - retrycnt] = etype.etypes[i];
	}
	etype.count = etype.count - retrycnt;

	NFS_GSS_DBG("etype count = %d preferred etype = %d\n", etype.count, etype.etypes[0]);

	  
                                                     
                                                     
                                                            
                                                              
                                                              
                                                               
                             
    

	if (cp->gss_clnt_principal && cp->gss_clnt_prinlen) {
		principal = cp->gss_clnt_principal;
		plen = cp->gss_clnt_prinlen;
		nt = cp->gss_clnt_prinnt;
	} else if (nmp->nm_principal && IS_VALID_CRED(nmp->nm_mcred) && req->r_cred == nmp->nm_mcred) {
		plen = (uint32_t)strlen(nmp->nm_principal);
		principal = (uint8_t *)nmp->nm_principal;
		cp->gss_clnt_prinnt = nt = GSSD_USER;
	} else if (nmp->nm_realm) {
		plen = (uint32_t)strlen(nmp->nm_realm);
		principal = (uint8_t *)nmp->nm_realm;
		nt = GSSD_USER;
	}

	if (!IPC_PORT_VALID(cp->gss_clnt_mport)) {
		cp->gss_clnt_mport = nfs_gss_clnt_get_upcall_port(req->r_cred);
		if (cp->gss_clnt_mport == IPC_PORT_NULL) {
			goto out;
		}
	}

	if (plen) {
		nfs_gss_mach_alloc_buffer(principal, plen, &pname);
	}
	if (cp->gss_clnt_svcnamlen) {
		nfs_gss_mach_alloc_buffer(cp->gss_clnt_svcname, cp->gss_clnt_svcnamlen, &svcname);
	}
	if (cp->gss_clnt_tokenlen) {
		nfs_gss_mach_alloc_buffer(cp->gss_clnt_token, cp->gss_clnt_tokenlen, &itoken);
	}

	                                             
	cp->gss_clnt_gssd_flags |= GSSD_LUCID_CONTEXT;

retry:
	kr = mach_gss_init_sec_context_v3(
		cp->gss_clnt_mport,
		GSSD_KRB5_MECH,
		(gssd_byte_buffer) itoken, (mach_msg_type_number_t) cp->gss_clnt_tokenlen,
		kauth_cred_getuid(cp->gss_clnt_cred),
		nt,
		(gssd_byte_buffer)pname, (mach_msg_type_number_t) plen,
		cp->gss_clnt_svcnt,
		(gssd_byte_buffer)svcname, (mach_msg_type_number_t) cp->gss_clnt_svcnamlen,
		GSSD_MUTUAL_FLAG,
		(gssd_etype_list)etype.etypes, (mach_msg_type_number_t)etype.count,
		&cp->gss_clnt_gssd_flags,
		&cp->gss_clnt_context,
		&cp->gss_clnt_cred_handle,
		&ret_flags,
		&octx, (mach_msg_type_number_t *) &lucidlen,
		&otoken, &otokenlen,
		cp->gss_clnt_display ? NULL : display_name,
		&cp->gss_clnt_major,
		&cp->gss_clnt_minor);

	                            
	cp->gss_clnt_gssd_flags &= ~GSSD_RESTART;
	if (cp->gss_clnt_major != GSS_S_CONTINUE_NEEDED) {
		                                      
		cp->gss_clnt_context = 0;
		cp->gss_clnt_cred_handle = 0;
	}

	if (kr != KERN_SUCCESS) {
		printf("nfs_gss_clnt_gssd_upcall: mach_gss_init_sec_context failed: %x (%d)\n", kr, kr);
		if (kr == MIG_SERVER_DIED && cp->gss_clnt_cred_handle == 0 &&
		    retry_cnt++ < NFS_GSS_MACH_MAX_RETRIES &&
		    !vfs_isforce(nmp->nm_mountp) && (nmp->nm_state & (NFSSTA_FORCE | NFSSTA_DEAD)) == 0) {
			if (plen) {
				nfs_gss_mach_alloc_buffer(principal, plen, &pname);
			}
			if (cp->gss_clnt_svcnamlen) {
				nfs_gss_mach_alloc_buffer(cp->gss_clnt_svcname, cp->gss_clnt_svcnamlen, &svcname);
			}
			if (cp->gss_clnt_tokenlen > 0) {
				nfs_gss_mach_alloc_buffer(cp->gss_clnt_token, cp->gss_clnt_tokenlen, &itoken);
			}
			goto retry;
		}

		host_release_special_port(cp->gss_clnt_mport);
		cp->gss_clnt_mport = IPC_PORT_NULL;
		goto out;
	}

	if (cp->gss_clnt_display == NULL && *display_name != '\0') {
		size_t dlen = strnlen(display_name, MAX_DISPLAY_STR) + 1;                                      

		if (dlen < MAX_DISPLAY_STR) {
			MALLOC(cp->gss_clnt_display, char *, dlen, M_TEMP, M_WAITOK);
			if (cp->gss_clnt_display == NULL) {
				goto skip;
			}
			bcopy(display_name, cp->gss_clnt_display, dlen);
		} else {
			goto skip;
		}
	}
skip:
	  
                                                                
   
                                                                                                   
    
	if (cp->gss_clnt_major != GSS_S_COMPLETE &&
	    cp->gss_clnt_major != GSS_S_CONTINUE_NEEDED) {
		NFS_GSS_DBG("Up call returned error\n");
		nfs_gss_clnt_log_error(req, cp, major, minor);
		                                              
		cp->gss_clnt_handle_len = 0;
		if (cp->gss_clnt_handle != NULL) {
			FREE(cp->gss_clnt_handle, M_TEMP);
			cp->gss_clnt_handle = NULL;
		}
	}

	if (lucidlen > 0) {
		if (lucidlen > MAX_LUCIDLEN) {
			printf("nfs_gss_clnt_gssd_upcall: bad context length (%d)\n", lucidlen);
			vm_map_copy_discard((vm_map_copy_t) octx);
			vm_map_copy_discard((vm_map_copy_t) otoken);
			goto out;
		}
		MALLOC(lucid_ctx_buffer, void *, lucidlen, M_TEMP, M_WAITOK | M_ZERO);
		error = nfs_gss_mach_vmcopyout((vm_map_copy_t) octx, lucidlen, lucid_ctx_buffer);
		if (error) {
			vm_map_copy_discard((vm_map_copy_t) otoken);
			goto out;
		}

		if (cp->gss_clnt_ctx_id) {
			gss_krb5_destroy_context(cp->gss_clnt_ctx_id);
		}
		cp->gss_clnt_ctx_id = gss_krb5_make_context(lucid_ctx_buffer, lucidlen);
		if (cp->gss_clnt_ctx_id == NULL) {
			printf("Failed to make context from lucid_ctx_buffer\n");
			goto out;
		}
		for (uint32_t i = 0; i < nmp->nm_etype.count; i++) {
			if (nmp->nm_etype.etypes[i] == cp->gss_clnt_ctx_id->gss_cryptor.etype) {
				selected = i;
				break;
			}
		}
	}

	                                      
	if (cp->gss_clnt_token) {
		FREE(cp->gss_clnt_token, M_TEMP);
	}
	cp->gss_clnt_token = NULL;
	cp->gss_clnt_tokenlen = 0;

	if (otokenlen > 0) {
		                                           
		MALLOC(cp->gss_clnt_token, u_char *, otokenlen, M_TEMP, M_WAITOK);
		if (cp->gss_clnt_token == NULL) {
			printf("nfs_gss_clnt_gssd_upcall: could not allocate %d bytes\n", otokenlen);
			vm_map_copy_discard((vm_map_copy_t) otoken);
			return ENOMEM;
		}
		error = nfs_gss_mach_vmcopyout((vm_map_copy_t) otoken, otokenlen, cp->gss_clnt_token);
		if (error) {
			printf("Could not copyout gss token\n");
			FREE(cp->gss_clnt_token, M_TEMP);
			cp->gss_clnt_token = NULL;
			return NFSERR_EAUTH;
		}
		cp->gss_clnt_tokenlen = otokenlen;
	}

	if (selected != (uint32_t)-1) {
		nmp->nm_etype.selected = selected;
		NFS_GSS_DBG("etype selected = %d\n", nmp->nm_etype.etypes[selected]);
	}
	NFS_GSS_DBG("Up call succeeded major = %d\n", cp->gss_clnt_major);
	return 0;

out:
	if (cp->gss_clnt_token) {
		FREE(cp->gss_clnt_token, M_TEMP);
	}
	cp->gss_clnt_token = NULL;
	cp->gss_clnt_tokenlen = 0;
	                                              
	cp->gss_clnt_handle_len = 0;
	if (cp->gss_clnt_handle != NULL) {
		FREE(cp->gss_clnt_handle, M_TEMP);
		cp->gss_clnt_handle = NULL;
	}

	NFS_GSS_DBG("Up call returned NFSERR_EAUTH");
	return NFSERR_EAUTH;
}

  
                                                                   
                                                                 
                                                                     
                                                                      
                                                                      
                                                                      
                                                                       
                                                  
  
                                                                     
                                                                  
                                                                        
   
void
nfs_gss_clnt_rpcdone(struct nfsreq *req)
{
	struct nfs_gss_clnt_ctx *cp = req->r_gss_ctx;
	struct gss_seq *gsp, *ngsp;
	int i = 0;

	if (cp == NULL || !(cp->gss_clnt_flags & GSS_CTX_COMPLETE)) {
		return;                             
	}
	  
                                         
                                                 
                                             
    
	lck_mtx_lock(&cp->gss_clnt_mtx);
	gsp = SLIST_FIRST(&req->r_gss_seqlist);
	if (gsp && gsp->gss_seqnum > (cp->gss_clnt_seqnum - cp->gss_clnt_seqwin)) {
		win_resetbit(cp->gss_clnt_seqbits,
		    gsp->gss_seqnum % cp->gss_clnt_seqwin);
	}

	  
                                                        
    
	SLIST_FOREACH_SAFE(gsp, &req->r_gss_seqlist, gss_seqnext, ngsp) {
		if (++i > GSS_CLNT_SEQLISTMAX) {
			SLIST_REMOVE(&req->r_gss_seqlist, gsp, gss_seq, gss_seqnext);
			FREE(gsp, M_TEMP);
		}
	}

	  
                                   
                                      
    
	if (cp->gss_clnt_flags & GSS_NEEDSEQ) {
		cp->gss_clnt_flags &= ~GSS_NEEDSEQ;
		wakeup(cp);
	}
	lck_mtx_unlock(&cp->gss_clnt_mtx);
}

  
                                                 
                               
   
void
nfs_gss_clnt_ctx_ref(struct nfsreq *req, struct nfs_gss_clnt_ctx *cp)
{
	req->r_gss_ctx = cp;

	lck_mtx_lock(&cp->gss_clnt_mtx);
	cp->gss_clnt_refcnt++;
	lck_mtx_unlock(&cp->gss_clnt_mtx);
}

  
                                            
                                                
                                          
   
void
nfs_gss_clnt_ctx_unref(struct nfsreq *req)
{
	struct nfsmount *nmp = req->r_nmp;
	struct nfs_gss_clnt_ctx *cp = req->r_gss_ctx;
	int on_neg_cache = 0;
	int neg_cache = 0;
	int destroy = 0;
	struct timeval now;
	char CTXBUF[NFS_CTXBUFSZ];

	if (cp == NULL) {
		return;
	}

	req->r_gss_ctx = NULL;

	lck_mtx_lock(&cp->gss_clnt_mtx);
	if (--cp->gss_clnt_refcnt < 0) {
		panic("Over release of gss context!\n");
	}

	if (cp->gss_clnt_refcnt == 0) {
		if ((cp->gss_clnt_flags & GSS_CTX_INVAL) &&
		    cp->gss_clnt_ctx_id) {
			gss_krb5_destroy_context(cp->gss_clnt_ctx_id);
			cp->gss_clnt_ctx_id = NULL;
		}
		if (cp->gss_clnt_flags & GSS_CTX_DESTROY) {
			destroy = 1;
			if ((cp->gss_clnt_flags & GSS_CTX_USECOUNT) && !nfs_gss_clnt_mnt_rele(nmp)) {
				cp->gss_clnt_flags &= ~GSS_CTX_USECOUNT;
			}
			if (cp->gss_clnt_nctime) {
				on_neg_cache = 1;
			}
		}
	}
	if (!destroy && cp->gss_clnt_nctime == 0 &&
	    (cp->gss_clnt_flags & GSS_CTX_INVAL)) {
		microuptime(&now);
		cp->gss_clnt_nctime = now.tv_sec;
		neg_cache = 1;
	}
	lck_mtx_unlock(&cp->gss_clnt_mtx);
	if (destroy) {
		NFS_GSS_DBG("Destroying context %s\n", NFS_GSS_CTX(req, cp));
		if (nmp) {
			lck_mtx_lock(&nmp->nm_lock);
			if (cp->gss_clnt_entries.tqe_next != NFSNOLIST) {
				TAILQ_REMOVE(&nmp->nm_gsscl, cp, gss_clnt_entries);
			}
			if (on_neg_cache) {
				nmp->nm_ncentries--;
			}
			lck_mtx_unlock(&nmp->nm_lock);
		}
		nfs_gss_clnt_ctx_destroy(cp);
	} else if (neg_cache) {
		NFS_GSS_DBG("Entering context %s into negative cache\n", NFS_GSS_CTX(req, cp));
		if (nmp) {
			lck_mtx_lock(&nmp->nm_lock);
			nmp->nm_ncentries++;
			nfs_gss_clnt_ctx_neg_cache_reap(nmp);
			lck_mtx_unlock(&nmp->nm_lock);
		}
	}
	NFS_GSS_CLNT_CTX_DUMP(nmp);
}

  
                                               
               
   
void
nfs_gss_clnt_ctx_neg_cache_reap(struct nfsmount *nmp)
{
	struct nfs_gss_clnt_ctx *cp, *tcp;
	struct timeval now;
	int reaped = 0;

	                                                      
	microuptime(&now);

	NFS_GSS_DBG("Reaping contexts ncentries = %d\n", nmp->nm_ncentries);

	TAILQ_FOREACH_SAFE(cp, &nmp->nm_gsscl, gss_clnt_entries, tcp) {
		int destroy = 0;

		                                
		if ((cp->gss_clnt_flags & GSS_CTX_STICKY) ||
		    !(cp->gss_clnt_flags & GSS_CTX_INVAL)) {
			continue;
		}
		                                          
		if (nmp->nm_ncentries <= GSS_MAX_NEG_CACHE_ENTRIES) {
			break;
		}
		                        
		if (cp->gss_clnt_nctime + GSS_NEG_CACHE_TO >= now.tv_sec) {
			continue;
		}
		                                
		lck_mtx_lock(&cp->gss_clnt_mtx);
		if (cp->gss_clnt_refcnt == 0) {
			cp->gss_clnt_flags |= GSS_CTX_DESTROY;
			destroy = 1;
		}
		lck_mtx_unlock(&cp->gss_clnt_mtx);
		if (destroy) {
			TAILQ_REMOVE(&nmp->nm_gsscl, cp, gss_clnt_entries);
			nmp->nm_ncentries++;
			reaped++;
			nfs_gss_clnt_ctx_destroy(cp);
		}
	}
	NFS_GSS_DBG("Reaped %d contexts ncentries = %d\n", reaped, nmp->nm_ncentries);
}

  
                               
   
static void
nfs_gss_clnt_ctx_clean(struct nfs_gss_clnt_ctx *cp)
{
	                           
	assert(cp->gss_clnt_thread == NULL);                                  
	                                                                  
	cp->gss_clnt_flags = 0;
	                                    
	assert(cp->gss_clnt_refcnt == 0);
	  
                               
                 
                      
                    
                   
                    
    
	                                                        
	cp->gss_clnt_seqnum = 0;
	                                                           
	if (cp->gss_clnt_handle) {
		FREE(cp->gss_clnt_handle, M_TEMP);
		cp->gss_clnt_handle = NULL;
	}
	cp->gss_clnt_handle_len = 0;
	cp->gss_clnt_nctime = 0;
	cp->gss_clnt_seqwin = 0;
	if (cp->gss_clnt_seqbits) {
		FREE(cp->gss_clnt_seqbits, M_TEMP);
		cp->gss_clnt_seqbits = NULL;
	}
	                                                             
	if (cp->gss_clnt_verf) {
		FREE(cp->gss_clnt_verf, M_TEMP);
		cp->gss_clnt_verf = NULL;
	}
	                                                        
	if (cp->gss_clnt_svcname) {
		FREE(cp->gss_clnt_svcname, M_TEMP);
		cp->gss_clnt_svcname = NULL;
		cp->gss_clnt_svcnt = 0;
	}
	cp->gss_clnt_svcnamlen = 0;
	cp->gss_clnt_cred_handle = 0;
	cp->gss_clnt_context = 0;
	if (cp->gss_clnt_token) {
		FREE(cp->gss_clnt_token, M_TEMP);
		cp->gss_clnt_token = NULL;
	}
	cp->gss_clnt_tokenlen = 0;
	                             
	  
             
                       
                  
                  
                  
    
}

  
                                                                               
                                                                               
                                                                                  
           
   
static int
nfs_gss_clnt_ctx_copy(struct nfs_gss_clnt_ctx *scp, struct nfs_gss_clnt_ctx **dcpp)
{
	struct nfs_gss_clnt_ctx *dcp;

	*dcpp = (struct nfs_gss_clnt_ctx *)NULL;
	MALLOC(dcp, struct nfs_gss_clnt_ctx *, sizeof(struct nfs_gss_clnt_ctx), M_TEMP, M_WAITOK);
	if (dcp == NULL) {
		return ENOMEM;
	}
	bzero(dcp, sizeof(struct nfs_gss_clnt_ctx));
	lck_mtx_init(&dcp->gss_clnt_mtx, &nfs_gss_clnt_grp, LCK_ATTR_NULL);
	dcp->gss_clnt_cred = scp->gss_clnt_cred;
	kauth_cred_ref(dcp->gss_clnt_cred);
	dcp->gss_clnt_prinlen = scp->gss_clnt_prinlen;
	dcp->gss_clnt_prinnt = scp->gss_clnt_prinnt;
	if (scp->gss_clnt_principal) {
		MALLOC(dcp->gss_clnt_principal, uint8_t *, dcp->gss_clnt_prinlen, M_TEMP, M_WAITOK | M_ZERO);
		if (dcp->gss_clnt_principal == NULL) {
			FREE(dcp, M_TEMP);
			return ENOMEM;
		}
		bcopy(scp->gss_clnt_principal, dcp->gss_clnt_principal, dcp->gss_clnt_prinlen);
	}
	                                                                                       
	dcp->gss_clnt_service = scp->gss_clnt_service;
	dcp->gss_clnt_mport = host_copy_special_port(scp->gss_clnt_mport);
	dcp->gss_clnt_ctx_id = NULL;                                           
	dcp->gss_clnt_gssd_flags = scp->gss_clnt_gssd_flags;
	dcp->gss_clnt_major = scp->gss_clnt_major;
	dcp->gss_clnt_minor = scp->gss_clnt_minor;
	dcp->gss_clnt_ptime = scp->gss_clnt_ptime;

	*dcpp = dcp;

	return 0;
}

  
                   
   
static void
nfs_gss_clnt_ctx_destroy(struct nfs_gss_clnt_ctx *cp)
{
	NFS_GSS_DBG("Destroying context %d/%d\n",
	    kauth_cred_getasid(cp->gss_clnt_cred),
	    kauth_cred_getauid(cp->gss_clnt_cred));

	host_release_special_port(cp->gss_clnt_mport);
	cp->gss_clnt_mport = IPC_PORT_NULL;

	lck_mtx_destroy(&cp->gss_clnt_mtx, &nfs_gss_clnt_grp);

	if (IS_VALID_CRED(cp->gss_clnt_cred)) {
		kauth_cred_unref(&cp->gss_clnt_cred);
	}
	cp->gss_clnt_entries.tqe_next = NFSNOLIST;
	cp->gss_clnt_entries.tqe_prev = NFSNOLIST;
	if (cp->gss_clnt_principal) {
		FREE(cp->gss_clnt_principal, M_TEMP);
		cp->gss_clnt_principal = NULL;
	}
	if (cp->gss_clnt_display) {
		FREE(cp->gss_clnt_display, M_TEMP);
		cp->gss_clnt_display = NULL;
	}
	if (cp->gss_clnt_ctx_id) {
		gss_krb5_destroy_context(cp->gss_clnt_ctx_id);
		cp->gss_clnt_ctx_id = NULL;
	}

	nfs_gss_clnt_ctx_clean(cp);

	FREE(cp, M_TEMP);
}

  
                                     
                                    
                        
   
int
nfs_gss_clnt_ctx_renew(struct nfsreq *req)
{
	struct nfs_gss_clnt_ctx *cp = req->r_gss_ctx;
	struct nfs_gss_clnt_ctx *ncp;
	struct nfsmount *nmp;
	int error = 0;
	char CTXBUF[NFS_CTXBUFSZ];

	if (cp == NULL) {
		return 0;
	}

	if (req->r_nmp == NULL) {
		return ENXIO;
	}
	nmp = req->r_nmp;

	lck_mtx_lock(&cp->gss_clnt_mtx);
	if (cp->gss_clnt_flags & GSS_CTX_INVAL) {
		lck_mtx_unlock(&cp->gss_clnt_mtx);
		nfs_gss_clnt_ctx_unref(req);
		return 0;                             
	}

	cp->gss_clnt_flags |= (GSS_CTX_INVAL | GSS_CTX_DESTROY);

	if (cp->gss_clnt_flags & (GSS_NEEDCTX | GSS_NEEDSEQ)) {
		cp->gss_clnt_flags &= ~GSS_NEEDSEQ;
		wakeup(cp);
	}
	lck_mtx_unlock(&cp->gss_clnt_mtx);

	if (cp->gss_clnt_proc == RPCSEC_GSS_DESTROY) {
		return EACCES;                                                         
	}
	  
                                                                                    
                                
    
	if (cp->gss_clnt_proc != RPCSEC_GSS_DATA) {
		return ENEEDAUTH;
	}
	error =  nfs_gss_clnt_ctx_copy(cp, &ncp);
	NFS_GSS_DBG("Renewing context %s\n", NFS_GSS_CTX(req, ncp));
	nfs_gss_clnt_ctx_unref(req);
	if (error) {
		return error;
	}

	lck_mtx_lock(&nmp->nm_lock);
	  
                                                              
                               
    
	ncp->gss_clnt_thread = current_thread();
	nfs_gss_clnt_ctx_ref(req, ncp);
	TAILQ_INSERT_HEAD(&nmp->nm_gsscl, ncp, gss_clnt_entries);
	lck_mtx_unlock(&nmp->nm_lock);

	error = nfs_gss_clnt_ctx_init_retry(req, ncp);                          
	if (error) {
		nfs_gss_clnt_ctx_unref(req);
	}

	return error;
}


  
                                                    
                                                 
   
void
nfs_gss_clnt_ctx_unmount(struct nfsmount *nmp)
{
	struct nfs_gss_clnt_ctx *cp;
	struct nfsm_chain nmreq, nmrep;
	int error, status;
	struct nfsreq *req;

	if (!nmp) {
		return;
	}

	req = zalloc(nfs_req_zone);
	req->r_nmp = nmp;
	lck_mtx_lock(&nmp->nm_lock);
	while ((cp = TAILQ_FIRST(&nmp->nm_gsscl))) {
		TAILQ_REMOVE(&nmp->nm_gsscl, cp, gss_clnt_entries);
		cp->gss_clnt_entries.tqe_next = NFSNOLIST;
		lck_mtx_lock(&cp->gss_clnt_mtx);
		if (cp->gss_clnt_flags & GSS_CTX_DESTROY) {
			lck_mtx_unlock(&cp->gss_clnt_mtx);
			continue;
		}
		cp->gss_clnt_refcnt++;
		lck_mtx_unlock(&cp->gss_clnt_mtx);
		req->r_gss_ctx = cp;

		lck_mtx_unlock(&nmp->nm_lock);
		  
                                            
                                               
     
		if (!nfs_mount_gone(nmp) &&
		    (cp->gss_clnt_flags & (GSS_CTX_INVAL | GSS_CTX_DESTROY | GSS_CTX_COMPLETE)) == GSS_CTX_COMPLETE) {
			cp->gss_clnt_proc = RPCSEC_GSS_DESTROY;

			error = 0;
			nfsm_chain_null(&nmreq);
			nfsm_chain_null(&nmrep);
			nfsm_chain_build_alloc_init(error, &nmreq, 0);
			nfsm_chain_build_done(error, &nmreq);
			if (!error) {
				nfs_request_gss(nmp->nm_mountp, &nmreq,
				    current_thread(), cp->gss_clnt_cred, 0, cp, &nmrep, &status);
			}
			nfsm_chain_cleanup(&nmreq);
			nfsm_chain_cleanup(&nmrep);
		}

		  
                                       
                                      
                      
     
		lck_mtx_lock(&cp->gss_clnt_mtx);
		cp->gss_clnt_flags |= (GSS_CTX_INVAL | GSS_CTX_DESTROY);
		lck_mtx_unlock(&cp->gss_clnt_mtx);
		nfs_gss_clnt_ctx_unref(req);
		lck_mtx_lock(&nmp->nm_lock);
	}
	lck_mtx_unlock(&nmp->nm_lock);
	assert(TAILQ_EMPTY(&nmp->nm_gsscl));
	NFS_ZFREE(nfs_req_zone, req);
}


  
                                            
   
int
nfs_gss_clnt_ctx_remove(struct nfsmount *nmp, kauth_cred_t cred)
{
	struct nfs_gss_clnt_ctx *cp, *tcp;
	struct nfsreq *req;

	req = zalloc(nfs_req_zone);
	req->r_nmp = nmp;

	NFS_GSS_DBG("Enter\n");
	NFS_GSS_CLNT_CTX_DUMP(nmp);
	lck_mtx_lock(&nmp->nm_lock);
	TAILQ_FOREACH_SAFE(cp, &nmp->nm_gsscl, gss_clnt_entries, tcp) {
		lck_mtx_lock(&cp->gss_clnt_mtx);
		if (nfs_gss_clnt_ctx_cred_match(cp->gss_clnt_cred, cred)) {
			if (cp->gss_clnt_flags & GSS_CTX_DESTROY) {
				NFS_GSS_DBG("Found destroyed context %d/%d. refcnt = %d continuing\n",
				    kauth_cred_getasid(cp->gss_clnt_cred),
				    kauth_cred_getauid(cp->gss_clnt_cred),
				    cp->gss_clnt_refcnt);
				lck_mtx_unlock(&cp->gss_clnt_mtx);
				continue;
			}
			cp->gss_clnt_refcnt++;
			cp->gss_clnt_flags |= (GSS_CTX_INVAL | GSS_CTX_DESTROY);
			lck_mtx_unlock(&cp->gss_clnt_mtx);
			req->r_gss_ctx = cp;
			lck_mtx_unlock(&nmp->nm_lock);
			  
                                            
                       
      
			NFS_GSS_DBG("Removed context %d/%d refcnt = %d\n",
			    kauth_cred_getasid(cp->gss_clnt_cred),
			    kauth_cred_getuid(cp->gss_clnt_cred),
			    cp->gss_clnt_refcnt);
			nfs_gss_clnt_ctx_unref(req);
			NFS_ZFREE(nfs_req_zone, req);
			return 0;
		}
		lck_mtx_unlock(&cp->gss_clnt_mtx);
	}

	lck_mtx_unlock(&nmp->nm_lock);

	NFS_ZFREE(nfs_req_zone, req);
	NFS_GSS_DBG("Returning ENOENT\n");
	return ENOENT;
}

  
                                                              
   
int
nfs_gss_clnt_ctx_set_principal(struct nfsmount *nmp, vfs_context_t ctx,
    uint8_t *principal, size_t princlen, uint32_t nametype)
{
	struct nfsreq *req;
	int error;

	NFS_GSS_DBG("Enter:\n");

	req = zalloc_flags(nfs_req_zone, Z_WAITOK | Z_ZERO);
	req->r_nmp = nmp;
	req->r_auth = nmp->nm_auth;
	req->r_thread = vfs_context_thread(ctx);
	req->r_cred = vfs_context_ucred(ctx);

	error = nfs_gss_clnt_ctx_find_principal(req, principal, princlen, nametype);
	NFS_GSS_DBG("nfs_gss_clnt_ctx_find_principal returned %d\n", error);
	  
                                                                                    
                                                                              
                                                                                        
    
	if (error == EACCES || error == EAUTH || error == ENEEDAUTH) {
		error = 0;
	}

	                                  
	nfs_gss_clnt_ctx_unref(req);
	NFS_ZFREE(nfs_req_zone, req);
	return error;
}

  
                                                              
   
int
nfs_gss_clnt_ctx_get_principal(struct nfsmount *nmp, vfs_context_t ctx,
    struct user_nfs_gss_principal *p)
{
	struct nfsreq *req;
	int error = 0;
	struct nfs_gss_clnt_ctx *cp;
	kauth_cred_t cred = vfs_context_ucred(ctx);
	const char *princ = NULL;
	char CTXBUF[NFS_CTXBUFSZ];

	                                                                                    
	p->nametype = GSSD_STRING_NAME;
	p->principal = USER_ADDR_NULL;
	p->princlen = 0;
	p->flags = 0;

	req = zalloc_flags(nfs_req_zone, Z_WAITOK);
	req->r_nmp = nmp;
	lck_mtx_lock(&nmp->nm_lock);
	TAILQ_FOREACH(cp, &nmp->nm_gsscl, gss_clnt_entries) {
		lck_mtx_lock(&cp->gss_clnt_mtx);
		if (cp->gss_clnt_flags & GSS_CTX_DESTROY) {
			NFS_GSS_DBG("Found destroyed context %s refcnt = %d continuing\n",
			    NFS_GSS_CTX(req, cp),
			    cp->gss_clnt_refcnt);
			lck_mtx_unlock(&cp->gss_clnt_mtx);
			continue;
		}
		if (nfs_gss_clnt_ctx_cred_match(cp->gss_clnt_cred, cred)) {
			cp->gss_clnt_refcnt++;
			lck_mtx_unlock(&cp->gss_clnt_mtx);
			goto out;
		}
		lck_mtx_unlock(&cp->gss_clnt_mtx);
	}

out:
	if (cp == NULL) {
		lck_mtx_unlock(&nmp->nm_lock);
		p->flags |= NFS_IOC_NO_CRED_FLAG;                                                      
		NFS_GSS_DBG("No context found for session %d by uid %d\n",
		    kauth_cred_getasid(cred), kauth_cred_getuid(cred));
		NFS_ZFREE(nfs_req_zone, req);
		return 0;
	}

	                                     
	if (cp->gss_clnt_flags & GSS_CTX_INVAL) {
		p->flags |= NFS_IOC_INVALID_CRED_FLAG;
	}

	                                          
	if (cp->gss_clnt_principal) {
		princ = (char *)cp->gss_clnt_principal;
		p->princlen = cp->gss_clnt_prinlen;
		p->nametype = cp->gss_clnt_prinnt;
	} else if (cp->gss_clnt_display) {
		                                                         
		princ = cp->gss_clnt_display;
		p->princlen = strlen(cp->gss_clnt_display);
	}

	  
                                                                         
                                                                  
    

	if (princ) {
		char *pp;

		MALLOC(pp, char *, p->princlen, M_TEMP, M_WAITOK);
		bcopy(princ, pp, p->princlen);
		p->principal = CAST_USER_ADDR_T(pp);
	}

	lck_mtx_unlock(&nmp->nm_lock);

	req->r_gss_ctx = cp;
	NFS_GSS_DBG("Found context %s\n", NFS_GSS_CTX(req, NULL));
	nfs_gss_clnt_ctx_unref(req);
	NFS_ZFREE(nfs_req_zone, req);
	return error;
}
#endif                        

              
  
                   
   

#if CONFIG_NFS_SERVER

  
                                                         
                               
   
static struct nfs_gss_svc_ctx *
nfs_gss_svc_ctx_find(uint32_t handle)
{
	struct nfs_gss_svc_ctx_hashhead *head;
	struct nfs_gss_svc_ctx *cp;
	uint64_t timenow;

	if (handle == 0) {
		return NULL;
	}

	head = &nfs_gss_svc_ctx_hashtbl[SVC_CTX_HASH(handle)];
	  
                                                                          
    
	clock_interval_to_deadline(GSS_CTX_PEND, NSEC_PER_SEC, &timenow);

	lck_mtx_lock(&nfs_gss_svc_ctx_mutex);

	LIST_FOREACH(cp, head, gss_svc_entries) {
		if (cp->gss_svc_handle == handle) {
			if (timenow > cp->gss_svc_incarnation + GSS_SVC_CTX_TTL) {
				  
                                                    
                                                           
                     
       
				cp->gss_svc_handle = 0;
				  
                                                                    
                                                         
       
				cp->gss_svc_incarnation = timenow;

				cp = NULL;
				break;
			}
			lck_mtx_lock(&cp->gss_svc_mtx);
			cp->gss_svc_refcnt++;
			lck_mtx_unlock(&cp->gss_svc_mtx);
			break;
		}
	}

	lck_mtx_unlock(&nfs_gss_svc_ctx_mutex);

	return cp;
}

  
                                                  
                                                  
   
static void
nfs_gss_svc_ctx_insert(struct nfs_gss_svc_ctx *cp)
{
	struct nfs_gss_svc_ctx_hashhead *head;
	struct nfs_gss_svc_ctx *p;

	lck_mtx_lock(&nfs_gss_svc_ctx_mutex);

	  
                                                        
                                                          
                                                
    
retry:
	cp->gss_svc_handle = random();
	if (cp->gss_svc_handle == 0) {
		goto retry;
	}
	head = &nfs_gss_svc_ctx_hashtbl[SVC_CTX_HASH(cp->gss_svc_handle)];
	LIST_FOREACH(p, head, gss_svc_entries)
	if (p->gss_svc_handle == cp->gss_svc_handle) {
		goto retry;
	}

	clock_interval_to_deadline(GSS_CTX_PEND, NSEC_PER_SEC,
	    &cp->gss_svc_incarnation);
	LIST_INSERT_HEAD(head, cp, gss_svc_entries);
	nfs_gss_ctx_count++;

	if (!nfs_gss_timer_on) {
		nfs_gss_timer_on = 1;

		nfs_interval_timer_start(nfs_gss_svc_ctx_timer_call,
		    min(GSS_TIMER_PERIOD, max(GSS_CTX_TTL_MIN, nfsrv_gss_context_ttl)) * MSECS_PER_SEC);
	}

	lck_mtx_unlock(&nfs_gss_svc_ctx_mutex);
}

  
                                                   
                                          
                              
   
void
nfs_gss_svc_ctx_timer(__unused void *param1, __unused void *param2)
{
	struct nfs_gss_svc_ctx *cp, *next;
	uint64_t timenow;
	int contexts = 0;
	int i;

	lck_mtx_lock(&nfs_gss_svc_ctx_mutex);
	clock_get_uptime(&timenow);

	NFS_GSS_DBG("is running\n");

	  
                            
    
	for (i = 0; i < SVC_CTX_HASHSZ; i++) {
		  
                                          
                                       
     
		LIST_FOREACH_SAFE(cp, &nfs_gss_svc_ctx_hashtbl[i], gss_svc_entries, next) {
			contexts++;
			if (timenow > cp->gss_svc_incarnation +
			    (cp->gss_svc_handle ? GSS_SVC_CTX_TTL : 0)
			    && cp->gss_svc_refcnt == 0) {
				  
                                  
       
				LIST_REMOVE(cp, gss_svc_entries);
				NFS_GSS_DBG("Removing contex for %d\n", cp->gss_svc_uid);
				if (cp->gss_svc_seqbits) {
					FREE(cp->gss_svc_seqbits, M_TEMP);
				}
				lck_mtx_destroy(&cp->gss_svc_mtx, &nfs_gss_svc_grp);
				FREE(cp, M_TEMP);
				contexts--;
			}
		}
	}

	nfs_gss_ctx_count = contexts;

	  
                                                 
                                                  
    
	nfs_gss_timer_on = nfs_gss_ctx_count > 0;
	if (nfs_gss_timer_on) {
		nfs_interval_timer_start(nfs_gss_svc_ctx_timer_call,
		    min(GSS_TIMER_PERIOD, max(GSS_CTX_TTL_MIN, nfsrv_gss_context_ttl)) * MSECS_PER_SEC);
	}

	lck_mtx_unlock(&nfs_gss_svc_ctx_mutex);
}

  
                                                          
                                                             
                                                               
                                                                
                                                                 
                                                              
                                                             
                                                                 
   
int
nfs_gss_svc_cred_get(struct nfsrv_descript *nd, struct nfsm_chain *nmc)
{
	uint32_t vers, proc, seqnum, service;
	uint32_t handle, handle_len;
	uint32_t major;
	struct nfs_gss_svc_ctx *cp = NULL;
	uint32_t flavor = 0;
	int error = 0;
	uint32_t arglen;
	size_t argsize, start, header_len;
	gss_buffer_desc cksum;
	struct nfsm_chain nmc_tmp;
	mbuf_t reply_mbuf, prev_mbuf, pad_mbuf;

	vers = proc = seqnum = service = handle_len = 0;
	arglen = 0;

	nfsm_chain_get_32(error, nmc, vers);
	if (vers != RPCSEC_GSS_VERS_1) {
		error = NFSERR_AUTHERR | AUTH_REJECTCRED;
		goto nfsmout;
	}

	nfsm_chain_get_32(error, nmc, proc);
	nfsm_chain_get_32(error, nmc, seqnum);
	nfsm_chain_get_32(error, nmc, service);
	nfsm_chain_get_32(error, nmc, handle_len);
	if (error) {
		goto nfsmout;
	}

	  
                                                                 
    
	if (proc != RPCSEC_GSS_DATA && nd->nd_procnum != NFSPROC_NULL) {
		error = NFSERR_AUTHERR | RPCSEC_GSS_CREDPROBLEM;
		goto nfsmout;
	}

	  
                                                  
                                                
                
    
	if (seqnum > GSS_MAXSEQ) {
		error = NFSERR_AUTHERR | RPCSEC_GSS_CTXPROBLEM;
		goto nfsmout;
	}

	nd->nd_sec =
	    service == RPCSEC_GSS_SVC_NONE ?      RPCAUTH_KRB5 :
	    service == RPCSEC_GSS_SVC_INTEGRITY ? RPCAUTH_KRB5I :
	    service == RPCSEC_GSS_SVC_PRIVACY ?   RPCAUTH_KRB5P : 0;

	if (proc == RPCSEC_GSS_INIT) {
		  
                                       
     
		if (nfs_gss_ctx_count > nfs_gss_ctx_max) {
			error = NFSERR_AUTHERR | RPCSEC_GSS_CTXPROBLEM;
			goto nfsmout;
		}

		  
                         
     
		MALLOC(cp, struct nfs_gss_svc_ctx *, sizeof(*cp), M_TEMP, M_WAITOK | M_ZERO);
		if (cp == NULL) {
			error = ENOMEM;
			goto nfsmout;
		}
		lck_mtx_init(&cp->gss_svc_mtx, &nfs_gss_svc_grp, LCK_ATTR_NULL);
		cp->gss_svc_refcnt = 1;
	} else {
		  
                                       
     
		if (handle_len != sizeof(handle)) {
			error = NFSERR_AUTHERR | RPCSEC_GSS_CREDPROBLEM;
			goto nfsmout;
		}
		nfsm_chain_get_32(error, nmc, handle);
		if (error) {
			goto nfsmout;
		}
		cp = nfs_gss_svc_ctx_find(handle);
		if (cp == NULL) {
			error = NFSERR_AUTHERR | RPCSEC_GSS_CTXPROBLEM;
			goto nfsmout;
		}
	}

	cp->gss_svc_proc = proc;

	if (proc == RPCSEC_GSS_DATA || proc == RPCSEC_GSS_DESTROY) {
		struct posix_cred temp_pcred;

		if (cp->gss_svc_seqwin == 0) {
			  
                            
      
			error = NFSERR_AUTHERR | RPCSEC_GSS_CTXPROBLEM;
			goto nfsmout;
		}

		if (!nfs_gss_svc_seqnum_valid(cp, seqnum)) {
			  
                            
      
			error = EINVAL;                    
			goto nfsmout;
		}

		  
                           
                                                
                                              
                                              
                                              
                  
     
		header_len = nfsm_chain_offset(nmc);
		nfsm_chain_get_32(error, nmc, flavor);
		nfsm_chain_get_32(error, nmc, cksum.length);
		if (error) {
			goto nfsmout;
		}
		if (flavor != RPCSEC_GSS || cksum.length > KRB5_MAX_MIC_SIZE) {
			error = NFSERR_AUTHERR | AUTH_BADVERF;
		} else {
			MALLOC(cksum.value, void *, cksum.length, M_TEMP, M_WAITOK);
			nfsm_chain_get_opaque(error, nmc, cksum.length, cksum.value);
		}
		if (error) {
			goto nfsmout;
		}

		                                                  
		major = gss_krb5_verify_mic_mbuf((uint32_t *)&error, cp->gss_svc_ctx_id, nmc->nmc_mhead, 0, header_len, &cksum, NULL);
		(void)gss_release_buffer(NULL, &cksum);
		if (major != GSS_S_COMPLETE) {
			printf("Server header: gss_krb5_verify_mic_mbuf failed %d\n", error);
			error = NFSERR_AUTHERR | RPCSEC_GSS_CTXPROBLEM;
			goto nfsmout;
		}

		nd->nd_gss_seqnum = seqnum;

		  
                           
     
		bzero(&temp_pcred, sizeof(temp_pcred));
		temp_pcred.cr_uid = cp->gss_svc_uid;
		bcopy(cp->gss_svc_gids, temp_pcred.cr_groups,
		    sizeof(gid_t) * cp->gss_svc_ngroups);
		temp_pcred.cr_ngroups = (short)cp->gss_svc_ngroups;

		nd->nd_cr = posix_cred_create(&temp_pcred);
		if (nd->nd_cr == NULL) {
			error = ENOMEM;
			goto nfsmout;
		}
		clock_get_uptime(&cp->gss_svc_incarnation);

		  
                                                             
                                     
     
		switch (service) {
		case RPCSEC_GSS_SVC_NONE:
			                   
			break;
		case RPCSEC_GSS_SVC_INTEGRITY:
			  
                                                       
     
                                               
                                 
                                  
                                
                                      
      
			nfsm_chain_get_32(error, nmc, arglen);                           
			if (arglen > NFS_MAXPACKET) {
				error = EBADRPC;
				goto nfsmout;
			}

			nmc_tmp = *nmc;
			nfsm_chain_adv(error, &nmc_tmp, arglen);
			nfsm_chain_get_32(error, &nmc_tmp, cksum.length);
			cksum.value = NULL;
			if (cksum.length > 0 && cksum.length < GSS_MAX_MIC_LEN) {
				MALLOC(cksum.value, void *, cksum.length, M_TEMP, M_WAITOK);
			}

			if (cksum.value == NULL) {
				error = EBADRPC;
				goto nfsmout;
			}
			nfsm_chain_get_opaque(error, &nmc_tmp, cksum.length, cksum.value);

			                                            
			start = nfsm_chain_offset(nmc);

			major = gss_krb5_verify_mic_mbuf((uint32_t *)&error, cp->gss_svc_ctx_id,
			    nmc->nmc_mhead, start, arglen, &cksum, NULL);
			FREE(cksum.value, M_TEMP);
			if (major != GSS_S_COMPLETE) {
				printf("Server args: gss_krb5_verify_mic_mbuf failed %d\n", error);
				error = EBADRPC;
				goto nfsmout;
			}

			  
                                                   
                                                
                      
      
			nfsm_chain_get_32(error, nmc, seqnum);
			if (seqnum != nd->nd_gss_seqnum) {
				error = EBADRPC;                                                 
				goto nfsmout;
			}
			break;
		case RPCSEC_GSS_SVC_PRIVACY:
			  
                                                     
     
                            
                                
      
			prev_mbuf = nmc->nmc_mcur;
			nfsm_chain_get_32(error, nmc, arglen);                           
			if (arglen > NFS_MAXPACKET) {
				error = EBADRPC;
				goto nfsmout;
			}

			                                                                                   
			start = nmc->nmc_ptr - (caddr_t)mbuf_data(nmc->nmc_mcur);

			                              
			argsize = arglen;
			error = gss_normalize_mbuf(nmc->nmc_mcur, start, &argsize, &reply_mbuf, &pad_mbuf, 0);
			if (error) {
				goto nfsmout;
			}

			assert(argsize == arglen);
			if (pad_mbuf) {
				assert(nfsm_pad(arglen) == mbuf_len(pad_mbuf));
				mbuf_free(pad_mbuf);
			} else {
				assert(nfsm_pad(arglen) == 0);
			}

			major = gss_krb5_unwrap_mbuf((uint32_t *)&error, cp->gss_svc_ctx_id, &reply_mbuf, 0, arglen, NULL, NULL);
			if (major != GSS_S_COMPLETE) {
				printf("%s: gss_krb5_unwrap_mbuf failes %d\n", __func__, error);
				goto nfsmout;
			}

			                                                               
			mbuf_setnext(prev_mbuf, reply_mbuf);
			nmc->nmc_mcur = reply_mbuf;
			nmc->nmc_ptr = mbuf_data(reply_mbuf);
			nmc->nmc_left = mbuf_len(reply_mbuf);

			  
                                 
                 
      

			                                             

			  
                                                   
                                                
                      
      
			nfsm_chain_get_32(error, nmc, seqnum);
			if (seqnum != nd->nd_gss_seqnum) {
				printf("%s: Sequence number mismatch seqnum = %d nd->nd_gss_seqnum = %d\n",
				    __func__, seqnum, nd->nd_gss_seqnum);
				printmbuf("reply_mbuf", nmc->nmc_mhead, 0, 0);
				printf("reply_mbuf %p nmc_head %p\n", reply_mbuf, nmc->nmc_mhead);
				error = EBADRPC;                                                 
				goto nfsmout;
			}
			break;
		}
	} else {
		uint32_t verflen;
		  
                                                               
                                    
     
		nfsm_chain_get_32(error, nmc, flavor);
		nfsm_chain_get_32(error, nmc, verflen);
		if (error || flavor != RPCAUTH_NULL || verflen > 0) {
			error = NFSERR_AUTHERR | RPCSEC_GSS_CREDPROBLEM;
		}
		if (error) {
			if (proc == RPCSEC_GSS_INIT) {
				lck_mtx_destroy(&cp->gss_svc_mtx, &nfs_gss_svc_grp);
				FREE(cp, M_TEMP);
				cp = NULL;
			}
			goto nfsmout;
		}
	}

	nd->nd_gss_context = cp;
	return 0;
nfsmout:
	if (cp) {
		nfs_gss_svc_ctx_deref(cp);
	}
	return error;
}

  
                                                          
                                                            
                                
                                                       
   
int
nfs_gss_svc_verf_put(struct nfsrv_descript *nd, struct nfsm_chain *nmc)
{
	struct nfs_gss_svc_ctx *cp;
	int error = 0;
	gss_buffer_desc cksum, seqbuf;
	uint32_t network_seqnum;
	cp = nd->nd_gss_context;
	uint32_t major;

	if (cp->gss_svc_major != GSS_S_COMPLETE) {
		  
                                      
                                 
     
		nfsm_chain_add_32(error, nmc, RPCAUTH_NULL);
		nfsm_chain_add_32(error, nmc, 0);
		return error;
	}

	  
                                              
                                            
                                           
                
    
	seqbuf.length = NFSX_UNSIGNED;
	if (cp->gss_svc_proc == RPCSEC_GSS_INIT ||
	    cp->gss_svc_proc == RPCSEC_GSS_CONTINUE_INIT) {
		network_seqnum = htonl(cp->gss_svc_seqwin);
	} else {
		network_seqnum = htonl(nd->nd_gss_seqnum);
	}
	seqbuf.value = &network_seqnum;

	major = gss_krb5_get_mic((uint32_t *)&error, cp->gss_svc_ctx_id, 0, &seqbuf, &cksum);
	if (major != GSS_S_COMPLETE) {
		return error;
	}

	  
                                  
                              
    
	nfsm_chain_add_32(error, nmc, RPCSEC_GSS);
	nfsm_chain_add_32(error, nmc, cksum.length);
	nfsm_chain_add_opaque(error, nmc, cksum.value, cksum.length);
	gss_release_buffer(NULL, &cksum);

	return error;
}

  
                                                           
                                                          
                                                             
                                                          
                                                                    
                                  
   
int
nfs_gss_svc_prepare_reply(struct nfsrv_descript *nd, struct nfsm_chain *nmc)
{
	struct nfs_gss_svc_ctx *cp = nd->nd_gss_context;
	int error = 0;

	if (cp->gss_svc_proc == RPCSEC_GSS_INIT ||
	    cp->gss_svc_proc == RPCSEC_GSS_CONTINUE_INIT) {
		return 0;
	}

	switch (nd->nd_sec) {
	case RPCAUTH_KRB5:
		                   
		break;
	case RPCAUTH_KRB5I:
	case RPCAUTH_KRB5P:
		nd->nd_gss_mb = nmc->nmc_mcur;                                        
		nfsm_chain_finish_mbuf(error, nmc);                                    
		break;
	}

	return error;
}

  
                                                                    
   
int
nfs_gss_svc_protect_reply(struct nfsrv_descript *nd, mbuf_t mrep __unused)
{
	struct nfs_gss_svc_ctx *cp = nd->nd_gss_context;
	struct nfsm_chain nmrep_res, *nmc_res = &nmrep_res;
	mbuf_t mb, results;
	uint32_t reslen;
	int error = 0;

	      
                                                                     
                                                                      
                                                                    
                                                                   
                                                                      
             
    

	mb = nd->nd_gss_mb;                                                       
	results = mbuf_next(mb);                                                    
	error = mbuf_setnext(mb, NULL);                                         
	if (error) {
		return error;
	}
	nfs_gss_nfsm_chain(nmc_res, mb);                                           
	nfsm_chain_build_done(error, nmc_res);
	if (error) {
		return error;
	}

	if (nd->nd_sec == RPCAUTH_KRB5I) {
		error = rpc_gss_integ_data_create(cp->gss_svc_ctx_id, &results, nd->nd_gss_seqnum, &reslen);
	} else {
		                   
		error = rpc_gss_priv_data_create(cp->gss_svc_ctx_id, &results, nd->nd_gss_seqnum, &reslen);
	}
	nfs_gss_append_chain(nmc_res, results);                            
	nfsm_chain_build_done(error, nmc_res);

	return error;
}

  
                                                                 
                                                               
                                    
                                                            
                                                      
                                                               
                                                               
                                                  
   
int
nfs_gss_svc_ctx_init(struct nfsrv_descript *nd, struct nfsrv_sock *slp, mbuf_t *mrepp)
{
	struct nfs_gss_svc_ctx *cp = NULL;
	int error = 0;
	int autherr = 0;
	struct nfsm_chain *nmreq, nmrep;
	int sz;

	nmreq = &nd->nd_nmreq;
	nfsm_chain_null(&nmrep);
	*mrepp = NULL;
	cp = nd->nd_gss_context;
	nd->nd_repstat = 0;

	switch (cp->gss_svc_proc) {
	case RPCSEC_GSS_INIT:
		nfs_gss_svc_ctx_insert(cp);
		OS_FALLTHROUGH;

	case RPCSEC_GSS_CONTINUE_INIT:
		                                    
		nfsm_chain_get_32(error, nmreq, cp->gss_svc_tokenlen);
		cp->gss_svc_token = NULL;
		if (cp->gss_svc_tokenlen > 0 && cp->gss_svc_tokenlen < GSS_MAX_TOKEN_LEN) {
			MALLOC(cp->gss_svc_token, u_char *, cp->gss_svc_tokenlen, M_TEMP, M_WAITOK);
		}
		if (cp->gss_svc_token == NULL) {
			autherr = RPCSEC_GSS_CREDPROBLEM;
			break;
		}
		nfsm_chain_get_opaque(error, nmreq, cp->gss_svc_tokenlen, cp->gss_svc_token);

		                                                      
		error = nfs_gss_svc_gssd_upcall(cp);
		if (error) {
			autherr = RPCSEC_GSS_CREDPROBLEM;
			if (error == NFSERR_EAUTH) {
				error = 0;
			}
			break;
		}

		  
                                                      
                                          
     
		if (cp->gss_svc_major != GSS_S_COMPLETE) {
			break;
		}

		  
                                        
                  
     
		clock_get_uptime(&cp->gss_svc_incarnation);

		cp->gss_svc_seqwin = GSS_SVC_SEQWINDOW;
		MALLOC(cp->gss_svc_seqbits, uint32_t *,
		    nfsm_rndup((cp->gss_svc_seqwin + 7) / 8), M_TEMP, M_WAITOK | M_ZERO);
		if (cp->gss_svc_seqbits == NULL) {
			autherr = RPCSEC_GSS_CREDPROBLEM;
			break;
		}
		break;

	case RPCSEC_GSS_DATA:
		                                       
		break;

	case RPCSEC_GSS_DESTROY:
		  
                                                  
                                                   
                                               
                                   
     
		cp = nfs_gss_svc_ctx_find(cp->gss_svc_handle);
		if (cp != NULL) {
			cp->gss_svc_handle = 0;                        
			lck_mtx_lock(&cp->gss_svc_mtx);
			clock_interval_to_deadline(GSS_CTX_PEND, NSEC_PER_SEC,
			    &cp->gss_svc_incarnation);
			lck_mtx_unlock(&cp->gss_svc_mtx);
		}
		break;
	default:
		autherr = RPCSEC_GSS_CREDPROBLEM;
		break;
	}

	                          

	if (nd->nd_repstat == 0) {
		nd->nd_repstat = autherr ? (NFSERR_AUTHERR | autherr) : NFSERR_RETVOID;
	}
	sz = 7 * NFSX_UNSIGNED + nfsm_rndup(cp->gss_svc_tokenlen);                   
	error = nfsrv_rephead(nd, slp, &nmrep, sz);
	*mrepp = nmrep.nmc_mhead;
	if (error || autherr) {
		goto nfsmout;
	}

	if (cp->gss_svc_proc == RPCSEC_GSS_INIT ||
	    cp->gss_svc_proc == RPCSEC_GSS_CONTINUE_INIT) {
		nfsm_chain_add_32(error, &nmrep, sizeof(cp->gss_svc_handle));
		nfsm_chain_add_32(error, &nmrep, cp->gss_svc_handle);

		nfsm_chain_add_32(error, &nmrep, cp->gss_svc_major);
		nfsm_chain_add_32(error, &nmrep, cp->gss_svc_minor);
		nfsm_chain_add_32(error, &nmrep, cp->gss_svc_seqwin);

		nfsm_chain_add_32(error, &nmrep, cp->gss_svc_tokenlen);
		if (cp->gss_svc_token != NULL) {
			nfsm_chain_add_opaque(error, &nmrep, cp->gss_svc_token, cp->gss_svc_tokenlen);
			FREE(cp->gss_svc_token, M_TEMP);
			cp->gss_svc_token = NULL;
		}
	}

nfsmout:
	if (autherr != 0) {
		nd->nd_gss_context = NULL;
		LIST_REMOVE(cp, gss_svc_entries);
		if (cp->gss_svc_seqbits != NULL) {
			FREE(cp->gss_svc_seqbits, M_TEMP);
		}
		if (cp->gss_svc_token != NULL) {
			FREE(cp->gss_svc_token, M_TEMP);
		}
		lck_mtx_destroy(&cp->gss_svc_mtx, &nfs_gss_svc_grp);
		FREE(cp, M_TEMP);
	}

	nfsm_chain_build_done(error, &nmrep);
	if (error) {
		nfsm_chain_cleanup(&nmrep);
		*mrepp = NULL;
	}
	return error;
}

  
                                                           
                                                                      
                                                                      
                                      
   
static int
nfs_gss_svc_gssd_upcall(struct nfs_gss_svc_ctx *cp)
{
	kern_return_t kr;
	mach_port_t mp;
	int retry_cnt = 0;
	gssd_byte_buffer octx = NULL;
	uint32_t lucidlen = 0;
	void *lucid_ctx_buffer;
	uint32_t ret_flags;
	vm_map_copy_t itoken = NULL;
	gssd_byte_buffer otoken = NULL;
	mach_msg_type_number_t otokenlen;
	int error = 0;
	char svcname[] = "nfs";

	kr = host_get_gssd_port(host_priv_self(), &mp);
	if (kr != KERN_SUCCESS) {
		printf("nfs_gss_svc_gssd_upcall: can't get gssd port, status %x (%d)\n", kr, kr);
		goto out;
	}
	if (!IPC_PORT_VALID(mp)) {
		printf("nfs_gss_svc_gssd_upcall: gssd port not valid\n");
		goto out;
	}

	if (cp->gss_svc_tokenlen > 0) {
		nfs_gss_mach_alloc_buffer(cp->gss_svc_token, cp->gss_svc_tokenlen, &itoken);
	}

retry:
	printf("Calling mach_gss_accept_sec_context\n");
	kr = mach_gss_accept_sec_context(
		mp,
		(gssd_byte_buffer) itoken, (mach_msg_type_number_t) cp->gss_svc_tokenlen,
		svcname,
		0,
		&cp->gss_svc_context,
		&cp->gss_svc_cred_handle,
		&ret_flags,
		&cp->gss_svc_uid,
		cp->gss_svc_gids,
		&cp->gss_svc_ngroups,
		&octx, (mach_msg_type_number_t *) &lucidlen,
		&otoken, &otokenlen,
		&cp->gss_svc_major,
		&cp->gss_svc_minor);

	printf("mach_gss_accept_sec_context returned %d\n", kr);
	if (kr != KERN_SUCCESS) {
		printf("nfs_gss_svc_gssd_upcall failed: %x (%d)\n", kr, kr);
		if (kr == MIG_SERVER_DIED && cp->gss_svc_context == 0 &&
		    retry_cnt++ < NFS_GSS_MACH_MAX_RETRIES) {
			if (cp->gss_svc_tokenlen > 0) {
				nfs_gss_mach_alloc_buffer(cp->gss_svc_token, cp->gss_svc_tokenlen, &itoken);
			}
			goto retry;
		}
		host_release_special_port(mp);
		goto out;
	}

	host_release_special_port(mp);

	if (lucidlen > 0) {
		if (lucidlen > MAX_LUCIDLEN) {
			printf("nfs_gss_svc_gssd_upcall: bad context length (%d)\n", lucidlen);
			vm_map_copy_discard((vm_map_copy_t) octx);
			vm_map_copy_discard((vm_map_copy_t) otoken);
			goto out;
		}
		MALLOC(lucid_ctx_buffer, void *, lucidlen, M_TEMP, M_WAITOK | M_ZERO);
		error = nfs_gss_mach_vmcopyout((vm_map_copy_t) octx, lucidlen, lucid_ctx_buffer);
		if (error) {
			vm_map_copy_discard((vm_map_copy_t) otoken);
			FREE(lucid_ctx_buffer, M_TEMP);
			goto out;
		}
		if (cp->gss_svc_ctx_id) {
			gss_krb5_destroy_context(cp->gss_svc_ctx_id);
		}
		cp->gss_svc_ctx_id = gss_krb5_make_context(lucid_ctx_buffer, lucidlen);
		if (cp->gss_svc_ctx_id == NULL) {
			printf("Failed to make context from lucid_ctx_buffer\n");
			goto out;
		}
	}

	                                      
	if (cp->gss_svc_token) {
		FREE(cp->gss_svc_token, M_TEMP);
	}
	cp->gss_svc_token = NULL;
	cp->gss_svc_tokenlen = 0;

	if (otokenlen > 0) {
		                                           
		MALLOC(cp->gss_svc_token, u_char *, otokenlen, M_TEMP, M_WAITOK);
		if (cp->gss_svc_token == NULL) {
			printf("nfs_gss_svc_gssd_upcall: could not allocate %d bytes\n", otokenlen);
			vm_map_copy_discard((vm_map_copy_t) otoken);
			return ENOMEM;
		}
		error = nfs_gss_mach_vmcopyout((vm_map_copy_t) otoken, otokenlen, cp->gss_svc_token);
		if (error) {
			FREE(cp->gss_svc_token, M_TEMP);
			cp->gss_svc_token = NULL;
			return NFSERR_EAUTH;
		}
		cp->gss_svc_tokenlen = otokenlen;
	}

	return 0;

out:
	FREE(cp->gss_svc_token, M_TEMP);
	cp->gss_svc_tokenlen = 0;
	cp->gss_svc_token = NULL;

	return NFSERR_EAUTH;
}

  
                                                              
                              
  
                                                              
                                                             
                                                            
                                                             
                           
   
static int
nfs_gss_svc_seqnum_valid(struct nfs_gss_svc_ctx *cp, uint32_t seq)
{
	uint32_t *bits = cp->gss_svc_seqbits;
	uint32_t win = cp->gss_svc_seqwin;
	uint32_t i;

	lck_mtx_lock(&cp->gss_svc_mtx);

	  
                                           
                                        
    
	if (seq > cp->gss_svc_seqmax) {
		if (seq - cp->gss_svc_seqmax > win) {
			bzero(bits, nfsm_rndup((win + 7) / 8));
		} else {
			for (i = cp->gss_svc_seqmax + 1; i < seq; i++) {
				win_resetbit(bits, i % win);
			}
		}
		win_setbit(bits, seq % win);
		cp->gss_svc_seqmax = seq;
		lck_mtx_unlock(&cp->gss_svc_mtx);
		return 1;
	}

	  
                                                  
    
	if (seq <= cp->gss_svc_seqmax - win) {
		lck_mtx_unlock(&cp->gss_svc_mtx);
		return 0;
	}

	  
                                                    
    
	if (win_getbit(bits, seq % win)) {
		lck_mtx_unlock(&cp->gss_svc_mtx);
		return 0;
	}
	win_setbit(bits, seq % win);
	lck_mtx_unlock(&cp->gss_svc_mtx);
	return 1;
}

  
                                
  
                                             
                                               
                                                    
   
void
nfs_gss_svc_ctx_deref(struct nfs_gss_svc_ctx *cp)
{
	lck_mtx_lock(&cp->gss_svc_mtx);
	if (cp->gss_svc_refcnt > 0) {
		cp->gss_svc_refcnt--;
	} else {
		printf("nfs_gss_ctx_deref: zero refcount\n");
	}
	lck_mtx_unlock(&cp->gss_svc_mtx);
}

  
                                                       
   
void
nfs_gss_svc_cleanup(void)
{
	struct nfs_gss_svc_ctx_hashhead *head;
	struct nfs_gss_svc_ctx *cp, *ncp;
	int i;

	lck_mtx_lock(&nfs_gss_svc_ctx_mutex);

	  
                               
    
	for (i = 0; i < SVC_CTX_HASHSZ; i++) {
		  
                                              
     
		head = &nfs_gss_svc_ctx_hashtbl[i];
		LIST_FOREACH_SAFE(cp, head, gss_svc_entries, ncp) {
			LIST_REMOVE(cp, gss_svc_entries);
			if (cp->gss_svc_seqbits) {
				FREE(cp->gss_svc_seqbits, M_TEMP);
			}
			lck_mtx_destroy(&cp->gss_svc_mtx, &nfs_gss_svc_grp);
			FREE(cp, M_TEMP);
		}
	}

	lck_mtx_unlock(&nfs_gss_svc_ctx_mutex);
}

#endif                        


              
                                                              
   

  
                                                                         
                                                          
                                         
   

                                                              
extern void ipc_port_release_send(ipc_port_t);
extern ipc_port_t ipc_port_copy_send(ipc_port_t);

static void
host_release_special_port(mach_port_t mp)
{
	if (IPC_PORT_VALID(mp)) {
		ipc_port_release_send(mp);
	}
}

static mach_port_t
host_copy_special_port(mach_port_t mp)
{
	return ipc_port_copy_send(mp);
}

  
                                                         
                                                         
                                                             
                                                         
                                                       
                                                        
                                                         
                                                          
            
   
static void
nfs_gss_mach_alloc_buffer(u_char *buf, size_t buflen, vm_map_copy_t *addr)
{
	kern_return_t kr;
	vm_offset_t kmem_buf;
	vm_size_t tbuflen;

	*addr = NULL;
	if (buf == NULL || buflen == 0) {
		return;
	}

	tbuflen = vm_map_round_page(buflen,
	    vm_map_page_mask(ipc_kernel_map));

	if (tbuflen < buflen) {
		printf("nfs_gss_mach_alloc_buffer: vm_map_round_page failed\n");
		return;
	}

	kr = vm_allocate_kernel(ipc_kernel_map, &kmem_buf, tbuflen, VM_FLAGS_ANYWHERE, VM_KERN_MEMORY_FILE);
	if (kr != 0) {
		printf("nfs_gss_mach_alloc_buffer: vm_allocate failed\n");
		return;
	}

	kr = vm_map_wire_kernel(ipc_kernel_map,
	    vm_map_trunc_page(kmem_buf,
	    vm_map_page_mask(ipc_kernel_map)),
	    vm_map_round_page(kmem_buf + tbuflen,
	    vm_map_page_mask(ipc_kernel_map)),
	    VM_PROT_READ | VM_PROT_WRITE, VM_KERN_MEMORY_FILE, FALSE);
	if (kr != 0) {
		printf("nfs_gss_mach_alloc_buffer: vm_map_wire failed\n");
		return;
	}

	bcopy(buf, (void *) kmem_buf, buflen);
	                                                                       
	                                              

	kr = vm_map_unwire(ipc_kernel_map,
	    vm_map_trunc_page(kmem_buf,
	    vm_map_page_mask(ipc_kernel_map)),
	    vm_map_round_page(kmem_buf + tbuflen,
	    vm_map_page_mask(ipc_kernel_map)),
	    FALSE);
	if (kr != 0) {
		printf("nfs_gss_mach_alloc_buffer: vm_map_unwire failed\n");
		return;
	}

	kr = vm_map_copyin(ipc_kernel_map, (vm_map_address_t) kmem_buf,
	    (vm_map_size_t) buflen, TRUE, addr);
	if (kr != 0) {
		printf("nfs_gss_mach_alloc_buffer: vm_map_copyin failed\n");
		return;
	}
}

  
                                                               
                                                       
                                                               
                                                            
   
static int
nfs_gss_mach_vmcopyout(vm_map_copy_t in, uint32_t len, u_char *out)
{
	vm_map_offset_t map_data;
	vm_offset_t data;
	int error;

	error = vm_map_copyout(ipc_kernel_map, &map_data, in);
	if (error) {
		return error;
	}

	data = CAST_DOWN(vm_offset_t, map_data);
	bcopy((void *) data, out, len);
	vm_deallocate(ipc_kernel_map, data, len);

	return 0;
}

  
                                               
   
static int
nfs_gss_mchain_length(mbuf_t mhead)
{
	mbuf_t mb;
	int len = 0;

	for (mb = mhead; mb; mb = mbuf_next(mb)) {
		len += mbuf_len(mb);
	}

	return len;
}

  
                                                           
   
static int
nfs_gss_append_chain(struct nfsm_chain *nmc, mbuf_t mc)
{
	int error = 0;
	mbuf_t mb, tail;

	                             
	error = mbuf_setnext(nmc->nmc_mcur, mc);
	if (error) {
		return error;
	}

	                                     
	tail = NULL;
	for (mb = mc; mb; mb = mbuf_next(mb)) {
		tail = mb;
	}

	nmc->nmc_mcur = tail;
	nmc->nmc_ptr = (caddr_t) mbuf_data(tail) + mbuf_len(tail);
	nmc->nmc_left = mbuf_trailingspace(tail);

	return 0;
}

#if CONFIG_NFS_SERVER                                     
  
                                             
   
static void
nfs_gss_nfsm_chain(struct nfsm_chain *nmc, mbuf_t mc)
{
	mbuf_t mb, tail;

	                                     
	tail = NULL;
	for (mb = mc; mb; mb = mbuf_next(mb)) {
		tail = mb;
	}

	nmc->nmc_mhead = mc;
	nmc->nmc_mcur = tail;
	nmc->nmc_ptr = (caddr_t) mbuf_data(tail) + mbuf_len(tail);
	nmc->nmc_left = mbuf_trailingspace(tail);
	nmc->nmc_flags = 0;
}
#endif                        


#if 0
#define DISPLAYLEN 16
#define MAXDISPLAYLEN 256

static void
hexdump(const char *msg, void *data, size_t len)
{
	size_t i, j;
	u_char *d = data;
	char *p, disbuf[3 * DISPLAYLEN + 1];

	printf("NFS DEBUG %s len=%d:\n", msg, (uint32_t)len);
	if (len > MAXDISPLAYLEN) {
		len = MAXDISPLAYLEN;
	}

	for (i = 0; i < len; i += DISPLAYLEN) {
		for (p = disbuf, j = 0; (j + i) < len && j < DISPLAYLEN; j++, p += 3) {
			snprintf(p, 4, "%02x ", d[i + j]);
		}
		printf("\t%s\n", disbuf);
	}
}
#endif

#endif                 
