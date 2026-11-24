/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
   
  
                   
                                                    
  
                              
   

#include <mach_cluster_stats.h>
#include <mach_pagemap.h>
#include <libkern/OSAtomic.h>

#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <mach/message.h>                            
#include <mach/vm_param.h>
#include <mach/vm_behavior.h>
#include <mach/memory_object.h>
                                             
#include <mach/sdt.h>

#include <kern/kern_types.h>
#include <kern/host_statistics.h>
#include <kern/counter.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <kern/host.h>
#include <kern/mach_param.h>
#include <kern/macro_help.h>
#include <kern/zalloc.h>
#include <kern/misc_protos.h>
#include <kern/policy_internal.h>

#include <vm/vm_compressor.h>
#include <vm/vm_compressor_pager.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>
#include <vm/vm_external.h>
#include <vm/memory_object.h>
#include <vm/vm_purgeable_internal.h>                                        
#include <vm/vm_shared_region.h>

#include <sys/codesign.h>
#include <sys/reason.h>
#include <sys/signalvar.h>

#include <san/kasan.h>

#define VM_FAULT_CLASSIFY       0

#define TRACEFAULTPAGE 0                   

int vm_protect_privileged_from_untrusted = 1;

unsigned int    vm_object_pagein_throttle = 16;

  
                                                                                                             
                                                                                                                
                                                                                                       
                                                                                                      
                                                                                                
                    
  
                                                                                                               
                                                                                                               
                                                                                                        
                                                                                              
   

extern void throttle_lowpri_io(int);

extern struct vnode *vnode_pager_lookup_vnode(memory_object_t);

uint64_t vm_hard_throttle_threshold;

#if DEBUG || DEVELOPMENT
static bool vmtc_panic_instead = false;
#endif                           

OS_ALWAYS_INLINE
boolean_t
NEED_TO_HARD_THROTTLE_THIS_TASK(void)
{
	return vm_wants_task_throttled(current_task()) ||
	       ((vm_page_free_count < vm_page_throttle_limit ||
	       HARD_THROTTLE_LIMIT_REACHED()) &&
	       proc_get_effective_thread_policy(current_thread(), TASK_POLICY_IO) >= THROTTLE_LEVEL_THROTTLED);
}

#define HARD_THROTTLE_DELAY     10000                          
#define SOFT_THROTTLE_DELAY     200                          

#define VM_PAGE_CREATION_THROTTLE_PERIOD_SECS   6
#define VM_PAGE_CREATION_THROTTLE_RATE_PER_SEC  20000


#define VM_STAT_DECOMPRESSIONS()        \
MACRO_BEGIN                             \
	counter_inc(&vm_statistics_decompressions); \
	current_thread()->decompressions++; \
MACRO_END

boolean_t current_thread_aborted(void);

                                                
static kern_return_t vm_fault_wire_fast(
	vm_map_t        map,
	vm_map_offset_t va,
	vm_prot_t       prot,
	vm_tag_t        wire_tag,
	vm_map_entry_t  entry,
	pmap_t          pmap,
	vm_map_offset_t pmap_addr,
	ppnum_t         *physpage_p);

static kern_return_t vm_fault_internal(
	vm_map_t        map,
	vm_map_offset_t vaddr,
	vm_prot_t       caller_prot,
	boolean_t       change_wiring,
	vm_tag_t        wire_tag,
	int             interruptible,
	pmap_t          pmap,
	vm_map_offset_t pmap_addr,
	ppnum_t         *physpage_p);

static void vm_fault_copy_cleanup(
	vm_page_t       page,
	vm_page_t       top_page);

static void vm_fault_copy_dst_cleanup(
	vm_page_t       page);

#if     VM_FAULT_CLASSIFY
extern void vm_fault_classify(vm_object_t       object,
    vm_object_offset_t    offset,
    vm_prot_t             fault_type);

extern void vm_fault_classify_init(void);
#endif

unsigned long vm_pmap_enter_blocked = 0;
unsigned long vm_pmap_enter_retried = 0;

unsigned long vm_cs_validates = 0;
unsigned long vm_cs_revalidates = 0;
unsigned long vm_cs_query_modified = 0;
unsigned long vm_cs_validated_dirtied = 0;
unsigned long vm_cs_bitmap_validated = 0;

void vm_pre_fault(vm_map_offset_t, vm_prot_t);

extern char *kdp_compressor_decompressed_page;
extern addr64_t kdp_compressor_decompressed_page_paddr;
extern ppnum_t  kdp_compressor_decompressed_page_ppnum;

struct vmrtfr {
	int vmrtfr_maxi;
	int vmrtfr_curi;
	int64_t vmrtf_total;
	vm_rtfault_record_t *vm_rtf_records;
} vmrtfrs;
#define VMRTF_DEFAULT_BUFSIZE (4096)
#define VMRTF_NUM_RECORDS_DEFAULT (VMRTF_DEFAULT_BUFSIZE / sizeof(vm_rtfault_record_t))
TUNABLE(int, vmrtf_num_records, "vm_rtfault_records", VMRTF_NUM_RECORDS_DEFAULT);

static void vm_rtfrecord_lock(void);
static void vm_rtfrecord_unlock(void);
static void vm_record_rtfault(thread_t, uint64_t, vm_map_offset_t, int);

extern lck_grp_t vm_page_lck_grp_bucket;
extern lck_attr_t vm_page_lck_attr;
LCK_SPIN_DECLARE_ATTR(vm_rtfr_slock, &vm_page_lck_grp_bucket, &vm_page_lck_attr);

  
                         
           
                                           
   
__startup_func
void
vm_fault_init(void)
{
	int i, vm_compressor_temp;
	boolean_t need_default_val = TRUE;
	  
                                                                                                
                                                                                                  
                                                                                                        
                                                                                                
                                                                                                  
                                                                                         
    

	vm_hard_throttle_threshold = sane_size * (35 - MIN((int)(sane_size / (1024 * 1024 * 1024)), 25)) / 100;

	  
                                                                                              
    

	if (PE_parse_boot_argn("vm_compressor", &vm_compressor_temp, sizeof(vm_compressor_temp))) {
		for (i = 0; i < VM_PAGER_MAX_MODES; i++) {
			if (((vm_compressor_temp & (1 << i)) == vm_compressor_temp)) {
				need_default_val = FALSE;
				vm_compressor_mode = vm_compressor_temp;
				break;
			}
		}
		if (need_default_val) {
			printf("Ignoring \"vm_compressor\" boot arg %d\n", vm_compressor_temp);
		}
	}
	if (need_default_val) {
		                                                            
		PE_get_default("kern.vm_compressor", &vm_compressor_mode, sizeof(vm_compressor_mode));
	}
	printf("\"vm_compressor_mode\" is %d\n", vm_compressor_mode);

	PE_parse_boot_argn("vm_protect_privileged_from_untrusted",
	    &vm_protect_privileged_from_untrusted,
	    sizeof(vm_protect_privileged_from_untrusted));

#if DEBUG || DEVELOPMENT
	(void)PE_parse_boot_argn("text_corruption_panic", &vmtc_panic_instead, sizeof(vmtc_panic_instead));
#endif                           
}

__startup_func
static void
vm_rtfault_record_init(void)
{
	size_t size;

	vmrtf_num_records = MAX(vmrtf_num_records, 1);
	size = vmrtf_num_records * sizeof(vm_rtfault_record_t);
	vmrtfrs.vm_rtf_records = zalloc_permanent(size,
	    ZALIGN(vm_rtfault_record_t));
	vmrtfrs.vmrtfr_maxi = vmrtf_num_records - 1;
}
STARTUP(ZALLOC, STARTUP_RANK_MIDDLE, vm_rtfault_record_init);

  
                            
           
                                         
           
                                                  
                         
                                             
                                                 
                              
  
                     
                            
   
void
vm_fault_cleanup(
	vm_object_t     object,
	vm_page_t       top_page)
{
	vm_object_paging_end(object);
	vm_object_unlock(object);

	if (top_page != VM_PAGE_NULL) {
		object = VM_PAGE_OBJECT(top_page);

		vm_object_lock(object);
		VM_PAGE_FREE(top_page);
		vm_object_paging_end(object);
		vm_object_unlock(object);
	}
}

#define ALIGNED(x) (((x) & (PAGE_SIZE_64 - 1)) == 0)


boolean_t       vm_page_deactivate_behind = TRUE;
  
                                                             
   
#define VM_DEFAULT_DEACTIVATE_BEHIND_WINDOW     128
#define VM_DEFAULT_DEACTIVATE_BEHIND_CLUSTER    16                                              
                                                                                                             

int vm_default_behind = VM_DEFAULT_DEACTIVATE_BEHIND_WINDOW;

#define MAX_SEQUENTIAL_RUN      (1024 * 1024 * 1024)

  
                        
  
                                                
                                             
                                                   
  
                                                 
   
static
void
vm_fault_is_sequential(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_behavior_t           behavior)
{
	vm_object_offset_t      last_alloc;
	int                     sequential;
	int                     orig_sequential;

	last_alloc = object->last_alloc;
	sequential = object->sequential;
	orig_sequential = sequential;

	offset = vm_object_trunc_page(offset);
	if (offset == last_alloc && behavior != VM_BEHAVIOR_RANDOM) {
		                                                         
		return;
	}

	switch (behavior) {
	case VM_BEHAVIOR_RANDOM:
		  
                                           
     
		sequential = 0;
		break;

	case VM_BEHAVIOR_SEQUENTIAL:
		if (offset && last_alloc == offset - PAGE_SIZE_64) {
			  
                                              
      
			if (sequential < MAX_SEQUENTIAL_RUN) {
				sequential += PAGE_SIZE;
			}
		} else {
			  
                                            
      
			sequential = 0;
		}
		break;

	case VM_BEHAVIOR_RSEQNTL:
		if (last_alloc && last_alloc == offset + PAGE_SIZE_64) {
			  
                                              
      
			if (sequential > -MAX_SEQUENTIAL_RUN) {
				sequential -= PAGE_SIZE;
			}
		} else {
			  
                                            
      
			sequential = 0;
		}
		break;

	case VM_BEHAVIOR_DEFAULT:
	default:
		if (offset && last_alloc == (offset - PAGE_SIZE_64)) {
			  
                                              
      
			if (sequential < 0) {
				sequential = 0;
			}
			if (sequential < MAX_SEQUENTIAL_RUN) {
				sequential += PAGE_SIZE;
			}
		} else if (last_alloc && last_alloc == (offset + PAGE_SIZE_64)) {
			  
                                              
      
			if (sequential > 0) {
				sequential = 0;
			}
			if (sequential > -MAX_SEQUENTIAL_RUN) {
				sequential -= PAGE_SIZE;
			}
		} else {
			  
                                            
      
			sequential = 0;
		}
		break;
	}
	if (sequential != orig_sequential) {
		if (!OSCompareAndSwap(orig_sequential, sequential, (UInt32 *)&object->sequential)) {
			  
                                                            
                                                            
      
			return;
		}
	}
	  
                                                           
                                                          
                                                                  
                                                             
                                                                         
                                                                       
                                                                       
                                                                    
                               
    
	object->last_alloc = offset;
}


int vm_page_deactivate_behind_count = 0;

  
                            
  
                                                
                                                 
                                                 
                 
  
                         
  
                                               
   
static
boolean_t
vm_fault_deactivate_behind(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_behavior_t           behavior)
{
	int             n;
	int             pages_in_run = 0;
	int             max_pages_in_run = 0;
	int             sequential_run;
	int             sequential_behavior = VM_BEHAVIOR_SEQUENTIAL;
	vm_object_offset_t      run_offset = 0;
	vm_object_offset_t      pg_offset = 0;
	vm_page_t       m;
	vm_page_t       page_run[VM_DEFAULT_DEACTIVATE_BEHIND_CLUSTER];

	pages_in_run = 0;
#if TRACEFAULTPAGE
	dbgTrace(0xBEEF0018, (unsigned int) object, (unsigned int) vm_fault_deactivate_behind);                   
#endif
	if (object == kernel_object || vm_page_deactivate_behind == FALSE || (vm_object_trunc_page(offset) != offset)) {
		  
                                                         
                                         
                                                      
                                                            
                                                        
                                                             
                                                                    
                                      
     
		return FALSE;
	}
	if ((sequential_run = object->sequential)) {
		if (sequential_run < 0) {
			sequential_behavior = VM_BEHAVIOR_RSEQNTL;
			sequential_run = 0 - sequential_run;
		} else {
			sequential_behavior = VM_BEHAVIOR_SEQUENTIAL;
		}
	}
	switch (behavior) {
	case VM_BEHAVIOR_RANDOM:
		break;
	case VM_BEHAVIOR_SEQUENTIAL:
		if (sequential_run >= (int)PAGE_SIZE) {
			run_offset = 0 - PAGE_SIZE_64;
			max_pages_in_run = 1;
		}
		break;
	case VM_BEHAVIOR_RSEQNTL:
		if (sequential_run >= (int)PAGE_SIZE) {
			run_offset = PAGE_SIZE_64;
			max_pages_in_run = 1;
		}
		break;
	case VM_BEHAVIOR_DEFAULT:
	default:
	{       vm_object_offset_t behind = vm_default_behind * PAGE_SIZE_64;

		  
                                                        
                                                          
                                    
     
		if ((uint64_t)sequential_run >= behind && (sequential_run % (VM_DEFAULT_DEACTIVATE_BEHIND_CLUSTER * PAGE_SIZE)) == 0) {
			  
                                                        
                                                                 
                       
      
			if (sequential_behavior == VM_BEHAVIOR_SEQUENTIAL) {
				if (offset >= behind) {
					run_offset = 0 - behind;
					pg_offset = PAGE_SIZE_64;
					max_pages_in_run = VM_DEFAULT_DEACTIVATE_BEHIND_CLUSTER;
				}
			} else {
				if (offset < -behind) {
					run_offset = behind;
					pg_offset = 0 - PAGE_SIZE_64;
					max_pages_in_run = VM_DEFAULT_DEACTIVATE_BEHIND_CLUSTER;
				}
			}
		}
		break;}
	}
	for (n = 0; n < max_pages_in_run; n++) {
		m = vm_page_lookup(object, offset + run_offset + (n * pg_offset));

		if (m && !m->vmp_laundry && !m->vmp_busy && !m->vmp_no_cache && (m->vmp_q_state != VM_PAGE_ON_THROTTLED_Q) && !m->vmp_fictitious && !m->vmp_absent) {
			page_run[pages_in_run++] = m;

			  
                                                                                                  
     
                                                                                            
                                                                                                
                                                                                                     
                                                                                                     
                                                                                                
                                                                                                  
                                                        
      
			pmap_clear_refmod_options(VM_PAGE_GET_PHYS_PAGE(m), VM_MEM_REFERENCED, PMAP_OPTIONS_NOFLUSH, (void *)NULL);
		}
	}
	if (pages_in_run) {
		vm_page_lockspin_queues();

		for (n = 0; n < pages_in_run; n++) {
			m = page_run[n];

			vm_page_deactivate_internal(m, FALSE);

			vm_page_deactivate_behind_count++;
#if TRACEFAULTPAGE
			dbgTrace(0xBEEF0019, (unsigned int) object, (unsigned int) m);                    
#endif
		}
		vm_page_unlock_queues();

		return TRUE;
	}
	return FALSE;
}


#if (DEVELOPMENT || DEBUG)
uint32_t        vm_page_creation_throttled_hard = 0;
uint32_t        vm_page_creation_throttled_soft = 0;
uint64_t        vm_page_creation_throttle_avoided = 0;
#endif                           

static int
vm_page_throttled(boolean_t page_kept)
{
	clock_sec_t     elapsed_sec;
	clock_sec_t     tv_sec;
	clock_usec_t    tv_usec;

	thread_t thread = current_thread();

	if (thread->options & TH_OPT_VMPRIV) {
		return 0;
	}

	if (thread->t_page_creation_throttled) {
		thread->t_page_creation_throttled = 0;

		if (page_kept == FALSE) {
			goto no_throttle;
		}
	}
	if (NEED_TO_HARD_THROTTLE_THIS_TASK()) {
#if (DEVELOPMENT || DEBUG)
		thread->t_page_creation_throttled_hard++;
		OSAddAtomic(1, &vm_page_creation_throttled_hard);
#endif                           
		return HARD_THROTTLE_DELAY;
	}

	if ((vm_page_free_count < vm_page_throttle_limit || (VM_CONFIG_COMPRESSOR_IS_PRESENT && SWAPPER_NEEDS_TO_UNTHROTTLE())) &&
	    thread->t_page_creation_count > (VM_PAGE_CREATION_THROTTLE_PERIOD_SECS * VM_PAGE_CREATION_THROTTLE_RATE_PER_SEC)) {
		if (vm_page_free_wanted == 0 && vm_page_free_wanted_privileged == 0) {
#if (DEVELOPMENT || DEBUG)
			OSAddAtomic64(1, &vm_page_creation_throttle_avoided);
#endif
			goto no_throttle;
		}
		clock_get_system_microtime(&tv_sec, &tv_usec);

		elapsed_sec = tv_sec - thread->t_page_creation_time;

		if (elapsed_sec <= VM_PAGE_CREATION_THROTTLE_PERIOD_SECS ||
		    (thread->t_page_creation_count / elapsed_sec) >= VM_PAGE_CREATION_THROTTLE_RATE_PER_SEC) {
			if (elapsed_sec >= (3 * VM_PAGE_CREATION_THROTTLE_PERIOD_SECS)) {
				  
                                                       
                                                             
                                                        
                                                                
                                                                   
                                                                 
                                         
       
				thread->t_page_creation_time = tv_sec;
				thread->t_page_creation_count = VM_PAGE_CREATION_THROTTLE_RATE_PER_SEC * (VM_PAGE_CREATION_THROTTLE_PERIOD_SECS - 1);
			}
			VM_PAGEOUT_DEBUG(vm_page_throttle_count, 1);

			thread->t_page_creation_throttled = 1;

			if (VM_CONFIG_COMPRESSOR_IS_PRESENT && HARD_THROTTLE_LIMIT_REACHED()) {
#if (DEVELOPMENT || DEBUG)
				thread->t_page_creation_throttled_hard++;
				OSAddAtomic(1, &vm_page_creation_throttled_hard);
#endif                           
				return HARD_THROTTLE_DELAY;
			} else {
#if (DEVELOPMENT || DEBUG)
				thread->t_page_creation_throttled_soft++;
				OSAddAtomic(1, &vm_page_creation_throttled_soft);
#endif                           
				return SOFT_THROTTLE_DELAY;
			}
		}
		thread->t_page_creation_time = tv_sec;
		thread->t_page_creation_count = 0;
	}
no_throttle:
	thread->t_page_creation_count++;

	return 0;
}


  
                                          
                                        
                                                      
  
                        
                          
   
static vm_fault_return_t
vm_fault_check(vm_object_t object, vm_page_t m, vm_page_t first_m, wait_interrupt_t interruptible_state, boolean_t page_throttle)
{
	int throttle_delay;

	if (object->shadow_severed ||
	    VM_OBJECT_PURGEABLE_FAULT_ERROR(object)) {
		  
            
                                     
                                                               
                                          
                                               
     
		if (m != VM_PAGE_NULL) {
			VM_PAGE_FREE(m);
		}
		vm_fault_cleanup(object, first_m);

		thread_interrupt_level(interruptible_state);

		return VM_FAULT_MEMORY_ERROR;
	}
	if (page_throttle == TRUE) {
		if ((throttle_delay = vm_page_throttled(FALSE))) {
			  
                                    
                                              
      
			if (m != VM_PAGE_NULL) {
				VM_PAGE_FREE(m);
			}
			vm_fault_cleanup(object, first_m);

			VM_DEBUG_EVENT(vmf_check_zfdelay, VMF_CHECK_ZFDELAY, DBG_FUNC_NONE, throttle_delay, 0, 0, 0);

			delay(throttle_delay);

			if (current_thread_aborted()) {
				thread_interrupt_level(interruptible_state);
				return VM_FAULT_INTERRUPTED;
			}
			thread_interrupt_level(interruptible_state);

			return VM_FAULT_MEMORY_SHORTAGE;
		}
	}
	return VM_FAULT_SUCCESS;
}

  
                                                  
   
static void
vm_fault_cs_clear(vm_page_t m)
{
	m->vmp_cs_validated = VMP_CS_ALL_FALSE;
	m->vmp_cs_tainted = VMP_CS_ALL_FALSE;
	m->vmp_cs_nx = VMP_CS_ALL_FALSE;
}

  
                                                  
                                                                             
   
static void
vm_fault_enqueue_throttled_locked(vm_page_t m)
{
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);
	assert(!VM_PAGE_WIRED(m));

	  
                                                
                                    
    
	vm_page_queues_remove(m, TRUE);
	vm_page_check_pageable_safe(m);
	vm_page_queue_enter(&vm_page_queue_throttled, m, vmp_pageq);
	m->vmp_q_state = VM_PAGE_ON_THROTTLED_Q;
	vm_page_throttled_count++;
}

  
                                      
                                          
  
                               
                                   
   
static int
vm_fault_zero_page(vm_page_t m, boolean_t no_zero_fill)
{
	int my_fault = DBG_ZERO_FILL_FAULT;
	vm_object_t     object;

	object = VM_PAGE_OBJECT(m);

	  
                                        
   
                                        
                                   
                                       
   
                                 
                               
                                             
                                               
                                               
                                         
                                        
    
	vm_fault_cs_clear(m);
	m->vmp_pmapped = TRUE;

	if (no_zero_fill == TRUE) {
		my_fault = DBG_NZF_PAGE_FAULT;

		if (m->vmp_absent && m->vmp_busy) {
			return my_fault;
		}
	} else {
		vm_page_zero_fill(m);

		counter_inc(&vm_statistics_zero_fill_count);
		DTRACE_VM2(zfod, int, 1, (uint64_t *), NULL);
	}
	assert(!m->vmp_laundry);
	assert(object != kernel_object);
	                                                           
	if (!VM_DYNAMIC_PAGING_ENABLED() &&
	    (object->purgable == VM_PURGABLE_DENY ||
	    object->purgable == VM_PURGABLE_NONVOLATILE ||
	    object->purgable == VM_PURGABLE_VOLATILE)) {
		vm_page_lockspin_queues();
		if (!VM_DYNAMIC_PAGING_ENABLED()) {
			vm_fault_enqueue_throttled_locked(m);
		}
		vm_page_unlock_queues();
	}
	return my_fault;
}


  
                         
           
                                                 
                                                
               
                        
                                                  
                                                      
                    
                                                          
                                                         
                                     
  
                                                    
                                                             
                                                   
  
                                                        
                                                  
                                 
  
           
                                                   
                     
  
                     
                                                    
                                                        
                                                       
             
  
                                                           
                                                          
                                                          
                                                               
                                                             
                                                             
                                                           
                                                           
                            
                
                                                             
                                                              
                                                                     
                                                                 
                                 
   
unsigned int vm_fault_page_blocked_access = 0;
unsigned int vm_fault_page_forced_retry = 0;

