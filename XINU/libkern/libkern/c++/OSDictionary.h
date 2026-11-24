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
/*
 * Copyright (c) 1998-1999 Apple Computer, Inc.  All rights reserved.
 *
 * HISTORY
 *
 * OSDictionary.h created by rsulack on Wed 17-Sep-1997
 * OSDictionary.h converted to C++ by gvdl on Fri 1998-10-30
 */

#ifndef _IOKIT_IODICTIONARY_H
#define _IOKIT_IODICTIONARY_H

#include <libkern/c++/OSCollection.h>
#include <libkern/c++/OSArray.h>
#include <libkern/c++/OSPtr.h>
#include <os/base.h>

class OSArray;
class OSSymbol;
class OSString;
class OSDictionary;

typedef OSDictionary* OSDictionaryPtr;

   
          
  
            
                                                          
   


   
                      
  
            
                                                                     
  
              
                                                      
                      
                                                                     
                                                                      
                                                                  
                                                               
                                                        
                                                                             
                                                                                
                                                                     
                                
  
                                                 
                                                   
                             
                                                                              
                                                           
                                                                    
  
                                       
                                                                               
                            
                                                                           
                             
                                                              
                                                                   
  
                                          
                                                     
                                                               
                                                                              
                                                                            
  
                                                                      
                                                                  
                                                                 
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                                   
                                                                    
                                        
                                                                     
                                                                  
              
   
class OSDictionary : public OSCollection
{
	friend class OSSerialize;

	OSDeclareDefaultStructors(OSDictionary);

#if APPLE_KEXT_ALIGN_CONTAINERS

protected:
	unsigned int   count;
	unsigned int   capacity;
	unsigned int   capacityIncrement;
	struct dictEntry {
		OSTaggedPtr<const OSSymbol>        key;
		OSTaggedPtr<const OSMetaClassBase> value;
#if XNU_KERNEL_PRIVATE
		static int compare(const void *, const void *);
#endif
	};
	dictEntry    * OS_PTRAUTH_SIGNED_PTR("OSDictionary.dictionary") dictionary;

#else                                  

protected:
	struct dictEntry {
		OSTaggedPtr<const OSSymbol>        key;
		OSTaggedPtr<const OSMetaClassBase> value;
#if XNU_KERNEL_PRIVATE
		static int compare(const void *, const void *);
#endif
	};
	dictEntry    * OS_PTRAUTH_SIGNED_PTR("OSDictionary.dictionary") dictionary;
	unsigned int   count;
	unsigned int   capacity;
	unsigned int   capacityIncrement;

	struct ExpansionData { };

                                                    
	ExpansionData * reserved;

#endif                                  

                                                           
	virtual unsigned int iteratorSize() const APPLE_KEXT_OVERRIDE;
	virtual bool initIterator(void * iterator) const APPLE_KEXT_OVERRIDE;
	virtual bool getNextObjectForIterator(void * iterator, OSObject ** ret) const APPLE_KEXT_OVERRIDE;

public:

   
                         
  
            
                                                 
  
                                                                               
  
          
                                    
                            
                                
  
              
                                         
                                                                              
                                                                               
                                                   
   
	static OSPtr<OSDictionary> withCapacity(unsigned int capacity);


   
                        
  
                                                    
                                                      
  
                                                                 
                                              
                                                                          
                                                  
                                                     
                                                                              
                                                                          
                                                                        
  
          
                              
                                            
                            
                                
  
              
                                                                            
                                          
                                       
                                                          
                                         
                                       
                 
                                                                
                                                   
   
	static OSPtr<OSDictionary> withObjects(
		const OSObject * objects[],
		const OSSymbol * keys[],
		unsigned int     count,
		unsigned int     capacity = 0);

   
                        
  
            
                                          
                                            
  
                                                                 
                                                                            
                                            
                                                  
                                                     
                                                                              
                                                                          
                                                                        
  
          
                              
                                            
                            
                                
  
              
                                                                            
                                          
                                                                                               
                                         
                                       
                 
                                                                
                                                   
   
	static OSPtr<OSDictionary> withObjects(
		const OSObject * objects[],
		const OSString * keys[],
		unsigned int     count,
		unsigned int     capacity = 0);


   
                           
  
            
                                          
                                                     
  
                                                              
                                        
                                                                              
                                                                              
                                         
                                                                                    
                                                                       
  
          
                              
                                                       
                            
                                
  
              
                                                   
                                                                                               
                                         
                                       
                 
                                                                 
                                                   
  
                                                                     
                           
              
   
