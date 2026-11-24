/*
 * Copyright (c) 2000-2016 Apple Inc. All rights reserved.
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
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/assert.h>
#include <kern/clock.h>
#include <kern/locks.h>
#include <kern/sched_prim.h>
#include <kern/debug.h>
#include <mach/machine/thread_status.h>
#include <mach/thread_act.h>

#include <sys/kernel.h>
#include <sys/vm.h>
#include <sys/proc_internal.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/kdebug.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/kauth.h>
#include <sys/systm.h>
#include <sys/bitstring.h>

#include <security/audit/audit.h>

#include <i386/seg.h>
#include <i386/machine_routines.h>
#include <mach/i386/syscall_sw.h>

#include <machine/pal_routines.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#if CONFIG_DTRACE
extern int32_t dtrace_systrace_syscall(struct proc *, void *, int *);
extern void dtrace_systrace_syscall_return(unsigned short, int, int *);
#endif

extern void unix_syscall(x86_saved_state_t *);
extern void unix_syscall64(x86_saved_state_t *);
extern void *find_user_regs(thread_t);

                                                                  
extern const char *syscallnames[];

#define code_is_kdebug_trace(code) (((code) == SYS_kdebug_trace) ||   \
	                            ((code) == SYS_kdebug_trace64) || \
	                            ((code) == SYS_kdebug_trace_string))

  
                         
  
                                           
  
                
   
__attribute__((noreturn))
void
unix_syscall(x86_saved_state_t *state)
{
	thread_t                thread;
	void                    *vt;
	unsigned int            code, syscode;
	const struct sysent     *callp;

	int                     error;
	vm_offset_t             params;
	struct proc             *p;
	struct uthread          *uthread;
	x86_saved_state32_t     *regs;
	boolean_t               is_vfork;
	pid_t                   pid;

	assert(is_saved_state32(state));
	regs = saved_state32(state);
#if DEBUG
	if (regs->eax == 0x800) {
		thread_exception_return();
	}
#endif
	thread = current_thread();
	uthread = get_bsdthread_info(thread);

	uthread_reset_proc_refcount(uthread);

	                                                                       
	is_vfork = uthread->uu_flag & UT_VFORK;
	if (__improbable(is_vfork != 0)) {
		p = current_proc();
	} else {
		p = (struct proc *)get_bsdtask_info(current_task());
	}

	code    = regs->eax & I386_SYSCALL_NUMBER_MASK;
	syscode = (code < nsysent) ? code : SYS_invalid;
	DEBUG_KPRINT_SYSCALL_UNIX("unix_syscall: code=%d(%s) eip=%u\n",
	    code, syscallnames[syscode], (uint32_t)regs->eip);
	params = (vm_offset_t) (regs->uesp + sizeof(int));

	regs->efl &= ~(EFL_CF);

	callp = &sysent[syscode];

	if (__improbable(callp == sysent)) {
		code = fuword(params);
		params += sizeof(int);
		syscode = (code < nsysent) ? code : SYS_invalid;
		callp = &sysent[syscode];
	}

	vt = (void *)uthread->uu_arg;

	if (callp->sy_arg_bytes != 0) {
#if CONFIG_REQUIRES_U32_MUNGING
		sy_munge_t      *mungerp;
#else
#error U32 syscalls on x86_64 kernel requires munging
#endif
		uint32_t         nargs;

		assert((unsigned) callp->sy_arg_bytes <= sizeof(uthread->uu_arg));
		nargs = callp->sy_arg_bytes;
		error = copyin((user_addr_t) params, (char *) vt, nargs);
		if (error) {
			regs->eax = error;
			regs->efl |= EFL_CF;
			thread_exception_return();
			                
		}

		if (__probable(!code_is_kdebug_trace(code))) {
			uint32_t *uip = vt;
			KDBG_RELEASE(BSDDBG_CODE(DBG_BSD_EXCP_SC, code) | DBG_FUNC_START,
			    uip[0], uip[1], uip[2], uip[3]);
		}

#if CONFIG_REQUIRES_U32_MUNGING
		mungerp = callp->sy_arg_munge32;

		if (mungerp != NULL) {
			(*mungerp)(vt);
		}
#endif
	} else {
		KDBG_RELEASE(BSDDBG_CODE(DBG_BSD_EXCP_SC, code) | DBG_FUNC_START);
	}

	  
                                                                     
                                                             
    
	kauth_cred_uthread_update(uthread, p);

	uthread->uu_rval[0] = 0;
	uthread->uu_rval[1] = 0;
	uthread->uu_flag |= UT_NOTCANCELPT;
	uthread->syscall_code = code;
	pid = proc_pid(p);

#ifdef JOE_DEBUG
	uthread->uu_iocount = 0;
	uthread->uu_vpindex = 0;
#endif

#if CONFIG_MACF
	if (__improbable(p->syscall_filter_mask != NULL && !bitstr_test(p->syscall_filter_mask, syscode))) {
		error = mac_proc_check_syscall_unix(p, syscode);
		if (error) {
			goto skip_syscall;
		}
	}
#endif                  

	AUDIT_SYSCALL_ENTER(code, p, uthread);
	error = (*(callp->sy_call))((void *) p, (void *) vt, &(uthread->uu_rval[0]));
	AUDIT_SYSCALL_EXIT(code, p, uthread, error);

#if CONFIG_MACF
skip_syscall:
#endif                  

#ifdef JOE_DEBUG
	if (uthread->uu_iocount) {
		printf("system call returned with uu_iocount != 0\n");
	}
#endif
#if CONFIG_DTRACE
	uthread->t_dtrace_errno = error;
#endif                    

	if (__improbable(error == ERESTART)) {
		  
                                                   
                                                
                                                              
                                              
     

		pal_syscall_restart(thread, state);
	} else if (error != EJUSTRETURN) {
		if (__improbable(error)) {
			regs->eax = error;
			regs->efl |= EFL_CF;                   
		} else {                  
			   
                                                        
                                                       
                                             
       
			regs->eax = uthread->uu_rval[0];
			regs->edx = uthread->uu_rval[1];
		}
	}

	DEBUG_KPRINT_SYSCALL_UNIX(
		"unix_syscall: error=%d retval=(%u,%u)\n",
		error, regs->eax, regs->edx);

	uthread->uu_flag &= ~UT_NOTCANCELPT;
	uthread->syscall_code = 0;

#if DEBUG || DEVELOPMENT
	kern_allocation_name_t
	prior __assert_only = thread_set_allocation_name(NULL);
	assertf(prior == NULL, "thread_set_allocation_name(\"%s\") not cleared", kern_allocation_get_name(prior));
#endif                           

	if (__improbable(uthread->uu_lowpri_window)) {
		  
                                              
                                                    
                                                 
                                                  
                                               
     
		throttle_lowpri_io(1);
	}
	if (__probable(!code_is_kdebug_trace(code))) {
		KDBG_RELEASE(BSDDBG_CODE(DBG_BSD_EXCP_SC, code) | DBG_FUNC_END,
		    error, uthread->uu_rval[0], uthread->uu_rval[1], pid);
	}

	if (__improbable(!is_vfork && callp->sy_call == (sy_call_t *)execve && !error)) {
		pal_execve_return(thread);
	}

#if PROC_REF_DEBUG
	if (__improbable(uthread_get_proc_refcount(uthread) != 0)) {
		panic("system call returned with uu_proc_refcount != 0");
	}
#endif

	thread_exception_return();
	                
}

__attribute__((noreturn))
void
unix_syscall64(x86_saved_state_t *state)
{
	thread_t        thread;
	void                    *vt;
	unsigned int    code, syscode;
	const struct sysent   *callp;
	int             args_in_regs;
	boolean_t       args_start_at_rdi;
	int             error;
	struct proc     *p;
	struct uthread  *uthread;
	x86_saved_state64_t *regs;
	pid_t           pid;

	assert(is_saved_state64(state));
	regs = saved_state64(state);
#if     DEBUG
	if (regs->rax == 0x2000800) {
		thread_exception_return();
	}
#endif
	thread = current_thread();
	uthread = get_bsdthread_info(thread);

	uthread_reset_proc_refcount(uthread);

	                                                                       
	if (__probable(!(uthread->uu_flag & UT_VFORK))) {
		p = (struct proc *)get_bsdtask_info(current_task());
	} else {
		p = current_proc();
	}

	                                                                    
	if (__improbable(p == NULL)) {
		regs->rax = EPERM;
		regs->isf.rflags |= EFL_CF;
		task_terminate_internal(current_task());
		thread_exception_return();
		                
	}

	code    = regs->rax & SYSCALL_NUMBER_MASK;
	syscode = (code < nsysent) ? code : SYS_invalid;
	DEBUG_KPRINT_SYSCALL_UNIX(
		"unix_syscall64: code=%d(%s) rip=%llx\n",
		code, syscallnames[syscode], regs->isf.rip);
	callp = &sysent[syscode];

	vt = (void *)uthread->uu_arg;

	if (__improbable(callp == sysent)) {
		  
                                               
                     
     
		code    = regs->rdi;
		syscode = (code < nsysent) ? code : SYS_invalid;
		callp   = &sysent[syscode];
		args_start_at_rdi = FALSE;
		args_in_regs = 5;
	} else {
		args_start_at_rdi = TRUE;
		args_in_regs = 6;
	}

	if (callp->sy_narg != 0) {
		assert(callp->sy_narg <= 8);                     

		args_in_regs = MIN(args_in_regs, callp->sy_narg);
		memcpy(vt, args_start_at_rdi ? &regs->rdi : &regs->rsi, args_in_regs * sizeof(syscall_arg_t));

		if (!code_is_kdebug_trace(code)) {
			uint64_t *uip = vt;

			KDBG_RELEASE(BSDDBG_CODE(DBG_BSD_EXCP_SC, code) | DBG_FUNC_START,
			    uip[0], uip[1], uip[2], uip[3]);
		}

		if (__improbable(callp->sy_narg > args_in_regs)) {
			int copyin_count;

			copyin_count = (callp->sy_narg - args_in_regs) * sizeof(syscall_arg_t);

			error = copyin((user_addr_t)(regs->isf.rsp + sizeof(user_addr_t)), (char *)&uthread->uu_arg[args_in_regs], copyin_count);
			if (error) {
				regs->rax = error;
				regs->isf.rflags |= EFL_CF;
				thread_exception_return();
				                
			}
		}
	} else {
		KDBG_RELEASE(BSDDBG_CODE(DBG_BSD_EXCP_SC, code) | DBG_FUNC_START);
	}

	  
                                                                     
                                                             
    
	kauth_cred_uthread_update(uthread, p);

	uthread->uu_rval[0] = 0;
	uthread->uu_rval[1] = 0;
	uthread->uu_flag |= UT_NOTCANCELPT;
	uthread->syscall_code = code;
	pid = proc_pid(p);

#ifdef JOE_DEBUG
	uthread->uu_iocount = 0;
	uthread->uu_vpindex = 0;
#endif

#if CONFIG_MACF
	if (__improbable(p->syscall_filter_mask != NULL && !bitstr_test(p->syscall_filter_mask, syscode))) {
		error = mac_proc_check_syscall_unix(p, syscode);
		if (error) {
			goto skip_syscall;
		}
	}
#endif                  

	AUDIT_SYSCALL_ENTER(code, p, uthread);
	error = (*(callp->sy_call))((void *) p, vt, &(uthread->uu_rval[0]));
	AUDIT_SYSCALL_EXIT(code, p, uthread, error);

#if CONFIG_MACF
skip_syscall:
#endif                  

#ifdef JOE_DEBUG
	if (uthread->uu_iocount) {
		printf("system call returned with uu_iocount != 0\n");
	}
#endif

#if CONFIG_DTRACE
	uthread->t_dtrace_errno = error;
#endif                    

	if (__improbable(error == ERESTART)) {
		  
                                                              
                                            
                                                   
     
		pal_syscall_restart( thread, state );
	} else if (error != EJUSTRETURN) {
		if (__improbable(error)) {
			regs->rax = error;
			regs->isf.rflags |= EFL_CF;                    
		} else {                  
			switch (callp->sy_return_type) {
			case _SYSCALL_RET_INT_T:
				regs->rax = uthread->uu_rval[0];
				regs->rdx = uthread->uu_rval[1];
				break;
			case _SYSCALL_RET_UINT_T:
				regs->rax = ((u_int)uthread->uu_rval[0]);
				regs->rdx = ((u_int)uthread->uu_rval[1]);
				break;
			case _SYSCALL_RET_OFF_T:
			case _SYSCALL_RET_ADDR_T:
			case _SYSCALL_RET_SIZE_T:
			case _SYSCALL_RET_SSIZE_T:
			case _SYSCALL_RET_UINT64_T:
				regs->rax = *((uint64_t *)(&uthread->uu_rval[0]));
				regs->rdx = 0;
				break;
			case _SYSCALL_RET_NONE:
				break;
			default:
				panic("unix_syscall: unknown return type");
				break;
			}
			regs->isf.rflags &= ~EFL_CF;
		}
	}

	DEBUG_KPRINT_SYSCALL_UNIX(
		"unix_syscall64: error=%d retval=(%llu,%llu)\n",
		error, regs->rax, regs->rdx);

	uthread->uu_flag &= ~UT_NOTCANCELPT;
	uthread->syscall_code = 0;

#if DEBUG || DEVELOPMENT
	kern_allocation_name_t
	prior __assert_only = thread_set_allocation_name(NULL);
	assertf(prior == NULL, "thread_set_allocation_name(\"%s\") not cleared", kern_allocation_get_name(prior));
#endif                           

	if (__improbable(uthread->uu_lowpri_window)) {
		  
                                              
                                                    
                                                 
                                                  
                                               
     
		throttle_lowpri_io(1);
	}
	if (__probable(!code_is_kdebug_trace(code))) {
		KDBG_RELEASE(BSDDBG_CODE(DBG_BSD_EXCP_SC, code) | DBG_FUNC_END,
		    error, uthread->uu_rval[0], uthread->uu_rval[1], pid);
	}

#if PROC_REF_DEBUG
	if (__improbable(uthread_get_proc_refcount(uthread))) {
		panic("system call returned with uu_proc_refcount != 0");
	}
#endif

	thread_exception_return();
	                
}


void
unix_syscall_return(int error)
{
	thread_t                thread;
	struct uthread          *uthread;
	struct proc *p;
	unsigned int code;
	const struct sysent *callp;

	thread = current_thread();
	uthread = get_bsdthread_info(thread);

	pal_register_cache_state(thread, DIRTY);

	p = current_proc();

	if (proc_is64bit(p)) {
		x86_saved_state64_t *regs;

		regs = saved_state64(find_user_regs(thread));

		code = uthread->syscall_code;
		callp = (code >= nsysent) ? &sysent[SYS_invalid] : &sysent[code];

#if CONFIG_DTRACE
		if (callp->sy_call == dtrace_systrace_syscall) {
			dtrace_systrace_syscall_return( code, error, uthread->uu_rval );
		}
#endif                    
		AUDIT_SYSCALL_EXIT(code, p, uthread, error);

		if (error == ERESTART) {
			  
                        
      
			pal_syscall_restart( thread, find_user_regs(thread));
		} else if (error != EJUSTRETURN) {
			if (error) {
				regs->rax = error;
				regs->isf.rflags |= EFL_CF;                    
			} else {                  
				switch (callp->sy_return_type) {
				case _SYSCALL_RET_INT_T:
					regs->rax = uthread->uu_rval[0];
					regs->rdx = uthread->uu_rval[1];
					break;
				case _SYSCALL_RET_UINT_T:
					regs->rax = ((u_int)uthread->uu_rval[0]);
					regs->rdx = ((u_int)uthread->uu_rval[1]);
					break;
				case _SYSCALL_RET_OFF_T:
				case _SYSCALL_RET_ADDR_T:
				case _SYSCALL_RET_SIZE_T:
				case _SYSCALL_RET_SSIZE_T:
				case _SYSCALL_RET_UINT64_T:
					regs->rax = *((uint64_t *)(&uthread->uu_rval[0]));
					regs->rdx = 0;
					break;
				case _SYSCALL_RET_NONE:
					break;
				default:
					panic("unix_syscall: unknown return type");
					break;
				}
				regs->isf.rflags &= ~EFL_CF;
			}
		}
		DEBUG_KPRINT_SYSCALL_UNIX(
			"unix_syscall_return: error=%d retval=(%llu,%llu)\n",
			error, regs->rax, regs->rdx);
	} else {
		x86_saved_state32_t     *regs;

		regs = saved_state32(find_user_regs(thread));

		regs->efl &= ~(EFL_CF);

		code = uthread->syscall_code;
		callp = (code >= nsysent) ? &sysent[SYS_invalid] : &sysent[code];

#if CONFIG_DTRACE
		if (callp->sy_call == dtrace_systrace_syscall) {
			dtrace_systrace_syscall_return( code, error, uthread->uu_rval );
		}
#endif                    
		AUDIT_SYSCALL_EXIT(code, p, uthread, error);

		if (error == ERESTART) {
			pal_syscall_restart( thread, find_user_regs(thread));
		} else if (error != EJUSTRETURN) {
			if (error) {
				regs->eax = error;
				regs->efl |= EFL_CF;                   
			} else {                  
				regs->eax = uthread->uu_rval[0];
				regs->edx = uthread->uu_rval[1];
			}
		}
		DEBUG_KPRINT_SYSCALL_UNIX(
			"unix_syscall_return: error=%d retval=(%u,%u)\n",
			error, regs->eax, regs->edx);
	}


	uthread->uu_flag &= ~UT_NOTCANCELPT;

#if DEBUG || DEVELOPMENT
	kern_allocation_name_t
	prior __assert_only = thread_set_allocation_name(NULL);
	assertf(prior == NULL, "thread_set_allocation_name(\"%s\") not cleared", kern_allocation_get_name(prior));
#endif                           

	if (uthread->uu_lowpri_window) {
		  
                                              
                                                    
                                                 
                                                  
                                               
     
		throttle_lowpri_io(1);
	}
	if (!code_is_kdebug_trace(code)) {
		KDBG_RELEASE(BSDDBG_CODE(DBG_BSD_EXCP_SC, code) | DBG_FUNC_END,
		    error, uthread->uu_rval[0], uthread->uu_rval[1], p->p_pid);
	}

	thread_exception_return();
	                
}
