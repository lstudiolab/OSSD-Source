/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
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

#ifndef _OS_REFCNT_H_
#define _OS_REFCNT_H_

  
                                   
  
                                                                                 
                                                                                 
                                                                       
                                         
  
                                                                          
                                                                              
                                                                                  
                                                                              
                                                                      
                                                                                  
                                                                                  
                                          
   

#include <stdatomic.h>
#include <stdbool.h>
#include <os/base.h>

struct os_refcnt;
struct os_refgrp;
typedef struct os_refcnt os_refcnt_t;

                                  
typedef uint32_t os_ref_count_t;
typedef _Atomic(os_ref_count_t) os_ref_atomic_t;

  
                     
                            
  
                                                                                  
                                                                                  
   

#ifndef KERNEL
# include <stdlib.h>
# include <stdio.h>
# ifndef __improbable
#  define __improbable(x) x
# endif
# ifndef panic
#  define panic(x, ...) do { fprintf(stderr, x, __VA_ARGS__); abort(); } while (0)
# endif
#endif

#ifndef OS_REFCNT_DEBUG
# if DEVELOPMENT || DEBUG
#  define OS_REFCNT_DEBUG 1
# else
#  define OS_REFCNT_DEBUG 0
# endif
#endif

#if __has_attribute(diagnose_if)
# define os_error_if(cond, msg) __attribute__((diagnose_if((cond), (msg), "error")))
#else
# define os_error_if(...)
#endif

__BEGIN_DECLS

  
                                                         
                                                                        
   
#define os_ref_init(rc, grp) os_ref_init_count((rc), (grp), 1)
static void os_ref_init_count(struct os_refcnt *, struct os_refgrp *, os_ref_count_t count)
os_error_if(count == 0, "Reference count must be non-zero initialized");

  
                                                                                
                                        
   

  
  
                                                                                  
  
                                                                                 
                                                                            
                                                                             
                                                                        
                                                     
  
                                                                                     
                                                                              
                                                      
  
                                                                                      
   
static void os_ref_retain(struct os_refcnt *);
static os_ref_count_t os_ref_release(struct os_refcnt *) OS_WARN_RESULT;
static os_ref_count_t os_ref_release_relaxed(struct os_refcnt *) OS_WARN_RESULT;
static void os_ref_release_live(struct os_refcnt *);

  
                                                                              
                                                                          
                                                                          
                                                                             
                                                                          
                                                   
   
static bool os_ref_retain_try(struct os_refcnt *) OS_WARN_RESULT;

  
                                                                             
                                                                          
                                              
  
                                                                              
         
   
static void os_ref_retain_locked(struct os_refcnt *);
static os_ref_count_t os_ref_release_locked(struct os_refcnt *) OS_WARN_RESULT;

  
                                                                           
                    
   
static os_ref_count_t os_ref_get_count(struct os_refcnt *rc);


#if XNU_KERNEL_PRIVATE
#pragma GCC visibility push(hidden)

  
                                                                            
                                                                             
                                                                         
                
   

#define os_ref_init_raw(rc, grp) os_ref_init_count_raw((rc), (grp), 1)
static void os_ref_init_count_raw(os_ref_atomic_t *, struct os_refgrp *, os_ref_count_t count)
os_error_if(count == 0, "Reference count must be non-zero initialized");
static void os_ref_retain_raw(os_ref_atomic_t *, struct os_refgrp *);
static os_ref_count_t os_ref_release_raw(os_ref_atomic_t *, struct os_refgrp *) OS_WARN_RESULT;
static os_ref_count_t os_ref_release_raw_relaxed(os_ref_atomic_t *, struct os_refgrp *) OS_WARN_RESULT;
static void os_ref_release_live_raw(os_ref_atomic_t *, struct os_refgrp *);
static bool os_ref_retain_try_raw(os_ref_atomic_t *, struct os_refgrp *) OS_WARN_RESULT;
static void os_ref_retain_locked_raw(os_ref_atomic_t *, struct os_refgrp *);
static os_ref_count_t os_ref_release_locked_raw(os_ref_atomic_t *, struct os_refgrp *) OS_WARN_RESULT;
static os_ref_count_t os_ref_get_count_raw(os_ref_atomic_t *rc);


  
                                                                                  
                                                                                 
                                                                       
  
                                                                    
                                                                                 
                       
  
                                                                              
                                                                          
            
  
                                                                                
                                                                              
                                                                                
           
  
                                                                     
                                                                     
                                         
   

                                                      
#define os_ref_init_mask(rc, b, grp, bits) os_ref_init_count_mask((rc), (b), (grp), 1, bits)
void os_ref_init_count_mask(os_ref_atomic_t *rc, uint32_t b, struct os_refgrp *grp,
    os_ref_count_t init_count, uint32_t init_bits)
os_error_if(init_count == 0, "Reference count must be non-zero initialized")
os_error_if(b > 26, "Bitwise reference count limited to 26 bits")
os_error_if(init_bits >= (1U << b), "Bits out of range");

static uint32_t os_ref_get_raw_mask(os_ref_atomic_t *rc);
static uint32_t os_ref_get_bits_mask(os_ref_atomic_t *rc, uint32_t b);
static os_ref_count_t os_ref_get_count_mask(os_ref_atomic_t *rc, uint32_t b);

static void
os_ref_retain_mask(os_ref_atomic_t *rc, uint32_t b, struct os_refgrp *grp);
static void
os_ref_retain_acquire_mask(os_ref_atomic_t *rc, uint32_t b, struct os_refgrp *grp);
static bool
os_ref_retain_try_mask(os_ref_atomic_t *, uint32_t b, uint32_t reject_mask,
    struct os_refgrp *grp) OS_WARN_RESULT;
static bool
os_ref_retain_try_acquire_mask(os_ref_atomic_t *, uint32_t b, uint32_t reject_mask,
    struct os_refgrp *grp) OS_WARN_RESULT;

static uint32_t
os_ref_release_raw_mask(os_ref_atomic_t *rc, uint32_t b, struct os_refgrp *grp) OS_WARN_RESULT;
static uint32_t
os_ref_release_raw_relaxed_mask(os_ref_atomic_t *rc, uint32_t b, struct os_refgrp *grp) OS_WARN_RESULT;
static os_ref_count_t
os_ref_release_mask(os_ref_atomic_t *rc, uint32_t b, struct os_refgrp *grp) OS_WARN_RESULT;
static os_ref_count_t
os_ref_release_relaxed_mask(os_ref_atomic_t *rc, uint32_t b, struct os_refgrp *grp) OS_WARN_RESULT;
static void
os_ref_release_live_mask(os_ref_atomic_t *rc, uint32_t b, struct os_refgrp *grp);

#pragma GCC visibility pop
#endif                         

__END_DECLS

#include <os/refcnt_internal.h>
#endif
