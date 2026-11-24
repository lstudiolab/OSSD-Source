/*
 * Copyright (c) 2007-2012 Apple Inc. All rights reserved.
 * Copyright (c) 1998-2006 Apple Computer, Inc. All rights reserved.
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

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOInterrupts.h>
#include <IOKit/IOInterruptController.h>
#include <IOKit/IOKitDebug.h>
#include <IOKit/IOTimeStamp.h>


                                                                           

#define super IOService

OSDefineMetaClassAndAbstractStructors(IOInterruptController, IOService);

OSMetaClassDefineReservedUsedX86(IOInterruptController, 0);
OSMetaClassDefineReservedUsedX86(IOInterruptController, 1);
OSMetaClassDefineReservedUsedX86(IOInterruptController, 2);
OSMetaClassDefineReservedUnused(IOInterruptController, 3);
OSMetaClassDefineReservedUnused(IOInterruptController, 4);
OSMetaClassDefineReservedUnused(IOInterruptController, 5);

                                                                           

IOReturn
IOInterruptController::registerInterrupt(IOService *nub, int source,
    void *target,
    IOInterruptHandler handler,
    void *refCon)
{
	IOInterruptSource *interruptSources;
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector;
	int               wasDisabledSoft;
	IOReturn          error;
	OSData            *vectorData;
	IOOptionBits      options;
	bool              canBeShared, shouldBeShared, wasAlreadyRegisterd;

	IOService         *originalNub = NULL;                                   
	int               originalSource = 0;                                   


	interruptSources = nub->_interruptSources;
	vectorData = interruptSources[source].vectorData;
	vectorNumber = *(IOInterruptVectorNumber *)vectorData->getBytesNoCopy();
	vector = &vectors[vectorNumber];

	                                
	IOLockLock(vector->interruptLock);

	                                                      
	canBeShared = vectorCanBeShared(vectorNumber, vector);
	IODTGetInterruptOptions(nub, source, &options);
#if defined(__i386__) || defined(__x86_64__)
	int   interruptType;
	if (OSDynamicCast(IOPlatformDevice, getProvider()) &&
	    (getInterruptType(nub, source, &interruptType) == kIOReturnSuccess) &&
	    (kIOInterruptTypeLevel & interruptType)) {
		options |= kIODTInterruptShared;
	}
#endif
	shouldBeShared = canBeShared && (options & kIODTInterruptShared);
	wasAlreadyRegisterd = vector->interruptRegistered;

	                                                                  
	if (wasAlreadyRegisterd && !canBeShared) {
		IOLockUnlock(vector->interruptLock);
		return kIOReturnNoResources;
	}

	                                                                 
	                                                
	                                  
	if (wasAlreadyRegisterd || shouldBeShared) {
		                                                      
		if (vector->sharedController == NULL) {
			                                               
			vector->sharedController = new IOSharedInterruptController;
			if (vector->sharedController == NULL) {
				IOLockUnlock(vector->interruptLock);
				return kIOReturnNoMemory;
			}

			if (wasAlreadyRegisterd) {
				                                                     
				originalNub = vector->nub;
				originalSource = vector->source;

				                                                                                  
				                                                                               
				disableVectorHard(vectorNumber, vector);
				vector->interruptDisabledHard = 0;
			}

			                                                  
			error = vector->sharedController->initInterruptController(this, vectorData);
			                                                              
			                                                                      
			                                                  
			if (error != kIOReturnSuccess) {
				if (wasAlreadyRegisterd) {
					enableInterrupt(originalNub, originalSource);
				}
				vector->sharedController->release();
				vector->sharedController = NULL;
				IOLockUnlock(vector->interruptLock);
				return error;
			}

			                                                                                 
			if (wasAlreadyRegisterd) {
				error = vector->sharedController->registerInterrupt(originalNub,
				    originalSource,
				    vector->target,
				    vector->handler,
				    vector->refCon);
				                                                                        
				                                                           
				                                                  
				if (error != kIOReturnSuccess) {
					                                                
					wasDisabledSoft = vector->interruptDisabledSoft;

					                                           
					vector->interruptDisabledSoft = 1;
					vector->interruptDisabledHard = 1;

					                                                      
					if (!wasDisabledSoft) {
						originalNub->enableInterrupt(originalSource);
					}
					enableInterrupt(originalNub, originalSource);

					vector->sharedController->release();
					vector->sharedController = NULL;
					IOLockUnlock(vector->interruptLock);
					return error;
				}
			}

			                                                    
			vector->handler = (IOInterruptHandler)vector->sharedController->getInterruptHandlerAddress();
			vector->nub     = vector->sharedController;
			vector->source  = 0;
			vector->target  = vector->sharedController;
			vector->refCon  = NULL;

			                                           
			                                                
			if (wasAlreadyRegisterd) {
				wasDisabledSoft = vector->interruptDisabledSoft;
			} else {
				wasDisabledSoft = true;
			}

			                                                                             
			if (!wasAlreadyRegisterd) {
				initVector(vectorNumber, vector);
			}

			                                           
			vector->interruptDisabledSoft = 1;
			vector->interruptDisabledHard = 1;
			vector->interruptRegistered   = 1;

			                                                      
			                                                                       
			if (!wasDisabledSoft) {
				originalNub->enableInterrupt(originalSource);
			}
		}

		error = vector->sharedController->registerInterrupt(nub, source, target,
		    handler, refCon);
		IOLockUnlock(vector->interruptLock);
		return error;
	}

	                                         
	vector->handler = handler;
	vector->nub     = nub;
	vector->source  = source;
	vector->target  = target;
	vector->refCon  = refCon;

	                                                 
	initVector(vectorNumber, vector);

	                                                  
	vector->interruptDisabledHard = 1;
	vector->interruptDisabledSoft = 1;
	vector->interruptRegistered   = 1;

	IOLockUnlock(vector->interruptLock);
	return kIOReturnSuccess;
}

IOReturn
IOInterruptController::unregisterInterrupt(IOService *nub, int source)
{
	IOInterruptSource *interruptSources;
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector;
	OSData            *vectorData;

	interruptSources = nub->_interruptSources;
	vectorData = interruptSources[source].vectorData;
	vectorNumber = *(IOInterruptVectorNumber *)vectorData->getBytesNoCopy();
	vector = &vectors[vectorNumber];

	                                
	IOLockLock(vector->interruptLock);

	                                                 
	if (!vector->interruptRegistered) {
		IOLockUnlock(vector->interruptLock);
		return kIOReturnSuccess;
	}

	                           
	disableInterrupt(nub, source);

	                                   
	disableVectorHard(vectorNumber, vector);

	                                                                 
	vector->interruptActive = 0;
	vector->interruptDisabledSoft = 0;
	vector->interruptDisabledHard = 0;
	vector->interruptRegistered = 0;
	vector->nub = NULL;
	vector->source = 0;
	vector->handler = NULL;
	vector->target = NULL;
	vector->refCon = NULL;

	IOLockUnlock(vector->interruptLock);
	return kIOReturnSuccess;
}

IOReturn
IOInterruptController::getInterruptType(IOService *nub, int source,
    int *interruptType)
{
	IOInterruptSource *interruptSources;
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector;
	OSData            *vectorData;

	if (interruptType == NULL) {
		return kIOReturnBadArgument;
	}

	interruptSources = nub->_interruptSources;
	vectorData = interruptSources[source].vectorData;
	vectorNumber = *(IOInterruptVectorNumber *)vectorData->getBytesNoCopy();
	vector = &vectors[vectorNumber];

	*interruptType = getVectorType(vectorNumber, vector);

	return kIOReturnSuccess;
}

IOReturn
IOInterruptController::enableInterrupt(IOService *nub, int source)
{
	IOInterruptSource *interruptSources;
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector;
	OSData            *vectorData;

	interruptSources = nub->_interruptSources;
	vectorData = interruptSources[source].vectorData;
	vectorNumber = *(IOInterruptVectorNumber *)vectorData->getBytesNoCopy();
	vector = &vectors[vectorNumber];

	if (vector->interruptDisabledSoft) {
		vector->interruptDisabledSoft = 0;
#if !defined(__i386__) && !defined(__x86_64__)
		OSMemoryBarrier();
#endif

		if (!getPlatform()->atInterruptLevel()) {
			while (vector->interruptActive) {
			}
		}
		if (vector->interruptDisabledHard) {
			vector->interruptDisabledHard = 0;

			                                                              
			                                                               
			                                                            
			                                                              
			                                                                 
			                                                           
			OSSynchronizeIO();

			enableVector(vectorNumber, vector);
		}
	}

	return kIOReturnSuccess;
}

IOReturn
IOInterruptController::disableInterrupt(IOService *nub, int source)
{
	IOInterruptSource *interruptSources;
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector;
	OSData            *vectorData;

	interruptSources = nub->_interruptSources;
	vectorData = interruptSources[source].vectorData;
	vectorNumber = *(IOInterruptVectorNumber *)vectorData->getBytesNoCopy();
	vector = &vectors[vectorNumber];

	vector->interruptDisabledSoft = 1;
#if !defined(__i386__) && !defined(__x86_64__)
	OSMemoryBarrier();
#endif

	if (!getPlatform()->atInterruptLevel()) {
		while (vector->interruptActive) {
		}
	}

	return kIOReturnSuccess;
}

IOReturn
IOInterruptController::causeInterrupt(IOService *nub, int source)
{
	IOInterruptSource *interruptSources;
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector;
	OSData            *vectorData;

	interruptSources = nub->_interruptSources;
	vectorData = interruptSources[source].vectorData;
	vectorNumber = *(IOInterruptVectorNumber *)vectorData->getBytesNoCopy();
	vector = &vectors[vectorNumber];

	causeVector(vectorNumber, vector);

	return kIOReturnSuccess;
}

IOInterruptAction
IOInterruptController::getInterruptHandlerAddress(void)
{
	return NULL;
}

IOReturn
IOInterruptController::handleInterrupt(void *refCon, IOService *nub,
    int source)
{
	return kIOReturnInvalid;
}


                                                                          

bool
IOInterruptController::vectorCanBeShared(IOInterruptVectorNumber                 ,
    IOInterruptVector *          )
{
	return false;
}

void
IOInterruptController::initVector(IOInterruptVectorNumber                 ,
    IOInterruptVector *          )
{
}

int
IOInterruptController::getVectorType(IOInterruptVectorNumber                 ,
    IOInterruptVector *          )
{
	return kIOInterruptTypeEdge;
}

void
IOInterruptController::disableVectorHard(IOInterruptVectorNumber                 ,
    IOInterruptVector *          )
{
}

void
IOInterruptController::enableVector(IOInterruptVectorNumber                 ,
    IOInterruptVector *          )
{
}

void
IOInterruptController::causeVector(IOInterruptVectorNumber                 ,
    IOInterruptVector *          )
{
}

void
IOInterruptController::setCPUInterruptProperties(IOService *           )
{
}

void
IOInterruptController::sendIPI(unsigned int           , bool             )
{
}

void
IOInterruptController::cancelDeferredIPI(unsigned int           )
{
}

void
IOInterruptController::timeStampSpuriousInterrupt(void)
{
	uint64_t providerID = 0;
	IOService * provider = getProvider();

	if (provider) {
		providerID = provider->getRegistryEntryID();
	}

	IOTimeStampConstant(IODBG_INTC(IOINTC_SPURIOUS), providerID);
}

void
IOInterruptController::timeStampInterruptHandlerInternal(bool isStart, IOInterruptVectorNumber vectorNumber, IOInterruptVector *vector)
{
	uint64_t providerID = 0;
	vm_offset_t unslidHandler = 0;
	vm_offset_t unslidTarget = 0;

	IOService * provider = getProvider();

	if (provider) {
		providerID = provider->getRegistryEntryID();
	}

	if (vector) {
		unslidHandler = VM_KERNEL_UNSLIDE((vm_offset_t)vector->handler);
		unslidTarget = VM_KERNEL_UNSLIDE_OR_PERM((vm_offset_t)vector->target);
	}


	if (isStart) {
#if INTERRUPT_MASKED_DEBUG
		ml_irq_debug_start((uintptr_t)vector->handler, (uintptr_t)vector);
#endif
		IOTimeStampStartConstant(IODBG_INTC(IOINTC_HANDLER), (uintptr_t)vectorNumber, (uintptr_t)unslidHandler,
		    (uintptr_t)unslidTarget, (uintptr_t)providerID);
	} else {
		IOTimeStampEndConstant(IODBG_INTC(IOINTC_HANDLER), (uintptr_t)vectorNumber, (uintptr_t)unslidHandler,
		    (uintptr_t)unslidTarget, (uintptr_t)providerID);
#if INTERRUPT_MASKED_DEBUG
		ml_irq_debug_end();
#endif
	}
}

void
IOInterruptController::timeStampInterruptHandlerStart(IOInterruptVectorNumber vectorNumber, IOInterruptVector *vector)
{
	timeStampInterruptHandlerInternal(true, vectorNumber, vector);
}

void
IOInterruptController::timeStampInterruptHandlerEnd(IOInterruptVectorNumber vectorNumber, IOInterruptVector *vector)
{
	timeStampInterruptHandlerInternal(false, vectorNumber, vector);
}

                                                                           

#undef  super
#define super IOInterruptController

OSDefineMetaClassAndStructors(IOSharedInterruptController, IOInterruptController);

OSMetaClassDefineReservedUnused(IOSharedInterruptController, 0);
OSMetaClassDefineReservedUnused(IOSharedInterruptController, 1);
OSMetaClassDefineReservedUnused(IOSharedInterruptController, 2);
OSMetaClassDefineReservedUnused(IOSharedInterruptController, 3);

                                                                           

#define kIOSharedInterruptControllerDefaultVectors (128)

IOReturn
IOSharedInterruptController::initInterruptController(IOInterruptController *parentController, OSData *parentSource)
{
	int      cnt, interruptType;
	IOReturn error;

	if (!super::init()) {
		return kIOReturnNoResources;
	}

	                                                          
	provider = this;

	                                                             
	_interruptSources = (IOInterruptSource *)IOMalloc(sizeof(IOInterruptSource));
	if (_interruptSources == NULL) {
		return kIOReturnNoMemory;
	}
	_numInterruptSources = 1;

	                                                 
	parentController->retain();
	parentSource->retain();
	_interruptSources[0].interruptController = parentController;
	_interruptSources[0].vectorData = parentSource;

	sourceIsLevel = false;
	error = provider->getInterruptType(0, &interruptType);
	if (error == kIOReturnSuccess) {
		if (interruptType & kIOInterruptTypeLevel) {
			sourceIsLevel = true;
		}
	}

	                                      
	numVectors = kIOSharedInterruptControllerDefaultVectors;                              
	vectors = (IOInterruptVector *)IOMalloc(numVectors * sizeof(IOInterruptVector));
	if (vectors == NULL) {
		IOFree(_interruptSources, sizeof(IOInterruptSource));
		return kIOReturnNoMemory;
	}
	bzero(vectors, numVectors * sizeof(IOInterruptVector));

	                                        
	controllerLock = IOSimpleLockAlloc();
	if (controllerLock == NULL) {
		return kIOReturnNoResources;
	}

	                                  
	for (cnt = 0; cnt < numVectors; cnt++) {
		vectors[cnt].interruptLock = IOLockAlloc();
		if (vectors[cnt].interruptLock == NULL) {
			for (cnt = 0; cnt < numVectors; cnt++) {
				if (vectors[cnt].interruptLock != NULL) {
					IOLockFree(vectors[cnt].interruptLock);
				}
			}
			return kIOReturnNoResources;
		}
	}

	numVectors = 0;                                              
	vectorsRegistered = 0;
	vectorsEnabled = 0;
	controllerDisabled = 1;

	return kIOReturnSuccess;
}

IOReturn
IOSharedInterruptController::registerInterrupt(IOService *nub,
    int source,
    void *target,
    IOInterruptHandler handler,
    void *refCon)
{
	IOInterruptSource *interruptSources;
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector = NULL;
	OSData            *vectorData;
	IOInterruptState  interruptState;

	interruptSources = nub->_interruptSources;

	                      
	vectorNumber = kIOSharedInterruptControllerDefaultVectors;
	while (vectorsRegistered != kIOSharedInterruptControllerDefaultVectors) {
		for (vectorNumber = 0; vectorNumber < kIOSharedInterruptControllerDefaultVectors; vectorNumber++) {
			vector = &vectors[vectorNumber];

			                                
			IOLockLock(vector->interruptLock);

			                      
			if (!vector->interruptRegistered) {
				break;
			}

			                              
			IOLockUnlock(vector->interruptLock);
		}

		if (vectorNumber != kIOSharedInterruptControllerDefaultVectors) {
			break;
		}
	}

	                                         
	if (vectorNumber == kIOSharedInterruptControllerDefaultVectors) {
		return kIOReturnNoResources;
	}

	                                                   
	vectorData = OSData::withBytes(&vectorNumber, sizeof(vectorNumber));
	if (vectorData == NULL) {
		IOLockUnlock(vector->interruptLock);
		return kIOReturnNoMemory;
	}

	                                                            
	interruptSources[source].interruptController = this;
	interruptSources[source].vectorData = vectorData;

	                                         
	vector->handler = handler;
	vector->nub     = nub;
	vector->source  = source;
	vector->target  = target;
	vector->refCon  = refCon;

	                                                      
	vector->interruptDisabledSoft = 1;
	vector->interruptRegistered   = 1;

	interruptState = IOSimpleLockLockDisableInterrupt(controllerLock);
	                                     
	if (++vectorsRegistered > numVectors) {
		numVectors = vectorsRegistered;
	}
	IOSimpleLockUnlockEnableInterrupt(controllerLock, interruptState);

	IOLockUnlock(vector->interruptLock);
	return kIOReturnSuccess;
}

IOReturn
IOSharedInterruptController::unregisterInterrupt(IOService *nub,
    int source)
{
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector;
	IOInterruptState  interruptState;

	for (vectorNumber = 0; vectorNumber < kIOSharedInterruptControllerDefaultVectors; vectorNumber++) {
		vector = &vectors[vectorNumber];

		                                
		IOLockLock(vector->interruptLock);

		                                                 
		if (!vector->interruptRegistered
		    || (vector->nub != nub) || (vector->source != source)) {
			IOLockUnlock(vector->interruptLock);
			continue;
		}

		                                                  
		disableInterrupt(nub, source);

		                                                                 
		vector->interruptActive = 0;
		vector->interruptDisabledSoft = 0;
		vector->interruptDisabledHard = 0;
		vector->interruptRegistered = 0;
		vector->nub = NULL;
		vector->source = 0;
		vector->handler = NULL;
		vector->target = NULL;
		vector->refCon = NULL;

		interruptState = IOSimpleLockLockDisableInterrupt(controllerLock);
		vectorsRegistered--;
		IOSimpleLockUnlockEnableInterrupt(controllerLock, interruptState);

		                              
		IOLockUnlock(vector->interruptLock);
	}

	                                                       
	if (vectorsEnabled == vectorsRegistered) {
		controllerDisabled = 0;
		provider->enableInterrupt(0);
	}

	return kIOReturnSuccess;
}

IOReturn
IOSharedInterruptController::getInterruptType(IOService *       ,
    int           ,
    int *interruptType)
{
	return provider->getInterruptType(0, interruptType);
}

IOReturn
IOSharedInterruptController::enableInterrupt(IOService *nub,
    int source)
{
	IOInterruptSource *interruptSources;
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector;
	OSData            *vectorData;
	IOInterruptState  interruptState;

	interruptSources = nub->_interruptSources;
	vectorData = interruptSources[source].vectorData;
	vectorNumber = *(IOInterruptVectorNumber *)vectorData->getBytesNoCopy();
	vector = &vectors[vectorNumber];

	interruptState = IOSimpleLockLockDisableInterrupt(controllerLock);
	if (!vector->interruptDisabledSoft) {
		IOSimpleLockUnlockEnableInterrupt(controllerLock, interruptState);
		return kIOReturnSuccess;
	}

	vector->interruptDisabledSoft = 0;
	vectorsEnabled++;
	IOSimpleLockUnlockEnableInterrupt(controllerLock, interruptState);

	if (controllerDisabled && (vectorsEnabled == vectorsRegistered)) {
		controllerDisabled = 0;
		provider->enableInterrupt(0);
	}

	return kIOReturnSuccess;
}

IOReturn
IOSharedInterruptController::disableInterrupt(IOService *nub,
    int source)
{
	IOInterruptSource *interruptSources;
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector;
	OSData            *vectorData;
	IOInterruptState  interruptState;

	interruptSources = nub->_interruptSources;
	vectorData = interruptSources[source].vectorData;
	vectorNumber = *(IOInterruptVectorNumber *)vectorData->getBytesNoCopy();
	vector = &vectors[vectorNumber];

	interruptState = IOSimpleLockLockDisableInterrupt(controllerLock);
	if (!vector->interruptDisabledSoft) {
		vector->interruptDisabledSoft = 1;
#if !defined(__i386__) && !defined(__x86_64__)
		OSMemoryBarrier();
#endif

		vectorsEnabled--;
	}
	IOSimpleLockUnlockEnableInterrupt(controllerLock, interruptState);

	if (!getPlatform()->atInterruptLevel()) {
		while (vector->interruptActive) {
		}
	}

	return kIOReturnSuccess;
}

IOInterruptAction
IOSharedInterruptController::getInterruptHandlerAddress(void)
{
	return OSMemberFunctionCast(IOInterruptAction,
	           this, &IOSharedInterruptController::handleInterrupt);
}

IOReturn
IOSharedInterruptController::handleInterrupt(void *           ,
    IOService * nub,
    int           )
{
	IOInterruptVectorNumber vectorNumber;
	IOInterruptVector *vector;

	for (vectorNumber = 0; vectorNumber < numVectors; vectorNumber++) {
		vector = &vectors[vectorNumber];

		vector->interruptActive = 1;
#if !defined(__i386__) && !defined(__x86_64__)
		OSMemoryBarrier();
#endif

		if (!vector->interruptDisabledSoft) {
			                                 
			if (vector->interruptRegistered) {
				bool trace = (gIOKitTrace & kIOTraceInterrupts) ? true : false;

				if (trace) {
					timeStampInterruptHandlerStart(vectorNumber, vector);
				}

				                
				vector->handler(vector->target, vector->refCon, vector->nub, vector->source);

				if (trace) {
					timeStampInterruptHandlerEnd(vectorNumber, vector);
				}
			}
		}

		vector->interruptActive = 0;
	}

	                                                                      
	IOSimpleLockLock(controllerLock);
	if (vectorsEnabled != vectorsRegistered) {
		nub->disableInterrupt(0);
		controllerDisabled = 1;
	}
	IOSimpleLockUnlock(controllerLock);

	return kIOReturnSuccess;
}
