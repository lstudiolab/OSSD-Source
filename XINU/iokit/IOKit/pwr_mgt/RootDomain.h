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
#ifndef _IOKIT_ROOTDOMAIN_H
#define _IOKIT_ROOTDOMAIN_H

#include <libkern/c++/OSPtr.h>
#include <IOKit/IOService.h>
#include <IOKit/pwr_mgt/IOPM.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <sys/vnode.h>

#ifdef XNU_KERNEL_PRIVATE

#include <IOKit/pwr_mgt/IOPMPrivate.h>

struct AggressivesRecord;
struct IOPMMessageFilterContext;
struct IOPMActions;
struct IOPMSystemSleepParameters;
class PMSettingObject;
class PMTraceWorker;
class IOPMPowerStateQueue;
class RootDomainUserClient;
class PMAssertionsTracker;
class IOTimerEventSource;

#define OBFUSCATE(x) (void *)VM_KERNEL_UNSLIDE_OR_PERM(x)

#endif

   
                          
                                                            
   

                           
                                                                                     
                                                                                
                                                                           
   
typedef uint64_t IOPMDriverAssertionType;

                        
                                                                                      
                                                                      
                                                                          
                                                
   
typedef uint64_t IOPMDriverAssertionID;
#define kIOPMUndefinedDriverAssertionID       0

                           
                                                                                             
                                               
   
typedef uint32_t IOPMDriverAssertionLevel;
#define kIOPMDriverAssertionLevelOff          0
#define kIOPMDriverAssertionLevelOn           255

  
                                    
   
enum {
	kRootDomainSleepNotSupported    = 0x00000000,
	kRootDomainSleepSupported         = 0x00000001,
	kFrameBufferDeepSleepSupported    = 0x00000002,
	kPCICantSleep                   = 0x00000004
};

  
                                        
   
#define kRootDomainSupportedFeatures        "Supported Features"
#define kRootDomainSleepReasonKey           "Last Sleep Reason"
#define kRootDomainSleepOptionsKey          "Last Sleep Options"
#define kIOPMRootDomainWakeReasonKey        "Wake Reason"
#define kIOPMRootDomainWakeTypeKey          "Wake Type"
#define kIOPMRootDomainPowerStatusKey       "Power Status"

  
                                                                
   
#define kIOPMClamshellSleepKey              "Clamshell Sleep"
#define kIOPMPowerButtonSleepKey            "Power Button Sleep"
#define kIOPMSoftwareSleepKey               "Software Sleep"
#define kIOPMOSSwitchHibernationKey         "OS Switch Sleep"
#define kIOPMIdleSleepKey                   "Idle Sleep"
#define kIOPMLowPowerSleepKey               "Low Power Sleep"
#define kIOPMThermalEmergencySleepKey       "Thermal Emergency Sleep"
#define kIOPMMaintenanceSleepKey            "Maintenance Sleep"

  
                                                 
   
#define kIOPMRootDomainLidCloseCString      "LidClose"
#define kIOPMRootDomainBatPowerCString      "BatPower"

  
                                                                   
   
enum {
	kIOPMSupportedOnAC      = (1 << 0),
	kIOPMSupportedOnBatt    = (1 << 1),
	kIOPMSupportedOnUPS     = (1 << 2)
};

typedef IOReturn (*IOPMSettingControllerCallback)
(OSObject *target, const OSSymbol *type,
    OSObject *val, uintptr_t refcon);

__BEGIN_DECLS
IONotifier *    registerSleepWakeInterest(
	IOServiceInterestHandler, void *, void * = NULL);

IONotifier *    registerPrioritySleepWakeInterest(
	IOServiceInterestHandler handler,
	void * self, void * ref = NULL);

IOReturn        acknowledgeSleepWakeNotification(void * );

IOReturn        vetoSleepWakeNotification(void * PMrefcon);
__END_DECLS

#define IOPM_ROOTDOMAIN_REV        2

class IOPMrootDomain : public IOService
{
	OSDeclareFinalStructors(IOPMrootDomain);

public:
	static IOPMrootDomain * construct( void );

