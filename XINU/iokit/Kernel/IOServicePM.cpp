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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <IOKit/assert.h>
#include <IOKit/IOKitDebug.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IOService.h>
#include <IOKit/IOUserServer.h>
#include <IOKit/IOEventSource.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommand.h>
#include <IOKit/IOTimeStamp.h>
#include <IOKit/IOReportMacros.h>
#include <IOKit/IODeviceTreeSupport.h>

#include <IOKit/pwr_mgt/IOPMlog.h>
#include <IOKit/pwr_mgt/IOPMinformee.h>
#include <IOKit/pwr_mgt/IOPMinformeeList.h>
#include <IOKit/pwr_mgt/IOPowerConnection.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <IOKit/pwr_mgt/IOPMPrivate.h>

#include <sys/proc.h>
#include <sys/proc_internal.h>
#include <sys/sysctl.h>
#include <libkern/OSDebug.h>
#include <kern/thread.h>

                                            
#include "IOServicePrivate.h"
#include "IOServicePMPrivate.h"
#include "IOKitKernelInternal.h"

#if USE_SETTLE_TIMER
static void settle_timer_expired(thread_call_param_t, thread_call_param_t);
#endif
static void idle_timer_expired(thread_call_param_t, thread_call_param_t);
static void tellKernelClientApplier(OSObject * object, void * arg);
static void tellAppClientApplier(OSObject * object, void * arg);
static const char * getNotificationPhaseString(uint32_t phase);

static uint64_t
computeTimeDeltaNS( const AbsoluteTime * start )
{
	AbsoluteTime    now;
	uint64_t        nsec;

	clock_get_uptime(&now);
	SUB_ABSOLUTETIME(&now, start);
	absolutetime_to_nanoseconds(now, &nsec);
	return nsec;
}

#if PM_VARS_SUPPORT
OSDefineMetaClassAndStructors(IOPMprot, OSObject)
#endif

                                                                                
          
                                                                                

static bool                  gIOPMInitialized       = false;
static uint32_t              gIOPMBusyRequestCount  = 0;
static uint32_t              gIOPMWorkInvokeCount   = 0;
static uint32_t              gIOPMTickleGeneration  = 0;
static IOWorkLoop *          gIOPMWorkLoop          = NULL;
static IOPMRequestQueue *    gIOPMRequestQueue      = NULL;
static IOPMRequestQueue *    gIOPMReplyQueue        = NULL;
static IOPMWorkQueue *       gIOPMWorkQueue         = NULL;
static IOPMCompletionQueue * gIOPMCompletionQueue   = NULL;
static IOPMRequest *         gIOPMRequest           = NULL;
static IOService *           gIOPMRootNode          = NULL;
static IOPlatformExpert *    gPlatform              = NULL;

                                                            
static uint64_t              gIOPMSetPowerStateLogNS =
#if defined(__i386__) || defined(__x86_64__)
    (300ULL * 1000ULL * 1000ULL)
#else
    (50ULL * 1000ULL * 1000ULL)
#endif
;

const OSSymbol *             gIOPMPowerClientDevice     = NULL;
const OSSymbol *             gIOPMPowerClientDriver     = NULL;
const OSSymbol *             gIOPMPowerClientChildProxy = NULL;
const OSSymbol *             gIOPMPowerClientChildren   = NULL;
const OSSymbol *             gIOPMPowerClientRootDomain = NULL;

static const OSSymbol *      gIOPMPowerClientAdvisoryTickle = NULL;
static bool                  gIOPMAdvisoryTickleEnabled = true;
static thread_t              gIOPMWatchDogThread        = NULL;
uint32_t                     gCanSleepTimeout           = 0;

static uint32_t
getPMRequestType( void )
{
	uint32_t type = kIOPMRequestTypeInvalid;
	if (gIOPMRequest) {
		type = gIOPMRequest->getType();
	}
	return type;
}

SYSCTL_UINT(_kern, OID_AUTO, pmtimeout, CTLFLAG_RW | CTLFLAG_LOCKED, &gCanSleepTimeout, 0, "Power Management Timeout");

                                                                                
         
                                                                                

#define PM_ERROR(x...)              do { kprintf(x);IOLog(x); \
	                            } while (false)
#define PM_LOG(x...)                do { kprintf(x); } while (false)

#define PM_LOG1(x...)               do {  \
	                            if (kIOLogDebugPower & gIOKitDebug) \
	                                kprintf(x); } while (false)

#define PM_LOG2(x...)               do {  \
	                            if (kIOLogDebugPower & gIOKitDebug) \
	                                kprintf(x); } while (false)

#if 0
#define PM_LOG3(x...)               do { kprintf(x); } while (false)
#else
#define PM_LOG3(x...)
#endif

#define RD_LOG(x...)                do { \
	                            if ((kIOLogPMRootDomain & gIOKitDebug) && \
	                                (getPMRootDomain() == this)) { \
	                                kprintf("PMRD: " x); \
	                            }} while (false)
#define PM_ASSERT_IN_GATE(x)          \
do {                                  \
    assert(gIOPMWorkLoop->inGate());  \
} while(false)

#define PM_LOCK()                   IOLockLock(fPMLock)
#define PM_UNLOCK()                 IOLockUnlock(fPMLock)
#define PM_LOCK_SLEEP(event, dl)    IOLockSleepDeadline(fPMLock, event, dl, THREAD_UNINT)
#define PM_LOCK_WAKEUP(event)       IOLockWakeup(fPMLock, event, false)

#define us_per_s                    1000000
#define ns_per_us                   1000
#define k30Seconds                  (30*us_per_s)
#define k5Seconds                   ( 5*us_per_s)
#if !defined(XNU_TARGET_OS_OSX)
#define kCanSleepMaxTimeReq         k5Seconds
#else                                 
#define kCanSleepMaxTimeReq         k30Seconds
#endif                                 
#define kMaxTimeRequested           k30Seconds
#define kMinAckTimeoutTicks         (10*1000000)
#define kIOPMTardyAckSPSKey         "IOPMTardyAckSetPowerState"
#define kIOPMTardyAckPSCKey         "IOPMTardyAckPowerStateChange"
#define kPwrMgtKey                  "IOPowerManagement"

#define OUR_PMLog(t, a, b) do {                 \
    if (pwrMgt) {                               \
	if (gIOKitDebug & kIOLogPower)          \
	    pwrMgt->pmPrint(t, a, b);           \
	if (gIOKitTrace & kIOTracePowerMgmt)    \
	    pwrMgt->pmTrace(t, DBG_FUNC_NONE, a, b);        \
    }                                           \
    } while(0)

#define OUR_PMLogFuncStart(t, a, b) do {        \
    if (pwrMgt) {                               \
	if (gIOKitDebug & kIOLogPower)          \
	    pwrMgt->pmPrint(t, a, b);           \
	if (gIOKitTrace & kIOTracePowerMgmt)    \
	    pwrMgt->pmTrace(t, DBG_FUNC_START, a, b);       \
    }                                           \
    } while(0)

#define OUR_PMLogFuncEnd(t, a, b) do {          \
    if (pwrMgt) {                               \
	if (gIOKitDebug & kIOLogPower)          \
	    pwrMgt->pmPrint(-t, a, b);          \
	if (gIOKitTrace & kIOTracePowerMgmt)    \
	    pwrMgt->pmTrace(t, DBG_FUNC_END, a, b);        \
    }                                           \
    } while(0)

#define NS_TO_MS(nsec)              ((int)((nsec) / 1000000ULL))
#define NS_TO_US(nsec)              ((int)((nsec) / 1000ULL))

#define SUPPORT_IDLE_CANCEL         1

#define kIOPMPowerStateMax          0xFFFFFFFF
#define kInvalidTicklePowerState    kIOPMPowerStateMax

#define kNoTickleCancelWindow       (60ULL * 1000ULL * 1000ULL * 1000ULL)

#define IS_PM_ROOT                  (this == gIOPMRootNode)
#define IS_ROOT_DOMAIN              (getPMRootDomain() == this)
#define IS_POWER_DROP               (StateOrder(fHeadNotePowerState) < StateOrder(fCurrentPowerState))
#define IS_POWER_RISE               (StateOrder(fHeadNotePowerState) > StateOrder(fCurrentPowerState))

                                      
#define LOG_APP_RESPONSE_TIMES      (100ULL * 1000ULL * 1000ULL)
                                                       
#define LOG_APP_RESPONSE_MSG_TRACER (3 * 1000ULL * 1000ULL * 1000ULL)

                                       
#define LOG_KEXT_RESPONSE_TIMES     (100ULL * 1000ULL * 1000ULL)

enum {
	kReserveDomainPower = 1
};

#define MS_PUSH(n)  \
    do { assert(kIOPM_BadMachineState == fSavedMachineState); \
	 assert(kIOPM_BadMachineState != n); \
	 fSavedMachineState = n; } while (false)

#define MS_POP()    \
    do { assert(kIOPM_BadMachineState != fSavedMachineState); \
	 fMachineState = fSavedMachineState; \
	 fSavedMachineState = kIOPM_BadMachineState; } while (false)

#define PM_ACTION_TICKLE(a) \
    do { if (fPMActions.a) { \
	 (fPMActions.a)(fPMActions.target, this, &fPMActions); } \
	 } while (false)

#define PM_ACTION_CHANGE(a, x, y) \
    do { if (fPMActions.a) { \
	 (fPMActions.a)(fPMActions.target, this, &fPMActions, gIOPMRequest, x, y); } \
	 } while (false)

#define PM_ACTION_CLIENT(a, x, y, z) \
    do { if (fPMActions.a) { \
	 (fPMActions.a)(fPMActions.target, this, &fPMActions, x, y, z); } \
	 } while (false)

static OSNumber * copyClientIDForNotification(
	OSObject *object,
	IOPMInterestContext *context);

static void logClientIDForNotification(
	OSObject *object,
	IOPMInterestContext *context,
	const char *logString);

                                                                                   
                    
  
                                                 
                                                                                   

enum {
	kIOPM_Finished                                      = 0,

	kIOPM_OurChangeTellClientsPowerDown                 = 1,
	kIOPM_OurChangeTellUserPMPolicyPowerDown            = 2,
	kIOPM_OurChangeTellPriorityClientsPowerDown         = 3,
	kIOPM_OurChangeNotifyInterestedDriversWillChange    = 4,
	kIOPM_OurChangeSetPowerState                        = 5,
	kIOPM_OurChangeWaitForPowerSettle                   = 6,
	kIOPM_OurChangeNotifyInterestedDriversDidChange     = 7,
	kIOPM_OurChangeTellCapabilityDidChange              = 8,
	kIOPM_OurChangeFinish                               = 9,

	kIOPM_ParentChangeTellPriorityClientsPowerDown      = 10,
	kIOPM_ParentChangeNotifyInterestedDriversWillChange = 11,
	kIOPM_ParentChangeSetPowerState                     = 12,
	kIOPM_ParentChangeWaitForPowerSettle                = 13,
	kIOPM_ParentChangeNotifyInterestedDriversDidChange  = 14,
	kIOPM_ParentChangeTellCapabilityDidChange           = 15,
	kIOPM_ParentChangeAcknowledgePowerChange            = 16,

	kIOPM_NotifyChildrenStart                           = 17,
	kIOPM_NotifyChildrenOrdered                         = 18,
	kIOPM_NotifyChildrenDelayed                         = 19,
	kIOPM_SyncTellClientsPowerDown                      = 20,
	kIOPM_SyncTellPriorityClientsPowerDown              = 21,
	kIOPM_SyncNotifyWillChange                          = 22,
	kIOPM_SyncNotifyDidChange                           = 23,
	kIOPM_SyncTellCapabilityDidChange                   = 24,
	kIOPM_SyncFinish                                    = 25,
	kIOPM_TellCapabilityChangeDone                      = 26,
	kIOPM_DriverThreadCallDone                          = 27,

	kIOPM_BadMachineState                               = 0xFFFFFFFF
};

                                                                                   
                  
  
                               
                                                                                   

void
IOService::PMinit( void )
{
	if (!initialized) {
		if (!gIOPMInitialized) {
			gPlatform = getPlatform();
			gIOPMWorkLoop = IOWorkLoop::workLoop();
			if (gIOPMWorkLoop) {
				gIOPMRequestQueue = IOPMRequestQueue::create(
					this, OSMemberFunctionCast(IOPMRequestQueue::Action,
					this, &IOService::actionPMRequestQueue));

				gIOPMReplyQueue = IOPMRequestQueue::create(
					this, OSMemberFunctionCast(IOPMRequestQueue::Action,
					this, &IOService::actionPMReplyQueue));

				gIOPMWorkQueue = IOPMWorkQueue::create(this,
				    OSMemberFunctionCast(IOPMWorkQueue::Action, this,
				    &IOService::actionPMWorkQueueInvoke),
				    OSMemberFunctionCast(IOPMWorkQueue::Action, this,
				    &IOService::actionPMWorkQueueRetire));

				gIOPMCompletionQueue = IOPMCompletionQueue::create(
					this, OSMemberFunctionCast(IOPMCompletionQueue::Action,
					this, &IOService::actionPMCompletionQueue));

				if (gIOPMWorkLoop->addEventSource(gIOPMRequestQueue) !=
				    kIOReturnSuccess) {
					gIOPMRequestQueue->release();
					gIOPMRequestQueue = NULL;
				}

				if (gIOPMWorkLoop->addEventSource(gIOPMReplyQueue) !=
				    kIOReturnSuccess) {
					gIOPMReplyQueue->release();
					gIOPMReplyQueue = NULL;
				}

				if (gIOPMWorkLoop->addEventSource(gIOPMWorkQueue) !=
				    kIOReturnSuccess) {
					gIOPMWorkQueue->release();
					gIOPMWorkQueue = NULL;
				}

				                                                           
				                                                           
				if (gIOPMWorkLoop->addEventSource(gIOPMCompletionQueue) !=
				    kIOReturnSuccess) {
					gIOPMCompletionQueue->release();
					gIOPMCompletionQueue = NULL;
				}

				gIOPMPowerClientDevice =
				    OSSymbol::withCStringNoCopy( "DevicePowerState" );

				gIOPMPowerClientDriver =
				    OSSymbol::withCStringNoCopy( "DriverPowerState" );

				gIOPMPowerClientChildProxy =
				    OSSymbol::withCStringNoCopy( "ChildProxyPowerState" );

				gIOPMPowerClientChildren =
				    OSSymbol::withCStringNoCopy( "ChildrenPowerState" );

				gIOPMPowerClientAdvisoryTickle =
				    OSSymbol::withCStringNoCopy( "AdvisoryTicklePowerState" );

				gIOPMPowerClientRootDomain =
				    OSSymbol::withCStringNoCopy( "RootDomainPower" );
			}

			if (gIOPMRequestQueue && gIOPMReplyQueue && gIOPMCompletionQueue) {
				gIOPMInitialized = true;
			}

#if (DEVELOPMENT || DEBUG)
			uint32_t setPowerStateLogMS = 0;
			if (PE_parse_boot_argn("setpowerstate_log", &setPowerStateLogMS, sizeof(setPowerStateLogMS))) {
				gIOPMSetPowerStateLogNS = setPowerStateLogMS * 1000000ULL;
			}
#endif
		}
		if (!gIOPMInitialized) {
			return;
		}

		pwrMgt = new IOServicePM;
		pwrMgt->init();
		setProperty(kPwrMgtKey, pwrMgt);

		queue_init(&pwrMgt->WorkChain);
		queue_init(&pwrMgt->RequestHead);
		queue_init(&pwrMgt->PMDriverCallQueue);

		fOwner                      = this;
		fPMLock                     = IOLockAlloc();
		fInterestedDrivers          = new IOPMinformeeList;
		fInterestedDrivers->initialize();
		fDesiredPowerState          = kPowerStateZero;
		fDeviceDesire               = kPowerStateZero;
		fInitialPowerChange         = true;
		fInitialSetPowerState       = true;
		fPreviousRequestPowerFlags  = 0;
		fDeviceOverrideEnabled      = false;
		fMachineState               = kIOPM_Finished;
		fSavedMachineState          = kIOPM_BadMachineState;
		fIdleTimerMinPowerState     = kPowerStateZero;
		fActivityLock               = IOLockAlloc();
		fStrictTreeOrder            = false;
		fActivityTicklePowerState   = kInvalidTicklePowerState;
		fAdvisoryTicklePowerState   = kInvalidTicklePowerState;
		fControllingDriver          = NULL;
		fPowerStates                = NULL;
		fNumberOfPowerStates        = 0;
		fCurrentPowerState          = kPowerStateZero;
		fParentsCurrentPowerFlags   = 0;
		fMaxPowerState              = kPowerStateZero;
		fName                       = getName();
		fParentsKnowState           = false;
		fSerialNumber               = 0;
		fResponseArray              = NULL;
		fNotifyClientArray          = NULL;
		fCurrentPowerConsumption    = kIOPMUnknown;
		fOverrideMaxPowerState      = kIOPMPowerStateMax;

		if (!gIOPMRootNode && (getParentEntry(gIOPowerPlane) == getRegistryRoot())) {
			gIOPMRootNode = this;
			fParentsKnowState = true;
		} else if (getProperty(kIOPMResetPowerStateOnWakeKey) == kOSBooleanTrue) {
			fResetPowerStateOnWake = true;
		}

		if (IS_ROOT_DOMAIN) {
			fWatchdogTimer = thread_call_allocate(
				&IOService::watchdog_timer_expired, (thread_call_param_t)this);
			fWatchdogLock = IOLockAlloc();

			fBlockedArray =  OSArray::withCapacity(4);
		}

		fAckTimer = thread_call_allocate(
			&IOService::ack_timer_expired, (thread_call_param_t)this);
#if USE_SETTLE_TIMER
		fSettleTimer = thread_call_allocate(
			&settle_timer_expired, (thread_call_param_t)this);
#endif
		fIdleTimer = thread_call_allocate(
			&idle_timer_expired, (thread_call_param_t)this);
		fDriverCallEntry = thread_call_allocate(
			(thread_call_func_t) &IOService::pmDriverCallout, this);
		assert(fDriverCallEntry);

		                                      
		if (OSMemberFunctionCast(void (*)(void),
		    getResourceService(), &IOService::powerChangeDone) !=
		    OSMemberFunctionCast(void (*)(void),
		    this, &IOService::powerChangeDone)) {
			fPCDFunctionOverride = true;
		}

#if PM_VARS_SUPPORT
		IOPMprot * prot = new IOPMprot;
		if (prot) {
			prot->init();
			prot->ourName = fName;
			prot->thePlatform = gPlatform;
			fPMVars = prot;
			pm_vars = prot;
		}
#else
		pm_vars = (void *) (uintptr_t) true;
#endif

		initialized = true;
	}
}

                                                                                   
                   
  
                                                                       
                                                                                   

void
IOService::PMfree( void )
{
	initialized = false;
	pm_vars = NULL;

	if (pwrMgt) {
		assert(fMachineState == kIOPM_Finished);
		assert(fInsertInterestSet == NULL);
		assert(fRemoveInterestSet == NULL);
		assert(fNotifyChildArray == NULL);
		assert(queue_empty(&pwrMgt->RequestHead));
		assert(queue_empty(&fPMDriverCallQueue));

		if (fWatchdogTimer) {
			thread_call_cancel(fWatchdogTimer);
			thread_call_free(fWatchdogTimer);
			fWatchdogTimer = NULL;
		}

		if (fWatchdogLock) {
			IOLockFree(fWatchdogLock);
			fWatchdogLock = NULL;
		}

		if (fBlockedArray) {
			fBlockedArray->release();
			fBlockedArray = NULL;
		}
#if USE_SETTLE_TIMER
		if (fSettleTimer) {
			thread_call_cancel(fSettleTimer);
			thread_call_free(fSettleTimer);
			fSettleTimer = NULL;
		}
#endif
		if (fAckTimer) {
			thread_call_cancel(fAckTimer);
			thread_call_free(fAckTimer);
			fAckTimer = NULL;
		}
		if (fIdleTimer) {
			thread_call_cancel(fIdleTimer);
			thread_call_free(fIdleTimer);
			fIdleTimer = NULL;
		}
		if (fDriverCallEntry) {
			thread_call_free(fDriverCallEntry);
			fDriverCallEntry = NULL;
		}
		if (fPMLock) {
			IOLockFree(fPMLock);
			fPMLock = NULL;
		}
		if (fActivityLock) {
			IOLockFree(fActivityLock);
			fActivityLock = NULL;
		}
		if (fInterestedDrivers) {
			fInterestedDrivers->release();
			fInterestedDrivers = NULL;
		}
		if (fDriverCallParamSlots && fDriverCallParamPtr) {
			IODelete(fDriverCallParamPtr, DriverCallParam, fDriverCallParamSlots);
			fDriverCallParamPtr = NULL;
			fDriverCallParamSlots = 0;
		}
		if (fResponseArray) {
			fResponseArray->release();
			fResponseArray = NULL;
		}
		if (fNotifyClientArray) {
			fNotifyClientArray->release();
			fNotifyClientArray = NULL;
		}
		if (fReportBuf && fNumberOfPowerStates) {
			IOFree(fReportBuf, STATEREPORT_BUFSIZE(fNumberOfPowerStates));
			fReportBuf = NULL;
		}
		if (fPowerStates && fNumberOfPowerStates) {
			IODelete(fPowerStates, IOPMPSEntry, fNumberOfPowerStates);
			fNumberOfPowerStates = 0;
			fPowerStates = NULL;
		}
		if (fPowerClients) {
			fPowerClients->release();
			fPowerClients = NULL;
		}

#if PM_VARS_SUPPORT
		if (fPMVars) {
			fPMVars->release();
			fPMVars = NULL;
		}
#endif

		pwrMgt->release();
		pwrMgt = NULL;
	}
}

void
IOService::PMDebug( uint32_t event, uintptr_t param1, uintptr_t param2 )
{
	OUR_PMLog(event, param1, param2);
}

                                                                                   
                      
  
                                                                           
                                                                       
                                                                        
                                                                       
                     
  
                                                                
                                                          
                                                                                   

void
IOService::joinPMtree( IOService * driver )
{
	IOPlatformExpert *  platform;

	platform = getPlatform();
	assert(platform != NULL);
	platform->PMRegisterDevice(this, driver);
}

#ifndef __LP64__
                                                                                   
                          
  
                                                                     
                                                                                   

IOReturn
IOService::youAreRoot( void )
{
	return IOPMNoErr;
}
#endif                

                                                                                   
                  
  
                                                                             
                    
                                                                                   

void
IOService::PMstop( void )
{
	IOPMRequest * request;

	if (!initialized) {
		return;
	}

	PM_LOCK();

	if (fLockedFlags.PMStop) {
		PM_LOG2("%s: PMstop() already stopped\n", fName);
		PM_UNLOCK();
		return;
	}

	                               
	fLockedFlags.PMStop = true;

	                                             
	waitForPMDriverCall();

	PM_UNLOCK();

	                                           
	request = acquirePMRequest( this, kIOPMRequestTypePMStop );
	if (request) {
		PM_LOG2("%s: %p PMstop\n", getName(), OBFUSCATE(this));
		submitPMRequest( request );
	}
}

                                                                                   
                         
  
                                                                        
                                                                                   

void
IOService::handlePMstop( IOPMRequest * request )
{
	OSIterator *        iter;
	OSObject *          next;
	IOPowerConnection * connection;
	IOService *         theChild;
	IOService *         theParent;

	PM_ASSERT_IN_GATE();
	PM_LOG2("%s: %p %s start\n", getName(), OBFUSCATE(this), __FUNCTION__);

	                                                
	getPMRootDomain()->updatePreventIdleSleepList(this, false);
	getPMRootDomain()->updatePreventSystemSleepList(this, false);

	                      
	removeProperty(kPwrMgtKey);

	                 
	iter = getParentIterator(gIOPowerPlane);
	if (iter) {
		while ((next = iter->getNextObject())) {
			if ((connection = OSDynamicCast(IOPowerConnection, next))) {
				theParent = (IOService *)connection->copyParentEntry(gIOPowerPlane);
				if (theParent) {
					theParent->removePowerChild(connection);
					theParent->release();
				}
			}
		}
		iter->release();
	}

	                       
	detachAbove( gIOPowerPlane );

	                              
	fParentsKnowState = false;

	                  
	iter = getChildIterator(gIOPowerPlane);
	if (iter) {
		while ((next = iter->getNextObject())) {
			if ((connection = OSDynamicCast(IOPowerConnection, next))) {
				theChild = ((IOService *)(connection->copyChildEntry(gIOPowerPlane)));
				if (theChild) {
					                        
					connection->detachFromChild(theChild, gIOPowerPlane);
					theChild->release();
				}
				                     
				detachFromChild(connection, gIOPowerPlane);
			}
		}
		iter->release();
	}

	                                                                   
	                      
	  
	                                                                     
	                                                                      
	                                                                   
	            

	if (fInterestedDrivers) {
		IOPMinformeeList *  list = fInterestedDrivers;
		IOPMinformee *      item;

		PM_LOCK();
		while ((item = list->firstInList())) {
			list->removeFromList(item->whatObject);
		}
		PM_UNLOCK();
	}

	                                                                 
	                          

	fIdleTimerPeriod = 0;
	if (fIdleTimer && thread_call_cancel(fIdleTimer)) {
		release();
	}

	PM_LOG2("%s: %p %s done\n", getName(), OBFUSCATE(this), __FUNCTION__);
}

                                                                                   
                         
  
                                                         
                                                                                   

IOReturn
IOService::addPowerChild( IOService * child )
{
	IOPowerConnection * connection  = NULL;
	IOPMRequest *       requests[3] = {NULL, NULL, NULL};
	OSIterator *        iter;
	bool                ok = true;

	if (!child) {
		return kIOReturnBadArgument;
	}

	if (!initialized || !child->initialized) {
		return IOPMNotYetInitialized;
	}

	OUR_PMLog( kPMLogAddChild, (uintptr_t) child, 0 );

	do {
		                                             

		iter = child->getParentIterator( gIOPowerPlane );
		if (iter) {
			IORegistryEntry *   entry;
			OSObject *          next;

			while ((next = iter->getNextObject())) {
				if ((entry = OSDynamicCast(IORegistryEntry, next)) &&
				    isChild(entry, gIOPowerPlane)) {
					ok = false;
					break;
				}
			}
			iter->release();
		}
		if (!ok) {
			PM_LOG2("%s: %s (%p) is already a child\n",
			    getName(), child->getName(), OBFUSCATE(child));
			break;
		}

		                                                        
		                                             
		                                                        
		                                                        
		                             

		connection = new IOPowerConnection;
		if (!connection) {
			break;
		}

		                                                           
		                              

		requests[0] = acquirePMRequest(
			             this,
			           kIOPMRequestTypeAddPowerChild1 );

		requests[1] = acquirePMRequest(
			             child,
			           kIOPMRequestTypeAddPowerChild2 );

		requests[2] = acquirePMRequest(
			             this,
			           kIOPMRequestTypeAddPowerChild3 );

		if (!requests[0] || !requests[1] || !requests[2]) {
			break;
		}

		requests[0]->attachNextRequest( requests[1] );
		requests[1]->attachNextRequest( requests[2] );

		connection->init();
		connection->start(this);
		connection->setAwaitingAck(false);
		connection->setReadyFlag(false);

		attachToChild( connection, gIOPowerPlane );
		connection->attachToChild( child, gIOPowerPlane );

		                                  
		requests[0]->fArg0 = connection;
		requests[1]->fArg0 = connection;
		requests[2]->fArg0 = connection;

		submitPMRequests( requests, 3 );
		return kIOReturnSuccess;
	}while (false);

	if (connection) {
		connection->release();
	}
	if (requests[0]) {
		releasePMRequest(requests[0]);
	}
	if (requests[1]) {
		releasePMRequest(requests[1]);
	}
	if (requests[2]) {
		releasePMRequest(requests[2]);
	}

	                                                                    
	                      

	return kIOReturnSuccess;
}

                                                                                   
                           
  
                                                                
                                                                                   

void
IOService::addPowerChild1( IOPMRequest * request )
{
	IOPMPowerStateIndex tempDesire = kPowerStateZero;

	                                                    

	PM_ASSERT_IN_GATE();
	OUR_PMLog( kPMLogMakeUsable, kPMLogMakeUsable, 0 );

	if (fControllingDriver && inPlane(gIOPowerPlane) && fParentsKnowState) {
		tempDesire = fHighestPowerState;
	}

	if ((tempDesire != kPowerStateZero) &&
	    (IS_PM_ROOT || (StateOrder(fMaxPowerState) >= StateOrder(tempDesire)))) {
		adjustPowerState(tempDesire);
	}
}

                                                                                   
                           
  
                                                                 
                                           
                                                                                   

void
IOService::addPowerChild2( IOPMRequest * request )
{
	IOPowerConnection * connection = (IOPowerConnection *) request->fArg0;
	IOService *         parent;
	IOPMPowerFlags      powerFlags;
	bool                knowsState;
	IOPMPowerStateIndex powerState;
	IOPMPowerStateIndex tempDesire;

	PM_ASSERT_IN_GATE();
	parent = (IOService *) connection->getParentEntry(gIOPowerPlane);

	if (!parent || !inPlane(gIOPowerPlane)) {
		PM_LOG("%s: addPowerChild2 not in power plane\n", getName());
		return;
	}

	                                                        
	                                               

	knowsState = (parent->fPowerStates) && (parent->fParentsKnowState);
	powerState = parent->fCurrentPowerState;

	if (knowsState) {
		powerFlags = parent->fPowerStates[powerState].outputPowerFlags;
	} else {
		powerFlags = 0;
	}

	                        

	OUR_PMLog(kPMLogSetParent, knowsState, powerFlags);

	setParentInfo( powerFlags, connection, knowsState );

	connection->setReadyFlag(true);

	if (fControllingDriver && fParentsKnowState) {
		fMaxPowerState = fControllingDriver->maxCapabilityForDomainState(fParentsCurrentPowerFlags);
		                                                    
		tempDesire = fControllingDriver->initialPowerStateForDomainState(fParentsCurrentPowerFlags);
		fPreviousRequestPowerFlags = (IOPMPowerFlags)(-1);
		adjustPowerState(tempDesire);
	}
}

                                                                                   
                           
  
                                                          
                                           
                                                                                   

void
IOService::addPowerChild3( IOPMRequest * request )
{
	IOPowerConnection * connection = (IOPowerConnection *) request->fArg0;
	IOService *         child;
	IOPMrootDomain *    rootDomain = getPMRootDomain();

	PM_ASSERT_IN_GATE();
	child = (IOService *) connection->getChildEntry(gIOPowerPlane);

	if (child && inPlane(gIOPowerPlane)) {
		if ((this != rootDomain) && child->getProperty("IOPMStrictTreeOrder")) {
			PM_LOG1("%s: strict PM order enforced\n", getName());
			fStrictTreeOrder = true;
		}

		if (rootDomain) {
			rootDomain->joinAggressiveness( child );
		}
	} else {
		PM_LOG("%s: addPowerChild3 not in power plane\n", getName());
	}

	connection->release();
}

#ifndef __LP64__
                                                                                   
                              
  
                                                      
                                                                      
                                                                  
                        
                                                                                   

