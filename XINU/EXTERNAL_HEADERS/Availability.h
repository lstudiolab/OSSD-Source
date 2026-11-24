/*
 * Copyright (c) 2007-2016 by Apple Inc.. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */
 
#ifndef __AVAILABILITY__
#define __AVAILABILITY__
        
                                                                                
                                                                          
                                                                           
                        
     
                                                                     
                                                                               
                                           
                                                         
                                                                                 
                                                 
    
                                                                            
                                                 
                                                            
                                                                               
                                            

                                                                         
                                                                               
                                                                            
                                                                           
                                                                            
                                                                           
                                                                                     
                                                                        
                                                                       
                               
    
                                                                                       
                                                                                   
                                                                                        
                                                                                       
                                                                                        
                                                                   
    
             

                                                                              
        
                                                                                     


                                                                               
        
                                         
                                                                                 
                

        
                                                                      
        
                                                
                                    
                  
                                                                         
                                                                                  
        

                                                                 
  
         
                                                    
                                                    
                                                    
       
                                                                
                                                          
                                                         
     

                                                                                    
                                                                                  
                                                                                         
                                                                                    
                                                                                 
                                                                                      
        
    
                                              
                                                                        
                                                     
                                                      
                                                           
                 
                                                        
                  
              


  

#define __MAC_10_0            1000
#define __MAC_10_1            1010
#define __MAC_10_2            1020
#define __MAC_10_3            1030
#define __MAC_10_4            1040
#define __MAC_10_5            1050
#define __MAC_10_6            1060
#define __MAC_10_7            1070
#define __MAC_10_8            1080
#define __MAC_10_9            1090
#define __MAC_10_10         101000
#define __MAC_10_10_2       101002
#define __MAC_10_10_3       101003
#define __MAC_10_11         101100
#define __MAC_10_11_2       101102
#define __MAC_10_11_3       101103
#define __MAC_10_11_4       101104
#define __MAC_10_12         101200
#define __MAC_10_12_1       101201
#define __MAC_10_12_2       101202
#define __MAC_10_12_4       101204
                                                                                                                 

#define __IPHONE_2_0      20000
#define __IPHONE_2_1      20100
#define __IPHONE_2_2      20200
#define __IPHONE_3_0      30000
#define __IPHONE_3_1      30100
#define __IPHONE_3_2      30200
#define __IPHONE_4_0      40000
#define __IPHONE_4_1      40100
#define __IPHONE_4_2      40200
#define __IPHONE_4_3      40300
#define __IPHONE_5_0      50000
#define __IPHONE_5_1      50100
#define __IPHONE_6_0      60000
#define __IPHONE_6_1      60100
#define __IPHONE_7_0      70000
#define __IPHONE_7_1      70100
#define __IPHONE_8_0      80000
#define __IPHONE_8_1      80100
#define __IPHONE_8_2      80200
#define __IPHONE_8_3      80300
#define __IPHONE_8_4      80400
#define __IPHONE_9_0      90000
#define __IPHONE_9_1      90100
#define __IPHONE_9_2      90200
#define __IPHONE_9_3      90300
#define __IPHONE_10_0    100000
#define __IPHONE_10_1    100100
#define __IPHONE_10_2    100200
#define __IPHONE_10_3    100300
                                                                                                                    

#define __TVOS_9_0        90000
#define __TVOS_9_1        90100
#define __TVOS_9_2        90200
#define __TVOS_10_0      100000
#define __TVOS_10_0_1    100001
#define __TVOS_10_1      100100
#define __TVOS_10_2      100200

#define __WATCHOS_1_0     10000
#define __WATCHOS_2_0     20000
#define __WATCHOS_3_0     30000
#define __WATCHOS_3_1     30100
#define __WATCHOS_3_1_1   30101
#define __WATCHOS_3_2     30200

#include <AvailabilityInternal.h>

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
    #define __OSX_AVAILABLE_STARTING(_osx, _ios) __AVAILABILITY_INTERNAL##_ios
    #define __OSX_AVAILABLE_BUT_DEPRECATED(_osxIntro, _osxDep, _iosIntro, _iosDep) \
                                                    __AVAILABILITY_INTERNAL##_iosIntro##_DEP##_iosDep
    #define __OSX_AVAILABLE_BUT_DEPRECATED_MSG(_osxIntro, _osxDep, _iosIntro, _iosDep, _msg) \
                                                    __AVAILABILITY_INTERNAL##_iosIntro##_DEP##_iosDep##_MSG(_msg)

