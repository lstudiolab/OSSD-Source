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
 *	@(#)nfs_subs.c	8.8 (Berkeley) 5/22/95
 * FreeBSD-Id: nfs_subs.c,v 1.47 1997/11/07 08:53:24 phk Exp $
 */

#include <nfs/nfs_conf.h>
#if CONFIG_NFS

  
                                                                     
                                                                      
                                               
   
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mount_internal.h>
#include <sys/vnode_internal.h>
#include <sys/kpi_mbuf.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/syscall.h>
#include <sys/ubc_internal.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/domain.h>
#include <libkern/OSAtomic.h>
#include <kern/thread_call.h>
#include <kern/task.h>

#include <sys/vm.h>
#include <sys/vmparam.h>

#include <sys/time.h>
#include <kern/clock.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#if CONFIG_NFS_CLIENT
#define _NFS_XDR_SUBS_FUNCS_                                                     
#endif
#include <nfs/xdr_subs.h>
#include <nfs/nfsm_subs.h>
#include <nfs/nfs_gss.h>
#include <nfs/nfsmount.h>
#include <nfs/nfs_lock.h>

#include <miscfs/specfs/specdev.h>

#include <netinet/in.h>
#include <net/kpi_interface.h>

#include <sys/utfconv.h>

  
              
   
struct nfsstats __attribute__((aligned(8))) nfsstats;
size_t nfs_mbuf_mhlen = 0, nfs_mbuf_minclsize = 0;

  
                                                 
   
nfstype
vtonfs_type(enum vtype vtype, int nfsvers)
{
	switch (vtype) {
	case VNON:
		return NFNON;
	case VREG:
		return NFREG;
	case VDIR:
		return NFDIR;
	case VBLK:
		return NFBLK;
	case VCHR:
		return NFCHR;
	case VLNK:
		return NFLNK;
	case VSOCK:
		if (nfsvers > NFS_VER2) {
			return NFSOCK;
		}
		return NFNON;
	case VFIFO:
		if (nfsvers > NFS_VER2) {
			return NFFIFO;
		}
		return NFNON;
	case VBAD:
	case VSTR:
	case VCPLX:
	default:
		return NFNON;
	}
}

enum vtype
nfstov_type(nfstype nvtype, int nfsvers)
{
	switch (nvtype) {
	case NFNON:
		return VNON;
	case NFREG:
		return VREG;
	case NFDIR:
		return VDIR;
	case NFBLK:
		return VBLK;
	case NFCHR:
		return VCHR;
	case NFLNK:
		return VLNK;
	case NFSOCK:
		if (nfsvers > NFS_VER2) {
			return VSOCK;
		}
		OS_FALLTHROUGH;
	case NFFIFO:
		if (nfsvers > NFS_VER2) {
			return VFIFO;
		}
		OS_FALLTHROUGH;
	case NFATTRDIR:
		if (nfsvers > NFS_VER3) {
			return VDIR;
		}
		OS_FALLTHROUGH;
	case NFNAMEDATTR:
		if (nfsvers > NFS_VER3) {
			return VREG;
		}
		OS_FALLTHROUGH;
	default:
		return VNON;
	}
}

int
vtonfsv2_mode(enum vtype vtype, mode_t m)
{
	switch (vtype) {
	case VNON:
	case VREG:
	case VDIR:
	case VBLK:
	case VCHR:
	case VLNK:
	case VSOCK:
		return vnode_makeimode(vtype, m);
	case VFIFO:
		return vnode_makeimode(VCHR, m);
	case VBAD:
	case VSTR:
	case VCPLX:
	default:
		return vnode_makeimode(VNON, m);
	}
}

#if CONFIG_NFS_SERVER

  
                                                               
   
int nfsv3_procid[NFS_NPROCS] = {
	NFSPROC_NULL,
	NFSPROC_GETATTR,
	NFSPROC_SETATTR,
	NFSPROC_NOOP,
	NFSPROC_LOOKUP,
	NFSPROC_READLINK,
	NFSPROC_READ,
	NFSPROC_NOOP,
	NFSPROC_WRITE,
	NFSPROC_CREATE,
	NFSPROC_REMOVE,
	NFSPROC_RENAME,
	NFSPROC_LINK,
	NFSPROC_SYMLINK,
	NFSPROC_MKDIR,
	NFSPROC_RMDIR,
	NFSPROC_READDIR,
	NFSPROC_FSSTAT,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP
};

#endif                        

  
                                                                      
   
int nfsv2_procid[NFS_NPROCS] = {
	NFSV2PROC_NULL,
	NFSV2PROC_GETATTR,
	NFSV2PROC_SETATTR,
	NFSV2PROC_LOOKUP,
	NFSV2PROC_NOOP,
	NFSV2PROC_READLINK,
	NFSV2PROC_READ,
	NFSV2PROC_WRITE,
	NFSV2PROC_CREATE,
	NFSV2PROC_MKDIR,
	NFSV2PROC_SYMLINK,
	NFSV2PROC_CREATE,
	NFSV2PROC_REMOVE,
	NFSV2PROC_RMDIR,
	NFSV2PROC_RENAME,
	NFSV2PROC_LINK,
	NFSV2PROC_READDIR,
	NFSV2PROC_NOOP,
	NFSV2PROC_STATFS,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP
};


  
                                           
   
void
nfs_mbuf_init(void)
{
	struct mbuf_stat ms;

	mbuf_stats(&ms);
	nfs_mbuf_mhlen = ms.mhlen;
	nfs_mbuf_minclsize = ms.minclsize;
}

#if CONFIG_NFS_SERVER

  
                                                            
   
int
nfsm_mbuf_get_list(size_t size, mbuf_t *mp, int *mbcnt)
{
	int error, cnt;
	mbuf_t mhead, mlast, m;
	size_t len, mlen;

	error = cnt = 0;
	mhead = mlast = NULL;
	len = 0;

	while (len < size) {
		nfsm_mbuf_getcluster(error, &m, (size - len));
		if (error) {
			break;
		}
		if (!mhead) {
			mhead = m;
		}
		if (mlast && ((error = mbuf_setnext(mlast, m)))) {
			mbuf_free(m);
			break;
		}
		mlen = mbuf_maxlen(m);
		if ((len + mlen) > size) {
			mlen = size - len;
		}
		mbuf_setlen(m, mlen);
		len += mlen;
		cnt++;
		mlast = m;
	}

	if (!error) {
		*mp = mhead;
		*mbcnt = cnt;
	}
	return error;
}

#endif                        

  
                        
  
                                     
   
int
nfsm_chain_new_mbuf(struct nfsm_chain *nmc, size_t sizehint)
{
	mbuf_t mb;
	int error = 0;

	if (nmc->nmc_flags & NFSM_CHAIN_FLAG_ADD_CLUSTERS) {
		sizehint = nfs_mbuf_minclsize;
	}

	                         
	nfsm_mbuf_getcluster(error, &mb, sizehint);
	if (error) {
		return error;
	}
	if (mb == NULL) {
		panic("got NULL mbuf?");
	}

	                                
	if (nmc->nmc_mcur) {
		                                
		mbuf_setlen(nmc->nmc_mcur, nmc->nmc_ptr - (caddr_t)mbuf_data(nmc->nmc_mcur));
		                              
		error = mbuf_setnext(nmc->nmc_mcur, mb);
		if (error) {
			mbuf_free(mb);
			return error;
		}
	}

	                                   
	nmc->nmc_mcur = mb;
	nmc->nmc_ptr = mbuf_data(mb);
	nmc->nmc_left = mbuf_trailingspace(mb);

	return 0;
}

  
                            
  
                                                                         
   
int
nfsm_chain_add_opaque_f(struct nfsm_chain *nmc, const u_char *buf, size_t len)
{
	size_t paddedlen, tlen;
	int error;

	paddedlen = nfsm_rndup(len);

	while (paddedlen) {
		if (!nmc->nmc_left) {
			error = nfsm_chain_new_mbuf(nmc, paddedlen);
			if (error) {
				return error;
			}
		}
		tlen = MIN(nmc->nmc_left, paddedlen);
		if (tlen) {
			if (len) {
				if (tlen > len) {
					tlen = len;
				}
				bcopy(buf, nmc->nmc_ptr, tlen);
			} else {
				bzero(nmc->nmc_ptr, tlen);
			}
			nmc->nmc_ptr += tlen;
			nmc->nmc_left -= tlen;
			paddedlen -= tlen;
			if (len) {
				buf += tlen;
				len -= tlen;
			}
		}
	}
	return 0;
}

  
                                  
  
                                                                         
                  
   
int
nfsm_chain_add_opaque_nopad_f(struct nfsm_chain *nmc, const u_char *buf, size_t len)
{
	size_t tlen;
	int error;

	while (len > 0) {
		if (nmc->nmc_left <= 0) {
			error = nfsm_chain_new_mbuf(nmc, len);
			if (error) {
				return error;
			}
		}
		tlen = MIN(nmc->nmc_left, len);
		bcopy(buf, nmc->nmc_ptr, tlen);
		nmc->nmc_ptr += tlen;
		nmc->nmc_left -= tlen;
		len -= tlen;
		buf += tlen;
	}
	return 0;
}

  
                       
  
                                                         
   
int
nfsm_chain_add_uio(struct nfsm_chain *nmc, uio_t uio, size_t len)
{
	size_t paddedlen, tlen;
	int error;

	paddedlen = nfsm_rndup(len);

	while (paddedlen) {
		if (!nmc->nmc_left) {
			error = nfsm_chain_new_mbuf(nmc, paddedlen);
			if (error) {
				return error;
			}
		}
		tlen = MIN(nmc->nmc_left, paddedlen);
		if (tlen) {
			if (len) {
				tlen = MIN(INT32_MAX, MIN(tlen, len));
				uiomove(nmc->nmc_ptr, (int)tlen, uio);
			} else {
				bzero(nmc->nmc_ptr, tlen);
			}
			nmc->nmc_ptr += tlen;
			nmc->nmc_left -= tlen;
			paddedlen -= tlen;
			if (len) {
				len -= tlen;
			}
		}
	}
	return 0;
}

  
                                        
                                              
   
size_t
nfsm_chain_offset(struct nfsm_chain *nmc)
{
	mbuf_t mb;
	size_t len = 0;

	for (mb = nmc->nmc_mhead; mb; mb = mbuf_next(mb)) {
		if (mb == nmc->nmc_mcur) {
			return len + (nmc->nmc_ptr - (caddr_t) mbuf_data(mb));
		}
		len += mbuf_len(mb);
	}

	return len;
}

  
                       
  
                                        
   
int
nfsm_chain_advance(struct nfsm_chain *nmc, size_t len)
{
	mbuf_t mb;

	while (len) {
		if (nmc->nmc_left >= len) {
			nmc->nmc_left -= len;
			nmc->nmc_ptr += len;
			return 0;
		}
		len -= nmc->nmc_left;
		nmc->nmc_mcur = mb = mbuf_next(nmc->nmc_mcur);
		if (!mb) {
			return EBADRPC;
		}
		nmc->nmc_ptr = mbuf_data(mb);
		nmc->nmc_left = mbuf_len(mb);
	}

	return 0;
}

  
                       
  
                                                         
   
int
nfsm_chain_reverse(struct nfsm_chain *nmc, size_t len)
{
	size_t mlen, new_offset;
	int error = 0;

	mlen = nmc->nmc_ptr - (caddr_t) mbuf_data(nmc->nmc_mcur);
	if (len <= mlen) {
		nmc->nmc_ptr -= len;
		nmc->nmc_left += len;
		return 0;
	}

	new_offset = nfsm_chain_offset(nmc) - len;
	nfsm_chain_dissect_init(error, nmc, nmc->nmc_mhead);
	if (error) {
		return error;
	}

	return nfsm_chain_advance(nmc, new_offset);
}

  
                                    
  
                                                                 
                                                                  
                                                  
  
                                                         
   
int
nfsm_chain_get_opaque_pointer_f(struct nfsm_chain *nmc, uint32_t len, u_char **pptr)
{
	mbuf_t mbcur, mb;
	uint32_t padlen;
	size_t mblen, cplen, need, left;
	u_char *ptr;
	int error = 0;

	                                 
	while (nmc->nmc_mcur && (nmc->nmc_left == 0)) {
		mb = mbuf_next(nmc->nmc_mcur);
		nmc->nmc_mcur = mb;
		if (!mb) {
			break;
		}
		nmc->nmc_ptr = mbuf_data(mb);
		nmc->nmc_left = mbuf_len(mb);
	}
	                                    
	if (!nmc->nmc_mcur) {
		return EBADRPC;
	}

	                                             
	if (nmc->nmc_left >= len) {
		                                                      
		*pptr = (u_char*)nmc->nmc_ptr;
		error = nfsm_chain_advance(nmc, nfsm_rndup(len));
		return error;
	}

	padlen = nfsm_rndup(len) - len;

	                                     
	mbcur = nmc->nmc_mcur;
	left = nmc->nmc_left;
	need = len - left;

	if (need > mbuf_trailingspace(mbcur)) {
		  
                                                            
                                                              
     
		nfsm_mbuf_getcluster(error, &mb, len);
		if (error) {
			return error;
		}
		                                                       
		if (mbuf_maxlen(mb) < len) {
			mbuf_free(mb);
			return EOVERFLOW;
		}

		                                                              
		*pptr = ptr = mbuf_data(mb);

		                                       
		bcopy(nmc->nmc_ptr, ptr, left);
		ptr += left;
		mbuf_setlen(mb, left);

		                                                            
		error = mbuf_setnext(mb, mbuf_next(mbcur));
		if (!error) {
			error = mbuf_setnext(mbcur, mb);
		}
		if (error) {
			mbuf_free(mb);
			return error;
		}

		                                            
		mbuf_setlen(mbcur, mbuf_len(mbcur) - left);

		  
                                                       
                                             
     
		nmc->nmc_mcur = mbcur = mb;
		nmc->nmc_left = 0;
		nmc->nmc_ptr = (caddr_t)ptr;
	} else {
		                                                 

		                                                      
		*pptr = (u_char*)nmc->nmc_ptr;

		  
                                                       
                                             
     
		nmc->nmc_ptr += left;
		nmc->nmc_left = 0;
	}

	  
                                               
                                   
    

	                                
	mbuf_setlen(mbcur, mbuf_len(mbcur) + need);

	                                                         
	mb = mbuf_next(mbcur);

	while (need && mb) {
		                                 
		ptr = mbuf_data(mb);
		mblen = mbuf_len(mb);
		cplen = MIN(mblen, need);
		if (cplen) {
			bcopy(ptr, nmc->nmc_ptr, cplen);
			  
                                                          
                                                          
      
			error = mbuf_setdata(mb, ptr + cplen, mblen - cplen);
			if (error) {
				mbuf_setlen(mbcur, mbuf_len(mbcur) - need);
				return error;
			}
			                         
			nmc->nmc_ptr += cplen;
			need -= cplen;
		}
		                                     
		if (need) {
			mb = mbuf_next(mb);
		}
	}

	                                               
	if (need) {
		mbuf_setlen(mbcur, mbuf_len(mbcur) - need);
		return EBADRPC;
	}

	  
                                                          
   
                                                          
                                                           
    
	nmc->nmc_mcur = mb;
	nmc->nmc_ptr = mbuf_data(mb);
	nmc->nmc_left = mbuf_len(mb);

	                           
	if (padlen) {
		error = nfsm_chain_advance(nmc, padlen);
	}

	return error;
}

  
                            
  
                                                     
                                                         
   
int
nfsm_chain_get_opaque_f(struct nfsm_chain *nmc, size_t len, u_char *buf)
{
	size_t cplen, padlen;
	int error = 0;

	padlen = nfsm_rndup(len) - len;

	                                                     
	while (len && nmc->nmc_mcur) {
		                                 
		cplen = MIN(nmc->nmc_left, len);
		if (cplen) {
			bcopy(nmc->nmc_ptr, buf, cplen);
			nmc->nmc_ptr += cplen;
			nmc->nmc_left -= cplen;
			buf += cplen;
			len -= cplen;
		}
		                                     
		if (len) {
			mbuf_t mb = mbuf_next(nmc->nmc_mcur);
			nmc->nmc_mcur = mb;
			nmc->nmc_ptr = mb ? mbuf_data(mb) : NULL;
			nmc->nmc_left = mb ? mbuf_len(mb) : 0;
		}
	}

	                                               
	if (len) {
		return EBADRPC;
	}

	if (padlen) {
		nfsm_chain_adv(error, nmc, padlen);
	}

	return error;
}

  
                       
  
                                                             
                                                         
   
int
nfsm_chain_get_uio(struct nfsm_chain *nmc, size_t len, uio_t uio)
{
	size_t cplen, padlen;
	int error = 0;

	padlen = nfsm_rndup(len) - len;

	                                                     
	while (len && nmc->nmc_mcur) {
		                                 
		cplen = MIN(nmc->nmc_left, len);
		if (cplen) {
			cplen = MIN(cplen, INT32_MAX);
			error = uiomove(nmc->nmc_ptr, (int)cplen, uio);
			if (error) {
				return error;
			}
			nmc->nmc_ptr += cplen;
			nmc->nmc_left -= cplen;
			len -= cplen;
		}
		                                     
		if (len) {
			mbuf_t mb = mbuf_next(nmc->nmc_mcur);
			nmc->nmc_mcur = mb;
			nmc->nmc_ptr = mb ? mbuf_data(mb) : NULL;
			nmc->nmc_left = mb ? mbuf_len(mb) : 0;
		}
	}

	                                               
	if (len) {
		return EBADRPC;
	}

	if (padlen) {
		nfsm_chain_adv(error, nmc, padlen);
	}

	return error;
}

#if CONFIG_NFS_CLIENT

int
nfsm_chain_add_string_nfc(struct nfsm_chain *nmc, const uint8_t *s, size_t slen)
{
	uint8_t smallbuf[64];
	uint8_t *nfcname = smallbuf;
	size_t buflen = sizeof(smallbuf), nfclen;
	int error;

	error = utf8_normalizestr(s, slen, nfcname, &nfclen, buflen, UTF_PRECOMPOSED | UTF_NO_NULL_TERM);
	if (error == ENAMETOOLONG) {
		buflen = MAXPATHLEN;
		nfcname = zalloc(ZV_NAMEI);
		error = utf8_normalizestr(s, slen, nfcname, &nfclen, buflen, UTF_PRECOMPOSED | UTF_NO_NULL_TERM);
	}

	                                                      
	if (error) {
		nfsm_chain_add_string(error, nmc, s, slen);
	} else {
		nfsm_chain_add_string(error, nmc, nfcname, nfclen);
	}

	if (nfcname && (nfcname != smallbuf)) {
		NFS_ZFREE(ZV_NAMEI, nfcname);
	}
	return error;
}

  
                                                  
   
int
nfsm_chain_add_v2sattr_f(struct nfsm_chain *nmc, struct vnode_attr *vap, uint32_t szrdev)
{
	int error = 0;

	nfsm_chain_add_32(error, nmc, vtonfsv2_mode(vap->va_type,
	    (VATTR_IS_ACTIVE(vap, va_mode) ? vap->va_mode : 0600)));
	nfsm_chain_add_32(error, nmc,
	    VATTR_IS_ACTIVE(vap, va_uid) ? vap->va_uid : (uint32_t)-1);
	nfsm_chain_add_32(error, nmc,
	    VATTR_IS_ACTIVE(vap, va_gid) ? vap->va_gid : (uint32_t)-1);
	nfsm_chain_add_32(error, nmc, szrdev);
	nfsm_chain_add_v2time(error, nmc,
	    VATTR_IS_ACTIVE(vap, va_access_time) ?
	    &vap->va_access_time : NULL);
	nfsm_chain_add_v2time(error, nmc,
	    VATTR_IS_ACTIVE(vap, va_modify_time) ?
	    &vap->va_modify_time : NULL);

	return error;
}

  
                                                  
   
int
nfsm_chain_add_v3sattr_f(
	__unused struct nfsmount *nmp,
	struct nfsm_chain *nmc,
	struct vnode_attr *vap)
{
	int error = 0;

	if (VATTR_IS_ACTIVE(vap, va_mode)) {
		nfsm_chain_add_32(error, nmc, TRUE);
		nfsm_chain_add_32(error, nmc, vap->va_mode);
	} else {
		nfsm_chain_add_32(error, nmc, FALSE);
	}
	if (VATTR_IS_ACTIVE(vap, va_uid)) {
		nfsm_chain_add_32(error, nmc, TRUE);
		nfsm_chain_add_32(error, nmc, vap->va_uid);
	} else {
		nfsm_chain_add_32(error, nmc, FALSE);
	}
	if (VATTR_IS_ACTIVE(vap, va_gid)) {
		nfsm_chain_add_32(error, nmc, TRUE);
		nfsm_chain_add_32(error, nmc, vap->va_gid);
	} else {
		nfsm_chain_add_32(error, nmc, FALSE);
	}
	if (VATTR_IS_ACTIVE(vap, va_data_size)) {
		nfsm_chain_add_32(error, nmc, TRUE);
		nfsm_chain_add_64(error, nmc, vap->va_data_size);
	} else {
		nfsm_chain_add_32(error, nmc, FALSE);
	}
	if (vap->va_vaflags & VA_UTIMES_NULL) {
		nfsm_chain_add_32(error, nmc, NFS_TIME_SET_TO_SERVER);
		nfsm_chain_add_32(error, nmc, NFS_TIME_SET_TO_SERVER);
	} else {
		if (VATTR_IS_ACTIVE(vap, va_access_time)) {
			nfsm_chain_add_32(error, nmc, NFS_TIME_SET_TO_CLIENT);
			nfsm_chain_add_32(error, nmc, vap->va_access_time.tv_sec);
			nfsm_chain_add_32(error, nmc, vap->va_access_time.tv_nsec);
		} else {
			nfsm_chain_add_32(error, nmc, NFS_TIME_DONT_CHANGE);
		}
		if (VATTR_IS_ACTIVE(vap, va_modify_time)) {
			nfsm_chain_add_32(error, nmc, NFS_TIME_SET_TO_CLIENT);
			nfsm_chain_add_32(error, nmc, vap->va_modify_time.tv_sec);
			nfsm_chain_add_32(error, nmc, vap->va_modify_time.tv_nsec);
		} else {
			nfsm_chain_add_32(error, nmc, NFS_TIME_DONT_CHANGE);
		}
	}


	return error;
}


  
                           
  
                                                                    
   
int
nfsm_chain_get_fh_attr(
	struct nfsmount *nmp,
	struct nfsm_chain *nmc,
	nfsnode_t dnp,
	vfs_context_t ctx,
	int nfsvers,
	uint64_t *xidp,
	fhandle_t *fhp,
	struct nfs_vattr *nvap)
{
	int error = 0, gotfh, gotattr;

	gotfh = gotattr = 1;

	if (nfsvers == NFS_VER3) {                            
		nfsm_chain_get_32(error, nmc, gotfh);
	}
	if (!error && gotfh) {                      
		nfsm_chain_get_fh(error, nmc, nfsvers, fhp);
	} else {
		fhp->fh_len = 0;
	}
	if (nfsvers == NFS_VER3) {                                
		nfsm_chain_get_32(error, nmc, gotattr);
	}
	nfsmout_if(error);
	if (gotattr) {
		if (!gotfh) {                      
			nfsm_chain_adv(error, nmc, NFSX_V3FATTR);
		} else {                     
			error = nfs_parsefattr(nmp, nmc, nfsvers, nvap);
		}
	} else if (gotfh) {
		                                                          
		if (nfs3_getattr_rpc(NULL, NFSTOMP(dnp), fhp->fh_data, fhp->fh_len, 0, ctx, nvap, xidp)) {
			gotattr = 0;
			fhp->fh_len = 0;
		}
	}
nfsmout:
	return error;
}

  
                                                    
   
int
nfsm_chain_get_wcc_data_f(
	struct nfsm_chain *nmc,
	nfsnode_t np,
	struct timespec *premtime,
	int *newpostattr,
	u_int64_t *xidp)
{
	int error = 0;
	uint32_t flag = 0;

	nfsm_chain_get_32(error, nmc, flag);
	if (!error && flag) {
		nfsm_chain_adv(error, nmc, 2 * NFSX_UNSIGNED);
		nfsm_chain_get_32(error, nmc, premtime->tv_sec);
		nfsm_chain_get_32(error, nmc, premtime->tv_nsec);
		nfsm_chain_adv(error, nmc, 2 * NFSX_UNSIGNED);
	} else {
		premtime->tv_sec = 0;
		premtime->tv_nsec = 0;
	}
	nfsm_chain_postop_attr_update_flag(error, nmc, np, *newpostattr, xidp);

	return error;
}

  
                                        
   
void
nfs_get_xid(uint64_t *xidp)
{
	struct timeval tv;

	lck_mtx_lock(&nfs_request_mutex);
	if (!nfs_xid) {
		  
                                         
    
                                                                
                                                               
                                                                  
                                                      
     
		microtime(&tv);
		nfs_xid = tv.tv_sec << 12;
	}
	if (++nfs_xid == 0) {
		                                             
		nfs_xidwrap++;
		nfs_xid++;
	}
	*xidp = nfs_xid + (nfs_xidwrap << 32);
	lck_mtx_unlock(&nfs_request_mutex);
}

  
                                                           
                                                 
   

int
nfsm_rpchead(
	struct nfsreq *req,
	mbuf_t mrest,
	u_int64_t *xidp,
	mbuf_t *mreqp)
{
	struct nfsmount *nmp = req->r_nmp;
	int nfsvers = nmp->nm_vers;
	int proc = ((nfsvers == NFS_VER2) ? nfsv2_procid[req->r_procnum] : (int)req->r_procnum);

	return nfsm_rpchead2(nmp, nmp->nm_sotype, NFS_PROG, nfsvers, proc,
	           req->r_auth, req->r_cred, req, mrest, xidp, mreqp);
}

  
                                                                        
  
                                                           
                                                  
                                                                                          
  
                                       
  
                                                                                            
                
   
static size_t
get_auxiliary_groups(kauth_cred_t cred, gid_t groups[NGROUPS], size_t count)
{
	gid_t pgid;
	size_t maxcount = count < NGROUPS ? count + 1 : NGROUPS;
	size_t i;

	for (i = 0; i < NGROUPS; i++) {
		groups[i] = -2;                                     
	}
	(void)kauth_cred_getgroups(cred, groups, &maxcount);
	if (maxcount < 1) {
		return maxcount;
	}

	  
                                                              
                                                                       
                                                                   
                                             
    


	if (count < NGROUPS) {
		pgid = kauth_cred_getgid(cred);
		if (pgid == groups[0]) {
			maxcount -= 1;
			for (i = 0; i < maxcount; i++) {
				groups[i] = groups[i + 1];
			}
		}
	}

	return maxcount;
}

int
nfsm_rpchead2(__unused struct nfsmount *nmp, int sotype, int prog, int vers, int proc, int auth_type,
    kauth_cred_t cred, struct nfsreq *req, mbuf_t mrest, u_int64_t *xidp, mbuf_t *mreqp)
{
	mbuf_t mreq, mb;
	size_t i;
	int error, auth_len = 0, authsiz, reqlen;
	size_t headlen;
	struct nfsm_chain nmreq;
	gid_t grouplist[NGROUPS];
	size_t groupcount = 0;

	                                    
	switch (auth_type) {
	case RPCAUTH_NONE:
		auth_len = 0;
		break;
	case RPCAUTH_SYS:
	{
		size_t count = nmp->nm_numgrps < NGROUPS ? nmp->nm_numgrps : NGROUPS;

		if (!cred) {
			return EINVAL;
		}
		groupcount = get_auxiliary_groups(cred, grouplist, count);
		auth_len = ((uint32_t)groupcount + 5) * NFSX_UNSIGNED;
		break;
	}
#if CONFIG_NFS_GSS
	case RPCAUTH_KRB5:
	case RPCAUTH_KRB5I:
	case RPCAUTH_KRB5P:
		if (!req || !cred) {
			return EINVAL;
		}
		auth_len = 5 * NFSX_UNSIGNED + 0;                                       
		break;
#endif                     
	default:
		return EINVAL;
	}
	authsiz = nfsm_rndup(auth_len);

	                         
	headlen = authsiz + 10 * NFSX_UNSIGNED;
	if (sotype == SOCK_STREAM) {                                                
		headlen += NFSX_UNSIGNED;
	}
	if (headlen >= nfs_mbuf_minclsize) {
		error = mbuf_getpacket(MBUF_WAITOK, &mreq);
	} else {
		error = mbuf_gethdr(MBUF_WAITOK, MBUF_TYPE_DATA, &mreq);
		if (!error) {
			if (headlen < nfs_mbuf_mhlen) {
				mbuf_align_32(mreq, headlen);
			} else {
				mbuf_align_32(mreq, 8 * NFSX_UNSIGNED);
			}
		}
	}
	if (error) {
		                               
		                                                     
		return error;
	}

	  
                                                            
                                                             
                                  
    
	if (*xidp == 0) {
		nfs_get_xid(xidp);
	}

	                         
	nfsm_chain_init(&nmreq, mreq);

	                                                                     
	if (sotype == SOCK_STREAM) {
		nfsm_chain_add_32(error, &nmreq, 0);
	}

	                          
	nfsm_chain_add_32(error, &nmreq, (*xidp & 0xffffffff));
	nfsm_chain_add_32(error, &nmreq, RPC_CALL);
	nfsm_chain_add_32(error, &nmreq, RPC_VER2);
	nfsm_chain_add_32(error, &nmreq, prog);
	nfsm_chain_add_32(error, &nmreq, vers);
	nfsm_chain_add_32(error, &nmreq, proc);

#if CONFIG_NFS_GSS
add_cred:
#endif
	switch (auth_type) {
	case RPCAUTH_NONE:
		nfsm_chain_add_32(error, &nmreq, RPCAUTH_NONE);           
		nfsm_chain_add_32(error, &nmreq, 0);                        
		nfsm_chain_add_32(error, &nmreq, RPCAUTH_NONE);           
		nfsm_chain_add_32(error, &nmreq, 0);                        
		nfsm_chain_build_done(error, &nmreq);
		                           
		if (!error) {
			error = mbuf_setnext(nmreq.nmc_mcur, mrest);
		}
		break;
	case RPCAUTH_SYS: {
		nfsm_chain_add_32(error, &nmreq, RPCAUTH_SYS);
		nfsm_chain_add_32(error, &nmreq, authsiz);
		{
			nfsm_chain_add_32(error, &nmreq, 0);               
		}
		nfsm_chain_add_32(error, &nmreq, 0);                              
		nfsm_chain_add_32(error, &nmreq, kauth_cred_getuid(cred));               
		nfsm_chain_add_32(error, &nmreq, kauth_cred_getgid(cred));               
		nfsm_chain_add_32(error, &nmreq, groupcount);                     
		for (i = 0; i < groupcount; i++) {
			nfsm_chain_add_32(error, &nmreq, grouplist[i]);
		}

		                         
		nfsm_chain_add_32(error, &nmreq, RPCAUTH_NONE);             
		nfsm_chain_add_32(error, &nmreq, 0);                        
		nfsm_chain_build_done(error, &nmreq);

		                           
		if (!error) {
			error = mbuf_setnext(nmreq.nmc_mcur, mrest);
		}
		break;
	}
#if CONFIG_NFS_GSS
	case RPCAUTH_KRB5:
	case RPCAUTH_KRB5I:
	case RPCAUTH_KRB5P:
		error = nfs_gss_clnt_cred_put(req, &nmreq, mrest);
		if (error == ENEEDAUTH) {
			size_t count = nmp->nm_numgrps < NGROUPS ? nmp->nm_numgrps : NGROUPS;

			  
                               
      
			error = 0;
			req->r_auth = auth_type = RPCAUTH_SYS;
			groupcount = get_auxiliary_groups(cred, grouplist, count);
			auth_len = ((uint32_t)groupcount + 5) * NFSX_UNSIGNED;
			authsiz = nfsm_rndup(auth_len);
			goto add_cred;
		}
		break;
#endif                     
	}
	;

	                                  
	if (!error) {
		error = mbuf_pkthdr_setrcvif(mreq, 0);
	}

	if (error) {
		mbuf_freem(mreq);
		return error;
	}

	                                       
	reqlen = 0;
	for (mb = nmreq.nmc_mhead; mb; mb = mbuf_next(mb)) {
		reqlen += mbuf_len(mb);
	}

	mbuf_pkthdr_setlen(mreq, reqlen);

	  
                                        
                                        
                                                
                                     
    
	if (sotype == SOCK_STREAM) {
		nfsm_chain_set_recmark(error, &nmreq,
		    (reqlen - NFSX_UNSIGNED) | 0x80000000);
	}

	*mreqp = mreq;
	return 0;
}

  
                                                              
   
int
nfs_parsefattr(
	__unused struct nfsmount *nmp,
	struct nfsm_chain *nmc,
	int nfsvers,
	struct nfs_vattr *nvap)
{
	int error = 0;
	enum vtype vtype;
	nfstype nvtype;
	uint32_t vmode, val, val2;
	dev_t rdev;

	val = val2 = 0;
	NVATTR_INIT(nvap);

	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_TYPE);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_MODE);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_NUMLINKS);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_OWNER);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_OWNER_GROUP);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_SIZE);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_SPACE_USED);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_RAWDEV);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_FSID);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_FILEID);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_TIME_ACCESS);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_TIME_MODIFY);
	NFS_BITMAP_SET(nvap->nva_bitmap, NFS_FATTR_TIME_METADATA);

	nfsm_chain_get_32(error, nmc, nvtype);
	nfsm_chain_get_32(error, nmc, vmode);
	nfsmout_if(error);

	if (nfsvers == NFS_VER3) {
		nvap->nva_type = vtype = nfstov_type(nvtype, nfsvers);
	} else {
		  
                                                              
                                                   
    
                                                            
                                                               
                                                              
                                                           
                                                            
                                                               
                                                               
                                                         
    
                                                         
                                                         
                                    
     
		vtype = nfstov_type(nvtype, nfsvers);
		if ((vtype == VNON) || ((vtype == VREG) && ((vmode & S_IFMT) != 0))) {
			vtype = IFTOVT(vmode);
		}
		nvap->nva_type = vtype;
	}

	nvap->nva_mode = (vmode & 07777);

	nfsm_chain_get_32(error, nmc, nvap->nva_nlink);
	nfsm_chain_get_32(error, nmc, nvap->nva_uid);
	nfsm_chain_get_32(error, nmc, nvap->nva_gid);

	if (nfsvers == NFS_VER3) {
		nfsm_chain_get_64(error, nmc, nvap->nva_size);
		nfsm_chain_get_64(error, nmc, nvap->nva_bytes);
		nfsm_chain_get_32(error, nmc, nvap->nva_rawdev.specdata1);
		nfsm_chain_get_32(error, nmc, nvap->nva_rawdev.specdata2);
		nfsmout_if(error);
		nfsm_chain_get_64(error, nmc, nvap->nva_fsid.major);
		nvap->nva_fsid.minor = 0;
		nfsm_chain_get_64(error, nmc, nvap->nva_fileid);
	} else {
		nfsm_chain_get_32(error, nmc, nvap->nva_size);
		nfsm_chain_adv(error, nmc, NFSX_UNSIGNED);
		nfsm_chain_get_32(error, nmc, rdev);
		nfsmout_if(error);
		nvap->nva_rawdev.specdata1 = major(rdev);
		nvap->nva_rawdev.specdata2 = minor(rdev);
		nfsm_chain_get_32(error, nmc, val);             
		nfsmout_if(error);
		nvap->nva_bytes = val * NFS_FABLKSIZE;
		nfsm_chain_get_32(error, nmc, val);
		nfsmout_if(error);
		nvap->nva_fsid.major = (uint64_t)val;
		nvap->nva_fsid.minor = 0;
		nfsm_chain_get_32(error, nmc, val);
		nfsmout_if(error);
		nvap->nva_fileid = (uint64_t)val;
		                               
		if ((vtype == VCHR) && (rdev == (dev_t)0xffffffff)) {
			nvap->nva_type = VFIFO;
		}
	}
	nfsm_chain_get_time(error, nmc, nfsvers,
	    nvap->nva_timesec[NFSTIME_ACCESS],
	    nvap->nva_timensec[NFSTIME_ACCESS]);
	nfsm_chain_get_time(error, nmc, nfsvers,
	    nvap->nva_timesec[NFSTIME_MODIFY],
	    nvap->nva_timensec[NFSTIME_MODIFY]);
	nfsm_chain_get_time(error, nmc, nfsvers,
	    nvap->nva_timesec[NFSTIME_CHANGE],
	    nvap->nva_timensec[NFSTIME_CHANGE]);

