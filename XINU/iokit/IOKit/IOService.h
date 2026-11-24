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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1998,1999 Apple Computer, Inc.  All rights reserved.
 *
 * HISTORY
 *
 */
   
            
                                                                                                                                                                                                                                                                                                                                                                             
  
                                                                      
   

#ifndef _IOKIT_IOSERVICE_H
#define _IOKIT_IOSERVICE_H

#include <IOKit/IORegistryEntry.h>
#include <IOKit/IOReturn.h>
#include <IOKit/IODeviceMemory.h>
#include <IOKit/IONotifier.h>
#include <IOKit/IOLocks.h>

#include <IOKit/IOKitDebug.h>
#include <IOKit/IOInterrupts.h>

#include <IOKit/pwr_mgt/IOPMpowerState.h>
#include <IOKit/IOServicePM.h>
#include <IOKit/IOReportTypes.h>
#include <DriverKit/IOService.h>
#include <libkern/c++/OSPtr.h>

extern "C" {
#include <kern/thread_call.h>
}


#ifndef UINT64_MAX
#define UINT64_MAX        18446744073709551615ULL
#endif


enum {
	kIODefaultProbeScore    = 0
};

                       
enum {
	kIOServiceInactiveState = 0x00000001,
	kIOServiceRegisteredState   = 0x00000002,
	kIOServiceMatchedState  = 0x00000004,
	kIOServiceFirstPublishState = 0x00000008,
	kIOServiceFirstMatchState   = 0x00000010
};

enum {
	                                
	kIOServiceExclusive     = 0x00000001,

	                          
	kIOServiceRequired      = 0x00000001,
	kIOServiceTerminate     = 0x00000004,
	kIOServiceTerminateWithRematch = 0x00000010,

	                                              
	kIOServiceSynchronous   = 0x00000002,
	                                
	kIOServiceAsynchronous  = 0x00000008
};

                     
enum {
	kIOServiceSeize     = 0x00000001,
	kIOServiceFamilyOpenOptions = 0xffff0000
};

                      
enum {
	kIOServiceFamilyCloseOptions = 0xffff0000
};

typedef void * IONotificationRef;

extern const IORegistryPlane *  gIOServicePlane;
extern const IORegistryPlane *  gIOPowerPlane;

extern const OSSymbol *     gIOResourcesKey;
extern const OSSymbol *     gIOResourceMatchKey;
extern const OSSymbol *     gIOResourceMatchedKey;
extern const OSSymbol *     gIOResourceIOKitKey;

extern const OSSymbol *     gIOProviderClassKey;
extern const OSSymbol *     gIONameMatchKey;
extern const OSSymbol *     gIONameMatchedKey;
extern const OSSymbol *     gIOPropertyMatchKey;
extern const OSSymbol *     gIOPropertyExistsMatchKey;
extern const OSSymbol *     gIOLocationMatchKey;
extern const OSSymbol *     gIOParentMatchKey;
extern const OSSymbol *     gIOPathMatchKey;
extern const OSSymbol *     gIOMatchCategoryKey;
extern const OSSymbol *     gIODefaultMatchCategoryKey;
extern const OSSymbol *     gIOMatchedServiceCountKey;
extern const OSSymbol *     gIOMatchedPersonalityKey;
extern const OSSymbol *     gIORematchPersonalityKey;
extern const OSSymbol *     gIORematchCountKey;
extern const OSSymbol *     gIODEXTMatchCountKey;

extern const OSSymbol *     gIOUserClientClassKey;

extern const OSSymbol *     gIOUserClassKey;
extern const OSSymbol *     gIOUserServerClassKey;
extern const OSSymbol *     gIOUserServerNameKey;
extern const OSSymbol *     gIOUserServerTagKey;
extern const OSSymbol *     gIOUserUserClientKey;

extern const OSSymbol *     gIOKitDebugKey;
extern const OSSymbol *     gIOServiceKey;

extern const OSSymbol *     gIOCommandPoolSizeKey;

extern const OSSymbol *     gIOPublishNotification;
extern const OSSymbol *     gIOFirstPublishNotification;
extern const OSSymbol *     gIOMatchedNotification;
extern const OSSymbol *     gIOFirstMatchNotification;
extern const OSSymbol *     gIOTerminatedNotification;
extern const OSSymbol *     gIOWillTerminateNotification;

extern const OSSymbol *     gIOGeneralInterest;
extern const OSSymbol *     gIOBusyInterest;
extern const OSSymbol *     gIOOpenInterest;
extern const OSSymbol *     gIOAppPowerStateInterest;
extern const OSSymbol *     gIOPriorityPowerStateInterest;
extern const OSSymbol *     gIOConsoleSecurityInterest;

extern const OSSymbol *     gIODeviceMemoryKey;
extern const OSSymbol *     gIOInterruptControllersKey;
extern const OSSymbol *     gIOInterruptSpecifiersKey;

extern const OSSymbol *     gIOSupportedPropertiesKey;
extern const OSSymbol *     gIOUserServicePropertiesKey;
extern const OSSymbol *     gIOCompatibilityMatchKey;
extern const OSSymbol *     gIOCompatibilityPropertiesKey;
extern const OSSymbol *     gIOPathKey;

extern const OSSymbol *     gIOBSDKey;
extern const OSSymbol *     gIOBSDNameKey;
extern const OSSymbol *     gIOBSDMajorKey;
extern const OSSymbol *     gIOBSDMinorKey;
extern const OSSymbol *     gIOBSDUnitKey;

extern const OSSymbol *     gIODriverKitEntitlementKey;
extern const OSSymbol *     gIOServiceDEXTEntitlementsKey;
extern const OSSymbol *     gIODriverKitUserClientEntitlementsKey;
extern const OSSymbol *     gIODriverKitUserClientEntitlementAllowAnyKey;
extern const OSSymbol *     gIOMatchDeferKey;
extern const OSSymbol *     gIOAllCPUInitializedKey;

#if XNU_KERNEL_PRIVATE && !defined(IOServiceTrace)

#include <IOKit/IOTimeStamp.h>
#if (KDEBUG_LEVEL >= KDEBUG_LEVEL_STANDARD)
#define IOServiceTrace(csc, a, b, c, d) do {                            \
    if(kIOTraceIOService & gIOKitTrace) {                               \
	KERNEL_DEBUG_CONSTANT(IODBG_IOSERVICE(csc), a, b, c, d, 0);     \
    }                                                                   \
} while(0)
#else                                              
#define IOServiceTrace(csc, a, b, c, d) do {    \
  (void)a;                                      \
  (void)b;                                      \
  (void)c;                                      \
  (void)d;                                      \
} while (0)
#endif                                              

#endif                                         

extern SInt32 IOServiceOrdering( const OSMetaClassBase * inObj1, const OSMetaClassBase * inObj2, void * ref );

typedef void (*IOInterruptAction)( OSObject * target, void * refCon,
    IOService * nub, int source );

#ifdef __BLOCKS__
typedef void (^IOInterruptActionBlock)(IOService * nub, int source);
#endif                 

                                         
                                                                           
                                                                                    
                                                                                                                                                                                          

typedef bool (*IOServiceNotificationHandler)( void * target, void * refCon,
    IOService * newService );

typedef bool (*IOServiceMatchingNotificationHandler)( void * target, void * refCon,
    IOService * newService,
    IONotifier * notifier );

