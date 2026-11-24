/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995, 1997 Apple Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1986, 1989, 1991, 1993
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
 *	@(#)proc.h	8.15 (Berkeley) 5/19/95
 */

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>
#include <sys/select.h>                                          
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/param.h>
#include <sys/event.h>
#include <sys/time.h>
#ifdef KERNEL
#include <sys/kernel_types.h>
#include <uuid/uuid.h>
#endif
#include <mach/boolean.h>

#ifdef XNU_KERNEL_PRIVATE
#include <mach/coalition.h>                                      
#endif

#ifndef KERNEL
#include <Availability.h>
#endif

#if defined(XNU_KERNEL_PRIVATE) || !defined(KERNEL)

struct session;
struct pgrp;
struct proc;
struct proc_ident;

                                      
struct extern_proc {
	union {
		struct {
			struct  proc *__p_forw;                                     
			struct  proc *__p_back;
		} p_st1;
		struct timeval __p_starttime;                           
	} p_un;
#define p_forw p_un.p_st1.__p_forw
#define p_back p_un.p_st1.__p_back
#define p_starttime p_un.__p_starttime
	struct  vmspace *p_vmspace;                         
	struct  sigacts *p_sigacts;                                             
	int     p_flag;                                 
	char    p_stat;                                         
	pid_t   p_pid;                                           
	pid_t   p_oppid;                                                 
	int     p_dupfd;                                                     
	                   
	caddr_t user_stack;                                         
	void    *exit_thread;                                     
	int             p_debugger;                                 
	boolean_t       sigwait;                                   
	                
	u_int   p_estcpu;                                               
	int     p_cpticks;                               
	fixpt_t p_pctcpu;                                                   
	void    *p_wchan;                            
	char    *p_wmesg;                               
	u_int   p_swtime;                                     
	u_int   p_slptime;                                     
	struct  itimerval p_realtimer;                    
	struct  timeval p_rtime;                        
	u_quad_t p_uticks;                                                
	u_quad_t p_sticks;                                                  
	u_quad_t p_iticks;                                                   
	int     p_traceflag;                                      
	struct  vnode *p_tracep;                             
	int     p_siglist;                               
	struct  vnode *p_textvp;                                  
	int     p_holdcnt;                                            
	sigset_t p_sigmask;                      
	sigset_t p_sigignore;                               
	sigset_t p_sigcatch;                                       
	u_char  p_priority;                            
	u_char  p_usrpri;                                                     
	char    p_nice;                                    
	char    p_comm[MAXCOMLEN + 1];
	struct  pgrp *p_pgrp;                                  
	struct  user *p_addr;                                                   
	u_short p_xstat;                                                     
	u_short p_acflag;                              
	struct  rusage *p_ru;                              
};


                    
#define SIDL    1                                                   
#define SRUN    2                                        
#define SSLEEP  3                                            
#define SSTOP   4                                                     
#define SZOMB   5                                                   

                                                 
#define P_ADVLOCK       0x00000001                                            
#define P_CONTROLT      0x00000002                                      
#define P_LP64          0x00000004                           
#define P_NOCLDSTOP     0x00000008                                         

#define P_PPWAIT        0x00000010                                             
#define P_PROFIL        0x00000020                                 
#define P_SELECT        0x00000040                                            
#define P_CONTINUED     0x00000080                                             

#define P_SUGID         0x00000100                                              
#define P_SYSTEM        0x00000200                                            
#define P_TIMEOUT       0x00000400                                   
#define P_TRACED        0x00000800                                         

#define P_DISABLE_ASLR  0x00001000                                                      
#define P_WEXIT         0x00002000                              
#define P_EXEC          0x00004000                                

                                                 
#define P_OWEUPC        0x00008000                                                     

#define P_AFFINITY      0x00010000               
#define P_TRANSLATED    0x00020000               
#define P_CLASSIC       P_TRANSLATED             

#define P_DELAYIDLESLEEP 0x00040000                                                           
#define P_CHECKOPENEVT  0x00080000                                                             

#define P_DEPENDENCY_CAPABLE    0x00100000                                                      
#define P_REBOOT        0x00200000                                   
#define P_RESV6         0x00400000                            
#define P_RESV7         0x00800000                                       

#define P_THCWD         0x01000000                                   
#define P_RESV9         0x02000000                                               
#define P_ADOPTPERSONA  0x04000000                                                             
#define P_RESV11        0x08000000                                     

#define P_NOSHLIB       0x10000000                                              
                                                              
#define P_FORCEQUOTA    0x20000000                                
#define P_NOCLDWAIT     0x40000000                                           
#define P_NOREMOTEHANG  0x80000000                                       

#define P_INMEM         0                                                       
#define P_NOSWAP        0                                                       
#define P_PHYSIO        0                                                       
#define P_FSTRACE       0                                                       
#define P_SSTEP         0                                                       

#define P_DIRTY_TRACK                           0x00000001                             
#define P_DIRTY_ALLOW_IDLE_EXIT                 0x00000002                                                 
#define P_DIRTY_DEFER                           0x00000004                                             
#define P_DIRTY                                 0x00000008                            
#define P_DIRTY_SHUTDOWN                        0x00000010                                            
#define P_DIRTY_TERMINATED                      0x00000020                                                   
#define P_DIRTY_BUSY                            0x00000040                              
#define P_DIRTY_MARKED                          0x00000080                                   
#define P_DIRTY_AGING_IN_PROGRESS               0x00000100                                             
#define P_DIRTY_LAUNCH_IN_PROGRESS              0x00000200                                 
#define P_DIRTY_DEFER_ALWAYS                    0x00000400                                                                      
                                                                                                                                         

#define P_DIRTY_IS_DIRTY                        (P_DIRTY | P_DIRTY_SHUTDOWN)
#define P_DIRTY_IDLE_EXIT_ENABLED               (P_DIRTY_TRACK|P_DIRTY_ALLOW_IDLE_EXIT)

#endif                                    

#ifdef KERNEL
__BEGIN_DECLS

extern proc_t kernproc;

extern int proc_is_classic(proc_t p);
extern bool proc_is_exotic(proc_t p);
extern bool proc_is_alien(proc_t p);
proc_t current_proc_EXTERNAL(void);

extern int      msleep(void *chan, lck_mtx_t *mtx, int pri, const char *wmesg, struct timespec * ts );
extern void     wakeup(void *chan);
extern void wakeup_one(caddr_t chan);

               
                                                         
extern int proc_selfpid(void);
                                                                       
extern int proc_selfppid(void);
                                                             
extern uint64_t proc_selfcsflags(void);
                                                                                                                            
extern int proc_csflags(proc_t p, uint64_t* flags);
                                                                                     
extern void proc_signal(int pid, int signum);
                                                                                                                                                                   
extern int proc_issignal(int pid, sigset_t mask);
                                                            
extern int proc_isinferior(int pid1, int pid2);
                                                                                       
                                                                                     
                                                
   
void proc_name(int pid, char * buf, int size);
                                                                               
extern char *proc_best_name(proc_t p);
                                                                                 
void proc_selfname(char * buf, int size);

                                                                                                         
extern proc_t proc_find(int pid);
                                                  
extern proc_t proc_find_ident(struct proc_ident const *i);
                                                                                                               
extern proc_t proc_self(void);
                                                
extern int proc_rele(proc_t p);
                                          
extern int proc_pid(proc_t);
                                                      
extern int proc_ppid(proc_t);
                                                               
extern int proc_original_ppid(proc_t);
                                                 
extern int proc_starttime(proc_t, struct timeval *);
                                                                                
extern uint32_t proc_platform(const proc_t);
                                                                 
extern uint32_t proc_min_sdk(proc_t);
                                                         
extern uint32_t proc_sdk(proc_t);
                                                            
extern int proc_noremotehang(proc_t);
                                                        
extern int proc_forcequota(proc_t);
                                          
extern int proc_chrooted(proc_t);
                                                                        
extern boolean_t proc_send_synchronous_EXC_RESOURCE(proc_t p);

                                                                                         
extern int proc_is64bit(proc_t);
                                                                                          
extern int proc_is64bit_data(proc_t);
                                                       
extern int proc_isinitproc(proc_t);
                              
extern int proc_exiting(proc_t);
                                                              
extern int proc_in_teardown(proc_t);
                                                                                     
int proc_suser(proc_t p);
                                                                 
kauth_cred_t proc_ucred(proc_t p);
                                                                        
extern int proc_issetugid(proc_t p);

extern int proc_tbe(proc_t);

   
                         
                                                                                                                                       
                                                                                                                              
   
extern int proc_gettty(proc_t p, vnode_t *vp);

                                                                                                                                    
extern int proc_gettty_dev(proc_t p, dev_t *dev);

   
                             
                                                                                      
                                      
   
pid_t proc_selfpgrpid(void);

   
                         
                                                                 
                                          
                           
   
pid_t proc_pgrpid(proc_t p);

   
                            
                                                                   
                                              
                                          
   
pid_t proc_sessionid(proc_t p);

#ifdef KERNEL_PRIVATE
                                                               
void bsd_set_dependency_capable(task_t task);
#ifdef  __arm__
static inline int
IS_64BIT_PROCESS(__unused proc_t p)
{
	return 0;
}
#else
extern int IS_64BIT_PROCESS(proc_t);
#endif              

extern int      tsleep(void *chan, int pri, const char *wmesg, int timo);
extern int      msleep1(void *chan, lck_mtx_t *mtx, int pri, const char *wmesg, u_int64_t timo);

task_t proc_task(proc_t);
extern int proc_pidversion(proc_t);
extern proc_t proc_parent(proc_t);
extern void proc_parent_audit_token(proc_t, audit_token_t *);
extern uint32_t proc_persona_id(proc_t);
extern uint32_t proc_getuid(proc_t);
extern uint32_t proc_getgid(proc_t);
extern int proc_getcdhash(proc_t, unsigned char *);

   
                                     
                                                                         
                                                                                  
                                                                        
                                                  
                                                                         
                                                      
                                                                     
  
                                       
   
extern int proc_pidbackgrounded(pid_t pid, uint32_t* state);

  
                                                      
                                               
                               
   
extern uint64_t proc_uniqueid(proc_t);

                                                   
extern uint64_t proc_puniqueid(proc_t);

extern void proc_set_responsible_pid(proc_t target_proc, pid_t responsible_pid);

                                                                              
extern int proc_is_forcing_hfs_case_sensitivity(proc_t);

                                                                 
extern boolean_t proc_is_translated(proc_t);

                                                                     
extern bool proc_ignores_content_protection(proc_t proc);

                                                                                  
extern bool proc_skip_mtime_update(proc_t proc);

   
                                
                                                          
                                                                          
                                                                             
                                               
                                           
                                                                       
   
extern int proc_exitstatus(proc_t p);

#endif                     

#ifdef XNU_KERNEL_PRIVATE

extern void proc_getexecutableuuid(proc_t, unsigned char *, unsigned long);
extern int proc_get_originatorbgstate(uint32_t *is_backgrounded);

                                                                    
extern int proc_pidoriginatoruuid(uuid_t uuid_buf, uint32_t buffersize);

extern uint64_t proc_was_throttled(proc_t);
extern uint64_t proc_did_throttle(proc_t);
extern bool proc_is_traced(proc_t p);

extern void proc_coalitionids(proc_t, uint64_t[COALITION_NUM_TYPES]);

#ifdef CONFIG_32BIT_TELEMETRY
extern void proc_log_32bit_telemetry(proc_t p);
#endif                             
extern uint64_t get_current_unique_pid(void);
#endif                        

#ifdef KERNEL_PRIVATE
                                                                                     
extern int proc_selfexecutableargs(uint8_t *buf, size_t *buflen);
extern off_t proc_getexecutableoffset(proc_t p);
extern vnode_t proc_getexecutablevnode(proc_t);                                                     
extern int networking_memstatus_callout(proc_t p, uint32_t);

                                                                              
#define SYSCALL_MASK_UNIX 0
#define SYSCALL_MASK_MACH 1
#define SYSCALL_MASK_KOBJ 2

#define SYSCALL_FILTER_CALLBACK_VERSION 1
typedef int (*syscall_filter_cbfunc_t)(proc_t p, int num);
typedef int (*kobject_filter_cbfunc_t)(proc_t p, int msgid, int idx);
struct syscall_filter_callbacks {
	int version;
	const syscall_filter_cbfunc_t unix_filter_cbfunc;
	const syscall_filter_cbfunc_t mach_filter_cbfunc;
	const kobject_filter_cbfunc_t kobj_filter_cbfunc;
};
typedef struct syscall_filter_callbacks * syscall_filter_cbs_t;

extern int proc_set_syscall_filter_callbacks(syscall_filter_cbs_t callback);
extern int proc_set_syscall_filter_index(int which, int num, int index);
extern size_t proc_get_syscall_filter_mask_size(int which);
extern int proc_set_syscall_filter_mask(proc_t p, int which, unsigned char *maskptr, size_t masklen);

extern int proc_set_filter_message_flag(proc_t p, boolean_t flag);
extern int proc_get_filter_message_flag(proc_t p, boolean_t *flag);

#endif                     

__END_DECLS

#endif              

#ifdef PRIVATE

                                     
#define SHUTDOWN_SOCKET_LEVEL_DISCONNECT_SVC            0x00000001
#define SHUTDOWN_SOCKET_LEVEL_DISCONNECT_ALL            0x00000002

#ifdef KERNEL
#define SHUTDOWN_SOCKET_LEVEL_DISCONNECT_INTERNAL       0x10000000
#define SHUTDOWN_SOCKET_LEVEL_NECP                      0x20000000
#define SHUTDOWN_SOCKET_LEVEL_CONTENT_FILTER            0x40000000
#endif

#ifndef KERNEL

__BEGIN_DECLS

int pid_suspend(int pid);
int pid_resume(int pid);
__API_AVAILABLE(macos(11.3), ios(14.5), tvos(14.5), watchos(7.3))
int task_inspect_for_pid(unsigned int target_tport, int pid, unsigned int *t);                                 
__API_AVAILABLE(macos(11.3), ios(14.5), tvos(14.5), watchos(7.3))
int task_read_for_pid(unsigned int target_tport, int pid, unsigned int *t);                                 

#if defined(__arm__) || defined(__arm64__)
int pid_hibernate(int pid);
#endif                                              
int pid_shutdown_sockets(int pid, int level);
int pid_shutdown_networking(int pid, int level);
__END_DECLS

#endif              

                                                                        
#define PROCESS_RESUME_SUSPEND_ENTITLEMENT "com.apple.private.process.suspend-resume.any"

#endif              

#endif                     
