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
#ifndef _IOMEMORYCURSOR_H
#define _IOMEMORYCURSOR_H

#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSPtr.h>
#include <IOKit/IOTypes.h>

class IOMemoryDescriptor;

                                                                               

   
                          
                                                                              
                                                                     
                                                                                                                                                                                                                                                                                                                            
            
                                                                                                                                                                                                                                                                                                                                                                                                                                                        
            
                                                           
                                                                          
                                                                    
                                                                        
         
                                                                      
     
  
   
class IOMemoryCursor : public OSObject
{
	OSDeclareDefaultStructors(IOMemoryCursor);

public:
   
                             
                                                
   
	struct PhysicalSegment {
		IOPhysicalAddress location;
		IOPhysicalLength  length;
	};

                              
                                                                                                                                 
   
#define IOPhysicalSegment IOMemoryCursor::PhysicalSegment

   
                             
                                                                                                                                                                  
                                                                              
                                                                           
                                                                           
   
	typedef void (*SegmentFunction)(PhysicalSegment segment,
	    void *          segments,
	    UInt32          segmentIndex);

                              
                                                                                                                                    
#define OutputSegmentFunc IOMemoryCursor::SegmentFunction

protected:
                                                                            
	SegmentFunction outSeg;

                                                                               
	IOPhysicalLength  maxSegmentSize;

                        
                                                                                 
	IOPhysicalLength  maxTransferSize;

                  
                                                                            
	IOPhysicalLength  alignMask;

public:
                               
                                                                          
                                                                                                                                                                
                                                                              
                                                                                  
                                                                                                    
                                                                                                                                          
                                                                                              
   
	static OSPtr<IOMemoryCursor>
	withSpecification(SegmentFunction  outSegFunc,
	    IOPhysicalLength maxSegmentSize = 0,
	    IOPhysicalLength maxTransferSize = 0,
	    IOPhysicalLength alignment = 1);

                                   
                                                                
                                                                              
                                                                                  
                                                                                                    
                                                                                                                                          
                                                                               
                 
   
	virtual bool initWithSpecification(SegmentFunction  outSegFunc,
	    IOPhysicalLength maxSegmentSize = 0,
	    IOPhysicalLength maxTransferSize = 0,
	    IOPhysicalLength alignment = 1);

                                 
                                                                                  
                                                                                                                                            
                                                                                                 
                                                                                 
                                                                                                                                                                        
                                                                                         
                                                                                                                                                                                     
                                                                                                                                                                                              
                                                                                                                                            
   
	virtual UInt32 genPhysicalSegments(
		IOMemoryDescriptor *descriptor,
		IOByteCount         fromPosition,
		void *              segments,
		UInt32              maxSegments,
		UInt32              maxTransferSize = 0,
		IOByteCount        *transferSize = NULL);
};

                                                                               


   
                                 
                                                                                                                                
                                                                                                                                                                                                                          
   
class IONaturalMemoryCursor : public IOMemoryCursor
{
	OSDeclareDefaultStructors(IONaturalMemoryCursor);

public:
                           
                                                                                              
                                                                              
                                                                           
                                                                           
   
	static void outputSegment(PhysicalSegment segment,
	    void *          segments,
	    UInt32          segmentIndex);

                                 
                                                                                                                                 
   
#define naturalOutputSegment IONaturalMemoryCursor::outputSegment

                               
                                                                                 
                                                                                                                                                                              
                                                                                  
                                                                                                    
                                                                                                                                          
                                                                                              
   
