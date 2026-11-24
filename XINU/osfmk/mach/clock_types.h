/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
 * @OSF_COPYRIGHT@
 */
  
                       
                                                    
                                              
                          
   

  
                                            
   

#ifndef _MACH_CLOCK_TYPES_H_
#define _MACH_CLOCK_TYPES_H_

#include <stdint.h>
#include <mach/time_value.h>

  
                    
   
typedef int     alarm_type_t;                                
typedef int     sleep_type_t;                                
typedef int     clock_id_t;                                                    
typedef int     clock_flavor_t;                                
typedef int     *clock_attr_t;                                    
typedef int     clock_res_t;                                       

  
                                                               
   
struct mach_timespec {
	unsigned int    tv_sec;                              
	clock_res_t             tv_nsec;                                 
};
typedef struct mach_timespec    mach_timespec_t;

  
                                               
   
#define SYSTEM_CLOCK            0
#define CALENDAR_CLOCK          1

#define REALTIME_CLOCK          0

  
                   
   
#define CLOCK_GET_TIME_RES      1                                     
                                             
#define CLOCK_ALARM_CURRES      3                                     
#define CLOCK_ALARM_MINRES      4                                     
#define CLOCK_ALARM_MAXRES      5                                     

#define NSEC_PER_USEC   1000ull                                          
#define USEC_PER_SEC    1000000ull                                   
#define NSEC_PER_SEC    1000000000ull                               
#define NSEC_PER_MSEC   1000000ull                                       

#define BAD_MACH_TIMESPEC(t)                                            \
	((t)->tv_nsec < 0 || (t)->tv_nsec >= (long)NSEC_PER_SEC)

                                                            
#define CMP_MACH_TIMESPEC(t1, t2)                                       \
	((t1)->tv_sec > (t2)->tv_sec ? (long) +NSEC_PER_SEC :   \
	((t1)->tv_sec < (t2)->tv_sec ? (long) -NSEC_PER_SEC :   \
	                (t1)->tv_nsec - (t2)->tv_nsec))

               
#define ADD_MACH_TIMESPEC(t1, t2)                                                               \
  do {                                                                                                                  \
	if (((t1)->tv_nsec += (t2)->tv_nsec) >= (long) NSEC_PER_SEC) {          \
	        (t1)->tv_nsec -= (long) NSEC_PER_SEC;                                                   \
	        (t1)->tv_sec  += 1;                                                                             \
	}                                                                                                                       \
	(t1)->tv_sec += (t2)->tv_sec;                                                           \
  } while (0)

               
#define SUB_MACH_TIMESPEC(t1, t2)                                                               \
  do {                                                                                                                  \
	if (((t1)->tv_nsec -= (t2)->tv_nsec) < 0) {                                     \
	        (t1)->tv_nsec += (long) NSEC_PER_SEC;                                                   \
	        (t1)->tv_sec  -= 1;                                                                             \
	}                                                                                                                       \
	(t1)->tv_sec -= (t2)->tv_sec;                                                           \
  } while (0)

  
                           
   
#define ALRMTYPE                        0xff                                    
#define TIME_ABSOLUTE           0x00                               
#define TIME_RELATIVE           0x01                               

#define BAD_ALRMTYPE(t)         (((t) &~ TIME_RELATIVE) != 0)

#endif                           
