/*
 * Copyright (c) 2000-2008 Apple Inc. All rights reserved.
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
 *
 *
 * Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved
 *
 *
 * Copyright (c) 1982, 1986, 1989, 1990, 1991, 1993
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
 *	@(#)kern_prot.c	8.9 (Berkeley) 2/14/95
 *
 *
 * NOTICE: This file was modified by McAfee Research in 2004 to introduce
 * support for mandatory and extensible security protections.  This notice
 * is included in support of clause 2.2 (b) of the Apple Public License,
 * Version 2.0.
 *
 *
 * NOTICE: This file was modified by SPARTA, Inc. in 2005 to introduce
 * support for mandatory and extensible security protections.  This notice
 * is included in support of clause 2.2 (b) of the Apple Public License,
 * Version 2.0.
 *
 */

  
                                                   
   

#include <sys/param.h>
#include <sys/acct.h>
#include <sys/systm.h>
#include <sys/ucred.h>
#include <sys/proc_internal.h>
#include <sys/user.h>
#include <sys/kauth.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/malloc.h>
#include <sys/persona.h>

#include <security/audit/audit.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#include <sys/mount_internal.h>
#include <sys/sysproto.h>
#include <mach/message.h>
#include <mach/host_security.h>

#include <kern/host.h>
#include <kern/task.h>                                  
#include <kern/assert.h>


  
                                                                      
                                                                       
          
  
                                                                   
   
#if DEBUG_CRED
#define DEBUG_CRED_ENTER                printf
#define DEBUG_CRED_CHANGE               printf
#else                    
#define DEBUG_CRED_ENTER(fmt, ...)      do {} while (0)
#define DEBUG_CRED_CHANGE(fmt, ...)     do {} while (0)
#endif                   

#if DEVELOPMENT || DEBUG
extern void task_importance_update_owner_info(task_t);
#endif


  
              
  
                                                                          
                                     
  
                                             
  
                                         
  
                               
   
int
setprivexec(proc_t p, struct setprivexec_args *uap, int32_t *retval)
{
	AUDIT_ARG(value32, uap->flag);
	*retval = p->p_debugger;
	p->p_debugger = (uap->flag != 0);
	return 0;
}


  
         
  
                                  
  
                     
  
                                       
  
                               
   
int
getpid(proc_t p, __unused struct getpid_args *uap, int32_t *retval)
{
	*retval = p->p_pid;
	return 0;
}


  
          
  
                                         
  
                     
  
                                      
  
                               
   
int
getppid(proc_t p, __unused struct getppid_args *uap, int32_t *retval)
{
	*retval = p->p_ppid;
	return 0;
}


  
          
  
                                                               
  
                     
  
                                     
  
                               
   
int
getpgrp(proc_t p, __unused struct getpgrp_args *uap, int32_t *retval)
{
	*retval = p->p_pgrpid;
	return 0;
}


  
          
  
                                                      
  
                                        
  
                        
                            
  
                                                                      
                                                              
                                           
  
                               
   
int
getpgid(proc_t p, struct getpgid_args *uap, int32_t *retval)
{
	proc_t pt;
	int refheld = 0;

	pt = p;
	if (uap->pid == 0) {
		goto found;
	}

	if ((pt = proc_find(uap->pid)) == 0) {
		return ESRCH;
	}
	refheld = 1;
found:
	*retval = pt->p_pgrpid;
	if (refheld != 0) {
		proc_rele(pt);
	}
	return 0;
}


  
         
  
                                                                      
  
                                        
  
                        
                            
  
                                                                      
                                                              
                                           
  
                               
   
int
getsid(proc_t p, struct getsid_args *uap, int32_t *retval)
{
	proc_t pt;
	int refheld = 0;
	struct session * sessp;

	pt = p;
	if (uap->pid == 0) {
		goto found;
	}

	if ((pt = proc_find(uap->pid)) == 0) {
		return ESRCH;
	}
	refheld = 1;
found:
	sessp = proc_session(pt);
	*retval = sessp->s_sid;
	session_rele(sessp);

	if (refheld != 0) {
		proc_rele(pt);
	}
	return 0;
}


  
         
  
                                           
  
                     
  
                                               
   
int
getuid(__unused proc_t p, __unused struct getuid_args *uap, int32_t *retval)
{
	*retval = kauth_getruid();
	return 0;
}


  
          
  
                                                
  
                     
  
                                                    
   
int
geteuid(__unused proc_t p, __unused struct geteuid_args *uap, int32_t *retval)
{
	*retval = kauth_getuid();
	return 0;
}


  
         
  
                                                        
  
                                                      
                                           
  
                        
                                          
   
int
gettid(__unused proc_t p, struct gettid_args *uap, int32_t *retval)
{
	struct uthread *uthread = get_bsdthread_info(current_thread());
	int     error;

	  
                                                                     
                                                         
    
	if (!(uthread->uu_flag & UT_SETUID)) {
		return ESRCH;
	}

	if ((error = suword(uap->uidp, kauth_cred_getruid(uthread->uu_ucred)))) {
		return error;
	}
	if ((error = suword(uap->gidp, kauth_cred_getrgid(uthread->uu_ucred)))) {
		return error;
	}

	*retval = 0;
	return 0;
}


  
         
  
                                                             
  
                     
  
                                               
   
int
getgid(__unused proc_t p, __unused struct getgid_args *uap, int32_t *retval)
{
	*retval = kauth_getrgid();
	return 0;
}


  
          
  
                                                                  
  
                     
  
                                                    
  
                                                                     
                                                      
  
                                                                  
           
   
int
getegid(__unused proc_t p, __unused struct getegid_args *uap, int32_t *retval)
{
	*retval = kauth_getgid();
	return 0;
}


  
            
  
                                                                            
  
                                                            
                                        
  
                        
                                   
                                        
  
                      
                     
  
                                                                      
                                                              
                                                              
                                                               
  
                                                                 
                                                                
                                                              
                                                             
                                 
  
                                                               
                                                               
                             
   
