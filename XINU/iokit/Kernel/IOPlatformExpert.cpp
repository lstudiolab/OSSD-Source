/*
 * Copyright (c) 1998-2017 Apple Inc. All rights reserved.
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

#include <IOKit/IOCPU.h>
#include <device/ossd_pci.h>
#include <IOKit/IOPlatformActions.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOKitDebug.h>
#include <IOKit/IOMapper.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IONVRAM.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IORangeAllocator.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOTimeStamp.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IOKitDiagnosticsUserClient.h>
#include <IOKit/IOUserServer.h>

#include "IOKitKernelInternal.h"

#include <IOKit/system.h>
#include <sys/csr.h>

#include <libkern/c++/OSContainers.h>
#include <libkern/c++/OSSharedPtr.h>
#include <libkern/crypto/sha1.h>
#include <libkern/OSAtomic.h>

#if defined(__arm64__)
#include <arm64/tlb.h>
#endif

extern "C" {
#include <machine/machine_routines.h>
#include <pexpert/pexpert.h>
#include <uuid/uuid.h>
#include <sys/sysctl.h>
}

#define kShutdownTimeout    30          

#if defined(XNU_TARGET_OS_OSX)

boolean_t coprocessor_cross_panic_enabled = TRUE;
#define APPLE_VENDOR_VARIABLE_GUID "4d1ede05-38c7-4a6a-9cc6-4bcca8b38c14"
#endif                                 

void printDictionaryKeys(OSDictionary * inDictionary, char * inMsg);
static void getCStringForObject(OSObject *inObj, char *outStr, size_t outStrLen);

  
                                                                      
                                                                        
                                                            
                                                                 
                             
  
                                                               
   
uint32_t gEnforceQuiesceSafety = 0;

                                                                           

#define super IOService

OSDefineMetaClassAndStructors(IOPlatformExpert, IOService)

OSMetaClassDefineReservedUsedX86(IOPlatformExpert, 0);
OSMetaClassDefineReservedUsedX86(IOPlatformExpert, 1);
OSMetaClassDefineReservedUsedX86(IOPlatformExpert, 2);
OSMetaClassDefineReservedUsedX86(IOPlatformExpert, 3);
OSMetaClassDefineReservedUsedX86(IOPlatformExpert, 4);
OSMetaClassDefineReservedUsedX86(IOPlatformExpert, 5);
OSMetaClassDefineReservedUsedX86(IOPlatformExpert, 6);

OSMetaClassDefineReservedUnused(IOPlatformExpert, 7);
OSMetaClassDefineReservedUnused(IOPlatformExpert, 8);
OSMetaClassDefineReservedUnused(IOPlatformExpert, 9);
OSMetaClassDefineReservedUnused(IOPlatformExpert, 10);
OSMetaClassDefineReservedUnused(IOPlatformExpert, 11);

static IOPlatformExpert * gIOPlatform;
static OSDictionary * gIOInterruptControllers;
static IOLock * gIOInterruptControllersLock;
static IODTNVRAM *gIOOptionsEntry;

OSSymbol * gPlatformInterruptControllerName;

#if defined(__x86_64__)
static void
ossd_pci_log_device(void *context __unused,
    const struct ossd_pci_device *device)
{
	IOLog("XINU: PCI %02x:%02x.%u %04x:%04x class %02x:%02x\n",
	    device->bus, device->device, device->function,
	    device->vendor_id, device->device_id,
	    device->class_code, device->subclass);
}
#endif


                                                                           

bool
IOPlatformExpert::attach( IOService * provider )
{
	if (!super::attach( provider )) {
		return false;
	}

	return true;
}

bool
IOPlatformExpert::start( IOService * provider )
{
	IORangeAllocator *  physicalRanges;
	OSData *            busFrequency;
	uint32_t            debugFlags;


	if (!super::start(provider)) {
		return false;
	}

	                                                                                    
#if CONFIG_CSR
	if (csr_check(CSR_ALLOW_UNRESTRICTED_FS) == 0)
#endif                 
	{
		if (PE_parse_boot_argn("dart", &debugFlags, sizeof(debugFlags)) && (debugFlags == 0)) {
			removeProperty(kIOPlatformMapperPresentKey);
		}
#if DEBUG || DEVELOPMENT
		if (PE_parse_boot_argn("-x", &debugFlags, sizeof(debugFlags))) {
			removeProperty(kIOPlatformMapperPresentKey);
		}
#endif                           
	}

	                                                 
	                                             
	IOMapper::setMapperRequired(NULL != getProperty(kIOPlatformMapperPresentKey));

	gIOInterruptControllers = OSDictionary::withCapacity(1);
	gIOInterruptControllersLock = IOLockAlloc();

	                                                
	busFrequency = OSData::withBytesNoCopy((void *)&gPEClockFrequencyInfo.bus_clock_rate_hz, 4);
	provider->setProperty("clock-frequency", busFrequency);
	busFrequency->release();

	gPlatformInterruptControllerName = (OSSymbol *)OSSymbol::withCStringNoCopy("IOPlatformInterruptController");

	physicalRanges = IORangeAllocator::withRange(0xffffffff, 1, 16,
	    IORangeAllocator::kLocking);
	assert(physicalRanges);
	setProperty("Platform Memory Ranges", physicalRanges);

	setPlatform( this );
	gIOPlatform = this;

#if defined(__x86_64__)
	/* Read-only inventory pass; no device is enabled or reconfigured. */
	(void)ossd_pci_platform_enumerate(ossd_pci_log_device, NULL);
#endif

	PMInstantiatePowerDomains();

#if !defined(__x86_64__)
	publishPlatformUUIDAndSerial();
#endif                           

#if defined (__x86_64__)
	if (PEGetCoprocessorVersion() >= kCoprocessorVersion2) {
		coprocessor_paniclog_flush = TRUE;
		extended_debug_log_init();
	}
#endif

	PE_parse_boot_argn("enforce_quiesce_safety", &gEnforceQuiesceSafety,
	    sizeof(gEnforceQuiesceSafety));

	return configure(provider);
}

bool
IOPlatformExpert::configure( IOService * provider )
{
	OSSet *             topLevel;
	OSDictionary *      dict;
	IOService *         nub;

	topLevel = OSDynamicCast( OSSet, getProperty("top-level"));

	if (topLevel) {
		while ((dict = OSDynamicCast( OSDictionary,
		    topLevel->getAnyObject()))) {
			dict->retain();
			topLevel->removeObject( dict );
			nub = createNub( dict );
			if (NULL == nub) {
				continue;
			}
			dict->release();
			nub->attach( this );
			nub->registerService();
		}
	}

	return true;
}

IOService *
IOPlatformExpert::createNub( OSDictionary * from )
{
	IOService *         nub;

	nub = new IOPlatformDevice;
	if (nub) {
		if (!nub->init( from )) {
			nub->release();
			nub = NULL;
		}
	}
	return nub;
}

bool
IOPlatformExpert::compareNubName( const IOService * nub,
    OSString * name, OSString ** matched ) const
{
	return nub->IORegistryEntry::compareName( name, matched );
}

bool
IOPlatformExpert::compareNubName( const IOService * nub,
    OSString * name, OSSharedPtr<OSString>& matched ) const
{
	OSString* matchedRaw = NULL;
	bool result = compareNubName(nub, name, &matchedRaw);
	matched.reset(matchedRaw, OSNoRetain);
	return result;
}