	static OSPtr<OSDictionary> withDictionary(
		const OSDictionary * dict,
		unsigned int         capacity = 0);


   
                             
  
            
                                              
  
                                                                              
          
                                                               
  
              
                                                               
                                                                                               
                            
           
  
                                         
                                         
                                       
                 
                                                                
                                                   
   
	virtual bool initWithCapacity(unsigned int capacity);


   
                            
  
                                                                           
  
                                                                 
                                              
                                                                          
                                                               
                                        
                                                                              
                                                                          
                                                                        
  
          
                                                               
  
              
                                                               
              
                                                                                                                            
                           
           
  
                                                                            
                                          
                                       
                                                          
                                        
                                       
                 
                                                                
                                                   
   
	virtual bool initWithObjects(
		const OSObject * objects[],
		const OSSymbol * keys[],
		unsigned int     count,
		unsigned int     capacity = 0);


   
                            
  
            
                                                                 
  
                                                                 
                                              
                                                                          
                                                  
                                                     
                                                                              
                                                                          
                                                                        
  
          
                                                               
  
              
                                                               
              
                                                                                                                            
                           
           
  
                                                                            
                                          
                                                                                               
                                         
                                       
                 
                                                                
                                                   
   
	virtual bool initWithObjects(
		const OSObject * objects[],
		const OSString * keys[],
		unsigned int     count,
		unsigned int     capacity = 0);


   
                               
  
            
                                 
                                           
  
                                                              
                                        
                                                                              
                                                                              
                                         
                                                                                    
                                                                       
  
          
                                                               
  
              
                                                               
                                                                  
  
                                                   
                                       
                                                          
                                         
                                       
                 
                                                                
                                                   
  
                                                                     
                           
              
   
	virtual bool initWithDictionary(
		const OSDictionary * dict,
		unsigned int         capacity = 0);


   
                 
  
            
                                        
                                     
  
              
                                               
      
              
                                                        
                       
           
   
	virtual void free() APPLE_KEXT_OVERRIDE;


   
                     
  
            
                                                 
                                   
  
          
                                         
                                   
   
	virtual unsigned int getCount() const APPLE_KEXT_OVERRIDE;


   
                        
  
            
                                                                               
  
          
                                                                    
  
              
                                      
                                              
      
              
                                                                                
                                    
      
              
                                                                                     
                               
   
	virtual unsigned int getCapacity() const APPLE_KEXT_OVERRIDE;


   
                                 
  
            
                                                   
  
          
                                           
  
              
                                                                      
                             
   
	virtual unsigned int getCapacityIncrement() const APPLE_KEXT_OVERRIDE;


   
                                 
  
            
                                                
  
          
                                               
                                                    
  
              
                                                                      
                             
                                                                 
   
	virtual unsigned int setCapacityIncrement(unsigned increment) APPLE_KEXT_OVERRIDE;


   
                           
  
            
                                          
                                                      
  
                                                                          
                                               
  
          
                                      
                                                   
                                                
  
              
                                                                  
                                                                     
                                                                        
                                                                       
  
                                                             
   
	virtual unsigned int ensureCapacity(unsigned int newCapacity) APPLE_KEXT_OVERRIDE;


   
                            
  
            
                                                                   
  
              
                                                                      
                                   
   
	virtual void flushCollection() APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                                  
  
                                                      
                                                 
                                                 
                                                              
                                                 
  
          
                                                    
                                
  
              
                                                                
   
	virtual bool setObject(
		const OSSymbol        * aKey,
		const OSMetaClassBase * anObject);

