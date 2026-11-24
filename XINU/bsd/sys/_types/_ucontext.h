/*
 * Copyright (c) 2003-2012 Apple Inc. All rights reserved.
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
#ifndef _STRUCT_UCONTEXT

#include <sys/cdefs.h>                      

#if __DARWIN_UNIX03
#define _STRUCT_UCONTEXT        struct __darwin_ucontext
#else                       
#define _STRUCT_UCONTEXT        struct ucontext
#endif                      

#include <machine/types.h>                      
#include <machine/_mcontext.h>                       
#include <sys/_types.h>                        
#include <sys/_types/_sigaltstack.h>                          

_STRUCT_UCONTEXT
{
	int                     uc_onstack;
	__darwin_sigset_t       uc_sigmask;                                           
	_STRUCT_SIGALTSTACK     uc_stack;                                       
	_STRUCT_UCONTEXT        *uc_link;                                        
	__darwin_size_t         uc_mcsize;                                                 
	_STRUCT_MCONTEXT        *uc_mcontext;                                            
#ifdef _XOPEN_SOURCE
	_STRUCT_MCONTEXT        __mcontext_data;
#endif                    
};

                  
typedef _STRUCT_UCONTEXT        ucontext_t;                             

#endif                       
