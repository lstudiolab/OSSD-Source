/*
 * Copyright (c) 2000-2010 Apple Inc. All rights reserved.
 */
/*-
 * Copyright (c) 1990, 1993
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
 *	@(#)param.h	8.1 (Berkeley) 4/4/95
 */

  
                                      
   

#ifndef _ARM_PARAM_H_
#define _ARM_PARAM_H_

#include <arm/_param.h>

  
                                                                          
                                                                        
                                    
   
#define ALIGNBYTES      __DARWIN_ALIGNBYTES
#define ALIGN(p)        __DARWIN_ALIGN(p)

#define NBPG            4096                            
#define PGOFSET         (NBPG-1)                                   
#define PGSHIFT         12                              

#define DEV_BSIZE       512
#define DEV_BSHIFT      9                                    
#define BLKDEV_IOSIZE   2048
#define MAXPHYS         (64 * 1024)                                    

#define CLSIZE          1
#define CLSIZELOG2      0

  
                                                  
                                                                         
                                                                       
                                                                      
                             
   
#define MSIZESHIFT      8                                
#define MSIZE           (1 << MSIZESHIFT)                            
#define MCLSHIFT        11                                
#define MCLBYTES        (1 << MCLSHIFT)                                      
#define MBIGCLSHIFT     12                                
#define MBIGCLBYTES     (1 << MBIGCLSHIFT)                                 
#define M16KCLSHIFT     14                                 
#define M16KCLBYTES     (1 << M16KCLSHIFT)                                   

#define MCLOFSET        (MCLBYTES - 1)
#ifndef NMBCLUSTERS
#define NMBCLUSTERS     CONFIG_NMBCLUSTERS                       
#endif

  
                                   
   
                                                                   
#define ctos(x) (x)
#define stoc(x) (x)

                                             
#define ctod(x) ((x)<<(PGSHIFT-DEV_BSHIFT))
#define dtoc(x) ((x)>>(PGSHIFT-DEV_BSHIFT))
#define dtob(x) ((x)<<DEV_BSHIFT)

                     
#define ctob(x) ((x)<<PGSHIFT)

                     
#define btoc(x) (((unsigned)(x)+(NBPG-1))>>PGSHIFT)

#ifdef __APPLE__
#define  btodb(bytes, devBlockSize)         \
	((unsigned)(bytes) / devBlockSize)
#define  dbtob(db, devBlockSize)            \
	((unsigned)(db) * devBlockSize)
#else
#define btodb(bytes)                                                         \
	((unsigned)(bytes) >> DEV_BSHIFT)
#define dbtob(db)                                                         \
	((unsigned)(db) << DEV_BSHIFT)
#endif

  
                                                       
                                                     
                                                  
                                     
   
#define bdbtofsb(bn)    ((bn) / (BLKDEV_IOSIZE/DEV_BSIZE))

  
                                                       
   
#define STATUS_WORD(rpl, ipl)   (((ipl) << 8) | (rpl))
#define USERMODE(x)             (((x) & 3) == 3)
#define BASEPRI(x)              (((x) & (255 << 8)) == 0)


#if     defined(KERNEL) || defined(STANDALONE)
#define DELAY(n) delay(n)

#else                                               
#define DELAY(n)        { int N = (n); while (--N > 0); }
#endif                                              

#endif                    
