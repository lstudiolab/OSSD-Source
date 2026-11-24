/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
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
#ifdef  PRIVATE

#ifndef _ARM_CPU_CAPABILITIES_H
#define _ARM_CPU_CAPABILITIES_H

#ifndef __ASSEMBLER__
#include <stdint.h>
#include <mach/vm_types.h>
#endif


#define USER_TIMEBASE_NONE   0
#define USER_TIMEBASE_SPEC   1

  
                                                                 
                                                            
                                                  
  
   

  
                                         
   
#define kHasNeonFP16                    0x00000008                                     
#define kCache32                        0x00000010                                    
#define kCache64                        0x00000020                                    
#define kCache128                       0x00000040                                     
#define kFastThreadLocalStorage         0x00000080                                                         
#define kHasNeon                        0x00000100                                   
#define kHasNeonHPFP                    0x00000200                                     
#define kHasVfp                         0x00000400                         
#define kHasUCNormalMemory              0x00000800                                                 
#define kHasEvent                       0x00001000                                        
#define kHasFMA                         0x00002000                                        
#define kHasARMv82FHM                   0x00004000                                                                        
#define kUP                             0x00008000                               
#define kNumCPUs                        0x00FF0000                                              
#define kHasARMv8Crypto                 0x01000000                                         
#define kHasARMv81Atomics               0x02000000                                              
#define kHasARMv8Crc32                  0x04000000                                                                
#define kHasARMv82SHA512                0x80000000                                             
                                       
#define kHasARMv82SHA3          0x0000000100000000                                           

#define kNumCPUsShift                   16                                     
  
                                        
   
#define kIsMultiUserDevice      0x80000000                                         
#define kMultiUserCurrentUserMask       0x7fffffff                                                     

#ifndef __ASSEMBLER__
#include <sys/commpage.h>

__BEGIN_DECLS
extern uint64_t _get_cpu_capabilities( void );
__END_DECLS

__inline static
int
_NumCPUs( void )
{
	return (_get_cpu_capabilities() & kNumCPUs) >> kNumCPUsShift;
}


typedef struct {
	volatile uint64_t       TimeBase;
	volatile uint32_t       TimeStamp_sec;
	volatile uint32_t       TimeStamp_usec;
	volatile uint32_t       TimeBaseTicks_per_sec;
	volatile uint32_t       TimeBaseTicks_per_usec;
	volatile uint64_t       TimeBase_magic;
	volatile uint32_t       TimeBase_add;
	volatile uint32_t       TimeBase_shift;
} commpage_timeofday_data_t;

__BEGIN_DECLS
extern vm_address_t                             _get_commpage_priv_address(void);
extern vm_address_t                             _get_commpage_text_priv_address(void);
__END_DECLS

#endif                    


  
                                         
   

#if defined(__LP64__)

#define _COMM_PAGE64_BASE_ADDRESS               (0x0000000FFFFFC000ULL)               
#if defined(ARM_LARGE_MEMORY)
#define _COMM_HIGH_PAGE64_BASE_ADDRESS  (0xFFFFFE00001FC000ULL)                                                                    
#else
#define _COMM_HIGH_PAGE64_BASE_ADDRESS  (0xFFFFFFF0001FC000ULL)                                                                    
#endif

#define _COMM_PAGE64_AREA_LENGTH                (_COMM_PAGE32_AREA_LENGTH)
#define _COMM_PAGE64_AREA_USED                  (-1)

#define _COMM_PAGE_PRIV(_addr_)                 ((_addr_) - (_COMM_PAGE_START_ADDRESS) + _get_commpage_priv_address())

#ifdef KERNEL_PRIVATE
#define _COMM_PAGE_RW_OFFSET                    (0)
#define _COMM_PAGE_AREA_LENGTH                  (PAGE_SIZE)

#define _COMM_PAGE_BASE_ADDRESS                 (_get_commpage_priv_address())
#define _COMM_PAGE_START_ADDRESS                (_get_commpage_priv_address())

   
                                                                                
                                                                                
                                                                             
                                       
  
                                                                           
                                                                           
                                                                              
                                                                         
  
                                                                           
           
   
