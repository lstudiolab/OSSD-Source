/*
 * Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.
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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_LICENSE_HEADER_END@
 */
#ifndef _MACHO_NLIST_H_
#define _MACHO_NLIST_H_
                                                          

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)nlist.h	8.2 (Berkeley) 1/21/94
 */
#include <stdint.h>

  
                                                                            
                                                                            
                                                                            
                                                                                
                                                                               
        
   
struct nlist {
	union {
#ifndef __LP64__
		char *n_name;	                          
#endif
		uint32_t n_strx;	                                 
	} n_un;
	uint8_t n_type;		                          
	uint8_t n_sect;		                               
	int16_t n_desc;		                         
	uint32_t n_value;	                                           
};

  
                                                                     
   
struct nlist_64 {
    union {
        uint32_t  n_strx;                                  
    } n_un;
    uint8_t n_type;                                  
    uint8_t n_sect;                                       
    uint16_t n_desc;                                
    uint64_t n_value;                                                 
};

  
                                                                            
                                                                              
                                                                               
                                       
   

  
                                                
                          
                   
                   
                  
                                          
   
#define	N_STAB	0xe0                                                            
#define	N_PEXT	0x10                                   
#define	N_TYPE	0x0e                              
#define	N_EXT	0x01                                                     

  
                                                                              
                                                                            
                                                                              
                     
   

  
                                              
   
#define	N_UNDF	0x0		                                  
#define	N_ABS	0x2		                                 
#define	N_SECT	0xe		                                      
#define	N_PBUD	0xc		                                             
#define N_INDR	0xa		              

   
                                                                             
                                                                            
                                                                               
                                      
   

  
                                                                         
                                                                           
                                                                           
                                                                             
                               
  
                                                                           
                                                                               
                                                                             
                                                                              
   
#define	NO_SECT		0	                                  
#define MAX_SECT	255	                          

  
                                                                              
                                                                               
                                                                              
                                                                           
                                                                              
                                                                             
                    
   
#define GET_COMM_ALIGN(n_desc) (((n_desc) >> 8) & 0x0f)
#define SET_COMM_ALIGN(n_desc,align) \
    (n_desc) = (((n_desc) & 0xf0ff) | (((align) & 0x0f) << 8))

  
                                                                               
                                                                              
                                                                        
                                                                            
                                                                                
                                                                            
                                            
  
                                                                         
                                                                             
                                                                               
                                                                         
                                          
  
                                                                              
                                                                             
                                        
   
                                                                  
#define REFERENCE_TYPE				0x7
                         
#define REFERENCE_FLAG_UNDEFINED_NON_LAZY		0
#define REFERENCE_FLAG_UNDEFINED_LAZY			1
#define REFERENCE_FLAG_DEFINED				2
#define REFERENCE_FLAG_PRIVATE_DEFINED			3
#define REFERENCE_FLAG_PRIVATE_UNDEFINED_NON_LAZY	4
#define REFERENCE_FLAG_PRIVATE_UNDEFINED_LAZY		5

  
                                                                           
                                                                              
                                                                              
                                                            
   
#define REFERENCED_DYNAMICALLY	0x0010

  
                                                                            
                                                                     
                                                                             
                                                                              
                                                                             
                                                                         
                                                                 
                                                                   
                                                                         
                                                
                                                                         
                                                                                
                                                                            
                                                                               
                                                                         
                                                                            
        
   
                                                                               
                                                                                
                                                                       
                                                                             
                                                                               
                                                                              
                      
   
#define GET_LIBRARY_ORDINAL(n_desc) (((n_desc) >> 8) & 0xff)
#define SET_LIBRARY_ORDINAL(n_desc,ordinal) \
	(n_desc) = (((n_desc) & 0x00ff) | (((ordinal) & 0xff) << 8))
#define SELF_LIBRARY_ORDINAL 0x0
#define MAX_LIBRARY_ORDINAL 0xfd
#define DYNAMIC_LOOKUP_ORDINAL 0xfe
#define EXECUTABLE_ORDINAL 0xff

  
                                                                              
                                                                              
   

  
                                                                      
                                                                           
                                                           
   
#define N_NO_DEAD_STRIP 0x0020                                        

  
                                                                              
                                                                          
                                                         
   
#define N_DESC_DISCARDED 0x0020	                         

  
                                                                              
                                                                              
                     
   
#define N_WEAK_REF	0x0040                                

  
                                                                             
                                                                            
                                                                                
                                                      
   
#define N_WEAK_DEF	0x0080                                           

  
                                                                            
                                                                               
   
#define	N_REF_TO_WEAK	0x0080                                 

  
                                                                           
                                   
   
#define N_ARM_THUMB_DEF	0x0008                                       

  
                                                                   
                                                               
                                                            
                                                              
   
#define N_SYMBOL_RESOLVER  0x0100 

  
                                                             
                                            
   
#define N_ALT_ENTRY 0x0200

#ifndef __STRICT_BSD__
#ifdef __cplusplus
extern "C" {
#endif                  
  
                                            
   
extern int nlist (const char *filename, struct nlist *list);
#ifdef __cplusplus
}
#endif                  
#endif                     

#endif                     
