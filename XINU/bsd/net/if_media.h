/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
 * Copyright (c) 1997
 *	Jonathan Stone and Jason R. Thorpe.  All rights reserved.
 *
 * This software is derived from information provided by Matt Thomas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jonathan Stone
 *	and Jason R. Thorpe for the NetBSD Project.
 * 4. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NET_IF_MEDIA_H_
#define _NET_IF_MEDIA_H_
#ifndef DRIVERKIT
#include <sys/appleapiopts.h>
#endif                

  
                                                                     
                   
  
                                                                       
                                                                   
                                   
  
                                                                     
                               
   

#ifdef KERNEL_PRIVATE
                                                       
#include <sys/queue.h>
#endif                     

  
                         
           
               
                    
                     
                             
                                    
                                
                 
   

  
           
  
                                                                          
               
  
                                                                      
                                                                    
                                              
   
#define IFM_X(x) IFM_X_SUBTYPE(x)                           

#define IFM_ETHER       0x00000020
#define IFM_10_T        3                                   
#define IFM_10_2        4                                      
#define IFM_10_5        5                                  
#define IFM_100_TX      6                                     
#define IFM_100_FX      7                                      
#define IFM_100_T4      8                                             
#define IFM_100_VG      9                                 
#define IFM_100_T2      10                             
#define IFM_1000_SX     11                                                 
#define IFM_10_STP      12                                            
#define IFM_10_FL       13                                    
#define IFM_1000_LX     14                                                  
#define IFM_1000_CX     15                                           
#define IFM_1000_T      16                                            
#ifdef PRIVATE
#define IFM_1000_TX     IFM_1000_T                             
#endif              
#define IFM_HPNA_1      17                                       
#define IFM_10G_SR      18                                                
#define IFM_10G_LR      19                                                 
#define IFM_10G_CX4     20                                       
#define IFM_10G_T       21                                           
#define IFM_2500_T      22                                            
#define IFM_5000_T      23                                            
#define IFM_1000_CX_SGMII    24                                
#define IFM_1000_KX     25                                         
#define IFM_10G_KX4     26                                         
#define IFM_10G_KR      27                                        
#define IFM_10G_CR1     28                                               
#define IFM_10G_ER      29                              
#define IFM_20G_KR2     30                                         
#define IFM_OTHER       31                                               

                                                                                     
#define IFM_2500_SX     IFM_X(32)                                          
#define IFM_10G_TWINAX  IFM_X(33)                                  
#define IFM_10G_TWINAX_LONG     IFM_X(34)                                       
#define IFM_10G_LRM     IFM_X(35)                                         
#define IFM_2500_KX     IFM_X(36)                                  
#define IFM_40G_CR4     IFM_X(37)                        
#define IFM_40G_SR4     IFM_X(38)                        
#define IFM_50G_PCIE    IFM_X(39)                                   
#define IFM_25G_PCIE    IFM_X(40)                                   
#define IFM_1000_SGMII  IFM_X(41)                               
#define IFM_10G_SFI     IFM_X(42)                                
#define IFM_40G_XLPPI   IFM_X(43)                                
#define IFM_40G_LR4     IFM_X(44)                        
#define IFM_40G_KR4     IFM_X(45)                        
#define IFM_100G_CR4    IFM_X(47)                         
#define IFM_100G_SR4    IFM_X(48)                         
#define IFM_100G_KR4    IFM_X(49)                         
#define IFM_100G_LR4    IFM_X(50)                         
#define IFM_56G_R4      IFM_X(51)                       
#define IFM_100_T       IFM_X(52)                            
#define IFM_25G_CR      IFM_X(53)                       
#define IFM_25G_KR      IFM_X(54)                       
#define IFM_25G_SR      IFM_X(55)                       
#define IFM_50G_CR2     IFM_X(56)                        
#define IFM_50G_KR2     IFM_X(57)                        
#define IFM_25G_LR      IFM_X(58)                       
#define IFM_10G_AOC     IFM_X(59)                                     
#define IFM_25G_ACC     IFM_X(60)                                    
#define IFM_25G_AOC     IFM_X(61)                                     
#define IFM_100_SGMII   IFM_X(62)                                 
#define IFM_2500_X      IFM_X(63)                      
#define IFM_5000_KR     IFM_X(64)                                
#define IFM_25G_T       IFM_X(65)                             
#define IFM_25G_CR_S    IFM_X(66)                               
#define IFM_25G_CR1     IFM_X(67)                                 
#define IFM_25G_KR_S    IFM_X(68)                               
#define IFM_5000_KR_S   IFM_X(69)                                        
#define IFM_5000_KR1    IFM_X(70)                                
#define IFM_25G_AUI     IFM_X(71)                                       
#define IFM_40G_XLAUI   IFM_X(72)                      
#define IFM_40G_XLAUI_AC IFM_X(73)                                     
#define IFM_40G_ER4     IFM_X(74)                        
#define IFM_50G_SR2     IFM_X(75)                        
#define IFM_50G_LR2     IFM_X(76)                        
#define IFM_50G_LAUI2_AC IFM_X(77)                                     
#define IFM_50G_LAUI2   IFM_X(78)                      
#define IFM_50G_AUI2_AC IFM_X(79)                                      
#define IFM_50G_AUI2    IFM_X(80)                     
#define IFM_50G_CP      IFM_X(81)                       
#define IFM_50G_SR      IFM_X(82)                       
#define IFM_50G_LR      IFM_X(83)                       
#define IFM_50G_FR      IFM_X(84)                       
#define IFM_50G_KR_PAM4 IFM_X(85)                            
#define IFM_25G_KR1     IFM_X(86)                        
#define IFM_50G_AUI1_AC IFM_X(87)                                      
#define IFM_50G_AUI1    IFM_X(88)                     
#define IFM_100G_CAUI4_AC IFM_X(89)                                           
#define IFM_100G_CAUI4 IFM_X(90)                        
#define IFM_100G_AUI4_AC IFM_X(91)                                           
#define IFM_100G_AUI4   IFM_X(92)                      
#define IFM_100G_CR_PAM4 IFM_X(93)                            
#define IFM_100G_KR_PAM4 IFM_X(94)                            
#define IFM_100G_CP2    IFM_X(95)                         
#define IFM_100G_SR2    IFM_X(96)                         
#define IFM_100G_DR     IFM_X(97)                        
#define IFM_100G_KR2_PAM4 IFM_X(98)                            
#define IFM_100G_CAUI2_AC IFM_X(99)                                           
#define IFM_100G_CAUI2  IFM_X(100)                      
#define IFM_100G_AUI2_AC IFM_X(101)                                          
#define IFM_100G_AUI2   IFM_X(102)                     
#define IFM_200G_CR4_PAM4 IFM_X(103)                           
#define IFM_200G_SR4    IFM_X(104)                        
#define IFM_200G_FR4    IFM_X(105)                        
#define IFM_200G_LR4    IFM_X(106)                        
#define IFM_200G_DR4    IFM_X(107)                        
#define IFM_200G_KR4_PAM4 IFM_X(108)                           
#define IFM_200G_AUI4_AC IFM_X(109)                                          
#define IFM_200G_AUI4   IFM_X(110)                     
#define IFM_200G_AUI8_AC IFM_X(111)                                          
#define IFM_200G_AUI8   IFM_X(112)                     
#define IFM_400G_FR8    IFM_X(113)                        
#define IFM_400G_LR8    IFM_X(114)                        
#define IFM_400G_DR4    IFM_X(115)                        
#define IFM_400G_AUI8_AC IFM_X(116)                                          
#define IFM_400G_AUI8   IFM_X(117)                     

  
             
   
#define IFM_TOKEN       0x00000040
#define IFM_TOK_STP4    3                                                   
#define IFM_TOK_STP16   4                                                    
#define IFM_TOK_UTP4    5                                                      
#define IFM_TOK_UTP16   6                                                       
#define IFM_TOK_STP100  7                                                     
#define IFM_TOK_UTP100  8                                                        
#define IFM_TOK_ETR     0x00000200                               
#define IFM_TOK_SRCRT   0x00000400                                          
#define IFM_TOK_ALLR    0x00000800                                           
#define IFM_TOK_DTR     0x00002000                                
#define IFM_TOK_CLASSIC 0x00004000                              
#define IFM_TOK_AUTO    0x00008000                                                 

  
       
   
#define IFM_FDDI        0x00000060
#define IFM_FDDI_SMF    3                                      
#define IFM_FDDI_MMF    4                                     
#define IFM_FDDI_UTP    5                               
#define IFM_FDDI_DA     0x00000100                                       

  
                       
   
#define IFM_IEEE80211   0x00000080
#define IFM_IEEE80211_FH1       3                                    
#define IFM_IEEE80211_FH2       4                                    
#define IFM_IEEE80211_DS2       5                                  
#define IFM_IEEE80211_DS5       6                                 
#define IFM_IEEE80211_DS11      7                                  
#define IFM_IEEE80211_DS1       8                                  
#define IFM_IEEE80211_DS22      9                                   
#define IFM_IEEE80211_ADHOC     0x00000100                                 

  
                         
   
#define IFM_AUTO        0                                          
#define IFM_MANUAL      1                                                   
#define IFM_NONE        2                                       

  
                 
   
#define IFM_FDX         0x00100000                             
#define IFM_HDX         0x00200000                             
#define IFM_FLOW        0x00400000                                        
#define IFM_EEE         0x00800000                                             
#define IFM_FLAG0       0x01000000                               
#define IFM_FLAG1       0x02000000                               
#define IFM_FLAG2       0x04000000                               
#define IFM_LOOP        0x08000000                                    

  
                                                                     
   
#define IFM_TMASK_COMPAT        0x0000001f                                        
#define IFM_TMASK_EXT           0x000f0000                                       
#define IFM_TMASK_EXT_SHIFT     11                                        
#define IFM_X_SUBTYPE(x) (((x) & IFM_TMASK_COMPAT) | \
	(((x) & (IFM_TMASK_EXT >> IFM_TMASK_EXT_SHIFT)) << IFM_TMASK_EXT_SHIFT))

  
        
   
