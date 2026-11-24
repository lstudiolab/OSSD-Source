/*
 * Copyright (c) 2016 Apple Inc. All rights reserved.
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
/*
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

  
                                           
   

#ifndef _EXC_GUARD_H_
#define _EXC_GUARD_H_

  
                                      
  
        
                                                        
                                                        
                                                        
  
           
                                                        
                                                        
                                                        
   

#define EXC_GUARD_DECODE_GUARD_TYPE(code) \
	((((uint64_t)(code)) >> 61) & 0x7ull)
#define EXC_GUARD_DECODE_GUARD_FLAVOR(code) \
	((((uint64_t)(code)) >> 32) & 0x1fffffff)
#define EXC_GUARD_DECODE_GUARD_TARGET(code) \
	((uint32_t)(code))

                     

#define GUARD_TYPE_NONE         0x0

  
                                                      
  
        
                                                                     
                                                                     
                                                                     
  
           
                                                                     
                                                                     
                                                                     
   

#define GUARD_TYPE_MACH_PORT    0x1                             

  
                                                       
  
        
                                                                     
                                                                     
                                                                     
  
           
                                                                     
                                                                     
                                                                     
   

#define GUARD_TYPE_FD           0x2                                  

  
                                                      
  
        
                                                                     
                                                                     
                                                                     
  
           
                                                                     
                                                                     
                                                                     
   

#define GUARD_TYPE_USER         0x3                              

  
                                                  
  
        
                                                                     
                                                                     
                                                                     
  
           
                                                                     
                                                                     
                                                                     
   

#define GUARD_TYPE_VN           0x4                        

  
                                               
  
        
                                                                       
                                                                       
                                                                       
  
           
                                                                     
                                                                     
                                                                     
   

#define GUARD_TYPE_VIRT_MEMORY  0x5                                       

#ifdef KERNEL

#define EXC_GUARD_ENCODE_TYPE(code, type) \
	((code) |= (((uint64_t)(type) & 0x7ull) << 61))
#define EXC_GUARD_ENCODE_FLAVOR(code, flavor) \
	((code) |= (((uint64_t)(flavor) & 0x1fffffffull) << 32))
#define EXC_GUARD_ENCODE_TARGET(code, target) \
	((code) |= (((uint64_t)(target) & 0xffffffffull)))

#endif             

#endif                    
