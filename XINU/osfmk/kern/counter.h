/*
 * Copyright (c) 2020 Apple Inc. All rights reserved.
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
#ifdef XNU_KERNEL_PRIVATE

#ifndef _KERN_COUNTER_H
#define _KERN_COUNTER_H

   
                         
  
         
                                                         
  
              
                                                                     
                                     
  
                                                                 
  
                                                                                 
                                                                                       
  
                                                                                    
                                                                               
                                                      
                                                                   
  
                                                                          
                                                                       
  
                                                                           
                                                           
         
                      
      
                  
                        
                                               
      
                                           
                           
                       
          
  
                          
         
                                       
      
                            
                                          
          
  
                           
         
                                        
                                      
                        
  
                                                                                
                                    
                                      
                                    
                             
  
                                     
                                   
                            
  
                                                   
                                                 
                                          
          
   

#include <mach/mach_types.h>
#include <kern/macro_help.h>
#include <kern/startup.h>
#include <kern/zalloc.h>

typedef __zpercpu uint64_t *scalable_counter_t;
typedef uint64_t atomic_counter_t;
                                                                 
struct generic_counter_t;

   
                                  
  
            
                                                                
  
                                                
   
#define SCALABLE_COUNTER_DECLARE(name) \
	extern scalable_counter_t name;

   
                                 
  
            
                                    
                                                                    
  
                                                
   
#define SCALABLE_COUNTER_DEFINE(name) \
	__startup_data uint64_t __ ##name##_early_storage = 0;                                   \
	scalable_counter_t name = {&__##name##_early_storage};                                   \
	STARTUP_ARG(TUNABLES, STARTUP_RANK_MIDDLE, scalable_counter_static_boot_mangle, &name);  \
	STARTUP_ARG(PERCPU, STARTUP_RANK_SECOND, scalable_counter_static_init, &name);

  
                                
                                 
                                                
   
OS_OVERLOADABLE
extern void counter_alloc(struct generic_counter_t *);

OS_OVERLOADABLE
extern void counter_free(struct generic_counter_t *);
  
                         
                                    
   
OS_OVERLOADABLE
extern void counter_add(struct generic_counter_t *, uint64_t amount);

  
                         
   
OS_OVERLOADABLE
extern void counter_inc(struct generic_counter_t *);

  
                                
   
OS_OVERLOADABLE
extern void counter_dec(struct generic_counter_t *);

                                                                                                      
OS_OVERLOADABLE
extern void counter_add_preemption_disabled(struct generic_counter_t *, uint64_t amount);
OS_OVERLOADABLE
extern void counter_inc_preemption_disabled(struct generic_counter_t *);
OS_OVERLOADABLE
extern void counter_dec_preemption_disabled(struct generic_counter_t *);

  
                                        
                                                                       
   
OS_OVERLOADABLE
extern uint64_t counter_load(struct generic_counter_t *);

#pragma mark implementation details
                                                     

__startup_func void scalable_counter_static_boot_mangle(scalable_counter_t *counter);
__startup_func void scalable_counter_static_init(scalable_counter_t *counter);

#if XNU_TARGET_OS_WATCH || XNU_TARGET_OS_TV
#define ATOMIC_COUNTER_USE_PERCPU 0
#else
#define ATOMIC_COUNTER_USE_PERCPU 1
#endif                        

#if ATOMIC_COUNTER_USE_PERCPU
typedef scalable_counter_t counter_t;
#else
typedef atomic_counter_t counter_t;
#endif                                

#define COUNTER_MAKE_PROTOTYPES(counter_t)                                 \
OS_OVERLOADABLE                                                            \
extern void counter_alloc(counter_t *);                                    \
                                                                           \
OS_OVERLOADABLE                                                            \
extern void counter_free(counter_t *);                                     \
                                                                           \
OS_OVERLOADABLE                                                            \
extern void counter_add(counter_t *, uint64_t amount);                     \
                                                                           \
OS_OVERLOADABLE                                                            \
extern void counter_inc(counter_t *);                                      \
                                                                           \
OS_OVERLOADABLE                                                            \
extern void counter_dec(counter_t *);                                      \
                                                                           \
OS_OVERLOADABLE                                                            \
extern void counter_add_preemption_disabled(counter_t *, uint64_t amount); \
                                                                           \
OS_OVERLOADABLE                                                            \
extern void counter_inc_preemption_disabled(counter_t *);                  \
                                                                           \
OS_OVERLOADABLE                                                            \
extern void counter_dec_preemption_disabled(counter_t *);                  \
                                                                           \
OS_OVERLOADABLE                                                            \
extern uint64_t counter_load(counter_t *);

COUNTER_MAKE_PROTOTYPES(scalable_counter_t);
COUNTER_MAKE_PROTOTYPES(atomic_counter_t);

#endif                      

#endif                         