	virtual bool        start( IOService * provider ) APPLE_KEXT_OVERRIDE;
	virtual IOReturn    setAggressiveness( unsigned long, unsigned long ) APPLE_KEXT_OVERRIDE;
	virtual IOReturn    getAggressiveness( unsigned long, unsigned long * ) APPLE_KEXT_OVERRIDE;

	virtual IOReturn    sleepSystem( void );
	IOReturn            sleepSystemOptions( OSDictionary *options );

	virtual IOReturn    setProperties( OSObject * ) APPLE_KEXT_OVERRIDE;
	virtual bool        serializeProperties( OSSerialize * s ) const APPLE_KEXT_OVERRIDE;
	virtual OSPtr<OSObject>  copyProperty( const char * aKey ) const APPLE_KEXT_OVERRIDE;

                                      
                                                                           
                                                                                                
                                                                                     
                                                                  
                                                                       
                                            

	IOReturn            systemPowerEventOccurred(
		const OSSymbol *event,
		uint32_t intValue );

	IOReturn            systemPowerEventOccurred(
		const OSSymbol *event,
		OSObject *value );

#ifdef XNU_KERNEL_PRIVATE                                  
                                  
                                                                   
                                                                                      
                                                                                      
                                           
  
                                                                             
                                                                              
                                                                       
  
                                                                               
                                                                              
                                            
  
                                                                                          
                                                                                          
  
                  
                                           
                                                                   
                                               
                                                                              
                                              
                                        
  
                                                                           
  
                                                                          
                                                                                
                                                                                  
                                                 
  
                                                                                
                                                                                    
                                                          
                                                 
                                               
                                               
                                                
                                               
                                           
                                              
                                                 
                                               
                                                
                                               
                                           
                                              
                                           
                                              
                                                      
                                                
                                         
                                                 
                                            
                                               
                                               
  
                                                                        
                                                                
   
#endif
	void                claimSystemWakeEvent(
		IOService     *device,
		IOOptionBits  flags,
		const char    *reason,
		OSObject      *details = NULL );

	void                claimSystemBootEvent(
		IOService     *device,
		IOOptionBits  flags,
		const char    *reason,
		OSObject      *details = NULL );

	void                claimSystemShutdownEvent(
		IOService     *device,
		IOOptionBits  flags,
		const char    *reason,
		OSObject      *details = NULL );

	virtual IOReturn    receivePowerNotification( UInt32 msg );

	virtual void        setSleepSupported( IOOptionBits flags );

	virtual IOOptionBits getSleepSupported( void );

	void                wakeFromDoze( void );

	void                requestUserActive(IOService *driver, const char *reason);

                                                            

	void                publishFeature( const char *feature );

                                                            
                                                                           
                                                                           
           
                                                                            

	void                publishFeature( const char *feature,
	    uint32_t supportedWhere,
	    uint32_t *uniqueFeatureID);

                                                                           
                                                                 

	IOReturn            removePublishedFeature( uint32_t removeFeatureID );

                           
                                                                              
                                         
                                                     
                                                        

	OSPtr<OSObject>           copyPMSetting( OSSymbol *whichSetting );

                                         
                                                                        
                                                                             
                                                                                 
                                                                   
                                                             
                                                                          
                                                                                
                                                                               
                                        
                                            

	IOReturn            registerPMSettingController(
		const OSSymbol *settings[],
		IOPMSettingControllerCallback callout,
		OSObject   *target,
		uintptr_t  refcon,
		OSObject   **handle);                                 

                                         
                                                                        
                                                                             
                                                                                 
                                                                               
                                                              
                                                                   
                                                             
                                                                          
                                                                                
                                                                               
                                        
                                            

	IOReturn            registerPMSettingController(
		const OSSymbol *settings[],
		uint32_t   supportedPowerSources,
		IOPMSettingControllerCallback callout,
		OSObject   *target,
		uintptr_t  refcon,
		OSObject   **handle);                                 

	virtual OSPtr<IONotifier> registerInterest(
		const OSSymbol * typeOfInterest,
		IOServiceInterestHandler handler,
		void * target, void * ref = NULL ) APPLE_KEXT_OVERRIDE;

