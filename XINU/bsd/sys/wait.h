/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1989, 1993, 1994
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
 *	@(#)wait.h	8.2 (Berkeley) 7/10/94
 */

#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

  
                                                                
                                                                   
   

  
                                                                        
                                                                   
   
typedef enum {
	P_ALL,
	P_PID,
	P_PGID
} idtype_t;

  
                                                               
                   
   
#include <sys/_types/_pid_t.h>
#include <sys/_types/_id_t.h>

  
                                                                       
                                                                               
                                                                       
                                               
  
                                                                    
                                                            
  
                                                                
                                                             
                                                           
                                                               
                                                                
                                                           
                         
   
#include <sys/signal.h>                                  
#include <sys/resource.h>                                    

  
                                                                   
                                                                           
                                                                  
                                                                          
                                                                         
                                                                          
                          
   
#define WNOHANG         0x00000001                                              
#define WUNTRACED       0x00000002                                            

  
                                                  
                                   
   
#if defined(_POSIX_C_SOURCE) && !defined(_DARWIN_C_SOURCE)
#define _W_INT(i)       (i)
#else
#define _W_INT(w)       (*(int *)&(w))                                 
#define WCOREFLAG       0200
#endif                                             

                                                                            
#define _WSTATUS(x)     (_W_INT(x) & 0177)
#define _WSTOPPED       0177                                                

  
                                                                      
                                    
   
#if __DARWIN_UNIX03
#define WEXITSTATUS(x)  ((_W_INT(x) >> 8) & 0x000000ff)
#else                       
#define WEXITSTATUS(x)  (_W_INT(x) >> 8)
#endif                       
                     
#define WSTOPSIG(x)     (_W_INT(x) >> 8)
#define WIFCONTINUED(x) (_WSTATUS(x) == _WSTOPPED && WSTOPSIG(x) == 0x13)
#define WIFSTOPPED(x)   (_WSTATUS(x) == _WSTOPPED && WSTOPSIG(x) != 0x13)
#define WIFEXITED(x)    (_WSTATUS(x) == 0)
#define WIFSIGNALED(x)  (_WSTATUS(x) != _WSTOPPED && _WSTATUS(x) != 0)
#define WTERMSIG(x)     (_WSTATUS(x))
#if (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
#define WCOREDUMP(x)    (_W_INT(x) & WCOREFLAG)

#define W_EXITCODE(ret, sig)    ((ret) << 8 | (sig))
#define W_STOPCODE(sig)         ((sig) << 8 | _WSTOPPED)
#endif                                                               

  
                                                                      
                                              
   
                                         
                                                        
#define WEXITED         0x00000004                                          
#if __DARWIN_UNIX03
                        
#define WSTOPPED        0x00000008                                         
#endif
#define WCONTINUED      0x00000010                                              
#define WNOWAIT         0x00000020                                             


#if (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
                                                 

  
                                                             
   
#define WAIT_ANY        (-1)                     
#define WAIT_MYPGRP     0                                            

#include <machine/endian.h>

  
              
                                                                     
                                                               
                                            
   
union wait {
	int     w_status;                                    
	  
                              
    
	struct {
#if __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN
		unsigned int    w_Termsig:7,                            
		    w_Coredump:1,                                        
		    w_Retcode:8,                                               
		    w_Filler:16;                                       
#endif
#if __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN
		unsigned int    w_Filler:16,                           
		    w_Retcode:8,                                               
		    w_Coredump:1,                                        
		    w_Termsig:7;                                        
#endif
	} w_T;
	  
                                     
                                             
                                  
    
	struct {
#if __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN
		unsigned int    w_Stopval:8,                                 
		    w_Stopsig:8,                                            
		    w_Filler:16;                                       
#endif
#if __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN
		unsigned int    w_Filler:16,                           
		    w_Stopsig:8,                                            
		    w_Stopval:8;                                             
#endif
	} w_S;
};
#define w_termsig       w_T.w_Termsig
#define w_coredump      w_T.w_Coredump
#define w_retcode       w_T.w_Retcode
#define w_stopval       w_S.w_Stopval
#define w_stopsig       w_S.w_Stopsig

#endif                                                               

#if !(__DARWIN_UNIX03 - 0)
  
                                                                      
                    
   
#define WSTOPPED        _WSTOPPED
#endif                       

#ifndef KERNEL
__BEGIN_DECLS
pid_t   wait(int *) __DARWIN_ALIAS_C(wait);
pid_t   waitpid(pid_t, int *, int) __DARWIN_ALIAS_C(waitpid);
#ifndef _ANSI_SOURCE
int     waitid(idtype_t, id_t, siginfo_t *, int) __DARWIN_ALIAS_C(waitid);
#endif                    
#if  (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
pid_t   wait3(int *, int, struct rusage *);
pid_t   wait4(pid_t, int *, int, struct rusage *);
#endif                                                               
__END_DECLS
#endif
#endif                    
