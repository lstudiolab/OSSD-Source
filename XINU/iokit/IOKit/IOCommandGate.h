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
   
                                           
                 
       
                             

#ifndef _IOKIT_IOCOMMANDGATE_H
#define _IOKIT_IOCOMMANDGATE_H

#include <IOKit/IOEventSource.h>
#include <libkern/c++/OSPtr.h>

   
                                                
                                                                  
                                                                                
                                                                                  
                                                                                     
                                                                                   
                                                                                 
                                                                                    
                                                     
            
                                                                                
                                                                                    
                                                                                   
                                                                               
                                                                                   
                                                                
            
                                                                                                                 
  
   
class IOCommandGate : public IOEventSource
{
	OSDeclareDefaultStructors(IOCommandGate);

public:
   
                    
                                                                           
                                                                                
                                                                                  
                                                      
                 
                                                                           
                                                                                
                                                                               
                                     
                                                       
                                                       
                                                       
                                                       
   
	typedef IOReturn (*Action)(OSObject *owner,
	    void *arg0, void *arg1,
	    void *arg2, void *arg3);

protected:

                         
                                                                                                        
   
	struct ExpansionData { };

                 
                                                      
	APPLE_KEXT_WSHADOW_PUSH;
	ExpansionData *reserved;
	APPLE_KEXT_WSHADOW_POP;

public:
                         
                                                                                        
                                                                                    
	static OSPtr<IOCommandGate> commandGate(OSObject *owner, Action action = NULL);

                  
                                 
                                                                             
                                                                
                                                                                                                                                              
                  
                                                                        
                                                                               
                                                                                    
                                                                            
                                                                  
	virtual bool init(OSObject *owner, Action action = NULL);

                       
	virtual void free() APPLE_KEXT_OVERRIDE;
	virtual void setWorkLoop(IOWorkLoop *inWorkLoop) APPLE_KEXT_OVERRIDE;

                        
                                                                 
                                                                               
                                                                                   
                                                                            
                                                                              
                                                                                
                                                                                                                             
                                                                     
                                                                     
                                                                     
                                                                     
                                                                                                                                                                         
   
	virtual IOReturn runCommand(void *arg0 = NULL, void *arg1 = NULL,
	    void *arg2 = NULL, void *arg3 = NULL);

                       
                                                                           
                                                                             
                                                                                   
                                                                            
                                                                             
                                                                                
                                                                                                                             
                                                                                      
                                                               
                                                               
                                                               
                                                               
                                                                                                                                                                                        
   
	virtual IOReturn runAction(Action action,
	    void *arg0 = NULL, void *arg1 = NULL,
	    void *arg2 = NULL, void *arg3 = NULL);

#ifdef __BLOCKS__
                            
                                                                           
                                                                             
                                                                                   
                                                                            
                                                                             
                                                                                
                                                                                                                             
                                                                        
                                                                                                                                                                                        
   
	IOReturn runActionBlock(ActionBlock action);
#endif                 

                            
                                                                 
                                                                               
                                                                                                                                  
                                                                     
                                                                     
                                                                     
                                                                     
                                                                                                                                                                                                      
   
	virtual IOReturn attemptCommand(void *arg0 = NULL, void *arg1 = NULL,
	    void *arg2 = NULL, void *arg3 = NULL);

                           
                                                                           
                                                                             
                                                                                   
                                                                            
                                                                               
                                                                                  
                                                               
                                                               
                                                               
                                                               
                                                                                                                                                                                                        
  
   
	virtual IOReturn attemptAction(Action action,
	    void *arg0 = NULL, void *arg1 = NULL,
	    void *arg2 = NULL, void *arg3 = NULL);

                          
                                                                                
                                                                                                                                                                                                                     
                                        
                                                                                                                                                                                                                                                                                                                                                          
                                                                                                                                                                   
	virtual IOReturn commandSleep(void *event,
	    UInt32 interruptible = THREAD_ABORTSAFE);

                           
                                                                      
                                        
                                                                                  
	virtual void commandWakeup(void *event, bool oneThread = false);

                     
                                       
                                                                                                                                                                                                                                                                                                                                                                                                          
	virtual void disable() APPLE_KEXT_OVERRIDE;

                    
                                                                                       
                                                                                                                                                                                 
	virtual void enable() APPLE_KEXT_OVERRIDE;

                          
                                                                                
                                                                                                                                                                                                                                        
                                        
                                                             
                                                                                                                                                                                                                                                                                                                                      
                                                                                                                                                                   
	virtual IOReturn commandSleep(void *event,
	    AbsoluteTime deadline,
	    UInt32 interruptible);

private:
#if __LP64__
	OSMetaClassDeclareReservedUnused(IOCommandGate, 0);
#else
	OSMetaClassDeclareReservedUsedX86(IOCommandGate, 0);
#endif
	OSMetaClassDeclareReservedUnused(IOCommandGate, 1);
	OSMetaClassDeclareReservedUnused(IOCommandGate, 2);
	OSMetaClassDeclareReservedUnused(IOCommandGate, 3);
	OSMetaClassDeclareReservedUnused(IOCommandGate, 4);
	OSMetaClassDeclareReservedUnused(IOCommandGate, 5);
	OSMetaClassDeclareReservedUnused(IOCommandGate, 6);
	OSMetaClassDeclareReservedUnused(IOCommandGate, 7);
};

#endif                              
