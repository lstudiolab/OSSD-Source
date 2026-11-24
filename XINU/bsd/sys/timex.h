/*-
 ***********************************************************************
 *								       *
 * Copyright (c) David L. Mills 1993-2001			       *
 * Copyright (c) Poul-Henning Kamp 2000-2001                           *
 *								       *
 * Permission to use, copy, modify, and distribute this software and   *
 * its documentation for any purpose and without fee is hereby	       *
 * granted, provided that the above copyright notice appears in all    *
 * copies and that both the copyright notice and this permission       *
 * notice appear in supporting documentation, and that the name        *
 * University of Delaware not be used in advertising or publicity      *
 * pertaining to distribution of the software without specific,	       *
 * written prior permission. The University of Delaware makes no       *
 * representations about the suitability this software for any	       *
 * purpose. It is provided "as is" without express or implied	       *
 * warranty.							       *
 *								       *
 ***********************************************************************
 *
 * $FreeBSD$
 *
 * This header file defines the Network Time Protocol (NTP) interfaces
 * for user and daemon application programs.
 *
 * This file was originally created 17 Sep 93 by David L. Mills, Professor
 * of University of Delaware, building on work which had already been ongoing
 * for a decade and a half at that point in time.
 *
 * In 2000 the APIs got a upgrade from microseconds to nanoseconds,
 * a joint work between Poul-Henning Kamp and David L. Mills.
 *
 */

/*
 * Copyright (c) 2017 Apple Computer, Inc. All rights reserved.
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

#ifndef _SYS_TIMEX_H_
#define _SYS_TIMEX_H_ 1

#include <sys/time.h>

#define NTP_API         4                                    

  
                                                                  
                                                                 
                                                                        
                                                                     
                                                                        
                                                                       
                                  
   

#define MAXPHASE        500000000L                                
#define MAXFREQ         500000L                                    
#define MINSEC          256                                              
#define MAXSEC          2048                                             
#define NANOSECOND      1000000000L                                    
#define SCALE_PPM       (65536 / 1000)                                
#define MAXTC           10                                     

                                                                                 
                                                    
   

  
                                   
   
#define MOD_OFFSET      0x0001                               
#define MOD_FREQUENCY   0x0002                                    
#define MOD_MAXERROR    0x0004                                      
#define MOD_ESTERROR    0x0008                                        
#define MOD_STATUS      0x0010                                     
#define MOD_TIMECONST   0x0020                                     
#define MOD_PPSMAX      0x0040                                              
#define MOD_TAI         0x0080                              
#define MOD_MICRO       0x1000                                             
#define MOD_NANO        0x2000                                            
#define MOD_CLKB        0x4000                              
#define MOD_CLKA        0x8000                              

  
                              
   
#define STA_PLL         0x0001                                       
#define STA_PPSFREQ     0x0002                                               
#define STA_PPSTIME     0x0004                                               
#define STA_FLL         0x0008                                    
#define STA_INS         0x0010                                
#define STA_DEL         0x0020                                
#define STA_UNSYNC      0x0040                                         
#define STA_FREQHOLD    0x0080                                   
#define STA_PPSSIGNAL   0x0100                                       
#define STA_PPSJITTER   0x0200                                               
#define STA_PPSWANDER   0x0400                                               
#define STA_PPSERROR    0x0800                                                 
#define STA_CLOCKERR    0x1000                                         
#define STA_NANO        0x2000                                                
#define STA_MODE        0x4000                                            
#define STA_CLK         0x8000                                                

#define STA_RONLY (STA_PPSSIGNAL | STA_PPSJITTER | STA_PPSWANDER | \
    STA_PPSERROR | STA_CLOCKERR | STA_NANO | STA_MODE | STA_CLK)

#define STA_SUPPORTED (STA_PLL | STA_FLL | STA_UNSYNC | STA_FREQHOLD | \
    STA_CLOCKERR | STA_NANO | STA_MODE | STA_CLK)

  
                                       
   
#define TIME_OK         0                                           
#define TIME_INS        1                                               
#define TIME_DEL        2                                               
#define TIME_OOP        3                                            
#define TIME_WAIT       4                                             
#define TIME_ERROR      5                                            

  
                                                                       
   
struct ntptimeval {
	struct timespec time;                                       
	long maxerror;                                               
	long esterror;                                                 
	long tai;                                       
	int time_state;                                  
};

  
                                                                      
                                           
  
                                                                        
                                           
   
struct timex {
	unsigned int modes;                                       
	long    offset;                                               
	long    freq;                                                           
	long    maxerror;                                            
	long    esterror;                                              
	int     status;                                             
	long    constant;                                                
	long    precision;                                                
	long    tolerance;                                                  
                                                
	  
                                                         
                                                              
                                        
    
	long    ppsfreq;                                                     
	long    jitter;                                              
	int     shift;                                                          
	long    stabil;                                                      
	long    jitcnt;                                                 
	long    calcnt;                                                 
	long    errcnt;                                              
	long    stbcnt;                                                    
};

#ifdef KERNEL
#ifdef XNU_KERNEL_PRIVATE
#include <sys/_types/_user32_timex.h>
#include <sys/_types/_user64_timex.h>
#include <sys/_types/_user32_ntptimeval.h>
#include <sys/_types/_user64_ntptimeval.h>
#include <kern/clock.h>

int64_t ntp_get_freq(void);
void    ntp_update_second(int64_t *adjustment, clock_sec_t secs);
void    ntp_init(void);
#endif
#else               
#include <sys/cdefs.h>

__BEGIN_DECLS
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
int     ntp_adjtime(struct timex *);
int     ntp_gettime(struct ntptimeval *);
#endif                                             
__END_DECLS
#endif             


#endif                     
