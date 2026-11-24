/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
                                                      

#ifndef _OS_OSCOLLECTION_H
#define _OS_OSCOLLECTION_H

#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSPtr.h>

class OSDictionary;
class OSCollection;

typedef OSCollection* OSCollectionPtr;

                                                                      
                       
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-extensions"

template <typename T>
using OSCollectionTaggedPtr = T *;

#pragma clang diagnostic pop


   
          
  
            
                                                          
   


   
                      
  
            
                                                   
  
              
                                          
                                          
                                                                 
                                                
           
                                                          
      
                                                                              
            
                                  
                                                         
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                                   
                                                                    
                                        
                                                                     
                                                                  
              
   
class OSCollection : public OSObject
{
	friend class OSCollectionIterator;

	OSDeclareAbstractStructors(OSCollection);

	struct ExpansionData { };

protected:
                                   
  
                   
  
            
                                                  
  
              
                                                       
                         
                                                                  
                                                                              
                        
   
	unsigned int updateStamp;

#ifdef XNU_KERNEL_PRIVATE
protected:
#else
private:
#endif                         
                                                    
                            
	unsigned int fOptions;

protected:
                                                           


   
                         
  
            
                                                               
  
          
                                             
                                          
  
              
                                                                      
                  
                                                                              
                                                    
                             
                                                   
                                     
   
	virtual unsigned int iteratorSize() const = 0;


   
                         
  
            
                                                               
  
                                                                
  
          
                                                      
                                
  
              
                                                                      
                  
                                                                              
                                                              
                                                                                     
                                                     
  
                                                              
                                         
                                                                        
                                    
            
   
	virtual bool initIterator(void * iterationContext) const = 0;


   
                                     
  
            
                                           
  
                                                  
                                                                           
  
          
                                                                          
  
              
                                                                      
                  
                                                                              
                                                        
                                         
                                               
                                              
                                     
                                                              
                                                      
                                                                                    
  
                                                                     
                                                                 
   
	virtual bool getNextObjectForIterator(
		void      * iterationContext,
		OSObject ** nextObject) const = 0;


   
                 
  
            
                                       
  
          
                                                              
  
              
                                            
                                              
   
	virtual bool init() APPLE_KEXT_OVERRIDE;

public:

   
                              
  
                    
              
                                                           
                                               
                        
  
                                                                       
                                              
                                                                
                                                          
                                                     
                          
   
	typedef enum {
		kImmutable  = 0x00000001,
		kSort       = 0x00000002,
		kMASK       = (unsigned) - 1
	} _OSCollectionFlags;

                                             

   
                        
  
            
                                    
  
              
                                              
                                                                        
                                                    
                                                        
   
	void haveUpdated();


   
                     
  
            
                                                   
  
          
                                           
  
              
                                                               
   
	virtual unsigned int getCount() const = 0;


   
                        
  
            
                                               
                                  
  
          
                                    
                                  
  
              
                                                               
   
	virtual unsigned int getCapacity() const = 0;


   
                                 
  
            
                                                   
  
          
                                           
  
              
                                                               
                                                    
                                          
  
      
              
                                                                                     
                              
                                          
   
	virtual unsigned int getCapacityIncrement() const = 0;


   
                                 
  
            
                                                
  
          
                                               
                                                    
  
              
                                                               
                                                    
                                          
  
                                                 
                                    
                                                           
  
                                                                          
                             
  
      
        
                                                                                     
                       
                                          
   
	virtual unsigned int setCapacityIncrement(unsigned increment) = 0;


   
                           
  
            
                                                   
                                   
  
                                                                 
                                               
  
          
                                      
                                                   
                                                
  
              
                                                         
                                                
                                             
                                                 
                                            
  
                                                      
                                                      
                                                 
  
                                                 
                                                                   
                                           
   
	virtual unsigned int ensureCapacity(unsigned int newCapacity) = 0;


   
                            
  
            
                                                          
  
              
                                                         
                                   
                                               
   
	virtual void flushCollection() = 0;


   
                       
  
            
                                                  
                             
  
                                                                              
                                               
                                                     
                                                                   
                                                 
                          
  
          
                                                           
  
              
                                                   
  
                                      
                                                                        
                           
  
                                                                
                                                                
   
	virtual unsigned setOptions(
		unsigned   options,
		unsigned   mask,
		void     * context = NULL);

   
                           
  
            
                                       
  
                                                           
                                                  
                                                       
                                                                       
  
          
                              
                                
  
              
                                      
                                                    
                                                              
                      
  
                                                         
                                   
   
	virtual OSPtr<OSCollection> copyCollection(OSDictionary * cycleDict = NULL);

   
                           
  
            
                                                       
  
                                                         
                                         
                                                                
                                                                    
                                                                     
                                                  
  
          
                                                     
                                                        
   
	bool iterateObjects(void * refcon, bool (*callback)(void * refcon, OSObject * object));

#ifdef __BLOCKS__

   
                           
  
            
                                                    
  
                             
                                                                
                                                                    
                                                                  
                                                  
  
          
                                                     
                                                        
   
	bool iterateObjects(bool (^block)(OSObject * object));

#endif                 

	OSMetaClassDeclareReservedUsedX86(OSCollection, 0);
	OSMetaClassDeclareReservedUsedX86(OSCollection, 1);
	OSMetaClassDeclareReservedUnused(OSCollection, 2);
	OSMetaClassDeclareReservedUnused(OSCollection, 3);
	OSMetaClassDeclareReservedUnused(OSCollection, 4);
	OSMetaClassDeclareReservedUnused(OSCollection, 5);
	OSMetaClassDeclareReservedUnused(OSCollection, 6);
	OSMetaClassDeclareReservedUnused(OSCollection, 7);
};

#endif                          
