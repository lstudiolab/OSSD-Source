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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

                                                                              


#ifndef _IOKERNELREPORTSTRUCTS_H_
#define _IOKERNELREPORTSTRUCTS_H_

#include <stdint.h>

#include <IOKit/IOReportTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

                                                                 
                                                                 
                                                           
                                                      
#define kIOReportLegendPublicKey        "IOReportLegendPublic"             
#define kIOReportLegendKey              "IOReportLegend"                  
#define kIOReportLegendChannelsKey      "IOReportChannels"                
#define kIOReportLegendGroupNameKey     "IOReportGroupName"               
#define kIOReportLegendSubGroupNameKey  "IOReportSubGroupName"            
#define kIOReportLegendInfoKey          "IOReportChannelInfo"              
#define kIOReportLegendUnitKey          "IOReportChannelUnit"             
#define kIOReportLegendConfigKey        "IOReportChannelConfig"            
#define kIOReportLegendStateNamesKey    "IOReportChannelStateNames"         

                                                                             
#define kIOReportChannelIDIdx           0                  
#define kIOReportChannelTypeIdx         1                  
#define kIOReportChannelNameIdx         2                  

                                   
                                                                          
                           
                                   
                                        
                                                                    
   
#define kIOHistogramScaleLinear 0
#define kIOHistogramScaleExponential 1
typedef struct {
	uint32_t    base_bucket_width;                                         
	uint32_t    scale_flag;                                               
	uint32_t    segment_idx;                                         
	uint32_t    segment_bucket_count;                                    
} __attribute((packed)) IOHistogramSegmentConfig;

                                                              
typedef struct {
	uint64_t    samples;
	uint64_t    mean;
	uint64_t    variance;
	uint64_t    reserved;
} __attribute((packed)) IONormDistReportValues;

#ifdef __cplusplus
}
#endif

#endif                             
