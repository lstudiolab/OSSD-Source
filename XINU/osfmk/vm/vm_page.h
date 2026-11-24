/*
 * Copyright (c) 2000-2020 Apple Computer, Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
   
  
                     
                                                    
             
  
                                      
   

#ifndef _VM_VM_PAGE_H_
#define _VM_VM_PAGE_H_

#include <debug.h>
#include <vm/vm_options.h>
#include <vm/vm_protos.h>
#include <mach/boolean.h>
#include <mach/vm_prot.h>
#include <mach/vm_param.h>
#include <mach/memory_object_types.h>                         


#if    defined(__LP64__)

  
                                                                                                
                                                                                                
                                                                                                
                                                                                                  
                                           
   
typedef uint32_t        vm_page_packed_t;

struct vm_page_packed_queue_entry {
	vm_page_packed_t        next;                            
	vm_page_packed_t        prev;                                
};

typedef struct vm_page_packed_queue_entry       *vm_page_queue_t;
typedef struct vm_page_packed_queue_entry       vm_page_queue_head_t;
typedef struct vm_page_packed_queue_entry       vm_page_queue_chain_t;
typedef struct vm_page_packed_queue_entry       *vm_page_queue_entry_t;

typedef vm_page_packed_t                        vm_page_object_t;

#else            

  
                                                        
                                      
   
typedef struct vm_page          *vm_page_packed_t;

#define vm_page_queue_t         queue_t
#define vm_page_queue_head_t    queue_head_t
#define vm_page_queue_chain_t   queue_chain_t
#define vm_page_queue_entry_t   queue_entry_t

#define vm_page_object_t        vm_object_t
#endif            


#include <vm/vm_object.h>
#include <kern/queue.h>
#include <kern/locks.h>

#include <kern/macro_help.h>
#include <libkern/OSAtomic.h>



#define VM_PAGE_COMPRESSOR_COUNT        (compressor_object->resident_page_count)

  
                                          
  
                                              
                                                
                                  
  
                                       
                                 
  
                                           
                                         
                         
  
                                             
  
                                                 
                                                       
                          
  
                                                                
                                                                 
                                                               
                                                                 
   

#define VM_PAGE_NULL            ((vm_page_t) 0)

extern  char    vm_page_inactive_states[];
extern  char    vm_page_pageable_states[];
extern  char    vm_page_non_speculative_pageable_states[];
extern  char    vm_page_active_or_inactive_states[];


#define VM_PAGE_INACTIVE(m)                     (vm_page_inactive_states[m->vmp_q_state])
#define VM_PAGE_PAGEABLE(m)                     (vm_page_pageable_states[m->vmp_q_state])
#define VM_PAGE_NON_SPECULATIVE_PAGEABLE(m)     (vm_page_non_speculative_pageable_states[m->vmp_q_state])
#define VM_PAGE_ACTIVE_OR_INACTIVE(m)           (vm_page_active_or_inactive_states[m->vmp_q_state])


#define VM_PAGE_NOT_ON_Q                0                                                                                                  
#define VM_PAGE_IS_WIRED                1                                            
#define VM_PAGE_USED_BY_COMPRESSOR      2                                                                             
#define VM_PAGE_ON_FREE_Q               3                                                   
#define VM_PAGE_ON_FREE_LOCAL_Q         4                                                              
#define VM_PAGE_ON_FREE_LOPAGE_Q        5                                                         
#define VM_PAGE_ON_THROTTLED_Q          6                                                                                                    
#define VM_PAGE_ON_PAGEOUT_Q            7                                                                                                 
#define VM_PAGE_ON_SPECULATIVE_Q        8                                                             
#define VM_PAGE_ON_ACTIVE_LOCAL_Q       9                                                                                                        
#define VM_PAGE_ON_ACTIVE_Q             10                                                  
#define VM_PAGE_ON_INACTIVE_INTERNAL_Q  11                                                                                  
#define VM_PAGE_ON_INACTIVE_EXTERNAL_Q  12                                                                                    
#define VM_PAGE_ON_INACTIVE_CLEANED_Q   13                                                                                     
#define VM_PAGE_ON_SECLUDED_Q           14                                             
#define VM_PAGE_Q_STATE_LAST_VALID_VALUE        14                                                                                

#define VM_PAGE_Q_STATE_ARRAY_SIZE      (VM_PAGE_Q_STATE_LAST_VALID_VALUE+1)


  
                                                                               
   
#define vmp_pageq vmp_q_un.vmp_q_pageq
#define vmp_snext vmp_q_un.vmp_q_snext

struct vm_page {
	union {
		vm_page_queue_chain_t vmp_q_pageq;                                                           
		struct vm_page        *vmp_q_snext;
	} vmp_q_un;

	vm_page_queue_chain_t         vmp_listq;                                             

#if CONFIG_BACKGROUND_QUEUE
	vm_page_queue_chain_t         vmp_backgroundq;                                                     
#endif

	vm_object_offset_t            vmp_offset;                                             
	vm_page_object_t              vmp_object;                                          

	  
                                                                              
   
                                                                                    
                                                                             
    
#define vmp_local_id vmp_wire_count
	unsigned int vmp_wire_count:16,                                                  
	    vmp_q_state:4,                                                   
	    vmp_in_background:1,
	    vmp_on_backgroundq:1,
	    vmp_gobbled:1,                                                 
	    vmp_laundry:1,                                                     
	    vmp_no_cache:1,                                                           
	                                                                             
	    vmp_private:1,                                                                         
	    vmp_reference:1,                                             
	    vmp_lopage:1,
	    vmp_unused_page_bits:4;

	  
                                                   
                                                  
                                               
                                               
                                                 
                     
    
	vm_page_packed_t vmp_next_m;                                    

	  
                                                                     
   
                                                                                                           
                                                                                              
                                                                     
                                                                           
    
	unsigned int    vmp_busy:1,                                       
	    vmp_wanted:1,                                                          
	    vmp_tabled:1,                                                  
	    vmp_hashed:1,                                                                             
	    vmp_fictitious:1,                                                      
	    vmp_clustered:1,                                                                                    
	    vmp_pmapped:1,                                                                               
	                                                                    
	    vmp_xpmapped:1,                                                                             
	                                                                    
	    vmp_wpmapped:1,                                                                                      
	    vmp_free_when_done:1,                                                                     
	    vmp_absent:1,                                                                                
	    vmp_error:1,                                                                                    
	    vmp_dirty:1,                                                    
	    vmp_cleaning:1,                                                 
	    vmp_precious:1,                                                                                  
	    vmp_overwriting:1,                                                                              
	                                                                         
	    vmp_restart:1,                                                                                            
	                                                                       
	    vmp_unusual:1,                                                                       
	    vmp_cs_validated:VMP_CS_BITS,                                     
	    vmp_cs_tainted:VMP_CS_BITS,                                      
	    vmp_cs_nx:VMP_CS_BITS,                                      
	    vmp_reusable:1,
	    vmp_written_by_kernel:1;                                                                 

#if    !defined(__arm__) && !defined(__arm64__)
	ppnum_t         vmp_phys_page;                                              
#endif
};

typedef struct vm_page  *vm_page_t;
extern vm_page_t        vm_pages;
extern vm_page_t        vm_page_array_beginning_addr;
extern vm_page_t        vm_page_array_ending_addr;

static inline int
VMP_CS_FOR_OFFSET(
	vm_map_offset_t fault_phys_offset)
{
	assertf(fault_phys_offset < PAGE_SIZE &&
	    !(fault_phys_offset & FOURK_PAGE_MASK),
	    "offset 0x%llx\n", (uint64_t)fault_phys_offset);
	return 1 << (fault_phys_offset >> FOURK_PAGE_SHIFT);
}
static inline bool
VMP_CS_VALIDATED(
	vm_page_t p,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset)
{
	assertf(fault_page_size <= PAGE_SIZE,
	    "fault_page_size 0x%llx fault_phys_offset 0x%llx\n",
	    (uint64_t)fault_page_size, (uint64_t)fault_phys_offset);
	if (fault_page_size == PAGE_SIZE) {
		return p->vmp_cs_validated == VMP_CS_ALL_TRUE;
	}
	return p->vmp_cs_validated & VMP_CS_FOR_OFFSET(fault_phys_offset);
}
static inline bool
VMP_CS_TAINTED(
	vm_page_t p,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset)
{
	assertf(fault_page_size <= PAGE_SIZE,
	    "fault_page_size 0x%llx fault_phys_offset 0x%llx\n",
	    (uint64_t)fault_page_size, (uint64_t)fault_phys_offset);
	if (fault_page_size == PAGE_SIZE) {
		return p->vmp_cs_tainted != VMP_CS_ALL_FALSE;
	}
	return p->vmp_cs_tainted & VMP_CS_FOR_OFFSET(fault_phys_offset);
}
static inline bool
VMP_CS_NX(
	vm_page_t p,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset)
{
	assertf(fault_page_size <= PAGE_SIZE,
	    "fault_page_size 0x%llx fault_phys_offset 0x%llx\n",
	    (uint64_t)fault_page_size, (uint64_t)fault_phys_offset);
	if (fault_page_size == PAGE_SIZE) {
		return p->vmp_cs_nx != VMP_CS_ALL_FALSE;
	}
	return p->vmp_cs_nx & VMP_CS_FOR_OFFSET(fault_phys_offset);
}
static inline void
VMP_CS_SET_VALIDATED(
	vm_page_t p,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	boolean_t value)
{
	assertf(fault_page_size <= PAGE_SIZE,
	    "fault_page_size 0x%llx fault_phys_offset 0x%llx\n",
	    (uint64_t)fault_page_size, (uint64_t)fault_phys_offset);
	if (value) {
		if (fault_page_size == PAGE_SIZE) {
			p->vmp_cs_validated = VMP_CS_ALL_TRUE;
		}
		p->vmp_cs_validated |= VMP_CS_FOR_OFFSET(fault_phys_offset);
	} else {
		if (fault_page_size == PAGE_SIZE) {
			p->vmp_cs_validated = VMP_CS_ALL_FALSE;
		}
		p->vmp_cs_validated &= ~VMP_CS_FOR_OFFSET(fault_phys_offset);
	}
}
static inline void
VMP_CS_SET_TAINTED(
	vm_page_t p,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	boolean_t value)
{
	assertf(fault_page_size <= PAGE_SIZE,
	    "fault_page_size 0x%llx fault_phys_offset 0x%llx\n",
	    (uint64_t)fault_page_size, (uint64_t)fault_phys_offset);
	if (value) {
		if (fault_page_size == PAGE_SIZE) {
			p->vmp_cs_tainted = VMP_CS_ALL_TRUE;
		}
		p->vmp_cs_tainted |= VMP_CS_FOR_OFFSET(fault_phys_offset);
	} else {
		if (fault_page_size == PAGE_SIZE) {
			p->vmp_cs_tainted = VMP_CS_ALL_FALSE;
		}
		p->vmp_cs_tainted &= ~VMP_CS_FOR_OFFSET(fault_phys_offset);
	}
}
static inline void
VMP_CS_SET_NX(
	vm_page_t p,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	boolean_t value)
{
	assertf(fault_page_size <= PAGE_SIZE,
	    "fault_page_size 0x%llx fault_phys_offset 0x%llx\n",
	    (uint64_t)fault_page_size, (uint64_t)fault_phys_offset);
	if (value) {
		if (fault_page_size == PAGE_SIZE) {
			p->vmp_cs_nx = VMP_CS_ALL_TRUE;
		}
		p->vmp_cs_nx |= VMP_CS_FOR_OFFSET(fault_phys_offset);
	} else {
		if (fault_page_size == PAGE_SIZE) {
			p->vmp_cs_nx = VMP_CS_ALL_FALSE;
		}
		p->vmp_cs_nx &= ~VMP_CS_FOR_OFFSET(fault_phys_offset);
	}
}


#if defined(__arm__) || defined(__arm64__)

extern  unsigned int vm_first_phys_ppnum;

struct vm_page_with_ppnum {
	struct  vm_page vm_page_wo_ppnum;

	ppnum_t vmp_phys_page;
};
typedef struct vm_page_with_ppnum *vm_page_with_ppnum_t;


static inline ppnum_t
VM_PAGE_GET_PHYS_PAGE(vm_page_t m)
{
	if (m >= vm_page_array_beginning_addr && m < vm_page_array_ending_addr) {
		return (ppnum_t)((uintptr_t)(m - vm_page_array_beginning_addr) + vm_first_phys_ppnum);
	} else {
		return ((vm_page_with_ppnum_t)m)->vmp_phys_page;
	}
}

#define VM_PAGE_SET_PHYS_PAGE(m, ppnum)         \
	MACRO_BEGIN                             \
	if ((m) < vm_page_array_beginning_addr || (m) >= vm_page_array_ending_addr)     \
	        ((vm_page_with_ppnum_t)(m))->vmp_phys_page = ppnum;     \
	assert(ppnum == VM_PAGE_GET_PHYS_PAGE(m));              \
	MACRO_END

#define VM_PAGE_GET_COLOR(m)    (VM_PAGE_GET_PHYS_PAGE(m) & vm_color_mask)

#else                                               


struct vm_page_with_ppnum {
	struct  vm_page vm_page_with_ppnum;
};
typedef struct vm_page_with_ppnum *vm_page_with_ppnum_t;


#define VM_PAGE_GET_PHYS_PAGE(page)     (page)->vmp_phys_page
#define VM_PAGE_SET_PHYS_PAGE(page, ppnum)      \
	MACRO_BEGIN                             \
	(page)->vmp_phys_page = ppnum;          \
	MACRO_END

#define VM_PAGE_GET_CLUMP(m)    ((VM_PAGE_GET_PHYS_PAGE(m)) >> vm_clump_shift)
#define VM_PAGE_GET_COLOR(m)    ((VM_PAGE_GET_CLUMP(m)) & vm_color_mask)

#endif                                              



#if defined(__LP64__)
  
                                 
  
  
                                    
  
                                                    
  
                                                            
  
                                                                                
               
  
  
                                                                           
                                                                           
                                                                   
  
                                                                            
   
#define VM_VPLQ_ALIGNMENT               128
#define VM_PAGE_PACKED_PTR_ALIGNMENT    64                                        
#define VM_PAGE_PACKED_ALIGNED          __attribute__((aligned(VM_PAGE_PACKED_PTR_ALIGNMENT)))
#define VM_PAGE_PACKED_PTR_BITS         31
#define VM_PAGE_PACKED_PTR_SHIFT        6
#define VM_PAGE_PACKED_PTR_BASE         ((uintptr_t)VM_MIN_KERNEL_AND_KEXT_ADDRESS)

#define VM_PAGE_PACKED_FROM_ARRAY       0x80000000

static inline vm_page_packed_t
vm_page_pack_ptr(uintptr_t p)
{
	if (p >= (uintptr_t)vm_page_array_beginning_addr &&
	    p < (uintptr_t)vm_page_array_ending_addr) {
		ptrdiff_t diff = (vm_page_t)p - vm_page_array_beginning_addr;
		assert((vm_page_t)p == &vm_pages[diff]);
		return (vm_page_packed_t)(diff | VM_PAGE_PACKED_FROM_ARRAY);
	}

	VM_ASSERT_POINTER_PACKABLE(p, VM_PAGE_PACKED_PTR);
	vm_offset_t packed = VM_PACK_POINTER(p, VM_PAGE_PACKED_PTR);
	return CAST_DOWN_EXPLICIT(vm_page_packed_t, packed);
}


static inline uintptr_t
vm_page_unpack_ptr(uintptr_t p)
{
	extern unsigned int vm_pages_count;

	if (p >= VM_PAGE_PACKED_FROM_ARRAY) {
		p &= ~VM_PAGE_PACKED_FROM_ARRAY;
		assert(p < (uintptr_t)vm_pages_count);
		return (uintptr_t)&vm_pages[p];
	}

	return VM_UNPACK_POINTER(p, VM_PAGE_PACKED_PTR);
}


#define VM_PAGE_PACK_PTR(p)     vm_page_pack_ptr((uintptr_t)(p))
#define VM_PAGE_UNPACK_PTR(p)   vm_page_unpack_ptr((uintptr_t)(p))

#define VM_PAGE_OBJECT(p)       ((vm_object_t)(VM_PAGE_UNPACK_PTR(p->vmp_object)))
#define VM_PAGE_PACK_OBJECT(o)  ((vm_page_object_t)(VM_PAGE_PACK_PTR(o)))


#define VM_PAGE_ZERO_PAGEQ_ENTRY(p)     \
MACRO_BEGIN                             \
	(p)->vmp_snext = 0;             \
MACRO_END


#define VM_PAGE_CONVERT_TO_QUEUE_ENTRY(p)       VM_PAGE_PACK_PTR(p)


static __inline__ void
vm_page_enqueue_tail(
	vm_page_queue_t         que,
	vm_page_queue_entry_t   elt)
{
	vm_page_queue_entry_t   old_tail;

	old_tail = (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(que->prev);
	elt->next = VM_PAGE_PACK_PTR(que);
	elt->prev = que->prev;
	que->prev = old_tail->next = VM_PAGE_PACK_PTR(elt);
}


static __inline__ void
vm_page_remque(
	vm_page_queue_entry_t elt)
{
	vm_page_queue_entry_t next;
	vm_page_queue_entry_t prev;
	vm_page_packed_t      next_pck = elt->next;
	vm_page_packed_t      prev_pck = elt->prev;

	next = (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(next_pck);

	                                                                          
	prev = (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(prev_pck);

	next->prev = prev_pck;
	prev->next = next_pck;

	elt->next = 0;
	elt->prev = 0;
}


  
                            
            
                               
          
                             
                                     
   
#define vm_page_queue_init(q)               \
MACRO_BEGIN                                 \
	VM_ASSERT_POINTER_PACKABLE((vm_offset_t)(q), VM_PAGE_PACKED_PTR); \
	(q)->next = VM_PAGE_PACK_PTR(q);        \
	(q)->prev = VM_PAGE_PACK_PTR(q);        \
MACRO_END


  
                             
            
                                                             
          
                                              
                     
                         
                                                 
  
                                                                                  
                                           
   
#define vm_page_queue_enter(head, elt, field)                       \
MACRO_BEGIN                                                         \
	vm_page_packed_t __pck_elt = VM_PAGE_PACK_PTR(elt);         \
	vm_page_packed_t __pck_head = VM_PAGE_PACK_PTR(head);       \
	vm_page_packed_t __pck_prev = (head)->prev;                 \
                                                                    \
	if (__pck_head == __pck_prev) {                             \
	        (head)->next = __pck_elt;                           \
	} else {                                                    \
	        vm_page_t __prev;                                   \
	        __prev = (vm_page_t)VM_PAGE_UNPACK_PTR(__pck_prev); \
	        __prev->field.next = __pck_elt;                     \
	}                                                           \
	(elt)->field.prev = __pck_prev;                             \
	(elt)->field.next = __pck_head;                             \
	(head)->prev = __pck_elt;                                   \
MACRO_END


#if defined(__x86_64__)
  
                                                                  
                                                               
   
#if DEVELOPMENT || DEBUG

#define __DEBUG_CHECK_BUDDIES(__prev, __p, field)                                             \
MACRO_BEGIN                                                                                   \
	if (__prev != NULL) {                                                                 \
	        assert(__p == (vm_page_t)VM_PAGE_UNPACK_PTR(__prev->next));                   \
	        assert(__prev == (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(__p->field.prev)); \
	}                                                                                     \
MACRO_END

#define __DEBUG_VERIFY_LINKS(__first, __n_free, __last_next)                    \
MACRO_BEGIN                                                                     \
	unsigned int __i;                                                       \
	vm_page_queue_entry_t __tmp;                                            \
	for (__i = 0, __tmp = __first; __i < __n_free; __i++) {                 \
	        __tmp = (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(__tmp->next); \
	}                                                                       \
	assert(__tmp == __last_next);                                           \
MACRO_END

#define __DEBUG_STAT_INCREMENT_INRANGE              vm_clump_inrange++
#define __DEBUG_STAT_INCREMENT_INSERTS              vm_clump_inserts++
#define __DEBUG_STAT_INCREMENT_PROMOTES(__n_free)   vm_clump_promotes+=__n_free

#else

#define __DEBUG_CHECK_BUDDIES(__prev, __p, field)
#define __DEBUG_VERIFY_LINKS(__first, __n_free, __last_next)
#define __DEBUG_STAT_INCREMENT_INRANGE
#define __DEBUG_STAT_INCREMENT_INSERTS
#define __DEBUG_STAT_INCREMENT_PROMOTES(__n_free)

#endif                               

  
                                                                                            
   
static inline void
vm_page_queue_enter_clump(
	vm_page_queue_t       head,
	vm_page_t             elt)
{
	vm_page_queue_entry_t first = NULL;                                 
	vm_page_queue_entry_t last = NULL;                                 
	vm_page_queue_entry_t prev = NULL;
	vm_page_queue_entry_t next;
	uint_t                n_free = 1;
	extern unsigned int   vm_pages_count;
	extern unsigned int   vm_clump_size, vm_clump_mask, vm_clump_shift, vm_clump_promote_threshold;
	extern unsigned long  vm_clump_allocs, vm_clump_inserts, vm_clump_inrange, vm_clump_promotes;

	  
                                                                                      
    
	if (vm_page_array_beginning_addr <= elt && elt < &vm_pages[vm_pages_count]) {
		vm_page_t p;
		uint_t    i;
		uint_t    n;
		ppnum_t   clump_num;

		first = last = (vm_page_queue_entry_t)elt;
		clump_num = VM_PAGE_GET_CLUMP(elt);
		n = VM_PAGE_GET_PHYS_PAGE(elt) & vm_clump_mask;

		  
                                                              
     
		for (i = 0, p = elt - 1; i < n && vm_page_array_beginning_addr <= p; i++, p--) {
			if (p->vmp_q_state == VM_PAGE_ON_FREE_Q && clump_num == VM_PAGE_GET_CLUMP(p)) {
				if (prev == NULL) {
					prev = (vm_page_queue_entry_t)p;
				}
				first = (vm_page_queue_entry_t)p;
				n_free++;
			}
		}

		  
                                                             
     
		for (i = n + 1, p = elt + 1; i < vm_clump_size && p < &vm_pages[vm_pages_count]; i++, p++) {
			if (p->vmp_q_state == VM_PAGE_ON_FREE_Q && clump_num == VM_PAGE_GET_CLUMP(p)) {
				if (last == (vm_page_queue_entry_t)elt) {                                   
					__DEBUG_CHECK_BUDDIES(prev, p, vmp_pageq);
				}

				if (prev == NULL) {
					prev = (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(p->vmp_pageq.prev);
				}
				last = (vm_page_queue_entry_t)p;
				n_free++;
			}
		}
		__DEBUG_STAT_INCREMENT_INRANGE;
	}

	                                                                            
	if (prev == NULL) {
		prev = (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(head->prev);
	}

	                        
	next = (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(prev->next);
	elt->vmp_pageq.next = prev->next;
	elt->vmp_pageq.prev = next->prev;
	prev->next = next->prev = VM_PAGE_PACK_PTR(elt);
	__DEBUG_STAT_INCREMENT_INSERTS;

	  
                                                
    
	if (n_free >= vm_clump_promote_threshold && n_free > 1) {
		vm_page_queue_entry_t first_prev;

		first_prev = (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(first->prev);

		                            
		if (first_prev != head) {
			vm_page_queue_entry_t last_next;
			vm_page_queue_entry_t head_next;

			last_next = (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(last->next);

			                                                           
			__DEBUG_VERIFY_LINKS(first, n_free, last_next);

			                           
			first_prev->next = last->next;
			last_next->prev = first->prev;
			first->prev = VM_PAGE_PACK_PTR(head);
			last->next = head->next;

			head_next = (vm_page_queue_entry_t)VM_PAGE_UNPACK_PTR(head->next);
			head_next->prev = VM_PAGE_PACK_PTR(last);
			head->next = VM_PAGE_PACK_PTR(first);
			__DEBUG_STAT_INCREMENT_PROMOTES(n_free);
		}
	}
}
#endif

  
                                   
            
                                                             
          
                                              
                     
                         
                                                  
  
                                                                                        
                                           
   
#define vm_page_queue_enter_first(head, elt, field)                 \
MACRO_BEGIN                                                         \
	vm_page_packed_t __pck_next = (head)->next;                 \
	vm_page_packed_t __pck_head = VM_PAGE_PACK_PTR(head);       \
	vm_page_packed_t __pck_elt = VM_PAGE_PACK_PTR(elt);         \
                                                                    \
	if (__pck_head == __pck_next) {                             \
	        (head)->prev = __pck_elt;                           \
	} else {                                                    \
	        vm_page_t __next;                                   \
	        __next = (vm_page_t)VM_PAGE_UNPACK_PTR(__pck_next); \
	        __next->field.prev = __pck_elt;                     \
	}                                                           \
                                                                    \
	(elt)->field.next = __pck_next;                             \
	(elt)->field.prev = __pck_head;                             \
	(head)->next = __pck_elt;                                   \
MACRO_END


  
                              
            
                                                     
          
                                              
                                              
  
                                                                                  
                                           
   
#define vm_page_queue_remove(head, elt, field)                          \
MACRO_BEGIN                                                             \
	vm_page_packed_t __pck_next = (elt)->field.next;                \
	vm_page_packed_t __pck_prev = (elt)->field.prev;                \
	vm_page_t        __next = (vm_page_t)VM_PAGE_UNPACK_PTR(__pck_next); \
	vm_page_t        __prev = (vm_page_t)VM_PAGE_UNPACK_PTR(__pck_prev); \
                                                                        \
	if ((void *)(head) == (void *)__next) {                         \
	        (head)->prev = __pck_prev;                              \
	} else {                                                        \
	        __next->field.prev = __pck_prev;                        \
	}                                                               \
                                                                        \
	if ((void *)(head) == (void *)__prev) {                         \
	        (head)->next = __pck_next;                              \
	} else {                                                        \
	        __prev->field.next = __pck_next;                        \
	}                                                               \
                                                                        \
	(elt)->field.next = 0;                                          \
	(elt)->field.prev = 0;                                          \
MACRO_END


  
                                    
  
            
                                                                  
  
          
                                                     
                                          
  
                                                                                         
                                           
   
#define vm_page_queue_remove_first(head, entry, field)            \
MACRO_BEGIN                                                       \
	vm_page_packed_t __pck_head = VM_PAGE_PACK_PTR(head);     \
	vm_page_packed_t __pck_next;                              \
	vm_page_t        __next;                                  \
                                                                  \
	(entry) = (vm_page_t)VM_PAGE_UNPACK_PTR((head)->next);    \
	__pck_next = (entry)->field.next;                         \
	__next = (vm_page_t)VM_PAGE_UNPACK_PTR(__pck_next);       \
                                                                  \
	if (__pck_head == __pck_next) {                           \
	        (head)->prev = __pck_head;                        \
	} else {                                                  \
	        __next->field.prev = __pck_head;                  \
	}                                                         \
                                                                  \
	(head)->next = __pck_next;                                \
	(entry)->field.next = 0;                                  \
	(entry)->field.prev = 0;                                  \
MACRO_END


#if defined(__x86_64__)
  
                                                
            
                                                                
                                                                                 
  
          
                                                              
                                     
                                   
   
#define vm_page_queue_remove_first_with_clump(head, entry, end)              \
MACRO_BEGIN                                                                  \
	vm_page_packed_t __pck_head = VM_PAGE_PACK_PTR(head);                \
	vm_page_packed_t __pck_next;                                         \
	vm_page_t        __next;                                             \
                                                                             \
	(entry) = (vm_page_t)VM_PAGE_UNPACK_PTR((head)->next);               \
	__pck_next = (entry)->vmp_pageq.next;                                \
	__next = (vm_page_t)VM_PAGE_UNPACK_PTR(__pck_next);                  \
                                                                             \
	(end) = 0;                                                           \
	if (__pck_head == __pck_next) {                                      \
	        (head)->prev = __pck_head;                                   \
	        (end) = 1;                                                   \
	} else {                                                             \
	        __next->vmp_pageq.prev = __pck_head;                         \
	        if (VM_PAGE_GET_CLUMP(entry) != VM_PAGE_GET_CLUMP(__next)) { \
	                (end) = 1;                                           \
	        }                                                            \
	}                                                                    \
                                                                             \
	(head)->next = __pck_next;                                           \
	(entry)->vmp_pageq.next = 0;                                         \
	(entry)->vmp_pageq.prev = 0;                                         \
MACRO_END
#endif

  
                           
            
                                                 
              
          
                                      
                       
                              
   
#define vm_page_queue_end(q, qe)        ((q) == (qe))


  
                             
            
                                   
          
                                    
                       
   
#define vm_page_queue_empty(q)          vm_page_queue_end((q), ((vm_page_queue_entry_t)vm_page_queue_first(q)))



  
                             
            
                                         
          
                                   
                                
   
#define vm_page_queue_first(q)          (VM_PAGE_UNPACK_PTR((q)->next))



  
                             
            
                                        
          
                                       
                         
   
#define vm_page_queue_last(q)           (VM_PAGE_UNPACK_PTR((q)->prev))



  
                            
            
                                                 
          
                                   
                        
   
#define vm_page_queue_next(qc)          (VM_PAGE_UNPACK_PTR((qc)->next))



  
                            
            
                                                  
          
                                   
                        
   
#define vm_page_queue_prev(qc)          (VM_PAGE_UNPACK_PTR((qc)->prev))



  
                               
            
                                              
                                          
                                     
          
                                        
               
                   
                                            
   
#define vm_page_queue_iterate(head, elt, field)                       \
	for ((elt) = (vm_page_t)vm_page_queue_first(head);            \
	    !vm_page_queue_end((head), (vm_page_queue_entry_t)(elt)); \
	    (elt) = (vm_page_t)vm_page_queue_next(&(elt)->field))     \

#else        

#define VM_VPLQ_ALIGNMENT               128
#define VM_PAGE_PACKED_PTR_ALIGNMENT    sizeof(vm_offset_t)
#define VM_PAGE_PACKED_ALIGNED
#define VM_PAGE_PACKED_PTR_BITS         32
#define VM_PAGE_PACKED_PTR_SHIFT        0
#define VM_PAGE_PACKED_PTR_BASE         0

#define VM_PAGE_PACKED_FROM_ARRAY       0

#define VM_PAGE_PACK_PTR(p)     (p)
#define VM_PAGE_UNPACK_PTR(p)   ((uintptr_t)(p))

#define VM_PAGE_OBJECT(p)       ((vm_object_t)((p)->vmp_object))
#define VM_PAGE_PACK_OBJECT(o)  ((vm_page_object_t)(VM_PAGE_PACK_PTR(o)))


#define VM_PAGE_ZERO_PAGEQ_ENTRY(p)     \
MACRO_BEGIN                             \
	(p)->vmp_pageq.next = 0;                \
	(p)->vmp_pageq.prev = 0;                \
MACRO_END

#define VM_PAGE_CONVERT_TO_QUEUE_ENTRY(p)   ((queue_entry_t)(p))

#define vm_page_remque                      remque
#define vm_page_enqueue_tail                enqueue_tail
#define vm_page_queue_init                  queue_init
#define vm_page_queue_enter(h, e, f)        queue_enter(h, e, vm_page_t, f)
#define vm_page_queue_enter_first(h, e, f)  queue_enter_first(h, e, vm_page_t, f)
#define vm_page_queue_remove(h, e, f)       queue_remove(h, e, vm_page_t, f)
#define vm_page_queue_remove_first(h, e, f) queue_remove_first(h, e, vm_page_t, f)
#define vm_page_queue_end                   queue_end
#define vm_page_queue_empty                 queue_empty
#define vm_page_queue_first                 queue_first
#define vm_page_queue_last                  queue_last
#define vm_page_queue_next                  queue_next
#define vm_page_queue_prev                  queue_prev
#define vm_page_queue_iterate(h, e, f)      queue_iterate(h, e, vm_page_t, f)

#endif            



  
                                                                      
                                                         
  
                                                                          
                                                                       
                                                                    
                                                                       
                                                                            
                                                                          
                                                                    
                                                                                   
                                    
                                                                     
                                                 
  
                                                               
                                                            
                                                               
                                                             
                                                               
                                                                     
                                                                   
                                                               
                                    
  
                                                                 
                                                            
   
#define VM_PAGE_MAX_SPECULATIVE_AGE_Q   10
#define VM_PAGE_MIN_SPECULATIVE_AGE_Q   1
#define VM_PAGE_SPECULATIVE_AGED_Q      0

#define VM_PAGE_SPECULATIVE_Q_AGE_MS    500

struct vm_speculative_age_q {
	  
                                                            
    
	vm_page_queue_head_t    age_q;
	mach_timespec_t age_ts;
} VM_PAGE_PACKED_ALIGNED;



extern
struct vm_speculative_age_q     vm_page_queue_speculative[];

extern int                      speculative_steal_index;
extern int                      speculative_age_index;
extern unsigned int             vm_page_speculative_q_age_ms;


typedef struct vm_locks_array {
	char    pad  __attribute__ ((aligned(64)));
	lck_mtx_t       vm_page_queue_lock2 __attribute__ ((aligned(64)));
	lck_mtx_t       vm_page_queue_free_lock2 __attribute__ ((aligned(64)));
	char    pad2  __attribute__ ((aligned(64)));
} vm_locks_array_t;


#if CONFIG_BACKGROUND_QUEUE
extern  void    vm_page_assign_background_state(vm_page_t mem);
extern  void    vm_page_update_background_state(vm_page_t mem);
extern  void    vm_page_add_to_backgroundq(vm_page_t mem, boolean_t first);
extern  void    vm_page_remove_from_backgroundq(vm_page_t mem);
#endif

#define VM_PAGE_WIRED(m)        ((m)->vmp_q_state == VM_PAGE_IS_WIRED)
#define NEXT_PAGE(m)            ((m)->vmp_snext)
#define NEXT_PAGE_PTR(m)        (&(m)->vmp_snext)

  
                                                                
                                            
   

  
                                                      
                                         
                                            
                                                         
   

#define VM_PAGE_CHECK(mem)                      \
	MACRO_BEGIN                             \
	MACRO_END

                     
  
                                                             
                                                                
                                                               
                                                           
                                                               
                                                                        
   

#define MAX_COLORS      128
#define DEFAULT_COLORS  32

extern
unsigned int    vm_colors;                                                  
extern
unsigned int    vm_color_mask;                                     
extern
unsigned int    vm_cache_geometry_colors;                                              

  
                                                                            
                                                                    
  
                              
                                                          
  
                                                                           
                                                           
  
                                                                        
                                                       
                                                                   
  
                                                                     
                                                          
   
#if defined(__LP64__)
#define VM_NOT_USER_WIREABLE_MAX (32ULL*1024*1024*1024)               
#else
#define VM_NOT_USER_WIREABLE_MAX (1UL*1024*1024*1024)              
#endif               
extern
vm_map_size_t   vm_per_task_user_wire_limit;
extern
vm_map_size_t   vm_global_user_wire_limit;
extern
uint64_t        vm_add_wire_count_over_global_limit;
extern
uint64_t        vm_add_wire_count_over_user_limit;

  
                                                             
  
       
                                                   
                                              
           
                                               
                                                 
                                            
                                           
                                                 
                                            
                                                   
                                  
         
                                             
                                            
                                 
   


#define VPL_LOCK_SPIN 1

struct vpl {
	vm_page_queue_head_t    vpl_queue;
	unsigned int    vpl_count;
	unsigned int    vpl_internal_count;
	unsigned int    vpl_external_count;
#ifdef  VPL_LOCK_SPIN
	lck_spin_t      vpl_lock;
#else
	lck_mtx_t       vpl_lock;
	lck_mtx_ext_t   vpl_lock_ext;
#endif
};

extern
struct vpl     *                 vm_page_local_q;
extern
unsigned int    vm_page_local_q_soft_limit;
extern
unsigned int    vm_page_local_q_hard_limit;
extern
vm_locks_array_t vm_page_locks;

extern
vm_page_queue_head_t    vm_lopage_queue_free;                                      
extern
vm_page_queue_head_t    vm_page_queue_active;                            
extern
vm_page_queue_head_t    vm_page_queue_inactive;                                             
#if CONFIG_SECLUDED_MEMORY
extern
vm_page_queue_head_t    vm_page_queue_secluded;                                            
#endif                             
extern
vm_page_queue_head_t    vm_page_queue_cleaned;                                  
extern
vm_page_queue_head_t    vm_page_queue_anonymous;                                                       
extern
vm_page_queue_head_t    vm_page_queue_throttled;                                                      

extern
queue_head_t    vm_objects_wired;
extern
lck_spin_t      vm_objects_wired_lock;

#if CONFIG_BACKGROUND_QUEUE

#define VM_PAGE_BACKGROUND_TARGET_MAX   50000

#define VM_PAGE_BG_DISABLED     0
#define VM_PAGE_BG_LEVEL_1      1

extern
vm_page_queue_head_t    vm_page_queue_background;
extern
uint64_t        vm_page_background_promoted_count;
extern
uint32_t        vm_page_background_count;
extern
uint32_t        vm_page_background_target;
extern
uint32_t        vm_page_background_internal_count;
extern
uint32_t        vm_page_background_external_count;
extern
uint32_t        vm_page_background_mode;
extern
uint32_t        vm_page_background_exclude_external;

#endif

extern
vm_offset_t     first_phys_addr;                                             
extern
vm_offset_t     last_phys_addr;                                             

extern
unsigned int    vm_page_free_count;                                                       
extern
unsigned int    vm_page_active_count;                                   
extern
unsigned int    vm_page_inactive_count;                                   
extern
unsigned int vm_page_kernelcache_count;                                                   
#if CONFIG_SECLUDED_MEMORY
extern
unsigned int    vm_page_secluded_count;                                   
extern
unsigned int    vm_page_secluded_count_free;                                 
extern
unsigned int    vm_page_secluded_count_inuse;                                   
  
                                                                
                                        
                                                                              
                                                                
                                                                            
                                                                       
                                                                         
                                                            
   
extern
unsigned int    vm_page_secluded_count_over_target;
#define VM_PAGE_SECLUDED_COUNT_OVER_TARGET_UPDATE()                     \
	MACRO_BEGIN                                                     \
	if (vm_page_secluded_count > vm_page_secluded_target) {         \
	        vm_page_secluded_count_over_target =                    \
	                (vm_page_secluded_count - vm_page_secluded_target); \
	} else {                                                        \
	        vm_page_secluded_count_over_target = 0;                 \
	}                                                               \
	MACRO_END
#define VM_PAGE_SECLUDED_COUNT_OVER_TARGET() vm_page_secluded_count_over_target
#else                             
#define VM_PAGE_SECLUDED_COUNT_OVER_TARGET_UPDATE() \
	MACRO_BEGIN                                 \
	MACRO_END
#define VM_PAGE_SECLUDED_COUNT_OVER_TARGET() 0
#endif                             
extern
unsigned int    vm_page_cleaned_count;                                             
extern
unsigned int    vm_page_throttled_count;                                      
extern
unsigned int    vm_page_speculative_count;                                                     
extern unsigned int     vm_page_pageable_internal_count;
extern unsigned int     vm_page_pageable_external_count;
extern
unsigned int    vm_page_xpmapped_external_count;                                                   
extern
unsigned int    vm_page_external_count;                                      
extern
unsigned int    vm_page_internal_count;                                    
extern
unsigned int    vm_page_wire_count;                                            
extern
unsigned int    vm_page_wire_count_initial;                                          
extern
unsigned int    vm_page_wire_count_on_boot;                                     
extern
unsigned int    vm_page_free_target;                                   
extern
unsigned int    vm_page_free_min;                                   
extern
unsigned int    vm_page_throttle_limit;                                         
extern
unsigned int    vm_page_inactive_target;                                   
#if CONFIG_SECLUDED_MEMORY
extern
unsigned int    vm_page_secluded_target;                                   
#endif                             
extern
unsigned int    vm_page_anonymous_min;                                 
extern
unsigned int    vm_page_free_reserved;                                             
extern
unsigned int    vm_page_gobble_count;
extern
unsigned int    vm_page_stolen_count;                                                      
extern
unsigned int    vm_page_kern_lpage_count;                                                


#if DEVELOPMENT || DEBUG
extern
unsigned int    vm_page_speculative_used;
#endif

extern
unsigned int    vm_page_purgeable_count;                                        
extern
unsigned int    vm_page_purgeable_wired_count;                                              
extern
uint64_t        vm_page_purged_count;                                           

extern unsigned int     vm_page_free_wanted;
                                             

extern unsigned int     vm_page_free_wanted_privileged;
                                                           
#if CONFIG_SECLUDED_MEMORY
extern unsigned int     vm_page_free_wanted_secluded;
                                                      
#endif                             

extern const ppnum_t    vm_page_fictitious_addr;
                                          

extern const ppnum_t    vm_page_guard_addr;
                                     


extern boolean_t        vm_page_deactivate_hint;

extern int              vm_compressor_mode;

  
                                                     
   
extern boolean_t        vm_himemory_mode;

extern boolean_t        vm_lopage_needed;
extern uint32_t         vm_lopage_free_count;
extern uint32_t         vm_lopage_free_limit;
extern uint32_t         vm_lopage_lowater;
extern boolean_t        vm_lopage_refill;
extern uint64_t         max_valid_dma_address;
extern ppnum_t          max_valid_low_ppnum;

  
                                                    
   
extern void             vm_page_bootstrap(
	vm_offset_t     *startp,
	vm_offset_t     *endp);

extern void             vm_page_init_local_q(unsigned int num_cpus);

extern void             vm_page_create(
	ppnum_t         start,
	ppnum_t         end);

extern void             vm_page_create_retired(
	ppnum_t         pn);

extern vm_page_t        kdp_vm_page_lookup(
	vm_object_t             object,
	vm_object_offset_t      offset);

extern vm_page_t        vm_page_lookup(
	vm_object_t             object,
	vm_object_offset_t      offset);

extern vm_page_t        vm_page_grab_fictitious(boolean_t canwait);

extern vm_page_t        vm_page_grab_guard(boolean_t canwait);

extern void             vm_page_release_fictitious(
	vm_page_t page);

extern void             vm_free_delayed_pages(void);

extern bool             vm_pool_low(void);

extern vm_page_t        vm_page_grab(void);
extern vm_page_t        vm_page_grab_options(int flags);

#define VM_PAGE_GRAB_OPTIONS_NONE 0x00000000
#if CONFIG_SECLUDED_MEMORY
#define VM_PAGE_GRAB_SECLUDED     0x00000001
#endif                             
#define VM_PAGE_GRAB_Q_LOCK_HELD  0x00000002

extern vm_page_t        vm_page_grablo(void);

extern void             vm_page_release(
	vm_page_t       page,
	boolean_t       page_queues_locked);

extern boolean_t        vm_page_wait(
	int             interruptible );

extern vm_page_t        vm_page_alloc(
	vm_object_t             object,
	vm_object_offset_t      offset);

extern void             vm_page_init(
	vm_page_t       page,
	ppnum_t         phys_page,
	boolean_t       lopage);

extern void             vm_page_free(
	vm_page_t       page);

extern void             vm_page_free_unlocked(
	vm_page_t       page,
	boolean_t       remove_from_hash);

extern void             vm_page_balance_inactive(
	int             max_to_move);

extern void             vm_page_activate(
	vm_page_t       page);

extern void             vm_page_deactivate(
	vm_page_t       page);

extern void             vm_page_deactivate_internal(
	vm_page_t       page,
	boolean_t       clear_hw_reference);

extern void             vm_page_enqueue_cleaned(vm_page_t page);

extern void             vm_page_lru(
	vm_page_t       page);

extern void             vm_page_speculate(
	vm_page_t       page,
	boolean_t       new);

extern void             vm_page_speculate_ageit(
	struct vm_speculative_age_q *aq);

extern void             vm_page_reactivate_all_throttled(void);

extern void             vm_page_reactivate_local(uint32_t lid, boolean_t force, boolean_t nolocks);

extern void             vm_page_rename(
	vm_page_t               page,
	vm_object_t             new_object,
	vm_object_offset_t      new_offset);

extern void             vm_page_insert(
	vm_page_t               page,
	vm_object_t             object,
	vm_object_offset_t      offset);

extern void             vm_page_insert_wired(
	vm_page_t               page,
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_tag_t                tag);

extern void             vm_page_insert_internal(
	vm_page_t               page,
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_tag_t                tag,
	boolean_t               queues_lock_held,
	boolean_t               insert_in_hash,
	boolean_t               batch_pmap_op,
	boolean_t               delayed_accounting,
	uint64_t                *delayed_ledger_update);

extern void             vm_page_replace(
	vm_page_t               mem,
	vm_object_t             object,
	vm_object_offset_t      offset);

extern void             vm_page_remove(
	vm_page_t       page,
	boolean_t       remove_from_hash);

extern void             vm_page_zero_fill(
	vm_page_t       page);

extern void             vm_page_part_zero_fill(
	vm_page_t       m,
	vm_offset_t     m_pa,
	vm_size_t       len);

extern void             vm_page_copy(
	vm_page_t       src_page,
	vm_page_t       dest_page);

extern void             vm_page_part_copy(
	vm_page_t       src_m,
	vm_offset_t     src_pa,
	vm_page_t       dst_m,
	vm_offset_t     dst_pa,
	vm_size_t       len);

extern void             vm_page_wire(
	vm_page_t       page,
	vm_tag_t        tag,
	boolean_t       check_memorystatus);

extern void             vm_page_unwire(
	vm_page_t       page,
	boolean_t       queueit);

extern void             vm_set_page_size(void);

extern void             vm_page_gobble(
	vm_page_t      page);

extern void             vm_page_validate_cs(
	vm_page_t       page,
	vm_map_size_t   fault_page_size,
	vm_map_offset_t fault_phys_offset);
extern void             vm_page_validate_cs_mapped(
	vm_page_t       page,
	vm_map_size_t   fault_page_size,
	vm_map_offset_t fault_phys_offset,
	const void      *kaddr);
extern void             vm_page_validate_cs_mapped_slow(
	vm_page_t       page,
	const void      *kaddr);
extern void             vm_page_validate_cs_mapped_chunk(
	vm_page_t       page,
	const void      *kaddr,
	vm_offset_t     chunk_offset,
	vm_size_t       chunk_size,
	boolean_t       *validated,
	unsigned        *tainted);

extern void             vm_page_free_prepare_queues(
	vm_page_t       page);

extern void             vm_page_free_prepare_object(
	vm_page_t       page,
	boolean_t       remove_from_hash);

#if CONFIG_IOSCHED
extern wait_result_t    vm_page_sleep(
	vm_object_t     object,
	vm_page_t       m,
	int     interruptible);
#endif

extern void vm_pressure_response(void);

#if CONFIG_JETSAM
extern void memorystatus_pages_update(unsigned int pages_avail);

#define VM_CHECK_MEMORYSTATUS do { \
	memorystatus_pages_update(              \
	        vm_page_pageable_external_count + \
	        vm_page_free_count +            \
	        VM_PAGE_SECLUDED_COUNT_OVER_TARGET() + \
	        (VM_DYNAMIC_PAGING_ENABLED() ? 0 : vm_page_purgeable_count) \
	        ); \
	} while(0)

#else                    

#if !XNU_TARGET_OS_OSX

#define VM_CHECK_MEMORYSTATUS do {} while(0)

#else                         

#define VM_CHECK_MEMORYSTATUS   vm_pressure_response()

#endif                         

#endif                    

  
                                                                     
                                
   

#if !XNU_TARGET_OS_OSX
#define SET_PAGE_DIRTY(m, set_pmap_modified)                            \
	        MACRO_BEGIN                                             \
	        vm_page_t __page__ = (m);                               \
	        if (__page__->vmp_pmapped == TRUE &&                    \
	            __page__->vmp_wpmapped == TRUE &&                   \
	            __page__->vmp_dirty == FALSE &&                     \
	            (set_pmap_modified)) {                              \
	                pmap_set_modify(VM_PAGE_GET_PHYS_PAGE(__page__)); \
	        }                                                       \
	        __page__->vmp_dirty = TRUE;                             \
	        MACRO_END
#else                         
#define SET_PAGE_DIRTY(m, set_pmap_modified)                            \
	        MACRO_BEGIN                                             \
	        vm_page_t __page__ = (m);                               \
	        __page__->vmp_dirty = TRUE;                             \
	        MACRO_END
#endif                         

#define PAGE_ASSERT_WAIT(m, interruptible)                      \
	        (((m)->vmp_wanted = TRUE),                      \
	         assert_wait((event_t) (m), (interruptible)))

#if CONFIG_IOSCHED
#define PAGE_SLEEP(o, m, interruptible)                         \
	        vm_page_sleep(o, m, interruptible)
#else
#define PAGE_SLEEP(o, m, interruptible)                         \
	(((m)->vmp_wanted = TRUE),                              \
	 thread_sleep_vm_object((o), (m), (interruptible)))
#endif

#define PAGE_WAKEUP_DONE(m)                                     \
	        MACRO_BEGIN                                     \
	        (m)->vmp_busy = FALSE;                          \
	        if ((m)->vmp_wanted) {                          \
	                (m)->vmp_wanted = FALSE;                \
	                thread_wakeup((event_t) (m));           \
	        }                                               \
	        MACRO_END

#define PAGE_WAKEUP(m)                                          \
	        MACRO_BEGIN                                     \
	        if ((m)->vmp_wanted) {                          \
	                (m)->vmp_wanted = FALSE;                \
	                thread_wakeup((event_t) (m));           \
	        }                                               \
	        MACRO_END

#define VM_PAGE_FREE(p)                         \
	        MACRO_BEGIN                     \
	        vm_page_free_unlocked(p, TRUE); \
	        MACRO_END

#define VM_PAGE_WAIT()          ((void)vm_page_wait(THREAD_UNINT))

#define vm_page_queue_lock (vm_page_locks.vm_page_queue_lock2)
#define vm_page_queue_free_lock (vm_page_locks.vm_page_queue_free_lock2)

#define vm_page_lock_queues()   lck_mtx_lock(&vm_page_queue_lock)
#define vm_page_trylock_queues() lck_mtx_try_lock(&vm_page_queue_lock)
#define vm_page_unlock_queues() lck_mtx_unlock(&vm_page_queue_lock)

#define vm_page_lockspin_queues()       lck_mtx_lock_spin(&vm_page_queue_lock)
#define vm_page_trylockspin_queues()    lck_mtx_try_lock_spin(&vm_page_queue_lock)
#define vm_page_lockconvert_queues()    lck_mtx_convert_spin(&vm_page_queue_lock)

#ifdef  VPL_LOCK_SPIN
extern lck_grp_t vm_page_lck_grp_local;

#define VPL_LOCK_INIT(vlq, vpl_grp, vpl_attr) lck_spin_init(&vlq->vpl_lock, vpl_grp, vpl_attr)
#define VPL_LOCK(vpl) lck_spin_lock_grp(vpl, &vm_page_lck_grp_local)
#define VPL_UNLOCK(vpl) lck_spin_unlock(vpl)
#else
#define VPL_LOCK_INIT(vlq, vpl_grp, vpl_attr) lck_mtx_init_ext(&vlq->vpl_lock, &vlq->vpl_lock_ext, vpl_grp, vpl_attr)
#define VPL_LOCK(vpl) lck_mtx_lock_spin(vpl)
#define VPL_UNLOCK(vpl) lck_mtx_unlock(vpl)
#endif


#if DEVELOPMENT || DEBUG
#define VM_PAGE_SPECULATIVE_USED_ADD()                          \
	MACRO_BEGIN                                             \
	OSAddAtomic(1, &vm_page_speculative_used);              \
	MACRO_END
#else
#define VM_PAGE_SPECULATIVE_USED_ADD()
#endif


#define VM_PAGE_CONSUME_CLUSTERED(mem)                          \
	MACRO_BEGIN                                             \
	ppnum_t	__phys_page;                                    \
	__phys_page = VM_PAGE_GET_PHYS_PAGE(mem);               \
	pmap_lock_phys_page(__phys_page);                       \
	if (mem->vmp_clustered) {                               \
	        vm_object_t o;                                  \
	        o = VM_PAGE_OBJECT(mem);                        \
	        assert(o);                                      \
	        o->pages_used++;                                \
	        mem->vmp_clustered = FALSE;                     \
	        VM_PAGE_SPECULATIVE_USED_ADD();                 \
	}                                                       \
	pmap_unlock_phys_page(__phys_page);                     \
	MACRO_END


#define VM_PAGE_COUNT_AS_PAGEIN(mem)                            \
	MACRO_BEGIN                                             \
	{                                                       \
	vm_object_t o;                                          \
	o = VM_PAGE_OBJECT(mem);                                \
	DTRACE_VM2(pgin, int, 1, (uint64_t *), NULL);           \
	current_task()->pageins++;                              \
	if (o->internal) {                                      \
	        DTRACE_VM2(anonpgin, int, 1, (uint64_t *), NULL);       \
	} else {                                                \
	        DTRACE_VM2(fspgin, int, 1, (uint64_t *), NULL); \
	}                                                       \
	}                                                       \
	MACRO_END

                                                 
#define VM_PAGE_MOVE_STOLEN(page_count)                         \
	MACRO_BEGIN                                             \
	vm_page_stolen_count -=	(page_count);                   \
	vm_page_wire_count_initial -= (page_count);             \
	MACRO_END

#define DW_vm_page_unwire               0x01
#define DW_vm_page_wire                 0x02
#define DW_vm_page_free                 0x04
#define DW_vm_page_activate             0x08
#define DW_vm_page_deactivate_internal  0x10
#define DW_vm_page_speculate            0x20
#define DW_vm_page_lru                  0x40
#define DW_vm_pageout_throttle_up       0x80
#define DW_PAGE_WAKEUP                  0x100
#define DW_clear_busy                   0x200
#define DW_clear_reference              0x400
#define DW_set_reference                0x800
#define DW_move_page                    0x1000
#define DW_VM_PAGE_QUEUES_REMOVE        0x2000
#define DW_enqueue_cleaned              0x4000
#define DW_vm_phantom_cache_update      0x8000

struct vm_page_delayed_work {
	vm_page_t       dw_m;
	int             dw_mask;
};

#define DEFAULT_DELAYED_WORK_LIMIT      32

struct vm_page_delayed_work_ctx {
	struct vm_page_delayed_work dwp[DEFAULT_DELAYED_WORK_LIMIT];
	thread_t delayed_owner;
};

void vm_page_do_delayed_work(vm_object_t object, vm_tag_t tag, struct vm_page_delayed_work *dwp, int dw_count);

extern unsigned int vm_max_delayed_work_limit;

extern void vm_page_delayed_work_init_ctx(void);

#define DELAYED_WORK_LIMIT(max) ((vm_max_delayed_work_limit >= max ? max : vm_max_delayed_work_limit))

  
                                                              
                                                   
                                                            
                                                         
                                                           
                                          
   

#define VM_PAGE_ADD_DELAYED_WORK(dwp, mem, dw_cnt)              \
	MACRO_BEGIN                                             \
	if (mem->vmp_busy == FALSE) {                           \
	        mem->vmp_busy = TRUE;                           \
	        if ( !(dwp->dw_mask & DW_vm_page_free))         \
	                dwp->dw_mask |= (DW_clear_busy | DW_PAGE_WAKEUP); \
	}                                                       \
	dwp->dw_m = mem;                                        \
	dwp++;                                                  \
	dw_cnt++;                                               \
	MACRO_END

extern vm_page_t vm_object_page_grab(vm_object_t);

#if VM_PAGE_BUCKETS_CHECK
extern void vm_page_buckets_check(void);
#endif                            

extern void vm_page_queues_remove(vm_page_t mem, boolean_t remove_from_backgroundq);
extern void vm_page_remove_internal(vm_page_t page);
extern void vm_page_enqueue_inactive(vm_page_t mem, boolean_t first);
extern void vm_page_enqueue_active(vm_page_t mem, boolean_t first);
extern void vm_page_check_pageable_safe(vm_page_t page);

#if CONFIG_SECLUDED_MEMORY
extern uint64_t secluded_shutoff_trigger;
extern uint64_t secluded_shutoff_headroom;
extern void start_secluded_suppression(task_t);
extern void stop_secluded_suppression(task_t);
#endif                             

extern void vm_retire_boot_pages(void);
extern uint32_t vm_retired_pages_count(void);

#endif                      
