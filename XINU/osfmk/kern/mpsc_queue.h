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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef _KERN_MPSC_QUEUE_H_
#define _KERN_MPSC_QUEUE_H_

#ifdef XNU_KERNEL_PRIVATE

#include <machine/atomic.h>
#include <kern/macro_help.h>
#include <kern/thread_call.h>

#endif                      

#include <sys/cdefs.h>

__BEGIN_DECLS

   
                                   
  
         
                                                      
   
typedef struct mpsc_queue_chain {
	struct mpsc_queue_chain *_Atomic mpqc_next;
} *mpsc_queue_chain_t;

   
                                  
  
         
                                                       
  
              
                                                                               
                                                                           
                                                                              
                                         
  
                                                                         
                                                                 
                                                                            
                                       
  
                                                            
                                                                             
  
                                                                          
                                                                          
                  
  
                     
  
                                                                             
                                                                             
                                    
  
                                                                        
                                                                  
  
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
  
  
                                                                             
                                                                           
                                                               
  
                                                                              
                                                                            
  
                                                                            
                                                                          
                                                                           
                                 
   
typedef struct mpsc_queue_head {
	struct mpsc_queue_chain mpqh_head;
	struct mpsc_queue_chain *_Atomic mpqh_tail;
} *mpsc_queue_head_t;

   
                                
  
         
                                                       
  
              
                                          
   
#define MPSC_QUEUE_INITIALIZER(head)   { .mpqh_tail = &(head).mpqh_head }

#ifdef XNU_KERNEL_PRIVATE

   
                            
  
         
                                        
  
              
                                                                     
                                                                       
  
           
                           
   
static inline void
mpsc_queue_init(mpsc_queue_head_t q)
{
	os_atomic_init(&q->mpqh_head.mpqc_next, NULL);
	os_atomic_init(&q->mpqh_tail, &q->mpqh_head);
}

   
                                   
   
typedef enum mpsc_queue_options {
	MPSC_QUEUE_NONE                = 0,
	MPSC_QUEUE_DISABLE_PREEMPTION  = 1 << 0,
} mpsc_queue_options_t;

   
                                     
  
         
                                                                             
                                  
   
#define MPSC_QUEUE_NOTQUEUED_MARKER ((mpsc_queue_chain_t)~0ul)

   
                            
  
         
                                                                            
   
#define mpsc_queue_element(ptr, type, field) __container_of(ptr, type, field)


#pragma mark Advanced Multi Producer calls

   
                                            
  
         
                                                                            
         
  
              
                                                                            
                                                                       
  
                                               
                                               
                                                  
  
           
                       
  
             
                                
  
           
                                                             
                           
   
static inline mpsc_queue_chain_t
__mpsc_queue_append_update_tail(mpsc_queue_head_t q, mpsc_queue_chain_t elm)
{
	os_atomic_store(&elm->mpqc_next, NULL, relaxed);
	return os_atomic_xchg(&q->mpqh_tail, elm, release);
}

   
                                          
  
         
                                                
                                                
  
           
                                   
  
              
                                                           
  
           
                                                     
   
static inline bool
__mpsc_queue_append_was_empty(mpsc_queue_head_t q, mpsc_queue_chain_t prev)
{
	return &q->mpqh_head == prev;
}

   
                                            
  
         
                                                                             
         
  
              
                                                                            
                                                                       
  
                                               
                                               
                                                  
  
              
                                                           
  
             
                                      
   
static inline void
__mpsc_queue_append_update_prev(mpsc_queue_chain_t prev, mpsc_queue_chain_t elm)
{
	os_atomic_store(&prev->mpqc_next, elm, relaxed);
}


#pragma mark Multi Producer calls

   
                                   
  
         
                                            
  
              
                                                                          
                     
  
                                                                       
  
           
                       
  
               
                                                 
  
              
                                                
   
static inline bool
mpsc_queue_append_list(mpsc_queue_head_t q, mpsc_queue_chain_t first,
    mpsc_queue_chain_t last)
{
	mpsc_queue_chain_t prev = __mpsc_queue_append_update_tail(q, last);
	__mpsc_queue_append_update_prev(prev, first);
	return __mpsc_queue_append_was_empty(q, prev);
}

   
                                            
  
         
                                    
  
              
                                                                  
  
                                  
                                    
   
static inline bool
mpsc_queue_append(mpsc_queue_head_t q, mpsc_queue_chain_t elm)
{
	return mpsc_queue_append_list(q, elm, elm);
}


#pragma mark Single Consumer calls

   
                                       
  
         
                                                                       
  
              
                                                                            
                           
  
           
            
  
              
                                                        
  
                    
                                                                  
                                                                              
  
           
                                                                      
   
mpsc_queue_chain_t
mpsc_queue_dequeue_batch(mpsc_queue_head_t q, mpsc_queue_chain_t *tail,
    os_atomic_dependency_t dependency);

   
                                    
  
         
                                                                 
                              
  
              
                                                                             
                         
  
                                                                        
                   
  
             
                                                                        
                                                
  
              
                                 
  
           
                           
   