nfsmout:
	return error;
}


  
                                                                  
                                                                      
                                                                     
                             
  
                                                                          
                                  
   
int
nfs_loadattrcache(
	nfsnode_t np,
	struct nfs_vattr *nvap,
	u_int64_t *xidp,
	int dontshrink)
{
	mount_t mp;
	vnode_t vp;
	struct timeval now;
	struct nfs_vattr *npnvap;
	int xattr = np->n_vattr.nva_flags & NFS_FFLAG_IS_ATTR;
	int referral = np->n_vattr.nva_flags & NFS_FFLAG_TRIGGER_REFERRAL;
	int aclbit, monitored, error = 0;
	kauth_acl_t acl;
	struct nfsmount *nmp;
	uint32_t events = np->n_events;

	if (np->n_hflag & NHINIT) {
		vp = NULL;
		mp = np->n_mount;
	} else {
		vp = NFSTOV(np);
		mp = vnode_mount(vp);
	}
	monitored = vp ? vnode_ismonitored(vp) : 0;

	FSDBG_TOP(527, np, vp, *xidp >> 32, *xidp);

	if (!((nmp = VFSTONFS(mp)))) {
		FSDBG_BOT(527, ENXIO, 1, 0, *xidp);
		return ENXIO;
	}

	if (*xidp < np->n_xid) {
		  
                                                            
                                                               
                                                       
                                                                 
                                                                 
                                                               
                                                           
                                       
     
		NATTRINVALIDATE(np);
		FSDBG_BOT(527, 0, np, np->n_xid, *xidp);
		*xidp = 0;
		return 0;
	}

	if (vp && (nvap->nva_type != vnode_vtype(vp))) {
		  
                                                        
                                                              
                                                       
                                                      
                                               
    
                                                         
                                                      
                          
    
                                           
    
                                                         
                                                         
                                                        
                                                         
                                                        
                                                          
                 
     
		printf("nfs loadattrcache vnode changed type, was %d now %d\n",
		    vnode_vtype(vp), nvap->nva_type);
		error = ESTALE;
		if (monitored) {
			events |= VNODE_EVENT_DELETE;
		}
		goto out;
	}

	npnvap = &np->n_vattr;

	  
                                                          
                                                           
                                                            
    
	aclbit = NFS_BITMAP_ISSET(npnvap->nva_bitmap, NFS_FATTR_ACL);
	acl = npnvap->nva_acl;

	if (monitored) {
		  
                                                                                  
     
		if (NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_NUMLINKS) &&
		    (nvap->nva_nlink != npnvap->nva_nlink)) {
			events |= VNODE_EVENT_ATTRIB | VNODE_EVENT_LINK;
		}
		if (events & VNODE_EVENT_PERMS) {
			                                                        ;
		} else if (NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_MODE) &&
		    (nvap->nva_mode != npnvap->nva_mode)) {
			events |= VNODE_EVENT_ATTRIB | VNODE_EVENT_PERMS;
		} else if (NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_OWNER) &&
		    (nvap->nva_uid != npnvap->nva_uid)) {
			events |= VNODE_EVENT_ATTRIB | VNODE_EVENT_PERMS;
		} else if (NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_OWNER_GROUP) &&
		    (nvap->nva_gid != npnvap->nva_gid)) {
			events |= VNODE_EVENT_ATTRIB | VNODE_EVENT_PERMS;
#if CONFIG_NFS4
		} else if (nmp->nm_vers >= NFS_VER4) {
			if (NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_OWNER) &&
			    !kauth_guid_equal(&nvap->nva_uuuid, &npnvap->nva_uuuid)) {
				events |= VNODE_EVENT_ATTRIB | VNODE_EVENT_PERMS;
			} else if (NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_OWNER_GROUP) &&
			    !kauth_guid_equal(&nvap->nva_guuid, &npnvap->nva_guuid)) {
				events |= VNODE_EVENT_ATTRIB | VNODE_EVENT_PERMS;
			} else if ((NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_ACL) &&
			    nvap->nva_acl && npnvap->nva_acl &&
			    ((nvap->nva_acl->acl_entrycount != npnvap->nva_acl->acl_entrycount) ||
			    bcmp(nvap->nva_acl, npnvap->nva_acl, KAUTH_ACL_COPYSIZE(nvap->nva_acl))))) {
				events |= VNODE_EVENT_ATTRIB | VNODE_EVENT_PERMS;
			}
