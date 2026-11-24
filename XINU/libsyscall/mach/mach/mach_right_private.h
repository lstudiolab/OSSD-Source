/*
 * Copyright (c) 2018 Apple Inc. All rights reserved.
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
#ifndef __MACH_RIGHT_PRIVATE_H
#define __MACH_RIGHT_PRIVATE_H

#include <os/base.h>
#include <mach/mach.h>
#include <mach/port.h>
#include <mach/mach_port.h>
#include <sys/cdefs.h>
#include <stdbool.h>

__BEGIN_DECLS;

   
                             
                                                        
   
typedef struct _mach_right_recv {
	mach_port_t mrr_name;
} mach_right_recv_t;

   
                              
                                                        
   
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define MACH_RIGHT_RECV_NULL ((mach_right_recv_t){MACH_PORT_NULL})
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define MACH_RIGHT_RECV_NULL (mach_right_recv_t{MACH_PORT_NULL})
#elif defined(__cplusplus)
#define MACH_RIGHT_RECV_NULL \
	        (mach_right_recv_t((mach_right_recv_t){MACH_PORT_NULL}))
#else
#define MACH_RIGHT_RECV_NULL {MACH_PORT_NULL}
#endif

   
                             
                                                   
   
typedef struct _mach_right_send {
	mach_port_t mrs_name;
} mach_right_send_t;

   
                              
                                                     
   
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define MACH_RIGHT_SEND_NULL ((mach_right_send_t){MACH_PORT_NULL})
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define MACH_RIGHT_SEND_NULL (mach_right_send_t{MACH_PORT_NULL})
#elif defined(__cplusplus)
#define MACH_RIGHT_SEND_NULL \
	        (mach_right_send_t((mach_right_send_t){MACH_PORT_NULL}))
#else
#define MACH_RIGHT_SEND_NULL {MACH_PORT_NULL}
#endif

   
                                  
                                                        
   
typedef struct _mach_right_send_once {
	mach_port_t mrso_name;
} mach_right_send_once_t;

   
                                   
                                                          
   
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define MACH_RIGHT_SEND_ONCE_NULL ((mach_right_send_once_t){MACH_PORT_NULL})
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define MACH_RIGHT_SEND_ONCE_NULL (mach_right_send_once_t{MACH_PORT_NULL})
#elif defined(__cplusplus)
#define MACH_RIGHT_SEND_ONCE_NULL \
	        (mach_right_send_once_t((mach_right_send_once_t){MACH_PORT_NULL}))
#else
#define MACH_RIGHT_SEND_ONCE_NULL {MACH_PORT_NULL}
#endif

   
                            
                                               
  
            
                                                                               
                                                                  
  
          
                              
   
OS_ALWAYS_INLINE OS_WARN_RESULT
static inline mach_right_recv_t
mach_right_recv(mach_port_name_t pn)
{
	mach_right_recv_t mrr = {pn};
	return mrr;
}

   
                            
                                            
  
            
                                                                                
                                                              
  
          
                           
   
OS_ALWAYS_INLINE OS_WARN_RESULT
static inline mach_right_send_t
mach_right_send(mach_port_name_t pn)
{
	mach_right_send_t mrs = {pn};
	return mrs;
}

   
                                  
                                                  
  
             
                                  
  
          
                                                   
   
OS_ALWAYS_INLINE OS_WARN_RESULT
static inline bool
mach_right_send_valid(mach_right_send_t mrs)
{
	return MACH_PORT_VALID(mrs.mrs_name);
}

   
                                 
                                                 
  
            
                                                                          
                                                                              
  
          
                                
   
OS_ALWAYS_INLINE OS_WARN_RESULT
static inline mach_right_send_once_t
mach_right_send_once(mach_port_name_t pn)
{
	mach_right_send_once_t mrso = {pn};
	return mrso;
}

   
                                       
                                                       
  
              
                                       
  
          
                                                   
   
OS_ALWAYS_INLINE OS_WARN_RESULT
static inline bool
mach_right_send_once_valid(mach_right_send_once_t mrso)
{
	return MACH_PORT_VALID(mrso.mrso_name);
}

   
                              
                                                             
  
                              
                                                                    
  
                                   
                                                                          
               
   
OS_ENUM(mach_right_flags, uint64_t,
    MACH_RIGHT_RECV_FLAG_INIT = 0,
    MACH_RIGHT_RECV_FLAG_UNGUARDED = (1 << 0),
    );

   
                                      
                                                                         
  
               
                                                   
  
            
                                                                     
                                                                               
                                                   
  
             
                                                                               
                                                                                
                                                
                                          
  
          
                                                                            
                                                                    
                                    
  
              
                                                                                
                                                                            
                   
   
OS_EXPORT OS_WARN_RESULT
mach_right_recv_t
mach_right_recv_construct(mach_right_flags_t flags,
    mach_right_send_t *_Nullable sr, uintptr_t ctx);

   
                                     
                                                          
  
           
                                                
  
           
                                                                               
                                                                              
                                                                               
                                          
  
             
                                                                           
                                                                              
           
  
              
                                                                              
     
  
                                                        
                                  
  
                                                                     
  
                                                                                
                                                                           
                             
   
OS_EXPORT
void
mach_right_recv_destruct(mach_right_recv_t r, mach_right_send_t *_Nullable s,
    uintptr_t ctx);

   
                                   
                                                                          
  
           
                                                                     
  
          
                                                                             
                                                        
  
                                                                              
                           
  
              
                                                                            
                                                                             
                                 
   
OS_EXPORT OS_WARN_RESULT
mach_right_send_t
mach_right_send_create(mach_right_recv_t r);

   
                                   
                                                                
  
           
                                
  
          
                                                                              
                                                                            
                                                                          
                                                                           
  
                                                                               
   
OS_EXPORT OS_WARN_RESULT
mach_right_send_t
mach_right_send_retain(mach_right_send_t s);

   
                                    
                                                                
  
           
                                
  
              
                                                                             
                                                                 
  
                                                                               
   
OS_EXPORT
void
mach_right_send_release(mach_right_send_t s);

   
                                        
                                                          
  
           
                                                                          
  
          
                                     
  
              
                                                                           
                                                                               
                                                                                
                            
  
                                                                              
                                                                         
                                                                        
  
                                                                           
                                    
   
OS_EXPORT OS_WARN_RESULT
mach_right_send_once_t
mach_right_send_once_create(mach_right_recv_t r);

   
                                         
                                      
  
            
                                     
  
              
                                                                               
                                                                    
  
                                                                               
  
                                                                            
                                                                               
                                        
   
OS_EXPORT
void
mach_right_send_once_consume(mach_right_send_once_t so);

__END_DECLS;

#endif                          