#ifdef __BLOCKS__
typedef bool (^IOServiceMatchingNotificationHandlerBlock)(IOService * newService,
    IONotifier * notifier );
#endif                 


                                     
                                                                           
                                                                                    
                                                                                                    
                                                                                                                                                                                         
                                                                           
                                                                                                                                   

typedef IOReturn (*IOServiceInterestHandler)( void * target, void * refCon,
    UInt32 messageType, IOService * provider,
    void * messageArgument, vm_size_t argSize );

#ifdef __BLOCKS__
typedef IOReturn (^IOServiceInterestHandlerBlock)( uint32_t messageType, IOService * provider,
    void   * messageArgument, size_t argSize );
#endif                 

typedef void (*IOServiceApplierFunction)(IOService * service, void * context);
typedef void (*OSObjectApplierFunction)(OSObject * object, void * context);
#ifdef __BLOCKS__
typedef void (^IOServiceApplierBlock)(IOService * service);
typedef void (^OSObjectApplierBlock)(OSObject * object);
#endif                 


class IOUserClient;
class IOPlatformExpert;
class IOUserServerCheckInToken;

                    
                                                                              
                                                                                                                                                                                                                                                                                                                                         
  
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
  
                                                                                                                                                                   
  
                                                                                                                                                                                                                                                   
  
                                                     
  
                                                                                                                                                                                                                                                                                                                     
  
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
  
                                                                                                                                                        
  
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
  
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
  
                                                                                                                                                                                                                                                                                                                      
  
                                                                                                                                                                                                                                                                                        
  
                                                                                                                                                   
  
                                                
  
                                                                             
        
        
                                                             
        
        
                                                                                                     
        
        
                                                                                                                     
        
        
                                                                                            
        
        
                                                                      
        
        
                                                                                                     
        
        
                                                                                                                                                                                                                                            
        
        
                                                                                         
        
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
        
            
         
              
                           
                               
               
          
  
                                                                                         
         
              
                           
            
                                      
                                   
             
               
          
  
        
                                                                                               
        
                                                                                                                                                                                 
        
        
                                                                                                     
        
                                                                                                                                              
  
         
              
                               
                 
                                
                                    
                  
               
          
  
        
                                                                                                           
        
                                                                                                                                                                                         
        
        
                                                                                      
        
                                                                                                                                                           

struct IOInterruptAccountingData;
struct IOInterruptAccountingReporter;
struct OSObjectUserVars;

class IOService : public IORegistryEntry
{
	OSDeclareDefaultStructorsWithDispatch(IOService);

#if XNU_KERNEL_PRIVATE
public:
#else
protected:
#endif                          
                         
                                                                                                    
   
	struct ExpansionData {
		uint64_t authorizationID;
		  
                                                                          
                                                                           
                                                                           
                                                                            
                                                       
     
		IOLock * interruptStatisticsLock;
		IOInterruptAccountingReporter * interruptStatisticsArray;
		int interruptStatisticsArrayCount;

		OSObjectUserVars * uvars;
	};

                 
                                                      
	APPLE_KEXT_WSHADOW_PUSH;
	ExpansionData * reserved;
	APPLE_KEXT_WSHADOW_POP;

private:
	IOService *     __provider;
	SInt32      __providerGeneration;
	IOService *     __owner;
	IOOptionBits    __state[2];
	uint64_t        __timeBusy;
	uint64_t        __accumBusy;
	IOServicePM *   pwrMgt;

protected:
                                   
	bool            initialized;

public:
             
	void *          pm_vars;

public:
                                                 
                              
                                                 
                                                                                                                                                                                                                                                                                                                                                                                                                            
                                                            
                                                                                                  
                                                                                                     

	virtual bool requestTerminate( IOService * provider, IOOptionBits options );

                           
                                                 
                                                                                                                             
                                                            
                                                           
                                  

	virtual bool willTerminate( IOService * provider, IOOptionBits options );

                          
                                                 
                                                                                                                            
                                                            
                                                           
                                                                                                                                                                                                                                                                                                                                                        
                                  

	virtual bool didTerminate( IOService * provider, IOOptionBits options, bool * defer );

                             
                                           
                                                                             
                                                                                                                                                     
  
                                                                                                                                                         
                                        
                                                                 
                                                        
                                                                                                            

	virtual SInt32 nextIdleTimeout(AbsoluteTime currentTime,
	    AbsoluteTime lastActivity, unsigned int powerState);

                                
                                           
                                                                                  
                                                                                                                                                                                                                                               
  
                                                                                                                                                                                                 
                                                                                                               

	virtual void systemWillShutdown( IOOptionBits specifier );

                                    
                                           
                                                                                       
                                                                                                                             
   

	virtual IOService * copyClientWithCategory( const OSSymbol * category );

public:
                                   
                                                   
                                                        
  
                                                
                                                   
                                                         
                                                              
   
	virtual IOReturn configureReport(IOReportChannelList   *channels,
	    IOReportConfigureAction action,
	    void                  *result,
	    void                  *destination);

                                
                                                                    
                                                       
  
                                                 
                                               
                                                                  
                                                                        
   
	virtual IOReturn updateReport(IOReportChannelList      *channels,
	    IOReportUpdateAction      action,
	    void                     *result,
	    void                     *destination);

private:
#if __LP64__
	OSMetaClassDeclareReservedUsedX86(IOService, 0);
	OSMetaClassDeclareReservedUsedX86(IOService, 1);
	OSMetaClassDeclareReservedUnused(IOService, 2);
	OSMetaClassDeclareReservedUnused(IOService, 3);
	OSMetaClassDeclareReservedUnused(IOService, 4);
	OSMetaClassDeclareReservedUnused(IOService, 5);
	OSMetaClassDeclareReservedUnused(IOService, 6);
	OSMetaClassDeclareReservedUnused(IOService, 7);
#else
	OSMetaClassDeclareReservedUsedX86(IOService, 0);
	OSMetaClassDeclareReservedUsedX86(IOService, 1);
	OSMetaClassDeclareReservedUsedX86(IOService, 2);
	OSMetaClassDeclareReservedUsedX86(IOService, 3);
	OSMetaClassDeclareReservedUsedX86(IOService, 4);
	OSMetaClassDeclareReservedUsedX86(IOService, 5);
	OSMetaClassDeclareReservedUsedX86(IOService, 6);
	OSMetaClassDeclareReservedUsedX86(IOService, 7);
#endif

