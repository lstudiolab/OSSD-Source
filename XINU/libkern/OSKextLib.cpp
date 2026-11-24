/*
 * Copyright (c) 2008-2016 Apple Inc. All rights reserved.
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
#include <libkern/OSKextLibPrivate.h>
#include <libkern/mkext.h>
};

#include <libkern/c++/OSContainers.h>
#include <libkern/c++/OSKext.h>
#include <libkern/OSKextLib.h>
#include <libkern/OSKextLibPrivate.h>

extern "C" {
#if PRAGMA_MARK
#pragma mark C-based kext interface (loading/loaded kexts only)
#endif
                                                                      
                                                                      
kern_return_t
OSKextLoadKextWithIdentifier(const char * bundle_id)
{
	return OSKext::loadKextWithIdentifier(bundle_id);
}

uint32_t OSKextGetLoadTagForIdentifier(const char * kextIdentifier);
                                                                      
                                                                      
uint32_t
OSKextGetLoadTagForIdentifier(const char * kextIdentifier)
{
	uint32_t result  = kOSKextInvalidLoadTag;
	OSKext * theKext = NULL;                

	if (!kextIdentifier) {
		goto finish;
	}

	theKext = OSKext::lookupKextWithIdentifier(kextIdentifier);
	if (theKext && theKext->isLoaded()) {
		result = theKext->getLoadTag();
	}
finish:
	if (theKext) {
		theKext->release();
	}
	return result;
}

                                                                      
                                                                      

                                                                             
                                                                 
                                                                      
                                                           
#ifndef __clang_analyzer__
OSReturn
OSKextRetainKextWithLoadTag(uint32_t loadTag)
{
	OSReturn   result = kOSKextReturnNotFound;
	OSKext   * theKext = NULL;                                               

	if (loadTag == kOSKextInvalidLoadTag) {
		result = kOSKextReturnInvalidArgument;
		goto finish;
	}
	theKext = OSKext::lookupKextWithLoadTag(loadTag);
	if (theKext) {
		result = kOSReturnSuccess;

		OSKextLog(theKext,
		    kOSKextLogDebugLevel |
		    kOSKextLogKextBookkeepingFlag,
		    "Kext %s (load tag %d) has been retained.",
		    theKext->getIdentifierCString(),
		    loadTag);

		                                                                  
     
		theKext->setAutounloadEnabled(true);
	} else {
		OSKextLog(theKext,
		    kOSKextLogErrorLevel |
		    kOSKextLogKextBookkeepingFlag,
		    "Can't retain kext with load tag %d - no such kext is loaded.",
		    loadTag);
	}
finish:
	return result;
}
#endif                      

                                                                      
                                                                      

                                                                             
                                                                
                                                                      
                                                           
#ifndef __clang_analyzer__
OSReturn
OSKextReleaseKextWithLoadTag(uint32_t loadTag)
{
	OSReturn result  = kOSKextReturnNotFound;
	OSKext * theKext = NULL;                       

	if (loadTag == kOSKextInvalidLoadTag) {
		result = kOSKextReturnInvalidArgument;
		goto finish;
	}
	theKext = OSKext::lookupKextWithLoadTag(loadTag);
	if (theKext) {
		result = kOSReturnSuccess;
		OSKext::considerUnloads();                            
		theKext->release();                                   
		theKext->release();                                    
		OSKextLog(theKext,
		    kOSKextLogDebugLevel |
		    kOSKextLogKextBookkeepingFlag,
		    "Kext %s (load tag %d) has been released.",
		    theKext->getIdentifierCString(),
		    loadTag);
	} else {
		OSKextLog(theKext,
		    kOSKextLogErrorLevel |
		    kOSKextLogKextBookkeepingFlag,
		    "Can't release kext with load tag %d - no such kext is loaded.",
		    loadTag);
	}

	                                                                                 
	                                                              
finish:
	return result;
}
#endif                      

                                                                      
                                              
                                                                      
OSReturn
OSKextUnloadKextWithLoadTag(uint32_t loadTag)
{
	return OSKext::removeKextWithLoadTag(loadTag,
	                                                             false);
}


#if PRAGMA_MARK
#pragma mark Kext Requests
#endif
                                                                      
               
                                                                      
OSReturn
OSKextRequestResource(
	const char                    * kextIdentifier,
	const char                    * resourceName,
	OSKextRequestResourceCallback   callback,
	void                          * context,
	OSKextRequestTag              * requestTagOut)
{
	return OSKext::requestResource(kextIdentifier, resourceName,
	           callback, context, requestTagOut);
}

                                                                      
                                                                      
OSReturn
OSKextCancelRequest(
	OSKextRequestTag    requestTag,
	void             ** contextOut)
{
	return OSKext::cancelRequest(requestTag, contextOut);
}

#if PRAGMA_MARK
#pragma mark MIG Functions & Wrappers
#endif
                                                                      
                                                              
                                                                     
                                                                   
                                                                
                                                                      
                              
                                                                      
kern_return_t
kext_request(
	host_priv_t                             hostPriv,
	              uint32_t                 clientLogSpec,
	              vm_offset_t              requestIn,
	              mach_msg_type_number_t   requestLengthIn,
	               vm_offset_t            * responseOut,
	               mach_msg_type_number_t * responseLengthOut,
	               vm_offset_t            * logDataOut,
	               mach_msg_type_number_t * logDataLengthOut,
	               kern_return_t          * op_result)
{
	kern_return_t     result          = KERN_FAILURE;
	vm_map_address_t  map_addr        = 0;                          
	char            * request         = NULL;                     

	mkext2_header   * mkextHeader     = NULL;                 
	bool              isMkext         = false;

	char            * response        = NULL;                 
	uint32_t          responseLength  = 0;
	char            * logData         = NULL;                 
	uint32_t          logDataLength   = 0;

	                                                                        
                                                         
    
	*op_result = KERN_FAILURE;
	*responseOut = 0;
	*responseLengthOut = 0;
	*logDataOut = 0;
	*logDataLengthOut = 0;

	                                                           
    
	if (!requestLengthIn || !requestIn) {
		OSKextLog(           NULL,
		    kOSKextLogErrorLevel |
		    kOSKextLogIPCFlag,
		    "Invalid request from user space (no data).");
		*op_result = KERN_INVALID_ARGUMENT;
		goto finish;
	}

	result = vm_map_copyout_size(kernel_map, &map_addr, (vm_map_copy_t)requestIn, requestLengthIn);
	if (result != KERN_SUCCESS) {
		OSKextLog(           NULL,
		    kOSKextLogErrorLevel |
		    kOSKextLogIPCFlag,
		    "vm_map_copyout() failed for request from user space.");
		  
                                                               
                                                                  
                             
     
		goto finish;
	}
	request = CAST_DOWN(char *, map_addr);

	                                                              
                                                               
                                                                 
                
    
	if (requestLengthIn > sizeof(mkext2_header)) {
		mkextHeader = (mkext2_header *)request;
		if (MKEXT_GET_MAGIC(mkextHeader) == MKEXT_MAGIC &&
		    MKEXT_GET_SIGNATURE(mkextHeader) == MKEXT_SIGN) {
			isMkext = true;
		}
	}

	if (isMkext) {
#if defined(SECURE_KERNEL) || !CONFIG_KXLD
		                                                                    
		                                        
		*op_result = KERN_NOT_SUPPORTED;
		goto finish;
#else
		                                                  
		                                                  
		if (hostPriv == HOST_PRIV_NULL) {
			OSKextLog(           NULL,
			    kOSKextLogErrorLevel |
			    kOSKextLogLoadFlag | kOSKextLogIPCFlag,
			    "Attempt by non-root process to load a kext.");
			*op_result = kOSKextReturnNotPrivileged;
			goto finish;
		}

		*op_result = OSKext::loadFromMkext((OSKextLogSpec)clientLogSpec,
		    request, requestLengthIn,
		    &logData, &logDataLength);

#endif                             
	} else {
		                                                                 
                                                     
     
		*op_result = OSKext::handleRequest(hostPriv,
		    (OSKextLogSpec)clientLogSpec,
		    request, requestLengthIn,
		    &response, &responseLength,
		    &logData, &logDataLength);
	}

	if (response && responseLength > 0) {
		kern_return_t copyin_result;

		copyin_result = vm_map_copyin(kernel_map,
		    CAST_USER_ADDR_T(response), responseLength,
		                      false, (vm_map_copy_t *)responseOut);
		if (copyin_result == KERN_SUCCESS) {
			*responseLengthOut = responseLength;
		} else {
			OSKextLog(           NULL,
			    kOSKextLogErrorLevel |
			    kOSKextLogIPCFlag,
			    "Failed to copy response to request from user space.");
			*op_result = copyin_result;                                        
			*responseOut = 0;
			*responseLengthOut = 0;
			goto finish;
		}
	}

	if (logData && logDataLength > 0) {
		kern_return_t copyin_result;

		copyin_result = vm_map_copyin(kernel_map,
		    CAST_USER_ADDR_T(logData), logDataLength,
		                      false, (vm_map_copy_t *)logDataOut);
		if (copyin_result == KERN_SUCCESS) {
			*logDataLengthOut = logDataLength;
		} else {
			OSKextLog(           NULL,
			    kOSKextLogErrorLevel |
			    kOSKextLogIPCFlag,
			    "Failed to copy log data for request from user space.");
			*op_result = copyin_result;                                        
			*logDataOut = 0;
			*logDataLengthOut = 0;
			goto finish;
		}
	}

finish:
	if (request) {
		(void)vm_deallocate(kernel_map, (vm_offset_t)request, requestLengthIn);
	}
	if (response) {
		                                                      
		kmem_free(kernel_map, (vm_offset_t)response, round_page(responseLength));
	}
	if (logData) {
		                                                      
		kmem_free(kernel_map, (vm_offset_t)logData, round_page(logDataLength));
	}

	return result;
}

                                                                      
                                      
                                                                      
extern vm_offset_t segPRELINKTEXTB;
extern vm_offset_t segLINKB;
extern unsigned long segSizePRELINKTEXT;
extern vm_map_t g_kext_map;

vm_map_t
kext_get_vm_map(kmod_info_t *info)
{
	vm_map_t kext_map = NULL;
	kc_format_t kcformat;

	if (PE_get_primary_kc_format(&kcformat) && kcformat == KCFormatFileset) {
		                                           
		assert(segLINKB >= (segPRELINKTEXTB + segSizePRELINKTEXT));
		if ((info->address >= segPRELINKTEXTB) &&
		    (info->address < segLINKB)) {
			kext_map = kernel_map;
		} else {
			kext_map = g_kext_map;
		}
	} else {
		if ((info->address >= segPRELINKTEXTB) &&
		    (info->address < (segPRELINKTEXTB + segSizePRELINKTEXT))) {
			kext_map = kernel_map;
		} else {
			kext_map = g_kext_map;
		}
	}

	return kext_map;
}


#if PRAGMA_MARK
                                                                      
#pragma mark Weak linking support
                                                                      
#endif
void
kext_weak_symbol_referenced(void)
{
	panic("A kext referenced an unresolved weak symbol\n");
}

const void * const gOSKextUnresolved = (const void *)&kext_weak_symbol_referenced;

#if PRAGMA_MARK
#pragma mark Kernel-Internal C Functions
#endif
                                                                      
                        
                                                                      
void
OSKextRemoveKextBootstrap(void)
{
	OSKext::removeKextBootstrap();
	return;
}

#if CONFIG_DTRACE
                                                                      
                                                                      
void
OSKextRegisterKextsWithDTrace(void)
{
	OSKext::registerKextsWithDTrace();
	return;
}
#endif                    

                                                                      
                                                                      
void
kext_dump_panic_lists(int (*printf_func)(const char * fmt, ...))
{
	OSKext::printKextPanicLists(printf_func);
	return;
}

#if PRAGMA_MARK
#pragma mark Kmod Compatibility Functions
#endif
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                      
                                                                   
                                                                    
                  
 
                                                                  
                                                        
                                                                      
void
kmod_panic_dump(vm_offset_t * addr, unsigned int cnt)
{
	extern int paniclog_append_noflush(const char *format, ...) __printflike(1, 2);

	OSKext::printKextsInBacktrace(addr, cnt, &paniclog_append_noflush, 0);

	return;
}

                                                                      
void kmod_dump_log(vm_offset_t *addr, unsigned int cnt, boolean_t doUnslide);

void
kmod_dump_log(
	vm_offset_t * addr,
	unsigned int cnt,
	boolean_t doUnslide)
{
	uint32_t flags = OSKext::kPrintKextsLock;
	if (doUnslide) {
		flags |= OSKext::kPrintKextsUnslide;
	}
	OSKext::printKextsInBacktrace(addr, cnt, &printf, flags);
}

void *
OSKextKextForAddress(const void *addr)
{
	return OSKext::kextForAddress(addr);
}


                                                                      
                                                                     
                                             
                                                                      

#if PRAGMA_MARK
#pragma mark Loaded Kext Summary
#endif

void
OSKextLoadedKextSummariesUpdated(void)
{
	              
}
};
