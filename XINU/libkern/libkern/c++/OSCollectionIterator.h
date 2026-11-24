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
                                                              

#ifndef _OS_OSCOLLECTIONITERATOR_H
#define _OS_OSCOLLECTIONITERATOR_H

#include <libkern/c++/OSIterator.h>
#include <libkern/c++/OSCollection.h>
#include <libkern/c++/OSPtr.h>

class OSCollectionIterator;

typedef OSCollectionIterator* OSCollectionIteratorPtr;

   
          
  
            
                                                                  
   


   
                              
  
              
                                                                 
                                          
                                       
                                                          
                                                                  
  
                                                                    
                            
                                                          
                                   
  
             
        
                                              
                                                              
                         
                                                   
                                      
        
                  
                                  
                                                             
        
                          
         
              
  
                                                    
                                                               
                                                  
                                                       
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                                           
   
class OSCollectionIterator : public OSIterator
{
	OSDeclareDefaultStructors(OSCollectionIterator);

protected:
                                           
	OSPtr<const OSCollection> collection;
	void               * collIterator;
	unsigned int         initialUpdateStamp;
	bool                 valid;

public:
   
                           
  
            
                                                  
                                      
  
                                                                              
  
          
                                                                           
   
	static OSPtr<OSCollectionIterator> withCollection(const OSCollection * inColl);


   
                               
  
            
                                      
                                      
  
                                                                              
          
                                                          
                                    
  
              
                                                               
                                                                  
   
	virtual bool initWithCollection(const OSCollection * inColl);


   
                 
  
            
                                             
                                      
  
              
                                               
      
              
                                                        
                       
           
   
	virtual void free() APPLE_KEXT_OVERRIDE;


   
                  
  
            
                                                          
                                  
   
	virtual void reset() APPLE_KEXT_OVERRIDE;


   
                    
  
            
                                                                    
  
          
                                                                
                               
                                                               
   
	virtual bool isValid() APPLE_KEXT_OVERRIDE;


   
                          
  
            
                                                            
  
          
                                            
                                               
                                         
  
              
                            
                                                                                
                       
                                                 
                              
  
                                                       
                             
              
                                                                    
                        
                                                                        
                                                                      
   
	virtual OSObject * getNextObject() APPLE_KEXT_OVERRIDE;
};

#endif                                  
