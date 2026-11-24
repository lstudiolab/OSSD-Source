/*
 * Copyright (c) 2001-2010 by Apple Inc.. All rights reserved.
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
 * @APPLE_LICENSE_HEADER_END@
 */

  
                                     
 
                                                

                                                                    

                                                                      
                                                                    
                                                                           
                             
                 
                                                   
                                                  
                      
                                                                          
                                                                            
                                                                         
                                                                             
                                                                 
                
                                                                     
                
                                                                           
                                                                           
                     
                                          
                
                                                                                       
                                                                                                                               
                                                                                                                               
                                                                                       
                                                                                       
                                                                                       
                                                                                       
                     
                                                                                      
                                                                                      
                                                                                      
                                                                                      
                                                                                      

                                                                                          
                
                                                                  
                                                                     
                                                                    
                                                                    
                                                                    
                                                                    
                                                                    
                                                                                                      
                                                                 
                                       
                                       
                                       
                                      
                                      
                  
  
  
#ifndef __AVAILABILITYMACROS__
#define __AVAILABILITYMACROS__


  
                                    
   
#define MAC_OS_X_VERSION_10_0         1000
#define MAC_OS_X_VERSION_10_1         1010
#define MAC_OS_X_VERSION_10_2         1020
#define MAC_OS_X_VERSION_10_3         1030
#define MAC_OS_X_VERSION_10_4         1040
#define MAC_OS_X_VERSION_10_5         1050
#define MAC_OS_X_VERSION_10_6         1060
#define MAC_OS_X_VERSION_10_7         1070
#define MAC_OS_X_VERSION_10_8         1080
#define MAC_OS_X_VERSION_10_9         1090
#define MAC_OS_X_VERSION_10_10      101000
#define MAC_OS_X_VERSION_10_10_2    101002
#define MAC_OS_X_VERSION_10_10_3    101003
#define MAC_OS_X_VERSION_10_11      101100
#define MAC_OS_X_VERSION_10_11_2    101102
#define MAC_OS_X_VERSION_10_11_3    101103
#define MAC_OS_X_VERSION_10_11_4    101104
#define MAC_OS_X_VERSION_10_12      101200
#define MAC_OS_X_VERSION_10_12_1    101201
#define MAC_OS_X_VERSION_10_12_2    101202
#define MAC_OS_X_VERSION_10_12_4    101204

   
                                                 
                                                                                                                                   
   
#ifndef MAC_OS_X_VERSION_MIN_REQUIRED
    #ifdef __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
        #if (__i386__ || __x86_64__) && (__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < MAC_OS_X_VERSION_10_4)
            #warning Building for Intel with Mac OS X Deployment Target < 10.4 is invalid.
        #endif
        #define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
    #else
        #if __i386__ || __x86_64__
            #define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_4
        #elif __arm__ || __arm64__
            #define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_5
        #else
            #define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_1
        #endif
     #endif
#endif

  
                                                           
   
#ifndef MAC_OS_X_VERSION_MAX_ALLOWED
    #if MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_12_4
        #define MAC_OS_X_VERSION_MAX_ALLOWED MAC_OS_X_VERSION_MIN_REQUIRED
    #else
        #define MAC_OS_X_VERSION_MAX_ALLOWED MAC_OS_X_VERSION_10_12_4
    #endif
#endif

  
                      
   
#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_MIN_REQUIRED
    #error MAC_OS_X_VERSION_MAX_ALLOWED must be >= MAC_OS_X_VERSION_MIN_REQUIRED
#endif
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_0
    #error MAC_OS_X_VERSION_MIN_REQUIRED must be >= MAC_OS_X_VERSION_10_0
#endif

  
                                                              
   
#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))) && (MAC_OS_X_VERSION_MIN_REQUIRED >= 1020)
    #define WEAK_IMPORT_ATTRIBUTE __attribute__((weak_import))
#elif defined(__MWERKS__) && (__MWERKS__ >= 0x3205) && (MAC_OS_X_VERSION_MIN_REQUIRED >= 1020) && !defined(__INTEL__)
    #define WEAK_IMPORT_ATTRIBUTE __attribute__((weak_import))
