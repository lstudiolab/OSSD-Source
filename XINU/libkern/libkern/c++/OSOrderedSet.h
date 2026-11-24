/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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

#ifndef _OS_OSORDEREDSET_H
#define _OS_OSORDEREDSET_H

#include <libkern/c++/OSCollection.h>
#include <libkern/c++/OSPtr.h>
#include <libkern/OSTypes.h>

class OSOffset;
class OSOrderedSet;

typedef OSOrderedSet* OSOrderedSetPtr;

   
          
  
            
                                                          
   


   
                      
  
            
                                                         
  
              
                                                      
                      
                                                                     
                                                                      
                                               
                                                              
       
                                                       
                                
                                                                       
                                             
                                                             
                                                        
        
  
                                                                          
                                                             
                                                                  
                                      
                                                                        
                                                           
  
                                          
                                            
                                        
                                                                      
                                                                            
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                                   
                                                                    
                                        
                                                                     
                                                                  
              
   
class OSOrderedSet : public OSCollection
{
	OSDeclareDefaultStructors(OSOrderedSet);

public:
   
                           
  
            
                                                                 
  
                                                                            
                                                                   
                                            
                                                                                  
  
          
                                     
       
                                                           
                                                           
                                                                 
        
   
	typedef SInt32 (*OSOrderFunction)(const OSMetaClassBase * obj1,
	    const OSMetaClassBase * obj2,
	    void * context);

protected:
	struct _Element * array;
	OSOrderFunction   ordering;
	void            * orderingRef;
	unsigned int      count;
	unsigned int      capacity;
	unsigned int      capacityIncrement;

	struct ExpansionData { };

                                                    
	ExpansionData *reserved;

protected:
                                      
	virtual unsigned int iteratorSize() const APPLE_KEXT_OVERRIDE;
	virtual bool initIterator(void *iterator) const APPLE_KEXT_OVERRIDE;
	virtual bool getNextObjectForIterator(void *iterator, OSObject **ret) const APPLE_KEXT_OVERRIDE;

public:

   
                         
  
            
                                                 
  
                                                       
                                                         
                                                                             
                                       
                                               
                                                                     
          
                                    
                            
                                
  
              
                                         
                                           
                                       
                                              
                                                   
  
                                                       
              
                                                                                   
                                                  
                                             
                                                  
  
                                                                           
                      
                                 
                                                      
                                           
      
                                                          
   
	static OSPtr<OSOrderedSet> withCapacity(
		unsigned int      capacity,
		OSOrderFunction   orderFunc = NULL,
		void            * orderingContext = NULL);


   
                             
  
            
                                              
  
                                                       
                                                         
                                                                             
                                       
                                               
                                                                     
  
          
                                                               
  
              
                                                               
              
                                                                                                         
                            
           
  
                                         
                                                                       
                                              
                                                   
  
                                                       
              
                                                                                   
                                                  
                                             
                                                  
  
                                                                           
                      
                                 
                                                      
                                           
      
                                                          
   
	virtual bool initWithCapacity(
		unsigned int      capacity,
		OSOrderFunction   orderFunc = NULL,
		void            * orderingContext = NULL);


   
                 
  
            
                                        
                                     
  
              
                                               
      
              
                                                        
                       
           
   
	virtual void free() APPLE_KEXT_OVERRIDE;


   
                     
  
            
                                                                
  
          
                                                        
   
	virtual unsigned int getCount() const APPLE_KEXT_OVERRIDE;


   
                        
  
            
                                                
                                  
  
          
                                     
                                  
  
              
                                                                         
      
              
                                                                                
                                    
      
              
                                                                                     
                               
   
	virtual unsigned int getCapacity() const APPLE_KEXT_OVERRIDE;


   
                                 
  
            
                                                    
  
          
                                            
  
              
                                                             
                             
   
	virtual unsigned int getCapacityIncrement() const APPLE_KEXT_OVERRIDE;


   
                                 
  
            
                                                 
  
          
                                                
                                                    
  
              
                                                             
                             
                                                                 
   
	virtual unsigned int setCapacityIncrement(unsigned increment) APPLE_KEXT_OVERRIDE;


   
                           
  
            
                                   
                                                     
  
                                                                           
                                               
  
          
                                       
                                                   
                                                
  
              
                                                                   
                                                                     
                                                                        
                                                                       
  
                                                             
   
	virtual unsigned int ensureCapacity(unsigned int newCapacity) APPLE_KEXT_OVERRIDE;


   
                            
  
            
                                                           
  
              
                                                                       
                                   
   
	virtual void flushCollection() APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                                                   
                                                            
  
                                                                  
                                       
          
                                                              
                                                         
                                                    
  
              
                                                                   
                                                 
  
                                                             
                                  
                                                    
                                                         
                                                             
                           
                                          
                                   
                                             
                                                                             
  
                                                                    
  
                                                    
                                                            
                                                
                                         
                          
              
                                                                                        
                                                        
   