IOReturn
IOService::setPowerParent(
	IOPowerConnection * theParent, bool stateKnown, IOPMPowerFlags powerFlags )
{
	return kIOReturnUnsupported;
}
#endif                

                                                                                   
                            
  
                                                               
                                                                                   

IOReturn
IOService::removePowerChild( IOPowerConnection * theNub )
{
	IORegistryEntry *   theChild;

	PM_ASSERT_IN_GATE();
	OUR_PMLog( kPMLogRemoveChild, 0, 0 );

	theNub->retain();

	                        
	theChild = theNub->copyChildEntry(gIOPowerPlane);
	if (theChild) {
		theNub->detachFromChild(theChild, gIOPowerPlane);
		theChild->release();
	}
	                      
	detachFromChild(theNub, gIOPowerPlane);

	                                          
	if (theNub->getAwaitingAck()) {
		                          
		theNub->setAwaitingAck(false);
		if (fHeadNotePendingAcks != 0) {
			                                      
			fHeadNotePendingAcks--;

			                    
			if (fHeadNotePendingAcks == 0) {
				stop_ack_timer();
				getPMRootDomain()->reset_watchdog_timer(this, 0);

				                                                           
				                                                            
				                                                               
				                                                               
				                                              

				gIOPMWorkQueue->incrementProducerCount();
			}
		}
	}

	theNub->release();

	                                                                  
	                                                                        

	if (!fAdjustPowerScheduled) {
		IOPMRequest * request;
		request = acquirePMRequest( this, kIOPMRequestTypeAdjustPowerState );
		if (request) {
			submitPMRequest( request );
			fAdjustPowerScheduled = true;
		}
	}

	return IOPMNoErr;
}

                                                                                   
                               
  
                                                                      
                                                                                   

IOReturn
IOService::registerPowerDriver(
	IOService *         powerDriver,
	IOPMPowerState *    powerStates,
	unsigned long       numberOfStates )
{
	IOPMRequest *       request;
	IOPMPSEntry *       powerStatesCopy = NULL;
	IOPMPowerStateIndex stateOrder;
	IOReturn            error = kIOReturnSuccess;

	if (!initialized) {
		return IOPMNotYetInitialized;
	}

	if (!powerStates || (numberOfStates < 2)) {
		OUR_PMLog(kPMLogControllingDriverErr5, numberOfStates, 0);
		return kIOReturnBadArgument;
	}

	if (!powerDriver || !powerDriver->initialized) {
		OUR_PMLog(kPMLogControllingDriverErr4, 0, 0);
		return kIOReturnBadArgument;
	}

	if (powerStates[0].version > kIOPMPowerStateVersion2) {
		OUR_PMLog(kPMLogControllingDriverErr1, powerStates[0].version, 0);
		return kIOReturnBadArgument;
	}

	do {
		                                                 
		powerStatesCopy = IONew(IOPMPSEntry, numberOfStates);
		if (!powerStatesCopy) {
			error = kIOReturnNoMemory;
			break;
		}

		                             
		for (IOPMPowerStateIndex i = 0; i < numberOfStates; i++) {
			powerStatesCopy[i].stateOrderToIndex = kIOPMPowerStateMax;
		}

		for (uint32_t i = 0; i < numberOfStates; i++) {
			powerStatesCopy[i].capabilityFlags  = powerStates[i].capabilityFlags;
			powerStatesCopy[i].outputPowerFlags = powerStates[i].outputPowerCharacter;
			powerStatesCopy[i].inputPowerFlags  = powerStates[i].inputPowerRequirement;
			powerStatesCopy[i].staticPower      = powerStates[i].staticPower;
#if USE_SETTLE_TIMER
			powerStatesCopy[i].settleUpTime     = powerStates[i].settleUpTime;
			powerStatesCopy[i].settleDownTime   = powerStates[i].settleDownTime;
#endif
			if (powerStates[i].version >= kIOPMPowerStateVersion2) {
				stateOrder = powerStates[i].stateOrder;
			} else {
				stateOrder = i;
			}

			if (stateOrder < numberOfStates) {
				powerStatesCopy[i].stateOrder = stateOrder;
				powerStatesCopy[stateOrder].stateOrderToIndex = i;
			}
		}

		for (IOPMPowerStateIndex i = 0; i < numberOfStates; i++) {
			if (powerStatesCopy[i].stateOrderToIndex == kIOPMPowerStateMax) {
				                            
				error = kIOReturnBadArgument;
				break;
			}
		}
		if (kIOReturnSuccess != error) {
			break;
		}

		request = acquirePMRequest( this, kIOPMRequestTypeRegisterPowerDriver );
		if (!request) {
			error = kIOReturnNoMemory;
			break;
		}

		powerDriver->retain();
		request->fArg0 = (void *) powerDriver;
		request->fArg1 = (void *) powerStatesCopy;
		request->fArg2 = (void *) numberOfStates;

		submitPMRequest( request );
		return kIOReturnSuccess;
	}while (false);

	if (powerStatesCopy) {
		IODelete(powerStatesCopy, IOPMPSEntry, numberOfStates);
	}

	return error;
}

                                                                                   
                                      
                                                                                   

void
IOService::handleRegisterPowerDriver( IOPMRequest * request )
{
	IOService *           powerDriver    = (IOService *)   request->fArg0;
	IOPMPSEntry *         powerStates    = (IOPMPSEntry *) request->fArg1;
	IOPMPowerStateIndex   numberOfStates = (IOPMPowerStateIndex) request->fArg2;
	IOPMPowerStateIndex   i, stateIndex;
	IOPMPowerStateIndex   lowestPowerState;
	IOService *           root;
	OSIterator *          iter;

	PM_ASSERT_IN_GATE();
	assert(powerStates);
	assert(powerDriver);
	assert(numberOfStates > 1);

	if (!fNumberOfPowerStates) {
		OUR_PMLog(kPMLogControllingDriver, numberOfStates, kIOPMPowerStateVersion1);

		fPowerStates            = powerStates;
		fNumberOfPowerStates    = numberOfStates;
		fControllingDriver      = powerDriver;
		fCurrentCapabilityFlags = fPowerStates[0].capabilityFlags;

		lowestPowerState   = fPowerStates[0].stateOrderToIndex;
		fHighestPowerState = fPowerStates[numberOfStates - 1].stateOrderToIndex;

		{
			uint32_t        aotFlags;
			IOService *     service;
			OSObject *      object;
			OSData *        data;

			                                                           

			aotFlags = 0;
			service  = this;
			while (service && !service->inPlane(gIODTPlane)) {
				service = service->getProvider();
			}
			if (service) {
				object = service->copyProperty(kIOPMAOTPowerKey, gIODTPlane);
				data = OSDynamicCast(OSData, object);
				if (data && (data->getLength() >= sizeof(uint32_t))) {
					aotFlags = ((uint32_t *)data->getBytesNoCopy())[0];
				}
				OSSafeReleaseNULL(object);
			}
			if (!aotFlags) {
				for (i = 0; i < numberOfStates; i++) {
					if (kIOPMAOTPower & fPowerStates[i].inputPowerFlags) {
						fPowerStates[i].inputPowerFlags  = 0xFFFFFFFF;
						fPowerStates[i].capabilityFlags  = 0;
						fPowerStates[i].outputPowerFlags = 0;
					}
				}
			}
		}

		                                   
		fMergedOutputPowerFlags = 0;
		fDeviceUsablePowerState = lowestPowerState;
		for (i = 0; i < numberOfStates; i++) {
			fMergedOutputPowerFlags |= fPowerStates[i].outputPowerFlags;

			stateIndex = fPowerStates[i].stateOrderToIndex;
			assert(stateIndex < numberOfStates);
			if ((fDeviceUsablePowerState == lowestPowerState) &&
			    (fPowerStates[stateIndex].capabilityFlags & IOPMDeviceUsable)) {
				                                                    
				fDeviceUsablePowerState = stateIndex;
			}
		}

		                                                           
		                                                             
		                                                              
		                                      

		root = getPlatform()->getProvider();
		assert(root);
		if (!root ||
		    ((OSMemberFunctionCast(void (*)(void),
		    root, &IOService::powerStateDidChangeTo)) !=
		    ((OSMemberFunctionCast(void (*)(void),
		    this, &IOService::powerStateDidChangeTo)))) ||
		    ((OSMemberFunctionCast(void (*)(void),
		    root, &IOService::powerStateWillChangeTo)) !=
		    ((OSMemberFunctionCast(void (*)(void),
		    this, &IOService::powerStateWillChangeTo))))) {
			if (fInterestedDrivers->findItem(powerDriver) == NULL) {
				PM_LOCK();
				fInterestedDrivers->appendNewInformee(powerDriver);
				PM_UNLOCK();
			}
		}

		                                                              

		if (fPowerClients &&
		    (iter = OSCollectionIterator::withCollection(fPowerClients))) {
			const OSSymbol * client;
			while ((client = (const OSSymbol *) iter->getNextObject())) {
				IOPMPowerStateIndex powerState = getPowerStateForClient(client);
				if (powerState >= numberOfStates) {
					updatePowerClient(client, fHighestPowerState);
				}
			}
			iter->release();
		}

		                                                  
		getPMRootDomain()->tagPowerPlaneService(this, &fPMActions, fNumberOfPowerStates - 1);

		if (inPlane(gIOPowerPlane) && fParentsKnowState) {
			IOPMPowerStateIndex tempDesire;
			fMaxPowerState = fControllingDriver->maxCapabilityForDomainState(fParentsCurrentPowerFlags);
			                                                    
			tempDesire = fControllingDriver->initialPowerStateForDomainState(fParentsCurrentPowerFlags);
			adjustPowerState(tempDesire);
		}
	} else {
		OUR_PMLog(kPMLogControllingDriverErr2, numberOfStates, 0);
		IODelete(powerStates, IOPMPSEntry, numberOfStates);
	}

	powerDriver->release();
}

                                                                                   
                                    
  
                                                                          
                                                                         
                                                                           
                                                     
                                                                                   

IOPMPowerFlags
IOService::registerInterestedDriver( IOService * driver )
{
	IOPMRequest *   request;
	bool            signal;

	if (!driver || !initialized || !fInterestedDrivers) {
		return 0;
	}

	PM_LOCK();
	signal = (!fInsertInterestSet && !fRemoveInterestSet);
	if (fInsertInterestSet == NULL) {
		fInsertInterestSet = OSSet::withCapacity(4);
	}
	if (fInsertInterestSet) {
		fInsertInterestSet->setObject(driver);
		if (fRemoveInterestSet) {
			fRemoveInterestSet->removeObject(driver);
		}
	}
	PM_UNLOCK();

	if (signal) {
		request = acquirePMRequest( this, kIOPMRequestTypeInterestChanged );
		if (request) {
			submitPMRequest( request );
		}
	}

	                                                          
	                               

	OUR_PMLog(kPMLogInterestedDriver, kIOPMDeviceUsable, 2);
	return kIOPMDeviceUsable;
}

                                                                                   
                                      
                                                                                   

IOReturn
IOService::deRegisterInterestedDriver( IOService * driver )
{
	IOPMinformee *      item;
	IOPMRequest *       request;
	bool                signal;

	if (!driver) {
		return kIOReturnBadArgument;
	}
	if (!initialized || !fInterestedDrivers) {
		return IOPMNotPowerManaged;
	}

	PM_LOCK();
	if (fInsertInterestSet) {
		fInsertInterestSet->removeObject(driver);
	}

	item = fInterestedDrivers->findItem(driver);
	if (!item) {
		PM_UNLOCK();
		return kIOReturnNotFound;
	}

	signal = (!fRemoveInterestSet && !fInsertInterestSet);
	if (fRemoveInterestSet == NULL) {
		fRemoveInterestSet = OSSet::withCapacity(4);
	}
	if (fRemoveInterestSet) {
		fRemoveInterestSet->setObject(driver);
		if (item->active) {
			item->active = false;
			waitForPMDriverCall( driver );
		}
	}
	PM_UNLOCK();

	if (signal) {
		request = acquirePMRequest( this, kIOPMRequestTypeInterestChanged );
		if (request) {
			submitPMRequest( request );
		}
	}

	return IOPMNoErr;
}

                                                                                   
                                  
  
                                    
                                                                                   

void
IOService::handleInterestChanged( IOPMRequest * request )
{
	IOService *         driver;
	IOPMinformee *      informee;
	IOPMinformeeList *  list = fInterestedDrivers;

	PM_LOCK();

	if (fInsertInterestSet) {
		while ((driver = (IOService *) fInsertInterestSet->getAnyObject())) {
			if (list->findItem(driver) == NULL) {
				list->appendNewInformee(driver);
			}
			fInsertInterestSet->removeObject(driver);
		}
		fInsertInterestSet->release();
		fInsertInterestSet = NULL;
	}

	if (fRemoveInterestSet) {
		while ((driver = (IOService *) fRemoveInterestSet->getAnyObject())) {
			informee = list->findItem(driver);
			if (informee) {
				                                          
				if (fHeadNotePendingAcks && informee->timer) {
					informee->timer = 0;
					fHeadNotePendingAcks--;
				}
				list->removeFromList(driver);
			}
			fRemoveInterestSet->removeObject(driver);
		}
		fRemoveInterestSet->release();
		fRemoveInterestSet = NULL;
	}

	PM_UNLOCK();
}

                                                                                   
                                  
  
                                                                          
                                                                  
                                                          
                                                                        
       
                                                                        
                                                                            
                                                                            
                               
                                                                                   

IOReturn
IOService::acknowledgePowerChange( IOService * whichObject )
{
	IOPMRequest * request;

	if (!initialized) {
		return IOPMNotYetInitialized;
	}
	if (!whichObject) {
		return kIOReturnBadArgument;
	}

	request = acquirePMRequest( this, kIOPMRequestTypeAckPowerChange );
	if (!request) {
		return kIOReturnNoMemory;
	}

	whichObject->retain();
	request->fArg0 = whichObject;

	submitPMRequest( request );
	return IOPMNoErr;
}

                                                                                   
                                         
                                                                                   

bool
IOService::handleAcknowledgePowerChange( IOPMRequest * request )
{
	IOPMinformee *      informee;
	IOPMPowerStateIndex childPower = kIOPMUnknown;
	IOService *         theChild;
	IOService *         whichObject;
	bool                all_acked  = false;

	PM_ASSERT_IN_GATE();
	whichObject = (IOService *) request->fArg0;
	assert(whichObject);

	                                 
	informee = fInterestedDrivers->findItem( whichObject );
	if (informee == NULL) {
		if (!isChild(whichObject, gIOPowerPlane)) {
			OUR_PMLog(kPMLogAcknowledgeErr1, 0, 0);
			goto no_err;
		} else {
			OUR_PMLog(kPMLogChildAcknowledge, fHeadNotePendingAcks, 0);
		}
	} else {
		OUR_PMLog(kPMLogDriverAcknowledge, fHeadNotePendingAcks, 0);
	}

	if (fHeadNotePendingAcks != 0) {
		assert(fPowerStates != NULL);

		                                      
		if (informee != NULL) {
			                            
			                                     
			if (informee->timer != 0) {
				if (informee->timer > 0) {
					uint64_t nsec = computeTimeDeltaNS(&informee->startTime);
					if (nsec > gIOPMSetPowerStateLogNS) {
						getPMRootDomain()->pmStatsRecordApplicationResponse(
							gIOPMStatsDriverPSChangeSlow, informee->whatObject->getName(),
							fDriverCallReason, NS_TO_MS(nsec), informee->whatObject->getRegistryEntryID(),
							NULL, fHeadNotePowerState, true);
					}
				}

				                
				informee->timer = 0;
				                                  
				fHeadNotePendingAcks--;
			} else {
				                                
				OUR_PMLog(kPMLogAcknowledgeErr2, 0, 0);
			}
		} else {
			               
			                                     
			if (((IOPowerConnection *)whichObject)->getAwaitingAck()) {
				                                  
				fHeadNotePendingAcks--;
				((IOPowerConnection *)whichObject)->setAwaitingAck(false);
				theChild = (IOService *)whichObject->copyChildEntry(gIOPowerPlane);
				if (theChild) {
					childPower = theChild->currentPowerConsumption();
					theChild->release();
				}
				if (childPower == kIOPMUnknown) {
					fHeadNotePowerArrayEntry->staticPower = kIOPMUnknown;
				} else {
					if (fHeadNotePowerArrayEntry->staticPower != kIOPMUnknown) {
						fHeadNotePowerArrayEntry->staticPower += childPower;
					}
				}
			}
		}

		if (fHeadNotePendingAcks == 0) {
			                      
			stop_ack_timer();
			                          
			all_acked = true;
			getPMRootDomain()->reset_watchdog_timer(this, 0);
		}
	} else {
		OUR_PMLog(kPMLogAcknowledgeErr3, 0, 0);                                
	}

no_err:
	if (whichObject) {
		whichObject->release();
	}

	return all_acked;
}

                                                                                   
                                    
  
                                                                     
                                                 
                                                 
                                                                                   

IOReturn
IOService::acknowledgeSetPowerState( void )
{
	IOPMRequest * request;

	if (!initialized) {
		return IOPMNotYetInitialized;
	}

	request = acquirePMRequest( this, kIOPMRequestTypeAckSetPowerState );
	if (!request) {
		return kIOReturnNoMemory;
	}

	submitPMRequest( request );
	return kIOReturnSuccess;
}

                                                                                   
                             
                                                                                   

void
IOService::adjustPowerState( IOPMPowerStateIndex clamp )
{
	PM_ASSERT_IN_GATE();
	computeDesiredState(clamp, false);
	if (fControllingDriver && fParentsKnowState && inPlane(gIOPowerPlane)) {
		IOPMPowerChangeFlags changeFlags = kIOPMSelfInitiated;

		                                                                 
		                                                                 
		                    

		if (getPMRequestType() == kIOPMRequestTypeRequestPowerStateOverride) {
			changeFlags |= (kIOPMIgnoreChildren | kIOPMSkipAskPowerDown);
		}

		startPowerChange(
			                   changeFlags,
			                   fDesiredPowerState,
			                   0,
			                   NULL,
			                   0);
	}
}

                                                                                   
                                
                                                                                   

IOReturn
IOService::synchronizePowerTree(
	IOOptionBits    options,
	IOService *     notifyRoot )
{
	IOPMRequest *   request_c = NULL;
	IOPMRequest *   request_s;

	if (this != getPMRootDomain()) {
		return kIOReturnBadArgument;
	}
	if (!initialized) {
		return kIOPMNotYetInitialized;
	}

	OUR_PMLog(kPMLogCSynchronizePowerTree, options, (notifyRoot != NULL));

	if (notifyRoot) {
		IOPMRequest * nr;

		                                         
		nr = acquirePMRequest(notifyRoot, kIOPMRequestTypeChildNotifyDelayCancel);
		if (nr) {
			submitPMRequest(nr);
		}

		                                                                       
		                                                      
		nr = acquirePMRequest(getPMRootDomain(), kIOPMRequestTypeChildNotifyDelayCancel);
		if (nr) {
			submitPMRequest(nr);
		}
	}

	request_s = acquirePMRequest( this, kIOPMRequestTypeSynchronizePowerTree );
	if (!request_s) {
		goto error_no_memory;
	}

	if (options & kIOPMSyncCancelPowerDown) {
		request_c = acquirePMRequest( this, kIOPMRequestTypeIdleCancel );
	}
	if (request_c) {
		request_c->attachNextRequest( request_s );
		submitPMRequest(request_c);
	}

	request_s->fArg0 = (void *)(uintptr_t) options;
	submitPMRequest(request_s);

	return kIOReturnSuccess;

error_no_memory:
	if (request_c) {
		releasePMRequest(request_c);
	}
	if (request_s) {
		releasePMRequest(request_s);
	}
	return kIOReturnNoMemory;
}

                                                                                   
                                       
                                                                                   

void
IOService::handleSynchronizePowerTree( IOPMRequest * request )
{
	PM_ASSERT_IN_GATE();
	if (fControllingDriver && fParentsKnowState && inPlane(gIOPowerPlane) &&
	    (fCurrentPowerState == fHighestPowerState)) {
		IOPMPowerChangeFlags options = (IOPMPowerChangeFlags)(uintptr_t) request->fArg0;

		startPowerChange(
			                   kIOPMSelfInitiated | kIOPMSynchronize |
			(options & kIOPMSyncNoChildNotify),
			                   fCurrentPowerState,
			                   0,
			                   NULL,
			                   0);
	}
}

#ifndef __LP64__
                                                                                   
                                       
  
                                                                      
                       
                                                                  
                                                                  
                                            
                                                                                   

IOReturn
IOService::powerDomainWillChangeTo(
	IOPMPowerFlags      newPowerFlags,
	IOPowerConnection * whichParent )
{
	assert(false);
	return kIOReturnUnsupported;
}
#endif                

                                                                                   
                                          
                                                                                   

void
IOService::handlePowerDomainWillChangeTo( IOPMRequest * request )
{
	IOPMPowerFlags       parentPowerFlags = (IOPMPowerFlags) request->fArg0;
	IOPowerConnection *  whichParent = (IOPowerConnection *) request->fArg1;
	IOPMPowerChangeFlags parentChangeFlags = (IOPMPowerChangeFlags)(uintptr_t) request->fArg2;
	IOPMPowerChangeFlags myChangeFlags;
	OSIterator *         iter;
	OSObject *           next;
	IOPowerConnection *  connection;
	IOPMPowerStateIndex  maxPowerState;
	IOPMPowerFlags       combinedPowerFlags;
	IOReturn             result = IOPMAckImplied;

	PM_ASSERT_IN_GATE();
	OUR_PMLog(kPMLogWillChange, parentPowerFlags, 0);

	if (!inPlane(gIOPowerPlane) || !whichParent || !whichParent->getAwaitingAck()) {
		PM_LOG("%s::%s not in power tree\n", getName(), __FUNCTION__);
		goto exit_no_ack;
	}

	                                       

	combinedPowerFlags = 0;

	iter = getParentIterator(gIOPowerPlane);
	if (iter) {
		while ((next = iter->getNextObject())) {
			if ((connection = OSDynamicCast(IOPowerConnection, next))) {
				if (connection == whichParent) {
					combinedPowerFlags |= parentPowerFlags;
				} else {
					combinedPowerFlags |= connection->parentCurrentPowerFlags();
				}
			}
		}
		iter->release();
	}

	                                                                      
	                                                                   

	if (fControllingDriver && !fInitialPowerChange) {
		maxPowerState = fControllingDriver->maxCapabilityForDomainState(
			combinedPowerFlags);

		if (parentChangeFlags & kIOPMDomainPowerDrop) {
			                                                              
			                                        
			fMaxPowerState = maxPowerState;
		}

		                                                                
		                                                                 
		                                                             
		                                                         

		myChangeFlags = kIOPMParentInitiated | kIOPMDomainWillChange |
		    (parentChangeFlags & kIOPMSynchronize);

		result = startPowerChange(
			                   myChangeFlags,
			                   maxPowerState,
			                   combinedPowerFlags,
			                   whichParent,
			                   parentPowerFlags);
	}

	                                                               
	                                                             
	                                                  

	if (parentChangeFlags & kIOPMDomainPowerDrop) {
		setParentInfo(parentPowerFlags, whichParent, true);
	}

	                                                                      
	                                                                      
	                                                

	if (IOPMAckImplied == result) {
		IOService * parent;
		parent = (IOService *) whichParent->copyParentEntry(gIOPowerPlane);
		assert(parent);
		if (parent) {
			parent->acknowledgePowerChange( whichParent );
			parent->release();
		}
	}

exit_no_ack:
	                                      
	if (whichParent) {
		whichParent->release();
	}
}

#ifndef __LP64__
                                                                                   
                                      
  
                                                                                 
                              
                                                                  
                                                                  
                                            
                                                                                   

IOReturn
IOService::powerDomainDidChangeTo(
	IOPMPowerFlags      newPowerFlags,
	IOPowerConnection * whichParent )
{
	assert(false);
	return kIOReturnUnsupported;
}
#endif                

                                                                                   
                                         
                                                                                   

void
IOService::handlePowerDomainDidChangeTo( IOPMRequest * request )
{
	IOPMPowerFlags       parentPowerFlags = (IOPMPowerFlags) request->fArg0;
	IOPowerConnection *  whichParent = (IOPowerConnection *) request->fArg1;
	IOPMPowerChangeFlags parentChangeFlags = (IOPMPowerChangeFlags)(uintptr_t) request->fArg2;
	IOPMPowerChangeFlags myChangeFlags;
	IOPMPowerStateIndex  maxPowerState;
	IOPMPowerStateIndex  initialDesire = kPowerStateZero;
	bool                 computeDesire = false;
	bool                 desireChanged = false;
	bool                 savedParentsKnowState;
	IOReturn             result = IOPMAckImplied;

	PM_ASSERT_IN_GATE();
	OUR_PMLog(kPMLogDidChange, parentPowerFlags, 0);

	if (!inPlane(gIOPowerPlane) || !whichParent || !whichParent->getAwaitingAck()) {
		PM_LOG("%s::%s not in power tree\n", getName(), __FUNCTION__);
		goto exit_no_ack;
	}

	savedParentsKnowState = fParentsKnowState;

	setParentInfo(parentPowerFlags, whichParent, true);

	if (fControllingDriver) {
		maxPowerState = fControllingDriver->maxCapabilityForDomainState(
			fParentsCurrentPowerFlags);

		if ((parentChangeFlags & kIOPMDomainPowerDrop) == 0) {
			                                                              
			                                       
			fMaxPowerState = maxPowerState;
		}

		if (fInitialPowerChange) {
			computeDesire = true;
			initialDesire = fControllingDriver->initialPowerStateForDomainState(
				fParentsCurrentPowerFlags);
		} else if (parentChangeFlags & kIOPMRootChangeUp) {
			if (fAdvisoryTickleUsed) {
				                                                           
				                                                          
				                                                          
				                                                           
				                                                          

				desireChanged = true;
			}

			if (fResetPowerStateOnWake) {
				                                                               
				                                                         

				IOPMPowerStateIndex wakePowerState =
				    fControllingDriver->initialPowerStateForDomainState(
					kIOPMRootDomainState | kIOPMPowerOn );

				                                                        
				                             

				if (StateOrder(wakePowerState) > StateOrder(fDesiredPowerState)) {
					                                                     
					                                                     
					                                                    
					                             

					updatePowerClient(gIOPMPowerClientDevice, wakePowerState);
					desireChanged = true;
				}
			}
		}

		if (computeDesire || desireChanged) {
			computeDesiredState(initialDesire, false);
		}

		                                                
		myChangeFlags = kIOPMParentInitiated | kIOPMDomainDidChange |
		    (parentChangeFlags & kIOPMRootBroadcastFlags);

		if (kIOPMAOTPower & fPowerStates[maxPowerState].inputPowerFlags) {
			IOLog("aotPS %s0x%qx[%ld]\n", getName(), getRegistryEntryID(), maxPowerState);
		}

		result = startPowerChange(
			                   myChangeFlags,
			                   maxPowerState,
			                   fParentsCurrentPowerFlags,
			                   whichParent,
			                   0);
	}

	                                                                      
	                                                                      
	                                                

	if (IOPMAckImplied == result) {
		IOService * parent;
		parent = (IOService *) whichParent->copyParentEntry(gIOPowerPlane);
		assert(parent);
		if (parent) {
			parent->acknowledgePowerChange( whichParent );
			parent->release();
		}
	}

	                                                                  
	                                                                   
	                                                        

	if (fControllingDriver &&
	    ((!savedParentsKnowState && fParentsKnowState) || desireChanged)) {
		PM_LOG1("%s::powerDomainDidChangeTo parentsKnowState %d\n",
		    getName(), fParentsKnowState);
		requestDomainPower( fDesiredPowerState );
	}

exit_no_ack:
	                                      
	if (whichParent) {
		whichParent->release();
	}
}

                                                                                   
                          
  
                                                                                   
                 
                                                                                   

void
IOService::setParentInfo(
	IOPMPowerFlags      newPowerFlags,
	IOPowerConnection * whichParent,
	bool                knowsState )
{
	OSIterator *        iter;
	OSObject *          next;
	IOPowerConnection * conn;

	PM_ASSERT_IN_GATE();

	                          
	whichParent->setParentCurrentPowerFlags(newPowerFlags);
	whichParent->setParentKnowsState(knowsState);

	                            
	fParentsCurrentPowerFlags = 0;
	fParentsKnowState = true;

	iter = getParentIterator(gIOPowerPlane);
	if (iter) {
		while ((next = iter->getNextObject())) {
			if ((conn = OSDynamicCast(IOPowerConnection, next))) {
				fParentsKnowState &= conn->parentKnowsState();
				fParentsCurrentPowerFlags |= conn->parentCurrentPowerFlags();
			}
		}
		iter->release();
	}
}

                                                                                
                                       
                                                                                

void
IOService::trackSystemSleepPreventers(
	IOPMPowerStateIndex     oldPowerState,
	IOPMPowerStateIndex     newPowerState,
	IOPMPowerChangeFlags    changeFlags __unused )
{
	IOPMPowerFlags  oldCapability, newCapability;

	oldCapability = fPowerStates[oldPowerState].capabilityFlags &
	    (kIOPMPreventIdleSleep | kIOPMPreventSystemSleep);
	newCapability = fPowerStates[newPowerState].capabilityFlags &
	    (kIOPMPreventIdleSleep | kIOPMPreventSystemSleep);

	if (fHeadNoteChangeFlags & kIOPMInitialPowerChange) {
		oldCapability = 0;
	}
	if (oldCapability == newCapability) {
		return;
	}

	if ((oldCapability ^ newCapability) & kIOPMPreventIdleSleep) {
		bool enablePrevention  = ((oldCapability & kIOPMPreventIdleSleep) == 0);
		bool idleCancelAllowed = getPMRootDomain()->updatePreventIdleSleepList(
			this, enablePrevention);
#if SUPPORT_IDLE_CANCEL
		if (idleCancelAllowed && enablePrevention) {
			IOPMRequest *   cancelRequest;

			cancelRequest = acquirePMRequest( getPMRootDomain(), kIOPMRequestTypeIdleCancel );
			if (cancelRequest) {
				submitPMRequest( cancelRequest );
			}
		}
#endif
	}

	if ((oldCapability ^ newCapability) & kIOPMPreventSystemSleep) {
		getPMRootDomain()->updatePreventSystemSleepList(this,
		    ((oldCapability & kIOPMPreventSystemSleep) == 0));
	}
}

                                                                                   
                                   
  
                                                                     
                                                                                   

