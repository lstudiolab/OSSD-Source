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
 *	@(#)nfs_syscalls.c	8.5 (Berkeley) 3/30/95
 * FreeBSD-Id: nfs_syscalls.c,v 1.32 1997/11/07 08:53:25 phk Exp $
 */

#include <nfs/nfs_conf.h>

  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file_internal.h>
#include <sys/filedesc.h>
#include <sys/stat.h>
#include <sys/vnode_internal.h>
#include <sys/mount_internal.h>
#include <sys/proc_internal.h>                  
#include <sys/kauth.h>
#include <sys/sysctl.h>
#include <sys/ubc.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/kpi_mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/fcntl.h>
#include <sys/lockf.h>
#include <sys/syslog.h>
#include <sys/user.h>
#include <sys/sysproto.h>
#include <sys/kpi_socket.h>
#include <sys/fsevents.h>
#include <libkern/OSAtomic.h>
#include <kern/thread_call.h>
#include <kern/task.h>

#include <security/audit/audit.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <nfs/xdr_subs.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsm_subs.h>
#include <nfs/nfsrvcache.h>
#include <nfs/nfs_gss.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsnode.h>
#include <nfs/nfs_lock.h>
#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

kern_return_t   thread_terminate(thread_t);          

#if CONFIG_NFS_SERVER

extern const nfsrv_proc_t nfsrv_procs[NFS_NPROCS];

extern int nfsrv_wg_delay;
extern int nfsrv_wg_delay_v3;

static int nfsrv_require_resv_port = 0;
static time_t  nfsrv_idlesock_timer_on = 0;
static int nfsrv_sock_tcp_cnt = 0;
#define NFSD_MIN_IDLE_TIMEOUT 30
static int nfsrv_sock_idle_timeout = 3600;               

int     nfssvc_export(user_addr_t argp);
int     nfssvc_nfsd(void);
int     nfssvc_addsock(socket_t, mbuf_t);
void    nfsrv_zapsock(struct nfsrv_sock *);
void    nfsrv_slpderef(struct nfsrv_sock *);
void    nfsrv_slpfree(struct nfsrv_sock *);

#endif                        

#if CONFIG_NFS
  
               
   
SYSCTL_DECL(_vfs_generic);
SYSCTL_NODE(_vfs_generic, OID_AUTO, nfs, CTLFLAG_RW | CTLFLAG_LOCKED, 0, "nfs hinge");
#endif                 

#if CONFIG_NFS_CLIENT
SYSCTL_NODE(_vfs_generic_nfs, OID_AUTO, client, CTLFLAG_RW | CTLFLAG_LOCKED, 0, "nfs client hinge");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, initialdowndelay, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_tprintf_initial_delay, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, nextdowndelay, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_tprintf_delay, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, iosize, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_iosize, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, access_cache_timeout, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_access_cache_timeout, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, allow_async, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_allow_async, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, statfs_rate_limit, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_statfs_rate_limit, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, nfsiod_thread_max, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsiod_thread_max, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, nfsiod_thread_count, CTLFLAG_RD | CTLFLAG_LOCKED, &nfsiod_thread_count, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, lockd_mounts, CTLFLAG_RD | CTLFLAG_LOCKED, &nfs_lockd_mounts, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, max_async_writes, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_max_async_writes, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, access_delete, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_access_delete, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, access_dotzfs, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_access_dotzfs, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, access_for_getattr, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_access_for_getattr, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, idmap_ctrl, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_idmap_ctrl, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, callback_port, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_callback_port, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, is_mobile, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_is_mobile, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, squishy_flags, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_squishy_flags, 0, "");
SYSCTL_UINT(_vfs_generic_nfs_client, OID_AUTO, tcp_sockbuf, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_tcp_sockbuf, 0, "");
SYSCTL_UINT(_vfs_generic_nfs_client, OID_AUTO, debug_ctl, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_debug_ctl, 0, "");
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, readlink_nocache, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_readlink_nocache, 0, "");
#if CONFIG_NFS_GSS
SYSCTL_INT(_vfs_generic_nfs_client, OID_AUTO, root_steals_gss_context, CTLFLAG_RW | CTLFLAG_LOCKED, &nfs_root_steals_ctx, 0, "");
#endif
#if CONFIG_NFS4
SYSCTL_STRING(_vfs_generic_nfs_client, OID_AUTO, default_nfs4domain, CTLFLAG_RW | CTLFLAG_LOCKED, nfs4_default_domain, sizeof(nfs4_default_domain), "");
#endif
#endif                        

#if CONFIG_NFS_SERVER
SYSCTL_NODE(_vfs_generic_nfs, OID_AUTO, server, CTLFLAG_RW | CTLFLAG_LOCKED, 0, "nfs server hinge");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, wg_delay, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_wg_delay, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, wg_delay_v3, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_wg_delay_v3, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, require_resv_port, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_require_resv_port, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, async, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_async, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, export_hash_size, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_export_hash_size, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, reqcache_size, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_reqcache_size, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, request_queue_length, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_sock_max_rec_queue_length, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, user_stats, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_user_stat_enabled, 0, "");
SYSCTL_UINT(_vfs_generic_nfs_server, OID_AUTO, gss_context_ttl, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_gss_context_ttl, 0, "");
#if CONFIG_FSE
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, fsevents, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_fsevents_enabled, 0, "");
#endif
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, nfsd_thread_max, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsd_thread_max, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, nfsd_thread_count, CTLFLAG_RD | CTLFLAG_LOCKED, &nfsd_thread_count, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, nfsd_sock_idle_timeout, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_sock_idle_timeout, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, nfsd_tcp_connections, CTLFLAG_RD | CTLFLAG_LOCKED, &nfsrv_sock_tcp_cnt, 0, "");
#ifdef NFS_UC_Q_DEBUG
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, use_upcall_svc, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_uc_use_proxy, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, upcall_queue_limit, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_uc_queue_limit, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, upcall_queue_max_seen, CTLFLAG_RW | CTLFLAG_LOCKED, &nfsrv_uc_queue_max_seen, 0, "");
SYSCTL_INT(_vfs_generic_nfs_server, OID_AUTO, upcall_queue_count, CTLFLAG_RD | CTLFLAG_LOCKED, __DECONST(int *, &nfsrv_uc_queue_count), 0, "");
#endif
#endif                        

#if CONFIG_NFS_CLIENT && CONFIG_NFS4
static int
mapname2id(struct nfs_testmapid *map)
{
	int error;
	error = nfs4_id2guid(map->ntm_name, &map->ntm_guid, map->ntm_grpflag);
	if (error) {
		return error;
	}

	if (map->ntm_grpflag) {
		error = kauth_cred_guid2gid(&map->ntm_guid, (gid_t *)&map->ntm_id);
	} else {
		error = kauth_cred_guid2uid(&map->ntm_guid, (uid_t *)&map->ntm_id);
	}

	return error;
}

static int
mapid2name(struct nfs_testmapid *map)
{
	int error;
	size_t len = sizeof(map->ntm_name);

	if (map->ntm_grpflag) {
		error = kauth_cred_gid2guid((gid_t)map->ntm_id, &map->ntm_guid);
	} else {
		error = kauth_cred_uid2guid((uid_t)map->ntm_id, &map->ntm_guid);
	}

	if (error) {
		return error;
	}

	error = nfs4_guid2id(&map->ntm_guid, map->ntm_name, &len, map->ntm_grpflag);

	return error;
}

