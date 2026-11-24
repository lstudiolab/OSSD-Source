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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 *  Copyright (c) 1998 Apple Computer, Inc.	 All rights reserved.
 *
 *  HISTORY
 *   1998-7-13	Godfrey van der Linden(gvdl)
 *       Created.
 *   1998-10-30	Godfrey van der Linden(gvdl)
 *       Converted to C++
 */

#ifndef _IOKIT_IOINTERRUPTEVENTSOURCE_H
#define _IOKIT_IOINTERRUPTEVENTSOURCE_H

#include <libkern/c++/OSPtr.h>
#include <IOKit/IOEventSource.h>

class IOService;

struct IOInterruptAccountingData;

                                                        
                                                                              
                                                                                                                                                                                                                                                                                                                                               
            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
            
                                                                                                                                                                                                                                                            
            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
            
                                                                                                                                                                                                                                                                                                         
            
                                                                                                                                                                     
   
class IOInterruptEventSource : public IOEventSource
{
	OSDeclareDefaultStructors(IOInterruptEventSource);

public:
                   
                                                                                                                          
                                             
                                                                
                                                               
	typedef void (*Action)(OSObject *owner, IOInterruptEventSource *sender, int count);

#ifdef __BLOCKS__
	typedef void (^ActionBlock)(IOInterruptEventSource *sender, int count);
#endif                 

                                   
                                                                                                                                        
#define IOInterruptEventAction IOInterruptEventSource::Action

protected:
                                                                     
	IOService *provider;

                    
	int intIndex;

                      
                                                                     
	volatile unsigned int producerCount;

                      
                                                                                 
	unsigned int consumerCount;

                                                                                                                                        
	bool autoDisable;

                                                                                                                                                      
	bool explicitDisable;

                         
                                                                                                        
   
	struct ExpansionData {
		IOInterruptAccountingData * statistics;
	};

                 
                                                      
	APPLE_KEXT_WSHADOW_PUSH;
	ExpansionData *reserved;
	APPLE_KEXT_WSHADOW_POP;

                  
                                                                                                 
	virtual void free() APPLE_KEXT_OVERRIDE;

                          
                                                                                           
                                                                                                                                    
                                                                                                                            
	virtual bool checkForWork() APPLE_KEXT_OVERRIDE;

                         
                                                                 
	virtual void setWorkLoop(IOWorkLoop *inWorkLoop) APPLE_KEXT_OVERRIDE;

public:

                                  
                                                                                        
                                                        
                                                               
                                                                                                                                                                                                                                                                                                                        
                                                                                                                                                  
                                                                                                   
	static OSPtr<IOInterruptEventSource>
	interruptEventSource(OSObject *owner,
	    Action action,
	    IOService *provider = NULL,
	    int intIndex = 0);


#ifdef __BLOCKS__
                                  
                                                                                        
                                                        
                                                                                                                                                                                                                                                                                                        
                                                                                        
                                                                       
                                                                                                   
	static OSPtr<IOInterruptEventSource>
	interruptEventSource(OSObject *owner,
	    IOService *provider,
	    int intIndex,
	    ActionBlock action);
#endif                 

#if XNU_KERNEL_PRIVATE
	static void actionToBlock(OSObject *owner, IOInterruptEventSource *sender, int count);
#endif                         

                  
                                                                        
                                                        
                                                               
                                                                                                                                                                                                                                                                                                                        
                                                                                                                                                  
                                                                       
                     
	virtual bool init(OSObject *owner,
	    Action action,
	    IOService *provider = NULL,
	    int intIndex = 0);

                    
                                   
                                                                             
                                                                            
                                                                     
	virtual void enable() APPLE_KEXT_OVERRIDE;

                     
                                    
                                                                             
                                         
	virtual void disable() APPLE_KEXT_OVERRIDE;

                         
                                                   
                                  
	virtual const IOService *getProvider() const;

                         
                                                                   
                                  
	virtual int getIntIndex() const;

                            
                                                      
                                     
	virtual bool getAutoDisable() const;

                               
                                                                                                            
                                                      
                                                              
	virtual void interruptOccurred(void *, IOService *nub, int ind);

                                     
                                                                                                           
                                                      
                                                              
	virtual void normalInterruptOccurred(void *, IOService *nub, int ind);

                                      
                                                                                                           
                                                      
                                                              
	virtual void disableInterruptOccurred(void *, IOService *nub, int ind);

                     
                                                                                                     
                                                                                                                          
                                                                                                                        
                                                                                                                          
                                                                                                                               
                                                                                                                              
                                                                                                                                 
                                                           
	IOReturn warmCPU(uint64_t abstime);

                                             
                                                                             
                                                                               
                                               

	void enablePrimaryInterruptTimestamp(bool enable);

                                          
                                                                         
                                                                           
                                                                                                                         

	uint64_t getPrimaryInterruptTimestamp();

private:
	IOReturn registerInterruptHandler(IOService *inProvider, int inIntIndex);
	void unregisterInterruptHandler(IOService *inProvider, int inIntIndex);

private:
	OSMetaClassDeclareReservedUnused(IOInterruptEventSource, 0);
	OSMetaClassDeclareReservedUnused(IOInterruptEventSource, 1);
	OSMetaClassDeclareReservedUnused(IOInterruptEventSource, 2);
	OSMetaClassDeclareReservedUnused(IOInterruptEventSource, 3);
	OSMetaClassDeclareReservedUnused(IOInterruptEventSource, 4);
	OSMetaClassDeclareReservedUnused(IOInterruptEventSource, 5);
	OSMetaClassDeclareReservedUnused(IOInterruptEventSource, 6);
	OSMetaClassDeclareReservedUnused(IOInterruptEventSource, 7);
};

#endif                                       
