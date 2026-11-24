/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
 */

#ifndef _BSD_ARM_VMPARAM_H_
#define _BSD_ARM_VMPARAM_H_ 1

#include <sys/resource.h>

#ifndef KERNEL
#include <TargetConditionals.h>
#endif

#define USRSTACK        (0x27E00000)                                             
#define USRSTACK64      (0x000000016FE00000ULL)

  
                                                 
   
#ifndef DFLDSIZ
#define DFLDSIZ         (RLIM_INFINITY)                                      
#endif
#ifndef MAXDSIZ
#define MAXDSIZ         (RLIM_INFINITY)                            
#endif
#ifndef DFLSSIZ
                                                                     
#if (defined(TARGET_OS_OSX) && (TARGET_OS_OSX != 0)) || \
        (defined(KERNEL) && XNU_TARGET_OS_OSX)
#define DFLSSIZ         (8*1024*1024 - 16*1024)
#else
#define DFLSSIZ         (1024*1024 - 16*1024)                                 
#endif                                                
#endif              
#ifndef MAXSSIZ
                                                                   
#if (defined(TARGET_OS_OSX) && (TARGET_OS_OSX != 0)) || \
        (defined(KERNEL) && XNU_TARGET_OS_OSX)
#define MAXSSIZ         (64*1024*1024)                              
#else
#define MAXSSIZ         (1024*1024)                                 
#endif                                                
#endif              
#ifndef DFLCSIZ
#define DFLCSIZ         (0)                                                  
#endif
#ifndef MAXCSIZ
#define MAXCSIZ         (RLIM_INFINITY)                            
#endif               

#endif                           
