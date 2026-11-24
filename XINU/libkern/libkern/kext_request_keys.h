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

#ifndef _LIBKERN_KEXT_REQUEST_KEYS_H
#define _LIBKERN_KEXT_REQUEST_KEYS_H

#ifdef __cplusplus
extern "C" {
#endif                  

                                                                      
                                                                    
                                                                
                                                                    
                                                        
 
                                                                     
                                                          
 
                                                                    
                                                           
                                                        
                                                                      

#if PRAGMA_MARK
                                                                      
#pragma mark Top-Level Request Properties
                                                                      
#endif

                    
                                                                  
                       
   
#define kKextRequestPredicateKey                   "Kext Request Predicate"

                    
                                                                 
                   
   
#define kKextRequestArgumentsKey                   "Kext Request Arguments"

#if PRAGMA_MARK
                                                                      
#pragma mark Request Predicates - User-Space to Kernel
                                                                      
#endif

                                                                      
                                             
  
                                                                     
                                         
            

                                  
                    
                                                                           
                                                                            
  
                                                                          
                                  
   
#define kKextRequestPredicateGetLoaded             "Get Loaded Kext Info"

                                          
                    
                                                                           
                                                                            
  
                                                                          
                                  
   
#define kKextRequestPredicateGetLoadedByUUID       "Get Loaded Kext Info By UUID"

                                             
                                  
                                
                                                                            
  
                                                                       
                             
   
#define kKextRequestPredicateGetUUIDByAddress      "Get Kext UUID by Address"

                                   
                  
                                                                       
                                                                            
  
                                                                          
                         
  
   
#define kKextRequestPredicateGetAllLoadRequests    "Get All Load Requests"

                                     
                                                                     
                                         
                                                                             
                                
                                                                            
  
                                                                          
                                            
                                  
   
#define kKextRequestPredicateGetKextsInCollection   "Get Kexts in Collection"


                                                                      
                                          
  
                                                                 
                                                                     
                                                                      
                                                 
            

                                 
                    
                                                      
                                                                            
  
                                                                       
                                                                       
                             
   
#define kKextRequestPredicateGetKernelRequests     "Get Kernel Requests"

                  
                                                 
                                                                    
                                                                                
  
                                                                      
                                                         
   
#define kKextRequestPredicateLoad                  "Load"

                        
                                             
                                                                    
                                                                                
  
                                                                            
                                                                            
                                                      
   
#define kKextRequestPredicateLoadFromKC            "LoadFromKC"

                              
                                               
                                                                    
                                                                                
  
                                                                     
                                                                             
                                                                       
  
                                                         
   
#define kKextRequestPredicateLoadCodeless          "LoadCodelessKext"

                   
                                                                          
                                                                    
                                                                
  
                                                                       
                                                                      
   
#define kKextRequestPredicateStart                 "Start"

                  
                                                                          
                                                                    
                                                               
  
                                                  
                                            
                                                                  
   
#define kKextRequestPredicateStop                  "Stop"

                    
                                                                          
                                                                    
                                                                      
  
                                                    
                                                                  
   
#define kKextRequestPredicateUnload                "Unload"

                           
                                  
                                                                    
                                                                 
  
                                         
   
#define kKextRequestPredicateLoadFileSetKC        "loadfilesetkc"

                                 
                                                  
                  
                                                    
  
                                                               
                                                                
                                                        
   
#define kKextRequestPredicateMissingAuxKCBundles  "MissingAuxKCBundles"

                                  
                                                                          
                                                                         
                  
                                                    
  
                                                           
   
#define kKextRequestPredicateAuxKCBundleAvailable  "AuxKCBundleAvailable"

                         
                  
                  
                                                                       
  
                                                                   
   
#define kKextRequestPredicateDaemonReady "DaemonReady"

#if PRAGMA_MARK
                                                                      
#pragma mark Requests Predicates - Kernel to User Space (kextd)
                                                                      
#endif
                           
                                               
                                                     
                                         
                                          
                                        
                  
                                                          
  
                                                                           
                                  
   
#define kKextRequestPredicateSendResource          "Send Resource"

                                                                      
                                                               
  
                                                                  
                                                     
            

                               
                                                     
                                                                     
                                                          
  
                                                               
                                                                     
                                                                          
                   
   
#define kKextRequestPredicateRequestLoad           "Kext Load Request"

                                    
                             
                  
                                                          
  
                                                                    
          
   
#define kKextRequestPredicateLoadNotification      "Kext Load Notification"

                                      
                             
                  
                                                          
  
                                                                      
          
   
#define kKextRequestPredicateUnloadNotification    "Kext Unload Notification"

                                      
                  
                  
                                                          
  
                                                                             
                                                          
   
#define kKextRequestPredicateRequestPrelink        "Kext Prelinked Kernel Request"

                                   
                                               
                                                     
                                         
                                                                             
                                                          
  
                                                                        
   
#define kKextRequestPredicateRequestResource       "Kext Resource Request"


                                       
                  
                  
                                                          
  
                                                                            
   
#define kKextRequestPredicateRequestDaemonExit     "IOKit Daemon Exit"

                           
   
#define kKextRequestPredicateRequestKextdExit      kKextRequestPredicateRequestDaemonExit


                                
                                                    
                             
                                                            
                                                          
  
                                                                
   
#define kKextRequestPredicateRequestDaemonLaunch "Dext Daemon Launch"

#if PRAGMA_MARK
                                                                      
#pragma mark -
#pragma mark Generic Request Arguments
                                                                      
#endif
                                
                                                                      
                                       
  
                                                                       
                                                                      
                                                                      
                                                                       
                                                                            
                                                                     
                                                                       
   
#define kKextRequestArgumentLoadRequestsKey        "Kext Load Requests"

                                
                    
                     
  
                                                            
   
#define kKextRequestArgumentBundleIdentifierKey    "CFBundleIdentifier"

                      
                        
                                
  
                                                                
   
#define kKextRequestArgumentInfoKeysKey          "Kext Request Info Keys"

                      
                               
                     
  
                                                             
   
#define kKextRequestArgumentResultKey              "Kext Request Result Code"

                     
                               
                                    
  
                                                  
   
#define kKextRequestArgumentLookupAddressKey       "Kext Request Lookup Address"

                   
                                       
                     
  
                                                                                
   
#define kKextRequestArgumentValueKey               "Value"

                      
                    
                                               
  
                                            
   
#define kKextRequestArgumentNameKey                "Name"

                      
                  
                                               
  
                                                         
   
#define kKextRequestArgumentFileContentsKey        "File Contents"

                              
                     
                   
  
                                                                       
                                                                            
                                                                               
                                             
  
                                                                              
                                                                            
                                                                              
                                                       
  
                                                                              
                                                                            
                                                                             
                                                                   
   
#define kKextRequestArgumentDelayAutounloadKey         "Delay Autounload"

#if PRAGMA_MARK
#pragma mark Load Request Arguments
#endif

                                                                      
                               
  
                                                                
                       
  
                                                                      
                                                                  
                                        
            

                          
                                                          
                                                 
  
                                                                      
                                                                   
                                                                    
                             
   
#define kKextRequestArgumentStartExcludeKey        "Start Exclude Level"

                                          
                                                          
                                                
  
                                                                      
                                                                         
                                       
  
                                                                       
                                                                   
                                                  
   
#define kKextRequestArgumentStartMatchingExcludeKey    "Start Matching Exclude Level"

                            

                               
                              
                                        
  
                                                                              
                                                                       
                                                                             
                                                      
  
                                                                        
            
   
#define kKextRequestArgumentPersonalityNamesKey        "Personality Names"

                                
                                                      
                               
  
                                                                            
                                                                 
  
                                                                       
                                                       
   
#define kKextRequestArgumentCodelessInfoKey            "Codeless Kext Info"


                                    
                      
                             
  
                                                                              
                                                                     
                                                                        
                          
   
#define kKextRequestArgumentCodelessInfoBundlePathKey   "_CodelessKextBundlePath"

#if PRAGMA_MARK
#pragma mark Unload Request Arguments
#endif

                       
                     
                   
  
                                                                           
                                                                       
                                                                               
   
#define kKextRequestArgumentTerminateIOServicesKey     "Terminate IOServices"

#if PRAGMA_MARK
#pragma mark Daemon Launch Request Arguments
#endif

                       
                    
                
  
                                                                     
                                                                    
                                                                   
   
#define kKextRequestArgumentDriverExtensionServerTag   "Driver Extension Server Tag"

                        
                   
                
  
                                                                      
                                                                      
                                                                     
   
#define kKextRequestArgumentDriverExtensionServerName  "Driver Extension Server Name"

#if PRAGMA_MARK
#pragma mark Missing AuxKC Bundles Arguments
#endif

                               
                  
                
                                                     
  
                                                                        
                                                   
   
#define kKextRequestArgumentMissingBundleIDs           "Missing Bundle IDs"

                                
                    
                 
                                                      
  
                                                                      
                                                    
   
#define kKextRequestArgumentBundleAvailability         "Bundle Availability"

#if PRAGMA_MARK
#pragma mark Internal Tracking Properties
#endif
                                                                      
                               
            

                         
                               
                                                           
  
                                                   
   
#define kKextRequestArgumentRequestTagKey              "Request Tag"

                              
                            
                               
  
                                                   
   
#define kKextRequestArgumentCallbackKey                "Request Callback"

                              
                                     
                     
   
#define kKextRequestArgumentContextKey                 "Context"

                           
                     
                               
  
                                                                  
                                                   
   
#define kKextRequestStaleKey                           "Request Stale"

                            
                             
                                     
   
#define kKextRequestArgumentCheckInToken               "Check In Token"

#if PRAGMA_MARK
#pragma mark fileset load request arguments
#endif

                            
                           
                                                
  
                                               
   
#define kKextRequestArgumentPageableKCFilename         "PageableKCName"

                       
                           
                                                
  
                                          
   
#define kKextRequestArgumentAuxKCFilename              "AuxKCName"

                                    
                                   
                                                
  
                                                                  
   
#define kKextRequestArgumentCodelessPersonalities       "Codeless Personalities"

#if PRAGMAA_MARK
#pragma mark kext collection request arguments
#endif

                        
                    
                                                       
  
                                                           
   
#define kKextRequestArgumentCollectionTypeKey         "Collection Type"

                         
                    
                                   
                 
                                                       
  
                                                                       
                                                    
                                                      
                                                   
   
#define kKextRequestArgumentLoadedStateKey             "Loaded State"

#ifdef __cplusplus
};
#endif                  

#endif                                   
