/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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

#ifndef BSD_KDEBUG_PRIVATE_H
#define BSD_KDEBUG_PRIVATE_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/cdefs.h>
#include <sys/kdebug.h>

__BEGIN_DECLS

#if !KERNEL

#include <Availability.h>

#pragma mark - user space SPI

  
                                                               
                                                                   
         
  
                                                                       
  
                                                                    
                                                                     
                                                                               
                                                                         
                                          
  
                                                             
  
                                                                    
          
  
                                                                   
   
int kdebug_trace(uint32_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3,
    uint64_t arg4)
__OSX_AVAILABLE(10.10) __IOS_AVAILABLE(8.2);

   
                                
  
              
                                                                          
                                                                   
  
                                                                      
  
                                                              
  
                                         
  
                                                                          
  
                             
  
                                                                  
  
                       
  
                                                                
  
                 
                                                                      
                                                  
  
                                                                        
                                                                     
                        
  
                
                                                                
           
  
                                                                      
                                                                    
                                                                   
                                                           
  
             
                                                                       
                             
  
                                                   
                                                                   
                                                           
  
          
                                                                        
                                                                 
                                                            
                         
  
                                 
  
         
                                                                      
                                                    
        
                                                                       
         
                                                               
   
extern uint64_t kdebug_trace_string(uint32_t debugid, uint64_t str_id,
    const char *str)
__OSX_AVAILABLE(10.11) __IOS_AVAILABLE(9.0);

  
                                                                
                                                                  
                                                                   
                                                         
  
                                                      
                           
                           
                                                                                 
    
  
                                                                 
                                                   
   
extern bool kdebug_is_enabled(uint32_t code)
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0)
__WATCHOS_AVAILABLE(3.0) __TVOS_AVAILABLE(10.0);

  
                                                                      
                   
   
extern void *kdebug_typefilter(void)
__OSX_AVAILABLE(10.12) __IOS_AVAILABLE(10.0)
__WATCHOS_AVAILABLE(3.0) __TVOS_AVAILABLE(10.0);

  
                                                                            
             
   
extern bool kdebug_using_continuous_time(void)
__API_AVAILABLE(macos(10.15), ios(13), tvos(13), watchos(6));

#endif              

#pragma mark - private debugids

#define DBG_PPT         36
#define DBG_PERFCTRL    39
#define DBG_CLPC        50
#define DBG_MUSE        52

                                                       
#define DBG_ANS         128
#define DBG_SIO         129
#define DBG_SEP         130
#define DBG_ISP         131
#define DBG_OSCAR       132
#define DBG_EMBEDDEDGFX 133
#define DBG_PMP         134
#define DBG_RTKIT       135

#define MACH_BRIDGE_RCV_TS      0x1                                                        
#define MACH_BRIDGE_REMOTE_TIME 0x2                                     
#define MACH_BRIDGE_RESET_TS    0x3                                                
#define MACH_BRIDGE_TS_PARAMS   0x4                                                    
#define MACH_BRIDGE_SKIP_TS     0x5                         
#define MACH_BRIDGE_TS_MISMATCH 0x6                                                                   
#define MACH_BRIDGE_OBSV_RATE   0x7                                      

                                                                             
#define DBG_SKYWALK_ALWAYSON    0x10
#define DBG_SKYWALK_FLOWSWITCH  0x11
#define DBG_SKYWALK_NETIF       0x12
#define DBG_SKYWALK_CHANNEL     0x13
#define DBG_SKYWALK_PACKET      0x14

#define PPT_TEST            0x01
#define PPT_JETSAM_HIWAT    0x02
#define PPT_JETSAM_TOPPROC  0x03

#define SKYWALKDBG_CODE(SubClass, code) KDBG_CODE(DBG_DLIL, SubClass, code)
#define PPTDBG_CODE(SubClass, code) KDBG_CODE(DBG_PPT, SubClass, code)
#define PERFCTRL_CODE(SubClass, code) KDBG_CODE(DBG_PERFCTRL, SubClass, code)

#if !defined(DRIVERKIT)

extern unsigned int kdebug_enable;

  
                                                                        
           
   
#define KDEBUG_ENABLE_TRACE   (1U << 0)
#define KDEBUG_ENABLE_ENTROPY (1U << 1)               
#define KDEBUG_ENABLE_CHUD    (1U << 2)               
#define KDEBUG_ENABLE_PPT     (1U << 3)               
#define KDEBUG_ENABLE_SERIAL  (1U << 4)               

  
                                                                        
                                                                              
                                                                              
   
#define KDEBUG_ENABLE_CONT_TIME 0x20

#define KDEBUG_TRACE (KDEBUG_ENABLE_TRACE)

  
                                                                                
                                                                      
   
