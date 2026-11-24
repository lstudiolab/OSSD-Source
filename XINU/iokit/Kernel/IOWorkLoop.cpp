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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <pexpert/pexpert.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOEventSource.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOCommandPool.h>
#include <IOKit/IOTimeStamp.h>
#include <IOKit/IOKitDebug.h>
#include <libkern/OSDebug.h>
#include <kern/thread.h>

#define super OSObject

OSDefineMetaClassAndStructors(IOWorkLoop, OSObject);

                                                    
#if __LP64__
OSMetaClassDefineReservedUnused(IOWorkLoop, 0);
OSMetaClassDefineReservedUnused(IOWorkLoop, 1);
OSMetaClassDefineReservedUnused(IOWorkLoop, 2);
#else
OSMetaClassDefineReservedUsedX86(IOWorkLoop, 0);
OSMetaClassDefineReservedUsedX86(IOWorkLoop, 1);
OSMetaClassDefineReservedUsedX86(IOWorkLoop, 2);
#endif
OSMetaClassDefineReservedUnused(IOWorkLoop, 3);
OSMetaClassDefineReservedUnused(IOWorkLoop, 4);
OSMetaClassDefineReservedUnused(IOWorkLoop, 5);
OSMetaClassDefineReservedUnused(IOWorkLoop, 6);
OSMetaClassDefineReservedUnused(IOWorkLoop, 7);

enum IOWorkLoopState { kLoopRestart = 0x1, kLoopTerminate = 0x2 };
static inline void
SETP(void *addr, unsigned int flag)
{
	unsigned char *num = (unsigned char *) addr; *num |= flag;
}
static inline void
CLRP(void *addr, unsigned int flag)
{
	unsigned char *num = (unsigned char *) addr; *num &= ~flag;
}
static inline bool
ISSETP(void *addr, unsigned int flag)
{
	unsigned char *num = (unsigned char *) addr; return (*num & flag) != 0;
}

#define fFlags loopRestart

#define passiveEventChain       reserved->passiveEventChain

#if IOKITSTATS

#define IOStatisticsRegisterCounter() \
do { \
	reserved->counter = IOStatistics::registerWorkLoop(this); \
} while(0)

#define IOStatisticsUnregisterCounter() \
do { \
	if (reserved) \
	        IOStatistics::unregisterWorkLoop(reserved->counter); \
} while(0)

#define IOStatisticsOpenGate() \
do { \
	IOStatistics::countWorkLoopOpenGate(reserved->counter); \
	if (reserved->lockInterval) lockTime();                 \
} while(0)
#define IOStatisticsCloseGate() \
do { \
	IOStatistics::countWorkLoopCloseGate(reserved->counter);                    \
	if (reserved->lockInterval) reserved->lockTime = mach_absolute_time();      \
} while(0)

#define IOStatisticsAttachEventSource() \
do { \
	IOStatistics::attachWorkLoopEventSource(reserved->counter, inEvent->reserved->counter); \
} while(0)

#define IOStatisticsDetachEventSource() \
do { \
	IOStatistics::detachWorkLoopEventSource(reserved->counter, inEvent->reserved->counter); \
} while(0)

#else

#define IOStatisticsRegisterCounter()
#define IOStatisticsUnregisterCounter()
#define IOStatisticsOpenGate()
#define IOStatisticsCloseGate()
#define IOStatisticsAttachEventSource()
#define IOStatisticsDetachEventSource()

#endif                 

bool
IOWorkLoop::init()
{
	                                                             
	if (!super::init()) {
		return false;
	}

	                                                                  
	if (!reserved) {
		reserved = IONew(ExpansionData, 1);
		if (!reserved) {
			return false;
		}

		bzero(reserved, sizeof(ExpansionData));
	}

	if (gateLock == NULL) {
		if (!(gateLock = IORecursiveLockAlloc())) {
			return false;
		}
	}

	if (workToDoLock == NULL) {
		if (!(workToDoLock = IOSimpleLockAlloc())) {
			return false;
		}
		IOSimpleLockInit(workToDoLock);
		workToDo = false;
	}

	IOStatisticsRegisterCounter();

	if (controlG == NULL) {
		controlG = IOCommandGate::commandGate(
			this,
			OSMemberFunctionCast(
				IOCommandGate::Action,
				this,
				&IOWorkLoop::_maintRequest));

		if (!controlG) {
			return false;
		}
		                                                                 
		                                                                       
		                                                       
		controlG->setWorkLoop(this);
		if (addEventSource(controlG) != kIOReturnSuccess) {
			return false;
		}
	}

	if (workThread == NULL) {
		thread_continue_t cptr = OSMemberFunctionCast(
			thread_continue_t,
			this,
			&IOWorkLoop::threadMain);
		if (KERN_SUCCESS != kernel_thread_start(cptr, this, &workThread)) {
			return false;
		}
	}

	(void) thread_set_tag(workThread, THREAD_TAG_IOWORKLOOP);
	return true;
}