	virtual IOReturn    callPlatformFunction(
		const OSSymbol *functionName,
		bool waitForFunction,
		void *param1, void *param2,
		void *param3, void *param4 ) APPLE_KEXT_OVERRIDE;

                               
                                                                       
                                                                                          
                                                                                                        
                                         
                                                                                 
                                                                                              
                                                                              
   
	IOPMDriverAssertionID createPMAssertion(
		IOPMDriverAssertionType whichAssertionsBits,
		IOPMDriverAssertionLevel assertionLevel,
		IOService *ownerService,
		const char *ownerDescription);

                                
                                                           
                                                                                
                                                                                      
                                                                                       
                    
                                                                                            
                                                  
                                                                                                
   
	IOReturn setPMAssertionLevel(IOPMDriverAssertionID assertionID, IOPMDriverAssertionLevel assertionLevel);

                                 
                                                                     
                                                                     
                                                                                       
                                                                               
                                      
                                                                                                    
                                                                                                 
                                                                 
                                                                                                            
                                                                                                
                             
   
	IOPMDriverAssertionLevel getPMAssertionLevel(IOPMDriverAssertionType whichAssertionBits);

                                
                                                                       
                                                                                
   
	IOReturn releasePMAssertion(IOPMDriverAssertionID releaseAssertion);

                                  
                                                                     
                                                                                                
   
	IOReturn restartWithStackshot();

	IOReturn    setWakeTime(uint64_t wakeContinuousTime);

private:
	unsigned long getRUN_STATE(void);

	virtual IOReturn    changePowerStateTo( unsigned long ordinal ) APPLE_KEXT_COMPATIBILITY_OVERRIDE;
	virtual IOReturn    changePowerStateToPriv( unsigned long ordinal );
	virtual IOReturn    requestPowerDomainState( IOPMPowerFlags, IOPowerConnection *, unsigned long ) APPLE_KEXT_OVERRIDE;
	virtual void        powerChangeDone( unsigned long ) APPLE_KEXT_OVERRIDE;
	virtual bool        tellChangeDown( unsigned long ) APPLE_KEXT_OVERRIDE;
	virtual bool        askChangeDown( unsigned long ) APPLE_KEXT_OVERRIDE;
	virtual void        tellChangeUp( unsigned long ) APPLE_KEXT_OVERRIDE;
	virtual void        tellNoChangeDown( unsigned long ) APPLE_KEXT_OVERRIDE;
	virtual IOReturn configureReport(IOReportChannelList   *channels,
	    IOReportConfigureAction action,
	    void                    *result,
	    void                    *destination) APPLE_KEXT_OVERRIDE;
	virtual IOReturn updateReport(IOReportChannelList      *channels,
	    IOReportUpdateAction     action,
	    void                     *result,
	    void                     *destination) APPLE_KEXT_OVERRIDE;

	void             configureReportGated(uint64_t channel_id,
	    uint64_t action,
	    void     *result);
	IOReturn         updateReportGated(uint64_t ch_id,
	    void *result,
	    IOBufferMemoryDescriptor *dest);

#ifdef XNU_KERNEL_PRIVATE
                           
public:
	void        tagPowerPlaneService(
		IOService *             service,
		IOPMActions *           actions,
		IOPMPowerStateIndex     maxPowerState );

	void        overrideOurPowerChange(
		IOService *             service,
		IOPMActions *           actions,
		const IOPMRequest *     request,
		IOPMPowerStateIndex *   inOutPowerState,
		IOPMPowerChangeFlags *  inOutChangeFlags );

	void        handleOurPowerChangeStart(
		IOService *             service,
		IOPMActions *           actions,
		const IOPMRequest *     request,
		IOPMPowerStateIndex     powerState,
		IOPMPowerChangeFlags *  inOutChangeFlags );

	void        handleOurPowerChangeDone(
		IOService *             service,
		IOPMActions *           actions,
		const IOPMRequest *     request,
		IOPMPowerStateIndex     powerState,
		IOPMPowerChangeFlags    changeFlags );