int
getgroups(__unused proc_t p, struct getgroups_args *uap, int32_t *retval)
{
	int ngrp;
	int error;
	kauth_cred_t cred;
	posix_cred_t pcred;

	                                                             
	cred = kauth_cred_get_with_ref();
	pcred = posix_cred_get(cred);

	if ((ngrp = uap->gidsetsize) == 0) {
		*retval = pcred->cr_ngroups;
		kauth_cred_unref(&cred);
		return 0;
	}
	if (ngrp < pcred->cr_ngroups) {
		kauth_cred_unref(&cred);
		return EINVAL;
	}
	ngrp = pcred->cr_ngroups;
	if ((error = copyout((caddr_t)pcred->cr_groups,
	    uap->gidset,
	    ngrp * sizeof(gid_t)))) {
		kauth_cred_unref(&cred);
		return error;
	}
	kauth_cred_unref(&cred);
	*retval = ngrp;
	return 0;
}


  
                                                               
  
                           
  
   

int
getsgroups(__unused proc_t p, __unused struct getsgroups_args *uap, __unused int32_t *retval)
{
	return ENOTSUP;
}

  
                                                          
  
                           
  
   

int
getwgroups(__unused proc_t p, __unused struct getwgroups_args *uap, __unused int32_t *retval)
{
	return ENOTSUP;
}

  
                  
  
                                                
   
int
setsid_internal(proc_t p)
{
	struct pgrp * pg = PGRP_NULL;

	if (p->p_pgrpid == p->p_pid || (pg = pgfind(p->p_pid)) || p->p_lflag & P_LINVFORK) {
		if (pg != PGRP_NULL) {
			pg_rele(pg);
		}
		return EPERM;
	} else {
		                                                 
		(void)enterpgrp(p, p->p_pid, 1);
		return 0;
	}
}

  
         
  
                                                                        
              
  
                     
  
                        
                              
  
                                                                       
                                                            
                                                            
                                                           
                                                              
                                                
  
                                                             
                                                             
                                         
  
                               
   
int
setsid(proc_t p, __unused struct setsid_args *uap, int32_t *retval)
{
	int rc = setsid_internal(p);
	if (rc == 0) {
		*retval = p->p_pid;
	}
	return rc;
}


  
          
  
                                                    
  
                                           
                                               
  
                       
                                               
                 
                                    
                                          
                             
                                                 
                                     
                                                  
                                              
                                         
                                             
           
  
                                                                    
                                                              
                                                              
                                                             
                             
  
                                                             
                                                               
                                           
  
                                                                     
                                                       
  
                               
   
int
setpgid(proc_t curp, struct setpgid_args *uap, __unused int32_t *retval)
{
	proc_t targp = PROC_NULL;                           
	struct pgrp *pg = PGRP_NULL;                     
	int error = 0;
	int refheld = 0;
	int samesess = 0;
	struct session * curp_sessp = SESSION_NULL;
	struct session * targp_sessp = SESSION_NULL;

	curp_sessp = proc_session(curp);

	if (uap->pid != 0 && uap->pid != curp->p_pid) {
		if ((targp = proc_find(uap->pid)) == 0 || !inferior(targp)) {
			if (targp != PROC_NULL) {
				refheld = 1;
			}
			error = ESRCH;
			goto out;
		}
		refheld = 1;
		targp_sessp = proc_session(targp);
		if (targp_sessp != curp_sessp) {
			error = EPERM;
			goto out;
		}
		if (targp->p_flag & P_EXEC) {
			error = EACCES;
			goto out;
		}
	} else {
		targp = curp;
		targp_sessp = proc_session(targp);
	}

	if (SESS_LEADER(targp, targp_sessp)) {
		error = EPERM;
		goto out;
	}
	if (targp_sessp != SESSION_NULL) {
		session_rele(targp_sessp);
		targp_sessp = SESSION_NULL;
	}

	if (uap->pgid < 0) {
		error = EINVAL;
		goto out;
	}
	if (uap->pgid == 0) {
		uap->pgid = targp->p_pid;
	} else if (uap->pgid != targp->p_pid) {
		if ((pg = pgfind(uap->pgid)) == 0) {
			error = EPERM;
			goto out;
		}
		samesess = (pg->pg_session != curp_sessp);
		pg_rele(pg);
		if (samesess != 0) {
			error = EPERM;
			goto out;
		}
	}
	error = enterpgrp(targp, uap->pgid, 0);
out:
	if (targp_sessp != SESSION_NULL) {
		session_rele(targp_sessp);
	}
	if (curp_sessp != SESSION_NULL) {
		session_rele(curp_sessp);
	}
	if (refheld != 0) {
		proc_rele(targp);
	}
	return error;
}


  
            
  
                                                                            
  
                     
  
                            
                
  
                                                                       
                                                                
                                                                   
                                                               
              
   
int
proc_issetugid(proc_t p)
{
	return (p->p_flag & P_SUGID) ? 1 : 0;
}

int
issetugid(proc_t p, __unused struct issetugid_args *uap, int32_t *retval)
{
	  
                                                               
                                                             
                       
                                                                 
                                                              
                                                     
    

	*retval = proc_issetugid(p);
	return 0;
}


  
         
  
                                       
  
                                    
  
                        
                                   
  
                                                                       
                                                          
  
                                                                   
                                                                
                                                                
  
                                                                  
                                                                 
   
