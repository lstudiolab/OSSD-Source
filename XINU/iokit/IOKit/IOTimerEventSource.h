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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
 *
 * IOTimerEventSource.h
 *
 * HISTORY
 * 2-Feb-1999		Joe Liu (jliu) created.
 *
 */

#ifndef _IOTIMEREVENTSOURCE
#define _IOTIMEREVENTSOURCE

#include <sys/cdefs.h>

__BEGIN_DECLS
#include <kern/clock.h>
__END_DECLS

#include <libkern/c++/OSPtr.h>
#include <IOKit/IOEventSource.h>
#include <IOKit/IOTypes.h>

   
                                                     
                                                                         
                                                                                                   
                                                                                      
                                                                                       
                                                                                              
                                                                                        
                        
                                                                                                  
                 
                                                                                                       
                 
                                                                                                            
                                                                             
                                                                                                             
                                            
                                                                                                             
                                                                                                            
                                                                                 
   
enum{
	kIOTimerEventSourceOptionsPriorityMask       = 0x000000ff,
	kIOTimerEventSourceOptionsPriorityHigh       = 0x00000000,
	kIOTimerEventSourceOptionsPriorityKernelHigh = 0x00000001,
	kIOTimerEventSourceOptionsPriorityKernel     = 0x00000002,
	kIOTimerEventSourceOptionsPriorityUser       = 0x00000003,
	kIOTimerEventSourceOptionsPriorityLow        = 0x00000004,
	kIOTimerEventSourceOptionsPriorityWorkLoop   = 0x000000ff,

	kIOTimerEventSourceOptionsAllowReenter       = 0x00000100,

	kIOTimerEventSourceOptionsDefault            = kIOTimerEventSourceOptionsPriorityKernelHigh
};

#define IOTIMEREVENTSOURCEOPTIONS_DEFINED       1

   
                                                               
                                                                                           
                                                                                 
                                                                                                
   
enum{
	kIOTimeOptionsWithLeeway = 0x00000020,
	kIOTimeOptionsContinuous = 0x00000100,
};

   
                                                     
                                                 
                                                                                                                                                                                                                           
            
                                                                                                                                                                                                                                                       
            
                                                                                                                                                                                                                                                                                                                                                                            
   

class IOTimerEventSource : public IOEventSource
{
	OSDeclareDefaultStructors(IOTimerEventSource);

protected:
                                                                            
	void *calloutEntry;

                                                     
	AbsoluteTime abstime;

                         
                                                                                    
   
	struct ExpansionData {
		SInt32       calloutGeneration;
		SInt32       calloutGenerationSignaled;
		IOWorkLoop * workLoop;
	};

                 
                                                      
	APPLE_KEXT_WSHADOW_PUSH;
	ExpansionData *reserved;
	APPLE_KEXT_WSHADOW_POP;

                     
                                                                                                     
                                                                                                                                                                               
                                                                        
	static void timeout(void *self);

                            
                                                             
                                                                                                                                                                                                                                                                                                                                                                        
	virtual void setTimeoutFunc();

                  
                                                                             
	virtual void free() APPLE_KEXT_OVERRIDE;

	virtual void setWorkLoop(IOWorkLoop *workLoop) APPLE_KEXT_OVERRIDE;

public:

                   
                                                                                        
                                                                                                                                                                           
                                                
	typedef void (*Action)(OSObject *owner, IOTimerEventSource *sender);

#ifdef __BLOCKS__
	typedef void (^ActionBlock)(IOTimerEventSource *sender);
#endif                 

	static OSPtr<IOTimerEventSource>
	timerEventSource(OSObject *owner, Action action = NULL);

                              
                                                                   
                                                                
                                                                             
                                                                                     
   
