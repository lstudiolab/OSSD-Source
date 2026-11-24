/*
 * Copyright (c) 1998-2000 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
 *
 * Common symbol definitions for IOKit.
 *
 * HISTORY
 *
 */


#ifndef _IOKIT_IOKITKEYS_H
#define _IOKIT_IOKITKEYS_H

                                        
#define kIOKitBuildVersionKey           "IOKitBuildVersion"
#define kIOKitDiagnosticsKey            "IOKitDiagnostics"
                                   
#define kIORegistryPlanesKey            "IORegistryPlanes"
#define kIOCatalogueKey                 "IOCatalogue"

                       
#define kIOServicePlane                 "IOService"
#define kIOPowerPlane                   "IOPower"
#define kIODeviceTreePlane              "IODeviceTree"
#define kIOAudioPlane                   "IOAudio"
#define kIOFireWirePlane                "IOFireWire"
#define kIOUSBPlane                     "IOUSB"

                     
#define kIORegistryEntryIDKey           "IORegistryEntryID"
                                               
#define kIORegistryEntryPropertyKeysKey "IORegistryEntryPropertyKeys"

                       
#define kIOServiceClass                 "IOService"

                         
#define kIOResourcesClass               "IOResources"

                                          
#define kIOClassKey                     "IOClass"
#define kIOProbeScoreKey                "IOProbeScore"
#define kIOKitDebugKey                  "IOKitDebug"

                                    
#define kIOSupportedPropertiesKey       "IOSupportedProperties"
                               
#define kIOUserServicePropertiesKey     "IOUserServiceProperties"


                                    
#define kIOProviderClassKey             "IOProviderClass"
#define kIONameMatchKey                 "IONameMatch"
#define kIOPropertyMatchKey             "IOPropertyMatch"
#define kIOPropertyExistsMatchKey       "IOPropertyExistsMatch"
#define kIOPathMatchKey                 "IOPathMatch"
#define kIOLocationMatchKey             "IOLocationMatch"
#define kIOParentMatchKey               "IOParentMatch"
#define kIOResourceMatchKey             "IOResourceMatch"
#define kIOResourceMatchedKey           "IOResourceMatched"
#define kIOMatchedServiceCountKey       "IOMatchedServiceCountMatch"

#define kIONameMatchedKey               "IONameMatched"

#define kIOMatchCategoryKey             "IOMatchCategory"
#define kIODefaultMatchCategoryKey      "IODefaultMatchCategory"

#define kIOMatchedPersonalityKey        "IOMatchedPersonality"
#define kIORematchPersonalityKey        "IORematchPersonality"
#define kIORematchCountKey              "IORematchCount"
#define kIODEXTMatchCountKey            "IODEXTMatchCount"

                                             
                                                            
                                                         
                                    
#define kIOServiceDEXTEntitlementsKey   "IOServiceDEXTEntitlements"

                                               
#define kIODriverKitEntitlementKey      "com.apple.developer.driverkit"

                                                  
                                                                                         
#define kIODriverKitUserClientEntitlementsKey "com.apple.developer.driverkit.userclient-access"

                                                                              
#define kIODriverKitUserClientEntitlementAllowAnyKey "com.apple.developer.driverkit.allow-any-userclient-access"

                               
#define kIODriverKitUSBTransportEntitlementKey "com.apple.developer.driverkit.transport.usb"
#define kIODriverKitHIDTransportEntitlementKey "com.apple.developer.driverkit.transport.hid"
#define kIODriverKitHIDFamilyDeviceEntitlementKey "com.apple.developer.driverkit.family.hid.device"
#define kIODriverKitHIDFamilyEventServiceEntitlementKey "com.apple.developer.driverkit.family.hid.eventservice"
#define kIODriverKitTransportBuiltinEntitlementKey "com.apple.developer.driverkit.builtin"

                                                                           
#define kIONVRAMReadAccessKey           "com.apple.private.iokit.nvram-read-access"
                                                                  
#define kIONVRAMWriteAccessKey           "com.apple.private.iokit.nvram-write-access"
                                                                                    
#define kIOResourcesSetPropertyKey       "com.apple.private.iokit.ioresources.setproperty"
                                                                
#define kIONVRAMSystemAllowKey           "com.apple.private.iokit.system-nvram-allow"

                                                                        
#define kIOMatchDeferKey                                "IOMatchDefer"

                                                                              
#define kIOAllCPUInitializedKey                         "IOAllCPUInitialized"

                                                                 
#define kIOUserClientClassKey           "IOUserClientClass"

                        
#define kIOMapperIDKey                          "IOMapperID"

#ifdef XNU_KERNEL_PRIVATE
                          