vm_fault_return_t
vm_fault_page(
	                
	vm_object_t     first_object,                               
	vm_object_offset_t first_offset,                                
	vm_prot_t       fault_type,                                   
	boolean_t       must_be_resident,                            
	boolean_t       caller_lookup,                             
	                        
	vm_prot_t       *protection,                                
	vm_page_t       *result_page,                                  
	              
	vm_page_t       *top_page,                               
                                                               
	int             *type_of_fault,                                           
                                                                                            
	                     
	kern_return_t   *error_code,                                  
	boolean_t       no_zero_fill,                                     
	boolean_t       data_supply,                              
                                                                         
                                                              
	vm_object_fault_info_t fault_info)
{
	vm_page_t               m;
	vm_object_t             object;
	vm_object_offset_t      offset;
	vm_page_t               first_m;
	vm_object_t             next_object;
	vm_object_t             copy_object;
	boolean_t               look_for_page;
	boolean_t               force_fault_retry = FALSE;
	vm_prot_t               access_required = fault_type;
	vm_prot_t               wants_copy_flag;
	kern_return_t           wait_result;
	wait_interrupt_t        interruptible_state;
	boolean_t               data_already_requested = FALSE;
	vm_behavior_t           orig_behavior;
	vm_size_t               orig_cluster_size;
	vm_fault_return_t       error;
	int                     my_fault;
	uint32_t                try_failed_count;
	int                     interruptible;                                    
	int                     external_state = VM_EXTERNAL_STATE_UNKNOWN;
	memory_object_t         pager;
	vm_fault_return_t       retval;
	int                     grab_options;

  
                                                                               
                                                                          
                                                     
                                                                         
                       
  
                                                                           
                                                  
                                                                     
                                                                       
   
#define MUST_ASK_PAGER(o, f, s)                                 \
	((s = VM_COMPRESSOR_PAGER_STATE_GET((o), (f))) != VM_EXTERNAL_STATE_ABSENT)

#define PAGED_OUT(o, f) \
	(VM_COMPRESSOR_PAGER_STATE_GET((o), (f)) == VM_EXTERNAL_STATE_EXISTS)

  
                   
   
#define RELEASE_PAGE(m)                                 \
	MACRO_BEGIN                                     \
	PAGE_WAKEUP_DONE(m);                            \
	if ( !VM_PAGE_PAGEABLE(m)) {                    \
	        vm_page_lockspin_queues();              \
	        if ( !VM_PAGE_PAGEABLE(m)) {            \
	                if (VM_CONFIG_COMPRESSOR_IS_ACTIVE)     \
	                        vm_page_deactivate(m);          \
	                else                                    \
	                        vm_page_activate(m);            \
	        }                                               \
	        vm_page_unlock_queues();                        \
	}                                                       \
	MACRO_END

#if TRACEFAULTPAGE
	dbgTrace(0xBEEF0002, (unsigned int) first_object, (unsigned int) first_offset);                   
#endif

	interruptible = fault_info->interruptible;
	interruptible_state = thread_interrupt_level(interruptible);

	  
                                        
   
                                                   
                                                  
                                              
                   
   
                                                  
                                                
                             
   
                                                        
                                                    
                                                     
                                                     
   
                                                         
                                                  
                    
   
                                                 
                                                    
                                                     
    

	object = first_object;
	offset = first_offset;
	first_m = VM_PAGE_NULL;
	access_required = fault_type;

	  
                         
    
	my_fault = DBG_CACHE_HIT_FAULT;

	while (TRUE) {
#if TRACEFAULTPAGE
		dbgTrace(0xBEEF0003, (unsigned int) 0, (unsigned int) 0);                         
#endif

		grab_options = 0;
#if CONFIG_SECLUDED_MEMORY
		if (object->can_grab_secluded) {
			grab_options |= VM_PAGE_GRAB_SECLUDED;
		}
#endif                             

		if (!object->alive) {
			  
                               
                               
      
			vm_fault_cleanup(object, first_m);
			thread_interrupt_level(interruptible_state);

			return VM_FAULT_MEMORY_ERROR;
		}

		if (!object->pager_created && object->phys_contiguous) {
			  
                                                     
                                                    
                                    
      
			caller_lookup = FALSE;
			m = VM_PAGE_NULL;
			goto phys_contig_object;
		}

		if (object->blocked_access) {
			  
                                                
                                                     
                                                     
                             
      
			caller_lookup = FALSE;                                  
			vm_object_activity_begin(object);
			vm_object_paging_end(object);
			while (object->blocked_access) {
				vm_object_sleep(object,
				    VM_OBJECT_EVENT_UNBLOCKED,
				    THREAD_UNINT);
			}
			vm_fault_page_blocked_access++;
			vm_object_paging_begin(object);
			vm_object_activity_end(object);
		}

		  
                                                 
     
		if (caller_lookup == TRUE) {
			  
                                               
                                              
                                              
                                                
                 
      
			m = *result_page;
			caller_lookup = FALSE;                                 
		} else {
			m = vm_page_lookup(object, vm_object_trunc_page(offset));
		}
#if TRACEFAULTPAGE
		dbgTrace(0xBEEF0004, (unsigned int) m, (unsigned int) object);                    
#endif
		if (m != VM_PAGE_NULL) {
			if (m->vmp_busy) {
				  
                                    
                                  
       
#if TRACEFAULTPAGE
				dbgTrace(0xBEEF0005, (unsigned int) m, (unsigned int) 0);                         
#endif
				wait_result = PAGE_SLEEP(object, m, interruptible);

				if (wait_result != THREAD_AWAKENED) {
					vm_fault_cleanup(object, first_m);
					thread_interrupt_level(interruptible_state);

					if (wait_result == THREAD_RESTART) {
						return VM_FAULT_RETRY;
					} else {
						return VM_FAULT_INTERRUPTED;
					}
				}
				continue;
			}
			if (m->vmp_laundry) {
				m->vmp_free_when_done = FALSE;

				if (!m->vmp_cleaning) {
					vm_pageout_steal_laundry(m, FALSE);
				}
			}
			if (VM_PAGE_GET_PHYS_PAGE(m) == vm_page_guard_addr) {
				  
                               
       
				if (fault_type == VM_PROT_NONE) {
					  
                                       
                                            
                                     
                                     
        
					m->vmp_busy = TRUE;
					*result_page = m;
					assert(first_m == VM_PAGE_NULL);
					*top_page = first_m;
					if (type_of_fault) {
						*type_of_fault = DBG_GUARD_FAULT;
					}
					thread_interrupt_level(interruptible_state);
					return VM_FAULT_SUCCESS;
				} else {
					  
                                        
                                     
        
					vm_fault_cleanup(object, first_m);
					thread_interrupt_level(interruptible_state);
					return VM_FAULT_MEMORY_ERROR;
				}
			}

			if (m->vmp_error) {
				  
                                         
       
#if TRACEFAULTPAGE
				dbgTrace(0xBEEF0006, (unsigned int) m, (unsigned int) error_code);                        
#endif
				if (error_code) {
					*error_code = KERN_MEMORY_ERROR;
				}
				VM_PAGE_FREE(m);

				vm_fault_cleanup(object, first_m);
				thread_interrupt_level(interruptible_state);

				return VM_FAULT_MEMORY_ERROR;
			}
			if (m->vmp_restart) {
				  
                                    
                               
                                         
                                         
       
#if TRACEFAULTPAGE
				dbgTrace(0xBEEF0007, (unsigned int) m, (unsigned int) 0);                         
#endif
				VM_PAGE_FREE(m);

				vm_fault_cleanup(object, first_m);
				thread_interrupt_level(interruptible_state);

				return VM_FAULT_RETRY;
			}
			if (m->vmp_absent) {
				  
                                          
                                           
      
                                                
                                                 
                                     
       
#if TRACEFAULTPAGE
				dbgTrace(0xBEEF0008, (unsigned int) m, (unsigned int) object->shadow);                    
#endif
				next_object = object->shadow;

				if (next_object == VM_OBJECT_NULL) {
					  
                                       
                                         
                                          
                        
        
					assert(!must_be_resident);

					  
                                             
                                             
                                         
                                                       
                                                      
        
					error = vm_fault_check(object, m, first_m, interruptible_state, (type_of_fault == NULL) ? TRUE : FALSE);

					if (error != VM_FAULT_SUCCESS) {
						return error;
					}

					if (object != first_object) {
						  
                                           
         
						VM_PAGE_FREE(m);

						  
                                                  
         
						vm_object_paging_end(object);
						vm_object_unlock(object);

						  
                                  
                                
                                      
         
						m = first_m;
						first_m = VM_PAGE_NULL;

						object = first_object;
						offset = first_offset;

						vm_object_lock(object);
					} else {
						  
                                                         
                                       
         
						m->vmp_absent = FALSE;
						m->vmp_busy = TRUE;
					}
					if (fault_info->mark_zf_absent && no_zero_fill == TRUE) {
						m->vmp_absent = TRUE;
					}
					  
                                        
                                
        
					my_fault = vm_fault_zero_page(m, no_zero_fill);

					break;
				} else {
					if (must_be_resident) {
						vm_object_paging_end(object);
					} else if (object != first_object) {
						vm_object_paging_end(object);
						VM_PAGE_FREE(m);
					} else {
						first_m = m;
						m->vmp_absent = FALSE;
						m->vmp_busy = TRUE;

						vm_page_lockspin_queues();
						vm_page_queues_remove(m, FALSE);
						vm_page_unlock_queues();
					}

					offset += object->vo_shadow_offset;
					fault_info->lo_offset += object->vo_shadow_offset;
					fault_info->hi_offset += object->vo_shadow_offset;
					access_required = VM_PROT_READ;

					vm_object_lock(next_object);
					vm_object_unlock(object);
					object = next_object;
					vm_object_paging_begin(object);

					  
                                      
        
					my_fault = DBG_CACHE_HIT_FAULT;

					continue;
				}
			}
			if ((m->vmp_cleaning)
			    && ((object != first_object) || (object->copy != VM_OBJECT_NULL))
			    && (fault_type & VM_PROT_WRITE)) {
				  
                                              
                                                  
                                                   
                                                 
                                              
                                            
                                             
                     
       
#if TRACEFAULTPAGE
				dbgTrace(0xBEEF0009, (unsigned int) m, (unsigned int) offset);                    
#endif
				  
                                                 
       
				vm_object_reference_locked(object);

				vm_fault_cleanup(object, first_m);

				vm_object_lock(object);
				assert(object->ref_count > 0);

				m = vm_page_lookup(object, vm_object_trunc_page(offset));

				if (m != VM_PAGE_NULL && m->vmp_cleaning) {
					PAGE_ASSERT_WAIT(m, interruptible);

					vm_object_unlock(object);
					wait_result = thread_block(THREAD_CONTINUE_NULL);
					vm_object_deallocate(object);

					goto backoff;
				} else {
					vm_object_unlock(object);

					vm_object_deallocate(object);
					thread_interrupt_level(interruptible_state);

					return VM_FAULT_RETRY;
				}
			}
			if (type_of_fault == NULL && (m->vmp_q_state == VM_PAGE_ON_SPECULATIVE_Q) &&
			    !(fault_info != NULL && fault_info->stealth)) {
				  
                                               
                                         
                                         
                               
                                                   
                                        
                                               
                                           
                                             
      
                                                 
                                                
                                         
       
				vm_page_lockspin_queues();
				if (m->vmp_q_state == VM_PAGE_ON_SPECULATIVE_Q) {
					vm_page_queues_remove(m, FALSE);
				}
				vm_page_unlock_queues();
			}
			assert(object == VM_PAGE_OBJECT(m));

			if (object->code_signed) {
				  
                    
                                            
                                         
                                             
                                              
                                                
                                                
                          
       
			}

			  
                                           
                                         
                                          
                                                        
      
#if TRACEFAULTPAGE
			dbgTrace(0xBEEF000B, (unsigned int) m, (unsigned int) 0);                         
#endif
			assert(!m->vmp_busy);
			assert(!m->vmp_absent);

			m->vmp_busy = TRUE;
			break;
		}


		  
                                                               
                                                            
                                               
                                                                
                                 
     

		if (must_be_resident) {
			if (fault_type == VM_PROT_NONE &&
			    object == kernel_object) {
				  
                                               
                                                    
                                                  
                                                 
                                   
       
				vm_fault_cleanup(object, first_m);
				thread_interrupt_level(interruptible_state);

				return VM_FAULT_MEMORY_ERROR;
			}

			goto dont_look_for_page;
		}

		                                                         
		assert(object != kernel_object);

		data_supply = FALSE;

		look_for_page = (object->pager_created && (MUST_ASK_PAGER(object, offset, external_state) == TRUE) && !data_supply);

#if TRACEFAULTPAGE
		dbgTrace(0xBEEF000C, (unsigned int) look_for_page, (unsigned int) object);                        
#endif
		if (!look_for_page && object == first_object && !object->phys_contiguous) {
			  
                                                                      
      
			m = vm_page_grab_options(grab_options);
#if TRACEFAULTPAGE
			dbgTrace(0xBEEF000D, (unsigned int) m, (unsigned int) object);                    
#endif
			if (m == VM_PAGE_NULL) {
				vm_fault_cleanup(object, first_m);
				thread_interrupt_level(interruptible_state);

				return VM_FAULT_MEMORY_SHORTAGE;
			}

			if (fault_info && fault_info->batch_pmap_op == TRUE) {
				vm_page_insert_internal(m, object,
				    vm_object_trunc_page(offset),
				    VM_KERN_MEMORY_NONE, FALSE, TRUE, TRUE, FALSE, NULL);
			} else {
				vm_page_insert(m, object, vm_object_trunc_page(offset));
			}
		}
		if (look_for_page) {
			kern_return_t   rc;
			int             my_fault_type;

			  
                                            
                           
      
			if (!object->pager_ready) {
#if TRACEFAULTPAGE
				dbgTrace(0xBEEF000E, (unsigned int) 0, (unsigned int) 0);                         
#endif
				if (m != VM_PAGE_NULL) {
					VM_PAGE_FREE(m);
				}

				  
                                            
       
				vm_object_reference_locked(object);
				vm_fault_cleanup(object, first_m);

				vm_object_lock(object);
				assert(object->ref_count > 0);

				if (!object->pager_ready) {
					wait_result = vm_object_assert_wait(object, VM_OBJECT_EVENT_PAGER_READY, interruptible);

					vm_object_unlock(object);
					if (wait_result == THREAD_WAITING) {
						wait_result = thread_block(THREAD_CONTINUE_NULL);
					}
					vm_object_deallocate(object);

					goto backoff;
				} else {
					vm_object_unlock(object);
					vm_object_deallocate(object);
					thread_interrupt_level(interruptible_state);

					return VM_FAULT_RETRY;
				}
			}
			if (!object->internal && !object->phys_contiguous && object->paging_in_progress > vm_object_pagein_throttle) {
				  
                                             
                                                   
                                        
       
#if TRACEFAULTPAGE
				dbgTrace(0xBEEF0010, (unsigned int) m, (unsigned int) 0);                         
#endif
				if (m != VM_PAGE_NULL) {
					VM_PAGE_FREE(m);
				}
				  
                                            
       
				vm_object_reference_locked(object);

				vm_fault_cleanup(object, first_m);

				vm_object_lock(object);
				assert(object->ref_count > 0);

				if (object->paging_in_progress >= vm_object_pagein_throttle) {
					vm_object_assert_wait(object, VM_OBJECT_EVENT_PAGING_ONLY_IN_PROGRESS, interruptible);

					vm_object_unlock(object);
					wait_result = thread_block(THREAD_CONTINUE_NULL);
					vm_object_deallocate(object);

					goto backoff;
				} else {
					vm_object_unlock(object);
					vm_object_deallocate(object);
					thread_interrupt_level(interruptible_state);

					return VM_FAULT_RETRY;
				}
			}
			if (object->internal) {
				int compressed_count_delta;

				assert(VM_CONFIG_COMPRESSOR_IS_PRESENT);

				if (m == VM_PAGE_NULL) {
					  
                                                                        
        
					m = vm_page_grab_options(grab_options);
#if TRACEFAULTPAGE
					dbgTrace(0xBEEF000D, (unsigned int) m, (unsigned int) object);                    
#endif
					if (m == VM_PAGE_NULL) {
						vm_fault_cleanup(object, first_m);
						thread_interrupt_level(interruptible_state);

						return VM_FAULT_MEMORY_SHORTAGE;
					}

					m->vmp_absent = TRUE;
					if (fault_info && fault_info->batch_pmap_op == TRUE) {
						vm_page_insert_internal(m, object, vm_object_trunc_page(offset), VM_KERN_MEMORY_NONE, FALSE, TRUE, TRUE, FALSE, NULL);
					} else {
						vm_page_insert(m, object, vm_object_trunc_page(offset));
					}
				}
				assert(m->vmp_busy);

				m->vmp_absent = TRUE;
				pager = object->pager;

				assert(object->paging_in_progress > 0);
				vm_object_unlock(object);

				rc = vm_compressor_pager_get(
					pager,
					offset + object->paging_offset,
					VM_PAGE_GET_PHYS_PAGE(m),
					&my_fault_type,
					0,
					&compressed_count_delta);

				if (type_of_fault == NULL) {
					int     throttle_delay;

					  
                                              
                                              
                                            
        
					if (my_fault_type == DBG_COMPRESSOR_FAULT) {
						if ((throttle_delay = vm_page_throttled(TRUE))) {
							VM_DEBUG_EVENT(vmf_compressordelay, VMF_COMPRESSORDELAY, DBG_FUNC_NONE, throttle_delay, 0, 1, 0);
							delay(throttle_delay);
						}
					}
				}
				vm_object_lock(object);
				assert(object->paging_in_progress > 0);

				vm_compressor_pager_count(
					pager,
					compressed_count_delta,
					FALSE,                  
					object);

				switch (rc) {
				case KERN_SUCCESS:
					m->vmp_absent = FALSE;
					m->vmp_dirty = TRUE;
					if ((object->wimg_bits &
					    VM_WIMG_MASK) !=
					    VM_WIMG_USE_DEFAULT) {
						  
                                      
                                  
                                 
                                 
         
						pmap_sync_page_attributes_phys(
							VM_PAGE_GET_PHYS_PAGE(m));
					} else {
						m->vmp_written_by_kernel = TRUE;
					}

					  
                                       
                                           
                                           
                                        
                                         
                        
        
					if (((object->purgable !=
					    VM_PURGABLE_DENY) ||
					    object->vo_ledger_tag) &&
					    (object->vo_owner !=
					    NULL)) {
						  
                            
                               
         
						vm_object_owner_compressed_update(
							object,
							-1);
					}

					break;
				case KERN_MEMORY_FAILURE:
					m->vmp_unusual = TRUE;
					m->vmp_error = TRUE;
					m->vmp_absent = FALSE;
					break;
				case KERN_MEMORY_ERROR:
					assert(m->vmp_absent);
					break;
				default:
					panic("vm_fault_page(): unexpected "
					    "error %d from "
					    "vm_compressor_pager_get()\n",
					    rc);
				}
				PAGE_WAKEUP_DONE(m);

				rc = KERN_SUCCESS;
				goto data_requested;
			}
			my_fault_type = DBG_PAGEIN_FAULT;

			if (m != VM_PAGE_NULL) {
				VM_PAGE_FREE(m);
				m = VM_PAGE_NULL;
			}

#if TRACEFAULTPAGE
			dbgTrace(0xBEEF0012, (unsigned int) object, (unsigned int) 0);                    
#endif

			  
                                                                     
                                                                   
           
      

			pager = object->pager;

			if (pager == MEMORY_OBJECT_NULL) {
				vm_fault_cleanup(object, first_m);
				thread_interrupt_level(interruptible_state);
				return VM_FAULT_MEMORY_ERROR;
			}

			  
                                                              
                                        
      

			if (object->object_is_shared_cache) {
				set_thread_rwlock_boost();
			}

			vm_object_unlock(object);

			  
                                               
                                                    
                                             
                                                 
                                            
                                             
                                               
                                             
                                 
      
			if (object->copy_strategy == MEMORY_OBJECT_COPY_CALL && object != first_object) {
				wants_copy_flag = VM_PROT_WANTS_COPY;
			} else {
				wants_copy_flag = VM_PROT_NONE;
			}

			if (object->copy == first_object) {
				  
                                                    
                                                    
                                                   
                                                   
                                                    
                                                         
                                                       
                                                        
                                                      
                                                                     
                                                             
                                                                 
                                                                   
                                                              
                                                                          
       
				assert(first_m != VM_PAGE_NULL);
				assert(VM_PAGE_OBJECT(first_m) == first_object);

				vm_object_lock(first_object);
				VM_PAGE_FREE(first_m);
				vm_object_paging_end(first_object);
				vm_object_unlock(first_object);

				first_m = VM_PAGE_NULL;
				force_fault_retry = TRUE;

				vm_fault_page_forced_retry++;
			}

			if (data_already_requested == TRUE) {
				orig_behavior = fault_info->behavior;
				orig_cluster_size = fault_info->cluster_size;

				fault_info->behavior = VM_BEHAVIOR_RANDOM;
				fault_info->cluster_size = PAGE_SIZE;
			}
			  
                                                   
      
			rc = memory_object_data_request(
				pager,
				vm_object_trunc_page(offset) + object->paging_offset,
				PAGE_SIZE,
				access_required | wants_copy_flag,
				(memory_object_fault_info_t)fault_info);

			if (data_already_requested == TRUE) {
				fault_info->behavior = orig_behavior;
				fault_info->cluster_size = orig_cluster_size;
			} else {
				data_already_requested = TRUE;
			}

			DTRACE_VM2(maj_fault, int, 1, (uint64_t *), NULL);
#if TRACEFAULTPAGE
			dbgTrace(0xBEEF0013, (unsigned int) object, (unsigned int) rc);                   
#endif
			vm_object_lock(object);

			if (object->object_is_shared_cache) {
				clear_thread_rwlock_boost();
			}

data_requested:
			if (rc != KERN_SUCCESS) {
				vm_fault_cleanup(object, first_m);
				thread_interrupt_level(interruptible_state);

				return (rc == MACH_SEND_INTERRUPTED) ?
				       VM_FAULT_INTERRUPTED :
				       VM_FAULT_MEMORY_ERROR;
			} else {
				clock_sec_t     tv_sec;
				clock_usec_t    tv_usec;

				if (my_fault_type == DBG_PAGEIN_FAULT) {
					clock_get_system_microtime(&tv_sec, &tv_usec);
					current_thread()->t_page_creation_time = tv_sec;
					current_thread()->t_page_creation_count = 0;
				}
			}
			if ((interruptible != THREAD_UNINT) && (current_thread()->sched_flags & TH_SFLAG_ABORT)) {
				vm_fault_cleanup(object, first_m);
				thread_interrupt_level(interruptible_state);

				return VM_FAULT_INTERRUPTED;
			}
			if (force_fault_retry == TRUE) {
				vm_fault_cleanup(object, first_m);
				thread_interrupt_level(interruptible_state);

				return VM_FAULT_RETRY;
			}
			if (m == VM_PAGE_NULL && object->phys_contiguous) {
				  
                                            
                                          
                                                  
                                              
                                               
                                               
                               
                                            
                                              
                                                  
                                       
       
phys_contig_object:
				goto done;
			}
			  
                                
                                            
                                        
      
			my_fault = my_fault_type;

			  
                                                       
                                                       
                  
      
			continue;
		}
dont_look_for_page:
		  
                                                                
                                                             
                                                            
                                                                   
                                                      
     
#if TRACEFAULTPAGE
		dbgTrace(0xBEEF0014, (unsigned int) object, (unsigned int) m);                    
#endif
		if (object == first_object) {
			first_m = m;
		} else {
			assert(m == VM_PAGE_NULL);
		}

		next_object = object->shadow;

		if (next_object == VM_OBJECT_NULL) {
			  
                                                
                                                 
      
			assert(!must_be_resident);

			if (object != first_object) {
				vm_object_paging_end(object);
				vm_object_unlock(object);

				object = first_object;
				offset = first_offset;
				vm_object_lock(object);
			}
			m = first_m;
			assert(VM_PAGE_OBJECT(m) == object);
			first_m = VM_PAGE_NULL;

			  
                                           
                                           
                                       
                                                     
                                                    
      
			error = vm_fault_check(object, m, first_m, interruptible_state, (type_of_fault == NULL) ? TRUE : FALSE);

			if (error != VM_FAULT_SUCCESS) {
				return error;
			}

			if (m == VM_PAGE_NULL) {
				m = vm_page_grab_options(grab_options);

				if (m == VM_PAGE_NULL) {
					vm_fault_cleanup(object, VM_PAGE_NULL);
					thread_interrupt_level(interruptible_state);

					return VM_FAULT_MEMORY_SHORTAGE;
				}
				vm_page_insert(m, object, vm_object_trunc_page(offset));
			}
			if (fault_info->mark_zf_absent && no_zero_fill == TRUE) {
				m->vmp_absent = TRUE;
			}

			my_fault = vm_fault_zero_page(m, no_zero_fill);

			break;
		} else {
			  
                                                
                                              
      
			if ((object != first_object) || must_be_resident) {
				vm_object_paging_end(object);
			}

			offset += object->vo_shadow_offset;
			fault_info->lo_offset += object->vo_shadow_offset;
			fault_info->hi_offset += object->vo_shadow_offset;
			access_required = VM_PROT_READ;

			vm_object_lock(next_object);
			vm_object_unlock(object);

			object = next_object;
			vm_object_paging_begin(object);
		}
	}

	  
                        
   
                     
                                       
                                                  
                                   
   
                                    
                                              
                       
                                                  
   
                                                    
                                                   
            
    

#if TRACEFAULTPAGE
	dbgTrace(0xBEEF0015, (unsigned int) object, (unsigned int) m);                    
#endif
#if     EXTRA_ASSERTIONS
	assert(m->vmp_busy && !m->vmp_absent);
	assert((first_m == VM_PAGE_NULL) ||
	    (first_m->vmp_busy && !first_m->vmp_absent &&
	    !first_m->vmp_active && !first_m->vmp_inactive && !first_m->vmp_secluded));
#endif                        

	  
                                           
                                          
                                            
                            
    
	if (object != first_object) {
#if TRACEFAULTPAGE
		dbgTrace(0xBEEF0016, (unsigned int) object, (unsigned int) fault_type);                   
#endif
		if (fault_type & VM_PROT_WRITE) {
			vm_page_t copy_m;

			  
                                       
                       
      
			assert(!must_be_resident);

			  
                                                
                                               
                                                  
                                               
                                                
                                               
     
                                                  
                                                
                                       
     
                                                  
                                   
      

			  
                                  
      
			copy_m = vm_page_grab_options(grab_options);

			if (copy_m == VM_PAGE_NULL) {
				RELEASE_PAGE(m);

				vm_fault_cleanup(object, first_m);
				thread_interrupt_level(interruptible_state);

				return VM_FAULT_MEMORY_SHORTAGE;
			}

			vm_page_copy(m, copy_m);

			  
                                          
                                        
                                         
                                            
                                        
               
     
                                           
                                        
                                       
      
			if (m->vmp_pmapped) {
				pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m));
			}

			if (m->vmp_clustered) {
				VM_PAGE_COUNT_AS_PAGEIN(m);
				VM_PAGE_CONSUME_CLUSTERED(m);
			}
			assert(!m->vmp_cleaning);

			  
                                               
      
			RELEASE_PAGE(m);

			  
                                                                             
                                                                             
                                                                             
                                                        
      
			if (object->internal == FALSE) {
				vm_fault_is_sequential(object, offset, fault_info->behavior);
			}

			vm_object_paging_end(object);
			vm_object_unlock(object);

			my_fault = DBG_COW_FAULT;
			counter_inc(&vm_statistics_cow_faults);
			DTRACE_VM2(cow_fault, int, 1, (uint64_t *), NULL);
			current_task()->cow_faults++;

			object = first_object;
			offset = first_offset;

			vm_object_lock(object);
			  
                                 
                                      
      
			VM_PAGE_FREE(first_m);
			first_m = VM_PAGE_NULL;

			  
                             
                              
      
			assert(copy_m->vmp_busy);
			vm_page_insert(copy_m, object, vm_object_trunc_page(offset));
			SET_PAGE_DIRTY(copy_m, TRUE);

			m = copy_m;
			  
                                               
                                                
                                         
                                      
      
			vm_object_paging_end(object);
			vm_object_collapse(object, vm_object_trunc_page(offset), TRUE);
			vm_object_paging_begin(object);
		} else {
			*protection &= (~VM_PROT_WRITE);
		}
	}
	  
                                                          
                                                         
                                                               
                                                       
                                                           
    
	try_failed_count = 0;

	while ((copy_object = first_object->copy) != VM_OBJECT_NULL) {
		vm_object_offset_t      copy_offset;
		vm_page_t               copy_m;

#if TRACEFAULTPAGE
		dbgTrace(0xBEEF0017, (unsigned int) copy_object, (unsigned int) fault_type);                      
#endif
		  
                                                  
                                                         
     
		if ((fault_type & VM_PROT_WRITE) == 0) {
			*protection &= ~VM_PROT_WRITE;
			break;
		}

		  
                                               
                                             
     
		if (must_be_resident) {
			break;
		}

		  
                                            
     
		if (!vm_object_lock_try(copy_object)) {
			vm_object_unlock(object);
			try_failed_count++;

			mutex_pause(try_failed_count);                  
			vm_object_lock(object);

			continue;
		}
		try_failed_count = 0;

		  
                                               
                                            
          
     
		vm_object_reference_locked(copy_object);

		  
                                     
     
		copy_offset = first_offset - copy_object->vo_shadow_offset;
		copy_offset = vm_object_trunc_page(copy_offset);

		if (copy_object->vo_size <= copy_offset) {
			  
                                                        
      
			;
		} else if ((copy_m = vm_page_lookup(copy_object, copy_offset)) != VM_PAGE_NULL) {
			  
                                              
      
			if (copy_m->vmp_busy) {
				  
                                   
                                      
       
				RELEASE_PAGE(m);

				  
                                            
       
				vm_object_reference_locked(copy_object);
				vm_object_unlock(copy_object);
				vm_fault_cleanup(object, first_m);

				vm_object_lock(copy_object);
				assert(copy_object->ref_count > 0);
				vm_object_lock_assert_exclusive(copy_object);
				copy_object->ref_count--;
				assert(copy_object->ref_count > 0);
				copy_m = vm_page_lookup(copy_object, copy_offset);

				if (copy_m != VM_PAGE_NULL && copy_m->vmp_busy) {
					PAGE_ASSERT_WAIT(copy_m, interruptible);

					vm_object_unlock(copy_object);
					wait_result = thread_block(THREAD_CONTINUE_NULL);
					vm_object_deallocate(copy_object);

					goto backoff;
				} else {
					vm_object_unlock(copy_object);
					vm_object_deallocate(copy_object);
					thread_interrupt_level(interruptible_state);

					return VM_FAULT_RETRY;
				}
			}
		} else if (!PAGED_OUT(copy_object, copy_offset)) {
			  
                                                       
                                                         
                                                   
                                                       
                                               
                                                           
                                               
     
                                  
      
			copy_m = vm_page_alloc(copy_object, copy_offset);

			if (copy_m == VM_PAGE_NULL) {
				RELEASE_PAGE(m);

				vm_object_lock_assert_exclusive(copy_object);
				copy_object->ref_count--;
				assert(copy_object->ref_count > 0);

				vm_object_unlock(copy_object);
				vm_fault_cleanup(object, first_m);
				thread_interrupt_level(interruptible_state);

				return VM_FAULT_MEMORY_SHORTAGE;
			}
			  
                                      
      
			vm_page_copy(m, copy_m);

			  
                                             
                                            
                                           
                    
      
			if (m->vmp_pmapped) {
				pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m));
			}

			if (m->vmp_clustered) {
				VM_PAGE_COUNT_AS_PAGEIN(m);
				VM_PAGE_CONSUME_CLUSTERED(m);
			}
			  
                                          
                                                
                                     
      
			if ((!copy_object->pager_ready)
			    || VM_COMPRESSOR_PAGER_STATE_GET(copy_object, copy_offset) == VM_EXTERNAL_STATE_ABSENT
			    ) {
				vm_page_lockspin_queues();
				assert(!m->vmp_cleaning);
				vm_page_activate(copy_m);
				vm_page_unlock_queues();

				SET_PAGE_DIRTY(copy_m, TRUE);
				PAGE_WAKEUP_DONE(copy_m);
			} else {
				assert(copy_m->vmp_busy == TRUE);
				assert(!m->vmp_cleaning);

				  
                                            
       
				SET_PAGE_DIRTY(copy_m, TRUE);

				  
                                             
                                      
                                   
                          
       
				vm_object_unlock(object);

				  
                                         
                                   
       
				vm_pageout_initialize_page(copy_m);

				  
                                 
                              
                             
                               
                                  
       
				if ((copy_object->shadow != object) || (copy_object->ref_count == 1)) {
					vm_object_unlock(copy_object);
					vm_object_deallocate(copy_object);
					vm_object_lock(object);

					continue;
				}
				  
                                    
                                   
                                     
                    
       
				vm_object_lock(object);
			}

			  
                                         
                                         
                                       
                                        
                                              
                                              
                                              
      
			if (m->vmp_wanted) {
				m->vmp_wanted = FALSE;
				thread_wakeup_with_result((event_t) m, THREAD_RESTART);
			}
		}
		  
                                               
                                             
                                            
                                         
                  
     
		vm_object_lock_assert_exclusive(copy_object);
		copy_object->ref_count--;
		assert(copy_object->ref_count > 0);

		vm_object_unlock(copy_object);

		break;
	}

