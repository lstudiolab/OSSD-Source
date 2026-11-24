/*
 * Copyright (c) 2016-2020 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef unicode_h
#define unicode_h

#ifdef KERNEL_PRIVATE

#include <sys/cdefs.h>
#include <stdbool.h>

  
                                                                                       
                                                                                      
                                       
   

enum {
	                                                                    
	kNCFStreamSafeBufMax = 32
};

  
                                   
  
                                                                                   
                                                                                
                                       
                                                                                
                                                                           
                                                                     
  
                                                                                 
                                                                                 
                                           
  
                                                               
                                                                              
                                                                                     
                                                                       
                                                                        
                                                                       
                                                                       
                            
                                                
  
                            
                                                                            
                                                                              
                                                                                
                                                               
   
int utf8_normalizeOptCaseFoldAndHash(const char *str,
    size_t      str_len,
    bool        case_sens,
    void      (*hash_func)(void *buf, size_t buf_len, void *ctx),
    void       *hash_ctx);

  
                                      
  
                                                                                    
                                                                       
                                                                                  
                                                                       
                                                                                         
  
                                                                                 
                                                                                 
                                             
  
                                                                      
                                                                  
                                                                               
                                                                  
                                                                                     
                                                                            
                                                                               
                              
  
                            
                                                                                 
                                                                                      
                                                                                   
                                                               
                                                                                 
                                                                                  
                                                             
   
int utf8_normalizeOptCaseFoldAndCompare(const char *strA,
    size_t      strA_len,
    const char *strB,
    size_t      strB_len,
    bool        case_sens,
    bool       *are_equal);

  
                            
  
                                                                                   
                                                                           
          
                                                                                
                                                                           
                                                                     
  
                                                                                 
                                                                                 
                                           
  
                                                               
                                                                              
                                                                                     
                                                                       
                                                                    
                                                    
                                                                              
                                                           
  
                            
                                                                            
                                                                              
                                                                                
                                                               
                                                                               
                                                                    
   
int utf8_normalizeOptCaseFold(const char *str,
    size_t      str_len,
    bool        case_sens,
    int32_t    *ustr,
    int32_t     ustr_size,
    int32_t    *ustr_len);

  
                                  
  
                                                                                  
                                                                           
          
                                                                                
                                                                           
                                                                     
  
                                                                                 
                                                                                 
                                           
  
                                                               
                                                                              
                                                                                     
                                                                       
                                                                   
                                             
                                                                              
                                                    
  
                            
                                                                            
                                                                              
                                                                                
                                                               
                                                                               
                                                                    
   
int utf8_normalizeOptCaseFoldToUTF8(const char *str,
    size_t      str_len,
    bool        case_sens,
    char       *ustr,
    size_t      ustr_size,
    size_t     *ustr_len);

  
                                             
  
                                                                                      
                                                                                        
                                                                                         
                                           
                                                                                  
                                                                       
                                                                                 
  
                                                                                      
                                                                                 
                                           
  
                                                                                  
                                           
                                                                  
                                                                                     
                                                                                       
                                                                                     
                                                                                      
                                                                               
                                                                                     
                                                                            
                                                                                  
                                                                 
                              
                                                                                      
                                                                    
  
                            
                                                                                     
                                                                                      
                                                                                    
                              
                                                                                      
                                                                                       
                                             
   
int utf8_normalizeOptCaseFoldAndMatchSubstring(const char    *strA,
    size_t         strA_len,
    const int32_t *ustrB,
    int32_t        ustrB_len,
    bool           case_sens,
    void          *buf,
    size_t         buf_size,
    bool          *has_match);

  
                                       
  
                                                                                      
                                                                                   
                                            
                                                                        
                                                                        
                                                                        
                                                                                     
                                                                            
                                               
   
void utf8_normalizeOptCaseFoldGetUVersion(unsigned char version[4]);

#endif                     

#endif                 
