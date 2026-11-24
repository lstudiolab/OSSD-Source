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
                                                      
                                                           

#ifndef _OS_OSNUMBER_H
#define _OS_OSNUMBER_H

#include <libkern/c++/OSPtr.h>
#include <libkern/c++/OSObject.h>

   
          
  
            
                                                     
   

class OSNumber;

typedef OSNumber* OSNumberPtr;

   
                  
  
            
                                                  
                                  
  
              
                                                          
                           
                                                                    
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                               
                                                                    
                                        
                                                                     
                                                                  
              
   
class OSNumber : public OSObject
{
	friend class OSSerialize;

	OSDeclareDefaultStructors(OSNumber);

#if APPLE_KEXT_ALIGN_CONTAINERS

protected:
	unsigned int size;
	unsigned long long value;

#else                                  

protected:
	unsigned long long value;
	unsigned int size;

	struct ExpansionData { };

                                                    
	ExpansionData * reserved;

#endif                                  

public:

   
                       
  
            
                                                  
                         
  
                                                                           
                                                              
  
          
                                                       
                                
  
              
                                                                         
                                           
  
                                                
                                                   
                                                  
                                     
   
	static OSPtr<OSNumber> withNumber(
		unsigned long long value,
		unsigned int       numberOfBits);


   
                       
  
            
                                                  
                                                            
  
                                                              
                                                 
                                                              
  
          
                                                       
                                
  
              
                                                                                
                                             
                                                        
                                                                        
                                               
  
                                                                       
                                           
  
                                                
                                                   
                                                  
                                     
   
	static OSPtr<OSNumber> withNumber(
		const char   * valueString,
		unsigned int   numberOfBits);


   
                 
  
            
                                                             
  
                                                                           
                                                              
  
          
                                                
                                 
  
              
                                                               
              
                                                                                    
                                                            
           
   
	virtual bool init(
		unsigned long long value,
		unsigned int       numberOfBits);


   
                 
  
            
                                      
                                                            
  
                                                              
                                                 
                                                              
  
          
                                                
                                 
  
              
                                                               
              
                                                                                    
                                                      
           
   
	virtual bool init(
		const char   * valueString,
		unsigned int   numberOfBits);


   
                 
  
            
                                        
                                 
  
              
                                               
      
              
                                                        
                       
           
   
	virtual void free() APPLE_KEXT_OVERRIDE;


   
                         
  
            
                                               
                                       
  
          
                                       
                                       
  
              
                                                                        
                                                                             
                                            
   
	virtual unsigned int numberOfBits() const;


   
                          
  
            
                                                
                                       
  
          
                                        
                                       
                                                          
   
	virtual unsigned int numberOfBytes() const;


                                                         
                                                        


   
                              
  
            
                                              
                                     
  
          
                                      
                                     
  
              
                                                         
                                                                             
   
	virtual unsigned char unsigned8BitValue() const;


   
                               
  
            
                                              
                                      
  
          
                                              
                                      
  
              
                                                         
                                                                             
   
	virtual unsigned short unsigned16BitValue() const;


   
                               
  
            
                                              
                                      
  
          
                                              
                                      
  
              
                                                         
                                                                             
   
	virtual unsigned int unsigned32BitValue() const;


   
                               
  
            
                                              
                                      
  
          
                                              
                                      
  
              
                                                         
                                                                             
   
	virtual unsigned long long unsigned64BitValue() const;

                                                    

   
                     
  
            
                                                            
                          
  
                                       
  
              
                                                
                                       
                                                            
                                             
                                       
   
	virtual void addValue(signed long long value);


   
                     
  
            
                                              
                                             
  
                                                       
                                                                          
                                                                          
   
	virtual void setValue(unsigned long long value);


   
                      
  
            
                                              
  
                                                                       
  
          
                                                       
                             
  
              
                                            
                                              
   
	virtual bool isEqualTo(const OSNumber * aNumber) const;


   
                      
  
            
                                                         
  
                                                                 
  
          
                                              
                             
  
              
                                                                      
                                                                 
   
	virtual bool isEqualTo(const OSMetaClassBase * anObject) const APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                          
                                                                    
  
                                            
  
          
                                                                          
   
	virtual bool serialize(OSSerialize * serializer) const APPLE_KEXT_OVERRIDE;


	OSMetaClassDeclareReservedUnused(OSNumber, 0);
	OSMetaClassDeclareReservedUnused(OSNumber, 1);
	OSMetaClassDeclareReservedUnused(OSNumber, 2);
	OSMetaClassDeclareReservedUnused(OSNumber, 3);
	OSMetaClassDeclareReservedUnused(OSNumber, 4);
	OSMetaClassDeclareReservedUnused(OSNumber, 5);
	OSMetaClassDeclareReservedUnused(OSNumber, 6);
	OSMetaClassDeclareReservedUnused(OSNumber, 7);
};

#endif                      
