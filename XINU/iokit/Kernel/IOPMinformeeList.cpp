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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
#include <IOKit/pwr_mgt/IOPM.h>
#include <IOKit/pwr_mgt/IOPMinformeeList.h>
#include <IOKit/pwr_mgt/IOPMinformee.h>

#define super OSObject
OSDefineMetaClassAndStructors(IOPMinformeeList, OSObject)

                                                                                   
       
  
                                                                                   
void
IOPMinformeeList::initialize( void )
{
	firstItem = NULL;
	length = 0;
}

                                                                                
                         
  
                                                                                
IORecursiveLock *
IOPMinformeeList::getSharedRecursiveLock( void )
{
	static IORecursiveLock *sharedListLock = NULL;

	                                                                    
                                                                           
                                                                   
   
                                                                      
                                                                     
   
                                                                             
                                           
    

	if (NULL == sharedListLock) {
		sharedListLock = IORecursiveLockAlloc();
	}
	return sharedListLock;
}

                                                                                   
                    
  
                                                                                   
IOPMinformee *
IOPMinformeeList::appendNewInformee( IOService * newObject )
{
	IOPMinformee * newInformee;

	if (!newObject) {
		return NULL;
	}

	newInformee = IOPMinformee::withObject(newObject);

	if (!newInformee) {
		return NULL;
	}

	if (IOPMNoErr == addToList(newInformee)) {
		return newInformee;
	} else {
		newInformee->release();
		return NULL;
	}
}


                                                                                   
            
                                                    
                                  
                                                                                   
IOReturn
IOPMinformeeList::addToList( IOPMinformee * newInformee )
{
	IORecursiveLock *listLock = getSharedRecursiveLock();
	IOReturn        ret = kIOReturnError;

	if (!listLock) {
		return ret;
	}

	IORecursiveLockLock(listLock);

	                                     
	if (findItem(newInformee->whatObject) != NULL) {
		                               
		goto unlock_and_exit;
	}

	                                  
	newInformee->nextInList = firstItem;
	firstItem = newInformee;
	length++;
	ret = IOPMNoErr;

unlock_and_exit:
	IORecursiveLockUnlock(listLock);
	return ret;
}


                                                                                   
                 
  
                                                     
                                                                                   

IOReturn
IOPMinformeeList::removeFromList( IOService * theItem )
{
	IOPMinformee * item = firstItem;
	IOPMinformee * temp;
	IORecursiveLock    *listLock = getSharedRecursiveLock();

	if (NULL == item) {
		return IOPMNoErr;
	}
	if (!listLock) {
		return kIOReturnError;
	}

	IORecursiveLockLock( listLock );

	if (item->whatObject == theItem) {
		firstItem = item->nextInList;
		length--;
		item->release();
		goto unlock_and_exit;
	}

	while (item->nextInList != NULL) {
		if (item->nextInList->whatObject == theItem) {
			temp = item->nextInList;
			item->nextInList = temp->nextInList;
			length--;
			temp->release();
			goto unlock_and_exit;
		}
		item = item->nextInList;
	}

unlock_and_exit:
	IORecursiveLockUnlock(listLock);
	return IOPMNoErr;
}


                                                                                   
              
  
                                                                                   

IOPMinformee *
IOPMinformeeList::firstInList( void )
{
	return firstItem;
}

                                                                                   
             
  
                                                                                   

IOPMinformee *
IOPMinformeeList::nextInList( IOPMinformee * currentItem )
{
	if (currentItem != NULL) {
		return currentItem->nextInList;
	}
	return NULL;
}

                                                                                   
                
  
                                                                                   

unsigned long
IOPMinformeeList::numberOfItems( void )
{
	return length;
}

                                                                                   
           
  
                                                                          
                                                                
                                                                                   

IOPMinformee *
IOPMinformeeList::findItem( IOService * driverOrChild )
{
	IOPMinformee * nextObject;

	nextObject = firstInList();
	while (nextObject != NULL) {
		if (nextObject->whatObject == driverOrChild) {
			return nextObject;
		}
		nextObject = nextInList(nextObject);
	}
	return NULL;
}



                                                                                   
       
  
                                                            
                                                                                   

void
IOPMinformeeList::free(void )
{
	IOPMinformee * next = firstItem;

	while (next != NULL) {
		firstItem = next->nextInList;
		length--;
		next->release();
		next = firstItem;
	}
	super::free();
}
