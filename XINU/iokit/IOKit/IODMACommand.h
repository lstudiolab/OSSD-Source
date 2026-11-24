/*
 * Copyright (c) 2005-2016 Apple Inc. All rights reserved.
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
#ifndef _IODMACOMMAND_H
#define _IODMACOMMAND_H

#include <IOKit/IOCommand.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <DriverKit/IODMACommand.h>
class IOMapper;
class IOBufferMemoryDescriptor;

enum{
	kIODMAMapOptionMapped       = 0x00000000,
	kIODMAMapOptionBypassed     = 0x00000001,
	kIODMAMapOptionNonCoherent  = 0x00000002,
	kIODMAMapOptionUnmapped     = 0x00000003,
	kIODMAMapOptionTypeMask     = 0x0000000f,

	kIODMAMapOptionNoCacheStore = 0x00000010,                        
	kIODMAMapOptionOnChip       = 0x00000020,                                   
	kIODMAMapOptionIterateOnly  = 0x00000040                                           
};

                                                                             

   
                        
                                                                             
                                                                                                                                                                                                                                                                                                  
            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
            
                                                                                                                                                                                                                                                                                                                                                                                                
            
                                                                                                                                                                                                                                                                                                                                                                                                               
   

class IODMACommand : public IOCommand
{
	OSDeclareDefaultStructorsWithDispatch(IODMACommand);

	friend class IODMAEventSource;

public:

   
                       
                                                     
   
	struct Segment32 {
		UInt32 fIOVMAddr, fLength;
	};

   
                       
                                                     
   
	struct Segment64 {
		UInt64 fIOVMAddr, fLength;
	};

                        
                                                                                                                               
                                                                                                
                                                                
                                                            
                                                   
   
	enum MappingOptions {
		kMapped       = kIODMAMapOptionMapped,
		kBypassed     = kIODMAMapOptionBypassed,
		kNonCoherent  = kIODMAMapOptionNonCoherent,
		kUnmapped     = kIODMAMapOptionUnmapped,
		kTypeMask     = kIODMAMapOptionTypeMask,

		kNoCacheStore = kIODMAMapOptionNoCacheStore,                        
		kOnChip       = kIODMAMapOptionOnChip,                                     
		kIterateOnly  = kIODMAMapOptionIterateOnly                                           
	};

	struct SegmentOptions {
		uint8_t  fStructSize;
		uint8_t  fNumAddressBits;
		uint64_t fMaxSegmentSize;
		uint64_t fMaxTransferSize;
		uint32_t fAlignment;
		uint32_t fAlignmentLength;
		uint32_t fAlignmentInternalSegments;
	};

                            
                                                  
                                                                                              
   
	enum SynchronizeOptions {
		kForceDoubleBuffer = 0x01000000
	};

   
                             
                                                                                                                                                                                                                                                 
                                                         
                                                                           
                                                                           
                                                                                                                                                                                           
   
	typedef bool (*SegmentFunction)(IODMACommand *target,
	    Segment64 segment,
	    void     *segments,
	    UInt32    segmentIndex);

                                                                  

                          
                                                                     
   
	static bool OutputHost32(IODMACommand *target,
	    Segment64 seg, void *segs, UInt32 ind);

                                      
                                                                     
   
#define kIODMACommandOutputHost32   (IODMACommand::OutputHost32)

                         
                                                                   
   
	static bool OutputBig32(IODMACommand *target,
	    Segment64 seg, void *segs, UInt32 ind);

                                     
                                                                   
   
#define kIODMACommandOutputBig32    (IODMACommand::OutputBig32)

                            
                                                                      
   
	static bool OutputLittle32(IODMACommand *target,
	    Segment64 seg, void *segs, UInt32 ind);

                                        
                                                                      
   
#define kIODMACommandOutputLittle32 (IODMACommand::OutputLittle32)

                          
                                                                     
   
	static bool OutputHost64(IODMACommand *target,
	    Segment64 seg, void *segs, UInt32 ind);

                                      
                                                                     
   
#define kIODMACommandOutputHost64   (IODMACommand::OutputHost64)

                         
                                                                   
   
	static bool OutputBig64(IODMACommand *target,
	    Segment64 seg, void *segs, UInt32 ind);

                                     
                                                                   
   
#define kIODMACommandOutputBig64    (IODMACommand::OutputBig64)

                            
                                                                      
   
	static bool OutputLittle64(IODMACommand *target,
	    Segment64 seg, void *segs, UInt32 ind);

                                        
                                                                      
   
#define kIODMACommandOutputLittle64 (IODMACommand::OutputLittle64)

                               
                                                                        
                                                                                            
                                                                                                                                              
                                                                                                                                                                                                                                  
                                                                                                                         
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
                                                                                                    
                                                                                                                
                                                                                                                                                            
                                     
                                                                                             
   
	static OSPtr<IODMACommand>
	withSpecification(SegmentFunction  outSegFunc,
	    UInt8            numAddressBits,
	    UInt64           maxSegmentSize,
	    MappingOptions   mappingOptions = kMapped,
	    UInt64           maxTransferSize = 0,
	    UInt32           alignment = 1,
	    IOMapper        *mapper = NULL,
	    void            *refCon = NULL);

                                   
                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                     
                                                                                   
                                                                                                                                              
                                                                                                                                                                                                                                  
                                                                                                                
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
                                                                                                     
                                                                                                                
                                                                                                                                                            
                                     
                                                                                                                                                                                                                                                      
   
                                                          
                                                               
                                                                    
                                                               
                                            
	static inline IOReturn weakWithSpecification
	(IODMACommand   **newCommand,
	    SegmentFunction outSegFunc,
	    UInt8           numAddressBits,
	    UInt64          maxSegmentSize,
	    MappingOptions  mapType = kMapped,
	    UInt64          maxTransferSize = 0,
	    UInt32          alignment = 1,
	    IOMapper       *mapper = NULL,
	    void           *refCon = NULL) __attribute__((always_inline));

	static OSPtr<IODMACommand>
	withSpecification(SegmentFunction        outSegFunc,
	    const SegmentOptions * segmentOptions,
	    uint32_t               mappingOptions,
	    IOMapper             * mapper,
	    void                 * refCon);


                        
                                                                   
                                                                                                                                                                       
                                     
                                                                                             
   
	static OSPtr<IODMACommand> withRefCon(void * refCon);

   
                           
                                                                                   
                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                                                             
   
	virtual OSPtr<IODMACommand> cloneCommand(void *refCon = NULL);

                                   
                                                              
                                                                                                                                              
                                                                                                                                                                                                                                  
                                                                                                       
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
                                                                                                    
                                                                                                                
                                                                                                                                                            
                                     
                                                                                                                                                                                                                                                                                                         
   
	virtual bool initWithSpecification( SegmentFunction  outSegFunc,
	    UInt8     numAddressBits,
	    UInt64    maxSegmentSize,
	    MappingOptions mappingOptions = kMapped,
	    UInt64    maxTransferSize = 0,
	    UInt32    alignment = 1,
	    IOMapper *mapper = NULL,
	    void     *refCon = NULL);

                                 
                                                                         
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                                
                                                                                                                                                                    
                                                                                                                                                                                       
   
	virtual IOReturn setMemoryDescriptor(const IOMemoryDescriptor *mem,
	    bool autoPrepare = true);

                                   
                                                                
                                                                                                                                                                                                             
                                                                                                                                                                       
   
	virtual IOReturn clearMemoryDescriptor(bool autoComplete = true);

                                 
                                                
   
	virtual const IOMemoryDescriptor * getMemoryDescriptor() const;

                                   
                                                           
   
	IOMemoryDescriptor * getIOMemoryDescriptor() const;

                     
                                                      
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                                                                                                                                                                              
                                                                                                                                                                                                              
                                                                                                                                                                                             
                                                                                                                                                                                                            
                                 

	virtual IOReturn prepare(UInt64 offset = 0, UInt64 length = 0, bool flushCache = true, bool synchronize = true);

                      
                                                                                     
                                                                                                                                                                            
                                                                                                                                                  
                                                                                                                                                                                                          
                                                                              

	virtual IOReturn complete(bool invalidateCache = true, bool synchronize = true);

                         
                                                                           
                                                                                                                                                                                                   
                                                        
                                                                                                                                                  
                                                                                                                                                     
                                                                                        
                                                                                                                                  

	virtual IOReturn synchronize(IOOptionBits options);

                             
                                                                              
                                                                                                                                                                                                                                                                                                                                                                      
                                                                                                                                                                  
                                                                                                                                   
                                                                                                                                             
                                                                                                                                                                                                                                                                                                                                                                                                                                                       
   
	virtual IOReturn genIOVMSegments(UInt64 *offset,
	    void   *segments,
	    UInt32 *numSegments);

private:
	virtual UInt64 transfer( IOOptionBits transferOp, UInt64 offset, void * buffer, UInt64 length );

public:

                        
                                                                                
                                                                                                                                                                                                          
                                                                                                 
                                                                   
                                                  
                                                                                                                                            

	UInt64 writeBytes(UInt64 offset, const void *bytes, UInt64 length);

                       
                                                                                
                                                                                                                                                                                                          
                                                                                                 
                                                                 
                                                  
                                                                                                                                            

	UInt64 readBytes(UInt64 offset, void *bytes, UInt64 length);

                               
                                                                                                                                                                                                                                                                                                                                
   
	inline IOReturn
	gen32IOVMSegments(UInt64   *offset,
	    Segment32 *segments,
	    UInt32     *numSegments)
	{
		return genIOVMSegments(offset, segments, numSegments);
	}

                               
                                                                                                                                                                                                                                                                                                                                
   
	inline IOReturn
	gen64IOVMSegments(UInt64    *offset,
	    Segment64 *segments,
	    UInt32    *numSegments)
	{
		return genIOVMSegments(offset, segments, numSegments);
	}

	IOReturn
	genIOVMSegments(SegmentFunction segmentFunction,
	    UInt64 *offsetP,
	    void   *segmentsP,
	    UInt32 *numSegmentsP);

	virtual void free() APPLE_KEXT_OVERRIDE;

private:
	IOReturn setSpecification(SegmentFunction        outSegFunc,
	    const SegmentOptions * segmentOptions,
	    uint32_t               mappingOptions,
	    IOMapper             * mapper);

	typedef IOReturn (*InternalSegmentFunction)(
		void         *reference,
		IODMACommand *target,
		Segment64     segment,
		void         *segments,
		UInt32        segmentIndex);

	IOReturn genIOVMSegments(uint32_t op,
	    InternalSegmentFunction outSegFunc,
	    void   *reference,
	    UInt64 *offsetP,
	    void   *segmentsP,
	    UInt32 *numSegmentsP);

	static IOReturn clientOutputSegment(
		void *reference, IODMACommand *target,
		Segment64 segment, void *vSegList, UInt32 outSegIndex);

	static IOReturn segmentOp(
		void         *reference,
		IODMACommand *target,
		Segment64     segment,
		void         *segments,
		UInt32        segmentIndex);
	IOReturn walkAll(uint32_t op);

public:

                                      
                                                                               
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                                                                                                              
                                                                                                                                                                                                                                  
                                                                                                       
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
                                                                                                    
                                                                                                                
                                                                                                                                                            
                                                                                                                                                                                                              
                                                                                                                                                                                                              
                                                                                                                                                                                             
                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                  
   

	virtual IOReturn prepareWithSpecification(SegmentFunction   outSegFunc,
	    UInt8             numAddressBits,
	    UInt64            maxSegmentSize,
	    MappingOptions    mappingOptions = kMapped,
	    UInt64            maxTransferSize = 0,
	    UInt32            alignment = 1,
	    IOMapper          *mapper = NULL,
	    UInt64            offset = 0,
	    UInt64            length = 0,
	    bool              flushCache = true,
	    bool              synchronize = true);

	static IOReturn transferSegment(void         *reference,
	    IODMACommand *target,
	    Segment64     segment,
	    void         *segments,
	    UInt32        segmentIndex);

                                        
                                                                                                            
                                                                                                                                               
                                                                                                                                    
                                                                                                                           
                                                                                        

	virtual IOReturn getPreparedOffsetAndLength(UInt64 * offset, UInt64 * length);

	UInt8    getNumAddressBits(void);
	UInt32   getAlignment(void);
	uint32_t getAlignmentLength(void);
	uint32_t getAlignmentInternalSegments(void);


                            
                                                                
                                     
                                                                   
   

	virtual
	bool initWithRefCon(void * refCon = NULL);

	virtual
	bool initWithSpecification(SegmentFunction        outSegFunc,
	    const SegmentOptions * segmentOptions,
	    uint32_t               mappingOptions,
	    IOMapper             * mapper,
	    void                 * refCon);

	virtual
	IOReturn prepareWithSpecification(SegmentFunction        outSegFunc,
	    const SegmentOptions * segmentOptions,
	    uint32_t               mappingOptions,
	    IOMapper             * mapper,
	    uint64_t               offset,
	    uint64_t               length,
	    bool                   flushCache = true,
	    bool                   synchronize = true);

	virtual
	OSPtr<IOBufferMemoryDescriptor> createCopyBuffer(IODirection direction, UInt64 length);

private:
	OSMetaClassDeclareReservedUsedX86(IODMACommand, 0);
	OSMetaClassDeclareReservedUsedX86(IODMACommand, 1);
	OSMetaClassDeclareReservedUsedX86(IODMACommand, 2);
	OSMetaClassDeclareReservedUsedX86(IODMACommand, 3);
	OSMetaClassDeclareReservedUsedX86(IODMACommand, 4);
	OSMetaClassDeclareReservedUsedX86(IODMACommand, 5);
	OSMetaClassDeclareReservedUsedX86(IODMACommand, 6);
	OSMetaClassDeclareReservedUnused(IODMACommand, 7);
	OSMetaClassDeclareReservedUnused(IODMACommand, 8);
	OSMetaClassDeclareReservedUnused(IODMACommand, 9);
	OSMetaClassDeclareReservedUnused(IODMACommand, 10);
	OSMetaClassDeclareReservedUnused(IODMACommand, 11);
	OSMetaClassDeclareReservedUnused(IODMACommand, 12);
	OSMetaClassDeclareReservedUnused(IODMACommand, 13);
	OSMetaClassDeclareReservedUnused(IODMACommand, 14);
	OSMetaClassDeclareReservedUnused(IODMACommand, 15);

public:
                                                                         
	void *fRefCon;

protected:

                                                                                
	UInt64  fMaxSegmentSize;

                         
                                                                                 
	UInt64  fMaxTransferSize;

	UInt32  fAlignMaskLength;
	UInt32  fAlignMaskInternalSegments;

                
                              
	OSPtr<IOMapper> fMapper;

                
                                          
	OSPtr<IOMemoryDescriptor> fMemory;

                                                                             
	SegmentFunction fOutSeg;

                   
                                   
	UInt32  fAlignMask;

                        
                                                    
	UInt32 fNumAddressBits;

                     
                                                                                                    
	UInt32  fNumSegments;

                        
                                                                             
	uint32_t  fMappingOptions;

                
                                                                                     
	UInt32 fActive;

                 
                                                      
	struct IODMACommandInternal  *reserved;
};

IOReturn
IODMACommand::
weakWithSpecification(IODMACommand **newCommand,
    SegmentFunction  outSegFunc,
    UInt8     numAddressBits,
    UInt64    maxSegmentSize,
    MappingOptions mapType,
    UInt64    maxTransferSize,
    UInt32    alignment,
    IOMapper *mapper,
    void     *refCon)
{
	if (!newCommand) {
		return kIOReturnBadArgument;
	}

	IODMACommand *self = (IODMACommand *)
	    OSMetaClass::allocClassWithName("IODMACommand");
	if (!self) {
		return kIOReturnUnsupported;
	}

	IOReturn ret;
	bool inited = self->
	    initWithSpecification(outSegFunc,
	    numAddressBits, maxSegmentSize, mapType,
	    maxTransferSize, alignment, mapper, refCon);
	if (inited) {
		ret =  kIOReturnSuccess;
	} else {
		self->release();
		self = NULL;
		ret = kIOReturnError;
	}

	*newCommand = self;
	return ret;
};
#endif                       
