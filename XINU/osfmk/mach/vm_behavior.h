/*
 * Copyright (c) 2000-2002 Apple Computer, Inc. All rights reserved.
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
  
                           
  
                                           
  
   

#ifndef _MACH_VM_BEHAVIOR_H_
#define _MACH_VM_BEHAVIOR_H_

  
                 
  
                                
   

typedef int             vm_behavior_t;

  
                                                 
                                                      
                                                           
                            
   


  
                                                                     
                                                     
   
#define VM_BEHAVIOR_DEFAULT     ((vm_behavior_t) 0)                  
#define VM_BEHAVIOR_RANDOM      ((vm_behavior_t) 1)                 
#define VM_BEHAVIOR_SEQUENTIAL  ((vm_behavior_t) 2)                             
#define VM_BEHAVIOR_RSEQNTL     ((vm_behavior_t) 3)                             

  
                                                                             
                                               
   
#define VM_BEHAVIOR_WILLNEED    ((vm_behavior_t) 4)                                   
#define VM_BEHAVIOR_DONTNEED    ((vm_behavior_t) 5)                                   
#define VM_BEHAVIOR_FREE        ((vm_behavior_t) 6)                                         
#define VM_BEHAVIOR_ZERO_WIRED_PAGES    ((vm_behavior_t) 7)                                                                                                  
#define VM_BEHAVIOR_REUSABLE    ((vm_behavior_t) 8)
#define VM_BEHAVIOR_REUSE       ((vm_behavior_t) 9)
#define VM_BEHAVIOR_CAN_REUSE   ((vm_behavior_t) 10)
#define VM_BEHAVIOR_PAGEOUT     ((vm_behavior_t) 11)

#endif                          
