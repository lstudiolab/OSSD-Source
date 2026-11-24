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
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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
  
   
  
                       
                                                    
  
                                         
  
   

#ifndef _MACH_VM_PROT_H_
#define _MACH_VM_PROT_H_

  
                 
  
                                   
   

typedef int             vm_prot_t;

  
                                                               
   

#define VM_PROT_NONE    ((vm_prot_t) 0x00)

#define VM_PROT_READ    ((vm_prot_t) 0x01)                           
#define VM_PROT_WRITE   ((vm_prot_t) 0x02)                            
#define VM_PROT_EXECUTE ((vm_prot_t) 0x04)                              

  
                                                          
   

#define VM_PROT_DEFAULT (VM_PROT_READ|VM_PROT_WRITE)

  
                                                           
   

#define VM_PROT_ALL     (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE)

  
                               
                                                                
                                                         
                                        
   

#define VM_PROT_NO_CHANGE       ((vm_prot_t) 0x08)

  
                                                                       
                                                                     
                                                                        
                                                                     
                                 
   

#define VM_PROT_COPY            ((vm_prot_t) 0x10)


  
                                    
                                                         
                                                            
                                                          
                                                       
                                                               
                                                       
                                 
   

#define VM_PROT_WANTS_COPY      ((vm_prot_t) 0x10)

#ifdef PRIVATE
  
                                                                   
                  
   

#define VM_PROT_TRUSTED         ((vm_prot_t) 0x20)
#endif              

  
                                         
                                                                       
                                                       
   
#define VM_PROT_IS_MASK         ((vm_prot_t) 0x40)

  
                                                                       
                                                                    
                                                                    
                                                                    
                                                                    
                                                                       
                                                                       
   
#define VM_PROT_STRIP_READ              ((vm_prot_t) 0x80)
#define VM_PROT_EXECUTE_ONLY    (VM_PROT_EXECUTE|VM_PROT_STRIP_READ)

#ifdef PRIVATE
  
                                                                          
                                                  
   
#define VM_PROT_COPY_FAIL_IF_EXECUTABLE ((vm_prot_t)0x100)
#endif              

#endif                        
