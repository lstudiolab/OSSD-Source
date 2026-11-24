/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
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
 *	@(#)signal.h	8.2 (Berkeley) 1/21/94
 */

#ifndef _SYS_SIGNAL_H_
#define _SYS_SIGNAL_H_

#include <sys/cdefs.h>
#include <sys/appleapiopts.h>
#include <Availability.h>

#define __DARWIN_NSIG   32                                                  

#if !defined(_ANSI_SOURCE) && (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
#define NSIG    __DARWIN_NSIG
#endif

#include <machine/signal.h>                                               

#define SIGHUP  1                   
#define SIGINT  2                      
#define SIGQUIT 3                 
#define SIGILL  4                                                        
#define SIGTRAP 5                                               
#define SIGABRT 6                    
#if  (defined(_POSIX_C_SOURCE) && !defined(_DARWIN_C_SOURCE))
#define SIGPOLL 7                                                            
#else                                               
#define SIGIOT  SIGABRT                    
#define SIGEMT  7                            
#endif                                              
#define SIGFPE  8                                     
#define SIGKILL 9                                               
#define SIGBUS  10                     
#define SIGSEGV 11                                  
#define SIGSYS  12                                       
#define SIGPIPE 13                                                  
#define SIGALRM 14                       
#define SIGTERM 15                                                 
#define SIGURG  16                                          
#define SIGSTOP 17                                             
#define SIGTSTP 18                                
#define SIGCONT 19                                      
#define SIGCHLD 20                                           
#define SIGTTIN 21                                                    
#define SIGTTOU 22                                                         
#if  (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
#define SIGIO   23                                        
#endif
#define SIGXCPU 24                                   
#define SIGXFSZ 25                                    
#define SIGVTALRM 26                            
#define SIGPROF 27                                
#if  (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
#define SIGWINCH 28                              
#define SIGINFO 29                               
#endif
#define SIGUSR1 30                                 
#define SIGUSR2 31                                 

#if defined(_ANSI_SOURCE) || __DARWIN_UNIX03 || defined(__cplusplus)
  
                                                                       
                               
                                                                     
   
#define SIG_DFL         (void (*)(int))0
#define SIG_IGN         (void (*)(int))1
#define SIG_HOLD        (void (*)(int))5
#define SIG_ERR         ((void (*)(int))-1)
#else
                                                                        
#define SIG_DFL         (void (*)(         ))0
#define SIG_IGN         (void (*)(         ))1
#define SIG_HOLD        (void (*)(         ))5
#define SIG_ERR         ((void (*)(         ))-1)
#endif

#ifndef _ANSI_SOURCE
#include <sys/_types.h>

#include <machine/_mcontext.h>

#ifndef KERNEL
#include <sys/_pthread/_pthread_attr_t.h>
#endif             

#include <sys/_types/_sigaltstack.h>
#include <sys/_types/_ucontext.h>

#include <sys/_types/_pid_t.h>
#include <sys/_types/_sigset_t.h>
#include <sys/_types/_size_t.h>
#include <sys/_types/_uid_t.h>

union sigval {
	                                                       
	int     sival_int;
	void    *sival_ptr;
};

#define SIGEV_NONE      0                                  
#define SIGEV_SIGNAL    1                                          
#define SIGEV_THREAD    3                                                      

#ifndef KERNEL
struct sigevent {
	int                             sigev_notify;                                                  
	int                             sigev_signo;                                               
	union sigval    sigev_value;                                              
	void                    (*sigev_notify_function)(union sigval);                              
	pthread_attr_t  *sigev_notify_attributes;                                    
};
#endif             

#ifdef BSD_KERNEL_PRIVATE

union user64_sigval {
	struct {
		uint32_t                pad;                                     
		int32_t                 sival_int;
	} size_equivalent;
	user64_addr_t   sival_ptr;
};

union user32_sigval {
	                                                       
	int32_t sival_int;
	user32_addr_t sival_ptr;
};

union user_sigval {
	struct {
		uint32_t                pad;                                     
		int32_t                 sival_int;
	} size_equivalent;
	user_addr_t sival_ptr;
};

struct user64_sigevent {
	int             sigev_notify;                                          
	int             sigev_signo;                                       
	union user64_sigval sigev_value;                                          
	user64_addr_t   sigev_notify_function;                               
	user64_addr_t   sigev_notify_attributes;                               
};

struct user32_sigevent {
	int             sigev_notify;                                          
	int             sigev_signo;                                       
	union user32_sigval sigev_value;                                          
	user32_addr_t   sigev_notify_function;                               
	user32_addr_t   sigev_notify_attributes;                               
};

struct user_sigevent {
	int             sigev_notify;                                          
	int             sigev_signo;                                       
	union user_sigval sigev_value;                                    
	user_addr_t     sigev_notify_function;                               
	user_addr_t     sigev_notify_attributes;                               
};

#endif                          

typedef struct __siginfo {
	int     si_signo;                                  
	int     si_errno;                                      
	int     si_code;                                 
	pid_t   si_pid;                                      
	uid_t   si_uid;                                    
	int     si_status;                              
	void    *si_addr;                                         
	union sigval si_value;                            
	long    si_band;                                            
	unsigned long   __pad[7];                                    
} siginfo_t;

#ifdef BSD_KERNEL_PRIVATE

typedef struct user_siginfo {
	int             si_signo;                          
	int             si_errno;                              
	int             si_code;                         
	pid_t           si_pid;                              
	uid_t           si_uid;                            
	int             si_status;                      
	user_addr_t     si_addr;                                              
	union user_sigval si_value;                       
	user_long_t     si_band;                                    
	user_ulong_t    pad[7];                                      
} user_siginfo_t;

typedef struct user64_siginfo {
	int             si_signo;                          
	int             si_errno;                              
	int             si_code;                         
	pid_t           si_pid;                              
	uid_t           si_uid;                            
	int             si_status;                      
	user64_addr_t   si_addr;                                              
	union user64_sigval si_value;                     
	user64_long_t   si_band;                                    
	user64_ulong_t  __pad[7];                                            
} user64_siginfo_t;

typedef struct user32_siginfo {
	int             si_signo;                          
	int             si_errno;                              
	int             si_code;                         
	pid_t           si_pid;                              
	uid_t           si_uid;                            
	int             si_status;                      
	user32_addr_t   si_addr;                                              
	union user32_sigval     si_value;                         
	user32_long_t   si_band;                                    
	user32_ulong_t  __pad[7];                                            
} user32_siginfo_t;

void siginfo_user_to_user32(user_siginfo_t *, user32_siginfo_t *);
void siginfo_user_to_user64(user_siginfo_t *, user64_siginfo_t *);

#endif                          

  
                                                                       
                            
                                                                        
                                                                             
                                                      
                                                                                
                                                   
                                                                         
   

                        

                      
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define ILL_NOOP        0                              
#endif
#define ILL_ILLOPC      1                                 
#define ILL_ILLTRP      2                               
#define ILL_PRVOPC      3                                    
#define ILL_ILLOPN      4                                          
#define ILL_ILLADR      5                                                  
#define ILL_PRVREG      6                                              
#define ILL_COPROC      7                                            
#define ILL_BADSTK      8                                               

                      
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define FPE_NOOP        0                              
#endif
#define FPE_FLTDIV      1                                                
#define FPE_FLTOVF      2                                          
#define FPE_FLTUND      3                                           
#define FPE_FLTRES      4                                                
#define FPE_FLTINV      5                                                   
#define FPE_FLTSUB      6                                                 
#define FPE_INTDIV      7                                         
#define FPE_INTOVF      8                                   

                       
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define SEGV_NOOP       0                              
#endif
#define SEGV_MAPERR     1                                               
#define SEGV_ACCERR     2                                                       

                      
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define BUS_NOOP        0                              
#endif
#define BUS_ADRALN      1                                            
#define BUS_ADRERR      2                                                       
#define BUS_OBJERR      3                                                    

                       
#define TRAP_BRKPT      1                                             
#define TRAP_TRACE      2                                             

                       
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define CLD_NOOP        0                              
#endif
#define CLD_EXITED      1                                   
#define CLD_KILLED      2                                                      
#define CLD_DUMPED      3                                                   
#define CLD_TRAPPED     4                                           
#define CLD_STOPPED     5                                    
#define CLD_CONTINUED   6                                              

                       
#define POLL_IN         1                                       
#define POLL_OUT        2                                           
#define POLL_MSG        3                                          
#define POLL_ERR        4                            
#define POLL_PRI        5                                                
#define POLL_HUP        6                                      

                               
union __sigaction_u {
	void    (*__sa_handler)(int);
	void    (*__sa_sigaction)(int, struct __siginfo *,
	    void *);
};

                                                     
struct  __sigaction {
	union __sigaction_u __sigaction_u;                      
	void    (*sa_tramp)(void *, int, int, siginfo_t *, void *);
	sigset_t sa_mask;                                         
	int     sa_flags;                                             
};

  
                                                   
   
struct  sigaction {
	union __sigaction_u __sigaction_u;                      
	sigset_t sa_mask;                                         
	int     sa_flags;                                             
};

#ifdef  BSD_KERNEL_PRIVATE
#include <machine/types.h>

union __user32_sigaction_u {
	user32_addr_t __sa_handler;
	user32_addr_t __sa_sigaction;
};

struct  user32_sigaction {
	union __user32_sigaction_u __sigaction_u;                      
	sigset_t sa_mask;                                         
	int     sa_flags;                                             
};

struct  __user32_sigaction {
	union __user32_sigaction_u __sigaction_u;                      
	user32_addr_t sa_tramp;
	sigset_t sa_mask;                                         
	int     sa_flags;                                             
};

union __user64_sigaction_u {
	user64_addr_t   __sa_handler;
	user64_addr_t   __sa_sigaction;
};

struct  user64_sigaction {
	union __user64_sigaction_u __sigaction_u;                      
	sigset_t sa_mask;                                         
	int     sa_flags;                                             
};

struct  __user64_sigaction {
	union __user64_sigaction_u __sigaction_u;                      
	user64_addr_t   sa_tramp;                                 
	sigset_t sa_mask;                                         
	int     sa_flags;                                             
};

union __kern_sigaction_u {
	user_addr_t     __sa_handler;
	user_addr_t     __sa_sigaction;
};

struct  kern_sigaction {
	union __kern_sigaction_u __sigaction_u;                      
	sigset_t sa_mask;                                         
	int     sa_flags;                                             
};

struct  __kern_sigaction {
	union __kern_sigaction_u __sigaction_u;                      
	user_addr_t     sa_tramp;                                 
	sigset_t sa_mask;                                         
	int     sa_flags;                                             
};

#undef SIG_DFL
#undef SIG_IGN
#undef SIG_ERR
#define  SIG_DFL        ((user_addr_t)0LL)
#define  SIG_IGN        ((user_addr_t)1LL)
#define  SIG_ERR        ((user_addr_t)-1LL)

#endif                          


                                                                             
#define sa_handler      __sigaction_u.__sa_handler
#define sa_sigaction    __sigaction_u.__sa_sigaction

#define SA_ONSTACK      0x0001                                   
#define SA_RESTART      0x0002                                       
#ifdef  BSD_KERNEL_PRIVATE
#define SA_DISABLE      0x0004                                                                                      
#endif                          
#define SA_RESETHAND    0x0004                                           
#define SA_NOCLDSTOP    0x0008                                             
#define SA_NODEFER      0x0010                                              
#define SA_NOCLDWAIT    0x0020                                 
#define SA_SIGINFO      0x0040                                           
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define SA_USERTRAMP    0x0100                                           
                                                                        
#define SA_64REGSET     0x0200                                                                       
#endif                                             
#ifdef  BSD_KERNEL_PRIVATE
#define SA_VALIDATE_SIGRETURN_FROM_SIGTRAMP     0x0400                                                                         
#endif                          

                                                                  
                                
   
#define SA_USERSPACE_MASK (SA_ONSTACK | SA_RESTART | SA_RESETHAND | SA_NOCLDSTOP | SA_NODEFER | SA_NOCLDWAIT | SA_SIGINFO)

  
                         
   
#define SIG_BLOCK       1                                       
#define SIG_UNBLOCK     2                                         
#define SIG_SETMASK     3                                     

                                    
#define SI_USER         0x10001                              
#define SI_QUEUE        0x10002                                  
#define SI_TIMER        0x10003                            
#define SI_ASYNCIO      0x10004                                  
#define SI_MESGQ        0x10005                                               

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
typedef void (*sig_t)(int);                                  
#endif

  
                                      
   
#ifdef  BSD_KERNEL_PRIVATE

struct  user32_sigaltstack {
	user32_addr_t   ss_sp;                                 
	user32_size_t   ss_size;                                 
	int                 ss_flags;                                     
};

struct  user64_sigaltstack {
	user64_addr_t   ss_sp;                                 
	user64_size_t   ss_size;                                 
	int             ss_flags;                                         
};

struct  kern_sigaltstack {
	user_addr_t     ss_sp;                                 
	user_size_t     ss_size;                                 
	int             ss_flags;                                         
};

#endif                          

#define SS_ONSTACK      0x0001                                   
#define SS_DISABLE      0x0004                                                 
#define MINSIGSTKSZ     32768                                     
#define SIGSTKSZ        131072                                    

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                     
                                                
   
struct  sigvec {
	void    (*sv_handler)(int);                         
	int     sv_mask;                                          
	int     sv_flags;                                             
};

#define SV_ONSTACK      SA_ONSTACK
#define SV_INTERRUPT    SA_RESTART                                    
#define SV_RESETHAND    SA_RESETHAND
#define SV_NODEFER      SA_NODEFER
#define SV_NOCLDSTOP    SA_NOCLDSTOP
#define SV_SIGINFO      SA_SIGINFO

#define sv_onstack sv_flags                                         
#endif                                             

  
                                   
   
struct  sigstack {
	char    *ss_sp;                                           
	int     ss_onstack;                                 
};

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                                                            
              
   
#define sigmask(m)      (1 << ((m)-1))

#ifdef  KERNEL_PRIVATE
  
                                           
   
#define threadmask (sigmask(SIGILL)|sigmask(SIGTRAP)|\
	            sigmask(SIGABRT)|sigmask(SIGEMT)|\
	            sigmask(SIGFPE)|sigmask(SIGBUS)|\
	            sigmask(SIGSEGV)|sigmask(SIGSYS)|\
	            sigmask(SIGPIPE)|sigmask(SIGKILL))

#define workq_threadmask ((threadmask | sigcantmask | sigmask(SIGPROF)) & ~sigmask(SIGABRT))

  
                               
   
#define execmask   (sigmask(SIGHUP)|sigmask(SIGINT)|\
	            sigmask(SIGQUIT)|sigmask(SIGKILL)|\
	            sigmask(SIGTERM)|sigmask(SIGSTOP)|\
	            sigmask(SIGTSTP)|sigmask(SIGCONT)|\
	            sigmask(SIGTTIN)|sigmask(SIGTTOU)|\
	            sigmask(SIGUSR1)|sigmask(SIGUSR2))

#endif                      

#define BADSIG          SIG_ERR

#endif                                              
#endif                     

  
                                                                      
                             
   
__BEGIN_DECLS
    void(*signal(int, void (*)(int)))(int);
__END_DECLS
#endif                       
