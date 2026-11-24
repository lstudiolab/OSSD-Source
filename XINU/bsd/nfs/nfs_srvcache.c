/*
 * Copyright (c) 2000-2010 Apple Inc. All rights reserved.
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
 *	@(#)nfs_srvcache.c	8.3 (Berkeley) 3/30/95
 * FreeBSD-Id: nfs_srvcache.c,v 1.15 1997/10/12 20:25:46 phk Exp $
 */

#include <nfs/nfs_conf.h>
#if CONFIG_NFS_SERVER

  
                                                                       
                                                              
                                          
   
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/mount_internal.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kpi_mbuf.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <libkern/OSAtomic.h>

#include <netinet/in.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsrvcache.h>

extern int nfsv2_procid[NFS_NPROCS];
static int nfsrv_reqcache_count;
int nfsrv_reqcache_size = NFSRVCACHESIZ;

#define NFSRCHASH(xid) \
	(&nfsrv_reqcache_hashtbl[((xid) + ((xid) >> 24)) & nfsrv_reqcache_hash])
LIST_HEAD(nfsrv_reqcache_hash, nfsrvcache) * nfsrv_reqcache_hashtbl;
TAILQ_HEAD(nfsrv_reqcache_lru, nfsrvcache) nfsrv_reqcache_lruhead;
u_long nfsrv_reqcache_hash;

static LCK_GRP_DECLARE(nfsrv_reqcache_lck_grp, "nfsrv_reqcache");
LCK_MTX_DECLARE(nfsrv_reqcache_mutex, &nfsrv_reqcache_lck_grp);

  
                                                              
   
static int nonidempotent[NFS_NPROCS] = {
	FALSE,
	FALSE,
	TRUE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	TRUE,
	TRUE,
	TRUE,
	TRUE,
	TRUE,
	TRUE,
	TRUE,
	TRUE,
	TRUE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
};

                                                   
static int nfsv2_repstat[NFS_NPROCS] = {
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	TRUE,
	TRUE,
	TRUE,
	TRUE,
	FALSE,
	TRUE,
	FALSE,
	FALSE,
};

  
                                           
   
void
nfsrv_initcache(void)
{
	if (nfsrv_reqcache_size <= 0) {
		return;
	}

	lck_mtx_lock(&nfsrv_reqcache_mutex);
	                                              
	nfsrv_reqcache_hashtbl = hashinit(nfsrv_reqcache_size, M_NFSD, &nfsrv_reqcache_hash);
	TAILQ_INIT(&nfsrv_reqcache_lruhead);
	lck_mtx_unlock(&nfsrv_reqcache_mutex);
}

  
                                                                      
                             
                                       
                                                                        
                                                                           
                                             
   
static int
netaddr_match(
	int family,
	union nethostaddr *haddr,
	mbuf_t nam)
{
	struct sockaddr_in *inetaddr;
	struct sockaddr_in6 *inet6addr;

	switch (family) {
	case AF_INET:
		inetaddr = mbuf_data(nam);
		if ((inetaddr->sin_family == AF_INET) &&
		    (inetaddr->sin_addr.s_addr == haddr->had_inetaddr)) {
			return 1;
		}
		break;
	case AF_INET6:
		inet6addr = mbuf_data(nam);
		if ((inet6addr->sin6_family == AF_INET6) &&
		    !bcmp(&inet6addr->sin6_addr, &haddr->had_inet6addr, sizeof(inet6addr->sin6_addr))) {
			return 1;
		}
		break;
	}
	return 0;
}

  
                                    
                
                                        
       
                            
  
                            
                                        
                                                                  
                                                                           
                
                                            
   
