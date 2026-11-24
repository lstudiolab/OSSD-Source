/*
 * Copyright (c) 2007-2018 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
                 
/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

  
   

  
                              
   
#ifndef _ARM_PROC_REG_H_
#define _ARM_PROC_REG_H_

#if defined (__arm64__)
#include <pexpert/arm64/board_config.h>
#elif defined (__arm__)
#include <pexpert/arm/board_config.h>
#endif

#if defined (ARMA7)
#define __ARM_ARCH__               7
#define __ARM_SUB_ARCH__           CPU_ARCH_ARMv7k
#define __ARM_VMSA__               7
#define __ARM_VFP__                3

                                                                         
                                                                                      
                                                                                      
                                                                                       
                                                                                     
                                                                 
#define __ARM_PTE_PHYSMAP__        1
                                                                                                                
                                                                     
   
#define __ARMA7_SMP__              1
#define __ARM_COHERENT_CACHE__     1
#define __ARM_DEBUG__              7
#define __ARM_USER_PROTECT__       1
#define __ARM_TIME_TIMEBASE_ONLY__ 1

#endif

#if __ARM_42BIT_PA_SPACE__
                               
                                                                     
#undef __ARM64_PMAP_SUBPAGE_L1__
#endif                             

#if __ARM_KERNEL_PROTECT__
  
                                                                              
                                                                             
                                     
   
#if __arm__
#error __ARM_KERNEL_PROTECT__ is not supported on ARM32
#endif              
#endif                             

#if defined(ARM_BOARD_WFE_TIMEOUT_NS)
#define __ARM_ENABLE_WFE_ 1
#else                                        
#define __ARM_ENABLE_WFE_ 0
#endif                                        

  
                                                            
                                                                         
                                                                      
                                                                     
                                                                       
                         
  
                                      
   
#if __ARM_AMP__
#define MAX_PSETS 2
#else                 
#define MAX_PSETS 1
#endif                  

  
                                                                     
   
#if CONFIG_CLUTCH

#if __ARM_AMP__

                                                      
#if XNU_TARGET_OS_OSX
#define CONFIG_SCHED_CLUTCH 1
#define CONFIG_SCHED_EDGE   1
#endif                        

#else                  
#define CONFIG_SCHED_CLUTCH 1
#endif                  

#endif                    

                                                                                
#define CONFIG_THREAD_GROUPS 1

#ifdef XNU_KERNEL_PRIVATE

#if __ARM_VFP__
#define ARM_VFP_DEBUG 0
#endif                  

#endif                         



  
                
  
                                        
                                       
  
                                                               
                                                                
                                                                
                                                                
   

  
        
   
#define PSR_NF 0x80000000                         
#define PSR_ZF 0x40000000           
#define PSR_CF 0x20000000                          
#define PSR_VF 0x10000000               
#define PSR_QF 0x08000000                                   

  
                                
   
#define PSR_JF  0x01000000                               
#define PSR_EF  0x00000200                                       
#define PSR_AF  0x00000100                                 
#define PSR_TF  0x00000020                             
#define PSR_TFb 5                                      

  
             
   
#define PSR_IRQFb 7                                    
#define PSR_IRQF  0x00000080                           
#define PSR_FIQF  0x00000040                           

  
           
   
#define PSR_USER_MODE 0x00000010                
#define PSR_FIQ_MODE  0x00000011               
#define PSR_IRQ_MODE  0x00000012               
#define PSR_SVC_MODE  0x00000013                      
#define PSR_ABT_MODE  0x00000017                 
#define PSR_UND_MODE  0x0000001B                     

#define PSR_MODE_MASK      0x0000001F
#define PSR_IS_KERNEL(psr) (((psr) & PSR_MODE_MASK) != PSR_USER_MODE)
#define PSR_IS_USER(psr)   (((psr) & PSR_MODE_MASK) == PSR_USER_MODE)

#define PSR_USERDFLT  PSR_USER_MODE
#define PSR_USER_MASK (PSR_AF | PSR_IRQF | PSR_FIQF | PSR_MODE_MASK)
#define PSR_USER_SET  PSR_USER_MODE

#define PSR_INTMASK PSR_IRQF                        

  
                                           
   

#define FPEXC_EX     0x80000000                       
#define FPEXC_EX_BIT 31
#define FPEXC_EN     0x40000000                          
#define FPEXC_EN_BIT 30


  
                                                    
   

#define FPSCR_DN      0x02000000                           
#define FPSCR_FZ      0x01000000                             

#define FPSCR_DEFAULT FPSCR_DN | FPSCR_FZ


  
                
  
                                          
                                   
   
#define FSR_ALIGN      0x00000001                
#define FSR_DEBUG      0x00000002                          
#define FSR_ICFAULT    0x00000004                                             
#define FSR_SFAULT     0x00000005                          
#define FSR_PFAULT     0x00000007                       
#define FSR_SACCESS    0x00000003                     
#define FSR_PACCESS    0x00000006                  
#define FSR_SDOM       0x00000009                     
#define FSR_PDOM       0x0000000B                  
#define FSR_SPERM      0x0000000D                         
#define FSR_PPERM      0x0000000F                      
#define FSR_EXT        0x00001000                                                       

#define FSR_MASK       0x0000040F                 
#define FSR_ALIGN_MASK 0x0000040D                                

#define DFSR_WRITE     0x00000800                             

#if defined (ARMA7) || defined (APPLE_ARM64_ARCH_FAMILY) || defined (BCM2837)

#define TEST_FSR_VMFAULT(status) \
	(((status) == FSR_PFAULT)     \
	|| ((status) == FSR_PPERM)    \
	|| ((status) == FSR_SFAULT)   \
	|| ((status) == FSR_SPERM)    \
	|| ((status) == FSR_ICFAULT)  \
	|| ((status) == FSR_SACCESS)  \
	|| ((status) == FSR_PACCESS))

#define TEST_FSR_TRANSLATION_FAULT(status)      \
	                        (((status) == FSR_SFAULT)       \
	                        || ((status) == FSR_PFAULT))

#else

#error Incompatible CPU type configured

#endif

  
                      
   

#if defined (ARMA7)

             
#define MMU_I_CLINE     5                                                                  

             
#define MMU_CSIZE       15                                                           
#define MMU_CLINE       6                                                                
#define MMU_NWAY        2                                                             
#define MMU_I7SET       6                                                                
#define MMU_I7WAY       30                                                               

#define MMU_SWAY        (MMU_CSIZE - MMU_NWAY)                           
#define MMU_NSET        (MMU_SWAY - MMU_CLINE)                                

#define __ARM_L2CACHE__ 1

#define L2_CSIZE        20                                                       
#define L2_CLINE        6                                                                  
#define L2_NWAY         3                                                               
#define L2_I7SET        6                                                                  
#define L2_I7WAY        29                                                                 
#define L2_I9WAY        29                                                                 

#define L2_SWAY         (L2_CSIZE - L2_NWAY)                               
#define L2_NSET         (L2_SWAY - L2_CLINE)                                    

#elif defined (APPLETYPHOON)

             
#define MMU_I_CLINE 6                                                                  

             
#define MMU_CLINE   6                                                                

#elif defined (APPLETWISTER)

             
#define MMU_I_CLINE 6                                                                  

             
#define MMU_CLINE   6                                                                

#elif defined (APPLEHURRICANE)

             
#define MMU_I_CLINE 6                                                                  

             
#define MMU_CLINE   6                                                                

#elif defined (APPLEMONSOON)

                                                         
#define MMU_I_CLINE 6                                                                  

                                                         
#define MMU_CLINE   6                                                                

#elif defined (APPLEVORTEX)

                                                              
#define MMU_I_CLINE 6                                                                  

                                                              
#define MMU_CLINE   6                                                                

#elif defined (APPLELIGHTNING)

                                                            
#define MMU_I_CLINE 6                                                                  

                                                                   
#define MMU_CLINE   6                                                                

#elif defined (APPLEFIRESTORM)

                                                              
#define MMU_I_CLINE 6                                                                  

                                                                    
#define MMU_CLINE   6                                                                

#elif defined (BCM2837)                     

                                                                                         
#define MMU_I_CLINE 6

              
#define MMU_CLINE   6

#elif defined (VMAPPLE)

              
#define MMU_I_CLINE 6

              
#define MMU_CLINE   6

#else
#error processor not supported
#endif


#if (__ARM_VMSA__ <= 7)

  
                                  
   
  
                                  
  
                                                                                          
                                                                                           
                                                                                           
                                                                                           
  
         
                                   
                               
                             
                                            
                                 
                                     
                                                 
                       
                                                         
                                        
                                        
                               
                                   
                                     
                                     
                            
                                
                         
                                   
                       
   

#define SCTLR_RESERVED 0x82DD8394

#define SCTLR_ENABLE   0x00000001                 
#define SCTLR_ALIGN    0x00000002                             
#define SCTLR_DCACHE   0x00000004                                   
#define SCTLR_BEN      0x00000040                          
#define SCTLR_SW       0x00000400                      
#define SCTLR_PREDIC   0x00000800                               
#define SCTLR_ICACHE   0x00001000                                 
#define SCTLR_HIGHVEC  0x00002000                                 
#define SCTLR_RROBIN   0x00004000                              
#define SCTLR_HA       0x00020000                                  
#define SCTLR_NMFI     0x08000000                       
#define SCTLR_TRE      0x10000000                       
#define SCTLR_AFE      0x20000000                         
#define SCTLR_TE       0x40000000                             

#define SCTLR_DEFAULT \
	(SCTLR_AFE|SCTLR_TRE|SCTLR_HIGHVEC|SCTLR_ICACHE|SCTLR_PREDIC|SCTLR_DCACHE|SCTLR_ENABLE)


  
                                       
  
                                                                   
                                                                    
                                                                    
                                                                    
   

#define PRRR_NS1 0x00080000
#define PRRR_NS0 0x00040000
#define PRRR_DS1 0x00020000
#define PRRR_DS0 0x00010000

#define PRRR_NOSn_ISH(region) (0x1<<((region)+24))

#if defined (ARMA7)
#define PRRR_SETUP (0x1F08022A)
#else
#error processor not supported
#endif

  
                                      
  
                                                                   
                                                                    
                                                                    
                                                                    
   

#define NMRR_DISABLED    0x0                    
#define NMRR_WRITEBACK   0x1                                 
#define NMRR_WRITETHRU   0x2                                       
#define NMRR_WRITEBACKNO 0x3                                    

#if defined (ARMA7)
#define NMRR_SETUP (0x01210121)
#else
#error processor not supported
#endif

  
                                        
  
   

#define TTBR_IRGN_DISBALED    0x00000000                          
#define TTBR_IRGN_WRITEBACK   0x00000040                                    
#define TTBR_IRGN_WRITETHRU   0x00000001                       
#define TTBR_IRGN_WRITEBACKNO 0x00000041                                   

#define TTBR_RGN_DISBALED     0x00000000                          
#define TTBR_RGN_WRITEBACK    0x00000008                                    
#define TTBR_RGN_WRITETHRU    0x00000010                                   
#define TTBR_RGN_WRITEBACKNO  0x00000018                                   

#define TTBR_SHARED           0x00000002                                
#define TTBR_SHARED_NOTOUTER  0x00000020                                          

#if defined (ARMA7)
#define TTBR_SETUP (TTBR_RGN_WRITEBACK|TTBR_IRGN_WRITEBACK|TTBR_SHARED)
#else
#error processor not supported
#endif

  
                                                 
  
              
               
               
               
  
                                                                       
                                                                              
                       
  
                                                                            
                                                                              
   

#define TTBCR_N_1GB_TTB0 0x2                          
#define TTBCR_N_2GB_TTB0 0x1                          
#define TTBCR_N_4GB_TTB0 0x0                
#define TTBCR_N_MASK     0x3

#define TTBCR_N_SETUP (TTBCR_N_2GB_TTB0)



  
                   
   
#define ARM_PGSHIFT 12
#define ARM_PGBYTES (1 << ARM_PGSHIFT)
#define ARM_PGMASK  (ARM_PGBYTES-1)

  
                                       
   

#define DAC_FAULT   0x0                                      
#define DAC_CLIENT  0x1                                  
#define DAC_RESERVE 0x2                                  
#define DAC_MANAGER 0x3                                  

#define DACR_SET(dom, x) ((x)<<((dom)<<1))


#define ARM_DOM_DEFAULT 0                                  
#define ARM_DAC_SETUP   0x1

  
                                 
   

  
                          
   
#define CACHE_ATTRINDX_WRITEBACK                 0x0                                    
#define CACHE_ATTRINDX_WRITECOMB                 0x1                                
#define CACHE_ATTRINDX_WRITETHRU                 0x2                                     
#define CACHE_ATTRINDX_DISABLE                   0x3                          
#define CACHE_ATTRINDX_INNERWRITEBACK            0x4                                                          
#define CACHE_ATTRINDX_POSTED                    CACHE_ATTRINDX_DISABLE
#define CACHE_ATTRINDX_POSTED_REORDERED          CACHE_ATTRINDX_DISABLE
#define CACHE_ATTRINDX_POSTED_COMBINED_REORDERED CACHE_ATTRINDX_DISABLE
#define CACHE_ATTRINDX_DEFAULT                   CACHE_ATTRINDX_WRITEBACK


  
                               
   
#define AP_RWNA 0x0                                       
#define AP_RWRW 0x1                                       
#define AP_RONA 0x2                                       
#define AP_RORO 0x3                                       

  
                        
  
                                        
                                                
   

#define ARM_TT_L1_SIZE          0x00100000                                                 
#define ARM_TT_L1_OFFMASK       0x000FFFFF                                             
#define ARM_TT_L1_TABLE_OFFMASK 0x000FFFFF                                             
#define ARM_TT_L1_BLOCK_OFFMASK 0x000FFFFF                                             
#define ARM_TT_L1_SUPER_OFFMASK 0x00FFFFFF                                             
#define ARM_TT_L1_SHIFT         20                                                 
#define ARM_TT_L1_INDEX_MASK    0xfff00000                                                                           

#define ARM_TT_L1_PT_SIZE       (4 * ARM_TT_L1_SIZE)                                                                    
#define ARM_TT_L1_PT_OFFMASK    (ARM_TT_L1_PT_SIZE - 1)

  
                        
  
                                       
                                                       
   

#define ARM_TT_L2_SIZE       0x00001000                                    
#define ARM_TT_L2_OFFMASK    0x00000FFF                                
#define ARM_TT_L2_SHIFT      12                                    
#define ARM_TT_L2_INDEX_MASK 0x000ff000                                                              

  
                               
                                                                     
                                                                               
                                                                      
  
                                                           
   
#define ARM_TT_LEAF_SIZE       ARM_TT_L2_SIZE
#define ARM_TT_LEAF_OFFMASK    ARM_TT_L2_OFFMASK
#define ARM_TT_LEAF_SHIFT      ARM_TT_L2_SHIFT
#define ARM_TT_LEAF_INDEX_MASK ARM_TT_L2_INDEX_MASK

#define ARM_TT_TWIG_SIZE       ARM_TT_L1_SIZE
#define ARM_TT_TWIG_OFFMASK    ARM_TT_L1_OFFMASK
#define ARM_TT_TWIG_SHIFT      ARM_TT_L1_SHIFT
#define ARM_TT_TWIG_INDEX_MASK ARM_TT_L1_INDEX_MASK

#define ARM_TT_ROOT_SIZE       ARM_TT_L1_SIZE
#define ARM_TT_ROOT_OFFMASK    ARM_TT_L1_OFFMASK
#define ARM_TT_ROOT_SHIFT      ARM_TT_L1_SHIFT
#define ARM_TT_ROOT_INDEX_MASK ARM_TT_L1_INDEX_MASK

  
                                  
  
                   
  
                                          
                                           
                                           
                                           
  
                             
  
                                                
                                                 
                                                 
                                                 
  
                              
  
                                                 
                                                  
                                                  
                                                  
  
         
                             
                          
                                                     
                                      
                                  
                                            
                                  
                                       
   

#define ARM_TTE_EMPTY            0x00000000                                          

#define ARM_TTE_TYPE_FAULT       0x00000000                                           
#define ARM_TTE_TYPE_TABLE       0x00000001                                          
#define ARM_TTE_TYPE_BLOCK       0x00000002                                             
#define ARM_TTE_TYPE_MASK        0x00000003                                                       

#define ARM_TTE_BLOCK_NGSHIFT    17
#define ARM_TTE_BLOCK_NG_MASK    0x00020000                                                          
#define ARM_TTE_BLOCK_NG         0x00020000                                                          

#define ARM_TTE_BLOCK_SHSHIFT    16
#define ARM_TTE_BLOCK_SH_MASK    0x00010000                                                    
#define ARM_TTE_BLOCK_SH         0x00010000                                               

#define ARM_TTE_BLOCK_CBSHIFT    2
#define ARM_TTE_BLOCK_CB(x)      ((x) << ARM_TTE_BLOCK_CBSHIFT)
#define ARM_TTE_BLOCK_CB_MASK    (3<< ARM_TTE_BLOCK_CBSHIFT)

#define ARM_TTE_BLOCK_AP0SHIFT   10
#define ARM_TTE_BLOCK_AP0        (1<<ARM_TTE_BLOCK_AP0SHIFT)
#define ARM_TTE_BLOCK_AP0_MASK   (1<<ARM_TTE_BLOCK_AP0SHIFT)

#define ARM_TTE_BLOCK_AP1SHIFT   11
#define ARM_TTE_BLOCK_AP1        (1<<ARM_TTE_BLOCK_AP1SHIFT)
#define ARM_TTE_BLOCK_AP1_MASK   (1<<ARM_TTE_BLOCK_AP1SHIFT)

#define ARM_TTE_BLOCK_AP2SHIFT   15
#define ARM_TTE_BLOCK_AP2        (1<<ARM_TTE_BLOCK_AP2SHIFT)
#define ARM_TTE_BLOCK_AP2_MASK   (1<<ARM_TTE_BLOCK_AP2SHIFT)

                        
#define ARM_TTE_BLOCK_AP(ap) \
	((((ap)&0x1)<<ARM_TTE_BLOCK_AP1SHIFT) |    \
	((((ap)>>1)&0x1)<<ARM_TTE_BLOCK_AP2SHIFT))

                             
#define ARM_TTE_BLOCK_APMASK \
	(ARM_TTE_BLOCK_AP1_MASK | ARM_TTE_BLOCK_AP2_MASK)

#define ARM_TTE_BLOCK_AF         ARM_TTE_BLOCK_AP0                                   
#define ARM_TTE_BLOCK_AFMASK     ARM_TTE_BLOCK_AP0_MASK                         

#define ARM_TTE_TABLE_MASK       0xFFFFFC00                                                        
#define ARM_TTE_TABLE_SHIFT      10                                                                       

#define ARM_TTE_BLOCK_L1_MASK    0xFFF00000                                                                            
#define ARM_TTE_BLOCK_L1_SHIFT   20                                                                    

#define ARM_TTE_SUPER_L1_MASK    0xFF000000                                                                          
#define ARM_TTE_SUPER_L1_SHIFT   24                                                                     

#define ARM_TTE_BLOCK_SUPER      0x00040000                                                     
#define ARM_TTE_BLOCK_SUPER_MASK 0x00F40000                                                     

#define ARM_TTE_BLOCK_NXSHIFT    4
#define ARM_TTE_BLOCK_NX         0x00000010                                               
#define ARM_TTE_BLOCK_NX_MASK    0x00000010                                                            
#define ARM_TTE_BLOCK_PNX        ARM_TTE_BLOCK_NX

#define ARM_TTE_BLOCK_TEX0SHIFT  12
#define ARM_TTE_BLOCK_TEX0       (1<<ARM_TTE_BLOCK_TEX0SHIFT)
#define ARM_TTE_BLOCK_TEX0_MASK  (1<<ARM_TTE_BLOCK_TEX0SHIFT)

#define ARM_TTE_BLOCK_TEX1SHIFT  13
#define ARM_TTE_BLOCK_TEX1       (1<<ARM_TTE_BLOCK_TEX1SHIFT)
#define ARM_TTE_BLOCK_TEX1_MASK  (1<<ARM_TTE_BLOCK_TEX1SHIFT)

#define ARM_TTE_BLOCK_TEX2SHIFT  14
#define ARM_TTE_BLOCK_TEX2       (1<<ARM_TTE_BLOCK_TEX2SHIFT)
#define ARM_TTE_BLOCK_TEX2_MASK  (1<<ARM_TTE_BLOCK_TEX2SHIFT)


                                  
#define ARM_TTE_BLOCK_ATTRINDX(i) \
	((((i)&0x3)<<ARM_TTE_BLOCK_CBSHIFT) |      \
	((((i)>>2)&0x1)<<ARM_TTE_BLOCK_TEX0SHIFT))

                                  
#define ARM_TTE_BLOCK_ATTRINDXMASK \
	(ARM_TTE_BLOCK_CB_MASK | ARM_TTE_BLOCK_TEX0_MASK)


  
                             
  
                                                     
  
                   
                                               
                                                
                                                
                                                
  
                          
                                               
                                                
                                                
                                                
  
                         
                                                 
                                                  
                                                  
                                                  
  
              
                                  
                                                
                          
                                                     
                                      
                                  
                                            
                               
                                      
   

                                                             
#define ARM_PTE_COMPRESSED      ARM_PTE_TEX1                                                     
#define ARM_PTE_COMPRESSED_ALT  ARM_PTE_TEX2                                                              
#define ARM_PTE_COMPRESSED_MASK (ARM_PTE_COMPRESSED | ARM_PTE_COMPRESSED_ALT)
#define ARM_PTE_IS_COMPRESSED(x, p) \
	((((x) & 0x3) == 0) &&                                                     \
	 ((x) & ARM_PTE_COMPRESSED) &&                                             \
	 ((!((x) & ~ARM_PTE_COMPRESSED_MASK)) ||                                   \
	  (panic("compressed PTE %p 0x%x has extra bits 0x%x: corrupted?",         \
	         (p), (x), (x) & ~ARM_PTE_COMPRESSED_MASK), FALSE)))

#define PTE_SHIFT              2                                                                                  
#define PTE_PGENTRIES          (1024 >> PTE_SHIFT)                                 

#define ARM_PTE_EMPTY          0x00000000                                            

#define ARM_PTE_TYPE_FAULT     0x00000000                                   
#define ARM_PTE_TYPE_VALID     0x00000002                                 
#define ARM_PTE_TYPE           0x00000002                                        
#define ARM_PTE_TYPE_MASK      0x00000002                                       

#define ARM_PTE_NG_MASK        0x00000800                                                  
#define ARM_PTE_NG             0x00000800                                                  

#define ARM_PTE_SHSHIFT        10
#define ARM_PTE_SHMASK         0x00000400                                            
#define ARM_PTE_SH             0x00000400                                       

#define ARM_PTE_CBSHIFT        2
#define ARM_PTE_CB(x)          ((x)<<ARM_PTE_CBSHIFT)
#define ARM_PTE_CB_MASK        (0x3<<ARM_PTE_CBSHIFT)

#define ARM_PTE_AP0SHIFT       4
#define ARM_PTE_AP0            (1<<ARM_PTE_AP0SHIFT)
#define ARM_PTE_AP0_MASK       (1<<ARM_PTE_AP0SHIFT)

#define ARM_PTE_AP1SHIFT       5
#define ARM_PTE_AP1            (1<<ARM_PTE_AP1SHIFT)
#define ARM_PTE_AP1_MASK       (1<<ARM_PTE_AP1SHIFT)

#define ARM_PTE_AP2SHIFT       9
#define ARM_PTE_AP2            (1<<ARM_PTE_AP2SHIFT)
#define ARM_PTE_AP2_MASK       (1<<ARM_PTE_AP2SHIFT)

                        
#define ARM_PTE_AP(ap) \
	((((ap)&0x1)<<ARM_PTE_AP1SHIFT) |    \
	((((ap)>>1)&0x1)<<ARM_PTE_AP2SHIFT))

                             
#define ARM_PTE_APMASK \
	(ARM_PTE_AP1_MASK | ARM_PTE_AP2_MASK)

#define ARM_PTE_AF             ARM_PTE_AP0                                  
#define ARM_PTE_AFMASK         ARM_PTE_AP0_MASK                        

#define ARM_PTE_PAGE_MASK      0xFFFFF000                                        
#define ARM_PTE_PAGE_SHIFT     12                                                  

#define ARM_PTE_NXSHIFT        0
#define ARM_PTE_NX             0x00000001                                        
#define ARM_PTE_NX_MASK        (1<<ARM_PTE_NXSHIFT)

#define ARM_PTE_PNXSHIFT       0
#define ARM_PTE_PNX            0x00000000                                                 
#define ARM_PTE_PNX_MASK       (0<<ARM_PTE_NXSHIFT)

#define ARM_PTE_XMASK          (ARM_PTE_PNX_MASK | ARM_PTE_NX_MASK)

#define ARM_PTE_TEX0SHIFT      6
#define ARM_PTE_TEX0           (1<<ARM_PTE_TEX0SHIFT)
#define ARM_PTE_TEX0_MASK      (1<<ARM_PTE_TEX0SHIFT)

#define ARM_PTE_TEX1SHIFT      7
#define ARM_PTE_TEX1           (1<<ARM_PTE_TEX1SHIFT)
#define ARM_PTE_TEX1_MASK      (1<<ARM_PTE_TEX1SHIFT)

#define ARM_PTE_WRITEABLESHIFT ARM_PTE_TEX1SHIFT
#define ARM_PTE_WRITEABLE      ARM_PTE_TEX1
#define ARM_PTE_WRITEABLE_MASK ARM_PTE_TEX1_MASK

#define ARM_PTE_TEX2SHIFT      8
#define ARM_PTE_TEX2           (1<<ARM_PTE_TEX2SHIFT)
#define ARM_PTE_TEX2_MASK      (1<<ARM_PTE_TEX2SHIFT)

#define ARM_PTE_WIREDSHIFT     ARM_PTE_TEX2SHIFT
#define ARM_PTE_WIRED          ARM_PTE_TEX2
#define ARM_PTE_WIRED_MASK     ARM_PTE_TEX2_MASK

                                  
#define ARM_PTE_ATTRINDX(indx) \
	((((indx)&0x3)<<ARM_PTE_CBSHIFT) |      \
	((((indx)>>2)&0x1)<<ARM_PTE_TEX0SHIFT))

                                  
#define ARM_PTE_ATTRINDXMASK \
	(ARM_PTE_CB_MASK | ARM_PTE_TEX0_MASK)

#define ARM_SMALL_PAGE_SIZE    (4096)                  
#define ARM_LARGE_PAGE_SIZE    (64*1024)                
#define ARM_SECTION_SIZE       (1024*1024)             
#define ARM_SUPERSECTION_SIZE  (16*1024*1024)           

#define TLBI_ADDR_SHIFT (12)
#define TLBI_ADDR_SIZE  (20)
#define TLBI_ADDR_MASK  (((1ULL << TLBI_ADDR_SIZE) - 1))
#define TLBI_ASID_SHIFT (0)
#define TLBI_ASID_SIZE  (8)
#define TLBI_ASID_MASK  (((1ULL << TLBI_ASID_SIZE) - 1))
#endif

  
                                                            
   
#define ARM_DBGDSCR_RXFULL                 (1 << 30)
#define ARM_DBGDSCR_TXFULL                 (1 << 29)
#define ARM_DBGDSCR_RXFULL_1               (1 << 27)
#define ARM_DBGDSCR_TXFULL_1               (1 << 26)
#define ARM_DBGDSCR_PIPEADV                (1 << 25)
#define ARM_DBGDSCR_INSTRCOMPL_1           (1 << 24)
#define ARM_DBGDSCR_EXTDCCMODE_MASK        (3 << 20)
#define ARM_DBGDSCR_EXTDCCMODE_NONBLOCKING (0 << 20)
#define ARM_DBGDSCR_EXTDCCMODE_STALL       (1 << 20)
#define ARM_DBGDSCR_EXTDCCMODE_FAST        (1 << 20)
#define ARM_DBGDSCR_ADADISCARD             (1 << 19)
#define ARM_DBGDSCR_NS                     (1 << 18)
#define ARM_DBGDSCR_SPNIDDIS               (1 << 17)
#define ARM_DBGDSCR_SPIDDIS                (1 << 16)
#define ARM_DBGDSCR_MDBGEN                 (1 << 15)
#define ARM_DBGDSCR_HDBGEN                 (1 << 14)
#define ARM_DBGDSCR_ITREN                  (1 << 13)
#define ARM_DBGDSCR_UDCCDIS                (1 << 12)
#define ARM_DBGDSCR_INTDIS                 (1 << 11)
#define ARM_DBGDSCR_DBGACK                 (1 << 10)
#define ARM_DBGDSCR_DBGNOPWRDWN            (1 << 9)
#define ARM_DBGDSCR_UND_1                  (1 << 8)
#define ARM_DBGDSCR_ADABORT_1              (1 << 7)
#define ARM_DBGDSCR_SDABORT_1              (1 << 6)
#define ARM_DBGDSCR_MOE_MASK               (15 << 2)
#define ARM_DBGDSCR_MOE_HALT_REQUEST       (0 << 2)
#define ARM_DBGDSCR_MOE_BREAKPOINT         (1 << 2)
#define ARM_DBGDSCR_MOE_ASYNC_WATCHPOINT   (2 << 2)
#define ARM_DBGDSCR_MOE_BKPT_INSTRUCTION   (3 << 2)
#define ARM_DBGDSCR_MOE_EXT_DEBUG_REQ      (4 << 2)
#define ARM_DBGDSCR_MOE_VECTOR_CATCH       (5 << 2)
#define ARM_DBGDSCR_MOE_DSIDE_ABORT        (6 << 2)
#define ARM_DBGDSCR_MOE_ISIDE_ABORT        (7 << 2)
#define ARM_DBGDSCR_MOE_OS_UNLOCK_CATCH    (8 << 2)
#define ARM_DBGDSCR_MOE_SYNC_WATCHPOINT    (10 << 2)

#define ARM_DBGDSCR_RESTARTED              (1 << 1)
#define ARM_DBGDSCR_HALTED                 (1 << 0)

  
                                                                          
                                                          
   
#define ARM_DBG_VR_ADDRESS_MASK             0xFFFFFFFC                
#define ARM_DBGBVR_CONTEXTID_MASK           0xFFFFFFFF                

#define ARM_DBG_CR_ADDRESS_MASK_MASK        0x1F000000                
#define ARM_DBGBCR_MATCH_MASK               (1 << 22)                 
#define ARM_DBGBCR_MATCH_MATCH              (0 << 22)
#define ARM_DBGBCR_MATCH_MISMATCH           (1 << 22)
#define ARM_DBGBCR_TYPE_MASK                (1 << 21)                
#define ARM_DBGBCR_TYPE_IVA                 (0 << 21)
#define ARM_DBGBCR_TYPE_CONTEXTID           (1 << 21)
#define ARM_DBG_CR_LINKED_MASK              (1 << 20)                 
#define ARM_DBG_CR_LINKED_LINKED            (1 << 20)
#define ARM_DBG_CR_LINKED_UNLINKED          (0 << 20)
#define ARM_DBG_CR_LINKED_BRP_MASK          0x000F0000                
#define ARM_DBG_CR_SECURITY_STATE_MASK      (3 << 14)                 
#define ARM_DBG_CR_SECURITY_STATE_BOTH      (0 << 14)
#define ARM_DBG_CR_SECURITY_STATE_NONSECURE (1 << 14)
#define ARM_DBG_CR_SECURITY_STATE_SECURE    (2 << 14)
#define ARM_DBG_CR_HIGHER_MODE_MASK         (1 << 13)                 
#define ARM_DBG_CR_HIGHER_MODE_ENABLE       (1 << 13)
#define ARM_DBG_CR_HIGHER_MODE_DISABLE      (0 << 13)
#define ARM_DBGWCR_BYTE_ADDRESS_SELECT_MASK 0x00001FE0                
#define ARM_DBG_CR_BYTE_ADDRESS_SELECT_MASK 0x000001E0                
#define ARM_DBGWCR_ACCESS_CONTROL_MASK      (3 << 3)                 
#define ARM_DBCWCR_ACCESS_CONTROL_LOAD      (1 << 3)
#define ARM_DBCWCR_ACCESS_CONTROL_STORE     (2 << 3)
#define ARM_DBCWCR_ACCESS_CONTROL_ANY       (3 << 3)
#define ARM_DBG_CR_MODE_CONTROL_MASK        (3 << 1)                  
#define ARM_DBG_CR_MODE_CONTROL_U_S_S       (0 << 1)                  
#define ARM_DBG_CR_MODE_CONTROL_PRIVILEGED  (1 << 1)                  
#define ARM_DBG_CR_MODE_CONTROL_USER        (2 << 1)                  
#define ARM_DBG_CR_MODE_CONTROL_ANY         (3 << 1)                  
#define ARM_DBG_CR_ENABLE_MASK              (1 << 0)                  
#define ARM_DBG_CR_ENABLE_ENABLE            (1 << 0)
#define ARM_DBG_CR_ENABLE_DISABLE           (0 << 0)

  
                                                                      
   
#define ARM_DBGPRSR_STICKY_RESET_STATUS     (1 << 3)
#define ARM_DBGPRSR_RESET_STATUS            (1 << 2)
#define ARM_DBGPRSR_STICKY_POWERDOWN_STATUS (1 << 1)
#define ARM_DBGPRSR_POWERUP_STATUS          (1 << 0)

  
                                                                             
   
#define ARM_DBG_LOCK_ACCESS_KEY 0xC5ACCE55

                              
#define ARM_DEBUG_OFFSET_DBGDIDR       (0x000)
#define ARM_DEBUG_OFFSET_DBGWFAR       (0x018)
#define ARM_DEBUG_OFFSET_DBGVCR        (0x01C)
#define ARM_DEBUG_OFFSET_DBGECR        (0x024)
#define ARM_DEBUG_OFFSET_DBGDSCCR      (0x028)
#define ARM_DEBUG_OFFSET_DBGDSMCR      (0x02C)
#define ARM_DEBUG_OFFSET_DBGDTRRX      (0x080)
#define ARM_DEBUG_OFFSET_DBGITR        (0x084)                 
#define ARM_DEBUG_OFFSET_DBGPCSR       (0x084)                
#define ARM_DEBUG_OFFSET_DBGDSCR       (0x088)
#define ARM_DEBUG_OFFSET_DBGDTRTX      (0x08C)
#define ARM_DEBUG_OFFSET_DBGDRCR       (0x090)
#define ARM_DEBUG_OFFSET_DBGBVR        (0x100)                    
#define ARM_DEBUG_OFFSET_DBGBCR        (0x140)                    
#define ARM_DEBUG_OFFSET_DBGWVR        (0x180)                    
#define ARM_DEBUG_OFFSET_DBGWCR        (0x1C0)                    
#define ARM_DEBUG_OFFSET_DBGOSLAR      (0x300)
#define ARM_DEBUG_OFFSET_DBGOSLSR      (0x304)
#define ARM_DEBUG_OFFSET_DBGOSSRR      (0x308)
#define ARM_DEBUG_OFFSET_DBGPRCR       (0x310)
#define ARM_DEBUG_OFFSET_DBGPRSR       (0x314)
#define ARM_DEBUG_OFFSET_DBGITCTRL     (0xF00)
#define ARM_DEBUG_OFFSET_DBGCLAIMSET   (0xFA0)
#define ARM_DEBUG_OFFSET_DBGCLAIMCLR   (0xFA4)
#define ARM_DEBUG_OFFSET_DBGLAR        (0xFB0)
#define ARM_DEBUG_OFFSET_DBGLSR        (0xFB4)
#define ARM_DEBUG_OFFSET_DBGAUTHSTATUS (0xFB8)
#define ARM_DEBUG_OFFSET_DBGDEVID      (0xFC8)
#define ARM_DEBUG_OFFSET_DBGDEVTYPE    (0xFCC)
#define ARM_DEBUG_OFFSET_DBGPID0       (0xFD0)
#define ARM_DEBUG_OFFSET_DBGPID1       (0xFD4)
#define ARM_DEBUG_OFFSET_DBGPID2       (0xFD8)
#define ARM_DEBUG_OFFSET_DBGPID3       (0xFDA)
#define ARM_DEBUG_OFFSET_DBGPID4       (0xFDC)
#define ARM_DEBUG_OFFSET_DBGCID0       (0xFF0)
#define ARM_DEBUG_OFFSET_DBGCID1       (0xFF4)
#define ARM_DEBUG_OFFSET_DBGCID2       (0xFF8)
#define ARM_DEBUG_OFFSET_DBGCID3       (0xFFA)
#define ARM_DEBUG_OFFSET_DBGCID4       (0xFFC)

  
                                           
   
#define MVFR_ASIMD_HPFP 0x00100000UL

  
                          
  
                                   
                                    
                                    
                                    
  
         
                           
                         
                            
                              
                                
   
#define MIDR_REV_SHIFT  0
#define MIDR_REV_MASK   (0xf << MIDR_REV_SHIFT)
#define MIDR_PNUM_SHIFT 4
#define MIDR_PNUM_MASK  (0xfff << MIDR_PNUM_SHIFT)
#define MIDR_ARCH_SHIFT 16
#define MIDR_ARCH_MASK  (0xf << MIDR_ARCH_SHIFT)
#define MIDR_VAR_SHIFT  20
#define MIDR_VAR_MASK   (0xf << MIDR_VAR_SHIFT)
#define MIDR_IMP_SHIFT  24
#define MIDR_IMP_MASK   (0xff << MIDR_IMP_SHIFT)

#ifdef __arm__

                                                                   
#define MRC_SCTLR    15,0,1,0,0
#define MCR_SCTLR(x) 15,0,(x),1,0,0

#define MRC_ACTLR    15,0,1,0,1
#define MCR_ACTLR(x) 15,0,(x),1,0,1

#endif              


#endif                       