IOWorkLoop *
IOWorkLoop::workLoop()
{
	return IOWorkLoop::workLoopWithOptions(0);
}

IOWorkLoop *
IOWorkLoop::workLoopWithOptions(IOOptionBits options)
{
	IOWorkLoop *me = new IOWorkLoop;

	if (me && options) {
		me->reserved = IONew(ExpansionData, 1);
		if (!me->reserved) {
			me->release();
			return NULL;
		}
		bzero(me->reserved, sizeof(ExpansionData));
		me->reserved->options = options;
	}

	if (me && !me->init()) {
		me->release();
		return NULL;
	}

	return me;
}

void
IOWorkLoop::releaseEventChain(LIBKERN_CONSUMED IOEventSource *eventChain)
{
	IOEventSource *event, *next;
	for (event = eventChain; event; event = next) {
		next = event->getNext();
#ifdef __clang_analyzer__
		                                                                  
		                                                                  
		                                                                      
		                                                                    
		                                                          
		                                                                  
		                                                                          
		if (next) {
			next->retain();
		}
#endif
		event->setWorkLoop(NULL);
		event->setNext(NULL);
		event->release();
	}
}
                        
                                                            
                                                            
                                                          
void
IOWorkLoop::free()
{
	if (workThread) {
		IOInterruptState is;

		                                                                    
		                                                              
		                                                              
		                                                                    
		                                                               
		closeGate();

		disableAllEventSources();

		is = IOSimpleLockLockDisableInterrupt(workToDoLock);
		SETP(&fFlags, kLoopTerminate);
		thread_wakeup_thread((void *) &workToDo, workThread);
		IOSimpleLockUnlockEnableInterrupt(workToDoLock, is);

		openGate();
	} else {                  
		releaseEventChain(eventChain);
		eventChain = NULL;

		releaseEventChain(passiveEventChain);
		passiveEventChain = NULL;

		                                                      
		                                                    
		                                                       

		if (controlG) {
			controlG->workLoop = NULL;
			controlG->release();
			controlG = NULL;
		}

		if (workToDoLock) {
			IOSimpleLockFree(workToDoLock);
			workToDoLock = NULL;
		}

		if (gateLock) {
			IORecursiveLockFree(gateLock);
			gateLock = NULL;
		}

		IOStatisticsUnregisterCounter();

		if (reserved) {
			IODelete(reserved, ExpansionData, 1);
			reserved = NULL;
		}

		super::free();
	}
}

IOReturn
IOWorkLoop::addEventSource(IOEventSource *newEvent)
{
	if ((workThread)
	    && !thread_has_thread_name(workThread)
	    && (newEvent->owner)
	    && !OSDynamicCast(IOCommandPool, newEvent->owner)) {
		thread_set_thread_name(workThread, newEvent->owner->getMetaClass()->getClassName());
	}

	return controlG->runCommand((void *) mAddEvent, (void *) newEvent);
}

IOReturn
IOWorkLoop::removeEventSource(IOEventSource *toRemove)
{
	return controlG->runCommand((void *) mRemoveEvent, (void *) toRemove);
}

void
IOWorkLoop::enableAllEventSources() const
{
	IOEventSource *event;

	for (event = eventChain; event; event = event->getNext()) {
		event->enable();
	}

	for (event = passiveEventChain; event; event = event->getNext()) {
		event->enable();
	}
}

void
IOWorkLoop::disableAllEventSources() const
{
	IOEventSource *event;

	for (event = eventChain; event; event = event->getNext()) {
		event->disable();
	}

	                                                                        
	for (event = passiveEventChain; event; event = event->getNext()) {
		if (event != controlG) {                                  
			event->disable();
		}
	}
}

void
IOWorkLoop::enableAllInterrupts() const
{
	IOEventSource *event;

	for (event = eventChain; event; event = event->getNext()) {
		if (OSDynamicCast(IOInterruptEventSource, event)) {
			event->enable();
		}
	}
}

void
IOWorkLoop::disableAllInterrupts() const
{
	IOEventSource *event;

	for (event = eventChain; event; event = event->getNext()) {
		if (OSDynamicCast(IOInterruptEventSource, event)) {
			event->disable();
		}
	}
}


              bool