IOReturn
IOPlatformExpert::getNubResources( IOService * nub )
{
	return kIOReturnSuccess;
}

long
IOPlatformExpert::getBootROMType(void)
{
	return _peBootROMType;
}

long
IOPlatformExpert::getChipSetType(void)
{
	return _peChipSetType;
}

long
IOPlatformExpert::getMachineType(void)
{
	return _peMachineType;
}

void
IOPlatformExpert::setBootROMType(long peBootROMType)
{
	_peBootROMType = peBootROMType;
}

void
IOPlatformExpert::setChipSetType(long peChipSetType)
{
	_peChipSetType = peChipSetType;
}

void
IOPlatformExpert::setMachineType(long peMachineType)
{
	_peMachineType = peMachineType;
}

bool
IOPlatformExpert::getMachineName( char *         , int              )
{
	return false;
}

bool
IOPlatformExpert::getModelName( char *         , int              )
{
	return false;
}

bool
IOPlatformExpert::getTargetName( char *         , int              )
{
	return false;
}

bool
IOPlatformExpert::getProductName( char *         , int              )
{
	return false;
}

OSString*
IOPlatformExpert::createSystemSerialNumberString(OSData* myProperty)
{
	return NULL;
}

IORangeAllocator *
IOPlatformExpert::getPhysicalRangeAllocator(void)
{
	return OSDynamicCast(IORangeAllocator,
	           getProperty("Platform Memory Ranges"));
}

int (*PE_halt_restart)(unsigned int type) = NULL;

int
IOPlatformExpert::haltRestart(unsigned int type)
{
	if (type == kPEPanicSync) {
		return 0;
	}

	if (type == kPEHangCPU) {
		while (true) {
		}
	}

	if (type == kPEUPSDelayHaltCPU) {
		                                                                   
		type = kPEHaltCPU;
	}

#if defined (__x86_64__)
	                                                        
	if (type == kPEPanicRestartCPU) {
		type = kPERestartCPU;
	}
#endif

	if (PE_halt_restart) {
		return (*PE_halt_restart)(type);
	} else {
		return -1;
	}
}

void
IOPlatformExpert::sleepKernel(void)
{
#if 0
	long cnt;
	boolean_t intState;

	intState = ml_set_interrupts_enabled(false);

	for (cnt = 0; cnt < 10000; cnt++) {
		IODelay(1000);
	}

	ml_set_interrupts_enabled(intState);
#else
                                               

	IOCPUSleepKernel();

                                              
#endif
}

long
IOPlatformExpert::getGMTTimeOfDay(void)
{
	return 0;
}

void
IOPlatformExpert::setGMTTimeOfDay(long secs)
{
}


IOReturn
IOPlatformExpert::getConsoleInfo( PE_Video * consoleInfo )
{
	return PE_current_console( consoleInfo);
}

IOReturn
IOPlatformExpert::setConsoleInfo( PE_Video * consoleInfo,
    unsigned int op)
{
	return PE_initialize_console( consoleInfo, op );
}

IOReturn
IOPlatformExpert::registerInterruptController(OSSymbol *name, IOInterruptController *interruptController)
{
	IOLockLock(gIOInterruptControllersLock);

	gIOInterruptControllers->setObject(name, interruptController);

	IOLockWakeup(gIOInterruptControllersLock,
	    gIOInterruptControllers,                  false);

	IOLockUnlock(gIOInterruptControllersLock);

	return kIOReturnSuccess;
}

IOReturn
IOPlatformExpert::deregisterInterruptController(OSSymbol *name)
{
	IOLockLock(gIOInterruptControllersLock);

	gIOInterruptControllers->removeObject(name);

	IOLockUnlock(gIOInterruptControllersLock);

	return kIOReturnSuccess;
}

IOInterruptController *
IOPlatformExpert::lookUpInterruptController(OSSymbol *name)
{
	OSObject              *object;

	IOLockLock(gIOInterruptControllersLock);
	while (1) {
		object = gIOInterruptControllers->getObject(name);

		if (object != NULL) {
			break;
		}

		IOLockSleep(gIOInterruptControllersLock,
		    gIOInterruptControllers, THREAD_UNINT);
	}

	IOLockUnlock(gIOInterruptControllersLock);
	return OSDynamicCast(IOInterruptController, object);
}


void
IOPlatformExpert::setCPUInterruptProperties(IOService *service)
{
	IOInterruptController *controller;

	OSDictionary *matching = serviceMatching("IOInterruptController");
	matching = propertyMatching(gPlatformInterruptControllerName, kOSBooleanTrue, matching);

	controller = OSDynamicCast(IOInterruptController, waitForService(matching));
	if (controller) {
		controller->setCPUInterruptProperties(service);
	}
}

bool
IOPlatformExpert::atInterruptLevel(void)
{
	return ml_at_interrupt_context();
}

bool
IOPlatformExpert::platformAdjustService(IOService *           )
{
	return true;
}

void
IOPlatformExpert::getUTCTimeOfDay(clock_sec_t * secs, clock_nsec_t * nsecs)
{
	*secs = getGMTTimeOfDay();
	*nsecs = 0;
}

void
IOPlatformExpert::setUTCTimeOfDay(clock_sec_t secs, __unused clock_nsec_t nsecs)
{
	setGMTTimeOfDay(secs);
}


                                                                                   
        
  
                                                                                   

void
IOPlatformExpert::
PMLog(const char *who, unsigned long event,
    unsigned long param1, unsigned long param2)
{
	clock_sec_t nows;
	clock_usec_t nowus;
	clock_get_system_microtime(&nows, &nowus);
	nowus += (nows % 1000) * 1000000;

	kprintf("pm%u %p %.30s %d %lx %lx\n",
	    nowus, OBFUSCATE(current_thread()), who,                       
	    (int) event, (long)OBFUSCATE(param1), (long)OBFUSCATE(param2));                            
}


                                                                                   
                            
  
                                                                       
                                                                  
                                                                       
                                   
                                                                                   

void
IOPlatformExpert::PMInstantiatePowerDomains( void )
{
	root = new IOPMrootDomain;
	root->init();
	root->attach(this);
	root->start(this);
}


                                                                                   
                   
  
                                                                                          
                                                                                               
                                                                                   

void
IOPlatformExpert::PMRegisterDevice(IOService * theNub, IOService * theDevice)
{
	root->addPowerChild( theDevice );
}

                                                                                   
               
  
                                                                                   

bool
IOPlatformExpert::hasPMFeature(unsigned long featureMask)
{
	return (_pePMFeatures & featureMask) != 0;
}

                                                                                   
                   
  
                                                                                   

bool
IOPlatformExpert::hasPrivPMFeature(unsigned long privFeatureMask)
{
	return (_pePrivPMFeatures & privFeatureMask) != 0;
}

                                                                                   
                        
  
                                                                                   

int
IOPlatformExpert::numBatteriesSupported(void)
{
	return _peNumBatteriesSupported;
}

                                                                                   
               
  
                                                                              
                                                                                
                                                                                 
                                                                                   
                                                                                  
                                    
                                                                                   