#endif
		}
		if (              
#if CONFIG_NFS4
			((nmp->nm_vers >= NFS_VER4) && NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_CHANGE) && (nvap->nva_change != npnvap->nva_change)) ||
#endif
			(NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_TIME_MODIFY) &&
			((nvap->nva_timesec[NFSTIME_MODIFY] != npnvap->nva_timesec[NFSTIME_MODIFY]) ||
			(nvap->nva_timensec[NFSTIME_MODIFY] != npnvap->nva_timensec[NFSTIME_MODIFY])))) {
			events |= VNODE_EVENT_ATTRIB | VNODE_EVENT_WRITE;
		}
		if (!events && NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_RAWDEV) &&
		    ((nvap->nva_rawdev.specdata1 != npnvap->nva_rawdev.specdata1) ||
		    (nvap->nva_rawdev.specdata2 != npnvap->nva_rawdev.specdata2))) {
			events |= VNODE_EVENT_ATTRIB;
		}
		if (!events && NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_FILEID) &&
		    (nvap->nva_fileid != npnvap->nva_fileid)) {
			events |= VNODE_EVENT_ATTRIB;
		}
		if (!events && NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_ARCHIVE) &&
		    ((nvap->nva_flags & NFS_FFLAG_ARCHIVED) != (npnvap->nva_flags & NFS_FFLAG_ARCHIVED))) {
			events |= VNODE_EVENT_ATTRIB;
		}
		if (!events && NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_HIDDEN) &&
		    ((nvap->nva_flags & NFS_FFLAG_HIDDEN) != (npnvap->nva_flags & NFS_FFLAG_HIDDEN))) {
			events |= VNODE_EVENT_ATTRIB;
		}
		if (!events && NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_TIME_CREATE) &&
		    ((nvap->nva_timesec[NFSTIME_CREATE] != npnvap->nva_timesec[NFSTIME_CREATE]) ||
		    (nvap->nva_timensec[NFSTIME_CREATE] != npnvap->nva_timensec[NFSTIME_CREATE]))) {
			events |= VNODE_EVENT_ATTRIB;
		}
		if (!events && NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_TIME_BACKUP) &&
		    ((nvap->nva_timesec[NFSTIME_BACKUP] != npnvap->nva_timesec[NFSTIME_BACKUP]) ||
		    (nvap->nva_timensec[NFSTIME_BACKUP] != npnvap->nva_timensec[NFSTIME_BACKUP]))) {
			events |= VNODE_EVENT_ATTRIB;
		}
	}

#if CONFIG_NFS4
	                                                
	if (nmp->nm_vers >= NFS_VER4 && npnvap->nva_flags & NFS_FFLAG_PARTIAL_WRITE) {
		  
                                                                                                                            
                                                                                    
     
		NFS_BITMAP_COPY_ATTR(nvap, npnvap, TYPE, type);
		NFS_BITMAP_COPY_ATTR(nvap, npnvap, CHANGE, change);
		NFS_BITMAP_COPY_ATTR(nvap, npnvap, SIZE, size);
		NFS_BITMAP_COPY_TIME(nvap, npnvap, METADATA, CHANGE);
		NFS_BITMAP_COPY_TIME(nvap, npnvap, MODIFY, MODIFY);
	} else
#endif                  
	{
		bcopy((caddr_t)nvap, (caddr_t)npnvap, sizeof(*nvap));
		microuptime(&now);
		np->n_attrstamp = now.tv_sec;
	}

	np->n_xid = *xidp;
	                                                                           
	if (vp && xattr) {
		nvap->nva_flags |= xattr;
	}
	if (vp && referral) {
		nvap->nva_flags |= referral;
	}

	if (NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_ACL)) {
		                            
		if (nvap->nva_acl) {
			                                          
			npnvap->nva_acl = kauth_acl_alloc(nvap->nva_acl->acl_entrycount);
			if (npnvap->nva_acl) {
				bcopy(nvap->nva_acl, npnvap->nva_acl, KAUTH_ACL_COPYSIZE(nvap->nva_acl));
			} else {
				                                                      
				NFS_BITMAP_CLR(npnvap->nva_bitmap, NFS_FATTR_ACL);
				NACLINVALIDATE(np);
				aclbit = 0;
			}
		}
		if (acl) {
			kauth_acl_free(acl);
			acl = NULL;
		}
	}
	if (NFS_BITMAP_ISSET(nvap->nva_bitmap, NFS_FATTR_ACL)) {
		                              
		np->n_aclstamp = now.tv_sec;
	} else {
		                                                            
		if (aclbit) {
			NFS_BITMAP_SET(npnvap->nva_bitmap, NFS_FATTR_ACL);
		}
		npnvap->nva_acl = acl;
	}

#if CONFIG_TRIGGERS
#if CONFIG_NFS4
	  
                                                                     
                                                                       
                                                                   
    
	if ((nmp->nm_vers >= NFS_VER4) && (nvap->nva_type == VDIR) &&
	    ((np->n_vattr.nva_fsid.major != nmp->nm_fsid.major) ||
	    (np->n_vattr.nva_fsid.minor != nmp->nm_fsid.minor))) {
		np->n_vattr.nva_flags |= NFS_FFLAG_TRIGGER;
	}
#endif                  
#endif                      

	if (!vp || (nvap->nva_type != VREG)) {
		np->n_size = nvap->nva_size;
	} else if (nvap->nva_size != np->n_size) {
		FSDBG(527, np, nvap->nva_size, np->n_size, (nvap->nva_type == VREG) | (np->n_flag & NMODIFIED ? 6 : 4));
		if (!UBCINFOEXISTS(vp) || (dontshrink && (nvap->nva_size < np->n_size))) {
			                                                     
			FSDBG(527, np, np->n_size, np->n_vattr.nva_size, 0xf00d0001);
			nvap->nva_size = np->n_size;
			NATTRINVALIDATE(np);
		} else if ((np->n_flag & NMODIFIED) && (nvap->nva_size < np->n_size)) {
			                                               
			FSDBG(527, np, np->n_size, np->n_vattr.nva_size, 0xf00d0002);
			nvap->nva_size = np->n_size;
			npnvap->nva_size = np->n_size;
		} else {
			  
                                                         
                                                              
                                                                    
                                                     
      
			np->n_newsize = nvap->nva_size;
			SET(np->n_flag, NUPDATESIZE);
			if (monitored) {
				events |= VNODE_EVENT_ATTRIB | VNODE_EVENT_EXTEND;
			}
		}
	}

	if (np->n_flag & NCHG) {
		if (np->n_flag & NACC) {
			nvap->nva_timesec[NFSTIME_ACCESS] = np->n_atim.tv_sec;
			nvap->nva_timensec[NFSTIME_ACCESS] = np->n_atim.tv_nsec;
		}
		if (np->n_flag & NUPD) {
			nvap->nva_timesec[NFSTIME_MODIFY] = np->n_mtim.tv_sec;
			nvap->nva_timensec[NFSTIME_MODIFY] = np->n_mtim.tv_nsec;
		}
	}

out:
	if (monitored && events) {
		nfs_vnode_notify(np, events);
	}
	FSDBG_BOT(527, error, np, np->n_size, *xidp);
	return error;
}

  
                                           
                                           
   
long
nfs_attrcachetimeout(nfsnode_t np)
{
	struct nfsmount *nmp;
	struct timeval now;
	int isdir;
	long timeo;

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return 0;
	}

	isdir = vnode_isdir(NFSTOV(np));
#if CONFIG_NFS4
	if ((nmp->nm_vers >= NFS_VER4) && (np->n_openflags & N_DELEG_MASK)) {
		                                                             
		timeo = isdir ? nmp->nm_acdirmax : nmp->nm_acregmax;
	} else
