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

#ifndef _IOINTERLEAVEDMEMORYDESCRIPTOR_H
#define _IOINTERLEAVEDMEMORYDESCRIPTOR_H

#include <IOKit/IOMemoryDescriptor.h>
#include <libkern/c++/OSPtr.h>

                                                                    
                                                                                                                                         
                                                                                                                                                                                                                                                             

class IOInterleavedMemoryDescriptor : public IOMemoryDescriptor
{
	OSDeclareDefaultStructors(IOInterleavedMemoryDescriptor);

protected:

	IOByteCount           _descriptorCapacity;
	UInt32                _descriptorCount;
	IOMemoryDescriptor ** _descriptors;
	IOByteCount         * _descriptorOffsets;
	IOByteCount         * _descriptorLengths;
	bool                  _descriptorPrepared;

	virtual void free() APPLE_KEXT_OVERRIDE;

public:

                          
                                                                                                                              
                                                                                                                                                                                                                                                                            
                                                                                                                                    
                                                                                                                                                                      
                                                                                                                       

	static OSPtr<IOInterleavedMemoryDescriptor>  withCapacity( IOByteCount           capacity,
	    IODirection           direction);

                              
                                                                                                                                  
                                                                                                                                                                                                                                                                
                                                                                                                                    
                                                                                                                                                                      
                                                                                                                       

	virtual bool initWithCapacity( IOByteCount           capacity,
	    IODirection           direction );

                                    
                                                                                                                       
                                                                                                                                                                                                                          
                                                                                                                                                                         

	virtual void clearMemoryDescriptors( IODirection direction = kIODirectionNone );

                                 
                                                                                           
                                                                                                                                                                                                                                                                                                                                             
                                                                                                                                                                                
                                                                                                                      
                                                                                                             
                                                                

	virtual bool setMemoryDescriptor( IOMemoryDescriptor * descriptor,
	    IOByteCount offset,
	    IOByteCount length );

                                
                                                                                 
                                                                                                                                                                                           
                                                                                  
                                                                                                                                        
                                                                                             

	virtual addr64_t getPhysicalSegment( IOByteCount   offset,
	    IOByteCount * length,
	    IOOptionBits  options = 0 ) APPLE_KEXT_OVERRIDE;

                     
                                                      
                                                                                                                                                                                                                                                                            
                                                                                                                                            
                                 

	virtual IOReturn prepare(IODirection forDirection = kIODirectionNone) APPLE_KEXT_OVERRIDE;

                      
                                                                                
                                                                                                                                                                                                       
                                                                                                                                           
                                 

	virtual IOReturn complete(IODirection forDirection = kIODirectionNone) APPLE_KEXT_OVERRIDE;
};

#endif                                        