static int
nfsclnt_testidmap(proc_t p, user_addr_t argp)
{
	struct nfs_testmapid mapid;
	int error, coerror;
	size_t len = sizeof(mapid.ntm_name);

	                              
	error = proc_suser(p);
	if (error) {
		return error;
	}

	error = copyin(argp, &mapid, sizeof(mapid));
	mapid.ntm_name[MAXIDNAMELEN - 1] = '\0';

	if (error) {
		return error;
	}
	switch (mapid.ntm_lookup) {
	case NTM_NAME2ID:
		error = mapname2id(&mapid);
		break;
	case NTM_ID2NAME:
		error = mapid2name(&mapid);
		break;
	case NTM_NAME2GUID:
		error = nfs4_id2guid(mapid.ntm_name, &mapid.ntm_guid, mapid.ntm_grpflag);
		break;
	case NTM_GUID2NAME:
		error = nfs4_guid2id(&mapid.ntm_guid, mapid.ntm_name, &len, mapid.ntm_grpflag);
		break;
	default:
		return EINVAL;
	}

	coerror = copyout(&mapid, argp, sizeof(mapid));

	return error ? error : coerror;
}
#endif                                       

#if !CONFIG_NFS_CLIENT
#define __no_nfs_client_unused      __unused
#else
#define __no_nfs_client_unused                   
#endif

int
nfsclnt(
	proc_t p __no_nfs_client_unused,
	struct nfsclnt_args *uap __no_nfs_client_unused,
	__unused int *retval)
{
#if CONFIG_NFS_CLIENT
	struct lockd_ans la;
	int error;

	switch (uap->flag) {
	case NFSCLNT_LOCKDANS:
		error = copyin(uap->argp, &la, sizeof(la));
		if (!error) {
			error = nfslockdans(p, &la);
		}
		break;
	case NFSCLNT_LOCKDNOTIFY:
		error = nfslockdnotify(p, uap->argp);
		break;
#if CONFIG_NFS4
	case NFSCLNT_TESTIDMAP:
		error = nfsclnt_testidmap(p, uap->argp);
		break;
#endif
	default:
		error = EINVAL;
	}
	return error;
#else
	return ENOSYS;
#endif                        
}

#if CONFIG_NFS_CLIENT

  
                                           
                                                                         
  
                                                                             
                                                                          
                                                                               
                                                                               
                                                                              
                                                                   
                                                                                  
                                                                                
   

  
                             
  
                                                    
                                                        
   
void
nfsiod_terminate(struct nfsiod *niod)
{
	nfsiod_thread_count--;
	lck_mtx_unlock(&nfsiod_mutex);
	if (niod) {
		FREE(niod, M_TEMP);
	} else {
		printf("nfsiod: terminating without niod\n");
	}
	thread_terminate(current_thread());
	              
}

                                   
void
nfsiod_thread(void)
{
	struct nfsiod *niod;
	int error;

	MALLOC(niod, struct nfsiod *, sizeof(struct nfsiod), M_TEMP, M_WAITOK);
	if (!niod) {
		lck_mtx_lock(&nfsiod_mutex);
		nfsiod_thread_count--;
		wakeup(current_thread());
		lck_mtx_unlock(&nfsiod_mutex);
		thread_terminate(current_thread());
		              
	}
	bzero(niod, sizeof(*niod));
	lck_mtx_lock(&nfsiod_mutex);
	TAILQ_INSERT_HEAD(&nfsiodfree, niod, niod_link);
	wakeup(current_thread());
	error = msleep0(niod, &nfsiod_mutex, PWAIT | PDROP, "nfsiod", NFS_ASYNCTHREADMAXIDLE * hz, nfsiod_continue);
	                                             
	                                               
	lck_mtx_lock(&nfsiod_mutex);
	if ((niod = TAILQ_LAST(&nfsiodfree, nfsiodlist))) {
		TAILQ_REMOVE(&nfsiodfree, niod, niod_link);
	}
	nfsiod_terminate(niod);
	              
}

  
                                  
                                                                 
   
int
nfsiod_start(void)
{
	thread_t thd = THREAD_NULL;

	lck_mtx_lock(&nfsiod_mutex);
	if ((nfsiod_thread_count >= NFSIOD_MAX) && (nfsiod_thread_count > 0)) {
		lck_mtx_unlock(&nfsiod_mutex);
		return EBUSY;
	}
	nfsiod_thread_count++;
	if (kernel_thread_start((thread_continue_t)nfsiod_thread, NULL, &thd) != KERN_SUCCESS) {
		lck_mtx_unlock(&nfsiod_mutex);
		return EBUSY;
	}
	                                             
	msleep(thd, &nfsiod_mutex, PWAIT | PDROP, "nfsiodw", NULL);
	thread_deallocate(thd);
	return 0;
}

  
                                                            
  
                                                               
   
int
nfsiod_continue(int error)
{
	struct nfsiod *niod;
	struct nfsmount *nmp;
	struct nfsreq *req, *treq;
	struct nfs_reqqhead iodq;
	int morework;

	lck_mtx_lock(&nfsiod_mutex);
	niod = TAILQ_FIRST(&nfsiodwork);
	if (!niod) {
		                               
		                                               
		if ((niod = TAILQ_LAST(&nfsiodfree, nfsiodlist))) {
			TAILQ_REMOVE(&nfsiodfree, niod, niod_link);
		}
		nfsiod_terminate(niod);
		              
	}
	TAILQ_REMOVE(&nfsiodwork, niod, niod_link);

worktodo:
	while ((nmp = niod->niod_nmp)) {
		if (nmp == NULL) {
			niod->niod_nmp = NULL;
			break;
		}

		  
                                          
    
                                                              
                                                              
                                                           
                                                                
                                                                
                                                
     
		                                            
		TAILQ_INIT(&iodq);
		TAILQ_CONCAT(&iodq, &nmp->nm_iodq, r_achain);
		                                                      
		TAILQ_FOREACH(req, &iodq, r_achain) {
			lck_mtx_lock(&req->r_mtx);
			assert(!(req->r_flags & R_IOD));
			req->r_flags |= R_IOD;
			lck_mtx_unlock(&req->r_mtx);
		}
		lck_mtx_unlock(&nfsiod_mutex);

		                       
		TAILQ_FOREACH_SAFE(req, &iodq, r_achain, treq) {
			TAILQ_REMOVE(&iodq, req, r_achain);
			req->r_achain.tqe_next = NFSREQNOLIST;
			req->r_callback.rcb_func(req);
		}

		                                                     
		lck_mtx_lock(&nfsiod_mutex);
		morework = !TAILQ_EMPTY(&nmp->nm_iodq);
		if (!morework || !TAILQ_EMPTY(&nfsiodmounts)) {
			  
                                                          
                                                
      
			if (morework && nmp->nm_iodlink.tqe_next == NFSNOLIST) {
				TAILQ_INSERT_TAIL(&nfsiodmounts, nmp, nm_iodlink);
			}
			nmp->nm_niod = NULL;
			niod->niod_nmp = NULL;
		}
	}

	                                              
	if (!niod->niod_nmp && !TAILQ_EMPTY(&nfsiodmounts)) {
		niod->niod_nmp = TAILQ_FIRST(&nfsiodmounts);
		TAILQ_REMOVE(&nfsiodmounts, niod->niod_nmp, nm_iodlink);
		niod->niod_nmp->nm_iodlink.tqe_next = NFSNOLIST;
	}
	if (niod->niod_nmp) {
		goto worktodo;
	}

	                                                                        
	if (nfsiod_thread_count <= NFSIOD_MAX) {
		TAILQ_INSERT_HEAD(&nfsiodfree, niod, niod_link);
		error = msleep0(niod, &nfsiod_mutex, PWAIT | PDROP, "nfsiod", NFS_ASYNCTHREADMAXIDLE * hz, nfsiod_continue);
		                                             
		                                               
		lck_mtx_lock(&nfsiod_mutex);
		if ((niod = TAILQ_LAST(&nfsiodfree, nfsiodlist))) {
			TAILQ_REMOVE(&nfsiodfree, niod, niod_link);
		}
	}
	nfsiod_terminate(niod);
	              
	return 0;
}

