/*
 * Copyright (c) 2003 Apple Computer, Inc. All rights reserved.
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
  
                                                             
   

#ifndef _NET_FIREWIRE_H_
#define _NET_FIREWIRE_H_

#include <sys/appleapiopts.h>

  
                                            
   
#define FIREWIRE_EUI64_LEN              8

  
                                         
   
#define FIREWIRE_TYPE_LEN               2

  
                                                                    
   
#define FIREWIRE_HDR_LEN                (FIREWIRE_EUI64_LEN*2+FIREWIRE_TYPE_LEN)

  
                             
   
#define FIREWIRE_MIN_LEN                64

  
                             
   
#define FIREWIRE_MAX_LEN                4096

  
                                    
   
#define FIREWIRE_IS_VALID_LEN(foo)      \
	((foo) >= FIREWIRE_MIN_LEN && (foo) <= FIREWIRE_MAX_LEN)

  
                                                               
  
                                                                 
                                                
   
struct  firewire_header {
	u_char  firewire_dhost[FIREWIRE_EUI64_LEN];
	u_char  firewire_shost[FIREWIRE_EUI64_LEN];
	u_short firewire_type;                         
};

  
                             
   
struct  firewire_eui64 {
	u_char octet[FIREWIRE_EUI64_LEN];
};

  
                                       
   
struct firewire_address {
	u_char      eui64[FIREWIRE_EUI64_LEN];
	u_char      maxRec;
	u_char      spd;
	u_int16_t   unicastFifoHi;
	u_int32_t   unicastFifoLo;
};

#define FIREWIRE_ADDR_LEN       16                                           


#define FIREWIRE_MTU    (FIREWIRE_MAX_LEN - FIREWIRE_HDR_LEN)
#define FIREWIRE_MIN    (FIREWIRE_MIN_LEN - FIREWIRE_HDR_LEN)

#endif                        
