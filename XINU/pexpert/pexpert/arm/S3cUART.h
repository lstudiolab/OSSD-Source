/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
 */
#ifndef _PEXPERT_ARM_S3CUART_H
#define _PEXPERT_ARM_S3CUART_H

#define S3CUART


       
#define rULCON0     (*(volatile unsigned *)(uart_base + 0x00))                           
#define rUCON0      (*(volatile unsigned *)(uart_base + 0x04))                      
#define rUFCON0     (*(volatile unsigned *)(uart_base + 0x08))                           
#define rUMCON0     (*(volatile unsigned *)(uart_base + 0x0c))                            
#define rUTRSTAT0   (*(volatile unsigned *)(uart_base + 0x10))                           
#define rUERSTAT0   (*(volatile unsigned *)(uart_base + 0x14))                              
#define rUFSTAT0    (*(volatile unsigned *)(uart_base + 0x18))                          
#define rUMSTAT0    (*(volatile unsigned *)(uart_base + 0x1c))                           
#define rUTXH0      (*(volatile unsigned *)(uart_base + 0x20))                           
#define rURXH0      (*(volatile unsigned *)(uart_base + 0x24))                        
#define rUBRDIV0    (*(volatile unsigned *)(uart_base + 0x28))                                
#define rUDIVSLOT0  (*(volatile unsigned *)(uart_base + 0x2C))                                

#define rULCON1     (*(volatile unsigned *)(uart1_base + 0x00))                           
#define rUCON1      (*(volatile unsigned *)(uart1_base + 0x04))                      
#define rUFCON1     (*(volatile unsigned *)(uart1_base + 0x08))                           
#define rUMCON1     (*(volatile unsigned *)(uart1_base + 0x0c))                            
#define rUTRSTAT1   (*(volatile unsigned *)(uart1_base + 0x10))                           
#define rUERSTAT1   (*(volatile unsigned *)(uart1_base + 0x14))                              
#define rUFSTAT1    (*(volatile unsigned *)(uart1_base + 0x18))                          
#define rUMSTAT1    (*(volatile unsigned *)(uart1_base + 0x1c))                           
#define rUTXH1      (*(volatile unsigned *)(uart1_base + 0x20))                           
#define rURXH1      (*(volatile unsigned *)(uart1_base + 0x24))                        
#define rUBRDIV1    (*(volatile unsigned *)(uart1_base + 0x28))                                

#define rULCON2     (*(volatile unsigned *)(uart2_base + 0x00))                           
#define rUCON2      (*(volatile unsigned *)(uart2_base + 0x04))                      
#define rUFCON2     (*(volatile unsigned *)(uart2_base + 0x08))                           
#define rUMCON2     (*(volatile unsigned *)(uart2_base + 0x0c))                            
#define rUTRSTAT2   (*(volatile unsigned *)(uart2_base + 0x10))                           
#define rUERSTAT2   (*(volatile unsigned *)(uart2_base + 0x14))                              
#define rUFSTAT2    (*(volatile unsigned *)(uart2_base + 0x18))                          
#define rUMSTAT2    (*(volatile unsigned *)(uart2_base + 0x1c))                           
#define rUTXH2      (*(volatile unsigned *)(uart2_base + 0x20))                           
#define rURXH2      (*(volatile unsigned *)(uart2_base + 0x24))                        
#define rUBRDIV2    (*(volatile unsigned *)(uart2_base + 0x28))                                

#if 0
#define WrUTXH0(ch) (*(volatile unsigned char *)0x50000020)=(unsigned char)(ch)
#define RdURXH0()   (*(volatile unsigned char *)0x50000024)
#define WrUTXH1(ch) (*(volatile unsigned char *)0x50004020)=(unsigned char)(ch)
#define RdURXH1()   (*(volatile unsigned char *)0x50004024)
#define WrUTXH2(ch) (*(volatile unsigned char *)0x50008020)=(unsigned char)(ch)
#define RdURXH2()   (*(volatile unsigned char *)0x50008024)

#define UTXH0       (0x50000020)                                
#define URXH0       (0x50000024)
#define UTXH1       (0x50004020)
#define URXH1       (0x50004024)
#define UTXH2       (0x50008020)
#define URXH2       (0x50008024)
#endif

#endif                             