#elif defined(__MAC_OS_X_VERSION_MIN_REQUIRED)
    #define __OSX_AVAILABLE_STARTING(_osx, _ios) __AVAILABILITY_INTERNAL##_osx
    #define __OSX_AVAILABLE_BUT_DEPRECATED(_osxIntro, _osxDep, _iosIntro, _iosDep) \
                                                    __AVAILABILITY_INTERNAL##_osxIntro##_DEP##_osxDep
    #define __OSX_AVAILABLE_BUT_DEPRECATED_MSG(_osxIntro, _osxDep, _iosIntro, _iosDep, _msg) \
                                                    __AVAILABILITY_INTERNAL##_osxIntro##_DEP##_osxDep##_MSG(_msg)

#else
    #define __OSX_AVAILABLE_STARTING(_osx, _ios)
    #define __OSX_AVAILABLE_BUT_DEPRECATED(_osxIntro, _osxDep, _iosIntro, _iosDep)
    #define __OSX_AVAILABLE_BUT_DEPRECATED_MSG(_osxIntro, _osxDep, _iosIntro, _iosDep, _msg)
#endif


#if defined(__has_feature)
  #if __has_feature(attribute_availability_with_message)
    #define __OS_AVAILABILITY(_target, _availability)            __attribute__((availability(_target,_availability)))
    #define __OS_AVAILABILITY_MSG(_target, _availability, _msg)  __attribute__((availability(_target,_availability,message=_msg)))
  #elif __has_feature(attribute_availability)
    #define __OS_AVAILABILITY(_target, _availability)            __attribute__((availability(_target,_availability)))
    #define __OS_AVAILABILITY_MSG(_target, _availability, _msg)  __attribute__((availability(_target,_availability)))
  #else
    #define __OS_AVAILABILITY(_target, _availability)
    #define __OS_AVAILABILITY_MSG(_target, _availability, _msg)
  #endif
#else
    #define __OS_AVAILABILITY(_target, _availability)
    #define __OS_AVAILABILITY_MSG(_target, _availability, _msg)
#endif


                                             
#if defined(__has_feature)
  #if __has_feature(attribute_availability_app_extension)
    #define __OSX_EXTENSION_UNAVAILABLE(_msg)  __OS_AVAILABILITY_MSG(macosx_app_extension,unavailable,_msg)
    #define __IOS_EXTENSION_UNAVAILABLE(_msg)  __OS_AVAILABILITY_MSG(ios_app_extension,unavailable,_msg)
  #else
    #define __OSX_EXTENSION_UNAVAILABLE(_msg)
    #define __IOS_EXTENSION_UNAVAILABLE(_msg)
  #endif
#else
    #define __OSX_EXTENSION_UNAVAILABLE(_msg)
    #define __IOS_EXTENSION_UNAVAILABLE(_msg)
#endif

#define __OS_EXTENSION_UNAVAILABLE(_msg)  __OSX_EXTENSION_UNAVAILABLE(_msg) __IOS_EXTENSION_UNAVAILABLE(_msg)



                                                     
#if defined(__has_attribute)
  #if __has_attribute(availability)
    #define __OSX_UNAVAILABLE                    __OS_AVAILABILITY(macosx,unavailable)
    #define __OSX_AVAILABLE(_vers)               __OS_AVAILABILITY(macosx,introduced=_vers)
    #define __OSX_DEPRECATED(_start, _dep, _msg) __OSX_AVAILABLE(_start) __OS_AVAILABILITY_MSG(macosx,deprecated=_dep,_msg)
  #endif
#endif

#ifndef __OSX_UNAVAILABLE
  #define __OSX_UNAVAILABLE
#endif

#ifndef __OSX_AVAILABLE
  #define __OSX_AVAILABLE(_vers)
#endif

#ifndef __OSX_DEPRECATED
  #define __OSX_DEPRECATED(_start, _dep, _msg)
#endif


                                                 
#if defined(__has_attribute)
  #if __has_attribute(availability)
    #define __IOS_UNAVAILABLE                    __OS_AVAILABILITY(ios,unavailable)
    #define __IOS_PROHIBITED                     __OS_AVAILABILITY(ios,unavailable)
    #define __IOS_AVAILABLE(_vers)               __OS_AVAILABILITY(ios,introduced=_vers)
    #define __IOS_DEPRECATED(_start, _dep, _msg) __IOS_AVAILABLE(_start) __OS_AVAILABILITY_MSG(ios,deprecated=_dep,_msg)
  #endif
#endif

#ifndef __IOS_UNAVAILABLE
  #define __IOS_UNAVAILABLE
#endif

#ifndef __IOS_PROHIBITED
  #define __IOS_PROHIBITED
#endif

#ifndef __IOS_AVAILABLE
  #define __IOS_AVAILABLE(_vers)
#endif

#ifndef __IOS_DEPRECATED
  #define __IOS_DEPRECATED(_start, _dep, _msg)
