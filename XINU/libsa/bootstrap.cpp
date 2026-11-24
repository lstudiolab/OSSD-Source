/*
 * Copyright (c) 2000-2012 Apple Inc. All rights reserved.
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
extern "C" {
#include <mach/kmod.h>
#include <libkern/kernel_mach_header.h>
#include <libkern/prelink.h>
#include <libkern/crypto/sha2.h>
}

#define IOKIT_ENABLE_SHARED_PTR

#include <libkern/version.h>
#include <libkern/c++/OSContainers.h>
#include <libkern/OSKextLibPrivate.h>
#include <libkern/c++/OSKext.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOCatalogue.h>

#if __x86_64__
#define KASLR_KEXT_DEBUG 0
#endif

#if PRAGMA_MARK
#pragma mark Bootstrap Declarations
#endif
                                                                      
                        
 
                                                                   
                                                                  
                                                            
 
                                                                 
                                                              
                                                 
 
                                                                 
                                                                  
                    
                                                                      
extern "C" {
extern void (*record_startup_extensions_function)(void);
extern void (*load_security_extensions_function)(void);
};

static void bootstrapRecordStartupExtensions(void);
static void bootstrapLoadSecurityExtensions(void);


#if NO_KEXTD
extern "C" bool IORamDiskBSDRoot(void);
#endif

#if PRAGMA_MARK
#pragma mark Macros
#endif
                                                                      
        
                                                                      
#define CONST_STRLEN(str) (sizeof(str) - 1)

#if PRAGMA_MARK
#pragma mark Kernel Component Kext Identifiers
#endif
                                                                      
                                   
 
                                                                 
                                                                 
                                                                  
                                           
                                                                      
static const char * sKernelComponentNames[] = {
	                                                                    
	"com.apple.kernel",
	"com.apple.kpi.bsd",
	"com.apple.kpi.dsep",
	"com.apple.kpi.iokit",
	"com.apple.kpi.kasan",
	"com.apple.kpi.libkern",
	"com.apple.kpi.mach",
	"com.apple.kpi.private",
	"com.apple.kpi.unsupported",
	"com.apple.iokit.IONVRAMFamily",
	"com.apple.driver.AppleNMI",
	"com.apple.iokit.IOSystemManagementFamily",
	"com.apple.iokit.ApplePlatformFamily",
	NULL
};

#if PRAGMA_MARK
#pragma mark KLDBootstrap Class
#endif
                                                                      
                    
 
                                                                  
                                                                  
                                                          
                                                                      
class KLDBootstrap {
	friend void bootstrapRecordStartupExtensions(void);
	friend void bootstrapLoadSecurityExtensions(void);

private:
	void readStartupExtensions(void);

	void readPrelinkedExtensions(kernel_mach_header_t *mh, kc_kind_t type);
	void readBooterExtensions(void);

	OSReturn loadKernelComponentKexts(void);
	void     loadKernelExternalComponents(void);
	void     readBuiltinPersonalities(void);

	void     loadSecurityExtensions(void);

public:
	KLDBootstrap(void);
	~KLDBootstrap(void);
};

LIBKERN_ALWAYS_DESTROY static KLDBootstrap sBootstrapObject;

                                                                      
                                                                   
                                                 
                                                                      
KLDBootstrap::KLDBootstrap(void)
{
	if (this != &sBootstrapObject) {
		panic("Attempt to access bootstrap segment.");
	}
	record_startup_extensions_function = &bootstrapRecordStartupExtensions;
	load_security_extensions_function = &bootstrapLoadSecurityExtensions;
}

                                                                      
                                                                     
                                                
                                                                      
KLDBootstrap::~KLDBootstrap(void)
{
	if (this != &sBootstrapObject) {
		panic("Attempt to access bootstrap segment.");
	}


	record_startup_extensions_function = NULL;
	load_security_extensions_function = NULL;
}

                                                                      
                                                                      
void
KLDBootstrap::readStartupExtensions(void)
{
	kernel_section_t * prelinkInfoSect = NULL;               

	OSKextLog(           NULL,
	    kOSKextLogProgressLevel |
	    kOSKextLogGeneralFlag | kOSKextLogDirectoryScanFlag |
	    kOSKextLogKextBookkeepingFlag,
	    "Reading startup extensions.");

	kc_format_t kc_format;
	kernel_mach_header_t *mh = &_mh_execute_header;
	if (PE_get_primary_kc_format(&kc_format) && kc_format == KCFormatFileset) {
		mh = (kernel_mach_header_t *)PE_get_kc_header(KCKindPrimary);
	}

	                                                                   
                                                          
                                                                      
                  
    
	prelinkInfoSect = getsectbynamefromheader(mh, kPrelinkInfoSegment, kPrelinkInfoSection);
	if (prelinkInfoSect->size) {
		readPrelinkedExtensions(mh, KCKindPrimary);
	} else {
		readBooterExtensions();
	}

	kernel_mach_header_t *akc_mh;
	akc_mh = (kernel_mach_header_t*)PE_get_kc_header(KCKindAuxiliary);
	if (akc_mh) {
		readPrelinkedExtensions(akc_mh, KCKindAuxiliary);
	}

	loadKernelComponentKexts();
	loadKernelExternalComponents();
	readBuiltinPersonalities();
	OSKext::sendAllKextPersonalitiesToCatalog(true);

	return;
}

                                                                      
                                                                      
void
KLDBootstrap::readPrelinkedExtensions(kernel_mach_header_t *mh, kc_kind_t type)
{
	bool ret;
	OSSharedPtr<OSData> loaded_kcUUID;
	OSSharedPtr<OSString> errorString;
	OSSharedPtr<OSObject> parsedXML;
	kernel_section_t *infoPlistSection = NULL;
	OSDictionary *infoDict = NULL;                          

	OSKextLog(           NULL,
	    kOSKextLogProgressLevel |
	    kOSKextLogDirectoryScanFlag | kOSKextLogArchiveFlag,
	    "Starting from prelinked kernel.");

	  
                                                                 
                                                                      
                                                                    
                                
    
	infoPlistSection = getsectbynamefromheader(mh, kPrelinkInfoSegment, kPrelinkInfoSection);
	parsedXML = OSUnserializeXML((const char *)infoPlistSection->addr, errorString);
	if (parsedXML) {
		infoDict = OSDynamicCast(OSDictionary, parsedXML.get());
	}

	if (!infoDict) {
		const char *errorCString = "(unknown error)";

		if (errorString && errorString->getCStringNoCopy()) {
			errorCString = errorString->getCStringNoCopy();
		} else if (parsedXML) {
			errorCString = "not a dictionary";
		}
		OSKextLog(           NULL, kOSKextLogErrorLevel | kOSKextLogArchiveFlag,
		    "Error unserializing kext info plist section: %s.", errorCString);
		return;
	}

	                                                                     
	if (type == KCKindAuxiliary) {
		if (OSKext::validateKCFileSetUUID(infoDict, KCKindAuxiliary) != 0) {
			OSKextLog(           NULL, kOSKextLogErrorLevel | kOSKextLogArchiveFlag,
			    "Early boot AuxKC  doesn't appear to be linked against the loaded BootKC.");
			return;
		}

		  
                                                        
                                                              
                 
     
		if (!OSKext::registerDeferredKextCollection(mh, parsedXML, KCKindAuxiliary)) {
			OSKextLog(           NULL, kOSKextLogErrorLevel | kOSKextLogArchiveFlag,
			    "Error deferring AuxKC kext processing: Kexts in this collection will be unusable.");
		}
		goto skip_adding_kexts;
	}

	  
                                                                     
                                             
    
	ret = OSKext::addKextsFromKextCollection(mh, infoDict,
	    kPrelinkTextSegment, loaded_kcUUID, (mh->filetype == MH_FILESET) ? type : KCKindUnknown);

	if (!ret) {
		OSKextLog(           NULL, kOSKextLogErrorLevel | kOSKextLogArchiveFlag,
		    "Error loading kext info from prelinked primary KC");
		return;
	}

	                                  
	if (!loaded_kcUUID) {
		OSKextLog(           NULL, kOSKextLogErrorLevel | kOSKextLogArchiveFlag,
		    "WARNING: did not find UUID in %s KC!", (type == KCKindAuxiliary) ? "Aux" : "Primary");
	} else if (type != KCKindAuxiliary) {
		kernelcache_uuid_valid = TRUE;
		memcpy((void *)&kernelcache_uuid, (const void *)loaded_kcUUID->getBytesNoCopy(), loaded_kcUUID->getLength());
		uuid_unparse_upper(kernelcache_uuid, kernelcache_uuid_string);
	} else {
		auxkc_uuid_valid = TRUE;
		memcpy((void *)&auxkc_uuid, (const void *)loaded_kcUUID->getBytesNoCopy(), loaded_kcUUID->getLength());
		uuid_unparse_upper(auxkc_uuid, auxkc_uuid_string);
	}

skip_adding_kexts:
#if CONFIG_KEXT_BASEMENT
	if (mh->filetype != MH_FILESET) {
		  
                                                                
                                                              
                                                              
                           
     
		kernel_segment_command_t *prelinkTextSegment = NULL;
		prelinkTextSegment = getsegbyname(kPrelinkTextSegment);
		if (!prelinkTextSegment) {
			OSKextLog(           NULL,
			    kOSKextLogErrorLevel | kOSKextLogArchiveFlag,
			    "Can't find prelinked kexts' text segment.");
			return;
		}

		ml_static_mfree((vm_offset_t)prelinkTextSegment->vmaddr, prelinkTextSegment->vmsize);
	}
#endif                           

	  
                                                      
    
	kernel_segment_command_t *prelinkInfoSegment = NULL;
	prelinkInfoSegment = getsegbyname(kPrelinkInfoSegment);
	if (prelinkInfoSegment) {
		ml_static_mfree((vm_offset_t)prelinkInfoSegment->vmaddr,
		    (vm_size_t)prelinkInfoSegment->vmsize);
	}

	return;
}


                                                                      
                                                                      
#define BOOTER_KEXT_PREFIX   "Driver-"

typedef struct _DeviceTreeBuffer {
	uint32_t paddr;
	uint32_t length;
} _DeviceTreeBuffer;

void
KLDBootstrap::readBooterExtensions(void)
{
	OSSharedPtr<IORegistryEntry> booterMemoryMap;
	OSSharedPtr<OSDictionary>    propertyDict;
	OSSharedPtr<OSCollectionIterator>      keyIterator;
	OSString                  * deviceTreeName          = NULL;                 

	const _DeviceTreeBuffer   * deviceTreeBuffer        = NULL;              
	char                      * booterDataPtr           = NULL;              
	OSSharedPtr<OSData>         booterData;
	OSSharedPtr<OSKext>         aKext;

	OSKextLog(           NULL,
	    kOSKextLogProgressLevel |
	    kOSKextLogDirectoryScanFlag | kOSKextLogKextBookkeepingFlag,
	    "Reading startup extensions from booter memory.");

	booterMemoryMap = IORegistryEntry::fromPath( "/chosen/memory-map", gIODTPlane);

	if (!booterMemoryMap) {
		OSKextLog(           NULL,
		    kOSKextLogErrorLevel |
		    kOSKextLogGeneralFlag | kOSKextLogDirectoryScanFlag,
		    "Can't read booter memory map.");
		goto finish;
	}

	propertyDict = booterMemoryMap->dictionaryWithProperties();
	if (!propertyDict) {
		OSKextLog(           NULL,
		    kOSKextLogErrorLevel |
		    kOSKextLogDirectoryScanFlag,
		    "Can't get property dictionary from memory map.");
		goto finish;
	}

	keyIterator = OSCollectionIterator::withCollection(propertyDict.get());
	if (!keyIterator) {
		OSKextLog(           NULL,
		    kOSKextLogErrorLevel |
		    kOSKextLogGeneralFlag,
		    "Can't allocate iterator for driver images.");
		goto finish;
	}

	                                      
    
#ifndef CONFIG_EMBEDDED
	OSKext::createExcludeListFromBooterData(propertyDict.get(), keyIterator.get());
#endif
	                                         
	keyIterator->reset();

	while ((deviceTreeName =
	    OSDynamicCast(OSString, keyIterator->getNextObject()))) {
		const char * devTreeNameCString = deviceTreeName->getCStringNoCopy();
		OSData * deviceTreeEntry = OSDynamicCast(OSData,
		    propertyDict->getObject(deviceTreeName));

		                                                                  
		if (!deviceTreeEntry) {
			continue;
		}

		                            
		if (strncmp(devTreeNameCString,
		    BOOTER_KEXT_PREFIX,
		    CONST_STRLEN(BOOTER_KEXT_PREFIX))) {
			continue;
		}

		deviceTreeBuffer = (const _DeviceTreeBuffer *)
		    deviceTreeEntry->getBytesNoCopy(0, sizeof(deviceTreeBuffer));
		if (!deviceTreeBuffer) {
			                                                     
                                                            
      
			OSKextLog(           NULL,
			    kOSKextLogErrorLevel |
			    kOSKextLogDirectoryScanFlag,
			    "Device tree entry %s has NULL pointer.",
			    devTreeNameCString);
			goto finish;                          
		}

		booterDataPtr = (char *)ml_static_ptovirt(deviceTreeBuffer->paddr);
		if (!booterDataPtr) {
			OSKextLog(           NULL,
			    kOSKextLogErrorLevel |
			    kOSKextLogDirectoryScanFlag,
			    "Can't get virtual address for device tree entry %s.",
			    devTreeNameCString);
			goto finish;
		}

		                                                                      
                                                                       
                                                                        
                                                   
     
		booterData = OSData::withBytesNoCopy(booterDataPtr,
		    deviceTreeBuffer->length);
		if (!booterData) {
			OSKextLog(           NULL,
			    kOSKextLogErrorLevel |
			    kOSKextLogGeneralFlag,
			    "Error - Can't allocate OSData wrapper for device tree entry %s.",
			    devTreeNameCString);
			goto finish;
		}
		booterData->setDeallocFunction(osdata_phys_free);

		                                                              
                                                            
                                                                  
     
		OSSharedPtr<OSKext> newKext = OSKext::withBooterData(deviceTreeName, booterData.get());

		booterMemoryMap->removeProperty(deviceTreeName);
	}                                                                 

finish:
	return;
}

                                                                      
                                                                      
#define COM_APPLE  "com.apple."

void
KLDBootstrap::loadSecurityExtensions(void)
{
	OSSharedPtr<OSDictionary>         extensionsDict;
	OSSharedPtr<OSCollectionIterator> keyIterator;
	OSString             * bundleID       = NULL;                
	OSKext               * theKext        = NULL;                

	OSKextLog(           NULL,
	    kOSKextLogStepLevel |
	    kOSKextLogLoadFlag,
	    "Loading security extensions.");

	extensionsDict = OSKext::copyKexts();
	if (!extensionsDict) {
		return;
	}

	keyIterator = OSCollectionIterator::withCollection(extensionsDict.get());
	if (!keyIterator) {
		OSKextLog(           NULL,
		    kOSKextLogErrorLevel |
		    kOSKextLogGeneralFlag,
		    "Failed to allocate iterator for security extensions.");
		goto finish;
	}

	while ((bundleID = OSDynamicCast(OSString, keyIterator->getNextObject()))) {
		const char * bundle_id = bundleID->getCStringNoCopy();

		                                                                  
     
		if (!bundle_id ||
		    (strncmp(bundle_id, COM_APPLE, CONST_STRLEN(COM_APPLE)) != 0)) {
			continue;
		}

		theKext = OSDynamicCast(OSKext, extensionsDict->getObject(bundleID));
		if (!theKext) {
			continue;
		}

		if (kOSBooleanTrue == theKext->getPropertyForHostArch(kAppleSecurityExtensionKey)) {
			OSKextLog(           NULL,
			    kOSKextLogStepLevel |
			    kOSKextLogLoadFlag,
			    "Loading security extension %s.", bundleID->getCStringNoCopy());
			OSKext::loadKextWithIdentifier(bundleID->getCStringNoCopy(),
			                     false);
		}
	}

finish:
	return;
}

                                                                      
                                                                
                                                                   
                                                       
 
                                                                
                                                                     
 
                                                                
                                                  
                                                                      
OSReturn
KLDBootstrap::loadKernelComponentKexts(void)
{
	OSReturn            result      = kOSReturnSuccess;             
	OSSharedPtr<OSKext> theKext;
	const char       ** kextIDPtr   = NULL;                           

	for (kextIDPtr = &sKernelComponentNames[0]; *kextIDPtr; kextIDPtr++) {
		theKext = OSKext::lookupKextWithIdentifier(*kextIDPtr);

		if (theKext) {
			if (kOSReturnSuccess != OSKext::loadKextWithIdentifier(
				    *kextIDPtr,                  false)) {
				                                                  
				OSKextLog(           NULL,
				    kOSKextLogErrorLevel |
				    kOSKextLogDirectoryScanFlag | kOSKextLogKextBookkeepingFlag,
				    "Failed to initialize kernel component %s.", *kextIDPtr);
				result = kOSReturnError;
			}
		}
	}

	return result;
}

                                                                      
                                                                  
                                                                    
                                                                         
                                                                
                                       
                                                                      

#define COM_APPLE_KEC  "com.apple.kec."

void
KLDBootstrap::loadKernelExternalComponents(void)
{
	OSSharedPtr<OSDictionary>         extensionsDict;
	OSSharedPtr<OSCollectionIterator> keyIterator;
	OSString             * bundleID       = NULL;                
	OSKext               * theKext        = NULL;                
	OSBoolean            * isKernelExternalComponent = NULL;                

	OSKextLog(           NULL,
	    kOSKextLogStepLevel |
	    kOSKextLogLoadFlag,
	    "Loading Kernel External Components.");

	extensionsDict = OSKext::copyKexts();
	if (!extensionsDict) {
		return;
	}

	keyIterator = OSCollectionIterator::withCollection(extensionsDict.get());
	if (!keyIterator) {
		OSKextLog(           NULL,
		    kOSKextLogErrorLevel |
		    kOSKextLogGeneralFlag,
		    "Failed to allocate iterator for Kernel External Components.");
		goto finish;
	}

	while ((bundleID = OSDynamicCast(OSString, keyIterator->getNextObject()))) {
		const char * bundle_id = bundleID->getCStringNoCopy();

		                                                                      
     
		if (!bundle_id ||
		    (strncmp(bundle_id, COM_APPLE_KEC, CONST_STRLEN(COM_APPLE_KEC)) != 0)) {
			continue;
		}

		theKext = OSDynamicCast(OSKext, extensionsDict->getObject(bundleID));
		if (!theKext) {
			continue;
		}

		isKernelExternalComponent = OSDynamicCast(OSBoolean,
		    theKext->getPropertyForHostArch(kAppleKernelExternalComponentKey));
		if (isKernelExternalComponent && isKernelExternalComponent->isTrue()) {
			OSKextLog(           NULL,
			    kOSKextLogStepLevel |
			    kOSKextLogLoadFlag,
			    "Loading kernel external component %s.", bundleID->getCStringNoCopy());
			OSKext::loadKextWithIdentifier(bundleID->getCStringNoCopy(),
			                     false);
		}
	}

finish:
	return;
}

                                                                      
                                                                      
void
KLDBootstrap::readBuiltinPersonalities(void)
{
	OSSharedPtr<OSObject>   parsedXML;
	OSArray               * builtinExtensions     = NULL;                 
	OSSharedPtr<OSArray>    allPersonalities;
	OSSharedPtr<OSString>   errorString;
	kernel_section_t      * infosect              = NULL;              
	OSSharedPtr<OSCollectionIterator>  personalitiesIterator;
	unsigned int            count, i;

	OSKextLog(           NULL,
	    kOSKextLogStepLevel |
	    kOSKextLogLoadFlag,
	    "Reading built-in kernel personalities for I/O Kit drivers.");

	                                                                  
                                                                    
                                                             
                                                                  
                                                                       
                          
    
	infosect   = getsectbyname("__BUILTIN", "__info");
	if (!infosect) {
		                   
		goto finish;
	}

	parsedXML = OSUnserializeXML((const char *) (uintptr_t)infosect->addr,
	    errorString);
	if (parsedXML) {
		builtinExtensions = OSDynamicCast(OSArray, parsedXML.get());
	}
	if (!builtinExtensions) {
		const char * errorCString = "(unknown error)";

		if (errorString && errorString->getCStringNoCopy()) {
			errorCString = errorString->getCStringNoCopy();
		} else if (parsedXML) {
			errorCString = "not an array";
		}
		OSKextLog(           NULL,
		    kOSKextLogErrorLevel |
		    kOSKextLogLoadFlag,
		    "Error unserializing built-in personalities: %s.", errorCString);
		goto finish;
	}

	                                               
	count = builtinExtensions->getCount();
	allPersonalities = OSArray::withCapacity(count * 3);

	for (i = 0; i < count; i++) {
		OSDictionary            * infoDict = NULL;                 
		OSString                * moduleName = NULL;                 
		OSDictionary            * personalities;                 
		OSString                * personalityName;                 

		infoDict = OSDynamicCast(OSDictionary,
		    builtinExtensions->getObject(i));
		if (!infoDict) {
			continue;
		}

		moduleName = OSDynamicCast(OSString,
		    infoDict->getObject(kCFBundleIdentifierKey));
		if (!moduleName) {
			continue;
		}

		OSKextLog(           NULL,
		    kOSKextLogStepLevel |
		    kOSKextLogLoadFlag,
		    "Adding personalities for built-in driver %s:",
		    moduleName->getCStringNoCopy());

		personalities = OSDynamicCast(OSDictionary,
		    infoDict->getObject("IOKitPersonalities"));
		if (!personalities) {
			continue;
		}

		personalitiesIterator = OSCollectionIterator::withCollection(personalities);
		if (!personalitiesIterator) {
			continue;                                                       
		}

		while ((personalityName = OSDynamicCast(OSString,
		    personalitiesIterator->getNextObject()))) {
			OSDictionary * personality = OSDynamicCast(OSDictionary,
			    personalities->getObject(personalityName));

			OSKextLog(           NULL,
			    kOSKextLogDetailLevel |
			    kOSKextLogLoadFlag,
			    "Adding built-in driver personality %s.",
			    personalityName->getCStringNoCopy());

			if (personality && !personality->getObject(kCFBundleIdentifierKey)) {
				personality->setObject(kCFBundleIdentifierKey, moduleName);
			}
			allPersonalities->setObject(personality);
		}
	}

	gIOCatalogue->addDrivers(allPersonalities.get(), false);

finish:
	return;
}

#if PRAGMA_MARK
#pragma mark Bootstrap Functions
#endif
                                                                      
                     
                                                                      
static void
bootstrapRecordStartupExtensions(void)
{
	sBootstrapObject.readStartupExtensions();
	return;
}

static void
bootstrapLoadSecurityExtensions(void)
{
	sBootstrapObject.loadSecurityExtensions();
	return;
}

