/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
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
 *	@(#)sysctl.h	8.1 (Berkeley) 6/2/93
 */
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef _SYS_SYSCTL_H_
#define _SYS_SYSCTL_H_

  
                                                   
   
#include <sys/cdefs.h>

#include <sys/appleapiopts.h>
#ifndef KERNEL
#include <sys/time.h>
#include <sys/ucred.h>
#else
#ifdef XNU_KERNEL_PRIVATE
#include <kern/startup.h>
#include <libkern/section_keywords.h>
#else
#include <libkern/sysctl.h>
#include <os/base.h>
#endif                         
#endif             

#include <sys/proc.h>
#include <sys/vm.h>

  
                                                                         
                                                                          
                                                                      
                                                                           
                                                                         
                                     
   

#define CTL_MAXNAME     12                                                  

  
                                                               
                                                              
                                                              
                                                                 
                                                               
  
                                                                      
                                                              
                     
  
                                                                 
                                           
  
                                                             
                                                        
                                                       
                                                       
                                                  
  
         
                                       
           
                                          
  
                                                       
                                                     
                                        
  
                                                       
                                                      
                                                       
                                       
   
struct ctlname {
	char    *ctl_name;                          
	int     ctl_type;                         
};

#define CTLTYPE             0xf                                    
#define CTLTYPE_NODE        1                                   
#define CTLTYPE_INT         2                                              
#define CTLTYPE_STRING      3                                            
#define CTLTYPE_QUAD        4                                                   
#define CTLTYPE_OPAQUE      5                                               
#define CTLTYPE_STRUCT      CTLTYPE_OPAQUE                                  

#define CTLFLAG_RD          0x80000000                                   
#define CTLFLAG_WR          0x40000000                                        
#define CTLFLAG_RW          (CTLFLAG_RD|CTLFLAG_WR)
#define CTLFLAG_NOLOCK      0x20000000                          
#define CTLFLAG_ANYBODY     0x10000000                                      
#define CTLFLAG_SECURE      0x08000000                                             
#define CTLFLAG_MASKED      0x04000000                                               
#define CTLFLAG_NOAUTO      0x02000000                                
#define CTLFLAG_KERN        0x01000000                                   
#define CTLFLAG_LOCKED      0x00800000                                           
#define CTLFLAG_OID2        0x00400000                                              
#if XNU_KERNEL_PRIVATE
#define CTLFLAG_PERMANENT   0x00200000                                
#endif
#define CTLFLAG_EXPERIMENT 0x00100000                                                          

  
                                                                   
                                                                  
                                                                     
                  
  
                                                                          
  
                                                                        
                                                                       
                                                                   
                                  
   
#define OID_AUTO              (-1)
#if XNU_KERNEL_PRIVATE
  
                                                                           
                                                                            
                                                                          
  
                                            
   
#define OID_MUTABLE_ANCHOR    (INT_MIN)
#endif
#define OID_AUTO_START        100                   

#ifdef KERNEL
#define SYSCTL_HANDLER_ARGS \
	(struct sysctl_oid *oidp __unused, void *arg1 __unused, int arg2 __unused, \
	struct sysctl_req *req)


  
                                                                        
                                                                       
   
struct sysctl_req {
	struct proc     *p;
	int             lock;
	user_addr_t     oldptr;                                              
	size_t          oldlen;                                                 
	size_t          oldidx;                                                
	int             (*oldfunc)(struct sysctl_req *, const void *, size_t);
	user_addr_t     newptr;                                          
	size_t          newlen;                                  
	size_t          newidx;                                               
	int             (*newfunc)(struct sysctl_req *, void *, size_t);
};

SLIST_HEAD(sysctl_oid_list, sysctl_oid);

#define SYSCTL_OID_VERSION      1                                          

  
                                                                        
                                                
  
                                                                    
                                                              
                                                             
                                                            
                                                             
                                                              
                                                            
                                                            
                                                             
                                                              
                                                                  
                                                                
                                                           
                                                            
          
  
                                                                 
                           
  
                                                         
                                                              
                                                               
                                                              
                                                               
                                                                
                                                               
                                                               
                                                 
  
                                                                
                                                                
                                                                 
                        
   
struct sysctl_oid {
	struct sysctl_oid_list * OS_PTRAUTH_SIGNED_PTR("sysctl_oid.oid_parent") oid_parent;
	SLIST_ENTRY(sysctl_oid) oid_link;
	int             oid_number;
	int             oid_kind;
	void            *oid_arg1;
	int             oid_arg2;
	const char      *oid_name;
	int             (*oid_handler)SYSCTL_HANDLER_ARGS;
	const char      *oid_fmt;
	const char      *oid_descr;                                          
	int             oid_version;
	int             oid_refcnt;
};

#define SYSCTL_IN(r, p, l) (r->newfunc)(r, p, l)
#define SYSCTL_OUT(r, p, l) (r->oldfunc)(r, p, l)

typedef int (* sysctl_handler_t) SYSCTL_HANDLER_ARGS;

__BEGIN_DECLS

                   
int sysctl_handle_int SYSCTL_HANDLER_ARGS;
int sysctl_handle_long SYSCTL_HANDLER_ARGS;
int sysctl_handle_quad SYSCTL_HANDLER_ARGS;
int sysctl_handle_int2quad SYSCTL_HANDLER_ARGS;
int sysctl_handle_string SYSCTL_HANDLER_ARGS;
int sysctl_handle_opaque SYSCTL_HANDLER_ARGS;
                   
int sysctl_io_number(struct sysctl_req *req, long long bigValue, size_t valueSize, void *pValue, int *changed);
int sysctl_io_string(struct sysctl_req *req, char *pValue, size_t valueSize, int trunc, int *changed);
int sysctl_io_opaque(struct sysctl_req *req, void *pValue, size_t valueSize, int *changed);

  
                                                              
   
void sysctl_register_oid(struct sysctl_oid *oidp);
void sysctl_unregister_oid(struct sysctl_oid *oidp);

#define nvram_osenvironment "osenvironment"
void sysctl_set_osenvironment(unsigned int size, const void* value);
void sysctl_unblock_osenvironment(void);

                
void sysctl_register_fixed(void) __deprecated;

__END_DECLS

                                                           
#define SYSCTL_DECL(name)                                       \
	extern struct sysctl_oid_list sysctl_##name##_children

  
                                                                      
                                                                     
                                                                          
                    
  
                                                           
                                                                    
                                                                              
                                                                         
                                                                      
                                                                                        
                                                                                           
                                                                                   
                                                         
                                                              
                                                                        
                               
                                                                                             
                                     
                                              
                  
                                   
                                                                                                        
                      
   


                                      
#define SYSCTL_STRUCT_INIT(parent, nbr, name, kind, a1, a2, fn, fmt, desc) {    \
	    .oid_parent     = &sysctl_##parent##_children,                      \
	    .oid_number     = nbr,                                              \
	    .oid_kind       = (int)(kind | CTLFLAG_OID2),                       \
	    .oid_arg1       = a1,                                               \
	    .oid_arg2       = (int)(a2),                                        \
	    .oid_name       = #name,                                            \
	    .oid_handler    = fn,                                               \
	    .oid_fmt        = fmt,                                              \
	    .oid_descr      = desc,                                             \
	    .oid_version    = SYSCTL_OID_VERSION,                               \
	}

