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
 *	@(#)proc_internal.h	8.15 (Berkeley) 5/19/95
 */
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef _SYS_PROC_INTERNAL_H_
#define _SYS_PROC_INTERNAL_H_

#include <libkern/OSAtomic.h>
#include <sys/proc.h>
#include <mach/resource_monitors.h>                           

__BEGIN_DECLS
#include <kern/locks.h>
#if PSYNCH
#include <kern/thread_call.h>
#endif             
__END_DECLS

#if DEBUG
#define __PROC_INTERNAL_DEBUG 1
#endif

  
                                                                               
                    
                           
                                
                                       
                          
                 
                    
   
struct label;

  
                                       
   
struct  session {
	int                     s_count;                                                     
	struct  proc *          s_leader;                                            
	struct  vnode *         s_ttyvp;                                                        
	int                     s_ttyvid;                                                              
	struct  tty *           s_ttyp;                                                                 
	pid_t                   s_ttypgrpid;                               
	pid_t                   s_sid;                                           
	char                    s_login[MAXLOGNAME];                              
	int                     s_flags;                                              
	LIST_ENTRY(session)     s_hash;                                      
	lck_mtx_t               s_mlock;                                                   
	int                     s_listflags;
};

#define SESSION_NULL (struct session *)NULL

  
                                                                          
                                                                         
                                                                             
                                                                               
                                                      
  
                                                                             
                                    
   
#define SESSION_TP(sp)  (((sp)->s_ttyvp != 0) ? (sp)->s_ttyp : 0)

  
                                                                     
                    
   
#define S_DEFAULT       0x00000000                        
#define S_NOCTTY        0x00000001                                            
#define S_CTTYREF       0x00000010                                       


#define S_LIST_TERM     1                                           
#define S_LIST_DEAD     2                                 
  
                                             
   
struct  pgrp {
	LIST_ENTRY(pgrp)        pg_hash;                              
	LIST_HEAD(, proc)       pg_members;                                         
	struct  session *       pg_session;                                    
	pid_t                   pg_id;                                 
	int                     pg_jobc;                                                           
	int                     pg_membercnt;                                                        
	int                     pg_refcount;                                          
	unsigned int            pg_listflags;             
	lck_mtx_t               pg_mlock;                                       
};

#define PGRP_FLAG_TERMINATE     1
#define PGRP_FLAG_WAITTERMINATE 2
#define PGRP_FLAG_DEAD          4
#define PGRP_FLAG_ITERABEGIN    8
#define PGRP_FLAG_ITERWAIT      0x10

#define PGRP_NULL (struct pgrp *)NULL
struct proc;

#define PROC_NULL (struct proc *)NULL

#define PROC_UPDATE_CREDS_ONPROC(p) { \
	p->p_uid =  kauth_cred_getuid(p->p_ucred); \
	p->p_gid =  kauth_cred_getgid(p->p_ucred); \
	p->p_ruid =  kauth_cred_getruid(p->p_ucred); \
	p->p_rgid =  kauth_cred_getrgid(p->p_ucred); \
	p->p_svuid =  kauth_cred_getsvuid(p->p_ucred); \
	p->p_svgid =  kauth_cred_getsvgid(p->p_ucred); \
	}
  
                            
  
                                                                       
                                                                          
                                                                         
                                                                       
                                                                      
                                                                      
              
   
struct  proc {
	LIST_ENTRY(proc) p_list;                                            

	void *          XNU_PTRAUTH_SIGNED_PTR("proc.task") task;                                       
	struct  proc *  XNU_PTRAUTH_SIGNED_PTR("proc.p_pptr") p_pptr;                                       
	pid_t           p_ppid;                                                  
	pid_t           p_original_ppid;                                                                                
	pid_t           p_pgrpid;                                                        
	uid_t           p_uid;
	gid_t           p_gid;
	uid_t           p_ruid;
	gid_t           p_rgid;
	uid_t           p_svuid;
	gid_t           p_svgid;
	uint64_t        p_uniqueid;                                                                                                 
	uint64_t        p_puniqueid;                                                                                             

	lck_mtx_t       p_mlock;                                         
	pid_t           p_pid;                                                   
	char            p_stat;                                             
	char            p_shutdownstate;
	char            p_kdebug;                                    
	char            p_btrace;                                    