done:
	*result_page = m;
	*top_page = first_m;

	if (m != VM_PAGE_NULL) {
		assert(VM_PAGE_OBJECT(m) == object);

		retval = VM_FAULT_SUCCESS;

		if (my_fault == DBG_PAGEIN_FAULT) {
			VM_PAGE_COUNT_AS_PAGEIN(m);

			if (object->internal) {
				my_fault = DBG_PAGEIND_FAULT;
			} else {
				my_fault = DBG_PAGEINV_FAULT;
			}

			  
                                              
                                               
                            
      
			vm_fault_is_sequential(object, offset, fault_info->behavior);
			vm_fault_deactivate_behind(object, offset, fault_info->behavior);
		} else if (type_of_fault == NULL && my_fault == DBG_CACHE_HIT_FAULT) {
			  
                                                    
                                           
      
			if (m->vmp_clustered) {
				VM_PAGE_COUNT_AS_PAGEIN(m);
				VM_PAGE_CONSUME_CLUSTERED(m);
			}
			vm_fault_is_sequential(object, offset, fault_info->behavior);
			vm_fault_deactivate_behind(object, offset, fault_info->behavior);
		} else if (my_fault == DBG_COMPRESSOR_FAULT || my_fault == DBG_COMPRESSOR_SWAPIN_FAULT) {
			VM_STAT_DECOMPRESSIONS();
		}
		if (type_of_fault) {
			*type_of_fault = my_fault;
		}
	} else {
		retval = VM_FAULT_SUCCESS_NO_VM_PAGE;
		assert(first_m == VM_PAGE_NULL);
		assert(object == first_object);
	}

	thread_interrupt_level(interruptible_state);

#if TRACEFAULTPAGE
	dbgTrace(0xBEEF001A, (unsigned int) VM_FAULT_SUCCESS, 0);                         
#endif
	return retval;

backoff:
	thread_interrupt_level(interruptible_state);

	if (wait_result == THREAD_INTERRUPTED) {
		return VM_FAULT_INTERRUPTED;
	}
	return VM_FAULT_RETRY;

#undef  RELEASE_PAGE
}


extern int panic_on_cs_killed;
extern int proc_selfpid(void);
extern char *proc_name_address(void *p);
unsigned long cs_enter_tainted_rejected = 0;
unsigned long cs_enter_tainted_accepted = 0;

  
                
                                                              
                                            
                                                        
                                              
                                                                       
   
static bool
vm_fault_cs_need_validation(
	pmap_t pmap,
	vm_page_t page,
	vm_object_t page_obj,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset)
{
	if (pmap == kernel_pmap) {
		                        
		return false;
	}
	if (!page_obj->code_signed) {
		                                                      
		return false;
	}
	if (fault_page_size == PAGE_SIZE) {
		                               
		assertf(fault_phys_offset == 0,
		    "fault_page_size 0x%llx fault_phys_offset 0x%llx\n",
		    (uint64_t)fault_page_size,
		    (uint64_t)fault_phys_offset);
		if (page->vmp_cs_tainted == VMP_CS_ALL_TRUE) {
			                             
			return false;
		}
		if (page->vmp_cs_validated == VMP_CS_ALL_TRUE &&
		    !page->vmp_wpmapped) {
			                                                           
			return false;
		}
	} else {
		                                    
		if (VMP_CS_TAINTED(page, fault_page_size, fault_phys_offset)) {
			                                                
			return false;
		}
		if (VMP_CS_VALIDATED(page, fault_page_size, fault_phys_offset) &&
		    !page->vmp_wpmapped) {
			                                                     
			return false;
		}
	}
	                                
	return true;
}


static bool
vm_fault_cs_page_immutable(
	vm_page_t m,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	vm_prot_t prot __unused)
{
	if (VMP_CS_VALIDATED(m, fault_page_size, fault_phys_offset)
	                                     ) {
		return true;
	}
	return false;
}

static bool
vm_fault_cs_page_nx(
	vm_page_t m,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset)
{
	return VMP_CS_NX(m, fault_page_size, fault_phys_offset);
}

  
                                                                       
   
static kern_return_t
vm_fault_cs_check_violation(
	bool cs_bypass,
	vm_object_t object,
	vm_page_t m,
	pmap_t pmap,
	vm_prot_t prot,
	vm_prot_t caller_prot,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	vm_object_fault_info_t fault_info,
	bool map_is_switched,
	bool map_is_switch_protected,
	bool *cs_violation)
{
#if !PMAP_CS
#pragma unused(caller_prot)
#pragma unused(fault_info)
#endif               
	int             cs_enforcement_enabled;
	if (!cs_bypass &&
	    vm_fault_cs_need_validation(pmap, m, object,
	    fault_page_size, fault_phys_offset)) {
		vm_object_lock_assert_exclusive(object);

		if (VMP_CS_VALIDATED(m, fault_page_size, fault_phys_offset)) {
			vm_cs_revalidates++;
		}

		                                                        
                                                               

		vm_page_validate_cs(m, fault_page_size, fault_phys_offset);
	}

	                                                                   
                                                                   
                                                                         
                                                
                                                                     
                                                                     
                                                                        
                                                                 
               
    
	if (pmap == kernel_pmap) {
		                                                 
		cs_enforcement_enabled = 0;
	} else {
		cs_enforcement_enabled = pmap_get_vm_map_cs_enforced(pmap);
	}

	if (cs_enforcement_enabled && map_is_switched &&
	    map_is_switch_protected &&
	    vm_fault_cs_page_immutable(m, fault_page_size, fault_phys_offset, prot) &&
	    (prot & VM_PROT_WRITE)) {
		return KERN_CODESIGN_ERROR;
	}

	if (cs_enforcement_enabled &&
	    vm_fault_cs_page_nx(m, fault_page_size, fault_phys_offset) &&
	    (prot & VM_PROT_EXECUTE)) {
		if (cs_debug) {
			printf("page marked to be NX, not letting it be mapped EXEC\n");
		}
		return KERN_CODESIGN_ERROR;
	}

	                                                                  
                                                                  
                                                        
                                                 
                                          
                                                                      
                                                                      
                                                                    
                         
    
	if (cs_bypass) {
		                              
		*cs_violation = FALSE;
	} else if (VMP_CS_TAINTED(m, fault_page_size, fault_phys_offset)) {
		                  
		*cs_violation = TRUE;
	} else if (!cs_enforcement_enabled) {
		                                         
		*cs_violation = FALSE;
	} else if (vm_fault_cs_page_immutable(m, fault_page_size, fault_phys_offset, prot) &&
	    ((prot & VM_PROT_WRITE) ||
	    m->vmp_wpmapped)) {
		  
                                                            
              
                                                        
                                                              
                                                                 
                                                                  
                                
                                                              
                                                               
                                                                 
                                                               
                                                               
                             
     
		*cs_violation = TRUE;
	} else if (!VMP_CS_VALIDATED(m, fault_page_size, fault_phys_offset) &&
	    (prot & VM_PROT_EXECUTE)
	    ) {
		*cs_violation = TRUE;
	} else {
		*cs_violation = FALSE;
	}
	return KERN_SUCCESS;
}

  
                                                                                         
                                                                                      
             
                                                                                               
   
static kern_return_t
vm_fault_cs_handle_violation(
	vm_object_t object,
	vm_page_t m,
	pmap_t pmap,
	vm_prot_t prot,
	vm_map_offset_t vaddr,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	bool map_is_switched,
	bool map_is_switch_protected,
	bool *must_disconnect)
{
#if !MACH_ASSERT
#pragma unused(pmap)
#pragma unused(map_is_switch_protected)
#endif                   
	  
                                                                
                                                               
                                               
                                                              
                                                                 
                                                           
                                   
    
	boolean_t reject_page, cs_killed;
	kern_return_t kr;
	if (map_is_switched) {
		assert(pmap == vm_map_pmap(current_thread()->map));
		assert(!(prot & VM_PROT_WRITE) || (map_is_switch_protected == FALSE));
		reject_page = FALSE;
	} else {
		if (cs_debug > 5) {
			printf("vm_fault: signed: %s validate: %s tainted: %s wpmapped: %s prot: 0x%x\n",
			    object->code_signed ? "yes" : "no",
			    VMP_CS_VALIDATED(m, fault_page_size, fault_phys_offset) ? "yes" : "no",
			    VMP_CS_TAINTED(m, fault_page_size, fault_phys_offset) ? "yes" : "no",
			    m->vmp_wpmapped ? "yes" : "no",
			    (int)prot);
		}
		reject_page = cs_invalid_page((addr64_t) vaddr, &cs_killed);
	}

	if (reject_page) {
		                                                   
		int                     pid;
		const char              *procname;
		task_t                  task;
		vm_object_t             file_object, shadow;
		vm_object_offset_t      file_offset;
		char                    *pathname, *filename;
		vm_size_t               pathname_len, filename_len;
		boolean_t               truncated_path;
#define __PATH_MAX 1024
		struct timespec         mtime, cs_mtime;
		int                     shadow_depth;
		os_reason_t             codesigning_exit_reason = OS_REASON_NULL;

		kr = KERN_CODESIGN_ERROR;
		cs_enter_tainted_rejected++;

		                              
		procname = "?";
		task = current_task();
		pid = proc_selfpid();
		if (task->bsd_info != NULL) {
			procname = proc_name_address(task->bsd_info);
		}

		                          
		file_object = object;
		file_offset = m->vmp_offset;
		for (shadow = file_object->shadow,
		    shadow_depth = 0;
		    shadow != VM_OBJECT_NULL;
		    shadow = file_object->shadow,
		    shadow_depth++) {
			vm_object_lock_shared(shadow);
			if (file_object != object) {
				vm_object_unlock(file_object);
			}
			file_offset += file_object->vo_shadow_offset;
			file_object = shadow;
		}

		mtime.tv_sec = 0;
		mtime.tv_nsec = 0;
		cs_mtime.tv_sec = 0;
		cs_mtime.tv_nsec = 0;

		                                         
		pathname = NULL;
		filename = NULL;
		pathname_len = 0;
		filename_len = 0;
		truncated_path = FALSE;
		                                                                  
		if (file_object->pager != NULL) {
			pathname = kheap_alloc(KHEAP_TEMP, __PATH_MAX * 2, Z_WAITOK);
			if (pathname) {
				pathname[0] = '\0';
				pathname_len = __PATH_MAX;
				filename = pathname + pathname_len;
				filename_len = __PATH_MAX;

				if (vnode_pager_get_object_name(file_object->pager,
				    pathname,
				    pathname_len,
				    filename,
				    filename_len,
				    &truncated_path) == KERN_SUCCESS) {
					                     
					pathname[__PATH_MAX - 1] = '\0';
					filename[__PATH_MAX - 1] = '\0';

					vnode_pager_get_object_mtime(file_object->pager,
					    &mtime,
					    &cs_mtime);
				} else {
					kheap_free(KHEAP_TEMP, pathname, __PATH_MAX * 2);
					pathname = NULL;
					filename = NULL;
					pathname_len = 0;
					filename_len = 0;
					truncated_path = FALSE;
				}
			}
		}
		printf("CODE SIGNING: process %d[%s]: "
		    "rejecting invalid page at address 0x%llx "
		    "from offset 0x%llx in file \"%s%s%s\" "
		    "(cs_mtime:%lu.%ld %s mtime:%lu.%ld) "
		    "(signed:%d validated:%d tainted:%d nx:%d "
		    "wpmapped:%d dirty:%d depth:%d)\n",
		    pid, procname, (addr64_t) vaddr,
		    file_offset,
		    (pathname ? pathname : "<nil>"),
		    (truncated_path ? "/.../" : ""),
		    (truncated_path ? filename : ""),
		    cs_mtime.tv_sec, cs_mtime.tv_nsec,
		    ((cs_mtime.tv_sec == mtime.tv_sec &&
		    cs_mtime.tv_nsec == mtime.tv_nsec)
		    ? "=="
		    : "!="),
		    mtime.tv_sec, mtime.tv_nsec,
		    object->code_signed,
		    VMP_CS_VALIDATED(m, fault_page_size, fault_phys_offset),
		    VMP_CS_TAINTED(m, fault_page_size, fault_phys_offset),
		    VMP_CS_NX(m, fault_page_size, fault_phys_offset),
		    m->vmp_wpmapped,
		    m->vmp_dirty,
		    shadow_depth);

		  
                                                                                                               
                                                                                                                   
                                                                                                               
                                           
     
		if (cs_killed) {
			KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
			    pid, OS_REASON_CODESIGNING, CODESIGNING_EXIT_REASON_INVALID_PAGE, 0, 0);

			codesigning_exit_reason = os_reason_create(OS_REASON_CODESIGNING, CODESIGNING_EXIT_REASON_INVALID_PAGE);
			if (codesigning_exit_reason == NULL) {
				printf("vm_fault_enter: failed to allocate codesigning exit reason\n");
			} else {
				mach_vm_address_t data_addr = 0;
				struct codesigning_exit_reason_info *ceri = NULL;
				uint32_t reason_buffer_size_estimate = kcdata_estimate_required_buffer_size(1, sizeof(*ceri));

				if (os_reason_alloc_buffer_noblock(codesigning_exit_reason, reason_buffer_size_estimate)) {
					printf("vm_fault_enter: failed to allocate buffer for codesigning exit reason\n");
				} else {
					if (KERN_SUCCESS == kcdata_get_memory_addr(&codesigning_exit_reason->osr_kcd_descriptor,
					    EXIT_REASON_CODESIGNING_INFO, sizeof(*ceri), &data_addr)) {
						ceri = (struct codesigning_exit_reason_info *)data_addr;
						static_assert(__PATH_MAX == sizeof(ceri->ceri_pathname));

						ceri->ceri_virt_addr = vaddr;
						ceri->ceri_file_offset = file_offset;
						if (pathname) {
							strncpy((char *)&ceri->ceri_pathname, pathname, sizeof(ceri->ceri_pathname));
						} else {
							ceri->ceri_pathname[0] = '\0';
						}
						if (filename) {
							strncpy((char *)&ceri->ceri_filename, filename, sizeof(ceri->ceri_filename));
						} else {
							ceri->ceri_filename[0] = '\0';
						}
						ceri->ceri_path_truncated = (truncated_path ? 1 : 0);
						ceri->ceri_codesig_modtime_secs = cs_mtime.tv_sec;
						ceri->ceri_codesig_modtime_nsecs = cs_mtime.tv_nsec;
						ceri->ceri_page_modtime_secs = mtime.tv_sec;
						ceri->ceri_page_modtime_nsecs = mtime.tv_nsec;
						ceri->ceri_object_codesigned = (object->code_signed);
						ceri->ceri_page_codesig_validated = VMP_CS_VALIDATED(m, fault_page_size, fault_phys_offset);
						ceri->ceri_page_codesig_tainted = VMP_CS_TAINTED(m, fault_page_size, fault_phys_offset);
						ceri->ceri_page_codesig_nx = VMP_CS_NX(m, fault_page_size, fault_phys_offset);
						ceri->ceri_page_wpmapped = (m->vmp_wpmapped);
						ceri->ceri_page_slid = 0;
						ceri->ceri_page_dirty = (m->vmp_dirty);
						ceri->ceri_page_shadow_depth = shadow_depth;
					} else {
#if DEBUG || DEVELOPMENT
						panic("vm_fault_enter: failed to allocate kcdata for codesigning exit reason");
#else
						printf("vm_fault_enter: failed to allocate kcdata for codesigning exit reason\n");
#endif                           
						                     
						os_reason_alloc_buffer_noblock(codesigning_exit_reason, 0);
					}
				}
			}

			set_thread_exit_reason(current_thread(), codesigning_exit_reason, FALSE);
		}
		if (panic_on_cs_killed &&
		    object->object_is_shared_cache) {
			char *tainted_contents;
			vm_map_offset_t src_vaddr;
			src_vaddr = (vm_map_offset_t) phystokv((pmap_paddr_t)VM_PAGE_GET_PHYS_PAGE(m) << PAGE_SHIFT);
			tainted_contents = kalloc(PAGE_SIZE);
			bcopy((const char *)src_vaddr, tainted_contents, PAGE_SIZE);
			printf("CODE SIGNING: tainted page %p phys 0x%x phystokv 0x%llx copied to %p\n", m, VM_PAGE_GET_PHYS_PAGE(m), (uint64_t)src_vaddr, tainted_contents);
			panic("CODE SIGNING: process %d[%s]: "
			    "rejecting invalid page (phys#0x%x) at address 0x%llx "
			    "from offset 0x%llx in file \"%s%s%s\" "
			    "(cs_mtime:%lu.%ld %s mtime:%lu.%ld) "
			    "(signed:%d validated:%d tainted:%d nx:%d"
			    "wpmapped:%d dirty:%d depth:%d)\n",
			    pid, procname,
			    VM_PAGE_GET_PHYS_PAGE(m),
			    (addr64_t) vaddr,
			    file_offset,
			    (pathname ? pathname : "<nil>"),
			    (truncated_path ? "/.../" : ""),
			    (truncated_path ? filename : ""),
			    cs_mtime.tv_sec, cs_mtime.tv_nsec,
			    ((cs_mtime.tv_sec == mtime.tv_sec &&
			    cs_mtime.tv_nsec == mtime.tv_nsec)
			    ? "=="
			    : "!="),
			    mtime.tv_sec, mtime.tv_nsec,
			    object->code_signed,
			    VMP_CS_VALIDATED(m, fault_page_size, fault_phys_offset),
			    VMP_CS_TAINTED(m, fault_page_size, fault_phys_offset),
			    VMP_CS_NX(m, fault_page_size, fault_phys_offset),
			    m->vmp_wpmapped,
			    m->vmp_dirty,
			    shadow_depth);
		}

		if (file_object != object) {
			vm_object_unlock(file_object);
		}
		if (pathname_len != 0) {
			kheap_free(KHEAP_TEMP, pathname, __PATH_MAX * 2);
			pathname = NULL;
			filename = NULL;
		}
	} else {
		                                   
		kr = KERN_SUCCESS;
		if (!VMP_CS_VALIDATED(m, fault_page_size, fault_phys_offset) &&
		    !object->code_signed) {
			  
                                                  
                                             
                                               
                 
                                                
                                               
                                              
                       
                                         
                                            
                                               
                                               
                                                  
                                                 
                                                 
                        
      
		} else {
			  
                                                 
                                                
                                             
                                             
                                                 
                                   
      
			if (!VMP_CS_TAINTED(m, fault_page_size, fault_phys_offset)) {
				*must_disconnect = TRUE;
				VMP_CS_SET_TAINTED(m, fault_page_size, fault_phys_offset, TRUE);
			}
		}
		cs_enter_tainted_accepted++;
	}
	if (kr != KERN_SUCCESS) {
		if (cs_debug) {
			printf("CODESIGNING: vm_fault_enter(0x%llx): "
			    "*** INVALID PAGE ***\n",
			    (long long)vaddr);
		}
#if !SECURE_KERNEL
		if (cs_enforcement_panic) {
			panic("CODESIGNING: panicking on invalid page\n");
		}
#endif
	}
	return kr;
}

  
                                                                                
            
  
                                                                                      
             
                                                                                               
   
