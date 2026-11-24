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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *	@(#)kern_clock.c	8.5 (Berkeley) 1/21/94
 */
  
          
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/resource.h>
#include <sys/proc_internal.h>
#include <sys/vm.h>
#include <sys/sysctl.h>

#include <kern/thread.h>
#include <kern/ast.h>
#include <kern/assert.h>
#include <mach/boolean.h>

#include <kern/thread_call.h>

void bsd_uprofil(struct time_value *syst, user_addr_t pc);
int tvtohz(struct timeval *tv);

  
                           
  
                                                            
                                                             
                                                                       
                                                                      
                                                                       
                                                                 
                                                                  
                                                                        
                                                                          
                                               
   

  
                                  
   

int             hz = 100;                                         
int             tick = (1000000 / 100);                           

  
                           
   

  
                 
  
                         
                                        
                                     
   
void
timeout(
	timeout_fcn_t                   fcn,
	void                                    *param,
	int                                             interval)
{
	uint64_t                deadline;

	clock_interval_to_deadline(interval, NSEC_PER_SEC / hz, &deadline);
	thread_call_func_delayed((thread_call_func_t)fcn, param, deadline);
}

  
                             
  
                         
                                        
                                     
                                           
   
void
timeout_with_leeway(
	timeout_fcn_t                   fcn,
	void                                    *param,
	int                                             interval,
	int                                             leeway_interval)
{
	uint64_t                deadline;
	uint64_t                leeway;

	clock_interval_to_deadline(interval, NSEC_PER_SEC / hz, &deadline);

	clock_interval_to_absolutetime_interval(leeway_interval, NSEC_PER_SEC / hz, &leeway);

	thread_call_func_delayed_with_leeway((thread_call_func_t)fcn, param, deadline, leeway, THREAD_CALL_DELAY_LEEWAY);
}

  
                    
                                            
                                              
   
void
untimeout(
	timeout_fcn_t           fcn,
	void                    *param)
{
	thread_call_func_cancel((thread_call_func_t)fcn, param, FALSE);
}


  
                 
  
                         
                                        
                                     
   
void
bsd_timeout(
	timeout_fcn_t                   fcn,
	void                                    *param,
	struct timespec         *ts)
{
	uint64_t                deadline = 0;

	if (ts && (ts->tv_sec || ts->tv_nsec)) {
		nanoseconds_to_absolutetime((uint64_t)ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec, &deadline );
		clock_absolutetime_interval_to_deadline( deadline, &deadline );
	}
	thread_call_func_delayed((thread_call_func_t)fcn, param, deadline);
}

  
                    
                                            
                                              
   
void
bsd_untimeout(
	timeout_fcn_t           fcn,
	void                    *param)
{
	thread_call_func_cancel((thread_call_func_t)fcn, param, FALSE);
}


  
                                             
                                                      
                 
   
int
hzto(struct timeval *tv)
{
	struct timeval now;
	long ticks;
	long sec;

	microtime(&now);
	  
                                                            
                                                            
                                                                 
                                                      
   
                                                              
                                                            
    
	sec = tv->tv_sec - now.tv_sec;
	if (sec <= 0x7fffffff / 1000 - 1000) {
		ticks = ((tv->tv_sec - now.tv_sec) * 1000 +
		    (tv->tv_usec - now.tv_usec) / 1000)
		    / (tick / 1000);
	} else if (sec <= 0x7fffffff / hz) {
		ticks = sec * hz;
	} else {
		ticks = 0x7fffffff;
	}

	return (int)ticks;
}

  
                                          
   
static int
sysctl_clockrate
(__unused struct sysctl_oid *oidp, __unused void *arg1, __unused int arg2, __unused struct sysctl_req *req)
{
	struct clockinfo clkinfo = {
		.hz         = hz,
		.tick       = tick,
		.tickadj    = 0,
		.stathz     = hz,
		.profhz     = hz,
	};

	return sysctl_io_opaque(req, &clkinfo, sizeof(clkinfo), NULL);
}

SYSCTL_PROC(_kern, KERN_CLOCKRATE, clockrate,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED,
    0, 0, sysctl_clockrate, "S,clockinfo", "");


  
                                                           
   
int
tvtohz(struct timeval *tv)
{
	unsigned long ticks;
	long sec, usec;

	  
                                                                
                                                                  
                                                                 
                                                                 
                                                                 
                      
   
                                                                  
                                                                 
                                                                
                                                               
                                                                   
   
                                                            
                        
   
                                                                   
                           
    
	sec = tv->tv_sec;
	usec = tv->tv_usec;
	if (usec < 0) {
		sec--;
		usec += 1000000;
	}
	if (sec < 0) {
#ifdef DIAGNOSTIC
		if (usec > 0) {
			sec++;
			usec -= 1000000;
		}
		printf("tvotohz: negative time difference %ld sec %ld usec\n",
		    sec, usec);
#endif
		ticks = 1;
	} else if (sec <= LONG_MAX / 1000000) {
		ticks = (sec * 1000000 + (unsigned long)usec + (tick - 1))
		    / tick + 1;
	} else if (sec <= LONG_MAX / hz) {
		ticks = sec * hz
		    + ((unsigned long)usec + (tick - 1)) / tick + 1;
	} else {
		ticks = LONG_MAX;
	}
	if (ticks > INT_MAX) {
		ticks = INT_MAX;
	}
	return (int)ticks;
}

                                                    
void
get_procrustime(time_value_t *tv)
{
	struct proc *p = current_proc();
	struct timeval st;

	if (p == NULL) {
		return;
	}
	if (!(p->p_flag & P_PROFIL)) {
		return;
	}

	               
	st = p->p_stats->p_ru.ru_stime;
	                 

	tv->seconds = (integer_t)st.tv_sec;
	tv->microseconds = st.tv_usec;
}
