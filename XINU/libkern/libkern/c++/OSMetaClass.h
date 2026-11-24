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
#ifndef _LIBKERN_OSMETACLASS_H
#define _LIBKERN_OSMETACLASS_H

#include <sys/types.h>

#include <libkern/OSReturn.h>
#include <kern/debug.h>
#include <ptrauth.h>
#ifdef KERNEL_PRIVATE
#include <kern/zalloc.h>
#include <kern/kalloc.h>
#endif                     

  
                                                                        
                                                
                                                                   
                                                                      
                                               
   

  
                                                                            
                      
                                                                           
                                                       
                                                                           
                    
  
                                                                        
                                                  
  
                                                                         
                                                                                   
                                    
                                                                             
                                
  
                                                                        
                                                                     
                                                                   
                                                             
                                                                          
                                                           
                                                                
                       
                                               
                                                                             
   
#if __has_attribute(os_returns_retained)
#define LIBKERN_RETURNS_RETAINED __attribute__((os_returns_retained))
#else
#define LIBKERN_RETURNS_RETAINED
#endif
#if __has_attribute(os_returns_not_retained)
#define LIBKERN_RETURNS_NOT_RETAINED __attribute__((os_returns_not_retained))
#else
#define LIBKERN_RETURNS_NOT_RETAINED
#endif

  
                                                           
                                                                          
                       
   
#if __has_attribute(os_consumed)
#define LIBKERN_CONSUMED __attribute__((os_consumed))
#else
#define LIBKERN_CONSUMED
#endif

  
                                                             
                                                                          
                                                           
   
#if __has_attribute(os_consumes_this)
#define LIBKERN_CONSUMES_THIS __attribute__((os_consumes_this))
#else
#define LIBKERN_CONSUMES_THIS
#endif

  
                                                                     
              
                                                                           
                                            
   
#if __has_attribute(os_returns_retained_on_zero)
#define LIBKERN_RETURNS_RETAINED_ON_ZERO __attribute__((os_returns_retained_on_zero))
#else
#define LIBKERN_RETURNS_RETAINED_ON_ZERO
#endif

  
                                                                         
              
                                                                           
                                                
   
#if __has_attribute(os_returns_retained_on_non_zero)
#define LIBKERN_RETURNS_RETAINED_ON_NONZERO __attribute__((os_returns_retained_on_non_zero))
#else
#define LIBKERN_RETURNS_RETAINED_ON_NONZERO
#endif

class OSMetaClass;
class OSObject;
class OSString;
class OSSymbol;
class OSDictionary;
class OSSerialize;
#ifdef XNU_KERNEL_PRIVATE
class OSOrderedSet;
class OSCollection;
class OSKext;
#endif                         
struct IORPC;
class OSInterface
{
};

   
          
  
            
                                                                    
                                                                               
                                          
   


                 
#define APPLE_KEXT_COMPATIBILITY

#ifdef XNU_KERNEL_PRIVATE

#if !XNU_TARGET_OS_OSX
#define APPLE_KEXT_VTABLE_PADDING   0
#else                         
#define APPLE_KEXT_VTABLE_PADDING   1
#endif                         

#else                         

                                           

#include <TargetConditionals.h>
#if TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
#define APPLE_KEXT_VTABLE_PADDING   0
#else                                               
#define APPLE_KEXT_VTABLE_PADDING   1
#endif                                               

#endif                         

#ifdef XNU_KERNEL_PRIVATE
#if XNU_TARGET_OS_OSX && defined(__arm64__)
#define APPLE_KEXT_ALIGN_CONTAINERS      1
#else                                              
#define APPLE_KEXT_ALIGN_CONTAINERS     (0 == APPLE_KEXT_VTABLE_PADDING)
#endif                                              

#else                         

#if TARGET_OS_OSX && defined(__arm64__)
#define APPLE_KEXT_ALIGN_CONTAINERS      1
#else                                          
#define APPLE_KEXT_ALIGN_CONTAINERS     (0 == APPLE_KEXT_VTABLE_PADDING)
#endif                                          

#endif                         

#if defined(__LP64__)
                 
#define APPLE_KEXT_LEGACY_ABI  0
#elif defined(__arm__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
#define APPLE_KEXT_LEGACY_ABI  0
#else
#define APPLE_KEXT_LEGACY_ABI  1
#endif

#if defined(__LP64__)
                 
#define APPLE_KEXT_COMPATIBILITY_VIRTUAL
#else
                                                            
#define APPLE_KEXT_COMPATIBILITY_VIRTUAL  virtual
#endif

                 
#define APPLE_KEXT_DEPRECATED  __attribute__((deprecated))


  
                                                                                      
   
#if !defined(BUILD_FOR_USERSPACE) && (__cplusplus >= 201103L)
#define APPLE_KEXT_OVERRIDE                             override
#if defined(__LP64__)
#define APPLE_KEXT_COMPATIBILITY_OVERRIDE
#else
#define APPLE_KEXT_COMPATIBILITY_OVERRIDE       APPLE_KEXT_OVERRIDE
#endif
#else
#define APPLE_KEXT_OVERRIDE
#define APPLE_KEXT_COMPATIBILITY_OVERRIDE
#endif