IOReturn
IOService::requestPowerDomainState(
	IOPMPowerFlags      childRequestPowerFlags,
	IOPowerConnection * childConnection,
	unsigned long       specification )
{
	IOPMPowerStateIndex order, powerState;
	IOPMPowerFlags      outputPowerFlags;
	IOService *         child;
	IOPMRequest *       subRequest;
	bool                adjustPower = false;

	if (!initialized) {
		return IOPMNotYetInitialized;
	}

	if (gIOPMWorkLoop->onThread() == false) {
		PM_LOG("%s::requestPowerDomainState\n", getName());
		return kIOReturnSuccess;
	}

	OUR_PMLog(kPMLogRequestDomain, childRequestPowerFlags, specification);

	if (!isChild(childConnection, gIOPowerPlane)) {
		return kIOReturnNotAttached;
	}

	if (!fControllingDriver || !fNumberOfPowerStates) {
		return kIOReturnNotReady;
	}

	child = (IOService *) childConnection->getChildEntry(gIOPowerPlane);
	assert(child);

	                                                                 
	childRequestPowerFlags &= fMergedOutputPowerFlags;

	                                                            
	                                           

	outputPowerFlags = fPowerStates[fCurrentPowerState].outputPowerFlags;
	if (fMachineState != kIOPM_Finished) {
		if (IS_POWER_DROP && !IS_ROOT_DOMAIN) {
			                                                 
			                                                      
			                             
			                                        
			                                                
			  
			                                                         
			                                                             
			                                                             
			                                        
			  
			                                                                
			                                                                

			if ((fMachineState == kIOPM_OurChangeTellClientsPowerDown) ||
			    (fMachineState == kIOPM_OurChangeTellPriorityClientsPowerDown)) {
				fDoNotPowerDown = true;                     
				adjustPower     = true;                         
				PM_LOG1("%s: power drop cancelled in state %u by %s\n",
				    getName(), fMachineState, child->getName());
			} else {
				                                                         
				outputPowerFlags =
				    fPowerStates[fHeadNotePowerState].outputPowerFlags;
			}
		} else if (IS_POWER_RISE) {
			                                                              
			                                                              
			                                                              
			                                                              
			                                                             
			                                                             
			                              

			outputPowerFlags = childConnection->parentCurrentPowerFlags();
		}
	}
	child->fHeadNoteDomainTargetFlags |= outputPowerFlags;

	                                                               

	for (order = 0; order < fNumberOfPowerStates; order++) {
		powerState = fPowerStates[order].stateOrderToIndex;
		if ((fPowerStates[powerState].outputPowerFlags & childRequestPowerFlags)
		    == childRequestPowerFlags) {
			break;
		}
	}
	if (order >= fNumberOfPowerStates) {
		powerState = kPowerStateZero;
	}

	                                                              
	                                                              
	                                                    

	if (!childConnection->childHasRequestedPower() ||
	    (powerState != childConnection->getDesiredDomainState())) {
		adjustPower = true;
	}

#if ENABLE_DEBUG_LOGS
	if (adjustPower) {
		PM_LOG("requestPowerDomainState[%s]: %s, init %d, %u->%u\n",
		    getName(), child->getName(),
		    !childConnection->childHasRequestedPower(),
		    (uint32_t) childConnection->getDesiredDomainState(),
		    (uint32_t) powerState);
	}
#endif

	                                                
	childConnection->setChildHasRequestedPower();
	childConnection->setDesiredDomainState( powerState );

	                                                             
	                                                              
	                                                    

	if (adjustPower && !fDeviceOverrideEnabled &&
	    (!fAdjustPowerScheduled || gIOPMRequest->getRootRequest())) {
		subRequest = acquirePMRequest(
			this, kIOPMRequestTypeAdjustPowerState, gIOPMRequest );
		if (subRequest) {
			submitPMRequest( subRequest );
			fAdjustPowerScheduled = true;
		}
	}

	return kIOReturnSuccess;
}

                                                                                   
                                 
  
                                                                              
                                              
  
                                                            
                                                                                   

IOReturn
IOService::temporaryPowerClampOn( void )
{
	return requestPowerState( gIOPMPowerClientChildProxy, kIOPMPowerStateMax );
}

                                                                                   
                      
  
                                                                       
                                                                      
                                                                       
                                                         
  
                                                              
                       
                                                                                   

IOReturn
IOService::makeUsable( void )
{
	OUR_PMLog(kPMLogMakeUsable, 0, 0);
	return requestPowerState( gIOPMPowerClientDevice, kIOPMPowerStateMax );
}

                                                                                   
                             
                                                                                   

IOPMPowerFlags
IOService::currentCapability( void )
{
	if (!initialized) {
		return IOPMNotPowerManaged;
	}

	return fCurrentCapabilityFlags;
}

                                                                                   
                              
  
                                                                                
                                                                                 
                                                               
                                                                                   

IOReturn
IOService::changePowerStateTo( unsigned long ordinal )
{
	OUR_PMLog(kPMLogChangeStateTo, ordinal, 0);
	return requestPowerState( gIOPMPowerClientDriver, ordinal );
}

                                                                                   
                                     
  
                                                                             
                                                                           
                                                               
                                                                                   

IOReturn
IOService::changePowerStateToPriv( unsigned long ordinal )
{
	OUR_PMLog(kPMLogChangeStateToPriv, ordinal, 0);
	return requestPowerState( gIOPMPowerClientDevice, ordinal );
}

                                                                                   
                                          
  
                                                                             
                                                                           
                                                               
                                                               
                                                                                   

IOReturn
IOService::changePowerStateWithOverrideTo( IOPMPowerStateIndex ordinal,
    IOPMRequestTag tag )
{
	IOPMRequest * request;

	if (!initialized) {
		return kIOPMNotYetInitialized;
	}

	OUR_PMLog(kPMLogChangeStateToPriv, ordinal, 0);

	request = acquirePMRequest( this, kIOPMRequestTypeRequestPowerStateOverride );
	if (!request) {
		return kIOReturnNoMemory;
	}

	gIOPMPowerClientDevice->retain();
	request->fTag  = tag;
	request->fArg0 = (void *) ordinal;
	request->fArg1 = (void *) gIOPMPowerClientDevice;
	request->fArg2 = NULL;
#if NOT_READY
	if (action) {
		request->installCompletionAction( action, target, param );
	}
#endif

	                                                                 
	                                           
	  
	                                      

	if (gIOPMWorkLoop->inGate() && (ordinal < fNumberOfPowerStates)) {
		fTempClampPowerState = StateMax(fTempClampPowerState, ordinal);
		fTempClampCount++;
		request->fArg2 = (void *)(uintptr_t) true;

		                                                             
		                                                  
		fOverrideMaxPowerState = ordinal;
	}

	submitPMRequest( request );
	return IOPMNoErr;
}

                                                                                   
                                      
                                                                                   

IOReturn
IOService::changePowerStateWithTagTo( IOPMPowerStateIndex ordinal, IOPMRequestTag tag )
{
	OUR_PMLog(kPMLogChangeStateTo, ordinal, tag);
	return requestPowerState(gIOPMPowerClientDriver, ordinal, tag);
}

                                                                                   
                                          
                                                                                   

IOReturn
IOService::changePowerStateWithTagToPriv( unsigned long ordinal, IOPMRequestTag tag )
{
	OUR_PMLog(kPMLogChangeStateToPriv, ordinal, tag);
	return requestPowerState(gIOPMPowerClientDevice, ordinal, tag);
}

                                                                                   
                                         
  
                                                      
                                                                                   

IOReturn
IOService::changePowerStateForRootDomain( IOPMPowerStateIndex ordinal )
{
	OUR_PMLog(kPMLogChangeStateForRootDomain, ordinal, 0);
	return requestPowerState( gIOPMPowerClientRootDomain, ordinal );
}

                                                                                   
                                     
  
                                                                
                                             
                                                                                   

IOReturn
IOService::quiescePowerTree(
	void * target, IOPMCompletionAction action, void * param )
{
	IOPMRequest * request;

	if (!initialized) {
		return kIOPMNotYetInitialized;
	}
	if (!target || !action) {
		return kIOReturnBadArgument;
	}

	OUR_PMLog(kPMLogQuiescePowerTree, 0, 0);

	                                                                         
	                                                                         
	                                                                         
	                                               

	request = acquirePMRequest(gIOPMRootNode, kIOPMRequestTypeQuiescePowerTree);
	if (!request) {
		return kIOReturnNoMemory;
	}

	request->installCompletionAction(target, action, param);

	                                                                          
	                                                                          
	                                                                           

	submitPMRequest( request );
	return kIOReturnSuccess;
}

                                                                                   
                              
                                                                                   

IOReturn
IOService::requestPowerState(
	const OSSymbol *      client,
	IOPMPowerStateIndex   state,
	IOPMRequestTag        tag )
{
	IOPMRequest * request;

	if (!client || (state > UINT_MAX)) {
		return kIOReturnBadArgument;
	}
	if (!initialized) {
		return kIOPMNotYetInitialized;
	}

	request = acquirePMRequest( this, kIOPMRequestTypeRequestPowerState );
	if (!request) {
		return kIOReturnNoMemory;
	}

	client->retain();
	request->fTag  = tag;
	request->fArg0 = (void *)(uintptr_t) state;
	request->fArg1 = (void *)            client;
	request->fArg2 = NULL;
#if NOT_READY
	if (action) {
		request->installCompletionAction( action, target, param );
	}
#endif

	                                                                 
	                                           

	if (gIOPMWorkLoop->inGate() && (state < fNumberOfPowerStates)) {
		fTempClampPowerState = StateMax(fTempClampPowerState, state);
		fTempClampCount++;
		request->fArg2 = (void *)(uintptr_t) true;
	}

	submitPMRequest( request );
	return IOPMNoErr;
}

                                                                                   
                                    
                                                                                   

void
IOService::handleRequestPowerState( IOPMRequest * request )
{
	const OSSymbol * client   = (const OSSymbol *)    request->fArg1;
	IOPMPowerStateIndex state = (IOPMPowerStateIndex) request->fArg0;

	PM_ASSERT_IN_GATE();
	if (request->fArg2) {
		assert(fTempClampCount != 0);
		if (fTempClampCount) {
			fTempClampCount--;
		}
		if (!fTempClampCount) {
			fTempClampPowerState = kPowerStateZero;
		}
	}

	if (fNumberOfPowerStates && (state >= fNumberOfPowerStates)) {
		state = fHighestPowerState;
	}

	                                                                        
	                                                                   

	if ((getPMRequestType() != kIOPMRequestTypeRequestPowerStateOverride) &&
	    (client == gIOPMPowerClientDevice)) {
		fOverrideMaxPowerState = kIOPMPowerStateMax;
	}

	if ((state == kPowerStateZero) &&
	    (client != gIOPMPowerClientDevice) &&
	    (client != gIOPMPowerClientDriver) &&
	    (client != gIOPMPowerClientChildProxy)) {
		removePowerClient(client);
	} else {
		updatePowerClient(client, state);
	}

	adjustPowerState();
	client->release();
}

                                                                                   
                                                             
                                                                                   

void
IOService::updatePowerClient( const OSSymbol * client, IOPMPowerStateIndex powerState )
{
	IOPMPowerStateIndex oldPowerState = kPowerStateZero;

	if (powerState > UINT_MAX) {
		assert(false);
		return;
	}

	if (!fPowerClients) {
		fPowerClients = OSDictionary::withCapacity(4);
	}
	if (fPowerClients && client) {
		OSNumber * num = (OSNumber *) fPowerClients->getObject(client);
		if (num) {
			oldPowerState = num->unsigned32BitValue();
			num->setValue(powerState);
		} else {
			num = OSNumber::withNumber(powerState, 32);
			if (num) {
				fPowerClients->setObject(client, num);
				num->release();
			}
		}

		PM_ACTION_CLIENT(actionUpdatePowerClient, client, oldPowerState, powerState);
	}
}

void
IOService::removePowerClient( const OSSymbol * client )
{
	if (fPowerClients && client) {
		fPowerClients->removeObject(client);
	}
}

IOPMPowerStateIndex
IOService::getPowerStateForClient( const OSSymbol * client )
{
	IOPMPowerStateIndex powerState = kPowerStateZero;

	if (fPowerClients && client) {
		OSNumber * num = (OSNumber *) fPowerClients->getObject(client);
		if (num) {
			powerState = num->unsigned32BitValue();
		}
	}
	return powerState;
}

                                                                                   
                                  
                                                                                   

IOReturn
IOService::powerOverrideOnPriv( void )
{
	IOPMRequest * request;

	if (!initialized) {
		return IOPMNotYetInitialized;
	}

	if (gIOPMWorkLoop->inGate()) {
		fDeviceOverrideEnabled = true;
		return IOPMNoErr;
	}

	request = acquirePMRequest( this, kIOPMRequestTypePowerOverrideOnPriv );
	if (!request) {
		return kIOReturnNoMemory;
	}

	submitPMRequest( request );
	return IOPMNoErr;
}

                                                                                   
                                   
                                                                                   

IOReturn
IOService::powerOverrideOffPriv( void )
{
	IOPMRequest * request;

	if (!initialized) {
		return IOPMNotYetInitialized;
	}

	if (gIOPMWorkLoop->inGate()) {
		fDeviceOverrideEnabled = false;
		return IOPMNoErr;
	}

	request = acquirePMRequest( this, kIOPMRequestTypePowerOverrideOffPriv );
	if (!request) {
		return kIOReturnNoMemory;
	}

	submitPMRequest( request );
	return IOPMNoErr;
}

                                                                                   
                                       
                                                                                   

void
IOService::handlePowerOverrideChanged( IOPMRequest * request )
{
	PM_ASSERT_IN_GATE();
	if (request->getType() == kIOPMRequestTypePowerOverrideOnPriv) {
		OUR_PMLog(kPMLogOverrideOn, 0, 0);
		fDeviceOverrideEnabled = true;
	} else {
		OUR_PMLog(kPMLogOverrideOff, 0, 0);
		fDeviceOverrideEnabled = false;
	}

	adjustPowerState();
}

                                                                                   
                                
                                                                                   

void
IOService::computeDesiredState( unsigned long localClamp, bool computeOnly )
{
	OSIterator *        iter;
	OSObject *          next;
	IOPowerConnection * connection;
	IOPMPowerStateIndex desiredState  = kPowerStateZero;
	IOPMPowerStateIndex newPowerState = kPowerStateZero;
	bool                hasChildren   = false;

	                                                                

	if (!fNumberOfPowerStates) {
		fDesiredPowerState = kPowerStateZero;
		return;
	}

	                                              

	iter = getChildIterator(gIOPowerPlane);
	if (iter) {
		while ((next = iter->getNextObject())) {
			if ((connection = OSDynamicCast(IOPowerConnection, next))) {
				if (connection->getReadyFlag() == false) {
					PM_LOG3("[%s] %s: connection not ready\n",
					    getName(), __FUNCTION__);
					continue;
				}
				if (connection->childHasRequestedPower()) {
					hasChildren = true;
				}
				desiredState = StateMax(connection->getDesiredDomainState(), desiredState);
			}
		}
		iter->release();
	}
	if (hasChildren) {
		updatePowerClient(gIOPMPowerClientChildren, desiredState);
	} else {
		removePowerClient(gIOPMPowerClientChildren);
	}

	                                                                      

	iter = OSCollectionIterator::withCollection(fPowerClients);
	if (iter) {
		const OSSymbol * client;
		while ((client = (const OSSymbol *) iter->getNextObject())) {
			                                                      
			if ((fDeviceOverrideEnabled ||
			    (getPMRequestType() == kIOPMRequestTypeRequestPowerStateOverride)) &&
			    ((client == gIOPMPowerClientChildren) ||
			    (client == gIOPMPowerClientDriver))) {
				continue;
			}

			                                                
			if (hasChildren && (client == gIOPMPowerClientChildProxy)) {
				continue;
			}

			                                                                
			if (client == gIOPMPowerClientAdvisoryTickle &&
			    !gIOPMAdvisoryTickleEnabled) {
				continue;
			}

			desiredState = getPowerStateForClient(client);
			assert(desiredState < fNumberOfPowerStates);
			PM_LOG1("  %u %s\n",
			    (uint32_t) desiredState, client->getCStringNoCopy());

			newPowerState = StateMax(newPowerState, desiredState);

			if (client == gIOPMPowerClientDevice) {
				fDeviceDesire = desiredState;
			}
		}
		iter->release();
	}

	                                         

	newPowerState = StateMax(newPowerState, localClamp);
	newPowerState = StateMax(newPowerState, fTempClampPowerState);

	                                          

	newPowerState = StateMin(newPowerState, fOverrideMaxPowerState);

	                                              

	if (newPowerState >= fNumberOfPowerStates) {
		newPowerState = fHighestPowerState;
	}

	if (getPMRootDomain()->isAOTMode()) {
		if ((kIOPMPreventIdleSleep & fPowerStates[newPowerState].capabilityFlags)
		    && !(kIOPMPreventIdleSleep & fPowerStates[fDesiredPowerState].capabilityFlags)) {
			getPMRootDomain()->claimSystemWakeEvent(this, kIOPMWakeEventAOTExit, getName(), NULL);
		}
	}

	fDesiredPowerState = newPowerState;

	PM_LOG1("  temp %u, clamp %u, current %u, new %u\n",
	    (uint32_t) localClamp, (uint32_t) fTempClampPowerState,
	    (uint32_t) fCurrentPowerState, (uint32_t) newPowerState);

	if (!computeOnly) {
		                                                                   
		                                   

		if (fIdleTimerPeriod && fIdleTimerStopped) {
			restartIdleTimer();
		}

		                                                                    
		                                                                    
		                                                                    
		                                                                    
		                                                                     
		                                        

		if ((getPMRequestType() != kIOPMRequestTypeActivityTickle) &&
		    (fActivityTicklePowerState != kInvalidTicklePowerState)) {
			IOLockLock(fActivityLock);
			fActivityTicklePowerState = kInvalidTicklePowerState;
			IOLockUnlock(fActivityLock);
		}
	}
}

                                                                                   
                                   
  
                                                                                   

unsigned long
IOService::currentPowerConsumption( void )
{
	if (!initialized) {
		return kIOPMUnknown;
	}

	return fCurrentPowerConsumption;
}

                                                                                   
                             
                                                                                   

#ifndef __LP64__
IOWorkLoop *
IOService::getPMworkloop( void )
{
	return gIOPMWorkLoop;
}
#endif

#if NOT_YET

                                                                                   
                                
                                                                                   

static void
applyToPowerChildren(
	IOService *               service,
	IOServiceApplierFunction  applier,
	void *                    context,
	IOOptionBits              options )
{
	PM_ASSERT_IN_GATE();

	IORegistryEntry *       entry;
	IORegistryIterator *    iter;
	IOPowerConnection *     connection;
	IOService *             child;

	iter = IORegistryIterator::iterateOver(service, gIOPowerPlane, options);
	if (iter) {
		while ((entry = iter->getNextObject())) {
			                                         
			if ((connection = OSDynamicCast(IOPowerConnection, entry))) {
				child = (IOService *) connection->copyChildEntry(gIOPowerPlane);
				if (child) {
					(*applier)(child, context);
					child->release();
				}
			}
		}
		iter->release();
	}
}

static void
applyToPowerParent(
	IOService *               service,
	IOServiceApplierFunction  applier,
	void *                    context,
	IOOptionBits              options )
{
	PM_ASSERT_IN_GATE();

	IORegistryEntry *       entry;
	IORegistryIterator *    iter;
	IOPowerConnection *     connection;
	IOService *             parent;

	iter = IORegistryIterator::iterateOver(service, gIOPowerPlane,
	    options | kIORegistryIterateParents);
	if (iter) {
		while ((entry = iter->getNextObject())) {
			                                         
			if ((connection = OSDynamicCast(IOPowerConnection, entry))) {
				parent = (IOService *) connection->copyParentEntry(gIOPowerPlane);
				if (parent) {
					(*applier)(parent, context);
					parent->release();
				}
			}
		}
		iter->release();
	}
}

#endif              

          
                                     

void
IOService::setAdvisoryTickleEnable( bool enable )
{
	gIOPMAdvisoryTickleEnabled = enable;
}

                                                                                   
                          
  
                                                                       
                                                                        
                                        
                                                                    
                                       
                                                                                   

bool
IOService::activityTickle( unsigned long type, unsigned long stateNumber )
{
	IOPMRequest *   request;
	bool            noPowerChange = true;
	uint32_t        tickleFlags;

	if (!initialized) {
		return true;                   
	}
	if ((type == kIOPMSuperclassPolicy1) && StateOrder(stateNumber)) {
		IOLockLock(fActivityLock);

		                                                     

		fDeviceWasActive = true;
		fActivityTickleCount++;
		clock_get_uptime(&fDeviceActiveTimestamp);

		PM_ACTION_TICKLE(actionActivityTickle);

		                                      
		                                                
		                                                  

		if ((fActivityTicklePowerState == kInvalidTicklePowerState)
		    || StateOrder(fActivityTicklePowerState) < StateOrder(stateNumber)) {
			fActivityTicklePowerState = stateNumber;
			noPowerChange = false;

			tickleFlags = kTickleTypeActivity | kTickleTypePowerRise;
			request = acquirePMRequest( this, kIOPMRequestTypeActivityTickle );
			if (request) {
				request->fArg0 = (void *)            stateNumber;
				request->fArg1 = (void *)(uintptr_t) tickleFlags;
				request->fArg2 = (void *)(uintptr_t) gIOPMTickleGeneration;
				submitPMRequest(request);
			}
		}

		IOLockUnlock(fActivityLock);
	} else if ((type == kIOPMActivityTickleTypeAdvisory) &&
	    ((stateNumber = fDeviceUsablePowerState) != kPowerStateZero)) {
		IOLockLock(fActivityLock);

		fAdvisoryTickled = true;

		if (fAdvisoryTicklePowerState != stateNumber) {
			fAdvisoryTicklePowerState = stateNumber;
			noPowerChange = false;

			tickleFlags = kTickleTypeAdvisory | kTickleTypePowerRise;
			request = acquirePMRequest( this, kIOPMRequestTypeActivityTickle );
			if (request) {
				request->fArg0 = (void *)            stateNumber;
				request->fArg1 = (void *)(uintptr_t) tickleFlags;
				request->fArg2 = (void *)(uintptr_t) gIOPMTickleGeneration;
				submitPMRequest(request);
			}
		}

		IOLockUnlock(fActivityLock);
	}

	                                                                    
	                                        

	return noPowerChange;
}

                                                                                   
                                 
                                                                                   

void
IOService::handleActivityTickle( IOPMRequest * request )
{
	IOPMPowerStateIndex ticklePowerState = (IOPMPowerStateIndex) request->fArg0;
	IOPMPowerStateIndex tickleFlags      = (IOPMPowerStateIndex) request->fArg1;
	uint32_t            tickleGeneration = (uint32_t)(uintptr_t) request->fArg2;
	bool adjustPower = false;

	PM_ASSERT_IN_GATE();
	if (fResetPowerStateOnWake && (tickleGeneration != gIOPMTickleGeneration)) {
		                                                               
		                                                               
		                                                              
		                                                              
		                                                           
		                                                 

		return;
	}

	if (tickleFlags & kTickleTypeActivity) {
		IOPMPowerStateIndex deviceDesireOrder = StateOrder(fDeviceDesire);
		IOPMPowerStateIndex idleTimerGeneration = ticklePowerState;                        

		if (tickleFlags & kTickleTypePowerRise) {
			if ((StateOrder(ticklePowerState) > deviceDesireOrder) &&
			    (ticklePowerState < fNumberOfPowerStates)) {
				fIdleTimerMinPowerState = ticklePowerState;
				updatePowerClient(gIOPMPowerClientDevice, ticklePowerState);
				adjustPower = true;
			}
		} else if ((deviceDesireOrder > StateOrder(fIdleTimerMinPowerState)) &&
		    (idleTimerGeneration == fIdleTimerGeneration)) {
			                                           
			                                                              
			                                                                 
			                                                               
			                                                               
			                                               

			deviceDesireOrder--;
			if (deviceDesireOrder < fNumberOfPowerStates) {
				ticklePowerState = fPowerStates[deviceDesireOrder].stateOrderToIndex;
				updatePowerClient(gIOPMPowerClientDevice, ticklePowerState);
				adjustPower = true;
			}
		}
	} else {                   
		if (tickleFlags & kTickleTypePowerRise) {
			if ((ticklePowerState == fDeviceUsablePowerState) &&
			    (ticklePowerState < fNumberOfPowerStates)) {
				updatePowerClient(gIOPMPowerClientAdvisoryTickle, ticklePowerState);
				fHasAdvisoryDesire = true;
				fAdvisoryTickleUsed = true;
				adjustPower = true;
			} else {
				IOLockLock(fActivityLock);
				fAdvisoryTicklePowerState = kInvalidTicklePowerState;
				IOLockUnlock(fActivityLock);
			}
		} else if (fHasAdvisoryDesire) {
			removePowerClient(gIOPMPowerClientAdvisoryTickle);
			fHasAdvisoryDesire = false;
			adjustPower = true;
		}
	}

	if (adjustPower) {
		adjustPowerState();
	}
}

                                                                                
                              
  
                                                                            
                                              
                                                                                

IOReturn
IOService::setIdleTimerPeriod( unsigned long period )
{
	if (!initialized) {
		return IOPMNotYetInitialized;
	}

	OUR_PMLog(kPMLogSetIdleTimerPeriod, period, fIdleTimerPeriod);

	if (period > INT_MAX) {
		return kIOReturnBadArgument;
	}

	IOPMRequest * request =
	    acquirePMRequest( this, kIOPMRequestTypeSetIdleTimerPeriod );
	if (!request) {
		return kIOReturnNoMemory;
	}

	request->fArg0 = (void *) period;
	submitPMRequest( request );

	return kIOReturnSuccess;
}

IOReturn
IOService::setIgnoreIdleTimer( bool ignore )
{
	if (!initialized) {
		return IOPMNotYetInitialized;
	}

	OUR_PMLog(kIOPMRequestTypeIgnoreIdleTimer, ignore, 0);

	IOPMRequest * request =
	    acquirePMRequest( this, kIOPMRequestTypeIgnoreIdleTimer );
	if (!request) {
		return kIOReturnNoMemory;
	}

	request->fArg0 = (void *) ignore;
	submitPMRequest( request );

	return kIOReturnSuccess;
}

                                                                                
                           
  
                                                                      
                           
                                                                                

SInt32
IOService::nextIdleTimeout(
	AbsoluteTime currentTime,
	AbsoluteTime lastActivity,
	unsigned int powerState)
{
	AbsoluteTime        delta;
	UInt64              delta_ns;
	SInt32              delta_secs;
	SInt32              delay_secs;

	                                                            
	delta = currentTime;
	SUB_ABSOLUTETIME(&delta, &lastActivity);

	                        
	absolutetime_to_nanoseconds(delta, &delta_ns);
	delta_secs = (SInt32)(delta_ns / NSEC_PER_SEC);

	                                                          
	if (delta_secs < (int) fIdleTimerPeriod) {
		delay_secs = (int) fIdleTimerPeriod - delta_secs;
	} else {
		delay_secs = (int) fIdleTimerPeriod;
	}

	return (SInt32)delay_secs;
}

                                                                                   
                               
                                                                                   

void
IOService::start_PM_idle_timer( void )
{
	static const int    maxTimeout = 100000;
	static const int    minTimeout = 1;
	AbsoluteTime        uptime, deadline;
	SInt32              idle_in = 0;
	boolean_t           pending;

	if (!initialized || !fIdleTimerPeriod ||
	    ((unsigned int) fCurrentPowerState != fCurrentPowerState)) {
		return;
	}

	IOLockLock(fActivityLock);

	clock_get_uptime(&uptime);

	                                             
	idle_in = nextIdleTimeout(uptime, fDeviceActiveTimestamp, (unsigned int) fCurrentPowerState);

	                                   
	if (idle_in > maxTimeout) {
		                              
		idle_in = IOService::nextIdleTimeout(uptime,
		    fDeviceActiveTimestamp,
		    (unsigned int) fCurrentPowerState);
	} else if (idle_in < minTimeout) {
		idle_in = fIdleTimerPeriod;
	}

	IOLockUnlock(fActivityLock);

	fNextIdleTimerPeriod = idle_in;
	fIdleTimerStartTime = uptime;

	retain();
	clock_interval_to_absolutetime_interval(idle_in, kSecondScale, &deadline);
	ADD_ABSOLUTETIME(&deadline, &uptime);
	pending = thread_call_enter_delayed(fIdleTimer, deadline);
	if (pending) {
		release();
	}
}

                                                                                   
                             
                                                                                   

void
IOService::restartIdleTimer( void )
{
	if (fDeviceDesire != kPowerStateZero) {
		fIdleTimerStopped = false;
		fActivityTickleCount = 0;
		start_PM_idle_timer();
	} else if (fHasAdvisoryDesire) {
		fIdleTimerStopped = false;
		start_PM_idle_timer();
	} else {
		fIdleTimerStopped = true;
	}
}

                                                                                   
                     
                                                                                   

static void
idle_timer_expired(
	thread_call_param_t arg0, thread_call_param_t arg1 )
{
	IOService * me = (IOService *) arg0;

	if (gIOPMWorkLoop) {
		gIOPMWorkLoop->runAction(
			OSMemberFunctionCast(IOWorkLoop::Action, me,
			&IOService::idleTimerExpired),
			me);
	}

	me->release();
}

                                                                                   
                             
  
                                                                        
                                                                        
                                                                        
                                                                                   

void
IOService::idleTimerExpired( void )
{
	IOPMRequest *   request;
	bool            restartTimer = true;
	uint32_t        tickleFlags;

	if (!initialized || !fIdleTimerPeriod || fIdleTimerStopped ||
	    fLockedFlags.PMStop) {
		return;
	}

	fIdleTimerStartTime = 0;

	IOLockLock(fActivityLock);

	                                                              

	if (fDeviceWasActive) {
		                                                        
		fDeviceWasActive = false;
	} else if (!fIdleTimerIgnored) {
		                                                      
		                                                         
		                                                                     
		                                                                   

		if ((fActivityTicklePowerState != kInvalidTicklePowerState) &&
		    (fActivityTicklePowerState != kPowerStateZero)) {
			fActivityTicklePowerState--;
		}

		tickleFlags = kTickleTypeActivity | kTickleTypePowerDrop;
		request = acquirePMRequest( this, kIOPMRequestTypeActivityTickle );
		if (request) {
			request->fArg0 = (void *)(uintptr_t) fIdleTimerGeneration;
			request->fArg1 = (void *)(uintptr_t) tickleFlags;
			request->fArg2 = (void *)(uintptr_t) gIOPMTickleGeneration;
			submitPMRequest( request );

			                                                               
			             

			restartTimer = false;
		}
	}

	if (fAdvisoryTickled) {
		fAdvisoryTickled = false;
	} else if (fHasAdvisoryDesire) {
		                                                                  
		fAdvisoryTicklePowerState = kInvalidTicklePowerState;

		tickleFlags = kTickleTypeAdvisory | kTickleTypePowerDrop;
		request = acquirePMRequest( this, kIOPMRequestTypeActivityTickle );
		if (request) {
			request->fArg0 = (void *)(uintptr_t) fIdleTimerGeneration;
			request->fArg1 = (void *)(uintptr_t) tickleFlags;
			request->fArg2 = (void *)(uintptr_t) gIOPMTickleGeneration;
			submitPMRequest( request );

			                                                               
			             

			restartTimer = false;
		}
	}

	IOLockUnlock(fActivityLock);

	if (restartTimer) {
		start_PM_idle_timer();
	}
}

#ifndef __LP64__
                                                                                   
                                        
                                                                                   

void
IOService::PM_idle_timer_expiration( void )
{
}

                                                                                   
                                
                                                                                   