bool
IOPlatformExpert::CheckSubTree(OSArray * inSubTree, IOService * theNub, IOService * theDevice, OSDictionary * theParent)
{
	unsigned int    i;
	unsigned int    numPowerTreeNodes;
	OSDictionary *  entry;
	OSDictionary *  matchingDictionary;
	OSDictionary *  providerDictionary;
	OSDictionary *  deviceDictionary;
	OSDictionary *  nubDictionary;
	OSArray *       children;
	bool            nodeFound            = false;
	bool            continueSearch       = false;
	bool            deviceMatch          = false;
	bool            providerMatch        = false;
	bool            multiParentMatch     = false;

	if ((NULL == theDevice) || (NULL == inSubTree)) {
		return false;
	}

	numPowerTreeNodes = inSubTree->getCount();

	                                                                

	for (i = 0; i < numPowerTreeNodes; i++) {
		entry =  (OSDictionary *) inSubTree->getObject(i);

		matchingDictionary = (OSDictionary *) entry->getObject("device");
		providerDictionary = (OSDictionary *) entry->getObject("provider");

		deviceMatch = true;                                                                       
		if (matchingDictionary) {
			deviceMatch = false;
			if (NULL != (deviceDictionary = theDevice->dictionaryWithProperties())) {
				deviceMatch = deviceDictionary->isEqualTo( matchingDictionary, matchingDictionary );
				deviceDictionary->release();
			}
		}

		providerMatch = true;                                                      
		if (theNub && providerDictionary) {
			providerMatch = false;
			if (NULL != (nubDictionary = theNub->dictionaryWithProperties())) {
				providerMatch = nubDictionary->isEqualTo( providerDictionary, providerDictionary );
				nubDictionary->release();
			}
		}

		multiParentMatch = true;                                                              
		if (deviceMatch && providerMatch) {
			if (NULL != multipleParentKeyValue) {
				OSNumber * aNumber = (OSNumber *) entry->getObject("multiple-parent");
				multiParentMatch   = (NULL != aNumber) ? multipleParentKeyValue->isEqualTo(aNumber) : false;
			}
		}

		nodeFound = (deviceMatch && providerMatch && multiParentMatch);

		                                                                  
		                                             

		if (theNub == NULL && providerDictionary != NULL) {
			nodeFound = false;
		}

		                                                     

		if (nodeFound) {
			if (RegisterServiceInTree(theDevice, entry, theParent, theNub)) {
				if (kIOLogPower & gIOKitDebug) {
					IOLog("PMRegisterDevice/CheckSubTree - service registered!\n");
				}

				numInstancesRegistered++;

				                                                                    
				multipleParentKeyValue = (OSNumber *) entry->getObject("multiple-parent");
			} else {
				nodeFound = false;
			}
		}

		continueSearch = ((false == nodeFound) || (NULL != multipleParentKeyValue));

		if (continueSearch && (NULL != (children = (OSArray *) entry->getObject("children")))) {
			nodeFound = CheckSubTree( children, theNub, theDevice, entry );
			continueSearch = ((false == nodeFound) || (NULL != multipleParentKeyValue));
		}

		if (false == continueSearch) {
			break;
		}
	}

	return nodeFound;
}

                                                                                   
                        
  
                                                             
                                                                                   

bool
IOPlatformExpert::RegisterServiceInTree(IOService * theService, OSDictionary * theTreeNode, OSDictionary * theTreeParentNode, IOService * theProvider)
{
	IOService *    aService;
	bool           registered = false;
	OSArray *      children;
	unsigned int   numChildren;
	OSDictionary * child;

	                                                   

	if (NULL == theTreeNode->getObject("service")) {
		if (theTreeNode->setObject("service", OSDynamicCast( OSObject, theService))) {
			                                 

			                                                                    
			                                                    

			if (NULL != (children = (OSArray *) theTreeNode->getObject("children"))) {
				numChildren = children->getCount();
				for (unsigned int i = 0; i < numChildren; i++) {
					if (NULL != (child = (OSDictionary *) children->getObject(i))) {
						if (NULL != (aService = (IOService *) child->getObject("service"))) {
							theService->addPowerChild(aService);
						}
					}
				}
			}

			                                 

			                                                                       
			                                

			if (theTreeParentNode) {
				if (NULL != (aService = (IOService *) theTreeParentNode->getObject("service"))) {
					if (aService != theProvider) {
						aService->addPowerChild(theService);
					}
				}
			}

			registered = true;
		}
	}

	return registered;
}

                                                                                   
                      
  
                                                                 
                                                                                   
void
printDictionaryKeys(OSDictionary * inDictionary, char * inMsg)
{
	OSCollectionIterator * mcoll = OSCollectionIterator::withCollection(inDictionary);
	OSSymbol * mkey;
	OSString * ioClass;
	unsigned int i = 0;

	mcoll->reset();

	mkey = OSDynamicCast(OSSymbol, mcoll->getNextObject());

	while (mkey) {
		                                                                       

		                                                  

		if (mkey->isEqualTo("IOClass")) {
			ioClass = (OSString *) inDictionary->getObject("IOClass");
			if (ioClass) {
				IOLog("%s IOClass is %s\n", inMsg, ioClass->getCStringNoCopy());
			}
		}

		                                             

		if (mkey->isEqualTo("IOProviderClass")) {
			ioClass = (OSString *) inDictionary->getObject("IOProviderClass");
			if (ioClass) {
				IOLog("%s IOProviderClass is %s\n", inMsg, ioClass->getCStringNoCopy());
			}
		}

		                              
		if (mkey->isEqualTo("IONameMatch")) {
			ioClass = (OSString *) inDictionary->getObject("IONameMatch");
			if (ioClass) {
				IOLog("%s IONameMatch is %s\n", inMsg, ioClass->getCStringNoCopy());
			}
		}

		                                

		if (mkey->isEqualTo("IONameMatched")) {
			ioClass = (OSString *) inDictionary->getObject("IONameMatched");
			if (ioClass) {
				IOLog("%s IONameMatched is %s\n", inMsg, ioClass->getCStringNoCopy());
			}
		}

#if 0
		                 

		if (mkey->isEqualTo("AAPL,clock-id")) {
			char * cstr;
			cstr = getCStringForObject(inDictionary->getObject("AAPL,clock-id"));
			if (cstr) {
				kprintf(" ===> AAPL,clock-id is %s\n", cstr );
			}
		}
#endif

		             

		if (mkey->isEqualTo("name")) {
			char nameStr[64];
			nameStr[0] = 0;
			getCStringForObject(inDictionary->getObject("name"), nameStr,
			    sizeof(nameStr));
			if (strlen(nameStr) > 0) {
				IOLog("%s name is %s\n", inMsg, nameStr);
			}
		}

		mkey = (OSSymbol *) mcoll->getNextObject();

		i++;
	}

	mcoll->release();
}