int
setuid(proc_t p, struct setuid_args *uap, __unused int32_t *retval)
{
	uid_t uid;
	uid_t svuid = KAUTH_UID_NONE;
	uid_t ruid = KAUTH_UID_NONE;
	uid_t gmuid = KAUTH_UID_NONE;
	int error;
	kauth_cred_t my_cred, my_new_cred;
	posix_cred_t my_pcred;

	uid = uap->uid;

	                                                                       
	my_cred = kauth_cred_proc_ref(p);
	my_pcred = posix_cred_get(my_cred);

	DEBUG_CRED_ENTER("setuid (%d/%d): %p %d\n", p->p_pid, (p->p_pptr ? p->p_pptr->p_pid : 0), my_cred, uap->uid);
	AUDIT_ARG(uid, uid);

	for (;;) {
		if (uid != my_pcred->cr_ruid &&                                     
		    uid != my_pcred->cr_svuid &&                                     
		    (error = suser(my_cred, &p->p_acflag))) {
			kauth_cred_unref(&my_cred);
			return error;
		}

		  
                                                               
                                          
     
		if (suser(my_cred, &p->p_acflag) == 0) {
			svuid = uid;
			ruid = uid;
		} else {
			svuid = KAUTH_UID_NONE;
			ruid = KAUTH_UID_NONE;
		}
		  
                                                               
                                                                
                                                                 
                                                                 
                                                                
                                                              
                                                        
     
		if (!(my_pcred->cr_flags & CRF_NOMEMBERD)) {
			gmuid = uid;
		}

		  
                                                              
                                                              
                                                         
                                                              
                                                      
     
		my_new_cred = kauth_cred_setresuid(my_cred, ruid, uid, svuid, gmuid);
		if (my_cred != my_new_cred) {
			DEBUG_CRED_CHANGE("setuid CH(%d): %p/0x%08x -> %p/0x%08x\n", p->p_pid, my_cred, my_pcred->cr_flags, my_new_cred, posix_cred_get(my_new_cred)->cr_flags);

			  
                                                                                                                
                                                                                                          
                                                                                                                            
                                                                                                                    
                                                                                                       
                                                                                                                   
      
			if (ruid != KAUTH_UID_NONE && !proc_has_persona(p)) {
				(void)chgproccnt(ruid, 1);
			}

			proc_ucred_lock(p);
			  
                                                        
                                                   
                                                       
                                           
     
                                                                                                                                       
      
			if (p->p_ucred != my_cred) {
				proc_ucred_unlock(p);
				  
                                                                  
                                                     
       
				if (ruid != KAUTH_UID_NONE && !proc_has_persona(p)) {
					(void)chgproccnt(ruid, -1);
				}
				kauth_cred_unref(&my_new_cred);
				my_cred = kauth_cred_proc_ref(p);
				my_pcred = posix_cred_get(my_cred);
				               
				continue;
			}
			p->p_ucred = my_new_cred;
			                         
			PROC_UPDATE_CREDS_ONPROC(p);

			OSBitOrAtomic(P_SUGID, &p->p_flag);
			proc_ucred_unlock(p);
			  
                                                                     
                             
      
			if (ruid != KAUTH_UID_NONE && !proc_has_persona(p)) {
				(void)chgproccnt(my_pcred->cr_ruid, -1);
			}
		}
		break;
	}
	                                                    
	kauth_cred_unref(&my_cred);

	set_security_token(p);
	return 0;
}


  
          
  
                                                 
  
                                               
  
                        
                                   
  
                                                              
                                                               
                                                            
                                                      
  
                                                                  
                                                                 
   
