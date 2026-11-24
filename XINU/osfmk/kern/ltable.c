/*
 * Copyright (c) 2016-2020 Apple Inc. All rights reserved.
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
#include <kern/cpu_data.h>
#include <kern/kern_types.h>
#include <kern/clock.h>
#include <kern/locks.h>
#include <kern/ltable.h>
#include <kern/zalloc.h>
#include <libkern/OSAtomic.h>
#include <pexpert/pexpert.h>
#include <vm/vm_kern.h>


#define P2ROUNDUP(x, align) (-(-((uintptr_t)(x)) & -((uintptr_t)align)))
#define ROUNDDOWN(x, y)  (((x)/(y))*(y))

                                                                         
  
                                
  
                                                                            

                                                       
#define DEFAULT_MAX_TABLE_SIZE  P2ROUNDUP(8 * 1024 * 1024, PAGE_SIZE)

TUNABLE(vm_size_t, g_lt_max_tbl_size, "lt_tbl_size", 0);
LCK_GRP_DECLARE(g_lt_lck_grp, "link_table_locks");

#if DEVELOPMENT || DEBUG
                            
uint64_t g_lt_idx_max = LT_IDX_MAX;
#endif

__startup_func
static void
ltable_startup_tunables_init(void)
{
	                                                             
	                           
	  
	                                                     
	                                 
	if (g_lt_max_tbl_size == 0) {
		g_lt_max_tbl_size = (typeof(g_lt_max_tbl_size))DEFAULT_MAX_TABLE_SIZE;
	} else {
		g_lt_max_tbl_size = round_page(g_lt_max_tbl_size);
	}
}
STARTUP(TUNABLES, STARTUP_RANK_MIDDLE, ltable_startup_tunables_init);


                                                                        
#define lt_elem_ofst_slab(slab, slab_msk, ofst) \
	                                                                         \
	((struct lt_elem *)((void *)((uintptr_t)(slab) + ((ofst) & (slab_msk)))))

#if CONFIG_LTABLE_STATS
                                                             
static inline struct lt_elem *
lt_elem_idx(struct link_table *table, uint32_t idx)
{
	int slab_idx = idx / table->slab_elem;
	struct lt_elem *slab = table->table[slab_idx];
	if (!slab) {
		panic("Invalid index:%d slab:%d (NULL) for table:%p\n",
		    idx, slab_idx, table);
	}
	assert(slab->lt_id.idx <= idx && (slab->lt_id.idx + table->slab_elem) > idx);
	return lt_elem_ofst_slab(slab, table->slab_msk, (idx - slab->lt_id.idx) * table->elem_sz);
}
#else                           
                                                               
static inline struct lt_elem *
lt_elem_idx(struct link_table *table, uint32_t idx)
{
	uint32_t ofst = idx * table->elem_sz;
	struct lt_elem *slab = table->table[ofst >> table->slab_shift];
	if (!slab) {
		panic("Invalid index:%d slab:%d (NULL) for table:%p\n",
		    idx, (ofst >> table->slab_shift), table);
	}
	assert(slab->lt_id.idx <= idx && (slab->lt_id.idx + table->slab_elem) > idx);
	return lt_elem_ofst_slab(slab, table->slab_msk, ofst);
}
#endif                          

static int __assert_only
lt_elem_in_range(struct lt_elem *elem, struct link_table *table)
{
	struct lt_elem **base = table->table;
	uintptr_t e = (uintptr_t)elem;
	assert(base != NULL);
	while (*base != NULL) {
		uintptr_t b = (uintptr_t)(*base);
		if (e >= b && e < b + table->slab_sz) {
			return 1;
		}
		base++;
		if ((uintptr_t)base >= (uintptr_t)table->table + PAGE_SIZE) {
			return 0;
		}
	}
	return 0;
}


   
                                             
  
                                                         
   
void
lt_elem_invalidate(struct lt_elem *elem)
{
	uint32_t __assert_only old = OSBitAndAtomic(~LT_BITS_VALID, &elem->lt_bits);
	OSMemoryBarrier();
	assert(((lt_bits_type(old) != LT_RESERVED) && (old & LT_BITS_VALID)) ||
	    ((lt_bits_type(old) == LT_RESERVED) && !(old & LT_BITS_VALID)));
}

   
                                        
  
                                                         
   
void
lt_elem_mkvalid(struct lt_elem *elem)
{
	uint32_t __assert_only old = OSBitOrAtomic(LT_BITS_VALID, &elem->lt_bits);
	OSMemoryBarrier();
	assert(!(old & LT_BITS_VALID));
}

static void
lt_elem_set_type(struct lt_elem *elem, int type)
{
	uint32_t old_bits, new_bits;
	do {
		old_bits = elem->lt_bits;
		new_bits = (old_bits & ~LT_BITS_TYPE) |
		    ((type & LT_BITS_TYPE_MASK) << LT_BITS_TYPE_SHIFT);
	} while (OSCompareAndSwap(old_bits, new_bits, &elem->lt_bits) == FALSE);
	OSMemoryBarrier();
}

static void
ltable_enqueue_list(struct link_table *table,
    struct lt_elem *first, struct lt_elem *last)
{
	uint32_t idx = first->lt_id.idx;
	struct lt_elem *prev;

	last->lt_next_idx = LT_IDX_MAX;

	  
                                                                     
                                                  
   
                                                                
                          
    
	disable_preemption();
	prev = os_atomic_xchg(&table->free_tail, last, release);
	                     
	os_atomic_store(&prev->lt_next_idx, idx, relaxed);
	enable_preemption();
}


   
                                                             
  
   
void
ltable_init(struct link_table *table, const char *name,
    uint32_t max_tbl_elem, uint32_t elem_sz,
    ltable_poison_func poison)
{
	kern_return_t kr;
	uint32_t slab_sz, slab_shift, slab_msk, slab_elem;
	zone_t slab_zone;
	size_t max_tbl_sz;
	struct lt_elem *e, **base;

#ifndef CONFIG_LTABLE_STATS
	                                                
	if ((elem_sz & (elem_sz - 1)) != 0) {
		panic("elem_sz:%d for table:'%s' must be a power of two!",
		    elem_sz, name);
	}
#endif

	  
                                                              
                                 
    
	kr = kernel_memory_allocate(kernel_map, (vm_offset_t *)&base,
	    PAGE_SIZE, 0, KMA_NOPAGEWAIT | KMA_ZERO, VM_KERN_MEMORY_LTABLE);
	if (kr != KERN_SUCCESS) {
		panic("Cannot initialize %s table: "
		    "kernel_memory_allocate failed:%d\n", name, kr);
	}

	  
                                                             
                                                                     
                                                                     
                                           
    
	max_tbl_sz = (max_tbl_elem * elem_sz);
	max_tbl_sz = P2ROUNDUP(max_tbl_sz, PAGE_SIZE);

	                                                                    
	slab_sz = (uint32_t)(max_tbl_sz / (PAGE_SIZE / (sizeof(void *))));
	if (slab_sz < PAGE_SIZE) {
		slab_sz = PAGE_SIZE;
	}

	                                               
	slab_shift = 0;
	slab_msk = ~0;
	for (uint32_t i = 0; i < 31; i++) {
		uint32_t bit = (1 << i);
		if ((slab_sz & bit) == slab_sz) {
			slab_shift = i;
			slab_msk = 0;
			for (uint32_t j = 0; j < i; j++) {
				slab_msk |= (1 << j);
			}
			break;
		}
		slab_sz &= ~bit;
	}
	slab_elem = slab_sz / elem_sz;

	                                                         
	ltdbg("Initializing %s zone: slab:%d (%d,0x%x) max:%ld",
	    name, slab_sz, slab_shift, slab_msk, max_tbl_sz);
	slab_zone = zone_create(name, slab_sz, ZC_NONE);
	assert(slab_zone != ZONE_NULL);

	                                             
	base[0] = (struct lt_elem *)zalloc(slab_zone);
	if (base[0] == NULL) {
		panic("Can't allocate a %s table slab from zone:%p",
		    name, slab_zone);
	}

	memset(base[0], 0, slab_sz);

	                                
	ltdbg("initializing %d links (%d bytes each)...", slab_elem, elem_sz);
	for (unsigned l = 0; l < slab_elem; l++) {
		e = lt_elem_ofst_slab(base[0], slab_msk, l * elem_sz);
		e->lt_id.idx = l;
		  
                                                         
                                                              
                               
     
		e->lt_id.generation = 0;
		e->lt_next_idx = l + 1;
	}

	                                                                 
	e = lt_elem_ofst_slab(base[0], slab_msk, (slab_elem - 1) * elem_sz);
	e->lt_next_idx = LT_IDX_MAX;

	lck_mtx_init(&table->lock, &g_lt_lck_grp, LCK_ATTR_NULL);

	table->slab_sz = slab_sz;
	table->slab_shift = slab_shift;
	table->slab_msk = slab_msk;
	table->slab_elem = slab_elem;
	table->slab_zone = slab_zone;

	table->elem_sz = elem_sz;
	table->nelem = slab_elem;
	os_atomic_init(&table->used_elem, 1);                            
	table->elem_sz = elem_sz;
	table->poison = poison;

	table->table = base;
	table->next_free_slab = &base[1];
	os_atomic_init(&table->free_head, base[0]->lt_id);
	os_atomic_init(&table->free_tail, e);

#if CONFIG_LTABLE_STATS
	table->nslabs = 1;
	table->nallocs = 0;
	table->nreallocs = 0;
	table->npreposts = 0;
	table->nreservations = 0;
	table->nreserved_releases = 0;

	table->max_used = 0;
	table->avg_used = 0;
	table->max_reservations = 0;
	table->avg_reservations = 0;
#endif
}


   
                                                                            
  
              
                          
                           
   
void
ltable_grow(struct link_table *table, uint32_t min_free)
{
	struct lt_elem *slab, **slot;
	struct lt_elem *e = NULL, *first_new_elem, *last_new_elem;
	uint32_t free_elem;

	assert(get_preemption_level() == 0);
	assert(table && table->slab_zone);

	lck_mtx_lock(&table->lock);

	free_elem = table->nelem - os_atomic_load(&table->used_elem, relaxed);

	  
                                                                     
                                                                     
                                                                   
                                                                      
             
    
	if (free_elem > min_free) {
		lck_mtx_unlock(&table->lock);
		return;
	}

	                                          
	ltdbg_v("BEGIN");

	if (table->next_free_slab == NULL) {
		  
                                                             
                                                 
     
		if ((table->nelem - os_atomic_load(&table->used_elem, relaxed)) > 0) {
			                                                      
			lck_mtx_unlock(&table->lock);
			return;
		}
		panic("No more room to grow table: %p (nelem: %d, used: %d)",
		    table, table->nelem,
		    os_atomic_load(&table->used_elem, relaxed));
	}
	slot = table->next_free_slab;
	table->next_free_slab++;
	if ((uintptr_t)table->next_free_slab >= (uintptr_t)table->table + PAGE_SIZE) {
		table->next_free_slab = NULL;
	}

	assert(*slot == NULL);

	                           
	slab = (struct lt_elem *)zalloc(table->slab_zone);
	if (slab == NULL) {
		panic("Can't allocate a %s%s table (%p) slab from zone:%p",
		    zone_heap_name(table->slab_zone), zone_name(table->slab_zone),
		    table, table->slab_zone);
	}

	memset(slab, 0, table->slab_sz);

	                                          
	ltdbg_v("    init %d new links...", table->slab_elem);
	for (unsigned l = 0; l < table->slab_elem; l++) {
		uint32_t idx = l + table->nelem;
		if (idx >= (LT_IDX_MAX - 1)) {
			break;                                        
		}
		e = lt_elem_ofst_slab(slab, table->slab_msk, l * table->elem_sz);
		e->lt_id.idx = idx;
		e->lt_next_idx = idx + 1;
	}
	last_new_elem = e;
	assert(last_new_elem != NULL);

	first_new_elem = lt_elem_ofst_slab(slab, table->slab_msk, 0);

	                                                                      
	*slot = slab;
	if (table->nelem + table->slab_elem >= LT_IDX_MAX) {
		table->nelem = LT_IDX_MAX - 1;
	} else {
		table->nelem += table->slab_elem;
	}

#if CONFIG_LTABLE_STATS
	table->nslabs += 1;
#endif

	  
                                                                
                                                                  
                     
    
	ltable_enqueue_list(table, first_new_elem, last_new_elem);

	lck_mtx_unlock(&table->lock);

	return;
}

#if DEVELOPMENT || DEBUG

int
ltable_nelem(struct link_table *table)
{
	return os_atomic_load(&table->used_elem, relaxed);
}
#endif

   
                                                                      
  
                                                                           
  
                                                                              
                                                                              
                     
  
                                                                       
                                                             
   
__attribute__((noinline))
struct lt_elem *
ltable_alloc_elem(struct link_table *table, int type,
    int nelem, int nattempts)
{
	int nspins = 0, ntries = 0, nalloc = 0;
	uint32_t table_size;
	struct lt_elem *elem = NULL;
	struct ltable_id free_id, next_id;

	static const int max_retries = 500;

	if (type != LT_ELEM && type != LT_LINK && type != LT_RESERVED) {
		panic("link_table_aloc of invalid elem type:%d from table @%p",
		    type, table);
	}

	assert(nelem > 0);

	  
                                                                       
                                                                      
    
	if (nattempts > 0 && nattempts <= max_retries) {
		ntries = max_retries - nattempts;
	}

try_again:
	elem = NULL;
	if (ntries++ > max_retries) {
		struct lt_elem *tmp;
		if (nattempts > 0) {
			  
                                                 
                                                    
                                               
      
			return NULL;
		}

		if (table->used_elem + nelem >= table_size) {
			panic("No more room to grow table: 0x%p size:%d, used:%d, requested elem:%d",
			    table, table_size, table->used_elem, nelem);
		}
		if (nelem == 1) {
			panic("Too many alloc retries: %d, table:%p, type:%d, nelem:%d",
			    ntries, table, type, nelem);
		}
		                                               
		while (nelem > 0) {
			tmp = ltable_alloc_elem(table, type, 1, nattempts);
			if (elem) {
				lt_elem_list_link(table, tmp, elem);
			}
			elem = tmp;
			--nelem;
		}
		assert(elem != NULL);
		return elem;
	}

	nalloc = 0;
	table_size = table->nelem;

	if (table->used_elem + nelem >= table_size) {
		if (get_preemption_level() != 0) {
#if CONFIG_LTABLE_STATS
			table->nspins += 1;
#endif
			  
                                                     
                                                      
      
			if (++nspins > 4) {
				panic("Can't grow table %p with preemption"
				    " disabled!", table);
			}
			delay(1);
			goto try_again;
		}
		ltable_grow(table, nelem);
		goto try_again;
	}

	                                              
	free_id = *(struct ltable_id volatile *)&table->free_head;
	if (free_id.idx >= table_size) {
		goto try_again;
	}

	  
                                                                      
                                                                     
                                                                   
                                                                    
                                                   
   
                                                                        
                                                                        
                                                             
    
	for (struct lt_elem *next_elem = lt_elem_idx(table, free_id.idx);
	    nalloc < nelem;
	    nalloc++) {
		uint32_t idx = os_atomic_load(&next_elem->lt_next_idx, relaxed);

		if (idx >= table->nelem) {
			  
                                                           
                                                           
                                                    
     
                                                        
                                      
     
                                                   
      
			goto try_again;
		}

		if (os_atomic_load(&next_elem->lt_bits, relaxed)) {
			  
                                                    
               
     
                                                   
                                               
            
      
			goto try_again;
		}

		elem = next_elem;
		next_elem = lt_elem_idx(table, idx);
	}

	                    
	next_id.generation = free_id.generation + 1;
	next_id.idx = elem->lt_next_idx;

	                                                       
	if (!os_atomic_cmpxchg(&table->free_head, free_id, next_id, acquire)) {
		goto try_again;
	}

	  
                                                                  
                                                                 
                      
    
	assert(elem != os_atomic_load(&table->free_tail, relaxed));
	os_atomic_add(&table->used_elem, nelem, relaxed);

	                                        
	elem->lt_next_idx = LT_IDX_MAX;
	                                                          
	elem = lt_elem_idx(table, free_id.idx);

	  
                                                          
                                                      
                                                        
                                                            
                                                         
                                   
    
	for (struct lt_elem *tmp = elem;;) {
		assert(!lt_bits_valid(tmp->lt_bits) &&
		    (lt_bits_refcnt(tmp->lt_bits) == 0));
		--nalloc;
		tmp->lt_id.generation += 1;
		tmp->lt_bits = 1;
		lt_elem_set_type(tmp, type);
		if (tmp->lt_next_idx == LT_IDX_MAX) {
			break;
		}
		assert(tmp->lt_next_idx != LT_IDX_MAX);
		tmp = lt_elem_idx(table, tmp->lt_next_idx);
	}
	assert(nalloc == 0);

#if CONFIG_LTABLE_STATS
	uint64_t nreservations;
	table->nallocs += nelem;
	if (type == LT_RESERVED) {
		OSIncrementAtomic64(&table->nreservations);
	}
	nreservations = table->nreservations;
	if (table->used_elem > table->max_used) {
		table->max_used = table->used_elem;
	}
	if (nreservations > table->max_reservations) {
		table->max_reservations = nreservations;
	}
	table->avg_used = (table->avg_used + table->used_elem) / 2;
	table->avg_reservations = (table->avg_reservations + nreservations) / 2;
#endif

	return elem;
}


   
                                                                       
  
                                                                            
                                                                            
                                                                            
          
   
void
ltable_realloc_elem(struct link_table *table, struct lt_elem *elem, int type)
{
	(void)table;
	assert(lt_elem_in_range(elem, table) &&
	    !lt_bits_valid(elem->lt_bits));

#if CONFIG_LTABLE_STATS
	table->nreallocs += 1;
	if (lt_bits_type(elem->lt_bits) == LT_RESERVED && type != LT_RESERVED) {
		  
                                                  
                                                         
             
     
		OSDecrementAtomic64(&table->nreservations);
	}
	table->avg_reservations = (table->avg_reservations + table->nreservations) / 2;
#endif

	  
                                                              
                                                                
                                                                
    
	elem->lt_id.generation += 1;
	elem->lt_next_idx = LT_IDX_MAX;
	lt_elem_set_type(elem, type);

	return;
}


   
                                                            
  
                                                                 
  
              
                                                   
                                   
                                        
   
static void
ltable_free_elem(struct link_table *table, struct lt_elem *elem)
{
	assert(lt_elem_in_range(elem, table) &&
	    !lt_bits_valid(elem->lt_bits) &&
	    (lt_bits_refcnt(elem->lt_bits) == 0));

#if CONFIG_LTABLE_STATS
	table->avg_used = (table->avg_used + table->used_elem) / 2;
	if (lt_bits_type(elem->lt_bits) == LT_RESERVED) {
		OSDecrementAtomic64(&table->nreservations);
	}
	table->avg_reservations = (table->avg_reservations + table->nreservations) / 2;
#endif

	elem->lt_bits = 0;

	if (table->poison) {
		(table->poison)(table, elem);
	}

	ltable_enqueue_list(table, elem, elem);
	os_atomic_dec(&table->used_elem, relaxed);
}


   
                                                                         
  
                                                                              
                                                                           
                                                                
  
                                                                      
                                        
   
struct lt_elem *
ltable_get_elem(struct link_table *table, uint64_t id)
{
	struct lt_elem *elem;
	uint32_t idx, bits, new_bits;

	  
                                                                       
                                     
    

	idx = ((struct ltable_id *)&id)->idx;

	if (idx >= table->nelem) {
		panic("id:0x%llx : idx:%d > %d", id, idx, table->nelem);
	}

	elem = lt_elem_idx(table, idx);

	                                                                   
	bits = elem->lt_bits;
	if (!lt_bits_valid(bits)) {
		return NULL;
	}

	  
                                                    
                             
    
	if (elem->lt_id.id != id) {
		return NULL;
	}

	new_bits = bits + 1;

	                        
	assert(lt_bits_refcnt(new_bits) > 0);

	while (OSCompareAndSwap(bits, new_bits, &elem->lt_bits) == FALSE) {
		  
                                       
                                                 
     
		bits = elem->lt_bits;
		if (!lt_bits_valid(bits)) {
			                                   
			return NULL;
		}
		new_bits = bits + 1;
		assert(lt_bits_refcnt(new_bits) > 0);
	}

	                  
	OSMemoryBarrier();

	                                                                
	if (elem->lt_id.id != id) {
		  
                                                    
                                    
                                                
     
		ltable_put_elem(table, elem);
		return NULL;
	}

	                                               
	return elem;
}

   
                                                        
  
                                                                  
                                                                            
                                                                        
           
   
void
ltable_put_elem(struct link_table *table, struct lt_elem *elem)
{
	uint32_t bits, new_bits;

	assert(lt_elem_in_range(elem, table));

	bits = elem->lt_bits;
	new_bits = bits - 1;

	                         
	assert(lt_bits_refcnt(new_bits) < LT_BITS_REFCNT_MASK);

	while (OSCompareAndSwap(bits, new_bits, &elem->lt_bits) == FALSE) {
		bits = elem->lt_bits;
		new_bits = bits - 1;
		                     
		assert(lt_bits_refcnt(new_bits) < LT_BITS_REFCNT_MASK);
	}

	                  
	OSMemoryBarrier();

	  
                                                                 
                                                          
    
	if (!lt_bits_valid(new_bits) && (lt_bits_refcnt(new_bits) == 0)) {
		ltable_free_elem(table, elem);
	}

	return;
}


                                                                                
  
                        
  
                                                                       
                                                                     
                                                                       
  
                                                                                

   
                                                
  
                                                                              
                                                                       
                            
                                                        
   
int
lt_elem_list_link(struct link_table *table, struct lt_elem *parent, struct lt_elem *child)
{
	int nelem = 1;

	assert(lt_elem_in_range(parent, table));

	                                       
	while (parent->lt_next_idx != LT_IDX_MAX) {
		assert(parent->lt_next_idx < table->nelem);
		parent = lt_elem_idx(table, parent->lt_next_idx);
		nelem++;
	}

	if (child) {
		assert(lt_elem_in_range(child, table));
		parent->lt_next_idx = child->lt_id.idx;
	}

	return nelem;
}


   
                                                                       
  
                                                                             
                                                      
  
                                                                           
                        
   
struct lt_elem *
lt_elem_list_first(struct link_table *table, uint64_t id)
{
	uint32_t idx;
	struct lt_elem *elem = NULL;

	if (id == 0) {
		return NULL;
	}

	idx = ((struct ltable_id *)&id)->idx;

	if (idx > table->nelem) {
		panic("Invalid element for id:0x%llx", id);
	}
	elem = lt_elem_idx(table, idx);

	                                                                   
	if (elem->lt_id.id != id) {
		return NULL;
	}

	                                                        
	if (lt_bits_valid(elem->lt_bits) ||
	    lt_bits_type(elem->lt_bits) != LT_RESERVED ||
	    lt_bits_refcnt(elem->lt_bits) != 1) {
		panic("Valid/unreserved element %p (0x%x) in reserved list",
		    elem, elem->lt_bits);
	}

	return elem;
}


   
                                                                    
  
                                                                             
   
struct lt_elem *
lt_elem_list_next(struct link_table *table, struct lt_elem *head)
{
	struct lt_elem *elem;

	if (!head) {
		return NULL;
	}
	if (head->lt_next_idx >= table->nelem) {
		return NULL;
	}

	elem = lt_elem_idx(table, head->lt_next_idx);
	assert(lt_elem_in_range(elem, table));

	return elem;
}


   
                                                        
  
                                                                              
                                                                     
                        
   
struct lt_elem *
lt_elem_list_break(struct link_table *table, struct lt_elem *elem)
{
	struct lt_elem *next;

	if (!elem) {
		return NULL;
	}
	next = lt_elem_list_next(table, elem);
	elem->lt_next_idx = LT_IDX_MAX;

	return next;
}


   
                                                       
  
                                                                              
                                                                             
                                                                        
                                                                              
                         
   
struct lt_elem *
lt_elem_list_pop(struct link_table *table, uint64_t *id, int type)
{
	struct lt_elem *first, *next;

	if (!id || *id == 0) {
		return NULL;
	}

	                                        

	first = lt_elem_list_first(table, *id);
	if (!first) {
		*id = 0;
		return NULL;
	}

	next = lt_elem_list_next(table, first);
	if (next) {
		*id = next->lt_id.id;
	} else {
		*id = 0;
	}

	ltable_realloc_elem(table, first, type);

	return first;
}

   
                                                                 
  
                                                                              
                                                                           
                                                                         
   
int
lt_elem_list_release(struct link_table *table, struct lt_elem *head,
    int __assert_only type)
{
	struct lt_elem *elem;
	int nelem = 0;

	if (!head) {
		return 0;
	}

	for (elem = head;;) {
		assert(lt_elem_in_range(elem, table));
		assert(!lt_bits_valid(elem->lt_bits) && (lt_bits_refcnt(elem->lt_bits) == 1));
		assert(lt_bits_type(elem->lt_bits) == type);

		nelem++;
		elem->lt_bits = 0;
		if (table->poison) {
			(table->poison)(table, elem);
		}

		if (elem->lt_next_idx == LT_IDX_MAX) {
			break;
		}
		assert(elem->lt_next_idx < table->nelem);
		elem = lt_elem_idx(table, elem->lt_next_idx);
	}

	  
                                                                      
                                                                    
                                                                      
         
    

	ltable_enqueue_list(table, head, elem);
	os_atomic_sub(&table->used_elem, nelem, relaxed);
	return nelem;
}