#define __SYSCTL_OID(parent, nbr, name, kind, a1, a2, handler, fmt, descr) \
	struct sysctl_oid sysctl_##parent##_##name = SYSCTL_STRUCT_INIT(\
	    parent, nbr, name, kind, a1, a2, handler, fmt, descr)

#if XNU_KERNEL_PRIVATE

  
                                                                           
                       
  
                                                                                
                                                                                
                                                          
  
                                                                           
                                                                           
            
  
                                                                             
                                                                           
  
                                                                           
                                       
  
                                                                              
                                                                           
  
                                                                               
                                                   
  
                                                                              
                          
  
                                                                      
                                                                        
                                          
  
                                          
   

__BEGIN_DECLS
void sysctl_register_oid_early(struct sysctl_oid *oidp);
__END_DECLS

#define SYSCTL_OID_MANUAL(parent, nbr, name, kind, a1, a2, handler, fmt, descr) \
	__XNU_PRIVATE_EXTERN                                                    \
	__SYSCTL_OID(parent, nbr, name, kind, a1, a2, handler, fmt, descr)

#define SYSCTL_NODE_MANUAL(parent, nbr, name, access, handler, descr)           \
	struct sysctl_oid_list sysctl_##parent##_##name##_children;             \
	__XNU_PRIVATE_EXTERN                                                    \
	__SYSCTL_OID(parent, nbr, name, CTLTYPE_NODE|access,                    \
	    &sysctl_##parent##_##name##_children, 0, handler, "N", descr);

#define SYSCTL_OID(parent, nbr, name, kind, a1, a2, handler, fmt, descr)        \
	__security_const_late __XNU_PRIVATE_EXTERN                              \
	__SYSCTL_OID(parent, nbr, name, CTLFLAG_PERMANENT|kind,                 \
	    a1, a2, handler, fmt, descr);                                       \
	__STARTUP_ARG(sysctl_##parent, _##name,                                 \
	    SYSCTL, STARTUP_RANK_SECOND, sysctl_register_oid_early,             \
	    &sysctl_##parent##_##name)

#define __SYSCTL_NODE(parent, nbr, name, access, handler, descr)                \
	__security_const_late                                                   \
	struct sysctl_oid_list sysctl_##parent##_##name##_children;             \
	__security_const_late __XNU_PRIVATE_EXTERN                              \
	__SYSCTL_OID(parent, nbr, name, CTLFLAG_PERMANENT|CTLTYPE_NODE|access,  \
	    &sysctl_##parent##_##name##_children, 0, handler, "N", descr);      \
	__STARTUP_ARG(sysctl_##parent, _##name,                                 \
	    SYSCTL, STARTUP_RANK_FIRST, sysctl_register_oid_early,              \
	    &sysctl_##parent##_##name)

#define __SYSCTL_EXTENSION_NODE(name)                                           \
	static __security_read_write                                            \
	struct sysctl_oid_list sysctl_##name##_children_mutable;                \
	static __security_const_late                                            \
	struct sysctl_oid sysctl_##name##_wranchor = {                          \
	    .oid_parent     = &sysctl_##name##_children,                        \
	    .oid_number     = OID_MUTABLE_ANCHOR,                               \
	    .oid_kind       = CTLFLAG_OID2 | CTLFLAG_PERMANENT,                 \
	    .oid_arg1       = &sysctl_##name##_children_mutable,                \
	    .oid_name       = "__anchor__(" #name ")",                          \
	    .oid_version    = SYSCTL_OID_VERSION,                               \
	};                                                                      \
	__STARTUP_ARG(sysctl_##name, _wranchor,                                 \
	    SYSCTL, STARTUP_RANK_LAST, sysctl_register_oid_early,               \
	    &sysctl_##name##_wranchor)

#define SYSCTL_NODE(parent, nbr, name, access, handler, descr)                  \
	__XNU_PRIVATE_EXTERN                                                    \
	__SYSCTL_NODE(parent, nbr, name, access, handler, descr)

#define SYSCTL_EXTENSIBLE_NODE(parent, nbr, name, access, handler, descr)       \
	__SYSCTL_NODE(parent, nbr, name, access, handler, descr);               \
	__SYSCTL_EXTENSION_NODE(parent##_##name)
#else
#define SYSCTL_OID(parent, nbr, name, kind, a1, a2, handler, fmt, descr) \
	__SYSCTL_OID(parent, nbr, name, kind, a1, a2, handler, fmt, descr)

                                                            
#define SYSCTL_NODE(parent, nbr, name, access, handler, descr)                  \
	struct sysctl_oid_list sysctl_##parent##_##name##_children;             \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_NODE|access,                      \
	    &sysctl_##parent##_##name##_children, 0, handler, "N", descr)
#endif                         

                                                                   
#define SYSCTL_STRING(parent, nbr, name, access, arg, len, descr) \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_STRING|access, \
	    arg, len, sysctl_handle_string, "A", descr)

#define SYSCTL_COMPAT_INT(parent, nbr, name, access, ptr, val, descr) \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_INT|access, \
	    ptr, val, sysctl_handle_int, "I", descr)

#define SYSCTL_COMPAT_UINT(parent, nbr, name, access, ptr, val, descr) \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_INT|access, \
	    ptr, val, sysctl_handle_int, "IU", descr)

                                                       
#define SYSCTL_INT(parent, nbr, name, access, ptr, val, descr) \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_INT|access, \
	    ptr, val, sysctl_handle_int, "I", descr); \
	_Static_assert(__builtin_constant_p(ptr) || sizeof(*(ptr)) == sizeof(int), \
	    "must be integer sized");

                                                                
#define SYSCTL_UINT(parent, nbr, name, access, ptr, val, descr) \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_INT|access, \
	    ptr, val, sysctl_handle_int, "IU", descr); \
	_Static_assert(__builtin_constant_p(ptr) || sizeof(*(ptr)) == sizeof(unsigned int), \
	    "must be integer sized");

                                                    