#endif


                                                  
#if defined(__has_feature)
  #if __has_feature(attribute_availability_tvos)
    #define __TVOS_UNAVAILABLE                    __OS_AVAILABILITY(tvos,unavailable)
    #define __TVOS_PROHIBITED                     __OS_AVAILABILITY(tvos,unavailable)
    #define __TVOS_AVAILABLE(_vers)               __OS_AVAILABILITY(tvos,introduced=_vers)
    #define __TVOS_DEPRECATED(_start, _dep, _msg) __TVOS_AVAILABLE(_start) __OS_AVAILABILITY_MSG(tvos,deprecated=_dep,_msg)
  #endif
#endif

#ifndef __TVOS_UNAVAILABLE
  #define __TVOS_UNAVAILABLE
#endif

#ifndef __TVOS_PROHIBITED
  #define __TVOS_PROHIBITED
#endif

#ifndef __TVOS_AVAILABLE
  #define __TVOS_AVAILABLE(_vers)
#endif

#ifndef __TVOS_DEPRECATED
  #define __TVOS_DEPRECATED(_start, _dep, _msg)
#endif


                                                      
#if defined(__has_feature)
  #if __has_feature(attribute_availability_watchos)
    #define __WATCHOS_UNAVAILABLE                    __OS_AVAILABILITY(watchos,unavailable)
    #define __WATCHOS_PROHIBITED                     __OS_AVAILABILITY(watchos,unavailable)
    #define __WATCHOS_AVAILABLE(_vers)               __OS_AVAILABILITY(watchos,introduced=_vers)
    #define __WATCHOS_DEPRECATED(_start, _dep, _msg) __WATCHOS_AVAILABLE(_start) __OS_AVAILABILITY_MSG(watchos,deprecated=_dep,_msg)
  #endif
#endif

#ifndef __WATCHOS_UNAVAILABLE
  #define __WATCHOS_UNAVAILABLE
#endif

#ifndef __WATCHOS_PROHIBITED
  #define __WATCHOS_PROHIBITED
#endif

#ifndef __WATCHOS_AVAILABLE
  #define __WATCHOS_AVAILABLE(_vers)
#endif

#ifndef __WATCHOS_DEPRECATED
  #define __WATCHOS_DEPRECATED(_start, _dep, _msg)
#endif


                                                
#if defined(__has_feature)
  #if __has_feature(attribute_availability_swift)
    #define __SWIFT_UNAVAILABLE                   __OS_AVAILABILITY(swift,unavailable)
    #define __SWIFT_UNAVAILABLE_MSG(_msg)         __OS_AVAILABILITY_MSG(swift,unavailable,_msg)
  #endif
#endif

#ifndef __SWIFT_UNAVAILABLE
  #define __SWIFT_UNAVAILABLE
#endif

#ifndef __SWIFT_UNAVAILABLE_MSG
  #define __SWIFT_UNAVAILABLE_MSG(_msg)
#endif

#if __has_include(<AvailabilityProhibitedInternal.h>)
  #include <AvailabilityProhibitedInternal.h>
#endif

  
                                                                        
 
                                                                     
   

  
                    
  
                                                                     
  
                  
                              
  
            
                                   
                                             
                                                                      
   
#define __API_AVAILABLE(...) __API_AVAILABLE_GET_MACRO(__VA_ARGS__,__API_AVAILABLE4, __API_AVAILABLE3, __API_AVAILABLE2, __API_AVAILABLE1)(__VA_ARGS__)


  
                   
  
                                                                       
  
                  
                              
  
            
  
                                                                
                                                                                                                   
  
                                                                                      
                                                                                              
   
#define __API_DEPRECATED(...) __API_DEPRECATED_MSG_GET_MACRO(__VA_ARGS__,__API_DEPRECATED_MSG5,__API_DEPRECATED_MSG4,__API_DEPRECATED_MSG3,__API_DEPRECATED_MSG2,__API_DEPRECATED_MSG1)(__VA_ARGS__)
#define __API_DEPRECATED_WITH_REPLACEMENT(...) __API_DEPRECATED_REP_GET_MACRO(__VA_ARGS__,__API_DEPRECATED_REP5,__API_DEPRECATED_REP4,__API_DEPRECATED_REP3,__API_DEPRECATED_REP2,__API_DEPRECATED_REP1)(__VA_ARGS__)

  
                     
                                                                       
  
           
                              
                                      
   
#define __API_UNAVAILABLE(...) __API_UNAVAILABLE_GET_MACRO(__VA_ARGS__,__API_UNAVAILABLE3,__API_UNAVAILABLE2,__API_UNAVAILABLE1)(__VA_ARGS__)

#endif                       
