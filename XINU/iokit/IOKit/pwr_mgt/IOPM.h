/*
 * Copyright (c) 1998-2005 Apple Computer, Inc. All rights reserved.
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
#ifndef _IOKIT_IOPM_H
#define _IOKIT_IOPM_H

#include <IOKit/IOTypes.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOReturn.h>

                  
                                                                                                                  
                                                                                                                                                                                                                                    
  
                                                                                                                                   
   

enum {
	kIOPMMaxPowerStates = 10,
	IOPMMaxPowerStates = kIOPMMaxPowerStates
};

                        
                                                                                                                                           
                                                                                                                                                                             
  
                                                                                                                                                                        
  
                                                                                                                                                                                             
  
                                                                                                                                                                                                                                                                                                                                                                  
  
                                                                                                                                                                
  
                                                                                                          
  
                            
                                                                          
                                                                         
  
                                                     
                                                              
                                                                        
                                      
  
                                                                                                                                                          
  
                                    
                                                                                                             
  
                                                                                                                                                                                            
  
                                 
  
                                   
                                                                                                                                                                                                  
  
                                     
                                                                                                                                                                                                  
  
                         
                                                                                                                                                                                                  
  
                           
                                                                                                                                                                                                  
  
                                      
                                                                                                                                                                                                                                                                                
  
                                   
                                                                             
                                                                      
                                                                            
                                                                   
   
typedef unsigned long IOPMPowerFlags;
enum {
	kIOPMPowerOn                    = 0x00000002,
	kIOPMDeviceUsable               = 0x00008000,
	kIOPMLowPower                   = 0x00010000,
#if PRIVATE
	kIOPMAOTPower                   = 0x00020000,
	kIOPMAOTCapability              = kIOPMAOTPower,
#endif              
	kIOPMPreventIdleSleep           = 0x00000040,
	kIOPMSleepCapability            = 0x00000004,
	kIOPMRestartCapability          = 0x00000080,
	kIOPMSleep                      = 0x00000001,
	kIOPMRestart                    = 0x00000080,
	kIOPMInitialDeviceState         = 0x00000100,
	kIOPMRootDomainState            = 0x00000200
};

  
                         
  
                     
                                     
                              
   
enum {
	kIOPMClockNormal                = 0x0004,
	kIOPMClockRunning               = 0x0008,
	kIOPMPreventSystemSleep         = 0x0010,
	kIOPMDoze                       = 0x0400,
	kIOPMChildClamp                 = 0x0080,
	kIOPMChildClamp2                = 0x0200,
	kIOPMNotPowerManaged            = 0x0800
};

  
                            
                                                          
                                                                     
   
enum {
	kIOPMMaxPerformance             = 0x4000,
	kIOPMPassThrough                = 0x0100,
	kIOPMAuxPowerOn                 = 0x0020,
	kIOPMNotAttainable              = 0x0001,
	kIOPMContextRetained            = 0x2000,
	kIOPMConfigRetained             = 0x1000,
	kIOPMStaticPowerValid           = 0x0800,
	kIOPMSoftSleep                  = 0x0400,
	kIOPMCapabilitiesMask =     kIOPMPowerOn | kIOPMDeviceUsable |
	    kIOPMMaxPerformance | kIOPMContextRetained |
	    kIOPMConfigRetained | kIOPMSleepCapability |
	    kIOPMRestartCapability
};

  
                                                   
   
enum {
	IOPMNotAttainable           = kIOPMNotAttainable,
	IOPMPowerOn                 = kIOPMPowerOn,
	IOPMClockNormal             = kIOPMClockNormal,
	IOPMClockRunning            = kIOPMClockRunning,
	IOPMAuxPowerOn              = kIOPMAuxPowerOn,
	IOPMDeviceUsable            = kIOPMDeviceUsable,
	IOPMMaxPerformance          = kIOPMMaxPerformance,
	IOPMContextRetained         = kIOPMContextRetained,
	IOPMConfigRetained          = kIOPMConfigRetained,
	IOPMNotPowerManaged         = kIOPMNotPowerManaged,
	IOPMSoftSleep               = kIOPMSoftSleep
};


enum {
	kIOPMNextHigherState        = 1,
	kIOPMHighestState           = 2,
	kIOPMNextLowerState         = 3,
	kIOPMLowestState            = 4
};

enum {
	IOPMNextHigherState         = kIOPMNextHigherState,
	IOPMHighestState            = kIOPMHighestState,
	IOPMNextLowerState          = kIOPMNextLowerState,
	IOPMLowestState             = kIOPMLowestState
};

                                                          
enum {
	kIOPMBroadcastAggressiveness = 1,
	kIOPMUnidleDevice
};

                                  
enum {
	kIOPMUnknown = 0xFFFF
};

                                                                                
  
                                        
  
                                                                                

                      
                                                           
                          
                                      
                                    
                                                
   
#define kAppleClamshellStateKey             "AppleClamshellState"

                            
                                                       
                          
                                                             
                                                           
                                                     
                                                
   
#define kAppleClamshellCausesSleepKey       "AppleClamshellCausesSleep"

                        
                                                          
                                             
                                                                      
                                         
                                                                    
                                                                      
                      
  
                                                                  
   
 #define kIOPMSleepWakeUUIDKey              "SleepWakeUUID"

                          
                                                          
                
                                                       
                                                            
                              
   
#define kIOPMBootSessionUUIDKey             "BootSessionUUID"

                           
                                         
                          
                                        
                                         
                                                               
   
#define kIOPMDeepSleepEnabledKey            "Standby Enabled"

                         
                                                                          
                                                                     
                                                                    
                                                        
   
#define kIOPMDeepSleepDelayKey              "Standby Delay"

                             
                                                                          
                                                                            
                                                                      
                                                                      
                                        
   
#define kIOPMDeepSleepDelayHighKey          "High Standby Delay"

                              
                                                                           
                                                                          
                                                                       
                                        
   
#define kIOPMStandbyBatteryThresholdKey     "Standby Battery Threshold"

                                
                                                                      
                          
                                                            
                                                           
                                                           
   
#define kIOPMDestroyFVKeyOnStandbyKey       "DestroyFVKeyOnStandby"

                                                                                
  
                                                        
  
                                                                                

                                
                                                                       
                                                                        
                                                                        
                                                                         
                                                                           
                                                                         
                                                                          
                                                                       
                                                                        
                                                                        
                                                                       
                                                                          
                                                                         
                                   
   
#define kIOPMResetPowerStateOnWakeKey       "IOPMResetPowerStateOnWake"

                                                                                
  
                       
  
                                                                                

                                        
                                                  
   
enum {
	                              
                                                                      
                                                      
    
	kIOPMDriverAssertionCPUBit                      = 0x01,

	                                                 
                                                                     
                 
    
	kIOPMDriverAssertionPreventSystemIdleSleepBit   = 0x02,

	                                            
                                                                             
    
	kIOPMDriverAssertionUSBExternalDeviceBit        = 0x04,

	                                                   
                                                                           
    
	kIOPMDriverAssertionBluetoothHIDDevicePairedBit = 0x08,

	                                               
                                                                       
    
	kIOPMDriverAssertionExternalMediaMountedBit     = 0x10,

	                                    
                             
    
	kIOPMDriverAssertionReservedBit5                = 0x20,

	                                              
                                                                            
    
	kIOPMDriverAssertionPreventDisplaySleepBit      = 0x40,

	                                    
                                
    
	kIOPMDriverAssertionReservedBit7                = 0x80,

	                                                 
                                                                       
    
	kIOPMDriverAssertionMagicPacketWakeEnabledBit   = 0x100,

	                                                 
                                                                   
                                   
    
	kIOPMDriverAssertionNetworkKeepAliveActiveBit   = 0x200
};

                           
                                                                        
                                                           
                                                                        
                                                         
                                                                                 
   
#define kIOPMAssertionsDriverKey            "DriverPMAssertions"

                           
                                                                        
                                                           
                                                                        
                                                         
                                                                                 
   
#define kIOPMAssertionsDriverDetailedKey    "DriverPMAssertionsDetailed"

                                                                                
  
                                                   
  
                                                                                  
                             
  
   
#define kIOPMDriverAssertionIDKey               "ID"
#define kIOPMDriverAssertionCreatedTimeKey      "CreatedTime"
#define kIOPMDriverAssertionModifiedTimeKey     "ModifiedTime"
#define kIOPMDriverAssertionOwnerStringKey      "Owner"
#define kIOPMDriverAssertionOwnerServiceKey     "ServicePtr"
#define kIOPMDriverAssertionRegistryEntryIDKey  "RegistryEntryID"
#define kIOPMDriverAssertionLevelKey            "Level"
#define kIOPMDriverAssertionAssertedKey         "Assertions"

                                                                                
  
                                        
  
                                                                  
                     
  
                                                                                

                                   
                                                                     
                                                                             
                                                                            
                                                                        
                                                                          
                                                                                
   
enum {
	kClamshellStateBit = (1 << 0),
	kClamshellSleepBit = (1 << 1)
};

#define kIOPMMessageClamshellStateChange   \
	        iokit_family_msg(sub_iokit_powermanagement, 0x100)

                            
                                                                                
                                                                            
                                  
                                                   
   
#define kIOPMMessageFeatureChange           \
	        iokit_family_msg(sub_iokit_powermanagement, 0x110)

                                     
                                                                      
                                                            
                                                           
                                                
                                           
   
enum {
	kInflowForciblyEnabledBit = (1 << 0)
};

                                             
                                                                      
   
#define kIOPMMessageInternalBatteryFullyDischarged  \
	        iokit_family_msg(sub_iokit_powermanagement, 0x120)

                                       
                                                                             
                         
   
#define kIOPMMessageSystemPowerEventOccurred  \
	        iokit_family_msg(sub_iokit_powermanagement, 0x130)

                                  
                                                                             
                                                                       
   
#define kIOPMMessageSleepWakeUUIDChange  \
	        iokit_family_msg(sub_iokit_powermanagement, 0x140)

                               
                                                                              
                                 
   
#define kIOPMMessageSleepWakeUUIDSet                    ((void *)1)

                                   
                                                                              
                                     
   
#define kIOPMMessageSleepWakeUUIDCleared                ((void *)NULL)

                                       
                                                       
   
#define kIOPMMessageDriverAssertionsChanged  \
	        iokit_family_msg(sub_iokit_powermanagement, 0x150)

                                        
                                                            
                                                                       
   
#define kIOPMMessageDarkWakeThermalEmergency \
	        iokit_family_msg(sub_iokit_powermanagement, 0x160)

                                                                                
  
                                       
                                                      
  
                                                      
                                                       
  
                                                        
                                                                                
enum {
	kIOPMSleepNow                 = (1 << 0),                           
	kIOPMAllowSleep               = (1 << 1),                   
	kIOPMPreventSleep             = (1 << 2),                          
	kIOPMPowerButton              = (1 << 3),                           
	kIOPMClamshellClosed          = (1 << 4),                       
	kIOPMPowerEmergency           = (1 << 5),                          
	kIOPMDisableClamshell         = (1 << 6),                                    
	kIOPMEnableClamshell          = (1 << 7),                             
	kIOPMProcessorSpeedChange     = (1 << 8),                             
	kIOPMOverTemp                 = (1 << 9),                         
	kIOPMClamshellOpened          = (1 << 10),                       
	kIOPMDWOverTemp               = (1 << 11),                                    
	kIOPMPowerButtonUp            = (1 << 12),                  
	kIOPMProModeEngaged           = (1 << 13),                         
	kIOPMProModeDisengaged        = (1 << 14)                         
};


                                                                                
  
                                
  
                                                                                
enum {
	kIOPMNoErr                  = 0,

	                                                                  
	                                                            
	                                                            
	kIOPMAckImplied             = 0,

	             
	kIOPMWillAckLater           = 1,

	                                                    
	                                        
	kIOPMBadSpecification       = 4,

	                                                    
	                                               
	kIOPMNoSuchState            = 5,

	             
	kIOPMCannotRaisePower       = 6,

	             
	kIOPMParameterError         = 7,

	                                                   
	                                     
	kIOPMNotYetInitialized      = 8,

	                                    
	IOPMNoErr = kIOPMNoErr,
	IOPMAckImplied = kIOPMAckImplied,
	IOPMWillAckLater = kIOPMWillAckLater,
	IOPMBadSpecification = kIOPMBadSpecification,
	IOPMNoSuchState = kIOPMNoSuchState,
	IOPMCannotRaisePower = kIOPMCannotRaisePower,
	IOPMParameterError = kIOPMParameterError,
	IOPMNotYetInitialized = kIOPMNotYetInitialized
};


                                            
                                                                              
        
#define kIOPMPSExternalConnectedKey                 "ExternalConnected"
#define kIOPMPSExternalChargeCapableKey             "ExternalChargeCapable"
#define kIOPMPSBatteryInstalledKey                  "BatteryInstalled"
#define kIOPMPSIsChargingKey                        "IsCharging"
#define kIOPMFullyChargedKey                        "FullyCharged"
#define kIOPMPSAtWarnLevelKey                       "AtWarnLevel"
#define kIOPMPSAtCriticalLevelKey                   "AtCriticalLevel"
#define kIOPMPSCurrentCapacityKey                   "CurrentCapacity"
#define kIOPMPSMaxCapacityKey                       "MaxCapacity"
#define kIOPMPSDesignCapacityKey                    "DesignCapacity"
#define kIOPMPSTimeRemainingKey                     "TimeRemaining"
#define kIOPMPSAmperageKey                          "Amperage"
#define kIOPMPSVoltageKey                           "Voltage"
#define kIOPMPSCycleCountKey                        "CycleCount"
#define kIOPMPSMaxErrKey                            "MaxErr"
#define kIOPMPSAdapterInfoKey                       "AdapterInfo"
#define kIOPMPSLocationKey                          "Location"
#define kIOPMPSErrorConditionKey                    "ErrorCondition"
#define kIOPMPSManufacturerKey                      "Manufacturer"
#define kIOPMPSManufactureDateKey                   "ManufactureDate"
#define kIOPMPSModelKey                             "Model"
#define kIOPMPSSerialKey                            "Serial"
#define kIOPMDeviceNameKey                          "DeviceName"
#define kIOPMPSLegacyBatteryInfoKey                 "LegacyBatteryInfo"
#define kIOPMPSBatteryHealthKey                     "BatteryHealth"
#define kIOPMPSHealthConfidenceKey                  "HealthConfidence"
#define kIOPMPSCapacityEstimatedKey                 "CapacityEstimated"
#define kIOPMPSBatteryChargeStatusKey               "ChargeStatus"
#define kIOPMPSBatteryTemperatureKey                "Temperature"
#define kIOPMPSAdapterDetailsKey                    "AdapterDetails"
#define kIOPMPSChargerConfigurationKey              "ChargerConfiguration"

                                                                                  
                                                                                             
                                                                                               
                                             
#define kIOPMBatteryChargeStatusTooHot              "HighTemperature"
#define kIOPMBatteryChargeStatusTooCold             "LowTemperature"
#define kIOPMBatteryChargeStatusTooHotOrCold        "HighOrLowTemperature"
#define kIOPMBatteryChargeStatusGradient            "BatteryTemperatureGradient"

                                                                   
                                 
                                                   
enum {
	kIOPMPSLocationLeft = 1001,
	kIOPMPSLocationRight = 1002
};

                                                                                
                                                 
enum {
	kIOPMUndefinedValue = 0,
	kIOPMPoorValue      = 1,
	kIOPMFairValue      = 2,
	kIOPMGoodValue      = 3
};

                                               
#define kIOPMPSAdapterDetailsIDKey                  "AdapterID"
#define kIOPMPSAdapterDetailsWattsKey               "Watts"
#define kIOPMPSAdapterDetailsRevisionKey            "AdapterRevision"
#define kIOPMPSAdapterDetailsSerialNumberKey        "SerialNumber"
#define kIOPMPSAdapterDetailsFamilyKey              "FamilyCode"
#define kIOPMPSAdapterDetailsAmperageKey            "Current"
#define kIOPMPSAdapterDetailsDescriptionKey         "Description"
#define kIOPMPSAdapterDetailsPMUConfigurationKey    "PMUConfiguration"
#define kIOPMPSAdapterDetailsVoltage                "Voltage"
#define kIOPMPSAdapterDetailsSourceIDKey            "Source"
#define kIOPMPSAdapterDetailsErrorFlagsKey          "ErrorFlags"
#define kIOPMPSAdapterDetailsSharedSourceKey        "SharedSource"
#define kIOPMPSAdapterDetailsCloakedKey             "CloakedSource"

                                        
enum {
	kIOPSFamilyCodeDisconnected     = 0,
	kIOPSFamilyCodeUnsupported      = kIOReturnUnsupported,
	kIOPSFamilyCodeFirewire     = iokit_family_err(sub_iokit_firewire, 0),
	kIOPSFamilyCodeUSBHost      = iokit_family_err(sub_iokit_usb, 0),
	kIOPSFamilyCodeUSBHostSuspended   = iokit_family_err(sub_iokit_usb, 1),
	kIOPSFamilyCodeUSBDevice      = iokit_family_err(sub_iokit_usb, 2),
	kIOPSFamilyCodeUSBAdapter     = iokit_family_err(sub_iokit_usb, 3),
	kIOPSFamilyCodeUSBChargingPortDedicated = iokit_family_err(sub_iokit_usb, 4),
	kIOPSFamilyCodeUSBChargingPortDownstream  = iokit_family_err(sub_iokit_usb, 5),
	kIOPSFamilyCodeUSBChargingPort    = iokit_family_err(sub_iokit_usb, 6),
	kIOPSFamilyCodeUSBUnknown     = iokit_family_err(sub_iokit_usb, 7),
	kIOPSFamilyCodeUSBCBrick      = iokit_family_err(sub_iokit_usb, 8),
	kIOPSFamilyCodeUSBCTypeC      = iokit_family_err(sub_iokit_usb, 9),
	kIOPSFamilyCodeUSBCPD         = iokit_family_err(sub_iokit_usb, 10),
	kIOPSFamilyCodeAC       = iokit_family_err(sub_iokit_pmu, 0),
	kIOPSFamilyCodeExternal     = iokit_family_err(sub_iokit_pmu, 1),
	kIOPSFamilyCodeExternal2     = iokit_family_err(sub_iokit_pmu, 2),
	kIOPSFamilyCodeExternal3     = iokit_family_err(sub_iokit_pmu, 3),
	kIOPSFamilyCodeExternal4     = iokit_family_err(sub_iokit_pmu, 4),
	kIOPSFamilyCodeExternal5     = iokit_family_err(sub_iokit_pmu, 5),
	kIOPSFamilyCodeExternal6     = iokit_family_err(sub_iokit_pmu, 6),
	kIOPSFamilyCodeExternal7     = iokit_family_err(sub_iokit_pmu, 7),
};

                                                
enum {
	kIOPSAdapterErrorFlagNoErrors                     = 0,
	kIOPSAdapterErrorFlagInsufficientAvailablePower   = (1 << 1),
	kIOPSAdapterErrorFlagForeignObjectDetected        = (1 << 2),
	kIOPSAdapterErrorFlagDeviceNeedsToBeRepositioned  = (1 << 3),
};

                                                                                
#define kIOPMPSInvalidWakeSecondsKey           "BatteryInvalidWakeSeconds"

                                                                              
                          
#define kIOPMPSPostChargeWaitSecondsKey        "PostChargeWaitSeconds"

                                                                          
                                 
#define kIOPMPSPostDishargeWaitSecondsKey      "PostDischargeWaitSeconds"


                                   
                                                                
                                                    
                                                                     
                                                                  
                                                                    
                     
   


                              
                                                                  
                                                                            
                                                                     
   
#define kIOPMGraphicsPowerLimitsKey                     "Graphics_Power_Limits"

                                        
                                                                           
                                                            
   
#define kIOPMGraphicsPowerLimitPerformanceKey           "Graphics_Power_Performance"



                         
                                                             
                                                                            
                                                                     
   
#define kIOPMCPUPowerLimitsKey                          "CPU_Power_Limits"

                                                                                
                
                                                              
   
#define kIOPMCPUPowerLimitProcessorSpeedKey             "CPU_Speed_Limit"

                                                                                
                                                                            
   
#define kIOPMCPUPowerLimitProcessorCountKey             "CPU_Available_CPUs"

                                                                                   
                                                                                     
                    
   
#define kIOPMCPUPowerLimitSchedulerTimeKey              "CPU_Scheduler_Limit"


                            
                                                                         
                                                             
                                                                     
   
#define kIOPMThermalLevelWarningKey                     "Thermal_Level_Warning"

                               
                                                                     
                                                                              
                                                                                  
  
                                                                  
                                                          
   
enum {
	kIOPMThermalLevelNormal    = 0,
	kIOPMThermalLevelDanger    = 5,
	kIOPMThermalLevelCritical  = 10,

	kIOPMThermalLevelWarning = 100,
	kIOPMThermalLevelTrap    = 110,

	kIOPMThermalLevelUnknown = 255,
};

#define kIOPMThermalWarningLevelNormal kIOPMThermalLevelNormal
#define kIOPMThermalWarningLevelDanger kIOPMThermalLevelWarning
#define kIOPMThermalWarningLevelCrisis kIOPMThermalLevelCritical

                                       
                                      
                                                   
                                                                             
                                                                
#define kIOPMSettingWakeOnRingKey                   "Wake On Modem Ring"
#define kIOPMSettingRestartOnPowerLossKey           "Automatic Restart On Power Loss"
#define kIOPMSettingWakeOnACChangeKey               "Wake On AC Change"
#define kIOPMSettingSleepOnPowerButtonKey           "Sleep On Power Button"
#define kIOPMSettingWakeOnClamshellKey              "Wake On Clamshell Open"
#define kIOPMSettingReduceBrightnessKey             "ReduceBrightness"
#define kIOPMSettingDisplaySleepUsesDimKey          "Display Sleep Uses Dim"
#define kIOPMSettingTimeZoneOffsetKey               "TimeZoneOffsetSeconds"
#define kIOPMSettingMobileMotionModuleKey           "MobileMotionModule"
#define kIOPMSettingGraphicsSwitchKey               "GPUSwitch"
#define kIOPMSettingProModeControl                  "ProModeControl"
#define kIOPMSettingProModeDefer                    "ProModeDefer"

                                                                          
                                                                          
                                    
#define kIOPMSettingAutoWakeSecondsKey              "wake"
#define kIOPMSettingAutoWakeCalendarKey             "WakeByCalendarDate"
#define kIOPMSettingAutoPowerSecondsKey             "poweron"
#define kIOPMSettingAutoPowerCalendarKey            "PowerByCalendarDate"

                          
                                    
#define kIOPMSettingDebugWakeRelativeKey            "WakeRelativeToSleep"
#define kIOPMSettingDebugPowerRelativeKey           "PowerRelativeToShutdown"

                             
#define kIOPMSettingMaintenanceWakeCalendarKey      "MaintenanceWakeCalendarDate"


struct IOPMCalendarStruct {
	UInt32      year;
	UInt8       month;
	UInt8       day;
	UInt8       hour;
	UInt8       minute;
	UInt8       second;
	UInt8       selector;
};
typedef struct IOPMCalendarStruct IOPMCalendarStruct;

                          
enum {
	kPMGeneralAggressiveness = 0,
	kPMMinutesToDim,
	kPMMinutesToSpinDown,
	kPMMinutesToSleep,
	kPMEthernetWakeOnLANSettings,
	kPMSetProcessorSpeed,
	kPMPowerSource,
	kPMMotionSensor,
	kPMLastAggressivenessType
};
#define kMaxType (kPMLastAggressivenessType-1)

                                                                      
enum {
	kIOPMInternalPower = 1,
	kIOPMExternalPower
};

#define kIOREMSleepEnabledKey               "REMSleepEnabled"

                                                                           
#define kIOBatteryInfoKey                   "IOBatteryInfo"
#define kIOBatteryCurrentChargeKey          "Current"
#define kIOBatteryCapacityKey               "Capacity"
#define kIOBatteryFlagsKey                  "Flags"
#define kIOBatteryVoltageKey                "Voltage"
#define kIOBatteryAmperageKey               "Amperage"
#define kIOBatteryCycleCountKey             "Cycle Count"

enum {
	kIOBatteryInstalled         = (1 << 2),
	kIOBatteryCharge            = (1 << 1),
	kIOBatteryChargerConnect    = (1 << 0)
};

                                                                       
                                               
#define kIOPMMessageBatteryStatusHasChanged         iokit_family_msg(sub_iokit_pmu, 0x100)

                                                                                          
                                                      
#define kIOPMUMessageLegacyAutoWake                 iokit_family_msg(sub_iokit_pmu, 0x200)
#define kIOPMUMessageLegacyAutoPower                iokit_family_msg(sub_iokit_pmu, 0x210)

                                      
#define IOPM_POWER_SOURCE_REV   2
enum {
	kIOPMACInstalled      = kIOBatteryChargerConnect,
	kIOPMBatteryCharging  = kIOBatteryCharge,
	kIOPMBatteryInstalled = kIOBatteryInstalled,
	kIOPMUPSInstalled     = (1 << 3),
	kIOPMBatteryAtWarn    = (1 << 4),
	kIOPMBatteryDepleted  = (1 << 5),
	kIOPMACnoChargeCapability = (1 << 6),                                    
	kIOPMRawLowBattery    = (1 << 7),                                 
	kIOPMForceLowSpeed    = (1 << 8),                                             
	kIOPMClosedClamshell  = (1 << 9),                                                
	kIOPMClamshellStateOnWake = (1 << 10)                                
};

                                                 
                                            
                                                 

#if KERNEL && __cplusplus
class IOService;

enum {
	kIOPowerEmergencyLevel = 1000
};

enum {
	kIOPMSubclassPolicy,
	kIOPMSuperclassPolicy1
#ifdef KERNEL_PRIVATE
	, kIOPMActivityTickleTypeAdvisory = 128
#endif
};

struct stateChangeNote {
	IOPMPowerFlags    stateFlags;
	unsigned long    stateNum;
	void *         powerRef;
};
typedef struct stateChangeNote stateChangeNote;

#endif                            
struct IOPowerStateChangeNotification {
	void *        powerRef;
	unsigned long    returnValue;
	unsigned long    stateNumber;
	IOPMPowerFlags    stateFlags;
};
typedef struct IOPowerStateChangeNotification IOPowerStateChangeNotification;
typedef IOPowerStateChangeNotification sleepWakeNote;

                                                
                                                                 
                                                                                   
                                                                                     
                                                                                   
                                                             
                                                                                  
                 
                                                                                       
                                                                                       
                                                                                 
                                                
                                                                                       
                                                                                       
                                                                                     
                                                                                
                                                                                       
                                                                                       
                                                                                          
                                    
                                                                                    
                                                                                
                                    
   
struct IOPMSystemCapabilityChangeParameters {
	uint32_t    notifyRef;
	uint32_t    maxWaitForReply;
	uint32_t    changeFlags;
	uint32_t    __reserved1;
	uint32_t    fromCapabilities;
	uint32_t    toCapabilities;
	uint32_t    __reserved2[4];
};

                                         
                                                                                           
                                                                                          
   
enum {
	kIOPMSystemCapabilityWillChange = 0x01,
	kIOPMSystemCapabilityDidChange  = 0x02
};

enum {
	kIOPMSystemCapabilityCPU        = 0x01,
	kIOPMSystemCapabilityGraphics   = 0x02,
	kIOPMSystemCapabilityAudio      = 0x04,
	kIOPMSystemCapabilityNetwork    = 0x08
};

#endif                      