	void        overridePowerChangeForService(
		IOService *             service,
		IOPMActions *           actions,
		const IOPMRequest *     request,
		IOPMPowerStateIndex *   inOutPowerState,
		IOPMPowerChangeFlags *  inOutChangeFlags );

	void        handleActivityTickleForDisplayWrangler(
		IOService *             service,
		IOPMActions *           actions );

	void        handleUpdatePowerClientForDisplayWrangler(
		IOService *             service,
		IOPMActions *           actions,
		const OSSymbol *        powerClient,
		IOPMPowerStateIndex     oldPowerState,
		IOPMPowerStateIndex     newPowerState );

	bool        shouldDelayChildNotification(
		IOService *     service );

	void        handlePowerChangeStartForPCIDevice(
		IOService *             service,
		IOPMActions *           actions,
		const IOPMRequest *     request,
		IOPMPowerStateIndex     powerState,
		IOPMPowerChangeFlags *  inOutChangeFlags );

	void        handlePowerChangeDoneForPCIDevice(
		IOService *             service,
		IOPMActions *           actions,
		const IOPMRequest *     request,
		IOPMPowerStateIndex     powerState,
		IOPMPowerChangeFlags    changeFlags );

	void        askChangeDownDone(
		IOPMPowerChangeFlags * inOutChangeFlags,
		bool * cancel );

	void        handlePublishSleepWakeUUID(
		bool shouldPublish);

	void        handleQueueSleepWakeUUID(
		OSObject *obj);

	void        willTellSystemCapabilityDidChange(void);

	void        handleSetDisplayPowerOn(bool powerOn);

	void        willNotifyPowerChildren( IOPMPowerStateIndex newPowerState );

	IOReturn    setMaintenanceWakeCalendar(
		const IOPMCalendarStruct * calendar );

	IOReturn    getSystemSleepType(uint32_t * sleepType, uint32_t * standbyTimer);

                                                        
	void        acknowledgeSystemWillShutdown( IOService * from );

                                                 
	void        handlePlatformHaltRestart( UInt32 pe_type );

	IOReturn    shutdownSystem( void );
	IOReturn    restartSystem( void );
	void        handleSleepTimerExpiration( void );

	bool        activitySinceSleep(void);
	bool        abortHibernation(void);
	void        updateConsoleUsers(void);

	IOReturn    joinAggressiveness( IOService * service );
	void        handleAggressivesRequests( void );

	void        kdebugTrace(uint32_t event, uint64_t regId,
	    uintptr_t param1, uintptr_t param2, uintptr_t param3 = 0);
	void        tracePoint(uint8_t point);
	void        traceDetail(uint32_t msgType, uint32_t msgIndex, uint32_t delay);
	void        traceNotification(OSObject *notifier, bool start, uint64_t ts = 0, uint32_t msgIndex = UINT_MAX);
	void        traceNotificationAck(OSObject *notifier, uint32_t delay_ms);
	void        traceNotificationResponse(OSObject *object, uint32_t delay_ms, uint32_t ack_time_us);
	void        traceFilteredNotification(OSObject *notifier);
	const char * getNotificationClientName(OSObject *notifier);

	void        startSpinDump(uint32_t spindumpKind);

	bool        systemMessageFilter(
		void * object, void * arg1, void * arg2, void * arg3 );

	bool        updatePreventIdleSleepList(
		IOService * service, bool addNotRemove );
	void        updatePreventSystemSleepList(
		IOService * service, bool addNotRemove );

	bool        updatePreventIdleSleepListInternal(
		IOService * service, bool addNotRemove, unsigned int oldCount);
	unsigned int idleSleepPreventersCount();

	void        publishPMSetting(
		const OSSymbol * feature, uint32_t where, uint32_t * featureID );

	void        pmStatsRecordEvent(
		int             eventIndex,
		AbsoluteTime    timestamp);

	void        pmStatsRecordApplicationResponse(
		const OSSymbol      *response,
		const char          *name,
		int                 messageType,
		uint32_t            delay_ms,
		uint64_t            id,
		OSObject            *object,
		IOPMPowerStateIndex ps = 0,
		bool                async = false);