static kern_return_t
vm_fault_validate_cs(
	bool cs_bypass,
	vm_object_t object,
	vm_page_t m,
	pmap_t pmap,
	vm_map_offset_t vaddr,
	vm_prot_t prot,
	vm_prot_t caller_prot,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	vm_object_fault_info_t fault_info,
	bool *must_disconnect)
{
	bool map_is_switched, map_is_switch_protected, cs_violation;
	kern_return_t kr;
	                                           
	map_is_switched = ((pmap != vm_map_pmap(current_task()->map)) &&
	    (pmap == vm_map_pmap(current_thread()->map)));
	map_is_switch_protected = current_thread()->map->switch_protect;
	kr = vm_fault_cs_check_violation(cs_bypass, object, m, pmap,
	    prot, caller_prot, fault_page_size, fault_phys_offset, fault_info,
	    map_is_switched, map_is_switch_protected, &cs_violation);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	if (cs_violation) {
		kr = vm_fault_cs_handle_violation(object, m, pmap, prot, vaddr,
		    fault_page_size, fault_phys_offset,
		    map_is_switched, map_is_switch_protected, must_disconnect);
	}
	return kr;
}

  
                                                    
   
static void
vm_fault_enqueue_page(
	vm_object_t object,
	vm_page_t m,
	bool wired,
	bool change_wiring,
	vm_tag_t wire_tag,
	bool no_cache,
	int *type_of_fault,
	kern_return_t kr)
{
	assert((m->vmp_q_state == VM_PAGE_USED_BY_COMPRESSOR) || object != compressor_object);
	boolean_t       page_queues_locked = FALSE;
	boolean_t       previously_pmapped = m->vmp_pmapped;
#define __VM_PAGE_LOCKSPIN_QUEUES_IF_NEEDED()   \
MACRO_BEGIN                                     \
	if (! page_queues_locked) {             \
	        page_queues_locked = TRUE;      \
	        vm_page_lockspin_queues();      \
	}                                       \
MACRO_END
#define __VM_PAGE_UNLOCK_QUEUES_IF_NEEDED()     \
MACRO_BEGIN                                     \
	if (page_queues_locked) {               \
	        page_queues_locked = FALSE;     \
	        vm_page_unlock_queues();        \
	}                                       \
MACRO_END

#if CONFIG_BACKGROUND_QUEUE
	vm_page_update_background_state(m);
#endif
	if (m->vmp_q_state == VM_PAGE_USED_BY_COMPRESSOR) {
		  
                                       
                                          
     
		assert(object == compressor_object);
	} else if (change_wiring) {
		__VM_PAGE_LOCKSPIN_QUEUES_IF_NEEDED();

		if (wired) {
			if (kr == KERN_SUCCESS) {
				vm_page_wire(m, wire_tag, TRUE);
			}
		} else {
			vm_page_unwire(m, TRUE);
		}
		                                                       
	} else {
		if (object->internal == TRUE) {
			  
                                    
                            
      
			no_cache = FALSE;
		}
		if (kr != KERN_SUCCESS) {
			__VM_PAGE_LOCKSPIN_QUEUES_IF_NEEDED();
			vm_page_deactivate(m);
			                                                       
		} else if (((m->vmp_q_state == VM_PAGE_NOT_ON_Q) ||
		    (m->vmp_q_state == VM_PAGE_ON_SPECULATIVE_Q) ||
		    (m->vmp_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q) ||
		    ((m->vmp_q_state != VM_PAGE_ON_THROTTLED_Q) && no_cache)) &&
		    !VM_PAGE_WIRED(m)) {
			if (vm_page_local_q &&
			    (*type_of_fault == DBG_COW_FAULT ||
			    *type_of_fault == DBG_ZERO_FILL_FAULT)) {
				struct vpl      *lq;
				uint32_t        lid;

				assert(m->vmp_q_state == VM_PAGE_NOT_ON_Q);

				__VM_PAGE_UNLOCK_QUEUES_IF_NEEDED();
				vm_object_lock_assert_exclusive(object);

				  
                                         
                     
                                       
                                         
                                          
                                           
      
                                          
                                          
                                          
                                           
                               
       
				lid = cpu_number();

				lq = zpercpu_get_cpu(vm_page_local_q, lid);

				VPL_LOCK(&lq->vpl_lock);

				vm_page_check_pageable_safe(m);
				vm_page_queue_enter(&lq->vpl_queue, m, vmp_pageq);
				m->vmp_q_state = VM_PAGE_ON_ACTIVE_LOCAL_Q;
				m->vmp_local_id = lid;
				lq->vpl_count++;

				if (object->internal) {
					lq->vpl_internal_count++;
				} else {
					lq->vpl_external_count++;
				}

				VPL_UNLOCK(&lq->vpl_lock);

				if (lq->vpl_count > vm_page_local_q_soft_limit) {
					  
                                   
                           
                                     
                                     
                                 
                                  
                                 
                                    
                                
                                  
                                   
                                
                                  
             
        
					vm_page_reactivate_local(lid, FALSE, FALSE);
				}
			} else {
				__VM_PAGE_LOCKSPIN_QUEUES_IF_NEEDED();

				  
                                      
                      
       
				if (!VM_PAGE_WIRED(m)) {
					if (m->vmp_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q) {
						vm_page_queues_remove(m, FALSE);

						VM_PAGEOUT_DEBUG(vm_pageout_cleaned_reactivated, 1);
						VM_PAGEOUT_DEBUG(vm_pageout_cleaned_fault_reactivated, 1);
					}

					if (!VM_PAGE_ACTIVE_OR_INACTIVE(m) ||
					    no_cache) {
						  
                                      
                                    
                             
                                    
                                    
                                    
                                 
                                     
                                    
                             
         

						if (no_cache &&
						    (!previously_pmapped ||
						    m->vmp_no_cache)) {
							m->vmp_no_cache = TRUE;

							if (m->vmp_q_state != VM_PAGE_ON_SPECULATIVE_Q) {
								vm_page_speculate(m, FALSE);
							}
						} else if (!VM_PAGE_ACTIVE_OR_INACTIVE(m)) {
							vm_page_activate(m);
						}
					}
				}
				                                                       
			}
		}
	}
	                                                              
	__VM_PAGE_UNLOCK_QUEUES_IF_NEEDED();
}

  
                                                                                         
                                                                             
                                       
   
static bool
vm_fault_enter_set_mapped(
	vm_object_t object,
	vm_page_t m,
	vm_prot_t prot,
	vm_prot_t fault_type)
{
	bool page_needs_sync = false;
	  
                                                    
                                                   
                                               
                 
    
	if ((prot & VM_PROT_EXECUTE) && !m->vmp_xpmapped) {
		ppnum_t phys_page = VM_PAGE_GET_PHYS_PAGE(m);

		pmap_lock_phys_page(phys_page);
		m->vmp_pmapped = TRUE;

		if (!m->vmp_xpmapped) {
			m->vmp_xpmapped = TRUE;

			pmap_unlock_phys_page(phys_page);

			if (!object->internal) {
				OSAddAtomic(1, &vm_page_xpmapped_external_count);
			}

#if defined(__arm__) || defined(__arm64__)
			page_needs_sync = true;
#else
			if (object->internal &&
			    object->pager != NULL) {
				  
                                
                          
                               
                                
                      
                                  
                                   
                                 
            
       
				assert(VM_CONFIG_COMPRESSOR_IS_PRESENT);
				page_needs_sync = true;
			}
#endif
		} else {
			pmap_unlock_phys_page(phys_page);
		}
	} else {
		if (m->vmp_pmapped == FALSE) {
			ppnum_t phys_page = VM_PAGE_GET_PHYS_PAGE(m);

			pmap_lock_phys_page(phys_page);
			m->vmp_pmapped = TRUE;
			pmap_unlock_phys_page(phys_page);
		}
	}

	if (fault_type & VM_PROT_WRITE) {
		if (m->vmp_wpmapped == FALSE) {
			vm_object_lock_assert_exclusive(object);
			if (!object->internal && object->pager) {
				task_update_logical_writes(current_task(), PAGE_SIZE, TASK_WRITE_DEFERRED, vnode_pager_lookup_vnode(object->pager));
			}
			m->vmp_wpmapped = TRUE;
		}
	}
	return page_needs_sync;
}

  
                                             
                                                                                
                                                
   
static kern_return_t
vm_fault_attempt_pmap_enter(
	pmap_t pmap,
	vm_map_offset_t vaddr,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	vm_page_t m,
	vm_prot_t *prot,
	vm_prot_t caller_prot,
	vm_prot_t fault_type,
	bool wired,
	int pmap_options)
{
#if !PMAP_CS
#pragma unused(caller_prot)
#endif               
	kern_return_t kr;
	if (fault_page_size != PAGE_SIZE) {
		DEBUG4K_FAULT("pmap %p va 0x%llx pa 0x%llx (0x%llx+0x%llx) prot 0x%x fault_type 0x%x\n", pmap, (uint64_t)vaddr, (uint64_t)((((pmap_paddr_t)VM_PAGE_GET_PHYS_PAGE(m)) << PAGE_SHIFT) + fault_phys_offset), (uint64_t)(((pmap_paddr_t)VM_PAGE_GET_PHYS_PAGE(m)) << PAGE_SHIFT), (uint64_t)fault_phys_offset, *prot, fault_type);
		assertf((!(fault_phys_offset & FOURK_PAGE_MASK) &&
		    fault_phys_offset < PAGE_SIZE),
		    "0x%llx\n", (uint64_t)fault_phys_offset);
	} else {
		assertf(fault_phys_offset == 0,
		    "0x%llx\n", (uint64_t)fault_phys_offset);
	}

	PMAP_ENTER_OPTIONS(pmap, vaddr,
	    fault_phys_offset,
	    m, *prot, fault_type, 0,
	    wired,
	    pmap_options,
	    kr);
	return kr;
}

  
                                      
                                 
                                    
  
                                                                                 
                                                                                            
   
static kern_return_t
vm_fault_pmap_enter(
	pmap_t pmap,
	vm_map_offset_t vaddr,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	vm_page_t m,
	vm_prot_t *prot,
	vm_prot_t caller_prot,
	vm_prot_t fault_type,
	bool wired,
	int pmap_options,
	boolean_t *need_retry)
{
	kern_return_t kr;
	if (need_retry != NULL) {
		  
                                                                 
                                                              
                                         
     
		pmap_options |= PMAP_OPTIONS_NOWAIT;
	}
	kr = vm_fault_attempt_pmap_enter(pmap, vaddr,
	    fault_page_size, fault_phys_offset,
	    m, prot, caller_prot, fault_type, wired, pmap_options);
	if (kr == KERN_RESOURCE_SHORTAGE) {
		if (need_retry) {
			  
                                                      
                                                     
                                                                     
      
			*need_retry = TRUE;
			vm_pmap_enter_retried++;
		}
	}
	return kr;
}

  
                                      
                                    
                                                                       
                                                                   
  
                                                                                 
                                                                                            
   
static kern_return_t
vm_fault_pmap_enter_with_object_lock(
	vm_object_t object,
	pmap_t pmap,
	vm_map_offset_t vaddr,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	vm_page_t m,
	vm_prot_t *prot,
	vm_prot_t caller_prot,
	vm_prot_t fault_type,
	bool wired,
	int pmap_options,
	boolean_t *need_retry)
{
	kern_return_t kr;
	  
                             
                                                            
                                           
    
	kr = vm_fault_attempt_pmap_enter(pmap, vaddr,
	    fault_page_size, fault_phys_offset,
	    m, prot, caller_prot, fault_type, wired, pmap_options | PMAP_OPTIONS_NOWAIT);
#if __x86_64__
	if (kr == KERN_INVALID_ARGUMENT &&
	    pmap == PMAP_NULL &&
	    wired) {
		  
                                         
                                                
                   
                                                        
     
		kr = KERN_SUCCESS;
	}
#endif                 

	if (kr == KERN_RESOURCE_SHORTAGE) {
		if (need_retry) {
			  
                                                              
                                                           
                                                          
                                                        
                                                      
                                                          
                                                           
                                                       
                      
      
			*need_retry = TRUE;
			vm_pmap_enter_retried++;
			goto done;
		}
		  
                                                           
                                                    
                              
                                                       
                                           
     
		boolean_t was_busy = m->vmp_busy;

		vm_object_lock_assert_exclusive(object);

		m->vmp_busy = TRUE;
		vm_object_unlock(object);

		PMAP_ENTER_OPTIONS(pmap, vaddr,
		    fault_phys_offset,
		    m, *prot, fault_type,
		    0, wired,
		    pmap_options, kr);

		assert(VM_PAGE_OBJECT(m) == object);

		                                 
		vm_object_lock(object);

		                                                      
                                        
		assert(m->vmp_busy);
		if (!was_busy) {
			PAGE_WAKEUP_DONE(m);
		}
		vm_pmap_enter_blocked++;
	}

done:
	return kr;
}

  
                                                                         
                                         
                                           
  
                                   
                               
  
                                                                     
                                           
   
static kern_return_t
vm_fault_enter_prepare(
	vm_page_t m,
	pmap_t pmap,
	vm_map_offset_t vaddr,
	vm_prot_t *prot,
	vm_prot_t caller_prot,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	boolean_t change_wiring,
	vm_prot_t fault_type,
	vm_object_fault_info_t fault_info,
	int *type_of_fault,
	bool *page_needs_data_sync)
{
	kern_return_t   kr;
	bool            is_tainted = false;
	vm_object_t     object;
	boolean_t       cs_bypass = fault_info->cs_bypass;

	object = VM_PAGE_OBJECT(m);

	vm_object_lock_assert_held(object);

#if KASAN
	if (pmap == kernel_pmap) {
		kasan_notify_address(vaddr, PAGE_SIZE);
	}
#endif

	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_NOTOWNED);

	if (*type_of_fault == DBG_ZERO_FILL_FAULT) {
		vm_object_lock_assert_exclusive(object);
	} else if ((fault_type & VM_PROT_WRITE) == 0 &&
	    !change_wiring &&
	    (!m->vmp_wpmapped
#if VM_OBJECT_ACCESS_TRACKING
	    || object->access_tracking
#endif                                
	    )) {
		  
                                       
                                         
                                         
                                    
                      
                                    
                               
                                      
                    
     

		                                        
		if (!pmap_has_prot_policy(pmap, fault_info->pmap_options & PMAP_OPTIONS_TRANSLATED_ALLOW_EXECUTE, *prot)) {
			*prot &= ~VM_PROT_WRITE;
		} else {
			assert(cs_bypass);
		}
	}
	if (m->vmp_pmapped == FALSE) {
		if (m->vmp_clustered) {
			if (*type_of_fault == DBG_CACHE_HIT_FAULT) {
				  
                                      
                                                                  
                                         
                                               
       
				if (object->internal) {
					*type_of_fault = DBG_PAGEIND_FAULT;
				} else {
					*type_of_fault = DBG_PAGEINV_FAULT;
				}

				VM_PAGE_COUNT_AS_PAGEIN(m);
			}
			VM_PAGE_CONSUME_CLUSTERED(m);
		}
	}

	if (*type_of_fault != DBG_COW_FAULT) {
		DTRACE_VM2(as_fault, int, 1, (uint64_t *), NULL);

		if (pmap == kernel_pmap) {
			DTRACE_VM2(kernel_asflt, int, 1, (uint64_t *), NULL);
		}
	}

	kr = vm_fault_validate_cs(cs_bypass, object, m, pmap, vaddr,
	    *prot, caller_prot, fault_page_size, fault_phys_offset,
	    fault_info, &is_tainted);
	if (kr == KERN_SUCCESS) {
		  
                                                                                         
                                                          
     
		*page_needs_data_sync = vm_fault_enter_set_mapped(object, m, *prot, fault_type);
		if ((fault_type & VM_PROT_WRITE) && is_tainted) {
			  
                                                          
                                                                     
                                           
      

			  
                          
                              
      
			assert(pmap_get_vm_map_cs_enforced(pmap));
			pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m));
			  
                                                  
                                                    
                                                  
                                    
                                               
                                              
                                                   
      
			if (!cs_bypass) {
				assert(!pmap_has_prot_policy(pmap, fault_info->pmap_options & PMAP_OPTIONS_TRANSLATED_ALLOW_EXECUTE, *prot));
				*prot &= ~VM_PROT_EXECUTE;
			}
		}
		assert(VM_PAGE_OBJECT(m) == object);

#if VM_OBJECT_ACCESS_TRACKING
		if (object->access_tracking) {
			DTRACE_VM2(access_tracking, vm_map_offset_t, vaddr, int, fault_type);
			if (fault_type & VM_PROT_WRITE) {
				object->access_tracking_writes++;
				vm_object_access_tracking_writes++;
			} else {
				object->access_tracking_reads++;
				vm_object_access_tracking_reads++;
			}
		}
#endif                                
	}

	return kr;
}

  
                                   
                               
  
                                                                     
                                                              
                                                             
                               
   
kern_return_t
vm_fault_enter(
	vm_page_t m,
	pmap_t pmap,
	vm_map_offset_t vaddr,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	vm_prot_t prot,
	vm_prot_t caller_prot,
	boolean_t wired,
	boolean_t change_wiring,
	vm_tag_t  wire_tag,
	vm_object_fault_info_t fault_info,
	boolean_t *need_retry,
	int *type_of_fault)
{
	kern_return_t   kr;
	vm_object_t     object;
	bool            page_needs_data_sync;
	vm_prot_t       fault_type;
	int             pmap_options = fault_info->pmap_options;

	if (VM_PAGE_GET_PHYS_PAGE(m) == vm_page_guard_addr) {
		assert(m->vmp_fictitious);
		return KERN_SUCCESS;
	}

	fault_type = change_wiring ? VM_PROT_NONE : caller_prot;

	kr = vm_fault_enter_prepare(m, pmap, vaddr, &prot, caller_prot,
	    fault_page_size, fault_phys_offset, change_wiring, fault_type,
	    fault_info, type_of_fault, &page_needs_data_sync);
	object = VM_PAGE_OBJECT(m);

	vm_fault_enqueue_page(object, m, wired, change_wiring, wire_tag, fault_info->no_cache, type_of_fault, kr);

	if (kr == KERN_SUCCESS) {
		if (page_needs_data_sync) {
			pmap_sync_page_data_phys(VM_PAGE_GET_PHYS_PAGE(m));
		}

		kr = vm_fault_pmap_enter_with_object_lock(object, pmap, vaddr,
		    fault_page_size, fault_phys_offset, m,
		    &prot, caller_prot, fault_type, wired, pmap_options, need_retry);
	}

	return kr;
}

void
vm_pre_fault(vm_map_offset_t vaddr, vm_prot_t prot)
{
	if (pmap_find_phys(current_map()->pmap, vaddr) == 0) {
		vm_fault(current_map(),               
		    vaddr,                              
		    prot,                                    
		    FALSE,                                      
		    VM_KERN_MEMORY_NONE,                           
		    THREAD_UNINT,                               
		    NULL,                                     
		    0                       );
	}
}


  
                    
           
                                               
                                              
           
                                             
                  
                                               
                                                    
                                                       
                                          
   

extern uint64_t get_current_unique_pid(void);

unsigned long vm_fault_collapse_total = 0;
unsigned long vm_fault_collapse_skipped = 0;


kern_return_t
vm_fault_external(
	vm_map_t        map,
	vm_map_offset_t vaddr,
	vm_prot_t       fault_type,
	boolean_t       change_wiring,
	int             interruptible,
	pmap_t          caller_pmap,
	vm_map_offset_t caller_pmap_addr)
{
	return vm_fault_internal(map, vaddr, fault_type, change_wiring,
	           change_wiring ? vm_tag_bt() : VM_KERN_MEMORY_NONE,
	           interruptible, caller_pmap, caller_pmap_addr,
	           NULL);
}

kern_return_t
vm_fault(
	vm_map_t        map,
	vm_map_offset_t vaddr,
	vm_prot_t       fault_type,
	boolean_t       change_wiring,
	vm_tag_t        wire_tag,                                                                   
	int             interruptible,
	pmap_t          caller_pmap,
	vm_map_offset_t caller_pmap_addr)
{
	return vm_fault_internal(map, vaddr, fault_type, change_wiring, wire_tag,
	           interruptible, caller_pmap, caller_pmap_addr,
	           NULL);
}

static boolean_t
current_proc_is_privileged(void)
{
	return csproc_get_platform_binary(current_proc());
}

uint64_t vm_copied_on_read = 0;

  
                                  
                                                                          
                                                                     
  
                
                             
                           
                                                      
                           
  
                 
                       
                            
                              
                                          
   
static void
vm_fault_complete(
	vm_map_t map,
	vm_map_t real_map,
	vm_object_t object,
	vm_object_t m_object,
	vm_page_t m,
	vm_map_offset_t offset,
	vm_map_offset_t trace_real_vaddr,
	vm_object_fault_info_t fault_info,
	vm_prot_t caller_prot,
#if CONFIG_DTRACE
	vm_map_offset_t real_vaddr,
#else
	__unused vm_map_offset_t real_vaddr,
#endif                    
	int type_of_fault,
	boolean_t need_retry,
	kern_return_t kr,
	ppnum_t *physpage_p,
	vm_prot_t prot,
	vm_object_t top_object,
	boolean_t need_collapse,
	vm_map_offset_t cur_offset,
	vm_prot_t fault_type,
	vm_object_t *written_on_object,
	memory_object_t *written_on_pager,
	vm_object_offset_t *written_on_offset)
{
	int     event_code = 0;
	vm_map_lock_assert_shared(map);
	vm_object_lock_assert_held(m_object);
	if (top_object != VM_OBJECT_NULL) {
		vm_object_lock_assert_held(top_object);
	}
	vm_map_lock_assert_held(real_map);

	if (m_object->internal) {
		event_code = (MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_REAL_FAULT_ADDR_INTERNAL));
	} else if (m_object->object_is_shared_cache) {
		event_code = (MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_REAL_FAULT_ADDR_SHAREDCACHE));
	} else {
		event_code = (MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_REAL_FAULT_ADDR_EXTERNAL));
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, event_code, trace_real_vaddr, (fault_info->user_tag << 16) | (caller_prot << 8) | type_of_fault, m->vmp_offset, get_current_unique_pid(), 0);
	if (need_retry == FALSE) {
		KDBG_FILTERED(MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_REAL_FAULT_FAST), get_current_unique_pid(), 0, 0, 0, 0);
	}
	DTRACE_VM6(real_fault, vm_map_offset_t, real_vaddr, vm_map_offset_t, m->vmp_offset, int, event_code, int, caller_prot, int, type_of_fault, int, fault_info->user_tag);
	if (kr == KERN_SUCCESS &&
	    physpage_p != NULL) {
		                                   
		*physpage_p = VM_PAGE_GET_PHYS_PAGE(m);
		if (prot & VM_PROT_WRITE) {
			vm_object_lock_assert_exclusive(m_object);
			m->vmp_dirty = TRUE;
		}
	}

	if (top_object != VM_OBJECT_NULL) {
		  
                                     
                            
                                       
                                 
                                      
                                         
                     
     
		vm_object_unlock(top_object);
		top_object = VM_OBJECT_NULL;
	}

	if (need_collapse == TRUE) {
		vm_object_collapse(object, vm_object_trunc_page(offset), TRUE);
	}

	if (need_retry == FALSE &&
	    (type_of_fault == DBG_PAGEIND_FAULT || type_of_fault == DBG_PAGEINV_FAULT || type_of_fault == DBG_CACHE_HIT_FAULT)) {
		  
                                             
                                              
                           
     
		vm_fault_is_sequential(m_object, cur_offset, fault_info->behavior);

		vm_fault_deactivate_behind(m_object, cur_offset, fault_info->behavior);
	}
	  
                                   
    
	if (m->vmp_busy) {
		vm_object_lock_assert_exclusive(m_object);
		PAGE_WAKEUP_DONE(m);
	}

	if (need_retry == FALSE && !m_object->internal && (fault_type & VM_PROT_WRITE)) {
		vm_object_paging_begin(m_object);

		assert(*written_on_object == VM_OBJECT_NULL);
		*written_on_object = m_object;
		*written_on_pager = m_object->pager;
		*written_on_offset = m_object->paging_offset + m->vmp_offset;
	}
	vm_object_unlock(object);

	vm_map_unlock_read(map);
	if (real_map != map) {
		vm_map_unlock(real_map);
	}
}

static inline int
vm_fault_type_for_tracing(boolean_t need_copy_on_read, int type_of_fault)
{
	if (need_copy_on_read && type_of_fault == DBG_COW_FAULT) {
		return DBG_COR_FAULT;
	}
	return type_of_fault;
}

kern_return_t
vm_fault_internal(
	vm_map_t        map,
	vm_map_offset_t vaddr,
	vm_prot_t       caller_prot,
	boolean_t       change_wiring,
	vm_tag_t        wire_tag,                                                                   
	int             interruptible,
	pmap_t          caller_pmap,
	vm_map_offset_t caller_pmap_addr,
	ppnum_t         *physpage_p)
{
	vm_map_version_t        version;                                           
	boolean_t               wired;                                             
	vm_object_t             object;                               
	vm_object_offset_t      offset;                               
	vm_prot_t               prot;                                       
	vm_object_t             old_copy_object;                        
	vm_page_t               result_page;                                 
	vm_page_t               top_page;                             
	kern_return_t           kr;

	vm_page_t               m;                                      
	kern_return_t           error_code;
	vm_object_t             cur_object;
	vm_object_t             m_object = NULL;
	vm_object_offset_t      cur_offset;
	vm_page_t               cur_m;
	vm_object_t             new_object;
	int                     type_of_fault;
	pmap_t                  pmap;
	wait_interrupt_t        interruptible_state;
	vm_map_t                real_map = map;
	vm_map_t                original_map = map;
	bool                    object_locks_dropped = FALSE;
	vm_prot_t               fault_type;
	vm_prot_t               original_fault_type;
	struct vm_object_fault_info fault_info = {};
	bool                    need_collapse = FALSE;
	boolean_t               need_retry = FALSE;
	boolean_t               *need_retry_ptr = NULL;
	uint8_t                 object_lock_type = 0;
	uint8_t                 cur_object_lock_type;
	vm_object_t             top_object = VM_OBJECT_NULL;
	vm_object_t             written_on_object = VM_OBJECT_NULL;
	memory_object_t         written_on_pager = NULL;
	vm_object_offset_t      written_on_offset = 0;
	int                     throttle_delay;
	int                     compressed_count_delta;
	uint8_t                 grab_options;
	bool                    need_copy;
	bool                    need_copy_on_read;
	vm_map_offset_t         trace_vaddr;
	vm_map_offset_t         trace_real_vaddr;
	vm_map_size_t           fault_page_size;
	vm_map_size_t           fault_page_mask;
	int                     fault_page_shift;
	vm_map_offset_t         fault_phys_offset;
	vm_map_offset_t         real_vaddr;
	bool                    resilient_media_retry = FALSE;
	vm_object_t             resilient_media_object = VM_OBJECT_NULL;
	vm_object_offset_t      resilient_media_offset = (vm_object_offset_t)-1;
	bool                    page_needs_data_sync = false;
	  
                                                                    
                                                
                                                                 
                                                                     
    
	bool                    object_is_contended = false;

	real_vaddr = vaddr;
	trace_real_vaddr = vaddr;

	if (VM_MAP_PAGE_SIZE(original_map) < PAGE_SIZE) {
		fault_phys_offset = (vm_map_offset_t)-1;
		fault_page_size = VM_MAP_PAGE_SIZE(original_map);
		fault_page_mask = VM_MAP_PAGE_MASK(original_map);
		fault_page_shift = VM_MAP_PAGE_SHIFT(original_map);
		if (fault_page_size < PAGE_SIZE) {
			DEBUG4K_FAULT("map %p vaddr 0x%llx caller_prot 0x%x\n", map, (uint64_t)trace_real_vaddr, caller_prot);
			vaddr = vm_map_trunc_page(vaddr, fault_page_mask);
		}
	} else {
		fault_phys_offset = 0;
		fault_page_size = PAGE_SIZE;
		fault_page_mask = PAGE_MASK;
		fault_page_shift = PAGE_SHIFT;
		vaddr = vm_map_trunc_page(vaddr, PAGE_MASK);
	}

	if (map == kernel_map) {
		trace_vaddr = VM_KERNEL_ADDRHIDE(vaddr);
		trace_real_vaddr = VM_KERNEL_ADDRHIDE(trace_real_vaddr);
	} else {
		trace_vaddr = vaddr;
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (MACHDBG_CODE(DBG_MACH_VM, 2)) | DBG_FUNC_START,
	    ((uint64_t)trace_vaddr >> 32),
	    trace_vaddr,
	    (map == kernel_map),
	    0,
	    0);

	if (get_preemption_level() != 0) {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (MACHDBG_CODE(DBG_MACH_VM, 2)) | DBG_FUNC_END,
		    ((uint64_t)trace_vaddr >> 32),
		    trace_vaddr,
		    KERN_FAILURE,
		    0,
		    0);

		return KERN_FAILURE;
	}

	thread_t cthread = current_thread();
	bool      rtfault = (cthread->sched_mode == TH_MODE_REALTIME);
	uint64_t fstart = 0;

	if (rtfault) {
		fstart = mach_continuous_time();
	}

	interruptible_state = thread_interrupt_level(interruptible);

	fault_type = (change_wiring ? VM_PROT_NONE : caller_prot);

	counter_inc(&vm_statistics_faults);
	counter_inc(&current_task()->faults);
	original_fault_type = fault_type;

	need_copy = FALSE;
	if (fault_type & VM_PROT_WRITE) {
		need_copy = TRUE;
	}

	if (need_copy || change_wiring) {
		object_lock_type = OBJECT_LOCK_EXCLUSIVE;
	} else {
		object_lock_type = OBJECT_LOCK_SHARED;
	}

	cur_object_lock_type = OBJECT_LOCK_SHARED;

	if ((map == kernel_map) && (caller_prot & VM_PROT_WRITE)) {
		if (compressor_map) {
			if ((vaddr >= vm_map_min(compressor_map)) && (vaddr < vm_map_max(compressor_map))) {
				panic("Write fault on compressor map, va: %p type: %u bounds: %p->%p", (void *) vaddr, caller_prot, (void *) vm_map_min(compressor_map), (void *) vm_map_max(compressor_map));
			}
		}
	}
