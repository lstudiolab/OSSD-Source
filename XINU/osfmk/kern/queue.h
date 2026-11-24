/*
 * Copyright (c) 2000-2009 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon rights
 * to redistribute these changes.
 */
  
   
  
                
                               
             
  
                                       
  
   

#ifndef _KERN_QUEUE_H_
#define _KERN_QUEUE_H_

#include <mach/mach_types.h>
#include <kern/macro_help.h>

#include <sys/cdefs.h>
#include <string.h>

__BEGIN_DECLS

  
                        
  
                                                                 
                                                                
                         
                                                            
  
                                                            
                                           
                                    
  
                                                         
                                                               
                                  
                                   
                                   
                           
                                           
                           
                                       
                  
                                                               
                                                                    
  
                                                                     
                                                                             
                                                             
  
                                                       
                                                                             
                                                                          
                                                                          
                                                                  
                                                                          
                                                     
  
                                                                            
                                                                         
                                                                             
                                                                      
                                           
  
                                             
                                             
                              
                               
                              
                              
                              
                             
                               
  
                           
                           
                                
                                
                                
                                
                            
                          
                          
                                 
                                 
                            
                              
                              
                                   
                                      
                                           
  
                                                  
                                                                             
                                                                          
                                                                         
                                                                              
                                                                           
                                                                       
                                                                         
                                        
  
                                                                          
                                                                          
                                                                         
                                                                          
                                                        
  
                                             
                                             
                              
                               
                              
                              
                              
                             
                               
  
                               
                                     
                                       
                                      
                               
                                
                                      
                                     
                                
                                  
                                 
  
          
                                         
                                                         
                                                         
   

  
                                        
   

struct queue_entry {
	struct queue_entry      *next;                            
	struct queue_entry      *prev;                                

#if __arm__ && (__BIGGEST_ALIGNMENT__ > 4)
                                                                               
              
                                                                   
                                                                 
                                                                   
              
   
} __attribute__ ((aligned(8)));
#else
};
#endif

typedef struct queue_entry      *queue_t;
typedef struct queue_entry      queue_head_t;
typedef struct queue_entry      queue_chain_t;
typedef struct queue_entry      *queue_entry_t;

  
                                     
                                                    
                                                       
   

#define enqueue(queue, elt)      enqueue_tail(queue, elt)
#define dequeue(queue)          dequeue_head(queue)

#ifdef XNU_KERNEL_PRIVATE
#include <kern/debug.h>
static inline void
__QUEUE_ELT_VALIDATE(queue_entry_t elt)
{
	queue_entry_t   elt_next, elt_prev;

	if (__improbable(elt == (queue_entry_t)NULL)) {
		panic("Invalid queue element %p", elt);
	}

	elt_next = elt->next;
	elt_prev = elt->prev;

	if (__improbable(elt_next == (queue_entry_t)NULL || elt_prev == (queue_entry_t)NULL)) {
		panic("Invalid queue element pointers for %p: next %p prev %p", elt, elt_next, elt_prev);
	}
	if (__improbable(elt_next->prev != elt || elt_prev->next != elt)) {
		panic("Invalid queue element linkage for %p: next %p next->prev %p prev %p prev->next %p",
		    elt, elt_next, elt_next->prev, elt_prev, elt_prev->next);
	}
}

static inline void
__DEQUEUE_ELT_CLEANUP(queue_entry_t elt)
{
	(elt)->next = (queue_entry_t)NULL;
	(elt)->prev = (queue_entry_t)NULL;
}
#else
#define __QUEUE_ELT_VALIDATE(elt) do { } while (0)
#define __DEQUEUE_ELT_CLEANUP(elt) do { } while(0)
#endif                          

static __inline__ void
enqueue_head(
	queue_t         que,
	queue_entry_t   elt)
{
	queue_entry_t   old_head;

	__QUEUE_ELT_VALIDATE((queue_entry_t)que);
	old_head = que->next;
	elt->next = old_head;
	elt->prev = que;
	old_head->prev = elt;
	que->next = elt;
}

