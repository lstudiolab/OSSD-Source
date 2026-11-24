/*
 * Copyright (c) 2000-2005, 2020 Apple Computer, Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
   

#include <mach/mach_types.h>
#include <mach/mach_traps.h>

#include <kern/syscall_sw.h>
#if CONFIG_REQUIRES_U32_MUNGING || (__arm__ && (__BIGGEST_ALIGNMENT__ > 4))
#include <sys/munge.h>
#endif

              


  
                      
                                                                                               
          
                                                       
                                                                                           
                                                                                      
                                                                                     
                                                                                
  
                                                      
                           
  
  
                                                        
                                                                
                                                            
                         
  
                                                             
                                                
   

int kern_invalid_debug = 0;

                                                 

#include <mach/mach_traps.h>
#include <mach/mach_syscalls.h>
#include <kern/syscall_subr.h>

#include <kern/clock.h>
#include <mach/mk_timer.h>

const mach_trap_t       mach_trap_table[MACH_TRAP_TABLE_COUNT] = {
        MACH_TRAP(kern_invalid, 0, 0, NULL),
        MACH_TRAP(kern_invalid, 0, 0, NULL),
        MACH_TRAP(kern_invalid, 0, 0, NULL),
        MACH_TRAP(kern_invalid, 0, 0, NULL),
        MACH_TRAP(kern_invalid, 0, 0, NULL),
        MACH_TRAP(kern_invalid, 0, 0, NULL),
        MACH_TRAP(kern_invalid, 0, 0, NULL),
        MACH_TRAP(kern_invalid, 0, 0, NULL),
        MACH_TRAP(kern_invalid, 0, 0, NULL),
        MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(_kernelrpc_mach_vm_allocate_trap, 4, 5, munge_wwlw),
         MACH_TRAP(_kernelrpc_mach_vm_purgable_control_trap, 4, 5, munge_wlww),
         MACH_TRAP(_kernelrpc_mach_vm_deallocate_trap, 3, 5, munge_wll),
         MACH_TRAP(task_dyld_process_info_notify_get_trap, 2, 4, munge_ll),
         MACH_TRAP(_kernelrpc_mach_vm_protect_trap, 5, 7, munge_wllww),
         MACH_TRAP(_kernelrpc_mach_vm_map_trap, 6, 8, munge_wwllww),
         MACH_TRAP(_kernelrpc_mach_port_allocate_trap, 3, 3, munge_www),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(_kernelrpc_mach_port_deallocate_trap, 2, 2, munge_ww),
         MACH_TRAP(_kernelrpc_mach_port_mod_refs_trap, 4, 4, munge_wwww),
         MACH_TRAP(_kernelrpc_mach_port_move_member_trap, 3, 3, munge_www),
         MACH_TRAP(_kernelrpc_mach_port_insert_right_trap, 4, 4, munge_wwww),
         MACH_TRAP(_kernelrpc_mach_port_insert_member_trap, 3, 3, munge_www),
         MACH_TRAP(_kernelrpc_mach_port_extract_member_trap, 3, 3, munge_www),
         MACH_TRAP(_kernelrpc_mach_port_construct_trap, 4, 5, munge_wwlw),
         MACH_TRAP(_kernelrpc_mach_port_destruct_trap, 4, 5, munge_wwwl),
         MACH_TRAP(mach_reply_port, 0, 0, NULL),
         MACH_TRAP(thread_self_trap, 0, 0, NULL),
         MACH_TRAP(task_self_trap, 0, 0, NULL),
         MACH_TRAP(host_self_trap, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(mach_msg_trap, 7, 7, munge_wwwwwww),
         MACH_TRAP(mach_msg_overwrite_trap, 8, 8, munge_wwwwwwww),
         MACH_TRAP(semaphore_signal_trap, 1, 1, munge_w),
         MACH_TRAP(semaphore_signal_all_trap, 1, 1, munge_w),
         MACH_TRAP(semaphore_signal_thread_trap, 2, 2, munge_ww),
         MACH_TRAP(semaphore_wait_trap, 1, 1, munge_w),
         MACH_TRAP(semaphore_wait_signal_trap, 2, 2, munge_ww),
         MACH_TRAP(semaphore_timedwait_trap, 3, 3, munge_www),
         MACH_TRAP(semaphore_timedwait_signal_trap, 4, 4, munge_wwww),
         MACH_TRAP(_kernelrpc_mach_port_get_attributes_trap, 5, 5, munge_wwwww),
         MACH_TRAP(_kernelrpc_mach_port_guard_trap, 4, 5, munge_wwlw),
         MACH_TRAP(_kernelrpc_mach_port_unguard_trap, 3, 4, munge_wwl),
         MACH_TRAP(mach_generate_activity_id, 3, 3, munge_www),
         MACH_TRAP(task_name_for_pid, 3, 3, munge_www),
         MACH_TRAP(task_for_pid, 3, 3, munge_www),
         MACH_TRAP(pid_for_task, 2, 2, munge_ww),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(macx_swapon, 4, 5, munge_lwww),
         MACH_TRAP(macx_swapoff, 2, 3, munge_lw),
         MACH_TRAP(thread_get_special_reply_port, 0, 0, NULL),
         MACH_TRAP(macx_triggers, 4, 4, munge_wwww),
         MACH_TRAP(macx_backing_store_suspend, 1, 1, munge_w),
         MACH_TRAP(macx_backing_store_recovery, 1, 1, munge_w),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(pfz_exit, 0, 0, NULL),
         MACH_TRAP(swtch_pri, 0, 0, NULL),
         MACH_TRAP(swtch, 0, 0, NULL),
         MACH_TRAP(thread_switch, 3, 3, munge_www),
         MACH_TRAP(clock_sleep_trap, 5, 5, munge_wwwww),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(host_create_mach_voucher_trap, 4, 4, munge_wwww),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(mach_voucher_extract_attr_recipe_trap, 4, 4, munge_wwww),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(_kernelrpc_mach_port_type_trap, 3, 3, munge_wwww),
         MACH_TRAP(_kernelrpc_mach_port_request_notification_trap, 7, 7, munge_wwwwwww),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(mach_timebase_info_trap, 1, 1, munge_w),
         MACH_TRAP(mach_wait_until_trap, 1, 2, munge_l),
         MACH_TRAP(mk_timer_create_trap, 0, 0, NULL),
         MACH_TRAP(mk_timer_destroy_trap, 1, 1, munge_w),
         MACH_TRAP(mk_timer_arm_trap, 2, 3, munge_wl),
         MACH_TRAP(mk_timer_cancel_trap, 2, 2, munge_ww),
         MACH_TRAP(mk_timer_arm_leeway_trap, 4, 6, munge_wlll),
         MACH_TRAP(debug_control_port_for_pid, 3, 3, munge_www),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
         MACH_TRAP(kern_invalid, 0, 0, NULL),
                                      
          MACH_TRAP(iokit_user_client_trap, 8, 8, munge_wwwwwwww),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
          MACH_TRAP(kern_invalid, 0, 0, NULL),
};

const char * const mach_syscall_name_table[MACH_TRAP_TABLE_COUNT] = {
        "kern_invalid",
        "kern_invalid",
        "kern_invalid",
        "kern_invalid",
        "kern_invalid",
        "kern_invalid",
        "kern_invalid",
        "kern_invalid",
        "kern_invalid",
        "kern_invalid",
         "_kernelrpc_mach_vm_allocate_trap",
         "kern_invalid",
         "_kernelrpc_mach_vm_deallocate_trap",
         "task_dyld_process_info_notify_get_trap",
         "_kernelrpc_mach_vm_protect_trap",
         "_kernelrpc_mach_vm_map_trap",
         "_kernelrpc_mach_port_allocate_trap",
         "kern_invalid",
         "_kernelrpc_mach_port_deallocate_trap",
         "_kernelrpc_mach_port_mod_refs_trap",
         "_kernelrpc_mach_port_move_member_trap",
         "_kernelrpc_mach_port_insert_right_trap",
         "_kernelrpc_mach_port_insert_member_trap",
         "_kernelrpc_mach_port_extract_member_trap",
         "_kernelrpc_mach_port_construct_trap",
         "_kernelrpc_mach_port_destruct_trap",
         "mach_reply_port",
         "thread_self_trap",
         "task_self_trap",
         "host_self_trap",
         "kern_invalid",
         "mach_msg_trap",
         "mach_msg_overwrite_trap",
         "semaphore_signal_trap",
         "semaphore_signal_all_trap",
         "semaphore_signal_thread_trap",
         "semaphore_wait_trap",
         "semaphore_wait_signal_trap",
         "semaphore_timedwait_trap",
         "semaphore_timedwait_signal_trap",
         "kern_invalid",
         "_kernelrpc_mach_port_guard_trap",
         "_kernelrpc_mach_port_unguard_trap",
         "mach_generate_activity_id",
         "task_name_for_pid",
         "task_for_pid",
         "pid_for_task",
         "kern_invalid",
         "macx_swapon",
         "macx_swapoff",
         "thread_get_special_reply_port",
         "macx_triggers",
         "macx_backing_store_suspend",
         "macx_backing_store_recovery",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "pfz_exit",
         "swtch_pri",
         "swtch",
         "thread_switch",
         "clock_sleep_trap",
         "kern_invalid",
                                   
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "host_create_mach_voucher_trap",
         "kern_invalid",
         "mach_voucher_extract_attr_recipe_trap",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "_kernelrpc_mach_port_type_trap",
         "_kernelrpc_mach_port_request_notification_trap",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
         "mach_timebase_info_trap",
         "mach_wait_until_trap",
         "mk_timer_create_trap",
         "mk_timer_destroy_trap",
         "mk_timer_arm_trap",
         "mk_timer_cancel_trap",
         "kern_invalid",
                                   
         "debug_control_port_for_pid",
         "kern_invalid",
         "kern_invalid",
         "kern_invalid",
                                            
          "iokit_user_client_trap",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
                          
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
          "kern_invalid",
};

const int mach_trap_count = (sizeof(mach_trap_table) / sizeof(mach_trap_table[0]));

kern_return_t
kern_invalid(
	__unused struct kern_invalid_args *args)
{
	if (kern_invalid_debug) {
		Debugger("kern_invalid mach trap");
	}
	return KERN_INVALID_ARGUMENT;
}
