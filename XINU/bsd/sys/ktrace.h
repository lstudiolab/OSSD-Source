/*
 * Copyright (c) 2015 Apple Inc. All rights reserved.
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

#ifndef SYS_KTRACE_H
#define SYS_KTRACE_H

#include <stdint.h>

#include <kern/locks.h>

                                       
enum ktrace_state {
	                                    
	KTRACE_STATE_OFF = 0,
	                                              
	KTRACE_STATE_FG,
	                                              
	KTRACE_STATE_BG
};

void ktrace_lock(void);
void ktrace_unlock(void);
void ktrace_assert_lock_held(void);
void ktrace_start_single_threaded(void);
void ktrace_end_single_threaded(void);

  
                                                                              
                                                                           
   
#define KTRACE_KDEBUG (1 << 0)
#define KTRACE_KPERF  (1 << 1)

  
                                                                         
                                                                    
                                                                  
  
                              
  
                                                                             
                                                   
   
int ktrace_configure(uint32_t config_mask);

  
                                                                            
                              
  
                              
   
void ktrace_reset(uint32_t reset_mask);

  
                                                                         
                                                                   
  
                              
  
                                         
   
int ktrace_read_check(void);

  
                                                                          
                                                                           
                                                                            
                                                
  
                                                                            
                                                                     
  
                              
   
void ktrace_kernel_configure(uint32_t config_mask);

  
                                                                         
                                                             
  
                                                                           
                                             
  
                              
   
void ktrace_disable(enum ktrace_state state_to_match);

  
                                                                          
             
  
                              
   
int ktrace_get_owning_pid(void);

  
                                                                 
  
                              
   
bool ktrace_background_active(void);

  
                                                                             
                                                        
   
extern bool ktrace_keep_ownership_on_reset;
extern int ktrace_root_set_owner_allowed;
int ktrace_set_owning_pid(int pid);

                                                                      
void ktrace_init(void);

#endif                   
