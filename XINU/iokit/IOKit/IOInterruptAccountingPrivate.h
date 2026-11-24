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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef __IOKIT_IOINTERRUPTACCOUNTING_PRIVATE_H
#define __IOKIT_IOINTERRUPTACCOUNTING_PRIVATE_H

  
                                                                                                   
                                                    
   

#include <stdint.h>
#include <IOKit/IOInterruptAccounting.h>
#include <kern/queue.h>

class OSObject;
class IOSimpleReporter;

  
                                                                                                   
                                                                                                     
                                                                                                      
  
                                                                                                   
                                                                                                  
                                                                                                     
                                                                                                    
                                                                              
  
                                                                                                 
                                                                                                
                                                                                               
                                                                                                  
                                      
  
                                                                                                 
                                                                                                 
                                                                                                  
                                                                                                 
                                                                                                  
                                                               
   

  
                                                                                                   
                                                                                                    
                                          
  
                                                                                                          
                                             
   
extern uint32_t gInterruptAccountingStatisticBitmask;

  
                                                                                                        
   
#define IA_GET_ENABLE_BIT(statisticIndex) \
    (((uint32_t) 1) << ((uint32_t) statisticIndex))

#define IA_GET_STATISTIC_ENABLED(statisticIndex) \
    (IA_GET_ENABLE_BIT(statisticIndex) & gInterruptAccountingStatisticBitmask)

  
                                             
   
#define IA_ANY_STATISTICS_ENABLED \
    ((IA_GET_ENABLE_BIT(kInterruptAccountingInvalidStatisticIndex) - 1) & gInterruptAccountingStatisticBitmask)

  
                                                    
   
#define kInterruptAccountingChannelNameFirstLevelCount       ("               First Level Interrupt Handler Count")
#define kInterruptAccountingChannelNameSecondLevelCount      ("              Second Level Interrupt Handler Count")
#define kInterruptAccountingChannelNameFirstLevelTime        ("        First Level Interrupt Handler Time (MATUs)")
#define kInterruptAccountingChannelNameSecondLevelCPUTime    ("   Second Level Interrupt Handler CPU Time (MATUs)")
#define kInterruptAccountingChannelNameSecondLevelSystemTime ("Second Level Interrupt Handler System Time (MATUs)")
#define kInterruptAccountingChannelNameNoThreadWakeups       ("      Interrupts that did not try to wake a thread")
#define kInterruptAccountingChannelNameTotalThreadWakeups    ("       Sleeping threads woken up by this interrupt")
#define kInterruptAccountingChannelNamePackageWakeups        ("          Package wakeups caused by this interrupt")
#define kInterruptAccountingChannelNameCPUWakeups            ("              CPU wakeups caused by this interrupt")
#define kInterruptAccountingChannelNameIdleExits             ("               Idle exits caused by this interrupt")

static const char * const kInterruptAccountingStatisticNameArray[IA_NUM_INTERRUPT_ACCOUNTING_STATISTICS] = {
	[kInterruptAccountingFirstLevelCountIndex] = kInterruptAccountingChannelNameFirstLevelCount,
	[kInterruptAccountingSecondLevelCountIndex] = kInterruptAccountingChannelNameSecondLevelCount,
	[kInterruptAccountingFirstLevelTimeIndex] = kInterruptAccountingChannelNameFirstLevelTime,
	[kInterruptAccountingSecondLevelCPUTimeIndex] = kInterruptAccountingChannelNameSecondLevelCPUTime,
	[kInterruptAccountingSecondLevelSystemTimeIndex] = kInterruptAccountingChannelNameSecondLevelSystemTime,
	[kInterruptAccountingNoThreadWakeupsIndex] = kInterruptAccountingChannelNameNoThreadWakeups,
	[kInterruptAccountingTotalThreadWakeupsIndex] = kInterruptAccountingChannelNameTotalThreadWakeups,
	[kInterruptAccountingPackageWakeupsIndex] = kInterruptAccountingChannelNamePackageWakeups,
	[kInterruptAccountingCPUWakeupsIndex] = kInterruptAccountingChannelNameCPUWakeups,
	[kInterruptAccountingIdleExitsIndex] = kInterruptAccountingChannelNameIdleExits,
};

  
                                                                                                       
                                                                                                      
                                                                                                  
                                                            
  
                                                                                                          
                                                                                                          
                                                                                                     
                                                                                                         
                 
  
                                                                                                        
                                                                                                    
                                                             
   
#if __x86_64__ || __arm64__
#define IA_ADD_VALUE(target, value) \
    (*(target) += (value))
#else                                 
#define IA_ADD_VALUE(target, value) \
    (OSAddAtomic64((value), (target)))
#endif                                 

  
                                                                                          
   
struct IOInterruptAccountingData {
	OSObject * owner;                                                                                 
	queue_chain_t chain;
	  
                                                                                                        
                                                                                                    
                                                                
   
                                                                                                       
                                                                                                      
                                                                                                      
                                        
    
	int interruptIndex;

	bool enablePrimaryTimestamp;
	volatile uint64_t primaryTimestamp __attribute__((aligned(8)));

	  
                                                                                                     
                                                                                                       
                            
    
	volatile uint64_t interruptStatistics[IA_NUM_INTERRUPT_ACCOUNTING_STATISTICS] __attribute__((aligned(8)));
};

  
                                                                        
   
void interruptAccountingInit(void);

  
                                                                                                       
                                                                                                   
                     
   
void interruptAccountingDataAddToList(IOInterruptAccountingData * data);
void interruptAccountingDataRemoveFromList(IOInterruptAccountingData * data);

  
                                                                                                       
                                                                                                      
                                                              
   
void interruptAccountingDataUpdateChannels(IOInterruptAccountingData * data, IOSimpleReporter * reporter);

  
                                                                                                         
                                                                                                     
                                                                                                 
                                                                                                          
                                                                      
   
void interruptAccountingDataInheritChannels(IOInterruptAccountingData * data, IOSimpleReporter * reporter);

#endif                                              
