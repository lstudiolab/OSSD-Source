/*
 * Copyright (c) 2012-2014 Apple Computer, Inc.  All Rights Reserved.
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

#ifndef _IOREPORT_TYPES_H_
#define _IOREPORT_TYPES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IOR_VALUES_PER_ELEMENT  4

                                     
                                         
                                                            
  
                
                                                                    
                                                                       
                                                                
   
#define kIOReportInvalidIntValue INT64_MIN
#define kIOReportInvalidValue (uint64_t)kIOReportInvalidIntValue

                                  
                                                                         
  
                
                                                                    
                                                                       
                                                                        
                                                                       
                                                                      
                                                                
   
typedef uint16_t IOReportCategories;
#define kIOReportCategoryPower           (1 << 1)                    
#define kIOReportCategoryTraffic         (1 << 2)                          
#define kIOReportCategoryPerformance     (1 << 3)                          
#define kIOReportCategoryPeripheral      (1 << 4)                      

#define kIOReportCategoryField           (1 << 8)                          

                        
#define kIOReportCategoryDebug           (1 << 15)
#define kIOReportInvalidCategory         UINT16_MAX


                                    
typedef uint8_t IOReportFormat;
enum {
	kIOReportInvalidFormat = 0,
	kIOReportFormatSimple = 1,
	kIOReportFormatState = 2,
	kIOReportFormatHistogram = 3,
	kIOReportFormatSimpleArray = 4
};

                       
typedef struct {
	int64_t    simple_value;
	uint64_t    reserved1;
	uint64_t    reserved2;
	uint64_t    reserved3;
} __attribute((packed)) IOSimpleReportValues;

                     
typedef struct {
	int64_t    simple_values[IOR_VALUES_PER_ELEMENT];
} __attribute((packed)) IOSimpleArrayReportValues;

                      
typedef struct {
	uint64_t    state_id;                                              
	uint64_t    intransitions;                                          
	uint64_t    upticks;                                                
	uint64_t    last_intransition;                              
} __attribute((packed)) IOStateReportValues;

                          
typedef struct {
	uint64_t    bucket_hits;
	int64_t     bucket_min;
	int64_t     bucket_max;
	int64_t     bucket_sum;
} __attribute((packed)) IOHistogramReportValues;



                                                         
typedef uint32_t IOReportConfigureAction;
enum {
	                                       
	kIOReportEnable             = 0x01,
	kIOReportGetDimensions      = 0x02,
	kIOReportDisable            = 0x00,

	                           
	kIOReportNotifyHubOnChange  = 0x10,                     

	kIOReportTraceOnChange      = 0x20                     
};

                                                         
typedef uint32_t IOReportUpdateAction;
enum {
	kIOReportCopyChannelData    = 1,
	kIOReportTraceChannelData   = 2
};

typedef struct {
	uint8_t     report_format;                                    
	uint8_t     reserved;                      
	uint16_t    categories;                                          
	uint16_t    nelements;                                 

	                                       
	int16_t     element_idx;                     
	                                                                    
} __attribute((packed)) IOReportChannelType;

   
                                  
                                                                              
                                                                            
                                                                
                                  
                                                                           
                                                                        
                                                                       
                                                                         
                                                                       
                                                                       
                                                    
                                                                       
                                                                       
   
#define __IOR_lshiftchr(c, chshift)     ((uint64_t)(c) << (8*(chshift)))
#define IOREPORT_MAKEID(A, B, C, D, E, F, G, H) \
    (__IOR_lshiftchr(A, 7) | __IOR_lshiftchr(B, 6) | __IOR_lshiftchr(C, 5) \
    | __IOR_lshiftchr(D, 4) | __IOR_lshiftchr(E, 3) | __IOR_lshiftchr(F, 2) \
    | __IOR_lshiftchr(G, 1) | __IOR_lshiftchr(H, 0))

typedef struct {
	uint64_t                channel_id;
	IOReportChannelType     channel_type;
} IOReportChannel;

typedef struct {
	uint32_t                nchannels;
	IOReportChannel         channels[];
} IOReportChannelList;

typedef struct {
	uint64_t                provider_id;
	IOReportChannel         channel;
} IOReportInterest;

typedef struct {
	uint32_t                ninterests;
	IOReportInterest        interests[];
} IOReportInterestList;

typedef struct {
	uint64_t                v[IOR_VALUES_PER_ELEMENT];
} __attribute((packed)) IOReportElementValues;

typedef struct {
	uint64_t                provider_id;
	uint64_t                channel_id;
	IOReportChannelType     channel_type;
	uint64_t                timestamp;                       
	IOReportElementValues   values;
} __attribute((packed)) IOReportElement;



  
                                       
   

               

                                                               
                          

typedef uint64_t IOReportUnit;
typedef uint64_t IOReportUnits;                                      
#define __IOR_MAKEUNIT(quantity, scale) \
	(((IOReportUnit)quantity << 56) | (uint64_t)scale)
#define IOREPORT_GETUNIT_QUANTITY(unit) \
	((IOReportQuantity)((uint64_t)unit >> 56) & 0xff)
#define IOREPORT_GETUNIT_SCALE(unit) \
	((IOReportScaleFactor)unit & 0x00ffffffffffffff)

                                                                              
typedef uint8_t IOReportQuantity;                                          
typedef uint64_t IOReportScaleFactor;

                                                             
                                   
enum {
	                             
	kIOReportQuantityUndefined = 0,

	kIOReportQuantityTime           = 1,          
	kIOReportQuantityPower          = 2,        
	kIOReportQuantityEnergy         = 3,         
	kIOReportQuantityCurrent        = 4,          
	kIOReportQuantityVoltage        = 5,        
	kIOReportQuantityCapacitance    = 6,        
	kIOReportQuantityInductance     = 7,        
	kIOReportQuantityFrequency      = 8,        
	kIOReportQuantityData           = 9,                         
	kIOReportQuantityTemperature    = 10,                         

	kIOReportQuantityEventCount     = 100,
	kIOReportQuantityPacketCount    = 101,
	kIOReportQuantityCPUInstrs      = 102
};


                                                                            
                                                                     
                                                                      
                                                           
   
#define kIOReportScaleConstMask 0x000000007fffffff                            
#define kIOReportScaleOneOver   (1LL << 31)                          
#define kIOReportExpBase        (-127)                                      
#define kIOReportExpZeroOffset  -(kIOReportExpBase)                          
#define kIOReportScaleSIShift   32                               
#define kIOReportScaleSIMask    0x000000ff00000000
#define kIOReportScaleIECShift  40                              
#define kIOReportScaleIECMask   0x0000ff0000000000
#define kIOReportCardinalShift  48                                     
#define kIOReportCardinalMask   0x00ff000000000000


  
                                                 
                                 
  
                                                                 
                                   
                                              
   

               
#define kIOReportScalePico  ((-12LL + kIOReportExpZeroOffset)  \
	                                << kIOReportScaleSIShift)
#define kIOReportScaleNano  ((-9LL + kIOReportExpZeroOffset)  \
	                                << kIOReportScaleSIShift)
#define kIOReportScaleMicro ((-6LL + kIOReportExpZeroOffset)  \
	                                << kIOReportScaleSIShift)
#define kIOReportScaleMilli ((-3LL + kIOReportExpZeroOffset)  \
	                                << kIOReportScaleSIShift)
#define kIOReportScaleUnity 0                     
                                                              
#define kIOReportScaleKilo  ((3LL + kIOReportExpZeroOffset)  \
	                                << kIOReportScaleSIShift)
#define kIOReportScaleMega  ((6LL + kIOReportExpZeroOffset)  \
	                                << kIOReportScaleSIShift)
#define kIOReportScaleGiga  ((9LL + kIOReportExpZeroOffset)  \
	                                << kIOReportScaleSIShift)
#define kIOReportScaleTera  ((12LL + kIOReportExpZeroOffset)  \
	                                << kIOReportScaleSIShift)

                          
                                                                      
#define kIOReportScaleBits  kIOReportScaleUnity
#define kIOReportScaleBytes     ((3LL + kIOReportExpZeroOffset)  \
	                                    << kIOReportScaleIECShift)
                                                                       
#define kIOReportScaleKibi      ((10LL + kIOReportExpZeroOffset)  \
	                                    << kIOReportScaleIECShift)
#define kIOReportScaleKiBytes   ((13LL + kIOReportExpZeroOffset)  \
	                                    << kIOReportScaleIECShift)
#define kIOReportScaleMebi      ((20LL + kIOReportExpZeroOffset)  \
	                                    << kIOReportScaleIECShift)
#define kIOReportScaleMiBytes   ((23LL + kIOReportExpZeroOffset)  \
	                                    << kIOReportScaleIECShift)
#define kIOReportScaleGibi      ((30LL + kIOReportExpZeroOffset)  \
	                                    << kIOReportScaleIECShift)
#define kIOReportScaleGiBytes   ((33LL + kIOReportExpZeroOffset)  \
	                                    << kIOReportScaleIECShift)
#define kIOReportScaleTebi      ((40LL + kIOReportExpZeroOffset)  \
	                                    << kIOReportScaleIECShift)
#define kIOReportScaleTiBytes   ((43LL + kIOReportExpZeroOffset)  \
	                                    << kIOReportScaleIECShift)
                                                                       
                                                                          


                                                  
                                  
#define kIOReportScaleMachHWTicks   (1LL << kIOReportCardinalShift)
#define kIOReportScaleHWPageSize    (2LL << kIOReportCardinalShift)

                                                
#define kIOReportScale4KiB      (4 | kIOReportScaleKiBytes)
#define kIOReportScale8KiB      (8 | kIOReportScaleKiBytes)
#define kIOReportScale16KiB     (16 | kIOReportScaleKiBytes)

                                              
                                                  
                                                              
#define kIOReportScale1GHz      (1 | kIOReportScaleNano)
                                                                              
                                                               
#define kIOReportScale24MHz     (kIOReportScaleOneOver|24 |kIOReportScaleMicro)

                           


                    
#define kIOReportUnitNone       __IOR_MAKEUNIT(kIOReportQuantityUndefined,  \
	                                          kIOReportScaleUnity)

#define kIOReportUnit_s         __IOR_MAKEUNIT(kIOReportQuantityTime,  \
	                                       kIOReportScaleUnity)
#define kIOReportUnit_ms        __IOR_MAKEUNIT(kIOReportQuantityTime,  \
	                                       kIOReportScaleMilli)
#define kIOReportUnit_us        __IOR_MAKEUNIT(kIOReportQuantityTime,  \
	                                       kIOReportScaleMicro)
#define kIOReportUnit_ns        __IOR_MAKEUNIT(kIOReportQuantityTime,  \
	                                       kIOReportScaleNano)

#define kIOReportUnit_J         __IOR_MAKEUNIT(kIOReportQuantityEnergy,  \
	                                       kIOReportScaleUnity)
#define kIOReportUnit_mJ        __IOR_MAKEUNIT(kIOReportQuantityEnergy,  \
	                                       kIOReportScaleMilli)
#define kIOReportUnit_uJ        __IOR_MAKEUNIT(kIOReportQuantityEnergy,  \
	                                       kIOReportScaleMicro)
#define kIOReportUnit_nJ        __IOR_MAKEUNIT(kIOReportQuantityEnergy,  \
	                                       kIOReportScaleNano)
#define kIOReportUnit_pJ        __IOR_MAKEUNIT(kIOReportQuantityEnergy,  \
	                                       kIOReportScalePico)

#define kIOReportUnitHWTicks    __IOR_MAKEUNIT(kIOReportQuantityTime,  \
	                                       kIOReportScaleMachHWTicks)
#define kIOReportUnit24MHzTicks __IOR_MAKEUNIT(kIOReportQuantityTime,  \
	                                       kIOReportScale24MHz)
#define kIOReportUnit1GHzTicks  __IOR_MAKEUNIT(kIOReportQuantityTime,  \
	                                       kIOReportScale1GHz)

#define kIOReportUnitBits       __IOR_MAKEUNIT(kIOReportQuantityData,  \
	                                        kIOReportScaleBits)
#define kIOReportUnitBytes      __IOR_MAKEUNIT(kIOReportQuantityData,  \
	                                        kIOReportScaleBytes)
#define kIOReportUnit_KiB       __IOR_MAKEUNIT(kIOReportQuantityData,  \
	                                       kIOReportScaleKiBytes)
#define kIOReportUnit_MiB       __IOR_MAKEUNIT(kIOReportQuantityData,  \
	                                       kIOReportScaleMiBytes)
#define kIOReportUnit_GiB       __IOR_MAKEUNIT(kIOReportQuantityData,  \
	                                       kIOReportScaleGiBytes)
#define kIOReportUnit_TiB       __IOR_MAKEUNIT(kIOReportQuantityData,  \
	                                       kIOReportScaleTiBytes)

#define kIOReportUnitEvents     __IOR_MAKEUNIT(kIOReportQuantityEventCount,  \
	                                       kIOReportScaleUnity)

#define kIOReportUnitPackets    __IOR_MAKEUNIT(kIOReportQuantityPacketCount,  \
	                                       kIOReportScaleUnity)

#define kIOReportUnitInstrs     __IOR_MAKEUNIT(kIOReportQuantityCPUInstrs,  \
	                                       kIOReportScaleUnity)
#define kIOReportUnit_KI        __IOR_MAKEUNIT(kIOReportQuantityCPUInstrs,  \
	                                       kIOReportScaleKilo)
#define kIOReportUnit_MI        __IOR_MAKEUNIT(kIOReportQuantityCPUInstrs,  \
	                                       kIOReportScaleMega)
#define kIOReportUnit_GI        __IOR_MAKEUNIT(kIOReportQuantityCPUInstrs,  \
	                                       kIOReportScaleGiga)

                                                             

                          


#ifdef __cplusplus
}
#endif

#endif                      
