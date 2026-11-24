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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
                                                         

/*
 * Copyright (c) 1994 Adam Glass
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
 *      This product includes software developed by Adam Glass.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
  
                                                                      
                                                                          
                                                                        
               
   

  
                                                                 
                                          
   

#ifndef _SYS_SHM_INTERNALH_
#define _SYS_SHM_INTERNALH_

#include <sys/shm.h>
#include <sys/cdefs.h>

#include <machine/types.h>

#pragma pack(4)

struct user_shmid_ds {
	struct ipc_perm shm_perm;                                           
	user_size_t     shm_segsz;                                    
	pid_t           shm_lpid;                                         
	pid_t           shm_cpid;                           
	short           shm_nattch;                                     
	user_time_t     shm_atime;                                
	user_time_t     shm_dtime;                                
	user_time_t     shm_ctime;                                           
	user_addr_t     shm_internal;                                
};

struct user32_shmid_ds {
	struct ipc_perm shm_perm;                                           
	uint32_t        shm_segsz;                                    
	pid_t           shm_lpid;                                         
	pid_t           shm_cpid;                           
	short           shm_nattch;                                     
	uint32_t                shm_atime;                                
	uint32_t                shm_dtime;                                
	uint32_t                shm_ctime;                                           
	user32_addr_t   shm_internal;                                
};

#pragma pack()

  
                                                                      
                                                                           
                                                                        
                                                     
                                                              
                                      
   
struct shminfo {
	int64_t shmmax;                                           
	int64_t shmmin;                                           
	int64_t shmmni;                                            
	int64_t shmseg;                                           
	int64_t shmall;                                        
};

#ifdef KERNEL

struct label;

  
                                                                             
                                                                      
   
struct shmid_kernel {
	struct user_shmid_ds u;
	struct label *label;                   
};

extern struct shminfo shminfo;
extern struct shmid_kernel *shmsegs;

struct proc;

__BEGIN_DECLS

void    shmexit(struct proc *);
int     shmfork(struct proc *, struct proc *);
__private_extern__ void shmexec(struct proc *);

__END_DECLS

#endif             

#endif                           
