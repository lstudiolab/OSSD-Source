/*
 * Copyright (c) 2002-2020 Apple Computer, Inc. All rights reserved.
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
#ifndef _IOKIT_IOPMPRIVATE_H
#define _IOKIT_IOPMPRIVATE_H

#include <IOKit/pwr_mgt/IOPM.h>

                                           
                                                                  
                                                           
   
#define kIOPMEventTypeIntermediateFlag              0x10000000

                         
                                                                       
   
enum {
    kIOPMEventTypeUndefined                     = 0,

                                     
       
    kIOPMEventTypeSetPowerStateImmediate        = 1001,
    kIOPMEventTypeSetPowerStateDelayed          = 1002,
    kIOPMEventTypePSWillChangeTo                = 1003,
    kIOPMEventTypePSDidChangeTo                 = 1004,
    kIOPMEventTypeAppResponse                   = 1005,


                                               
                                      
       
    kIOPMEventTypeSleep                         = 2001,
    kIOPMEventTypeSleepDone                     = 2002,
    kIOPMEventTypeWake                          = 3001,
    kIOPMEventTypeWakeDone                      = 3002,
    kIOPMEventTypeDoze                          = 4001,
    kIOPMEventTypeDozeDone                      = 4002,
    kIOPMEventTypeUUIDSet                       = 6001,
    kIOPMEventTypeUUIDClear                     = 6002,

                                                                
                                                                                     
                                                                              
                                                                                        
      
                                                                                        
                                                         
       
    kIOPMEventTypeAppNotificationsFinished      = 501 | kIOPMEventTypeIntermediateFlag,
    kIOPMEventTypeDriverNotificationsFinished   = 502 | kIOPMEventTypeIntermediateFlag,
    kIOPMEventTypeCalTimeChange                 = 503 | kIOPMEventTypeIntermediateFlag
};

                                                                              
  
                                                
  
                                                           
  
                                                                               

                                  
                                                                              
                                         
   
#define kIOPMMessageUserIsActiveChanged \
                iokit_family_msg(sub_iokit_powermanagement, 0x400)

  
                                                                                
   
#define kIOPMMessageLastCallBeforeSleep \
                iokit_family_msg(sub_iokit_powermanagement, 0x410)

#define kIOPMMessageIdleSleepPreventers \
                iokit_family_msg(sub_iokit_powermanagement, 0x420)

#define kIOPMMessageSystemSleepPreventers \
                iokit_family_msg(sub_iokit_powermanagement, 0x430)

#define kIOPMMessageLaunchBootSpinDump \
                iokit_family_msg(sub_iokit_powermanagement, 0x440)

#define kIOPMMessageProModeStateChange \
                iokit_family_msg(sub_iokit_powermanagement, 0x450)

#define kIOPMMessageRequestUserActive \
                iokit_family_msg(sub_iokit_powermanagement, 0x460)

                           
                                                                                        
   
enum {
    kIOPMSleepReasonClamshell                   = 101,
    kIOPMSleepReasonPowerButton                 = 102,
    kIOPMSleepReasonSoftware                    = 103,
    kIOPMSleepReasonOSSwitchHibernate           = 104,
    kIOPMSleepReasonIdle                        = 105,
    kIOPMSleepReasonLowPower                    = 106,
    kIOPMSleepReasonThermalEmergency            = 107,
    kIOPMSleepReasonMaintenance                 = 108,
    kIOPMSleepReasonSleepServiceExit            = 109,
    kIOPMSleepReasonDarkWakeThermalEmergency    = 110,
    kIOPMSleepReasonNotificationWakeExit        = 111
};

  
                                                                         
   
#define kIOPMClamshellSleepKey                      "Clamshell Sleep"
#define kIOPMPowerButtonSleepKey                    "Power Button Sleep"
#define kIOPMSoftwareSleepKey                       "Software Sleep"
#define kIOPMOSSwitchHibernationKey                 "OS Switch Sleep"
#define kIOPMIdleSleepKey                           "Idle Sleep"
#define kIOPMLowPowerSleepKey                       "Low Power Sleep"
#define kIOPMThermalEmergencySleepKey               "Thermal Emergency Sleep"
#define kIOPMSleepServiceExitKey                    "Sleep Service Back to Sleep"
#define kIOPMDarkWakeThermalEmergencyKey            "Dark Wake Thermal Emergency"
#define kIOPMNotificationWakeExitKey                "Notification Wake Back to Sleep"

                            
                                   
                                      
                                                                                 
                                                                   
                                             
  
#define kIOPMPSRestrictedModeKey                    "RestrictedMode"

                                                       
#define kIOPMPSAdapterDetailsIsWirelessKey          "IsWireless"

#pragma mark Stray Bitfields
                                               
                     

enum {
    kIOPMSetValue                   = (1<<16),
                                                                       
    kIOPMSetDesktopMode             = (1<<17),
                                        
    kIOPMSetACAdaptorConnected      = (1<<18)
};

                                                                               
                                                                               

  
                        
   

                                                
  
                                                                            
                                                   
  
   
#define kIOPMSleepServiceScheduleImmediate     "SleepServiceImmediate"

                                                       
  
                                                                            
                                         
   
#define kIOPMSettingSleepServiceWakeCalendarKey     "SleepServiceWakeCalendarKey"

                                    
  
                                                                 
   
enum {
    kPMCalendarTypeInvalid = 0,
    kPMCalendarTypeMaintenance = 1,
    kPMCalendarTypeSleepService = 2
};


                                      
                                                                   
                                                                                    
                        
   
#define kIOPMStateConsoleShutdown   "ConsoleShutdown"

                       
                                                                       
   
enum {
                                          
                                                                       
                            
   
    kIOPMStateConsoleShutdownNone   = 1,
                                              
                                                                                                       
                                      
   
    kIOPMStateConsoleShutdownPossible = 2,
                                              
                                                                          
                                                                       
   
    kIOPMStateConsoleShutdownUnderway = 3,
                                             
                                                     
                                                               
   
    kIOPMStateConsoleShutdownCertain = 4,
                                               
                                                                      
                                                           
   
    kIOPMStateConsoleSULogoutInitiated = 5
};

                                         
                                                                  
                                                                                     
                                                                   
   
#define kIOPMSettingSilentRunningKey    "SilentRunning"
#define kIOPMFeatureSilentRunningKey    kIOPMSettingSilentRunningKey

                           
                                                                               
                                                        
   
enum {
    kIOPMSilentRunningModeOn = 0x00000001
};

                                               
                                                                                      
                                                                                    
                        
   
#define kIOPMSettingLowLatencyAudioModeKey          "LowLatencyAudioMode"

                                                                               
                                                                               

                                
                                                               
   
enum {
    kIOPMStatsHibernateImageWrite         = 1,
    kIOPMStatsHibernateImageRead,
    kIOPMStatsDriversNotify,
    kIOPMStatsApplicationNotify,
    kIOPMStatsLateDriverAcknowledge,
    kIOPMStatsLateAppAcknowledge,

                                                                 
                                                                   
                                                                      
                 
    kIOPMStatsEventStartFlag              = (1 << 24),
    kIOPMStatsEventStopFlag               = (1 << 25)
};

                                              
#define kIOPMSleepStatisticsKey                 "SleepStatistics"
#define kIOPMSleepStatisticsAppsKey             "AppStatistics"
#define kIOPMIdleSleepPreventersKey             "IdleSleepPreventers"
#define kIOPMSystemSleepPreventersKey           "SystemSleepPreventers"
#define kIOPMIdleSleepPreventersWithIDKey       "IdleSleepPreventersWithID"
#define kIOPMSystemSleepPreventersWithIDKey     "SystemSleepPreventersWithID"

                                  
#define kIOPMStatsNameKey                       "Name"
#define kIOPMStatsPIDKey                        "Pid"
#define kIOPMStatsTimeMSKey                     "TimeMS"
#define kIOPMStatsApplicationResponseTypeKey    "ResponseType"
#define kIOPMStatsMessageTypeKey                "MessageType"
#define kIOPMStatsPowerCapabilityKey            "PowerCaps"
#define kIOPMStatsSystemTransitionKey           "TransitionType"

                                                                                   
                                          
#define kIOPMStatsResponseTimedOut      "ResponseTimedOut"
#define kIOPMStatsResponseCancel        "ResponseCancel"
#define kIOPMStatsResponseSlow          "ResponseSlow"
#define kIOPMStatsResponsePrompt        "ResponsePrompt"
#define kIOPMStatsDriverPSChangeSlow    "DriverPSChangeSlow"

struct PMStatsBounds{
    uint64_t start;
    uint64_t stop;
};
typedef struct {

    struct PMStatsBounds    hibWrite;
    struct PMStatsBounds    hibRead;
                                  
                                 
                               
                              
                                 
                                    
} PMStatsStruct;

                                                                               

                            
  
                                                                         
                                                                                
                                                                                 
                                                                                      
                                                     
  
                                                                                                
                                                                                            
                                  
   

enum {
                                                     
                                                              
  
                                                           
                                                                    
                                                 
   
    kIOPMTracePointSystemUp                     = 0,

                                                             
                            
  
                                                                
                                                                
   
    kIOPMTracePointSleepStarted                 = 0x10,

                                                                  
                                                               
                              
  
                                                                       
                                                                                 
                                                              
   
    kIOPMTracePointSleepApplications            = 0x11,

                                                                     
                                                                   
                                                         
  
                                                                                 
                                                      
   
    kIOPMTracePointSleepPriorityClients         = 0x12,

                                                                         
                                                                               
  
                                                                      
                                                                            
                                                          
   
    kIOPMTracePointSleepWillChangeInterests     = 0x13,

                                                                       
                                                                          
  
                                                                            
                                                                          
                                                                          
                                                                         
                              
   
    kIOPMTracePointSleepPowerPlaneDrivers       = 0x14,

                                                                        
                                                                              
  
                                                                      
                                                                            
                                                          
   
    kIOPMTracePointSleepDidChangeInterests      = 0x15,

                                                                       
                                                                
  
                                                                              
                                                                 
                                              
   
    kIOPMTracePointSleepCapabilityClients       = 0x16,

                                                                     
                                                                      
   
    kIOPMTracePointSleepPlatformActions         = 0x17,

                                                          
                                               
  
                                                  
                                                          
   
    kIOPMTracePointSleepCPUs                    = 0x18,

                                                                    
                                                                       
   
    kIOPMTracePointSleepPlatformDriver          = 0x19,

                                                          
                                             
   
    kIOPMTracePointHibernate                    = 0x1a,

                                                            
                                                                     
                                                                          
                                 
  
                                                                           
                                                                           
                                                                        
  
                                                                         
                                                                                
                         
   
    kIOPMTracePointSystemSleep                  = 0x1f,

                                                                   
                                                                      
   
    kIOPMTracePointWakePlatformDriver           = 0x21,

                                                                    
                                                                     
   
    kIOPMTracePointWakePlatformActions          = 0x22,

                                                         
                                                 
   
    kIOPMTracePointWakeCPUs                     = 0x23,

                                                                       
                                                                       
                                                                     
                              
  
                                                     
                                                       
                                              
   
    kIOPMTracePointWakeWillPowerOnClients       = 0x24,

                                                                        
                                                                               
  
                                                                      
                                                                            
                                                          
   
    kIOPMTracePointWakeWillChangeInterests      = 0x25,

                                                                       
                                                                              
  
                                                                      
                                                                            
                                                          
   
    kIOPMTracePointWakeDidChangeInterests       = 0x26,

                                                                      
                                                                         
  
                                                                            
                                                                          
                                                                          
                                                                         
                              
   
    kIOPMTracePointWakePowerPlaneDrivers        = 0x27,

                                                                      
                                                                           
                                    
  
                                                                               
                                                      
   
    kIOPMTracePointWakeCapabilityClients        = 0x28,

                                                                 
                                                        
  
                                                                              
                                                           
   
    kIOPMTracePointWakeApplications             = 0x29,

                                        
                                                                                 
                                                                                        
                                                                         
  
                                                                                 
                                                                                 
                                                                                        
   
    kIOPMTracePointSystemLoginwindowPhase       = 0x30,

                                                              
                                                           
  
                                               
                                                                             
   
    kIOPMTracePointDarkWakeEntry                = 0x31,

                                                             
                                                           
  
                                               
                                                                             
   
    kIOPMTracePointDarkWakeExit                 = 0x32
};

                                                                               

  
                               
  
                                                                      
                                             
  
                       
                       
                        
  
                                                                 
                    
   

                                                         
enum {
    kIOPMLoginWindowProgress    = 1,
    kIOPMCoreDisplayProgress    = 2,
    kIOPMCoreGraphicsProgress   = 3
};

enum {
    kIOPMLoginWindowProgressMask    = 0x0f,
    kIOPMCoreDisplayProgressMask    = 0xff,
    kIOPMCoreGraphicsProgressMask   = 0xff
};

  
                                                                                 
                                                                  
   
#define kIOPMLoginWindowProgressKey             "LoginWindowProgress"

  
                                                                                 
                                                                  
   
#define kIOPMCoreDisplayProgressKey             "CoreDisplayProgress"

  
                                                                                   
                                                                  
   
#define kIOPMCoreGraphicsProgressKey            "CoreGraphicsProgress"

                                                                                        
#define kIOPMDynamicStoreSleepFailureKey        "SleepFailure"

                                                                               

                       
#define kIOPMSleepWakeFailureKey            "PMFailurePhase"
#define kIOPMSleepWakeFailureCodeKey        "PMStatusCode"
#define kIOPMSleepWakeFailureLoginKey       "LWFailurePhase"
#define kIOPMSleepWakeFailureUUIDKey        "UUID"
#define kIOPMSleepWakeFailureDateKey        "Date"
#define kIOPMSleepWakeWdogRebootKey         "SWWdogTriggeredRestart"
#define kIOPMSleepWakeWdogLogsValidKey      "SWWdogLogsValid"

                                                                              
  
                                    
  
                                                                               

                              
                                                   
   
#define kIOPMFeatureAutoPowerOffKey         "AutoPowerOff"

                              
                                          
                          
                                            
                                             
                                                                   
   
#define kIOPMAutoPowerOffEnabledKey         "AutoPowerOff Enabled"

                            
                                                                          
                                                                          
                            
   
#define kIOPMAutoPowerOffDelayKey           "AutoPowerOff Delay"

                            
                                                                           
                                                                           
                                               
   
#define kIOPMAutoPowerOffTimerKey           "AutoPowerOff Timer"

                         
                                                                           
                                                                    
                                               
   
#define kIOPMDeepSleepTimerKey                "Standby Timer"

                                 
                                                                              
              
   
#define kIOPMUserWakeAlarmScheduledKey      "UserWakeAlarmScheduled"

                            
                                                                          
                                                      
   
#define kIOPMDeepIdleSupportedKey           "IOPMDeepIdleSupported"

                                
                                                                            
                                                                          
                                                                           
                            
   
#define kIOPMUserTriggeredFullWakeKey       "IOPMUserTriggeredFullWake"

                       
                                                                      
   
#define kIOPMUserIsActiveKey                "IOPMUserIsActive"

                           
                                                                   
   
#define kIOPMDriverWakeEventsKey            "IOPMDriverWakeEvents"

                                                                              
  
                                       
  
                                                                               

#define kIOPMWakeEventTimeKey               "Time"
#define kIOPMWakeEventFlagsKey              "Flags"
#define kIOPMWakeEventReasonKey             "Reason"
#define kIOPMWakeEventDetailsKey            "Details"

                         
                                            
   
#define kIOPMFeatureProModeKey              "ProMode"

                                                                              
  
                                                                      
  
                                                                               

#define kIOPMWakeEventSource                0x00000001

                                                                              
  
           
  
                                                                               

                                                          
#define kIOPMWakeEventAOTExit                   0x00000002

                                            
#define kIOPMWakeEventAOTPossibleExit           0x00000004

                                                          
#define kIOPMWakeEventAOTConfirmedPossibleExit  0x00000008

                                           
#define kIOPMWakeEventAOTRejectedPossibleExit   0x00000010

                                           
#define kIOPMWakeEventAOTExpiredPossibleExit    0x00000020

#define kIOPMWakeEventAOTFlags \
                                 (kIOPMWakeEventAOTExit \
                                | kIOPMWakeEventAOTPossibleExit \
                                | kIOPMWakeEventAOTConfirmedPossibleExit \
                                | kIOPMWakeEventAOTRejectedPossibleExit \
                                | kIOPMWakeEventAOTExpiredPossibleExit)

#define kIOPMWakeEventAOTPossibleFlags \
                                 (kIOPMWakeEventAOTPossibleExit \
                                | kIOPMWakeEventAOTConfirmedPossibleExit \
                                | kIOPMWakeEventAOTRejectedPossibleExit \
                                | kIOPMWakeEventAOTExpiredPossibleExit)

#define kIOPMWakeEventAOTPerCycleFlags \
                                 (kIOPMWakeEventAOTPossibleExit \
                                | kIOPMWakeEventAOTRejectedPossibleExit \
                                | kIOPMWakeEventAOTExpiredPossibleExit)

#define kIOPMWakeEventAOTExitFlags \
                                 (kIOPMWakeEventAOTExit \
                                | kIOPMWakeEventAOTConfirmedPossibleExit)

enum {
	kIOPMAOTModeMask          = 0x000000ff,
    kIOPMAOTModeEnable        = 0x00000001,
    kIOPMAOTModeCycle         = 0x00000002,
    kIOPMAOTModeAddEventFlags = 0x00000004,
    kIOPMAOTModeRespectTimers = 0x00000008,
    kIOPMAOTModeDefault       = (kIOPMAOTModeEnable | kIOPMAOTModeAddEventFlags | kIOPMAOTModeRespectTimers)
};

enum {
    kIOPMAOTMetricsKernelWakeCountMax = 24
};

struct IOPMAOTMetrics
{
    uint32_t sleepCount;
    uint32_t possibleCount;
    uint32_t confirmedPossibleCount;
    uint32_t rejectedPossibleCount;
    uint32_t expiredPossibleCount;
    uint32_t noTimeSetCount;
    uint32_t rtcAlarmsCount;
    uint32_t softwareRequestCount;
    uint64_t totalTime;

	char     kernelWakeReason[kIOPMAOTMetricsKernelWakeCountMax][64];
	                              
    uint64_t kernelSleepTime[kIOPMAOTMetricsKernelWakeCountMax];
    uint64_t kernelWakeTime[kIOPMAOTMetricsKernelWakeCountMax];
};

#define kIOPMAOTPowerKey    "aot-power"

                                                                              
  
            
  
                                                                               

                                                                          
                                                                         
                                                                       
   
#define kIOPMDarkWakeMaxPowerStateKey       "IOPMDarkWakeMaxPowerState"

                                                                              
  
                      
  
                                                                               

#define kIOPMSystemSleepPolicySignature     0x54504c53
#define kIOPMSystemSleepPolicyVersion       2

   
                                   
                                                
                                                                          
                                                                            
                                                                           
                                                                            
                                                   
   
#define kIOPMSystemSleepTypeKey             "IOPMSystemSleepType"

struct IOPMSystemSleepPolicyVariables
{
    uint32_t    signature;                                                    
    uint32_t    version;                                                    

    uint64_t    currentCapability;                                           
    uint64_t    highestCapability;                                           

    uint64_t    sleepFactors;                                   
    uint32_t    sleepReason;                                    
    uint32_t    sleepPhase;                                            
    uint32_t    hibernateMode;                                       

    uint32_t    standbyDelay;                                          
    uint32_t    standbyTimer;                                          
    uint32_t    poweroffDelay;                                               
    uint32_t    scheduledAlarms;                                               
    uint32_t    poweroffTimer;                                               

    uint32_t    reserved[49];                                      
};

enum {
    kIOPMAlarmBitDebugWake                  = 0x01,
    kIOPMAlarmBitCalendarWake               = 0x02,
    kIOPMAlarmBitMaintenanceWake            = 0x04,
    kIOPMAlarmBitSleepServiceWake           = 0x08
};

enum {
    kIOPMSleepPhase0 = 0,
    kIOPMSleepPhase1,
    kIOPMSleepPhase2
};

                           
enum {
    kIOPMSleepFactorSleepTimerWake          = 0x00000001ULL,
    kIOPMSleepFactorLidOpen                 = 0x00000002ULL,
    kIOPMSleepFactorACPower                 = 0x00000004ULL,
    kIOPMSleepFactorBatteryLow              = 0x00000008ULL,
    kIOPMSleepFactorStandbyNoDelay          = 0x00000010ULL,
    kIOPMSleepFactorStandbyForced           = 0x00000020ULL,
    kIOPMSleepFactorStandbyDisabled         = 0x00000040ULL,
    kIOPMSleepFactorUSBExternalDevice       = 0x00000080ULL,
    kIOPMSleepFactorBluetoothHIDDevice      = 0x00000100ULL,
    kIOPMSleepFactorExternalMediaMounted    = 0x00000200ULL,
    kIOPMSleepFactorThunderboltDevice       = 0x00000400ULL,
    kIOPMSleepFactorRTCAlarmScheduled       = 0x00000800ULL,
    kIOPMSleepFactorMagicPacketWakeEnabled  = 0x00001000ULL,
    kIOPMSleepFactorHibernateForced         = 0x00010000ULL,
    kIOPMSleepFactorAutoPowerOffDisabled    = 0x00020000ULL,
    kIOPMSleepFactorAutoPowerOffForced      = 0x00040000ULL,
    kIOPMSleepFactorExternalDisplay         = 0x00080000ULL,
    kIOPMSleepFactorNetworkKeepAliveActive  = 0x00100000ULL,
    kIOPMSleepFactorLocalUserActivity       = 0x00200000ULL,
    kIOPMSleepFactorHibernateFailed         = 0x00400000ULL,
    kIOPMSleepFactorThermalWarning          = 0x00800000ULL,
    kIOPMSleepFactorDisplayCaptured         = 0x01000000ULL
};

                     
enum {
    kIOPMSleepTypeInvalid                   = 0,
    kIOPMSleepTypeAbortedSleep              = 1,
    kIOPMSleepTypeNormalSleep               = 2,
    kIOPMSleepTypeSafeSleep                 = 3,
    kIOPMSleepTypeHibernate                 = 4,
    kIOPMSleepTypeStandby                   = 5,
    kIOPMSleepTypePowerOff                  = 6,
    kIOPMSleepTypeDeepIdle                  = 7,
    kIOPMSleepTypeLast                      = 8
};

                     
enum {
    kIOPMSleepFlagDisableHibernateAbort     = 0x00000001,
    kIOPMSleepFlagDisableUSBWakeEvents      = 0x00000002,
    kIOPMSleepFlagDisableBatlowAssertion    = 0x00000004,
    kIOPMSleepFlagDisableS4WakeSources      = 0x00000008
};

                     
enum {
    kIOPMWakeEventLidOpen                   = 0x00000001,
    kIOPMWakeEventLidClose                  = 0x00000002,
    kIOPMWakeEventACAttach                  = 0x00000004,
    kIOPMWakeEventACDetach                  = 0x00000008,
    kIOPMWakeEventCDInsert                  = 0x00000010,
    kIOPMWakeEventCDEject                   = 0x00000020,
    kIOPMWakeEventHPDAttach                 = 0x00000040,
    kIOPMWakeEventHPDDetach                 = 0x00000080,
    kIOPMWakeEventPowerButton               = 0x00000100,
    kIOPMWakeEventG3PowerOn                 = 0x00000200,
    kIOPMWakeEventUserPME                   = 0x00000400,
    kIOPMWakeEventSleepTimer                = 0x00000800,
    kIOPMWakeEventBatteryLow                = 0x00001000,
    kIOPMWakeEventDarkPME                   = 0x00002000,
    kIOPMWakeEventWifi                      = 0x00004000,
    kIOPMWakeEventRTCSystem                 = 0x00008000,                         
    kIOPMWakeEventUSBCPlugin                = 0x00010000,                 
    kIOPMWakeEventHID                       = 0x00020000,
    kIOPMWakeEventBluetooth                 = 0x00040000,
    kIOPMWakeEventDFR                       = 0x00080000,
    kIOPMWakeEventSD                        = 0x00100000,            
    kIOPMWakeEventLANWake                   = 0x00200000,                
    kIOPMWakeEventLANPlugin                 = 0x00400000,                         
    kIOPMWakeEventThunderbolt               = 0x00800000,
    kIOPMWakeEventRTCUser                   = 0x01000000,                            
};

   
                                         
                                                           
                                                                           
                                                                           
                                                                  
   
#define kIOPMSystemSleepParametersKey       "IOPMSystemSleepParameters"
#define kIOPMSystemSleepParametersVersion   2

struct IOPMSystemSleepParameters
{
    uint16_t    version;
    uint16_t    reserved1;
    uint32_t    sleepType;
    uint32_t    sleepFlags;
    uint32_t    ecWakeEvents;
    uint32_t    ecWakeTimer;
    uint32_t    ecPoweroffTimer;
    uint32_t    reserved2[10];
} __attribute__((packed));


  
                                 
   
typedef struct {
   uint32_t signature;
   uint32_t alloc_size;
   uint32_t          crc;                                                    
   uint32_t          spindump_offset;                                                
   uint32_t          spindump_size;
   uint8_t           is_osx_watchdog;

                                                              
   char             UUID[44];
   char             spindump_status[24];                        
   char             PMStatusCode[32];
   char             reason[32];
} swd_hdr;


#define SWD_HDR_SIGNATURE       0xdeb8da2a

#define SWD_STACKSHOT_SIZE      (40*PAGE_SIZE)
#define SWD_COMPRESSED_BUFSIZE  (5*PAGE_SIZE)
#define SWD_ZLIB_BUFSIZE        (10*PAGE_SIZE)
#define SWD_STACKSHOT_VAR_PREFIX    "sleepwake_diags"

#define SWD_SPINDUMP_SIZE          (256*1024)
#define SWD_INITIAL_SPINDUMP_SIZE  ((SWD_SPINDUMP_SIZE/2)-sizeof(swd_hdr))
#define SWD_MAX_STACKSHOTS          (10)

                       
#define SWD_WDOG_ENABLED        0x01
#define SWD_BOOT_BY_SW_WDOG     0x02
#define SWD_BOOT_BY_OSX_WDOG    0x04
#define SWD_VALID_LOGS          0x08
#define SWD_LOGS_IN_FILE        0x10
#define SWD_LOGS_IN_MEM         0x20
#define SWD_PWR_BTN_STACKSHOT   0x30

#define SWD_DATA_CRC_ERROR      0x010000
#define SWD_BUF_SIZE_ERROR      0x020000
#define SWD_HDR_SIZE_ERROR      0x040000
#define SWD_FILEOP_ERROR        0x080000
#define SWD_HDR_SIGNATURE_ERROR 0x100000
#define SWD_INTERNAL_FAILURE    0x200000


                                                                        
#define kOSWatchdogStacksFilename           "/var/log/OSXWatchdogStacks.gz"
#define kOSWatchdogFailureStringFile        "/var/log/OSWatchdogFailureString.txt"
#define kSleepWakeStacksFilename            "/var/log/SleepWakeStacks.gz"
#define kSleepWakeFailureStringFile         "/var/log/SleepWakeFailureString.txt"


                                     
#define kSleepCntChID IOREPORT_MAKEID('S','l','e','e','p','C','n','t')
#define kDarkWkCntChID IOREPORT_MAKEID('G','U','I','W','k','C','n','t')
#define kUserWkCntChID IOREPORT_MAKEID('D','r','k','W','k','C','n','t')

  
                                                                            
   
#define kAssertDelayBcktCnt     11
#define kAssertDelayBcktSize    3
#define kAssertDelayChID IOREPORT_MAKEID('r','d','A','s','r','t','D','l')

  
                                                                    
   
#define kSleepDelaysBcktCnt     13
#define kSleepDelaysBcktSize    10
#define kSleepDelaysChID IOREPORT_MAKEID('r','d','S','l','p','D','l','y')

                            
#define kSleepOptionDisplayCapturedModeKey         "DisplayCapturedMode"


#if defined(KERNEL) && defined(__cplusplus)

   
                                                   
                                                                             
                                                                             
                                                                            
                                                                          
                
   
#define kIOPMInstallSystemSleepPolicyHandlerKey \
        "IOPMInstallSystemSleepPolicyHandler"

typedef IOReturn (*IOPMSystemSleepPolicyHandler)(
        void * target,
        const IOPMSystemSleepPolicyVariables * vars,
        IOPMSystemSleepParameters * params );

#endif             

                                                                              
  
                      
  
                                                                               

                          
                                                                     
                             
   
#define kIOPMPerformanceWarningKey          "Performance_Warning"

                                
enum {
  kIOPMPerformanceNormal    = 0,
  kIOPMPerformanceWarning   = 100
};

#endif                             