int
seteuid(proc_t p, struct seteuid_args *uap, __unused int32_t *retval)
{
	uid_t euid;
	int error;
	kauth_cred_t my_cred, my_new_cred;
	posix_cred_t my_pcred;

	DEBUG_CRED_ENTER("seteuid: %d\n", uap->euid);

	euid = uap->euid;
	AUDIT_ARG(euid, euid);

	my_cred = kauth_cred_proc_ref(p);
	my_pcred = posix_cred_get(my_cred);

	for (;;) {
		if (euid != my_pcred->cr_ruid && euid != my_pcred->cr_svuid &&
		    (error = suser(my_cred, &p->p_acflag))) {
			kauth_cred_unref(&my_cred);
			return error;
		}

		  
                                                              
                                                              
                                                         
                                                              
                                                      
     
		my_new_cred = kauth_cred_setresuid(my_cred, KAUTH_UID_NONE, euid, KAUTH_UID_NONE, my_pcred->cr_gmuid);

		if (my_cred != my_new_cred) {
			DEBUG_CRED_CHANGE("seteuid CH(%d): %p/0x%08x -> %p/0x%08x\n", p->p_pid, my_cred, my_pcred->cr_flags, my_new_cred, posix_cred_get(my_new_cred)->cr_flags);

			proc_ucred_lock(p);
			  
                                                        
                                                   
                                                
                                                  
      
			if (p->p_ucred != my_cred) {
				proc_ucred_unlock(p);
				kauth_cred_unref(&my_new_cred);
				my_cred = kauth_cred_proc_ref(p);
				my_pcred = posix_cred_get(my_cred);
				               
				continue;
			}
			p->p_ucred = my_new_cred;
			                         
			PROC_UPDATE_CREDS_ONPROC(p);
			OSBitOrAtomic(P_SUGID, &p->p_flag);
			proc_ucred_unlock(p);
		}
		break;
	}
	                                                    
	kauth_cred_unref(&my_cred);

	set_security_token(p);
	return 0;
}


  
           
  
                                                          
  
                                          
                                    
  
                        
                                   
  
                                                                     
                                                                 
                                            
  
                                                                 
                                              
  
                                                               
                                                               
                                                                
                                                              
  
                                                             
                                                          
                                                           
               
  
                                                                  
                                                                 
   
int
setreuid(proc_t p, struct setreuid_args *uap, __unused int32_t *retval)
{
	uid_t ruid, euid;
	int error;
	kauth_cred_t my_cred, my_new_cred;
	posix_cred_t my_pcred;

	DEBUG_CRED_ENTER("setreuid %d %d\n", uap->ruid, uap->euid);

	ruid = uap->ruid;
	euid = uap->euid;
	if (ruid == (uid_t)-1) {
		ruid = KAUTH_UID_NONE;
	}
	if (euid == (uid_t)-1) {
		euid = KAUTH_UID_NONE;
	}
	AUDIT_ARG(euid, euid);
	AUDIT_ARG(ruid, ruid);

	my_cred = kauth_cred_proc_ref(p);
	my_pcred = posix_cred_get(my_cred);

	for (;;) {
		if (((ruid != KAUTH_UID_NONE &&                                      
		    ruid != my_pcred->cr_ruid &&                               
		    ruid != my_pcred->cr_uid &&                                
		    ruid != my_pcred->cr_svuid) ||                              
		    (euid != KAUTH_UID_NONE &&                                       
		    euid != my_pcred->cr_uid &&                                
		    euid != my_pcred->cr_ruid &&                               
		    euid != my_pcred->cr_svuid)) &&                             
		    (error = suser(my_cred, &p->p_acflag))) {                          
			kauth_cred_unref(&my_cred);
			return error;
		}

		uid_t new_euid;
		uid_t svuid = KAUTH_UID_NONE;

		new_euid = my_pcred->cr_uid;
		  
                                                              
                                                              
                                                         
                                                              
                                                      
     
		if (euid != KAUTH_UID_NONE && my_pcred->cr_uid != euid) {
			                                
			new_euid = euid;
			OSBitOrAtomic(P_SUGID, &p->p_flag);
		}
		  
                                                          
                                                           
                                                         
                        
     
		if (my_pcred->cr_svuid != uap->ruid &&
		    my_pcred->cr_svuid != uap->euid) {
			svuid = new_euid;
			OSBitOrAtomic(P_SUGID, &p->p_flag);
		}

		my_new_cred = kauth_cred_setresuid(my_cred, ruid, euid, svuid, my_pcred->cr_gmuid);

		if (my_cred != my_new_cred) {
			DEBUG_CRED_CHANGE("setreuid CH(%d): %p/0x%08x -> %p/0x%08x\n", p->p_pid, my_cred, my_pcred->cr_flags, my_new_cred, posix_cred_get(my_new_cred)->cr_flags);

			  
                                                                                                                
                                                                                                          
                                                                                                                            
                                                                                                                    
                                                                                                       
                                                                                                                   
      
			if (ruid != KAUTH_UID_NONE && !proc_has_persona(p)) {
				(void)chgproccnt(ruid, 1);
			}

			proc_ucred_lock(p);
			  
                                                        
                                                   
                                                       
                                           
     
                                                                                                                                       
      
			if (p->p_ucred != my_cred) {
				proc_ucred_unlock(p);
				if (ruid != KAUTH_UID_NONE && !proc_has_persona(p)) {
					  
                                                                   
                                                      
        
					(void)chgproccnt(ruid, -1);
				}
				kauth_cred_unref(&my_new_cred);
				my_cred = kauth_cred_proc_ref(p);
				my_pcred = posix_cred_get(my_cred);
				               
				continue;
			}

			p->p_ucred = my_new_cred;
			                         
			PROC_UPDATE_CREDS_ONPROC(p);
			OSBitOrAtomic(P_SUGID, &p->p_flag);
			proc_ucred_unlock(p);

			if (ruid != KAUTH_UID_NONE && !proc_has_persona(p)) {
				  
                                                                         
                              
       
				(void)chgproccnt(my_pcred->cr_ruid, -1);
			}
		}
		break;
	}
	                                                    
	kauth_cred_unref(&my_cred);

	set_security_token(p);
	return 0;
}


  
         
  
                                        
  
                                    
  
                        
                                   
  
                                                                       
                                                          
  
                                                                   
                                                                
                                                                
  
                                                                  
                                                                 
  
                                                               
                                                          
                                                               
                                           
   
int
setgid(proc_t p, struct setgid_args *uap, __unused int32_t *retval)
{
	gid_t gid;
	gid_t rgid = KAUTH_GID_NONE;
	gid_t svgid = KAUTH_GID_NONE;
	int error;
	kauth_cred_t my_cred, my_new_cred;
	posix_cred_t my_pcred;

	DEBUG_CRED_ENTER("setgid(%d/%d): %d\n", p->p_pid, (p->p_pptr ? p->p_pptr->p_pid : 0), uap->gid);

	gid = uap->gid;
	AUDIT_ARG(gid, gid);

	                                                                       
	my_cred = kauth_cred_proc_ref(p);
	my_pcred = posix_cred_get(my_cred);

	for (;;) {
		if (gid != my_pcred->cr_rgid &&                                     
		    gid != my_pcred->cr_svgid &&                                     
		    (error = suser(my_cred, &p->p_acflag))) {
			kauth_cred_unref(&my_cred);
			return error;
		}

		  
                                                               
                                          
     
		if (suser(my_cred, &p->p_acflag) == 0) {
			svgid = gid;
			rgid = gid;
		} else {
			svgid = KAUTH_GID_NONE;
			rgid = KAUTH_GID_NONE;
		}

		  
                                                              
                                                              
                                                         
                                                              
                                                      
     
		my_new_cred = kauth_cred_setresgid(my_cred, rgid, gid, svgid);
		if (my_cred != my_new_cred) {
			DEBUG_CRED_CHANGE("setgid(CH)%d: %p/0x%08x->%p/0x%08x\n", p->p_pid, my_cred, my_cred->cr_flags, my_new_cred, my_new_cred->cr_flags);

			proc_ucred_lock(p);
			  
                                                        
                                                   
                                                
                                                  
      
			if (p->p_ucred != my_cred) {
				proc_ucred_unlock(p);
				kauth_cred_unref(&my_new_cred);
				               
				my_cred = kauth_cred_proc_ref(p);
				my_pcred = posix_cred_get(my_cred);
				continue;
			}
			p->p_ucred = my_new_cred;
			                         
			PROC_UPDATE_CREDS_ONPROC(p);
			OSBitOrAtomic(P_SUGID, &p->p_flag);
			proc_ucred_unlock(p);
		}
		break;
	}
	                                                    
	kauth_cred_unref(&my_cred);

	set_security_token(p);
	return 0;
}


  
          
  
                                                  
  
                                               
  
                        
              
  
                                                              
                                                               
                                                            
                                                      
  
                                                                  
                                                                 
  
                                                               
                                                          
                                                               
                                           
   
int
setegid(proc_t p, struct setegid_args *uap, __unused int32_t *retval)
{
	gid_t egid;
	int error;
	kauth_cred_t my_cred, my_new_cred;
	posix_cred_t my_pcred;

	DEBUG_CRED_ENTER("setegid %d\n", uap->egid);

	egid = uap->egid;
	AUDIT_ARG(egid, egid);

	                                                                       
	my_cred = kauth_cred_proc_ref(p);
	my_pcred = posix_cred_get(my_cred);


	for (;;) {
		if (egid != my_pcred->cr_rgid &&
		    egid != my_pcred->cr_svgid &&
		    (error = suser(my_cred, &p->p_acflag))) {
			kauth_cred_unref(&my_cred);
			return error;
		}
		  
                                                              
                                                              
                                                         
                                                              
                                                      
     
		my_new_cred = kauth_cred_setresgid(my_cred, KAUTH_GID_NONE, egid, KAUTH_GID_NONE);
		if (my_cred != my_new_cred) {
			DEBUG_CRED_CHANGE("setegid(CH)%d: %p/0x%08x->%p/0x%08x\n", p->p_pid, my_cred, my_pcred->cr_flags, my_new_cred, posix_cred_get(my_new_cred)->cr_flags);

			proc_ucred_lock(p);
			  
                                                        
                                                   
                                                
                                                  
      
			if (p->p_ucred != my_cred) {
				proc_ucred_unlock(p);
				kauth_cred_unref(&my_new_cred);
				               
				my_cred = kauth_cred_proc_ref(p);
				my_pcred = posix_cred_get(my_cred);
				continue;
			}
			p->p_ucred = my_new_cred;
			                         
			PROC_UPDATE_CREDS_ONPROC(p);
			OSBitOrAtomic(P_SUGID, &p->p_flag);
			proc_ucred_unlock(p);
		}
		break;
	}

	                                                    
	kauth_cred_unref(&my_cred);

	set_security_token(p);
	return 0;
}

  
           
  
                                                           
  
                                          
                                    
  
                        
                                   
  
                                                                     
                                                                 
                                            
  
                                                                 
                                              
  
                                                               
                                                               
                                                                
                                                              
  
                                                               
                                                               
                                                         
                                                           
               
  
                                                                  
                                                                 
  
                                                               
                                                          
                                                               
                                           
   
int
setregid(proc_t p, struct setregid_args *uap, __unused int32_t *retval)
{
	gid_t rgid, egid;
	int error;
	kauth_cred_t my_cred, my_new_cred;
	posix_cred_t my_pcred;

	DEBUG_CRED_ENTER("setregid %d %d\n", uap->rgid, uap->egid);

	rgid = uap->rgid;
	egid = uap->egid;

	if (rgid == (uid_t)-1) {
		rgid = KAUTH_GID_NONE;
	}
	if (egid == (uid_t)-1) {
		egid = KAUTH_GID_NONE;
	}
	AUDIT_ARG(egid, egid);
	AUDIT_ARG(rgid, rgid);

	                                                                       
	my_cred = kauth_cred_proc_ref(p);
	my_pcred = posix_cred_get(my_cred);

	for (;;) {
		if (((rgid != KAUTH_UID_NONE &&                                      
		    rgid != my_pcred->cr_rgid &&                               
		    rgid != my_pcred->cr_gid &&                                
		    rgid != my_pcred->cr_svgid) ||                              
		    (egid != KAUTH_UID_NONE &&                                       
		    egid != my_pcred->cr_groups[0] &&                                
		    egid != my_pcred->cr_gid &&                                
		    egid != my_pcred->cr_rgid &&                               
		    egid != my_pcred->cr_svgid)) &&                             
		    (error = suser(my_cred, &p->p_acflag))) {                          
			kauth_cred_unref(&my_cred);
			return error;
		}

		uid_t new_egid = my_pcred->cr_gid;
		uid_t new_rgid = my_pcred->cr_rgid;
		uid_t svgid = KAUTH_UID_NONE;


		  
                                                              
                                                              
                                                         
                                                              
                                                      
     
		if (egid != KAUTH_UID_NONE && my_pcred->cr_gid != egid) {
			                                
			new_egid = egid;
			OSBitOrAtomic(P_SUGID, &p->p_flag);
		}
		if (rgid != KAUTH_UID_NONE && my_pcred->cr_rgid != rgid) {
			                           
			new_rgid = rgid;
			OSBitOrAtomic(P_SUGID, &p->p_flag);
		}
		  
                                                          
                                                           
                                                         
                        
     
		if (my_pcred->cr_svgid != uap->rgid &&
		    my_pcred->cr_svgid != uap->egid) {
			svgid = new_egid;
			OSBitOrAtomic(P_SUGID, &p->p_flag);
		}

		my_new_cred = kauth_cred_setresgid(my_cred, rgid, egid, svgid);
		if (my_cred != my_new_cred) {
			DEBUG_CRED_CHANGE("setregid(CH)%d: %p/0x%08x->%p/0x%08x\n", p->p_pid, my_cred, my_pcred->cr_flags, my_new_cred, posix_cred_get(my_new_cred)->cr_flags);

			proc_ucred_lock(p);
			                                                  
                                                   
                                                
                                                  
      
			if (p->p_ucred != my_cred) {
				proc_ucred_unlock(p);
				kauth_cred_unref(&my_new_cred);
				               
				my_cred = kauth_cred_proc_ref(p);
				my_pcred = posix_cred_get(my_cred);
				continue;
			}
			p->p_ucred = my_new_cred;
			                         
			PROC_UPDATE_CREDS_ONPROC(p);
			OSBitOrAtomic(P_SUGID, &p->p_flag);                     
			proc_ucred_unlock(p);
		}
		break;
	}
	                                                    
	kauth_cred_unref(&my_cred);

	set_security_token(p);
	return 0;
}


  
                                                                        
                                                                        
                                                                         
                                                                         
                                                                          
                                                                         
   
