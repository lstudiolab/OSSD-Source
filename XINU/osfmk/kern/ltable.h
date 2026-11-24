/*
 * Copyright (c) 2016 Apple Inc. All rights reserved.
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
#ifdef XNU_KERNEL_PRIVATE

#include <kern/kern_types.h>
#include <machine/locks.h>

#if CONFIG_LTABLE_DEBUG
#define ltdbg(fmt, ...) \
	printf("LT[%s]:  " fmt "\n", __func__, ## __VA_ARGS__)
#else
#define ltdbg(fmt, ...) do { } while (0)
#endif

#ifdef LTABLE_VERBOSE_DEBUG
#define ltdbg_v(fmt, ...) \
	printf("LT[v:%s]:  " fmt "\n", __func__, ## __VA_ARGS__)
#else
#define ltdbg_v(fmt, ...) do { } while (0)
#endif

#define ltinfo(fmt, ...) \
	printf("LT[%s]: " fmt "\n", __func__,  ## __VA_ARGS__)

#define lterr(fmt, ...) \
	printf("LT[%s] ERROR: " fmt "\n", __func__, ## __VA_ARGS__)



                                                                         
  
                                
  
                                                                            

struct ltable_id {
	union {
		uint64_t id;
		struct {
			  
                                                  
                                        
      
			uint64_t idx:18,                                                  
			    generation:46;
		};
	};
};

                                                                       
#define LT_IDX_MAX  (0x3ffff)

extern vm_size_t     g_lt_max_tbl_size;


struct lt_elem {
	struct ltable_id lt_id;
	uint32_t lt_bits;
	uint32_t lt_next_idx;
};

                                                                
#define LT_BITS_REFCNT_MASK  (0x1FFFFFFFU)
#define LT_BITS_REFCNT_SHIFT (0)
#define LT_BITS_REFCNT       (LT_BITS_REFCNT_MASK << LT_BITS_REFCNT_SHIFT)

#define LT_BITS_TYPE_MASK    (0x3U)
#define LT_BITS_TYPE_SHIFT   (29)
#define LT_BITS_TYPE         (LT_BITS_TYPE_MASK << LT_BITS_TYPE_SHIFT)

#define LT_BITS_VALID_MASK   (0x1U)
#define LT_BITS_VALID_SHIFT  (31)
#define LT_BITS_VALID        (LT_BITS_VALID_MASK << LT_BITS_VALID_SHIFT)

#define lt_bits_refcnt(bits) \
	(((bits) >> LT_BITS_REFCNT_SHIFT) & LT_BITS_REFCNT_MASK)

#define lt_bits_type(bits) \
	(((bits) >> LT_BITS_TYPE_SHIFT) & LT_BITS_TYPE_MASK)

#define lt_bits_valid(bits) \
	((bits) & LT_BITS_VALID)

enum lt_elem_type {
	LT_FREE     = 0,
	LT_ELEM     = 1,
	LT_LINK     = 2,
	LT_RESERVED = 3,
};

struct link_table;
typedef void (*ltable_poison_func)(struct link_table *, struct lt_elem *);

  
                       
  
                                                                            
                                                                           
                                                                              
                                                                            
                                                                             
                                   
   
struct link_table {
	struct lt_elem **table;                                        
	struct lt_elem **next_free_slab;
	struct ltable_id free_head __attribute__((aligned(8)));
	struct lt_elem  *free_tail;

	uint32_t         elem_sz;                                       
	uint32_t         slab_shift;
	uint32_t         slab_msk;
	uint32_t         slab_elem;
	uint32_t         slab_sz;                                             

	uint32_t         nelem;
	zone_t           slab_zone;

	ltable_poison_func poison;

	lck_mtx_t        lock;
	uint32_t         state;
	uint32_t volatile used_elem;

#if CONFIG_LTABLE_STATS
	uint32_t         nslabs;

	uint64_t         nallocs;
	uint64_t         nreallocs;
	uint64_t         npreposts;
	int64_t          nreservations;
	uint64_t         nreserved_releases;
	uint64_t         nspins;

	uint64_t         max_used;
	uint64_t         avg_used;
	uint64_t         max_reservations;
	uint64_t         avg_reservations;
#endif
} __attribute__((aligned(8)));

   
                                                             
  
   
extern void ltable_init(struct link_table *table, const char *name,
    uint32_t max_tbl_elem, uint32_t elem_sz,
    ltable_poison_func poison);


   
                                                                            
  
              
                          
                           
   
extern void ltable_grow(struct link_table *table, uint32_t min_free);


   
                                                                      
  
                                                                           
  
                                                                              
                                                                              
                     
  
                                                                       
                                                             
   
extern __attribute__((noinline))
struct lt_elem *ltable_alloc_elem(struct link_table *table, int type,
    int nelem, int nattempts);


#if DEVELOPMENT || DEBUG
   
                                                           
         
   
extern
int ltable_nelem(struct link_table *table);
#endif

   
                                                                       
  
                                                                            
                                                                            
                                                                            
          
   
extern void ltable_realloc_elem(struct link_table *table,
    struct lt_elem *elem, int type);


   
                                                                         
  
                                                                              
                                                                           
                                                                
  
                                                                      
                                        
   
extern struct lt_elem *ltable_get_elem(struct link_table *table, uint64_t id);


   
                                                        
  
                                                                  
                                                                            
                                                                        
           
   
extern void ltable_put_elem(struct link_table *table, struct lt_elem *elem);


   
                                             
  
                                                         
   
extern void lt_elem_invalidate(struct lt_elem *elem);


   
                                        
  
                                                         
   
extern void lt_elem_mkvalid(struct lt_elem *elem);


                                                                                
  
                      
  
                                                                            
                                                                          
                                                                  
  
                                                                                

   
                                                
  
                                                                              
                                                                       
                            
                                                        
   
extern int lt_elem_list_link(struct link_table *table,
    struct lt_elem *parent, struct lt_elem *child);


   
                                                                       
  
                                                                             
                                                      
  
                                                                           
                        
   
extern struct lt_elem *lt_elem_list_first(struct link_table *table, uint64_t id);


   
                                                                    
  
                                                                             
   
extern struct lt_elem *lt_elem_list_next(struct link_table *table,
    struct lt_elem *elem);


   
                                                        
  
                                                                              
                                                                     
                        
   
extern struct lt_elem *lt_elem_list_break(struct link_table *table,
    struct lt_elem *elem);


   
                                                       
  
                                                                              
                                                                             
                                                                        
                                                                              
                         
   
extern struct lt_elem *lt_elem_list_pop(struct link_table *table,
    uint64_t *id, int type);


   
                                                                 
  
                                                                              
                                                                           
                                                                         
   
extern int lt_elem_list_release(struct link_table *table,
    struct lt_elem *head,
    int __assert_only type);

static inline int
lt_elem_list_release_id(struct link_table *table,
    uint64_t id, int type)
{
	return lt_elem_list_release(table, lt_elem_list_first(table, id), type);
}

#endif                         