#define APPLE_KEXT_WSHADOW_PUSH _Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wunknown-warning-option\"") \
	_Pragma("clang diagnostic ignored \"-Wshadow-field\"")

#define APPLE_KEXT_WSHADOW_POP _Pragma("clang diagnostic pop")


   
                         
  
            
                                                  
                                                                
  
              
                                                 
                                                             
                                                   
                                                            
                                                           
                                     
                                                 
                                                          
                                                             
                                                     
  
                            
                                          
                                                     
  
                                  
  
                                                                         
                                                       
                                                         
       
                                                                                          
                                                                                    
                                                                 
                                               
                                                                           
                                                          
                                         
        
  
                                                                
                                                 
  
                          
  
                                                                 
                                                                          
  
                                                             
                                           
  
                                
  
                                                                             
   

class OSMetaClassBase
{
public:


   
                      
                
  
            
                                                   
  
                                                              
                                                               
  
          
                                                         
                                
  
              
           
              
                                                                                   
                                                             
      
              
                                                               
                                   
  
                                                                           
                                                  
   
#define OSTypeAlloc(type)   ((type *) ((type::metaClass)->alloc()))


   
                   
                
  
            
                                                                
  
                                                                
                                               
  
          
                                                
  
              
                                                                      
                    
                                                                             
      
                                                                                  
   
#define OSTypeID(type)   (type::metaClass)
#define OSMTypeID(type)  (const_cast<OSMetaClass *>(type::metaClass))


   
                       
                
  
            
                                                                       
  
                                                       
  
          
                                                              
  
              
                                                                        
                               
                                                                             
      
                                                                                  
   
#define OSTypeIDInst(typeinst)   ((typeinst)->getMetaClass())


   
                        
                
  
            
                                             
  
                                                                     
                                               
                                                               
                                            
                                                              
                                                   
                                                    
                                                              
                                           
  
          
                                                   
                                      
                               
  
              
                                                   
                                                                        
                                                           
                                 
                                          
                                                    
                                      
                                                      
                                                                   
   
#define OSDynamicCast(type, inst)   \
    ((type *) OSMetaClassBase::safeMetaCast((inst), OSTypeID(type)))

   
                         
                
  
            
                                                                
                                                                            
  
                                                                   
                                     
  
              
                                                         
                                                        
                                                     
                                             
                                                                             
                         
   
#define OSRequiredCast(type, inst)  \
    (type *) OSMetaClassBase::requiredMetaCast((inst), OSTypeID(type))

   
                          
                
  
            
                                                  
  
                                        
                                                              
  
          
                                                  
                                                  
                                                                            
                                
   
#define OSCheckTypeInst(typeinst, inst) \
    OSMetaClassBase::checkTypeInst(inst, typeinst)

#define OSSafeRelease(inst) \
  do { int OSSafeRelease __attribute__ ((deprecated("Use OSSafeReleaseNULL"))); (OSSafeRelease); \
	if (inst) (inst)->release(); } while (0)

                               
                                                                                           
                                                                      
   
#define OSSafeReleaseNULL(inst)   do { if (inst != NULL) (inst)->release(); (inst) = NULL; } while (0)

	typedef void (*_ptf_t)(void);

#if defined(__arm__) || defined(__arm64__)

	static _ptf_t _ptmf2ptf(const OSMetaClassBase * self, void (OSMetaClassBase::*func)(void));

#elif defined(__i386__) || defined(__x86_64__)

                                                         
                                                            
      

	static inline _ptf_t
	_ptmf2ptf(const OSMetaClassBase *self, void (OSMetaClassBase::*func)(void))
	{
		union {
			void (OSMetaClassBase::*fIn)(void);
			uintptr_t fVTOffset;
			_ptf_t fPFN;
		} map;

		map.fIn = func;

		if (map.fVTOffset & 1) {
			          
			union {
				const OSMetaClassBase *fObj;
				_ptf_t **vtablep;
			} u;
			u.fObj = self;

			                                                
			return *(_ptf_t *)(((uintptr_t)*u.vtablep) + map.fVTOffset - 1);
		} else {
			                                      
			return map.fPFN;
		}
	}

#else
#error Unknown architecture.
#endif              


   
                               
                
  
            
                                                                   
                                    
  
                                                           
                                                                                         
                                                                             
                                     
                                                             
                                                                
                                                                             
                                          
                                                                          
                                                                              
  
          
                                                                           
  
              
                                                                             
                                                                      
  
                             
  
                                                            
                                    
   
#if __has_builtin(__builtin_load_member_function_pointer)
#define OSMemberFunctionCast(cptrtype, self, func)                       \
     ((cptrtype) __builtin_load_member_function_pointer(*self, func) ? : \
      (cptrtype) OSMetaClassBase::                                       \
	  _ptmf2ptf(self, (void (OSMetaClassBase::*)(void)) func))
#else
#define OSMemberFunctionCast(cptrtype, self, func)         \
    (cptrtype) OSMetaClassBase::                           \
	_ptmf2ptf(self, (void (OSMetaClassBase::*)(void)) func)
#endif

protected:
	OSMetaClassBase();
	virtual
	~OSMetaClassBase();

private:
                                                             
                                   
  
                       
  
            
                                                      
  
                                                                         
   
	void operator =(OSMetaClassBase &src);

                                   
  
                            
  
            
                                                      
  
                                                                         
   
	OSMetaClassBase(OSMetaClassBase &src);

public:

                                                                              
                                                

   
                    
  
            
                          
              
                                                           
                                      
  
              
      
              
                                                           
                                      
   
	virtual void release(int freeWhen) const = 0;


   
                           
  
            
                          
              
                                                              
                                 
  
              
      
              
                                                              
                                           
   
	virtual int getRetainCount() const = 0;


   
                   
  
            
                          
              
                                                       
                         
  
              
      
              
                                                       
                                   
   
	virtual void retain() const = 0;


   
                    
  
            
                          
              
                                                        
                        
  
              
      
              
                                                        
                                  
   
	virtual void release() const = 0;


   
                      
  
            
                          
              
                                                                      
                          
  
              
      
              
                                                                      
                                    
   
	virtual bool serialize(OSSerialize * serializer) const = 0;


   
                         
  
            
                                       
                               
  
              
                                                   
                                                   
                                  
   
