/*
 * Copyright (c) 2007-2013 Apple Inc. All rights reserved.
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
  
                                
   
#ifndef _ARM64_PROC_REG_H_
#define _ARM64_PROC_REG_H_

#include <arm/proc_reg.h>

#if __ARM_KERNEL_PROTECT__
  
                                                                          
                                                                                
                                                                        
                                                                              
                                                                               
                      
  
                                                                                
                                                                         
                                                                           
                
  
                                                                                
               
                                                                               
         
  
                           
                                                    
                                                    
                            
  
                                                                           
                              
  
                                                             
                                                              
                                                              
                                                              
  
                                                                          
  
                                                             
                                                              
                                                              
                                                              
  
                                                                                
                                                                               
                                                                               
                                                                             
                                            
  
                                                                               
                                                                            
                                                             
  
                                                                           
                                                                             
        
   
#if XNU_MONITOR
  
                                                                               
                                                                           
                                                                          
                                                                      
  
                                                                             
                                                      
   
#endif
#endif                           

#if ARM_PARAMETERIZED_PMAP
  
                                                                             
                                                                           
                                                                            
                                                                         
          
   
#endif                             

#if __ARM_MIXED_PAGE_SIZE__
  
                                                                               
                                                                            
                                            
  
                                                                              
                                                      
   
#if !ARM_PARAMETERIZED_PMAP
  
                                                                               
                                                                             
                            
   
#error __ARM_MIXED_PAGE_SIZE__ requires ARM_PARAMETERIZED_PMAP
#endif                              
#if __ARM_KERNEL_PROTECT__
  
                                                                               
                                                                            
                                                                  
                                                      
   
#error __ARM_MIXED_PAGE_SIZE__ and __ARM_KERNEL_PROTECT__ are mutually exclusive
#endif                             
#endif                              

  
                                         
  
                                                      
                                                       
                                                       
                                                       
  
         
                           
                                  
                      
                        
                          
                     
   

#define PSR64_NZCV_SHIFT 28
#define PSR64_NZCV_MASK  (0xF << PSR64_NZCV_SHIFT)

#define PSR64_N_SHIFT    31
#define PSR64_N          (1 << PSR64_N_SHIFT)

#define PSR64_Z_SHIFT    30
#define PSR64_Z          (1 << PSR64_Z_SHIFT)

#define PSR64_C_SHIFT    29
#define PSR64_C          (1 << PSR64_C_SHIFT)

#define PSR64_V_SHIFT    28
#define PSR64_V          (1 << PSR64_V_SHIFT)

#define PSR64_PAN_SHIFT  22
#define PSR64_PAN        (1 << PSR64_PAN_SHIFT)

#define PSR64_SS_SHIFT   21
#define PSR64_SS         (1 << PSR64_SS_SHIFT)

#define PSR64_IL_SHIFT   20
#define PSR64_IL         (1 << PSR64_IL_SHIFT)

  
                                                      
                                  
   
#define PSR64_SSBS_SHIFT_32   23
#define PSR64_SSBS_SHIFT_64   12
#define PSR64_SSBS_32         (1 << PSR64_SSBS_SHIFT_32)
#define PSR64_SSBS_64         (1 << PSR64_SSBS_SHIFT_64)

  
                                              
                      
   
#define DAIF_DEBUG_SHIFT      9
#define DAIF_DEBUGF           (1 << DAIF_DEBUG_SHIFT)

#define DAIF_ASYNC_SHIFT      8
#define DAIF_ASYNCF           (1 << DAIF_ASYNC_SHIFT)

#define DAIF_IRQF_SHIFT       7
#define DAIF_IRQF             (1 << DAIF_IRQF_SHIFT)

#define DAIF_FIQF_SHIFT       6
#define DAIF_FIQF             (1 << DAIF_FIQF_SHIFT)

#define DAIF_ALL              (DAIF_DEBUGF | DAIF_ASYNCF | DAIF_IRQF | DAIF_FIQF)
#define DAIF_STANDARD_DISABLE (DAIF_ASYNCF | DAIF_IRQF | DAIF_FIQF)

#define SPSR_INTERRUPTS_ENABLED(x) (!(x & DAIF_FIQF))

#if __ARM_ARCH_8_5__
#define PSR64_SSBS_U32_DEFAULT  PSR64_SSBS_32
#define PSR64_SSBS_U64_DEFAULT  PSR64_SSBS_64
#define PSR64_SSBS_KRN_DEFAULT  PSR64_SSBS_64
#else
#define PSR64_SSBS_U32_DEFAULT  (0)
#define PSR64_SSBS_U64_DEFAULT  (0)
#define PSR64_SSBS_KRN_DEFAULT  (0)
#endif

  
                                                
                 
   
#define DAIFSC_DEBUGF           (1 << 3)
#define DAIFSC_ASYNCF           (1 << 2)
#define DAIFSC_IRQF             (1 << 1)
#define DAIFSC_FIQF             (1 << 0)
#define DAIFSC_ALL              (DAIFSC_DEBUGF | DAIFSC_ASYNCF | DAIFSC_IRQF | DAIFSC_FIQF)
#define DAIFSC_STANDARD_DISABLE (DAIFSC_ASYNCF | DAIFSC_IRQF | DAIFSC_FIQF)

  
                              
   
#define PSR64_CF         0x20000000                          

#define PSR64_MODE_MASK         0x1F

#define PSR64_USER_MASK         PSR64_NZCV_MASK

#define PSR64_MODE_USER32_THUMB 0x20

#define PSR64_MODE_RW_SHIFT     4
#define PSR64_MODE_RW_64        0
#define PSR64_MODE_RW_32        (0x1 << PSR64_MODE_RW_SHIFT)

#define PSR64_MODE_EL_SHIFT     2
#define PSR64_MODE_EL_MASK      (0x3 << PSR64_MODE_EL_SHIFT)
#define PSR64_MODE_EL3          (0x3 << PSR64_MODE_EL_SHIFT)
#define PSR64_MODE_EL2          (0x2 << PSR64_MODE_EL_SHIFT)
#define PSR64_MODE_EL1          (0x1 << PSR64_MODE_EL_SHIFT)
#define PSR64_MODE_EL0          0

#define PSR64_MODE_SPX          0x1
#define PSR64_MODE_SP0          0

#define PSR64_USER32_DEFAULT    (PSR64_MODE_RW_32 | PSR64_MODE_EL0 | PSR64_MODE_SP0 | PSR64_SSBS_U32_DEFAULT)
#define PSR64_USER64_DEFAULT    (PSR64_MODE_RW_64 | PSR64_MODE_EL0 | PSR64_MODE_SP0 | PSR64_SSBS_U64_DEFAULT)
#define PSR64_KERNEL_STANDARD   (DAIF_STANDARD_DISABLE | PSR64_MODE_RW_64 | PSR64_MODE_EL1 | PSR64_MODE_SP0 | PSR64_SSBS_KRN_DEFAULT)
#if __ARM_PAN_AVAILABLE__
#define PSR64_KERNEL_DEFAULT    (PSR64_KERNEL_STANDARD | PSR64_PAN)
#else
#define PSR64_KERNEL_DEFAULT    PSR64_KERNEL_STANDARD
#endif
#define PSR64_KERNEL_POISON     (PSR64_IL | PSR64_MODE_EL1)

#define PSR64_IS_KERNEL(x)      ((x & PSR64_MODE_EL_MASK) > PSR64_MODE_EL0)
#define PSR64_IS_USER(x)        ((x & PSR64_MODE_EL_MASK) == PSR64_MODE_EL0)

#define PSR64_IS_USER32(x)      (PSR64_IS_USER(x) && (x & PSR64_MODE_RW_32))
#define PSR64_IS_USER64(x)      (PSR64_IS_USER(x) && !(x & PSR64_MODE_RW_32))



  
                                  
   

#define SCTLR_DSSBS               (1ULL << 44)

#define SCTLR_RESERVED     ((3ULL << 28) | (1ULL << 20))
#if defined(HAS_APPLE_PAC)

                                                            
#define SCTLR_PACIA_ENABLED_SHIFT 31
#define SCTLR_PACIA_ENABLED       (1ULL << SCTLR_PACIA_ENABLED_SHIFT)
                                                            
#define SCTLR_PACIB_ENABLED       (1ULL << 30)
                
                                                            
#define SCTLR_PACDA_ENABLED       (1ULL << 27)
                                                             
#define SCTLR_PACDB_ENABLED       (1ULL << 13)

#define SCTLR_JOP_KEYS_ENABLED (SCTLR_PACIA_ENABLED | SCTLR_PACDA_ENABLED | SCTLR_PACDB_ENABLED)
#endif                             

                                    
#define SCTLR_UCI_ENABLED         (1ULL << 26)

                                            
#define SCTLR_EE_BIG_ENDIAN       (1ULL << 25)

                                      
#define SCTLR_E0E_BIG_ENDIAN      (1ULL << 24)

                               
#define SCTLR_PAN_UNCHANGED       (1ULL << 23)

                                                                              
#define SCTLR_EIS                 (1ULL << 22)

                         
                         

                                                       
#define SCTLR_WXN_ENABLED         (1ULL << 19)

                                             
#define SCTLR_nTWE_WFE_ENABLED    (1ULL << 18)

                         

                                             
#define SCTRL_nTWI_WFI_ENABLED    (1ULL << 16)

                                                          
#define SCTLR_UCT_ENABLED         (1ULL << 15)

                                                     
#define SCTLR_DZE_ENABLED         (1ULL << 14)

                                                
#define SCTLR_I_ENABLED           (1ULL << 12)

                                                                           
#define SCTLR_EOS                 (1ULL << 11)

                         

                                        
#define SCTLR_UMA_ENABLED         (1ULL << 9)

                                      
#define SCTLR_SED_DISABLED        (1ULL << 8)

                                  
#define SCTLR_ITD_DISABLED        (1ULL << 7)

                         

                                           
#define SCTLR_CP15BEN_ENABLED     (1ULL << 5)

                                                     
#define SCTLR_SA0_ENABLED         (1ULL << 4)

                                             
#define SCTLR_SA_ENABLED          (1ULL << 3)

                                    
#define SCTLR_C_ENABLED           (1ULL << 2)

                                       
#define SCTLR_A_ENABLED           (1ULL << 1)

                                  
#define SCTLR_M_ENABLED           (1ULL << 0)

#if __ARM_ARCH_8_5__
#define SCTLR_CSEH_DEFAULT        (0)
#define SCTLR_DSSBS_DEFAULT       SCTLR_DSSBS
#else
#define SCTLR_CSEH_DEFAULT        (SCTLR_EIS | SCTLR_EOS)
#define SCTLR_DSSBS_DEFAULT       (0)
#endif

#define SCTLR_EL1_DEFAULT \
	(SCTLR_RESERVED | SCTLR_UCI_ENABLED | SCTLR_nTWE_WFE_ENABLED | SCTLR_DZE_ENABLED | \
	 SCTLR_I_ENABLED | SCTLR_SED_DISABLED | SCTLR_CP15BEN_ENABLED |                    \
	 SCTLR_SA0_ENABLED | SCTLR_SA_ENABLED | SCTLR_C_ENABLED | SCTLR_M_ENABLED |        \
	 SCTLR_CSEH_DEFAULT | SCTLR_DSSBS_DEFAULT)

  
                                              
  
                                              
                                               
                                               
                                               
  
         
                     
                                
   
#define CPACR_TTA_SHIFT     28
#define CPACR_TTA           (1 << CPACR_TTA_SHIFT)

#define CPACR_FPEN_SHIFT    20
#define CPACR_FPEN_EL0_TRAP (0x1 << CPACR_FPEN_SHIFT)
#define CPACR_FPEN_ENABLE   (0x3 << CPACR_FPEN_SHIFT)

  
                                        
  
                                                               
                                                                  
                                                                  
                                                                  
   

#define FPSR_N_SHIFT   31
#define FPSR_Z_SHIFT   30
#define FPSR_C_SHIFT   29
#define FPSR_V_SHIFT   28
#define FPSR_QC_SHIFT  27
#define FPSR_IDC_SHIFT 7
#define FPSR_IXC_SHIFT 4
#define FPSR_UFC_SHIFT 3
#define FPSR_OFC_SHIFT 2
#define FPSR_DZC_SHIFT 1
#define FPSR_IOC_SHIFT 0
#define FPSR_N         (1 << FPSR_N_SHIFT)
#define FPSR_Z         (1 << FPSR_Z_SHIFT)
#define FPSR_C         (1 << FPSR_C_SHIFT)
#define FPSR_V         (1 << FPSR_V_SHIFT)
#define FPSR_QC        (1 << FPSR_QC_SHIFT)
#define FPSR_IDC       (1 << FPSR_IDC_SHIFT)
#define FPSR_IXC       (1 << FPSR_IXC_SHIFT)
#define FPSR_UFC       (1 << FPSR_UFC_SHIFT)
#define FPSR_OFC       (1 << FPSR_OFC_SHIFT)
#define FPSR_DZC       (1 << FPSR_DZC_SHIFT)
#define FPSR_IOC       (1 << FPSR_IOC_SHIFT)

  
                                                                     
                                                                  
                                                        
   
#define FPSR_MASK \
	(FPSR_N | FPSR_Z | FPSR_C | FPSR_V | FPSR_QC | FPSR_IDC | FPSR_IXC | \
	 FPSR_UFC | FPSR_OFC | FPSR_DZC | FPSR_IOC)

  
                                         
  
                                                                           
                                                                            
                                                                            
                                                                            
   

#define FPCR_AHP_SHIFT    26
#define FPCR_DN_SHIFT     25
#define FPCR_FZ_SHIFT     24
#define FPCR_RMODE_SHIFT  22
#define FPCR_STRIDE_SHIFT 20
#define FPCR_LEN_SHIFT    16
#define FPCR_IDE_SHIFT    15
#define FPCR_IXE_SHIFT    12
#define FPCR_UFE_SHIFT    11
#define FPCR_OFE_SHIFT    10
#define FPCR_DZE_SHIFT    9
#define FPCR_IOE_SHIFT    8
#define FPCR_AHP          (1 << FPCR_AHP_SHIFT)
#define FPCR_DN           (1 << FPCR_DN_SHIFT)
#define FPCR_FZ           (1 << FPCR_FZ_SHIFT)
#define FPCR_RMODE        (0x3 << FPCR_RMODE_SHIFT)
#define FPCR_STRIDE       (0x3 << FPCR_STRIDE_SHIFT)
#define FPCR_LEN          (0x7 << FPCR_LEN_SHIFT)
#define FPCR_IDE          (1 << FPCR_IDE_SHIFT)
#define FPCR_IXE          (1 << FPCR_IXE_SHIFT)
#define FPCR_UFE          (1 << FPCR_UFE_SHIFT)
#define FPCR_OFE          (1 << FPCR_OFE_SHIFT)
#define FPCR_DZE          (1 << FPCR_DZE_SHIFT)
#define FPCR_IOE          (1 << FPCR_IOE_SHIFT)
#define FPCR_DEFAULT      (0)
#define FPCR_DEFAULT_32   (FPCR_DN|FPCR_FZ)

  
                                                                     
                                                                  
                                                        
   
#define FPCR_MASK \
	(FPCR_AHP | FPCR_DN | FPCR_FZ | FPCR_RMODE | FPCR_STRIDE | FPCR_LEN | \
	 FPCR_IDE | FPCR_IXE | FPCR_UFE | FPCR_OFE | FPCR_DZE | FPCR_IOE)

  
                                     
  
          
  
                                                                                                          
                                                                                                           
                                                                                                           
                                                                                                           
  
                                       
  
                                                                                                          
                                                                                                           
                                                                                                           
                                                                                                           
  
                                           
                                           
                   
                                     
                                       
                                       
                                             
                                             
                                                  
                                          
                                        
                                       
                                       
                                             
                                             
                                        
   

#define TCR_T0SZ_SHIFT          0ULL
#define TCR_TSZ_BITS            6ULL
#define TCR_TSZ_MASK            ((1ULL << TCR_TSZ_BITS) - 1ULL)

#define TCR_IRGN0_SHIFT         8ULL
#define TCR_IRGN0_DISABLED      (0ULL << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WRITEBACK     (1ULL << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WRITETHRU     (2ULL << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WRITEBACKNO   (3ULL << TCR_IRGN0_SHIFT)

#define TCR_ORGN0_SHIFT         10ULL
#define TCR_ORGN0_DISABLED      (0ULL << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WRITEBACK     (1ULL << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WRITETHRU     (2ULL << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WRITEBACKNO   (3ULL << TCR_ORGN0_SHIFT)

#define TCR_SH0_SHIFT           12ULL
#define TCR_SH0_NONE            (0ULL << TCR_SH0_SHIFT)
#define TCR_SH0_OUTER           (2ULL << TCR_SH0_SHIFT)
#define TCR_SH0_INNER           (3ULL << TCR_SH0_SHIFT)

#define TCR_TG0_GRANULE_SHIFT   (14ULL)
#define TCR_TG0_GRANULE_BITS    (2ULL)
#define TCR_TG0_GRANULE_MASK    ((1ULL << TCR_TG0_GRANULE_BITS) - 1ULL)

#define TCR_TG0_GRANULE_4KB     (0ULL << TCR_TG0_GRANULE_SHIFT)
#define TCR_TG0_GRANULE_64KB    (1ULL << TCR_TG0_GRANULE_SHIFT)
#define TCR_TG0_GRANULE_16KB    (2ULL << TCR_TG0_GRANULE_SHIFT)

#if __ARM_16K_PG__
#define TCR_TG0_GRANULE_SIZE    (TCR_TG0_GRANULE_16KB)
#else
#define TCR_TG0_GRANULE_SIZE    (TCR_TG0_GRANULE_4KB)
#endif

#define TCR_T1SZ_SHIFT          16ULL

#define TCR_A1_ASID1            (1ULL << 22ULL)
#define TCR_EPD1_TTBR1_DISABLED (1ULL << 23ULL)

#define TCR_IRGN1_SHIFT          24ULL
#define TCR_IRGN1_DISABLED       (0ULL << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WRITEBACK      (1ULL << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WRITETHRU      (2ULL << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WRITEBACKNO    (3ULL << TCR_IRGN1_SHIFT)

#define TCR_ORGN1_SHIFT          26ULL
#define TCR_ORGN1_DISABLED       (0ULL << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WRITEBACK      (1ULL << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WRITETHRU      (2ULL << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WRITEBACKNO    (3ULL << TCR_ORGN1_SHIFT)

#define TCR_SH1_SHIFT            28ULL
#define TCR_SH1_NONE             (0ULL << TCR_SH1_SHIFT)
#define TCR_SH1_OUTER            (2ULL << TCR_SH1_SHIFT)
#define TCR_SH1_INNER            (3ULL << TCR_SH1_SHIFT)

#define TCR_TG1_GRANULE_SHIFT    30ULL

#define TCR_TG1_GRANULE_16KB     (1ULL << TCR_TG1_GRANULE_SHIFT)
#define TCR_TG1_GRANULE_4KB      (2ULL << TCR_TG1_GRANULE_SHIFT)
#define TCR_TG1_GRANULE_64KB     (3ULL << TCR_TG1_GRANULE_SHIFT)

#if __ARM_16K_PG__
#define TCR_TG1_GRANULE_SIZE     (TCR_TG1_GRANULE_16KB)
#else
#define TCR_TG1_GRANULE_SIZE     (TCR_TG1_GRANULE_4KB)
#endif

#define TCR_IPS_SHIFT            32ULL
#define TCR_IPS_BITS             3ULL
#define TCR_IPS_MASK             ((1ULL << TCR_IPS_BITS) - 1ULL)
#define TCR_IPS_32BITS           (0ULL << TCR_IPS_SHIFT)
#define TCR_IPS_36BITS           (1ULL << TCR_IPS_SHIFT)
#define TCR_IPS_40BITS           (2ULL << TCR_IPS_SHIFT)
#define TCR_IPS_42BITS           (3ULL << TCR_IPS_SHIFT)
#define TCR_IPS_44BITS           (4ULL << TCR_IPS_SHIFT)
#define TCR_IPS_48BITS           (5ULL << TCR_IPS_SHIFT)

#define TCR_AS_16BIT_ASID        (1ULL << 36)
#define TCR_TBI0_TOPBYTE_IGNORED (1ULL << 37)
#define TCR_TBI1_TOPBYTE_IGNORED (1ULL << 38)
#define TCR_TBID0_TBI_DATA_ONLY  (1ULL << 51)
#define TCR_TBID1_TBI_DATA_ONLY  (1ULL << 52)

#if defined(HAS_APPLE_PAC)
#define TCR_TBID0_ENABLE         TCR_TBID0_TBI_DATA_ONLY
#else
#define TCR_TBID0_ENABLE         0
#endif

#define TCR_E0PD0_BIT            (1ULL << 55)
#define TCR_E0PD1_BIT            (1ULL << 56)

#if defined(HAS_E0PD)
#define TCR_E0PD_VALUE           (TCR_E0PD1_BIT)
#else
#define TCR_E0PD_VALUE           0
#endif


  
                                               
  
                                                                  
                                                                  
                                                                  
  
        
                       
                                                   
                                   
                     
                 
   
#define MPIDR_AFF0_SHIFT 0
#define MPIDR_AFF0_WIDTH 8
#define MPIDR_AFF0_MASK  (((1 << MPIDR_AFF0_WIDTH) - 1) << MPIDR_AFF0_SHIFT)
#define MPIDR_AFF1_SHIFT 8
#define MPIDR_AFF1_WIDTH 8
#define MPIDR_AFF1_MASK  (((1 << MPIDR_AFF1_WIDTH) - 1) << MPIDR_AFF1_SHIFT)
#define MPIDR_AFF2_SHIFT 16
#define MPIDR_AFF2_WIDTH 8
#define MPIDR_AFF2_MASK  (((1 << MPIDR_AFF2_WIDTH) - 1) << MPIDR_AFF2_SHIFT)

  
                                                                  
                            
  
                                         
                                        
                                          
                                         
                                                   
   
#if __ARM_KERNEL_PROTECT__
  
                                                                            
                                                                                
                                                                           
              
   
#endif                             
#ifdef __ARM_16K_PG__
#if __ARM64_PMAP_SUBPAGE_L1__
#define T0SZ_BOOT 25ULL
#else                                 
#define T0SZ_BOOT 17ULL
#endif                                 
#else                     
#if __ARM64_PMAP_SUBPAGE_L1__
#define T0SZ_BOOT 26ULL
#else                                
#define T0SZ_BOOT 25ULL
#endif                                
#endif                     

#if defined(APPLE_ARM64_ARCH_FAMILY)
                                   
#define T1SZ_BOOT T0SZ_BOOT
#else                                       
#ifdef __ARM_16K_PG__
#if __ARM64_PMAP_SUBPAGE_L1__
#define T1SZ_BOOT 25ULL
#else                                 
#define T1SZ_BOOT 17ULL
#endif                                 
#else                     
#if __ARM64_PMAP_SUBPAGE_L1__
#define T1SZ_BOOT 26ULL
#else                                
#define T1SZ_BOOT 25ULL
#endif                              
#endif                     
#endif                                       

#if __ARM_42BIT_PA_SPACE__
#define TCR_IPS_VALUE TCR_IPS_42BITS
#else                              
#define TCR_IPS_VALUE TCR_IPS_40BITS
#endif                              

#define TCR_EL1_BASE \
	(TCR_IPS_VALUE | TCR_SH0_OUTER | TCR_ORGN0_WRITEBACK |         \
	 TCR_IRGN0_WRITEBACK | (T0SZ_BOOT << TCR_T0SZ_SHIFT) |          \
	 TCR_SH1_OUTER | TCR_ORGN1_WRITEBACK | \
	 TCR_IRGN1_WRITEBACK | (TCR_TG1_GRANULE_SIZE) |                 \
	 TCR_TBI0_TOPBYTE_IGNORED | (TCR_TBID0_ENABLE) | TCR_E0PD_VALUE)

#if __ARM_KERNEL_PROTECT__
#define TCR_EL1_BOOT (TCR_EL1_BASE | (T1SZ_BOOT << TCR_T1SZ_SHIFT) | (TCR_TG0_GRANULE_SIZE))
#define T1SZ_USER (T1SZ_BOOT + 1)
#define TCR_EL1_USER (TCR_EL1_BASE | (T1SZ_USER << TCR_T1SZ_SHIFT) | (TCR_TG0_GRANULE_SIZE))
#else
#define TCR_EL1_BOOT (TCR_EL1_BASE | (T1SZ_BOOT << TCR_T1SZ_SHIFT) | (TCR_TG0_GRANULE_SIZE))
#endif                             

#define TCR_EL1_4KB  (TCR_EL1_BASE | (T1SZ_BOOT << TCR_T1SZ_SHIFT) | (TCR_TG0_GRANULE_4KB))
#define TCR_EL1_16KB (TCR_EL1_BASE | (T1SZ_BOOT << TCR_T1SZ_SHIFT) | (TCR_TG0_GRANULE_16KB))




  
                                                
   

#define MDSCR_TFO_SHIFT                 31
#define MDSCR_TFO                       (1ULL << MDSCR_TFO_SHIFT)
#define MDSCR_RXFULL_SHIFT              30
#define MDSCR_RXFULL                    (1ULL << MDSCR_RXFULL_SHIFT)
#define MDSCR_TXFULL_SHIFT              29
#define MDSCR_TXFULL                    (1ULL << MDSCR_TXFULL_SHIFT)
#define MDSCR_RXO_SHIFT                 27
#define MDSCR_RXO                       (1ULL << MDSCR_RXO_SHIFT)
#define MDSCR_TXU_SHIFT                 26
#define MDSCR_TXU                       (1ULL << MDSCR_TXU_SHIFT)
#define MDSCR_INTDIS_SHIFT              22
#define MDSCR_INTDIS_MASK               (0x2U << MDSCR_INTDIS_SHIFT)
#define MDSCR_TDA_SHIFT                 21
#define MDSCR_TDA                       (1ULL << MDSCR_TDA_SHIFT)
#define MDSCR_SC2_SHIFT                 19
#define MDSCR_SC2                       (1ULL << MDSCR_SC2_SHIFT)
#define MDSCR_MDE_SHIFT                 15
#define MDSCR_MDE                       (1ULL << MDSCR_MDE_SHIFT)
#define MDSCR_HDE_SHIFT                 14
#define MDSCR_HDE                       (1ULL << MDSCR_HDE_SHIFT)
#define MDSCR_KDE_SHIFT                 13
#define MDSCR_KDE                       (1ULL << MDSCR_KDE_SHIFT)
#define MDSCR_TDCC_SHIFT                12
#define MDSCR_TDCC                      (1ULL << MDSCR_TDCC_SHIFT)
#define MDSCR_ERR_SHIFT                 6
#define MDSCR_ERR                       (1ULL << MDSCR_ERR_SHIFT)
#define MDSCR_SS_SHIFT                  0
#define MDSCR_SS                        (1ULL << MDSCR_SS_SHIFT)

  
                                         
  
                                      
                                       
                                       
                                       
  
   
#define TTBR_ASID_SHIFT 48
#define TTBR_ASID_MASK  0xffff000000000000

#define TTBR_BADDR_MASK 0x0000ffffffffffff

  
                                        
  
                                                                   
                                                                    
                                                                    
                                                                    
  
   

#define MAIR_ATTR_SHIFT(x)          (8*(x))

                                                  
#define MAIR_OUTER_STRONGLY_ORDERED 0x0
#define MAIR_OUTER_DEVICE           0x0

#define MAIR_INNER_STRONGLY_ORDERED 0x0
#define MAIR_INNER_DEVICE           0x4

                              
#define MAIR_OUTER_NON_CACHEABLE    0x40
#define MAIR_OUTER_WRITE_THROUGH    0x80
#define MAIR_OUTER_WRITE_BACK       0xc0

#define MAIR_INNER_NON_CACHEABLE    0x4
#define MAIR_INNER_WRITE_THROUGH    0x8
#define MAIR_INNER_WRITE_BACK       0xc

                                          
#define MAIR_OUTER_WRITE_ALLOCATE   0x10
#define MAIR_OUTER_READ_ALLOCATE    0x20

#define MAIR_INNER_WRITE_ALLOCATE   0x1
#define MAIR_INNER_READ_ALLOCATE    0x2

                              

  
                       
                                                       
                                                                        
                                                                                 
   
#define MAIR_DISABLE                   0x00                                               
#define MAIR_POSTED                    0x04                                                             
#define MAIR_POSTED_REORDERED          0x08                                                       
#define MAIR_POSTED_COMBINED_REORDERED 0x0C                                                                       
#define MAIR_WRITECOMB                 0x44                                                              
#define MAIR_WRITETHRU                 0xBB                                                              
#define MAIR_WRITEBACK                 0xFF                                                        
#define MAIR_INNERWRITEBACK            0x4F                                                           


  
                                                                    
   


  
                          
   
#define CACHE_ATTRINDX_WRITEBACK                 0x0                                                     
#define CACHE_ATTRINDX_WRITECOMB                 0x1                                                
#define CACHE_ATTRINDX_WRITETHRU                 0x2                                                     
#define CACHE_ATTRINDX_DISABLE                   0x3                                          
#define CACHE_ATTRINDX_INNERWRITEBACK            0x4                                                                          
#define CACHE_ATTRINDX_POSTED                    0x5                                                         
#define CACHE_ATTRINDX_POSTED_REORDERED          0x6                                                                  
#define CACHE_ATTRINDX_POSTED_COMBINED_REORDERED 0x7                                                                                   
#define CACHE_ATTRINDX_DEFAULT                   CACHE_ATTRINDX_WRITEBACK


  
                                                        
  
                                                                                       
   
#define AP_RWNA 0x0                                      
#define AP_RWRW 0x1                                       
#define AP_RONA 0x2                                     
#define AP_RORO 0x3                                     
#define AP_MASK 0x3                           

  
                          
   
#define SH_NONE         0x0                     
#define SH_NONE         0x0                       
#define SH_DEVICE       0x2                                                              
#define SH_OUTER_MEMORY 0x2                                                      
#define SH_INNER_MEMORY 0x3                                                          


  
                   
   
#ifdef __ARM_16K_PG__
#define ARM_PGSHIFT 14
#else
#define ARM_PGSHIFT 12
#endif
#define ARM_PGBYTES (1 << ARM_PGSHIFT)
#define ARM_PGMASK  (ARM_PGBYTES-1)

  
                        
  
                     
                                   
                                                          
                                           
  
                      
                                    
                                                        
                                           
   

            
#define ARM_16K_TT_L0_SIZE       0x0000800000000000ULL                                    
#define ARM_16K_TT_L0_OFFMASK    0x00007fffffffffffULL                                
#define ARM_16K_TT_L0_SHIFT      47                                               
#define ARM_16K_TT_L0_INDEX_MASK 0x0000800000000000ULL                                                              

           
#define ARM_4K_TT_L0_SIZE       0x0000008000000000ULL                                    
#define ARM_4K_TT_L0_OFFMASK    0x0000007fffffffffULL                                
#define ARM_4K_TT_L0_SHIFT      39                                               
#define ARM_4K_TT_L0_INDEX_MASK 0x0000ff8000000000ULL                                                              

  
                        
  
                     
                                   
                                                        
                                           
  
                      
                                    
                                                          
                                           
   

            
#define ARM_16K_TT_L1_SIZE       0x0000001000000000ULL                                    
#define ARM_16K_TT_L1_OFFMASK    0x0000000fffffffffULL                                
#define ARM_16K_TT_L1_SHIFT      36                                               
#if __ARM64_PMAP_SUBPAGE_L1__ && __ARM_16K_PG__
                                          
#define ARM_16K_TT_L1_INDEX_MASK 0x0000007000000000ULL                                                                
#else                                
#define ARM_16K_TT_L1_INDEX_MASK 0x00007ff000000000ULL                                                                
#endif                                

           
#define ARM_4K_TT_L1_SIZE       0x0000000040000000ULL                                    
#define ARM_4K_TT_L1_OFFMASK    0x000000003fffffffULL                                
#define ARM_4K_TT_L1_SHIFT      30                                               
#if __ARM64_PMAP_SUBPAGE_L1__ && !__ARM_16K_PG__
                                          
#define ARM_4K_TT_L1_INDEX_MASK 0x0000003fc0000000ULL                                                                
#else                                
#define ARM_4K_TT_L1_INDEX_MASK 0x0000007fc0000000ULL                                                                
#endif                                

                                                                

#define L1_TABLE_INDEX(va) (((va) & ARM_TT_L1_INDEX_MASK) >> ARM_TT_L1_SHIFT)
#define L2_TABLE_INDEX(va) (((va) & ARM_TT_L2_INDEX_MASK) >> ARM_TT_L2_SHIFT)
#define L3_TABLE_INDEX(va) (((va) & ARM_TT_L3_INDEX_MASK) >> ARM_TT_L3_SHIFT)

#define L2_TABLE_VA(tte)  ((tt_entry_t*) phystokv((*(tte)) & ARM_TTE_TABLE_MASK))
#define L3_TABLE_VA(tte2) ((pt_entry_t*) phystokv((*(tte2)) & ARM_TTE_TABLE_MASK))

  
                        
  
                     
                                   
                                                        
                                         
  
                      
                                    
                                                          
                                          
   

            
#define ARM_16K_TT_L2_SIZE       0x0000000002000000ULL                                    
#define ARM_16K_TT_L2_OFFMASK    0x0000000001ffffffULL                                
#define ARM_16K_TT_L2_SHIFT      25                                               
#define ARM_16K_TT_L2_INDEX_MASK 0x0000000ffe000000ULL                                                              

           
#define ARM_4K_TT_L2_SIZE       0x0000000000200000ULL                                    
#define ARM_4K_TT_L2_OFFMASK    0x00000000001fffffULL                                
#define ARM_4K_TT_L2_SHIFT      21                                               
#define ARM_4K_TT_L2_INDEX_MASK 0x000000003fe00000ULL                                                              

  
                        
  
                     
                                   
                                                        
                                         
  
                      
                                    
                                                          
                                          
   

            
#define ARM_16K_TT_L3_SIZE       0x0000000000004000ULL                                    
#define ARM_16K_TT_L3_OFFMASK    0x0000000000003fffULL                           
#define ARM_16K_TT_L3_SHIFT      14                                               
#define ARM_16K_TT_L3_INDEX_MASK 0x0000000001ffc000ULL                                     

           
#define ARM_4K_TT_L3_SIZE       0x0000000000001000ULL                                    
#define ARM_4K_TT_L3_OFFMASK    0x0000000000000fffULL                           
#define ARM_4K_TT_L3_SHIFT      12                                               
#define ARM_4K_TT_L3_INDEX_MASK 0x00000000001ff000ULL                                     

#ifdef __ARM_16K_PG__

                       
#define ARM_TT_L0_SIZE       ARM_16K_TT_L0_SIZE
#define ARM_TT_L0_OFFMASK    ARM_16K_TT_L0_OFFMASK
#define ARM_TT_L0_SHIFT      ARM_16K_TT_L0_SHIFT
#define ARM_TT_L0_INDEX_MASK ARM_16K_TT_L0_INDEX_MASK

                       
#define ARM_TT_L1_SIZE       ARM_16K_TT_L1_SIZE
#define ARM_TT_L1_OFFMASK    ARM_16K_TT_L1_OFFMASK
#define ARM_TT_L1_SHIFT      ARM_16K_TT_L1_SHIFT
#define ARM_TT_L1_INDEX_MASK ARM_16K_TT_L1_INDEX_MASK

                       
#define ARM_TT_L2_SIZE       ARM_16K_TT_L2_SIZE
#define ARM_TT_L2_OFFMASK    ARM_16K_TT_L2_OFFMASK
#define ARM_TT_L2_SHIFT      ARM_16K_TT_L2_SHIFT
#define ARM_TT_L2_INDEX_MASK ARM_16K_TT_L2_INDEX_MASK

                       
#define ARM_TT_L3_SIZE       ARM_16K_TT_L3_SIZE
#define ARM_TT_L3_OFFMASK    ARM_16K_TT_L3_OFFMASK
#define ARM_TT_L3_SHIFT      ARM_16K_TT_L3_SHIFT
#define ARM_TT_L3_INDEX_MASK ARM_16K_TT_L3_INDEX_MASK

#else                      

                       
#define ARM_TT_L0_SIZE       ARM_4K_TT_L0_SIZE
#define ARM_TT_L0_OFFMASK    ARM_4K_TT_L0_OFFMASK
#define ARM_TT_L0_SHIFT      ARM_4K_TT_L0_SHIFT
#define ARM_TT_L0_INDEX_MASK ARM_4K_TT_L0_INDEX_MASK

                       
#define ARM_TT_L1_SIZE       ARM_4K_TT_L1_SIZE
#define ARM_TT_L1_OFFMASK    ARM_4K_TT_L1_OFFMASK
#define ARM_TT_L1_SHIFT      ARM_4K_TT_L1_SHIFT
#define ARM_TT_L1_INDEX_MASK ARM_4K_TT_L1_INDEX_MASK

                       
#define ARM_TT_L2_SIZE       ARM_4K_TT_L2_SIZE
#define ARM_TT_L2_OFFMASK    ARM_4K_TT_L2_OFFMASK
#define ARM_TT_L2_SHIFT      ARM_4K_TT_L2_SHIFT
#define ARM_TT_L2_INDEX_MASK ARM_4K_TT_L2_INDEX_MASK

                       
#define ARM_TT_L3_SIZE       ARM_4K_TT_L3_SIZE
#define ARM_TT_L3_OFFMASK    ARM_4K_TT_L3_OFFMASK
#define ARM_TT_L3_SHIFT      ARM_4K_TT_L3_SHIFT
#define ARM_TT_L3_INDEX_MASK ARM_4K_TT_L3_INDEX_MASK

#endif                      

  
                               
                                                                     
                                                                               
                                                                      
  
                                                           
   
#define ARM_TT_LEAF_SIZE       ARM_TT_L3_SIZE
#define ARM_TT_LEAF_OFFMASK    ARM_TT_L3_OFFMASK
#define ARM_TT_LEAF_SHIFT      ARM_TT_L3_SHIFT
#define ARM_TT_LEAF_INDEX_MASK ARM_TT_L3_INDEX_MASK

#define ARM_TT_TWIG_SIZE       ARM_TT_L2_SIZE
#define ARM_TT_TWIG_OFFMASK    ARM_TT_L2_OFFMASK
#define ARM_TT_TWIG_SHIFT      ARM_TT_L2_SHIFT
#define ARM_TT_TWIG_INDEX_MASK ARM_TT_L2_INDEX_MASK

#define ARM_TT_ROOT_SIZE       ARM_TT_L1_SIZE
#define ARM_TT_ROOT_OFFMASK    ARM_TT_L1_OFFMASK
#define ARM_TT_ROOT_SHIFT      ARM_TT_L1_SHIFT
#define ARM_TT_ROOT_INDEX_MASK ARM_TT_L1_INDEX_MASK

  
                    
  
                                  
  
                                                                     
                                                                      
                                                                      
                                                                      
  
                                  
  
                                                                     
                                                                      
                                                                      
                                                                      
  
                                  
  
                                                                                                
                                                                                                 
                                                                                                 
                                                                                                 
  
                                  
  
                                                                     
                                                                      
                                                                      
                                                                      
  
                                  
  
                                                                                                
                                                                                                 
                                                                                                 
                                                                                                 
  
                     
  
                                  
  
                                                                     
                                                                      
                                                                      
                                                                      
  
                                  
  
                                                                     
                                                                      
                                                                      
                                                                      
  
                                  
  
                                                                     
                                                                      
                                                                      
                                                                      
  
                                  
  
                                                                                                
                                                                                                 
                                                                                                 
                                                                                                 
  
         
                           
                                
                               
                               
                                         
                            
                                               
                                    
   

#define TTE_SHIFT                   3                                                                                          
#ifdef __ARM_16K_PG__
#define TTE_PGENTRIES               (16384 >> TTE_SHIFT)                                        
#else
#define TTE_PGENTRIES               (4096 >> TTE_SHIFT)                                         
#endif

#define ARM_TTE_MAX                 (TTE_PGENTRIES)

#define ARM_TTE_EMPTY               0x0000000000000000ULL                                         
#define ARM_TTE_TYPE_FAULT          0x0000000000000000ULL                                         

#define ARM_TTE_VALID               0x0000000000000001ULL                           

#define ARM_TTE_TYPE_MASK           0x0000000000000002ULL                                            
#define ARM_TTE_TYPE_TABLE          0x0000000000000002ULL                               
#define ARM_TTE_TYPE_BLOCK          0x0000000000000000ULL                                
#define ARM_TTE_TYPE_L3BLOCK        0x0000000000000002ULL
#define ARM_TTE_TYPE_MASK           0x0000000000000002ULL                                            

#ifdef __ARM_16K_PG__
  
                                                                               
                           
   
#define ARM_TTE_BLOCK_SHIFT         12                                                                      
#define ARM_TTE_BLOCK_L0_SHIFT      ARM_TT_L0_SHIFT                                                   
#define ARM_TTE_BLOCK_L1_MASK       0x0000fff000000000ULL                                                                
#define ARM_TTE_BLOCK_L1_SHIFT      ARM_TT_L1_SHIFT                                                  
#define ARM_TTE_BLOCK_L2_MASK       0x0000fffffe000000ULL                                                                                 
#define ARM_TTE_BLOCK_L2_SHIFT      ARM_TT_L2_SHIFT                                                  
#else
#define ARM_TTE_BLOCK_SHIFT         12                                                                     
#define ARM_TTE_BLOCK_L0_SHIFT      ARM_TT_L0_SHIFT                                                    
#define ARM_TTE_BLOCK_L1_MASK       0x0000ffffc0000000ULL                                                                
#define ARM_TTE_BLOCK_L1_SHIFT      ARM_TT_L1_SHIFT                                                 
#define ARM_TTE_BLOCK_L2_MASK       0x0000ffffffe00000ULL                                                                                 
#define ARM_TTE_BLOCK_L2_SHIFT      ARM_TT_L2_SHIFT                                                 
#endif

#define ARM_TTE_BLOCK_APSHIFT       6
#define ARM_TTE_BLOCK_AP(x)         ((x)<<ARM_TTE_BLOCK_APSHIFT)                          
#define ARM_TTE_BLOCK_APMASK        (0x3 << ARM_TTE_BLOCK_APSHIFT)

#define ARM_TTE_BLOCK_ATTRINDX(x)   ((x) << 2)                                                  
#define ARM_TTE_BLOCK_ATTRINDXMASK  (0x7ULL << 2)                                                    

#define ARM_TTE_BLOCK_SH(x)         ((x) << 8)                                        
#define ARM_TTE_BLOCK_SHMASK        (0x3ULL << 8)                                          

#define ARM_TTE_BLOCK_AF            0x0000000000000400ULL                                
#define ARM_TTE_BLOCK_AFMASK        0x0000000000000400ULL                           

#define ARM_TTE_BLOCK_NG            0x0000000000000800ULL                                          
#define ARM_TTE_BLOCK_NG_MASK       0x0000000000000800ULL                                      

#define ARM_TTE_BLOCK_NS            0x0000000000000020ULL                                          
#define ARM_TTE_BLOCK_NS_MASK       0x0000000000000020ULL                                      

#define ARM_TTE_BLOCK_PNX           0x0020000000000000ULL                                                  
#define ARM_TTE_BLOCK_PNXMASK       0x0020000000000000ULL                                         

#define ARM_TTE_BLOCK_NX            0x0040000000000000ULL                                    
#define ARM_TTE_BLOCK_NXMASK        0x0040000000000000ULL                               

#define ARM_TTE_BLOCK_WIRED         0x0400000000000000ULL                                            
#define ARM_TTE_BLOCK_WIREDMASK     0x0400000000000000ULL                                   

#define ARM_TTE_BLOCK_WRITEABLE     0x0800000000000000ULL                                                
#define ARM_TTE_BLOCK_WRITEABLEMASK 0x0800000000000000ULL                                       

#define ARM_TTE_TABLE_MASK          0x0000fffffffff000ULL                                                                              

#define ARM_TTE_TABLE_APSHIFT       61
#define ARM_TTE_TABLE_AP(x)         ((x)<<TTE_BLOCK_APSHIFT)                              

#define ARM_TTE_TABLE_NS            0x8000000000000020ULL                                          
#define ARM_TTE_TABLE_NS_MASK       0x8000000000000020ULL                                      

#define ARM_TTE_TABLE_XN            0x1000000000000000ULL                                    
#define ARM_TTE_TABLE_XNMASK        0x1000000000000000ULL                               

#define ARM_TTE_TABLE_PXN           0x0800000000000000ULL                                                  
#define ARM_TTE_TABLE_PXNMASK       0x0800000000000000ULL                                      

#if __ARM_KERNEL_PROTECT__
#define ARM_TTE_BOOT_BLOCK \
	(ARM_TTE_TYPE_BLOCK | ARM_TTE_VALID | ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) | \
	 ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_WRITEBACK) | ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_NG)
#else                             
#define ARM_TTE_BOOT_BLOCK \
	(ARM_TTE_TYPE_BLOCK | ARM_TTE_VALID | ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY) | \
	 ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_WRITEBACK) | ARM_TTE_BLOCK_AF)
#endif                             

#define ARM_TTE_BOOT_TABLE (ARM_TTE_TYPE_TABLE | ARM_TTE_VALID )
  
                        
  
                     
                                   
                                                        
                                         
  
                      
                                    
                                                          
                                          
   

#ifdef __ARM_16K_PG__
#define ARM_PTE_SIZE    0x0000000000004000ULL                                    
#define ARM_PTE_OFFMASK 0x0000000000003fffULL                             
#define ARM_PTE_SHIFT   14                                               
#define ARM_PTE_MASK    0x0000ffffffffc000ULL                                     
#else
#define ARM_PTE_SIZE    0x0000000000001000ULL                                    
#define ARM_PTE_OFFMASK 0x0000000000000fffULL                             
#define ARM_PTE_SHIFT   12                                               
#define ARM_PTE_MASK    0x0000fffffffff000ULL                                     
#endif

#define ARM_TTE_PA_MASK 0x0000fffffffff000ULL

  
                        
  
                                                     
  
                   
                                      
                                      
                                      
                                      
  
  
                                                                                         
                                                                                          
                                                                                          
                                                                                          
  
         
                           
                                
                               
                               
                                         
                            
                                               
                                    
   

#define PTE_SHIFT               3                                                                                 
#ifdef __ARM_16K_PG__
#define PTE_PGENTRIES           (16384 >> PTE_SHIFT)                               
#else
#define PTE_PGENTRIES           (4096 >> PTE_SHIFT)                                
#endif

#define ARM_PTE_EMPTY           0x0000000000000000ULL                                 

                                                             
#define ARM_PTE_COMPRESSED      0x8000000000000000ULL                    
#define ARM_PTE_COMPRESSED_ALT  0x4000000000000000ULL                             
#define ARM_PTE_COMPRESSED_MASK 0xC000000000000000ULL

#define ARM_PTE_IS_COMPRESSED(x, p) \
	((((x) & 0x3) == 0) &&                                                \
	 ((x) & ARM_PTE_COMPRESSED) &&                                        \
	 ((!((x) & ~ARM_PTE_COMPRESSED_MASK)) ||                              \
	 (panic("compressed PTE %p 0x%llx has extra bits 0x%llx: corrupted?", \
	        (p), (x), (x) & ~ARM_PTE_COMPRESSED_MASK), FALSE)))

#define ARM_PTE_TYPE               0x0000000000000003ULL                                                           
#define ARM_PTE_TYPE_VALID         0x0000000000000003ULL                                                           
#define ARM_PTE_TYPE_FAULT         0x0000000000000000ULL                       
#define ARM_PTE_TYPE_MASK          0x0000000000000002ULL                           

                                                                                            
#define ARM_PTE_PAGE_MASK          0x0000FFFFFFFFF000ULL                                   
#define ARM_PTE_PAGE_SHIFT         12                                                                        

#define ARM_PTE_AP(x)              ((x) << 6)                                    
#define ARM_PTE_APMASK             (0x3ULL << 6)                                      
#define ARM_PTE_EXTRACT_AP(x)      (((x) >> 6) & 0x3ULL)                                          

#define ARM_PTE_ATTRINDX(x)        ((x) << 2)                                         
#define ARM_PTE_ATTRINDXMASK       (0x7ULL << 2)                                           

#define ARM_PTE_SH(x)              ((x) << 8)                               
#define ARM_PTE_SHMASK             (0x3ULL << 8)                                 

#define ARM_PTE_AF                 0x0000000000000400ULL                       
#define ARM_PTE_AFMASK             0x0000000000000400ULL                  

#define ARM_PTE_NG                 0x0000000000000800ULL                                 
#define ARM_PTE_NG_MASK            0x0000000000000800ULL                             

#define ARM_PTE_NS                 0x0000000000000020ULL                                 
#define ARM_PTE_NS_MASK            0x0000000000000020ULL                             

#define ARM_PTE_HINT               0x0010000000000000ULL                                        
#define ARM_PTE_HINT_MASK          0x0010000000000000ULL                                       

#if __ARM_16K_PG__
#define ARM_PTE_HINT_ENTRIES       128ULL                                                       
#define ARM_PTE_HINT_ENTRIES_SHIFT 7ULL                                                                
#define ARM_PTE_HINT_ADDR_MASK     0x0000FFFFFFE00000ULL                                                
#define ARM_PTE_HINT_ADDR_SHIFT    21                                                    
#define ARM_KVA_HINT_ADDR_MASK     0xFFFFFFFFFFE00000ULL                                                
#else
#define ARM_PTE_HINT_ENTRIES       16ULL                                                        
#define ARM_PTE_HINT_ENTRIES_SHIFT 4ULL                                                                
#define ARM_PTE_HINT_ADDR_MASK     0x0000FFFFFFFF0000ULL                                                
#define ARM_PTE_HINT_ADDR_SHIFT    16                                                    
#define ARM_KVA_HINT_ADDR_MASK     0xFFFFFFFFFFFF0000ULL                                                
#endif

#define ARM_PTE_PNX                0x0020000000000000ULL                                         
#define ARM_PTE_PNXMASK            0x0020000000000000ULL                                

#define ARM_PTE_NX                 0x0040000000000000ULL                               
#define ARM_PTE_NXMASK             0x0040000000000000ULL                      

#define ARM_PTE_XMASK              (ARM_PTE_PNXMASK | ARM_PTE_NXMASK)

#define ARM_PTE_WIRED              0x0400000000000000ULL                                   
#define ARM_PTE_WIRED_MASK         0x0400000000000000ULL                          

#define ARM_PTE_WRITEABLE          0x0800000000000000ULL                                       
#define ARM_PTE_WRITEABLE_MASK     0x0800000000000000ULL                              

#if CONFIG_PGTRACE
#define ARM_PTE_PGTRACE            0x0200000000000000ULL                                   
#define ARM_PTE_PGTRACE_MASK       0x0200000000000000ULL                          
#endif

#define ARM_PTE_BOOT_PAGE_BASE \
	(ARM_PTE_TYPE_VALID | ARM_PTE_SH(SH_OUTER_MEMORY) |       \
	 ARM_PTE_ATTRINDX(CACHE_ATTRINDX_WRITEBACK) | ARM_PTE_AF)

#if __ARM_KERNEL_PROTECT__
#define ARM_PTE_BOOT_PAGE (ARM_PTE_BOOT_PAGE_BASE | ARM_PTE_NG)
#else                             
#define ARM_PTE_BOOT_PAGE (ARM_PTE_BOOT_PAGE_BASE)
#endif                             

  
                                                          
                              
   
#define TLBI_ADDR_SHIFT (0)
#define TLBI_ADDR_SIZE  (44)
#define TLBI_ADDR_MASK  ((1ULL << TLBI_ADDR_SIZE) - 1)
#define TLBI_ASID_SHIFT (48)
#define TLBI_ASID_SIZE  (16)
#define TLBI_ASID_MASK  (((1ULL << TLBI_ASID_SIZE) - 1))

#define RTLBI_ADDR_SIZE (37)
#define RTLBI_ADDR_MASK ((1ULL << RTLBI_ADDR_SIZE) - 1)
#define RTLBI_ADDR_SHIFT ARM_TT_L3_SHIFT
#define RTLBI_TG(_page_shift_) ((uint64_t)((((_page_shift_) - 12) >> 1) + 1) << 46)
#define RTLBI_SCALE_SHIFT (44)
#define RTLBI_NUM_SHIFT (39)

  
                              
  
                                
                                 
                                 
                                 
  
                        
                           
                                      
  
                                                                         
   

#define ESR_EC_SHIFT           26
#define ESR_EC_MASK            (0x3FULL << ESR_EC_SHIFT)
#define ESR_EC(x)              ((x & ESR_EC_MASK) >> ESR_EC_SHIFT)

#define ESR_IL_SHIFT           25
#define ESR_IL                 (1 << ESR_IL_SHIFT)

#define ESR_INSTR_IS_2BYTES(x) (!(x & ESR_IL))

#define ESR_ISS_MASK           0x01FFFFFF
#define ESR_ISS(x)             (x & ESR_ISS_MASK)

#ifdef __ASSEMBLER__
                                                                       
#define ESR_EC_IABORT_EL1      0x21
#define ESR_EC_DABORT_EL1      0x25
#define ESR_EC_SP_ALIGN        0x26
#else
typedef enum {
	ESR_EC_UNCATEGORIZED       = 0x00,
	ESR_EC_WFI_WFE             = 0x01,
	ESR_EC_MCR_MRC_CP15_TRAP   = 0x03,
	ESR_EC_MCRR_MRRC_CP15_TRAP = 0x04,
	ESR_EC_MCR_MRC_CP14_TRAP   = 0x05,
	ESR_EC_LDC_STC_CP14_TRAP   = 0x06,
	ESR_EC_TRAP_SIMD_FP        = 0x07,
	ESR_EC_PTRAUTH_INSTR_TRAP  = 0x09,
	ESR_EC_MCRR_MRRC_CP14_TRAP = 0x0c,
	ESR_EC_ILLEGAL_INSTR_SET   = 0x0e,
	ESR_EC_SVC_32              = 0x11,
	ESR_EC_SVC_64              = 0x15,
	ESR_EC_MSR_TRAP            = 0x18,
	ESR_EC_IABORT_EL0          = 0x20,
	ESR_EC_IABORT_EL1          = 0x21,
	ESR_EC_PC_ALIGN            = 0x22,
	ESR_EC_DABORT_EL0          = 0x24,
	ESR_EC_DABORT_EL1          = 0x25,
	ESR_EC_SP_ALIGN            = 0x26,
	ESR_EC_FLOATING_POINT_32   = 0x28,
	ESR_EC_FLOATING_POINT_64   = 0x2C,
	ESR_EC_BKPT_REG_MATCH_EL0  = 0x30,                                                           
	ESR_EC_BKPT_REG_MATCH_EL1  = 0x31,                                                       
	ESR_EC_SW_STEP_DEBUG_EL0   = 0x32,                                                              
	ESR_EC_SW_STEP_DEBUG_EL1   = 0x33,                                                          
	ESR_EC_WATCHPT_MATCH_EL0   = 0x34,                                                           
	ESR_EC_WATCHPT_MATCH_EL1   = 0x35,                                                       
	ESR_EC_BKPT_AARCH32        = 0x38,
	ESR_EC_BRK_AARCH64         = 0x3C,
} esr_exception_class_t;

typedef enum {
	FSC_TRANSLATION_FAULT_L0   = 0x04,
	FSC_TRANSLATION_FAULT_L1   = 0x05,
	FSC_TRANSLATION_FAULT_L2   = 0x06,
	FSC_TRANSLATION_FAULT_L3   = 0x07,
	FSC_ACCESS_FLAG_FAULT_L1   = 0x09,
	FSC_ACCESS_FLAG_FAULT_L2   = 0x0A,
	FSC_ACCESS_FLAG_FAULT_L3   = 0x0B,
	FSC_PERMISSION_FAULT_L1    = 0x0D,
	FSC_PERMISSION_FAULT_L2    = 0x0E,
	FSC_PERMISSION_FAULT_L3    = 0x0F,
	FSC_SYNC_EXT_ABORT         = 0x10,
	FSC_ASYNC_EXT_ABORT        = 0x11,
	FSC_SYNC_EXT_ABORT_TT_L1   = 0x15,
	FSC_SYNC_EXT_ABORT_TT_L2   = 0x16,
	FSC_SYNC_EXT_ABORT_TT_L3   = 0x17,
	FSC_SYNC_PARITY            = 0x18,
	FSC_ASYNC_PARITY           = 0x19,
	FSC_SYNC_PARITY_TT_L1      = 0x1D,
	FSC_SYNC_PARITY_TT_L2      = 0x1E,
	FSC_SYNC_PARITY_TT_L3      = 0x1F,
	FSC_ALIGNMENT_FAULT        = 0x21,
	FSC_DEBUG_FAULT            = 0x22,
} fault_status_t;
#endif                

  
                                      
                                   
                                    
                                    
                                    
  
         
                                     
                           
                                        
   

#define ISS_SSDE_ISV_SHIFT 24
#define ISS_SSDE_ISV       (0x1 << ISS_SSDE_ISV_SHIFT)

#define ISS_SSDE_EX_SHIFT  6
#define ISS_SSDE_EX        (0x1 << ISS_SSDE_EX_SHIFT)

#define ISS_SSDE_FSC_MASK  0x3F
#define ISS_SSDE_FSC(x)    (x & ISS_SSDE_FSC_MASK)

  
                              
                                 
                                  
                                  
                                  
  
         
                              
                                        
   

#define ISS_IA_EA_SHIFT 9
#define ISS_IA_EA       (0x1 << ISS_IA_EA_SHIFT)

#define ISS_IA_FSC_MASK 0x3F
#define ISS_IA_FSC(x)   (x & ISS_IA_FSC_MASK)


  
                       
  
                                    
                                     
                                         
                                     
  
         
                               
                                       
                          
                                                        
                                  
   
#define ISS_DA_EA_SHIFT  9
#define ISS_DA_EA        (0x1 << ISS_DA_EA_SHIFT)

#define ISS_DA_CM_SHIFT  8
#define ISS_DA_CM        (0x1 << ISS_DA_CM_SHIFT)

#define ISS_DA_WNR_SHIFT 6
#define ISS_DA_WNR       (0x1 << ISS_DA_WNR_SHIFT)

#define ISS_DA_S1PTW_SHIFT 7
#define ISS_DA_S1PTW     (0x1 << ISS_DA_S1PTW_SHIFT)

#define ISS_DA_FSC_MASK  0x3F
#define ISS_DA_FSC(x)    (x & ISS_DA_FSC_MASK)

  
                                     
  
                                                   
                                                     
                                                     
                                                     
  
         
                             
                                  
                                 
                             
                            
                                  
                                     
   
#define ISS_FP_TFV_SHIFT 23
#define ISS_FP_TFV       (0x1 << ISS_FP_TFV_SHIFT)

#define ISS_FP_IDF_SHIFT 7
#define ISS_FP_IDF       (0x1 << ISS_FP_IDF_SHIFT)

#define ISS_FP_IXF_SHIFT 4
#define ISS_FP_IXF       (0x1 << ISS_FP_IXF_SHIFT)

#define ISS_FP_UFF_SHIFT 3
#define ISS_FP_UFF       (0x1 << ISS_FP_UFF_SHIFT)

#define ISS_FP_OFF_SHIFT 2
#define ISS_FP_OFF       (0x1 << ISS_FP_OFF_SHIFT)

#define ISS_FP_DZF_SHIFT 1
#define ISS_FP_DZF       (0x1 << ISS_FP_DZF_SHIFT)

#define ISS_FP_IOF_SHIFT 0
#define ISS_FP_IOF       (0x1 << ISS_FP_IOF_SHIFT)

  
                                 
                        
                        
                        
                        
  
         
                                             
   
#define ISS_BRK_COMMENT_MASK    0xFFFF
#define ISS_BRK_COMMENT(x)      (x & ISS_BRK_COMMENT_MASK)





  
                                  
   
#define PAR_F_SHIFT 0
#define PAR_F       (0x1 << PAR_F_SHIFT)

#define PLATFORM_SYSCALL_TRAP_NO 0x80000000

#define ARM64_SYSCALL_CODE_REG_NUM (16)

#define ARM64_CLINE_SHIFT 6

#if defined(APPLE_ARM64_ARCH_FAMILY)
#define L2CERRSTS_DATSBEESV (1ULL << 2)                                    
#define L2CERRSTS_DATDBEESV (1ULL << 4)                                    
#endif

  
                     
   
#define CNTKCTL_EL1_PL0PTEN      (0x1 << 9)                                                               
#define CNTKCTL_EL1_PL0VTEN      (0x1 << 8)                                                              
#define CNTKCTL_EL1_EVENTI_MASK  (0x000000f0)                                                                                    
#define CNTKCTL_EL1_EVENTI_SHIFT (0x4)                                    
#define CNTKCTL_EL1_EVENTDIR     (0x1 << 3)                                                                        
#define CNTKCTL_EL1_EVNTEN       (0x1 << 2)                                       
#define CNTKCTL_EL1_PL0VCTEN     (0x1 << 1)                                                                          
#define CNTKCTL_EL1_PL0PCTEN     (0x1 << 0)                                                                           

#define CNTV_CTL_EL0_ISTATUS     (0x1 << 2)                                                        
#define CNTV_CTL_EL0_IMASKED     (0x1 << 1)                                    
#define CNTV_CTL_EL0_ENABLE      (0x1 << 0)                                         

#define CNTP_CTL_EL0_ISTATUS     CNTV_CTL_EL0_ISTATUS
#define CNTP_CTL_EL0_IMASKED     CNTV_CTL_EL0_IMASKED
#define CNTP_CTL_EL0_ENABLE      CNTV_CTL_EL0_ENABLE

  
                                                                                              
                             
   
#define ARM_DBG_VR_ADDRESS_MASK64 0xFFFFFFFFFFFFFFFCull                

#define MIDR_EL1_REV_SHIFT  0
#define MIDR_EL1_REV_MASK   (0xf << MIDR_EL1_REV_SHIFT)
#define MIDR_EL1_PNUM_SHIFT 4
#define MIDR_EL1_PNUM_MASK  (0xfff << MIDR_EL1_PNUM_SHIFT)
#define MIDR_EL1_ARCH_SHIFT 16
#define MIDR_EL1_ARCH_MASK  (0xf << MIDR_EL1_ARCH_SHIFT)
#define MIDR_EL1_VAR_SHIFT  20
#define MIDR_EL1_VAR_MASK   (0xf << MIDR_EL1_VAR_SHIFT)
#define MIDR_EL1_IMP_SHIFT  24
#define MIDR_EL1_IMP_MASK   (0xff << MIDR_EL1_IMP_SHIFT)

#define MIDR_FIJI             (0x002 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_CAPRI            (0x003 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_MAUI             (0x004 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_ELBA             (0x005 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_CAYMAN           (0x006 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_MYST             (0x007 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_SKYE_MONSOON     (0x008 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_SKYE_MISTRAL     (0x009 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_CYPRUS_VORTEX    (0x00B << MIDR_EL1_PNUM_SHIFT)
#define MIDR_CYPRUS_TEMPEST   (0x00C << MIDR_EL1_PNUM_SHIFT)
#define MIDR_M9               (0x00F << MIDR_EL1_PNUM_SHIFT)
#define MIDR_ARUBA_VORTEX     (0x010 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_ARUBA_TEMPEST    (0x011 << MIDR_EL1_PNUM_SHIFT)

#ifdef APPLELIGHTNING
#define MIDR_CEBU_LIGHTNING   (0x012 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_CEBU_THUNDER     (0x013 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_TURKS            (0x026 << MIDR_EL1_PNUM_SHIFT)
#endif

#ifdef APPLEFIRESTORM
#define MIDR_SICILY_ICESTORM            (0x020 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_SICILY_FIRESTORM           (0x021 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_TONGA_ICESTORM             (0x022 << MIDR_EL1_PNUM_SHIFT)
#define MIDR_TONGA_FIRESTORM            (0x023 << MIDR_EL1_PNUM_SHIFT)
#endif


  
                                    
   
#define AIDR_MUL53            (1 << 0)
#define AIDR_WKDM             (1 << 1)
#define AIDR_ARCHRETENTION    (1 << 2)


  
                            
   
#define CORESIGHT_ED  0
#define CORESIGHT_CTI 1
#define CORESIGHT_PMU 2
#define CORESIGHT_UTT 3                                                                                                       

#define CORESIGHT_OFFSET(x) ((x) * 0x10000)
#define CORESIGHT_REGIONS   4
#define CORESIGHT_SIZE      0x1000










  
                                                                  
  
                                                         
                                                          
                                                          
                                                          
   

#define ID_AA64ISAR0_EL1_FHM_OFFSET    48
#define ID_AA64ISAR0_EL1_FHM_MASK      (0xfull << ID_AA64ISAR0_EL1_FHM_OFFSET)
#define ID_AA64ISAR0_EL1_FHM_8_2       (1ull << ID_AA64ISAR0_EL1_FHM_OFFSET)

#define ID_AA64ISAR0_EL1_ATOMIC_OFFSET 20
#define ID_AA64ISAR0_EL1_ATOMIC_MASK   (0xfull << ID_AA64ISAR0_EL1_ATOMIC_OFFSET)
#define ID_AA64ISAR0_EL1_ATOMIC_8_1    (2ull << ID_AA64ISAR0_EL1_ATOMIC_OFFSET)

#define ID_AA64ISAR0_EL1_CRC32_OFFSET  16
#define ID_AA64ISAR0_EL1_CRC32_MASK    (0xfull << ID_AA64ISAR0_EL1_CRC32_OFFSET)
#define ID_AA64ISAR0_EL1_CRC32_EN      (1ull << ID_AA64ISAR0_EL1_CRC32_OFFSET)

#define ID_AA64ISAR0_EL1_SHA3_OFFSET   32
#define ID_AA64ISAR0_EL1_SHA3_MASK     (0xfull << ID_AA64ISAR0_EL1_SHA3_OFFSET)
#define ID_AA64ISAR0_EL1_SHA3_EN       (1ull << ID_AA64ISAR0_EL1_SHA3_OFFSET)

#define ID_AA64ISAR0_EL1_SHA2_OFFSET   12
#define ID_AA64ISAR0_EL1_SHA2_MASK     (0xfull << ID_AA64ISAR0_EL1_SHA2_OFFSET)
#define ID_AA64ISAR0_EL1_SHA2_EN       (1ull << ID_AA64ISAR0_EL1_SHA2_OFFSET)

#define ID_AA64ISAR0_EL1_SHA1_OFFSET   8
#define ID_AA64ISAR0_EL1_SHA1_MASK     (0xfull << ID_AA64ISAR0_EL1_SHA1_OFFSET)
#define ID_AA64ISAR0_EL1_SHA1_EN       (1ull << ID_AA64ISAR0_EL1_SHA1_OFFSET)

#define ID_AA64ISAR0_EL1_AES_OFFSET    4
#define ID_AA64ISAR0_EL1_AES_MASK      (0xfull << ID_AA64ISAR0_EL1_AES_OFFSET)
#define ID_AA64ISAR0_EL1_AES_EN        (1ull << ID_AA64ISAR0_EL1_AES_OFFSET)
#define ID_AA64ISAR0_EL1_AES_PMULL_EN  (2ull << ID_AA64ISAR0_EL1_AES_OFFSET)



#define APSTATE_G_SHIFT  (0)
#define APSTATE_P_SHIFT  (1)
#define APSTATE_A_SHIFT  (2)
#define APSTATE_AP_MASK  ((1ULL << APSTATE_A_SHIFT) | (1ULL << APSTATE_P_SHIFT))


#define ACTLR_EL1_EnTSO   (1ULL << 1)
#define ACTLR_EL1_EnAPFLG (1ULL << 4)
#define ACTLR_EL1_EnAFP   (1ULL << 5)
#define ACTLR_EL1_EnPRSV  (1ULL << 6)

#define ACTLR_EL1_DisHWP_OFFSET  3
#define ACTLR_EL1_DisHWP_MASK    (1ULL << ACTLR_EL1_DisHWP_OFFSET)
#define ACTLR_EL1_DisHWP         ACTLR_EL1_DisHWP_MASK



#define AFPCR_DAZ_SHIFT  (0)
#define AFPCR_FTZ_SHIFT  (1)

#if defined(HAS_APPLE_PAC)
                                                                                                       
#define PAC_DISCRIMINATOR_RECOVER    0x1e02
#endif


#define CTR_EL0_L1Ip_OFFSET 14
#define CTR_EL0_L1Ip_VIPT (2ULL << CTR_EL0_L1Ip_OFFSET)
#define CTR_EL0_L1Ip_PIPT (3ULL << CTR_EL0_L1Ip_OFFSET)
#define CTR_EL0_L1Ip_MASK (3ULL << CTR_EL0_L1Ip_OFFSET)


#ifdef __ASSEMBLER__

  
                                                          
                                                               
                                                       
                         
                                                                   
  
                                                 
                                              
                                          
                                                                       
   
.macro CMSR
cmp $1, $2

                                        
b.eq $3f
msr $0, $2
.endmacro

.macro CMSR_ISB
CMSR $0, $1, $2, $3
isb sy
.endmacro

  
                                                                 
                                              
                        
                                                                  
   
.macro SANITIZE_FPCR
mov $1, #FPCR_DEFAULT
CMSR FPCR, $0, $1, $2
.endmacro

  
                                                                            
                                                                             
                                                                        
                                                                       
                      
                                                      
                                                                                                
   

  
                                               
                                           
                                     
                        
   
.macro EXEC_COREEQ_REVEQ
and $3, $2, #MIDR_EL1_PNUM_MASK
cmp $3, $0
b.ne 1f

mov $3, $2
bfi  $3, $3, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $3, $3, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $3, $1
b.ne 1f
.endmacro

.macro EXEC_COREEQ_REVLO
and $3, $2, #MIDR_EL1_PNUM_MASK
cmp $3, $0
b.ne 1f

mov $3, $2
bfi  $3, $3, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $3, $3, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $3, $1
b.pl 1f
.endmacro

.macro EXEC_COREEQ_REVHS
and $3, $2, #MIDR_EL1_PNUM_MASK
cmp $3, $0
b.ne 1f

mov $3, $2
bfi  $3, $3, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $3, $3, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $3, $1
b.mi 1f
.endmacro

  
                                           
                                     
                        
   
.macro EXEC_COREALL_REVEQ
mov $2, $1
bfi  $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $2, $0
b.ne 1f
.endmacro

.macro EXEC_COREALL_REVLO
mov  $2, $1
bfi  $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $2, $0
b.pl 1f
.endmacro

.macro EXEC_COREALL_REVHS
mov $2, $1
bfi  $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $2, $0
b.mi 1f
.endmacro

  
                                               
                                     
                        
   
.macro EXEC_COREEQ_REVALL
and $2, $1, #MIDR_EL1_PNUM_MASK
cmp $2, $0
b.ne 1f
.endmacro

  
                                           
                                     
                        
   
.macro EXEC_PCORE_REVEQ
mrs $2, MPIDR_EL1
and $2, $2, #(MPIDR_PNE)
cmp $2, xzr
b.eq 1f

mov $2, $1
bfi  $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $2, $0
b.ne 1f
.endmacro

.macro EXEC_PCORE_REVLO
mrs $2, MPIDR_EL1
and $2, $2, #(MPIDR_PNE)
cmp $2, xzr
b.eq 1f

mov $2, $1
bfi  $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $2, $0
b.pl 1f
.endmacro

.macro EXEC_PCORE_REVHS
mrs $2, MPIDR_EL1
and $2, $2, #(MPIDR_PNE)
cmp $2, xzr
b.eq 1f

mov $2, $1
bfi  $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $2, $0
b.mi 1f
.endmacro

.macro EXEC_ECORE_REVEQ
mrs $2, MPIDR_EL1
and $2, $2, #(MPIDR_PNE)
cmp $2, xzr
b.ne 1f

mov $2, $1
bfi  $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $2, $0
b.ne 1f
.endmacro

.macro EXEC_ECORE_REVLO
mrs $2, MPIDR_EL1
and $2, $2, #(MPIDR_PNE)
cmp $2, xzr
b.ne 1f

mov $2, $1
bfi  $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $2, $0
b.pl 1f
.endmacro

.macro EXEC_ECORE_REVHS
mrs $2, MPIDR_EL1
and $2, $2, #(MPIDR_PNE)
cmp $2, xzr
b.ne 1f

mov $2, $1
bfi  $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #4
ubfx $2, $2, #(MIDR_EL1_VAR_SHIFT - 4), #8
cmp $2, $0
b.mi 1f
.endmacro

  
                                     
                        
   
.macro EXEC_PCORE_REVALL
mrs $1, MPIDR_EL1
and $1, $1, #(MPIDR_PNE)
cmp $1, xzr
b.eq 1f
.endmacro

.macro EXEC_ECORE_REVALL
mrs $1, MPIDR_EL1
and $1, $1, #(MPIDR_PNE)
cmp $1, xzr
b.ne 1f
.endmacro



  
                                                                          
   
.macro EXEC_END
1:
.endmacro

  
                                                                               
                                                                             
                                                   
   
.macro DEPRECATE_COREEQ_REVLO   core, rev, midr_el1, scratch
EXEC_COREEQ_REVLO \core, \rev, \midr_el1, \scratch
                            
b .
                          
EXEC_END
.endmacro

  
                                
                                             
                                
                         
   
.macro HID_SET_BITS
mrs $2, $0
orr $2, $2, $1
msr $0, $2
.endmacro

  
                                  
                                             
                                    
                         
   
.macro HID_CLEAR_BITS
mrs $2, $0
bic $2, $2, $1
msr $0, $2
.endmacro

  
                                  
                                             
                                    
                        
                         
   
.macro HID_INSERT_BITS
mrs $3, $0
bic $3, $3, $1
orr $3, $3, $2
msr $0, $3
.endmacro

  
                                                       
                                                            
                           
   
.macro ERET_CONTEXT_SYNCHRONIZING
eret
#if __ARM_SB_AVAILABLE__
sb                                                                                                                                          
#else                           
isb                                                                                                                                             
nop                                                                                                         
nop
nop
nop
nop
nop
#endif                            
.endmacro

#endif                    

#define MSR(reg, src)  __asm__ volatile ("msr " reg ", %0" :: "r" (src))
#define MRS(dest, reg) __asm__ volatile ("mrs %0, " reg : "=r" (dest))

#if XNU_MONITOR
#define __ARM_PTE_PHYSMAP__ 1
#define PPL_STATE_KERNEL    0
#define PPL_STATE_DISPATCH  1
#define PPL_STATE_PANIC     2
#define PPL_STATE_EXCEPTION 3
#endif


#endif                         
