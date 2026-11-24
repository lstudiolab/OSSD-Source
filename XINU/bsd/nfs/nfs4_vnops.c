/*
 * Copyright (c) 2006-2020 Apple Inc. All rights reserved.
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
#if CONFIG_NFS_CLIENT

  
                                   
   
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/resourcevar.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/mount_internal.h>
#include <sys/malloc.h>
#include <sys/kpi_mbuf.h>
#include <sys/conf.h>
#include <sys/vnode_internal.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/lockf.h>
#include <sys/ubc_internal.h>
#include <sys/attr.h>
#include <sys/signalvar.h>
#include <sys/uio_internal.h>
#include <sys/xattr.h>
#include <sys/paths.h>

#include <vfs/vfs_support.h>

#include <sys/vm.h>

#include <sys/time.h>
#include <kern/clock.h>
#include <libkern/OSAtomic.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/specfs/specdev.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <nfs/nfs_gss.h>
#include <nfs/nfsmount.h>
#include <nfs/nfs_lock.h>
#include <nfs/xdr_subs.h>
#include <nfs/nfsm_subs.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <vm/vm_kern.h>

#include <kern/task.h>
#include <kern/sched_prim.h>

#if CONFIG_NFS4
int
nfs4_access_rpc(nfsnode_t np, u_int32_t *access, int rpcflags, vfs_context_t ctx)
{
	int error = 0, lockerror = ENOENT, status, numops, slot;
	u_int64_t xid;
	struct nfsm_chain nmreq, nmrep;
	struct timeval now;
	uint32_t access_result = 0, supported = 0, missing;
	struct nfsmount *nmp = NFSTONMP(np);
	int nfsvers = nmp->nm_vers;
	uid_t uid;
	struct nfsreq_secinfo_args si;

	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return 0;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                         
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 17 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "access", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_ACCESS);
	nfsm_chain_add_32(error, &nmreq, *access);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(np, NULL, &nmreq, NFSPROC4_COMPOUND,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx),
	    &si, rpcflags, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_ACCESS);
	nfsm_chain_get_32(error, &nmrep, supported);
	nfsm_chain_get_32(error, &nmrep, access_result);
	nfsmout_if(error);
	if ((missing = (*access & ~supported))) {
		                                                
		if (missing & NFS_ACCESS_DELETE) {
			  
                                                   
                                                  
                                                    
                                   
      
			access_result |= NFS_ACCESS_DELETE;
		}
	}
	                                                                                  
	if (nfs_access_dotzfs) {
		vnode_t dvp = NULLVP;
		if (np->n_flag & NISDOTZFSCHILD) {                                                 
			access_result |= (NFS_ACCESS_MODIFY | NFS_ACCESS_EXTEND | NFS_ACCESS_DELETE);
		} else if (((dvp = vnode_getparent(NFSTOV(np))) != NULLVP) && (VTONFS(dvp)->n_flag & NISDOTZFSCHILD)) {
			access_result |= NFS_ACCESS_DELETE;                                          
		}
		if (dvp != NULLVP) {
			vnode_put(dvp);
		}
	}
	                                                                              
	if (nfs_access_delete && (*access & NFS_ACCESS_DELETE) && !(access_result & NFS_ACCESS_DELETE)) {
		access_result |= NFS_ACCESS_DELETE;
	}
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, np, nfsvers, &xid);
	nfsmout_if(error);

	if (nfs_mount_gone(nmp)) {
		error = ENXIO;
	}
	nfsmout_if(error);

	if (auth_is_kerberized(np->n_auth) || auth_is_kerberized(nmp->nm_auth)) {
		uid = nfs_cred_getasid2uid(vfs_context_ucred(ctx));
	} else {
		uid = kauth_cred_getuid(vfs_context_ucred(ctx));
	}
	slot = nfs_node_access_slot(np, uid, 1);
	np->n_accessuid[slot] = uid;
	microuptime(&now);
	np->n_accessstamp[slot] = now.tv_sec;
	np->n_access[slot] = access_result;

	                                                     
	*access = np->n_access[slot];
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

int
nfs4_getattr_rpc(
	nfsnode_t np,
	mount_t mp,
	u_char *fhp,
	size_t fhsize,
	int flags,
	vfs_context_t ctx,
	struct nfs_vattr *nvap,
	u_int64_t *xidp)
{
	struct nfsmount *nmp = mp ? VFSTONFS(mp) : NFSTONMP(np);
	int error = 0, status, nfsvers, numops, rpcflags = 0, acls;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN];
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq_secinfo_args si;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	acls = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_ACL);

	if (np && (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL)) {
		nfs4_default_attrs_for_referral_trigger(VTONFS(np->n_parent), NULL, 0, nvap, NULL);
		return 0;
	}

	if (flags & NGA_MONITOR) {                                            
		rpcflags = R_RECOVER;
	}

	if (flags & NGA_SOFT) {                                                
		rpcflags |= R_SOFT;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                 
	numops = 2;
	nfsm_chain_build_alloc_init(error, &nmreq, 15 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "getattr", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, fhp, fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, bitmap);
	if ((flags & NGA_ACL) && acls) {
		NFS_BITMAP_SET(bitmap, NFS_FATTR_ACL);
	}
	nfsm_chain_add_bitmap_supported(error, &nmreq, bitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(np, mp, &nmreq, NFSPROC4_COMPOUND,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx),
	    NULL, rpcflags, &nmrep, xidp, &status);

	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsmout_if(error);
	error = nfs4_parsefattr(&nmrep, NULL, nvap, NULL, NULL, NULL);
	nfsmout_if(error);
	if ((flags & NGA_ACL) && acls && !NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_ACL)) {
		                                                                       
		NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_ACL);
		nvap->nva_acl = NULL;
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

int
nfs4_readlink_rpc(nfsnode_t np, char *buf, size_t *buflenp, vfs_context_t ctx)
{
	struct nfsmount *nmp;
	int error = 0, lockerror = ENOENT, status, numops;
	size_t len = 0;
	u_int64_t xid;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}
	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                           
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 16 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "readlink", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, NFS_VER4, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, np);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_READLINK);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request(np, NULL, &nmreq, NFSPROC4_COMPOUND, ctx, &si, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, np, NFS_VER4, &xid);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_READLINK);
	nfsm_chain_get_32(error, &nmrep, len);
	nfsmout_if(error);
	if (len >= *buflenp) {
		if (np->n_size && (np->n_size < *buflenp)) {
			len = np->n_size;
		} else {
			len = *buflenp - 1;
		}
	}
	nfsm_chain_get_opaque(error, &nmrep, len, buf);
	if (!error) {
		*buflenp = len;
	}
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

int
nfs4_read_rpc_async(
	nfsnode_t np,
	off_t offset,
	size_t len,
	thread_t thd,
	kauth_cred_t cred,
	struct nfsreq_cbinfo *cb,
	struct nfsreq **reqp)
{
	struct nfsmount *nmp;
	int error = 0, nfsvers, numops;
	nfs_stateid stateid;
	struct nfsm_chain nmreq;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);

	              
	numops = 2;
	nfsm_chain_build_alloc_init(error, &nmreq, 22 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "read", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_READ);
	nfs_get_stateid(np, thd, cred, &stateid);
	nfsm_chain_add_stateid(error, &nmreq, &stateid);
	nfsm_chain_add_64(error, &nmreq, offset);
	nfsm_chain_add_32(error, &nmreq, len);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request_async(np, NULL, &nmreq, NFSPROC4_COMPOUND, thd, cred, &si, 0, cb, reqp);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	return error;
}

int
nfs4_read_rpc_async_finish(
	nfsnode_t np,
	struct nfsreq *req,
	uio_t uio,
	size_t *lenp,
	int *eofp)
{
	struct nfsmount *nmp;
	int error = 0, lockerror, nfsvers, numops, status, eof = 0;
	size_t retlen = 0;
	u_int64_t xid;
	struct nfsm_chain nmrep;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		nfs_request_async_cancel(req);
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	nfsm_chain_null(&nmrep);

	error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	if (error == EINPROGRESS) {                              
		return error;
	}

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_READ);
	nfsm_chain_get_32(error, &nmrep, eof);
	nfsm_chain_get_32(error, &nmrep, retlen);
	if (!error) {
		*lenp = MIN(retlen, *lenp);
		error = nfsm_chain_get_uio(&nmrep, *lenp, uio);
	}
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	if (eofp) {
		if (!eof && !retlen) {
			eof = 1;
		}
		*eofp = eof;
	}
	nfsm_chain_cleanup(&nmrep);
	if (np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR) {
		microuptime(&np->n_lastio);
	}
	return error;
}

int
nfs4_write_rpc_async(
	nfsnode_t np,
	uio_t uio,
	size_t len,
	thread_t thd,
	kauth_cred_t cred,
	int iomode,
	struct nfsreq_cbinfo *cb,
	struct nfsreq **reqp)
{
	struct nfsmount *nmp;
	mount_t mp;
	int error = 0, nfsvers, numops;
	nfs_stateid stateid;
	struct nfsm_chain nmreq;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	                                                                
	if ((iomode != NFS_WRITE_UNSTABLE) && nfs_allow_async &&
	    ((mp = NFSTOMP(np))) && (vfs_flags(mp) & MNT_ASYNC)) {
		iomode = NFS_WRITE_UNSTABLE;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);

	                        
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 25 * NFSX_UNSIGNED + len);
	nfsm_chain_add_compound_header(error, &nmreq, "write", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_WRITE);
	nfs_get_stateid(np, thd, cred, &stateid);
	nfsm_chain_add_stateid(error, &nmreq, &stateid);
	nfsm_chain_add_64(error, &nmreq, uio_offset(uio));
	nfsm_chain_add_32(error, &nmreq, iomode);
	nfsm_chain_add_32(error, &nmreq, len);
	if (!error) {
		error = nfsm_chain_add_uio(&nmreq, uio, len);
	}
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs4_getattr_write_bitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);

	error = nfs_request_async(np, NULL, &nmreq, NFSPROC4_COMPOUND, thd, cred, &si, 0, cb, reqp);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	return error;
}

int
nfs4_write_rpc_async_finish(
	nfsnode_t np,
	struct nfsreq *req,
	int *iomodep,
	size_t *rlenp,
	uint64_t *wverfp)
{
	struct nfsmount *nmp;
	int error = 0, lockerror = ENOENT, nfsvers, numops, status;
	int committed = NFS_WRITE_FILESYNC;
	size_t rlen = 0;
	u_int64_t xid, wverf;
	mount_t mp;
	struct nfsm_chain nmrep;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		nfs_request_async_cancel(req);
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	nfsm_chain_null(&nmrep);

	error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	if (error == EINPROGRESS) {                              
		return error;
	}
	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		error = ENXIO;
	}
	if (!error && (lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_WRITE);
	nfsm_chain_get_32(error, &nmrep, rlen);
	nfsmout_if(error);
	*rlenp = rlen;
	if (rlen <= 0) {
		error = NFSERR_IO;
	}
	nfsm_chain_get_32(error, &nmrep, committed);
	nfsm_chain_get_64(error, &nmrep, wverf);
	nfsmout_if(error);
	if (wverfp) {
		*wverfp = wverf;
	}
	lck_mtx_lock(&nmp->nm_lock);
	if (!(nmp->nm_state & NFSSTA_HASWRITEVERF)) {
		nmp->nm_verf = wverf;
		nmp->nm_state |= NFSSTA_HASWRITEVERF;
	} else if (nmp->nm_verf != wverf) {
		nmp->nm_verf = wverf;
	}
	lck_mtx_unlock(&nmp->nm_lock);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);

	  
                                                                                                                           
                                                                                   
    
	np->n_vattr.nva_flags |= NFS_FFLAG_PARTIAL_WRITE;
	nfsm_chain_loadattr(error, &nmrep, np, nfsvers, &xid);
	np->n_vattr.nva_flags &= ~NFS_FFLAG_PARTIAL_WRITE;

nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsm_chain_cleanup(&nmrep);
	if ((committed != NFS_WRITE_FILESYNC) && nfs_allow_async &&
	    ((mp = NFSTOMP(np))) && (vfs_flags(mp) & MNT_ASYNC)) {
		committed = NFS_WRITE_FILESYNC;
	}
	*iomodep = committed;
	if (np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR) {
		microuptime(&np->n_lastio);
	}
	return error;
}

int
nfs4_remove_rpc(
	nfsnode_t dnp,
	char *name,
	int namelen,
	thread_t thd,
	kauth_cred_t cred)
{
	int error = 0, lockerror = ENOENT, remove_error = 0, status;
	struct nfsmount *nmp;
	int nfsvers, numops;
	u_int64_t xid;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if (dnp->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}
	NFSREQ_SECINFO_SET(&si, dnp, NULL, 0, NULL, 0);
restart:
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                         
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 17 * NFSX_UNSIGNED + namelen);
	nfsm_chain_add_compound_header(error, &nmreq, "remove", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_REMOVE);
	nfsm_chain_add_name(error, &nmreq, name, namelen, nmp);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, dnp);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);

	error = nfs_request2(dnp, NULL, &nmreq, NFSPROC4_COMPOUND, thd, cred, &si, 0, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_REMOVE);
	remove_error = error;
	nfsm_chain_check_change_info(error, &nmrep, dnp);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, dnp, nfsvers, &xid);
	if (error && !lockerror) {
		NATTRINVALIDATE(dnp);
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	if (!lockerror) {
		dnp->n_flag |= NMODIFIED;
		nfs_node_unlock(dnp);
	}
	if (error == NFSERR_GRACE) {
		tsleep(&nmp->nm_state, (PZERO - 1), "nfsgrace", 2 * hz);
		goto restart;
	}

	return remove_error;
}

int
nfs4_rename_rpc(
	nfsnode_t fdnp,
	char *fnameptr,
	int fnamelen,
	nfsnode_t tdnp,
	char *tnameptr,
	int tnamelen,
	vfs_context_t ctx)
{
	int error = 0, lockerror = ENOENT, status, nfsvers, numops;
	struct nfsmount *nmp;
	u_int64_t xid, savedxid;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(fdnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if (fdnp->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}
	if (tdnp->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	NFSREQ_SECINFO_SET(&si, fdnp, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                                                                                
	numops = 7;
	nfsm_chain_build_alloc_init(error, &nmreq, 30 * NFSX_UNSIGNED + fnamelen + tnamelen);
	nfsm_chain_add_compound_header(error, &nmreq, "rename", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, fdnp->n_fhp, fdnp->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_SAVEFH);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, tdnp->n_fhp, tdnp->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_RENAME);
	nfsm_chain_add_name(error, &nmreq, fnameptr, fnamelen, nmp);
	nfsm_chain_add_name(error, &nmreq, tnameptr, tnamelen, nmp);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, tdnp);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_RESTOREFH);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, fdnp);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);

	error = nfs_request(fdnp, NULL, &nmreq, NFSPROC4_COMPOUND, ctx, &si, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock2(fdnp, tdnp))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_SAVEFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_RENAME);
	nfsm_chain_check_change_info(error, &nmrep, fdnp);
	nfsm_chain_check_change_info(error, &nmrep, tdnp);
	                                                                         
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	savedxid = xid;
	nfsm_chain_loadattr(error, &nmrep, tdnp, nfsvers, &xid);
	if (error && !lockerror) {
		NATTRINVALIDATE(tdnp);
	}
	nfsm_chain_op_check(error, &nmrep, NFS_OP_RESTOREFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	xid = savedxid;
	nfsm_chain_loadattr(error, &nmrep, fdnp, nfsvers, &xid);
	if (error && !lockerror) {
		NATTRINVALIDATE(fdnp);
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	if (!lockerror) {
		fdnp->n_flag |= NMODIFIED;
		tdnp->n_flag |= NMODIFIED;
		nfs_node_unlock2(fdnp, tdnp);
	}
	return error;
}

  
                      
   
int
nfs4_readdir_rpc(nfsnode_t dnp, struct nfsbuf *bp, vfs_context_t ctx)
{
	struct nfsmount *nmp;
	int error = 0, lockerror, nfsvers, namedattr, rdirplus, bigcookies, numops;
	int i, status, more_entries = 1, eof, bp_dropped = 0;
	uint16_t namlen, reclen;
	uint32_t nmreaddirsize, nmrsize;
	uint32_t namlen32, skiplen, fhlen, xlen, attrlen;
	uint64_t padlen, cookie, lastcookie, xid, savedxid, space_free, space_needed;
	struct nfsm_chain nmreq, nmrep, nmrepsave;
	fhandle_t *fh;
	struct nfs_vattr *nvattr, *nvattrp;
	struct nfs_dir_buf_header *ndbhp;
	struct direntry *dp;
	char *padstart;
	const char *tag;
	uint32_t entry_attrs[NFS_ATTR_BITMAP_LEN];
	struct timeval now;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	nmreaddirsize = nmp->nm_readdirsize;
	nmrsize = nmp->nm_rsize;
	bigcookies = nmp->nm_state & NFSSTA_BIGCOOKIES;
	namedattr = (dnp->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR) ? 1 : 0;
	rdirplus = (NMFLAG(nmp, RDIRPLUS) || namedattr) ? 1 : 0;
	if (dnp->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}
	NFSREQ_SECINFO_SET(&si, dnp, NULL, 0, NULL, 0);

	  
                                         
                                                  
                                                         
    
	if (rdirplus) {
		tag = "readdirplus";
		NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, entry_attrs);
		NFS_BITMAP_SET(entry_attrs, NFS_FATTR_FILEHANDLE);
	} else {
		tag = "readdir";
		NFS_CLEAR_ATTRIBUTES(entry_attrs);
		NFS_BITMAP_SET(entry_attrs, NFS_FATTR_TYPE);
		NFS_BITMAP_SET(entry_attrs, NFS_FATTR_FILEID);
		NFS_BITMAP_SET(entry_attrs, NFS_FATTR_MOUNTED_ON_FILEID);
	}
	NFS_BITMAP_SET(entry_attrs, NFS_FATTR_RDATTR_ERROR);

	                                               
	if ((lockerror = nfs_node_lock(dnp))) {
		return lockerror;
	}

	fh = zalloc(nfs_fhandle_zone);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);

	                                                              
	ndbhp = (struct nfs_dir_buf_header*)bp->nb_data;
	dp = NFS_DIR_BUF_FIRST_DIRENTRY(bp);
	if (ndbhp->ndbh_count) {
		for (i = 0; i < ndbhp->ndbh_count - 1; i++) {
			dp = NFS_DIRENTRY_NEXT(dp);
		}
		cookie = dp->d_seekoff;
		dp = NFS_DIRENTRY_NEXT(dp);
	} else {
		cookie = bp->nb_lblkno;
		                                      
		OSAddAtomic64(1, &nfsstats.readdir_bios);
	}
	lastcookie = cookie;

	  
                                                                     
                                                                 
                                           
    
	if (((bp->nb_lblkno == 0) && (ndbhp->ndbh_count == 0)) &&
	    !(dnp->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR)) {
		fh->fh_len = 0;
		fhlen = rdirplus ? fh->fh_len + 1 : 0;
		xlen = rdirplus ? (fhlen + sizeof(time_t)) : 0;
		         
		namlen = 1;
		reclen = NFS_DIRENTRY_LEN_16(namlen + xlen);
		if (xlen) {
			bzero(&dp->d_name[namlen + 1], xlen);
		}
		dp->d_namlen = namlen;
		strlcpy(dp->d_name, ".", namlen + 1);
		dp->d_fileno = dnp->n_vattr.nva_fileid;
		dp->d_type = DT_DIR;
		dp->d_reclen = reclen;
		dp->d_seekoff = 1;
		padstart = dp->d_name + dp->d_namlen + 1 + xlen;
		dp = NFS_DIRENTRY_NEXT(dp);
		padlen = (char*)dp - padstart;
		if (padlen > 0) {
			bzero(padstart, padlen);
		}
		if (rdirplus) {                          
			bzero(NFS_DIR_BUF_NVATTR(bp, 0), sizeof(struct nfs_vattr));
		}

		          
		namlen = 2;
		reclen = NFS_DIRENTRY_LEN_16(namlen + xlen);
		if (xlen) {
			bzero(&dp->d_name[namlen + 1], xlen);
		}
		dp->d_namlen = namlen;
		strlcpy(dp->d_name, "..", namlen + 1);
		if (dnp->n_parent) {
			dp->d_fileno = VTONFS(dnp->n_parent)->n_vattr.nva_fileid;
		} else {
			dp->d_fileno = dnp->n_vattr.nva_fileid;
		}
		dp->d_type = DT_DIR;
		dp->d_reclen = reclen;
		dp->d_seekoff = 2;
		padstart = dp->d_name + dp->d_namlen + 1 + xlen;
		dp = NFS_DIRENTRY_NEXT(dp);
		padlen = (char*)dp - padstart;
		if (padlen > 0) {
			bzero(padstart, padlen);
		}
		if (rdirplus) {                          
			bzero(NFS_DIR_BUF_NVATTR(bp, 1), sizeof(struct nfs_vattr));
		}

		ndbhp->ndbh_entry_end = (char*)dp - bp->nb_data;
		ndbhp->ndbh_count = 2;
	}

	  
                                                                     
                                                                      
                                  
    
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);
	while (nfs_dir_buf_freespace(bp, rdirplus) && !(ndbhp->ndbh_flags & NDB_FULL)) {
		                          
		numops = 3;
		nfsm_chain_build_alloc_init(error, &nmreq, 26 * NFSX_UNSIGNED);
		nfsm_chain_add_compound_header(error, &nmreq, tag, nmp->nm_minor_vers, numops);
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
		nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
		nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, dnp);
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_READDIR);
		nfsm_chain_add_64(error, &nmreq, (cookie <= 2) ? 0 : cookie);
		nfsm_chain_add_64(error, &nmreq, dnp->n_cookieverf);
		nfsm_chain_add_32(error, &nmreq, nmreaddirsize);
		nfsm_chain_add_32(error, &nmreq, nmrsize);
		nfsm_chain_add_bitmap_supported(error, &nmreq, entry_attrs, nmp, dnp);
		nfsm_chain_build_done(error, &nmreq);
		nfsm_assert(error, (numops == 0), EPROTO);
		nfs_node_unlock(dnp);
		nfsmout_if(error);
		error = nfs_request(dnp, NULL, &nmreq, NFSPROC4_COMPOUND, ctx, &si, &nmrep, &xid, &status);

		if ((lockerror = nfs_node_lock(dnp))) {
			error = lockerror;
		}

		savedxid = xid;
		nfsm_chain_skip_tag(error, &nmrep);
		nfsm_chain_get_32(error, &nmrep, numops);
		nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
		nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
		nfsm_chain_loadattr(error, &nmrep, dnp, nfsvers, &xid);
		nfsm_chain_op_check(error, &nmrep, NFS_OP_READDIR);
		nfsm_chain_get_64(error, &nmrep, dnp->n_cookieverf);
		nfsm_chain_get_32(error, &nmrep, more_entries);

		if (!lockerror) {
			nfs_node_unlock(dnp);
			lockerror = ENOENT;
		}
		nfsmout_if(error);

		if (rdirplus) {
			microuptime(&now);
			if (lastcookie == 0) {
				dnp->n_rdirplusstamp_sof = now.tv_sec;
				dnp->n_rdirplusstamp_eof = 0;
			}
		}

		                                                           
		while (more_entries) {
			                                
			nfsm_chain_get_64(error, &nmrep, cookie);
			nfsm_chain_get_32(error, &nmrep, namlen32);
			if (namlen32 > UINT16_MAX) {
				error = EBADRPC;
				goto nfsmout;
			}
			namlen = (uint16_t)namlen32;
			nfsmout_if(error);
			if (!bigcookies && (cookie >> 32) && (nmp == NFSTONMP(dnp))) {
				                                                   
				lck_mtx_lock(&nmp->nm_lock);
				nmp->nm_state |= NFSSTA_BIGCOOKIES;
				lck_mtx_unlock(&nmp->nm_lock);
				bigcookies = 1;
			}
			                                                           
			if (namlen <= 0) {
				error = EBADRPC;
				goto nfsmout;
			}
			if (namlen > (sizeof(dp->d_name) - 1)) {
				skiplen = namlen - sizeof(dp->d_name) + 1;
				namlen = sizeof(dp->d_name) - 1;
			} else {
				skiplen = 0;
			}
			                                               
			fhlen = rdirplus ? (1 + dnp->n_fhsize) : 0;
			xlen = rdirplus ? (fhlen + sizeof(time_t)) : 0;
			attrlen = rdirplus ? sizeof(struct nfs_vattr) : 0;
			reclen = NFS_DIRENTRY_LEN_16(namlen + xlen);
			space_needed = reclen + attrlen;
			space_free = nfs_dir_buf_freespace(bp, rdirplus);
			if (space_needed > space_free) {
				  
                                               
                                             
                                             
                                            
                                         
       
nextbuffer:
				ndbhp->ndbh_flags |= NDB_FULL;
				nfs_buf_release(bp, 0);
				bp_dropped = 1;
				bp = NULL;
				error = nfs_buf_get(dnp, lastcookie, NFS_DIRBLKSIZ, vfs_context_thread(ctx), NBLK_READ, &bp);
				nfsmout_if(error);
				                       
				ndbhp = (struct nfs_dir_buf_header*)bp->nb_data;
				ndbhp->ndbh_flags = 0;
				ndbhp->ndbh_count = 0;
				ndbhp->ndbh_entry_end = sizeof(*ndbhp);
				ndbhp->ndbh_ncgen = dnp->n_ncgen;
				space_free = nfs_dir_buf_freespace(bp, rdirplus);
				dp = NFS_DIR_BUF_FIRST_DIRENTRY(bp);
				                                      
				OSAddAtomic64(1, &nfsstats.readdir_bios);
			}
			nmrepsave = nmrep;
			dp->d_fileno = cookie;                  
			dp->d_seekoff = cookie;
			dp->d_namlen = namlen;
			dp->d_reclen = reclen;
			dp->d_type = DT_UNKNOWN;
			nfsm_chain_get_opaque(error, &nmrep, namlen, dp->d_name);
			nfsmout_if(error);
			dp->d_name[namlen] = '\0';
			if (skiplen) {
				nfsm_chain_adv(error, &nmrep,
				    nfsm_rndup(namlen + skiplen) - nfsm_rndup(namlen));
			}
			nfsmout_if(error);
			nvattrp = rdirplus ? NFS_DIR_BUF_NVATTR(bp, ndbhp->ndbh_count) : nvattr;
			error = nfs4_parsefattr(&nmrep, NULL, nvattrp, fh, NULL, NULL);
			if (!error && NFS_BITMAP_ISSET(nvattrp->nva_bitmap, NFS_FATTR_ACL)) {
				                                             
				NFS_BITMAP_CLR(nvattrp->nva_bitmap, NFS_FATTR_ACL);
				if (nvattrp->nva_acl) {
					kauth_acl_free(nvattrp->nva_acl);
					nvattrp->nva_acl = NULL;
				}
			}
			if (error && NFS_BITMAP_ISSET(nvattrp->nva_bitmap, NFS_FATTR_RDATTR_ERROR)) {
				                                                                                   
				if ((error == NFSERR_MOVED) || (error == NFSERR_INVAL)) {
					                                                 
					nfs4_default_attrs_for_referral_trigger(dnp, dp->d_name, namlen, nvattrp, fh);
				}
				error = 0;
			}
			                                           
			nfsm_chain_get_32(error, &nmrep, more_entries);
			nfsmout_if(error);

			                                                         
			                                                       
			if (((dp->d_name[0] == '.') && ((namlen == 1) || ((namlen == 2) && (dp->d_name[1] == '.')))) ||
			    (namedattr && (namlen == 11) && (!strcmp(dp->d_name, "SUNWattr_ro") || !strcmp(dp->d_name, "SUNWattr_rw")))) {
				lastcookie = cookie;
				continue;
			}

			if (NFS_BITMAP_ISSET(nvattrp->nva_bitmap, NFS_FATTR_TYPE)) {
				dp->d_type = IFTODT(VTTOIF(nvattrp->nva_type));
			}
			if (NFS_BITMAP_ISSET(nvattrp->nva_bitmap, NFS_FATTR_FILEID)) {
				dp->d_fileno = nvattrp->nva_fileid;
			}
			if (rdirplus) {
				                                                          
				nvattrp->nva_fileid = savedxid;
				nvattrp->nva_flags |= NFS_FFLAG_FILEID_CONTAINS_XID;
				if (NFS_BITMAP_ISSET(nvattrp->nva_bitmap, NFS_FATTR_FILEHANDLE)) {
					fhlen = fh->fh_len + 1;
					xlen = fhlen + sizeof(time_t);
					reclen = NFS_DIRENTRY_LEN_16(namlen + xlen);
					space_needed = reclen + attrlen;
					if (space_needed > space_free) {
						                                                             
						nmrep = nmrepsave;
						goto nextbuffer;
					}
					                                          
					dp->d_name[dp->d_namlen + 1] = (unsigned char)fh->fh_len;                                                                               
					bcopy(fh->fh_data, &dp->d_name[dp->d_namlen + 2], fh->fh_len);
				} else {
					                                  
					fh->fh_len = 0;
					fhlen = fh->fh_len + 1;
					xlen = fhlen + sizeof(time_t);
					reclen = NFS_DIRENTRY_LEN_16(namlen + xlen);
					bzero(&dp->d_name[dp->d_namlen + 1], fhlen);
				}
				*(time_t*)(&dp->d_name[dp->d_namlen + 1 + fhlen]) = now.tv_sec;
				dp->d_reclen = reclen;
				nfs_rdirplus_update_node_attrs(dnp, dp, fh, nvattrp, &savedxid);
			}
			padstart = dp->d_name + dp->d_namlen + 1 + xlen;
			ndbhp->ndbh_count++;
			lastcookie = cookie;

			                                        
			dp = NFS_DIRENTRY_NEXT(dp);
			ndbhp->ndbh_entry_end = (char*)dp - bp->nb_data;
			                            
			padlen = (char*)dp - padstart;
			if (padlen > 0) {
				bzero(padstart, padlen);
			}
		}
		                                  
		nfsm_chain_get_32(error, &nmrep, eof);
		nfsmout_if(error);
		if (eof) {
			ndbhp->ndbh_flags |= (NDB_FULL | NDB_EOF);
			nfs_node_lock_force(dnp);
			dnp->n_eofcookie = lastcookie;
			if (rdirplus) {
				dnp->n_rdirplusstamp_eof = now.tv_sec;
			}
			nfs_node_unlock(dnp);
		} else {
			more_entries = 1;
		}
		if (bp_dropped) {
			nfs_buf_release(bp, 0);
			bp = NULL;
			break;
		}
		if ((lockerror = nfs_node_lock(dnp))) {
			error = lockerror;
		}
		nfsmout_if(error);
		nfsm_chain_cleanup(&nmrep);
		nfsm_chain_null(&nmreq);
	}
nfsmout:
	if (bp_dropped && bp) {
		nfs_buf_release(bp, 0);
	}
	if (!lockerror) {
		nfs_node_unlock(dnp);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	FREE(nvattr, M_TEMP);
	return bp_dropped ? NFSERR_DIRBUFDROPPED : error;
}

int
nfs4_lookup_rpc_async(
	nfsnode_t dnp,
	char *name,
	int namelen,
	vfs_context_t ctx,
	struct nfsreq **reqp)
{
	int error = 0, isdotdot = 0, nfsvers, numops;
	struct nfsm_chain nmreq;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN];
	struct nfsmount *nmp;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if (dnp->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	if ((name[0] == '.') && (name[1] == '.') && (namelen == 2)) {
		isdotdot = 1;
		NFSREQ_SECINFO_SET(&si, dnp, NULL, 0, NULL, 0);
	} else {
		NFSREQ_SECINFO_SET(&si, dnp, dnp->n_fhp, dnp->n_fhsize, name, namelen);
	}

	nfsm_chain_null(&nmreq);

	                                                 
	numops = 5;
	nfsm_chain_build_alloc_init(error, &nmreq, 20 * NFSX_UNSIGNED + namelen);
	nfsm_chain_add_compound_header(error, &nmreq, "lookup", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, dnp);
	numops--;
	if (isdotdot) {
		nfsm_chain_add_32(error, &nmreq, NFS_OP_LOOKUPP);
	} else {
		nfsm_chain_add_32(error, &nmreq, NFS_OP_LOOKUP);
		nfsm_chain_add_name(error, &nmreq, name, namelen, nmp);
	}
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETFH);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, bitmap);
	                                                                          
	if ((dnp->n_flag & NISDOTZFS) && !isdotdot) {
		NFS_BITMAP_CLR(bitmap, NFS_FATTR_NAMED_ATTR);
	}
	if ((dnp->n_flag & NISDOTZFSCHILD) && isdotdot) {
		NFS_BITMAP_CLR(bitmap, NFS_FATTR_NAMED_ATTR);
	}
	if (((namelen == 4) && (name[0] == '.') && (name[1] == 'z') && (name[2] == 'f') && (name[3] == 's'))) {
		NFS_BITMAP_CLR(bitmap, NFS_FATTR_NAMED_ATTR);
	}
	nfsm_chain_add_bitmap_supported(error, &nmreq, bitmap, nmp, NULL);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request_async(dnp, NULL, &nmreq, NFSPROC4_COMPOUND,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), &si, 0, NULL, reqp);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	return error;
}


int
nfs4_lookup_rpc_async_finish(
	nfsnode_t dnp,
	char *name,
	int namelen,
	vfs_context_t ctx,
	struct nfsreq *req,
	u_int64_t *xidp,
	fhandle_t *fhp,
	struct nfs_vattr *nvap)
{
	int error = 0, lockerror = ENOENT, status, nfsvers, numops, isdotdot = 0;
	uint32_t op = NFS_OP_LOOKUP;
	u_int64_t xid;
	struct nfsmount *nmp;
	struct nfsm_chain nmrep;

	nmp = NFSTONMP(dnp);
	if (nmp == NULL) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if ((name[0] == '.') && (name[1] == '.') && (namelen == 2)) {
		isdotdot = 1;
	}

	nfsm_chain_null(&nmrep);

	error = nfs_request_async_finish(req, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	if (xidp) {
		*xidp = xid;
	}
	nfsm_chain_loadattr(error, &nmrep, dnp, nfsvers, &xid);

	nfsm_chain_op_check(error, &nmrep, (isdotdot ? NFS_OP_LOOKUPP : NFS_OP_LOOKUP));
	nfsmout_if(error || !fhp || !nvap);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETFH);
	nfsm_chain_get_32(error, &nmrep, fhp->fh_len);
	if (error == 0 && fhp->fh_len > sizeof(fhp->fh_data)) {
		error = EBADRPC;
	}
	nfsmout_if(error);
	nfsm_chain_get_opaque(error, &nmrep, fhp->fh_len, fhp->fh_data);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	if ((error == NFSERR_MOVED) || (error == NFSERR_INVAL)) {
		                                                 
		nfs4_default_attrs_for_referral_trigger(dnp, name, namelen, nvap, fhp);
		error = 0;
	} else {
		nfsmout_if(error);
		error = nfs4_parsefattr(&nmrep, NULL, nvap, NULL, NULL, NULL);
	}
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(dnp);
	}
	nfsm_chain_cleanup(&nmrep);
	if (!error && (op == NFS_OP_LOOKUP) && (nmp->nm_state & NFSSTA_NEEDSECINFO)) {
		                                                            
		                                                           
		struct nfs_sec sec;

		sec.count = NX_MAX_SEC_FLAVORS;
		error = nfs4_secinfo_rpc(nmp, &req->r_secinfo, vfs_context_ucred(ctx), sec.flavors, &sec.count);
		                                                                            
		if (error == NFSERR_OP_ILLEGAL) {
			error = 0;
		}
		if (!error) {
			                                                              
			lck_mtx_lock(&nmp->nm_lock);
			if (sec.count) {
				nmp->nm_auth = sec.flavors[0];
			}
			nmp->nm_state &= ~NFSSTA_NEEDSECINFO;
			lck_mtx_unlock(&nmp->nm_lock);
		}
	}
	return error;
}

int
nfs4_commit_rpc(
	nfsnode_t np,
	uint64_t offset,
	uint64_t count,
	kauth_cred_t cred,
	uint64_t wverf)
{
	struct nfsmount *nmp;
	int error = 0, lockerror, status, nfsvers, numops;
	u_int64_t xid, newwverf;
	uint32_t count32;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	FSDBG(521, np, offset, count, nmp ? nmp->nm_state : 0);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}
	if (!(nmp->nm_state & NFSSTA_HASWRITEVERF)) {
		return 0;
	}
	nfsvers = nmp->nm_vers;
	count32 = count > UINT32_MAX ? 0 : (uint32_t)count;

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                         
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 19 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "commit", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_COMMIT);
	nfsm_chain_add_64(error, &nmreq, offset);
	nfsm_chain_add_32(error, &nmreq, count32);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(np, NULL, &nmreq, NFSPROC4_COMPOUND,
	    current_thread(), cred, &si, 0, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_COMMIT);
	nfsm_chain_get_64(error, &nmrep, newwverf);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, np, nfsvers, &xid);
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsmout_if(error);
	lck_mtx_lock(&nmp->nm_lock);
	if (nmp->nm_verf != newwverf) {
		nmp->nm_verf = newwverf;
	}
	if (wverf != newwverf) {
		error = NFSERR_STALEWRITEVERF;
	}
	lck_mtx_unlock(&nmp->nm_lock);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

int
nfs4_pathconf_rpc(
	nfsnode_t np,
	struct nfs_fsattr *nfsap,
	vfs_context_t ctx)
{
	u_int64_t xid;
	int error = 0, lockerror, status, nfsvers, numops;
	struct nfsm_chain nmreq, nmrep;
	struct nfsmount *nmp = NFSTONMP(np);
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN];
	struct nfs_vattr *nvattr;
	struct nfsreq_secinfo_args si;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	NVATTR_INIT(nvattr);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                                                
	                 
	numops = 2;
	nfsm_chain_build_alloc_init(error, &nmreq, 16 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "pathconf", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, bitmap);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_MAXLINK);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_MAXNAME);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_NO_TRUNC);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_CHOWN_RESTRICTED);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_CASE_INSENSITIVE);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_CASE_PRESERVING);
	nfsm_chain_add_bitmap_supported(error, &nmreq, bitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request(np, NULL, &nmreq, NFSPROC4_COMPOUND, ctx, &si, &nmrep, &xid, &status);

	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsmout_if(error);
	error = nfs4_parsefattr(&nmrep, nfsap, nvattr, NULL, NULL, NULL);
	nfsmout_if(error);
	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	if (!error) {
		nfs_loadattrcache(np, nvattr, &xid, 0);
	}
	if (!lockerror) {
		nfs_node_unlock(np);
	}
nfsmout:
	NVATTR_CLEANUP(nvattr);
	FREE(nvattr, M_TEMP);
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

int
nfs4_vnop_getattr(
	struct vnop_getattr_args     
                                                                 
                                                  
                                                              
                                                             
                                         *ap)
{
	struct vnode_attr *vap = ap->a_vap;
	struct nfsmount *nmp;
	struct nfs_vattr *nva;
	int error, acls, ngaflags;

	nmp = VTONMP(ap->a_vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	acls = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_ACL);

	ngaflags = NGA_CACHED;
	if (VATTR_IS_ACTIVE(vap, va_acl) && acls) {
		ngaflags |= NGA_ACL;
	}
	MALLOC(nva, struct nfs_vattr *, sizeof(*nva), M_TEMP, M_WAITOK);
	error = nfs_getattr(VTONFS(ap->a_vp), nva, ap->a_context, ngaflags);
	if (error) {
		goto out;
	}

	                                        
	if (VATTR_IS_ACTIVE(vap, va_rdev) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_RAWDEV)) {
		dev_t rdev = makedev(nva->nva_rawdev.specdata1, nva->nva_rawdev.specdata2);
		VATTR_RETURN(vap, va_rdev, rdev);
	}
	if (VATTR_IS_ACTIVE(vap, va_nlink) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_NUMLINKS)) {
		VATTR_RETURN(vap, va_nlink, nva->nva_nlink);
	}
	if (VATTR_IS_ACTIVE(vap, va_data_size) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_SIZE)) {
		VATTR_RETURN(vap, va_data_size, nva->nva_size);
	}
	                                         
	                                         
	if (VATTR_IS_ACTIVE(vap, va_total_alloc) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_SPACE_USED)) {
		VATTR_RETURN(vap, va_total_alloc, nva->nva_bytes);
	}
	if (VATTR_IS_ACTIVE(vap, va_uid) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_OWNER)) {
		VATTR_RETURN(vap, va_uid, nva->nva_uid);
	}
	if (VATTR_IS_ACTIVE(vap, va_uuuid) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_OWNER)) {
		VATTR_RETURN(vap, va_uuuid, nva->nva_uuuid);
	}
	if (VATTR_IS_ACTIVE(vap, va_gid) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_OWNER_GROUP)) {
		VATTR_RETURN(vap, va_gid, nva->nva_gid);
	}
	if (VATTR_IS_ACTIVE(vap, va_guuid) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_OWNER_GROUP)) {
		VATTR_RETURN(vap, va_guuid, nva->nva_guuid);
	}
	if (VATTR_IS_ACTIVE(vap, va_mode)) {
		if (NMFLAG(nmp, ACLONLY) || !NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_MODE)) {
			VATTR_RETURN(vap, va_mode, ACCESSPERMS);
		} else {
			VATTR_RETURN(vap, va_mode, nva->nva_mode);
		}
	}
	if (VATTR_IS_ACTIVE(vap, va_flags) &&
	    (NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_ARCHIVE) ||
	    NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_HIDDEN) ||
	    (nva->nva_flags & NFS_FFLAG_TRIGGER))) {
		uint32_t flags = 0;
		if (NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_ARCHIVE) &&
		    (nva->nva_flags & NFS_FFLAG_ARCHIVED)) {
			flags |= SF_ARCHIVED;
		}
		if (NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_HIDDEN) &&
		    (nva->nva_flags & NFS_FFLAG_HIDDEN)) {
			flags |= UF_HIDDEN;
		}
		VATTR_RETURN(vap, va_flags, flags);
	}
	if (VATTR_IS_ACTIVE(vap, va_create_time) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_TIME_CREATE)) {
		vap->va_create_time.tv_sec = nva->nva_timesec[NFSTIME_CREATE];
		vap->va_create_time.tv_nsec = nva->nva_timensec[NFSTIME_CREATE];
		VATTR_SET_SUPPORTED(vap, va_create_time);
	}
	if (VATTR_IS_ACTIVE(vap, va_access_time) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_TIME_ACCESS)) {
		vap->va_access_time.tv_sec = nva->nva_timesec[NFSTIME_ACCESS];
		vap->va_access_time.tv_nsec = nva->nva_timensec[NFSTIME_ACCESS];
		VATTR_SET_SUPPORTED(vap, va_access_time);
	}
	if (VATTR_IS_ACTIVE(vap, va_modify_time) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_TIME_MODIFY)) {
		vap->va_modify_time.tv_sec = nva->nva_timesec[NFSTIME_MODIFY];
		vap->va_modify_time.tv_nsec = nva->nva_timensec[NFSTIME_MODIFY];
		VATTR_SET_SUPPORTED(vap, va_modify_time);
	}
	if (VATTR_IS_ACTIVE(vap, va_change_time) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_TIME_METADATA)) {
		vap->va_change_time.tv_sec = nva->nva_timesec[NFSTIME_CHANGE];
		vap->va_change_time.tv_nsec = nva->nva_timensec[NFSTIME_CHANGE];
		VATTR_SET_SUPPORTED(vap, va_change_time);
	}
	if (VATTR_IS_ACTIVE(vap, va_backup_time) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_TIME_BACKUP)) {
		vap->va_backup_time.tv_sec = nva->nva_timesec[NFSTIME_BACKUP];
		vap->va_backup_time.tv_nsec = nva->nva_timensec[NFSTIME_BACKUP];
		VATTR_SET_SUPPORTED(vap, va_backup_time);
	}
	if (VATTR_IS_ACTIVE(vap, va_fileid) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_FILEID)) {
		VATTR_RETURN(vap, va_fileid, nva->nva_fileid);
	}
	if (VATTR_IS_ACTIVE(vap, va_type) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_TYPE)) {
		VATTR_RETURN(vap, va_type, nva->nva_type);
	}
	if (VATTR_IS_ACTIVE(vap, va_filerev) && NFS_BITMAP_ISSET(nva->nva_bitmap, NFS_FATTR_CHANGE)) {
		VATTR_RETURN(vap, va_filerev, nva->nva_change);
	}

	if (VATTR_IS_ACTIVE(vap, va_acl) && acls) {
		VATTR_RETURN(vap, va_acl, nva->nva_acl);
		nva->nva_acl = NULL;
	}

	                                        
	                                                                             

	NVATTR_CLEANUP(nva);
out:
	FREE(nva, M_TEMP);
	return error;
}

int
nfs4_setattr_rpc(
	nfsnode_t np,
	struct vnode_attr *vap,
	vfs_context_t ctx)
{
	struct nfsmount *nmp = NFSTONMP(np);
	int error = 0, setattr_error = 0, lockerror = ENOENT, status, nfsvers, numops;
	u_int64_t xid, nextxid;
	struct nfsm_chain nmreq, nmrep;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN], bmlen;
	uint32_t getbitmap[NFS_ATTR_BITMAP_LEN];
	uint32_t setbitmap[NFS_ATTR_BITMAP_LEN];
	nfs_stateid stateid;
	struct nfsreq_secinfo_args si;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	if (VATTR_IS_ACTIVE(vap, va_flags) && (vap->va_flags & ~(SF_ARCHIVED | UF_HIDDEN))) {
		                                                       
		if (vap->va_active & ~VNODE_ATTR_va_flags) {
			return EINVAL;                                                        
		} else {
			return ENOTSUP;                                          
		}
	}

	                                                                                    
	if (VATTR_IS_ACTIVE(vap, va_uid) && (vap->va_uid == np->n_vattr.nva_uid)) {
		VATTR_CLEAR_ACTIVE(vap, va_uid);
	}
	if (VATTR_IS_ACTIVE(vap, va_gid) && (vap->va_gid == np->n_vattr.nva_gid)) {
		VATTR_CLEAR_ACTIVE(vap, va_gid);
	}
	if (VATTR_IS_ACTIVE(vap, va_uuuid) && kauth_guid_equal(&vap->va_uuuid, &np->n_vattr.nva_uuuid)) {
		VATTR_CLEAR_ACTIVE(vap, va_uuuid);
	}
	if (VATTR_IS_ACTIVE(vap, va_guuid) && kauth_guid_equal(&vap->va_guuid, &np->n_vattr.nva_guuid)) {
		VATTR_CLEAR_ACTIVE(vap, va_guuid);
	}

tryagain:
	                                              
	nfs_vattr_set_bitmap(nmp, bitmap, vap);
	if (!bitmap[0] && !bitmap[1]) {
		return 0;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	  
                                                                 
                                                                    
                                              
    
	NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, getbitmap);
	if (NFS_BITMAP_ISSET(bitmap, NFS_FATTR_ACL) ||
	    NFS_BITMAP_ISSET(bitmap, NFS_FATTR_MODE)) {
		if (NACLVALID(np)) {
			NFS_BITMAP_SET(getbitmap, NFS_FATTR_ACL);
		}
		NACLINVALIDATE(np);
	}

	                          
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 40 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "setattr", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_SETATTR);
	if (VATTR_IS_ACTIVE(vap, va_data_size)) {
		nfs_get_stateid(np, vfs_context_thread(ctx), vfs_context_ucred(ctx), &stateid);
	} else {
		stateid.seqid = stateid.other[0] = stateid.other[1] = stateid.other[2] = 0;
	}
	nfsm_chain_add_stateid(error, &nmreq, &stateid);
	nfsm_chain_add_fattr4(error, &nmreq, vap, nmp);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, getbitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request(np, NULL, &nmreq, NFSPROC4_COMPOUND, ctx, &si, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_SETATTR);
	nfsmout_if(error == EBADRPC);
	setattr_error = error;
	error = 0;
	bmlen = NFS_ATTR_BITMAP_LEN;
	nfsm_chain_get_bitmap(error, &nmrep, setbitmap, bmlen);
	if (!error) {
		if (VATTR_IS_ACTIVE(vap, va_data_size) && (np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR)) {
			microuptime(&np->n_lastio);
		}
		nfs_vattr_set_supported(setbitmap, vap);
		error = setattr_error;
	}
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, np, nfsvers, &xid);
	if (error) {
		NATTRINVALIDATE(np);
	}
	  
                                                                   
                                                                  
                                                                   
                                                                        
                                                                     
                                                                   
                    
    
	nextxid = 0;
	nfs_get_xid(&nextxid);
	if (nextxid != (xid + 1)) {
		np->n_xid = nextxid;
		NATTRINVALIDATE(np);
	}
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	if ((setattr_error == EINVAL) && VATTR_IS_ACTIVE(vap, va_acl) && VATTR_IS_ACTIVE(vap, va_mode) && !NMFLAG(nmp, ACLONLY)) {
		  
                                                              
                                                                   
                                                                   
                                            
     
		if (((bitmap[0] & setbitmap[0]) != bitmap[0]) ||
		    ((bitmap[1] & (setbitmap[1] | NFS_FATTR_MODE)) != bitmap[1])) {
			VATTR_CLEAR_ACTIVE(vap, va_mode);
			error = 0;
			goto tryagain;
		}
	}
	return error;
}
#endif                  

  
                                             
   
int
nfs_mount_state_wait_for_recovery(struct nfsmount *nmp)
{
	struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
	int error = 0, slpflag = NMFLAG(nmp, INTR) ? PCATCH : 0;

	lck_mtx_lock(&nmp->nm_lock);
	while (nmp->nm_state & NFSSTA_RECOVER) {
		if ((error = nfs_sigintr(nmp, NULL, current_thread(), 1))) {
			break;
		}
		nfs_mount_sock_thread_wake(nmp);
		msleep(&nmp->nm_state, &nmp->nm_lock, slpflag | (PZERO - 1), "nfsrecoverwait", &ts);
		slpflag = 0;
	}
	lck_mtx_unlock(&nmp->nm_lock);

	return error;
}

  
                                                             
                                                        
                                                      
                                         
   
int
nfs_mount_state_in_use_start(struct nfsmount *nmp, thread_t thd)
{
	struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
	int error = 0, slpflag = (NMFLAG(nmp, INTR) && thd) ? PCATCH : 0;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	lck_mtx_lock(&nmp->nm_lock);
	if (nmp->nm_state & (NFSSTA_FORCE | NFSSTA_DEAD)) {
		lck_mtx_unlock(&nmp->nm_lock);
		return ENXIO;
	}
	while (nmp->nm_state & NFSSTA_RECOVER) {
		if ((error = nfs_sigintr(nmp, NULL, thd, 1))) {
			break;
		}
		nfs_mount_sock_thread_wake(nmp);
		msleep(&nmp->nm_state, &nmp->nm_lock, slpflag | (PZERO - 1), "nfsrecoverwait", &ts);
		slpflag = 0;
	}
	if (!error) {
		nmp->nm_stateinuse++;
	}
	lck_mtx_unlock(&nmp->nm_lock);

	return error;
}

  
                                                          
                                                            
                                         
   
int
nfs_mount_state_in_use_end(struct nfsmount *nmp, int error)
{
	int restart = nfs_mount_state_error_should_restart(error);

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	lck_mtx_lock(&nmp->nm_lock);
	if (restart && (error != NFSERR_OLD_STATEID) && (error != NFSERR_GRACE)) {
		printf("nfs_mount_state_in_use_end: error %d, initiating recovery for %s, 0x%x\n",
		    error, vfs_statfs(nmp->nm_mountp)->f_mntfromname, nmp->nm_stategenid);
		nfs_need_recover(nmp, error);
	}
	if (nmp->nm_stateinuse > 0) {
		nmp->nm_stateinuse--;
	} else {
		panic("NFS mount state in use count underrun");
	}
	if (!nmp->nm_stateinuse && (nmp->nm_state & NFSSTA_RECOVER)) {
		wakeup(&nmp->nm_stateinuse);
	}
	lck_mtx_unlock(&nmp->nm_lock);
	if (error == NFSERR_GRACE) {
		tsleep(&nmp->nm_state, (PZERO - 1), "nfsgrace", 2 * hz);
	}

	return restart;
}

  
                                                                        
   
int
nfs_mount_state_error_should_restart(int error)
{
	switch (error) {
	case NFSERR_STALE_STATEID:
	case NFSERR_STALE_CLIENTID:
	case NFSERR_ADMIN_REVOKED:
	case NFSERR_EXPIRED:
	case NFSERR_OLD_STATEID:
	case NFSERR_BAD_STATEID:
	case NFSERR_GRACE:
		return 1;
	}
	return 0;
}

  
                                                                 
                                                                        
                                                               
   
uint
nfs_mount_state_max_restarts(struct nfsmount *nmp)
{
	return MAX(nmp->nm_fsattr.nfsa_lease, 60);
}

  
                                                     
   
int
nfs_mount_state_error_delegation_lost(int error)
{
	switch (error) {
	case NFSERR_STALE_STATEID:
	case NFSERR_ADMIN_REVOKED:
	case NFSERR_EXPIRED:
	case NFSERR_OLD_STATEID:
	case NFSERR_BAD_STATEID:
	case NFSERR_GRACE:                                                                                            
		return 1;
	}
	return 0;
}


  
                                         
   
int
nfs_open_state_set_busy(nfsnode_t np, thread_t thd)
{
	struct nfsmount *nmp;
	struct timespec ts = { .tv_sec = 2, .tv_nsec = 0 };
	int error = 0, slpflag;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	slpflag = (NMFLAG(nmp, INTR) && thd) ? PCATCH : 0;

	lck_mtx_lock(&np->n_openlock);
	while (np->n_openflags & N_OPENBUSY) {
		if ((error = nfs_sigintr(nmp, NULL, thd, 0))) {
			break;
		}
		np->n_openflags |= N_OPENWANT;
		msleep(&np->n_openflags, &np->n_openlock, slpflag, "nfs_open_state_set_busy", &ts);
		slpflag = 0;
	}
	if (!error) {
		np->n_openflags |= N_OPENBUSY;
	}
	lck_mtx_unlock(&np->n_openlock);

	return error;
}

  
                                                       
                     
   
void
nfs_open_state_clear_busy(nfsnode_t np)
{
	int wanted;

	lck_mtx_lock(&np->n_openlock);
	if (!(np->n_openflags & N_OPENBUSY)) {
		panic("nfs_open_state_clear_busy");
	}
	wanted = (np->n_openflags & N_OPENWANT);
	np->n_openflags &= ~(N_OPENBUSY | N_OPENWANT);
	lck_mtx_unlock(&np->n_openlock);
	if (wanted) {
		wakeup(&np->n_openflags);
	}
}

  
                                                                      
                                                            
   
struct nfs_open_owner *
nfs_open_owner_find(struct nfsmount *nmp, kauth_cred_t cred, int alloc)
{
	uid_t uid = kauth_cred_getuid(cred);
	struct nfs_open_owner *noop, *newnoop = NULL;

tryagain:
	lck_mtx_lock(&nmp->nm_lock);
	TAILQ_FOREACH(noop, &nmp->nm_open_owners, noo_link) {
		if (kauth_cred_getuid(noop->noo_cred) == uid) {
			break;
		}
	}

	if (!noop && !newnoop && alloc) {
		lck_mtx_unlock(&nmp->nm_lock);
		MALLOC(newnoop, struct nfs_open_owner *, sizeof(struct nfs_open_owner), M_TEMP, M_WAITOK);
		if (!newnoop) {
			return NULL;
		}
		bzero(newnoop, sizeof(*newnoop));
		lck_mtx_init(&newnoop->noo_lock, &nfs_open_grp, LCK_ATTR_NULL);
		newnoop->noo_mount = nmp;
		kauth_cred_ref(cred);
		newnoop->noo_cred = cred;
		newnoop->noo_name = OSAddAtomic(1, &nfs_open_owner_seqnum);
		TAILQ_INIT(&newnoop->noo_opens);
		goto tryagain;
	}
	if (!noop && newnoop) {
		newnoop->noo_flags |= NFS_OPEN_OWNER_LINK;
		os_ref_init(&newnoop->noo_refcnt, NULL);
		TAILQ_INSERT_HEAD(&nmp->nm_open_owners, newnoop, noo_link);
		noop = newnoop;
	}
	lck_mtx_unlock(&nmp->nm_lock);

	if (newnoop && (noop != newnoop)) {
		nfs_open_owner_destroy(newnoop);
	}

	if (noop) {
		nfs_open_owner_ref(noop);
	}

	return noop;
}

  
                                                
   
void
nfs_open_owner_destroy(struct nfs_open_owner *noop)
{
	if (noop->noo_cred) {
		kauth_cred_unref(&noop->noo_cred);
	}
	lck_mtx_destroy(&noop->noo_lock, &nfs_open_grp);
	FREE(noop, M_TEMP);
}

  
                                             
   
void
nfs_open_owner_ref(struct nfs_open_owner *noop)
{
	lck_mtx_lock(&noop->noo_lock);
	os_ref_retain_locked(&noop->noo_refcnt);
	lck_mtx_unlock(&noop->noo_lock);
}

  
                                                            
                                                                
   
void
nfs_open_owner_rele(struct nfs_open_owner *noop)
{
	os_ref_count_t newcount;

	lck_mtx_lock(&noop->noo_lock);
	if (os_ref_get_count(&noop->noo_refcnt) < 1) {
		panic("nfs_open_owner_rele: no refcnt");
	}
	newcount = os_ref_release_locked(&noop->noo_refcnt);
	if (!newcount && (noop->noo_flags & NFS_OPEN_OWNER_BUSY)) {
		panic("nfs_open_owner_rele: busy");
	}
	                                                                               
	if (newcount || (noop->noo_flags & NFS_OPEN_OWNER_LINK)) {
		lck_mtx_unlock(&noop->noo_lock);
		return;
	}
	                                                                     
	lck_mtx_unlock(&noop->noo_lock);
	nfs_open_owner_destroy(noop);
}

  
                                                     
                                                             
   
int
nfs_open_owner_set_busy(struct nfs_open_owner *noop, thread_t thd)
{
	struct nfsmount *nmp;
	struct timespec ts = { .tv_sec = 2, .tv_nsec = 0 };
	int error = 0, slpflag;

	nmp = noop->noo_mount;
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	slpflag = (NMFLAG(nmp, INTR) && thd) ? PCATCH : 0;

	lck_mtx_lock(&noop->noo_lock);
	while (noop->noo_flags & NFS_OPEN_OWNER_BUSY) {
		if ((error = nfs_sigintr(nmp, NULL, thd, 0))) {
			break;
		}
		noop->noo_flags |= NFS_OPEN_OWNER_WANT;
		msleep(noop, &noop->noo_lock, slpflag, "nfs_open_owner_set_busy", &ts);
		slpflag = 0;
	}
	if (!error) {
		noop->noo_flags |= NFS_OPEN_OWNER_BUSY;
	}
	lck_mtx_unlock(&noop->noo_lock);

	return error;
}

  
                                                                  
                   
   
void
nfs_open_owner_clear_busy(struct nfs_open_owner *noop)
{
	int wanted;

	lck_mtx_lock(&noop->noo_lock);
	if (!(noop->noo_flags & NFS_OPEN_OWNER_BUSY)) {
		panic("nfs_open_owner_clear_busy");
	}
	wanted = (noop->noo_flags & NFS_OPEN_OWNER_WANT);
	noop->noo_flags &= ~(NFS_OPEN_OWNER_BUSY | NFS_OPEN_OWNER_WANT);
	lck_mtx_unlock(&noop->noo_lock);
	if (wanted) {
		wakeup(noop);
	}
}

  
                                                            
                              
   
void
nfs_owner_seqid_increment(struct nfs_open_owner *noop, struct nfs_lock_owner *nlop, int error)
{
	switch (error) {
	case NFSERR_STALE_CLIENTID:
	case NFSERR_STALE_STATEID:
	case NFSERR_OLD_STATEID:
	case NFSERR_BAD_STATEID:
	case NFSERR_BAD_SEQID:
	case NFSERR_BADXDR:
	case NFSERR_RESOURCE:
	case NFSERR_NOFILEHANDLE:
		                                                     
		return;
	}
	if (noop) {
		noop->noo_seqid++;
	}
	if (nlop) {
		nlop->nlo_seqid++;
	}
}

  
                                                                      
                                                   
                                                      
   
int
nfs_open_file_find(
	nfsnode_t np,
	struct nfs_open_owner *noop,
	struct nfs_open_file **nofpp,
	uint32_t accessMode,
	uint32_t denyMode,
	int alloc)
{
	*nofpp = NULL;
	return nfs_open_file_find_internal(np, noop, nofpp, accessMode, denyMode, alloc);
}

  
                                                                             
                                                                          
                                                                            
   
int
nfs_open_file_find_internal(
	nfsnode_t np,
	struct nfs_open_owner *noop,
	struct nfs_open_file **nofpp,
	uint32_t accessMode,
	uint32_t denyMode,
	int alloc)
{
	struct nfs_open_file *nofp = NULL, *nofp2, *newnofp = NULL;

	if (!np) {
		goto alloc;
	}
tryagain:
	lck_mtx_lock(&np->n_openlock);
	TAILQ_FOREACH(nofp2, &np->n_opens, nof_link) {
		if (nofp2->nof_owner == noop) {
			nofp = nofp2;
			if (!accessMode) {
				break;
			}
		}
		if ((accessMode & nofp2->nof_deny) || (denyMode & nofp2->nof_access)) {
			                                                                  
			lck_mtx_unlock(&np->n_openlock);
			return EACCES;
		}
	}

	  
                                           
                                             
    
	if (!nofp && !*nofpp && !newnofp && alloc) {
		lck_mtx_unlock(&np->n_openlock);
alloc:
		MALLOC(newnofp, struct nfs_open_file *, sizeof(struct nfs_open_file), M_TEMP, M_WAITOK);
		if (!newnofp) {
			return ENOMEM;
		}
		bzero(newnofp, sizeof(*newnofp));
		lck_mtx_init(&newnofp->nof_lock, &nfs_open_grp, LCK_ATTR_NULL);
		newnofp->nof_owner = noop;
		nfs_open_owner_ref(noop);
		newnofp->nof_np = np;
		lck_mtx_lock(&noop->noo_lock);
		TAILQ_INSERT_HEAD(&noop->noo_opens, newnofp, nof_oolink);
		lck_mtx_unlock(&noop->noo_lock);
		if (np) {
			goto tryagain;
		}
	}
	if (!nofp) {
		if (*nofpp) {
			(*nofpp)->nof_np = np;
			nofp = *nofpp;
		} else {
			nofp = newnofp;
		}
		if (nofp && np) {
			TAILQ_INSERT_HEAD(&np->n_opens, nofp, nof_link);
		}
	}
	if (np) {
		lck_mtx_unlock(&np->n_openlock);
	}

	if (alloc && newnofp && (nofp != newnofp)) {
		nfs_open_file_destroy(newnofp);
	}

	*nofpp = nofp;
	return nofp ? 0 : ESRCH;
}

  
                                  
   
void
nfs_open_file_destroy(struct nfs_open_file *nofp)
{
	lck_mtx_lock(&nofp->nof_owner->noo_lock);
	TAILQ_REMOVE(&nofp->nof_owner->noo_opens, nofp, nof_oolink);
	lck_mtx_unlock(&nofp->nof_owner->noo_lock);
	nfs_open_owner_rele(nofp->nof_owner);
	lck_mtx_destroy(&nofp->nof_lock, &nfs_open_grp);
	FREE(nofp, M_TEMP);
}

  
                                                    
                                                            
   
int
nfs_open_file_set_busy(struct nfs_open_file *nofp, thread_t thd)
{
	struct nfsmount *nmp;
	struct timespec ts = { .tv_sec = 2, .tv_nsec = 0 };
	int error = 0, slpflag;

	nmp = nofp->nof_owner->noo_mount;
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	slpflag = (NMFLAG(nmp, INTR) && thd) ? PCATCH : 0;

	lck_mtx_lock(&nofp->nof_lock);
	while (nofp->nof_flags & NFS_OPEN_FILE_BUSY) {
		if ((error = nfs_sigintr(nmp, NULL, thd, 0))) {
			break;
		}
		nofp->nof_flags |= NFS_OPEN_FILE_WANT;
		msleep(nofp, &nofp->nof_lock, slpflag, "nfs_open_file_set_busy", &ts);
		slpflag = 0;
	}
	if (!error) {
		nofp->nof_flags |= NFS_OPEN_FILE_BUSY;
	}
	lck_mtx_unlock(&nofp->nof_lock);

	return error;
}

  
                                                                 
                   
   
void
nfs_open_file_clear_busy(struct nfs_open_file *nofp)
{
	int wanted;

	lck_mtx_lock(&nofp->nof_lock);
	if (!(nofp->nof_flags & NFS_OPEN_FILE_BUSY)) {
		panic("nfs_open_file_clear_busy");
	}
	wanted = (nofp->nof_flags & NFS_OPEN_FILE_WANT);
	nofp->nof_flags &= ~(NFS_OPEN_FILE_BUSY | NFS_OPEN_FILE_WANT);
	lck_mtx_unlock(&nofp->nof_lock);
	if (wanted) {
		wakeup(nofp);
	}
}

  
                                                                        
   
void
nfs_open_file_add_open(struct nfs_open_file *nofp, uint32_t accessMode, uint32_t denyMode, int delegated)
{
	lck_mtx_lock(&nofp->nof_lock);
	nofp->nof_access |= accessMode;
	nofp->nof_deny |= denyMode;

	if (delegated) {
		if (denyMode == NFS_OPEN_SHARE_DENY_NONE) {
			if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
				nofp->nof_d_r++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
				nofp->nof_d_w++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
				nofp->nof_d_rw++;
			}
		} else if (denyMode == NFS_OPEN_SHARE_DENY_WRITE) {
			if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
				nofp->nof_d_r_dw++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
				nofp->nof_d_w_dw++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
				nofp->nof_d_rw_dw++;
			}
		} else {                               
			if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
				nofp->nof_d_r_drw++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
				nofp->nof_d_w_drw++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
				nofp->nof_d_rw_drw++;
			}
		}
	} else {
		if (denyMode == NFS_OPEN_SHARE_DENY_NONE) {
			if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
				nofp->nof_r++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
				nofp->nof_w++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
				nofp->nof_rw++;
			}
		} else if (denyMode == NFS_OPEN_SHARE_DENY_WRITE) {
			if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
				nofp->nof_r_dw++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
				nofp->nof_w_dw++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
				nofp->nof_rw_dw++;
			}
		} else {                               
			if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
				nofp->nof_r_drw++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
				nofp->nof_w_drw++;
			} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
				nofp->nof_rw_drw++;
			}
		}
	}

	nofp->nof_opencnt++;
	lck_mtx_unlock(&nofp->nof_lock);
}

  
                                                                  
                                                            
   
void
nfs_open_file_remove_open_find(
	struct nfs_open_file *nofp,
	uint32_t accessMode,
	uint32_t denyMode,
	uint8_t *newAccessMode,
	uint8_t *newDenyMode,
	int *delegated)
{
	  
                                                                  
                                             
    
	*newAccessMode = nofp->nof_access;
	*newDenyMode = nofp->nof_deny;

	if ((accessMode & NFS_OPEN_SHARE_ACCESS_READ) &&
	    (nofp->nof_access & NFS_OPEN_SHARE_ACCESS_READ) &&
	    ((nofp->nof_r + nofp->nof_d_r +
	    nofp->nof_rw + nofp->nof_d_rw +
	    nofp->nof_r_dw + nofp->nof_d_r_dw +
	    nofp->nof_rw_dw + nofp->nof_d_rw_dw +
	    nofp->nof_r_drw + nofp->nof_d_r_drw +
	    nofp->nof_rw_dw + nofp->nof_d_rw_dw) == 1)) {
		*newAccessMode &= ~NFS_OPEN_SHARE_ACCESS_READ;
	}
	if ((accessMode & NFS_OPEN_SHARE_ACCESS_WRITE) &&
	    (nofp->nof_access & NFS_OPEN_SHARE_ACCESS_WRITE) &&
	    ((nofp->nof_w + nofp->nof_d_w +
	    nofp->nof_rw + nofp->nof_d_rw +
	    nofp->nof_w_dw + nofp->nof_d_w_dw +
	    nofp->nof_rw_dw + nofp->nof_d_rw_dw +
	    nofp->nof_w_drw + nofp->nof_d_w_drw +
	    nofp->nof_rw_dw + nofp->nof_d_rw_dw) == 1)) {
		*newAccessMode &= ~NFS_OPEN_SHARE_ACCESS_WRITE;
	}
	if ((denyMode & NFS_OPEN_SHARE_DENY_READ) &&
	    (nofp->nof_deny & NFS_OPEN_SHARE_DENY_READ) &&
	    ((nofp->nof_r_drw + nofp->nof_d_r_drw +
	    nofp->nof_w_drw + nofp->nof_d_w_drw +
	    nofp->nof_rw_drw + nofp->nof_d_rw_drw) == 1)) {
		*newDenyMode &= ~NFS_OPEN_SHARE_DENY_READ;
	}
	if ((denyMode & NFS_OPEN_SHARE_DENY_WRITE) &&
	    (nofp->nof_deny & NFS_OPEN_SHARE_DENY_WRITE) &&
	    ((nofp->nof_r_drw + nofp->nof_d_r_drw +
	    nofp->nof_w_drw + nofp->nof_d_w_drw +
	    nofp->nof_rw_drw + nofp->nof_d_rw_drw +
	    nofp->nof_r_dw + nofp->nof_d_r_dw +
	    nofp->nof_w_dw + nofp->nof_d_w_dw +
	    nofp->nof_rw_dw + nofp->nof_d_rw_dw) == 1)) {
		*newDenyMode &= ~NFS_OPEN_SHARE_DENY_WRITE;
	}

	                                                           
	if (denyMode == NFS_OPEN_SHARE_DENY_NONE) {
		if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
			*delegated = (nofp->nof_d_r != 0);
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
			*delegated = (nofp->nof_d_w != 0);
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
			*delegated = (nofp->nof_d_rw != 0);
		} else {
			*delegated = 0;
		}
	} else if (denyMode == NFS_OPEN_SHARE_DENY_WRITE) {
		if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
			*delegated = (nofp->nof_d_r_dw != 0);
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
			*delegated = (nofp->nof_d_w_dw != 0);
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
			*delegated = (nofp->nof_d_rw_dw != 0);
		} else {
			*delegated = 0;
		}
	} else {                               
		if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
			*delegated = (nofp->nof_d_r_drw != 0);
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
			*delegated = (nofp->nof_d_w_drw != 0);
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
			*delegated = (nofp->nof_d_rw_drw != 0);
		} else {
			*delegated = 0;
		}
	}
}

  
                                                                           
   
void
nfs_open_file_remove_open(struct nfs_open_file *nofp, uint32_t accessMode, uint32_t denyMode)
{
	uint8_t newAccessMode, newDenyMode;
	int delegated = 0;

	lck_mtx_lock(&nofp->nof_lock);
	nfs_open_file_remove_open_find(nofp, accessMode, denyMode, &newAccessMode, &newDenyMode, &delegated);

	                                                                
	if (denyMode == NFS_OPEN_SHARE_DENY_NONE) {
		if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
			if (delegated) {
				if (nofp->nof_d_r == 0) {
					NP(nofp->nof_np, "nfs: open(R) delegated count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_d_r--;
				}
			} else {
				if (nofp->nof_r == 0) {
					NP(nofp->nof_np, "nfs: open(R) count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_r--;
				}
			}
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
			if (delegated) {
				if (nofp->nof_d_w == 0) {
					NP(nofp->nof_np, "nfs: open(W) delegated count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_d_w--;
				}
			} else {
				if (nofp->nof_w == 0) {
					NP(nofp->nof_np, "nfs: open(W) count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_w--;
				}
			}
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
			if (delegated) {
				if (nofp->nof_d_rw == 0) {
					NP(nofp->nof_np, "nfs: open(RW) delegated count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_d_rw--;
				}
			} else {
				if (nofp->nof_rw == 0) {
					NP(nofp->nof_np, "nfs: open(RW) count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_rw--;
				}
			}
		}
	} else if (denyMode == NFS_OPEN_SHARE_DENY_WRITE) {
		if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
			if (delegated) {
				if (nofp->nof_d_r_dw == 0) {
					NP(nofp->nof_np, "nfs: open(R,DW) delegated count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_d_r_dw--;
				}
			} else {
				if (nofp->nof_r_dw == 0) {
					NP(nofp->nof_np, "nfs: open(R,DW) count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_r_dw--;
				}
			}
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
			if (delegated) {
				if (nofp->nof_d_w_dw == 0) {
					NP(nofp->nof_np, "nfs: open(W,DW) delegated count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_d_w_dw--;
				}
			} else {
				if (nofp->nof_w_dw == 0) {
					NP(nofp->nof_np, "nfs: open(W,DW) count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_w_dw--;
				}
			}
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
			if (delegated) {
				if (nofp->nof_d_rw_dw == 0) {
					NP(nofp->nof_np, "nfs: open(RW,DW) delegated count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_d_rw_dw--;
				}
			} else {
				if (nofp->nof_rw_dw == 0) {
					NP(nofp->nof_np, "nfs: open(RW,DW) count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_rw_dw--;
				}
			}
		}
	} else {                               
		if (accessMode == NFS_OPEN_SHARE_ACCESS_READ) {
			if (delegated) {
				if (nofp->nof_d_r_drw == 0) {
					NP(nofp->nof_np, "nfs: open(R,DRW) delegated count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_d_r_drw--;
				}
			} else {
				if (nofp->nof_r_drw == 0) {
					NP(nofp->nof_np, "nfs: open(R,DRW) count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_r_drw--;
				}
			}
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_WRITE) {
			if (delegated) {
				if (nofp->nof_d_w_drw == 0) {
					NP(nofp->nof_np, "nfs: open(W,DRW) delegated count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_d_w_drw--;
				}
			} else {
				if (nofp->nof_w_drw == 0) {
					NP(nofp->nof_np, "nfs: open(W,DRW) count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_w_drw--;
				}
			}
		} else if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
			if (delegated) {
				if (nofp->nof_d_rw_drw == 0) {
					NP(nofp->nof_np, "nfs: open(RW,DRW) delegated count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_d_rw_drw--;
				}
			} else {
				if (nofp->nof_rw_drw == 0) {
					NP(nofp->nof_np, "nfs: open(RW,DRW) count underrun, %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
				} else {
					nofp->nof_rw_drw--;
				}
			}
		}
	}

	                      
	nofp->nof_access = newAccessMode;
	nofp->nof_deny = newDenyMode;
	nofp->nof_opencnt--;
	lck_mtx_unlock(&nofp->nof_lock);
}

#if CONFIG_NFS4
  
                                                                           
                                              
                                                  
                                  
                                                      
   
void
nfs_get_stateid(nfsnode_t np, thread_t thd, kauth_cred_t cred, nfs_stateid *sid)
{
	struct nfsmount *nmp = NFSTONMP(np);
	proc_t p = thd ? get_bsdthreadtask_info(thd) : current_proc();                                               
	struct nfs_open_owner *noop = NULL;
	struct nfs_open_file *nofp = NULL;
	struct nfs_lock_owner *nlop = NULL;
	nfs_stateid *s = NULL;

	if (np->n_openflags & N_DELEG_MASK) {
		s = &np->n_dstateid;
	} else {
		if (p) {
			nlop = nfs_lock_owner_find(np, p, 0);
		}
		if (nlop && !TAILQ_EMPTY(&nlop->nlo_locks)) {
			                                     
			s = &nlop->nlo_stateid;
		} else if (((noop = nfs_open_owner_find(nmp, cred, 0))) &&
		    (nfs_open_file_find(np, noop, &nofp, 0, 0, 0) == 0) &&
		    !(nofp->nof_flags & NFS_OPEN_FILE_LOST) &&
		    nofp->nof_access) {
			                                                      
			if (nofp->nof_flags & NFS_OPEN_FILE_REOPEN) {
				nfs4_reopen(nofp, thd);
			}
			if (!(nofp->nof_flags & NFS_OPEN_FILE_LOST)) {
				s = &nofp->nof_stateid;
			}
		}
	}

	if (s) {
		sid->seqid = s->seqid;
		sid->other[0] = s->other[0];
		sid->other[1] = s->other[1];
		sid->other[2] = s->other[2];
	} else {
		                                                                                   
		if (!(np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR)) {
			NP(np, "nfs_get_stateid: no stateid");
		}
		sid->seqid = sid->other[0] = sid->other[1] = sid->other[2] = 0xffffffff;
	}
	if (nlop) {
		nfs_lock_owner_rele(nlop);
	}
	if (noop) {
		nfs_open_owner_rele(noop);
	}
}


  
                                                                         
                                                                              
   
int
nfs4_open_delegated(
	nfsnode_t np,
	struct nfs_open_file *nofp,
	uint32_t accessMode,
	uint32_t denyMode,
	vfs_context_t ctx)
{
	int error = 0, ismember, readtoo = 0, authorized = 0;
	uint32_t action;
	struct kauth_acl_eval eval;
	kauth_cred_t cred = vfs_context_ucred(ctx);

	if (!(accessMode & NFS_OPEN_SHARE_ACCESS_READ)) {
		  
                                        
                                       
     
		readtoo = 1;
		accessMode |= NFS_OPEN_SHARE_ACCESS_READ;
	}

tryagain:
	action = 0;
	if (accessMode & NFS_OPEN_SHARE_ACCESS_READ) {
		action |= KAUTH_VNODE_READ_DATA;
	}
	if (accessMode & NFS_OPEN_SHARE_ACCESS_WRITE) {
		action |= KAUTH_VNODE_WRITE_DATA;
	}

	                                   
	if (np->n_dace.ace_flags) {
		eval.ae_requested = action;
		eval.ae_acl = &np->n_dace;
		eval.ae_count = 1;
		eval.ae_options = 0;
		if (np->n_vattr.nva_uid == kauth_cred_getuid(cred)) {
			eval.ae_options |= KAUTH_AEVAL_IS_OWNER;
		}
		error = kauth_cred_ismember_gid(cred, np->n_vattr.nva_gid, &ismember);
		if (!error && ismember) {
			eval.ae_options |= KAUTH_AEVAL_IN_GROUP;
		}

		eval.ae_exp_gall = KAUTH_VNODE_GENERIC_ALL_BITS;
		eval.ae_exp_gread = KAUTH_VNODE_GENERIC_READ_BITS;
		eval.ae_exp_gwrite = KAUTH_VNODE_GENERIC_WRITE_BITS;
		eval.ae_exp_gexec = KAUTH_VNODE_GENERIC_EXECUTE_BITS;

		error = kauth_acl_evaluate(cred, &eval);

		if (!error && (eval.ae_result == KAUTH_RESULT_ALLOW)) {
			authorized = 1;
		}
	}

	if (!authorized) {
		                                       
		struct vnop_access_args naa;
		naa.a_desc = &vnop_access_desc;
		naa.a_vp = NFSTOV(np);
		naa.a_action = action;
		naa.a_context = ctx;
		if (!(error = nfs_vnop_access(&naa))) {
			authorized = 1;
		}
	}

	if (!authorized) {
		if (readtoo) {
			                                             
			accessMode &= ~NFS_OPEN_SHARE_ACCESS_READ;
			readtoo = 0;
			goto tryagain;
		}
		return error ? error : EACCES;
	}

	nfs_open_file_add_open(nofp, accessMode, denyMode, 1);

	return 0;
}


  
                                                
  
                                                                      
                                                                          
                                                                           
                                                      
  
                                                                                  
   
int
nfs4_open(
	nfsnode_t np,
	struct nfs_open_file *nofp,
	uint32_t accessMode,
	uint32_t denyMode,
	vfs_context_t ctx)
{
	vnode_t vp = NFSTOV(np);
	vnode_t dvp = NULL;
	struct componentname cn;
	const char *vname = NULL;
	uint32_t namelen;
	char smallname[128];
	char *filename = NULL;
	int error = 0, readtoo = 0;

	  
                                                             
                                                            
                                                              
                                              
    
	if (np->n_openflags & N_DELEG_MASK) {
		if ((error = nfs_open_state_set_busy(np, vfs_context_thread(ctx)))) {
			return error;
		}
		if ((np->n_openflags & N_DELEG_MASK) && !(np->n_openflags & N_DELEG_RETURN) &&
		    (((np->n_openflags & N_DELEG_MASK) == N_DELEG_WRITE) ||
		    (!(accessMode & NFS_OPEN_SHARE_ACCESS_WRITE) && !(denyMode & NFS_OPEN_SHARE_DENY_READ)))) {
			error = nfs4_open_delegated(np, nofp, accessMode, denyMode, ctx);
			nfs_open_state_clear_busy(np);
			return error;
		}
		nfs_open_state_clear_busy(np);
	}

	  
                                                               
                                                                 
                                                                
                                           
    
	if ((np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR) &&
	    (((dvp = np->n_parent)) && (error = vnode_get(dvp)))) {
		dvp = NULL;
	}
	if (!dvp) {
		dvp = vnode_getparent(vp);
	}
	vname = vnode_getname(vp);
	if (!dvp || !vname) {
		if (!error) {
			error = EIO;
		}
		goto out;
	}
	filename = &smallname[0];
	namelen = snprintf(filename, sizeof(smallname), "%s", vname);
	if (namelen >= sizeof(smallname)) {
		MALLOC(filename, char *, namelen + 1, M_TEMP, M_WAITOK);
		if (!filename) {
			error = ENOMEM;
			goto out;
		}
		snprintf(filename, namelen + 1, "%s", vname);
	}
	bzero(&cn, sizeof(cn));
	cn.cn_nameptr = filename;
	cn.cn_namelen = namelen;

	if (!(accessMode & NFS_OPEN_SHARE_ACCESS_READ)) {
		  
                                        
                                       
     
		readtoo = 1;
		accessMode |= NFS_OPEN_SHARE_ACCESS_READ;
	}
tryagain:
	error = nfs4_open_rpc(nofp, ctx, &cn, NULL, dvp, &vp, NFS_OPEN_NOCREATE, accessMode, denyMode);
	if (error) {
		if (!nfs_mount_state_error_should_restart(error) &&
		    (error != EINTR) && (error != ERESTART) && readtoo) {
			                                             
			accessMode &= ~NFS_OPEN_SHARE_ACCESS_READ;
			readtoo = 0;
			goto tryagain;
		}
		goto out;
	}
	nfs_open_file_add_open(nofp, accessMode, denyMode, 0);
out:
	if (filename && (filename != &smallname[0])) {
		FREE(filename, M_TEMP);
	}
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
nfs_vnop_mmap(
	struct vnop_mmap_args     
                                                              
                                               
                                               
                                                          
                                      *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	nfsnode_t np = VTONFS(vp);
	int error = 0, delegated = 0;
	uint8_t accessMode, denyMode;
	struct nfsmount *nmp;
	struct nfs_open_owner *noop = NULL;
	struct nfs_open_file *nofp = NULL;

	nmp = VTONMP(vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (!vnode_isreg(vp) || !(ap->a_fflags & (PROT_READ | PROT_WRITE))) {
		return EINVAL;
	}
	if (np->n_flag & NREVOKE) {
		return EIO;
	}

	  
                                                              
                                                                               
                                                                     
    
	accessMode = NFS_OPEN_SHARE_ACCESS_READ;
	if (ap->a_fflags & PROT_WRITE) {
		accessMode |= NFS_OPEN_SHARE_ACCESS_WRITE;
	}
	denyMode = NFS_OPEN_SHARE_DENY_NONE;

	noop = nfs_open_owner_find(nmp, vfs_context_ucred(ctx), 1);
	if (!noop) {
		return ENOMEM;
	}

restart:
	error = nfs_mount_state_in_use_start(nmp, NULL);
	if (error) {
		nfs_open_owner_rele(noop);
		return error;
	}
	if (np->n_flag & NREVOKE) {
		error = EIO;
		nfs_mount_state_in_use_end(nmp, 0);
		nfs_open_owner_rele(noop);
		return error;
	}

	error = nfs_open_file_find(np, noop, &nofp, 0, 0, 1);
	if (error || (!error && (nofp->nof_flags & NFS_OPEN_FILE_LOST))) {
		NP(np, "nfs_vnop_mmap: no open file for owner, error %d, %d", error, kauth_cred_getuid(noop->noo_cred));
		error = EPERM;
	}
#if CONFIG_NFS4
	if (!error && (nofp->nof_flags & NFS_OPEN_FILE_REOPEN)) {
		error = nfs4_reopen(nofp, NULL);
		nofp = NULL;
		if (!error) {
			nfs_mount_state_in_use_end(nmp, 0);
			goto restart;
		}
	}
#endif
	if (!error) {
		error = nfs_open_file_set_busy(nofp, NULL);
	}
	if (error) {
		nofp = NULL;
		goto out;
	}

	  
                                                                    
                                                       
                                                                 
                                                                        
                                                               
   
                                                               
                                                                   
                                                                
                                                                             
                              
    

	if (!(nofp->nof_access & NFS_OPEN_SHARE_ACCESS_READ)) {
		if (nofp->nof_flags & NFS_OPEN_FILE_NEEDCLOSE) {
			                                                                   
			NP(np, "nfs_vnop_mmap: File already needs close access: 0x%x, cred: %d thread: %lld",
			    nofp->nof_access, kauth_cred_getuid(nofp->nof_owner->noo_cred), thread_tid(vfs_context_thread(ctx)));
		}
		  
                                                                                                   
                                                                                                  
     
		if (accessMode != NFS_OPEN_SHARE_ACCESS_READ || (accessMode & nofp->nof_deny)) {
			                                             
			error = EPERM;
			goto out;
		}
		                                                             
		if (nmp->nm_vers < NFS_VER4) {
			                                                          
			nfs_open_file_add_open(nofp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_NONE, 0);
			error = 0;
		}
#if CONFIG_NFS4
		else {
			error = nfs4_open(np, nofp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_NONE, ctx);
		}
#endif
		if (!error) {
			nofp->nof_flags |= NFS_OPEN_FILE_NEEDCLOSE;
		}
		if (error) {
			goto out;
		}
	}

	                                  
	if (accessMode == NFS_OPEN_SHARE_ACCESS_BOTH) {
		if (nofp->nof_d_rw || nofp->nof_d_rw_dw || nofp->nof_d_rw_drw) {
			delegated = 1;
			if (nofp->nof_d_rw) {
				denyMode = NFS_OPEN_SHARE_DENY_NONE;
			} else if (nofp->nof_d_rw_dw) {
				denyMode = NFS_OPEN_SHARE_DENY_WRITE;
			} else if (nofp->nof_d_rw_drw) {
				denyMode = NFS_OPEN_SHARE_DENY_BOTH;
			}
		} else if (nofp->nof_rw || nofp->nof_rw_dw || nofp->nof_rw_drw) {
			delegated = 0;
			if (nofp->nof_rw) {
				denyMode = NFS_OPEN_SHARE_DENY_NONE;
			} else if (nofp->nof_rw_dw) {
				denyMode = NFS_OPEN_SHARE_DENY_WRITE;
			} else if (nofp->nof_rw_drw) {
				denyMode = NFS_OPEN_SHARE_DENY_BOTH;
			}
		} else {
			error = EPERM;
		}
	} else {                                 
		if (nofp->nof_d_r || nofp->nof_d_r_dw || nofp->nof_d_r_drw) {
			delegated = 1;
			if (nofp->nof_d_r) {
				denyMode = NFS_OPEN_SHARE_DENY_NONE;
			} else if (nofp->nof_d_r_dw) {
				denyMode = NFS_OPEN_SHARE_DENY_WRITE;
			} else if (nofp->nof_d_r_drw) {
				denyMode = NFS_OPEN_SHARE_DENY_BOTH;
			}
		} else if (nofp->nof_r || nofp->nof_r_dw || nofp->nof_r_drw) {
			delegated = 0;
			if (nofp->nof_r) {
				denyMode = NFS_OPEN_SHARE_DENY_NONE;
			} else if (nofp->nof_r_dw) {
				denyMode = NFS_OPEN_SHARE_DENY_WRITE;
			} else if (nofp->nof_r_drw) {
				denyMode = NFS_OPEN_SHARE_DENY_BOTH;
			}
		} else if (nofp->nof_d_rw || nofp->nof_d_rw_dw || nofp->nof_d_rw_drw) {
			  
                                                                    
                                                              
                                                             
      
			delegated = 1;
			accessMode = NFS_OPEN_SHARE_ACCESS_BOTH;
			if (nofp->nof_d_rw) {
				denyMode = NFS_OPEN_SHARE_DENY_NONE;
			} else if (nofp->nof_d_rw_dw) {
				denyMode = NFS_OPEN_SHARE_DENY_WRITE;
			} else if (nofp->nof_d_rw_drw) {
				denyMode = NFS_OPEN_SHARE_DENY_BOTH;
			}
		} else if (nofp->nof_rw || nofp->nof_rw_dw || nofp->nof_rw_drw) {
			delegated = 0;
			accessMode = NFS_OPEN_SHARE_ACCESS_BOTH;
			if (nofp->nof_rw) {
				denyMode = NFS_OPEN_SHARE_DENY_NONE;
			} else if (nofp->nof_rw_dw) {
				denyMode = NFS_OPEN_SHARE_DENY_WRITE;
			} else if (nofp->nof_rw_drw) {
				denyMode = NFS_OPEN_SHARE_DENY_BOTH;
			}
		} else {
			error = EPERM;
		}
	}
	if (error) {                                         
		goto out;
	}

	  
                                                                  
                                                                   
                                                      
    
	if ((nofp->nof_mmap_access > accessMode) ||
	    ((nofp->nof_mmap_access == accessMode) && (nofp->nof_mmap_deny <= denyMode))) {
		goto out;
	}

	                           
	if (nofp->nof_mmap_access) {
		error = nfs_close(np, nofp, nofp->nof_mmap_access, nofp->nof_mmap_deny, ctx);
		if (error) {
			if (!nfs_mount_state_error_should_restart(error)) {
				NP(np, "nfs_vnop_mmap: close of previous mmap mode failed: %d, %d", error, kauth_cred_getuid(nofp->nof_owner->noo_cred));
			}
			NP(np, "nfs_vnop_mmap: update, close error %d, %d", error, kauth_cred_getuid(nofp->nof_owner->noo_cred));
			goto out;
		}
		nofp->nof_mmap_access = nofp->nof_mmap_deny = 0;
	}

	nfs_open_file_add_open(nofp, accessMode, denyMode, delegated);
	nofp->nof_mmap_access = accessMode;
	nofp->nof_mmap_deny = denyMode;

out:
	if (nofp) {
		nfs_open_file_clear_busy(nofp);
	}
	if (nfs_mount_state_in_use_end(nmp, error)) {
		nofp = NULL;
		goto restart;
	}
	if (noop) {
		nfs_open_owner_rele(noop);
	}

	if (!error) {
		int ismapped = 0;
		nfs_node_lock_force(np);
		if ((np->n_flag & NISMAPPED) == 0) {
			np->n_flag |= NISMAPPED;
			ismapped = 1;
		}
		nfs_node_unlock(np);
		if (ismapped) {
			lck_mtx_lock(&nmp->nm_lock);
			nmp->nm_state &= ~NFSSTA_SQUISHY;
			nmp->nm_curdeadtimeout = nmp->nm_deadtimeout;
			if (nmp->nm_curdeadtimeout <= 0) {
				nmp->nm_deadto_start = 0;
			}
			nmp->nm_mappers++;
			lck_mtx_unlock(&nmp->nm_lock);
		}
	}

	return error;
}

int
nfs_vnop_mmap_check(
	struct vnop_mmap_check_args     
                                                                    
                                                     
                                                    
                                                                
                                            *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	struct nfsmount *nmp = VTONMP(vp);
	struct vnop_access_args naa;
	int error = 0;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (vnode_isreg(vp)) {
		  
                                                  
                                                 
                                           
     
		naa.a_desc = &vnop_access_desc;
		naa.a_vp = vp;
		naa.a_action = KAUTH_VNODE_READ_DATA;
		naa.a_context = ctx;

		                                                           
		error = nfs_vnop_access(&naa);
	}

	return error;
}

int
nfs_vnop_mnomap(
	struct vnop_mnomap_args     
                                                                
                                                 
                                                            
                                        *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	nfsnode_t np = VTONFS(vp);
	struct nfsmount *nmp;
	struct nfs_open_file *nofp = NULL;
	off_t size;
	int error;
	int is_mapped_flag = 0;

	nmp = VTONMP(vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	nfs_node_lock_force(np);
	if (np->n_flag & NISMAPPED) {
		is_mapped_flag = 1;
		np->n_flag &= ~NISMAPPED;
	}
	nfs_node_unlock(np);
	if (is_mapped_flag) {
		lck_mtx_lock(&nmp->nm_lock);
		if (nmp->nm_mappers) {
			nmp->nm_mappers--;
		} else {
			NP(np, "nfs_vnop_mnomap: removing mmap reference from mount, but mount has no files mmapped");
		}
		lck_mtx_unlock(&nmp->nm_lock);
	}

	                                                                            
	nfs_flush(np, MNT_WAIT, vfs_context_thread(ctx), V_IGNORE_WRITEERR);
	if (UBCINFOEXISTS(vp) && (size = ubc_getsize(vp))) {
		ubc_msync(vp, 0, size, NULL, UBC_PUSHALL | UBC_SYNC);
	}

	                                                  
loop:
	error = nfs_mount_state_in_use_start(nmp, NULL);
	if (error) {
		return error;
	}
	lck_mtx_lock(&np->n_openlock);
	TAILQ_FOREACH(nofp, &np->n_opens, nof_link) {
		if (!nofp->nof_mmap_access) {
			continue;
		}
		lck_mtx_unlock(&np->n_openlock);
#if CONFIG_NFS4
		if (nofp->nof_flags & NFS_OPEN_FILE_REOPEN) {
			error = nfs4_reopen(nofp, NULL);
			if (!error) {
				nfs_mount_state_in_use_end(nmp, 0);
				goto loop;
			}
		}
#endif
		if (!error) {
			error = nfs_open_file_set_busy(nofp, NULL);
		}
		if (error) {
			lck_mtx_lock(&np->n_openlock);
			break;
		}
		if (nofp->nof_mmap_access) {
			error = nfs_close(np, nofp, nofp->nof_mmap_access, nofp->nof_mmap_deny, ctx);
			if (!nfs_mount_state_error_should_restart(error)) {
				if (error) {                                                                       
					NP(np, "nfs_vnop_mnomap: close of mmap mode failed: %d, %d", error, kauth_cred_getuid(nofp->nof_owner->noo_cred));
				}
				nofp->nof_mmap_access = nofp->nof_mmap_deny = 0;
			}
			if (error) {
				NP(np, "nfs_vnop_mnomap: error %d, %d", error, kauth_cred_getuid(nofp->nof_owner->noo_cred));
			}
		}
		nfs_open_file_clear_busy(nofp);
		nfs_mount_state_in_use_end(nmp, error);
		goto loop;
	}
	lck_mtx_unlock(&np->n_openlock);
	nfs_mount_state_in_use_end(nmp, error);
	return error;
}

  
                                                                  
                                                            
   
struct nfs_lock_owner *
nfs_lock_owner_find(nfsnode_t np, proc_t p, int alloc)
{
	pid_t pid = proc_pid(p);
	struct nfs_lock_owner *nlop, *newnlop = NULL;

tryagain:
	lck_mtx_lock(&np->n_openlock);
	TAILQ_FOREACH(nlop, &np->n_lock_owners, nlo_link) {
		os_ref_count_t newcount;

		if (nlop->nlo_pid != pid) {
			continue;
		}
		if (timevalcmp(&nlop->nlo_pid_start, &p->p_start, ==)) {
			break;
		}
		                                            
		if (os_ref_get_count(&nlop->nlo_refcnt)) {
			TAILQ_REMOVE(&np->n_lock_owners, nlop, nlo_link);
			nlop->nlo_flags &= ~NFS_LOCK_OWNER_LINK;
			newcount = os_ref_release_locked(&nlop->nlo_refcnt);
			lck_mtx_unlock(&np->n_openlock);
			goto tryagain;
		}
		nlop->nlo_pid_start = p->p_start;
		nlop->nlo_seqid = 0;
		nlop->nlo_stategenid = 0;
		break;
	}

	if (!nlop && !newnlop && alloc) {
		lck_mtx_unlock(&np->n_openlock);
		MALLOC(newnlop, struct nfs_lock_owner *, sizeof(struct nfs_lock_owner), M_TEMP, M_WAITOK);
		if (!newnlop) {
			return NULL;
		}
		bzero(newnlop, sizeof(*newnlop));
		lck_mtx_init(&newnlop->nlo_lock, &nfs_open_grp, LCK_ATTR_NULL);
		newnlop->nlo_pid = pid;
		newnlop->nlo_pid_start = p->p_start;
		newnlop->nlo_name = OSAddAtomic(1, &nfs_lock_owner_seqnum);
		TAILQ_INIT(&newnlop->nlo_locks);
		goto tryagain;
	}
	if (!nlop && newnlop) {
		newnlop->nlo_flags |= NFS_LOCK_OWNER_LINK;
		os_ref_init(&newnlop->nlo_refcnt, NULL);
		TAILQ_INSERT_HEAD(&np->n_lock_owners, newnlop, nlo_link);
		nlop = newnlop;
	}
	lck_mtx_unlock(&np->n_openlock);

	if (newnlop && (nlop != newnlop)) {
		nfs_lock_owner_destroy(newnlop);
	}

	if (nlop) {
		nfs_lock_owner_ref(nlop);
	}

	return nlop;
}

  
                                               
   
void
nfs_lock_owner_destroy(struct nfs_lock_owner *nlop)
{
	if (nlop->nlo_open_owner) {
		nfs_open_owner_rele(nlop->nlo_open_owner);
		nlop->nlo_open_owner = NULL;
	}
	lck_mtx_destroy(&nlop->nlo_lock, &nfs_open_grp);
	FREE(nlop, M_TEMP);
}

  
                                            
   
void
nfs_lock_owner_ref(struct nfs_lock_owner *nlop)
{
	lck_mtx_lock(&nlop->nlo_lock);
	os_ref_retain_locked(&nlop->nlo_refcnt);
	lck_mtx_unlock(&nlop->nlo_lock);
}

  
                                                           
                                                                
   
void
nfs_lock_owner_rele(struct nfs_lock_owner *nlop)
{
	os_ref_count_t newcount;

	lck_mtx_lock(&nlop->nlo_lock);
	if (os_ref_get_count(&nlop->nlo_refcnt) < 1) {
		panic("nfs_lock_owner_rele: no refcnt");
	}
	newcount = os_ref_release_locked(&nlop->nlo_refcnt);
	if (!newcount && (nlop->nlo_flags & NFS_LOCK_OWNER_BUSY)) {
		panic("nfs_lock_owner_rele: busy");
	}
	                                                                               
	if (newcount || (nlop->nlo_flags & NFS_LOCK_OWNER_LINK)) {
		lck_mtx_unlock(&nlop->nlo_lock);
		return;
	}
	                                                                     
	lck_mtx_unlock(&nlop->nlo_lock);
	nfs_lock_owner_destroy(nlop);
}

  
                                                    
                                                             
   
int
nfs_lock_owner_set_busy(struct nfs_lock_owner *nlop, thread_t thd)
{
	struct nfsmount *nmp;
	struct timespec ts = { .tv_sec = 2, .tv_nsec = 0 };
	int error = 0, slpflag;

	nmp = nlop->nlo_open_owner->noo_mount;
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	slpflag = (NMFLAG(nmp, INTR) && thd) ? PCATCH : 0;

	lck_mtx_lock(&nlop->nlo_lock);
	while (nlop->nlo_flags & NFS_LOCK_OWNER_BUSY) {
		if ((error = nfs_sigintr(nmp, NULL, thd, 0))) {
			break;
		}
		nlop->nlo_flags |= NFS_LOCK_OWNER_WANT;
		msleep(nlop, &nlop->nlo_lock, slpflag, "nfs_lock_owner_set_busy", &ts);
		slpflag = 0;
	}
	if (!error) {
		nlop->nlo_flags |= NFS_LOCK_OWNER_BUSY;
	}
	lck_mtx_unlock(&nlop->nlo_lock);

	return error;
}

  
                                                                 
                   
   
void
nfs_lock_owner_clear_busy(struct nfs_lock_owner *nlop)
{
	int wanted;

	lck_mtx_lock(&nlop->nlo_lock);
	if (!(nlop->nlo_flags & NFS_LOCK_OWNER_BUSY)) {
		panic("nfs_lock_owner_clear_busy");
	}
	wanted = (nlop->nlo_flags & NFS_LOCK_OWNER_WANT);
	nlop->nlo_flags &= ~(NFS_LOCK_OWNER_BUSY | NFS_LOCK_OWNER_WANT);
	lck_mtx_unlock(&nlop->nlo_lock);
	if (wanted) {
		wakeup(nlop);
	}
}

  
                                                      
                                                         
   
void
nfs_lock_owner_insert_held_lock(struct nfs_lock_owner *nlop, struct nfs_file_lock *newnflp)
{
	struct nfs_file_lock *nflp;

	                                                    
	lck_mtx_lock(&nlop->nlo_lock);
	if ((newnflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) == NFS_FILE_LOCK_STYLE_FLOCK) {
		TAILQ_INSERT_HEAD(&nlop->nlo_locks, newnflp, nfl_lolink);
	} else {
		TAILQ_FOREACH(nflp, &nlop->nlo_locks, nfl_lolink) {
			if (newnflp->nfl_start < nflp->nfl_start) {
				break;
			}
		}
		if (nflp) {
			TAILQ_INSERT_BEFORE(nflp, newnflp, nfl_lolink);
		} else {
			TAILQ_INSERT_TAIL(&nlop->nlo_locks, newnflp, nfl_lolink);
		}
	}
	lck_mtx_unlock(&nlop->nlo_lock);
}

  
                                                 
   
struct nfs_file_lock *
nfs_file_lock_alloc(struct nfs_lock_owner *nlop)
{
	struct nfs_file_lock *nflp = NULL;

	lck_mtx_lock(&nlop->nlo_lock);
	if (!nlop->nlo_alock.nfl_owner) {
		nflp = &nlop->nlo_alock;
		nflp->nfl_owner = nlop;
	}
	lck_mtx_unlock(&nlop->nlo_lock);
	if (!nflp) {
		MALLOC(nflp, struct nfs_file_lock *, sizeof(struct nfs_file_lock), M_TEMP, M_WAITOK);
		if (!nflp) {
			return NULL;
		}
		bzero(nflp, sizeof(*nflp));
		nflp->nfl_flags |= NFS_FILE_LOCK_ALLOC;
		nflp->nfl_owner = nlop;
	}
	nfs_lock_owner_ref(nlop);
	return nflp;
}

  
                                            
   
void
nfs_file_lock_destroy(struct nfs_file_lock *nflp)
{
	struct nfs_lock_owner *nlop = nflp->nfl_owner;

	if (nflp->nfl_flags & NFS_FILE_LOCK_ALLOC) {
		nflp->nfl_owner = NULL;
		FREE(nflp, M_TEMP);
	} else {
		lck_mtx_lock(&nlop->nlo_lock);
		bzero(nflp, sizeof(*nflp));
		lck_mtx_unlock(&nlop->nlo_lock);
	}
	nfs_lock_owner_rele(nlop);
}

  
                                                 
                                                        
   
int
nfs_file_lock_conflict(struct nfs_file_lock *nflp1, struct nfs_file_lock *nflp2, int *willsplit)
{
	                                 
	if ((nflp1->nfl_flags & NFS_FILE_LOCK_DEAD) || (nflp2->nfl_flags & NFS_FILE_LOCK_DEAD)) {
		return 0;
	}
	                                                                    
	if ((nflp1->nfl_owner == nflp2->nfl_owner) &&
	    ((nflp1->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) == (nflp2->nfl_flags & NFS_FILE_LOCK_STYLE_MASK))) {
		if (willsplit && (nflp1->nfl_type != nflp2->nfl_type) &&
		    (nflp1->nfl_start > nflp2->nfl_start) &&
		    (nflp1->nfl_end < nflp2->nfl_end)) {
			*willsplit = 1;
		}
		return 0;
	}
	                                         
	if ((nflp1->nfl_start > nflp2->nfl_end) || (nflp1->nfl_end < nflp2->nfl_start)) {
		return 0;
	}
	                                              
	if ((nflp1->nfl_type != F_WRLCK) && (nflp2->nfl_type != F_WRLCK)) {
		return 0;
	}
	              
	return 1;
}

#if CONFIG_NFS4
  
                                        
   
int
nfs4_setlock_rpc(
	nfsnode_t np,
	struct nfs_open_file *nofp,
	struct nfs_file_lock *nflp,
	int reclaim,
	int flags,
	thread_t thd,
	kauth_cred_t cred)
{
	struct nfs_lock_owner *nlop = nflp->nfl_owner;
	struct nfsmount *nmp;
	struct nfsm_chain nmreq, nmrep;
	uint64_t xid;
	uint32_t locktype;
	int error = 0, lockerror = ENOENT, newlocker, numops, status;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	newlocker = (nlop->nlo_stategenid != nmp->nm_stategenid);
	locktype = (nflp->nfl_flags & NFS_FILE_LOCK_WAIT) ?
	    ((nflp->nfl_type == F_WRLCK) ?
	    NFS_LOCK_TYPE_WRITEW :
	    NFS_LOCK_TYPE_READW) :
	    ((nflp->nfl_type == F_WRLCK) ?
	    NFS_LOCK_TYPE_WRITE :
	    NFS_LOCK_TYPE_READ);
	if (newlocker) {
		error = nfs_open_file_set_busy(nofp, thd);
		if (error) {
			return error;
		}
		error = nfs_open_owner_set_busy(nofp->nof_owner, thd);
		if (error) {
			nfs_open_file_clear_busy(nofp);
			return error;
		}
		if (!nlop->nlo_open_owner) {
			nfs_open_owner_ref(nofp->nof_owner);
			nlop->nlo_open_owner = nofp->nof_owner;
		}
	}
	error = nfs_lock_owner_set_busy(nlop, thd);
	if (error) {
		if (newlocker) {
			nfs_open_owner_clear_busy(nofp->nof_owner);
			nfs_open_file_clear_busy(nofp);
		}
		return error;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                       
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 33 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "lock", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, NFS_VER4, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, np);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_LOCK);
	nfsm_chain_add_32(error, &nmreq, locktype);
	nfsm_chain_add_32(error, &nmreq, reclaim);
	nfsm_chain_add_64(error, &nmreq, nflp->nfl_start);
	nfsm_chain_add_64(error, &nmreq, NFS_LOCK_LENGTH(nflp->nfl_start, nflp->nfl_end));
	nfsm_chain_add_32(error, &nmreq, newlocker);
	if (newlocker) {
		nfsm_chain_add_32(error, &nmreq, nofp->nof_owner->noo_seqid);
		nfsm_chain_add_stateid(error, &nmreq, &nofp->nof_stateid);
		nfsm_chain_add_32(error, &nmreq, nlop->nlo_seqid);
		nfsm_chain_add_lock_owner4(error, &nmreq, nmp, nlop);
	} else {
		nfsm_chain_add_stateid(error, &nmreq, &nlop->nlo_stateid);
		nfsm_chain_add_32(error, &nmreq, nlop->nlo_seqid);
	}
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);

	error = nfs_request2(np, NULL, &nmreq, NFSPROC4_COMPOUND, thd, cred, &si, flags | R_NOINTR, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, np, NFS_VER4, &xid);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_LOCK);
	nfs_owner_seqid_increment(newlocker ? nofp->nof_owner : NULL, nlop, error);
	nfsm_chain_get_stateid(error, &nmrep, &nlop->nlo_stateid);

	                                                                                     
	                                                                                
	if (newlocker && !error) {
		nlop->nlo_stategenid = nmp->nm_stategenid;
	}
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfs_lock_owner_clear_busy(nlop);
	if (newlocker) {
		nfs_open_owner_clear_busy(nofp->nof_owner);
		nfs_open_file_clear_busy(nofp);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

  
                                         
   
int
nfs4_unlock_rpc(
	nfsnode_t np,
	struct nfs_lock_owner *nlop,
	int type,
	uint64_t start,
	uint64_t end,
	int flags,
	thread_t thd,
	kauth_cred_t cred)
{
	struct nfsmount *nmp;
	struct nfsm_chain nmreq, nmrep;
	uint64_t xid;
	int error = 0, lockerror = ENOENT, numops, status;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	error = nfs_lock_owner_set_busy(nlop, NULL);
	if (error) {
		return error;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                        
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 26 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "unlock", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, NFS_VER4, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, np);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_LOCKU);
	nfsm_chain_add_32(error, &nmreq, (type == F_WRLCK) ? NFS_LOCK_TYPE_WRITE : NFS_LOCK_TYPE_READ);
	nfsm_chain_add_32(error, &nmreq, nlop->nlo_seqid);
	nfsm_chain_add_stateid(error, &nmreq, &nlop->nlo_stateid);
	nfsm_chain_add_64(error, &nmreq, start);
	nfsm_chain_add_64(error, &nmreq, NFS_LOCK_LENGTH(start, end));
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);

	error = nfs_request2(np, NULL, &nmreq, NFSPROC4_COMPOUND, thd, cred, &si, flags | R_NOINTR, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, np, NFS_VER4, &xid);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_LOCKU);
	nfs_owner_seqid_increment(NULL, nlop, error);
	nfsm_chain_get_stateid(error, &nmrep, &nlop->nlo_stateid);
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfs_lock_owner_clear_busy(nlop);
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

  
                                         
   
int
nfs4_getlock_rpc(
	nfsnode_t np,
	struct nfs_lock_owner *nlop,
	struct flock *fl,
	uint64_t start,
	uint64_t end,
	vfs_context_t ctx)
{
	struct nfsmount *nmp;
	struct nfsm_chain nmreq, nmrep;
	uint64_t xid, val64 = 0;
	uint32_t val = 0;
	int error = 0, lockerror, numops, status;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	lockerror = ENOENT;
	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                        
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 26 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "locktest", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, NFS_VER4, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, np);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_LOCKT);
	nfsm_chain_add_32(error, &nmreq, (fl->l_type == F_WRLCK) ? NFS_LOCK_TYPE_WRITE : NFS_LOCK_TYPE_READ);
	nfsm_chain_add_64(error, &nmreq, start);
	nfsm_chain_add_64(error, &nmreq, NFS_LOCK_LENGTH(start, end));
	nfsm_chain_add_lock_owner4(error, &nmreq, nmp, nlop);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);

	error = nfs_request(np, NULL, &nmreq, NFSPROC4_COMPOUND, ctx, &si, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, np, NFS_VER4, &xid);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_LOCKT);
	if (error == NFSERR_DENIED) {
		error = 0;
		nfsm_chain_get_64(error, &nmrep, fl->l_start);
		nfsm_chain_get_64(error, &nmrep, val64);
		fl->l_len = (val64 == UINT64_MAX) ? 0 : val64;
		nfsm_chain_get_32(error, &nmrep, val);
		fl->l_type = (val == NFS_LOCK_TYPE_WRITE) ? F_WRLCK : F_RDLCK;
		fl->l_pid = 0;
		fl->l_whence = SEEK_SET;
	} else if (!error) {
		fl->l_type = F_UNLCK;
	}
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}
#endif                  

  
                                               
  
                                                             
                                                            
                                         
   
int
nfs_advlock_getlock(
	nfsnode_t np,
	struct nfs_lock_owner *nlop,
	struct flock *fl,
	uint64_t start,
	uint64_t end,
	vfs_context_t ctx)
{
	struct nfsmount *nmp;
	struct nfs_file_lock *nflp;
	int error = 0, answered = 0;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

restart:
	if ((error = nfs_mount_state_in_use_start(nmp, vfs_context_thread(ctx)))) {
		return error;
	}

	lck_mtx_lock(&np->n_openlock);
	                                            
	TAILQ_FOREACH(nflp, &np->n_locks, nfl_link) {
		if (nflp->nfl_flags & (NFS_FILE_LOCK_BLOCKED | NFS_FILE_LOCK_DEAD)) {
			continue;
		}
		if ((start <= nflp->nfl_end) && (end >= nflp->nfl_start) &&
		    ((fl->l_type == F_WRLCK) || (nflp->nfl_type == F_WRLCK))) {
			break;
		}
	}
	if (nflp) {
		                              
		fl->l_type = nflp->nfl_type;
		fl->l_pid = (nflp->nfl_flags & NFS_FILE_LOCK_STYLE_FLOCK) ? -1 : nflp->nfl_owner->nlo_pid;
		fl->l_start = nflp->nfl_start;
		fl->l_len = NFS_FLOCK_LENGTH(nflp->nfl_start, nflp->nfl_end);
		fl->l_whence = SEEK_SET;
		answered = 1;
	} else if ((np->n_openflags & N_DELEG_WRITE) && !(np->n_openflags & N_DELEG_RETURN)) {
		  
                                                                
                                                                      
     
		fl->l_type = F_UNLCK;
		answered = 1;
	}
	lck_mtx_unlock(&np->n_openlock);
	if (answered) {
		nfs_mount_state_in_use_end(nmp, 0);
		return 0;
	}

	                                                  
	error = nmp->nm_funcs->nf_getlock_rpc(np, nlop, fl, start, end, ctx);

	if (nfs_mount_state_in_use_end(nmp, error)) {
		goto restart;
	}
	return error;
}

  
                                           
  
                                            
                                                 
                                                    
                                                            
                                                         
                                                         
                                                             
   
int
nfs_advlock_setlock(
	nfsnode_t np,
	struct nfs_open_file *nofp,
	struct nfs_lock_owner *nlop,
	int op,
	uint64_t start,
	uint64_t end,
	int style,
	short type,
	vfs_context_t ctx)
{
	struct nfsmount *nmp;
	struct nfs_file_lock *newnflp, *nflp, *nflp2 = NULL, *nextnflp, *flocknflp = NULL;
	struct nfs_file_lock *coalnflp;
	int error = 0, error2, willsplit = 0, delay, slpflag, busy = 0, inuse = 0, restart, inqueue = 0;
	struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	slpflag = NMFLAG(nmp, INTR) ? PCATCH : 0;

	if ((type != F_RDLCK) && (type != F_WRLCK)) {
		return EINVAL;
	}

	                         
	newnflp = nfs_file_lock_alloc(nlop);
	if (!newnflp) {
		return ENOLCK;
	}
	newnflp->nfl_start = start;
	newnflp->nfl_end = end;
	newnflp->nfl_type = type;
	if (op == F_SETLKW) {
		newnflp->nfl_flags |= NFS_FILE_LOCK_WAIT;
	}
	newnflp->nfl_flags |= style;
	newnflp->nfl_flags |= NFS_FILE_LOCK_BLOCKED;

	if ((style == NFS_FILE_LOCK_STYLE_FLOCK) && (type == F_WRLCK)) {
		  
                                                                 
                                                             
                                                                
                                    
     
		nflp = TAILQ_FIRST(&nlop->nlo_locks);
		if (nflp && ((nflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) != NFS_FILE_LOCK_STYLE_FLOCK)) {
			nflp = NULL;
		}
		if (nflp && (nflp->nfl_type != F_RDLCK)) {
			nflp = NULL;
		}
		flocknflp = nflp;
	}

restart:
	restart = 0;
	error = nfs_mount_state_in_use_start(nmp, vfs_context_thread(ctx));
	if (error) {
		goto error_out;
	}
	inuse = 1;
	if (np->n_flag & NREVOKE) {
		error = EIO;
		nfs_mount_state_in_use_end(nmp, 0);
		inuse = 0;
		goto error_out;
	}
#if CONFIG_NFS4
	if (nofp->nof_flags & NFS_OPEN_FILE_REOPEN) {
		nfs_mount_state_in_use_end(nmp, 0);
		inuse = 0;
		error = nfs4_reopen(nofp, vfs_context_thread(ctx));
		if (error) {
			goto error_out;
		}
		goto restart;
	}
#endif

	lck_mtx_lock(&np->n_openlock);
	if (!inqueue) {
		                                          
		TAILQ_INSERT_HEAD(&np->n_locks, newnflp, nfl_link);
		inqueue = 1;
	}

	                                                                 
	for (nflp = TAILQ_NEXT(newnflp, nfl_link); nflp; nflp = nextnflp) {
		nextnflp = TAILQ_NEXT(nflp, nfl_link);
		if (!nfs_file_lock_conflict(newnflp, nflp, &willsplit)) {
			continue;
		}
		              
		if (!(newnflp->nfl_flags & NFS_FILE_LOCK_WAIT)) {
			error = EAGAIN;
			break;
		}
		                                              
		if (nflp->nfl_blockcnt == UINT_MAX) {
			error = ENOLCK;
			break;
		}
		nflp->nfl_blockcnt++;
		do {
			if (flocknflp) {
				                                                            
				lck_mtx_unlock(&np->n_openlock);
				nfs_mount_state_in_use_end(nmp, 0);
				inuse = 0;
				error = nfs_advlock_unlock(np, nofp, nlop, 0, UINT64_MAX, NFS_FILE_LOCK_STYLE_FLOCK, ctx);
				flocknflp = NULL;
				if (!error) {
					error = nfs_mount_state_in_use_start(nmp, vfs_context_thread(ctx));
				}
				if (error) {
					lck_mtx_lock(&np->n_openlock);
					break;
				}
				inuse = 1;
				lck_mtx_lock(&np->n_openlock);
				                                                    
				if (!nfs_file_lock_conflict(newnflp, nflp, NULL)) {
					break;
				}
			}
			msleep(nflp, &np->n_openlock, slpflag, "nfs_advlock_setlock_blocked", &ts);
			slpflag = 0;
			error = nfs_sigintr(NFSTONMP(np), NULL, vfs_context_thread(ctx), 0);
			if (!error && (nmp->nm_state & NFSSTA_RECOVER)) {
				                                                     
				restart = 1;
				lck_mtx_unlock(&np->n_openlock);
				nfs_mount_state_in_use_end(nmp, 0);
				inuse = 0;
				lck_mtx_lock(&np->n_openlock);
				break;
			}
			if (!error && (np->n_flag & NREVOKE)) {
				error = EIO;
			}
		} while (!error && nfs_file_lock_conflict(newnflp, nflp, NULL));
		nflp->nfl_blockcnt--;
		if ((nflp->nfl_flags & NFS_FILE_LOCK_DEAD) && !nflp->nfl_blockcnt) {
			TAILQ_REMOVE(&np->n_locks, nflp, nfl_link);
			nfs_file_lock_destroy(nflp);
		}
		if (error || restart) {
			break;
		}
		                                                                                  
		                                                                   
		nextnflp = TAILQ_NEXT(newnflp, nfl_link);
	}
	lck_mtx_unlock(&np->n_openlock);
	if (restart) {
		goto restart;
	}
	if (error) {
		goto error_out;
	}

	if (willsplit) {
		  
                                                      
                                                         
                                                                 
     
		nflp2 = nfs_file_lock_alloc(nlop);
		if (!nflp2) {
			error = ENOLCK;
			goto error_out;
		}
	}

	                                                                    
	if ((error = nfs_open_state_set_busy(np, vfs_context_thread(ctx)))) {
		goto error_out;
	}
	busy = 1;
	delay = 0;
	do {
#if CONFIG_NFS4
		                                                          
		if ((np->n_openflags & N_DELEG_MASK) && !(np->n_openflags & N_DELEG_RETURN)) {
			if (np->n_openflags & N_DELEG_WRITE) {
				                                                           
				newnflp->nfl_flags |= NFS_FILE_LOCK_DELEGATED;
				error = 0;
				                                                   
				if (!nlop->nlo_open_owner) {
					nfs_open_owner_ref(nofp->nof_owner);
					nlop->nlo_open_owner = nofp->nof_owner;
				}
				break;
			} else {
				  
                                                              
                                                                 
                                                                     
                                   
       
				if ((!nofp->nof_rw_drw && !nofp->nof_w_drw && !nofp->nof_r_drw &&
				    !nofp->nof_rw_dw && !nofp->nof_w_dw && !nofp->nof_r_dw &&
				    !nofp->nof_rw && !nofp->nof_w && !nofp->nof_r) &&
				    (nofp->nof_d_rw_drw || nofp->nof_d_w_drw || nofp->nof_d_r_drw ||
				    nofp->nof_d_rw_dw || nofp->nof_d_w_dw || nofp->nof_d_r_dw ||
				    nofp->nof_d_rw || nofp->nof_d_w || nofp->nof_d_r)) {
					error = nfs4_claim_delegated_state_for_open_file(nofp, 0);
					if (error) {
						break;
					}
				}
			}
		}
#endif
		if (np->n_flag & NREVOKE) {
			error = EIO;
		}
		if (!error) {
			if (busy) {
				nfs_open_state_clear_busy(np);
				busy = 0;
			}
			error = nmp->nm_funcs->nf_setlock_rpc(np, nofp, newnflp, 0, 0, vfs_context_thread(ctx), vfs_context_ucred(ctx));
			if (!busy && !nfs_open_state_set_busy(np, vfs_context_thread(ctx))) {
				busy = 1;
			}
		}
		if (!error || ((error != NFSERR_DENIED) && (error != NFSERR_GRACE))) {
			break;
		}
		                                                               
		if ((error == NFSERR_DENIED) && !(newnflp->nfl_flags & NFS_FILE_LOCK_WAIT)) {
			error = EAGAIN;
			break;
		}
		if (flocknflp) {
			                                                            
			nfs_open_state_clear_busy(np);
			busy = 0;
			if (inuse) {
				nfs_mount_state_in_use_end(nmp, 0);
				inuse = 0;
			}
			error2 = nfs_advlock_unlock(np, nofp, nlop, 0, UINT64_MAX, NFS_FILE_LOCK_STYLE_FLOCK, ctx);
			flocknflp = NULL;
			if (!error2) {
				error2 = nfs_mount_state_in_use_start(nmp, vfs_context_thread(ctx));
			}
			if (!error2) {
				inuse = 1;
				error2 = nfs_open_state_set_busy(np, vfs_context_thread(ctx));
			}
			if (error2) {
				error = error2;
				break;
			}
			busy = 1;
		}
		  
                                                  
                                                                                  
     
		if ((nmp->nm_vers >= NFS_VER4) || (error == NFSERR_GRACE)) {
			if (error == NFSERR_GRACE) {
				delay = 4;
			}
			if (delay < 4) {
				delay++;
			}
			tsleep(newnflp, slpflag, "nfs_advlock_setlock_delay", delay * (hz / 2));
			slpflag = 0;
		}
		error = nfs_sigintr(NFSTONMP(np), NULL, vfs_context_thread(ctx), 0);
		if (!error && (nmp->nm_state & NFSSTA_RECOVER)) {
			                                                     
			nfs_open_state_clear_busy(np);
			busy = 0;
			if (inuse) {
				nfs_mount_state_in_use_end(nmp, 0);
				inuse = 0;
			}
			goto restart;
		}
		if (!error && (np->n_flag & NREVOKE)) {
			error = EIO;
		}
	} while (!error);

error_out:
	if (nfs_mount_state_error_should_restart(error)) {
		                                                  
		if (busy) {
			nfs_open_state_clear_busy(np);
			busy = 0;
		}
		if (inuse) {
			nfs_mount_state_in_use_end(nmp, error);
			inuse = 0;
		}
		goto restart;
	}
	lck_mtx_lock(&np->n_openlock);
	newnflp->nfl_flags &= ~NFS_FILE_LOCK_BLOCKED;
	if (error) {
		newnflp->nfl_flags |= NFS_FILE_LOCK_DEAD;
		if (newnflp->nfl_blockcnt) {
			                                         
			wakeup(newnflp);
		} else {
			                                               
			if (inqueue) {
				TAILQ_REMOVE(&np->n_locks, newnflp, nfl_link);
			}
			nfs_file_lock_destroy(newnflp);
		}
		lck_mtx_unlock(&np->n_openlock);
		if (busy) {
			nfs_open_state_clear_busy(np);
		}
		if (inuse) {
			nfs_mount_state_in_use_end(nmp, error);
		}
		if (nflp2) {
			nfs_file_lock_destroy(nflp2);
		}
		return error;
	}

	                             

	  
                             
   
                                        
                                             
                                                          
    
	TAILQ_FOREACH_SAFE(nflp, &np->n_locks, nfl_link, nextnflp) {
		if (nflp == newnflp) {
			continue;
		}
		if (nflp->nfl_flags & (NFS_FILE_LOCK_BLOCKED | NFS_FILE_LOCK_DEAD)) {
			continue;
		}
		if (nflp->nfl_owner != nlop) {
			continue;
		}
		if ((newnflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) != (nflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK)) {
			continue;
		}
		if ((newnflp->nfl_start > nflp->nfl_end) || (newnflp->nfl_end < nflp->nfl_start)) {
			continue;
		}
		                          
		if ((newnflp->nfl_start <= nflp->nfl_start) && (newnflp->nfl_end >= nflp->nfl_end)) {
			                                        
			nflp->nfl_flags |= NFS_FILE_LOCK_DEAD;
			lck_mtx_lock(&nlop->nlo_lock);
			TAILQ_REMOVE(&nlop->nlo_locks, nflp, nfl_lolink);
			lck_mtx_unlock(&nlop->nlo_lock);
			                                                 
		} else if ((newnflp->nfl_start > nflp->nfl_start) && (newnflp->nfl_end < nflp->nfl_end)) {
			                                                      
			                                                    
			                                                          
			nflp2->nfl_flags |= (nflp->nfl_flags & (NFS_FILE_LOCK_STYLE_MASK | NFS_FILE_LOCK_DELEGATED));
			nflp2->nfl_type = nflp->nfl_type;
			nflp2->nfl_start = newnflp->nfl_end + 1;
			nflp2->nfl_end = nflp->nfl_end;
			nflp->nfl_end = newnflp->nfl_start - 1;
			TAILQ_INSERT_AFTER(&np->n_locks, nflp, nflp2, nfl_link);
			nfs_lock_owner_insert_held_lock(nlop, nflp2);
			nextnflp = nflp2;
			nflp2 = NULL;
		} else if (newnflp->nfl_start > nflp->nfl_start) {
			                                        
			nflp->nfl_end = newnflp->nfl_start - 1;
		} else if (newnflp->nfl_end < nflp->nfl_end) {
			                                          
			nflp->nfl_start = newnflp->nfl_end + 1;
		}
		if (nflp->nfl_blockcnt) {
			                                         
			wakeup(nflp);
		} else if (nflp->nfl_flags & NFS_FILE_LOCK_DEAD) {
			                                            
			TAILQ_REMOVE(&np->n_locks, nflp, nfl_link);
			nfs_file_lock_destroy(nflp);
		}
	}

	nfs_lock_owner_insert_held_lock(nlop, newnflp);

	  
                                                  
    
	if ((style == NFS_FILE_LOCK_STYLE_POSIX) && (nofp->nof_flags & NFS_OPEN_FILE_POSIXLOCK)) {
		  
                                                                      
                                                                      
                                                                      
                                                                         
                                                                      
                                                                     
                                                                       
                                                   
     
		TAILQ_FOREACH_SAFE(nflp, &np->n_locks, nfl_link, nextnflp) {
			if (nflp->nfl_flags & (NFS_FILE_LOCK_BLOCKED | NFS_FILE_LOCK_DEAD)) {
				continue;
			}
			if (nflp->nfl_owner != nlop) {
				continue;
			}
			if ((nflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) != NFS_FILE_LOCK_STYLE_POSIX) {
				continue;
			}
			if (((coalnflp = TAILQ_PREV(nflp, nfs_file_lock_queue, nfl_lolink))) &&
			    ((coalnflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) == NFS_FILE_LOCK_STYLE_POSIX) &&
			    (coalnflp->nfl_type == nflp->nfl_type) &&
			    (coalnflp->nfl_end == (nflp->nfl_start - 1))) {
				coalnflp->nfl_end = nflp->nfl_end;
				nflp->nfl_flags |= NFS_FILE_LOCK_DEAD;
				lck_mtx_lock(&nlop->nlo_lock);
				TAILQ_REMOVE(&nlop->nlo_locks, nflp, nfl_lolink);
				lck_mtx_unlock(&nlop->nlo_lock);
			} else if (((coalnflp = TAILQ_NEXT(nflp, nfl_lolink))) &&
			    ((coalnflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) == NFS_FILE_LOCK_STYLE_POSIX) &&
			    (coalnflp->nfl_type == nflp->nfl_type) &&
			    (coalnflp->nfl_start == (nflp->nfl_end + 1))) {
				coalnflp->nfl_start = nflp->nfl_start;
				nflp->nfl_flags |= NFS_FILE_LOCK_DEAD;
				lck_mtx_lock(&nlop->nlo_lock);
				TAILQ_REMOVE(&nlop->nlo_locks, nflp, nfl_lolink);
				lck_mtx_unlock(&nlop->nlo_lock);
			}
			if (!(nflp->nfl_flags & NFS_FILE_LOCK_DEAD)) {
				continue;
			}
			if (nflp->nfl_blockcnt) {
				                                         
				wakeup(nflp);
			} else {
				                                            
				TAILQ_REMOVE(&np->n_locks, nflp, nfl_link);
				nfs_file_lock_destroy(nflp);
			}
		}
	}

	lck_mtx_unlock(&np->n_openlock);
	nfs_open_state_clear_busy(np);

	if (inuse) {
		nfs_mount_state_in_use_end(nmp, error);
	}
	if (nflp2) {
		nfs_file_lock_destroy(nflp2);
	}
	return error;
}

  
                                                         
   
int
nfs_advlock_unlock(
	nfsnode_t np,
	struct nfs_open_file *nofp
#if !CONFIG_NFS4
	__unused
#endif
	,
	struct nfs_lock_owner *nlop,
	uint64_t start,
	uint64_t end,
	int style,
	vfs_context_t ctx)
{
	struct nfsmount *nmp;
	struct nfs_file_lock *nflp, *nextnflp, *newnflp = NULL;
	int error = 0, willsplit = 0, send_unlock_rpcs = 1;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

restart:
	if ((error = nfs_mount_state_in_use_start(nmp, NULL))) {
		return error;
	}
#if CONFIG_NFS4
	if (nofp->nof_flags & NFS_OPEN_FILE_REOPEN) {
		nfs_mount_state_in_use_end(nmp, 0);
		error = nfs4_reopen(nofp, NULL);
		if (error) {
			return error;
		}
		goto restart;
	}
#endif
	if ((error = nfs_open_state_set_busy(np, NULL))) {
		nfs_mount_state_in_use_end(nmp, error);
		return error;
	}

	lck_mtx_lock(&np->n_openlock);
	if ((start > 0) && (end < UINT64_MAX) && !willsplit) {
		  
                                                                       
                                                                    
                                             
     
		TAILQ_FOREACH_SAFE(nflp, &np->n_locks, nfl_link, nextnflp) {
			if (nflp->nfl_flags & (NFS_FILE_LOCK_BLOCKED | NFS_FILE_LOCK_DEAD)) {
				continue;
			}
			if (nflp->nfl_owner != nlop) {
				continue;
			}
			if ((nflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) != style) {
				continue;
			}
			if ((start > nflp->nfl_end) || (end < nflp->nfl_start)) {
				continue;
			}
			if ((start > nflp->nfl_start) && (end < nflp->nfl_end)) {
				willsplit = 1;
				break;
			}
		}
		if (willsplit) {
			lck_mtx_unlock(&np->n_openlock);
			nfs_open_state_clear_busy(np);
			nfs_mount_state_in_use_end(nmp, 0);
			newnflp = nfs_file_lock_alloc(nlop);
			if (!newnflp) {
				return ENOMEM;
			}
			goto restart;
		}
	}

	  
                                             
   
                                                                   
                                                                    
                                                                    
                                                                     
                                                                    
    

	  
                                                           
                                                                   
                                                                     
                                                               
    
	if ((style == NFS_FILE_LOCK_STYLE_POSIX) &&
	    ((nflp = TAILQ_FIRST(&nlop->nlo_locks))) &&
	    ((nflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) == NFS_FILE_LOCK_STYLE_FLOCK)) {
		send_unlock_rpcs = 0;
	}
	if ((style == NFS_FILE_LOCK_STYLE_FLOCK) &&
	    ((nflp = TAILQ_FIRST(&nlop->nlo_locks))) &&
	    ((nflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) == NFS_FILE_LOCK_STYLE_FLOCK) &&
	    ((nflp = TAILQ_NEXT(nflp, nfl_lolink))) &&
	    ((nflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) == NFS_FILE_LOCK_STYLE_POSIX)) {
		uint64_t s = 0;
		int type = TAILQ_FIRST(&nlop->nlo_locks)->nfl_type;
		int delegated = (TAILQ_FIRST(&nlop->nlo_locks)->nfl_flags & NFS_FILE_LOCK_DELEGATED);
		while (!delegated && nflp) {
			if ((nflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) == NFS_FILE_LOCK_STYLE_POSIX) {
				                                          
				lck_mtx_unlock(&np->n_openlock);
				error = nmp->nm_funcs->nf_unlock_rpc(np, nlop, type, s, nflp->nfl_start - 1, 0,
				    vfs_context_thread(ctx), vfs_context_ucred(ctx));
				if (nfs_mount_state_error_should_restart(error)) {
					nfs_open_state_clear_busy(np);
					nfs_mount_state_in_use_end(nmp, error);
					goto restart;
				}
				lck_mtx_lock(&np->n_openlock);
				if (error) {
					goto out;
				}
				s = nflp->nfl_end + 1;
			}
			nflp = TAILQ_NEXT(nflp, nfl_lolink);
		}
		if (!delegated) {
			lck_mtx_unlock(&np->n_openlock);
			error = nmp->nm_funcs->nf_unlock_rpc(np, nlop, type, s, end, 0,
			    vfs_context_thread(ctx), vfs_context_ucred(ctx));
			if (nfs_mount_state_error_should_restart(error)) {
				nfs_open_state_clear_busy(np);
				nfs_mount_state_in_use_end(nmp, error);
				goto restart;
			}
			lck_mtx_lock(&np->n_openlock);
			if (error) {
				goto out;
			}
		}
		send_unlock_rpcs = 0;
	}

	TAILQ_FOREACH_SAFE(nflp, &np->n_locks, nfl_link, nextnflp) {
		if (nflp->nfl_flags & (NFS_FILE_LOCK_BLOCKED | NFS_FILE_LOCK_DEAD)) {
			continue;
		}
		if (nflp->nfl_owner != nlop) {
			continue;
		}
		if ((nflp->nfl_flags & NFS_FILE_LOCK_STYLE_MASK) != style) {
			continue;
		}
		if ((start > nflp->nfl_end) || (end < nflp->nfl_start)) {
			continue;
		}
		                          
		if ((start <= nflp->nfl_start) && (end >= nflp->nfl_end)) {
			                                        
			if (send_unlock_rpcs && !(nflp->nfl_flags & NFS_FILE_LOCK_DELEGATED)) {
				lck_mtx_unlock(&np->n_openlock);
				error = nmp->nm_funcs->nf_unlock_rpc(np, nlop, nflp->nfl_type, nflp->nfl_start, nflp->nfl_end, 0,
				    vfs_context_thread(ctx), vfs_context_ucred(ctx));
				if (nfs_mount_state_error_should_restart(error)) {
					nfs_open_state_clear_busy(np);
					nfs_mount_state_in_use_end(nmp, error);
					goto restart;
				}
				lck_mtx_lock(&np->n_openlock);
			}
			nextnflp = TAILQ_NEXT(nflp, nfl_link);
			if (error) {
				break;
			}
			nflp->nfl_flags |= NFS_FILE_LOCK_DEAD;
			lck_mtx_lock(&nlop->nlo_lock);
			TAILQ_REMOVE(&nlop->nlo_locks, nflp, nfl_lolink);
			lck_mtx_unlock(&nlop->nlo_lock);
			                                                 
		} else if ((start > nflp->nfl_start) && (end < nflp->nfl_end)) {
			                                                      
			                                                    
			if (send_unlock_rpcs && !(nflp->nfl_flags & NFS_FILE_LOCK_DELEGATED)) {
				lck_mtx_unlock(&np->n_openlock);
				error = nmp->nm_funcs->nf_unlock_rpc(np, nlop, nflp->nfl_type, start, end, 0,
				    vfs_context_thread(ctx), vfs_context_ucred(ctx));
				if (nfs_mount_state_error_should_restart(error)) {
					nfs_open_state_clear_busy(np);
					nfs_mount_state_in_use_end(nmp, error);
					goto restart;
				}
				lck_mtx_lock(&np->n_openlock);
			}
			if (error) {
				break;
			}
			                                                         
			newnflp->nfl_flags |= (nflp->nfl_flags & (NFS_FILE_LOCK_STYLE_MASK | NFS_FILE_LOCK_DELEGATED));
			newnflp->nfl_type = nflp->nfl_type;
			newnflp->nfl_start = end + 1;
			newnflp->nfl_end = nflp->nfl_end;
			nflp->nfl_end = start - 1;
			TAILQ_INSERT_AFTER(&np->n_locks, nflp, newnflp, nfl_link);
			nfs_lock_owner_insert_held_lock(nlop, newnflp);
			nextnflp = newnflp;
			newnflp = NULL;
		} else if (start > nflp->nfl_start) {
			                                        
			if (send_unlock_rpcs && !(nflp->nfl_flags & NFS_FILE_LOCK_DELEGATED)) {
				lck_mtx_unlock(&np->n_openlock);
				error = nmp->nm_funcs->nf_unlock_rpc(np, nlop, nflp->nfl_type, start, nflp->nfl_end, 0,
				    vfs_context_thread(ctx), vfs_context_ucred(ctx));
				if (nfs_mount_state_error_should_restart(error)) {
					nfs_open_state_clear_busy(np);
					nfs_mount_state_in_use_end(nmp, error);
					goto restart;
				}
				lck_mtx_lock(&np->n_openlock);
			}
			nextnflp = TAILQ_NEXT(nflp, nfl_link);
			if (error) {
				break;
			}
			nflp->nfl_end = start - 1;
		} else if (end < nflp->nfl_end) {
			                                          
			if (send_unlock_rpcs && !(nflp->nfl_flags & NFS_FILE_LOCK_DELEGATED)) {
				lck_mtx_unlock(&np->n_openlock);
				error = nmp->nm_funcs->nf_unlock_rpc(np, nlop, nflp->nfl_type, nflp->nfl_start, end, 0,
				    vfs_context_thread(ctx), vfs_context_ucred(ctx));
				if (nfs_mount_state_error_should_restart(error)) {
					nfs_open_state_clear_busy(np);
					nfs_mount_state_in_use_end(nmp, error);
					goto restart;
				}
				lck_mtx_lock(&np->n_openlock);
			}
			nextnflp = TAILQ_NEXT(nflp, nfl_link);
			if (error) {
				break;
			}
			nflp->nfl_start = end + 1;
		}
		if (nflp->nfl_blockcnt) {
			                                         
			wakeup(nflp);
		} else if (nflp->nfl_flags & NFS_FILE_LOCK_DEAD) {
			                                            
			TAILQ_REMOVE(&np->n_locks, nflp, nfl_link);
			nfs_file_lock_destroy(nflp);
		}
	}
out:
	lck_mtx_unlock(&np->n_openlock);
	nfs_open_state_clear_busy(np);
	nfs_mount_state_in_use_end(nmp, 0);

	if (newnflp) {
		nfs_file_lock_destroy(newnflp);
	}
	return error;
}

  
                              
   
int
nfs_vnop_advlock(
	struct vnop_advlock_args     
                                                                 
                                                  
                                                  
                                              
                                                        
                                                 
                                                             
                                         *ap)
{
	vnode_t vp = ap->a_vp;
	nfsnode_t np = VTONFS(ap->a_vp);
	struct flock *fl = ap->a_fl;
	int op = ap->a_op;
	int flags = ap->a_flags;
	vfs_context_t ctx = ap->a_context;
	struct nfsmount *nmp;
	struct nfs_open_owner *noop = NULL;
	struct nfs_open_file *nofp = NULL;
	struct nfs_lock_owner *nlop = NULL;
	off_t lstart;
	uint64_t start, end;
	int error = 0, modified, style;
	enum vtype vtype;
#define OFF_MAX QUAD_MAX

	nmp = VTONMP(ap->a_vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	lck_mtx_lock(&nmp->nm_lock);
	if ((nmp->nm_vers <= NFS_VER3) && (nmp->nm_lockmode == NFS_LOCK_MODE_DISABLED)) {
		lck_mtx_unlock(&nmp->nm_lock);
		return ENOTSUP;
	}
	lck_mtx_unlock(&nmp->nm_lock);

	if (np->n_flag & NREVOKE) {
		return EIO;
	}
	vtype = vnode_vtype(ap->a_vp);
	if (vtype == VDIR) {                                          
		return 0;
	}
	if (vtype != VREG) {                                                   
		return EINVAL;
	}

	                                                       
	switch (fl->l_whence) {
	case SEEK_SET:
	case SEEK_CUR:
		  
                                                          
                                          
     
		lstart = fl->l_start;
		break;
	case SEEK_END:
		                                                   
		                                                   
		if ((error = nfs_node_lock(np))) {
			return error;
		}
		modified = (np->n_flag & NMODIFIED);
		nfs_node_unlock(np);
		if (modified && ((error = nfs_vinvalbuf(vp, V_SAVE, ctx, 1)))) {
			return error;
		}
		if ((error = nfs_getattr(np, NULL, ctx, NGA_UNCACHED))) {
			return error;
		}
		nfs_data_lock(np, NFS_DATA_LOCK_SHARED);
		if ((np->n_size > OFF_MAX) ||
		    ((fl->l_start > 0) && (np->n_size > (u_quad_t)(OFF_MAX - fl->l_start)))) {
			error = EOVERFLOW;
		}
		lstart = np->n_size + fl->l_start;
		nfs_data_unlock(np);
		if (error) {
			return error;
		}
		break;
	default:
		return EINVAL;
	}
	if (lstart < 0) {
		return EINVAL;
	}
	start = lstart;
	if (fl->l_len == 0) {
		end = UINT64_MAX;
	} else if (fl->l_len > 0) {
		if ((fl->l_len - 1) > (OFF_MAX - lstart)) {
			return EOVERFLOW;
		}
		end = start - 1 + fl->l_len;
	} else {                        
		if ((lstart + fl->l_len) < 0) {
			return EINVAL;
		}
		end = start - 1;
		start += fl->l_len;
	}
	if ((nmp->nm_vers == NFS_VER2) && ((start > INT32_MAX) || (fl->l_len && (end > INT32_MAX)))) {
		return EINVAL;
	}

	style = (flags & F_FLOCK) ? NFS_FILE_LOCK_STYLE_FLOCK : NFS_FILE_LOCK_STYLE_POSIX;
	if ((style == NFS_FILE_LOCK_STYLE_FLOCK) && ((start != 0) || (end != UINT64_MAX))) {
		return EINVAL;
	}

	                                              
	nlop = nfs_lock_owner_find(np, vfs_context_proc(ctx), (op != F_UNLCK));
	if (!nlop) {
		error = (op == F_UNLCK) ? 0 : ENOMEM;
		if (error) {
			NP(np, "nfs_vnop_advlock: no lock owner, error %d", error);
		}
		goto out;
	}

	if (op == F_GETLK) {
		error = nfs_advlock_getlock(np, nlop, fl, start, end, ctx);
	} else {
		                         
		noop = nfs_open_owner_find(nmp, vfs_context_ucred(ctx), 0);
		if (!noop) {
			NP(np, "nfs_vnop_advlock: no open owner %d", kauth_cred_getuid(vfs_context_ucred(ctx)));
			error = EPERM;
			goto out;
		}
		                        
#if CONFIG_NFS4
restart:
#endif
		error = nfs_open_file_find(np, noop, &nofp, 0, 0, 0);
		if (error) {
			error = EBADF;
		}
		if (!error && (nofp->nof_flags & NFS_OPEN_FILE_LOST)) {
			NP(np, "nfs_vnop_advlock: LOST %d", kauth_cred_getuid(nofp->nof_owner->noo_cred));
			error = EIO;
		}
#if CONFIG_NFS4
		if (!error && (nofp->nof_flags & NFS_OPEN_FILE_REOPEN)) {
			error = nfs4_reopen(nofp, ((op == F_UNLCK) ? NULL : vfs_context_thread(ctx)));
			nofp = NULL;
			if (!error) {
				goto restart;
			}
		}
#endif
		if (error) {
			NP(np, "nfs_vnop_advlock: no open file %d, %d", error, kauth_cred_getuid(noop->noo_cred));
			goto out;
		}
		if (op == F_UNLCK) {
			error = nfs_advlock_unlock(np, nofp, nlop, start, end, style, ctx);
		} else if ((op == F_SETLK) || (op == F_SETLKW)) {
			if ((op == F_SETLK) && (flags & F_WAIT)) {
				op = F_SETLKW;
			}
			error = nfs_advlock_setlock(np, nofp, nlop, op, start, end, style, fl->l_type, ctx);
		} else {
			                                
			error = EINVAL;
		}
	}

out:
	if (nlop) {
		nfs_lock_owner_rele(nlop);
	}
	if (noop) {
		nfs_open_owner_rele(noop);
	}
	return error;
}

  
                                                    
   
int
nfs_check_for_locks(struct nfs_open_owner *noop, struct nfs_open_file *nofp)
{
	struct nfs_lock_owner *nlop;

	TAILQ_FOREACH(nlop, &nofp->nof_np->n_lock_owners, nlo_link) {
		if (nlop->nlo_open_owner != noop) {
			continue;
		}
		if (!TAILQ_EMPTY(&nlop->nlo_locks)) {
			break;
		}
	}
	return nlop ? 1 : 0;
}

#if CONFIG_NFS4
  
                                                              
   
int
nfs4_reopen(struct nfs_open_file *nofp, thread_t thd)
{
	struct nfs_open_owner *noop = nofp->nof_owner;
	struct nfsmount *nmp = NFSTONMP(nofp->nof_np);
	nfsnode_t np = nofp->nof_np;
	vnode_t vp = NFSTOV(np);
	vnode_t dvp = NULL;
	struct componentname cn;
	const char *vname = NULL;
	const char *name = NULL;
	uint32_t namelen;
	char smallname[128];
	char *filename = NULL;
	int error = 0, done = 0, slpflag = NMFLAG(nmp, INTR) ? PCATCH : 0;
	struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };

	lck_mtx_lock(&nofp->nof_lock);
	while (nofp->nof_flags & NFS_OPEN_FILE_REOPENING) {
		if ((error = nfs_sigintr(nmp, NULL, thd, 0))) {
			break;
		}
		msleep(&nofp->nof_flags, &nofp->nof_lock, slpflag | (PZERO - 1), "nfsreopenwait", &ts);
		slpflag = 0;
	}
	if (error || !(nofp->nof_flags & NFS_OPEN_FILE_REOPEN)) {
		lck_mtx_unlock(&nofp->nof_lock);
		return error;
	}
	nofp->nof_flags |= NFS_OPEN_FILE_REOPENING;
	lck_mtx_unlock(&nofp->nof_lock);

	nfs_node_lock_force(np);
	if ((vnode_vtype(vp) != VDIR) && np->n_sillyrename) {
		  
                                                    
                                                   
     
		struct nfs_sillyrename *nsp = np->n_sillyrename;
		dvp = NFSTOV(nsp->nsr_dnp);
		if ((error = vnode_get(dvp))) {
			dvp = NULLVP;
			nfs_node_unlock(np);
			goto out;
		}
		name = nsp->nsr_name;
	} else {
		  
                                                                
                                                                  
                                                                 
                                            
     
		if ((np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR) &&
		    (((dvp = np->n_parent)) && (error = vnode_get(dvp)))) {
			dvp = NULL;
		}
		if (!dvp) {
			dvp = vnode_getparent(vp);
		}
		vname = vnode_getname(vp);
		if (!dvp || !vname) {
			if (!error) {
				error = EIO;
			}
			nfs_node_unlock(np);
			goto out;
		}
		name = vname;
	}
	filename = &smallname[0];
	namelen = snprintf(filename, sizeof(smallname), "%s", name);
	if (namelen >= sizeof(smallname)) {
		MALLOC(filename, char *, namelen + 1, M_TEMP, M_WAITOK);
		if (!filename) {
			error = ENOMEM;
			goto out;
		}
		snprintf(filename, namelen + 1, "%s", name);
	}
	nfs_node_unlock(np);
	bzero(&cn, sizeof(cn));
	cn.cn_nameptr = filename;
	cn.cn_namelen = namelen;

restart:
	done = 0;
	if ((error = nfs_mount_state_in_use_start(nmp, thd))) {
		goto out;
	}

	if (nofp->nof_rw) {
		error = nfs4_open_reopen_rpc(nofp, thd, noop->noo_cred, &cn, dvp, &vp, NFS_OPEN_SHARE_ACCESS_BOTH, NFS_OPEN_SHARE_DENY_NONE);
	}
	if (!error && nofp->nof_w) {
		error = nfs4_open_reopen_rpc(nofp, thd, noop->noo_cred, &cn, dvp, &vp, NFS_OPEN_SHARE_ACCESS_WRITE, NFS_OPEN_SHARE_DENY_NONE);
	}
	if (!error && nofp->nof_r) {
		error = nfs4_open_reopen_rpc(nofp, thd, noop->noo_cred, &cn, dvp, &vp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_NONE);
	}

	if (nfs_mount_state_in_use_end(nmp, error)) {
		if (error == NFSERR_GRACE) {
			goto restart;
		}
		printf("nfs4_reopen: RPC failed, error %d, lost %d, %s\n", error,
		    (nofp->nof_flags & NFS_OPEN_FILE_LOST) ? 1 : 0, name ? name : "???");
		error = 0;
		goto out;
	}
	done = 1;
out:
	if (error && (error != EINTR) && (error != ERESTART)) {
		nfs_revoke_open_state_for_node(np);
	}
	lck_mtx_lock(&nofp->nof_lock);
	nofp->nof_flags &= ~NFS_OPEN_FILE_REOPENING;
	if (done) {
		nofp->nof_flags &= ~NFS_OPEN_FILE_REOPEN;
	} else if (error) {
		printf("nfs4_reopen: failed, error %d, lost %d, %s\n", error,
		    (nofp->nof_flags & NFS_OPEN_FILE_LOST) ? 1 : 0, name ? name : "???");
	}
	lck_mtx_unlock(&nofp->nof_lock);
	if (filename && (filename != &smallname[0])) {
		FREE(filename, M_TEMP);
	}
	if (vname) {
		vnode_putname(vname);
	}
	if (dvp != NULLVP) {
		vnode_put(dvp);
	}
	return error;
}

  
                                                
   
int
nfs4_open_rpc(
	struct nfs_open_file *nofp,
	vfs_context_t ctx,
	struct componentname *cnp,
	struct vnode_attr *vap,
	vnode_t dvp,
	vnode_t *vpp,
	int create,
	int share_access,
	int share_deny)
{
	return nfs4_open_rpc_internal(nofp, ctx, vfs_context_thread(ctx), vfs_context_ucred(ctx),
	           cnp, vap, dvp, vpp, create, share_access, share_deny);
}

  
                                     
   
int
nfs4_open_reopen_rpc(
	struct nfs_open_file *nofp,
	thread_t thd,
	kauth_cred_t cred,
	struct componentname *cnp,
	vnode_t dvp,
	vnode_t *vpp,
	int share_access,
	int share_deny)
{
	return nfs4_open_rpc_internal(nofp, NULL, thd, cred, cnp, NULL, dvp, vpp, NFS_OPEN_NOCREATE, share_access, share_deny);
}

  
                                               
   
int
nfs4_open_confirm_rpc(
	struct nfsmount *nmp,
	nfsnode_t dnp,
	u_char *fhp,
	int fhlen,
	struct nfs_open_owner *noop,
	nfs_stateid *sid,
	thread_t thd,
	kauth_cred_t cred,
	struct nfs_vattr *nvap,
	uint64_t *xidp)
{
	struct nfsm_chain nmreq, nmrep;
	int error = 0, status, numops;
	struct nfsreq_secinfo_args si;

	NFSREQ_SECINFO_SET(&si, dnp, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                               
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 23 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "open_confirm", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nmp->nm_vers, fhp, fhlen);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_OPEN_CONFIRM);
	nfsm_chain_add_stateid(error, &nmreq, sid);
	nfsm_chain_add_32(error, &nmreq, noop->noo_seqid);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, dnp);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(dnp, NULL, &nmreq, NFSPROC4_COMPOUND, thd, cred, &si, R_NOINTR, &nmrep, xidp, &status);

	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_OPEN_CONFIRM);
	nfs_owner_seqid_increment(noop, NULL, error);
	nfsm_chain_get_stateid(error, &nmrep, sid);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsmout_if(error);
	error = nfs4_parsefattr(&nmrep, NULL, nvap, NULL, NULL, NULL);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

  
                       
  
                                           
                                                  
   
int
nfs4_open_rpc_internal(
	struct nfs_open_file *nofp,
	vfs_context_t ctx,
	thread_t thd,
	kauth_cred_t cred,
	struct componentname *cnp,
	struct vnode_attr *vap,
	vnode_t dvp,
	vnode_t *vpp,
	int create,
	int share_access,
	int share_deny)
{
	struct nfsmount *nmp;
	struct nfs_open_owner *noop = nofp->nof_owner;
	struct nfs_vattr *nvattr;
	int error = 0, open_error = EIO, lockerror = ENOENT, busyerror = ENOENT, status;
	int nfsvers, namedattrs, numops, exclusive = 0, gotuid, gotgid;
	u_int64_t xid, savedxid = 0;
	nfsnode_t dnp = VTONFS(dvp);
	nfsnode_t np, newnp = NULL;
	vnode_t newvp = NULL;
	struct nfsm_chain nmreq, nmrep;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN], bmlen;
	uint32_t rflags, delegation, recall;
	struct nfs_stateid stateid, dstateid, *sid;
	fhandle_t *fh;
	struct nfsreq *req;
	struct nfs_dulookup *dul;
	char sbuf[64], *s;
	uint32_t ace_type, ace_flags, ace_mask, len, slen;
	struct kauth_ace ace;
	struct nfsreq_secinfo_args si;

	if (create && !ctx) {
		return EINVAL;
	}

	nmp = VTONMP(dvp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	namedattrs = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR);
	bzero(&dstateid, sizeof(dstateid));
	if (dnp->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	np = *vpp ? VTONFS(*vpp) : NULL;
	if (create && vap) {
		exclusive = (vap->va_vaflags & VA_EXCLUSIVE);
		nfs_avoid_needless_id_setting_on_create(dnp, vap, ctx);
		gotuid = VATTR_IS_ACTIVE(vap, va_uid);
		gotgid = VATTR_IS_ACTIVE(vap, va_gid);
		if (exclusive && (!VATTR_IS_ACTIVE(vap, va_access_time) || !VATTR_IS_ACTIVE(vap, va_modify_time))) {
			vap->va_vaflags |= VA_UTIMES_NULL;
		}
	} else {
		exclusive = gotuid = gotgid = 0;
	}
	if (nofp) {
		sid = &nofp->nof_stateid;
	} else {
		stateid.seqid = stateid.other[0] = stateid.other[1] = stateid.other[2] = 0;
		sid = &stateid;
	}

	if ((error = nfs_open_owner_set_busy(noop, thd))) {
		return error;
	}

	fh = zalloc(nfs_fhandle_zone);
	req = zalloc(nfs_req_zone);
	MALLOC(dul, struct nfs_dulookup *, sizeof(*dul), M_TEMP, M_WAITOK);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);

again:
	rflags = delegation = recall = 0;
	ace.ace_flags = 0;
	s = sbuf;
	slen = sizeof(sbuf);
	NVATTR_INIT(nvattr);
	NFSREQ_SECINFO_SET(&si, dnp, NULL, 0, cnp->cn_nameptr, cnp->cn_namelen);

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                                                                
	numops = 6;
	nfsm_chain_build_alloc_init(error, &nmreq, 53 * NFSX_UNSIGNED + cnp->cn_namelen);
	nfsm_chain_add_compound_header(error, &nmreq, create ? "create" : "open", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_SAVEFH);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_OPEN);
	nfsm_chain_add_32(error, &nmreq, noop->noo_seqid);
	nfsm_chain_add_32(error, &nmreq, share_access);
	nfsm_chain_add_32(error, &nmreq, share_deny);
	nfsm_chain_add_64(error, &nmreq, nmp->nm_clientid);
	nfsm_chain_add_32(error, &nmreq, NFSX_UNSIGNED);
	nfsm_chain_add_32(error, &nmreq, kauth_cred_getuid(noop->noo_cred));
	nfsm_chain_add_32(error, &nmreq, create);
	if (create) {
		if (exclusive) {
			static uint32_t create_verf;                              
			create_verf++;
			nfsm_chain_add_32(error, &nmreq, NFS_CREATE_EXCLUSIVE);
			                            
			nfsm_chain_add_32(error, &nmreq, create_verf);
			nfsm_chain_add_32(error, &nmreq, create_verf);
		} else {
			nfsm_chain_add_32(error, &nmreq, NFS_CREATE_UNCHECKED);
			nfsm_chain_add_fattr4(error, &nmreq, vap, nmp);
		}
	}
	nfsm_chain_add_32(error, &nmreq, NFS_CLAIM_NULL);
	nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, bitmap);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_FILEHANDLE);
	nfsm_chain_add_bitmap_supported(error, &nmreq, bitmap, nmp, np);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_RESTOREFH);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, dnp);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	if (!error) {
		error = busyerror = nfs_node_set_busy(dnp, thd);
	}
	nfsmout_if(error);

	if (create && !namedattrs) {
		nfs_dulookup_init(dul, dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx);
	}

	error = nfs_request_async(dnp, NULL, &nmreq, NFSPROC4_COMPOUND, thd, cred, &si, R_NOINTR, NULL, &req);
	if (!error) {
		if (create && !namedattrs) {
			nfs_dulookup_start(dul, dnp, ctx);
		}
		error = nfs_request_async_finish(req, &nmrep, &xid, &status);
		savedxid = xid;
	}

	if (create && !namedattrs) {
		nfs_dulookup_finish(dul, dnp, ctx);
	}

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_SAVEFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_OPEN);
	nfs_owner_seqid_increment(noop, NULL, error);
	nfsm_chain_get_stateid(error, &nmrep, sid);
	nfsm_chain_check_change_info(error, &nmrep, dnp);
	nfsm_chain_get_32(error, &nmrep, rflags);
	bmlen = NFS_ATTR_BITMAP_LEN;
	nfsm_chain_get_bitmap(error, &nmrep, bitmap, bmlen);
	nfsm_chain_get_32(error, &nmrep, delegation);
	if (!error) {
		switch (delegation) {
		case NFS_OPEN_DELEGATE_NONE:
			break;
		case NFS_OPEN_DELEGATE_READ:
		case NFS_OPEN_DELEGATE_WRITE:
			nfsm_chain_get_stateid(error, &nmrep, &dstateid);
			nfsm_chain_get_32(error, &nmrep, recall);
			if (delegation == NFS_OPEN_DELEGATE_WRITE) {                    
				nfsm_chain_adv(error, &nmrep, 3 * NFSX_UNSIGNED);
			}
			                                                                  
			ace_type = ace_flags = ace_mask = len = 0;
			nfsm_chain_get_32(error, &nmrep, ace_type);
			nfsm_chain_get_32(error, &nmrep, ace_flags);
			nfsm_chain_get_32(error, &nmrep, ace_mask);
			nfsm_chain_get_32(error, &nmrep, len);
			ace.ace_flags = nfs4_ace_nfstype_to_vfstype(ace_type, &error);
			ace.ace_flags |= nfs4_ace_nfsflags_to_vfsflags(ace_flags);
			ace.ace_rights = nfs4_ace_nfsmask_to_vfsrights(ace_mask);
			if (!error && (len >= slen)) {
				MALLOC(s, char*, len + 1, M_TEMP, M_WAITOK);
				if (s) {
					slen = len + 1;
				} else {
					ace.ace_flags = 0;
				}
			}
			if (s) {
				nfsm_chain_get_opaque(error, &nmrep, len, s);
			} else {
				nfsm_chain_adv(error, &nmrep, nfsm_rndup(len));
			}
			if (!error && s) {
				s[len] = '\0';
				if (nfs4_id2guid(s, &ace.ace_applicable, (ace_flags & NFS_ACE_IDENTIFIER_GROUP))) {
					ace.ace_flags = 0;
				}
			}
			if (error || !s) {
				ace.ace_flags = 0;
			}
			if (s && (s != sbuf)) {
				FREE(s, M_TEMP);
			}
			break;
		default:
			error = EBADRPC;
			break;
		}
	}
	                                                                       
	open_error = error;
	nfsmout_if(error);
	if (create && vap && !exclusive) {
		nfs_vattr_set_supported(bitmap, vap);
	}
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsmout_if(error);
	error = nfs4_parsefattr(&nmrep, NULL, nvattr, fh, NULL, NULL);
	nfsmout_if(error);
	if (!NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_FILEHANDLE)) {
		printf("nfs: open/create didn't return filehandle? %s\n", cnp->cn_nameptr);
		error = EBADRPC;
		goto nfsmout;
	}
	if (!create && np && !NFS_CMPFH(np, fh->fh_data, fh->fh_len)) {
		                                                                                    
		                                                                                            
		if (!(np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR)) {
			NP(np, "nfs4_open_rpc: warning: file handle mismatch");
		}
	}
	                                                                         
	nfsm_chain_op_check(error, &nmrep, NFS_OP_RESTOREFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, dnp, nfsvers, &xid);
	if (error) {
		NATTRINVALIDATE(dnp);
	}
	nfsmout_if(error);

	if (rflags & NFS_OPEN_RESULT_LOCKTYPE_POSIX) {
		nofp->nof_flags |= NFS_OPEN_FILE_POSIXLOCK;
	}

	if (rflags & NFS_OPEN_RESULT_CONFIRM) {
		nfs_node_unlock(dnp);
		lockerror = ENOENT;
		NVATTR_CLEANUP(nvattr);
		error = nfs4_open_confirm_rpc(nmp, dnp, fh->fh_data, fh->fh_len, noop, sid, thd, cred, nvattr, &xid);
		nfsmout_if(error);
		savedxid = xid;
		if ((lockerror = nfs_node_lock(dnp))) {
			error = lockerror;
		}
	}

nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	if (!lockerror && create) {
		if (!open_error && (dnp->n_flag & NNEGNCENTRIES)) {
			dnp->n_flag &= ~NNEGNCENTRIES;
			cache_purge_negatives(dvp);
		}
		dnp->n_flag |= NMODIFIED;
		nfs_node_unlock(dnp);
		lockerror = ENOENT;
		nfs_getattr(dnp, NULL, ctx, NGA_CACHED);
	}
	if (!lockerror) {
		nfs_node_unlock(dnp);
	}
	if (!error && !np && fh->fh_len) {
		                                                         
		xid = savedxid;
		error = nfs_nget(NFSTOMP(dnp), dnp, cnp, fh->fh_data, fh->fh_len, nvattr, &xid, req->r_auth, NG_MAKEENTRY, &newnp);
		if (!error) {
			newvp = NFSTOV(newnp);
		}
	}
	NVATTR_CLEANUP(nvattr);
	if (!busyerror) {
		nfs_node_clear_busy(dnp);
	}
	if ((delegation == NFS_OPEN_DELEGATE_READ) || (delegation == NFS_OPEN_DELEGATE_WRITE)) {
		if (!np) {
			np = newnp;
		}
		if (!error && np && !recall) {
			                                            
			lck_mtx_lock(&np->n_openlock);
			np->n_openflags &= ~N_DELEG_MASK;
			np->n_openflags |= ((delegation == NFS_OPEN_DELEGATE_READ) ? N_DELEG_READ : N_DELEG_WRITE);
			np->n_dstateid = dstateid;
			np->n_dace = ace;
			if (np->n_dlink.tqe_next == NFSNOLIST) {
				lck_mtx_lock(&nmp->nm_lock);
				if (np->n_dlink.tqe_next == NFSNOLIST) {
					TAILQ_INSERT_TAIL(&nmp->nm_delegations, np, n_dlink);
				}
				lck_mtx_unlock(&nmp->nm_lock);
			}
			lck_mtx_unlock(&np->n_openlock);
		} else {
			                              
			if (np) {
				if (NFS_CMPFH(np, fh->fh_data, fh->fh_len)) {
					                                           
					lck_mtx_lock(&np->n_openlock);
					np->n_openflags &= ~N_DELEG_MASK;
					np->n_openflags |= ((delegation == NFS_OPEN_DELEGATE_READ) ? N_DELEG_READ : N_DELEG_WRITE);
					np->n_dstateid = dstateid;
					np->n_dace = ace;
					if (np->n_dlink.tqe_next == NFSNOLIST) {
						lck_mtx_lock(&nmp->nm_lock);
						if (np->n_dlink.tqe_next == NFSNOLIST) {
							TAILQ_INSERT_TAIL(&nmp->nm_delegations, np, n_dlink);
						}
						lck_mtx_unlock(&nmp->nm_lock);
					}
					lck_mtx_unlock(&np->n_openlock);
					                                                      
					fh->fh_len = 0;
				}
				                                    
				nfs4_delegation_return(np, 0, thd, cred);
			}
			if (fh->fh_len) {                                                 
				nfs4_delegreturn_rpc(nmp, fh->fh_data, fh->fh_len, &dstateid, 0, thd, cred);
			}
		}
	}
	if (error) {
		if (exclusive && (error == NFSERR_NOTSUPP)) {
			exclusive = 0;
			goto again;
		}
		if (newvp) {
			nfs_node_unlock(newnp);
			vnode_put(newvp);
		}
	} else if (create) {
		nfs_node_unlock(newnp);
		if (exclusive) {
			error = nfs4_setattr_rpc(newnp, vap, ctx);
			if (error && (gotuid || gotgid)) {
				                                                                  
				                                          
				VATTR_CLEAR_ACTIVE(vap, va_uid);
				VATTR_CLEAR_ACTIVE(vap, va_gid);
				error = nfs4_setattr_rpc(newnp, vap, ctx);
			}
		}
		if (error) {
			vnode_put(newvp);
		} else {
			*vpp = newvp;
		}
	}
	nfs_open_owner_clear_busy(noop);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	NFS_ZFREE(nfs_req_zone, req);
	FREE(dul, M_TEMP);
	FREE(nvattr, M_TEMP);
	return error;
}


  
                                                        
   
int
nfs4_claim_delegated_open_rpc(
	struct nfs_open_file *nofp,
	int share_access,
	int share_deny,
	int flags)
{
	struct nfsmount *nmp;
	struct nfs_open_owner *noop = nofp->nof_owner;
	struct nfs_vattr *nvattr;
	int error = 0, lockerror = ENOENT, status;
	int nfsvers, numops;
	u_int64_t xid;
	nfsnode_t np = nofp->nof_np;
	struct nfsm_chain nmreq, nmrep;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN], bmlen;
	uint32_t rflags = 0, delegation, recall = 0;
	fhandle_t *fh;
	struct nfs_stateid dstateid;
	char sbuf[64], *s = sbuf;
	uint32_t ace_type, ace_flags, ace_mask, len, slen = sizeof(sbuf);
	struct kauth_ace ace;
	vnode_t dvp = NULL;
	const char *vname = NULL;
	const char *name = NULL;
	uint32_t namelen;
	char smallname[128];
	char *filename = NULL;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	fh = zalloc(nfs_fhandle_zone);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	nfsvers = nmp->nm_vers;

	nfs_node_lock_force(np);
	if ((vnode_vtype(NFSTOV(np)) != VDIR) && np->n_sillyrename) {
		  
                                                    
                                                   
     
		struct nfs_sillyrename *nsp = np->n_sillyrename;
		dvp = NFSTOV(nsp->nsr_dnp);
		if ((error = vnode_get(dvp))) {
			dvp = NULLVP;
			nfs_node_unlock(np);
			goto out;
		}
		name = nsp->nsr_name;
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
			goto out;
		}
		name = vname;
	}
	filename = &smallname[0];
	namelen = snprintf(filename, sizeof(smallname), "%s", name);
	if (namelen >= sizeof(smallname)) {
		MALLOC(filename, char *, namelen + 1, M_TEMP, M_WAITOK);
		if (!filename) {
			error = ENOMEM;
			nfs_node_unlock(np);
			goto out;
		}
		snprintf(filename, namelen + 1, "%s", name);
	}
	nfs_node_unlock(np);

	if ((error = nfs_open_owner_set_busy(noop, NULL))) {
		goto out;
	}
	NVATTR_INIT(nvattr);
	delegation = NFS_OPEN_DELEGATE_NONE;
	dstateid = np->n_dstateid;
	NFSREQ_SECINFO_SET(&si, VTONFS(dvp), NULL, 0, filename, namelen);

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                           
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 48 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "open_claim_d", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, VTONFS(dvp)->n_fhp, VTONFS(dvp)->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_OPEN);
	nfsm_chain_add_32(error, &nmreq, noop->noo_seqid);
	nfsm_chain_add_32(error, &nmreq, share_access);
	nfsm_chain_add_32(error, &nmreq, share_deny);
	                             
	nfsm_chain_add_64(error, &nmreq, nmp->nm_clientid);                        
	nfsm_chain_add_32(error, &nmreq, NFSX_UNSIGNED);
	nfsm_chain_add_32(error, &nmreq, kauth_cred_getuid(noop->noo_cred));                     
	            
	nfsm_chain_add_32(error, &nmreq, NFS_OPEN_NOCREATE);
	              
	nfsm_chain_add_32(error, &nmreq, NFS_CLAIM_DELEGATE_CUR);
	nfsm_chain_add_stateid(error, &nmreq, &np->n_dstateid);
	nfsm_chain_add_name(error, &nmreq, filename, namelen, nmp);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, bitmap);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_FILEHANDLE);
	nfsm_chain_add_bitmap_supported(error, &nmreq, bitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);

	error = nfs_request2(np, nmp->nm_mountp, &nmreq, NFSPROC4_COMPOUND, current_thread(),
	    noop->noo_cred, &si, flags | R_NOINTR, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_OPEN);
	nfs_owner_seqid_increment(noop, NULL, error);
	nfsm_chain_get_stateid(error, &nmrep, &nofp->nof_stateid);
	nfsm_chain_check_change_info(error, &nmrep, np);
	nfsm_chain_get_32(error, &nmrep, rflags);
	bmlen = NFS_ATTR_BITMAP_LEN;
	nfsm_chain_get_bitmap(error, &nmrep, bitmap, bmlen);
	nfsm_chain_get_32(error, &nmrep, delegation);
	if (!error) {
		switch (delegation) {
		case NFS_OPEN_DELEGATE_NONE:
			                                                                                            
			                                                                                                         
			break;
		case NFS_OPEN_DELEGATE_READ:
		case NFS_OPEN_DELEGATE_WRITE:
			if ((((np->n_openflags & N_DELEG_MASK) == N_DELEG_READ) &&
			    (delegation == NFS_OPEN_DELEGATE_WRITE)) ||
			    (((np->n_openflags & N_DELEG_MASK) == N_DELEG_WRITE) &&
			    (delegation == NFS_OPEN_DELEGATE_READ))) {
				printf("nfs: open delegated claim returned a different delegation type! have %s got %s %s\n",
				    ((np->n_openflags & N_DELEG_MASK) == N_DELEG_WRITE) ? "W" : "R",
				    (delegation == NFS_OPEN_DELEGATE_WRITE) ? "W" : "R", filename ? filename : "???");
			}
			nfsm_chain_get_stateid(error, &nmrep, &dstateid);
			nfsm_chain_get_32(error, &nmrep, recall);
			if (delegation == NFS_OPEN_DELEGATE_WRITE) {                    
				nfsm_chain_adv(error, &nmrep, 3 * NFSX_UNSIGNED);
			}
			                                                                  
			ace_type = ace_flags = ace_mask = len = 0;
			nfsm_chain_get_32(error, &nmrep, ace_type);
			nfsm_chain_get_32(error, &nmrep, ace_flags);
			nfsm_chain_get_32(error, &nmrep, ace_mask);
			nfsm_chain_get_32(error, &nmrep, len);
			ace.ace_flags = nfs4_ace_nfstype_to_vfstype(ace_type, &error);
			ace.ace_flags |= nfs4_ace_nfsflags_to_vfsflags(ace_flags);
			ace.ace_rights = nfs4_ace_nfsmask_to_vfsrights(ace_mask);
			if (!error && (len >= slen)) {
				MALLOC(s, char*, len + 1, M_TEMP, M_WAITOK);
				if (s) {
					slen = len + 1;
				} else {
					ace.ace_flags = 0;
				}
			}
			if (s) {
				nfsm_chain_get_opaque(error, &nmrep, len, s);
			} else {
				nfsm_chain_adv(error, &nmrep, nfsm_rndup(len));
			}
			if (!error && s) {
				s[len] = '\0';
				if (nfs4_id2guid(s, &ace.ace_applicable, (ace_flags & NFS_ACE_IDENTIFIER_GROUP))) {
					ace.ace_flags = 0;
				}
			}
			if (error || !s) {
				ace.ace_flags = 0;
			}
			if (s && (s != sbuf)) {
				FREE(s, M_TEMP);
			}
			if (!error) {
				                                                   
				lck_mtx_lock(&np->n_openlock);
				np->n_openflags &= ~N_DELEG_MASK;
				np->n_openflags |= ((delegation == NFS_OPEN_DELEGATE_READ) ? N_DELEG_READ : N_DELEG_WRITE);
				np->n_dstateid = dstateid;
				np->n_dace = ace;
				if (np->n_dlink.tqe_next == NFSNOLIST) {
					lck_mtx_lock(&nmp->nm_lock);
					if (np->n_dlink.tqe_next == NFSNOLIST) {
						TAILQ_INSERT_TAIL(&nmp->nm_delegations, np, n_dlink);
					}
					lck_mtx_unlock(&nmp->nm_lock);
				}
				lck_mtx_unlock(&np->n_openlock);
			}
			break;
		default:
			error = EBADRPC;
			break;
		}
	}
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	error = nfs4_parsefattr(&nmrep, NULL, nvattr, fh, NULL, NULL);
	nfsmout_if(error);
	if (!NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_FILEHANDLE)) {
		printf("nfs: open reclaim didn't return filehandle? %s\n", filename ? filename : "???");
		error = EBADRPC;
		goto nfsmout;
	}
	if (!NFS_CMPFH(np, fh->fh_data, fh->fh_len)) {
		                                                                    
		                                                                                            
		if (!(np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR)) {
			printf("nfs4_claim_delegated_open_rpc: warning: file handle mismatch %s\n", filename ? filename : "???");
		}
	}
	error = nfs_loadattrcache(np, nvattr, &xid, 1);
	nfsmout_if(error);
	if (rflags & NFS_OPEN_RESULT_LOCKTYPE_POSIX) {
		nofp->nof_flags |= NFS_OPEN_FILE_POSIXLOCK;
	}
nfsmout:
	NVATTR_CLEANUP(nvattr);
	FREE(nvattr, M_TEMP);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfs_open_owner_clear_busy(noop);
	if ((delegation == NFS_OPEN_DELEGATE_READ) || (delegation == NFS_OPEN_DELEGATE_WRITE)) {
		if (recall) {
			  
                                     
                                                                     
                                                   
      
			nfs4_delegation_return_enqueue(np);
		}
	}
out:
	              
	                                                                                                                               
	if (filename && (filename != &smallname[0])) {
		FREE(filename, M_TEMP);
	}
	if (vname) {
		vnode_putname(vname);
	}
	if (dvp != NULLVP) {
		vnode_put(dvp);
	}
	return error;
}

  
                                            
   
int
nfs4_open_reclaim_rpc(
	struct nfs_open_file *nofp,
	int share_access,
	int share_deny)
{
	struct nfsmount *nmp;
	struct nfs_open_owner *noop = nofp->nof_owner;
	struct nfs_vattr *nvattr;
	int error = 0, lockerror = ENOENT, status;
	int nfsvers, numops;
	u_int64_t xid;
	nfsnode_t np = nofp->nof_np;
	struct nfsm_chain nmreq, nmrep;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN], bmlen;
	uint32_t rflags = 0, delegation, recall = 0;
	fhandle_t *fh;
	struct nfs_stateid dstateid;
	char sbuf[64], *s = sbuf;
	uint32_t ace_type, ace_flags, ace_mask, len, slen = sizeof(sbuf);
	struct kauth_ace ace;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	if ((error = nfs_open_owner_set_busy(noop, NULL))) {
		return error;
	}

	fh = zalloc(nfs_fhandle_zone);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	NVATTR_INIT(nvattr);
	delegation = NFS_OPEN_DELEGATE_NONE;
	dstateid = np->n_dstateid;
	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);

	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                           
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 48 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "open_reclaim", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_OPEN);
	nfsm_chain_add_32(error, &nmreq, noop->noo_seqid);
	nfsm_chain_add_32(error, &nmreq, share_access);
	nfsm_chain_add_32(error, &nmreq, share_deny);
	                             
	nfsm_chain_add_64(error, &nmreq, nmp->nm_clientid);                        
	nfsm_chain_add_32(error, &nmreq, NFSX_UNSIGNED);
	nfsm_chain_add_32(error, &nmreq, kauth_cred_getuid(noop->noo_cred));                     
	            
	nfsm_chain_add_32(error, &nmreq, NFS_OPEN_NOCREATE);
	              
	nfsm_chain_add_32(error, &nmreq, NFS_CLAIM_PREVIOUS);
	delegation = (np->n_openflags & N_DELEG_READ) ? NFS_OPEN_DELEGATE_READ :
	    (np->n_openflags & N_DELEG_WRITE) ? NFS_OPEN_DELEGATE_WRITE :
	    NFS_OPEN_DELEGATE_NONE;
	nfsm_chain_add_32(error, &nmreq, delegation);
	delegation = NFS_OPEN_DELEGATE_NONE;
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, bitmap);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_FILEHANDLE);
	nfsm_chain_add_bitmap_supported(error, &nmreq, bitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);

	error = nfs_request2(np, nmp->nm_mountp, &nmreq, NFSPROC4_COMPOUND, current_thread(),
	    noop->noo_cred, &si, R_RECOVER | R_NOINTR, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_OPEN);
	nfs_owner_seqid_increment(noop, NULL, error);
	nfsm_chain_get_stateid(error, &nmrep, &nofp->nof_stateid);
	nfsm_chain_check_change_info(error, &nmrep, np);
	nfsm_chain_get_32(error, &nmrep, rflags);
	bmlen = NFS_ATTR_BITMAP_LEN;
	nfsm_chain_get_bitmap(error, &nmrep, bitmap, bmlen);
	nfsm_chain_get_32(error, &nmrep, delegation);
	if (!error) {
		switch (delegation) {
		case NFS_OPEN_DELEGATE_NONE:
			if (np->n_openflags & N_DELEG_MASK) {
				  
                                                             
                                                           
      
                                                  
       
				                                                         
				delegation = (np->n_openflags & N_DELEG_WRITE) ? NFS_OPEN_DELEGATE_WRITE : NFS_OPEN_DELEGATE_READ;
				recall = 1;
			}
			break;
		case NFS_OPEN_DELEGATE_READ:
		case NFS_OPEN_DELEGATE_WRITE:
			nfsm_chain_get_stateid(error, &nmrep, &dstateid);
			nfsm_chain_get_32(error, &nmrep, recall);
			if (delegation == NFS_OPEN_DELEGATE_WRITE) {                    
				nfsm_chain_adv(error, &nmrep, 3 * NFSX_UNSIGNED);
			}
			                                                                  
			ace_type = ace_flags = ace_mask = len = 0;
			nfsm_chain_get_32(error, &nmrep, ace_type);
			nfsm_chain_get_32(error, &nmrep, ace_flags);
			nfsm_chain_get_32(error, &nmrep, ace_mask);
			nfsm_chain_get_32(error, &nmrep, len);
			ace.ace_flags = nfs4_ace_nfstype_to_vfstype(ace_type, &error);
			ace.ace_flags |= nfs4_ace_nfsflags_to_vfsflags(ace_flags);
			ace.ace_rights = nfs4_ace_nfsmask_to_vfsrights(ace_mask);
			if (!error && (len >= slen)) {
				MALLOC(s, char*, len + 1, M_TEMP, M_WAITOK);
				if (s) {
					slen = len + 1;
				} else {
					ace.ace_flags = 0;
				}
			}
			if (s) {
				nfsm_chain_get_opaque(error, &nmrep, len, s);
			} else {
				nfsm_chain_adv(error, &nmrep, nfsm_rndup(len));
			}
			if (!error && s) {
				s[len] = '\0';
				if (nfs4_id2guid(s, &ace.ace_applicable, (ace_flags & NFS_ACE_IDENTIFIER_GROUP))) {
					ace.ace_flags = 0;
				}
			}
			if (error || !s) {
				ace.ace_flags = 0;
			}
			if (s && (s != sbuf)) {
				FREE(s, M_TEMP);
			}
			if (!error) {
				                                            
				lck_mtx_lock(&np->n_openlock);
				np->n_openflags &= ~N_DELEG_MASK;
				np->n_openflags |= ((delegation == NFS_OPEN_DELEGATE_READ) ? N_DELEG_READ : N_DELEG_WRITE);
				np->n_dstateid = dstateid;
				np->n_dace = ace;
				if (np->n_dlink.tqe_next == NFSNOLIST) {
					lck_mtx_lock(&nmp->nm_lock);
					if (np->n_dlink.tqe_next == NFSNOLIST) {
						TAILQ_INSERT_TAIL(&nmp->nm_delegations, np, n_dlink);
					}
					lck_mtx_unlock(&nmp->nm_lock);
				}
				lck_mtx_unlock(&np->n_openlock);
			}
			break;
		default:
			error = EBADRPC;
			break;
		}
	}
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	error = nfs4_parsefattr(&nmrep, NULL, nvattr, fh, NULL, NULL);
	nfsmout_if(error);
	if (!NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_FILEHANDLE)) {
		NP(np, "nfs: open reclaim didn't return filehandle?");
		error = EBADRPC;
		goto nfsmout;
	}
	if (!NFS_CMPFH(np, fh->fh_data, fh->fh_len)) {
		                                                                    
		                                                                   
		                                                                    
		                                                                                            
		if (!(np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR)) {
			NP(np, "nfs4_open_reclaim_rpc: warning: file handle mismatch");
		}
	}
	error = nfs_loadattrcache(np, nvattr, &xid, 1);
	nfsmout_if(error);
	if (rflags & NFS_OPEN_RESULT_LOCKTYPE_POSIX) {
		nofp->nof_flags |= NFS_OPEN_FILE_POSIXLOCK;
	}
nfsmout:
	              
	                                                                                 
	NVATTR_CLEANUP(nvattr);
	FREE(nvattr, M_TEMP);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfs_open_owner_clear_busy(noop);
	if ((delegation == NFS_OPEN_DELEGATE_READ) || (delegation == NFS_OPEN_DELEGATE_WRITE)) {
		if (recall) {
			nfs4_delegation_return_enqueue(np);
		}
	}
	return error;
}

int
nfs4_open_downgrade_rpc(
	nfsnode_t np,
	struct nfs_open_file *nofp,
	vfs_context_t ctx)
{
	struct nfs_open_owner *noop = nofp->nof_owner;
	struct nfsmount *nmp;
	int error, lockerror = ENOENT, status, nfsvers, numops;
	struct nfsm_chain nmreq, nmrep;
	u_int64_t xid;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	if ((error = nfs_open_owner_set_busy(noop, NULL))) {
		return error;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                                 
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 23 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "open_downgrd", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_OPEN_DOWNGRADE);
	nfsm_chain_add_stateid(error, &nmreq, &nofp->nof_stateid);
	nfsm_chain_add_32(error, &nmreq, noop->noo_seqid);
	nfsm_chain_add_32(error, &nmreq, nofp->nof_access);
	nfsm_chain_add_32(error, &nmreq, nofp->nof_deny);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(np, NULL, &nmreq, NFSPROC4_COMPOUND,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx),
	    &si, R_NOINTR, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_OPEN_DOWNGRADE);
	nfs_owner_seqid_increment(noop, NULL, error);
	nfsm_chain_get_stateid(error, &nmrep, &nofp->nof_stateid);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, np, nfsvers, &xid);
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfs_open_owner_clear_busy(noop);
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}

int
nfs4_close_rpc(
	nfsnode_t np,
	struct nfs_open_file *nofp,
	thread_t thd,
	kauth_cred_t cred,
	int flags)
{
	struct nfs_open_owner *noop = nofp->nof_owner;
	struct nfsmount *nmp;
	int error, lockerror = ENOENT, status, nfsvers, numops;
	struct nfsm_chain nmreq, nmrep;
	u_int64_t xid;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;

	if ((error = nfs_open_owner_set_busy(noop, NULL))) {
		return error;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                        
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 23 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "close", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_CLOSE);
	nfsm_chain_add_32(error, &nmreq, noop->noo_seqid);
	nfsm_chain_add_stateid(error, &nmreq, &nofp->nof_stateid);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(np, NULL, &nmreq, NFSPROC4_COMPOUND, thd, cred, &si, flags | R_NOINTR, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock(np))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_CLOSE);
	nfs_owner_seqid_increment(noop, NULL, error);
	nfsm_chain_get_stateid(error, &nmrep, &nofp->nof_stateid);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsm_chain_loadattr(error, &nmrep, np, nfsvers, &xid);
nfsmout:
	if (!lockerror) {
		nfs_node_unlock(np);
	}
	nfs_open_owner_clear_busy(noop);
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}


  
                                                              
   
int
nfs4_claim_delegated_state_for_open_file(struct nfs_open_file *nofp, int flags)
{
	struct nfs_open_owner *noop = nofp->nof_owner;
	struct nfs_lock_owner *nlop;
	struct nfs_file_lock *nflp, *nextnflp;
	struct nfsmount *nmp;
	int error = 0, reopen = 0;

	if (nofp->nof_d_rw_drw) {
		error = nfs4_claim_delegated_open_rpc(nofp, NFS_OPEN_SHARE_ACCESS_BOTH, NFS_OPEN_SHARE_DENY_BOTH, flags);
		if (!error) {
			lck_mtx_lock(&nofp->nof_lock);
			nofp->nof_rw_drw += nofp->nof_d_rw_drw;
			nofp->nof_d_rw_drw = 0;
			lck_mtx_unlock(&nofp->nof_lock);
		}
	}
	if (!error && nofp->nof_d_w_drw) {
		error = nfs4_claim_delegated_open_rpc(nofp, NFS_OPEN_SHARE_ACCESS_WRITE, NFS_OPEN_SHARE_DENY_BOTH, flags);
		if (!error) {
			lck_mtx_lock(&nofp->nof_lock);
			nofp->nof_w_drw += nofp->nof_d_w_drw;
			nofp->nof_d_w_drw = 0;
			lck_mtx_unlock(&nofp->nof_lock);
		}
	}
	if (!error && nofp->nof_d_r_drw) {
		error = nfs4_claim_delegated_open_rpc(nofp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_BOTH, flags);
		if (!error) {
			lck_mtx_lock(&nofp->nof_lock);
			nofp->nof_r_drw += nofp->nof_d_r_drw;
			nofp->nof_d_r_drw = 0;
			lck_mtx_unlock(&nofp->nof_lock);
		}
	}
	if (!error && nofp->nof_d_rw_dw) {
		error = nfs4_claim_delegated_open_rpc(nofp, NFS_OPEN_SHARE_ACCESS_BOTH, NFS_OPEN_SHARE_DENY_WRITE, flags);
		if (!error) {
			lck_mtx_lock(&nofp->nof_lock);
			nofp->nof_rw_dw += nofp->nof_d_rw_dw;
			nofp->nof_d_rw_dw = 0;
			lck_mtx_unlock(&nofp->nof_lock);
		}
	}
	if (!error && nofp->nof_d_w_dw) {
		error = nfs4_claim_delegated_open_rpc(nofp, NFS_OPEN_SHARE_ACCESS_WRITE, NFS_OPEN_SHARE_DENY_WRITE, flags);
		if (!error) {
			lck_mtx_lock(&nofp->nof_lock);
			nofp->nof_w_dw += nofp->nof_d_w_dw;
			nofp->nof_d_w_dw = 0;
			lck_mtx_unlock(&nofp->nof_lock);
		}
	}
	if (!error && nofp->nof_d_r_dw) {
		error = nfs4_claim_delegated_open_rpc(nofp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_WRITE, flags);
		if (!error) {
			lck_mtx_lock(&nofp->nof_lock);
			nofp->nof_r_dw += nofp->nof_d_r_dw;
			nofp->nof_d_r_dw = 0;
			lck_mtx_unlock(&nofp->nof_lock);
		}
	}
	                                                              
	if (!error && nofp->nof_d_rw) {
		error = nfs4_claim_delegated_open_rpc(nofp, NFS_OPEN_SHARE_ACCESS_BOTH, NFS_OPEN_SHARE_DENY_NONE, flags);
		                                                            
		if (nfs_mount_state_error_delegation_lost(error)) {
			reopen = error;
		}
		if (!error || reopen) {
			lck_mtx_lock(&nofp->nof_lock);
			nofp->nof_rw += nofp->nof_d_rw;
			nofp->nof_d_rw = 0;
			lck_mtx_unlock(&nofp->nof_lock);
		}
	}
	                                                                                                       
	if ((!error || reopen) && nofp->nof_d_w) {
		if (!error) {
			error = nfs4_claim_delegated_open_rpc(nofp, NFS_OPEN_SHARE_ACCESS_WRITE, NFS_OPEN_SHARE_DENY_NONE, flags);
			                                                            
			if (nfs_mount_state_error_delegation_lost(error)) {
				reopen = error;
			}
		}
		if (!error || reopen) {
			lck_mtx_lock(&nofp->nof_lock);
			nofp->nof_w += nofp->nof_d_w;
			nofp->nof_d_w = 0;
			lck_mtx_unlock(&nofp->nof_lock);
		}
	}
	if ((!error || reopen) && nofp->nof_d_r) {
		if (!error) {
			error = nfs4_claim_delegated_open_rpc(nofp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_NONE, flags);
			                                                            
			if (nfs_mount_state_error_delegation_lost(error)) {
				reopen = error;
			}
		}
		if (!error || reopen) {
			lck_mtx_lock(&nofp->nof_lock);
			nofp->nof_r += nofp->nof_d_r;
			nofp->nof_d_r = 0;
			lck_mtx_unlock(&nofp->nof_lock);
		}
	}

	if (reopen) {
		  
                                                                
                                                              
     
		if ((nmp = NFSTONMP(nofp->nof_np))) {
			nfs4_delegation_return_enqueue(nofp->nof_np);
			lck_mtx_lock(&nmp->nm_lock);
			nfs_need_recover(nmp, NFSERR_EXPIRED);
			lck_mtx_unlock(&nmp->nm_lock);
		}
		if (reopen && (nfs_check_for_locks(noop, nofp) == 0)) {
			                                         
			NP(nofp->nof_np, "nfs4_claim_delegated_state_for_open_file: %d, need reopen, %d",
			    reopen, kauth_cred_getuid(nofp->nof_owner->noo_cred));
			lck_mtx_lock(&nofp->nof_lock);
			nofp->nof_flags |= NFS_OPEN_FILE_REOPEN;
			lck_mtx_unlock(&nofp->nof_lock);
			return 0;
		}
		if (reopen) {
			NP(nofp->nof_np, "nfs4_claim_delegated_state_for_open_file: %d, locks prevent reopen, %d",
			    reopen, kauth_cred_getuid(nofp->nof_owner->noo_cred));
		}
	}

	if (!error && ((nmp = NFSTONMP(nofp->nof_np)))) {
		                           
		TAILQ_FOREACH(nlop, &nofp->nof_np->n_lock_owners, nlo_link) {
			if (nlop->nlo_open_owner != noop) {
				continue;
			}
			TAILQ_FOREACH_SAFE(nflp, &nlop->nlo_locks, nfl_lolink, nextnflp) {
				                                                                                
				if (nflp->nfl_flags & (NFS_FILE_LOCK_DEAD | NFS_FILE_LOCK_BLOCKED)) {
					continue;
				}
				                              
				if (!(nflp->nfl_flags & NFS_FILE_LOCK_DELEGATED)) {
					continue;
				}
				error = nmp->nm_funcs->nf_setlock_rpc(nofp->nof_np, nofp, nflp, 0, flags, current_thread(), noop->noo_cred);
				if (error) {
					NP(nofp->nof_np, "nfs: delegated lock claim (0x%llx, 0x%llx) failed %d, %d",
					    nflp->nfl_start, nflp->nfl_end, error, kauth_cred_getuid(nofp->nof_owner->noo_cred));
					break;
				}
				         
				                                                                                    
				                                                                                              
				    
			}
			if (error) {
				break;
			}
		}
	}

	if (!error) {                                      
		return 0;
	}

	                                                                             
	if (!nfs_mount_state_error_delegation_lost(error) &&
	    ((error == ETIMEDOUT) || nfs_mount_state_error_should_restart(error))) {
		NP(nofp->nof_np, "nfs delegated lock claim error %d, %d", error, kauth_cred_getuid(nofp->nof_owner->noo_cred));
		if ((error == ETIMEDOUT) && ((nmp = NFSTONMP(nofp->nof_np)))) {
			nfs_need_reconnect(nmp);
		}
		return error;
	}

	                                                            
	NP(nofp->nof_np, "nfs delegated state claim error %d, state lost, %d", error, kauth_cred_getuid(nofp->nof_owner->noo_cred));

	  
                                                               
                                                             
    
	if ((nmp = NFSTONMP(nofp->nof_np))) {
		nfs4_delegation_return_enqueue(nofp->nof_np);
		lck_mtx_lock(&nmp->nm_lock);
		nfs_need_recover(nmp, NFSERR_EXPIRED);
		lck_mtx_unlock(&nmp->nm_lock);
	}

	                                
	nfs_revoke_open_state_for_node(nofp->nof_np);

	return error;
}
#endif                 

  
                                             
   
void
nfs_release_open_state_for_node(nfsnode_t np, int force)
{
	struct nfsmount *nmp = NFSTONMP(np);
	struct nfs_open_file *nofp;
	struct nfs_file_lock *nflp, *nextnflp;

	                     
	TAILQ_FOREACH_SAFE(nflp, &np->n_locks, nfl_link, nextnflp) {
		                                       
		if (nflp->nfl_flags & (NFS_FILE_LOCK_DEAD | NFS_FILE_LOCK_BLOCKED)) {
			continue;
		}
		                                            
		if (!force && nmp && !(nflp->nfl_flags & NFS_FILE_LOCK_DELEGATED)) {
			nmp->nm_funcs->nf_unlock_rpc(np, nflp->nfl_owner, F_WRLCK, nflp->nfl_start, nflp->nfl_end, R_RECOVER,
			    NULL, nflp->nfl_owner->nlo_open_owner->noo_cred);
		}
		                          
		lck_mtx_lock(&np->n_openlock);
		nflp->nfl_flags |= NFS_FILE_LOCK_DEAD;
		lck_mtx_lock(&nflp->nfl_owner->nlo_lock);
		TAILQ_REMOVE(&nflp->nfl_owner->nlo_locks, nflp, nfl_lolink);
		lck_mtx_unlock(&nflp->nfl_owner->nlo_lock);
		if (nflp->nfl_blockcnt) {
			                                         
			wakeup(nflp);
		} else {
			                                            
			TAILQ_REMOVE(&np->n_locks, nflp, nfl_link);
			nfs_file_lock_destroy(nflp);
		}
		lck_mtx_unlock(&np->n_openlock);
	}

	lck_mtx_lock(&np->n_openlock);

	                    
	TAILQ_FOREACH(nofp, &np->n_opens, nof_link) {
		if (nofp->nof_flags & NFS_OPEN_FILE_LOST) {
			continue;
		}
		                             
		lck_mtx_lock(&nofp->nof_lock);
		nofp->nof_flags &= ~NFS_OPEN_FILE_REOPEN;
		nofp->nof_flags |= NFS_OPEN_FILE_LOST;

		lck_mtx_unlock(&nofp->nof_lock);
#if CONFIG_NFS4
		if (!force && nmp && (nmp->nm_vers >= NFS_VER4)) {
			nfs4_close_rpc(np, nofp, NULL, nofp->nof_owner->noo_cred, R_RECOVER);
		}
#endif
	}

	lck_mtx_unlock(&np->n_openlock);
}

  
                                                                
                                                             
                                  
   
void
nfs_revoke_open_state_for_node(nfsnode_t np)
{
	struct nfsmount *nmp;

	                                        
	nfs_node_lock_force(np);
	if (np->n_flag & NREVOKE) {                       
		NP(np, "nfs_revoke_open_state_for_node(): already revoked");
		nfs_node_unlock(np);
		return;
	}
	np->n_flag |= NREVOKE;
	nfs_node_unlock(np);

	nfs_release_open_state_for_node(np, 0);
	NP(np, "nfs: state lost for %p 0x%x", np, np->n_flag);

	                                                                           
	if ((nmp = NFSTONMP(np))) {
		lck_mtx_lock(&nmp->nm_lock);
		nmp->nm_state |= NFSSTA_REVOKE;
		nfs_mount_sock_thread_wake(nmp);
		lck_mtx_unlock(&nmp->nm_lock);
	}
}

#if CONFIG_NFS4
  
                                                                                  
   
int
nfs4_claim_delegated_state_for_node(nfsnode_t np, int flags)
{
	struct nfs_open_file *nofp;
	int error = 0;

	lck_mtx_lock(&np->n_openlock);

	                                                                             
restart:
	TAILQ_FOREACH(nofp, &np->n_opens, nof_link) {
		if (!nofp->nof_d_rw_drw && !nofp->nof_d_w_drw && !nofp->nof_d_r_drw &&
		    !nofp->nof_d_rw_dw && !nofp->nof_d_w_dw && !nofp->nof_d_r_dw &&
		    !nofp->nof_d_rw && !nofp->nof_d_w && !nofp->nof_d_r) {
			continue;
		}
		lck_mtx_unlock(&np->n_openlock);
		error = nfs4_claim_delegated_state_for_open_file(nofp, flags);
		lck_mtx_lock(&np->n_openlock);
		if (error) {
			break;
		}
		goto restart;
	}

	lck_mtx_unlock(&np->n_openlock);

	return error;
}

  
                                                         
                                              
                                   
   
void
nfs4_delegation_return_enqueue(nfsnode_t np)
{
	struct nfsmount *nmp;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return;
	}

	lck_mtx_lock(&np->n_openlock);
	np->n_openflags |= N_DELEG_RETURN;
	lck_mtx_unlock(&np->n_openlock);

	lck_mtx_lock(&nmp->nm_lock);
	if (np->n_dreturn.tqe_next == NFSNOLIST) {
		TAILQ_INSERT_TAIL(&nmp->nm_dreturnq, np, n_dreturn);
	}
	nfs_mount_sock_thread_wake(nmp);
	lck_mtx_unlock(&nmp->nm_lock);
}

  
                                                       
   
int
nfs4_delegation_return(nfsnode_t np, int flags, thread_t thd, kauth_cred_t cred)
{
	struct nfsmount *nmp;
	fhandle_t *fh;
	nfs_stateid dstateid;
	int error;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	fh = zalloc(nfs_fhandle_zone);

	                                                              
	lck_mtx_lock(&np->n_openlock);
	np->n_openflags |= (N_DELEG_RETURN | N_DELEG_RETURNING);
	lck_mtx_unlock(&np->n_openlock);

	                                                         
	if ((error = nfs_open_state_set_busy(np, NULL))) {
		goto out;
	}

	                               
	if ((error = nfs4_claim_delegated_state_for_node(np, flags))) {
		goto out;
	}

	                           
	lck_mtx_lock(&np->n_openlock);
	dstateid = np->n_dstateid;
	fh->fh_len = np->n_fhsize;
	bcopy(np->n_fhp, fh->fh_data, fh->fh_len);
	lck_mtx_unlock(&np->n_openlock);
	error = nfs4_delegreturn_rpc(NFSTONMP(np), fh->fh_data, fh->fh_len, &dstateid, flags, thd, cred);
	                                                                              
	if ((error != ETIMEDOUT) && (error != NFSERR_MOVED) && (error != NFSERR_LEASE_MOVED)) {
		lck_mtx_lock(&np->n_openlock);
		np->n_openflags &= ~N_DELEG_MASK;
		lck_mtx_lock(&nmp->nm_lock);
		if (np->n_dlink.tqe_next != NFSNOLIST) {
			TAILQ_REMOVE(&nmp->nm_delegations, np, n_dlink);
			np->n_dlink.tqe_next = NFSNOLIST;
		}
		lck_mtx_unlock(&nmp->nm_lock);
		lck_mtx_unlock(&np->n_openlock);
	}

out:
	                                                                             
	lck_mtx_lock(&nmp->nm_lock);
	if (np->n_dreturn.tqe_next != NFSNOLIST) {
		TAILQ_REMOVE(&nmp->nm_dreturnq, np, n_dreturn);
		np->n_dreturn.tqe_next = NFSNOLIST;
	}
	lck_mtx_unlock(&nmp->nm_lock);
	lck_mtx_lock(&np->n_openlock);
	np->n_openflags &= ~(N_DELEG_RETURN | N_DELEG_RETURNING);
	lck_mtx_unlock(&np->n_openlock);

	if (error) {
		NP(np, "nfs4_delegation_return, error %d", error);
		if (error == ETIMEDOUT) {
			nfs_need_reconnect(nmp);
		}
		if (nfs_mount_state_error_should_restart(error)) {
			                                
			lck_mtx_lock(&nmp->nm_lock);
			nfs_need_recover(nmp, nfs_mount_state_error_delegation_lost(error) ? NFSERR_EXPIRED : 0);
			lck_mtx_unlock(&nmp->nm_lock);
		}
	}

	nfs_open_state_clear_busy(np);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	return error;
}

  
                                               
   
int
nfs4_delegreturn_rpc(struct nfsmount *nmp, u_char *fhp, int fhlen, struct nfs_stateid *sid, int flags, thread_t thd, kauth_cred_t cred)
{
	int error = 0, status, numops;
	uint64_t xid;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq_secinfo_args si;

	NFSREQ_SECINFO_SET(&si, NULL, fhp, fhlen, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                     
	numops = 2;
	nfsm_chain_build_alloc_init(error, &nmreq, 16 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "delegreturn", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nmp->nm_vers, fhp, fhlen);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_DELEGRETURN);
	nfsm_chain_add_stateid(error, &nmreq, sid);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request2(NULL, nmp->nm_mountp, &nmreq, NFSPROC4_COMPOUND, thd, cred, &si, flags, &nmrep, &xid, &status);
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_DELEGRETURN);
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	return error;
}
#endif                  

  
                 
                                          
  
                                                                              
                                                                          
   
int
nfs_vnop_read(
	struct vnop_read_args     
                                                              
                                               
                                                    
                                               
                                                          
                                      *ap)
{
	vnode_t vp = ap->a_vp;
	vfs_context_t ctx = ap->a_context;
	nfsnode_t np;
	struct nfsmount *nmp;
	struct nfs_open_owner *noop;
	struct nfs_open_file *nofp;
	int error;

	if (vnode_vtype(ap->a_vp) != VREG) {
		return (vnode_vtype(vp) == VDIR) ? EISDIR : EPERM;
	}

	np = VTONFS(vp);
	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (np->n_flag & NREVOKE) {
		return EIO;
	}

	noop = nfs_open_owner_find(nmp, vfs_context_ucred(ctx), 1);
	if (!noop) {
		return ENOMEM;
	}
restart:
	error = nfs_open_file_find(np, noop, &nofp, 0, 0, 1);
	if (!error && (nofp->nof_flags & NFS_OPEN_FILE_LOST)) {
		NP(np, "nfs_vnop_read: LOST %d", kauth_cred_getuid(noop->noo_cred));
		error = EIO;
	}
#if CONFIG_NFS4
	if (!error && (nofp->nof_flags & NFS_OPEN_FILE_REOPEN)) {
		error = nfs4_reopen(nofp, vfs_context_thread(ctx));
		nofp = NULL;
		if (!error) {
			goto restart;
		}
	}
#endif
	if (error) {
		nfs_open_owner_rele(noop);
		return error;
	}
	  
                                                         
                                                         
                                                             
   
                                                       
                                                        
                                                       
                                                               
                                      
    
	if (nofp->nof_access & NFS_OPEN_SHARE_ACCESS_READ) {
		nfs_open_owner_rele(noop);
		goto do_read;
	}
	error = nfs_mount_state_in_use_start(nmp, vfs_context_thread(ctx));
	if (error) {
		nfs_open_owner_rele(noop);
		return error;
	}
	  
                                                                            
                                                                               
                                                                  
                    
    
	error = nfs_open_file_set_busy(nofp, vfs_context_thread(ctx));
	if (error) {
		nfs_mount_state_in_use_end(nmp, 0);
		nfs_open_owner_rele(noop);
		return error;
	}
	if (!(nofp->nof_access & NFS_OPEN_SHARE_ACCESS_READ)) {
		                                                                                 
		if (nofp->nof_flags & NFS_OPEN_FILE_NEEDCLOSE) {
			NP(np, "nfs_vnop_read: File already needs close access: 0x%x, cred: %d thread: %lld",
			    nofp->nof_access, kauth_cred_getuid(nofp->nof_owner->noo_cred), thread_tid(vfs_context_thread(ctx)));
		}
		if (nofp->nof_deny & NFS_OPEN_SHARE_DENY_READ) {
			nfs_open_file_clear_busy(nofp);
			nfs_mount_state_in_use_end(nmp, 0);
			nfs_open_owner_rele(noop);
			return EPERM;
		}
		if (np->n_flag & NREVOKE) {
			error = EIO;
			nfs_open_file_clear_busy(nofp);
			nfs_mount_state_in_use_end(nmp, 0);
			nfs_open_owner_rele(noop);
			return error;
		}
		if (nmp->nm_vers < NFS_VER4) {
			                                                          
			nfs_open_file_add_open(nofp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_NONE, 0);
		}
#if CONFIG_NFS4
		else {
			error = nfs4_open(np, nofp, NFS_OPEN_SHARE_ACCESS_READ, NFS_OPEN_SHARE_DENY_NONE, ctx);
		}
#endif
		if (!error) {
			nofp->nof_flags |= NFS_OPEN_FILE_NEEDCLOSE;
		}
	}
	if (nofp) {
		nfs_open_file_clear_busy(nofp);
	}
	if (nfs_mount_state_in_use_end(nmp, error)) {
		nofp = NULL;
		goto restart;
	}
	nfs_open_owner_rele(noop);
	if (error) {
		return error;
	}
do_read:
	return nfs_bioread(VTONFS(ap->a_vp), ap->a_uio, ap->a_ioflag, ap->a_context);
}

#if CONFIG_NFS4
  
                                                                     
                                                                   
                                       
   
int
nfs4_vnop_create(
	struct vnop_create_args     
                                                                
                                                  
                                                   
                                                                
                                                             
                                                            
                                        *ap)
{
	vfs_context_t ctx = ap->a_context;
	struct componentname *cnp = ap->a_cnp;
	struct vnode_attr *vap = ap->a_vap;
	vnode_t dvp = ap->a_dvp;
	vnode_t *vpp = ap->a_vpp;
	struct nfsmount *nmp;
	nfsnode_t np;
	int error = 0, busyerror = 0, accessMode, denyMode;
	struct nfs_open_owner *noop = NULL;
	struct nfs_open_file *newnofp = NULL, *nofp = NULL;

	nmp = VTONMP(dvp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (vap) {
		nfs_avoid_needless_id_setting_on_create(VTONFS(dvp), vap, ctx);
	}

	noop = nfs_open_owner_find(nmp, vfs_context_ucred(ctx), 1);
	if (!noop) {
		return ENOMEM;
	}

restart:
	error = nfs_mount_state_in_use_start(nmp, vfs_context_thread(ctx));
	if (error) {
		nfs_open_owner_rele(noop);
		return error;
	}

	                                            
	error = nfs_open_file_find(NULL, noop, &newnofp, 0, 0, 1);
	if (!error && (newnofp->nof_flags & NFS_OPEN_FILE_LOST)) {
		printf("nfs_vnop_create: LOST\n");
		error = EIO;
	}
	if (!error && (newnofp->nof_flags & NFS_OPEN_FILE_REOPEN)) {
		                                                                   
		error = nfs4_reopen(newnofp, vfs_context_thread(ctx));
		nfs_open_file_destroy(newnofp);
		newnofp = NULL;
		if (!error) {
			nfs_mount_state_in_use_end(nmp, 0);
			goto restart;
		}
	}
	if (!error) {
		error = nfs_open_file_set_busy(newnofp, vfs_context_thread(ctx));
	}
	if (error) {
		if (newnofp) {
			nfs_open_file_destroy(newnofp);
		}
		newnofp = NULL;
		goto out;
	}

	  
                                         
                                                          
    
	accessMode = NFS_OPEN_SHARE_ACCESS_BOTH;
	denyMode = NFS_OPEN_SHARE_DENY_NONE;

	                        
	error = nfs4_open_rpc(newnofp, ctx, cnp, vap, dvp, vpp, NFS_OPEN_CREATE, accessMode, denyMode);
	if ((error == EACCES) && vap && !(vap->va_vaflags & VA_EXCLUSIVE) &&
	    VATTR_IS_ACTIVE(vap, va_mode) && !(vap->va_mode & S_IWUSR)) {
		  
                                                                       
                                                                              
                                                                             
                                                                           
    
                                                                       
                       
     
		struct vnode_attr vattr;

		                                
		int error2 = nfs_lookitup(VTONFS(dvp), cnp->cn_nameptr, cnp->cn_namelen, ctx, &np);
		if (!error2 && np) {
			nfs_node_unlock(np);
			*vpp = NFSTOV(np);
			if (vnode_vtype(NFSTOV(np)) == VREG) {
				VATTR_INIT(&vattr);
				VATTR_SET(&vattr, va_mode, (vap->va_mode | S_IWUSR));
				if (!nfs4_setattr_rpc(np, &vattr, ctx)) {
					error2 = nfs4_open_rpc(newnofp, ctx, cnp, NULL, dvp, vpp, NFS_OPEN_NOCREATE, accessMode, denyMode);
					VATTR_INIT(&vattr);
					VATTR_SET(&vattr, va_mode, vap->va_mode);
					nfs4_setattr_rpc(np, &vattr, ctx);
					if (!error2) {
						error = 0;
					}
				}
			}
			if (error) {
				vnode_put(*vpp);
				*vpp = NULL;
			}
		}
	}
	if (!error && !*vpp) {
		printf("nfs4_open_rpc returned without a node?\n");
		                                                                    
		error = EIO;
	}
	if (error) {
		                                        
		nfs_open_file_clear_busy(newnofp);
		nfs_open_file_destroy(newnofp);
		newnofp = NULL;
		goto out;
	}
	                                                                
	np = VTONFS(*vpp);
	nfs_open_file_add_open(newnofp, accessMode, denyMode, 0);
	nofp = newnofp;
	error = nfs_open_file_find_internal(np, noop, &nofp, 0, 0, 0);
	if (error) {
		                                                                    
		printf("nfs_open_file_find_internal failed! %d\n", error);
		goto out;
	} else if (nofp != newnofp) {
		  
                                               
                                                           
                                     
                                                           
                                   
     
		busyerror = nfs_open_file_set_busy(nofp, NULL);
		nfs_open_file_add_open(nofp, accessMode, denyMode, 0);
		nofp->nof_stateid = newnofp->nof_stateid;
		if (newnofp->nof_flags & NFS_OPEN_FILE_POSIXLOCK) {
			nofp->nof_flags |= NFS_OPEN_FILE_POSIXLOCK;
		}
		nfs_open_file_clear_busy(newnofp);
		nfs_open_file_destroy(newnofp);
	}
	newnofp = NULL;
	                                                      
	nofp->nof_flags |= NFS_OPEN_FILE_CREATE;
	nofp->nof_creator = current_thread();
out:
	if (nofp && !busyerror) {
		nfs_open_file_clear_busy(nofp);
	}
	if (nfs_mount_state_in_use_end(nmp, error)) {
		nofp = newnofp = NULL;
		busyerror = 0;
		goto restart;
	}
	if (noop) {
		nfs_open_owner_rele(noop);
	}
	return error;
}

  
                                                                     
   
int
nfs4_create_rpc(
	vfs_context_t ctx,
	nfsnode_t dnp,
	struct componentname *cnp,
	struct vnode_attr *vap,
	int type,
	char *link,
	nfsnode_t *npp)
{
	struct nfsmount *nmp;
	struct nfs_vattr *nvattr;
	int error = 0, create_error = EIO, lockerror = ENOENT, busyerror = ENOENT, status;
	int nfsvers, namedattrs, numops;
	u_int64_t xid = 0, savedxid = 0;
	nfsnode_t np = NULL;
	vnode_t newvp = NULL;
	struct nfsm_chain nmreq, nmrep;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN], bmlen;
	const char *tag;
	nfs_specdata sd;
	fhandle_t *fh;
	struct nfsreq *req;
	struct nfs_dulookup *dul;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	namedattrs = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR);
	if (dnp->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	sd.specdata1 = sd.specdata2 = 0;

	switch (type) {
	case NFLNK:
		tag = "symlink";
		break;
	case NFBLK:
	case NFCHR:
		tag = "mknod";
		if (!VATTR_IS_ACTIVE(vap, va_rdev)) {
			return EINVAL;
		}
		sd.specdata1 = major(vap->va_rdev);
		sd.specdata2 = minor(vap->va_rdev);
		break;
	case NFSOCK:
	case NFFIFO:
		tag = "mknod";
		break;
	case NFDIR:
		tag = "mkdir";
		break;
	default:
		return EINVAL;
	}

	fh = zalloc(nfs_fhandle_zone);
	req = zalloc(nfs_req_zone);
	MALLOC(dul, struct nfs_dulookup *, sizeof(*dul), M_TEMP, M_WAITOK);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	nfs_avoid_needless_id_setting_on_create(dnp, vap, ctx);

	error = busyerror = nfs_node_set_busy(dnp, vfs_context_thread(ctx));
	if (!namedattrs) {
		nfs_dulookup_init(dul, dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx);
	}

	NFSREQ_SECINFO_SET(&si, dnp, NULL, 0, NULL, 0);
	NVATTR_INIT(nvattr);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                                                         
	numops = 6;
	nfsm_chain_build_alloc_init(error, &nmreq, 66 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, tag, nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, dnp->n_fhp, dnp->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_SAVEFH);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_CREATE);
	nfsm_chain_add_32(error, &nmreq, type);
	if (type == NFLNK) {
		nfsm_chain_add_name(error, &nmreq, link, strlen(link), nmp);
	} else if ((type == NFBLK) || (type == NFCHR)) {
		nfsm_chain_add_32(error, &nmreq, sd.specdata1);
		nfsm_chain_add_32(error, &nmreq, sd.specdata2);
	}
	nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	nfsm_chain_add_fattr4(error, &nmreq, vap, nmp);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, bitmap);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_FILEHANDLE);
	nfsm_chain_add_bitmap_supported(error, &nmreq, bitmap, nmp, NULL);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_RESTOREFH);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, dnp);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);

	error = nfs_request_async(dnp, NULL, &nmreq, NFSPROC4_COMPOUND,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), &si, 0, NULL, &req);
	if (!error) {
		if (!namedattrs) {
			nfs_dulookup_start(dul, dnp, ctx);
		}
		error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	}

	if ((lockerror = nfs_node_lock(dnp))) {
		error = lockerror;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_SAVEFH);
	nfsmout_if(error);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_CREATE);
	nfsm_chain_check_change_info(error, &nmrep, dnp);
	bmlen = NFS_ATTR_BITMAP_LEN;
	nfsm_chain_get_bitmap(error, &nmrep, bitmap, bmlen);
	                                                                
	                                                          
	create_error = error;
	nfsmout_if(error);
	nfs_vattr_set_supported(bitmap, vap);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsmout_if(error);
	error = nfs4_parsefattr(&nmrep, NULL, nvattr, fh, NULL, NULL);
	nfsmout_if(error);
	if (!NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_FILEHANDLE)) {
		printf("nfs: create/%s didn't return filehandle? %s\n", tag, cnp->cn_nameptr);
		error = EBADRPC;
		goto nfsmout;
	}
	                                                                         
	nfsm_chain_op_check(error, &nmrep, NFS_OP_RESTOREFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	savedxid = xid;
	nfsm_chain_loadattr(error, &nmrep, dnp, nfsvers, &xid);
	if (error) {
		NATTRINVALIDATE(dnp);
	}

nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	if (!lockerror) {
		if (!create_error && (dnp->n_flag & NNEGNCENTRIES)) {
			dnp->n_flag &= ~NNEGNCENTRIES;
			cache_purge_negatives(NFSTOV(dnp));
		}
		dnp->n_flag |= NMODIFIED;
		nfs_node_unlock(dnp);
		                                                       
		nfs_getattr(dnp, NULL, ctx, NGA_CACHED);
	}

	if (!error && fh->fh_len) {
		                                                         
		xid = savedxid;
		error = nfs_nget(NFSTOMP(dnp), dnp, cnp, fh->fh_data, fh->fh_len, nvattr, &xid, req->r_auth, NG_MAKEENTRY, &np);
		if (!error) {
			newvp = NFSTOV(np);
		}
	}

	if (!namedattrs) {
		nfs_dulookup_finish(dul, dnp, ctx);
	}

	NVATTR_CLEANUP(nvattr);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	NFS_ZFREE(nfs_req_zone, req);
	FREE(dul, M_TEMP);
	FREE(nvattr, M_TEMP);

	  
                                                                     
                                               
    
	if ((create_error == EEXIST) || (!create_error && !newvp)) {
		error = nfs_lookitup(dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx, &np);
		if (!error) {
			newvp = NFSTOV(np);
			if (vnode_vtype(newvp) != nfstov_type(type, nfsvers)) {
				error = EEXIST;
			}
		}
	}
	if (!busyerror) {
		nfs_node_clear_busy(dnp);
	}
	if (error) {
		if (newvp) {
			nfs_node_unlock(np);
			vnode_put(newvp);
		}
	} else {
		nfs_node_unlock(np);
		*npp = np;
	}
	return error;
}

int
nfs4_vnop_mknod(
	struct vnop_mknod_args     
                                                               
                                                 
                                                  
                                                               
                                                            
                                                           
                                       *ap)
{
	nfsnode_t np = NULL;
	struct nfsmount *nmp;
	int error;

	nmp = VTONMP(ap->a_dvp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (!VATTR_IS_ACTIVE(ap->a_vap, va_type)) {
		return EINVAL;
	}
	switch (ap->a_vap->va_type) {
	case VBLK:
	case VCHR:
	case VFIFO:
	case VSOCK:
		break;
	default:
		return ENOTSUP;
	}

	error = nfs4_create_rpc(ap->a_context, VTONFS(ap->a_dvp), ap->a_cnp, ap->a_vap,
	    vtonfs_type(ap->a_vap->va_type, nmp->nm_vers), NULL, &np);
	if (!error) {
		*ap->a_vpp = NFSTOV(np);
	}
	return error;
}

int
nfs4_vnop_mkdir(
	struct vnop_mkdir_args     
                                                               
                                                 
                                                  
                                                               
                                                            
                                                           
                                       *ap)
{
	nfsnode_t np = NULL;
	int error;

	error = nfs4_create_rpc(ap->a_context, VTONFS(ap->a_dvp), ap->a_cnp, ap->a_vap,
	    NFDIR, NULL, &np);
	if (!error) {
		*ap->a_vpp = NFSTOV(np);
	}
	return error;
}

int
nfs4_vnop_symlink(
	struct vnop_symlink_args     
                                                                 
                                                   
                                                    
                                                                 
                                                              
                                                    
                                                             
                                         *ap)
{
	nfsnode_t np = NULL;
	int error;

	error = nfs4_create_rpc(ap->a_context, VTONFS(ap->a_dvp), ap->a_cnp, ap->a_vap,
	    NFLNK, ap->a_target, &np);
	if (!error) {
		*ap->a_vpp = NFSTOV(np);
	}
	return error;
}

int
nfs4_vnop_link(
	struct vnop_link_args     
                                                              
                                               
                                                 
                                                              
                                                          
                                      *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	vnode_t tdvp = ap->a_tdvp;
	struct componentname *cnp = ap->a_cnp;
	int error = 0, lockerror = ENOENT, status;
	struct nfsmount *nmp;
	nfsnode_t np = VTONFS(vp);
	nfsnode_t tdnp = VTONFS(tdvp);
	int nfsvers, numops;
	u_int64_t xid, savedxid;
	struct nfsm_chain nmreq, nmrep;
	struct nfsreq_secinfo_args si;

	if (vnode_mount(vp) != vnode_mount(tdvp)) {
		return EXDEV;
	}

	nmp = VTONMP(vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	nfsvers = nmp->nm_vers;
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}
	if (tdnp->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return EINVAL;
	}

	  
                                                              
                                              
                                     
    
	nfs_flush(np, MNT_WAIT, vfs_context_thread(ctx), V_IGNORE_WRITEERR);

	if ((error = nfs_node_set_busy2(tdnp, np, vfs_context_thread(ctx)))) {
		return error;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	                                                                            
	numops = 7;
	nfsm_chain_build_alloc_init(error, &nmreq, 29 * NFSX_UNSIGNED + cnp->cn_namelen);
	nfsm_chain_add_compound_header(error, &nmreq, "link", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_SAVEFH);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nfsvers, tdnp->n_fhp, tdnp->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_LINK);
	nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, tdnp);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_RESTOREFH);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_supported(error, &nmreq, nfs_getattr_bitmap, nmp, np);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request(tdnp, NULL, &nmreq, NFSPROC4_COMPOUND, ctx, &si, &nmrep, &xid, &status);

	if ((lockerror = nfs_node_lock2(tdnp, np))) {
		error = lockerror;
		goto nfsmout;
	}
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_SAVEFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_LINK);
	nfsm_chain_check_change_info(error, &nmrep, tdnp);
	                                                                         
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	savedxid = xid;
	nfsm_chain_loadattr(error, &nmrep, tdnp, nfsvers, &xid);
	if (error) {
		NATTRINVALIDATE(tdnp);
	}
	                                                                    
	nfsm_chain_op_check(error, &nmrep, NFS_OP_RESTOREFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	xid = savedxid;
	nfsm_chain_loadattr(error, &nmrep, np, nfsvers, &xid);
	if (error) {
		NATTRINVALIDATE(np);
	}
nfsmout:
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
	if (!lockerror) {
		tdnp->n_flag |= NMODIFIED;
	}
	                                                                     
	if (error == EEXIST) {
		error = 0;
	}
	if (!error && (tdnp->n_flag & NNEGNCENTRIES)) {
		tdnp->n_flag &= ~NNEGNCENTRIES;
		cache_purge_negatives(tdvp);
	}
	if (!lockerror) {
		nfs_node_unlock2(tdnp, np);
	}
	nfs_node_clear_busy2(tdnp, np);
	return error;
}

int
nfs4_vnop_rmdir(
	struct vnop_rmdir_args     
                                                               
                                                 
                                                
                                                               
                                                           
                                       *ap)
{
	vfs_context_t ctx = ap->a_context;
	vnode_t vp = ap->a_vp;
	vnode_t dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	struct nfsmount *nmp;
	int error = 0, namedattrs;
	nfsnode_t np = VTONFS(vp);
	nfsnode_t dnp = VTONFS(dvp);
	struct nfs_dulookup *dul;

	if (vnode_vtype(vp) != VDIR) {
		return EINVAL;
	}

	nmp = NFSTONMP(dnp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	namedattrs = (nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR);

	if ((error = nfs_node_set_busy2(dnp, np, vfs_context_thread(ctx)))) {
		return error;
	}

	MALLOC(dul, struct nfs_dulookup *, sizeof(*dul), M_TEMP, M_WAITOK);
	if (!namedattrs) {
		nfs_dulookup_init(dul, dnp, cnp->cn_nameptr, cnp->cn_namelen, ctx);
		nfs_dulookup_start(dul, dnp, ctx);
	}

	error = nfs4_remove_rpc(dnp, cnp->cn_nameptr, cnp->cn_namelen,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx));

	nfs_name_cache_purge(dnp, np, cnp, ctx);
	                                                       
	nfs_getattr(dnp, NULL, ctx, NGA_CACHED);
	if (!namedattrs) {
		nfs_dulookup_finish(dul, dnp, ctx);
	}
	nfs_node_clear_busy2(dnp, np);

	  
                                                                      
    
	if (error == ENOENT) {
		error = 0;
	}
	if (!error) {
		  
                                                                  
                                                                  
                                     
     
		lck_mtx_lock(&nfs_node_hash_mutex);
		if (np->n_hflag & NHHASHED) {
			LIST_REMOVE(np, n_hash);
			np->n_hflag &= ~NHHASHED;
			FSDBG(266, 0, np, np->n_flag, 0xb1eb1e);
		}
		lck_mtx_unlock(&nfs_node_hash_mutex);
	}
	FREE(dul, M_TEMP);
	return error;
}

  
                         
  
                                                                         
                                                                          
                                                                        
                                                                          
                          
  
                                                                    
                                                                      
                                                                     
                                                                       
            
  
                                                              
   

  
                                                 
                                             
                                                                
                                          
   
nfsnode_t
nfs4_named_attr_dir_get(nfsnode_t np, int fetch, vfs_context_t ctx)
{
	nfsnode_t adnp = NULL;
	struct nfsmount *nmp;
	int error = 0, status, numops;
	struct nfsm_chain nmreq, nmrep;
	u_int64_t xid;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN];
	fhandle_t *fh;
	struct nfs_vattr *nvattr;
	struct componentname cn;
	struct nfsreq *req;
	struct nfsreq_secinfo_args si;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return NULL;
	}
	if (np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL) {
		return NULL;
	}

	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	fh = zalloc(nfs_fhandle_zone);
	req = zalloc(nfs_req_zone);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	NVATTR_INIT(nvattr);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	bzero(&cn, sizeof(cn));
	cn.cn_nameptr = __CAST_AWAY_QUALIFIER(_PATH_FORKSPECIFIER, const, char *);                      
	cn.cn_namelen = NFS_STRLEN_INT(_PATH_FORKSPECIFIER);
	cn.cn_nameiop = LOOKUP;

	if (np->n_attrdirfh) {
		                                             
		error = nfs_nget(nmp->nm_mountp, NULL, &cn, np->n_attrdirfh + 1, *np->n_attrdirfh,
		    NULL, NULL, RPCAUTH_UNKNOWN, NG_NOCREATE, &adnp);
		if (adnp) {
			goto nfsmout;
		}
	}
	if (!fetch) {
		error = ENOENT;
		goto nfsmout;
	}

	                           
	numops = 3;
	nfsm_chain_build_alloc_init(error, &nmreq, 22 * NFSX_UNSIGNED);
	nfsm_chain_add_compound_header(error, &nmreq, "openattr", nmp->nm_minor_vers, numops);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
	nfsm_chain_add_fh(error, &nmreq, nmp->nm_vers, np->n_fhp, np->n_fhsize);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_OPENATTR);
	nfsm_chain_add_32(error, &nmreq, 0);
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, bitmap);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_FILEHANDLE);
	nfsm_chain_add_bitmap_masked(error, &nmreq, bitmap,
	    NFS_ATTR_BITMAP_LEN, nmp->nm_fsattr.nfsa_supp_attr);
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request_async(np, NULL, &nmreq, NFSPROC4_COMPOUND,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), &si, 0, NULL, &req);
	if (!error) {
		error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	}

	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_OPENATTR);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsmout_if(error);
	error = nfs4_parsefattr(&nmrep, NULL, nvattr, fh, NULL, NULL);
	nfsmout_if(error);
	if (!NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_FILEHANDLE) || !fh->fh_len) {
		error = ENOENT;
		goto nfsmout;
	}
	if (!np->n_attrdirfh || (*np->n_attrdirfh != fh->fh_len)) {
		                                    
		if (np->n_attrdirfh) {
			FREE(np->n_attrdirfh, M_TEMP);
		}
		MALLOC(np->n_attrdirfh, u_char*, fh->fh_len + 1, M_TEMP, M_WAITOK);
	}
	if (!np->n_attrdirfh) {
		error = ENOMEM;
		goto nfsmout;
	}
	                                      
	*np->n_attrdirfh = (unsigned char)fh->fh_len;                                                                               
	bcopy(fh->fh_data, np->n_attrdirfh + 1, fh->fh_len);
	                             
	                                             
	error = nfs_nget(NFSTOMP(np), NULL, &cn, fh->fh_data, fh->fh_len, nvattr, &xid, req->r_auth, 0, &adnp);
nfsmout:
	NVATTR_CLEANUP(nvattr);
	NFS_ZFREE(nfs_fhandle_zone, fh);
	NFS_ZFREE(nfs_req_zone, req);
	FREE(nvattr, M_TEMP);
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);

	if (adnp) {
		                                                           
		if (adnp->n_vattr.nva_type != VDIR) {
			error = EINVAL;
		}
		if (!(adnp->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR)) {
			error = EINVAL;
		}
		nfs_node_unlock(adnp);
		if (error) {
			vnode_put(NFSTOV(adnp));
		}
	}
	return error ? NULL : adnp;
}

  
                                                                
  
                                                                             
                                                    
  
                                                                       
                                                           
                                                                       
                                                                                    
                                                                   
                                                                                  
  
                                                                              
                                                                           
                                                                       
  
                                                                               
                                                                           
   

#define NFS_GET_NAMED_ATTR_CREATE               0x1
#define NFS_GET_NAMED_ATTR_CREATE_GUARDED       0x2
#define NFS_GET_NAMED_ATTR_TRUNCATE             0x4
#define NFS_GET_NAMED_ATTR_PREFETCH             0x8

int
nfs4_named_attr_get(
	nfsnode_t np,
	struct componentname *cnp,
	uint32_t accessMode,
	int flags,
	vfs_context_t ctx,
	nfsnode_t *anpp,
	struct nfs_open_file **nofpp)
{
	struct nfsmount *nmp;
	int error = 0, open_error = EIO;
	int inuse = 0, adlockerror = ENOENT, busyerror = ENOENT, adbusyerror = ENOENT, nofpbusyerror = ENOENT;
	int create, guarded, prefetch, truncate, noopbusy = 0;
	int open, status, numops, hadattrdir, negnamecache;
	struct nfs_vattr *nvattr;
	struct vnode_attr vattr;
	nfsnode_t adnp = NULL, anp = NULL;
	vnode_t avp = NULL;
	u_int64_t xid = 0, savedxid = 0;
	struct nfsm_chain nmreq, nmrep;
	uint32_t bitmap[NFS_ATTR_BITMAP_LEN], bmlen;
	uint32_t denyMode = 0, rflags, delegation, recall, eof, rlen, retlen;
	nfs_stateid stateid, dstateid;
	fhandle_t *fh;
	struct nfs_open_owner *noop = NULL;
	struct nfs_open_file *newnofp = NULL, *nofp = NULL;
	struct vnop_access_args naa;
	thread_t thd;
	kauth_cred_t cred;
	struct timeval now;
	char sbuf[64], *s;
	uint32_t ace_type, ace_flags, ace_mask, len, slen;
	struct kauth_ace ace;
	struct nfsreq *req;
	struct nfsreq_secinfo_args si;

	*anpp = NULL;
	rflags = delegation = recall = eof = rlen = retlen = 0;
	ace.ace_flags = 0;
	s = sbuf;
	slen = sizeof(sbuf);

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	fh = zalloc(nfs_fhandle_zone);
	req = zalloc(nfs_req_zone);
	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	NVATTR_INIT(nvattr);
	fh->fh_len = 0;
	bzero(&dstateid, sizeof(dstateid));
	negnamecache = !NMFLAG(nmp, NONEGNAMECACHE);
	thd = vfs_context_thread(ctx);
	cred = vfs_context_ucred(ctx);
	create = (flags & NFS_GET_NAMED_ATTR_CREATE) ? NFS_OPEN_CREATE : NFS_OPEN_NOCREATE;
	guarded = (flags & NFS_GET_NAMED_ATTR_CREATE_GUARDED) ? NFS_CREATE_GUARDED : NFS_CREATE_UNCHECKED;
	truncate = (flags & NFS_GET_NAMED_ATTR_TRUNCATE);
	prefetch = (flags & NFS_GET_NAMED_ATTR_PREFETCH);

	if (!create) {
		error = nfs_getattr(np, nvattr, ctx, NGA_CACHED);
		if (error) {
			goto out_free;
		}
		if (NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_NAMED_ATTR) &&
		    !(nvattr->nva_flags & NFS_FFLAG_HAS_NAMED_ATTRS)) {
			error = ENOATTR;
			goto out_free;
		}
	} else if (accessMode == NFS_OPEN_SHARE_ACCESS_NONE) {
		                                          
		printf("nfs4_named_attr_get: create with no access %s\n", cnp->cn_nameptr);
		accessMode = NFS_OPEN_SHARE_ACCESS_READ;
	}
	open = (accessMode != NFS_OPEN_SHARE_ACCESS_NONE);
	if (open) {
		  
                                   
                                                     
                                  
     
		denyMode = NFS_OPEN_SHARE_DENY_NONE;
		if (prefetch && guarded) {
			prefetch = 0;                                                     
		}
		noop = nfs_open_owner_find(nmp, vfs_context_ucred(ctx), 1);
		if (!noop) {
			error = ENOMEM;
			goto out_free;
		}
	}

	if ((error = busyerror = nfs_node_set_busy(np, vfs_context_thread(ctx)))) {
		goto out_free;
	}

	adnp = nfs4_named_attr_dir_get(np, 0, ctx);
	hadattrdir = (adnp != NULL);
	if (prefetch) {
		microuptime(&now);
		                                                                       
		stateid.seqid = stateid.other[0] = stateid.other[1] = stateid.other[2] = 0;
		rlen = MIN(nmp->nm_rsize, nmp->nm_biosize);
	}
	NFSREQ_SECINFO_SET(&si, np, NULL, 0, NULL, 0);
	nfsm_chain_null(&nmreq);
	nfsm_chain_null(&nmrep);

	if (hadattrdir) {
		if ((error = adbusyerror = nfs_node_set_busy(adnp, vfs_context_thread(ctx)))) {
			goto nfsmout;
		}
		                                                       
		error = nfs_getattr(adnp, NULL, ctx, NGA_CACHED);
		nfsmout_if(error);
		error = cache_lookup(NFSTOV(adnp), &avp, cnp);
		switch (error) {
		case ENOENT:
			                          
			goto nfsmout;
		case 0:
			                
			                              
			error = nfs_dir_buf_cache_lookup(adnp, &anp, cnp, ctx, 0, NULL);
			if (!error && anp) {
				                       
				*anpp = anp;
				error = -1;
			}
			if (error != -1) {                 
				break;
			}
			OS_FALLTHROUGH;
		case -1:
			                                    
			OSAddAtomic64(1, &nfsstats.lookupcache_hits);
			if (!anp && avp) {
				*anpp = anp = VTONFS(avp);
			}

			nfs_node_clear_busy(adnp);
			adbusyerror = ENOENT;

			                                
			naa.a_desc = &vnop_access_desc;
			naa.a_vp = NFSTOV(adnp);
			naa.a_action = KAUTH_VNODE_SEARCH;
			naa.a_context = ctx;

			                                                           
			error = nfs_vnop_access(&naa);
			OS_FALLTHROUGH;
		default:
			                                         
			if (!error && guarded) {
				                                               
				error = EEXIST;
				vnode_put(NFSTOV(anp));
				*anpp = anp = NULL;
			}
			                                                  
			if (error || !open) {
				goto nfsmout;
			}
			                                     
		}
	}

	if (open) {
restart:
		error = nfs_mount_state_in_use_start(nmp, vfs_context_thread(ctx));
		if (error) {
			nfs_open_owner_rele(noop);
			noop = NULL;
			goto nfsmout;
		}
		inuse = 1;

		                                                                                
		error = nfs_open_file_find(anp, noop, &newnofp, 0, 0, 1);
		if (!error && (newnofp->nof_flags & NFS_OPEN_FILE_LOST)) {
			printf("nfs4_named_attr_get: LOST %d %s\n", kauth_cred_getuid(noop->noo_cred), cnp->cn_nameptr);
			error = EIO;
		}
		if (!error && (newnofp->nof_flags & NFS_OPEN_FILE_REOPEN)) {
			error = nfs4_reopen(newnofp, vfs_context_thread(ctx));
			nfs_open_file_destroy(newnofp);
			newnofp = NULL;
			if (!error) {
				nfs_mount_state_in_use_end(nmp, 0);
				inuse = 0;
				goto restart;
			}
		}
		if (!error) {
			error = nfs_open_file_set_busy(newnofp, vfs_context_thread(ctx));
		}
		if (error) {
			if (newnofp) {
				nfs_open_file_destroy(newnofp);
			}
			newnofp = NULL;
			goto nfsmout;
		}
		if (anp) {
			  
                                                        
                                                        
      
			open_error = error = nfs4_open(anp, newnofp, accessMode, denyMode, ctx);
			if (!error) {
				                                                             
				nofp = newnofp;
				nofpbusyerror = 0;
				newnofp = NULL;
				if (nofpp) {
					*nofpp = nofp;
				}
			}
			goto nfsmout;
		}
	}

	  
                                                                    
                                                        
   
                                                                            
                                                                         
                                     
                                                                             
                                  
    

	if (open) {
		                                                     
		if ((error = nfs_open_owner_set_busy(noop, thd))) {
			goto nfsmout;
		}
		noopbusy = 1;
	}

	  
                                                                
                                                                  
                                                                  
                                    
   
                                                                           
    
	numops = 5;
	if (!hadattrdir) {
		numops += 3;                                                
	}
	if (prefetch) {
		numops += 4;                                                     
	}
	nfsm_chain_build_alloc_init(error, &nmreq, 64 * NFSX_UNSIGNED + cnp->cn_namelen);
	nfsm_chain_add_compound_header(error, &nmreq, "getnamedattr", nmp->nm_minor_vers, numops);
	if (hadattrdir) {
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
		nfsm_chain_add_fh(error, &nmreq, nmp->nm_vers, adnp->n_fhp, adnp->n_fhsize);
	} else {
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
		nfsm_chain_add_fh(error, &nmreq, nmp->nm_vers, np->n_fhp, np->n_fhsize);
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_OPENATTR);
		nfsm_chain_add_32(error, &nmreq, create ? 1 : 0);
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
		NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, bitmap);
		NFS_BITMAP_SET(bitmap, NFS_FATTR_FILEHANDLE);
		nfsm_chain_add_bitmap_masked(error, &nmreq, bitmap,
		    NFS_ATTR_BITMAP_LEN, nmp->nm_fsattr.nfsa_supp_attr);
	}
	if (open) {
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_OPEN);
		nfsm_chain_add_32(error, &nmreq, noop->noo_seqid);
		nfsm_chain_add_32(error, &nmreq, accessMode);
		nfsm_chain_add_32(error, &nmreq, denyMode);
		nfsm_chain_add_64(error, &nmreq, nmp->nm_clientid);
		nfsm_chain_add_32(error, &nmreq, NFSX_UNSIGNED);
		nfsm_chain_add_32(error, &nmreq, kauth_cred_getuid(noop->noo_cred));
		nfsm_chain_add_32(error, &nmreq, create);
		if (create) {
			nfsm_chain_add_32(error, &nmreq, guarded);
			VATTR_INIT(&vattr);
			if (truncate) {
				VATTR_SET(&vattr, va_data_size, 0);
			}
			nfsm_chain_add_fattr4(error, &nmreq, &vattr, nmp);
		}
		nfsm_chain_add_32(error, &nmreq, NFS_CLAIM_NULL);
		nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	} else {
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_LOOKUP);
		nfsm_chain_add_name(error, &nmreq, cnp->cn_nameptr, cnp->cn_namelen, nmp);
	}
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	NFS_COPY_ATTRIBUTES(nfs_getattr_bitmap, bitmap);
	NFS_BITMAP_SET(bitmap, NFS_FATTR_FILEHANDLE);
	nfsm_chain_add_bitmap_masked(error, &nmreq, bitmap,
	    NFS_ATTR_BITMAP_LEN, nmp->nm_fsattr.nfsa_supp_attr);
	if (prefetch) {
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_SAVEFH);
	}
	if (hadattrdir) {
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
		nfsm_chain_add_fh(error, &nmreq, nmp->nm_vers, adnp->n_fhp, adnp->n_fhsize);
	} else {
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_PUTFH);
		nfsm_chain_add_fh(error, &nmreq, nmp->nm_vers, np->n_fhp, np->n_fhsize);
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_OPENATTR);
		nfsm_chain_add_32(error, &nmreq, 0);
	}
	numops--;
	nfsm_chain_add_32(error, &nmreq, NFS_OP_GETATTR);
	nfsm_chain_add_bitmap_masked(error, &nmreq, nfs_getattr_bitmap,
	    NFS_ATTR_BITMAP_LEN, nmp->nm_fsattr.nfsa_supp_attr);
	if (prefetch) {
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_RESTOREFH);
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_NVERIFY);
		VATTR_INIT(&vattr);
		VATTR_SET(&vattr, va_data_size, 0);
		nfsm_chain_add_fattr4(error, &nmreq, &vattr, nmp);
		numops--;
		nfsm_chain_add_32(error, &nmreq, NFS_OP_READ);
		nfsm_chain_add_stateid(error, &nmreq, &stateid);
		nfsm_chain_add_64(error, &nmreq, 0);
		nfsm_chain_add_32(error, &nmreq, rlen);
	}
	nfsm_chain_build_done(error, &nmreq);
	nfsm_assert(error, (numops == 0), EPROTO);
	nfsmout_if(error);
	error = nfs_request_async(hadattrdir ? adnp : np, NULL, &nmreq, NFSPROC4_COMPOUND,
	    vfs_context_thread(ctx), vfs_context_ucred(ctx), &si, open ? R_NOINTR: 0, NULL, &req);
	if (!error) {
		error = nfs_request_async_finish(req, &nmrep, &xid, &status);
	}

	if (hadattrdir && ((adlockerror = nfs_node_lock(adnp)))) {
		error = adlockerror;
	}
	savedxid = xid;
	nfsm_chain_skip_tag(error, &nmrep);
	nfsm_chain_get_32(error, &nmrep, numops);
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	if (!hadattrdir) {
		nfsm_chain_op_check(error, &nmrep, NFS_OP_OPENATTR);
		nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
		nfsmout_if(error);
		error = nfs4_parsefattr(&nmrep, NULL, nvattr, fh, NULL, NULL);
		nfsmout_if(error);
		if (NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_FILEHANDLE) && fh->fh_len) {
			if (!np->n_attrdirfh || (*np->n_attrdirfh != fh->fh_len)) {
				                                    
				if (np->n_attrdirfh) {
					FREE(np->n_attrdirfh, M_TEMP);
				}
				MALLOC(np->n_attrdirfh, u_char*, fh->fh_len + 1, M_TEMP, M_WAITOK);
			}
			if (np->n_attrdirfh) {
				                                         
				*np->n_attrdirfh = (unsigned char)fh->fh_len;                                                                                
				bcopy(fh->fh_data, np->n_attrdirfh + 1, fh->fh_len);
				                                    
				struct componentname cn;
				bzero(&cn, sizeof(cn));
				cn.cn_nameptr = __CAST_AWAY_QUALIFIER(_PATH_FORKSPECIFIER, const, char *);                      
				cn.cn_namelen = NFS_STRLEN_INT(_PATH_FORKSPECIFIER);
				cn.cn_nameiop = LOOKUP;
				                                             
				error = nfs_nget(NFSTOMP(np), NULL, &cn, fh->fh_data, fh->fh_len, nvattr, &xid, req->r_auth, 0, &adnp);
				if (!error) {
					adlockerror = 0;
					                       
					SET(adnp->n_flag, NBUSY);
					adbusyerror = 0;
				}
				                            
				error = 0;
			}
		}
		NVATTR_CLEANUP(nvattr);
		fh->fh_len = 0;
	}
	if (open) {
		nfsm_chain_op_check(error, &nmrep, NFS_OP_OPEN);
		nfs_owner_seqid_increment(noop, NULL, error);
		nfsm_chain_get_stateid(error, &nmrep, &newnofp->nof_stateid);
		nfsm_chain_check_change_info(error, &nmrep, adnp);
		nfsm_chain_get_32(error, &nmrep, rflags);
		bmlen = NFS_ATTR_BITMAP_LEN;
		nfsm_chain_get_bitmap(error, &nmrep, bitmap, bmlen);
		nfsm_chain_get_32(error, &nmrep, delegation);
		if (!error) {
			switch (delegation) {
			case NFS_OPEN_DELEGATE_NONE:
				break;
			case NFS_OPEN_DELEGATE_READ:
			case NFS_OPEN_DELEGATE_WRITE:
				nfsm_chain_get_stateid(error, &nmrep, &dstateid);
				nfsm_chain_get_32(error, &nmrep, recall);
				if (delegation == NFS_OPEN_DELEGATE_WRITE) {                    
					nfsm_chain_adv(error, &nmrep, 3 * NFSX_UNSIGNED);
				}
				                                                                  
				ace_type = ace_flags = ace_mask = len = 0;
				nfsm_chain_get_32(error, &nmrep, ace_type);
				nfsm_chain_get_32(error, &nmrep, ace_flags);
				nfsm_chain_get_32(error, &nmrep, ace_mask);
				nfsm_chain_get_32(error, &nmrep, len);
				ace.ace_flags = nfs4_ace_nfstype_to_vfstype(ace_type, &error);
				ace.ace_flags |= nfs4_ace_nfsflags_to_vfsflags(ace_flags);
				ace.ace_rights = nfs4_ace_nfsmask_to_vfsrights(ace_mask);
				if (!error && (len >= slen)) {
					MALLOC(s, char*, len + 1, M_TEMP, M_WAITOK);
					if (s) {
						slen = len + 1;
					} else {
						ace.ace_flags = 0;
					}
				}
				if (s) {
					nfsm_chain_get_opaque(error, &nmrep, len, s);
				} else {
					nfsm_chain_adv(error, &nmrep, nfsm_rndup(len));
				}
				if (!error && s) {
					s[len] = '\0';
					if (nfs4_id2guid(s, &ace.ace_applicable, (ace_flags & NFS_ACE_IDENTIFIER_GROUP))) {
						ace.ace_flags = 0;
					}
				}
				if (error || !s) {
					ace.ace_flags = 0;
				}
				if (s && (s != sbuf)) {
					FREE(s, M_TEMP);
				}
				break;
			default:
				error = EBADRPC;
				break;
			}
		}
		                                                                       
		open_error = error;
	} else {
		nfsm_chain_op_check(error, &nmrep, NFS_OP_LOOKUP);
	}
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsmout_if(error);
	error = nfs4_parsefattr(&nmrep, NULL, nvattr, fh, NULL, NULL);
	nfsmout_if(error);
	if (!NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_FILEHANDLE) || !fh->fh_len) {
		error = EIO;
		goto nfsmout;
	}
	if (prefetch) {
		nfsm_chain_op_check(error, &nmrep, NFS_OP_SAVEFH);
	}
	nfsm_chain_op_check(error, &nmrep, NFS_OP_PUTFH);
	if (!hadattrdir) {
		nfsm_chain_op_check(error, &nmrep, NFS_OP_OPENATTR);
	}
	nfsm_chain_op_check(error, &nmrep, NFS_OP_GETATTR);
	nfsmout_if(error);
	xid = savedxid;
	nfsm_chain_loadattr(error, &nmrep, adnp, nmp->nm_vers, &xid);
	nfsmout_if(error);

	if (open) {
		if (rflags & NFS_OPEN_RESULT_LOCKTYPE_POSIX) {
			newnofp->nof_flags |= NFS_OPEN_FILE_POSIXLOCK;
		}
		if (rflags & NFS_OPEN_RESULT_CONFIRM) {
			if (adnp) {
				nfs_node_unlock(adnp);
				adlockerror = ENOENT;
			}
			NVATTR_CLEANUP(nvattr);
			error = nfs4_open_confirm_rpc(nmp, adnp ? adnp : np, fh->fh_data, fh->fh_len, noop, &newnofp->nof_stateid, thd, cred, nvattr, &xid);
			nfsmout_if(error);
			savedxid = xid;
			if ((adlockerror = nfs_node_lock(adnp))) {
				error = adlockerror;
			}
		}
	}

nfsmout:
	if (open && adnp && !adlockerror) {
		if (!open_error && (adnp->n_flag & NNEGNCENTRIES)) {
			adnp->n_flag &= ~NNEGNCENTRIES;
			cache_purge_negatives(NFSTOV(adnp));
		}
		adnp->n_flag |= NMODIFIED;
		nfs_node_unlock(adnp);
		adlockerror = ENOENT;
		nfs_getattr(adnp, NULL, ctx, NGA_CACHED);
	}
	if (adnp && !adlockerror && (error == ENOENT) &&
	    (cnp->cn_flags & MAKEENTRY) && (cnp->cn_nameiop != CREATE) && negnamecache) {
		                                            
		cache_enter(NFSTOV(adnp), NULL, cnp);
		adnp->n_flag |= NNEGNCENTRIES;
	}
	if (adnp && !adlockerror) {
		nfs_node_unlock(adnp);
		adlockerror = ENOENT;
	}
	if (!error && !anp && fh->fh_len) {
		                                                         
		xid = savedxid;
		error = nfs_nget(NFSTOMP(np), adnp, cnp, fh->fh_data, fh->fh_len, nvattr, &xid, req->r_auth, NG_MAKEENTRY, &anp);
		if (!error) {
			*anpp = anp;
			nfs_node_unlock(anp);
		}
		if (!error && open) {
			nfs_open_file_add_open(newnofp, accessMode, denyMode, 0);
			                                                                
			nofp = newnofp;
			error = nfs_open_file_find_internal(anp, noop, &nofp, 0, 0, 0);
			if (error) {
				                                                                    
				printf("nfs_open_file_find_internal failed! %d\n", error);
				nofp = NULL;
			} else if (nofp != newnofp) {
				  
                                                 
                                                             
                                       
                                                             
                                     
       
				nofpbusyerror = nfs_open_file_set_busy(nofp, NULL);
				nfs_open_file_add_open(nofp, accessMode, denyMode, 0);
				nofp->nof_stateid = newnofp->nof_stateid;
				if (newnofp->nof_flags & NFS_OPEN_FILE_POSIXLOCK) {
					nofp->nof_flags |= NFS_OPEN_FILE_POSIXLOCK;
				}
				nfs_open_file_clear_busy(newnofp);
				nfs_open_file_destroy(newnofp);
				newnofp = NULL;
			}
			if (!error) {
				newnofp = NULL;
				nofpbusyerror = 0;
				                                                      
				nofp->nof_flags |= NFS_OPEN_FILE_CREATE;
				nofp->nof_creator = current_thread();
				if (nofpp) {
					*nofpp = nofp;
				}
			}
		}
	}
	NVATTR_CLEANUP(nvattr);
	if (open && ((delegation == NFS_OPEN_DELEGATE_READ) || (delegation == NFS_OPEN_DELEGATE_WRITE))) {
		if (!error && anp && !recall) {
			                                            
			lck_mtx_lock(&anp->n_openlock);
			anp->n_openflags &= ~N_DELEG_MASK;
			anp->n_openflags |= ((delegation == NFS_OPEN_DELEGATE_READ) ? N_DELEG_READ : N_DELEG_WRITE);
			anp->n_dstateid = dstateid;
			anp->n_dace = ace;
			if (anp->n_dlink.tqe_next == NFSNOLIST) {
				lck_mtx_lock(&nmp->nm_lock);
				if (anp->n_dlink.tqe_next == NFSNOLIST) {
					TAILQ_INSERT_TAIL(&nmp->nm_delegations, anp, n_dlink);
				}
				lck_mtx_unlock(&nmp->nm_lock);
			}
			lck_mtx_unlock(&anp->n_openlock);
		} else {
			                              
			if (anp) {
				if (NFS_CMPFH(anp, fh->fh_data, fh->fh_len)) {
					                                           
					lck_mtx_lock(&anp->n_openlock);
					anp->n_openflags &= ~N_DELEG_MASK;
					anp->n_openflags |= ((delegation == NFS_OPEN_DELEGATE_READ) ? N_DELEG_READ : N_DELEG_WRITE);
					anp->n_dstateid = dstateid;
					anp->n_dace = ace;
					if (anp->n_dlink.tqe_next == NFSNOLIST) {
						lck_mtx_lock(&nmp->nm_lock);
						if (anp->n_dlink.tqe_next == NFSNOLIST) {
							TAILQ_INSERT_TAIL(&nmp->nm_delegations, anp, n_dlink);
						}
						lck_mtx_unlock(&nmp->nm_lock);
					}
					lck_mtx_unlock(&anp->n_openlock);
					                                                      
					fh->fh_len = 0;
				}
				                                     
				nfs4_delegation_return(anp, 0, thd, cred);
			}
			if (fh->fh_len) {                                                  
				nfs4_delegreturn_rpc(nmp, fh->fh_data, fh->fh_len, &dstateid, 0, thd, cred);
			}
		}
	}
	if (open) {
		if (newnofp) {
			                                        
			nfs_open_file_clear_busy(newnofp);
			nfs_open_file_destroy(newnofp);
			newnofp = NULL;
		} else if (nofp && !nofpbusyerror) {
			nfs_open_file_clear_busy(nofp);
			nofpbusyerror = ENOENT;
		}
		if (inuse && nfs_mount_state_in_use_end(nmp, error)) {
			inuse = 0;
			nofp = newnofp = NULL;
			rflags = delegation = recall = eof = rlen = retlen = 0;
			ace.ace_flags = 0;
			s = sbuf;
			slen = sizeof(sbuf);
			nfsm_chain_cleanup(&nmreq);
			nfsm_chain_cleanup(&nmrep);
			if (anp) {
				vnode_put(NFSTOV(anp));
				*anpp = anp = NULL;
			}
			hadattrdir = (adnp != NULL);
			if (noopbusy) {
				nfs_open_owner_clear_busy(noop);
				noopbusy = 0;
			}
			goto restart;
		}
		inuse = 0;
		if (noop) {
			if (noopbusy) {
				nfs_open_owner_clear_busy(noop);
				noopbusy = 0;
			}
			nfs_open_owner_rele(noop);
		}
	}
	if (!error && prefetch && nmrep.nmc_mhead) {
		nfsm_chain_op_check(error, &nmrep, NFS_OP_RESTOREFH);
		nfsm_chain_op_check(error, &nmrep, NFS_OP_NVERIFY);
		nfsm_chain_op_check(error, &nmrep, NFS_OP_READ);
		nfsm_chain_get_32(error, &nmrep, eof);
		nfsm_chain_get_32(error, &nmrep, retlen);
		if (!error && anp) {
			  
                                                       
                                                                
                                                             
                                                             
                                                               
                                                              
                                       
                                                                
                                                              
                 
                                                               
                                                          
      
			struct nfsbuf *bp = NULL;
			int lastpg;
			nfsbufpgs pagemask, pagemaskand;

			retlen = MIN(retlen, rlen);

			                                                     
			if (ISSET(anp->n_flag, NUPDATESIZE)) {
				nfs_data_update_size(anp, 0);
			}
			if (!(error = nfs_node_lock(anp))) {
				if (anp->n_flag & NNEEDINVALIDATE) {
					anp->n_flag &= ~NNEEDINVALIDATE;
					nfs_node_unlock(anp);
					error = nfs_vinvalbuf(NFSTOV(anp), V_SAVE | V_IGNORE_WRITEERR, ctx, 1);
					if (!error) {                                               
						error = EIO;
					}
				} else {
					nfs_node_unlock(anp);
				}
			}

			                                                    
			lastpg = (retlen - 1) / PAGE_SIZE;
			nfs_buf_pgs_get_page_mask(&pagemask, lastpg + 1);

			if (!error) {
				error = nfs_buf_get(anp, 0, nmp->nm_biosize, thd, NBLK_READ | NBLK_NOWAIT, &bp);
			}
			                                                            
			nfs_buf_pgs_bit_and(&bp->nb_dirty, &pagemask, &pagemaskand);
			if (!error && bp && !bp->nb_dirtyoff && !nfs_buf_pgs_is_set(&pagemaskand) &&
			    timevalcmp(&anp->n_lastio, &now, <)) {
				OSAddAtomic64(1, &nfsstats.read_bios);
				CLR(bp->nb_flags, (NB_DONE | NB_ASYNC));
				SET(bp->nb_flags, NB_READ);
				NFS_BUF_MAP(bp);
				nfsm_chain_get_opaque(error, &nmrep, retlen, bp->nb_data);
				if (error) {
					bp->nb_error = error;
					SET(bp->nb_flags, NB_ERROR);
				} else {
					bp->nb_offio = 0;
					bp->nb_endio = rlen;
					if ((retlen > 0) && (bp->nb_endio < (int)retlen)) {
						bp->nb_endio = retlen;
					}
					if (eof || (retlen == 0)) {
						                                             
						off_t rpcrem, eofrem, rem;
						rpcrem = (rlen - retlen);
						eofrem = anp->n_size - (NBOFF(bp) + retlen);
						rem = (rpcrem < eofrem) ? rpcrem : eofrem;
						if (rem > 0) {
							bzero(bp->nb_data + retlen, rem);
						}
					} else if ((retlen < rlen) && !ISSET(bp->nb_flags, NB_ERROR)) {
						                                                      
						SET(bp->nb_flags, NB_INVAL);
					}
				}
				nfs_buf_read_finish(bp);
				microuptime(&anp->n_lastio);
			}
			if (bp) {
				nfs_buf_release(bp, 1);
			}
		}
		error = 0;                                                            
	}
	if (adnp && !adbusyerror) {
		nfs_node_clear_busy(adnp);
		adbusyerror = ENOENT;
	}
	if (!busyerror) {
		nfs_node_clear_busy(np);
		busyerror = ENOENT;
	}
	if (adnp) {
		vnode_put(NFSTOV(adnp));
	}
	if (inuse) {
		nfs_mount_state_in_use_end(nmp, error);
	}
	if (error && *anpp) {
		vnode_put(NFSTOV(*anpp));
		*anpp = NULL;
	}
	nfsm_chain_cleanup(&nmreq);
	nfsm_chain_cleanup(&nmrep);
out_free:
	NFS_ZFREE(nfs_fhandle_zone, fh);
	NFS_ZFREE(nfs_req_zone, req);
	FREE(nvattr, M_TEMP);
	return error;
}

  
                            
   
int
nfs4_named_attr_remove(nfsnode_t np, nfsnode_t anp, const char *name, vfs_context_t ctx)
{
	nfsnode_t adnp = NULL;
	struct nfsmount *nmp;
	struct componentname cn;
	struct vnop_remove_args vra;
	int error, putanp = 0;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	bzero(&cn, sizeof(cn));
	cn.cn_nameptr = __CAST_AWAY_QUALIFIER(name, const, char *);
	cn.cn_namelen = NFS_STRLEN_INT(name);
	cn.cn_nameiop = DELETE;
	cn.cn_flags = 0;

	if (!anp) {
		error = nfs4_named_attr_get(np, &cn, NFS_OPEN_SHARE_ACCESS_NONE,
		    0, ctx, &anp, NULL);
		if ((!error && !anp) || (error == ENOATTR)) {
			error = ENOENT;
		}
		if (error) {
			if (anp) {
				vnode_put(NFSTOV(anp));
				anp = NULL;
			}
			goto out;
		}
		putanp = 1;
	}

	if ((error = nfs_node_set_busy(np, vfs_context_thread(ctx)))) {
		goto out;
	}
	adnp = nfs4_named_attr_dir_get(np, 1, ctx);
	nfs_node_clear_busy(np);
	if (!adnp) {
		error = ENOENT;
		goto out;
	}

	vra.a_desc = &vnop_remove_desc;
	vra.a_dvp = NFSTOV(adnp);
	vra.a_vp = NFSTOV(anp);
	vra.a_cnp = &cn;
	vra.a_flags = 0;
	vra.a_context = ctx;
	error = nfs_vnop_remove(&vra);
out:
	if (adnp) {
		vnode_put(NFSTOV(adnp));
	}
	if (putanp) {
		vnode_put(NFSTOV(anp));
	}
	return error;
}

int
nfs4_vnop_getxattr(
	struct vnop_getxattr_args     
                                                                  
                                                   
                                                          
                                                  
                                                     
                                                    
                                                              
                                          *ap)
{
	vfs_context_t ctx = ap->a_context;
	struct nfsmount *nmp;
	struct nfs_vattr *nvattr;
	struct componentname cn;
	nfsnode_t anp;
	int error = 0, isrsrcfork;

	nmp = VTONMP(ap->a_vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (!(nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR)) {
		return ENOTSUP;
	}

	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	error = nfs_getattr(VTONFS(ap->a_vp), nvattr, ctx, NGA_CACHED);
	if (error) {
		goto out;
	}
	if (NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_NAMED_ATTR) &&
	    !(nvattr->nva_flags & NFS_FFLAG_HAS_NAMED_ATTRS)) {
		error = ENOATTR;
		goto out;
	}

	bzero(&cn, sizeof(cn));
	cn.cn_nameptr = __CAST_AWAY_QUALIFIER(ap->a_name, const, char *);
	cn.cn_namelen = NFS_STRLEN_INT(ap->a_name);
	cn.cn_nameiop = LOOKUP;
	cn.cn_flags = MAKEENTRY;

	                                                                                            
	isrsrcfork = (bcmp(ap->a_name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) == 0);

	error = nfs4_named_attr_get(VTONFS(ap->a_vp), &cn, NFS_OPEN_SHARE_ACCESS_NONE,
	    !isrsrcfork ? NFS_GET_NAMED_ATTR_PREFETCH : 0, ctx, &anp, NULL);
	if ((!error && !anp) || (error == ENOENT)) {
		error = ENOATTR;
	}
	if (!error) {
		if (ap->a_uio) {
			error = nfs_bioread(anp, ap->a_uio, 0, ctx);
		} else {
			*ap->a_size = anp->n_size;
		}
	}
	if (anp) {
		vnode_put(NFSTOV(anp));
	}
out:
	FREE(nvattr, M_TEMP);
	return error;
}

int
nfs4_vnop_setxattr(
	struct vnop_setxattr_args     
                                                                  
                                                   
                                                          
                                                  
                                                    
                                                              
                                          *ap)
{
	vfs_context_t ctx = ap->a_context;
	int options = ap->a_options;
	uio_t uio = ap->a_uio;
	const char *name = ap->a_name;
	struct nfsmount *nmp;
	struct componentname cn;
	nfsnode_t anp = NULL;
	int error = 0, closeerror = 0, flags, isrsrcfork, isfinderinfo, empty = 0, i;
#define FINDERINFOSIZE 32
	uint8_t finfo[FINDERINFOSIZE];
	uint32_t *finfop;
	struct nfs_open_file *nofp = NULL;
	char uio_buf[UIO_SIZEOF(1)];
	uio_t auio;
	struct vnop_write_args vwa;

	nmp = VTONMP(ap->a_vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (!(nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR)) {
		return ENOTSUP;
	}

	if ((options & XATTR_CREATE) && (options & XATTR_REPLACE)) {
		return EINVAL;
	}

	                                                                
	if (uio_iovcnt(uio) > 1) {
		printf("nfs4_vnop_setxattr: iovcnt > 1\n");
		return EINVAL;
	}

	bzero(&cn, sizeof(cn));
	cn.cn_nameptr = __CAST_AWAY_QUALIFIER(name, const, char *);
	cn.cn_namelen = NFS_STRLEN_INT(name);
	cn.cn_nameiop = CREATE;
	cn.cn_flags = MAKEENTRY;

	isfinderinfo = (bcmp(name, XATTR_FINDERINFO_NAME, sizeof(XATTR_FINDERINFO_NAME)) == 0);
	isrsrcfork = isfinderinfo ? 0 : (bcmp(name, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME)) == 0);
	if (!isrsrcfork) {
		uio_setoffset(uio, 0);
	}
	if (isfinderinfo) {
		if (uio_resid(uio) != sizeof(finfo)) {
			return ERANGE;
		}
		error = uiomove((char*)&finfo, sizeof(finfo), uio);
		if (error) {
			return error;
		}
		                                                                    
		empty = 1;
		for (i = 0, finfop = (uint32_t*)&finfo; i < (int)(sizeof(finfo) / sizeof(uint32_t)); i++) {
			if (finfop[i]) {
				empty = 0;
				break;
			}
		}
		if (empty && !(options & (XATTR_CREATE | XATTR_REPLACE))) {
			error = nfs4_named_attr_remove(VTONFS(ap->a_vp), anp, name, ctx);
			if (error == ENOENT) {
				error = 0;
			}
			return error;
		}
		                                                       
	}

	  
                         
   
                                                           
                                                            
                                                            
                                                 
    
	flags = 0;
	if (!(options & XATTR_REPLACE)) {
		flags |= NFS_GET_NAMED_ATTR_CREATE;
	}
	if (options & XATTR_CREATE) {
		flags |= NFS_GET_NAMED_ATTR_CREATE_GUARDED;
	}
	if (!isrsrcfork) {
		flags |= NFS_GET_NAMED_ATTR_TRUNCATE;
	}

	error = nfs4_named_attr_get(VTONFS(ap->a_vp), &cn, NFS_OPEN_SHARE_ACCESS_BOTH,
	    flags, ctx, &anp, &nofp);
	if (!error && !anp) {
		error = ENOATTR;
	}
	if (error) {
		goto out;
	}
	                                                  
	if (nofp && !(error = nfs_open_file_set_busy(nofp, NULL))) {
		nofp->nof_flags &= ~NFS_OPEN_FILE_CREATE;
		nofp->nof_creator = NULL;
		nfs_open_file_clear_busy(nofp);
	}

	                                                                                  
	if (isfinderinfo && empty) {
		goto doclose;
	}

	  
                                 
   
                                                                                 
    
	vwa.a_desc = &vnop_write_desc;
	vwa.a_vp = NFSTOV(anp);
	vwa.a_uio = NULL;
	vwa.a_ioflag = 0;
	vwa.a_context = ctx;
	if (isfinderinfo) {
		auio = uio_createwithbuffer(1, 0, UIO_SYSSPACE, UIO_WRITE, &uio_buf, sizeof(uio_buf));
		uio_addiov(auio, (uintptr_t)&finfo, sizeof(finfo));
		vwa.a_uio = auio;
	} else if (uio_resid(uio) > 0) {
		vwa.a_uio = uio;
	}
	if (vwa.a_uio) {
		error = nfs_vnop_write(&vwa);
		if (!error) {
			error = nfs_flush(anp, MNT_WAIT, vfs_context_thread(ctx), 0);
		}
	}
doclose:
	                      
	if (nofp) {
		int busyerror = nfs_open_file_set_busy(nofp, NULL);
		closeerror = nfs_close(anp, nofp, NFS_OPEN_SHARE_ACCESS_BOTH, NFS_OPEN_SHARE_DENY_NONE, ctx);
		if (!busyerror) {
			nfs_open_file_clear_busy(nofp);
		}
	}
	if (!error && isfinderinfo && empty) {                                                         
		error = nfs4_named_attr_remove(VTONFS(ap->a_vp), anp, name, ctx);
		if (error == ENOENT) {
			error = 0;
		}
	}
	if (!error) {
		error = closeerror;
	}
out:
	if (anp) {
		vnode_put(NFSTOV(anp));
	}
	if (error == ENOENT) {
		error = ENOATTR;
	}
	return error;
}

int
nfs4_vnop_removexattr(
	struct vnop_removexattr_args     
                                                                     
                                                      
                                                             
                                                       
                                                                 
                                             *ap)
{
	struct nfsmount *nmp = VTONMP(ap->a_vp);
	int error;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	if (!(nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR)) {
		return ENOTSUP;
	}

	error = nfs4_named_attr_remove(VTONFS(ap->a_vp), NULL, ap->a_name, ap->a_context);
	if (error == ENOENT) {
		error = ENOATTR;
	}
	return error;
}

int
nfs4_vnop_listxattr(
	struct vnop_listxattr_args     
                                                                   
                                                    
                                                   
                                                      
                                                     
                                                               
                                           *ap)
{
	vfs_context_t ctx = ap->a_context;
	nfsnode_t np = VTONFS(ap->a_vp);
	uio_t uio = ap->a_uio;
	nfsnode_t adnp = NULL;
	struct nfsmount *nmp;
	int error, done, i;
	struct nfs_vattr *nvattr;
	uint64_t cookie, nextcookie, lbn = 0;
	struct nfsbuf *bp = NULL;
	struct nfs_dir_buf_header *ndbhp;
	struct direntry *dp;

	nmp = VTONMP(ap->a_vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (!(nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR)) {
		return ENOTSUP;
	}

	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	error = nfs_getattr(np, nvattr, ctx, NGA_CACHED);
	if (error) {
		goto out_free;
	}
	if (NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_NAMED_ATTR) &&
	    !(nvattr->nva_flags & NFS_FFLAG_HAS_NAMED_ATTRS)) {
		error = 0;
		goto out_free;
	}

	if ((error = nfs_node_set_busy(np, vfs_context_thread(ctx)))) {
		goto out_free;
	}
	adnp = nfs4_named_attr_dir_get(np, 1, ctx);
	nfs_node_clear_busy(np);
	if (!adnp) {
		goto out;
	}

	if ((error = nfs_node_lock(adnp))) {
		goto out;
	}

	if (adnp->n_flag & NNEEDINVALIDATE) {
		adnp->n_flag &= ~NNEEDINVALIDATE;
		nfs_invaldir(adnp);
		nfs_node_unlock(adnp);
		error = nfs_vinvalbuf(NFSTOV(adnp), 0, ctx, 1);
		if (!error) {
			error = nfs_node_lock(adnp);
		}
		if (error) {
			goto out;
		}
	}

	  
                                                               
    
	if (adnp->n_flag & NMODIFIED) {
		nfs_invaldir(adnp);
		nfs_node_unlock(adnp);
		if ((error = nfs_vinvalbuf(NFSTOV(adnp), 0, ctx, 1))) {
			goto out;
		}
	} else {
		nfs_node_unlock(adnp);
	}
	                                                       
	if ((error = nfs_getattr(adnp, nvattr, ctx, NGA_UNCACHED))) {
		goto out;
	}

	if (uio && (uio_resid(uio) == 0)) {
		goto out;
	}

	done = 0;
	nextcookie = lbn = 0;

	while (!error && !done) {
		OSAddAtomic64(1, &nfsstats.biocache_readdirs);
		cookie = nextcookie;
getbuffer:
		error = nfs_buf_get(adnp, lbn, NFS_DIRBLKSIZ, vfs_context_thread(ctx), NBLK_READ, &bp);
		if (error) {
			goto out;
		}
		ndbhp = (struct nfs_dir_buf_header*)bp->nb_data;
		if (!ISSET(bp->nb_flags, NB_CACHE) || !ISSET(ndbhp->ndbh_flags, NDB_FULL)) {
			if (!ISSET(bp->nb_flags, NB_CACHE)) {                            
				ndbhp->ndbh_flags = 0;
				ndbhp->ndbh_count = 0;
				ndbhp->ndbh_entry_end = sizeof(*ndbhp);
				ndbhp->ndbh_ncgen = adnp->n_ncgen;
			}
			error = nfs_buf_readdir(bp, ctx);
			if (error == NFSERR_DIRBUFDROPPED) {
				goto getbuffer;
			}
			if (error) {
				nfs_buf_release(bp, 1);
			}
			if (error && (error != ENXIO) && (error != ETIMEDOUT) && (error != EINTR) && (error != ERESTART)) {
				if (!nfs_node_lock(adnp)) {
					nfs_invaldir(adnp);
					nfs_node_unlock(adnp);
				}
				nfs_vinvalbuf(NFSTOV(adnp), 0, ctx, 1);
				if (error == NFSERR_BAD_COOKIE) {
					error = ENOENT;
				}
			}
			if (error) {
				goto out;
			}
		}

		                                                 
		dp = NFS_DIR_BUF_FIRST_DIRENTRY(bp);
		for (i = 0; i < ndbhp->ndbh_count; i++) {
			if (!xattr_protected(dp->d_name)) {
				if (uio == NULL) {
					*ap->a_size += dp->d_namlen + 1;
				} else if (uio_resid(uio) < (dp->d_namlen + 1)) {
					error = ERANGE;
				} else {
					error = uiomove(dp->d_name, dp->d_namlen + 1, uio);
					if (error && (error != EFAULT)) {
						error = ERANGE;
					}
				}
			}
			nextcookie = dp->d_seekoff;
			dp = NFS_DIRENTRY_NEXT(dp);
		}

		if (i == ndbhp->ndbh_count) {
			                                            
			lbn = nextcookie;
			                                    
			if (ISSET(ndbhp->ndbh_flags, NDB_EOF)) {
				done = 1;
			}
		}
		if (!error && !done && (nextcookie == cookie)) {
			printf("nfs readdir cookie didn't change 0x%llx, %d/%d\n", cookie, i, ndbhp->ndbh_count);
			error = EIO;
		}
		nfs_buf_release(bp, 1);
	}
out:
	if (adnp) {
		vnode_put(NFSTOV(adnp));
	}
out_free:
	FREE(nvattr, M_TEMP);
	return error;
}

#if NAMEDSTREAMS
int
nfs4_vnop_getnamedstream(
	struct vnop_getnamedstream_args     
                                                                        
                                                         
                                                            
                                                               
                                                                         
                                                        
                                                                    
                                                *ap)
{
	vfs_context_t ctx = ap->a_context;
	struct nfsmount *nmp;
	struct nfs_vattr *nvattr;
	struct componentname cn;
	nfsnode_t anp;
	int error = 0;

	nmp = VTONMP(ap->a_vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (!(nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR)) {
		return ENOTSUP;
	}

	MALLOC(nvattr, struct nfs_vattr *, sizeof(*nvattr), M_TEMP, M_WAITOK);
	error = nfs_getattr(VTONFS(ap->a_vp), nvattr, ctx, NGA_CACHED);
	if (error) {
		goto out;
	}
	if (NFS_BITMAP_ISSET(nvattr->nva_bitmap, NFS_FATTR_NAMED_ATTR) &&
	    !(nvattr->nva_flags & NFS_FFLAG_HAS_NAMED_ATTRS)) {
		error = ENOATTR;
		goto out;
	}

	bzero(&cn, sizeof(cn));
	cn.cn_nameptr = __CAST_AWAY_QUALIFIER(ap->a_name, const, char *);
	cn.cn_namelen = NFS_STRLEN_INT(ap->a_name);
	cn.cn_nameiop = LOOKUP;
	cn.cn_flags = MAKEENTRY;

	error = nfs4_named_attr_get(VTONFS(ap->a_vp), &cn, NFS_OPEN_SHARE_ACCESS_NONE,
	    0, ctx, &anp, NULL);
	if ((!error && !anp) || (error == ENOENT)) {
		error = ENOATTR;
	}
	if (!error && anp) {
		*ap->a_svpp = NFSTOV(anp);
	} else if (anp) {
		vnode_put(NFSTOV(anp));
	}
out:
	FREE(nvattr, M_TEMP);
	return error;
}

int
nfs4_vnop_makenamedstream(
	struct vnop_makenamedstream_args     
                                                                         
                                                             
                                                          
                                                                
                                                         
                                                                     
                                                 *ap)
{
	vfs_context_t ctx = ap->a_context;
	struct nfsmount *nmp;
	struct componentname cn;
	nfsnode_t anp;
	int error = 0;

	nmp = VTONMP(ap->a_vp);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	if (!(nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR)) {
		return ENOTSUP;
	}

	bzero(&cn, sizeof(cn));
	cn.cn_nameptr = __CAST_AWAY_QUALIFIER(ap->a_name, const, char *);
	cn.cn_namelen = NFS_STRLEN_INT(ap->a_name);
	cn.cn_nameiop = CREATE;
	cn.cn_flags = MAKEENTRY;

	error = nfs4_named_attr_get(VTONFS(ap->a_vp), &cn, NFS_OPEN_SHARE_ACCESS_BOTH,
	    NFS_GET_NAMED_ATTR_CREATE, ctx, &anp, NULL);
	if ((!error && !anp) || (error == ENOENT)) {
		error = ENOATTR;
	}
	if (!error && anp) {
		*ap->a_svpp = NFSTOV(anp);
	} else if (anp) {
		vnode_put(NFSTOV(anp));
	}
	return error;
}

int
nfs4_vnop_removenamedstream(
	struct vnop_removenamedstream_args     
                                                                           
                                                            
                                                             
                                                                  
                                                           
                                                                       
                                                   *ap)
{
	struct nfsmount *nmp = VTONMP(ap->a_vp);
	nfsnode_t np = ap->a_vp ? VTONFS(ap->a_vp) : NULL;
	nfsnode_t anp = ap->a_svp ? VTONFS(ap->a_svp) : NULL;

	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}

	  
                                                    
                                               
    
	if (!(nmp->nm_fsattr.nfsa_flags & NFS_FSFLAG_NAMED_ATTR)) {
		return ENOTSUP;
	}

	return nfs4_named_attr_remove(np, anp, ap->a_name, ap->a_context);
}

#endif
#endif                  

#endif                        