static void
getCStringForObject(OSObject *inObj, char *outStr, size_t outStrLen)
{
	char * buffer;
	unsigned int    len, i;

	if ((NULL == inObj) || (NULL == outStr)) {
		return;
	}

	char * objString = (char *) (inObj->getMetaClass())->getClassName();

	if ((0 == strncmp(objString, "OSString", sizeof("OSString"))) ||
	    (0 == strncmp(objString, "OSSymbol", sizeof("OSSymbol")))) {
		strlcpy(outStr, ((OSString *)inObj)->getCStringNoCopy(), outStrLen);
	} else if (0 == strncmp(objString, "OSData", sizeof("OSData"))) {
		len = ((OSData *)inObj)->getLength();
		buffer = (char *)((OSData *)inObj)->getBytesNoCopy();
		if (buffer && (len > 0)) {
			for (i = 0; i < len; i++) {
				outStr[i] = buffer[i];
			}
			outStr[len] = 0;
		}
	}
}

                                  
                                                   
   
#if !defined(__x86_64)
__abortlike
#endif
static void
IOShutdownNotificationsTimedOut(
	thread_call_param_t p0,
	thread_call_param_t p1)
{
#if !defined(__x86_64__)
	                                    
	panic("Halt/Restart Timed Out");

#else                           
	int type = (int)(long)p0;
	uint32_t timeout = (uint32_t)(uintptr_t)p1;

	IOPMrootDomain *pmRootDomain = IOService::getPMRootDomain();
	if (pmRootDomain) {
		if ((PEGetCoprocessorVersion() >= kCoprocessorVersion2) || pmRootDomain->checkShutdownTimeout()) {
			pmRootDomain->panicWithShutdownLog(timeout * 1000);
		}
	}

	                                              
	if (gIOPlatform) {
		gIOPlatform->haltRestart(type);
	}
#endif                          
}


extern "C" {
  
                                             
   

  
                                                                  
                                                                      
                              
   
boolean_t
PEGetMachineName( char * name, int maxLength )
{
	if (gIOPlatform) {
		return gIOPlatform->getMachineName( name, maxLength );
	} else {
		return false;
	}
}

  
                                                                  
                                                                      
                              
   
boolean_t
PEGetModelName( char * name, int maxLength )
{
	if (gIOPlatform) {
		return gIOPlatform->getModelName( name, maxLength );
	} else {
		return false;
	}
}

boolean_t
PEGetTargetName( char * name, int maxLength )
{
	if (gIOPlatform) {
		return gIOPlatform->getTargetName( name, maxLength );
	} else {
		return false;
	}
}

boolean_t
PEGetProductName( char * name, int maxLength )
{
	if (gIOPlatform) {
		return gIOPlatform->getProductName( name, maxLength );
	} else {
		return false;
	}
}

int
PEGetPlatformEpoch(void)
{
	if (gIOPlatform) {
		return (int) gIOPlatform->getBootROMType();
	} else {
		return -1;
	}
}

                                                               
void
PEInitiatePanic(void)
{
#if defined(__arm64__)
	  
                                                                     
                                                                           
    
	flush_mmu_tlb_entry(0);
#endif
}

int
PEHaltRestartInternal(unsigned int type, uint32_t details)
{
	IOPMrootDomain    *pmRootDomain;
	AbsoluteTime      deadline;
	thread_call_t     shutdown_hang;
	IORegistryEntry   *node;
	OSData            *data;
	uint32_t          timeout = kShutdownTimeout;
	static boolean_t  panic_begin_called = FALSE;

	if (type == kPEHaltCPU || type == kPERestartCPU || type == kPEUPSDelayHaltCPU) {
		                                                                              
                                                                          
     
		if (panic_begin_called) {
			goto skip_to_haltRestart;
		}

		pmRootDomain = IOService::getPMRootDomain();
		                                              
                                                      
                                   
     

		                                                
                                                          
                                                        
                                                    
     
#if  defined(__arm__) || defined(__arm64__)
#define RESTART_NODE_PATH    "/defaults"
#else
#define RESTART_NODE_PATH    "/chosen"
#endif
		node = IORegistryEntry::fromPath( RESTART_NODE_PATH, gIODTPlane );
		if (node) {
			data = OSDynamicCast( OSData, node->getProperty( "halt-restart-timeout" ));
			if (data && data->getLength() == 4) {
				timeout = *((uint32_t *) data->getBytesNoCopy());
			}
		}

#if (DEVELOPMENT || DEBUG)
		                                                 
		uint32_t boot_arg_val;
		if (PE_parse_boot_argn("halt_restart_timeout", &boot_arg_val, sizeof(boot_arg_val))) {
			timeout = boot_arg_val;
		}
#endif

		if (timeout) {
			shutdown_hang = thread_call_allocate( &IOShutdownNotificationsTimedOut,
			    (thread_call_param_t)(uintptr_t) type);
			clock_interval_to_deadline( timeout, kSecondScale, &deadline );
			thread_call_enter1_delayed( shutdown_hang, (thread_call_param_t)(uintptr_t)timeout, deadline );
		}

		pmRootDomain->handlePlatformHaltRestart(type);
		                                                       
                               
    
                                                           
                                                                 
                                                                    
              
     
	} else if (type == kPEPanicRestartCPU || type == kPEPanicSync || type == kPEPanicRestartCPUNoCallouts) {
		if (type == kPEPanicRestartCPU) {
			                                                  
			                                                      
#if defined(__x86_64__)
			if (coprocessor_cross_panic_enabled)
#endif
			IOCPURunPlatformPanicActions(kPEPanicEnd, details);
		} else if (type == kPEPanicRestartCPUNoCallouts) {
			                                                 
			                                                         
			type = kPEPanicRestartCPU;
		}


		                                                              
		                                                                  
		                                                          
		                                            
		PE_sync_panic_buffers();
		IOCPURunPlatformPanicActions(type, details);
		PE_sync_panic_buffers();
	} else if (type == kPEPanicEnd) {
#if defined(__x86_64__)
		if (coprocessor_cross_panic_enabled)
#endif
		IOCPURunPlatformPanicActions(type, details);
	} else if (type == kPEPanicBegin) {
#if defined(__x86_64__)
		if (coprocessor_cross_panic_enabled)
#endif
		{
			                                           
			if (!panic_begin_called) {
				panic_begin_called = TRUE;
				IOCPURunPlatformPanicActions(type, details);
			}
		}
	} else if (type == kPEPanicDiagnosticsDone) {
		IOCPURunPlatformPanicActions(type, details);
	}

skip_to_haltRestart:
	if (gIOPlatform) {
		return gIOPlatform->haltRestart(type);
	} else {
		return -1;
	}
}

int
PEHaltRestart(unsigned int type)
{
	return PEHaltRestartInternal(type, 0);
}

UInt32
PESavePanicInfo(UInt8 *buffer, UInt32 length)
{
	if (gIOPlatform != NULL) {
		return (UInt32) gIOPlatform->savePanicInfo(buffer, length);
	} else {
		return 0;
	}
}

void
PESavePanicInfoAction(void *buffer, UInt32 offset, UInt32 length)
{
	IOCPURunPlatformPanicSyncAction(buffer, offset, length);
	return;
}


  
                                                                  
                                                                    
                                                                       
                                                                      
   
inline static int
init_gIOOptionsEntry(void)
{
	IORegistryEntry *entry;
	void *nvram_entry;
	volatile void **options;
	int ret = -1;

	if (gIOOptionsEntry) {
		return 0;
	}

	entry = IORegistryEntry::fromPath( "/options", gIODTPlane );
	if (!entry) {
		return -1;
	}

	nvram_entry = (void *) OSDynamicCast(IODTNVRAM, entry);
	if (!nvram_entry) {
		goto release;
	}

	options = (volatile void **) &gIOOptionsEntry;
	if (!OSCompareAndSwapPtr(NULL, nvram_entry, options)) {
		ret = 0;
		goto release;
	}

	return 0;

release:
	entry->release();
	return ret;
}

                                                                 
boolean_t
PEReadNVRAMProperty(const char *symbol, void *value,
    unsigned int *len)
{
	OSObject  *obj;
	OSData *data;
	unsigned int vlen;

	if (!symbol || !len) {
		goto err;
	}

	if (init_gIOOptionsEntry() < 0) {
		goto err;
	}

	vlen = *len;
	*len = 0;

	obj = gIOOptionsEntry->getProperty(symbol);
	if (!obj) {
		goto err;
	}

	                     
	data = OSDynamicCast(OSData, obj);
	if (!data) {
		goto err;
	}

	*len  = data->getLength();
	vlen  = min(vlen, *len);
	if (value && vlen) {
		memcpy((void *) value, data->getBytesNoCopy(), vlen);
	}

	return TRUE;

err:
	return FALSE;
}

boolean_t
PEWriteNVRAMBooleanProperty(const char *symbol, boolean_t value)
{
	const OSSymbol *sym = NULL;
	OSBoolean *data = NULL;
	bool ret = false;

	if (symbol == NULL) {
		goto exit;
	}

	if (init_gIOOptionsEntry() < 0) {
		goto exit;
	}

	if ((sym = OSSymbol::withCStringNoCopy(symbol)) == NULL) {
		goto exit;
	}

	data  = value ? kOSBooleanTrue : kOSBooleanFalse;
	ret = gIOOptionsEntry->setProperty(sym, data);

	sym->release();

	                                              
	if (ret == true) {
		gIOOptionsEntry->sync();
	}

exit:
	return ret;
}

static boolean_t
PEWriteNVRAMPropertyInternal(const char *symbol, boolean_t copySymbol, const void *value,
    const unsigned int len)
{
	const OSSymbol *sym;
	OSData *data;
	bool ret = false;

	if (!symbol || !value || !len) {
		goto err;
	}

	if (init_gIOOptionsEntry() < 0) {
		goto err;
	}

	if (copySymbol == TRUE) {
		sym = OSSymbol::withCString(symbol);
	} else {
		sym = OSSymbol::withCStringNoCopy(symbol);
	}

	if (!sym) {
		goto err;
	}

	data = OSData::withBytes((void *) value, len);
	if (!data) {
		goto sym_done;
	}

	ret = gIOOptionsEntry->setProperty(sym, data);
	data->release();

sym_done:
	sym->release();

	if (ret == true) {
		gIOOptionsEntry->sync();
		return TRUE;
	}

err:
	return FALSE;
}

boolean_t
PEWriteNVRAMProperty(const char *symbol, const void *value,
    const unsigned int len)
{
	return PEWriteNVRAMPropertyInternal(symbol, FALSE, value, len);
}

boolean_t
PEWriteNVRAMPropertyWithCopy(const char *symbol, const void *value,
    const unsigned int len)
{
	return PEWriteNVRAMPropertyInternal(symbol, TRUE, value, len);
}

boolean_t
PERemoveNVRAMProperty(const char *symbol)
{
	const OSSymbol *sym;

	if (!symbol) {
		goto err;
	}

	if (init_gIOOptionsEntry() < 0) {
		goto err;
	}

	sym = OSSymbol::withCStringNoCopy(symbol);
	if (!sym) {
		goto err;
	}

	gIOOptionsEntry->removeProperty(sym);

	sym->release();

	gIOOptionsEntry->sync();
	return TRUE;

err:
	return FALSE;
}

long
PEGetGMTTimeOfDay(void)
{
	clock_sec_t     secs;
	clock_usec_t    usecs;

	PEGetUTCTimeOfDay(&secs, &usecs);
	return secs;
}

void
PESetGMTTimeOfDay(long secs)
{
	PESetUTCTimeOfDay(secs, 0);
}

void
PEGetUTCTimeOfDay(clock_sec_t * secs, clock_usec_t * usecs)
{
	clock_nsec_t    nsecs = 0;

	*secs = 0;
	if (gIOPlatform) {
		gIOPlatform->getUTCTimeOfDay(secs, &nsecs);
	}

	assert(nsecs < NSEC_PER_SEC);
	*usecs = nsecs / NSEC_PER_USEC;
}

void
PESetUTCTimeOfDay(clock_sec_t secs, clock_usec_t usecs)
{
	assert(usecs < USEC_PER_SEC);
	if (gIOPlatform) {
		gIOPlatform->setUTCTimeOfDay(secs, usecs * NSEC_PER_USEC);
	}
}

coprocessor_type_t
PEGetCoprocessorVersion( void )
{
	coprocessor_type_t coprocessor_version = kCoprocessorVersionNone;
#if defined(__x86_64__)
	IORegistryEntry     *platform_entry = NULL;
	OSData              *coprocessor_version_obj = NULL;

	platform_entry = IORegistryEntry::fromPath(kIODeviceTreePlane ":/efi/platform");
	if (platform_entry != NULL) {
		coprocessor_version_obj = OSDynamicCast(OSData, platform_entry->getProperty("apple-coprocessor-version"));
		if ((coprocessor_version_obj != NULL) && (coprocessor_version_obj->getLength() <= sizeof(uint64_t))) {
			memcpy(&coprocessor_version, coprocessor_version_obj->getBytesNoCopy(), coprocessor_version_obj->getLength());
		}
		platform_entry->release();
	}
#endif
	return coprocessor_version;
}
}                 

