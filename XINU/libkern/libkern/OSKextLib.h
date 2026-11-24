/*
 * Copyright (c) 2008 Apple Inc. All rights reserved.
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

#ifndef _LIBKERN_OSKEXTLIB_H
#define _LIBKERN_OSKEXTLIB_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <mach/kmod.h>
#include <mach/vm_types.h>
#include <uuid/uuid.h>

#ifdef KERNEL
#include <libkern/OSTypes.h>
#include <libkern/OSReturn.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#include <libkern/OSReturn.h>
#endif             

   
          
  
                                                               
                                        
   

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark OSReturn Values for Kernel Extensions
                                                                      
#endif
   
                                               
                                                   
                                 
                                                                
                                                    
   

#ifdef XNU_KERNEL_PRIVATE
                                                                      
                                                              
               
                                                                      
#endif                         

#define sub_libkern_kext           err_sub(2)
#define libkern_kext_err(code)     (sys_libkern|sub_libkern_kext|(code))


   
                                       
                                                   
                                                                         
                                        
   
#define kOSKextReturnInternalError                   libkern_kext_err(0x1)

   
                                  
                                      
   
#define kOSKextReturnNoMemory                        libkern_kext_err(0x2)

   
                                     
                                                                          
                          
   
#define kOSKextReturnNoResources                     libkern_kext_err(0x3)

   
                                       
                                                                            
   
#define kOSKextReturnNotPrivileged                   libkern_kext_err(0x4)

   
                                         
                              
   
#define kOSKextReturnInvalidArgument                 libkern_kext_err(0x5)

   
                                  
                                   
   
#define kOSKextReturnNotFound                        libkern_kext_err(0x6)

   
                                 
                                               
   
#define kOSKextReturnBadData                         libkern_kext_err(0x7)

   
                                       
                                                                     
   
#define kOSKextReturnSerialization                   libkern_kext_err(0x8)

   
                                     
                                                         
   
#define kOSKextReturnUnsupported                     libkern_kext_err(0x9)

   
                                  
                                             
   
#define kOSKextReturnDisabled                        libkern_kext_err(0xa)

   
                                  
                                              
   
#define kOSKextReturnNotAKext                        libkern_kext_err(0xb)

   
                                    
                                                                            
   
#define kOSKextReturnValidation                      libkern_kext_err(0xc)

   
                                        
                                                                               
   
#define kOSKextReturnAuthentication                  libkern_kext_err(0xd)

   
                                      
                                                                                       
   
#define kOSKextReturnDependencies                    libkern_kext_err(0xe)

   
                                      
                                                                       
   
#define kOSKextReturnArchNotFound                    libkern_kext_err(0xf)

   
                               
                                                              
   
#define kOSKextReturnCache                           libkern_kext_err(0x10)

   
                                  
                                                                                  
   
#define kOSKextReturnDeferred                        libkern_kext_err(0x11)

   
                                   
                                                                              
   
#define kOSKextReturnBootLevel                       libkern_kext_err(0x12)

   
                                     
                                                                  
   
#define kOSKextReturnNotLoadable                     libkern_kext_err(0x13)

   
                                              
                                                                                
                                                     
   
#define kOSKextReturnLoadedVersionDiffers            libkern_kext_err(0x14)

   
                                             
                                                                            
   
#define kOSKextReturnDependencyLoadError             libkern_kext_err(0x15)

   
                                   
                                                                   
   
#define kOSKextReturnLinkError                       libkern_kext_err(0x16)

   
                                        
                                                              
   
#define kOSKextReturnStartStopError                  libkern_kext_err(0x17)

   
                               
                                                                        
                                    
   
#define kOSKextReturnInUse                           libkern_kext_err(0x18)

   
                                 
                                          
   
#define kOSKextReturnTimeout                         libkern_kext_err(0x19)

   
                                  
                                                                             
   
#define kOSKextReturnStopping                        libkern_kext_err(0x1a)

   
                                      
                                                                      
   
#define kOSKextReturnSystemPolicy                    libkern_kext_err(0x1b)

   
                                       
                                            
   
#define kOSKextReturnKCLoadFailure                  libkern_kext_err(0x1c)

   
                                               
                                            
  
                                                                         
                                        
  
                                                                            
                         
   
#define kOSKextReturnKCLoadFailureSystemKC          libkern_kext_err(0x1d)

   
                                            
                                         
  
                                                                         
                                           
  
                                                                            
                         
   
#define kOSKextReturnKCLoadFailureAuxKC             libkern_kext_err(0x1e)

                                                     

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Kext/OSBundle Property List Keys
                                                                      
#endif
   
                                 
                                                                           
                                                                   
                                                                        
   

#ifdef KERNEL
                                                                       
   
#define kCFBundleIdentifierKey                  "CFBundleIdentifier"
#define kCFBundleIdentifierKernelKey            "CFBundleIdentifierKernel"
#define kCFBundleVersionKey                     "CFBundleVersion"
#define kCFBundleNameKey                        "CFBundleName"
#define kCFBundleExecutableKey                  "CFBundleExecutable"
#define kCFBundlePackageTypeKey                 "CFBundlePackageType"
#define kCFBundleDriverKitUUIDKey               "CFBundleDriverKitUUID"
#define kCFBundleDriverKitExecutableKey         "CFBundleUEXTExecutable"
#endif             

   
                                          
                                                                               
                                                                           
                                                          
   
#define kOSBundleCompatibleVersionKey           "OSBundleCompatibleVersion"

   
                                          
                                                                     
                         
                                                               
                                        
   
#define kOSBundleEnableKextLoggingKey           "OSBundleEnableKextLogging"

   
                                    
                                                                   
                                             
   
#define kOSBundleIsInterfaceKey                 "OSBundleIsInterface"

   
                                  
                                                                  
                                                                     
   
#define kOSBundleLibrariesKey                   "OSBundleLibraries"

   
                                 
                                                                    
                                                          
                                                                                
              
                       
       
                                                                    
                                                                              
                                                                                  
                                                                            
                                                                          
        
  
                                 
                                                                         
                                                                  
                                          
   
#define kOSBundleRequiredKey                    "OSBundleRequired"

   
                                            
                                                                    
                                                       
   
#define kOSBundleRequireExplicitLoadKey         "OSBundleRequireExplicitLoad"

   
                                      
                                               
                                                                              
                                                                    
              
                                                                                  
                                          
                                                                                         
                                                  
   
#define kOSBundleAllowUserLoadKey               "OSBundleAllowUserLoad"

   
                                           
                                                                   
                                                                                     
                                                                     
                                                                                     
   
#define kOSBundleAllowUserTerminateKey          "OSBundleAllowUserTerminate"

   
                                 
                                                                              
                                     
   
#define kOSKernelResourceKey                    "OSKernelResource"

   
                                      
                                                                                
                           
   
#define kOSKextVariantOverrideKey               "OSKextVariantOverride"

   
                                   
                                                                               
   
#define kIOKitPersonalitiesKey                  "IOKitPersonalities"

  
                                       
                                                       
                                                        
                                                
   
#define kIOPersonalityPublisherKey              "IOPersonalityPublisher"

#if CONFIG_KEC_FIPS
  
                                
                                                                 
   
#define kAppleTextHashesKey                     "AppleTextHashes"
#endif

   
                                  
                                                                           
                                                                                    
                                                                              
                                               
   
#define kOSMutableSegmentCopy                   "OSMutableSegmentCopy"


#if PRAGMA_MARK
                                                                      
#pragma mark Kext/OSBundle Property Deprecated Keys
                                                                      
#endif
  
                                   
            
                                                                             
                                                                    
                                   
                                                                
                                        
   
#define kOSBundleDebugLevelKey                  "OSBundleDebugLevel"

   
                                                   
                                                          
                                         
                                                             
                                                             
   
#define kOSBundleSharedExecutableIdentifierKey  "OSBundleSharedExecutableIdentifier"


#if PRAGMA_MARK
                                                                      
#pragma mark Kext/OSBundle Property List Values
                                                                      
#endif

   
                                   
                                               
                                          
   

   
                                    
            
                                                             
   
#define kOSKextKernelIdentifier                 "__kernel__"

   
                                        
            
                                               
   
#define kOSKextBundlePackageTypeKext        "KEXT"

   
                                             
            
                                               
   
#define kOSKextBundlePackageTypeDriverKit   "DEXT"

   
                                  
            
                                                                      
                                                                           
                                                     
   
#define kOSBundleRequiredRoot                   "Root"

   
                                       
            
                                                                      
                                                                           
                                   
   
#define kOSBundleRequiredLocalRoot              "Local-Root"

   
                                         
            
                                                                      
                                                                           
                                           
   
#define kOSBundleRequiredNetworkRoot            "Network-Root"

   
                                      
            
                                                                      
                                                                     
                                                                       
                                      
   
#define kOSBundleRequiredSafeBoot               "Safe Boot"

   
                                     
            
                                                                      
                                                                 
                                              
                                                                 
                
                                             
   
#define kOSBundleRequiredConsole                "Console"

   
                                       
            
                                                                      
                                                                   
                                                                          
                                                                               
                                                                           
                                                                            
                                                                       
                                  
   
#define kOSBundleRequiredDriverKit              "DriverKit"

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Kext Information
                                                                      
#endif
   
                          
                                                                 
                
   

#ifdef KERNEL
   
                         
  
            
                                                                
  
              
                                                             
                            
  
                                                                  
                                                         
                                             
                                              
                                          
                                               
                                           
                                             
   
#else
   
                         
  
            
                                                                
  
              
                                                             
                            
  
                                                                  
                                                         
                                             
   
#endif
typedef uint32_t  OSKextLoadTag;

   
                                
  
            
                                                              
                            
   
#define  kOSKextInvalidLoadTag  ((OSKextLoadTag)(-1))

#ifdef KERNEL

                                                         
   
#if !XNU_KERNEL_PRIVATE

   
                                    
  
            
                                                           
                                                        
  
          
                                                   
                                          
  
              
                                                                         
                                                 
   
OSKextLoadTag OSKextGetCurrentLoadTag(void);

   
                                       
  
            
                                                                     
  
          
                                                             
   
const char * OSKextGetCurrentIdentifier(void);

   
                                          
  
            
                                                                  
  
          
                                                          
   
const char * OSKextGetCurrentVersionString(void);

#endif                          

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Kext Loading C Functions
                                                                      
#endif
   
                                  
                                                          
   

   
                                         
  
            
                                 
  
                                                                          
  
          
                                                                                 
                                                  
                                                                                           
                                          
                                                                               
                                                           
  
              
                                                                               
                                                     
                                                                        
                                                                                                             
                                                                            
   
OSReturn OSKextLoadKextWithIdentifier(const char * kextIdentifier);


   
                                        
  
            
                                              
                                       
  
                                                             
                                                                          
  
          
                                                                                 
                            
                                                                                           
                             
                                                               
                                            
                             
                                                                        
  
              
                                                    
                                                                           
                                                                           
                                                                               
                                                                     
  
                                  
                                                                     
                                         
                                                                     
                                                           
                                                                        
                                                          
                                                                                 
  
                                            
                                                    
  
                                                                      
                                
   
OSReturn OSKextRetainKextWithLoadTag(OSKextLoadTag loadTag);


   
                                         
  
            
                                               
  
                                                             
                                                                          
  
          
                                                                                 
                            
                                                        
                                     
                             
                                                               
                                            
                             
                                                                        
  
              
                                                    
                                                        
  
                                                            
                                                           
                                                                   
                                                                  
  
                                  
                                                                     
                                         
                                                                     
                                                           
                                                                      
                                                    
                                                                                 
  
                                                                            
                                                                     
                                                                              
                                          
  
                                            
                                                    
  
                                                                      
                                
   
OSReturn OSKextReleaseKextWithLoadTag(OSKextLoadTag loadTag);

#if PRAGMA_MARK
                                                                      
#pragma mark -
#pragma mark Kext Requests
                                                                      
#endif
   
                                     
                                                        
   

   
                            
  
            
                                                
   
typedef uint32_t OSKextRequestTag;

   
                                   
  
            
                                                                  
                                                 
   
#define kOSKextRequestTagInvalid  ((OSKextRequestTag)-1)

   
                                         
  
            
                                                          
  
                                                                                    
                                                         
                                                           
                                                            
                                                            
                                                               
                                                                
                                                            
                                                                
                                                                 
                                                                  
                                                                      
                                                        
                                                                        
                                                                               
                                                                              
                                                                       
                                                                       
                                                                
                                                                  
   
typedef void (* OSKextRequestResourceCallback)(
	OSKextRequestTag                requestTag,
	OSReturn                        result,
	const void                    * resourceData,
	uint32_t                        resourceDataLength,
	void                          * context);

   
                                  
  
            
                                                                            
  
                                                             
                                                       
                                                                 
                                                                        
                                                                  
                                                               
                                                              
                                                                        
                                                    
                                                                       
                                          
                                                                                   
                                                                 
                                           
                                                          
                                                            
  
          
                                                             
                                         
                                                                                     
                                                                    
                                                                           
                                                                       
                                     
                                                
                                                          
  
              
                                                                             
                                                                 
                                                 
                                                  
                                                     
                                                    
                                                                  
                                 
                                                                           
                            
  
                                                                   
                          
                                                                        
                                       
                                              
                                 
                                        
                                                
                                     
  
                                                                 
       
                                                                       
                                     
                                                                  
                                
                                                               
                                              
                                                           
                                                    
                                                            
        
  
                                                             
                                                           
                                                                  
                                                     
                                                                      
  
                                                                        
                           
                                                                        
                                        
                                                   
                                                                       
                                
   
OSReturn OSKextRequestResource(
	const char                    * kextIdentifier,
	const char                    * resourceName,
	OSKextRequestResourceCallback   callback,
	void                          * context,
	OSKextRequestTag              * requestTagOut);

   
                                
  
            
                                                                           
  
                                                           
                                                                                
                                                          
  
          
                                                             
                                           
                                                                       
                                                                    
                                                          
  
              
                                                        
                                            
                                        
                                                 
                                                                     
  
                                                   
                                  
                                                         
                               
                                                                     
                                      
   
OSReturn OSKextCancelRequest(
	OSKextRequestTag    requestTag,
	void             ** contextOut);


   
                              
  
            
                                                      
  
                                                                                
                                                                                             
                                                                         
                                                                     
                                                                       
                                                              
  
          
               
                                                              
                                                  
                                              
                                                                                
   
int
OSKextGrabPgoData(uuid_t uuid,
    uint64_t *pSize,
    char *pBuffer,
    uint64_t bufferSize,
    int wait_for_unload,
    int metadata);

   
                                       
  
            
                                                                                       
   
void
OSKextResetPgoCountersLock(void);

   
                                         
  
            
                                                                                      
   
void
OSKextResetPgoCountersUnlock(void);

   
                                   
  
                                                                           
                                                       
   
void
OSKextResetPgoCounters(void);


#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Weak linking
                                                                      
#endif
   
                      
                                                   
   

   
                         
  
            
                                                                               
  
              
                                                                 
                                                                            
  
                                                    
        
             
                                        
                  
                
                                               
         
              
         
   
extern const void * const gOSKextUnresolved;

   
                                 
  
            
                                                               
  
                                                                 
  
          
                                                                   
                             
  
              
                                                                        
  
                                                    
        
             
                                          
                  
                
                                              
         
              
         
   
#define OSKextSymbolIsResolved(weak_sym)        \
    (&(weak_sym) != gOSKextUnresolved)


#if CONFIG_KEC_FIPS

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Kernel External Components for FIPS compliance
                                                                      
#endif

                                                            
                                                         
typedef struct AppleTEXTHash {
	int                 ath_version;                                              
	int             ath_length;                       
	void *          ath_hash;                                              
} AppleTEXTHash_t;
#endif                   

#endif             

__END_DECLS

#endif                           
