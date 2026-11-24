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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef _KERN_PRIORITY_QUEUE_H_
#define _KERN_PRIORITY_QUEUE_H_

#if KERNEL
#include <kern/kern_types.h>
#include <kern/macro_help.h>
#include <kern/assert.h>
#endif

#include <stdbool.h>
#include <sys/cdefs.h>

#pragma GCC visibility push(hidden)

__BEGIN_DECLS

  
                                                                         
  
                    
                                                                                          
                                                        
                                                               
  
                                                                 
                                                                
  
                                                                            
                                            
  
                 
  
                                                                        
  
                                                    
  
                                                                         
                                                          
  
                                                                               
  
                              
  
                                                                                
                                     
  
                                                                     
                                     
                                                       
                             
  
                                                                           
                                     
  
                   
  
                                                                  
                                                       
  
                                                                  
  
                                                                             
                                                                              
  
                                  
                                   
                                   
                           
                                                         
                           
                                       
                  
                                                                 
  
                                                                           
                                                                             
                                                                               
                                                                            
                                                                           
                                                                             
                                                                                
                                                                             
                    
   


  
                                                  
                                                                                         
   
#define PRIORITY_QUEUE_KEY_NONE             0
typedef uint16_t priority_queue_key_t;

#ifdef __LP64__

  
                                                                     
                                                                            
                                                                         
                                                                          
                                                                        
   
#define PRIORITY_QUEUE_ENTRY_CHILD_BITS     48
#define PRIORITY_QUEUE_ENTRY_KEY_BITS       16

typedef struct priority_queue_entry {
	struct priority_queue_entry        *next;
	struct priority_queue_entry        *prev;
	long                                __key: PRIORITY_QUEUE_ENTRY_KEY_BITS;
	long                                child: PRIORITY_QUEUE_ENTRY_CHILD_BITS;
} *priority_queue_entry_t;

typedef struct priority_queue_entry_deadline {
	struct priority_queue_entry_deadline *next;
	struct priority_queue_entry_deadline *prev;
	long                                  __key: PRIORITY_QUEUE_ENTRY_KEY_BITS;
	long                                  child: PRIORITY_QUEUE_ENTRY_CHILD_BITS;
	uint64_t                              deadline;
} *priority_queue_entry_deadline_t;

typedef struct priority_queue_entry_sched {
	struct priority_queue_entry_sched  *next;
	struct priority_queue_entry_sched  *prev;
	long                                key: PRIORITY_QUEUE_ENTRY_KEY_BITS;
	long                                child: PRIORITY_QUEUE_ENTRY_CHILD_BITS;
} *priority_queue_entry_sched_t;

typedef struct priority_queue_entry_stable {
	struct priority_queue_entry_stable *next;
	struct priority_queue_entry_stable *prev;
	long                                key: PRIORITY_QUEUE_ENTRY_KEY_BITS;
	long                                child: PRIORITY_QUEUE_ENTRY_CHILD_BITS;
	uint64_t                            stamp;
} *priority_queue_entry_stable_t;

#else               

typedef struct priority_queue_entry {
	struct priority_queue_entry        *next;
	struct priority_queue_entry        *prev;
	long                                child;
} *priority_queue_entry_t;

typedef struct priority_queue_entry_deadline {
	struct priority_queue_entry_deadline *next;
	struct priority_queue_entry_deadline *prev;
	long                                  child;
	uint64_t                              deadline;
} *priority_queue_entry_deadline_t;

  
                                                                                        
                                                                                        
                                           
   
typedef struct priority_queue_entry_sched {
	struct priority_queue_entry_sched  *next;
	struct priority_queue_entry_sched  *prev;
	long                                child;
	priority_queue_key_t                key;
} *priority_queue_entry_sched_t;

typedef struct priority_queue_entry_stable {
	struct priority_queue_entry_stable *next;
	struct priority_queue_entry_stable *prev;
	long                                child;
	priority_queue_key_t                key;
	uint64_t                            stamp;
} *priority_queue_entry_stable_t;

