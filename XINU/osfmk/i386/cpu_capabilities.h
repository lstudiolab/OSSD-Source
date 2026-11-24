/*
 * Copyright (c) 2003-2019 Apple Inc. All rights reserved.
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
#ifdef  PRIVATE

#ifndef _I386_CPU_CAPABILITIES_H
#define _I386_CPU_CAPABILITIES_H

#ifndef __ASSEMBLER__
#include <stdint.h>
#endif

  
                                                  
   

                                            

#define kHasMMX                         0x00000001
#define kHasSSE                         0x00000002
#define kHasSSE2                        0x00000004
#define kHasSSE3                        0x00000008
#define kCache32                        0x00000010                                       
#define kCache64                        0x00000020
#define kCache128                       0x00000040
#define kFastThreadLocalStorage         0x00000080                                                            
#define kHasSupplementalSSE3            0x00000100
#define k64Bit                          0x00000200                                                                      
#define kHasSSE4_1                      0x00000400
#define kHasSSE4_2                      0x00000800
#define kHasAES                         0x00001000
#define kInOrderPipeline                0x00002000
#define kSlow                           0x00004000                            
#define kUP                             0x00008000                                  
#define kNumCPUs                        0x00FF0000                                                 
#define kNumCPUsShift                   16
#define kHasAVX1_0                      0x01000000
#define kHasRDRAND                      0x02000000
#define kHasF16C                        0x04000000
#define kHasENFSTRG                     0x08000000
#define kHasFMA                         0x10000000
#define kHasAVX2_0                      0x20000000
#define kHasBMI1                        0x40000000
#define kHasBMI2                        0x80000000
                                       
#define kHasRTM                 0x0000000100000000ULL
#define kHasHLE                 0x0000000200000000ULL
#define kHasRDSEED              0x0000000800000000ULL
#define kHasADX                 0x0000000400000000ULL
#define kHasMPX                 0x0000001000000000ULL
#define kHasSGX                 0x0000002000000000ULL
#define kHasAVX512F             0x0000004000000000ULL
#define kHasAVX512CD            0x0000008000000000ULL
#define kHasAVX512DQ            0x0000010000000000ULL
#define kHasAVX512BW            0x0000020000000000ULL
#define kHasAVX512IFMA          0x0000040000000000ULL
#define kHasAVX512VBMI          0x0000080000000000ULL
#define kHasAVX512VL            0x0000100000000000ULL
#define kHasVAES                0x0000200000000000ULL
#define kHasVPCLMULQDQ          0x0000400000000000ULL
#define kHasAVX512VNNI          0x0000800000000000ULL
#define kHasAVX512BITALG        0x0001000000000000ULL
#define kHasAVX512VPOPCNTDQ     0x0002000000000000ULL

#define kIsTranslated           0x4000000000000000ULL

#ifndef __ASSEMBLER__
#include <sys/cdefs.h>
#include <sys/commpage.h>

__BEGIN_DECLS
extern uint64_t  _get_cpu_capabilities( void );
__END_DECLS

__inline static
int
_NumCPUs( void )
{
	return (int) (_get_cpu_capabilities() & kNumCPUs) >> kNumCPUsShift;
}

#endif                    

                                                                                  
                                                                               
                                                                                    
                                   
   
#define _COMM_PAGE_32_TO_64( ADDRESS )  ( ADDRESS + _COMM_PAGE64_START_ADDRESS - _COMM_PAGE32_START_ADDRESS )


  
                                         
  
                                                                                 
                                                                                      
                                                             
  
                                                                                 
   

#define _COMM_PAGE32_AREA_LENGTH        ( 1 * 4096 )                                                                     
#define _COMM_PAGE32_BASE_ADDRESS       ( 0xffff0000 )                                                                
#define _COMM_PAGE32_START_ADDRESS      ( _COMM_PAGE32_BASE_ADDRESS )                                                    
#define _COMM_PAGE32_AREA_USED          ( 1 * 4096 )                                                                    
#define _COMM_PAGE32_SIGS_OFFSET        0x8000                                                                        

#define _COMM_PAGE64_AREA_LENGTH        ( 1 * 4096 )                                                                   
#ifdef __ASSEMBLER__
#define _COMM_PAGE64_BASE_ADDRESS       ( 0x00007fffffe00000 )                                         
#else                    
#define _COMM_PAGE64_BASE_ADDRESS       ( 0x00007fffffe00000ULL )                                         
#endif                    
#define _COMM_PAGE64_START_ADDRESS      ( _COMM_PAGE64_BASE_ADDRESS )                                                    
#define _COMM_PAGE64_AREA_USED          ( 1 * 4096 )                                                                    

                                              
#define _COMM_PAGE32_OBJC_SIZE          0ULL
#define _COMM_PAGE32_OBJC_BASE          0ULL
#define _COMM_PAGE64_OBJC_SIZE          0ULL
#define _COMM_PAGE64_OBJC_BASE          0ULL

#ifdef KERNEL_PRIVATE

                                                                
                                                            
                                                                
   
#define _COMM_PAGE_START_ADDRESS        _COMM_PAGE32_START_ADDRESS
#define _COMM_PAGE_SIGS_OFFSET          _COMM_PAGE32_SIGS_OFFSET

#else                      

  
                                                          
                                       
   
#if defined(__i386__)

#define _COMM_PAGE_AREA_LENGTH          _COMM_PAGE32_AREA_LENGTH
#define _COMM_PAGE_BASE_ADDRESS         _COMM_PAGE32_BASE_ADDRESS
#define _COMM_PAGE_START_ADDRESS        _COMM_PAGE32_START_ADDRESS
#define _COMM_PAGE_AREA_USED            _COMM_PAGE32_AREA_USED
#define _COMM_PAGE_SIGS_OFFSET          _COMM_PAGE32_SIGS_OFFSET

#elif defined(__x86_64__)

#define _COMM_PAGE_AREA_LENGTH          _COMM_PAGE64_AREA_LENGTH
#define _COMM_PAGE_BASE_ADDRESS         _COMM_PAGE64_BASE_ADDRESS
#define _COMM_PAGE_START_ADDRESS        _COMM_PAGE64_START_ADDRESS
#define _COMM_PAGE_AREA_USED            _COMM_PAGE64_AREA_USED

#else
#error architecture not supported
#endif

#endif                      

                           

#define _COMM_PAGE_SIGNATURE            (_COMM_PAGE_START_ADDRESS+0x000)                                            
#define _COMM_PAGE_SIGNATURELEN         (0x10)
#define _COMM_PAGE_CPU_CAPABILITIES64   (_COMM_PAGE_START_ADDRESS+0x010)                                        
#define _COMM_PAGE_UNUSED               (_COMM_PAGE_START_ADDRESS+0x018)                            
#define _COMM_PAGE_VERSION              (_COMM_PAGE_START_ADDRESS+0x01E)                             
#define _COMM_PAGE_THIS_VERSION         14

#define _COMM_PAGE_CPU_CAPABILITIES     (_COMM_PAGE_START_ADDRESS+0x020)                                                                     
#define _COMM_PAGE_NCPUS                (_COMM_PAGE_START_ADDRESS+0x022)                                                                            
#define _COMM_PAGE_UNUSED0              (_COMM_PAGE_START_ADDRESS+0x024)                                                                                  
#define _COMM_PAGE_CACHE_LINESIZE       (_COMM_PAGE_START_ADDRESS+0x026)                                      

#define _COMM_PAGE_UNUSED4              (_COMM_PAGE_START_ADDRESS+0x028)                                                                                                           
#define _COMM_PAGE_MEMORY_PRESSURE      (_COMM_PAGE_START_ADDRESS+0x02c)                                                 
#define _COMM_PAGE_UNUSED3              (_COMM_PAGE_START_ADDRESS+0x030)                                                                                   

#define _COMM_PAGE_ACTIVE_CPUS          (_COMM_PAGE_START_ADDRESS+0x034)                                                          
#define _COMM_PAGE_PHYSICAL_CPUS        (_COMM_PAGE_START_ADDRESS+0x035)                                                                  
#define _COMM_PAGE_LOGICAL_CPUS         (_COMM_PAGE_START_ADDRESS+0x036)                                                                
#define _COMM_PAGE_UNUSED1              (_COMM_PAGE_START_ADDRESS+0x037)                            
#define _COMM_PAGE_MEMORY_SIZE          (_COMM_PAGE_START_ADDRESS+0x038)                                      

#define _COMM_PAGE_CPUFAMILY            (_COMM_PAGE_START_ADDRESS+0x040)                                       
#define _COMM_PAGE_KDEBUG_ENABLE        (_COMM_PAGE_START_ADDRESS+0x044)                                                          
#define _COMM_PAGE_ATM_DIAGNOSTIC_CONFIG        (_COMM_PAGE_START_ADDRESS+0x48)                                                           

#define _COMM_PAGE_DTRACE_DOF_ENABLED   (_COMM_PAGE_START_ADDRESS+0x04C)                                                              
#define _COMM_PAGE_KERNEL_PAGE_SHIFT    (_COMM_PAGE_START_ADDRESS+0x04D)                                                                   
#define _COMM_PAGE_USER_PAGE_SHIFT_64   (_COMM_PAGE_START_ADDRESS+0x04E)                                                                 
#define _COMM_PAGE_UNUSED2              (_COMM_PAGE_START_ADDRESS+0x04F)                         

#define _COMM_PAGE_TIME_DATA_START      (_COMM_PAGE_START_ADDRESS+0x050)                                                   
#define _COMM_PAGE_NT_TSC_BASE          (_COMM_PAGE_START_ADDRESS+0x050)                                
#define _COMM_PAGE_NT_SCALE             (_COMM_PAGE_START_ADDRESS+0x058)                                
#define _COMM_PAGE_NT_SHIFT             (_COMM_PAGE_START_ADDRESS+0x05c)                                
#define _COMM_PAGE_NT_NS_BASE           (_COMM_PAGE_START_ADDRESS+0x060)                                
#define _COMM_PAGE_NT_GENERATION        (_COMM_PAGE_START_ADDRESS+0x068)                                
#define _COMM_PAGE_GTOD_GENERATION      (_COMM_PAGE_START_ADDRESS+0x06c)                                    
#define _COMM_PAGE_GTOD_NS_BASE         (_COMM_PAGE_START_ADDRESS+0x070)                                    
#define _COMM_PAGE_GTOD_SEC_BASE        (_COMM_PAGE_START_ADDRESS+0x078)                                    

                                                                   
#define _COMM_PAGE_APPROX_TIME          (_COMM_PAGE_START_ADDRESS+0x080)                                             
#define _COMM_PAGE_APPROX_TIME_SUPPORTED (_COMM_PAGE_START_ADDRESS+0x088)                                            

                                                
#define _COMM_PAGE_CONT_TIMEBASE        (_COMM_PAGE_START_ADDRESS+0x0C0)                                            
#define _COMM_PAGE_BOOTTIME_USEC        (_COMM_PAGE_START_ADDRESS+0x0C8)                               
#define _COMM_PAGE_NEWTIMEOFDAY_DATA    (_COMM_PAGE_START_ADDRESS+0x0D0)                                                                                           

                                                
#define _COMM_PAGE_DYLD_FLAGS           (_COMM_PAGE_START_ADDRESS+0x100)                                                                 

#define _COMM_PAGE_END                  (_COMM_PAGE_START_ADDRESS+0xfff)                                

                                                                                                         
                                                       

#define _NT_TSC_BASE                    0
#define _NT_SCALE                       8
#define _NT_SHIFT                       12
#define _NT_NS_BASE                     16
#define _NT_GENERATION                  24
#define _GTOD_GENERATION                28
#define _GTOD_NS_BASE                   32
#define _GTOD_SEC_BASE                  40

                                                                                               
                                                                                               
                                                                                               
                                                                                                       
#define _COMM_PAGE_TEXT_START           (_COMM_PAGE_START_ADDRESS+0x1000)
#define _COMM_PAGE32_TEXT_START         (_COMM_PAGE32_BASE_ADDRESS+0x1000)                               
#define _COMM_PAGE64_TEXT_START         (_COMM_PAGE64_BASE_ADDRESS+0x1000)
#define _COMM_PAGE_TEXT_AREA_USED       ( 1 * 4096 )
#define _COMM_PAGE_TEXT_AREA_LENGTH     ( 1 * 4096 )
#define _PFZ32_SLIDE_RANGE              ( 14 )                                                           
#define _PFZ64_SLIDE_RANGE              ( 510 )                                                                   

                                                                           
                                                                 
   

#define _COMM_TEXT_PREEMPT_OFFSET               (0x5a0)                              
#define _COMM_TEXT_BACKOFF_OFFSET               (0x600)                      
#define _COMM_TEXT_RET_OFFSET                   (0x680)                      
#define _COMM_TEXT_PFZ_START_OFFSET             (0xc00)                                      
#define _COMM_TEXT_PFZ_ENQUEUE_OFFSET           (0xc00)                            
#define _COMM_TEXT_PFZ_DEQUEUE_OFFSET           (0xc80)                            
#define _COMM_TEXT_UNUSED_OFFSET                (0xd00)                                   
#define _COMM_TEXT_PFZ_END_OFFSET               (0xd00)                            


#define _COMM_PAGE_PREEMPT              (_COMM_PAGE_TEXT_START+_COMM_TEXT_PREEMPT_OFFSET)
#define _COMM_PAGE_BACKOFF              (_COMM_PAGE_TEXT_START+_COMM_TEXT_BACKOFF_OFFSET)
#define _COMM_PAGE_RET                  (_COMM_PAGE_TEXT_START+_COMM_TEXT_RET_OFFSET)

#define _COMM_PAGE_PFZ_START            (_COMM_PAGE_TEXT_START+_COMM_PAGE_PFZ_START_OFFSET)

#define _COMM_PAGE_PFZ_ENQUEUE          (_COMM_PAGE_TEXT_START+_COMM_TEXT_PFZ_ENQUEUE_OFFSET)
#define _COMM_PAGE_PFZ_DEQUEUE          (_COMM_PAGE_TEXT_START+_COMM_TEXT_PFZ_DEQUEUE_OFFSET)

#define _COMM_PAGE_UNUSED6              (_COMM_PAGE_TEXT_START+_COMM_TEXT_UNUSED_OFFSET)
#define _COMM_PAGE_PFZ_END              (_COMM_PAGE_TEXT_START+_COMM_TEXT_PFZ_END_OFFSET)
#define _COMM_PAGE_TEXT_END             (_COMM_PAGE_TEXT_START+_COMM_TEXT_PFZ_END_OFFSET)                              

                                                                  
                                                                      
                                              
#define _COMM_PAGE_COMPARE_AND_SWAP32B  (_COMM_PAGE_START_ADDRESS+0xf80)                                             
#define _COMM_PAGE_COMPARE_AND_SWAP64B  (_COMM_PAGE_START_ADDRESS+0xfc0)                                                   

  
                                                                                     
                                                                    
   
#define _COMM_PAGE_USER_PAGE_SHIFT_32 _COMM_PAGE_USER_PAGE_SHIFT_64

#ifdef __ASSEMBLER__
#ifdef __COMM_PAGE_SYMBOLS

#define CREATE_COMM_PAGE_SYMBOL(symbol_name, symbol_address)            \
	                        .org	(symbol_address - (_COMM_PAGE_START_ADDRESS & 0xFFFFE000)) ;\
symbol_name: nop

.text
                                                 

CREATE_COMM_PAGE_SYMBOL(___preempt, _COMM_PAGE_PREEMPT)
CREATE_COMM_PAGE_SYMBOL(___backoff, _COMM_PAGE_BACKOFF)
CREATE_COMM_PAGE_SYMBOL(___pfz_enqueue, _COMM_PAGE_PFZ_ENQUEUE)
CREATE_COMM_PAGE_SYMBOL(___pfz_dequeue, _COMM_PAGE_PFZ_DEQUEUE)
CREATE_COMM_PAGE_SYMBOL(___end_comm_page, _COMM_PAGE_END)

.data                                                                    
.long   0                                                                

#endif                          
#endif                    

#endif                               
#endif              
