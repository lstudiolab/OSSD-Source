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
 * Copyright (c) 1998 Apple Computer, Inc.  All rights reserved.
 *
 * HISTORY
 *
 */


#ifndef _IOKIT_IOREGISTRYENTRY_H
#define _IOKIT_IOREGISTRYENTRY_H

#include <IOKit/IOTypes.h>
#include <libkern/c++/OSContainers.h>
#include <libkern/c++/OSPtr.h>

#if defined(IOKIT_ENABLE_SHARED_PTR)
                 
#define APPLE_KEXT_DEPRECATED_WITH_SHARED_PTR  __attribute__((deprecated))
#else
                 
#define APPLE_KEXT_DEPRECATED_WITH_SHARED_PTR
#endif                              


extern const OSSymbol * gIONameKey;
extern const OSSymbol * gIOLocationKey;
extern const OSSymbol * gIORegistryEntryIDKey;
extern const OSSymbol * gIORegistryEntryPropertyKeysKey;

class IORegistryEntry;
class IORegistryPlane;
class IORegistryIterator;

typedef void (*IORegistryEntryApplierFunction)(IORegistryEntry * entry,
    void * context);

enum {
	kIORegistryIterateRecursively   = 0x00000001,
	kIORegistryIterateParents       = 0x00000002,
};

#ifdef KERNEL_PRIVATE
enum{
	kIORegistryEntryIndexedPropertyCLPC = 0,
	kIORegistryEntryIndexedPropertyCount,
};
#endif                     

                                            
                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
   

class IORegistryEntry : public OSObject
{
	friend class IORegistryIterator;

	OSDeclareDefaultStructors(IORegistryEntry);

protected:
                         
                                                                                                    
   
	struct ExpansionData;

                 
                                                      
	ExpansionData * reserved;

private:

	OSDictionary *      fRegistryTable;
	OSDictionary *      fPropertyTable;

public:
                                                 

                          
                                                                                                                                                                        
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                   
                                                                 
                                                                                                                                                                                                                                                                                                         
                                                                                                                                     

	virtual OSPtr<OSObject> copyProperty( const char *           aKey,
	    const IORegistryPlane * plane,
	    IOOptionBits            options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const;

                          
                                                                                                                                                                        
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                    
                                                                 
                                                                                                                                                                                                                                                                                                         
                                                                                                                                     

	virtual OSPtr<OSObject> copyProperty( const OSString *        aKey,
	    const IORegistryPlane * plane,
	    IOOptionBits            options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const;

                          
                                                                                                                                                                        
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                    
                                                                 
                                                                                                                                                                                                                                                                                                         
                                                                                                                                     

	virtual OSPtr<OSObject> copyProperty( const OSSymbol *        aKey,
	    const IORegistryPlane * plane,
	    IOOptionBits            options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const;

                             
                                                                                                              
                                                                                                                                                                                                  
                                   
                                                                                                                                                                                                               

	virtual OSPtr<IORegistryEntry> copyParentEntry( const IORegistryPlane * plane ) const;

                            
                                                                                                             
                                                                                              
                                   
                                                                                                                                                                                                              

	virtual OSPtr<IORegistryEntry> copyChildEntry( const IORegistryPlane * plane ) const;

                                                
   
                    
                                                                           
                                                                                
                                                                                                                        
                  
                                                                                 
                                                                                 
                                     
                                                       
                                                       
                                                       
                                                       
   
	typedef IOReturn (*Action)(OSObject *target,
	    void *arg0, void *arg1,
	    void *arg2, void *arg3);

                               
                                                                         
                                                                                                                                                                                                                                          
                                                                           
                                                               
                                                               
                                                               
                                                               
                                                     
   
	virtual IOReturn runPropertyAction(Action action, OSObject *target,
	    void *arg0 = NULL, void *arg1 = NULL,
	    void *arg2 = NULL, void *arg3 = NULL);

#ifdef __BLOCKS__
	typedef IOReturn (^ActionBlock)(void);
	IOReturn runPropertyActionBlock(ActionBlock block);
#endif                 

private:
#if __LP64__
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 0);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 1);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 2);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 3);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 4);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 5);
#else
	OSMetaClassDeclareReservedUsedX86(IORegistryEntry, 0);
	OSMetaClassDeclareReservedUsedX86(IORegistryEntry, 1);
	OSMetaClassDeclareReservedUsedX86(IORegistryEntry, 2);
	OSMetaClassDeclareReservedUsedX86(IORegistryEntry, 3);
	OSMetaClassDeclareReservedUsedX86(IORegistryEntry, 4);
	OSMetaClassDeclareReservedUsedX86(IORegistryEntry, 5);
