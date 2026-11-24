/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

#ifndef _PRNG_ENTROPY_H_
#define _PRNG_ENTROPY_H_

#include <kern/kern_types.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

                                                                     
                                          
  
                                                                 
                                                                
                                                                      
  
                                                               
                                                                   
                                                                    
                                            

typedef uint32_t entropy_sample_t;

                                                                
void entropy_init(void);

                                                                  
              
void entropy_collect(void);

                                                                  
                                                                   
                                                                
                              
int32_t entropy_provide(size_t *entropy_size, void *entropy, void *arg);

extern entropy_sample_t *entropy_analysis_buffer;
extern uint32_t entropy_analysis_buffer_size;

typedef struct entropy_health_stats {
	                                                            
	                                                               
	                                                                
	                     
	uint32_t reset_count;

	                                                        
	                                                           
	                                                
	uint32_t failure_count;

	                                                                 
	                                     
	uint32_t max_observation_count;
} entropy_health_stats_t;

extern int entropy_health_startup_done;
extern entropy_health_stats_t entropy_health_rct_stats;
extern entropy_health_stats_t entropy_health_apt_stats;

__END_DECLS

#endif                       