	LIST_ENTRY(proc) p_pglist;                                                   
	LIST_ENTRY(proc) p_sibling;                                                 
	LIST_HEAD(, proc) p_children;                                                 
	TAILQ_HEAD(, uthread) p_uthlist;                                    

	LIST_ENTRY(proc) p_hash;                                     

#if CONFIG_PERSONAS
	struct persona  *p_persona;
	LIST_ENTRY(proc) p_persona_list;
#endif

	lck_mtx_t       p_fdmlock;                                              
	lck_mtx_t       p_ucred_mlock;                                             

	                    
	kauth_cred_t    XNU_PTRAUTH_SIGNED_PTR("proc.p_ucred") p_ucred;                                       
	struct  filedesc *p_fd;                                                          
	struct  pstats *p_stats;                                                 
	struct  plimit *p_limit;                                         

	struct  sigacts *p_sigacts;                                             
	lck_spin_t      p_slock;                                                            

	int             p_siglist;                                                      
	unsigned int    p_flag;                                                  
	unsigned int    p_lflag;                                       
	unsigned int    p_listflag;                                  
	unsigned int    p_ladvflag;                                           
	int             p_refcount;                                                  
	int             p_childrencnt;                                                   
	int             p_parentref;                                                    
	pid_t           p_oppid;                                                        
	u_int           p_xstat;                                                             

#ifdef _PROC_HAS_SCHEDINFO_
	                                
	u_int           p_estcpu;                                                                                    
	fixpt_t         p_pctcpu;                                                                       
	u_int           p_slptime;                                        
#endif                           

	struct  itimerval p_realtimer;                                  
	struct  timeval p_rtime;                                      
	struct  itimerval p_vtimer_user;                                   
	struct  itimerval p_vtimer_prof;                   

	struct  timeval p_rlim_cpu;                                                 
	int             p_debugger;                                                            
	boolean_t       sigwait;                                        
	void    *sigwait_thread;                                           
	void    *exit_thread;                                             
	void *  p_vforkact;                                                               
	int     p_vforkcnt;                                                    
	int     p_fpdrainwait;                      
	                                                 
	pid_t   si_pid;                           
	u_int   si_status;                        
	u_int   si_code;                          
	uid_t   si_uid;                           

	void * vm_shm;                                                              

#if CONFIG_DTRACE
	user_addr_t                     p_dtrace_argv;                                                          
	user_addr_t                     p_dtrace_envp;                                                          
	lck_mtx_t                       p_dtrace_sprlock;                                            
	uint8_t                         p_dtrace_stop;                                                       
	int                             p_dtrace_probes;                                                          
	u_int                           p_dtrace_count;                                                             
	struct dtrace_ptss_page*        p_dtrace_ptss_pages;                                                   
	struct dtrace_ptss_page_entry*  p_dtrace_ptss_free_list;                                                      
	struct dtrace_helpers*          p_dtrace_helpers;                                                          
	struct dof_ioctl_data*          p_dtrace_lazy_dofs;                                                    
#endif                    

                                                     
                                                                
#define p_startcopy     p_argslen

	u_int   p_argslen;                                         
	int     p_argc;                                                       
	user_addr_t user_stack;                                             
	struct  vnode * XNU_PTRAUTH_SIGNED_PTR("proc.p_textvp") p_textvp;                                 
	off_t   p_textoff;                                              

	sigset_t p_sigmask;                             
	sigset_t p_sigignore;                                    
	sigset_t p_sigcatch;                                            

	u_char  p_priority;                                 
	u_char  p_resv0;                                                           
	char    p_nice;                                        
	u_char  p_resv1;                                                           

	                                 
	command_t   p_comm;
	proc_name_t p_name;                                        
	uint8_t p_xhighbits;                                                               
	pid_t   p_contproc;                                             

	struct  pgrp *  XNU_PTRAUTH_SIGNED_PTR("proc.p_pgrp") p_pgrp;                                     
	uint32_t        p_csflags;                                   
	uint32_t        p_pcaction;                                                    
	uint8_t p_uuid[16];                                            

	  
                                                    
                                          
    
