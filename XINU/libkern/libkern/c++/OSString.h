/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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
                                                      
                                                           

#ifndef _OS_OSSTRING_H
#define _OS_OSSTRING_H

#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSPtr.h>
#include <os/base.h>

class OSData;
class OSString;

typedef OSString* OSStringPtr;
typedef OSString const* OSStringConstPtr;

   
          
  
            
                                                     
   


                                   
  
                    
   
enum { kOSStringNoCopy = 0x00000001 };


   
                  
  
            
                                                                            
  
              
                                                                   
                                                                              
                                             
                                       
                                               
                                                                              
  
                   
  
                                                                     
                                                                                
                                                                            
                                                                           
  
                          
  
                                                                     
                                  
                                                                  
                                                               
                                           
                                 
                                       
                                            
                                                            
                               
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                               
                                                                    
                                        
                                                                     
                                                                  
              
   
class OSString : public OSObject
{
	OSDeclareDefaultStructors(OSString);

	enum { kMaxStringLength  = 262142 };

#if APPLE_KEXT_ALIGN_CONTAINERS

protected:

	unsigned int   flags:14,
	    length:18;
	char         * OS_PTRAUTH_SIGNED_PTR("OSString.string") string;

#else                                  

protected:
	char         * OS_PTRAUTH_SIGNED_PTR("OSString.string") string;
	unsigned int   flags;
	unsigned int   length;

#endif                                  

public:

   
                       
  
            
                                                             
  
                                                               
  
          
                                       
                                               
                                   
                                
  
              
                                                                     
                                        
                                        
                                                     
   
	static OSPtr<OSString> withString(const OSString * aString);


   
                        
  
            
                                                       
  
                                                               
  
          
                                       
                                               
                                   
                                
   
	static OSPtr<OSString> withCString(const char * cString);


   
                              
  
            
                                                
                                            
  
                                              
  
          
                                                           
                                   
                                
  
              
                                                
                                            
                                 
                                                                               
                                            
                                                  
                                                                        
  
                                                         
                                                                          
   
	static OSPtr<OSString> withCStringNoCopy(const char * cString);

#if XNU_KERNEL_PRIVATE
	static OSPtr<OSString> withStringOfLength(const char *cString, size_t length);
#endif                          

   
                           
  
            
                                                 
  
                                                               
  
          
                                                               
  
              
                                                               
                                                          
   
	virtual bool initWithString(const OSString * aString);


   
                            
  
            
                                           
  
                                                               
  
          
                                                               
  
              
                                                               
                                                            
   
	virtual bool initWithCString(const char * cString);


   
                                  
  
            
                                    
                                         
  
                                              
  
          
                                                               
  
              
                                                               
                                                                        
  
                                                    
                                            
                                 
                                                                               
                                            
                                                  
                                                                        
  
                                                         
                                                                          
   
	virtual bool initWithCStringNoCopy(const char * cString);

#if XNU_KERNEL_PRIVATE
	bool initWithStringOfLength(const char *cString, size_t inlength);
#endif                          

   
                 
  
            
                                        
                                 
  
              
                                               
      
              
                                                        
                       
           
   
	virtual void free() APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                                           
  
          
                                                   
   
	virtual unsigned int getLength() const;


   
                    
  
            
                                                               
  
                                          
  
          
                                                         
                                                               
   
	virtual char getChar(unsigned int index) const;


   
                    
  
            
                                                              
  
                                           
                                          
  
          
                                                   
                                                 
                                                       
   
	virtual bool setChar(char aChar, unsigned int index);


   
                             
  
            
                                                     
  
          
                                             
   
	virtual const char * getCStringNoCopy() const;


   
                      
  
            
                                              
  
                                                                           
  
          
                                                                
                                
  
              
                                                                     
                                                    
   
	virtual bool isEqualTo(const OSString * aString) const;


   
                      
  
            
                                                            
  
                                                                
  
          
                                                 
                                    
                                
   
	virtual bool isEqualTo(const char * cString) const;


   
                      
  
            
                                                                   
  
                                                                  
  
          
                                                               
                                
  
              
                                                    
                                          
                                                        
   
	virtual bool isEqualTo(const OSMetaClassBase * anObject) const APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                                                    
  
                                       
  
          
                                                                                     
  
              
                                                        
                                 
                                                
                                 
                                         
                                                 
                                         
                      
                                                          
   
	virtual bool isEqualTo(const OSData * aDataObject) const;


   
                      
  
            
                                          
                                                                    
  
                                            
  
          
                                                                          
   
	virtual bool serialize(OSSerialize * serializer) const APPLE_KEXT_OVERRIDE;

	OSMetaClassDeclareReservedUnused(OSString, 0);
	OSMetaClassDeclareReservedUnused(OSString, 1);
	OSMetaClassDeclareReservedUnused(OSString, 2);
	OSMetaClassDeclareReservedUnused(OSString, 3);
	OSMetaClassDeclareReservedUnused(OSString, 4);
	OSMetaClassDeclareReservedUnused(OSString, 5);
	OSMetaClassDeclareReservedUnused(OSString, 6);
	OSMetaClassDeclareReservedUnused(OSString, 7);
	OSMetaClassDeclareReservedUnused(OSString, 8);
	OSMetaClassDeclareReservedUnused(OSString, 9);
	OSMetaClassDeclareReservedUnused(OSString, 10);
	OSMetaClassDeclareReservedUnused(OSString, 11);
	OSMetaClassDeclareReservedUnused(OSString, 12);
	OSMetaClassDeclareReservedUnused(OSString, 13);
	OSMetaClassDeclareReservedUnused(OSString, 14);
	OSMetaClassDeclareReservedUnused(OSString, 15);
};

#endif                      
