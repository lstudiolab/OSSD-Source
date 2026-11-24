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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
#ifndef _PEXPERT_DEVICE_TREE_H_
#define _PEXPERT_DEVICE_TREE_H_

#include <stdbool.h>

#include <mach/mach_types.h>
#include <mach/vm_types.h>

#include <sys/appleapiopts.h>

#ifdef __APPLE_API_PRIVATE

#ifdef __cplusplus
extern "C" {
#endif

  
                                                                                   
                    
                                                                                   
   
enum {
	kDTPathNameSeparator    = '/'                                     
};


                                                             
enum {
	kDTMaxPropertyNameLength=31                                                                
};

typedef char DTPropertyNameBuf[32];


                                                       
enum {
	kDTMaxEntryNameLength           = 63                                                                       
};

                                                        
typedef char DTEntryNameBuf[kDTMaxEntryNameLength + 1];

  
                                          
   

#define kPropNameLength 32

typedef struct DeviceTreeNodeProperty {
	char                name[kPropNameLength];                               
	uint32_t            length;                                             
                                                                
	                                      
} DeviceTreeNodeProperty;

typedef struct OpaqueDTEntry {
	uint32_t            nProperties;                                        
	uint32_t            nChildren;                                  
                                                               
                                                         
} DeviceTreeNode;

typedef const DeviceTreeNode *RealDTEntry;

typedef struct DTSavedScope {
	struct DTSavedScope * nextScope;
	RealDTEntry scope;
	RealDTEntry entry;
	unsigned long index;
} *DTSavedScopePtr;

                   
typedef struct OpaqueDTEntryIterator {
	RealDTEntry outerScope;
	RealDTEntry currentScope;
	RealDTEntry currentEntry;
	DTSavedScopePtr savedScope;
	unsigned long currentIndex;
} OpaqueDTEntryIterator, *DTEntryIterator;

                      
typedef struct OpaqueDTPropertyIterator {
	RealDTEntry entry;
	DeviceTreeNodeProperty const *currentProperty;
	unsigned long currentIndex;
} OpaqueDTPropertyIterator, *DTPropertyIterator;

          
typedef const struct OpaqueDTEntry* DTEntry;

                   
typedef struct OpaqueDTEntryIterator* DTEntryIterator;

                      
typedef struct OpaqueDTPropertyIterator* DTPropertyIterator;


                  
enum {
	kError = -1,
	kIterationDone = 0,
	kSuccess = 1
};


#ifndef __MWERKS__
  
                                                                                   
                     
                                                                                   
   

                                                  
                                                          
extern void SecureDTInit(void const *base, size_t size);

                                                                    
                                                             
                                    
extern bool SecureDTIsLockedDown(void);

  
                                                                                   
                  
                                                                                   
   
                                       
extern int SecureDTEntryIsEqual(const DTEntry ref1, const DTEntry ref2);

  
                                                                                   
                        
                                                                                   
   
  
              
                                                                
                                     
                                                              
                                                                
                                            
   
extern int SecureDTFindEntry(const char *propName, const char *propValue, DTEntry *entryH);

  
                
                                                                                   
                                                                                
                                               
                                                                    
                                            
   
extern int SecureDTLookupEntry(const DTEntry searchPoint, const char *pathName, DTEntry *foundEntry);

  
                                                                                   
                   
                                                                                   
   
  
                                                                                
                                                                                  
                                                                                  
                                                                                   
                                                                                   
                        
  
                             
                                                                                        
                                                                            
                                                                                    
                 
   
extern int SecureDTInitEntryIterator(const DTEntry startEntry, DTEntryIterator iter);

  
                     
                                                                          
                                                                                   
                                                                              
                                    
   
extern int SecureDTEnterEntry(DTEntryIterator iterator, DTEntry childEntry);

  
                        
                                                                                      
                                                                                 
                                                                                       
                                                             
   
extern int SecureDTExitEntry(DTEntryIterator iterator, DTEntry *currentPosition);

  
                   
                                                                                
                                                                    
                                                                   
                                   
   
extern int SecureDTIterateEntries(DTEntryIterator iterator, DTEntry *nextEntry);

  
                           
                                                                                   
                                                                             
                                                                                      
                                     
   
extern int SecureDTRestartEntryIteration(DTEntryIterator iterator);

  
                                                                                   
                       
                                                                                   
   
  
                                                                    
  
                
   
extern int SecureDTGetProperty(const DTEntry entry, const char *propertyName,
    void const **propertyValue, unsigned int *propertySize);

extern int SecureDTGetPropertyRegion(const DTEntry entry, const char *propertyName,
    void const **propertyValue, unsigned int *propertySize,
    vm_offset_t const region_start, vm_size_t region_size);

  
                                                                                   
                        
                                                                                   
   
  
                                
                                                                                   
   
extern int SecureDTInitPropertyIterator(const DTEntry entry, DTPropertyIterator iter);

  
                      
                                                  
                                                                   
   

extern int SecureDTIterateProperties(DTPropertyIterator iterator,
    char const **foundProperty);

  
                              
                                                                           
                                                                  
   

extern int SecureDTRestartPropertyIteration(DTPropertyIterator iterator);

#ifdef __cplusplus
}
#endif

#endif                 

#endif                          

#endif                              
