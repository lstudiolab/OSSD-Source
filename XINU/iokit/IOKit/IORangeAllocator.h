/*
 * Copyright (c) 1998-2019 Apple Inc. All rights reserved.
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
/*
 * Copyright (c) 1999 Apple Computer, Inc.
 *
 *
 * HISTORY
 *
 * sdouglas 05 Nov 99 - created.
 */

#ifndef _IOKIT_IORANGEALLOCATOR_H
#define _IOKIT_IORANGEALLOCATOR_H

#include <libkern/c++/OSPtr.h>
#include <libkern/c++/OSObject.h>
#include <IOKit/IOTypes.h>

typedef IOByteCount IORangeScalar;

                           
                                                                  
                                                                                                                                                                                                                                                                                                                                                                                                                                                   
   

class IORangeAllocator : public OSObject {
	OSDeclareDefaultStructors(IORangeAllocator);

protected:
	UInt32              numElements;
	UInt32              capacity;
	UInt32              capacityIncrement;
	IORangeScalar       defaultAlignmentMask;
	IOOptionBits        options;

	struct IORangeAllocatorElement *    elements;

private:
	virtual bool allocElement( UInt32 index );

	virtual void deallocElement( UInt32 index );

public:
	enum {
		kLocking        = 0x00000001
	};

                  
                                                         
                                                                                                                                                                                                                                                   
                                                                                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                                              
                                                                                                                                                    
                                                                                  
                                                                                           

	virtual bool init( IORangeScalar endOfRange,
	    IORangeScalar defaultAlignment,
	    UInt32 capacity,
	    IOOptionBits options );

                       
                                                            
                                                                                                                                                                                                                                                                 
                                                                                                                                                                                                                                                                                                                                   
                                                                                                                                                                                                                              
                                                                                                                                                     
                                                                                  
                                                                                                            

	static OSPtr<IORangeAllocator>  withRange( IORangeScalar endOfRange,
	    IORangeScalar defaultAlignment = 0,
	    UInt32 capacity = 0,
	    IOOptionBits options = 0 );

	virtual void free() APPLE_KEXT_OVERRIDE;
	virtual bool serialize(OSSerialize *s) const APPLE_KEXT_OVERRIDE;

                              
                                                                            
                                                                                                                                                                             
                                                 
   

	virtual UInt32 getFragmentCount( void );

                                 
                                                                            
                                                                                    
                                                                
   

	virtual UInt32 getFragmentCapacity( void );

                                          
                                                                                    
                                                                                                                                            
                                                                                                  
   

	virtual void setFragmentCapacityIncrement( UInt32 count );

                          
                                                      
                                                                                              
                                                           
   

	virtual IORangeScalar getFreeCount( void );

                      
                                                           
                                                                                                                                                                     
                                                 
                                                                                    
                                                                                                                                                                      
                                                                       
   

	virtual bool allocate( IORangeScalar size,
	    IORangeScalar * result,
	    IORangeScalar alignment = 0 );

                           
                                                             
                                                                                                
                                                       
                                                 
                                                                       
   

	virtual bool allocateRange( IORangeScalar start,
	    IORangeScalar size );

                        
                                                    
                                                                                                                 
                                                       
                                                         
   

	virtual void deallocate( IORangeScalar start,
	    IORangeScalar size );
};

#endif                                
