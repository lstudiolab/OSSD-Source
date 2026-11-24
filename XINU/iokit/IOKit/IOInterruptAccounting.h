/*
 * Copyright (c) 2014 Apple Computer, Inc. All rights reserved.
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

#ifndef __IOKIT_IOINTERRUPTACCOUNTING_H
#define __IOKIT_IOINTERRUPTACCOUNTING_H

#include <IOKit/IOReportTypes.h>

  
                                                                                                        
              
   

#define IA_INDEX_MAX (255)
#define IA_INDEX_MASK (0xFFULL)
#define IA_STATISTIC_INDEX_SHIFT (0ULL)
#define IA_INTERRUPT_INDEX_SHIFT (16ULL)

  
                                                                                                       
                                                                                                         
                                                                              
   
#define IA_NUM_INTERRUPT_ACCOUNTING_STATISTICS (10)

  
                                                                                                      
                                                                                                       
                                                                                                   
                                                                                                     
                                                                                                    
                                                                                                 
                                                                                                   
                                                                                                        
                                          
   

  
                                                                   
   
#define IA_BASE_CHANNEL_ID IOREPORT_MAKEID('I', 'n', 't', 'r', 0, 0, 0, 0)                             
#define IA_MAX_CHANNEL_ID IOREPORT_MAKEID('I', 'n', 't', 'r', 0xFF, 0xFF, 0xFF, 0xFF)                            

  
                                                                                                    
                                                                             
   
#define IA_GET_CHANNEL_ID(interruptIndex, statisticIndex) \
    ((IA_BASE_CHANNEL_ID) + \
    ((interruptIndex % IA_INDEX_MASK) << IA_INTERRUPT_INDEX_SHIFT) + \
    ((statisticIndex % IA_INDEX_MASK) << IA_STATISTIC_INDEX_SHIFT))

  
                                                    
   
#define IA_GET_INTERRUPT_INDEX(channelID) \
    (((channelID - IA_BASE_CHANNEL_ID) >> IA_INTERRUPT_INDEX_SHIFT) % IA_INDEX_MASK)

  
                                                    
   
#define IA_GET_STATISTIC_INDEX(channelID) \
    (((channelID - IA_BASE_CHANNEL_ID) >> IA_STATISTIC_INDEX_SHIFT) % IA_INDEX_MASK)

  
                                                                                                   
                                                                                                   
                                                                   
  
                                                                                             
                                                                                                      
  
                                                                                                   
                                                                                                       
  
                                                                                                   
                                                                                                    
                                                                                                     
                                                                                              
  
                                                                                                    
                                                                                                    
                                                                                                 
                                                                                                  
                                                                                                  
                                                  
  
                                                                                                  
                                                                                                    
                                                                                                
                                                    
  
                                                                                                
                                                                    
  
                                                                                                
                                                                                                 
                                           
  
                                                                                                  
                
  
                                                                                                   
                 
  
                                                                                                 
                                                                                                   
           
   
enum {
	kInterruptAccountingFirstLevelCountIndex = 0,                                                       
	kInterruptAccountingSecondLevelCountIndex,                                                     
	kInterruptAccountingFirstLevelTimeIndex,                                                                
	kInterruptAccountingSecondLevelCPUTimeIndex,                                            
	kInterruptAccountingSecondLevelSystemTimeIndex,                                               
	kInterruptAccountingNoThreadWakeupsIndex,                                                                               
	kInterruptAccountingTotalThreadWakeupsIndex,                                                               
	kInterruptAccountingPackageWakeupsIndex,                                                         
	kInterruptAccountingCPUWakeupsIndex,                                                   
	kInterruptAccountingIdleExitsIndex,                                                                       
	kInterruptAccountingInvalidStatisticIndex                                                          
};

  
                                                                                        
                                                                                        
                                
   
#define kInterruptAccountingGroupName "Interrupt Statistics (by index)"

#endif                                              