static __inline__ void
enqueue_tail(
	queue_t         que,
	queue_entry_t   elt)
{
	queue_entry_t   old_tail;

	__QUEUE_ELT_VALIDATE((queue_entry_t)que);
	old_tail = que->prev;
	elt->next = que;
	elt->prev = old_tail;
	old_tail->next = elt;
	que->prev = elt;
}

static __inline__ queue_entry_t
dequeue_head(
	queue_t que)
{
	queue_entry_t   elt = (queue_entry_t)NULL;
	queue_entry_t   new_head;

	if (que->next != que) {
		elt = que->next;
		__QUEUE_ELT_VALIDATE(elt);
		new_head = elt->next;                                                            
		new_head->prev = que;
		que->next = new_head;
		__DEQUEUE_ELT_CLEANUP(elt);
	}

	return elt;
}

static __inline__ queue_entry_t
dequeue_tail(
	queue_t que)
{
	queue_entry_t   elt = (queue_entry_t)NULL;
	queue_entry_t   new_tail;

	if (que->prev != que) {
		elt = que->prev;
		__QUEUE_ELT_VALIDATE(elt);
		new_tail = elt->prev;                                                              
		new_tail->next = que;
		que->prev = new_tail;
		__DEQUEUE_ELT_CLEANUP(elt);
	}

	return elt;
}

static __inline__ void
remqueue(
	queue_entry_t   elt)
{
	queue_entry_t   next_elt, prev_elt;

	__QUEUE_ELT_VALIDATE(elt);
	next_elt = elt->next;
	prev_elt = elt->prev;                                                                                   
	next_elt->prev = prev_elt;
	prev_elt->next = next_elt;
	__DEQUEUE_ELT_CLEANUP(elt);
}

static __inline__ void
insque(
	queue_entry_t   entry,
	queue_entry_t   pred)
{
	queue_entry_t   successor;

	__QUEUE_ELT_VALIDATE(pred);
	successor = pred->next;
	entry->next = successor;
	entry->prev = pred;
	successor->prev = entry;
	pred->next = entry;
}

static __inline__ void
remque(
	queue_entry_t elt)
{
	remqueue(elt);
}

  
                          
              
                                                               
                                           
               
                                                         
                       
        
                                                                           
   
static __inline__ void
re_queue_head(queue_t que, queue_entry_t elt)
{
	queue_entry_t   n_elt, p_elt;

	__QUEUE_ELT_VALIDATE(elt);
	__QUEUE_ELT_VALIDATE((queue_entry_t)que);

	              
	n_elt = elt->next;
	p_elt = elt->prev;                                                                                   
	n_elt->prev = p_elt;
	p_elt->next = n_elt;

	                  
	n_elt = que->next;
	elt->next = n_elt;
	elt->prev = que;
	n_elt->prev = elt;
	que->next = elt;
}

  
                          
              
                                                             
                                           
               
                                                         
                      
        
                                                                           
   
static __inline__ void
re_queue_tail(queue_t que, queue_entry_t elt)
{
	queue_entry_t   n_elt, p_elt;

	__QUEUE_ELT_VALIDATE(elt);
	__QUEUE_ELT_VALIDATE((queue_entry_t)que);

	              
	n_elt = elt->next;
	p_elt = elt->prev;                                                                                   
	n_elt->prev = p_elt;
	p_elt->next = n_elt;

	                  
	p_elt = que->prev;
	elt->next = que;
	elt->prev = p_elt;
	p_elt->next = elt;
	que->prev = elt;
}

  
                     
            
                                                       
                                                        
                         
          
                                                        
                                     
                                                           
                                           
        
                                       
   
#define qe_element(qe, type, field) __container_of(qe, type, field)

  
                     
            
                                              
                                           
                                    
          
                                              
                              
                                                   
        
                                                                           
   
#define qe_foreach(qe, head) \
	for (qe = (head)->next; qe != (head); qe = (qe)->next)

  
                          
            
                                                     
  
                                                     
                                                
              
          
                                                   
                              
                                                   
        
                                                                           
   
