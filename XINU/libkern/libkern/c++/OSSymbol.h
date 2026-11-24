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
                                                  
                                                                                        

#ifndef _OS_OSSYMBOL_H
#define _OS_OSSYMBOL_H

#include <libkern/c++/OSString.h>
#include <libkern/c++/OSPtr.h>

class OSSymbol;

typedef OSSymbol* OSSymbolPtr;
typedef OSSymbol const* OSSymbolConstPtr;

   
          
  
            
                                                     
   

                                                

   
                  
  
            
                                                   
                                          
  
              
                                                              
                                              
                                               
                                       
                                                                  
                                                               
                                     
                                         
  
                                                                
                                                            
                                                          
  
                          
  
                                                                 
                                                   
                                         
                                                                  
                        
  
                                               
                                                                    
                                        
                                                                     
                                                                  
              
   
class OSSymbol : public OSString
{
	friend class OSSymbolPool;

	OSDeclareAbstractStructors(OSSymbol);

private:

	static void initialize();

   
                           
  
            
                                                       
  
                           
  
          
                      
  
              
                                                          
                                                    
   
	virtual bool initWithString(const OSString * aString) APPLE_KEXT_OVERRIDE;


   
                            
  
            
                                                       
  
                           
  
          
                      
  
              
                                                          
                                                    
   
	virtual bool initWithCString(const char * cString) APPLE_KEXT_OVERRIDE;


   
                                  
  
            
                                                       
  
                           
  
          
                      
  
              
                                                          
                                                    
   
	virtual bool initWithCStringNoCopy(const char *cString) APPLE_KEXT_OVERRIDE;

protected:

                                                      
   
                          
  
            
            
              
                                                                                 
                                                                
                                       
  
                                                           
                                                               
                                                                             
  
              
                                                                      
                                                       
                                 
   
	virtual void taggedRelease(
		const void * tag,
		const int    freeWhen) const APPLE_KEXT_OVERRIDE;


                                                      
   
                 
  
            
            
              
                                                     
                              
                                       
  
              
                                                                      
                                                       
                                 
   
	virtual void free() APPLE_KEXT_OVERRIDE;

public:

                                                      
   
                          
  
            
            
              
                                                                        
                                                     
                                       
  
                                                           
  
              
                                                                      
                                                       
                                 
   

                                     
                                                         
                                       
                                              
                                                                      
                                                                            
      
              
                                                                                 
                                                                 
   
	virtual void taggedRelease(const void * tag) const  APPLE_KEXT_OVERRIDE;


   
                       
  
            
                                                
                                                     
  
                                                           
  
          
                          
                                                             
                                
  
              
                                                                
                                                         
                                                                              
  
                                            
                                 
                                         
                                                           
   
	static OSPtr<const OSSymbol> withString(const OSString * aString);


   
                        
  
            
                                               
                                                     
  
                                                    
  
          
                                       
                                               
                                
  
              
                                                     
                                                         
                                                                              
  
                                            
                                 
                                         
                                                           
   
	static OSPtr<const OSSymbol> withCString(const char * cString);


   
                              
  
            
                                               
                               
                                                     
  
                                                   
          
                                       
                                               
                     
  
              
                             
                                                      
  
                                                     
                                                         
                                                                              
  
                                            
                                 
                                         
                                                           
   
	static OSPtr<const OSSymbol> withCStringNoCopy(const char * cString);

   
                                    
  
            
                                                       
  
                                                   
  
          
                                                
                                               
                                      
  
              
                                                                        
                             
   
	static OSPtr<const OSSymbol> existingSymbolForString(const OSString *aString);

   
                                     
  
            
                                                       
  
                                             
  
          
                                                
                                               
                                      
  
              
                                                                        
                             
   
	static OSPtr<const OSSymbol> existingSymbolForCString(const char *aCString);

   
                      
  
            
                                              
  
                                                                          
  
          
                                                                
                                
  
              
                                                                           
                                                                        
   
	virtual bool isEqualTo(const OSSymbol * aSymbol) const;


   
                      
  
                                                                      
  
                                                                
  
          
                                                 
                                    
                                
   
	virtual bool isEqualTo(const char * cString) const APPLE_KEXT_OVERRIDE;


   
                      
  
                                                                             
  
                                                                  
                                                                               
                                                
  
              
                                                    
                                 
                                                             
                                                        
   
	virtual bool isEqualTo(const OSMetaClassBase * anObject) const APPLE_KEXT_OVERRIDE;


#ifdef XNU_KERNEL_PRIVATE
                                              
                                      
                                                         

	static void checkForPageUnload(
		void * startAddr,
		void * endAddr);

	static unsigned int bsearch(
		const void *  key,
		const void *  array,
		unsigned int  arrayCount,
		size_t        memberSize);
#endif                         

	OSMetaClassDeclareReservedUnused(OSSymbol, 0);
	OSMetaClassDeclareReservedUnused(OSSymbol, 1);
	OSMetaClassDeclareReservedUnused(OSSymbol, 2);
	OSMetaClassDeclareReservedUnused(OSSymbol, 3);
	OSMetaClassDeclareReservedUnused(OSSymbol, 4);
	OSMetaClassDeclareReservedUnused(OSSymbol, 5);
	OSMetaClassDeclareReservedUnused(OSSymbol, 6);
	OSMetaClassDeclareReservedUnused(OSSymbol, 7);
};

#endif                      