	OSMetaClassDeclareReservedUnused(IOService, 8);
	OSMetaClassDeclareReservedUnused(IOService, 9);
	OSMetaClassDeclareReservedUnused(IOService, 10);
	OSMetaClassDeclareReservedUnused(IOService, 11);
	OSMetaClassDeclareReservedUnused(IOService, 12);
	OSMetaClassDeclareReservedUnused(IOService, 13);
	OSMetaClassDeclareReservedUnused(IOService, 14);
	OSMetaClassDeclareReservedUnused(IOService, 15);
	OSMetaClassDeclareReservedUnused(IOService, 16);
	OSMetaClassDeclareReservedUnused(IOService, 17);
	OSMetaClassDeclareReservedUnused(IOService, 18);
	OSMetaClassDeclareReservedUnused(IOService, 19);
	OSMetaClassDeclareReservedUnused(IOService, 20);
	OSMetaClassDeclareReservedUnused(IOService, 21);
	OSMetaClassDeclareReservedUnused(IOService, 22);
	OSMetaClassDeclareReservedUnused(IOService, 23);
	OSMetaClassDeclareReservedUnused(IOService, 24);
	OSMetaClassDeclareReservedUnused(IOService, 25);
	OSMetaClassDeclareReservedUnused(IOService, 26);
	OSMetaClassDeclareReservedUnused(IOService, 27);
	OSMetaClassDeclareReservedUnused(IOService, 28);
	OSMetaClassDeclareReservedUnused(IOService, 29);
	OSMetaClassDeclareReservedUnused(IOService, 30);
	OSMetaClassDeclareReservedUnused(IOService, 31);
	OSMetaClassDeclareReservedUnused(IOService, 32);
	OSMetaClassDeclareReservedUnused(IOService, 33);
	OSMetaClassDeclareReservedUnused(IOService, 34);
	OSMetaClassDeclareReservedUnused(IOService, 35);
	OSMetaClassDeclareReservedUnused(IOService, 36);
	OSMetaClassDeclareReservedUnused(IOService, 37);
	OSMetaClassDeclareReservedUnused(IOService, 38);
	OSMetaClassDeclareReservedUnused(IOService, 39);
	OSMetaClassDeclareReservedUnused(IOService, 40);
	OSMetaClassDeclareReservedUnused(IOService, 41);
	OSMetaClassDeclareReservedUnused(IOService, 42);
	OSMetaClassDeclareReservedUnused(IOService, 43);
	OSMetaClassDeclareReservedUnused(IOService, 44);
	OSMetaClassDeclareReservedUnused(IOService, 45);
	OSMetaClassDeclareReservedUnused(IOService, 46);
	OSMetaClassDeclareReservedUnused(IOService, 47);

public:
                      
                                                                                                
                                                                                                                               

	virtual IOOptionBits getState( void ) const;

                        
                                                                                                            
                                                                                                                                                                                                                                                                                
                                                                              

	bool isInactive( void ) const;

                    

                             
                                                                                       
                                                                                                                                                                                                                                                                                                                                          
                                                                                                                                                                                                                                                                                                                                                                                                     

	virtual void registerService( IOOptionBits options = 0 );

                   
                                                                                                             
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
                                                                                                             
                                                                                                                                                                                                                                                                                                                                                                                 
                                                                                                                                                                                                                                                                                                                   

	virtual LIBKERN_RETURNS_NOT_RETAINED IOService * probe(  IOService *     provider,
	    SInt32    *     score );

                   
                                                                                                                                     
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                                                                                                                             

	virtual bool start( IOService * provider );

                  
                                                                                                                                    
                                                                                                                                                                                                          

	virtual void stop( IOService * provider );

                  

                  
                                                    
                                                                                                                                                                                                                                                                                                                                                                                                                                                      
                                                                                
                                                                                                                                                                                                            
                                                                         
                                                                                           

	virtual bool open(   IOService *       forClient,
	    IOOptionBits      options = 0,
	    void *        arg = NULL );

                   
                                                    
                                                                                                                                                                                                                                                                                                                                                                              
                                                                                 
                                                                                                                                    

	virtual void close(  IOService *       forClient,
	    IOOptionBits      options = 0 );

                    
                                                                                          
                                                                                                                                      
                                                                                                                                                                             
                                                                                                

	virtual bool isOpen( const IOService * forClient = NULL ) const;

                        
                                                                                                      
                                                                                                                                                                                                                                                                                                                                                                                     
                                                                                
                                                                                                           
                                                                                          

	virtual bool handleOpen(    IOService *   forClient,
	    IOOptionBits      options,
	    void *        arg );

                         
                                                                                                      
                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                 
                                                                                                                         

	virtual void handleClose(   IOService *       forClient,
	    IOOptionBits      options );

                          
                                                                                                      
                                                                                                                                                                                                                                                                                                                                                                                           
                                                                                                                                                                             
                                                                                                

	virtual bool handleIsOpen(  const IOService * forClient ) const;

                     

                       
                                                                             
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                          

	virtual bool terminate( IOOptionBits options = 0 );

                      
                                                                
                                                                                                                                                                                                                                                                                                                                                                                                                          
                                                                                                                                                    
                                  

	virtual bool finalize( IOOptionBits options );

                  
                                                                                      
	virtual bool init( OSDictionary * dictionary = NULL ) APPLE_KEXT_OVERRIDE;

                  
                                                                                      
	virtual bool init( IORegistryEntry * from,
	    const IORegistryPlane * inPlane ) APPLE_KEXT_OVERRIDE;

                  
                                                                                                                  

	virtual void free( void ) APPLE_KEXT_OVERRIDE;

                                
                                                                               
                                                                                                                                                                                                                                                     
                                                                                                                                                                                

	virtual bool lockForArbitration( bool isSuccessRequired = true );

                                  
                                                                                                                
                                                                                                                                      

	virtual void unlockForArbitration( void );

#ifdef XNU_KERNEL_PRIVATE
	static uint32_t isLockedForArbitration(IOService * service);
#endif                         


                             
                                                 
                                                                                                                                                                                                                                                                                                                                                                                                                  
                                                         
                                                                                                                        
                                                              

	virtual bool terminateClient( IOService * client, IOOptionBits options );

                                                                            

                          
                                                                         
                                                                                                                                                                                                                                                                                                                                                                                                                                                               
                                                 

	virtual UInt32 getBusyState( void );

                        
                                                                         
                                                                                                                                                                                                                                             
                                                                                              

	virtual void adjustBusy( SInt32 delta );

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOReturn waitQuiet(mach_timespec_t * timeout)
	APPLE_KEXT_DEPRECATED;

                       
                                                                                 
                                                                         
                                                                                        
                                                                                                                                               

	IOReturn waitQuiet(uint64_t timeout = UINT64_MAX);

              

                                
                                                                                          
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                     
                                                                                                                                                                                                                                                                                                        
                                                                                                                                                            

	virtual bool matchPropertyTable( OSDictionary * table,
	    SInt32       * score );

	virtual bool matchPropertyTable( OSDictionary * table );

                           
                                                                                
                                                                                                                                                                           
                                                                          
                                                                                

	virtual LIBKERN_RETURNS_NOT_RETAINED IOService * matchLocation( IOService * client );

                      

                             
                                                               
                                                                                                                                                                                                                                                                                    
                                                                    
                                                

	static void publishResource( const OSSymbol * key, OSObject * value = NULL );
	static void publishUserResource( const OSSymbol * key, OSObject * value = NULL );

                             
                                                               
                                                                                                                                                                                                                                                                                           
                                                                   
                                                

	static void publishResource( const char * key, OSObject * value = NULL );
	virtual bool addNeededResource( const char * key );

                   

                             
                                                                                                                                   
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                                                      
                                                                                                 
                                                                                                                                                                                                                    
                                                                                                                                                                   
                                                                                                                                                                                                                                             
                                                                                                                                                                                                   
                                                                                                                                          
                                                                                                                                                                                                             
                                                                   
                                                                
                                                            
                                                                          
                                                                                                                    

	static OSPtr<IONotifier>  addNotification(
		const OSSymbol * type, OSDictionary * matching,
		IOServiceNotificationHandler handler,
		void * target, void * ref = NULL,
		SInt32 priority = 0 )
	APPLE_KEXT_DEPRECATED;

                                     
                                                                                         
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
                                                                                      
                                                                                                 
                                                                                                                                                                                                                    
                                                                                                                                                                   
                                                                                                                                                                                                                                             
                                                                                                                                                                                                   
                                                                                                                                          
                                                                                                                                                                                                        
                                                                   
                                                                
                                                            
                                                                          
                                                                                                                    

