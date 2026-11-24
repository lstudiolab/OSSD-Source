/*
 * Copyright (c) 1998-2019 Apple Inc. All rights reserved.
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
 * Copyright (c) 1998 Apple Inc.  All rights reserved.
 *
 * HISTORY
 *
 */


#ifndef _IOKIT_IOCATALOGUE_H
#define _IOKIT_IOCATALOGUE_H

#include <libkern/c++/OSPtr.h>
#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSCollectionIterator.h>
#include <libkern/c++/OSArray.h>
#include <libkern/c++/OSDictionary.h>
#include <IOKit/IOLocks.h>
#include <sys/cdefs.h>

#include <IOKit/IOKitServer.h>

class IOService;

extern OSPtr<const OSSymbol> gIOModuleIdentifierKey;
extern OSPtr<const OSSymbol> gIOModuleIdentifierKernelKey;


   
                       
                                                                 
                                                                                                                                                                              
   
class IOCatalogue : public OSObject
{
	OSDeclareDefaultStructors(IOCatalogue);

private:
	IORWLock *               lock;
	SInt32                   generation;
	OSPtr<OSDictionary> personalities;
	OSArray * arrayForPersonality(OSDictionary * dict);
	void addPersonality(OSDictionary * dict);

public:
   
                         
                                                                                                                
   
	static void initialize( void );

   
                   
                                               
                                                                                          
   
	bool init( OSArray * initArray );

   
                   
                                                                                                                                                  
   
	void free( void ) APPLE_KEXT_OVERRIDE;

   
                          
                                                             
                               
                                                                                                                                                                                                                       
                                                                                                                                      
   
	OSPtr<OSOrderedSet>  findDrivers( IOService * service, SInt32 * generationCount );

   
                          
                                                                                                                                                                                              
                                                                                                                                                                                                                                                                         
                                                                                                                                                                                                                         
                                                                                                                                     
   
	OSPtr<OSOrderedSet>  findDrivers( OSDictionary * matching, SInt32 * generationCount );

   
                         
                                                                     
                                                                            
                                                                                     
                                                                                                                                      
   
	bool addDrivers( OSArray * array, bool doNubMatching = true );

   
                            
                                                                                                    
                                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                                               
                                                                                                                 
   
	bool removeDrivers( OSDictionary * matching, bool doNubMatching = true );

#if XNU_KERNEL_PRIVATE
	bool removeDrivers(bool doNubMatching, bool (^shouldRemove)(OSDictionary *personality));
#endif                         

   
                                 
                                                                
   
	SInt32 getGenerationCount( void ) const;

   
                             
                                                                                       
                                                         
                                                           
                                                                                                                                                  
   
	bool isModuleLoaded( OSDictionary * driver, OSObject ** kextRef ) const;

	bool isModuleLoaded( OSDictionary * driver, OSSharedPtr<OSObject>& kextRef ) const;

   
                              
                                                                                        
                                            
   
	void moduleHasLoaded( const OSSymbol * name );

   
                              
                                                                                        
                                            
   
	void moduleHasLoaded( const char * name );

   
                               
                                                                                                                                
                                                                                                                                                                                                                                                                                                                                     
   
	IOReturn terminateDrivers( OSDictionary * matching );

   
                                        
                                                                                                                
                                                                                                                    
                                                                    
   
	IOReturn terminateDriversForModule( OSString * moduleName, bool unload = true);

   
                                        
                                                                                                                
                                                                                                                    
                                                                    
   
	IOReturn terminateDriversForModule( const char * moduleName, bool unload = true);
#if XNU_KERNEL_PRIVATE
	IOReturn terminateDrivers(OSDictionary * matching, io_name_t className);

	IOReturn terminateDriversForUserspaceReboot();

	IOReturn resetAfterUserspaceReboot();
#endif                         

   
                            
                                           
                                                                                         
   
	bool startMatching( const OSSymbol * identifier );

	                             
#if defined(__i386__) || defined(__x86_64__)
	bool startMatching( OSDictionary * matching );
#endif

   
                    
                                                         
                
                                                                                                   
   
	void reset(void);

   
                                 
                                                                      
                
                                                                                                                   
   
	bool resetAndAddDrivers(OSArray * drivers, bool doNubMatching = true);

   
                        
                                                                
                                    
                                                                                               
   
	virtual bool serialize(OSSerialize * s) const APPLE_KEXT_OVERRIDE;

	bool serializeData(IOOptionBits kind, OSSerialize * s) const;

                                                                 
                                                             
   

private:

   
                           
                                                                                                
                                                                               
   
	IOReturn unloadModule( OSString * moduleName ) const;

	IOReturn _removeDrivers(OSDictionary * matching);
};

extern OSPtr<const OSSymbol> gIOClassKey;
extern OSPtr<const OSSymbol> gIOProbeScoreKey;
extern OSPtr<IOCatalogue> gIOCatalogue;

#endif                             
