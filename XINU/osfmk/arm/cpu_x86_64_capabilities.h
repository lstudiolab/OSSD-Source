/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
 */

#ifdef  PRIVATE

#ifndef _ARM_CPU_X86_64_CAPABILITIES_H
#define _ARM_CPU_X86_64_CAPABILITIES_H

#ifndef __ASSEMBLER__
#include <stdint.h>
#include <sys/commpage.h>
#ifdef KERNEL_PRIVATE
#include <mach/vm_types.h>
#endif
#endif

  
                                                                        
                                                            
  
                                                  
   

                                                     

#define x86_64_kHasMMX                  0x00000001
#define x86_64_kHasSSE                  0x00000002
#define x86_64_kHasSSE2                 0x00000004
#define x86_64_kHasSSE3                 0x00000008
#define x86_64_kCache32                 0x00000010                                       
#define x86_64_kCache64                 0x00000020
#define x86_64_kCache128                0x00000040
#define x86_64_kFastThreadLocalStorage  0x00000080                                                            
#define x86_64_kHasSupplementalSSE3             0x00000100
#define x86_64_k64Bit                   0x00000200                                                                      
#define x86_64_kHasSSE4_1               0x00000400
#define x86_64_kHasSSE4_2               0x00000800
#define x86_64_kHasAES                  0x00001000
#define x86_64_kInOrderPipeline 0x00002000
#define x86_64_kSlow                    0x00004000                            
#define x86_64_kUP                              0x00008000                                  
#define x86_64_kNumCPUs                 0x00FF0000                                                 
#define x86_64_kNumCPUsShift    16
#define x86_64_kHasAVX1_0               0x01000000
#define x86_64_kHasRDRAND               0x02000000
#define x86_64_kHasF16C                 0x04000000
#define x86_64_kHasENFSTRG              0x08000000
#define x86_64_kHasFMA                  0x10000000
#define x86_64_kHasAVX2_0               0x20000000
#define x86_64_kHasBMI1                 0x40000000
#define x86_64_kHasBMI2                 0x80000000
                                       
#define x86_64_kHasRTM                  0x0000000100000000ULL
#define x86_64_kHasHLE                  0x0000000200000000ULL
#define x86_64_kHasRDSEED               0x0000000800000000ULL
#define x86_64_kHasADX                  0x0000000400000000ULL
#define x86_64_kHasMPX                  0x0000001000000000ULL
#define x86_64_kHasSGX                  0x0000002000000000ULL
#if !defined(RC_HIDE_XNU_J137)
#define x86_64_kHasAVX512F              0x0000004000000000ULL
#define x86_64_kHasAVX512CD             0x0000008000000000ULL
#define x86_64_kHasAVX512DQ             0x0000010000000000ULL
#define x86_64_kHasAVX512BW             0x0000020000000000ULL
#define x86_64_kHasAVX512IFMA   0x0000040000000000ULL
#define x86_64_kHasAVX512VBMI   0x0000080000000000ULL
#define x86_64_kHasAVX512VL             0x0000100000000000ULL
#endif                           

#define x86_64_kIsTranslated    0x4000000000000000ULL                  
                                                     
#define x86_64_kVmPageShift     0xFFB

  
                                                                 
                                       
   

#define X86_64_COMM_PAGE_BASE_ADDRESS           (0x7fffffe00000ULL)
#define X86_64_COMM_PAGE_AREA_LENGTH            4096
#define X86_64_COMM_PAGE_VERSION                        14
#define X86_64_MP_SPIN_TRIES                    1000

#ifdef KERNEL_PRIVATE
extern vm_address_t x86_64_sharedpage_rw_addr;
extern uint64_t _get_x86_64_cpu_capabilities(void);
#endif

typedef struct {
         uint8_t signature[16];
         uint64_t cpu_capabilities64;
         uint8_t _unused[6];
         uint16_t version;
         uint32_t cpu_capabilities;
         uint8_t _unused0[2];
         uint16_t cache_linesize;
         volatile uint32_t sched_gen;
         volatile uint32_t memory_pressure;
         volatile uint32_t spin_count;
         volatile uint8_t active_cpus;
         uint8_t physical_cpus;
         uint8_t logical_cpus;
         uint8_t _unused1[1];
         uint64_t memory_size;
         uint32_t cpufamily;
         volatile uint32_t kdebug_enable;
         volatile uint32_t atm_diagnostic_config;
         uint8_t cp_dtrace_dof_enabled;
         uint8_t cp_kernel_page_shift;                               
         uint8_t cp_user_page_shift;                               
         uint8_t _unused2;
	volatile struct {
         uint64_t nt_tsc_base;
         uint32_t nt_scale;
         uint32_t nt_shift;
         uint64_t nt_ns_base;
         uint32_t nt_generation;
         uint32_t gtod_generation;
         uint64_t gtod_ns_base;
         uint64_t gtod_sec_base;
	} time_data;
	volatile union {
		struct {
         uint64_t time;
         uint64_t time_supported;
		} _;
		uint8_t _fill[64];
	} approx;
         volatile uint64_t cont_timebase;
         volatile uint64_t boottime_usec;
	new_commpage_timeofday_data_t new_time_data;
                
                                   
                                   
                                   
                                 
                                   
                

         uint64_t unused;
          uint64_t dyld_system_flags;

          uint8_t unused2[3800];
            uint8_t cp_aprr_shadow_supported;
            uint8_t unused3[7];
            uint64_t cp_aprr_shadow_jit_rw;
           uint64_t cp_aprr_shadow_jit_rx;
            uint32_t unused4;
          uint32_t arm_cpufamily;
} x86_64_commpage_t;

#endif                                     
#endif              