bool gIOPlatformUUIDAndSerialDone = false;

void
IOPlatformExpert::publishPlatformUUIDAndSerial( void )
{
	if (!gIOPlatformUUIDAndSerialDone) {
		                                                                  
		if (NULL == getProvider()->getProperty(kIOPlatformSerialNumberKey)) {
			OSData* mydata = (OSData*) (getProvider()->getProperty("serial-number"));
			if (mydata != NULL) {
				OSString *serNoString = createSystemSerialNumberString(mydata);
				if (serNoString != NULL) {
					getProvider()->setProperty(kIOPlatformSerialNumberKey, serNoString);
					serNoString->release();
				}
			}
		}
		IOPlatformExpertDevice *provider = OSDynamicCast(IOPlatformExpertDevice, getProvider());
		assert(provider != NULL);
		provider->generatePlatformUUID();
	}

	if (gIOPlatformUUIDAndSerialDone) {
		publishResource(kIOPlatformUUIDKey, getProvider()->getProperty(kIOPlatformUUIDKey));
	}
}

void
IOPlatformExpert::publishNVRAM( void )
{
	if (init_gIOOptionsEntry() < 0) {
		IOPlatformExpertDevice *provider = OSDynamicCast(IOPlatformExpertDevice, getProvider());
		assert(provider != NULL);
		provider->createNVRAM();
	}
	if (gIOOptionsEntry != NULL) {
		gIOOptionsEntry->registerService();
	}
}