	cpu_type_t      p_cputype;
	cpu_subtype_t   p_cpusubtype;

	uint8_t  *syscall_filter_mask;                                                             
	uint32_t        p_platform;
	uint32_t        p_min_sdk;
	uint32_t        p_sdk;

                                          
                                                                     
#define p_endcopy       p_aio_total_count
	int             p_aio_total_count;                                                            
	TAILQ_HEAD(, aio_workq_entry ) p_aio_activeq;                                 
	TAILQ_HEAD(, aio_workq_entry ) p_aio_doneq;                                      

	struct klist p_klist;                        

	struct  rusage_superset *p_ru;                              
	thread_t        p_signalholder;
	thread_t        p_transholder;
	int             p_sigwaitcnt;
	                                
	u_short p_acflag;                              
	volatile u_short p_vfs_iopolicy;                                                  

	user_addr_t     p_threadstart;                                
	user_addr_t     p_wqthread;                                       
	int     p_pthsize;                                        
	uint32_t        p_pth_tsd_offset;                                                         
	user_addr_t     p_stack_addr_hint;                                                
	struct workqueue *_Atomic p_wqptr;                                     

	struct  timeval p_start;                                   
	void *  p_rcall;
	int             p_ractive;
	int     p_idversion;                                             
	void *  p_pthhash;                                                  
	volatile uint64_t was_throttled __attribute__((aligned(8)));                                           
	volatile uint64_t did_throttle __attribute__((aligned(8)));                                                      

#if DIAGNOSTIC
	unsigned int p_fdlock_pc[4];
	unsigned int p_fdunlock_pc[4];
#if SIGNAL_DEBUG
	unsigned int lockpc[8];
	unsigned int unlockpc[8];
#endif                   
#endif                 
	uint64_t        p_dispatchqueue_offset;
	uint64_t        p_dispatchqueue_serialno_offset;
	uint64_t        p_dispatchqueue_label_offset;
	uint64_t        p_return_to_kernel_offset;
	uint64_t        p_mach_thread_self_offset;
#if VM_PRESSURE_EVENTS
	struct timeval  vm_pressure_last_notify_tstamp;
#endif

#if CONFIG_MEMORYSTATUS
	                                        
	TAILQ_ENTRY(proc) p_memstat_list;                                         
	uint32_t          p_memstat_state;                                                                                              
	int32_t           p_memstat_effectivepriority;                                                      
	int32_t           p_memstat_requestedpriority;                       
	int32_t           p_memstat_assertionpriority;                                 
	uint32_t          p_memstat_dirty;                               
#if CONFIG_FREEZE
	uint8_t           p_memstat_freeze_skip_reason;                                                                           
#endif
	uint64_t          p_memstat_userdata;                           
	uint64_t          p_memstat_idledeadline;                                               
	uint64_t          p_memstat_idle_start;                                                             
	uint64_t          p_memstat_idle_delta;                                               
	int32_t           p_memstat_memlimit;                                                                                
	int32_t           p_memstat_memlimit_active;                                                                      
	int32_t           p_memstat_memlimit_inactive;                                                                      
	int32_t           p_memstat_relaunch_flags;                                                             
#if CONFIG_FREEZE
	uint32_t          p_memstat_freeze_sharedanon_pages;                                            
	uint32_t          p_memstat_frozen_count;
	uint32_t          p_memstat_thaw_count;
	uint32_t          p_memstat_last_thaw_interval;                                                      
#endif                    
#endif                          

	                                                              
	pid_t             p_responsible_pid;                                         
	_Atomic uint32_t  p_user_faults;                                                

	uint32_t          p_memlimit_increase;                                                                                                               

	struct os_reason     *p_exit_reason;

#if CONFIG_PROC_UDATA_STORAGE
	uint64_t        p_user_data;                                                                            
#endif                                

	char * p_subsystem_root_path;
	lck_rw_t        p_dirs_lock;                                                                          
	pid_t           p_sessionid;
};

  
                               
                                                           
   
struct proc_ident {
	uint64_t        p_uniqueid;
	pid_t           p_pid;
	int             p_idversion;
};