#define qe_foreach_safe(qe, head) \
	for (queue_entry_t _ne = ((head)->next)->next, \
	         __ ## qe ## _unused_shadow __unused = (qe = (head)->next); \
	     qe != (head); \
	     qe = _ne, _ne = (qe)->next)

  
                             
            
                                          
                                              
                                                          
                                                 
          
                                                          
                                 
                                                           
                                           
        
                                                                           
   
#define qe_foreach_element(elt, head, field) \
	for (elt = qe_element((head)->next, typeof(*(elt)), field); \
	     &((elt)->field) != (head); \
	     elt = qe_element((elt)->field.next, typeof(*(elt)), field))

  
                                  
            
                                                 
                                              
                                                          
                                                 
  
                                                     
                                                      
          
                                                               
                                 
                                                           
                                           
        
                                                                           
   
#define qe_foreach_element_safe(elt, head, field) \
	for (typeof(*(elt)) *_nelt = qe_element(((head)->next)->next, typeof(*(elt)), field), \
	     *__ ## elt ## _unused_shadow __unused = \
	         (elt = qe_element((head)->next, typeof(*(elt)), field)); \
	     &((elt)->field) != (head); \
	     elt = _nelt, _nelt = qe_element((elt)->field.next, typeof(*(elt)), field)) \

#ifdef XNU_KERNEL_PRIVATE

                                                                        
#define qe_dequeue_head(head, type, field) ({ \
	queue_entry_t _tmp_entry = dequeue_head((head)); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) NULL) \
	        _tmp_element = qe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

                                                                        
#define qe_dequeue_tail(head, type, field) ({ \
	queue_entry_t _tmp_entry = dequeue_tail((head)); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) NULL) \
	        _tmp_element = qe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

                                                                     
#define qe_queue_first(head, type, field) ({ \
	queue_entry_t _tmp_entry = queue_first((head)); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) head) \
	        _tmp_element = qe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

                                                                    
#define qe_queue_last(head, type, field) ({ \
	queue_entry_t _tmp_entry = queue_last((head)); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) head) \
	        _tmp_element = qe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

                                                                                                 
#define qe_queue_next(head, element, type, field) ({ \
	queue_entry_t _tmp_entry = queue_next(&(element)->field); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) head) \
	        _tmp_element = qe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

                                                                                                 
#define qe_queue_prev(head, element, type, field) ({ \
	queue_entry_t _tmp_entry = queue_prev(&(element)->field); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) head) \
	        _tmp_element = qe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

#endif                         

  
                                   
            
                                 
   
#define QUEUE_HEAD_INITIALIZER(name) \
	{ &name, &name }

  
                     
            
                               
          
                      
                               
   
#define queue_init(q)   \
MACRO_BEGIN             \
	(q)->next = (q);\
	(q)->prev = (q);\
MACRO_END

  
                          
            
                                   
          
                           
                                   
   
#define queue_head_init(q) \
	queue_init(&(q))

  
                           
            
                                            
          
                            
                                    
   
#define queue_chain_init(q) \
	queue_init(&(q))

  
                      
            
                                         
          
                                
                         
   
#define queue_first(q)  ((q)->next)

  
                     
            
                                                 
          
                                
                
   
#define queue_next(qc)  ((qc)->next)

  
                     
            
                                        
          
                               
                         
   
#define queue_last(q)   ((q)->prev)

  
                     
            
                                                  
          
                                
                
   
#define queue_prev(qc)  ((qc)->prev)

  
                    
            
                                                  
              
          
                              
               
                      
   
#define queue_end(q, qe)        ((q) == (qe))

  
                      
            
                                   
          
                            
               
   
#define queue_empty(q)          queue_end((q), queue_first(q))

  
                     
              
                                                            
                                                                
               
                                                           
                                      
                                                   
        
                                                                
                                                                
                                                             
                                                                           
   
static __inline__ void
movqueue(queue_t _old, queue_t _new)
{
	queue_entry_t   next_elt, prev_elt;

	__QUEUE_ELT_VALIDATE((queue_entry_t)_old);

	if (queue_empty(_old)) {
		queue_init(_new);
		return;
	}

	  
                                  
                          
    
	next_elt = _old->next;
	prev_elt = _old->prev;

	_new->next = next_elt;
	_new->prev = prev_elt;
	next_elt->prev = _new;
	prev_elt->next = _new;

	queue_init(_old);
}

                                                                    
  
                                                        
                                                               
                              
   

  
                      
            
                                                  
          
                                         
               
                
                                  
                                            
        
                                                                           
  
                                                                        
                                                                          
                                                                        
                                                                            
                                                                     
                                                                       
                                                                   
              
   