#endif                        

#if !CONFIG_NFS_SERVER
#define __no_nfs_server_unused      __unused
#else
#define __no_nfs_server_unused                   
#endif

  
                          
                                                                       
   

#if CONFIG_NFS_SERVER
static struct nfs_exportfs *
nfsrv_find_exportfs(const char *ptr)
{
	struct nfs_exportfs *nxfs;

	LIST_FOREACH(nxfs, &nfsrv_exports, nxfs_next) {
		if (!strncmp(nxfs->nxfs_path, ptr, MAXPATHLEN)) {
			break;
		}
	}
	if (nxfs && strncmp(nxfs->nxfs_path, ptr, strlen(nxfs->nxfs_path))) {
		nxfs = NULL;
	}

	return nxfs;
}

  
                              
   
int
getfh(
	proc_t p __no_nfs_server_unused,
	struct getfh_args *uap __no_nfs_server_unused,
	__unused int *retval)
{
	vnode_t vp;
	struct nfs_filehandle nfh;
	int error, fhlen = 0, fidlen;
	struct nameidata nd;
	char path[MAXPATHLEN], real_mntonname[MAXPATHLEN], *ptr;
	size_t pathlen;
	struct nfs_exportfs *nxfs;
	struct nfs_export *nx;

	  
                      
    
	error = proc_suser(p);
	if (error) {
		return error;
	}

	error = copyinstr(uap->fname, path, MAXPATHLEN, &pathlen);
	if (!error) {
		error = copyin(uap->fhp, &fhlen, sizeof(fhlen));
	}
	if (error) {
		return error;
	}
	                                                               
	if ((fhlen != NFSV2_MAX_FH_SIZE) && (fhlen != NFSV3_MAX_FH_SIZE)) {
		fhlen = NFSV3_MAX_FH_SIZE;
	}
	fidlen = fhlen - sizeof(struct nfs_exphandle);

	if (!nfsrv_is_initialized()) {
		return EINVAL;
	}

	NDINIT(&nd, LOOKUP, OP_LOOKUP, FOLLOW | LOCKLEAF | AUDITVNPATH1,
	    UIO_SYSSPACE, CAST_USER_ADDR_T(path), vfs_context_current());
	error = namei(&nd);
	if (error) {
		return error;
	}
	nameidone(&nd);

	vp = nd.ni_vp;

	                                         
	lck_rw_lock_shared(&nfsrv_export_rwlock);
	ptr = vnode_mount(vp)->mnt_vfsstat.f_mntonname;
	if ((nxfs = nfsrv_find_exportfs(ptr)) == NULL) {
		  
                                                       
                                           
     
		int pathbuflen = MAXPATHLEN;
		vnode_t rvp;

		error = VFS_ROOT(vnode_mount(vp), &rvp, vfs_context_current());
		if (error) {
			goto out;
		}
		error = vn_getpath_ext(rvp, NULLVP, real_mntonname, &pathbuflen,
		    VN_GETPATH_FSENTER | VN_GETPATH_NO_FIRMLINK);
		vnode_put(rvp);
		if (error) {
			goto out;
		}
		ptr = real_mntonname;
		nxfs = nfsrv_find_exportfs(ptr);
	}
	if (nxfs == NULL) {
		error = EINVAL;
		goto out;
	}
	                                                  
	ptr = path + strlen(nxfs->nxfs_path);
	while (*ptr && (*ptr == '/')) {
		ptr++;
	}
	LIST_FOREACH(nx, &nxfs->nxfs_exports, nx_next) {
		size_t len = strlen(nx->nx_path);
		if (len == 0) {                                                     
			break;
		}
		if (!strncmp(nx->nx_path, ptr, len)) {
			break;
		}
	}
	if (!nx) {
		error = EINVAL;
		goto out;
	}

	bzero(&nfh, sizeof(nfh));
	nfh.nfh_xh.nxh_version = htonl(NFS_FH_VERSION);
	nfh.nfh_xh.nxh_fsid = htonl(nxfs->nxfs_id);
	nfh.nfh_xh.nxh_expid = htonl(nx->nx_id);
	nfh.nfh_xh.nxh_flags = 0;
	nfh.nfh_xh.nxh_reserved = 0;
	nfh.nfh_len = fidlen;
	error = VFS_VPTOFH(vp, (int*)&nfh.nfh_len, &nfh.nfh_fid[0], NULL);
	if (nfh.nfh_len > (uint32_t)fidlen) {
		error = EOVERFLOW;
	}
	nfh.nfh_xh.nxh_fidlen = nfh.nfh_len;
	nfh.nfh_len += sizeof(nfh.nfh_xh);
	nfh.nfh_fhp = (u_char*)&nfh.nfh_xh;

out:
	lck_rw_done(&nfsrv_export_rwlock);
	vnode_put(vp);
	if (error) {
		return error;
	}
	  
                                                          
                                                         
                                      
    
	error = copyout((caddr_t)&nfh, uap->fhp, sizeof(fhandle_t));
	return error;
}
#endif                        