#define SYSCTL_LONG(parent, nbr, name, access, ptr, descr) \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_INT|access, \
	    ptr, 0, sysctl_handle_long, "L", descr); \
	_Static_assert(__builtin_constant_p(ptr) || sizeof(*(ptr)) == sizeof(long), \
	    "must be long sized");

                                                             
#define SYSCTL_ULONG(parent, nbr, name, access, ptr, descr) \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_INT|access, \
	    ptr, 0, sysctl_handle_long, "LU", descr); \
	_Static_assert(__builtin_constant_p(ptr) || sizeof(*(ptr)) == sizeof(unsigned long), \
	    "must be long sized");

                                                    
#define SYSCTL_QUAD(parent, nbr, name, access, ptr, descr) \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_QUAD|access, \
	    ptr, 0, sysctl_handle_quad, "Q", descr); \
	_Static_assert(__builtin_constant_p(ptr) || sizeof(*(ptr)) == sizeof(long long), \
	    "must be long long sized");

                                                                     
#define SYSCTL_OPAQUE(parent, nbr, name, access, ptr, len, fmt, descr) \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_OPAQUE|access, \
	        ptr, len, sysctl_handle_opaque, fmt, descr)

                                                           
#define SYSCTL_STRUCT(parent, nbr, name, access, ptr, type, descr) \
	SYSCTL_OID(parent, nbr, name, CTLTYPE_OPAQUE|access, \
	    ptr, sizeof(struct type), sysctl_handle_opaque, \
	    "S," #type, descr)

  
                                                           
                                                                  
                                  
   
#define SYSCTL_PROC(parent, nbr, name, access, ptr, arg, handler, fmt, descr) \
	SYSCTL_OID(parent, nbr, name, access, \
	    ptr, arg, handler, fmt, descr)

  
                                                                                                   
                                                                                          
                                    
                                                                                                  
                                                                                                
                                                                      
                                                                                                                   
                                                                                                          
                                                                                                             
   

struct experiment_spec {
	void *ptr;                                        
	uint64_t min_value;                                                           
	uint64_t max_value;                                                           
	uint64_t original_value;                                                      
	_Atomic bool modified;                                                          
};

  
                                                                          
                                     
   
#define experiment_factor_numeric_types \
    X(uint, unsigned int) \
    X(int, int) \
    X(ulong, unsigned long) \
    X(long, long) \
    X(uint64, uint64_t) \
    X(int64, int64_t)

#define X(experiment_factor_typename, _) \
int experiment_factor_##experiment_factor_typename##_handler SYSCTL_HANDLER_ARGS;

experiment_factor_numeric_types
#undef X

#define __EXPERIMENT_FACTOR_SPEC(parent, name, p, min, max) \
	struct experiment_spec experiment_##parent##_##name = { \
	        .ptr = p, \
	        .min_value = min, \
	        .max_value = max, \
	        .original_value = 0, \
	        .modified = false \
	}