void
IOService::command_received( void *statePtr, void *, void *, void * )
{
}
#endif                

                                                                                   
                             
  
                                                                                 
                                                        
                                                                                   

IOReturn
IOService::setAggressiveness( unsigned long type, unsigned long newLevel )
{
	return kIOReturnSuccess;
}

                                                                                   
                             
  
                             
                                                                                   

IOReturn
IOService::getAggressiveness( unsigned long type, unsigned long * currentLevel )
{
	IOPMrootDomain *    rootDomain = getPMRootDomain();

	if (!rootDomain) {
		return kIOReturnNotReady;
	}

	return rootDomain->getAggressiveness( type, currentLevel );
}

                                                                                   
                         
  
                                                                                   

UInt32
IOService::getPowerState( void )
{
	if (!initialized) {
		return kPowerStateZero;
	}

	return (UInt32) fCurrentPowerState;
}

#ifndef __LP64__
                                                                                   
                          
  
                                                              
                                                        
                                                                                   

IOReturn
IOService::systemWake( void )
{
	OSIterator *        iter;
	OSObject *          next;
	IOPowerConnection * connection;
	IOService *         theChild;

	iter = getChildIterator(gIOPowerPlane);
	if (iter) {
		while ((next = iter->getNextObject())) {
			if ((connection = OSDynamicCast(IOPowerConnection, next))) {
				if (connection->getReadyFlag() == false) {
					PM_LOG3("[%s] %s: connection not ready\n",
					    getName(), __FUNCTION__);
					continue;
				}

				theChild = (IOService *)connection->copyChildEntry(gIOPowerPlane);
				if (theChild) {
					theChild->systemWake();
					theChild->release();
				}
			}
		}
		iter->release();
	}

	if (fControllingDriver != NULL) {
		if (fControllingDriver->didYouWakeSystem()) {
			makeUsable();
		}
	}

	return IOPMNoErr;
}

                                                                                   
                                          
                                                                                   

IOReturn
IOService::temperatureCriticalForZone( IOService * whichZone )
{
	IOService * theParent;
	IOService * theNub;

	OUR_PMLog(kPMLogCriticalTemp, 0, 0);

	if (inPlane(gIOPowerPlane) && !IS_PM_ROOT) {
		theNub = (IOService *)copyParentEntry(gIOPowerPlane);
		if (theNub) {
			theParent = (IOService *)theNub->copyParentEntry(gIOPowerPlane);
			theNub->release();
			if (theParent) {
				theParent->temperatureCriticalForZone(whichZone);
				theParent->release();
			}
		}
	}
	return IOPMNoErr;
}
#endif                

          
                              

                                                                                   
                             
  
                                       
                                                                                   

IOReturn
IOService::startPowerChange(
	IOPMPowerChangeFlags    changeFlags,
	IOPMPowerStateIndex     powerState,
	IOPMPowerFlags          domainFlags,
	IOPowerConnection *     parentConnection,
	IOPMPowerFlags          parentFlags )
{
	uint32_t savedPMActionsState;

	PM_ASSERT_IN_GATE();
	assert( fMachineState == kIOPM_Finished );
	assert( powerState < fNumberOfPowerStates );

	if (powerState >= fNumberOfPowerStates) {
		return IOPMAckImplied;
	}

	fIsPreChange = true;
	savedPMActionsState = fPMActions.state;
	PM_ACTION_CHANGE(actionPowerChangeOverride, &powerState, &changeFlags);

	                          
	                                                             
	                                                                
	                                                                 
	                                                     
	if (!fAdjustPowerScheduled &&
	    ((changeFlags & kIOPMSelfInitiated) == 0) &&
	    ((fPMActions.state & kPMActionsStatePowerClamped) == 0) &&
	    ((savedPMActionsState & kPMActionsStatePowerClamped) != 0)) {
		IOPMRequest * request = acquirePMRequest(this, kIOPMRequestTypeAdjustPowerState);
		if (request) {
			submitPMRequest(request);
			fAdjustPowerScheduled = true;
		}
	}

	if (changeFlags & kIOPMExpireIdleTimer) {
		                                                    
		if (StateOrder(fDeviceDesire) > StateOrder(powerState)) {
			                                                      
			updatePowerClient(gIOPMPowerClientDevice, powerState);
			computeDesiredState(kPowerStateZero, true);

			                                                                  
			IOLockLock(fActivityLock);
			fDeviceWasActive = false;
			fActivityTicklePowerState = kInvalidTicklePowerState;
			IOLockUnlock(fActivityLock);

			fIdleTimerMinPowerState = kPowerStateZero;
		}
	}

	                                                                
	                                 

	if (changeFlags & kIOPMNotDone) {
		return IOPMAckImplied;
	}

	                                                                 

	fHeadNoteChangeFlags      = changeFlags;
	fHeadNotePowerState       = powerState;
	fHeadNotePowerArrayEntry  = &fPowerStates[powerState];
	fHeadNoteParentConnection = NULL;

	if (changeFlags & kIOPMSelfInitiated) {
		if (changeFlags & kIOPMSynchronize) {
			OurSyncStart();
		} else {
			OurChangeStart();
		}
		return 0;
	} else {
		assert(changeFlags & kIOPMParentInitiated);
		fHeadNoteDomainFlags = domainFlags;
		fHeadNoteParentFlags = parentFlags;
		fHeadNoteParentConnection = parentConnection;
		return ParentChangeStart();
	}
}

                                                                                   
                                    
                                                                                   

bool
IOService::notifyInterestedDrivers( void )
{
	IOPMinformee *      informee;
	IOPMinformeeList *  list = fInterestedDrivers;
	DriverCallParam *   param;
	unsigned long       numItems;
	uint32_t            count;
	uint32_t            skipCnt = 0;

	PM_ASSERT_IN_GATE();
	assert( fDriverCallParamCount == 0 );
	assert( fHeadNotePendingAcks == 0 );

	fHeadNotePendingAcks = 0;

	numItems = list->numberOfItems();
	if (!numItems || ((uint32_t) numItems != numItems)) {
		goto done;                                         
	}
	count = (uint32_t) numItems;

	                                                                  
	                                                                  
	                                                                  
	             

	param = (DriverCallParam *) fDriverCallParamPtr;
	if (count > fDriverCallParamSlots) {
		if (fDriverCallParamSlots) {
			assert(fDriverCallParamPtr);
			IODelete(fDriverCallParamPtr, DriverCallParam, fDriverCallParamSlots);
			fDriverCallParamPtr = NULL;
			fDriverCallParamSlots = 0;
		}

		param = IONew(DriverCallParam, count);
		if (!param) {
			goto done;             
		}
		fDriverCallParamPtr   = (void *) param;
		fDriverCallParamSlots = count;
	}

	informee = list->firstInList();
	assert(informee);
	for (IOItemCount i = 0; i < count; i++) {
		if (fInitialSetPowerState || (fHeadNoteChangeFlags & kIOPMInitialPowerChange)) {
			                                                               
			                                         
			if ((this == informee->whatObject) &&
			    (fHeadNotePowerArrayEntry->capabilityFlags & kIOPMInitialDeviceState)) {
				skipCnt++;
				continue;
			}
		}
		informee->timer = -1;
		param[i].Target = informee;
		informee->retain();
		informee = list->nextInList( informee );
	}

	count -= skipCnt;
	if (!count) {
		goto done;
	}
	fDriverCallParamCount = count;
	fHeadNotePendingAcks  = count;

	                                                       
	assert(!fDriverCallBusy);
	fDriverCallBusy = true;
	thread_call_enter( fDriverCallEntry );
	return true;

done:
	                                                                        
	                               
	return false;
}

                                                                                   
                                        
                                                                                   

void
IOService::notifyInterestedDriversDone( void )
{
	IOPMinformee *      informee;
	IOItemCount         count;
	DriverCallParam *   param;
	IOReturn            result;
	int                 maxTimeout = 0;

	PM_ASSERT_IN_GATE();
	assert( fDriverCallBusy == false );
	assert( fMachineState == kIOPM_DriverThreadCallDone );

	param = (DriverCallParam *) fDriverCallParamPtr;
	count = fDriverCallParamCount;

	if (param && count) {
		for (IOItemCount i = 0; i < count; i++, param++) {
			informee = (IOPMinformee *) param->Target;
			result   = param->Result;

			if ((result == IOPMAckImplied) || (result < 0)) {
				                                           
				                                                        
				                                                      
				                         

				if (fHeadNotePendingAcks && informee->timer) {
					fHeadNotePendingAcks--;
				}

				informee->timer = 0;
			} else if (informee->timer) {
				assert(informee->timer == -1);

				                                                            
				                                               
				                                                            
				                                                            
				                                                            
				                                                

				if (result < kMinAckTimeoutTicks) {
					result = kMinAckTimeoutTicks;
				}

				informee->timer = (result / (ACK_TIMER_PERIOD / ns_per_us)) + 1;
				if (result > maxTimeout) {
					maxTimeout = result;
				}
			}
			                                                                
			                                         
			                                                                   
			                                                         

			informee->release();
		}

		fDriverCallParamCount = 0;

		if (fHeadNotePendingAcks) {
			OUR_PMLog(kPMLogStartAckTimer, 0, 0);
			start_ack_timer();
			getPMRootDomain()->reset_watchdog_timer(this, maxTimeout / USEC_PER_SEC + 1);
		}
	}

	MS_POP();                                               

	                                                                  
	                                                                   
	                                         

	if (!fHeadNotePendingAcks) {
		notifyRootDomain();
	} else {
		MS_PUSH(fMachineState);
		fMachineState = kIOPM_NotifyChildrenStart;
	}
}

                                                                                   
                             
                                                                                   

void
IOService::notifyRootDomain( void )
{
	assert( fDriverCallBusy == false );

	                                                 
	                                                                           
	                                                           
	if (!IS_ROOT_DOMAIN || (fMachineState != kIOPM_OurChangeSetPowerState)) {
		notifyChildren();
		return;
	}

	MS_PUSH(fMachineState);                                  
	fMachineState = kIOPM_DriverThreadCallDone;

	                                                                  
	                       
	fDriverCallReason = kRootDomainInformPreChange;
	fDriverCallBusy   = true;
	thread_call_enter( fDriverCallEntry );
}

void
IOService::notifyRootDomainDone( void )
{
	assert( fDriverCallBusy == false );
	assert( fMachineState == kIOPM_DriverThreadCallDone );

	MS_POP();                                 
	notifyChildren();
}

                                                                                   
                           
                                                                                   

void
IOService::notifyChildren( void )
{
	OSIterator *        iter;
	OSObject *          next;
	IOPowerConnection * connection;
	OSArray *           children = NULL;
	IOPMrootDomain *    rootDomain;
	bool                delayNotify = false;

	if ((fHeadNotePowerState != fCurrentPowerState) &&
	    (IS_POWER_DROP == fIsPreChange) &&
	    ((rootDomain = getPMRootDomain()) == this)) {
		rootDomain->tracePoint( IS_POWER_DROP ?
		    kIOPMTracePointSleepPowerPlaneDrivers :
		    kIOPMTracePointWakePowerPlaneDrivers  );
	}

	if (fStrictTreeOrder) {
		children = OSArray::withCapacity(8);
	}

	                                               
	fHeadNotePowerArrayEntry->staticPower = 0;

	iter = getChildIterator(gIOPowerPlane);
	if (iter) {
		while ((next = iter->getNextObject())) {
			if ((connection = OSDynamicCast(IOPowerConnection, next))) {
				if (connection->getReadyFlag() == false) {
					PM_LOG3("[%s] %s: connection not ready\n",
					    getName(), __FUNCTION__);
					continue;
				}

				                                                       
				                                                       
				                                                     

				if (!fIsPreChange &&
				    connection->delayChildNotification &&
				    getPMRootDomain()->shouldDelayChildNotification(this)) {
					if (!children) {
						children = OSArray::withCapacity(8);
						if (children) {
							delayNotify = true;
						}
					}
					if (delayNotify) {
						children->setObject( connection );
						continue;
					}
				}

				if (!delayNotify && children) {
					children->setObject( connection );
				} else {
					notifyChild( connection );
				}
			}
		}
		iter->release();
	}

	if (children && (children->getCount() == 0)) {
		children->release();
		children = NULL;
	}
	if (children) {
		assert(fNotifyChildArray == NULL);
		fNotifyChildArray = children;
		MS_PUSH(fMachineState);

		if (delayNotify) {
			                                                        
			                                                        
			                                                        
			                                                       
			                                                        
			                                                         
			                                 

			fMachineState = kIOPM_NotifyChildrenDelayed;
			PM_LOG2("%s: %d children in delayed array\n",
			    getName(), children->getCount());
		} else {
			                                                            
			                                              

			fMachineState = kIOPM_NotifyChildrenOrdered;
		}
	}
}

                                                                                   
                                  
                                                                                   

void
IOService::notifyChildrenOrdered( void )
{
	PM_ASSERT_IN_GATE();
	assert(fNotifyChildArray);
	assert(fMachineState == kIOPM_NotifyChildrenOrdered);

	                                                                    
	                                                                   
	                                                                     
	                                                                    
	                                    
	  
	                                                                 
	                                                                 

	if (fNotifyChildArray->getCount()) {
		IOPowerConnection * connection;
		connection = (IOPowerConnection *) fNotifyChildArray->getObject(0);
		notifyChild( connection );
		fNotifyChildArray->removeObject(0);
	} else {
		fNotifyChildArray->release();
		fNotifyChildArray = NULL;

		MS_POP();                              
	}
}

                                                                                   
                                  
                                                                                   

void
IOService::notifyChildrenDelayed( void )
{
	IOPowerConnection * connection;

	PM_ASSERT_IN_GATE();
	assert(fNotifyChildArray);
	assert(fMachineState == kIOPM_NotifyChildrenDelayed);

	                                                                          
	                                                                       
	                                                                        
	                                                 

	for (int i = 0;; i++) {
		connection = (IOPowerConnection *) fNotifyChildArray->getObject(i);
		if (!connection) {
			break;
		}

		notifyChild( connection );
	}

	PM_LOG2("%s: notified delayed children\n", getName());
	fNotifyChildArray->release();
	fNotifyChildArray = NULL;

	MS_POP();                              
}

                                                                                   
                      
                                                                                   

IOReturn
IOService::notifyAll( uint32_t nextMS )
{
	                                                                         

	PM_ASSERT_IN_GATE();
	MS_PUSH(nextMS);
	fMachineState     = kIOPM_DriverThreadCallDone;
	fDriverCallReason = fIsPreChange ?
	    kDriverCallInformPreChange : kDriverCallInformPostChange;

	if (!notifyInterestedDrivers()) {
		notifyInterestedDriversDone();
	}

	return IOPMWillAckLater;
}

                                                                                   
                                    
  
                      
                                                                                   

IOReturn
IOService::actionDriverCalloutDone(
	OSObject * target,
	void * arg0, void * arg1,
	void * arg2, void * arg3 )
{
	IOServicePM * pwrMgt = (IOServicePM *) arg0;

	assert( fDriverCallBusy );
	fDriverCallBusy = false;

	assert(gIOPMWorkQueue);
	gIOPMWorkQueue->signalWorkAvailable();

	return kIOReturnSuccess;
}

void
IOService::pmDriverCallout( IOService * from )
{
	assert(from);
	switch (from->fDriverCallReason) {
	case kDriverCallSetPowerState:
		from->driverSetPowerState();
		break;

	case kDriverCallInformPreChange:
	case kDriverCallInformPostChange:
		from->driverInformPowerChange();
		break;

	case kRootDomainInformPreChange:
		getPMRootDomain()->willNotifyPowerChildren(from->fHeadNotePowerState);
		break;

	default:
		panic("IOService::pmDriverCallout bad machine state %x",
		    from->fDriverCallReason);
	}

	gIOPMWorkLoop->runAction(actionDriverCalloutDone,
	                 from,
	                 (void *) from->pwrMgt );
}

                                                                                   
                                
  
                      
                                                                                   

void
IOService::driverSetPowerState( void )
{
	IOPMPowerStateIndex powerState;
	DriverCallParam *   param;
	IOPMDriverCallEntry callEntry;
	AbsoluteTime        end;
	IOReturn            result;
	uint32_t            oldPowerState = getPowerState();

	assert( fDriverCallBusy );
	assert( fDriverCallParamPtr );
	assert( fDriverCallParamCount == 1 );

	param = (DriverCallParam *) fDriverCallParamPtr;
	powerState = fHeadNotePowerState;

	if (assertPMDriverCall(&callEntry, kIOPMDriverCallMethodSetPowerState)) {
		OUR_PMLogFuncStart(kPMLogProgramHardware, (uintptr_t) this, powerState);
		clock_get_uptime(&fDriverCallStartTime);

		if (reserved && reserved->uvars && reserved->uvars->userServer) {
			result = reserved->uvars->userServer->serviceSetPowerState(fControllingDriver, this, fHeadNotePowerArrayEntry->capabilityFlags, powerState);
		} else {
			result = fControllingDriver->setPowerState( powerState, this );
		}
		clock_get_uptime(&end);
		OUR_PMLogFuncEnd(kPMLogProgramHardware, (uintptr_t) this, (UInt32) result);

		deassertPMDriverCall(&callEntry);

		                                                            
		                                                            
		if (powerState == fHighestPowerState) {
			fMaxPowerStateEntryTime = end;
		} else if (oldPowerState == fHighestPowerState) {
			fMaxPowerStateExitTime = end;
		}

		if (result < 0) {
			PM_LOG("%s::setPowerState(%p, %lu -> %lu) returned 0x%x\n",
			    fName, OBFUSCATE(this), fCurrentPowerState, powerState, result);
		}


		if ((result == IOPMAckImplied) || (result < 0)) {
			uint64_t    nsec;

			SUB_ABSOLUTETIME(&end, &fDriverCallStartTime);
			absolutetime_to_nanoseconds(end, &nsec);
			if (nsec > gIOPMSetPowerStateLogNS) {
				getPMRootDomain()->pmStatsRecordApplicationResponse(
					gIOPMStatsDriverPSChangeSlow,
					fName, kDriverCallSetPowerState, NS_TO_MS(nsec), getRegistryEntryID(),
					NULL, powerState);
			}
		}
	} else {
		result = kIOPMAckImplied;
	}

	param->Result = result;
}

                                                                                   
                                    
  
                      
                                                                                   

void
IOService::driverInformPowerChange( void )
{
	IOPMinformee *      informee;
	IOService *         driver;
	DriverCallParam *   param;
	IOPMDriverCallEntry callEntry;
	IOPMPowerFlags      powerFlags;
	IOPMPowerStateIndex powerState;
	AbsoluteTime        end;
	IOReturn            result;
	IOItemCount         count;
	IOOptionBits        callMethod = (fDriverCallReason == kDriverCallInformPreChange) ?
	    kIOPMDriverCallMethodWillChange : kIOPMDriverCallMethodDidChange;

	assert( fDriverCallBusy );
	assert( fDriverCallParamPtr );
	assert( fDriverCallParamCount );

	param = (DriverCallParam *) fDriverCallParamPtr;
	count = fDriverCallParamCount;

	powerFlags = fHeadNotePowerArrayEntry->capabilityFlags;
	powerState = fHeadNotePowerState;

	for (IOItemCount i = 0; i < count; i++) {
		informee = (IOPMinformee *) param->Target;
		driver   = informee->whatObject;

		if (assertPMDriverCall(&callEntry, callMethod, informee)) {
			if (fDriverCallReason == kDriverCallInformPreChange) {
				OUR_PMLogFuncStart(kPMLogInformDriverPreChange, (uintptr_t) this, powerState);
				clock_get_uptime(&informee->startTime);
				result = driver->powerStateWillChangeTo(powerFlags, powerState, this);
				clock_get_uptime(&end);
				OUR_PMLogFuncEnd(kPMLogInformDriverPreChange, (uintptr_t) this, result);
			} else {
				OUR_PMLogFuncStart(kPMLogInformDriverPostChange, (uintptr_t) this, powerState);
				clock_get_uptime(&informee->startTime);
				result = driver->powerStateDidChangeTo(powerFlags, powerState, this);
				clock_get_uptime(&end);
				OUR_PMLogFuncEnd(kPMLogInformDriverPostChange, (uintptr_t) this, result);
			}

			deassertPMDriverCall(&callEntry);


			if ((result == IOPMAckImplied) || (result < 0)) {
				uint64_t nsec;

				SUB_ABSOLUTETIME(&end, &informee->startTime);
				absolutetime_to_nanoseconds(end, &nsec);
				if (nsec > gIOPMSetPowerStateLogNS) {
					getPMRootDomain()->pmStatsRecordApplicationResponse(
						gIOPMStatsDriverPSChangeSlow, driver->getName(),
						fDriverCallReason, NS_TO_MS(nsec), driver->getRegistryEntryID(),
						NULL, powerState);
				}
			}
		} else {
			result = kIOPMAckImplied;
		}

		param->Result = result;
		param++;
	}
}

                                                                                   
                        
  
                                                           
                                                                 
                                                                                   

bool
IOService::notifyChild( IOPowerConnection * theNub )
{
	IOReturn                ret = IOPMAckImplied;
	unsigned long           childPower;
	IOService *             theChild;
	IOPMRequest *           childRequest;
	IOPMPowerChangeFlags    requestArg2;
	int                     requestType;

	PM_ASSERT_IN_GATE();
	theChild = (IOService *)(theNub->copyChildEntry(gIOPowerPlane));
	if (!theChild) {
		return true;
	}

	                                                                    
	                                                                  
	fHeadNotePendingAcks++;
	theNub->setAwaitingAck(true);

	requestArg2 = fHeadNoteChangeFlags;
	if (StateOrder(fHeadNotePowerState) < StateOrder(fCurrentPowerState)) {
		requestArg2 |= kIOPMDomainPowerDrop;
	}

	requestType = fIsPreChange ?
	    kIOPMRequestTypePowerDomainWillChange :
	    kIOPMRequestTypePowerDomainDidChange;

	childRequest = acquirePMRequest( theChild, requestType );
	if (childRequest) {
		theNub->retain();
		childRequest->fArg0 = (void *) fHeadNotePowerArrayEntry->outputPowerFlags;
		childRequest->fArg1 = (void *) theNub;
		childRequest->fArg2 = (void *)(uintptr_t) requestArg2;
		theChild->submitPMRequest( childRequest );
		ret = IOPMWillAckLater;
	} else {
		ret = IOPMAckImplied;
		fHeadNotePendingAcks--;
		theNub->setAwaitingAck(false);
		childPower = theChild->currentPowerConsumption();
		if (childPower == kIOPMUnknown) {
			fHeadNotePowerArrayEntry->staticPower = kIOPMUnknown;
		} else {
			if (fHeadNotePowerArrayEntry->staticPower != kIOPMUnknown) {
				fHeadNotePowerArrayEntry->staticPower += childPower;
			}
		}
	}

	theChild->release();
	return IOPMAckImplied == ret;
}

                                                                                   
                                    
                                                                                   

bool
IOService::notifyControllingDriver( void )
{
	DriverCallParam *   param;

	PM_ASSERT_IN_GATE();
	assert( fDriverCallParamCount == 0  );
	assert( fControllingDriver );

	if (fInitialSetPowerState) {
		fInitialSetPowerState = false;
		fHeadNoteChangeFlags |= kIOPMInitialPowerChange;

		                                                           
		if (fHeadNotePowerArrayEntry->capabilityFlags & kIOPMInitialDeviceState) {
			return false;
		}
	}

	param = (DriverCallParam *) fDriverCallParamPtr;
	if (!param) {
		param = IONew(DriverCallParam, 1);
		if (!param) {
			return false;             
		}
		fDriverCallParamPtr   = (void *) param;
		fDriverCallParamSlots = 1;
	}

	param->Target = fControllingDriver;
	fDriverCallParamCount = 1;
	fDriverTimer = -1;

	                                                       
	assert(!fDriverCallBusy);
	fDriverCallBusy = true;
	thread_call_enter( fDriverCallEntry );

	return true;
}

                                                                                   
                                        
                                                                                   

void
IOService::notifyControllingDriverDone( void )
{
	DriverCallParam *   param;
	IOReturn            result;

	PM_ASSERT_IN_GATE();
	param = (DriverCallParam *) fDriverCallParamPtr;

	assert( fDriverCallBusy == false );
	assert( fMachineState == kIOPM_DriverThreadCallDone );

	if (param && fDriverCallParamCount) {
		assert(fDriverCallParamCount == 1);

		                                        
		result = param->Result;

		if ((result == IOPMAckImplied) || (result < 0)) {
			fDriverTimer = 0;
		} else if (fDriverTimer) {
			assert(fDriverTimer == -1);

			                                                            
			                                               
			                                                            
			                                                            
			                                                            
			                                                

			if (result < kMinAckTimeoutTicks) {
				result = kMinAckTimeoutTicks;
			}

			fDriverTimer = (result / (ACK_TIMER_PERIOD / ns_per_us)) + 1;
		}
		                                                             

		fDriverCallParamCount = 0;

		if (fDriverTimer) {
			OUR_PMLog(kPMLogStartAckTimer, 0, 0);
			start_ack_timer();
			getPMRootDomain()->reset_watchdog_timer(this, result / USEC_PER_SEC + 1);
		}
	}

	MS_POP();                                      
	fIsPreChange  = false;
}

                                                                                   
                     
  
                          
                                                                                   

void
IOService::all_done( void )
{
	IOPMPowerStateIndex     prevPowerState;
	const IOPMPSEntry *     powerStatePtr;
	IOPMDriverCallEntry     callEntry;
	uint32_t                prevMachineState = fMachineState;
	bool                    actionCalled = false;
	uint64_t                ts;

	fMachineState = kIOPM_Finished;

	if ((fHeadNoteChangeFlags & kIOPMSynchronize) &&
	    ((prevMachineState == kIOPM_Finished) ||
	    (prevMachineState == kIOPM_SyncFinish))) {
		                                               
		                                                                  
		                                           

		PM_ACTION_CHANGE(actionPowerChangeDone,
		    fHeadNotePowerState, fHeadNoteChangeFlags);

		if (getPMRequestType() == kIOPMRequestTypeSynchronizePowerTree) {
			powerChangeDone(fCurrentPowerState);
		} else if (fAdvisoryTickleUsed) {
			                                              
			                                                          
			                                                          
			                                                        
			                                                        
			                                     

			if (!fAdjustPowerScheduled &&
			    (fHeadNoteChangeFlags & kIOPMDomainDidChange)) {
				IOPMRequest * request;
				request = acquirePMRequest( this, kIOPMRequestTypeAdjustPowerState );
				if (request) {
					submitPMRequest( request );
					fAdjustPowerScheduled = true;
				}
			}
		}

		return;
	}

	                   
	if (fHeadNoteChangeFlags & kIOPMSelfInitiated) {
		                      
		if ((fHeadNoteChangeFlags & kIOPMNotDone) == 0) {
			trackSystemSleepPreventers(
				fCurrentPowerState, fHeadNotePowerState, fHeadNoteChangeFlags);

			                              
			requestDomainPower(fHeadNotePowerState);

			                        
			if (StateOrder(fCurrentPowerState) < StateOrder(fHeadNotePowerState)) {
				                               
				tellChangeUp(fHeadNotePowerState);
			}
			prevPowerState = fCurrentPowerState;
			             
			fCurrentPowerState = fHeadNotePowerState;
			PM_LOCK();
			if (fReportBuf) {
				ts = mach_absolute_time();
				STATEREPORT_SETSTATE(fReportBuf, (uint16_t) fCurrentPowerState, ts);
			}
			PM_UNLOCK();
#if PM_VARS_SUPPORT
			fPMVars->myCurrentState = fCurrentPowerState;
#endif
			OUR_PMLog(kPMLogChangeDone, fCurrentPowerState, prevPowerState);
			PM_ACTION_CHANGE(actionPowerChangeDone,
			    prevPowerState, fHeadNoteChangeFlags);
			actionCalled = true;

			powerStatePtr = &fPowerStates[fCurrentPowerState];
			fCurrentCapabilityFlags = powerStatePtr->capabilityFlags;
			if (fCurrentCapabilityFlags & kIOPMStaticPowerValid) {
				fCurrentPowerConsumption = powerStatePtr->staticPower;
			}

			if (fHeadNoteChangeFlags & kIOPMRootChangeDown) {
				                                                            
				gIOPMTickleGeneration++;
			}

			                               
			if (fPCDFunctionOverride && fParentsKnowState &&
			    assertPMDriverCall(&callEntry, kIOPMDriverCallMethodChangeDone, NULL, kIOPMDriverCallNoInactiveCheck)) {
				powerChangeDone(prevPowerState);
				deassertPMDriverCall(&callEntry);
			}
		} else if (getPMRequestType() == kIOPMRequestTypeRequestPowerStateOverride) {
			                                                 
			fOverrideMaxPowerState = kIOPMPowerStateMax;
		}
	}

	                                
	if (fHeadNoteChangeFlags & kIOPMParentInitiated) {
		if (fHeadNoteChangeFlags & kIOPMRootChangeDown) {
			ParentChangeRootChangeDown();
		}

		                      
		if ((fHeadNoteChangeFlags & kIOPMNotDone) == 0) {
			trackSystemSleepPreventers(
				fCurrentPowerState, fHeadNotePowerState, fHeadNoteChangeFlags);

			                   
			if (StateOrder(fCurrentPowerState) < StateOrder(fHeadNotePowerState)) {
				                               
				tellChangeUp(fHeadNotePowerState);
			}
			             
			prevPowerState = fCurrentPowerState;
			fCurrentPowerState = fHeadNotePowerState;
			PM_LOCK();
			if (fReportBuf) {
				ts = mach_absolute_time();
				STATEREPORT_SETSTATE(fReportBuf, (uint16_t) fCurrentPowerState, ts);
			}
			PM_UNLOCK();
#if PM_VARS_SUPPORT
			fPMVars->myCurrentState = fCurrentPowerState;
#endif

			OUR_PMLog(kPMLogChangeDone, fCurrentPowerState, prevPowerState);
			PM_ACTION_CHANGE(actionPowerChangeDone,
			    prevPowerState, fHeadNoteChangeFlags);
			actionCalled = true;

			powerStatePtr = &fPowerStates[fCurrentPowerState];
			fCurrentCapabilityFlags = powerStatePtr->capabilityFlags;
			if (fCurrentCapabilityFlags & kIOPMStaticPowerValid) {
				fCurrentPowerConsumption = powerStatePtr->staticPower;
			}

			                               
			if (fPCDFunctionOverride && fParentsKnowState &&
			    assertPMDriverCall(&callEntry, kIOPMDriverCallMethodChangeDone, NULL, kIOPMDriverCallNoInactiveCheck)) {
				powerChangeDone(prevPowerState);
				deassertPMDriverCall(&callEntry);
			}
		}
	}

	                                                                         
	                                                                      

	if (StateOrder(fCurrentPowerState) >= StateOrder(fIdleTimerMinPowerState)) {
		fIdleTimerMinPowerState = kPowerStateZero;
	}

	if (!actionCalled) {
		PM_ACTION_CHANGE(actionPowerChangeDone,
		    fHeadNotePowerState, fHeadNoteChangeFlags);
	}
}

          
                                         

                                                                                   
                           
  
                                                          
                                                                                   

