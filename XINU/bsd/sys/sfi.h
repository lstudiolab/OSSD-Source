/*
 * Copyright (c) 2013 Apple Inc. All rights reserved.
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

#ifndef _SYS_SFI_H_
#define _SYS_SFI_H_

#include <stdint.h>
#include <sys/types.h>
#include <mach/sfi_class.h>
  
                                                       
                                             
   

                                                
#define SFI_PROCESS_SET_MANAGED         0x00000001
#define SFI_PROCESS_SET_UNMANAGED       0x00000002

#define SFI_PROCESS_SET_MANAGED_MASK    0x00000003

#ifndef KERNEL
  
                                                                 
                                                              
                                                                   
                                                                  
                                                                     
                                                         
                                              
  
                                                                     
  
                                                                     
                
   
int system_set_sfi_window(uint64_t sfi_window_usec);

int system_get_sfi_window(uint64_t *sfi_window_usec);

  
                                                                     
                                                                      
                                                                
                                                                
                                                               
                                                                   
                                              
                                                                      
         
  
                                                                  
                                                       
  
                                                                
                                                     
   
int sfi_set_class_offtime(sfi_class_id_t class_id, uint64_t offtime_usec);

int sfi_get_class_offtime(sfi_class_id_t class_id, uint64_t *offtime_usec);

  
                                                                      
                                                                          
                     
   

int sfi_process_set_flags(pid_t pid, uint32_t flags);

int sfi_process_get_flags(pid_t pid, uint32_t *flags);

#endif              

#if PRIVATE

                                                                          
#define SFI_CTL_OPERATION_SFI_SET_WINDOW        0x00000001
#define SFI_CTL_OPERATION_SFI_GET_WINDOW        0x00000002
#define SFI_CTL_OPERATION_SET_CLASS_OFFTIME     0x00000003
#define SFI_CTL_OPERATION_GET_CLASS_OFFTIME     0x00000004

#define SFI_PIDCTL_OPERATION_PID_SET_FLAGS      0x00000001
#define SFI_PIDCTL_OPERATION_PID_GET_FLAGS      0x00000002

int __sfi_ctl(uint32_t operation, uint32_t sfi_class, uint64_t time, uint64_t *out_time);
int __sfi_pidctl(uint32_t operation, pid_t pid, uint32_t sfi_flags, uint32_t *out_sfi_flags);

#endif              

#endif                   
