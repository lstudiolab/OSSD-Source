/*
 * Copyright (c) 2008 Apple Inc. All rights reserved.
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
/* inflate.h -- internal inflate state definition
 * Copyright (C) 1995-2004 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

                                                                 
                                                               
                                                          
   

                                                                       
                                                                            
                                                                           
                             
#ifndef NO_GZIP
#  define GUNZIP
#endif

                                                    
typedef enum {
    HEAD,                                        
    FLAGS,                                                  
    TIME,                                                    
    OS,                                                                     
    EXLEN,                                              
    EXTRA,                                             
    NAME,                                                   
    COMMENT,                                              
    HCRC,                                             
    DICTID,                                                
    DICT,                                                    
        TYPE,                                                              
        TYPEDO,                                                               
        STORED,                                                             
        COPY,                                                                  
        TABLE,                                                      
        LENLENS,                                                 
        CODELENS,                                                            
            LEN,                                            
            LENEXT,                                           
            DIST,                                         
            DISTEXT,                                            
            MATCH,                                                      
            LIT,                                                          
    CHECK,                                             
    LENGTH,                                              
    DONE,                                                            
    BAD,                                                         
    MEM,                                                                      
    SYNC                                                                    
} inflate_mode;

  
                                           

                                                                       

                   
                                
                                                               
                                       
                                
                              
                        
                                                   
                                  
                                               
                       
                                            
                                                         
                          
                    
                               
   

                                                                        
struct inflate_state {
    inflate_mode mode;                                    
    int last;                                                      
    int wrap;                                                                 
    int havedict;                                                
    int flags;                                                                
    unsigned dmax;                                                             
    unsigned long check;                                           
    unsigned long total;                                            
    gz_headerp head;                                                       
                            
    unsigned wbits;                                                      
    unsigned wsize;                                                          
    unsigned whave;                                            
    unsigned write;                                     
    unsigned char FAR *window;                                           
                             
    unsigned long hold;                                    
    unsigned bits;                                          
                                                 
    unsigned length;                                                   
    unsigned offset;                                                   
                                         
    unsigned extra;                                    
                                           
    code const FAR *lencode;                                                 
    code const FAR *distcode;                                          
    unsigned lenbits;                                       
    unsigned distbits;                                       
                                    
    unsigned ncode;                                                     
    unsigned nlen;                                                 
    unsigned ndist;                                                  
    unsigned have;                                                    
    code FAR *next;                                                  
    unsigned short lens[320];                                           
    unsigned short work[288];                                          
    code codes[ENOUGH];                                    
};