#define EXPERIMENT_FACTOR_UINT(parent, name, ptr, min, max, descr) \
	__EXPERIMENT_FACTOR_SPEC(parent, name, ptr, min, max); \
	_Static_assert(sizeof(*(ptr)) == sizeof(unsigned int), "must be integer sized"); \
	SYSCTL_PROC(parent, OID_AUTO, name, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_EXPERIMENT, &experiment_##parent##_##name, 1, &experiment_factor_uint_handler, "IU", descr);

#define EXPERIMENT_FACTOR_INT(parent, name, ptr, min, max, descr) \
	__EXPERIMENT_FACTOR_SPEC(parent, name, ptr, min, max); \
	_Static_assert(sizeof(*(ptr)) == sizeof(int), "must be integer sized"); \
	SYSCTL_PROC(parent, OID_AUTO, name, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_EXPERIMENT, &experiment_##parent##_##name, 1, &experiment_factor_int_handler, "I", descr);

#define EXPERIMENT_FACTOR_ULONG(parent, name, ptr, min, max, descr) \
	__EXPERIMENT_FACTOR_SPEC(parent, name, ptr, min, max); \
	_Static_assert(sizeof(*(ptr)) == sizeof(unsigned long), "must be long sized"); \
	SYSCTL_PROC(parent, OID_AUTO, name, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_EXPERIMENT, &experiment_##parent##_##name, 1, &experiment_factor_ulong_handler, "LU", descr);

#define EXPERIMENT_FACTOR_LONG(parent, name, ptr, min, max, descr) \
	__EXPERIMENT_FACTOR_SPEC(parent, name, ptr, min, max); \
	_Static_assert(sizeof(*(ptr)) == sizeof(long), "must be long sized"); \
	SYSCTL_PROC(parent, OID_AUTO, name, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_EXPERIMENT, &experiment_##parent##_##name, 1, &experiment_factor_long_handler, "L", descr);

#define EXPERIMENT_FACTOR_UINT64(parent, name, ptr, min, max, descr) \
	__EXPERIMENT_FACTOR_SPEC(parent, name, ptr, min, max); \
	_Static_assert(sizeof(*(ptr)) == sizeof(uint64_t), "must be 8 bytes"); \
	SYSCTL_PROC(parent, OID_AUTO, name, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_EXPERIMENT, &experiment_##parent##_##name, 1, &experiment_factor_uint64_handler, "QU", descr);

#define EXPERIMENT_FACTOR_INT64(parent, name, ptr, min, max, descr) \
	__EXPERIMENT_FACTOR_SPEC(parent, name, ptr, min, max); \
	_Static_assert(sizeof(*(ptr)) == sizeof(int64_t), "must be 8 bytes"); \
	SYSCTL_PROC(parent, OID_AUTO, name, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY | CTLFLAG_EXPERIMENT, &experiment_##parent##_##name, 1, &experiment_factor_int64_handler, "Q", descr);

  
                                                              
                                                                                                                     
                                                                          
   
#define EXPERIMENT_FACTOR_PROC(parent, name, access, ptr, arg, handler, fmt, descr) \
	_Static_assert(arg != 1, "arg can not be 1"); \
	SYSCTL_PROC(parent, OID_AUTO, name, access | CTLFLAG_ANYBODY | CTLFLAG_EXPERIMENT, ptr, arg, handler, fmt, descr);

#ifdef XNU_KERNEL_PRIVATE
  
                                               
                                                                                
   
int scalable_counter_sysctl_handler SYSCTL_HANDLER_ARGS;

   
                                 
  
            
                                                               
   
#define SYSCTL_SCALABLE_COUNTER(parent, name, counter, descr) \
SYSCTL_PROC(parent, OID_AUTO, name, CTLTYPE_QUAD | CTLFLAG_RD | CTLFLAG_LOCKED, \
    (void *)(&counter), 0, &scalable_counter_sysctl_handler, "Q", descr);
#endif                         

extern struct sysctl_oid_list sysctl__children;
SYSCTL_DECL(_kern);
SYSCTL_DECL(_sysctl);
SYSCTL_DECL(_vm);
SYSCTL_DECL(_vfs);
SYSCTL_DECL(_net);
SYSCTL_DECL(_debug);
SYSCTL_DECL(_hw);
SYSCTL_DECL(_machdep);
SYSCTL_DECL(_user);

#ifdef PRIVATE
SYSCTL_DECL(_kern_bridge);
SYSCTL_DECL(_hw_features);
#endif


#ifndef SYSCTL_SKMEM_UPDATE_FIELD

#define SYSCTL_SKMEM 0
#define SYSCTL_SKMEM_UPDATE_FIELD(field, value)
#define SYSCTL_SKMEM_UPDATE_AT_OFFSET(offset, value)
#define SYSCTL_SKMEM_INT(parent, oid, sysctl_name, access, ptr, offset, descr) \
	SYSCTL_INT(parent, oid, sysctl_name, access, ptr, 0, descr)

#define SYSCTL_SKMEM_TCP_INT(oid, sysctl_name, access, variable_type,   \
	    variable_name, initial_value, descr)           \
	variable_type variable_name = initial_value;                                            \
	SYSCTL_SKMEM_INT(_net_inet_tcp, oid, sysctl_name, access,                       \
	                                 &variable_name, 0, descr)

#else                                
#define SYSCTL_SKMEM 1
#endif                                



#endif             

#ifdef XNU_KERNEL_PRIVATE
#define SYSCTL_DEF_ENABLED
#else
#ifndef KERNEL
#define SYSCTL_DEF_ENABLED
#endif
#endif

#ifdef SYSCTL_DEF_ENABLED
  
                        
   
#define CTL_UNSPEC      0                           
#define CTL_KERN        1                                                
#define CTL_VM          2                                   
#define CTL_VFS         3                                                    
#define CTL_NET         4                                          
#define CTL_DEBUG       5                                         
#define CTL_HW          6                                   
#define CTL_MACHDEP     7                                      
#define CTL_USER        8                               
#define CTL_MAXID       9                                                  

#define CTL_NAMES { \
	{ 0, 0 }, \
	{ "kern", CTLTYPE_NODE }, \
	{ "vm", CTLTYPE_NODE }, \
	{ "vfs", CTLTYPE_NODE }, \
	{ "net", CTLTYPE_NODE }, \
	{ "debug", CTLTYPE_NODE }, \
	{ "hw", CTLTYPE_NODE }, \
	{ "machdep", CTLTYPE_NODE }, \
	{ "user", CTLTYPE_NODE }, \
}

  
                       
   
#define KERN_OSTYPE              1                                  
#define KERN_OSRELEASE           2                                  
#define KERN_OSREV               3                                
#define KERN_VERSION             4                                     
#define KERN_MAXVNODES           5                           
#define KERN_MAXPROC             6                              
#define KERN_MAXFILES            7                               
#define KERN_ARGMAX              8                                      
#define KERN_SECURELVL           9                                      
#define KERN_HOSTNAME           10                            
#define KERN_HOSTID             11                                
#define KERN_CLOCKRATE          12                                    
#define KERN_VNODE              13                                    
#define KERN_PROC               14                                   
#define KERN_FILE               15                                
#define KERN_PROF               16                                       
#define KERN_POSIX1             17                                
#define KERN_NGROUPS            18                                            
#define KERN_JOB_CONTROL        19                                         
#define KERN_SAVED_IDS          20                                        
#define KERN_BOOTTIME           21                                          
#define KERN_NISDOMAINNAME      22                                  
#define KERN_DOMAINNAME         KERN_NISDOMAINNAME
#define KERN_MAXPARTITIONS      23                                          
#define KERN_KDEBUG                     24                                    
#define KERN_UPDATEINTERVAL     25                                          
#define KERN_OSRELDATE          26                                
#define KERN_NTP_PLL            27                                 
#define KERN_BOOTFILE           28                                         
#define KERN_MAXFILESPERPROC    29                                        
#define KERN_MAXPROCPERUID      30                                      
#define KERN_DUMPDEV            31                                    
#define KERN_IPC                32                                         
#define KERN_DUMMY              33                  
#define KERN_PS_STRINGS 34                                      
#define KERN_USRSTACK32 35                                    
#define KERN_LOGSIGEXIT 36                                         
#define KERN_SYMFILE            37                                          
#define KERN_PROCARGS           38
                                           
#define KERN_NETBOOT            40                                             
                                                               
#define KERN_SYSV               42                                          
#define KERN_AFFINITY           43               
#define KERN_TRANSLATE          44               
#define KERN_CLASSIC            KERN_TRANSLATE                            
#define KERN_EXEC               45               
#define KERN_CLASSICHANDLER     KERN_EXEC                                  
#define KERN_AIOMAX             46                                 
#define KERN_AIOPROCMAX         47                                             
#define KERN_AIOTHREADS         48                                       
#ifdef __APPLE_API_UNSTABLE
#define KERN_PROCARGS2          49
#endif                           
#define KERN_COREFILE           50                                          
#define KERN_COREDUMP           51                                           
#define KERN_SUGID_COREDUMP     52                                            
#define KERN_PROCDELAYTERM      53                                                                               
#define KERN_SHREG_PRIVATIZABLE 54                                                   
                                                   
#define KERN_LOW_PRI_WINDOW     56                                                         
#define KERN_LOW_PRI_DELAY      57                                                        
#define KERN_POSIX              58                                
#define KERN_USRSTACK64         59                                 
#define KERN_NX_PROTECTION      60                                                         
#define KERN_TFP                61                                 
#define KERN_PROCNAME           62                                                    
#define KERN_THALTSTACK         63                                                      
#define KERN_SPECULATIVE_READS  64                                                       
#define KERN_OSVERSION          65                                       
#define KERN_SAFEBOOT           66                               
                                      
#define KERN_RAGEVNODE          68
#define KERN_TTY                69                              
#define KERN_CHECKOPENEVT       70                                                               
#define KERN_THREADNAME         71                               
#define KERN_MAXID              72                                    
  
                                                                            
                                                                            
                                     
   

#if COUNT_SYSCALLS && defined(KERNEL)
#define KERN_COUNT_SYSCALLS (KERN_OSTYPE + 1000)                                                    
#endif

#if defined(__LP64__)
#define KERN_USRSTACK KERN_USRSTACK64
#else
#define KERN_USRSTACK KERN_USRSTACK32
#endif


                          
#define KERN_RAGE_PROC          1
#define KERN_RAGE_THREAD        2
#define KERN_UNRAGE_PROC        3
#define KERN_UNRAGE_THREAD      4

                        
#define KERN_OPENEVT_PROC     1
#define KERN_UNOPENEVT_PROC   2

                    
#define KERN_TFP_POLICY                 1

                                                                    
#define KERN_TFP_POLICY_DENY            0                                                      
#define KERN_TFP_POLICY_DEFAULT         2                                                                          

                       
#define KERN_KDEFLAGS         1
#define KERN_KDDFLAGS         2
#define KERN_KDENABLE         3
#define KERN_KDSETBUF         4
#define KERN_KDGETBUF         5
#define KERN_KDSETUP          6
#define KERN_KDREMOVE         7
#define KERN_KDSETREG         8
#define KERN_KDGETREG         9
#define KERN_KDREADTR         10
#define KERN_KDPIDTR          11
#define KERN_KDTHRMAP         12
                                                      
#define KERN_KDPIDEX          14
#define KERN_KDSETRTCDEC      15               
#define KERN_KDGETENTROPY     16               
#define KERN_KDWRITETR        17
#define KERN_KDWRITEMAP       18
#define KERN_KDTEST           19
               
#define KERN_KDREADCURTHRMAP  21
#define KERN_KDSET_TYPEFILTER 22
#define KERN_KDBUFWAIT        23
#define KERN_KDCPUMAP         24
                    
#define KERN_KDWRITEMAP_V3    27
#define KERN_KDWRITETR_V3     28

#define CTL_KERN_NAMES { \
	{ 0, 0 }, \
	{ "ostype", CTLTYPE_STRING }, \
	{ "osrelease", CTLTYPE_STRING }, \
	{ "osrevision", CTLTYPE_INT }, \
	{ "version", CTLTYPE_STRING }, \
	{ "maxvnodes", CTLTYPE_INT }, \
	{ "maxproc", CTLTYPE_INT }, \
	{ "maxfiles", CTLTYPE_INT }, \
	{ "argmax", CTLTYPE_INT }, \
	{ "securelevel", CTLTYPE_INT }, \
	{ "hostname", CTLTYPE_STRING }, \
	{ "hostid", CTLTYPE_INT }, \
	{ "clockrate", CTLTYPE_STRUCT }, \
	{ "vnode", CTLTYPE_STRUCT }, \
	{ "proc", CTLTYPE_STRUCT }, \
	{ "file", CTLTYPE_STRUCT }, \
	{ "profiling", CTLTYPE_NODE }, \
	{ "posix1version", CTLTYPE_INT }, \
	{ "ngroups", CTLTYPE_INT }, \
	{ "job_control", CTLTYPE_INT }, \
	{ "saved_ids", CTLTYPE_INT }, \
	{ "boottime", CTLTYPE_STRUCT }, \
	{ "nisdomainname", CTLTYPE_STRING }, \
	{ "maxpartitions", CTLTYPE_INT }, \
	{ "kdebug", CTLTYPE_INT }, \
	{ "update", CTLTYPE_INT }, \
	{ "osreldate", CTLTYPE_INT }, \
	{ "ntp_pll", CTLTYPE_NODE }, \
	{ "bootfile", CTLTYPE_STRING }, \
	{ "maxfilesperproc", CTLTYPE_INT }, \
	{ "maxprocperuid", CTLTYPE_INT }, \
	{ "dumpdev", CTLTYPE_STRUCT },                                  \
	{ "ipc", CTLTYPE_NODE }, \
	{ "dummy", CTLTYPE_INT }, \
	{ "dummy", CTLTYPE_INT }, \
	{ "usrstack", CTLTYPE_INT }, \
	{ "logsigexit", CTLTYPE_INT }, \
	{ "symfile",CTLTYPE_STRING },\
	{ "procargs",CTLTYPE_STRUCT },\
	{ "dummy", CTLTYPE_INT },                                          \
	{ "netboot", CTLTYPE_INT }, \
	{ "dummy", CTLTYPE_INT },                                           \
	{ "sysv", CTLTYPE_NODE }, \
	{ "dummy", CTLTYPE_INT }, \
	{ "dummy", CTLTYPE_INT }, \
	{ "exec", CTLTYPE_NODE }, \
	{ "aiomax", CTLTYPE_INT }, \
	{ "aioprocmax", CTLTYPE_INT }, \
	{ "aiothreads", CTLTYPE_INT }, \
	{ "procargs2",CTLTYPE_STRUCT }, \
	{ "corefile",CTLTYPE_STRING }, \
	{ "coredump", CTLTYPE_INT }, \
	{ "sugid_coredump", CTLTYPE_INT }, \
	{ "delayterm", CTLTYPE_INT }, \
	{ "shreg_private", CTLTYPE_INT }, \
	{ "proc_low_pri_io", CTLTYPE_INT }, \
	{ "low_pri_window", CTLTYPE_INT }, \
	{ "low_pri_delay", CTLTYPE_INT }, \
	{ "posix", CTLTYPE_NODE }, \
	{ "usrstack64", CTLTYPE_QUAD }, \
	{ "nx", CTLTYPE_INT }, \
	{ "tfp", CTLTYPE_NODE }, \
	{ "procname", CTLTYPE_STRING }, \
	{ "threadsigaltstack", CTLTYPE_INT }, \
	{ "speculative_reads_disabled", CTLTYPE_INT }, \
	{ "osversion", CTLTYPE_STRING }, \
	{ "safeboot", CTLTYPE_INT }, \
	{ "dummy", CTLTYPE_INT },                                      \
	{ "rage_vnode", CTLTYPE_INT }, \
	{ "tty", CTLTYPE_NODE },        \
	{ "check_openevt", CTLTYPE_INT }, \
	{ "thread_name", CTLTYPE_STRING } \
}

  
                      
   
#define CTL_VFS_NAMES { \
	{ "vfsconf", CTLTYPE_STRUCT } \
}

  
                     
   
#define KERN_PROC_ALL           0                       
#define KERN_PROC_PID           1                          
#define KERN_PROC_PGRP          2                                
#define KERN_PROC_SESSION       3                              
#define KERN_PROC_TTY           4                               
#define KERN_PROC_UID           5                             
#define KERN_PROC_RUID          6                        
#define KERN_PROC_LCID          7                                

  
                          
   
#define KERN_VFSNSPACE_HANDLE_PROC              1
#define KERN_VFSNSPACE_UNHANDLE_PROC    2

#if defined(XNU_KERNEL_PRIVATE) || !defined(KERNEL)
  
                                                                    
   

struct _pcred {
	char    pc_lock[72];                                
	struct  ucred *pc_ucred;                                  
	uid_t   p_ruid;                                    
	uid_t   p_svuid;                                              
	gid_t   p_rgid;                                     
	gid_t   p_svgid;                                               
	int     p_refcnt;                                          
};

struct _ucred {
	int32_t cr_ref;                                      
	uid_t   cr_uid;                                        
	short   cr_ngroups;                                   
	gid_t   cr_groups[NGROUPS];                 
};

struct kinfo_proc {
	struct  extern_proc kp_proc;                                        
	struct  eproc {
		struct  proc *e_paddr;                               
		struct  session *e_sess;                             
		struct  _pcred e_pcred;                                  
		struct  _ucred e_ucred;                                  
		struct   vmspace e_vm;                             
		pid_t   e_ppid;                                        
		pid_t   e_pgid;                                       
		short   e_jobc;                                          
		dev_t   e_tdev;                                          
		pid_t   e_tpgid;                                          
		struct  session *e_tsess;                                
#define WMESGLEN        7
		char    e_wmesg[WMESGLEN + 1];                       
		segsz_t e_xsize;                               
		short   e_xrssize;                            
		short   e_xccount;                                   
		short   e_xswrss;
		int32_t e_flag;
#define EPROC_CTTY      0x01                                      
#define EPROC_SLEADER   0x02                        
#define COMAPT_MAXLOGNAME       12
		char    e_login[COMAPT_MAXLOGNAME];                                
		int32_t e_spare[4];
	} kp_eproc;
};

#endif                                                      

#ifdef BSD_KERNEL_PRIVATE
#include <sys/proc_internal.h>

                                        
                                                 
                                     
   

struct user32_pcred {
	char          pc_lock[72];                        
	user32_addr_t pc_ucred;                                 
	uid_t         p_ruid;                            
	uid_t         p_svuid;                                      
	gid_t         p_rgid;                             
	gid_t         p_svgid;                                       
	int           p_refcnt;                                  
};
struct user64_pcred {
	char          pc_lock[72];                          
	user64_addr_t pc_ucred;                                   
	uid_t         p_ruid;                              
	uid_t         p_svuid;                                        
	gid_t         p_rgid;                               
	gid_t         p_svgid;                                         
	int           p_refcnt;                                    
};

                                            
                                                 
                                         
   
struct user32_kinfo_proc {
	struct  user32_extern_proc kp_proc;                         
	struct  user32_eproc {
		user32_addr_t e_paddr;                               
		user32_addr_t e_sess;                                        
		struct  user32_pcred e_pcred;                                    
		struct  _ucred e_ucred;                                  
		struct  user32_vmspace e_vm;                    
		pid_t   e_ppid;                                        
		pid_t   e_pgid;                                       
		int     e_jobc;                                          
		dev_t   e_tdev;                                          
		pid_t   e_tpgid;                                          
		user32_addr_t   e_tsess;                                 
		char    e_wmesg[WMESGLEN + 1];                       
		segsz_t e_xsize;                               
		short   e_xrssize;                            
		short   e_xccount;                                   
		short   e_xswrss;
		int32_t e_flag;
		char    e_login[COMAPT_MAXLOGNAME];                                
		int32_t e_spare[4];
	} kp_eproc;
};
struct user64_kinfo_proc {
	struct  user64_extern_proc kp_proc;                         
	struct  user64_eproc {
		user_addr_t e_paddr;                                 
		user_addr_t e_sess;                                          
		struct  user64_pcred e_pcred;                                    
		struct  _ucred e_ucred;                                  
		struct   user_vmspace e_vm;                    
		pid_t   e_ppid;                                        
		pid_t   e_pgid;                                       
		int     e_jobc;                                          
		dev_t   e_tdev;                                          
		pid_t   e_tpgid;                                          
		user64_addr_t   e_tsess __attribute((aligned(8)));                               
		char    e_wmesg[WMESGLEN + 1];                       
		segsz_t e_xsize;                               
		short   e_xrssize;                            
		short   e_xccount;                                   
		short   e_xswrss;
		int32_t e_flag;
		char    e_login[COMAPT_MAXLOGNAME];                                
		int32_t e_spare[4];
	} kp_eproc;
};

#endif                          

  
                       
   
#define KIPC_MAXSOCKBUF         1                                             
#define KIPC_SOCKBUF_WASTE      2                                           
#define KIPC_SOMAXCONN          3                                            
#define KIPC_MAX_LINKHDR        4                                           
#define KIPC_MAX_PROTOHDR       5                                              
#define KIPC_MAX_HDR            6                                             
#define KIPC_MAX_DATALEN        7                                     
#define KIPC_MBSTAT             8                                          
#define KIPC_NMBCLUSTERS        9                                       
#define KIPC_SOQLIMITCOMPAT     10                                   

  
                     
   
#define VM_METER        1                                   
#define VM_LOADAVG      2                                   
  
                                                                       
                                                                 
   
#define VM_MACHFACTOR   4                                                   
#define VM_SWAPUSAGE    5                                     
#define VM_MAXID        6                                           

#define CTL_VM_NAMES { \
	{ 0, 0 }, \
	{ "vmmeter", CTLTYPE_STRUCT }, \
	{ "loadavg", CTLTYPE_STRUCT }, \
	{ 0, 0 },                                               \
	{ "dummy", CTLTYPE_INT }, \
	{ "swapusage", CTLTYPE_STRUCT } \
}

struct xsw_usage {
	u_int64_t       xsu_total;
	u_int64_t       xsu_avail;
	u_int64_t       xsu_used;
	u_int32_t       xsu_pagesize;
	boolean_t       xsu_encrypted;
};

#ifdef __APPLE_API_PRIVATE
                                                                            
                                                                               
struct loadavg {
	fixpt_t ldavg[3];
	long    fscale;
};
extern struct loadavg averunnable;
#define LSCALE  1000                                                      

#ifdef BSD_KERNEL_PRIVATE

struct user32_loadavg {
	fixpt_t ldavg[3];
	user32_long_t       fscale;
};

struct user64_loadavg {
	fixpt_t ldavg[3];
	user64_long_t       fscale;
};

#endif                          
#endif                          


  
                     
   
#define HW_MACHINE       1                                                                      
#define HW_MODEL         2                                                                              
#define HW_NCPU          3                                       
#define HW_BYTEORDER     4                                           
#define HW_PHYSMEM       5                                     
#define HW_USERMEM       6                                          
#define HW_PAGESIZE      7                                           
#define HW_DISKNAMES     8                                             
#define HW_DISKSTATS     9                                       
#define HW_EPOCH        10                                                    
#define HW_FLOATINGPT   11                                               
#define HW_MACHINE_ARCH 12                                                
#define HW_VECTORUNIT   13                                            
#define HW_BUS_FREQ     14                                      
#define HW_CPU_FREQ     15                                      
#define HW_CACHELINE    16                                                 
#define HW_L1ICACHESIZE 17                                                 
#define HW_L1DCACHESIZE 18                                                 
#define HW_L2SETTINGS   19                                          
#define HW_L2CACHESIZE  20                                               
#define HW_L3SETTINGS   21                                          
#define HW_L3CACHESIZE  22                                               
#define HW_TB_FREQ      23                                      
#define HW_MEMSIZE      24                                               
#define HW_AVAILCPU     25                                                 
#define HW_TARGET       26                                            
#define HW_PRODUCT      27                                              
#define HW_MAXID        28                                          

#define CTL_HW_NAMES { \
	{ 0, 0 }, \
	{ "machine", CTLTYPE_STRING },                                           \
	{ "model", CTLTYPE_STRING },                                            \
	{ "ncpu", CTLTYPE_INT }, \
	{ "byteorder", CTLTYPE_INT }, \
	{ "physmem", CTLTYPE_INT }, \
	{ "usermem", CTLTYPE_INT }, \
	{ "pagesize", CTLTYPE_INT }, \
	{ "disknames", CTLTYPE_STRUCT }, \
	{ "diskstats", CTLTYPE_STRUCT }, \
	{ "epoch", CTLTYPE_INT }, \
	{ "floatingpoint", CTLTYPE_INT }, \
	{ "machinearch", CTLTYPE_STRING }, \
	{ "vectorunit", CTLTYPE_INT }, \
	{ "busfrequency", CTLTYPE_INT }, \
	{ "cpufrequency", CTLTYPE_INT }, \
	{ "cachelinesize", CTLTYPE_INT }, \
	{ "l1icachesize", CTLTYPE_INT }, \
	{ "l1dcachesize", CTLTYPE_INT }, \
	{ "l2settings", CTLTYPE_INT }, \
	{ "l2cachesize", CTLTYPE_INT }, \
	{ "l3settings", CTLTYPE_INT }, \
	{ "l3cachesize", CTLTYPE_INT }, \
	{ "tbfrequency", CTLTYPE_INT }, \
	{ "memsize", CTLTYPE_QUAD }, \
	{ "availcpu", CTLTYPE_INT }, \
	{ "target", CTLTYPE_STRING }, \
	{ "product", CTLTYPE_STRING }, \
}

  
                                                        
  
                                                                                                                           
                                           
  
                                                                                      
  
                                                                                                    
                                                                                                                          
  
                                                                                                    
                                                                                                                     
                                                                                                
  
                                                                                                                  
                                                                                                            
  
                                                                                                                 
                                                                                                           
  
                                                                                                                           
                                                                                                                      
                                                                                               
  
                                                                                                                      
                                                                                                                             
                                                           
  
                                                                                                                      
                                                                                                                             
                                                           
  
                                                                                                                              
                                                                                                                               
                                                                                                                            
                                                                                        
                                                                                                                                
                                                                                                                                
                                                                           
  
                                                                                                              
  
                                                                                                          
  
                                                                                        
                                                                                                                             
                                                          
  
                                                                                                                               
                                                                         
                                
                                
  
                                                                        
  
                                                                                                                                 
                                                                                                                                     
                                                                                                                             
  
               
  
                                                            
                                                     
                                                    
                                                    
                                                                          
                                                                                          
                                                                      
                                                          
                                                                               
  
                        
  
                                                                
                                                                     
                                                              
                                                                
                                                                
                                                                             
                                                   
   


  
                       
   
#define USER_CS_PATH             1                            
#define USER_BC_BASE_MAX         2                            
#define USER_BC_DIM_MAX          3                           
#define USER_BC_SCALE_MAX        4                             
#define USER_BC_STRING_MAX       5                              
#define USER_COLL_WEIGHTS_MAX    6                                 
#define USER_EXPR_NEST_MAX       7                              
#define USER_LINE_MAX            8                         
#define USER_RE_DUP_MAX          9                           
#define USER_POSIX2_VERSION     10                               
#define USER_POSIX2_C_BIND      11                              
#define USER_POSIX2_C_DEV       12                             
#define USER_POSIX2_CHAR_TERM   13                                 
#define USER_POSIX2_FORT_DEV    14                                
#define USER_POSIX2_FORT_RUN    15                                
#define USER_POSIX2_LOCALEDEF   16                                 
#define USER_POSIX2_SW_DEV      17                              
#define USER_POSIX2_UPE         18                           
#define USER_STREAM_MAX         19                                  
#define USER_TZNAME_MAX         20                                  
#define USER_MAXID              21                                    