#endif
	if ((np)->n_flag & NMODIFIED) {
		                                                              
		timeo = isdir ? nmp->nm_acdirmin : nmp->nm_acregmin;
	} else {
		                                                               
		                                                                    
		                                                             
		microtime(&now);
		timeo = (now.tv_sec - (np)->n_vattr.nva_timesec[NFSTIME_MODIFY]) / 10;
		if (isdir) {
			if (timeo < nmp->nm_acdirmin) {
				timeo = nmp->nm_acdirmin;
			} else if (timeo > nmp->nm_acdirmax) {
				timeo = nmp->nm_acdirmax;
			}
		} else {
			if (timeo < nmp->nm_acregmin) {
				timeo = nmp->nm_acregmin;
			} else if (timeo > nmp->nm_acregmax) {
				timeo = nmp->nm_acregmax;
			}
		}
	}

	return timeo;
}

  
                                        
                                                               
                             
                                       
   
int
nfs_getattrcache(nfsnode_t np, struct nfs_vattr *nvaper, int flags)
{
	struct nfs_vattr *nvap;
	struct timeval nowup;
	long timeo;
	struct nfsmount *nmp;

	                                        
	if (!NATTRVALID(np) || ((flags & NGA_ACL) && !NACLVALID(np))) {
		FSDBG(528, np, 0, 0xffffff01, ENOENT);
		OSAddAtomic64(1, &nfsstats.attrcache_misses);
		return ENOENT;
	}

	nmp = NFSTONMP(np);
	if (nfs_mount_gone(nmp)) {
		return ENXIO;
	}
	  
                                                   
                                                  
                                 
    
	if (!nfs_use_cache(nmp)) {
		microuptime(&nowup);
		if (np->n_attrstamp > nowup.tv_sec) {
			printf("NFS: Attribute time stamp is in the future by %ld seconds. Invalidating cache\n",
			    np->n_attrstamp - nowup.tv_sec);
			NATTRINVALIDATE(np);
			NACCESSINVALIDATE(np);
			return ENOENT;
		}
		timeo = nfs_attrcachetimeout(np);
		if ((nowup.tv_sec - np->n_attrstamp) >= timeo) {
			FSDBG(528, np, 0, 0xffffff02, ENOENT);
			OSAddAtomic64(1, &nfsstats.attrcache_misses);
			return ENOENT;
		}
		if ((flags & NGA_ACL) && ((nowup.tv_sec - np->n_aclstamp) >= timeo)) {
			FSDBG(528, np, 0, 0xffffff02, ENOENT);
			OSAddAtomic64(1, &nfsstats.attrcache_misses);
			return ENOENT;
		}
	}

	nvap = &np->n_vattr;
	FSDBG(528, np, nvap->nva_size, np->n_size, 0xcace);
	OSAddAtomic64(1, &nfsstats.attrcache_hits);

	if (nvap->nva_type != VREG) {
		np->n_size = nvap->nva_size;
	} else if (nvap->nva_size != np->n_size) {
		FSDBG(528, np, nvap->nva_size, np->n_size, (nvap->nva_type == VREG) | (np->n_flag & NMODIFIED ? 6 : 4));
		if ((np->n_flag & NMODIFIED) && (nvap->nva_size < np->n_size)) {
			                                               
			nvap->nva_size = np->n_size;
		} else {
			  
                                                         
                                                              
                                                                    
                                                     
      
			np->n_newsize = nvap->nva_size;
			SET(np->n_flag, NUPDATESIZE);
		}
	}

	bcopy((caddr_t)nvap, (caddr_t)nvaper, sizeof(struct nfs_vattr));
	if (np->n_flag & NCHG) {
		if (np->n_flag & NACC) {
			nvaper->nva_timesec[NFSTIME_ACCESS] = np->n_atim.tv_sec;
			nvaper->nva_timensec[NFSTIME_ACCESS] = np->n_atim.tv_nsec;
		}
		if (np->n_flag & NUPD) {
			nvaper->nva_timesec[NFSTIME_MODIFY] = np->n_mtim.tv_sec;
			nvaper->nva_timensec[NFSTIME_MODIFY] = np->n_mtim.tv_nsec;
		}
	}
	if (nvap->nva_acl) {
		if (flags & NGA_ACL) {
			nvaper->nva_acl = kauth_acl_alloc(nvap->nva_acl->acl_entrycount);
			if (!nvaper->nva_acl) {
				return ENOMEM;
			}
			bcopy(nvap->nva_acl, nvaper->nva_acl, KAUTH_ACL_COPYSIZE(nvap->nva_acl));
		} else {
			nvaper->nva_acl = NULL;
		}
	}
	return 0;
}

  
                                     
                                                                                     
                                                                            
   
void
nfs_avoid_needless_id_setting_on_create(nfsnode_t dnp, struct vnode_attr *vap, vfs_context_t ctx)
{
	if (VATTR_IS_ACTIVE(vap, va_uid)) {
		if (kauth_cred_getuid(vfs_context_ucred(ctx)) == vap->va_uid) {
			VATTR_CLEAR_ACTIVE(vap, va_uid);
			VATTR_CLEAR_ACTIVE(vap, va_uuuid);
		}
	}
	if (VATTR_IS_ACTIVE(vap, va_gid)) {
		if ((vap->va_gid == dnp->n_vattr.nva_gid) ||
		    (kauth_cred_getgid(vfs_context_ucred(ctx)) == vap->va_gid)) {
			VATTR_CLEAR_ACTIVE(vap, va_gid);
			VATTR_CLEAR_ACTIVE(vap, va_guuid);
		}
	}
}

  
                                                              
  
                                                       
  
                     
                         
                     
  
          
              
                  
                      
                      
                          
  
                                                       
                                                                         
  
                      
                          
                          
                                   
                                                               
                                                                       
  
                               
                                                              
                     
                             
                         
   

#define IS_DIGIT(C) \
	(((C) >= '0') && ((C) <= '9'))

#define IS_XDIGIT(C) \
	(IS_DIGIT(C) || \
	 (((C) >= 'A') && ((C) <= 'F')) || \
	 (((C) >= 'a') && ((C) <= 'f')))

int
nfs_uaddr2sockaddr(const char *uaddr, struct sockaddr *addr)
{
	const char *p, *pd;                                                
	const char *pnum;                                                
	const char *pscope;                                   
	uint8_t a[18];                                                  
	int i;                                                     
	int dci;                                                                  
	int dcount, xdcount;                                           
	int needmore;                                                                               
	int dots;                              
	int hex;                                         
	unsigned long val;                         
	int s;                                                                            

	                                                                 
	if (*uaddr == '/' || *uaddr == '\0') {                       
		struct sockaddr_un *sun = (struct sockaddr_un *)addr;
		sun->sun_family = AF_LOCAL;
		sun->sun_len = sizeof(struct sockaddr_un);
		strlcpy(sun->sun_path, uaddr, sizeof(sun->sun_path));

		return 1;
	}

#define HEXVALUE        0
#define DECIMALVALUE    1

#define GET(TYPE) \
	do { \
	        if ((dcount <= 0) || (dcount > (((TYPE) == DECIMALVALUE) ? 3 : 4))) \
	                return (0); \
	        if (((TYPE) == DECIMALVALUE) && xdcount) \
	                return (0); \
	        val = strtoul(pnum, NULL, ((TYPE) == DECIMALVALUE) ? 10 : 16); \
	        if (((TYPE) == DECIMALVALUE) && (val >= 256)) \
	                return (0); \
	                                               \
	        if (i > (int)(sizeof(a) - (((TYPE) == HEXVALUE) ? 2 : 1) - ((dci != -1) ? 2 : 0))) \
	                return (0); \
	        if ((TYPE) == HEXVALUE) \
	                a[i++] = ((val >> 8) & 0xff); \
	        a[i++] = (val & 0xff); \
	} while (0)

	hex = 0;
	dots = 0;
	dci = -1;
	i = dcount = xdcount = 0;
	pnum = p = uaddr;
	pscope = NULL;
	needmore = 1;
	if ((*p == ':') && (*++p != ':')) {                                                 
		return 0;
	}

	while (*p) {
		if (IS_XDIGIT(*p)) {
			dcount++;
			if (!IS_DIGIT(*p)) {
				xdcount++;
			}
			needmore = 0;
			p++;
		} else if (*p == '.') {
			                                                  
			if (!dots) {
				                                      
				for (pd = p; *pd; pd++) {
					if (*pd == '.') {
						if (++dots > 5) {
							return 0;
						}
					} else if (hex && (*pd == '%')) {
						break;
					} else if ((*pd < '0') || (*pd > '9')) {
						return 0;
					}
				}
				if ((dots != 2) && (dots != 3) && (dots != 5)) {
					return 0;
				}
				if (hex && (dots == 2)) {               
					if (!dcount && needmore) {
						return 0;
					}
					if (dcount) {                                  
						GET(HEXVALUE);
					}
				} else {
					GET(DECIMALVALUE);
				}
			} else {
				GET(DECIMALVALUE);
			}
			dcount = xdcount = 0;
			needmore = 1;
			pnum = ++p;
		} else if (*p == ':') {
			hex = 1;
			if (dots) {
				return 0;
			}
			if (!dcount) {                                            
				if (dci >= 0) {                                     
					return 0;
				}
				dci = i;
				needmore = 0;
			} else {
				GET(HEXVALUE);
				dcount = xdcount = 0;
				needmore = 1;
			}
			pnum = ++p;
		} else if (*p == '%') {                         
			if (!hex) {
				return 0;
			}
			p++;
			pscope = p;
			break;
		} else {                           
			return 0;
		}
	}
	if (needmore && !dcount) {
		return 0;
	}
	if (dcount) {                             
		GET(dots ? DECIMALVALUE : HEXVALUE);
	}
	if (dci >= 0) {                                                                 
		                                             
		                                                          
		int end = ((dots == 2) || (dots == 5)) ? sizeof(a) : (sizeof(a) - 2);
		if (i % 2) {                                                 
			return 0;
		}
		if (i >= end) {               
			return 0;
		}
		                                             
		for (s = 0; s < (i - dci); s++) {
			a[end - 1 - s] = a[i - 1 - s];
		}
		                                       
		for (s = 0; s < (end - i); s++) {
			a[dci + s] = 0;
		}
		i = end;
	}

	                                       
	if (hex) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)addr;
		if ((((dots == 0) || (dots == 3)) && (i != (sizeof(a) - 2)))) {
			return 0;
		}
		if ((((dots == 2) || (dots == 5)) && (i != sizeof(a)))) {
			return 0;
		}
		bzero(sin6, sizeof(struct sockaddr_in6));
		sin6->sin6_len = sizeof(struct sockaddr_in6);
		sin6->sin6_family = AF_INET6;
		bcopy(a, &sin6->sin6_addr.s6_addr, sizeof(struct in6_addr));
		if ((dots == 5) || (dots == 2)) {
			sin6->sin6_port = htons((in_port_t)((a[16] << 8) | a[17]));
		}
		if (pscope) {
			for (p = pscope; IS_DIGIT(*p); p++) {
				;
			}
			if (*p && !IS_DIGIT(*p)) {           
				ifnet_t interface = NULL;
				if (ifnet_find_by_name(pscope, &interface) == 0) {
					sin6->sin6_scope_id = ifnet_index(interface);
				}
				if (interface) {
					ifnet_release(interface);
				}
			} else {                     
				sin6->sin6_scope_id = (uint32_t)strtoul(pscope, NULL, 10);
			}
			                                                      
		}
	} else {
		struct sockaddr_in *sin = (struct sockaddr_in*)addr;
		if ((dots != 3) && (dots != 5)) {
			return 0;
		}
		if ((dots == 3) && (i != 4)) {
			return 0;
		}
		if ((dots == 5) && (i != 6)) {
			return 0;
		}
		bzero(sin, sizeof(struct sockaddr_in));
		sin->sin_len = sizeof(struct sockaddr_in);
		sin->sin_family = AF_INET;
		bcopy(a, &sin->sin_addr.s_addr, sizeof(struct in_addr));
		if (dots == 5) {
			sin->sin_port = htons((in_port_t)((a[4] << 8) | a[5]));
		}
	}
	return 1;
}


                                  
uint32_t nfs_debug_ctl;

#include <libkern/libkern.h>
#include <stdarg.h>

void
nfs_printf(unsigned int facility, unsigned int level, const char *fmt, ...)
{
	va_list ap;

	if (NFS_IS_DBG(facility, level)) {
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	}
}


#define DISPLAYLEN 16

static bool
isprint(int ch)
{
	return ch >= 0x20 && ch <= 0x7e;
}

static void
hexdump(void *data, size_t len)
{
	size_t i, j;
	unsigned char *d = data;
	char *p, disbuf[3 * DISPLAYLEN + 1];

	for (i = 0; i < len; i += DISPLAYLEN) {
		for (p = disbuf, j = 0; (j + i) < len && j < DISPLAYLEN; j++, p += 3) {
			snprintf(p, 4, "%2.2x ", d[i + j]);
		}
		for (; j < DISPLAYLEN; j++, p += 3) {
			snprintf(p, 4, "   ");
		}
		printf("%s    ", disbuf);
		for (p = disbuf, j = 0; (j + i) < len && j < DISPLAYLEN; j++, p++) {
			snprintf(p, 2, "%c", isprint(d[i + j]) ? d[i + j] : '.');
		}
		printf("%s\n", disbuf);
	}
}

void
nfs_dump_mbuf(const char *func, int lineno, const char *msg, mbuf_t mb)
{
	mbuf_t m;

	printf("%s:%d %s\n", func, lineno, msg);
	for (m = mb; m; m = mbuf_next(m)) {
		hexdump(mbuf_data(m), mbuf_len(m));
	}
}

                           
int
nfs_mount_gone(struct nfsmount *nmp)
{
	return !nmp || vfs_isforce(nmp->nm_mountp) || (nmp->nm_state & (NFSSTA_FORCE | NFSSTA_DEAD));
}

  
                                                    
                                                                      
   
int
nfs_mountopts(struct nfsmount *nmp, char *buf, int buflen)
{
	int c;

	c = snprintf(buf, buflen, "%s,%s,%s,%s,vers=%d,sec=%s,%sdeadtimeout=%d",
	    (vfs_flags(nmp->nm_mountp) & MNT_RDONLY) ? "ro" : "rw",
	    NMFLAG(nmp, SOFT) ? "soft" : "hard",
	    NMFLAG(nmp, INTR) ? "intr" : "nointr",
	    nmp->nm_sotype == SOCK_STREAM ? "tcp" : "udp",
	    nmp->nm_vers,
	    nmp->nm_auth == RPCAUTH_KRB5  ? "krb5" :
	    nmp->nm_auth == RPCAUTH_KRB5I ? "krb5i" :
	    nmp->nm_auth == RPCAUTH_KRB5P ? "krb5p" :
	    nmp->nm_auth == RPCAUTH_SYS   ? "sys" : "none",
	    nmp->nm_lockmode == NFS_LOCK_MODE_ENABLED ?  "locks," :
	    nmp->nm_lockmode == NFS_LOCK_MODE_DISABLED ? "nolocks," :
	    nmp->nm_lockmode == NFS_LOCK_MODE_LOCAL ? "locallocks," : "",
	    nmp->nm_deadtimeout);

	return c > buflen ? ENOMEM : 0;
}

#endif                        

  
                                                         
                                       
   
void
nfs_interval_timer_start(thread_call_t call, time_t interval)
{
	uint64_t deadline;

	clock_interval_to_deadline((int)interval, 1000 * 1000, &deadline);
	thread_call_enter_delayed(call, deadline);
}


#if CONFIG_NFS_SERVER

int nfsrv_cmp_secflavs(struct nfs_sec *, struct nfs_sec *);
int nfsrv_hang_addrlist(struct nfs_export *, struct user_nfs_export_args *);
int nfsrv_free_netopt(struct radix_node *, void *);
int nfsrv_free_addrlist(struct nfs_export *, struct user_nfs_export_args *);
struct nfs_export_options *nfsrv_export_lookup(struct nfs_export *, mbuf_t);
struct nfs_export *nfsrv_fhtoexport(struct nfs_filehandle *);
struct nfs_user_stat_node *nfsrv_get_user_stat_node(struct nfs_active_user_list *, struct sockaddr *, uid_t);
void nfsrv_init_user_list(struct nfs_active_user_list *);
void nfsrv_free_user_list(struct nfs_active_user_list *);

  
                                      
   
int
nfsm_chain_add_wcc_data_f(
	struct nfsrv_descript *nd,
	struct nfsm_chain *nmc,
	int preattrerr,
	struct vnode_attr *prevap,
	int postattrerr,
	struct vnode_attr *postvap)
{
	int error = 0;

