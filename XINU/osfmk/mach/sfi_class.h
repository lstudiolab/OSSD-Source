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

#ifndef _MACH_SFI_CLASS_H_
#define _MACH_SFI_CLASS_H_

#include <stdint.h>
#include <mach/mach_types.h>

#define SFI_CLASS_DEFINITIONS 20140526

  
                                                       
                              
  
                                                     
                                                         
                                                       
                                                         
           
   
typedef uint32_t sfi_class_id_t;

#ifdef XNU_KERNEL_PRIVATE

  
                                                                     
                                                
   
#define MAX_SFI_CLASS_ID        0x00000011

  
                                              
   
#define SFI_CLASS_UNSPECIFIED   0x00000000

#endif                         

  
                                                                     
                                                                     
                                                        
   
#define SFI_CLASS_DARWIN_BG             0x00000001

  
                                                                 
                       
   
#define SFI_CLASS_APP_NAP               0x00000002

  
                                                               
                                                                  
                                                           
                                                                
        
   
#define SFI_CLASS_MANAGED_FOCAL                 0x00000003

#define SFI_CLASS_MANAGED_NONFOCAL              0x00000004

#define SFI_CLASS_MANAGED               SFI_CLASS_MANAGED_FOCAL

  
                                                            
                                                          
                                                                 
                    
   
#define SFI_CLASS_DEFAULT_FOCAL                 0x00000005

#define SFI_CLASS_DEFAULT_NONFOCAL              0x00000006

#define SFI_CLASS_DEFAULT               SFI_CLASS_DEFAULT_FOCAL

  
                                                                 
                                                               
                                          
   
#define SFI_CLASS_KERNEL                0x00000007

  
                                                               
                                                      
                                                                
                                              
   
#define SFI_CLASS_OPTED_OUT             0x00000008

  
                                         
   
#define SFI_CLASS_UTILITY                   0x00000009
#define SFI_CLASS_LEGACY_FOCAL              0x0000000A
#define SFI_CLASS_LEGACY_NONFOCAL           0x0000000B
#define SFI_CLASS_USER_INITIATED_FOCAL      0x0000000C
#define SFI_CLASS_USER_INITIATED_NONFOCAL   0x0000000D
#define SFI_CLASS_USER_INTERACTIVE_FOCAL    0x0000000E
#define SFI_CLASS_USER_INTERACTIVE_NONFOCAL 0x0000000F
#define SFI_CLASS_MAINTENANCE               0x00000010

  
                                                              
                                    
   
#define MIN_SFI_WINDOW_USEC             500

#endif                          