	static IONotifier * addMatchingNotification(
		const OSSymbol * type, OSDictionary * matching,
		IOServiceMatchingNotificationHandler handler,
		void * target, void * ref = NULL,
		SInt32 priority = 0 );


#ifdef __BLOCKS__
	static IONotifier * addMatchingNotification(
		const OSSymbol * type, OSDictionary * matching,
		SInt32 priority,
		IOServiceMatchingNotificationHandlerBlock handler);
#endif                 

                            
                                                                                                        
                                                                                                                                                   
                                                                                                                                                                    
                                             
                                                                              

	static LIBKERN_RETURNS_NOT_RETAINED IOService * waitForService(
		LIBKERN_CONSUMED OSDictionary * matching,
		mach_timespec_t * timeout = NULL);

                                    
                                                               
                                                                                                                                                   
                                                                                                                                                                                                                                          
                                                                                        
                                                                                                                                                                                                                                                             

	static OSPtr<IOService>  waitForMatchingService( OSDictionary * matching,
	    uint64_t timeout = UINT64_MAX);

#ifdef XNU_KERNEL_PRIVATE
	static IOService * waitForMatchingServiceWithToken( OSDictionary * matching,
	    uint64_t timeout, IOUserServerCheckInToken * token );
#endif

                                 
                                                                                                   
                                                                                                                                       
                                                                                      
                                                                                                         

	static OSPtr<OSIterator> getMatchingServices( OSDictionary * matching );

                                 
                                                                                                   
                                                                                                                                          
                                                                                     
                                                                           

	static OSPtr<IOService>  copyMatchingService( OSDictionary * matching );

public:
                                                          
                                                             

                             
                                                                                                                                           
                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                                                                           
                                                                                                                                                                                             
                                                                                                          

	static OSPtr<OSDictionary>  serviceMatching( const char * className,
	    OSDictionary * table = NULL );

                             
                                                                                                                                           
                                                                                                                                                                                                                                                                                                                                                                                                       
                                                                                                                                                                
                                                                                                                                                                                             
                                                                                                          

	static OSPtr<OSDictionary>  serviceMatching( const OSString * className,
	    OSDictionary * table = NULL );

                          
                                                                                                                                          
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                                                                                                                                                             
                                                                                                                                                                                          
                                                                                                          

	static OSPtr<OSDictionary>  nameMatching( const char * name,
	    OSDictionary * table = NULL );

                          
                                                                                                                                          
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                                                                                                                                                                                  
                                                                                                                                                                                          
                                                                                                          

	static OSPtr<OSDictionary>  nameMatching( const OSString* name,
	    OSDictionary * table = NULL );

                              
                                                                                                                                           
                                                                                                                                                                                                                        
                                                                                                                                                                                     
                                                                                                                                                                                              
                                                                                                          

	static OSPtr<OSDictionary>  resourceMatching( const char * name,
	    OSDictionary * table = NULL );

                              
                                                                                                                                           
                                                                                                                                                                                                                        
                                                                                                                                                                                                          
                                                                                                                                                                                              
                                                                                                          

	static OSPtr<OSDictionary>  resourceMatching( const OSString * name,
	    OSDictionary * table = NULL );


                              
                                                                                                                                             
                                                                                                                                                                                                                                                                                                                                                                                           
                                                                                                                                                                                  
                                                                                                                                                                               
                                                                                                                                                                                
                                                                                                          

	static OSPtr<OSDictionary>  propertyMatching( const OSSymbol * key, const OSObject * value,
	    OSDictionary * table = NULL );

                                     
                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                             
                                                                                                                                                                            
                                                                                                                                                                                                     
                                                                                                          

	static OSDictionary * registryEntryIDMatching( uint64_t entryID,
	    OSDictionary * table = NULL );


                         
                                                                           
                                                                                                                                                                                                                                                                                             
                                                                                                        
                                                                                                    

	static OSPtr<OSDictionary>  addLocation( OSDictionary * table );

                                        

                             
                                                                                                      
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
                                                                     
                                                                                         
                                                                                                                                                                                                                                                                                                                                                                      

	virtual bool compareProperty(   OSDictionary   * matching,
	    const char     * key );
                             
                                                                                                      
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
                                                                     
                                                                                                                    
                                                                                                                                                                                                                                                                                                                                                                      

	virtual bool compareProperty(   OSDictionary   * matching,
	    const OSString * key );

                               
                                                                                                               
                                                                                                                                                                                                                                                                                                                                                                                                                
                                                                     
                                                                                                                                                             
                                                                                                                                    

	virtual bool compareProperties( OSDictionary   * matching,
	    OSCollection   * keys );

                                 

                    
                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                           
                                                                                     
                                                                                                                     

	virtual bool attach( IOService * provider );

                    
                                                                                
                                                                                                                                                                                                                                                                                     
                                                            

	virtual void detach( IOService * provider );

                         
                                                              
                                                                                                                                                                                                                                                                                                     
                                                                                                                                                                                                                         

	virtual IOService * getProvider( void ) const;

                         
                                                                                   
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
                                                                                                                                                                                                                                                                                                                                                 

	virtual IOWorkLoop * getWorkLoop() const;

                                 
                                                                        
                                                                                                                                                         
                                                                                                                                                                                                                                                                                                               

	virtual OSPtr<OSIterator> getProviderIterator( void ) const;

                                     
                                                                                                      
                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                                  

	virtual OSPtr<OSIterator> getOpenProviderIterator( void ) const;

                       
                                                            
                                                                                                                                                                                                        
                                                                                                                                                                                                               

	virtual IOService * getClient( void ) const;

                               
                                                                      
                                                                                                                                  
                                                                                                                                                                                                                                                                                                               

	virtual OSPtr<OSIterator> getClientIterator( void ) const;

                                   
                                                                                                     
                                                                                                                                                                                                                                                                 
                                                                                                                                                                                                                                                                                                                                                                                          

	virtual OSPtr<OSIterator> getOpenClientIterator( void ) const;

                                  
                                                               
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
                                                                                                                                                                                                                                                                                                                              
                                                                                                                                       
                                                                                                                                                                                                                                                        

	virtual IOReturn callPlatformFunction( const OSSymbol * functionName,
	    bool waitForFunction,
	    void *param1, void *param2,
	    void *param3, void *param4 );

	virtual IOReturn callPlatformFunction( const char * functionName,
	    bool waitForFunction,
	    void *param1, void *param2,
	    void *param3, void *param4 );


                    

                         
                                                                                  
                                                                                                   
                                                                                                    

	static IOPlatformExpert * getPlatform( void );

                             
                                                                                               
                                                                                                                
                                                                                                                

	static class IOPMrootDomain * getPMRootDomain( void );

                            
                                                                  
                                                                                                    
                                                                                                                              

	static IOService * getServiceRoot( void );

                                
                                                            
                                                                                                                                                                                                                        
                                                                                               

	static IOService * getResourceService( void );

                                              

                          
                                                                                                     
                                                                                                                                                                                                                                                                    
                                                                                                                                                                                           

