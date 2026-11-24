/*
 * Copyright (c) 1998-2000 Apple Inc. All rights reserved.
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

#ifndef _LIBKERN_OSKEXTLIBPRIVATE_H
#define _LIBKERN_OSKEXTLIBPRIVATE_H


#include <sys/cdefs.h>
#include <uuid/uuid.h>

__BEGIN_DECLS
#ifdef KERNEL
#include <libkern/OSTypes.h>
#include <mach/kmod.h>
#include <mach/vm_types.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#include <mach/kmod.h>
#endif             
__END_DECLS

#include <libkern/OSReturn.h>

__BEGIN_DECLS

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Misc Constants
                                                                      
#endif

typedef uint8_t OSKextExcludeLevel;
#define kOSKextExcludeNone  (0)
#define kOSKextExcludeKext  (1)
#define kOSKextExcludeAll   (2)

#define kIOCatalogManagementEntitlement "com.apple.private.security.iocatalog-management"
#define kOSKextCollectionManagementEntitlement "com.apple.private.security.kext-collection-management"
#define kOSKextOnlyBootKCManagementEntitlement "com.apple.private.security.only-bootkc-management"

#define kOSKextCodelessKextLoadAddr (0x7FFFFFFFFFFFFFFFULL)

#define kIOKitDaemonName "kernelmanagerd"

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Kext/OSBundle Property List Keys
                                                                      
#endif

   
                               
                                 
   
#define kOSBundleHelperKey      "OSBundleHelper"

   
                                      
                                                                            
                               
   
#define kOSBundleDeveloperOnlyKey               "OSBundleDeveloperOnly"

   
                                    
                                                                              
                                    
   
#define kOSBundleRamDiskOnlyKey         "OSBundleRamDiskOnly"


   
                                       
                                                                  
                        
   
#define kAppleSecurityExtensionKey      "AppleSecurityExtension"

   
                                             
                                                                          
                                                     
   
#define kAppleKernelExternalComponentKey        "AppleKernelExternalComponent"

   
                                    
                                                  
   
#define kOSKextInfoPlistDigestKey       "_InfoPlistDigest"

   
                                         
                                                                       
                                                                     
                                    
   
#define kOSKextBundleCollectionTypeKey  "_BundleCollectionType"

   
                                      
                                                                 
                                             
   
#define kOSKextAuxKCAvailabilityKey     "_AuxKCAvailability"


                                        
#define kOSKernelCPUTypeKey             "OSKernelCPUType"
#define kOSKernelCPUSubtypeKey          "OSKernelCPUSubtype"
#define kOSStartupMkextCRC              "OSStartupMkextCRC"                                         
#define kOSPrelinkKextCountKey          "OSPrelinkKextCount"                                          
#define kOSPrelinkPersonalityCountKey   "OSPrelinkPersonalityCount"                                   

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Load Info Keys
                                                                      
#endif
                                                                      
                                                      
                          
                       
                                 
                           
                        
                                                                      
#define kOSBundleMachOHeadersKey                "OSBundleMachOHeaders"
#define kOSBundleLogStringsKey                  "OSBundleLogStrings"
#define kOSBundleCPUTypeKey                     "OSBundleCPUType"
#define kOSBundleCPUSubtypeKey                  "OSBundleCPUSubtype"
#define kOSBundlePathKey                        "OSBundlePath"
#define kOSBundleExecutablePathKey              "OSBundleExecutablePath"
#define kOSBundleUUIDKey                        "OSBundleUUID"
#define kOSBundleTextUUIDKey                    "OSBundleTextUUID"
#define kOSBundleStartedKey                     "OSBundleStarted"
#define kOSBundlePrelinkedKey                   "OSBundlePrelinked"
#define kOSBundleLoadTagKey                     "OSBundleLoadTag"
#define kOSBundleLoadAddressKey                 "OSBundleLoadAddress"
#define kOSBundleLoadSizeKey                    "OSBundleLoadSize"
#define kOSBundleExecLoadAddressKey             "OSBundleExecLoadAddress"
#define kOSBundleExecLoadSizeKey                "OSBundleExecLoadSize"
#define kOSBundleWiredSizeKey                   "OSBundleWiredSize"
#define kOSBundleDependenciesKey                "OSBundleDependencies"
#define kOSBundleRetainCountKey                 "OSBundleRetainCount"
#define kOSBundleCacheLoadAddressKey            "OSBundleCacheLoadAddress"
                                
#define kOSBundleKextsInKernelTextKey           "OSBundleKextsInKernelText"
                                                                    
#define kOSBundleAllPrelinkedKey                "OSBundleAllPrelinked"

                                                   
   
#define kOSBundleClassesKey                     "OSBundleClasses"

                                                                            
                                      
   
#define kOSMetaClassNameKey                     "OSMetaClassName"
#define kOSMetaClassSuperclassNameKey           "OSMetaClassSuperclassName"
#define kOSMetaClassTrackingCountKey            "OSMetaClassTrackingCount"

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Kext Collection Type Keys
                                                                      
#endif
#define kKCTypePrimary   "Primary"
#define kKCTypeSystem    "System"
#define kKCTypeAuxiliary "Auxiliary"
#define kKCTypeCodeless  "Codeless"
#define kKCTypeAny       "Any"

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Kext Log Specification
                                                                      
#endif
   
                                
                                                
                                                                              
                   
   

   
                          
                                                     
                                                                          
  
              
                                                                     
                                                      
                                                               
                                                                     
       
                                                                       
                                                                       
                                                                     
                                           
                                                                
                                                                  
                                                                              
                                                                            
        
  
                                                                          
                                                                    
                                                                            
                             
  
                    
  
                                                            
                           
  
                 
                                                                                            
                                        
                                           
                                                                     
                                                                                  
                                                                      
                                                  
                                                                         
                                                                                     
                                                                   
        
  
                  
                                                                        
                                                                         
                                
                                                          
                                            
                                                                          
                               
                                                                         
                                                                          
                                                                   
                                                              
                        
                                                          
                                                                     
                                                                                
                                                                               
                            
  
                        
  
                                            
                                                                        
                                                                           
                                        
  
                                                      
                                                                         
                                                                      
                                                                         
  
                                                
  
                                                        
                                                                               
                                                                                          
                                                                                   
  
                                                                   
                                                                                     
                                                                              
                                                                                          
  
                                                             
                                                                      
                                                                     
  
                                                     
   
typedef uint32_t OSKextLogSpec;

#if PRAGMA_MARK
                                                                      
#pragma mark Masks
                                                                      
#endif
   
                                
                                          
                                                                            
                           
   
#define kOSKextLogLevelMask              ((OSKextLogSpec) 0x00000007)

   
                                       
                                                                 
  
              
                                                          
                                                           
                     
                                                          
                                  
                                     
                                                      
              
  
                                                                            
                                                                       
                                                          
                                  
                                     
   
#define kOSKextLogKextOrGlobalMask       ((OSKextLogSpec) 0x00000008)


   
                                
                                      
                                                                  
   
#define kOSKextLogFlagsMask              ((OSKextLogSpec) 0x0ffffff0)

   
                                
                                      
                                                                 
                                                                
   
#define kOSKextLogVerboseFlagsMask       ((OSKextLogSpec) 0x00000ff0)

   
                                 
                                        
                                                                  
   
#define kOSKextLogConfigMask             ((OSKextLogSpec) 0xf0000000)

#if PRAGMA_MARK
                                                                      
#pragma mark 0xF - Log Level
                                                                      
#endif

   
                                    
                                                                             
                                                         
                                                   
                                               
                                        
   
#define kOSKextLogExplicitLevel          ((OSKextLogSpec)        0x0)

   
                                 
                                                                      
   
#define kOSKextLogErrorLevel            ((OSKextLogSpec)        0x1)


   
                                   
                                                                        
                                                       
                                                                            
   
#define kOSKextLogWarningLevel          ((OSKextLogSpec)        0x2)


   
                                 
                                                                      
                                                                         
   
#define kOSKextLogBasicLevel           ((OSKextLogSpec)        0x3)


   
                                    
                                                                         
                                                          
                                                        
                                                                           
   
#define kOSKextLogProgressLevel          ((OSKextLogSpec)        0x4)


   
                                
                                                                 
                                                                           
                                                                                
   
#define kOSKextLogStepLevel             ((OSKextLogSpec)        0x5)


   
                                  
                                                                      
                                                             
                                                     
   
#define kOSKextLogDetailLevel           ((OSKextLogSpec)        0x6)


   
                                 
                                                                    
                                                                
   
#define kOSKextLogDebugLevel             ((OSKextLogSpec)        0x7)


#if PRAGMA_MARK
                                                                      
#pragma mark 0xF0 - General Activity, Load, Kernel IPC, Personalities
                                                                      
#endif

   
                                  
                                                                    
   
#define kOSKextLogGeneralFlag            ((OSKextLogSpec)       0x10)

   
                               
                                                                                         
                           
   
#define kOSKextLogLoadFlag               ((OSKextLogSpec)       0x20)

   
                              
                                                                             
                                         
   
#define kOSKextLogIPCFlag                ((OSKextLogSpec)       0x40)

   
                                  
                                                                                
                                         
   
#define kOSKextLogArchiveFlag           ((OSKextLogSpec)       0x80)


#if PRAGMA_MARK
                                                                      
#pragma mark 0xF00 - Reserved Verbose Area
                                                                      
#endif
                                                                     
                                                                     
                                                                     
                                                                     

#if PRAGMA_MARK
                                                                      
#pragma mark 0xF000 - Kext diagnostic activity
                                                                      
#endif

   
                                     
                                                            
   
#define kOSKextLogValidationFlag         ((OSKextLogSpec)     0x1000)

   
                                         
                                                                     
                                      
   
#define kOSKextLogAuthenticationFlag     ((OSKextLogSpec)     0x2000)

   
                                       
                                                                             
   
#define kOSKextLogDependenciesFlag       ((OSKextLogSpec)     0x4000)

                                                                     

#if PRAGMA_MARK
                                                                      
#pragma mark 0xF0000 - Archives, caches, directory scan, file access
                                                                      
#endif

   
                                        
                                                                          
                                                                  
   
#define kOSKextLogDirectoryScanFlag      ((OSKextLogSpec)    0x10000)

   
                                     
                                                                               
                                      
   
#define kOSKextLogFileAccessFlag         ((OSKextLogSpec)    0x20000)

   
                                          
                                                                                
   
#define kOSKextLogKextBookkeepingFlag    ((OSKextLogSpec)    0x40000)

                                                                     

#if PRAGMA_MARK
                                                                      
#pragma mark 0xF00000 - Linking & Patching
                                                                      
#endif

   
                               
                                        
   
#define kOSKextLogLinkFlag               ((OSKextLogSpec)   0x100000)

   
                                
                                         
   
#define kOSKextLogPatchFlag              ((OSKextLogSpec)   0x200000)

                                                                     
                                                                     

#if PRAGMA_MARK
                                                                      
#pragma mark 0xF000000 - Reserved
                                                                      
#endif

                                                                     
                                                                     
                                                                     
                                                                     


#if PRAGMA_MARK
                                                                      
#pragma mark 0xF0000000 - Config Flags
                                                                      
#endif

                                                                     
                                                                     
                                                                     

#if PRAGMA_MARK
                                                                      
#pragma mark Predefined Specifications
                                                                      
#endif

   
                                   
                                     
                                                                  
                                                                                      
   
#define kOSKextLogSilentFilter           ((OSKextLogSpec)        0x0)

   
                                    
                                     
                                                                 
                                            
                                                                    
                                            
                                      
   
#define kOSKextLogShowAllFilter          ((OSKextLogSpec) 0x0ffffff7)

   
                                         
                                     
                                       
                                                                    
                                            
                                      
   
#define kOSKextLogShowAllKextsFilter     ((OSKextLogSpec) \
	                                   (kOSKextLogShowAllFilter | \
	                                    kOSKextLogKextOrGlobalMask))

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Kext Version String Processing
                                                                      
#endif
   
                                        
                                                                    
   

   
                           
                                                                          
  
              
                                                                                             
                                                         
                                                    
                                                                   
                                 
  
                                          
                                                  
                                                                             
   
typedef int64_t OSKextVersion;

   
                                     
                                           
                                                           
  
              
                                                                            
                                                                 
                                               
   
#define kOSKextVersionMaxLength (20)
                                                 
                                                                                  

   
                                     
                                    
  
              
                                                                                    
                    
                                                                   
   
#define kOSKextVersionUndefined  (0)

   
                                     
  
            
                                                                                             
  
                                                           
  
          
                                                           
                                       
                                                                
                       
  
              
                                                                            
                                                                 
                                               
   
OSKextVersion OSKextParseVersionString(const char * versionString);


   
                                   
  
            
                                                                                        
                  
  
                   
                                         
                 
                                        
                                                                                          
                                                                
  
          
                                                                      
                                                                    
                                       
                                                                            
  
              
                                                                  
                                      
                                                                      
                                          
                                           
                                       
   
Boolean OSKextVersionGetString(
	OSKextVersion   aVersion,
	char          * buffer,
	uint32_t        bufferSize);


#ifdef KERNEL


#if PRAGMA_MARK
                                                                      
#pragma mark -
#pragma mark Weak linking
                                                                      
#endif
#ifdef XNU_KERNEL_PRIVATE
void kext_weak_symbol_referenced(void) __abortlike;
#endif                         

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Miscellaneous Kernel-Only Kext Functions
                                                                      
#endif

   
                            
                                                                  
  
                                                     
                                                                       
                                   
   
vm_map_t kext_get_vm_map(kmod_info_t * info);

#ifdef XNU_KERNEL_PRIVATE

#if CONFIG_DTRACE
   
                                          
            
                                                                        
                                                  
   
void OSKextRegisterKextsWithDTrace(void);

#endif                    

   
                                  
                                                                       
                            
  
                                                                  
   
void kext_dump_panic_lists(int (*printf_func)(const char *fmt, ...));

#endif                         

#ifdef XNU_KERNEL_PRIVATE

   
                                
                                                                          
   
#define kOSKextReceiptQueried  "OSKextReceiptQueried"

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Kext Loading C Functions
                                                                      
#endif
   
                                                
                                             
  
                                                                        
          
                                         
                                                                                           
                             
  
              
                                                
                                                                  
                                   
  
                                                                        
                                                                         
                                                                   
  
                                                              
      
                                                             
                                           
      
                                                              
                                             
                                                                        
                                    
                                                                                                            
  
                                                            
                                                    
                                                                      
                                                   
                                       
                                                                         
                                                                               
                                                                              
                                                  
                                                                      
   
uint32_t OSKextGetLoadTagForBundleIdentifier(
	const char * kextIdentifier);


   
                                        
                                                          
  
                                                        
          
                                                                                 
                                      
                                                        
                                     
                             
                                                     
                                  
                                         
                                                           
                                                             
                                                    
  
              
                                                                     
                                                        
                                                             
                                           
                        
                                                                      
              
                                                              
                                             
                                                               
  
                                                                      
               
                                                   
                       
                                                                                
   
OSReturn OSKextUnloadKextWithLoadTag(uint32_t loadTag);

#endif                         

#endif             

#if PRAGMA_MARK
#pragma mark -
                                                                      
#pragma mark Loaded Kext Summary
                                                                      
#endif

   
                                          
                                                                    
   
#define kOSKextLoadedKextSummaryVersion 2

   
                                   
                                                      
  
                                            
                               
                                          
                                    
                                                                        
                                      
                                        
                                                  
                                                                                                   
                                                                           
  
              
                                                                            
                                                                          
   
typedef struct _loaded_kext_summary {
	char        name[KMOD_MAX_NAME];
	uuid_t      uuid;
	uint64_t    address;
	uint64_t    size;
	uint64_t    version;
	uint32_t    loadTag;
	uint32_t    flags;
	uint64_t    reference_list;
	uint64_t    text_exec_address;
	size_t      text_exec_size;
} OSKextLoadedKextSummary;

   
                                         
                                                                         
  
                                                                    
                                                         
                                                                       
                               
                                                                             
                     
  
              
                                                                               
                                                          
                                                                             
                                                                     
   
typedef struct _loaded_kext_summary_header {
	uint32_t version;
	uint32_t entry_size;
	uint32_t numSummaries;
	uint32_t reserved;                                  
	OSKextLoadedKextSummary summaries[0];
} OSKextLoadedKextSummaryHeader;

   
                            
                                                                            
   
extern OSKextLoadedKextSummaryHeader * gLoadedKextSummaries;

   
                                     
  
                                                                       
                                                                               
                                                        
   
extern uint64_t gLoadedKextSummariesTimestamp;

   
                                             
                                                               
  
              
                                                                     
                                                                           
                                                                              
   
void OSKextLoadedKextSummariesUpdated(void);

#ifdef XNU_KERNEL_PRIVATE

extern const vm_allocation_site_t * OSKextGetAllocationSiteForCaller(uintptr_t address);
extern uint32_t                     OSKextGetKmodIDForSite(const vm_allocation_site_t * site,
    char * name, vm_size_t namelen);
extern void                         OSKextFreeSite(vm_allocation_site_t * site);
extern kern_return_t                OSKextSetReceiptQueried(void);

#if CONFIG_IMAGEBOOT
extern int OSKextGetUUIDForName(const char *, uuid_t);
#endif

extern vm_tag_t gIOSurfaceTag;

extern void *OSKextKextForAddress(const void *addr);

#endif                         

__END_DECLS

#endif                                    