	bool setObject(
		OSSharedPtr<const OSSymbol> const& aKey,
		OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                      
  
                                                            
  
                                                      
                                                 
                                                              
                                                 
  
          
                                                    
                                
  
              
                                                           
                                                                
   
	virtual bool setObject(
		const OSString        * aKey,
		const OSMetaClassBase * anObject);

	bool setObject(
		const OSString        * aKey,
		OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                      
  
            
                                                  
  
                                                     
                                                 
                                                              
                                                 
  
          
                                                    
                                
  
              
                                                           
                                                                
   
	virtual bool setObject(
		const char            * aKey,
		const OSMetaClassBase * anObject);

	bool setObject(
		const char            * aKey,
		OSSharedPtr<const OSMetaClassBase> const& anObject);


   
                         
  
            
                                                 
  
                                                  
                                                  
  
              
                                                             
                                         
   
	virtual void removeObject(const OSSymbol * aKey);


   
                         
  
            
                                                 
  
                                                 
                                                  
  
              
                                                             
                                         
   
	virtual void removeObject(const OSString * aKey);


   
                         
  
            
                                                 
  
                                                 
                                                  
  
              
                                           
                                         
   
	virtual void removeObject(const char * aKey);


   
                  
  
            
                                                         
  
                                                    
                                                          
          
                                                                         
  
              
                                                                
                               
                                                
                                          
                                         
   
	virtual bool merge(const OSDictionary * aDictionary);


   
                      
  
            
                                               
  
                                                      
                                             
  
          
                                             
                                                                    
  
              
                                                                       
                                                      
              
                                                       
                      
                  
   
	virtual OSObject * getObject(const OSSymbol * aKey) const;


   
                      
  
                                                         
  
                                                      
                                         
  
          
                                             
                                                                    
  
              
                                                                       
                                                      
              
                                                       
                      
                  
   
	virtual OSObject * getObject(const OSString * aKey) const;


   
                      
  
            
                                               
  
                                                     
                                         
  
          
                                             
                                                                    
  
              
                                                                       
                                                      
              
                                                       
                      
                  
   
	virtual OSObject * getObject(const char * aKey) const;


   
                      
  
                                                           
                         
  
                                                                          
                                                                     
                                                                               
                                                                         
                                                                      
  
          
                                         
                                     
  
              
                                                                 
                                                     
                                          
                                     
              
                                                                                      
                          
   
	virtual bool isEqualTo(
		const OSDictionary * aDictionary,
		const OSCollection * keys) const;


   
                      
  
                                                            
  
                                                                          
  
          
                                                   
                             
  
              
                                                                         
                                                         
                                     
              
                                                                                      
                          
   
	virtual bool isEqualTo(const OSDictionary * aDictionary) const;


   
                      
  
            
                                                                
  
                                                                 
  
          
                                              
  
              
                                                        
                                              
                                               
   
	virtual bool isEqualTo(const OSMetaClassBase * anObject) const APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                          
                                                                    
  
                                             
  
          
                                                                          
   
	virtual bool serialize(OSSerialize * serializer) const APPLE_KEXT_OVERRIDE;


   
                       
  
            
                                                 
                             
  
                                                                              
                                               
                                                     
                                                                   
                                                 
                          
  
          
                                                           
  
              
                                                   
  
                                                                            
                           
   
	virtual unsigned setOptions(
		unsigned   options,
		unsigned   mask,
		void     * context = NULL) APPLE_KEXT_OVERRIDE;


   
                           
  
            
                                        
                             
  
                                                           
                                                  
                                                                
                                                        
                                            
  
          
                                                         
                                                                       
  
              
                                                                          
              
                                                              
                      
   
	OSPtr<OSCollection> copyCollection(OSDictionary * cycleDict = NULL) APPLE_KEXT_OVERRIDE;

#if XNU_KERNEL_PRIVATE
	bool setObject(const OSSymbol *aKey, const OSMetaClassBase *anObject, bool onlyAdd);
	void sortBySymbol(void);
	OSPtr<OSArray> copyKeys(void);
#endif                         


   
                           
  
            
                                                       
  
                                                         
                                         
                                                                      
                                                                    
                                                                     
                                                  
  
          
                                                     
                                                        
   
	bool iterateObjects(void * refcon, bool (*callback)(void * refcon, const OSSymbol * key, OSObject * object));

#ifdef __BLOCKS__

   
                           
  
            
                                                    
  
                             
                                                                      
                                                                    
                                                                     
                                                  
  
          
                                                     
                                                        
   
	bool iterateObjects(bool (^block)(const OSSymbol * key, OSObject * object));

#endif                 

	OSMetaClassDeclareReservedUnused(OSDictionary, 0);
	OSMetaClassDeclareReservedUnused(OSDictionary, 1);
	OSMetaClassDeclareReservedUnused(OSDictionary, 2);
	OSMetaClassDeclareReservedUnused(OSDictionary, 3);
	OSMetaClassDeclareReservedUnused(OSDictionary, 4);
	OSMetaClassDeclareReservedUnused(OSDictionary, 5);
	OSMetaClassDeclareReservedUnused(OSDictionary, 6);
	OSMetaClassDeclareReservedUnused(OSDictionary, 7);
};

#endif                             
