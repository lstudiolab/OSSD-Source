/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
 *
 *	Copyright (c) 2000 Apple Computer, Inc.  All rights reserved.
 *
 *	HISTORY
 *
 *	2001-01-17	gvdl	Re-implement on IOCommandGate::commandSleep
 *	11/13/2000	CJS	Created IOCommandPool class and implementation
 *
 */

   
                        
            
                                                           
   

#ifndef _IOKIT_IO_COMMAND_POOL_H_
#define _IOKIT_IO_COMMAND_POOL_H_

  
         
   

#if defined(KERNEL) && defined(__cplusplus)

#include <kern/kern_types.h>
#include <kern/queue.h>
#include <IOKit/IOCommand.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOService.h>
#include <IOKit/IOWorkLoop.h>
#include <libkern/c++/OSPtr.h>

   
                       
                                                                         
              
                                                                         
                                                                        
                                                                       
                                                                            
                                                                           
                                                                             
            
   

class IOCommandPool : public OSObject
{
	OSDeclareDefaultStructors(IOCommandPool);


protected:

	queue_head_t fQueueHead;                                              
	UInt32 fSleepers;                                                   
	OSPtr<IOCommandGate> fSerializer;                                                    

                         
                                                                                                           
   
	struct ExpansionData { };

                 
                                                      
	ExpansionData *reserved;

   
                                   
                                                  
              
                                                                     
                                                                    
                      
   

	static const UInt32 kIOCommandPoolDefaultSize = 2;

  
                                                   
   

	virtual void free(void) APPLE_KEXT_OVERRIDE;


public:

   
                             
                                                             
                                                        
                                                                            
                  
                                                               
                                                                     
   
	virtual bool initWithWorkLoop(IOWorkLoop *workLoop);

   
                         
                                                               
              
                                                                           
                                                                           
                    
                                                               
          
                                                                   
                  
   

	static OSPtr<IOCommandPool> withWorkLoop(IOWorkLoop *inWorkLoop);

   
                 
                                                                  
   
	virtual bool init(IOService *inOwner,
	    IOWorkLoop *inWorkLoop,
	    UInt32 inSize = kIOCommandPoolDefaultSize);

   
                         
                                                                             
   
	static OSPtr<IOCommandPool> commandPool(IOService *inOwner,
	    IOWorkLoop *inWorkLoop,
	    UInt32 inSize = kIOCommandPoolDefaultSize);


   
                       
                                                                                                           
                         
                                                                       
                                              
          
                                                                           
                                                                        
                                                                               
                        
   

	virtual OSPtr<IOCommand> getCommand(
		bool blockForCommand = true);

   
                          
              
                                                                        
                                                                   
                 
                                    
   

	virtual void returnCommand(LIBKERN_CONSUMED IOCommand *command);

protected:

   
                            
              
                                                                      
                                                                           
                 
                                                                   
                          
                         
                                                                     
                     
          
                                                                         
                                                         
                                                      
   
	virtual IOReturn gatedGetCommand(
		LIBKERN_RETURNS_NOT_RETAINED IOCommand **command, bool blockForCommand);

   
                               
              
                                                                     
                                                                            
                 
                                                                
          
                                                                     
   
	virtual IOReturn gatedReturnCommand(IOCommand *command);

private:
	OSMetaClassDeclareReservedUnused(IOCommandPool, 0);
	OSMetaClassDeclareReservedUnused(IOCommandPool, 1);
	OSMetaClassDeclareReservedUnused(IOCommandPool, 2);
	OSMetaClassDeclareReservedUnused(IOCommandPool, 3);
	OSMetaClassDeclareReservedUnused(IOCommandPool, 4);
	OSMetaClassDeclareReservedUnused(IOCommandPool, 5);
	OSMetaClassDeclareReservedUnused(IOCommandPool, 6);
	OSMetaClassDeclareReservedUnused(IOCommandPool, 7);
};

#endif                                               

#endif                                 
