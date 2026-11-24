/*
 * Copyright (c) 2006-2020 Apple Computer, Inc. All rights reserved.
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

  
                                 
  
                                                                         
                                                                          
                                           
   

#ifndef _SYS_SPAWN_H_
#define _SYS_SPAWN_H_

  
                                                                             
                                                                               
                                                      
   
#define POSIX_SPAWN_RESETIDS            0x0001                                 
#define POSIX_SPAWN_SETPGROUP           0x0002                                 
#define POSIX_SPAWN_SETSIGDEF           0x0004                                  
#define POSIX_SPAWN_SETSIGMASK          0x0008                             

#if 0                                                        
#define POSIX_SPAWN_SETSCHEDPARAM       0x0010
#define POSIX_SPAWN_SETSCHEDULER        0x0020
#endif         

#if     !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                        
   
#define POSIX_SPAWN_SETEXEC             0x0040
#define POSIX_SPAWN_START_SUSPENDED     0x0080
#ifdef  PRIVATE
#define _POSIX_SPAWN_DISABLE_ASLR       0x0100
#define _POSIX_SPAWN_NANO_ALLOCATOR     0x0200
#endif               
#define POSIX_SPAWN_SETSID              0x0400
#ifdef  PRIVATE
                                                 
#if (DEBUG || DEVELOPMENT)
#define _POSIX_SPAWN_FORCE_4K_PAGES     0x1000
#endif                             
#define _POSIX_SPAWN_ALLOW_DATA_EXEC    0x2000
#endif               
#define POSIX_SPAWN_CLOEXEC_DEFAULT     0x4000
#ifdef PRIVATE
#define _POSIX_SPAWN_HIGH_BITS_ASLR     0x8000
#endif              

#define _POSIX_SPAWN_RESLIDE            0x0800

  
                                                                                    
                                                                                             
                                                                                            
                                                                                           
   
#define POSIX_SPAWN_PCONTROL_NONE       0x0000
#define POSIX_SPAWN_PCONTROL_THROTTLE   0x0001
#define POSIX_SPAWN_PCONTROL_SUSPEND    0x0002
#define POSIX_SPAWN_PCONTROL_KILL       0x0003

#endif                                             

#endif                     