	virtual const OSMetaClass * getMetaClass() const = 0;


   
                      
  
            
                                                          
  
                                                         
  
          
                                                                            
  
              
                                                                  
                                                          
                                              
                                      
                                                
                                             
                                                             
   
	virtual bool isEqualTo(const OSMetaClassBase * anObject) const;


   
                     
  
            
                                                                      
  
                                                    
                                             
  
          
                                             
                                                 
                               
  
              
                                   
                                                        
   
	OSMetaClassBase * metaCast(const OSMetaClass * toMeta) const;


   
                     
  
            
                                                                      
  
                                                            
  
          
                                             
                                               
                               
  
              
                                   
                                                        
   
	OSMetaClassBase * metaCast(const OSSymbol * toMeta) const;


   
                     
  
            
                                                                      
  
                                                            
          
                                             
                                               
                               
  
              
                                   
                                                        
   
	OSMetaClassBase * metaCast(const OSString * toMeta) const;


   
                     
  
            
                                                                      
  
                                                           
          
                                             
                                               
                               
  
              
                                   
                                                        
   
	OSMetaClassBase * metaCast(const char * toMeta) const;

                                                       
   
                         
  
            
                                                                    
  
                                                      
                                                      
                                               
  
          
                                                 
                                                 
                               
  
              
                                   
                                                        
   
	static OSMetaClassBase * safeMetaCast(
		const OSMetaClassBase * anObject,
		const OSMetaClass     * toMeta);

   
                             
  
            
                                                                   
                                                          
  
                                                      
                                                      
                                               
  
          
                                                 
                                                 
                                                                    
                          
  
              
                                   
                                                          
   
	static OSMetaClassBase *requiredMetaCast(
		const OSMetaClassBase * anObject,
		const OSMetaClass     * toMeta);

   
                          
  
            
                                                         
                                                            
  
                                                      
                                                                       
  
          
                                             
                                          
                               
                                
  
              
                                   
                                                            
   
	static bool checkTypeInst(
		const OSMetaClassBase * inst,
		const OSMetaClassBase * typeinst);

	static void initialize(void);

public:

   
                         
  
            
                          
              
                                                                       
                                           
  
              
      
              
                                                                       
                                                     
   
                                                 
	virtual void taggedRetain(const void * tag = NULL) const = 0;


   
                          
  
            
                          
              
                                                                        
                                            
  
              
      
              
                                                                        
                                                      
   
                                                  
	virtual void taggedRelease(const void * tag = NULL) const = 0;

protected:
   
                          
  
            
                          
              
                                                                                 
                                                                
  
              
      
              
                                                                                 
                                                                          
   
                                                  
	virtual void taggedRelease(
		const void * tag,
		const int    freeWhen) const = 0;

public:
	virtual kern_return_t
	Dispatch(const IORPC rpc);

	kern_return_t
	Invoke(const IORPC rpc);

private:
#if APPLE_KEXT_VTABLE_PADDING
                  
#if defined(__arm64__) || defined(__arm__)
	virtual void _RESERVEDOSMetaClassBase0();
	virtual void _RESERVEDOSMetaClassBase1();
	virtual void _RESERVEDOSMetaClassBase2();
	virtual void _RESERVEDOSMetaClassBase3();
#endif                                             
	virtual void _RESERVEDOSMetaClassBase4();
	virtual void _RESERVEDOSMetaClassBase5();
	virtual void _RESERVEDOSMetaClassBase6();
	virtual void _RESERVEDOSMetaClassBase7();
#endif                                
} APPLE_KEXT_COMPATIBILITY;


#ifdef XNU_KERNEL_PRIVATE
typedef bool (*OSMetaClassInstanceApplierFunction)(const OSObject * instance,
    void * context);
#endif                         

   
                     
  
            
                                                
                                       
  
                                                            
                                       
                                                                       
                                                                    
                             
                                                          
                                                
                                              
  
                                  
  
                                                                                
                                                             
                                  
                                                           
                                                
       
                                                                                     
                                                                     
                                                                                       
                                                                              
                                                                                 
                                                                       
        
  
                                                                
       
                                                
                                                         
                                                                     
                                                        
                                                                 
                                                                              
                                                     
                                                              
                                                                       
        
  
                                                                      
                                                                    
                                                                
       
                                                   
                                                            
                                  
                                                  
                                                           
                                                                         
                                                 
                                                          
                                                               
                                                
                                                         
                                                              
        
  
                          
  
                                                                       
                                       
                                                                          
  
                                             
                                                        
  
                                
  
                                                   
                                     
                                          
                                                      
                                           
                                                    
   
class OSMetaClass : public OSMetaClassBase
{
	friend class OSKext;
#if IOKITSTATS
	friend class IOStatistics;
#endif

private:
                                                         