#endif               

  
                             
        
                             
          
                                                                                          
   
typedef int (^priority_queue_compare_fn_t)(struct priority_queue_entry *e1,
    struct priority_queue_entry *e2);

#define priority_heap_compare_ints(a, b) ((a) < (b) ? 1 : -1)

#define priority_heap_make_comparator(name1, name2, type, field, ...) \
	(^int(priority_queue_entry_t __e1, priority_queue_entry_t __e2){        \
	    type *name1 = pqe_element_fast(__e1, type, field);                  \
	    type *name2 = pqe_element_fast(__e2, type, field);                  \
	    __VA_ARGS__;                                                        \
	})

  
                                                                     
   
struct priority_queue;

  
                        
   
struct priority_queue_min {
	struct priority_queue_entry *pq_root;
	priority_queue_compare_fn_t  pq_cmp_fn;
};
struct priority_queue_max {
	struct priority_queue_entry *pq_root;
	priority_queue_compare_fn_t  pq_cmp_fn;
};

  
                         
   
struct priority_queue_deadline_min {
	struct priority_queue_entry_deadline *pq_root;
};
struct priority_queue_deadline_max {
	struct priority_queue_entry_deadline *pq_root;
};

  
                                         
   
struct priority_queue_sched_min {
	struct priority_queue_entry_sched *pq_root;
};
struct priority_queue_sched_max {
	struct priority_queue_entry_sched *pq_root;
};

  
                                                
   
struct priority_queue_sched_stable_min {
	struct priority_queue_entry_stable *pq_root;
};
struct priority_queue_sched_stable_max {
	struct priority_queue_entry_stable *pq_root;
};

#pragma mark generic interface

#define PRIORITY_QUEUE_INITIALIZER { .pq_root = NULL }

#define __pqueue_overloadable  __attribute__((overloadable))

#define priority_queue_is_min_heap(pq) _Generic(pq, \
	struct priority_queue_min *: true, \
	struct priority_queue_max *: false, \
	struct priority_queue_deadline_min *: true, \
	struct priority_queue_deadline_max *: false, \
	struct priority_queue_sched_min *: true, \
	struct priority_queue_sched_max *: false, \
	struct priority_queue_sched_stable_min *: true, \
	struct priority_queue_sched_stable_max *: false)

#define priority_queue_is_max_heap(pq) \
	(!priority_queue_is_min_heap(pq))

  
                                        
                 
                                                                            
                                                                    
                                              
  
                                                              
               
                                                 
                                                   
                                                                
                                                        
                
                                      
   
#define pqe_element_fast(qe, type, field)  __container_of(qe, type, field)

  
                                   
                 
                                                                            
                                                                    
                                              
  
                                                      
               
                                            
                                                   
                                                                
                                                        
                
                                      
   
#define pqe_element(qe, type, field)  ({                                        \
	__auto_type _tmp_entry = (qe);                                          \
	_tmp_entry ? pqe_element_fast(_tmp_entry, type, field) : ((type *)NULL);\
})

  
                                        
   

  
                                            
                 
                                                        
               
                                                  
                                                    
   
#define priority_queue_empty(pq)         ((pq)->pq_root == NULL)

  
                                           
                 
                                                       
               
                                       
                                                    
                                                               
                
                    
   
__pqueue_overloadable
extern void
priority_queue_init(struct priority_queue *pq, ...);

  
                                                 
                 
                                                   
               
                                             
                                                   
                
                    
   
#define priority_queue_entry_init(qe) \
	__builtin_bzero(qe, sizeof(*(qe)))

  
                                              
                 
                                                                                
                                                                                        
                                                                                           
                                                                                 
               
                                                                 
                                                    
                                                            
  
                
                    
   
#define priority_queue_destroy(pq, type, field, callback)                       \
MACRO_BEGIN                                                                     \
	void (^__callback)(type *) = (callback);                                \
	_priority_queue_destroy(pq, offsetof(type, field),                      \
	    (void (^)(void *))(__callback));                                    \
