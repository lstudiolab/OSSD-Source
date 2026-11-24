/*
 * Copyright (c) 2000-2013 Apple Inc. All rights reserved.
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
/* Copyright (c) 1998, 1999 Apple Computer, Inc. All Rights Reserved */
/* Copyright (c) 1995, 1997 Apple Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1987, 1993
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
 *	@(#)malloc.h	8.5 (Berkeley) 5/3/95
 */
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef _SYS_MALLOC_H_
#define _SYS_MALLOC_H_

#include <sys/appleapiopts.h>


#ifdef KERNEL
  
                  
   
#define M_WAITOK        0x0000
#define M_NOWAIT        0x0001
#define M_ZERO          0x0004                                    
#define M_NULL          0x0008                                                  

#ifdef XNU_KERNEL_PRIVATE

#include <mach/vm_types.h>
#include <kern/kalloc.h>

ZONE_VIEW_DECLARE(ZV_NAMEI);

  
                                                           
   
#define M_DEVBUF        2                                 
#define M_PCB           4                                   
#define M_RTABLE        5                           
#define M_IFADDR        9                              
#define M_SONAME        11                       
#define M_PGRP          17                                
#define M_FHANDLE       21                               
#define M_NFSNODE       24                                  
#define M_VNODE         25                                        
#define M_CACHE         26                                               
#define M_DQUOT         27                             
#define M_PROC_UUID_POLICY      28                                    
#define M_SHM           29                                                  
#define M_LOCKF         40                                         
#define M_PROC          41                           
#define M_NETADDR       49                                         
#define M_NFSSVC        50                                
#define M_NFSD          52                                       
#define M_IPMOPTS       53                                      
#define M_IFMADDR       55                                        
#define M_NFSBIO        58                                  
#define M_NFSBIGFH      61                                     
#define M_TTYS          65                                    
#define M_OFILETABL     73                                      
#define M_TEMP          80                                       
#define M_SECA          81                                                 
#define M_DEVFS         82
#define M_UDFNODE       84                      
#define M_UDFMNT        85                                
#define M_IP6OPT        87                                   
#define M_KQUEUE        94                         
#define M_KAUTH         100                          
#define M_DUMMYNET      101                   
#define M_MACTEMP       104                        
#define M_SBUF          105                         
#define M_SELECT        107                                   
#define M_INMFILTER     110                                                 
#define M_IN6MFILTER    112                                                 
#define M_IP6MOPTS      113                                 
#define M_IP6CGA        117
#define M_NECP          118                                   
#define M_FD_VN_DATA    122                            
#define M_FD_DIRBUF     123                                    
#define M_NETAGENT      124                         
#define M_EVENTHANDLER  125                       
#define M_LLTABLE       126                           
#define M_NWKWQ         127                             
#define M_CFIL          128                         

#define M_LAST          129                                

  
                                        
                                       
   
#define KM_SHM          KHEAP_DEFAULT

#define MALLOC(space, cast, size, type, flags)                      \
	({ VM_ALLOC_SITE_STATIC(0, 0);                              \
	(space) = (cast)__MALLOC(size, type, flags, &site); })

#define REALLOC(space, cast, addr, size, type, flags)               \
	({ VM_ALLOC_SITE_STATIC(0, 0);                              \
	(space) = (cast)__REALLOC(addr, size, type, flags, &site); })

#define _MALLOC(size, type, flags)                                  \
	({ VM_ALLOC_SITE_STATIC(0, 0);                              \
	__MALLOC(size, type, flags, &site); })

#define _REALLOC(addr, size, type, flags)                           \
	({ VM_ALLOC_SITE_STATIC(0, 0);                              \
	__REALLOC(addr, size, type, flags, &site); })

#define _FREE(addr, type)                                           \
	(kheap_free_addr)(KHEAP_DEFAULT, addr)

#define FREE(addr, type)                                            \
	kheap_free_addr(KHEAP_DEFAULT, addr)

#pragma GCC visibility push(hidden)

extern void     *__MALLOC(
	size_t                size,
	int                   type,
	int                   flags,
	vm_allocation_site_t *site)  __attribute__((alloc_size(1)));

extern void     *__REALLOC(
	void                 *addr,
	size_t                size,
	int                   type,
	int                   flags,
	vm_allocation_site_t *site)  __attribute__((alloc_size(2)));

#pragma GCC visibility pop
#else                         

#define M_PCB           4                                         
#define M_RTABLE        5                           
#define M_IFADDR        9                                            
#define M_SONAME        11                             
#define M_LOCKF         40                                                 
#define M_TEMP          80                                       
#define M_UDFNODE       84                           
#define M_UDFMNT        85                                     
#define M_KAUTH         100                                

#define MALLOC(space, cast, size, type, flags) \
	(space) = (cast)_MALLOC(size, type, flags)

#define FREE(addr, type) \
	_FREE((void *)addr, type)

#define MALLOC_ZONE(space, cast, size, type, flags) \
	(space) = (cast)_MALLOC_ZONE(size, type, flags)

#define FREE_ZONE(addr, size, type) \
	_FREE_ZONE((void *)addr, size, type)

extern void     *_MALLOC(
	size_t          size,
	int             type,
	int             flags);

extern void     _FREE(
	void            *addr,
	int             type);

extern void     *_MALLOC_ZONE(
	size_t          size,
	int             type,
	int             flags);

extern void     _FREE_ZONE(
	void            *elem,
	size_t          size,
	int             type);

#endif                          

#endif              

#endif                      
