/*
 * Copyright (c) 1998-2012 Apple Inc. All rights reserved.
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
  
  
   

#ifndef __IOKIT_IOLOCKS_H
#define __IOKIT_IOLOCKS_H

#ifndef KERNEL
#error IOLocks.h is for kernel use only
#endif

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>

#include <IOKit/system.h>

#include <IOKit/IOReturn.h>
#include <IOKit/IOTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libkern/locks.h>
#include <machine/machine_routines.h>

                    
                                                                                                                                                                                        
   
extern lck_grp_t        *IOLockGroup;

#if defined(XNU_KERNEL_PRIVATE)
#define IOLOCKS_INLINE  1
#endif

  
                        
   

#ifdef  IOLOCKS_INLINE
typedef lck_mtx_t       IOLock;
#else
typedef struct _IOLock  IOLock;
#endif                      


                         
                                                 
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
                                                                  

IOLock * IOLockAlloc( void );

                        
                             
                                                                                                   
                                                  

void    IOLockFree( IOLock * lock);

                               
                                        
                                            
                                                  

lck_mtx_t * IOLockGetMachLock( IOLock * lock);

                        
                            
                                                                                                                                                                                                                                                                               
                                                  

#ifdef  IOLOCKS_INLINE
#define IOLockLock(l)   lck_mtx_lock(l)
#else
void    IOLockLock( IOLock * lock);
#endif                       

                           
                                       
                                                                                                                              
                                               
                                                                                                

#ifdef  IOLOCKS_INLINE
#define IOLockTryLock(l)        lck_mtx_try_lock(l)
#else
boolean_t IOLockTryLock( IOLock * lock);
#endif                       

                          
                              
                                                                                                                                                                                                                                   
                                                  

#ifdef  IOLOCKS_INLINE
#define IOLockUnlock(l) lck_mtx_unlock(l)
#else
void    IOLockUnlock( IOLock * lock);
#endif                       

                         
                                                 
                                                                                                                                                                                                                                                    
                                            
                                                          
                                                       
                                                                                
int     IOLockSleep( IOLock * lock, void *event, UInt32 interType) __DARWIN14_ALIAS(IOLockSleep);

int     IOLockSleepDeadline( IOLock * lock, void *event,
    AbsoluteTime deadline, UInt32 interType) __DARWIN14_ALIAS(IOLockSleepDeadline);

void    IOLockWakeup(IOLock * lock, void *event, bool oneThread) __DARWIN14_ALIAS(IOLockWakeup);

#ifdef XNU_KERNEL_PRIVATE
                               
                                                                   
   
typedef enum {
	kIOLockAssertOwned    = LCK_ASSERT_OWNED,
	kIOLockAssertNotOwned = LCK_ASSERT_NOTOWNED
} IOLockAssertState;

#ifdef IOLOCKS_INLINE
#define IOLockAssert(l, type) LCK_MTX_ASSERT(l, type)
#else
                            
                                                                              
                                                                             
                                                                                 
                   
   
void    IOLockAssert(IOLock * lock, IOLockAssertState type);
#endif                      
#endif                          

#ifdef __APPLE_API_OBSOLETE

                                     

typedef enum {
	kIOLockStateUnlocked        = 0,
	kIOLockStateLocked          = 1
} IOLockState;

void    IOLockInitWithState( IOLock * lock, IOLockState state);
#define IOLockInit( l ) IOLockInitWithState( l, kIOLockStateUnlocked);

static __inline__ void
IOTakeLock( IOLock * lock)
{
	IOLockLock(lock);
}
static __inline__ boolean_t
IOTryLock(  IOLock * lock)
{
	return IOLockTryLock(lock);
}
static __inline__ void
IOUnlock(   IOLock * lock)
{
	IOLockUnlock(lock);
}

#endif                           

  
                            
   

typedef struct _IORecursiveLock IORecursiveLock;

                                  
                                                           
                                                                                                                                                                                                                                                                                                                                                                                                                       
                                                                  

IORecursiveLock * IORecursiveLockAlloc( void );

                                 
                                      
                                                                                                           
                                                  

void            IORecursiveLockFree( IORecursiveLock * lock);

                                        
                                        
                                            
                                                  

lck_mtx_t * IORecursiveLockGetMachLock( IORecursiveLock * lock);

                                 
                                     
                                                                                                                                                                                                                                                                                                                                     
                                                  

void            IORecursiveLockLock( IORecursiveLock * lock);

                                    
                                                
                                                                                                                                                                                                                                                                   
                                               
                                                                              

boolean_t       IORecursiveLockTryLock( IORecursiveLock * lock);

                                   
                                       
                                                                                                                                                                                                                                                                                                                                
                                                  

void            IORecursiveLockUnlock( IORecursiveLock * lock);

                                     
                                                                       
                                                                                                                                                         
                                               
                                                                          

boolean_t       IORecursiveLockHaveLock( const IORecursiveLock * lock);

extern int      IORecursiveLockSleep( IORecursiveLock *_lock,
    void *event, UInt32 interType);
extern int      IORecursiveLockSleepDeadline( IORecursiveLock * _lock, void *event,
    AbsoluteTime deadline, UInt32 interType);
extern void     IORecursiveLockWakeup( IORecursiveLock *_lock,
    void *event, bool oneThread);

  
                                       
   

#ifdef  IOLOCKS_INLINE
typedef lck_rw_t                IORWLock;
#else
typedef struct _IORWLock        IORWLock;
#endif                      

                           
                                                           
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
                                                                  

IORWLock * IORWLockAlloc( void );

                          
                                      
                                                                                                   
                                                  

void    IORWLockFree( IORWLock * lock);

                                 
                                                  
                                                      
                                                  

lck_rw_t * IORWLockGetMachLock( IORWLock * lock);

                          
                                               
                                                                                                                                                                                                                                                                                                                                                         
                                                  

#ifdef  IOLOCKS_INLINE
#define IORWLockRead(l) lck_rw_lock_shared(l)
#else
void    IORWLockRead(IORWLock * lock);
#endif                       

                             
                                                          
                                                                                                                                                                
                                                  

#ifdef  IOLOCKS_INLINE
#define IORWLockTryRead(l)        lck_rw_try_lock_shared(l)
#else
void    IORWLockTryRead( IORWLock * lock);
#endif                       

                           
                                                
                                                                                                                                                                                                                                                                                                                                                   
                                                  

#ifdef  IOLOCKS_INLINE
#define IORWLockWrite(l)        lck_rw_lock_exclusive(l)
#else
void    IORWLockWrite( IORWLock * lock);
#endif                       

                              
                                                           
                                                                                                                                                          
                                                  

#ifdef  IOLOCKS_INLINE
#define IORWLockTryWrite(l)        lck_rw_try_lock_exclusive(l)
#else
void    IORWLockTryWrite( IORWLock * lock);
#endif                       

                            
                                        
                                                                                                                                                                                                                                    
                                                  

#ifdef  IOLOCKS_INLINE
#define IORWLockUnlock(l)       lck_rw_done(l)
#else
void    IORWLockUnlock( IORWLock * lock);
#endif                       

#ifdef XNU_KERNEL_PRIVATE
                                 
                                                                     
   
typedef enum {
	kIORWLockAssertRead    = LCK_RW_ASSERT_SHARED,
	kIORWLockAssertWrite   = LCK_RW_ASSERT_EXCLUSIVE,
	kIORWLockAssertHeld    = LCK_RW_ASSERT_HELD,
	kIORWLockAssertNotHeld = LCK_RW_ASSERT_NOTHELD
} IORWLockAssertState;

#ifdef IOLOCKS_INLINE
#define IORWLockAssert(l, type) LCK_RW_ASSERT(l, type)
#else
                              
                                                                           
                          
                                                                          
                                                                             
                               
   
void    IORWLockAssert(IORWLock * lock, IORWLockAssertState type);
#endif                      
#endif                          

#ifdef __APPLE_API_OBSOLETE

                                     

static __inline__ void
IOReadLock( IORWLock * lock)
{
	IORWLockRead(lock);
}
static __inline__ void
IOWriteLock(  IORWLock * lock)
{
	IORWLockWrite(lock);
}
static __inline__ void
IORWUnlock(   IORWLock * lock)
{
	IORWLockUnlock(lock);
}

#endif                           


  
                                                          
   

#ifdef  IOLOCKS_INLINE
typedef lck_spin_t              IOSimpleLock;
#else
typedef struct _IOSimpleLock    IOSimpleLock;
#endif                      

                               
                                                     
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
                                                                  

IOSimpleLock * IOSimpleLockAlloc( void );

                              
                                 
                                                               
                                        

void IOSimpleLockFree( IOSimpleLock * lock );

                                     
                                            
                                                
                                                  

lck_spin_t * IOSimpleLockGetMachLock( IOSimpleLock * lock);

                              
                                      
                                                                                                                                                                                                                                                                        
                                        

void IOSimpleLockInit( IOSimpleLock * lock );

                                 
                                                                                      
                                                                                                                                     
                                        

void IOSimpleLockDestroy( IOSimpleLock * lock );

                              
                                
                                                                                                                                                                                                                                                                                                                                                                                                                                                           
                                        

#ifdef  IOLOCKS_INLINE
#define IOSimpleLockLock(l)     lck_spin_lock(l)
#else
void IOSimpleLockLock( IOSimpleLock * lock );
#endif                       


                                 
                                           
                                                                                                                                                                                                                
                                     
                                                                                               

#ifdef  IOLOCKS_INLINE
#define IOSimpleLockTryLock(l)  lck_spin_try_lock(l)
#else
boolean_t IOSimpleLockTryLock( IOSimpleLock * lock );
#endif                       

                                
                                  
                                                                                                                      
                                        

#ifdef  IOLOCKS_INLINE
#define IOSimpleLockUnlock(l)   lck_spin_unlock(l)
#else
void IOSimpleLockUnlock( IOSimpleLock * lock );
#endif                       

#ifdef XNU_KERNEL_PRIVATE
                                     
                                                                         
   
typedef enum {
	kIOSimpleLockAssertOwned    = LCK_ASSERT_OWNED,
	kIOSimpleLockAssertNotOwned = LCK_ASSERT_NOTOWNED
} IOSimpleLockAssertState;

#ifdef IOLOCKS_INLINE
#define IOSimpleLockAssert(l, type) LCK_SPIN_ASSERT(l, type)
#else
                                  
                                                                                  
                                                                                         
                                                             
                                            
   
void    IOSimpleLockAssert(IOSimpleLock *lock, IOSimpleLockAssertState type);
#endif                      
#endif                          

#if __LP64__
typedef boolean_t IOInterruptState;
#else
typedef long int IOInterruptState;
#endif

                                              
                                
                                                                                                                                                                                                                                                                                                                                                                                                                                                              
                                        

static __inline__
IOInterruptState
IOSimpleLockLockDisableInterrupt( IOSimpleLock * lock )
{
	IOInterruptState    state = ml_set_interrupts_enabled( false );
	IOSimpleLockLock( lock );
	return state;
}

                                               
                                                               
                                                                                                                                                                                       
                                     
                                                                                       

static __inline__
void
IOSimpleLockUnlockEnableInterrupt( IOSimpleLock * lock,
    IOInterruptState state )
{
	IOSimpleLockUnlock( lock );
	ml_set_interrupts_enabled( state );
}

#ifdef __cplusplus
}                 
#endif

#endif                         