MACRO_END

  
                                          
                 
                                                           
  
               
                                                   
                                                    
                                                                
                                                        
                
                                     
   
#define priority_queue_min(pq, type, field) ({                                  \
	static_assert(priority_queue_is_min_heap(pq), "queue is min heap");     \
	pqe_element((pq)->pq_root, type, field);                                \
})

  
                                          
                 
                                                                   
  
               
                                                   
                                                    
                                                                
                                                        
                
                                     
   
#define priority_queue_max(pq, type, field) ({                                  \
	static_assert(priority_queue_is_max_heap(pq), "queue is max heap");     \
	pqe_element((pq)->pq_root, type, field);                                \
})

  
                                             
                 
                                                         
  
                                                                  
  
               
                                                       
                                                    
                                                    
                
                                                                
   
extern bool
priority_queue_insert(struct priority_queue *pq,
    struct priority_queue_entry *elt) __pqueue_overloadable;

  
                                                 
                 
                                                                        
               
                                                          
                                                    
                                                                
                                                        
                
                                    
   
#define priority_queue_remove_min(pq, type, field) ({                           \
	static_assert(priority_queue_is_min_heap(pq), "queue is min heap");     \
	pqe_element(_priority_queue_remove_root(pq), type, field);              \
})

  
                                                 
                 
                                                                        
               
                                                          
                                                    
                                                                
                                                        
                
                                    
   
#define priority_queue_remove_max(pq, type, field) ({                           \
	static_assert(priority_queue_is_max_heap(pq), "queue is max heap");     \
	pqe_element(_priority_queue_remove_root(pq), type, field);              \
})

  
                                             
                 
                                                          
               
                                              
                                                    
                                                    
                
                                                        
   
extern bool
priority_queue_remove(struct priority_queue *pq,
    struct priority_queue_entry *elt) __pqueue_overloadable;


  
                                                      
  
                 
                                                                                
  
                                                                         
                                               
  
               
                                                       
                                                    
                                                    
                
                                                                        
   
extern bool
priority_queue_entry_decreased(struct priority_queue *pq,
    struct priority_queue_entry *elt) __pqueue_overloadable;

  
                                                      
  
                 
                                                                                
  
                                                                         
                                               
  
               
                                                                
                                                    
                                                    
                
                                                                        
   
extern bool
priority_queue_entry_increased(struct priority_queue *pq,
    struct priority_queue_entry *elt) __pqueue_overloadable;


#pragma mark priority_queue_sched_*

__enum_decl(priority_queue_entry_sched_modifier_t, uint8_t, {
	PRIORITY_QUEUE_ENTRY_NONE      = 0,
	PRIORITY_QUEUE_ENTRY_PREEMPTED = 1,
});

#define priority_queue_is_sched_heap(pq) _Generic(pq, \
	struct priority_queue_sched_min *: true, \
	struct priority_queue_sched_max *: true, \
	struct priority_queue_sched_stable_min *: true, \
	struct priority_queue_sched_stable_max *: true, \
	default: false)

  
                                                          
  
                 
                                                                                
  
                                                          
                                             
  
               
                                                                          
                                                    
                                                    
                                     
                                                                        
   
#define priority_queue_entry_set_sched_pri(pq, elt, pri, modifier)              \
MACRO_BEGIN                                                                     \
	static_assert(priority_queue_is_sched_heap(pq), "is a sched heap");     \
	(elt)->key = (priority_queue_key_t)(((pri) << 8) + (modifier));         \
MACRO_END

  
                                                      
  
                 
                                                                         
                        
  
               
                                                       
                                                    
                                                    
  
                
                                                    
   
#define priority_queue_entry_sched_pri(pq, elt) ({                              \
	static_assert(priority_queue_is_sched_heap(pq), "is a sched heap");     \
	(priority_queue_key_t)((elt)->key >> 8);                                \
})

  
                                                           
  
                 
                                                                         
                        
  
               
                                                            
                                                    
                                                    
  
                
                                                    
   
#define priority_queue_entry_sched_modifier(pq, elt) ({                         \
	static_assert(priority_queue_is_sched_heap(pq), "is a sched heap");     \
	(priority_queue_entry_sched_modifier_t)(elt)->key;                      \
})

  
                                                    
  
                 
                                                                    
                                              
  
               
                                                
                                                    
  
                
                                                    
   
