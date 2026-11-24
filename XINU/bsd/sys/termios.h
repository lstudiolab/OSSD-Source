/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
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
/* Copyright (c) 1997 Apple Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1988, 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)termios.h	8.3 (Berkeley) 3/28/94
 */

#ifndef _SYS_TERMIOS_H_
#define _SYS_TERMIOS_H_

#include <sys/cdefs.h>

  
                             
  
                                     
  
                                 
   
#define VEOF            0                   
#define VEOL            1                   
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define VEOL2           2                                        
#endif
#define VERASE          3                   
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define VWERASE         4                                        
#endif
#define VKILL           5                   
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define VREPRINT        6                                        
#endif
                    
#define VINTR           8                 
#define VQUIT           9                 
#define VSUSP           10                
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define VDSUSP          11                                     
#endif
#define VSTART          12                       
#define VSTOP           13                       
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define VLNEXT          14                  
#define VDISCARD        15                  
#endif
#define VMIN            16                   
#define VTIME           17                   
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define VSTATUS         18                                       
                     
#endif
#define NCCS            20

#include <sys/_types/_posix_vdisable.h>

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define CCEQ(val, c)    ((c) == (val) ? (val) != _POSIX_VDISABLE : 0)
#endif

  
                                          
   
#define IGNBRK          0x00000001                                  
#define BRKINT          0x00000002                                
#define IGNPAR          0x00000004                                          
#define PARMRK          0x00000008                                          
#define INPCK           0x00000010                                            
#define ISTRIP          0x00000020                                   
#define INLCR           0x00000040                          
#define IGNCR           0x00000080                     
#define ICRNL           0x00000100                                    
#define IXON            0x00000200                                      
#define IXOFF           0x00000400                                     
#define IXANY           0x00000800                                            
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IMAXBEL         0x00002000                                         
#define IUTF8           0x00004000                                           
#endif                                             

  
                                            
   
#define OPOST           0x00000001                                              
#define ONLCR           0x00000002                                       
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define OXTABS          0x00000004                                 
#define ONOEOT          0x00000008                                         
#endif                                             
  
                                                                           
                                                          
  
                                 
   
#define OCRNL           0x00000010                                  
#define ONOCR           0x00000020                                    
#define ONLRET          0x00000040                                   
#define OFILL           0x00000080                                         
#define NLDLY           0x00000300                    
#define TABDLY          0x00000c04                                
#define CRDLY           0x00003000                    
#define FFDLY           0x00004000                           
#define BSDLY           0x00008000                    
#define VTDLY           0x00010000                              
#define OFDEL           0x00020000                                 
#if !defined(_SYS_IOCTL_COMPAT_H_) || __DARWIN_UNIX03
  
                                                                      
                                                                          
                                                                       
                                                                      
                                                                        
                                                                        
                                                                         
                                       
   
#define         NL0     0x00000000
#define         NL1     0x00000100
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define         NL2     0x00000200
#define         NL3     0x00000300
#endif                                             
#define         TAB0    0x00000000
#define         TAB1    0x00000400
#define         TAB2    0x00000800
                                                 
#define         TAB3    0x00000004
#define         CR0     0x00000000
#define         CR1     0x00001000
#define         CR2     0x00002000
#define         CR3     0x00003000
#define         FF0     0x00000000
#define         FF1     0x00004000
#define         BS0     0x00000000
#define         BS1     0x00008000
#define         VT0     0x00000000
#define         VT1     0x00010000
#endif                             
  
                               
   

  
                                               
   
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define CIGNORE         0x00000001                                
#endif
#define CSIZE           0x00000300                               
#define     CS5             0x00000000                           
#define     CS6             0x00000100                  
#define     CS7             0x00000200                  
#define     CS8             0x00000300                  
#define CSTOPB          0x00000400                            
#define CREAD           0x00000800                           
#define PARENB          0x00001000                         
#define PARODD          0x00002000                                 
#define HUPCL           0x00004000                                 
#define CLOCAL          0x00008000                                     
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define CCTS_OFLOW      0x00010000                                      
#define CRTSCTS         (CCTS_OFLOW | CRTS_IFLOW)
#define CRTS_IFLOW      0x00020000                                     
#define CDTR_IFLOW      0x00040000                                     
#define CDSR_OFLOW      0x00080000                                      
#define CCAR_OFLOW      0x00100000                                      
#define MDMBUF          0x00100000                                   
#endif


  
                                                 
  
                                                   
                                                  
              
   

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define ECHOKE          0x00000001                                      
#endif                                             
#define ECHOE           0x00000002                                
#define ECHOK           0x00000004                                   
#define ECHO            0x00000008                          
#define ECHONL          0x00000010                                       
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define ECHOPRT         0x00000020                                          
#define ECHOCTL         0x00000040                                         
#endif                                             
#define ISIG            0x00000080                                              
#define ICANON          0x00000100                                    
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define ALTWERASE       0x00000200                                          
#endif                                             
#define IEXTEN          0x00000400                                    
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define EXTPROC         0x00000800                               
#endif                                             
#define TOSTOP          0x00400000                                            
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define FLUSHO          0x00800000                                        
#define NOKERNINFO      0x02000000                                         
#define PENDIN          0x20000000                                            
#endif                                             
#define NOFLSH          0x80000000                                       

