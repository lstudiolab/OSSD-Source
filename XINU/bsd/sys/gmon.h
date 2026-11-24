/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1982, 1986, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)gmon.h	8.2 (Berkeley) 1/4/94
 */

#ifndef _SYS_GMON_H_
#define _SYS_GMON_H_
#include <stdint.h>
#include <sys/types.h>

  
                                                       
   
struct gmonhdr {
	uint32_t lpc;                                                 
	uint32_t hpc;                                                 
	uint32_t ncnt;                                                        
	int32_t version;                            
	int32_t profrate;                                 
	int32_t spare[3];                     
};
#define GMONVERSION     0x00051879

struct gmonhdr_64 {
	uint64_t lpc;                                                 
	uint64_t hpc;                                                 
	uint32_t ncnt;                                                        
	int32_t version;                            
	int32_t profrate;                                 
	int32_t spare[3];                     
};

typedef struct
#ifndef __LP64__
    gmonhdr
#else
    gmonhdr_64
#endif
    gmonhdr_t;

  
                                                                    
   
#define HISTCOUNTER     unsigned short

  
                                                                      
   
#define HISTFRACTION    2

  
                                                            
                                                                    
                                                                       
                                                             
                                 
  
                                                                
  
                                                              
  
                
                
  
                                                               
                 
  
                                     
  
                                                                      
                                                    
  
                                                                  
                                                                   
                                                              
                                                              
                   
   
#define HASHFRACTION    2

  
                                                                  
   
#define ARCDENSITY      2
#define MINARCS         50
#define MAXARCS         ((1 << (8 * sizeof(HISTCOUNTER))) - 2)

struct tostruct {
	uint32_t        selfpc;
	int32_t         count;
	uint16_t        link;
	uint16_t        order;
};

struct tostruct_64 {
	uint64_t        selfpc;
	int32_t         count;
	uint16_t        link;
	uint16_t        order;
};

typedef struct
#ifndef __LP64__
    tostruct
#else
    tostruct_64
#endif
    tostruct_t;

  
                                                   
                               
   
struct rawarc {
	uint32_t        raw_frompc;
	uint32_t        raw_selfpc;
	int32_t         raw_count;
};

struct rawarc_64 {
	uint64_t        raw_frompc;
	uint64_t        raw_selfpc;
	int32_t         raw_count;
};

typedef struct
#ifndef __LP64__
    rawarc
#else
    rawarc_64
#endif
    rawarc_t;

  
                              
   
#define ROUNDDOWN(x, y)  (((x)/(y))*(y))
#define ROUNDUP(x, y)    ((((x)+(y)-1)/(y))*(y))

  
                                                              
   
struct gmonparam {
	int             state;
	u_short         *kcount;
	u_long          kcountsize;
	u_short         *froms;
	u_long          fromssize;
	tostruct_t      *tos;
	u_long          tossize;
	long            tolimit;
	u_long          lowpc;
	u_long          highpc;
	u_long          textsize;
	u_long          hashfraction;
};
extern struct gmonparam _gmonparam;

  
                                
   
#define GMON_PROF_ON    0
#define GMON_PROF_BUSY  1
#define GMON_PROF_ERROR 2
#define GMON_PROF_OFF   3

  
                                                                        
                                                                            
                                                                     
                                  
   
#define GMON_MAGIC 0xbeefbabe
#define GMON_MAGIC_64 0xbeefbabf
typedef struct gmon_data {
	uint32_t type;                                                      
	uint32_t size;                                                      
} gmon_data_t;

  
                                                                              
                                                      
   
#define GMONTYPE_SAMPLES        1
  
                                                                           
   
#define GMONTYPE_RAWARCS        2
  
                                                                          
                                                                               
                                                                             
                                                                       
   
#define GMONTYPE_ARCS_ORDERS    3
struct rawarc_order {
	uint32_t    raw_frompc;
	uint32_t    raw_selfpc;
	uint32_t    raw_count;
	uint32_t    raw_order;
}; struct rawarc_order_64 {
	uint64_t    raw_frompc;
	uint64_t    raw_selfpc;
	uint32_t    raw_count;
	uint32_t    raw_order;
};

typedef struct
#ifndef __LP64__
    rawarc_order
#else
    rawarc_order_64
#endif
    rawarc_order_t;

  
                                                                              
                  
                                                                     
                   
                                                                
                                                                          
                                                        
                                                                              
                   
   
#define GMONTYPE_DYLD_STATE     4

  
                                                                               
                  
                                                                     
                   
                                                                
                                                               
                                                        
                                                                              
                   
   
#define GMONTYPE_DYLD2_STATE     5

#endif                    
