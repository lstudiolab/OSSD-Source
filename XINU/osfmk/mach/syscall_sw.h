/*
 * Copyright (c) 2000-2004 Apple Computer, Inc. All rights reserved.
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

#ifdef	PRIVATE

#ifndef	_MACH_SYSCALL_SW_H_
#define _MACH_SYSCALL_SW_H_

  
                                                   
                     
                                                  
                                               
        
  
                                                            
   

#include <mach/machine/syscall_sw.h>

  
                                              
                                
   

   
                                                
                                               
                                                 
                                               
                                                   
   
kernel_trap(_kernelrpc_mach_vm_allocate_trap,-10,5)                                    
kernel_trap(_kernelrpc_mach_vm_purgable_control_trap,-11,5)                                      
kernel_trap(_kernelrpc_mach_vm_deallocate_trap,-12,5)                                                          
kernel_trap(task_dyld_process_info_notify_get,-13,4)                                       
kernel_trap(_kernelrpc_mach_vm_protect_trap,-14,7)                                                          
kernel_trap(_kernelrpc_mach_vm_map_trap,-15,9)
kernel_trap(_kernelrpc_mach_port_allocate_trap,-16,3)
                       
kernel_trap(_kernelrpc_mach_port_deallocate_trap,-18,2)
kernel_trap(_kernelrpc_mach_port_mod_refs_trap,-19,4)
kernel_trap(_kernelrpc_mach_port_move_member_trap,-20,3)
kernel_trap(_kernelrpc_mach_port_insert_right_trap,-21,4)
kernel_trap(_kernelrpc_mach_port_insert_member_trap,-22,3)
kernel_trap(_kernelrpc_mach_port_extract_member_trap,-23,3)
kernel_trap(_kernelrpc_mach_port_construct_trap,-24,5)
kernel_trap(_kernelrpc_mach_port_destruct_trap,-25,5)

kernel_trap(mach_reply_port,-26,0)
kernel_trap(thread_self_trap,-27,0)
kernel_trap(task_self_trap,-28,0)
kernel_trap(host_self_trap,-29,0)

kernel_trap(mach_msg_trap,-31,7)
kernel_trap(mach_msg_overwrite_trap,-32,9)
kernel_trap(semaphore_signal_trap, -33, 1)
kernel_trap(semaphore_signal_all_trap, -34, 1)
kernel_trap(semaphore_signal_thread_trap, -35, 2)
kernel_trap(semaphore_wait_trap,-36,1)
kernel_trap(semaphore_wait_signal_trap,-37,2)
kernel_trap(semaphore_timedwait_trap,-38,3)
kernel_trap(semaphore_timedwait_signal_trap,-39,4)

kernel_trap(_kernelrpc_mach_port_get_attributes_trap,-40,5)
kernel_trap(_kernelrpc_mach_port_guard_trap,-41,5)
kernel_trap(_kernelrpc_mach_port_unguard_trap,-42,4)
kernel_trap(mach_generate_activity_id, -43, 3)

kernel_trap(task_name_for_pid,-44,3)
kernel_trap(task_for_pid,-45,3)
kernel_trap(pid_for_task,-46,2)

#if defined(__LP64__)
kernel_trap(macx_swapon,-48, 4)
kernel_trap(macx_swapoff,-49, 2)
#else	              
kernel_trap(macx_swapon,-48, 5)
kernel_trap(macx_swapoff,-49, 3)
#endif	              
kernel_trap(thread_get_special_reply_port,-50,0)
kernel_trap(macx_triggers,-51, 4)
kernel_trap(macx_backing_store_suspend,-52, 1)
kernel_trap(macx_backing_store_recovery,-53, 1)

                                                       
                                                            
kernel_trap(swtch_pri,-59,1)
kernel_trap(swtch,-60,0)

kernel_trap(syscall_thread_switch,-61,3)
kernel_trap(clock_sleep_trap,-62,5)

                   
kernel_trap(host_create_mach_voucher_trap,-70,4)
                                       
kernel_trap(mach_voucher_extract_attr_recipe_trap,-72,4)
                                           
                               
                             

                          
kernel_trap(_kernelrpc_mach_port_type_trap,-76,3)
kernel_trap(_kernelrpc_mach_port_request_notification_trap,-77,7)

kernel_trap(mach_timebase_info_trap,-89,1)

#if		defined(__LP64__)
                                                       
kernel_trap(mach_wait_until,-90,1)
#else	              
kernel_trap(mach_wait_until,-90,2)
#endif	              

kernel_trap(mk_timer_create,-91,0)
kernel_trap(mk_timer_destroy,-92,1)

#if		defined(__LP64__)
                                                       
kernel_trap(mk_timer_arm,-93,2)
#else	              
kernel_trap(mk_timer_arm,-93,3)
#endif	              

kernel_trap(mk_timer_cancel,-94,2)
#if		defined(__LP64__)
kernel_trap(mk_timer_arm_leeway,-95,4)
#else
kernel_trap(mk_timer_arm_leeway,-95,7)
#endif
kernel_trap(debug_control_port_for_pid,-96,3)

  
                                                               
                           
   
#endif	                         

#endif	             
