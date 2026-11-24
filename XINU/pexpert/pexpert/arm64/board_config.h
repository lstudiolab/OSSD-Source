/*
 * Copyright (c) 2007-2020 Apple Inc. All rights reserved.
 * Copyright (c) 2005-2006 Apple Computer, Inc. All rights reserved.
 */
#ifndef _PEXPERT_ARM_BOARD_CONFIG_H
#define _PEXPERT_ARM_BOARD_CONFIG_H

#include <mach/machine.h>

  
                                            
  
           
                    
                                     
               
                                                 
  
                                                                   
  
                                                                              
  
                                                                            
                                                                     
                       
   


#ifdef ARM64_BOARD_CONFIG_T7000
#include <pexpert/arm64/H7.h>

#define MAX_L2_CLINE                   6
#define MAX_CPUS                       3
#define MAX_CPU_CLUSTERS               1
#endif                                

#ifdef ARM64_BOARD_CONFIG_T7001
#include <pexpert/arm64/H7.h>

#define MAX_L2_CLINE                   6
#define MAX_CPUS                       3
#define MAX_CPU_CLUSTERS               1
#endif                                

#ifdef ARM64_BOARD_CONFIG_S8000
#include <pexpert/arm64/H8.h>
#define MAX_CPUS                       2
#define MAX_CPU_CLUSTERS               1
  
                                                                   
                                                                     
                                                                 
           
   
#define MAX_L2_CLINE                   6
#endif                                

#ifdef ARM64_BOARD_CONFIG_S8001
#include <pexpert/arm64/H8.h>
#define MAX_CPUS                       2
#define MAX_CPU_CLUSTERS               1
  
                                                                   
                                                                     
                                                               
           
   
#define MAX_L2_CLINE                   6
#endif                                

#ifdef ARM64_BOARD_CONFIG_T8010
#include <pexpert/arm64/H9.h>
#define MAX_CPUS                       3
#define MAX_CPU_CLUSTERS               1
  
                                                                            
                                                                     
                                                               
           
   
#define MAX_L2_CLINE                   7

#if DEVELOPMENT || DEBUG
#define PMAP_CS                        1
#define PMAP_CS_ENABLE                 0
#endif
#endif                                

#ifdef ARM64_BOARD_CONFIG_T8011
#include <pexpert/arm64/H9.h>

#define MAX_L2_CLINE                   7
#define MAX_CPUS                       3
#define MAX_CPU_CLUSTERS               1

#if DEVELOPMENT || DEBUG
#define PMAP_CS                        1
#define PMAP_CS_ENABLE                 0
#endif
#endif                                

#ifdef ARM64_BOARD_CONFIG_T8015
#include <pexpert/arm64/H10.h>

#define MAX_L2_CLINE                   7
#define MAX_CPUS                       6
#define MAX_CPU_CLUSTERS               2

#define BROKEN_FRIGGING_SLEEP          1                                                 

#if DEVELOPMENT || DEBUG
#define PMAP_CS                        1
#define PMAP_CS_ENABLE                 0
#endif
#endif                                

#ifdef ARM64_BOARD_CONFIG_T8020
#include <pexpert/arm64/H11.h>

#define MAX_L2_CLINE                   7
#define MAX_CPUS                       8
#define MAX_CPU_CLUSTERS               2

#define XNU_MONITOR                    1                          
#endif                                

#ifdef ARM64_BOARD_CONFIG_T8006
  
                                                                         
                                                                       
   
#include <pexpert/arm64/H11.h>

#undef HAS_UNCORE_CTRS
#ifdef XNU_TARGET_OS_WATCH                                 
#undef __APRR_SHADOW_SUPPORTED__
#endif

#define MAX_L2_CLINE                   7
#define MAX_CPUS                       2
#define MAX_CPU_CLUSTERS               1

#define XNU_MONITOR                    1                          
#define XNU_MONITOR_T8020_DART         1                                                
#define T8020_DART_ALLOW_BYPASS        (1 << 1)                                                      
#define XNU_MONITOR_NVME_PPL           1                                              
#define XNU_MONITOR_ANS2_SART          1                                               
#define PMAP_CS                        1
#define PMAP_CS_ENABLE                 1
#define PREFER_ARM64_32_BINARIES
#define PEXPERT_NO_3X_IMAGES           1
#endif                               

