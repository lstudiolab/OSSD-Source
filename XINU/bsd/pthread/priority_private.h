/*
 * Copyright (c) 2000-2017 Apple Computer, Inc. All rights reserved.
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

#ifndef _PTHREAD_PRIORITY_PRIVATE_H_
#define _PTHREAD_PRIORITY_PRIVATE_H_

   
                              
  
            
                                                                              
                                                                          
                                                     
  
                                          
  
                                           
                                                                              
                                      
  
                                         
                                                                            
                                                                               
           
  
                                                                         
  
                                              
                                                                           
                                                                             
                                                                              
               
  
                                       
  
                                          
                                          
                                                    
                                                                             
                                   
  
                                                                              
                                                                            
                                                                            
                                                       
  
                                             
                                                                           
                                                        
  
                             
  
                                        
                                                                          
                         
  
                                          
                                                                          
                         
  
                                        
                                                                               
                                                                              
                        
  
                                         
                 
   
typedef unsigned long pthread_priority_t;

#define _PTHREAD_PRIORITY_FLAGS_MASK                    0xff000000u
#define _PTHREAD_PRIORITY_FLAGS_SHIFT                   (24ull)

#define _PTHREAD_PRIORITY_OVERCOMMIT_FLAG               0x80000000u
#define _PTHREAD_PRIORITY_INHERIT_FLAG                  0x40000000u                    
#define _PTHREAD_PRIORITY_ROOTQUEUE_FLAG                0x20000000u                    
#define _PTHREAD_PRIORITY_SCHED_PRI_FLAG                0x20000000u
#define _PTHREAD_PRIORITY_SCHED_PRI_MASK                0x0000ffffu
#define _PTHREAD_PRIORITY_ENFORCE_FLAG                  0x10000000u                    
#define _PTHREAD_PRIORITY_OVERRIDE_FLAG                 0x08000000u             
#define _PTHREAD_PRIORITY_FALLBACK_FLAG                 0x04000000u
#define _PTHREAD_PRIORITY_EVENT_MANAGER_FLAG            0x02000000u
#define _PTHREAD_PRIORITY_NEEDS_UNBIND_FLAG             0x01000000u
#define _PTHREAD_PRIORITY_DEFAULTQUEUE_FLAG             _PTHREAD_PRIORITY_FALLBACK_FLAG          

#define _PTHREAD_PRIORITY_ENCODING_MASK                 0x00a00000u
#define _PTHREAD_PRIORITY_ENCODING_SHIFT                (22ull)
#define _PTHREAD_PRIORITY_ENCODING_V0                   0x00000000u
#define _PTHREAD_PRIORITY_ENCODING_V1                   0x00400000u             
#define _PTHREAD_PRIORITY_ENCODING_V2                   0x00800000u             
#define _PTHREAD_PRIORITY_ENCODING_V3                   0x00a00000u             

#define _PTHREAD_PRIORITY_QOS_CLASS_MASK                0x003fff00u
#define _PTHREAD_PRIORITY_VALID_QOS_CLASS_MASK          0x00003f00u
#define _PTHREAD_PRIORITY_QOS_CLASS_SHIFT               (8ull)

#define _PTHREAD_PRIORITY_PRIORITY_MASK                 0x000000ffu
#define _PTHREAD_PRIORITY_PRIORITY_SHIFT                (0)

#if PRIVATE
#if XNU_KERNEL_PRIVATE && !defined(__PTHREAD_EXPOSE_INTERNALS__)
#define __PTHREAD_EXPOSE_INTERNALS__ 1
#endif                      
#ifdef __PTHREAD_EXPOSE_INTERNALS__
  
                                                        
                                                  
   
#include <mach/thread_policy.h>                
#include <stdbool.h>

                                                            
                                                                 
typedef unsigned int pthread_priority_compact_t;

__attribute__((always_inline, const))
static inline bool
_pthread_priority_has_qos(pthread_priority_t pp)
{
	return (pp & (_PTHREAD_PRIORITY_SCHED_PRI_FLAG |
	       _PTHREAD_PRIORITY_EVENT_MANAGER_FLAG)) == 0 &&
	       (pp & (_PTHREAD_PRIORITY_QOS_CLASS_MASK &
	       ~_PTHREAD_PRIORITY_VALID_QOS_CLASS_MASK)) == 0 &&
	       (pp & _PTHREAD_PRIORITY_VALID_QOS_CLASS_MASK) != 0;
}

__attribute__((always_inline, const))
static inline pthread_priority_compact_t
_pthread_priority_make_from_thread_qos(thread_qos_t qos, int relpri,
    unsigned long flags)
{
	pthread_priority_compact_t pp = (flags & _PTHREAD_PRIORITY_FLAGS_MASK);
	if (qos && qos < THREAD_QOS_LAST) {
		pp |= (1 << (_PTHREAD_PRIORITY_QOS_CLASS_SHIFT + qos - 1));
		pp |= ((uint8_t)relpri - 1) & _PTHREAD_PRIORITY_PRIORITY_MASK;
	}
	return pp;
}

__attribute__((always_inline, const))
static inline pthread_priority_compact_t
_pthread_event_manager_priority(void)
{
	return _PTHREAD_PRIORITY_EVENT_MANAGER_FLAG;
}

__attribute__((always_inline, const))
static inline pthread_priority_compact_t
_pthread_unspecified_priority(void)
{
	return _pthread_priority_make_from_thread_qos(THREAD_QOS_UNSPECIFIED, 0, 0);
}

__attribute__((always_inline, const))
static inline pthread_priority_compact_t
_pthread_default_priority(unsigned long flags)
{
	return _pthread_priority_make_from_thread_qos(THREAD_QOS_LEGACY, 0, flags);
}

__attribute__((always_inline, const))
static inline thread_qos_t
_pthread_priority_thread_qos_fast(pthread_priority_t pp)
{
	pp &= _PTHREAD_PRIORITY_QOS_CLASS_MASK;
	pp >>= _PTHREAD_PRIORITY_QOS_CLASS_SHIFT;
	return (thread_qos_t)__builtin_ffs((int)pp);
}

__attribute__((always_inline, const))
static inline thread_qos_t
_pthread_priority_thread_qos(pthread_priority_t pp)
{
	if (_pthread_priority_has_qos(pp)) {
		return _pthread_priority_thread_qos_fast(pp);
	}
	return THREAD_QOS_UNSPECIFIED;
}

__attribute__((always_inline, const))
static inline int
_pthread_priority_relpri(pthread_priority_t pp)
{
	if (_pthread_priority_has_qos(pp)) {
		pp &= _PTHREAD_PRIORITY_PRIORITY_MASK;
		pp >>= _PTHREAD_PRIORITY_PRIORITY_SHIFT;
		return (int8_t)pp + 1;
	}
	return 0;
}

#if KERNEL
                                                                       

  
                                                                     
                                    
   
__attribute__((const))
pthread_priority_compact_t
_pthread_priority_normalize(pthread_priority_t pp);

  
                                         
                                    
   
__attribute__((const))
pthread_priority_compact_t
_pthread_priority_normalize_for_ipc(pthread_priority_t pp);

  
                                                                                
                                                                   
   
__attribute__((const))
pthread_priority_compact_t
_pthread_priority_combine(pthread_priority_t base_pp, thread_qos_t qos);

#endif          
#endif                                
#endif           
#endif                                