#define PGRPID_DEAD 0xdeaddead

                
#define P_LIST_DRAIN                    0x00000001
#define P_LIST_DRAINWAIT                0x00000002
#define P_LIST_DRAINED                  0x00000004
#define P_LIST_DEAD                             0x00000008
#define P_LIST_WAITING                  0x00000010
#define P_LIST_EXITED                   0x00000040
#define P_LIST_CHILDDRSTART     0x00000080
#define P_LIST_CHILDDRAINED     0x00000100
#define P_LIST_CHILDDRWAIT      0x00000200
#define P_LIST_CHILDLKWAIT      0x00000400
#define P_LIST_DEADPARENT       0x00000800
#define P_LIST_PARENTREFWAIT    0x00001000
#define P_LIST_INCREATE                 0x00002000
                               
#define P_LIST_INHASH                   0x00010000                              
#define P_LIST_INPGRP                   0x00020000                              
#define P_LIST_PGRPTRANS                0x00040000                                    
#define P_LIST_PGRPTRWAIT               0x00080000                                     
#define P_LIST_EXITCOUNT                0x00100000                                    
#define P_LIST_REFWAIT                  0x00200000                              


                 
#define P_LDELAYTERM    0x00000001           
#define P_LNOZOMB       0x00000002           
#define P_LTERM         0x00000004           
#define P_LEXIT         0x00000008           
#define P_LPEXIT        0x00000010
#define P_LTRANSCOMMIT  0x00000020                                         
#define P_LINTRANSIT    0x00000040                                          
#define P_LTRANSWAIT    0x00000080                                         
#define P_LVFORK        0x00000100                                  
#define P_LINVFORK      0x00000200                                 
#define P_LTRACED       0x00000400           
#define P_LSIGEXC       0x00000800           
#define P_LNOATTACH     0x00001000           
#define P_LPPWAIT       0x00002000           
#define P_LPTHREADJITALLOWLIST  0x00004000                                                            
                                                             
                                                             
#define P_LLIMCHANGE    0x00020000                                                               
#define P_LLIMWAIT      0x00040000
#define P_LWAITED       0x00080000
#define P_LINSIGNAL     0x00100000
#define P_LCUSTOM_STACK 0x00200000                                              
#define P_LRAGE_VNODES  0x00400000
#define P_LREGISTER     0x00800000                                        
#define P_LVMRSRCOWNER  0x01000000                                                 
#define P_LTERM_DECRYPTFAIL     0x04000000                                                             
#define P_LTERM_JETSAM          0x08000000                                     

#define P_JETSAM_VMPAGESHORTAGE 0x00000000                                                                               
#define P_JETSAM_VMTHRASHING    0x10000000                                                                           
#define P_JETSAM_HIWAT          0x20000000                                   
#define P_JETSAM_PID            0x30000000                       
#define P_JETSAM_IDLEEXIT       0x40000000                             
#define P_JETSAM_VNODE          0x50000000                              
#define P_JETSAM_FCTHRASHING    0x60000000                                                                                  
#define P_JETSAM_MASK           0x70000000                            
#define P_LNSPACE_RESOLVER      0x80000000                                             

                                                   
#define P_PCTHROTTLE    1
#define P_PCSUSP        2
#define P_PCKILL        3
#define P_PCMAX         3

                                                         
#define PROC_ACTION_MASK 0xffff0000;
#define PROC_CONTROL_STATE(p) (p->p_pcaction & P_PCMAX)
#define PROC_ACTION_STATE(p) ((p->p_pcaction >> 16) & P_PCMAX)
#define PROC_SETACTION_STATE(p) (p->p_pcaction = (PROC_CONTROL_STATE(p) | (PROC_CONTROL_STATE(p) << 16)))
#define PROC_RESETACTION_STATE(p) (p->p_pcaction = PROC_CONTROL_STATE(p))

                                
#define PROC_HAS_EXITREASON(p) (p->p_exit_reason != OS_REASON_NULL)
#define PROC_EXITREASON_FLAGS(p) p->p_exit_reason->osr_flags

                              
#define P_LADVLOCK              0x01
#define P_LXBKIDLEINPROG        0x02

                          
#define P_VFS_IOPOLICY_FORCE_HFS_CASE_SENSITIVITY       0x0001
#define P_VFS_IOPOLICY_ATIME_UPDATES                    0x0002
#define P_VFS_IOPOLICY_MATERIALIZE_DATALESS_FILES       0x0004
#define P_VFS_IOPOLICY_STATFS_NO_DATA_VOLUME            0x0008
#define P_VFS_IOPOLICY_TRIGGER_RESOLVE_DISABLE          0x0010
#define P_VFS_IOPOLICY_IGNORE_CONTENT_PROTECTION        0x0020
#define P_VFS_IOPOLICY_IGNORE_NODE_PERMISSIONS          0x0040
#define P_VFS_IOPOLICY_SKIP_MTIME_UPDATE                                0x0080
#define P_VFS_IOPOLICY_VALID_MASK                       (P_VFS_IOPOLICY_ATIME_UPDATES | P_VFS_IOPOLICY_FORCE_HFS_CASE_SENSITIVITY | P_VFS_IOPOLICY_MATERIALIZE_DATALESS_FILES | P_VFS_IOPOLICY_STATFS_NO_DATA_VOLUME | \
	        P_VFS_IOPOLICY_TRIGGER_RESOLVE_DISABLE | P_VFS_IOPOLICY_IGNORE_CONTENT_PROTECTION | P_VFS_IOPOLICY_IGNORE_NODE_PERMISSIONS | P_VFS_IOPOLICY_SKIP_MTIME_UPDATE)

                                