RetryFault:
	assert(written_on_object == VM_OBJECT_NULL);

	  
                                          
                                       
                                            
    
	type_of_fault = DBG_CACHE_HIT_FAULT;

	  
                                                 
                           
    
	fault_type = original_fault_type;
	map = original_map;
	vm_map_lock_read(map);

	if (resilient_media_retry) {
		  
                                                         
                                                         
                                                       
                                                      
                                                   
     
		need_copy = TRUE;
		object_lock_type = OBJECT_LOCK_EXCLUSIVE;
	}

	kr = vm_map_lookup_locked(&map, vaddr,
	    (fault_type | (need_copy ? VM_PROT_COPY : 0)),
	    object_lock_type, &version,
	    &object, &offset, &prot, &wired,
	    &fault_info,
	    &real_map,
	    &object_is_contended);

	if (kr != KERN_SUCCESS) {
		vm_map_unlock_read(map);
		goto done;
	}


	pmap = real_map->pmap;
	fault_info.interruptible = interruptible;
	fault_info.stealth = FALSE;
	fault_info.io_sync = FALSE;
	fault_info.mark_zf_absent = FALSE;
	fault_info.batch_pmap_op = FALSE;

	if (resilient_media_retry) {
		  
                                                            
                                              
                                                           
                                        
     
		assert(resilient_media_object != VM_OBJECT_NULL);
		assert(resilient_media_offset != (vm_object_offset_t)-1);
		if (object != VM_OBJECT_NULL &&
		    object == resilient_media_object &&
		    offset == resilient_media_offset &&
		    fault_info.resilient_media) {
			  
                                                  
                                                
                                                     
                                                  
      
                                                                                                                                        
		} else {
			                                 
                                                                                                                                                                                                                                        
			resilient_media_retry = FALSE;
			                                                  
                                                                                                                                   
			vm_object_deallocate(resilient_media_object);
			resilient_media_object = VM_OBJECT_NULL;
			resilient_media_offset = (vm_object_offset_t)-1;
		}
	} else {
		assert(resilient_media_object == VM_OBJECT_NULL);
		resilient_media_offset = (vm_object_offset_t)-1;
	}

	  
                                                                  
                                   
    
	if (wired) {
		fault_type = prot | VM_PROT_WRITE;
	}
	if (wired || need_copy) {
		  
                                                 
                                                 
     
		if (object_lock_type == OBJECT_LOCK_SHARED) {
			object_lock_type = OBJECT_LOCK_EXCLUSIVE;

			if (vm_object_lock_upgrade(object) == FALSE) {
				  
                                     
                                
       
				vm_object_lock(object);
			}
		}
	}

#if     VM_FAULT_CLASSIFY
	  
                                 
    
	vm_fault_classify(object, offset, fault_type);
#endif
	  
                                                        
                                                         
                                                             
                                                             
                                                           
                                             
   
                                   
                                                        
                                                         
                                                       
                                            
                              
                                         
                                        
                                           
                                   
                                          
   
                                                          
                                                          
                                                                      
                                                             
                                                                
   
                                                              
                                                       
                                
   
    

#if defined(__arm64__)
	  
                                             
                                               
    
	if (fault_type == VM_PROT_READ &&
	    (prot & VM_PROT_EXECUTE) &&
	    !(prot & VM_PROT_READ) &&
	    pmap_enforces_execute_only(pmap)) {
		vm_object_unlock(object);
		vm_map_unlock_read(map);
		if (real_map != map) {
			vm_map_unlock(real_map);
		}
		kr = KERN_PROTECTION_FAILURE;
		goto done;
	}
#endif

	fault_phys_offset = (vm_map_offset_t)offset - vm_map_trunc_page((vm_map_offset_t)offset, PAGE_MASK);

	  
                                                         
                                                       
                                                              
    
	if (object->copy_strategy == MEMORY_OBJECT_COPY_DELAY &&
	    object->copy != VM_OBJECT_NULL && (fault_type & VM_PROT_WRITE)) {
		goto handle_copy_delay;
	}

	cur_object = object;
	cur_offset = offset;

	grab_options = 0;
#if CONFIG_SECLUDED_MEMORY
	if (object->can_grab_secluded) {
		grab_options |= VM_PAGE_GRAB_SECLUDED;
	}
#endif                             

	while (TRUE) {
		if (!cur_object->pager_created &&
		    cur_object->phys_contiguous) {                
			break;
		}

		if (cur_object->blocked_access) {
			  
                                                
                                  
      
			break;
		}

		m = vm_page_lookup(cur_object, vm_object_trunc_page(cur_offset));
		m_object = NULL;

		if (m != VM_PAGE_NULL) {
			m_object = cur_object;

			if (m->vmp_busy) {
				wait_result_t   result;

				  
                                                   
                                                         
       
				if (object != cur_object) {
					if (cur_object_lock_type == OBJECT_LOCK_SHARED) {
						cur_object_lock_type = OBJECT_LOCK_EXCLUSIVE;

						if (vm_object_lock_upgrade(cur_object) == FALSE) {
							  
                                                
                                               
                                            
                                          
                                        
          
							vm_object_unlock(object);

							vm_map_unlock_read(map);
							if (real_map != map) {
								vm_map_unlock(real_map);
							}

							goto RetryFault;
						}
					}
				} else if (object_lock_type == OBJECT_LOCK_SHARED) {
					object_lock_type = OBJECT_LOCK_EXCLUSIVE;

					if (vm_object_lock_upgrade(object) == FALSE) {
						  
                                                     
                                                      
                                              
                                               
                              
                                             
                                             
         
						vm_object_lock(object);

						continue;
					}
				}
				if ((m->vmp_q_state == VM_PAGE_ON_PAGEOUT_Q) && m_object->internal) {
					  
                                                                
                                                        
                                                           
                                                   
       
                                                              
                               
        
					assert(VM_CONFIG_COMPRESSOR_IS_PRESENT);

					vm_page_lock_queues();

					if (m->vmp_q_state == VM_PAGE_ON_PAGEOUT_Q) {
						vm_pageout_throttle_up(m);
						vm_page_unlock_queues();

						PAGE_WAKEUP_DONE(m);
						goto reclaimed_from_pageout;
					}
					vm_page_unlock_queues();
				}
				if (object != cur_object) {
					vm_object_unlock(object);
				}

				vm_map_unlock_read(map);
				if (real_map != map) {
					vm_map_unlock(real_map);
				}

				result = PAGE_ASSERT_WAIT(m, interruptible);

				vm_object_unlock(cur_object);

				if (result == THREAD_WAITING) {
					result = thread_block(THREAD_CONTINUE_NULL);
				}
				if (result == THREAD_AWAKENED || result == THREAD_RESTART) {
					goto RetryFault;
				}

				kr = KERN_ABORTED;
				goto done;
			}
reclaimed_from_pageout:
			if (m->vmp_laundry) {
				if (object != cur_object) {
					if (cur_object_lock_type == OBJECT_LOCK_SHARED) {
						cur_object_lock_type = OBJECT_LOCK_EXCLUSIVE;

						vm_object_unlock(object);
						vm_object_unlock(cur_object);

						vm_map_unlock_read(map);
						if (real_map != map) {
							vm_map_unlock(real_map);
						}

						goto RetryFault;
					}
				} else if (object_lock_type == OBJECT_LOCK_SHARED) {
					object_lock_type = OBJECT_LOCK_EXCLUSIVE;

					if (vm_object_lock_upgrade(object) == FALSE) {
						  
                                                     
                                                      
                                              
                                               
                              
                                             
                                             
         
						vm_object_lock(object);

						continue;
					}
				}
				vm_pageout_steal_laundry(m, FALSE);
			}

			if (VM_PAGE_GET_PHYS_PAGE(m) == vm_page_guard_addr) {
				  
                                                 
       
				break;
			}
			if (m->vmp_unusual && (m->vmp_error || m->vmp_restart || m->vmp_private || m->vmp_absent)) {
				  
                                                     
       
				break;
			}
			if (VM_OBJECT_PURGEABLE_FAULT_ERROR(m_object)) {
				if (object != cur_object) {
					vm_object_unlock(object);
				}
				vm_map_unlock_read(map);
				if (real_map != map) {
					vm_map_unlock(real_map);
				}
				vm_object_unlock(cur_object);
				kr = KERN_MEMORY_ERROR;
				goto done;
			}
			assert(m_object == VM_PAGE_OBJECT(m));

			if (vm_fault_cs_need_validation(map->pmap, m, m_object,
			    PAGE_SIZE, 0) ||
			    (physpage_p != NULL && (prot & VM_PROT_WRITE))) {
upgrade_lock_and_retry:
				  
                                          
                                        
                                              
       
				if (object != cur_object) {
					if (cur_object_lock_type == OBJECT_LOCK_SHARED) {
						vm_object_unlock(object);
						vm_object_unlock(cur_object);

						cur_object_lock_type = OBJECT_LOCK_EXCLUSIVE;

						vm_map_unlock_read(map);
						if (real_map != map) {
							vm_map_unlock(real_map);
						}

						goto RetryFault;
					}
				} else if (object_lock_type == OBJECT_LOCK_SHARED) {
					object_lock_type = OBJECT_LOCK_EXCLUSIVE;

					if (vm_object_lock_upgrade(object) == FALSE) {
						  
                                                     
                                                      
                                              
                                               
                              
                                             
                                             
         
						vm_object_lock(object);

						continue;
					}
				}
			}
			  
                                 
                                         
                                
                               
      

			if (object == cur_object && object->copy == VM_OBJECT_NULL) {
				goto FastPmapEnter;
			}

			if (!need_copy &&
			    !fault_info.no_copy_on_read &&
			    cur_object != object &&
			    !cur_object->internal &&
			    !cur_object->pager_trusted &&
			    vm_protect_privileged_from_untrusted &&
			    !cur_object->code_signed &&
			    current_proc_is_privileged()) {
				  
                                               
                                                 
                                            
                                                 
                                          
                                                  
                                                   
                                       
                                                 
                                            
                                  
      
                                              
                                               
                           
      
                                                 
                                                  
                                              
                                            
                                                
                                                   
                                              
                                                 
                         
       
                                                                                                                                                                                                       
				vm_copied_on_read++;
				need_copy = TRUE;

				vm_object_unlock(object);
				vm_object_unlock(cur_object);
				object_lock_type = OBJECT_LOCK_EXCLUSIVE;
				vm_map_unlock_read(map);
				if (real_map != map) {
					vm_map_unlock(real_map);
				}
				goto RetryFault;
			}

			if (!(fault_type & VM_PROT_WRITE) && !need_copy) {
				if (!pmap_has_prot_policy(pmap, fault_info.pmap_options & PMAP_OPTIONS_TRANSLATED_ALLOW_EXECUTE, prot)) {
					prot &= ~VM_PROT_WRITE;
				} else {
					  
                                            
                                         
                                            
                                            
                                         
                                         
            
        
					assert(fault_info.cs_bypass);
				}

				if (object != cur_object) {
					  
                                            
                                           
                                          
                                        
                                      
               
                                            
                                       
                                            
                                             
                                             
                                           
                          
       
                                             
                                      
        
					top_object = object;

					  
                                                  
        
					object = cur_object;
					object_lock_type = cur_object_lock_type;
				}
FastPmapEnter:
				assert(m_object == VM_PAGE_OBJECT(m));

				  
                                    
                                     
                            
                              
                                               
                                                          
       
				if (top_object != VM_OBJECT_NULL || object_lock_type != OBJECT_LOCK_EXCLUSIVE) {
					need_retry_ptr = &need_retry;
				} else {
					need_retry_ptr = NULL;
				}

				if (fault_page_size < PAGE_SIZE) {
					DEBUG4K_FAULT("map %p original %p pmap %p va 0x%llx caller pmap %p va 0x%llx pa 0x%llx (0x%llx+0x%llx) prot 0x%x caller_prot 0x%x\n", map, original_map, pmap, (uint64_t)vaddr, caller_pmap, (uint64_t)caller_pmap_addr, (uint64_t)((((pmap_paddr_t)VM_PAGE_GET_PHYS_PAGE(m)) << PAGE_SHIFT) + fault_phys_offset), (uint64_t)(((pmap_paddr_t)VM_PAGE_GET_PHYS_PAGE(m)) << PAGE_SHIFT), (uint64_t)fault_phys_offset, prot, caller_prot);
					assertf((!(fault_phys_offset & FOURK_PAGE_MASK) &&
					    fault_phys_offset < PAGE_SIZE),
					    "0x%llx\n", (uint64_t)fault_phys_offset);
				} else {
					assertf(fault_phys_offset == 0,
					    "0x%llx\n", (uint64_t)fault_phys_offset);
				}

				if (caller_pmap) {
					kr = vm_fault_enter(m,
					    caller_pmap,
					    caller_pmap_addr,
					    fault_page_size,
					    fault_phys_offset,
					    prot,
					    caller_prot,
					    wired,
					    change_wiring,
					    wire_tag,
					    &fault_info,
					    need_retry_ptr,
					    &type_of_fault);
				} else {
					kr = vm_fault_enter(m,
					    pmap,
					    vaddr,
					    fault_page_size,
					    fault_phys_offset,
					    prot,
					    caller_prot,
					    wired,
					    change_wiring,
					    wire_tag,
					    &fault_info,
					    need_retry_ptr,
					    &type_of_fault);
				}

				vm_fault_complete(
					map,
					real_map,
					object,
					m_object,
					m,
					offset,
					trace_real_vaddr,
					&fault_info,
					caller_prot,
					real_vaddr,
					vm_fault_type_for_tracing(need_copy_on_read, type_of_fault),
					need_retry,
					kr,
					physpage_p,
					prot,
					top_object,
					need_collapse,
					cur_offset,
					fault_type,
					&written_on_object,
					&written_on_pager,
					&written_on_offset);
				top_object = VM_OBJECT_NULL;
				if (need_retry == TRUE) {
					  
                                                          
                                                          
                                                         
                                                        
                                                                
                                                                     
        
					(void)pmap_enter_options(
						pmap, vaddr, 0, 0, 0, 0, 0,
						PMAP_OPTIONS_NOENTER, NULL);

					need_retry = FALSE;
					goto RetryFault;
				}
				goto done;
			}
			  
                         
      
			assert(object_lock_type == OBJECT_LOCK_EXCLUSIVE);

			  
                            
                                                 
                                              
                                                
                                         
      
			if (cur_object == object) {
				  
                                 
                              
       
				break;
			}

			  
                                              
                                               
            
      
			assert(m_object == VM_PAGE_OBJECT(m));

			if ((cur_object_lock_type == OBJECT_LOCK_SHARED) &&
			    vm_fault_cs_need_validation(NULL, m, m_object,
			    PAGE_SIZE, 0)) {
				goto upgrade_lock_and_retry;
			}

			  
                                               
                                              
                                                
                         
     
                                         
                                              
                                          
                                            
                                           
      
			cur_m = m;
			m = vm_page_grab_options(grab_options);
			m_object = NULL;

			if (m == VM_PAGE_NULL) {
				  
                                          
                              
       
				break;
			}
			  
                                                    
     
                                               
                    
      
			vm_page_copy(cur_m, m);
			vm_page_insert(m, object, vm_object_trunc_page(offset));
			if (VM_MAP_PAGE_MASK(map) != PAGE_MASK) {
				DEBUG4K_FAULT("map %p vaddr 0x%llx page %p [%p 0x%llx] copied to %p [%p 0x%llx]\n", map, (uint64_t)vaddr, cur_m, VM_PAGE_OBJECT(cur_m), cur_m->vmp_offset, m, VM_PAGE_OBJECT(m), m->vmp_offset);
			}
			m_object = object;
			SET_PAGE_DIRTY(m, FALSE);

			  
                                              
      
			if (object->ref_count > 1 && cur_m->vmp_pmapped) {
				pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(cur_m));
			} else if (VM_MAP_PAGE_SIZE(map) < PAGE_SIZE) {
				  
                                               
                                              
                                                 
                                               
                               
                                                
                                              
                                               
                                               
                                                
                                                
                                                   
               
       
				  
                                        
                                                
                                                 
                                                  
                                                 
                                                  
                                                  
             
                                                
                                                 
                                                
           
                                               
                                 
       
				DEBUG4K_FAULT("pmap_disconnect() page %p object %p offset 0x%llx phys 0x%x\n", cur_m, VM_PAGE_OBJECT(cur_m), cur_m->vmp_offset, VM_PAGE_GET_PHYS_PAGE(cur_m));
				pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(cur_m));
			}

			if (cur_m->vmp_clustered) {
				VM_PAGE_COUNT_AS_PAGEIN(cur_m);
				VM_PAGE_CONSUME_CLUSTERED(cur_m);
				vm_fault_is_sequential(cur_object, cur_offset, fault_info.behavior);
			}
			need_collapse = TRUE;

			if (!cur_object->internal &&
			    cur_object->copy_strategy == MEMORY_OBJECT_COPY_DELAY) {
				  
                                       
                                            
                                              
                                                  
                                                 
                         
       
				if (cur_object->copy == object) {
					  
                                     
                                    
                                       
        
					need_collapse = FALSE;
				} else if (cur_object->copy == object->shadow &&
				    object->shadow->resident_page_count == 0) {
					  
                                            
        
					need_collapse = FALSE;
				}
			}
			vm_object_unlock(cur_object);

			if (need_collapse == FALSE) {
				vm_fault_collapse_skipped++;
			}
			vm_fault_collapse_total++;

			type_of_fault = DBG_COW_FAULT;
			counter_inc(&vm_statistics_cow_faults);
			DTRACE_VM2(cow_fault, int, 1, (uint64_t *), NULL);
			current_task()->cow_faults++;

			goto FastPmapEnter;
		} else {
			  
                                                    
      
			if (cur_object->pager_created) {
				vm_external_state_t compressor_external_state = VM_EXTERNAL_STATE_UNKNOWN;

				if (MUST_ASK_PAGER(cur_object, cur_offset, compressor_external_state) == TRUE) {
					int             my_fault_type;
					uint8_t         c_flags = C_DONT_BLOCK;
					bool            insert_cur_object = FALSE;

					  
                                      
                                    
                                                  
       
                                                                   
                                                             
        
					if (compressor_external_state != VM_EXTERNAL_STATE_EXISTS) {
						break;
					}

					if (map == kernel_map || real_map == kernel_map) {
						  
                                                           
                                                           
                                                                
         
						break;
					}
					if (object != cur_object) {
						if (fault_type & VM_PROT_WRITE) {
							c_flags |= C_KEEP;
						} else {
							insert_cur_object = TRUE;
						}
					}
					if (insert_cur_object == TRUE) {
						if (cur_object_lock_type == OBJECT_LOCK_SHARED) {
							cur_object_lock_type = OBJECT_LOCK_EXCLUSIVE;

							if (vm_object_lock_upgrade(cur_object) == FALSE) {
								  
                                                 
                                                
                                             
                                           
                                         
           
								vm_object_unlock(object);

								vm_map_unlock_read(map);
								if (real_map != map) {
									vm_map_unlock(real_map);
								}

								goto RetryFault;
							}
						}
					} else if (object_lock_type == OBJECT_LOCK_SHARED) {
						object_lock_type = OBJECT_LOCK_EXCLUSIVE;

						if (object != cur_object) {
							  
                                                
                                                  
                                               
                                                  
                                                  
                                                     
                                                   
                       
          
							vm_object_unlock(object);
							vm_object_unlock(cur_object);

							vm_map_unlock_read(map);
							if (real_map != map) {
								vm_map_unlock(real_map);
							}

							goto RetryFault;
						}
						if (vm_object_lock_upgrade(object) == FALSE) {
							  
                                                      
                                                       
                                               
                                                
                               
                                              
                                              
          
							vm_object_lock(object);

							continue;
						}
					}
					m = vm_page_grab_options(grab_options);
					m_object = NULL;

					if (m == VM_PAGE_NULL) {
						  
                                            
                                
         
						break;
					}

					  
                                        
                            
                                       
        
					bool      shared_lock;
					if ((object == cur_object &&
					    object_lock_type == OBJECT_LOCK_EXCLUSIVE) ||
					    (object != cur_object &&
					    cur_object_lock_type == OBJECT_LOCK_EXCLUSIVE)) {
						shared_lock = FALSE;
					} else {
						shared_lock = TRUE;
					}

					kr = vm_compressor_pager_get(
						cur_object->pager,
						(vm_object_trunc_page(cur_offset)
						+ cur_object->paging_offset),
						VM_PAGE_GET_PHYS_PAGE(m),
						&my_fault_type,
						c_flags,
						&compressed_count_delta);

					vm_compressor_pager_count(
						cur_object->pager,
						compressed_count_delta,
						shared_lock,
						cur_object);

					if (kr != KERN_SUCCESS) {
						vm_page_release(m, FALSE);
						m = VM_PAGE_NULL;
					}
					  
                                            
                                     
                                            
                                         
        
					if (kr == KERN_MEMORY_FAILURE) {
						if (object != cur_object) {
							vm_object_unlock(cur_object);
						}
						vm_object_unlock(object);
						vm_map_unlock_read(map);
						if (real_map != map) {
							vm_map_unlock(real_map);
						}
						goto done;
					} else if (kr != KERN_SUCCESS) {
						break;
					}
					m->vmp_dirty = TRUE;

					  
                                       
                                         
                                           
                                        
                                         
                        
        
					if (object != cur_object &&
					    !insert_cur_object) {
						  
                                  
                                   
                                 
        
                             
                               
                  
                                  
                                   
                              
                              
                                  
                                 
              
         
					} else if (((cur_object->purgable ==
					    VM_PURGABLE_DENY) &&
					    (!cur_object->vo_ledger_tag)) ||
					    (cur_object->vo_owner ==
					    NULL)) {
						  
                                      
                                    
                                 
                                 
                
         
					} else {
						  
                            
                                  
                            
         
						vm_object_owner_compressed_update(
							cur_object,
							-1);
					}

					if (insert_cur_object) {
						vm_page_insert(m, cur_object, vm_object_trunc_page(cur_offset));
						m_object = cur_object;
					} else {
						vm_page_insert(m, object, vm_object_trunc_page(offset));
						m_object = object;
					}

					if ((m_object->wimg_bits & VM_WIMG_MASK) != VM_WIMG_USE_DEFAULT) {
						  
                                      
                                  
                                 
                                 
         
						pmap_sync_page_attributes_phys(VM_PAGE_GET_PHYS_PAGE(m));
					}

					type_of_fault = my_fault_type;

					VM_STAT_DECOMPRESSIONS();

					if (cur_object != object) {
						if (insert_cur_object) {
							top_object = object;
							  
                                                    
          
							object = cur_object;
							object_lock_type = cur_object_lock_type;
						} else {
							vm_object_unlock(cur_object);
							cur_object = object;
						}
					}
					goto FastPmapEnter;
				}
				  
                                          
                                            
       
			}
			if (cur_object->shadow == VM_OBJECT_NULL ||
			    resilient_media_retry) {
				  
                                  
                                         
       
				if (cur_object->shadow_severed ||
				    VM_OBJECT_PURGEABLE_FAULT_ERROR(cur_object) ||
				    cur_object == compressor_object ||
				    cur_object == kernel_object ||
				    cur_object == vm_submap_object) {
					if (object != cur_object) {
						vm_object_unlock(cur_object);
					}
					vm_object_unlock(object);

					vm_map_unlock_read(map);
					if (real_map != map) {
						vm_map_unlock(real_map);
					}

					kr = KERN_MEMORY_ERROR;
					goto done;
				}
				if (cur_object != object) {
					vm_object_unlock(cur_object);

					cur_object = object;
				}
				if (object_lock_type == OBJECT_LOCK_SHARED) {
					object_lock_type = OBJECT_LOCK_EXCLUSIVE;

					if (vm_object_lock_upgrade(object) == FALSE) {
						  
                                                         
                                               
                                             
                              
         
						vm_map_unlock_read(map);
						if (real_map != map) {
							vm_map_unlock(real_map);
						}

						goto RetryFault;
					}
				}
				if (!object->internal) {
					panic("%s:%d should not zero-fill page at offset 0x%llx in external object %p", __FUNCTION__, __LINE__, (uint64_t)offset, object);
				}
				m = vm_page_alloc(object, vm_object_trunc_page(offset));
				m_object = NULL;

				if (m == VM_PAGE_NULL) {
					  
                                           
                               
        
					break;
				}
				m_object = object;

				  
                                                          
                                                                             
      
                                                                                         
                                                           
                                                                             
                                                                              
                                                                       
                                                     
       
				vm_fault_cs_clear(m);
				m->vmp_pmapped = TRUE;
				if (map->no_zero_fill) {
					type_of_fault = DBG_NZF_PAGE_FAULT;
				} else {
					type_of_fault = DBG_ZERO_FILL_FAULT;
				}
				{
					pmap_t destination_pmap;
					vm_map_offset_t destination_pmap_vaddr;
					vm_prot_t enter_fault_type;
					if (caller_pmap) {
						destination_pmap = caller_pmap;
						destination_pmap_vaddr = caller_pmap_addr;
					} else {
						destination_pmap = pmap;
						destination_pmap_vaddr = vaddr;
					}
					if (change_wiring) {
						enter_fault_type = VM_PROT_NONE;
					} else {
						enter_fault_type = caller_prot;
					}
					kr = vm_fault_enter_prepare(m,
					    destination_pmap,
					    destination_pmap_vaddr,
					    &prot,
					    caller_prot,
					    fault_page_size,
					    fault_phys_offset,
					    change_wiring,
					    enter_fault_type,
					    &fault_info,
					    &type_of_fault,
					    &page_needs_data_sync);
					if (kr != KERN_SUCCESS) {
						goto zero_fill_cleanup;
					}

					if (object_is_contended) {
						  
                                                                               
                                                                             
                                                                              
         
						m->vmp_busy = TRUE;
						vm_object_unlock(object);
					}
					if (type_of_fault == DBG_ZERO_FILL_FAULT) {
						  
                              
                                      
                                         
                                  
        
                                        
                                     
         
						vm_page_zero_fill(m);
						counter_inc(&vm_statistics_zero_fill_count);
						DTRACE_VM2(zfod, int, 1, (uint64_t *), NULL);
					}
					if (page_needs_data_sync) {
						pmap_sync_page_data_phys(VM_PAGE_GET_PHYS_PAGE(m));
					}

					if (top_object != VM_OBJECT_NULL) {
						need_retry_ptr = &need_retry;
					} else {
						need_retry_ptr = NULL;
					}
					if (object_is_contended) {
						kr = vm_fault_pmap_enter(destination_pmap, destination_pmap_vaddr,
						    fault_page_size, fault_phys_offset,
						    m, &prot, caller_prot, enter_fault_type, wired,
						    fault_info.pmap_options, need_retry_ptr);
						vm_object_lock(object);
					} else {
						kr = vm_fault_pmap_enter_with_object_lock(object, destination_pmap, destination_pmap_vaddr,
						    fault_page_size, fault_phys_offset,
						    m, &prot, caller_prot, enter_fault_type, wired,
						    fault_info.pmap_options, need_retry_ptr);
					}
				}
zero_fill_cleanup:
				if (!VM_DYNAMIC_PAGING_ENABLED() &&
				    (object->purgable == VM_PURGABLE_DENY ||
				    object->purgable == VM_PURGABLE_NONVOLATILE ||
				    object->purgable == VM_PURGABLE_VOLATILE)) {
					vm_page_lockspin_queues();
					if (!VM_DYNAMIC_PAGING_ENABLED()) {
						vm_fault_enqueue_throttled_locked(m);
					}
					vm_page_unlock_queues();
				}
				vm_fault_enqueue_page(object, m, wired, change_wiring, wire_tag, fault_info.no_cache, &type_of_fault, kr);

				vm_fault_complete(
					map,
					real_map,
					object,
					m_object,
					m,
					offset,
					trace_real_vaddr,
					&fault_info,
					caller_prot,
					real_vaddr,
					type_of_fault,
					need_retry,
					kr,
					physpage_p,
					prot,
					top_object,
					need_collapse,
					cur_offset,
					fault_type,
					&written_on_object,
					&written_on_pager,
					&written_on_offset);
				top_object = VM_OBJECT_NULL;
				if (need_retry == TRUE) {
					  
                                                          
                                                          
                                                         
                                                        
                                                                
                                                                     
        
					(void)pmap_enter_options(
						pmap, vaddr, 0, 0, 0, 0, 0,
						PMAP_OPTIONS_NOENTER, NULL);

					need_retry = FALSE;
					goto RetryFault;
				}
				goto done;
			}
			  
                                              
      
			cur_offset += cur_object->vo_shadow_offset;
			new_object = cur_object->shadow;
			fault_phys_offset = cur_offset - vm_object_trunc_page(cur_offset);

			  
                                                         
      
			if (cur_object_lock_type == OBJECT_LOCK_SHARED) {
				vm_object_lock_shared(new_object);
			} else {
				vm_object_lock(new_object);
			}

			if (cur_object != object) {
				vm_object_unlock(cur_object);
			}

			cur_object = new_object;

			continue;
		}
	}
	  
                                                     
                                               
    
	if (object != cur_object) {
		vm_object_unlock(cur_object);
	}

	  
                                                      
    
	if (object_lock_type == OBJECT_LOCK_SHARED) {
		object_lock_type = OBJECT_LOCK_EXCLUSIVE;

		if (vm_object_lock_upgrade(object) == FALSE) {
			  
                                    
                               
                                        
                                      
                                      
      
			vm_object_lock(object);
		}
	}

