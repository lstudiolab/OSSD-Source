/*
 * Copyright (c) 1998-2016 Apple Inc. All rights reserved.
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
#ifndef _IOBUFFERMEMORYDESCRIPTOR_H
#define _IOBUFFERMEMORYDESCRIPTOR_H

#include <libkern/c++/OSPtr.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>

enum {
	kIOMemoryPhysicallyContiguous       = 0x00000010,
	kIOMemoryPageable                   = 0x00000020,
	kIOMemoryPurgeable                  = 0x00000040,
	kIOMemoryHostPhysicallyContiguous   = 0x00000080,
	kIOMemorySharingTypeMask            = 0x000f0000,
	kIOMemoryUnshared                   = 0x00000000,
	kIOMemoryKernelUserShared           = 0x00010000,
	                                                                  
	kIOBufferDescriptorMemoryFlags      = kIOMemoryDirectionMask
#ifdef XNU_KERNEL_PRIVATE
	    | kIOMemoryAutoPrepare
#endif
	    | kIOMemoryThreadSafe
	    | kIOMemoryClearEncrypt
	    | kIOMemoryMapperNone
	    | kIOMemoryUseReserve
};

#define _IOBUFFERMEMORYDESCRIPTOR_INTASKWITHOPTIONS_            1
#define _IOBUFFERMEMORYDESCRIPTOR_HOSTPHYSICALLYCONTIGUOUS_     1
#define IOBUFFERMEMORYDESCRIPTOR_SUPPORTS_INTASKWITHOPTIONS_TAGS        1
   
                                    
                                                                                        
   

class IOBufferMemoryDescriptor : public IOGeneralMemoryDescriptor
{
	OSDeclareDefaultStructorsWithDispatch(IOBufferMemoryDescriptor);

private:
                         
                                                                                                    
   
	struct ExpansionData {
		IOMemoryMap *   map;
	};

                 
                                                      
	APPLE_KEXT_WSHADOW_PUSH;
	ExpansionData * reserved;

protected:
	void *               _buffer;
	vm_size_t            _capacity;
	vm_offset_t          _alignment;
	IOOptionBits         _options;
private:
	uintptr_t            _internalReserved;
	unsigned             _internalFlags;
	APPLE_KEXT_WSHADOW_POP;

private:
#ifndef __LP64__
	virtual bool initWithOptions(
		IOOptionBits options,
		vm_size_t    capacity,
		vm_offset_t  alignment,
		task_t       inTask) APPLE_KEXT_DEPRECATED;                                               
#endif                

public:
	virtual bool initWithPhysicalMask(
		task_t            inTask,
		IOOptionBits      options,
		mach_vm_size_t    capacity,
		mach_vm_address_t alignment,
		mach_vm_address_t physicalMask);

#ifdef __LP64__
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 0);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 1);
#else                
	OSMetaClassDeclareReservedUsedX86(IOBufferMemoryDescriptor, 0);
	OSMetaClassDeclareReservedUsedX86(IOBufferMemoryDescriptor, 1);
#endif                
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 2);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 3);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 4);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 5);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 6);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 7);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 8);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 9);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 10);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 11);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 12);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 13);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 14);
	OSMetaClassDeclareReservedUnused(IOBufferMemoryDescriptor, 15);

protected:
	virtual void free() APPLE_KEXT_OVERRIDE;

public:

  
               
  
                                                                       
                                                                        
            
   
#ifndef __LP64__
	virtual bool initWithOptions(   IOOptionBits options,
	    vm_size_t    capacity,
	    vm_offset_t  alignment) APPLE_KEXT_DEPRECATED;                                                        
#endif                

	static OSPtr<IOBufferMemoryDescriptor> withCopy(
		task_t            inTask,
		IOOptionBits      options,
		vm_map_t          sourceMap,
		mach_vm_address_t source,
		mach_vm_size_t    size);

	static OSPtr<IOBufferMemoryDescriptor> withOptions(  IOOptionBits options,
	    vm_size_t    capacity,
	    vm_offset_t  alignment = 1);

                               
                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                            
                                                   
                                                                                 
                                                                                                                                                                                                                                                                 
                                                                                                                    
                                                                                                                                                                                         
                                                                                                                             
                                                                            
                                                                              
                                                                            
                                                                                
                                                     
                                                                                                                                                                                                         
                                                                                                                                                               

	static OSPtr<IOBufferMemoryDescriptor> inTaskWithOptions(
		task_t       inTask,
		IOOptionBits options,
		vm_size_t    capacity,
		vm_offset_t  alignment = 1);

                               
                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                            
                                                   
                                                                                 
                                                                                                                                                                                                                                                                 
                                                                                                                    
                                                                                                                                                                                         
                                                                                                                             
                                                                            
                                                                              
                                                                            
                                                                                
                                                     
                                                                                                                                                                                                         
                                         
                                       
                                                                                                                                                               

	static OSPtr<IOBufferMemoryDescriptor> inTaskWithOptions(
		task_t       inTask,
		IOOptionBits options,
		vm_size_t    capacity,
		vm_offset_t  alignment,
		uint32_t     kernTag,
		uint32_t     userTag);

                                    
                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                                                                                                        
                                                   
                                                                                 
                                                                                                                                                                                                                                                                 
                                                                                                                             
                                                                            
                                                                              
                                                                            
                                                                                
                                                     
                                                                                                                                                                                                                                                        
                                                                                                                                                               

	static OSPtr<IOBufferMemoryDescriptor> inTaskWithPhysicalMask(
		task_t            inTask,
		IOOptionBits      options,
		mach_vm_size_t    capacity,
		mach_vm_address_t physicalMask);

  
                
  
                                                                       
                                                                        
            
   
	static OSPtr<IOBufferMemoryDescriptor> withCapacity(
		vm_size_t    capacity,
		IODirection  withDirection,
		bool         withContiguousMemory = false);
#ifndef __LP64__
	virtual bool initWithBytes(const void * bytes,
	    vm_size_t    withLength,
	    IODirection  withDirection,
	    bool         withContiguousMemory = false) APPLE_KEXT_DEPRECATED;                                                 
#endif                

  
             
  
                                                                        
                                                                           
   
	static OSPtr<IOBufferMemoryDescriptor> withBytes(
		const void * bytes,
		vm_size_t    withLength,
		IODirection  withDirection,
		bool         withContiguousMemory = false);

  
             
  
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
                                                                        
   
	virtual void setLength(vm_size_t length);

  
                
  
                                                                            
                                                                            
                                                                        
   
	virtual void setDirection(IODirection direction);

  
               
  
                          
   
	virtual vm_size_t getCapacity() const;

  
                  
  
                                                            
   
	virtual void *getBytesNoCopy();

  
                  
  
                                                                           
   
	virtual void *getBytesNoCopy(vm_size_t start, vm_size_t withLength);

  
               
  
                                                                     
                                                                        
                                                                          
   
	virtual bool appendBytes(const void *bytes, vm_size_t withLength);

#ifndef __LP64__
	virtual void * getVirtualSegment(IOByteCount offset,
	    IOByteCount * length) APPLE_KEXT_OVERRIDE APPLE_KEXT_DEPRECATED;                                                               
#endif                
};

#endif                                   
