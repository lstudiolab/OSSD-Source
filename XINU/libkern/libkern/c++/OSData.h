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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
                                                    
                                                         

#ifndef _OS_OSDATA_H
#define _OS_OSDATA_H

#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSPtr.h>
#include <os/base.h>

class OSData;
class OSString;

typedef OSData* OSDataPtr;
typedef OSData const* OSDataConstPtr;

   
          
  
            
                                                   
   


   
                
  
            
                                                 
                                  
  
              
                                                               
                              
                                
                                        
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                             
                                                                    
                                        
                                                                     
                                                                  
              
   
class OSData : public OSObject
{
	friend class OSSerialize;

	OSDeclareDefaultStructors(OSData);

#if APPLE_KEXT_ALIGN_CONTAINERS

protected:
	unsigned int   length;
	unsigned int   capacity;
	unsigned int   capacityIncrement;
	void         * OS_PTRAUTH_SIGNED_PTR("OSData.data") data;

#else                                  

protected:
	void         * OS_PTRAUTH_SIGNED_PTR("OSData.data") data;
	unsigned int   length;
	unsigned int   capacity;
	unsigned int   capacityIncrement;

#endif                                  

#ifdef XNU_KERNEL_PRIVATE
                                      
public:
	typedef void (*DeallocFunction)(void * ptr, unsigned int length);
protected:
	struct ExpansionData {
		DeallocFunction deallocFunction;
		bool            disableSerialization;
	};
#else                         
private:
	typedef void (*DeallocFunction)(void * ptr, unsigned int length);
protected:
	struct ExpansionData;
#endif                         

                                                   
	ExpansionData * reserved;

public:

   
                         
  
            
                                                       
  
                                                                       
  
          
                                                     
                                
  
              
                                     
                                                      
                                                                    
                                                                         
                                                         
   
	static OSPtr<OSData> withCapacity(unsigned int capacity);


   
                      
  
            
                                                
                                           
  
                                               
                                                     
  
          
                                                                      
                               
                                
  
              
                                                                      
                                                                         
                                                         
   
	static OSPtr<OSData> withBytes(
		const void   * bytes,
		unsigned int   numBytes);


   
                            
  
            
                                                
                                        
  
                                                    
                                                     
  
          
                                                            
                               
                                
  
              
                                              
                           
                                                     
                                                                             
                                            
                                                  
                                                                      
  
                                                                          
                                       
                                         
   
	static OSPtr<OSData> withBytesNoCopy(
		void         * bytes,
		unsigned int   numBytes);


   
                     
  
            
                                                
                                                   
  
                                                                 
  
          
                                                                              
                               
                                
  
              
                                                                      
                                                                         
                                                         
   
	static OSPtr<OSData> withData(const OSData * inData);


   
                     
  
            
                                                
                                                                  
  
                                                                    
                                                                       
                                                                             
  
          
                                          
                                                        
                               
                                
  
              
                                                                      
                                                                         
                                                         
   
	static OSPtr<OSData> withData(
		const OSData * inData,
		unsigned int   start,
		unsigned int   numBytes);


   
                             
  
            
                                     
  
                                                                      
  
          
                                                               
  
              
                                                               
              
                                                                       
                                     
  
                                     
                                                      
                                                                    
                                                                         
                                                         
   
	virtual bool initWithCapacity(unsigned int capacity);


   
                          
  
            
                                    
                                           
  
                                               
                                                     
  
          
                                                               
  
              
                                                               
                                                        
  
                                                                      
                                                                         
                                                         
   
	virtual bool initWithBytes(
		const void   * bytes,
		unsigned int   numBytes);


   
                                
  
            
                                    
                                     
  
                                                    
                                                     
  
          
                                                               
  
              
                                                               
                                                                    
  
                                                  
                           
                                                            
  
                                                                          
                                       
                                         
   
	virtual bool initWithBytesNoCopy(
		void         * bytes,
		unsigned int   numBytes);


   
                         
  
            
                                                
                                                   
  
                                                                 
  
          
                                                               
  
              
                                                               
              
                                                                    
                                  
           
  
                                                                      
                                                                         
                                                         
   
	virtual bool initWithData(const OSData * inData);


   
                         
  
            
                                    
                                                                  
  
                                                                    
                                                                       
                                                                             
  
          
                                                                       
  
              
                                                               
              
                                                                                            
                                                              
           
  
                                                                      
                                                                         
                                                         
   
	virtual bool initWithData(
		const OSData * inData,
		unsigned int   start,
		unsigned int   numBytes);


   
                 
  
            
                                        
                               
  
              
                                               
      
              
                                                        
                       
           
   
	virtual void free() APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                                                     
  
          
                                                             
   
	virtual unsigned int getLength() const;


   
                        
  
            
                                                                               
  
          
                                                                    
  
              
                                                                 
      
              
                                                                          
                                    
      
              
                                                                               
                               
  
                                                               
                                                                       
   
	virtual unsigned int getCapacity() const;


   
                                 
  
            
                                                      
  
          
                                              
  
              
                                                            
                             
  
                                                               
                                     
   