#else
    #define WEAK_IMPORT_ATTRIBUTE
#endif

  
                                                             
   
#if defined(__has_feature) && defined(__has_attribute)
    #if __has_attribute(deprecated)
        #define DEPRECATED_ATTRIBUTE        __attribute__((deprecated))
        #if __has_feature(attribute_deprecated_with_message)
            #define DEPRECATED_MSG_ATTRIBUTE(s) __attribute__((deprecated(s)))
        #else
            #define DEPRECATED_MSG_ATTRIBUTE(s) __attribute__((deprecated))
        #endif
    #else
        #define DEPRECATED_ATTRIBUTE
        #define DEPRECATED_MSG_ATTRIBUTE(s)
    #endif
#elif defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
    #define DEPRECATED_ATTRIBUTE        __attribute__((deprecated))
    #if (__GNUC__ >= 5) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5))
        #define DEPRECATED_MSG_ATTRIBUTE(s) __attribute__((deprecated(s)))
    #else
        #define DEPRECATED_MSG_ATTRIBUTE(s) __attribute__((deprecated))
    #endif
#else
    #define DEPRECATED_ATTRIBUTE
    #define DEPRECATED_MSG_ATTRIBUTE(s)
#endif

  
                                                              
   
#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
    #define UNAVAILABLE_ATTRIBUTE __attribute__((unavailable))
#else
    #define UNAVAILABLE_ATTRIBUTE
#endif


  
                                            
   
                                                 
   
#define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER

  
                                                           
   
                                                  
                                  
   
#define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE

  
                                                
   
                                             
   
#define DEPRECATED_IN_MAC_OS_X_VERSION_10_0_AND_LATER     DEPRECATED_ATTRIBUTE

#ifndef __AVAILABILITY_MACROS_USES_AVAILABILITY
	#ifdef __has_attribute
		#if __has_attribute(availability)
			#include <Availability.h>
			#define __AVAILABILITY_MACROS_USES_AVAILABILITY 1
		#endif
	#endif
#endif

  
                                            
   
                                                    
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_1, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                           
  
                                                    
                                  
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_1    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                
  
                                            
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_1_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_1
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_1_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_1_AND_LATER
#endif


  
                                            
   
                                                    
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_2, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                           
  
                                                    
                                  
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_2    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_2    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                
  
                                            
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_2_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_2
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_2_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_2_AND_LATER
#endif


  
                                            
   
                                                    
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_3, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                           
  
                                                    
                                  
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_3    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_3    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_3    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                
  
                                            
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_3_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_3_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_3_AND_LATER
#endif


  
                                            
   
                                                    
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_4, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                           
  
                                                    
                                  
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_4    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                
  
                                            
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_4_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_4_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_4_AND_LATER
#endif


  
                                            
   
                                                    
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                           
  
                                                    
                                  
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_5, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_5, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_5, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_5, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_5, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_5, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_5    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                
  
                                            
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_5_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_5, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_5_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_5_AND_LATER
#endif


  
                                            
   
                                                    
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_6
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_6
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                           
  
                                                    
                                  
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_6, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_6, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_6, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_6, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_6, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_6, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_6    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                
  
                                            
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_6_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_6, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_6_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_6_AND_LATER
#endif


  
                                            
   
                                                    
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_7
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                           
  
                                                    
                                  
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_7, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_7, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_7, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_7, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_7, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_7, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_7, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_7, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_7    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                
  
                                            
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_7, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#endif


  
                                            
   
                                                    
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_8, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                           
  
                                                    
                                  
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_8, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_8, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_8, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_8, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_8, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_8, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_8, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_8, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_8, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_8    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                
  
                                            
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_8_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_8, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_8
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_8_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_8_AND_LATER
#endif


  
                                            
   
                                                    
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_9, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                           
  
                                                    
                                  
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                    
  
                                                    
                                        
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_9    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                
  
                                            
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_9_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_9, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_9
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_9_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_9_AND_LATER
#endif


  
                                             
   
                                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_10, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                            
  
                                                     
                                   
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                 
  
                                             
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_10_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_10, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_10_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_10_AND_LATER
#endif


  
                                               
   
                                                       
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_10_2, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                              
  
                                                       
                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_2, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                   
  
                                               
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_10_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_2
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif


  
                                               
   
                                                       
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_10_3, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif

  
                                                              
  
                                                       
                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_3, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_2, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3    AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                   
  
                                               
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_10_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10_3
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif


  
                                             
   
                                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_11, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER
