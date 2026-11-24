/*
 * Copyright (c) 2011-2018 Apple Computer, Inc. All rights reserved.
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

#ifndef KPERF_KPTIMER_H
#define KPERF_KPTIMER_H

  
                                                                        
                                                                              
                                                                             
                                                                             
                                                                              
                                                     
  
                                                                                
                                                                                
                                                                            
                
  
                                                                                
                                                                              
                                                               
   

  
                                                                                 
                                          
   
enum kptimer_period_limit {
	KTPL_FG,
	KTPL_BG,
	KTPL_FG_PET,
	KTPL_BG_PET,
	KTPL_MAX,
};

  
                                                                          
                                              
   
extern const uint64_t kptimer_minperiods_ns[KTPL_MAX];

  
                                                           
   
void kptimer_init(void);

  
                                                      
   
uint64_t kptimer_min_period_abs(bool pet);

  
                                         
   
unsigned int kptimer_get_count(void);

  
                                                 
  
                                            
   
int kptimer_set_count(unsigned int count);

  
                                                                          
  
                                            
   
int kptimer_get_period(unsigned int timerid, uint64_t *period_out);

  
                                                                   
  
                                                 
   
int kptimer_set_period(unsigned int timerid, uint64_t period);

  
                                                            
                  
   
int kptimer_get_action(unsigned int timerid, uint32_t *actionid_out);

  
                                                                     
   
int kptimer_set_action(unsigned int timer, uint32_t actionid);

  
                                                          
   
int kptimer_set_pet_timerid(unsigned int timerid);

  
                                  
   
unsigned int kptimer_get_pet_timerid(void);

  
                                                                            
             
   
void kptimer_pet_enter(uint64_t sampledur_abs);

  
                                                          
   
void kptimer_start(void);

  
                                                                                   
   
void kptimer_stop(void);

  
                                          
   
void kptimer_expire(processor_t processor, int cpuid, uint64_t now);

  
                            
   
void kptimer_reset(void);

#endif                                