	virtual IOReturn getResources( void );

                             

                                  
                                                                                    
                                                                                                                                                                              
                                                                   

	virtual IOItemCount getDeviceMemoryCount( void );

                                      
                                                                                                         
                                                                                                                                                            
                                                                           
                                                                                                                                                                                                                                                                                                                                                                                      

	virtual IODeviceMemory * getDeviceMemoryWithIndex( unsigned int index );

                                      
                                                 
                                                                                                                                                                                                                                                                                   
                                                                           
                                                                                                                                                                            

	virtual IOMemoryMap * mapDeviceMemoryWithIndex( unsigned int index,
	    IOOptionBits options = 0 );

                             
                                                                                                        
                                                                                                                                                    
                                                                                                                                                       

	virtual OSArray * getDeviceMemory( void );

                             
                                                                                                     
                                                                                                                                                 
                                                                                                                                   

	virtual void setDeviceMemory( OSArray * array );

                         

                               
                                                                                          
                                                                                                                                                                                                                                                                                                                                            
                                                                   
                                                                            
                                                                                                                                                                                                    
                                                              
                                                                                                                                                                                                               

	virtual IOReturn registerInterrupt(int source, OSObject *target,
	    IOInterruptAction handler,
	    void *refCon = NULL);

#ifdef __BLOCKS__
                               
                                                                           
                                                                                                                                                                                                                                                                                                                                            
                                                                   
                                                                            
                                                                                                                                                                                                
                                                                                                                                                                                                               

	IOReturn registerInterruptBlock(int source, OSObject *target,
	    IOInterruptActionBlock handler);
#endif                 

                                 
                                                                                                 
                                                                                                                                              
                                                                   
                                                                                                            

	virtual IOReturn unregisterInterrupt(int source);

                                    
                                                                                 
                                                                                                                                                                                                                           
                                                                                                       
                                                                      
	IOReturn addInterruptStatistics(IOInterruptAccountingData * statistics, int source);

                                       
                                                                                 
                                                                                                                                                                                                                                                             
                                                                      
	IOReturn removeInterruptStatistics(int source);

                              
                                                                                             
                                                                   
                                                                                                                                                                                                                                                                                                    
                                                                                                            

	virtual IOReturn getInterruptType(int source, int *interruptType);

                             
                                          
                                                                                                            
                                                                   
                                                                                                            

	virtual IOReturn enableInterrupt(int source);

                              
                                                         
                                                                                                                                                                                                  
                                                                   
                                                                                                            

	virtual IOReturn disableInterrupt(int source);

                            
                                                  
                                                                             
                                                                   
                                                                                                            

	virtual IOReturn causeInterrupt(int source);

                          
                                                                
                                                                                                                                                                                                                   
                                                                         
                                 

	virtual IOReturn requestProbe( IOOptionBits options );

                                                  

                     
                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                                                              
                                                                    
                                                                                       
                                                             

	virtual IOReturn message( UInt32 type, IOService * provider,
	    void * argument = NULL );

                           
                                                             
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
                                                                                                     
                                                                 
                                                                                              
                                                                                                                                                                                                                                                             
                                                             

	virtual IOReturn messageClient( UInt32 messageType, OSObject * client,
	    void * messageArgument = NULL, vm_size_t argSize = 0 );

                            
                                                               
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
                                                                                              
                                                                                       
                                                                                                                                                                                                                                        
                                                                                                                                                                         

	virtual IOReturn messageClients( UInt32 type,
	    void * argument = NULL, vm_size_t argSize = 0 );

	virtual OSPtr<IONotifier> registerInterest( const OSSymbol * typeOfInterest,
	    IOServiceInterestHandler handler,
	    void * target, void * ref = NULL );

#ifdef __BLOCKS__
	OSPtr<IONotifier>  registerInterest(const OSSymbol * typeOfInterest,
	    IOServiceInterestHandlerBlock handler);
#endif                 

	virtual void applyToProviders( IOServiceApplierFunction applier,
	    void * context );

	virtual void applyToClients( IOServiceApplierFunction applier,
	    void * context );

#ifdef __BLOCKS__
	void applyToProviders(IOServiceApplierBlock applier);
	void applyToClients(IOServiceApplierBlock applier);
#endif                 

	virtual void applyToInterested( const OSSymbol * typeOfInterest,
	    OSObjectApplierFunction applier,
	    void * context );

	virtual IOReturn acknowledgeNotification( IONotificationRef notification,
	    IOOptionBits response );

                        

                           
                                                            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                
                                                                          
                                                                                                                                                                                                      
                                                                                                                                                                                      
                                                                                
                                                                                            

	virtual IOReturn newUserClient( task_t owningTask, void * securityID,
	    UInt32 type, OSDictionary * properties,
	    LIBKERN_RETURNS_RETAINED IOUserClient ** handler );

	virtual IOReturn newUserClient( task_t owningTask, void * securityID,
	    UInt32 type,
	    LIBKERN_RETURNS_RETAINED IOUserClient ** handler );

	IOReturn newUserClient( task_t owningTask, void * securityID,
	    UInt32 type, OSDictionary * properties,
	    OSSharedPtr<IOUserClient>& handler );

	IOReturn newUserClient( task_t owningTask, void * securityID,
	    UInt32 type,
	    OSSharedPtr<IOUserClient>& handler );

                           

                              
                                                                           
                                                                                                                                                                                                    
                                  
                                                                                      

	virtual const char * stringFromReturn( IOReturn rtn );

                             
                                                                       
                                                                                                                                                                                                                                                                                                                                                            
                                  
                                                                          

	virtual int errnoFromReturn( IOReturn rtn );

                                                           
                                                           
                                                           

                                            

	int               _numInterruptSources;
	IOInterruptSource *_interruptSources;

               
	virtual bool serializeProperties( OSSerialize * s ) const APPLE_KEXT_OVERRIDE;

	IOReturn   requireMaxBusStall(UInt32 ns);
	IOReturn   requireMaxInterruptDelay(uint32_t ns);

                                                           
                                                           
                                                           

#ifdef XNU_KERNEL_PRIVATE
public:
                                   
	static void initialize( void );
	static void setPlatform( IOPlatformExpert * platform);
	static void setPMRootDomain( class IOPMrootDomain * rootDomain );
	static IOReturn catalogNewDrivers( OSOrderedSet * newTables );
	uint64_t getAccumulatedBusyTime( void );
	static void updateConsoleUsers(OSArray * consoleUsers, IOMessage systemMessage,
	    bool afterUserspaceReboot = false);
	static void consoleLockTimer(thread_call_param_t p0, thread_call_param_t p1);
	void setTerminateDefer(IOService * provider, bool defer);
	uint64_t getAuthorizationID( void );
	IOReturn setAuthorizationID( uint64_t authorizationID );
	void cpusRunning(void);
	void scheduleFinalize(bool now);
	static void willShutdown();
	static void startDeferredMatches();
	static void iokitDaemonLaunched();
	void resetRematchProperties();
	bool hasUserServer() const;
	static void userSpaceWillReboot();
	static void userSpaceDidReboot();

private:
	static IOReturn waitMatchIdle( UInt32 ms );
	static OSPtr<IONotifier>  installNotification(
		const OSSymbol * type, OSDictionary * matching,
		IOServiceMatchingNotificationHandler handler,
		void * target, void * ref,
		SInt32 priority,
		LIBKERN_RETURNS_RETAINED OSIterator ** existing);
#if !defined(__LP64__)
	static OSPtr<IONotifier>  installNotification(
		const OSSymbol * type, OSDictionary * matching,
		IOServiceNotificationHandler handler,
		void * target, void * ref,
		SInt32 priority,
		LIBKERN_RETURNS_RETAINED OSIterator ** existing);
#endif                         
#endif

private:
	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	bool checkResources( void );
	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	bool checkResource( OSObject * matching );

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	void probeCandidates( LIBKERN_CONSUMED OSOrderedSet * matches );
	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	bool startCandidate( IOService * candidate );

public:
	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOService * getClientWithCategory( const OSSymbol * category )
	APPLE_KEXT_DEPRECATED;
                                                   

#ifdef XNU_KERNEL_PRIVATE
                                                                                
public:
#else
private:
#endif
	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	bool passiveMatch( OSDictionary * matching, bool changesOK = false);
	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	void startMatching( IOOptionBits options = 0 );
	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	void doServiceMatch( IOOptionBits options );
	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	void doServiceTerminate( IOOptionBits options );

private:

	bool matchPassive(OSDictionary * table, uint32_t options);
	bool matchInternal(OSDictionary * table, uint32_t options, unsigned int * did);
	static bool instanceMatch(const OSObject * entry, void * context);

	static OSPtr<OSObject>  copyExistingServices( OSDictionary * matching,
	    IOOptionBits inState, IOOptionBits options = 0 );

	static OSPtr<IONotifier>  setNotification(
		const OSSymbol * type, OSDictionary * matching,
		IOServiceMatchingNotificationHandler handler,
		void * target, void * ref,
		SInt32 priority = 0 );

	static OSPtr<IONotifier>  doInstallNotification(
		const OSSymbol * type, OSDictionary * matching,
		IOServiceMatchingNotificationHandler handler,
		void * target, void * ref,
		SInt32 priority, OSIterator ** existing );

	static bool syncNotificationHandler( void * target, void * ref,
	    IOService * newService, IONotifier * notifier  );

	static void userServerCheckInTokenNotificationHandler(
		IOUserServerCheckInToken * token,
		void * ref);

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	void deliverNotification( const OSSymbol * type,
	    IOOptionBits orNewState, IOOptionBits andNewState );

	OSPtr<OSArray>  copyNotifiers(const OSSymbol * type,
	    IOOptionBits orNewState, IOOptionBits andNewState);

	bool invokeNotifiers(OSArray * willSend[]);
	bool invokeNotifier( class _IOServiceNotifier * notify );

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	void unregisterAllInterest( void );

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOReturn waitForState( UInt32 mask, UInt32 value,
	    mach_timespec_t * timeout = NULL );

	IOReturn waitForState( UInt32 mask, UInt32 value, uint64_t timeout );

	UInt32 _adjustBusy( SInt32 delta );

	bool terminatePhase1( IOOptionBits options = 0 );
	void scheduleTerminatePhase2( IOOptionBits options = 0 );
	void scheduleStop( IOService * provider );

	static void waitToBecomeTerminateThread( void );
	static void __attribute__((__noreturn__)) terminateThread( void * arg, wait_result_t unused );
	static void terminateWorker( IOOptionBits options );
	static void actionWillTerminate( IOService * victim, IOOptionBits options,
	    OSArray * doPhase2List, bool, void * );
	static void actionDidTerminate( IOService * victim, IOOptionBits options,
	    void *, void *, void *);

	static void actionWillStop( IOService * victim, IOOptionBits options,
	    void *, void *, void *);
	static void actionDidStop( IOService * victim, IOOptionBits options,
	    void *, void *, void *);

	static void actionFinalize( IOService * victim, IOOptionBits options,
	    void *, void *, void *);
	static void actionStop( IOService * client, IOService * provider,
	    void *, void *, void *);

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOReturn resolveInterrupt(IOService *nub, int source);
	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOReturn lookupInterrupt(
		int source, bool resolve,
		LIBKERN_RETURNS_NOT_RETAINED IOInterruptController *
		*interruptController);

#ifdef XNU_KERNEL_PRIVATE
                       
#endif

                      
public:

                    
                                                         
                                                                                                                                                                                                                                                                                                                                                  
                                                                                                                                                             

	virtual void PMinit( void );

                    
                                              
                                                                                                                                                                                                                                                                                                                                                                                                             
                                                                                                                                                                                                                         

	virtual void PMstop( void );

                        
                                                                         
                                                                                                                                                                                                                                                                                                                                                                  
                                                                                                                                                                                                                                                                                                            
                                                                                           

	virtual void joinPMtree( IOService * driver );

                                 
                                                                        
                                                                                                                                                                                                                                                                                                                   
                                                                                                                                                                                                                                                    
                                                                                         
                                                                                                                                                                     
                                                                   
                                                                                       

	virtual IOReturn registerPowerDriver(
		IOService *      controllingDriver,
		IOPMPowerState * powerStates,
		unsigned long    numberOfStates );

                                      
                                                                                                                               
                                                                                                                                                                                                                              
                                                                                                                              
                                                                                                                                                                                                        
                                                                                                                                                                                                                                         
                                                                                                                                                                    
                                                                                                                                                                                                                                                                                           

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOPMPowerFlags registerInterestedDriver( IOService * theDriver );

                                        
                                                                                                               
                                                                                                                                                                                                                                                                                                                                    
                                                                                  
                                                                                                                                 
                                                                

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOReturn deRegisterInterestedDriver( IOService * theDriver );

                                    
                                                              
                                                                                                                                                                                                                                                                                                                                                                                   
                                                                                                                                                                                                                                                                                                                                                                                                       
                                                                                                                                                                     
                                       

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOReturn acknowledgePowerChange( IOService * whichDriver );

                                      
                                                                                                              
                                                                                                                                                                                                                                                                                                                                                                             
                                                                                                                                                                          
                                      

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOReturn acknowledgeSetPowerState( void );

                                     
                                                        
                                                                                                                                  

	virtual IOReturn requestPowerDomainState(
		IOPMPowerFlags desiredState,
		IOPowerConnection * whichChild,
		unsigned long specificationFlags );

                        
                                                    
                                                                                                                                                                                                                                                                  
                                                                                                                                                                                                                               
                                                                

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOReturn makeUsable( void );

                                   
                                                                                                          
                                                                                                                                                                                                                                                                                
                                                                

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOReturn temporaryPowerClampOn( void );

                                
                                           
                                                                                                                                                                                                                
                                                                                                                                                                                                                                                                                                                                                                                                                  
                                                                                                                                                                                                                                                                                     
                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                                                   
                                                                

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOReturn changePowerStateTo( unsigned long ordinal );

                               
                                                                          
                                                                                                                      

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IOPMPowerFlags currentCapability( void );

                                     
                                                                   
                                                                                                                                                                                                                 
                                                                                                                  

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	unsigned long currentPowerConsumption( void );

                            
                                                                                                                                                                         
                                                                                                                                                                        
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                       
                                                                                                                                                                                                                                                                                                                                                                                     
                                                                                                                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                            

	virtual bool activityTickle(
		unsigned long type,
		unsigned long stateNumber = 0 );

                               
                                                                                             
                                                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                                                                                                                                        
                                                                                                                                                                                                                                                                           
                                                                         
                                                                                                                                                                                                                                                               
                                                           
                                       