#define PROC_CREATE_FORK        0                                        
#define PROC_CREATE_SPAWN       1                                          
#define PROC_CREATE_VFORK       2                                  

                                             
                                                 
                                          
                                              
   

#ifdef KERNEL
#include <sys/time.h>                                     

  
                                                                                
                                                                              
                                                                       
                                                      
   
#if defined(__x86_64__)
#pragma pack(4)
#endif
struct user32_extern_proc {
	union {
		struct {
			uint32_t __p_forw;                                          
			uint32_t __p_back;
		} p_st1;
		struct user32_timeval __p_starttime;                            
	} p_un;
	uint32_t        p_vmspace;                          
	uint32_t        p_sigacts;                                              
	int             p_flag;                                 
	char    p_stat;                                         
	pid_t   p_pid;                                           
	pid_t   p_oppid;                                                        
	int             p_dupfd;                                                            
	                   
	uint32_t user_stack;                                        
	uint32_t exit_thread;                                    
	int             p_debugger;                                 
	boolean_t       sigwait;                                   
	                
	u_int   p_estcpu;                                               
	int             p_cpticks;                               
	fixpt_t p_pctcpu;                                                   
	uint32_t        p_wchan;                             
	uint32_t        p_wmesg;                                
	u_int   p_swtime;                                     
	u_int   p_slptime;                                     
	struct  user32_itimerval p_realtimer;                     
	struct  user32_timeval p_rtime;                 
	u_quad_t p_uticks;                                                
	u_quad_t p_sticks;                                                  
	u_quad_t p_iticks;                                                   
	int             p_traceflag;                                      
	uint32_t        p_tracep;                            
	int             p_siglist;                              
	uint32_t        p_textvp;                                 
	int             p_holdcnt;                                            
	sigset_t p_sigmask;                      
	sigset_t p_sigignore;                               
	sigset_t p_sigcatch;                                       
	u_char  p_priority;                            
	u_char  p_usrpri;                                                     
	char    p_nice;                                    
	char    p_comm[MAXCOMLEN + 1];
	uint32_t        p_pgrp;                                
	uint32_t        p_addr;                                                 
	u_short p_xstat;                                                     
	u_short p_acflag;                              
	uint32_t        p_ru;                              
};
#pragma pack()
struct user64_extern_proc {
	union {
		struct {
			user_addr_t __p_forw;                                       
			user_addr_t __p_back;
		} p_st1;
		struct user64_timeval __p_starttime;                            
	} p_un;
	user_addr_t     p_vmspace;                          
	user_addr_t             p_sigacts;                                              
	int             p_flag;                                 
	char    p_stat;                                         
	pid_t   p_pid;                                           
	pid_t   p_oppid;                                                        
	int             p_dupfd;                                                            
	                   
