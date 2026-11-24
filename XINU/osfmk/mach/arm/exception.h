/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
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

#ifndef _MACH_ARM_EXCEPTION_H_
#define _MACH_ARM_EXCEPTION_H_

#define EXC_TYPES_COUNT         14                                     

#define EXC_MASK_MACHINE         0

#define EXCEPTION_CODE_MAX       2                             

#if XNU_KERNEL_PRIVATE
#if __has_feature(ptrauth_calls)
#define EXC_PTRAUTH_BIT         0x200                                                                      
#endif                                   
#endif                         

  
                                                             
   

  
                           
   

#define EXC_ARM_UNDEFINED       1                      

  
                      
   

#define EXC_ARM_FP_UNDEFINED    0                                               
#define EXC_ARM_FP_IO           1                                             
#define EXC_ARM_FP_DZ           2                                          
#define EXC_ARM_FP_OF           3                                    
#define EXC_ARM_FP_UF           4                                     
#define EXC_ARM_FP_IX           5                                          
#define EXC_ARM_FP_ID           6                                          

  
                      
                                                                            
   

#define EXC_ARM_DA_ALIGN        0x101                        
#define EXC_ARM_DA_DEBUG        0x102                                  
#define EXC_ARM_SP_ALIGN        0x103                           
#define EXC_ARM_SWP             0x104                        
#define EXC_ARM_PAC_FAIL        0x105                                   

  
                 
   

#define EXC_ARM_BREAKPOINT      1                            


#endif                              
