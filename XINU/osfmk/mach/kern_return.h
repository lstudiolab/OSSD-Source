/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
  
   
  
                        
                               
             
  
                       
  
   

#ifndef _MACH_KERN_RETURN_H_
#define _MACH_KERN_RETURN_H_

#include <mach/machine/kern_return.h>

#define KERN_SUCCESS                    0

#define KERN_INVALID_ADDRESS            1
                                            
   

#define KERN_PROTECTION_FAILURE         2
                                                     
                            
   

#define KERN_NO_SPACE                   3
                                                    
                                                  
         
   

#define KERN_INVALID_ARGUMENT           4
                                                    
                                              
   

#define KERN_FAILURE                    5
                                                     
   

#define KERN_RESOURCE_SHORTAGE          6
                                                      
                                                    
   

#define KERN_NOT_RECEIVER               7
                                                    
                         
   

#define KERN_NO_ACCESS                  8
                            
   

#define KERN_MEMORY_FAILURE             9
                                                      
                                               
                        
   

#define KERN_MEMORY_ERROR               10
                                                   
                                                     
                                                   
                                                  
          
   

#define KERN_ALREADY_IN_SET             11
                                                        
   

#define KERN_NOT_IN_SET                 12
                                                   
   

#define KERN_NAME_EXISTS                13
                                                
   

#define KERN_ABORTED                    14
                                            
                                                
   

#define KERN_INVALID_NAME               15
                                               
   

#define KERN_INVALID_TASK               16
                                    
   

#define KERN_INVALID_RIGHT              17
                                                          
   

#define KERN_INVALID_VALUE              18
                         
   

#define KERN_UREFS_OVERFLOW             19
                                                     
   

#define KERN_INVALID_CAPABILITY         20
                                              
   

#define KERN_RIGHT_EXISTS               21
                                              
                                   
   

#define KERN_INVALID_HOST               22
                                     
   

#define KERN_MEMORY_PRESENT             23
                                                
                                          
                 
   

#define KERN_MEMORY_DATA_MOVED          24
                                               
                                                 
                                               
                                                
                                             
                                              
                                                
                                                
                                               
                                            
                                              
                                               
                 
   

#define KERN_MEMORY_RESTART_COPY        25
                                              
                                             
                                         
                                          
                              
   

#define KERN_INVALID_PROCESSOR_SET      26
                                                        
                                        
   

#define KERN_POLICY_LIMIT               27
                                                          
          
   

#define KERN_INVALID_POLICY             28
                                                   
                                 
   

#define KERN_INVALID_OBJECT             29
                                                       
                 
   

#define KERN_ALREADY_WAITING            30
                                                        
                                     
   

#define KERN_DEFAULT_SET                31
                                                       
       
   

#define KERN_EXCEPTION_PROTECTED        32
                                                         
                                                     
                       
   

#define KERN_INVALID_LEDGER             33
                                          
   

#define KERN_INVALID_MEMORY_CONTROL     34
                                                
   

#define KERN_INVALID_SECURITY           35
                                                    
                                
   

#define KERN_NOT_DEPRESSED              36
                                                    
                               
   

#define KERN_TERMINATED                 37
                                                        
   

#define KERN_LOCK_SET_DESTROYED         38
                                                          
   

#define KERN_LOCK_UNSTABLE              39
                                                          
           
   

#define KERN_LOCK_OWNED                 40
                                              
   

#define KERN_LOCK_OWNED_SELF            41
                                                  
   

#define KERN_SEMAPHORE_DESTROYED        42
                                                           
   

#define KERN_RPC_SERVER_TERMINATED      43
                                                   
                                            
   

#define KERN_RPC_TERMINATE_ORPHAN       44
                                    
   

#define KERN_RPC_CONTINUE_ORPHAN        45
                                                      
   

#define KERN_NOT_SUPPORTED              46
                                                   
   

#define KERN_NODE_DOWN                  47
                                    
   

#define KERN_NOT_WAITING                48
                                                  

#define KERN_OPERATION_TIMED_OUT        49
                                                            
   

#define KERN_CODESIGN_ERROR             50
                                                            
                                    
   

#define KERN_POLICY_STATIC              51
                                                         
   

#define KERN_INSUFFICIENT_BUFFER_SIZE   52
                                                                      
   

#define KERN_DENIED                     53
                            
   

#define KERN_MISSING_KC                 54
                                                       
   

#define KERN_INVALID_KC                 55
                                                       
   

#define KERN_RETURN_MAX                 0x100
                                 
   

#endif                            
