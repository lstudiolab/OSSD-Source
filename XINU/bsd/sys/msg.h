/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
 * SVID compatible msg.h file
 *
 * Author:  Daniel Boulet
 *
 * Copyright 1993 Daniel Boulet and RTMX Inc.
 *
 * This system call was implemented by Daniel Boulet under contract from RTMX.
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 * Obviously, it would be nice if you gave credit where credit is due
 * but requiring it would be too onerous.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef _SYS_MSG_H_
#define _SYS_MSG_H_

#include <sys/appleapiopts.h>

#include <sys/_types.h>
#include <sys/cdefs.h>

  
                                                                       
                     
   
#include <sys/ipc.h>


  
                                                                     
                                         
  
                                                              
                            
   
#include <sys/_types/_pid_t.h>
#include <sys/_types/_time_t.h>
#include <sys/_types/_size_t.h>
#include <sys/_types/_ssize_t.h>

                                                                
typedef unsigned long           msgqnum_t;

                                                                   
typedef unsigned long           msglen_t;

  
                                                                          
                                       
   
#define MSG_NOERROR     010000                                             


  
                                                                        
                                                                        
                                                                       
                                                                       
   
#if __DARWIN_UNIX03 || defined(KERNEL)
#pragma pack(4)
  
                             
  
                                                                       
                                                                        
                                                                       
                                                                        
                                                                     
                                
  
                                                                    
   
#if (defined(_POSIX_C_SOURCE) && !defined(_DARWIN_C_SOURCE))
struct msqid_ds
#else
#define msqid_ds        __msqid_ds_new
struct __msqid_ds_new
#endif
{
	struct __ipc_perm_new   msg_perm;                                  
	__int32_t       msg_first;                                     
	__int32_t       msg_last;                                      
	msglen_t        msg_cbytes;                                  
	msgqnum_t       msg_qnum;                                              
	msglen_t        msg_qbytes;                                       
	pid_t           msg_lspid;                                      
	pid_t           msg_lrpid;                                      
	time_t          msg_stime;                                       
	__int32_t       msg_pad1;                                 
	time_t          msg_rtime;                                       
	__int32_t       msg_pad2;                                 
	time_t          msg_ctime;                                       
	__int32_t       msg_pad3;                                 
	__int32_t       msg_pad4[4];                              
};
#pragma pack()
#else                         
#define msqid_ds        __msqid_ds_old
#endif                        

#if !__DARWIN_UNIX03
struct __msqid_ds_old {
	struct __ipc_perm_old   msg_perm;                                  
	__int32_t       msg_first;                                     
	__int32_t       msg_last;                                      
	msglen_t        msg_cbytes;                                  
	msgqnum_t       msg_qnum;                                              
	msglen_t        msg_qbytes;                                       
	pid_t           msg_lspid;                                      
	pid_t           msg_lrpid;                                      
	time_t          msg_stime;                                       
	__int32_t       msg_pad1;                                 
	time_t          msg_rtime;                                       
	__int32_t       msg_pad2;                                 
	time_t          msg_ctime;                                       
	__int32_t       msg_pad3;                                 
	__int32_t       msg_pad4[4];                              
};
#endif                        

#ifdef KERNEL
#ifdef __APPLE_API_PRIVATE
#include <machine/types.h>

#if __DARWIN_ALIGN_NATURAL
#pragma options align=natural
#endif

typedef user_ulong_t    user_msgqnum_t;
typedef user64_ulong_t  user64_msgqnum_t;
typedef user32_ulong_t  user32_msgqnum_t;

typedef user_ulong_t    user_msglen_t;
typedef user64_ulong_t  user64_msglen_t;
typedef user32_ulong_t  user32_msglen_t;

                    
struct user_msqid_ds {
	struct ipc_perm msg_perm;                                        
	struct msg      *msg_first;                                     
	struct msg      *msg_last;                                     
	user_msglen_t   msg_cbytes;                                  
	user_msgqnum_t  msg_qnum;                                              
	user_msglen_t   msg_qbytes;                                       
	pid_t           msg_lspid;                                      
	pid_t           msg_lrpid;                                      
	user_time_t     msg_stime;                                       
	__int32_t       msg_pad1;                                 
	user_time_t     msg_rtime;                                       
	__int32_t       msg_pad2;                                 
	user_time_t     msg_ctime;                                       
	__int32_t       msg_pad3;                                 
	__int32_t       msg_pad4[4];
};

  
                                                            
                                                           
   