	static OSPtr<IONaturalMemoryCursor>
	withSpecification(IOPhysicalLength maxSegmentSize,
	    IOPhysicalLength maxTransferSize,
	    IOPhysicalLength alignment = 1);

                                   
                                                                       
                                                                                  
                                                                                                    
                                                                                                                                          
                                                                                             
   
	virtual bool initWithSpecification(IOPhysicalLength  maxSegmentSize,
	    IOPhysicalLength  maxTransferSize,
	    IOPhysicalLength  alignment = 1);


                                 
                                                                                              
                                                                                                                                                                                        
                                                                                                 
                                                                                 
                                                                                                                         
                                                                                         
                                                                                                                                                                                       
                                                                                                                                                                                              
                                                                                                                                            
   
	virtual UInt32
	getPhysicalSegments(IOMemoryDescriptor *descriptor,
	    IOByteCount         fromPosition,
	    PhysicalSegment    *segments,
	    UInt32              maxSegments,
	    UInt32              inMaxTransferSize = 0,
	    IOByteCount        *transferSize = NULL)
	{
		return genPhysicalSegments(descriptor, fromPosition, segments,
		           maxSegments, inMaxTransferSize, transferSize);
	}
};

                                                                               

   
                             
                                                                                                                 
                                                                                                                                                                                                                  
   
class IOBigMemoryCursor : public IOMemoryCursor
{
	OSDeclareDefaultStructors(IOBigMemoryCursor);

public:
                           
                                                                                                 
                                                                              
                                                                           
                                                                           
   
	static void outputSegment(PhysicalSegment segment,
	    void *          segments,
	    UInt32          segmentIndex);

                             
                                                                                                                            
   
#define bigOutputSegment IOBigMemoryCursor::outputSegment

                               
                                                                             
                                                                                                                                                     
                                                                                  
                                                                                                    
                                                                                                                                          
                                                                                              
   
	static OSPtr<IOBigMemoryCursor>
	withSpecification(IOPhysicalLength maxSegmentSize,
	    IOPhysicalLength maxTransferSize,
	    IOPhysicalLength alignment = 1);

                                   
                                                                   
                                                                                  
                                                                                                    
                                                                                                                                          
                                                                               
                 
   
	virtual bool initWithSpecification(IOPhysicalLength  maxSegmentSize,
	    IOPhysicalLength  maxTransferSize,
	    IOPhysicalLength  alignment = 1);


                                 
                                                                                             
                                                                                                                                                                                        
                                                                                                 
                                                                                 
                                                                                                                         
                                                                                         
                                                                                                                                                                                       
                                                                                                                                                                                              
                                                                                                                                            
   
	virtual UInt32
	getPhysicalSegments(IOMemoryDescriptor * descriptor,
	    IOByteCount          fromPosition,
	    PhysicalSegment *    segments,
	    UInt32               maxSegments,
	    UInt32               inMaxTransferSize = 0,
	    IOByteCount       *  transferSize = NULL)
	{
		return genPhysicalSegments(descriptor, fromPosition, segments,
		           maxSegments, inMaxTransferSize, transferSize);
	}
};

                                                                               

   
                                
                                                                                                                    
                                                                                                                                                                                                                          
   
class IOLittleMemoryCursor : public IOMemoryCursor
{
	OSDeclareDefaultStructors(IOLittleMemoryCursor);

public:
                           
                                                                                                    
                                                                              
                                                                           
                                                                           
   
	static void outputSegment(PhysicalSegment segment,
	    void *          segments,
	    UInt32          segmentIndex);

                                
                                                                                                                                        
#define littleOutputSegment IOLittleMemoryCursor::outputSegment

                               
                                                                                
                                                                                                                                                           
                                                                                  
                                                                                                    
                                                                                                                                          
                                                                                              
   
	static OSPtr<IOLittleMemoryCursor>
	withSpecification(IOPhysicalLength maxSegmentSize,
	    IOPhysicalLength maxTransferSize,
	    IOPhysicalLength alignment = 1);

                                   
                                                                      
                                                                                  
                                                                                                    
                                                                                                                                          
                                                                                             
   
	virtual bool initWithSpecification(IOPhysicalLength  maxSegmentSize,
	    IOPhysicalLength  maxTransferSize,
	    IOPhysicalLength  alignment = 1);


                                 
                                                                                                
                                                                                                                                                                                        
                                                                                                 
                                                                                 
                                                                                                                         
                                                                                         
                                                                                                                                                                                       
                                                                                                                                                                                              
                                                                                                                                            
   
	virtual UInt32
	getPhysicalSegments(IOMemoryDescriptor * descriptor,
	    IOByteCount          fromPosition,
	    PhysicalSegment *    segments,
	    UInt32               maxSegments,
	    UInt32               inMaxTransferSize = 0,
	    IOByteCount       *  transferSize = NULL)
	{
		return genPhysicalSegments(descriptor, fromPosition, segments,
		           maxSegments, inMaxTransferSize, transferSize);
	}
};

#endif                         
