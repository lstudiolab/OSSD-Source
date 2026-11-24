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

#ifndef _IOKIT_IODATAQUEUE_H
#define _IOKIT_IODATAQUEUE_H

#ifndef DISABLE_DATAQUEUE_WARNING
#warning "IODataQueue is deprecated due to security issues in its interfaces, please use IOSharedDataQueue instead"
#endif

#include <libkern/c++/OSPtr.h>
#include <libkern/c++/OSObject.h>
#include <libkern/OSTypes.h>
#include <mach/port.h>
#include <mach/message.h>

typedef struct _IODataQueueMemory IODataQueueMemory;
class IOMemoryDescriptor;

struct _notifyMsg {
	mach_msg_header_t h;
};


#ifdef dequeue
#undef dequeue
#endif
#ifdef enqueue
#undef enqueue
#endif

   
                                       
                                                                                     
                                                                                                                                                                                                                                                                                                                                                                                                                         
  
                                                                                                                                                          
  
                                                                                                                                                                                                                                 
  
                                                                                                                                                                                        
  
                                                                                                                                                                                                                                                                                                                                                                                     
   
#ifndef DISABLE_DATAQUEUE_WARNING
class __attribute__((deprecated)) IODataQueue: public OSObject
#else
class IODataQueue : public OSObject
#endif
{
	OSDeclareDefaultStructors(IODataQueue);

protected:
	IODataQueueMemory * dataQueue;

	void *              notifyMsg;

	virtual void free() APPLE_KEXT_OVERRIDE;

   
                                          
                                                                                  
                                                                                                                                                         
   
	virtual void sendDataAvailableNotification();

public:
   
                         
                                                                                                                     
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                        
                                                                                          
   
	static OSPtr<IODataQueue> withCapacity(UInt32 size);

   
                        
                                                                                                                          
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
                                                             
                                       
                                                                                          
   
	static OSPtr<IODataQueue> withEntries(UInt32 numEntries, UInt32 entrySize);

   
                             
                                                                                                   
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
                                                        
                                                        
   
	virtual Boolean initWithCapacity(UInt32 size);

   
                            
                                                                                                        
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
                                                             
                                       
                                                        
   
	virtual Boolean initWithEntries(UInt32 numEntries, UInt32 entrySize);

   
                    
                                               
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                            
                                                       
                                                                                                         
   
	virtual Boolean enqueue(void *data, UInt32 dataSize);

   
                                
                                                                                     
                                                                                                                                             
                                                                     
   
	virtual void setNotificationPort(mach_port_t port);

   
                                
                                                                               
                                                                                                                                                                                            
                                                                                                                   
   
	virtual OSPtr<IOMemoryDescriptor> getMemoryDescriptor();
};

#endif                           