int
settid(proc_t p, struct settid_args *uap, __unused int32_t *retval)
{
	kauth_cred_t uc;
	struct uthread *uthread = get_bsdthread_info(current_thread());
	uid_t uid;
	gid_t gid;

	uid = uap->uid;
	gid = uap->gid;
	AUDIT_ARG(uid, uid);
	AUDIT_ARG(gid, gid);

	if (proc_suser(p) != 0) {
		return EPERM;
	}

	if (uid == KAUTH_UID_NONE) {
		                                                                       
		if ((uthread->uu_flag & UT_SETUID) == 0) {
			return EPERM;
		}

		                                                     
		uc = kauth_cred_proc_ref(p);
		kauth_cred_unref(&uthread->uu_ucred);
		uthread->uu_ucred = uc;
		uthread->uu_flag &= ~UT_SETUID;
	} else {
		kauth_cred_t my_cred, my_new_cred;

		                                                 
		if ((uthread->uu_flag & UT_SETUID) != 0) {
			return EPERM;
		}

		  
                                                           
                                                          
                                                        
                                                           
                                             
     
		kauth_cred_ref(uthread->uu_ucred);
		my_cred = uthread->uu_ucred;
		my_new_cred = kauth_cred_setuidgid(my_cred, uid, gid);
		if (my_cred != my_new_cred) {
			uthread->uu_ucred = my_new_cred;
		}
		uthread->uu_flag |= UT_SETUID;

		                                                       
		kauth_cred_unref(&my_cred);
	}
	  
                                                                  
              
                                                                 
                                          
    
	return 0;
}


  
                                                                              
                                                                              
                          
  
                                                                            
                                                           
  
                                                                          
   
