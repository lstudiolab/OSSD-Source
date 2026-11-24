/*
 * Copyright (c) 1998-2000 Apple Computer, Inc. All rights reserved.
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

#ifndef __IOKIT_IOMESSAGE_H
#define __IOKIT_IOMESSAGE_H

#include <IOKit/IOReturn.h>
#include <IOKit/IOTypes.h>

   
                      
  
                                                                    
  
   

typedef UInt32 IOMessage;

#define iokit_common_msg(message)          (UInt32)(sys_iokit|sub_iokit_common|message)
#define iokit_family_msg(sub, message)      (UInt32)(sys_iokit|sub|message)

   
                                             
                                                                                              
                                                                                            
                                                                                                       
   
#define iokit_vendor_specific_msg(message) (UInt32)(sys_iokit|sub_iokit_vendor_specific|message)

#define kIOMessageServiceIsTerminated      iokit_common_msg(0x010)
#define kIOMessageServiceIsSuspended       iokit_common_msg(0x020)
#define kIOMessageServiceIsResumed         iokit_common_msg(0x030)

#define kIOMessageServiceIsRequestingClose iokit_common_msg(0x100)
#define kIOMessageServiceIsAttemptingOpen  iokit_common_msg(0x101)
#define kIOMessageServiceWasClosed         iokit_common_msg(0x110)

#define kIOMessageServiceBusyStateChange   iokit_common_msg(0x120)

#define kIOMessageConsoleSecurityChange    iokit_common_msg(0x128)

#define kIOMessageServicePropertyChange    iokit_common_msg(0x130)

#define kIOMessageCopyClientID             iokit_common_msg(0x330)

#define kIOMessageSystemCapabilityChange   iokit_common_msg(0x340)
#define kIOMessageDeviceSignaledWakeup     iokit_common_msg(0x350)

#ifdef KERNEL_PRIVATE
                                                                              
                                               
                                                                          
#define kIOMessageTaskAppSuspendedChange   iokit_common_msg(0x800)
#endif

   
                                                
                                                                                 
                                                                                                             
                                                        
   
#define kIOMessageDeviceWillPowerOff       iokit_common_msg(0x210)

   
                                                
                                                                                
                                                                                                             
                                                        
   
#define kIOMessageDeviceHasPoweredOn       iokit_common_msg(0x230)

                                                                       
   

   
                                                
                                                                                    
                                                                                    
                                                     
                                                                                                          
                                                                                       
                                                                       
   
#define kIOMessageSystemWillPowerOff       iokit_common_msg(0x250)

   
                                               
                                                                                   
                                                                                    
                                                    
                                                                                                          
                                                                                       
                                                                       
   
#define kIOMessageSystemWillRestart        iokit_common_msg(0x310)

   
                                             
                                                                                         
                                                                         
                                                                
                                                                                             
                                                                       
   
#define kIOMessageSystemPagingOff       iokit_common_msg(0x255)


                                                       
   

   
                                            
                                                                             
                                                                                            
                                                                   
                                                                                       
   
#define kIOMessageCanSystemSleep           iokit_common_msg(0x270)

   
                                                
                                                                                        
                                                                     
                                                                                            
                                                                   
                                                                                       
   
#define kIOMessageSystemWillNotSleep       iokit_common_msg(0x290)

   
                                             
                                                      
                                                                                            
                                                                   
                                                                                       
   
#define kIOMessageSystemWillSleep          iokit_common_msg(0x280)

   
                                               
                                                                                         
                                                           
                                                                                            
                                                                   
                                                                                       
   
#define kIOMessageSystemWillPowerOn        iokit_common_msg(0x320)

   
                                                
                                                                            
                                                                                            
                                                                   
                                                                                       
   
#define kIOMessageSystemHasPoweredOn       iokit_common_msg(0x300)

                                                       
   

   
                                               
                                                                                 
                                                                   
                                                           
   
#define kIOMessageCanDevicePowerOff        iokit_common_msg(0x200)

   
                                                   
                                                                       
                                              
   
#define kIOMessageDeviceWillNotPowerOff    iokit_common_msg(0x220)

   
                                                   
                                                 
   
#define kIOMessageSystemWillNotPowerOff    iokit_common_msg(0x260)

   
                                               
                                                 
   
#define kIOMessageCanSystemPowerOff        iokit_common_msg(0x240)

   
                                               
                                                                                  
   
#define kIOMessageDeviceWillPowerOn        iokit_common_msg(0x215)

   
                                                 
                                                                                    
   
#define kIOMessageDeviceHasPoweredOff      iokit_common_msg(0x225)


#endif                            
