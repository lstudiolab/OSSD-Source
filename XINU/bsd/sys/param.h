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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1995, 1997 Apple Computer, Inc. All Rights Reserved */
/*-
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
 *	@(#)param.h	8.3 (Berkeley) 4/4/95
 */

#ifndef _SYS_PARAM_H_
#define _SYS_PARAM_H_

#define BSD     199506                                              
#define BSD4_3  1
#define BSD4_4  1

#define NeXTBSD 1995064                                                     
#define NeXTBSD4_0 0                             

#include <sys/_types.h>
#include <sys/_types/_null.h>

#ifndef LOCORE
#include <sys/types.h>
#endif

  
                                                                        
                                                         
  
                                                        
                                                     
   
#include <sys/syslimits.h>

#define MAXCOMLEN       16                                               
#define MAXINTERP       64                                                    
#define MAXLOGNAME      255                                        
#define MAXUPRC         CHILD_MAX                                       
#define NCARGS          ARG_MAX                                             
#define NGROUPS         NGROUPS_MAX                            
#define NOFILE          256                                                     
#define NOGROUP         65535                                                  
#define MAXHOSTNAMELEN  256                                    
#define MAXDOMNAMELEN   256                                             

                                        
#include <machine/param.h>

                                                            
#ifdef KERNEL
#include <machine/limits.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ucred.h>
#include <sys/uio.h>
#else
#include <limits.h>
#endif

              
#include <sys/signal.h>

  
                                                                         
                 
   
#define PSWP    0
#define PVM     4
#define PINOD   8
#define PRIBIO  16
#define PVFS    20
#define PZERO   22                                                            
#define PSOCK   24
#define PWAIT   32
#define PLOCK   36
#define PPAUSE  40
#define PUSER   50
#define MAXPRI  127                                                          

#define PRIMASK 0x0ff
#define PCATCH  0x100                                                          
#define PTTYBLOCK 0x200                                                   
#define PDROP   0x400                                                                        
#define PSPIN   0x800                                                                        

#define NBPW    sizeof(int)                                             

#define CMASK   022                                                     
#define NODEV   (dev_t)(-1)                              

  
                                                                   
                                                                     
                                                                  
   
#define CLBYTES         (CLSIZE*NBPG)
#define CLOFSET         (CLSIZE*NBPG-1)                                 
#define claligned(x)    ((((int)(x))&CLOFSET)==0)
#define CLOFF           CLOFSET
#define CLSHIFT         (PGSHIFT+CLSIZELOG2)

#if CLSIZE == 1
#define clbase(i)       (i)
#define clrnd(i)        (i)
#else
                                                      
#define clbase(i)       ((i) &~ (CLSIZE-1))
                                                     
#define clrnd(i)        (((i) + (CLSIZE-1)) &~ (CLSIZE-1))
#endif

#define CBLOCK  64                                                           
#define CBQSIZE (CBLOCK/NBBY)                                            
                                                       
#define CBSIZE  (CBLOCK - sizeof(struct cblock *) - CBQSIZE)
#define CROUND  (CBLOCK - 1)                         

  
                                     
  
                                                                       
                                                                     
                                                                          
                                                                        
                                                                    
                                                                
                                                                        
   
#define MAXBSIZE        (256 * 4096)
#define MAXPHYSIO       MAXPHYS
#define MAXFRAG         8

#define MAXPHYSIO_WIRED (16 * 1024 * 1024)

  
                                                                         
                                                                            
                                                                          
                                                                       
                                                                        
                                                                      
                                     
   
#define MAXPATHLEN      PATH_MAX
#define MAXSYMLINKS     32

                             
#define setbit(a, i)     (((unsigned char *)(a))[(i)/NBBY] |= 1u<<((i)%NBBY))
#define clrbit(a, i)     (((unsigned char *)(a))[(i)/NBBY] &= ~(1u<<((i)%NBBY)))
#define isset(a, i)      (((unsigned char *)(a))[(i)/NBBY] & (1u<<((i)%NBBY)))
#define isclr(a, i)      ((((unsigned char *)(a))[(i)/NBBY] & (1u<<((i)%NBBY))) == 0)

                                       
#ifndef howmany
#define howmany(x, y)   ((((x) % (y)) == 0) ? ((x) / (y)) : (((x) / (y)) + 1))
#endif
#define roundup(x, y)   ((((x) % (y)) == 0) ? \
	                (x) : ((x) + ((y) - ((x) % (y)))))
#define powerof2(x)     ((((x)-1)&(x))==0)

                         
#ifndef MIN
#define MIN(a, b) (((a)<(b))?(a):(b))
#endif          
#ifndef MAX
#define MAX(a, b) (((a)>(b))?(a):(b))
#endif           

  
                                                                          
  
                                                                         
                                                                           
                                                                         
                                                           
  
                                                                       
                                                                         
   
#define FSHIFT  11                                                       
#define FSCALE  (1<<FSHIFT)

#endif                     