	void        copyWakeReasonString( char * outBuf, size_t bufSize );
	void        copyShutdownReasonString( char * outBuf, size_t bufSize );
	void        lowLatencyAudioNotify(uint64_t time, boolean_t state);

#if HIBERNATION
	bool        getHibernateSettings(
		uint32_t *  hibernateMode,
		uint32_t *  hibernateFreeRatio,
		uint32_t *  hibernateFreeTime );
	bool        mustHibernate( void );
#endif
	void        takeStackshot(bool restart);
	void        sleepWakeDebugTrig(bool restart);
	void        sleepWakeDebugEnableWdog();
	bool        sleepWakeDebugIsWdogEnabled();
	void        sleepWakeDebugSaveSpinDumpFile();
	bool        checkShutdownTimeout();
	void        panicWithShutdownLog(uint32_t timeoutInMs) __abortlike;
	uint32_t    getWatchdogTimeout();
	void        deleteStackshot();

private:
	friend class PMSettingObject;
	friend class RootDomainUserClient;
	friend class PMAssertionsTracker;

	static IOReturn sysPowerDownHandler( void * target, void * refCon,
	    UInt32 messageType, IOService * service,
	    void * messageArgument, vm_size_t argSize );

	static IOReturn displayWranglerNotification( void * target, void * refCon,
	    UInt32 messageType, IOService * service,
	    void * messageArgument, vm_size_t argSize );

	static IOReturn rootBusyStateChangeHandler( void * target, void * refCon,
	    UInt32 messageType, IOService * service,
	    void * messageArgument, vm_size_t argSize );

	void initializeBootSessionUUID( void );

	void fullWakeDelayedWork( void );

	OSPtr<IOService>        wrangler;
	OSPtr<OSDictionary>     wranglerIdleSettings;

	IOLock                  *featuresDictLock;                           
	IOLock                  *wakeEventLock;
	IOPMPowerStateQueue     *pmPowerStateQueue;

	OSPtr<OSArray>           allowedPMSettings;
	OSPtr<OSArray>           noPublishPMSettings;
	OSPtr<PMTraceWorker>     pmTracer;
	PMAssertionsTracker     *pmAssertions;

                           
	IOLock                  *settingsCtrlLock;
	OSPtr<OSDictionary>      settingsCallbacks;
	OSPtr<OSDictionary>      fPMSettingsDict;

             
	OSPtr<const OSSymbol>   _statsNameKey;
	OSPtr<const OSSymbol>   _statsPIDKey;
	OSPtr<const OSSymbol>   _statsTimeMSKey;
	OSPtr<const OSSymbol>   _statsResponseTypeKey;
	OSPtr<const OSSymbol>   _statsMessageTypeKey;
	OSPtr<const OSSymbol>   _statsPowerCapsKey;
	uint32_t                sleepCnt;
	uint32_t                darkWakeCnt;
	uint32_t                displayWakeCnt;

	OSPtr<OSString>         queuedSleepWakeUUIDString;
	OSPtr<OSArray>          pmStatsAppResponses;
	IOLock                  *pmStatsLock;                             

	void                    *sleepDelaysReport;                                             
	uint32_t                sleepDelaysClientCnt;                                                    
	uint64_t                ts_sleepStart;
	uint64_t                wake2DarkwakeDelay;                                                     

	void                    *assertOnWakeReport;                                                                    
	uint32_t                assertOnWakeClientCnt;                                                     
	clock_sec_t             assertOnWakeSecs;                                                