	static void * operator new(size_t size);

                                                   
	struct ExpansionData *reserved;

                                                       
	const OSMetaClass *superClassLink;

                                            
	const OSSymbol *className;

                                                           
	unsigned int classSize;

                                                           
                                                       
                                        
   
	mutable unsigned int instanceCount;

                                   
  
                        
  
            
                                   
   
	OSMetaClass();

                        
                                   
  
                     
  
            
                                                          
                             
  
                                                                             
  
              
                                                                 
                                                   
   
	static void logError(OSReturn result);

public:

   
                                 
  
            
                                                               
  
                                                         
  
          
                                                                           
   
	static const OSMetaClass * getMetaClassWithName(const OSSymbol * name);

#if XNU_KERNEL_PRIVATE

   
                                  
  
            
                                                               
  
                                                         
  
          
                                                                           
                                                                          
             
   
	static const OSMetaClass * copyMetaClassWithName(const OSSymbol * name);
   
                             
  
            
                                                            
  
              
                                                                          
             
   
	void releaseMetaClass() const;

#endif                         

protected:
   
                   
  
            
                                                                      
  
              
                                                         
                                                 
                                               
   
	virtual void retain() const;


   
                    
  
            
                                                                       
  
              
                                                         
                                                 
                                               
   
	virtual void release() const;


   
                    
  
            
                                                             
                          
  
                           
  
              
                                                         
                                                 
                                               
   
	virtual void release(int freeWhen) const;


   
                         
  
            
                                                                  
                          
  
                      
  
              
                                                         
                                                 
                                               
   
	virtual void taggedRetain(const void * tag = NULL) const;


   
                          
  
            
                                                                   
                          
  
                      
  
              
                                                         
                                                 
                                               
   
	virtual void taggedRelease(const void * tag = NULL) const;


   
                          
  
            
                                                                             
                          
  
                           
                           
  
              
                                                         
                                                 
                                               
   
	virtual void taggedRelease(
		const void * tag,
		const int    freeWhen) const;


   
                           
  
            
                                                      
                        
  
          
                    
  
              
                                                         
                                                 
                                               
   
	virtual int getRetainCount() const;


                                   
  
                         
  
            
                              
  
          
                                           
   
	virtual const OSMetaClass * getMetaClass() const;


   
                        
  
            
                                       
  
                                                    
                                                      
                                                                       
                                               
                                                                      
  
              
                                                   
                                                                            
                                              
                                         
                                                         
                                                    
   
	OSMetaClass(const char * className,
	    const OSMetaClass  * superclass,
	    unsigned int         classSize);

#ifdef KERNEL_PRIVATE
   
                        
  
            
                                       
  
                                                    
                                                      
                                                                       
                                               
                                                                      
                                                        
                                           
                                        
  
              
                                                   
                                                                            
                                              
                                         
                                                         
                                                    
   
	OSMetaClass(const char * className,
	    const OSMetaClass  * superclass,
	    unsigned int         classSize,
	    zone_t             * zone,
	    const char         * zone_name,
	    zone_create_flags_t  zflags);
#endif

   
                         
  
            
                                      
  
              
                                                                    
                                     
                                                     
                                             
   
	virtual
	~OSMetaClass();

                                                                         
                                                                 
	void
	operator delete(void *, size_t)
	{
	}

public:
	static const OSMetaClass * const metaClass;

   
                       
  
            
                                    
                                      
                                                 
  
                                                         
  
          
                                       
                                       
                                
  
              
                                           
  
                                                                       
                                                                        
                                                    
                                                                        
                                                    
                                                    
                                                               
                                               
   
	static void * preModLoad(const char * kextID);


   
                         
  
            
                                                              
  
                                               
                                                                       
          
                                                 
                                                       
  
              
                                           
   
	static bool checkModLoad(void * loadHandle);


   
                        
  
            
                                                                          
  
                                               
                                                                       
          
                                                 
     
              
                                         
                                
                        
  
              
                                           
  
                                                             
                            
                                                  
                                                       
                                                          
                                            
                                          
                                                      
                                                  
   
	static OSReturn postModLoad(void * loadHandle);

   
                           
  
            
                                                   
                                                                  
  
                                                                  
  
          
                                             
                                    
                                
                                
  
              
                                                                         
                                                 
                                                                   
                           
                                                                             
                                           
   
	static bool modHasInstance(const char * kextID);


   
                               
  
            
                                       
                                 
  
                                                                      
  
              
                                                     
                                              
                                     
                                                                        
                                                         
   