#endif

  
                                                            
  
                                                     
                                   
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                      
  
                                                     
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                                                        
  
                                                       
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_2, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                                                        
  
                                                       
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_3, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11    AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif

  
                                                 
  
                                             
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_11, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_AND_LATER
#endif


  
                                               
   
                                                       
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_11_2, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER
#endif

  
                                                              
  
                                                       
                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_2, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_2, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_3, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2    AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER
#endif

  
                                                   
  
                                               
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_11_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_2
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_2_AND_LATER
#endif


  
                                               
   
                                                       
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_11_3, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER
#endif

  
                                                              
  
                                                       
                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_3, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_2, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_3, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_2, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3    AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER
#endif

  
                                                   
  
                                               
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_11_3, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_3
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_3_AND_LATER
#endif


  
                                               
   
                                                       
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_11_4, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER
#endif

  
                                                              
  
                                                       
                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_4, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_2, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_3, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_2, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_3, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4    AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER
#endif

  
                                                   
  
                                               
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_11_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_11_4
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_11_4_AND_LATER
#endif


  
                                             
   
                                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_12, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER
#endif

  
                                                            
  
                                                     
                                   
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_12, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                     
  
                                                    
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                      
  
                                                     
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                                                        
  
                                                       
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_2, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                                                        
  
                                                       
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_3, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif

  
                                                                                      
  
                                                     
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER
#endif

  
                                                                                        
  
                                                       
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_2, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER
#endif

  
                                                                                        
  
                                                       
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_3, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER
#endif

  
                                                                                        
  
                                                       
                                         
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_4, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12    AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER
#endif

  
                                                 
  
                                             
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_12, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_AND_LATER
#endif


  
                                               
   
                                                       
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_12_1, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER
#endif

  
                                                              
  
                                                       
                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_12_1, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_2, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_3, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_2, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_3, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_4, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_12, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1    AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER
#endif

  
                                                   
  
                                               
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_12_1, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_1
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_1_AND_LATER
#endif


  
                                               
   
                                                       
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_12_2, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER
#endif

  
                                                              
  
                                                       
                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_12_2, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_2, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_3, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_2, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_3, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_4, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_12, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_12_1, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2    AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER
#endif

  
                                                   
  
                                               
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_12_2, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_2
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_2_AND_LATER
#endif


  
                                               
   
                                                       
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_4_AND_LATER     __OSX_AVAILABLE_STARTING(__MAC_10_12_4, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_4_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_4_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_4_AND_LATER
#endif

  
                                                              
  
                                                       
                                     
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_4_AND_LATER_BUT_DEPRECATED     __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_12_4, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_4_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_4_AND_LATER_BUT_DEPRECATED    AVAILABLE_MAC_OS_X_VERSION_10_12_4_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_1, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_1_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_2, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_3, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_4, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_6, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_7, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_8, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
#endif

  
                                                                                       
  
                                                    
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_9, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_9_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_10_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_2, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_10_2_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_10_3, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_10_3_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_2, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_11_2_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_3, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_11_3_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_11_4, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_11_4_AND_LATER
#endif

  
                                                                                        
  
                                                     
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_12, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_12_1, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_12_1_AND_LATER
#endif

  
                                                                                          
  
                                                       
                                           
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_12_2, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER_BUT_DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4    AVAILABLE_MAC_OS_X_VERSION_10_12_2_AND_LATER
#endif

  
                                                   
  
                                               
   
#if __AVAILABILITY_MACROS_USES_AVAILABILITY
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4_AND_LATER    __OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_12_4, __IPHONE_4_0, __IPHONE_4_0)
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12_4
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_MAC_OS_X_VERSION_10_12_4_AND_LATER
#endif




#endif                              


