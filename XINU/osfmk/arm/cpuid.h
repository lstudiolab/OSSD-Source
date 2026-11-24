/*
 * Copyright (c) 2007-2016 Apple Inc. All rights reserved.
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
 * @OSF_COPYRIGHT@
 */

  
                         
   

#ifndef _MACHINE_CPUID_H_
#define _MACHINE_CPUID_H_

#include <stdint.h>
#include <mach/boolean.h>
#include <machine/machine_cpuid.h>

typedef struct {
	uint32_t arm_rev : 4,                             
	    arm_part         : 12,                               
	    arm_arch         : 4,                        
	    arm_variant      : 4,                    
	    arm_implementor  : 8;                              
} arm_cpuid_bits_t;

typedef union {
	arm_cpuid_bits_t arm_info;                                             
	uint32_t         value;
} arm_cpu_info_t;

                       
#define CPU_VID_ARM      0x41               
#define CPU_VID_DEC      0x44                                 
#define CPU_VID_MOTOROLA 0x4D                                           
#define CPU_VID_MARVELL  0x56                              
#define CPU_VID_INTEL    0x69                    
#define CPU_VID_APPLE    0x61              


                            

#define CPU_ARCH_ARMv4    0x1            
#define CPU_ARCH_ARMv4T   0x2                    
#define CPU_ARCH_ARMv5    0x3            
#define CPU_ARCH_ARMv5T   0x4                    
#define CPU_ARCH_ARMv5TE  0x5                                    
#define CPU_ARCH_ARMv5TEJ 0x6                                                       
#define CPU_ARCH_ARMv6    0x7            
#define CPU_ARCH_ARMv7    0x8            
#define CPU_ARCH_ARMv7f   0x9                          
#define CPU_ARCH_ARMv7s   0xa                      
#define CPU_ARCH_ARMv7k   0xb                          

#define CPU_ARCH_ARMv8    0xc                                 

#define CPU_ARCH_ARMv8E   0xd                                                              

                                                                                         
#define CPU_ARCH_EXTENDED 0xF

                      
  
                
                                              
   

                                
#define CPU_PART_920T               0x920
#define CPU_PART_926EJS             0x926                 

                                 
#define CPU_PART_1136JFS            0xB36                                
#define CPU_PART_1176JZFS           0xB76                   

                             
#define CPU_PART_CORTEXA5           0xC05

                             
#define CPU_PART_CORTEXA7           0xC07

                                
#define CPU_PART_CORTEXA8           0xC08

                             
#define CPU_PART_CORTEXA9           0xC09

                             
#define CPU_PART_TYPHOON            0x2

                              
#define CPU_PART_TYPHOON_CAPRI      0x3

                             
#define CPU_PART_TWISTER            0x4

                                  
#define CPU_PART_TWISTER_ELBA_MALTA 0x5

                             
#define CPU_PART_HURRICANE          0x6

                              
#define CPU_PART_HURRICANE_MYST     0x7

                                     
#define CPU_PART_MONSOON            0x8

                                     
#define CPU_PART_MISTRAL            0x9

                                     
#define CPU_PART_VORTEX             0xB

                                     
#define CPU_PART_TEMPEST            0xC

                                    
#define CPU_PART_TEMPEST_M9         0xF

                                      
#define CPU_PART_VORTEX_ARUBA       0x10

                                      
#define CPU_PART_TEMPEST_ARUBA      0x11

                                     
#define CPU_PART_LIGHTNING          0x12

                                     
#define CPU_PART_THUNDER            0x13

#ifndef RC_HIDE_XNU_FIRESTORM
  
                                         
                                         
                        
   
                                     
#define CPU_PART_THUNDER_M10        0x26
#endif

#ifndef RC_HIDE_XNU_FIRESTORM

                
#define CPU_PART_ICESTORM           0x20

                
#define CPU_PART_FIRESTORM          0x21

                 
#define CPU_PART_ICESTORM_TONGA     0x22

                 
#define CPU_PART_FIRESTORM_TONGA    0x23

#endif                             



                               

                           
typedef enum {
	CACHE_WRITE_THROUGH,
	CACHE_WRITE_BACK,
	CACHE_READ_ALLOCATION,
	CACHE_WRITE_ALLOCATION,
	CACHE_UNKNOWN
} cache_type_t;

typedef struct {
	boolean_t    c_unified;                                    
	uint32_t     c_isize;                                                     
	boolean_t    c_i_ppage;                                                   
                                                                              
	uint32_t     c_dsize;                                                     
	boolean_t    c_d_ppage;                                                   
                                                                              
	cache_type_t c_type;                           
	uint32_t     c_linesz;                                
	uint32_t     c_assoc;                                     
	uint32_t     c_l2size;                                    
	uint32_t     c_bulksize_op;                                                    
	uint32_t     c_inner_cache_size;                       
} cache_info_t;

typedef struct {
	uint32_t
	    RB:4,                                                         
	    SP:4,                                                     
	    DP:4,                                                      
	    TE:4,                                                                     
	    D:4,                                                 
	    SR:4,                                                      
	    SV:4,                                              
	    RM:4;                                                    
} arm_mvfr0_t;

typedef union {
	arm_mvfr0_t bits;
	uint32_t    value;
} arm_mvfr0_info_t;

typedef struct {
	uint32_t
	    FZ:4,                                                            
	    DN:4,                                                             
	    LS:4,                                                             
	    I:4,                                                           
	    SP:4,                                                                                   
	    HPFP:4,                                                                   
	    RSVP:8;                       
} arm_mvfr1_t;

typedef union {
	arm_mvfr1_t bits;
	uint32_t    value;
} arm_mvfr1_info_t;

typedef struct {
	uint32_t neon;
	uint32_t neon_hpfp;
	uint32_t neon_fp16;
} arm_mvfp_info_t;

#ifdef __cplusplus
extern "C" {
#endif                  

extern void do_cpuid(void);
extern arm_cpu_info_t *cpuid_info(void);
extern int cpuid_get_cpufamily(void);
extern int cpuid_get_cpusubfamily(void);

extern void do_debugid(void);
extern arm_debug_info_t *arm_debug_info(void);

extern void do_cacheid(void);
extern cache_info_t *cache_info(void);

extern void do_mvfpid(void);
extern arm_mvfp_info_t *arm_mvfp_info(void);

#ifdef __cplusplus
}
#endif                  

#endif                     