#define IFM_NMASK       0x000000e0                        
#define IFM_TMASK       (IFM_TMASK_COMPAT|IFM_TMASK_EXT)                        
#define IFM_IMASK       0xf0000000                    
#define IFM_ISHIFT      28                                  
#define IFM_OMASK       0x0000ff00                                 
#define IFM_GMASK       0x0ff00000                          

  
              
   
#define IFM_AVALID      0x00000001                            
#define IFM_ACTIVE      0x00000002                                             
#define IFM_WAKESAMENET 0x00000004                                           

  
                                                                     
   
#define IFM_TYPE(x)         ((x) & IFM_NMASK)
#define IFM_SUBTYPE(x)      ((x) & IFM_TMASK)
#define IFM_TYPE_OPTIONS(x) ((x) & IFM_OMASK)
#define IFM_INST(x)         (((x) & IFM_IMASK) >> IFM_ISHIFT)
#define IFM_OPTIONS(x)  ((x) & (IFM_OMASK|IFM_GMASK))

#define IFM_INST_MAX    IFM_INST(IFM_IMASK)

  
                                
   
#define IFM_MAKEWORD(type, subtype, options, instance)                  \
	((type) | (subtype) | (options) | ((instance) << IFM_ISHIFT))

  
                                                                         
                                                                    
  
                                                                            
                                                                
   