#define CTL_USER_NAMES { \
	{ 0, 0 }, \
	{ "cs_path", CTLTYPE_STRING }, \
	{ "bc_base_max", CTLTYPE_INT }, \
	{ "bc_dim_max", CTLTYPE_INT }, \
	{ "bc_scale_max", CTLTYPE_INT }, \
	{ "bc_string_max", CTLTYPE_INT }, \
	{ "coll_weights_max", CTLTYPE_INT }, \
	{ "expr_nest_max", CTLTYPE_INT }, \
	{ "line_max", CTLTYPE_INT }, \
	{ "re_dup_max", CTLTYPE_INT }, \
	{ "posix2_version", CTLTYPE_INT }, \
	{ "posix2_c_bind", CTLTYPE_INT }, \
	{ "posix2_c_dev", CTLTYPE_INT }, \
	{ "posix2_char_term", CTLTYPE_INT }, \
	{ "posix2_fort_dev", CTLTYPE_INT }, \
	{ "posix2_fort_run", CTLTYPE_INT }, \
	{ "posix2_localedef", CTLTYPE_INT }, \
	{ "posix2_sw_dev", CTLTYPE_INT }, \
	{ "posix2_upe", CTLTYPE_INT }, \
	{ "stream_max", CTLTYPE_INT }, \
	{ "tzname_max", CTLTYPE_INT } \
}



  
                        
  
                                                          
                                                             
   