#define KDEBUG_PPT    (KDEBUG_ENABLE_PPT)
#define KDEBUG_COMMON (KDEBUG_ENABLE_TRACE | KDEBUG_ENABLE_PPT)

  
                                                                               
                                                    
  
                                                                        
                                                                                
        
   
#define KDEBUG_LEVEL_NONE     0
#define KDEBUG_LEVEL_IST      1
#define KDEBUG_LEVEL_STANDARD 2
#define KDEBUG_LEVEL_FULL     3

#if NO_KDEBUG
#define KDEBUG_LEVEL KDEBUG_LEVEL_NONE
#elif IST_KDEBUG
#define KDEBUG_LEVEL KDEBUG_LEVEL_IST
#elif KDEBUG
#define KDEBUG_LEVEL KDEBUG_LEVEL_FULL
#else
#define KDEBUG_LEVEL KDEBUG_LEVEL_STANDARD
  
                                                                           
                         
   
#endif

  
                                                                          
   
#ifndef KERNEL_DEBUG
#define KERNEL_DEBUG(...) do { } while (0)
#endif                             

#pragma mark - private definitions

  
                                                                       
             
   
#if defined(__arm64__)
typedef uint64_t kd_buf_argtype;
#else
typedef uintptr_t kd_buf_argtype;
#endif

typedef struct {
	uint64_t timestamp;
	kd_buf_argtype arg1;
	kd_buf_argtype arg2;
	kd_buf_argtype arg3;
	kd_buf_argtype arg4;
	kd_buf_argtype arg5;                    
	uint32_t debugid;
  
                                                                       
             
   
#if defined(__LP64__) || defined(__arm64__)
	uint32_t cpuid;
	kd_buf_argtype unused;
#endif
} kd_buf;

#if defined(__LP64__) || defined(__arm64__)
#define KDBG_TIMESTAMP_MASK             0xffffffffffffffffULL
static inline void
kdbg_set_cpu(kd_buf *kp, int cpu)
{
	kp->cpuid = (unsigned int)cpu;
}
static inline int
kdbg_get_cpu(kd_buf *kp)
{
	return (int)kp->cpuid;
}
static inline void
kdbg_set_timestamp(kd_buf *kp, uint64_t thetime)
{
	kp->timestamp = thetime;
}
static inline uint64_t
kdbg_get_timestamp(kd_buf *kp)
{
	return kp->timestamp;
}
static inline void
kdbg_set_timestamp_and_cpu(kd_buf *kp, uint64_t thetime, int cpu)
{
	kdbg_set_timestamp(kp, thetime);
	kdbg_set_cpu(kp, cpu);
}
#else
#define KDBG_TIMESTAMP_MASK 0x00ffffffffffffffULL
#define KDBG_CPU_MASK       0xff00000000000000ULL
#define KDBG_CPU_SHIFT      56
static inline void
kdbg_set_cpu(kd_buf *kp, int cpu)
{
	kp->timestamp = (kp->timestamp & KDBG_TIMESTAMP_MASK) |
	    (((uint64_t) cpu) << KDBG_CPU_SHIFT);
}
static inline int
kdbg_get_cpu(kd_buf *kp)
{
	return (int) (((kp)->timestamp & KDBG_CPU_MASK) >> KDBG_CPU_SHIFT);
}
static inline void
kdbg_set_timestamp(kd_buf *kp, uint64_t thetime)
{
	kp->timestamp = thetime & KDBG_TIMESTAMP_MASK;
}
static inline uint64_t
kdbg_get_timestamp(kd_buf *kp)
{
	return kp->timestamp & KDBG_TIMESTAMP_MASK;
}
static inline void
kdbg_set_timestamp_and_cpu(kd_buf *kp, uint64_t thetime, int cpu)
{
	kp->timestamp = (thetime & KDBG_TIMESTAMP_MASK) |
	    (((uint64_t) cpu) << KDBG_CPU_SHIFT);
}
#endif

  
                                                                            
   
#define KDBG_TYPEFILTER_BITMAP_SIZE ((256 * 256) / 8)

  
                                                  
   
#define KDBG_INIT       (1U << 0)               
                                           
#define KDBG_NOWRAP     (1U << 1)
#define KDBG_FREERUN    (1U << 2)               
                        
#define KDBG_WRAPPED    (1U << 3)
                                                    
#define KDBG_USERFLAGS  (KDBG_FREERUN | KDBG_NOWRAP | KDBG_INIT)
                                                        
#define KDBG_PIDCHECK   (1U << 4)
                               
#define KDBG_MAPINIT    (1U << 5)
                                                   
#define KDBG_PIDEXCLUDE (1U << 6)
                                             
