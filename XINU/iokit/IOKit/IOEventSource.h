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
 *  HISTORY
 *   1998-7-13	Godfrey van der Linden(gvdl)
 *       Created.
 *   1998-10-30	Godfrey van der Linden(gvdl)
 *       Converted to C++
 */
#ifndef _IOKIT_IOEVENTSOURCE_H
#define _IOKIT_IOEVENTSOURCE_H

#include <sys/cdefs.h>

#include <libkern/c++/OSObject.h>

#include <IOKit/IOLib.h>
#include <IOKit/system.h>
#include <IOKit/IOWorkLoop.h>

#if IOKITSTATS
#include <IOKit/IOStatisticsPrivate.h>
#endif

__BEGIN_DECLS
#include <mach/clock_types.h>
#include <kern/clock.h>
__END_DECLS

   
                                           
                                                              
                                                                             
                                                                                    
            
                                                                                     
                                                                              
                                                                                   
                                    
            
                                                                                     
                                                                               
                                                                                 
                                                                              
                                                                               
                                                                              
                          
            
                                                                                                                                                                                                                                                                                                                                                                                                  
            
                                                                                                                                                                                                                                                                                               
  
            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
            
                                                                                                                                                                                                                                                       
   
class IOEventSource : public OSObject
{
	OSDeclareAbstractStructors(IOEventSource);
	friend class IOWorkLoop;
#if IOKITSTATS
	friend class IOStatistics;
#endif

public:
   
                    
                                                                              
                                                                                
                                           
                 
                                                                           
                                                                               
                                                                                    
   
	typedef void (*Action)(OSObject *owner, ...);

                                
                                                                                                                               
 #define IOEventSourceAction IOEventSource::Action

#ifdef __BLOCKS__
	typedef IOReturn (^ActionBlock)();
#endif                 

protected:
                       
                                                                          
	IOEventSource *eventChainNext;

                                                                           
	OSObject *owner;

               
                                                                     

#if XNU_KERNEL_PRIVATE
	union { Action action; ActionBlock actionBlock; };
#else                         
	Action action;
#endif                          

                
                                                                              
	bool enabled;

#if XNU_KERNEL_PRIVATE
	enum{
		kPassive         = 0x0001,
		kActive          = 0x0002,
		kActionBlock     = 0x0004,
		kSubClass0       = 0x0008,
	};
	uint8_t  eventSourceReserved1[1];
	uint16_t flags;
#if __LP64__
	uint8_t eventSourceReserved2[4];
#endif               

#endif                         

                                                                 
	IOWorkLoop *workLoop;

                                                                         
	void *refcon;

                         
                                                                                                           
   
	struct ExpansionData {
#if IOKITSTATS
		struct IOEventSourceCounter *counter;
#else
		void *iokitstatsReserved;
#endif
	};

                 
                                                      
	ExpansionData *reserved;

                  
                                                               
                 
                                                                                
                                                     
                  
                                                                             
                                                                                  
                                                                             
                                                                                 
                                       
                                                                       
                 
   
	virtual bool init(OSObject *owner, IOEventSource::Action action = NULL);

	virtual void free( void ) APPLE_KEXT_OVERRIDE;

                          
                                                                  
               
                                                                            
                                                                                
                                                                           
                                                                                 
                                                                   
                                                                                                                         
   
	virtual bool checkForWork();

                         
                                                   
                    
                                                                       
                                                                                    
   
	virtual void setWorkLoop(IOWorkLoop *workLoop);

                     
                                                         
                
                                                   
   
	virtual void setNext(IOEventSource *next);

                     
                                                         
                                     
   
	virtual IOEventSource *getNext() const;


protected:
                                                   
	void signalWorkAvailable();
	void openGate();
	void closeGate();
	bool tryCloseGate();
	int sleepGate(void *event, UInt32 type);
	int sleepGate(void *event, AbsoluteTime deadline, UInt32 type);
	void wakeupGate(void *event, bool oneThread);

public:
                       
                                                 
                                                                            
	virtual void setAction(IOEventSource::Action action);

                       
                                                 
                                
	virtual IOEventSource::Action getAction() const;

#ifdef __BLOCKS__
                            
                                                                                                  
                                                                      
	void setActionBlock(ActionBlock block);
                            
                                      
                                                                                  
	ActionBlock getActionBlock(ActionBlock) const;
#endif                 

                       
                                                                                                
                             
	void setRefcon(void *refcon);
                       
                                      
                                                                                     
	void * getRefcon() const;

                    
                                   
                                                                             
                                                                            
                                                                     
	virtual void enable();

                     
                                    
                                                                             
                                         
	virtual void disable();

                       
                                                 
                                
	virtual bool isEnabled() const;

                         
                                                   
                                  
	virtual IOWorkLoop *getWorkLoop() const;

                      
                                                           
                                                    
   
	virtual bool onThread() const;

private:
	OSMetaClassDeclareReservedUnused(IOEventSource, 0);
	OSMetaClassDeclareReservedUnused(IOEventSource, 1);
	OSMetaClassDeclareReservedUnused(IOEventSource, 2);
	OSMetaClassDeclareReservedUnused(IOEventSource, 3);
	OSMetaClassDeclareReservedUnused(IOEventSource, 4);
	OSMetaClassDeclareReservedUnused(IOEventSource, 5);
	OSMetaClassDeclareReservedUnused(IOEventSource, 6);
	OSMetaClassDeclareReservedUnused(IOEventSource, 7);
};

#endif                              