int
settid_with_pid(proc_t p, struct settid_with_pid_args *uap, __unused int32_t *retval)
{
	proc_t target_proc;
	struct uthread *uthread = get_bsdthread_info(current_thread());
	kauth_cred_t my_cred, my_target_cred, my_new_cred;
	posix_cred_t my_target_pcred;

	AUDIT_ARG(pid, uap->pid);
	AUDIT_ARG(value32, uap->assume);

	if (proc_suser(p) != 0) {
		return EPERM;
	}

	  
                                                                  
              
                                                                 
                                          
    

	  
                                                                           
                                  
    
	if (uap->assume != 0) {
		                                                          
		if ((uthread->uu_flag & UT_SETUID) != 0) {
			return EPERM;
		}

		target_proc = proc_find(uap->pid);
		                                                     
		if (target_proc == NULL || target_proc == kernproc) {
			if (target_proc != NULL) {
				proc_rele(target_proc);
			}
			return ESRCH;
		}

		  
                                                          
                                                        
                                                       
                                                       
                                         
    
                                                             
                                                              
                                                             
                                                                
     
		kauth_cred_ref(uthread->uu_ucred);
		my_cred = uthread->uu_ucred;
		my_target_cred = kauth_cred_proc_ref(target_proc);
		my_target_pcred = posix_cred_get(my_target_cred);
		my_new_cred = kauth_cred_setuidgid(my_cred, my_target_pcred->cr_uid, my_target_pcred->cr_gid);
		if (my_cred != my_new_cred) {
			uthread->uu_ucred = my_new_cred;
		}

		uthread->uu_flag |= UT_SETUID;

		                                                       
		proc_rele(target_proc);
		kauth_cred_unref(&my_cred);
		kauth_cred_unref(&my_target_cred);

		return 0;
	}

	  
                                                                      
                                                                    
                         
    
	if ((uthread->uu_flag & UT_SETUID) == 0) {
		return EPERM;
	}

	                                                     
	my_new_cred = kauth_cred_proc_ref(p);
	kauth_cred_unref(&uthread->uu_ucred);
	uthread->uu_ucred = my_new_cred;
	uthread->uu_flag &= ~UT_SETUID;

	return 0;
}


  
             
  
                                                                             
                
  
                                                   
                                   
                                        
  
                        
                                  
                                      
                                               
                 
  
                                                                       
                                                               
                                                               
                                                             
                                                                
                                                             
  
                                                              
                                                                 
                                                               
                                                                 
                   
  
                                                               
                                                    
  
                                                                  
                                                                 
   
static int
setgroups1(proc_t p, u_int ngrp, user_addr_t gidset, uid_t gmuid, __unused int32_t *retval)
{
	gid_t   newgroups[NGROUPS] = { 0 };
	int     error;

	DEBUG_CRED_ENTER("setgroups1 (%d/%d): %d 0x%016x %d\n", p->p_pid,
	    (p->p_pptr ? p->p_pptr->p_pid : 0), ngrp, gidset, gmuid);

	if (ngrp > NGROUPS) {
		return EINVAL;
	}

	if (ngrp >= 1) {
		error = copyin(gidset,
		    (caddr_t)newgroups, ngrp * sizeof(gid_t));
		if (error) {
			return error;
		}
	}
	return setgroups_internal(p, ngrp, newgroups, gmuid);
}