	if (preattrerr) {
		nfsm_chain_add_32(error, nmc, FALSE);
	} else {
		nfsm_chain_add_32(error, nmc, TRUE);
		nfsm_chain_add_64(error, nmc, prevap->va_data_size);
		nfsm_chain_add_time(error, nmc, NFS_VER3, &prevap->va_modify_time);
		nfsm_chain_add_time(error, nmc, NFS_VER3, &prevap->va_change_time);
	}
	nfsm_chain_add_postop_attr(error, nd, nmc, postattrerr, postvap);

	return error;
}

  
                                                             
                                                                   
   
int
nfsm_chain_get_path_namei(
	struct nfsm_chain *nmc,
	uint32_t len,
	struct nameidata *nip)
{
	struct componentname *cnp = &nip->ni_cnd;
	int error = 0;
	char *cp;

	if (len > (MAXPATHLEN - 1)) {
		return ENAMETOOLONG;
	}

	  
                                                            
                         
    
	cnp->cn_pnbuf = zalloc(ZV_NAMEI);
	cnp->cn_pnlen = MAXPATHLEN;
	cnp->cn_flags |= HASBUF;

	                                                    
	cp = cnp->cn_pnbuf;
	nfsm_chain_get_opaque(error, nmc, len, cp);
	if (error) {
		goto out;
	}
	cnp->cn_pnbuf[len] = '\0';

	                             
	if ((strlen(cp) != len) || strchr(cp, '/')) {
		error = EACCES;
	}
out:
	if (error) {
		if (cnp->cn_pnbuf) {
			NFS_ZFREE(ZV_NAMEI, cnp->cn_pnbuf);
		}
		cnp->cn_flags &= ~HASBUF;
	} else {
		nip->ni_pathlen = len;
	}
	return error;
}

  
                                                  
   
int
nfsrv_namei(
	struct nfsrv_descript *nd,
	vfs_context_t ctx,
	struct nameidata *nip,
	struct nfs_filehandle *nfhp,
	vnode_t *retdirp,
	struct nfs_export **nxp,
	struct nfs_export_options **nxop)
{
	vnode_t dp;
	int error;
	struct componentname *cnp = &nip->ni_cnd;
	uint32_t cnflags;
	char *tmppn;

	*retdirp = NULL;

	  
                                       
    
	error = nfsrv_fhtovp(nfhp, nd, &dp, nxp, nxop);
	if (error) {
		goto out;
	}
	error = nfsrv_credcheck(nd, ctx, *nxp, *nxop);
	if (error || (vnode_vtype(dp) != VDIR)) {
		vnode_put(dp);
		error = ENOTDIR;
		goto out;
	}
	*retdirp = dp;

	nip->ni_cnd.cn_context = ctx;

	if (*nxop && ((*nxop)->nxo_flags & NX_READONLY)) {
		cnp->cn_flags |= RDONLY;
	}

	cnp->cn_flags |= NOCROSSMOUNT;
	cnp->cn_nameptr = cnp->cn_pnbuf;
	nip->ni_usedvp = nip->ni_startdir = dp;
	nip->ni_rootdir = rootvnode;

	  
                                         
    
	cnflags = nip->ni_cnd.cn_flags;                                       
	while ((error = lookup(nip)) == ERECYCLE) {
		nip->ni_cnd.cn_flags = cnflags;
		cnp->cn_nameptr = cnp->cn_pnbuf;
		nip->ni_usedvp = nip->ni_dvp = nip->ni_startdir = dp;
	}
	if (error) {
		goto out;
	}

	                                            
	if (cnp->cn_flags & ISSYMLINK) {
		if (cnp->cn_flags & (LOCKPARENT | WANTPARENT)) {
			vnode_put(nip->ni_dvp);
		}
		if (nip->ni_vp) {
			vnode_put(nip->ni_vp);
			nip->ni_vp = NULL;
		}
		error = EINVAL;
	}
out:
	if (error) {
		tmppn = cnp->cn_pnbuf;
		cnp->cn_pnbuf = NULL;
		cnp->cn_flags &= ~HASBUF;
		NFS_ZFREE(ZV_NAMEI, tmppn);
	}
	return error;
}

  
                                                                  
                                           
   
void
nfsm_adj(mbuf_t mp, int len, int nul)
{
	mbuf_t m, mnext;
	int count, i;
	long mlen;
	char *cp;

	  
                                         
                                                     
                                                       
                                                      
                             
    
	count = 0;
	m = mp;
	for (;;) {
		mlen = mbuf_len(m);
		count += mlen;
		mnext = mbuf_next(m);
		if (mnext == NULL) {
			break;
		}
		m = mnext;
	}
	if (mlen > len) {
		mlen -= len;
		mbuf_setlen(m, mlen);
		if (nul > 0) {
			cp = (caddr_t)mbuf_data(m) + mlen - nul;
			for (i = 0; i < nul; i++) {
				*cp++ = '\0';
			}
		}
		return;
	}
	count -= len;
	if (count < 0) {
		count = 0;
	}
	  
                                        
                                                    
                                                
    
	for (m = mp; m; m = mbuf_next(m)) {
		mlen = mbuf_len(m);
		if (mlen >= count) {
			mlen = count;
			mbuf_setlen(m, count);
			if (nul > 0) {
				cp = (caddr_t)mbuf_data(m) + mlen - nul;
				for (i = 0; i < nul; i++) {
					*cp++ = '\0';
				}
			}
			break;
		}
		count -= mlen;
	}
	for (m = mbuf_next(m); m; m = mbuf_next(m)) {
		mbuf_setlen(m, 0);
	}
}

  
                                                                 
                                                      
   
int
nfsm_chain_trim_data(struct nfsm_chain *nmc, int len, int *mlen)
{
	int cnt = 0;
	long dlen, adjust;
	caddr_t data;
	mbuf_t m;

	if (mlen) {
		*mlen = 0;
	}

	                 
	for (m = nmc->nmc_mhead; m && (m != nmc->nmc_mcur); m = mbuf_next(m)) {
		mbuf_setlen(m, 0);
	}
	if (!m) {
		return EIO;
	}

	                       
	data = mbuf_data(m);
	dlen = mbuf_len(m);
	adjust = nmc->nmc_ptr - data;
	dlen -= adjust;
	if ((dlen > 0) && (adjust > 0)) {
		if (mbuf_setdata(m, nmc->nmc_ptr, dlen)) {
			return EIO;
		}
	} else {
		mbuf_setlen(m, dlen);
	}

	                          
	for (; m && (cnt < len); m = mbuf_next(m)) {
		dlen = mbuf_len(m);
		cnt += dlen;
		if (cnt > len) {
			                             
			mbuf_setlen(m, dlen - (cnt - len));
			if (m == nmc->nmc_mcur) {
				nmc->nmc_left -= (cnt - len);
			}
			cnt = len;
		}
	}
	if (mlen) {
		*mlen = cnt;
	}

	                            
	if (m == nmc->nmc_mcur) {
		nmc->nmc_left = 0;
	}
	for (; m; m = mbuf_next(m)) {
		mbuf_setlen(m, 0);
	}

	return 0;
}

int
nfsm_chain_add_fattr(
	struct nfsrv_descript *nd,
	struct nfsm_chain *nmc,
	struct vnode_attr *vap)
{
	int error = 0;

	                                                           

	nfsm_chain_add_32(error, nmc, vtonfs_type(vap->va_type, nd->nd_vers));
	if (nd->nd_vers == NFS_VER3) {
		nfsm_chain_add_32(error, nmc, vap->va_mode & 07777);
	} else {
		nfsm_chain_add_32(error, nmc, vtonfsv2_mode(vap->va_type, vap->va_mode));
	}
	nfsm_chain_add_32(error, nmc, vap->va_nlink);
	nfsm_chain_add_32(error, nmc, vap->va_uid);
	nfsm_chain_add_32(error, nmc, vap->va_gid);
	if (nd->nd_vers == NFS_VER3) {
		nfsm_chain_add_64(error, nmc, vap->va_data_size);
		nfsm_chain_add_64(error, nmc, vap->va_data_alloc);
		nfsm_chain_add_32(error, nmc, major(vap->va_rdev));
		nfsm_chain_add_32(error, nmc, minor(vap->va_rdev));
		nfsm_chain_add_64(error, nmc, vap->va_fsid);
		nfsm_chain_add_64(error, nmc, vap->va_fileid);
	} else {
		nfsm_chain_add_32(error, nmc, vap->va_data_size);
		nfsm_chain_add_32(error, nmc, NFS_FABLKSIZE);
		if (vap->va_type == VFIFO) {
			nfsm_chain_add_32(error, nmc, 0xffffffff);
		} else {
			nfsm_chain_add_32(error, nmc, vap->va_rdev);
		}
		nfsm_chain_add_32(error, nmc, vap->va_data_alloc / NFS_FABLKSIZE);
		nfsm_chain_add_32(error, nmc, vap->va_fsid);
		nfsm_chain_add_32(error, nmc, vap->va_fileid);
	}
	nfsm_chain_add_time(error, nmc, nd->nd_vers, &vap->va_access_time);
	nfsm_chain_add_time(error, nmc, nd->nd_vers, &vap->va_modify_time);
	nfsm_chain_add_time(error, nmc, nd->nd_vers, &vap->va_change_time);

	return error;
}

int
nfsm_chain_get_sattr(
	struct nfsrv_descript *nd,
	struct nfsm_chain *nmc,
	struct vnode_attr *vap)
{
	int error = 0;
	uint32_t val = 0;
	uint64_t val64 = 0;
	struct timespec now;

	if (nd->nd_vers == NFS_VER2) {
		  
                                                                      
                                                                  
                                                                     
     
		nfsm_chain_get_32(error, nmc, val);
		if ((val & 0xffff) != 0xffff) {
			VATTR_SET(vap, va_mode, val & 07777);
			                                           
			VATTR_SET(vap, va_type, IFTOVT(val));
			VATTR_CLEAR_ACTIVE(vap, va_type);
		}
		nfsm_chain_get_32(error, nmc, val);
		if (val != (uint32_t)-1) {
			VATTR_SET(vap, va_uid, val);
		}
		nfsm_chain_get_32(error, nmc, val);
		if (val != (uint32_t)-1) {
			VATTR_SET(vap, va_gid, val);
		}
		                                                                       
		nfsm_chain_get_32(error, nmc, val);
		VATTR_SET(vap, va_data_size, val);
		if (val == (uint32_t)-1) {
			VATTR_CLEAR_ACTIVE(vap, va_data_size);
		}
		nfsm_chain_get_time(error, nmc, NFS_VER2,
		    vap->va_access_time.tv_sec,
		    vap->va_access_time.tv_nsec);
		if (vap->va_access_time.tv_sec != -1) {
			VATTR_SET_ACTIVE(vap, va_access_time);
		}
		nfsm_chain_get_time(error, nmc, NFS_VER2,
		    vap->va_modify_time.tv_sec,
		    vap->va_modify_time.tv_nsec);
		if (vap->va_modify_time.tv_sec != -1) {
			VATTR_SET_ACTIVE(vap, va_modify_time);
		}
		return error;
	}

	           
	nfsm_chain_get_32(error, nmc, val);
	if (val) {
		nfsm_chain_get_32(error, nmc, val);
		VATTR_SET(vap, va_mode, val & 07777);
	}
	nfsm_chain_get_32(error, nmc, val);
	if (val) {
		nfsm_chain_get_32(error, nmc, val);
		VATTR_SET(vap, va_uid, val);
	}
	nfsm_chain_get_32(error, nmc, val);
	if (val) {
		nfsm_chain_get_32(error, nmc, val);
		VATTR_SET(vap, va_gid, val);
	}
	nfsm_chain_get_32(error, nmc, val);
	if (val) {
		nfsm_chain_get_64(error, nmc, val64);
		VATTR_SET(vap, va_data_size, val64);
	}
	nanotime(&now);
	nfsm_chain_get_32(error, nmc, val);
	switch (val) {
	case NFS_TIME_SET_TO_CLIENT:
		nfsm_chain_get_time(error, nmc, nd->nd_vers,
		    vap->va_access_time.tv_sec,
		    vap->va_access_time.tv_nsec);
		VATTR_SET_ACTIVE(vap, va_access_time);
		vap->va_vaflags &= ~VA_UTIMES_NULL;
		break;
	case NFS_TIME_SET_TO_SERVER:
		VATTR_SET(vap, va_access_time, now);
		vap->va_vaflags |= VA_UTIMES_NULL;
		break;
	}
	nfsm_chain_get_32(error, nmc, val);
	switch (val) {
	case NFS_TIME_SET_TO_CLIENT:
		nfsm_chain_get_time(error, nmc, nd->nd_vers,
		    vap->va_modify_time.tv_sec,
		    vap->va_modify_time.tv_nsec);
		VATTR_SET_ACTIVE(vap, va_modify_time);
		vap->va_vaflags &= ~VA_UTIMES_NULL;
		break;
	case NFS_TIME_SET_TO_SERVER:
		VATTR_SET(vap, va_modify_time, now);
		if (!VATTR_IS_ACTIVE(vap, va_access_time)) {
			vap->va_vaflags |= VA_UTIMES_NULL;
		}
		break;
	}

	return error;
}

  
                                      
   
int
nfsrv_cmp_secflavs(struct nfs_sec *sf1, struct nfs_sec *sf2)
{
	int i;

	if (sf1->count != sf2->count) {
		return 1;
	}
	for (i = 0; i < sf1->count; i++) {
		if (sf1->flavors[i] != sf2->flavors[i]) {
			return 1;
		}
	}
	return 0;
}

  
                                                                      
                                                                    
   
