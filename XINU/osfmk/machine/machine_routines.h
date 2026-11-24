/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
#ifndef _MACHINE_MACHINE_ROUTINES_H
#define _MACHINE_MACHINE_ROUTINES_H

#include <sys/cdefs.h>

#if defined (__i386__) || defined(__x86_64__)
#include "i386/machine_routines.h"
#elif defined (__arm__) || defined (__arm64__)
#include "arm/machine_routines.h"
#else
#error architecture not supported
#endif

__BEGIN_DECLS

#ifdef XNU_KERNEL_PRIVATE
#pragma GCC visibility push(hidden)

   
                                 
                                                                       
                                       
                                                  
   
bool ml_cpu_can_exit(int cpu_id);

   
                                               
                                                                  
                                                                        
                            
                                                            
   
void ml_cpu_begin_state_transition(int cpu_id);

   
                                             
                                                                  
                                                                  
                                                                        
                                                  
                                                            
   
void ml_cpu_end_state_transition(int cpu_id);

   
                                   
                                                                          
                                                                       
                                                      
   
void ml_cpu_begin_loop(void);

   
                                 
                                                                          
                                                                             
   
void ml_cpu_end_loop(void);

   
                                           
                                                                               
  
                                                                  
  
              
                                                                 
                                        
  
                                                               
                                                                      
                                  
   
int ml_early_cpu_max_number(void);

#pragma GCC visibility pop
#endif                                  

   
                      
                                                                             
                                                                              
                                                                                                      
                                                                           
                                                                              
                                                                             
                                                                                                  
                                                                              
                                                                            
   
enum cpu_event {
	CPU_BOOT_REQUESTED = 0,
	CPU_BOOTED,
	CPU_ACTIVE,
	CLUSTER_ACTIVE,
	CPU_EXIT_REQUESTED,
	CPU_DOWN,
	CLUSTER_EXIT_REQUESTED,
	CPU_EXITED,
};

typedef bool (*cpu_callback_t)(void *param, enum cpu_event event, unsigned int cpu_or_cluster);

   
                                                     
                                                                                
                                                                  
                                                                                               
   
void cpu_event_register_callback(cpu_callback_t fn, void *param);

   
                                                       
                                                                               
                                                                                              
   
void cpu_event_unregister_callback(cpu_callback_t fn);

#if XNU_KERNEL_PRIVATE
   
                                                
                                                                                               
                                                      
                                                                                        
   
void ml_broadcast_cpu_event(enum cpu_event event, unsigned int cpu_or_cluster);
#endif

__END_DECLS

#endif                                  
