/*
 * Copyright (c) 2000-2002 Apple Computer, Inc. All rights reserved.
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
/* Copyright (c) 1997 Apple Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1982, 1986, 1990, 1993, 1994
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *	@(#)ttycom.h	8.1 (Berkeley) 3/28/94
 */

#ifndef _SYS_TTYCOM_H_
#define _SYS_TTYCOM_H_

#include <sys/ioccom.h>
  
                                                                          
                           
   

  
                                                                            
                                                                             
   
struct winsize {
	unsigned short  ws_row;                                  
	unsigned short  ws_col;                                     
	unsigned short  ws_xpixel;                                   
	unsigned short  ws_ypixel;                                 
};

#define TIOCMODG        _IOR('t', 3, int)                                    
#define TIOCMODS        _IOW('t', 4, int)                                    
#define         TIOCM_LE        0001                             
#define         TIOCM_DTR       0002                                     
#define         TIOCM_RTS       0004                                 
#define         TIOCM_ST        0010                                    
#define         TIOCM_SR        0020                                   
#define         TIOCM_CTS       0040                               
#define         TIOCM_CAR       0100                                
#define         TIOCM_CD        TIOCM_CAR
#define         TIOCM_RNG       0200                      
#define         TIOCM_RI        TIOCM_RNG
#define         TIOCM_DSR       0400                                
                                                                 
#define TIOCEXCL         _IO('t', 13)                                         
#define TIOCNXCL         _IO('t', 14)                                           
                                                               
#define TIOCFLUSH       _IOW('t', 16, int)                         
                                                                  
#define TIOCGETA        _IOR('t', 19, struct termios)                         
#define TIOCSETA        _IOW('t', 20, struct termios)                         
#define TIOCSETAW       _IOW('t', 21, struct termios)                        
#define TIOCSETAF       _IOW('t', 22, struct termios)                           
#ifdef KERNEL
#define TIOCGETA_32     _IOR('t', 19, struct termios32)                         
#define TIOCSETA_32     _IOW('t', 20, struct termios32)                         
#define TIOCSETAW_32    _IOW('t', 21, struct termios32)                        
#define TIOCSETAF_32    _IOW('t', 22, struct termios32)                           
#define TIOCGETA_64     _IOR('t', 19, struct user_termios)
#define TIOCSETA_64     _IOW('t', 20, struct user_termios)
#define TIOCSETAW_64    _IOW('t', 21, struct user_termios)
#define TIOCSETAF_64    _IOW('t', 22, struct user_termios)
#endif              
#define TIOCGETD        _IOR('t', 26, int)                               
#define TIOCSETD        _IOW('t', 27, int)                               
#define TIOCIXON         _IO('t', 129)                                     
#define TIOCIXOFF        _IO('t', 128)                                    
                                                                    
#define TIOCSBRK         _IO('t', 123)                             
#define TIOCCBRK         _IO('t', 122)                               
#define TIOCSDTR         _IO('t', 121)                                       
#define TIOCCDTR         _IO('t', 120)                                         
#define TIOCGPGRP       _IOR('t', 119, int)                          
#define TIOCSPGRP       _IOW('t', 118, int)                          
                                                                    
#define TIOCOUTQ        _IOR('t', 115, int)                            
#define TIOCSTI         _IOW('t', 114, char)                                 
#define TIOCNOTTY        _IO('t', 113)                                    
#define TIOCPKT         _IOW('t', 112, int)                                     
#define         TIOCPKT_DATA            0x00                     
#define         TIOCPKT_FLUSHREAD       0x01                      
#define         TIOCPKT_FLUSHWRITE      0x02                      
#define         TIOCPKT_STOP            0x04                     
#define         TIOCPKT_START           0x08                      
#define         TIOCPKT_NOSTOP          0x10                        
#define         TIOCPKT_DOSTOP          0x20                      
#define         TIOCPKT_IOCTL           0x40                                    
#define TIOCSTOP         _IO('t', 111)                                    
#define TIOCSTART        _IO('t', 110)                                     
#define TIOCMSET        _IOW('t', 109, int)                             
#define TIOCMBIS        _IOW('t', 108, int)                         
#define TIOCMBIC        _IOW('t', 107, int)                         
#define TIOCMGET        _IOR('t', 106, int)                             
#define TIOCREMOTE      _IOW('t', 105, int)                               
#define TIOCGWINSZ      _IOR('t', 104, struct winsize)                       
#define TIOCSWINSZ      _IOW('t', 103, struct winsize)                       
#define TIOCUCNTL       _IOW('t', 102, int)                                     
#define TIOCSTAT         _IO('t', 101)                                          
#define         UIOCCMD(n)      _IO('u', n)                          
#define TIOCSCONS       _IO('t', 99)                                   
#define TIOCCONS        _IOW('t', 98, int)                                  
#define TIOCSCTTY        _IO('t', 97)                                       
#define TIOCEXT         _IOW('t', 96, int)                                    
#define TIOCSIG          _IO('t', 95)                                     
#define TIOCDRAIN        _IO('t', 94)                                         
#define TIOCMSDTRWAIT   _IOW('t', 91, int)                                    
#define TIOCMGDTRWAIT   _IOR('t', 90, int)                                    
#define TIOCTIMESTAMP   _IOR('t', 89, struct timeval)                          
                                                                          
#define TIOCDCDTIMESTAMP _IOR('t', 88, struct timeval)                         
                                                                       
#ifdef KERNEL
#define TIOCTIMESTAMP_32        _IOR('t', 89, struct user32_timeval)
#define TIOCDCDTIMESTAMP_32     _IOR('t', 88, struct user32_timeval)
#define TIOCTIMESTAMP_64        _IOR('t', 89, struct user64_timeval)
#define TIOCDCDTIMESTAMP_64     _IOR('t', 88, struct user64_timeval)
#endif
#define TIOCSDRAINWAIT  _IOW('t', 87, int)                               
#define TIOCGDRAINWAIT  _IOR('t', 86, int)                               
#define TIOCDSIMICROCODE _IO('t', 85)                                   
                                                            
#define TIOCPTYGRANT    _IO('t', 84)                            
#define TIOCPTYGNAME    _IOC(IOC_OUT, 't', 83, 128)                     
#define TIOCPTYUNLK     _IO('t', 82)                             
#ifdef KERNEL
#define TIOCREVOKE       _IO('t', 81)
#define TIOCREVOKECLEAR  _IO('t', 80)
#endif

#define TTYDISC         0                                                
#define TABLDISC        3                                      
#define SLIPDISC        4                                         
#define PPPDISC         5                                   

#endif                      