#define _COMM_PAGE64_NESTING_START                (0x0000000FC0000000ULL)
#define _COMM_PAGE64_NESTING_SIZE                 (0x40000000ULL)           
_Static_assert((_COMM_PAGE64_BASE_ADDRESS >= _COMM_PAGE64_NESTING_START) &&
    (_COMM_PAGE64_BASE_ADDRESS < (_COMM_PAGE64_NESTING_START + _COMM_PAGE64_NESTING_SIZE)),
    "_COMM_PAGE64_BASE_ADDRESS is not within the nesting region. Commpage nesting "
    "region probably needs to be updated.");

#else                     
  
                                                          
                                       
   
#define _COMM_PAGE_AREA_LENGTH                  (4096)

#define _COMM_PAGE_BASE_ADDRESS                 _COMM_PAGE64_BASE_ADDRESS
#define _COMM_PAGE_START_ADDRESS                _COMM_PAGE64_BASE_ADDRESS
#endif                     

#else               

#define _COMM_PAGE64_BASE_ADDRESS               (-1)
#define _COMM_PAGE64_AREA_LENGTH                (-1)
#define _COMM_PAGE64_AREA_USED                  (-1)

                                                                                          
                                                            
                                                   
#define _COMM_PAGE_PRIV(_addr_)                 (_addr_)

#ifdef KERNEL_PRIVATE
#define _COMM_PAGE_RW_OFFSET                    (_get_commpage_priv_address()-_COMM_PAGE_BASE_ADDRESS)
#define _COMM_PAGE_AREA_LENGTH                  (PAGE_SIZE)
#else                     
#define _COMM_PAGE_AREA_LENGTH                  (4096)
#endif                     

#define _COMM_PAGE_BASE_ADDRESS                 _COMM_PAGE32_BASE_ADDRESS
#define _COMM_PAGE_START_ADDRESS                _COMM_PAGE32_BASE_ADDRESS

#endif               

#define _COMM_PAGE32_BASE_ADDRESS               (0xFFFF4000)                                                      
#define _COMM_PAGE32_AREA_LENGTH                (_COMM_PAGE_AREA_LENGTH)
#define _COMM_PAGE32_TEXT_START                 (-1)

#define _COMM_PAGE32_OBJC_SIZE                  0ULL
#define _COMM_PAGE32_OBJC_BASE                  0ULL
#define _COMM_PAGE64_OBJC_SIZE                  0ULL
#define _COMM_PAGE64_OBJC_BASE                  0ULL

  
                         
                                                              
   
#define _COMM_PAGE_SIGNATURE                    (_COMM_PAGE_START_ADDRESS+0x000)                                          
#define _COMM_PAGE_SIGNATURELEN                 (0x10)
#define _COMM_PAGE_CPU_CAPABILITIES64           (_COMM_PAGE_START_ADDRESS+0x010)                                        
#define _COMM_PAGE_UNUSED                       (_COMM_PAGE_START_ADDRESS+0x018)                            
#define _COMM_PAGE_VERSION                      (_COMM_PAGE_START_ADDRESS+0x01E)                          
#define _COMM_PAGE_THIS_VERSION                 3                                                                        

