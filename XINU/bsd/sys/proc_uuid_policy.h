/*
 * Copyright (c) 2013 Apple Inc. All rights reserved.
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

#ifndef _SYS_PROC_UUID_POLICY_H
#define _SYS_PROC_UUID_POLICY_H

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <stdint.h>
#include <uuid/uuid.h>

__BEGIN_DECLS

  
                                                               
                                                           
                                                              
                                                           
                                                           
                                                         
                                 
   

#define PROC_UUID_POLICY_OPERATION_CLEAR        0x00000000
#define PROC_UUID_POLICY_OPERATION_ADD          0x00000001
#define PROC_UUID_POLICY_OPERATION_REMOVE       0x00000002

                                                             
#define PROC_UUID_POLICY_FLAGS_NONE                     0x00000000
#define PROC_UUID_NO_CELLULAR                           0x00000001
#define PROC_UUID_NECP_APP_POLICY                       0x00000002
#define PROC_UUID_ALT_DYLD_POLICY                       0x00000004

                                                          
#define PROC_UUID_FLOW_DIVERT                           0x00000002

#ifdef BSD_KERNEL_PRIVATE
  
                                    
  
              
                                                               
                                                                                
                            
                                                                                      
  
                                                                                  
                                                                                   
                                                                               
  
          
                                                                        
                                                   
                              
  
   
extern int proc_uuid_policy_lookup(uuid_t uuid, uint32_t *flags, int32_t *gencount);

extern void proc_uuid_policy_init(void);

extern int proc_uuid_policy_kernel(uint32_t operation, uuid_t uuid, uint32_t flags);
#endif                         

#ifndef KERNEL
  
                                   
  
              
                                                                    
                                                                                  
                                                                                   
                                                                                 
                                                                                 
                                                                            
                                                          
                                   
                                                                                       
  
          
                                                           
                                           
                                                                              
                                                                   
                                     
                                   
                                          
   
extern int proc_uuid_policy(uint32_t operation, uuid_t uuid, size_t uuidlen, uint32_t flags);
#endif              

__END_DECLS

#endif                             