	virtual unsigned int getCapacityIncrement() const;


   
                                 
  
            
                                           
  
          
                                               
  
              
                                                        
                             
  
                                                               
                                     
   
	virtual unsigned int setCapacityIncrement(unsigned increment);


                                                     

   
                           
  
            
                                     
                                          
  
                                                                 
                                              
  
          
                                                 
                                                   
                                                
  
              
                                                                       
                                                          
                                                                        
                                                                       
  
                                                       
  
                                                             
   
	virtual unsigned int ensureCapacity(unsigned int newCapacity);


   
                        
  
            
                                                                         
  
                                                    
                                                              
                                                                             
                                
                                                                          
  
          
                                                            
                                 
  
              
                                                                       
                                     
  
                                                         
                  
   
	virtual bool appendBytes(
		const void   * bytes,
		unsigned int   numBytes);


   
                        
  
            
                                                       
  
                                                                      
  
          
                                                            
                                 
  
              
                                                                       
                                     
  
                                                         
                  
   
	virtual bool appendBytes(const OSData * aDataObj);


   
                           
  
            
                                                                 
  
          
                                                         
  
              
                                                           
                     
                                                                
                                                           
  
                                                         
                                                  
                                                         
   
	virtual const void * getBytesNoCopy() const;


   
                           
  
            
                                                                  
                                  
  
                                                                        
                                             
  
          
                                                
                            
                                             
                              
  
              
                                                           
                     
                                                                
                                                           
  
                                                         
                                                  
                                                         
   
	virtual const void * getBytesNoCopy(
		unsigned int start,
		unsigned int numBytes) const;


   
                      
  
            
                                            
  
                                                                         
  
          
                                                              
                                
  
              
                                          
                                        
                                       
   
	virtual bool isEqualTo(const OSData * aDataObj) const;


   
                      
  
            
                                                    
                         
  
                                                     
                                                  
  
          
                                                  
                         
                                
   
	virtual bool isEqualTo(
		const void   * bytes,
		unsigned int   numBytes) const;


   
                      
  
            
                                                                 
  
                                                                  
  
          
                                                       
                                
  
              
                                                  
                                        
                                                        
   
	virtual bool isEqualTo(const OSMetaClassBase * anObject) const APPLE_KEXT_OVERRIDE;


   
                      
  
            
                                                         
  
                                                                         
  
          
                                                       
                                
  
              
                                                        
                                 
                                                
                                 
                                         
                                                 
                                         
                      
                                                          
   
	virtual bool isEqualTo(const OSString * aString) const;


   
                      
  
            
                                          
                                                                        
  
                                            
  
          
                                                                          
   
	virtual bool serialize(OSSerialize * serializer) const APPLE_KEXT_OVERRIDE;


   
                       
  
            
                              
                                              
                               
  
                                            
                                                                       
  
          
                                                            
                             
  
              
                                                                       
                                     
  
                                                         
                  
   
	virtual bool appendByte(
		unsigned char byte,
		unsigned int  numBytes);


	void setSerializable(bool serializable);

#ifdef XNU_KERNEL_PRIVATE
                                      
public:
#else
private:
#endif
	virtual void setDeallocFunction(DeallocFunction func);
	bool isSerializable(void);

private:
	OSMetaClassDeclareReservedUsedX86(OSData, 0);
	OSMetaClassDeclareReservedUnused(OSData, 1);
	OSMetaClassDeclareReservedUnused(OSData, 2);
	OSMetaClassDeclareReservedUnused(OSData, 3);
	OSMetaClassDeclareReservedUnused(OSData, 4);
	OSMetaClassDeclareReservedUnused(OSData, 5);
	OSMetaClassDeclareReservedUnused(OSData, 6);
	OSMetaClassDeclareReservedUnused(OSData, 7);
};

#endif                    
