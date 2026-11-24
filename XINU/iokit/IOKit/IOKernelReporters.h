/*
 * Copyright (c) 2012-2016 Apple Inc.  All Rights Reserved.
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
 * FILE: IOReporter.h
 * AUTH: Cyril & Soren (Core OS)
 * DATE: 2012-2013 (Copyright Apple Inc.)
 * DESC: IOReporting interfaces for I/O Kit drivers
 *
 */

#ifndef _IOKERNEL_REPORTERS_H_
#define _IOKERNEL_REPORTERS_H_

#include <machine/limits.h>

#include <libkern/c++/OSPtr.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/IOLocks.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#include <IOKit/IOReportTypes.h>
#include <IOKit/IOKernelReportStructs.h>

#include <libkern/c++/OSPtr.h>

typedef OSDictionary IOReportLegendEntry;

                                
                           
                    
                                                                   
                                                         
                                                                      
                                        
                                

   
                   
  
                                                                       
                                                                        
                                                                    
                                                                 
                                                                          
                                                                           
                                                                          
                                                                         
                                                        
  
                                                                    
                                                                         
                                                                     
                                                                
                                                                         
                                                                      
                                                                      
                                                      
  
                                                                       
                                                                          
                                                                      
                                                                          
                                                                      
  
                                                                          
                                                                      
                                                                   
                                                      
  
          
                                                                        
                                                                            
                                    
                                                                        
                                                                
                                                              
  
                                                                     
                                                                         
                                                               
  
                                                                         
                                                                           
                                                                      
                                                                        
                                                                          
                                                                  
                                                                    
                                                                           
  
                                                                           
   


                                   
                                   
                                   

class IOReporter : public OSObject
{
	OSDeclareDefaultStructors(IOReporter);

protected:
                                
                                                                          
  
                                                                      
                                                                       
                                                        
                                                 
  
                
                                                                            
                                                                           
                            
  
                                                                            
                                                                        
                                                                          
                                    
  
                                              
   
	virtual bool init(IOService *reportingService,
	    IOReportChannelType channelType,
	    IOReportUnit unit);

public:

                                      
                                                                   
  
                                                               
                                                                         
                                          
  
                
                                                                          
                                                                          
                                                                            
                                                                           
                                                                 
                                                                          
                                                                          
  
                                                       
   
	IOReturn addChannel(uint64_t channelID, const char *channelName = NULL);

                                        
                                                                            
                                                                  
                
                                                               
                                       
  
                                                                 
                                                                        
                                                                           
                                                                        
                                                                    
                                                                          
                                                                           
                                                              
  
                                                                         
                                                                        
                                                                      
                                                              
  
                                                            
  
                                                                         
                                                                        
                                                                         
                                                         
  
                                                       
   
	OSPtr<IOReportLegendEntry> createLegend(void);

                                           
                                                                        
  
                                                
                                                                    
                                                              
                                                              
                                          
  
                
                                                                           
                                                                       
                                                               
  
                                                                           
                                                                        
                                                                 
                                                                    
                                                                        
                                      
  
                                                                           
                                                                          
                                                               
                                                                       
                                                                       
                                                                      
                                               
  
                                                                           
                                                   
  
                                                       
   
	IOReturn configureReport(IOReportChannelList *channelList,
	    IOReportConfigureAction action,
	    void *result,
	    void *destination);

                                        
                                                                    
  
                                             
                                                           
                                                                      
                                                                        
                                          
  
                
                                                                      
                                                                        
                                                                        
                                                                        
                                                                   
  
                                                                        
                                  
  
                                                            
   
	IOReturn updateReport(IOReportChannelList *channelList,
	    IOReportConfigureAction action,
	    void *result,
	    void *destination);

                                
                                                           
  
                
                                                                          
                                                                
  
                                              
   
	virtual void free(void) APPLE_KEXT_OVERRIDE;

                              
                                      
  
                
                                                                     
                                
  
   