IOWorkLoop::runEventSources()
{
	bool res = false;
	bool traceWL = (gIOKitTrace & kIOTraceWorkLoops) ? true : false;
	bool traceES = (gIOKitTrace & kIOTraceEventSources) ? true : false;

	closeGate();
	if (ISSETP(&fFlags, kLoopTerminate)) {
		goto abort;
	}

	if (traceWL) {
		IOTimeStampStartConstant(IODBG_WORKLOOP(IOWL_WORK), VM_KERNEL_ADDRHIDE(this));
	}

	bool more;
	do {
		CLRP(&fFlags, kLoopRestart);
		more = false;
		IOInterruptState is = IOSimpleLockLockDisableInterrupt(workToDoLock);
		workToDo = false;
		IOSimpleLockUnlockEnableInterrupt(workToDoLock, is);
		                                                                                                      
		for (IOEventSource *evnt = eventChain; evnt; evnt = evnt->getNext()) {
			if (traceES) {
				IOTimeStampStartConstant(IODBG_WORKLOOP(IOWL_CLIENT), VM_KERNEL_ADDRHIDE(this), VM_KERNEL_ADDRHIDE(evnt));
			}

			more |= evnt->checkForWork();

			if (traceES) {
				IOTimeStampEndConstant(IODBG_WORKLOOP(IOWL_CLIENT), VM_KERNEL_ADDRHIDE(this), VM_KERNEL_ADDRHIDE(evnt));
			}

			if (ISSETP(&fFlags, kLoopTerminate)) {
				goto abort;
			} else if (fFlags & kLoopRestart) {
				more = true;
				break;
			}
		}
	} while (more);

	res = true;

	if (traceWL) {
		IOTimeStampEndConstant(IODBG_WORKLOOP(IOWL_WORK), VM_KERNEL_ADDRHIDE(this));
	}

abort:
	openGate();
	return res;
}

              void
IOWorkLoop::threadMain()
{
restartThread:
	do {
		if (!runEventSources()) {
			goto exitThread;
		}

		IOInterruptState is = IOSimpleLockLockDisableInterrupt(workToDoLock);
		if (!ISSETP(&fFlags, kLoopTerminate) && !workToDo) {
			assert_wait((void *) &workToDo, false);
			IOSimpleLockUnlockEnableInterrupt(workToDoLock, is);
			thread_continue_t cptr = NULL;
			if (!reserved || !(kPreciousStack & reserved->options)) {
				cptr = OSMemberFunctionCast(
					thread_continue_t, this, &IOWorkLoop::threadMain);
			}
			thread_block_parameter(cptr, this);
			goto restartThread;
			                
		}

		                                                     
		                                    
		                                                       
		IOSimpleLockUnlockEnableInterrupt(workToDoLock, is);
	} while (workToDo);

exitThread:
	closeGate();
	thread_t thread = workThread;
	workThread = NULL;                                               
	openGate();

	free();

	thread_deallocate(thread);
	(void) thread_terminate(thread);
}

IOThread
IOWorkLoop::getThread() const
{
	return workThread;
}

bool
IOWorkLoop::onThread() const
{
	return IOThreadSelf() == workThread;
}

bool
IOWorkLoop::inGate() const
{
	return IORecursiveLockHaveLock(gateLock);
}

                                                            
void
IOWorkLoop::signalWorkAvailable()
{
	if (workToDoLock) {
		IOInterruptState is = IOSimpleLockLockDisableInterrupt(workToDoLock);
		workToDo = true;
		thread_wakeup_thread((void *) &workToDo, workThread);
		IOSimpleLockUnlockEnableInterrupt(workToDoLock, is);
	}
}

void
IOWorkLoop::openGate()
{
	IOStatisticsOpenGate();
	IORecursiveLockUnlock(gateLock);
}

void
IOWorkLoop::closeGate()
{
	IORecursiveLockLock(gateLock);
	IOStatisticsCloseGate();
}

bool
IOWorkLoop::tryCloseGate()
{
	bool res = (IORecursiveLockTryLock(gateLock) != 0);
	if (res) {
		IOStatisticsCloseGate();
	}
	return res;
}

int
IOWorkLoop::sleepGate(void *event, UInt32 interuptibleType)
{
	int res;
	IOStatisticsOpenGate();
	res = IORecursiveLockSleep(gateLock, event, interuptibleType);
	IOStatisticsCloseGate();
	return res;
}

int
IOWorkLoop::sleepGate(void *event, AbsoluteTime deadline, UInt32 interuptibleType)
{
	int res;
	IOStatisticsOpenGate();
	res = IORecursiveLockSleepDeadline(gateLock, event, deadline, interuptibleType);
	IOStatisticsCloseGate();
	return res;
}

void
IOWorkLoop::wakeupGate(void *event, bool oneThread)
{
	IORecursiveLockWakeup(gateLock, event, oneThread);
}

static IOReturn
IOWorkLoopActionToBlock(OSObject *owner,
    void *arg0, void *arg1,
    void *arg2, void *arg3)
{
	return ((IOWorkLoop::ActionBlock) arg0)();
}

