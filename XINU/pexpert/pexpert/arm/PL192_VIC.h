/*
 * Copyright (c) 2005-2007 Apple Inc. All rights reserved.
 */

#ifndef _PEXPERT_ARM_PL192_VIC_H
#define _PEXPERT_ARM_PL192_VIC_H

#define ARM_CELL_PL192_VIC

      
#define rVICIRQSTATUS           (*(volatile unsigned *)(pic_base + 0x000))                           
#define rVICFIQSTATUS           (*(volatile unsigned *)(pic_base + 0x004))                           
#define rVICRAWINTR             (*(volatile unsigned *)(pic_base + 0x008))                                     
#define rVICINTSELECT           (*(volatile unsigned *)(pic_base + 0x00C))                                 
#define rVICINTENABLE           (*(volatile unsigned *)(pic_base + 0x010))                                 
#define rVICINTENCLEAR          (*(volatile unsigned *)(pic_base + 0x014))                                        
#define rVICSOFTINT             (*(volatile unsigned *)(pic_base + 0x018))                               
#define rVICSOFTINTCLEAR        (*(volatile unsigned *)(pic_base + 0x01C))                                     
#define rVICPROTECTION          (*(volatile unsigned *)(pic_base + 0x020))                           
#define rVICSWPRIORITYMASK      (*(volatile unsigned *)(pic_base + 0x024))                                       
#define rVICPRIORITYDAISY       (*(volatile unsigned *)(pic_base + 0x028))                                     
#define rVICVECTOR(x)           (*(volatile unsigned *)(pic_base + 0x100 + 4 * (x)))                        
#define rVICVECTPRIORITY(x)     (*(volatile unsigned *)(pic_base + 0x200 + 4 * (x)))                                 
#define rVICPERIPHID0           (*(volatile unsigned *)(pic_base + 0xFE0))                                
#define rVICPERIPHID1           (*(volatile unsigned *)(pic_base + 0xFE4))                                
#define rVICPERIPHID2           (*(volatile unsigned *)(pic_base + 0xFE8))                                
#define rVICPERIPHID3           (*(volatile unsigned *)(pic_base + 0xFEC))                                
#define rVICPCELLID0            (*(volatile unsigned *)(pic_base + 0xFF0))                               
#define rVICPCELLID1            (*(volatile unsigned *)(pic_base + 0xFF4))                               
#define rVICPCELLID2            (*(volatile unsigned *)(pic_base + 0xFF8))                               
#define rVICPCELLID3            (*(volatile unsigned *)(pic_base + 0xFFC))                               

#endif                                 