void
IOService::OurChangeStart( void )
{
	PM_ASSERT_IN_GATE();
	OUR_PMLog( kPMLogStartDeviceChange, fHeadNotePowerState, fCurrentPowerState );

	                                                                          
	                                                                          
	                                                                

	if (!IS_PM_ROOT && (StateOrder(fMaxPowerState) < StateOrder(fHeadNotePowerState))) {
		fHeadNoteChangeFlags |= kIOPMNotDone;
		requestDomainPower(fHeadNotePowerState);
		OurChangeFinish();
		return;
	}

	                                                                 

	if (!fInitialPowerChange && (fHeadNotePowerState == fCurrentPowerState)) {
		OurChangeFinish();
		return;
	}
	fInitialPowerChange = false;

	                                          
	                                                         

	PM_ACTION_CHANGE(actionPowerChangeStart, fHeadNotePowerState, &fHeadNoteChangeFlags);

	                                                                     
	                                                                    

	if (IS_POWER_DROP) {
		fDoNotPowerDown = false;

		                                          
		fMachineState = kIOPM_OurChangeTellClientsPowerDown;
		fOutOfBandParameter = kNotifyApps;
		askChangeDown(fHeadNotePowerState);
	} else {
		                                                                    
		                                                                  
		                                                                       
		                                                                    
		                     
		  
		                                                                     
		                                                                     
		                                                                   
		                                                                       
		                               

		IOReturn ret;

		                                                                 
		ret = requestDomainPower( fHeadNotePowerState, kReserveDomainPower );
		if (ret != kIOReturnSuccess) {
			                                        
			fHeadNoteChangeFlags |= kIOPMNotDone;
			OurChangeFinish();
			return;
		}

		OurChangeTellCapabilityWillChange();
	}
}

                                                                                   
                                      
  
                                                       
                                                                                   

struct IOPMRequestDomainPowerContext {
	IOService *     child;                                 
	IOPMPowerFlags  requestPowerFlags;                                 
};

static void
requestDomainPowerApplier(
	IORegistryEntry *   entry,
	void *              inContext )
{
	IOPowerConnection *             connection;
	IOService *                     parent;
	IOPMRequestDomainPowerContext * context;

	if ((connection = OSDynamicCast(IOPowerConnection, entry)) == NULL) {
		return;
	}
	parent = (IOService *) connection->copyParentEntry(gIOPowerPlane);
	if (!parent) {
		return;
	}

	assert(inContext);
	context = (IOPMRequestDomainPowerContext *) inContext;

	if (connection->parentKnowsState() && connection->getReadyFlag()) {
		parent->requestPowerDomainState(
			context->requestPowerFlags,
			connection,
			IOPMLowestState);
	}

	parent->release();
}

                                                                                   
                               
  
                                                                               
                                                                            
                                           
                                                                                   

IOReturn
IOService::requestDomainPower(
	IOPMPowerStateIndex ourPowerState,
	IOOptionBits        options )
{
	IOPMPowerFlags                  requestPowerFlags;
	IOPMPowerStateIndex             maxPowerState;
	IOPMRequestDomainPowerContext   context;

	PM_ASSERT_IN_GATE();
	assert(ourPowerState < fNumberOfPowerStates);
	if (ourPowerState >= fNumberOfPowerStates) {
		return kIOReturnBadArgument;
	}
	if (IS_PM_ROOT) {
		return kIOReturnSuccess;
	}

	                                                             
	                                                             

	requestPowerFlags = fPowerStates[ourPowerState].inputPowerFlags;

	                                                          

	if (((options & kReserveDomainPower) == 0) &&
	    (fPreviousRequestPowerFlags == requestPowerFlags)) {
		                                                
		goto done;
	}
	fPreviousRequestPowerFlags = requestPowerFlags;

	                                                              
	context.child              = this;
	context.requestPowerFlags  = requestPowerFlags;
	fHeadNoteDomainTargetFlags = 0;
	applyToParents(requestDomainPowerApplier, &context, gIOPowerPlane);

	if (options & kReserveDomainPower) {
		maxPowerState = fControllingDriver->maxCapabilityForDomainState(
			fHeadNoteDomainTargetFlags );

		if (StateOrder(maxPowerState) < StateOrder(ourPowerState)) {
			PM_LOG1("%s: power desired %u:0x%x got %u:0x%x\n",
			    getName(),
			    (uint32_t) ourPowerState, (uint32_t) requestPowerFlags,
			    (uint32_t) maxPowerState, (uint32_t) fHeadNoteDomainTargetFlags);
			return kIOReturnNoPower;
		}
	}

done:
	return kIOReturnSuccess;
}

                                                                                   
                         
                                                                                   

void
IOService::OurSyncStart( void )
{
	PM_ASSERT_IN_GATE();

	if (fInitialPowerChange) {
		return;
	}

	PM_ACTION_CHANGE(actionPowerChangeStart, fHeadNotePowerState, &fHeadNoteChangeFlags);

	if (fHeadNoteChangeFlags & kIOPMNotDone) {
		OurChangeFinish();
		return;
	}

	if (fHeadNoteChangeFlags & kIOPMSyncTellPowerDown) {
		fDoNotPowerDown = false;

		                                         
		fMachineState = kIOPM_SyncTellClientsPowerDown;
		fOutOfBandParameter = kNotifyApps;
		askChangeDown(fHeadNotePowerState);
	} else {
		                                          
		tellSystemCapabilityChange( kIOPM_SyncNotifyWillChange );
	}
}

                                                                                   
                                          
  
                                                                               
                                                                             
                                                                                   

void
IOService::OurChangeTellClientsPowerDown( void )
{
	if (!IS_ROOT_DOMAIN) {
		fMachineState = kIOPM_OurChangeTellPriorityClientsPowerDown;
	} else {
		fMachineState = kIOPM_OurChangeTellUserPMPolicyPowerDown;
	}
	tellChangeDown1(fHeadNotePowerState);
}

                                                                                   
                                               
  
                                                                               
                                                                                
                                      
                                                                                   
void
IOService::OurChangeTellUserPMPolicyPowerDown( void )
{
	fMachineState = kIOPM_OurChangeTellPriorityClientsPowerDown;
	fOutOfBandParameter = kNotifyApps;

	tellClientsWithResponse(kIOPMMessageLastCallBeforeSleep);
}

                                                                                   
                                                  
  
                                                                              
                                                                        
                                                                                   

void
IOService::OurChangeTellPriorityClientsPowerDown( void )
{
	fMachineState = kIOPM_OurChangeNotifyInterestedDriversWillChange;
	tellChangeDown2(fHeadNotePowerState);
}

                                                                                   
                                              
  
                                                                   
                                                     
                                                                                   

void
IOService::OurChangeTellCapabilityWillChange( void )
{
	if (!IS_ROOT_DOMAIN) {
		return OurChangeNotifyInterestedDriversWillChange();
	}

	tellSystemCapabilityChange( kIOPM_OurChangeNotifyInterestedDriversWillChange );
}

                                                                                   
                                                       
  
                                                                                
                                                
                                                                                   

void
IOService::OurChangeNotifyInterestedDriversWillChange( void )
{
	IOPMrootDomain * rootDomain;
	if ((rootDomain = getPMRootDomain()) == this) {
		if (IS_POWER_DROP) {
			rootDomain->tracePoint( kIOPMTracePointSleepWillChangeInterests );
		} else {
			rootDomain->tracePoint( kIOPMTracePointWakeWillChangeInterests );
		}
	}

	notifyAll( kIOPM_OurChangeSetPowerState );
}

                                                                                   
                                   
  
                                                                              
                                      
                                                                                   

void
IOService::OurChangeSetPowerState( void )
{
	MS_PUSH( kIOPM_OurChangeWaitForPowerSettle );
	fMachineState     = kIOPM_DriverThreadCallDone;
	fDriverCallReason = kDriverCallSetPowerState;

	if (notifyControllingDriver() == false) {
		notifyControllingDriverDone();
	}
}

                                                                                   
                                        
  
                                                                            
                                                       
                                                                                   

void
IOService::OurChangeWaitForPowerSettle( void )
{
	fMachineState = kIOPM_OurChangeNotifyInterestedDriversDidChange;
	startSettleTimer();
}

                                                                                   
                                                      
  
                                                                   
                                          
                                                                                   

void
IOService::OurChangeNotifyInterestedDriversDidChange( void )
{
	IOPMrootDomain * rootDomain;
	if ((rootDomain = getPMRootDomain()) == this) {
		rootDomain->tracePoint( IS_POWER_DROP ?
		    kIOPMTracePointSleepDidChangeInterests :
		    kIOPMTracePointWakeDidChangeInterests  );
	}

	notifyAll( kIOPM_OurChangeTellCapabilityDidChange );
}

                                                                                   
                                             
  
                                                     
                                                                                   

void
IOService::OurChangeTellCapabilityDidChange( void )
{
	if (!IS_ROOT_DOMAIN) {
		return OurChangeFinish();
	}

	if (!IS_POWER_DROP) {
		                                                            
		                              
		getPMRootDomain()->willTellSystemCapabilityDidChange();
	}

	getPMRootDomain()->tracePoint( IS_POWER_DROP ?
	    kIOPMTracePointSleepCapabilityClients :
	    kIOPMTracePointWakeCapabilityClients  );

	tellSystemCapabilityChange( kIOPM_OurChangeFinish );
}

                                                                                   
                            
  
                                                  
                                                                                   

void
IOService::OurChangeFinish( void )
{
	all_done();
}

          
                                         

                                                                                   
                              
  
                                                                          
                                                                                   

IOReturn
IOService::ParentChangeStart( void )
{
	PM_ASSERT_IN_GATE();
	OUR_PMLog( kPMLogStartParentChange, fHeadNotePowerState, fCurrentPowerState );

	                                                            
	if ((fHeadNoteChangeFlags & (kIOPMDomainDidChange | kIOPMRootChangeUp)) ==
	    (kIOPMDomainDidChange | kIOPMRootChangeUp)) {
		                                                                 
		if (fIdleTimerPeriod && fIdleTimerStopped) {
			restartIdleTimer();
		}
	}

	                                            
	if (StateOrder(fHeadNotePowerState) < StateOrder(fCurrentPowerState)) {
		PM_ACTION_CHANGE(actionPowerChangeStart, fHeadNotePowerState, &fHeadNoteChangeFlags);

		                               
		fInitialPowerChange = false;
		fMachineState = kIOPM_ParentChangeTellPriorityClientsPowerDown;
		tellChangeDown1(fHeadNotePowerState);
		return IOPMWillAckLater;
	}

	                                                                       
	if (StateOrder(fHeadNotePowerState) > StateOrder(fCurrentPowerState)) {
		if (StateOrder(fDesiredPowerState) > StateOrder(fCurrentPowerState)) {
			if (StateOrder(fDesiredPowerState) < StateOrder(fHeadNotePowerState)) {
				                                   
				fHeadNotePowerState = fDesiredPowerState;
				fHeadNotePowerArrayEntry = &fPowerStates[fDesiredPowerState];
				OUR_PMLog(kPMLogAmendParentChange, fHeadNotePowerState, 0);
			}
		} else {
			                          
			fHeadNotePowerState = fCurrentPowerState;
			fHeadNotePowerArrayEntry = &fPowerStates[fCurrentPowerState];
			OUR_PMLog(kPMLogAmendParentChange, fHeadNotePowerState, 0);
		}
	}

	if (fHeadNoteChangeFlags & kIOPMDomainDidChange) {
		if (StateOrder(fHeadNotePowerState) > StateOrder(fCurrentPowerState)) {
			PM_ACTION_CHANGE(actionPowerChangeStart,
			    fHeadNotePowerState, &fHeadNoteChangeFlags);

			                                             
			fInitialPowerChange = false;
			ParentChangeTellCapabilityWillChange();
			return IOPMWillAckLater;
		} else if (fHeadNoteChangeFlags & kIOPMRootBroadcastFlags) {
			                                                      
			                   
			fMachineState     = kIOPM_SyncNotifyDidChange;
			fDriverCallReason = kDriverCallInformPreChange;
			fHeadNoteChangeFlags |= kIOPMNotDone;
			notifyChildren();
			return IOPMWillAckLater;
		}
	}

	                                  
	fHeadNoteChangeFlags |= kIOPMNotDone;

	all_done();
	return IOPMAckImplied;
}

                                                                                
                                       
  
                                                                             
                                                                              
                                                                           
                                  
                                                                                

void
IOService::ParentChangeRootChangeDown( void )
{
	                                                    
	if (fIdleTimerPeriod && !fIdleTimerStopped) {
		fIdleTimerStopped = true;
		if (fIdleTimer && thread_call_cancel(fIdleTimer)) {
			release();
		}
	}

	if (fResetPowerStateOnWake) {
		                                                      
		                                                          
		                                                          

		if (fDeviceDesire != kPowerStateZero) {
			updatePowerClient(gIOPMPowerClientDevice, kPowerStateZero);
			computeDesiredState(kPowerStateZero, true);
			requestDomainPower( fDesiredPowerState );
			PM_LOG1("%s: tickle desire removed\n", fName);
		}

		                                                                  
		IOLockLock(fActivityLock);
		fDeviceWasActive = false;
		fActivityTicklePowerState = kInvalidTicklePowerState;
		IOLockUnlock(fActivityLock);

		fIdleTimerMinPowerState = kPowerStateZero;
	} else if (fAdvisoryTickleUsed) {
		                                                                
		                                                               
		                                               

		AbsoluteTime    now;
		uint64_t        nsec;
		bool            dropTickleDesire = false;

		if (fIdleTimerPeriod && !fIdleTimerIgnored &&
		    (fIdleTimerMinPowerState == kPowerStateZero) &&
		    (fDeviceDesire != kPowerStateZero)) {
			IOLockLock(fActivityLock);

			if (!fDeviceWasActive) {
				                                                   
				                                          
				dropTickleDesire = true;
			} else {
				                                                    
				                              
				clock_get_uptime(&now);
				SUB_ABSOLUTETIME(&now, &fDeviceActiveTimestamp);
				absolutetime_to_nanoseconds(now, &nsec);
				if (nsec >= kNoTickleCancelWindow) {
					dropTickleDesire = true;
				}
			}

			if (dropTickleDesire) {
				                                             
				fDeviceWasActive = false;
				fActivityTicklePowerState = kInvalidTicklePowerState;
			}

			IOLockUnlock(fActivityLock);
		}

		if (dropTickleDesire) {
			                                                          
			                                                          

			updatePowerClient(gIOPMPowerClientDevice, kPowerStateZero);
			computeDesiredState(kPowerStateZero, true);
			PM_LOG1("%s: tickle desire dropped\n", fName);
		}
	}
}

                                                                                   
                                                     
  
                                                                              
                                                                        
                                                                                   

void
IOService::ParentChangeTellPriorityClientsPowerDown( void )
{
	fMachineState = kIOPM_ParentChangeNotifyInterestedDriversWillChange;
	tellChangeDown2(fHeadNotePowerState);
}

                                                                                   
                                                 
  
                                                                                  
                                                                             
                                                                                   

void
IOService::ParentChangeTellCapabilityWillChange( void )
{
	if (!IS_ROOT_DOMAIN) {
		return ParentChangeNotifyInterestedDriversWillChange();
	}

	tellSystemCapabilityChange( kIOPM_ParentChangeNotifyInterestedDriversWillChange );
}

                                                                                   
                                                          
  
                                                                                
                                                
                                                                                   

void
IOService::ParentChangeNotifyInterestedDriversWillChange( void )
{
	notifyAll( kIOPM_ParentChangeSetPowerState );
}

                                                                                   
                                      
  
                                                                              
                                      
                                                                                   

void
IOService::ParentChangeSetPowerState( void )
{
	MS_PUSH( kIOPM_ParentChangeWaitForPowerSettle );
	fMachineState     = kIOPM_DriverThreadCallDone;
	fDriverCallReason = kDriverCallSetPowerState;

	if (notifyControllingDriver() == false) {
		notifyControllingDriverDone();
	}
}

                                                                                   
                                           
  
                                                                               
                                                               
                                                                                   

void
IOService::ParentChangeWaitForPowerSettle( void )
{
	fMachineState = kIOPM_ParentChangeNotifyInterestedDriversDidChange;
	startSettleTimer();
}

                                                                                   
                                                         
  
                                                                              
                                          
                                                                                   

void
IOService::ParentChangeNotifyInterestedDriversDidChange( void )
{
	notifyAll( kIOPM_ParentChangeTellCapabilityDidChange );
}

                                                                                   
                                                
  
                                                     
                                                                                   

void
IOService::ParentChangeTellCapabilityDidChange( void )
{
	if (!IS_ROOT_DOMAIN) {
		return ParentChangeAcknowledgePowerChange();
	}

	tellSystemCapabilityChange( kIOPM_ParentChangeAcknowledgePowerChange );
}

                                                                                   
                                         
  
                                                              
                                                                                   

void
IOService::ParentChangeAcknowledgePowerChange( void )
{
	IORegistryEntry *   nub;
	IOService *         parent;

	nub = fHeadNoteParentConnection;
	nub->retain();
	all_done();
	parent = (IOService *)nub->copyParentEntry(gIOPowerPlane);
	if (parent) {
		parent->acknowledgePowerChange((IOService *)nub);
		parent->release();
	}
	nub->release();
}

          
                              

                                                                                   
                               
  
                                                                           
                              
                                                                                   

void
IOService::settleTimerExpired( void )
{
#if USE_SETTLE_TIMER
	fSettleTimeUS = 0;
	gIOPMWorkQueue->signalWorkAvailable();
#endif
}

                                                                                   
                       
  
                                                              
                                                                                   

#if USE_SETTLE_TIMER
static void
settle_timer_expired( thread_call_param_t arg0, thread_call_param_t arg1 )
{
	IOService * me = (IOService *) arg0;

	if (gIOPMWorkLoop && gIOPMWorkQueue) {
		gIOPMWorkLoop->runAction(
			OSMemberFunctionCast(IOWorkLoop::Action, me, &IOService::settleTimerExpired),
			me);
	}
	me->release();
}
#endif

                                                                                   
                             
  
                                                                      
                                                                                   

void
IOService::startSettleTimer( void )
{
#if USE_SETTLE_TIMER
	                                                                      
	                                                                        
	                                                                      
	                                                                 

	AbsoluteTime        deadline;
	IOPMPowerStateIndex stateIndex;
	IOPMPowerStateIndex currentOrder, newOrder, i;
	uint32_t            settleTime = 0;
	boolean_t           pending;

	PM_ASSERT_IN_GATE();

	currentOrder = StateOrder(fCurrentPowerState);
	newOrder     = StateOrder(fHeadNotePowerState);

	i = currentOrder;

	                 
	if (newOrder < currentOrder) {
		while (i > newOrder) {
			stateIndex = fPowerStates[i].stateOrderToIndex;
			settleTime += (uint32_t) fPowerStates[stateIndex].settleDownTime;
			i--;
		}
	}

	                
	if (newOrder > currentOrder) {
		while (i < newOrder) {
			stateIndex = fPowerStates[i + 1].stateOrderToIndex;
			settleTime += (uint32_t) fPowerStates[stateIndex].settleUpTime;
			i++;
		}
	}

	if (settleTime) {
		retain();
		clock_interval_to_deadline(settleTime, kMicrosecondScale, &deadline);
		pending = thread_call_enter_delayed(fSettleTimer, deadline);
		if (pending) {
			release();
		}
	}
#endif
}

                                                                                   
                         
  
                                                         
                                                           
                                                                            
                                                                   
                                                                   
                                                                        
                                            
  
                                                                         
                                  
                                                                                   

#ifndef __LP64__
#if MACH_ASSERT
__dead2
#endif
void
IOService::ack_timer_ticked( void )
{
	assert(false);
}
#endif                

bool
IOService::ackTimerTick( void )
{
	IOPMinformee *      nextObject;
	bool                done = false;

	PM_ASSERT_IN_GATE();
	switch (fMachineState) {
	case kIOPM_OurChangeWaitForPowerSettle:
	case kIOPM_ParentChangeWaitForPowerSettle:
		                                                        
		if (fDriverTimer > 0) {
			                            
			fDriverTimer--;
			if (fDriverTimer == 0) {
				                              
				uint64_t nsec = computeTimeDeltaNS(&fDriverCallStartTime);
				OUR_PMLog(kPMLogCtrlDriverTardy, 0, 0);
				setProperty(kIOPMTardyAckSPSKey, kOSBooleanTrue);
				PM_ERROR("%s::setPowerState(%p, %lu -> %lu) timed out after %d ms\n",
				    fName, OBFUSCATE(this), fCurrentPowerState, fHeadNotePowerState, NS_TO_MS(nsec));

#if DEBUG || DEVELOPMENT || !defined(XNU_TARGET_OS_OSX)
				bool panic_allowed = false;
				uint32_t setpowerstate_panic = -1;
				PE_parse_boot_argn("setpowerstate_panic", &setpowerstate_panic, sizeof(setpowerstate_panic));
				panic_allowed = setpowerstate_panic != 0;
#ifdef CONFIG_XNUPOST
				uint64_t kernel_post_args = 0;
				PE_parse_boot_argn("kernPOST", &kernel_post_args, sizeof(kernel_post_args));
				if (kernel_post_args != 0) {
					panic_allowed = false;
				}
#endif                     
				if (panic_allowed) {
					                                                                 
					const char *whitelist = "AppleSEPManager";
					if (strncmp(fName, whitelist, strlen(whitelist))) {
						panic("%s::setPowerState(%p, %lu -> %lu) timed out after %d ms",
						    fName, this, fCurrentPowerState, fHeadNotePowerState, NS_TO_MS(nsec));
					}
				} else {
#ifdef CONFIG_XNUPOST
					if (kernel_post_args != 0) {
						PM_ERROR("setPowerState panic disabled by kernPOST boot-arg\n");
					}
#endif                     
					if (setpowerstate_panic != 0) {
						PM_ERROR("setPowerState panic disabled by setpowerstate_panic boot-arg\n");
					}
				}
#else                                                             
				if (gIOKitDebug & kIOLogDebugPower) {
					panic("%s::setPowerState(%p, %lu -> %lu) timed out after %d ms",
					    fName, this, fCurrentPowerState, fHeadNotePowerState, NS_TO_MS(nsec));
				} else {
					                              
					const void *function_addr = NULL;
					OSKext *kext = NULL;
					function_addr = OSMemberFunctionCast(const void *, fControllingDriver, &IOService::setPowerState);
					kext = OSKext::lookupKextWithAddress((vm_address_t)function_addr);
					if (kext) {
#if __has_feature(ptrauth_calls)
						function_addr = (const void*)VM_KERNEL_STRIP_PTR(function_addr);
#endif                                   
						const char *bundleID = kext->getIdentifierCString();
						const char *apple_prefix = "com.apple";
						const char *kernel_prefix = "__kernel__";
						if (strncmp(bundleID, apple_prefix, strlen(apple_prefix)) == 0 || strncmp(bundleID, kernel_prefix, strlen(kernel_prefix)) == 0) {
							                     
							panic("%s::setPowerState(%p : %p, %lu -> %lu) timed out after %d ms",
							    fName, this, function_addr, fCurrentPowerState, fHeadNotePowerState, NS_TO_MS(nsec));
						}
						kext->release();
					}
				}
#endif                                                             
				                                                      
				done = true;
				getPMRootDomain()->reset_watchdog_timer(this, 0);
			} else {
				                                 
				start_ack_timer();
			}
		}
		break;

	case kIOPM_NotifyChildrenStart:
		                                                        
		if (fHeadNotePendingAcks != 0) {
			                                                 
			nextObject = fInterestedDrivers->firstInList();
			                     
			while (nextObject != NULL) {
				if (nextObject->timer > 0) {
					nextObject->timer--;
					                                    
					if (nextObject->timer == 0) {
						uint64_t nsec = computeTimeDeltaNS(&nextObject->startTime);
						OUR_PMLog(kPMLogIntDriverTardy, 0, 0);
						nextObject->whatObject->setProperty(kIOPMTardyAckPSCKey, kOSBooleanTrue);
						PM_ERROR("%s::powerState%sChangeTo(%p, %s, %lu -> %lu) timed out after %d ms\n",
						    nextObject->whatObject->getName(),
						    (fDriverCallReason == kDriverCallInformPreChange) ? "Will" : "Did",
						    OBFUSCATE(nextObject->whatObject), fName, fCurrentPowerState, fHeadNotePowerState,
						    NS_TO_MS(nsec));

						                            
						fHeadNotePendingAcks--;
					}
				}
				nextObject = fInterestedDrivers->nextInList(nextObject);
			}

			                    
			if (fHeadNotePendingAcks == 0) {
				                       
				done = true;
				getPMRootDomain()->reset_watchdog_timer(this, 0);
			} else {
				                      
				start_ack_timer();
			}
		}
		break;

	                        
	case kIOPM_OurChangeTellClientsPowerDown:
	case kIOPM_OurChangeTellUserPMPolicyPowerDown:
	case kIOPM_OurChangeTellPriorityClientsPowerDown:
	case kIOPM_OurChangeNotifyInterestedDriversWillChange:
	case kIOPM_ParentChangeTellPriorityClientsPowerDown:
	case kIOPM_ParentChangeNotifyInterestedDriversWillChange:
	case kIOPM_SyncTellClientsPowerDown:
	case kIOPM_SyncTellPriorityClientsPowerDown:
	case kIOPM_SyncNotifyWillChange:
	case kIOPM_TellCapabilityChangeDone:
		                              
		cleanClientResponses(true);
		OUR_PMLog(kPMLogClientTardy, 0, 1);
		                            
		done = true;
		break;

	default:
		PM_LOG1("%s: unexpected ack timer tick (state = %d)\n",
		    getName(), fMachineState);
		break;
	}
	return done;
}

                                                                                   
                                 
                                                                                   
void
IOService::start_watchdog_timer( void )
{
	int             timeout;
	uint64_t        deadline;

	if (!fWatchdogTimer || (kIOSleepWakeWdogOff & gIOKitDebug)) {
		return;
	}

	IOLockLock(fWatchdogLock);

	timeout = getPMRootDomain()->getWatchdogTimeout();
	clock_interval_to_deadline(timeout, kSecondScale, &deadline);
	fWatchdogDeadline = deadline;
	start_watchdog_timer(deadline);
	IOLockUnlock(fWatchdogLock);
}

void
IOService::start_watchdog_timer(uint64_t deadline)
{
	IOLockAssert(fWatchdogLock, kIOLockAssertOwned);

	if (!thread_call_isactive(fWatchdogTimer)) {
		thread_call_enter_delayed(fWatchdogTimer, deadline);
	}
}

                                                                                   
                                
                                                                                   

void
IOService::stop_watchdog_timer( void )
{
	if (!fWatchdogTimer || (kIOSleepWakeWdogOff & gIOKitDebug)) {
		return;
	}

	IOLockLock(fWatchdogLock);

	thread_call_cancel(fWatchdogTimer);
	fWatchdogDeadline = 0;

	while (fBlockedArray->getCount()) {
		IOService *obj = OSDynamicCast(IOService, fBlockedArray->getObject(0));
		if (obj) {
			PM_ERROR("WDOG:Object %s unexpected in blocked array\n", obj->fName);
			fBlockedArray->removeObject(0);
		}
	}

	IOLockUnlock(fWatchdogLock);
}

                                                                                   
                       
                                                                                   

void
IOService::reset_watchdog_timer(IOService *blockedObject, int pendingResponseTimeout)
{
	unsigned int i;
	uint64_t    deadline;
	IOService *obj;

	if (!fWatchdogTimer || (kIOSleepWakeWdogOff & gIOKitDebug)) {
		return;
	}


	IOLockLock(fWatchdogLock);
	if (!fWatchdogDeadline) {
		goto exit;
	}

	i = fBlockedArray->getNextIndexOfObject(blockedObject, 0);
	if (pendingResponseTimeout == 0) {
		blockedObject->fPendingResponseDeadline = 0;
		if (i == (unsigned int)-1) {
			goto exit;
		}
		fBlockedArray->removeObject(i);
	} else {
		                                                                  
		                                   
		clock_interval_to_deadline(pendingResponseTimeout + 2, kSecondScale, &deadline);

		if (i != (unsigned int)-1) {
			PM_ERROR("WDOG:Object %s is already blocked for responses. Ignoring timeout %d\n",
			    fName, pendingResponseTimeout);
			goto exit;
		}

		for (i = 0; i < fBlockedArray->getCount(); i++) {
			obj = OSDynamicCast(IOService, fBlockedArray->getObject(i));
			if (obj && (obj->fPendingResponseDeadline < deadline)) {
				blockedObject->fPendingResponseDeadline = deadline;
				fBlockedArray->setObject(i, blockedObject);
				break;
			}
		}
		if (i == fBlockedArray->getCount()) {
			blockedObject->fPendingResponseDeadline = deadline;
			fBlockedArray->setObject(blockedObject);
		}
	}

	obj = OSDynamicCast(IOService, fBlockedArray->getObject(0));
	if (!obj) {
		int timeout = getPMRootDomain()->getWatchdogTimeout();
		clock_interval_to_deadline(timeout, kSecondScale, &deadline);
	} else {
		deadline = obj->fPendingResponseDeadline;
	}

	thread_call_cancel(fWatchdogTimer);
	start_watchdog_timer(deadline);

exit:
	IOLockUnlock(fWatchdogLock);
}


                                                                                   
                                  
  
                              
                                                                                   

void
IOService::watchdog_timer_expired( thread_call_param_t arg0, thread_call_param_t arg1 )
{
	IOService * me = (IOService *) arg0;


	gIOPMWatchDogThread = current_thread();
	getPMRootDomain()->sleepWakeDebugTrig(true);
	gIOPMWatchDogThread = NULL;
	thread_call_free(me->fWatchdogTimer);
	me->fWatchdogTimer = NULL;

	return;
}


IOWorkLoop *
IOService::getIOPMWorkloop( void )
{
	return gIOPMWorkLoop;
}



                                                                                   
                            
                                                                                   

void
IOService::start_ack_timer( void )
{
	start_ack_timer( ACK_TIMER_PERIOD, kNanosecondScale );
}

void
IOService::start_ack_timer( UInt32 interval, UInt32 scale )
{
	AbsoluteTime    deadline;
	boolean_t       pending;

	clock_interval_to_deadline(interval, scale, &deadline);

	retain();
	pending = thread_call_enter_delayed(fAckTimer, deadline);
	if (pending) {
		release();
	}
}

                                                                                   
                           
                                                                                   

void
IOService::stop_ack_timer( void )
{
	boolean_t   pending;

	pending = thread_call_cancel(fAckTimer);
	if (pending) {
		release();
	}
}

                                                                                   
                                 
  
                              
                                                                                   

IOReturn
IOService::actionAckTimerExpired(
	OSObject * target,
	void * arg0, void * arg1,
	void * arg2, void * arg3 )
{
	IOService * me = (IOService *) target;
	bool        done;

	                                                                  
	                                             

	done = me->ackTimerTick();
	if (done && gIOPMWorkQueue) {
		gIOPMWorkQueue->signalWorkAvailable();
	}

	return kIOReturnSuccess;
}

                                                                                   
                    
  
                                                                       
                                                                                   

