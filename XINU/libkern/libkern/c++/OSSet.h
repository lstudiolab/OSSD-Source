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
                                                   
                                                        

#ifndef _OS_OSSET_H
#define _OS_OSSET_H

#include <libkern/c++/OSCollection.h>
#include <libkern/c++/OSPtr.h>

class OSArray;
class OSSet;

typedef OSSet* OSSetPtr;
typedef OSArray* OSArrayPtr;

   
          
  
            
                                                   
   


   
               
  
            
                                                    
  
              
                                               
                      
                                                                     
                                                                      
                                                                              
                                                         
                                                 
                                                  
                                                                          
                                  
  
                                          
                                     
                                        
                                                               
                                                                            
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                            
                                                                    
                                        
                                                                     
                                                                  
              
   
class OSSet : public OSCollection
{
	friend class OSSerialize;

	OSDeclareDefaultStructors(OSSet);

#if APPLE_KEXT_ALIGN_CONTAINERS

private:
	OSPtr<OSArray> members;

#else                                  

private:
	OSPtr<OSArray> members;

protected:
	struct ExpansionData { };

                                                    
	ExpansionData * reserved;

#endif                                  

  
                                   
   
	virtual unsigned int iteratorSize() const APPLE_KEXT_OVERRIDE;
	virtual bool initIterator(void * iterator) const APPLE_KEXT_OVERRIDE;
	virtual bool getNextObjectForIterator(void * iterator, OSObject ** ret) const APPLE_KEXT_OVERRIDE;

public:


   
                         
  
            
                                          
  
                                                                        
  
          
                             
                                    
                                
  
              
                                         
                                                                         
                                                                        
                                                   
   
	static OSPtr<OSSet> withCapacity(unsigned int capacity);


   
                        
  
            
                                   
                                   
  
                                                                 
                                                                    
                                                                       
                                                                          
                                                                        
  
          
                       
                                   
                            
                                
  
              
                                                      
                                          
                                       
                                                          
                                                                
                                                                        
                                                   
  
                                                                               
              
   
	static OSPtr<OSSet> withObjects(
		const OSObject * objects[],
		unsigned int     count,
		unsigned int     capacity = 0);


   
                      
  
            
                                   
                                             
  
                                                                           
                                                                       
                                                                      
                                          
                                                                                    
                                                                
          
                                  
                                     
                            
                                
  
              
                                                                      
  
                                                    
                                       
                                                          
                                                                         
                                                                        
                                                   
  
                                                                             
              
   
	static OSPtr<OSSet> withArray(
		const OSArray * array,
		unsigned int    capacity = 0);


   
                    
  
            
                                   
                                                
  
                                                          
                                        
                                                                   
                                                                      
                                        
                                                                                    
                                                                
          
                         
                                              
                            
                                
  
              
                                                  
                                       
                                                          
                                                                     
                                                                        
                                                   
  
                                                                           
              
   
	static OSPtr<OSSet> withSet(const OSSet * set,
	    unsigned int capacity = 0);


   
                             
  
            
                                       
  
                                                                       
  
          
                                                               
  
              
                                                               
              
                                                                     
                            
           
  
                                         
                                                                       
                                                                        
                                                   
   
	virtual bool initWithCapacity(unsigned int capacity);


   
                            
  
            
                                                           
  
                                                          
                                                                    
                                                                       
                                                                          
                                                                        
  
          
                                                               
  
              
                                                               
              
                                                                                               
                           
           
  
                                                      
                                          
                                                                                               
                                                                         
                                                                        
                                                   
  
                                                                               
              
   
	virtual bool initWithObjects(
		const OSObject * objects[],
		unsigned int     count,
		unsigned int     capacity = 0);


   
                          
  
                                    
                                                       
  
                                                           
                                        
                                                                       
                                             
                                                                   
                                                                                    
                                                                
  
          
                                                               
  
              
                                                               
              
                                                                                
                         
           
  
                                                    
                                       
                                                          
                                                                         
                                                                        
                                                   
  
                                                                             
              
   
	virtual bool initWithArray(
		const OSArray * array,
		unsigned int capacity = 0);


   
                        
  
            
                          
                                                
  
                                                                            
                                                                       
                                             
                                                                 
                                                                                    
                                                              
  
          
                                                               
  
              
                                                               
                                                    
  
                                                  
                                       
                                                          
                                                                       
                                                                        
                                                   
  
                                                                           
              
   
	virtual bool initWithSet(const OSSet *set,
	    unsigned int capacity = 0);


   
                 
  
            
                                        
                              
  
              
                                               
      
              
                                                        
                       
           
   
	virtual void free() APPLE_KEXT_OVERRIDE;


   
                     
  
            
                                                        
  
          
                                                
   
	virtual unsigned int getCount() const APPLE_KEXT_OVERRIDE;


   
                        
  
            
                                        
                                  
  
          
                             
                                  
  
              
                                                                  
      
              
                                                                         
                                    
      
              
                                                                              
                               
   
