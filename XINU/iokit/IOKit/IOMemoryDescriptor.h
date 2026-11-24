/*
 * Copyright (c) 1998-2020 Apple Inc. All rights reserved.
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
#ifndef _IOMEMORYDESCRIPTOR_H
#define _IOMEMORYDESCRIPTOR_H

#include <sys/cdefs.h>

#include <IOKit/IOTypes.h>
#include <IOKit/IOLocks.h>
#include <libkern/c++/OSPtr.h>
#include <libkern/c++/OSContainers.h>
#include <DriverKit/IOMemoryDescriptor.h>
#include <DriverKit/IOMemoryMap.h>
#ifdef XNU_KERNEL_PRIVATE
#include <IOKit/IOKitDebug.h>
#endif

#include <mach/memory_object_types.h>

class IOMemoryDescriptor;
class IOMemoryMap;
class IOMapper;
class IOService;
class IODMACommand;

  
                                                               
   
#ifdef __LP64__
enum
#else                
enum IODirection
#endif                
{
	kIODirectionNone  = 0x0,                                          
	kIODirectionIn    = 0x1,                                          
	kIODirectionOut   = 0x2,                                           
	kIODirectionOutIn = kIODirectionOut | kIODirectionIn,
	kIODirectionInOut = kIODirectionIn  | kIODirectionOut,

	                                                      
	kIODirectionPrepareToPhys32   = 0x00000004,
	kIODirectionPrepareNoFault    = 0x00000008,
	kIODirectionPrepareReserved1  = 0x00000010,
#define IODIRECTIONPREPARENONCOHERENTDEFINED    1
	kIODirectionPrepareNonCoherent = 0x00000020,
#if KERNEL_PRIVATE
#define IODIRECTIONPREPAREAVOIDTHROTTLING       1
	kIODirectionPrepareAvoidThrottling = 0x00000100,
#endif

	                                                       
#define IODIRECTIONCOMPLETEWITHERRORDEFINED             1
	kIODirectionCompleteWithError = 0x00000040,
#define IODIRECTIONCOMPLETEWITHDATAVALIDDEFINED 1
	kIODirectionCompleteWithDataValid = 0x00000080,
};

#ifdef __LP64__
typedef IOOptionBits IODirection;
#endif               

  
                                               
   
enum {
	kIOMemoryDirectionMask      = 0x00000007,
#ifdef XNU_KERNEL_PRIVATE
	kIOMemoryAutoPrepare        = 0x00000008,                        
#endif

	kIOMemoryTypeVirtual        = 0x00000010,
	kIOMemoryTypePhysical       = 0x00000020,
	kIOMemoryTypeUPL            = 0x00000030,
	kIOMemoryTypePersistentMD   = 0x00000040,                               
	kIOMemoryTypeUIO            = 0x00000050,
#ifdef __LP64__
	kIOMemoryTypeVirtual64      = kIOMemoryTypeVirtual,
	kIOMemoryTypePhysical64     = kIOMemoryTypePhysical,
#else                
	kIOMemoryTypeVirtual64      = 0x00000060,
	kIOMemoryTypePhysical64     = 0x00000070,
#endif                
	kIOMemoryTypeMask           = 0x000000f0,

	kIOMemoryAsReference        = 0x00000100,
	kIOMemoryBufferPageable     = 0x00000400,
	kIOMemoryMapperNone         = 0x00000800,                        
	kIOMemoryHostOnly           = 0x00001000,                       
#ifdef XNU_KERNEL_PRIVATE
	kIOMemoryRedirected         = 0x00004000,
	kIOMemoryPreparedReadOnly   = 0x00008000,
#endif
	kIOMemoryPersistent         = 0x00010000,
	kIOMemoryMapCopyOnWrite     = 0x00020000,
	kIOMemoryRemote             = 0x00040000,
	kIOMemoryThreadSafe         = 0x00100000,                        
	kIOMemoryClearEncrypt       = 0x00200000,                        
	kIOMemoryUseReserve         = 0x00800000,                        
#define IOMEMORYUSERESERVEDEFINED       1

#ifdef XNU_KERNEL_PRIVATE
	kIOMemoryBufferPurgeable    = 0x00400000,
	kIOMemoryBufferCacheMask    = 0x70000000,
	kIOMemoryBufferCacheShift   = 28,
#endif
};

#define kIOMapperSystem ((IOMapper *) NULL)

enum{
	kIOMemoryLedgerTagDefault       = VM_LEDGER_TAG_DEFAULT,
	kIOmemoryLedgerTagNetwork       = VM_LEDGER_TAG_NETWORK,
	kIOMemoryLedgerTagMedia         = VM_LEDGER_TAG_MEDIA,
	kIOMemoryLedgerTagGraphics      = VM_LEDGER_TAG_GRAPHICS,
	kIOMemoryLedgerTagNeural        = VM_LEDGER_TAG_NEURAL,
};
enum{
	kIOMemoryLedgerFlagNoFootprint  = VM_LEDGER_FLAG_NO_FOOTPRINT,
};

enum{
	kIOMemoryPurgeableKeepCurrent = 1,

	kIOMemoryPurgeableNonVolatile = 2,
	kIOMemoryPurgeableVolatile    = 3,
	kIOMemoryPurgeableEmpty       = 4,

	                                                    
	kIOMemoryPurgeableVolatileGroup0           = VM_VOLATILE_GROUP_0,
	kIOMemoryPurgeableVolatileGroup1           = VM_VOLATILE_GROUP_1,
	kIOMemoryPurgeableVolatileGroup2           = VM_VOLATILE_GROUP_2,
	kIOMemoryPurgeableVolatileGroup3           = VM_VOLATILE_GROUP_3,
	kIOMemoryPurgeableVolatileGroup4           = VM_VOLATILE_GROUP_4,
	kIOMemoryPurgeableVolatileGroup5           = VM_VOLATILE_GROUP_5,
	kIOMemoryPurgeableVolatileGroup6           = VM_VOLATILE_GROUP_6,
	kIOMemoryPurgeableVolatileGroup7           = VM_VOLATILE_GROUP_7,
	kIOMemoryPurgeableVolatileBehaviorFifo     = VM_PURGABLE_BEHAVIOR_FIFO,
	kIOMemoryPurgeableVolatileBehaviorLifo     = VM_PURGABLE_BEHAVIOR_LIFO,
	kIOMemoryPurgeableVolatileOrderingObsolete = VM_PURGABLE_ORDERING_OBSOLETE,
	kIOMemoryPurgeableVolatileOrderingNormal   = VM_PURGABLE_ORDERING_NORMAL,
	kIOMemoryPurgeableFaultOnAccess            = VM_PURGABLE_DEBUG_FAULT,
};
enum{
	kIOMemoryIncoherentIOFlush   = 1,
	kIOMemoryIncoherentIOStore   = 2,

	kIOMemoryClearEncrypted      = 50,
	kIOMemorySetEncrypted        = 51,
};

#define IOMEMORYDESCRIPTOR_SUPPORTS_DMACOMMAND  1

struct IODMAMapSpecification {
	uint64_t    alignment;
	IOService * device;
	uint32_t    options;
	uint8_t     numAddressBits;
	uint8_t     resvA[3];
	uint32_t    resvB[4];
};

struct IODMAMapPageList {
	uint32_t                pageOffset;
	uint32_t                pageListCount;
	const upl_page_info_t * pageList;
};

                               
enum{
	kIODMAMapReadAccess           = 0x00000001,
	kIODMAMapWriteAccess          = 0x00000002,
	kIODMAMapPhysicallyContiguous = 0x00000010,
	kIODMAMapDeviceMemory         = 0x00000020,
	kIODMAMapPagingPath           = 0x00000040,
	kIODMAMapIdentityMap          = 0x00000080,

	kIODMAMapPageListFullyOccupied = 0x00000100,
	kIODMAMapFixedAddress          = 0x00000200,
};

#ifdef KERNEL_PRIVATE

                                                                           

enum  {
	kIOMDWalkSegments             = 0x01000000,
	kIOMDFirstSegment             = 1 | kIOMDWalkSegments,
	kIOMDGetCharacteristics       = 0x02000000,
	kIOMDGetCharacteristicsMapped = 1 | kIOMDGetCharacteristics,
	kIOMDDMAActive                = 0x03000000,
	kIOMDSetDMAActive             = 1 | kIOMDDMAActive,
	kIOMDSetDMAInactive           = kIOMDDMAActive,
	kIOMDAddDMAMapSpec            = 0x04000000,
	kIOMDDMAMap                   = 0x05000000,
	kIOMDDMAUnmap                 = 0x06000000,
	kIOMDDMACommandOperationMask  = 0xFF000000,
};
struct IOMDDMACharacteristics {
	UInt64 fLength;
	UInt32 fSGCount;
	UInt32 fPages;
	UInt32 fPageAlign;
	ppnum_t fHighestPage;
	IODirection fDirection;
	UInt8 fIsPrepared;
};

struct IOMDDMAMapArgs {
	IOMapper            * fMapper;
	IODMACommand        * fCommand;
	IODMAMapSpecification fMapSpec;
	uint64_t              fOffset;
	uint64_t              fLength;
	uint64_t              fAlloc;
	uint64_t              fAllocLength;
};

struct IOMDDMAWalkSegmentArgs {
	UInt64 fOffset;                                       
	UInt64 fIOVMAddr, fLength;                         
	UInt8 fMapped;                                                         
	UInt64 fMappedBase;                                     
};
typedef UInt8 IOMDDMAWalkSegmentState[128];

#endif                     

enum{
	kIOPreparationIDUnprepared = 0,
	kIOPreparationIDUnsupported = 1,
	kIOPreparationIDAlwaysPrepared = 2,
};

#ifdef KERNEL_PRIVATE
#define kIODescriptorIDInvalid (0)
#endif

#ifdef XNU_KERNEL_PRIVATE
struct IOMemoryReference;
#endif


                                               
                                                                                                        
                                                                                                                                                                                                                                                                                                                                                                            

class IOMemoryDescriptor : public OSObject
{
	friend class IOMemoryMap;
	friend class IOMultiMemoryDescriptor;

	OSDeclareDefaultStructorsWithDispatch(IOMemoryDescriptor);

protected:

                 
                                                      
	struct IOMemoryDescriptorReserved * reserved;

protected:
	OSPtr<OSSet>        _mappings;
	IOOptionBits        _flags;


#ifdef XNU_KERNEL_PRIVATE
public:
	struct IOMemoryReference *  _memRef;
	vm_tag_t _kernelTag;
	vm_tag_t _userTag;
	int16_t _dmaReferences;
	uint16_t _internalFlags;
	kern_allocation_name_t _mapName;
protected:
#else                         
	void *              __iomd_reserved5;
	uint16_t            __iomd_reserved1[4];
	uintptr_t           __iomd_reserved2;
#endif                         

	uintptr_t           __iomd_reserved3;
	uintptr_t           __iomd_reserved4;

#ifndef __LP64__
	IODirection         _direction;                            
#endif                
	IOByteCount         _length;                                 
	IOOptionBits        _tag;

public:
	typedef IOOptionBits DMACommandOps;
#ifndef __LP64__
	virtual IOPhysicalAddress getSourceSegment( IOByteCount offset,
	    IOByteCount * length ) APPLE_KEXT_DEPRECATED;
#endif                

                             
                                                                                                                                               
                                                                                                  
                                                  
	virtual bool initWithOptions(void *         buffers,
	    UInt32         count,
	    UInt32         offset,
	    task_t         task,
	    IOOptionBits   options,
	    IOMapper *     mapper = kIOMapperSystem);

#ifndef __LP64__
	virtual addr64_t getPhysicalSegment64( IOByteCount offset,
	    IOByteCount * length ) APPLE_KEXT_DEPRECATED;                                                                                               
#endif                

                          
                                                                           
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                                         
                                                                                         
                                                                                                                                                              
                                                                                                                                                                   
                                                                                                     
                                                                                                    
                                                                    
                                                                                                                         
                                                                                                         
                                 

	virtual IOReturn setPurgeable( IOOptionBits newState,
	    IOOptionBits * oldState );

                          
                                                                    
                                                                                                                                             
                                                                  
                                                                         
                                                 
                                 

	IOReturn setOwnership( task_t newOwner,
	    int newLedgerTag,
	    IOOptionBits newLedgerOptions );

                           
                                                                                                       
                                                                                                                    
                                                                                                                                                            
                                                                                                                                                      
                                 

	IOReturn getPageCounts( IOByteCount * residentPageCount,
	    IOByteCount * dirtyPageCount);

                              
                                                                      
                                                                                                                                                                                                                                                              
                                                               
                                                                                                                                                                                                                                                                                                                                                                                                                                                             
                                                                                                                                                                                                                                                                                                                                                                                                                                               
                                                                     
                                                
                                 

	virtual IOReturn performOperation( IOOptionBits options,
	    IOByteCount offset, IOByteCount length );

                                                     
	virtual IOReturn dmaCommandOperation(DMACommandOps op, void *vData, UInt dataSize) const;

                                
                                                                                 
                                                                                                                                                                                           
                                                                                  
                                                                                                                                        
                                                                                             

#ifdef __LP64__
	virtual addr64_t getPhysicalSegment( IOByteCount   offset,
	    IOByteCount * length,
	    IOOptionBits  options = 0 ) = 0;
#else                
	virtual addr64_t getPhysicalSegment( IOByteCount   offset,
	    IOByteCount * length,
	    IOOptionBits  options );
#endif                

	virtual uint64_t getPreparationID( void );
	void             setPreparationID( void );

	void     setVMTags(uint32_t kernelTag, uint32_t userTag);
	uint32_t getVMTag(vm_map_t map);

#ifdef KERNEL_PRIVATE
	uint64_t getDescriptorID( void );
	void     setDescriptorID( void );

	IOReturn ktraceEmitPhysicalSegments( void );
#endif

#ifdef XNU_KERNEL_PRIVATE
	IOMemoryDescriptorReserved * getKernelReserved( void );
	void                         cleanKernelReserved(IOMemoryDescriptorReserved * reserved);
	IOReturn dmaMap(
		IOMapper                    * mapper,
		IOMemoryDescriptor          * memory,
		IODMACommand                * command,
		const IODMAMapSpecification * mapSpec,
		uint64_t                      offset,
		uint64_t                      length,
		uint64_t                    * mapAddress,
		uint64_t                    * mapLength);
	IOReturn dmaUnmap(
		IOMapper                    * mapper,
		IODMACommand                * command,
		uint64_t                      offset,
		uint64_t                      mapAddress,
		uint64_t                      mapLength);
	void dmaMapRecord(
		IOMapper                    * mapper,
		IODMACommand                * command,
		uint64_t                      mapLength);
#endif

private:
	OSMetaClassDeclareReservedUsedX86(IOMemoryDescriptor, 0);
#ifdef __LP64__
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 1);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 2);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 3);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 4);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 5);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 6);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 7);
#else                
	OSMetaClassDeclareReservedUsedX86(IOMemoryDescriptor, 1);
	OSMetaClassDeclareReservedUsedX86(IOMemoryDescriptor, 2);
	OSMetaClassDeclareReservedUsedX86(IOMemoryDescriptor, 3);
	OSMetaClassDeclareReservedUsedX86(IOMemoryDescriptor, 4);
	OSMetaClassDeclareReservedUsedX86(IOMemoryDescriptor, 5);
	OSMetaClassDeclareReservedUsedX86(IOMemoryDescriptor, 6);
	OSMetaClassDeclareReservedUsedX86(IOMemoryDescriptor, 7);
#endif                
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 8);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 9);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 10);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 11);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 12);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 13);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 14);
	OSMetaClassDeclareReservedUnused(IOMemoryDescriptor, 15);

protected:
	virtual void free(void) APPLE_KEXT_OVERRIDE;
public:
	static void initialize( void );

public:
                         
                                                                                             
                                                                                                                                                                                                                                                                           
                                                                        
                                            
                                                                                                                                                                          
                                                                                                            

	static OSPtr<IOMemoryDescriptor>  withAddress(void *       address,
	    IOByteCount  withLength,
	    IODirection  withDirection);

#ifndef __LP64__
	static OSPtr<IOMemoryDescriptor>  withAddress(IOVirtualAddress address,
	    IOByteCount  withLength,
	    IODirection  withDirection,
	    task_t       withTask) APPLE_KEXT_DEPRECATED;                                                                                   
#endif                

                                 
                                                                           
                                                                                                                                   
                                                                         
                                            
                                                                                                                                                                          
                                                                                                            

	static OSPtr<IOMemoryDescriptor>  withPhysicalAddress(
		IOPhysicalAddress       address,
		IOByteCount             withLength,
		IODirection             withDirection );

#ifndef __LP64__
	static OSPtr<IOMemoryDescriptor>  withRanges(IOVirtualRange * ranges,
	    UInt32           withCount,
	    IODirection      withDirection,
	    task_t           withTask,
	    bool             asReference = false) APPLE_KEXT_DEPRECATED;                                                                      
#endif                

                              
                                                                                               
                                                                                                                                                                                                                                                                              
                                                                        
                                        
                   
                                                                                                                                                                                                                           
                                                                                                                                                                                                                                                              
                                                                                                            

	static OSPtr<IOMemoryDescriptor>  withAddressRange(
		mach_vm_address_t address,
		mach_vm_size_t    length,
		IOOptionBits      options,
		task_t            task);

                               
                                                                                   
                                                                                                                                                                                                                                                                                             
                                                                                                                                                                                                               
                                                            
                   
                                                                                                                                                                                                                           
                                                                                                                                                                                                                                    
                                                                                                                                                                                                                                                                      
                                                                                                            

	static OSPtr<IOMemoryDescriptor>  withAddressRanges(
		IOAddressRange * ranges,
		UInt32           rangeCount,
		IOOptionBits     options,
		task_t           task);

                         
                                                                         
                                                                                                                                                                                                                                                                                                                                 
  
  
                                                                                                                                                                                                                                                                                                                                                                                                                              
  
                                                                                                                                                                                      
  
                                                                                                                                        
  
                                                                                                       
  
                   
                                                                                                                                                                                                                           
                                                                                                                                                                                                                                                                                                                             
                                                                                                                                                                                                                                    
                                                                                                                                                                                                                         
  
                                                                                                                                                                                                   
  
                                                                                                            

	static OSPtr<IOMemoryDescriptor> withOptions(void *       buffers,
	    UInt32       count,
	    UInt32       offset,
	    task_t       task,
	    IOOptionBits options,
	    IOMapper *   mapper = kIOMapperSystem);

#ifndef __LP64__
	static OSPtr<IOMemoryDescriptor>  withPhysicalRanges(
		IOPhysicalRange *   ranges,
		UInt32              withCount,
		IODirection         withDirection,
		bool                asReference = false) APPLE_KEXT_DEPRECATED;                                                                                      
#endif                

#ifndef __LP64__
	static OSPtr<IOMemoryDescriptor>  withSubRange(IOMemoryDescriptor *of,
	    IOByteCount offset,
	    IOByteCount length,
	    IODirection withDirection) APPLE_KEXT_DEPRECATED;                                                                                                                 
#endif                

                                            
                                                                                                                                                        
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                              
                                                                                                                                                                                   
	static OSPtr<IOMemoryDescriptor>
	withPersistentMemoryDescriptor(IOMemoryDescriptor *originalMD);

#ifndef __LP64__
                        
                                                  
	virtual bool initWithAddress(void *       address,
	    IOByteCount  withLength,
	    IODirection  withDirection) APPLE_KEXT_DEPRECATED;                                                         
	virtual bool initWithAddress(IOVirtualAddress address,
	    IOByteCount  withLength,
	    IODirection  withDirection,
	    task_t       withTask) APPLE_KEXT_DEPRECATED;                                                         
	virtual bool initWithPhysicalAddress(
		IOPhysicalAddress      address,
		IOByteCount            withLength,
		IODirection            withDirection ) APPLE_KEXT_DEPRECATED;                                                     
	virtual bool initWithRanges(IOVirtualRange * ranges,
	    UInt32           withCount,
	    IODirection      withDirection,
	    task_t           withTask,
	    bool             asReference = false) APPLE_KEXT_DEPRECATED;                                                        
	virtual bool initWithPhysicalRanges(IOPhysicalRange * ranges,
	    UInt32           withCount,
	    IODirection      withDirection,
	    bool             asReference = false) APPLE_KEXT_DEPRECATED;                                                                
#endif               

                          
                                                                                    
                                                                                          
                              

	virtual IODirection getDirection() const;

                       
                                                                                         
                                                                                                                                    
                               

	virtual IOByteCount getLength() const;

#define IOMEMORYDESCRIPTOR_SUPPORTS_GETDMAMAPLENGTH
	uint64_t getDMAMapLength(uint64_t * offset = NULL);

                    
                                                     
                                                                                                                        
                           

	virtual void setTag( IOOptionBits tag );

                    
                                                                          
                                                                                                                           
                        

	virtual IOOptionBits getTag( void );

                      
                                                                                           
                                                                                                                                                                      
                                   

	uint64_t getFlags(void);

                       
                                                                                     
                                                                                                                                                                                                                                                                                                                                                                                                     
                                                                     
                                                                 
                                                      
                                                                                                                                                                                                                                   

	virtual IOByteCount readBytes(IOByteCount offset,
	    void * bytes, IOByteCount withLength);

                        
                                                                                     
                                                                                                                                                                                                                                                                                                                                                                                      
                                                                     
                                                                   
                                                      
                                                                                                                                                                                                                                   

	virtual IOByteCount writeBytes(IOByteCount offset,
	    const void * bytes, IOByteCount withLength);

#ifndef __LP64__
	virtual IOPhysicalAddress getPhysicalSegment(IOByteCount offset,
	    IOByteCount * length);
#endif                

                                
                                                                           
                                                                                                                                                          
                                   

	IOPhysicalAddress getPhysicalAddress();

#ifndef __LP64__
	virtual void * getVirtualSegment(IOByteCount offset,
	    IOByteCount * length) APPLE_KEXT_DEPRECATED;                                                                            
#endif                

                     
                                                      
                                                                                                                                                                                                                                                                                                                                                                                               
                                                                                                                                           
                                 

	virtual IOReturn prepare(IODirection forDirection = kIODirectionNone) = 0;

                      
                                                                                
                                                                                                                                                                                                                                                                                                                                                       
                                                                                                                                                      
                                 

	virtual IOReturn complete(IODirection forDirection = kIODirectionNone) = 0;

  
                     
   

                                 
                                                     
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
                                                                                                         
                                                                                                                                                                   
                                                                 
                                                                                                                                                                                                
                                                                                                              
                                                                                                          
                                                                                                                
                                                                                                                                                                        
                                                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                                        
                                                                                                                                                  
                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               

	OSPtr<IOMemoryMap>        createMappingInTask(
		task_t                  intoTask,
		mach_vm_address_t       atAddress,
		IOOptionBits            options,
		mach_vm_size_t          offset = 0,
		mach_vm_size_t          length = 0 );

#ifndef __LP64__
	virtual OSPtr<IOMemoryMap>       map(
		task_t          intoTask,
		IOVirtualAddress        atAddress,
		IOOptionBits            options,
		IOByteCount             offset = 0,
		IOByteCount             length = 0 ) APPLE_KEXT_DEPRECATED;                                       
#endif                

                 
                                                             
                                                                                                                                                                                                                           
                                                                                                                          
                                                                       

	virtual OSPtr<IOMemoryMap>       map(
		IOOptionBits            options = 0 );

                        
                                                       
                                                                                                                                                                                                                                                                                                     
                                                           
                                                      
                                                                    
                                                                      

	virtual OSPtr<IOMemoryMap>       setMapping(
		task_t          task,
		IOVirtualAddress        mapAddress,
		IOOptionBits            options = 0 );

                                               

#ifdef __LP64__
	virtual
#endif               
	IOReturn redirect( task_t safeTask, bool redirect );

	IOReturn handleFault(
		void *                  _pager,
		mach_vm_size_t          sourceOffset,
		mach_vm_size_t          length);

	IOReturn populateDevicePager(
		void *                  pager,
		vm_map_t                addressMap,
		mach_vm_address_t       address,
		mach_vm_size_t          sourceOffset,
		mach_vm_size_t          length,
		IOOptionBits            options );

	virtual IOMemoryMap *      makeMapping(
		IOMemoryDescriptor *    owner,
		task_t                  intoTask,
		IOVirtualAddress        atAddress,
		IOOptionBits            options,
		IOByteCount             offset,
		IOByteCount             length );

protected:
	virtual void                addMapping(
		IOMemoryMap *           mapping );

	virtual void                removeMapping(
		IOMemoryMap *           mapping );

	virtual IOReturn doMap(
		vm_map_t                addressMap,
		IOVirtualAddress *      atAddress,
		IOOptionBits            options,
		IOByteCount             sourceOffset = 0,
		IOByteCount             length = 0 );

	virtual IOReturn doUnmap(
		vm_map_t                addressMap,
		IOVirtualAddress        logical,
		IOByteCount             length );
};

                                                                           

                                        
                                                                               
                                                                                                                                                                                                                                                                                                                                                 

class IOMemoryMap : public OSObject
{
	OSDeclareDefaultStructorsWithDispatch(IOMemoryMap);
#ifdef XNU_KERNEL_PRIVATE
public:
	IOOptionBits         fOptions;
	OSPtr<IOMemoryDescriptor>  fMemory;
	OSPtr<IOMemoryMap>         fSuperMap;
	mach_vm_size_t       fOffset;
	mach_vm_address_t    fAddress;
	mach_vm_size_t       fLength;
	task_t               fAddressTask;
	vm_map_t             fAddressMap;
	upl_t                fRedirUPL;
	uint8_t              fUserClientUnmap;
#if IOTRACKING
	IOTrackingUser       fTracking;
#endif
#endif                         

protected:
	virtual void taggedRelease(const void *tag = NULL) const APPLE_KEXT_OVERRIDE;
	virtual void free(void) APPLE_KEXT_OVERRIDE;

public:
                               
                                                                                
                                                                                                                                                                                                                                        
                                  

	virtual IOVirtualAddress    getVirtualAddress(void);

                                
                                                                       
                                                                                                                                                                                                                                                              
                                                                                   
                                                                                                                                        
                                                                                              

#ifdef __LP64__
	virtual IOPhysicalAddress   getPhysicalSegment(IOByteCount offset,
	    IOByteCount * length,
	    IOOptionBits  options = 0);
#else                
	virtual IOPhysicalAddress   getPhysicalSegment(IOByteCount offset,
	    IOByteCount * length);
#endif                

                                
                                                                            
                                                                                                                                                             
                                   

	IOPhysicalAddress getPhysicalAddress(void);

                       
                                                     
                                                               
                             

	virtual IOByteCount         getLength(void);

                            
                                                   
                                                                         
                              

	virtual task_t              getAddressTask();

                                 
                                                                               
                                                                                         
                                                                                                                                                  

	virtual IOMemoryDescriptor * getMemoryDescriptor();

                           
                                                                    
                                                                                                         
                                                                  

	virtual IOOptionBits        getMapOptions();

                   
                                                                             
                                                                                                                                                                                                                                                                        
                                 

	virtual IOReturn            unmap();

	virtual void                taskDied();

                      
                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                                                                                                                                                                      
                                                                                                             
                                                                                                                                                            
                                 

#ifndef __LP64__
                                                                                           
                                                     
	virtual IOReturn            redirect(IOMemoryDescriptor * newBackingMemory,
	    IOOptionBits         options,
	    IOByteCount          offset = 0);
#endif
	virtual IOReturn            redirect(IOMemoryDescriptor * newBackingMemory,
	    IOOptionBits         options,
	    mach_vm_size_t       offset = 0);

#ifdef __LP64__
                        
                                                                                
                                                                                          
                                  
	inline mach_vm_address_t    getAddress() __attribute__((always_inline));
                     
                                                     
                                                               
                             
	inline mach_vm_size_t       getSize() __attribute__((always_inline));
#else                
                        
                                                                                
                                                                                          
                                  
	virtual mach_vm_address_t   getAddress();
                     
                                                     
                                                               
                             
	virtual mach_vm_size_t      getSize();
#endif                

#ifdef XNU_KERNEL_PRIVATE
                             
	IOMemoryMap *  copyCompatible( IOMemoryMap * newMapping );

	bool init(
		task_t                  intoTask,
		mach_vm_address_t       toAddress,
		IOOptionBits            options,
		mach_vm_size_t          offset,
		mach_vm_size_t          length );

	bool    setMemoryDescriptor(IOMemoryDescriptor * _memory, mach_vm_size_t _offset);

	IOReturn redirect(
		task_t                  intoTask, bool redirect );

	IOReturn userClientUnmap();
#endif                         

	IOReturn wireRange(
		uint32_t                options,
		mach_vm_size_t          offset,
		mach_vm_size_t          length);

	OSMetaClassDeclareReservedUnused(IOMemoryMap, 0);
	OSMetaClassDeclareReservedUnused(IOMemoryMap, 1);
	OSMetaClassDeclareReservedUnused(IOMemoryMap, 2);
	OSMetaClassDeclareReservedUnused(IOMemoryMap, 3);
	OSMetaClassDeclareReservedUnused(IOMemoryMap, 4);
	OSMetaClassDeclareReservedUnused(IOMemoryMap, 5);
	OSMetaClassDeclareReservedUnused(IOMemoryMap, 6);
	OSMetaClassDeclareReservedUnused(IOMemoryMap, 7);
};

                                                                           
#ifdef XNU_KERNEL_PRIVATE
                                                          
                                                                 
enum {
	_kIOMemorySourceSegment     = 0x00002000
};
#endif                         

                                                                                
                                                                          
                                                                               
                                                                                  
                                                                       

class IOGeneralMemoryDescriptor : public IOMemoryDescriptor
{
	OSDeclareDefaultStructors(IOGeneralMemoryDescriptor);

public:
	union Ranges {
		IOVirtualRange   *v;
		IOAddressRange   *v64;
		IOPhysicalRange  *p;
		void             *uio;
	};
protected:
	Ranges              _ranges;
	unsigned            _rangesCount;                                         
#ifndef __LP64__
	bool                _rangesIsAllocated;                              
#endif                

	task_t              _task;                                                    

	union {
		IOVirtualRange  v;
		IOPhysicalRange p;
	}                   _singleRange;                                        

	unsigned            _wireCount;                                     

#ifndef __LP64__
	uintptr_t _cachedVirtualAddress;

	IOPhysicalAddress   _cachedPhysicalAddress;
#endif                

	bool                _initialized;                                        

public:
	virtual void free() APPLE_KEXT_OVERRIDE;

	virtual IOReturn dmaCommandOperation(DMACommandOps op, void *vData, UInt dataSize) const APPLE_KEXT_OVERRIDE;

	virtual uint64_t getPreparationID( void ) APPLE_KEXT_OVERRIDE;

#ifdef XNU_KERNEL_PRIVATE
                                                                
	IOReturn wireVirtual(IODirection forDirection);
	IOReturn dmaMap(
		IOMapper                    * mapper,
		IOMemoryDescriptor          * memory,
		IODMACommand                * command,
		const IODMAMapSpecification * mapSpec,
		uint64_t                      offset,
		uint64_t                      length,
		uint64_t                    * mapAddress,
		uint64_t                    * mapLength);
	bool initMemoryEntries(size_t size, IOMapper * mapper);

	IOMemoryReference * memoryReferenceAlloc(uint32_t capacity,
	    IOMemoryReference * realloc);
	void memoryReferenceFree(IOMemoryReference * ref);
	void memoryReferenceRelease(IOMemoryReference * ref);

	IOReturn memoryReferenceCreate(
		IOOptionBits         options,
		IOMemoryReference ** reference);

	IOReturn memoryReferenceMap(IOMemoryReference * ref,
	    vm_map_t            map,
	    mach_vm_size_t      inoffset,
	    mach_vm_size_t      size,
	    IOOptionBits        options,
	    mach_vm_address_t * inaddr);

	IOReturn memoryReferenceMapNew(IOMemoryReference * ref,
	    vm_map_t            map,
	    mach_vm_size_t      inoffset,
	    mach_vm_size_t      size,
	    IOOptionBits        options,
	    mach_vm_address_t * inaddr);

	static IOReturn memoryReferenceSetPurgeable(
		IOMemoryReference * ref,
		IOOptionBits newState,
		IOOptionBits * oldState);
	static IOReturn memoryReferenceSetOwnership(
		IOMemoryReference * ref,
		task_t newOwner,
		int newLedgerTag,
		IOOptionBits newLedgerOptions);
	static IOReturn memoryReferenceGetPageCounts(
		IOMemoryReference * ref,
		IOByteCount       * residentPageCount,
		IOByteCount       * dirtyPageCount);

	static uint64_t memoryReferenceGetDMAMapLength(
		IOMemoryReference * ref,
		uint64_t * offset);

#endif

private:

#ifndef __LP64__
	virtual void setPosition(IOByteCount position);
	virtual void mapIntoKernel(unsigned rangeIndex);
	virtual void unmapFromKernel();
#endif                

           
	OSPtr<OSData>   _memoryEntries;
	unsigned int    _pages;
	ppnum_t         _highestPage;
	uint32_t        __iomd_reservedA;
	uint32_t        __iomd_reservedB;

	IOLock *        _prepareLock;

public:
  
                                      
   

                    
	virtual bool initWithOptions(void *         buffers,
	    UInt32         count,
	    UInt32         offset,
	    task_t         task,
	    IOOptionBits   options,
	    IOMapper *     mapper = kIOMapperSystem) APPLE_KEXT_OVERRIDE;

#ifndef __LP64__
                         
	virtual bool initWithAddress(void *         address,
	    IOByteCount    withLength,
	    IODirection    withDirection) APPLE_KEXT_OVERRIDE APPLE_KEXT_DEPRECATED;

	virtual bool initWithAddress(IOVirtualAddress address,
	    IOByteCount    withLength,
	    IODirection    withDirection,
	    task_t         withTask) APPLE_KEXT_OVERRIDE APPLE_KEXT_DEPRECATED;

	virtual bool initWithPhysicalAddress(
		IOPhysicalAddress      address,
		IOByteCount            withLength,
		IODirection            withDirection ) APPLE_KEXT_OVERRIDE APPLE_KEXT_DEPRECATED;

	virtual bool initWithRanges(        IOVirtualRange * ranges,
	    UInt32           withCount,
	    IODirection      withDirection,
	    task_t           withTask,
	    bool             asReference = false) APPLE_KEXT_OVERRIDE APPLE_KEXT_DEPRECATED;

	virtual bool initWithPhysicalRanges(IOPhysicalRange * ranges,
	    UInt32           withCount,
	    IODirection      withDirection,
	    bool             asReference = false) APPLE_KEXT_OVERRIDE APPLE_KEXT_DEPRECATED;

	virtual addr64_t getPhysicalSegment64( IOByteCount offset,
	    IOByteCount * length ) APPLE_KEXT_OVERRIDE APPLE_KEXT_DEPRECATED;

	virtual IOPhysicalAddress getPhysicalSegment(IOByteCount offset,
	    IOByteCount * length) APPLE_KEXT_OVERRIDE;

	virtual IOPhysicalAddress getSourceSegment(IOByteCount offset,
	    IOByteCount * length) APPLE_KEXT_OVERRIDE APPLE_KEXT_DEPRECATED;

	virtual void * getVirtualSegment(IOByteCount offset,
	    IOByteCount * length) APPLE_KEXT_OVERRIDE APPLE_KEXT_DEPRECATED;
#endif                

	virtual IOReturn setPurgeable( IOOptionBits newState,
	    IOOptionBits * oldState ) APPLE_KEXT_OVERRIDE;

	IOReturn setOwnership( task_t newOwner,
	    int newLedgerTag,
	    IOOptionBits newLedgerOptions );

	virtual addr64_t getPhysicalSegment( IOByteCount   offset,
	    IOByteCount * length,
#ifdef __LP64__
	    IOOptionBits  options = 0 ) APPLE_KEXT_OVERRIDE;
#else                
	    IOOptionBits  options)APPLE_KEXT_OVERRIDE;
#endif                

	virtual IOReturn prepare(IODirection forDirection = kIODirectionNone) APPLE_KEXT_OVERRIDE;

	virtual IOReturn complete(IODirection forDirection = kIODirectionNone) APPLE_KEXT_OVERRIDE;

	virtual IOReturn doMap(
		vm_map_t                addressMap,
		IOVirtualAddress *      atAddress,
		IOOptionBits            options,
		IOByteCount             sourceOffset = 0,
		IOByteCount             length = 0 ) APPLE_KEXT_OVERRIDE;

	virtual IOReturn doUnmap(
		vm_map_t                addressMap,
		IOVirtualAddress        logical,
		IOByteCount             length ) APPLE_KEXT_OVERRIDE;

	virtual bool serialize(OSSerialize *s) const APPLE_KEXT_OVERRIDE;

                                                                       
	static OSPtr<IOMemoryDescriptor>
	withPersistentMemoryDescriptor(IOGeneralMemoryDescriptor *originalMD);
};

                                                                           

#ifdef __LP64__
mach_vm_address_t
IOMemoryMap::getAddress()
{
	return getVirtualAddress();
}

mach_vm_size_t
IOMemoryMap::getSize()
{
	return getLength();
}
#else                
#include <IOKit/IOSubMemoryDescriptor.h>
#endif                

                                                                           

extern bool iokit_iomd_setownership_enabled;

#endif                             
