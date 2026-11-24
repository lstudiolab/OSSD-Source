/*
 * Copyright (c) 2000-2002 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)signalvar.h	8.3 (Berkeley) 1/4/94
 */

#ifndef _SYS_SIGNALVAR_H_                                   
#define _SYS_SIGNALVAR_H_

#include <sys/appleapiopts.h>

#ifdef BSD_KERNEL_PRIVATE

#include <stdatomic.h>

  
                                                 
                                 
   

  
                                                                   
                              
   
struct  sigacts {
	user_addr_t     ps_sigact[NSIG];                                    
	user_addr_t     ps_trampact[NSIG];                                  
	sigset_t ps_catchmask[NSIG];                               
	sigset_t ps_sigonstack;                                          
	sigset_t ps_sigintr;                                                 
	sigset_t ps_sigreset;                                               
	sigset_t ps_signodefer;                                               
	sigset_t ps_siginfo;                                                   
	sigset_t ps_oldmask;                                                 
	user_addr_t ps_sigreturn_token;                                                        
	_Atomic uint32_t ps_sigreturn_validation;                                          
	int     ps_flags;                                        
	int     ps_sig;                                                 
	int     ps_code;                                                
	int     ps_addr;                                                
};

                  
#define SAS_OLDMASK     0x01                                                   
#define SAS_ALTSTACK    0x02                                             

  
                                                                           
                                                                           
                                      
   
#define KERN_SIG_CATCH  CAST_USER_ADDR_T(2)
#define KERN_SIG_HOLD   CAST_USER_ADDR_T(3)
#define KERN_SIG_WAIT   CAST_USER_ADDR_T(4)

                                        
#define PS_SIGRETURN_VALIDATION_DEFAULT 0x0u
#define PS_SIGRETURN_VALIDATION_ENABLED 0x1u
#define PS_SIGRETURN_VALIDATION_DISABLED 0x2u

  
                                                                               
   
#define SIGACTION(p, sig)       (p->p_sigacts->ps_sigact[(sig)])

  
                                                
   
#define SHOULDissignal(p, uthreadp) \
	 (((uthreadp)->uu_siglist)      \
	  & ~((((uthreadp)->uu_sigmask) \
	       | (((p)->p_lflag & P_LTRACED) ? 0 : (p)->p_sigignore)) \
	      & ~sigcantmask))

  
                                            
                                      
                  
   
#define CHECK_SIGNALS(p, thread, uthreadp)      \
	(!thread_should_halt(thread)    \
	 && (SHOULDissignal(p,uthreadp)))

  
                                 
                                                                    
                                         
   
#define SA_KILL         0x01                                               
#define SA_CORE         0x02                                     
#define SA_STOP         0x04                                 
#define SA_TTYSTOP      0x08                                 
#define SA_IGNORE       0x10                                   
#define SA_CONT         0x20                                       
#define SA_CANTMASK     0x40                                         

#ifdef  SIGPROP
int sigprop[NSIG] = {
	0,                                  
	SA_KILL,                            
	SA_KILL,                            
	SA_KILL | SA_CORE,                     
	SA_KILL | SA_CORE,                    
	SA_KILL | SA_CORE,                     
	SA_KILL | SA_CORE,                     
	SA_KILL | SA_CORE,                    
	SA_KILL | SA_CORE,                    
	SA_KILL,                             
	SA_KILL | SA_CORE,                    
	SA_KILL | SA_CORE,                     
	SA_KILL | SA_CORE,                    
	SA_KILL,                             
	SA_KILL,                             
	SA_KILL,                             
	SA_IGNORE,                          
	SA_STOP,                             
	SA_STOP | SA_TTYSTOP,                  
	SA_IGNORE | SA_CONT,                   
	SA_IGNORE,                           
	SA_STOP | SA_TTYSTOP,                  
	SA_STOP | SA_TTYSTOP,                  
	SA_IGNORE,                         
	SA_KILL,                             
	SA_KILL,                             
	SA_KILL,                               
	SA_KILL,                             
	SA_IGNORE,                             
	SA_IGNORE,                           
	SA_KILL,                             
	SA_KILL,                             
};

#define contsigmask     (sigmask(SIGCONT))
#define stopsigmask     (sigmask(SIGSTOP) | sigmask(SIGTSTP) | \
	                    sigmask(SIGTTIN) | sigmask(SIGTTOU))

#endif              

#define sigcantmask     (sigmask(SIGKILL) | sigmask(SIGSTOP))

#define SIGRESTRICTMASK (sigmask(SIGILL) | sigmask(SIGTRAP) | sigmask(SIGABRT) | \
	                 sigmask(SIGFPE) | sigmask(SIGBUS)  | sigmask(SIGSEGV) | \
	                 sigmask(SIGSYS))

extern unsigned sigrestrict_arg;

  
                                 
   

void    execsigs(struct proc *p, thread_t thread);
void    gsignal(int pgid, int sig);
int     issignal_locked(struct proc *p);
int     CURSIG(struct proc *p);
int clear_procsiglist(struct proc *p, int bit, int in_signalstart);
int set_procsigmask(struct proc *p, int bit);
void    postsig_locked(int sig);
void    siginit(struct proc *p);
void    trapsignal(struct proc *p, int sig, unsigned code);
void    pt_setrunnable(struct proc *p);
int     hassigprop(int sig, int prop);
int setsigvec(proc_t, thread_t, int signum, struct __kern_sigaction *, boolean_t in_sigstart);

struct os_reason;
  
                               
   
void    sendsig(struct proc *,           user_addr_t  action, int sig,
    int returnmask, uint32_t code, sigset_t siginfo);

void    psignal(struct proc *p, int sig);
void    psignal_with_reason(struct proc *p, int sig, struct os_reason *signal_reason);
void    psignal_locked(struct proc *, int);
void    psignal_try_thread(proc_t, thread_t, int signum);
void    psignal_try_thread_with_reason(proc_t, thread_t, int, struct os_reason*);
void    psignal_thread_with_reason(proc_t, thread_t, int, struct os_reason*);
void    psignal_uthread(thread_t, int);
void    pgsignal(struct pgrp *pgrp, int sig, int checkctty);
void    tty_pgsignal(struct tty * tp, int sig, int checkctty);
void    threadsignal(thread_t sig_actthread, int signum,
    mach_exception_code_t code, boolean_t set_exitreason);
int     thread_issignal(proc_t p, thread_t th, sigset_t mask);
void    psignal_vfork(struct proc *p, task_t new_task, thread_t thread,
    int signum);
void psignal_vfork_with_reason(proc_t p, task_t new_task, thread_t thread,
    int signum, struct os_reason *signal_reason);
void    signal_setast(thread_t sig_actthread);
void    pgsigio(pid_t pgid, int signalnum);

void sig_lock_to_exit(struct proc *p);
int sig_try_locked(struct proc *p);

#endif                          

#if defined(KERNEL_PRIVATE)
                                                                                    
struct proc;
struct os_reason;
void    psignal_sigkill_with_reason(struct proc *p, struct os_reason *signal_reason);
#endif                              

#ifdef XNU_KERNEL_PRIVATE

                                        

#define COREDUMP_IGNORE_ULIMIT  0x0001                                             
#define COREDUMP_FULLFSYNC      0x0002                                               

int     coredump(struct proc *p, uint32_t reserve_mb, int coredump_flags);
void set_thread_exit_reason(void *th, void *reason, boolean_t proc_locked);

#endif                          


#endif                          
