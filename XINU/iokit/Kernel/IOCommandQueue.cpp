/*
 * Copyright (c) 1998-2010 Apple Inc. All rights reserved.
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

#define IOKIT_ENABLE_SHARED_PTR

#if !defined(__LP64__)

#include <IOKit/IOCommandQueue.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOTimeStamp.h>
#include <IOKit/IOKitDebug.h>
#include <libkern/c++/OSSharedPtr.h>

#include <mach/sync_policy.h>

#if IOKITSTATS

#define IOStatisticsInitializeCounter() \
	IOStatistics::setCounterType(reserved->counter, kIOStatisticsCommandQueueCounter)

#define IOStatisticsActionCall() \
	IOStatistics::countCommandQueueActionCall(reserved->counter)

#else

#define IOStatisticsInitializeCounter()
#define IOStatisticsActionCall()

#endif                 

#define NUM_FIELDS_IN_COMMAND   4
typedef struct commandEntryTag {
	void *f[NUM_FIELDS_IN_COMMAND];
} commandEntryT;

#define super IOEventSource

OSDefineMetaClassAndStructors(IOCommandQueue, IOEventSource)

   
                    
  
                                   
                                             
                                
                                   
                                  
  
                                                                 
                                                                     
                                                                            
                                      
  
                                                                           
                                       
  
                                                       
       
bool
IOCommandQueue::init(OSObject *inOwner,
    IOCommandQueueAction inAction,
    int inSize)
{
	if (!super::init(inOwner, (IOEventSourceAction) inAction)) {
		return false;
	}

	if (KERN_SUCCESS
	    != semaphore_create(kernel_task, &producerSema, SYNC_POLICY_FIFO, inSize)) {
		return false;
	}

	size = inSize + 1;                                          

	queue = (void *)kalloc(size * sizeof(commandEntryT));
	if (!queue) {
		return false;
	}

	producerLock = IOLockAlloc();
	if (!producerLock) {
		return false;
	}

	producerIndex = consumerIndex = 0;

	IOStatisticsInitializeCounter();

	return true;
}

OSSharedPtr<IOCommandQueue>
IOCommandQueue::commandQueue(OSObject *inOwner,
    IOCommandQueueAction inAction,
    int inSize)
{
	OSSharedPtr<IOCommandQueue> me = OSMakeShared<IOCommandQueue>();

	if (me && !me->init(inOwner, inAction, inSize)) {
		me.reset();
		return nullptr;
	}

	return me;
}

   
        
           
  
                                                                         
                
       
void
IOCommandQueue::free()
{
	if (queue) {
		kfree(queue, size * sizeof(commandEntryT));
	}
	if (producerSema) {
		semaphore_destroy(kernel_task, producerSema);
	}
	if (producerLock) {
		IOLockFree(producerLock);
	}

	super::free();
}

#if NUM_FIELDS_IN_COMMAND != 4
#error IOCommandQueue::checkForWork needs to be updated for new command size
#endif

bool
IOCommandQueue::checkForWork()
{
	void        *field0, *field1, *field2, *field3;
	bool    trace = (gIOKitTrace & kIOTraceCommandGates) ? true : false;

	if (!enabled || consumerIndex == producerIndex) {
		return false;
	}

	{
		commandEntryT *q = (commandEntryT *) queue;
		int localIndex = consumerIndex;

		field0 = q[localIndex].f[0]; field1 = q[localIndex].f[1];
		field2 = q[localIndex].f[2]; field3 = q[localIndex].f[3];
		semaphore_signal(producerSema);
	}

	if (++consumerIndex >= size) {
		consumerIndex = 0;
	}

	if (trace) {
		IOTimeStampStartConstant(IODBG_CMDQ(IOCMDQ_ACTION),
		    VM_KERNEL_ADDRHIDE(action), VM_KERNEL_ADDRHIDE(owner));
	}

	IOStatisticsActionCall();
	(*(IOCommandQueueAction) action)(owner, field0, field1, field2, field3);

	if (trace) {
		IOTimeStampEndConstant(IODBG_CMDQ(IOCMDQ_ACTION),
		    VM_KERNEL_ADDRHIDE(action), VM_KERNEL_ADDRHIDE(owner));
	}

	return consumerIndex != producerIndex;
}

   
                         
                                                        
                                                                               
                                                                                
  
                                                                         
                                                                           
                                                                        
  
                                                                           
                                                                            
                                                                      
  
                                               
       
#if NUM_FIELDS_IN_COMMAND != 4
#error IOCommandQueue::enqueueCommand needs to be updated
#endif

kern_return_t
IOCommandQueue::enqueueCommand(bool gotoSleep,
    void *field0, void *field1,
    void *field2, void *field3)
{
	kern_return_t rtn = KERN_SUCCESS;
	int retry;

	                                                                     

	if (gotoSleep) {
		retry = 0;
		do{
			rtn = semaphore_wait(producerSema);
		} while ((KERN_SUCCESS != rtn)
		    && (KERN_OPERATION_TIMED_OUT != rtn)
		    && (KERN_SEMAPHORE_DESTROYED != rtn)
		    && (KERN_TERMINATED != rtn)
		    && ((retry++) < 4));
	} else {
		rtn = semaphore_timedwait(producerSema, MACH_TIMESPEC_ZERO);
	}

	if (KERN_SUCCESS != rtn) {
		return rtn;
	}

	                           
	IOTakeLock(producerLock);

	  
                                                                 
                                                                    
                                                                 
    
	{
		commandEntryT *q = (commandEntryT *) queue;
		int localIndex = producerIndex;

		q[localIndex].f[0] = field0; q[localIndex].f[1] = field1;
		q[localIndex].f[2] = field2; q[localIndex].f[3] = field3;
	}
	if (++producerIndex >= size) {
		producerIndex = 0;
	}

	                                              
	IOUnlock(producerLock);

	  
                                                                     
                                                              
    
	signalWorkAvailable();
	return rtn;
}

int
IOCommandQueue::performAndFlush(OSObject *target,
    IOCommandQueueAction inAction)
{
	int numEntries;
	kern_return_t rtn;

	                                
	if (!target) {
		target = owner;
	}
	if (!inAction) {
		inAction = (IOCommandQueueAction) action;
	}

	                               
	do {
		rtn = semaphore_timedwait(producerSema, MACH_TIMESPEC_ZERO);
	} while (rtn == KERN_SUCCESS);

	                                                           
	for (numEntries = 0; consumerIndex != producerIndex;) {
		void *field0, *field1, *field2, *field3;

		{
			commandEntryT *q = (commandEntryT *) queue;
			int localIndex = consumerIndex;

			field0 = q[localIndex].f[0]; field1 = q[localIndex].f[1];
			field2 = q[localIndex].f[2]; field3 = q[localIndex].f[3];
		}

		if (++consumerIndex >= size) {
			consumerIndex = 0;
		}

		(*inAction)(target, field0, field1, field2, field3);
	}

	                                                    
	for (int i = 1; i < size; i++) {
		semaphore_signal(producerSema);
	}

	return numEntries;
}

#endif                         