	static void reportModInstances(const char * kextID);


   
                            
  
            
                                                            
  
              
                                                        
                                      
                                           
                                                             
                                           
  
                                                           
  
                                              
                                                        
                                        
  
                                                         
                                                      
                                        
                                                        
                                                                   
                                                                    
                                                         
   
	static void considerUnloads();

#if XNU_KERNEL_PRIVATE
	static bool removeClasses(OSCollection * metaClasses);
#endif                         

   
                               
  
            
                                                           
  
                                             
  
          
                                                                     
                                
  
              
                                                         
                                                             
                      
  
                                                              
                                             
                                                                              
                                   
   
	static OSObject * allocClassWithName(const OSSymbol * name);


   
                              
  
            
                                                           
  
                                             
  
          
                                                                     
                                
  
              
                                                         
                                                             
                      
  
                                                              
                                             
                                                                              
                                   
   
	static OSObject * allocClassWithName(const OSString * name);


   
                              
  
            
                                                           
  
                                             
  
          
                                                                     
                                
  
              
                                                         
                                                             
                      
  
                                                              
                                             
                                                                              
                                   
   
	static OSObject * allocClassWithName(const char * name);


   
                                  
  
            
                                                                             
  
                                                                
                                                                 
  
          
                                                                   
                               
  
              
                                                                           
                                                
                                                                        
                                                            
   
	static OSMetaClassBase * checkMetaCastWithName(
		const OSSymbol        * className,
		const OSMetaClassBase * object);

   
                                  
  
            
                                                                             
  
                                                                
                                                                 
  
          
                                                                   
                               
  
              
                                                           
                                                                        
                                                            
   
	static OSMetaClassBase * checkMetaCastWithName(
		const OSString        * className,
		const OSMetaClassBase * object);

   
                                  
  
            
                                                                             
  
                                                                
                                                                 
  
          
                                                                   
                               
  
              
                                                           
                                                                        
                                                            
   
	static OSMetaClassBase * checkMetaCastWithName(
		const char            * className,
		const OSMetaClassBase * object);


   
                                
  
            
                                                               
  
              
                                           
  
                                                       
                                                            
                                             
                                     
              
                                
                                             
                                               
  
                                                         
                                                             
                                                   
                                                                    
                                                                       
   
	void instanceConstructed() const;


   
                               
  
            
                                                               
  
              
                                                       
                                                           
                                            
                                     
                                                                          
                                               
  
                                                         
                                                               
                                                   
                                                                       
                            
   
	void instanceDestructed() const;


   
                          
  
            
                                                               
                                            
  
                                                     
  
          
                                                                  
                            
   
	OSMetaClassBase * checkMetaCast(const OSMetaClassBase * object) const;


   
                             
  
            
                                                                     
  
          
                                                             
                                              
   
	unsigned int getInstanceCount() const;


   
                          
  
            
                                               
  
          
                                                            
                                                              
   
	const OSMetaClass * getSuperClass() const;

   
                        
  
            
                                                        
                               
  
          
                                                                             
  
              
                                                
   
	const OSSymbol * getKmodName() const;


   
                         
  
            
                                                               
  
          
                                                               
   
	const char * getClassName() const;
	const OSSymbol * getClassNameSymbol() const;


   
                         
  
            
                                                                          
  
          
                                                                  
   
	unsigned int getClassSize() const;


   
                  
  
            
                                                                    
  
          
                                                            
                                                                     
  
              
                                                                              
                                         
   