	user_addr_t user_stack __attribute((aligned(8)));                                           
	user_addr_t exit_thread;                                    
	int             p_debugger;                                 
	boolean_t       sigwait;                                   
	                
	u_int   p_estcpu;                                               
	int             p_cpticks;                               
	fixpt_t p_pctcpu;                                                   
	user_addr_t     p_wchan __attribute((aligned(8)));                           
	user_addr_t     p_wmesg;                                
	u_int   p_swtime;                                     
	u_int   p_slptime;                                     
	struct  user64_itimerval p_realtimer;                     
	struct  user64_timeval p_rtime;                 
	u_quad_t p_uticks;                                                
	u_quad_t p_sticks;                                                  
	u_quad_t p_iticks;                                                   
	int             p_traceflag;                                      
	user_addr_t     p_tracep __attribute((aligned(8)));                          
	int             p_siglist;                              
	user_addr_t     p_textvp __attribute((aligned(8)));                               
	int             p_holdcnt;                                            
	sigset_t p_sigmask;                      
	sigset_t p_sigignore;                               
	sigset_t p_sigcatch;                                       
	u_char  p_priority;                            
	u_char  p_usrpri;                                                     
	char    p_nice;                                    
	char    p_comm[MAXCOMLEN + 1];
	user_addr_t     p_pgrp __attribute((aligned(8)));                                      
	user_addr_t     p_addr;                                                 
	u_short p_xstat;                                                     
	u_short p_acflag;                              
	user_addr_t     p_ru __attribute((aligned(8)));                            
};
#endif              

#ifdef BSD_KERNEL_PRIVATE

#include <os/refcnt.h>                                        

  
                                            
                                                                                      
                                                                                  
                                                                                     
                                                                                     
                                                                              
                                      
   
struct plimit {
	struct  rlimit   pl_rlimit[RLIM_NLIMITS];
	os_refcnt_t      pl_refcnt;                                                           
};

extern rlim_t proc_limitgetcur(proc_t p, int which, boolean_t to_lock_proc);
extern void proc_limitsetcur_internal(proc_t p, int which, rlim_t value);
extern struct proc proc0;
#endif                         

  
                                                                       
                                                 
   
extern int nprocs, maxproc;                                                   
extern int maxprocperuid;                                                    
extern int hard_maxproc;                        
extern unsigned int proc_shutdown_exitcount;

#define PID_MAX         99999
#define NO_PID          100000
extern lck_mtx_t proc_list_mlock;

#define BSD_SIMUL_EXECS         33                              
#define BSD_PAGEABLE_SIZE_PER_EXEC      (NCARGS + PAGE_SIZE + PAGE_SIZE)                                                      
extern int execargs_cache_size;
extern int execargs_free_count;
extern vm_offset_t * execargs_cache;

#define SESS_LEADER(p, sessp)   ((sessp)->s_leader == (p))

#define PIDHASH(pid)    (&pidhashtbl[(pid) & pidhash])
extern LIST_HEAD(pidhashhead, proc) * pidhashtbl;
extern u_long pidhash;

#define PGRPHASH(pgid)  (&pgrphashtbl[(pgid) & pgrphash])
extern LIST_HEAD(pgrphashhead, pgrp) * pgrphashtbl;
extern u_long pgrphash;
#define SESSHASH(sessid) (&sesshashtbl[(sessid) & sesshash])
extern LIST_HEAD(sesshashhead, session) * sesshashtbl;
extern u_long sesshash;

extern lck_attr_t proc_lck_attr;
extern lck_grp_t proc_fdmlock_grp;
extern lck_grp_t proc_lck_grp;
extern lck_grp_t proc_kqhashlock_grp;
extern lck_grp_t proc_knhashlock_grp;
extern lck_grp_t proc_slock_grp;
extern lck_grp_t proc_mlock_grp;
extern lck_grp_t proc_ucred_mlock_grp;
extern lck_grp_t proc_dirslock_grp;

LIST_HEAD(proclist, proc);
extern struct proclist allproc;                                     
extern struct proclist zombproc;                                       