handle_copy_delay:
	vm_map_unlock_read(map);
	if (real_map != map) {
		vm_map_unlock(real_map);
	}

	if (__improbable(object == compressor_object ||
	    object == kernel_object ||
	    object == vm_submap_object)) {
		  
                                                              
                                                               
                                                                
                                                            
                                                                
                                                                
                                                             
                             
     
		vm_object_unlock(object);
		kr = KERN_MEMORY_ERROR;
		goto done;
	}

	assert(object != compressor_object);
	assert(object != kernel_object);
	assert(object != vm_submap_object);

	if (resilient_media_retry) {
		  
                                                      
                                                      
                                                  
     
		assert(resilient_media_object != VM_OBJECT_NULL);
		assert(resilient_media_offset != (vm_object_offset_t)-1);
		                                                  
                                                                                                                           
		vm_object_deallocate(resilient_media_object);
		resilient_media_object = VM_OBJECT_NULL;
		resilient_media_offset = (vm_object_offset_t)-1;
		resilient_media_retry = FALSE;
	}

	  
                                      
                                                  
                                                    
                                                 
                                                        
    
	vm_object_reference_locked(object);
	vm_object_paging_begin(object);

	set_thread_pagein_error(cthread, 0);
	error_code = 0;

	result_page = VM_PAGE_NULL;
	kr = vm_fault_page(object, offset, fault_type,
	    (change_wiring && !wired),
	    FALSE,                                        
	    &prot, &result_page, &top_page,
	    &type_of_fault,
	    &error_code, map->no_zero_fill,
	    FALSE, &fault_info);

	  
                                                        
                                                             
                 
   
                                                        
                                                                 
   
                                                         
   
                                               
                                               
                            
    
	if (kr != VM_FAULT_SUCCESS &&
	    kr != VM_FAULT_SUCCESS_NO_VM_PAGE) {
		if (kr == VM_FAULT_MEMORY_ERROR &&
		    fault_info.resilient_media) {
			assertf(object->internal, "object %p", object);
			  
                                           
                                                    
                                                    
                 
                                                 
                                                 
                                             
      
                                                                                                                                                                                                                                  
			assert(!resilient_media_retry);                      
			assert(resilient_media_object == VM_OBJECT_NULL);
			assert(resilient_media_offset == (vm_object_offset_t)-1);
			resilient_media_retry = TRUE;
			resilient_media_object = object;
			resilient_media_offset = offset;
                                                                                                                                                                              
			goto RetryFault;
		} else {
			  
                                                  
                  
      
			vm_object_deallocate(object);
			object = VM_OBJECT_NULL;                      
		}

		  
                                                   
     
		switch (kr) {
		case VM_FAULT_MEMORY_SHORTAGE:
			if (vm_page_wait((change_wiring) ?
			    THREAD_UNINT :
			    THREAD_ABORTSAFE)) {
				goto RetryFault;
			}
			OS_FALLTHROUGH;
		case VM_FAULT_INTERRUPTED:
			kr = KERN_ABORTED;
			goto done;
		case VM_FAULT_RETRY:
			goto RetryFault;
		case VM_FAULT_MEMORY_ERROR:
			if (error_code) {
				kr = error_code;
			} else {
				kr = KERN_MEMORY_ERROR;
			}
			goto done;
		default:
			panic("vm_fault: unexpected error 0x%x from "
			    "vm_fault_page()\n", kr);
		}
	}
	m = result_page;
	m_object = NULL;

	if (m != VM_PAGE_NULL) {
		m_object = VM_PAGE_OBJECT(m);
		assert((change_wiring && !wired) ?
		    (top_page == VM_PAGE_NULL) :
		    ((top_page == VM_PAGE_NULL) == (m_object == object)));
	}

	  
                                                         
                                                    
    
#define RELEASE_PAGE(m)                                 \
	MACRO_BEGIN                                     \
	PAGE_WAKEUP_DONE(m);                            \
	if ( !VM_PAGE_PAGEABLE(m)) {                    \
	        vm_page_lockspin_queues();              \
	        if ( !VM_PAGE_PAGEABLE(m))              \
	                vm_page_activate(m);            \
	        vm_page_unlock_queues();                \
	}                                               \
	MACRO_END


	object_locks_dropped = FALSE;
	  
                                                 
                                                    
                                                      
                                                      
                                                           
    
	if (!vm_map_try_lock_read(original_map)) {
		if (m != VM_PAGE_NULL) {
			old_copy_object = m_object->copy;
			vm_object_unlock(m_object);
		} else {
			old_copy_object = VM_OBJECT_NULL;
			vm_object_unlock(object);
		}

		object_locks_dropped = TRUE;

		vm_map_lock_read(original_map);
	}

	if ((map != original_map) || !vm_map_verify(map, &version)) {
		if (object_locks_dropped == FALSE) {
			if (m != VM_PAGE_NULL) {
				old_copy_object = m_object->copy;
				vm_object_unlock(m_object);
			} else {
				old_copy_object = VM_OBJECT_NULL;
				vm_object_unlock(object);
			}

			object_locks_dropped = TRUE;
		}

		  
                                           
     
		vm_object_t             retry_object;
		vm_object_offset_t      retry_offset;
		vm_prot_t               retry_prot;

		  
                                                        
                                                       
                                                     
                                                         
                                                        
                                                      
                        
     
		map = original_map;

		kr = vm_map_lookup_locked(&map, vaddr,
		    fault_type & ~VM_PROT_WRITE,
		    OBJECT_LOCK_EXCLUSIVE, &version,
		    &retry_object, &retry_offset, &retry_prot,
		    &wired,
		    &fault_info,
		    &real_map,
		    NULL);
		pmap = real_map->pmap;

		if (kr != KERN_SUCCESS) {
			vm_map_unlock_read(map);

			if (m != VM_PAGE_NULL) {
				assert(VM_PAGE_OBJECT(m) == m_object);

				  
                              
                                       
                                     
                                       
       
				vm_object_lock(m_object);

				RELEASE_PAGE(m);

				vm_fault_cleanup(m_object, top_page);
			} else {
				  
                              
                                       
                          
       
				vm_object_lock(object);

				vm_fault_cleanup(object, top_page);
			}
			vm_object_deallocate(object);

			goto done;
		}
		vm_object_unlock(retry_object);

		if ((retry_object != object) || (retry_offset != offset)) {
			vm_map_unlock_read(map);
			if (real_map != map) {
				vm_map_unlock(real_map);
			}

			if (m != VM_PAGE_NULL) {
				assert(VM_PAGE_OBJECT(m) == m_object);

				  
                              
                                       
                                     
                                       
       
				vm_object_lock(m_object);

				RELEASE_PAGE(m);

				vm_fault_cleanup(m_object, top_page);
			} else {
				  
                              
                                       
                          
       
				vm_object_lock(object);

				vm_fault_cleanup(object, top_page);
			}
			vm_object_deallocate(object);

			goto RetryFault;
		}
		  
                                                           
                                                    
     
		if (pmap_has_prot_policy(pmap, fault_info.pmap_options & PMAP_OPTIONS_TRANSLATED_ALLOW_EXECUTE, retry_prot)) {
			                                                 
			prot = retry_prot;
		} else {
			prot &= retry_prot;
		}
	}

	if (object_locks_dropped == TRUE) {
		if (m != VM_PAGE_NULL) {
			vm_object_lock(m_object);

			if (m_object->copy != old_copy_object) {
				  
                                                         
                                                   
       
				assert(!pmap_has_prot_policy(pmap, fault_info.pmap_options & PMAP_OPTIONS_TRANSLATED_ALLOW_EXECUTE, prot));
				prot &= ~VM_PROT_WRITE;
			}
		} else {
			vm_object_lock(object);
		}

		object_locks_dropped = FALSE;
	}

	if (!need_copy &&
	    !fault_info.no_copy_on_read &&
	    m != VM_PAGE_NULL &&
	    VM_PAGE_OBJECT(m) != object &&
	    !VM_PAGE_OBJECT(m)->pager_trusted &&
	    vm_protect_privileged_from_untrusted &&
	    !VM_PAGE_OBJECT(m)->code_signed &&
	    current_proc_is_privileged()) {
		  
                                                          
                                                             
                                                                
                                                          
                                                       
                                                            
                       
    
                                                                 
                                                               
                                                              
                                                             
                                                             
                                                                  
     
                                                                                                                                                                                                                                                   
		vm_copied_on_read++;
		need_copy_on_read = TRUE;
		need_copy = TRUE;
	} else {
		need_copy_on_read = FALSE;
	}

	  
                                                         
                                                 
                                                               
    
	if ((wired && (fault_type != (prot | VM_PROT_WRITE))) ||
	    need_copy_on_read) {
		vm_map_unlock_read(map);
		if (real_map != map) {
			vm_map_unlock(real_map);
		}

		if (m != VM_PAGE_NULL) {
			assert(VM_PAGE_OBJECT(m) == m_object);

			RELEASE_PAGE(m);

			vm_fault_cleanup(m_object, top_page);
		} else {
			vm_fault_cleanup(object, top_page);
		}

		vm_object_deallocate(object);

		goto RetryFault;
	}
	if (m != VM_PAGE_NULL) {
		  
                                         
                                                     
                                                
                                                     
                                                        
     
		if (fault_page_size < PAGE_SIZE) {
			DEBUG4K_FAULT("map %p original %p pmap %p va 0x%llx pa 0x%llx(0x%llx+0x%llx) prot 0x%x caller_prot 0x%x\n", map, original_map, pmap, (uint64_t)vaddr, (uint64_t)((((pmap_paddr_t)VM_PAGE_GET_PHYS_PAGE(m)) << PAGE_SHIFT) + fault_phys_offset), (uint64_t)(((pmap_paddr_t)VM_PAGE_GET_PHYS_PAGE(m)) << PAGE_SHIFT), (uint64_t)fault_phys_offset, prot, caller_prot);
			assertf((!(fault_phys_offset & FOURK_PAGE_MASK) &&
			    fault_phys_offset < PAGE_SIZE),
			    "0x%llx\n", (uint64_t)fault_phys_offset);
		} else {
			assertf(fault_phys_offset == 0,
			    "0x%llx\n", (uint64_t)fault_phys_offset);
		}
		if (caller_pmap) {
			kr = vm_fault_enter(m,
			    caller_pmap,
			    caller_pmap_addr,
			    fault_page_size,
			    fault_phys_offset,
			    prot,
			    caller_prot,
			    wired,
			    change_wiring,
			    wire_tag,
			    &fault_info,
			    NULL,
			    &type_of_fault);
		} else {
			kr = vm_fault_enter(m,
			    pmap,
			    vaddr,
			    fault_page_size,
			    fault_phys_offset,
			    prot,
			    caller_prot,
			    wired,
			    change_wiring,
			    wire_tag,
			    &fault_info,
			    NULL,
			    &type_of_fault);
		}
		assert(VM_PAGE_OBJECT(m) == m_object);

		{
			int     event_code = 0;

			if (m_object->internal) {
				event_code = (MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_REAL_FAULT_ADDR_INTERNAL));
			} else if (m_object->object_is_shared_cache) {
				event_code = (MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_REAL_FAULT_ADDR_SHAREDCACHE));
			} else {
				event_code = (MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_REAL_FAULT_ADDR_EXTERNAL));
			}

			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, event_code, trace_real_vaddr, (fault_info.user_tag << 16) | (caller_prot << 8) | vm_fault_type_for_tracing(need_copy_on_read, type_of_fault), m->vmp_offset, get_current_unique_pid(), 0);
			KDBG_FILTERED(MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_REAL_FAULT_SLOW), get_current_unique_pid(), 0, 0, 0, 0);

			DTRACE_VM6(real_fault, vm_map_offset_t, real_vaddr, vm_map_offset_t, m->vmp_offset, int, event_code, int, caller_prot, int, type_of_fault, int, fault_info.user_tag);
		}
		if (kr != KERN_SUCCESS) {
			                           
			vm_map_unlock_read(map);
			if (real_map != map) {
				vm_map_unlock(real_map);
			}
			PAGE_WAKEUP_DONE(m);
			vm_fault_cleanup(m_object, top_page);
			vm_object_deallocate(object);
			goto done;
		}
		if (physpage_p != NULL) {
			                                   
			*physpage_p = VM_PAGE_GET_PHYS_PAGE(m);
			if (prot & VM_PROT_WRITE) {
				vm_object_lock_assert_exclusive(m_object);
				m->vmp_dirty = TRUE;
			}
		}
	} else {
		vm_map_entry_t          entry;
		vm_map_offset_t         laddr;
		vm_map_offset_t         ldelta, hdelta;

		  
                                                      
                  
     

		if (real_map != map) {
			vm_map_unlock(real_map);
		}

		if (original_map != map) {
			vm_map_unlock_read(map);
			vm_map_lock_read(original_map);
			map = original_map;
		}
		real_map = map;

		laddr = vaddr;
		hdelta = 0xFFFFF000;
		ldelta = 0xFFFFF000;

		while (vm_map_lookup_entry(map, laddr, &entry)) {
			if (ldelta > (laddr - entry->vme_start)) {
				ldelta = laddr - entry->vme_start;
			}
			if (hdelta > (entry->vme_end - laddr)) {
				hdelta = entry->vme_end - laddr;
			}
			if (entry->is_sub_map) {
				laddr = ((laddr - entry->vme_start)
				    + VME_OFFSET(entry));
				vm_map_lock_read(VME_SUBMAP(entry));

				if (map != real_map) {
					vm_map_unlock_read(map);
				}
				if (entry->use_pmap) {
					vm_map_unlock_read(real_map);
					real_map = VME_SUBMAP(entry);
				}
				map = VME_SUBMAP(entry);
			} else {
				break;
			}
		}

		if (vm_map_lookup_entry(map, laddr, &entry) &&
		    (VME_OBJECT(entry) != NULL) &&
		    (VME_OBJECT(entry) == object)) {
			uint16_t superpage;

			if (!object->pager_created &&
			    object->phys_contiguous &&
			    VME_OFFSET(entry) == 0 &&
			    (entry->vme_end - entry->vme_start == object->vo_size) &&
			    VM_MAP_PAGE_ALIGNED(entry->vme_start, (object->vo_size - 1))) {
				superpage = VM_MEM_SUPERPAGE;
			} else {
				superpage = 0;
			}

			if (superpage && physpage_p) {
				                                   
				*physpage_p = (ppnum_t)
				    ((((vm_map_offset_t)
				    object->vo_shadow_offset)
				    + VME_OFFSET(entry)
				    + (laddr - entry->vme_start))
				    >> PAGE_SHIFT);
			}

			if (caller_pmap) {
				  
                                 
       
				assert((uint32_t)((ldelta + hdelta) >> fault_page_shift) == ((ldelta + hdelta) >> fault_page_shift));
				kr = pmap_map_block_addr(caller_pmap,
				    (addr64_t)(caller_pmap_addr - ldelta),
				    (pmap_paddr_t)(((vm_map_offset_t) (VME_OBJECT(entry)->vo_shadow_offset)) +
				    VME_OFFSET(entry) + (laddr - entry->vme_start) - ldelta),
				    (uint32_t)((ldelta + hdelta) >> fault_page_shift), prot,
				    (VM_WIMG_MASK & (int)object->wimg_bits) | superpage, 0);

				if (kr != KERN_SUCCESS) {
					goto cleanup;
				}
			} else {
				  
                                 
       
				assert((uint32_t)((ldelta + hdelta) >> fault_page_shift) == ((ldelta + hdelta) >> fault_page_shift));
				kr = pmap_map_block_addr(real_map->pmap,
				    (addr64_t)(vaddr - ldelta),
				    (pmap_paddr_t)(((vm_map_offset_t)(VME_OBJECT(entry)->vo_shadow_offset)) +
				    VME_OFFSET(entry) + (laddr - entry->vme_start) - ldelta),
				    (uint32_t)((ldelta + hdelta) >> fault_page_shift), prot,
				    (VM_WIMG_MASK & (int)object->wimg_bits) | superpage, 0);

				if (kr != KERN_SUCCESS) {
					goto cleanup;
				}
			}
		}
	}

	  
           
    
	kr = KERN_SUCCESS;

	  
                                                        
    
cleanup:
	  
                                 
    
	vm_map_unlock_read(map);
	if (real_map != map) {
		vm_map_unlock(real_map);
	}

	if (m != VM_PAGE_NULL) {
		assert(VM_PAGE_OBJECT(m) == m_object);

		if (!m_object->internal && (fault_type & VM_PROT_WRITE)) {
			vm_object_paging_begin(m_object);

			assert(written_on_object == VM_OBJECT_NULL);
			written_on_object = m_object;
			written_on_pager = m_object->pager;
			written_on_offset = m_object->paging_offset + m->vmp_offset;
		}
		PAGE_WAKEUP_DONE(m);

		vm_fault_cleanup(m_object, top_page);
	} else {
		vm_fault_cleanup(object, top_page);
	}

	vm_object_deallocate(object);

#undef  RELEASE_PAGE

