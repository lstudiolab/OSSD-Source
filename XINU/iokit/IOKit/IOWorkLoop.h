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

#ifndef __IOKIT_IOWORKLOOP_H
#define __IOKIT_IOWORKLOOP_H

#include <libkern/c++/OSObject.h>
#include <IOKit/IOReturn.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOLocks.h>
#include <libkern/c++/OSPtr.h>

#include <IOKit/system.h>

#if IOKITSTATS
#include <IOKit/IOStatisticsPrivate.h>
#endif

class IOEventSource;
class IOTimerEventSource;
class IOCommandGate;

                     
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
            
                                                                                                                                                                                                                                                                                                                                                                                                            
            
                                                                                                                        
   
class IOWorkLoop : public OSObject
{
	OSDeclareDefaultStructors(IOWorkLoop);

public:
   
                    
                                                                           
                                                                                
                                                                                  
                                                      
                  
                                                                                 
                                                                                 
                                     
                                                       
                                                       
                                                       
                                                       
   
	typedef IOReturn (*Action)(OSObject *target,
	    void *arg0, void *arg1,
	    void *arg2, void *arg3);

#ifdef __BLOCKS__
	typedef IOReturn (^ActionBlock)();
#endif                 

	enum {
		kPreciousStack  = 0x00000001,
		kTimeLockPanics = 0x00000002,
	};

private:
                                    
                                                                  
   
	static void threadMainContinuation(IOWorkLoop *self);

                                     
                                                                                                     
                                                                                                       
                                                        
   
	bool eventSourcePerformsWork(IOEventSource *inEventSource);

                               
                                                                           
                              
   
	static void releaseEventChain(LIBKERN_CONSUMED IOEventSource *eventChain);

protected:

                             
                                                                          
                                                                   
                                                                      
   
	typedef enum { mAddEvent, mRemoveEvent } maintCommandEnum;

                 
                                                                         
                                                                                                                                                                                                                                                                                                                        
   
	IORecursiveLock *gateLock;

                   
                                                  
   
	IOEventSource *eventChain;

                 
                                                    
   
	IOCommandGate *controlG;

                     
                                                                 
   
	IOSimpleLock *workToDoLock;

                   
                      
   
	IOThread workThread;

                 
                                                                               
   
	volatile bool workToDo;

                    
                                                                                                               
   
	bool loopRestart;

                         
                                                                                                        
   
	struct ExpansionData {
		IOOptionBits options;
		IOEventSource *passiveEventChain;
#if IOKITSTATS
		struct IOWorkLoopCounter *counter;
#else
		void *iokitstatsReserved;
#endif
		uint64_t lockInterval;
		uint64_t lockTime;
	};

                 
                                                  
   
	ExpansionData *reserved;

                           
                                                                                            
                                                                                                                                                                                                                                                                                                                                               
                                                                                                      
   
	virtual IOReturn _maintRequest(void *command, void *data, void *, void *);

                  
                                                                                                                                                                                                                                                                                            
            
                                                                                                                                                                                                                              
   
	virtual void free() APPLE_KEXT_OVERRIDE;

                        
                                                                              
                                                                               
                                                                              
                                                                            
                                                                              
                                                                           
                                       
   
	virtual void threadMain();

public:

                      
                                                                              
                                                                                  
   
	static OSPtr<IOWorkLoop> workLoop();

                                                       
                                                                             
                                                                                        
                                                                                  
   
	static OSPtr<IOWorkLoop> workLoopWithOptions(IOOptionBits options);

                  
                                                                                                                                                                                                     
                                                                       
   
	virtual bool init() APPLE_KEXT_OVERRIDE;

                       
                                   
                                
   
	virtual IOThread getThread() const;

                      
                                                                   
                                                          
   
	virtual bool onThread() const;

                    
                                                                             
                                                           
   
	virtual bool inGate() const;

                            
                                                                                                                                                                                                                                                                                                                                              
                                                              
                                             
   
	virtual IOReturn addEventSource(IOEventSource *newEvent);

                               
                                                                                                                                                                                                                                                                                                                                
                                                                 
                                                                                                        
   
	virtual IOReturn removeEventSource(IOEventSource *toRemove);

                                   
                                                   
                                                                                                           
   
	virtual void enableAllEventSources() const;

                                    
                                                    
                                                                                                             
   
	virtual void disableAllEventSources() const;

                                 
                                                             
                                                                                                                                                                             
   
	virtual void enableAllInterrupts() const;

                                  
                                                              
                                                                                                                                                                                
   
	virtual void disableAllInterrupts() const;


protected:
                                                            
	friend class IOEventSource;
	friend class IOTimerEventSource;
	friend class IOCommandGate;
#if IOKITSTATS
	friend class IOStatistics;
#endif
	virtual void signalWorkAvailable();
	virtual void openGate();
	virtual void closeGate();
	virtual bool tryCloseGate();
	virtual int  sleepGate(void *event, UInt32 interuptibleType);
	virtual void wakeupGate(void *event, bool oneThread);

public:
                                                 

                       
                                                                    
                                                                                                                                                                                                                                                                                                                                                                                                                
                                                                           
                                                               
                                                               
                                                               
                                                               
                                                     
   
	virtual IOReturn runAction(Action action, OSObject *target,
	    void *arg0 = NULL, void *arg1 = NULL,
	    void *arg2 = NULL, void *arg3 = NULL);

#ifdef __BLOCKS__
                       
                                                                    
                                                                                                                                                                                                                                                                                                                                                                                                                
                                                             
                                                    
   
	IOReturn runActionBlock(ActionBlock action);
#endif                 

                             
                                                                              
                                                                              
                                                                               
                                                                             
                            
            
                                                                               
                                                                             
                                                                             
                                                                               
                                                                              
                                                                               
                                                                              
                                                                       
              
  
                                                                            
   
	virtual bool runEventSources();

                                
                                                                                                                                                                                        
                                                                                                           
                                                                                                                                                        
   
	void setMaximumLockTime(uint64_t interval, uint32_t options);

protected:
                                                            
	virtual int sleepGate(void *event, AbsoluteTime deadline, UInt32 interuptibleType);

#if XNU_KERNEL_PRIVATE
	void lockTime(void);
#endif                         

protected:
#if __LP64__
	OSMetaClassDeclareReservedUnused(IOWorkLoop, 0);
	OSMetaClassDeclareReservedUnused(IOWorkLoop, 1);
	OSMetaClassDeclareReservedUnused(IOWorkLoop, 2);
#else
	OSMetaClassDeclareReservedUsedX86(IOWorkLoop, 0);
	OSMetaClassDeclareReservedUsedX86(IOWorkLoop, 1);
	OSMetaClassDeclareReservedUsedX86(IOWorkLoop, 2);
#endif
	OSMetaClassDeclareReservedUnused(IOWorkLoop, 3);
	OSMetaClassDeclareReservedUnused(IOWorkLoop, 4);
	OSMetaClassDeclareReservedUnused(IOWorkLoop, 5);
	OSMetaClassDeclareReservedUnused(IOWorkLoop, 6);
	OSMetaClassDeclareReservedUnused(IOWorkLoop, 7);
};

#endif                            
