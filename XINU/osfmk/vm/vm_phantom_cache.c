/*
 * Copyright (c) 2000-2013 Apple Inc. All rights reserved.
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

#include <vm/vm_page.h>
#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_pageout.h>
#include <vm/vm_phantom_cache.h>
#include <vm/vm_compressor.h>


uint32_t phantom_cache_eval_period_in_msecs = 250;
uint32_t phantom_cache_thrashing_threshold_ssd = 1000;
#if !XNU_TARGET_OS_OSX
uint32_t phantom_cache_thrashing_threshold = 500;
#else                         
uint32_t phantom_cache_thrashing_threshold = 50;
#endif                         

  
                                                          
                                                  
   
#if !XNU_TARGET_OS_OSX
unsigned phantom_cache_contiguous_periods = 4;
#else                         
unsigned phantom_cache_contiguous_periods = 2;
#endif                         

clock_sec_t     pc_start_of_eval_period_sec = 0;
clock_nsec_t    pc_start_of_eval_period_nsec = 0;
boolean_t       pc_need_eval_reset = FALSE;

                                                                 
uint32_t        pc_history = 0;

uint32_t        sample_period_ghost_added_count = 0;
uint32_t        sample_period_ghost_added_count_ssd = 0;
uint32_t        sample_period_ghost_found_count = 0;
uint32_t        sample_period_ghost_found_count_ssd = 0;

uint32_t        vm_phantom_object_id = 1;
#define         VM_PHANTOM_OBJECT_ID_AFTER_WRAP 1000000

vm_ghost_t      vm_phantom_cache;
uint32_t        vm_phantom_cache_nindx = 1;
uint32_t        vm_phantom_cache_num_entries = 0;
uint32_t        vm_phantom_cache_size;

typedef uint32_t        vm_phantom_hash_entry_t;
vm_phantom_hash_entry_t *vm_phantom_cache_hash;
uint32_t        vm_phantom_cache_hash_size;
uint32_t        vm_ghost_hash_mask;                                         
uint32_t        vm_ghost_bucket_hash;                                  


int pg_masks[4] = {
	0x1, 0x2, 0x4, 0x8
};


#define vm_phantom_hash(obj_id, offset) (\
	        ( (natural_t)((uintptr_t)obj_id * vm_ghost_bucket_hash) + (offset ^ vm_ghost_bucket_hash)) & vm_ghost_hash_mask)


struct phantom_cache_stats {
	uint32_t        pcs_wrapped;
	uint32_t        pcs_added_page_to_entry;
	uint32_t        pcs_added_new_entry;
	uint32_t        pcs_replaced_entry;

	uint32_t        pcs_lookup_found_page_in_cache;
	uint32_t        pcs_lookup_entry_not_in_cache;
	uint32_t        pcs_lookup_page_not_in_entry;

	uint32_t        pcs_updated_phantom_state;
} phantom_cache_stats;



void
vm_phantom_cache_init()
{
	unsigned int    num_entries;
	unsigned int    log1;
	unsigned int    size;

	if (!VM_CONFIG_COMPRESSOR_IS_ACTIVE) {
		return;
	}
#if !XNU_TARGET_OS_OSX
	num_entries = (uint32_t)(((max_mem / PAGE_SIZE) / 10) / VM_GHOST_PAGES_PER_ENTRY);
#else                         
	num_entries = (uint32_t)(((max_mem / PAGE_SIZE) / 4) / VM_GHOST_PAGES_PER_ENTRY);
#endif                         
	vm_phantom_cache_num_entries = 1;

	while (vm_phantom_cache_num_entries < num_entries) {
		vm_phantom_cache_num_entries <<= 1;
	}

	  
                                                                                
    
	if (vm_phantom_cache_num_entries > (1 << VM_GHOST_INDEX_BITS)) {
		vm_phantom_cache_num_entries = (1 << VM_GHOST_INDEX_BITS);
	}

	vm_phantom_cache_size = sizeof(struct vm_ghost) * vm_phantom_cache_num_entries;
	vm_phantom_cache_hash_size = sizeof(vm_phantom_hash_entry_t) * vm_phantom_cache_num_entries;

	if (kernel_memory_allocate(kernel_map, (vm_offset_t *)(&vm_phantom_cache), vm_phantom_cache_size, 0, KMA_KOBJECT | KMA_PERMANENT, VM_KERN_MEMORY_PHANTOM_CACHE) != KERN_SUCCESS) {
		panic("vm_phantom_cache_init: kernel_memory_allocate failed\n");
	}
	bzero(vm_phantom_cache, vm_phantom_cache_size);

	if (kernel_memory_allocate(kernel_map, (vm_offset_t *)(&vm_phantom_cache_hash), vm_phantom_cache_hash_size, 0, KMA_KOBJECT | KMA_PERMANENT, VM_KERN_MEMORY_PHANTOM_CACHE) != KERN_SUCCESS) {
		panic("vm_phantom_cache_init: kernel_memory_allocate failed\n");
	}
	bzero(vm_phantom_cache_hash, vm_phantom_cache_hash_size);


	vm_ghost_hash_mask = vm_phantom_cache_num_entries - 1;

	  
                                                          
                                       
                                   
                                             
            
    
	size = vm_phantom_cache_num_entries;
	for (log1 = 0; size > 1; log1++) {
		size /= 2;
	}

	vm_ghost_bucket_hash = 1 << ((log1 + 1) >> 1);                                                   
	vm_ghost_bucket_hash |= 1 << ((log1 + 1) >> 2);                                                      
	vm_ghost_bucket_hash |= 1;                                                                                                

	if (vm_ghost_hash_mask & vm_phantom_cache_num_entries) {
		printf("vm_phantom_cache_init: WARNING -- strange page hash\n");
	}
}


void
vm_phantom_cache_add_ghost(vm_page_t m)
{
	vm_ghost_t      vpce;
	vm_object_t     object;
	int             ghost_index;
	int             pg_mask;
	boolean_t       isSSD = FALSE;
	vm_phantom_hash_entry_t ghost_hash_index;

	object = VM_PAGE_OBJECT(m);

	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);
	vm_object_lock_assert_exclusive(object);

	if (vm_phantom_cache_num_entries == 0) {
		return;
	}

	pg_mask = pg_masks[(m->vmp_offset >> PAGE_SHIFT) & VM_GHOST_PAGE_MASK];

	if (object->phantom_object_id == 0) {
		vnode_pager_get_isSSD(object->pager, &isSSD);

		if (isSSD == TRUE) {
			object->phantom_isssd = TRUE;
		}

		object->phantom_object_id = vm_phantom_object_id++;

		if (vm_phantom_object_id == 0) {
			vm_phantom_object_id = VM_PHANTOM_OBJECT_ID_AFTER_WRAP;
		}
	} else {
		if ((vpce = vm_phantom_cache_lookup_ghost(m, 0))) {
			vpce->g_pages_held |= pg_mask;

			phantom_cache_stats.pcs_added_page_to_entry++;
			goto done;
		}
	}
	  
                                                       
                                                        
                                                          
                                   
    
	ghost_index = vm_phantom_cache_nindx++;

	if (vm_phantom_cache_nindx == vm_phantom_cache_num_entries) {
		vm_phantom_cache_nindx = 1;

		phantom_cache_stats.pcs_wrapped++;
	}
	vpce = &vm_phantom_cache[ghost_index];

	if (vpce->g_obj_id) {
		  
                                             
                                     
     
		vm_ghost_t      nvpce;

		ghost_hash_index = vm_phantom_hash(vpce->g_obj_id, vpce->g_obj_offset);

		nvpce = &vm_phantom_cache[vm_phantom_cache_hash[ghost_hash_index]];

		if (nvpce == vpce) {
			vm_phantom_cache_hash[ghost_hash_index] = vpce->g_next_index;
		} else {
			for (;;) {
				if (nvpce->g_next_index == 0) {
					panic("didn't find ghost in hash\n");
				}

				if (&vm_phantom_cache[nvpce->g_next_index] == vpce) {
					nvpce->g_next_index = vpce->g_next_index;
					break;
				}
				nvpce = &vm_phantom_cache[nvpce->g_next_index];
			}
		}
		phantom_cache_stats.pcs_replaced_entry++;
	} else {
		phantom_cache_stats.pcs_added_new_entry++;
	}

	vpce->g_pages_held = pg_mask;
	vpce->g_obj_offset = (m->vmp_offset >> (PAGE_SHIFT + VM_GHOST_PAGE_SHIFT)) & VM_GHOST_OFFSET_MASK;
	vpce->g_obj_id = object->phantom_object_id;

	ghost_hash_index = vm_phantom_hash(vpce->g_obj_id, vpce->g_obj_offset);
	vpce->g_next_index = vm_phantom_cache_hash[ghost_hash_index];
	vm_phantom_cache_hash[ghost_hash_index] = ghost_index;

done:
	vm_pageout_vminfo.vm_phantom_cache_added_ghost++;

	if (object->phantom_isssd) {
		OSAddAtomic(1, &sample_period_ghost_added_count_ssd);
	} else {
		OSAddAtomic(1, &sample_period_ghost_added_count);
	}
}


vm_ghost_t
vm_phantom_cache_lookup_ghost(vm_page_t m, uint32_t pg_mask)
{
	uint64_t        g_obj_offset;
	uint32_t        g_obj_id;
	uint32_t        ghost_index;
	vm_object_t     object;

	object = VM_PAGE_OBJECT(m);

	if ((g_obj_id = object->phantom_object_id) == 0) {
		  
                                                
     
		return NULL;
	}
	g_obj_offset = (m->vmp_offset >> (PAGE_SHIFT + VM_GHOST_PAGE_SHIFT)) & VM_GHOST_OFFSET_MASK;

	ghost_index = vm_phantom_cache_hash[vm_phantom_hash(g_obj_id, g_obj_offset)];

	while (ghost_index) {
		vm_ghost_t      vpce;

		vpce = &vm_phantom_cache[ghost_index];

		if (vpce->g_obj_id == g_obj_id && vpce->g_obj_offset == g_obj_offset) {
			if (pg_mask == 0 || (vpce->g_pages_held & pg_mask)) {
				phantom_cache_stats.pcs_lookup_found_page_in_cache++;

				return vpce;
			}
			phantom_cache_stats.pcs_lookup_page_not_in_entry++;

			return NULL;
		}
		ghost_index = vpce->g_next_index;
	}
	phantom_cache_stats.pcs_lookup_entry_not_in_cache++;

	return NULL;
}



void
vm_phantom_cache_update(vm_page_t m)
{
	int             pg_mask;
	vm_ghost_t      vpce;
	vm_object_t     object;

	object = VM_PAGE_OBJECT(m);

	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);
	vm_object_lock_assert_exclusive(object);

	if (vm_phantom_cache_num_entries == 0) {
		return;
	}

	pg_mask = pg_masks[(m->vmp_offset >> PAGE_SHIFT) & VM_GHOST_PAGE_MASK];

	if ((vpce = vm_phantom_cache_lookup_ghost(m, pg_mask))) {
		vpce->g_pages_held &= ~pg_mask;

		phantom_cache_stats.pcs_updated_phantom_state++;
		vm_pageout_vminfo.vm_phantom_cache_found_ghost++;

		if (object->phantom_isssd) {
			OSAddAtomic(1, &sample_period_ghost_found_count_ssd);
		} else {
			OSAddAtomic(1, &sample_period_ghost_found_count);
		}
	}
}


#define PHANTOM_CACHE_DEBUG     1

#if     PHANTOM_CACHE_DEBUG

int     sample_period_ghost_counts_indx = 0;

struct {
	uint32_t        added;
	uint32_t        found;
	uint32_t        added_ssd;
	uint32_t        found_ssd;
	uint32_t        elapsed_ms;
	boolean_t       pressure_detected;
} sample_period_ghost_counts[256];

#endif

  
                                                                              
  
                                                                        
                                                                                
                                                                            
   
static boolean_t
is_thrashing(uint32_t added, uint32_t found, uint32_t threshold)
{
	                                                 
	if (added < threshold || found < threshold) {
		return FALSE;
	}

	  
                                                                        
                                                                        
               
   
                                                               
                                                                       
         
   
                                                                  
           
    
	if (found < added / 2) {
		return FALSE;
	}

	return TRUE;
}

  
                                         
                                           
                                            
                                          
                                               
   
boolean_t
vm_phantom_cache_check_pressure()
{
	clock_sec_t     cur_ts_sec;
	clock_nsec_t    cur_ts_nsec;
	uint64_t        elapsed_msecs_in_eval;
	boolean_t       pressure_detected = FALSE;

	clock_get_system_nanotime(&cur_ts_sec, &cur_ts_nsec);

	elapsed_msecs_in_eval = vm_compressor_compute_elapsed_msecs(cur_ts_sec, cur_ts_nsec, pc_start_of_eval_period_sec, pc_start_of_eval_period_nsec);

	  
                                                                       
                                                             
    
	if (elapsed_msecs_in_eval >= phantom_cache_eval_period_in_msecs) {
		pc_need_eval_reset = TRUE;
	}

	if (pc_need_eval_reset == TRUE) {
#if PHANTOM_CACHE_DEBUG
		  
                                                         
     
		sample_period_ghost_counts[sample_period_ghost_counts_indx].added = sample_period_ghost_added_count;
		sample_period_ghost_counts[sample_period_ghost_counts_indx].found = sample_period_ghost_found_count;
		sample_period_ghost_counts[sample_period_ghost_counts_indx].added_ssd = sample_period_ghost_added_count_ssd;
		sample_period_ghost_counts[sample_period_ghost_counts_indx].found_ssd = sample_period_ghost_found_count_ssd;
		sample_period_ghost_counts[sample_period_ghost_counts_indx].elapsed_ms = (uint32_t)elapsed_msecs_in_eval;

		sample_period_ghost_counts_indx++;

		if (sample_period_ghost_counts_indx >= 256) {
			sample_period_ghost_counts_indx = 0;
		}
#endif
		sample_period_ghost_added_count = 0;
		sample_period_ghost_found_count = 0;
		sample_period_ghost_added_count_ssd = 0;
		sample_period_ghost_found_count_ssd = 0;

		pc_start_of_eval_period_sec = cur_ts_sec;
		pc_start_of_eval_period_nsec = cur_ts_nsec;
		pc_history <<= 1;
		pc_need_eval_reset = FALSE;
	} else {
		  
                                                                                 
                                                                                 
                                                                              
                                                                                 
                                                                               
                                                                            
     
		if (is_thrashing(sample_period_ghost_added_count,
		    sample_period_ghost_found_count,
		    phantom_cache_thrashing_threshold) ||
		    is_thrashing(sample_period_ghost_added_count_ssd,
		    sample_period_ghost_found_count_ssd,
		    phantom_cache_thrashing_threshold_ssd)) {
			                                                 
			pc_history |= 1;
		}
	}

	  
                                                                     
   
                                                                        
                                                           
    
	uint32_t bitmask = (1u << phantom_cache_contiguous_periods) - 1;
	if ((pc_history & bitmask) == bitmask) {
		pressure_detected = TRUE;
	}

	if (vm_page_external_count > ((AVAILABLE_MEMORY) * 50) / 100) {
		pressure_detected = FALSE;
	}

#if PHANTOM_CACHE_DEBUG
	sample_period_ghost_counts[sample_period_ghost_counts_indx].pressure_detected = pressure_detected;
#endif
	return pressure_detected;
}

  
                                                                              
                                          
  
                                               
   
void
vm_phantom_cache_restart_sample(void)
{
	pc_need_eval_reset = TRUE;
}