void
IOService::ack_timer_expired( thread_call_param_t arg0, thread_call_param_t arg1 )
{
	IOService * me = (IOService *) arg0;

	if (gIOPMWorkLoop) {
		gIOPMWorkLoop->runAction(&actionAckTimerExpired, me);
	}
	me->release();
}


          
                         

                                                                                   
                                       
                                                                                   

void
IOService::tellSystemCapabilityChange( uint32_t nextMS )
{
	assert(IS_ROOT_DOMAIN);

	MS_PUSH( nextMS );
	fMachineState       = kIOPM_TellCapabilityChangeDone;
	fOutOfBandMessage   = kIOMessageSystemCapabilityChange;

	if (fIsPreChange) {
		                                  
		fOutOfBandParameter = kNotifyCapabilityChangeApps;
	} else {
		                                              
		fOutOfBandParameter = kNotifyCapabilityChangePriority;
	}

	tellClientsWithResponse( fOutOfBandMessage );
}

                                                                                   
                         
  
                                                                             
               
  
                                                                             
                                
  
                                                            
                                                                                   

bool
IOService::askChangeDown( unsigned long stateNum )
{
	return tellClientsWithResponse( kIOMessageCanDevicePowerOff );
}

                                                                                   
                            
  
                                                                           
                  
  
                                                            
                                                                                   

bool
IOService::tellChangeDown1( unsigned long stateNum )
{
	fOutOfBandParameter = kNotifyApps;
	return tellChangeDown(stateNum);
}

                                                                                   
                            
  
                                                                 
  
                                                            
                                                                                   

bool
IOService::tellChangeDown2( unsigned long stateNum )
{
	fOutOfBandParameter = kNotifyPriority;
	return tellChangeDown(stateNum);
}

                                                                                   
                          
  
                                                                           
                  
  
                                                                             
                                
  
                                                            
                                                                                   

bool
IOService::tellChangeDown( unsigned long stateNum )
{
	return tellClientsWithResponse( kIOMessageDeviceWillPowerOff );
}

                                                                                   
                       
  
                                                                                   

static void
logAppTimeouts( OSObject * object, void * arg )
{
	IOPMInterestContext *   context = (IOPMInterestContext *) arg;
	OSObject *              flag;
	unsigned int            clientIndex;
	int                     pid = 0;
	char                    name[128];

	if (OSDynamicCast(_IOServiceInterestNotifier, object)) {
		                                                                
		                                                                
		                                                            

		clientIndex = context->notifyClients->getNextIndexOfObject(object, 0);

		if ((clientIndex != (unsigned int) -1) &&
		    (flag = context->responseArray->getObject(clientIndex)) &&
		    (flag != kOSBooleanTrue)) {
			OSNumber *clientID = copyClientIDForNotification(object, context);

			name[0] = '\0';
			if (clientID) {
				pid = clientID->unsigned32BitValue();
				proc_name(pid, name, sizeof(name));
				clientID->release();
			}

			PM_ERROR(context->errorLog, pid, name);

			                                        
			IOService::getPMRootDomain()->pmStatsRecordApplicationResponse(
				gIOPMStatsResponseTimedOut,
				name, 0, (30 * 1000), pid, object);
		}
	}
}

void
IOService::cleanClientResponses( bool logErrors )
{
	if (logErrors && fResponseArray) {
		switch (fOutOfBandParameter) {
		case kNotifyApps:
		case kNotifyCapabilityChangeApps:
			if (fNotifyClientArray) {
				IOPMInterestContext context;

				context.responseArray    = fResponseArray;
				context.notifyClients    = fNotifyClientArray;
				context.serialNumber     = fSerialNumber;
				context.messageType      = kIOMessageCopyClientID;
				context.notifyType       = kNotifyApps;
				context.isPreChange      = fIsPreChange;
				context.enableTracing    = false;
				context.us               = this;
				context.maxTimeRequested = 0;
				context.stateNumber      = fHeadNotePowerState;
				context.stateFlags       = fHeadNotePowerArrayEntry->capabilityFlags;
				context.changeFlags      = fHeadNoteChangeFlags;
				context.errorLog         = "PM notification timeout (pid %d, %s)\n";

				applyToInterested(gIOAppPowerStateInterest, logAppTimeouts, (void *) &context);
			}
			break;

		default:
			                                                   
			                                                        
			PM_ERROR("PM priority notification timeout\n");
			if (gIOKitDebug & kIOLogDebugPower) {
				panic("PM priority notification timeout");
			}
			break;
		}
	}

	if (IS_ROOT_DOMAIN) {
		getPMRootDomain()->reset_watchdog_timer(this, 0);
	}
	if (fResponseArray) {
		fResponseArray->release();
		fResponseArray = NULL;
	}
	if (fNotifyClientArray) {
		fNotifyClientArray->release();
		fNotifyClientArray = NULL;
	}
}

                                                                                   
                                      
  
                                                                           
                  
  
                                                            
                                                                                   

bool
IOService::tellClientsWithResponse( int messageType )
{
	IOPMInterestContext     context;
	bool                    isRootDomain = IS_ROOT_DOMAIN;
	uint32_t                maxTimeOut = kMaxTimeRequested;

	PM_ASSERT_IN_GATE();
	assert( fResponseArray == NULL );
	assert( fNotifyClientArray == NULL );

	RD_LOG("tellClientsWithResponse( %s, %s )\n", getIOMessageString(messageType),
	    getNotificationPhaseString(fOutOfBandParameter));

	fResponseArray = OSArray::withCapacity( 1 );
	if (!fResponseArray) {
		goto exit;
	}

	fResponseArray->setCapacityIncrement(8);
	if (++fSerialNumber == 0) {
		fSerialNumber++;
	}

	context.responseArray    = fResponseArray;
	context.notifyClients    = NULL;
	context.serialNumber     = fSerialNumber;
	context.messageType      = messageType;
	context.notifyType       = fOutOfBandParameter;
	context.skippedInDark    = 0;
	context.notSkippedInDark = 0;
	context.isPreChange      = fIsPreChange;
	context.enableTracing    = false;
	context.us               = this;
	context.maxTimeRequested = 0;
	context.stateNumber      = fHeadNotePowerState;
	context.stateFlags       = fHeadNotePowerArrayEntry->capabilityFlags;
	context.changeFlags      = fHeadNoteChangeFlags;
	context.messageFilter    = (isRootDomain) ?
	    OSMemberFunctionCast(
		IOPMMessageFilter,
		(IOPMrootDomain *)this,
		&IOPMrootDomain::systemMessageFilter) : NULL;

	switch (fOutOfBandParameter) {
	case kNotifyApps:
		applyToInterested( gIOAppPowerStateInterest,
		    pmTellAppWithResponse, (void *) &context );

		if (isRootDomain &&
		    (fMachineState != kIOPM_OurChangeTellClientsPowerDown) &&
		    (fMachineState != kIOPM_SyncTellClientsPowerDown) &&
		    (context.messageType != kIOPMMessageLastCallBeforeSleep)) {
			                                              
			                               
			context.notifyType  = kNotifyCapabilityChangeApps;
			context.messageType = kIOMessageSystemCapabilityChange;
			applyToInterested( gIOAppPowerStateInterest,
			    pmTellCapabilityAppWithResponse, (void *) &context );
			context.notifyType  = fOutOfBandParameter;
			context.messageType = messageType;
		}
		if (context.messageType == kIOMessageCanSystemSleep) {
			maxTimeOut = kCanSleepMaxTimeReq;
			if (gCanSleepTimeout) {
				maxTimeOut = (gCanSleepTimeout * us_per_s);
			}
		}
		context.maxTimeRequested = maxTimeOut;
		context.enableTracing = isRootDomain;
		applyToInterested( gIOGeneralInterest,
		    pmTellClientWithResponse, (void *) &context );

		break;

	case kNotifyPriority:
		context.enableTracing = isRootDomain;
		applyToInterested( gIOPriorityPowerStateInterest,
		    pmTellClientWithResponse, (void *) &context );

		if (isRootDomain) {
			                                                   
			context.notifyType  = kNotifyCapabilityChangePriority;
			context.messageType = kIOMessageSystemCapabilityChange;
			applyToInterested( gIOPriorityPowerStateInterest,
			    pmTellCapabilityClientWithResponse, (void *) &context );
		}
		break;

	case kNotifyCapabilityChangeApps:
		context.enableTracing = isRootDomain;
		applyToInterested( gIOAppPowerStateInterest,
		    pmTellCapabilityAppWithResponse, (void *) &context );
		if (context.messageType == kIOMessageCanSystemSleep) {
			maxTimeOut = kCanSleepMaxTimeReq;
			if (gCanSleepTimeout) {
				maxTimeOut = (gCanSleepTimeout * us_per_s);
			}
		}
		context.maxTimeRequested = maxTimeOut;
		break;

	case kNotifyCapabilityChangePriority:
		context.enableTracing = isRootDomain;
		applyToInterested( gIOPriorityPowerStateInterest,
		    pmTellCapabilityClientWithResponse, (void *) &context );
		break;
	}
	fNotifyClientArray = context.notifyClients;

	if (context.skippedInDark) {
		IOLog("tellClientsWithResponse(%s, %s) %d of %d skipped in dark\n",
		    getIOMessageString(messageType), getNotificationPhaseString(fOutOfBandParameter),
		    context.skippedInDark, context.skippedInDark + context.notSkippedInDark);
	}

	                                   
	if (!checkForDone()) {
		OUR_PMLog(kPMLogStartAckTimer, context.maxTimeRequested, 0);
		if (context.enableTracing) {
			getPMRootDomain()->traceDetail(context.messageType, 0, context.maxTimeRequested / 1000);
			getPMRootDomain()->reset_watchdog_timer(this, context.maxTimeRequested / USEC_PER_SEC + 1);
		}
		start_ack_timer( context.maxTimeRequested / 1000, kMillisecondScale );
		return false;
	}

exit:
	                      
	if (fResponseArray) {
		fResponseArray->release();
		fResponseArray = NULL;
	}
	if (fNotifyClientArray) {
		fNotifyClientArray->release();
		fNotifyClientArray = NULL;
	}

	return true;
}

                                                                                   
                                         
  
                                                                                 
                                            
                                                                                   

void
IOService::pmTellAppWithResponse( OSObject * object, void * arg )
{
	IOPMInterestContext *   context = (IOPMInterestContext *) arg;
	IOServicePM *           pwrMgt = context->us->pwrMgt;
	uint32_t                msgIndex, msgRef, msgType;
	OSNumber                *clientID = NULL;
	proc_t                  proc = NULL;
	boolean_t               proc_suspended = FALSE;
	OSObject *              waitForReply = kOSBooleanTrue;
#if LOG_APP_RESPONSE_TIMES
	AbsoluteTime            now;
#endif

	if (!OSDynamicCast(_IOServiceInterestNotifier, object)) {
		return;
	}

	if (context->us == getPMRootDomain()) {
		if ((clientID = copyClientIDForNotification(object, context))) {
			uint32_t clientPID = clientID->unsigned32BitValue();
			clientID->release();
			proc = proc_find(clientPID);

			if (proc) {
				proc_suspended = get_task_pidsuspended((task_t) proc->task);
				if (proc_suspended) {
					logClientIDForNotification(object, context, "PMTellAppWithResponse - Suspended");
				} else if (getPMRootDomain()->isAOTMode() && get_task_suspended((task_t) proc->task)) {
					proc_suspended = true;
					context->skippedInDark++;
				}
				proc_rele(proc);
				if (proc_suspended) {
					return;
				}
			}
		}
	}

	if (context->messageFilter &&
	    !context->messageFilter(context->us, object, context, NULL, &waitForReply)) {
		if (kIOLogDebugPower & gIOKitDebug) {
			logClientIDForNotification(object, context, "DROP App");
		}
		return;
	}
	context->notSkippedInDark++;

	                                                                  
	                                                  
	if (NULL == context->notifyClients) {
		context->notifyClients = OSArray::withCapacity( 32 );
	}

	msgType  = context->messageType;
	msgIndex = context->responseArray->getCount();
	msgRef   = ((context->serialNumber & 0xFFFF) << 16) + (msgIndex & 0xFFFF);

	OUR_PMLog(kPMLogAppNotify, msgType, msgRef);
	if (kIOLogDebugPower & gIOKitDebug) {
		logClientIDForNotification(object, context, "MESG App");
	}

	if (waitForReply == kOSBooleanTrue) {
		OSNumber * num;
		clock_get_uptime(&now);
		num = OSNumber::withNumber(AbsoluteTime_to_scalar(&now), sizeof(uint64_t) * 8);
		if (num) {
			context->responseArray->setObject(msgIndex, num);
			num->release();
		} else {
			context->responseArray->setObject(msgIndex, kOSBooleanFalse);
		}
	} else {
		context->responseArray->setObject(msgIndex, kOSBooleanTrue);
		if (kIOLogDebugPower & gIOKitDebug) {
			logClientIDForNotification(object, context, "App response ignored");
		}
	}

	if (context->notifyClients) {
		context->notifyClients->setObject(msgIndex, object);
	}

	context->us->messageClient(msgType, object, (void *)(uintptr_t) msgRef);
}

                                                                                   
                                            
  
                                                                      
                                                            
                                                                                   

void
IOService::pmTellClientWithResponse( OSObject * object, void * arg )
{
	IOPowerStateChangeNotification  notify;
	IOPMInterestContext *           context = (IOPMInterestContext *) arg;
	OSObject *                      replied = kOSBooleanTrue;
	_IOServiceInterestNotifier *    notifier;
	uint32_t                        msgIndex, msgRef, msgType;
	IOReturn                        retCode;
	AbsoluteTime                    start, end;
	uint64_t                        nsec;
	bool                            enableTracing;

	if (context->messageFilter &&
	    !context->messageFilter(context->us, object, context, NULL, NULL)) {
		getPMRootDomain()->traceFilteredNotification(object);
		return;
	}

	                                                                               
	                                                                             
	                                                                            
	                                                           
	notifier = OSDynamicCast(_IOServiceInterestNotifier, object);
	msgType  = context->messageType;
	msgIndex = context->responseArray->getCount();
	msgRef   = ((context->serialNumber & 0xFFFF) << 16) + (msgIndex & 0xFFFF);
	enableTracing = context->enableTracing && (notifier != NULL);

	IOServicePM * pwrMgt = context->us->pwrMgt;
	if (gIOKitDebug & kIOLogPower) {
		OUR_PMLog(kPMLogClientNotify, msgRef, msgType);
		if (OSDynamicCast(IOService, object)) {
			const char *who = ((IOService *) object)->getName();
			gPlatform->PMLog(who, kPMLogClientNotify, (uintptr_t) object, 0);
		} else if (notifier) {
			OUR_PMLog(kPMLogClientNotify, (uintptr_t) notifier->handler, 0);
		}
	}

	if (NULL == context->notifyClients) {
		context->notifyClients = OSArray::withCapacity(32);
		assert(context->notifyClients != NULL);
	}

	notify.powerRef    = (void *)(uintptr_t) msgRef;
	notify.returnValue = 0;
	notify.stateNumber = context->stateNumber;
	notify.stateFlags  = context->stateFlags;

	clock_get_uptime(&start);
	if (enableTracing) {
		getPMRootDomain()->traceNotification(notifier, true, start, msgIndex);
	}

	retCode = context->us->messageClient(msgType, object, (void *) &notify, sizeof(notify));

	clock_get_uptime(&end);
	if (enableTracing) {
		getPMRootDomain()->traceNotification(notifier, false, end);
	}

	if (kIOReturnSuccess == retCode) {
		if (0 == notify.returnValue) {
			OUR_PMLog(kPMLogClientAcknowledge, msgRef, (uintptr_t) object);
			context->responseArray->setObject(msgIndex, replied);
		} else {
			replied = kOSBooleanFalse;
			if (notify.returnValue > context->maxTimeRequested) {
				if (notify.returnValue > kPriorityClientMaxWait) {
					context->maxTimeRequested = kPriorityClientMaxWait;
					PM_ERROR("%s: client %p returned %llu for %s\n",
					    context->us->getName(),
					    notifier ? (void *)  OBFUSCATE(notifier->handler) : OBFUSCATE(object),
					    (uint64_t) notify.returnValue,
					    getIOMessageString(msgType));
				} else {
					context->maxTimeRequested = (typeof(context->maxTimeRequested))notify.returnValue;
				}
			}
			  
			                                                       
			                      
			OSNumber * num;
			num = OSNumber::withNumber(AbsoluteTime_to_scalar(&end), sizeof(uint64_t) * 8);
			if (num) {
				context->responseArray->setObject(msgIndex, num);
				num->release();
			} else {
				context->responseArray->setObject(msgIndex, replied);
			}
		}

		if (enableTracing) {
			SUB_ABSOLUTETIME(&end, &start);
			absolutetime_to_nanoseconds(end, &nsec);

			if ((nsec > LOG_KEXT_RESPONSE_TIMES) || (notify.returnValue != 0)) {
				getPMRootDomain()->traceNotificationResponse(notifier, NS_TO_MS(nsec), (uint32_t) notify.returnValue);
			}
		}
	} else {
		                       
		                                      
		OUR_PMLog(kPMLogClientAcknowledge, msgRef, 0);
		context->responseArray->setObject(msgIndex, replied);
	}
	if (context->notifyClients) {
		context->notifyClients->setObject(msgIndex, object);
	}
}

                                                                                   
                                                   
                                                                                   

void
IOService::pmTellCapabilityAppWithResponse( OSObject * object, void * arg )
{
	IOPMSystemCapabilityChangeParameters msgArg;
	IOPMInterestContext *       context = (IOPMInterestContext *) arg;
	OSObject *                  replied = kOSBooleanTrue;
	IOServicePM *               pwrMgt = context->us->pwrMgt;
	uint32_t                    msgIndex, msgRef, msgType;
#if LOG_APP_RESPONSE_TIMES
	AbsoluteTime                now;
#endif

	if (!OSDynamicCast(_IOServiceInterestNotifier, object)) {
		return;
	}

	memset(&msgArg, 0, sizeof(msgArg));
	if (context->messageFilter &&
	    !context->messageFilter(context->us, object, context, &msgArg, &replied)) {
		return;
	}

	if (context->us == getPMRootDomain() &&
	    getPMRootDomain()->isAOTMode()
	    ) {
		OSNumber                *clientID = NULL;
		boolean_t               proc_suspended = FALSE;
		proc_t                proc = NULL;
		if ((clientID = copyClientIDForNotification(object, context))) {
			uint32_t clientPID = clientID->unsigned32BitValue();
			clientID->release();
			proc = proc_find(clientPID);
			if (proc) {
				proc_suspended = get_task_pidsuspended((task_t) proc->task);
				if (proc_suspended) {
					logClientIDForNotification(object, context, "PMTellCapablityAppWithResponse - Suspended");
				} else if (get_task_suspended((task_t) proc->task)) {
					proc_suspended = true;
					context->skippedInDark++;
				}
				proc_rele(proc);
				if (proc_suspended) {
					return;
				}
			}
		}
	}
	context->notSkippedInDark++;

	                                                                  
	                                                  
	if (NULL == context->notifyClients) {
		context->notifyClients = OSArray::withCapacity(32);
		assert(context->notifyClients != NULL);
	}

	msgType  = context->messageType;
	msgIndex = context->responseArray->getCount();
	msgRef   = ((context->serialNumber & 0xFFFF) << 16) + (msgIndex & 0xFFFF);

	OUR_PMLog(kPMLogAppNotify, msgType, msgRef);
	if (kIOLogDebugPower & gIOKitDebug) {
		                                              
		OSNumber * clientID = NULL;
		OSString * clientIDString = NULL;;
		context->us->messageClient(kIOMessageCopyClientID, object, &clientID);
		if (clientID) {
			clientIDString = IOCopyLogNameForPID(clientID->unsigned32BitValue());
		}

		PM_LOG("%s MESG App(%u) %s, wait %u, %s\n",
		    context->us->getName(),
		    msgIndex, getIOMessageString(msgType),
		    (replied != kOSBooleanTrue),
		    clientIDString ? clientIDString->getCStringNoCopy() : "");
		if (clientID) {
			clientID->release();
		}
		if (clientIDString) {
			clientIDString->release();
		}
	}

	msgArg.notifyRef = msgRef;
	msgArg.maxWaitForReply = 0;

	if (replied == kOSBooleanTrue) {
		msgArg.notifyRef = 0;
		context->responseArray->setObject(msgIndex, kOSBooleanTrue);
		if (context->notifyClients) {
			context->notifyClients->setObject(msgIndex, kOSBooleanTrue);
		}
	} else {
		OSNumber * num;
		clock_get_uptime(&now);
		num = OSNumber::withNumber(AbsoluteTime_to_scalar(&now), sizeof(uint64_t) * 8);
		if (num) {
			context->responseArray->setObject(msgIndex, num);
			num->release();
		} else {
			context->responseArray->setObject(msgIndex, kOSBooleanFalse);
		}

		if (context->notifyClients) {
			context->notifyClients->setObject(msgIndex, object);
		}
	}

	context->us->messageClient(msgType, object, (void *) &msgArg, sizeof(msgArg));
}

                                                                                   
                                                      
                                                                                   

void
IOService::pmTellCapabilityClientWithResponse(
	OSObject * object, void * arg )
{
	IOPMSystemCapabilityChangeParameters msgArg;
	IOPMInterestContext *           context = (IOPMInterestContext *) arg;
	OSObject *                      replied = kOSBooleanTrue;
	_IOServiceInterestNotifier *    notifier;
	uint32_t                        msgIndex, msgRef, msgType;
	IOReturn                        retCode;
	AbsoluteTime                    start, end;
	uint64_t                        nsec;
	bool                            enableTracing;

	memset(&msgArg, 0, sizeof(msgArg));
	if (context->messageFilter &&
	    !context->messageFilter(context->us, object, context, &msgArg, NULL)) {
		getPMRootDomain()->traceFilteredNotification(object);
		return;
	}

	if (NULL == context->notifyClients) {
		context->notifyClients = OSArray::withCapacity(32);
		assert(context->notifyClients != NULL);
	}

	notifier = OSDynamicCast(_IOServiceInterestNotifier, object);
	msgType  = context->messageType;
	msgIndex = context->responseArray->getCount();
	msgRef   = ((context->serialNumber & 0xFFFF) << 16) + (msgIndex & 0xFFFF);
	enableTracing = context->enableTracing && (notifier != NULL);

	IOServicePM * pwrMgt = context->us->pwrMgt;
	if (gIOKitDebug & kIOLogPower) {
		OUR_PMLog(kPMLogClientNotify, msgRef, msgType);
		if (OSDynamicCast(IOService, object)) {
			const char *who = ((IOService *) object)->getName();
			gPlatform->PMLog(who, kPMLogClientNotify, (uintptr_t) object, 0);
		} else if (notifier) {
			OUR_PMLog(kPMLogClientNotify, (uintptr_t) notifier->handler, 0);
		}
	}

	msgArg.notifyRef = msgRef;
	msgArg.maxWaitForReply = 0;

	clock_get_uptime(&start);
	if (enableTracing) {
		getPMRootDomain()->traceNotification(notifier, true, start, msgIndex);
	}

	retCode = context->us->messageClient(msgType, object, (void *) &msgArg, sizeof(msgArg));

	clock_get_uptime(&end);
	if (enableTracing) {
		getPMRootDomain()->traceNotification(notifier, false, end, msgIndex);
	}

	if (kIOReturnSuccess == retCode) {
		if (0 == msgArg.maxWaitForReply) {
			                                      
			OUR_PMLog(kPMLogClientAcknowledge, msgRef, (uintptr_t) object);
			context->responseArray->setObject(msgIndex, replied);
		} else {
			replied = kOSBooleanFalse;
			if (msgArg.maxWaitForReply > context->maxTimeRequested) {
				if (msgArg.maxWaitForReply > kCapabilityClientMaxWait) {
					context->maxTimeRequested = kCapabilityClientMaxWait;
					PM_ERROR("%s: client %p returned %u for %s\n",
					    context->us->getName(),
					    notifier ? (void *) OBFUSCATE(notifier->handler) : OBFUSCATE(object),
					    msgArg.maxWaitForReply,
					    getIOMessageString(msgType));
				} else {
					context->maxTimeRequested = msgArg.maxWaitForReply;
				}
			}

			                                                       
			                      
			OSNumber * num;
			num = OSNumber::withNumber(AbsoluteTime_to_scalar(&end), sizeof(uint64_t) * 8);
			if (num) {
				context->responseArray->setObject(msgIndex, num);
				num->release();
			} else {
				context->responseArray->setObject(msgIndex, replied);
			}
		}

		if (enableTracing) {
			SUB_ABSOLUTETIME(&end, &start);
			absolutetime_to_nanoseconds(end, &nsec);

			if ((nsec > LOG_KEXT_RESPONSE_TIMES) || (msgArg.maxWaitForReply != 0)) {
				getPMRootDomain()->traceNotificationResponse(notifier, NS_TO_MS(nsec), msgArg.maxWaitForReply);
			}
		}
	} else {
		                       
		                                      
		OUR_PMLog(kPMLogClientAcknowledge, msgRef, 0);
		context->responseArray->setObject(msgIndex, replied);
	}
	if (context->notifyClients) {
		context->notifyClients->setObject(msgIndex, object);
	}
}

                                                                                   
                            
  
                                                                    
                  
  
                                                                             
                                        
                                                                                   

void
IOService::tellNoChangeDown( unsigned long )
{
	return tellClients( kIOMessageDeviceWillNotPowerOff );
}

                                                                                   
                        
  
                                                                               
  
                                                                             
                                        
                                                                                   

void
IOService::tellChangeUp( unsigned long )
{
	return tellClients( kIOMessageDeviceHasPoweredOn );
}

                                                                                   
                          
  
                                                                  
                                                                                   

void
IOService::tellClients( int messageType )
{
	IOPMInterestContext     context;

	RD_LOG("tellClients( %s )\n", getIOMessageString(messageType));

	memset(&context, 0, sizeof(context));
	context.messageType   = messageType;
	context.isPreChange   = fIsPreChange;
	context.us            = this;
	context.stateNumber   = fHeadNotePowerState;
	context.stateFlags    = fHeadNotePowerArrayEntry->capabilityFlags;
	context.changeFlags   = fHeadNoteChangeFlags;
	context.enableTracing = IS_ROOT_DOMAIN;
	context.messageFilter = (IS_ROOT_DOMAIN) ?
	    OSMemberFunctionCast(
		IOPMMessageFilter,
		(IOPMrootDomain *)this,
		&IOPMrootDomain::systemMessageFilter) : NULL;

	context.notifyType = kNotifyPriority;
	applyToInterested( gIOPriorityPowerStateInterest,
	    tellKernelClientApplier, (void *) &context );

	context.notifyType = kNotifyApps;
	applyToInterested( gIOAppPowerStateInterest,
	    tellAppClientApplier, (void *) &context );

	applyToInterested( gIOGeneralInterest,
	    tellKernelClientApplier, (void *) &context );
}

                                                                                   
                                    
  
                           
                                                                                   

static void
tellKernelClientApplier( OSObject * object, void * arg )
{
	IOPowerStateChangeNotification  notify;
	IOPMInterestContext *           context = (IOPMInterestContext *) arg;
	bool                            enableTracing = context->enableTracing;

	if (context->messageFilter &&
	    !context->messageFilter(context->us, object, context, NULL, NULL)) {
		IOService::getPMRootDomain()->traceFilteredNotification(object);
		return;
	}

	notify.powerRef     = (void *) NULL;
	notify.returnValue  = 0;
	notify.stateNumber  = context->stateNumber;
	notify.stateFlags   = context->stateFlags;

	if (enableTracing) {
		IOService::getPMRootDomain()->traceNotification(object, true);
	}

	context->us->messageClient(context->messageType, object, &notify, sizeof(notify));

	if (enableTracing) {
		IOService::getPMRootDomain()->traceNotification(object, false);
	}
}

static OSNumber *
copyClientIDForNotification(
	OSObject *object,
	IOPMInterestContext *context)
{
	OSNumber *clientID = NULL;
	context->us->messageClient(kIOMessageCopyClientID, object, &clientID);
	return clientID;
}

static void
logClientIDForNotification(
	OSObject *object,
	IOPMInterestContext *context,
	const char *logString)
{
	OSString *logClientID = NULL;
	OSNumber *clientID = copyClientIDForNotification(object, context);

	if (logString) {
		if (clientID) {
			logClientID = IOCopyLogNameForPID(clientID->unsigned32BitValue());
		}

		PM_LOG("%s %s %s, %s\n",
		    context->us->getName(), logString,
		    IOService::getIOMessageString(context->messageType),
		    logClientID ? logClientID->getCStringNoCopy() : "");

		if (logClientID) {
			logClientID->release();
		}
	}

	if (clientID) {
		clientID->release();
	}

	return;
}

static void
tellAppClientApplier( OSObject * object, void * arg )
{
	IOPMInterestContext * context = (IOPMInterestContext *) arg;
	OSNumber            * clientID = NULL;
	proc_t                proc = NULL;
	boolean_t             proc_suspended = FALSE;

	if (context->us == IOService::getPMRootDomain()) {
		if ((clientID = copyClientIDForNotification(object, context))) {
			uint32_t clientPID = clientID->unsigned32BitValue();
			clientID->release();
			proc = proc_find(clientPID);

			if (proc) {
				proc_suspended = get_task_pidsuspended((task_t) proc->task);
				if (proc_suspended) {
					logClientIDForNotification(object, context, "tellAppClientApplier - Suspended");
				} else if (IOService::getPMRootDomain()->isAOTMode() && get_task_suspended((task_t) proc->task)) {
					proc_suspended = true;
					context->skippedInDark++;
				}
				proc_rele(proc);
				if (proc_suspended) {
					return;
				}
			}
		}
	}

	if (context->messageFilter &&
	    !context->messageFilter(context->us, object, context, NULL, NULL)) {
		if (kIOLogDebugPower & gIOKitDebug) {
			logClientIDForNotification(object, context, "DROP App");
		}
		return;
	}
	context->notSkippedInDark++;

	if (kIOLogDebugPower & gIOKitDebug) {
		logClientIDForNotification(object, context, "MESG App");
	}

	context->us->messageClient(context->messageType, object, NULL);
}

                                                                                   
                         
                                                                                   

bool
IOService::checkForDone( void )
{
	int         i = 0;
	OSObject *  theFlag;

	if (fResponseArray == NULL) {
		return true;
	}

	for (i = 0;; i++) {
		theFlag = fResponseArray->getObject(i);

		if (NULL == theFlag) {
			break;
		}

		if (kOSBooleanTrue != theFlag) {
			return false;
		}
	}
	return true;
}

                                                                                   
                         
                                                                                   

