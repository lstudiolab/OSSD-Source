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
                                                     
                                                          

#ifndef _OS_OSARRAY_H
#define _OS_OSARRAY_H

#include <libkern/c++/OSCollection.h>
#include <libkern/c++/OSPtr.h>
#include <os/base.h>

class OSSerialize;
class OSArray;

typedef OSArray* OSArrayPtr;

   
          
  
            
                                                     
   


   
                 
  
            
                                                
  
              
                                                 
                      
                                                                     
                
                                                        
                                         
  
                                                 
                                                   
                             
                                                                              
                                                           
                                                                    
  
                                          
                                       
                                                      
                                                                 
                                                                            
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                              
                                                                    
                                        
                                                                     
                                                                  
              
   
class OSArray : public OSCollection
{
	friend class OSSet;
	friend class OSSerialize;

	OSDeclareDefaultStructors(OSArray);

	typedef OSTaggedPtr<const OSMetaClassBase> ArrayPtrType;
	typedef OSTaggedSharedPtr<const OSMetaClassBase, OSCollection> ArraySharedPtrType;

#if APPLE_KEXT_ALIGN_CONTAINERS

protected:
	unsigned int             count;
	unsigned int             capacity;
	unsigned int             capacityIncrement;
	ArrayPtrType           * OS_PTRAUTH_SIGNED_PTR("OSArray.array") array;

#else                                  

protected:
	ArrayPtrType           * OS_PTRAUTH_SIGNED_PTR("OSArray.array") array;
	unsigned int             count;
	unsigned int             capacity;
	unsigned int             capacityIncrement;

	struct ExpansionData { };

                                                  
	ExpansionData          * reserved;

#endif                                  

                                      
	virtual unsigned int iteratorSize() const APPLE_KEXT_OVERRIDE;
	virtual bool initIterator(void * iterator) const APPLE_KEXT_OVERRIDE;
	virtual bool getNextObjectForIterator(void * iterator, OSObject ** ret) const APPLE_KEXT_OVERRIDE;

public:

   
                         
  
            
                                            
  
                                                                      
  
          
                                                         
                                
  
              
                                         
                                                                
                                                                          
                                                   
   
	static OSPtr<OSArray> withCapacity(unsigned int capacity);


   
                        
  
            
                                                                      
  
                                                            
                                                                      
                                                                     
                                                                          
                                                                        
  
          
                                                          
                            
                                
  
              
                                                                                              
                                       
                                                          
                                                                
                                                                          
                                                   
   
	static OSPtr<OSArray> withObjects(
		const OSObject * objects[],
		unsigned int     count,
		unsigned int     capacity = 0);


   
                      
  
            
                                                                                   
  
                                                            
                                        
                                                                     
                                                                      
                                          
                                                           
                                                                  
                                          
  
          
                                                                       
                            
                                
  
              
                                                    
                                       
                                                          
                                                                
                                                                          
                                                   
  
                                                 
                                  
              
   
	static OSPtr<OSArray> withArray(
		const OSArray * array,
		unsigned int    capacity = 0);


   
                             
  
            
                                         
  
                                                                     
  
          
                                                               
  
              
                                                               
                                                                                     
                            
           
  
                                         
                                                                
                                                                          
                                                   
   
	virtual bool initWithCapacity(unsigned int capacity);


   
                            
  
            
                                                             
  
                                                          
                                                                      
                                                                     
                                                                          
                                                                        
  
          
                                                               
  
              
                                                               
              
                                                                                                   
                           
           
  
                                                      
                                          
                                       
                                                          
                                                                
                                                                          
                                                   
   
	virtual bool initWithObjects(
		const OSObject * objects[],
		unsigned int     count,
		unsigned int     capacity = 0);

   
                          
  
            
                                                                          
  
                                                          
                                       
                                                                    
                                                                     
                                         
                                                          
                                                                 
                                         
  
          
                                                               
  
              
                                                               
                                                                                                
                                  
  
                                                    
                                       
                                                          
                                                                
                                                                          
                                                   
  
                                                                                 
              
   
	virtual bool initWithArray(
		const OSArray * anArray,
		unsigned int    capacity = 0);


   
                 
  
            
                                        
                                
  
              
                                               
      
              
                                                        
                       
           
   
	virtual void free() APPLE_KEXT_OVERRIDE;


   
                     
  
            
                                                          
  
          
                                                  
   
	virtual unsigned int getCount() const APPLE_KEXT_OVERRIDE;


   
                        
  
            
                                                    
                        
  
          
                                         
                        
  
              
                                                                    
      
              
                                                                  
                                    
      
        
                                                                                
                                     
   