done:
	thread_interrupt_level(interruptible_state);

	if (resilient_media_object != VM_OBJECT_NULL) {
		assert(resilient_media_retry);
		assert(resilient_media_offset != (vm_object_offset_t)-1);
		                                              
                                                                                                                           
		vm_object_deallocate(resilient_media_object);
		resilient_media_object = VM_OBJECT_NULL;
		resilient_media_offset = (vm_object_offset_t)-1;
		resilient_media_retry = FALSE;
	}
	assert(!resilient_media_retry);

	  
                                                            
    
	if ((type_of_fault == DBG_PAGEIND_FAULT) || (type_of_fault == DBG_PAGEINV_FAULT) || (type_of_fault == DBG_COMPRESSOR_SWAPIN_FAULT)) {
		throttle_lowpri_io(1);
	} else {
		if (kr == KERN_SUCCESS && type_of_fault != DBG_CACHE_HIT_FAULT && type_of_fault != DBG_GUARD_FAULT) {
			if ((throttle_delay = vm_page_throttled(TRUE))) {
				if (vm_debug_events) {
					if (type_of_fault == DBG_COMPRESSOR_FAULT) {
						VM_DEBUG_EVENT(vmf_compressordelay, VMF_COMPRESSORDELAY, DBG_FUNC_NONE, throttle_delay, 0, 0, 0);
					} else if (type_of_fault == DBG_COW_FAULT) {
						VM_DEBUG_EVENT(vmf_cowdelay, VMF_COWDELAY, DBG_FUNC_NONE, throttle_delay, 0, 0, 0);
					} else {
						VM_DEBUG_EVENT(vmf_zfdelay, VMF_ZFDELAY, DBG_FUNC_NONE, throttle_delay, 0, 0, 0);
					}
				}
				delay(throttle_delay);
			}
		}
	}

	if (written_on_object) {
		vnode_pager_dirtied(written_on_pager, written_on_offset, written_on_offset + PAGE_SIZE_64);

		vm_object_lock(written_on_object);
		vm_object_paging_end(written_on_object);
		vm_object_unlock(written_on_object);

		written_on_object = VM_OBJECT_NULL;
	}

	if (rtfault) {
		vm_record_rtfault(cthread, fstart, trace_vaddr, type_of_fault);
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (MACHDBG_CODE(DBG_MACH_VM, 2)) | DBG_FUNC_END,
	    ((uint64_t)trace_vaddr >> 32),
	    trace_vaddr,
	    kr,
	    vm_fault_type_for_tracing(need_copy_on_read, type_of_fault),
	    0);

	if (fault_page_size < PAGE_SIZE && kr != KERN_SUCCESS) {
		DEBUG4K_FAULT("map %p original %p vaddr 0x%llx -> 0x%x\n", map, original_map, (uint64_t)trace_real_vaddr, kr);
	}

	return kr;
}

  
                 
  
                                                   
   
kern_return_t
vm_fault_wire(
	vm_map_t        map,
	vm_map_entry_t  entry,
	vm_prot_t       prot,
	vm_tag_t        wire_tag,
	pmap_t          pmap,
	vm_map_offset_t pmap_addr,
	ppnum_t         *physpage_p)
{
	vm_map_offset_t va;
	vm_map_offset_t end_addr = entry->vme_end;
	kern_return_t   rc;
	vm_map_size_t   effective_page_size;

	assert(entry->in_transition);

	if ((VME_OBJECT(entry) != NULL) &&
	    !entry->is_sub_map &&
	    VME_OBJECT(entry)->phys_contiguous) {
		return KERN_SUCCESS;
	}

	  
                                               
                                             
                                                    
    

	pmap_pageable(pmap, pmap_addr,
	    pmap_addr + (end_addr - entry->vme_start), FALSE);

	  
                                                    
                        
    

	effective_page_size = MIN(VM_MAP_PAGE_SIZE(map), PAGE_SIZE);
	for (va = entry->vme_start;
	    va < end_addr;
	    va += effective_page_size) {
		rc = vm_fault_wire_fast(map, va, prot, wire_tag, entry, pmap,
		    pmap_addr + (va - entry->vme_start),
		    physpage_p);
		if (rc != KERN_SUCCESS) {
			rc = vm_fault_internal(map, va, prot, TRUE, wire_tag,
			    ((pmap == kernel_pmap)
			    ? THREAD_UNINT
			    : THREAD_ABORTSAFE),
			    pmap,
			    (pmap_addr +
			    (va - entry->vme_start)),
			    physpage_p);
			DTRACE_VM2(softlock, int, 1, (uint64_t *), NULL);
		}

		if (rc != KERN_SUCCESS) {
			struct vm_map_entry     tmp_entry = *entry;

			                        
			tmp_entry.vme_end = va;
			vm_fault_unwire(map,
			    &tmp_entry, FALSE, pmap, pmap_addr);

			return rc;
		}
	}
	return KERN_SUCCESS;
}

  
                   
  
                                                
   
void
vm_fault_unwire(
	vm_map_t        map,
	vm_map_entry_t  entry,
	boolean_t       deallocate,
	pmap_t          pmap,
	vm_map_offset_t pmap_addr)
{
	vm_map_offset_t va;
	vm_map_offset_t end_addr = entry->vme_end;
	vm_object_t             object;
	struct vm_object_fault_info fault_info = {};
	unsigned int    unwired_pages;
	vm_map_size_t   effective_page_size;

	object = (entry->is_sub_map) ? VM_OBJECT_NULL : VME_OBJECT(entry);

	  
                                                                        
                                                                        
                          
    

	if (object != VM_OBJECT_NULL && object->phys_contiguous) {
		return;
	}

	fault_info.interruptible = THREAD_UNINT;
	fault_info.behavior = entry->behavior;
	fault_info.user_tag = VME_ALIAS(entry);
	if (entry->iokit_acct ||
	    (!entry->is_sub_map && !entry->use_pmap)) {
		fault_info.pmap_options |= PMAP_OPTIONS_ALT_ACCT;
	}
	fault_info.lo_offset = VME_OFFSET(entry);
	fault_info.hi_offset = (entry->vme_end - entry->vme_start) + VME_OFFSET(entry);
	fault_info.no_cache = entry->no_cache;
	fault_info.stealth = TRUE;

	unwired_pages = 0;

	  
                                                      
                                                    
    

	effective_page_size = MIN(VM_MAP_PAGE_SIZE(map), PAGE_SIZE);
	for (va = entry->vme_start;
	    va < end_addr;
	    va += effective_page_size) {
		if (object == VM_OBJECT_NULL) {
			if (pmap) {
				pmap_change_wiring(pmap,
				    pmap_addr + (va - entry->vme_start), FALSE);
			}
			(void) vm_fault(map, va, VM_PROT_NONE,
			    TRUE, VM_KERN_MEMORY_NONE, THREAD_UNINT, pmap, pmap_addr);
		} else {
			vm_prot_t       prot;
			vm_page_t       result_page;
			vm_page_t       top_page;
			vm_object_t     result_object;
			vm_fault_return_t result;

			                                          
			upl_size_t cluster_size;
			if (os_sub_overflow(end_addr, va, &cluster_size)) {
				cluster_size = 0 - (upl_size_t)PAGE_SIZE;
			}
			fault_info.cluster_size = cluster_size;

			do {
				prot = VM_PROT_NONE;

				vm_object_lock(object);
				vm_object_paging_begin(object);
				result_page = VM_PAGE_NULL;
				result = vm_fault_page(
					object,
					(VME_OFFSET(entry) +
					(va - entry->vme_start)),
					VM_PROT_NONE, TRUE,
					FALSE,                         
					&prot, &result_page, &top_page,
					(int *)0,
					NULL, map->no_zero_fill,
					FALSE, &fault_info);
			} while (result == VM_FAULT_RETRY);

			  
                                                                        
                                                                          
                                                                         
                                                                           
                                                                             
                                                                           
                                                         
      

			if (result == VM_FAULT_MEMORY_ERROR && !object->alive) {
				continue;
			}

			if (result == VM_FAULT_MEMORY_ERROR &&
			    object == kernel_object) {
				  
                                         
                                             
                                       
                                    
       
				assert(deallocate);
				continue;
			}

			if (result != VM_FAULT_SUCCESS) {
				panic("vm_fault_unwire: failure");
			}

			result_object = VM_PAGE_OBJECT(result_page);

			if (deallocate) {
				assert(VM_PAGE_GET_PHYS_PAGE(result_page) !=
				    vm_page_fictitious_addr);
				pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(result_page));
				if (VM_PAGE_WIRED(result_page)) {
					unwired_pages++;
				}
				VM_PAGE_FREE(result_page);
			} else {
				if ((pmap) && (VM_PAGE_GET_PHYS_PAGE(result_page) != vm_page_guard_addr)) {
					pmap_change_wiring(pmap,
					    pmap_addr + (va - entry->vme_start), FALSE);
				}


				if (VM_PAGE_WIRED(result_page)) {
					vm_page_lockspin_queues();
					vm_page_unwire(result_page, TRUE);
					vm_page_unlock_queues();
					unwired_pages++;
				}
				if (entry->zero_wired_pages) {
					pmap_zero_page(VM_PAGE_GET_PHYS_PAGE(result_page));
					entry->zero_wired_pages = FALSE;
				}

				PAGE_WAKEUP_DONE(result_page);
			}
			vm_fault_cleanup(result_object, top_page);
		}
	}

	  
                                                     
                                                   
                                   
    

	pmap_pageable(pmap, pmap_addr,
	    pmap_addr + (end_addr - entry->vme_start), TRUE);

	if (kernel_object == object) {
		  
                                                        
                                                                  
                                                                   
                                                                              
                         
    
                                                                      
     
		assertf((fault_info.user_tag & VME_ALIAS_MASK) == fault_info.user_tag,
		    "VM Tag truncated from 0x%x to 0x%x\n", fault_info.user_tag, (fault_info.user_tag & VME_ALIAS_MASK));
		vm_tag_update_size((vm_tag_t) fault_info.user_tag, -ptoa_64(unwired_pages));
	}
}

  
                      
  
                                                                     
                                                                        
                                                                    
  
                                                     
                       
  
                                                                    
                                                                  
  
                                                          
                                     
  
                                                                       
                                                                      
                                  
   
static kern_return_t
vm_fault_wire_fast(
	__unused vm_map_t       map,
	vm_map_offset_t va,
	__unused vm_prot_t       caller_prot,
	vm_tag_t        wire_tag,
	vm_map_entry_t  entry,
	pmap_t          pmap,
	vm_map_offset_t pmap_addr,
	ppnum_t         *physpage_p)
{
	vm_object_t             object;
	vm_object_offset_t      offset;
	vm_page_t               m;
	vm_prot_t               prot;
	thread_t                thread = current_thread();
	int                     type_of_fault;
	kern_return_t           kr;
	vm_map_size_t           fault_page_size;
	vm_map_offset_t         fault_phys_offset;
	struct vm_object_fault_info fault_info = {};

	counter_inc(&vm_statistics_faults);

	if (thread != THREAD_NULL && thread->task != TASK_NULL) {
		counter_inc(&thread->task->faults);
	}

  
                   
   

#undef  RELEASE_PAGE
#define RELEASE_PAGE(m) {                               \
	PAGE_WAKEUP_DONE(m);                            \
	vm_page_lockspin_queues();                      \
	vm_page_unwire(m, TRUE);                        \
	vm_page_unlock_queues();                        \
}


#undef  UNLOCK_THINGS
#define UNLOCK_THINGS   {                               \
	vm_object_paging_end(object);                      \
	vm_object_unlock(object);                          \
}

#undef  UNLOCK_AND_DEALLOCATE
#define UNLOCK_AND_DEALLOCATE   {                       \
	UNLOCK_THINGS;                                  \
	vm_object_deallocate(object);                   \
}
  
                                                  
   

#define GIVE_UP {                                       \
	UNLOCK_AND_DEALLOCATE;                          \
	return(KERN_FAILURE);                           \
}


	  
                                                           
    
	if (entry->is_sub_map) {
		assert(physpage_p == NULL);
		return KERN_FAILURE;
	}

	  
                                                     
    

	object = VME_OBJECT(entry);
	offset = (va - entry->vme_start) + VME_OFFSET(entry);
	prot = entry->protection;

	  
                                                  
                                          
    

	vm_object_lock(object);
	vm_object_reference_locked(object);
	vm_object_paging_begin(object);

	  
                                        
   
                                                   
                                                  
                                              
                   
   
                                                       
                                                   
                           
   
    

	  
                                                            
                                        
    
	m = vm_page_lookup(object, vm_object_trunc_page(offset));
	if ((m == VM_PAGE_NULL) || (m->vmp_busy) ||
	    (m->vmp_unusual && (m->vmp_error || m->vmp_restart || m->vmp_absent))) {
		GIVE_UP;
	}
	if (m->vmp_fictitious &&
	    VM_PAGE_GET_PHYS_PAGE(m) == vm_page_guard_addr) {
		  
                                                   
                                                         
     
		kr = KERN_SUCCESS;
		goto done;
	}

	  
                                                      
                               
    

	vm_page_lockspin_queues();
	vm_page_wire(m, wire_tag, TRUE);
	vm_page_unlock_queues();

	  
                                     
    
	assert(!m->vmp_busy);
	m->vmp_busy = TRUE;
	assert(!m->vmp_absent);

	  
                                                                  
    
	if ((object->copy != VM_OBJECT_NULL) && (prot & VM_PROT_WRITE)) {
		RELEASE_PAGE(m);
		GIVE_UP;
	}

	fault_info.user_tag = VME_ALIAS(entry);
	fault_info.pmap_options = 0;
	if (entry->iokit_acct ||
	    (!entry->is_sub_map && !entry->use_pmap)) {
		fault_info.pmap_options |= PMAP_OPTIONS_ALT_ACCT;
	}

	fault_page_size = MIN(VM_MAP_PAGE_SIZE(map), PAGE_SIZE);
	fault_phys_offset = offset - vm_object_trunc_page(offset);

	  
                                        
    
	type_of_fault = DBG_CACHE_HIT_FAULT;
	kr = vm_fault_enter(m,
	    pmap,
	    pmap_addr,
	    fault_page_size,
	    fault_phys_offset,
	    prot,
	    prot,
	    TRUE,                             
	    FALSE,                                    
	    wire_tag,
	    &fault_info,
	    NULL,
	    &type_of_fault);
	if (kr != KERN_SUCCESS) {
		RELEASE_PAGE(m);
		GIVE_UP;
	}

done:
	  
                                 
    

	if (physpage_p) {
		                                   
		if (kr == KERN_SUCCESS) {
			assert(object == VM_PAGE_OBJECT(m));
			*physpage_p = VM_PAGE_GET_PHYS_PAGE(m);
			if (prot & VM_PROT_WRITE) {
				vm_object_lock_assert_exclusive(object);
				m->vmp_dirty = TRUE;
			}
		} else {
			*physpage_p = 0;
		}
	}

	PAGE_WAKEUP_DONE(m);
	UNLOCK_AND_DEALLOCATE;

	return kr;
}

  
                                 
           
                                         
   

static void
vm_fault_copy_cleanup(
	vm_page_t       page,
	vm_page_t       top_page)
{
	vm_object_t     object = VM_PAGE_OBJECT(page);

	vm_object_lock(object);
	PAGE_WAKEUP_DONE(page);
	if (!VM_PAGE_PAGEABLE(page)) {
		vm_page_lockspin_queues();
		if (!VM_PAGE_PAGEABLE(page)) {
			vm_page_activate(page);
		}
		vm_page_unlock_queues();
	}
	vm_fault_cleanup(object, top_page);
}

static void
vm_fault_copy_dst_cleanup(
	vm_page_t       page)
{
	vm_object_t     object;

	if (page != VM_PAGE_NULL) {
		object = VM_PAGE_OBJECT(page);
		vm_object_lock(object);
		vm_page_lockspin_queues();
		vm_page_unwire(page, TRUE);
		vm_page_unlock_queues();
		vm_object_paging_end(object);
		vm_object_unlock(object);
	}
}

  
                         
  
           
                                                           
                                                              
  
                                                               
                                                
  
                     
                                                        
                                                         
                    
  
           
                                                         
                                                             
                                                       
                                                               
                                                   
  
                                                            
                                                                
                                                                
              
   
kern_return_t
vm_fault_copy(
	vm_object_t             src_object,
	vm_object_offset_t      src_offset,
	vm_map_size_t           *copy_size,                        
	vm_object_t             dst_object,
	vm_object_offset_t      dst_offset,
	vm_map_t                dst_map,
	vm_map_version_t         *dst_version,
	int                     interruptible)
{
	vm_page_t               result_page;

	vm_page_t               src_page;
	vm_page_t               src_top_page;
	vm_prot_t               src_prot;

	vm_page_t               dst_page;
	vm_page_t               dst_top_page;
	vm_prot_t               dst_prot;

	vm_map_size_t           amount_left;
	vm_object_t             old_copy_object;
	vm_object_t             result_page_object = NULL;
	kern_return_t           error = 0;
	vm_fault_return_t       result;

	vm_map_size_t           part_size;
	struct vm_object_fault_info fault_info_src = {};
	struct vm_object_fault_info fault_info_dst = {};

	  
                                                        
                                             
    

#define RETURN(x)                                       \
	MACRO_BEGIN                                     \
	*copy_size -= amount_left;                      \
	MACRO_RETURN(x);                                \
	MACRO_END

	amount_left = *copy_size;

	fault_info_src.interruptible = interruptible;
	fault_info_src.behavior = VM_BEHAVIOR_SEQUENTIAL;
	fault_info_src.lo_offset = vm_object_trunc_page(src_offset);
	fault_info_src.hi_offset = fault_info_src.lo_offset + amount_left;
	fault_info_src.stealth = TRUE;

	fault_info_dst.interruptible = interruptible;
	fault_info_dst.behavior = VM_BEHAVIOR_SEQUENTIAL;
	fault_info_dst.lo_offset = vm_object_trunc_page(dst_offset);
	fault_info_dst.hi_offset = fault_info_dst.lo_offset + amount_left;
	fault_info_dst.stealth = TRUE;

	do {                              
		  
                                                           
                                                              
                                                            
                          
     

RetryDestinationFault:;

		dst_prot = VM_PROT_WRITE | VM_PROT_READ;

		vm_object_lock(dst_object);
		vm_object_paging_begin(dst_object);

		                                          
		upl_size_t cluster_size;
		if (os_convert_overflow(amount_left, &cluster_size)) {
			cluster_size = 0 - (upl_size_t)PAGE_SIZE;
		}
		fault_info_dst.cluster_size = cluster_size;

		dst_page = VM_PAGE_NULL;
		result = vm_fault_page(dst_object,
		    vm_object_trunc_page(dst_offset),
		    VM_PROT_WRITE | VM_PROT_READ,
		    FALSE,
		    FALSE,                                            
		    &dst_prot, &dst_page, &dst_top_page,
		    (int *)0,
		    &error,
		    dst_map->no_zero_fill,
		    FALSE, &fault_info_dst);
		switch (result) {
		case VM_FAULT_SUCCESS:
			break;
		case VM_FAULT_RETRY:
			goto RetryDestinationFault;
		case VM_FAULT_MEMORY_SHORTAGE:
			if (vm_page_wait(interruptible)) {
				goto RetryDestinationFault;
			}
			OS_FALLTHROUGH;
		case VM_FAULT_INTERRUPTED:
			RETURN(MACH_SEND_INTERRUPTED);
		case VM_FAULT_SUCCESS_NO_VM_PAGE:
			                                           
			vm_object_paging_end(dst_object);
			vm_object_unlock(dst_object);
			OS_FALLTHROUGH;
		case VM_FAULT_MEMORY_ERROR:
			if (error) {
				return error;
			} else {
				return KERN_MEMORY_ERROR;
			}
		default:
			panic("vm_fault_copy: unexpected error 0x%x from "
			    "vm_fault_page()\n", result);
		}
		assert((dst_prot & VM_PROT_WRITE) != VM_PROT_NONE);

		assert(dst_object == VM_PAGE_OBJECT(dst_page));
		old_copy_object = dst_object->copy;

		  
                                                    
                                                 
                                                
                                              
                                                        
                                                     
                                                     
                                                          
                                                 
     

		vm_page_lockspin_queues();
		vm_page_wire(dst_page, VM_KERN_MEMORY_OSFMK, TRUE);
		vm_page_unlock_queues();
		PAGE_WAKEUP_DONE(dst_page);
		vm_object_unlock(dst_object);

		if (dst_top_page != VM_PAGE_NULL) {
			vm_object_lock(dst_object);
			VM_PAGE_FREE(dst_top_page);
			vm_object_paging_end(dst_object);
			vm_object_unlock(dst_object);
		}

RetrySourceFault:;

		if (src_object == VM_OBJECT_NULL) {
			  
                                     
                                       
      
			src_page = VM_PAGE_NULL;
			result_page = VM_PAGE_NULL;
		} else {
			vm_object_lock(src_object);
			src_page = vm_page_lookup(src_object,
			    vm_object_trunc_page(src_offset));
			if (src_page == dst_page) {
				src_prot = dst_prot;
				result_page = VM_PAGE_NULL;
			} else {
				src_prot = VM_PROT_READ;
				vm_object_paging_begin(src_object);

				                                          
				if (os_convert_overflow(amount_left, &cluster_size)) {
					cluster_size = 0 - (upl_size_t)PAGE_SIZE;
				}
				fault_info_src.cluster_size = cluster_size;

				result_page = VM_PAGE_NULL;
				result = vm_fault_page(
					src_object,
					vm_object_trunc_page(src_offset),
					VM_PROT_READ, FALSE,
					FALSE,                         
					&src_prot,
					&result_page, &src_top_page,
					(int *)0, &error, FALSE,
					FALSE, &fault_info_src);

				switch (result) {
				case VM_FAULT_SUCCESS:
					break;
				case VM_FAULT_RETRY:
					goto RetrySourceFault;
				case VM_FAULT_MEMORY_SHORTAGE:
					if (vm_page_wait(interruptible)) {
						goto RetrySourceFault;
					}
					OS_FALLTHROUGH;
				case VM_FAULT_INTERRUPTED:
					vm_fault_copy_dst_cleanup(dst_page);
					RETURN(MACH_SEND_INTERRUPTED);
				case VM_FAULT_SUCCESS_NO_VM_PAGE:
					                                  
					vm_object_paging_end(src_object);
					vm_object_unlock(src_object);
					OS_FALLTHROUGH;
				case VM_FAULT_MEMORY_ERROR:
					vm_fault_copy_dst_cleanup(dst_page);
					if (error) {
						return error;
					} else {
						return KERN_MEMORY_ERROR;
					}
				default:
					panic("vm_fault_copy(2): unexpected "
					    "error 0x%x from "
					    "vm_fault_page()\n", result);
				}

				result_page_object = VM_PAGE_OBJECT(result_page);
				assert((src_top_page == VM_PAGE_NULL) ==
				    (result_page_object == src_object));
			}
			assert((src_prot & VM_PROT_READ) != VM_PROT_NONE);
			vm_object_unlock(result_page_object);
		}

		vm_map_lock_read(dst_map);

		if (!vm_map_verify(dst_map, dst_version)) {
			vm_map_unlock_read(dst_map);
			if (result_page != VM_PAGE_NULL && src_page != dst_page) {
				vm_fault_copy_cleanup(result_page, src_top_page);
			}
			vm_fault_copy_dst_cleanup(dst_page);
			break;
		}
		assert(dst_object == VM_PAGE_OBJECT(dst_page));

		vm_object_lock(dst_object);

		if (dst_object->copy != old_copy_object) {
			vm_object_unlock(dst_object);
			vm_map_unlock_read(dst_map);
			if (result_page != VM_PAGE_NULL && src_page != dst_page) {
				vm_fault_copy_cleanup(result_page, src_top_page);
			}
			vm_fault_copy_dst_cleanup(dst_page);
			break;
		}
		vm_object_unlock(dst_object);

		  
                                             
                 
     

		if (!page_aligned(src_offset) ||
		    !page_aligned(dst_offset) ||
		    !page_aligned(amount_left)) {
			vm_object_offset_t      src_po,
			    dst_po;

			src_po = src_offset - vm_object_trunc_page(src_offset);
			dst_po = dst_offset - vm_object_trunc_page(dst_offset);

			if (dst_po > src_po) {
				part_size = PAGE_SIZE - dst_po;
			} else {
				part_size = PAGE_SIZE - src_po;
			}
			if (part_size > (amount_left)) {
				part_size = amount_left;
			}

			if (result_page == VM_PAGE_NULL) {
				assert((vm_offset_t) dst_po == dst_po);
				assert((vm_size_t) part_size == part_size);
				vm_page_part_zero_fill(dst_page,
				    (vm_offset_t) dst_po,
				    (vm_size_t) part_size);
			} else {
				assert((vm_offset_t) src_po == src_po);
				assert((vm_offset_t) dst_po == dst_po);
				assert((vm_size_t) part_size == part_size);
				vm_page_part_copy(result_page,
				    (vm_offset_t) src_po,
				    dst_page,
				    (vm_offset_t) dst_po,
				    (vm_size_t)part_size);
				if (!dst_page->vmp_dirty) {
					vm_object_lock(dst_object);
					SET_PAGE_DIRTY(dst_page, TRUE);
					vm_object_unlock(dst_object);
				}
			}
		} else {
			part_size = PAGE_SIZE;

			if (result_page == VM_PAGE_NULL) {
				vm_page_zero_fill(dst_page);
			} else {
				vm_object_lock(result_page_object);
				vm_page_copy(result_page, dst_page);
				vm_object_unlock(result_page_object);

				if (!dst_page->vmp_dirty) {
					vm_object_lock(dst_object);
					SET_PAGE_DIRTY(dst_page, TRUE);
					vm_object_unlock(dst_object);
				}
			}
		}

		  
                                  
     

		vm_map_unlock_read(dst_map);

		if (result_page != VM_PAGE_NULL && src_page != dst_page) {
			vm_fault_copy_cleanup(result_page, src_top_page);
		}
		vm_fault_copy_dst_cleanup(dst_page);

		amount_left -= part_size;
		src_offset += part_size;
		dst_offset += part_size;
	} while (amount_left > 0);

	RETURN(KERN_SUCCESS);
#undef  RETURN

	              
}

#if     VM_FAULT_CLASSIFY
  
                                          
   

  
                     
   
#define VM_FAULT_TYPES_MAX      5
#define VM_FAULT_LEVEL_MAX      8

int     vm_fault_stats[VM_FAULT_TYPES_MAX][VM_FAULT_LEVEL_MAX];

#define VM_FAULT_TYPE_ZERO_FILL 0
#define VM_FAULT_TYPE_MAP_IN    1
#define VM_FAULT_TYPE_PAGER     2
#define VM_FAULT_TYPE_COPY      3
#define VM_FAULT_TYPE_OTHER     4


void
vm_fault_classify(vm_object_t           object,
    vm_object_offset_t    offset,
    vm_prot_t             fault_type)
{
	int             type, level = 0;
	vm_page_t       m;

	while (TRUE) {
		m = vm_page_lookup(object, offset);
		if (m != VM_PAGE_NULL) {
			if (m->vmp_busy || m->vmp_error || m->vmp_restart || m->vmp_absent) {
				type = VM_FAULT_TYPE_OTHER;
				break;
			}
			if (((fault_type & VM_PROT_WRITE) == 0) ||
			    ((level == 0) && object->copy == VM_OBJECT_NULL)) {
				type = VM_FAULT_TYPE_MAP_IN;
				break;
			}
			type = VM_FAULT_TYPE_COPY;
			break;
		} else {
			if (object->pager_created) {
				type = VM_FAULT_TYPE_PAGER;
				break;
			}
			if (object->shadow == VM_OBJECT_NULL) {
				type = VM_FAULT_TYPE_ZERO_FILL;
				break;
			}

			offset += object->vo_shadow_offset;
			object = object->shadow;
			level++;
			continue;
		}
	}

	if (level > VM_FAULT_LEVEL_MAX) {
		level = VM_FAULT_LEVEL_MAX;
	}

	vm_fault_stats[type][level] += 1;

	return;
}

                                           

void
vm_fault_classify_init(void)
{
	int type, level;

	for (type = 0; type < VM_FAULT_TYPES_MAX; type++) {
		for (level = 0; level < VM_FAULT_LEVEL_MAX; level++) {
			vm_fault_stats[type][level] = 0;
		}
	}

	return;
}
#endif                         

vm_offset_t
kdp_lightweight_fault(vm_map_t map, vm_offset_t cur_target_addr)
{
	vm_map_entry_t  entry;
	vm_object_t     object;
	vm_offset_t     object_offset;
	vm_page_t       m;
	int             compressor_external_state, compressed_count_delta;
	int             compressor_flags = (C_DONT_BLOCK | C_KEEP | C_KDP);
	int             my_fault_type = VM_PROT_READ;
	kern_return_t   kr;
	int effective_page_mask, effective_page_size;

	if (VM_MAP_PAGE_SHIFT(map) < PAGE_SHIFT) {
		effective_page_mask = VM_MAP_PAGE_MASK(map);
		effective_page_size = VM_MAP_PAGE_SIZE(map);
	} else {
		effective_page_mask = PAGE_MASK;
		effective_page_size = PAGE_SIZE;
	}

	if (not_in_kdp) {
		panic("kdp_lightweight_fault called from outside of debugger context");
	}

	assert(map != VM_MAP_NULL);

	assert((cur_target_addr & effective_page_mask) == 0);
	if ((cur_target_addr & effective_page_mask) != 0) {
		return 0;
	}

	if (kdp_lck_rw_lock_is_acquired_exclusive(&map->lock)) {
		return 0;
	}

	if (!vm_map_lookup_entry(map, cur_target_addr, &entry)) {
		return 0;
	}

	if (entry->is_sub_map) {
		return 0;
	}

	object = VME_OBJECT(entry);
	if (object == VM_OBJECT_NULL) {
		return 0;
	}

	object_offset = cur_target_addr - entry->vme_start + VME_OFFSET(entry);

	while (TRUE) {
		if (kdp_lck_rw_lock_is_acquired_exclusive(&object->Lock)) {
			return 0;
		}

		if (object->pager_created && (object->paging_in_progress ||
		    object->activity_in_progress)) {
			return 0;
		}

		m = kdp_vm_page_lookup(object, vm_object_trunc_page(object_offset));

		if (m != VM_PAGE_NULL) {
			if ((object->wimg_bits & VM_WIMG_MASK) != VM_WIMG_DEFAULT) {
				return 0;
			}

			if (m->vmp_laundry || m->vmp_busy || m->vmp_free_when_done || m->vmp_absent || m->vmp_error || m->vmp_cleaning ||
			    m->vmp_overwriting || m->vmp_restart || m->vmp_unusual) {
				return 0;
			}

			assert(!m->vmp_private);
			if (m->vmp_private) {
				return 0;
			}

			assert(!m->vmp_fictitious);
			if (m->vmp_fictitious) {
				return 0;
			}

			assert(m->vmp_q_state != VM_PAGE_USED_BY_COMPRESSOR);
			if (m->vmp_q_state == VM_PAGE_USED_BY_COMPRESSOR) {
				return 0;
			}

			return ptoa(VM_PAGE_GET_PHYS_PAGE(m));
		}

		compressor_external_state = VM_EXTERNAL_STATE_UNKNOWN;

		if (object->pager_created && MUST_ASK_PAGER(object, object_offset, compressor_external_state)) {
			if (compressor_external_state == VM_EXTERNAL_STATE_EXISTS) {
				kr = vm_compressor_pager_get(object->pager,
				    vm_object_trunc_page(object_offset + object->paging_offset),
				    kdp_compressor_decompressed_page_ppnum, &my_fault_type,
				    compressor_flags, &compressed_count_delta);
				if (kr == KERN_SUCCESS) {
					return kdp_compressor_decompressed_page_paddr;
				} else {
					return 0;
				}
			}
		}

		if (object->shadow == VM_OBJECT_NULL) {
			return 0;
		}

		object_offset += object->vo_shadow_offset;
		object = object->shadow;
	}
}

  
                              
                                                                        
                                                 
                                                      
                                  
                                            
                                     
                                                         
   
