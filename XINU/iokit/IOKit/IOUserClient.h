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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

  
                                    
   

#ifndef _IOKIT_IOUSERCLIENT_H
#define _IOKIT_IOUSERCLIENT_H

#include <IOKit/IOTypes.h>
#include <IOKit/IOService.h>
#include <IOKit/OSMessageNotification.h>
#include <DriverKit/IOUserClient.h>
#include <libkern/c++/OSPtr.h>

#if IOKITSTATS
#include <IOKit/IOStatisticsPrivate.h>
#endif

#define _IOUSERCLIENT_SENDASYNCRESULT64WITHOPTIONS_             1

enum {
	kIOUCTypeMask       = 0x0000000f,
	kIOUCScalarIScalarO = 0,
	kIOUCScalarIStructO = 2,
	kIOUCStructIStructO = 3,
	kIOUCScalarIStructI = 4,

	kIOUCForegroundOnly = 0x00000010,
};

         
                                                                                       
                                                                                                                                                                                   
   
enum {
	kIOUCVariableStructureSize = 0xffffffff
};


typedef IOReturn (IOService::*IOMethod)(void * p1, void * p2, void * p3,
    void * p4, void * p5, void * p6 );

typedef IOReturn (IOService::*IOAsyncMethod)(OSAsyncReference asyncRef,
    void * p1, void * p2, void * p3,
    void * p4, void * p5, void * p6 );

typedef IOReturn (IOService::*IOTrap)(void * p1, void * p2, void * p3,
    void * p4, void * p5, void * p6 );

struct IOExternalMethod {
	IOService *         object;
	IOMethod            func;
	IOOptionBits        flags;
	IOByteCount         count0;
	IOByteCount         count1;
};

struct IOExternalAsyncMethod {
	IOService *         object;
	IOAsyncMethod       func;
	IOOptionBits        flags;
	IOByteCount         count0;
	IOByteCount         count1;
};

struct IOExternalTrap {
	IOService *         object;
	IOTrap              func;
};

enum {
	kIOUserNotifyMaxMessageSize = 64
};

enum {
	kIOUserNotifyOptionCanDrop = 0x1                                                             
};

                              
#define kIOClientPrivilegeAdministrator "root"
#define kIOClientPrivilegeLocalUser     "local"
#define kIOClientPrivilegeForeground    "foreground"

         
                                                                                                                                                                                                                                             
                                                                                                          
                                                                                                            
   
enum {
	kIOExternalMethodScalarInputCountMax  = 16,
	kIOExternalMethodScalarOutputCountMax = 16,
};


struct IOExternalMethodArguments {
	uint32_t            version;

	uint32_t            selector;

	mach_port_t           asyncWakePort;
	io_user_reference_t * asyncReference;
	uint32_t              asyncReferenceCount;

	const uint64_t *    scalarInput;
	uint32_t            scalarInputCount;

	const void *        structureInput;
	uint32_t            structureInputSize;

	IOMemoryDescriptor * structureInputDescriptor;

	uint64_t *          scalarOutput;
	uint32_t            scalarOutputCount;

	void *              structureOutput;
	uint32_t            structureOutputSize;

	IOMemoryDescriptor * structureOutputDescriptor;
	uint32_t             structureOutputDescriptorSize;

	uint32_t            __reservedA;

	OSObject **         structureVariableOutputData;

	uint32_t            __reserved[30];
};

typedef IOReturn (*IOExternalMethodAction)(OSObject * target, void * reference,
    IOExternalMethodArguments * arguments);
struct IOExternalMethodDispatch {
	IOExternalMethodAction function;
	uint32_t               checkScalarInputCount;
	uint32_t               checkStructureInputSize;
	uint32_t               checkScalarOutputCount;
	uint32_t               checkStructureOutputSize;
};

enum {
#define IO_EXTERNAL_METHOD_ARGUMENTS_CURRENT_VERSION    2
	kIOExternalMethodArgumentsCurrentVersion = IO_EXTERNAL_METHOD_ARGUMENTS_CURRENT_VERSION
};