#endif
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 6);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 7);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 8);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 9);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 10);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 11);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 12);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 13);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 14);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 15);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 16);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 17);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 18);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 19);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 20);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 21);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 22);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 23);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 24);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 25);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 26);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 27);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 28);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 29);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 30);
	OSMetaClassDeclareReservedUnused(IORegistryEntry, 31);

public:

                        

                             
                                                                      
                                                                                                                                                                                                                                                                                                                    
                                                                                                        

	static IORegistryEntry * getRegistryRoot( void );

                                
                                                                                
                                                                                                                                                                                                                                                                                                               
                                            

	static SInt32            getGenerationCount( void );

                      
                                                            
                                                                                                                                                                                               
                                                                                                                          

	static const IORegistryPlane * getPlane( const char * name );

                                      

                  
                                                                       
                                                                                                                                                                                                                                                   
                                                                                                                                                                        
                                                                

	virtual bool init( OSDictionary * dictionary = NULL );

                  
                                                                       
                                                                                                                                                                                                                                                                                                                                                       

	virtual void free( void ) APPLE_KEXT_OVERRIDE;

                              
                                                         
                                                                                                                                                  
                                                                                

	virtual void setPropertyTable( OSDictionary * dict );

                                                            
                                    

                         
                                                                                          
                                                                                                                                                                                                                                     
                                                    
                                        
                                                               

	virtual bool setProperty(const OSSymbol * aKey,
	    OSObject * anObject);

#ifdef KERNEL_PRIVATE
	OSObject * setIndexedProperty(uint32_t index, OSObject * anObject);
	OSObject * getIndexedProperty(uint32_t index) const;