mpsc_queue_chain_t
mpsc_queue_batch_next(mpsc_queue_chain_t cur, mpsc_queue_chain_t tail);

   
                                       
  
         
                                                                            
  
              
                                    
  
              
                                  
  
              
                                 
   
#define mpsc_queue_batch_foreach_safe(item, head, tail) \
	        for (mpsc_queue_chain_t __tmp, __item = (head), __tail = (tail); \
	                        __tmp = mpsc_queue_batch_next(__item, __tail), (item) = __item; \
	                        __item = __tmp)

   
                                       
  
         
                                               
  
              
                                                                            
                           
  
           
            
  
               
                                 
  
              
                                
                                                                              
                                                         
   
void
mpsc_queue_restore_batch(mpsc_queue_head_t q, mpsc_queue_chain_t first,
    mpsc_queue_chain_t last);


#pragma mark "GCD"-like facilities

   
                                    
  
         
                                                                         
             
  
              
                                                                       
                                                   
  
                                                                               
                                                                          
                                                           
  
                                                                        
                                                                              
   
typedef struct mpsc_daemon_queue *mpsc_daemon_queue_t;

   
                                    
  
         
                                                    
   
typedef void (*mpsc_daemon_invoke_fn_t)(mpsc_queue_chain_t elm,
    mpsc_daemon_queue_t dq);

   
                               
  
         
                                            
   
typedef enum mpsc_daemon_queue_kind {
	MPSC_QUEUE_KIND_UNKNOWN,
	MPSC_QUEUE_KIND_NESTED,
	MPSC_QUEUE_KIND_THREAD,
	MPSC_QUEUE_KIND_THREAD_CRITICAL,
	MPSC_QUEUE_KIND_THREAD_CALL,
} mpsc_daemon_queue_kind_t;

   
                                
  
         
                                            
   
__options_decl(mpsc_daemon_queue_state_t, uint32_t, {
	MPSC_QUEUE_STATE_DRAINING = 0x0001,
	MPSC_QUEUE_STATE_WAKEUP   = 0x0002,
	MPSC_QUEUE_STATE_CANCELED = 0x0004,
});

struct mpsc_daemon_queue {
	mpsc_daemon_queue_kind_t    mpd_kind;
	mpsc_daemon_queue_state_t _Atomic mpd_state;
	mpsc_daemon_invoke_fn_t     mpd_invoke;
	union {
		mpsc_daemon_queue_t     mpd_target;
		struct thread          *mpd_thread;
		struct thread_call     *mpd_call;
	};
	struct mpsc_queue_head      mpd_queue;
	struct mpsc_queue_chain     mpd_chain;
};

   
                                               
  
         
                                                                        
  
              
                                                                     
  
            
                          
  
                
                                                                            
  
             
                                                 
  
              
                                          
  
           
                                              
   
kern_return_t
mpsc_daemon_queue_init_with_thread(mpsc_daemon_queue_t dq,
    mpsc_daemon_invoke_fn_t invoke, int pri, const char *name);


   
                                                    
  
         
                                                                      
  
            
                          
  
                
                                                                            
  
             
                                            
   
void
mpsc_daemon_queue_init_with_thread_call(mpsc_daemon_queue_t dq,
    mpsc_daemon_invoke_fn_t invoke, thread_call_priority_t pri);

   
                                               
  
         
                                                         
  
              
                                                                              
                                                                              
                                             
  
            
                          
  
                
                                                                            
  
                
                                                                              
                                                      
   
void
mpsc_daemon_queue_init_with_target(mpsc_daemon_queue_t dq,
    mpsc_daemon_invoke_fn_t invoke, mpsc_daemon_queue_t target);

   
                                            
  
         
                                                                                
                                  
   
void
mpsc_daemon_queue_nested_invoke(mpsc_queue_chain_t elm,
    mpsc_daemon_queue_t dq);

   
                                              
  
         
                                                                   
  
              
                                                                      
                                                                           
            
  
                                                                      
  
                                                     
  
                                                                              
   
void
mpsc_daemon_queue_cancel_and_wait(mpsc_daemon_queue_t dq);

   
                                
  
         
                                                             
  
              
                                                                               
                     
  
            
                                                
  
             
                       
  
                 
                                                           
                                                                           
                      
   
void
mpsc_daemon_enqueue(mpsc_daemon_queue_t dq, mpsc_queue_chain_t elm,
    mpsc_queue_options_t options);


#pragma mark Deferred deallocation daemon

   
                                          
  
         
                                                                                
  
              
                                                                              
                                       
  
                                                                               
                                                                             
  
                                                                        
  
           
                                                             
                                                             
   
void
thread_deallocate_daemon_init(void);

   
                                                    
  
         
                                                                       
                       
  
            
                                                                      
  
                
                                                                         
          
   
void
thread_deallocate_daemon_register_queue(mpsc_daemon_queue_t dq,
    mpsc_daemon_invoke_fn_t invoke);


#pragma mark tests
#if DEBUG || DEVELOPMENT

int
mpsc_test_pingpong(uint64_t count, uint64_t *out);

#endif                           

#endif                         

__END_DECLS

#endif                          
