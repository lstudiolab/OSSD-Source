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

#ifndef _IOKIT_IOSHAREDDATAQUEUE_H
#define _IOKIT_IOSHAREDDATAQUEUE_H

#define DISABLE_DATAQUEUE_WARNING                                                                      

#include <IOKit/IODataQueue.h>
#include <libkern/c++/OSPtr.h>

#undef DISABLE_DATAQUEUE_WARNING

typedef struct _IODataQueueEntry IODataQueueEntry;

   
                                                
                                                                                                                                
                                                                                                                                                                                                                                                                                                                                                                                                                                          
  
                                                                                                                                                          
  
                                                                                                                                                                                                                                 
  
                                                                                                                                                                                                                                                                                                                                                                                     
   
class IOSharedDataQueue : public IODataQueue
{
	OSDeclareDefaultStructors(IOSharedDataQueue);

	struct ExpansionData {
		UInt32 queueSize;
	};
                 
                                                      
	ExpansionData * _reserved;

protected:
	virtual void free() APPLE_KEXT_OVERRIDE;

   
                         
                                                
                                                                                                                                                                         
                                                                          
   
	UInt32 getQueueSize();

   
                         
                                                            
                                                                                                                                                                                     
                                          
                                                            
   
	Boolean setQueueSize(UInt32 size);

public:
   
                         
                                                                                                                           
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                        
                                                                                                
   
	static OSPtr<IOSharedDataQueue> withCapacity(UInt32 size);

   
                        
                                                                                                                                
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
                                                             
                                       
                                                                                                 
   
	static OSPtr<IOSharedDataQueue> withEntries(UInt32 numEntries, UInt32 entrySize);

   
                             
                                                                                                         
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
                                                        
                                                        
   
	virtual Boolean initWithCapacity(UInt32 size) APPLE_KEXT_OVERRIDE;

   
                                
                                                                               
                                                                                                                                                                                            
                                                                                                                   
   
	virtual OSPtr<IOMemoryDescriptor> getMemoryDescriptor() APPLE_KEXT_OVERRIDE;

   
                 
                                                         
                                                                                                                                                                                                                                                                                                                                    
                                                                                                                           
   
	virtual IODataQueueEntry * peek();

   
                    
                                                                                                      
                                                                                                                                                                                                                                                                                                                                    
                                                                                                                                                                            
                                                                                                                                                                             
                                                                                                          
   
	virtual Boolean dequeue(void *data, UInt32 *dataSize);

   
                    
                                               
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                            
                                                       
                                                                                                         
   
	virtual Boolean enqueue(void *data, UInt32 dataSize) APPLE_KEXT_OVERRIDE;

#ifdef PRIVATE
                                                        
	__inline__ Boolean
	enqueue_tail(void *data, UInt32 dataSize)
	{
		return IOSharedDataQueue::enqueue(data, dataSize);
	}
#endif

#if APPLE_KEXT_VTABLE_PADDING
	OSMetaClassDeclareReservedUnused(IOSharedDataQueue, 0);
	OSMetaClassDeclareReservedUnused(IOSharedDataQueue, 1);
	OSMetaClassDeclareReservedUnused(IOSharedDataQueue, 2);
	OSMetaClassDeclareReservedUnused(IOSharedDataQueue, 3);
	OSMetaClassDeclareReservedUnused(IOSharedDataQueue, 4);
	OSMetaClassDeclareReservedUnused(IOSharedDataQueue, 5);
	OSMetaClassDeclareReservedUnused(IOSharedDataQueue, 6);
	OSMetaClassDeclareReservedUnused(IOSharedDataQueue, 7);
#endif
};

#endif                                 