bool
IOService::responseValid( uint32_t refcon, int pid )
{
	UInt16          serialComponent;
	UInt16          ordinalComponent;
	OSObject *      theFlag;
	OSObject        *object = NULL;

	serialComponent  = (refcon >> 16) & 0xFFFF;
	ordinalComponent = (refcon & 0xFFFF);

	if (serialComponent != fSerialNumber) {
		return false;
	}

	if (fResponseArray == NULL) {
		return false;
	}

	theFlag = fResponseArray->getObject(ordinalComponent);

	if (theFlag == NULL) {
		return false;
	}

	if (fNotifyClientArray) {
		object = fNotifyClientArray->getObject(ordinalComponent);
	}

	OSNumber * num;
	if ((num = OSDynamicCast(OSNumber, theFlag))) {
		AbsoluteTime    now;
		AbsoluteTime    start;
		uint64_t        nsec;
		char            name[128];

		clock_get_uptime(&now);
		AbsoluteTime_to_scalar(&start) = num->unsigned64BitValue();
		SUB_ABSOLUTETIME(&now, &start);
		absolutetime_to_nanoseconds(now, &nsec);

		if (pid != 0) {
			name[0] = '\0';
			proc_name(pid, name, sizeof(name));

			if (nsec > LOG_APP_RESPONSE_TIMES) {
				IOLog("PM response took %d ms (%d, %s)\n", NS_TO_MS(nsec),
				    pid, name);
			}


			if (nsec > LOG_APP_RESPONSE_MSG_TRACER) {
				                                          
				getPMRootDomain()->pmStatsRecordApplicationResponse(
					gIOPMStatsResponseSlow,
					name, 0, NS_TO_MS(nsec), pid, object);
			} else {
				getPMRootDomain()->pmStatsRecordApplicationResponse(
					gIOPMStatsResponsePrompt,
					name, 0, NS_TO_MS(nsec), pid, object);
			}
		} else {
			getPMRootDomain()->traceNotificationAck(object, NS_TO_MS(nsec));
		}

		if (kIOLogDebugPower & gIOKitDebug) {
			PM_LOG("Ack(%u) %u ms\n",
			    (uint32_t) ordinalComponent,
			    NS_TO_MS(nsec));
		}
		theFlag = kOSBooleanFalse;
	} else if (object) {
		getPMRootDomain()->pmStatsRecordApplicationResponse(
			gIOPMStatsResponsePrompt,
			NULL, 0, 0, pid, object);
	}

	if (kOSBooleanFalse == theFlag) {
		fResponseArray->replaceObject(ordinalComponent, kOSBooleanTrue);
	}

	return true;
}

                                                                                   
                            
  
                                                                       
                                                                                   
                                                                                
                                                                                   

IOReturn
IOService::allowPowerChange( unsigned long refcon )
{
	IOPMRequest * request;

	if (!initialized) {
		                  
		return kIOReturnSuccess;
	}

	request = acquirePMRequest( this, kIOPMRequestTypeAllowPowerChange );
	if (!request) {
		return kIOReturnNoMemory;
	}

	request->fArg0 = (void *)            refcon;
	request->fArg1 = (void *)(uintptr_t) proc_selfpid();
	request->fArg2 = (void *)            NULL;
	submitPMRequest( request );

	return kIOReturnSuccess;
}

#ifndef __LP64__
IOReturn
IOService::serializedAllowPowerChange2( unsigned long refcon )
{
	                      
	return kIOReturnUnsupported;
}
#endif                

                                                                                   
                             
  
                                                                       
                                                                                  
                                                  
                                                                                   

IOReturn
IOService::cancelPowerChange( unsigned long refcon )
{
	IOPMRequest *   request;
	char            name[128];
	pid_t           pid = proc_selfpid();

	if (!initialized) {
		                  
		return kIOReturnSuccess;
	}

	name[0] = '\0';
	proc_name(pid, name, sizeof(name));
	PM_ERROR("PM notification cancel (pid %d, %s)\n", pid, name);

	request = acquirePMRequest( this, kIOPMRequestTypeCancelPowerChange );
	if (!request) {
		return kIOReturnNoMemory;
	}

	request->fArg0 = (void *)            refcon;
	request->fArg1 = (void *)(uintptr_t) proc_selfpid();
	request->fArg2 = (void *)            OSString::withCString(name);
	submitPMRequest( request );

	return kIOReturnSuccess;
}

                                                                                   
                      
  
                                                      
                                                                                   

void
IOService::cancelIdlePowerDown( IOService * service )
{
	IOPMRequest * request;

	request = acquirePMRequest(service, kIOPMRequestTypeIdleCancel);
	if (request) {
		submitPMRequest(request);
	}
}

#ifndef __LP64__
IOReturn
IOService::serializedCancelPowerChange2( unsigned long refcon )
{
	                      
	return kIOReturnUnsupported;
}

                                                                                   
                         
  
                                                          
                                                                                   

void
IOService::PM_Clamp_Timer_Expired( void )
{
}

                                                                                   
               
  
                                                                              
                                                                                   

void
IOService::clampPowerOn( unsigned long duration )
{
}
#endif                

                                                                                   
                             
  
                                                         
                                                                                   
IOReturn
IOService::configurePowerStatesReport( IOReportConfigureAction action, void *result )
{
	IOReturn rc = kIOReturnSuccess;
	size_t  reportSize;
	unsigned long i;
	uint64_t ts;

	if (!pwrMgt) {
		return kIOReturnUnsupported;
	}

	if (!fNumberOfPowerStates) {
		return kIOReturnSuccess;                                                                                  
	}

	if (fNumberOfPowerStates > INT16_MAX) {
		return kIOReturnOverrun;
	}
	PM_LOCK();

	switch (action) {
	case kIOReportEnable:
		if (fReportBuf) {
			fReportClientCnt++;
			break;
		}
		reportSize = STATEREPORT_BUFSIZE(fNumberOfPowerStates);
		fReportBuf = IOMalloc(reportSize);
		if (!fReportBuf) {
			rc = kIOReturnNoMemory;
			break;
		}
		memset(fReportBuf, 0, reportSize);

		STATEREPORT_INIT((uint16_t) fNumberOfPowerStates, fReportBuf, reportSize,
		    getRegistryEntryID(), kPMPowerStatesChID, kIOReportCategoryPower);

		for (i = 0; i < fNumberOfPowerStates; i++) {
			unsigned bits = 0;

			if (fPowerStates[i].capabilityFlags & kIOPMPowerOn) {
				bits |= kPMReportPowerOn;
			}
			if (fPowerStates[i].capabilityFlags & kIOPMDeviceUsable) {
				bits |= kPMReportDeviceUsable;
			}
			if (fPowerStates[i].capabilityFlags & kIOPMLowPower) {
				bits |= kPMReportLowPower;
			}

			STATEREPORT_SETSTATEID(fReportBuf, i, ((bits & 0xff) << 8) |
			    ((StateOrder(fMaxPowerState) & 0xf) << 4) | (StateOrder(i) & 0xf));
		}
		ts = mach_absolute_time();
		STATEREPORT_SETSTATE(fReportBuf, (uint16_t) fCurrentPowerState, ts);
		break;

	case kIOReportDisable:
		if (fReportClientCnt == 0) {
			rc = kIOReturnBadArgument;
			break;
		}
		if (fReportClientCnt == 1) {
			IOFree(fReportBuf, STATEREPORT_BUFSIZE(fNumberOfPowerStates));
			fReportBuf = NULL;
		}
		fReportClientCnt--;
		break;

	case kIOReportGetDimensions:
		if (fReportBuf) {
			STATEREPORT_UPDATERES(fReportBuf, kIOReportGetDimensions, result);
		}
		break;
	}

	PM_UNLOCK();

	return rc;
}

                                                                                   
                          
  
                                                      
                                                                                   
IOReturn
IOService::updatePowerStatesReport( IOReportConfigureAction action, void *result, void *destination )
{
	uint32_t size2cpy;
	void *data2cpy;
	uint64_t ts;
	IOReturn rc = kIOReturnSuccess;
	IOBufferMemoryDescriptor *dest = OSDynamicCast(IOBufferMemoryDescriptor, (OSObject *)destination);


	if (!pwrMgt) {
		return kIOReturnUnsupported;
	}
	if (!fNumberOfPowerStates) {
		return kIOReturnSuccess;
	}

	if (!result || !dest) {
		return kIOReturnBadArgument;
	}
	PM_LOCK();

	switch (action) {
	case kIOReportCopyChannelData:
		if (!fReportBuf) {
			rc = kIOReturnNotOpen;
			break;
		}

		ts = mach_absolute_time();
		STATEREPORT_UPDATEPREP(fReportBuf, ts, data2cpy, size2cpy);
		if (size2cpy > (dest->getCapacity() - dest->getLength())) {
			rc = kIOReturnOverrun;
			break;
		}

		STATEREPORT_UPDATERES(fReportBuf, kIOReportCopyChannelData, result);
		dest->appendBytes(data2cpy, size2cpy);
		break;

	default:
		break;
	}

	PM_UNLOCK();

	return rc;
}

                                                                                   
                              
  
                                                      
                                                                                   
IOReturn
IOService::configureSimplePowerReport(IOReportConfigureAction action, void *result )
{
	IOReturn rc = kIOReturnSuccess;

	if (!pwrMgt) {
		return kIOReturnUnsupported;
	}

	if (!fNumberOfPowerStates) {
		return rc;
	}

	switch (action) {
	case kIOReportEnable:
	case kIOReportDisable:
		break;

	case kIOReportGetDimensions:
		SIMPLEREPORT_UPDATERES(kIOReportGetDimensions, result);
		break;
	}


	return rc;
}

                                                                                   
                           
  
                                                      
                                                                                   
IOReturn
IOService::updateSimplePowerReport( IOReportConfigureAction action, void *result, void *destination )
{
	uint32_t size2cpy;
	void *data2cpy;
	uint64_t buf[SIMPLEREPORT_BUFSIZE / sizeof(uint64_t) + 1];                            
	IOBufferMemoryDescriptor *dest = OSDynamicCast(IOBufferMemoryDescriptor, (OSObject *)destination);
	IOReturn rc = kIOReturnSuccess;
	unsigned bits = 0;


	if (!pwrMgt) {
		return kIOReturnUnsupported;
	}
	if (!result || !dest) {
		return kIOReturnBadArgument;
	}

	if (!fNumberOfPowerStates) {
		return rc;
	}
	PM_LOCK();

	switch (action) {
	case kIOReportCopyChannelData:

		SIMPLEREPORT_INIT(buf, sizeof(buf), getRegistryEntryID(), kPMCurrStateChID, kIOReportCategoryPower);

		if (fPowerStates[fCurrentPowerState].capabilityFlags & kIOPMPowerOn) {
			bits |= kPMReportPowerOn;
		}
		if (fPowerStates[fCurrentPowerState].capabilityFlags & kIOPMDeviceUsable) {
			bits |= kPMReportDeviceUsable;
		}
		if (fPowerStates[fCurrentPowerState].capabilityFlags & kIOPMLowPower) {
			bits |= kPMReportLowPower;
		}


		SIMPLEREPORT_SETVALUE(buf, ((bits & 0xff) << 8) | ((StateOrder(fMaxPowerState) & 0xf) << 4) |
		    (StateOrder(fCurrentPowerState) & 0xf));

		SIMPLEREPORT_UPDATEPREP(buf, data2cpy, size2cpy);
		if (size2cpy > (dest->getCapacity() - dest->getLength())) {
			rc = kIOReturnOverrun;
			break;
		}

		SIMPLEREPORT_UPDATERES(kIOReportCopyChannelData, result);
		dest->appendBytes(data2cpy, size2cpy);
		break;

	default:
		break;
	}

	PM_UNLOCK();

	return rc;
}



          
                         

                                                                                   
                         
  
                                                                       
                                                                                   

IOReturn
IOService::setPowerState(
	unsigned long powerStateOrdinal, IOService * whatDevice )
{
	return IOPMNoErr;
}

                                                                                   
                                       
  
                                                                           
                                                                        
                                                    
                                                                                   

IOPMPowerStateIndex
IOService::getPowerStateForDomainFlags( IOPMPowerFlags flags )
{
	IOPMPowerStateIndex stateIndex;

	if (!fNumberOfPowerStates) {
		return kPowerStateZero;
	}

	for (long order = fNumberOfPowerStates - 1; order >= 0; order--) {
		stateIndex = fPowerStates[order].stateOrderToIndex;

		if ((flags & fPowerStates[stateIndex].inputPowerFlags) ==
		    fPowerStates[stateIndex].inputPowerFlags) {
			return stateIndex;
		}
	}
	return kPowerStateZero;
}

unsigned long
IOService::maxCapabilityForDomainState( IOPMPowerFlags domainState )
{
	return getPowerStateForDomainFlags(domainState);
}

                                                                                   
                                           
  
                                                                    
                                                                                   

unsigned long
IOService::initialPowerStateForDomainState( IOPMPowerFlags domainState )
{
	if (fResetPowerStateOnWake && (domainState & kIOPMRootDomainState)) {
		                                                              
		return kPowerStateZero;
	}

	return getPowerStateForDomainFlags(domainState);
}

                                                                                   
                                    
  
                                     
                                                                                   

unsigned long
IOService::powerStateForDomainState( IOPMPowerFlags domainState )
{
	return getPowerStateForDomainFlags(domainState);
}

#ifndef __LP64__
                                                                                   
                                
  
                                                                       
                                                                                   

bool
IOService::didYouWakeSystem( void )
{
	return false;
}
#endif                

                                                                                   
                                  
  
                                                                       
                                                                                   

IOReturn
IOService::powerStateWillChangeTo( IOPMPowerFlags, unsigned long, IOService * )
{
	return kIOPMAckImplied;
}

                                                                                   
                                 
  
                                                                       
                                                                                   

IOReturn
IOService::powerStateDidChangeTo( IOPMPowerFlags, unsigned long, IOService * )
{
	return kIOPMAckImplied;
}

                                                                                   
                              
  
                                   
                                                                             
                                                                                   

void
IOService::powerChangeDone( unsigned long )
{
}

#ifndef __LP64__
                                                                                   
                              
  
                                                                       
                                                                                   

IOReturn
IOService::newTemperature( long currentTemp, IOService * whichZone )
{
	return IOPMNoErr;
}
#endif                

                                                                                   
                              
  
                                            
                                                                                   

void
IOService::systemWillShutdown( IOOptionBits specifier )
{
	IOPMrootDomain * rootDomain = IOService::getPMRootDomain();
	if (rootDomain) {
		rootDomain->acknowledgeSystemWillShutdown( this );
	}
}

          
                         

                                                                                   
                                    
                                                                                   

IOPMRequest *
IOService::acquirePMRequest( IOService * target, IOOptionBits requestType,
    IOPMRequest * active )
{
	IOPMRequest * request;

	assert(target);

	request = IOPMRequest::create();
	if (request) {
		request->init( target, requestType );
		if (active) {
			IOPMRequest * root = active->getRootRequest();
			if (root) {
				request->attachRootRequest(root);
			}
		}
	} else {
		PM_ERROR("%s: No memory for PM request type 0x%x\n",
		    target->getName(), (uint32_t) requestType);
	}
	return request;
}

                                                                                   
                                    
                                                                                   

void
IOService::releasePMRequest( IOPMRequest * request )
{
	if (request) {
		request->reset();
		request->release();
	}
}

                                                                                   
                                   
                                                                                   

void
IOService::submitPMRequest( IOPMRequest * request )
{
	assert( request );
	assert( gIOPMReplyQueue );
	assert( gIOPMRequestQueue );

	PM_LOG1("[+ %02lx] %p [%p %s] %p %p %p\n",
	    (long)request->getType(), OBFUSCATE(request),
	    OBFUSCATE(request->getTarget()), request->getTarget()->getName(),
	    OBFUSCATE(request->fArg0),
	    OBFUSCATE(request->fArg1), OBFUSCATE(request->fArg2));

	if (request->isReplyType()) {
		gIOPMReplyQueue->queuePMRequest( request );
	} else {
		gIOPMRequestQueue->queuePMRequest( request );
	}
}

void
IOService::submitPMRequests( IOPMRequest ** requests, IOItemCount count )
{
	assert( requests );
	assert( count > 0 );
	assert( gIOPMRequestQueue );

	for (IOItemCount i = 0; i < count; i++) {
		IOPMRequest * req = requests[i];
		PM_LOG1("[+ %02lx] %p [%p %s] %p %p %p\n",
		    (long)req->getType(), OBFUSCATE(req),
		    OBFUSCATE(req->getTarget()), req->getTarget()->getName(),
		    OBFUSCATE(req->fArg0),
		    OBFUSCATE(req->fArg1), OBFUSCATE(req->fArg2));
	}

	gIOPMRequestQueue->queuePMRequestChain( requests, count );
}

                                                                                   
                                 
  
                                                                                
                                                                                   

bool
IOService::actionPMRequestQueue(
	IOPMRequest *       request,
	IOPMRequestQueue *  queue )
{
	bool more;

	if (initialized) {
		                                                                     
		                                                                   

		more = gIOPMWorkQueue->queuePMRequest(request, pwrMgt);
	} else {
		                                                                
		                                                         

		PM_LOG("%s: PM not initialized\n", getName());
		PM_LOG1("[- %02x] %p [%p %s] !initialized\n",
		    request->getType(), OBFUSCATE(request),
		    OBFUSCATE(this), getName());

		more = gIOPMCompletionQueue->queuePMRequest(request);
		if (more) {
			gIOPMWorkQueue->incrementProducerCount();
		}
	}

	return more;
}

                                                                                   
                                    
  
                                                                         
                  
                                                                                   

bool
IOService::actionPMCompletionQueue(
	IOPMRequest *         request,
	IOPMCompletionQueue * queue )
{
	bool            more = (request->getNextRequest() != NULL);
	IOPMRequest *   root = request->getRootRequest();

	if (root && (root != request)) {
		more = true;
	}
	if (more) {
		gIOPMWorkQueue->incrementProducerCount();
	}

	releasePMRequest( request );
	return more;
}

                                                                                   
                                    
  
                                                                                 
                                                                                   

bool
IOService::actionPMWorkQueueRetire( IOPMRequest * request, IOPMWorkQueue * queue )
{
	assert(request && queue);

	PM_LOG1("[- %02x] %p [%p %s] state %d, busy %d\n",
	    request->getType(), OBFUSCATE(request),
	    OBFUSCATE(this), getName(),
	    fMachineState, gIOPMBusyRequestCount);

	                                               
	if (request->getType() == kIOPMRequestTypeActivityTickle) {
		uint32_t tickleFlags = (uint32_t)(uintptr_t) request->fArg1;

		if ((tickleFlags & kTickleTypePowerDrop) && fIdleTimerPeriod) {
			restartIdleTimer();
		} else if (tickleFlags == (kTickleTypeActivity | kTickleTypePowerRise)) {
			                                                       
			                           
			fIdleTimerGeneration++;
		}
	}

	                                                                 
	                     

	return gIOPMCompletionQueue->queuePMRequest( request );
}

                                                                                   
                        
  
                                                
                                                                                   

bool
IOService::isPMBlocked( IOPMRequest * request, int count )
{
	int reason = 0;

	do {
		if (kIOPM_Finished == fMachineState) {
			break;
		}

		if (kIOPM_DriverThreadCallDone == fMachineState) {
			                                 
			                                  
			                               
			                                 
			if (fDriverCallBusy) {
				reason = 5 + fDriverCallReason;
			}
			break;
		}

		                                               
		if (fDriverTimer) {
			reason = 1; break;
		}

		                                           
		if (fHeadNotePendingAcks) {
			reason = 2; break;
		}

		                                                      
		if (fResponseArray) {
			reason = 3; break;
		}

#if USE_SETTLE_TIMER
		                                      
		if (fSettleTimeUS) {
			reason = 4; break;
		}
#endif
	} while (false);

	fWaitReason = reason;

	if (reason) {
		if (count) {
			PM_LOG1("[B %02x] %p [%p %s] state %d, reason %d\n",
			    request->getType(), OBFUSCATE(request),
			    OBFUSCATE(this), getName(),
			    fMachineState, reason);
		}

		return true;
	}

	return false;
}

                                                                                   
                                    
  
                                                         
                                
                                                                                   

bool
IOService::actionPMWorkQueueInvoke( IOPMRequest * request, IOPMWorkQueue * queue )
{
	bool    done = false;
	int     loop = 0;

	assert(request && queue);

	while (isPMBlocked(request, loop++) == false) {
		PM_LOG1("[W %02x] %p [%p %s] state %d\n",
		    request->getType(), OBFUSCATE(request),
		    OBFUSCATE(this), getName(), fMachineState);

		gIOPMRequest = request;
		gIOPMWorkInvokeCount++;

		                                                                     

		switch (fMachineState) {
		case kIOPM_Finished:
			start_watchdog_timer();

			executePMRequest( request );
			break;

		case kIOPM_OurChangeTellClientsPowerDown:
			                                                   
			if (IS_ROOT_DOMAIN) {
				bool cancel = (bool) fDoNotPowerDown;
				getPMRootDomain()->askChangeDownDone(
					&fHeadNoteChangeFlags, &cancel);
				fDoNotPowerDown = cancel;
			}

			                                       
			if (!fDoNotPowerDown) {
				                      
				OurChangeTellClientsPowerDown();
			} else {
				OUR_PMLog(kPMLogIdleCancel, (uintptr_t) this, fMachineState);
				PM_ERROR("%s: idle cancel, state %u\n", fName, fMachineState);
				if (IS_ROOT_DOMAIN) {
					                                                     
					tellChangeUp(fCurrentPowerState);
				} else {
					tellNoChangeDown(fHeadNotePowerState);
				}
				                                   
				fHeadNoteChangeFlags |= kIOPMNotDone;
				                 
				OurChangeFinish();
			}
			break;

		case kIOPM_OurChangeTellUserPMPolicyPowerDown:
			                                                           
			if (fDoNotPowerDown) {
				OUR_PMLog(kPMLogIdleCancel, (uintptr_t) this, fMachineState);
				PM_ERROR("%s: idle cancel, state %u\n", fName, fMachineState);
				if (IS_ROOT_DOMAIN) {
					                                                     
					tellChangeUp(fCurrentPowerState);
				} else {
					tellNoChangeDown(fHeadNotePowerState);
				}
				                                   
				fHeadNoteChangeFlags |= kIOPMNotDone;
				                 
				OurChangeFinish();
			} else {
				OurChangeTellUserPMPolicyPowerDown();
			}
			break;

		case kIOPM_OurChangeTellPriorityClientsPowerDown:
			                                            
			                                            
			if (fDoNotPowerDown) {
				OUR_PMLog(kPMLogIdleCancel, (uintptr_t) this, fMachineState);
				PM_ERROR("%s: idle revert, state %u\n", fName, fMachineState);
				                                               
				tellChangeUp(fCurrentPowerState);
				                                   
				fHeadNoteChangeFlags |= kIOPMNotDone;
				                 
				OurChangeFinish();
			} else {
				                       
				OurChangeTellPriorityClientsPowerDown();
			}
			break;

		case kIOPM_OurChangeNotifyInterestedDriversWillChange:
			OurChangeNotifyInterestedDriversWillChange();
			break;

		case kIOPM_OurChangeSetPowerState:
			OurChangeSetPowerState();
			break;

		case kIOPM_OurChangeWaitForPowerSettle:
			OurChangeWaitForPowerSettle();
			break;

		case kIOPM_OurChangeNotifyInterestedDriversDidChange:
			OurChangeNotifyInterestedDriversDidChange();
			break;

		case kIOPM_OurChangeTellCapabilityDidChange:
			OurChangeTellCapabilityDidChange();
			break;

		case kIOPM_OurChangeFinish:
			OurChangeFinish();
			break;

		case kIOPM_ParentChangeTellPriorityClientsPowerDown:
			ParentChangeTellPriorityClientsPowerDown();
			break;

		case kIOPM_ParentChangeNotifyInterestedDriversWillChange:
			ParentChangeNotifyInterestedDriversWillChange();
			break;

		case kIOPM_ParentChangeSetPowerState:
			ParentChangeSetPowerState();
			break;

		case kIOPM_ParentChangeWaitForPowerSettle:
			ParentChangeWaitForPowerSettle();
			break;

		case kIOPM_ParentChangeNotifyInterestedDriversDidChange:
			ParentChangeNotifyInterestedDriversDidChange();
			break;

		case kIOPM_ParentChangeTellCapabilityDidChange:
			ParentChangeTellCapabilityDidChange();
			break;

		case kIOPM_ParentChangeAcknowledgePowerChange:
			ParentChangeAcknowledgePowerChange();
			break;

		case kIOPM_DriverThreadCallDone:
			switch (fDriverCallReason) {
			case kDriverCallInformPreChange:
			case kDriverCallInformPostChange:
				notifyInterestedDriversDone();
				break;
			case kDriverCallSetPowerState:
				notifyControllingDriverDone();
				break;
			case kRootDomainInformPreChange:
				notifyRootDomainDone();
				break;
			default:
				panic("%s: bad call reason %x",
				    getName(), fDriverCallReason);
			}
			break;

		case kIOPM_NotifyChildrenOrdered:
			notifyChildrenOrdered();
			break;

		case kIOPM_NotifyChildrenDelayed:
			notifyChildrenDelayed();
			break;

		case kIOPM_NotifyChildrenStart:
			                                                               
			MS_POP();
			notifyRootDomain();
			break;

		case kIOPM_SyncTellClientsPowerDown:
			                                                   
			if (IS_ROOT_DOMAIN) {
				bool cancel = (bool) fDoNotPowerDown;
				getPMRootDomain()->askChangeDownDone(
					&fHeadNoteChangeFlags, &cancel);
				fDoNotPowerDown = cancel;
			}
			if (!fDoNotPowerDown) {
				fMachineState = kIOPM_SyncTellPriorityClientsPowerDown;
				fOutOfBandParameter = kNotifyApps;
				tellChangeDown(fHeadNotePowerState);
			} else {
				                                                      
				                            
				OUR_PMLog(kPMLogIdleCancel, (uintptr_t) this, fMachineState);
				PM_ERROR("%s: idle cancel, state %u\n", fName, fMachineState);
				tellNoChangeDown(fHeadNotePowerState);
				fHeadNoteChangeFlags |= kIOPMNotDone;
				OurChangeFinish();
			}
			break;

		case kIOPM_SyncTellPriorityClientsPowerDown:
			                                                           
			if (!fDoNotPowerDown) {
				fMachineState = kIOPM_SyncNotifyWillChange;
				fOutOfBandParameter = kNotifyPriority;
				tellChangeDown(fHeadNotePowerState);
			} else {
				OUR_PMLog(kPMLogIdleCancel, (uintptr_t) this, fMachineState);
				PM_ERROR("%s: idle revert, state %u\n", fName, fMachineState);
				tellChangeUp(fCurrentPowerState);
				fHeadNoteChangeFlags |= kIOPMNotDone;
				OurChangeFinish();
			}
			break;

		case kIOPM_SyncNotifyWillChange:
			if (kIOPMSyncNoChildNotify & fHeadNoteChangeFlags) {
				fMachineState = kIOPM_SyncFinish;
				continue;
			}
			fMachineState     = kIOPM_SyncNotifyDidChange;
			fDriverCallReason = kDriverCallInformPreChange;
			notifyChildren();
			break;

		case kIOPM_SyncNotifyDidChange:
			fIsPreChange = false;

			if (fHeadNoteChangeFlags & kIOPMParentInitiated) {
				fMachineState = kIOPM_SyncFinish;
			} else {
				assert(IS_ROOT_DOMAIN);
				fMachineState = kIOPM_SyncTellCapabilityDidChange;
			}

			fDriverCallReason = kDriverCallInformPostChange;
			notifyChildren();
			break;

		case kIOPM_SyncTellCapabilityDidChange:
			tellSystemCapabilityChange( kIOPM_SyncFinish );
			break;

		case kIOPM_SyncFinish:
			if (fHeadNoteChangeFlags & kIOPMParentInitiated) {
				ParentChangeAcknowledgePowerChange();
			} else {
				OurChangeFinish();
			}
			break;

		case kIOPM_TellCapabilityChangeDone:
			if (fIsPreChange) {
				if (fOutOfBandParameter == kNotifyCapabilityChangePriority) {
					MS_POP();                                             
					continue;
				}
				fOutOfBandParameter = kNotifyCapabilityChangePriority;
			} else {
				if (fOutOfBandParameter == kNotifyCapabilityChangeApps) {
					MS_POP();                                             
					continue;
				}
				fOutOfBandParameter = kNotifyCapabilityChangeApps;
			}
			tellClientsWithResponse( fOutOfBandMessage );
			break;

		default:
			panic("PMWorkQueueInvoke: unknown machine state %x",
			    fMachineState);
		}

		gIOPMRequest = NULL;

		if (fMachineState == kIOPM_Finished) {
			stop_watchdog_timer();
			done = true;
			break;
		}
	}

	return done;
}

                                                                                   
                             
                                                                                   

void
IOService::executePMRequest( IOPMRequest * request )
{
	assert( kIOPM_Finished == fMachineState );

	switch (request->getType()) {
	case kIOPMRequestTypePMStop:
		handlePMstop( request );
		break;

	case kIOPMRequestTypeAddPowerChild1:
		addPowerChild1( request );
		break;

	case kIOPMRequestTypeAddPowerChild2:
		addPowerChild2( request );
		break;

	case kIOPMRequestTypeAddPowerChild3:
		addPowerChild3( request );
		break;

	case kIOPMRequestTypeRegisterPowerDriver:
		handleRegisterPowerDriver( request );
		break;

	case kIOPMRequestTypeAdjustPowerState:
		fAdjustPowerScheduled = false;
		adjustPowerState();
		break;

	case kIOPMRequestTypePowerDomainWillChange:
		handlePowerDomainWillChangeTo( request );
		break;

	case kIOPMRequestTypePowerDomainDidChange:
		handlePowerDomainDidChangeTo( request );
		break;

	case kIOPMRequestTypeRequestPowerState:
	case kIOPMRequestTypeRequestPowerStateOverride:
		handleRequestPowerState( request );
		break;

	case kIOPMRequestTypePowerOverrideOnPriv:
	case kIOPMRequestTypePowerOverrideOffPriv:
		handlePowerOverrideChanged( request );
		break;

	case kIOPMRequestTypeActivityTickle:
		handleActivityTickle( request );
		break;

	case kIOPMRequestTypeSynchronizePowerTree:
		handleSynchronizePowerTree( request );
		break;

	case kIOPMRequestTypeSetIdleTimerPeriod:
	{
		fIdleTimerPeriod = (typeof(fIdleTimerPeriod))(uintptr_t) request->fArg0;
		fNextIdleTimerPeriod = fIdleTimerPeriod;
		if ((false == fLockedFlags.PMStop) && (fIdleTimerPeriod > 0)) {
			restartIdleTimer();
		}
	}
	break;

	case kIOPMRequestTypeIgnoreIdleTimer:
		fIdleTimerIgnored = request->fArg0 ? 1 : 0;
		break;

	case kIOPMRequestTypeQuiescePowerTree:
		gIOPMWorkQueue->finishQuiesceRequest(request);
		break;

	default:
		panic("executePMRequest: unknown request type %x", request->getType());
	}
}

                                                                                   
                               
  
                                                                       
                  
                                                                                   