int
nfsrv_hang_addrlist(struct nfs_export *nx, struct user_nfs_export_args *unxa)
{
	struct nfs_export_net_args nxna;
	struct nfs_netopt *no, *rn_no;
	struct radix_node_head *rnh;
	struct radix_node *rn;
	struct sockaddr *saddr, *smask;
	struct domain *dom;
	size_t i, ss_minsize;
	int error;
	unsigned int net;
	user_addr_t uaddr;
	kauth_cred_t cred;

	uaddr = unxa->nxa_nets;
	ss_minsize = sizeof(((struct sockaddr_storage *)0)->ss_len) + sizeof(((struct sockaddr_storage *)0)->ss_family);
	for (net = 0; net < unxa->nxa_netcount; net++, uaddr += sizeof(nxna)) {
		error = copyin(uaddr, &nxna, sizeof(nxna));
		if (error) {
			return error;
		}

		if (nxna.nxna_addr.ss_len > sizeof(struct sockaddr_storage) ||
		    (nxna.nxna_addr.ss_len != 0 && nxna.nxna_addr.ss_len < ss_minsize) ||
		    nxna.nxna_mask.ss_len > sizeof(struct sockaddr_storage) ||
		    (nxna.nxna_mask.ss_len != 0 && nxna.nxna_mask.ss_len < ss_minsize) ||
		    nxna.nxna_addr.ss_family > AF_MAX ||
		    nxna.nxna_mask.ss_family > AF_MAX) {
			return EINVAL;
		}

		if (nxna.nxna_flags & (NX_MAPROOT | NX_MAPALL)) {
			struct posix_cred temp_pcred;
			bzero(&temp_pcred, sizeof(temp_pcred));
			temp_pcred.cr_uid = nxna.nxna_cred.cr_uid;
			temp_pcred.cr_ngroups = nxna.nxna_cred.cr_ngroups;
			for (i = 0; i < (size_t)nxna.nxna_cred.cr_ngroups && i < NGROUPS; i++) {
				temp_pcred.cr_groups[i] = nxna.nxna_cred.cr_groups[i];
			}
			cred = posix_cred_create(&temp_pcred);
			if (!IS_VALID_CRED(cred)) {
				return ENOMEM;
			}
		} else {
			cred = NOCRED;
		}

		if (nxna.nxna_addr.ss_len == 0) {
			                                                     
			if (nx->nx_flags & NX_DEFAULTEXPORT) {
				if (IS_VALID_CRED(cred)) {
					kauth_cred_unref(&cred);
				}
				return EEXIST;
			}
			nx->nx_flags |= NX_DEFAULTEXPORT;
			nx->nx_defopt.nxo_flags = nxna.nxna_flags;
			nx->nx_defopt.nxo_cred = cred;
			bcopy(&nxna.nxna_sec, &nx->nx_defopt.nxo_sec, sizeof(struct nfs_sec));
			nx->nx_expcnt++;
			continue;
		}

		i = sizeof(struct nfs_netopt);
		i += nxna.nxna_addr.ss_len + nxna.nxna_mask.ss_len;
		MALLOC(no, struct nfs_netopt *, i, M_NETADDR, M_WAITOK);
		if (!no) {
			if (IS_VALID_CRED(cred)) {
				kauth_cred_unref(&cred);
			}
			return ENOMEM;
		}
		bzero(no, sizeof(struct nfs_netopt));
		no->no_opt.nxo_flags = nxna.nxna_flags;
		no->no_opt.nxo_cred = cred;
		bcopy(&nxna.nxna_sec, &no->no_opt.nxo_sec, sizeof(struct nfs_sec));

		saddr = (struct sockaddr *)(no + 1);
		bcopy(&nxna.nxna_addr, saddr, nxna.nxna_addr.ss_len);
		if (nxna.nxna_mask.ss_len) {
			smask = (struct sockaddr *)((caddr_t)saddr + nxna.nxna_addr.ss_len);
			bcopy(&nxna.nxna_mask, smask, nxna.nxna_mask.ss_len);
		} else {
			smask = NULL;
		}
		sa_family_t family = saddr->sa_family;
		if ((rnh = nx->nx_rtable[family]) == 0) {
			  
                                                          
                                
      
			TAILQ_FOREACH(dom, &domains, dom_entry) {
				if (dom->dom_family == family && dom->dom_rtattach) {
					dom->dom_rtattach((void **)&nx->nx_rtable[family],
					    dom->dom_rtoffset);
					break;
				}
			}
			if ((rnh = nx->nx_rtable[family]) == 0) {
				if (IS_VALID_CRED(cred)) {
					kauth_cred_unref(&cred);
				}
				_FREE(no, M_NETADDR);
				return ENOBUFS;
			}
		}
		rn = (*rnh->rnh_addaddr)((caddr_t)saddr, (caddr_t)smask, rnh, no->no_rnodes);
		if (rn == 0) {
			  
                                                          
                                                          
                                                        
                                                      
     
                                              
      
			int matched = 0;
			rn = (*rnh->rnh_matchaddr)((caddr_t)saddr, rnh);
			rn_no = (struct nfs_netopt *)rn;
			if (rn != 0 && (rn->rn_flags & RNF_ROOT) == 0 &&
			    (rn_no->no_opt.nxo_flags == nxna.nxna_flags) &&
			    (!nfsrv_cmp_secflavs(&rn_no->no_opt.nxo_sec, &nxna.nxna_sec))) {
				kauth_cred_t cred2 = rn_no->no_opt.nxo_cred;
				if (cred == cred2) {
					                                   
					matched = 1;
				} else if (cred && cred2 && (kauth_cred_getuid(cred) == kauth_cred_getuid(cred2))) {
					  
                                     
                               
       
                                          
                                        
                                        
                                          
                                          
                                            
                                      
                        
        
					gid_t groups[NGROUPS];
					gid_t groups2[NGROUPS];
					size_t groupcount = NGROUPS;
					size_t group2count = NGROUPS;

					if (!kauth_cred_getgroups(cred, groups, &groupcount) &&
					    !kauth_cred_getgroups(cred2, groups2, &group2count) &&
					    groupcount == group2count) {
						for (i = 0; i < group2count; i++) {
							if (groups[i] != groups2[i]) {
								break;
							}
						}
						if (i >= group2count || i >= NGROUPS) {
							matched = 1;
						}
					}
				}
			}
			if (IS_VALID_CRED(cred)) {
				kauth_cred_unref(&cred);
			}
			_FREE(no, M_NETADDR);
			if (matched) {
				continue;
			}
			return EPERM;
		}
		nx->nx_expcnt++;
	}

	return 0;
}

  
                                                                       
                                                                         
                             
   
struct nfsrv_free_netopt_arg {
	uint32_t *cnt;
	struct radix_node_head *rnh;
};

int
nfsrv_free_netopt(struct radix_node *rn, void *w)
{
	struct nfsrv_free_netopt_arg *fna = (struct nfsrv_free_netopt_arg *)w;
	struct radix_node_head *rnh = fna->rnh;
	uint32_t *cnt = fna->cnt;
	struct nfs_netopt *nno = (struct nfs_netopt *)rn;

	(*rnh->rnh_deladdr)(rn->rn_key, rn->rn_mask, rnh);
	if (IS_VALID_CRED(nno->no_opt.nxo_cred)) {
		kauth_cred_unref(&nno->no_opt.nxo_cred);
	}
	_FREE((caddr_t)rn, M_NETADDR);
	*cnt -= 1;
	return 0;
}

  
                                                                         
   
int
nfsrv_free_addrlist(struct nfs_export *nx, struct user_nfs_export_args *unxa)
{
	struct nfs_export_net_args nxna;
	struct radix_node_head *rnh;
	struct radix_node *rn;
	struct nfsrv_free_netopt_arg fna;
	struct nfs_netopt *nno;
	size_t ss_minsize;
	user_addr_t uaddr;
	unsigned int net;
	int i, error;

	if (!unxa || !unxa->nxa_netcount) {
		                       
		for (i = 0; i <= AF_MAX; i++) {
			if ((rnh = nx->nx_rtable[i])) {
				fna.rnh = rnh;
				fna.cnt = &nx->nx_expcnt;
				(*rnh->rnh_walktree)(rnh, nfsrv_free_netopt, (caddr_t)&fna);
				_FREE((caddr_t)rnh, M_RTABLE);
				nx->nx_rtable[i] = 0;
			}
		}
		return 0;
	}

	                                       
	uaddr = unxa->nxa_nets;
	ss_minsize = sizeof(((struct sockaddr_storage *)0)->ss_len) + sizeof(((struct sockaddr_storage *)0)->ss_family);
	for (net = 0; net < unxa->nxa_netcount; net++, uaddr += sizeof(nxna)) {
		error = copyin(uaddr, &nxna, sizeof(nxna));
		if (error) {
			return error;
		}

		if (nxna.nxna_addr.ss_len == 0) {
			                                                     
			if (nx->nx_flags & NX_DEFAULTEXPORT) {
				nx->nx_flags &= ~NX_DEFAULTEXPORT;
				if (IS_VALID_CRED(nx->nx_defopt.nxo_cred)) {
					kauth_cred_unref(&nx->nx_defopt.nxo_cred);
				}
				nx->nx_expcnt--;
			}
			continue;
		}

		if (nxna.nxna_addr.ss_len > sizeof(struct sockaddr_storage) ||
		    (nxna.nxna_addr.ss_len != 0 && nxna.nxna_addr.ss_len < ss_minsize) ||
		    nxna.nxna_addr.ss_family > AF_MAX) {
			printf("nfsrv_free_addrlist: invalid socket address (%u)\n", net);
			continue;
		}

		if (nxna.nxna_mask.ss_len > sizeof(struct sockaddr_storage) ||
		    (nxna.nxna_mask.ss_len != 0 && nxna.nxna_mask.ss_len < ss_minsize) ||
		    nxna.nxna_mask.ss_family > AF_MAX) {
			printf("nfsrv_free_addrlist: invalid socket mask (%u)\n", net);
			continue;
		}

		if ((rnh = nx->nx_rtable[nxna.nxna_addr.ss_family]) == 0) {
			                         
			if (!(unxa->nxa_flags & NXA_ADD)) {
				printf("nfsrv_free_addrlist: address not found (0)\n");
			}
			continue;
		}

		rn = (*rnh->rnh_lookup)(&nxna.nxna_addr,
		    nxna.nxna_mask.ss_len ? &nxna.nxna_mask : NULL, rnh);
		if (!rn || (rn->rn_flags & RNF_ROOT)) {
			if (!(unxa->nxa_flags & NXA_ADD)) {
				printf("nfsrv_free_addrlist: address not found (1)\n");
			}
			continue;
		}

		(*rnh->rnh_deladdr)(rn->rn_key, rn->rn_mask, rnh);
		nno = (struct nfs_netopt *)rn;
		if (IS_VALID_CRED(nno->no_opt.nxo_cred)) {
			kauth_cred_unref(&nno->no_opt.nxo_cred);
		}
		_FREE((caddr_t)rn, M_NETADDR);

		nx->nx_expcnt--;
		if (nx->nx_expcnt == ((nx->nx_flags & NX_DEFAULTEXPORT) ? 1 : 0)) {
			                                           
			_FREE((caddr_t)rnh, M_RTABLE);
			nx->nx_rtable[nxna.nxna_addr.ss_family] = 0;
		}
	}

	return 0;
}

void enablequotas(struct mount *mp, vfs_context_t ctx);       

#define DATA_VOLUME_MP "/System/Volumes/Data"                                    

