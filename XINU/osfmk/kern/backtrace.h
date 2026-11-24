/*
 * Copyright (c) 2016-2019 Apple Inc. All rights reserved.
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

#ifndef KERN_BACKTRACE_H
#define KERN_BACKTRACE_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

   
                      
  
                                                        
  
                                                                           
                                                                           
                                                                                
                                                                               
                                                                             
                   
  
                                                                       
             
  
                                                                              
                    
  
                                                                               
                                                                           
                
  
                                                                         
                                   
   
unsigned int backtrace(uintptr_t *bt, unsigned int btlen, bool *was_truncated)
__attribute__((noinline));

   
                           
  
                                                                             
  
                                                                              
                                                                              
                                                                            
                                                                              
                                                                             
         
  
                                                                              
                                                                
  
                     
   
unsigned int backtrace_frame(uintptr_t *bt, unsigned int btlen, void *startfp,
    bool *was_truncated)
__attribute__((noinline, not_tail_called));

   
                                  
  
                                              
  
                                                                               
                                                                                
           
  
                     
   
unsigned int backtrace_interrupted(uintptr_t *bt, unsigned int btlen,
    bool *was_truncated);

   
                           
  
                                                            
  
                                                                            
                                                                            
                                                                         
            
  
                                                                               
                  
  
                                                                             
                                              
  
                                                                                  
         
  
                                                      
  
                     
   
unsigned int backtrace_user(uintptr_t *bt, unsigned int btlen, int *error,
    bool *user64, bool *was_truncated);

  
                                  
  
                                                        
  
                                                                                
                                                                              
                                                                           
  
                                                          
  
                      
   
unsigned int backtrace_thread_user(void *thread, uintptr_t *bt,
    unsigned int btlen, int *error, bool *user64, bool *was_truncated,
    bool faults_permitted);

__END_DECLS

#endif                                 
