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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_acct.c	8.1 (Berkeley) 6/14/93
 */
          
                                          
                     
                                
   
  
                                                                      
                                                                          
                                                                        
               
   


#include <sys/param.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/mount_internal.h>
#include <sys/vnode_internal.h>
#include <sys/file_internal.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/namei.h>
#include <sys/errno.h>
#include <sys/acct.h>
#include <sys/resourcevar.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/sysproto.h>
#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

  
                                                          
                                                                    
                                                   
                  
  
                                                                     
                                                                            
                                                                       
   

  
                                 
                                                                         
                                                   
   
comp_t  encode_comp_t(uint32_t, uint32_t);
void    acctwatch(void *);

  
                                                                            
                  
  
                             
                                                            
                                  
                                  
                                                 
                              
   
struct  vnode *acctp;
struct  vnode *suspend_acctp;

  
                                                           
   
int     acctsuspend = 2;                                                       
int     acctresume = 4;                                                   
int     acctchkfreq = 15;                                                  


static LCK_GRP_DECLARE(acct_subsys_lck_grp, "acct");
static LCK_MTX_DECLARE(acct_subsys_mutex, &acct_subsys_lck_grp);

#define ACCT_SUBSYS_LOCK() lck_mtx_lock(&acct_subsys_mutex)
#define ACCT_SUBSYS_UNLOCK() lck_mtx_unlock(&acct_subsys_mutex)


  
                                                                  
                                                 
   
int
acct(proc_t p, struct acct_args *uap, __unused int *retval)
{
	struct nameidata nd;
	int error;
	struct vfs_context *ctx;

	ctx = vfs_context_current();

	                                        
	if ((error = suser(vfs_context_ucred(ctx), &p->p_acflag))) {
		return error;
	}

	  
                                                                
                                          
    
	if (uap->path != USER_ADDR_NULL) {
		NDINIT(&nd, LOOKUP, OP_OPEN, NOFOLLOW, UIO_USERSPACE, uap->path, ctx);
		if ((error = vn_open(&nd, FWRITE, 0))) {
			return error;
		}
#if CONFIG_MACF
		error = mac_system_check_acct(vfs_context_ucred(ctx), nd.ni_vp);
		if (error) {
			vnode_put(nd.ni_vp);
			vn_close(nd.ni_vp, FWRITE, ctx);
			return error;
		}
#endif
		vnode_put(nd.ni_vp);

		if (nd.ni_vp->v_type != VREG) {
			vn_close(nd.ni_vp, FWRITE, ctx);
			return EACCES;
		}
	}
#if CONFIG_MACF
	else {
		error = mac_system_check_acct(vfs_context_ucred(ctx), NULL);
		if (error) {
			return error;
		}
	}
#endif

	  
                                                                     
                                                              
    
	ACCT_SUBSYS_LOCK();
	if (acctp != NULLVP || suspend_acctp != NULLVP) {
		untimeout(acctwatch, NULL);
		error = vn_close((acctp != NULLVP ? acctp : suspend_acctp),
		    FWRITE, vfs_context_current());

		acctp = suspend_acctp = NULLVP;
	}
	if (uap->path == USER_ADDR_NULL) {
		ACCT_SUBSYS_UNLOCK();
		return error;
	}

	  
                                                            
                       
    
	acctp = nd.ni_vp;
	ACCT_SUBSYS_UNLOCK();

	acctwatch(NULL);
	return error;
}

  
                                                             
                                                         
                                                               
                         
   