int
nfsrv_export(struct user_nfs_export_args *unxa, vfs_context_t ctx)
{
	int error = 0;
	size_t pathlen, nxfs_pathlen;
	struct nfs_exportfs *nxfs, *nxfs2, *nxfs3;
	struct nfs_export *nx, *nx2, *nx3;
	struct nfs_filehandle nfh;
	struct nameidata mnd, xnd;
	vnode_t mvp = NULL, xvp = NULL;
	mount_t mp = NULL;
	char path[MAXPATHLEN], *nxfs_path;
	char fl_pathbuff[MAXPATHLEN];
	int fl_pathbuff_len = MAXPATHLEN;
	int expisroot;
	size_t datavol_len = strlen(DATA_VOLUME_MP);

	if (unxa->nxa_flags == NXA_CHECK) {
		                                                             
		error = copyinstr(unxa->nxa_fspath, path, MAXPATHLEN, &pathlen);
		if (error) {
			return error;
		}
		NDINIT(&mnd, LOOKUP, OP_LOOKUP, FOLLOW | LOCKLEAF | AUDITVNPATH1,
		    UIO_SYSSPACE, CAST_USER_ADDR_T(path), ctx);
		error = namei(&mnd);
		if (error) {
			return error;
		}
		mvp = mnd.ni_vp;
		mp = vnode_mount(mvp);
		                                              
		if (!vnode_isvroot(mvp)) {
			error = EINVAL;
		}
		                                                 
		if (!error) {
			nfh.nfh_len = NFSV3_MAX_FID_SIZE;
			error = VFS_VPTOFH(mvp, (int*)&nfh.nfh_len, &nfh.nfh_fid[0], NULL);
		}
		if (!error && (nfh.nfh_len > (int)NFSV3_MAX_FID_SIZE)) {
			error = EIO;
		}
		if (!error && !(mp->mnt_vtable->vfc_vfsflags & VFC_VFSREADDIR_EXTENDED)) {
			error = EISDIR;
		}
		vnode_put(mvp);
		nameidone(&mnd);
		return error;
	}

	                                              
	if ((error = vfs_context_suser(ctx))) {
		return error;
	}

	if (unxa->nxa_flags & NXA_DELETE_ALL) {
		                                            
		lck_rw_lock_exclusive(&nfsrv_export_rwlock);
		while ((nxfs = LIST_FIRST(&nfsrv_exports))) {
			mp = vfs_getvfs_by_mntonname(nxfs->nxfs_path);
			if (mp) {
				vfs_clearflags(mp, MNT_EXPORTED);
				mount_iterdrop(mp);
				mp = NULL;
			}
			                                            
			while ((nx = LIST_FIRST(&nxfs->nxfs_exports))) {
				LIST_REMOVE(nx, nx_next);
				LIST_REMOVE(nx, nx_hash);
				                                        
				nfsrv_free_addrlist(nx, NULL);
				nx->nx_flags &= ~NX_DEFAULTEXPORT;
				if (IS_VALID_CRED(nx->nx_defopt.nxo_cred)) {
					kauth_cred_unref(&nx->nx_defopt.nxo_cred);
				}
				                                           
				nfsrv_free_user_list(&nx->nx_user_list);
				FREE(nx->nx_path, M_TEMP);
				FREE(nx, M_TEMP);
			}
			LIST_REMOVE(nxfs, nxfs_next);
			FREE(nxfs->nxfs_path, M_TEMP);
			FREE(nxfs, M_TEMP);
		}
		if (nfsrv_export_hashtbl) {
			                                                     
			FREE(nfsrv_export_hashtbl, M_TEMP);
			nfsrv_export_hashtbl = NULL;
		}
		lck_rw_done(&nfsrv_export_rwlock);
		return 0;
	}

	error = copyinstr(unxa->nxa_fspath, path, MAXPATHLEN, &pathlen);
	if (error) {
		return error;
	}

	lck_rw_lock_exclusive(&nfsrv_export_rwlock);

	                                           
	if (!nfsrv_export_hashtbl) {
		if (nfsrv_export_hash_size <= 0) {
			nfsrv_export_hash_size = NFSRVEXPHASHSZ;
		}
		nfsrv_export_hashtbl = hashinit(nfsrv_export_hash_size, M_TEMP, &nfsrv_export_hash);
	}

	                                                                 
	LIST_FOREACH(nxfs, &nfsrv_exports, nxfs_next) {
		if (nxfs->nxfs_id == unxa->nxa_fsid) {
			break;
		}
	}
	if (nxfs) {
		                                                
		if (strncmp(path, nxfs->nxfs_path, MAXPATHLEN) &&
		    (strncmp(path, DATA_VOLUME_MP, datavol_len) || strncmp(path + datavol_len, nxfs->nxfs_path, MAXPATHLEN - datavol_len))) {
			error = EEXIST;
			goto unlock_out;
		}
		if ((unxa->nxa_flags & (NXA_ADD | NXA_OFFLINE)) == NXA_ADD) {
			                                 
			NDINIT(&mnd, LOOKUP, OP_LOOKUP, FOLLOW | LOCKLEAF | AUDITVNPATH1,
			    UIO_SYSSPACE, CAST_USER_ADDR_T(nxfs->nxfs_path), ctx);
			error = namei(&mnd);
			if (error) {
				goto unlock_out;
			}
			mvp = mnd.ni_vp;
			                                                      
			if (!vnode_isvroot(mvp)) {
				error = EINVAL;
				goto out;
			}
			                                                              
			mp = vfs_getvfs_by_mntonname(nxfs->nxfs_path);
			if (!mp) {
				                                  
				if (vn_getpath_no_firmlink(mvp, fl_pathbuff, &fl_pathbuff_len) == 0 &&
				    fl_pathbuff_len > 0 &&
				    !strncmp(nxfs->nxfs_path, fl_pathbuff, MAXPATHLEN)) {
					mp = vfs_getvfs_by_mntonname(vnode_mount(mvp)->mnt_vfsstat.f_mntonname);
				}
			}
			if (mp) {
				mount_ref(mp, 0);
				mount_iterdrop(mp);
			}
			                                             
			if (mp != vnode_mount(mvp)) {
				error = EINVAL;
				goto out;
			}
		}
	} else {
		                                                  
		if (!(unxa->nxa_flags & NXA_ADD)) {
			error = ENOENT;
			goto unlock_out;
		}

		                                 
		NDINIT(&mnd, LOOKUP, OP_LOOKUP, FOLLOW | LOCKLEAF | AUDITVNPATH1,
		    UIO_SYSSPACE, CAST_USER_ADDR_T(path), ctx);
		error = namei(&mnd);
		if (error) {
			if (!(unxa->nxa_flags & NXA_OFFLINE)) {
				goto unlock_out;
			}
		} else {
			mvp = mnd.ni_vp;
			                                              
			if (!vnode_isvroot(mvp)) {
				                                
				if (!(unxa->nxa_flags & NXA_OFFLINE)) {
					error = EINVAL;
					goto out;
				}
				vnode_put(mvp);
				nameidone(&mnd);
				mvp = NULL;
			} else {
				mp = vnode_mount(mvp);
				mount_ref(mp, 0);

				                                                 
				nfh.nfh_len = NFSV3_MAX_FID_SIZE;
				error = VFS_VPTOFH(mvp, (int*)&nfh.nfh_len, &nfh.nfh_fid[0], NULL);
				if (!error && (nfh.nfh_len > (int)NFSV3_MAX_FID_SIZE)) {
					error = EIO;
				}
				if (!error && !(mp->mnt_vtable->vfc_vfsflags & VFC_VFSREADDIR_EXTENDED)) {
					error = EISDIR;
				}
				if (error) {
					goto out;
				}
			}
		}

		                            
		MALLOC(nxfs, struct nfs_exportfs *, sizeof(struct nfs_exportfs), M_TEMP, M_WAITOK);
		if (!nxfs) {
			error = ENOMEM;
			goto out;
		}
		bzero(nxfs, sizeof(struct nfs_exportfs));
		nxfs->nxfs_id = unxa->nxa_fsid;
		if (mp) {
			nxfs_path = mp->mnt_vfsstat.f_mntonname;
			nxfs_pathlen = sizeof(mp->mnt_vfsstat.f_mntonname);
		} else {
			nxfs_path = path;
			nxfs_pathlen = pathlen;
		}
		MALLOC(nxfs->nxfs_path, char*, nxfs_pathlen, M_TEMP, M_WAITOK);
		if (!nxfs->nxfs_path) {
			FREE(nxfs, M_TEMP);
			error = ENOMEM;
			goto out;
		}
		bcopy(nxfs_path, nxfs->nxfs_path, nxfs_pathlen);
		                                              
		nxfs3 = NULL;
		LIST_FOREACH(nxfs2, &nfsrv_exports, nxfs_next) {
			if (strncmp(nxfs->nxfs_path, nxfs2->nxfs_path, MAXPATHLEN) > 0) {
				break;
			}
			nxfs3 = nxfs2;
		}
		if (nxfs2) {
			LIST_INSERT_BEFORE(nxfs2, nxfs, nxfs_next);
		} else if (nxfs3) {
			LIST_INSERT_AFTER(nxfs3, nxfs, nxfs_next);
		} else {
			LIST_INSERT_HEAD(&nfsrv_exports, nxfs, nxfs_next);
		}

		                                                                       
		if (mp) {
			enablequotas(mp, ctx);
		}
	}

	if (unxa->nxa_exppath) {
		error = copyinstr(unxa->nxa_exppath, path, MAXPATHLEN, &pathlen);
		if (error) {
			goto out;
		}
		LIST_FOREACH(nx, &nxfs->nxfs_exports, nx_next) {
			if (nx->nx_id == unxa->nxa_expid) {
				break;
			}
		}
		if (nx) {
			                                                
			if (strncmp(path, nx->nx_path, MAXPATHLEN)) {
				error = EEXIST;
				goto out;
			}
		} else {
			                                    
			if (!(unxa->nxa_flags & NXA_ADD)) {
				error = ENOENT;
				goto out;
			}
			                          
			MALLOC(nx, struct nfs_export *, sizeof(struct nfs_export), M_TEMP, M_WAITOK);
			if (!nx) {
				error = ENOMEM;
				goto out1;
			}
			bzero(nx, sizeof(struct nfs_export));
			nx->nx_id = unxa->nxa_expid;
			nx->nx_fs = nxfs;
			microtime(&nx->nx_exptime);
			MALLOC(nx->nx_path, char*, pathlen, M_TEMP, M_WAITOK);
			if (!nx->nx_path) {
				error = ENOMEM;
				FREE(nx, M_TEMP);
				nx = NULL;
				goto out1;
			}
			bcopy(path, nx->nx_path, pathlen);
			                                     
			nfsrv_init_user_list(&nx->nx_user_list);
			                                              
			nx3 = NULL;
			LIST_FOREACH(nx2, &nxfs->nxfs_exports, nx_next) {
				if (strncmp(nx->nx_path, nx2->nx_path, MAXPATHLEN) > 0) {
					break;
				}
				nx3 = nx2;
			}
			if (nx2) {
				LIST_INSERT_BEFORE(nx2, nx, nx_next);
			} else if (nx3) {
				LIST_INSERT_AFTER(nx3, nx, nx_next);
			} else {
				LIST_INSERT_HEAD(&nxfs->nxfs_exports, nx, nx_next);
			}
			                      
			LIST_INSERT_HEAD(NFSRVEXPHASH(nxfs->nxfs_id, nx->nx_id), nx, nx_hash);

			  
                                                                    
                                                              
                                                 
      
			error = 0;
			if ((nx3 && !strncmp(nx3->nx_path, nx->nx_path, pathlen - 1) &&
			    (nx3->nx_path[pathlen - 1] == '/')) ||
			    (nx2 && !strncmp(nx2->nx_path, nx->nx_path, strlen(nx2->nx_path)) &&
			    (nx->nx_path[strlen(nx2->nx_path)] == '/'))) {
				error = EINVAL;
			}
			if (!error) {
				                                                              
				expisroot = !nx->nx_path[0] ||
				    ((nx->nx_path[0] == '.') && !nx->nx_path[1]);
				LIST_FOREACH(nx2, &nxfs->nxfs_exports, nx_next) {
					if (expisroot) {
						if (nx2 != nx) {
							break;
						}
					} else if (!nx2->nx_path[0]) {
						break;
					} else if ((nx2->nx_path[0] == '.') && !nx2->nx_path[1]) {
						break;
					}
				}
				if (nx2) {
					error = EINVAL;
				}
			}
			if (error) {
				  
                                                       
                                                       
                                                              
                                                             
                                                           
                                                              
                                                            
       
				printf("nfsrv_export: warning: nested exports: %s/%s\n",
				    nxfs->nxfs_path, nx->nx_path);
				error = 0;
			}
			nx->nx_fh.nfh_xh.nxh_flags = NXHF_INVALIDFH;
		}
		                                     
		if ((nx->nx_fh.nfh_xh.nxh_version != htonl(NFS_FH_VERSION)) ||
		    (nx->nx_fh.nfh_xh.nxh_flags & NXHF_INVALIDFH)) {
			                                           
			nx->nx_fh.nfh_xh.nxh_version = htonl(NFS_FH_VERSION);
			nx->nx_fh.nfh_xh.nxh_fsid = htonl(nx->nx_fs->nxfs_id);
			nx->nx_fh.nfh_xh.nxh_expid = htonl(nx->nx_id);
			nx->nx_fh.nfh_xh.nxh_flags = 0;
			nx->nx_fh.nfh_xh.nxh_reserved = 0;
			nx->nx_fh.nfh_fhp = (u_char*)&nx->nx_fh.nfh_xh;
			bzero(&nx->nx_fh.nfh_fid[0], NFSV2_MAX_FID_SIZE);
			if (mvp) {
				                            
				if (!nx->nx_path[0] || ((nx->nx_path[0] == '.') && !nx->nx_path[1])) {
					                                            
					xvp = mvp;
					vnode_get(xvp);
				} else {
					xnd.ni_cnd.cn_nameiop = LOOKUP;
#if CONFIG_TRIGGERS
					xnd.ni_op = OP_LOOKUP;
#endif
					xnd.ni_cnd.cn_flags = LOCKLEAF;
					xnd.ni_pathlen = (uint32_t)pathlen - 1;                                            
					xnd.ni_cnd.cn_nameptr = xnd.ni_cnd.cn_pnbuf = path;
					xnd.ni_startdir = mvp;
					xnd.ni_usedvp   = mvp;
					xnd.ni_rootdir = rootvnode;
					xnd.ni_cnd.cn_context = ctx;
					while ((error = lookup(&xnd)) == ERECYCLE) {
						xnd.ni_cnd.cn_flags = LOCKLEAF;
						xnd.ni_cnd.cn_nameptr = xnd.ni_cnd.cn_pnbuf;
						xnd.ni_usedvp = xnd.ni_dvp = xnd.ni_startdir = mvp;
					}
					if (error) {
						goto out1;
					}
					xvp = xnd.ni_vp;
				}

				if (vnode_vtype(xvp) != VDIR) {
					error = EINVAL;
					vnode_put(xvp);
					goto out1;
				}

				                      
				nx->nx_fh.nfh_len = NFSV3_MAX_FID_SIZE;
				error = VFS_VPTOFH(xvp, (int*)&nx->nx_fh.nfh_len, &nx->nx_fh.nfh_fid[0], NULL);
				if (!error && (nx->nx_fh.nfh_len > (int)NFSV3_MAX_FID_SIZE)) {
					error = EIO;
				} else {
					nx->nx_fh.nfh_xh.nxh_fidlen = nx->nx_fh.nfh_len;
					nx->nx_fh.nfh_len += sizeof(nx->nx_fh.nfh_xh);
				}

				vnode_put(xvp);
				if (error) {
					goto out1;
				}
			} else {
				nx->nx_fh.nfh_xh.nxh_flags = NXHF_INVALIDFH;
				nx->nx_fh.nfh_xh.nxh_fidlen = 0;
				nx->nx_fh.nfh_len = sizeof(nx->nx_fh.nfh_xh);
			}
		}
	} else {
		nx = NULL;
	}

	                                
	if (unxa->nxa_flags & NXA_DELETE) {
		if (!nx) {
			                                            
			while ((nx = LIST_FIRST(&nxfs->nxfs_exports))) {
				LIST_REMOVE(nx, nx_next);
				LIST_REMOVE(nx, nx_hash);
				                                        
				nfsrv_free_addrlist(nx, NULL);
				nx->nx_flags &= ~NX_DEFAULTEXPORT;
				if (IS_VALID_CRED(nx->nx_defopt.nxo_cred)) {
					kauth_cred_unref(&nx->nx_defopt.nxo_cred);
				}
				                                             
				nfsrv_free_user_list(&nx->nx_user_list);
				FREE(nx->nx_path, M_TEMP);
				FREE(nx, M_TEMP);
			}
			goto out1;
		} else if (!unxa->nxa_netcount) {
			                                        
			nfsrv_free_addrlist(nx, NULL);
			nx->nx_flags &= ~NX_DEFAULTEXPORT;
			if (IS_VALID_CRED(nx->nx_defopt.nxo_cred)) {
				kauth_cred_unref(&nx->nx_defopt.nxo_cred);
			}
		} else {
			                                                     
			error = nfsrv_free_addrlist(nx, unxa);
			if (error) {
				goto out1;
			}
		}
	}
	if (unxa->nxa_flags & NXA_ADD) {
		  
                                                      
                                                             
                   
     
		if (unxa->nxa_flags & NXA_OFFLINE) {
			microtime(&nx->nx_exptime);
		}

		error = nfsrv_hang_addrlist(nx, unxa);
		if (!error && mp) {
			vfs_setflags(mp, MNT_EXPORTED);
		}
	}

out1:
	if (nx && !nx->nx_expcnt) {
		                                  
		LIST_REMOVE(nx, nx_next);
		LIST_REMOVE(nx, nx_hash);
		                                             
		nfsrv_free_user_list(&nx->nx_user_list);
		FREE(nx->nx_path, M_TEMP);
		FREE(nx, M_TEMP);
	}
	if (LIST_EMPTY(&nxfs->nxfs_exports)) {
		                                              
		LIST_REMOVE(nxfs, nxfs_next);
		FREE(nxfs->nxfs_path, M_TEMP);
		FREE(nxfs, M_TEMP);
		if (mp) {
			vfs_clearflags(mp, MNT_EXPORTED);
		}
	}

out:
	if (mvp) {
		vnode_put(mvp);
		nameidone(&mnd);
	}
unlock_out:
	if (mp) {
		mount_drop(mp, 0);
	}
	lck_rw_done(&nfsrv_export_rwlock);
	return error;
}

  
                                                                     
  
                                                                
                     
   
int
nfsrv_check_exports_allow_address(mbuf_t nam)
{
	struct nfs_exportfs             *nxfs;
	struct nfs_export               *nx;
	struct nfs_export_options       *nxo = NULL;

	if (nam == NULL) {
		return EACCES;
	}

	lck_rw_lock_shared(&nfsrv_export_rwlock);
	LIST_FOREACH(nxfs, &nfsrv_exports, nxfs_next) {
		LIST_FOREACH(nx, &nxfs->nxfs_exports, nx_next) {
			                                                           
			if (nx->nx_flags & NX_DEFAULTEXPORT) {
				nxo = &nx->nx_defopt;
			}
			if (nxo || (nxo = nfsrv_export_lookup(nx, nam))) {
				goto found;
			}
		}
	}
found:
	lck_rw_done(&nfsrv_export_rwlock);

	return nxo ? 0 : EACCES;
}

struct nfs_export_options *
nfsrv_export_lookup(struct nfs_export *nx, mbuf_t nam)
{
	struct nfs_export_options *nxo = NULL;
	struct nfs_netopt *no = NULL;
	struct radix_node_head *rnh;
	struct sockaddr *saddr;

	                                      
	if (nam != NULL) {
		saddr = mbuf_data(nam);
		if (saddr->sa_family > AF_MAX) {
			                                            
			return NULL;
		}
		rnh = nx->nx_rtable[saddr->sa_family];
		if (rnh != NULL) {
			no = (struct nfs_netopt *)
			    (*rnh->rnh_matchaddr)((caddr_t)saddr, rnh);
			if (no && no->no_rnodes->rn_flags & RNF_ROOT) {
				no = NULL;
			}
			if (no) {
				nxo = &no->no_opt;
			}
		}
	}
	                                                        
	if ((nxo == NULL) && (nx->nx_flags & NX_DEFAULTEXPORT)) {
		nxo = &nx->nx_defopt;
	}
	return nxo;
}

                                         
struct nfs_export *
nfsrv_fhtoexport(struct nfs_filehandle *nfhp)
{
	struct nfs_exphandle *nxh = (struct nfs_exphandle*)nfhp->nfh_fhp;
	struct nfs_export *nx;
	uint32_t fsid, expid;

	if (!nfsrv_export_hashtbl) {
		return NULL;
	}
	fsid = ntohl(nxh->nxh_fsid);
	expid = ntohl(nxh->nxh_expid);
	nx = NFSRVEXPHASH(fsid, expid)->lh_first;
	for (; nx; nx = LIST_NEXT(nx, nx_hash)) {
		if (nx->nx_fs->nxfs_id != fsid) {
			continue;
		}
		if (nx->nx_id != expid) {
			continue;
		}
		break;
	}
	return nx;
}

struct nfsrv_getvfs_by_mntonname_callback_args {
	const char      *path;                  
	mount_t         mp;                      
};

static int
nfsrv_getvfs_by_mntonname_callback(mount_t mp, void *v)
{
	struct nfsrv_getvfs_by_mntonname_callback_args * const args = v;
	char real_mntonname[MAXPATHLEN];
	int pathbuflen = MAXPATHLEN;
	vnode_t rvp;
	int error;

	error = VFS_ROOT(mp, &rvp, vfs_context_current());
	if (error) {
		goto out;
	}
	error = vn_getpath_ext(rvp, NULLVP, real_mntonname, &pathbuflen,
	    VN_GETPATH_FSENTER | VN_GETPATH_NO_FIRMLINK);
	vnode_put(rvp);
	if (error) {
		goto out;
	}
	if (strcmp(args->path, real_mntonname) == 0) {
		error = vfs_busy(mp, LK_NOWAIT);
		if (error == 0) {
			args->mp = mp;
		}
		return VFS_RETURNED_DONE;
	}
out:
	return VFS_RETURNED;
}

static mount_t
nfsrv_getvfs_by_mntonname(char *path)
{
	struct nfsrv_getvfs_by_mntonname_callback_args args = {
		.path = path,
		.mp = NULL,
	};
	mount_t mp;
	int error;

	mp = vfs_getvfs_by_mntonname(path);
	if (mp) {
		error = vfs_busy(mp, LK_NOWAIT);
		mount_iterdrop(mp);
		if (error) {
			mp = NULL;
		}
	} else if (vfs_iterate(0, nfsrv_getvfs_by_mntonname_callback,
	    &args) == 0) {
		mp = args.mp;
	}
	return mp;
}

  
                                                       
   
int
nfsrv_fhtovp(
	struct nfs_filehandle *nfhp,
	struct nfsrv_descript *nd,
	vnode_t *vpp,
	struct nfs_export **nxp,
	struct nfs_export_options **nxop)
{
	struct nfs_exphandle *nxh = (struct nfs_exphandle*)nfhp->nfh_fhp;
	struct nfs_export_options *nxo;
	u_char *fidp;
	int error;
	struct mount *mp;
	mbuf_t nam = NULL;
	uint32_t v;
	int i, valid;

	*vpp = NULL;
	*nxp = NULL;
	*nxop = NULL;

	if (nd != NULL) {
		nam = nd->nd_nam;
	}

