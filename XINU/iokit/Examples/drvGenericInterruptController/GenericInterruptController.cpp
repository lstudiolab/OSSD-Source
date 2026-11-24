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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
 *
 *  DRI: Josh de Cesare
 */

#include <IOKit/IOPlatformExpert.h>

#include "GenericInterruptController.h"

                                                                           

#undef  super
#define super IOInterruptController

IODefineMetaClassAndStructors(GenericInterruptController,
    IOInterruptController);

                                                                           


bool
GenericInterruptController::start(IOService *provider)
{
	IOInterruptAction    handler;
	IOSymbol             *interruptControllerName;

	                                    
	if (!super::start(provider)) {
		return false;
	}

	                                                   

	                                                     
	                                                                         
	                                                   
#if 0
	                                       
	vectors = (IOInterruptVector *)IOMalloc(numVectors *
	    sizeof(IOInterruptVector));
	if (vectors == NULL) {
		return false;
	}
	bzero(vectors, numVectors * sizeof(IOInterruptVector));

	                                  
	for (cnt = 0; cnt < numVectors; cnt++) {
		vectors[cnt].interruptLock = IOLockAlloc();
		if (vectors[cnt].interruptLock == NULL) {
			for (cnt = 0; cnt < numVectors; cnt++) {
				if (vectors[cnt].interruptLock != NULL) {
					IOLockFree(vectors[cnt].interruptLock);
				}
			}
		}
	}
#endif

	                                                            
	                                                                    
	                                         
	getPlatform()->setCPUInterruptProperties(provider);

	                                                               
	handler = getInterruptHandlerAddress();
	provider->registerInterrupt(0, this, handler, 0);

	                                                                            
	provider->enableInterrupt(0);

	                                                    
	                                

	                                                             
	getPlatform()->registerInterruptController(interruptControllerName, this);

	                            
	return true;
}

IOReturn
GenericInterruptController::getInterruptType(IOService *nub,
    int source,
    int *interruptType)
{
	if (interruptType == 0) {
		return kIOReturnBadArgument;
	}

	                                                                

	return kIOReturnSuccess;
}

                                                                      
IOInterruptAction
GenericInterruptController::getInterruptHandlerAddress(void)
{
	return (IOInterruptAction)handleInterrupt;
}

                                 
IOReturn
GenericInterruptController::handleInterrupt(void * refCon,
    IOService * nub,
    int source)
{
	IOInterruptVector *vector;
	int               vectorNumber;

	while (1) {
		                                                               

		                                                
		if (vectorNumber == 0              ) {
			break;
		}

		                                                     
		vector = &vectors[vectorNumber];

		                                                            
		                                                         
		                      
		vector->interruptActive = 1;
		         
		          

		                                                 
		if (!vector->interruptDisabledSoft) {
			                                                             
			          

			                                 
			if (vector->interruptRegistered) {
				vector->handler(vector->target, vector->refCon,
				    vector->nub, vector->source);
			}
		} else {
			                                                        
			vector->interruptDisabledHard = 1;
			disableVectorHard(vectorNumber, vector);
		}

		                                                     
		vector->interruptActive = 0;
	}

	return kIOReturnSuccess;
}

bool
GenericInterruptController::vectorCanBeShared(long vectorNumber,
    IOInterruptVector *vector)
{
	                                                                           
	return true;
}

void
GenericInterruptController::initVector(long vectorNumber,
    IOInterruptVector *vector)
{
	                                               
	                                                                
	                                         
}

void
GenericInterruptController::disableVectorHard(long vectorNumber,
    IOInterruptVector *vector)
{
	                                               
	                                      
}

void
GenericInterruptController::enableVector(long vectorNumber,
    IOInterruptVector *vector)
{
	                                               
	                                     
}

void
GenericInterruptController::causeVector(long vectorNumber,
    IOInterruptVector *vector)
{
	                                               
	                                                                          

	                                                                         
	                                                 
	getPlatform()->causeInterrupt(0);
}
