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
  
          
  
                                                  
                                       
  
                                                  
                                     
  
                                                
                                             
                                   
  
           
   

#ifndef _MACH_TYPES_H_
#define _MACH_TYPES_H_

#include <stddef.h>
#include "libsa/machine/types.h"

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long   size_t;
#endif               

  
                                                               
   

typedef unsigned char   u_char;                            
typedef unsigned short  u_short;                            
typedef unsigned int    u_int;                            
typedef unsigned long   u_long;                            

typedef struct _quad_ {
	unsigned int    val[2];                                      
} quad;                                                     

typedef char *          caddr_t;                                        

typedef unsigned int    daddr_t;                            
#if 0                               
typedef unsigned int    off_t;                                   
#endif


#define major(i)        (((i) >> 8) & 0xFF)
#define minor(i)        ((i) & 0xFF)
#define makedev(i, j)    ((((i) & 0xFF) << 8) | ((j) & 0xFF))

#ifndef NULL
#define NULL            ((void *) 0)                          
#endif

  
                                                          
   
typedef unsigned char   uchar_t;
typedef unsigned short  ushort_t;
typedef unsigned int    uint_t;
typedef unsigned long   ulong_t;
typedef volatile unsigned char  vuchar_t;
typedef volatile unsigned short vushort_t;
typedef volatile unsigned int   vuint_t;
typedef volatile unsigned long  vulong_t;

  
                    
   
#if __GNUC__ >= 3
#define __deprecated    __attribute__((__deprecated__))
#else
#define __deprecated              
#endif

#endif                      