void
IOPlatformExpert::registerNVRAMController(IONVRAMController * caller)
{
#if defined(__x86_64__)
	OSData *          data;
	IORegistryEntry * entry;

	  
                                                                    
                                                                          
                                                                       
    
	if (panicDebugging) {
		entry = IORegistryEntry::fromPath( "/options", gIODTPlane );
		if (entry) {
			data = OSDynamicCast( OSData, entry->getProperty( APPLE_VENDOR_VARIABLE_GUID":BridgeOSPanicWatchdogEnabled" ));
			if (data && (data->getLength() == sizeof(UInt8))) {
				UInt8 *panicWatchdogEnabled = (UInt8 *) data->getBytesNoCopy();
				UInt32 debug_flags = 0;
				if (*panicWatchdogEnabled || (PE_i_can_has_debugger(&debug_flags) &&
				    (debug_flags & DB_DISABLE_CROSS_PANIC))) {
					coprocessor_cross_panic_enabled = FALSE;
				}
			}
			entry->release();
		}
	}

#if (DEVELOPMENT || DEBUG)
	entry = IORegistryEntry::fromPath( "/options", gIODTPlane );
	if (entry) {
		data = OSDynamicCast( OSData, entry->getProperty(nvram_osenvironment));
		if (data) {
			sysctl_set_osenvironment(data->getLength(), data->getBytesNoCopy());
			entry->removeProperty(nvram_osenvironment);
			IODTNVRAM * nvramOptionsEntry = OSDynamicCast(IODTNVRAM, entry);
			if (nvramOptionsEntry) {
				nvramOptionsEntry->sync();
			}
		}
		entry->release();
	}
	sysctl_unblock_osenvironment();
#endif
	                                                                  
	publishPlatformUUIDAndSerial();

#endif                          

	publishResource("IONVRAM");
}

IOReturn
IOPlatformExpert::callPlatformFunction(const OSSymbol *functionName,
    bool waitForFunction,
    void *param1, void *param2,
    void *param3, void *param4)
{
	IOService *service, *_resources;

	if (functionName == gIOPlatformQuiesceActionKey ||
	    functionName == gIOPlatformActiveActionKey) {
		  
                                                                                 
                                                                                         
     
		if (gEnforceQuiesceSafety) {
			panic("Class %s passed the quiesce/active action to IOPlatformExpert",
			    getMetaClass()->getClassName());
		}
	}

	if (waitForFunction) {
		_resources = waitForService(resourceMatching(functionName));
	} else {
		_resources = getResourceService();
	}
	if (_resources == NULL) {
		return kIOReturnUnsupported;
	}

	service = OSDynamicCast(IOService, _resources->getProperty(functionName));
	if (service == NULL) {
		return kIOReturnUnsupported;
	}

	return service->callPlatformFunction(functionName, waitForFunction,
	           param1, param2, param3, param4);
}

IOByteCount
IOPlatformExpert::savePanicInfo(UInt8 *buffer, IOByteCount length)
{
	return 0;
}

                                                                           

#undef super
#define super IOPlatformExpert

OSDefineMetaClassAndAbstractStructors( IODTPlatformExpert, IOPlatformExpert )

OSMetaClassDefineReservedUnused(IODTPlatformExpert, 0);
OSMetaClassDefineReservedUnused(IODTPlatformExpert, 1);
OSMetaClassDefineReservedUnused(IODTPlatformExpert, 2);
OSMetaClassDefineReservedUnused(IODTPlatformExpert, 3);
OSMetaClassDefineReservedUnused(IODTPlatformExpert, 4);
OSMetaClassDefineReservedUnused(IODTPlatformExpert, 5);
OSMetaClassDefineReservedUnused(IODTPlatformExpert, 6);
OSMetaClassDefineReservedUnused(IODTPlatformExpert, 7);

                                                                           

IOService *
IODTPlatformExpert::probe( IOService * provider,
    SInt32 * score )
{
	if (!super::probe( provider, score)) {
		return NULL;
	}

	                      
	if (!provider->compareNames( getProperty( gIONameMatchKey ))) {
		return NULL;
	}

	return this;
}

bool
IODTPlatformExpert::configure( IOService * provider )
{
	if (!super::configure( provider)) {
		return false;
	}

	processTopLevel( provider );

	return true;
}

IOService *
IODTPlatformExpert::createNub( IORegistryEntry * from )
{
	IOService *         nub;

	nub = new IOPlatformDevice;
	if (nub) {
		if (!nub->init( from, gIODTPlane )) {
			nub->free();
			nub = NULL;
		}
	}
	return nub;
}

bool
IODTPlatformExpert::createNubs( IOService * parent, OSIterator * iter )
{
	IORegistryEntry *   next;
	IOService *         nub;
	bool                ok = true;

	if (iter) {
		while ((next = (IORegistryEntry *) iter->getNextObject())) {
			if (NULL == (nub = createNub( next ))) {
				continue;
			}

			nub->attach( parent );
#if !defined(__x86_64__)
			OSData *tmpData = (OSData *)next->getProperty("device_type");
			if (tmpData == NULL) {
				nub->registerService();
				continue;
			}

			char *device_type = (char *)tmpData->getBytesNoCopy();
			if (strcmp(device_type, "cpu") != 0) {
				nub->registerService();
				continue;
			}

			tmpData = (OSData *)next->getProperty("reg");
			assert(tmpData != NULL);
			assert(tmpData->getLength() >= sizeof(UInt32));

			uint32_t phys_id = *(UInt32 *)tmpData->getBytesNoCopy();
			int logical_cpu_id = ml_get_cpu_number(phys_id);
			int logical_cluster_id = ml_get_cluster_number(phys_id);

			  
                                                                                         
                                                                                           
                                                                                           
     
                                                                                                
                                              
      
			if (logical_cpu_id < 0) {
				nub->registerService();
				continue;
			}

			__assert_only bool logical_id_added_to_ioreg = nub->setProperty("logical-cpu-id", logical_cpu_id, 32U);
			assert(logical_id_added_to_ioreg == true);
			logical_id_added_to_ioreg = nub->setProperty("logical-cluster-id", logical_cluster_id, 32U);
			assert(logical_id_added_to_ioreg == true);
#endif
			nub->registerService();
		}
		iter->release();
	}

	return ok;
}

void
IODTPlatformExpert::processTopLevel( IORegistryEntry * rootEntry )
{
	OSIterator *        kids;
	IORegistryEntry *   next;
	IORegistryEntry *   cpus;

	              
	kids = IODTFindMatchingEntries( rootEntry, 0, deleteList());
	if (kids) {
		while ((next = (IORegistryEntry *)kids->getNextObject())) {
			next->detachAll( gIODTPlane);
		}
		kids->release();
	}

	publishNVRAM();
	assert(gIOOptionsEntry != NULL);                                                                               
	dtNVRAM = gIOOptionsEntry;

	                    
	cpus = rootEntry->childFromPath( "cpus", gIODTPlane);
	if (cpus) {
		createNubs( this, IODTFindMatchingEntries( cpus, kIODTExclusive, NULL));
		cpus->release();
	}

	                                       
	createNubs( this, IODTFindMatchingEntries( rootEntry, kIODTExclusive, excludeList()));
}

IOReturn
IODTPlatformExpert::getNubResources( IOService * nub )
{
	if (nub->getDeviceMemory()) {
		return kIOReturnSuccess;
	}

	IODTResolveAddressing( nub, "reg", NULL);

	return kIOReturnSuccess;
}

bool
IODTPlatformExpert::compareNubName( const IOService * nub,
    OSString * name, OSString ** matched ) const
{
	return IODTCompareNubName( nub, name, matched )
	       || super::compareNubName( nub, name, matched);
}


  
                                                                   
                                                     
  
                                                                   
  
                     
                          
  
                      
                          
   

bool
IODTPlatformExpert::getModelName( char * name, int maxLength )
{
	OSData *            prop;
	const char *        str;
	int                 len;
	char                c;
	bool                ok = false;

	maxLength--;

	prop = (OSData *) getProvider()->getProperty( gIODTCompatibleKey );
	if (prop) {
		str = (const char *) prop->getBytesNoCopy();

		if (0 == strncmp( str, "AAPL,", strlen( "AAPL," ))) {
			str += strlen( "AAPL," );
		}

		len = 0;
		while ((c = *str++)) {
			if ((c == '/') || (c == ' ')) {
				c = '-';
			}

			name[len++] = c;
			if (len >= maxLength) {
				break;
			}
		}

		name[len] = 0;
		ok = true;
	}
	return ok;
}

  
                                                                   
                                                     
  
                                                                   
  
                     
                          
  
                      
                          
   