	virtual OSObject * alloc() const = 0;

#ifdef XNU_KERNEL_PRIVATE
	OSKext * getKext() const;
	void addInstance(const OSObject * instance, bool super = false) const;
	void removeInstance(const OSObject * instance, bool super = false) const;
	void applyToInstances(OSMetaClassInstanceApplierFunction applier,
	    void * context) const;
	static void applyToInstancesOfClassName(
		const OSSymbol * name,
		OSMetaClassInstanceApplierFunction  applier,
		void * context);
private:
	static void applyToInstances(OSOrderedSet * set,
	    OSMetaClassInstanceApplierFunction  applier,
	    void * context);
public:
#endif                         

                                   
  
                                   
                
  
            
                                                                   
              
  
                                                              
                                                 
   

#define _OS_ADD_METAMETHODS(b) _OS_ADD_METAMETHODS_ ## b
#define _OS_ADD_METAMETHODS_
#define _OS_ADD_METAMETHODS_dispatch                            \
    virtual kern_return_t Dispatch(const IORPC rpc) APPLE_KEXT_OVERRIDE;

#define _OS_ADD_METHODS(className, b) _OS_ADD_METHODS_ ## b(className)
#define _OS_ADD_METHODS_(className)
#define _OS_ADD_METHODS_dispatch(className)                     \
    className ## _Methods                                       \
    className ## _KernelMethods

#define SUPERDISPATCH ((OSDispatchMethod)&super::_Dispatch)

#define OSDeclareCommonStructors(className, dispatch)           \
    private:                                                    \
    static const OSMetaClass * const superClass;                \
    public:                                                     \
    static const OSMetaClass * const metaClass;                 \
	static class MetaClass : public OSMetaClass {                 \
	public:                                                       \
	    MetaClass();                                              \
	    virtual OSObject *alloc() const APPLE_KEXT_OVERRIDE;      \
	    _OS_ADD_METAMETHODS(dispatch);                            \
	} gMetaClass;                                                 \
	friend class className ::MetaClass;                           \
	virtual const OSMetaClass * getMetaClass() const APPLE_KEXT_OVERRIDE; \
    protected:                                                  \
    className (const OSMetaClass *);                            \
    virtual ~ className () APPLE_KEXT_OVERRIDE;                 \
    _OS_ADD_METHODS(className, dispatch)

#define _OS_ADD_OPERATOR_PROTO                                  \
    public:                                                     \
    static void *operator new(size_t size);                     \
    protected:                                                  \
    static void operator delete(void *mem, size_t size);

   
                                    
                
  
            
                                                   
                                                    
  
                                                              
                                                 
  
              
                                                        
                                                              
                                                                  
   
#define _OSDeclareDefaultStructors(className, dispatch)         \
	OSDeclareCommonStructors(className, dispatch);              \
	public:                                                     \
	className (void);                                           \
	_OS_ADD_OPERATOR_PROTO                                      \
    protected:

#define OSDeclareDefaultStructors(className)                    \
	_OSDeclareDefaultStructors(className, )

#define OSDeclareDefaultStructorsWithDispatch(className)        \
	_OSDeclareDefaultStructors(className, dispatch)


   
                                     
                
  
            
                                                   
                                     
  
                                                              
                                                 
  
              
                                                        
                                                
                                                              
                                                                  
   
#define _OSDeclareAbstractStructors(className, dispatch)        \
	OSDeclareCommonStructors(className, dispatch)               \
    private:                                                    \
	                                                            \
	className (void);                                           \
    protected:                                                  \

#define OSDeclareAbstractStructors(className)                   \
	_OSDeclareAbstractStructors(className, )                    \
	_OS_ADD_OPERATOR_PROTO

#define OSDeclareAbstractStructorsWithDispatch(className)       \
	_OSDeclareAbstractStructors(className, dispatch)            \
	_OS_ADD_OPERATOR_PROTO

#define OSDeclareAbstractStructorsWithDispatchAndNoOperators(   \
		className)                                              \
	_OSDeclareAbstractStructors(className, dispatch)


   
                                  
                
  
            
                                                   
                                    
  
                                                              
                                                 
  
              
                                                                         
                                                                                
                                                                           
                                                            
                                                                  
  
                                                                      
                                                            
                                                
  
                                                                        
                        
   
#define _OSDeclareFinalStructors(className, dispatch)           \
	_OSDeclareDefaultStructors(className, dispatch)             \
    private:                                                    \
	void __OSFinalClass(void);                                  \
    protected:

#define OSDeclareFinalStructors(className)                      \
	_OSDeclareFinalStructors(className, )

#define OSDeclareFinalStructorsWithDispatch(className)          \
	_OSDeclareFinalStructors(className, dispatch)


                                   
  
                                    
                
  
            
                                                                   
              
  
                                                                   
                                                      
                                                                     
                                        
                                                      
                                                              
                                                    
  
              
                                       
                                             
                                                                     
                                                   
   
#define OSMetaClassConstructorInit(className, superclassName,   \
	    init)                                                   \
	                                                            \
	className ::MetaClass::MetaClass()                          \
	: OSMetaClass(#className, className::superClass,            \
	                sizeof(className))                          \
	{ init; }

#ifdef XNU_KERNEL_PRIVATE
#define declareZone(className)                                  \
	static SECURITY_READ_ONLY_LATE(zone_t) className ## _zone;
#elif KERNEL_PRIVATE                         
#define declareZone(className)                                  \
	static zone_t className ## _zone;
#endif                     