extern struct proc * XNU_PTRAUTH_SIGNED_PTR("initproc") initproc;
extern void procinit(void);
extern void proc_lock(struct proc *);
extern void proc_unlock(struct proc *);
extern void proc_spinlock(struct proc *);
extern void proc_spinunlock(struct proc *);
extern void proc_list_lock(void);
extern void proc_list_unlock(void);
extern void proc_klist_lock(void);
extern void proc_klist_unlock(void);
extern void proc_fdlock(struct proc *);
extern void proc_fdlock_spin(struct proc *);
extern void proc_fdunlock(struct proc *);
extern void proc_fdlock_assert(proc_t p, int assertflags);
extern void proc_dirs_lock_shared(struct proc *);
extern void proc_dirs_unlock_shared(struct proc *);
extern void proc_dirs_lock_exclusive(struct proc *);
extern void proc_dirs_unlock_exclusive(struct proc *);
extern void proc_ucred_lock(struct proc *);
extern void proc_ucred_unlock(struct proc *);
__private_extern__ int proc_core_name(const char *name, uid_t uid, pid_t pid,
    char *cr_name, size_t cr_name_len);
extern int isinferior(struct proc *, struct proc *);
__private_extern__ struct proc *pzfind(pid_t);                          
__private_extern__ struct proc *proc_find_zombref(pid_t);                               
__private_extern__ void proc_drop_zombref(struct proc * p);                             

extern size_t   chgproccnt(uid_t uid, int diff);
extern void     pinsertchild(struct proc *parent, struct proc *child);
extern int      setsid_internal(struct proc *p);
#ifndef __cplusplus
extern void     setlogin_internal(proc_t p, const char login[static MAXLOGNAME]);
#endif               
extern int      setgroups_internal(proc_t p, u_int gidsetsize, gid_t *gidset, uid_t gmuid);
extern int      enterpgrp(struct proc *p, pid_t pgid, int mksess);
extern void     fixjobc(struct proc *p, struct pgrp *pgrp, int entering);
extern int      inferior(struct proc *p);
extern int      leavepgrp(struct proc *p);
extern void     resetpriority(struct proc *);
extern void     setrunnable(struct proc *);
extern void     setrunqueue(struct proc *);
extern int      sleep(void *chan, int pri);
extern int      tsleep0(void *chan, int pri, const char *wmesg, int timo, int (*continuation)(int));
extern int      tsleep1(void *chan, int pri, const char *wmesg, u_int64_t abstime, int (*continuation)(int));
extern int      msleep0(void *chan, lck_mtx_t *mtx, int pri, const char *wmesg, int timo, int (*continuation)(int));
extern void     vfork_return(struct proc *child, int32_t *retval, int rval);
extern int      exit1(struct proc *, int, int *);
extern int      exit1_internal(struct proc *, int, int *, boolean_t, boolean_t, int);
extern int      exit_with_reason(struct proc *, int, int *, boolean_t, boolean_t, int, struct os_reason *);
extern int      fork1(proc_t, thread_t *, int, coalition_t *);
extern void vfork_exit_internal(struct proc *p, int rv, int forced);
extern void proc_reparentlocked(struct proc *child, struct proc * newparent, int cansignal, int locked);

extern proc_t proc_findinternal(int pid, int locked);
extern proc_t proc_findthread(thread_t thread);
extern void proc_refdrain(proc_t);
extern proc_t proc_refdrain_with_refwait(proc_t p, boolean_t get_ref_and_allow_wait);
extern void proc_refwake(proc_t p);
extern void proc_childdrainlocked(proc_t);
extern void proc_childdrainstart(proc_t);
extern void proc_childdrainend(proc_t);
extern void  proc_checkdeadrefs(proc_t);
struct proc *pfind_locked(pid_t);
extern struct pgrp *pgfind(pid_t);
extern void pg_rele(struct pgrp * pgrp);
extern struct session * session_find_internal(pid_t sessid);
extern struct pgrp * proc_pgrp(proc_t);
extern struct pgrp * tty_pgrp(struct tty * tp);
extern struct pgrp * pgfind_internal(pid_t);
extern struct session * proc_session(proc_t);
extern void pgrp_lock(struct pgrp * pgrp);
extern void pgrp_unlock(struct pgrp * pgrp);
extern void session_lock(struct session * sess);
extern void session_unlock(struct session * sess);
extern struct session * pgrp_session(struct pgrp * pgrp);
extern void     session_rele(struct session *sess);
extern int isbackground(proc_t p, struct tty  *tp);
extern proc_t proc_parentholdref(proc_t);
extern int proc_parentdropref(proc_t, int);
int     itimerfix(struct timeval *tv);
int     itimerdecr(struct proc * p, struct itimerval *itp, int usec);
void    proc_free_realitimer(proc_t proc);
int  timespec_is_valid(const struct timespec *);
void proc_signalstart(struct proc *, int locked);
void proc_signalend(struct proc *, int locked);
int  proc_transstart(struct proc *, int locked, int non_blocking);
void proc_transcommit(struct proc *, int locked);
void proc_transend(struct proc *, int locked);
int  proc_transwait(struct proc *, int locked);
void  proc_rele_locked(struct proc *  p);
struct proc *proc_ref_locked(struct proc *  p);
void proc_knote(struct proc * p, long hint);
void proc_knote_drain(struct proc *p);
void proc_setregister(proc_t p);
void proc_resetregister(proc_t p);
bool proc_get_pthread_jit_allowlist(proc_t p);
void proc_set_pthread_jit_allowlist(proc_t p);
                                                                                