#define _COMM_PAGE_CPU_CAPABILITIES             (_COMM_PAGE_START_ADDRESS+0x020)                                     
#define _COMM_PAGE_NCPUS                        (_COMM_PAGE_START_ADDRESS+0x022)                                            
#define _COMM_PAGE_USER_PAGE_SHIFT_32           (_COMM_PAGE_START_ADDRESS+0x024)                                             
#define _COMM_PAGE_USER_PAGE_SHIFT_64           (_COMM_PAGE_START_ADDRESS+0x025)                                             
#define _COMM_PAGE_CACHE_LINESIZE               (_COMM_PAGE_START_ADDRESS+0x026)                                   
#define _COMM_PAGE_UNUSED4                      (_COMM_PAGE_START_ADDRESS+0x028)                                                                                                        
#define _COMM_PAGE_UNUSED3                      (_COMM_PAGE_START_ADDRESS+0x02C)                                                                                
#define _COMM_PAGE_MEMORY_PRESSURE              (_COMM_PAGE_START_ADDRESS+0x030)                                              
#define _COMM_PAGE_ACTIVE_CPUS                  (_COMM_PAGE_START_ADDRESS+0x034)                                                       
#define _COMM_PAGE_PHYSICAL_CPUS                (_COMM_PAGE_START_ADDRESS+0x035)                                                               
#define _COMM_PAGE_LOGICAL_CPUS                 (_COMM_PAGE_START_ADDRESS+0x036)                                                             
#define _COMM_PAGE_KERNEL_PAGE_SHIFT            (_COMM_PAGE_START_ADDRESS+0x037)                                          
#define _COMM_PAGE_MEMORY_SIZE                  (_COMM_PAGE_START_ADDRESS+0x038)                                      
#define _COMM_PAGE_TIMEOFDAY_DATA               (_COMM_PAGE_START_ADDRESS+0x040)                                                                                                                                                                                  
#define _COMM_PAGE_CPUFAMILY                    (_COMM_PAGE_START_ADDRESS+0x080)                                    
#define _COMM_PAGE_DEV_FIRM                     (_COMM_PAGE_START_ADDRESS+0x084)                                                   
#define _COMM_PAGE_TIMEBASE_OFFSET              (_COMM_PAGE_START_ADDRESS+0x088)                                                                         
#define _COMM_PAGE_USER_TIMEBASE                (_COMM_PAGE_START_ADDRESS+0x090)                                                                                    
#define _COMM_PAGE_CONT_HWCLOCK                 (_COMM_PAGE_START_ADDRESS+0x091)                                                                                 
#define _COMM_PAGE_DTRACE_DOF_ENABLED           (_COMM_PAGE_START_ADDRESS+0x092)                                                           
#define _COMM_PAGE_UNUSED0                      (_COMM_PAGE_START_ADDRESS+0x093)                         
#define _COMM_PAGE_CONT_TIMEBASE                (_COMM_PAGE_START_ADDRESS+0x098)                                                                                    
#define _COMM_PAGE_BOOTTIME_USEC                (_COMM_PAGE_START_ADDRESS+0x0A0)                                            
#define _COMM_PAGE_CONT_HW_TIMEBASE             (_COMM_PAGE_START_ADDRESS+0x0A8)                                                                         

                                        
#define _COMM_PAGE_APPROX_TIME                  (_COMM_PAGE_START_ADDRESS+0x0C0)                                                   
#define _COMM_PAGE_APPROX_TIME_SUPPORTED        (_COMM_PAGE_START_ADDRESS+0x0C8)                                                     
#define _COMM_PAGE_UNUSED1                      (_COMM_PAGE_START_ADDRESS+0x0C9)                                                                             

#define _COMM_PAGE_KDEBUG_ENABLE                (_COMM_PAGE_START_ADDRESS+0x100)                                                          
#define _COMM_PAGE_ATM_DIAGNOSTIC_CONFIG        (_COMM_PAGE_START_ADDRESS+0x104)                                                               
#define _COMM_PAGE_MULTIUSER_CONFIG             (_COMM_PAGE_START_ADDRESS+0x108)                                                          


#define _COMM_PAGE_NEWTIMEOFDAY_DATA            (_COMM_PAGE_START_ADDRESS+0x120)                                                                                         
#define _COMM_PAGE_REMOTETIME_PARAMS            (_COMM_PAGE_START_ADDRESS+0x148)                                                                                      
#define _COMM_PAGE_DYLD_FLAGS                   (_COMM_PAGE_START_ADDRESS+0x160)                                                              

                                                  
#define _COMM_PAGE_CPU_QUIESCENT_COUNTER        (_COMM_PAGE_START_ADDRESS+0x180)                                                           

#define _COMM_PAGE_END                          (_COMM_PAGE_START_ADDRESS+0xfff)                             

#if defined(__LP64__)
#if KERNEL_PRIVATE
#define _COMM_PAGE64_TEXT_START_ADDRESS          (_get_commpage_text_priv_address())                                          
#endif
                                                                                
                                                   
#define _COMM_PAGE_TEXT_ATOMIC_ENQUEUE                  (0x0)
#define _COMM_PAGE_TEXT_ATOMIC_DEQUEUE                  (0x4)

#else               
                           
#endif               

#endif                              
#endif              