#define priority_queue_min_sched_pri(pq) ({                                     \
	static_assert(priority_queue_is_min_heap(pq), "queue is min heap");     \
	priority_queue_entry_sched_pri(pq, (pq)->pq_root);                      \
})

  
                                                    
  
                 
                                                                    
                                              
  
               
                                                
                                                    
  
                
                                                    
   
#define priority_queue_max_sched_pri(pq) ({                                     \
	static_assert(priority_queue_is_max_heap(pq), "queue is max heap");     \
	priority_queue_entry_sched_pri(pq, (pq)->pq_root);                      \
})


#pragma mark implementation details

#define PRIORITY_QUEUE_MAKE_BASE(pqueue_t, pqelem_t) \
                                                                                \
__pqueue_overloadable extern void                                               \
_priority_queue_destroy(pqueue_t pq, uintptr_t offset, void (^cb)(void *));     \
                                                                                \
__pqueue_overloadable extern bool                                               \
priority_queue_insert(pqueue_t que, pqelem_t elt);                              \
                                                                                \
__pqueue_overloadable extern pqelem_t                                           \
_priority_queue_remove_root(pqueue_t que);                                      \
                                                                                \
__pqueue_overloadable extern bool                                               \
priority_queue_remove(pqueue_t que, pqelem_t elt);                              \
                                                                                \
__pqueue_overloadable extern bool                                               \
priority_queue_entry_decreased(pqueue_t que, pqelem_t elt);                     \
                                                                                \
__pqueue_overloadable extern bool                                               \
priority_queue_entry_increased(pqueue_t que, pqelem_t elt)

#define PRIORITY_QUEUE_MAKE(pqueue_t, pqelem_t) \
__pqueue_overloadable                                                           \
static inline void                                                              \
priority_queue_init(pqueue_t que)                                               \
{                                                                               \
	__builtin_bzero(que, sizeof(*que));                                     \
}                                                                               \
                                                                                \
PRIORITY_QUEUE_MAKE_BASE(pqueue_t, pqelem_t)

#define PRIORITY_QUEUE_MAKE_CB(pqueue_t, pqelem_t) \
__pqueue_overloadable                                                           \
static inline void                                                              \
priority_queue_init(pqueue_t pq, priority_queue_compare_fn_t cmp_fn)            \
{                                                                               \
	pq->pq_root = NULL;                                                     \
	pq->pq_cmp_fn = cmp_fn;                                                 \
}                                                                               \
                                                                                \
PRIORITY_QUEUE_MAKE_BASE(pqueue_t, pqelem_t)

PRIORITY_QUEUE_MAKE_CB(struct priority_queue_min *, priority_queue_entry_t);
PRIORITY_QUEUE_MAKE_CB(struct priority_queue_max *, priority_queue_entry_t);

PRIORITY_QUEUE_MAKE(struct priority_queue_deadline_min *, priority_queue_entry_deadline_t);
PRIORITY_QUEUE_MAKE(struct priority_queue_deadline_max *, priority_queue_entry_deadline_t);

PRIORITY_QUEUE_MAKE(struct priority_queue_sched_min *, priority_queue_entry_sched_t);
PRIORITY_QUEUE_MAKE(struct priority_queue_sched_max *, priority_queue_entry_sched_t);

PRIORITY_QUEUE_MAKE(struct priority_queue_sched_stable_min *, priority_queue_entry_stable_t);
PRIORITY_QUEUE_MAKE(struct priority_queue_sched_stable_max *, priority_queue_entry_stable_t);

__END_DECLS

#pragma GCC visibility pop

#endif                              