#define KDBG_LOCKINIT   (1U << 7)
                             
#define KDBG_LP64       (1U << 8)

                                                      

                                      
#define KDBG_RANGECHECK       0x00100000U
                                                                   
#define KDBG_VALCHECK         0x00200000U
                                                     
#define KDBG_TYPEFILTER_CHECK 0x00400000U
                                          
#define KDBG_BUFINIT          0x80000000U

                                           
#define KDBG_CLASSTYPE  0x10000
#define KDBG_SUBCLSTYPE 0x20000
#define KDBG_RANGETYPE  0x40000
#define KDBG_TYPENONE   0x80000
#define KDBG_CKTYPES    0xF0000

typedef struct {
	unsigned int type;
	unsigned int value1;
	unsigned int value2;
	unsigned int value3;
	unsigned int value4;
} kd_regtype;

typedef struct {
	                                                  
	int nkdbufs;
	                              
	int nolog;
	                        
	unsigned int flags;
	                                     
	int nkdthreads;
	                    
	int bufid;
} kbufinfo_t;

typedef struct {
	                   
#if defined(__arm64__)
	uint64_t thread;
#else
	uintptr_t thread;
#endif
	                                                             
	int valid;
	                                               
	char command[20];
} kd_threadmap;

typedef struct {
	uint32_t version_no;
	uint32_t cpu_count;
} kd_cpumap_header;

                  
#define KDBG_CPUMAP_IS_IOP      0x1

typedef struct {
	uint32_t cpu_id;
	uint32_t flags;
	char name[8];
} kd_cpumap;

typedef struct {
	int             version_no;
	int             thread_count;
	uint64_t        TOD_secs;
	uint32_t        TOD_usecs;
} RAW_header;

#define RAW_VERSION0    0x55aa0000
#define RAW_VERSION1    0x55aa0101
#define RAW_VERSION2    0x55aa0200                                         

  
                                        
   
#define KDEBUG_COMMPAGE_ENABLE_TRACE      0x1
#define KDEBUG_COMMPAGE_ENABLE_TYPEFILTER 0x2                                           

#pragma mark - EnergyTracing

                                            
#define kEnTrCompKernel     2

  
                          
  
                                        
                              
   

                                                                     
                                                                        
                                                                         
                                                                        
   
#define kEnTrActKernSocket      1
#define kEnTrActKernSockRead    2
#define kEnTrActKernSockWrite   3

#define kEnTrActKernPoll        10
#define kEnTrActKernSelect      11
#define kEnTrActKernKQWait      12

         
#define kEnTrEvUnblocked        256

                                                           
#define kEnTrFlagNonBlocking    1 << 0
#define kEnTrFlagNoWork         1 << 1

  
                        
   

#if (KDEBUG_LEVEL >= KDEBUG_LEVEL_IST)
                                                     
                                                             
#define ENTR_SHOULDTRACE kdebug_enable
                                                       
#define ENTR_KDTRACE(component, opcode, lifespan, id, quality, value)   \
do {                                                                    \
    uint32_t kdcode__;                                                  \
    uintptr_t highval__, lowval__, mask__ = 0xffffffff;                 \
    kdcode__ = KDBG_CODE(DBG_ENERGYTRACE,component,opcode)|(lifespan);  \
    highval__ = ((value) >> 32) & mask__;                               \
    lowval__ = (value) & mask__;                                        \
    ENTR_KDTRACEFUNC(kdcode__, id, quality, highval__, lowval__);       \
} while(0)

  
                                                       
  
                                                                         
                                                                         
                                                                          
                                                                         
                                                                    
                   
  
                                                           
                                                           
                                                           
                                                           
                                                                  
                                                     
                                                        
  
   
#define kEnTrModAssociate (1 << 28)
#define ENTR_KDASSOCIATE(par_comp, par_opcode, par_act_id,              \
	    sub_comp, sub_opcode, sub_act_id)              \
do {                                                                    \
    unsigned sub_compcode = ((unsigned)sub_comp << 16) | sub_opcode;    \
    ENTR_KDTRACEFUNC(KDBG_CODE(DBG_ENERGYTRACE,par_comp,par_opcode),    \
	             par_act_id, kEnTrModAssociate, sub_compcode,       \
	             sub_act_id);                                       \
} while(0)

#else                                         

#define ENTR_SHOULDTRACE FALSE
#define ENTR_KDTRACE(component, opcode, lifespan, id, quality, value)   \
	                            do {} while (0)
#define ENTR_KDASSOCIATE(par_comp, par_opcode, par_act_id,              \
	    sub_comp, sub_opcode, sub_act_id)              \
	                            do {} while (0)

#endif                                         

#endif                          

__END_DECLS

#endif                                     