	virtual bool setObject(const OSMetaClassBase * anObject);

	bool setObject(OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                           
  
            
                                                
                                
  
                                                      
                                                   
          
                                                                    
                                                   
                                                                 
  
              
                                                                   
                                                 
  
                                                                  
                                                   
                                                                        
  
                                                                               
                                 
                                                
                                         
                          
              
                                                                                        
                                                        
   
	virtual bool setFirstObject(const OSMetaClassBase * anObject);

	bool setFirstObject(OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                          
  
            
                                                
                                
  
                                                                  
                                       
          
                                                                    
                                                   
                                                                 
  
              
                                                                   
                                                 
  
                                                                  
                                                   
                                                                        
  
                                                                               
                                 
                                                
                                         
                          
              
                                                                                        
                                                        
   
	virtual bool setLastObject(const OSMetaClassBase * anObject);

	bool setLastObject(OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                         
  
            
                                          
  
                                                      
                                                       
  
              
                                                       
   
	virtual void removeObject(const OSMetaClassBase * anObject);

	void removeObject(OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                           
  
            
                                                        
  
                                                                   
                                       
  
          
                                                        
                                                        
  
              
                            
                                                                        
   
	virtual bool containsObject(const OSMetaClassBase * anObject) const;


   
                   
  
            
                                                        
  
                                                                   
                                       
  
          
                                                        
                                                                        
  
              
                            
                                                          
  
              
                                                                                        
                                                       
                                                             
                                                      
   
	virtual bool member(const OSMetaClassBase * anObject) const;


   
                           
  
            
                                                            
                               
  
              
                                                                        
                                                      
              
                                                       
                      
                  
   
	virtual OSObject * getFirstObject() const;


   
                          
  
            
                                                      
                               
  
              
                                                                        
                                                      
              
                                                       
                      
                  
   
	virtual OSObject * getLastObject() const;


   
                        
  
            
                                                                             
  
                                             
  
          
                                     
  
              
                                        
                                             
                                                                          
                                          
                                           
   
	virtual SInt32 orderObject(const OSMetaClassBase * anObject);


   
                      
  
            
                                                         
                                
  
                                                                
                                                                  
                                       
  
          
                                                         
                                                   
                                            
  
              
                                                                   
                                                 
  
                                                                  
                                                   
                                                                        
  
                                                                    
                                 
                                                
                                         
                          
                                                                                                    
                                              
   
	virtual bool setObject(
		unsigned int            index,
		const OSMetaClassBase * anObject);

	bool setObject(
		unsigned int            index,
		OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                      
  
            
                                         
  
                                        
          
                                 
                                                        
  
              
                                                                
                                                      
              
                                                       
                      
                  
   
	virtual OSObject * getObject(unsigned int index) const;


   
                           
  
            
                                                                 
  
          
                                      
                                               
   
	virtual void * getOrderingRef();


   
                      
  
            
                                                  
  
                                                             
                                             
          
                                                    
                                
  
              
                                                                        
                                                        
   
	virtual bool isEqualTo(const OSOrderedSet * anOrderedSet) const;


   
                      
  
            
                                        
                               
  
                                                                   
          
                                                       
                                
  
              
                                                               
                                                   
                                         
   
	virtual bool isEqualTo(const OSMetaClassBase * anObject) const APPLE_KEXT_OVERRIDE;


   
                       
  
                                                  
                             
  
                                                                              
                                               
                                                     
                                                                   
                                                 
                          
  
          
                                                           
  
              
                                                   
  
                                                                             
                           
   
	virtual unsigned setOptions(
		unsigned   options,
		unsigned   mask,
		void     * context = NULL) APPLE_KEXT_OVERRIDE;


   
                           
  
            
                                                                     
  
                                                           
                                                  
                                                                
                                                        
                                            
  
          
                                                          
                                                                       
  
              
                                                              
                           
                                                              
                      
   
	OSPtr<OSCollection> copyCollection(OSDictionary * cycleDict = NULL) APPLE_KEXT_OVERRIDE;

	OSMetaClassDeclareReservedUnused(OSOrderedSet, 0);
	OSMetaClassDeclareReservedUnused(OSOrderedSet, 1);
	OSMetaClassDeclareReservedUnused(OSOrderedSet, 2);
	OSMetaClassDeclareReservedUnused(OSOrderedSet, 3);
	OSMetaClassDeclareReservedUnused(OSOrderedSet, 4);
	OSMetaClassDeclareReservedUnused(OSOrderedSet, 5);
	OSMetaClassDeclareReservedUnused(OSOrderedSet, 6);
	OSMetaClassDeclareReservedUnused(OSOrderedSet, 7);
};

#endif                           
