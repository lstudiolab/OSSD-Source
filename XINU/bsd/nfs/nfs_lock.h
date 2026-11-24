/*
 * Copyright (c) 2002-2010 Apple Inc.  All rights reserved.
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
/*-
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Berkeley Software Design Inc's name may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN INC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      from nfs_lock.h,v 2.2 1998/04/28 19:38:41 don Exp
 * $FreeBSD$
 */

#include <sys/appleapiopts.h>

#ifdef __APPLE_API_PRIVATE

  
                                                                              
                                                                       
                                                                      
                                                
  
                                                            
   

#include <sys/mount.h>

  
                                                                   
   
#define LOCKD_MSG_VERSION       3
typedef struct nfs_lock_msg {
	int                     lm_version;                                    
	int                     lm_flags;                                  
	u_int64_t               lm_xid;                                                    
	struct flock            lm_fl;                                         
	struct sockaddr_storage lm_addr;                                  
	int                     lm_fh_len;                                           
	struct xucred           lm_cred;                                            
	u_int8_t                lm_fh[NFSV3_MAX_FH_SIZE];                       
} LOCKD_MSG;

              
#define LOCKD_MSG_BLOCK         0x0001                          
#define LOCKD_MSG_TEST          0x0002                        
#define LOCKD_MSG_NFSV3         0x0004                     
#define LOCKD_MSG_CANCEL        0x0008                                  
#define LOCKD_MSG_DENIED_GRACE  0x0010                                       
#define LOCKD_MSG_RECLAIM       0x0020                            
#define LOCKD_MSG_TCP           0x0040                                    

                                                              
typedef struct nfs_lock_msg_request {
	TAILQ_ENTRY(nfs_lock_msg_request) lmr_next;                                         
	int                     lmr_answered;                                    
	int                     lmr_errno;                                 
	int                     lmr_saved_errno;                                    
	LOCKD_MSG               lmr_msg;                                 
} LOCKD_MSG_REQUEST;

TAILQ_HEAD(nfs_lock_msg_queue, nfs_lock_msg_request);
typedef struct nfs_lock_msg_queue LOCKD_MSG_QUEUE;


  
                                                                  
   
#define LOCKD_ANS_VERSION       2
struct lockd_ans {
	int             la_version;                                    
	int             la_errno;                                  
	u_int64_t       la_xid;                                                    
	int             la_flags;                                 
	pid_t           la_pid;                                                  
	off_t           la_start;                                         
	off_t           la_len;                                  
	int             la_fh_len;                                           
	u_int8_t        la_fh[NFSV3_MAX_FH_SIZE];                      
};

              
#define LOCKD_ANS_GRANTED       0x0001                           
#define LOCKD_ANS_LOCK_INFO     0x0002                       
#define LOCKD_ANS_LOCK_EXCL     0x0004                         
#define LOCKD_ANS_DENIED_GRACE  0x0008                                       


  
                                                             
   
#define LOCKD_NOTIFY_VERSION    1
struct lockd_notify {
	int                     ln_version;                                       
	int                     ln_flags;                                 
	int                     ln_pad;                                      
	int                     ln_addrcount;                              
	struct sockaddr_storage ln_addr[1];                                     
};


#ifdef KERNEL
void    nfs_lockd_mount_register(struct nfsmount *);
void    nfs_lockd_mount_unregister(struct nfsmount *);
int     nfs3_lockd_request(nfsnode_t, int, LOCKD_MSG_REQUEST *, int, thread_t);
int     nfslockdans(proc_t p, struct lockd_ans *ansp);
int     nfslockdnotify(proc_t p, user_addr_t argp);

#endif
#endif                          