IOReturn
IOWorkLoop::runActionBlock(ActionBlock action)
{
	return runAction(&IOWorkLoopActionToBlock, this, action);
}

IOReturn
IOWorkLoop::runAction(Action inAction, OSObject *target,
    void *arg0, void *arg1,
    void *arg2, void *arg3)
{
	IOReturn res;

	                                                                     
	closeGate();
	res = (*inAction)(target, arg0, arg1, arg2, arg3);
	openGate();

	return res;
}

IOReturn
IOWorkLoop::_maintRequest(void *inC, void *inD, void *, void *)
{
	maintCommandEnum command = (maintCommandEnum) (uintptr_t) inC;
	IOEventSource *inEvent = (IOEventSource *) inD;
	IOReturn res = kIOReturnSuccess;

	switch (command) {
	case mAddEvent:
		if (!inEvent->getWorkLoop()) {
			SETP(&fFlags, kLoopRestart);

			inEvent->retain();
			inEvent->setWorkLoop(this);
			inEvent->setNext(NULL);

			                                                                   
			if (eventSourcePerformsWork(inEvent)) {
				if (!eventChain) {
					eventChain = inEvent;
				} else {
					IOEventSource *event, *next;

					for (event = eventChain; (next = event->getNext()); event = next) {
						;
					}
					event->setNext(inEvent);
				}
			} else {
				if (!passiveEventChain) {
					passiveEventChain = inEvent;
				} else {
					IOEventSource *event, *next;

					for (event = passiveEventChain; (next = event->getNext()); event = next) {
						;
					}
					event->setNext(inEvent);
				}
			}
			IOStatisticsAttachEventSource();
		}
		break;

	case mRemoveEvent:
		if (inEvent->getWorkLoop()) {
			IOStatisticsDetachEventSource();

			if (eventSourcePerformsWork(inEvent)) {
				if (eventChain == inEvent) {
					eventChain = inEvent->getNext();
				} else {
					IOEventSource *event, *next = NULL;

					event = eventChain;
					if (event) {
						while ((next = event->getNext()) && (next != inEvent)) {
							event = next;
						}
					}

					if (!next) {
						res = kIOReturnBadArgument;
						break;
					}
					event->setNext(inEvent->getNext());
				}
			} else {
				if (passiveEventChain == inEvent) {
					passiveEventChain = inEvent->getNext();
				} else {
					IOEventSource *event, *next = NULL;

					event = passiveEventChain;
					if (event) {
						while ((next = event->getNext()) && (next != inEvent)) {
							event = next;
						}
					}

					if (!next) {
						res = kIOReturnBadArgument;
						break;
					}
					event->setNext(inEvent->getNext());
				}
			}

			inEvent->setWorkLoop(NULL);
			inEvent->setNext(NULL);
			inEvent->release();
			SETP(&fFlags, kLoopRestart);
		}
		break;

	default:
		return kIOReturnUnsupported;
	}

	return res;
}

bool
IOWorkLoop::eventSourcePerformsWork(IOEventSource *inEventSource)
{
	bool    result = true;

	  
                                                                                           
                                                                         
                           
   
                                                              
                                                                           
                                                                
   
                                                                                 
                                                                     
                                                              
                                        
   
                                                                             
                                                                           
   
    

	if (IOEventSource::kPassive & inEventSource->flags) {
		result = false;
	} else if (IOEventSource::kActive & inEventSource->flags) {
		result = true;
	} else if (controlG) {
		void *  ptr1;
		void *  ptr2;

		ptr1 = OSMemberFunctionCast(void*, inEventSource, &IOEventSource::checkForWork);
		ptr2 = OSMemberFunctionCast(void*, controlG, &IOEventSource::checkForWork);

		if (ptr1 == ptr2) {
			result = false;
		}
	}

	return result;
}

void
IOWorkLoop::lockTime(void)
{
	uint64_t time;
	time = mach_absolute_time() - reserved->lockTime;
	if (time > reserved->lockInterval) {
		absolutetime_to_nanoseconds(time, &time);
		if (kTimeLockPanics & reserved->options) {
			panic("IOWorkLoop %p lock time %qd us", this, time / 1000ULL);
		} else {
			OSReportWithBacktrace("IOWorkLoop %p lock time %qd us", this, time / 1000ULL);
		}
	}
}

void
IOWorkLoop::setMaximumLockTime(uint64_t interval, uint32_t options)
{
	IORecursiveLockLock(gateLock);
	reserved->lockInterval = interval;
	reserved->options = (reserved->options & ~kTimeLockPanics) | (options & kTimeLockPanics);
	IORecursiveLockUnlock(gateLock);
}