	bool                    uuidPublished;

                                    
	bool                    idleSleepEnabled;
	uint32_t                sleepSlider;
	uint32_t                idleSeconds;

                                                           
	uint32_t                extraSleepDelay;

                                                        
	thread_call_t           extraSleepTimer;
	thread_call_t           powerButtonDown;
	thread_call_t           powerButtonUp;
	thread_call_t           diskSyncCalloutEntry;
	thread_call_t           fullWakeThreadCall;
	thread_call_t           updateConsoleUsersEntry;

                             
	uint32_t                _desiredCapability;
	uint32_t                _currentCapability;
	uint32_t                _pendingCapability;
	uint32_t                _highestCapability;
	OSPtr<OSSet>            _joinedCapabilityClients;
	uint32_t                _systemStateGeneration;

                                                    
	enum {
		kSystemMessageClientPowerd    = 0x01,
		kSystemMessageClientLegacyApp = 0x02,
		kSystemMessageClientKernel    = 0x04,
		kSystemMessageClientAll       = 0x07
	};
	uint32_t                _systemMessageClientMask;

                                                 
	enum {
		kSystemTransitionNone         = 0,
		kSystemTransitionSleep        = 1,
		kSystemTransitionWake         = 2,
		kSystemTransitionCapability   = 3,
		kSystemTransitionNewCapClient = 4
	}                       _systemTransitionType;

	unsigned int            systemBooting           :1;
	unsigned int            systemShutdown          :1;
	unsigned int            systemDarkWake          :1;
	unsigned int            clamshellExists         :1;
	unsigned int            clamshellClosed         :1;
	unsigned int            clamshellDisabled       :1;
	unsigned int            desktopMode             :1;
	unsigned int            acAdaptorConnected      :1;

	unsigned int            clamshellIgnoreClose    :1;
	unsigned int            idleSleepTimerPending   :1;
	unsigned int            userDisabledAllSleep    :1;
	unsigned int            ignoreTellChangeDown    :1;
	unsigned int            wranglerAsleep          :1;
	unsigned int            darkWakeExit            :1;
	unsigned int            _preventUserActive      :1;
	unsigned int            darkWakePowerClamped    :1;

	unsigned int            capabilityLoss          :1;
	unsigned int            pciCantSleepFlag        :1;
	unsigned int            pciCantSleepValid       :1;
	unsigned int            darkWakeLogClamp        :1;
	unsigned int            darkWakeToSleepASAP     :1;
	unsigned int            darkWakeMaintenance     :1;
	unsigned int            darkWakeSleepService    :1;
	unsigned int            darkWakePostTickle      :1;

	unsigned int            sleepTimerMaintenance   :1;
	unsigned int            sleepToStandby          :1;
	unsigned int            lowBatteryCondition     :1;
	unsigned int            hibernateDisabled       :1;
	unsigned int            hibernateRetry          :1;
	unsigned int            wranglerTickled         :1;
	unsigned int            userIsActive            :1;
	unsigned int            userWasActive           :1;

	unsigned int            displayIdleForDemandSleep :1;
	unsigned int            darkWakeHibernateError  :1;
	unsigned int            thermalWarningState     :1;
	unsigned int            toldPowerdCapWillChange :1;
	unsigned int            displayPowerOnRequested :1;
	unsigned int            isRTCAlarmWake          :1;
	unsigned int            wranglerPowerOff        :1;
	unsigned int            thermalEmergencyState   :1;

	uint8_t                 tasksSuspended;
	uint8_t                 tasksSuspendState;
	uint32_t                hibernateMode;
	AbsoluteTime            userActivityTime;
	AbsoluteTime            userActivityTime_prev;
	uint32_t                userActivityCount;
	uint32_t                userActivityAtSleep;
	uint32_t                lastSleepReason;
	uint32_t                fullToDarkReason;
	uint32_t                hibernateAborted;
	uint8_t                 standbyNixed;
	uint8_t                 resetTimers;

	enum FullWakeReason {
		kFullWakeReasonNone = 0,
		kFullWakeReasonLocalUser = 1,
		kFullWakeReasonDisplayOn = 2,
		fFullWakeReasonDisplayOnAndLocalUser = 3
	};
	uint32_t                fullWakeReason;

	enum {
		kClamshellSleepDisableInternal = 0x01,
		kClamshellSleepDisablePowerd   = 0x02
	};
	uint32_t                clamshellSleepDisableMask;

                                                       
	int32_t                 idxPMCPUClamshell;
	int32_t                 idxPMCPULimitedPower;

	IOOptionBits            platformSleepSupport;
	uint32_t                _debugWakeSeconds;

