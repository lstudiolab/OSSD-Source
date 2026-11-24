/*
 * Copyright (c) 2006-2019 Apple Inc. All rights reserved.
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

#include <nfs/nfs_conf.h>
#if CONFIG_NFS_CLIENT

  
                                            
   
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mount_internal.h>
#include <sys/vnode_internal.h>
#include <sys/kpi_mbuf.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/syscall.h>
#include <sys/ubc_internal.h>
#include <sys/fcntl.h>
#include <sys/quota.h>
#include <sys/domain.h>
#include <libkern/OSAtomic.h>
#include <kern/thread_call.h>

#include <sys/vm.h>
#include <sys/vmparam.h>

#include <sys/time.h>
#include <kern/clock.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <nfs/xdr_subs.h>
#include <nfs/nfsm_subs.h>
#include <nfs/nfs_gss.h>
#include <nfs/nfsmount.h>
#include <nfs/nfs_lock.h>

#include <miscfs/specfs/specdev.h>

#include <netinet/in.h>
#include <net/kpi_interface.h>

#if CONFIG_NFS4
  
                                                                    
                                                                       
                                                                     
                                                                   
                                                          
  
                                                                          
                                                                           
                                                                             
                                 
   
#define NFS_MAX_WHO     MAXPATHLEN

  
                                                     
  
                                                                             
  
                                                                         
                                                                   
  
                                                                               
                                                                                    
  
                                                                        
                                                                          
                                                               
  
                                                                                 
                                                                              
                                                                                    
  
                                                                                  
                                                                                  
                                                            
   
int
nfs4_init_clientid(struct nfsmount *nmp)
{
	struct nfs_client_id *ncip, *ncip2;
	struct sockaddr *saddr;
	int error, cmp;
	long len, len2;
	struct vfsstatfs *vsfs;

	static uint8_t en0addr[6];
	static uint8_t en0addr_set = 0;

	lck_mtx_lock(&nfs_global_mutex);
	if (!en0addr_set) {
		ifnet_t interface = NULL;
		error = ifnet_find_by_name("en0", &interface);
		if (!error) {
			error = ifnet_lladdr_copy_bytes(interface, en0addr, sizeof(en0addr));
		}
		if (error) {
			printf("nfs4_init_clientid: error getting en0 address, %d\n", error);
		}
		if (!error) {
			en0addr_set = 1;
		}
		if (interface) {
			ifnet_release(interface);
		}
	}
	lck_mtx_unlock(&nfs_global_mutex);

	MALLOC(ncip, struct nfs_client_id *, sizeof(struct nfs_client_id), M_TEMP, M_WAITOK);
	if (!ncip) {
		return ENOMEM;
	}

	vsfs = vfs_statfs(nmp->nm_mountp);
	saddr = nmp->nm_saddr;
	ncip->nci_idlen = sizeof(uint32_t) + sizeof(en0addr) + saddr->sa_len +
	    strlen(vsfs->f_mntfromname) + 1 + strlen(vsfs->f_mntonname) + 1;
	if (ncip->nci_idlen > NFS4_OPAQUE_LIMIT) {
		ncip->nci_idlen = NFS4_OPAQUE_LIMIT;
	}
	MALLOC(ncip->nci_id, char *, ncip->nci_idlen, M_TEMP, M_WAITOK);
	if (!ncip->nci_id) {
		FREE(ncip, M_TEMP);
		return ENOMEM;
	}

	*(uint32_t*)ncip->nci_id = 0;
	len = sizeof(uint32_t);
	len2 = lmin(sizeof(en0addr), ncip->nci_idlen - len);
	bcopy(en0addr, &ncip->nci_id[len], len2);
	len += sizeof(en0addr);
	len2 = lmin(saddr->sa_len, ncip->nci_idlen - len);
	bcopy(saddr, &ncip->nci_id[len], len2);
	len += len2;
	if (len < ncip->nci_idlen) {
		len2 = strlcpy(&ncip->nci_id[len], vsfs->f_mntfromname, ncip->nci_idlen - len);
		if (len2 < (ncip->nci_idlen - len)) {
			len += len2 + 1;
		} else {
			len = ncip->nci_idlen;
		}
	}
	if (len < ncip->nci_idlen) {
		len2 = strlcpy(&ncip->nci_id[len], vsfs->f_mntonname, ncip->nci_idlen - len);
		if (len2 < (ncip->nci_idlen - len)) {
			len += len2 + 1;
		} else {
			len = ncip->nci_idlen;
		}
	}

	                                                               
	lck_mtx_lock(&nfs_global_mutex);
	TAILQ_FOREACH(ncip2, &nfsclientids, nci_link) {
		if (ncip->nci_idlen > ncip2->nci_idlen) {
			continue;
		}
		if (ncip->nci_idlen < ncip2->nci_idlen) {
			break;
		}
		cmp = bcmp(ncip->nci_id + sizeof(uint32_t),
		    ncip2->nci_id + sizeof(uint32_t),
		    ncip->nci_idlen - sizeof(uint32_t));
		if (cmp > 0) {
			continue;
		}
		if (cmp < 0) {
			break;
		}
		if (*(uint32_t*)ncip->nci_id > *(uint32_t*)ncip2->nci_id) {
			continue;
		}
		if (*(uint32_t*)ncip->nci_id < *(uint32_t*)ncip2->nci_id) {
			break;
		}
		*(uint32_t*)ncip->nci_id += 1;
	}
	if (*(uint32_t*)ncip->nci_id) {
		printf("nfs client ID collision (%d) for %s on %s\n", *(uint32_t*)ncip->nci_id,
		    vsfs->f_mntfromname, vsfs->f_mntonname);
	}
	if (ncip2) {
		TAILQ_INSERT_BEFORE(ncip2, ncip, nci_link);
	} else {
		TAILQ_INSERT_TAIL(&nfsclientids, ncip, nci_link);
	}
	nmp->nm_longid = ncip;
	lck_mtx_unlock(&nfs_global_mutex);

	return 0;
}

  
                    
   
int
nfs4_setclientid(struct nfsmount *nmp)
{
	uint64_t verifier, xid;
	int error = 0, status, numops;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN];
	thread_t thd;
	kauth_cred_t cred;
	struct nfsm_chain nmreq, nmrep;
	struct sockaddr_storage ss;
	void *sinaddr = NULL;
	char raddr[MAX_IPv6_STR_LEN];
	char uaddr[MAX_IPv6_STR_LEN + 16];
	int ualen = 0;
	in_port_t port = 0;

	thd = current_thread();
	cred = IS_VALID_CRED(nmp->nm_mcred) ? nmp->nm_mcred : vfs_context_ucred(vfs_context_kernel());
	kauth_cred_ref(cred);

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	if (!nmp->nm_longid) {
		error = nfs4_init_clientid(nmp);
	}

	              
	numops = 1;
	nfsm_chain_build_alloc_init(error, &nmreq, 14 * NFSX_UNSIGNED + nmp->nm_longid->nci_idlen);
	nfsm_chain_add_compound_header(error, &nmreq, "setclid", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_SETCLIENTID);
	                             
	nfsm_chain_add_64(error, &nmreq, nmp->nm_mounttime);
	nfsm_chain_add_32(error, &nmreq, nmp->nm_longid->nci_idlen);
	nfsm_chain_add_opaque(error, &nmreq, nmp->nm_longid->nci_id, nmp->nm_longid->nci_idlen);
	nfsmout_if(error);
	                               
	if (!NMFLAG(nmp, NOCALLBACK) && nmp->nm_cbid && nfs4_cb_port &&
	    !sock_getsockname(nmp->nm_nso->nso_so, (struct sockaddr*)&ss, sizeof(ss))) {
		if (ss.ss_family == AF_INET) {
			sinaddr = &((struct sockaddr_in*)&ss)->sin_addr;
			port = nfs4_cb_port;
		} else if (ss.ss_family == AF_INET6) {
			sinaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
			port = nfs4_cb_port6;
		} else {
			error = EINVAL;
			nfsmout_if(error);
		}
		if (sinaddr && port && (inet_ntop(ss.ss_family, sinaddr, raddr, sizeof(raddr)) == raddr)) {
			                                                                                     
			ualen = snprintf(uaddr, sizeof(uaddr), "%s.%d.%d", raddr,
			    ((port >> 8) & 0xff),
			    (port & 0xff));
			                                            
			if (ualen >= (int)sizeof(uaddr)) {
				ualen = 0;
			}
		}
	}
	if (ualen > 0) {
		                       
		nfsm_chain_add_32(error, &nmreq, NFS4_CALLBACK_PROG);                       
		if (ss.ss_family == AF_INET) {
			nfsm_chain_add_string(error, &nmreq, "tcp", 3);                       
		} else if (ss.ss_family == AF_INET6) {
			nfsm_chain_add_string(error, &nmreq, "tcp6", 4);                       
		}
		nfsm_chain_add_string(error, &nmreq, uaddr, ualen);                      
		nfsm_chain_add_32(error, &nmreq, nmp->nm_cbid);                     
	} else {
		                                       
		nfsm_chain_add_32(error, &nmreq, 0);                       
		nfsm_chain_add_string(error, &nmreq, "", 0);                       
		nfsm_chain_add_string(error, &nmreq, "", 0);                      
		nfsm_chain_add_32(error, &nmreq, 0);                     
	}
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(NULL, nmp->nm_mountp, &nmreq, NFSPROC4_COMPOUND, thd, cred, NULL, R_SETUP, &nmrep, &xid, &status);
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	if (!error && (numops != 1) && status) {
		error = status;
	}
	nfsm_chain_op_check(error, &nmrep, NFS_OP_SETCLIENTID);
	if (error == NFSERR_CLID_INUSE) {
		printf("nfs4_setclientid: client ID in use?\n");
	}
	nfsmout_if(error);
	nfsm_chain_get_64(error, &nmrep, nmp->nm_clientid);
	nfsm_chain_get_64(error, &nmrep, verifier);
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	                      
	numops = 1;
	nfsm_chain_build_alloc_init(error, &nmreq, 15 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "setclid_conf", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_SETCLIENTID_CONFIRM);
	nfsm_chain_add_64(error, &nmreq, nmp->nm_clientid);
	nfsm_chain_add_64(error, &nmreq, verifier);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(NULL, nmp->nm_mountp, &nmreq, NFSPROC4_COMPOUND, thd, cred, NULL, R_SETUP, &nmrep, &xid, &status);
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_SETCLIENTID_CONFIRM);
	if (error) {
		printf("nfs4_setclientid: confirm error %d\n", error);
	}
	lck_mtx_lock(&nmp->nm_lock);
	if (!error) {
		nmp->nm_state |= NFSSTA_CLIENTID;
	}
	lck_mtx_unlock(&nmp->nm_lock);

	nfsmout_if(error || !nmp->nm_dnp);

	                                                       
	                     
	numops = 2;
	nfsm_chain_build_alloc_init(error, &nmreq, 23 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "setclid_attr", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nmp->nm_vers, nmp->nm_dnp->n_fhp, nmp->nm_dnp->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_CLEAR_ATTRIBUTES(bitmap);
	NFS4_PER_FS_ATTRIBUTES(bitmap);
	nfsm_chain_add_bitmap(error, &nmreq, bitmap, NFS_ATTR_BITMAP_LEN);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(NULL, nmp->nm_mountp, &nmreq, NFSPROC4_COMPOUND, thd, cred, NULL, R_SETUP, &nmrep, &xid, &status);
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	lck_mtx_lock(&nmp->nm_lock);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	if (!error) {
		error = nfs4_parsefattr(&nmrep, &nmp->nm_fsattr, NULL, NULL, NULL, NULL);
	}
	lck_mtx_unlock(&nmp->nm_lock);
	if (error) {                                        
		error = 0;
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	kauth_cred_unref(&cred);
	if (error) {
		printf("nfs4_setclientid failed, %d\n", error);
	}
	return error;
}

  
                                    
   
int
nfs4_renew(struct nfsmount *nmp, int rpcflag)
{
	int error = 0, status, numops;
	u_int64_t xid;
	struct nfsm_chain nmreq, nmrep;
	kauth_cred_t cred;

	cred = IS_VALID_CRED(nmp->nm_mcred) ? nmp->nm_mcred : vfs_context_ucred(vfs_context_kernel());
	kauth_cred_ref(cred);

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	        
	numops = 1;
	nfsm_chain_build_alloc_init(error, &nmreq, 8 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "renew", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_RENEW);
	nfsm_chain_add_64(error, &nmreq, nmp->nm_clientid);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(NULL, nmp->nm_mountp, &nmreq, NFSPROC4_COMPOUND,
	    current_thread(), cred, NULL, rpcflag, &nmrep, &xid, &status);
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_RENEW);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	kauth_cred_unref(&cred);
	return error;
}


  
                                                
   
void
nfs4_renew_timer(void *param0, __unused void *param1)
{
	struct nfsmount *nmp = param0;
	u_int64_t clientid;
	int error = 0, interval;

	lck_mtx_lock(&nmp->nm_lock);
	clientid = nmp->nm_clientid;
	if ((nmp->nm_state & NFSSTA_RECOVER) || !(nmp->nm_sockflags & NMSOCK_READY)) {
		lck_mtx_unlock(&nmp->nm_lock);
		goto out;
	}
	lck_mtx_unlock(&nmp->nm_lock);

	error = nfs4_renew(nmp, R_RECOVER);
out:
	if (error == ETIMEDOUT) {
		nfs_need_reconnect(nmp);
	} else if (error) {
		printf("nfs4_renew_timer: error %d\n", error);
	}
	lck_mtx_lock(&nmp->nm_lock);
	if (error && (error != ETIMEDOUT) &&
	    (nmp->nm_clientid == clientid) && !(nmp->nm_state & NFSSTA_RECOVER)) {
		printf("nfs4_renew_timer: error %d, initiating recovery\n", error);
		nfs_need_recover(nmp, error);
	}

	interval = nmp->nm_fsattr.nfsa_lease / (error ? 4 : 2);
	if ((interval < 1) || (nmp->nm_state & NFSSTA_RECOVER)) {
		interval = 1;
	}
	nfs_interval_timer_start(nmp->nm_renew_timer, interval * 1000);
	lck_mtx_unlock(&nmp->nm_lock);
}

  
                                             
  
                                                     
  
                      
                      
                                            
                                                 
                                                                 
                                                                                       
   
int
nfs4_secinfo_rpc(struct nfsmount *nmp, struct nfsreq_secinfo_args *siap, kauth_cred_t cred, uint32_t *sec, int *seccountp)
{
	int error = 0, status, nfsvers, numops, fhsize;
	vnode_t dvp = NULLVP;
	nfsnode_t np, dnp;
	u_char *fhp;
	const char *vname = NULL, *name;
	uint64_t xid;
	size_t namelen;
	struct nfsm_chain nmreq, nmrep;

	*seccountp = 0;
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	np = siap->rsia_np;

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	fhp = siap->rsia_fh;
	fhsize = fhp ? siap->rsia_fhsize : 0;
	name = siap->rsia_name;
	namelen = name ? siap->rsia_namelen : 0;
	if (name && !namelen) {
		namelen = strlen(name);
	}
	if (!fhp && name) {
		if (!np) {                    
			goto gotargs;
		}
		fhp = np->n_fhp;
		fhsize = np->n_fhsize;
	}
	if (fhp && name) {
		goto gotargs;
	}

	if (!np) {
		return EIO;
	}
	nfs_node_lock_force(np);
	if ((vnode_vtype(NFSTOV(np)) != VDIR) && np->n_sillyrename) {
		  
                                                    
                                                   
     
		struct nfs_sillyrename *nsp = np->n_sillyrename;
		dnp = nsp->nsr_dnp;
		dvp = NFSTOV(dnp);
		if ((error = vnode_get(dvp))) {
			dvp = NULLVP;
			nfs_node_unlock(np);
			goto nfsmout;
		}
		fhp = dnp->n_fhp;
		fhsize = dnp->n_fhsize;
		name = nsp->nsr_name;
		namelen = nsp->nsr_namlen;
	} else {
		  
                                                                
                                                                  
                                                                 
                                            
     
		if ((np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR) &&
		    (((dvp = np->n_parent)) && (error = vnode_get(dvp)))) {
			dvp = NULL;
		}
		if (!dvp) {
			dvp = vnode_getparent(NFSTOV(np));
		}
		vname = vnode_getname(NFSTOV(np));
		if (!dvp || !vname) {
			if (!error) {
				error = EIO;
			}
			nfs_node_unlock(np);
			goto nfsmout;
		}
		dnp = VTONFS(dvp);
		fhp = dnp->n_fhp;
		fhsize = dnp->n_fhsize;
		name = vname;
		namelen = strnlen(vname, MAXPATHLEN);
	}
	nfs_node_unlock(np);

gotargs:
	                        
	numops = 2;
	nfsm_chain_build_alloc_init(error, &nmreq,
	    4 * NFSX_UNSIGNED + NFSX_FH(nfsvers) + nfsm_rndup(namelen));
	nfsm_chain_add_compound_header(error, &nmreq, "secinfo", nmp->nm_minor_vers, numops);
	numops--;
	if (fhp) {
		nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
		nfsm_chain_add_fh(error, &nmreq, nfsvers, fhp, fhsize);
	} else {
		nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTROOTFH);
	}
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_SECINFO);
	nfsm_chain_add_name(error, &nmreq, name, namelen, nmp);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(np, nmp->nm_mountp, &nmreq, NFSPROC4_COMPOUND,
	    current_thread(), cred, NULL, 0, &nmrep, &xid, &status);
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, fhp ? NFS_OP_PUTFH : NFS_OP_PUTROOTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_SECINFO);
	nfsmout_if(error);
	error = nfsm_chain_get_secinfo(&nmrep, sec, seccountp);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	if (vname) {
		vnode_putname(vname);
	}
	if (dvp != NULLVP) {
		vnode_put(dvp);
	}
	return error;
}
#endif                  

  
                                                              
                                                  
   
int
nfsm_chain_get_secinfo(struct nfsm_chain *nmc, uint32_t *sec, int *seccountp)
{
	int error = 0, secmax, seccount, srvcount;
	uint32_t flavor;

#if CONFIG_NFS_GSS
	uint32_t val;
	u_char oid[12];
#endif

	seccount = srvcount = 0;
	secmax = *seccountp;
	*seccountp = 0;

	nfsm_chain_get_32(error, nmc, srvcount);
	while (!error && (srvcount > 0) && (seccount < secmax)) {
		nfsm_chain_get_32(error, nmc, flavor);
		nfsmout_if(error);
		switch (flavor) {
		case RPCAUTH_NONE:
		case RPCAUTH_SYS:
#if CONFIG_NFS_GSS
		case RPCAUTH_KRB5:
		case RPCAUTH_KRB5I:
		case RPCAUTH_KRB5P:
#endif                     
			sec[seccount++] = flavor;
			break;
#if CONFIG_NFS_GSS
		case RPCSEC_GSS:
			                                          
			nfsm_chain_get_32(error, nmc, val);                 
			nfsmout_if(error);
			if (val != sizeof(krb5_mech_oid)) {
				nfsm_chain_adv(error, nmc, val);
				nfsm_chain_adv(error, nmc, 2 * NFSX_UNSIGNED);
				break;
			}
			nfsm_chain_get_opaque(error, nmc, val, oid);                
			nfsmout_if(error);
			if (bcmp(oid, krb5_mech_oid, sizeof(krb5_mech_oid))) {
				nfsm_chain_adv(error, nmc, 2 * NFSX_UNSIGNED);
				break;
			}
			nfsm_chain_get_32(error, nmc, val);          
			nfsm_chain_get_32(error, nmc, val);              
			nfsmout_if(error);
			switch (val) {
			case RPCSEC_GSS_SVC_NONE:
				sec[seccount++] = RPCAUTH_KRB5;
				break;
			case RPCSEC_GSS_SVC_INTEGRITY:
				sec[seccount++] = RPCAUTH_KRB5I;
				break;
			case RPCSEC_GSS_SVC_PRIVACY:
				sec[seccount++] = RPCAUTH_KRB5P;
				break;
			}
			break;
#endif                     
		}
		srvcount--;
	}
nfsmout:
	if (!error) {
		*seccountp = seccount;
	}
	return error;
}

#if CONFIG_NFS4
  
                                                                         
   
int
nfs4_get_fs_locations(
	struct nfsmount *nmp,
	nfsnode_t dnp,
	u_char *fhp,
	int fhsize,
	const char *name,
	vfs_context_t ctx,
	struct nfs_fs_locations *nfslsp)
{
	int error = 0, numops, status;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN];
	struct nfsreq *req;
	struct nfsreq_secinfo_args si;
	struct nfsm_chain nmreq, nmrep;
	uint64_t xid;

	if (!fhp && dnp) {
		fhp = dnp->n_fhp;
		fhsize = dnp->n_fhsize;
	}
	if (!fhp) {
		return EINVAL;
	}

	req = zalloc(nfs_req_zone);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	NFSREQ_SECINFO_SET(&si, NULL, fhp, fhsize, name, 0);
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 18 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "fs_locations", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, NFS_VER4, fhp, fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_LOOKUP);
	nfsm_chain_add_name(error, &nmreq, name, strlen(name), nmp);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_CLEAR_ATTRIBUTES(bitmap);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_FS_LOCATIONS);
	nfsm_chain_add_bitmap(error, &nmreq, bitmap, NFS_ATTR_BITMAP_LEN);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request_async(dnp, nmp->nm_mountp, &nmreq, NFSPROC4_COMPOUND,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), &si, 0, NULL, &req);
	if (!error) {
		error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_LOOKUP);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsmout_if(error);
	error = nfs4_parsefattr(&nmrep, NULL, NULL, NULL, NULL, nfslsp);
nfsmout:
	NFS_ZFREE(nfs_req_zone, req);
	nfsm_chain_cleanup(&nmrep);
	nfsm_chain_cleanup(&nmreq);
	return error;
}

  
                                                                      
                                               
   
void
nfs4_default_attrs_for_referral_trigger(
	nfsnode_t dnp,
	char *name,
	int namelen,
	struct nfs_vattr *nvap,
	fhandle_t *fhp)
{
	struct timespec now;
	nanotime(&now);
	int len;

	nvap->nva_flags = NFS_FFLAG_TRIGGER | NFS_FFLAG_TRIGGER_REFERRAL;
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_TYPE)) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_TYPE);
		nvap->nva_type = VDIR;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_FSID)) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_FSID);
		nvap->nva_fsid.major = 0;
		nvap->nva_fsid.minor = 0;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_OWNER) && dnp) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_OWNER);
		nvap->nva_uid = dnp->n_vattr.nva_uid;
		nvap->nva_uuuid = dnp->n_vattr.nva_uuuid;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_OWNER_GROUP) && dnp) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_OWNER_GROUP);
		nvap->nva_gid = dnp->n_vattr.nva_gid;
		nvap->nva_guuid = dnp->n_vattr.nva_guuid;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_MODE)) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_MODE);
		nvap->nva_mode = 0777;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_SIZE)) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_SIZE);
		nvap->nva_size = 0;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_SPACE_USED)) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_SPACE_USED);
		nvap->nva_bytes = 0;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_NUMLINKS)) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_NUMLINKS);
		nvap->nva_nlink = 2;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_TIME_ACCESS)) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_TIME_ACCESS);
		nvap->nva_timesec[NFSTIME_ACCESS] = now.tv_sec;
		nvap->nva_timensec[NFSTIME_ACCESS] = now.tv_nsec;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_TIME_MODIFY)) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_TIME_MODIFY);
		nvap->nva_timesec[NFSTIME_MODIFY] = now.tv_sec;
		nvap->nva_timensec[NFSTIME_MODIFY] = now.tv_nsec;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_TIME_METADATA)) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_TIME_METADATA);
		nvap->nva_timesec[NFSTIME_CHANGE] = now.tv_sec;
		nvap->nva_timensec[NFSTIME_CHANGE] = now.tv_nsec;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_FILEID)) {
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_FILEID);
		nvap->nva_fileid = 42;
	}
	if (!NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_FILEHANDLE) && dnp && name && fhp) {
		                                                                     
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_FILEHANDLE);
		bcopy(&dnp, &fhp->fh_data[0], sizeof(dnp));
		len = sizeof(fhp->fh_data) - sizeof(dnp);
		bcopy(name, &fhp->fh_data[0] + sizeof(dnp), MIN(len, namelen));
		fhp->fh_len = sizeof(dnp) + namelen;
		if (fhp->fh_len > (int)sizeof(fhp->fh_data)) {
			fhp->fh_len = sizeof(fhp->fh_data);
		}
	}
}

  
                                                                                      
   
void
nfs_vattr_set_bitmap(struct nfsmount *nmp, uint32_t *bitmap, struct vnode_attr *vap)
{
	int i;

	NFS_CLEAR_ATTRIBUTES(bitmap);
	if (VATTR_IS_ACTIVE(vap, va_data_size)) {
		NFS_BITMAP_SET(bitmap, NFS_FATTR_SIZE);
	}
	if (VATTR_IS_ACTIVE(vap, va_acl) && (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_ACL)) {
		NFS_BITMAP_SET(bitmap, NFS_FATTR_ACL);
	}
	if (VATTR_IS_ACTIVE(vap, va_flags)) {
		NFS_BITMAP_SET(bitmap, NFS_FATTR_ARCHIVE);
		NFS_BITMAP_SET(bitmap, NFS_FATTR_HIDDEN);
	}
	                                             
	if (VATTR_IS_ACTIVE(vap, va_mode) && !NMFLAG(nmp, ACLONLY)) {
		NFS_BITMAP_SET(bitmap, NFS_FATTR_MODE);
	}
	if (VATTR_IS_ACTIVE(vap, va_uid) || VATTR_IS_ACTIVE(vap, va_uuuid)) {
		NFS_BITMAP_SET(bitmap, NFS_FATTR_OWNER);
	}
	if (VATTR_IS_ACTIVE(vap, va_gid) || VATTR_IS_ACTIVE(vap, va_guuid)) {
		NFS_BITMAP_SET(bitmap, NFS_FATTR_OWNER_GROUP);
	}
	                                           
	if (vap->va_vaflags & VA_UTIMES_NULL) {
		NFS_BITMAP_SET(bitmap, NFS_FATTR_TIME_ACCESS_SET);
		NFS_BITMAP_SET(bitmap, NFS_FATTR_TIME_MODIFY_SET);
	} else {
		if (VATTR_IS_ACTIVE(vap, va_access_time)) {
			NFS_BITMAP_SET(bitmap, NFS_FATTR_TIME_ACCESS_SET);
		}
		if (VATTR_IS_ACTIVE(vap, va_modify_time)) {
			NFS_BITMAP_SET(bitmap, NFS_FATTR_TIME_MODIFY_SET);
		}
	}
	if (VATTR_IS_ACTIVE(vap, va_backup_time)) {
		NFS_BITMAP_SET(bitmap, NFS_FATTR_TIME_BACKUP);
	}
	if (VATTR_IS_ACTIVE(vap, va_create_time)) {
		NFS_BITMAP_SET(bitmap, NFS_FATTR_TIME_CREATE);
	}
	                                              
	for (i = 0; i < NFS_ATTR_BITMAP_LEN; i++) {
		bitmap[i] &= nmp->nm_fsattr.nfsa_supp_attr[i];
	}
}

  
                                          
   
uint32_t
nfs4_ace_nfstype_to_vfstype(uint32_t nfsacetype, int *errorp)
{
	switch (nfsacetype) {
	case NFS_ACE_ACCESS_ALLOWED_ACE_TYPE:
		return KAUTH_ACE_PERMIT;
	case NFS_ACE_ACCESS_DENIED_ACE_TYPE:
		return KAUTH_ACE_DENY;
	case NFS_ACE_SYSTEM_AUDIT_ACE_TYPE:
		return KAUTH_ACE_AUDIT;
	case NFS_ACE_SYSTEM_ALARM_ACE_TYPE:
		return KAUTH_ACE_ALARM;
	}
	*errorp = EBADRPC;
	return 0;
}

uint32_t
nfs4_ace_vfstype_to_nfstype(uint32_t vfstype, int *errorp)
{
	switch (vfstype) {
	case KAUTH_ACE_PERMIT:
		return NFS_ACE_ACCESS_ALLOWED_ACE_TYPE;
	case KAUTH_ACE_DENY:
		return NFS_ACE_ACCESS_DENIED_ACE_TYPE;
	case KAUTH_ACE_AUDIT:
		return NFS_ACE_SYSTEM_AUDIT_ACE_TYPE;
	case KAUTH_ACE_ALARM:
		return NFS_ACE_SYSTEM_ALARM_ACE_TYPE;
	}
	*errorp = EINVAL;
	return 0;
}

  
                                          
   
uint32_t
nfs4_ace_nfsflags_to_vfsflags(uint32_t nfsflags)
{
	uint32_t vfsflags = 0;

	if (nfsflags & NFS_ACE_FILE_INHERIT_ACE) {
		vfsflags |= KAUTH_ACE_FILE_INHERIT;
	}
	if (nfsflags & NFS_ACE_DIRECTORY_INHERIT_ACE) {
		vfsflags |= KAUTH_ACE_DIRECTORY_INHERIT;
	}
	if (nfsflags & NFS_ACE_NO_PROPAGATE_INHERIT_ACE) {
		vfsflags |= KAUTH_ACE_LIMIT_INHERIT;
	}
	if (nfsflags & NFS_ACE_INHERIT_ONLY_ACE) {
		vfsflags |= KAUTH_ACE_ONLY_INHERIT;
	}
	if (nfsflags & NFS_ACE_SUCCESSFUL_ACCESS_ACE_FLAG) {
		vfsflags |= KAUTH_ACE_SUCCESS;
	}
	if (nfsflags & NFS_ACE_FAILED_ACCESS_ACE_FLAG) {
		vfsflags |= KAUTH_ACE_FAILURE;
	}
	if (nfsflags & NFS_ACE_INHERITED_ACE) {
		vfsflags |= KAUTH_ACE_INHERITED;
	}

	return vfsflags;
}

uint32_t
nfs4_ace_vfsflags_to_nfsflags(uint32_t vfsflags)
{
	uint32_t nfsflags = 0;

	if (vfsflags & KAUTH_ACE_FILE_INHERIT) {
		nfsflags |= NFS_ACE_FILE_INHERIT_ACE;
	}
	if (vfsflags & KAUTH_ACE_DIRECTORY_INHERIT) {
		nfsflags |= NFS_ACE_DIRECTORY_INHERIT_ACE;
	}
	if (vfsflags & KAUTH_ACE_LIMIT_INHERIT) {
		nfsflags |= NFS_ACE_NO_PROPAGATE_INHERIT_ACE;
	}
	if (vfsflags & KAUTH_ACE_ONLY_INHERIT) {
		nfsflags |= NFS_ACE_INHERIT_ONLY_ACE;
	}
	if (vfsflags & KAUTH_ACE_SUCCESS) {
		nfsflags |= NFS_ACE_SUCCESSFUL_ACCESS_ACE_FLAG;
	}
	if (vfsflags & KAUTH_ACE_FAILURE) {
		nfsflags |= NFS_ACE_FAILED_ACCESS_ACE_FLAG;
	}
	if (vfsflags & KAUTH_ACE_INHERITED) {
		nfsflags |= NFS_ACE_INHERITED_ACE;
	}

	return nfsflags;
}

  
                                                               
   
uint32_t
nfs4_ace_nfsmask_to_vfsrights(uint32_t nfsmask)
{
	uint32_t vfsrights = 0;

	if (nfsmask & NFS_ACE_READ_DATA) {
		vfsrights |= KAUTH_VNODE_READ_DATA;
	}
	if (nfsmask & NFS_ACE_LIST_DIRECTORY) {
		vfsrights |= KAUTH_VNODE_LIST_DIRECTORY;
	}
	if (nfsmask & NFS_ACE_WRITE_DATA) {
		vfsrights |= KAUTH_VNODE_WRITE_DATA;
	}
	if (nfsmask & NFS_ACE_ADD_FILE) {
		vfsrights |= KAUTH_VNODE_ADD_FILE;
	}
	if (nfsmask & NFS_ACE_APPEND_DATA) {
		vfsrights |= KAUTH_VNODE_APPEND_DATA;
	}
	if (nfsmask & NFS_ACE_ADD_SUBDIRECTORY) {
		vfsrights |= KAUTH_VNODE_ADD_SUBDIRECTORY;
	}
	if (nfsmask & NFS_ACE_READ_NAMED_ATTRS) {
		vfsrights |= KAUTH_VNODE_READ_EXTATTRIBUTES;
	}
	if (nfsmask & NFS_ACE_WRITE_NAMED_ATTRS) {
		vfsrights |= KAUTH_VNODE_WRITE_EXTATTRIBUTES;
	}
	if (nfsmask & NFS_ACE_EXECUTE) {
		vfsrights |= KAUTH_VNODE_EXECUTE;
	}
	if (nfsmask & NFS_ACE_DELETE_CHILD) {
		vfsrights |= KAUTH_VNODE_DELETE_CHILD;
	}
	if (nfsmask & NFS_ACE_READ_ATTRIBUTES) {
		vfsrights |= KAUTH_VNODE_READ_ATTRIBUTES;
	}
	if (nfsmask & NFS_ACE_WRITE_ATTRIBUTES) {
		vfsrights |= KAUTH_VNODE_WRITE_ATTRIBUTES;
	}
	if (nfsmask & NFS_ACE_DELETE) {
		vfsrights |= KAUTH_VNODE_DELETE;
	}
	if (nfsmask & NFS_ACE_READ_ACL) {
		vfsrights |= KAUTH_VNODE_READ_SECURITY;
	}
	if (nfsmask & NFS_ACE_WRITE_ACL) {
		vfsrights |= KAUTH_VNODE_WRITE_SECURITY;
	}
	if (nfsmask & NFS_ACE_WRITE_OWNER) {
		vfsrights |= KAUTH_VNODE_CHANGE_OWNER;
	}
	if (nfsmask & NFS_ACE_SYNCHRONIZE) {
		vfsrights |= KAUTH_VNODE_SYNCHRONIZE;
	}
	if ((nfsmask & NFS_ACE_GENERIC_READ) == NFS_ACE_GENERIC_READ) {
		vfsrights |= KAUTH_ACE_GENERIC_READ;
	}
	if ((nfsmask & NFS_ACE_GENERIC_WRITE) == NFS_ACE_GENERIC_WRITE) {
		vfsrights |= KAUTH_ACE_GENERIC_WRITE;
	}
	if ((nfsmask & NFS_ACE_GENERIC_EXECUTE) == NFS_ACE_GENERIC_EXECUTE) {
		vfsrights |= KAUTH_ACE_GENERIC_EXECUTE;
	}

	return vfsrights;
}

uint32_t
nfs4_ace_vfsrights_to_nfsmask(uint32_t vfsrights)
{
	uint32_t nfsmask = 0;

	if (vfsrights & KAUTH_VNODE_READ_DATA) {
		nfsmask |= NFS_ACE_READ_DATA;
	}
	if (vfsrights & KAUTH_VNODE_LIST_DIRECTORY) {
		nfsmask |= NFS_ACE_LIST_DIRECTORY;
	}
	if (vfsrights & KAUTH_VNODE_WRITE_DATA) {
		nfsmask |= NFS_ACE_WRITE_DATA;
	}
	if (vfsrights & KAUTH_VNODE_ADD_FILE) {
		nfsmask |= NFS_ACE_ADD_FILE;
	}
	if (vfsrights & KAUTH_VNODE_APPEND_DATA) {
		nfsmask |= NFS_ACE_APPEND_DATA;
	}
	if (vfsrights & KAUTH_VNODE_ADD_SUBDIRECTORY) {
		nfsmask |= NFS_ACE_ADD_SUBDIRECTORY;
	}
	if (vfsrights & KAUTH_VNODE_READ_EXTATTRIBUTES) {
		nfsmask |= NFS_ACE_READ_NAMED_ATTRS;
	}
	if (vfsrights & KAUTH_VNODE_WRITE_EXTATTRIBUTES) {
		nfsmask |= NFS_ACE_WRITE_NAMED_ATTRS;
	}
	if (vfsrights & KAUTH_VNODE_EXECUTE) {
		nfsmask |= NFS_ACE_EXECUTE;
	}
	if (vfsrights & KAUTH_VNODE_DELETE_CHILD) {
		nfsmask |= NFS_ACE_DELETE_CHILD;
	}
	if (vfsrights & KAUTH_VNODE_READ_ATTRIBUTES) {
		nfsmask |= NFS_ACE_READ_ATTRIBUTES;
	}
	if (vfsrights & KAUTH_VNODE_WRITE_ATTRIBUTES) {
		nfsmask |= NFS_ACE_WRITE_ATTRIBUTES;
	}
	if (vfsrights & KAUTH_VNODE_DELETE) {
		nfsmask |= NFS_ACE_DELETE;
	}
	if (vfsrights & KAUTH_VNODE_READ_SECURITY) {
		nfsmask |= NFS_ACE_READ_ACL;
	}
	if (vfsrights & KAUTH_VNODE_WRITE_SECURITY) {
		nfsmask |= NFS_ACE_WRITE_ACL;
	}
	if (vfsrights & KAUTH_VNODE_CHANGE_OWNER) {
		nfsmask |= NFS_ACE_WRITE_OWNER;
	}
	if (vfsrights & KAUTH_VNODE_SYNCHRONIZE) {
		nfsmask |= NFS_ACE_SYNCHRONIZE;
	}
	if (vfsrights & KAUTH_ACE_GENERIC_READ) {
		nfsmask |= NFS_ACE_GENERIC_READ;
	}
	if (vfsrights & KAUTH_ACE_GENERIC_WRITE) {
		nfsmask |= NFS_ACE_GENERIC_WRITE;
	}
	if (vfsrights & KAUTH_ACE_GENERIC_EXECUTE) {
		nfsmask |= NFS_ACE_GENERIC_EXECUTE;
	}
	if (vfsrights & KAUTH_ACE_GENERIC_ALL) {
		nfsmask |= (KAUTH_ACE_GENERIC_READ | KAUTH_ACE_GENERIC_WRITE | NFS_ACE_GENERIC_EXECUTE);
	}

	return nfsmask;
}

  
                   
                                       
                                                                                               
   
static int
nfs4_wkid2sid(const char *id, ntsid_t *sp)
{
	size_t len = strnlen(id, MAXIDNAMELEN);

	if (len == MAXIDNAMELEN || id[len - 1] != '@') {
		return EINVAL;
	}

	bzero(sp, sizeof(ntsid_t));
	sp->sid_kind = 1;
	sp->sid_authcount = 1;
	if (!strcmp(id, "OWNER@")) {
		          
		sp->sid_authority[5] = 3;
		sp->sid_authorities[0] = 0;
	} else if (!strcmp(id, "GROUP@")) {
		          
		sp->sid_authority[5] = 3;
		sp->sid_authorities[0] = 1;
	} else if (!strcmp(id, "EVERYONE@")) {
		          
		sp->sid_authority[5] = 1;
		sp->sid_authorities[0] = 0;
	} else if (!strcmp(id, "INTERACTIVE@")) {
		          
		sp->sid_authority[5] = 5;
		sp->sid_authorities[0] = 4;
	} else if (!strcmp(id, "NETWORK@")) {
		          
		sp->sid_authority[5] = 5;
		sp->sid_authorities[0] = 2;
	} else if (!strcmp(id, "DIALUP@")) {
		          
		sp->sid_authority[5] = 5;
		sp->sid_authorities[0] = 1;
	} else if (!strcmp(id, "BATCH@")) {
		          
		sp->sid_authority[5] = 5;
		sp->sid_authorities[0] = 3;
	} else if (!strcmp(id, "ANONYMOUS@")) {
		          
		sp->sid_authority[5] = 5;
		sp->sid_authorities[0] = 7;
	} else if (!strcmp(id, "AUTHENTICATED@")) {
		           
		sp->sid_authority[5] = 5;
		sp->sid_authorities[0] = 11;
	} else if (!strcmp(id, "SERVICE@")) {
		          
		sp->sid_authority[5] = 5;
		sp->sid_authorities[0] = 6;
	} else {
		                   
		sp->sid_authority[5] = 0;
		sp->sid_authorities[0] = 0;
	}
	return 0;
}

static int
nfs4_fallback_name(const char *id, int have_at)
{
	if (have_at) {
		                         
		                                         
		if (!strncmp(id, "root@", 5)) {
			return 0;
		} else if (!strncmp(id, "wheel@", 6)) {
			return 0;
		} else if (!strncmp(id, "nobody@", 7)) {
			return -2;
		} else if (!strncmp(id, "nfsnobody@", 10)) {
			return -2;
		}
	}
	return -2;
}

static void
nfs4_mapid_log(int error, const char *idstr, int isgroup, guid_t *gp)
{
	if (error && (nfs_idmap_ctrl & NFS_IDMAP_CTRL_LOG_FAILED_MAPPINGS)) {
		printf("nfs4_id2guid: idmap failed for %s %s error %d\n", idstr, isgroup ? "G" : " ", error);
	}
	if (!error && (nfs_idmap_ctrl & NFS_IDMAP_CTRL_LOG_SUCCESSFUL_MAPPINGS)) {
		printf("nfs4_id2guid: idmap for %s %s got guid "
		    "%02x%02x%02x%02x_%02x%02x%02x%02x_%02x%02x%02x%02x_%02x%02x%02x%02x\n",
		    idstr, isgroup ? "G" : " ",
		    gp->g_guid[0], gp->g_guid[1], gp->g_guid[2], gp->g_guid[3],
		    gp->g_guid[4], gp->g_guid[5], gp->g_guid[6], gp->g_guid[7],
		    gp->g_guid[8], gp->g_guid[9], gp->g_guid[10], gp->g_guid[11],
		    gp->g_guid[12], gp->g_guid[13], gp->g_guid[14], gp->g_guid[15]);
	}
}

static char *
nfs4_map_domain(char *id, char **atp)
{
	char *at = *atp;
	char *dsnode, *otw_nfs4domain;
	char *new_id = NULL;
	size_t otw_domain_len;
	size_t otw_id_2_at_len;
	int error;

	if (at == NULL) {
		at = strchr(id, '@');
	}
	if (at == NULL || *at != '@') {
		return NULL;
	}

	otw_nfs4domain = at + 1;
	otw_domain_len = strnlen(otw_nfs4domain, MAXPATHLEN);
	otw_id_2_at_len = at - id + 1;

	dsnode = zalloc(ZV_NAMEI);
	                                                               
	error = kauth_cred_nfs4domain2dsnode(otw_nfs4domain, dsnode);
	if (!error) {
		                                       
		size_t dsnode_len = strnlen(dsnode, MAXPATHLEN);
		size_t new_id_len = otw_id_2_at_len + dsnode_len + 1;
		char tmp;

		MALLOC(new_id, char*, new_id_len, M_TEMP, M_WAITOK);
		tmp = *otw_nfs4domain;
		*otw_nfs4domain = '\0';                              
		strlcpy(new_id, id, MAXPATHLEN);
		*otw_nfs4domain = tmp;                                                
		strlcat(new_id, dsnode, MAXPATHLEN);
		at = strchr(new_id, '@');
	} else {
		                                                           
		size_t default_domain_len = strnlen(nfs4_default_domain, MAXPATHLEN);

		if ((otw_domain_len == default_domain_len) &&
		    (strncmp(otw_nfs4domain, nfs4_default_domain, otw_domain_len) == 0)) {
			                                                           
			*at = '\0';
		}
	}
	NFS_ZFREE(ZV_NAMEI, dsnode);

	if (nfs_idmap_ctrl & NFS_IDMAP_CTRL_LOG_SUCCESSFUL_MAPPINGS) {
		printf("nfs4_id2guid: after domain mapping id is %s\n", id);
	}

	*atp = at;
	return new_id;
}

  
                                        
  
                                                                                         
   
int
nfs4_id2guid(          char *id, guid_t *guidp, int isgroup)
{
	int  error = 0;
	ntsid_t sid;
	long num;
	char *p, *at, *new_id = NULL;

	*guidp = kauth_null_guid;

	  
                                                                                  
                                                                                
                                                                               
    
	num = 1;
	at = NULL;
	p = id;
	while (*p) {
		if ((*p < '0') || (*p > '9')) {
			num = 0;
		}
		if (*p == '@') {
			at = p;
		}
		p++;
	}

	if (num) {
		                                   
		num = *id ? strtol(id, NULL, 10) : -2;
		if (isgroup) {
			error = kauth_cred_gid2guid((gid_t)num, guidp);
		} else {
			error = kauth_cred_uid2guid((uid_t)num, guidp);
		}
		nfs4_mapid_log(error, id, isgroup, guidp);
		return error;
	}

	                                            
	error = nfs4_wkid2sid(id, &sid);
	if (!error) {
		error = kauth_cred_ntsid2guid(&sid, guidp);
		nfs4_mapid_log(error, id, 1, guidp);
		return error;
	}

	                              
	if (at && at[1]) {
		new_id = nfs4_map_domain(id, &at);
		if (new_id) {
			id = new_id;
		}
	}

	                                     
	if (nfs_idmap_ctrl & NFS_IDMAP_CTRL_USE_IDMAP_SERVICE) {
		  
                                                               
    
                                                             
     
		if (isgroup) {
			error = kauth_cred_grnam2guid(id, guidp);
		} else {
			error = kauth_cred_pwnam2guid(id, guidp);
		}
		nfs4_mapid_log(error, id, isgroup, guidp);
	} else {
		error = ENOTSUP;
	}

	if (error) {
		  
                                                                     
     
		num = nfs4_fallback_name(id, at != NULL);
		if (isgroup) {
			error = kauth_cred_gid2guid((gid_t)num, guidp);
		} else {
			error = kauth_cred_uid2guid((uid_t)num, guidp);
		}
		nfs4_mapid_log(error, id, isgroup, guidp);
	}


	                                                              
	if (at && *at == '\0') {
		*at = '@';
	}

	                                  
	if (new_id) {
		FREE(new_id, M_TEMP);
	}

	return error;
}

  
                 
                                       
                                                                   
   
#define MAXWELLKNOWNID 18

static const char*
nfs4_sid2wkid(ntsid_t *sp)
{
	if ((sp->sid_kind == 1) && (sp->sid_authcount == 1)) {
		                                                       
		if (sp->sid_authority[5] == 0) {
			if (sp->sid_authorities[0] == 0) {           
				return "nobody@localdomain";
			}
		} else if (sp->sid_authority[5] == 1) {
			if (sp->sid_authorities[0] == 0) {           
				return "EVERYONE@";
			}
		} else if (sp->sid_authority[5] == 3) {
			if (sp->sid_authorities[0] == 0) {           
				return "OWNER@";
			} else if (sp->sid_authorities[0] == 1) {           
				return "GROUP@";
			}
		} else if (sp->sid_authority[5] == 5) {
			if (sp->sid_authorities[0] == 1) {           
				return "DIALUP@";
			} else if (sp->sid_authorities[0] == 2) {           
				return "NETWORK@";
			} else if (sp->sid_authorities[0] == 3) {           
				return "BATCH@";
			} else if (sp->sid_authorities[0] == 4) {           
				return "INTERACTIVE@";
			} else if (sp->sid_authorities[0] == 6) {           
				return "SERVICE@";
			} else if (sp->sid_authorities[0] == 7) {           
				return "ANONYMOUS@";
			} else if (sp->sid_authorities[0] == 11) {            
				return "AUTHENTICATED@";
			}
		}
	}
	return NULL;
}

static void
nfs4_mapguid_log(int error, const char *where, guid_t *gp, int isgroup, const char *idstr)
{
	if (error && (nfs_idmap_ctrl & NFS_IDMAP_CTRL_LOG_FAILED_MAPPINGS)) {
		printf("nfs4_guid2id: %s idmap failed for "
		    "%02x%02x%02x%02x_%02x%02x%02x%02x_%02x%02x%02x%02x_%02x%02x%02x%02x %s "
		    "error %d\n", where,
		    gp->g_guid[0], gp->g_guid[1], gp->g_guid[2], gp->g_guid[3],
		    gp->g_guid[4], gp->g_guid[5], gp->g_guid[6], gp->g_guid[7],
		    gp->g_guid[8], gp->g_guid[9], gp->g_guid[10], gp->g_guid[11],
		    gp->g_guid[12], gp->g_guid[13], gp->g_guid[14], gp->g_guid[15],
		    isgroup ? "G" : " ", error);
	}
	if (!error && (nfs_idmap_ctrl & NFS_IDMAP_CTRL_LOG_SUCCESSFUL_MAPPINGS)) {
		printf("nfs4_guid2id: %s idmap for "
		    "%02x%02x%02x%02x_%02x%02x%02x%02x_%02x%02x%02x%02x_%02x%02x%02x%02x %s "
		    "got ID %s\n", where,
		    gp->g_guid[0], gp->g_guid[1], gp->g_guid[2], gp->g_guid[3],
		    gp->g_guid[4], gp->g_guid[5], gp->g_guid[6], gp->g_guid[7],
		    gp->g_guid[8], gp->g_guid[9], gp->g_guid[10], gp->g_guid[11],
		    gp->g_guid[12], gp->g_guid[13], gp->g_guid[14], gp->g_guid[15],
		    isgroup ? "G" : " ", idstr);
	}
}

static int
nfs4_addv4domain(char *id, size_t *idlen)
{
	char *at = NULL, *cp;
	int have_domain;
	int error = 0;
	size_t idsize;


	if (id == NULL || *id == '\0') {
		return EINVAL;
	}

	for (cp = id; *cp != '\0'; cp++) {
		if (*cp == '@') {
			at = cp;
			break;
		}
	}

	have_domain = (at && at[1] != '\0');

	if (have_domain) {
		char *dsnode = at + 1;
		char *nfs4domain;
		size_t domain_len;
		char *mapped_domain;

		nfs4domain = zalloc(ZV_NAMEI);
		error = kauth_cred_dsnode2nfs4domain(dsnode, nfs4domain);
		if (!error) {
			domain_len = strnlen(nfs4domain, MAXPATHLEN);
			mapped_domain = nfs4domain;
		} else {
			error = 0;
			domain_len = strnlen(nfs4_default_domain, MAXPATHLEN);
			mapped_domain = nfs4_default_domain;
		}
		if (domain_len) {
			                               
			at[1] = '\0';
			                           
			idsize = strlcat(id, mapped_domain, *idlen);
			if (*idlen > idsize) {
				*idlen = idsize;
			} else {
				error = ENOSPC;
			}
		}
		NFS_ZFREE(ZV_NAMEI, nfs4domain);
	} else if (at == NULL) {
		  
                                                                         
                                                                
                                                  
     
		size_t default_domain_len = strnlen(nfs4_default_domain, MAXPATHLEN);

		if (default_domain_len) {
			strlcat(id, "@", *idlen);
			idsize = strlcat(id, nfs4_default_domain, *idlen);
			if (*idlen > idsize) {
				*idlen = idsize;
			} else {
				error = ENOSPC;
			}
		} else {
			;                        
		}
	}

	if (!error && nfs_idmap_ctrl & NFS_IDMAP_CTRL_LOG_SUCCESSFUL_MAPPINGS) {
		printf("nfs4_guid2id: id after nfs4 domain map: %s[%zd].\n", id, *idlen);
	}

	return error;
}

static char *
nfs4_fallback_id(int numid, int isgrp, char *buf, size_t size)
{
	const char *idp = NULL;

	if (!(nfs_idmap_ctrl & NFS_IDMAP_CTRL_FALLBACK_NO_COMMON_IDS)) {
		                                     
		if (numid == 0) {
			idp = isgrp ? "wheel" : "root";
		} else if (numid == -2) {
			idp = "nobody";
		}
	}
	if (!idp) {
		                                          
		snprintf(buf, size - 1, "%d", numid);
		buf[size - 1] = '\0';
	} else {
		size_t idplen = strlcpy(buf, idp, size);
		if (idplen >= size) {
			return NULL;
		}
	}

	return buf;
}

  
                                        
  
                                                                                         
   
int
nfs4_guid2id(guid_t *guidp, char *id, size_t *idlen, int isgroup)
{
	int  error = 0;
	size_t id1len, len;
	char *id1buf, *id1;
	char numbuf[32];
	ntsid_t sid;

	id1buf = id1 = NULL;
	id1len = 0;

	  
                                                   
    
	error = kauth_cred_guid2ntsid(guidp, &sid);
	if (!error) {
		const char *wkid = nfs4_sid2wkid(&sid);
		if (wkid) {
			len = strnlen(wkid, MAXWELLKNOWNID);
			strlcpy(id, wkid, *idlen);
			error = (len < *idlen) ? 0 : ENOSPC;
			*idlen = len;
			nfs4_mapguid_log(error, "kauth_cred_guid2ntsid", guidp, 1, id);
			return error;
		}
	} else {
		nfs4_mapguid_log(error, "kauth_cred_guid2ntsid", guidp, isgroup, NULL);
	}

	if (nfs_idmap_ctrl & NFS_IDMAP_CTRL_USE_IDMAP_SERVICE) {
		  
                                                                
    
                                                       
     

		  
                                                                     
                                                                     
                                                 
     

		if (*idlen < MAXPATHLEN) {
			id1buf = zalloc(ZV_NAMEI);
			id1 = id1buf;
			id1len = MAXPATHLEN;
		} else {
			id1 = id;
			id1len = *idlen;
		}

		if (isgroup) {
			error = kauth_cred_guid2grnam(guidp, id1);
		} else {
			error = kauth_cred_guid2pwnam(guidp, id1);
		}
		if (error) {
			nfs4_mapguid_log(error, "kauth_cred2[pw|gr]nam", guidp, isgroup, id1);
		}
	} else {
		error = ENOTSUP;
	}

	if (error) {
		  
                                                                     
     
		uid_t uid;

		                                                
		if (isgroup) {
			error = kauth_cred_guid2gid(guidp, (gid_t*)&uid);
		} else {
			error = kauth_cred_guid2uid(guidp, &uid);
		}
		if (!error) {
			char *fbidp = nfs4_fallback_id(uid, isgroup, numbuf, sizeof(numbuf));
			if (fbidp == NULL) {
				error = ENOSPC;
			} else {
				id1 = fbidp;
			}
		}
	} else {
		error = nfs4_addv4domain(id1, &id1len);
	}

	if (!error) {
		if (id1 != id) {
			                                        
			len = strlcpy(id, id1, *idlen);
			if (len >= *idlen) {
				error = ENOSPC;
			} else {
				*idlen = len;
			}
		}
	}
	nfs4_mapguid_log(error, "End of routine", guidp, isgroup, id1);

	if (id1buf) {
		NFS_ZFREE(ZV_NAMEI, id1buf);
	}

	return error;
}

  
                                                                  
   
void
nfs_vattr_set_supported(uint32_t *bitmap, struct vnode_attr *vap)
{
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TYPE)) {
		VATTR_SET_SUPPORTED(vap, va_type);
	}
	                                                  
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SIZE)) {
		VATTR_SET_SUPPORTED(vap, va_data_size);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_FSID)) {
		VATTR_SET_SUPPORTED(vap, va_fsid);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_ACL)) {
		VATTR_SET_SUPPORTED(vap, va_acl);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_ARCHIVE)) {
		VATTR_SET_SUPPORTED(vap, va_flags);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_FILEID)) {
		VATTR_SET_SUPPORTED(vap, va_fileid);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_HIDDEN)) {
		VATTR_SET_SUPPORTED(vap, va_flags);
	}
	                                                    
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MODE)) {
		VATTR_SET_SUPPORTED(vap, va_mode);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_NUMLINKS)) {
		VATTR_SET_SUPPORTED(vap, va_nlink);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_OWNER)) {
		VATTR_SET_SUPPORTED(vap, va_uid);
		VATTR_SET_SUPPORTED(vap, va_uuuid);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_OWNER_GROUP)) {
		VATTR_SET_SUPPORTED(vap, va_gid);
		VATTR_SET_SUPPORTED(vap, va_guuid);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_RAWDEV)) {
		VATTR_SET_SUPPORTED(vap, va_rdev);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SPACE_USED)) {
		VATTR_SET_SUPPORTED(vap, va_total_alloc);
	}
	                                                  
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_ACCESS)) {
		VATTR_SET_SUPPORTED(vap, va_access_time);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_BACKUP)) {
		VATTR_SET_SUPPORTED(vap, va_backup_time);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_CREATE)) {
		VATTR_SET_SUPPORTED(vap, va_create_time);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_METADATA)) {
		VATTR_SET_SUPPORTED(vap, va_change_time);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_MODIFY)) {
		VATTR_SET_SUPPORTED(vap, va_modify_time);
	}
}

  
                                                                   
                        
   
int
nfs4_parsefattr(
	struct nfsm_chain *nmc,
	struct nfs_fsattr *nfsap,
	struct nfs_vattr *nvap,
	fhandle_t *fhp,
	struct dqblk *dqbp,
	struct nfs_fs_locations *nfslsp)
{
	int error = 0, error2, rderror = 0, attrbytes;
	uint32_t val, val2, val3, i;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN], len;
	size_t slen;
	char sbuf[64], *s;
	struct nfs_fsattr nfsa_dummy;
	struct nfs_vattr *nva_dummy = NULL;
	struct dqblk dqb_dummy;
	kauth_acl_t acl = NULL;
	uint32_t ace_type, ace_flags, ace_mask;
	struct nfs_fs_locations nfsls_dummy;
	struct sockaddr_storage ss;

	                                                                               
	if (!nfsap) {
		nfsap = &nfsa_dummy;
	}
	if (!nvap) {
		MALLOC(nva_dummy, struct nfs_vattr *, sizeof(*nva_dummy), M_TEMP, M_WAITOK);
		nvap = nva_dummy;
	}
	if (!dqbp) {
		dqbp = &dqb_dummy;
	}
	if (!nfslsp) {
		nfslsp = &nfsls_dummy;
	}
	bzero(nfslsp, sizeof(*nfslsp));

	attrbytes = val = val2 = val3 = 0;
	s = sbuf;
	slen = sizeof(sbuf);
	NVATTR_INIT(nvap);

	len = NFS_ATTR_BITMAP_LEN;
	nfsm_chain_get_bitmap(error, nmc, bitmap, len);
	                                        
	for (i = 0; i < NFS_ATTR_BITMAP_LEN; i++) {
		nvap->nva_bitmap[i] |= bitmap[i] & nfs_object_attr_bitmap[i];
		nfsap->nfsa_bitmap[i] |= bitmap[i] & nfs_fs_attr_bitmap[i];
	}

	nfsm_chain_get_32(error, nmc, attrbytes);
	nfsmout_if(error);

	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SUPPORTED_ATTRS)) {
		len = NFS_ATTR_BITMAP_LEN;
		nfsm_chain_get_bitmap(error, nmc, nfsap->nfsa_supp_attr, len);
		attrbytes -= (len + 1) * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TYPE)) {
		nfsm_chain_get_32(error, nmc, val);
		nvap->nva_type = nfstov_type(val, NFS_VER4);
		if ((val == NFATTRDIR) || (val == NFNAMEDATTR)) {
			nvap->nva_flags |= NFS_FFLAG_IS_ATTR;
		} else {
			nvap->nva_flags &= ~NFS_FFLAG_IS_ATTR;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_FH_EXPIRE_TYPE)) {
		nfsm_chain_get_32(error, nmc, val);
		nfsmout_if(error);
		nfsap->nfsa_flags &= ~NFS_FSFLAG_FHTYPE_MASK;
		nfsap->nfsa_flags |= val << NFS_FSFLAG_FHTYPE_SHIFT;
		if (val & ~0xff) {
			printf("nfs: warning unknown fh type: 0x%x\n", val);
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_CHANGE)) {
		nfsm_chain_get_64(error, nmc, nvap->nva_change);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SIZE)) {
		nfsm_chain_get_64(error, nmc, nvap->nva_size);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_LINK_SUPPORT)) {
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nfsap->nfsa_flags |= NFS_FSFLAG_LINK;
		} else {
			nfsap->nfsa_flags &= ~NFS_FSFLAG_LINK;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SYMLINK_SUPPORT)) {
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nfsap->nfsa_flags |= NFS_FSFLAG_SYMLINK;
		} else {
			nfsap->nfsa_flags &= ~NFS_FSFLAG_SYMLINK;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_NAMED_ATTR)) {
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nvap->nva_flags |= NFS_FFLAG_HAS_NAMED_ATTRS;
		} else {
			nvap->nva_flags &= ~NFS_FFLAG_HAS_NAMED_ATTRS;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_FSID)) {
		nfsm_chain_get_64(error, nmc, nvap->nva_fsid.major);
		nfsm_chain_get_64(error, nmc, nvap->nva_fsid.minor);
		attrbytes -= 4 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_UNIQUE_HANDLES)) {
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nfsap->nfsa_flags |= NFS_FSFLAG_UNIQUE_FH;
		} else {
			nfsap->nfsa_flags &= ~NFS_FSFLAG_UNIQUE_FH;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_LEASE_TIME)) {
		nfsm_chain_get_32(error, nmc, nfsap->nfsa_lease);
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_RDATTR_ERROR)) {
		nfsm_chain_get_32(error, nmc, rderror);
		attrbytes -= NFSX_UNSIGNED;
		if (!rderror) {               
			NFS_BITMAP_CLR(bitmap, NFS_FATTR_RDATTR_ERROR);
			NFS_BITMAP_CLR(nvap->nva_bitmap, NFS_FATTR_RDATTR_ERROR);
		}
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_ACL)) {
		error2 = 0;
		ace_type = ace_flags = ace_mask = 0;
		nfsm_chain_get_32(error, nmc, val);                
		if (!error && (val > KAUTH_ACL_MAX_ENTRIES)) {
			error = EOVERFLOW;
		}
		if (!error && !((acl = kauth_acl_alloc(val)))) {
			error = ENOMEM;
		}
		if (!error && acl) {
			acl->acl_entrycount = val;
			acl->acl_flags = 0;
		}
		attrbytes -= NFSX_UNSIGNED;
		nfsm_assert(error, (attrbytes >= 0), EBADRPC);
		for (i = 0; !error && (i < val); i++) {
			nfsm_chain_get_32(error, nmc, ace_type);
			nfsm_chain_get_32(error, nmc, ace_flags);
			nfsm_chain_get_32(error, nmc, ace_mask);
			nfsm_chain_get_32(error, nmc, len);
			if (!error && len >= NFS_MAX_WHO) {
				error = EBADRPC;
			}
			acl->acl_ace[i].ace_flags = nfs4_ace_nfstype_to_vfstype(ace_type, &error);
			acl->acl_ace[i].ace_flags |= nfs4_ace_nfsflags_to_vfsflags(ace_flags);
			acl->acl_ace[i].ace_rights = nfs4_ace_nfsmask_to_vfsrights(ace_mask);
			if (!error && !error2 && (len >= slen)) {
				if (s != sbuf) {
					FREE(s, M_TEMP);
					s = sbuf;
					slen = sizeof(sbuf);
				}
				                                                                                             
				MALLOC(s, char*, (len + 16 < NFS_MAX_WHO) ? len + 16 : NFS_MAX_WHO, M_TEMP, M_WAITOK);
				if (s) {
					slen = (len + 16 < NFS_MAX_WHO) ? len + 16 : NFS_MAX_WHO;
				} else {
					error = ENOMEM;
				}
			}
			if (error2) {
				nfsm_chain_adv(error, nmc, nfsm_rndup(len));
			} else {
				nfsm_chain_get_opaque(error, nmc, len, s);
			}
			if (!error && !error2) {
				s[len] = '\0';
				error2 = nfs4_id2guid(s, &acl->acl_ace[i].ace_applicable,
				    (ace_flags & NFS_ACE_IDENTIFIER_GROUP));
				if (error2 && (nfs_idmap_ctrl & NFS_IDMAP_CTRL_LOG_FAILED_MAPPINGS)) {
					printf("nfs4_parsefattr: ACE WHO %s is no one, no guid?, error %d\n", s, error2);
				}
			}
			attrbytes -= 4 * NFSX_UNSIGNED + nfsm_rndup(len);
			nfsm_assert(error, (attrbytes >= 0), EBADRPC);
		}
		nfsmout_if(error);
		if ((nvap != nva_dummy) && !error2) {
			nvap->nva_acl = acl;
			acl = NULL;
		}
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_ACLSUPPORT)) {
		  
                                                             
                                                              
     
		nfsm_chain_get_32(error, nmc, val);
		if ((val & (NFS_ACL_SUPPORT_ALLOW_ACL | NFS_ACL_SUPPORT_DENY_ACL)) &&
		    NFS_BITMAP_ISSET(nfsap->nfsa_supp_attr, NFS_FATTR_ACL)) {
			nfsap->nfsa_flags |= NFS_FSFLAG_ACL;
		} else {
			nfsap->nfsa_flags &= ~NFS_FSFLAG_ACL;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_ARCHIVE)) {                  
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nvap->nva_flags |= NFS_FFLAG_ARCHIVED;
		} else {
			nvap->nva_flags &= ~NFS_FFLAG_ARCHIVED;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_CANSETTIME)) {
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nfsap->nfsa_flags |= NFS_FSFLAG_SET_TIME;
		} else {
			nfsap->nfsa_flags &= ~NFS_FSFLAG_SET_TIME;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_CASE_INSENSITIVE)) {
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nfsap->nfsa_flags |= NFS_FSFLAG_CASE_INSENSITIVE;
		} else {
			nfsap->nfsa_flags &= ~NFS_FSFLAG_CASE_INSENSITIVE;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_CASE_PRESERVING)) {
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nfsap->nfsa_flags |= NFS_FSFLAG_CASE_PRESERVING;
		} else {
			nfsap->nfsa_flags &= ~NFS_FSFLAG_CASE_PRESERVING;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_CHOWN_RESTRICTED)) {
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nfsap->nfsa_flags |= NFS_FSFLAG_CHOWN_RESTRICTED;
		} else {
			nfsap->nfsa_flags &= ~NFS_FSFLAG_CHOWN_RESTRICTED;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_FILEHANDLE)) {
		nfsm_chain_get_32(error, nmc, val);
		if (error == 0 && val > NFS_MAX_FH_SIZE) {
			error = EBADRPC;
		}
		nfsmout_if(error);
		if (fhp) {
			fhp->fh_len = val;
			nfsm_chain_get_opaque(error, nmc, nfsm_rndup(val), fhp->fh_data);
		} else {
			nfsm_chain_adv(error, nmc, nfsm_rndup(val));
		}
		attrbytes -= NFSX_UNSIGNED + nfsm_rndup(val);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_FILEID)) {
		nfsm_chain_get_64(error, nmc, nvap->nva_fileid);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_FILES_AVAIL)) {
		nfsm_chain_get_64(error, nmc, nfsap->nfsa_files_avail);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_FILES_FREE)) {
		nfsm_chain_get_64(error, nmc, nfsap->nfsa_files_free);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_FILES_TOTAL)) {
		nfsm_chain_get_64(error, nmc, nfsap->nfsa_files_total);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_FS_LOCATIONS)) {
		uint32_t loc, serv, comp;
		struct nfs_fs_location *fsl;
		struct nfs_fs_server *fss;
		struct nfs_fs_path *fsp;

		                       
		fsp = &nfslsp->nl_root;
		nfsm_chain_get_32(error, nmc, fsp->np_compcount);                      
		attrbytes -= NFSX_UNSIGNED;
		                                  
		if (!error && (fsp->np_compcount > MAXPATHLEN)) {
			error = EBADRPC;
		}
		nfsmout_if(error);
		if (fsp->np_compcount) {
			MALLOC(fsp->np_components, char **, fsp->np_compcount * sizeof(char*), M_TEMP, M_WAITOK | M_ZERO);
			if (!fsp->np_components) {
				error = ENOMEM;
			}
		}
		for (comp = 0; comp < fsp->np_compcount; comp++) {
			nfsm_chain_get_32(error, nmc, val);                       
			                                   
			if (!error && (val == 0)) {
				  
                                                                      
                                                                      
                              
       
				comp--;
				fsp->np_compcount--;
				if (fsp->np_compcount == 0) {
					FREE(fsp->np_components, M_TEMP);
					fsp->np_components = NULL;
				}
				attrbytes -= NFSX_UNSIGNED;
				continue;
			}
			if (!error && ((val < 1) || (val > MAXPATHLEN))) {
				error = EBADRPC;
			}
			nfsmout_if(error);
			MALLOC(fsp->np_components[comp], char *, val + 1, M_TEMP, M_WAITOK | M_ZERO);
			if (!fsp->np_components[comp]) {
				error = ENOMEM;
			}
			nfsmout_if(error);
			nfsm_chain_get_opaque(error, nmc, val, fsp->np_components[comp]);                
			attrbytes -= NFSX_UNSIGNED + nfsm_rndup(val);
		}
		nfsm_chain_get_32(error, nmc, nfslsp->nl_numlocs);                        
		attrbytes -= NFSX_UNSIGNED;
		                                 
		if (!error && (nfslsp->nl_numlocs > 256)) {
			error = EBADRPC;
		}
		nfsmout_if(error);
		if (nfslsp->nl_numlocs > 0) {
			MALLOC(nfslsp->nl_locations, struct nfs_fs_location **, nfslsp->nl_numlocs * sizeof(struct nfs_fs_location*), M_TEMP, M_WAITOK | M_ZERO);
			if (!nfslsp->nl_locations) {
				error = ENOMEM;
			}
		}
		nfsmout_if(error);
		for (loc = 0; loc < nfslsp->nl_numlocs; loc++) {
			nfsmout_if(error);
			MALLOC(fsl, struct nfs_fs_location *, sizeof(struct nfs_fs_location), M_TEMP, M_WAITOK | M_ZERO);
			if (!fsl) {
				error = ENOMEM;
			}
			nfslsp->nl_locations[loc] = fsl;
			nfsm_chain_get_32(error, nmc, fsl->nl_servcount);                   
			attrbytes -= NFSX_UNSIGNED;
			                               
			if (!error && ((fsl->nl_servcount < 1) || (fsl->nl_servcount > 256))) {
				error = EBADRPC;
			}
			nfsmout_if(error);
			MALLOC(fsl->nl_servers, struct nfs_fs_server **, fsl->nl_servcount * sizeof(struct nfs_fs_server*), M_TEMP, M_WAITOK | M_ZERO);
			if (!fsl->nl_servers) {
				error = ENOMEM;
			}
			for (serv = 0; serv < fsl->nl_servcount; serv++) {
				nfsmout_if(error);
				MALLOC(fss, struct nfs_fs_server *, sizeof(struct nfs_fs_server), M_TEMP, M_WAITOK | M_ZERO);
				if (!fss) {
					error = ENOMEM;
				}
				fsl->nl_servers[serv] = fss;
				nfsm_chain_get_32(error, nmc, val);                         
				                                     
				if (!error && ((val < 1) || (val > MAXPATHLEN))) {
					error = EINVAL;
				}
				nfsmout_if(error);
				MALLOC(fss->ns_name, char *, val + 1, M_TEMP, M_WAITOK | M_ZERO);
				if (!fss->ns_name) {
					error = ENOMEM;
				}
				nfsm_chain_get_opaque(error, nmc, val, fss->ns_name);                  
				attrbytes -= NFSX_UNSIGNED + nfsm_rndup(val);
				nfsmout_if(error);
				                                                       
				if (nfs_uaddr2sockaddr(fss->ns_name, (struct sockaddr*)&ss)) {
					fss->ns_addrcount = 1;
					MALLOC(fss->ns_addresses, char **, sizeof(char *), M_TEMP, M_WAITOK | M_ZERO);
					if (!fss->ns_addresses) {
						error = ENOMEM;
					}
					nfsmout_if(error);
					MALLOC(fss->ns_addresses[0], char *, val + 1, M_TEMP, M_WAITOK | M_ZERO);
					if (!fss->ns_addresses[0]) {
						error = ENOMEM;
					}
					nfsmout_if(error);
					strlcpy(fss->ns_addresses[0], fss->ns_name, val + 1);
				}
			}
			                  
			fsp = &fsl->nl_path;
			nfsm_chain_get_32(error, nmc, fsp->np_compcount);                      
			attrbytes -= NFSX_UNSIGNED;
			                                  
			if (!error && (fsp->np_compcount > MAXPATHLEN)) {
				error = EINVAL;
			}
			nfsmout_if(error);
			if (fsp->np_compcount) {
				MALLOC(fsp->np_components, char **, fsp->np_compcount * sizeof(char*), M_TEMP, M_WAITOK | M_ZERO);
				if (!fsp->np_components) {
					error = ENOMEM;
				}
			}
			for (comp = 0; comp < fsp->np_compcount; comp++) {
				nfsm_chain_get_32(error, nmc, val);                       
				                                   
				if (!error && (val == 0)) {
					  
                                                                       
                                                                       
                               
        
					comp--;
					fsp->np_compcount--;
					if (fsp->np_compcount == 0) {
						FREE(fsp->np_components, M_TEMP);
						fsp->np_components = NULL;
					}
					attrbytes -= NFSX_UNSIGNED;
					continue;
				}
				if (!error && ((val < 1) || (val > MAXPATHLEN))) {
					error = EINVAL;
				}
				nfsmout_if(error);
				MALLOC(fsp->np_components[comp], char *, val + 1, M_TEMP, M_WAITOK | M_ZERO);
				if (!fsp->np_components[comp]) {
					error = ENOMEM;
				}
				nfsm_chain_get_opaque(error, nmc, val, fsp->np_components[comp]);                
				attrbytes -= NFSX_UNSIGNED + nfsm_rndup(val);
			}
		}
		nfsm_assert(error, (attrbytes >= 0), EBADRPC);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_HIDDEN)) {                
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nvap->nva_flags |= NFS_FFLAG_HIDDEN;
		} else {
			nvap->nva_flags &= ~NFS_FFLAG_HIDDEN;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_HOMOGENEOUS)) {
		                                                                     
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nfsap->nfsa_flags |= NFS_FSFLAG_HOMOGENEOUS;
		} else {
			nfsap->nfsa_flags &= ~NFS_FSFLAG_HOMOGENEOUS;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MAXFILESIZE)) {
		nfsm_chain_get_64(error, nmc, nfsap->nfsa_maxfilesize);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MAXLINK)) {
		nfsm_chain_get_32(error, nmc, nvap->nva_maxlink);
		if (!error && (nfsap->nfsa_maxlink > INT32_MAX)) {
			nfsap->nfsa_maxlink = INT32_MAX;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MAXNAME)) {
		nfsm_chain_get_32(error, nmc, nfsap->nfsa_maxname);
		if (!error && (nfsap->nfsa_maxname > INT32_MAX)) {
			nfsap->nfsa_maxname = INT32_MAX;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MAXREAD)) {
		nfsm_chain_get_64(error, nmc, nfsap->nfsa_maxread);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MAXWRITE)) {
		nfsm_chain_get_64(error, nmc, nfsap->nfsa_maxwrite);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MIMETYPE)) {
		nfsm_chain_get_32(error, nmc, val);
		nfsm_chain_adv(error, nmc, nfsm_rndup(val));
		attrbytes -= NFSX_UNSIGNED + nfsm_rndup(val);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MODE)) {
		nfsm_chain_get_32(error, nmc, val);
		if (val > ALLPERMS) {
			error = EBADRPC;
		} else {
			nvap->nva_mode = (mode_t)val;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_NO_TRUNC)) {
		nfsm_chain_get_32(error, nmc, val);
		if (val) {
			nfsap->nfsa_flags |= NFS_FSFLAG_NO_TRUNC;
		} else {
			nfsap->nfsa_flags &= ~NFS_FSFLAG_NO_TRUNC;
		}
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_NUMLINKS)) {
		nfsm_chain_get_32(error, nmc, val);
		nvap->nva_nlink = val;
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_OWNER)) {
		nfsm_chain_get_32(error, nmc, len);
		if (!error && len >= NFS_MAX_WHO) {
			error = EBADRPC;
		}
		if (!error && (len >= slen)) {
			if (s != sbuf) {
				FREE(s, M_TEMP);
				s = sbuf;
				slen = sizeof(sbuf);
			}
			                                                                                             
			MALLOC(s, char*, (len + 16 < NFS_MAX_WHO) ? len + 16 : NFS_MAX_WHO, M_TEMP, M_WAITOK);
			if (s) {
				slen = (len + 16 < NFS_MAX_WHO) ? len + 16 : NFS_MAX_WHO;
			} else {
				error = ENOMEM;
			}
		}
		nfsm_chain_get_opaque(error, nmc, len, s);
		if (!error) {
			s[len] = '\0';
			error = nfs4_id2guid(s, &nvap->nva_uuuid, 0);
			if (!error) {
				error = kauth_cred_guid2uid(&nvap->nva_uuuid, &nvap->nva_uid);
			}
			if (error) {
				                                                      
				nvap->nva_uid = (uid_t)(-2);
				if (nfs_idmap_ctrl & NFS_IDMAP_CTRL_LOG_FAILED_MAPPINGS) {
					printf("nfs4_parsefattr: owner %s is no one, no %s?, error %d\n", s,
					    kauth_guid_equal(&nvap->nva_uuuid, &kauth_null_guid) ? "guid" : "uid",
					    error);
				}
				error = 0;
			}
		}
		attrbytes -= NFSX_UNSIGNED + nfsm_rndup(len);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_OWNER_GROUP)) {
		nfsm_chain_get_32(error, nmc, len);
		if (!error && len >= NFS_MAX_WHO) {
			error = EBADRPC;
		}
		if (!error && (len >= slen)) {
			if (s != sbuf) {
				FREE(s, M_TEMP);
				s = sbuf;
				slen = sizeof(sbuf);
			}
			                                                                                             
			MALLOC(s, char*, (len + 16 < NFS_MAX_WHO) ? len + 16 : NFS_MAX_WHO, M_TEMP, M_WAITOK);
			if (s) {
				slen = (len + 16 < NFS_MAX_WHO) ? len + 16 : NFS_MAX_WHO;
			} else {
				error = ENOMEM;
			}
		}
		nfsm_chain_get_opaque(error, nmc, len, s);
		if (!error) {
			s[len] = '\0';
			error = nfs4_id2guid(s, &nvap->nva_guuid, 1);
			if (!error) {
				error = kauth_cred_guid2gid(&nvap->nva_guuid, &nvap->nva_gid);
			}
			if (error) {
				                                                      
				nvap->nva_gid = (gid_t)(-2);
				if (nfs_idmap_ctrl & NFS_IDMAP_CTRL_LOG_FAILED_MAPPINGS) {
					printf("nfs4_parsefattr: group %s is no one, no %s?, error %d\n", s,
					    kauth_guid_equal(&nvap->nva_guuid, &kauth_null_guid) ? "guid" : "gid",
					    error);
				}
				error = 0;
			}
		}
		attrbytes -= NFSX_UNSIGNED + nfsm_rndup(len);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_QUOTA_AVAIL_HARD)) {
		nfsm_chain_get_64(error, nmc, dqbp->dqb_bhardlimit);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_QUOTA_AVAIL_SOFT)) {
		nfsm_chain_get_64(error, nmc, dqbp->dqb_bsoftlimit);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_QUOTA_USED)) {
		nfsm_chain_get_64(error, nmc, dqbp->dqb_curbytes);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_RAWDEV)) {
		nfsm_chain_get_32(error, nmc, nvap->nva_rawdev.specdata1);
		nfsm_chain_get_32(error, nmc, nvap->nva_rawdev.specdata2);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SPACE_AVAIL)) {
		nfsm_chain_get_64(error, nmc, nfsap->nfsa_space_avail);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SPACE_FREE)) {
		nfsm_chain_get_64(error, nmc, nfsap->nfsa_space_free);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SPACE_TOTAL)) {
		nfsm_chain_get_64(error, nmc, nfsap->nfsa_space_total);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SPACE_USED)) {
		nfsm_chain_get_64(error, nmc, nvap->nva_bytes);
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SYSTEM)) {
		                                                        
		nfsm_chain_adv(error, nmc, NFSX_UNSIGNED);
		attrbytes -= NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_ACCESS)) {
		nfsm_chain_get_64(error, nmc, nvap->nva_timesec[NFSTIME_ACCESS]);
		nfsm_chain_get_32(error, nmc, nvap->nva_timensec[NFSTIME_ACCESS]);
		attrbytes -= 3 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_ACCESS_SET)) {
		nfsm_chain_adv(error, nmc, 4 * NFSX_UNSIGNED);                   
		attrbytes -= 4 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_BACKUP)) {
		nfsm_chain_get_64(error, nmc, nvap->nva_timesec[NFSTIME_BACKUP]);
		nfsm_chain_get_32(error, nmc, nvap->nva_timensec[NFSTIME_BACKUP]);
		attrbytes -= 3 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_CREATE)) {
		nfsm_chain_get_64(error, nmc, nvap->nva_timesec[NFSTIME_CREATE]);
		nfsm_chain_get_32(error, nmc, nvap->nva_timensec[NFSTIME_CREATE]);
		attrbytes -= 3 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_DELTA)) {                   
		nfsm_chain_adv(error, nmc, 3 * NFSX_UNSIGNED);
		attrbytes -= 3 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_METADATA)) {
		nfsm_chain_get_64(error, nmc, nvap->nva_timesec[NFSTIME_CHANGE]);
		nfsm_chain_get_32(error, nmc, nvap->nva_timensec[NFSTIME_CHANGE]);
		attrbytes -= 3 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_MODIFY)) {
		nfsm_chain_get_64(error, nmc, nvap->nva_timesec[NFSTIME_MODIFY]);
		nfsm_chain_get_32(error, nmc, nvap->nva_timensec[NFSTIME_MODIFY]);
		attrbytes -= 3 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_MODIFY_SET)) {
		nfsm_chain_adv(error, nmc, 4 * NFSX_UNSIGNED);                   
		attrbytes -= 4 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MOUNTED_ON_FILEID)) {
#if CONFIG_TRIGGERS
		                                                                  
		nfsm_chain_get_64(error, nmc, nvap->nva_fileid);
#else
		nfsm_chain_adv(error, nmc, 2 * NFSX_UNSIGNED);
#endif
		attrbytes -= 2 * NFSX_UNSIGNED;
	}
	                                         
	nfsm_assert(error, (attrbytes >= 0), EBADRPC);
	nfsm_chain_adv(error, nmc, nfsm_rndup(attrbytes));
nfsmout:
	if (error) {
		nfs_fs_locations_cleanup(nfslsp);
	}
	if (!error && rderror) {
		error = rderror;
	}
	                                 
	if (s && (s != sbuf)) {
		FREE(s, M_TEMP);
	}
	if (acl) {
		kauth_acl_free(acl);
	}
	if (error && nvap->nva_acl) {
		kauth_acl_free(nvap->nva_acl);
		nvap->nva_acl = NULL;
	}
	FREE(nva_dummy, M_TEMP);
	return error;
}

  
                                                  
   
int
nfsm_chain_add_fattr4_f(struct nfsm_chain *nmc, struct vnode_attr *vap, struct nfsmount *nmp)
{
	int error = 0, attrbytes, i, isgroup;
	size_t slen, len;
	uint32_t *pattrbytes, val, acecount;;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN];
	char sbuf[64], *s;
	kauth_acl_t acl;
	gid_t gid;

	s = sbuf;
	slen = sizeof(sbuf);

	                                   
	nfs_vattr_set_bitmap(nmp, bitmap, vap);

	  
                             
                             
                                                        
    
	nfsm_chain_add_bitmap(error, nmc, bitmap, NFS_ATTR_BITMAP_LEN);
	attrbytes = 0;
	nfsm_chain_add_32(error, nmc, attrbytes);
	pattrbytes = (uint32_t*)(nmc->nmc_ptr - NFSX_UNSIGNED);

	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_SIZE)) {
		nfsm_chain_add_64(error, nmc, vap->va_data_size);
		attrbytes += 2 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_ACL)) {
		acl = vap->va_acl;
		if (!acl || (acl->acl_entrycount == KAUTH_FILESEC_NOACL)) {
			acecount = 0;
		} else {
			acecount = acl->acl_entrycount;
		}
		nfsm_chain_add_32(error, nmc, acecount);
		attrbytes += NFSX_UNSIGNED;
		for (i = 0; !error && (i < (int)acecount); i++) {
			val = (acl->acl_ace[i].ace_flags & KAUTH_ACE_KINDMASK);
			val = nfs4_ace_vfstype_to_nfstype(val, &error);
			nfsm_chain_add_32(error, nmc, val);
			val = nfs4_ace_vfsflags_to_nfsflags(acl->acl_ace[i].ace_flags);
			isgroup = (kauth_cred_guid2gid(&acl->acl_ace[i].ace_applicable, &gid) == 0);
			val |= (isgroup) ? NFS_ACE_IDENTIFIER_GROUP : 0;
			nfsm_chain_add_32(error, nmc, val);
			val = nfs4_ace_vfsrights_to_nfsmask(acl->acl_ace[i].ace_rights);
			nfsm_chain_add_32(error, nmc, val);
			len = slen;
			error = nfs4_guid2id(&acl->acl_ace[i].ace_applicable, s, &len, isgroup);
			if (error == ENOSPC) {
				if (s != sbuf) {
					FREE(s, M_TEMP);
					s = sbuf;
				}
				len += 8;
				MALLOC(s, char*, len, M_TEMP, M_WAITOK);
				if (s) {
					slen = len;
					error = nfs4_guid2id(&acl->acl_ace[i].ace_applicable, s, &len, isgroup);
				} else {
					error = ENOMEM;
				}
			}
			nfsm_chain_add_name(error, nmc, s, len, nmp);
			attrbytes += 4 * NFSX_UNSIGNED + nfsm_rndup(len);
		}
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_ARCHIVE)) {
		nfsm_chain_add_32(error, nmc, (vap->va_flags & SF_ARCHIVED) ? 1 : 0);
		attrbytes += NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_HIDDEN)) {
		nfsm_chain_add_32(error, nmc, (vap->va_flags & UF_HIDDEN) ? 1 : 0);
		attrbytes += NFSX_UNSIGNED;
	}
	                                               
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MODE)) {
		nfsm_chain_add_32(error, nmc, vap->va_mode);
		attrbytes += NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_OWNER)) {
		nfsmout_if(error);
		                                                   
		if (!VATTR_IS_ACTIVE(vap, va_uuuid)) {
			error = kauth_cred_uid2guid(vap->va_uid, &vap->va_uuuid);
			nfsmout_if(error);
		}
		len = slen;
		error = nfs4_guid2id(&vap->va_uuuid, s, &len, 0);
		if (error == ENOSPC) {
			if (s != sbuf) {
				FREE(s, M_TEMP);
				s = sbuf;
			}
			len += 8;
			MALLOC(s, char*, len, M_TEMP, M_WAITOK);
			if (s) {
				slen = len;
				error = nfs4_guid2id(&vap->va_uuuid, s, &len, 0);
			} else {
				error = ENOMEM;
			}
		}
		nfsm_chain_add_name(error, nmc, s, len, nmp);
		attrbytes += NFSX_UNSIGNED + nfsm_rndup(len);
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_OWNER_GROUP)) {
		nfsmout_if(error);
		                                                   
		if (!VATTR_IS_ACTIVE(vap, va_guuid)) {
			error = kauth_cred_gid2guid(vap->va_gid, &vap->va_guuid);
			nfsmout_if(error);
		}
		len = slen;
		error = nfs4_guid2id(&vap->va_guuid, s, &len, 1);
		if (error == ENOSPC) {
			if (s != sbuf) {
				FREE(s, M_TEMP);
				s = sbuf;
			}
			len += 8;
			MALLOC(s, char*, len, M_TEMP, M_WAITOK);
			if (s) {
				slen = len;
				error = nfs4_guid2id(&vap->va_guuid, s, &len, 1);
			} else {
				error = ENOMEM;
			}
		}
		nfsm_chain_add_name(error, nmc, s, len, nmp);
		attrbytes += NFSX_UNSIGNED + nfsm_rndup(len);
	}
	                                           
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_ACCESS_SET)) {
		if (vap->va_vaflags & VA_UTIMES_NULL) {
			nfsm_chain_add_32(error, nmc, NFS4_TIME_SET_TO_SERVER);
			attrbytes += NFSX_UNSIGNED;
		} else {
			nfsm_chain_add_32(error, nmc, NFS4_TIME_SET_TO_CLIENT);
			nfsm_chain_add_64(error, nmc, vap->va_access_time.tv_sec);
			nfsm_chain_add_32(error, nmc, vap->va_access_time.tv_nsec);
			attrbytes += 4 * NFSX_UNSIGNED;
		}
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_BACKUP)) {
		nfsm_chain_add_64(error, nmc, vap->va_backup_time.tv_sec);
		nfsm_chain_add_32(error, nmc, vap->va_backup_time.tv_nsec);
		attrbytes += 3 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_CREATE)) {
		nfsm_chain_add_64(error, nmc, vap->va_create_time.tv_sec);
		nfsm_chain_add_32(error, nmc, vap->va_create_time.tv_nsec);
		attrbytes += 3 * NFSX_UNSIGNED;
	}
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_TIME_MODIFY_SET)) {
		if (vap->va_vaflags & VA_UTIMES_NULL) {
			nfsm_chain_add_32(error, nmc, NFS4_TIME_SET_TO_SERVER);
			attrbytes += NFSX_UNSIGNED;
		} else {
			nfsm_chain_add_32(error, nmc, NFS4_TIME_SET_TO_CLIENT);
			nfsm_chain_add_64(error, nmc, vap->va_modify_time.tv_sec);
			nfsm_chain_add_32(error, nmc, vap->va_modify_time.tv_nsec);
			attrbytes += 4 * NFSX_UNSIGNED;
		}
	}
	nfsmout_if(error);
	                                        
	*pattrbytes = txdr_unsigned(attrbytes);
nfsmout:
	if (s && (s != sbuf)) {
		FREE(s, M_TEMP);
	}
	return error;
}
#endif                  

  
                                                                           
                            
   
void
nfs_need_recover(struct nfsmount *nmp, int error)
{
	int wake = !(nmp->nm_state & NFSSTA_RECOVER);

	nmp->nm_state |= NFSSTA_RECOVER;
	if ((error == NFSERR_ADMIN_REVOKED) ||
	    (error == NFSERR_EXPIRED) ||
	    (error == NFSERR_STALE_CLIENTID)) {
		nmp->nm_state |= NFSSTA_RECOVER_EXPIRED;
	}
	if (wake) {
		nfs_mount_sock_thread_wake(nmp);
	}
}

#if CONFIG_NFS4
  
                                                          
                                                   
  
                                                           
                                                             
                        
   
static void
nfs4_expired_check_delegation(nfsnode_t np, vfs_context_t ctx)
{
	struct nfsmount *nmp = NFSTONMP(np);
	struct nfs_open_file *nofp;
	int drop = 1;

	if ((np->n_flag & NREVOKE) || !(np->n_openflags & N_DELEG_MASK)) {
		return;
	}

	lck_mtx_lock(&np->n_openlock);

	TAILQ_FOREACH(nofp, &np->n_opens, nof_link) {
		if (!nofp->nof_opencnt) {
			continue;
		}
		if (nofp->nof_flags & NFS_OPEN_FILE_LOST) {
			continue;
		}
		if (nofp->nof_flags & NFS_OPEN_FILE_REOPEN) {
			continue;
		}
		                                                                
		                                                                       
		NP(nofp->nof_np, "nfs4_expired_check_delegation: !drop: opencnt %d flags 0x%x access %d %d mmap %d %d",
		    nofp->nof_opencnt, nofp->nof_flags,
		    nofp->nof_access, nofp->nof_deny,
		    nofp->nof_mmap_access, nofp->nof_mmap_deny);
		drop = 0;
		break;
	}

	if (drop) {
		                               
		if (np->n_dreturn.tqe_next != NFSNOLIST) {
			                                                      
			lck_mtx_lock(&nmp->nm_lock);
			if (np->n_dreturn.tqe_next != NFSNOLIST) {
				TAILQ_REMOVE(&nmp->nm_dreturnq, np, n_dreturn);
				np->n_dreturn.tqe_next = NFSNOLIST;
			}
			lck_mtx_unlock(&nmp->nm_lock);
		}
		if (np->n_openflags & N_DELEG_MASK) {
			np->n_openflags &= ~N_DELEG_MASK;
			lck_mtx_lock(&nmp->nm_lock);
			if (np->n_dlink.tqe_next != NFSNOLIST) {
				TAILQ_REMOVE(&nmp->nm_delegations, np, n_dlink);
				np->n_dlink.tqe_next = NFSNOLIST;
			}
			lck_mtx_unlock(&nmp->nm_lock);
			nfs4_delegreturn_rpc(nmp, np->n_fhp, np->n_fhsize, &np->n_dstateid,
			    0, vfs_context_thread(ctx), vfs_context_ucred(ctx));
		}
	}

	lck_mtx_unlock(&np->n_openlock);
}
#endif                 

  
                                  
  
                                                                 
   
void
nfs_recover(struct nfsmount *nmp)
{
	struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
	int error, lost, reopen;
	struct nfs_open_owner *noop;
	struct nfs_open_file *nofp;
	struct nfs_file_lock *nflp, *nextnflp;
	struct nfs_lock_owner *nlop;
	thread_t thd = current_thread();
#if CONFIG_NFS4
	nfsnode_t np, nextnp;
#endif
	struct timeval now;

restart:
	error = 0;
	lck_mtx_lock(&nmp->nm_lock);
	  
                                                          
                                                      
    
	do {
		if ((error = nfs_sigintr(nmp, NULL, NULL, 1))) {
			break;
		}
		if (!(nmp->nm_sockflags & NMSOCK_READY)) {
			error = EPIPE;
		}
		if (nmp->nm_state & (NFSSTA_FORCE | NFSSTA_DEAD)) {
			error = ENXIO;
		}
		if (nmp->nm_sockflags & NMSOCK_UNMOUNT) {
			error = ENXIO;
		}
		if (error) {
			break;
		}
		if (nmp->nm_stateinuse) {
			msleep(&nmp->nm_stateinuse, &nmp->nm_lock, (PZERO - 1), "nfsrecoverstartwait", &ts);
		}
	} while (nmp->nm_stateinuse);
	if (error) {
		if (error == EPIPE) {
			printf("nfs recovery reconnecting for %s, 0x%x\n", vfs_statfs(nmp->nm_mountp)->f_mntfromname, nmp->nm_stategenid);
		} else {
			printf("nfs recovery aborted for %s, 0x%x\n", vfs_statfs(nmp->nm_mountp)->f_mntfromname, nmp->nm_stategenid);
		}
		lck_mtx_unlock(&nmp->nm_lock);
		return;
	}

	microuptime(&now);
	if (now.tv_sec == nmp->nm_recover_start) {
		printf("nfs recovery throttled for %s, 0x%x\n", vfs_statfs(nmp->nm_mountp)->f_mntfromname, nmp->nm_stategenid);
		lck_mtx_unlock(&nmp->nm_lock);
		tsleep(nfs_recover, (PZERO - 1), "nfsrecoverrestart", hz);
		goto restart;
	}
	nmp->nm_recover_start = now.tv_sec;
	if (++nmp->nm_stategenid == 0) {
		++nmp->nm_stategenid;
	}
	printf("nfs recovery started for %s, 0x%x\n", vfs_statfs(nmp->nm_mountp)->f_mntfromname, nmp->nm_stategenid);
	lck_mtx_unlock(&nmp->nm_lock);

	                            
	TAILQ_FOREACH(noop, &nmp->nm_open_owners, noo_link) {
		                              
		TAILQ_FOREACH(nofp, &noop->noo_opens, nof_oolink) {
			if (!nofp->nof_access || (nofp->nof_flags & NFS_OPEN_FILE_LOST) || (nofp->nof_np->n_flag & NREVOKE)) {
				continue;
			}
			lost = reopen = 0;
			                                                      
			if (nmp->nm_vers < NFS_VER4) {
				goto reclaim_locks;
			}
#if CONFIG_NFS4
			if (nofp->nof_rw_drw) {
				error = nfs4_open_reclaim_rpc(nofp, NFS_OPEN_SHARE_ACCESS_BOTH, NFS_OPEN_SHARE_DENY_BOTH);
			}
			if (!error && nofp->nof_w_drw) {
				error = nfs4_open_reclaim_rpc(nofp, NFS_OPEN_SHARE_ACCESS_WRITE, NFS_OPEN_SHARE_DENY_BOTH);
			}
			if (!error && nofp->nof_r_drw) {
				error = nfs4_open_reclaim_rpc(nofp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_BOTH);
			}
			if (!error && nofp->nof_rw_dw) {
				error = nfs4_open_reclaim_rpc(nofp, NFS_OPEN_SHARE_ACCESS_BOTH, NFS_OPEN_SHARE_DENY_WRITE);
			}
			if (!error && nofp->nof_w_dw) {
				error = nfs4_open_reclaim_rpc(nofp, NFS_OPEN_SHARE_ACCESS_WRITE, NFS_OPEN_SHARE_DENY_WRITE);
			}
			if (!error && nofp->nof_r_dw) {
				error = nfs4_open_reclaim_rpc(nofp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_WRITE);
			}
			  
                                                                                  
      
			if (!error && nofp->nof_rw) {
				error = nfs4_open_reclaim_rpc(nofp, NFS_OPEN_SHARE_ACCESS_BOTH, NFS_OPEN_SHARE_DENY_NONE);
				if ((error == NFSERR_ADMIN_REVOKED) || (error == NFSERR_EXPIRED) || (error == NFSERR_NO_GRACE)) {
					reopen = error;
					error = 0;
				}
			}
			if (!error && !reopen && nofp->nof_w) {
				error = nfs4_open_reclaim_rpc(nofp, NFS_OPEN_SHARE_ACCESS_WRITE, NFS_OPEN_SHARE_DENY_NONE);
				if ((error == NFSERR_ADMIN_REVOKED) || (error == NFSERR_EXPIRED) || (error == NFSERR_NO_GRACE)) {
					reopen = error;
					error = 0;
				}
			}
			if (!error && !reopen && nofp->nof_r) {
				error = nfs4_open_reclaim_rpc(nofp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_NONE);
				if ((error == NFSERR_ADMIN_REVOKED) || (error == NFSERR_EXPIRED) || (error == NFSERR_NO_GRACE)) {
					reopen = error;
					error = 0;
				}
			}

			  
                                                                           
                                                                          
                                  
      
			if ((nofp->nof_d_rw_drw || nofp->nof_d_w_drw || nofp->nof_d_r_drw ||
			    nofp->nof_d_rw_dw || nofp->nof_d_w_dw || nofp->nof_d_r_dw ||
			    nofp->nof_d_rw || nofp->nof_d_w || nofp->nof_d_r) &&
			    (!nofp->nof_rw_drw && !nofp->nof_w_drw && !nofp->nof_r_drw &&
			    !nofp->nof_rw_dw && !nofp->nof_w_dw && !nofp->nof_r_dw &&
			    !nofp->nof_rw && !nofp->nof_w && !nofp->nof_r)) {
				if (!error && !nfs_open_state_set_busy(nofp->nof_np, NULL)) {
					error = nfs4_claim_delegated_state_for_node(nofp->nof_np, R_RECOVER);
					if (!error && (nofp->nof_flags & NFS_OPEN_FILE_REOPEN)) {
						reopen = EAGAIN;
					}
					nfs_open_state_clear_busy(nofp->nof_np);
					                                                                   
					if (nofp->nof_np->n_openflags & N_DELEG_RETURN) {
						nfs4_delegation_return(nofp->nof_np, R_RECOVER, thd, noop->noo_cred);
						if (!(nmp->nm_sockflags & NMSOCK_READY)) {
							error = ETIMEDOUT;                                      
						}
					}
				}
			}

			  
                                           
                                                                      
      
			if (error || reopen) {
				                       
				if ((error == ETIMEDOUT) || nfs_mount_state_error_should_restart(error)) {
					if (error == ETIMEDOUT) {
						nfs_need_reconnect(nmp);
					}
					tsleep(nfs_recover, (PZERO - 1), "nfsrecoverrestart", hz);
					printf("nfs recovery restarting for %s, 0x%x, error %d\n",
					    vfs_statfs(nmp->nm_mountp)->f_mntfromname, nmp->nm_stategenid, error);
					goto restart;
				}
				if (reopen && (nfs_check_for_locks(noop, nofp) == 0)) {
					                                         
					NP(nofp->nof_np, "nfs_recover: %d, need reopen for %d %p 0x%x", reopen,
					    kauth_cred_getuid(noop->noo_cred), nofp->nof_np, nofp->nof_np->n_flag);
					lck_mtx_lock(&nofp->nof_lock);
					nofp->nof_flags |= NFS_OPEN_FILE_REOPEN;
					lck_mtx_unlock(&nofp->nof_lock);
				} else {
					                          
					if (reopen) {
						NP(nofp->nof_np, "nfs_recover: %d, can't reopen because of locks %d %p", reopen,
						    kauth_cred_getuid(noop->noo_cred), nofp->nof_np);
					}
					lost = 1;
					error = 0;
					reopen = 0;
				}
			} else {
				                                                      
				lck_mtx_lock(&nofp->nof_lock);
				nofp->nof_flags &= ~NFS_OPEN_FILE_REOPEN;
				lck_mtx_unlock(&nofp->nof_lock);
			}
#endif                  
			  
                                                                        
                                                                     
      
reclaim_locks:
			TAILQ_FOREACH(nlop, &nofp->nof_np->n_lock_owners, nlo_link) {
				if (lost || reopen) {
					break;
				}
				if (nlop->nlo_open_owner != noop) {
					continue;
				}
				TAILQ_FOREACH_SAFE(nflp, &nlop->nlo_locks, nfl_lolink, nextnflp) {
					                                                                                
					if (nflp->nfl_flags & (NFS_FILE_LOCK_DEAD | NFS_FILE_LOCK_BLOCKED)) {
						continue;
					}
					                          
					if (nflp->nfl_flags & NFS_FILE_LOCK_DELEGATED) {
						continue;
					}
					error = nmp->nm_funcs->nf_setlock_rpc(nofp->nof_np, nofp, nflp, 1, R_RECOVER, thd, noop->noo_cred);
					if (error) {
						NP(nofp->nof_np, "nfs: lock reclaim (0x%llx, 0x%llx) %s %d",
						    nflp->nfl_start, nflp->nfl_end,
						    error ? "failed" : "succeeded", error);
					}
					if (!error) {
						continue;
					}
					                       
					if ((error == ETIMEDOUT) || nfs_mount_state_error_should_restart(error)) {
						if (error == ETIMEDOUT) {
							nfs_need_reconnect(nmp);
						}
						tsleep(nfs_recover, (PZERO - 1), "nfsrecoverrestart", hz);
						printf("nfs recovery restarting for %s, 0x%x, error %d\n",
						    vfs_statfs(nmp->nm_mountp)->f_mntfromname, nmp->nm_stategenid, error);
						goto restart;
					}
					                                             
					lost = 1;
					error = 0;
					break;
				}
			}
#if CONFIG_NFS4
			  
                                                                          
                                                                           
                                                             
     
                                                                       
                    
      
			if ((nofp->nof_np->n_openflags & N_DELEG_RETURN) ||
			    (reopen && (nofp->nof_np->n_openflags & N_DELEG_MASK))) {
				nfs4_delegation_return(nofp->nof_np, R_RECOVER, thd, noop->noo_cred);
				if (!(nmp->nm_sockflags & NMSOCK_READY)) {
					                                    
					tsleep(nfs_recover, (PZERO - 1), "nfsrecoverrestart", hz);
					printf("nfs recovery restarting for %s, 0x%x, error %d\n",
					    vfs_statfs(nmp->nm_mountp)->f_mntfromname, nmp->nm_stategenid, error);
					goto restart;
				}
			}
#endif
			if (lost) {
				                            
				NP(nofp->nof_np, "nfs_recover: state lost for %d %p 0x%x",
				    kauth_cred_getuid(noop->noo_cred), nofp->nof_np, nofp->nof_np->n_flag);
				nfs_revoke_open_state_for_node(nofp->nof_np);
			}
		}
	}

	if (!error) {
		                                                                              
		lck_mtx_lock(&nmp->nm_lock);
#if CONFIG_NFS4
		if ((nmp->nm_vers >= NFS_VER4) && (nmp->nm_state & NFSSTA_RECOVER_EXPIRED)) {
recheckdeleg:
			TAILQ_FOREACH_SAFE(np, &nmp->nm_delegations, n_dlink, nextnp) {
				lck_mtx_unlock(&nmp->nm_lock);
				nfs4_expired_check_delegation(np, vfs_context_kernel());
				lck_mtx_lock(&nmp->nm_lock);
				if (nextnp == NFSNOLIST) {
					goto recheckdeleg;
				}
			}
		}
#endif
		nmp->nm_state &= ~(NFSSTA_RECOVER | NFSSTA_RECOVER_EXPIRED);
		wakeup(&nmp->nm_state);
		printf("nfs recovery completed for %s, 0x%x\n",
		    vfs_statfs(nmp->nm_mountp)->f_mntfromname, nmp->nm_stategenid);
		lck_mtx_unlock(&nmp->nm_lock);
	} else {
		printf("nfs recovery failed for %s, 0x%x, error %d\n",
		    vfs_statfs(nmp->nm_mountp)->f_mntfromname, nmp->nm_stategenid, error);
	}
}

#endif                        
