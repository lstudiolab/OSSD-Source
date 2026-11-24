/*
 * Copyright (c) 2007-2012 Apple Inc. All rights reserved.
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
 *
 * HISTORY
 *
 */

#ifndef _OS_OSATOMIC_H
#define _OS_OSATOMIC_H

#include <libkern/OSBase.h>
#include <string.h>

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef XNU_KERNEL_PRIVATE
  
                                                                                   
                                                                                    
                                                                                     
                                                                                   
                                                                                    
              
                                                   
                                                           
   
#define __SAFE_CAST_PTR(type, var) (((type)(var))+(0/(sizeof(*var) == sizeof(*(type)NULL) ? 1 : 0)))
#else
#define __SAFE_CAST_PTR(type, var) ((type)(var))
#endif

   
          
  
            
                                                                  
                                  
   

   
                               
  
            
                                     
  
              
                        
   
extern Boolean OSCompareAndSwap64(
	UInt64            oldValue,
	UInt64            newValue,
	volatile UInt64 * address);
#define OSCompareAndSwap64(a, b, c) \
	(OSCompareAndSwap64(a, b, __SAFE_CAST_PTR(volatile UInt64*,c)))

   
                          
  
            
                               
  
              
                   
   
extern SInt64 OSAddAtomic64(
	SInt64            theAmount,
	volatile SInt64 * address);
#define OSAddAtomic64(a, b) \
	(OSAddAtomic64(a, __SAFE_CAST_PTR(volatile SInt64*,b)))

   
                                
  
            
                    
  
              
                         
   
inline static SInt64
OSIncrementAtomic64(volatile SInt64 * address)
{
	return OSAddAtomic64(1LL, address);
}

   
                                
  
            
                    
  
              
                         
   
inline static SInt64
OSDecrementAtomic64(volatile SInt64 * address)
{
	return OSAddAtomic64(-1LL, address);
}

#if XNU_KERNEL_PRIVATE
                                   
  
                            
  
            
                                                             
  
              
                   
   
extern long OSAddAtomicLong(
	long            theAmount,
	volatile long * address);
#define OSAddAtomicLong(a, b) \
	(OSAddAtomicLong(a, __SAFE_CAST_PTR(volatile long*,b)))

                                   
  
                                  
  
            
                                                 
  
              
                         
   
inline static long
OSIncrementAtomicLong(volatile long * address)
{
	return OSAddAtomicLong(1L, address);
}

                                   
  
                                  
  
            
                                                 
                                    
   
inline static long
OSDecrementAtomicLong(volatile long * address)
{
	return OSAddAtomicLong(-1L, address);
}
#endif                         

#if XNU_KERNEL_PRIVATE
   
                              
  
            
                                                                                                                                               
  
              
                                                                                                                                                                                                                                                                                                     
  
                                                                                                                                                                                                                                                                                                                                                                   
  
                                                   
                                                                           
                                                                            
                                                       
   
extern Boolean OSCompareAndSwap8(
	UInt8            oldValue,
	UInt8            newValue,
	volatile UInt8 * address);
#define OSCompareAndSwap8(a, b, c) \
	(OSCompareAndSwap8(a, b, __SAFE_CAST_PTR(volatile UInt8*,c)))

   
                               
  
            
                                                                                                                                               
  
              
                                                                                                                                                                                                                                                                                                      
  
                                                                                                                                                                                                                                                                                                                                                                   
  
                                                   
                                                                           
                                                                              
                                                       
   
extern Boolean OSCompareAndSwap16(
	UInt16            oldValue,
	UInt16            newValue,
	volatile UInt16 * address);
#define OSCompareAndSwap16(a, b, c) \
	(OSCompareAndSwap16(a, b, __SAFE_CAST_PTR(volatile UInt16*,c)))

#endif                         

   
                             
  
            
                                                                                                                                               
  
              
                                                                                                                                                                                                                                                                                                    
  
                                                                                                                                                                                                                                                                                                                                                                   
  
                                                   
                                                                           
                                                                              
                                                       
   
extern Boolean OSCompareAndSwap(
	UInt32            oldValue,
	UInt32            newValue,
	volatile UInt32 * address);