	static void initialize(void);


                                   
                                   
                                   

                                                                         
                                                                     
                         
   

                                               
                                                                      
  
                                                    
                                                             
                                              
                                                    
                                                              
                                                             
                                              
  
                
                                                                        
                                                                       
                                                                     
                                                     
  
                                                                  
                                                                      
                                        
   
	static IOReturn configureAllReports(OSSet *reporters,
	    IOReportChannelList *channelList,
	    IOReportConfigureAction action,
	    void *result,
	    void *destination);
                                                  

                                            
                                                                   
  
                                                    
                                                          
                                          
                                                             
                                                                        
                              
                
                                                                        
                                                                       
                                                                     
                                                     
  
                                                                  
                                                                      
                                        
   
	static IOReturn updateAllReports(OSSet *reporters,
	    IOReportChannelList *channelList,
	    IOReportConfigureAction action,
	    void *result,
	    void *destination);
                                                  


                                     
  
                                                                  
                                                                   
                                                                    
              
  
                                                                       
                                                                        
   
protected:

                                              
                                                 
                                                                 
  
                
                                                                          
                                                                      
                                                                        
                                                                          
                                                                    
  
                                                                       
                                                            
   
	void lockReporterConfig(void);
	void unlockReporterConfig(void);

                                        
                                           
                                                               
  
                
                                                                           
                                                                         
                                                                           
                                                             
                                                    
  
                                                                      
                                                               
                                                   
                                 
                                  
                                  
                                  
                        
                                  
          
                                                       
                           
                            
                             
          
  
                                                                          
                                                                       
                                                                         
  
                                                                         
                                                                        
                                                                         
                                                                        
                           
  
                                                                          
                                                                       
                                                                      
                                              
   
	void lockReporter(void);
	void unlockReporter(void);

   
                
                                                                            
                                                                      
                                            
                                                                            
                                                     
                                                
  
                                                                      
                                                               
                           
  
                                        
                                 
   
                                             
                                                                             
  
                                                    
                              
  
                
                                                                           
                                                                        
                                                                       
                                                                          
  
                                                                       
                                                          
   
	virtual IOReturn handleSwapPrepare(int newNChannels);

                                                
                                                                   
  
                                                  
                                                                          
                              
  
                
                                                                      
                                                                          
                                                                       
                                                                          
                                                                        
                       
  
                                                                     
                                                                         
                                                                       
                                                                        
                                      
  
                                                                      
                                                                           
                                                                       
                                       
  
                                                     
                                           
                                            
                                                                              
            
                             
                                   
            
                
                                              
  
                                                                    
                                             
   
	virtual IOReturn handleAddChannelSwap(uint64_t channel_id,
	    const OSSymbol *symChannelName);

                                             
                                                  
  
                                                                      
  
                
                                                                          
                                                                  
                                                                     
                                                                     
                                                      
  
                                                                       
                                                          
   
	virtual void handleSwapCleanup(int swapNChannels);

                                                 
                                                                  
                                                             
  
                
                                                                         
                                                                       
                                
   
	virtual IOReturn handleConfigureReport(IOReportChannelList *channelList,
	    IOReportConfigureAction action,
	    void *result,
	    void *destination);

                                              
                                                               
                                                             
  
                
                                                                         
                                                                       
                                
  
                                                                        
                                                               
                                           
   
	virtual IOReturn handleUpdateReport(IOReportChannelList *channelList,
	    IOReportConfigureAction action,
	    void *result,
	    void *destination);

                                              
                                                               
                                                             
  
                
                                                                         
                                                                       
                                
   
	virtual OSPtr<IOReportLegendEntry> handleCreateLegend(void);

                                               
                                                                     
  
                                                                    
                                          
  
                
                                                                       
                                                                      
                                                                       
                                                                   
                                                                   
  
                                                                      
                               
  
                                                                 
                                                                       
                                                                           
                                                                       
                                                   
                                         
   
	virtual IOReturn updateChannelValues(int channel_index);


                                               
                                                                         
  
                                                                
                                                                            
                                                            
                              
  
                
                                                                    
                                                
  
                                                                       
   
	IOReturn updateReportChannel(int channel_index,
	    int *nElements,
	    IOBufferMemoryDescriptor *destination);


                                            
                                                                    
  
                                                                    
                                                                               
                                                                              
                              
  
                
                                                                       
                                                                         
                                                                      
                                                              
  
                                                                       
   
	virtual IOReturn setElementValues(int element_index,
	    IOReportElementValues *values,
	    uint64_t record_time = 0);

                                            
                                                                            
  
                                                                    
                                                                             
  
                                                                                   
                                                                         
   
	virtual const IOReportElementValues* getElementValues(int element_index);

                                                
                                                              
  
                                           
                                                                  
                                          
  
                
                                                            
  
                                                                       
   
