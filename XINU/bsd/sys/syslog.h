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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1982, 1986, 1988, 1993
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
 *	@(#)syslog.h	8.1 (Berkeley) 6/2/93
 * $FreeBSD: src/sys/sys/syslog.h,v 1.27.2.1.4.1 2010/06/14 02:09:06 kensmith Exp $
 */

#ifndef _SYS_SYSLOG_H_
#define _SYS_SYSLOG_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>

#define _PATH_LOG       "/var/run/syslog"

  
                                                                             
                                                                            
                                                                      
                                                                        
                         
  
                                 
   
#define LOG_EMERG       0                               
#define LOG_ALERT       1                                             
#define LOG_CRIT        2                                
#define LOG_ERR         3                             
#define LOG_WARNING     4                               
#define LOG_NOTICE      5                                             
#define LOG_INFO        6                          
#define LOG_DEBUG       7                                 

#define LOG_PRIMASK     0x07                                                  
                      
#define LOG_PRI(p)      ((p) & LOG_PRIMASK)
#define LOG_MAKEPRI(fac, pri)   ((fac) | (pri))

#ifdef SYSLOG_NAMES
#define INTERNAL_NOPRI  0x10                                    
                     
#define INTERNAL_MARK   LOG_MAKEPRI((LOG_NFACILITIES<<3), 0)
typedef struct _code {
	const char      *c_name;
	int             c_val;
} CODE;

CODE prioritynames[] = {
	{ "alert", LOG_ALERT, },
	{ "crit", LOG_CRIT, },
	{ "debug", LOG_DEBUG, },
	{ "emerg", LOG_EMERG, },
	{ "err", LOG_ERR, },
	{ "error", LOG_ERR, },                                  
	{ "info", LOG_INFO, },
	{ "none", INTERNAL_NOPRI, },                          
	{ "notice", LOG_NOTICE, },
	{ "panic", LOG_EMERG, },                                
	{ "warn", LOG_WARNING, },                               
	{ "warning", LOG_WARNING, },
	{ NULL, -1, }
};
#endif

                    
#define LOG_KERN        (0<<3)                       
#define LOG_USER        (1<<3)                                  
#define LOG_MAIL        (2<<3)                   
#define LOG_DAEMON      (3<<3)                      
#define LOG_AUTH        (4<<3)                              
#define LOG_SYSLOG      (5<<3)                                                
#define LOG_LPR         (6<<3)                              
#define LOG_NEWS        (7<<3)                              
#define LOG_UUCP        (8<<3)                      
#define LOG_CRON        (9<<3)                    
#define LOG_AUTHPRIV    (10<<3)                                       
                                             
                                             
                                             
#define LOG_FTP         (11<<3)                 
                                              
                                                                            
                                                       
#define LOG_NETINFO     (12<<3)              
#define LOG_REMOTEAUTH  (13<<3)                                          
#define LOG_INSTALL     (14<<3)                          
#define LOG_RAS         (15<<3)                                        

                                                    
#define LOG_LOCAL0      (16<<3)                             
#define LOG_LOCAL1      (17<<3)                             
#define LOG_LOCAL2      (18<<3)                             
#define LOG_LOCAL3      (19<<3)                             
#define LOG_LOCAL4      (20<<3)                             
#define LOG_LOCAL5      (21<<3)                             
#define LOG_LOCAL6      (22<<3)                             
#define LOG_LOCAL7      (23<<3)                             

#define LOG_LAUNCHD     (24<<3)                                         

#define LOG_NFACILITIES 25                                        
#define LOG_FACMASK     0x03f8                                     
                     
#define LOG_FAC(p)      (((p) & LOG_FACMASK) >> 3)

#ifdef SYSLOG_NAMES
CODE facilitynames[] = {
	{ "auth", LOG_AUTH, },
	{ "authpriv", LOG_AUTHPRIV, },
	{ "cron", LOG_CRON, },
	{ "daemon", LOG_DAEMON, },
	{ "ftp", LOG_FTP, },
	{ "install", LOG_INSTALL     },
	{ "kern", LOG_KERN, },
	{ "lpr", LOG_LPR, },
	{ "mail", LOG_MAIL, },
	{ "mark", INTERNAL_MARK, },                           
	{ "netinfo", LOG_NETINFO, },
	{ "ras", LOG_RAS         },
	{ "remoteauth", LOG_REMOTEAUTH  },
	{ "news", LOG_NEWS, },
	{ "security", LOG_AUTH        },                        
	{ "syslog", LOG_SYSLOG, },
	{ "user", LOG_USER, },
	{ "uucp", LOG_UUCP, },
	{ "local0", LOG_LOCAL0, },
	{ "local1", LOG_LOCAL1, },
	{ "local2", LOG_LOCAL2, },
	{ "local3", LOG_LOCAL3, },
	{ "local4", LOG_LOCAL4, },
	{ "local5", LOG_LOCAL5, },
	{ "local6", LOG_LOCAL6, },
	{ "local7", LOG_LOCAL7, },
	{ "launchd", LOG_LAUNCHD     },
	{ NULL, -1, }
};
#endif

#ifdef KERNEL
#ifdef __APPLE_API_PRIVATE
#define LOG_PRINTF      -1                                                     
#endif                          
#endif

  
                           
   
#define LOG_MASK(pri)   (1 << (pri))                                       
#define LOG_UPTO(pri)   ((1 << ((pri)+1)) - 1)                                  

  
                            
  
                                      
                                                   
   
#define LOG_PID         0x01                                       
#define LOG_CONS        0x02                                                 
#define LOG_ODELAY      0x04                                                   
#define LOG_NDELAY      0x08                          
#define LOG_NOWAIT      0x10                                                  
#define LOG_PERROR      0x20                               

#ifndef KERNEL

  
                                                                              
                                                                            
                                                                              
                                                                              
                                            
   
#include <sys/_types.h>

__BEGIN_DECLS
void    closelog(void);
void    openlog(const char *, int, int);
int     setlogmask(int);
#if defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && __DARWIN_C_LEVEL >= __DARWIN_C_FULL
void    syslog(int, const char *, ...) __DARWIN_ALIAS_STARTING(__MAC_10_13, __IPHONE_NA, __DARWIN_EXTSN(syslog)) __printflike(2, 3) __not_tail_called;
#else
void    syslog(int, const char *, ...) __printflike(2, 3) __not_tail_called;
#endif
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
void    vsyslog(int, const char *, __darwin_va_list) __printflike(2, 0) __not_tail_called;
#endif
__END_DECLS

#else              
#ifdef __APPLE_API_OBSOLETE
  
                                                      
   

  
                                   
                              
  
                                                              
                                                                           
                                                                       
                                               
                                                               
                                                                   
  
                                
                                                                 
                                    
  
                                                           
                                                  
                                                   
                                           
  
                                                             
                                                
                                                   
                                             
                            
  
                                                            
                                   
  
                                                                  
                                                  
                                             
                                                 
                                      
  
                                     
                                  
  
                                                                     
                                                                        
                                                                         
                                                                     
                                                                       
                                                                      
                    
  
                     
           
  
                                                       
                                                                   
                                    
   

  
                  
                                                 
   
struct reg_values {
	unsigned rv_value;
	char *rv_name;
};

  
                                                                        
                                                                  
   
struct reg_desc {
	unsigned rd_mask;                                  
	int rd_shift;                                                      
	char *rd_name;                          
	char *rd_format;                                   
	struct reg_values *rd_values;                                 
};

#endif                           

#include <stdarg.h>
__BEGIN_DECLS
void    log(int, const char *, ...) __printflike(2, 3);
#ifdef XNU_KERNEL_PRIVATE
int     vaddlog(const char *, va_list) __printflike(1, 0);
void    logtime(time_t);
#endif                         

__END_DECLS

#endif              
#endif                      
