/*
 * Copyright (c) 2012-2020 Apple Computer, Inc.  All Rights Reserved.
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

#ifndef _IOREPORT_MACROS_H_
#define _IOREPORT_MACROS_H_

#include "IOReportTypes.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IOREPORT_ABORT
#define IOREPORT_ABORT panic
#endif

  
               
  
                                                                    
                                                                  
                                         
  
                                                                 
                                                                   
                                                                
                                                                  
                                                             
                       
   


                                                          

  
                                               
   

#define SIMPLEREPORT_BUFSIZE   (sizeof(IOReportElement))


  
                                              
  
                                                                    
                                                                
                                                                             
                                                          
                                                             
  
                                                                             
                                                   
   

#define SIMPLEREPORT_INIT(buf, bufSize, providerID, channelID, cats)  \
do {  \
    memset((buf), '&', (bufSize));  \
    IOReportElement     *__elem = (IOReportElement *)(buf);  \
    IOSimpleReportValues *__vals;  \
    if ((bufSize) >= SIMPLEREPORT_BUFSIZE) {  \
	__elem->provider_id = (providerID);  \
	__elem->channel_id = (channelID);  \
	__elem->channel_type.report_format = kIOReportFormatSimple;  \
	__elem->channel_type.reserved = 0;  \
	__elem->channel_type.categories = (cats);  \
	__elem->channel_type.nelements = 1;  \
	__elem->channel_type.element_idx = 0;  \
	__elem->timestamp = 0;  \
	__vals = (IOSimpleReportValues*)&__elem->values;  \
	__vals->simple_value = kIOReportInvalidIntValue;  \
    }  \
    else {  \
	IOREPORT_ABORT("bufSize is smaller than the required size\n");  \
    }  \
} while(0)


  
                                     
  
                                                                       
                                               
   

#define SIMPLEREPORT_SETVALUE(simp_buf, new_value)  \
do {  \
    IOReportElement *__elem = (IOReportElement *)(simp_buf);  \
    IOSimpleReportValues *__vals;  \
    __vals = (IOSimpleReportValues*)&__elem->values;  \
    __vals->simple_value = (new_value);  \
} while(0)


  
                                         
  
                                                                       
                                                             
   
#define SIMPLEREPORT_INCREMENTVALUE(simp_buf, new_value)  \
do {  \
    IOReportElement *__elem = (IOReportElement *)(simp_buf);  \
    IOSimpleReportValues *__vals;  \
    __vals = (IOSimpleReportValues*)&__elem->values;  \
    __vals->simple_value += (new_value);  \
} while(0)


  
                             
                                                       
  
                                                                     
                                                                               
                                                         
                                                                           
                                                                               
   

#define SIMPLEREPORT_UPDATEPREP(simp_buf, ptr2cpy, size2cpy)  \
do {  \
    (ptr2cpy) = (simp_buf);  \
    (size2cpy) = sizeof(IOReportElement);  \
} while(0)


  
                                                      
                                                             
  
                                                                           
                                                                           
   

#define SIMPLEREPORT_UPDATERES(action, result)  \
do {  \
    if (((action) == kIOReportGetDimensions) || ((action) == kIOReportCopyChannelData)) {  \
	int *__nElements = (int *)(result);  \
	*__nElements += 1;  \
    }  \
} while (0)


  
                                               
  
                                                                    
   

#define SIMPLEREPORT_GETCHID(simp_buf)  \
    (((IOReportElement *)(simp_buf))->channel_id)

  
                                                 
  
                                                                    
   

#define SIMPLEREPORT_GETCHTYPE(simp_buf)  \
    (*(uint64_t*)&(((IOReportElement *)(simp_buf))->channel_type))


  
                                           
  
                                                             
   

#define SIMPLEREPORT_GETVALUE(simp_buf)  \
    (((IOSimpleReportValues*)&(((IOReportElement*)(simp_buf))->values))  \
	    ->simple_value)


                                                       

                                  
typedef struct {
	uint16_t        curr_state;
	uint64_t        update_ts;
	IOReportElement elem[];                     
} IOStateReportInfo;

  
                                                        
  
                                                
   
#define STATEREPORT_BUFSIZE(nstates)  \
    (sizeof(IOStateReportInfo) + (nstates) * sizeof(IOReportElement))


  
                                   
  
                                                                  
                                                                            
                                                                
                                                                             
                                                                            
                                                             
  
                                                                             
                                                   
   
#define STATEREPORT_INIT(nstates, buf, bufSize, providerID, channelID, cats) \
do {  \
    memset((buf), '&', (bufSize));  \
    IOStateReportInfo *__info = (IOStateReportInfo *)(buf);  \
    IOStateReportValues *__rep;  \
    IOReportElement     *__elem;  \
    if ((bufSize) >= STATEREPORT_BUFSIZE(nstates)) {  \
	for (uint16_t __no = 0; __no < (nstates); __no++) {  \
	    __elem =  &(__info->elem[__no]);  \
	    __rep = (IOStateReportValues *) &(__elem->values);  \
	    __elem->provider_id = (providerID);  \
	    __elem->channel_id = (channelID);  \
	    __elem->channel_type.report_format = kIOReportFormatState;  \
	    __elem->channel_type.reserved = 0;  \
	    __elem->channel_type.categories = (cats);  \
	    __elem->channel_type.nelements = (nstates);  \
	    __elem->channel_type.element_idx = __no;  \
	    __elem->timestamp = 0;  \
	    __rep->state_id = __no;  \
	    __rep->intransitions = 0;  \
	    __rep->upticks = 0;  \
	    __rep->last_intransition = 0;  \
	}  \
	__info->curr_state = 0;  \
	__info->update_ts = 0;  \
    }  \
    else {  \
	IOREPORT_ABORT("bufSize is smaller than the required size\n");  \
    }  \
} while(0)

  
                                                                         
                                                                          
                                                                              
  
                                                                      
                                                                 
                                                          
   
#define STATEREPORT_SETSTATEID(state_buf, stateIdx, stateID)  \
do {  \
    IOStateReportInfo *__info = (IOStateReportInfo *)(state_buf);  \
    IOStateReportValues *__rep;  \
    if ((stateIdx) < __info->elem[0].channel_type.nelements) {  \
	__rep = (IOStateReportValues*) &(__info->elem[(stateIdx)].values);  \
	__rep->state_id = (stateID);  \
    }  \
} while (0)


  
                                  
  
                                                                             
                                                                    
                                                               
   
#define STATEREPORT_SETSTATE(state_buf, newStateIdx, changeTime)  \
do {  \
    IOStateReportInfo *__info = (IOStateReportInfo *)(state_buf);  \
    IOStateReportValues *__rep;  \
    if ((newStateIdx) < __info->elem[0].channel_type.nelements ) {  \
	__rep = (IOStateReportValues*) &(__info->elem[__info->curr_state].values);  \
	if (__info->update_ts)  \
	    __rep->upticks += (changeTime) - __info->update_ts;  \
	__info->elem[(newStateIdx)].timestamp = (changeTime);  \
	__rep = (IOStateReportValues*) &(__info->elem[(newStateIdx)].values);  \
	__rep->intransitions++;  \
	__info->curr_state = (newStateIdx);  \
	__info->update_ts = (changeTime);  \
    }  \
} while(0)

  
                            
                                                       
  
                                                                         
                                           
                                                                           
                                                                           
   
#define STATEREPORT_UPDATEPREP(state_buf, currentTime, ptr2cpy, size2cpy)  \
do {  \
    IOStateReportInfo *__info = (IOStateReportInfo *)(state_buf);  \
    IOReportElement     *__elem;  \
    IOStateReportValues *__state;  \
    (size2cpy) = __info->elem[0].channel_type.nelements * sizeof(IOReportElement);  \
    (ptr2cpy) =  (void *) &__info->elem[0];  \
    if (__info->update_ts)  {  \
	__elem = &__info->elem[__info->curr_state];  \
	__state = (IOStateReportValues *)&__elem->values;  \
	__elem->timestamp = (currentTime);  \
	__state->upticks  += (currentTime) - __info->update_ts;  \
	__info->update_ts = (currentTime);  \
    }  \
} while(0)

  
                                                                               
                                    
  
                                                                            
                                                                     
                                                                     
   

#define STATEREPORT_UPDATERES(state_buf, action, result)  \
do {  \
    IOStateReportInfo *__info = (IOStateReportInfo *)(state_buf);  \
    IOReportElement     *__elem;  \
    int *__nElements = (int *)(result);  \
    if (((action) == kIOReportGetDimensions) || ((action) == kIOReportCopyChannelData)) {  \
	__elem =  &(__info->elem[0]);  \
	*__nElements += __elem->channel_type.nelements;  \
    }  \
} while (0)


  
                                              
  
                                                                    
   
#define STATEREPORT_GETCHID(state_buf)  \
    (((IOStateReportInfo *)(state_buf))->elem[0].channel_id)

  
                                                
  
                                                                    
   
#define STATEREPORT_GETCHTYPE(state_buf)  \
    (*(uint64_t*)&(((IOStateReportInfo *)(state_buf))->elem[0].channel_type))

  
                                                    
  
                                                                      
                                                                             
  
   
#define STATEREPORT_GETTRANSITIONS(state_buf, stateIdx)  \
    (((stateIdx) < ((IOStateReportInfo *)(state_buf))->elem[0].channel_type.nelements)  \
	? ((IOStateReportValues*)&(((IOStateReportInfo*)(state_buf))->elem[(stateIdx)].values))->intransitions  \
	: kIOReportInvalidValue)

  
                                                        
  
                                                                      
                                                                             
   
#define STATEREPORT_GETTICKS(state_buf, stateIdx)  \
    (((stateIdx) < ((IOStateReportInfo*)(state_buf))->elem[0].channel_type.nelements)  \
	? ((IOStateReportValues*)&(((IOStateReportInfo*)(state_buf))->elem[(stateIdx)].values))->upticks  \
	: kIOReportInvalidValue)


                                                                    

  
                                                     
  
                                                
   

#define SIMPLEARRAY_BUFSIZE(nValues) \
    ((((nValues)/IOR_VALUES_PER_ELEMENT) + (((nValues) % IOR_VALUES_PER_ELEMENT) ? 1:0)) \
	* sizeof(IOReportElement))

  
                                                      
  
                                                                      
                                                                              
                                                                  
                                                                               
                                                                              
                                                               
  
                                                                               
                                                   
   

#define SIMPLEARRAY_INIT(nValues, buf, bufSize, providerID, channelID, cats) \
do {  \
    memset((buf), '&', (bufSize));  \
    IOSimpleArrayReportValues *__rep;  \
    IOReportElement     *__elem;  \
    uint32_t            __nElems = (((nValues) / IOR_VALUES_PER_ELEMENT) + \
	                            (((nValues) % IOR_VALUES_PER_ELEMENT) ? 1 : 0)); \
    if ((bufSize) >= SIMPLEARRAY_BUFSIZE(nValues)) {  \
	for (unsigned __no = 0; __no < __nElems; __no++) {  \
	    __elem =  &(((IOReportElement *)(buf))[__no]);  \
	    __rep = (IOSimpleArrayReportValues *) &(__elem->values);  \
	    __elem->provider_id = (providerID);  \
	    __elem->channel_id = (channelID);  \
	    __elem->channel_type.report_format = kIOReportFormatSimpleArray;  \
	    __elem->channel_type.reserved = 0;  \
	    __elem->channel_type.categories = (cats);  \
	    __elem->channel_type.nelements = (__nElems);  \
	    __elem->channel_type.element_idx = __no;  \
	    __elem->timestamp = 0;  \
	    __rep->simple_values[0] = kIOReportInvalidIntValue;  \
	    __rep->simple_values[1] = kIOReportInvalidIntValue;  \
	    __rep->simple_values[2] = kIOReportInvalidIntValue;  \
	    __rep->simple_values[3] = kIOReportInvalidIntValue;  \
	}  \
    }  \
    else {  \
	IOREPORT_ABORT("bufSize is smaller than the required size\n");  \
    }  \
} while(0)


                               

    #define __SA_FINDREP(array_buf, idx)  \
	IOSimpleArrayReportValues *__rep;  \
	IOReportElement     *__elem;  \
	unsigned __elemIdx = (idx) / IOR_VALUES_PER_ELEMENT;  \
	unsigned __valueIdx = (idx) % IOR_VALUES_PER_ELEMENT;  \
	__elem = &(((IOReportElement *)(array_buf))[0]);  \
	if (__elemIdx < __elem->channel_type.nelements)  { \
	    __elem = &(((IOReportElement *)(array_buf))[__elemIdx]);  \
	    __rep = (IOSimpleArrayReportValues *) &(__elem->values);  \

    #define __SA_MAXINDEX(array_buf)  \
	((((IOReportElement*)(array_buf))->channel_type.nelements)  \
	    * IOR_VALUES_PER_ELEMENT) - 1

  
                                                           
  
                                                                         
                                                             
                                                              
   
#define SIMPLEARRAY_SETVALUE(array_buf, idx, newValue) \
do {  \
    __SA_FINDREP((array_buf), (idx)) \
	__rep->simple_values[__valueIdx] = (newValue);  \
    } \
} while(0)

  
                                                       
  
                                                                        
                                                                         
                                                                
   
#define SIMPLEARRAY_INCREMENTVALUE(array_buf, idx, value)  \
do {  \
    __SA_FINDREP((array_buf), (idx)) \
	__rep->simple_values[__valueIdx] += (value);  \
    } \
} while(0)


  
                                  
                                                       
  
                                                                         
                                                                           
                                                                           
   

#define SIMPLEARRAY_UPDATEPREP(array_buf, ptr2cpy, size2cpy) \
do {  \
    IOReportElement     *__elem;  \
    __elem = &(((IOReportElement *)(array_buf))[0]);  \
    (ptr2cpy) =  (void *) (array_buf);  \
    (size2cpy) = __elem->channel_type.nelements * sizeof(IOReportElement);  \
} while(0)


  
                                                                               
                                    
  
                                                                            
                                                                     
                                                                     
   

#define SIMPLEARRAY_UPDATERES(array_buf, action, result) \
do {  \
    IOReportElement     *__elem;  \
    int *__nElements = (int *)(result);  \
    __elem = &(((IOReportElement *)(array_buf))[0]);  \
    if (((action) == kIOReportGetDimensions) || ((action) == kIOReportCopyChannelData)) {  \
	*__nElements += __elem->channel_type.nelements;  \
    }  \
} while (0)


  
                                                    
  
                                                                    
   
#define SIMPLEARRAY_GETCHID(array_buf)  \
    (((IOReportElement *)(array_buf))->channel_id)


  
                                                      
  
                                                                    
   
#define SIMPLEARRAY_GETCHTYPE(array_buf)  \
    (*(uint64_t*)&(((IOReportElement *)(array_buf))->channel_type))

  
                                        
  
                                                                    
                                                                               
   
#define SIMPLEARRAY_GETVALUE(array_buf, idx)  \
    (((idx) > __SA_MAXINDEX(array_buf) || (idx) < 0) ? kIOReportInvalidIntValue :  \
    ((IOSimpleArrayReportValues*)&(  \
	((IOReportElement*)(array_buf))[(idx) / IOR_VALUES_PER_ELEMENT].values))  \
	    ->simple_values[(idx) % IOR_VALUES_PER_ELEMENT])


                                                       

                                      
typedef struct {
	int             bucketWidth;
	IOReportElement elem[];                     
} IOHistReportInfo;

  
                                                            
  
                                                    
   
#define HISTREPORT_BUFSIZE(nbuckets)  \
    (sizeof(IOHistReportInfo) + ((nbuckets) * sizeof(IOReportElement)))

  
                                                                             
  
                                                                          
                                                      
                                                                            
                                                                
                                                                             
                                                                            
                                                             
  
                                                                             
                                                   
   
#define HISTREPORT_INIT(nbuckets, bktSize, buf, bufSize, providerID, channelID, cats) \
do {  \
    memset((buf), '&', (bufSize));  \
    IOHistReportInfo   *__info = (IOHistReportInfo *)(buf);  \
    IOReportElement         *__elem;  \
    IOHistogramReportValues *__rep;  \
    if ((bufSize) >= HISTREPORT_BUFSIZE(nbuckets)) {  \
	__info->bucketWidth = (bktSize);  \
	for (uint16_t __no = 0; __no < (nbuckets); __no++) {  \
	    __elem =  &(__info->elem[__no]);  \
	    __rep = (IOHistogramReportValues *) &(__elem->values);  \
	    __elem->provider_id = (providerID);  \
	    __elem->channel_id = (channelID);  \
	    __elem->channel_type.report_format = kIOReportFormatHistogram;  \
	    __elem->channel_type.reserved = 0;  \
	    __elem->channel_type.categories = (cats);  \
	    __elem->channel_type.nelements = (nbuckets);  \
	    __elem->channel_type.element_idx = __no;  \
	    __elem->timestamp = 0;  \
	    memset(__rep, '\0', sizeof(IOHistogramReportValues)); \
	}  \
    }  \
    else {  \
	IOREPORT_ABORT("bufSize is smaller than the required size\n");  \
    }  \
} while (0)

  
                                     
  
  
                                                                           
                                                           
   
#define HISTREPORT_TALLYVALUE(hist_buf, value) \
do {  \
    IOHistReportInfo   *__info = (IOHistReportInfo *)(hist_buf);  \
    IOReportElement         *__elem;  \
    IOHistogramReportValues *__rep;  \
    for (unsigned __no = 0; __no < __info->elem[0].channel_type.nelements; __no++) {  \
	if ((value) <= __info->bucketWidth * (__no+1)) {  \
	    __elem =  &(__info->elem[__no]);  \
	    __rep = (IOHistogramReportValues *) &(__elem->values);  \
	    if (__rep->bucket_hits == 0) {  \
	        __rep->bucket_min = __rep->bucket_max = (value);  \
	    }  \
	    else if ((value) < __rep->bucket_min) {  \
	        __rep->bucket_min = (value);  \
	    }  \
	    else if ((value) > __rep->bucket_max) {  \
	        __rep->bucket_max = (value);  \
	    }  \
	    __rep->bucket_sum += (value);  \
	    __rep->bucket_hits++;  \
	    break;  \
	}  \
    }  \
} while (0)

  
                                
                                                       
  
                                                                        
                                                                           
                                                                           
   

#define HISTREPORT_UPDATEPREP(hist_buf, ptr2cpy, size2cpy) \
do {  \
    IOHistReportInfo   *__info = (IOHistReportInfo *)(hist_buf);  \
    (size2cpy) = __info->elem[0].channel_type.nelements * sizeof(IOReportElement);  \
    (ptr2cpy) =  (void *) &__info->elem[0];  \
} while(0)


  
                                                                               
                                    
  
                                                                           
                                                                     
                                                                     
   

#define HISTREPORT_UPDATERES(hist_buf, action, result) \
do {  \
    IOHistReportInfo   *__info = (IOHistReportInfo *)(hist_buf);  \
    int *__nElements = (int *)(result);  \
    if (((action) == kIOReportGetDimensions) || ((action) == kIOReportCopyChannelData)) {  \
	*__nElements += __info->elem[0].channel_type.nelements;  \
    }  \
} while (0)

  
                                                  
  
                                                                  
   
#define HISTREPORT_GETCHID(hist_buf)  \
    (((IOHistReportInfo *)(hist_buf))->elem[0].channel_id)

  
                                                    
  
                                                                  
   
#define HISTREPORT_GETCHTYPE(hist_buf)  \
    (*(uint64_t*)&(((IOHistReportInfo *)(hist_buf))->elem[0].channel_type))

#ifdef __cplusplus
}
#endif

#endif                       
