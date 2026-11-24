/*
 * Copyright (c) 2003-2007 Apple Inc. All rights reserved.
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

#ifndef _SYS__TYPES_H_
#define _SYS__TYPES_H_

#include <sys/cdefs.h>
#include <machine/_types.h>

  
                                                                    
                                                                      
                                                    
   

#ifdef __cplusplus
#ifdef __GNUG__
#define __DARWIN_NULL __null
#else                 
#ifdef __LP64__
#define __DARWIN_NULL (0L)
#else                
#define __DARWIN_NULL 0
#endif               
#endif               
#else                    
#define __DARWIN_NULL ((void *)0)
#endif                  

#if !defined(DRIVERKIT)
typedef __int64_t       __darwin_blkcnt_t;                        
typedef __int32_t       __darwin_blksize_t;                               
typedef __int32_t       __darwin_dev_t;                    
typedef unsigned int    __darwin_fsblkcnt_t;                                      
typedef unsigned int    __darwin_fsfilcnt_t;                                      
typedef __uint32_t      __darwin_gid_t;                                          
typedef __uint32_t      __darwin_id_t;                                           
typedef __uint64_t      __darwin_ino64_t;                                         
#if __DARWIN_64_BIT_INO_T
typedef __darwin_ino64_t __darwin_ino_t;                                   
#else                             
typedef __uint32_t      __darwin_ino_t;                                    
#endif                            
typedef __darwin_natural_t __darwin_mach_port_name_t;                   
typedef __darwin_mach_port_name_t __darwin_mach_port_t;                   
typedef __uint16_t      __darwin_mode_t;                                        
typedef __int64_t       __darwin_off_t;                                        
typedef __int32_t       __darwin_pid_t;                                          
typedef __uint32_t      __darwin_sigset_t;                            
typedef __int32_t       __darwin_suseconds_t;                           
typedef __uint32_t      __darwin_uid_t;                             
typedef __uint32_t      __darwin_useconds_t;                            
#endif                          
typedef unsigned char   __darwin_uuid_t[16];
typedef char    __darwin_uuid_string_t[37];

#if !defined(KERNEL) && !defined(DRIVERKIT)
#include <sys/_pthread/_pthread_types.h>
#endif                                              

#if defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 5 || __GNUC__ > 3)
#define __offsetof(type, field) __builtin_offsetof(type, field)
#else                    
#define __offsetof(type, field) ((size_t)(&((type *)0)->field))
#endif                   

#ifdef KERNEL
#include <sys/_types/_offsetof.h>
#endif             

#endif                      
