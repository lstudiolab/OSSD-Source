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
/*
 *  Copyright (c) 1999 Apple Computer, Inc.	 All rights reserved.
 *
 *  HISTORY
 *   1999-4-15	Godfrey van der Linden(gvdl)
 *       Created.
 */
#ifndef _IOKIT_IOFILTERINTERRUPTEVENTSOURCE_H
#define _IOKIT_IOFILTERINTERRUPTEVENTSOURCE_H

#include <libkern/c++/OSPtr.h>
#include <IOKit/IOInterruptEventSource.h>
#include <libkern/c++/OSPtr.h>

class IOService;

                                                                       
                                                                     
                                                                                                                                                                                                                                                                                                                                      
            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
            
                                                                                                                                                                                                                                                
   
class IOFilterInterruptEventSource : public IOInterruptEventSource
{
	OSDeclareDefaultStructors(IOFilterInterruptEventSource);

public:
   
                    
                                                                                  
                                                        
                                                       
                                                       
	typedef bool (*Filter)(OSObject *owner, IOFilterInterruptEventSource *sender);

                                    
                                                                                                                                         
#define IOFilterInterruptAction IOFilterInterruptEventSource::Filter

#ifdef __BLOCKS__
	typedef bool (^FilterBlock)(IOFilterInterruptEventSource *sender);
#endif                 

private:
                                   
	virtual bool init(OSObject *inOwner,
	    IOInterruptEventSource::Action inAction = NULL,
	    IOService *inProvider = NULL,
	    int inIntIndex = 0) APPLE_KEXT_OVERRIDE;

	static OSPtr<IOInterruptEventSource>
	interruptEventSource(OSObject *inOwner,
	    IOInterruptEventSource::Action inAction = NULL,
	    IOService *inProvider = NULL,
	    int inIntIndex = 0);

protected:
                                       

#if XNU_KERNEL_PRIVATE
	union { Filter filterAction; FilterBlock filterActionBlock; };
#else                         
	Filter filterAction;
#endif                          

                         
                                                                                                        
   
	struct ExpansionData { };

                 
                                                      
	APPLE_KEXT_WSHADOW_PUSH;
	ExpansionData *reserved;
	APPLE_KEXT_WSHADOW_POP;

public:
                                        
                                                                                                       
                                                    
                                                               
                                                              
                                                      
                                   
                                                              
	static OSPtr<IOFilterInterruptEventSource>
	filterInterruptEventSource(OSObject *owner,
	    IOInterruptEventSource::Action action,
	    Filter filter,
	    IOService *provider,
	    int intIndex = 0);

#ifdef __BLOCKS__
                                        
                                                                                                       
                                                    
                                                      
                                                                                        
                                                                      
                                                            
                                                              
	static OSPtr<IOFilterInterruptEventSource>
	filterInterruptEventSource(OSObject *owner,
	    IOService *provider,
	    int intIndex,
	    IOInterruptEventSource::ActionBlock action,
	    FilterBlock filter);
#endif                 

#if XNU_KERNEL_PRIVATE
	enum{
		kFilterBlock = kSubClass0,
	};
#endif

                  
                                                                              
                                                    
                                                               
                                                                     
                                                      
                                                                      
                                                                       
                     
	virtual bool init(OSObject *owner,
	    IOInterruptEventSource::Action action,
	    Filter filter,
	    IOService *provider,
	    int intIndex = 0);

	virtual void free( void ) APPLE_KEXT_OVERRIDE;

                             
                                                          
                                                                                                                                                                                                                                                                                                                                           
	virtual void signalInterrupt();

                             
                                                 
                                      
	virtual Filter getFilterAction() const;

#ifdef __BLOCKS__
                                  
                                                 
                                      
	FilterBlock getFilterActionBlock() const;
#endif                 

                                     
                                                                                                         
	virtual void normalInterruptOccurred(void *self, IOService *prov, int ind) APPLE_KEXT_OVERRIDE;

                                      
                                                                                                           
	virtual void disableInterruptOccurred(void *self, IOService *prov, int ind) APPLE_KEXT_OVERRIDE;

private:
	OSMetaClassDeclareReservedUnused(IOFilterInterruptEventSource, 0);
	OSMetaClassDeclareReservedUnused(IOFilterInterruptEventSource, 1);
	OSMetaClassDeclareReservedUnused(IOFilterInterruptEventSource, 2);
	OSMetaClassDeclareReservedUnused(IOFilterInterruptEventSource, 3);
	OSMetaClassDeclareReservedUnused(IOFilterInterruptEventSource, 4);
	OSMetaClassDeclareReservedUnused(IOFilterInterruptEventSource, 5);
	OSMetaClassDeclareReservedUnused(IOFilterInterruptEventSource, 6);
	OSMetaClassDeclareReservedUnused(IOFilterInterruptEventSource, 7);
};

#endif                                             