#define queue_enter(head, elt, type, field)                     \
MACRO_BEGIN                                                     \
	queue_entry_t __prev;                                   \
                                                                \
	__prev = (head)->prev;                                  \
	(elt)->field.prev = __prev;                             \
	(elt)->field.next = head;                               \
	__compiler_barrier();                                   \
	if ((head) == __prev) {                                 \
	        (head)->next = (queue_entry_t) (elt);           \
	}                                                       \
	else {                                                  \
	        ((type)(void *)__prev)->field.next =            \
	                (queue_entry_t)(elt);                   \
	}                                                       \
	(head)->prev = (queue_entry_t) elt;                     \
MACRO_END

  
                            
            
                                                  
          
                                               
               
                
                                  
                                            
        
                                                                           
   
#define queue_enter_first(head, elt, type, field)               \
MACRO_BEGIN                                                     \
	queue_entry_t __next;                                   \
                                                                \
	__next = (head)->next;                                  \
	if ((head) == __next) {                                 \
	        (head)->prev = (queue_entry_t) (elt);           \
	}                                                       \
	else {                                                  \
	        ((type)(void *)__next)->field.prev =            \
	                (queue_entry_t)(elt);                   \
	}                                                       \
	(elt)->field.next = __next;                             \
	(elt)->field.prev = head;                               \
	(head)->next = (queue_entry_t) elt;                     \
MACRO_END

  
                              
            
                                                
          
                                                      
               
                
                
                                  
                                            
        
                                                                           
   
#define queue_insert_before(head, elt, cur, type, field)                \
MACRO_BEGIN                                                             \
	queue_entry_t __prev;                                           \
                                                                        \
	if ((head) == (queue_entry_t)(cur)) {                           \
	        (elt)->field.next = (head);                             \
	        if ((head)->next == (head)) {                           \
	                (elt)->field.prev = (head);                     \
	                (head)->next = (queue_entry_t)(elt);            \
	        } else {                                                \
	                __prev = (elt)->field.prev = (head)->prev;      \
	                ((type)(void *)__prev)->field.next =            \
	                        (queue_entry_t)(elt);                   \
	        }                                                       \
	        (head)->prev = (queue_entry_t)(elt);                    \
	} else {                                                        \
	        (elt)->field.next = (queue_entry_t)(cur);               \
	        if ((head)->next == (queue_entry_t)(cur)) {             \
	                        \
	                (elt)->field.prev = (head);                     \
	                (head)->next = (queue_entry_t)(elt);            \
	        } else {                                                \
	                __prev = (elt)->field.prev = (cur)->field.prev; \
	                ((type)(void *)__prev)->field.next =            \
	                        (queue_entry_t)(elt);                   \
	        }                                                       \
	        (cur)->field.prev = (queue_entry_t)(elt);               \
	}                                                               \
MACRO_END

  
                             
            
                                               
          
                                                     
               
                
                
                                  
                                            
        
                                                                           
   
#define queue_insert_after(head, elt, cur, type, field)                 \
MACRO_BEGIN                                                             \
	queue_entry_t __next;                                           \
                                                                        \
	if ((head) == (queue_entry_t)(cur)) {                           \
	        (elt)->field.prev = (head);                             \
	        if ((head)->next == (head)) {                           \
	                (elt)->field.next = (head);                     \
	                (head)->prev = (queue_entry_t)(elt);            \
	        } else {                                                \
	                __next = (elt)->field.next = (head)->next;      \
	                ((type)(void *)__next)->field.prev =            \
	                        (queue_entry_t)(elt);                   \
	        }                                                       \
	        (head)->next = (queue_entry_t)(elt);                    \
	} else {                                                        \
	        (elt)->field.prev = (queue_entry_t)(cur);               \
	        if ((head)->prev == (queue_entry_t)(cur)) {             \
	                        \
	                (elt)->field.next = (head);                     \
	                (head)->prev = (queue_entry_t)(elt);            \
	        } else {                                                \
	                __next = (elt)->field.next = (cur)->field.next; \
	                ((type)(void *)__next)->field.prev =            \
	                        (queue_entry_t)(elt);                   \
	        }                                                       \
	        (cur)->field.next = (queue_entry_t)(elt);               \
	}                                                               \
