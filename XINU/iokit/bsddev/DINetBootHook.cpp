/*
 * Copyright (c) 2002-2016 Apple Inc. All rights reserved.
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
  
                   
              
  
                                            
  
                   
  
                              
                                            
                                                                            
                    
  
                                                   
                                                          
  
                                                
                                 
  
                                            
                                                                            
                                                          
  
                                                 
                                                             
  
                                                 
                 
                                                             
  
                                            
                                                                             
                         
  
                                                
                 
                                         
  
                                         
                                                                     
  
                                         
                                                                       
  
                                         
                                
                                        
  
                                         
                                  
  
  
   
#ifndef qDEBUG
#define qDEBUG 0
#endif

#if qDEBUG
#warning qDEBUG is 1!
#endif

#include <sys/types.h>
#include <mach/clock_types.h>
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include "DINetBootHook.h"

#define kIOHDIXControllerClassName      "IOHDIXController"
#define kDIRootImageKey                         "di-root-image"
#define kDIRootImageRemovableKey                "di-root-removable"
#define kDIRootImageResultKey           "di-root-image-result"
#define kDIRootImageDevNameKey          "di-root-image-devname"
#define kDIRootImageDevTKey                     "di-root-image-devt"
#define kDIRootRamFileKey           "di-root-ram-file"

#define kDIMatchQuiesceTimeout          30ull

static IOService *
di_load_controller( void )
{
	OSIterator *    controllerIterator      = NULL;
	OSDictionary *  matchDictionary         = NULL;
	IOService *     controller                      = NULL;

	do {
		IOService::getResourceService()->publishResource("com.apple.AppleDiskImageController.load", kOSBooleanTrue);
		IOService::getResourceService()->waitQuiet();

		                              
		matchDictionary = IOService::serviceMatching(kIOHDIXControllerClassName);
		if (!matchDictionary) {
			break;
		}

		controllerIterator = IOService::getMatchingServices(matchDictionary);
		if (!controllerIterator) {
			break;
		}

		controller = OSDynamicCast(IOService, controllerIterator->getNextObject());
		if (!controller) {
			break;
		}

		controller->retain();
	} while (false);

	if (matchDictionary) {
		matchDictionary->release();
	}
	if (controllerIterator) {
		controllerIterator->release();
	}

	return controller;
}

extern "C" {
                                                                           
                                                                           
                                                                   
                                           
   
static int
di_add_properties(IOService *controller, bool removable)
{
	if (!controller->setProperty(kDIRootImageRemovableKey, removable ? kOSBooleanTrue : kOSBooleanFalse)) {
		IOLog("IOHDIXController::setProperty(%s, %d) failed.\n", kDIRootImageRemovableKey, !!removable);
		return kIOReturnBadArgument;
	}

	return kIOReturnSuccess;
}

int
di_root_image_ext(const char *path, char *devname, size_t devsz, dev_t *dev_p, bool removable)
{
	IOReturn                        res                             = 0;
	IOService               *       controller                      = NULL;
	OSString                *       pathString                      = NULL;
	OSNumber                *       myResult                        = NULL;
	OSString                *       myDevName                       = NULL;
	OSNumber                *       myDevT                          = NULL;

	                                
	if (devname) {
		*devname = 0;
	}
	if (dev_p) {
		*dev_p = 0;
	}

	if (!path) {
		return kIOReturnBadArgument;
	}
	if (!devname) {
		return kIOReturnBadArgument;
	}
	if (!dev_p) {
		return kIOReturnBadArgument;
	}

	controller = di_load_controller();
	if (!controller) {
		res = kIOReturnNotFound;
		goto NoIOHDIXController;
	}

	                          
	pathString = OSString::withCString(path);
	if (!pathString) {
		res = kIOReturnNoMemory;
		goto CannotCreatePathOSString;
	}

	  
                                                             
                                                                    
                                                              
                        
    
	res = di_add_properties(controller, removable);
	if (res) {
		goto error_add_properties;
	}

	        
	if (!controller->setProperty(kDIRootImageKey, pathString)) {
		IOLog("IOHDIXController::setProperty(%s, %s) failed.\n", kDIRootImageKey, pathString->getCStringNoCopy());
	}

	myResult = OSDynamicCast(OSNumber, controller->getProperty(kDIRootImageResultKey));
	res = kIOReturnError;
	if (myResult) {
		res = myResult->unsigned32BitValue();
	}

	if (res) {
		IOLog("%s is 0x%08X/%d\n", kDIRootImageResultKey, res, res);
		goto di_root_image_FAILED;
	}

	                 
	myDevT = OSDynamicCast(OSNumber, controller->getProperty(kDIRootImageDevTKey));
	if (myDevT) {
		*dev_p = myDevT->unsigned32BitValue();
	} else {
		IOLog("could not get %s\n", kDIRootImageDevTKey);
		res = kIOReturnError;
		goto di_root_image_FAILED;
	}

	myDevName = OSDynamicCast(OSString, controller->getProperty(kDIRootImageDevNameKey));
	if (myDevName) {
		strlcpy(devname, myDevName->getCStringNoCopy(), devsz);
	} else {
		IOLog("could not get %s\n", kDIRootImageDevNameKey);
		res = kIOReturnError;
		goto di_root_image_FAILED;
	}

	  
                                                                                           
                                                                                             
                                                                                               
   
                                                                                              
                                                                                                
                                               
    
	res = controller->waitQuiet((NSEC_PER_SEC * kDIMatchQuiesceTimeout));
	if (res) {
		IOLog("failed to quiesce attached disk image (%s)! \n", devname);
		goto di_root_image_FAILED;
	}

di_root_image_FAILED:
CannotCreatePathOSString:
NoIOHDIXController:
error_add_properties:

	                              
	if (pathString) {
		pathString->release();
	}
	if (controller) {
		controller->release();
	}

	return res;
}

  
                             
                                                              
                                                    
                                                                                                
                                                                                           
                  
                                                                    
   
int
di_root_image(const char *path, char *devname, size_t devsz, dev_t *dev_p)
{
	return di_root_image_ext(path, devname, devsz, dev_p, false);
}

int
di_root_ramfile_buf(void *buf, size_t bufsz, char *devname, size_t devsz, dev_t *dev_p)
{
	IOReturn res = 0;
	IOService *controller = NULL;
	OSNumber *myResult = NULL;
	OSString *myDevName = NULL;
	OSNumber *myDevT = NULL;
	IOMemoryDescriptor *mem = NULL;

	mem = IOMemoryDescriptor::withAddress(buf, bufsz, kIODirectionInOut);
	assert(mem);

	controller = di_load_controller();
	if (controller) {
		                      
		controller->setProperty(kDIRootRamFileKey, mem);
		controller->release();
	} else {
		res = kIOReturnNotFound;
		goto out;
	}

	myResult = OSDynamicCast(OSNumber, controller->getProperty(kDIRootImageResultKey));
	res = kIOReturnError;
	if (myResult) {
		res = myResult->unsigned32BitValue();
	}

	if (res) {
		IOLog("%s is 0x%08X/%d\n", kDIRootImageResultKey, res, res);
		goto out;
	}

	myDevT = OSDynamicCast(OSNumber, controller->getProperty(kDIRootImageDevTKey));
	if (myDevT) {
		*dev_p = myDevT->unsigned32BitValue();
	} else {
		IOLog("could not get %s\n", kDIRootImageDevTKey);
		res = kIOReturnError;
		goto out;
	}

	myDevName = OSDynamicCast(OSString, controller->getProperty(kDIRootImageDevNameKey));
	if (myDevName) {
		strlcpy(devname, myDevName->getCStringNoCopy(), devsz);
	} else {
		IOLog("could not get %s\n", kDIRootImageDevNameKey);
		res = kIOReturnError;
		goto out;
	}

out:
	if (res) {
		OSSafeReleaseNULL(mem);
	}

	return res;
}

void
di_root_ramfile( IORegistryEntry * entry )
{
	OSData *                data;
	IOMemoryDescriptor *    mem;
	uint64_t                dmgSize;
	uint64_t                remain, length;
	OSData *                extentData = NULL;
	IOAddressRange *        extentList;
	uint64_t                extentSize;
	uint32_t                extentCount;

	do {
		data = OSDynamicCast(OSData, entry->getProperty("boot-ramdmg-size"));
		if (!data || (data->getLength() != sizeof(uint64_t))) {
			break;                       
		}
		dmgSize = *(uint64_t *) data->getBytesNoCopy();
		if (!dmgSize) {
			break;
		}

		data = OSDynamicCast(OSData, entry->getProperty("boot-ramdmg-extents"));
		if (!data || (data->getLength() == 0) ||
		    ((data->getLength() & (sizeof(IOAddressRange) - 1)) != 0)) {
			break;               
		}
		                                   
		extentData  = OSData::withData(data);
		assert(extentData);

		extentList  = (IOAddressRange *) extentData->getBytesNoCopy();
		extentCount = extentData->getLength() / sizeof(IOAddressRange);
		extentSize  = 0;
		remain = dmgSize;

		                                                 
		for (uint32_t i = 0; i < extentCount; i++) {
			length = extentList[i].length;
			if (!length) {
				break;
			}

			extentSize += length;
			if (length >= remain) {
				extentList[i].length = remain;
				extentCount = i + 1;
				break;
			}
			remain -= length;
		}
		if (extentSize < dmgSize) {
			break;                                                    
		}
		mem = IOMemoryDescriptor::withAddressRanges(
			extentList, extentCount,
			kIODirectionOut | kIOMemoryMapperNone, NULL);

		if (mem) {
			IOService * controller = di_load_controller();
			if (controller) {
				controller->setProperty(kDIRootRamFileKey, mem);
				controller->release();
			}
			mem->release();
		}
	} while (false);

	if (extentData) {
		extentData->release();
	}
}
};