typedef unsigned long   tcflag_t;
typedef unsigned char   cc_t;
typedef unsigned long   speed_t;

struct termios {
	tcflag_t        c_iflag;                         
	tcflag_t        c_oflag;                          
	tcflag_t        c_cflag;                           
	tcflag_t        c_lflag;                         
	cc_t            c_cc[NCCS];                        
	speed_t         c_ispeed;                        
	speed_t         c_ospeed;                         
};

#ifdef KERNEL
typedef __uint64_t      user_tcflag_t;
typedef __uint64_t      user_speed_t;

  
                                                                          
                                                 
                                             
   

struct user_termios {
	user_tcflag_t   c_iflag;                         
	user_tcflag_t   c_oflag;                          
	user_tcflag_t   c_cflag;                           
	user_tcflag_t   c_lflag;                         
	cc_t            c_cc[NCCS];                        
	user_speed_t    c_ispeed __attribute((aligned(8)));                      
	user_speed_t    c_ospeed;                         
};

                    
struct termios32 {
	__uint32_t      c_iflag;                         
	__uint32_t      c_oflag;                          
	__uint32_t      c_cflag;                           
	__uint32_t      c_lflag;                         
	cc_t            c_cc[NCCS];                        
	__uint32_t      c_ispeed;                        
	__uint32_t      c_ospeed;                         
};

#endif              

  
                                                                    
   
#define TCSANOW         0                                          
#define TCSADRAIN       1                                              
#define TCSAFLUSH       2                                              
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define TCSASOFT        0x10                                               
#endif

  
                  
   
#define B0      0
#define B50     50
#define B75     75
#define B110    110
#define B134    134
#define B150    150
#define B200    200
#define B300    300
#define B600    600
#define B1200   1200
#define B1800   1800
#define B2400   2400
#define B4800   4800
#define B9600   9600
#define B19200  19200
#define B38400  38400
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define B7200   7200
#define B14400  14400
#define B28800  28800
#define B57600  57600
#define B76800  76800
#define B115200 115200
#define B230400 230400
#define EXTA    19200
#define EXTB    38400
#endif                                              

#ifndef KERNEL

#define TCIFLUSH        1
#define TCOFLUSH        2
#define TCIOFLUSH       3
#define TCOOFF          1
#define TCOON           2
#define TCIOFF          3
#define TCION           4

#include <sys/cdefs.h>

__BEGIN_DECLS
speed_t cfgetispeed(const struct termios *);
speed_t cfgetospeed(const struct termios *);
int     cfsetispeed(struct termios *, speed_t);
int     cfsetospeed(struct termios *, speed_t);
int     tcgetattr(int, struct termios *);
int     tcsetattr(int, int, const struct termios *);
int     tcdrain(int) __DARWIN_ALIAS_C(tcdrain);
int     tcflow(int, int);
int     tcflush(int, int);
int     tcsendbreak(int, int);

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
void    cfmakeraw(struct termios *);
int     cfsetspeed(struct termios *, speed_t);
#endif                                             
__END_DECLS

#endif              

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)

  
                                                                   
                                                                    
                    
   
#include <sys/ttycom.h>
#endif

  
                            
   
#endif                       

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#include <sys/ttydefaults.h>
#endif
