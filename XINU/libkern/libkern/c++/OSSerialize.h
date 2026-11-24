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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
                                                         

#ifndef _OS_OSSERIALIZE_H
#define _OS_OSSERIALIZE_H

#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSPtr.h>

class OSCollection;
class OSSet;
class OSDictionary;
class OSArray;
class OSData;

class OSSerializer;
typedef OSSerializer* OSSerializerPtr;

class OSSerialize;
typedef OSSerialize* OSSerializePtr;

   
          
  
            
                                              
   

OSObjectPtr
OSUnserializeBinary(const void *buffer, size_t bufferSize);

   
                     
  
            
                                                               
                      
  
              
                                                                       
                                                                           
                                  
                                            
               
              
                                                                      
                                     
            
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                                  
                                                                    
                                        
                                                                     
                                                      
                              
   

class OSSerialize : public OSObject
{
	OSDeclareDefaultStructors(OSSerialize);
	friend class OSBoolean;

private:
	char         * data;                                           
	unsigned int   length;                                              
	unsigned int   capacity;                      
	unsigned int   capacityIncrement;               

	OSPtr<OSArray> tags;                                                

#ifdef XNU_KERNEL_PRIVATE
public:
	typedef OSPtr<const OSMetaClassBase> (*Editor)(void                  * reference,
	    OSSerialize           * s,
	    OSCollection          * container,
	    const OSSymbol        * name,
	    const OSMetaClassBase * value);
#else
	typedef void * Editor;
#endif

	bool     binary;
	bool     endCollection;
	Editor   editor;
	void   * editRef;
	OSPtr<OSData> indexData;

	bool binarySerialize(const OSMetaClassBase *o);
	bool binarySerializeInternal(const OSMetaClassBase *o);
	bool addBinary(const void * data, size_t size);
	bool addBinaryObject(const OSMetaClassBase * o, uint32_t key, const void * _bits, uint32_t size,
	    uint32_t * startCollection);
	void endBinaryCollection(uint32_t startCollection);

public:

   
                         
  
            
                                                       
  
                                                       
  
          
                                
                            
                                
  
              
                                                               
   
	static OSPtr<OSSerialize> withCapacity(unsigned int capacity);

	static OSPtr<OSSerialize> binaryWithCapacity(unsigned int inCapacity, Editor editor = NULL, void * reference = NULL);
	void setIndexed(bool index);

   
                 
  
            
                                          
  
          
                                                 
   
	virtual char * text() const;


   
                      
  
            
                                 
  
              
                                                                
                              
   
	virtual void clearText();

                                 

   
                                 
  
            
                                                        
                                                       
  
                                     
  
          
                                                                       
                                             
                                                 
                                
  
  
              
                                                       
                                                                   
                                                           
                                         
                                                         
                                              
  
                            
              
                                                                      
                                    
                                                                   
                                                                             
                                            
   
	virtual bool previouslySerialized(const OSMetaClassBase * object);


   
                           
  
            
                                              
  
                                                
                                                                           
  
          
                                                                   
                                                                         
  
              
                                     
                                            
  
                                                                  
                                                                   
                          
                                                           
                   
       
                 
                
                   
               
               
                  
        
  
                                                       
                                                     
                                         
   
	virtual bool addXMLStartTag(
		const OSMetaClassBase * object,
		const char            * tagString);


   
                         
  
            
                                            
  
                                                                           
  
          
                                                                 
                                                                         
  
              
                                     
                                                                      
                                                
  
                                                          
                                                         
                                         
   
	virtual bool addXMLEndTag(const char * tagString);


   
                    
  
            
                                                
  
                                                          
  
          
                                         
                                                                         
   
	virtual bool addChar(const char aChar);


   
                      
  
            
                                        
  
                                                           
  
          
                                             
                                                                         
   
	virtual bool addString(const char * cString);

                                                 

	virtual bool initWithCapacity(unsigned int inCapacity);
	virtual unsigned int getLength() const;
	virtual unsigned int getCapacity() const;
	virtual unsigned int getCapacityIncrement() const;
	virtual unsigned int setCapacityIncrement(unsigned increment);
	virtual unsigned int ensureCapacity(unsigned int newCapacity);
	virtual void free() APPLE_KEXT_OVERRIDE;

	OSMetaClassDeclareReservedUnused(OSSerialize, 0);
	OSMetaClassDeclareReservedUnused(OSSerialize, 1);
	OSMetaClassDeclareReservedUnused(OSSerialize, 2);
	OSMetaClassDeclareReservedUnused(OSSerialize, 3);
	OSMetaClassDeclareReservedUnused(OSSerialize, 4);
	OSMetaClassDeclareReservedUnused(OSSerialize, 5);
	OSMetaClassDeclareReservedUnused(OSSerialize, 6);
	OSMetaClassDeclareReservedUnused(OSSerialize, 7);
};


typedef bool (*OSSerializerCallback)(void * target, void * ref,
    OSSerialize * serializer);

#ifdef __BLOCKS__
typedef bool (^OSSerializerBlock)(OSSerialize * serializer);
#endif                 


class OSSerializer : public OSObject
{
	OSDeclareDefaultStructors(OSSerializer);

	void * target;
	void * ref;
	OSSerializerCallback callback;

public:

	static OSPtr<OSSerializer> forTarget(
		void * target,
		OSSerializerCallback callback,
		void * ref = NULL);

#ifdef __BLOCKS__
	static OSPtr<OSSerializer> withBlock(
		OSSerializerBlock callback);
#endif

	virtual void free( void ) APPLE_KEXT_OVERRIDE;

#if XNU_KERNEL_PRIVATE
	static bool callbackToBlock(void * target, void * ref,
	    OSSerialize * serializer);
#endif                         

	virtual bool serialize(OSSerialize * serializer) const APPLE_KEXT_OVERRIDE;
};

#endif                        