	virtual IOReturn setAggressiveness(
		unsigned long type,
		unsigned long newLevel );

                               
                                                                           
                                                    
                                                                                                               
                                                                                            

	virtual IOReturn getAggressiveness(
		unsigned long type,
		unsigned long * currentLevel );

#ifndef __LP64__
                        
                                                                                  
                                                                                                                                  

	virtual IOReturn systemWake( void )
	APPLE_KEXT_DEPRECATED;

                                        
                                                                              
                                                                                                           

	virtual IOReturn temperatureCriticalForZone( IOService * whichZone )
	APPLE_KEXT_DEPRECATED;

                        
                                                                                       
                                                                                                                                  

	virtual IOReturn youAreRoot( void )
	APPLE_KEXT_DEPRECATED;

                            
                                                                                                                                

	virtual IOReturn setPowerParent(
		IOPowerConnection * parent,
		bool stateKnown,
		IOPMPowerFlags currentState )
	APPLE_KEXT_DEPRECATED;
#endif                

                           
                                                        
                                                                                                                                                                                                                      
                                                                

	virtual IOReturn addPowerChild( IOService * theChild );

                              
                                                                                                  
                                                                                                                                  

	virtual IOReturn removePowerChild( IOPowerConnection * theChild );

#ifndef __LP64__
                              
                                                                                                                                  

	virtual void command_received( void *, void *, void *, void * );
#endif

                                 
                                                                                                                                  

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	void start_PM_idle_timer( void );

#ifndef __LP64__
                                      
                                                                                                                                  

	virtual void PM_idle_timer_expiration( void )
	APPLE_KEXT_DEPRECATED;

                                    
                                                                                                                                  

	virtual void PM_Clamp_Timer_Expired( void )
	APPLE_KEXT_DEPRECATED;
#endif

                                
                                                     
                                                                                                                                                                                                                                                                      
                                                            
                                                                                            

	virtual IOReturn setIdleTimerPeriod( unsigned long period );

#ifndef __LP64__
                           
                                                                               
                                                       
                                                                                                                                                                       

	virtual IOWorkLoop * getPMworkloop( void )
	APPLE_KEXT_DEPRECATED;
#endif

                           
                                                 
                                                                                                                                                                                                                                                                                                                                                                                                                                                     
                                                                                    

	UInt32 getPowerState( void );

                           
                                                                                       
                                                                                                                                                                                                                                                                                  
                                                                                                                                                                                                                                                                       
                                                                                                                           
                                                                                                                                                                                                                      
                                                                                                                                                                                                                                                                                                                                                                                                                                                      

	virtual IOReturn setPowerState(
		unsigned long powerStateOrdinal,
		IOService *   whatDevice );

#ifndef __LP64__
                          
                                         

	virtual void clampPowerOn( unsigned long duration );
#endif

                                         
                                                                                                 
                                                                                                                                                                           
                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
                                                                                                                                                                                           
                               

	virtual unsigned long maxCapabilityForDomainState( IOPMPowerFlags domainState );

                                             
                                                                                                 
                                                                                                           
                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                           
                               

	virtual unsigned long initialPowerStateForDomainState( IOPMPowerFlags domainState );

                                      
                                                                                                 
                                                                                                                                                                                                                                                           
                                                                                                                                                                                           
                               

	virtual unsigned long powerStateForDomainState( IOPMPowerFlags domainState );

                                    
                                                                                           
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
                                                                                                                                                                                                                                                                      
                                                                                                                                                                                                   
                                                                                                   
                                                                                                                                                                                                
                                                                                                                                                                                                                                                                                                                                                                                                             

	virtual IOReturn powerStateWillChangeTo(
		IOPMPowerFlags  capabilities,
		unsigned long   stateNumber,
		IOService *     whatDevice );

                                   
                                                                                               
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
                                                                                                                                                                                                                                                                     
                                                                                                                                                                                                   
                                                                                                   
                                                                                                                                                                                                
                                                                                                                                                                                                                                                                                                                                                                                                             

	virtual IOReturn powerStateDidChangeTo(
		IOPMPowerFlags  capabilities,
		unsigned long   stateNumber,
		IOService *     whatDevice );

#ifndef __LP64__
                              
                                                                                           
                                                       
                                                                                                                                                                                                                                                                                                                                                         
                                                                                                          

	virtual bool didYouWakeSystem( void )
	APPLE_KEXT_DEPRECATED;

                            
                                                                                                 
                                                                                                           

	virtual IOReturn newTemperature( long currentTemp, IOService * whichZone )
	APPLE_KEXT_DEPRECATED;
#endif

	virtual bool askChangeDown( unsigned long );
	virtual bool tellChangeDown( unsigned long );
	virtual void tellNoChangeDown( unsigned long );
	virtual void tellChangeUp( unsigned long );
	virtual IOReturn allowPowerChange( unsigned long refcon );
	virtual IOReturn cancelPowerChange( unsigned long refcon );

protected:
                                    
                                                                                   
                                                                                                                                                                             
                                                                                                                                                                                                                                                                                    
                                                                                                                                                                                                                                                                                                                                                                            
                                                                                   
                                                                
public:
	IOReturn changePowerStateToPriv( unsigned long ordinal );

                                 
                                                                                                                                                    
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                                

	IOReturn powerOverrideOnPriv( void );

                                  
                                                           
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
                                                                

	IOReturn powerOverrideOffPriv( void );

                             
                                                                    
                                                                                                                                                                                                                                                                                                                                                                                                                                                  
                                                                                                        

	virtual void powerChangeDone( unsigned long stateNumber );
#ifdef XNU_KERNEL_PRIVATE
                                
public:
	void idleTimerExpired( void );
	void settleTimerExpired( void );
	IOReturn synchronizePowerTree( IOOptionBits options = 0, IOService * notifyRoot = NULL );
	bool assertPMDriverCall( IOPMDriverCallEntry * callEntry, IOOptionBits method, const IOPMinformee * inform = NULL, IOOptionBits options = 0 );
	void deassertPMDriverCall( IOPMDriverCallEntry * callEntry );
	IOReturn changePowerStateWithOverrideTo( IOPMPowerStateIndex ordinal, IOPMRequestTag tag );
	IOReturn changePowerStateWithTagToPriv( IOPMPowerStateIndex ordinal, IOPMRequestTag tag );
	IOReturn changePowerStateWithTagTo( IOPMPowerStateIndex ordinal, IOPMRequestTag tag );
	IOReturn changePowerStateForRootDomain( IOPMPowerStateIndex ordinal );
	IOReturn setIgnoreIdleTimer( bool ignore );
	IOReturn quiescePowerTree( void * target, IOPMCompletionAction action, void * param );
	IOPMPowerStateIndex getPowerStateForClient( const OSSymbol * client );
	static const char * getIOMessageString( uint32_t msg );
	static void setAdvisoryTickleEnable( bool enable );
	void reset_watchdog_timer(IOService *obj, int timeout);
	void start_watchdog_timer( void );
	void stop_watchdog_timer( void );
	void start_watchdog_timer(uint64_t deadline);
	IOReturn registerInterestForNotifier( IONotifier *notify, const OSSymbol * typeOfInterest,
	    IOServiceInterestHandler handler, void * target, void * ref );