int
nfsrv_getcache(
	struct nfsrv_descript *nd,
	struct nfsrv_sock *slp,
	mbuf_t *mrepp)
{
	struct nfsrvcache *rp;
	struct nfsm_chain nmrep;
	struct sockaddr *saddr;
	int ret, error;

	  
                                                                 
                                                        
    
	if (!nd->nd_nam2) {
		return RC_DOIT;
	}
	lck_mtx_lock(&nfsrv_reqcache_mutex);
loop:
	for (rp = NFSRCHASH(nd->nd_retxid)->lh_first; rp != 0;
	    rp = rp->rc_hash.le_next) {
		if (nd->nd_retxid == rp->rc_xid && nd->nd_procnum == rp->rc_proc &&
		    netaddr_match(rp->rc_family, &rp->rc_haddr, nd->nd_nam)) {
			if ((rp->rc_flag & RC_LOCKED) != 0) {
				rp->rc_flag |= RC_WANTED;
				msleep(rp, &nfsrv_reqcache_mutex, PZERO - 1, "nfsrc", NULL);
				goto loop;
			}
			rp->rc_flag |= RC_LOCKED;
			                                               
			if (rp->rc_lru.tqe_next) {
				TAILQ_REMOVE(&nfsrv_reqcache_lruhead, rp, rc_lru);
				TAILQ_INSERT_TAIL(&nfsrv_reqcache_lruhead, rp, rc_lru);
			}
			if (rp->rc_state == RC_UNUSED) {
				panic("nfsrv cache");
			}
			if (rp->rc_state == RC_INPROG) {
				OSAddAtomic64(1, &nfsstats.srvcache_inproghits);
				ret = RC_DROPIT;
			} else if (rp->rc_flag & RC_REPSTATUS) {
				OSAddAtomic64(1, &nfsstats.srvcache_nonidemdonehits);
				nd->nd_repstat = rp->rc_status;
				error = nfsrv_rephead(nd, slp, &nmrep, 0);
				if (error) {
					printf("nfsrv cache: reply alloc failed for nonidem request hit\n");
					ret = RC_DROPIT;
					*mrepp = NULL;
				} else {
					ret = RC_REPLY;
					*mrepp = nmrep.nmc_mhead;
				}
			} else if (rp->rc_flag & RC_REPMBUF) {
				OSAddAtomic64(1, &nfsstats.srvcache_nonidemdonehits);
				error = mbuf_copym(rp->rc_reply, 0, MBUF_COPYALL, MBUF_WAITOK, mrepp);
				if (error) {
					printf("nfsrv cache: reply copym failed for nonidem request hit\n");
					ret = RC_DROPIT;
				} else {
					ret = RC_REPLY;
				}
			} else {
				OSAddAtomic64(1, &nfsstats.srvcache_idemdonehits);
				rp->rc_state = RC_INPROG;
				ret = RC_DOIT;
			}
			rp->rc_flag &= ~RC_LOCKED;
			if (rp->rc_flag & RC_WANTED) {
				rp->rc_flag &= ~RC_WANTED;
				wakeup(rp);
			}
			lck_mtx_unlock(&nfsrv_reqcache_mutex);
			return ret;
		}
	}
	OSAddAtomic64(1, &nfsstats.srvcache_misses);
	if (nfsrv_reqcache_count < nfsrv_reqcache_size) {
		                                 
		MALLOC(rp, struct nfsrvcache *, sizeof *rp, M_NFSD, M_WAITOK);
		if (rp) {
			bzero((char *)rp, sizeof *rp);
			nfsrv_reqcache_count++;
			rp->rc_flag = RC_LOCKED;
		}
	} else {
		rp = NULL;
	}
	if (!rp) {
		                                                
		rp = nfsrv_reqcache_lruhead.tqh_first;
		if (!rp) {
			                        
			                                                     
			lck_mtx_unlock(&nfsrv_reqcache_mutex);
			return RC_DOIT;
		}
		while ((rp->rc_flag & RC_LOCKED) != 0) {
			rp->rc_flag |= RC_WANTED;
			msleep(rp, &nfsrv_reqcache_mutex, PZERO - 1, "nfsrc", NULL);
			rp = nfsrv_reqcache_lruhead.tqh_first;
		}
		rp->rc_flag |= RC_LOCKED;
		LIST_REMOVE(rp, rc_hash);
		TAILQ_REMOVE(&nfsrv_reqcache_lruhead, rp, rc_lru);
		if (rp->rc_flag & RC_REPMBUF) {
			mbuf_freem(rp->rc_reply);
		}
		if (rp->rc_flag & RC_NAM) {
			mbuf_freem(rp->rc_nam);
		}
		rp->rc_flag &= (RC_LOCKED | RC_WANTED);
	}
	TAILQ_INSERT_TAIL(&nfsrv_reqcache_lruhead, rp, rc_lru);
	rp->rc_state = RC_INPROG;
	rp->rc_xid = nd->nd_retxid;
	saddr = mbuf_data(nd->nd_nam);
	rp->rc_family = saddr->sa_family;
	switch (saddr->sa_family) {
	case AF_INET:
		rp->rc_flag |= RC_INETADDR;
		rp->rc_inetaddr = ((struct sockaddr_in*)saddr)->sin_addr.s_addr;
		break;
	case AF_INET6:
		rp->rc_flag |= RC_INETADDR;
		rp->rc_inet6addr = ((struct sockaddr_in6*)saddr)->sin6_addr;
		break;
	default:
		error = mbuf_copym(nd->nd_nam, 0, MBUF_COPYALL, MBUF_WAITOK, &rp->rc_nam);
		if (error) {
			printf("nfsrv cache: nam copym failed\n");
		} else {
			rp->rc_flag |= RC_NAM;
		}
		break;
	}
	;
	rp->rc_proc = nd->nd_procnum;
	LIST_INSERT_HEAD(NFSRCHASH(nd->nd_retxid), rp, rc_hash);
	rp->rc_flag &= ~RC_LOCKED;
	if (rp->rc_flag & RC_WANTED) {
		rp->rc_flag &= ~RC_WANTED;
		wakeup(rp);
	}
	lck_mtx_unlock(&nfsrv_reqcache_mutex);
	return RC_DOIT;
}

  
                                                           
   
void
nfsrv_updatecache(
	struct nfsrv_descript *nd,
	int repvalid,
	mbuf_t repmbuf)
{
	struct nfsrvcache *rp;
	int error;

	if (!nd->nd_nam2) {
		return;
	}
	lck_mtx_lock(&nfsrv_reqcache_mutex);
loop:
	for (rp = NFSRCHASH(nd->nd_retxid)->lh_first; rp != 0;
	    rp = rp->rc_hash.le_next) {
		if (nd->nd_retxid == rp->rc_xid && nd->nd_procnum == rp->rc_proc &&
		    netaddr_match(rp->rc_family, &rp->rc_haddr, nd->nd_nam)) {
			if ((rp->rc_flag & RC_LOCKED) != 0) {
				rp->rc_flag |= RC_WANTED;
				msleep(rp, &nfsrv_reqcache_mutex, PZERO - 1, "nfsrc", NULL);
				goto loop;
			}
			rp->rc_flag |= RC_LOCKED;
			if (rp->rc_state == RC_DONE) {
				  
                                                
                                             
                                          
                               
       
				if (rp->rc_flag & RC_REPMBUF) {
					mbuf_freem(rp->rc_reply);
					rp->rc_flag &= ~RC_REPMBUF;
				}
			}
			rp->rc_state = RC_DONE;
			  
                                                     
                                         
      
			if (repvalid && nonidempotent[nd->nd_procnum]) {
				if ((nd->nd_vers == NFS_VER2) &&
				    nfsv2_repstat[nfsv2_procid[nd->nd_procnum]]) {
					rp->rc_status = nd->nd_repstat;
					rp->rc_flag |= RC_REPSTATUS;
				} else {
					error = mbuf_copym(repmbuf, 0, MBUF_COPYALL, MBUF_WAITOK, &rp->rc_reply);
					if (!error) {
						rp->rc_flag |= RC_REPMBUF;
					}
				}
			}
			rp->rc_flag &= ~RC_LOCKED;
			if (rp->rc_flag & RC_WANTED) {
				rp->rc_flag &= ~RC_WANTED;
				wakeup(rp);
			}
			lck_mtx_unlock(&nfsrv_reqcache_mutex);
			return;
		}
	}
	lck_mtx_unlock(&nfsrv_reqcache_mutex);
}

  
                                                             
   
void
nfsrv_cleancache(void)
{
	struct nfsrvcache *rp, *nextrp;

	lck_mtx_lock(&nfsrv_reqcache_mutex);
	for (rp = nfsrv_reqcache_lruhead.tqh_first; rp != 0; rp = nextrp) {
		nextrp = rp->rc_lru.tqe_next;
		LIST_REMOVE(rp, rc_hash);
		TAILQ_REMOVE(&nfsrv_reqcache_lruhead, rp, rc_lru);
		_FREE(rp, M_NFSD);
	}
	nfsrv_reqcache_count = 0;
	FREE(nfsrv_reqcache_hashtbl, M_TEMP);
	lck_mtx_unlock(&nfsrv_reqcache_mutex);
}

#endif                        