#define kIOExcludeListBundleID                  "com.apple.driver.KextExcludeList"
#endif

#define kIOUserClientCrossEndianKey             "IOUserClientCrossEndian"
#define kIOUserClientCrossEndianCompatibleKey   "IOUserClientCrossEndianCompatible"
#define kIOUserClientSharedInstanceKey          "IOUserClientSharedInstance"
#if KERNEL_PRIVATE
#define kIOUserClientMessageAppSuspendedKey     "IOUserClientMessageAppSuspended"
#endif
#define kIOUserClientDefaultLockingKey                  "IOUserClientDefaultLocking"
                                                 
#define kIOUserClientCreatorKey         "IOUserClientCreator"
                                                               
#define kIOUserServerCDHashKey          "IOUserServerCDHash"

#define kIOUserUserClientKey                    "IOUserUserClient"


                               
#define kIOPublishNotification          "IOServicePublish"
#define kIOFirstPublishNotification     "IOServiceFirstPublish"
#define kIOMatchedNotification          "IOServiceMatched"
#define kIOFirstMatchNotification       "IOServiceFirstMatch"
#define kIOTerminatedNotification       "IOServiceTerminate"
#define kIOWillTerminateNotification    "IOServiceWillTerminate"

                                        
#define kIOGeneralInterest              "IOGeneralInterest"
#define kIOBusyInterest                 "IOBusyInterest"
#define kIOAppPowerStateInterest        "IOAppPowerStateInterest"
#define kIOPriorityPowerStateInterest   "IOPriorityPowerStateInterest"

#define kIOPlatformDeviceMessageKey     "IOPlatformDeviceMessage"

                                        
#define kIOCFPlugInTypesKey             "IOCFPlugInTypes"

#define kIOCompatibilityMatchKey            "IOCompatibilityMatch"
#define kIOCompatibilityPropertiesKey   "IOCompatibilityProperties"
#define kIOPathKey                                      "IOPath"


                                                              
#define kIOCommandPoolSizeKey           "IOCommandPoolSize"                          

                                                       
#define kIOMaximumPriorityCountKey      "IOMaximumPriorityCount"                     

                                                              
#define kIOMaximumBlockCountReadKey             "IOMaximumBlockCountRead"                          
#define kIOMaximumBlockCountWriteKey            "IOMaximumBlockCountWrite"                         
#define kIOMaximumByteCountReadKey              "IOMaximumByteCountRead"                           
#define kIOMaximumByteCountWriteKey             "IOMaximumByteCountWrite"                          
#define kIOMaximumSegmentCountReadKey           "IOMaximumSegmentCountRead"                        
#define kIOMaximumSegmentCountWriteKey          "IOMaximumSegmentCountWrite"                       
#define kIOMaximumSegmentByteCountReadKey       "IOMaximumSegmentByteCountRead"                    
#define kIOMaximumSegmentByteCountWriteKey      "IOMaximumSegmentByteCountWrite"                   
#define kIOMinimumSegmentAlignmentByteCountKey  "IOMinimumSegmentAlignmentByteCount"               
#define kIOMaximumSegmentAddressableBitCountKey "IOMaximumSegmentAddressableBitCount"              
#define kIOMinimumSaturationByteCountKey        "IOMinimumSaturationByteCount"                     
#define kIOMaximumSwapWriteKey                  "IOMaximumSwapWrite"                               

                                                             
  
           
    
                                                           
                                             
     
  
                                                             

#define kIOIconKey               "IOIcon"                                
#define kIOBundleResourceFileKey "IOBundleResourceFile"              

#define kIOBusBadgeKey           "IOBusBadge"                            
#define kIODeviceIconKey         "IODeviceIcon"                          

                                                                          
#define kIOPlatformSerialNumberKey      "IOPlatformSerialNumber"                     

                                                                 
#define kIOPlatformUUIDKey      "IOPlatformUUID"                     

                          
#define kIONVRAMBootArgsKey             "boot-args"
#define kIONVRAMDeletePropertyKey       "IONVRAM-DELETE-PROPERTY"
#define kIONVRAMSyncNowPropertyKey      "IONVRAM-SYNCNOW-PROPERTY"
#define kIONVRAMActivateCSRConfigPropertyKey    "IONVRAM-ARMCSR-PROPERTY"
#define kIODTNVRAMPanicInfoKey          "aapl,panic-info"

                                    
#define kIOBootDeviceKey          "IOBootDevice"                                        
#define kIOBootDevicePathKey      "IOBootDevicePath"                            
#define kIOBootDeviceSizeKey      "IOBootDeviceSize"                        

                                  
#define kOSBuildVersionKey              "OS Build Version"

#endif                           
