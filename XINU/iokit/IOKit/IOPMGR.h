/*
 * Copyright (c) 2019 Apple Computer, Inc. All rights reserved.
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

#pragma once

extern "C" {
#include <machine/machine_routines.h>
};

#include <stdint.h>
#include <IOKit/IOService.h>

   
                     
                                                                    
   
class IOPMGR : public IOService
{
	OSDeclareAbstractStructors(IOPMGR);

public:
	   
                                  
                                              
                                                                        
                                                                     
                                                                 
                                                
                                                                       
                                                                      
                                                                        
                              
    
	virtual void enableCPUCore(unsigned int cpu_id, uint64_t entry_pa);

	   
                                
                                                         
    
	virtual void enableCPUCore(unsigned int cpu_id);

	   
                                 
                                             
                                                                   
                                                                  
                                                                        
                                                                      
                                                                    
                                     
                                              
    
	virtual void disableCPUCore(unsigned int cpu_id) = 0;

	   
                                       
                                                         
                                                                           
                                                                          
                                           
                                  
    
	virtual void enableCPUCluster(unsigned int cluster_id) = 0;

	   
                                        
                                                          
                                                                   
                                                                      
                                                                  
                                                                    
                                                                 
                                       
                                  
    
	virtual void disableCPUCluster(unsigned int cluster_id) = 0;

	   
                                           
                                                                   
                                                                                 
                                                                                   
    
	virtual void initCPUIdle(ml_processor_info_t *info) = 0;

	   
                                            
                                                                                          
                                                
                                                                                               
                                                                                              
    
	virtual void enterCPUIdle(UInt64 *newIdleTimeoutTicks) = 0;

	   
                                           
                                                                                           
                                                
                                                                                               
                                                                                              
    
	virtual void exitCPUIdle(UInt64 *newIdleTimeoutTicks) = 0;

	   
                                             
                                                                                              
                                                           
                                                                                               
                                                                                              
    
	virtual void updateCPUIdle(UInt64 *newIdleTimeoutTicks) = 0;
};