#define OSCompareAndSwap(a, b, c) \
	(OSCompareAndSwap(a, b, __SAFE_CAST_PTR(volatile UInt32*,c)))

   
                                
  
            
                                                                                                                                               
  
              
                                                                                                                                                                                                                                                                                                                        
  
                                                                                                                                                                                                                                                                                                                                                                   
                                                           
                                                                                   
                                                                                    
                                                       
   
extern Boolean OSCompareAndSwapPtr(
	void            * oldValue,
	void            * newValue,
	void * volatile * address);
#define OSCompareAndSwapPtr(a, b, c) \
	(OSCompareAndSwapPtr(a, b, __SAFE_CAST_PTR(void * volatile *,c)))

   
                        
  
            
                                                                                                                                         
  
              
                                                                                                                           
  
                                                                                                                                                                                                                                                                                                                                                                                                                        
                                   
                                                                               
                                        
   
extern SInt32 OSAddAtomic(
	SInt32            amount,
	volatile SInt32 * address);
#define OSAddAtomic(a, b) \
	(OSAddAtomic(a, __SAFE_CAST_PTR(volatile SInt32*,b)))

   
                          
  
            
                                                                                                                                         
  
              
                                                                                                                             
  
                                                                                                                                                                                                                                                                                                                                                                                                                        
                                                                               
                                        
   
extern SInt16 OSAddAtomic16(
	SInt32            amount,
	volatile SInt16 * address);

   
                         
  
            
                                                                                                                                        
  
              
                                                                                                                            
  
                                                                                                                                                                                                                                                                                                                                                                                                                        
                                   
                                                                
                                         
   
extern SInt8 OSAddAtomic8(
	SInt32           amount,
	volatile SInt8 * address);

   
                              
  
            
                                                                                                                                               
  
              
                                                                                                                      
  
                                                                                                                                                                                                                                                    
                                                                               
                                          
   
extern SInt32 OSIncrementAtomic(volatile SInt32 * address);
#define OSIncrementAtomic(a) \
	(OSIncrementAtomic(__SAFE_CAST_PTR(volatile SInt32*,a)))

   
                                
  
            
                                                                                                                                               
  
              
                                                                                                                        
  
                                                                                                                                                                                                                                                                                                                                                                                                                        
                                                                               
                                          
   
extern SInt16 OSIncrementAtomic16(volatile SInt16 * address);

   
                               
  
            
                                                                                                                                              
  
              
                                                                                                                       
  
                                                                                                                                                                                                                                                                                                                                                                                                                        
                                                                
                                          
   
extern SInt8 OSIncrementAtomic8(volatile SInt8 * address);

   
                              
  
            
                                                                                                                                               
  
              
                                                                                                                      
  
                                                                                                                                                                                                                                                                                                                                                                                                                        
                                                                               
                                          
   
extern SInt32 OSDecrementAtomic(volatile SInt32 * address);
#define OSDecrementAtomic(a) \
	(OSDecrementAtomic(__SAFE_CAST_PTR(volatile SInt32*,a)))

   
                                
  
            
                                                                                                                                               
  
              
                                                                                                                        
  
                                                                                                                                                                                                                                                                                                                                                                                                                        
                                                                               
                                          
   
extern SInt16 OSDecrementAtomic16(volatile SInt16 * address);

   
                               
  
            
                                                                                                                                              
  
              
                                                                                                                       
  
                                                                                                                                                                                                                                                                                                                                                                                                                        
                                                                
                                          
   
extern SInt8 OSDecrementAtomic8(volatile SInt8 * address);

   
                           
  
            
                                                                                                                                                 
  
              
                                                                                                                                                    
  
                                                                                                                                                                                                                                                                                                                                                                                                                         
                                                        
                                                                               
                                                 
   
extern UInt32 OSBitAndAtomic(
	UInt32            mask,
	volatile UInt32 * address);
#define OSBitAndAtomic(a, b) \
	(OSBitAndAtomic(a, __SAFE_CAST_PTR(volatile UInt32*,b)))

   
                             
  
            
                                                                                                                                                 
  
              
                                                                                                                                                      
  
                                                                                                                                                                                                                                                    
                                                        
                                                                               
                                                  
   
extern UInt16 OSBitAndAtomic16(
	UInt32            mask,
	volatile UInt16 * address);

   
                            
  
            
                                                                                                                                                
  
              
                                                                                                                                                     
  
                                                                                                                                                                                                                                                    
                                                        
                                                                
                                                  
   
