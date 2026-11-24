/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

#ifndef _KERN_CIRCLE_QUEUE_H_
#define _KERN_CIRCLE_QUEUE_H_

#include <kern/queue.h>
#include <kern/assert.h>

__BEGIN_DECLS

  
                               
  
                                                
                                                                     
                
   

typedef struct circle_queue_head {
	queue_entry_t head;
} circle_queue_head_t, *circle_queue_t;

static inline bool
circle_queue_empty(circle_queue_t cq)
{
	return cq->head == NULL;
}

static inline queue_entry_t
circle_queue_first(circle_queue_t cq)
{
	return cq->head;
}

static inline queue_entry_t
circle_queue_last(circle_queue_t cq)
{
	queue_entry_t elt = circle_queue_first(cq);
	if (elt) {
		__builtin_assume(elt->prev != NULL);
		return elt->prev;
	}
	return NULL;
}

static inline queue_entry_t
circle_queue_next(circle_queue_t cq, queue_entry_t elt)
{
	return elt->next == cq->head ? NULL : elt->next;
}

static inline size_t
circle_queue_length(circle_queue_t cq)
{
	queue_entry_t elt = circle_queue_first(cq);
	size_t n = 0;

	for (; elt; elt = circle_queue_next(cq, elt)) {
		n++;
	}
	return n;
}

static inline void
circle_enqueue_tail(circle_queue_t cq, queue_entry_t elt)
{
	queue_entry_t head = circle_queue_first(cq);
	queue_entry_t tail = circle_queue_last(cq);

	if (head == NULL) {
		cq->head = elt->next = elt->prev = elt;
	} else {
		elt->next = head;
		elt->prev = tail;
		tail->next = elt;
		head->prev = elt;
	}
}

static inline void
circle_enqueue_head(circle_queue_t cq, queue_entry_t elt)
{
	circle_enqueue_tail(cq, elt);
	cq->head = elt;
}

static inline void
circle_dequeue(circle_queue_t cq, queue_entry_t elt)
{
	queue_entry_t elt_prev = elt->prev;
	queue_entry_t elt_next = elt->next;

	if (elt == elt_next) {
		assert(cq->head == elt);
		cq->head = NULL;
	} else {
		elt_prev->next = elt_next;
		elt_next->prev = elt_prev;
		if (cq->head == elt) {
			cq->head = elt_next;
		}
	}
	__DEQUEUE_ELT_CLEANUP(elt);
}

static inline queue_entry_t
circle_dequeue_head(circle_queue_t cq)
{
	queue_entry_t elt = circle_queue_first(cq);
	if (elt) {
		circle_dequeue(cq, elt);
	}
	return elt;
}

static inline queue_entry_t
circle_dequeue_tail(circle_queue_t cq)
{
	queue_entry_t elt = circle_queue_last(cq);
	if (elt) {
		circle_dequeue(cq, elt);
	}
	return elt;
}

static inline void
circle_queue_rotate_head_forward(circle_queue_t cq)
{
	queue_entry_t first = circle_queue_first(cq);
	if (first != NULL) {
		cq->head = first->next;
	}
}

static inline void
circle_queue_rotate_head_backward(circle_queue_t cq)
{
	queue_entry_t last = circle_queue_last(cq);
	if (last != NULL) {
		cq->head = last;
	}
}

  
                      
            
                                                                     
                                                        
                               
          
                                                               
                                     
                                                           
                                           
        
                                       
   
#define cqe_element(qe, type, field) __container_of(qe, type, field)

  
                      
            
                                              
                                           
                                    
          
                                               
                              
                                                   
        
                                                                           
   
#define cqe_foreach(qe, head) \
	for (qe = circle_queue_first(head); qe; qe = circle_queue_next(head, qe))

  
                           
            
                                                     
  
                                                     
                                                
              
          
                                                    
                              
                                                   
        
                                                                           
   
#define cqe_foreach_safe(qe, head) \
	for (queue_entry_t _ne, _qe = circle_queue_first(head); \
	     (qe = _qe) && (_ne = circle_queue_next(head, _qe), 1); \
	     _qe = _ne)

  
                              
            
                                          
                                              
                                                          
                                                 
          
                                                           
                                 
                                                           
                                           
        
                                                                           
   
#define cqe_foreach_element(elt, head, field) \
	for (queue_entry_t _qe = circle_queue_first(head); \
	     _qe && (elt = cqe_element(_qe, typeof(*(elt)), field), 1); \
	     _qe = circle_queue_next(head, _qe))

  
                                   
            
                                                 
                                              
                                                          
                                                 
  
                                                     
                                                      
          
                                                                
                                 
                                                           
                                           
        
                                                                           
   
#define cqe_foreach_element_safe(elt, head, field) \
	for (queue_entry_t _ne, _qe = circle_queue_first(head); \
	     _qe && (elt = cqe_element(_qe, typeof(*(elt)), field), \
	     _ne = circle_queue_next(head, _qe), 1); \
	     _qe = _ne)

                                                                        
#define cqe_dequeue_head(head, type, field) ({ \
	queue_entry_t _tmp_entry = circle_dequeue_head((head)); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) NULL) \
	        _tmp_element = cqe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

                                                                        
#define cqe_dequeue_tail(head, type, field) ({ \
	queue_entry_t _tmp_entry = circle_dequeue_tail((head)); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) NULL) \
	        _tmp_element = cqe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

                                                                     
#define cqe_queue_first(head, type, field) ({ \
	queue_entry_t _tmp_entry = circle_queue_first((head)); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) NULL) \
	        _tmp_element = cqe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

                                                            
#define cqe_queue_next(elt, head, type, field) ({ \
	queue_entry_t _tmp_entry = circle_queue_next((head), (elt)); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) NULL) \
	        _tmp_element = cqe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

                                                                    
#define cqe_queue_last(head, type, field) ({ \
	queue_entry_t _tmp_entry = circle_queue_last((head)); \
	type *_tmp_element = (type*) NULL; \
	if (_tmp_entry != (queue_entry_t) NULL) \
	        _tmp_element = cqe_element(_tmp_entry, type, field); \
	_tmp_element; \
})

  
                            
            
                                      
          
                             
                                      
   
#define circle_queue_init(q)   \
MACRO_BEGIN             \
	(q)->head = NULL; \
MACRO_END

__END_DECLS

#endif                      