	queue_head_t            aggressivesQueue;
	thread_call_t           aggressivesThreadCall;
	OSPtr<OSData>                aggressivesData;

	AbsoluteTime            userBecameInactiveTime;

                         
	OSPtr<IOService>        pciHostBridgeDevice;
	OSPtr<IOService>        pciHostBridgeDriver;

	OSPtr<IONotifier>       systemCapabilityNotifier;

	typedef struct {
		uint32_t            pid;
		uint32_t            refcount;
	} PMNotifySuspendedStruct;

	uint32_t                pmSuspendedCapacity;
	uint32_t                pmSuspendedSize;
	PMNotifySuspendedStruct *pmSuspendedPIDS;

	OSPtr<OSSet>            preventIdleSleepList;
	OSPtr<OSSet>            preventSystemSleepList;

	OSPtr<const OSSymbol>   _nextScheduledAlarmType;
	clock_sec_t             _nextScheduledAlarmUTC;
	clock_sec_t             _calendarWakeAlarmUTC;
	UInt32                  _scheduledAlarmMask;
	UInt32                  _userScheduledAlarmMask;

#if HIBERNATION
	clock_sec_t             _standbyTimerResetSeconds;
#endif
	volatile uint32_t   swd_lock;                                                
	void  *             swd_buffer;                                                  
	uint32_t            swd_flags;                                    
	void *              swd_compressed_buffer;
	void  *             swd_spindump_buffer;
	thread_t            notifierThread;
	OSPtr<OSObject>     notifierObject;

	OSPtr<IOBufferMemoryDescriptor>    swd_spindump_memDesc;
	OSPtr<IOBufferMemoryDescriptor>    swd_memDesc;

                       
	OSPtr<OSArray>       _systemWakeEventsArray;
	bool                 _acceptSystemWakeEvents;

	         
	IOPMCalendarStruct   _aotWakeTimeCalendar;
	OSPtr<IOTimerEventSource> _aotTimerES;
	clock_sec_t          _aotWakeTimeUTC;
	uint64_t             _aotTestTime;
	uint64_t             _aotTestInterval;
	uint32_t             _aotPendingFlags;
public:
	IOPMAOTMetrics     * _aotMetrics;
	uint8_t              _aotMode;
private:
	uint8_t              _aotNow;
	uint8_t              _aotTasksSuspended;
	uint8_t              _aotExit;
	uint8_t              _aotTimerScheduled;
	uint8_t              _aotReadyToFullWake;
	uint64_t             _aotLastWakeTime;
	uint64_t             _aotWakeTimeContinuous;
	uint64_t             _aotWakePreWindow;
	uint64_t             _aotWakePostWindow;
	uint64_t             _aotLingerTime;

	bool        aotShouldExit(bool checkTimeSet, bool software);
	void        aotExit(bool cps);
	void        aotEvaluate(IOTimerEventSource * timer);
public:
	bool        isAOTMode(void);
private:
	         

	void        updateTasksSuspend(void);
	int         findSuspendedPID(uint32_t pid, uint32_t *outRefCount);

                                     
	IOReturn    privateSleepSystem( uint32_t sleepReason );
	void        reportUserInput( void );
	void        updateUserActivity( void );
	void        setDisableClamShellSleep( bool );
	void        setClamShellSleepDisable(bool disable, uint32_t bitmask);
	bool        checkSystemSleepAllowed( IOOptionBits options,
	    uint32_t sleepReason );
	bool        checkSystemSleepEnabled( void );
	bool        checkSystemCanSleep( uint32_t sleepReason );
	bool        checkSystemCanSustainFullWake( void );

	void        adjustPowerState( bool sleepASAP = false );
	void        setQuickSpinDownTimeout( void );
	void        restoreUserSpinDownTimeout( void );

	bool        shouldSleepOnClamshellClosed(void );
	bool        shouldSleepOnRTCAlarmWake(void );
	void        sendClientClamshellNotification( void );

                                                            
	void        informCPUStateChange( uint32_t type, uint32_t value );

	void        dispatchPowerEvent( uint32_t event, void * arg0, uint64_t arg1 );
	void        handlePowerNotification( UInt32 msg );