	virtual IOReturn getFirstElementIndex(uint64_t channel_id,
	    int *element_index);

                                           
                                                                             
  
                                           
                                                                  
                                          
  
                
                                                           
  
                                                                       
   
	virtual IOReturn getChannelIndex(uint64_t channel_id,
	    int *channel_index);

                                             
                                                                     
                                                                 
  
                                            
                                                                   
                                                                   
                                           
  
                 
                                                                    
                                                                        
                                                                        
                                         
  
                                                                       
   
	virtual IOReturn getChannelIndices(uint64_t channel_id,
	    int *channel_index,
	    int *element_index);

                                             
                                                                           
  
                                                                       
                                                                        
                                                  
  
                
                                                             
                                                         
  
                                                                       
   
	virtual IOReturn copyElementValues(int element_index,
	    IOReportElementValues *elementValues);

                  
private:
                                          
                                                       
                            
  
                                                                      
  
                
                                                                     
                                                                        
                                                                            
  
                                                                       
   
	OSPtr<OSArray> copyChannelIDs(void);

                                      
                                                              
  
                                                                     
                                                                     
                                                                  
                                                                              
  
                                                                 
  
                
                                                                                 
                                                                  
                                                                   
                                                                            
                  
  
                                                                      
   
	static OSPtr<IOReportLegendEntry> legendWith(OSArray *channelIDs,
	    OSArray *channelNames,
	    IOReportChannelType channelType,
	    IOReportUnit unit);

                                                          
protected:
	IOReportChannelType _channelType;
	uint64_t            _driver_id;                             

                                                                      
	IOReportElement    *_elements;
	int                *_enableCounts;                                     
	uint16_t            _channelDimension;                   
	int                 _nElements;
	int                 _nChannels;                                       
	OSPtr<OSArray>      _channelNames;

                                              
	bool                _reporterIsLocked;
	bool                _reporterConfigIsLocked;

                                          
	IOReportElement    *_swapElements;
	int                *_swapEnableCounts;

                             
private:
	IOReportUnit       _unit;

	int                 _enabled;                            

	IOLock             *_configLock;
	IOInterruptState    _interruptState;
	IOSimpleLock       *_reporterLock;
};


                                      
                                      
                                      

   
                                 
                                       
                
                                                                        
                                                                  
   

class IOSimpleReporter : public IOReporter
{
	OSDeclareDefaultStructors(IOSimpleReporter);

public:

                                      
                                                      
  
                                                                      
                                                                               
                                                                             
                                                                       
  
                
                                                       
  
                                                                       
   
	static OSPtr<IOSimpleReporter> with(IOService *reportingService,
	    IOReportCategories categories,
	    IOReportUnit unit);

                                          
                                                    
  
                                                                               
                                      
                                          
  
                
                                                              
  
                                                            
   
	IOReturn setValue(uint64_t channel_id,
	    int64_t value);

                                                
                                                                            
  
                                                                                       
                                                                        
                                          
                
                                                                       
  
                                                            
   
	IOReturn incrementValue(uint64_t channel_id,
	    int64_t increment);

                                          
                                                     
  
                                                               
                                                                
                
                                                            
  
                                                            
   
	int64_t getValue(uint64_t channel_id);

protected:

                                          
                                                                                
  
                
                                            
  
                                              
   
	virtual bool initWith(IOService *reportingService,
	    IOReportCategories categories,
	    IOReportUnit unit);

private:
};



   
                                
                                          
                
                                                                            
                                                                             
   
class IOStateReporter : public IOReporter
{
	OSDeclareDefaultStructors(IOStateReporter);

public:

                                     
                                                      
  
                                                                                
                                                                     
                                                                            
                                                                       
                                                                   
  
                
                                                                        
                                                                         
                                                   
                                                                
                                                              
  
                                                                      
   
	static OSPtr<IOStateReporter> with(IOService *reportingService,
	    IOReportCategories categories,
	    int nstates,
	    IOReportUnit unit = kIOReportUnitHWTicks);

                                           
                                                   
  
                                                                        
                                                                         
                                                                       
  
                                          
  
                
                                                                     
                                                                      
                                                                       
                                                                           
                                                                          
                                                                        
                                                                     
  
                                                            
   