bool
IOService::actionPMReplyQueue( IOPMRequest * request, IOPMRequestQueue * queue )
{
	bool more = false;

	assert( request && queue );
	assert( request->isReplyType());

	PM_LOG1("[A %02x] %p [%p %s] state %d\n",
	    request->getType(), OBFUSCATE(request),
	    OBFUSCATE(this), getName(), fMachineState);

	switch (request->getType()) {
	case kIOPMRequestTypeAllowPowerChange:
	case kIOPMRequestTypeCancelPowerChange:
		                                           
		if (responseValid((uint32_t)(uintptr_t) request->fArg0,
		    (int)(uintptr_t) request->fArg1)) {
			if (kIOPMRequestTypeCancelPowerChange == request->getType()) {
				                                                               
				                                                    
				                                                              
				                                                                
				                                          

				if ((fMachineState == kIOPM_OurChangeTellUserPMPolicyPowerDown)
				    || (fMachineState == kIOPM_OurChangeTellPriorityClientsPowerDown)
				    || ((fHeadNoteChangeFlags & kIOPMSkipAskPowerDown) == 0)) {
					fDoNotPowerDown = true;

					OSString * name = (OSString *) request->fArg2;
					getPMRootDomain()->pmStatsRecordApplicationResponse(
						gIOPMStatsResponseCancel,
						name ? name->getCStringNoCopy() : "", 0,
						0, (int)(uintptr_t) request->fArg1, NULL);
				}
			}

			if (checkForDone()) {
				stop_ack_timer();
				cleanClientResponses(false);
				more = true;
			}
		}
		                                                         
		if (request->getType() == kIOPMRequestTypeCancelPowerChange) {
			OSObject * obj = (OSObject *) request->fArg2;
			if (obj) {
				obj->release();
			}
		}
		break;

	case kIOPMRequestTypeAckPowerChange:
		more = handleAcknowledgePowerChange( request );
		break;

	case kIOPMRequestTypeAckSetPowerState:
		if (fDriverTimer == -1) {
			                                                        
			                                                                  
			OUR_PMLog(kPMLogDriverAcknowledgeSet,
			    (uintptr_t) this, fDriverTimer);
			fDriverTimer = 0;
		} else if (fDriverTimer > 0) {
			                               
			stop_ack_timer();

			getPMRootDomain()->reset_watchdog_timer(this, 0);

			uint64_t nsec = computeTimeDeltaNS(&fDriverCallStartTime);
			if (nsec > gIOPMSetPowerStateLogNS) {
				getPMRootDomain()->pmStatsRecordApplicationResponse(
					gIOPMStatsDriverPSChangeSlow,
					fName, kDriverCallSetPowerState, NS_TO_MS(nsec), getRegistryEntryID(),
					NULL, fHeadNotePowerState, true);
			}

			OUR_PMLog(kPMLogDriverAcknowledgeSet, (uintptr_t) this, fDriverTimer);
			fDriverTimer = 0;
			more = true;
		} else {
			                 
			OUR_PMLog(kPMLogAcknowledgeErr4, (uintptr_t) this, 0);
		}
		break;

	case kIOPMRequestTypeInterestChanged:
		handleInterestChanged( request );
		more = true;
		break;

	case kIOPMRequestTypeIdleCancel:
		if ((fMachineState == kIOPM_OurChangeTellClientsPowerDown)
		    || (fMachineState == kIOPM_OurChangeTellUserPMPolicyPowerDown)
		    || (fMachineState == kIOPM_OurChangeTellPriorityClientsPowerDown)
		    || (fMachineState == kIOPM_SyncTellClientsPowerDown)
		    || (fMachineState == kIOPM_SyncTellPriorityClientsPowerDown)) {
			OUR_PMLog(kPMLogIdleCancel, (uintptr_t) this, fMachineState);
			PM_LOG2("%s: cancel from machine state %d\n",
			    getName(), fMachineState);
			fDoNotPowerDown = true;
			                               
			if ((fMachineState == kIOPM_OurChangeTellPriorityClientsPowerDown) ||
			    (fMachineState == kIOPM_OurChangeTellUserPMPolicyPowerDown) ||
			    (fMachineState == kIOPM_SyncTellPriorityClientsPowerDown) ||
			    (fMachineState == kIOPM_SyncTellClientsPowerDown)) {
				cleanClientResponses(false);
			}
			more = true;
		}
		break;

	case kIOPMRequestTypeChildNotifyDelayCancel:
		if (fMachineState == kIOPM_NotifyChildrenDelayed) {
			PM_LOG2("%s: delay notify cancelled\n", getName());
			notifyChildrenDelayed();
		}
		break;

	default:
		panic("PMReplyQueue: unknown reply type %x", request->getType());
	}

	more |= gIOPMCompletionQueue->queuePMRequest(request);
	if (more) {
		gIOPMWorkQueue->incrementProducerCount();
	}

	return more;
}

                                                                                   
                                                      
                                                                                   

bool
IOService::assertPMDriverCall(
	IOPMDriverCallEntry *   entry,
	IOOptionBits            method,
	const IOPMinformee *    inform,
	IOOptionBits            options )
{
	IOService * target = NULL;
	bool        ok = false;

	if (!initialized) {
		return false;
	}

	PM_LOCK();

	if (fLockedFlags.PMStop) {
		goto fail;
	}

	if (((options & kIOPMDriverCallNoInactiveCheck) == 0) && isInactive()) {
		goto fail;
	}

	if (inform) {
		if (!inform->active) {
			goto fail;
		}
		target = inform->whatObject;
		if (target->isInactive()) {
			goto fail;
		}
	}

	                                                       
	switch (method) {
	case kIOPMDriverCallMethodSetPowerState:
		entry->callMethod = OSMemberFunctionCast(const void *, fControllingDriver, &IOService::setPowerState);
		break;
	case kIOPMDriverCallMethodWillChange:
		entry->callMethod = OSMemberFunctionCast(const void *, target, &IOService::powerStateWillChangeTo);
		break;
	case kIOPMDriverCallMethodDidChange:
		entry->callMethod = OSMemberFunctionCast(const void *, target, &IOService::powerStateDidChangeTo);
		break;
	case kIOPMDriverCallMethodUnknown:
	case kIOPMDriverCallMethodSetAggressive:
	default:
		entry->callMethod = NULL;
		break;
	}

	entry->thread = current_thread();
	entry->target = target;
	queue_enter(&fPMDriverCallQueue, entry, IOPMDriverCallEntry *, link);
	ok = true;

fail:
	PM_UNLOCK();

	return ok;
}

void
IOService::deassertPMDriverCall( IOPMDriverCallEntry * entry )
{
	bool wakeup = false;

	PM_LOCK();

	assert( !queue_empty(&fPMDriverCallQueue));
	queue_remove(&fPMDriverCallQueue, entry, IOPMDriverCallEntry *, link);
	if (fLockedFlags.PMDriverCallWait) {
		wakeup = true;
	}

	PM_UNLOCK();

	if (wakeup) {
		PM_LOCK_WAKEUP(&fPMDriverCallQueue);
	}
}

bool
IOService::getBlockingDriverCall(thread_t *thread, const void **callMethod)
{
	const IOPMDriverCallEntry * entry = NULL;
	bool    blocked = false;

	if (!initialized) {
		return false;
	}

	if (current_thread() != gIOPMWatchDogThread) {
		                                                 
		return false;
	}

	PM_LOCK();
	entry = qe_queue_first(&fPMDriverCallQueue, IOPMDriverCallEntry, link);
	if (entry) {
		*thread = entry->thread;
		*callMethod = entry->callMethod;
		blocked = true;
	}
	PM_UNLOCK();

	return blocked;
}


void
IOService::waitForPMDriverCall( IOService * target )
{
	const IOPMDriverCallEntry * entry;
	thread_t                    thread = current_thread();
	AbsoluteTime                deadline;
	int                         waitResult;
	bool                        log = true;
	bool                        wait;

	do {
		wait = false;
		queue_iterate(&fPMDriverCallQueue, entry, const IOPMDriverCallEntry *, link)
		{
			                                   
			if (target && (target != entry->target)) {
				continue;
			}

			if (entry->thread == thread) {
				if (log) {
					PM_LOG("%s: %s(%s) on PM thread\n",
					    fName, __FUNCTION__, target ? target->getName() : "");
					OSReportWithBacktrace("%s: %s(%s) on PM thread\n",
					    fName, __FUNCTION__, target ? target->getName() : "");
					log = false;
				}
				continue;
			}

			wait = true;
			break;
		}

		if (wait) {
			fLockedFlags.PMDriverCallWait = true;
			clock_interval_to_deadline(15, kSecondScale, &deadline);
			waitResult = PM_LOCK_SLEEP(&fPMDriverCallQueue, deadline);
			fLockedFlags.PMDriverCallWait = false;
			if (THREAD_TIMED_OUT == waitResult) {
				PM_ERROR("%s: waitForPMDriverCall timeout\n", fName);
				wait = false;
			}
		}
	} while (wait);
}

                                                                                   
                          
                                                                                   

const char *
IOService::getIOMessageString( uint32_t msg )
{
#define MSG_ENTRY(x)    {(int) x, #x}

	static const IONamedValue msgNames[] = {
		MSG_ENTRY( kIOMessageCanDevicePowerOff      ),
		MSG_ENTRY( kIOMessageDeviceWillPowerOff     ),
		MSG_ENTRY( kIOMessageDeviceWillNotPowerOff  ),
		MSG_ENTRY( kIOMessageDeviceHasPoweredOn     ),
		MSG_ENTRY( kIOMessageCanSystemPowerOff      ),
		MSG_ENTRY( kIOMessageSystemWillPowerOff     ),
		MSG_ENTRY( kIOMessageSystemWillNotPowerOff  ),
		MSG_ENTRY( kIOMessageCanSystemSleep         ),
		MSG_ENTRY( kIOMessageSystemWillSleep        ),
		MSG_ENTRY( kIOMessageSystemWillNotSleep     ),
		MSG_ENTRY( kIOMessageSystemHasPoweredOn     ),
		MSG_ENTRY( kIOMessageSystemWillRestart      ),
		MSG_ENTRY( kIOMessageSystemWillPowerOn      ),
		MSG_ENTRY( kIOMessageSystemCapabilityChange ),
		MSG_ENTRY( kIOPMMessageLastCallBeforeSleep  ),
		MSG_ENTRY( kIOMessageSystemPagingOff        ),
		{ 0, NULL }
	};

	return IOFindNameForValue(msg, msgNames);
}

static const char *
getNotificationPhaseString( uint32_t phase )
{
#define PHASE_ENTRY(x)    {(int) x, #x}

	static const IONamedValue phaseNames[] = {
		PHASE_ENTRY( kNotifyApps                     ),
		PHASE_ENTRY( kNotifyPriority                 ),
		PHASE_ENTRY( kNotifyCapabilityChangeApps     ),
		PHASE_ENTRY( kNotifyCapabilityChangePriority ),
		{ 0, NULL }
	};

	return IOFindNameForValue(phase, phaseNames);
}

          
                    

                                                                                   
                    
  
                                                                                
                                                                                   

OSDefineMetaClassAndStructors( IOPMRequest, IOCommand );

IOPMRequest *
IOPMRequest::create( void )
{
	IOPMRequest * me = OSTypeAlloc(IOPMRequest);
	if (me && !me->init(NULL, kIOPMRequestTypeInvalid)) {
		me->release();
		me = NULL;
	}
	return me;
}

bool
IOPMRequest::init( IOService * target, IOOptionBits type )
{
	if (!IOCommand::init()) {
		return false;
	}

	fRequestType = type;
	fTarget = target;

	if (fTarget) {
		fTarget->retain();
	}

	                                                                          
	                      

	fIsQuiesceBlocker = ((fTarget != gIOPMRootNode) &&
	    (fTarget != IOService::getPMRootDomain()));

	return true;
}

void
IOPMRequest::reset( void )
{
	assert( fWorkWaitCount == 0 );
	assert( fFreeWaitCount == 0 );

	detachNextRequest();
	detachRootRequest();

	if (fCompletionAction && (fRequestType == kIOPMRequestTypeQuiescePowerTree)) {
		                                              
		fCompletionAction(fCompletionTarget, fCompletionParam);
		fCompletionAction = NULL;
	}

	fRequestType = kIOPMRequestTypeInvalid;

	if (fTarget) {
		fTarget->release();
		fTarget = NULL;
	}
}

bool
IOPMRequest::attachNextRequest( IOPMRequest * next )
{
	bool ok = false;

	if (!fRequestNext) {
		                                                   
		                
		fRequestNext = next;
		fRequestNext->fWorkWaitCount++;
#if LOG_REQUEST_ATTACH
		PM_LOG("Attached next: %p [0x%x] -> %p [0x%x, %u] %s\n",
		    OBFUSCATE(this), fRequestType, OBFUSCATE(fRequestNext),
		    fRequestNext->fRequestType,
		    (uint32_t) fRequestNext->fWorkWaitCount,
		    fTarget->getName());
#endif
		ok = true;
	}
	return ok;
}

bool
IOPMRequest::detachNextRequest( void )
{
	bool ok = false;

	if (fRequestNext) {
		assert(fRequestNext->fWorkWaitCount);
		if (fRequestNext->fWorkWaitCount) {
			fRequestNext->fWorkWaitCount--;
		}
#if LOG_REQUEST_ATTACH
		PM_LOG("Detached next: %p [0x%x] -> %p [0x%x, %u] %s\n",
		    OBFUSCATE(this), fRequestType, OBFUSCATE(fRequestNext),
		    fRequestNext->fRequestType,
		    (uint32_t) fRequestNext->fWorkWaitCount,
		    fTarget->getName());
#endif
		fRequestNext = NULL;
		ok = true;
	}
	return ok;
}

bool
IOPMRequest::attachRootRequest( IOPMRequest * root )
{
	bool ok = false;

	if (!fRequestRoot) {
		                                                 
		                
		fRequestRoot = root;
		fRequestRoot->fFreeWaitCount++;
#if LOG_REQUEST_ATTACH
		PM_LOG("Attached root: %p [0x%x] -> %p [0x%x, %u] %s\n",
		    OBFUSCATE(this), (uint32_t) fType, OBFUSCATE(fRequestRoot),
		    (uint32_t) fRequestRoot->fType,
		    (uint32_t) fRequestRoot->fFreeWaitCount,
		    fTarget->getName());
#endif
		ok = true;
	}
	return ok;
}

bool
IOPMRequest::detachRootRequest( void )
{
	bool ok = false;

	if (fRequestRoot) {
		assert(fRequestRoot->fFreeWaitCount);
		if (fRequestRoot->fFreeWaitCount) {
			fRequestRoot->fFreeWaitCount--;
		}
#if LOG_REQUEST_ATTACH
		PM_LOG("Detached root: %p [0x%x] -> %p [0x%x, %u] %s\n",
		    OBFUSCATE(this), (uint32_t) fType, OBFUSCATE(fRequestRoot),
		    (uint32_t) fRequestRoot->fType,
		    (uint32_t) fRequestRoot->fFreeWaitCount,
		    fTarget->getName());
#endif
		fRequestRoot = NULL;
		ok = true;
	}
	return ok;
}

          
                         

                                                                                   
                         
  
                                                             
                                                                                   

OSDefineMetaClassAndStructors( IOPMRequestQueue, IOEventSource );

IOPMRequestQueue *
IOPMRequestQueue::create( IOService * inOwner, Action inAction )
{
	IOPMRequestQueue * me = OSTypeAlloc(IOPMRequestQueue);
	if (me && !me->init(inOwner, inAction)) {
		me->release();
		me = NULL;
	}
	return me;
}

bool
IOPMRequestQueue::init( IOService * inOwner, Action inAction )
{
	if (!inAction || !IOEventSource::init(inOwner, (IOEventSourceAction)inAction)) {
		return false;
	}

	queue_init(&fQueue);
	fLock = IOLockAlloc();
	return fLock != NULL;
}

void
IOPMRequestQueue::free( void )
{
	if (fLock) {
		IOLockFree(fLock);
		fLock = NULL;
	}
	return IOEventSource::free();
}

void
IOPMRequestQueue::queuePMRequest( IOPMRequest * request )
{
	uint64_t now = mach_continuous_time();

	assert(request);
	request->setTimestamp(now);
	IOLockLock(fLock);
	queue_enter(&fQueue, request, typeof(request), fCommandChain);
	IOLockUnlock(fLock);
	if (workLoop) {
		signalWorkAvailable();
	}
}

void
IOPMRequestQueue::queuePMRequestChain( IOPMRequest ** requests, IOItemCount count )
{
	IOPMRequest * next;
	uint64_t now = mach_continuous_time();

	assert(requests && count);
	IOLockLock(fLock);
	while (count--) {
		next = *requests;
		next->setTimestamp(now);
		requests++;
		queue_enter(&fQueue, next, typeof(next), fCommandChain);
	}
	IOLockUnlock(fLock);
	if (workLoop) {
		signalWorkAvailable();
	}
}

bool
IOPMRequestQueue::checkForWork( void )
{
	Action          dqAction = (Action) action;
	IOPMRequest *   request;
	IOService *     target;
	int             dequeueCount = 0;
	bool            more = false;

	IOLockLock( fLock );

	while (!queue_empty(&fQueue)) {
		if (dequeueCount++ >= kMaxDequeueCount) {
			                                      
			more = true;
			break;
		}

		queue_remove_first(&fQueue, request, typeof(request), fCommandChain);
		IOLockUnlock(fLock);
		target = request->getTarget();
		assert(target);
		more |= (*dqAction)( target, request, this );
		IOLockLock( fLock );
	}

	IOLockUnlock( fLock );
	return more;
}

          
                      

                                                                                   
                      
  
                                                                             
               
                                                                                   

OSDefineMetaClassAndStructors( IOPMWorkQueue, IOEventSource );

IOPMWorkQueue *
IOPMWorkQueue::create( IOService * inOwner, Action invoke, Action retire )
{
	IOPMWorkQueue * me = OSTypeAlloc(IOPMWorkQueue);
	if (me && !me->init(inOwner, invoke, retire)) {
		me->release();
		me = NULL;
	}
	return me;
}

bool
IOPMWorkQueue::init( IOService * inOwner, Action invoke, Action retire )
{
	if (!invoke || !retire ||
	    !IOEventSource::init(inOwner, (IOEventSourceAction)NULL)) {
		return false;
	}

	queue_init(&fWorkQueue);

	fInvokeAction  = invoke;
	fRetireAction  = retire;
	fConsumerCount = fProducerCount = 0;

	return true;
}

bool
IOPMWorkQueue::queuePMRequest( IOPMRequest * request, IOServicePM * pwrMgt )
{
	queue_head_t *  requestQueue;
	bool            more  = false;
	bool            empty;

	assert( request );
	assert( pwrMgt );
	assert( onThread());
	assert( queue_next(&request->fCommandChain) ==
	    queue_prev(&request->fCommandChain));

	gIOPMBusyRequestCount++;

	if (request->isQuiesceType()) {
		if ((request->getTarget() == gIOPMRootNode) && !fQuiesceStartTime) {
			                                                                  
			fQuiesceStartTime = mach_absolute_time();
			attachQuiesceRequest(request);
			fQuiesceRequest = request;
		}
	} else if (fQuiesceRequest && request->isQuiesceBlocker()) {
		                                                                
		request->attachNextRequest(fQuiesceRequest);
	}

	                                                                
	                                                               
	                          

	requestQueue = &pwrMgt->RequestHead;
	empty = queue_empty(requestQueue);
	queue_enter(requestQueue, request, typeof(request), fCommandChain);
	if (empty) {
		more = checkRequestQueue(requestQueue, &empty);
		if (!empty) {
			                                                            
			                     
			assert( queue_next(&pwrMgt->WorkChain) ==
			    queue_prev(&pwrMgt->WorkChain));

			queue_enter(&fWorkQueue, pwrMgt, typeof(pwrMgt), WorkChain);
			fQueueLength++;
			PM_LOG3("IOPMWorkQueue: [%u] added %s@%p to queue\n",
			    fQueueLength, pwrMgt->Name, OBFUSCATE(pwrMgt));
		}
	}

	return more;
}

bool
IOPMWorkQueue::checkRequestQueue( queue_head_t * requestQueue, bool * empty )
{
	IOPMRequest *   request;
	IOService *     target;
	bool            more = false;
	bool            done = false;

	assert(!queue_empty(requestQueue));
	do {
		request = (typeof(request))queue_first(requestQueue);
		if (request->isWorkBlocked()) {
			break;                                                
		}
		target = request->getTarget();
		if (fInvokeAction) {
			done = (*fInvokeAction)( target, request, this );
		} else {
			PM_LOG("PM request 0x%x dropped\n", request->getType());
			done = true;
		}
		if (!done) {
			break;                            
		}
		assert(gIOPMBusyRequestCount > 0);
		if (gIOPMBusyRequestCount) {
			gIOPMBusyRequestCount--;
		}

		if (request == fQuiesceRequest) {
			fQuiesceRequest = NULL;
		}

		queue_remove_first(requestQueue, request, typeof(request), fCommandChain);
		more |= (*fRetireAction)( target, request, this );
		done = queue_empty(requestQueue);
	} while (!done);

	*empty = done;

	if (more) {
		                                                                  
		                                                                
		               
		fProducerCount++;
	}

	return more;
}

bool
IOPMWorkQueue::checkForWork( void )
{
	IOServicePM *   entry;
	IOServicePM *   next;
	bool            more = false;
	bool            empty;

#if WORK_QUEUE_STATS
	fStatCheckForWork++;
#endif

	                                                          
	                                        

	while (fConsumerCount != fProducerCount) {
		PM_LOG3("IOPMWorkQueue: checkForWork %u %u\n",
		    fProducerCount, fConsumerCount);

		fConsumerCount = fProducerCount;

#if WORK_QUEUE_STATS
		if (queue_empty(&fWorkQueue)) {
			fStatQueueEmpty++;
			break;
		}
		fStatScanEntries++;
		uint32_t cachedWorkCount = gIOPMWorkInvokeCount;
#endif

		__IGNORE_WCASTALIGN(entry = (typeof(entry))queue_first(&fWorkQueue));
		while (!queue_end(&fWorkQueue, (queue_entry_t) entry)) {
			more |= checkRequestQueue(&entry->RequestHead, &empty);

			                                                           
			__IGNORE_WCASTALIGN(next = (typeof(next))queue_next(&entry->WorkChain));

			                                                                 
			if (empty) {
				assert(fQueueLength);
				if (fQueueLength) {
					fQueueLength--;
				}
				PM_LOG3("IOPMWorkQueue: [%u] removed %s@%p from queue\n",
				    fQueueLength, entry->Name, OBFUSCATE(entry));
				queue_remove(&fWorkQueue, entry, typeof(entry), WorkChain);
			}
			entry = next;
		}

#if WORK_QUEUE_STATS
		if (cachedWorkCount == gIOPMWorkInvokeCount) {
			fStatNoWorkDone++;
		}
#endif
	}

	return more;
}

void
IOPMWorkQueue::signalWorkAvailable( void )
{
	fProducerCount++;
	IOEventSource::signalWorkAvailable();
}

void
IOPMWorkQueue::incrementProducerCount( void )
{
	fProducerCount++;
}

void
IOPMWorkQueue::attachQuiesceRequest( IOPMRequest * quiesceRequest )
{
	IOServicePM *   entry;
	IOPMRequest *   request;

	if (queue_empty(&fWorkQueue)) {
		return;
	}

	queue_iterate(&fWorkQueue, entry, typeof(entry), WorkChain)
	{
		queue_iterate(&entry->RequestHead, request, typeof(request), fCommandChain)
		{
			                                                              
			                                                             
			                       

			if (request->isQuiesceBlocker()) {
				request->attachNextRequest(quiesceRequest);
			}
		}
	}
}

void
IOPMWorkQueue::finishQuiesceRequest( IOPMRequest * quiesceRequest )
{
	if (fQuiesceRequest && (quiesceRequest == fQuiesceRequest) &&
	    (fQuiesceStartTime != 0)) {
		fInvokeAction = NULL;
		fQuiesceFinishTime = mach_absolute_time();
	}
}

          
                            

                                                                                   
                            
                                                                                   

OSDefineMetaClassAndStructors( IOPMCompletionQueue, IOEventSource );

IOPMCompletionQueue *
IOPMCompletionQueue::create( IOService * inOwner, Action inAction )
{
	IOPMCompletionQueue * me = OSTypeAlloc(IOPMCompletionQueue);
	if (me && !me->init(inOwner, inAction)) {
		me->release();
		me = NULL;
	}
	return me;
}

bool
IOPMCompletionQueue::init( IOService * inOwner, Action inAction )
{
	if (!inAction || !IOEventSource::init(inOwner, (IOEventSourceAction)inAction)) {
		return false;
	}

	queue_init(&fQueue);
	return true;
}

bool
IOPMCompletionQueue::queuePMRequest( IOPMRequest * request )
{
	bool more;

	assert(request);
	                            
	more = request->detachNextRequest();
	queue_enter(&fQueue, request, typeof(request), fCommandChain);
	return more;
}

bool
IOPMCompletionQueue::checkForWork( void )
{
	Action          dqAction = (Action) action;
	IOPMRequest *   request;
	IOPMRequest *   next;
	IOService *     target;
	bool            more = false;

	request = (typeof(request))queue_first(&fQueue);
	while (!queue_end(&fQueue, (queue_entry_t) request)) {
		next = (typeof(next))queue_next(&request->fCommandChain);
		if (!request->isFreeBlocked()) {
			queue_remove(&fQueue, request, typeof(request), fCommandChain);
			target = request->getTarget();
			assert(target);
			more |= (*dqAction)( target, request, this );
		}
		request = next;
	}

	return more;
}

          
                    

OSDefineMetaClassAndStructors(IOServicePM, OSObject)

                                                                                   
            
  
                                       
                                                                                   

static void
setPMProperty( OSDictionary * dict, const char * key, uint64_t value )
{
	OSNumber * num = OSNumber::withNumber(value, sizeof(value) * 8);
	if (num) {
		dict->setObject(key, num);
		num->release();
	}
}

IOReturn
IOServicePM::gatedSerialize( OSSerialize * s  ) const
{
	OSDictionary *  dict;
	bool            ok = false;
	int             powerClamp = -1;
	int             dictSize = 6;

	if (IdleTimerPeriod) {
		dictSize += 4;
	}

	if (PMActions.state & kPMActionsStatePowerClamped) {
		dictSize += 1;
		powerClamp = 0;
		if (PMActions.flags &
		    (kPMActionsFlagIsDisplayWrangler | kPMActionsFlagIsGraphicsDriver)) {
			powerClamp++;
		}
	}

#if WORK_QUEUE_STATS
	if (gIOPMRootNode == ControllingDriver) {
		dictSize += 4;
	}
#endif

	if (PowerClients) {
		dict = OSDictionary::withDictionary(
			PowerClients, PowerClients->getCount() + dictSize);
	} else {
		dict = OSDictionary::withCapacity(dictSize);
	}

	if (dict) {
		setPMProperty(dict, "CurrentPowerState", CurrentPowerState);
		setPMProperty(dict, "CapabilityFlags", CurrentCapabilityFlags);
		if (NumberOfPowerStates) {
			setPMProperty(dict, "MaxPowerState", NumberOfPowerStates - 1);
		}
		if (DesiredPowerState != CurrentPowerState) {
			setPMProperty(dict, "DesiredPowerState", DesiredPowerState);
		}
		if (kIOPM_Finished != MachineState) {
			setPMProperty(dict, "MachineState", MachineState);
		}
		if (DeviceOverrideEnabled) {
			dict->setObject("PowerOverrideOn", kOSBooleanTrue);
		}
		if (powerClamp >= 0) {
			setPMProperty(dict, "PowerClamp", powerClamp);
		}

		if (IdleTimerPeriod) {
			AbsoluteTime    now;
			AbsoluteTime    delta;
			uint64_t        nsecs;

			clock_get_uptime(&now);

			                                        
			setPMProperty(dict, "IdleTimerPeriod", NextIdleTimerPeriod * 1000ULL);

			                                                         
			setPMProperty(dict, "ActivityTickles", ActivityTickleCount);

			if (AbsoluteTime_to_scalar(&DeviceActiveTimestamp)) {
				                                              
				delta = now;
				SUB_ABSOLUTETIME(&delta, &DeviceActiveTimestamp);
				absolutetime_to_nanoseconds(delta, &nsecs);
				setPMProperty(dict, "TimeSinceLastTickle", NS_TO_MS(nsecs));
			}

			if (!IdleTimerStopped && AbsoluteTime_to_scalar(&IdleTimerStartTime)) {
				                                          
				delta = now;
				SUB_ABSOLUTETIME(&delta, &IdleTimerStartTime);
				absolutetime_to_nanoseconds(delta, &nsecs);
				setPMProperty(dict, "IdleTimerElapsedTime", NS_TO_MS(nsecs));
			}
		}

#if WORK_QUEUE_STATS
		if (gIOPMRootNode == Owner) {
			setPMProperty(dict, "WQ-CheckForWork",
			    gIOPMWorkQueue->fStatCheckForWork);
			setPMProperty(dict, "WQ-ScanEntries",
			    gIOPMWorkQueue->fStatScanEntries);
			setPMProperty(dict, "WQ-QueueEmpty",
			    gIOPMWorkQueue->fStatQueueEmpty);
			setPMProperty(dict, "WQ-NoWorkDone",
			    gIOPMWorkQueue->fStatNoWorkDone);
		}
#endif

		if (HasAdvisoryDesire && !gIOPMAdvisoryTickleEnabled) {
			                                                        
			dict->removeObject(gIOPMPowerClientAdvisoryTickle);
		}

		ok = dict->serialize(s);
		dict->release();
	}

	return ok ? kIOReturnSuccess : kIOReturnNoMemory;
}

bool
IOServicePM::serialize( OSSerialize * s ) const
{
	IOReturn ret = kIOReturnNotReady;

	if (gIOPMWatchDogThread == current_thread()) {
		                                                                                   
		                                                          
		ret = gatedSerialize(s);
	} else if (gIOPMWorkLoop) {
		ret = gIOPMWorkLoop->runAction(
			OSMemberFunctionCast(IOWorkLoop::Action, this, &IOServicePM::gatedSerialize),
			(OSObject *) this, (void *) s);
	}

	return kIOReturnSuccess == ret;
}

void
IOServicePM::pmPrint(
	uint32_t        event,
	uintptr_t       param1,
	uintptr_t       param2 ) const
{
	gPlatform->PMLog(Name, event, param1, param2);
}

void
IOServicePM::pmTrace(
	uint32_t        event,
	uint32_t        eventFunc,
	uintptr_t       param1,
	uintptr_t       param2 ) const
{
	uintptr_t nameAsArg = 0;

	assert(event < KDBG_CODE_MAX);
	assert((eventFunc & ~KDBG_FUNC_MASK) == 0);

	                                                           
	                                    
	strncpy((char*)&nameAsArg, Name, sizeof(nameAsArg));

#if defined(XNU_TARGET_OS_OSX)
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, IODBG_POWER(event) | eventFunc, nameAsArg,
	    (uintptr_t)Owner->getRegistryEntryID(), (uintptr_t)(OBFUSCATE(param1)),
	    (uintptr_t)(OBFUSCATE(param2)), 0);
#else
	IOTimeStampConstant(IODBG_POWER(event) | eventFunc, nameAsArg, (uintptr_t)Owner->getRegistryEntryID(), (uintptr_t)(OBFUSCATE(param1)), (uintptr_t)(OBFUSCATE(param2)));
#endif
}