#define CTL_DEBUG_NAME          0                                  
#define CTL_DEBUG_VALUE         1                                
#define CTL_DEBUG_MAXID         20


#if (CTL_MAXID != 9) || (KERN_MAXID != 72) || (VM_MAXID != 6) || (HW_MAXID != 28) || (USER_MAXID != 21) || (CTL_DEBUG_MAXID != 20)
#error Use the SYSCTL_*() macros and OID_AUTO instead!
#endif


#ifdef  KERNEL

#ifdef BSD_KERNEL_PRIVATE
extern char     machine[];
extern char     osrelease[];
extern char     ostype[];
extern char     osversion[];
extern char     osproductversion[];
extern char     osbuild_config[];
#if defined(XNU_TARGET_OS_BRIDGE)
  
                                                      
                                                    
                                                    
   
#define MACOS_VERS_LEN 15

extern char     macosproductversion[];
extern char     macosversion[];
#endif

void    sysctl_mib_init(void);
void    hvg_bsd_init(void);

#endif                         
#else                

__BEGIN_DECLS
int     sysctl(int *, u_int, void *, size_t *, void *, size_t);
int     sysctlbyname(const char *, void *, size_t *, void *, size_t);
int     sysctlnametomib(const char *, int *, size_t *);
__END_DECLS

#endif              


#endif                         


#endif                       
