/*
 * Copyright (c) 2000-2009 Apple Inc. All rights reserved.
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
  
                         
  
                                                            
  
   

#ifndef _KERN_HOST_STATISTICS_H_
#define _KERN_HOST_STATISTICS_H_

#include <kern/counter.h>

SCALABLE_COUNTER_DECLARE(vm_statistics_zero_fill_count);                                  
SCALABLE_COUNTER_DECLARE(vm_statistics_reactivations);                                      
SCALABLE_COUNTER_DECLARE(vm_statistics_pageins);                                  
SCALABLE_COUNTER_DECLARE(vm_statistics_pageouts);                                  
SCALABLE_COUNTER_DECLARE(vm_statistics_faults);                                  
SCALABLE_COUNTER_DECLARE(vm_statistics_cow_faults);                                      
SCALABLE_COUNTER_DECLARE(vm_statistics_lookups);                                          
SCALABLE_COUNTER_DECLARE(vm_statistics_hits);                                          
SCALABLE_COUNTER_DECLARE(vm_statistics_purges);                                        
SCALABLE_COUNTER_DECLARE(vm_statistics_decompressions);                                      
SCALABLE_COUNTER_DECLARE(vm_statistics_compressions);                                      
SCALABLE_COUNTER_DECLARE(vm_statistics_swapins);                                                                      
SCALABLE_COUNTER_DECLARE(vm_statistics_swapouts);                                                                      
SCALABLE_COUNTER_DECLARE(vm_statistics_total_uncompressed_pages_in_compressor);                                                            

SCALABLE_COUNTER_DECLARE(vm_page_grab_count);

#endif                                