	IOReturn setStateID(uint64_t channel_id,
	    int state_index,
	    uint64_t state_id);

                                                
                                                                      
  
                                                                           
                                                                   
                                                                      
                                                                            
                                          
  
                
                                                                           
                                                                         
                                                                            
                                                                            
                                                                            
                                                        
  
                                                                
                                                           
  
                                                                      
                                                                          
                                                                       
                                                                  
                                                                      
                                                                        
                                                                   
                                                                        
  
                                                            
   
	IOReturn setChannelState(uint64_t channel_id,
	    uint64_t new_state_id,
	    uint64_t last_intransition,
	    uint64_t prev_state_residency) __deprecated;

                                                
                                                                      
  
                                                                           
                                                                   
                                          
  
                
                                                                           
                                                                         
                                                                            
                                                                            
                                                                            
                                                        
  
                                                                
                                                           
  
                                                                      
                                                                          
                                                                       
                                                                  
                                                                      
                                                                        
                                                                   
                                                                        
  
                                                            
   
	IOReturn setChannelState(uint64_t channel_id,
	    uint64_t new_state_id);


                                         
                                                           
  
                                                     
                                           
  
                
                                                                    
                                                                       
                                               
  
                                                     
  
                                                            
   
	IOReturn setState(uint64_t new_state_id);

                                         
                                                           
  
                                                     
                                                                      
                                                                       
                                           
  
                
                                                                    
                                                                       
                                               
  
                                                     
  
                                                            
   
	IOReturn setState(uint64_t new_state_id,
	    uint64_t last_intransition,
	    uint64_t prev_state_residency) __deprecated;

                                                     
                                                                         
  
                                                                        
                                                
                                                                 
                                                                   
                                                                              
                                          
  
                
                                                                       
                                                                   
                                                                         
                                                               
                                                                     
                                                                     
                                        
  
                                                                            
                                                                         
                                                                          
                                                                        
                                                   
  
                                                                          
                                                                     
                                                                        
                                                                            
                                                                        
                              
  
                                                                           
                                                                 
                                                                           
                                                        
                                                                         
                                                                         
                                                                         
                                                      
                               
  
                                                            
   
	IOReturn overrideChannelState(uint64_t channel_id,
	    uint64_t state_id,
	    uint64_t time_in_state,
	    uint64_t intransitions,
	    uint64_t last_intransition = 0);

                                                      
                                                                       
  
                                                                        
                                                
                                                                     
                                                                         
                                                                              
                                          
  
                
                                                                     
                                                                           
                                                                          
                                                                          
                                                                        
                                                           
  
                                                                            
                                                                         
                                                                          
                                                                        
                                                   
  
                                                                          
                                                                     
                                                                        
                                                                            
                                                                        
                              
  
                                                                           
                                                                  
                                                                     
                                                               
                                                                       
                                                                       
                                                                        
                                                            
                               
  
                                                            
   
	IOReturn incrementChannelState(uint64_t channel_id,
	    uint64_t state_id,
	    uint64_t time_in_state,
	    uint64_t intransitions,
	    uint64_t last_intransition = 0);

                                                  
                                                                     
  
                                                                             
                                                                   
                                          
  
                
                                                                          
                                                                         
                                                                       
                                                                            
                                                                        
                                                                   
                                                                
  
                                                                       
                                                                       
                                                                    
                                                   
  
                                                                      
                                                                          
                                                                       
                                                                    
                                                                      
                                                                        
                                                                   
                                                                        
  
                                                            
   
	IOReturn setStateByIndices(int channel_index,
	    int new_state_index);

                                                  
                                                                     
  
                                                                             
                                                                   
                                                                      
                                                                            
                                          
  
                
                                                                          
                                                                         
                                                                       
                                                                            
                                                                        
                                                                   
                                                                
  
                                                                       
                                                                       
                                                                    
                                                   
  
                                                                      
                                                                          
                                                                       
                                                                    
                                                                      
                                                                        
                                                                   
                                                                        
  
                                                            
   
	IOReturn setStateByIndices(int channel_index,
	    int new_state_index,
	    uint64_t last_intransition,
	    uint64_t prev_state_residency) __deprecated;

                                                      
                                                                    
  
                                           
                                            
                                                               
  
                
                                                                               
                                                                              
                                                                 
  
                                                            
   
