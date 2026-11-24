/*
 * Copyright (c) 2013 Apple Computer, Inc. All rights reserved.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc_internal.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/unistd.h>
#include <sys/priv.h>

#include <mach/mach_types.h>
#include <mach/vm_param.h>
#include <kern/task.h>
#include <kern/locks.h>
#include <kern/assert.h>
#include <kern/sched_prim.h>

#include <sys/kern_overrides.h>
#include <sys/bsdtask_info.h>
#include <sys/kdebug.h>
#include <sys/sysproto.h>
#include <sys/msgbuf.h>
#include <sys/kern_memorystatus.h>

                                            
static LCK_GRP_DECLARE(sys_override_mtx_grp, "system_override");
static LCK_MTX_DECLARE(sys_override_lock, &sys_override_mtx_grp);

  
                                                                               
  
                                                                     
  
                                                                 
  
                                                                            
                                                                        
                                          
  
                                                                             
                                                                          
  
                                                                          
                                                                           
                                                                            
  
                                                                          
                                                                            
                                                                            
                                
  
   
static int64_t          io_throttle_assert_cnt;
static int64_t          cpu_throttle_assert_cnt;
static int64_t          fast_jetsam_assert_cnt;

                                      
static uint64_t         sys_override_wait;

                     
static void system_override_begin(uint64_t flags);
static void system_override_end(uint64_t flags);
static void system_override_abort(uint64_t flags);
static void system_override_callouts(uint64_t flags, boolean_t enable_override);
static __attribute__((noinline)) int PROCESS_OVERRIDING_SYSTEM_DEFAULTS(uint64_t timeout);

                                
int
system_override(__unused struct proc *p, struct system_override_args * uap, __unused int32_t *retval)
{
	uint64_t timeout = uap->timeout;
	uint64_t flags = uap->flags;
	int error = 0;

	                                                                                          
	if ((error = priv_check_cred(kauth_cred_get(), PRIV_SYSTEM_OVERRIDE, 0))) {
		goto out;
	}

	                                               
	if ((flags & ~SYS_OVERRIDE_FLAGS_MASK) != 0) {
		error = EINVAL;
		goto out;
	}

	lck_mtx_lock(&sys_override_lock);

	if (flags & SYS_OVERRIDE_DISABLE) {
		flags &= ~SYS_OVERRIDE_DISABLE;
		system_override_abort(flags);
	} else {
		system_override_begin(flags);
		error = PROCESS_OVERRIDING_SYSTEM_DEFAULTS(timeout);
		system_override_end(flags);
	}

	lck_mtx_unlock(&sys_override_lock);

out:
	return error;
}

  
                                                                                  
                                                                                  
                                                                   
  
                                      
                                                                                   
                                                                                  
                          
  
                                                      
                                                                                  
                                                                                  
                                                                                    
                                                                                  
                                                                              
                                           
   

static void
system_override_callouts(uint64_t flags, boolean_t enable_override)
{
	switch (flags) {
	case SYS_OVERRIDE_IO_THROTTLE:
		if (enable_override) {
			KERNEL_DEBUG_CONSTANT(FSDBG_CODE(DBG_THROTTLE, IO_THROTTLE_DISABLE) | DBG_FUNC_START,
			    current_proc()->p_pid, 0, 0, 0, 0);
		} else {
			KERNEL_DEBUG_CONSTANT(FSDBG_CODE(DBG_THROTTLE, IO_THROTTLE_DISABLE) | DBG_FUNC_END,
			    current_proc()->p_pid, 0, 0, 0, 0);
		}
		sys_override_io_throttle(enable_override);
		break;

	case SYS_OVERRIDE_CPU_THROTTLE:
		if (enable_override) {
			KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_CPU_THROTTLE_DISABLE) | DBG_FUNC_START,
			    current_proc()->p_pid, 0, 0, 0, 0);
		} else {
			KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_CPU_THROTTLE_DISABLE) | DBG_FUNC_END,
			    current_proc()->p_pid, 0, 0, 0, 0);
		}
		sys_override_cpu_throttle(enable_override);
		break;

	case SYS_OVERRIDE_FAST_JETSAM:
		if (enable_override) {
			KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_FAST_JETSAM) | DBG_FUNC_START,
			    current_proc()->p_pid, 0, 0, 0, 0);
		} else {
			KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_FAST_JETSAM) | DBG_FUNC_END,
			    current_proc()->p_pid, 0, 0, 0, 0);
		}
#if CONFIG_JETSAM
		memorystatus_fast_jetsam_override(enable_override);
#endif                    
		break;

	default:
		panic("Unknown option to system_override_callouts(): %llu\n", flags);
	}
}

  
                                        
  
                                                            
                                                   
   
static void
system_override_begin(uint64_t flags)
{
	lck_mtx_assert(&sys_override_lock, LCK_MTX_ASSERT_OWNED);

	if (flags & SYS_OVERRIDE_IO_THROTTLE) {
		if (io_throttle_assert_cnt == 0) {
			system_override_callouts(SYS_OVERRIDE_IO_THROTTLE, true);
		}
		io_throttle_assert_cnt++;
	}

	if (flags & SYS_OVERRIDE_CPU_THROTTLE) {
		if (cpu_throttle_assert_cnt == 0) {
			system_override_callouts(SYS_OVERRIDE_CPU_THROTTLE, true);
		}
		cpu_throttle_assert_cnt++;
	}

	if (flags & SYS_OVERRIDE_FAST_JETSAM) {
		if (fast_jetsam_assert_cnt == 0) {
			system_override_callouts(SYS_OVERRIDE_FAST_JETSAM, true);
		}
		fast_jetsam_assert_cnt++;
	}
}

  
                                      
  
                                                          
                                                   
   
static void
system_override_end(uint64_t flags)
{
	lck_mtx_assert(&sys_override_lock, LCK_MTX_ASSERT_OWNED);

	if (flags & SYS_OVERRIDE_IO_THROTTLE) {
		assert(io_throttle_assert_cnt > 0);
		io_throttle_assert_cnt--;
		if (io_throttle_assert_cnt == 0) {
			system_override_callouts(SYS_OVERRIDE_IO_THROTTLE, false);
		}
	}

	if (flags & SYS_OVERRIDE_CPU_THROTTLE) {
		assert(cpu_throttle_assert_cnt > 0);
		cpu_throttle_assert_cnt--;
		if (cpu_throttle_assert_cnt == 0) {
			system_override_callouts(SYS_OVERRIDE_CPU_THROTTLE, false);
		}
	}

	if (flags & SYS_OVERRIDE_FAST_JETSAM) {
		assert(fast_jetsam_assert_cnt > 0);
		fast_jetsam_assert_cnt--;
		if (fast_jetsam_assert_cnt == 0) {
			system_override_callouts(SYS_OVERRIDE_FAST_JETSAM, false);
		}
	}
}

  
                                        
  
                                                         
                                                             
              
   
static void
system_override_abort(uint64_t flags)
{
	lck_mtx_assert(&sys_override_lock, LCK_MTX_ASSERT_OWNED);

	if ((flags & SYS_OVERRIDE_IO_THROTTLE) && (io_throttle_assert_cnt > 0)) {
		system_override_callouts(SYS_OVERRIDE_IO_THROTTLE, false);
	}

	if ((flags & SYS_OVERRIDE_CPU_THROTTLE) && (cpu_throttle_assert_cnt > 0)) {
		system_override_callouts(SYS_OVERRIDE_CPU_THROTTLE, false);
	}

	if ((flags & SYS_OVERRIDE_FAST_JETSAM) && (fast_jetsam_assert_cnt > 0)) {
		system_override_callouts(SYS_OVERRIDE_FAST_JETSAM, false);
	}
}

static __attribute__((noinline)) int
PROCESS_OVERRIDING_SYSTEM_DEFAULTS(uint64_t timeout)
{
	struct timespec ts;
	ts.tv_sec = timeout / NSEC_PER_SEC;
	ts.tv_nsec = timeout - ((long)ts.tv_sec * NSEC_PER_SEC);
	int error = msleep((caddr_t)&sys_override_wait, &sys_override_lock, PRIBIO | PCATCH, "system_override", &ts);
	                                                                          
	return (error == EWOULDBLOCK) ? 0 : error;
}