#if CONFIG_NFS_SERVER
extern const struct fileops vnops;

  
                                                                       
                      
  
                                                                    
                 
   
int
fhopen(proc_t p __no_nfs_server_unused,
    struct fhopen_args *uap __no_nfs_server_unused,
    int32_t *retval __no_nfs_server_unused)
{
	vnode_t vp;
	struct nfs_filehandle nfh;
	struct nfs_export *nx;
	struct nfs_export_options *nxo;
	struct flock lf;
	struct fileproc *fp, *nfp;
	int fmode, error, type;
	int indx;
	vfs_context_t ctx = vfs_context_current();
	kauth_action_t action;

	  
                      
    
	error = suser(vfs_context_ucred(ctx), 0);
	if (error) {
		return error;
	}

	if (!nfsrv_is_initialized()) {
		return EINVAL;
	}

	fmode = FFLAGS(uap->flags);
	                                                        
	if (((fmode & (FREAD | FWRITE)) == 0) || (fmode & O_CREAT)) {
		return EINVAL;
	}

	error = copyin(uap->u_fhp, &nfh.nfh_len, sizeof(nfh.nfh_len));
	if (error) {
		return error;
	}
	if ((nfh.nfh_len < (int)sizeof(struct nfs_exphandle)) ||
	    (nfh.nfh_len > (int)NFSV3_MAX_FH_SIZE)) {
		return EINVAL;
	}
	error = copyin(uap->u_fhp, &nfh, sizeof(nfh.nfh_len) + nfh.nfh_len);
	if (error) {
		return error;
	}
	nfh.nfh_fhp = (u_char*)&nfh.nfh_xh;

	lck_rw_lock_shared(&nfsrv_export_rwlock);
	                                                                   
	error = nfsrv_fhtovp(&nfh, NULL, &vp, &nx, &nxo);
	lck_rw_done(&nfsrv_export_rwlock);
	if (error) {
		if (error == NFSERR_TRYLATER) {
			error = EAGAIN;                                         
		}
		return error;
	}

	  
                                        
                              
                                                      
                                         
    

	  
                
    
	if (vnode_vtype(vp) == VSOCK) {
		error = EOPNOTSUPP;
		goto bad;
	}

	                                              
	if (vnode_isdir(vp) && (fmode & (FWRITE | O_TRUNC))) {
		error = EISDIR;
		goto bad;
	}

#if CONFIG_MACF
	if ((error = mac_vnode_check_open(ctx, vp, fmode))) {
		goto bad;
	}
#endif

	                                     
	action = 0;
	if (fmode & FREAD) {
		action |= KAUTH_VNODE_READ_DATA;
	}
	if (fmode & (FWRITE | O_TRUNC)) {
		action |= KAUTH_VNODE_WRITE_DATA;
	}
	if ((error = vnode_authorize(vp, NULL, action, ctx)) != 0) {
		goto bad;
	}

	if ((error = VNOP_OPEN(vp, fmode, ctx))) {
		goto bad;
	}
	if ((error = vnode_ref_ext(vp, fmode, 0))) {
		goto bad;
	}

	  
                       
    

	                                                              
	if ((error = falloc(p, &nfp, &indx, ctx)) != 0) {
		vn_close(vp, fmode & FMASK, ctx);
		goto bad;
	}
	fp = nfp;

	fp->fp_glob->fg_flag = fmode & FMASK;
	fp->fp_glob->fg_ops = &vnops;
	fp->fp_glob->fg_data = (caddr_t)vp;

	                                                       
	if (fmode & (O_EXLOCK | O_SHLOCK)) {
		lf.l_whence = SEEK_SET;
		lf.l_start = 0;
		lf.l_len = 0;
		if (fmode & O_EXLOCK) {
			lf.l_type = F_WRLCK;
		} else {
			lf.l_type = F_RDLCK;
		}
		type = F_FLOCK;
		if ((fmode & FNONBLOCK) == 0) {
			type |= F_WAIT;
		}
		if ((error = VNOP_ADVLOCK(vp, (caddr_t)fp->fp_glob, F_SETLK, &lf, type, ctx, NULL))) {
			struct vfs_context context = *vfs_context_current();
			                                                   
			context.vc_ucred = fp->fp_glob->fg_cred;

			vn_close(vp, fp->fp_glob->fg_flag, &context);
			fp_free(p, indx, fp);
			goto bad;
		}
		fp->fp_glob->fg_flag |= FWASLOCKED;
	}

	vnode_put(vp);

	proc_fdlock(p);
	procfdtbl_releasefd(p, indx, NULL);
	fp_drop(p, indx, fp, 1);
	proc_fdunlock(p);

	*retval = indx;
	return 0;

bad:
	vnode_put(vp);
	return error;
}
#endif                        

#if CONFIG_NFS_SERVER
  
                                
   
int
nfssvc(proc_t p __no_nfs_server_unused,
    struct nfssvc_args *uap __no_nfs_server_unused,
    __unused int *retval)
{
	mbuf_t nam;
	struct user_nfsd_args user_nfsdarg;
	socket_t so;
	int error;

	AUDIT_ARG(cmd, uap->flag);

	  
                                                                      
    
	if ((uap->flag != NFSSVC_EXPORT) && ((error = proc_suser(p)))) {
		return error;
	}
#if CONFIG_MACF
	error = mac_system_check_nfsd(kauth_cred_get());
	if (error) {
		return error;
	}
#endif

	                                                                
	nfsrv_init();

	if (uap->flag & NFSSVC_ADDSOCK) {
		if (IS_64BIT_PROCESS(p)) {
			error = copyin(uap->argp, (caddr_t)&user_nfsdarg, sizeof(user_nfsdarg));
		} else {
			struct nfsd_args    tmp_args;
			error = copyin(uap->argp, (caddr_t)&tmp_args, sizeof(tmp_args));
			if (error == 0) {
				user_nfsdarg.sock = tmp_args.sock;
				user_nfsdarg.name = CAST_USER_ADDR_T(tmp_args.name);
				user_nfsdarg.namelen = tmp_args.namelen;
			}
		}
		if (error) {
			return error;
		}
		                    
		error = file_socket(user_nfsdarg.sock, &so);
		if (error) {
			return error;
		}
		                                                   
		if (user_nfsdarg.name == USER_ADDR_NULL || user_nfsdarg.namelen == 0) {
			nam = NULL;
		} else {
			error = sockargs(&nam, user_nfsdarg.name, user_nfsdarg.namelen, MBUF_TYPE_SONAME);
			if (error) {
				                                                                   
				file_drop(user_nfsdarg.sock);
				return error;
			}
		}
		  
                                                            
                                                              
                            
     
		error = nfssvc_addsock(so, nam);
		                                                                   
		file_drop(user_nfsdarg.sock);
	} else if (uap->flag & NFSSVC_NFSD) {
		error = nfssvc_nfsd();
	} else if (uap->flag & NFSSVC_EXPORT) {
		error = nfssvc_export(uap->argp);
	} else {
		error = EINVAL;
	}
	if (error == EINTR || error == ERESTART) {
		error = 0;
	}
	return error;
}
#endif                        

