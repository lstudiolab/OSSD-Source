/*
 * Copyright (c) 2018 Apple Inc. All rights reserved.
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

  
                                                                     
   

#ifndef _NETINET_ISAKMP_H_
#define _NETINET_ISAKMP_H_

typedef u_char cookie_t[8];
typedef u_char msgid_t[4];

                                             
                                                                   
                                                                  
                                                                     
                                                                     
                                                                  
                                                                     
                                                                     
                                                                  
                                                                     
                                                                  
                                                                     
                                                                  
                                                                     
                                                                  
   
struct isakmp {
	cookie_t i_ck;                                
	cookie_t r_ck;                                
	uint8_t np;                                    
	uint8_t vers;
#define ISAKMP_VERS_MAJOR       0xf0
#define ISAKMP_VERS_MAJOR_SHIFT 4
#define ISAKMP_VERS_MINOR       0x0f
#define ISAKMP_VERS_MINOR_SHIFT 0
	uint8_t etype;                             
	uint8_t flags;                     
	msgid_t msgid;
	uint32_t len;                       
};

                             
                                                                   
                                                                  
                                                                     
                                                                  
   
struct isakmp_gen {
	uint8_t  np;                         
	uint8_t  critical;                                         
	uint16_t len;                          
};

#endif                         