MACRO_END

  
                                          
            
                                               
                                                   
        
                                                                           
   
#define queue_field(head, thing, type, field)                   \
	        (((head) == (thing)) ? (head) : &((type)(void *)(thing))->field)

  
                       
            
                                            
          
                                         
                                
        
                                                                           
   
#define queue_remove(head, elt, type, field)                    \
MACRO_BEGIN                                                     \
	queue_entry_t	__next, __prev;                         \
                                                                \
	__next = (elt)->field.next;                             \
	__prev = (elt)->field.prev;                             \
                                                                \
	if ((head) == __next)                                   \
	        (head)->prev = __prev;                          \
	else                                                    \
	        ((type)(void *)__next)->field.prev = __prev;    \
                                                                \
	if ((head) == __prev)                                   \
	        (head)->next = __next;                          \
	else                                                    \
	        ((type)(void *)__prev)->field.next = __next;    \
                                                                \
	(elt)->field.next = NULL;                               \
	(elt)->field.prev = NULL;                               \
MACRO_END

  
                             
            
                                              
              
          
                                                
                                  
        
                                                                           
   
#define queue_remove_first(head, entry, type, field)            \
MACRO_BEGIN                                                     \
	queue_entry_t	__next;                                 \
                                                                \
	(entry) = (type)(void *) ((head)->next);                \
	__next = (entry)->field.next;                           \
                                                                \
	if ((head) == __next)                                   \
	        (head)->prev = (head);                          \
	else                                                    \
	        ((type)(void *)(__next))->field.prev = (head);  \
	(head)->next = __next;                                  \
                                                                \
	(entry)->field.next = NULL;                             \
	(entry)->field.prev = NULL;                             \
MACRO_END

  
                            
            
                                              
              
          
                                               
                                  
        
                                                                           
   
#define queue_remove_last(head, entry, type, field)             \
MACRO_BEGIN                                                     \
	queue_entry_t	__prev;                                 \
                                                                \
	(entry) = (type)(void *) ((head)->prev);                \
	__prev = (entry)->field.prev;                           \
                                                                \
	if ((head) == __prev)                                   \
	        (head)->next = (head);                          \
	else                                                    \
	        ((type)(void *)(__prev))->field.next = (head);  \
	(head)->prev = __prev;                                  \
                                                                \
	(entry)->field.next = NULL;                             \
	(entry)->field.prev = NULL;                             \
MACRO_END

  
                       
        
                                                                           
   
#define queue_assign(to, from, type, field)                     \
MACRO_BEGIN                                                     \
	((type)(void *)((from)->prev))->field.next = (to);      \
	((type)(void *)((from)->next))->field.prev = (to);      \
	*to = *from;                                            \
MACRO_END

  
                         
            
                                      
          
                                         
                 
                 
                                  
                                                               
        
                                                                           
   
#define queue_new_head(old, new, type, field)                   \
MACRO_BEGIN                                                     \
	if (!queue_empty(old)) {                                \
	        *(new) = *(old);                                \
	        ((type)(void *)((new)->next))->field.prev =     \
	                (new);                                  \
	        ((type)(void *)((new)->prev))->field.next =     \
	                (new);                                  \
	} else {                                                \
	        queue_init(new);                                \
	}                                                       \
MACRO_END

  
                        
            
                                        
                                          
                                     
          
                                      
               
                
                                  
                                            
        
                                                                           
   
#define queue_iterate(head, elt, type, field)                   \
	for ((elt) = (type)(void *) queue_first(head);          \
	     !queue_end((head), (queue_entry_t)(elt));          \
	     (elt) = (type)(void *) queue_next(&(elt)->field))


__END_DECLS

#endif                      