#if PRIVATE
typedef uintptr_t io_filter_policy_t;
enum io_filter_type_t {
	io_filter_type_external_method       = 1,
	io_filter_type_external_async_method = 2,
	io_filter_type_trap                  = 3,
};

typedef IOReturn (*io_filter_resolver_t) (task_t task, IOUserClient * client, uint32_t type, io_filter_policy_t *filterp);
typedef IOReturn (*io_filter_applier_t) (io_filter_policy_t filter, io_filter_type_t type, uint32_t selector);
typedef void (*io_filter_release_t) (io_filter_policy_t filter);
struct io_filter_callbacks {
	const io_filter_resolver_t      io_filter_resolver;
	const io_filter_applier_t       io_filter_applier;
	const io_filter_release_t       io_filter_release;
};
struct IOUCFilterPolicy;
#endif              

   
                        
                                                                                                    
   

class IOUserClient : public IOService
{
	OSDeclareAbstractStructorsWithDispatch(IOUserClient);
#if IOKITSTATS
	friend class IOStatistics;
#endif

#if XNU_KERNEL_PRIVATE
public:
#else                         
protected:
#endif                          
                         
                                                                                                    
   
	struct ExpansionData {
#if IOKITSTATS
		IOUserClientCounter *counter;
#else
		void *iokitstatsReserved;
#endif
#if PRIVATE
		IOUCFilterPolicy * filterPolicies;
#else
		void *iokitFilterReserved;
#endif
	};

                 
                                                  
   
	APPLE_KEXT_WSHADOW_PUSH;
	ExpansionData * reserved;
	APPLE_KEXT_WSHADOW_POP;

	bool reserve();

#ifdef XNU_KERNEL_PRIVATE

public:
	OSSet * mappings;
	UInt8   sharedInstance;
	UInt8   closed;
	UInt8   __ipcFinal;
	UInt8   messageAppSuspended:1,
	    defaultLocking:1,
	    __reservedA:6;
	volatile SInt32 __ipc;
	queue_head_t owners;
	IORWLock * lock;
	IOLock   * filterLock;
#if __LP64__
	void  * __reserved[3];
#else
	void  * __reserved[2];
#endif

#else                         
private:
	void  * __reserved[9];
#endif                         

public:
	MIG_SERVER_ROUTINE virtual IOReturn
	externalMethod(uint32_t selector, IOExternalMethodArguments *arguments,
	    IOExternalMethodDispatch *dispatch = NULL,
	    OSObject *target = NULL, void *reference = NULL);