	IOReturn    setPMSetting(const OSSymbol *, OSObject *);

	void        startIdleSleepTimer( uint32_t inSeconds );
	void        cancelIdleSleepTimer( void );
	uint32_t    getTimeToIdleSleep( void );

	IOReturn    setAggressiveness(
		unsigned long type,
		unsigned long value,
		IOOptionBits  options );

	void        synchronizeAggressives(
		queue_head_t * services,
		const AggressivesRecord * array,
		int count );

	void        broadcastAggressives(
		const AggressivesRecord * array,
		int count );

	IOReturn    setPMAssertionUserLevels(IOPMDriverAssertionType);

	void        publishSleepWakeUUID( bool shouldPublish );

	void        evaluatePolicy( int stimulus, uint32_t arg = 0 );
	void        requestFullWake( FullWakeReason reason );
	void        willEnterFullWake( void );

	void        evaluateAssertions(IOPMDriverAssertionType newAssertions,
	    IOPMDriverAssertionType oldAssertions);

	void        deregisterPMSettingObject( PMSettingObject * pmso );

	uint32_t    checkForValidDebugData(const char *fname, vfs_context_t *ctx,
	    void *tmpBuf, struct vnode **vp);
	void        getFailureData(thread_t *thread, char *failureStr, size_t strLen);
	void        saveFailureData2File();
	void        tracePhase2String(uint32_t tracePhase, const char **phaseString, const char **description);
	void        sleepWakeDebugMemAlloc();
	void        sleepWakeDebugSpinDumpMemAlloc();
	errno_t     sleepWakeDebugSaveFile(const char *name, char *buf, int len);

	IOReturn    changePowerStateWithOverrideTo( IOPMPowerStateIndex ordinal, IOPMRequestTag tag );
	IOReturn    changePowerStateWithTagToPriv( IOPMPowerStateIndex ordinal, IOPMRequestTag tag );
	IOReturn    changePowerStateWithTagTo( IOPMPowerStateIndex ordinal, IOPMRequestTag tag );

#if HIBERNATION
	bool        getSleepOption( const char * key, uint32_t * option );
	bool        evaluateSystemSleepPolicy( IOPMSystemSleepParameters * p,
	    int phase, uint32_t * hibMode );
	void        evaluateSystemSleepPolicyEarly( void );
	void        evaluateSystemSleepPolicyFinal( void );
#endif                  

	bool        latchDisplayWranglerTickle( bool latch );
	void        setDisplayPowerOn( uint32_t options );

	void        acceptSystemWakeEvents( uint32_t control );
	void        systemDidNotSleep( void );
	void        preventTransitionToUserActive( bool prevent );
	void        setThermalState(OSObject *value);
	void        copySleepPreventersList(OSArray  **idleSleepList, OSArray  **systemSleepList);
	void        copySleepPreventersListWithID(OSArray  **idleSleepList, OSArray  **systemSleepList);
	void        recordRTCAlarm(const OSSymbol *type, OSObject *object);

	                                                                                   
	OSPtr<OSDictionary>   lowLatencyAudioNotifierDict;
	OSPtr<OSNumber>       lowLatencyAudioNotifyStateVal;
	OSPtr<OSNumber>       lowLatencyAudioNotifyTimestampVal;
	OSPtr<const OSSymbol> lowLatencyAudioNotifyStateSym;
	OSPtr<const OSSymbol> lowLatencyAudioNotifyTimestampSym;
#endif                         
};

#ifdef XNU_KERNEL_PRIVATE
class IORootParent : public IOService
{
	OSDeclareFinalStructors(IORootParent);

public:
	static void initialize( void );
	virtual OSPtr<OSObject> copyProperty( const char * aKey ) const APPLE_KEXT_OVERRIDE;
	bool start( IOService * nub ) APPLE_KEXT_OVERRIDE;
	void shutDownSystem( void );
	void restartSystem( void );
	void sleepSystem( void );
	void dozeSystem( void );
	void sleepToDoze( void );
	void wakeSystem( void );
};
#endif                         

#endif                          