int
setgroups_internal(proc_t p, u_int ngrp, gid_t *newgroups, uid_t gmuid)
{
	struct uthread *uthread = get_bsdthread_info(current_thread());
	kauth_cred_t my_cred, my_new_cred;
	int     error;

	my_cred = kauth_cred_proc_ref(p);
	if ((error = suser(my_cred, &p->p_acflag))) {
		kauth_cred_unref(&my_cred);
		return error;
	}

	if (ngrp < 1) {
		ngrp = 1;
		newgroups[0] = 0;
	}

	if ((uthread->uu_flag & UT_SETUID) != 0) {
#if DEBUG_CRED
		int my_cred_flags = uthread->uu_ucred->cr_flags;
#endif                  
		kauth_cred_unref(&my_cred);

		  
                                                         
                                                             
                                                               
                                                                 
                                                                 
                                                              
                                                                 
     
		my_cred = uthread->uu_ucred;
		uthread->uu_ucred = kauth_cred_setgroups(my_cred, &newgroups[0], ngrp, gmuid);
#if DEBUG_CRED
		if (my_cred != uthread->uu_ucred) {
			DEBUG_CRED_CHANGE("setgroups1(CH)%d: %p/0x%08x->%p/0x%08x\n", p->p_pid, my_cred, my_cred_flags, uthread->uu_ucred, uthread->uu_ucred->cr_flags);
		}
#endif                  
	} else {
		  
                                                              
            
     
		for (;;) {
			  
                                                       
                                                       
                                                        
                                                  
                                                      
                                     
      
			my_new_cred = kauth_cred_setgroups(my_cred, &newgroups[0], ngrp, gmuid);
			if (my_cred != my_new_cred) {
				DEBUG_CRED_CHANGE("setgroups1(CH)%d: %p/0x%08x->%p/0x%08x\n", p->p_pid, my_cred, my_cred->cr_flags, my_new_cred, my_new_cred->cr_flags);

				proc_ucred_lock(p);
				  
                                                  
                                                  
                                          
                                                
                         
       
				if (p->p_ucred != my_cred) {
					proc_ucred_unlock(p);
					kauth_cred_unref(&my_new_cred);
					my_cred = kauth_cred_proc_ref(p);
					               
					continue;
				}
				p->p_ucred = my_new_cred;
				                         
				PROC_UPDATE_CREDS_ONPROC(p);
				OSBitOrAtomic(P_SUGID, &p->p_flag);
				proc_ucred_unlock(p);
			}
			break;
		}
		                                                    
		AUDIT_ARG(groupset, posix_cred_get(my_cred)->cr_groups, ngrp);
		kauth_cred_unref(&my_cred);


		set_security_token(p);
	}

	return 0;
}


  
             
  
                                                                            
                                                         
  
                                                        
                                       
                         
  
                        
                                      
                                               
                                                  
  
                                                          
  
                                                           
                                                             
                                                               
                                                               
  
                                                              
                                                             
                                                                 
                                                 
  
                         
   
int
initgroups(proc_t p, struct initgroups_args *uap, __unused int32_t *retval)
{
	DEBUG_CRED_ENTER("initgroups\n");

	return setgroups1(p, uap->gidsetsize, uap->gidset, uap->gmuid, retval);
}


  
            
  
                                                                
  
                                                   
                                   
  
                        
                                      
                                               
                                                  
  
                                                             
  
                                                                 
                                                              
                                                               
                                           
  
                                                                 
                      
  
                         
   
int
setgroups(proc_t p, struct setgroups_args *uap, __unused int32_t *retval)
{
	DEBUG_CRED_ENTER("setgroups\n");

	return setgroups1(p, uap->gidsetsize, uap->gidset, KAUTH_UID_NONE, retval);
}


  
                                                            
  
                           
  
   

int
setsgroups(__unused proc_t p, __unused struct setsgroups_args *uap, __unused int32_t *retval)
{
	return ENOTSUP;
}

  
                                                       
  
                           
  
   

int
setwgroups(__unused proc_t p, __unused struct setwgroups_args *uap, __unused int32_t *retval)
{
	return ENOTSUP;
}


  
                                             
  
                                                                           
               
   
int
groupmember(gid_t gid, kauth_cred_t cred)
{
	int is_member;

	if (kauth_cred_ismember_gid(cred, gid, &is_member) == 0 && is_member) {
		return 1;
	}
	return 0;
}


  
                                                            
                                                              
                                  
                      
  
                                                                      
               
  
                                                                    
                                                             
                                                                  
                                                                  
   