#ifdef KERNEL_PRIVATE
#define OSMetaClassConstructorInitWithZone(className,           \
	    superclassName, init, zflags)                           \
	declareZone(className)                                      \
	                                                            \
	className ::MetaClass::MetaClass()                          \
	: OSMetaClass(#className, className::superClass,            \
	                sizeof(className),                          \
	                &(className ## _zone),                      \
	                "iokit." #className, zflags)                \
	{ init; }
#endif                     

#define OSDefineMetaClassWithInit(className, superclassName,    \
	    init)                                                   \
	                                                            \
	className ::MetaClass className ::gMetaClass;               \
	const OSMetaClass * const className ::metaClass =           \
	& className ::gMetaClass;                                   \
	const OSMetaClass * const className ::superClass =          \
	& superclassName ::gMetaClass;                              \
	                                                            \
	className :: className(const OSMetaClass *meta)             \
	: superclassName (meta) { }                                 \
	className ::~ className() { }                               \
	const OSMetaClass * className ::getMetaClass() const        \
	{ return &gMetaClass; }


                                   
  
                                    
                
  
            
                                                                   
              
  
                                                                   
                                                      
                                                                     
                                        
                                                      
   
#define OSDefineAbstractStructors(className, superclassName)    \
    OSObject * className ::MetaClass::alloc() const { return NULL; }


                                   
  
                                   
                
  
            
                                                                   
              
  
                                                                   
                                                      
                                                                     
                                        
                                                      
   
#define OSDefineBasicStructors(className, superclassName)       \
	OSObject * className ::MetaClass::alloc() const             \
	{ return new className; }                                   \
	className :: className () : superclassName (&gMetaClass)    \
	{ gMetaClass.instanceConstructed(); }

#define OSDefineOperatorMethods(className)                      \
	void * className::operator new(size_t size)                 \
	{ return OSObject::operator new(size); }                    \
	void className::operator delete(void *mem, size_t size)     \
	{ return OSObject::operator delete(mem, size); }

#ifdef KERNEL_PRIVATE
#define OSDefineOperatorMethodsWithZone(className)              \
	void * className :: operator new(size_t size) {             \
	    if(className ## _zone) {                                \
	        return zalloc_flags(className ## _zone,             \
	                        (zalloc_flags_t) (Z_WAITOK | Z_ZERO));\
	    } else {                                                \
	  
                                      
                     
                                                             \
	        return OSObject::operator new(size);                \
	    }                                                       \
	}                                                           \
	void className :: operator delete(void *mem, size_t size) { \
	    if(className ## _zone) {                                \
	        kern_os_zfree(className ## _zone, mem, size);       \
	    } else {                                                \
	  
                                      
                     
                                                             \
	        return OSObject::operator delete(mem, size);        \
	    }                                                       \
	}
#endif                     

#define OSDefineDefaultStructors(className, superclassName)     \
	OSDefineBasicStructors(className, superclassName)           \
	OSDefineOperatorMethods(className)


                                   
  
                                   
                
  
            
                                                                   
              
  
                                                                   
                                                      
                                                                     
                                        
                                                      
   
#define OSDefineFinalStructors(className, superclassName)       \
    OSDefineBasicStructors(className, superclassName)           \
    void className ::__OSFinalClass(void) { }


                                   
  
                                                
                
  
            
                                                                   
              
  
                                                                   
                                                      
                                                                     
                                        
                                                      
                                                              
                                                    
   
#define OSDefineMetaClassAndStructorsWithInit(className,        \
	    superclassName, init)                                   \
	OSDefineMetaClassWithInit(className, superclassName, init)  \
	OSMetaClassConstructorInit(className, superclassName, init) \
	OSDefineDefaultStructors(className, superclassName)

#ifdef KERNEL_PRIVATE
                                   
  
                                                        
                
  
            
                                                                   
              
  
                                                                   
                                                      
                                                                     
                                        
                                                      
                                                              
                                                    
                                             
  
              
                      
                                                               
                                                                
                                                              
                                                    
   
#define OSDefineMetaClassAndStructorsWithInitAndZone(className, \
	    superclassName, init, zflags)                           \
	OSDefineMetaClassWithInit(className, superclassName, init)  \
	OSMetaClassConstructorInitWithZone(className,               \
	                superclassName, init, zflags)               \
	OSDefineBasicStructors(className, superclassName)           \
	OSDefineOperatorMethodsWithZone(className)
#endif                     

                                   
  
                                                        
                
  
            
                                                                   
              
  
                                                                   
                                                      
                                                                     
                                        
                                                      
                                                              
                                                    
   
#define OSDefineMetaClassAndAbstractStructorsWithInit(          \
		className, superclassName, init)                        \
	OSDefineMetaClassWithInit(className, superclassName, init)  \
	OSMetaClassConstructorInit(className, superclassName, init) \
	OSDefineAbstractStructors(className, superclassName)        \
	OSDefineOperatorMethods(className)


                                   
  
                                                     
                
  
            
                                                                   
              
  
                                                                   
                                                      
                                                                     
                                        
                                                      
                                                              
                                                    
   
#define OSDefineMetaClassAndFinalStructorsWithInit(className,   \
	    superclassName, init)                                   \
	OSDefineMetaClassWithInit(className, superclassName, init)  \
	OSMetaClassConstructorInit(className, superclassName, init) \
	OSDefineFinalStructors(className, superclassName)           \
	OSDefineOperatorMethods(className)

#ifdef KERNEL_PRIVATE
                                   
  
                                                            
                
  
            
                                                                   
              
  
                                                                   
                                                      
                                                                     
                                        
                                                      
                                                              
                                                    
                                             
  
              
                      
                                                                    
                                                                
                                                              
                                                    
   
#define OSDefineMetaClassAndFinalStructorsWithInitAndZone(      \
		className, superclassName, init, zflags)                \
	OSDefineMetaClassWithInit(className, superclassName, init)  \
	OSMetaClassConstructorInitWithZone(className,               \
	                superclassName, init, zflags)               \
	OSDefineFinalStructors(className, superclassName)           \
	OSDefineOperatorMethodsWithZone(className)
#endif

             

                                   
  
                            
                
  
            
                                                                   
              
  
                                                                   
                                                      
                                                                     
                                        
                                                      
                                                              
                                                    
   
#define OSDefineMetaClass(className, superclassName)            \
	OSDefineMetaClassWithInit(className, superclassName, )      \
	OSMetaClassConstructorInit(className, superclassName, )     \
	OSDefineOperatorMethods(className)


   
                                        
                
  
            
                                                 
                                    
  
                                                                   
                                                      
                                                                     
                                        
                                                      
  
              
                                                        
                                                  
                                                     
   
#define OSDefineMetaClassAndStructors(className, superclassName) \
	OSDefineMetaClassAndStructorsWithInit(className,            \
	                superclassName, )

#ifdef KERNEL_PRIVATE
   
                                                
                
  
            
                                                 
                                    
  
                                                                   
                                                      
                                                                     
                                        
                                                      
                                             
  
              
                      
                                                               
                                                                
                                                              
                                                    
   
#define OSDefineMetaClassAndStructorsWithZone(className,        \
	    superclassName, zflags)                                 \
	OSDefineMetaClassAndStructorsWithInitAndZone(className,     \
	                superclassName, , zflags)
