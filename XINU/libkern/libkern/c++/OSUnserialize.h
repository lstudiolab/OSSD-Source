/*
 * Copyright (c) 2000-2016 Apple Inc. All rights reserved.
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
                                                           

#ifndef _OS_OSUNSERIALIZE_H
#define _OS_OSUNSERIALIZE_H

#include <libkern/c++/OSMetaClass.h>
#include <libkern/c++/OSString.h>
#include <libkern/c++/OSPtr.h>

#include <sys/appleapiopts.h>
#include <sys/types.h>

class OSObject;
class OSString;

   
          
  
            
                                                                   
   


   
                             
  
            
                                  
                                                                  
  
                                                                 
                                                              
                                                                  
                                                            
                                                                         
                                                    
  
          
                                                         
  
              
                                                          
   
extern "C++" OSPtr<OSObject> OSUnserializeXML(
	const char  * buffer,
	OSString * * errorString = NULL);

extern "C++" OSPtr<OSObject> OSUnserializeXML(
	const char  * buffer,
	OSSharedPtr<OSString>& errorString);

   
                             
  
            
                                  
                                                                  
  
                                                                 
                                                              
                                                                   
                                                                    
                                                                  
                                                            
                                                                         
                                                    
  
          
                                                         
  
              
                                                          
   
extern "C++" OSPtr<OSObject> OSUnserializeXML(
	const char  * buffer,
	size_t        bufferSize,
	OSString *   *errorString = NULL);

extern "C++" OSPtr<OSObject> OSUnserializeXML(
	const char  * buffer,
	size_t        bufferSize,
	OSSharedPtr<OSString> &errorString);

extern "C++" OSPtr<OSObject>
OSUnserializeBinary(const char *buffer, size_t bufferSize, OSString * *errorString);

extern "C++" OSPtr<OSObject>
OSUnserializeBinary(const char *buffer, size_t bufferSize, OSSharedPtr<OSString>& errorString);

#ifdef __APPLE_API_OBSOLETE
extern OSPtr<OSObject> OSUnserialize(const char *buffer, OSString * *errorString = NULL);

extern OSPtr<OSObject> OSUnserialize(const char *buffer, OSSharedPtr<OSString>&  errorString);

#endif                           

#endif                          