	virtual unsigned int getCapacity() const APPLE_KEXT_OVERRIDE;


   
                                 
  
            
                                              
  
          
                                      
  
              
                                                        
                             
   
	virtual unsigned int getCapacityIncrement() const APPLE_KEXT_OVERRIDE;


   
                                 
  
            
                                           
  
          
                                          
                                                    
  
              
                                                        
                             
                                                                 
   
	virtual unsigned int setCapacityIncrement(unsigned increment) APPLE_KEXT_OVERRIDE;


   
                           
  
            
                                     
                                            
  
                                                            
                                               
  
          
                                 
                                                   
                                                
  
              
                                                             
                                                            
                                                                        
                                                                       
  
                                                        
   
	virtual unsigned int ensureCapacity(unsigned int newCapacity) APPLE_KEXT_OVERRIDE;


   
                            
  
            
                                                     
  
              
                                                                 
                                   
   
	virtual void flushCollection() APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                               
                                   
  
                                                              
  
          
                                                                             
                             
  
              
                                                                     
                                                 
   
	virtual bool setObject(const OSMetaClassBase * anObject);

	bool setObject(OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                      
  
            
                                              
                         
  
                                                                         
                                                                    
                                                   
  
          
                                                             
                                             
  
              
                                                                   
                                          
                                                                    
              
                                                                                              
                              
                                                 
  
                                                                     
                                                                         
                                                                    
                                                                   
      
              
                                                                                
                               
   
	virtual bool setObject(
		unsigned int            index,
		const OSMetaClassBase * anObject);

	bool setObject(
		unsigned int index,
		OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                  
  
            
                                                             
  
                                                                
                                              
          
                                                                             
  
              
                                                       
                                                          
                                                           
   
	virtual bool merge(const OSArray * otherArray);


   
                          
  
            
                                                   
  
                                                           
                                                        
                                                           
  
              
                                                                  
   
	virtual void replaceObject(
		unsigned int            index,
		const OSMetaClassBase * anObject);

	void replaceObject(
		unsigned int            index,
		OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                         
  
            
                                    
  
                                                       
  
              
                                                                 
                             
                                  
   
	virtual void removeObject(unsigned int index);


   
                      
  
            
                                             
  
                                                                        
  
          
                                                      
                               
  
              
                                                                   
                                                                     
              
                                                                                      
                          
   
	virtual bool isEqualTo(const OSArray * anArray) const;


   
                      
  
            
                                                           
  
                                                                   
  
          
                                                       
                                
  
              
                                                   
                                         
                                               
   
	virtual bool isEqualTo(const OSMetaClassBase * anObject) const APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                             
  
                                                                 
  
          
                                           
                                                                             
  
              
                                                                  
                                                      
              
                                                       
                      
                  
   
	virtual OSObject * getObject(unsigned int index) const;


   
                          
  
            
                                        
  
          
                                
                                              
  
              
                                                                  
                                                      
              
                                                       
                      
                  
   
	virtual OSObject * getLastObject() const;


   
                                 
  
            
                                                             
                              
  
                                           
                                                         
  
          
                                                               
                    
  
              
                                                           
              
                                                                                      
                          
   
	virtual unsigned int getNextIndexOfObject(
		const OSMetaClassBase * anObject,
		unsigned int            index) const;

   
                      
  
            
                                          
                                                                    
  
                                             
          
                                                                          
   
	virtual bool serialize(OSSerialize * serializer) const APPLE_KEXT_OVERRIDE;


   
                       
  
            
                                           
                             
  
                                                                              
                                               
                                                     
                                                                   
                                                 
                          
  
          
                                                           
  
              
                                                   
  
                                                                       
                           
   
	virtual unsigned setOptions(
		unsigned   options,
		unsigned   mask,
		void     * context = NULL) APPLE_KEXT_OVERRIDE;


   
                           
  
            
                                                             
  
                                                           
                                                  
                                                                
                                                        
                                            
  
          
                                                    
                                                                       
  
              
                                                        
                           
                                                              
                      
   
	OSPtr<OSCollection> copyCollection(OSDictionary * cycleDict = NULL) APPLE_KEXT_OVERRIDE;

	OSMetaClassDeclareReservedUnused(OSArray, 0);
	OSMetaClassDeclareReservedUnused(OSArray, 1);
	OSMetaClassDeclareReservedUnused(OSArray, 2);
	OSMetaClassDeclareReservedUnused(OSArray, 3);
	OSMetaClassDeclareReservedUnused(OSArray, 4);
	OSMetaClassDeclareReservedUnused(OSArray, 5);
	OSMetaClassDeclareReservedUnused(OSArray, 6);
	OSMetaClassDeclareReservedUnused(OSArray, 7);
};

#endif                     