	uint64_t getStateInTransitions(uint64_t channel_id,
	    uint64_t state_id);

                                                      
                                                                
  
                                           
                                            
                                                       
  
                
                                                                        
                                                                       
                                                                          
                              
  
                                                            
   
	uint64_t getStateResidencyTime(uint64_t channel_id,
	    uint64_t state_id);

                                                           
                                                                   
  
                                           
                                            
                                                                   
  
                
                                                                        
                                                                       
                                                                           
                                       
  
                                                            
   
	uint64_t getStateLastTransitionTime(uint64_t channel_id, uint64_t state_id);

                                                                           
                                                                             
  
                                           
                                                                    
  
                
                                                                      
                                                                          
                                                                         
                                                                           
                                                                        
                          
  
                                                            
   
	uint64_t getStateLastChannelUpdateTime(uint64_t channel_id) __deprecated;

                                     
                                                           
  
                
                                                                        
                                                                         
                   
  
                                              
   
	virtual void free(void) APPLE_KEXT_OVERRIDE;

protected:

                                         
                                                                
  
                
                                            
   
	virtual bool initWith(IOService *reportingService,
	    IOReportCategories categories,
	    int16_t nstates, IOReportUnit unit);


                                                  
                                                                        
                                                 
   
	virtual IOReturn handleSwapPrepare(int newNChannels) APPLE_KEXT_OVERRIDE;

   
                                                      
                                                    
   
	virtual IOReturn handleAddChannelSwap(uint64_t channel_id,
	    const OSSymbol *symChannelName) APPLE_KEXT_OVERRIDE;

   
                                                   
                                                  
   
	virtual void handleSwapCleanup(int swapNChannels) APPLE_KEXT_OVERRIDE;

                                                    
                                                                 
  
                                                          
                                          
  
                
                                                                        
                                                                          
                  
  
                                                                       
   
	virtual IOReturn updateChannelValues(int channel_index) APPLE_KEXT_OVERRIDE;

                                                  
                                                                     
  
                                                                             
                                                        
                                                                     
                                                                           
                                          
  
                
                                                                               
                                  
  
                                                                       
   
	virtual IOReturn handleSetStateByIndices(int channel_index,
	    int new_state_index,
	    uint64_t last_intransition,
	    uint64_t prev_state_residency);

                                           
                                                   
  
                                                                        
                                                                         
                                                                       
  
                                          
  
                
                                                                       
                                 
  
                                                                       
   
	virtual IOReturn handleSetStateID(uint64_t channel_id,
	    int state_index,
	    uint64_t state_id);

                                                                    
                                                                         
  
                                                                              
                                                                
                                                                 
                                                                   
                                                                              
                                          
  
                
                                                                           
                                         
  
                                                                       
   
	virtual IOReturn handleOverrideChannelStateByIndices(int channel_index,
	    int state_index,
	    uint64_t time_in_state,
	    uint64_t intransitions,
	    uint64_t last_intransition = 0);

                                                                     
                                                                       
  
                                                                              
                                                                
                                                                 
                                                                   
                                                                              
                                          
  
                
                                                                           
                                         
  
                                                                       
   
	virtual IOReturn handleIncrementChannelStateByIndices(int channel_index,
	    int state_index,
	    uint64_t time_in_state,
	    uint64_t intransitions,
	    uint64_t last_intransition = 0);
private:

	int            *_currentStates;                                    
	uint64_t       *_lastUpdateTimes;                             

                                          
	int            *_swapCurrentStates;
	uint64_t       *_swapLastUpdateTimes;

	enum valueSelector {
		kInTransitions,
		kResidencyTime,
		kLastTransitionTime
	};
	uint64_t _getStateValue(uint64_t channel_id,
	    uint64_t state_id,
	    enum valueSelector value);

	IOReturn _getStateIndices(uint64_t channel_id,
	    uint64_t state_id,
	    int *channel_index,
	    int *state_index);
};


   
                                    
                                            
                
                                                                       
                                                 
   
class IOHistogramReporter : public IOReporter
{
	OSDeclareDefaultStructors(IOHistogramReporter);

public:
                                         
                                                                                           
  
                                                                                
                                                                                 
                                                    
                                                     
                                                                             
                                                                                          
                                                                                       
                                                                            
  
                
                                                          
  
                                              
  
                                                                      
  
  
   
