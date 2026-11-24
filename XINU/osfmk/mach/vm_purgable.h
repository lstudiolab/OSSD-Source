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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

  
                                                   
                                                                                            
                                                                                                
                                                                                      
                                                                                                  
                                                                                                 
                                                  
                                                                                               
                                                                                               
                           
                                                                                         
                                       
   

#ifndef _MACH_VM_PURGABLE_H_
#define _MACH_VM_PURGABLE_H_

  
                 
  
                                                
   

typedef int     vm_purgable_t;

  
                                                 
   
#define VM_PURGABLE_SET_STATE   ((vm_purgable_t) 0)                                        
#define VM_PURGABLE_GET_STATE   ((vm_purgable_t) 1)                                        
#define VM_PURGABLE_PURGE_ALL   ((vm_purgable_t) 2)                                         
#define VM_PURGABLE_SET_STATE_FROM_KERNEL ((vm_purgable_t) 3)                            

  
                   
  
                                          
                                           
                                           
                                           
                              
                       
                                                            
             
                               
              
                                   
             
                                
               
                                
               
                                  
   

#define VM_PURGABLE_NO_AGING_SHIFT      16
#define VM_PURGABLE_NO_AGING_MASK       (0x1 << VM_PURGABLE_NO_AGING_SHIFT)
#define VM_PURGABLE_NO_AGING            (0x1 << VM_PURGABLE_NO_AGING_SHIFT)

#define VM_PURGABLE_DEBUG_SHIFT 12
#define VM_PURGABLE_DEBUG_MASK  (0x3 << VM_PURGABLE_DEBUG_SHIFT)
#define VM_PURGABLE_DEBUG_EMPTY (0x1 << VM_PURGABLE_DEBUG_SHIFT)
#define VM_PURGABLE_DEBUG_FAULT (0x2 << VM_PURGABLE_DEBUG_SHIFT)

  
                                                                                        
                                                                                             
                                     
   
#define VM_VOLATILE_GROUP_SHIFT         8
#define VM_VOLATILE_GROUP_MASK          (7 << VM_VOLATILE_GROUP_SHIFT)
#define VM_VOLATILE_GROUP_DEFAULT   VM_VOLATILE_GROUP_0

#define VM_VOLATILE_GROUP_0                     (0 << VM_VOLATILE_GROUP_SHIFT)
#define VM_VOLATILE_GROUP_1                     (1 << VM_VOLATILE_GROUP_SHIFT)
#define VM_VOLATILE_GROUP_2                     (2 << VM_VOLATILE_GROUP_SHIFT)
#define VM_VOLATILE_GROUP_3                     (3 << VM_VOLATILE_GROUP_SHIFT)
#define VM_VOLATILE_GROUP_4                     (4 << VM_VOLATILE_GROUP_SHIFT)
#define VM_VOLATILE_GROUP_5                     (5 << VM_VOLATILE_GROUP_SHIFT)
#define VM_VOLATILE_GROUP_6                     (6 << VM_VOLATILE_GROUP_SHIFT)
#define VM_VOLATILE_GROUP_7                     (7 << VM_VOLATILE_GROUP_SHIFT)

  
                     
                                                                                           
                                                                   
                                               
   
#define VM_PURGABLE_BEHAVIOR_SHIFT  6
#define VM_PURGABLE_BEHAVIOR_MASK   (1 << VM_PURGABLE_BEHAVIOR_SHIFT)
#define VM_PURGABLE_BEHAVIOR_FIFO   (0 << VM_PURGABLE_BEHAVIOR_SHIFT)
#define VM_PURGABLE_BEHAVIOR_LIFO   (1 << VM_PURGABLE_BEHAVIOR_SHIFT)

  
                   
                                                                                                 
                
                                               
   
#define VM_PURGABLE_ORDERING_SHIFT              5
#define VM_PURGABLE_ORDERING_MASK               (1 << VM_PURGABLE_ORDERING_SHIFT)
#define VM_PURGABLE_ORDERING_OBSOLETE   (1 << VM_PURGABLE_ORDERING_SHIFT)
#define VM_PURGABLE_ORDERING_NORMAL             (0 << VM_PURGABLE_ORDERING_SHIFT)


  
                                  
   
#define VM_VOLATILE_ORDER_SHIFT                 4
#define VM_VOLATILE_ORDER_MASK                  (1 << VM_VOLATILE_ORDER_SHIFT)
#define VM_VOLATILE_MAKE_FIRST_IN_GROUP (1 << VM_VOLATILE_ORDER_SHIFT)
#define VM_VOLATILE_MAKE_LAST_IN_GROUP  (0 << VM_VOLATILE_ORDER_SHIFT)

  
                                      
   
#define VM_PURGABLE_STATE_MIN   0                                                         
#define VM_PURGABLE_STATE_MAX   3                                                         
#define VM_PURGABLE_STATE_MASK  3                                                      

#define VM_PURGABLE_NONVOLATILE 0                                                     
#define VM_PURGABLE_VOLATILE    1                                                 
#define VM_PURGABLE_EMPTY       2                                                           
#define VM_PURGABLE_DENY        3                                                

#define VM_PURGABLE_ALL_MASKS   (VM_PURGABLE_STATE_MASK | \
	                         VM_VOLATILE_ORDER_MASK | \
	                         VM_PURGABLE_ORDERING_MASK | \
	                         VM_PURGABLE_BEHAVIOR_MASK | \
	                         VM_VOLATILE_GROUP_MASK | \
	                         VM_PURGABLE_DEBUG_MASK | \
	                         VM_PURGABLE_NO_AGING_MASK)
#endif                            
