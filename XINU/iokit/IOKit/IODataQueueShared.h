/*
 * Copyright (c) 1998-2000 Apple Computer, Inc. All rights reserved.
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

#ifndef _IOKIT_IODATAQUEUESHARED_H
#define _IOKIT_IODATAQUEUESHARED_H

#include <libkern/OSTypes.h>
#include <mach/port.h>
#include <mach/message.h>

   
                            
                                                      
                                                                                                                                                                                                                                                                         
                                                     
                                                                                                                                        
   
typedef struct _IODataQueueEntry {
	UInt32  size;
	UInt8   data[4];
} IODataQueueEntry;

   
                             
                                                                   
                                                                                                                                                                                                                                                                                                                                    
                                                                               
                                                                                                                                         
                                                                                                                                         
                                                                                                                                                  
   
typedef struct _IODataQueueMemory {
	UInt32            queueSize;
	volatile UInt32   head;
	volatile UInt32   tail;
	IODataQueueEntry  queue[1];
} IODataQueueMemory;

   
                               
                                                                     
                                                                                                                                  
                                                    
                                                                                                    
   
typedef struct _IODataQueueAppendix {
	UInt32            version;
	mach_msg_header_t msgh;
} IODataQueueAppendix;

   
                                                                                                                                                                                                                                                                                                                               
   
#define DATA_QUEUE_ENTRY_HEADER_SIZE (sizeof(IODataQueueEntry) - 4)

   
                                                                                                                                                                                                                                                                                                                                                       
   
#define DATA_QUEUE_MEMORY_HEADER_SIZE (sizeof(IODataQueueMemory) - sizeof(IODataQueueEntry))

   
                                                                                                                                                                                                                                                                                                                                                 
   
#define DATA_QUEUE_MEMORY_APPENDIX_SIZE (sizeof(IODataQueueAppendix))

#endif                                 
