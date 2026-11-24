/*
 * Copyright (c) 1998-2016 Apple Computer, Inc. All rights reserved.
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
/*
 * Copyright (c) 1998 Apple Computer, Inc.  All rights reserved.
 *
 * HISTORY
 *
 */

#ifndef __IOKIT_IOLIB_H
#define __IOKIT_IOLIB_H

#ifndef KERNEL
#error IOLib.h is for kernel use only
#endif

#include <stdarg.h>
#include <sys/cdefs.h>
#include <os/overflow.h>

#include <sys/appleapiopts.h>

#include <IOKit/system.h>

#include <IOKit/IOReturn.h>
#include <IOKit/IOTypes.h>
#include <IOKit/IOLocks.h>

#include <libkern/OSAtomic.h>

__BEGIN_DECLS

#include <kern/thread_call.h>
#include <kern/clock.h>

  
                  
   

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

  
                                
   
typedef thread_t IOThread;
typedef void (*IOThreadFunc)(void *argument);

  
                               
   

                      
                                                                         
                                                                                                                                                                                                                                                                                                                           
                                              
                                                                    

#if defined(XNU_KERNEL_PRIVATE)

  
                                                                                  
                                                        
                                                               
                                                                       
                         
  
                                                                           
                                                                         
  
                                                
   

extern void *
IOMalloc_internal(
	struct kalloc_heap * kalloc_heap_cfg,
	vm_size_t            size)      __attribute__((alloc_size(2)));

extern void *
IOMallocZero_internal(
	struct kalloc_heap * kalloc_heap_cfg,
	vm_size_t            size)      __attribute__((alloc_size(2)));

#define IOMalloc(size)     IOMalloc_internal(KHEAP_DEFAULT, size)

#define IOMallocZero(size) IOMallocZero_internal(KHEAP_DEFAULT, size)

#else                                 

void * IOMalloc(vm_size_t size)      __attribute__((alloc_size(1)));
void * IOMallocZero(vm_size_t size)  __attribute__((alloc_size(1)));

#endif                                   

                    
                                                    
                                                                                                                                                                  
                                                                                
                          
                                                                           
                                   

void   IOFree(void * address, vm_size_t size);

                             
                                                                                       
                                                                                                                                                                                                                                               
                                              
                                                                                                                                                
                                                                    

#if defined(XNU_KERNEL_PRIVATE)

extern void *
IOMallocAligned_internal(
	struct kalloc_heap * kalloc_heap_cfg,
	vm_size_t            size,
	vm_size_t            alignment)      __attribute__((alloc_size(2)));

#define IOMallocAligned(size, alignment) IOMallocAligned_internal(KHEAP_DEFAULT, size, alignment)

#else                                 

void * IOMallocAligned(vm_size_t size, vm_offset_t alignment) __attribute__((alloc_size(1)));

#endif                                   

                           
                                                           
                                                                                                                                                                         
                                                    
                                                 

void   IOFreeAligned(void * address, vm_size_t size);

                                
                                                                                                                                                            
                                                                                                                                                                                                                                                                                                                                                                                                  
                                              
                                                                                                                                                 
                                                                                                                                                                                                                                                                                                                                                                                           
                                                                            

void * IOMallocContiguous(vm_size_t size, vm_size_t alignment,
    IOPhysicalAddress * physicalAddress) __attribute__((deprecated)) __attribute__((alloc_size(1)));

                              
                                                                                                         
                                                                                                                                                                            
                                                            
                                                 

void   IOFreeContiguous(void * address, vm_size_t size) __attribute__((deprecated));


                              
                                                           
                                                                                                                                                                                      
                                              
                                                                                                                                                 
                                                                    

void * IOMallocPageable(vm_size_t size, vm_size_t alignment) __attribute__((alloc_size(1)));

                                  
                                                                   
                                                                                            
                                              
                                                                                                                                                 
                                                                    

void * IOMallocPageableZero(vm_size_t size, vm_size_t alignment) __attribute__((alloc_size(1)));

                            
                                                            
                                                                                                                                                                          
                                                            
                                                 

void IOFreePageable(void * address, vm_size_t size);

  
                                                 
   

#define IONew(type, count)                              \
({                                                      \
    size_t __size;                                      \
    (os_mul_overflow(sizeof(type), (count), &__size)    \
    ? ((type *) NULL)                                   \
    : ((type *) IOMalloc(__size)));                     \
})

#define IONewZero(type, count)                          \
({                                                      \
    size_t __size;                                      \
    (os_mul_overflow(sizeof(type), (count), &__size)    \
    ? ((type *) NULL)                                   \
    : ((type *) IOMallocZero(__size)));                 \
})

#define IODelete(ptr, type, count)                          \
({                                                          \
    size_t __size;                                          \
    if (!os_mul_overflow(sizeof(type), (count), &__size)) { \
	IOFree(ptr, __size);                                \
    }                                                       \
})

#define IOSafeDeleteNULL(ptr, type, count)              \
    do {                                                \
	if (NULL != (ptr)) {                            \
	    IODelete((ptr), type, count);               \
	    (ptr) = NULL;                               \
	}                                               \
    } while (0)                                         \

                                                                             
  
  
                                                      
  
  
                                                                             

                           
                                                                         
                                                                                                                                                                                                                                                                                                                                
                                                                                               
                                               

