/*
 * Copyright (c) 2000-2005 Apple Computer, Inc. All rights reserved.
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

#ifndef _SYS_UTFCONV_H_
#define _SYS_UTFCONV_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>

#ifdef KERNEL
#ifdef __APPLE_API_UNSTABLE

  
                            
   
#define UTF_REVERSE_ENDIAN   0x0001                                 
#define UTF_NO_NULL_TERM     0x0002                                    
#define UTF_DECOMPOSED       0x0004                                        
#define UTF_PRECOMPOSED      0x0008                                   
#define UTF_ESCAPE_ILLEGAL   0x0010                             
#define UTF_SFM_CONVERSIONS  0x0020                                                

#define UTF_BIG_ENDIAN       \
	((BYTE_ORDER == BIG_ENDIAN) ? 0 : UTF_REVERSE_ENDIAN)

#define UTF_LITTLE_ENDIAN    \
	((BYTE_ORDER == LITTLE_ENDIAN) ? 0 : UTF_REVERSE_ENDIAN)

__BEGIN_DECLS


  
                                                               
  
                                                                   
                                                
   

int unicode_combinable(u_int16_t character);

  
                                    
  
                                                 
   

int unicode_decomposeable(u_int16_t character);


  
                                                       
  
                                                                     
                                                                      
                                                                     
  
        
                                                                         
  
                                                              
  
                                                                    
  
                                                     
  
         
          
   
size_t
utf8_encodelen(const u_int16_t * ucsp, size_t ucslen, u_int16_t altslash,
    int flags);


  
                                                       
  
                                                                     
                                                                      
                                                                     
                                                                    
                                                                 
  
                                                                   
                                                   
  
        
                                                                         
  
                                                              
  
                                                                    
  
                                                                     
  
                                                       
  
         
                                                                        
  
                                               
   
int
utf8_encodestr(const u_int16_t * ucsp, size_t ucslen, u_int8_t * utf8p,
    size_t * utf8len, size_t buflen, u_int16_t altslash, int flags);


  
                                                       
  
                                                                     
                                                                        
                                                                    
                                                                    
                    
  
                                                                   
                                                   
  
        
                                                                     
  
                                                              
  
                                                                    
  
                                                             
  
                                                         
  
                                                                 
  
         
                                                                        
  
                                                  
   
int
utf8_decodestr(const u_int8_t* utf8p, size_t utf8len, u_int16_t* ucsp,
    size_t *ucslen, size_t buflen, u_int16_t altslash, int flags);


  
                                                            
  
                                                                   
                                                                     
                                                                       
                                                                       
                                                                      
                    
  
        
                                                                   
  
                                                               
  
                                                                     
  
                                                                 
  
         
                                                                          
  
                                                                  
   
int
utf8_normalizestr(const u_int8_t* instr, size_t inlen, u_int8_t* outstr,
    size_t *outlen, size_t buflen, int flags);


  
                                              
  
                                                                     
                                                                       
          
  
         
                                                  
   
int
utf8_validatestr(const u_int8_t* utf8p, size_t utf8len);


__END_DECLS

#endif                           
#endif             

#endif                       