#if CONFIG_NFS_SERVER

  
                                                    
   
int
nfssvc_addsock(socket_t so, mbuf_t mynam)
{
	struct nfsrv_sock *slp;
	int error = 0, sodomain, sotype, soprotocol, on = 1;
	int first, sobufsize;
	struct timeval timeo;
	u_quad_t sbmaxsize;

	                                         
	if (!nfs_mbuf_mhlen) {
		nfs_mbuf_init();
	}

	sock_gettype(so, &sodomain, &sotype, &soprotocol);

	                                                                   
	if ((sodomain == AF_INET) && (soprotocol == IPPROTO_UDP) && nfsrv_udpsock) {
		mbuf_freem(mynam);
		return EEXIST;
	}
	if ((sodomain == AF_INET6) && (soprotocol == IPPROTO_UDP) && nfsrv_udp6sock) {
		mbuf_freem(mynam);
		return EEXIST;
	}

	                                                            
	if (sotype == SOCK_STREAM) {
		error = nfsrv_check_exports_allow_address(mynam);
		if (error) {
			log(LOG_INFO, "nfsvc_addsock:: nfsrv_check_exports_allow_address(myname) returned %d\n", error);
			mbuf_freem(mynam);
			return error;
		}
		sock_setsockopt(so, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
	}
	if ((sodomain == AF_INET) && (soprotocol == IPPROTO_TCP)) {
		sock_setsockopt(so, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
	}

	                                                      
	sbmaxsize = (u_quad_t)sb_max * MCLBYTES / (MSIZE + MCLBYTES);

	                                         
	sobufsize = min(sbmaxsize, (sotype == SOCK_DGRAM) ? NFS_UDPSOCKBUF : NFSRV_TCPSOCKBUF);
	error |= sock_setsockopt(so, SOL_SOCKET, SO_SNDBUF, &sobufsize, sizeof(sobufsize));
	error |= sock_setsockopt(so, SOL_SOCKET, SO_RCVBUF, &sobufsize, sizeof(sobufsize));

	if (error) {
		log(LOG_INFO, "nfssvc_addsock: socket buffer setting error(s) %d\n", error);
		error = 0;
	}
	sock_nointerrupt(so, 0);

	  
                                     
                                                                  
                                                                     
    
	timeo.tv_usec = 0;
	timeo.tv_sec = 1;
	error |= sock_setsockopt(so, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
	timeo.tv_sec = 30;
	error |= sock_setsockopt(so, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
	if (error) {
		log(LOG_INFO, "nfssvc_addsock: socket timeout setting error(s) %d\n", error);
		error = 0;
	}

	MALLOC(slp, struct nfsrv_sock *, sizeof(struct nfsrv_sock), M_NFSSVC, M_WAITOK);
	if (!slp) {
		mbuf_freem(mynam);
		return ENOMEM;
	}
	bzero((caddr_t)slp, sizeof(struct nfsrv_sock));
	lck_rw_init(&slp->ns_rwlock, &nfsrv_slp_rwlock_group, LCK_ATTR_NULL);
	lck_mtx_init(&slp->ns_wgmutex, &nfsrv_slp_mutex_group, LCK_ATTR_NULL);

	lck_mtx_lock(&nfsd_mutex);

	if (soprotocol == IPPROTO_UDP) {
		if (sodomain == AF_INET) {
			                                              
			if (nfsrv_udpsock) {
				lck_mtx_unlock(&nfsd_mutex);
				nfsrv_slpfree(slp);
				mbuf_freem(mynam);
				return EEXIST;
			}
			nfsrv_udpsock = slp;
		}
		if (sodomain == AF_INET6) {
			                                              
			if (nfsrv_udp6sock) {
				lck_mtx_unlock(&nfsd_mutex);
				nfsrv_slpfree(slp);
				mbuf_freem(mynam);
				return EEXIST;
			}
			nfsrv_udp6sock = slp;
		}
	}

	                                
	first = TAILQ_EMPTY(&nfsrv_socklist);
	TAILQ_INSERT_TAIL(&nfsrv_socklist, slp, ns_chain);
	if (sotype == SOCK_STREAM) {
		nfsrv_sock_tcp_cnt++;
		if (nfsrv_sock_idle_timeout < 0) {
			nfsrv_sock_idle_timeout = 0;
		}
		if (nfsrv_sock_idle_timeout && (nfsrv_sock_idle_timeout < NFSD_MIN_IDLE_TIMEOUT)) {
			nfsrv_sock_idle_timeout = NFSD_MIN_IDLE_TIMEOUT;
		}
		  
                                                                             
                                                                            
                                                                               
                               
     
		if ((nfsrv_sock_tcp_cnt > 2 * nfsd_thread_max) || nfsrv_idlesock_timer_on) {
			if (nfsrv_sock_idle_timeout == 0 || nfsrv_sock_tcp_cnt <= 2 * nfsd_thread_max) {
				if (nfsrv_idlesock_timer_on) {
					thread_call_cancel(nfsrv_idlesock_timer_call);
					nfsrv_idlesock_timer_on = 0;
				}
			} else {
				struct nfsrv_sock *old_slp;
				struct timeval now;
				time_t time_to_wait = nfsrv_sock_idle_timeout;
				  
                                                  
                                                    
                                                            
       
				TAILQ_FOREACH(old_slp, &nfsrv_socklist, ns_chain) {
					if (old_slp->ns_sotype == SOCK_STREAM) {
						microuptime(&now);
						time_to_wait -= now.tv_sec - old_slp->ns_timestamp;
						if (time_to_wait < 1) {
							time_to_wait = 1;
						}
						break;
					}
				}
				  
                                                                       
                   
       
				if (nfsrv_idlesock_timer_on > now.tv_sec + time_to_wait) {
					thread_call_cancel(nfsrv_idlesock_timer_call);
					nfsrv_idlesock_timer_on = 0;
				}
				                                                  
				if (!nfsrv_idlesock_timer_on) {
					nfs_interval_timer_start(nfsrv_idlesock_timer_call, time_to_wait * 1000);
					nfsrv_idlesock_timer_on = now.tv_sec + time_to_wait;
				}
			}
		}
	}

	sock_retain(so);                                        
	slp->ns_so = so;
	slp->ns_sotype = sotype;
	slp->ns_nam = mynam;
	slp->ns_sobufsize = sobufsize;

	                               
	nfsrv_uc_addsock(slp, first);

	                                                          
	slp->ns_wgq.tqe_next = SLPNOLIST;

	slp->ns_flag = SLP_VALID | SLP_NEEDQ;

	nfsrv_wakenfsd(slp);
	lck_mtx_unlock(&nfsd_mutex);

	return 0;
}

  
                
  
                            
  
                                                                         
                                                                             
                                                                      
                                     
  
                                              
                                                                
                                         
  
                                                                        
                                      
                                                                
                                                                            
                                                                 
                                                                 
                                                   
                                                                  
                                              
  
                                                                    
                                         
                                                                             
                                                                         
                                 
                                                                          
                                                                             
                                                                        
                                                     
  
   
int
nfssvc_nfsd(void)
{
	mbuf_t m, mrep = NULL;
	struct nfsrv_sock *slp;
	struct nfsd *nfsd;
	struct nfsrv_descript *nd = NULL;
	int error = 0, cacherep, writes_todo;
	int siz, procrastinate, opcnt = 0;
	time_t cur_usec;
	struct timeval now;
	struct vfs_context context;
	struct timespec to;

#ifndef nolint
	cacherep = RC_DOIT;
	writes_todo = 0;
#endif

	MALLOC(nfsd, struct nfsd *, sizeof(struct nfsd), M_NFSD, M_WAITOK);
	if (!nfsd) {
		return ENOMEM;
	}
	bzero(nfsd, sizeof(struct nfsd));
	lck_mtx_lock(&nfsd_mutex);
	if (nfsd_thread_count++ == 0) {
		nfsrv_initcache();                                                 
	}
	TAILQ_INSERT_TAIL(&nfsd_head, nfsd, nfsd_chain);
	lck_mtx_unlock(&nfsd_mutex);

	context.vc_thread = current_thread();

	                                                                                   
	to.tv_sec = 5;
	to.tv_nsec = 0;

	  
                                            
    
	for (;;) {
		if (nfsd_thread_max <= 0) {
			                                            
			error = EINTR;
			slp = nfsd->nfsd_slp;
		} else if (nfsd->nfsd_flag & NFSD_REQINPROG) {
			                                  
			error = 0;
			slp = nfsd->nfsd_slp;
		} else {
			                             
			error = 0;
			lck_mtx_lock(&nfsd_mutex);
			while (!nfsd->nfsd_slp && TAILQ_EMPTY(&nfsrv_sockwait) && TAILQ_EMPTY(&nfsrv_sockwork)) {
				if (nfsd_thread_count > nfsd_thread_max) {
					  
                                               
                                                 
        
					error = 0;
					goto done;
				}
				nfsd->nfsd_flag |= NFSD_WAITING;
				TAILQ_INSERT_HEAD(&nfsd_queue, nfsd, nfsd_queue);
				error = msleep(nfsd, &nfsd_mutex, PSOCK | PCATCH, "nfsd", &to);
				if (error) {
					if (nfsd->nfsd_flag & NFSD_WAITING) {
						TAILQ_REMOVE(&nfsd_queue, nfsd, nfsd_queue);
						nfsd->nfsd_flag &= ~NFSD_WAITING;
					}
					if (error == EWOULDBLOCK) {
						continue;
					}
					goto done;
				}
			}
			slp = nfsd->nfsd_slp;
			if (!slp && !TAILQ_EMPTY(&nfsrv_sockwait)) {
				                                                    
				while ((slp = TAILQ_FIRST(&nfsrv_sockwait))) {
					lck_rw_lock_exclusive(&slp->ns_rwlock);
					                                       
					TAILQ_REMOVE(&nfsrv_sockwait, slp, ns_svcq);
					slp->ns_flag &= ~SLP_WAITQ;
					if ((slp->ns_flag & SLP_VALID) && (slp->ns_flag & SLP_WORKTODO)) {
						break;
					}
					                                        
					lck_rw_done(&slp->ns_rwlock);
				}
			}
			if (!slp && !TAILQ_EMPTY(&nfsrv_sockwork)) {
				                                                    
				while ((slp = TAILQ_FIRST(&nfsrv_sockwork))) {
					lck_rw_lock_exclusive(&slp->ns_rwlock);
					                                       
					TAILQ_REMOVE(&nfsrv_sockwork, slp, ns_svcq);
					slp->ns_flag &= ~SLP_WORKQ;
					if ((slp->ns_flag & SLP_VALID) && (slp->ns_flag & SLP_WORKTODO)) {
						break;
					}
					                                        
					lck_rw_done(&slp->ns_rwlock);
				}
			}
			if (!nfsd->nfsd_slp && slp) {
				                                                    
				slp->ns_sref++;
				microuptime(&now);
				slp->ns_timestamp = now.tv_sec;
				                                                                                   
				TAILQ_REMOVE(&nfsrv_socklist, slp, ns_chain);
				TAILQ_INSERT_TAIL(&nfsrv_socklist, slp, ns_chain);
				nfsd->nfsd_slp = slp;
				opcnt = 0;
				                                              
				TAILQ_INSERT_TAIL(&nfsrv_sockwork, slp, ns_svcq);
				slp->ns_flag |= SLP_WORKQ;
				lck_rw_done(&slp->ns_rwlock);
			}
			lck_mtx_unlock(&nfsd_mutex);
			if (!slp) {
				continue;
			}
			lck_rw_lock_exclusive(&slp->ns_rwlock);
			if (slp->ns_flag & SLP_VALID) {
				if ((slp->ns_flag & (SLP_NEEDQ | SLP_DISCONN)) == SLP_NEEDQ) {
					slp->ns_flag &= ~SLP_NEEDQ;
					nfsrv_rcv_locked(slp->ns_so, slp, MBUF_WAITOK);
				}
				if (slp->ns_flag & SLP_DISCONN) {
					nfsrv_zapsock(slp);
				}
				error = nfsrv_dorec(slp, nfsd, &nd);
				if (error == EINVAL) {                    
					if (slp->ns_sotype == SOCK_STREAM) {
						nfsrv_zapsock(slp);                   
					}
				}
				writes_todo = 0;
				if (error && (slp->ns_wgtime || (slp->ns_flag & SLP_DOWRITES))) {
					microuptime(&now);
					cur_usec = (now.tv_sec * 1000000) + now.tv_usec;
					if (slp->ns_wgtime <= cur_usec) {
						error = 0;
						cacherep = RC_DOIT;
						writes_todo = 1;
					}
					slp->ns_flag &= ~SLP_DOWRITES;
				}
				nfsd->nfsd_flag |= NFSD_REQINPROG;
			}
			lck_rw_done(&slp->ns_rwlock);
		}
		if (error || (slp && !(slp->ns_flag & SLP_VALID))) {
			if (nd) {
				nfsm_chain_cleanup(&nd->nd_nmreq);
				if (nd->nd_nam2) {
					mbuf_freem(nd->nd_nam2);
				}
				if (IS_VALID_CRED(nd->nd_cr)) {
					kauth_cred_unref(&nd->nd_cr);
				}
				if (nd->nd_gss_context) {
					nfs_gss_svc_ctx_deref(nd->nd_gss_context);
				}
				NFS_ZFREE(nfsrv_descript_zone, nd);
			}
			nfsd->nfsd_slp = NULL;
			nfsd->nfsd_flag &= ~NFSD_REQINPROG;
			if (slp) {
				nfsrv_slpderef(slp);
			}
			if (nfsd_thread_max <= 0) {
				break;
			}
			continue;
		}
		if (nd) {
			microuptime(&nd->nd_starttime);
			if (nd->nd_nam2) {
				nd->nd_nam = nd->nd_nam2;
			} else {
				nd->nd_nam = slp->ns_nam;
			}

			cacherep = nfsrv_getcache(nd, slp, &mrep);

			if (nfsrv_require_resv_port) {
				                                             
				in_port_t port = 0;
				struct sockaddr *saddr = mbuf_data(nd->nd_nam);

				if (saddr->sa_family == AF_INET) {
					port = ntohs(((struct sockaddr_in*)saddr)->sin_port);
				} else if (saddr->sa_family == AF_INET6) {
					port = ntohs(((struct sockaddr_in6*)saddr)->sin6_port);
				}
				if ((port >= IPPORT_RESERVED) && (nd->nd_procnum != NFSPROC_NULL)) {
					nd->nd_procnum = NFSPROC_NOOP;
					nd->nd_repstat = (NFSERR_AUTHERR | AUTH_TOOWEAK);
					cacherep = RC_DOIT;
				}
			}
		}

		  
                                                         
                       
     
		do {
			switch (cacherep) {
			case RC_DOIT:
				if (nd && (nd->nd_vers == NFS_VER3)) {
					procrastinate = nfsrv_wg_delay_v3;
				} else {
					procrastinate = nfsrv_wg_delay;
				}
				lck_rw_lock_shared(&nfsrv_export_rwlock);
				context.vc_ucred = NULL;
				if (writes_todo || ((nd->nd_procnum == NFSPROC_WRITE) && (procrastinate > 0))) {
					error = nfsrv_writegather(&nd, slp, &context, &mrep);
				} else {
					error = (*(nfsrv_procs[nd->nd_procnum]))(nd, slp, &context, &mrep);
				}
				lck_rw_done(&nfsrv_export_rwlock);
				if (mrep == NULL) {
					  
                                                       
                                                      
                                   
        
					if (error && slp->ns_sotype == SOCK_STREAM) {
						lck_rw_lock_exclusive(&slp->ns_rwlock);
						nfsrv_zapsock(slp);
						lck_rw_done(&slp->ns_rwlock);
						printf("NFS server: NULL reply from proc = %d error = %d\n",
						    nd->nd_procnum, error);
					}
					break;
				}
				if (error) {
					OSAddAtomic64(1, &nfsstats.srv_errs);
					nfsrv_updatecache(nd, FALSE, mrep);
					if (nd->nd_nam2) {
						mbuf_freem(nd->nd_nam2);
						nd->nd_nam2 = NULL;
					}
					break;
				}
				OSAddAtomic64(1, &nfsstats.srvrpccnt[nd->nd_procnum]);
				nfsrv_updatecache(nd, TRUE, mrep);
				OS_FALLTHROUGH;

			case RC_REPLY:
				if (nd->nd_gss_mb != NULL) {                   
					  
                                             
        
					error = nfs_gss_svc_protect_reply(nd, mrep);
					if (error) {
						mbuf_freem(mrep);
						break;
					}
				}

				  
                                      
       
				m = mrep;
				siz = 0;
				while (m) {
					siz += mbuf_len(m);
					m = mbuf_next(m);
				}
				if (siz <= 0 || siz > NFS_MAXPACKET) {
					printf("mbuf siz=%d\n", siz);
					panic("Bad nfs svc reply");
				}
				m = mrep;
				mbuf_pkthdr_setlen(m, siz);
				error = mbuf_pkthdr_setrcvif(m, NULL);
				if (error) {
					panic("nfsd setrcvif failed: %d", error);
				}
				  
                                              
                   
       
				if (slp->ns_sotype == SOCK_STREAM) {
					error = mbuf_prepend(&m, NFSX_UNSIGNED, MBUF_WAITOK);
					if (!error) {
						*(u_int32_t*)mbuf_data(m) = htonl(0x80000000 | siz);
					}
				}
				if (!error) {
					if (slp->ns_flag & SLP_VALID) {
						error = nfsrv_send(slp, nd->nd_nam2, m);
					} else {
						error = EPIPE;
						mbuf_freem(m);
					}
				} else {
					mbuf_freem(m);
				}
				mrep = NULL;
				if (nd->nd_nam2) {
					mbuf_freem(nd->nd_nam2);
					nd->nd_nam2 = NULL;
				}
				if (error == EPIPE) {
					lck_rw_lock_exclusive(&slp->ns_rwlock);
					nfsrv_zapsock(slp);
					lck_rw_done(&slp->ns_rwlock);
				}
				if (error == EINTR || error == ERESTART) {
					nfsm_chain_cleanup(&nd->nd_nmreq);
					if (IS_VALID_CRED(nd->nd_cr)) {
						kauth_cred_unref(&nd->nd_cr);
					}
					if (nd->nd_gss_context) {
						nfs_gss_svc_ctx_deref(nd->nd_gss_context);
					}
					NFS_ZFREE(nfsrv_descript_zone, nd);
					nfsrv_slpderef(slp);
					lck_mtx_lock(&nfsd_mutex);
					goto done;
				}
				break;
			case RC_DROPIT:
				mbuf_freem(nd->nd_nam2);
				nd->nd_nam2 = NULL;
				break;
			}
			;
			opcnt++;
			if (nd) {
				nfsm_chain_cleanup(&nd->nd_nmreq);
				if (nd->nd_nam2) {
					mbuf_freem(nd->nd_nam2);
				}
				if (IS_VALID_CRED(nd->nd_cr)) {
					kauth_cred_unref(&nd->nd_cr);
				}
				if (nd->nd_gss_context) {
					nfs_gss_svc_ctx_deref(nd->nd_gss_context);
				}
				NFS_ZFREE(nfsrv_descript_zone, nd);
			}

			  
                                                       
                          
      
			writes_todo = 0;
			if (slp->ns_wgtime) {
				microuptime(&now);
				cur_usec = (now.tv_sec * 1000000) + now.tv_usec;
				if (slp->ns_wgtime <= cur_usec) {
					cacherep = RC_DOIT;
					writes_todo = 1;
				}
			}
		} while (writes_todo);

		nd = NULL;
		if (TAILQ_EMPTY(&nfsrv_sockwait) && (opcnt < 8)) {
			lck_rw_lock_exclusive(&slp->ns_rwlock);
			error = nfsrv_dorec(slp, nfsd, &nd);
			if (error == EINVAL) {                    
				if (slp->ns_sotype == SOCK_STREAM) {
					nfsrv_zapsock(slp);                   
				}
			}
			lck_rw_done(&slp->ns_rwlock);
		}
		if (!nd) {
			                                      
			nfsd->nfsd_flag &= ~NFSD_REQINPROG;
			nfsd->nfsd_slp = NULL;
			nfsrv_slpderef(slp);
		}
	}
	lck_mtx_lock(&nfsd_mutex);
done:
	TAILQ_REMOVE(&nfsd_head, nfsd, nfsd_chain);
	FREE(nfsd, M_NFSD);
	if (--nfsd_thread_count == 0) {
		nfsrv_cleanup();
	}
	lck_mtx_unlock(&nfsd_mutex);
	return error;
}

int
nfssvc_export(user_addr_t argp)
{
	int error = 0, is_64bit;
	struct user_nfs_export_args unxa;
	vfs_context_t ctx = vfs_context_current();

	is_64bit = IS_64BIT_PROCESS(vfs_context_proc(ctx));

	                                              
	if (is_64bit) {
		error = copyin(argp, (caddr_t)&unxa, sizeof(unxa));
	} else {
		struct nfs_export_args tnxa;
		error = copyin(argp, (caddr_t)&tnxa, sizeof(tnxa));
		if (error == 0) {
			                                                          
			unxa.nxa_fsid = tnxa.nxa_fsid;
			unxa.nxa_expid = tnxa.nxa_expid;
			unxa.nxa_fspath = CAST_USER_ADDR_T(tnxa.nxa_fspath);
			unxa.nxa_exppath = CAST_USER_ADDR_T(tnxa.nxa_exppath);
			unxa.nxa_flags = tnxa.nxa_flags;
			unxa.nxa_netcount = tnxa.nxa_netcount;
			unxa.nxa_nets = CAST_USER_ADDR_T(tnxa.nxa_nets);
		}
	}
	if (error) {
		return error;
	}

	error = nfsrv_export(&unxa, ctx);

	return error;
}

  
                                                              
                                                        
                                                                          
                                                                         
                             
   
void
nfsrv_zapsock(struct nfsrv_sock *slp)
{
	socket_t so;

	if ((slp->ns_flag & SLP_VALID) == 0) {
		return;
	}
	slp->ns_flag &= ~SLP_ALLFLAGS;

	so = slp->ns_so;
	if (so == NULL) {
		return;
	}

	sock_setupcall(so, NULL, NULL);
	sock_shutdown(so, SHUT_RDWR);

	  
                                 
    
	nfsrv_uc_dequeue(slp);
}

  
                                                 
   
void
nfsrv_slpfree(struct nfsrv_sock *slp)
{
	struct nfsrv_descript *nwp, *nnwp;

	if (slp->ns_so) {
		sock_release(slp->ns_so);
		slp->ns_so = NULL;
	}
	if (slp->ns_nam) {
		mbuf_free(slp->ns_nam);
	}
	if (slp->ns_raw) {
		mbuf_freem(slp->ns_raw);
	}
	if (slp->ns_rec) {
		mbuf_freem(slp->ns_rec);
	}
	if (slp->ns_frag) {
		mbuf_freem(slp->ns_frag);
	}
	slp->ns_nam = slp->ns_raw = slp->ns_rec = slp->ns_frag = NULL;
	slp->ns_reccnt = 0;

	for (nwp = slp->ns_tq.lh_first; nwp; nwp = nnwp) {
		nnwp = nwp->nd_tq.le_next;
		LIST_REMOVE(nwp, nd_tq);
		nfsm_chain_cleanup(&nwp->nd_nmreq);
		if (nwp->nd_mrep) {
			mbuf_freem(nwp->nd_mrep);
		}
		if (nwp->nd_nam2) {
			mbuf_freem(nwp->nd_nam2);
		}
		if (IS_VALID_CRED(nwp->nd_cr)) {
			kauth_cred_unref(&nwp->nd_cr);
		}
		if (nwp->nd_gss_context) {
			nfs_gss_svc_ctx_deref(nwp->nd_gss_context);
		}
		NFS_ZFREE(nfsrv_descript_zone, nwp);
	}
	LIST_INIT(&slp->ns_tq);

	lck_rw_destroy(&slp->ns_rwlock, &nfsrv_slp_rwlock_group);
	lck_mtx_destroy(&slp->ns_wgmutex, &nfsrv_slp_mutex_group);
	FREE(slp, M_NFSSVC);
}

  
                                                                        
                                             
   
static void
nfsrv_slpderef_locked(struct nfsrv_sock *slp)
{
	lck_rw_lock_exclusive(&slp->ns_rwlock);
	slp->ns_sref--;

	if (slp->ns_sref || (slp->ns_flag & SLP_VALID)) {
		if ((slp->ns_flag & SLP_QUEUED) && !(slp->ns_flag & SLP_WORKTODO)) {
			                                                    
			if (slp->ns_flag & SLP_WAITQ) {
				TAILQ_REMOVE(&nfsrv_sockwait, slp, ns_svcq);
			} else {
				TAILQ_REMOVE(&nfsrv_sockwork, slp, ns_svcq);
			}
			slp->ns_flag &= ~SLP_QUEUED;
		}
		lck_rw_done(&slp->ns_rwlock);
		return;
	}

	                                                            

	if (slp->ns_flag & SLP_QUEUED) {
		if (slp->ns_flag & SLP_WAITQ) {
			TAILQ_REMOVE(&nfsrv_sockwait, slp, ns_svcq);
		} else {
			TAILQ_REMOVE(&nfsrv_sockwork, slp, ns_svcq);
		}
		slp->ns_flag &= ~SLP_QUEUED;
	}
	lck_rw_done(&slp->ns_rwlock);

	TAILQ_REMOVE(&nfsrv_socklist, slp, ns_chain);
	if (slp->ns_sotype == SOCK_STREAM) {
		nfsrv_sock_tcp_cnt--;
	}

	                                                  
	if (slp->ns_wgq.tqe_next != SLPNOLIST) {
		TAILQ_REMOVE(&nfsrv_sockwg, slp, ns_wgq);
		slp->ns_wgq.tqe_next = SLPNOLIST;
	}
	nfsrv_slpfree(slp);
}

void
nfsrv_slpderef(struct nfsrv_sock *slp)
{
	lck_mtx_lock(&nfsd_mutex);
	nfsrv_slpderef_locked(slp);
	lck_mtx_unlock(&nfsd_mutex);
}

  
                                                    
            
   
void
nfsrv_idlesock_timer(__unused void *param0, __unused void *param1)
{
	struct nfsrv_sock *slp, *tslp;
	struct timeval now;
	time_t time_to_wait = nfsrv_sock_idle_timeout;

	microuptime(&now);
	lck_mtx_lock(&nfsd_mutex);

	                                                        
	if (nfsrv_sock_idle_timeout < NFSD_MIN_IDLE_TIMEOUT) {
		nfsrv_sock_idle_timeout = 0;
	}
	if ((nfsrv_sock_tcp_cnt <= 2 * nfsd_thread_max) || (nfsrv_sock_idle_timeout == 0)) {
		nfsrv_idlesock_timer_on = 0;
		lck_mtx_unlock(&nfsd_mutex);
		return;
	}

	TAILQ_FOREACH_SAFE(slp, &nfsrv_socklist, ns_chain, tslp) {
		lck_rw_lock_exclusive(&slp->ns_rwlock);
		                                     
		if (slp->ns_sotype == SOCK_DGRAM || slp->ns_sref) {
			lck_rw_done(&slp->ns_rwlock);
			continue;
		}
		  
                                                                     
                                                                   
                                                                        
                                                                   
                                          
     
		if (((slp->ns_timestamp + nfsrv_sock_idle_timeout) > now.tv_sec) ||
		    nfsrv_sock_tcp_cnt <= 2 * nfsd_thread_max) {
			time_to_wait -= now.tv_sec - slp->ns_timestamp;
			if (time_to_wait < 1) {
				time_to_wait = 1;
			}
			lck_rw_done(&slp->ns_rwlock);
			break;
		}
		  
                                                          
                                                   
     
		slp->ns_sref++;
		nfsrv_zapsock(slp);
		lck_rw_done(&slp->ns_rwlock);
		nfsrv_slpderef_locked(slp);
	}

	                           
	nfs_interval_timer_start(nfsrv_idlesock_timer_call, time_to_wait * 1000);
	                                                                
	nfsrv_idlesock_timer_on = now.tv_sec + time_to_wait;
	lck_mtx_unlock(&nfsd_mutex);
}

  
                                               
   
void
nfsrv_cleanup(void)
{
	struct nfsrv_sock *slp, *nslp;
	struct timeval now;
#if CONFIG_FSE
	struct nfsrv_fmod *fp, *nfp;
	int i;
#endif

	microuptime(&now);
	for (slp = TAILQ_FIRST(&nfsrv_socklist); slp != 0; slp = nslp) {
		nslp = TAILQ_NEXT(slp, ns_chain);
		lck_rw_lock_exclusive(&slp->ns_rwlock);
		slp->ns_sref++;
		if (slp->ns_flag & SLP_VALID) {
			nfsrv_zapsock(slp);
		}
		lck_rw_done(&slp->ns_rwlock);
		nfsrv_slpderef_locked(slp);
	}
#
#if CONFIG_FSE
	  
                                     
    
	lck_mtx_lock(&nfsrv_fmod_mutex);
	for (i = 0; i < NFSRVFMODHASHSZ; i++) {
		for (fp = LIST_FIRST(&nfsrv_fmod_hashtbl[i]); fp; fp = nfp) {
			  
                                                    
                                                  
      
			if (nfsrv_fsevents_enabled) {
				fp->fm_context.vc_thread = current_thread();
				add_fsevent(FSE_CONTENT_MODIFIED, &fp->fm_context,
				    FSE_ARG_VNODE, fp->fm_vp,
				    FSE_ARG_DONE);
			}
			vnode_put(fp->fm_vp);
			kauth_cred_unref(&fp->fm_context.vc_ucred);
			nfp = LIST_NEXT(fp, fm_link);
			LIST_REMOVE(fp, fm_link);
			FREE(fp, M_TEMP);
		}
	}
	nfsrv_fmod_pending = 0;
	lck_mtx_unlock(&nfsrv_fmod_mutex);
#endif

	nfsrv_uc_cleanup();                                          

	nfs_gss_svc_cleanup();                                      

	nfsrv_cleancache();                                     

	nfsrv_udpsock = NULL;
	nfsrv_udp6sock = NULL;
}

#endif                        
