/*
 * Copyright (c) 2005-2007 Apple Inc. All rights reserved.
 */

                                                                               
                         
                                              
                                 
                           
                  
          
                                                      
                                                                                
                                                                                
                                                                               

#ifndef __2410ADDR_H__
#define __2410ADDR_H__

#define ARM_BOARD_CONFIG_S3C2410

#ifdef __cplusplus
extern "C" {
#endif

#include <pexpert/arm/S3cUART.h>

#if 0
#define _ISR_STARTADDRESS       0x30000000

                 
#define rBWSCON    (*(volatile unsigned *)0x48000000)                          
#define rBANKCON0  (*(volatile unsigned *)0x48000004)                   
#define rBANKCON1  (*(volatile unsigned *)0x48000008)                
#define rBANKCON2  (*(volatile unsigned *)0x4800000c)                 
#define rBANKCON3  (*(volatile unsigned *)0x48000010)                
#define rBANKCON4  (*(volatile unsigned *)0x48000014)                
#define rBANKCON5  (*(volatile unsigned *)0x48000018)                
#define rBANKCON6  (*(volatile unsigned *)0x4800001c)                
#define rBANKCON7  (*(volatile unsigned *)0x48000020)                
#define rREFRESH   (*(volatile unsigned *)0x48000024)                     
#define rBANKSIZE  (*(volatile unsigned *)0x48000028)                     
#define rMRSRB6    (*(volatile unsigned *)0x4800002c)                              
#define rMRSRB7    (*(volatile unsigned *)0x48000030)                              


           
#endif

            
#define rSRCPND     (*(volatile unsigned *)(pic_base + 0x00))                           
#define rINTMOD     (*(volatile unsigned *)(pic_base + 0x04))                         
#define rINTMSK     (*(volatile unsigned *)(pic_base + 0x08))                         
#define rPRIORITY   (*(volatile unsigned *)(pic_base + 0x0c))                       
#define rINTPND     (*(volatile unsigned *)(pic_base + 0x10))                           
#define rINTOFFSET  (*(volatile unsigned *)(pic_base + 0x14))                                  
#define rSUBSRCPND  (*(volatile unsigned *)(pic_base + 0x18))                     
#define rINTSUBMSK  (*(volatile unsigned *)(pic_base + 0x1c))                     

#if 0
      
#define rDISRC0     (*(volatile unsigned *)0x4b000000)                       
#define rDISRCC0    (*(volatile unsigned *)0x4b000004)                               
#define rDIDST0     (*(volatile unsigned *)0x4b000008)                            
#define rDIDSTC0    (*(volatile unsigned *)0x4b00000c)                                    
#define rDCON0      (*(volatile unsigned *)0x4b000010)                
#define rDSTAT0     (*(volatile unsigned *)0x4b000014)               
#define rDCSRC0     (*(volatile unsigned *)0x4b000018)                       
#define rDCDST0     (*(volatile unsigned *)0x4b00001c)                            
#define rDMASKTRIG0 (*(volatile unsigned *)0x4b000020)                     

#define rDISRC1     (*(volatile unsigned *)0x4b000040)                       
#define rDISRCC1    (*(volatile unsigned *)0x4b000044)                               
#define rDIDST1     (*(volatile unsigned *)0x4b000048)                            
#define rDIDSTC1    (*(volatile unsigned *)0x4b00004c)                                    
#define rDCON1      (*(volatile unsigned *)0x4b000050)                
#define rDSTAT1     (*(volatile unsigned *)0x4b000054)               
#define rDCSRC1     (*(volatile unsigned *)0x4b000058)                       
#define rDCDST1     (*(volatile unsigned *)0x4b00005c)                            
#define rDMASKTRIG1 (*(volatile unsigned *)0x4b000060)                     

#define rDISRC2     (*(volatile unsigned *)0x4b000080)                       
#define rDISRCC2    (*(volatile unsigned *)0x4b000084)                               
#define rDIDST2     (*(volatile unsigned *)0x4b000088)                            
#define rDIDSTC2    (*(volatile unsigned *)0x4b00008c)                                    
#define rDCON2      (*(volatile unsigned *)0x4b000090)                
#define rDSTAT2     (*(volatile unsigned *)0x4b000094)               
#define rDCSRC2     (*(volatile unsigned *)0x4b000098)                       
#define rDCDST2     (*(volatile unsigned *)0x4b00009c)                            
#define rDMASKTRIG2 (*(volatile unsigned *)0x4b0000a0)                     

#define rDISRC3     (*(volatile unsigned *)0x4b0000c0)                       
#define rDISRCC3    (*(volatile unsigned *)0x4b0000c4)                               
#define rDIDST3     (*(volatile unsigned *)0x4b0000c8)                            
#define rDIDSTC3    (*(volatile unsigned *)0x4b0000cc)                                    
#define rDCON3      (*(volatile unsigned *)0x4b0000d0)                
#define rDSTAT3     (*(volatile unsigned *)0x4b0000d4)               
#define rDCSRC3     (*(volatile unsigned *)0x4b0000d8)                       
#define rDCDST3     (*(volatile unsigned *)0x4b0000dc)                            
#define rDMASKTRIG3 (*(volatile unsigned *)0x4b0000e0)                     


                           
#define rLOCKTIME   (*(volatile unsigned *)0x4c000000)                        
#define rMPLLCON    (*(volatile unsigned *)0x4c000004)               
#define rUPLLCON    (*(volatile unsigned *)0x4c000008)               
#define rCLKCON     (*(volatile unsigned *)0x4c00000c)                          
#define rCLKSLOW    (*(volatile unsigned *)0x4c000010)                     
#define rCLKDIVN    (*(volatile unsigned *)0x4c000014)                        


                 
#define rLCDCON1    (*(volatile unsigned *)0x4d000000)                
#define rLCDCON2    (*(volatile unsigned *)0x4d000004)                
#define rLCDCON3    (*(volatile unsigned *)0x4d000008)                
#define rLCDCON4    (*(volatile unsigned *)0x4d00000c)                
#define rLCDCON5    (*(volatile unsigned *)0x4d000010)                
#define rLCDSADDR1  (*(volatile unsigned *)0x4d000014)                                       
#define rLCDSADDR2  (*(volatile unsigned *)0x4d000018)                                       
#define rLCDSADDR3  (*(volatile unsigned *)0x4d00001c)                                     
#define rREDLUT     (*(volatile unsigned *)0x4d000020)                       
#define rGREENLUT   (*(volatile unsigned *)0x4d000024)                         
#define rBLUELUT    (*(volatile unsigned *)0x4d000028)                        
#define rDITHMODE   (*(volatile unsigned *)0x4d00004c)                     
#define rTPAL       (*(volatile unsigned *)0x4d000050)                        
#define rLCDINTPND  (*(volatile unsigned *)0x4d000054)                        
#define rLCDSRCPND  (*(volatile unsigned *)0x4d000058)                       
#define rLCDINTMSK  (*(volatile unsigned *)0x4d00005c)                     
#define rLPCSEL     (*(volatile unsigned *)0x4d000060)                  
#define PALETTE     0x4d000400                                                


             
#define rNFCONF     (*(volatile unsigned *)0x4e000000)                                
#define rNFCMD      (*(volatile U8 *)0x4e000004)                                
#define rNFADDR     (*(volatile U8 *)0x4e000008)                                
#define rNFDATA     (*(volatile U8 *)0x4e00000c)                             
#define rNFSTAT     (*(volatile unsigned *)0x4e000010)                                   
#define rNFECC      (*(volatile unsigned *)0x4e000014)                      
#define rNFECC0     (*(volatile U8  *)0x4e000014)
#define rNFECC1     (*(volatile U8  *)0x4e000015)
#define rNFECC2     (*(volatile U8  *)0x4e000016)
#endif

            
#define rTCFG0  (*(volatile unsigned *)(timer_base + 0x00))                        
#define rTCFG1  (*(volatile unsigned *)(timer_base + 0x04))                        
#define rTCON   (*(volatile unsigned *)(timer_base + 0x08))                
#define rTCNTB0 (*(volatile unsigned *)(timer_base + 0x0c))                       
#define rTCMPB0 (*(volatile unsigned *)(timer_base + 0x10))                         
#define rTCNTO0 (*(volatile unsigned *)(timer_base + 0x14))                            
#define rTCNTB1 (*(volatile unsigned *)(timer_base + 0x18))                       
#define rTCMPB1 (*(volatile unsigned *)(timer_base + 0x1c))                         
#define rTCNTO1 (*(volatile unsigned *)(timer_base + 0x20))                            
#define rTCNTB2 (*(volatile unsigned *)(timer_base + 0x24))                       
#define rTCMPB2 (*(volatile unsigned *)(timer_base + 0x28))                         
#define rTCNTO2 (*(volatile unsigned *)(timer_base + 0x2c))                            
#define rTCNTB3 (*(volatile unsigned *)(timer_base + 0x30))                       
#define rTCMPB3 (*(volatile unsigned *)(timer_base + 0x34))                         
#define rTCNTO3 (*(volatile unsigned *)(timer_base + 0x38))                            
#define rTCNTB4 (*(volatile unsigned *)(timer_base + 0x3c))                       
#define rTCNTO4 (*(volatile unsigned *)(timer_base + 0x40))                            
#define rTCNTCLRINT0 (*(volatile unsigned *)(timer_base + 0x44))                                  
#define rTCNTCLRINT1 (*(volatile unsigned *)(timer_base + 0x48))                                  
#define rTCNTCLRINT2 (*(volatile unsigned *)(timer_base + 0x4C))                                  
#define rTCNTCLRINT3 (*(volatile unsigned *)(timer_base + 0x54))                                  
#define rTCNTCLRINT4 (*(volatile unsigned *)(timer_base + 0x54))                                  


#if 0
             
#ifdef __BIG_ENDIAN
< ERROR IF BIG_ENDIAN >
#define rFUNC_ADDR_REG     (*(volatile unsigned char *)0x52000143)                   
#define rPWR_REG           (*(volatile unsigned char *)0x52000147)                   
#define rEP_INT_REG        (*(volatile unsigned char *)0x5200014b)                                 
#define rUSB_INT_REG       (*(volatile unsigned char *)0x5200015b)                                  
#define rEP_INT_EN_REG     (*(volatile unsigned char *)0x5200015f)                   
#define rUSB_INT_EN_REG    (*(volatile unsigned char *)0x5200016f)
#define rFRAME_NUM1_REG    (*(volatile unsigned char *)0x52000173)                          
#define rFRAME_NUM2_REG    (*(volatile unsigned char *)0x52000177)                           
#define rINDEX_REG         (*(volatile unsigned char *)0x5200017b)                 
#define rMAXP_REG          (*(volatile unsigned char *)0x52000183)                      
#define rEP0_CSR           (*(volatile unsigned char *)0x52000187)                    
#define rIN_CSR1_REG       (*(volatile unsigned char *)0x52000187)                             
#define rIN_CSR2_REG       (*(volatile unsigned char *)0x5200018b)
#define rOUT_CSR1_REG      (*(volatile unsigned char *)0x52000193)                              
#define rOUT_CSR2_REG      (*(volatile unsigned char *)0x52000197)
#define rOUT_FIFO_CNT1_REG (*(volatile unsigned char *)0x5200019b)                           
#define rOUT_FIFO_CNT2_REG (*(volatile unsigned char *)0x5200019f)
#define rEP0_FIFO          (*(volatile unsigned char *)0x520001c3)                  
#define rEP1_FIFO          (*(volatile unsigned char *)0x520001c7)                  
#define rEP2_FIFO          (*(volatile unsigned char *)0x520001cb)                  
#define rEP3_FIFO          (*(volatile unsigned char *)0x520001cf)                  
#define rEP4_FIFO          (*(volatile unsigned char *)0x520001d3)                  
#define rEP1_DMA_CON       (*(volatile unsigned char *)0x52000203)                            
#define rEP1_DMA_UNIT      (*(volatile unsigned char *)0x52000207)                          
#define rEP1_DMA_FIFO      (*(volatile unsigned char *)0x5200020b)                          
#define rEP1_DMA_TTC_L     (*(volatile unsigned char *)0x5200020f)                           
#define rEP1_DMA_TTC_M     (*(volatile unsigned char *)0x52000213)
#define rEP1_DMA_TTC_H     (*(volatile unsigned char *)0x52000217)
#define rEP2_DMA_CON       (*(volatile unsigned char *)0x5200021b)                            
#define rEP2_DMA_UNIT      (*(volatile unsigned char *)0x5200021f)                          
#define rEP2_DMA_FIFO      (*(volatile unsigned char *)0x52000223)                          
#define rEP2_DMA_TTC_L     (*(volatile unsigned char *)0x52000227)                           
#define rEP2_DMA_TTC_M     (*(volatile unsigned char *)0x5200022b)
#define rEP2_DMA_TTC_H     (*(volatile unsigned char *)0x5200022f)
#define rEP3_DMA_CON       (*(volatile unsigned char *)0x52000243)                            
#define rEP3_DMA_UNIT      (*(volatile unsigned char *)0x52000247)                          
#define rEP3_DMA_FIFO      (*(volatile unsigned char *)0x5200024b)                          
#define rEP3_DMA_TTC_L     (*(volatile unsigned char *)0x5200024f)                           
#define rEP3_DMA_TTC_M     (*(volatile unsigned char *)0x52000253)
#define rEP3_DMA_TTC_H     (*(volatile unsigned char *)0x52000257)
#define rEP4_DMA_CON       (*(volatile unsigned char *)0x5200025b)                            
#define rEP4_DMA_UNIT      (*(volatile unsigned char *)0x5200025f)                          
#define rEP4_DMA_FIFO      (*(volatile unsigned char *)0x52000263)                          
#define rEP4_DMA_TTC_L     (*(volatile unsigned char *)0x52000267)                           
#define rEP4_DMA_TTC_M     (*(volatile unsigned char *)0x5200026b)
#define rEP4_DMA_TTC_H     (*(volatile unsigned char *)0x5200026f)

#else                  
#define rFUNC_ADDR_REG     (*(volatile unsigned char *)0x52000140)                   
#define rPWR_REG           (*(volatile unsigned char *)0x52000144)                   
#define rEP_INT_REG        (*(volatile unsigned char *)0x52000148)                                 
#define rUSB_INT_REG       (*(volatile unsigned char *)0x52000158)                                  
#define rEP_INT_EN_REG     (*(volatile unsigned char *)0x5200015c)                   
#define rUSB_INT_EN_REG    (*(volatile unsigned char *)0x5200016c)
#define rFRAME_NUM1_REG    (*(volatile unsigned char *)0x52000170)                          
#define rFRAME_NUM2_REG    (*(volatile unsigned char *)0x52000174)                           
#define rINDEX_REG         (*(volatile unsigned char *)0x52000178)                 
#define rMAXP_REG          (*(volatile unsigned char *)0x52000180)                      
#define rEP0_CSR           (*(volatile unsigned char *)0x52000184)                    
#define rIN_CSR1_REG       (*(volatile unsigned char *)0x52000184)                             
#define rIN_CSR2_REG       (*(volatile unsigned char *)0x52000188)
#define rOUT_CSR1_REG      (*(volatile unsigned char *)0x52000190)                              
#define rOUT_CSR2_REG      (*(volatile unsigned char *)0x52000194)
#define rOUT_FIFO_CNT1_REG (*(volatile unsigned char *)0x52000198)                           
#define rOUT_FIFO_CNT2_REG (*(volatile unsigned char *)0x5200019c)
#define rEP0_FIFO          (*(volatile unsigned char *)0x520001c0)                  
#define rEP1_FIFO          (*(volatile unsigned char *)0x520001c4)                  
#define rEP2_FIFO          (*(volatile unsigned char *)0x520001c8)                  
#define rEP3_FIFO          (*(volatile unsigned char *)0x520001cc)                  
#define rEP4_FIFO          (*(volatile unsigned char *)0x520001d0)                  
#define rEP1_DMA_CON       (*(volatile unsigned char *)0x52000200)                            
#define rEP1_DMA_UNIT      (*(volatile unsigned char *)0x52000204)                          
#define rEP1_DMA_FIFO      (*(volatile unsigned char *)0x52000208)                          
#define rEP1_DMA_TTC_L     (*(volatile unsigned char *)0x5200020c)                           
#define rEP1_DMA_TTC_M     (*(volatile unsigned char *)0x52000210)
#define rEP1_DMA_TTC_H     (*(volatile unsigned char *)0x52000214)
#define rEP2_DMA_CON       (*(volatile unsigned char *)0x52000218)                            
#define rEP2_DMA_UNIT      (*(volatile unsigned char *)0x5200021c)                          
#define rEP2_DMA_FIFO      (*(volatile unsigned char *)0x52000220)                          
#define rEP2_DMA_TTC_L     (*(volatile unsigned char *)0x52000224)                           
#define rEP2_DMA_TTC_M     (*(volatile unsigned char *)0x52000228)
#define rEP2_DMA_TTC_H     (*(volatile unsigned char *)0x5200022c)
#define rEP3_DMA_CON       (*(volatile unsigned char *)0x52000240)                            
#define rEP3_DMA_UNIT      (*(volatile unsigned char *)0x52000244)                          
#define rEP3_DMA_FIFO      (*(volatile unsigned char *)0x52000248)                          
#define rEP3_DMA_TTC_L     (*(volatile unsigned char *)0x5200024c)                           
#define rEP3_DMA_TTC_M     (*(volatile unsigned char *)0x52000250)
#define rEP3_DMA_TTC_H     (*(volatile unsigned char *)0x52000254)
#define rEP4_DMA_CON       (*(volatile unsigned char *)0x52000258)                            
#define rEP4_DMA_UNIT      (*(volatile unsigned char *)0x5200025c)                          
#define rEP4_DMA_FIFO      (*(volatile unsigned char *)0x52000260)                          
#define rEP4_DMA_TTC_L     (*(volatile unsigned char *)0x52000264)                           
#define rEP4_DMA_TTC_M     (*(volatile unsigned char *)0x52000268)
#define rEP4_DMA_TTC_H     (*(volatile unsigned char *)0x5200026c)
#endif                  


                  
#define rWTCON   (*(volatile unsigned *)0x53000000)                       
#define rWTDAT   (*(volatile unsigned *)0x53000004)                       
#define rWTCNT   (*(volatile unsigned *)0x53000008)                        


      
#define rIICCON  (*(volatile unsigned *)0x54000000)              
#define rIICSTAT (*(volatile unsigned *)0x54000004)             
#define rIICADD  (*(volatile unsigned *)0x54000008)              
#define rIICDS   (*(volatile unsigned *)0x5400000c)                 


      
#define rIISCON  (*(volatile unsigned *)0x55000000)              
#define rIISMOD  (*(volatile unsigned *)0x55000004)           
#define rIISPSR  (*(volatile unsigned *)0x55000008)                
#define rIISFCON (*(volatile unsigned *)0x5500000c)                   

#ifdef __BIG_ENDIAN
#define IISFIFO  ((volatile unsigned short *)0x55000012)                 

#else                
#define IISFIFO  ((volatile unsigned short *)0x55000010)                 

#endif


           
#define rGPACON    (*(volatile unsigned *)0x56000000)                 
#define rGPADAT    (*(volatile unsigned *)0x56000004)              

#define rGPBCON    (*(volatile unsigned *)0x56000010)                 
#define rGPBDAT    (*(volatile unsigned *)0x56000014)              
#define rGPBUP     (*(volatile unsigned *)0x56000018)                    

#define rGPCCON    (*(volatile unsigned *)0x56000020)                 
#define rGPCDAT    (*(volatile unsigned *)0x56000024)              
#define rGPCUP     (*(volatile unsigned *)0x56000028)                    

#define rGPDCON    (*(volatile unsigned *)0x56000030)                 
#define rGPDDAT    (*(volatile unsigned *)0x56000034)              
#define rGPDUP     (*(volatile unsigned *)0x56000038)                    

#define rGPECON    (*(volatile unsigned *)0x56000040)                 
#define rGPEDAT    (*(volatile unsigned *)0x56000044)              
#define rGPEUP     (*(volatile unsigned *)0x56000048)                    

#define rGPFCON    (*(volatile unsigned *)0x56000050)                 
#define rGPFDAT    (*(volatile unsigned *)0x56000054)              
#define rGPFUP     (*(volatile unsigned *)0x56000058)                    

#define rGPGCON    (*(volatile unsigned *)0x56000060)                 
#define rGPGDAT    (*(volatile unsigned *)0x56000064)              
#define rGPGUP     (*(volatile unsigned *)0x56000068)                    

#define rGPHCON    (*(volatile unsigned *)0x56000070)                 
#define rGPHDAT    (*(volatile unsigned *)0x56000074)              
#define rGPHUP     (*(volatile unsigned *)0x56000078)                    

#define rMISCCR    (*(volatile unsigned *)0x56000080)                        
#define rDCLKCON   (*(volatile unsigned *)0x56000084)                  
#define rEXTINT0   (*(volatile unsigned *)0x56000088)                                        
#define rEXTINT1   (*(volatile unsigned *)0x5600008c)                                        
#define rEXTINT2   (*(volatile unsigned *)0x56000090)                                        
#define rEINTFLT0  (*(volatile unsigned *)0x56000094)           
#define rEINTFLT1  (*(volatile unsigned *)0x56000098)           
#define rEINTFLT2  (*(volatile unsigned *)0x5600009c)                                               
#define rEINTFLT3  (*(volatile unsigned *)0x560000a0)                                               
#define rEINTMASK  (*(volatile unsigned *)0x560000a4)                          
#define rEINTPEND  (*(volatile unsigned *)0x560000a8)                             
#define rGSTATUS0  (*(volatile unsigned *)0x560000ac)                      
#define rGSTATUS1  (*(volatile unsigned *)0x560000b0)                      
#define rGSTATUS2  (*(volatile unsigned *)0x560000b4)             
#define rGSTATUS3  (*(volatile unsigned *)0x560000b8)                                                     
#define rGSTATUS4  (*(volatile unsigned *)0x560000bc)                                                     


      
#ifdef __BIG_ENDIAN
#define rRTCCON    (*(volatile unsigned char *)0x57000043)              
#define rTICNT     (*(volatile unsigned char *)0x57000047)                  
#define rRTCALM    (*(volatile unsigned char *)0x57000053)                    
#define rALMSEC    (*(volatile unsigned char *)0x57000057)               
#define rALMMIN    (*(volatile unsigned char *)0x5700005b)               
#define rALMHOUR   (*(volatile unsigned char *)0x5700005f)             
#define rALMDATE   (*(volatile unsigned char *)0x57000063)                                     
#define rALMMON    (*(volatile unsigned char *)0x57000067)              
#define rALMYEAR   (*(volatile unsigned char *)0x5700006b)             
#define rRTCRST    (*(volatile unsigned char *)0x5700006f)                  
#define rBCDSEC    (*(volatile unsigned char *)0x57000073)             
#define rBCDMIN    (*(volatile unsigned char *)0x57000077)             
#define rBCDHOUR   (*(volatile unsigned char *)0x5700007b)           
#define rBCDDATE   (*(volatile unsigned char *)0x5700007f)                                     
#define rBCDDAY    (*(volatile unsigned char *)0x57000083)                                     
#define rBCDMON    (*(volatile unsigned char *)0x57000087)            
#define rBCDYEAR   (*(volatile unsigned char *)0x5700008b)           

#else                
#define rRTCCON    (*(volatile unsigned char *)0x57000040)              
#define rTICNT     (*(volatile unsigned char *)0x57000044)                  
#define rRTCALM    (*(volatile unsigned char *)0x57000050)                    
#define rALMSEC    (*(volatile unsigned char *)0x57000054)               
#define rALMMIN    (*(volatile unsigned char *)0x57000058)               
#define rALMHOUR   (*(volatile unsigned char *)0x5700005c)             
#define rALMDATE   (*(volatile unsigned char *)0x57000060)                                      
#define rALMMON    (*(volatile unsigned char *)0x57000064)              
#define rALMYEAR   (*(volatile unsigned char *)0x57000068)             
#define rRTCRST    (*(volatile unsigned char *)0x5700006c)                  
#define rBCDSEC    (*(volatile unsigned char *)0x57000070)             
#define rBCDMIN    (*(volatile unsigned char *)0x57000074)             
#define rBCDHOUR   (*(volatile unsigned char *)0x57000078)           
#define rBCDDATE   (*(volatile unsigned char *)0x5700007c)                                      
#define rBCDDAY    (*(volatile unsigned char *)0x57000080)                                      
#define rBCDMON    (*(volatile unsigned char *)0x57000084)            
#define rBCDYEAR   (*(volatile unsigned char *)0x57000088)           
#endif       


      
#define rADCCON    (*(volatile unsigned *)0x58000000)              
#define rADCTSC    (*(volatile unsigned *)0x58000004)                           
#define rADCDLY    (*(volatile unsigned *)0x58000008)                              
#define rADCDAT0   (*(volatile unsigned *)0x5800000c)                        
#define rADCDAT1   (*(volatile unsigned *)0x58000010)                        

      
#define rSPCON0    (*(volatile unsigned *)0x59000000)               
#define rSPSTA0    (*(volatile unsigned *)0x59000004)              
#define rSPPIN0    (*(volatile unsigned *)0x59000008)                   
#define rSPPRE0    (*(volatile unsigned *)0x5900000c)                           
#define rSPTDAT0   (*(volatile unsigned *)0x59000010)               
#define rSPRDAT0   (*(volatile unsigned *)0x59000014)               

#define rSPCON1    (*(volatile unsigned *)0x59000020)               
#define rSPSTA1    (*(volatile unsigned *)0x59000024)              
#define rSPPIN1    (*(volatile unsigned *)0x59000028)                   
#define rSPPRE1    (*(volatile unsigned *)0x5900002c)                           
#define rSPTDAT1   (*(volatile unsigned *)0x59000030)               
#define rSPRDAT1   (*(volatile unsigned *)0x59000034)               


               
#define rSDICON     (*(volatile unsigned *)0x5a000000)              
#define rSDIPRE     (*(volatile unsigned *)0x5a000004)                          
#define rSDICARG    (*(volatile unsigned *)0x5a000008)                       
#define rSDICCON    (*(volatile unsigned *)0x5a00000c)                      
#define rSDICSTA    (*(volatile unsigned *)0x5a000010)                     
#define rSDIRSP0    (*(volatile unsigned *)0x5a000014)                 
#define rSDIRSP1    (*(volatile unsigned *)0x5a000018)                 
#define rSDIRSP2    (*(volatile unsigned *)0x5a00001c)                 
#define rSDIRSP3    (*(volatile unsigned *)0x5a000020)                 
#define rSDIDTIMER  (*(volatile unsigned *)0x5a000024)                      
#define rSDIBSIZE   (*(volatile unsigned *)0x5a000028)                 
#define rSDIDCON    (*(volatile unsigned *)0x5a00002c)                   
#define rSDIDCNT    (*(volatile unsigned *)0x5a000030)                          
#define rSDIDSTA    (*(volatile unsigned *)0x5a000034)                  
#define rSDIFSTA    (*(volatile unsigned *)0x5a000038)                  
#define rSDIIMSK    (*(volatile unsigned *)0x5a000040)                     

#ifdef __BIG_ENDIAN
#define rSDIDAT    (*(volatile unsigned *)0x5a00003f)           
#define SDIDAT     0x5a00003f
#else                  
#define rSDIDAT    (*(volatile unsigned *)0x5a00003c)           
#define SDIDAT     0x5a00003c
#endif                 


      
#define pISR_RESET     (*(unsigned *)(_ISR_STARTADDRESS+0x0))
#define pISR_UNDEF     (*(unsigned *)(_ISR_STARTADDRESS+0x4))
#define pISR_SWI       (*(unsigned *)(_ISR_STARTADDRESS+0x8))
#define pISR_PABORT    (*(unsigned *)(_ISR_STARTADDRESS+0xc))
#define pISR_DABORT    (*(unsigned *)(_ISR_STARTADDRESS+0x10))
#define pISR_RESERVED  (*(unsigned *)(_ISR_STARTADDRESS+0x14))
#define pISR_IRQ       (*(unsigned *)(_ISR_STARTADDRESS+0x18))
#define pISR_FIQ       (*(unsigned *)(_ISR_STARTADDRESS+0x1c))

#define pISR_EINT0     (*(unsigned *)(_ISR_STARTADDRESS+0x20))
#define pISR_EINT1     (*(unsigned *)(_ISR_STARTADDRESS+0x24))
#define pISR_EINT2     (*(unsigned *)(_ISR_STARTADDRESS+0x28))
#define pISR_EINT3     (*(unsigned *)(_ISR_STARTADDRESS+0x2c))
#define pISR_EINT4_7   (*(unsigned *)(_ISR_STARTADDRESS+0x30))
#define pISR_EINT8_23  (*(unsigned *)(_ISR_STARTADDRESS+0x34))
#define pISR_NOTUSED6  (*(unsigned *)(_ISR_STARTADDRESS+0x38))
#define pISR_BAT_FLT   (*(unsigned *)(_ISR_STARTADDRESS+0x3c))
#define pISR_TICK      (*(unsigned *)(_ISR_STARTADDRESS+0x40))
#define pISR_WDT       (*(unsigned *)(_ISR_STARTADDRESS+0x44))
#define pISR_TIMER0    (*(unsigned *)(_ISR_STARTADDRESS+0x48))
#define pISR_TIMER1    (*(unsigned *)(_ISR_STARTADDRESS+0x4c))
#define pISR_TIMER2    (*(unsigned *)(_ISR_STARTADDRESS+0x50))
#define pISR_TIMER3    (*(unsigned *)(_ISR_STARTADDRESS+0x54))
#define pISR_TIMER4    (*(unsigned *)(_ISR_STARTADDRESS+0x58))
#define pISR_UART2     (*(unsigned *)(_ISR_STARTADDRESS+0x5c))
#define pISR_LCD       (*(unsigned *)(_ISR_STARTADDRESS+0x60))
#define pISR_DMA0      (*(unsigned *)(_ISR_STARTADDRESS+0x64))
#define pISR_DMA1      (*(unsigned *)(_ISR_STARTADDRESS+0x68))
#define pISR_DMA2      (*(unsigned *)(_ISR_STARTADDRESS+0x6c))
#define pISR_DMA3      (*(unsigned *)(_ISR_STARTADDRESS+0x70))
#define pISR_SDI       (*(unsigned *)(_ISR_STARTADDRESS+0x74))
#define pISR_SPI0      (*(unsigned *)(_ISR_STARTADDRESS+0x78))
#define pISR_UART1     (*(unsigned *)(_ISR_STARTADDRESS+0x7c))
#define pISR_NOTUSED24 (*(unsigned *)(_ISR_STARTADDRESS+0x80))
#define pISR_USBD      (*(unsigned *)(_ISR_STARTADDRESS+0x84))
#define pISR_USBH      (*(unsigned *)(_ISR_STARTADDRESS+0x88))
#define pISR_IIC       (*(unsigned *)(_ISR_STARTADDRESS+0x8c))
#define pISR_UART0     (*(unsigned *)(_ISR_STARTADDRESS+0x90))
#define pISR_SPI1      (*(unsigned *)(_ISR_STARTADDRESS+0x94))
#define pISR_RTC       (*(unsigned *)(_ISR_STARTADDRESS+0x98))
#define pISR_ADC       (*(unsigned *)(_ISR_STARTADDRESS+0x9c))


              
#define BIT_EINT0      (0x1)
#define BIT_EINT1      (0x1<<1)
#define BIT_EINT2      (0x1<<2)
#define BIT_EINT3      (0x1<<3)
#define BIT_EINT4_7    (0x1<<4)
#define BIT_EINT8_23   (0x1<<5)
#define BIT_NOTUSED6   (0x1<<6)
#define BIT_BAT_FLT    (0x1<<7)
#define BIT_TICK       (0x1<<8)
#define BIT_WDT        (0x1<<9)
#define BIT_TIMER0     (0x1<<10)
#define BIT_TIMER1     (0x1<<11)
#define BIT_TIMER2     (0x1<<12)
#define BIT_TIMER3     (0x1<<13)
#define BIT_TIMER4     (0x1<<14)
#define BIT_UART2      (0x1<<15)
#define BIT_LCD        (0x1<<16)
#define BIT_DMA0       (0x1<<17)
#define BIT_DMA1       (0x1<<18)
#define BIT_DMA2       (0x1<<19)
#define BIT_DMA3       (0x1<<20)
#define BIT_SDI        (0x1<<21)
#define BIT_SPI0       (0x1<<22)
#define BIT_UART1      (0x1<<23)
#define BIT_NOTUSED24  (0x1<<24)
#define BIT_USBD       (0x1<<25)
#define BIT_USBH       (0x1<<26)
#define BIT_IIC        (0x1<<27)
#define BIT_UART0      (0x1<<28)
#define BIT_SPI1       (0x1<<29)
#define BIT_RTC        (0x1<<30)
#define BIT_ADC        (0x1<<31)
#define BIT_ALLMSK     (0xffffffff)

#define BIT_SUB_ALLMSK (0x7ff)
#define BIT_SUB_ADC    (0x1<<10)
#define BIT_SUB_TC     (0x1<<9)
#define BIT_SUB_ERR2   (0x1<<8)
#define BIT_SUB_TXD2   (0x1<<7)
#define BIT_SUB_RXD2   (0x1<<6)
#define BIT_SUB_ERR1   (0x1<<5)
#define BIT_SUB_TXD1   (0x1<<4)
#define BIT_SUB_RXD1   (0x1<<3)
#define BIT_SUB_ERR0   (0x1<<2)
#define BIT_SUB_TXD0   (0x1<<1)
#define BIT_SUB_RXD0   (0x1<<0)

#define ClearPending(bit) {\
	        rSRCPND = bit;\
	        rINTPND = bit;\
	        rINTPND;\
	        }
                                                                        

#endif

#ifdef __cplusplus
}
#endif
#endif                   