	MIG_SERVER_ROUTINE virtual IOReturn registerNotificationPort(
		mach_port_t port, UInt32 type, io_user_reference_t refCon);

private:
#if __LP64__
	OSMetaClassDeclareReservedUnused(IOUserClient, 0);
	OSMetaClassDeclareReservedUnused(IOUserClient, 1);
#else
	OSMetaClassDeclareReservedUsedX86(IOUserClient, 0);
	OSMetaClassDeclareReservedUsedX86(IOUserClient, 1);
#endif
	OSMetaClassDeclareReservedUnused(IOUserClient, 2);
	OSMetaClassDeclareReservedUnused(IOUserClient, 3);
	OSMetaClassDeclareReservedUnused(IOUserClient, 4);
	OSMetaClassDeclareReservedUnused(IOUserClient, 5);
	OSMetaClassDeclareReservedUnused(IOUserClient, 6);
	OSMetaClassDeclareReservedUnused(IOUserClient, 7);
	OSMetaClassDeclareReservedUnused(IOUserClient, 8);
	OSMetaClassDeclareReservedUnused(IOUserClient, 9);
	OSMetaClassDeclareReservedUnused(IOUserClient, 10);
	OSMetaClassDeclareReservedUnused(IOUserClient, 11);
	OSMetaClassDeclareReservedUnused(IOUserClient, 12);
	OSMetaClassDeclareReservedUnused(IOUserClient, 13);
	OSMetaClassDeclareReservedUnused(IOUserClient, 14);
	OSMetaClassDeclareReservedUnused(IOUserClient, 15);

#ifdef XNU_KERNEL_PRIVATE

                                      
public:
	static void initialize( void );
	static void destroyUserReferences( OSObject * obj );
	static bool finalizeUserReferences( OSObject * obj );
	OSPtr<IOMemoryMap>  mapClientMemory64( IOOptionBits type,
	    task_t task,
	    IOOptionBits mapFlags = kIOMapAnywhere,
	    mach_vm_address_t atAddress = 0 );
	IOReturn registerOwner(task_t task);
	void     noMoreSenders(void);
	io_filter_policy_t filterForTask(task_t task, io_filter_policy_t addFilterPolicy);
#endif                         

#if PRIVATE
public:
	static IOReturn registerFilterCallbacks(const struct io_filter_callbacks *callbacks, size_t size);
#endif              

protected:
	static IOReturn sendAsyncResult(OSAsyncReference reference,
	    IOReturn result, void *args[], UInt32 numArgs);
	static void setAsyncReference(OSAsyncReference asyncRef,
	    mach_port_t wakePort,
	    void *callback, void *refcon);

	static IOReturn sendAsyncResult64(OSAsyncReference64 reference,
	    IOReturn result, io_user_reference_t args[], UInt32 numArgs);

   
                                           
                                                                                     
                                                                                             
                                                                                           
                                                                                              
   
	static IOReturn sendAsyncResult64WithOptions(OSAsyncReference64 reference,
	    IOReturn result, io_user_reference_t args[], UInt32 numArgs,
	    IOOptionBits options);

	static void setAsyncReference64(OSAsyncReference64 asyncRef,
	    mach_port_t wakePort,
	    mach_vm_address_t callback, io_user_reference_t refcon);

	static void setAsyncReference64(OSAsyncReference64 asyncRef,
	    mach_port_t wakePort,
	    mach_vm_address_t callback, io_user_reference_t refcon,
	    task_t task);

public:

	static IOReturn clientHasAuthorization( task_t task,
	    IOService * service );

	static IOReturn clientHasPrivilege( void * securityToken,
	    const char * privilegeName );

	static OSPtr<OSObject>  copyClientEntitlement(task_t task, const char *entitlement);
	static OSPtr<OSObject>  copyClientEntitlementVnode(struct vnode *vnode, off_t offset, const char *entitlement);

	static OSPtr<OSDictionary>  copyClientEntitlements(task_t task);
	static OSPtr<OSDictionary>  copyClientEntitlementsVnode(struct vnode *vnode, off_t offset);
	static OSPtr<OSDictionary>  copyEntitlementsFromBlob(void *blob, size_t len);

   
                                      
                                                                                              
                                                                                                                                                                                                                                                 
                                                                                                                                                    
                           
   
	static IOReturn releaseAsyncReference64(OSAsyncReference64 reference);
   
                                      
                                                                            
                                                                                                                                                                                                                         
                                                                                                                         
                           
   
	static IOReturn releaseNotificationPort(mach_port_t port);

	virtual bool init() APPLE_KEXT_OVERRIDE;
	virtual bool init( OSDictionary * dictionary ) APPLE_KEXT_OVERRIDE;
                                                                      
	virtual bool initWithTask(
		task_t owningTask, void * securityToken, UInt32 type,
		OSDictionary * properties);

	virtual bool initWithTask(
		task_t owningTask, void * securityToken, UInt32 type);

	virtual void free() APPLE_KEXT_OVERRIDE;

	virtual IOReturn clientClose( void );
	virtual IOReturn clientDied( void );

	virtual IOService * getService( void );

	MIG_SERVER_ROUTINE virtual IOReturn registerNotificationPort(
		mach_port_t port, UInt32 type, UInt32 refCon );