struct user64_msqid_ds {
	struct ipc_perm msg_perm;                                        
	__int32_t       msg_first;                                     
	__int32_t       msg_last;                                      
	user64_msglen_t msg_cbytes;                                  
	user64_msgqnum_t        msg_qnum;                                              
	user64_msglen_t msg_qbytes;                                       
	pid_t           msg_lspid;                                      
	pid_t           msg_lrpid;                                      
	user64_time_t   msg_stime;                                       
	__int32_t       msg_pad1;                                 
	user64_time_t   msg_rtime;                                       
	__int32_t       msg_pad2;                                 
	user64_time_t   msg_ctime;                                       
	__int32_t       msg_pad3;                                 
	__int32_t       msg_pad4[4];
} __attribute__((__packed__));

struct user32_msqid_ds {
	struct __ipc_perm_new   msg_perm;                                  
	__int32_t       msg_first;                                     
	__int32_t       msg_last;                                      
	user32_msglen_t msg_cbytes;                                  
	user32_msgqnum_t        msg_qnum;                                              
	user32_msglen_t msg_qbytes;                                       
	pid_t           msg_lspid;                                      
	pid_t           msg_lrpid;                                      
	user32_time_t           msg_stime;                                       
	__int32_t       msg_pad1;                                 
	user32_time_t           msg_rtime;                                       
	__int32_t       msg_pad2;                                 
	user32_time_t           msg_ctime;                                       
	__int32_t       msg_pad3;                                 
	__int32_t       msg_pad4[4];                              
};

#if __DARWIN_ALIGN_NATURAL
#pragma options align=reset
#endif

struct label;

  
                                              
   
struct msqid_kernel {
	struct  user_msqid_ds u;
	struct  label *label;                            
};

#endif                           
#endif              


#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#ifdef __APPLE_API_UNSTABLE
                                               

struct msg {
	struct msg      *msg_next;                                 
	long            msg_type;                                 
	                                                                
	                                                      
	unsigned short  msg_ts;                                   
	short           msg_spot;                                            
	struct label    *label;                        
};

  
                                                                          
                                                                        
                                                                       
                                                                        
                                                                         
                                                    
   
struct mymsg {
	long    mtype;                                          
	char    mtext[1];                         
};

  
                                                                        
                       
  
                                                                         
                                                                       
                                                                        
                                                                            
                                                              
   
struct msginfo {
	int     msgmax,                                     
	    msgmni,                                                
	    msgmnb,                                       
	    msgtql,                                         
	    msgssz,                                                              
	    msgseg;                                             
};
#ifdef KERNEL
extern struct msginfo   msginfo;

#ifndef MSGSSZ
#define MSGSSZ  8                                                  
#endif
#ifndef MSGSEG
#define MSGSEG  2048                                         
#endif
#define MSGMAX  (MSGSSZ*MSGSEG)
#ifndef MSGMNB
#define MSGMNB  2048                                           
#endif
#ifndef MSGMNI
#define MSGMNI  40
#endif
#ifndef MSGTQL
#define MSGTQL  40
#endif

  
                                                    
                                    
   
#define MSQID(ix, ds)    ((ix) & 0xffff | (((ds).msg_perm.seq << 16) & 0xffff0000))
#define MSQID_IX(id)    ((id) & 0xffff)
#define MSQID_SEQ(id)   (((id) >> 16) & 0xffff)

  
                                                                       
   


  
                               
   
struct msgmap {
	short   next;                                       
	                                             
	                                                                    
};

                                                                             
extern char *msgpool;                                                 
extern struct msgmap *msgmaps;                                
extern struct msg *msghdrs;                             
extern struct msqid_kernel *msqids;                                        

#define MSG_LOCKED      01000                                 

#endif              
#endif                            
#endif                                              

#ifndef KERNEL

__BEGIN_DECLS
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
int msgsys(int, ...);
#endif                                              
int msgctl(int, int, struct msqid_ds *) __DARWIN_ALIAS(msgctl);
int msgget(key_t, int);
ssize_t msgrcv(int, void *, size_t, long, int) __DARWIN_ALIAS_C(msgrcv);
int msgsnd(int, const void *, size_t, int) __DARWIN_ALIAS_C(msgsnd);
__END_DECLS

#endif              

#endif                   