int
suser(kauth_cred_t cred, u_short *acflag)
{
#if DIAGNOSTIC
	if (!IS_VALID_CRED(cred)) {
		panic("suser");
	}
#endif
	if (kauth_cred_getuid(cred) == 0) {
		if (acflag) {
			*acflag |= ASU;
		}
		return 0;
	}
	return EPERM;
}


  
           
  
                                             
  
                                                    
                                     
  
                        
                 
  
                                                                     
                                                                 
            
  
                                                             
                                                             
                                          
  
                            
  
                               
   
int
getlogin(proc_t p, struct getlogin_args *uap, __unused int32_t *retval)
{
	char buffer[MAXLOGNAME + 1];
	struct session * sessp;

	bzero(buffer, MAXLOGNAME + 1);

	sessp = proc_session(p);

	if (uap->namelen > MAXLOGNAME) {
		uap->namelen = MAXLOGNAME;
	}

	if (sessp != SESSION_NULL) {
		session_lock(sessp);
		bcopy( sessp->s_login, buffer, uap->namelen);
		session_unlock(sessp);
	}
	session_rele(sessp);

	return copyout((caddr_t)buffer, uap->namebuf, uap->namelen);
}

void
setlogin_internal(proc_t p, const char login[static MAXLOGNAME])
{
	struct session *sessp = proc_session(p);

	if (sessp != SESSION_NULL) {
		session_lock(sessp);
		bcopy(login, sessp->s_login, MAXLOGNAME);
		session_unlock(sessp);
		session_rele(sessp);
	}
}

  
           
  
                               
  
                                                         
  
                        
                                   
                                         
                                                
  
                                                              
  
                               
   
int
setlogin(proc_t p, struct setlogin_args *uap, __unused int32_t *retval)
{
	int error;
	size_t dummy = 0;
	char buffer[MAXLOGNAME + 1];

	if ((error = proc_suser(p))) {
		return error;
	}

	bzero(&buffer[0], MAXLOGNAME + 1);


	error = copyinstr(uap->namebuf,
	    (caddr_t) &buffer[0],
	    MAXLOGNAME - 1, (size_t *)&dummy);

	setlogin_internal(p, buffer);

	if (!error) {
		AUDIT_ARG(text, buffer);
	} else if (error == ENAMETOOLONG) {
		error = EINVAL;
	}
	return error;
}


                                                                   
  
                                                                         
                  
   
int
set_security_token(proc_t p)
{
	return set_security_token_task_internal(p, p->task);
}

static void
proc_calc_audit_token(proc_t p, kauth_cred_t my_cred, audit_token_t *audit_token)
{
	posix_cred_t my_pcred = posix_cred_get(my_cred);

	  
                                                         
                                                      
                                                       
                                                      
                                                      
                                                      
            
    
	audit_token->val[0] = my_cred->cr_audit.as_aia_p->ai_auid;
	audit_token->val[1] = my_pcred->cr_uid;
	audit_token->val[2] = my_pcred->cr_gid;
	audit_token->val[3] = my_pcred->cr_ruid;
	audit_token->val[4] = my_pcred->cr_rgid;
	audit_token->val[5] = p->p_pid;
	audit_token->val[6] = my_cred->cr_audit.as_aia_p->ai_asid;
	audit_token->val[7] = p->p_idversion;
}

  
                                                                
                                                                          
                                      
   

int
set_security_token_task_internal(proc_t p, void *t)
{
	kauth_cred_t my_cred;
	security_token_t sec_token;
	audit_token_t    audit_token;
	host_priv_t host_priv;
	task_t task = t;

	  
                                                                     
                                                                    
                                                                     
                                     
    
	if (task == current_task()) {
		uthread_t        uthread;
		uthread = (uthread_t)get_bsdthread_info(current_thread());
		if (uthread->uu_flag & UT_VFORK) {
			return 1;
		}
	}

	my_cred = kauth_cred_proc_ref(p);

	proc_calc_audit_token(p, my_cred, &audit_token);

	                                                                      
	if (IS_VALID_CRED(my_cred)) {
		sec_token.val[0] = kauth_cred_getuid(my_cred);
		sec_token.val[1] = kauth_cred_getgid(my_cred);
	} else {
		sec_token.val[0] = 0;
		sec_token.val[1] = 0;
	}

	host_priv = (sec_token.val[0]) ? HOST_PRIV_NULL : host_priv_self();
#if CONFIG_MACF
	if (host_priv != HOST_PRIV_NULL && mac_system_check_host_priv(my_cred)) {
		host_priv = HOST_PRIV_NULL;
	}
#endif
	kauth_cred_unref(&my_cred);

#if DEVELOPMENT || DEBUG
	  
                                                          
    
	task_importance_update_owner_info(task);
#endif

	return host_security_set_task_token(host_security_self(),
	           task,
	           sec_token,
	           audit_token,
	           host_priv) != KERN_SUCCESS;
}

void
proc_parent_audit_token(proc_t p, audit_token_t *token_out)
{
	proc_t parent;
	kauth_cred_t my_cred;

	proc_list_lock();

	parent = p->p_pptr;
	my_cred = kauth_cred_proc_ref(parent);
	proc_calc_audit_token(parent, my_cred, token_out);
	kauth_cred_unref(&my_cred);

	proc_list_unlock();
}


int get_audit_token_pid(audit_token_t *audit_token);

int
get_audit_token_pid(audit_token_t *audit_token)
{
	                                                  
	if (audit_token) {
		return (int)audit_token->val[5];
	}
	return -1;
}


  
                                                   
   
__private_extern__
void
cru2x(kauth_cred_t cr, struct xucred *xcr)
{
	posix_cred_t pcr = posix_cred_get(cr);

	bzero(xcr, sizeof(*xcr));
	xcr->cr_version = XUCRED_VERSION;
	xcr->cr_uid = kauth_cred_getuid(cr);
	xcr->cr_ngroups = pcr->cr_ngroups;
	bcopy(pcr->cr_groups, xcr->cr_groups, sizeof(xcr->cr_groups));
}
