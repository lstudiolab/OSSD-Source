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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef _IOKIT_IOKITKEYSPRIVATE_H
#define _IOKIT_IOKITKEYSPRIVATE_H

#include <IOKit/IOKitKeys.h>

                                        
#define kIOConsoleLockedKey                     "IOConsoleLocked"                                       
#define kIOConsoleUsersKey                      "IOConsoleUsers"                                      
#define kIOMaximumMappedIOByteCountKey          "IOMaximumMappedIOByteCount"                           

                                            
#define kIOConsoleSessionAuditIDKey             "kCGSSessionAuditIDKey"                               

#define kIOConsoleSessionUserNameKey            "kCGSSessionUserNameKey"                              
#define kIOConsoleSessionUIDKey                 "kCGSSessionUserIDKey"                                
#define kIOConsoleSessionConsoleSetKey          "kCGSSessionConsoleSetKey"                            
#define kIOConsoleSessionOnConsoleKey           "kCGSSessionOnConsoleKey"                              
#define kIOConsoleSessionLoginDoneKey           "kCGSessionLoginDoneKey"                               
#define kIOConsoleSessionSecureInputPIDKey      "kCGSSessionSecureInputPID"                           
#define kIOConsoleSessionScreenLockedTimeKey    "CGSSessionScreenLockedTime"                                       
#define kIOConsoleSessionScreenIsLockedKey      "CGSSessionScreenIsLocked"                             

                       
#define kIOConsoleUsersSeedKey                  "IOConsoleUsersSeed"                                  

                                 
#define kIOProgressBackbufferKey                "IOProgressBackbuffer"                                  
#define kIOProgressColorThemeKey                "IOProgressColorTheme"                                  
#define kIOBridgeBootSessionUUIDKey             "bridge-boot-session-uuid"                              

                
#define kIOConsoleSecurityInterest              "IOConsoleSecurityInterest"


                                      
#define kIOClientPrivilegeConsoleUser           "console"
#define kIOClientPrivilegeSecureConsoleProcess  "secureprocess"
#define kIOClientPrivilegeConsoleSession        "consolesession"


                                                                             
                                                                                 
        
#define kIONVRAMForceSyncNowPropertyKey         "IONVRAM-FORCESYNCNOW-PROPERTY"

                                                        
#define kIOKitSystemGUID                        "40A0DDD2-77F8-4392-B4A3-1E7304206516"
#define kIOKitSystemGUIDPrefix                  (kIOKitSystemGUID ":")
                                                                       
#define kIONVRAMSystemInternalAllowKey          "com.apple.private.iokit.system-nvram-internal-allow"
                                                                     
#define kIONVRAMSystemHiddenAllowKey            "com.apple.private.iokit.system-nvram-hidden-allow"

                                                                               
typedef struct _IOUCProcessToken {
	void *  token;
	UInt32  pid;
} IOUCProcessToken;

#define kIOKernelHasSafeSleep        1

#define kIOPlatformSleepActionKey                    "IOPlatformSleepAction"                                           
#define kIOPlatformWakeActionKey                     "IOPlatformWakeAction"                                            
#define kIOPlatformQuiesceActionKey                  "IOPlatformQuiesceAction"                                         
#define kIOPlatformActiveActionKey                   "IOPlatformActiveAction"                                          
#define kIOPlatformHaltRestartActionKey              "IOPlatformHaltRestartAction"                                     
#define kIOPlatformPanicActionKey                    "IOPlatformPanicAction"                                           

#define kIOPlatformFunctionHandlerSet                "IOPlatformFunctionHandlerSet"

#define kIOPlatformFunctionHandlerMaxBusDelay        "IOPlatformFunctionHandlerMaxBusDelay"
#define kIOPlatformMaxBusDelay                       "IOPlatformMaxBusDelay"

#if defined(__i386__) || defined(__x86_64__)

#define kIOPlatformFunctionHandlerMaxInterruptDelay  "IOPlatformFunctionHandlerMaxInterruptDelay"
#define kIOPlatformMaxInterruptDelay                 "IOPlatformMaxInterruptDelay"

#endif                                               

enum {
	                                                      
	kIODirectionPrepareNoZeroFill = 0x00000010,
};

enum {
	kIOServiceTerminateNeedWillTerminate = 0x00000100,
};

#define kIOClassNameOverrideKey "IOClassNameOverride"

enum {
	kIOClassNameOverrideNone = 0x00000001,
};

#define kIOServiceLegacyMatchingRegistryIDKey "IOServiceLegacyMatchingRegistryID"

#endif                                  
