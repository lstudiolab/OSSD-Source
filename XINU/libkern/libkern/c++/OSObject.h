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
/*
 *  Copyright (c) 1998 Apple Computer, Inc.  All rights reserved.
 *  HISTORY
 *   1998-10-30  Godfrey van der Linden(gvdl)
 *   Created
 */
#ifndef _LIBKERN_OSOBJECT_H
#define _LIBKERN_OSOBJECT_H

#include <libkern/c++/OSMetaClass.h>
#include <libkern/c++/OSPtr.h>
#include <IOKit/IORPC.h>
#include <DriverKit/OSObject.h>

#if defined(__clang__)
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif

class OSSymbol;
class OSString;
class OSObject;

typedef OSObject* OSObjectPtr;


   
          
  
            
                                           
                                                                 
   


   
                  
  
            
                                      
                                                  
  
              
                                             
                                               
                                                    
                                                  
                          
                                                                            
                                                                     
                                            
  
                                   
  
                                                   
                                                                     
                                                      
                                                            
                                                        
                                                                    
                                      
                                                                
                        
  
                                           
  
                                                               
                                                    
                                
                                         
                                                            
                                                            
                                                 
                                      
                                                  
                                           
                                                                     
                                                
                               
                            
              
                                                        
                       
                                                                         
                                                                             
                                   
                                                       
  
                            
  
                                                          
              
                                                       
                       
              
                                                        
                          
              
                                                           
                                     
      
              
                                                     
                    
             
                                                    
              
                                                       
                       
      
              
                                                        
                        
              
                                                           
                                     
              
                                                
  
                                                      
              
                                                     
                     
                                 
              
                                                           
                                     
                                                             
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                
  
                                                  
                                                                           
                                           
                                                                                 
                                            
                                                              
                                                       
   
class OSObject : public OSMetaClassBase
{
	OSDeclareAbstractStructorsWithDispatchAndNoOperators(OSObject);

#if IOKITSTATS
	friend class IOStatistics;
#endif

private:
                                   
  
                                                               
   
	mutable int retainCount;

protected:

                                                        

   
                    
  
            
                                     
                                                
                                       
  
                                                               
                                                                            
  
              
                                                                  
                                            
                           
  
                                       
                                                               
                                             
   
	virtual void release(int freeWhen) const APPLE_KEXT_OVERRIDE;

   
                          
  
            
                                            
                                                
                                       
  
                                                           
                                                               
                                                                            
  
              
                                                  
                                                        
                            
  
                                                                  
                                            
                           
  
                                       
                                                               
                                             
   
	virtual void taggedRelease(const void * tag, const int freeWhen) const APPLE_KEXT_OVERRIDE;


   
                 
  
            
                                        
  
          
                                                               
  
              
                                                                      
                   
                                       
                                    
                                     
                                                                           
                                                                                 
   
	virtual bool init();


   
                 
  
            
                                                     
  
              
                                                              
                                                                       
                                             
  
                  
       
                                                                
                                          
                                                      
                                                                     
                                                               
                                                              
                                       
                                                                        
        
   
	virtual void free();


   
                            
  
            
                                         
  
                                                 
                                                         
  
              
                                                                  
      
              
                                                        
                       
           
   
#ifdef XNU_KERNEL_PRIVATE
	static void operator delete(void * mem, size_t size)
	__XNU_INTERNAL(OSObject_operator_delete);
#else
	static void operator delete(void * mem, size_t size);
#endif


                                          
                               
#if 0
private:
#else
public:
#endif

   
                         
  
            
                                                 
  
                                              
  
          
                                                                          
   
#ifdef XNU_KERNEL_PRIVATE
	static void * operator new(size_t size)
	__XNU_INTERNAL(OSObject_operator_new);
#else
	static void * operator new(size_t size);
#endif

public:

   
                           
  
            
                                             
  
          
                                     
   
	virtual int getRetainCount() const APPLE_KEXT_OVERRIDE;


   
                   
  
            
                                     
  
              
                                                                     
                                                   
                                                
                                           
   
	virtual void retain() const APPLE_KEXT_OVERRIDE;


   
                    
  
            
                                      
                                                               
  
              
                                                                     
                                        
                                        
              
                                                     
                     
   
	virtual void release() const APPLE_KEXT_OVERRIDE;


   
                         
  
            
                                                     
                                   
  
                                                           
  
              
                                                  
                                                        
                            
  
                                                   
                                                
                                           
   
	virtual void taggedRetain(const void * tag = NULL) const APPLE_KEXT_OVERRIDE;


   
                          
  
            
                                            
                                                
                 
  
                                                           
  
              
                                                  
                                                        
                            
   
	virtual void taggedRelease(const void * tag = NULL) const APPLE_KEXT_OVERRIDE;
                                                                                                                                                                                                                                           


   
                      
  
            
                                                                     
                                                                    
  
                                            
  
          
                                                                          
  
              
                                                            
                                                      
                       
                                                     
                                                                          
      
                                                            
                        
   
	virtual bool serialize(OSSerialize * serializer) const APPLE_KEXT_OVERRIDE;

#ifdef XNU_KERNEL_PRIVATE
#if IOTRACKING
	void trackingAccumSize(size_t size);
#endif

	bool taggedTryRetain(const void *tag) const;

	bool iterateObjects(void * refcon, bool (*callback)(void * refcon, OSObject * object));
#ifdef __BLOCKS__
	bool iterateObjects(bool (^block)(OSObject * object));
#endif                 

#endif                         

                 
	OSMetaClassDeclareReservedUnused(OSObject, 0);
	OSMetaClassDeclareReservedUnused(OSObject, 1);
	OSMetaClassDeclareReservedUnused(OSObject, 2);
	OSMetaClassDeclareReservedUnused(OSObject, 3);
	OSMetaClassDeclareReservedUnused(OSObject, 4);
	OSMetaClassDeclareReservedUnused(OSObject, 5);
	OSMetaClassDeclareReservedUnused(OSObject, 6);
	OSMetaClassDeclareReservedUnused(OSObject, 7);
	OSMetaClassDeclareReservedUnused(OSObject, 8);
	OSMetaClassDeclareReservedUnused(OSObject, 9);
	OSMetaClassDeclareReservedUnused(OSObject, 10);
	OSMetaClassDeclareReservedUnused(OSObject, 11);
	OSMetaClassDeclareReservedUnused(OSObject, 12);
	OSMetaClassDeclareReservedUnused(OSObject, 13);
	OSMetaClassDeclareReservedUnused(OSObject, 14);
	OSMetaClassDeclareReservedUnused(OSObject, 15);
};

#endif                           