extern UInt8 OSBitAndAtomic8(
	UInt32           mask,
	volatile UInt8 * address);

   
                          
  
            
                                                                                                                                                
  
              
                                                                                                                                                  
  
                                                                                                                                                                                                                                                    
                                                       
                                                                               
                                                  
   
extern UInt32 OSBitOrAtomic(
	UInt32            mask,
	volatile UInt32 * address);
#define OSBitOrAtomic(a, b) \
	(OSBitOrAtomic(a, __SAFE_CAST_PTR(volatile UInt32*,b)))

   
                            
  
            
                                                                                                                                                
  
              
                                                                                                                                                    
  
                                                                                                                                                                                                                                                    
                                                       
                                                                               
                                                  
   
extern UInt16 OSBitOrAtomic16(
	UInt32            mask,
	volatile UInt16 * address);

   
                           
  
            
                                                                                                                                               
  
                                                                                                                                                                                                                                                    
  
              
                                                                                                                                                   
                                                       
                                                                
                                                  
   
extern UInt8 OSBitOrAtomic8(
	UInt32           mask,
	volatile UInt8 * address);

   
                           
  
            
                                                                                                                                                 
  
                                                                                                                                                                                                                                                    
  
              
                                                                                                                                                    
                                                       
                                                                               
                                                  
   
extern UInt32 OSBitXorAtomic(
	UInt32            mask,
	volatile UInt32 * address);
#define OSBitXorAtomic(a, b) \
	(OSBitXorAtomic(a, __SAFE_CAST_PTR(volatile UInt32*,b)))

   
                             
  
            
                                                                                                                                                 
  
              
                                                                                                                                                      
  
                                                                                                                                                                                                                                                    
                                                       
                                                                               
                                                  
   
extern UInt16 OSBitXorAtomic16(
	UInt32            mask,
	volatile UInt16 * address);

   
                            
  
            
                                                                                                                                                
  
                                                                                                                                                                                                                                                    
  
              
                                                                                                                                                     
                                                       
                                                                
                                                  
   
extern UInt8 OSBitXorAtomic8(
	UInt32           mask,
	volatile UInt8 * address);

   
                         
  
            
                                                                                                                                               
  
                                                                                                                                                                                                                                                    
  
              
                                                                                                                                             
                                                                                     
                                                                    
                                                            
   
extern Boolean OSTestAndSet(
	UInt32           bit,
	volatile UInt8 * startAddress);

   
                           
  
            
                                                                                                                                                 
  
              
                                                                                                                                                   
  
                                                                                                                                                                                                                                                    
                                                                                     
                                                                    
                                                              
   
extern Boolean OSTestAndClear(
	UInt32           bit,
	volatile UInt8 * startAddress);

   
                            
  
            
                                       
  
              
                                                              
   
#define OS_SPINLOCK_INIT 0

   
                      
  
            
                            
  
              
                                                                               
   
typedef SInt32 OSSpinLock;

#ifdef PRIVATE
   
                          
  
            
                                          
  
              
                                                                                                                                              
  
          
                                                                                                       
   
extern Boolean OSSpinLockTry(volatile OSSpinLock * lock);

   
                             
  
            
                      
  
              
                      
   
extern void OSSpinLockUnlock(volatile OSSpinLock * lock);
#endif              

   
                            
  
            
                                                                                                                
  
              
                                                                                                                                                                         
   
#if defined(__arm__) || defined(__arm64__)
extern void OSSynchronizeIO(void);
#else
static __inline__ void
OSSynchronizeIO(void)
{
}
#endif

#if     defined(KERNEL_PRIVATE)

#if     defined(__arm__) || defined(__arm64__)
static inline void
OSMemoryBarrier(void)
{
	__asm__ volatile ("dmb ish" ::: "memory");
}
#elif defined(__i386__) || defined(__x86_64__)
#if     defined(XNU_KERNEL_PRIVATE)
static inline void
OSMemoryBarrier(void)
{
	__asm__ volatile ("mfence" ::: "memory");
}
#endif                         
#endif

#endif                     

#if defined(__cplusplus)
}
#endif

#endif                       