	static OSPtr<IOTimerEventSource>
	timerEventSource(uint32_t options, OSObject *owner, Action action = NULL);

#ifdef __BLOCKS__
                              
                                                                   
                                                                
                                                                               
                                                                      
   
	static OSPtr<IOTimerEventSource>
	timerEventSource(uint32_t options, OSObject *inOwner, ActionBlock action);
#endif                 

#if XNU_KERNEL_PRIVATE
	__inline__ void invokeAction(IOEventSource::Action action, IOTimerEventSource * ts,
	    OSObject * owner, IOWorkLoop * workLoop);
#endif                         

                  
                                                                                                   
   
	virtual bool init(OSObject *owner, Action action = NULL);

                    
                                            
                                                                                                                                                                                                                                                                 
	virtual void enable() APPLE_KEXT_OVERRIDE;

                     
                                       
                                                                                                                
	virtual void disable() APPLE_KEXT_OVERRIDE;

                          
                                                                                          
                                                                                                                                    
                                                                                                                            
	virtual bool checkForWork() APPLE_KEXT_OVERRIDE;

                             
                                                                                                     
                                                                                      
                                                                                                            
	virtual IOReturn setTimeoutTicks(UInt32 ticks);

                          
                                                                                                  
                                                               
                                                                                                            
	virtual IOReturn setTimeoutMS(UInt32 ms);

                          
                                                                                                      
                                                               
                                                                                                            
	virtual IOReturn setTimeoutUS(UInt32 us);

                        
                                                                                               
                                                                    
                                                                             
                                                                                                            
	virtual IOReturn setTimeout(UInt32 interval,
	    UInt32 scale_factor = kNanosecondScale);

#if !defined(__LP64__)
	virtual IOReturn setTimeout(mach_timespec_t interval)
	APPLE_KEXT_DEPRECATED;
#endif

                        
                                                                                                       
                                                                    
                                                                                                            
	virtual IOReturn setTimeout(AbsoluteTime interval);

                             
                                                                                     
                                                                          
                                                                                                            
	virtual IOReturn wakeAtTimeTicks(UInt32 ticks);

                          
                                                                                     
                                               
                                                                                                            
	virtual IOReturn wakeAtTimeMS(UInt32 ms);

                          
                                                                                     
                                               
                                                                                                            
	virtual IOReturn wakeAtTimeUS(UInt32 us);

                        
                                                                                     
                                                 
                                                                            
                                                                                                            
	virtual IOReturn wakeAtTime(UInt32 abstime,
	    UInt32 scale_factor = kNanosecondScale);

#if !defined(__LP64__)
	virtual IOReturn wakeAtTime(mach_timespec_t abstime)
	APPLE_KEXT_DEPRECATED;
#endif

                        
                                                      
                                                                                                                                                                                                                                                            
                                                                                                                       
                                                                                                                                                      
	virtual IOReturn wakeAtTime(AbsoluteTime abstime);

                           
                                                                  
                                                                                                                                                    
	virtual void cancelTimeout();

                  
                                                                                                   
   
	virtual bool init(uint32_t options, OSObject *inOwner, Action inAction);

                        
                                                                                                       
                                                                             
                                                                    
                                                                                               
                                                                                                            
	virtual IOReturn setTimeout(uint32_t options, AbsoluteTime interval, AbsoluteTime leeway);

                        
                                                      
                                                                                                                                                                                                                                                            
                                                                             
                                                                                                                       
                                                                                               
                                                                                                                                                      
	virtual IOReturn wakeAtTime(uint32_t options, AbsoluteTime abstime, AbsoluteTime leeway);

private:
	static void timeoutAndRelease(void *self, void *c);
	static void timeoutSignaled(void *self, void *c);

private:
	OSMetaClassDeclareReservedUsedX86(IOTimerEventSource, 0);
	OSMetaClassDeclareReservedUsedX86(IOTimerEventSource, 1);
	OSMetaClassDeclareReservedUsedX86(IOTimerEventSource, 2);
	OSMetaClassDeclareReservedUnused(IOTimerEventSource, 3);
	OSMetaClassDeclareReservedUnused(IOTimerEventSource, 4);
	OSMetaClassDeclareReservedUnused(IOTimerEventSource, 5);
	OSMetaClassDeclareReservedUnused(IOTimerEventSource, 6);
	OSMetaClassDeclareReservedUnused(IOTimerEventSource, 7);
};

#endif                           