static boolean_t
vm_page_validate_cs_fast(
	vm_page_t       page,
	vm_map_size_t   fault_page_size,
	vm_map_offset_t fault_phys_offset)
{
	vm_object_t     object;

	object = VM_PAGE_OBJECT(page);
	vm_object_lock_assert_held(object);

	if (page->vmp_wpmapped &&
	    !VMP_CS_TAINTED(page, fault_page_size, fault_phys_offset)) {
		  
                                                            
                                                      
                         
                                                        
                           
     
		vm_object_lock_assert_exclusive(object);
		VMP_CS_SET_VALIDATED(page, fault_page_size, fault_phys_offset, TRUE);
		VMP_CS_SET_TAINTED(page, fault_page_size, fault_phys_offset, TRUE);
		if (cs_debug) {
			printf("CODESIGNING: %s: "
			    "page %p obj %p off 0x%llx "
			    "was modified\n",
			    __FUNCTION__,
			    page, object, page->vmp_offset);
		}
		vm_cs_validated_dirtied++;
	}

	if (VMP_CS_VALIDATED(page, fault_page_size, fault_phys_offset) ||
	    VMP_CS_TAINTED(page, fault_page_size, fault_phys_offset)) {
		return TRUE;
	}
	vm_object_lock_assert_exclusive(object);

#if CHECK_CS_VALIDATION_BITMAP
	kern_return_t kr;

	kr = vnode_pager_cs_check_validation_bitmap(
		object->pager,
		page->vmp_offset + object->paging_offset,
		CS_BITMAP_CHECK);
	if (kr == KERN_SUCCESS) {
		page->vmp_cs_validated = VMP_CS_ALL_TRUE;
		page->vmp_cs_tainted = VMP_CS_ALL_FALSE;
		vm_cs_bitmap_validated++;
		return TRUE;
	}
#endif                                 

	if (!object->alive || object->terminating || object->pager == NULL) {
		  
                                                          
                                     
     
		return TRUE;
	}

	                                          
	vm_object_lock_assert_exclusive(object);
	return FALSE;
}

void
vm_page_validate_cs_mapped_slow(
	vm_page_t       page,
	const void      *kaddr)
{
	vm_object_t             object;
	memory_object_offset_t  mo_offset;
	memory_object_t         pager;
	struct vnode            *vnode;
	int                     validated, tainted, nx;

	assert(page->vmp_busy);
	object = VM_PAGE_OBJECT(page);
	vm_object_lock_assert_exclusive(object);

	vm_cs_validates++;

	  
                                                               
                                                             
           
    
	assert(object->code_signed);
	assert(!object->internal);
	assert(object->pager != NULL);
	assert(object->pager_ready);

	pager = object->pager;
	assert(object->paging_in_progress);
	vnode = vnode_pager_lookup_vnode(pager);
	mo_offset = page->vmp_offset + object->paging_offset;

	                                        
	validated = 0;
	tainted = 0;
	nx = 0;
	cs_validate_page(vnode,
	    pager,
	    mo_offset,
	    (const void *)((const char *)kaddr),
	    &validated,
	    &tainted,
	    &nx);

	page->vmp_cs_validated |= validated;
	page->vmp_cs_tainted |= tainted;
	page->vmp_cs_nx |= nx;

#if CHECK_CS_VALIDATION_BITMAP
	if (page->vmp_cs_validated == VMP_CS_ALL_TRUE &&
	    page->vmp_cs_tainted == VMP_CS_ALL_FALSE) {
		vnode_pager_cs_check_validation_bitmap(object->pager,
		    mo_offset,
		    CS_BITMAP_SET);
	}
#endif                                 
}

void
vm_page_validate_cs_mapped(
	vm_page_t       page,
	vm_map_size_t   fault_page_size,
	vm_map_offset_t fault_phys_offset,
	const void      *kaddr)
{
	if (!vm_page_validate_cs_fast(page, fault_page_size, fault_phys_offset)) {
		vm_page_validate_cs_mapped_slow(page, kaddr);
	}
}

static void
vm_page_map_and_validate_cs(
	vm_object_t     object,
	vm_page_t       page)
{
	vm_object_offset_t      offset;
	vm_map_offset_t         koffset;
	vm_map_size_t           ksize;
	vm_offset_t             kaddr;
	kern_return_t           kr;
	boolean_t               busy_page;
	boolean_t               need_unmap;

	vm_object_lock_assert_exclusive(object);

	assert(object->code_signed);
	offset = page->vmp_offset;

	busy_page = page->vmp_busy;
	if (!busy_page) {
		                                                            
		page->vmp_busy = TRUE;
	}

	  
                                            
                                          
                                      
    
	vm_object_paging_begin(object);

	                                              
	ksize = PAGE_SIZE_64;
	koffset = 0;
	need_unmap = FALSE;
	kr = vm_paging_map_object(page,
	    object,
	    offset,
	    VM_PROT_READ,
	    FALSE,                                                  
	    &ksize,
	    &koffset,
	    &need_unmap);
	if (kr != KERN_SUCCESS) {
		panic("%s: could not map page: 0x%x\n", __FUNCTION__, kr);
	}
	kaddr = CAST_DOWN(vm_offset_t, koffset);

	                              
	vm_page_validate_cs_mapped_slow(page, (const void *) kaddr);

	assert(page->vmp_busy);
	assert(object == VM_PAGE_OBJECT(page));
	vm_object_lock_assert_exclusive(object);

	if (!busy_page) {
		PAGE_WAKEUP_DONE(page);
	}
	if (need_unmap) {
		                                                 
		vm_paging_unmap_object(object, koffset, koffset + ksize);
		koffset = 0;
		ksize = 0;
		kaddr = 0;
	}
	vm_object_paging_end(object);
}

void
vm_page_validate_cs(
	vm_page_t       page,
	vm_map_size_t   fault_page_size,
	vm_map_offset_t fault_phys_offset)
{
	vm_object_t             object;

	object = VM_PAGE_OBJECT(page);
	vm_object_lock_assert_held(object);

	if (vm_page_validate_cs_fast(page, fault_page_size, fault_phys_offset)) {
		return;
	}
	vm_page_map_and_validate_cs(object, page);
}

void
vm_page_validate_cs_mapped_chunk(
	vm_page_t       page,
	const void      *kaddr,
	vm_offset_t     chunk_offset,
	vm_size_t       chunk_size,
	boolean_t       *validated_p,
	unsigned        *tainted_p)
{
	vm_object_t             object;
	vm_object_offset_t      offset, offset_in_page;
	memory_object_t         pager;
	struct vnode            *vnode;
	boolean_t               validated;
	unsigned                tainted;

	*validated_p = FALSE;
	*tainted_p = 0;

	assert(page->vmp_busy);
	object = VM_PAGE_OBJECT(page);
	vm_object_lock_assert_exclusive(object);

	assert(object->code_signed);
	offset = page->vmp_offset;

	if (!object->alive || object->terminating || object->pager == NULL) {
		  
                                                          
                                     
     
		return;
	}
	  
                                                               
                                                             
           
    
	assert(!object->internal);
	assert(object->pager != NULL);
	assert(object->pager_ready);

	pager = object->pager;
	assert(object->paging_in_progress);
	vnode = vnode_pager_lookup_vnode(pager);

	                                         
	offset_in_page = chunk_offset;
	assert(offset_in_page < PAGE_SIZE);

	tainted = 0;
	validated = cs_validate_range(vnode,
	    pager,
	    (object->paging_offset +
	    offset +
	    offset_in_page),
	    (const void *)((const char *)kaddr
	    + offset_in_page),
	    chunk_size,
	    &tainted);
	if (validated) {
		*validated_p = TRUE;
	}
	if (tainted) {
		*tainted_p = tainted;
	}
}

static void
vm_rtfrecord_lock(void)
{
	lck_spin_lock(&vm_rtfr_slock);
}

static void
vm_rtfrecord_unlock(void)
{
	lck_spin_unlock(&vm_rtfr_slock);
}

unsigned int
vmrtfaultinfo_bufsz(void)
{
	return vmrtf_num_records * sizeof(vm_rtfault_record_t);
}

#include <kern/backtrace.h>

__attribute__((noinline))
static void
vm_record_rtfault(thread_t cthread, uint64_t fstart, vm_map_offset_t fault_vaddr, int type_of_fault)
{
	uint64_t fend = mach_continuous_time();

	uint64_t cfpc = 0;
	uint64_t ctid = cthread->thread_id;
	uint64_t cupid = get_current_unique_pid();

	uintptr_t bpc = 0;
	int btr = 0;
	bool u64 = false;

	                                                                   
                                                                       
                                                                      
           
    
	unsigned int bfrs = backtrace_thread_user(cthread, &bpc, 1U, &btr, &u64, NULL, false);

	if ((btr == 0) && (bfrs > 0)) {
		cfpc = bpc;
	}

	assert((fstart != 0) && fend >= fstart);
	vm_rtfrecord_lock();
	assert(vmrtfrs.vmrtfr_curi <= vmrtfrs.vmrtfr_maxi);

	vmrtfrs.vmrtf_total++;
	vm_rtfault_record_t *cvmr = &vmrtfrs.vm_rtf_records[vmrtfrs.vmrtfr_curi++];

	cvmr->rtfabstime = fstart;
	cvmr->rtfduration = fend - fstart;
	cvmr->rtfaddr = fault_vaddr;
	cvmr->rtfpc = cfpc;
	cvmr->rtftype = type_of_fault;
	cvmr->rtfupid = cupid;
	cvmr->rtftid = ctid;

	if (vmrtfrs.vmrtfr_curi > vmrtfrs.vmrtfr_maxi) {
		vmrtfrs.vmrtfr_curi = 0;
	}

	vm_rtfrecord_unlock();
}

int
vmrtf_extract(uint64_t cupid, __unused boolean_t isroot, unsigned long vrecordsz, void *vrecords, unsigned long *vmrtfrv)
{
	vm_rtfault_record_t *cvmrd = vrecords;
	size_t residue = vrecordsz;
	size_t numextracted = 0;
	boolean_t early_exit = FALSE;

	vm_rtfrecord_lock();

	for (int vmfi = 0; vmfi <= vmrtfrs.vmrtfr_maxi; vmfi++) {
		if (residue < sizeof(vm_rtfault_record_t)) {
			early_exit = TRUE;
			break;
		}

		if (vmrtfrs.vm_rtf_records[vmfi].rtfupid != cupid) {
#if     DEVELOPMENT || DEBUG
			if (isroot == FALSE) {
				continue;
			}
#else
			continue;
#endif               
		}

		*cvmrd = vmrtfrs.vm_rtf_records[vmfi];
		cvmrd++;
		residue -= sizeof(vm_rtfault_record_t);
		numextracted++;
	}

	vm_rtfrecord_unlock();

	*vmrtfrv = numextracted;
	return early_exit;
}

  
                                                               
                                             
   
static volatile uint_t vmtc_diagnosing;
unsigned int vmtc_total;
unsigned int vmtc_undiagnosed;
unsigned int vmtc_not_eligible;
unsigned int vmtc_copyin_fail;
unsigned int vmtc_not_found;
unsigned int vmtc_one_bit_flip;
unsigned int vmtc_byte_counts[MAX_TRACK_POWER2 + 1];

#if DEVELOPMENT || DEBUG
  
                                                                         
   
static size_t vmtc_last_buffer_size;
static uint64_t *vmtc_last_before_buffer = NULL;
static uint64_t *vmtc_last_after_buffer = NULL;
#endif                           

  
                                                                     
   
static uint64_t *
vmtc_text_page_diagnose_setup(
	vm_map_offset_t code_addr)
{
	uint64_t        *buffer;
	size_t          size = MIN(vm_map_page_size(current_map()), PAGE_SIZE);

	(void)OSAddAtomic(1, &vmtc_total);

	  
                                                 
    
	if (!OSCompareAndSwap(0, 1, &vmtc_diagnosing)) {
		(void)OSAddAtomic(1, &vmtc_undiagnosed);
		return NULL;
	}

	  
                                         
    
	buffer = kheap_alloc(KHEAP_DEFAULT, size, Z_WAITOK);
	if (copyin((user_addr_t)vm_map_trunc_page(code_addr, size - 1), buffer, size) != 0) {
		                                  
		kheap_free(KHEAP_DEFAULT, buffer, size);
		(void)OSAddAtomic(1, &vmtc_undiagnosed);
		++vmtc_copyin_fail;
		if (!OSCompareAndSwap(1, 0, &vmtc_diagnosing)) {
			panic("Bad compare and swap in setup!");
		}
		return NULL;
	}
	return buffer;
}

  
                                                        
                                  
   
static void
vmtc_text_page_diagnose(
	vm_map_offset_t code_addr,
	uint64_t        *old_code_buffer)
{
	uint64_t        *new_code_buffer;
	size_t          size = MIN(vm_map_page_size(current_map()), PAGE_SIZE);
	uint_t          count = (uint_t)size / sizeof(uint64_t);
	uint_t          diff_count = 0;
	bool            bit_flip = false;
	uint_t          b;
	uint64_t        *new;
	uint64_t        *old;

	new_code_buffer = kheap_alloc(KHEAP_DEFAULT, size, Z_WAITOK);
	if (copyin((user_addr_t)vm_map_trunc_page(code_addr, size - 1), new_code_buffer, size) != 0) {
		                                  
		(void)OSAddAtomic(1, &vmtc_undiagnosed);
		++vmtc_copyin_fail;
		goto done;
	}

	new = new_code_buffer;
	old = old_code_buffer;
	for (; count-- > 0; ++new, ++old) {
		if (*new == *old) {
			continue;
		}

		  
                                               
     
		if (diff_count == 0) {
			uint64_t x = (*new ^ *old);
			assert(x != 0);
			if ((x & (x - 1)) == 0) {
				bit_flip = true;
				++diff_count;
				continue;
			}
		}

		  
                                            
     
		for (b = 0; b < sizeof(uint64_t); ++b) {
			char *n = (char *)new;
			char *o = (char *)old;
			if (n[b] != o[b]) {
				++diff_count;
			}
		}

		                                 
		if (diff_count > (1 << MAX_TRACK_POWER2)) {
			break;
		}
	}

	if (diff_count > 1) {
		bit_flip = false;
	}

	if (diff_count == 0) {
		++vmtc_not_found;
	} else if (bit_flip) {
		++vmtc_one_bit_flip;
		++vmtc_byte_counts[0];
	} else {
		for (b = 0; b <= MAX_TRACK_POWER2; ++b) {
			if (diff_count <= (1 << b)) {
				++vmtc_byte_counts[b];
				break;
			}
		}
		if (diff_count > (1 << MAX_TRACK_POWER2)) {
			++vmtc_byte_counts[MAX_TRACK_POWER2];
		}
	}

done:
	  
                                                    
                                                   
                                                     
    
#if DEVELOPMENT || DEBUG
	if (vmtc_last_before_buffer != NULL) {
		kheap_free(KHEAP_DEFAULT, vmtc_last_before_buffer, vmtc_last_buffer_size);
	}
	if (vmtc_last_after_buffer != NULL) {
		kheap_free(KHEAP_DEFAULT, vmtc_last_after_buffer, vmtc_last_buffer_size);
	}
	vmtc_last_before_buffer = old_code_buffer;
	vmtc_last_after_buffer = new_code_buffer;
	vmtc_last_buffer_size = size;
#else                           
	kheap_free(KHEAP_DEFAULT, new_code_buffer, size);
	kheap_free(KHEAP_DEFAULT, old_code_buffer, size);
#endif                           

	  
                                                 
    
	if (!OSCompareAndSwap(1, 0, &vmtc_diagnosing)) {
		panic("Bad compare and swap in diagnose!");
	}
}

  
                                                                      
                                                                                
                                            
   
static kern_return_t
vmtc_revalidate_lookup(
	vm_map_t               map,
	vm_map_offset_t        vaddr,
	vm_object_t            *ret_object,
	vm_object_offset_t     *ret_offset,
	vm_page_t              *ret_page)
{
	vm_object_t            object;
	vm_object_offset_t     offset;
	vm_page_t              page;
	kern_return_t          kr = KERN_SUCCESS;
	uint8_t                object_lock_type = OBJECT_LOCK_EXCLUSIVE;
	vm_map_version_t       version;
	boolean_t              wired;
	struct vm_object_fault_info fault_info = {};
	vm_map_t               real_map = NULL;
	vm_prot_t              prot;
	vm_object_t            shadow;

	  
                                                      
                                             
    
restart:
	vm_map_lock_read(map);
	object = VM_OBJECT_NULL;                                                     
	kr = vm_map_lookup_locked(&map, vaddr, VM_PROT_READ,
	    object_lock_type, &version, &object, &offset, &prot, &wired,
	    &fault_info, &real_map, NULL);
	vm_map_unlock_read(map);
	if (real_map != NULL && real_map != map) {
		vm_map_unlock(real_map);
	}

	  
                                                              
                                                 
    
	if (kr != KERN_SUCCESS ||
	    object == NULL ||
	    !(prot & VM_PROT_EXECUTE)) {
		kr = KERN_FAILURE;
		goto done;
	}

	  
                                                         
    
	for (;;) {
		  
                                              
     
		page = vm_page_lookup(object, vm_object_trunc_page(offset));
		if (page != NULL) {
			                        
			if (page->vmp_restart) {
				vm_object_unlock(object);
				goto restart;
			}

			  
                                                   
      
			if (page->vmp_busy) {
				PAGE_SLEEP(object, page, TRUE);
				vm_object_unlock(object);
				goto restart;
			}
			break;
		}

		  
                                            
                                     
     
		shadow = object->shadow;
		if (shadow == NULL) {
			kr = KERN_FAILURE;
			goto done;
		}

		  
                            
     
		offset += object->vo_shadow_offset;
		vm_object_lock(shadow);
		vm_object_unlock(object);
		object = shadow;
		shadow = VM_OBJECT_NULL;
	}
	*ret_object = object;
	*ret_offset = vm_object_trunc_page(offset);
	*ret_page = page;

done:
	if (kr != KERN_SUCCESS && object != NULL) {
		vm_object_unlock(object);
	}
	return kr;
}

  
                                                 
   
static bool
is_page_wired(vm_page_t page)
{
	bool result;
	vm_page_lock_queues();
	result = VM_PAGE_WIRED(page);
	vm_page_unlock_queues();
	return result;
}

  
                                                        
                                                         
                                               
  
                                                         
                                                       
                                                      
                                   
   
kern_return_t
revalidate_text_page(task_t task, vm_map_offset_t code_addr)
{
	kern_return_t          kr;
	vm_map_t               map;
	vm_object_t            object = NULL;
	vm_object_offset_t     offset;
	vm_page_t              page = NULL;
	struct vnode           *vnode;
	bool                   do_invalidate = false;
	uint64_t               *diagnose_buffer = NULL;

	map = task->map;
	if (task->map == NULL) {
		return KERN_SUCCESS;
	}

	kr = vmtc_revalidate_lookup(map, code_addr, &object, &offset, &page);
	if (kr != KERN_SUCCESS) {
		goto done;
	}

	  
                                     
    
	if (object->pager == NULL) {
		goto done;
	}

	  
                                                        
    
	vnode = vnode_pager_lookup_vnode(object->pager);
	if (vnode == NULL) {
		goto done;
	}

	  
                                              
    
	if (!object->code_signed ||                                     
	    object->internal ||                                             
	    object->terminating ||                                                           
	    !object->pager_ready) {                                                       
		goto done;
	}

	  
                                                     
    
	vm_page_map_and_validate_cs(object, page);

	  
                  
                                                                  
                                                               
                        
   
                                                 
                                           
    
	if (page->vmp_cs_tainted != VMP_CS_ALL_FALSE) {
#if DEBUG || DEVELOPMENT
		  
                                                           
                                        
     
		if (vmtc_panic_instead) {
			panic("Text page corruption detected: vm_page_t 0x%llx\n", (long long)(uintptr_t)page);
		}
#endif                           

		  
                                                                   
                                                                
     
		do_invalidate = true;
		assert(!page->vmp_busy);
		page->vmp_busy = TRUE;
		vm_object_unlock(object);
		diagnose_buffer = vmtc_text_page_diagnose_setup(code_addr);
	}

done:
	if (do_invalidate) {
		vm_object_lock(object);
		assert(page->vmp_busy);
		assert(VM_PAGE_OBJECT(page) == object);                                                          
		assert(page->vmp_offset == offset);
		PAGE_WAKEUP_DONE(page);                                               

		  
                                               
     
		if (!page->vmp_cleaning &&
		    !page->vmp_laundry &&
		    !page->vmp_fictitious &&
		    !page->vmp_precious &&
		    !page->vmp_absent &&
		    !page->vmp_error &&
		    !page->vmp_dirty &&
		    !is_page_wired(page)) {
			if (page->vmp_pmapped) {
				int refmod = pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(page));
				if (refmod & VM_MEM_MODIFIED) {
					SET_PAGE_DIRTY(page, FALSE);
				}
				if (refmod & VM_MEM_REFERENCED) {
					page->vmp_reference = TRUE;
				}
			}
			                                                               
			if (!page->vmp_dirty) {
				VM_PAGE_FREE(page);
			} else {
				(void)OSAddAtomic(1, &vmtc_not_eligible);
			}
		} else {
			(void)OSAddAtomic(1, &vmtc_not_eligible);
		}
		vm_object_unlock(object);

		  
                                                        
                                                      
     
		if (diagnose_buffer) {
			vmtc_text_page_diagnose(code_addr, diagnose_buffer);
		}
		return KERN_FAILURE;
	}

	if (object != NULL) {
		vm_object_unlock(object);
	}
	return KERN_SUCCESS;
}

#if DEBUG || DEVELOPMENT
  
                                                                     
                                                                     
            
   
extern kern_return_t vm_corrupt_text_addr(uintptr_t);

kern_return_t
vm_corrupt_text_addr(uintptr_t va)
{
	task_t                 task = current_task();
	vm_map_t               map;
	kern_return_t          kr = KERN_SUCCESS;
	vm_object_t            object = VM_OBJECT_NULL;
	vm_object_offset_t     offset;
	vm_page_t              page = NULL;
	pmap_paddr_t           pa;

	map = task->map;
	if (task->map == NULL) {
		printf("corrupt_text_addr: no map\n");
		return KERN_FAILURE;
	}

	kr = vmtc_revalidate_lookup(map, (vm_map_offset_t)va, &object, &offset, &page);
	if (kr != KERN_SUCCESS) {
		printf("corrupt_text_addr: page lookup failed\n");
		return kr;
	}
	                                     
	pa = ptoa(VM_PAGE_GET_PHYS_PAGE(page)) + (va - vm_object_trunc_page(va));

	  
                                             
                                                      
                                                                
                      
   
                                                                     
                                                                 
                               
    
	if (page->vmp_busy) {
		printf("corrupt_text_addr: vmp_busy\n");
		kr = KERN_FAILURE;
	}
	if (page->vmp_cleaning) {
		printf("corrupt_text_addr: vmp_cleaning\n");
		kr = KERN_FAILURE;
	}
	if (page->vmp_laundry) {
		printf("corrupt_text_addr: vmp_cleaning\n");
		kr = KERN_FAILURE;
	}
	if (page->vmp_fictitious) {
		printf("corrupt_text_addr: vmp_fictitious\n");
		kr = KERN_FAILURE;
	}
	if (page->vmp_precious) {
		printf("corrupt_text_addr: vmp_precious\n");
		kr = KERN_FAILURE;
	}
	if (page->vmp_absent) {
		printf("corrupt_text_addr: vmp_absent\n");
		kr = KERN_FAILURE;
	}
	if (page->vmp_error) {
		printf("corrupt_text_addr: vmp_error\n");
		kr = KERN_FAILURE;
	}
	if (page->vmp_dirty) {
		printf("corrupt_text_addr: vmp_dirty\n");
		kr = KERN_FAILURE;
	}
	if (is_page_wired(page)) {
		printf("corrupt_text_addr: wired\n");
		kr = KERN_FAILURE;
	}
	if (!page->vmp_pmapped) {
		printf("corrupt_text_addr: !vmp_pmapped\n");
		kr = KERN_FAILURE;
	}

	if (kr == KERN_SUCCESS) {
		printf("corrupt_text_addr: using physaddr 0x%llx\n", (long long)pa);
		kr = pmap_test_text_corruption(pa);
		if (kr != KERN_SUCCESS) {
			printf("corrupt_text_addr: pmap error %d\n", kr);
		}
	} else {
		printf("corrupt_text_addr: object %p\n", object);
		printf("corrupt_text_addr: offset 0x%llx\n", (uint64_t)offset);
		printf("corrupt_text_addr: va 0x%llx\n", (uint64_t)va);
		printf("corrupt_text_addr: vm_object_trunc_page(va) 0x%llx\n", (uint64_t)vm_object_trunc_page(va));
		printf("corrupt_text_addr: vm_page_t %p\n", page);
		printf("corrupt_text_addr: ptoa(PHYS_PAGE) 0x%llx\n", (uint64_t)ptoa(VM_PAGE_GET_PHYS_PAGE(page)));
		printf("corrupt_text_addr: using physaddr 0x%llx\n", (uint64_t)pa);
	}

	if (object != VM_OBJECT_NULL) {
		vm_object_unlock(object);
	}
	return kr;
}
#endif                           