#endif                     

                         
                                                                                          
                                                                                                                                                                                                                                     
                                                    
                                        
                                                               

	virtual bool setProperty(const OSString * aKey, OSObject * anObject);

                         
                                                                                          
                                                                                                                                                                                                                                     
                                                   
                                        
                                                               

	virtual bool setProperty(const char * aKey, OSObject * anObject);

                         
                                                                                                                 
                                                                                                                                                                                                                                                                                                                                                                     
                                                   
                                                     
                                                               

	virtual bool setProperty(const char * aKey, const char * aString);

                         
                                                                                                                   
                                                                                                                                                                                                                                                                                                                                                                   
                                                   
                                                  
                                                               

	virtual bool setProperty(const char * aKey, bool aBoolean);

                         
                                                                                                                  
                                                                                                                                                                                                                                                                                                                                                                           
                                                   
                                                
                                                                    
                                                               

	virtual bool setProperty( const char *       aKey,
	    unsigned long long aValue,
	    unsigned int       aNumberOfBits);

                         
                                                                                                                
                                                                                                                                                                                                                                                                                                                                                                                 
                                                   
                                                                                
                                                            
                                                               

	virtual bool setProperty( const char *       aKey,
	    void *             bytes,
	    unsigned int       length);

                            
                                                                                               
                                                                                                                                                                                                                                  
                                                       

	virtual void removeProperty( const OSSymbol * aKey);

                            
                                                                                               
                                                                                                                                                                                                                                  
                                                       

	virtual void removeProperty( const OSString * aKey);

                            
                                                                                               
                                                                                                                                                                                                                                  
                                                      

	virtual void removeProperty( const char * aKey);

                         
                                                                                               
                                                                                                                                                                                                                             
                                                    
                                                  

	virtual OSObject * getProperty( const OSSymbol * aKey) const APPLE_KEXT_DEPRECATED_WITH_SHARED_PTR;

                         
                                                                                               
                                                                                                                                                                                                                             
                                                    
                                                  

	virtual OSObject * getProperty( const OSString * aKey) const APPLE_KEXT_DEPRECATED_WITH_SHARED_PTR;

                         
                                                                                               
                                                                                                                                                                                                                             
                                                   
                                                  

	virtual OSObject * getProperty( const char * aKey) const APPLE_KEXT_DEPRECATED_WITH_SHARED_PTR;

                            
                                                                                                      
                                                                                                                                                                                         
                                                    
                                                 

	bool propertyExists(const OSSymbol * aKey);

                            
                                                                                                      
                                                                                                                                                                                         
                                                    
                                                 

	bool propertyExists(const OSString * aKey);

                            
                                                                                                      
                                                                                                                                                                                         
                                                   
                                                 

	bool propertyExists(const char * aKey);

                              
                                                                                                                 
                                                                                                                                                                                                                                          
                                                    
                                                      
                                                                                        

	bool propertyHasValue(const OSSymbol * aKey,
	    const OSObject * value);

                              
                                                                                                                 
                                                                                                                                                                                                                                          
                                                    
                                                      
                                                                                        

	bool propertyHasValue(const OSString * aKey,
	    const OSObject * value);

                              
                                                                                                                 
                                                                                                                                                                                                                                          
                                                   
                                                      
                                                                                        

	bool propertyHasValue(const char * aKey,
	    const OSObject * value);

                         
                                                                                                                                   
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                    
                                                                 
                                                                                                                                                                                                                                                                                                         
                                                  

	virtual OSObject * getProperty( const OSSymbol *        aKey,
	    const IORegistryPlane * plane,
	    IOOptionBits            options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const APPLE_KEXT_DEPRECATED_WITH_SHARED_PTR;

                         
                                                                                                                                   
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                    
                                                                 
                                                                                                                                                                                                                                                                                                         
                                                  

	virtual OSObject * getProperty( const OSString *        aKey,
	    const IORegistryPlane * plane,
	    IOOptionBits            options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const APPLE_KEXT_DEPRECATED_WITH_SHARED_PTR;

                         
                                                                                                                                   
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                                                   
                                                                 
                                                                                                                                                                                                                                                                                                         
                                                  

	virtual OSObject * getProperty( const char *            aKey,
	    const IORegistryPlane * plane,
	    IOOptionBits            options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const APPLE_KEXT_DEPRECATED_WITH_SHARED_PTR;

                            
                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                    
                                                                 
                                                                                                                                                                                                                                                                                                            
                                               

	bool propertyExists( const OSSymbol *        aKey,
	    const IORegistryPlane * plane,
	    uint32_t                options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const;

                            
                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                    
                                                                 
                                                                                                                                                                                                                                                                                                            
                                               

	bool propertyExists( const OSString *        aKey,
	    const IORegistryPlane * plane,
	    uint32_t                options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const;

                            
                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                   
                                                                 
                                                                                                                                                                                                                                                                                                            
                                               

	bool propertyExists( const char *            aKey,
	    const IORegistryPlane * plane,
	    uint32_t                options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const;

                              
                                                                                                                                                       
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
                                                    
                                                    
                                                                 
                                                                                                                                                                                                                                                                                                            
                                                                                  

	bool propertyHasValue( const OSSymbol *        aKey,
	    const OSObject        * value,
	    const IORegistryPlane * plane,
	    uint32_t                options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const;

                              
                                                                                                                                                       
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
                                                    
                                                    
                                                                 
                                                                                                                                                                                                                                                                                                            
                                                                                  

	bool propertyHasValue( const OSString *        aKey,
	    const OSObject        * value,
	    const IORegistryPlane * plane,
	    uint32_t                options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const;

                              
                                                                                                                                                       
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
                                                   
                                                    
                                                                 
                                                                                                                                                                                                                                                                                                            
                                                                                  

	bool propertyHasValue( const char *            aKey,
	    const OSObject        * value,
	    const IORegistryPlane * plane,
	    uint32_t                options =
	    kIORegistryIterateRecursively |
	    kIORegistryIterateParents) const;


                          
                                                                                               
                                                                                                                                                                                                                                                                   
                                                    
                                                                                       

	virtual OSPtr<OSObject> copyProperty( const OSSymbol * aKey) const;

                          
                                                                                               
                                                                                                                                                                                                                                                                   
                                                    
                                                                                       

	virtual OSPtr<OSObject> copyProperty( const OSString * aKey) const;

                          
                                                                                               
                                                                                                                                                                                                                                                                   
                                                   
                                                                                       

	virtual OSPtr<OSObject> copyProperty( const char * aKey) const;

                                      
                                                                                    
                                                                                                                                                                                                                                                                                                                                              
                                                                                                         

	virtual OSPtr<OSDictionary> dictionaryWithProperties( void ) const;

                                 
                                                                                  
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                                                                      
                                                 

	virtual bool serializeProperties( OSSerialize * serialize ) const;

                                             

                              
                                                                            
                                                                                                                                                                                                                                                                                                                                                                                        
                                                                                                                                                                        

             OSDictionary * getPropertyTable( void ) const;
                                 

                                                                   

                           
                                                                                          
                                                                                                                                                                                                                                                                                                                                                                        
                                                                                                                                                             
                                                              

	virtual IOReturn setProperties( OSObject * properties );

              

                               
                                                                                                
                                   
                                                                                                                                                                                                                                                                                                                             

	virtual OSPtr<OSIterator> getParentIterator( const IORegistryPlane * plane )
	const;
	virtual void applyToParents( IORegistryEntryApplierFunction applier,
	    void * context,
	    const IORegistryPlane * plane ) const;

                            
                                                                         
                                                                                                                                                                                                  
                                   
                                                                                                                                                                                                                                   

	virtual IORegistryEntry * getParentEntry( const IORegistryPlane * plane ) const;

                              
                                                                                     
                                                                                                                                 
                                   
                                                                                                                                                                                                                                                                                                                     

	virtual OSPtr<OSIterator> getChildIterator( const IORegistryPlane * plane )
	const;

#if XNU_KERNEL_PRIVATE
	uint32_t getChildCount( const IORegistryPlane * plane ) const;
	OSPtr<OSArray> copyPropertyKeys(void) const;
#endif

	virtual void applyToChildren( IORegistryEntryApplierFunction applier,
	    void * context,
	    const IORegistryPlane * plane ) const;

                           
                                                                        
                                                                                              
                                   
                                                                                                                                                                                                                                 

	virtual IORegistryEntry * getChildEntry( const IORegistryPlane * plane ) const;

                     
                                                                                      
                                                                                                                                                                                                
                                                    
                                   
                                                                         
                                                                                                                                                                                                                    

	virtual bool isChild( IORegistryEntry * child,
	    const IORegistryPlane * plane,
	    bool onlyChild = false ) const;

                      
                                                                                       
                                                                                                                                                                                                 
                                                      
                                   
                                                                            
                                                                                                                                                                                                                         

	virtual bool isParent( IORegistryEntry * parent,
	    const IORegistryPlane * plane,
	    bool onlyParent = false ) const;

                     
                                                                          
                                                                                                                                                                        
                                                          
                                                                                                                                        

	virtual bool inPlane( const IORegistryPlane * plane = NULL) const;

                      
                                                                                                       
                                                                                                                                                                          
                                   
                                                                                                                                         

	virtual unsigned int getDepth( const IORegistryPlane * plane ) const;

                     

                            
                                                             
                                                                                                                                                                                                                                                                                                                                                                   
                                                   
                                   
                                                                                                           

	virtual bool attachToParent( IORegistryEntry * parent,
	    const IORegistryPlane * plane );

                              
                                                                
                                                                                                                                                                                                                                                                                                                 
                                                     
                                      

	virtual void detachFromParent( IORegistryEntry * parent,
	    const IORegistryPlane * plane );

                           
                                                                       
                                                                                                                                                                                                                                                                                                                                                                                      
                                                    
                                   
                                                                                                           

	virtual bool attachToChild( IORegistryEntry * child,
	    const IORegistryPlane * plane );

                             
                                                                 
                                                                                                                                                                                                                                                                                                                                             
                                               
                                      

	virtual void detachFromChild( IORegistryEntry * child,
	    const IORegistryPlane * plane );

                         
                                                                        
                                                                                                             
                                      

	virtual void detachAbove( const IORegistryPlane * plane );

                       
                                                                             
                                                                                                                                                                      
                                      

	virtual void detachAll( const IORegistryPlane * plane );

                                       

                     
                                                                             
                                                                                                                                                                                                                                                                                        
                                                                
                                                                   

	virtual const char * getName( const IORegistryPlane * plane = NULL ) const;

                      
                                                                              
                                                                                                                                                                                                                                                                                        
                                                                
                                                                                                

	virtual OSPtr<const OSSymbol> copyName(
		const IORegistryPlane * plane = NULL ) const;

                          
                                                                                                               
                                                                                                                                                                                                                                                                                                                                                                                
                                                                                                                                                                   
                                                                                                                                                                                                             
                                                                                    

	virtual bool compareNames( OSObject * name, OSString ** matched = NULL ) const;

	bool compareNames( OSObject * name, OSSharedPtr<OSString>& matched) const;

                         
                                                                                                      
                                                                                                                                                                                
                                                         
                                                                                                                                                                                                                                                                                                           
                                                                            

	virtual bool compareName( OSString * name, OSString ** matched = NULL ) const;

	bool compareName( OSString * name, OSSharedPtr<OSString>& matched) const;

                     
                                                                                      
                                                                                                                                                                                                                                                            
                                                    
                                                                      

	virtual void setName( const OSSymbol * name,
	    const IORegistryPlane * plane = NULL );

                     
                                                                                      
                                                                                                                                                                                                                                                            
                                                            
                                                                      

	virtual void setName( const char * name,
	    const IORegistryPlane * plane = NULL );

                         
                                                                                        
                                                                                                                                                                                                                                                                                                                        
                                                                
                                                                                       

	virtual const char * getLocation( const IORegistryPlane * plane = NULL ) const;

                          
                                                                                         
                                                                                                                                                                                                                                                                                                                        
                                                                
                                                                                                                           

	virtual OSPtr<const OSSymbol> copyLocation(
		const IORegistryPlane * plane = NULL ) const;

                         
                                                                                                 
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                                                                            
                                                                                 

	virtual void setLocation( const OSSymbol * location,
	    const IORegistryPlane * plane = NULL );
	virtual void setLocation( const char * location,
	    const IORegistryPlane * plane = NULL );

                     
                                                  
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
                                                       
                                                                                                                                                                   
                                   
                                                                                                                                      

	virtual bool getPath( char * path, int * length,
	    const IORegistryPlane * plane) const;

                              
                                                            
                                                                                                                                                                                                                                                                      
                                                       
                                                                                                                                                                                      
                                   
                                                                                      

	virtual bool getPathComponent( char * path, int * length,
	    const IORegistryPlane * plane ) const;

                      
                                                 
                                                                                                                                                                                                                                                                                                                                                                                    
                                 
                                                                                                                  
                                                                                                                                                                                                                                                                   
                                                                                                                                                                                                                                                                                     
                                                                                                                                 
                                                                                                                          

	static OSPtr<IORegistryEntry> fromPath(  const char * path,
	    const IORegistryPlane * plane = NULL,
	    char * residualPath = NULL,
	    int * residualLength = NULL,
	    IORegistryEntry * fromEntry = NULL );

                      
                                                          
                                                                                                                                                                                              
                                               
                                                
                                                       
                                                         
                                              

	virtual OSPtr<IORegistryEntry> childFromPath( const char * path,
	    const IORegistryPlane * plane = NULL,
	    char * residualPath = NULL,
	    int * residualLength = NULL );

                         
                                                                                  
                                                                                                                                                                                                                                                                                                
                                                                                                                                                                                                          
                                                   
                                                                                             

	static const char * dealiasPath( const char ** opath,
	    const IORegistryPlane * plane );

                       
                                                    
                                                                                                  
                                                                 
                                                                         

	static OSPtr<const IORegistryPlane> makePlane( const char * name );

                                                                                  
                                                                                                                                                                                                                                                                                                                                                                                                                             
                                                                                                        

	uint64_t getRegistryEntryID( void );

                                                           
                                                           
                                                           

	virtual bool init( IORegistryEntry * from,
	    const IORegistryPlane * inPlane );

#ifdef XNU_KERNEL_PRIVATE
public:
#else
private:
#endif
	static LIBKERN_RETURNS_NOT_RETAINED IORegistryEntry * initialize( void );

#ifdef XNU_KERNEL_PRIVATE
	SInt32 getRegistryEntryGenerationCount( void ) const;
	void setName(const OSString * name,
	    const IORegistryPlane * plane = NULL);
#endif

private:
	inline bool arrayMember( OSArray * set,
	    const IORegistryEntry * member,
	    unsigned int * index = NULL ) const;

	bool makeLink( IORegistryEntry * to,
	    unsigned int relation,
	    const IORegistryPlane * plane ) const;
	void breakLink( IORegistryEntry * to,
	    unsigned int relation,
	    const IORegistryPlane * plane ) const;

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	OSArray * getParentSetReference( const IORegistryPlane * plane )
	const;

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	OSArray * getChildSetReference( const IORegistryPlane * plane )
	const;

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	IORegistryEntry * getChildFromComponent( const char ** path,
	    const IORegistryPlane * plane );

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	LIBKERN_RETURNS_NOT_RETAINED const OSSymbol * hasAlias(
		const IORegistryPlane * plane,
		char * opath = NULL, int * length = NULL ) const;

	APPLE_KEXT_COMPATIBILITY_VIRTUAL
	const char * matchPathLocation( const char * cmp,
	    const IORegistryPlane * plane );
};

                                                                           

                                                 
                                             
                                                                                                                                                                                                                                                      

class IORegistryIterator : public OSIterator
{
	OSDeclareAbstractStructors(IORegistryIterator);

private:
	struct IORegCursor {
		IORegCursor          *  next;
		IORegistryEntry      *  current;
		OSIterator           *  iter;
	};
	IORegCursor                 start;
	IORegCursor           *     where;
	IORegistryEntry       *     root;
	OSOrderedSet          *     done;
	const IORegistryPlane *     plane;
	IOOptionBits                options;

	virtual void free( void ) APPLE_KEXT_OVERRIDE;

public:
                         
                                                                   
                                                                                                                                                                                                                                                                                                                                  
                                                           
                                                   
                                                                                                                                                                                                                                                                                                                                                                                                   
                                                                                                                 

	static OSPtr<IORegistryIterator> iterateOver( IORegistryEntry * start,
	    const IORegistryPlane * plane,
	    IOOptionBits options = 0 );

                         
                                                              
                                                                                                                                                                                                                                                                                                                                  
                                                   
                                                                                                                                                                                                                                                                                                                                                                                                   
                                                                                                                 

	static OSPtr<IORegistryIterator> iterateOver( const IORegistryPlane * plane,
	    IOOptionBits options = 0 );

                           
                                                                
                                                                                                                                                                                                                                                                                                                                                                
                                                                                                                                                                                                                                                                                                    

	virtual IORegistryEntry * getNextObject( void ) APPLE_KEXT_OVERRIDE;

                               
                                                                                                                   
                                                                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                                                                                                                                                               

	virtual IORegistryEntry * getNextObjectFlat( void );

                                    
                                                                              
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
                                                                                                                                                                                                                                                                                                      

	virtual IORegistryEntry * getNextObjectRecursive( void );

                             
                                                                  
                                                                                                                                                                                                                                                                                                                                 
                                                                                                                                                                                                                                                                                                         

	virtual IORegistryEntry * getCurrentEntry( void );

                        
                                                                        
                                                                                                                                                   

	virtual void enterEntry( void );

                        
                                                                        
                                                                                                                                                
                                                  

	virtual void enterEntry( const IORegistryPlane * plane );

                       
                                                                       
                                                                                                                                                                               
                                                                                                                

	virtual bool exitEntry( void );

                   
                                                                                              
                                                                                                                  

	virtual void reset( void ) APPLE_KEXT_OVERRIDE;

                     
                                                                              
                                                                                                                                                                                                                                                                                           
                                                                                                      

	virtual bool isValid( void ) APPLE_KEXT_OVERRIDE;

                        
                                                                                                   
                                                                                                        
                                                                                                                                                                   

	virtual OSPtr<OSOrderedSet> iterateAll( void );
};

#endif                               
