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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	@(#)ipc.h	8.4 (Berkeley) 2/19/95
 */

  
                             
   
#ifndef _SYS_IPC_H_
#define _SYS_IPC_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>

#include <sys/_types.h>

  
                                                                      
                              
   
#include <sys/_types/_uid_t.h>
#include <sys/_types/_gid_t.h>
#include <sys/_types/_mode_t.h>
#include <sys/_types/_key_t.h>


#pragma pack(4)

  
                                                                        
                                                                        
                                                                       
                                                                       
   
#if __DARWIN_UNIX03 || defined(KERNEL)
  
                                                                     
            
   
struct ipc_perm {
	uid_t           uid;                                       
	gid_t           gid;                                        
	uid_t           cuid;                                        
	gid_t           cgid;                                         
	mode_t          mode;                                            
	unsigned short  _seq;                                          
	key_t           _key;                                          
};
#define __ipc_perm_new  ipc_perm
#else                         
#define ipc_perm        __ipc_perm_old
#endif                        

#if !__DARWIN_UNIX03
  
                                                                     
                                                                   
                                                                   
   
struct __ipc_perm_old {
	__uint16_t      cuid;                                  
	__uint16_t      cgid;                                   
	__uint16_t      uid;                                 
	__uint16_t      gid;                                  
	mode_t          mode;                                      
	__uint16_t      seq;                                           
	key_t           key;                                           
};
#endif                        

#pragma pack()

  
                                                                   
   

               
#define IPC_CREAT       001000                                                  
#define IPC_EXCL        002000                                  
#define IPC_NOWAIT      004000                                          

          
#define IPC_PRIVATE     ((key_t)0)                       

                      
#define IPC_RMID        0                                      
#define IPC_SET         1                                
#define IPC_STAT        2                                


#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)

                      
#define IPC_R           000400                               
#define IPC_W           000200                                      
#define IPC_M           010000                                              

#endif                                              


#ifdef BSD_KERNEL_PRIVATE
  
                                                                     
                  
   

                                                                         
#define IPCID_TO_IX(id)         ((id) & 0xffff)
#define IPCID_TO_SEQ(id)        (((id) >> 16) & 0xffff)
#define IXSEQ_TO_IPCID(ix, perm) (((perm._seq) << 16L) | ((ix) & 0xffff))

struct ucred;

int     ipcperm(struct ucred *, struct ipc_perm *, int);
#endif                         

#ifndef KERNEL

__BEGIN_DECLS
           
key_t   ftok(const char *, int);
__END_DECLS

#endif               

#endif                   
