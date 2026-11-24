/*
 * Copyright (c) 2000-2012 Apple, Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)errno.h	8.5 (Berkeley) 1/21/94
 */

#ifndef _SYS_ERRNO_H_
#define _SYS_ERRNO_H_

#include <sys/cdefs.h>

#if !defined(KERNEL) && !defined(KERNEL_PRIVATE)

#if defined(__STDC_WANT_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__ >= 1
#include <sys/_types/_errno_t.h>
#endif

__BEGIN_DECLS
extern int * __error(void);
#define errno (*__error())
__END_DECLS
#endif

  
              
   

#define EPERM           1                                            
#define ENOENT          2                                              
#define ESRCH           3                                    
#define EINTR           4                                            
#define EIO             5                                       
#define ENXIO           6                                          
#define E2BIG           7                                           
#define ENOEXEC         8                                      
#define EBADF           9                                        
#define ECHILD          10                                      
#define EDEADLK         11                                             
                                                           
#define ENOMEM          12                                          
#define EACCES          13                                     
#define EFAULT          14                               
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define ENOTBLK         15                                         
#endif
#define EBUSY           16                                          
#define EEXIST          17                               
#define EXDEV           18                                     
#define ENODEV          19                                                     
#define ENOTDIR         20                                   
#define EISDIR          21                                  
#define EINVAL          22                                    
#define ENFILE          23                                                 
#define EMFILE          24                                       
#define ENOTTY          25                                                  
#define ETXTBSY         26                                  
#define EFBIG           27                                  
#define ENOSPC          28                                           
#define ESPIPE          29                                
#define EROFS           30                                         
#define EMLINK          31                                  
#define EPIPE           32                               

                   
#define EDOM            33                                                    
#define ERANGE          34                                    

                                    
#define EAGAIN          35                                                    
#define EWOULDBLOCK     EAGAIN                                     
#define EINPROGRESS     36                                             
#define EALREADY        37                                                 

                                             
#define ENOTSOCK        38                                                  
#define EDESTADDRREQ    39                                                
#define EMSGSIZE        40                                    
#define EPROTOTYPE      41                                                  
#define ENOPROTOOPT     42                                          
#define EPROTONOSUPPORT 43                                          
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define ESOCKTNOSUPPORT 44                                             
#endif
#define ENOTSUP         45                                           
#if !__DARWIN_UNIX03 && !defined(KERNEL)
  
                                                                          
                                                                        
                                                                      
                                                                           
                                                                          
                                                           
   
#define EOPNOTSUPP       ENOTSUP                                               
#endif                                  

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define EPFNOSUPPORT    46                                                 
#endif
#define EAFNOSUPPORT    47                                                                   
#define EADDRINUSE      48                                          
#define EADDRNOTAVAIL   49                                                  

                                                
#define ENETDOWN        50                                   
#define ENETUNREACH     51                                          
#define ENETRESET       52                                                       
#define ECONNABORTED    53                                                    
#define ECONNRESET      54                                            
#define ENOBUFS         55                                             
#define EISCONN         56                                               
#define ENOTCONN        57                                           
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define ESHUTDOWN       58                                                    
#define ETOOMANYREFS    59                                                     
#endif
#define ETIMEDOUT       60                                       
#define ECONNREFUSED    61                                      

#define ELOOP           62                                                     
#define ENAMETOOLONG    63                                      

                          
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define EHOSTDOWN       64                                
#endif
#define EHOSTUNREACH    65                                    
#define ENOTEMPTY       66                                       

                   
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define EPROCLIM        67                                      
#define EUSERS          68                                  
#endif
#define EDQUOT          69                                       

                         
#define ESTALE          70                                         
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define EREMOTE         71                                                     
#define EBADRPC         72                                     
#define ERPCMISMATCH    73                                     
#define EPROGUNAVAIL    74                                       
#define EPROGMISMATCH   75                                         
#define EPROCUNAVAIL    76                                             
#endif

#define ENOLCK          77                                      
#define ENOSYS          78                                            

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define EFTYPE          79                                                     
#define EAUTH           80                                        
#define ENEEDAUTH       81                                      

                               
#define EPWROFF         82                               
#define EDEVERR         83                                        
#endif

#define EOVERFLOW       84                                                             

                            
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define EBADEXEC        85                          
#define EBADARCH        86                                      
#define ESHLIBVERS      87                                           
#define EBADMACHO       88                                
#endif

#define ECANCELED       89                                      

#define EIDRM           90                                      
#define ENOMSG          91                                              
#define EILSEQ          92                                         
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define ENOATTR         93                                       
#endif

#define EBADMSG         94                               
#define EMULTIHOP       95                            
#define ENODATA         96                                                  
#define ENOLINK         97                            
#define ENOSR           98                                       
#define ENOSTR          99                                
#define EPROTO          100                                 
#define ETIME           101                                       

#if __DARWIN_UNIX03 || defined(KERNEL)
                                                                           
#define EOPNOTSUPP      102                                                    
#endif                                

#define ENOPOLICY       103                                            

#if __DARWIN_C_LEVEL >= 200809L
#define ENOTRECOVERABLE 104                                        
#define EOWNERDEAD      105                                      
#endif

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define EQFULL          106                                                 
#define ELAST           106                                              
#endif

#ifdef KERNEL
                                                                      
#define ERESTART        (-1)                                 
#define EJUSTRETURN     (-2)                                                

#ifdef KERNEL_PRIVATE
#define ERECYCLE    (-5)                                                                         
#endif
#ifdef BSD_KERNEL_PRIVATE
#define EREDRIVEOPEN    (-6)
#define EKEEPLOOKING    (-7)
#define EDATALESS       (-8)
                                           
#define ECVCERORR       256
#define ECVPERORR       512
#else                         
                                                                 
#endif                         
#ifdef PRIVATE
#define EQSUSPENDED     (-EQFULL)                                      
#endif              
#endif             
#endif                    
