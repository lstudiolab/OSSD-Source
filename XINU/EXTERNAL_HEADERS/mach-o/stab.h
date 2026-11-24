/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
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
#ifndef _MACHO_STAB_H_
#define _MACHO_STAB_H_
                                                         

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)stab.h	5.2 (Berkeley) 4/4/91
 */

  
                                                                           
                                                                          
                                                                               
                                                                              
                                                                                
                                                                           
                                                             
  
                                                                             
                                                                            
                                                                               
                                                                               
                                                
   

  
                                                                         
   
                                                    
  
                                                                             
                                                                         
               
   
#define	N_GSYM	0x20	                                         
#define	N_FNAME	0x22	                                                    
#define	N_FUN	0x24	                                                
#define	N_STSYM	0x26	                                              
#define	N_LCSYM	0x28	                                              
#define N_BNSYM 0x2e	                                          
#define N_AST	0x32	                                      
#define N_OPT	0x3c	                                                  
#define	N_RSYM	0x40	                                               
#define	N_SLINE	0x44	                                            
#define N_ENSYM 0x4e	                                        
#define	N_SSYM	0x60	                                                     
#define	N_SO	0x64	                                              
#define	N_OSO	0x66	                                          
#define	N_LSYM	0x80	                                          
#define N_BINCL	0x82	                                                 
#define	N_SOL	0x84	                                                 
#define	N_PARAMS  0x86	                                            
#define	N_VERSION 0x88	                                         
#define	N_OLEVEL  0x8A	                                          
#define	N_PSYM	0xa0	                                          
#define N_EINCL	0xa2	                                         
#define	N_ENTRY	0xa4	                                                      
#define	N_LBRAC	0xc0	                                                    
#define N_EXCL	0xc2	                                               
#define	N_RBRAC	0xe0	                                                     
#define	N_BCOMM	0xe2	                                     
#define	N_ECOMM	0xe4	                                  
#define	N_ECOML	0xe8	                                                  
#define	N_LENG	0xfe	                                               

  
                                           
   
#define	N_PC	0x30	                                                      

#endif                     
