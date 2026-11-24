/*
 * Coyright (c) 2005-2013 Apple Inc. All rights reserved.
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

#ifndef __MUNGE_H__
#define __MUNGE_H__

  
                            
  
                                                                      
                                                                 
                                                                      
                                                     
                         
  
                                                                     
                                                                         
  
                                                                        
  
                                  
  
                                                                          
              
  
                                                                   
                                                                   
                                                               
  
                                                                    
                                                               
                                                                 
                                            
  
                                                                  
                                   
  
                                                                        
                                                                       
                                                                          
                                                                         
                                                                             
                                        
  
                                                                         
                                                                   
   

#if __arm__ && (__BIGGEST_ALIGNMENT__ > 4)
int munge_w(const void *regs, void *args);
int munge_ww(const void *regs, void *args);
int munge_www(const void *regs, void *args);
int munge_wwww(const void *regs, void *args);
int munge_wwwww(const void *regs, void *args);
int munge_wwwwww(const void *regs, void *args);
int munge_wwwwwww(const void *regs, void *args);
int munge_wwwwwwww(const void *regs, void *args);
int munge_wl(const void *regs, void *args);
int munge_wwl(const void *regs, void *args);
int munge_wwlw(const void *regs, void *args);
int munge_wwlll(const void *regs, void *args);
int munge_wwllww(const void *regs, void *args);
int munge_wlw(const void *regs, void *args);
int munge_wlww(const void *regs, void *args);
int munge_wlwwwl(const void *regs, void *args);
int munge_wlwwwll(const void *regs, void *args);
int munge_wlwwwllw(const void *regs, void *args);
int munge_wlwwlwlw(const void *regs, void *args);
int munge_wll(const void *regs, void *args);
int munge_wllww(const void *regs, void *args);
int munge_wlll(const void *regs, void *args);
int munge_wlllww(const void *regs, void *args);
int munge_wllll(const void *regs, void *args);
int munge_wllwwll(const void *regs, void *args);
int munge_wwwlw(const void *regs, void *args);
int munge_wwwlww(const void *regs, void *args);
int munge_wwwlwww(const void *regs, void *args);
int munge_wwwl(const void *regs, void *args);
int munge_wwwwlw(const void *regs, void *args);
int munge_wwwwllww(const void *regs, void *args);
int munge_wwwwl(const void *regs, void *args);
int munge_wwwwwl(const void *regs, void *args);
int munge_wwwwwlww(const void *regs, void *args);
int munge_wwwwwllw(const void *regs, void *args);
int munge_wwwwwlll(const void *regs, void *args);
int munge_wwwwwwl(const void *regs, void *args);
int munge_wwwwwwlw(const void *regs, void *args);
int munge_wwwwwwll(const void *regs, void *args);
int munge_wsw(const void *regs, void *args);
int munge_wws(const void *regs, void *args);
int munge_wwws(const void *regs, void *args);
int munge_wwwsw(const void *regs, void *args);
int munge_llllll(const void *regs, void *args);
int munge_l(const void *regs, void *args);
int munge_ll(const void *regs, void *args);
int munge_lw(const void *regs, void *args);
int munge_lwww(const void *regs, void *args);
int munge_lwwwwwww(const void *regs, void *args);
int munge_wwlww(const void *regs, void *args);
int munge_wwlwww(const void *regs, void *args);
int munge_wwlwwwl(const void *regs, void *args);
#else
void munge_w(void *args);
void munge_ww(void *args);
void munge_www(void *args);
void munge_wwww(void *args);
void munge_wwwww(void *args);
void munge_wwwwww(void *args);
void munge_wwwwwww(void *args);
void munge_wwwwwwww(void *args);
void munge_wl(void *args);
void munge_wwl(void *args);
void munge_wwlw(void *args);
void munge_wwlll(void *args);
void munge_wwllww(void *args);
void munge_wlw(void *args);
void munge_wlww(void *args);
void munge_wlwwwl(void *args);
void munge_wlwwwll(void *args);
void munge_wlwwwllw(void *args);
void munge_wlwwlwlw(void *args);
void munge_wll(void *args);
void munge_wllww(void *args);
void munge_wlll(void *args);
void munge_wlllww(void *args);
void munge_wllll(void *args);
void munge_wllwwll(void *args);
void munge_wwwlw(void *args);
void munge_wwwlww(void *args);
void munge_wwwlwww(void *args);
void munge_wwwl(void *args);
void munge_wwwwlw(void *args);
void munge_wwwwllww(void *args);
void munge_wwwwl(void *args);
void munge_wwwwwl(void *args);
void munge_wwwwwlww(void *args);
void munge_wwwwwllw(void *args);
void munge_wwwwwlll(void *args);
void munge_wwwwwwl(void *args);
void munge_wwwwwwlw(void *args);
void munge_wwwwwwll(void *args);
void munge_wsw(void *args);
void munge_wws(void *args);
void munge_wwws(void *args);
void munge_wwwsw(void *args);
void munge_llllll(void *args);
void munge_llll(void *args);
void munge_l(void *args);
void munge_ll(void *args);
void munge_lw(void *args);
void munge_lwww(void *args);
void munge_lwwwwwww(void *args);
void munge_wwlww(void *args);
void munge_wwlwww(void *args);
void munge_wwlwwwl(void *args);
#endif                                             
#endif                  
