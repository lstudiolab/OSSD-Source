/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
 *	10/9/2000	CJS	Created IOCommandPool class and implementation
 *
 */

#define IOKIT_ENABLE_SHARED_PTR

#include <IOKit/IOCommandPool.h>
#include <libkern/c++/OSSharedPtr.h>

#define super OSObject
OSDefineMetaClassAndStructors(IOCommandPool, OSObject);
OSMetaClassDefineReservedUnused(IOCommandPool, 0);
OSMetaClassDefineReservedUnused(IOCommandPool, 1);
OSMetaClassDefineReservedUnused(IOCommandPool, 2);
OSMetaClassDefineReservedUnused(IOCommandPool, 3);
OSMetaClassDefineReservedUnused(IOCommandPool, 4);
OSMetaClassDefineReservedUnused(IOCommandPool, 5);
OSMetaClassDefineReservedUnused(IOCommandPool, 6);
OSMetaClassDefineReservedUnused(IOCommandPool, 7);

                                                                            
                                                        
                                                                            

OSSharedPtr<IOCommandPool>
IOCommandPool::
withWorkLoop(IOWorkLoop *inWorkLoop)
{
	OSSharedPtr<IOCommandPool> me = OSMakeShared<IOCommandPool>();

	if (me && !me->initWithWorkLoop(inWorkLoop)) {
		return nullptr;
	}

	return me;
}


bool
IOCommandPool::
initWithWorkLoop(IOWorkLoop *inWorkLoop)
{
	assert(inWorkLoop);

	if (!super::init()) {
		return false;
	}

	queue_init(&fQueueHead);

	fSerializer = IOCommandGate::commandGate(this);
	assert(fSerializer);
	if (!fSerializer) {
		return false;
	}

	if (kIOReturnSuccess != inWorkLoop->addEventSource(fSerializer.get())) {
		return false;
	}

	return true;
}

                                                                            
                                                               
                                                                            

OSSharedPtr<IOCommandPool>
IOCommandPool::
commandPool(IOService * inOwner, IOWorkLoop *inWorkLoop, UInt32 inSize)
{
	OSSharedPtr<IOCommandPool> me = OSMakeShared<IOCommandPool>();

	if (me && !me->init(inOwner, inWorkLoop, inSize)) {
		return nullptr;
	}

	return me;
}

bool
IOCommandPool::
init(IOService *             , IOWorkLoop *inWorkLoop, UInt32             )
{
	return initWithWorkLoop(inWorkLoop);
}


                                                                            
                                      
                                                                            

void
IOCommandPool::free(void)
{
	if (fSerializer) {
		                                                
		IOWorkLoop *wl = fSerializer->getWorkLoop();
		if (wl) {
			wl->removeEventSource(fSerializer.get());
		}

		fSerializer.reset();
	}

	                                     
	super::free();
}


                                                                            
                                                          
                                                     
                          
                                                                            

OSSharedPtr<IOCommand>
IOCommandPool::getCommand(bool blockForCommand)
{
	IOReturn     result  = kIOReturnSuccess;
	IOCommand *command = NULL;

	IOCommandGate::Action func = OSMemberFunctionCast(
		IOCommandGate::Action, this, &IOCommandPool::gatedGetCommand);
	result = fSerializer->
	    runAction(func, (void *) &command, (void *) blockForCommand);
	if (kIOReturnSuccess == result) {
		return OSSharedPtr<IOCommand>(command, OSNoRetain);
	} else {
		return NULL;
	}
}


                                                                            
                                                
                                    
                                                                            

IOReturn
IOCommandPool::
gatedGetCommand(IOCommand **command, bool blockForCommand)
{
	while (queue_empty(&fQueueHead)) {
		if (!blockForCommand) {
			return kIOReturnNoResources;
		}

		fSleepers++;
		fSerializer->commandSleep(&fSleepers, THREAD_UNINT);
	}

	queue_remove_first(&fQueueHead,
	    *command, IOCommand *, fCommandChain);
	return kIOReturnSuccess;
}


                                                                            
                                                
                                                                            

void
IOCommandPool::
returnCommand(IOCommand *command)
{
	IOCommandGate::Action func = OSMemberFunctionCast(
		IOCommandGate::Action, this, &IOCommandPool::gatedReturnCommand);
	(void) fSerializer->runAction(func, (void *) command);
}


                                                                            
                                            
                                                              
                                                                            

IOReturn
IOCommandPool::
gatedReturnCommand(IOCommand *command)
{
	queue_enter_first(&fQueueHead, command, IOCommand *, fCommandChain);
	if (fSleepers) {
		fSerializer->commandWakeup(&fSleepers,                 true);
		fSleepers--;
	}
	return kIOReturnSuccess;
}