bool
IODTPlatformExpert::getMachineName( char * name, int maxLength )
{
	OSData *            prop;
	bool                ok = false;

	maxLength--;
	prop = (OSData *) getProvider()->getProperty( gIODTModelKey );
	ok = (NULL != prop);

	if (ok) {
		strlcpy( name, (const char *) prop->getBytesNoCopy(), maxLength );
	}

	return ok;
}

                                  

bool
IODTPlatformExpert::getTargetName( char * name, int maxLength )
{
#if __x86_64__
	OSData *            prop;

	const OSSymbol *        key = gIODTBridgeModelKey;

	maxLength--;
	prop = (OSData *) getProvider()->getProperty( key );

	if (prop == NULL) {
		                                      
		char const * const  unknown = "";

		strlcpy( name, unknown, maxLength );
	} else {
		strlcpy( name, (const char *)prop->getBytesNoCopy(), maxLength );
	}

	return true;
#else
	return getModelName( name, maxLength );
#endif
}

                                         

bool
IODTPlatformExpert::getProductName( char * name, int maxLength )
{
#if __x86_64__
	return getModelName( name, maxLength );
#else
	return getMachineName( name, maxLength );
#endif
}

                                                                           

void
IODTPlatformExpert::registerNVRAMController( IONVRAMController * nvram )
{
	if (dtNVRAM) {
		dtNVRAM->registerNVRAMController(nvram);
	}

	super::registerNVRAMController(nvram);
}

int
IODTPlatformExpert::haltRestart(unsigned int type)
{
	if (dtNVRAM) {
		dtNVRAM->sync();
	}

	return super::haltRestart(type);
}

IOReturn
IODTPlatformExpert::readXPRAM(IOByteCount offset, UInt8 * buffer,
    IOByteCount length)
{
	if (dtNVRAM) {
		return dtNVRAM->readXPRAM(offset, buffer, length);
	} else {
		return kIOReturnNotReady;
	}
}

IOReturn
IODTPlatformExpert::writeXPRAM(IOByteCount offset, UInt8 * buffer,
    IOByteCount length)
{
	if (dtNVRAM) {
		return dtNVRAM->writeXPRAM(offset, buffer, length);
	} else {
		return kIOReturnNotReady;
	}
}

IOReturn
IODTPlatformExpert::readNVRAMProperty(
	IORegistryEntry * entry,
	const OSSymbol ** name, OSData ** value )
{
	if (dtNVRAM) {
		return dtNVRAM->readNVRAMProperty(entry, name, value);
	} else {
		return kIOReturnNotReady;
	}
}

IOReturn
IODTPlatformExpert::readNVRAMProperty(
	IORegistryEntry * entry,
	OSSharedPtr<const OSSymbol>& name, OSSharedPtr<OSData>& value )
{
	const OSSymbol* nameRaw = NULL;
	OSData* valueRaw = NULL;

	IOReturn result = readNVRAMProperty(entry, &nameRaw, &valueRaw);

	name.reset(nameRaw, OSNoRetain);
	value.reset(valueRaw, OSNoRetain);

	return result;
}

IOReturn
IODTPlatformExpert::writeNVRAMProperty(
	IORegistryEntry * entry,
	const OSSymbol * name, OSData * value )
{
	if (dtNVRAM) {
		return dtNVRAM->writeNVRAMProperty(entry, name, value);
	} else {
		return kIOReturnNotReady;
	}
}

OSDictionary *
IODTPlatformExpert::getNVRAMPartitions(void)
{
	if (dtNVRAM) {
		return dtNVRAM->getNVRAMPartitions();
	} else {
		return NULL;
	}
}

IOReturn
IODTPlatformExpert::readNVRAMPartition(const OSSymbol * partitionID,
    IOByteCount offset, UInt8 * buffer,
    IOByteCount length)
{
	if (dtNVRAM) {
		return dtNVRAM->readNVRAMPartition(partitionID, offset,
		           buffer, length);
	} else {
		return kIOReturnNotReady;
	}
}

IOReturn
IODTPlatformExpert::writeNVRAMPartition(const OSSymbol * partitionID,
    IOByteCount offset, UInt8 * buffer,
    IOByteCount length)
{
	if (dtNVRAM) {
		return dtNVRAM->writeNVRAMPartition(partitionID, offset,
		           buffer, length);
	} else {
		return kIOReturnNotReady;
	}
}

IOByteCount
IODTPlatformExpert::savePanicInfo(UInt8 *buffer, IOByteCount length)
{
	IOByteCount lengthSaved = 0;

	if (dtNVRAM) {
		lengthSaved = dtNVRAM->savePanicInfo(buffer, length);
	}

	if (lengthSaved == 0) {
		lengthSaved = super::savePanicInfo(buffer, length);
	}

	return lengthSaved;
}

OSString*
IODTPlatformExpert::createSystemSerialNumberString(OSData* myProperty)
{
	UInt8* serialNumber;
	unsigned int serialNumberSize;
	unsigned short pos = 0;
	char* temp;
	char SerialNo[30];

	if (myProperty != NULL) {
		serialNumberSize = myProperty->getLength();
		serialNumber = (UInt8*)(myProperty->getBytesNoCopy());
		temp = (char*)serialNumber;
		if (serialNumberSize > 0) {
			                                                 
			while (pos < serialNumberSize && temp[pos] != '-') {
				pos++;
			}

			if (pos < serialNumberSize) {                                                   
				memcpy(SerialNo, serialNumber + 12, 8);
				memcpy(&SerialNo[8], serialNumber, 3);
				SerialNo[11] = '-';
				memcpy(&SerialNo[12], serialNumber + 3, 8);
				SerialNo[20] = 0;
			} else {                               
				memcpy(SerialNo, serialNumber + 13, 8);
				memcpy(&SerialNo[8], serialNumber, 3);
				SerialNo[11] = 0;
			}
			return OSString::withCString(SerialNo);
		}
	}
	return NULL;
}


                                                                           

#undef super
#define super IOService

OSDefineMetaClassAndStructors(IOPlatformExpertDevice, IOService)

OSMetaClassDefineReservedUnused(IOPlatformExpertDevice, 0);
OSMetaClassDefineReservedUnused(IOPlatformExpertDevice, 1);
OSMetaClassDefineReservedUnused(IOPlatformExpertDevice, 2);
OSMetaClassDefineReservedUnused(IOPlatformExpertDevice, 3);

                                                                           

bool
IOPlatformExpertDevice::compareName( OSString * name,
    OSString ** matched ) const
{
	return IODTCompareNubName( this, name, matched );
}

bool
IOPlatformExpertDevice::init(void *dtRoot)
{
	IORegistryEntry *   dt = NULL;
	bool                ok;

	if ((dtRoot != NULL) && (dt = IODeviceTreeAlloc(dtRoot))) {
		ok = super::init( dt, gIODTPlane );
	} else {
		ok = super::init();
	}

	if (!ok) {
		return false;
	}

	return true;
}

