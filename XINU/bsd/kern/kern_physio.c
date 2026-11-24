/*
 * Copyright (c) 2000-2004 Apple Computer, Inc. All rights reserved.
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
/*-
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	from: @(#)kern_physio.c	8.1 (Berkeley) 6/10/93
 */
  
          
                                                     
                                  
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf_internal.h>
#include <sys/conf.h>
#include <sys/proc_internal.h>
#include <sys/uio_internal.h>
#include <kern/assert.h>

int
physio( void (*f_strategy)(buf_t),
    buf_t bp,
    dev_t dev,
    int flags,
    u_int (*f_minphys)(buf_t),
    struct uio *uio,
    int blocksize)
{
	struct proc *p = current_proc();
	int error, i, buf_allocated, todo;
	size_t iosize;
	int orig_bflags = 0;
	int64_t done;

	error = 0;
	flags &= B_READ | B_WRITE;
	buf_allocated = 0;

	  
                                                     
   
                                                                     
                                                                    
                                                       
    
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (UIO_SEG_IS_USER_SPACE(uio->uio_segflg)) {
			user_addr_t base;
			user_size_t len;

			if (uio_getiov(uio, i, &base, &len) ||
			    !useracc(base,
			    len,
			    (flags == B_READ) ? B_WRITE : B_READ)) {
				return EFAULT;
			}
		}
	}
	  
                                                          
    
	if (bp == NULL) {
		bp = buf_alloc((vnode_t)0);
		buf_allocated = 1;
	} else {
		orig_bflags = buf_flags(bp);
	}
	  
                                         
                                       
                                        
                                      
                                       
                                    
    
	assert(bp->b_lflags & BL_BUSY);

	  
                                                        
    
	bp->b_dev = dev;
	bp->b_proc = p;

	  
                                           
                                                 
                                             
                                                   
                            
    
	buf_setflags(bp, B_PHYS | B_RAW);

	  
                                                      
                                                                
                        
    
	while (uio_resid(uio) > 0) {
		iosize = uio_curriovlen(uio);
		if (iosize > MAXPHYSIO_WIRED) {
			iosize = MAXPHYSIO_WIRED;
		}

		  
                                             
                           
     
		buf_reset(bp, flags);

		                                                      
		buf_setblkno(bp, uio_offset(uio) / blocksize);
		assert(iosize <= UINT32_MAX);
		buf_setcount(bp, (uint32_t)iosize);
		buf_setdataptr(bp, (uintptr_t)CAST_DOWN(caddr_t, uio_curriovbase(uio)));

		  
                                               
                                                 
                          
     
		(*f_minphys)(bp);
		todo = buf_count(bp);

		  
                                                      
                        
     

		if (UIO_SEG_IS_USER_SPACE(uio->uio_segflg)) {
			error = vslock(CAST_USER_ADDR_T(buf_dataptr(bp)),
			    (user_size_t)todo);
			if (error) {
				goto finished;
			}
		}

		                                             
		(*f_strategy)(bp);


		                                         
		error = (int)buf_biowait(bp);

		  
                                                     
               
     
		if (UIO_SEG_IS_USER_SPACE(uio->uio_segflg)) {
			vsunlock(CAST_USER_ADDR_T(buf_dataptr(bp)),
			    (user_size_t)todo,
			    (flags & B_READ));
		}

		  
                                                    
                            
     
		done = buf_count(bp) - buf_resid(bp);
		assert(0 <= done && done <= UINT32_MAX);
		uio_update(uio, (user_size_t)done);

		  
                             
                                              
     
		if (error || done < todo) {
			goto finished;
		}
	}

finished:
	if (buf_allocated) {
		buf_free(bp);
	} else {
		buf_setflags(bp, orig_bflags);
	}

	return error;
}

  
                                   
                                                             
                                                           
                           
  
                                                                 
                            
   
u_int
minphys(struct buf *bp)
{
	buf_setcount(bp, min(MAXPHYS, buf_count(bp)));
	return buf_count(bp);
}
