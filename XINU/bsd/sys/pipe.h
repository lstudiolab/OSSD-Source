/*
 * Copyright (c) 2004-2020 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1996 John S. Dyson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Absolutely no warranty of function or purpose is made by the author
 *    John S. Dyson.
 * 4. This work was done expressly for inclusion into FreeBSD.  Other use
 *    is allowed if this notation is included.
 * 5. Modifications may be freely made to this file if the above conditions
 *    are met.
 *
 * $FreeBSD: src/sys/sys/pipe.h,v 1.24 2003/08/13 20:01:38 alc Exp $
 */
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef _SYS_PIPE_H_
#define _SYS_PIPE_H_

#ifdef  KERNEL
#include <libkern/locks.h>
#include <os/base.h>
#endif
#include <sys/queue.h>                                        
#include <sys/ev.h>
#include <sys/cdefs.h>
#include <sys/_types/_caddr_t.h>
#include <sys/_types/_u_int.h>

  
                                                                  
   
#ifndef PIPE_SIZE
#define PIPE_SIZE       16384
#endif

#define PIPE_KVAMAX     (1024 * 1024 * 16)

#ifndef BIG_PIPE_SIZE
#define BIG_PIPE_SIZE   (64*1024)
#endif

#ifndef SMALL_PIPE_SIZE
#define SMALL_PIPE_SIZE PAGE_SIZE
#endif

  
                                                                   
                 
   
#ifndef PIPE_MINDIRECT
#define PIPE_MINDIRECT  8192
#endif

#define PIPENPAGES      (BIG_PIPE_SIZE / PAGE_SIZE + 1)

  
                           
                                                           
                                                             
   
struct pipebuf {
	u_int   cnt;                                                     
	u_int   in;                             
	u_int   out;                             
	u_int   size;                               
#if KERNEL
	caddr_t OS_PTRAUTH_SIGNED_PTR("pipe.buffer") buffer;                    
#else
	caddr_t buffer;                    
#endif             
};


#ifdef PIPE_DIRECT
  
                                                                       
   
struct pipemapping {
	vm_offset_t     kva;                                        
	vm_size_t       cnt;                                           
	vm_size_t       pos;                                              
	int             npages;                              
	vm_page_t       ms[PIPENPAGES];                              
};
#endif

  
                      
   
#define PIPE_ASYNC      0x004                    
#define PIPE_WANTR      0x008                                      
#define PIPE_WANTW      0x010                                              
#define PIPE_WANT       0x020                                       
#define PIPE_SEL        0x040                                  
#define PIPE_EOF        0x080                                  
#define PIPE_LOCKFL     0x100                                                       
#define PIPE_LWANT      0x200                                                         
#define PIPE_DIRECTW    0x400                                  
#define PIPE_DIRECTOK   0x800                        
                              
#define PIPE_DRAIN      0x2000                                                            
                                                                        
#define PIPE_WSELECT    0x4000                                                         
#define PIPE_DEAD       0x8000                                                 

#ifdef  KERNEL

struct label;

  
                           
                                                                    
   
struct pipe {
	struct  pipebuf pipe_buffer;                      
#ifdef PIPE_DIRECT
	struct  pipemapping pipe_map;                                    
#endif
	struct  selinfo pipe_sel;                                   
	pid_t   pipe_pgid;                                             
	struct  pipe *pipe_peer;                                       
	u_int   pipe_state;                                   
	int     pipe_busy;                                                              
	lck_mtx_t *pipe_mtxp;                                                
	struct  timespec st_atimespec;                           
	struct  timespec st_mtimespec;                                      
	struct  timespec st_ctimespec;                                  
	struct  label *pipe_label;                                   
};

#define PIPE_MTX(pipe)          ((pipe)->pipe_mtxp)

#define PIPE_LOCK(pipe)         lck_mtx_lock(PIPE_MTX(pipe))
#define PIPE_UNLOCK(pipe)       lck_mtx_unlock(PIPE_MTX(pipe))
#define PIPE_LOCK_ASSERT(pipe, type)  LCK_MTX_ASSERT(PIPE_MTX(pipe), (type))

__BEGIN_DECLS
extern int pipe_stat(struct pipe *, void *, int);
#ifdef BSD_KERNEL_PRIVATE
extern uint64_t pipe_id(struct pipe *);
#endif
__END_DECLS

#endif              

#endif                    
