/*
 * Copyright (c) 2003-2010 Apple Inc. All rights reserved.
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
#ifdef KERNEL_PRIVATE
#ifndef _I386_CPU_TOPOLOGY_H_
#define _I386_CPU_TOPOLOGY_H_

  
                                                                       
                                                                             
                                                                      
                                                             
  
                                                              
                                                                   
   

  
                                                                    
   
typedef struct x86_cpu_cache {
	struct x86_cpu_cache *next;                                        
	struct x86_die      *die;                                                     
	uint8_t             maxcpus;                                          
	uint8_t             nlcpus;                                               
	uint8_t             type;                          
	uint8_t             level;                          
	uint16_t            ways;                               
	uint16_t            partitions;                               
	uint16_t            line_size;                            
	uint32_t            cache_size;                          
	struct x86_lcpu     *cpus[0];                                
} x86_cpu_cache_t;

#define CPU_CACHE_TYPE_DATA     1                       
#define CPU_CACHE_TYPE_INST     2                              
#define CPU_CACHE_TYPE_UNIF     3                          

#define CPU_CACHE_DEPTH_L1      0
#define CPU_CACHE_DEPTH_L2      1
#define CPU_CACHE_DEPTH_L3      2

#define MAX_CACHE_DEPTH         3                          

struct pmc;
struct cpu_data;
struct mca_state;

  
                                                    
  
                                                                     
                                                              
                                                      
  
                                                                        
                                                                
                                                   
  
                                                                    
                                                              
                                
  
                                                                       
                                  
  
                                                                    
                                                        
  
                                                                            
  
                                                                         
                          
   
typedef enum lcpu_state {
	LCPU_OFF            = 0,                                          
	LCPU_HALT           = 1,
	LCPU_NONSCHED       = 2,
	LCPU_PAUSE          = 3,
	LCPU_IDLE           = 4,
	LCPU_RUN            = 5,
} lcpu_state_t;

  
                                                                           
                   
  
                                                         
                                                                             
                                                                           
                                                                             
                                    
  
                                                                             
                                              
  
                                                                             
                                                                                
   
typedef struct x86_lcpu {
	struct x86_lcpu     *next_in_core;                              
	struct x86_lcpu     *next_in_die;                             
	struct x86_lcpu     *next_in_pkg;                                 
	struct x86_lcpu     *lcpu;                                
	struct x86_core     *core;                                           
	struct x86_die      *die;                                           
	struct x86_pkg      *package;                                           
	struct cpu_data     *cpu;                               
	uint32_t            flags;
	uint32_t            cpu_num;                    
	uint32_t            lnum;                                             
	uint32_t            pnum;                                
	boolean_t           master;                                               
	boolean_t           primary;                                               
	volatile lcpu_state_t       state;                              
	volatile boolean_t  stopped;                                                     
	uint64_t            rtcPop;                                    
	uint64_t            rtcDeadline;                                    
	x86_cpu_cache_t     *caches[MAX_CACHE_DEPTH];
	void                *pmStats;                                        
	void                *pmState;                                        
} x86_lcpu_t;

#define X86CORE_FL_PRESENT      0x80000000                           
#define X86CORE_FL_READY        0x40000000                                 
#define X86CORE_FL_HAS_HPET     0x10000000                                  
#define X86CORE_FL_HALTED       0x00008000                          
#define X86CORE_FL_IDLE         0x00004000                        

typedef struct x86_core {
	struct x86_core     *next_in_die;                      
	struct x86_core     *next_in_pkg;                          
	struct x86_die      *die;                                    
	struct x86_pkg      *package;                                
	struct x86_lcpu     *lcpus;                                       
	uint32_t            flags;
	uint32_t            lcore_num;                                          
	uint32_t            pcore_num;                                         
	uint32_t            num_lcpus;                              
	uint32_t            active_lcpus;                                    
	void                *pmStats;                                        
	void                *pmState;                                        
} x86_core_t;

#define X86DIE_FL_PRESENT       0x80000000                          
#define X86DIE_FL_READY         0x40000000                                

typedef struct x86_die {
	struct x86_die      *next_in_pkg;                         
	struct x86_lcpu     *lcpus;                               
	struct x86_core     *cores;                               
	struct x86_pkg      *package;                                   
	uint32_t            flags;
	uint32_t            ldie_num;                                          
	uint32_t            pdie_num;                                         
	uint32_t            num_cores;                              
	x86_cpu_cache_t     *LLC;                                      
	void                *pmStats;                                       
	void                *pmState;                                       
} x86_die_t;

#define X86PKG_FL_PRESENT       0x80000000                              
#define X86PKG_FL_READY         0x40000000                                 
#define X86PKG_FL_HAS_HPET      0x10000000                                     
#define X86PKG_FL_HALTED        0x00008000                             
#define X86PKG_FL_IDLE          0x00004000                           

typedef struct x86_pkg {
	struct x86_pkg      *next;                        
	struct x86_lcpu     *lcpus;                                          
	struct x86_core     *cores;                                   
	struct x86_die      *dies;                                   
	uint32_t            flags;
	uint32_t            lpkg_num;                          
	uint32_t            ppkg_num;                           
	uint32_t            num_dies;                                  
	void                *pmStats;                                          
	void                *pmState;                                          
	struct mca_state    *mca_state;                                  
	uint64_t            package_idle_exits;
	uint32_t            num_idle;
} x86_pkg_t;

extern x86_pkg_t        *x86_pkgs;                                    

typedef struct x86_topology_parameters {
	uint32_t            LLCDepth;
	uint32_t            nCoresSharingLLC;
	uint32_t            nLCPUsSharingLLC;
	uint32_t            maxSharingLLC;
	uint32_t            nLThreadsPerCore;
	uint32_t            nPThreadsPerCore;
	uint32_t            nLCoresPerDie;
	uint32_t            nPCoresPerDie;
	uint32_t            nLDiesPerPackage;
	uint32_t            nPDiesPerPackage;
	uint32_t            nLThreadsPerDie;
	uint32_t            nPThreadsPerDie;
	uint32_t            nLThreadsPerPackage;
	uint32_t            nPThreadsPerPackage;
	uint32_t            nLCoresPerPackage;
	uint32_t            nPCoresPerPackage;
	uint32_t            nPackages;
	boolean_t           stable;
} x86_topology_parameters_t;

                                
extern void             cpu_topology_sort(int ncpus);
extern kern_return_t    cpu_topology_start_cpu(int cpunum);


#endif                            
#endif                     
