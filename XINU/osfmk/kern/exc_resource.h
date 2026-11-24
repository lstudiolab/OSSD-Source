/*
 * Copyright (c) 2011-2012 Apple Computer, Inc. All rights reserved.
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
/*
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

  
                                                     
   

#ifndef _EXC_RESOURCE_H_
#define _EXC_RESOURCE_H_

  
                                 
  
        
                                                               
                                                               
                                                               
   


                                                    
#define EXC_RESOURCE_DECODE_RESOURCE_TYPE(code) \
	(((code) >> 61) & 0x7ULL)
#define EXC_RESOURCE_DECODE_FLAVOR(code) \
	(((code) >> 58) & 0x7ULL)

                        
#define RESOURCE_TYPE_CPU       1
#define RESOURCE_TYPE_WAKEUPS   2
#define RESOURCE_TYPE_MEMORY    3
#define RESOURCE_TYPE_IO        4
#define RESOURCE_TYPE_THREADS   5

                               
#define FLAVOR_CPU_MONITOR              1
#define FLAVOR_CPU_MONITOR_FATAL        2

  
                                              
  
                                                        
                                             
  
        
                                                    
                                                    
                                                    
                                                    
                                                    
                                                    
  
           
                                                    
                                                    
                                                    
                                                    
  
   

                                       
#define EXC_RESOURCE_CPUMONITOR_DECODE_INTERVAL(code) \
	(((code) >> 7) & 0x1FFFFFFULL)
#define EXC_RESOURCE_CPUMONITOR_DECODE_PERCENTAGE(code) \
	((code) & 0x7FULL)
#define EXC_RESOURCE_CPUMONITOR_DECODE_PERCENTAGE_OBSERVED(subcode) \
	((subcode) & 0x7FULL)


                                   
#define FLAVOR_WAKEUPS_MONITOR  1

  
                                                  
  
                                                    
                              
                                     
  
        
                                                    
                                                    
                                                    
                                                    
                                                    
                                                    
                                                    
  
           
                                                    
                                                    
                                                    
                                                    
  
   

#define EXC_RESOURCE_CPUMONITOR_DECODE_WAKEUPS_PERMITTED(code) \
	((code) & 0xFFFULL)
#define EXC_RESOURCE_CPUMONITOR_DECODE_OBSERVATION_INTERVAL(code) \
	(((code) >> 20) & 0xFFFFFULL)
#define EXC_RESOURCE_CPUMONITOR_DECODE_WAKEUPS_OBSERVED(subcode) \
	((subcode) & 0xFFFFFULL)

                                  
#define FLAVOR_HIGH_WATERMARK   1

  
                                               
                            
  
                                                          
                          
  
        
                                                     
                                                     
                                                     
                                                     
                                                     
                                                     
  
           
                                                     
                                                     
                                                     
  
   

#define EXC_RESOURCE_HWM_DECODE_LIMIT(code) \
	((code) & 0x1FFFULL)

                              
#define FLAVOR_IO_PHYSICAL_WRITES               1
#define FLAVOR_IO_LOGICAL_WRITES                2

  
                                             
  
                                                     
              
  
        
                                                    
                                                    
                                                    
                                                    
                                                    
                                                    
  
           
                                                    
                                                    
                                                    
                                                    
  
   

                                      
#define EXC_RESOURCE_IO_DECODE_INTERVAL(code) \
	(((code) >> 15) & 0x1FFFFULL)
#define EXC_RESOURCE_IO_DECODE_LIMIT(code) \
	((code) & 0x7FFFULL)
#define EXC_RESOURCE_IO_OBSERVED(subcode) \
	((subcode) & 0x7FFFULL)


  
                                                 
  
                                                     
                
   

#define EXC_RESOURCE_THREADS_DECODE_THREADS(code) \
	((code) & 0x7FFFULL)

                                   
#define FLAVOR_THREADS_HIGH_WATERMARK 1

#ifdef KERNEL

                                                  
#define EXC_RESOURCE_ENCODE_TYPE(code, type) \
	((code) |= (((uint64_t)(type) & 0x7ULL) << 61))
#define EXC_RESOURCE_ENCODE_FLAVOR(code, flavor) \
	((code) |= (((uint64_t)(flavor) & 0x7ULL) << 58))

                                                                    
#define EXC_RESOURCE_CPUMONITOR_ENCODE_INTERVAL(code, interval) \
	((code) |= (((uint64_t)(interval) & 0x1FFFFFFULL) << 7))
#define EXC_RESOURCE_CPUMONITOR_ENCODE_PERCENTAGE(code, percentage) \
	((code) |= (((uint64_t)(percentage) & 0x7FULL)))

                                                                            
#define EXC_RESOURCE_CPUMONITOR_ENCODE_WAKEUPS_PERMITTED(code, num) \
	((code) |= ((uint64_t)(num) & 0xFFFFFULL))
#define EXC_RESOURCE_CPUMONITOR_ENCODE_OBSERVATION_INTERVAL(code, num) \
	((code) |= (((uint64_t)(num) & 0xFFFULL) << 20))
#define EXC_RESOURCE_CPUMONITOR_ENCODE_WAKEUPS_OBSERVED(subcode, num) \
	((subcode) |= ((uint64_t)(num) & 0xFFFFFULL))

                                                                          
#define EXC_RESOURCE_HWM_ENCODE_LIMIT(code, num) \
	((code) |= ((uint64_t)(num) & 0x1FFFULL))

                                                                                                   
#define EXC_RESOURCE_IO_ENCODE_INTERVAL(code, interval) \
	((code) |= (((uint64_t)(interval) & 0x1FFFFULL) << 15))
#define EXC_RESOURCE_IO_ENCODE_LIMIT(code, limit) \
	((code) |= (((uint64_t)(limit) & 0x7FFFULL)))
#define EXC_RESOURCE_IO_ENCODE_OBSERVED(subcode, num) \
	((subcode) |= (((uint64_t)(num) & 0x7FFFULL)))

                                                    
#define EXC_RESOURCE_THREADS_ENCODE_THREADS(code, threads) \
	((code) |= (((uint64_t)(threads) & 0x7FFFULL)))

#endif             


#endif                       