#ifdef ARM64_BOARD_CONFIG_T8027
#include <pexpert/arm64/H11.h>

#define MAX_L2_CLINE                   7
#define MAX_CPUS                       8
#define MAX_CPU_CLUSTERS               2

#define XNU_MONITOR                    1                          
#endif                                

#ifdef ARM64_BOARD_CONFIG_T8028
#include <pexpert/arm64/H11.h>

#define MAX_L2_CLINE                   7
#define MAX_CPUS                       8
#define MAX_CPU_CLUSTERS               2

#define XNU_MONITOR                    1                          
#define XNU_MONITOR_T8020_DART         1                                                
#define T8020_DART_ALLOW_BYPASS        (1 << 1)                                                      
#define XNU_MONITOR_NVME_PPL           1                                              
#define XNU_MONITOR_ANS2_SART          1                                               
#define PMAP_CS                        1
#define PMAP_CS_ENABLE                 1
#endif                                

#ifdef ARM64_BOARD_CONFIG_T8030
#include <pexpert/arm64/H12.h>

#define MAX_L2_CLINE                   7
#define MAX_CPUS                       6
#define MAX_CPU_CLUSTERS               2

#define XNU_MONITOR                    1                          
#define XNU_MONITOR_T8020_DART         1                                                
#define T8020_DART_ALLOW_BYPASS        (1 << 1)                                                      
#define XNU_MONITOR_NVME_PPL           1                                              
#define XNU_MONITOR_ANS2_SART          1                                               
#define XNU_MONITOR_UAT_PPL            1                                             
#define PMAP_CS                        1
#define PMAP_CS_ENABLE                 1
#endif                                

#ifdef ARM64_BOARD_CONFIG_T8101
#include <pexpert/arm64/H13.h>
#include <pexpert/arm64/spr_locks.h>

#define MAX_L2_CLINE                   7
#define MAX_CPUS                       8
#define MAX_CPU_CLUSTERS               2

#define XNU_MONITOR                    1                          
#endif                                

#ifdef ARM64_BOARD_CONFIG_T8103
#include <pexpert/arm64/H13.h>
#include <pexpert/arm64/spr_locks.h>
#undef HAS_SIQ

#define MAX_L2_CLINE                   7
#define MAX_CPUS                       8
#define MAX_CPU_CLUSTERS               2

#define XNU_MONITOR                    1                          
#endif                                




#ifdef ARM64_BOARD_CONFIG_BCM2837
#include <pexpert/arm64/BCM2837.h>

#define MAX_L2_CLINE                   6
#define MAX_CPUS                       4
#define MAX_CPU_CLUSTERS               1

#define CORE_NCTRS                     8                                                      
#endif                                  

#ifdef ARM64_BOARD_CONFIG_VMAPPLE
#include <pexpert/arm64/VMAPPLE.h>

#define MAX_L2_CLINE                   7
#define MAX_CPUS                       32                                                           
#define MAX_CPU_CLUSTERS               1

#define CORE_NCTRS                     2

#define USE_APPLEARMSMP                1

#endif                                  

#ifndef HAS_UNCORE_CTRS
#undef UNCORE_VERSION
#undef UNCORE_PER_CLUSTER
#undef UNCORE_NCTRS
#endif

#if MAX_CPU_CLUSTERS == 1
#undef __ARM_AMP__
#endif

#ifndef MAX_CPU_CLUSTER_PHY_ID
#define MAX_CPU_CLUSTER_PHY_ID (MAX_CPU_CLUSTERS - 1)
#endif

#ifdef PREFER_ARM64_32_BINARIES
#define PREFERRED_USER_CPU_TYPE CPU_TYPE_ARM64_32
#define PREFERRED_USER_CPU_SUBTYPE CPU_SUBTYPE_ARM64_32_V8
#endif

#endif                                    