	v = ntohl(nxh->nxh_version);
	if (v != NFS_FH_VERSION) {
		                                      
		return ESTALE;
	}
	if (nfhp->nfh_len > NFSV3_MAX_FH_SIZE) {
		return EBADRPC;
	}
	if (nfhp->nfh_len < (int)sizeof(struct nfs_exphandle)) {
		return ESTALE;
	}
	v = ntohs(nxh->nxh_flags);
	if (v & NXHF_INVALIDFH) {
		return ESTALE;
	}

	*nxp = nfsrv_fhtoexport(nfhp);
	if (!*nxp) {
		return ESTALE;
	}

	                                                                      
	*nxop = nxo = nfsrv_export_lookup(*nxp, nam);
	if (nam && (*nxop == NULL)) {
		return EACCES;
	}

	if (nd != NULL) {
		                                                 
		for (i = 0, valid = 0; i < nxo->nxo_sec.count; i++) {
			if (nd->nd_sec == nxo->nxo_sec.flavors[i]) {
				valid = 1;
				break;
			}
		}
		if (!valid) {
			  
                                                         
                                                               
                                                            
                                                          
      
			if (nd->nd_vers == NFS_VER2) {
				if (nd->nd_procnum == NFSV2PROC_GETATTR ||
				    nd->nd_procnum == NFSV2PROC_STATFS) {
					valid = 1;
				}
			}
			if (nd->nd_vers == NFS_VER3) {
				if (nd->nd_procnum == NFSPROC_FSINFO) {
					valid = 1;
				}
			}

			if (!valid) {
				return NFSERR_AUTHERR | AUTH_REJECTCRED;
			}
		}
	}

	if (nxo && (nxo->nxo_flags & NX_OFFLINE)) {
		return (nd == NULL || nd->nd_vers == NFS_VER2) ? ESTALE : NFSERR_TRYLATER;
	}

	                          
	mp = nfsrv_getvfs_by_mntonname((*nxp)->nx_fs->nxfs_path);
	if (!mp) {
		  
                                     
                                                            
     
		return (nd == NULL || nd->nd_vers == NFS_VER2) ? ESTALE : NFSERR_TRYLATER;
	}

	fidp = nfhp->nfh_fhp + sizeof(*nxh);
	error = VFS_FHTOVP(mp, nxh->nxh_fidlen, fidp, vpp, NULL);
	vfs_unbusy(mp);
	if (error) {
		return error;
	}
	                                                       
	if (*vpp == NULL) {
		return ESTALE;
	}
	return 0;
}

  
                                                      
                           
   
int
nfsrv_credcheck(
	struct nfsrv_descript *nd,
	vfs_context_t ctx,
	__unused struct nfs_export *nx,
	struct nfs_export_options *nxo)
{
	if (nxo && nxo->nxo_cred) {
		if ((nxo->nxo_flags & NX_MAPALL) ||
		    ((nxo->nxo_flags & NX_MAPROOT) && !suser(nd->nd_cr, NULL))) {
			kauth_cred_ref(nxo->nxo_cred);
			kauth_cred_unref(&nd->nd_cr);
			nd->nd_cr = nxo->nxo_cred;
		}
	}
	ctx->vc_ucred = nd->nd_cr;
	return 0;
}

  
                                                                 
  
                                                                  
                                                                  
                                                                  
                                                                     
   
int
nfsrv_vptofh(
	struct nfs_export *nx,
	int nfsvers,
	struct nfs_filehandle *dnfhp,
	vnode_t vp,
	vfs_context_t ctx,
	struct nfs_filehandle *nfhp)
{
	int error;
	uint32_t maxfidsize;

	nfhp->nfh_fhp = (u_char*)&nfhp->nfh_xh;
	nfhp->nfh_xh.nxh_version = htonl(NFS_FH_VERSION);
	nfhp->nfh_xh.nxh_fsid = htonl(nx->nx_fs->nxfs_id);
	nfhp->nfh_xh.nxh_expid = htonl(nx->nx_id);
	nfhp->nfh_xh.nxh_flags = 0;
	nfhp->nfh_xh.nxh_reserved = 0;

	if (nfsvers == NFS_VER2) {
		bzero(&nfhp->nfh_fid[0], NFSV2_MAX_FID_SIZE);
	}

	                                                            
	if (dnfhp && nfsrv_fhmatch(dnfhp, &nx->nx_fh)) {
		if (nfsvers == NFS_VER2) {
			nfhp->nfh_len = NFSX_V2FH;
		} else {
			nfhp->nfh_len = sizeof(nfhp->nfh_xh);
		}
		nfhp->nfh_xh.nxh_fidlen = 0;
		nfhp->nfh_xh.nxh_flags = htons(NXHF_INVALIDFH);
		return 0;
	}

	if (nfsvers == NFS_VER2) {
		maxfidsize = NFSV2_MAX_FID_SIZE;
	} else {
		maxfidsize = NFSV3_MAX_FID_SIZE;
	}
	nfhp->nfh_len = maxfidsize;

	error = VFS_VPTOFH(vp, (int*)&nfhp->nfh_len, &nfhp->nfh_fid[0], ctx);
	if (error) {
		return error;
	}
	if (nfhp->nfh_len > maxfidsize) {
		return EOVERFLOW;
	}
	nfhp->nfh_xh.nxh_fidlen = nfhp->nfh_len;
	nfhp->nfh_len += sizeof(nfhp->nfh_xh);
	if ((nfsvers == NFS_VER2) && (nfhp->nfh_len < NFSX_V2FH)) {
		nfhp->nfh_len = NFSX_V2FH;
	}

	return 0;
}

  
                                                       
                                                          
                                   
   
int
nfsrv_fhmatch(struct nfs_filehandle *fh1, struct nfs_filehandle *fh2)
{
	struct nfs_exphandle *nxh1, *nxh2;
	int len1, len2;

	nxh1 = (struct nfs_exphandle *)fh1->nfh_fhp;
	nxh2 = (struct nfs_exphandle *)fh2->nfh_fhp;
	len1 = sizeof(fh1->nfh_xh) + nxh1->nxh_fidlen;
	len2 = sizeof(fh2->nfh_xh) + nxh2->nxh_fidlen;
	if (len1 != len2) {
		return 0;
	}
	if (bcmp(nxh1, nxh2, len1)) {
		return 0;
	}
	return 1;
}

  
                                               
   

  
                                                                                  
                                                                              
  
                                                                                           
                                                                                                 
  
                                           
   
struct nfs_user_stat_node *
nfsrv_get_user_stat_node(struct nfs_active_user_list *list, struct sockaddr *saddr, uid_t uid)
{
	struct nfs_user_stat_node               *unode;
	struct timeval                          now;
	struct nfs_user_stat_hashtbl_head       *head;

	                          
	head = NFS_USER_STAT_HASH(list->user_hashtbl, uid);
	LIST_FOREACH(unode, head, hash_link) {
		if ((uid == unode->uid) && (nfs_sockaddr_cmp(saddr, (struct sockaddr*)&unode->sock) == 0)) {
			                         
			break;
		}
	}

	if (unode) {
		                                                           
		TAILQ_REMOVE(&list->user_lru, unode, lru_link);
		TAILQ_INSERT_TAIL(&list->user_lru, unode, lru_link);

		                       
		microtime(&now);
		unode->tm_last = (uint32_t)now.tv_sec;
		return unode;
	}

	if (saddr->sa_len > sizeof(((struct nfs_user_stat_node *)0)->sock)) {
		                                        
		return NULL;
	}

	if (list->node_count < nfsrv_user_stat_max_nodes) {
		                         
		MALLOC(unode, struct nfs_user_stat_node *, sizeof(struct nfs_user_stat_node),
		    M_TEMP, M_WAITOK | M_ZERO);

		if (!unode) {
			return NULL;
		}

		                          
		OSAddAtomic(1, &nfsrv_user_stat_node_count);
		list->node_count++;
	} else {
		                                           
		unode = TAILQ_FIRST(&list->user_lru);

		if (!unode) {
			return NULL;
		}

		                     
		TAILQ_REMOVE(&list->user_lru, unode, lru_link);
		LIST_REMOVE(unode, hash_link);
	}

	                         
	unode->uid = uid;
	bcopy(saddr, &unode->sock, MIN(saddr->sa_len, sizeof(unode->sock)));
	microtime(&now);
	unode->ops = 0;
	unode->bytes_read = 0;
	unode->bytes_written = 0;
	unode->tm_start = (uint32_t)now.tv_sec;
	unode->tm_last = (uint32_t)now.tv_sec;

	                      
	TAILQ_INSERT_TAIL(&list->user_lru, unode, lru_link);
	LIST_INSERT_HEAD(head, unode, hash_link);

	return unode;
}

void
nfsrv_update_user_stat(struct nfs_export *nx, struct nfsrv_descript *nd, uid_t uid, u_int ops, u_int rd_bytes, u_int wr_bytes)
{
	struct nfs_user_stat_node       *unode;
	struct nfs_active_user_list     *ulist;
	struct sockaddr                 *saddr;

	if ((!nfsrv_user_stat_enabled) || (!nx) || (!nd) || (!nd->nd_nam)) {
		return;
	}

	saddr = (struct sockaddr *)mbuf_data(nd->nd_nam);

	                                                   
	if ((saddr->sa_family != AF_INET) && (saddr->sa_family != AF_INET6)) {
		return;
	}

	ulist = &nx->nx_user_list;

	                               
	lck_mtx_lock(&ulist->user_mutex);

	                       
	unode = nfsrv_get_user_stat_node(ulist, saddr, uid);

	if (!unode) {
		lck_mtx_unlock(&ulist->user_mutex);
		return;
	}

	                     
	unode->ops += ops;
	unode->bytes_read += rd_bytes;
	unode->bytes_written += wr_bytes;

	          
	lck_mtx_unlock(&ulist->user_mutex);
}

                                    
void
nfsrv_init_user_list(struct nfs_active_user_list *ulist)
{
	uint i;

	                        
	TAILQ_INIT(&ulist->user_lru);

	                               
	for (i = 0; i < NFS_USER_STAT_HASH_SIZE; i++) {
		LIST_INIT(&ulist->user_hashtbl[i]);
	}
	ulist->node_count = 0;

	lck_mtx_init(&ulist->user_mutex, &nfsrv_active_user_mutex_group, LCK_ATTR_NULL);
}

                                           
void
nfsrv_free_user_list(struct nfs_active_user_list *ulist)
{
	struct nfs_user_stat_node *unode;

	if (!ulist) {
		return;
	}

	while ((unode = TAILQ_FIRST(&ulist->user_lru))) {
		                          
		TAILQ_REMOVE(&ulist->user_lru, unode, lru_link);
		LIST_REMOVE(unode, hash_link);
		FREE(unode, M_TEMP);

		                          
		OSAddAtomic(-1, &nfsrv_user_stat_node_count);
	}
	ulist->node_count = 0;

	lck_mtx_destroy(&ulist->user_mutex, &nfsrv_active_user_mutex_group);
}

                                                            
void
nfsrv_active_user_list_reclaim(void)
{
	struct nfs_exportfs                     *nxfs;
	struct nfs_export                       *nx;
	struct nfs_active_user_list             *ulist;
	struct nfs_user_stat_hashtbl_head       oldlist;
	struct nfs_user_stat_node               *unode, *unode_next;
	struct timeval                          now;
	long                                    tstale;

	LIST_INIT(&oldlist);

	lck_rw_lock_shared(&nfsrv_export_rwlock);
	microtime(&now);
	tstale = now.tv_sec - nfsrv_user_stat_max_idle_sec;
	LIST_FOREACH(nxfs, &nfsrv_exports, nxfs_next) {
		LIST_FOREACH(nx, &nxfs->nxfs_exports, nx_next) {
			                                                
			ulist = &nx->nx_user_list;
			lck_mtx_lock(&ulist->user_mutex);
			for (unode = TAILQ_FIRST(&ulist->user_lru); unode; unode = unode_next) {
				unode_next = TAILQ_NEXT(unode, lru_link);

				                                    
				if (unode->tm_last >= tstale) {
					break;
				}

				                                           
				TAILQ_REMOVE(&ulist->user_lru, unode, lru_link);
				LIST_REMOVE(unode, hash_link);

				                           
				LIST_INSERT_HEAD(&oldlist, unode, hash_link);

				                          
				OSAddAtomic(-1, &nfsrv_user_stat_node_count);
				ulist->node_count--;
			}
			                                       
			lck_mtx_unlock(&ulist->user_mutex);
		}
	}
	lck_rw_done(&nfsrv_export_rwlock);

	                        
	while ((unode = LIST_FIRST(&oldlist))) {
		LIST_REMOVE(unode, hash_link);
		FREE(unode, M_TEMP);
	}
}

  
                                          
                                                                      
            
   
static u_char nfsrv_v2errmap[] = {
	NFSERR_PERM, NFSERR_NOENT, NFSERR_IO, NFSERR_IO, NFSERR_IO,
	NFSERR_NXIO, NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO,
	NFSERR_IO, NFSERR_IO, NFSERR_ACCES, NFSERR_IO, NFSERR_IO,
	NFSERR_IO, NFSERR_EXIST, NFSERR_IO, NFSERR_NODEV, NFSERR_NOTDIR,
	NFSERR_ISDIR, NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO,
	NFSERR_IO, NFSERR_FBIG, NFSERR_NOSPC, NFSERR_IO, NFSERR_ROFS,
	NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO,
	NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO,
	NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO,
	NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO,
	NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO,
	NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO, NFSERR_IO,
	NFSERR_IO, NFSERR_IO, NFSERR_NAMETOL, NFSERR_IO, NFSERR_IO,
	NFSERR_NOTEMPTY, NFSERR_IO, NFSERR_IO, NFSERR_DQUOT, NFSERR_STALE,
};

  
                                          
                                                                       
                                                                         
                                                                      
                                                                           
                                                                              
                                                  
                                                                         
                                                   
   
static short nfsv3err_null[] = {
	0,
	0,
};

static short nfsv3err_getattr[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_setattr[] = {
	NFSERR_IO,
	NFSERR_PERM,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOT_SYNC,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_lookup[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_NAMETOL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_access[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_readlink[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_read[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_NXIO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_write[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_FBIG,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_create[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_mkdir[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_symlink[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_mknod[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	NFSERR_BADTYPE,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_remove[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_rmdir[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_INVAL,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_NOTEMPTY,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_rename[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_XDEV,
	NFSERR_NOTDIR,
	NFSERR_ISDIR,
	NFSERR_INVAL,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_MLINK,
	NFSERR_NAMETOL,
	NFSERR_NOTEMPTY,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_link[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_XDEV,
	NFSERR_NOTDIR,
	NFSERR_INVAL,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_MLINK,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_readdir[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_BAD_COOKIE,
	NFSERR_TOOSMALL,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_readdirplus[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_BAD_COOKIE,
	NFSERR_NOTSUPP,
	NFSERR_TOOSMALL,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_fsstat[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_fsinfo[] = {
	NFSERR_STALE,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_pathconf[] = {
	NFSERR_STALE,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short nfsv3err_commit[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_TRYLATER,
	0,
};

static short *nfsrv_v3errmap[] = {
	nfsv3err_null,
	nfsv3err_getattr,
	nfsv3err_setattr,
	nfsv3err_lookup,
	nfsv3err_access,
	nfsv3err_readlink,
	nfsv3err_read,
	nfsv3err_write,
	nfsv3err_create,
	nfsv3err_mkdir,
	nfsv3err_symlink,
	nfsv3err_mknod,
	nfsv3err_remove,
	nfsv3err_rmdir,
	nfsv3err_rename,
	nfsv3err_link,
	nfsv3err_readdir,
	nfsv3err_readdirplus,
	nfsv3err_fsstat,
	nfsv3err_fsinfo,
	nfsv3err_pathconf,
	nfsv3err_commit,
};

  
                                                                       
                                                      
   
int
nfsrv_errmap(struct nfsrv_descript *nd, int err)
{
	short *defaulterrp, *errp;

	if (nd->nd_vers == NFS_VER2) {
		if (err <= (int)sizeof(nfsrv_v2errmap)) {
			return (int)nfsrv_v2errmap[err - 1];
		}
		return NFSERR_IO;
	}
	           
	if (nd->nd_procnum > NFSPROC_COMMIT) {
		return err & 0xffff;
	}
	errp = defaulterrp = nfsrv_v3errmap[nd->nd_procnum];
	while (*++errp) {
		if (*errp == err) {
			return err;
		} else if (*errp > err) {
			break;
		}
	}
	return (int)*defaulterrp;
}

#endif                        

#endif                 
