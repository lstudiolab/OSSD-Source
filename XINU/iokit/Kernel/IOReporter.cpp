/*
 * Copyright (c) 2012-2013 Apple Computer, Inc.  All Rights Reserved.
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

#include <IOKit/IOKernelReportStructs.h>
#include <IOKit/IOKernelReporters.h>
#include "IOReporterDefs.h"

#include <string.h>
#include <IOKit/IORegistryEntry.h>

#define super OSObject
OSDefineMetaClassAndStructors(IOReporter, OSObject);

static OSSharedPtr<const OSSymbol> gIOReportNoChannelName;

                                                                      

                                        
                                        
                                        
IOReturn
IOReporter::configureAllReports(OSSet *reporters,
    IOReportChannelList *channelList,
    IOReportConfigureAction action,
    void *result,
    void *destination)
{
	IOReturn rval = kIOReturnError;
	OSSharedPtr<OSCollectionIterator> iterator;

	if (reporters == NULL || channelList == NULL || result == NULL) {
		rval = kIOReturnBadArgument;
		goto finish;
	}

	switch (action) {
	case kIOReportGetDimensions:
	case kIOReportEnable:
	case kIOReportDisable:
	{
		OSObject * object;
		iterator = OSCollectionIterator::withCollection(reporters);

		while ((object = iterator->getNextObject())) {
			IOReporter *rep = OSDynamicCast(IOReporter, object);

			if (rep) {
				(void)rep->configureReport(channelList, action, result, destination);
			} else {
				rval = kIOReturnUnsupported;                      
				goto finish;
			}
		}

		break;
	}

	case kIOReportTraceOnChange:
	case kIOReportNotifyHubOnChange:
	default:
		rval = kIOReturnUnsupported;
		goto finish;
	}

	rval = kIOReturnSuccess;

finish:
	return rval;
}

                                                                               
IOReturn
IOReporter::updateAllReports(OSSet *reporters,
    IOReportChannelList *channelList,
    IOReportConfigureAction action,
    void *result,
    void *destination)
{
	IOReturn rval = kIOReturnError;
	OSSharedPtr<OSCollectionIterator> iterator;

	if (reporters == NULL ||
	    channelList == NULL ||
	    result == NULL ||
	    destination == NULL) {
		rval = kIOReturnBadArgument;
		goto finish;
	}

	switch (action) {
	case kIOReportCopyChannelData:
	{
		OSObject * object;
		iterator = OSCollectionIterator::withCollection(reporters);

		while ((object = iterator->getNextObject())) {
			IOReporter *rep = OSDynamicCast(IOReporter, object);

			if (rep) {
				(void)rep->updateReport(channelList, action, result, destination);
			} else {
				rval = kIOReturnUnsupported;                      
				goto finish;
			}
		}

		break;
	}

	case kIOReportTraceChannelData:
	default:
		rval = kIOReturnUnsupported;
		goto finish;
	}

	rval = kIOReturnSuccess;

finish:
	return rval;
}


                                        
                                        
                                        

bool
IOReporter::init(IOService *reportingService,
    IOReportChannelType channelType,
    IOReportUnit unit)
{
	bool success = false;

	                                             
	_reporterLock = NULL;
	_configLock = NULL;
	_elements = NULL;
	_enableCounts = NULL;
	_channelNames = nullptr;

	if (channelType.report_format == kIOReportInvalidFormat) {
		IORLOG("init ERROR: Channel Type ill-defined");
		goto finish;
	}

	_driver_id = reportingService->getRegistryEntryID();
	if (_driver_id == 0) {
		IORLOG("init() ERROR: no registry ID");
		goto finish;
	}

	if (!super::init()) {
		return false;
	}

	if (channelType.nelements > INT16_MAX) {
		return false;
	}
	_channelDimension = channelType.nelements;
	_channelType = channelType;
	                                     
	if (unit == kIOReportUnitHWTicks) {
#if defined(__arm__) || defined(__arm64__)
		unit = kIOReportUnit24MHzTicks;
#elif defined(__i386__) || defined(__x86_64__)
		                                  
		unit = kIOReportUnit1GHzTicks;
#else
#error kIOReportUnitHWTicks not defined
#endif
	}
	_unit = unit;

	                                  
	_reporterLock = IOSimpleLockAlloc();
	if (!_reporterLock) {
		goto finish;
	}
	_reporterIsLocked = false;

	                         
	_configLock = IOLockAlloc();
	if (!_configLock) {
		goto finish;
	}
	_reporterConfigIsLocked = false;

	                               
	_channelNames = OSArray::withCapacity(1);
	if (!_channelNames) {
		goto finish;
	}

	          
	success = true;

finish:
	return success;
}


                                 
                                 
                                 

void
IOReporter::initialize(void)
{
	gIOReportNoChannelName = OSSymbol::withCString("_NO_NAME_4");
}

                                                             
                               
void
IOReporter::free(void)
{
	if (_configLock) {
		IOLockFree(_configLock);
	}
	if (_reporterLock) {
		IOSimpleLockFree(_reporterLock);
	}

	if (_elements) {
		PREFL_MEMOP_PANIC(_nElements, IOReportElement);
		IOFree(_elements, (size_t)_nElements * sizeof(IOReportElement));
	}
	if (_enableCounts) {
		PREFL_MEMOP_PANIC(_nChannels, int);
		IOFree(_enableCounts, (size_t)_nChannels * sizeof(int));
	}

	super::free();
}

  
                           
                                  
                                  
                                  
                                                                       
                                                                 
                
   
IOReturn
IOReporter::addChannel(uint64_t channelID,
    const char *channelName             )
{
	IOReturn res = kIOReturnError, kerr;
	OSSharedPtr<const OSSymbol> symChannelName;
	int oldNChannels, newNChannels = 0, freeNChannels = 0;

	IORLOG("IOReporter::addChannel %llx", channelID);

	                                                
	lockReporterConfig();

	                                                                  

	                                       
	oldNChannels = _nChannels;
	if (oldNChannels < 0 || oldNChannels > INT_MAX - 1) {
		res = kIOReturnOverrun;
		goto finish;
	}
	newNChannels = oldNChannels + 1;
	freeNChannels = newNChannels;                        

	                                              
	if (_channelNames->ensureCapacity((unsigned)newNChannels) <
	    (unsigned)newNChannels) {
		res = kIOReturnNoMemory; goto finish;
	}
	if (channelName) {
		symChannelName = OSSymbol::withCString(channelName);
		if (!symChannelName) {
			res = kIOReturnNoMemory; goto finish;
		}
	} else {
		                                        
		symChannelName = gIOReportNoChannelName;
	}

	                                             
	if ((kerr = handleSwapPrepare(newNChannels))) {
		                                       
		res = kerr; goto finish;
	}

	                                                                  
	                                                                     
	lockReporter();
	res = handleAddChannelSwap(channelID, symChannelName.get());
	unlockReporter();
	                                                                     
	                                                                      
	if (res == kIOReturnSuccess) {
		freeNChannels = oldNChannels;
	}

finish:
	                                                 
	handleSwapCleanup(freeNChannels);

	unlockReporterConfig();

	return res;
}


OSSharedPtr<IOReportLegendEntry>
IOReporter::createLegend(void)
{
	OSSharedPtr<IOReportLegendEntry> legendEntry;

	lockReporterConfig();

	legendEntry = handleCreateLegend();

	unlockReporterConfig();

	return legendEntry;
}


IOReturn
IOReporter::configureReport(IOReportChannelList *channelList,
    IOReportConfigureAction action,
    void *result,
    void *destination)
{
	IOReturn res = kIOReturnError;

	lockReporterConfig();

	res = handleConfigureReport(channelList, action, result, destination);

	unlockReporterConfig();

	return res;
}


IOReturn
IOReporter::updateReport(IOReportChannelList *channelList,
    IOReportConfigureAction action,
    void *result,
    void *destination)
{
	IOReturn res = kIOReturnError;

	lockReporter();

	res = handleUpdateReport(channelList, action, result, destination);

	unlockReporter();

	return res;
}


                                 
                                 
                                 


void
IOReporter::lockReporter()
{
	_interruptState = IOSimpleLockLockDisableInterrupt(_reporterLock);
	_reporterIsLocked = true;
}


void
IOReporter::unlockReporter()
{
	_reporterIsLocked = false;
	IOSimpleLockUnlockEnableInterrupt(_reporterLock, _interruptState);
}

void
IOReporter::lockReporterConfig()
{
	IOLockLock(_configLock);
	_reporterConfigIsLocked = true;
}

void
IOReporter::unlockReporterConfig()
{
	_reporterConfigIsLocked = false;
	IOLockUnlock(_configLock);
}


IOReturn
IOReporter::handleSwapPrepare(int newNChannels)
{
	IOReturn res = kIOReturnError;
	int newNElements;
	size_t newElementsSize, newECSize;

	                       
	newElementsSize = newECSize = 0;

	                                          

	IOREPORTER_CHECK_CONFIG_LOCK();

	if (newNChannels < _nChannels) {
		panic("%s doesn't support shrinking", __func__);
	}
	if (newNChannels <= 0 || _channelDimension <= 0) {
		res = kIOReturnUnderrun;
		goto finish;
	}
	if (_swapElements || _swapEnableCounts) {
		panic("IOReporter::_swap* already in use");
	}

	                                                                     
	if (newNChannels < 0 || newNChannels > INT_MAX / _channelDimension) {
		res = kIOReturnOverrun;
		goto finish;
	}
	newNElements = newNChannels * _channelDimension;

	                                                       
	PREFL_MEMOP_FAIL(newNElements, IOReportElement);
	newElementsSize = (size_t)newNElements * sizeof(IOReportElement);
	_swapElements = (IOReportElement *)IOMalloc(newElementsSize);
	if (_swapElements == NULL) {
		res = kIOReturnNoMemory; goto finish;
	}
	memset(_swapElements, 0, newElementsSize);

	                                                            
	PREFL_MEMOP_FAIL(newNChannels, int);
	newECSize = (size_t)newNChannels * sizeof(int);
	_swapEnableCounts = (int *)IOMalloc(newECSize);
	if (_swapEnableCounts == NULL) {
		res = kIOReturnNoMemory; goto finish;
	}
	memset(_swapEnableCounts, 0, newECSize);

	          
	res = kIOReturnSuccess;

finish:
	if (res) {
		if (_swapElements) {
			IOFree(_swapElements, newElementsSize);
			_swapElements = NULL;
		}
		if (_swapEnableCounts) {
			IOFree(_swapEnableCounts, newECSize);
			_swapEnableCounts = NULL;
		}
	}

	return res;
}


IOReturn
IOReporter::handleAddChannelSwap(uint64_t channel_id,
    const OSSymbol *symChannelName)
{
	IOReturn res = kIOReturnError;
	int cnt;
	int *tmpWatchCounts = NULL;
	IOReportElement *tmpElements = NULL;
	bool swapComplete = false;

	                                   

	IOREPORTER_CHECK_CONFIG_LOCK();
	IOREPORTER_CHECK_LOCK();

	if (!_swapElements || !_swapEnableCounts) {
		IORLOG("IOReporter::handleSwap ERROR swap variables uninitialized!");
		goto finish;
	}

	                                                 
	                                                                         
	if (_elements) {
		PREFL_MEMOP_PANIC(_nElements, IOReportElement);
		memcpy(_swapElements, _elements,
		    (size_t)_nElements * sizeof(IOReportElement));

		PREFL_MEMOP_PANIC(_nElements, int);
		memcpy(_swapEnableCounts, _enableCounts,
		    (size_t)_nChannels * sizeof(int));
	}

	                                                                    
	tmpElements = _elements;
	_elements = _swapElements;
	_swapElements = tmpElements;

	tmpWatchCounts = _enableCounts;
	_enableCounts = _swapEnableCounts;
	_swapEnableCounts = tmpWatchCounts;

	swapComplete = true;

	                                                                  

	                                                                   
	for (cnt = 0; cnt < _channelDimension; cnt++) {
		_elements[_nElements + cnt].channel_id = channel_id;
		_elements[_nElements + cnt].provider_id = _driver_id;
		_elements[_nElements + cnt].channel_type = _channelType;
		_elements[_nElements + cnt].channel_type.element_idx = ((int16_t) cnt);

		                                                 
	}

	                                  
	if (!_channelNames->setObject((unsigned)_nChannels, symChannelName)) {
		                                                                  
		res = kIOReturnNoMemory;
		goto finish;
	}

	                                                                     
	_nChannels += 1;
	_nElements += _channelDimension;

	          
	res = kIOReturnSuccess;

finish:
	if (res && swapComplete) {
		                                                      
		tmpElements = _elements;
		_elements = _swapElements;
		_swapElements = tmpElements;

		tmpWatchCounts = _enableCounts;
		_enableCounts = _swapEnableCounts;
		_swapEnableCounts = tmpWatchCounts;
	}
	return res;
}

void
IOReporter::handleSwapCleanup(int swapNChannels)
{
	int swapNElements;

	if (!_channelDimension || swapNChannels > INT_MAX / _channelDimension) {
		panic("%s - can't free %d channels of dimension %d", __func__,
		    swapNChannels, _channelDimension);
	}
	swapNElements = swapNChannels * _channelDimension;

	IOREPORTER_CHECK_CONFIG_LOCK();

	                                                
	if (_swapElements) {
		PREFL_MEMOP_PANIC(swapNElements, IOReportElement);
		IOFree(_swapElements, (size_t)swapNElements * sizeof(IOReportElement));
		_swapElements = NULL;
	}
	if (_swapEnableCounts) {
		PREFL_MEMOP_PANIC(swapNChannels, int);
		IOFree(_swapEnableCounts, (size_t)swapNChannels * sizeof(int));
		_swapEnableCounts = NULL;
	}
}


                                                             
                                                                    
                                                                           
IOReturn
IOReporter::handleConfigureReport(IOReportChannelList *channelList,
    IOReportConfigureAction action,
    void *result,
    void *destination)
{
	IOReturn res = kIOReturnError;
	int channel_index = 0;
	uint32_t chIdx;
	int *nElements, *nChannels;

	                                                     
	if (!channelList || !result) {
		goto finish;
	}

	                                                                 
	                                         

	                                                                        
	for (chIdx = 0; chIdx < channelList->nchannels; chIdx++) {
		if (getChannelIndex(channelList->channels[chIdx].channel_id,
		    &channel_index) == kIOReturnSuccess) {
			                                                                                                

			switch (action) {
			case kIOReportEnable:
				nChannels = (int*)result;
				_enabled++;
				_enableCounts[channel_index]++;
				(*nChannels)++;
				break;

			case kIOReportDisable:
				nChannels = (int*)result;
				_enabled--;
				_enableCounts[channel_index]--;
				(*nChannels)++;
				break;

			case kIOReportGetDimensions:
				nElements = (int *)result;
				*nElements += _channelDimension;
				break;

			default:
				IORLOG("ERROR configureReport unknown action!");
				break;
			}
		}
	}

	          
	res = kIOReturnSuccess;

finish:
	return res;
}


IOReturn
IOReporter::handleUpdateReport(IOReportChannelList *channelList,
    IOReportConfigureAction action,
    void *result,
    void *destination)
{
	IOReturn res = kIOReturnError;
	int *nElements = (int *)result;
	int channel_index = 0;
	uint32_t chIdx;
	IOBufferMemoryDescriptor *dest;

	if (!channelList || !result || !destination) {
		goto finish;
	}

	dest = OSDynamicCast(IOBufferMemoryDescriptor, (OSObject *)destination);
	if (dest == NULL) {
		                      
		res = kIOReturnBadArgument;
		goto finish;
	}

	if (!_enabled) {
		goto finish;
	}

	for (chIdx = 0; chIdx < channelList->nchannels; chIdx++) {
		if (getChannelIndex(channelList->channels[chIdx].channel_id,
		    &channel_index) == kIOReturnSuccess) {
			                                                           
			                                                 
			                        

			switch (action) {
			case kIOReportCopyChannelData:
				res = updateChannelValues(channel_index);
				if (res) {
					IORLOG("ERROR: updateChannelValues() failed: %x", res);
					goto finish;
				}

				res = updateReportChannel(channel_index, nElements, dest);
				if (res) {
					IORLOG("ERROR: updateReportChannel() failed: %x", res);
					goto finish;
				}
				break;

			default:
				IORLOG("ERROR updateReport unknown action!");
				res = kIOReturnError;
				goto finish;
			}
		}
	}

	          
	res = kIOReturnSuccess;

finish:
	return res;
}


OSSharedPtr<IOReportLegendEntry>
IOReporter::handleCreateLegend(void)
{
	OSSharedPtr<IOReportLegendEntry> legendEntry = nullptr;
	OSSharedPtr<OSArray> channelIDs;

	channelIDs = copyChannelIDs();

	if (channelIDs) {
		legendEntry = IOReporter::legendWith(channelIDs.get(), _channelNames.get(), _channelType, _unit);
	}

	return legendEntry;
}


IOReturn
IOReporter::setElementValues(int element_index,
    IOReportElementValues *values,
    uint64_t record_time          )
{
	IOReturn res = kIOReturnError;

	IOREPORTER_CHECK_LOCK();

	if (record_time == 0) {
		record_time = mach_absolute_time();
	}

	if (element_index >= _nElements || values == NULL) {
		res = kIOReturnBadArgument;
		goto finish;
	}

	memcpy(&_elements[element_index].values, values, sizeof(IOReportElementValues));

	_elements[element_index].timestamp = record_time;

	                                  

	res = kIOReturnSuccess;

finish:
	return res;
}


const IOReportElementValues*
IOReporter::getElementValues(int element_index)
{
	IOReportElementValues *elementValues = NULL;

	IOREPORTER_CHECK_LOCK();

	if (element_index < 0 || element_index >= _nElements) {
		IORLOG("ERROR getElementValues out of bounds!");
		goto finish;
	}

	elementValues = &_elements[element_index].values;

finish:
	return elementValues;
}


IOReturn
IOReporter::updateChannelValues(int channel_index)
{
	return kIOReturnSuccess;
}


IOReturn
IOReporter::updateReportChannel(int channel_index,
    int *nElements,
    IOBufferMemoryDescriptor *destination)
{
	IOReturn res = kIOReturnError;
	int start_element_idx, chElems;
	size_t       size2cpy;

	res = kIOReturnBadArgument;
	if (!nElements || !destination) {
		goto finish;
	}
	if (channel_index > _nChannels) {
		goto finish;
	}

	IOREPORTER_CHECK_LOCK();

	res = kIOReturnOverrun;

	start_element_idx = channel_index * _channelDimension;
	if (start_element_idx >= _nElements) {
		goto finish;
	}

	chElems = _elements[start_element_idx].channel_type.nelements;

	                                                                  
	if (start_element_idx + chElems > _nElements) {
		goto finish;
	}

	PREFL_MEMOP_FAIL(chElems, IOReportElement);
	size2cpy = (size_t)chElems * sizeof(IOReportElement);

	                                             
	if (size2cpy > (destination->getCapacity() - destination->getLength())) {
		IORLOG("CRITICAL ERROR: Report Buffer Overflow (buffer cap %luB, length %luB, size2cpy %luB",
		    (unsigned long)destination->getCapacity(),
		    (unsigned long)destination->getLength(),
		    (unsigned long)size2cpy);
		goto finish;
	}

	destination->appendBytes(&_elements[start_element_idx], size2cpy);
	*nElements += chElems;

	res = kIOReturnSuccess;

finish:
	return res;
}


IOReturn
IOReporter::copyElementValues(int element_index,
    IOReportElementValues *elementValues)
{
	IOReturn res = kIOReturnError;

	if (!elementValues) {
		goto finish;
	}

	IOREPORTER_CHECK_LOCK();

	if (element_index >= _nElements) {
		IORLOG("ERROR getElementValues out of bounds!");
		res = kIOReturnBadArgument;
		goto finish;
	}

	memcpy(elementValues, &_elements[element_index].values, sizeof(IOReportElementValues));
	res = kIOReturnSuccess;

finish:
	return res;
}


IOReturn
IOReporter::getFirstElementIndex(uint64_t channel_id,
    int *index)
{
	IOReturn res = kIOReturnError;
	int channel_index = 0, element_index = 0;

	if (!index) {
		goto finish;
	}

	res = getChannelIndices(channel_id, &channel_index, &element_index);

	if (res == kIOReturnSuccess) {
		*index = element_index;
	}

finish:
	return res;
}


IOReturn
IOReporter::getChannelIndex(uint64_t channel_id,
    int *index)
{
	IOReturn res = kIOReturnError;
	int channel_index = 0, element_index = 0;

	if (!index) {
		goto finish;
	}

	res = getChannelIndices(channel_id, &channel_index, &element_index);

	if (res == kIOReturnSuccess) {
		*index = channel_index;
	}

finish:
	return res;
}


IOReturn
IOReporter::getChannelIndices(uint64_t channel_id,
    int *channel_index,
    int *element_index)
{
	IOReturn res = kIOReturnNotFound;
	int chIdx, elemIdx;

	if (!channel_index || !element_index) {
		goto finish;
	}

	for (chIdx = 0; chIdx < _nChannels; chIdx++) {
		elemIdx = chIdx * _channelDimension;
		if (elemIdx >= _nElements) {
			IORLOG("ERROR getChannelIndices out of bounds!");
			res = kIOReturnOverrun;
			goto finish;
		}

		if (channel_id == _elements[elemIdx].channel_id) {
			                                                                 
			*channel_index = chIdx;
			*element_index = elemIdx;

			res = kIOReturnSuccess;
			goto finish;
		}
	}

finish:
	return res;
}

                                  
                                  
                                  


                                                   
OSSharedPtr<OSArray>
IOReporter::copyChannelIDs()
{
	int    cnt, cnt2;
	OSSharedPtr<OSArray>    channelIDs;
	OSSharedPtr<OSNumber>   tmpNum;

	channelIDs = OSArray::withCapacity((unsigned)_nChannels);

	if (!channelIDs) {
		return nullptr;
	}

	for (cnt = 0; cnt < _nChannels; cnt++) {
		cnt2 = cnt * _channelDimension;

		                                         
		tmpNum = OSNumber::withNumber(_elements[cnt2].channel_id, 64);
		if (!tmpNum) {
			IORLOG("ERROR: Could not create array of channelIDs");
			return nullptr;
		}

		channelIDs->setObject((unsigned)cnt, tmpNum.get());
		tmpNum.reset();
	}

	return channelIDs;
}


                                                                       
            OSSharedPtr<IOReportLegendEntry>
IOReporter::legendWith(OSArray *channelIDs,
    OSArray *channelNames,
    IOReportChannelType channelType,
    IOReportUnit unit)
{
	unsigned int            cnt, chCnt;
	uint64_t                type64;
	OSSharedPtr<OSNumber>   tmpNum;
	const OSSymbol          *tmpSymbol;
	OSSharedPtr<OSArray>    channelLegendArray;
	OSSharedPtr<OSArray>    tmpChannelArray;
	OSSharedPtr<OSDictionary> channelInfoDict;
	OSSharedPtr<IOReportLegendEntry> legendEntry = nullptr;

	                                                                      
	if (!channelIDs) {
		goto finish;
	}
	chCnt = channelIDs->getCount();

	channelLegendArray = OSArray::withCapacity(chCnt);

	for (cnt = 0; cnt < chCnt; cnt++) {
		tmpChannelArray = OSArray::withCapacity(3);

		                                         
		tmpChannelArray->setObject(kIOReportChannelIDIdx, channelIDs->getObject(cnt));

		                                           
		memcpy(&type64, &channelType, sizeof(type64));
		tmpNum = OSNumber::withNumber(type64, 64);
		if (!tmpNum) {
			goto finish;
		}
		tmpChannelArray->setObject(kIOReportChannelTypeIdx, tmpNum.get());
		tmpNum.reset();

		                                           
		                               
		if (channelNames != NULL) {
			tmpSymbol = OSDynamicCast(OSSymbol, channelNames->getObject(cnt));
			if (tmpSymbol && tmpSymbol != gIOReportNoChannelName) {
				tmpChannelArray->setObject(kIOReportChannelNameIdx, tmpSymbol);
			}                                         
		}

		channelLegendArray->setObject(cnt, tmpChannelArray.get());
		tmpChannelArray.reset();
	}

	                                                     
	if (channelLegendArray->getCount() != 0) {
		channelInfoDict = OSDictionary::withCapacity(1);

		if (!channelInfoDict) {
			goto finish;
		}

		tmpNum = OSNumber::withNumber(unit, 64);
		if (tmpNum) {
			channelInfoDict->setObject(kIOReportLegendUnitKey, tmpNum.get());
		}

		legendEntry = OSDictionary::withCapacity(1);

		if (legendEntry) {
			legendEntry->setObject(kIOReportLegendChannelsKey, channelLegendArray.get());
			legendEntry->setObject(kIOReportLegendInfoKey, channelInfoDict.get());
		}
	}

finish:
	return legendEntry;
}