	MIG_SERVER_ROUTINE virtual IOReturn getNotificationSemaphore( UInt32 notification_type,
	    semaphore_t * semaphore );

	virtual IOReturn connectClient( IOUserClient * client );

                                                                    
	virtual IOReturn clientMemoryForType( UInt32 type,
	    IOOptionBits * options,
	    IOMemoryDescriptor ** memory );

	IOReturn clientMemoryForType( UInt32 type,
	    IOOptionBits * options,
	    OSSharedPtr<IOMemoryDescriptor>& memory );

#if !__LP64__
private:
	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	OSPtr<IOMemoryMap>  mapClientMemory( IOOptionBits type,
	    task_t task,
	    IOOptionBits mapFlags = kIOMapAnywhere,
	    IOVirtualAddress atAddress = 0 );
#endif

	static IOReturn _sendAsyncResult64(OSAsyncReference64 reference,
	    IOReturn result, io_user_reference_t args[], UInt32 numArgs, IOOptionBits options);
public:

   
                                         
                                                                                                                                                                                                                                                             
                                                                                                                     
                                                                                                                                                                                                                                 
   
	OSPtr<IOMemoryMap>  removeMappingForDescriptor(IOMemoryDescriptor * memory);

   
                                   
                                                             
                          
                                                           
                                                               
   
	virtual IOReturn exportObjectToClient(task_t task,
	    LIBKERN_CONSUMED OSObject *obj, io_object_t *clientObj);

#if KERNEL_PRIVATE

   
                                          
                                                                            
                                                      
                          
                                                              
                                                                                                          
                                                                                                             
                           
   
	static IOReturn copyPortNameForObjectInTask(task_t task, OSObject *object,
	    mach_port_name_t * port_name);

   
                                          
                                                                                             
                          
                                                                                                          
                                                                                                                         
                           
   
	static IOReturn copyObjectForPortNameInTask(task_t task, mach_port_name_t port_name,
	    OSObject **object);

	static IOReturn copyObjectForPortNameInTask(task_t task, mach_port_name_t port_name,
	    OSSharedPtr<OSObject>& object);

   
                                             
                                                                                       
                          
                                           
                                                                       
                           
   
	static IOReturn adjustPortNameReferencesInTask(task_t task, mach_port_name_t port_name, mach_port_delta_t delta);

#define IOUC_COPYPORTNAMEFOROBJECTINTASK    1

#endif                     

                                                                       
	virtual IOExternalMethod *
	getExternalMethodForIndex( UInt32 index )
	APPLE_KEXT_DEPRECATED;
	virtual IOExternalAsyncMethod *
	getExternalAsyncMethodForIndex( UInt32 index )
	APPLE_KEXT_DEPRECATED;

                                       
	virtual IOExternalMethod *
	getTargetAndMethodForIndex(
		LIBKERN_RETURNS_NOT_RETAINED IOService ** targetP, UInt32 index );
	virtual IOExternalAsyncMethod *
	getAsyncTargetAndMethodForIndex(
		LIBKERN_RETURNS_NOT_RETAINED IOService ** targetP, UInt32 index );
	IOExternalMethod *
	getTargetAndMethodForIndex(
		OSSharedPtr<IOService>& targetP, UInt32 index );
	IOExternalAsyncMethod *
	getAsyncTargetAndMethodForIndex(
		OSSharedPtr<IOService>& targetP, UInt32 index );

                                                        
	virtual IOExternalTrap *
	getExternalTrapForIndex( UInt32 index )
	APPLE_KEXT_DEPRECATED;

	virtual IOExternalTrap *
	getTargetAndTrapForIndex(
		LIBKERN_RETURNS_NOT_RETAINED IOService **targetP, UInt32 index );
};

#ifdef XNU_KERNEL_PRIVATE
extern "C" void IOMachPortDestroyUserReferences(OSObject * obj, natural_t type);
#endif                         

#endif                              
