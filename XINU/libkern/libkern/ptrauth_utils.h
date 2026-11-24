/*
 * Copyright (c) 2020 Apple Inc. All rights reserved.
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

#ifndef __PTRAUTH_UTILS_H
#define __PTRAUTH_UTILS_H

#include <ptrauth.h>

                         
#define PTRAUTH_ADDR_DIVERSIFY  0x0001                                           
#define PTRAUTH_NON_NULL        0x0002                            

                                  
  
                                                                              
                                      
  
                                                                                                
                                                                                                        
                                                                                                               
                                                                                                         
                                                                                                        
                                  
  
  
                                             
                                                
                                                  
                                         
  
                                                           
  
   
#if __has_feature(ptrauth_calls)
ptrauth_generic_signature_t
ptrauth_utils_sign_blob_generic(const void * ptr, size_t len_bytes, uint64_t data, int flags);
#else
static inline ptrauth_generic_signature_t
ptrauth_utils_sign_blob_generic(__unused const void * ptr, __unused size_t len_bytes, __unused uint64_t data, __unused int flags)
{
	return 0;
}
#endif                                


                                  
  
                                                            
  
                                                                                                             
                                                                                                           
                                                                                                           
                        
  
                                                         
                                                   
                                                        
                                     
                                        
  
                                                                          
                                                
  
   
#if __has_feature(ptrauth_calls)
void
ptrauth_utils_auth_blob_generic(const void * ptr, size_t len_bytes, uint64_t data, int flags, ptrauth_generic_signature_t signature);
#else
static inline void
ptrauth_utils_auth_blob_generic(__unused const void * ptr, __unused size_t len_bytes, __unused uint64_t data, __unused int flags, __unused ptrauth_generic_signature_t signature)
{
	return;
}
#endif                                


#endif                     