UInt8 IOMappedRead8(IOPhysicalAddress address);

                            
                                                                          
                                                                                                                                                                                                                                                                                                                                 
                                                                                               
                                               

UInt16 IOMappedRead16(IOPhysicalAddress address);

                            
                                                                           
                                                                                                                                                                                                                                                                                                                                  
                                                                                               
                                               

UInt32 IOMappedRead32(IOPhysicalAddress address);

                            
                                                                            
                                                                                                                                                                                                                                                                                                                                   
                                                                                               
                                               

UInt64 IOMappedRead64(IOPhysicalAddress address);

                            
                                                                        
                                                                                                                                                                                                       
                                                                                               
                                                              

void IOMappedWrite8(IOPhysicalAddress address, UInt8 value);

                             
                                                                         
                                                                                                                                                                                                        
                                                                                               
                                                              

void IOMappedWrite16(IOPhysicalAddress address, UInt16 value);

                             
                                                                          
                                                                                                                                                                                                         
                                                                                               
                                                              

void IOMappedWrite32(IOPhysicalAddress address, UInt32 value);

                             
                                                                           
                                                                                                                                                                                                          
                                                                                               
                                                              

void IOMappedWrite64(IOPhysicalAddress address, UInt64 value);

                                                                                                                        

IOReturn IOSetProcessorCacheMode( task_t task, IOVirtualAddress address,
    IOByteCount length, IOOptionBits cacheMode ) __attribute__((deprecated));

                                   
                                                             
                                                                                                                                                                                                                                                                                                                                                   
                                                
                                                  
                                              
                                 

IOReturn IOFlushProcessorCache( task_t task, IOVirtualAddress address,
    IOByteCount length );

                          
                                                                             
                                                                                                                      

#define IOThreadSelf() (current_thread())

                            
                                                                                       
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
                                                                                
                                                                          
                                                                                           

IOThread IOCreateThread(IOThreadFunc function, void *argument) __attribute__((deprecated));

                          
                                                                                                   
                                                                                             

void IOExitThread(void) __attribute__((deprecated));

                     
                                                                     
                                                                                                                                           
                                                                       

void IOSleep(unsigned milliseconds);

                               
                                                                                                                                      
                                                                                                                                                                                                                                                  
                                                                            
                                                                                                            

void IOSleepWithLeeway(unsigned intervalMilliseconds, unsigned leewayMilliseconds);

                     
                                                       
                                                                                                                                                                                                                                                                                                                                        
                                                                            

void IODelay(unsigned microseconds);

                     
                                                      
                                                                                                                                                                                                                                        
                                                                          

void IOPause(unsigned nanoseconds);

                   
                                                                              
                                                                                                                                                                                                                   
                                                                                
   

void IOLog(const char *format, ...)
__attribute__((format(printf, 1, 2)));

                    
                                                                              
                                                                                                                                                                                                                    
                                                                                
                                                     

void IOLogv(const char *format, va_list ap)
__attribute__((format(printf, 1, 0)));

#ifndef _FN_KPRINTF
#define _FN_KPRINTF
void kprintf(const char *format, ...) __printflike(1, 2);
#endif
#ifndef _FN_KPRINTF_DECLARED
#define _FN_KPRINTF_DECLARED
#endif

  
                                                                       
                                
   
const char *IOFindNameForValue(int value,
    const IONamedValue *namedValueArray);

  
                                                                   
                                                                       
   
IOReturn IOFindValueForName(const char *string,
    const IONamedValue *regValueArray,
    int *value);                                              

                      
                                         
                                                                                                                                                        
                                                                               

void Debugger(const char * reason);
#if __LP64__
#define IOPanic(reason) panic("%s", reason)
#else
void IOPanic(const char *reason) __attribute__((deprecated)) __abortlike;
#endif

#ifdef __cplusplus
class OSDictionary;
#endif

#ifdef __cplusplus
OSDictionary *
#else
struct OSDictionary *
#endif
IOBSDNameMatching( const char * name );

#ifdef __cplusplus
OSDictionary *
#else
struct OSDictionary *
#endif
IOOFPathMatching( const char * path, char * buf, int maxLen ) __attribute__((deprecated));

  
                                                     
   
IOAlignment IOSizeToAlignment(unsigned int size);
unsigned int IOAlignmentToSize(IOAlignment align);

  
                                                     
   

static inline IOFixed
IOFixedMultiply(IOFixed a, IOFixed b)
{
	return (IOFixed)((((SInt64) a) * ((SInt64) b)) >> 16);
}

static inline IOFixed
IOFixedDivide(IOFixed a, IOFixed b)
{
	return (IOFixed)((((SInt64) a) << 16) / ((SInt64) b));
}

  
                                                           
                                         
   
#define IORound(value, multiple) \
	((((value) + (multiple) - 1) / (multiple)) * (multiple))

#define IOTrunc(value, multiple) \
	(((value) / (multiple)) * (multiple));


#if defined(__APPLE_API_OBSOLETE)

                                     

                                   
                                                 

void IOGetTime( mach_timespec_t * clock_time) __attribute__((deprecated));

#if !defined(__LP64__)

#undef eieio
#define eieio() \
    OSSynchronizeIO()

extern mach_timespec_t IOZeroTvalspec;

#endif                         

#endif                           

#if XNU_KERNEL_PRIVATE
vm_tag_t
IOMemoryTag(vm_map_t map);
#endif

__END_DECLS

#endif                       