struct ifmedia_description {
	int     ifmt_word;                                             
	const char *ifmt_string;                         
};

#define IFM_TYPE_DESCRIPTIONS {                     \
    { IFM_ETHER,     "Ethernet"   },                \
    { IFM_TOKEN,     "Token ring" },                \
    { IFM_FDDI,      "FDDI"       },                \
    { IFM_IEEE80211, "IEEE802.11" },                \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_ETHERNET_DESCRIPTIONS {         \
    { IFM_10_T,     "10baseT/UTP" },                \
    { IFM_10_2,     "10base2/BNC" },                \
    { IFM_10_5,     "10base5/AUI" },                \
    { IFM_100_TX,   "100baseTX"   },                \
    { IFM_100_FX,   "100baseFX"   },                \
    { IFM_100_T4,   "100baseT4"   },                \
    { IFM_100_VG,   "100baseVG"   },                \
    { IFM_100_T2,   "100baseT2"   },                \
    { IFM_10_STP,   "10baseSTP"   },                \
    { IFM_10_FL,    "10baseFL"    },                \
    { IFM_1000_SX,              "1000baseSX" },                         \
    { IFM_1000_LX,  "1000baseLX"  },                \
    { IFM_1000_CX,  "1000baseCX"  },                \
    { IFM_1000_T,   "1000baseT"   },                \
    { IFM_HPNA_1,               "homePNA" },                            \
    { IFM_10G_LR,               "10Gbase-LR" },                         \
    { IFM_10G_SR,               "10Gbase-SR" },                         \
    { IFM_10G_CX4,              "10Gbase-CX4" },                        \
    { IFM_2500_SX,              "2500BaseSX" },                         \
    { IFM_10G_LRM,              "10Gbase-LRM" },                        \
    { IFM_10G_TWINAX,           "10Gbase-Twinax" },                     \
    { IFM_10G_TWINAX_LONG,      "10Gbase-Twinax-Long" },                \
    { IFM_10G_T,                "10Gbase-T" },                          \
    { IFM_40G_CR4,              "40Gbase-CR4" },                        \
    { IFM_40G_SR4,              "40Gbase-SR4" },                        \
    { IFM_40G_LR4,              "40Gbase-LR4" },                        \
    { IFM_1000_KX,  "1000Base-KX" },                \
    { IFM_OTHER,                "Other" },                              \
    { IFM_10G_KX4,  "10GBase-KX4" },                \
    { IFM_10G_KR,   "10GBase-KR" },                 \
    { IFM_10G_CR1,  "10GBase-CR1" },                \
    { IFM_20G_KR2,              "20GBase-KR2" },                        \
    { IFM_2500_KX,              "2500Base-KX" },                        \
    { IFM_2500_T,               "2500Base-T" },                         \
    { IFM_5000_T,               "5000Base-T" },                         \
    { IFM_50G_PCIE,             "PCIExpress-50G" },                     \
    { IFM_25G_PCIE,             "PCIExpress-25G" },                     \
    { IFM_1000_SGMII,           "1000Base-SGMII" },                     \
    { IFM_10G_SFI,              "10GBase-SFI" },                        \
    { IFM_40G_XLPPI,            "40GBase-XLPPI" },                      \
    { IFM_1000_CX_SGMII,        "1000Base-CX-SGMII" },                  \
    { IFM_40G_KR4,              "40GBase-KR4" },                        \
    { IFM_10G_ER,   "10GBase-ER" },                 \
    { IFM_100G_CR4,             "100GBase-CR4" },                       \
    { IFM_100G_SR4,             "100GBase-SR4" },                       \
    { IFM_100G_KR4,             "100GBase-KR4" },                       \
    { IFM_100G_LR4,             "100GBase-LR4" },                       \
    { IFM_56G_R4,               "56GBase-R4" },                         \
    { IFM_100_T,                "100BaseT" },                           \
    { IFM_25G_CR,   "25GBase-CR" },                 \
    { IFM_25G_KR,   "25GBase-KR" },                 \
    { IFM_25G_SR,   "25GBase-SR" },                 \
    { IFM_50G_CR2,  "50GBase-CR2" },                \
    { IFM_50G_KR2,  "50GBase-KR2" },                \
    { IFM_25G_LR,               "25GBase-LR" },                         \
    { IFM_10G_AOC,              "10GBase-AOC" },                        \
    { IFM_25G_ACC,              "25GBase-ACC" },                        \
    { IFM_25G_AOC,              "25GBase-AOC" },                        \
    { IFM_100_SGMII,            "100M-SGMII" },                         \
    { IFM_2500_X,               "2500Base-X" },                         \
    { IFM_5000_KR,              "5000Base-KR" },                        \
    { IFM_25G_T,                "25GBase-T" },                          \
    { IFM_25G_CR_S,             "25GBase-CR-S" },                       \
    { IFM_25G_CR1,              "25GBase-CR1" },                        \
    { IFM_25G_KR_S,             "25GBase-KR-S" },                       \
    { IFM_5000_KR_S,            "5000Base-KR-S" },                      \
    { IFM_5000_KR1,             "5000Base-KR1" },                       \
    { IFM_25G_AUI,              "25G-AUI" },                            \
    { IFM_40G_XLAUI,            "40G-XLAUI" },                          \
    { IFM_40G_XLAUI_AC,         "40G-XLAUI-AC" },                       \
    { IFM_40G_ER4,              "40GBase-ER4" },                        \
    { IFM_50G_SR2,  "50GBase-SR2" },                \
    { IFM_50G_LR2,  "50GBase-LR2" },                \
    { IFM_50G_LAUI2_AC,         "50G-LAUI2-AC" },                       \
    { IFM_50G_LAUI2,            "50G-LAUI2" },                          \
    { IFM_50G_AUI2_AC,          "50G-AUI2-AC" },                        \
    { IFM_50G_AUI2,             "50G-AUI2" },                           \
    { IFM_50G_CP,               "50GBase-CP" },                         \
    { IFM_50G_SR,               "50GBase-SR" },                         \
    { IFM_50G_LR,               "50GBase-LR" },                         \
    { IFM_50G_FR,               "50GBase-FR" },                         \
    { IFM_50G_KR_PAM4,          "50GBase-KR-PAM4" },                    \
    { IFM_25G_KR1,              "25GBase-KR1" },                        \
    { IFM_50G_AUI1_AC,          "50G-AUI1-AC" },                        \
    { IFM_50G_AUI1,             "50G-AUI1" },                           \
    { IFM_100G_CAUI4_AC,        "100G-CAUI4-AC" },                      \
    { IFM_100G_CAUI4,           "100G-CAUI4" },                         \
    { IFM_100G_AUI4_AC,         "100G-AUI4-AC" },                       \
    { IFM_100G_AUI4,            "100G-AUI4" },                          \
    { IFM_100G_CR_PAM4,         "100GBase-CR-PAM4" },                   \
    { IFM_100G_KR_PAM4,         "100GBase-KR-PAM4" },                   \
    { IFM_100G_CP2,             "100GBase-CP2" },                       \
    { IFM_100G_SR2,             "100GBase-SR2" },                       \
    { IFM_100G_DR,              "100GBase-DR" },                        \
    { IFM_100G_KR2_PAM4,        "100GBase-KR2-PAM4" },                  \
    { IFM_100G_CAUI2_AC,        "100G-CAUI2-AC" },                      \
    { IFM_100G_CAUI2,           "100G-CAUI2" },                         \
    { IFM_100G_AUI2_AC,         "100G-AUI2-AC" },                       \
    { IFM_100G_AUI2,            "100G-AUI2" },                          \
    { IFM_200G_CR4_PAM4,        "200GBase-CR4-PAM4" },                  \
    { IFM_200G_SR4,             "200GBase-SR4" },                       \
    { IFM_200G_FR4,             "200GBase-FR4" },                       \
    { IFM_200G_LR4,             "200GBase-LR4" },                       \
    { IFM_200G_DR4,             "200GBase-DR4" },                       \
    { IFM_200G_KR4_PAM4,        "200GBase-KR4-PAM4" },                  \
    { IFM_200G_AUI4_AC,         "200G-AUI4-AC" },                       \
    { IFM_200G_AUI4,            "200G-AUI4" },                          \
    { IFM_200G_AUI8_AC,         "200G-AUI8-AC" },                       \
    { IFM_200G_AUI8,            "200G-AUI8" },                          \
    { IFM_400G_FR8,             "400GBase-FR8" },                       \
    { IFM_400G_LR8,             "400GBase-LR8" },                       \
    { IFM_400G_DR4,             "400GBase-DR4" },                       \
    { IFM_400G_AUI8_AC,         "400G-AUI8-AC" },                       \
    { IFM_400G_AUI8,            "400G-AUI8" },                          \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_ETHERNET_ALIASES {              \
    { IFM_10_T,     "UTP"    },                     \
    { IFM_10_T,     "10UTP"  },                     \
    { IFM_10_2,     "BNC"    },                     \
    { IFM_10_2,     "10BNC"  },                     \
    { IFM_10_5,     "AUI"    },                     \
    { IFM_10_5,     "10AUI"  },                     \
    { IFM_100_TX,   "100TX"  },                     \
    { IFM_100_FX,   "100FX"  },                     \
    { IFM_100_T4,   "100T4"  },                     \
    { IFM_100_VG,   "100VG"  },                     \
    { IFM_100_T2,   "100T2"  },                     \
    { IFM_1000_SX,  "1000SX" },                     \
    { IFM_10_STP,   "STP"    },                     \
    { IFM_10_STP,   "10STP"  },                     \
    { IFM_10_FL,    "FL"     },                     \
    { IFM_10_FL,    "10FL"   },                     \
    { IFM_1000_LX,  "1000LX" },                     \
    { IFM_1000_CX,  "1000CX" },                     \
    { IFM_1000_T,   "1000T"  },                     \
    { IFM_HPNA_1,   "HPNA1"  },                     \
    { IFM_10G_SR,   "10GSR"  },                     \
    { IFM_10G_LR,   "10GLR"  },                     \
    { IFM_10G_CX4,  "10GCX4" },                     \
    { IFM_10G_T,    "10GT"   },                     \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_ETHERNET_OPTION_DESCRIPTIONS {  \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_TOKENRING_DESCRIPTIONS {        \
    { IFM_TOK_STP4,  "DB9/4Mbit" },                 \
    { IFM_TOK_STP16, "DB9/16Mbit" },                \
    { IFM_TOK_UTP4,  "UTP/4Mbit" },                 \
    { IFM_TOK_UTP16, "UTP/16Mbit" },                \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_TOKENRING_ALIASES {             \
    { IFM_TOK_STP4,  "4STP" },                      \
    { IFM_TOK_STP16, "16STP" },                     \
    { IFM_TOK_UTP4,  "4UTP" },                      \
    { IFM_TOK_UTP16, "16UTP" },                     \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_TOKENRING_OPTION_DESCRIPTIONS { \
    { IFM_TOK_ETR,   "EarlyTokenRelease" },         \
    { IFM_TOK_SRCRT, "SourceRouting" },             \
    { IFM_TOK_ALLR,  "AllRoutes" },                 \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_FDDI_DESCRIPTIONS {             \
    { IFM_FDDI_SMF, "Single-mode" },                \
    { IFM_FDDI_MMF, "Multi-mode" },                 \
    { IFM_FDDI_UTP, "UTP" },                        \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_FDDI_ALIASES {                  \
    { IFM_FDDI_SMF, "SMF" },                        \
    { IFM_FDDI_MMF, "MMF" },                        \
    { IFM_FDDI_UTP, "CDDI" },                       \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_FDDI_OPTION_DESCRIPTIONS {      \
    { IFM_FDDI_DA,  "Dual-attach" },                \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_IEEE80211_DESCRIPTIONS {        \
    { IFM_IEEE80211_FH1,  "FH1"  },                 \
    { IFM_IEEE80211_FH2,  "FH2"  },                 \
    { IFM_IEEE80211_DS1,  "DS1"  },                 \
    { IFM_IEEE80211_DS2,  "DS2"  },                 \
    { IFM_IEEE80211_DS5,  "DS5"  },                 \
    { IFM_IEEE80211_DS11, "DS11" },                 \
    { IFM_IEEE80211_DS22, "DS22" },                 \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_IEEE80211_OPTION_DESCRIPTIONS { \
    { IFM_IEEE80211_ADHOC,  "adhoc" },              \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_SHARED_DESCRIPTIONS {           \
    { IFM_AUTO,     "autoselect" },                 \
    { IFM_MANUAL,   "manual" },                     \
    { IFM_NONE,     "none" },                       \
    { 0, NULL },                                    \
}

#define IFM_SUBTYPE_SHARED_ALIASES {                \
    { IFM_AUTO,     "auto" },                       \
    { 0, NULL },                                    \
}

#define IFM_SHARED_OPTION_DESCRIPTIONS {            \
    { IFM_FDX,      "full-duplex" },                \
    { IFM_HDX,      "half-duplex" },                \
    { IFM_FLOW,     "flow-control" },               \
    { IFM_EEE,	    "energy-efficient-ethernet" },  \
    { IFM_FLAG0,    "flag0" },                      \
    { IFM_FLAG1,    "flag1" },                      \
    { IFM_FLAG2,    "flag2" },                      \
    { IFM_LOOP,     "hw-loopback" },                \
    { 0, NULL },                                    \
}

#endif                        