thread_t proc_thread(proc_t);
extern int proc_pendingsignals(proc_t, sigset_t);
int proc_getpcontrol(int pid, int * pcontrolp);
int proc_dopcontrol(proc_t p);
int proc_resetpcontrol(int pid);
#if PSYNCH
void pth_proc_hashinit(proc_t);
void pth_proc_hashdelete(proc_t);
void pth_global_hashinit(void);
extern thread_call_t psynch_thcall;
void psynch_wq_cleanup(__unused void *  param, __unused void * param1);
extern lck_mtx_t * pthread_list_mlock;
#endif             
struct uthread * current_uthread(void);


                       

#define ALLPROC_FOREACH(var) \
	LIST_FOREACH((var), &allproc, p_list)

#define ZOMBPROC_FOREACH(var) \
	LIST_FOREACH((var), &zombproc, p_list)

#define PGMEMBERS_FOREACH(group, var) \
	LIST_FOREACH((var), &((struct pgrp *)(group))->pg_members, p_pglist)

#define PCHILDREN_FOREACH(parent, var) \
	LIST_FOREACH((var), &(((struct proc *)(parent))->p_children), p_sibling)

typedef int (*proc_iterate_fn_t)(proc_t, void *);

  
                                                                            
                     
  
                                                                         
                                          
   
#define PROC_RETURNED      (0)
#define PROC_RETURNED_DONE (1)
#define PROC_CLAIMED       (2)
#define PROC_CLAIMED_DONE  (3)

  
                                                                         
                                                                         
                                                                              
                                                                              
                                              
  
                                                                          
                  
   
#define PGRP_DROPREF (1)

extern void pgrp_iterate(struct pgrp *pgrp, unsigned int flags, proc_iterate_fn_t callout, void *arg, proc_iterate_fn_t filterfn, void *filterarg);

  
                                                                               
                                                                              
                                                                               
                                            
  
                                                                       
                                                
   
#define PROC_ALLPROCLIST  (1U << 0)                                                       
#define PROC_ZOMBPROCLIST (1U << 1)                           
#define PROC_NOWAITTRANS  (1U << 2)                                                   

extern void proc_iterate(unsigned int flags, proc_iterate_fn_t callout, void *arg, proc_iterate_fn_t filterfn, void *filterarg);

  
                                                                             
            
  
                                                                               
                    
   
extern void proc_childrenwalk(proc_t p, proc_iterate_fn_t callout, void *arg);

  
                                                         
   
extern void proc_rebootscan(proc_iterate_fn_t callout, void *arg, proc_iterate_fn_t filterfn, void *filterarg);

pid_t dtrace_proc_selfpid(void);
pid_t dtrace_proc_selfppid(void);
uid_t dtrace_proc_selfruid(void);

extern zone_t proc_zone;
extern zone_t proc_stats_zone;
extern zone_t proc_sigacts_zone;

extern struct proc_ident proc_ident(proc_t p);

  
                                                                   
                 
   
bool proc_ignores_node_permissions(proc_t proc);

#endif                              
