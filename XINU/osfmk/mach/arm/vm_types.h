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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
  
   

  
                   
                               
             
  
                                               
   

#ifndef _MACH_ARM_VM_TYPES_H_
#define _MACH_ARM_VM_TYPES_H_

#ifndef ASSEMBLER

#include <arm/_types.h>
#include <stdint.h>
#include <Availability.h>

  
                                                               
                                                                  
                                                                 
                            
  
                                                                  
                                                                 
                                                                
                                                                   
                                                                       
                                                                
                                                                 
                                              
  
                                         
   
typedef __darwin_natural_t      natural_t;
typedef int                     integer_t;

  
                                           
                                              
   
#ifdef __LP64__
typedef uintptr_t               vm_offset_t;
typedef uintptr_t               vm_size_t;

typedef uint64_t                mach_vm_address_t;
typedef uint64_t                mach_vm_offset_t;
typedef uint64_t                mach_vm_size_t;

typedef uint64_t                vm_map_offset_t;
typedef uint64_t                vm_map_address_t;
typedef uint64_t                vm_map_size_t;
#else
typedef natural_t               vm_offset_t;
  
                                          
                                        
                        
   
typedef natural_t               vm_size_t;

  
                                                        
                                                         
                                                      
                                                       
                               
   
#if defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && (__IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_5_0)
typedef uint32_t                mach_vm_address_t;
typedef uint32_t                mach_vm_offset_t;
typedef uint32_t                mach_vm_size_t;
#else
typedef uint64_t                mach_vm_address_t;
typedef uint64_t                mach_vm_offset_t;
typedef uint64_t                mach_vm_size_t;
#endif

typedef uint32_t                vm_map_offset_t;
typedef uint32_t                vm_map_address_t;
typedef uint32_t                vm_map_size_t;
#endif               


typedef uint32_t                vm32_offset_t;
typedef uint32_t                vm32_address_t;
typedef uint32_t                vm32_size_t;

typedef vm_offset_t             mach_port_context_t;

#ifdef MACH_KERNEL_PRIVATE
typedef vm32_offset_t           mach_port_context32_t;
typedef mach_vm_offset_t        mach_port_context64_t;
#endif

#endif                 

  
                                                
   
#define MACH_MSG_TYPE_INTEGER_T MACH_MSG_TYPE_INTEGER_32

#endif                             
