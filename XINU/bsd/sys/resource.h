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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1993
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
 *	@(#)resource.h	8.2 (Berkeley) 1/4/94
 */

#ifndef _SYS_RESOURCE_H_
#define _SYS_RESOURCE_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>
#include <sys/_types.h>

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#include <stdint.h>
#endif                                          

#ifndef KERNEL
#include <Availability.h>
#endif

                                                               
               
   
#include <sys/_types/_timeval.h>
#ifdef KERNEL
#include <sys/_types/_user32_timeval.h>
#include <sys/_types/_user64_timeval.h>
#endif

                                                                  
#include <sys/_types/_id_t.h>


  
                                                            
   
typedef __uint64_t      rlim_t;


      
           
   

  
                                                                         
                                                     
   
#define PRIO_PROCESS    0                                             
#define PRIO_PGRP       1                                             
#define PRIO_USER       2                                             

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define PRIO_DARWIN_THREAD      3                                                                 
#define PRIO_DARWIN_PROCESS     4                                             

#ifdef PRIVATE

#define PRIO_DARWIN_GPU         5                                             

#define PRIO_DARWIN_GPU_ALLOW   0x1
#define PRIO_DARWIN_GPU_DENY    0x2

#define PRIO_DARWIN_ROLE        6                                             

#define PRIO_DARWIN_ROLE_DEFAULT        0x0                                 
#define PRIO_DARWIN_ROLE_UI_FOCAL       0x1                                   
#define PRIO_DARWIN_ROLE_UI             0x2                                        
#define PRIO_DARWIN_ROLE_NON_UI         0x3                                   
#define PRIO_DARWIN_ROLE_UI_NON_FOCAL   0x4                                   
#define PRIO_DARWIN_ROLE_TAL_LAUNCH     0x5                                                 
#define PRIO_DARWIN_ROLE_DARWIN_BG      0x6                                                  

#endif              

  
                                                                           
   
#define PRIO_MIN        -20
#define PRIO_MAX        20

  
                                                                       
                                                                              
                                   
   
#define PRIO_DARWIN_BG 0x1000

  
                                                                         
                        
   
#define PRIO_DARWIN_NONUI 0x1001

#endif                                           



      
                 
   

  
                                                                          
                                               
   
#define RUSAGE_SELF     0                                                
#define RUSAGE_CHILDREN -1                                             

  
                                                                       
                                                                      
               
  
                                                                       
                                                                      
                                                         
   
struct  rusage {
	struct timeval ru_utime;                                 
	struct timeval ru_stime;                                   
#if __DARWIN_C_LEVEL < __DARWIN_C_FULL
	long    ru_opaque[14];                                      
#else
	  
                                                                
                                                               
                                             
    
	long    ru_maxrss;                                              
#define ru_first        ru_ixrss                                           
	long    ru_ixrss;                                                     
	long    ru_idrss;                                                 
	long    ru_isrss;                                                 
	long    ru_minflt;                                      
	long    ru_majflt;                                    
	long    ru_nswap;                               
	long    ru_inblock;                                                  
	long    ru_oublock;                                                   
	long    ru_msgsnd;                                          
	long    ru_msgrcv;                                              
	long    ru_nsignals;                                           
	long    ru_nvcsw;                                                        
	long    ru_nivcsw;                                 
#define ru_last         ru_nivcsw                                        
#endif                                           
};

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
  
                                 
   
#define RUSAGE_INFO_V0  0
#define RUSAGE_INFO_V1  1
#define RUSAGE_INFO_V2  2
#define RUSAGE_INFO_V3  3
#define RUSAGE_INFO_V4  4
#define RUSAGE_INFO_V5  5
#define RUSAGE_INFO_CURRENT     RUSAGE_INFO_V5

  
                           
   
#define RU_PROC_RUNS_RESLIDE    0x00000001                                        

typedef void *rusage_info_t;

struct rusage_info_v0 {
	uint8_t  ri_uuid[16];
	uint64_t ri_user_time;
	uint64_t ri_system_time;
	uint64_t ri_pkg_idle_wkups;
	uint64_t ri_interrupt_wkups;
	uint64_t ri_pageins;
	uint64_t ri_wired_size;
	uint64_t ri_resident_size;
	uint64_t ri_phys_footprint;
	uint64_t ri_proc_start_abstime;
	uint64_t ri_proc_exit_abstime;
};

struct rusage_info_v1 {
	uint8_t  ri_uuid[16];
	uint64_t ri_user_time;
	uint64_t ri_system_time;
	uint64_t ri_pkg_idle_wkups;
	uint64_t ri_interrupt_wkups;
	uint64_t ri_pageins;
	uint64_t ri_wired_size;
	uint64_t ri_resident_size;
	uint64_t ri_phys_footprint;
	uint64_t ri_proc_start_abstime;
	uint64_t ri_proc_exit_abstime;
	uint64_t ri_child_user_time;
	uint64_t ri_child_system_time;
	uint64_t ri_child_pkg_idle_wkups;
	uint64_t ri_child_interrupt_wkups;
	uint64_t ri_child_pageins;
	uint64_t ri_child_elapsed_abstime;
};

struct rusage_info_v2 {
	uint8_t  ri_uuid[16];
	uint64_t ri_user_time;
	uint64_t ri_system_time;
	uint64_t ri_pkg_idle_wkups;
	uint64_t ri_interrupt_wkups;
	uint64_t ri_pageins;
	uint64_t ri_wired_size;
	uint64_t ri_resident_size;
	uint64_t ri_phys_footprint;
	uint64_t ri_proc_start_abstime;
	uint64_t ri_proc_exit_abstime;
	uint64_t ri_child_user_time;
	uint64_t ri_child_system_time;
	uint64_t ri_child_pkg_idle_wkups;
	uint64_t ri_child_interrupt_wkups;
	uint64_t ri_child_pageins;
	uint64_t ri_child_elapsed_abstime;
	uint64_t ri_diskio_bytesread;
	uint64_t ri_diskio_byteswritten;
};

struct rusage_info_v3 {
	uint8_t  ri_uuid[16];
	uint64_t ri_user_time;
	uint64_t ri_system_time;
	uint64_t ri_pkg_idle_wkups;
	uint64_t ri_interrupt_wkups;
	uint64_t ri_pageins;
	uint64_t ri_wired_size;
	uint64_t ri_resident_size;
	uint64_t ri_phys_footprint;
	uint64_t ri_proc_start_abstime;
	uint64_t ri_proc_exit_abstime;
	uint64_t ri_child_user_time;
	uint64_t ri_child_system_time;
	uint64_t ri_child_pkg_idle_wkups;
	uint64_t ri_child_interrupt_wkups;
	uint64_t ri_child_pageins;
	uint64_t ri_child_elapsed_abstime;
	uint64_t ri_diskio_bytesread;
	uint64_t ri_diskio_byteswritten;
	uint64_t ri_cpu_time_qos_default;
	uint64_t ri_cpu_time_qos_maintenance;
	uint64_t ri_cpu_time_qos_background;
	uint64_t ri_cpu_time_qos_utility;
	uint64_t ri_cpu_time_qos_legacy;
	uint64_t ri_cpu_time_qos_user_initiated;
	uint64_t ri_cpu_time_qos_user_interactive;
	uint64_t ri_billed_system_time;
	uint64_t ri_serviced_system_time;
};

struct rusage_info_v4 {
	uint8_t  ri_uuid[16];
	uint64_t ri_user_time;
	uint64_t ri_system_time;
	uint64_t ri_pkg_idle_wkups;
	uint64_t ri_interrupt_wkups;
	uint64_t ri_pageins;
	uint64_t ri_wired_size;
	uint64_t ri_resident_size;
	uint64_t ri_phys_footprint;
	uint64_t ri_proc_start_abstime;
	uint64_t ri_proc_exit_abstime;
	uint64_t ri_child_user_time;
	uint64_t ri_child_system_time;
	uint64_t ri_child_pkg_idle_wkups;
	uint64_t ri_child_interrupt_wkups;
	uint64_t ri_child_pageins;
	uint64_t ri_child_elapsed_abstime;
	uint64_t ri_diskio_bytesread;
	uint64_t ri_diskio_byteswritten;
	uint64_t ri_cpu_time_qos_default;
	uint64_t ri_cpu_time_qos_maintenance;
	uint64_t ri_cpu_time_qos_background;
	uint64_t ri_cpu_time_qos_utility;
	uint64_t ri_cpu_time_qos_legacy;
	uint64_t ri_cpu_time_qos_user_initiated;
	uint64_t ri_cpu_time_qos_user_interactive;
	uint64_t ri_billed_system_time;
	uint64_t ri_serviced_system_time;
	uint64_t ri_logical_writes;
	uint64_t ri_lifetime_max_phys_footprint;
	uint64_t ri_instructions;
	uint64_t ri_cycles;
	uint64_t ri_billed_energy;
	uint64_t ri_serviced_energy;
	uint64_t ri_interval_max_phys_footprint;
	uint64_t ri_runnable_time;
};

struct rusage_info_v5 {
	uint8_t  ri_uuid[16];
	uint64_t ri_user_time;
	uint64_t ri_system_time;
	uint64_t ri_pkg_idle_wkups;
	uint64_t ri_interrupt_wkups;
	uint64_t ri_pageins;
	uint64_t ri_wired_size;
	uint64_t ri_resident_size;
	uint64_t ri_phys_footprint;
	uint64_t ri_proc_start_abstime;
	uint64_t ri_proc_exit_abstime;
	uint64_t ri_child_user_time;
	uint64_t ri_child_system_time;
	uint64_t ri_child_pkg_idle_wkups;
	uint64_t ri_child_interrupt_wkups;
	uint64_t ri_child_pageins;
	uint64_t ri_child_elapsed_abstime;
	uint64_t ri_diskio_bytesread;
	uint64_t ri_diskio_byteswritten;
	uint64_t ri_cpu_time_qos_default;
	uint64_t ri_cpu_time_qos_maintenance;
	uint64_t ri_cpu_time_qos_background;
	uint64_t ri_cpu_time_qos_utility;
	uint64_t ri_cpu_time_qos_legacy;
	uint64_t ri_cpu_time_qos_user_initiated;
	uint64_t ri_cpu_time_qos_user_interactive;
	uint64_t ri_billed_system_time;
	uint64_t ri_serviced_system_time;
	uint64_t ri_logical_writes;
	uint64_t ri_lifetime_max_phys_footprint;
	uint64_t ri_instructions;
	uint64_t ri_cycles;
	uint64_t ri_billed_energy;
	uint64_t ri_serviced_energy;
	uint64_t ri_interval_max_phys_footprint;
	uint64_t ri_runnable_time;
	uint64_t ri_flags;
};

typedef struct rusage_info_v5 rusage_info_current;

#endif                                          

#ifdef KERNEL

struct rusage_superset {
	struct rusage                   ru;
	rusage_info_current     ri;
};

struct rusage_info_child {
	uint64_t ri_child_user_time;
	uint64_t ri_child_system_time;
	uint64_t ri_child_pkg_idle_wkups;
	uint64_t ri_child_interrupt_wkups;
	uint64_t ri_child_pageins;
	uint64_t ri_child_elapsed_abstime;
};

struct  user64_rusage {
	struct user64_timeval ru_utime;                     
	struct user64_timeval ru_stime;                       
	user64_long_t   ru_maxrss;                                         
	user64_long_t   ru_ixrss;                                                
	user64_long_t   ru_idrss;                                             
	user64_long_t   ru_isrss;                                              
	user64_long_t   ru_minflt;                                 
	user64_long_t   ru_majflt;                               
	user64_long_t   ru_nswap;                          
	user64_long_t   ru_inblock;                                         
	user64_long_t   ru_oublock;                                          
	user64_long_t   ru_msgsnd;                                 
	user64_long_t   ru_msgrcv;                                     
	user64_long_t   ru_nsignals;                          
	user64_long_t   ru_nvcsw;                                               
	user64_long_t   ru_nivcsw;                                 
};

struct  user32_rusage {
	struct user32_timeval ru_utime;                     
	struct user32_timeval ru_stime;                       
	user32_long_t   ru_maxrss;                                         
	user32_long_t   ru_ixrss;                                                
	user32_long_t   ru_idrss;                                             
	user32_long_t   ru_isrss;                                              
	user32_long_t   ru_minflt;                                 
	user32_long_t   ru_majflt;                               
	user32_long_t   ru_nswap;                          
	user32_long_t   ru_inblock;                                         
	user32_long_t   ru_oublock;                                          
	user32_long_t   ru_msgsnd;                                 
	user32_long_t   ru_msgrcv;                                     
	user32_long_t   ru_nsignals;                          
	user32_long_t   ru_nvcsw;                                               
	user32_long_t   ru_nivcsw;                                 
};

#endif             


      
                  
   

  
                                                                             
                                                                        
                 
   
#define RLIM_INFINITY   (((__uint64_t)1 << 63) - 1)                   
#define RLIM_SAVED_MAX  RLIM_INFINITY                                   
#define RLIM_SAVED_CUR  RLIM_INFINITY                                   

  
                                                                        
                                                                
   
#define RLIMIT_CPU      0                                         
#define RLIMIT_FSIZE    1                              
#define RLIMIT_DATA     2                                      
#define RLIMIT_STACK    3                               
#define RLIMIT_CORE     4                                   
#define RLIMIT_AS       5                                                      
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define RLIMIT_RSS      RLIMIT_AS                                       
#define RLIMIT_MEMLOCK  6                                                   
#define RLIMIT_NPROC    7                                        
#endif                                           
#define RLIMIT_NOFILE   8                                         
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define RLIM_NLIMITS    9                                                    
#endif                                           
#define _RLIMIT_POSIX_FLAG      0x1000                                

  
                                                                         
                                                                        
   
struct rlimit {
	rlim_t  rlim_cur;                                         
	rlim_t  rlim_max;                                               
};

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
  
                        
  
                         
   
#define RLIMIT_WAKEUPS_MONITOR          0x1                                     
#define RLIMIT_CPU_USAGE_MONITOR        0x2                                       
#define RLIMIT_THREAD_CPULIMITS         0x3                                                    
#define RLIMIT_FOOTPRINT_INTERVAL       0x4                                                   

  
                                     
   
#define WAKEMON_ENABLE                  0x01
#define WAKEMON_DISABLE                 0x02
#define WAKEMON_GET_PARAMS              0x04
#define WAKEMON_SET_DEFAULTS            0x08
#define WAKEMON_MAKE_FATAL              0x10                                                       

  
                                       
   
#define CPUMON_MAKE_FATAL               0x1000

  
                                                
   
#define FOOTPRINT_INTERVAL_RESET        0x1                                                   

struct proc_rlimit_control_wakeupmon {
	uint32_t wm_flags;
	int32_t wm_rate;
};

#if PRIVATE
  
                                 
   
#define IOMON_ENABLE                    0x01
#define IOMON_DISABLE                   0x02

#endif              


              
#define IOPOL_TYPE_DISK 0
#if PRIVATE
#define IOPOL_TYPE_VFS_HFS_CASE_SENSITIVITY 1
#endif
#define IOPOL_TYPE_VFS_ATIME_UPDATES 2
#define IOPOL_TYPE_VFS_MATERIALIZE_DATALESS_FILES 3
#define IOPOL_TYPE_VFS_STATFS_NO_DATA_VOLUME 4
#define IOPOL_TYPE_VFS_TRIGGER_RESOLVE 5
#define IOPOL_TYPE_VFS_IGNORE_CONTENT_PROTECTION 6
#define IOPOL_TYPE_VFS_IGNORE_PERMISSIONS 7
#define IOPOL_TYPE_VFS_SKIP_MTIME_UPDATE 8

           
#define IOPOL_SCOPE_PROCESS   0
#define IOPOL_SCOPE_THREAD    1
#define IOPOL_SCOPE_DARWIN_BG 2

                  
#define IOPOL_DEFAULT           0
#define IOPOL_IMPORTANT         1
#define IOPOL_PASSIVE           2
#define IOPOL_THROTTLE          3
#define IOPOL_UTILITY           4
#define IOPOL_STANDARD          5

                                    
#define IOPOL_APPLICATION       IOPOL_STANDARD
#define IOPOL_NORMAL            IOPOL_IMPORTANT

#if PRIVATE
#define IOPOL_VFS_HFS_CASE_SENSITIVITY_DEFAULT  0
#define IOPOL_VFS_HFS_CASE_SENSITIVITY_FORCE_CASE_SENSITIVE     1
#endif

#define IOPOL_ATIME_UPDATES_DEFAULT     0
#define IOPOL_ATIME_UPDATES_OFF         1

#define IOPOL_MATERIALIZE_DATALESS_FILES_DEFAULT 0
#define IOPOL_MATERIALIZE_DATALESS_FILES_OFF     1
#define IOPOL_MATERIALIZE_DATALESS_FILES_ON      2

#define IOPOL_VFS_STATFS_NO_DATA_VOLUME_DEFAULT 0
#define IOPOL_VFS_STATFS_FORCE_NO_DATA_VOLUME   1

#define IOPOL_VFS_TRIGGER_RESOLVE_DEFAULT 0
#define IOPOL_VFS_TRIGGER_RESOLVE_OFF     1

#define IOPOL_VFS_CONTENT_PROTECTION_DEFAULT 0
#define IOPOL_VFS_CONTENT_PROTECTION_IGNORE  1

#define IOPOL_VFS_IGNORE_PERMISSIONS_OFF 0
#define IOPOL_VFS_IGNORE_PERMISSIONS_ON  1

#define IOPOL_VFS_SKIP_MTIME_UPDATE_OFF 0
#define IOPOL_VFS_SKIP_MTIME_UPDATE_ON 1

#ifdef PRIVATE
  
                                                                             
                                                     
   

  
                               
   
#define IOPOL_CMD_GET           0x00000001                          
#define IOPOL_CMD_SET           0x00000002                          

  
                                    
   
struct _iopol_param_t {
	int iop_scope;                                   
	int iop_iotype;
	int iop_policy;
};

#endif               
#endif                                          

#ifndef KERNEL

__BEGIN_DECLS
int     getpriority(int, id_t);
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
int     getiopolicy_np(int, int) __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_2_0);
#endif                                          
int     getrlimit(int, struct rlimit *) __DARWIN_ALIAS(getrlimit);
int     getrusage(int, struct rusage *);
int     setpriority(int, id_t, int);
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
int     setiopolicy_np(int, int, int) __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_2_0);
#endif                                          
int     setrlimit(int, const struct rlimit *) __DARWIN_ALIAS(setrlimit);
__END_DECLS

#endif               
#endif                         
