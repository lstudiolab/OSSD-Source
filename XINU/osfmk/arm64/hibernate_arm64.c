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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
   
                                                                              
                                                                         
   

#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/thread.h>
#include <kern/processor.h>
#include <kern/kalloc.h>
#include <mach/machine.h>
#include <mach/processor_info.h>
#include <mach/mach_types.h>
#include <kern/cpu_data.h>
#include <kern/startup.h>
#include <IOKit/IOPlatformExpert.h>
#include <pexpert/device_tree.h>

#include <IOKit/IOHibernatePrivate.h>
#include <vm/vm_page.h>
#include <san/kasan.h>
#include <arm/cpu_internal.h>
#include <arm/cpu_data_internal.h>
#include <machine/pal_hibernate.h>


extern void
qsort(void *a, size_t n, size_t es, int (*cmp)(const void *, const void *));

void
pal_hib_teardown_pmap_structs(__unused addr64_t *unneeded_start, __unused addr64_t *unneeded_end)
{
}

void
pal_hib_rebuild_pmap_structs(void)
{
}

static void
set_dram_range(hibernate_bitmap_t *range, uint64_t start_addr, uint64_t size)
{
	uint64_t first_page = atop_64(start_addr);
	uint64_t page_count = atop_64(size);
	uint64_t last_page = first_page + page_count - 1;

	range->first_page = (uint32_t)first_page;
	assert(range->first_page == first_page);                                         

	range->last_page = (uint32_t)last_page;
	assert(range->last_page == last_page);                                         
}

                                                         
static int
dram_range_compare(const void *a, const void *b)
{
	return ((const hibernate_bitmap_t *)a)->first_page - ((const hibernate_bitmap_t *)b)->first_page;
}

hibernate_page_list_t *
hibernate_page_list_allocate(boolean_t log)
{
	vm_size_t               size;
	uint32_t                bank;
	uint32_t                pages, page_count;
	hibernate_page_list_t * list;
	hibernate_bitmap_t *    bitmap;

	                                                                   
	hibernate_bitmap_t      dram_ranges[1];
	uint32_t                num_banks = sizeof(dram_ranges) / sizeof(dram_ranges[0]);

	                                                                  
	set_dram_range(&dram_ranges[0], gPhysBase, gPhysSize);

	                                                                               
	                                           
	qsort((void*)dram_ranges, num_banks, sizeof(dram_ranges[0]), dram_range_compare);

	                              

	size = sizeof(hibernate_page_list_t);
	page_count = 0;
	for (bank = 0; bank < num_banks; bank++) {
		pages = dram_ranges[bank].last_page + 1 - dram_ranges[bank].first_page;
		page_count += pages;
		size += sizeof(hibernate_bitmap_t) + ((pages + 31) >> 5) * sizeof(uint32_t);
	}

	list = (hibernate_page_list_t *)kalloc(size);
	if (!list) {
		goto out;
	}

	list->list_size  = (uint32_t)size;
	list->page_count = page_count;
	list->bank_count = num_banks;

	                                 

	bitmap = &list->bank_bitmap[0];
	for (bank = 0; bank < num_banks; bank++) {
		bitmap->first_page = dram_ranges[bank].first_page;
		bitmap->last_page  = dram_ranges[bank].last_page;
		bitmap->bitmapwords = (bitmap->last_page + 1
		    - bitmap->first_page + 31) >> 5;
		if (log) {
			HIBLOG("hib bank[%d]: 0x%llx (%d) end 0x%llx (%d)\n",
			    bank,
			    ptoa_64(bitmap->first_page), bitmap->first_page,
			    ptoa_64(bitmap->last_page), bitmap->last_page);
		}
		bitmap = (hibernate_bitmap_t *) &bitmap->bitmap[bitmap->bitmapwords];
	}

out:

	return list;
}

void
pal_hib_get_stack_pages(vm_offset_t *first_page, vm_offset_t *page_count)
{
	vm_offset_t stack_end = BootCpuData.intstack_top;
	vm_offset_t stack_begin = stack_end - INTSTACK_SIZE;
	*first_page = atop_64(kvtophys(stack_begin));
	*page_count = atop_64(round_page(stack_end) - trunc_page(stack_begin));
}

                                                                             
void
hibernate_page_list_setall_machine(hibernate_page_list_t * page_list,
    hibernate_page_list_t * page_list_wired,
    boolean_t preflight,
    uint32_t * pagesOut)
{
	vm_offset_t stack_first_page, stack_page_count;
	pal_hib_get_stack_pages(&stack_first_page, &stack_page_count);

	extern pmap_paddr_t pmap_stacks_start_pa, pmap_stacks_end_pa;
	vm_offset_t pmap_stack_page_count = atop_64(pmap_stacks_end_pa - pmap_stacks_start_pa);

	if (!preflight) {
		                                                               
		                                                                
		                                      
		hibernate_set_page_state(page_list, page_list_wired,
		    stack_first_page, stack_page_count,
		    kIOHibernatePageStateWiredSave);

		                                                                        
		                                                                    
		                                                                   
		                                                                     
		                                                        
		hibernate_set_page_state(page_list, page_list_wired,
		    atop_64(pmap_stacks_start_pa), pmap_stack_page_count,
		    kIOHibernatePageStateFree);
	}
	*pagesOut += stack_page_count;
	*pagesOut -= pmap_stack_page_count;
}

                                                                      
void
hibernate_page_list_set_volatile(hibernate_page_list_t * page_list,
    hibernate_page_list_t * page_list_wired,
    uint32_t * pagesOut)
{
	vm_offset_t page, count;

	                                                   
	                                           
	pal_hib_get_stack_pages(&page, &count);
	hibernate_set_page_state(page_list, page_list_wired,
	    page, count,
	    kIOHibernatePageStateFree);
	*pagesOut -= count;
}

kern_return_t
hibernate_processor_setup(IOHibernateImageHeader * header)
{
	cpu_datap(master_cpu)->cpu_hibernate = 1;
	header->processorFlags = 0;
	return KERN_SUCCESS;
}

static boolean_t hibernate_vm_locks_safe;

void
hibernate_vm_lock(void)
{
	if (kIOHibernateStateHibernating == gIOHibernateState) {
		hibernate_vm_lock_queues();
		hibernate_vm_locks_safe = TRUE;
	}
}

void
hibernate_vm_unlock(void)
{
	assert(FALSE == ml_get_interrupts_enabled());
	if (kIOHibernateStateHibernating == gIOHibernateState) {
		hibernate_vm_unlock_queues();
	}
	ml_set_is_quiescing(TRUE);
}

                                                                                                                
                                                                                                  
                                                                                               

void
hibernate_vm_lock_end(void)
{
	assert(FALSE == ml_get_interrupts_enabled());
	hibernate_vm_locks_safe = FALSE;
}

boolean_t
hibernate_vm_locks_are_safe(void)
{
	assert(FALSE == ml_get_interrupts_enabled());
	return hibernate_vm_locks_safe;
}

void
pal_hib_init(void)
{
	gHibernateGlobals.kernelSlide = gVirtBase - gPhysBase;
}

void
pal_hib_write_hook(void)
{
}