int
acct_process(proc_t p)
{
	struct acct an_acct;
	struct rusage rup, *r;
	struct timeval ut, st, tmp;
	int t;
	int error;
	struct vnode *vp;
	kauth_cred_t safecred;
	struct session * sessp;
	struct  tty *tp;

	                                               
	ACCT_SUBSYS_LOCK();
	vp = acctp;
	if (vp == NULLVP) {
		ACCT_SUBSYS_UNLOCK();
		return 0;
	}

	  
                                       
    

	                                          
	bcopy(p->p_comm, an_acct.ac_comm, sizeof an_acct.ac_comm);

	                                                          
	calcru(p, &ut, &st, NULL);
	an_acct.ac_utime = encode_comp_t((uint32_t)ut.tv_sec, ut.tv_usec);
	an_acct.ac_stime = encode_comp_t((uint32_t)st.tv_sec, st.tv_usec);

	                                                                   
	an_acct.ac_btime = (u_int32_t)p->p_start.tv_sec;
	microtime(&tmp);
	timevalsub(&tmp, &p->p_start);
	an_acct.ac_etime = encode_comp_t((uint32_t)tmp.tv_sec, tmp.tv_usec);

	                                           
	proc_lock(p);
	rup = p->p_stats->p_ru;
	proc_unlock(p);
	r = &rup;
	tmp = ut;
	timevaladd(&tmp, &st);
	t = (int)(tmp.tv_sec * hz + tmp.tv_usec / tick);
	if (t) {
		an_acct.ac_mem = (u_int16_t)((r->ru_ixrss + r->ru_idrss + r->ru_isrss) / t);
	} else {
		an_acct.ac_mem = 0;
	}

	                                                
	an_acct.ac_io = encode_comp_t((uint32_t)(r->ru_inblock + r->ru_oublock), 0);

	                                        
	safecred = kauth_cred_proc_ref(p);

	an_acct.ac_uid = kauth_cred_getruid(safecred);
	an_acct.ac_gid = kauth_cred_getrgid(safecred);

	                                                         

	sessp = proc_session(p);
	if ((p->p_flag & P_CONTROLT) && (sessp != SESSION_NULL) && ((tp = SESSION_TP(sessp)) != TTY_NULL)) {
		tty_lock(tp);
		an_acct.ac_tty = tp->t_dev;
		tty_unlock(tp);
	} else {
		an_acct.ac_tty = NODEV;
	}

	if (sessp != SESSION_NULL) {
		session_rele(sessp);
	}

	                                                                      
	an_acct.ac_flag = (u_int8_t)p->p_acflag;

	  
                                                           
    
	if ((error = vnode_getwithref(vp)) == 0) {
		error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&an_acct, sizeof(an_acct),
		    (off_t)0, UIO_SYSSPACE, IO_APPEND | IO_UNIT, safecred,
		    (int *)0, p);
		vnode_put(vp);
	}

	kauth_cred_unref(&safecred);
	ACCT_SUBSYS_UNLOCK();

	return error;
}

  
                                                                
                                                           
                               
   

#define MANTSIZE        13                                            
#define EXPSIZE         3                                                     
#define MAXFRACT        ((1 << MANTSIZE) - 1)                                  

comp_t
encode_comp_t(uint32_t s, uint32_t us)
{
	int exp, rnd;

	exp = 0;
	rnd = 0;
	s *= AHZ;
	s += us / (1000000 / AHZ);                               

	while (s > MAXFRACT) {
		rnd = s & (1 << (EXPSIZE - 1));                
		s >>= EXPSIZE;                                               
		exp++;
	}

	                                                                    
	if (rnd && (++s > MAXFRACT)) {
		s >>= EXPSIZE;
		exp++;
	}

	                                    
	exp <<= MANTSIZE;                                                  
	exp += s;                                                     
	return (comp_t)exp;
}

  
                                                          
                                                               
                                                                
                                                                     
   
              
void
acctwatch(__unused void *a)
{
	vfs_context_t ctx = vfs_context_current();
	struct vfs_attr va;

	VFSATTR_INIT(&va);
	VFSATTR_WANTED(&va, f_blocks);
	VFSATTR_WANTED(&va, f_bavail);

	ACCT_SUBSYS_LOCK();
	if (suspend_acctp != NULLVP) {
		  
                                                              
                                                             
                          
     
		if (suspend_acctp->v_type == VBAD) {
			(void) vn_close(suspend_acctp, FWRITE, vfs_context_kernel());
			suspend_acctp = NULLVP;
			ACCT_SUBSYS_UNLOCK();
			return;
		}
		(void)vfs_getattr(suspend_acctp->v_mount, &va, ctx);
		if (va.f_bavail > acctresume * va.f_blocks / 100) {
			acctp = suspend_acctp;
			suspend_acctp = NULLVP;
			log(LOG_NOTICE, "Accounting resumed\n");
		}
	} else if (acctp != NULLVP) {
		  
                                                               
                                                             
                               
     
		if (acctp->v_type == VBAD) {
			(void) vn_close(acctp, FWRITE, vfs_context_kernel());
			acctp = NULLVP;
			ACCT_SUBSYS_UNLOCK();
			return;
		}
		(void)vfs_getattr(acctp->v_mount, &va, ctx);
		if (va.f_bavail <= acctsuspend * va.f_blocks / 100) {
			suspend_acctp = acctp;
			acctp = NULLVP;
			log(LOG_NOTICE, "Accounting suspended\n");
		}
	} else {
		ACCT_SUBSYS_UNLOCK();
		return;
	}
	ACCT_SUBSYS_UNLOCK();

	timeout(acctwatch, NULL, acctchkfreq * hz);
}