	static IOWorkLoop * getIOPMWorkloop( void );
	bool getBlockingDriverCall(thread_t *thread, const void **callMethod);
	void cancelIdlePowerDown(IOService * service);

protected:
	bool tellClientsWithResponse( int messageType );
	void tellClients( int messageType );
	void PMDebug( uint32_t event, uintptr_t param1, uintptr_t param2 );

private:
#ifndef __LP64__
	void ack_timer_ticked( void );
	IOReturn serializedAllowPowerChange2( unsigned long );
	IOReturn serializedCancelPowerChange2( unsigned long );
	IOReturn powerDomainWillChangeTo( IOPMPowerFlags, IOPowerConnection * );
	IOReturn powerDomainDidChangeTo( IOPMPowerFlags, IOPowerConnection * );
#endif
	void PMfree( void );
	bool tellChangeDown1( unsigned long );
	bool tellChangeDown2( unsigned long );
	IOReturn startPowerChange( IOPMPowerChangeFlags, IOPMPowerStateIndex, IOPMPowerFlags, IOPowerConnection *, IOPMPowerFlags );
	void setParentInfo( IOPMPowerFlags, IOPowerConnection *, bool );
	IOReturn notifyAll( uint32_t nextMS );
	bool notifyChild( IOPowerConnection * child );
	IOPMPowerStateIndex getPowerStateForDomainFlags( IOPMPowerFlags flags );

                                   
	void OurChangeStart( void );
	void OurSyncStart( void );
	void OurChangeTellClientsPowerDown( void );
	void OurChangeTellUserPMPolicyPowerDown( void );
	void OurChangeTellPriorityClientsPowerDown( void );
	void OurChangeTellCapabilityWillChange( void );
	void OurChangeNotifyInterestedDriversWillChange( void );
	void OurChangeSetPowerState( void );
	void OurChangeWaitForPowerSettle( void );
	void OurChangeNotifyInterestedDriversDidChange( void );
	void OurChangeTellCapabilityDidChange( void );
	void OurChangeFinish( void );

                                                    
	IOReturn ParentChangeStart( void );
	void ParentChangeTellPriorityClientsPowerDown( void );
	void ParentChangeTellCapabilityWillChange( void );
	void ParentChangeNotifyInterestedDriversWillChange( void );
	void ParentChangeSetPowerState( void );
	void ParentChangeWaitForPowerSettle( void );
	void ParentChangeNotifyInterestedDriversDidChange( void );
	void ParentChangeTellCapabilityDidChange( void );
	void ParentChangeAcknowledgePowerChange( void );
	void ParentChangeRootChangeDown( void );

	void all_done( void );
	void start_ack_timer( void );
	void stop_ack_timer( void );
	void start_ack_timer( UInt32 value, UInt32 scale );
	void startSettleTimer( void );
	void start_spindump_timer( const char * delay_type );
	void stop_spindump_timer( void );
	bool checkForDone( void );
	bool responseValid( uint32_t x, int pid );
	void computeDesiredState( unsigned long tempDesire, bool computeOnly );
	void trackSystemSleepPreventers( IOPMPowerStateIndex, IOPMPowerStateIndex, IOPMPowerChangeFlags );
	void tellSystemCapabilityChange( uint32_t nextMS );
	void restartIdleTimer( void );

	static void ack_timer_expired( thread_call_param_t, thread_call_param_t );
	static void watchdog_timer_expired( thread_call_param_t arg0, thread_call_param_t arg1 );
	static void spindump_timer_expired( thread_call_param_t arg0, thread_call_param_t arg1 );
	static IOReturn actionAckTimerExpired(OSObject *, void *, void *, void *, void * );
	static IOReturn actionSpinDumpTimerExpired(OSObject *, void *, void *, void *, void * );

	static IOReturn actionDriverCalloutDone(OSObject *, void *, void *, void *, void * );
	static IOPMRequest * acquirePMRequest( IOService * target, IOOptionBits type, IOPMRequest * active = NULL );
	static void releasePMRequest( IOPMRequest * request );
	static void pmDriverCallout( IOService * from );
	static void pmTellAppWithResponse( OSObject * object, void * context );
	static void pmTellClientWithResponse( OSObject * object, void * context );
	static void pmTellCapabilityAppWithResponse( OSObject * object, void * arg );
	static void pmTellCapabilityClientWithResponse( OSObject * object, void * arg );
	static void submitPMRequest(LIBKERN_CONSUMED IOPMRequest * request );
	static void submitPMRequests( IOPMRequest * requests[], IOItemCount count );
	bool ackTimerTick( void );
	void addPowerChild1( IOPMRequest * request );
	void addPowerChild2( IOPMRequest * request );
	void addPowerChild3( IOPMRequest * request );
	void adjustPowerState( IOPMPowerStateIndex clamp = 0 );
	void handlePMstop( IOPMRequest * request );
	void handleRegisterPowerDriver( IOPMRequest * request );
	bool handleAcknowledgePowerChange( IOPMRequest * request );
	void handlePowerDomainWillChangeTo( IOPMRequest * request );
	void handlePowerDomainDidChangeTo( IOPMRequest * request );
	void handleRequestPowerState( IOPMRequest * request );
	void handlePowerOverrideChanged( IOPMRequest * request );
	void handleActivityTickle( IOPMRequest * request );
	void handleInterestChanged( IOPMRequest * request );
	void handleSynchronizePowerTree( IOPMRequest * request );
	void executePMRequest( IOPMRequest * request );
	bool actionPMWorkQueueInvoke( IOPMRequest * request, IOPMWorkQueue * queue );
	bool actionPMWorkQueueRetire( IOPMRequest * request, IOPMWorkQueue * queue );
	bool actionPMRequestQueue( IOPMRequest * request, IOPMRequestQueue * queue );
	bool actionPMReplyQueue( IOPMRequest * request, IOPMRequestQueue * queue );
	bool actionPMCompletionQueue( LIBKERN_CONSUMED IOPMRequest * request, IOPMCompletionQueue * queue );
	bool notifyInterestedDrivers( void );
	void notifyInterestedDriversDone( void );
	bool notifyControllingDriver( void );
	void notifyControllingDriverDone( void );
	void driverSetPowerState( void );
	void driverInformPowerChange( void );
	bool isPMBlocked( IOPMRequest * request, int count );
	void notifyChildren( void );
	void notifyChildrenOrdered( void );
	void notifyChildrenDelayed( void );
	void notifyRootDomain( void );
	void notifyRootDomainDone( void );
	void cleanClientResponses( bool logErrors );
	void updatePowerClient( const OSSymbol * client, IOPMPowerStateIndex powerState );
	void removePowerClient( const OSSymbol * client );
	IOReturn requestPowerState( const OSSymbol * client, IOPMPowerStateIndex state, IOPMRequestTag tag = 0 );
	IOReturn requestDomainPower( IOPMPowerStateIndex ourPowerState, IOOptionBits options = 0 );
	IOReturn configurePowerStatesReport( IOReportConfigureAction action, void *result );
	IOReturn updatePowerStatesReport( IOReportConfigureAction action, void *result, void *destination );
	IOReturn configureSimplePowerReport(IOReportConfigureAction action, void *result );
	IOReturn updateSimplePowerReport( IOReportConfigureAction action, void *result, void *destination );
	void waitForPMDriverCall( IOService * target = NULL );
#endif                         
};

#ifdef PRIVATE

class IOServiceCompatibility : public IOService
{
	OSDeclareDefaultStructors(IOServiceCompatibility);
};

#endif              

#endif                           