#endif

   
                                                
                
  
            
                                                 
                                     
  
                                                                   
                                                      
                                                                     
                                        
                                                      
  
              
                                                        
                                                
                                                  
                                                     
   
#define OSDefineMetaClassAndAbstractStructors(className,        \
	    superclassName)                                         \
	OSDefineMetaClassAndAbstractStructorsWithInit (className,   \
	                superclassName, )


   
                                             
                
  
            
                                                 
                                  
  
                                                                   
                                                      
                                                                     
                                        
                                                      
  
              
                                                     
                                                         
                                 
                                                     
                                                                           
                                                            
  
                                                                      
                                                            
                                                
  
                                                                        
                        
   
#define OSDefineMetaClassAndFinalStructors(className,           \
	    superclassName)                                         \
	OSDefineMetaClassAndFinalStructorsWithInit(className,       \
	                superclassName, )

#ifdef KERNEL_PRIVATE
   
                                                     
                
  
            
                                                 
                                  
  
                                                                   
                                                      
                                                                     
                                        
                                                      
                                             
  
              
                      
                                                            
                                                                
                                                              
                                                    
   
#define OSDefineMetaClassAndFinalStructorsWithZone(className,   \
	    superclassName, zflags)                                 \
	OSDefineMetaClassAndFinalStructorsWithInitAndZone(          \
	                className, superclassName, , zflags)
#endif                     


                                                    
	void reservedCalled(int ind) const;


   
                                           
                
  
            
                                                  
                          
  
                                                                   
                                                      
                                                              
                                                             
  
              
                                                                         
                                                               
                           
                                                             
                                                                      
                                  
                                              
                                                      
  
                                                                 
                                                                    
                                                                  
                               
      
                                             
                                                     
   
#if APPLE_KEXT_VTABLE_PADDING
#define OSMetaClassDeclareReservedUnused(className, index)      \
    virtual void _RESERVED ## className ## index ()
#else
#define OSMetaClassDeclareReservedUnused(className, index)
#endif


   
                                         
                
  
            
                                                                   
                          
  
                                                                   
                                                      
                                                              
                                                             
  
              
                                                                    
                                   
      
                                               
                                                       
   
#define OSMetaClassDeclareReservedUsed(className, index)
#define OSMetaClassDeclareReservedUsedARM(className, x86index, armindex)


   
                                          
                
  
            
                                                          
  
                                                                   
                                                      
                                                              
                                                             
  
              
                                                                         
                                                               
                           
                                                             
                                      
                                           
                                               
                                                       
  
                                                
                                                                    
                                                                     
  
                                                                 
                                                                    
                                                                  
                               
      
                                            
                                                    
   
#if APPLE_KEXT_VTABLE_PADDING
#define OSMetaClassDefineReservedUnused(className, index)       \
void className ::_RESERVED ## className ## index ()             \
	{ gMetaClass.reservedCalled(index); }
#else
#define OSMetaClassDefineReservedUnused(className, index)
#endif


   
                                        
                
  
            
                                                                          
  
                                                                   
                                                      
                                                              
                                                             
  
              
                                                                    
                                   
      
                                              
                                                      
   
#define OSMetaClassDefineReservedUsed(className, index)
#define OSMetaClassDefineReservedUsedARM(className, x86index, armindex)

  
                                                                              
                                             
   
#if defined(__arm64__) || defined(__arm__)
#define OSMetaClassDeclareReservedUsedX86               OSMetaClassDeclareReservedUnused
#define OSMetaClassDefineReservedUsedX86                OSMetaClassDefineReservedUnused
#else
#define OSMetaClassDeclareReservedUsedX86               OSMetaClassDeclareReservedUsed
#define OSMetaClassDefineReservedUsedX86                OSMetaClassDefineReservedUsed

#endif

                                   
	static void printInstanceCounts();
	static void serializeClassDictionary(OSDictionary * dict);
#ifdef XNU_KERNEL_PRIVATE
#if IOTRACKING
public:
	static void * trackedNew(size_t size);
	static void trackedDelete(void * mem, size_t size);
	void trackedInstance(OSObject * instance) const;
	void trackedFree(OSObject * instance) const;
	void trackedAccumSize(OSObject * instance, size_t size) const;
	struct IOTrackingQueue * getTracking() const;
#endif                 
#endif                         

private:
                
	static OSDictionary * getClassDictionary();
	virtual bool serialize(OSSerialize * serializer) const;

                                            
	OSMetaClassDeclareReservedUnused(OSMetaClass, 0);
	OSMetaClassDeclareReservedUnused(OSMetaClass, 1);
	OSMetaClassDeclareReservedUnused(OSMetaClass, 2);
	OSMetaClassDeclareReservedUnused(OSMetaClass, 3);
	OSMetaClassDeclareReservedUnused(OSMetaClass, 4);
	OSMetaClassDeclareReservedUnused(OSMetaClass, 5);
	OSMetaClassDeclareReservedUnused(OSMetaClass, 6);
	OSMetaClassDeclareReservedUnused(OSMetaClass, 7);
};

#endif                              
