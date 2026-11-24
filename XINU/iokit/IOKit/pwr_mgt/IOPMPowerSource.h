/*
 * Copyright (c) 1998-2019 Apple Inc. All rights reserved.
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

#ifndef _IOPMPowerSource_h_
#define _IOPMPowerSource_h_

#include <libkern/c++/OSObject.h>
#include <IOKit/pwr_mgt/IOPM.h>
#include <IOKit/IOTypes.h>
#include <IOKit/IOReturn.h>
#include <IOKit/IOService.h>

enum {
	kSecondsPerHour = 3600,
	kTenMinutesInSeconds = 600
};


                          
  
                                                                               
                                                                                
                                                                             
                                
  
                                                                      
                                
                                                                                
                                                  
                                                                        
                                                         
                                                                               
  
                                                                           
                            
  
  
            
  
        
                    
             
                                              
                                             
  
                        
             
                                                  
                                                                 
  
                   
             
                                             
                                                 
  
             
             
                                       
                                                         
  
              
             
                                        
                                                        
  
                  
             
                                            
                                                            
  
                  
              
                     
                                                                   
                           
  
                
            
                                          
                                     
  
           
            
                                     
                            
  
          
                     
                                    
                         
  
             
                     
                                       
                                    
  
              
            
                                        
                            
  
           
            
                                     
                                                            
  
                 
                 
                                           
                                           
  
               
                 
                                         
                                         
  
                    
                                 
                                            
                                                                          
                    
                                           
                                             
                                                          
  
        
                 
                                  
                                 
  
         
                 
                                   
                                                 
                                                                               
                                                                            
                                               
  
                      
                     
                                              
                                              
         
   

class IOPMPowerSource : public IOService
{
	OSDeclareDefaultStructors(IOPMPowerSource);

	friend class IOPMPowerSourceList;

protected:

                                       
                                                                             
                                                                              
                   
   
	bool settingsChangedSinceUpdate;

                   
                            
   
	OSDictionary            *properties;

	const OSSymbol *externalConnectedKey;
	const OSSymbol *externalChargeCapableKey;
	const OSSymbol *batteryInstalledKey;
	const OSSymbol *chargingKey;
	const OSSymbol *warnLevelKey;
	const OSSymbol *criticalLevelKey;
	const OSSymbol *currentCapacityKey;
	const OSSymbol *maxCapacityKey;
	const OSSymbol *timeRemainingKey;
	const OSSymbol *amperageKey;
	const OSSymbol *voltageKey;
	const OSSymbol *cycleCountKey;
	const OSSymbol *adapterInfoKey;
	const OSSymbol *locationKey;
	const OSSymbol *errorConditionKey;
	const OSSymbol *manufacturerKey;
	const OSSymbol *modelKey;
	const OSSymbol *serialKey;
	const OSSymbol *batteryInfoKey;

                                   
	IOPMPowerSource         *nextInList;

public:

                         
                                                                                 
                                    
   
	static IOPMPowerSource *powerSource(void);

	virtual bool init(void) APPLE_KEXT_OVERRIDE;

	virtual void free(void) APPLE_KEXT_OVERRIDE;

                          
                                                                               
                                           
                                                                                   
                                                                                 
                                                                             
                                                                                  
                                    
   
	virtual void updateStatus(void);

                                     
   
	bool externalConnected(void);
	bool externalChargeCapable(void);
	bool batteryInstalled(void);
	bool isCharging(void);
	bool atWarnLevel(void);
	bool atCriticalLevel(void);

	unsigned int currentCapacity(void);
	unsigned int maxCapacity(void);
	unsigned int capacityPercentRemaining(void);
	int timeRemaining(void);
	int amperage(void);
	unsigned int voltage(void);
	unsigned int cycleCount(void);
	int adapterInfo(void);
	int location(void);

	OSSymbol *errorCondition(void);
	OSSymbol *manufacturer(void);
	OSSymbol *model(void);
	OSSymbol *serial(void);
	OSDictionary *legacyIOBatteryInfo(void);

	OSObject *getPSProperty(const OSSymbol *);

protected:

                                            
                                                                        
  
                                                                            
                                                                
  
   
	void setExternalConnected(bool);
	void setExternalChargeCapable(bool);
	void setBatteryInstalled(bool);
	void setIsCharging(bool);
	void setAtWarnLevel(bool);
	void setAtCriticalLevel(bool);

	void setCurrentCapacity(unsigned int);
	void setMaxCapacity(unsigned int);
	void setTimeRemaining(int);
	void setAmperage(int);
	void setVoltage(unsigned int);
	void setCycleCount(unsigned int);
	void setAdapterInfo(int);
	void setLocation(int);

	void setErrorCondition(OSSymbol *);
	void setManufacturer(OSSymbol *);
	void setModel(OSSymbol *);
	void setSerial(OSSymbol *);
	void setLegacyIOBatteryInfo(OSDictionary *);

                                                                   
                                                                     
                                                                        
                                       
   
	void setPSProperty(const OSSymbol *, OSObject *);
};

#endif