	static OSPtr<IOHistogramReporter> with(IOService *reportingService,
	    IOReportCategories categories,
	    uint64_t channelID,
	    const char *channelName,
	    IOReportUnit unit,
	    int nSegments,
	    IOHistogramSegmentConfig *config);

                                               
                                                                      
  
                                                                       
   
	IOReturn
	addChannel(__unused uint64_t channelID, __unused const char *channelName = NULL)
	{
		return kIOReturnUnsupported;
	}

                                                         
                                                               
  
                                                
                                                
                                                  
                                                  
                                        
                                          
  
                
                                                                                 
                                                                             
                                                            
  
                                                            
   

	IOReturn overrideBucketValues(unsigned int index,
	    uint64_t bucket_hits,
	    int64_t bucket_min,
	    int64_t bucket_max,
	    int64_t bucket_sum);

                                               
                                                 
  
                                                      
                                                                 
  
                
                                                                    
                                                                 
                                                                
  
                                                            
   
	int tallyValue(int64_t value);

                                         
                                                           
  
                
                                                                       
                                                                        
                  
  
                                              
   
	virtual void free(void) APPLE_KEXT_OVERRIDE;

protected:

                                             
                                                                
  
                
                                            
   
	virtual bool initWith(IOService *reportingService,
	    IOReportCategories categories,
	    uint64_t channelID,
	    const OSSymbol *channelName,
	    IOReportUnit unit,
	    int nSegments,
	    IOHistogramSegmentConfig  *config);

                                                       
                                                                                               
  
                                                          
  
                
                                                                        
                                                                  
                                               
  
                                                       
   
	OSPtr<IOReportLegendEntry> handleCreateLegend(void) APPLE_KEXT_OVERRIDE;


private:

	int                         _segmentCount;
	int64_t                    *_bucketBounds;
	int                         _bucketCount;
	IOHistogramSegmentConfig   *_histogramSegmentsConfig;
};


                                     
                                     
                                     

   
                               
                                                              
                
                                                                         
                                                              
                                                             
   
class IOReportLegend : public OSObject
{
	OSDeclareDefaultStructors(IOReportLegend);

public:
                                    
                                                     
  
                                                                                
                                                                  
  
                
                                                                            
                                                                        
                                                                   
                                                                           
  
                         
                                   
                                  
                                                                    
                                                          
  
                                                                   
                                                             
                                                                         
                                                       
  
                                                       
                                                                            
                                                                 
   
	static OSPtr<IOReportLegend> with(OSArray *legend);

                                              
                                       
  
                                                                         
                                                          
                                                               
                                          
  
                
                                                                        
                                                                     
                                                                        
                                                                        
                                                      
  
                                                                       
                                                                         
                                                                     
                                                                     
   
	IOReturn addLegendEntry(IOReportLegendEntry *legendEntry,
	    const char *groupName,
	    const char *subGroupName);

                                                 
                                                          
  
                                                                          
                                                          
                                                               
                                          
  
                
                                                                              
                                                                             
                                                                      
                                                                     
                                                                        
                                                                        
                                                      
  
                                                                       
                                                                         
                                                                     
                                                                     
  
                                                                             
   
	IOReturn addReporterLegend(IOReporter *reporter,
	    const char *groupName,
	    const char *subGroupName);

                                                 
                                                          
  
                                                                                
                                                                          
                                                          
                                                               
                                          
  
                
                                                                              
                                                                             
                                                                      
                                                                     
                                                                        
                                                                        
                                                      
  
                                                                       
                                                                         
                                                                     
                                                                     
  
                                                                           
                                                                      
                                                                            
  
                                                                             
   
	static  IOReturn addReporterLegend(IOService *reportingService,
	    IOReporter *reporter,
	    const char *groupName,
	    const char *subGroupName);

                                         
                                                        
  
                                                                                      
                
                                                                        
   
	OSArray* getLegend(void);

                                    
                                                
  
                
                                                                   
  
                                                                      
                                                                       
                                                                         
                                                                    
                  
   
	void free(void) APPLE_KEXT_OVERRIDE;



protected:

private:

	OSPtr<OSArray>     _reportLegend;

	IOReturn initWith(OSArray *legend);

                                              
                                                                                       
  
                                           
                                                
                              
   
	IOReturn organizeLegend(IOReportLegendEntry *legendEntry,
	    const OSSymbol *groupName,
	    const OSSymbol *subGroupName);

                                        
                                               
                                                                          
  
                                                                                       
                                          
                                                                      
                                                             
  
                
                                             
                                                                                      
                                                                                       
                                                                                 
                                                                                
                                                                    
   
                                  
                                                              
};

#endif                                