	virtual unsigned int getCapacity() const APPLE_KEXT_OVERRIDE;


   
                                 
  
            
                                            
  
          
                                    
  
              
                                                      
                             
   
	virtual unsigned int getCapacityIncrement() const APPLE_KEXT_OVERRIDE;


   
                                 
  
            
                                         
  
          
                                        
                                                    
  
              
                                                      
                             
                                                                 
   
	virtual unsigned int setCapacityIncrement(unsigned increment) APPLE_KEXT_OVERRIDE;


   
                           
  
            
                                   
                                                     
  
                                                                   
                                               
          
                               
                                                   
                                                
  
              
                                                           
                                                                     
                                                                        
                                                                       
  
                                                      
   
	virtual unsigned int ensureCapacity(unsigned int newCapacity) APPLE_KEXT_OVERRIDE;


   
                            
  
            
                                                   
  
              
                                                               
                                   
   
	virtual void flushCollection() APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                                            
  
                                                                              
  
          
                                                              
                                                 
                                            
  
              
                                                                   
                                                 
  
                                                    
                                                            
                                                
                                         
                          
                                                          
   
	virtual bool setObject(const OSMetaClassBase * anObject);

	bool setObject(OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                  
  
            
                                              
  
                                                                       
  
          
                                                           
                                                                 
                                
  
              
                                           
                                      
                                             
                                              
  
                                                                            
                                                                       
                                      
                                                                                   
                                       
   
	virtual bool merge(const OSArray * array);


   
                  
  
            
                                           
  
                                                                     
  
          
                                                        
                                                                 
                                
  
              
                                           
                                    
                                             
                                               
  
                                                                            
                                                                     
                                    
                                                                                   
                                       
   
	virtual bool merge(const OSSet * set);


   
                         
  
            
                                  
  
                                                      
                                               
  
              
                                               
   
	virtual void removeObject(const OSMetaClassBase * anObject);

	void removeObject(OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                           
  
            
                                                
  
                                                      
                                            
  
          
                                                                        
                                
  
              
                            
                                                                        
   
	virtual bool containsObject(const OSMetaClassBase * anObject) const;


   
                   
  
            
                                                
  
                                                      
                                            
  
          
                                                        
                                                
  
              
                                                                     
                               
  
                                                         
                                      
                                                      
   
	virtual bool member(const OSMetaClassBase * anObject) const;


   
                         
  
            
                                                         
  
          
                                   
                                
  
              
                                                                
                                                      
              
                                                       
                      
                  
   
	virtual OSObject * getAnyObject() const;


   
                      
  
            
                                           
  
                                                                   
          
                                                    
                                
  
              
                                                                 
                                      
   
	virtual bool isEqualTo(const OSSet * aSet) const;


   
                      
  
            
                                                              
  
                                                                   
          
                                                       
                                
  
              
                                                                            
                                                     
   
	virtual bool isEqualTo(const OSMetaClassBase * anObject) const APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                          
                                                                    
  
                                            
  
          
                                                                          
   
	virtual bool serialize(OSSerialize * serializer) const APPLE_KEXT_OVERRIDE;


   
                       
  
            
                                          
                             
  
                                                                              
                                               
                                                     
                                                                   
                                                 
                          
  
          
                                                           
  
              
                                                   
  
                                                                     
                           
   
	virtual unsigned setOptions(unsigned options, unsigned mask, void * context = NULL) APPLE_KEXT_OVERRIDE;


   
                           
  
            
                                                             
  
                                                           
                                                  
                                                                
                                                        
                                            
  
          
                                                  
                                                                       
  
              
                                                      
                           
                                                              
                      
   
	OSPtr<OSCollection>  copyCollection(OSDictionary *cycleDict = NULL) APPLE_KEXT_OVERRIDE;

	OSMetaClassDeclareReservedUnused(OSSet, 0);
	OSMetaClassDeclareReservedUnused(OSSet, 1);
	OSMetaClassDeclareReservedUnused(OSSet, 2);
	OSMetaClassDeclareReservedUnused(OSSet, 3);
	OSMetaClassDeclareReservedUnused(OSSet, 4);
	OSMetaClassDeclareReservedUnused(OSSet, 5);
	OSMetaClassDeclareReservedUnused(OSSet, 6);
	OSMetaClassDeclareReservedUnused(OSSet, 7);
};

#endif                   