bool
IOPlatformExpertDevice::startIOServiceMatching(void)
{
	workLoop = IOWorkLoop::workLoop();
	if (!workLoop) {
		return false;
	}

	registerService();

	return true;
}

IOWorkLoop *
IOPlatformExpertDevice::getWorkLoop() const
{
	return workLoop;
}

IOReturn
IOPlatformExpertDevice::setProperties( OSObject * properties )
{
	return kIOReturnUnsupported;
}

IOReturn
IOPlatformExpertDevice::newUserClient( task_t owningTask, void * securityID,
    UInt32 type, OSDictionary * properties,
    IOUserClient ** handler )
{
	IOReturn            err = kIOReturnSuccess;
	IOUserClient *      newConnect = NULL;
	IOUserClient *      theConnect = NULL;

	switch (type) {
	case kIOKitDiagnosticsClientType:
		newConnect = IOKitDiagnosticsClient::withTask(owningTask);
		if (!newConnect) {
			err = kIOReturnNotPermitted;
		}
		break;
	case kIOKitUserServerClientType:
		newConnect = IOUserServer::withTask(owningTask);
		if (!newConnect) {
			err = kIOReturnNotPermitted;
		}
		break;
	default:
		err = kIOReturnBadArgument;
	}

	if (newConnect) {
		if ((false == newConnect->attach(this))
		    || (false == newConnect->start(this))) {
			newConnect->detach( this );
			newConnect->release();
			err = kIOReturnNotPermitted;
		} else {
			theConnect = newConnect;
		}
	}

	*handler = theConnect;
	return err;
}

void
IOPlatformExpertDevice::free()
{
	if (workLoop) {
		workLoop->release();
	}
}

void
IOPlatformExpertDevice::configureDefaults( void )
{
	createNVRAM();
	                                                                  
	OSData* mydata = (OSData*) (getProperty("serial-number"));
	if (mydata != NULL) {
		OSString *serNoString = OSString::withCString((const char *)mydata->getBytesNoCopy());
		if (serNoString != NULL) {
			setProperty(kIOPlatformSerialNumberKey, serNoString);
			serNoString->release();
		}
	}
	generatePlatformUUID();
}

void
IOPlatformExpertDevice::createNVRAM( void )
{
	  
                                                       
                                                               
                                                             
                                               
    
	IORegistryEntry *options = IORegistryEntry::fromPath("/options", gIODTPlane);
	if (options == NULL) {
		return;                               
	}

	assert(gIOOptionsEntry == NULL);
	gIOOptionsEntry = new IODTNVRAM;

	assert(gIOOptionsEntry != NULL);

	gIOOptionsEntry->init(options, gIODTPlane);

	gIOOptionsEntry->attach(this);
	options->release();
}

void
IOPlatformExpertDevice::generatePlatformUUID( void )
{
	IORegistryEntry * entry;
	OSString *        string = NULL;
	uuid_string_t     uuid;

#if !defined(__x86_64__)
	entry = IORegistryEntry::fromPath( "/chosen", gIODTPlane );
	if (entry) {
		OSData * data1;

		data1 = OSDynamicCast( OSData, entry->getProperty( "unique-chip-id" ));
		if (data1 && data1->getLength() == 8) {
			OSData * data2;

			data2 = OSDynamicCast( OSData, entry->getProperty( "chip-id" ));
			if (data2 && data2->getLength() == 4) {
				SHA1_CTX     context;
				uint8_t      digest[SHA_DIGEST_LENGTH];
				const uuid_t space = { 0xA6, 0xDD, 0x4C, 0xCB, 0xB5, 0xE8, 0x4A, 0xF5, 0xAC, 0xDD, 0xB6, 0xDC, 0x6A, 0x05, 0x42, 0xB8 };

				SHA1Init( &context );
				SHA1Update( &context, space, sizeof(space));
				SHA1Update( &context, data1->getBytesNoCopy(), data1->getLength());
				SHA1Update( &context, data2->getBytesNoCopy(), data2->getLength());
				SHA1Final( digest, &context );

				digest[6] = (digest[6] & 0x0F) | 0x50;
				digest[8] = (digest[8] & 0x3F) | 0x80;

				uuid_unparse( digest, uuid );
				string = OSString::withCString( uuid );
			}
		}

		entry->release();
	}
#else                           
	OSData * data;

	entry = IORegistryEntry::fromPath( "/efi/platform", gIODTPlane );
	if (entry) {
		data = OSDynamicCast( OSData, entry->getProperty( "system-id" ));
		if (data && data->getLength() == 16) {
			SHA1_CTX     context;
			uint8_t      digest[SHA_DIGEST_LENGTH];
			const uuid_t space = { 0x2A, 0x06, 0x19, 0x90, 0xD3, 0x8D, 0x44, 0x40, 0xA1, 0x39, 0xC4, 0x97, 0x70, 0x37, 0x65, 0xAC };

			SHA1Init( &context );
			SHA1Update( &context, space, sizeof(space));
			SHA1Update( &context, data->getBytesNoCopy(), data->getLength());
			SHA1Final( digest, &context );

			digest[6] = (digest[6] & 0x0F) | 0x50;
			digest[8] = (digest[8] & 0x3F) | 0x80;

			uuid_unparse( digest, uuid );
			string = OSString::withCString( uuid );
		}

		entry->release();
	}
	if (!string) {
		                                 
		entry = IORegistryEntry::fromPath( "/options", gIODTPlane );
		if (entry) {
			data = OSDynamicCast( OSData, entry->getProperty( "platform-uuid" ));
			if (data && data->getLength() == sizeof(uuid_t)) {
				uuid_unparse((uint8_t *) data->getBytesNoCopy(), uuid );
				string = OSString::withCString( uuid );
			}
			entry->release();
		}
	}
#endif                          

	if (string) {
		setProperty( kIOPlatformUUIDKey, string );
		gIOPlatformUUIDAndSerialDone = true;

		string->release();
	}
}
                                                                           

#undef super
#define super IOService

OSDefineMetaClassAndStructors(IOPlatformDevice, IOService)

OSMetaClassDefineReservedUnused(IOPlatformDevice, 0);
OSMetaClassDefineReservedUnused(IOPlatformDevice, 1);
OSMetaClassDefineReservedUnused(IOPlatformDevice, 2);
OSMetaClassDefineReservedUnused(IOPlatformDevice, 3);

                                                                           

bool
IOPlatformDevice::compareName( OSString * name,
    OSString ** matched ) const
{
	return ((IOPlatformExpert *)getProvider())->
	       compareNubName( this, name, matched );
}

IOService *
IOPlatformDevice::matchLocation( IOService *              )
{
	return this;
}

IOReturn
IOPlatformDevice::getResources( void )
{
	return ((IOPlatformExpert *)getProvider())->getNubResources( this );
}

                                                                           

                                                                      
                       
 
                                                                     
                                     
                                                                      

class IOPanicPlatform : IOPlatformExpert {
	OSDeclareDefaultStructors(IOPanicPlatform);

public:
	bool start(IOService * provider) APPLE_KEXT_OVERRIDE;
};


OSDefineMetaClassAndStructors(IOPanicPlatform, IOPlatformExpert);


bool
IOPanicPlatform::start(IOService * provider)
{
	const char * platform_name = "(unknown platform name)";

	if (provider) {
		platform_name = provider->getName();
	}

	panic("Unable to find driver for this platform: \"%s\".\n",
	    platform_name);

	return false;
}
