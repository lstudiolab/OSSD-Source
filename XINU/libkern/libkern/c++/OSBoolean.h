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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
                                                         

#ifndef _OS_OSBOOLEAN_H
#define _OS_OSBOOLEAN_H

#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSPtr.h>

class OSString;
class OSBoolean;

typedef OSBoolean* OSBooleanPtr;

   
          
  
            
                                                      
   


   
                   
  
            
                                                  
                                  
  
              
                                                                            
                           
                                             
                                                         
                                                                
                                                                           
                                                    
                                       
                                                   
   
class OSBoolean : public OSObject
{
	OSDeclareDefaultStructors(OSBoolean);
	friend class OSSerialize;

protected:
	bool value;

   
                          
  
            
                                             
                                   
  
                      
                      
   
	virtual void taggedRelease(
		const void * tag,
		const int    when) const APPLE_KEXT_OVERRIDE;

public:
	static void initialize();

   
                        
  
            
                                                    
  
                                  
  
          
                                                                        
  
              
                                        
                                                            
                                                            
                                                                        
   
	static OSPtr<OSBoolean> withBoolean(bool value);

   
                 
  
            
                                                                     
  
              
                                        
   
	virtual void free() APPLE_KEXT_OVERRIDE;


   
                         
  
            
                                                                              
  
                      
   
	virtual void taggedRetain(const void * tag) const APPLE_KEXT_OVERRIDE;


   
                   
  
            
                                      
                                                          
  
          
                                                                  
                                
  
              
                                           
                                                          
   
	virtual bool isTrue() const;


   
                    
  
            
                                      
                                                           
  
          
                                                                   
                               
  
              
                                           
                                                            
   
	virtual bool isFalse() const;


   
                     
  
            
                                                                    
  
          
                                                                   
   
	virtual bool getValue() const;


   
                      
  
            
                                               
  
                                                                        
  
          
                                                        
                             
  
              
                                             
                                                        
   
	virtual bool isEqualTo(const OSBoolean * aBoolean) const;


   
                      
  
            
                                                          
  
                                                                  
  
          
                                                                         
  
              
                                                     
                                           
                                                       
   
	virtual bool isEqualTo(const OSMetaClassBase * anObject) const APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                          
                                                                    
  
                                             
  
          
                                                                          
   
	virtual bool serialize(OSSerialize * serializer) const APPLE_KEXT_OVERRIDE;

	OSMetaClassDeclareReservedUnused(OSBoolean, 0);
	OSMetaClassDeclareReservedUnused(OSBoolean, 1);
	OSMetaClassDeclareReservedUnused(OSBoolean, 2);
	OSMetaClassDeclareReservedUnused(OSBoolean, 3);
	OSMetaClassDeclareReservedUnused(OSBoolean, 4);
	OSMetaClassDeclareReservedUnused(OSBoolean, 5);
	OSMetaClassDeclareReservedUnused(OSBoolean, 6);
	OSMetaClassDeclareReservedUnused(OSBoolean, 7);
};

   
                        
  
            
                                                
  
              
                                                                  
                                                                        
                          
                                               
                                       
                                                  
   
extern OSBoolean * const & kOSBooleanTrue;

   
                         
  
            
                                                 
  
              
                                                                    
                                                                        
                          
                                                
                                       
                                                   
   
extern OSBoolean * const & kOSBooleanFalse;

#endif                       
