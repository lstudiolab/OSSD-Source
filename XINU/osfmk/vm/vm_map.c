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
  
   
  
                    
                                                    
             
  
                                 
   

#include <mach_assert.h>

#include <vm/vm_options.h>

#include <libkern/OSAtomic.h>

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <mach/vm_behavior.h>
#include <mach/vm_statistics.h>
#include <mach/memory_object.h>
#include <mach/mach_vm.h>
#include <machine/cpu_capabilities.h>
#include <mach/sdt.h>

#include <kern/assert.h>
#include <kern/backtrace.h>
#include <kern/counter.h>
#include <kern/exc_guard.h>
#include <kern/kalloc.h>
#include <kern/zalloc_internal.h>

#include <vm/cpm.h>
#include <vm/vm_compressor.h>
#include <vm/vm_compressor_pager.h>
#include <vm/vm_init.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <ipc/ipc_port.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>

#include <mach/vm_map_server.h>
#include <mach/mach_host_server.h>
#include <vm/vm_protos.h>
#include <vm/vm_purgeable_internal.h>

#include <vm/vm_protos.h>
#include <vm/vm_shared_region.h>
#include <vm/vm_map_store.h>

#include <san/kasan.h>

#include <sys/codesign.h>
#include <sys/mman.h>

#include <libkern/section_keywords.h>
#if DEVELOPMENT || DEBUG
extern int proc_selfcsflags(void);
int panic_on_unsigned_execute = 0;
#endif                           

#if MACH_ASSERT
int debug4k_filter = 0;
char debug4k_proc_name[1024] = "";
int debug4k_proc_filter = (int)-1 & ~(1 << __DEBUG4K_FAULT);
int debug4k_panic_on_misaligned_sharing = 0;
const char *debug4k_category_name[] = {
	"error",               
	"life",                
	"load",                
	"fault",               
	"copy",                
	"share",               
	"adjust",              
	"pmap",                
	"mementry",            
	"iokit",               
	"upl",                  
	"exc",                  
	"vfs"                   
};
#endif                  
int debug4k_no_cow_copyin = 0;


#if __arm64__
extern const int fourk_binary_compatibility_unsafe;
extern const int fourk_binary_compatibility_allow_wx;
#endif                
extern int proc_selfpid(void);
extern char *proc_name_address(void *p);

#if VM_MAP_DEBUG_APPLE_PROTECT
int vm_map_debug_apple_protect = 0;
#endif                                 
#if VM_MAP_DEBUG_FOURK
int vm_map_debug_fourk = 0;
#endif                         

SECURITY_READ_ONLY_LATE(int) vm_map_executable_immutable = 1;
int vm_map_executable_immutable_verbose = 0;

os_refgrp_decl(static, map_refgrp, "vm_map", NULL);

extern u_int32_t random(void);                                
                      
   

static void vm_map_simplify_range(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end);                

static boolean_t        vm_map_range_check(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end,
	vm_map_entry_t  *entry);

static vm_map_entry_t   _vm_map_entry_create(
	struct vm_map_header    *map_header, boolean_t map_locked);

static void             _vm_map_entry_dispose(
	struct vm_map_header    *map_header,
	vm_map_entry_t          entry);

static void             vm_map_pmap_enter(
	vm_map_t                map,
	vm_map_offset_t         addr,
	vm_map_offset_t         end_addr,
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_prot_t               protection);

static void             _vm_map_clip_end(
	struct vm_map_header    *map_header,
	vm_map_entry_t          entry,
	vm_map_offset_t         end);

static void             _vm_map_clip_start(
	struct vm_map_header    *map_header,
	vm_map_entry_t          entry,
	vm_map_offset_t         start);

static void             vm_map_entry_delete(
	vm_map_t        map,
	vm_map_entry_t  entry);

static kern_return_t    vm_map_delete(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end,
	int             flags,
	vm_map_t        zap_map);

static void             vm_map_copy_insert(
	vm_map_t        map,
	vm_map_entry_t  after_where,
	vm_map_copy_t   copy);

static kern_return_t    vm_map_copy_overwrite_unaligned(
	vm_map_t        dst_map,
	vm_map_entry_t  entry,
	vm_map_copy_t   copy,
	vm_map_address_t start,
	boolean_t       discard_on_success);

static kern_return_t    vm_map_copy_overwrite_aligned(
	vm_map_t        dst_map,
	vm_map_entry_t  tmp_entry,
	vm_map_copy_t   copy,
	vm_map_offset_t start,
	pmap_t          pmap);

static kern_return_t    vm_map_copyin_kernel_buffer(
	vm_map_t        src_map,
	vm_map_address_t src_addr,
	vm_map_size_t   len,
	boolean_t       src_destroy,
	vm_map_copy_t   *copy_result);           

static kern_return_t    vm_map_copyout_kernel_buffer(
	vm_map_t        map,
	vm_map_address_t *addr,             
	vm_map_copy_t   copy,
	vm_map_size_t   copy_size,
	boolean_t       overwrite,
	boolean_t       consume_on_success);

static void             vm_map_fork_share(
	vm_map_t        old_map,
	vm_map_entry_t  old_entry,
	vm_map_t        new_map);

static boolean_t        vm_map_fork_copy(
	vm_map_t        old_map,
	vm_map_entry_t  *old_entry_p,
	vm_map_t        new_map,
	int             vm_map_copyin_flags);

static kern_return_t    vm_map_wire_nested(
	vm_map_t                   map,
	vm_map_offset_t            start,
	vm_map_offset_t            end,
	vm_prot_t                  caller_prot,
	vm_tag_t                   tag,
	boolean_t                  user_wire,
	pmap_t                     map_pmap,
	vm_map_offset_t            pmap_addr,
	ppnum_t                    *physpage_p);

static kern_return_t    vm_map_unwire_nested(
	vm_map_t                   map,
	vm_map_offset_t            start,
	vm_map_offset_t            end,
	boolean_t                  user_wire,
	pmap_t                     map_pmap,
	vm_map_offset_t            pmap_addr);

static kern_return_t    vm_map_overwrite_submap_recurse(
	vm_map_t                   dst_map,
	vm_map_offset_t            dst_addr,
	vm_map_size_t              dst_size);

static kern_return_t    vm_map_copy_overwrite_nested(
	vm_map_t                   dst_map,
	vm_map_offset_t            dst_addr,
	vm_map_copy_t              copy,
	boolean_t                  interruptible,
	pmap_t                     pmap,
	boolean_t                  discard_on_success);

static kern_return_t    vm_map_remap_extract(
	vm_map_t                map,
	vm_map_offset_t         addr,
	vm_map_size_t           size,
	boolean_t               copy,
	struct vm_map_header    *map_header,
	vm_prot_t               *cur_protection,
	vm_prot_t               *max_protection,
	vm_inherit_t            inheritance,
	vm_map_kernel_flags_t   vmk_flags);

static kern_return_t    vm_map_remap_range_allocate(
	vm_map_t                map,
	vm_map_address_t        *address,
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	vm_map_entry_t          *map_entry);

static void             vm_map_region_look_for_page(
	vm_map_t                   map,
	vm_map_offset_t            va,
	vm_object_t                object,
	vm_object_offset_t         offset,
	int                        max_refcnt,
	unsigned short             depth,
	vm_region_extended_info_t  extended,
	mach_msg_type_number_t count);

static int              vm_map_region_count_obj_refs(
	vm_map_entry_t             entry,
	vm_object_t                object);


static kern_return_t    vm_map_willneed(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end);

static kern_return_t    vm_map_reuse_pages(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end);

static kern_return_t    vm_map_reusable_pages(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end);

static kern_return_t    vm_map_can_reuse(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end);

#if MACH_ASSERT
static kern_return_t    vm_map_pageout(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end);
#endif                  

kern_return_t vm_map_corpse_footprint_collect(
	vm_map_t        old_map,
	vm_map_entry_t  old_entry,
	vm_map_t        new_map);
void vm_map_corpse_footprint_collect_done(
	vm_map_t        new_map);
void vm_map_corpse_footprint_destroy(
	vm_map_t        map);
kern_return_t vm_map_corpse_footprint_query_page_info(
	vm_map_t        map,
	vm_map_offset_t va,
	int             *disposition_p);
void vm_map_footprint_query_page_info(
	vm_map_t        map,
	vm_map_entry_t  map_entry,
	vm_map_offset_t curr_s_offset,
	int             *disposition_p);

static const struct vm_map_entry vm_map_entry_template = {
	.behavior = VM_BEHAVIOR_DEFAULT,
	.inheritance = VM_INHERIT_DEFAULT,
};

pid_t find_largest_process_vm_map_entries(void);

  
                                                                 
                                                                 
                                                                  
                                                              
                                                                
                                                               
                  
   

static inline void
vm_map_entry_copy_pmap_cs_assoc(
	vm_map_t map __unused,
	vm_map_entry_t new __unused,
	vm_map_entry_t old __unused)
{
	                                                           
	assert(new->pmap_cs_associated == FALSE);
}

  
                                                                             
                                                                
                                                               
   
static inline void
vm_map_entry_copy_code_signing(
	vm_map_t map,
	vm_map_entry_t new,
	vm_map_entry_t old __unused)
{
	if (VM_MAP_POLICY_ALLOW_JIT_COPY(map)) {
		assert(new->used_for_jit == old->used_for_jit);
	} else {
		new->used_for_jit = FALSE;
	}
}

static inline void
vm_map_entry_copy(
	vm_map_t map,
	vm_map_entry_t new,
	vm_map_entry_t old)
{
	boolean_t _vmec_reserved = new->from_reserved_zone;
	*new = *old;
	new->is_shared = FALSE;
	new->needs_wakeup = FALSE;
	new->in_transition = FALSE;
	new->wired_count = 0;
	new->user_wired_count = 0;
	new->permanent = FALSE;
	vm_map_entry_copy_code_signing(map, new, old);
	vm_map_entry_copy_pmap_cs_assoc(map, new, old);
	new->from_reserved_zone = _vmec_reserved;
	if (new->iokit_acct) {
		assertf(!new->use_pmap, "old %p new %p\n", old, new);
		new->iokit_acct = FALSE;
		new->use_pmap = TRUE;
	}
	new->vme_resilient_codesign = FALSE;
	new->vme_resilient_media = FALSE;
	new->vme_atomic = FALSE;
	new->vme_no_copy_on_read = FALSE;
}

static inline void
vm_map_entry_copy_full(
	vm_map_entry_t new,
	vm_map_entry_t old)
{
	boolean_t _vmecf_reserved = new->from_reserved_zone;
	*new = *old;
	new->from_reserved_zone = _vmecf_reserved;
}

  
                                                          
                                                                             
   
__attribute__((always_inline))
int
vm_map_lock_read_to_write(vm_map_t map)
{
	if (lck_rw_lock_shared_to_exclusive(&(map)->lock)) {
		DTRACE_VM(vm_map_lock_upgrade);
		return 0;
	}
	return 1;
}

__attribute__((always_inline))
boolean_t
vm_map_try_lock(vm_map_t map)
{
	if (lck_rw_try_lock_exclusive(&(map)->lock)) {
		DTRACE_VM(vm_map_lock_w);
		return TRUE;
	}
	return FALSE;
}

__attribute__((always_inline))
boolean_t
vm_map_try_lock_read(vm_map_t map)
{
	if (lck_rw_try_lock_shared(&(map)->lock)) {
		DTRACE_VM(vm_map_lock_r);
		return TRUE;
	}
	return FALSE;
}

  
                                                  
                                                 
                                                                          
                                                                      
                                        
   
int
vm_self_region_page_shift_safely(
	vm_map_t target_map)
{
	int effective_page_shift = 0;

	if (PAGE_SIZE == (4096)) {
		                                          
		return PAGE_SHIFT;
	}
	                                                                      
	effective_page_shift = thread_self_region_page_shift();
	if (effective_page_shift) {
		                                           
		return effective_page_shift;
	}
	                                                          
	effective_page_shift = VM_MAP_PAGE_SHIFT(current_map());
	if (effective_page_shift == VM_MAP_PAGE_SHIFT(target_map)) {
		                                  
		return effective_page_shift;
	}
	                        
	return -1;
}
int
vm_self_region_page_shift(
	vm_map_t target_map)
{
	int effective_page_shift;

	effective_page_shift = vm_self_region_page_shift_safely(target_map);
	if (effective_page_shift == -1) {
		                                              
		effective_page_shift = MIN(VM_MAP_PAGE_SHIFT(current_map()),
		    VM_MAP_PAGE_SHIFT(target_map));
	}
	return effective_page_shift;
}


  
                                                                                  
                                                                                          
                                                                                        
                                                                                              
                                                                                         
                                                                                        
                                                                                        
                                                                                        
                                                                                        
                                          
  
                                                              
                                                              
                                                                  
  
                                                                               
                                                                           
                                                                                 
                                                          
  
                                                                                             
                                                                                    
                                                                                   
                                                                                             
                                                                                             
                                                                                           
                       
   

extern int allow_data_exec, allow_stack_exec;

int
override_nx(vm_map_t map, uint32_t user_tag)                        
{
	int current_abi;

	if (map->pmap == kernel_pmap) {
		return FALSE;
	}

	  
                                                         
    

	if (vm_map_is_64bit(map)) {
		current_abi = VM_ABI_64;
	} else {
		current_abi = VM_ABI_32;
	}

	  
                                                                      
                                                    
    

	if (user_tag == VM_MEMORY_STACK) {
		return allow_stack_exec & current_abi;
	}

	return (allow_data_exec & current_abi) && (map->map_disallow_data_exec == FALSE);
}


  
                                                           
                                                       
                                                        
                                  
  
                                                        
  
                                                          
                                                      
  
                                                            
                                                                 
                                                               
                                                                 
                                                                
                                                                  
                      
  
                                                              
                                                     
                                                            
                                                       
                                                          
                                                             
                                                               
                                                           
                    
  
                                                               
                                                  
                                                           
                                                            
                                                             
                                                           
                                                                   
                                                                
                                                                  
                                                                  
                                                            
                                                 
   

static SECURITY_READ_ONLY_LATE(zone_t) vm_map_zone;                                       
static SECURITY_READ_ONLY_LATE(zone_t) vm_map_entry_reserved_zone;                                                     
static SECURITY_READ_ONLY_LATE(zone_t) vm_map_copy_zone;                                       

SECURITY_READ_ONLY_LATE(zone_t)        vm_map_entry_zone;                                       
SECURITY_READ_ONLY_LATE(zone_t)        vm_map_holes_zone;                                                      

#define VM_MAP_ZONE_NAME "maps"
#define VM_MAP_ZFLAGS ( \
  ZC_NOENCRYPT | \
  ZC_NOGC      | \
  ZC_NOGZALLOC | \
  ZC_ALLOW_FOREIGN)

#define VME_RESERVED_ZONE_NAME "Reserved VM map entries"
#define VM_MAP_RESERVED_ZFLAGS ( \
    ZC_NOENCRYPT          | \
    ZC_ALLOW_FOREIGN      | \
    ZC_NOCALLOUT          | \
    ZC_NOGZALLOC          | \
    ZC_KASAN_NOQUARANTINE | \
    ZC_NOGC)

#define VM_MAP_HOLES_ZONE_NAME "VM map holes"
#define VM_MAP_HOLES_ZFLAGS ( \
    ZC_NOENCRYPT | \
    ZC_NOGC      | \
    ZC_NOGZALLOC | \
    ZC_ALLOW_FOREIGN)

  
                                                       
                                                              
                 
   
static inline void
vm_map_copy_require(struct vm_map_copy *copy)
{
	zone_id_require(ZONE_ID_VM_MAP_COPY, sizeof(struct vm_map_copy), copy);
}

  
                  
  
                                                                 
                                                    
   
void
vm_map_require(vm_map_t map)
{
	zone_id_require_allow_foreign(ZONE_ID_VM_MAP, sizeof(struct _vm_map), map);
}

static __startup_data vm_offset_t      map_data;
static __startup_data vm_size_t        map_data_size;
static __startup_data vm_offset_t      kentry_data;
static __startup_data vm_size_t        kentry_data_size;
static __startup_data vm_offset_t      map_holes_data;
static __startup_data vm_size_t        map_holes_data_size;

#if XNU_TARGET_OS_OSX
#define         NO_COALESCE_LIMIT  ((1024 * 128) - 1)
#else                        
#define         NO_COALESCE_LIMIT  0
#endif                        

                                                                      
unsigned int not_in_kdp = 1;

unsigned int vm_map_set_cache_attr_count = 0;

kern_return_t
vm_map_set_cache_attr(
	vm_map_t        map,
	vm_map_offset_t va)
{
	vm_map_entry_t  map_entry;
	vm_object_t     object;
	kern_return_t   kr = KERN_SUCCESS;

	vm_map_lock_read(map);

	if (!vm_map_lookup_entry(map, va, &map_entry) ||
	    map_entry->is_sub_map) {
		  
                                       
     
		kr = KERN_INVALID_ARGUMENT;
		goto done;
	}
	object = VME_OBJECT(map_entry);

	if (object == VM_OBJECT_NULL) {
		  
                                                   
     
		kr = KERN_INVALID_ARGUMENT;
		goto done;
	}
	vm_object_lock(object);
	object->set_cache_attr = TRUE;
	vm_object_unlock(object);

	vm_map_set_cache_attr_count++;
done:
	vm_map_unlock_read(map);

	return kr;
}


#if CONFIG_CODE_DECRYPTION
  
                          
                                                                        
                        
                                                                          
                                                                                     
                                                                                      
   
kern_return_t
vm_map_apple_protected(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_object_offset_t      crypto_backing_offset,
	struct pager_crypt_info *crypt_info,
	uint32_t                cryptid)
{
	boolean_t       map_locked;
	kern_return_t   kr;
	vm_map_entry_t  map_entry;
	struct vm_map_entry tmp_entry;
	memory_object_t unprotected_mem_obj;
	vm_object_t     protected_object;
	vm_map_offset_t map_addr;
	vm_map_offset_t start_aligned, end_aligned;
	vm_object_offset_t      crypto_start, crypto_end;
	int             vm_flags;
	vm_map_kernel_flags_t vmk_flags;
	boolean_t       cache_pager;

	vm_flags = 0;
	vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;

	map_locked = FALSE;
	unprotected_mem_obj = MEMORY_OBJECT_NULL;

	start_aligned = vm_map_trunc_page(start, PAGE_MASK_64);
	end_aligned = vm_map_round_page(end, PAGE_MASK_64);
	start_aligned = vm_map_trunc_page(start_aligned, VM_MAP_PAGE_MASK(map));
	end_aligned = vm_map_round_page(end_aligned, VM_MAP_PAGE_MASK(map));

#if __arm64__
	  
                                                              
                                                            
   
                                                               
                         
                 
                                                          
                         
                                                              
                                                                 
                                                     
    
#endif                

	map_addr = start_aligned;
	for (map_addr = start_aligned;
	    map_addr < end;
	    map_addr = tmp_entry.vme_end) {
		vm_map_lock(map);
		map_locked = TRUE;

		                                    
		if (!vm_map_lookup_entry(map,
		    map_addr,
		    &map_entry) ||
		    map_entry->is_sub_map ||
		    VME_OBJECT(map_entry) == VM_OBJECT_NULL) {
			                                        
			kr = KERN_INVALID_ARGUMENT;
			goto done;
		}

		                                                      
                                         
		if ((cryptid != CRYPTID_MODEL_ENCRYPTION) &&
		    !(map_entry->protection & VM_PROT_EXECUTE)) {
			kr = KERN_INVALID_ARGUMENT;
			goto done;
		}

		                                              
		protected_object = VME_OBJECT(map_entry);
		if (protected_object == VM_OBJECT_NULL) {
			                                                    
			kr = KERN_INVALID_ARGUMENT;
			goto done;
		}
		                                                               
		vm_object_reference(protected_object);

		                                                      
		vm_map_clip_start(map, map_entry, start_aligned);
		vm_map_clip_end(map, map_entry, end_aligned);

		tmp_entry = *map_entry;
		map_entry = VM_MAP_ENTRY_NULL;                                    
		vm_map_unlock(map);
		map_locked = FALSE;

		  
                                                     
                                   
     
		crypto_start = 0;
		crypto_end = tmp_entry.vme_end - tmp_entry.vme_start;
		if (tmp_entry.vme_start < start) {
			if (tmp_entry.vme_start != start_aligned) {
				kr = KERN_INVALID_ADDRESS;
			}
			crypto_start += (start - tmp_entry.vme_start);
		}
		if (tmp_entry.vme_end > end) {
			if (tmp_entry.vme_end != end_aligned) {
				kr = KERN_INVALID_ADDRESS;
			}
			crypto_end -= (tmp_entry.vme_end - end);
		}

		  
                                                                
                                                               
                                                   
     
		if (crypto_backing_offset == (vm_object_offset_t) -1) {
			crypto_backing_offset = VME_OFFSET(&tmp_entry);
		}

		cache_pager = TRUE;
#if XNU_TARGET_OS_OSX
		if (vm_map_is_alien(map)) {
			cache_pager = FALSE;
		}
#endif                        

		  
                                                                 
                             
                                                                     
                                                                       
        
     
		unprotected_mem_obj = apple_protect_pager_setup(
			protected_object,
			VME_OFFSET(&tmp_entry),
			crypto_backing_offset,
			crypt_info,
			crypto_start,
			crypto_end,
			cache_pager);

		                                           
		vm_object_deallocate(protected_object);

		if (unprotected_mem_obj == NULL) {
			kr = KERN_FAILURE;
			goto done;
		}

		vm_flags = VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE;
		                                        
		vmk_flags.vmkf_overwrite_immutable = TRUE;
#if __arm64__
		if (tmp_entry.used_for_jit &&
		    (VM_MAP_PAGE_SHIFT(map) != FOURK_PAGE_SHIFT ||
		    PAGE_SHIFT != FOURK_PAGE_SHIFT) &&
		    fourk_binary_compatibility_unsafe &&
		    fourk_binary_compatibility_allow_wx) {
			printf("** FOURK_COMPAT [%d]: "
			    "allowing write+execute at 0x%llx\n",
			    proc_selfpid(), tmp_entry.vme_start);
			vmk_flags.vmkf_map_jit = TRUE;
		}
#endif                

		                                                        
		map_addr = tmp_entry.vme_start;
		kr = vm_map_enter_mem_object(map,
		    &map_addr,
		    (tmp_entry.vme_end -
		    tmp_entry.vme_start),
		    (mach_vm_offset_t) 0,
		    vm_flags,
		    vmk_flags,
		    VM_KERN_MEMORY_NONE,
		    (ipc_port_t)(uintptr_t) unprotected_mem_obj,
		    0,
		    TRUE,
		    tmp_entry.protection,
		    tmp_entry.max_protection,
		    tmp_entry.inheritance);
		assertf(kr == KERN_SUCCESS,
		    "kr = 0x%x\n", kr);
		assertf(map_addr == tmp_entry.vme_start,
		    "map_addr=0x%llx vme_start=0x%llx tmp_entry=%p\n",
		    (uint64_t)map_addr,
		    (uint64_t) tmp_entry.vme_start,
		    &tmp_entry);

#if VM_MAP_DEBUG_APPLE_PROTECT
		if (vm_map_debug_apple_protect) {
			printf("APPLE_PROTECT: map %p [0x%llx:0x%llx] pager %p:"
			    " backing:[object:%p,offset:0x%llx,"
			    "crypto_backing_offset:0x%llx,"
			    "crypto_start:0x%llx,crypto_end:0x%llx]\n",
			    map,
			    (uint64_t) map_addr,
			    (uint64_t) (map_addr + (tmp_entry.vme_end -
			    tmp_entry.vme_start)),
			    unprotected_mem_obj,
			    protected_object,
			    VME_OFFSET(&tmp_entry),
			    crypto_backing_offset,
			    crypto_start,
			    crypto_end);
		}
#endif                                 

		  
                                      
                                 
                                                                
                       
     
		memory_object_deallocate(unprotected_mem_obj);
		unprotected_mem_obj = MEMORY_OBJECT_NULL;

		                                  
		crypto_backing_offset += (tmp_entry.vme_end -
		    tmp_entry.vme_start);
		crypto_backing_offset -= crypto_start;
	}
	kr = KERN_SUCCESS;

done:
	if (map_locked) {
		vm_map_unlock(map);
	}
	return kr;
}
#endif                              


LCK_GRP_DECLARE(vm_map_lck_grp, "vm_map");
LCK_ATTR_DECLARE(vm_map_lck_attr, 0, 0);
LCK_ATTR_DECLARE(vm_map_lck_rw_attr, 0, LCK_ATTR_DEBUG);

#if XNU_TARGET_OS_OSX
int malloc_no_cow = 0;
#else                        
int malloc_no_cow = 1;
#endif                        
uint64_t vm_memory_malloc_no_cow_mask = 0ULL;
#if DEBUG
int vm_check_map_sanity = 0;
#endif

  
               
  
                                                       
                             
  
                                                               
                          
  
                                     
  
                                       
                                                   
                                                                   
  
                                                                         
                                                                         
                                                                    
                                                                     
                  
   
__startup_func
void
vm_map_init(void)
{
	const char *mez_name = "VM map entries";


#if MACH_ASSERT
	PE_parse_boot_argn("debug4k_filter", &debug4k_filter,
	    sizeof(debug4k_filter));
#endif                  

	vm_map_zone = zone_create_ext(VM_MAP_ZONE_NAME, sizeof(struct _vm_map),
	    VM_MAP_ZFLAGS, ZONE_ID_VM_MAP, NULL);

	vm_map_entry_zone = zone_create(mez_name, sizeof(struct vm_map_entry),
	    ZC_NOENCRYPT | ZC_NOGZALLOC | ZC_NOCALLOUT);

	  
                                                              
                                              
    
	vm_map_entry_reserved_zone = zone_create_ext(VME_RESERVED_ZONE_NAME,
	    sizeof(struct vm_map_entry), VM_MAP_RESERVED_ZFLAGS,
	    ZONE_ID_ANY, NULL);

	vm_map_copy_zone = zone_create_ext("VM map copies", sizeof(struct vm_map_copy),
	    ZC_NOENCRYPT | ZC_CACHING, ZONE_ID_VM_MAP_COPY, NULL);

	vm_map_holes_zone = zone_create(VM_MAP_HOLES_ZONE_NAME,
	    sizeof(struct vm_map_links), VM_MAP_HOLES_ZFLAGS);

	  
                                                                       
    
	zone_cram_foreign(vm_map_zone, map_data, map_data_size);
	zone_cram_foreign(vm_map_entry_reserved_zone, kentry_data, kentry_data_size);
	zone_cram_foreign(vm_map_holes_zone, map_holes_data, map_holes_data_size);

	  
                                                                              
    
	VM_PAGE_MOVE_STOLEN(atop_64(map_data_size) + atop_64(kentry_data_size) + atop_64(map_holes_data_size));

#if VM_MAP_DEBUG_APPLE_PROTECT
	PE_parse_boot_argn("vm_map_debug_apple_protect",
	    &vm_map_debug_apple_protect,
	    sizeof(vm_map_debug_apple_protect));
#endif                                 
#if VM_MAP_DEBUG_APPLE_FOURK
	PE_parse_boot_argn("vm_map_debug_fourk",
	    &vm_map_debug_fourk,
	    sizeof(vm_map_debug_fourk));
#endif                         
	PE_parse_boot_argn("vm_map_executable_immutable",
	    &vm_map_executable_immutable,
	    sizeof(vm_map_executable_immutable));
	PE_parse_boot_argn("vm_map_executable_immutable_verbose",
	    &vm_map_executable_immutable_verbose,
	    sizeof(vm_map_executable_immutable_verbose));

	PE_parse_boot_argn("malloc_no_cow",
	    &malloc_no_cow,
	    sizeof(malloc_no_cow));
	if (malloc_no_cow) {
		vm_memory_malloc_no_cow_mask = 0ULL;
		vm_memory_malloc_no_cow_mask |= 1ULL << VM_MEMORY_MALLOC;
		vm_memory_malloc_no_cow_mask |= 1ULL << VM_MEMORY_MALLOC_SMALL;
		vm_memory_malloc_no_cow_mask |= 1ULL << VM_MEMORY_MALLOC_MEDIUM;
		vm_memory_malloc_no_cow_mask |= 1ULL << VM_MEMORY_MALLOC_LARGE;
                                                                  
                                                              
		vm_memory_malloc_no_cow_mask |= 1ULL << VM_MEMORY_MALLOC_TINY;
		vm_memory_malloc_no_cow_mask |= 1ULL << VM_MEMORY_MALLOC_LARGE_REUSABLE;
		vm_memory_malloc_no_cow_mask |= 1ULL << VM_MEMORY_MALLOC_LARGE_REUSED;
		vm_memory_malloc_no_cow_mask |= 1ULL << VM_MEMORY_MALLOC_NANO;
                                                               
		PE_parse_boot_argn("vm_memory_malloc_no_cow_mask",
		    &vm_memory_malloc_no_cow_mask,
		    sizeof(vm_memory_malloc_no_cow_mask));
	}

#if DEBUG
	PE_parse_boot_argn("vm_check_map_sanity", &vm_check_map_sanity, sizeof(vm_check_map_sanity));
	if (vm_check_map_sanity) {
		kprintf("VM sanity checking enabled\n");
	} else {
		kprintf("VM sanity checking disabled. Set bootarg vm_check_map_sanity=1 to enable\n");
	}
#endif            

#if DEVELOPMENT || DEBUG
	PE_parse_boot_argn("panic_on_unsigned_execute",
	    &panic_on_unsigned_execute,
	    sizeof(panic_on_unsigned_execute));
#endif                           
}

__startup_func
static void
vm_map_steal_memory(void)
{
	uint16_t kentry_initial_pages;
	uint16_t zone_foreign_pages;

	map_data_size = zone_get_foreign_alloc_size(VM_MAP_ZONE_NAME,
	    sizeof(struct _vm_map), VM_MAP_ZFLAGS, 1);

	  
                                                                        
                                                                  
                                                                     
                   
    
#if defined(__LP64__)
	kentry_initial_pages = (uint16_t)atop(16 * 4096);
#else
	kentry_initial_pages = 6;
#endif

#if CONFIG_GZALLOC
	                                                                   
                            
    
	if (gzalloc_enabled()) {
		kentry_initial_pages *= 1024;
	}
#endif
	if (PE_parse_boot_argn("zone_foreign_pages", &zone_foreign_pages,
	    sizeof(zone_foreign_pages))) {
		kentry_initial_pages = zone_foreign_pages;
	}

	kentry_data_size = zone_get_foreign_alloc_size(VME_RESERVED_ZONE_NAME,
	    sizeof(struct vm_map_entry), VM_MAP_RESERVED_ZFLAGS,
	    kentry_initial_pages);

	map_holes_data_size = zone_get_foreign_alloc_size(VM_MAP_HOLES_ZONE_NAME,
	    sizeof(struct vm_map_links), VM_MAP_HOLES_ZFLAGS,
	    kentry_initial_pages);

	  
                                                                   
                                                               
         
    
	vm_size_t total_size;
	if (os_add3_overflow(map_data_size, kentry_data_size,
	    map_holes_data_size, &total_size)) {
		panic("vm_map_steal_memory: overflow in amount of memory requested");
	}
	map_data = zone_foreign_mem_init(total_size);
	kentry_data = map_data + map_data_size;
	map_holes_data = kentry_data + kentry_data_size;
}
STARTUP(PMAP_STEAL, STARTUP_RANK_FIRST, vm_map_steal_memory);

boolean_t vm_map_supports_hole_optimization = FALSE;

void
vm_kernel_reserved_entry_init(void)
{
	zone_replenish_configure(vm_map_entry_reserved_zone);

	  
                                                                                       
    
	zone_replenish_configure(vm_map_holes_zone);
	vm_map_supports_hole_optimization = TRUE;
}

void
vm_map_disable_hole_optimization(vm_map_t map)
{
	vm_map_entry_t  head_entry, hole_entry, next_hole_entry;

	if (map->holelistenabled) {
		head_entry = hole_entry = CAST_TO_VM_MAP_ENTRY(map->holes_list);

		while (hole_entry != NULL) {
			next_hole_entry = hole_entry->vme_next;

			hole_entry->vme_next = NULL;
			hole_entry->vme_prev = NULL;
			zfree(vm_map_holes_zone, hole_entry);

			if (next_hole_entry == head_entry) {
				hole_entry = NULL;
			} else {
				hole_entry = next_hole_entry;
			}
		}

		map->holes_list = NULL;
		map->holelistenabled = FALSE;

		map->first_free = vm_map_first_entry(map);
		SAVE_HINT_HOLE_WRITE(map, NULL);
	}
}

boolean_t
vm_kernel_map_is_kernel(vm_map_t map)
{
	return map->pmap == kernel_pmap;
}

  
                 
  
                                              
                                               
                                            
   

vm_map_t
vm_map_create(
	pmap_t          pmap,
	vm_map_offset_t min,
	vm_map_offset_t max,
	boolean_t       pageable)
{
	int options;

	options = 0;
	if (pageable) {
		options |= VM_MAP_CREATE_PAGEABLE;
	}
	return vm_map_create_options(pmap, min, max, options);
}

vm_map_t
vm_map_create_options(
	pmap_t          pmap,
	vm_map_offset_t min,
	vm_map_offset_t max,
	int             options)
{
	vm_map_t        result;
	struct vm_map_links     *hole_entry = NULL;

	if (options & ~(VM_MAP_CREATE_ALL_OPTIONS)) {
		                    
		return VM_MAP_NULL;
	}

	result = (vm_map_t) zalloc(vm_map_zone);
	if (result == VM_MAP_NULL) {
		panic("vm_map_create");
	}

	vm_map_first_entry(result) = vm_map_to_entry(result);
	vm_map_last_entry(result)  = vm_map_to_entry(result);
	result->hdr.nentries = 0;
	if (options & VM_MAP_CREATE_PAGEABLE) {
		result->hdr.entries_pageable = TRUE;
	} else {
		result->hdr.entries_pageable = FALSE;
	}

	vm_map_store_init( &(result->hdr));

	result->hdr.page_shift = PAGE_SHIFT;

	result->size = 0;
	result->user_wire_limit = MACH_VM_MAX_ADDRESS;                                  
	result->user_wire_size  = 0;
#if XNU_TARGET_OS_OSX
	result->vmmap_high_start = 0;
#endif
	os_ref_init_count(&result->map_refcnt, &map_refgrp, 1);
	result->pmap = pmap;
	result->min_offset = min;
	result->max_offset = max;
	result->wiring_required = FALSE;
	result->no_zero_fill = FALSE;
	result->mapped_in_other_pmaps = FALSE;
	result->wait_for_space = FALSE;
	result->switch_protect = FALSE;
	result->disable_vmentry_reuse = FALSE;
	result->map_disallow_data_exec = FALSE;
	result->is_nested_map = FALSE;
	result->map_disallow_new_exec = FALSE;
	result->terminated = FALSE;
	result->cs_enforcement = FALSE;
	result->cs_debugged = FALSE;
	result->highest_entry_end = 0;
	result->first_free = vm_map_to_entry(result);
	result->hint = vm_map_to_entry(result);
	result->jit_entry_exists = FALSE;
	result->is_alien = FALSE;
	result->reserved_regions = FALSE;
	result->single_jit = FALSE;

	                                                                         
	if (options & VM_MAP_CREATE_CORPSE_FOOTPRINT) {
		result->has_corpse_footprint = TRUE;
		result->holelistenabled = FALSE;
		result->vmmap_corpse_footprint = NULL;
	} else {
		result->has_corpse_footprint = FALSE;
		if (vm_map_supports_hole_optimization) {
			hole_entry = zalloc(vm_map_holes_zone);

			hole_entry->start = min;
#if defined(__arm__) || defined(__arm64__)
			hole_entry->end = result->max_offset;
#else
			hole_entry->end = (max > (vm_map_offset_t)MACH_VM_MAX_ADDRESS) ? max : (vm_map_offset_t)MACH_VM_MAX_ADDRESS;
#endif
			result->holes_list = result->hole_hint = hole_entry;
			hole_entry->prev = hole_entry->next = CAST_TO_VM_MAP_ENTRY(hole_entry);
			result->holelistenabled = TRUE;
		} else {
			result->holelistenabled = FALSE;
		}
	}

	vm_map_lock_init(result);
	lck_mtx_init_ext(&result->s_lock, &result->s_lock_ext, &vm_map_lck_grp, &vm_map_lck_attr);

	return result;
}

vm_map_size_t
vm_map_adjusted_size(vm_map_t map)
{
	struct vm_reserved_region *regions = NULL;
	size_t num_regions = 0;
	mach_vm_size_t  reserved_size = 0, map_size = 0;

	if (map == NULL || (map->size == 0)) {
		return 0;
	}

	map_size = map->size;

	if (map->reserved_regions == FALSE || !vm_map_is_exotic(map) || map->terminated) {
		  
                                                                 
                                                                
                      
     
		return map_size;
	}

	num_regions = ml_get_vm_reserved_regions(vm_map_is_64bit(map), &regions);
	assert((num_regions == 0) || (num_regions > 0 && regions != NULL));

	while (num_regions) {
		reserved_size += regions[--num_regions].vmrr_size;
	}

	  
                                                                     
                                                                            
                                                                        
                                                                         
                                                                        
                                           
   
                                      
    

	return (map_size >= reserved_size) ? (map_size - reserved_size) : map_size;
}

  
                                             
  
                                                
                                                  
   
#define vm_map_entry_create(map, map_locked)    _vm_map_entry_create(&(map)->hdr, map_locked)

#define vm_map_copy_entry_create(copy, map_locked)                                      \
	_vm_map_entry_create(&(copy)->cpy_hdr, map_locked)
unsigned reserved_zalloc_count, nonreserved_zalloc_count;

static vm_map_entry_t
_vm_map_entry_create(
	struct vm_map_header    *map_header, boolean_t __unused map_locked)
{
	zone_t  zone;
	vm_map_entry_t  entry;

	zone = vm_map_entry_zone;

	assert(map_header->entries_pageable ? !map_locked : TRUE);

	if (map_header->entries_pageable) {
		entry = (vm_map_entry_t) zalloc(zone);
	} else {
		entry = (vm_map_entry_t) zalloc_noblock(zone);

		if (entry == VM_MAP_ENTRY_NULL) {
			zone = vm_map_entry_reserved_zone;
			entry = (vm_map_entry_t) zalloc(zone);
			OSAddAtomic(1, &reserved_zalloc_count);
		} else {
			OSAddAtomic(1, &nonreserved_zalloc_count);
		}
	}

	if (entry == VM_MAP_ENTRY_NULL) {
		panic("vm_map_entry_create");
	}
	*entry = vm_map_entry_template;
	entry->from_reserved_zone = (zone == vm_map_entry_reserved_zone);

	vm_map_store_update((vm_map_t) NULL, entry, VM_MAP_ENTRY_CREATE);
#if     MAP_ENTRY_CREATION_DEBUG
	entry->vme_creation_maphdr = map_header;
	backtrace(&entry->vme_creation_bt[0],
	    (sizeof(entry->vme_creation_bt) / sizeof(uintptr_t)), NULL);
#endif
	return entry;
}

  
                                              
  
                                  
  
                                         
                                            
                     
   
#define vm_map_entry_dispose(map, entry)                        \
	_vm_map_entry_dispose(&(map)->hdr, (entry))

#define vm_map_copy_entry_dispose(copy, entry) \
	_vm_map_entry_dispose(&(copy)->cpy_hdr, (entry))

static void
_vm_map_entry_dispose(
	struct vm_map_header    *map_header,
	vm_map_entry_t          entry)
{
	zone_t          zone;

	if (map_header->entries_pageable || !(entry->from_reserved_zone)) {
		zone = vm_map_entry_zone;
	} else {
		zone = vm_map_entry_reserved_zone;
	}

	if (!map_header->entries_pageable) {
		if (zone == vm_map_entry_zone) {
			OSAddAtomic(-1, &nonreserved_zalloc_count);
		} else {
			OSAddAtomic(-1, &reserved_zalloc_count);
		}
	}

	zfree(zone, entry);
}

#if MACH_ASSERT
static boolean_t first_free_check = FALSE;
boolean_t
first_free_is_valid(
	vm_map_t        map)
{
	if (!first_free_check) {
		return TRUE;
	}

	return first_free_is_valid_store( map );
}
#endif                  


#define vm_map_copy_entry_link(copy, after_where, entry)                \
	_vm_map_store_entry_link(&(copy)->cpy_hdr, after_where, (entry))

#define vm_map_copy_entry_unlink(copy, entry)                           \
	_vm_map_store_entry_unlink(&(copy)->cpy_hdr, (entry))

  
                  
  
                          
   
void
vm_map_destroy(
	vm_map_t        map,
	int             flags)
{
	vm_map_lock(map);

	                                                    
	flags |= VM_MAP_REMOVE_NO_UNNESTING;
	                                                    
	flags |= VM_MAP_REMOVE_IMMUTABLE;
	                                        
	flags |= VM_MAP_REMOVE_GAPS_OK;

	                                  
	(void) vm_map_delete(map, map->min_offset, map->max_offset,
	    flags, VM_MAP_NULL);
	                                                                         
#if     !defined(__arm__)
	(void) vm_map_delete(map, 0x0, 0xFFFFFFFFFFFFF000ULL,
	    flags, VM_MAP_NULL);
#endif               

	vm_map_disable_hole_optimization(map);
	vm_map_corpse_footprint_destroy(map);

	vm_map_unlock(map);

	assert(map->hdr.nentries == 0);

	if (map->pmap) {
		pmap_destroy(map->pmap);
	}

	if (vm_map_lck_attr.lck_attr_val & LCK_ATTR_DEBUG) {
		  
                                                                                 
                                                                           
                                             
                                                    
    
                                                                         
                                                                         
                                                                          
                                                  
    
                                                                                  
                                                                              
                                                                                    
                                                               
     
	} else {
		lck_rw_destroy(&(map)->lock, &vm_map_lck_grp);
		lck_mtx_destroy(&(map)->s_lock, &vm_map_lck_grp);
	}

	zfree(vm_map_zone, map);
}

  
                                                                     
                                               
   
pid_t
find_largest_process_vm_map_entries(void)
{
	pid_t victim_pid = -1;
	int max_vm_map_entries = 0;
	task_t task = TASK_NULL;
	queue_head_t *task_list = &tasks;

	lck_mtx_lock(&tasks_threads_lock);
	queue_iterate(task_list, task, task_t, tasks) {
		if (task == kernel_task || !task->active) {
			continue;
		}

		vm_map_t task_map = task->map;
		if (task_map != VM_MAP_NULL) {
			int task_vm_map_entries = task_map->hdr.nentries;
			if (task_vm_map_entries > max_vm_map_entries) {
				max_vm_map_entries = task_vm_map_entries;
				victim_pid = pid_from_task(task);
			}
		}
	}
	lck_mtx_unlock(&tasks_threads_lock);

	printf("zone_map_exhaustion: victim pid %d, vm region count: %d\n", victim_pid, max_vm_map_entries);
	return victim_pid;
}


  
                                             
  
                                                    
                                                  
                                                            
                                         
                                          
                                 
   
boolean_t
vm_map_lookup_entry(
	vm_map_t                map,
	vm_map_offset_t address,
	vm_map_entry_t          *entry)                  
{
	return vm_map_store_lookup_entry( map, address, entry );
}

  
                             
           
                                                          
                                                 
                            
  
                                                             
                                         
  
                                                      
                            
  
                                                                            
                                                                      
                                                 
   
kern_return_t
vm_map_find_space(
	vm_map_t                map,
	vm_map_offset_t         *address,                
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	vm_map_entry_t          *o_entry)                
{
	vm_map_entry_t          entry, new_entry, hole_entry;
	vm_map_offset_t         start;
	vm_map_offset_t         end;

	if (size == 0) {
		*address = 0;
		return KERN_INVALID_ARGUMENT;
	}

	new_entry = vm_map_entry_create(map, FALSE);
	vm_map_lock(map);

	if (flags & VM_MAP_FIND_LAST_FREE) {
		assert(!map->disable_vmentry_reuse);
		                                                                
		assert(!vmk_flags.vmkf_guard_after && !vmk_flags.vmkf_guard_before);
		assert(VM_MAP_PAGE_ALIGNED(size, VM_MAP_PAGE_MASK(map)));

		                                    
		vm_map_store_find_last_free(map, &entry);

		if (!entry) {
			goto noSpace;
		}

		if (entry == vm_map_to_entry(map)) {
			end = map->max_offset;
		} else {
			end = entry->vme_start;
		}

		while (TRUE) {
			vm_map_entry_t prev;

			start = end - size;

			if ((start < map->min_offset) || end < start) {
				goto noSpace;
			}

			prev = entry->vme_prev;
			entry = prev;

			if (prev == vm_map_to_entry(map)) {
				break;
			}

			if (prev->vme_end <= start) {
				break;
			}

			  
                                           
      

			end = entry->vme_start;
		}
	} else {
		if (vmk_flags.vmkf_guard_after) {
			                                                 
			size += VM_MAP_PAGE_SIZE(map);
		}

		  
                                                            
                                                          
     

		if (map->disable_vmentry_reuse == TRUE) {
			VM_MAP_HIGHEST_ENTRY(map, entry, start);
		} else {
			if (map->holelistenabled) {
				hole_entry = CAST_TO_VM_MAP_ENTRY(map->holes_list);

				if (hole_entry == NULL) {
					  
                                 
        
					goto noSpace;
				}

				entry = hole_entry;
				start = entry->vme_start;
			} else {
				assert(first_free_is_valid(map));
				if ((entry = map->first_free) == vm_map_to_entry(map)) {
					start = map->min_offset;
				} else {
					start = entry->vme_end;
				}
			}
		}

		  
                                             
                                                 
     

		while (TRUE) {
			vm_map_entry_t  next;

			  
                                              
                                             
                              
      

			if (vmk_flags.vmkf_guard_before) {
				                                            
				start += VM_MAP_PAGE_SIZE(map);
			}
			end = ((start + mask) & ~mask);

			if (end < start) {
				goto noSpace;
			}
			start = end;
			assert(VM_MAP_PAGE_ALIGNED(start, VM_MAP_PAGE_MASK(map)));
			end += size;
			assert(VM_MAP_PAGE_ALIGNED(end, VM_MAP_PAGE_MASK(map)));

			if ((end > map->max_offset) || (end < start)) {
				goto noSpace;
			}

			next = entry->vme_next;

			if (map->holelistenabled) {
				if (entry->vme_end >= end) {
					break;
				}
			} else {
				  
                                                 
      
         
      
                                            
                                                 
       

				if (next == vm_map_to_entry(map)) {
					break;
				}

				if (next->vme_start >= end) {
					break;
				}
			}

			  
                                           
      

			entry = next;

			if (map->holelistenabled) {
				if (entry == CAST_TO_VM_MAP_ENTRY(map->holes_list)) {
					  
                      
        
					goto noSpace;
				}
				start = entry->vme_start;
			} else {
				start = entry->vme_end;
			}
		}

		if (vmk_flags.vmkf_guard_before) {
			                                      
			start -= VM_MAP_PAGE_SIZE(map);
		}
	}

	if (map->holelistenabled) {
		if (vm_map_lookup_entry(map, entry->vme_start, &entry)) {
			panic("Found an existing entry (%p) instead of potential hole at address: 0x%llx.\n", entry, (unsigned long long)entry->vme_start);
		}
	}

	  
                  
                                                         
                              
                                                      
                
   
                              
    

	*address = start;

	assert(start < end);
	new_entry->vme_start = start;
	new_entry->vme_end = end;
	assert(page_aligned(new_entry->vme_start));
	assert(page_aligned(new_entry->vme_end));
	assert(VM_MAP_PAGE_ALIGNED(new_entry->vme_start,
	    VM_MAP_PAGE_MASK(map)));
	assert(VM_MAP_PAGE_ALIGNED(new_entry->vme_end,
	    VM_MAP_PAGE_MASK(map)));

	new_entry->is_shared = FALSE;
	new_entry->is_sub_map = FALSE;
	new_entry->use_pmap = TRUE;
	VME_OBJECT_SET(new_entry, VM_OBJECT_NULL);
	VME_OFFSET_SET(new_entry, (vm_object_offset_t) 0);

	new_entry->needs_copy = FALSE;

	new_entry->inheritance = VM_INHERIT_DEFAULT;
	new_entry->protection = VM_PROT_DEFAULT;
	new_entry->max_protection = VM_PROT_ALL;
	new_entry->behavior = VM_BEHAVIOR_DEFAULT;
	new_entry->wired_count = 0;
	new_entry->user_wired_count = 0;

	new_entry->in_transition = FALSE;
	new_entry->needs_wakeup = FALSE;
	new_entry->no_cache = FALSE;
	new_entry->permanent = FALSE;
	new_entry->superpage_size = FALSE;
	if (VM_MAP_PAGE_SHIFT(map) != PAGE_SHIFT) {
		new_entry->map_aligned = TRUE;
	} else {
		new_entry->map_aligned = FALSE;
	}

	new_entry->used_for_jit = FALSE;
	new_entry->pmap_cs_associated = FALSE;
	new_entry->zero_wired_pages = FALSE;
	new_entry->iokit_acct = FALSE;
	new_entry->vme_resilient_codesign = FALSE;
	new_entry->vme_resilient_media = FALSE;
	if (vmk_flags.vmkf_atomic_entry) {
		new_entry->vme_atomic = TRUE;
	} else {
		new_entry->vme_atomic = FALSE;
	}

	VME_ALIAS_SET(new_entry, tag);

	  
                                      
    

	vm_map_store_entry_link(map, entry, new_entry, VM_MAP_KERNEL_FLAGS_NONE);

	map->size += size;

	  
                          
    
	SAVE_HINT_MAP_WRITE(map, new_entry);

	*o_entry = new_entry;
	return KERN_SUCCESS;

noSpace:

	vm_map_entry_dispose(map, new_entry);
	vm_map_unlock(map);
	return KERN_NO_SPACE;
}

int vm_map_pmap_enter_print = FALSE;
int vm_map_pmap_enter_enable = FALSE;

  
                                             
  
               
                                                            
                                                          
                                                            
  
           
            
  
                     
                                                 
   
__unused static void
vm_map_pmap_enter(
	vm_map_t                map,
	vm_map_offset_t         addr,
	vm_map_offset_t         end_addr,
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_prot_t               protection)
{
	int                     type_of_fault;
	kern_return_t           kr;
	struct vm_object_fault_info fault_info = {};

	if (map->pmap == 0) {
		return;
	}

	assert(VM_MAP_PAGE_SHIFT(map) == PAGE_SHIFT);

	while (addr < end_addr) {
		vm_page_t       m;


		  
          
                                                                    
                                       
                                                       
                                                         
                                                           
                                               
     

		vm_object_lock(object);

		m = vm_page_lookup(object, offset);

		if (m == VM_PAGE_NULL || m->vmp_busy || m->vmp_fictitious ||
		    (m->vmp_unusual && (m->vmp_error || m->vmp_restart || m->vmp_absent))) {
			vm_object_unlock(object);
			return;
		}

		if (vm_map_pmap_enter_print) {
			printf("vm_map_pmap_enter:");
			printf("map: %p, addr: %llx, object: %p, offset: %llx\n",
			    map, (unsigned long long)addr, object, (unsigned long long)offset);
		}
		type_of_fault = DBG_CACHE_HIT_FAULT;
		kr = vm_fault_enter(m, map->pmap,
		    addr,
		    PAGE_SIZE, 0,
		    protection, protection,
		    VM_PAGE_WIRED(m),
		    FALSE,                                    
		    VM_KERN_MEMORY_NONE,                                       
		    &fault_info,
		    NULL,                                  
		    &type_of_fault);

		vm_object_unlock(object);

		offset += PAGE_SIZE_64;
		addr += PAGE_SIZE;
	}
}

boolean_t vm_map_pmap_is_empty(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end);
boolean_t
vm_map_pmap_is_empty(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end)
{
#ifdef MACHINE_PMAP_IS_EMPTY
	return pmap_is_empty(map->pmap, start, end);
#else                              
	vm_map_offset_t offset;
	ppnum_t         phys_page;

	if (map->pmap == NULL) {
		return TRUE;
	}

	for (offset = start;
	    offset < end;
	    offset += PAGE_SIZE) {
		phys_page = pmap_find_phys(map->pmap, offset);
		if (phys_page) {
			kprintf("vm_map_pmap_is_empty(%p,0x%llx,0x%llx): "
			    "page %d at 0x%llx\n",
			    map, (long long)start, (long long)end,
			    phys_page, (long long)offset);
			return FALSE;
		}
	}
	return TRUE;
#endif                             
}

#define MAX_TRIES_TO_GET_RANDOM_ADDRESS 1000
kern_return_t
vm_map_random_address_for_size(
	vm_map_t        map,
	vm_map_offset_t *address,
	vm_map_size_t   size)
{
	kern_return_t   kr = KERN_SUCCESS;
	int             tries = 0;
	vm_map_offset_t random_addr = 0;
	vm_map_offset_t hole_end;

	vm_map_entry_t  next_entry = VM_MAP_ENTRY_NULL;
	vm_map_entry_t  prev_entry = VM_MAP_ENTRY_NULL;
	vm_map_size_t   vm_hole_size = 0;
	vm_map_size_t   addr_space_size;

	addr_space_size = vm_map_max(map) - vm_map_min(map);

	assert(VM_MAP_PAGE_ALIGNED(size, VM_MAP_PAGE_MASK(map)));

	while (tries < MAX_TRIES_TO_GET_RANDOM_ADDRESS) {
		if (startup_phase < STARTUP_SUB_ZALLOC) {
			random_addr = (vm_map_offset_t)early_random();
		} else {
			random_addr = (vm_map_offset_t)random();
		}
		random_addr <<= VM_MAP_PAGE_SHIFT(map);
		random_addr = vm_map_trunc_page(
			vm_map_min(map) + (random_addr % addr_space_size),
			VM_MAP_PAGE_MASK(map));

		if (vm_map_lookup_entry(map, random_addr, &prev_entry) == FALSE) {
			if (prev_entry == vm_map_to_entry(map)) {
				next_entry = vm_map_first_entry(map);
			} else {
				next_entry = prev_entry->vme_next;
			}
			if (next_entry == vm_map_to_entry(map)) {
				hole_end = vm_map_max(map);
			} else {
				hole_end = next_entry->vme_start;
			}
			vm_hole_size = hole_end - random_addr;
			if (vm_hole_size >= size) {
				*address = random_addr;
				break;
			}
		}
		tries++;
	}

	if (tries == MAX_TRIES_TO_GET_RANDOM_ADDRESS) {
		kr = KERN_NO_SPACE;
	}
	return kr;
}

static boolean_t
vm_memory_malloc_no_cow(
	int alias)
{
	uint64_t alias_mask;

	if (alias > 63) {
		return FALSE;
	}

	alias_mask = 1ULL << alias;
	if (alias_mask & vm_memory_malloc_no_cow_mask) {
		return TRUE;
	}
	return FALSE;
}

  
                        
  
               
                                                          
                                                       
                                                        
  
                                                
   
static unsigned int vm_map_enter_restore_successes = 0;
static unsigned int vm_map_enter_restore_failures = 0;
kern_return_t
vm_map_enter(
	vm_map_t                map,
	vm_map_offset_t         *address,                   
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                alias,
	vm_object_t             object,
	vm_object_offset_t      offset,
	boolean_t               needs_copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	vm_map_entry_t          entry, new_entry;
	vm_map_offset_t         start, tmp_start, tmp_offset;
	vm_map_offset_t         end, tmp_end;
	vm_map_offset_t         tmp2_start, tmp2_end;
	vm_map_offset_t         desired_empty_end;
	vm_map_offset_t         step;
	kern_return_t           result = KERN_SUCCESS;
	vm_map_t                zap_old_map = VM_MAP_NULL;
	vm_map_t                zap_new_map = VM_MAP_NULL;
	boolean_t               map_locked = FALSE;
	boolean_t               pmap_empty = TRUE;
	boolean_t               new_mapping_established = FALSE;
	boolean_t               keep_map_locked = vmk_flags.vmkf_keep_map_locked;
	boolean_t               anywhere = ((flags & VM_FLAGS_ANYWHERE) != 0);
	boolean_t               purgable = ((flags & VM_FLAGS_PURGABLE) != 0);
	boolean_t               overwrite = ((flags & VM_FLAGS_OVERWRITE) != 0);
	boolean_t               no_cache = ((flags & VM_FLAGS_NO_CACHE) != 0);
	boolean_t               is_submap = vmk_flags.vmkf_submap;
	boolean_t               permanent = (((flags & VM_FLAGS_PERMANENT) != 0) || vmk_flags.vmkf_permanent);
	boolean_t               no_copy_on_read = vmk_flags.vmkf_no_copy_on_read;
	boolean_t               entry_for_jit = vmk_flags.vmkf_map_jit;
	boolean_t               iokit_acct = vmk_flags.vmkf_iokit_acct;
	boolean_t               translated_allow_execute = vmk_flags.vmkf_translated_allow_execute;
	boolean_t               resilient_codesign = ((flags & VM_FLAGS_RESILIENT_CODESIGN) != 0);
	boolean_t               resilient_media = ((flags & VM_FLAGS_RESILIENT_MEDIA) != 0);
	boolean_t               random_address = ((flags & VM_FLAGS_RANDOM_ADDR) != 0);
	unsigned int            superpage_size = ((flags & VM_FLAGS_SUPERPAGE_MASK) >> VM_FLAGS_SUPERPAGE_SHIFT);
	vm_tag_t                user_alias;
	vm_map_offset_t         effective_min_offset, effective_max_offset;
	kern_return_t           kr;
	boolean_t               clear_map_aligned = FALSE;
	vm_map_entry_t          hole_entry;
	vm_map_size_t           chunk_size = 0;

	assertf(vmk_flags.__vmkf_unused == 0, "vmk_flags unused=0x%x\n", vmk_flags.__vmkf_unused);

	if (flags & VM_FLAGS_4GB_CHUNK) {
#if defined(__LP64__)
		chunk_size = (4ULL * 1024 * 1024 * 1024);                                             
#else               
		chunk_size = ANON_CHUNK_SIZE;
#endif               
	} else {
		chunk_size = ANON_CHUNK_SIZE;
	}

	if (superpage_size) {
		switch (superpage_size) {
			  
                                                        
                                                       
                                                      
                                                        
                                                            
      
#ifdef __x86_64__
		case SUPERPAGE_SIZE_ANY:
			                                                   
			size = (size + 2 * 1024 * 1024 - 1) & ~(2 * 1024 * 1024 - 1);
			OS_FALLTHROUGH;
		case SUPERPAGE_SIZE_2MB:
			break;
#endif
		default:
			return KERN_INVALID_ARGUMENT;
		}
		mask = SUPERPAGE_SIZE - 1;
		if (size & (SUPERPAGE_SIZE - 1)) {
			return KERN_INVALID_ARGUMENT;
		}
		inheritance = VM_INHERIT_NONE;                                                
	}


	if ((cur_protection & VM_PROT_WRITE) &&
	    (cur_protection & VM_PROT_EXECUTE) &&
#if XNU_TARGET_OS_OSX
	    map->pmap != kernel_pmap &&
	    (cs_process_global_enforcement() ||
	    (vmk_flags.vmkf_cs_enforcement_override
	    ? vmk_flags.vmkf_cs_enforcement
	    : (vm_map_cs_enforcement(map)
#if __arm64__
	    || !VM_MAP_IS_EXOTIC(map)
#endif                
	    ))) &&
#endif                        
	    (VM_MAP_POLICY_WX_FAIL(map) ||
	    VM_MAP_POLICY_WX_STRIP_X(map)) &&
	    !entry_for_jit) {
		boolean_t vm_protect_wx_fail = VM_MAP_POLICY_WX_FAIL(map);

		DTRACE_VM3(cs_wx,
		    uint64_t, 0,
		    uint64_t, 0,
		    vm_prot_t, cur_protection);
		printf("CODE SIGNING: %d[%s] %s: curprot cannot be write+execute. %s\n",
		    proc_selfpid(),
		    (current_task()->bsd_info
		    ? proc_name_address(current_task()->bsd_info)
		    : "?"),
		    __FUNCTION__,
		    (vm_protect_wx_fail ? "failing" : "turning off execute"));
		cur_protection &= ~VM_PROT_EXECUTE;
		if (vm_protect_wx_fail) {
			return KERN_PROTECTION_FAILURE;
		}
	}

	  
                                                  
                                    
    
	if (map->map_disallow_new_exec == TRUE) {
		if (cur_protection & VM_PROT_EXECUTE) {
			return KERN_PROTECTION_FAILURE;
		}
	}

	if (resilient_codesign) {
		assert(!is_submap);
		int reject_prot = (needs_copy ? VM_PROT_EXECUTE : (VM_PROT_WRITE | VM_PROT_EXECUTE));
		if ((cur_protection | max_protection) & reject_prot) {
			return KERN_PROTECTION_FAILURE;
		}
	}

	if (resilient_media) {
		assert(!is_submap);
                        
		if (object != VM_OBJECT_NULL &&
		    !object->internal) {
			  
                                                    
                                                     
                                                
                                                 
                                                      
                                                    
                                                    
      
			return KERN_INVALID_ARGUMENT;
		}
	}

	if (is_submap) {
		if (purgable) {
			                                  
			return KERN_INVALID_ARGUMENT;
		}
		if (object == VM_OBJECT_NULL) {
			                                       
			return KERN_INVALID_ARGUMENT;
		}
	}
	if (vmk_flags.vmkf_already) {
		  
                                                           
                                                               
                                                                  
                                                            
                           
     
		if ((flags & VM_FLAGS_ANYWHERE) ||
		    (flags & VM_FLAGS_OVERWRITE)) {
			return KERN_INVALID_ARGUMENT;
		}
	}

	effective_min_offset = map->min_offset;

	if (vmk_flags.vmkf_beyond_max) {
		  
                                                    
     
#if     !defined(__arm__)
		if (vm_map_is_64bit(map)) {
			effective_max_offset = 0xFFFFFFFFFFFFF000ULL;
		} else
#endif               
		effective_max_offset = 0x00000000FFFFF000ULL;
	} else {
#if XNU_TARGET_OS_OSX
		if (__improbable(vmk_flags.vmkf_32bit_map_va)) {
			effective_max_offset = MIN(map->max_offset, 0x00000000FFFFF000ULL);
		} else {
			effective_max_offset = map->max_offset;
		}
#else                        
		effective_max_offset = map->max_offset;
#endif                        
	}

	if (size == 0 ||
	    (offset & MIN(VM_MAP_PAGE_MASK(map), PAGE_MASK_64)) != 0) {
		*address = 0;
		return KERN_INVALID_ARGUMENT;
	}

	if (map->pmap == kernel_pmap) {
		user_alias = VM_KERN_MEMORY_NONE;
	} else {
		user_alias = alias;
	}

	if (user_alias == VM_MEMORY_MALLOC_MEDIUM) {
		chunk_size = MALLOC_MEDIUM_CHUNK_SIZE;
	}

#define RETURN(value)   { result = value; goto BailOut; }

	assertf(VM_MAP_PAGE_ALIGNED(*address, FOURK_PAGE_MASK), "0x%llx", (uint64_t)*address);
	assertf(VM_MAP_PAGE_ALIGNED(size, FOURK_PAGE_MASK), "0x%llx", (uint64_t)size);
	if (VM_MAP_PAGE_MASK(map) >= PAGE_MASK) {
		assertf(page_aligned(*address), "0x%llx", (uint64_t)*address);
		assertf(page_aligned(size), "0x%llx", (uint64_t)size);
	}

	if (VM_MAP_PAGE_MASK(map) >= PAGE_MASK &&
	    !VM_MAP_PAGE_ALIGNED(size, VM_MAP_PAGE_MASK(map))) {
		  
                                                        
                     
                                                              
                                                         
                                                       
                                
     
		clear_map_aligned = TRUE;
	}
	if (!anywhere &&
	    VM_MAP_PAGE_MASK(map) >= PAGE_MASK &&
	    !VM_MAP_PAGE_ALIGNED(*address, VM_MAP_PAGE_MASK(map))) {
		  
                                                        
                                                            
                                                             
                                                               
                                                               
                                           
     
		clear_map_aligned = TRUE;
	}

	  
                                                      
                                                        
    
	if (purgable &&
	    (offset != 0 ||
	    (object != VM_OBJECT_NULL &&
	    (object->vo_size != size ||
	    object->purgable == VM_PURGABLE_DENY))
	    || size > ANON_MAX_SIZE)) {                                       
		return KERN_INVALID_ARGUMENT;
	}

	if (!anywhere && overwrite) {
		  
                                                              
                                               
                                             
                                               
                                                          
                                                            
                       
     
		zap_old_map = vm_map_create(PMAP_NULL,
		    *address,
		    *address + size,
		    map->hdr.entries_pageable);
		vm_map_set_page_shift(zap_old_map, VM_MAP_PAGE_SHIFT(map));
		vm_map_disable_hole_optimization(zap_old_map);
	}

StartAgain:;

	start = *address;

	if (anywhere) {
		vm_map_lock(map);
		map_locked = TRUE;

		if (entry_for_jit) {
			if (map->jit_entry_exists &&
			    !VM_MAP_POLICY_ALLOW_MULTIPLE_JIT(map)) {
				result = KERN_INVALID_ARGUMENT;
				goto BailOut;
			}
			if (VM_MAP_POLICY_ALLOW_JIT_RANDOM_ADDRESS(map)) {
				random_address = TRUE;
			}
		}

		if (random_address) {
			  
                                 
      
			result = vm_map_random_address_for_size(map, address, size);
			if (result != KERN_SUCCESS) {
				goto BailOut;
			}
			start = *address;
		}
#if XNU_TARGET_OS_OSX
		else if ((start == 0 || start == vm_map_min(map)) &&
		    !map->disable_vmentry_reuse &&
		    map->vmmap_high_start != 0) {
			start = map->vmmap_high_start;
		}
#endif                        


		  
                                          
     

		if (start < effective_min_offset) {
			start = effective_min_offset;
		}
		if (start > effective_max_offset) {
			RETURN(KERN_NO_SPACE);
		}

		  
                                         
                                         
                                        
     

		if (map->disable_vmentry_reuse == TRUE) {
			VM_MAP_HIGHEST_ENTRY(map, entry, start);
		} else {
			if (map->holelistenabled) {
				hole_entry = CAST_TO_VM_MAP_ENTRY(map->holes_list);

				if (hole_entry == NULL) {
					  
                                 
        
					result = KERN_NO_SPACE;
					goto BailOut;
				} else {
					boolean_t found_hole = FALSE;

					do {
						if (hole_entry->vme_start >= start) {
							start = hole_entry->vme_start;
							found_hole = TRUE;
							break;
						}

						if (hole_entry->vme_end > start) {
							found_hole = TRUE;
							break;
						}
						hole_entry = hole_entry->vme_next;
					} while (hole_entry != CAST_TO_VM_MAP_ENTRY(map->holes_list));

					if (found_hole == FALSE) {
						result = KERN_NO_SPACE;
						goto BailOut;
					}

					entry = hole_entry;

					if (start == 0) {
						start += PAGE_SIZE_64;
					}
				}
			} else {
				assert(first_free_is_valid(map));

				entry = map->first_free;

				if (entry == vm_map_to_entry(map)) {
					entry = NULL;
				} else {
					if (entry->vme_next == vm_map_to_entry(map)) {
						  
                                    
         
						entry = NULL;
					} else {
						if (start < (entry->vme_next)->vme_start) {
							start = entry->vme_end;
							start = vm_map_round_page(start,
							    VM_MAP_PAGE_MASK(map));
						} else {
							  
                              
          
							entry = NULL;
						}
					}
				}

				if (entry == NULL) {
					vm_map_entry_t  tmp_entry;
					if (vm_map_lookup_entry(map, start, &tmp_entry)) {
						assert(!entry_for_jit);
						start = tmp_entry->vme_end;
						start = vm_map_round_page(start,
						    VM_MAP_PAGE_MASK(map));
					}
					entry = tmp_entry;
				}
			}
		}

		  
                                             
                                           
          
     

		while (TRUE) {
			vm_map_entry_t  next;

			  
                                              
                                             
                              
      

			end = ((start + mask) & ~mask);
			end = vm_map_round_page(end,
			    VM_MAP_PAGE_MASK(map));
			if (end < start) {
				RETURN(KERN_NO_SPACE);
			}
			start = end;
			assert(VM_MAP_PAGE_ALIGNED(start,
			    VM_MAP_PAGE_MASK(map)));
			end += size;

			                                                                                    
			desired_empty_end = vm_map_round_page(end, VM_MAP_PAGE_MASK(map));

			if ((desired_empty_end > effective_max_offset) || (desired_empty_end < start)) {
				if (map->wait_for_space) {
					assert(!keep_map_locked);
					if (size <= (effective_max_offset -
					    effective_min_offset)) {
						assert_wait((event_t)map,
						    THREAD_ABORTSAFE);
						vm_map_unlock(map);
						map_locked = FALSE;
						thread_block(THREAD_CONTINUE_NULL);
						goto StartAgain;
					}
				}
				RETURN(KERN_NO_SPACE);
			}

			next = entry->vme_next;

			if (map->holelistenabled) {
				if (entry->vme_end >= desired_empty_end) {
					break;
				}
			} else {
				  
                                                 
      
         
      
                                            
                                                 
       

				if (next == vm_map_to_entry(map)) {
					break;
				}

				if (next->vme_start >= desired_empty_end) {
					break;
				}
			}

			  
                                           
      

			entry = next;

			if (map->holelistenabled) {
				if (entry == CAST_TO_VM_MAP_ENTRY(map->holes_list)) {
					  
                      
        
					result = KERN_NO_SPACE;
					goto BailOut;
				}
				start = entry->vme_start;
			} else {
				start = entry->vme_end;
			}

			start = vm_map_round_page(start,
			    VM_MAP_PAGE_MASK(map));
		}

		if (map->holelistenabled) {
			if (vm_map_lookup_entry(map, entry->vme_start, &entry)) {
				panic("Found an existing entry (%p) instead of potential hole at address: 0x%llx.\n", entry, (unsigned long long)entry->vme_start);
			}
		}

		*address = start;
		assert(VM_MAP_PAGE_ALIGNED(*address,
		    VM_MAP_PAGE_MASK(map)));
	} else {
		if (VM_MAP_PAGE_SHIFT(map) < PAGE_SHIFT &&
		    !overwrite &&
		    user_alias == VM_MEMORY_REALLOC) {
			  
                                                    
                                              
      
                                          
			return KERN_NO_SPACE;
		}

		  
                 
                                        
                           
     

		vm_map_lock(map);
		map_locked = TRUE;
		if ((start & mask) != 0) {
			RETURN(KERN_NO_SPACE);
		}

		  
                                     
     

		end = start + size;

		if ((start < effective_min_offset) ||
		    (end > effective_max_offset) ||
		    (start >= end)) {
			RETURN(KERN_INVALID_ADDRESS);
		}

		if (overwrite && zap_old_map != VM_MAP_NULL) {
			int remove_flags;
			  
                                                    
                                                   
                                                      
      
			remove_flags = VM_MAP_REMOVE_SAVE_ENTRIES;
			remove_flags |= VM_MAP_REMOVE_NO_MAP_ALIGN;
			if (vmk_flags.vmkf_overwrite_immutable) {
				                                         
				remove_flags |= VM_MAP_REMOVE_IMMUTABLE;
			}
			(void) vm_map_delete(map, start, end,
			    remove_flags,
			    zap_old_map);
		}

		  
                                             
     

		if (vm_map_lookup_entry(map, start, &entry)) {
			if (!(vmk_flags.vmkf_already)) {
				RETURN(KERN_NO_SPACE);
			}
			  
                                                    
      
			tmp_start = start;
			tmp_offset = offset;
			if (entry->vme_start < start) {
				tmp_start -= start - entry->vme_start;
				tmp_offset -= start - entry->vme_start;
			}
			for (; entry->vme_start < end;
			    entry = entry->vme_next) {
				  
                                        
                                    
       
				if (entry == vm_map_to_entry(map) ||
				    entry->vme_start != tmp_start ||
				    entry->is_sub_map != is_submap ||
				    VME_OFFSET(entry) != tmp_offset ||
				    entry->needs_copy != needs_copy ||
				    entry->protection != cur_protection ||
				    entry->max_protection != max_protection ||
				    entry->inheritance != inheritance ||
				    entry->iokit_acct != iokit_acct ||
				    VME_ALIAS(entry) != alias) {
					                            
					RETURN(KERN_NO_SPACE);
				}
				  
                                                
       
				if (is_submap) {
					if (VME_SUBMAP(entry) !=
					    (vm_map_t) object) {
						                         
						RETURN(KERN_NO_SPACE);
					}
				} else {
					if (VME_OBJECT(entry) != object) {
						                               
						vm_object_t obj2;

						obj2 = VME_OBJECT(entry);
						if ((obj2 == VM_OBJECT_NULL ||
						    obj2->internal) &&
						    (object == VM_OBJECT_NULL ||
						    object->internal)) {
							  
                          
                           
                        
          
						} else {
							RETURN(KERN_NO_SPACE);
						}
					}
				}

				tmp_offset += entry->vme_end - entry->vme_start;
				tmp_start += entry->vme_end - entry->vme_start;
				if (entry->vme_end >= end) {
					                                    
					break;
				}
			}
			                                                       
			RETURN(KERN_MEMORY_PRESENT);
		}

		  
                                            
                
     

		if ((entry->vme_next != vm_map_to_entry(map)) &&
		    (entry->vme_next->vme_start < end)) {
			RETURN(KERN_NO_SPACE);
		}
	}

	  
                  
                                                         
                              
                                                      
                
   
                              
    

	  
                                                                 
                                                                
                                                           
                                                         
                                                          
              
    

	if (purgable ||
	    entry_for_jit ||
	    vm_memory_malloc_no_cow(user_alias)) {
		if (object == VM_OBJECT_NULL) {
			object = vm_object_allocate(size);
			object->copy_strategy = MEMORY_OBJECT_COPY_NONE;
			object->true_share = FALSE;
			if (purgable) {
				task_t owner;
				object->purgable = VM_PURGABLE_NONVOLATILE;
				if (map->pmap == kernel_pmap) {
					  
                                           
                                            
                                         
                                            
                                     
                                          
                 
        
					owner = kernel_task;
				} else {
					owner = current_task();
				}
				assert(object->vo_owner == NULL);
				assert(object->resident_page_count == 0);
				assert(object->wired_page_count == 0);
				vm_object_lock(object);
				vm_purgeable_nonvolatile_enqueue(object, owner);
				vm_object_unlock(object);
			}
			offset = (vm_object_offset_t)0;
		}
	} else if (VM_MAP_PAGE_SHIFT(map) < PAGE_SHIFT) {
		                                                   
	} else if ((is_submap == FALSE) &&
	    (object == VM_OBJECT_NULL) &&
	    (entry != vm_map_to_entry(map)) &&
	    (entry->vme_end == start) &&
	    (!entry->is_shared) &&
	    (!entry->is_sub_map) &&
	    (!entry->in_transition) &&
	    (!entry->needs_wakeup) &&
	    (entry->behavior == VM_BEHAVIOR_DEFAULT) &&
	    (entry->protection == cur_protection) &&
	    (entry->max_protection == max_protection) &&
	    (entry->inheritance == inheritance) &&
	    ((user_alias == VM_MEMORY_REALLOC) ||
	    (VME_ALIAS(entry) == alias)) &&
	    (entry->no_cache == no_cache) &&
	    (entry->permanent == permanent) &&
	                                                         
	    !((entry->protection & VM_PROT_EXECUTE) &&
	    entry->permanent) &&
	    (!entry->superpage_size && !superpage_size) &&
	      
                                                              
                                               
        
	    (!entry->map_aligned || !clear_map_aligned) &&
	    (!entry->zero_wired_pages) &&
	    (!entry->used_for_jit && !entry_for_jit) &&
	    (!entry->pmap_cs_associated) &&
	    (entry->iokit_acct == iokit_acct) &&
	    (!entry->vme_resilient_codesign) &&
	    (!entry->vme_resilient_media) &&
	    (!entry->vme_atomic) &&
	    (entry->vme_no_copy_on_read == no_copy_on_read) &&

	    ((entry->vme_end - entry->vme_start) + size <=
	    (user_alias == VM_MEMORY_REALLOC ?
	    ANON_CHUNK_SIZE :
	    NO_COALESCE_LIMIT)) &&

	    (entry->wired_count == 0)) {                                           
		if (vm_object_coalesce(VME_OBJECT(entry),
		    VM_OBJECT_NULL,
		    VME_OFFSET(entry),
		    (vm_object_offset_t) 0,
		    (vm_map_size_t)(entry->vme_end - entry->vme_start),
		    (vm_map_size_t)(end - entry->vme_end))) {
			  
                                            
                                           
                
      
			map->size += (end - entry->vme_end);
			assert(entry->vme_start < end);
			assert(VM_MAP_PAGE_ALIGNED(end,
			    VM_MAP_PAGE_MASK(map)));
			if (__improbable(vm_debug_events)) {
				DTRACE_VM5(map_entry_extend, vm_map_t, map, vm_map_entry_t, entry, vm_address_t, entry->vme_start, vm_address_t, entry->vme_end, vm_address_t, end);
			}
			entry->vme_end = end;
			if (map->holelistenabled) {
				vm_map_store_update_first_free(map, entry, TRUE);
			} else {
				vm_map_store_update_first_free(map, map->first_free, TRUE);
			}
			new_mapping_established = TRUE;
			RETURN(KERN_SUCCESS);
		}
	}

	step = superpage_size ? SUPERPAGE_SIZE : (end - start);
	new_entry = NULL;

	for (tmp2_start = start; tmp2_start < end; tmp2_start += step) {
		tmp2_end = tmp2_start + step;
		  
                       
    
             
                                                                 
                                                                  
                                                                  
                                                                
                                                                
                                                                   
                   
     
		tmp_start = tmp2_start;
		if (object == VM_OBJECT_NULL &&
		    size > chunk_size &&
		    max_protection != VM_PROT_NONE &&
		    superpage_size == 0) {
			tmp_end = tmp_start + chunk_size;
		} else {
			tmp_end = tmp2_end;
		}
		do {
			new_entry = vm_map_entry_insert(map,
			    entry, tmp_start, tmp_end,
			    object, offset, vmk_flags,
			    needs_copy, FALSE, FALSE,
			    cur_protection, max_protection,
			    VM_BEHAVIOR_DEFAULT,
			    (entry_for_jit && !VM_MAP_POLICY_ALLOW_JIT_INHERIT(map) ?
			    VM_INHERIT_NONE : inheritance),
			    0,
			    no_cache,
			    permanent,
			    no_copy_on_read,
			    superpage_size,
			    clear_map_aligned,
			    is_submap,
			    entry_for_jit,
			    alias,
			    translated_allow_execute);

			assert((object != kernel_object) || (VM_KERN_MEMORY_NONE != alias));

			if (resilient_codesign) {
				int reject_prot = (needs_copy ? VM_PROT_EXECUTE : (VM_PROT_WRITE | VM_PROT_EXECUTE));
				if (!((cur_protection | max_protection) & reject_prot)) {
					new_entry->vme_resilient_codesign = TRUE;
				}
			}

			if (resilient_media &&
			    (object == VM_OBJECT_NULL ||
			    object->internal)) {
				new_entry->vme_resilient_media = TRUE;
			}

			assert(!new_entry->iokit_acct);
			if (!is_submap &&
			    object != VM_OBJECT_NULL &&
			    (object->purgable != VM_PURGABLE_DENY ||
			    object->vo_ledger_tag)) {
				assert(new_entry->use_pmap);
				assert(!new_entry->iokit_acct);
				  
                                     
                                               
                   
       
				new_entry->use_pmap = FALSE;
			} else if (!is_submap &&
			    iokit_acct &&
			    object != VM_OBJECT_NULL &&
			    object->internal) {
				                          
				assert(!new_entry->iokit_acct);
				assert(new_entry->use_pmap);
				new_entry->iokit_acct = TRUE;
				new_entry->use_pmap = FALSE;
				DTRACE_VM4(
					vm_map_iokit_mapped_region,
					vm_map_t, map,
					vm_map_offset_t, new_entry->vme_start,
					vm_map_offset_t, new_entry->vme_end,
					int, VME_ALIAS(new_entry));
				vm_map_iokit_mapped_region(
					map,
					(new_entry->vme_end -
					new_entry->vme_start));
			} else if (!is_submap) {
				assert(!new_entry->iokit_acct);
				assert(new_entry->use_pmap);
			}

			if (is_submap) {
				vm_map_t        submap;
				boolean_t       submap_is_64bit;
				boolean_t       use_pmap;

				assert(new_entry->is_sub_map);
				assert(!new_entry->use_pmap);
				assert(!new_entry->iokit_acct);
				submap = (vm_map_t) object;
				submap_is_64bit = vm_map_is_64bit(submap);
				use_pmap = vmk_flags.vmkf_nested_pmap;
#ifndef NO_NESTED_PMAP
				if (use_pmap && submap->pmap == NULL) {
					ledger_t ledger = map->pmap->ledger;
					                                   
					submap->pmap = pmap_create_options(ledger, 0,
					    submap_is_64bit ? PMAP_CREATE_64BIT : 0);
					if (submap->pmap == NULL) {
						                                      
					}
#if     defined(__arm__) || defined(__arm64__)
					else {
						pmap_set_nested(submap->pmap);
					}
#endif
				}
				if (use_pmap && submap->pmap != NULL) {
					if (VM_MAP_PAGE_SHIFT(map) != VM_MAP_PAGE_SHIFT(submap)) {
						DEBUG4K_ERROR("map %p (%d) submap %p (%d): incompatible page sizes\n", map, VM_MAP_PAGE_SHIFT(map), submap, VM_MAP_PAGE_SHIFT(submap));
						kr = KERN_FAILURE;
					} else {
						kr = pmap_nest(map->pmap,
						    submap->pmap,
						    tmp_start,
						    tmp_end - tmp_start);
					}
					if (kr != KERN_SUCCESS) {
						printf("vm_map_enter: "
						    "pmap_nest(0x%llx,0x%llx) "
						    "error 0x%x\n",
						    (long long)tmp_start,
						    (long long)tmp_end,
						    kr);
					} else {
						                        
						new_entry->use_pmap = TRUE;
						pmap_empty = FALSE;
					}
				}
#endif                     
			}
			entry = new_entry;

			if (superpage_size) {
				vm_page_t pages, m;
				vm_object_t sp_object;
				vm_object_offset_t sp_offset;

				VME_OFFSET_SET(entry, 0);

				                            
				kr = cpm_allocate(SUPERPAGE_SIZE, &pages, 0, SUPERPAGE_NBASEPAGES - 1, TRUE, 0);
				if (kr != KERN_SUCCESS) {
					                               
					new_mapping_established = TRUE;
					                                  
					size -= end - tmp_end;
					RETURN(kr);
				}

				                                        
				sp_object = vm_object_allocate((vm_map_size_t)(entry->vme_end - entry->vme_start));
				sp_object->phys_contiguous = TRUE;
				sp_object->vo_shadow_offset = (vm_object_offset_t)VM_PAGE_GET_PHYS_PAGE(pages) * PAGE_SIZE;
				VME_OBJECT_SET(entry, sp_object);
				assert(entry->use_pmap);

				                                          
				vm_object_lock(sp_object);
				for (sp_offset = 0;
				    sp_offset < SUPERPAGE_SIZE;
				    sp_offset += PAGE_SIZE) {
					m = pages;
					pmap_zero_page(VM_PAGE_GET_PHYS_PAGE(m));
					pages = NEXT_PAGE(m);
					*(NEXT_PAGE_PTR(m)) = VM_PAGE_NULL;
					vm_page_insert_wired(m, sp_object, sp_offset, VM_KERN_MEMORY_OSFMK);
				}
				vm_object_unlock(sp_object);
			}
		} while (tmp_end != tmp2_end &&
		    (tmp_start = tmp_end) &&
		    (tmp_end = (tmp2_end - tmp_end > chunk_size) ?
		    tmp_end + chunk_size : tmp2_end));
	}

	new_mapping_established = TRUE;

BailOut:
	assert(map_locked == TRUE);

	if (result == KERN_SUCCESS) {
		vm_prot_t pager_prot;
		memory_object_t pager;

#if DEBUG
		if (pmap_empty &&
		    !(vmk_flags.vmkf_no_pmap_check)) {
			assert(vm_map_pmap_is_empty(map,
			    *address,
			    *address + size));
		}
#endif            

		  
                                                        
                                                             
                                                            
                         
                                                              
                                              
                                                               
     
		pager_prot = max_protection;
		if (needs_copy) {
			  
                                         
                        
      
			pager_prot &= ~VM_PROT_WRITE;
		}
		if (!is_submap &&
		    object != VM_OBJECT_NULL &&
		    object->named &&
		    object->pager != MEMORY_OBJECT_NULL) {
			vm_object_lock(object);
			pager = object->pager;
			if (object->named &&
			    pager != MEMORY_OBJECT_NULL) {
				assert(object->pager_ready);
				vm_object_mapping_wait(object, THREAD_UNINT);
				vm_object_mapping_begin(object);
				vm_object_unlock(object);

				kr = memory_object_map(pager, pager_prot);
				assert(kr == KERN_SUCCESS);

				vm_object_lock(object);
				vm_object_mapping_end(object);
			}
			vm_object_unlock(object);
		}
	}

	assert(map_locked == TRUE);

	if (!keep_map_locked) {
		vm_map_unlock(map);
		map_locked = FALSE;
	}

	  
                                                      
    

	if (result == KERN_SUCCESS) {
		                                      
                                            
     
		if ((map->wiring_required) || (superpage_size)) {
			assert(!keep_map_locked);
			pmap_empty = FALSE;                          
			kr = vm_map_wire_kernel(map, start, end,
			    new_entry->protection, VM_KERN_MEMORY_MLOCK,
			    TRUE);
			result = kr;
		}

	}

	if (result != KERN_SUCCESS) {
		if (new_mapping_established) {
			  
                                                     
                                            
                                                      
                                                       
      
			zap_new_map = vm_map_create(PMAP_NULL,
			    *address,
			    *address + size,
			    map->hdr.entries_pageable);
			vm_map_set_page_shift(zap_new_map,
			    VM_MAP_PAGE_SHIFT(map));
			vm_map_disable_hole_optimization(zap_new_map);

			if (!map_locked) {
				vm_map_lock(map);
				map_locked = TRUE;
			}
			(void) vm_map_delete(map, *address, *address + size,
			    (VM_MAP_REMOVE_SAVE_ENTRIES |
			    VM_MAP_REMOVE_NO_MAP_ALIGN),
			    zap_new_map);
		}
		if (zap_old_map != VM_MAP_NULL &&
		    zap_old_map->hdr.nentries != 0) {
			vm_map_entry_t  entry1, entry2;

			  
                                                 
                                                   
      
			if (!map_locked) {
				vm_map_lock(map);
				map_locked = TRUE;
			}

			                                             
			start = vm_map_first_entry(zap_old_map)->vme_start;
			end = vm_map_last_entry(zap_old_map)->vme_end;
			if (vm_map_lookup_entry(map, start, &entry1) ||
			    vm_map_lookup_entry(map, end, &entry2) ||
			    entry1 != entry2) {
				  
                                          
                                           
                  
       
				vm_map_enter_restore_failures++;
			} else {
				  
                                          
                                           
                                         
       
				for (entry2 = vm_map_first_entry(zap_old_map);
				    entry2 != vm_map_to_entry(zap_old_map);
				    entry2 = vm_map_first_entry(zap_old_map)) {
					vm_map_size_t entry_size;

					entry_size = (entry2->vme_end -
					    entry2->vme_start);
					vm_map_store_entry_unlink(zap_old_map,
					    entry2);
					zap_old_map->size -= entry_size;
					vm_map_store_entry_link(map, entry1, entry2,
					    VM_MAP_KERNEL_FLAGS_NONE);
					map->size += entry_size;
					entry1 = entry2;
				}
				if (map->wiring_required) {
					  
                                      
                         
        
				}
				vm_map_enter_restore_successes++;
			}
		}
	}

	  
                                                                       
                        
    
	if (map_locked && !keep_map_locked) {
		vm_map_unlock(map);
	}

	  
                                                          
                           
    
	if (zap_old_map != VM_MAP_NULL) {
		vm_map_destroy(zap_old_map, VM_MAP_REMOVE_NO_PMAP_CLEANUP);
		zap_old_map = VM_MAP_NULL;
	}
	if (zap_new_map != VM_MAP_NULL) {
		vm_map_destroy(zap_new_map, VM_MAP_REMOVE_NO_PMAP_CLEANUP);
		zap_new_map = VM_MAP_NULL;
	}

	return result;

#undef  RETURN
}

#if __arm64__
extern const struct memory_object_pager_ops fourk_pager_ops;
kern_return_t
vm_map_enter_fourk(
	vm_map_t                map,
	vm_map_offset_t         *address,                   
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                alias,
	vm_object_t             object,
	vm_object_offset_t      offset,
	boolean_t               needs_copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	vm_map_entry_t          entry, new_entry;
	vm_map_offset_t         start, fourk_start;
	vm_map_offset_t         end, fourk_end;
	vm_map_size_t           fourk_size;
	kern_return_t           result = KERN_SUCCESS;
	vm_map_t                zap_old_map = VM_MAP_NULL;
	vm_map_t                zap_new_map = VM_MAP_NULL;
	boolean_t               map_locked = FALSE;
	boolean_t               pmap_empty = TRUE;
	boolean_t               new_mapping_established = FALSE;
	boolean_t               keep_map_locked = vmk_flags.vmkf_keep_map_locked;
	boolean_t               anywhere = ((flags & VM_FLAGS_ANYWHERE) != 0);
	boolean_t               purgable = ((flags & VM_FLAGS_PURGABLE) != 0);
	boolean_t               overwrite = ((flags & VM_FLAGS_OVERWRITE) != 0);
	boolean_t               no_cache = ((flags & VM_FLAGS_NO_CACHE) != 0);
	boolean_t               is_submap = vmk_flags.vmkf_submap;
	boolean_t               permanent = vmk_flags.vmkf_permanent;
	boolean_t               no_copy_on_read = vmk_flags.vmkf_permanent;
	boolean_t               entry_for_jit = vmk_flags.vmkf_map_jit;
                                                     
	boolean_t               translated_allow_execute = vmk_flags.vmkf_translated_allow_execute;
	unsigned int            superpage_size = ((flags & VM_FLAGS_SUPERPAGE_MASK) >> VM_FLAGS_SUPERPAGE_SHIFT);
	vm_map_offset_t         effective_min_offset, effective_max_offset;
	kern_return_t           kr;
	boolean_t               clear_map_aligned = FALSE;
	memory_object_t         fourk_mem_obj;
	vm_object_t             fourk_object;
	vm_map_offset_t         fourk_pager_offset;
	int                     fourk_pager_index_start, fourk_pager_index_num;
	int                     cur_idx;
	boolean_t               fourk_copy;
	vm_object_t             copy_object;
	vm_object_offset_t      copy_offset;

	if (VM_MAP_PAGE_MASK(map) < PAGE_MASK) {
		panic("%s:%d\n", __FUNCTION__, __LINE__);
	}
	fourk_mem_obj = MEMORY_OBJECT_NULL;
	fourk_object = VM_OBJECT_NULL;

	if (superpage_size) {
		return KERN_NOT_SUPPORTED;
	}

	if ((cur_protection & VM_PROT_WRITE) &&
	    (cur_protection & VM_PROT_EXECUTE) &&
#if XNU_TARGET_OS_OSX
	    map->pmap != kernel_pmap &&
	    (vm_map_cs_enforcement(map)
#if __arm64__
	    || !VM_MAP_IS_EXOTIC(map)
#endif                
	    ) &&
#endif                        
	    !entry_for_jit) {
		DTRACE_VM3(cs_wx,
		    uint64_t, 0,
		    uint64_t, 0,
		    vm_prot_t, cur_protection);
		printf("CODE SIGNING: %d[%s] %s: curprot cannot be write+execute. "
		    "turning off execute\n",
		    proc_selfpid(),
		    (current_task()->bsd_info
		    ? proc_name_address(current_task()->bsd_info)
		    : "?"),
		    __FUNCTION__);
		cur_protection &= ~VM_PROT_EXECUTE;
	}

	  
                                                  
                                    
    
	if (map->map_disallow_new_exec == TRUE) {
		if (cur_protection & VM_PROT_EXECUTE) {
			return KERN_PROTECTION_FAILURE;
		}
	}

	if (is_submap) {
		return KERN_NOT_SUPPORTED;
	}
	if (vmk_flags.vmkf_already) {
		return KERN_NOT_SUPPORTED;
	}
	if (purgable || entry_for_jit) {
		return KERN_NOT_SUPPORTED;
	}

	effective_min_offset = map->min_offset;

	if (vmk_flags.vmkf_beyond_max) {
		return KERN_NOT_SUPPORTED;
	} else {
		effective_max_offset = map->max_offset;
	}

	if (size == 0 ||
	    (offset & FOURK_PAGE_MASK) != 0) {
		*address = 0;
		return KERN_INVALID_ARGUMENT;
	}

#define RETURN(value)   { result = value; goto BailOut; }

	assert(VM_MAP_PAGE_ALIGNED(*address, FOURK_PAGE_MASK));
	assert(VM_MAP_PAGE_ALIGNED(size, FOURK_PAGE_MASK));

	if (!anywhere && overwrite) {
		return KERN_NOT_SUPPORTED;
	}
	if (!anywhere && overwrite) {
		  
                                                              
                                               
                                             
                                               
                                                          
                                                            
                       
     
		zap_old_map = vm_map_create(PMAP_NULL,
		    *address,
		    *address + size,
		    map->hdr.entries_pageable);
		vm_map_set_page_shift(zap_old_map, VM_MAP_PAGE_SHIFT(map));
		vm_map_disable_hole_optimization(zap_old_map);
	}

	fourk_start = *address;
	fourk_size = size;
	fourk_end = fourk_start + fourk_size;

	start = vm_map_trunc_page(*address, VM_MAP_PAGE_MASK(map));
	end = vm_map_round_page(fourk_end, VM_MAP_PAGE_MASK(map));
	size = end - start;

	if (anywhere) {
		return KERN_NOT_SUPPORTED;
	} else {
		  
                 
                                        
                           
     

		vm_map_lock(map);
		map_locked = TRUE;
		if ((start & mask) != 0) {
			RETURN(KERN_NO_SPACE);
		}

		  
                                     
     

		end = start + size;

		if ((start < effective_min_offset) ||
		    (end > effective_max_offset) ||
		    (start >= end)) {
			RETURN(KERN_INVALID_ADDRESS);
		}

		if (overwrite && zap_old_map != VM_MAP_NULL) {
			  
                                                    
                                                   
                                                      
      
			(void) vm_map_delete(map, start, end,
			    (VM_MAP_REMOVE_SAVE_ENTRIES |
			    VM_MAP_REMOVE_NO_MAP_ALIGN),
			    zap_old_map);
		}

		  
                                             
     
		if (vm_map_lookup_entry(map, start, &entry)) {
			vm_object_t cur_object, shadow_object;

			  
                                       
                         
      

			if (entry->vme_end - entry->vme_start
			    != SIXTEENK_PAGE_SIZE) {
				RETURN(KERN_NO_SPACE);
			}
			if (entry->is_sub_map) {
				RETURN(KERN_NO_SPACE);
			}
			if (VME_OBJECT(entry) == VM_OBJECT_NULL) {
				RETURN(KERN_NO_SPACE);
			}

			                                          
			cur_object = VME_OBJECT(entry);
			vm_object_lock(cur_object);
			while (cur_object->shadow != VM_OBJECT_NULL) {
				shadow_object = cur_object->shadow;
				vm_object_lock(shadow_object);
				vm_object_unlock(cur_object);
				cur_object = shadow_object;
				shadow_object = VM_OBJECT_NULL;
			}
			if (cur_object->internal ||
			    cur_object->pager == NULL) {
				vm_object_unlock(cur_object);
				RETURN(KERN_NO_SPACE);
			}
			if (cur_object->pager->mo_pager_ops
			    != &fourk_pager_ops) {
				vm_object_unlock(cur_object);
				RETURN(KERN_NO_SPACE);
			}
			fourk_object = cur_object;
			fourk_mem_obj = fourk_object->pager;

			                                
			vm_object_reference_locked(fourk_object);
			memory_object_reference(fourk_mem_obj);
			vm_object_unlock(fourk_object);

			                       
			entry->protection |= cur_protection;
			entry->max_protection |= max_protection;
			if ((entry->protection & (VM_PROT_WRITE |
			    VM_PROT_EXECUTE)) ==
			    (VM_PROT_WRITE | VM_PROT_EXECUTE) &&
			    fourk_binary_compatibility_unsafe &&
			    fourk_binary_compatibility_allow_wx) {
				                                     
				entry->used_for_jit = TRUE;
			}
			goto map_in_fourk_pager;
		}

		  
                                            
                
     

		if ((entry->vme_next != vm_map_to_entry(map)) &&
		    (entry->vme_next->vme_start < end)) {
			RETURN(KERN_NO_SPACE);
		}
	}

	  
                  
                                                         
                              
                                                      
                
   
                              
    

	                             
	fourk_mem_obj = fourk_pager_create();
	fourk_object = fourk_pager_to_vm_object(fourk_mem_obj);
	assert(fourk_object);

	                               
	vm_object_reference(fourk_object);

	                                                                  
	fourk_copy = TRUE;
	result = vm_object_copy_strategically(fourk_object,
	    0,
	    end - start,
	    &copy_object,
	    &copy_offset,
	    &fourk_copy);
	assert(result == KERN_SUCCESS);
	assert(copy_object != VM_OBJECT_NULL);
	assert(copy_offset == 0);

	                                      
	new_entry =
	    vm_map_entry_insert(map, entry,
	    vm_map_trunc_page(start,
	    VM_MAP_PAGE_MASK(map)),
	    vm_map_round_page(end,
	    VM_MAP_PAGE_MASK(map)),
	    copy_object,
	    0,                                     
	    vmk_flags,
	    FALSE,                                         
	    FALSE,
	    FALSE,
	    cur_protection, max_protection,
	    VM_BEHAVIOR_DEFAULT,
	    (entry_for_jit && !VM_MAP_POLICY_ALLOW_JIT_INHERIT(map) ?
	    VM_INHERIT_NONE : inheritance),
	    0,
	    no_cache,
	    permanent,
	    no_copy_on_read,
	    superpage_size,
	    clear_map_aligned,
	    is_submap,
	    FALSE,                                  
	    alias,
	    translated_allow_execute);
	entry = new_entry;

#if VM_MAP_DEBUG_FOURK
	if (vm_map_debug_fourk) {
		printf("FOURK_PAGER: map %p [0x%llx:0x%llx] new pager %p\n",
		    map,
		    (uint64_t) entry->vme_start,
		    (uint64_t) entry->vme_end,
		    fourk_mem_obj);
	}
#endif                         

	new_mapping_established = TRUE;

map_in_fourk_pager:
	                                                                    
	fourk_pager_offset = (fourk_start & SIXTEENK_PAGE_MASK);
	fourk_pager_index_start = (int) (fourk_pager_offset / FOURK_PAGE_SIZE);
	if (fourk_size > SIXTEENK_PAGE_SIZE) {
		fourk_pager_index_num = 4;
	} else {
		fourk_pager_index_num = (int) (fourk_size / FOURK_PAGE_SIZE);
	}
	if (fourk_pager_index_start + fourk_pager_index_num > 4) {
		fourk_pager_index_num = 4 - fourk_pager_index_start;
	}
	for (cur_idx = 0;
	    cur_idx < fourk_pager_index_num;
	    cur_idx++) {
		vm_object_t             old_object;
		vm_object_offset_t      old_offset;

		kr = fourk_pager_populate(fourk_mem_obj,
		    TRUE,                                      
		    fourk_pager_index_start + cur_idx,
		    object,
		    (object
		    ? (offset +
		    (cur_idx * FOURK_PAGE_SIZE))
		    : 0),
		    &old_object,
		    &old_offset);
#if VM_MAP_DEBUG_FOURK
		if (vm_map_debug_fourk) {
			if (old_object == (vm_object_t) -1 &&
			    old_offset == (vm_object_offset_t) -1) {
				printf("FOURK_PAGER: map %p [0x%llx:0x%llx] "
				    "pager [%p:0x%llx] "
				    "populate[%d] "
				    "[object:%p,offset:0x%llx]\n",
				    map,
				    (uint64_t) entry->vme_start,
				    (uint64_t) entry->vme_end,
				    fourk_mem_obj,
				    VME_OFFSET(entry),
				    fourk_pager_index_start + cur_idx,
				    object,
				    (object
				    ? (offset + (cur_idx * FOURK_PAGE_SIZE))
				    : 0));
			} else {
				printf("FOURK_PAGER: map %p [0x%llx:0x%llx] "
				    "pager [%p:0x%llx] "
				    "populate[%d] [object:%p,offset:0x%llx] "
				    "old [%p:0x%llx]\n",
				    map,
				    (uint64_t) entry->vme_start,
				    (uint64_t) entry->vme_end,
				    fourk_mem_obj,
				    VME_OFFSET(entry),
				    fourk_pager_index_start + cur_idx,
				    object,
				    (object
				    ? (offset + (cur_idx * FOURK_PAGE_SIZE))
				    : 0),
				    old_object,
				    old_offset);
			}
		}
#endif                         

		assert(kr == KERN_SUCCESS);
		if (object != old_object &&
		    object != VM_OBJECT_NULL &&
		    object != (vm_object_t) -1) {
			vm_object_reference(object);
		}
		if (object != old_object &&
		    old_object != VM_OBJECT_NULL &&
		    old_object != (vm_object_t) -1) {
			vm_object_deallocate(old_object);
		}
	}

BailOut:
	assert(map_locked == TRUE);

	if (result == KERN_SUCCESS) {
		vm_prot_t pager_prot;
		memory_object_t pager;

#if DEBUG
		if (pmap_empty &&
		    !(vmk_flags.vmkf_no_pmap_check)) {
			assert(vm_map_pmap_is_empty(map,
			    *address,
			    *address + size));
		}
#endif            

		  
                                                        
                                                             
                                                            
                         
                                                              
                                              
                                                               
     
		pager_prot = max_protection;
		if (needs_copy) {
			  
                                         
                        
      
			pager_prot &= ~VM_PROT_WRITE;
		}
		if (!is_submap &&
		    object != VM_OBJECT_NULL &&
		    object->named &&
		    object->pager != MEMORY_OBJECT_NULL) {
			vm_object_lock(object);
			pager = object->pager;
			if (object->named &&
			    pager != MEMORY_OBJECT_NULL) {
				assert(object->pager_ready);
				vm_object_mapping_wait(object, THREAD_UNINT);
				vm_object_mapping_begin(object);
				vm_object_unlock(object);

				kr = memory_object_map(pager, pager_prot);
				assert(kr == KERN_SUCCESS);

				vm_object_lock(object);
				vm_object_mapping_end(object);
			}
			vm_object_unlock(object);
		}
		if (!is_submap &&
		    fourk_object != VM_OBJECT_NULL &&
		    fourk_object->named &&
		    fourk_object->pager != MEMORY_OBJECT_NULL) {
			vm_object_lock(fourk_object);
			pager = fourk_object->pager;
			if (fourk_object->named &&
			    pager != MEMORY_OBJECT_NULL) {
				assert(fourk_object->pager_ready);
				vm_object_mapping_wait(fourk_object,
				    THREAD_UNINT);
				vm_object_mapping_begin(fourk_object);
				vm_object_unlock(fourk_object);

				kr = memory_object_map(pager, VM_PROT_READ);
				assert(kr == KERN_SUCCESS);

				vm_object_lock(fourk_object);
				vm_object_mapping_end(fourk_object);
			}
			vm_object_unlock(fourk_object);
		}
	}

	if (fourk_object != VM_OBJECT_NULL) {
		vm_object_deallocate(fourk_object);
		fourk_object = VM_OBJECT_NULL;
		memory_object_deallocate(fourk_mem_obj);
		fourk_mem_obj = MEMORY_OBJECT_NULL;
	}

	assert(map_locked == TRUE);

	if (!keep_map_locked) {
		vm_map_unlock(map);
		map_locked = FALSE;
	}

	  
                                                      
    

	if (result == KERN_SUCCESS) {
		                                      
                                            
     
		if ((map->wiring_required) || (superpage_size)) {
			assert(!keep_map_locked);
			pmap_empty = FALSE;                          
			kr = vm_map_wire_kernel(map, start, end,
			    new_entry->protection, VM_KERN_MEMORY_MLOCK,
			    TRUE);
			result = kr;
		}

	}

	if (result != KERN_SUCCESS) {
		if (new_mapping_established) {
			  
                                                     
                                            
                                                      
                                                       
      
			zap_new_map = vm_map_create(PMAP_NULL,
			    *address,
			    *address + size,
			    map->hdr.entries_pageable);
			vm_map_set_page_shift(zap_new_map,
			    VM_MAP_PAGE_SHIFT(map));
			vm_map_disable_hole_optimization(zap_new_map);

			if (!map_locked) {
				vm_map_lock(map);
				map_locked = TRUE;
			}
			(void) vm_map_delete(map, *address, *address + size,
			    (VM_MAP_REMOVE_SAVE_ENTRIES |
			    VM_MAP_REMOVE_NO_MAP_ALIGN),
			    zap_new_map);
		}
		if (zap_old_map != VM_MAP_NULL &&
		    zap_old_map->hdr.nentries != 0) {
			vm_map_entry_t  entry1, entry2;

			  
                                                 
                                                   
      
			if (!map_locked) {
				vm_map_lock(map);
				map_locked = TRUE;
			}

			                                             
			start = vm_map_first_entry(zap_old_map)->vme_start;
			end = vm_map_last_entry(zap_old_map)->vme_end;
			if (vm_map_lookup_entry(map, start, &entry1) ||
			    vm_map_lookup_entry(map, end, &entry2) ||
			    entry1 != entry2) {
				  
                                          
                                           
                  
       
				vm_map_enter_restore_failures++;
			} else {
				  
                                          
                                           
                                         
       
				for (entry2 = vm_map_first_entry(zap_old_map);
				    entry2 != vm_map_to_entry(zap_old_map);
				    entry2 = vm_map_first_entry(zap_old_map)) {
					vm_map_size_t entry_size;

					entry_size = (entry2->vme_end -
					    entry2->vme_start);
					vm_map_store_entry_unlink(zap_old_map,
					    entry2);
					zap_old_map->size -= entry_size;
					vm_map_store_entry_link(map, entry1, entry2,
					    VM_MAP_KERNEL_FLAGS_NONE);
					map->size += entry_size;
					entry1 = entry2;
				}
				if (map->wiring_required) {
					  
                                      
                         
        
				}
				vm_map_enter_restore_successes++;
			}
		}
	}

	  
                                                                       
                        
    
	if (map_locked && !keep_map_locked) {
		vm_map_unlock(map);
	}

	  
                                                          
                           
    
	if (zap_old_map != VM_MAP_NULL) {
		vm_map_destroy(zap_old_map, VM_MAP_REMOVE_NO_PMAP_CLEANUP);
		zap_old_map = VM_MAP_NULL;
	}
	if (zap_new_map != VM_MAP_NULL) {
		vm_map_destroy(zap_new_map, VM_MAP_REMOVE_NO_PMAP_CLEANUP);
		zap_new_map = VM_MAP_NULL;
	}

	return result;

#undef  RETURN
}
#endif                

  
                                          
   
int64_t vm_prefault_nb_pages = 0;
int64_t vm_prefault_nb_bailout = 0;

static kern_return_t
vm_map_enter_mem_object_helper(
	vm_map_t                target_map,
	vm_map_offset_t         *address,
	vm_map_size_t           initial_size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance,
	upl_page_list_ptr_t     page_list,
	unsigned int            page_list_count)
{
	vm_map_address_t        map_addr;
	vm_map_size_t           map_size;
	vm_object_t             object;
	vm_object_size_t        size;
	kern_return_t           result;
	boolean_t               mask_cur_protection, mask_max_protection;
	boolean_t               kernel_prefault, try_prefault = (page_list_count != 0);
	vm_map_offset_t         offset_in_mapping = 0;
#if __arm64__
	boolean_t               fourk = vmk_flags.vmkf_fourk;
#endif                

	if (VM_MAP_PAGE_SHIFT(target_map) < PAGE_SHIFT) {
		                                                    
		try_prefault = FALSE;
	}

	assertf(vmk_flags.__vmkf_unused == 0, "vmk_flags unused=0x%x\n", vmk_flags.__vmkf_unused);

	mask_cur_protection = cur_protection & VM_PROT_IS_MASK;
	mask_max_protection = max_protection & VM_PROT_IS_MASK;
	cur_protection &= ~VM_PROT_IS_MASK;
	max_protection &= ~VM_PROT_IS_MASK;

	  
                                
    
	if ((target_map == VM_MAP_NULL) ||
	    (cur_protection & ~VM_PROT_ALL) ||
	    (max_protection & ~VM_PROT_ALL) ||
	    (inheritance > VM_INHERIT_LAST_VALID) ||
	    (try_prefault && (copy || !page_list)) ||
	    initial_size == 0) {
		return KERN_INVALID_ARGUMENT;
	}

#if __arm64__
	if (fourk && VM_MAP_PAGE_SHIFT(target_map) < PAGE_SHIFT) {
		                                                     
		fourk = FALSE;
	}
	if (fourk) {
		map_addr = vm_map_trunc_page(*address, FOURK_PAGE_MASK);
		map_size = vm_map_round_page(initial_size, FOURK_PAGE_MASK);
	} else
#endif                
	{
		map_addr = vm_map_trunc_page(*address,
		    VM_MAP_PAGE_MASK(target_map));
		map_size = vm_map_round_page(initial_size,
		    VM_MAP_PAGE_MASK(target_map));
	}
	size = vm_object_round_page(initial_size);

	  
                                                           
    
	if (!IP_VALID(port)) {
		object = VM_OBJECT_NULL;
		offset = 0;
		copy = FALSE;
	} else if (ip_kotype(port) == IKOT_NAMED_ENTRY) {
		vm_named_entry_t        named_entry;
		vm_object_offset_t      data_offset;

		named_entry = (vm_named_entry_t) ip_get_kobject(port);

		if (flags & (VM_FLAGS_RETURN_DATA_ADDR |
		    VM_FLAGS_RETURN_4K_DATA_ADDR)) {
			data_offset = named_entry->data_offset;
			offset += named_entry->data_offset;
		} else {
			data_offset = 0;
		}

		                                                     
		if (size == 0) {
			if (offset >= named_entry->size) {
				return KERN_INVALID_RIGHT;
			}
			size = named_entry->size - offset;
		}
		if (mask_max_protection) {
			max_protection &= named_entry->protection;
		}
		if (mask_cur_protection) {
			cur_protection &= named_entry->protection;
		}
		if ((named_entry->protection & max_protection) !=
		    max_protection) {
			return KERN_INVALID_RIGHT;
		}
		if ((named_entry->protection & cur_protection) !=
		    cur_protection) {
			return KERN_INVALID_RIGHT;
		}
		if (offset + size < offset) {
			              
			return KERN_INVALID_ARGUMENT;
		}
		if (named_entry->size < (offset + initial_size)) {
			return KERN_INVALID_ARGUMENT;
		}

		if (named_entry->is_copy) {
			                                                 
			if ((size != named_entry->size) &&
			    (vm_map_round_page(size,
			    VM_MAP_PAGE_MASK(target_map)) ==
			    named_entry->size)) {
				                                      
				size = vm_map_round_page(
					size,
					VM_MAP_PAGE_MASK(target_map));
			}
		}

		                                                       
		                                                           
		offset = offset + named_entry->offset;

		if (!VM_MAP_PAGE_ALIGNED(size,
		    VM_MAP_PAGE_MASK(target_map))) {
			  
                                        
                                                       
           
      
			map_size = size;
		}

		named_entry_lock(named_entry);
		if (named_entry->is_sub_map) {
			vm_map_t                submap;

			if (flags & (VM_FLAGS_RETURN_DATA_ADDR |
			    VM_FLAGS_RETURN_4K_DATA_ADDR)) {
				panic("VM_FLAGS_RETURN_DATA_ADDR not expected for submap.");
			}

			submap = named_entry->backing.map;
			vm_map_reference(submap);
			named_entry_unlock(named_entry);

			vmk_flags.vmkf_submap = TRUE;

			result = vm_map_enter(target_map,
			    &map_addr,
			    map_size,
			    mask,
			    flags,
			    vmk_flags,
			    tag,
			    (vm_object_t)(uintptr_t) submap,
			    offset,
			    copy,
			    cur_protection,
			    max_protection,
			    inheritance);
			if (result != KERN_SUCCESS) {
				vm_map_deallocate(submap);
			} else {
				  
                                                 
                                              
                                               
                                                 
       
				if (submap->mapped_in_other_pmaps == FALSE &&
				    vm_map_pmap(submap) != PMAP_NULL &&
				    vm_map_pmap(submap) !=
				    vm_map_pmap(target_map)) {
					  
                                            
                                   
                                            
                                       
                                             
                                    
        
					vm_map_lock(submap);
					submap->mapped_in_other_pmaps = TRUE;
					vm_map_unlock(submap);
				}
				*address = map_addr;
			}
			return result;
		} else if (named_entry->is_copy) {
			kern_return_t   kr;
			vm_map_copy_t   copy_map;
			vm_map_entry_t  copy_entry;
			vm_map_offset_t copy_addr;
			vm_map_copy_t   target_copy_map;
			vm_map_offset_t overmap_start, overmap_end;
			vm_map_offset_t trimmed_start;
			vm_map_size_t   target_size;

			if (flags & ~(VM_FLAGS_FIXED |
			    VM_FLAGS_ANYWHERE |
			    VM_FLAGS_OVERWRITE |
			    VM_FLAGS_RETURN_4K_DATA_ADDR |
			    VM_FLAGS_RETURN_DATA_ADDR |
			    VM_FLAGS_ALIAS_MASK)) {
				named_entry_unlock(named_entry);
				return KERN_INVALID_ARGUMENT;
			}

			copy_map = named_entry->backing.copy;
			assert(copy_map->type == VM_MAP_COPY_ENTRY_LIST);
			if (copy_map->type != VM_MAP_COPY_ENTRY_LIST) {
				                                         
				printf("vm_map_enter_mem_object: "
				    "memory_entry->backing.copy "
				    "unsupported type 0x%x\n",
				    copy_map->type);
				named_entry_unlock(named_entry);
				return KERN_INVALID_ARGUMENT;
			}

			if (VM_MAP_PAGE_SHIFT(target_map) != copy_map->cpy_hdr.page_shift) {
				DEBUG4K_SHARE("copy_map %p offset %llx size 0x%llx pgshift %d -> target_map %p pgshift %d\n", copy_map, offset, (uint64_t)map_size, copy_map->cpy_hdr.page_shift, target_map, VM_MAP_PAGE_SHIFT(target_map));
			}

			if (flags & (VM_FLAGS_RETURN_DATA_ADDR |
			    VM_FLAGS_RETURN_4K_DATA_ADDR)) {
				offset_in_mapping = offset & VM_MAP_PAGE_MASK(target_map);
				if (flags & VM_FLAGS_RETURN_4K_DATA_ADDR) {
					offset_in_mapping &= ~((signed)(0xFFF));
				}
			}

			target_copy_map = VM_MAP_COPY_NULL;
			target_size = copy_map->size;
			overmap_start = 0;
			overmap_end = 0;
			trimmed_start = 0;
			if (copy_map->cpy_hdr.page_shift != VM_MAP_PAGE_SHIFT(target_map)) {
				DEBUG4K_ADJUST("adjusting...\n");
				kr = vm_map_copy_adjust_to_target(
					copy_map,
					offset                           ,
					initial_size,
					target_map,
					copy,
					&target_copy_map,
					&overmap_start,
					&overmap_end,
					&trimmed_start);
				if (kr != KERN_SUCCESS) {
					named_entry_unlock(named_entry);
					return kr;
				}
				target_size = target_copy_map->size;
				if (trimmed_start >= data_offset) {
					data_offset = offset & VM_MAP_PAGE_MASK(target_map);
				} else {
					data_offset -= trimmed_start;
				}
			} else {
				target_copy_map = copy_map;
			}

			                                
			kr = vm_map_enter(target_map,
			    &map_addr,
			    vm_map_round_page(target_size, VM_MAP_PAGE_MASK(target_map)),
			    mask,
			    flags & (VM_FLAGS_ANYWHERE |
			    VM_FLAGS_OVERWRITE |
			    VM_FLAGS_RETURN_4K_DATA_ADDR |
			    VM_FLAGS_RETURN_DATA_ADDR),
			    vmk_flags,
			    tag,
			    VM_OBJECT_NULL,
			    0,
			    FALSE,                         
			    cur_protection,
			    max_protection,
			    inheritance);
			if (kr != KERN_SUCCESS) {
				DEBUG4K_ERROR("kr 0x%x\n", kr);
				if (target_copy_map != copy_map) {
					vm_map_copy_discard(target_copy_map);
					target_copy_map = VM_MAP_COPY_NULL;
				}
				named_entry_unlock(named_entry);
				return kr;
			}

			copy_addr = map_addr;

			for (copy_entry = vm_map_copy_first_entry(target_copy_map);
			    copy_entry != vm_map_copy_to_entry(target_copy_map);
			    copy_entry = copy_entry->vme_next) {
				int                     remap_flags;
				vm_map_kernel_flags_t   vmk_remap_flags;
				vm_map_t                copy_submap;
				vm_object_t             copy_object;
				vm_map_size_t           copy_size;
				vm_object_offset_t      copy_offset;
				int                     copy_vm_alias;

				remap_flags = 0;
				vmk_remap_flags = VM_MAP_KERNEL_FLAGS_NONE;

				copy_object = VME_OBJECT(copy_entry);
				copy_offset = VME_OFFSET(copy_entry);
				copy_size = (copy_entry->vme_end -
				    copy_entry->vme_start);
				VM_GET_FLAGS_ALIAS(flags, copy_vm_alias);
				if (copy_vm_alias == 0) {
					  
                                       
                                        
                                          
        
					copy_vm_alias = VME_ALIAS(copy_entry);
				}

				                  
				if ((copy_addr + copy_size) >
				    (map_addr +
				    overmap_start + overmap_end +
				    named_entry->size                    )) {
					                              
					kr = KERN_INVALID_ARGUMENT;
					DEBUG4K_ERROR("kr 0x%x\n", kr);
					           
					break;
				}

				                                    
				if (copy_entry->is_sub_map) {
					vmk_remap_flags.vmkf_submap = TRUE;
					copy_submap = VME_SUBMAP(copy_entry);
					vm_map_lock(copy_submap);
					vm_map_reference(copy_submap);
					vm_map_unlock(copy_submap);
					copy_object = (vm_object_t)(uintptr_t) copy_submap;
				} else if (!copy &&
				    copy_object != VM_OBJECT_NULL &&
				    (copy_entry->needs_copy ||
				    copy_object->shadowed ||
				    (!copy_object->true_share &&
				    !copy_entry->is_shared &&
				    copy_object->vo_size > copy_size))) {
					  
                                           
                                         
                                           
                                        
                                      
                         
       
                                        
                                          
                                    
        
					                                                                      
					VME_OBJECT_SHADOW(copy_entry, copy_size);
					if (!copy_entry->needs_copy &&
					    copy_entry->protection & VM_PROT_WRITE) {
						vm_prot_t prot;

						prot = copy_entry->protection & ~VM_PROT_WRITE;
						vm_object_pmap_protect(copy_object,
						    copy_offset,
						    copy_size,
						    PMAP_NULL,
						    PAGE_SIZE,
						    0,
						    prot);
					}

					copy_entry->needs_copy = FALSE;
					copy_entry->is_shared = TRUE;
					copy_object = VME_OBJECT(copy_entry);
					copy_offset = VME_OFFSET(copy_entry);
					vm_object_lock(copy_object);
					vm_object_reference_locked(copy_object);
					if (copy_object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
						                                                         
						copy_object->copy_strategy = MEMORY_OBJECT_COPY_DELAY;
						copy_object->true_share = TRUE;
					}
					vm_object_unlock(copy_object);
				} else {
					  
                                        
               
        
					copy_object = VME_OBJECT(copy_entry);
					vm_object_reference(copy_object);
				}

				                                          
				remap_flags |= flags;
				remap_flags |= VM_FLAGS_FIXED;
				remap_flags |= VM_FLAGS_OVERWRITE;
				remap_flags &= ~VM_FLAGS_ANYWHERE;
				if (!copy && !copy_entry->is_sub_map) {
					  
                                      
                                           
                                          
        
					assert(!copy_entry->needs_copy);
				}
#if XNU_TARGET_OS_OSX
				if (copy_entry->used_for_jit) {
					vmk_remap_flags.vmkf_map_jit = TRUE;
				}
#endif                        

				assertf((copy_vm_alias & VME_ALIAS_MASK) == copy_vm_alias,
				    "VM Tag truncated from 0x%x to 0x%x\n", copy_vm_alias, (copy_vm_alias & VME_ALIAS_MASK));
				kr = vm_map_enter(target_map,
				    &copy_addr,
				    copy_size,
				    (vm_map_offset_t) 0,
				    remap_flags,
				    vmk_remap_flags,
				    (vm_tag_t) copy_vm_alias,                                                    
				    copy_object,
				    copy_offset,
				    ((copy_object == NULL)
				    ? FALSE
				    : (copy || copy_entry->needs_copy)),
				    cur_protection,
				    max_protection,
				    inheritance);
				if (kr != KERN_SUCCESS) {
					DEBUG4K_SHARE("failed kr 0x%x\n", kr);
					if (copy_entry->is_sub_map) {
						vm_map_deallocate(copy_submap);
					} else {
						vm_object_deallocate(copy_object);
					}
					           
					break;
				}

				                  
				copy_addr += copy_size;
			}

			if (kr == KERN_SUCCESS) {
				if (flags & (VM_FLAGS_RETURN_DATA_ADDR |
				    VM_FLAGS_RETURN_4K_DATA_ADDR)) {
					*address = map_addr + offset_in_mapping;
				} else {
					*address = map_addr;
				}
				if (overmap_start) {
					*address += overmap_start;
					DEBUG4K_SHARE("map %p map_addr 0x%llx offset_in_mapping 0x%llx overmap_start 0x%llx -> *address 0x%llx\n", target_map, (uint64_t)map_addr, (uint64_t) offset_in_mapping, (uint64_t)overmap_start, (uint64_t)*address);
				}
			}
			named_entry_unlock(named_entry);
			if (target_copy_map != copy_map) {
				vm_map_copy_discard(target_copy_map);
				target_copy_map = VM_MAP_COPY_NULL;
			}

			if (kr != KERN_SUCCESS) {
				if (!(flags & VM_FLAGS_OVERWRITE)) {
					                                     
					(void) vm_deallocate(target_map,
					    map_addr,
					    map_size);
				}
			}

			return kr;
		}

		if (named_entry->is_object) {
			unsigned int    access;
			vm_prot_t       protections;
			unsigned int    wimg_mode;

			                                

			protections = named_entry->protection & VM_PROT_ALL;
			access = GET_MAP_MEM(named_entry->protection);

			if (flags & (VM_FLAGS_RETURN_DATA_ADDR |
			    VM_FLAGS_RETURN_4K_DATA_ADDR)) {
				offset_in_mapping = offset - VM_MAP_TRUNC_PAGE(offset, VM_MAP_PAGE_MASK(target_map));
				if (flags & VM_FLAGS_RETURN_4K_DATA_ADDR) {
					offset_in_mapping &= ~((signed)(0xFFF));
				}
				offset = VM_MAP_TRUNC_PAGE(offset, VM_MAP_PAGE_MASK(target_map));
				map_size = VM_MAP_ROUND_PAGE((offset + offset_in_mapping + initial_size) - offset, VM_MAP_PAGE_MASK(target_map));
			}

			object = vm_named_entry_to_vm_object(named_entry);
			assert(object != VM_OBJECT_NULL);
			vm_object_lock(object);
			named_entry_unlock(named_entry);

			vm_object_reference_locked(object);

			wimg_mode = object->wimg_bits;
			vm_prot_to_wimg(access, &wimg_mode);
			if (object->wimg_bits != wimg_mode) {
				vm_object_change_wimg_mode(object, wimg_mode);
			}

			vm_object_unlock(object);
		} else {
			panic("invalid VM named entry %p", named_entry);
		}
	} else if (ip_kotype(port) == IKOT_MEMORY_OBJECT) {
		  
                                                         
                            
    
                                                     
                                                        
                                         
     
		if (flags & (VM_FLAGS_RETURN_DATA_ADDR |
		    VM_FLAGS_RETURN_4K_DATA_ADDR)) {
			panic("VM_FLAGS_RETURN_DATA_ADDR not expected for raw memory object.");
		}

		object = memory_object_to_vm_object((memory_object_t)port);
		if (object == VM_OBJECT_NULL) {
			return KERN_INVALID_OBJECT;
		}
		vm_object_reference(object);

		                                          
		if (object != VM_OBJECT_NULL) {
			if (object == kernel_object) {
				printf("Warning: Attempt to map kernel object"
				    " by a non-private kernel entity\n");
				return KERN_INVALID_OBJECT;
			}
			if (!object->pager_ready) {
				vm_object_lock(object);

				while (!object->pager_ready) {
					vm_object_wait(object,
					    VM_OBJECT_EVENT_PAGER_READY,
					    THREAD_UNINT);
					vm_object_lock(object);
				}
				vm_object_unlock(object);
			}
		}
	} else {
		return KERN_INVALID_OBJECT;
	}

	if (object != VM_OBJECT_NULL &&
	    object->named &&
	    object->pager != MEMORY_OBJECT_NULL &&
	    object->copy_strategy != MEMORY_OBJECT_COPY_NONE) {
		memory_object_t pager;
		vm_prot_t       pager_prot;
		kern_return_t   kr;

		  
                                                        
                                                             
                                                            
                         
                                                              
                                              
                                                               
     
		pager_prot = max_protection;
		if (copy) {
			  
                                             
                    
      
			pager_prot &= ~VM_PROT_WRITE;
		}
		vm_object_lock(object);
		pager = object->pager;
		if (object->named &&
		    pager != MEMORY_OBJECT_NULL &&
		    object->copy_strategy != MEMORY_OBJECT_COPY_NONE) {
			assert(object->pager_ready);
			vm_object_mapping_wait(object, THREAD_UNINT);
			vm_object_mapping_begin(object);
			vm_object_unlock(object);

			kr = memory_object_map(pager, pager_prot);
			assert(kr == KERN_SUCCESS);

			vm_object_lock(object);
			vm_object_mapping_end(object);
		}
		vm_object_unlock(object);
	}

	  
                                 
    

	if (copy) {
		vm_object_t             new_object;
		vm_object_offset_t      new_offset;

		result = vm_object_copy_strategically(object, offset,
		    map_size,
		    &new_object, &new_offset,
		    &copy);


		if (result == KERN_MEMORY_RESTART_COPY) {
			boolean_t success;
			boolean_t src_needs_copy;

			  
         
                                         
                                             
                                           
                                                    
                                                      
                                                
                                      
      
			new_object = object;
			new_offset = offset;
			success = vm_object_copy_quickly(&new_object,
			    new_offset,
			    map_size,
			    &src_needs_copy,
			    &copy);
			assert(success);
			result = KERN_SUCCESS;
		}
		  
                                    
                                            
     

		vm_object_deallocate(object);

		if (result != KERN_SUCCESS) {
			return result;
		}

		object = new_object;
		offset = new_offset;
	}

	  
                                                                               
                       
    
	kernel_prefault = (try_prefault && vm_kernel_map_is_kernel(target_map));
	vmk_flags.vmkf_keep_map_locked = (try_prefault && !kernel_prefault);

#if __arm64__
	if (fourk) {
		                                     
		result = vm_map_enter_fourk(target_map,
		    &map_addr,
		    map_size,
		    (vm_map_offset_t) mask,
		    flags,
		    vmk_flags,
		    tag,
		    object,
		    offset,
		    copy,
		    cur_protection,
		    max_protection,
		    inheritance);
	} else
#endif                
	{
		result = vm_map_enter(target_map,
		    &map_addr, map_size,
		    (vm_map_offset_t)mask,
		    flags,
		    vmk_flags,
		    tag,
		    object, offset,
		    copy,
		    cur_protection, max_protection,
		    inheritance);
	}
	if (result != KERN_SUCCESS) {
		vm_object_deallocate(object);
	}

	  
                                                                  
    
	if (result == KERN_SUCCESS && try_prefault) {
		mach_vm_address_t va = map_addr;
		kern_return_t kr = KERN_SUCCESS;
		unsigned int i = 0;
		int pmap_options;

		pmap_options = kernel_prefault ? 0 : PMAP_OPTIONS_NOWAIT;
		if (object->internal) {
			pmap_options |= PMAP_OPTIONS_INTERNAL;
		}

		for (i = 0; i < page_list_count; ++i) {
			if (!UPL_VALID_PAGE(page_list, i)) {
				if (kernel_prefault) {
					assertf(FALSE, "kernel_prefault && !UPL_VALID_PAGE");
					result = KERN_MEMORY_ERROR;
					break;
				}
			} else {
				  
                                                   
                                                 
                         
      
                                                
                                                  
                          
       
				kr = pmap_enter_options(target_map->pmap,
				    va, UPL_PHYS_PAGE(page_list, i),
				    cur_protection, VM_PROT_NONE,
				    0, TRUE, pmap_options, NULL);
				if (kr != KERN_SUCCESS) {
					OSIncrementAtomic64(&vm_prefault_nb_bailout);
					if (kernel_prefault) {
						result = kr;
					}
					break;
				}
				OSIncrementAtomic64(&vm_prefault_nb_pages);
			}

			                          
			va += PAGE_SIZE;
		}
		if (vmk_flags.vmkf_keep_map_locked) {
			vm_map_unlock(target_map);
		}
	}

	if (flags & (VM_FLAGS_RETURN_DATA_ADDR |
	    VM_FLAGS_RETURN_4K_DATA_ADDR)) {
		*address = map_addr + offset_in_mapping;
	} else {
		*address = map_addr;
	}
	return result;
}

kern_return_t
vm_map_enter_mem_object(
	vm_map_t                target_map,
	vm_map_offset_t         *address,
	vm_map_size_t           initial_size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	kern_return_t ret;

	ret = vm_map_enter_mem_object_helper(target_map,
	    address,
	    initial_size,
	    mask,
	    flags,
	    vmk_flags,
	    tag,
	    port,
	    offset,
	    copy,
	    cur_protection,
	    max_protection,
	    inheritance,
	    NULL,
	    0);

#if KASAN
	if (ret == KERN_SUCCESS && address && target_map->pmap == kernel_pmap) {
		kasan_notify_address(*address, initial_size);
	}
#endif

	return ret;
}

kern_return_t
vm_map_enter_mem_object_prefault(
	vm_map_t                target_map,
	vm_map_offset_t         *address,
	vm_map_size_t           initial_size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	upl_page_list_ptr_t     page_list,
	unsigned int            page_list_count)
{
	kern_return_t ret;

	ret = vm_map_enter_mem_object_helper(target_map,
	    address,
	    initial_size,
	    mask,
	    flags,
	    vmk_flags,
	    tag,
	    port,
	    offset,
	    FALSE,
	    cur_protection,
	    max_protection,
	    VM_INHERIT_DEFAULT,
	    page_list,
	    page_list_count);

#if KASAN
	if (ret == KERN_SUCCESS && address && target_map->pmap == kernel_pmap) {
		kasan_notify_address(*address, initial_size);
	}
#endif

	return ret;
}


kern_return_t
vm_map_enter_mem_object_control(
	vm_map_t                target_map,
	vm_map_offset_t         *address,
	vm_map_size_t           initial_size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	memory_object_control_t control,
	vm_object_offset_t      offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	vm_map_address_t        map_addr;
	vm_map_size_t           map_size;
	vm_object_t             object;
	vm_object_size_t        size;
	kern_return_t           result;
	memory_object_t         pager;
	vm_prot_t               pager_prot;
	kern_return_t           kr;
#if __arm64__
	boolean_t               fourk = vmk_flags.vmkf_fourk;
#endif                

	  
                                
    
	if ((target_map == VM_MAP_NULL) ||
	    (cur_protection & ~VM_PROT_ALL) ||
	    (max_protection & ~VM_PROT_ALL) ||
	    (inheritance > VM_INHERIT_LAST_VALID) ||
	    initial_size == 0) {
		return KERN_INVALID_ARGUMENT;
	}

#if __arm64__
	if (fourk && VM_MAP_PAGE_MASK(target_map) < PAGE_MASK) {
		fourk = FALSE;
	}

	if (fourk) {
		map_addr = vm_map_trunc_page(*address,
		    FOURK_PAGE_MASK);
		map_size = vm_map_round_page(initial_size,
		    FOURK_PAGE_MASK);
	} else
#endif                
	{
		map_addr = vm_map_trunc_page(*address,
		    VM_MAP_PAGE_MASK(target_map));
		map_size = vm_map_round_page(initial_size,
		    VM_MAP_PAGE_MASK(target_map));
	}
	size = vm_object_round_page(initial_size);

	object = memory_object_control_to_vm_object(control);

	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_OBJECT;
	}

	if (object == kernel_object) {
		printf("Warning: Attempt to map kernel object"
		    " by a non-private kernel entity\n");
		return KERN_INVALID_OBJECT;
	}

	vm_object_lock(object);
	object->ref_count++;

	  
                                                       
                                                            
                                                           
                        
                                                             
                                             
                                                              
    
	pager_prot = max_protection;
	if (copy) {
		pager_prot &= ~VM_PROT_WRITE;
	}
	pager = object->pager;
	if (object->named &&
	    pager != MEMORY_OBJECT_NULL &&
	    object->copy_strategy != MEMORY_OBJECT_COPY_NONE) {
		assert(object->pager_ready);
		vm_object_mapping_wait(object, THREAD_UNINT);
		vm_object_mapping_begin(object);
		vm_object_unlock(object);

		kr = memory_object_map(pager, pager_prot);
		assert(kr == KERN_SUCCESS);

		vm_object_lock(object);
		vm_object_mapping_end(object);
	}
	vm_object_unlock(object);

	  
                                 
    

	if (copy) {
		vm_object_t             new_object;
		vm_object_offset_t      new_offset;

		result = vm_object_copy_strategically(object, offset, size,
		    &new_object, &new_offset,
		    &copy);


		if (result == KERN_MEMORY_RESTART_COPY) {
			boolean_t success;
			boolean_t src_needs_copy;

			  
         
                                         
                                             
                                           
                                                    
                                                      
                                                
                                      
      
			new_object = object;
			new_offset = offset;
			success = vm_object_copy_quickly(&new_object,
			    new_offset, size,
			    &src_needs_copy,
			    &copy);
			assert(success);
			result = KERN_SUCCESS;
		}
		  
                                    
                                            
     

		vm_object_deallocate(object);

		if (result != KERN_SUCCESS) {
			return result;
		}

		object = new_object;
		offset = new_offset;
	}

#if __arm64__
	if (fourk) {
		result = vm_map_enter_fourk(target_map,
		    &map_addr,
		    map_size,
		    (vm_map_offset_t)mask,
		    flags,
		    vmk_flags,
		    tag,
		    object, offset,
		    copy,
		    cur_protection, max_protection,
		    inheritance);
	} else
#endif                
	{
		result = vm_map_enter(target_map,
		    &map_addr, map_size,
		    (vm_map_offset_t)mask,
		    flags,
		    vmk_flags,
		    tag,
		    object, offset,
		    copy,
		    cur_protection, max_protection,
		    inheritance);
	}
	if (result != KERN_SUCCESS) {
		vm_object_deallocate(object);
	}
	*address = map_addr;

	return result;
}


#if     VM_CPM

#ifdef MACH_ASSERT
extern pmap_paddr_t     avail_start, avail_end;
#endif

  
                                                             
                                                           
                                                         
                                                           
                   
  
                                                        
                                       
   
kern_return_t
vm_map_enter_cpm(
	vm_map_t                map,
	vm_map_offset_t *addr,
	vm_map_size_t           size,
	int                     flags)
{
	vm_object_t             cpm_obj;
	pmap_t                  pmap;
	vm_page_t               m, pages;
	kern_return_t           kr;
	vm_map_offset_t         va, start, end, offset;
#if     MACH_ASSERT
	vm_map_offset_t         prev_addr = 0;
#endif                   

	boolean_t               anywhere = ((VM_FLAGS_ANYWHERE & flags) != 0);
	vm_tag_t tag;

	if (VM_MAP_PAGE_SHIFT(map) != PAGE_SHIFT) {
		                                            
		*addr = 0;
		return KERN_NOT_SUPPORTED;
	}

	VM_GET_FLAGS_ALIAS(flags, tag);

	if (size == 0) {
		*addr = 0;
		return KERN_SUCCESS;
	}
	if (anywhere) {
		*addr = vm_map_min(map);
	} else {
		*addr = vm_map_trunc_page(*addr,
		    VM_MAP_PAGE_MASK(map));
	}
	size = vm_map_round_page(size,
	    VM_MAP_PAGE_MASK(map));

	  
                                                 
                                                 
                                                  
    
	if (size > VM_MAX_ADDRESS) {
		return KERN_RESOURCE_SHORTAGE;
	}
	if ((kr = cpm_allocate(CAST_DOWN(vm_size_t, size),
	    &pages, 0, 0, TRUE, flags)) != KERN_SUCCESS) {
		return kr;
	}

	cpm_obj = vm_object_allocate((vm_object_size_t)size);
	assert(cpm_obj != VM_OBJECT_NULL);
	assert(cpm_obj->internal);
	assert(cpm_obj->vo_size == (vm_object_size_t)size);
	assert(cpm_obj->can_persist == FALSE);
	assert(cpm_obj->pager_created == FALSE);
	assert(cpm_obj->pageout == FALSE);
	assert(cpm_obj->shadow == VM_OBJECT_NULL);

	  
                             
    

	vm_object_lock(cpm_obj);
	for (offset = 0; offset < size; offset += PAGE_SIZE) {
		m = pages;
		pages = NEXT_PAGE(m);
		*(NEXT_PAGE_PTR(m)) = VM_PAGE_NULL;

		assert(!m->vmp_gobbled);
		assert(!m->vmp_wanted);
		assert(!m->vmp_pageout);
		assert(!m->vmp_tabled);
		assert(VM_PAGE_WIRED(m));
		assert(m->vmp_busy);
		assert(VM_PAGE_GET_PHYS_PAGE(m) >= (avail_start >> PAGE_SHIFT) && VM_PAGE_GET_PHYS_PAGE(m) <= (avail_end >> PAGE_SHIFT));

		m->vmp_busy = FALSE;
		vm_page_insert(m, cpm_obj, offset);
	}
	assert(cpm_obj->resident_page_count == size / PAGE_SIZE);
	vm_object_unlock(cpm_obj);

	  
                                                 
                                                      
                                                       
                                     
   
                                                       
                                                        
                                                      
                 
    
	vm_object_reference(cpm_obj);

	  
                           
    

	kr = vm_map_enter(
		map,
		addr,
		size,
		(vm_map_offset_t)0,
		flags,
		VM_MAP_KERNEL_FLAGS_NONE,
		cpm_obj,
		(vm_object_offset_t)0,
		FALSE,
		VM_PROT_ALL,
		VM_PROT_ALL,
		VM_INHERIT_DEFAULT);

	if (kr != KERN_SUCCESS) {
		  
                                               
                                             
                         
     
		assert(cpm_obj->pager_created == FALSE);
		assert(cpm_obj->can_persist == FALSE);
		assert(cpm_obj->pageout == FALSE);
		assert(cpm_obj->shadow == VM_OBJECT_NULL);
		vm_object_deallocate(cpm_obj);                        
		vm_object_deallocate(cpm_obj);                        
	}

	  
                                               
                                             
                                                    
    
	start = *addr;
	end = start + size;
	pmap = vm_map_pmap(map);
	pmap_pageable(pmap, start, end, FALSE);

	  
                                                   
                                                        
                                                       
          
    
	for (offset = 0, va = start; offset < size;
	    va += PAGE_SIZE, offset += PAGE_SIZE) {
		int type_of_fault;

		vm_object_lock(cpm_obj);
		m = vm_page_lookup(cpm_obj, (vm_object_offset_t)offset);
		assert(m != VM_PAGE_NULL);

		vm_page_zero_fill(m);

		type_of_fault = DBG_ZERO_FILL_FAULT;

		vm_fault_enter(m, pmap, va,
		    PAGE_SIZE, 0,
		    VM_PROT_ALL, VM_PROT_WRITE,
		    VM_PAGE_WIRED(m),
		    FALSE,                                                
		    VM_KERN_MEMORY_NONE,                                                   
		    FALSE,                                           
		    FALSE,                                            
		    0,                                               
		    0,                                               
		    NULL,                                              
		    &type_of_fault);

		vm_object_unlock(cpm_obj);
	}

#if     MACH_ASSERT
	  
                                     
    
	for (offset = 0; offset < size; offset += PAGE_SIZE) {
		vm_object_lock(cpm_obj);
		m = vm_page_lookup(cpm_obj, (vm_object_offset_t)offset);
		vm_object_unlock(cpm_obj);
		if (m == VM_PAGE_NULL) {
			panic("vm_allocate_cpm:  obj %p off 0x%llx no page",
			    cpm_obj, (uint64_t)offset);
		}
		assert(m->vmp_tabled);
		assert(!m->vmp_busy);
		assert(!m->vmp_wanted);
		assert(!m->vmp_fictitious);
		assert(!m->vmp_private);
		assert(!m->vmp_absent);
		assert(!m->vmp_error);
		assert(!m->vmp_cleaning);
		assert(!m->vmp_laundry);
		assert(!m->vmp_precious);
		assert(!m->vmp_clustered);
		if (offset != 0) {
			if (VM_PAGE_GET_PHYS_PAGE(m) != prev_addr + 1) {
				printf("start 0x%llx end 0x%llx va 0x%llx\n",
				    (uint64_t)start, (uint64_t)end, (uint64_t)va);
				printf("obj %p off 0x%llx\n", cpm_obj, (uint64_t)offset);
				printf("m %p prev_address 0x%llx\n", m, (uint64_t)prev_addr);
				panic("vm_allocate_cpm:  pages not contig!");
			}
		}
		prev_addr = VM_PAGE_GET_PHYS_PAGE(m);
	}
#endif                   

	vm_object_deallocate(cpm_obj);                     

	return kr;
}


#else               

  
                                                           
                                                          
           
   

kern_return_t
vm_map_enter_cpm(
	__unused vm_map_t       map,
	__unused vm_map_offset_t        *addr,
	__unused vm_map_size_t  size,
	__unused int            flags)
{
	return KERN_FAILURE;
}
#endif             

                                   
#ifndef NO_NESTED_PMAP
  
                                                        
   


static void
vm_map_clip_unnest(
	vm_map_t        map,
	vm_map_entry_t  entry,
	vm_map_offset_t start_unnest,
	vm_map_offset_t end_unnest)
{
	vm_map_offset_t old_start_unnest = start_unnest;
	vm_map_offset_t old_end_unnest = end_unnest;

	assert(entry->is_sub_map);
	assert(VME_SUBMAP(entry) != NULL);
	assert(entry->use_pmap);

	  
                                                    
                                                       
                                                            
                                                          
                                                             
                                
    
	if (pmap_adjust_unnest_parameters(map->pmap, &start_unnest, &end_unnest)) {
		assert(VME_SUBMAP(entry)->is_nested_map);
		assert(!VME_SUBMAP(entry)->disable_vmentry_reuse);
		log_unnest_badness(map,
		    old_start_unnest,
		    old_end_unnest,
		    VME_SUBMAP(entry)->is_nested_map,
		    (entry->vme_start +
		    VME_SUBMAP(entry)->lowest_unnestable_start -
		    VME_OFFSET(entry)));
	}

	if (entry->vme_start > start_unnest ||
	    entry->vme_end < end_unnest) {
		panic("vm_map_clip_unnest(0x%llx,0x%llx): "
		    "bad nested entry: start=0x%llx end=0x%llx\n",
		    (long long)start_unnest, (long long)end_unnest,
		    (long long)entry->vme_start, (long long)entry->vme_end);
	}

	if (start_unnest > entry->vme_start) {
		_vm_map_clip_start(&map->hdr,
		    entry,
		    start_unnest);
		if (map->holelistenabled) {
			vm_map_store_update_first_free(map, NULL, FALSE);
		} else {
			vm_map_store_update_first_free(map, map->first_free, FALSE);
		}
	}
	if (entry->vme_end > end_unnest) {
		_vm_map_clip_end(&map->hdr,
		    entry,
		    end_unnest);
		if (map->holelistenabled) {
			vm_map_store_update_first_free(map, NULL, FALSE);
		} else {
			vm_map_store_update_first_free(map, map->first_free, FALSE);
		}
	}

	pmap_unnest(map->pmap,
	    entry->vme_start,
	    entry->vme_end - entry->vme_start);
	if ((map->mapped_in_other_pmaps) && os_ref_get_count(&map->map_refcnt) != 0) {
		                              
		vm_map_submap_pmap_clean(
			map, entry->vme_start,
			entry->vme_end,
			VME_SUBMAP(entry),
			VME_OFFSET(entry));
	}
	entry->use_pmap = FALSE;
	if ((map->pmap != kernel_pmap) &&
	    (VME_ALIAS(entry) == VM_MEMORY_SHARED_PMAP)) {
		VME_ALIAS_SET(entry, VM_MEMORY_UNSHARED_PMAP);
	}
}
#endif                      

  
                                           
  
                                                  
                                       
                                
   
void
vm_map_clip_start(
	vm_map_t        map,
	vm_map_entry_t  entry,
	vm_map_offset_t startaddr)
{
#ifndef NO_NESTED_PMAP
	if (entry->is_sub_map &&
	    entry->use_pmap &&
	    startaddr >= entry->vme_start) {
		vm_map_offset_t start_unnest, end_unnest;

		  
                                                         
                                                                
                
                                                             
                      
     
		start_unnest = startaddr & ~(pmap_shared_region_size_min(map->pmap) - 1);
		end_unnest = start_unnest + pmap_shared_region_size_min(map->pmap);
		vm_map_clip_unnest(map, entry, start_unnest, end_unnest);
	}
#endif                     
	if (startaddr > entry->vme_start) {
		if (VME_OBJECT(entry) &&
		    !entry->is_sub_map &&
		    VME_OBJECT(entry)->phys_contiguous) {
			pmap_remove(map->pmap,
			    (addr64_t)(entry->vme_start),
			    (addr64_t)(entry->vme_end));
		}
		if (entry->vme_atomic) {
			panic("Attempting to clip an atomic VM entry! (map: %p, entry: %p)\n", map, entry);
		}

		DTRACE_VM5(
			vm_map_clip_start,
			vm_map_t, map,
			vm_map_offset_t, entry->vme_start,
			vm_map_offset_t, entry->vme_end,
			vm_map_offset_t, startaddr,
			int, VME_ALIAS(entry));

		_vm_map_clip_start(&map->hdr, entry, startaddr);
		if (map->holelistenabled) {
			vm_map_store_update_first_free(map, NULL, FALSE);
		} else {
			vm_map_store_update_first_free(map, map->first_free, FALSE);
		}
	}
}


#define vm_map_copy_clip_start(copy, entry, startaddr) \
	MACRO_BEGIN \
	if ((startaddr) > (entry)->vme_start) \
	        _vm_map_clip_start(&(copy)->cpy_hdr,(entry),(startaddr)); \
	MACRO_END

  
                                                    
                           
   
static void
_vm_map_clip_start(
	struct vm_map_header    *map_header,
	vm_map_entry_t          entry,
	vm_map_offset_t         start)
{
	vm_map_entry_t  new_entry;

	  
                                  
                                    
                                  
                                         
            
    

	if (entry->map_aligned) {
		assert(VM_MAP_PAGE_ALIGNED(start,
		    VM_MAP_HDR_PAGE_MASK(map_header)));
	}

	new_entry = _vm_map_entry_create(map_header, !map_header->entries_pageable);
	vm_map_entry_copy_full(new_entry, entry);

	new_entry->vme_end = start;
	assert(new_entry->vme_start < new_entry->vme_end);
	VME_OFFSET_SET(entry, VME_OFFSET(entry) + (start - entry->vme_start));
	assert(start < entry->vme_end);
	entry->vme_start = start;

	_vm_map_store_entry_link(map_header, entry->vme_prev, new_entry);

	if (entry->is_sub_map) {
		vm_map_reference(VME_SUBMAP(new_entry));
	} else {
		vm_object_reference(VME_OBJECT(new_entry));
	}
}


  
                                         
  
                                                 
                                       
                                
   
void
vm_map_clip_end(
	vm_map_t        map,
	vm_map_entry_t  entry,
	vm_map_offset_t endaddr)
{
	if (endaddr > entry->vme_end) {
		  
                                                          
                                 
     
		endaddr = entry->vme_end;
	}
#ifndef NO_NESTED_PMAP
	if (entry->is_sub_map && entry->use_pmap) {
		vm_map_offset_t start_unnest, end_unnest;

		  
                                                            
                                                          
                                                           
                                                             
                      
     
		start_unnest = entry->vme_start;
		end_unnest =
		    (endaddr + pmap_shared_region_size_min(map->pmap) - 1) &
		    ~(pmap_shared_region_size_min(map->pmap) - 1);
		vm_map_clip_unnest(map, entry, start_unnest, end_unnest);
	}
#endif                     
	if (endaddr < entry->vme_end) {
		if (VME_OBJECT(entry) &&
		    !entry->is_sub_map &&
		    VME_OBJECT(entry)->phys_contiguous) {
			pmap_remove(map->pmap,
			    (addr64_t)(entry->vme_start),
			    (addr64_t)(entry->vme_end));
		}
		if (entry->vme_atomic) {
			panic("Attempting to clip an atomic VM entry! (map: %p, entry: %p)\n", map, entry);
		}
		DTRACE_VM5(
			vm_map_clip_end,
			vm_map_t, map,
			vm_map_offset_t, entry->vme_start,
			vm_map_offset_t, entry->vme_end,
			vm_map_offset_t, endaddr,
			int, VME_ALIAS(entry));

		_vm_map_clip_end(&map->hdr, entry, endaddr);
		if (map->holelistenabled) {
			vm_map_store_update_first_free(map, NULL, FALSE);
		} else {
			vm_map_store_update_first_free(map, map->first_free, FALSE);
		}
	}
}


#define vm_map_copy_clip_end(copy, entry, endaddr) \
	MACRO_BEGIN \
	if ((endaddr) < (entry)->vme_end) \
	        _vm_map_clip_end(&(copy)->cpy_hdr,(entry),(endaddr)); \
	MACRO_END

  
                                                    
                           
   
static void
_vm_map_clip_end(
	struct vm_map_header    *map_header,
	vm_map_entry_t          entry,
	vm_map_offset_t         end)
{
	vm_map_entry_t  new_entry;

	  
                                    
                             
    

	if (entry->map_aligned) {
		assert(VM_MAP_PAGE_ALIGNED(end,
		    VM_MAP_HDR_PAGE_MASK(map_header)));
	}

	new_entry = _vm_map_entry_create(map_header, !map_header->entries_pageable);
	vm_map_entry_copy_full(new_entry, entry);

	assert(entry->vme_start < end);
	new_entry->vme_start = entry->vme_end = end;
	VME_OFFSET_SET(new_entry,
	    VME_OFFSET(new_entry) + (end - entry->vme_start));
	assert(new_entry->vme_start < new_entry->vme_end);

	_vm_map_store_entry_link(map_header, entry, new_entry);

	if (entry->is_sub_map) {
		vm_map_reference(VME_SUBMAP(new_entry));
	} else {
		vm_object_reference(VME_OBJECT(new_entry));
	}
}


  
                                            
  
                                              
                                                    
   
#define VM_MAP_RANGE_CHECK(map, start, end)     \
	MACRO_BEGIN                             \
	if (start < vm_map_min(map))            \
	        start = vm_map_min(map);        \
	if (end > vm_map_max(map))              \
	        end = vm_map_max(map);          \
	if (start > end)                        \
	        start = end;                    \
	MACRO_END

  
                                            
  
                                                           
                                                         
                                                             
                                                        
                                                              
                                                                 
                                                              
                 
  
                                                             
   
static boolean_t
vm_map_range_check(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_map_entry_t          *entry)
{
	vm_map_entry_t          cur;
	vm_map_offset_t         prev;

	  
                                  
    
	if (start < vm_map_min(map) || end > vm_map_max(map) || start > end) {
		return FALSE;
	}

	  
                                                        
                        
    
	if (!vm_map_lookup_entry(map, start, &cur)) {
		return FALSE;
	}

	  
                                                      
                          
    
	if (entry != (vm_map_entry_t *) NULL) {
		*entry = cur;
	}
	if (end <= cur->vme_end) {
		return TRUE;
	}

	  
                                                       
                                                          
    
	prev = cur->vme_end;
	cur = cur->vme_next;
	while ((cur != vm_map_to_entry(map)) && (prev == cur->vme_start)) {
		if (end <= cur->vme_end) {
			return TRUE;
		}
		prev = cur->vme_end;
		cur = cur->vme_next;
	}
	return FALSE;
}

  
                                      
  
                                                        
  
                                                           
                                                              
                                                          
  
                                                       
                                                
            
                             
  
                                                    
                                                    
                                                    
   
kern_return_t
vm_map_submap(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end,
	vm_map_t        submap,
	vm_map_offset_t offset,
#ifdef NO_NESTED_PMAP
	__unused
#endif                      
	boolean_t       use_pmap)
{
	vm_map_entry_t          entry;
	kern_return_t           result = KERN_INVALID_ARGUMENT;
	vm_object_t             object;

	vm_map_lock(map);

	if (!vm_map_lookup_entry(map, start, &entry)) {
		entry = entry->vme_next;
	}

	if (entry == vm_map_to_entry(map) ||
	    entry->is_sub_map) {
		vm_map_unlock(map);
		return KERN_INVALID_ARGUMENT;
	}

	vm_map_clip_start(map, entry, start);
	vm_map_clip_end(map, entry, end);

	if ((entry->vme_start == start) && (entry->vme_end == end) &&
	    (!entry->is_sub_map) &&
	    ((object = VME_OBJECT(entry)) == vm_submap_object) &&
	    (object->resident_page_count == 0) &&
	    (object->copy == VM_OBJECT_NULL) &&
	    (object->shadow == VM_OBJECT_NULL) &&
	    (!object->pager_created)) {
		VME_OFFSET_SET(entry, (vm_object_offset_t)offset);
		VME_OBJECT_SET(entry, VM_OBJECT_NULL);
		vm_object_deallocate(object);
		entry->is_sub_map = TRUE;
		entry->use_pmap = FALSE;
		VME_SUBMAP_SET(entry, submap);
		vm_map_reference(submap);
		if (submap->mapped_in_other_pmaps == FALSE &&
		    vm_map_pmap(submap) != PMAP_NULL &&
		    vm_map_pmap(submap) != vm_map_pmap(map)) {
			  
                                          
                                 
                                          
                                     
                                           
                                  
      
			submap->mapped_in_other_pmaps = TRUE;
		}

#ifndef NO_NESTED_PMAP
		if (use_pmap) {
			                                      
			if (submap->pmap == NULL) {
				ledger_t ledger = map->pmap->ledger;
				submap->pmap = pmap_create_options(ledger,
				    (vm_map_size_t) 0, 0);
				if (submap->pmap == PMAP_NULL) {
					vm_map_unlock(map);
					return KERN_NO_SPACE;
				}
#if     defined(__arm__) || defined(__arm64__)
				pmap_set_nested(submap->pmap);
#endif
			}
			result = pmap_nest(map->pmap,
			    (VME_SUBMAP(entry))->pmap,
			    (addr64_t)start,
			    (uint64_t)(end - start));
			if (result) {
				panic("vm_map_submap: pmap_nest failed, rc = %08X\n", result);
			}
			entry->use_pmap = TRUE;
		}
#else                       
		pmap_remove(map->pmap, (addr64_t)start, (addr64_t)end);
#endif                      
		result = KERN_SUCCESS;
	}
	vm_map_unlock(map);

	return result;
}

  
                  
  
                                               
                                             
                                                  
                                                      
   
kern_return_t
vm_map_protect(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end,
	vm_prot_t       new_prot,
	boolean_t       set_max)
{
	vm_map_entry_t                  current;
	vm_map_offset_t                 prev;
	vm_map_entry_t                  entry;
	vm_prot_t                       new_max;
	int                             pmap_options = 0;
	kern_return_t                   kr;

	if (new_prot & VM_PROT_COPY) {
		vm_map_offset_t         new_start;
		vm_prot_t               cur_prot, max_prot;
		vm_map_kernel_flags_t   kflags;

		                          
		if (start >= map->max_offset) {
			return KERN_INVALID_ADDRESS;
		}

		if ((new_prot & VM_PROT_EXECUTE) &&
		    map->pmap != kernel_pmap &&
		    (vm_map_cs_enforcement(map)
#if XNU_TARGET_OS_OSX && __arm64__
		    || !VM_MAP_IS_EXOTIC(map)
#endif                                     
		    ) &&
		    VM_MAP_POLICY_WX_FAIL(map)) {
			DTRACE_VM3(cs_wx,
			    uint64_t, (uint64_t) start,
			    uint64_t, (uint64_t) end,
			    vm_prot_t, new_prot);
			printf("CODE SIGNING: %d[%s] %s can't have both write and exec at the same time\n",
			    proc_selfpid(),
			    (current_task()->bsd_info
			    ? proc_name_address(current_task()->bsd_info)
			    : "?"),
			    __FUNCTION__);
			return KERN_PROTECTION_FAILURE;
		}

		  
                                                          
                                 
                                               
                                                                 
                                                                
                
                                                              
                                                                 
          
     
		max_prot = new_prot & VM_PROT_ALL;
		cur_prot = VM_PROT_NONE;
		kflags = VM_MAP_KERNEL_FLAGS_NONE;
		kflags.vmkf_remap_prot_copy = TRUE;
		kflags.vmkf_overwrite_immutable = TRUE;
		new_start = start;
		kr = vm_map_remap(map,
		    &new_start,
		    end - start,
		    0,           
		    VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE,
		    kflags,
		    0,
		    map,
		    start,
		    TRUE,                               
		    &cur_prot,             
		    &max_prot,             
		    VM_INHERIT_DEFAULT);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		new_prot &= ~VM_PROT_COPY;
	}

	vm_map_lock(map);

	                                                        
                                                          
                               
    
	if (start >= map->max_offset) {
		vm_map_unlock(map);
		return KERN_INVALID_ADDRESS;
	}

	while (1) {
		  
                                                           
                            
     
		if (!vm_map_lookup_entry(map, start, &entry)) {
			vm_map_unlock(map);
			return KERN_INVALID_ADDRESS;
		}

		if (entry->superpage_size && (start & (SUPERPAGE_SIZE - 1))) {                                    
			start = SUPERPAGE_ROUND_DOWN(start);
			continue;
		}
		break;
	}
	if (entry->superpage_size) {
		end = SUPERPAGE_ROUND_UP(end);
	}

	  
                                                         
               
    

	current = entry;
	prev = current->vme_start;
	while ((current != vm_map_to_entry(map)) &&
	    (current->vme_start < end)) {
		  
                                         
     
		if (current->vme_start != prev) {
			vm_map_unlock(map);
			return KERN_INVALID_ADDRESS;
		}

		new_max = current->max_protection;
		if ((new_prot & new_max) != new_prot) {
			vm_map_unlock(map);
			return KERN_PROTECTION_FAILURE;
		}

		if (current->used_for_jit &&
		    pmap_has_prot_policy(map->pmap, current->translated_allow_execute, current->protection)) {
			vm_map_unlock(map);
			return KERN_PROTECTION_FAILURE;
		}

		if ((new_prot & VM_PROT_WRITE) &&
		    (new_prot & VM_PROT_EXECUTE) &&
#if XNU_TARGET_OS_OSX
		    map->pmap != kernel_pmap &&
		    (vm_map_cs_enforcement(map)
#if __arm64__
		    || !VM_MAP_IS_EXOTIC(map)
#endif                
		    ) &&
#endif                        
		    !(current->used_for_jit)) {
			DTRACE_VM3(cs_wx,
			    uint64_t, (uint64_t) current->vme_start,
			    uint64_t, (uint64_t) current->vme_end,
			    vm_prot_t, new_prot);
			printf("CODE SIGNING: %d[%s] %s can't have both write and exec at the same time\n",
			    proc_selfpid(),
			    (current_task()->bsd_info
			    ? proc_name_address(current_task()->bsd_info)
			    : "?"),
			    __FUNCTION__);
			new_prot &= ~VM_PROT_EXECUTE;
			if (VM_MAP_POLICY_WX_FAIL(map)) {
				vm_map_unlock(map);
				return KERN_PROTECTION_FAILURE;
			}
		}

		  
                                                   
               
                                       
                                                                  
     
		if (map->map_disallow_new_exec == TRUE) {
			if ((new_prot & VM_PROT_EXECUTE) ||
			    ((current->protection & VM_PROT_EXECUTE) && (new_prot & VM_PROT_WRITE))) {
				vm_map_unlock(map);
				return KERN_PROTECTION_FAILURE;
			}
		}

		prev = current->vme_end;
		current = current->vme_next;
	}

#if __arm64__
	if (end > prev &&
	    end == vm_map_round_page(prev, VM_MAP_PAGE_MASK(map))) {
		vm_map_entry_t prev_entry;

		prev_entry = current->vme_prev;
		if (prev_entry != vm_map_to_entry(map) &&
		    !prev_entry->map_aligned &&
		    (vm_map_round_page(prev_entry->vme_end,
		    VM_MAP_PAGE_MASK(map))
		    == end)) {
			  
                                                      
                                                    
                                                       
                                             
      
			prev = end;
		}
	}
#endif                

	if (end > prev) {
		vm_map_unlock(map);
		return KERN_INVALID_ADDRESS;
	}

	  
                                   
                                                 
              
    

	current = entry;
	if (current != vm_map_to_entry(map)) {
		                                  
		vm_map_clip_start(map, current, start);
	}

	while ((current != vm_map_to_entry(map)) &&
	    (current->vme_start < end)) {
		vm_prot_t       old_prot;

		vm_map_clip_end(map, current, end);

		if (current->is_sub_map) {
			                                   
			assert(!current->use_pmap);
		}

		old_prot = current->protection;

		if (set_max) {
			current->max_protection = new_prot;
			current->protection = new_prot & old_prot;
		} else {
			current->protection = new_prot;
		}

		  
                                      
                                                    
                                                       
                                                         
                                                
                                                      
                          
     
		if (current->protection != old_prot) {
			                                               
			                                                  
			                

			vm_prot_t prot;

			prot = current->protection;
			if (current->is_sub_map || (VME_OBJECT(current) == NULL) || (VME_OBJECT(current) != compressor_object)) {
				prot &= ~VM_PROT_WRITE;
			} else {
				assert(!VME_OBJECT(current)->code_signed);
				assert(VME_OBJECT(current)->copy_strategy == MEMORY_OBJECT_COPY_NONE);
			}

			if (override_nx(map, VME_ALIAS(current)) && prot) {
				prot |= VM_PROT_EXECUTE;
			}

#if DEVELOPMENT || DEBUG
			if (!(old_prot & VM_PROT_EXECUTE) &&
			    (prot & VM_PROT_EXECUTE) &&
			    panic_on_unsigned_execute &&
			    (proc_selfcsflags() & CS_KILL)) {
				panic("vm_map_protect(%p,0x%llx,0x%llx) old=0x%x new=0x%x - <rdar://23770418> code-signing bypass?\n", map, (uint64_t)current->vme_start, (uint64_t)current->vme_end, old_prot, prot);
			}
#endif                           

			if (pmap_has_prot_policy(map->pmap, current->translated_allow_execute, prot)) {
				if (current->wired_count) {
					panic("vm_map_protect(%p,0x%llx,0x%llx) new=0x%x wired=%x\n",
					    map, (uint64_t)current->vme_start, (uint64_t)current->vme_end, prot, current->wired_count);
				}

				                                     
                                         
                                      
                                        
                          
       
				  
                                        
                                     
                               
                                        
               
       
				prot = VM_PROT_NONE;
			}

			if (current->is_sub_map && current->use_pmap) {
				pmap_protect(VME_SUBMAP(current)->pmap,
				    current->vme_start,
				    current->vme_end,
				    prot);
			} else {
				if (prot & VM_PROT_WRITE) {
					if (VME_OBJECT(current) == compressor_object) {
						  
                                  
                                   
                                      
                                     
                                      
              
         
						pmap_options |= PMAP_OPTIONS_PROTECT_IMMEDIATE;
					}
				}

				pmap_protect_options(map->pmap,
				    current->vme_start,
				    current->vme_end,
				    prot,
				    pmap_options,
				    NULL);
			}
		}
		current = current->vme_next;
	}

	current = entry;
	while ((current != vm_map_to_entry(map)) &&
	    (current->vme_start <= end)) {
		vm_map_simplify_entry(map, current);
		current = current->vme_next;
	}

	vm_map_unlock(map);
	return KERN_SUCCESS;
}

  
                  
  
                                                
                                        
                                          
                                         
   
kern_return_t
vm_map_inherit(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end,
	vm_inherit_t    new_inheritance)
{
	vm_map_entry_t  entry;
	vm_map_entry_t  temp_entry;

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &temp_entry)) {
		entry = temp_entry;
	} else {
		temp_entry = temp_entry->vme_next;
		entry = temp_entry;
	}

	                                                                  
	                        
	while ((entry != vm_map_to_entry(map)) && (entry->vme_start < end)) {
		if (entry->is_sub_map) {
			if (new_inheritance == VM_INHERIT_COPY) {
				vm_map_unlock(map);
				return KERN_INVALID_ARGUMENT;
			}
		}

		entry = entry->vme_next;
	}

	entry = temp_entry;
	if (entry != vm_map_to_entry(map)) {
		                                  
		vm_map_clip_start(map, entry, start);
	}

	while ((entry != vm_map_to_entry(map)) && (entry->vme_start < end)) {
		vm_map_clip_end(map, entry, end);
		if (entry->is_sub_map) {
			                               
			assert(!entry->use_pmap);
		}

		entry->inheritance = new_inheritance;

		entry = entry->vme_next;
	}

	vm_map_unlock(map);
	return KERN_SUCCESS;
}

  
                                                                                     
                                                                                          
   

static kern_return_t
add_wire_counts(
	vm_map_t        map,
	vm_map_entry_t  entry,
	boolean_t       user_wire)
{
	vm_map_size_t   size;

	if (user_wire) {
		unsigned int total_wire_count =  vm_page_wire_count + vm_lopage_free_count;

		  
                                                                                                        
                    
     

		if (entry->user_wired_count == 0) {
			size = entry->vme_end - entry->vme_start;

			  
                                                                                           
                                                                                              
                                                                                                          
                                                                                                 
                          
      

			if (size + map->user_wire_size > MIN(map->user_wire_limit, vm_per_task_user_wire_limit) ||
			    size + ptoa_64(total_wire_count) > vm_global_user_wire_limit) {
				if (size + ptoa_64(total_wire_count) > vm_global_user_wire_limit) {
					os_atomic_inc(&vm_add_wire_count_over_global_limit, relaxed);
				} else {
					os_atomic_inc(&vm_add_wire_count_over_user_limit, relaxed);
				}
				return KERN_RESOURCE_SHORTAGE;
			}

			  
                                                                                               
                                               
      

			if (entry->wired_count >= MAX_WIRE_COUNT) {
				return KERN_FAILURE;
			}

			entry->wired_count++;
			map->user_wire_size += size;
		}

		if (entry->user_wired_count >= MAX_WIRE_COUNT) {
			return KERN_FAILURE;
		}

		entry->user_wired_count++;
	} else {
		  
                                                                       
     

		if (entry->wired_count >= MAX_WIRE_COUNT) {
			panic("vm_map_wire: too many wirings");
		}

		entry->wired_count++;
	}

	return KERN_SUCCESS;
}

  
                                                                                     
   

static void
subtract_wire_counts(
	vm_map_t        map,
	vm_map_entry_t  entry,
	boolean_t       user_wire)
{
	if (user_wire) {
		  
                                                                                                           
     

		if (entry->user_wired_count == 1) {
			  
                                                                                           
                                     
      

			assert(entry->wired_count >= 1);
			entry->wired_count--;
			map->user_wire_size -= entry->vme_end - entry->vme_start;
		}

		assert(entry->user_wired_count >= 1);
		entry->user_wired_count--;
	} else {
		  
                                                                
     

		assert(entry->wired_count >= 1);
		entry->wired_count--;
	}
}

int cs_executable_wire = 0;

  
               
  
                                                             
                                                                  
                                                           
                                                                 
                                                               
                            
  
                                                                 
                           
   
static kern_return_t
vm_map_wire_nested(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_prot_t               caller_prot,
	vm_tag_t                tag,
	boolean_t               user_wire,
	pmap_t                  map_pmap,
	vm_map_offset_t         pmap_addr,
	ppnum_t                 *physpage_p)
{
	vm_map_entry_t          entry;
	vm_prot_t               access_type;
	struct vm_map_entry     *first_entry, tmp_entry;
	vm_map_t                real_map;
	vm_map_offset_t         s, e;
	kern_return_t           rc;
	boolean_t               need_wakeup;
	boolean_t               main_map = FALSE;
	wait_interrupt_t        interruptible_state;
	thread_t                cur_thread;
	unsigned int            last_timestamp;
	vm_map_size_t           size;
	boolean_t               wire_and_extract;
	vm_prot_t               extra_prots;

	extra_prots = VM_PROT_COPY;
	extra_prots |= VM_PROT_COPY_FAIL_IF_EXECUTABLE;
#if XNU_TARGET_OS_OSX
	if (map->pmap == kernel_pmap ||
	    !vm_map_cs_enforcement(map)) {
		extra_prots &= ~VM_PROT_COPY_FAIL_IF_EXECUTABLE;
	}
#endif                        

	access_type = (caller_prot & VM_PROT_ALL);

	wire_and_extract = FALSE;
	if (physpage_p != NULL) {
		  
                                                     
                                                         
                                               
     
		if ((end - start) != PAGE_SIZE) {
			return KERN_INVALID_ARGUMENT;
		}
		wire_and_extract = TRUE;
		*physpage_p = 0;
	}

	vm_map_lock(map);
	if (map_pmap == NULL) {
		main_map = TRUE;
	}
	last_timestamp = map->timestamp;

	VM_MAP_RANGE_CHECK(map, start, end);
	assert(VM_MAP_PAGE_ALIGNED(start, VM_MAP_PAGE_MASK(map)));
	assert(VM_MAP_PAGE_ALIGNED(end, VM_MAP_PAGE_MASK(map)));

	if (start == end) {
		                                                    
		vm_map_unlock(map);
		return KERN_SUCCESS;
	}

	need_wakeup = FALSE;
	cur_thread = current_thread();

	s = start;
	rc = KERN_SUCCESS;

	if (vm_map_lookup_entry(map, s, &first_entry)) {
		entry = first_entry;
		  
                                          
                                                      
     
	} else {
		                                 
		rc = KERN_INVALID_ADDRESS;
		goto done;
	}

	while ((entry != vm_map_to_entry(map)) && (s < end)) {
		  
                                                      
                                             
    
                                                              
                            
     

		                                                  
		e = entry->vme_end;
		if (e > end) {
			e = end;
		}

		  
                                                         
                                                      
     
		if (entry->in_transition) {
			wait_result_t wait_result;

			  
                                                    
                                                      
                         
                                                      
                                                 
                                
      

			entry->needs_wakeup = TRUE;

			  
                                                     
                    
      
			if (need_wakeup) {
				vm_map_entry_wakeup(map);
				need_wakeup = FALSE;
			}
			  
                                  
      
			wait_result = vm_map_entry_wait(map,
			    (user_wire) ? THREAD_ABORTSAFE :
			    THREAD_UNINT);
			if (user_wire && wait_result == THREAD_INTERRUPTED) {
				  
                                           
                                             
                                            
                        
       
				rc = KERN_FAILURE;
				goto done;
			}

			  
                                                  
      
			last_timestamp = map->timestamp;

			  
                                                          
                                                         
      
			if (!vm_map_lookup_entry(map, s, &first_entry)) {
				  
                                              
                                            
                                          
       
				rc = KERN_FAILURE;
				goto done;
			}
			entry = first_entry;
			continue;
		}

		if (entry->is_sub_map) {
			vm_map_offset_t sub_start;
			vm_map_offset_t sub_end;
			vm_map_offset_t local_start;
			vm_map_offset_t local_end;
			pmap_t          pmap;

			if (wire_and_extract) {
				  
                                           
                                         
                                            
                               
       
				rc = KERN_INVALID_ARGUMENT;
				goto done;
			}

			vm_map_clip_start(map, entry, s);
			vm_map_clip_end(map, entry, end);

			sub_start = VME_OFFSET(entry);
			sub_end = entry->vme_end;
			sub_end += VME_OFFSET(entry) - entry->vme_start;

			local_end = entry->vme_end;
			if (map_pmap == NULL) {
				vm_object_t             object;
				vm_object_offset_t      offset;
				vm_prot_t               prot;
				boolean_t               wired;
				vm_map_entry_t          local_entry;
				vm_map_version_t         version;
				vm_map_t                lookup_map;

				if (entry->use_pmap) {
					pmap = VME_SUBMAP(entry)->pmap;
					                                      
					                                      
					                        
#ifdef notdef
					pmap_addr = sub_start;
#endif
					pmap_addr = s;
				} else {
					pmap = map->pmap;
					pmap_addr = s;
				}

				if (entry->wired_count) {
					if ((rc = add_wire_counts(map, entry, user_wire)) != KERN_SUCCESS) {
						goto done;
					}

					  
                                 
                                  
                                       
        
					entry = entry->vme_next;
					s = entry->vme_start;
					continue;
				}

				                                  
				                                  
				               
				local_start = entry->vme_start;
				lookup_map = map;
				vm_map_lock_write_to_read(map);
				rc = vm_map_lookup_locked(
					&lookup_map, local_start,
					(access_type | extra_prots),
					OBJECT_LOCK_EXCLUSIVE,
					&version, &object,
					&offset, &prot, &wired,
					NULL,
					&real_map, NULL);
				if (rc != KERN_SUCCESS) {
					vm_map_unlock_read(lookup_map);
					assert(map_pmap == NULL);
					vm_map_unwire(map, start,
					    s, user_wire);
					return rc;
				}
				vm_object_unlock(object);
				if (real_map != lookup_map) {
					vm_map_unlock(real_map);
				}
				vm_map_unlock_read(lookup_map);
				vm_map_lock(map);

				                                    
				if (!vm_map_lookup_entry(map,
				    local_start,
				    &local_entry)) {
					rc = KERN_FAILURE;
					goto done;
				}

				  
                                          
                 
       
				entry = local_entry;
				assert(s == local_start);
				vm_map_clip_start(map, entry, s);
				vm_map_clip_end(map, entry, end);
				                    
				e = entry->vme_end;
				if (e > end) {
					e = end;
				}

				                                   
				if (!entry->is_sub_map) {
					last_timestamp = map->timestamp;
					continue;
				}
			} else {
				local_start = entry->vme_start;
				pmap = map_pmap;
			}

			if ((rc = add_wire_counts(map, entry, user_wire)) != KERN_SUCCESS) {
				goto done;
			}

			entry->in_transition = TRUE;

			vm_map_unlock(map);
			rc = vm_map_wire_nested(VME_SUBMAP(entry),
			    sub_start, sub_end,
			    caller_prot, tag,
			    user_wire, pmap, pmap_addr,
			    NULL);
			vm_map_lock(map);

			  
                                                       
                                
      
			if (!vm_map_lookup_entry(map, local_start,
			    &first_entry)) {
				panic("vm_map_wire: re-lookup failed");
			}
			entry = first_entry;

			assert(local_start == s);
			                    
			e = entry->vme_end;
			if (e > end) {
				e = end;
			}

			last_timestamp = map->timestamp;
			while ((entry != vm_map_to_entry(map)) &&
			    (entry->vme_start < e)) {
				assert(entry->in_transition);
				entry->in_transition = FALSE;
				if (entry->needs_wakeup) {
					entry->needs_wakeup = FALSE;
					need_wakeup = TRUE;
				}
				if (rc != KERN_SUCCESS) {                    
					subtract_wire_counts(map, entry, user_wire);
				}
				entry = entry->vme_next;
			}
			if (rc != KERN_SUCCESS) {                           
				goto done;
			}

			                               
			s = entry->vme_start;
			continue;
		}

		  
                                                  
                                          
     
		if (entry->wired_count) {
			if ((entry->protection & access_type) != access_type) {
				                                

				  
               
                                       
                                       
                                  
                                             
                           
       
				if (wire_and_extract) {
					rc = KERN_PROTECTION_FAILURE;
					goto done;
				}
			}

			  
                                                    
                                  
      
			vm_map_clip_start(map, entry, s);
			vm_map_clip_end(map, entry, end);

			if ((rc = add_wire_counts(map, entry, user_wire)) != KERN_SUCCESS) {
				goto done;
			}

			if (wire_and_extract) {
				vm_object_t             object;
				vm_object_offset_t      offset;
				vm_page_t               m;

				  
                                             
                                         
                                              
              
       
				assert((entry->vme_end - entry->vme_start)
				    == PAGE_SIZE);
				assert(!entry->needs_copy);
				assert(!entry->is_sub_map);
				assert(VME_OBJECT(entry));
				if (((entry->vme_end - entry->vme_start)
				    != PAGE_SIZE) ||
				    entry->needs_copy ||
				    entry->is_sub_map ||
				    VME_OBJECT(entry) == VM_OBJECT_NULL) {
					rc = KERN_INVALID_ARGUMENT;
					goto done;
				}

				object = VME_OBJECT(entry);
				offset = VME_OFFSET(entry);
				                                            
				if (entry->protection & VM_PROT_WRITE) {
					vm_object_lock(object);
				} else {
					vm_object_lock_shared(object);
				}
				m = vm_page_lookup(object, offset);
				assert(m != VM_PAGE_NULL);
				assert(VM_PAGE_WIRED(m));
				if (m != VM_PAGE_NULL && VM_PAGE_WIRED(m)) {
					*physpage_p = VM_PAGE_GET_PHYS_PAGE(m);
					if (entry->protection & VM_PROT_WRITE) {
						vm_object_lock_assert_exclusive(
							object);
						m->vmp_dirty = TRUE;
					}
				} else {
					                          
					*physpage_p = 0;
				}
				vm_object_unlock(object);
			}

			                                               
			entry = entry->vme_next;
			s = entry->vme_start;
			continue;
		}

		  
                                                         
     

		  
                                                      
                                                  
                                                       
                                            
     

		if ((entry->protection & VM_PROT_EXECUTE)
#if XNU_TARGET_OS_OSX
		    &&
		    map->pmap != kernel_pmap &&
		    (vm_map_cs_enforcement(map)
#if __arm64__
		    || !VM_MAP_IS_EXOTIC(map)
#endif                
		    )
#endif                        
		    ) {
#if MACH_ASSERT
			printf("pid %d[%s] wiring executable range from "
			    "0x%llx to 0x%llx: rejected to preserve "
			    "code-signing\n",
			    proc_selfpid(),
			    (current_task()->bsd_info
			    ? proc_name_address(current_task()->bsd_info)
			    : "?"),
			    (uint64_t) entry->vme_start,
			    (uint64_t) entry->vme_end);
#endif                  
			DTRACE_VM2(cs_executable_wire,
			    uint64_t, (uint64_t)entry->vme_start,
			    uint64_t, (uint64_t)entry->vme_end);
			cs_executable_wire++;
			rc = KERN_PROTECTION_FAILURE;
			goto done;
		}

		  
                                                         
                                                  
                                                       
            
     
		size = entry->vme_end - entry->vme_start;
		  
                                                           
                                                           
                                                              
     
		if (entry->needs_copy) {
			if (wire_and_extract) {
				  
                                                
                                             
       
				rc = KERN_INVALID_ARGUMENT;
				goto done;
			}

			VME_OBJECT_SHADOW(entry, size);
			entry->needs_copy = FALSE;
		} else if (VME_OBJECT(entry) == VM_OBJECT_NULL) {
			if (wire_and_extract) {
				  
                                                
                                                 
       
				rc = KERN_INVALID_ARGUMENT;
				goto done;
			}
			VME_OBJECT_SET(entry, vm_object_allocate(size));
			VME_OFFSET_SET(entry, (vm_object_offset_t)0);
			assert(entry->use_pmap);
		}

		vm_map_clip_start(map, entry, s);
		vm_map_clip_end(map, entry, end);

		                    
		e = entry->vme_end;
		if (e > end) {
			e = end;
		}

		  
                                             
                                                       
                                
                                                         
                                  
     
		if ((entry->vme_end < end) &&
		    ((entry->vme_next == vm_map_to_entry(map)) ||
		    (entry->vme_next->vme_start > entry->vme_end))) {
			                  
			rc = KERN_INVALID_ADDRESS;
			goto done;
		}
		if ((entry->protection & access_type) != access_type) {
			                                
			rc = KERN_PROTECTION_FAILURE;
			goto done;
		}

		assert(entry->wired_count == 0 && entry->user_wired_count == 0);

		if ((rc = add_wire_counts(map, entry, user_wire)) != KERN_SUCCESS) {
			goto done;
		}

		entry->in_transition = TRUE;

		  
                                                       
                                                     
                                                      
                                                        
                                                      
                                                     
                          
     
		tmp_entry = *entry;

		  
                                                      
                                                          
                                                             
     
		vm_map_unlock(map);

		if (!user_wire && cur_thread != THREAD_NULL) {
			interruptible_state = thread_interrupt_level(THREAD_UNINT);
		} else {
			interruptible_state = THREAD_UNINT;
		}

		if (map_pmap) {
			rc = vm_fault_wire(map,
			    &tmp_entry, caller_prot, tag, map_pmap, pmap_addr,
			    physpage_p);
		} else {
			rc = vm_fault_wire(map,
			    &tmp_entry, caller_prot, tag, map->pmap,
			    tmp_entry.vme_start,
			    physpage_p);
		}

		if (!user_wire && cur_thread != THREAD_NULL) {
			thread_interrupt_level(interruptible_state);
		}

		vm_map_lock(map);

		if (last_timestamp + 1 != map->timestamp) {
			  
                                                       
                                
      
			if (!vm_map_lookup_entry(map, tmp_entry.vme_start,
			    &first_entry)) {
				panic("vm_map_wire: re-lookup failed");
			}

			entry = first_entry;
		}

		last_timestamp = map->timestamp;

		while ((entry != vm_map_to_entry(map)) &&
		    (entry->vme_start < tmp_entry.vme_end)) {
			assert(entry->in_transition);
			entry->in_transition = FALSE;
			if (entry->needs_wakeup) {
				entry->needs_wakeup = FALSE;
				need_wakeup = TRUE;
			}
			if (rc != KERN_SUCCESS) {                           
				subtract_wire_counts(map, entry, user_wire);
			}
			entry = entry->vme_next;
		}

		if (rc != KERN_SUCCESS) {                                   
			goto done;
		}

		if ((entry != vm_map_to_entry(map)) &&                                       
		    (tmp_entry.vme_end != end) &&                                                           
		    (entry->vme_start != tmp_entry.vme_end)) {                                             
			                        
			s = tmp_entry.vme_end;
			rc = KERN_INVALID_ADDRESS;
			goto done;
		}

		s = entry->vme_start;
	}                                         

done:
	if (rc == KERN_SUCCESS) {
		                                                      
		vm_map_simplify_range(map, start, end);
	}

	vm_map_unlock(map);

	  
                                                
    
	if (need_wakeup) {
		vm_map_entry_wakeup(map);
	}

	if (rc != KERN_SUCCESS) {
		                                     
		vm_map_unwire_nested(map, start, s, user_wire,
		    map_pmap, pmap_addr);
		if (physpage_p) {
			*physpage_p = 0;
		}
	}

	return rc;
}

kern_return_t
vm_map_wire_external(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_prot_t               caller_prot,
	boolean_t               user_wire)
{
	kern_return_t   kret;

	kret = vm_map_wire_nested(map, start, end, caller_prot, vm_tag_bt(),
	    user_wire, (pmap_t)NULL, 0, NULL);
	return kret;
}

kern_return_t
vm_map_wire_kernel(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_prot_t               caller_prot,
	vm_tag_t                tag,
	boolean_t               user_wire)
{
	kern_return_t   kret;

	kret = vm_map_wire_nested(map, start, end, caller_prot, tag,
	    user_wire, (pmap_t)NULL, 0, NULL);
	return kret;
}

kern_return_t
vm_map_wire_and_extract_external(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_prot_t       caller_prot,
	boolean_t       user_wire,
	ppnum_t         *physpage_p)
{
	kern_return_t   kret;

	kret = vm_map_wire_nested(map,
	    start,
	    start + VM_MAP_PAGE_SIZE(map),
	    caller_prot,
	    vm_tag_bt(),
	    user_wire,
	    (pmap_t)NULL,
	    0,
	    physpage_p);
	if (kret != KERN_SUCCESS &&
	    physpage_p != NULL) {
		*physpage_p = 0;
	}
	return kret;
}

kern_return_t
vm_map_wire_and_extract_kernel(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_prot_t       caller_prot,
	vm_tag_t        tag,
	boolean_t       user_wire,
	ppnum_t         *physpage_p)
{
	kern_return_t   kret;

	kret = vm_map_wire_nested(map,
	    start,
	    start + VM_MAP_PAGE_SIZE(map),
	    caller_prot,
	    tag,
	    user_wire,
	    (pmap_t)NULL,
	    0,
	    physpage_p);
	if (kret != KERN_SUCCESS &&
	    physpage_p != NULL) {
		*physpage_p = 0;
	}
	return kret;
}

  
                 
  
                                                                    
                                                                   
  
                                                                     
                       
  
                                                                
                                                                     
              
   
static kern_return_t
vm_map_unwire_nested(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	boolean_t               user_wire,
	pmap_t                  map_pmap,
	vm_map_offset_t         pmap_addr)
{
	vm_map_entry_t          entry;
	struct vm_map_entry     *first_entry, tmp_entry;
	boolean_t               need_wakeup;
	boolean_t               main_map = FALSE;
	unsigned int            last_timestamp;

	vm_map_lock(map);
	if (map_pmap == NULL) {
		main_map = TRUE;
	}
	last_timestamp = map->timestamp;

	VM_MAP_RANGE_CHECK(map, start, end);
	assert(VM_MAP_PAGE_ALIGNED(start, VM_MAP_PAGE_MASK(map)));
	assert(VM_MAP_PAGE_ALIGNED(end, VM_MAP_PAGE_MASK(map)));

	if (start == end) {
		                                                      
		vm_map_unlock(map);
		return KERN_SUCCESS;
	}

	if (vm_map_lookup_entry(map, start, &first_entry)) {
		entry = first_entry;
		  
                                          
                                                       
     
	} else {
		if (!user_wire) {
			panic("vm_map_unwire: start not found");
		}
		                                  
		vm_map_unlock(map);
		return KERN_INVALID_ADDRESS;
	}

	if (entry->superpage_size) {
		                                 
		vm_map_unlock(map);
		return KERN_INVALID_ADDRESS;
	}

	need_wakeup = FALSE;
	while ((entry != vm_map_to_entry(map)) && (entry->vme_start < end)) {
		if (entry->in_transition) {
			  
        
                                                    
                                                     
                                                
                                                        
                      
     
        
                                                        
                                                     
                                            
      
			if (!user_wire) {
				  
               
                                              
                                             
                
                                              
                                               
                                         
                                            
                                          
                                         
                                                  
                                
      
                                              
                                             
                                                   
                                                
                                                
                
       
				panic("vm_map_unwire: in_transition entry");
			}

			entry = entry->vme_next;
			continue;
		}

		if (entry->is_sub_map) {
			vm_map_offset_t sub_start;
			vm_map_offset_t sub_end;
			vm_map_offset_t local_end;
			pmap_t          pmap;

			vm_map_clip_start(map, entry, start);
			vm_map_clip_end(map, entry, end);

			sub_start = VME_OFFSET(entry);
			sub_end = entry->vme_end - entry->vme_start;
			sub_end += VME_OFFSET(entry);
			local_end = entry->vme_end;
			if (map_pmap == NULL) {
				if (entry->use_pmap) {
					pmap = VME_SUBMAP(entry)->pmap;
					pmap_addr = sub_start;
				} else {
					pmap = map->pmap;
					pmap_addr = start;
				}
				if (entry->wired_count == 0 ||
				    (user_wire && entry->user_wired_count == 0)) {
					if (!user_wire) {
						panic("vm_map_unwire: entry is unwired");
					}
					entry = entry->vme_next;
					continue;
				}

				  
                      
                                                    
                                     
       
				if (((entry->vme_end < end) &&
				    ((entry->vme_next == vm_map_to_entry(map)) ||
				    (entry->vme_next->vme_start
				    > entry->vme_end)))) {
					if (!user_wire) {
						panic("vm_map_unwire: non-contiguous region");
					}
  
                                                                 
                                                  
   
				}

				subtract_wire_counts(map, entry, user_wire);

				if (entry->wired_count != 0) {
					entry = entry->vme_next;
					continue;
				}

				entry->in_transition = TRUE;
				tmp_entry = *entry;                                  

				  
                                                         
                                         
       
				vm_map_unlock(map);
				vm_map_unwire_nested(VME_SUBMAP(entry),
				    sub_start, sub_end, user_wire, pmap, pmap_addr);
				vm_map_lock(map);

				if (last_timestamp + 1 != map->timestamp) {
					  
                                                 
                                                     
        
					if (!vm_map_lookup_entry(map,
					    tmp_entry.vme_start,
					    &first_entry)) {
						if (!user_wire) {
							panic("vm_map_unwire: re-lookup failed");
						}
						entry = first_entry->vme_next;
					} else {
						entry = first_entry;
					}
				}
				last_timestamp = map->timestamp;

				  
                                                       
                                                
                                           
       
				while ((entry != vm_map_to_entry(map)) &&
				    (entry->vme_start < tmp_entry.vme_end)) {
					assert(entry->in_transition);
					entry->in_transition = FALSE;
					if (entry->needs_wakeup) {
						entry->needs_wakeup = FALSE;
						need_wakeup = TRUE;
					}
					entry = entry->vme_next;
				}
				continue;
			} else {
				vm_map_unlock(map);
				vm_map_unwire_nested(VME_SUBMAP(entry),
				    sub_start, sub_end, user_wire, map_pmap,
				    pmap_addr);
				vm_map_lock(map);

				if (last_timestamp + 1 != map->timestamp) {
					  
                                                 
                                                     
        
					if (!vm_map_lookup_entry(map,
					    tmp_entry.vme_start,
					    &first_entry)) {
						if (!user_wire) {
							panic("vm_map_unwire: re-lookup failed");
						}
						entry = first_entry->vme_next;
					} else {
						entry = first_entry;
					}
				}
				last_timestamp = map->timestamp;
			}
		}


		if ((entry->wired_count == 0) ||
		    (user_wire && entry->user_wired_count == 0)) {
			if (!user_wire) {
				panic("vm_map_unwire: entry is unwired");
			}

			entry = entry->vme_next;
			continue;
		}

		assert(entry->wired_count > 0 &&
		    (!user_wire || entry->user_wired_count > 0));

		vm_map_clip_start(map, entry, start);
		vm_map_clip_end(map, entry, end);

		  
                    
                                                  
                                     
     
		if (((entry->vme_end < end) &&
		    ((entry->vme_next == vm_map_to_entry(map)) ||
		    (entry->vme_next->vme_start > entry->vme_end)))) {
			if (!user_wire) {
				panic("vm_map_unwire: non-contiguous region");
			}
			entry = entry->vme_next;
			continue;
		}

		subtract_wire_counts(map, entry, user_wire);

		if (entry->wired_count != 0) {
			entry = entry->vme_next;
			continue;
		}

		if (entry->zero_wired_pages) {
			entry->zero_wired_pages = FALSE;
		}

		entry->in_transition = TRUE;
		tmp_entry = *entry;                                       

		  
                                                       
                                       
     
		vm_map_unlock(map);
		if (map_pmap) {
			vm_fault_unwire(map,
			    &tmp_entry, FALSE, map_pmap, pmap_addr);
		} else {
			vm_fault_unwire(map,
			    &tmp_entry, FALSE, map->pmap,
			    tmp_entry.vme_start);
		}
		vm_map_lock(map);

		if (last_timestamp + 1 != map->timestamp) {
			  
                                                       
                                           
      
			if (!vm_map_lookup_entry(map, tmp_entry.vme_start,
			    &first_entry)) {
				if (!user_wire) {
					panic("vm_map_unwire: re-lookup failed");
				}
				entry = first_entry->vme_next;
			} else {
				entry = first_entry;
			}
		}
		last_timestamp = map->timestamp;

		  
                                                          
                                                           
                       
     
		while ((entry != vm_map_to_entry(map)) &&
		    (entry->vme_start < tmp_entry.vme_end)) {
			assert(entry->in_transition);
			entry->in_transition = FALSE;
			if (entry->needs_wakeup) {
				entry->needs_wakeup = FALSE;
				need_wakeup = TRUE;
			}
			entry = entry->vme_next;
		}
	}

	  
                                                                 
                                                                    
                                                          
                                                             
                                                              
                     
    
	vm_map_simplify_range(map, start, end);

	vm_map_unlock(map);
	  
                                                            
    
	if (need_wakeup) {
		vm_map_entry_wakeup(map);
	}
	return KERN_SUCCESS;
}

kern_return_t
vm_map_unwire(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	boolean_t               user_wire)
{
	return vm_map_unwire_nested(map, start, end,
	           user_wire, (pmap_t)NULL, 0);
}


  
                                             
  
                                                  
   
static void
vm_map_entry_delete(
	vm_map_t        map,
	vm_map_entry_t  entry)
{
	vm_map_offset_t s, e;
	vm_object_t     object;
	vm_map_t        submap;

	s = entry->vme_start;
	e = entry->vme_end;
	assert(VM_MAP_PAGE_ALIGNED(s, FOURK_PAGE_MASK));
	assert(VM_MAP_PAGE_ALIGNED(e, FOURK_PAGE_MASK));
	if (VM_MAP_PAGE_MASK(map) >= PAGE_MASK) {
		assert(page_aligned(s));
		assert(page_aligned(e));
	}
	if (entry->map_aligned == TRUE) {
		assert(VM_MAP_PAGE_ALIGNED(s, VM_MAP_PAGE_MASK(map)));
		assert(VM_MAP_PAGE_ALIGNED(e, VM_MAP_PAGE_MASK(map)));
	}
	assert(entry->wired_count == 0);
	assert(entry->user_wired_count == 0);
	assert(!entry->permanent);

	if (entry->is_sub_map) {
		object = NULL;
		submap = VME_SUBMAP(entry);
	} else {
		submap = NULL;
		object = VME_OBJECT(entry);
	}

	vm_map_store_entry_unlink(map, entry);
	map->size -= e - s;

	vm_map_entry_dispose(map, entry);

	vm_map_unlock(map);
	  
                                                 
                                       
    
	if (submap) {
		vm_map_deallocate(submap);
	} else {
		vm_object_deallocate(object);
	}
}

void
vm_map_submap_pmap_clean(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end,
	vm_map_t        sub_map,
	vm_map_offset_t offset)
{
	vm_map_offset_t submap_start;
	vm_map_offset_t submap_end;
	vm_map_size_t   remove_size;
	vm_map_entry_t  entry;

	submap_end = offset + (end - start);
	submap_start = offset;

	vm_map_lock_read(sub_map);
	if (vm_map_lookup_entry(sub_map, offset, &entry)) {
		remove_size = (entry->vme_end - entry->vme_start);
		if (offset > entry->vme_start) {
			remove_size -= offset - entry->vme_start;
		}


		if (submap_end < entry->vme_end) {
			remove_size -=
			    entry->vme_end - submap_end;
		}
		if (entry->is_sub_map) {
			vm_map_submap_pmap_clean(
				sub_map,
				start,
				start + remove_size,
				VME_SUBMAP(entry),
				VME_OFFSET(entry));
		} else {
			if (map->mapped_in_other_pmaps &&
			    os_ref_get_count(&map->map_refcnt) != 0 &&
			    VME_OBJECT(entry) != NULL) {
				vm_object_pmap_protect_options(
					VME_OBJECT(entry),
					(VME_OFFSET(entry) +
					offset -
					entry->vme_start),
					remove_size,
					PMAP_NULL,
					PAGE_SIZE,
					entry->vme_start,
					VM_PROT_NONE,
					PMAP_OPTIONS_REMOVE);
			} else {
				pmap_remove(map->pmap,
				    (addr64_t)start,
				    (addr64_t)(start + remove_size));
			}
		}
	}

	entry = entry->vme_next;

	while ((entry != vm_map_to_entry(sub_map))
	    && (entry->vme_start < submap_end)) {
		remove_size = (entry->vme_end - entry->vme_start);
		if (submap_end < entry->vme_end) {
			remove_size -= entry->vme_end - submap_end;
		}
		if (entry->is_sub_map) {
			vm_map_submap_pmap_clean(
				sub_map,
				(start + entry->vme_start) - offset,
				((start + entry->vme_start) - offset) + remove_size,
				VME_SUBMAP(entry),
				VME_OFFSET(entry));
		} else {
			if (map->mapped_in_other_pmaps &&
			    os_ref_get_count(&map->map_refcnt) != 0 &&
			    VME_OBJECT(entry) != NULL) {
				vm_object_pmap_protect_options(
					VME_OBJECT(entry),
					VME_OFFSET(entry),
					remove_size,
					PMAP_NULL,
					PAGE_SIZE,
					entry->vme_start,
					VM_PROT_NONE,
					PMAP_OPTIONS_REMOVE);
			} else {
				pmap_remove(map->pmap,
				    (addr64_t)((start + entry->vme_start)
				    - offset),
				    (addr64_t)(((start + entry->vme_start)
				    - offset) + remove_size));
			}
		}
		entry = entry->vme_next;
	}
	vm_map_unlock_read(sub_map);
	return;
}

  
                             
  
                                                         
                                                         
                              
   
void
virt_memory_guard_ast(
	thread_t thread,
	mach_exception_data_type_t code,
	mach_exception_data_type_t subcode)
{
	task_t task = thread->task;
	assert(task != kernel_task);
	assert(task == current_task());
	uint32_t behavior;

	behavior = task->task_exc_guard;

	                         
	if ((behavior & TASK_EXC_GUARD_VM_DELIVER) == 0) {
		return;
	}

	                                             
	while (behavior & TASK_EXC_GUARD_VM_ONCE) {
		uint32_t new_behavior = behavior & ~TASK_EXC_GUARD_VM_DELIVER;

		if (OSCompareAndSwap(behavior, new_behavior, &task->task_exc_guard)) {
			break;
		}
		behavior = task->task_exc_guard;
		if ((behavior & TASK_EXC_GUARD_VM_DELIVER) == 0) {
			return;
		}
	}

	                                                      
	if ((task->task_exc_guard & TASK_EXC_GUARD_VM_CORPSE) &&
	    (task->task_exc_guard & TASK_EXC_GUARD_VM_FATAL) == 0) {
		task_violated_guard(code, subcode, NULL);
	} else {
		task_exception_notify(EXC_GUARD, code, subcode);
	}

	                                   
	if (task->task_exc_guard & TASK_EXC_GUARD_VM_FATAL) {
		task_bsdtask_kill(current_task());
	}
}

  
                              
  
                                                                
  
                                                              
                                                              
                                                                   
   
static void
vm_map_guard_exception(
	vm_map_offset_t gap_start,
	unsigned reason)
{
	mach_exception_code_t code = 0;
	unsigned int guard_type = GUARD_TYPE_VIRT_MEMORY;
	unsigned int target = 0;                                                 
	mach_exception_data_type_t subcode = (uint64_t)gap_start;
	boolean_t fatal = FALSE;

	task_t task = current_task();

	                                             
	if (task == kernel_task) {
		return;
	}

	EXC_GUARD_ENCODE_TYPE(code, guard_type);
	EXC_GUARD_ENCODE_FLAVOR(code, reason);
	EXC_GUARD_ENCODE_TARGET(code, target);

	if (task->task_exc_guard & TASK_EXC_GUARD_VM_FATAL) {
		fatal = TRUE;
	}
	thread_guard_violation(current_thread(), code, subcode, fatal);
}

  
                                       
  
                                                           
                                                         
                                                                
                                                       
                                                       
  
                                                                
   
static kern_return_t
vm_map_delete(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	int                     flags,
	vm_map_t                zap_map)
{
	vm_map_entry_t          entry, next;
	struct   vm_map_entry   *first_entry, tmp_entry;
	vm_map_offset_t         s;
	vm_object_t             object;
	boolean_t               need_wakeup;
	unsigned int            last_timestamp = ~0;                     
	int                     interruptible;
	vm_map_offset_t         gap_start;
	__unused vm_map_offset_t save_start = start;
	__unused vm_map_offset_t save_end = end;
	const vm_map_offset_t   FIND_GAP = 1;                                 
	const vm_map_offset_t   GAPS_OK = 2;                                            

	if (map != kernel_map && !(flags & VM_MAP_REMOVE_GAPS_OK) && !map->terminated) {
		gap_start = FIND_GAP;
	} else {
		gap_start = GAPS_OK;
	}

	interruptible = (flags & VM_MAP_REMOVE_INTERRUPTIBLE) ?
	    THREAD_ABORTSAFE : THREAD_UNINT;

	  
                                                             
                                                                  
                                                           
                                                   
   
                                                           
                                                           
                                                          
             
    
	flags |= VM_MAP_REMOVE_WAIT_FOR_KWIRE;

	while (1) {
		  
                                              
     
		if (vm_map_lookup_entry(map, start, &first_entry)) {
			entry = first_entry;
			if (map == kalloc_map &&
			    (entry->vme_start != start ||
			    entry->vme_end != end)) {
				panic("vm_map_delete(%p,0x%llx,0x%llx): "
				    "mismatched entry %p [0x%llx:0x%llx]\n",
				    map,
				    (uint64_t)start,
				    (uint64_t)end,
				    entry,
				    (uint64_t)entry->vme_start,
				    (uint64_t)entry->vme_end);
			}

			  
                                                                              
      
			if (entry->superpage_size && (start & ~SUPERPAGE_MASK)) {
				start = SUPERPAGE_ROUND_DOWN(start);
				continue;
			}

			if (start == entry->vme_start) {
				  
                                               
                                                
       
			} else {
				if ((flags & VM_MAP_REMOVE_NO_MAP_ALIGN) &&
				    entry->map_aligned &&
				    !VM_MAP_PAGE_ALIGNED(
					    start,
					    VM_MAP_PAGE_MASK(map))) {
					  
                                   
                                  
                                    
        
					entry->map_aligned = FALSE;
				}
				if (map == kalloc_map) {
					panic("vm_map_delete(%p,0x%llx,0x%llx):"
					    " clipping %p at 0x%llx\n",
					    map,
					    (uint64_t)start,
					    (uint64_t)end,
					    entry,
					    (uint64_t)start);
				}
				vm_map_clip_start(map, entry, start);
			}

			  
                                               
                            
      
			SAVE_HINT_MAP_WRITE(map, entry->vme_prev);
		} else {
			if (map->pmap == kernel_pmap &&
			    os_ref_get_count(&map->map_refcnt) != 0) {
				panic("vm_map_delete(%p,0x%llx,0x%llx): "
				    "no map entry at 0x%llx\n",
				    map,
				    (uint64_t)start,
				    (uint64_t)end,
				    (uint64_t)start);
			}
			entry = first_entry->vme_next;
			if (gap_start == FIND_GAP) {
				gap_start = start;
			}
		}
		break;
	}
	if (entry->superpage_size) {
		end = SUPERPAGE_ROUND_UP(end);
	}

	need_wakeup = FALSE;
	  
                                           
    
	s = entry->vme_start;
	while ((entry != vm_map_to_entry(map)) && (s < end)) {
		  
                                                          
                                                      
                                              
                                                         
                                                            
                                                          
                                                            
                                                             
                                              
     

		if (entry->vme_start >= s) {
			  
                                       
                                   
      
		} else {
			  
                                           
                                          
                        
      
			if ((flags & VM_MAP_REMOVE_NO_MAP_ALIGN) &&
			    entry->map_aligned &&
			    !VM_MAP_PAGE_ALIGNED(s,
			    VM_MAP_PAGE_MASK(map))) {
				  
                                              
                                                  
       
				entry->map_aligned = FALSE;
			}
			if (map == kalloc_map) {
				panic("vm_map_delete(%p,0x%llx,0x%llx): "
				    "clipping %p at 0x%llx\n",
				    map,
				    (uint64_t)start,
				    (uint64_t)end,
				    entry,
				    (uint64_t)s);
			}
			vm_map_clip_start(map, entry, s);
		}
		if (entry->vme_end <= end) {
			  
                                                     
                                                          
      
		} else {
			if ((flags & VM_MAP_REMOVE_NO_MAP_ALIGN) &&
			    entry->map_aligned &&
			    !VM_MAP_PAGE_ALIGNED(end,
			    VM_MAP_PAGE_MASK(map))) {
				  
                                              
                                                  
       
				entry->map_aligned = FALSE;
			}
			if (map == kalloc_map) {
				panic("vm_map_delete(%p,0x%llx,0x%llx): "
				    "clipping %p at 0x%llx\n",
				    map,
				    (uint64_t)start,
				    (uint64_t)end,
				    entry,
				    (uint64_t)end);
			}
			vm_map_clip_end(map, entry, end);
		}

		if (entry->permanent) {
			if (map->pmap == kernel_pmap) {
				panic("%s(%p,0x%llx,0x%llx): "
				    "attempt to remove permanent "
				    "VM map entry "
				    "%p [0x%llx:0x%llx]\n",
				    __FUNCTION__,
				    map,
				    (uint64_t) start,
				    (uint64_t) end,
				    entry,
				    (uint64_t) entry->vme_start,
				    (uint64_t) entry->vme_end);
			} else if (flags & VM_MAP_REMOVE_IMMUTABLE) {
                                                                                                                                                                                                                                                                                                
				entry->permanent = FALSE;
			} else {
				if (vm_map_executable_immutable_verbose) {
					printf("%d[%s] %s(0x%llx,0x%llx): "
					    "permanent entry [0x%llx:0x%llx] "
					    "prot 0x%x/0x%x\n",
					    proc_selfpid(),
					    (current_task()->bsd_info
					    ? proc_name_address(current_task()->bsd_info)
					    : "?"),
					    __FUNCTION__,
					    (uint64_t) start,
					    (uint64_t) end,
					    (uint64_t)entry->vme_start,
					    (uint64_t)entry->vme_end,
					    entry->protection,
					    entry->max_protection);
				}
				  
                                                                                                                                            
       
				DTRACE_VM5(vm_map_delete_permanent,
				    vm_map_offset_t, entry->vme_start,
				    vm_map_offset_t, entry->vme_end,
				    vm_prot_t, entry->protection,
				    vm_prot_t, entry->max_protection,
				    int, VME_ALIAS(entry));
			}
		}


		if (entry->in_transition) {
			wait_result_t wait_result;

			  
                                                   
                                               
      
			assert(s == entry->vme_start);
			entry->needs_wakeup = TRUE;

			  
                                                     
                              
      
			if (need_wakeup) {
				vm_map_entry_wakeup(map);
				need_wakeup = FALSE;
			}

			wait_result = vm_map_entry_wait(map, interruptible);

			if (interruptible &&
			    wait_result == THREAD_INTERRUPTED) {
				  
                                             
                                                    
       
				return KERN_ABORTED;
			}

			  
                                             
                                               
      
			if (!vm_map_lookup_entry(map, s, &first_entry)) {
				  
                               
       
				if (gap_start == FIND_GAP) {
					gap_start = s;
				}
				entry = first_entry->vme_next;
				s = entry->vme_start;
			} else {
				entry = first_entry;
				SAVE_HINT_MAP_WRITE(map, entry->vme_prev);
			}
			last_timestamp = map->timestamp;
			continue;
		}                        

		if (entry->wired_count) {
			boolean_t       user_wire;

			user_wire = entry->user_wired_count > 0;

			  
                                              
      
			if (flags & VM_MAP_REMOVE_KUNWIRE) {
				entry->wired_count--;
			}

			  
                                                   
      
			if (entry->user_wired_count > 0) {
				while (entry->user_wired_count) {
					subtract_wire_counts(map, entry, user_wire);
				}
			}

			if (entry->wired_count != 0) {
				assert(map != kernel_map);
				  
                                             
                                               
                                         
                                            
             
       
				if (flags & VM_MAP_REMOVE_WAIT_FOR_KWIRE) {
					wait_result_t wait_result;

					assert(s == entry->vme_start);
					entry->needs_wakeup = TRUE;
					wait_result = vm_map_entry_wait(map,
					    interruptible);

					if (interruptible &&
					    wait_result == THREAD_INTERRUPTED) {
						  
                            
                                    
                                   
                  
         
						return KERN_ABORTED;
					}

					  
                                            
                                          
                 
        
					if (!vm_map_lookup_entry(map, s,
					    &first_entry)) {
						assert(map != kernel_map);
						  
                                 
         
						if (gap_start == FIND_GAP) {
							gap_start = s;
						}
						entry = first_entry->vme_next;
						s = entry->vme_start;
					} else {
						entry = first_entry;
						SAVE_HINT_MAP_WRITE(map, entry->vme_prev);
					}
					last_timestamp = map->timestamp;
					continue;
				} else {
					return KERN_FAILURE;
				}
			}

			entry->in_transition = TRUE;
			  
                                                       
      
			tmp_entry = *entry;
			assert(s == entry->vme_start);

			  
                                                  
                                              
      
			vm_map_unlock(map);

			if (tmp_entry.is_sub_map) {
				vm_map_t sub_map;
				vm_map_offset_t sub_start, sub_end;
				pmap_t pmap;
				vm_map_offset_t pmap_addr;


				sub_map = VME_SUBMAP(&tmp_entry);
				sub_start = VME_OFFSET(&tmp_entry);
				sub_end = sub_start + (tmp_entry.vme_end -
				    tmp_entry.vme_start);
				if (tmp_entry.use_pmap) {
					pmap = sub_map->pmap;
					pmap_addr = tmp_entry.vme_start;
				} else {
					pmap = map->pmap;
					pmap_addr = tmp_entry.vme_start;
				}
				(void) vm_map_unwire_nested(sub_map,
				    sub_start, sub_end,
				    user_wire,
				    pmap, pmap_addr);
			} else {
				if (VME_OBJECT(&tmp_entry) == kernel_object) {
					pmap_protect_options(
						map->pmap,
						tmp_entry.vme_start,
						tmp_entry.vme_end,
						VM_PROT_NONE,
						PMAP_OPTIONS_REMOVE,
						NULL);
				}
				vm_fault_unwire(map, &tmp_entry,
				    VME_OBJECT(&tmp_entry) == kernel_object,
				    map->pmap, tmp_entry.vme_start);
			}

			vm_map_lock(map);

			if (last_timestamp + 1 != map->timestamp) {
				  
                                           
                                              
       
				if (!vm_map_lookup_entry(map, s, &first_entry)) {
					assert((map != kernel_map) &&
					    (!entry->is_sub_map));
					if (gap_start == FIND_GAP) {
						gap_start = s;
					}
					first_entry = first_entry->vme_next;
					s = first_entry->vme_start;
				} else {
					SAVE_HINT_MAP_WRITE(map, entry->vme_prev);
				}
			} else {
				SAVE_HINT_MAP_WRITE(map, entry->vme_prev);
				first_entry = entry;
			}

			last_timestamp = map->timestamp;

			entry = first_entry;
			while ((entry != vm_map_to_entry(map)) &&
			    (entry->vme_start < tmp_entry.vme_end)) {
				assert(entry->in_transition);
				entry->in_transition = FALSE;
				if (entry->needs_wakeup) {
					entry->needs_wakeup = FALSE;
					need_wakeup = TRUE;
				}
				entry = entry->vme_next;
			}
			  
                                                
                  
      
			entry = first_entry;
			continue;
		}

		                      
		assert(entry->wired_count == 0);
		assert(entry->user_wired_count == 0);

		assert(s == entry->vme_start);

		if (flags & VM_MAP_REMOVE_NO_PMAP_CLEANUP) {
			  
                                                     
                                                       
                                                      
                                                       
                                                        
                                                       
                                                         
               
      
		} else if (entry->is_sub_map) {
			assertf(VM_MAP_PAGE_SHIFT(VME_SUBMAP(entry)) >= VM_MAP_PAGE_SHIFT(map),
			    "map %p (%d) entry %p submap %p (%d)\n",
			    map, VM_MAP_PAGE_SHIFT(map), entry,
			    VME_SUBMAP(entry),
			    VM_MAP_PAGE_SHIFT(VME_SUBMAP(entry)));
			if (entry->use_pmap) {
				assertf(VM_MAP_PAGE_SHIFT(VME_SUBMAP(entry)) == VM_MAP_PAGE_SHIFT(map),
				    "map %p (%d) entry %p submap %p (%d)\n",
				    map, VM_MAP_PAGE_SHIFT(map), entry,
				    VME_SUBMAP(entry),
				    VM_MAP_PAGE_SHIFT(VME_SUBMAP(entry)));
#ifndef NO_NESTED_PMAP
				int pmap_flags;

				if (flags & VM_MAP_REMOVE_NO_UNNESTING) {
					  
                                        
                                       
                                        
                                          
                                            
                                      
       
                                      
                            
        
					pmap_flags = PMAP_UNNEST_CLEAN;
				} else {
					  
                                          
                                           
                    
        
					pmap_flags = 0;
				}
				pmap_unnest_options(
					map->pmap,
					(addr64_t)entry->vme_start,
					entry->vme_end - entry->vme_start,
					pmap_flags);
#endif                      
				if (map->mapped_in_other_pmaps &&
				    os_ref_get_count(&map->map_refcnt) != 0) {
					                              
					vm_map_submap_pmap_clean(
						map, entry->vme_start,
						entry->vme_end,
						VME_SUBMAP(entry),
						VME_OFFSET(entry));
				}
			} else {
				vm_map_submap_pmap_clean(
					map, entry->vme_start, entry->vme_end,
					VME_SUBMAP(entry),
					VME_OFFSET(entry));
			}
		} else if (VME_OBJECT(entry) != kernel_object &&
		    VME_OBJECT(entry) != compressor_object) {
			object = VME_OBJECT(entry);
			if (map->mapped_in_other_pmaps &&
			    os_ref_get_count(&map->map_refcnt) != 0) {
				vm_object_pmap_protect_options(
					object, VME_OFFSET(entry),
					entry->vme_end - entry->vme_start,
					PMAP_NULL,
					PAGE_SIZE,
					entry->vme_start,
					VM_PROT_NONE,
					PMAP_OPTIONS_REMOVE);
			} else if ((VME_OBJECT(entry) != VM_OBJECT_NULL) ||
			    (map->pmap == kernel_pmap)) {
				                                 
                                       
                                  
                                          
                                           
                                           
                                     
                                    
                                    
                    
       
				pmap_remove_options(map->pmap,
				    (addr64_t)entry->vme_start,
				    (addr64_t)entry->vme_end,
				    PMAP_OPTIONS_REMOVE);
			}
		}

		if (entry->iokit_acct) {
			                          
			DTRACE_VM4(vm_map_iokit_unmapped_region,
			    vm_map_t, map,
			    vm_map_offset_t, entry->vme_start,
			    vm_map_offset_t, entry->vme_end,
			    int, VME_ALIAS(entry));
			vm_map_iokit_unmapped_region(map,
			    (entry->vme_end -
			    entry->vme_start));
			entry->iokit_acct = FALSE;
			entry->use_pmap = FALSE;
		}

		  
                                                        
                    
     
#if DEBUG
		assert(vm_map_pmap_is_empty(map,
		    entry->vme_start,
		    entry->vme_end));
#endif            

		next = entry->vme_next;

		if (map->pmap == kernel_pmap &&
		    os_ref_get_count(&map->map_refcnt) != 0 &&
		    entry->vme_end < end &&
		    (next == vm_map_to_entry(map) ||
		    next->vme_start != entry->vme_end)) {
			panic("vm_map_delete(%p,0x%llx,0x%llx): "
			    "hole after %p at 0x%llx\n",
			    map,
			    (uint64_t)start,
			    (uint64_t)end,
			    entry,
			    (uint64_t)entry->vme_end);
		}

		  
                                                                         
                                                                       
                       
    
                                                                                                
                                                                                   
     
		if (gap_start == FIND_GAP &&
		    vm_map_round_page(entry->vme_end, VM_MAP_PAGE_MASK(map)) < end &&
		    (next == vm_map_to_entry(map) || entry->vme_end != next->vme_start)) {
			gap_start = entry->vme_end;
		}
		s = next->vme_start;
		last_timestamp = map->timestamp;

		if (entry->permanent) {
			  
                                                       
                                                 
      
			entry->protection = VM_PROT_NONE;
			entry->max_protection = VM_PROT_NONE;
		} else if ((flags & VM_MAP_REMOVE_SAVE_ENTRIES) &&
		    zap_map != VM_MAP_NULL) {
			vm_map_size_t entry_size;
			  
                                                          
                                                       
                    
      
			                                     
			vm_map_store_entry_unlink(map, entry);
			                                                
			vm_map_store_entry_link(zap_map,
			    vm_map_last_entry(zap_map),
			    entry,
			    VM_MAP_KERNEL_FLAGS_NONE);
			entry_size = entry->vme_end - entry->vme_start;
			map->size -= entry_size;
			zap_map->size += entry_size;
			                                                        
			last_timestamp--;
		} else {
			vm_map_entry_delete(map, entry);
			                                         
			vm_map_lock(map);
		}

		entry = next;

		if (entry == vm_map_to_entry(map)) {
			break;
		}
		if (last_timestamp + 1 != map->timestamp) {
			  
                                                
                                                      
                                                     
                                                     
                                                    
                                 
      
			if (!vm_map_lookup_entry(map, s, &entry)) {
				entry = entry->vme_next;

				  
                                                                            
       
				if (gap_start == FIND_GAP && s < end) {
					gap_start = s;
				}
				s = entry->vme_start;
			} else {
				SAVE_HINT_MAP_WRITE(map, entry->vme_prev);
			}
			  
                                                    
                                                        
      
			if (entry == vm_map_to_entry(map)) {
				break;
			}
		}
		last_timestamp = map->timestamp;
	}

	if (map->wait_for_space) {
		thread_wakeup((event_t) map);
	}
	  
                                                                    
    
	if (need_wakeup) {
		vm_map_entry_wakeup(map);
	}

	if (gap_start != FIND_GAP && gap_start != GAPS_OK) {
		DTRACE_VM3(kern_vm_deallocate_gap,
		    vm_map_offset_t, gap_start,
		    vm_map_offset_t, save_start,
		    vm_map_offset_t, save_end);
		if (!(flags & VM_MAP_REMOVE_GAPS_OK)) {
			vm_map_guard_exception(gap_start, kGUARD_EXC_DEALLOC_GAP);
		}
	}

	return KERN_SUCCESS;
}


  
                    
  
                          
   
kern_return_t
vm_map_terminate(
	vm_map_t        map)
{
	vm_map_lock(map);
	map->terminated = TRUE;
	vm_map_unlock(map);

	return vm_map_remove(map,
	           map->min_offset,
	           map->max_offset,
	             
                             
                             
                                          
                                    
               
	           (VM_MAP_REMOVE_NO_UNNESTING |
	           VM_MAP_REMOVE_IMMUTABLE |
	           VM_MAP_REMOVE_GAPS_OK));
}

  
                 
  
                                                      
                                              
   
kern_return_t
vm_map_remove(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end,
	boolean_t      flags)
{
	kern_return_t   result;

	vm_map_lock(map);
	VM_MAP_RANGE_CHECK(map, start, end);
	  
                                                                            
                                                                        
                                                                          
                                                                        
          
    
	if ((start == end) && zone_maps_owned(start, 1)) {
		panic("Nothing being freed to a zone map. start = end = %p\n", (void *)start);
	}
	result = vm_map_delete(map, start, end, flags, VM_MAP_NULL);
	vm_map_unlock(map);

	return result;
}

  
                        
  
                                                             
                                              
   
kern_return_t
vm_map_remove_locked(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end,
	boolean_t       flags)
{
	kern_return_t   result;

	VM_MAP_RANGE_CHECK(map, start, end);
	result = vm_map_delete(map, start, end, flags, VM_MAP_NULL);
	return result;
}


  
                                
  
               
                                                
   
static vm_map_copy_t
vm_map_copy_allocate(void)
{
	vm_map_copy_t new_copy;

	new_copy = zalloc(vm_map_copy_zone);
	bzero(new_copy, sizeof(*new_copy));
	new_copy->c_u.hdr.rb_head_store.rbh_root = (void*)(int)SKIP_RB_TREE;
	vm_map_copy_first_entry(new_copy) = vm_map_copy_to_entry(new_copy);
	vm_map_copy_last_entry(new_copy) = vm_map_copy_to_entry(new_copy);
	return new_copy;
}

  
                               
  
               
                                             
                   
   
void
vm_map_copy_discard(
	vm_map_copy_t   copy)
{
	if (copy == VM_MAP_COPY_NULL) {
		return;
	}

	switch (copy->type) {
	case VM_MAP_COPY_ENTRY_LIST:
		while (vm_map_copy_first_entry(copy) !=
		    vm_map_copy_to_entry(copy)) {
			vm_map_entry_t  entry = vm_map_copy_first_entry(copy);

			vm_map_copy_entry_unlink(copy, entry);
			if (entry->is_sub_map) {
				vm_map_deallocate(VME_SUBMAP(entry));
			} else {
				vm_object_deallocate(VME_OBJECT(entry));
			}
			vm_map_copy_entry_dispose(copy, entry);
		}
		break;
	case VM_MAP_COPY_OBJECT:
		vm_object_deallocate(copy->cpy_object);
		break;
	case VM_MAP_COPY_KERNEL_BUFFER:

		  
                                                        
                                                          
                                                     
     
		if (copy->size > msg_ool_size_small || copy->offset) {
			panic("Invalid vm_map_copy_t sz:%lld, ofst:%lld",
			    (long long)copy->size, (long long)copy->offset);
		}
		kheap_free(KHEAP_DATA_BUFFERS, copy->cpy_kdata, copy->size);
	}
	zfree(vm_map_copy_zone, copy);
}

  
                            
  
               
                                                 
                                               
           
  
                                              
                                                 
                                               
                                                 
                                                
                                                
                                                
                                
   
vm_map_copy_t
vm_map_copy_copy(
	vm_map_copy_t   copy)
{
	vm_map_copy_t   new_copy;

	if (copy == VM_MAP_COPY_NULL) {
		return VM_MAP_COPY_NULL;
	}

	  
                                                        
                             
    

	new_copy = (vm_map_copy_t) zalloc(vm_map_copy_zone);
	memcpy((void *) new_copy, (void *) copy, sizeof(struct vm_map_copy));
#if __has_feature(ptrauth_calls)
	if (copy->type == VM_MAP_COPY_KERNEL_BUFFER) {
		new_copy->cpy_kdata = copy->cpy_kdata;
	}
#endif

	if (copy->type == VM_MAP_COPY_ENTRY_LIST) {
		  
                                         
                                             
     
		vm_map_copy_first_entry(copy)->vme_prev
		        = vm_map_copy_to_entry(new_copy);
		vm_map_copy_last_entry(copy)->vme_next
		        = vm_map_copy_to_entry(new_copy);
	}

	  
                                                     
                              
    
	copy->type = VM_MAP_COPY_OBJECT;
	copy->cpy_object = VM_OBJECT_NULL;

	  
                          
    
	return new_copy;
}

static boolean_t
vm_map_entry_is_overwritable(
	vm_map_t        dst_map __unused,
	vm_map_entry_t  entry)
{
	if (!(entry->protection & VM_PROT_WRITE)) {
		                                     
		return FALSE;
	}
#if !__x86_64__
	if (entry->used_for_jit &&
	    vm_map_cs_enforcement(dst_map) &&
	    !dst_map->cs_debugged) {
		  
                                                   
                         
     
		return FALSE;
	}
#endif                  
	return TRUE;
}

static kern_return_t
vm_map_overwrite_submap_recurse(
	vm_map_t        dst_map,
	vm_map_offset_t dst_addr,
	vm_map_size_t   dst_size)
{
	vm_map_offset_t dst_end;
	vm_map_entry_t  tmp_entry;
	vm_map_entry_t  entry;
	kern_return_t   result;
	boolean_t       encountered_sub_map = FALSE;



	  
                                                
                                                
                                                   
                                      
    

	dst_end = vm_map_round_page(dst_addr + dst_size,
	    VM_MAP_PAGE_MASK(dst_map));
	vm_map_lock(dst_map);

start_pass_1:
	if (!vm_map_lookup_entry(dst_map, dst_addr, &tmp_entry)) {
		vm_map_unlock(dst_map);
		return KERN_INVALID_ADDRESS;
	}

	vm_map_clip_start(dst_map,
	    tmp_entry,
	    vm_map_trunc_page(dst_addr,
	    VM_MAP_PAGE_MASK(dst_map)));
	if (tmp_entry->is_sub_map) {
		                                   
		assert(!tmp_entry->use_pmap);
	}

	for (entry = tmp_entry;;) {
		vm_map_entry_t  next;

		next = entry->vme_next;
		while (entry->is_sub_map) {
			vm_map_offset_t sub_start;
			vm_map_offset_t sub_end;
			vm_map_offset_t local_end;

			if (entry->in_transition) {
				  
                                                   
       
				entry->needs_wakeup = TRUE;
				vm_map_entry_wait(dst_map, THREAD_UNINT);

				goto start_pass_1;
			}

			encountered_sub_map = TRUE;
			sub_start = VME_OFFSET(entry);

			if (entry->vme_end < dst_end) {
				sub_end = entry->vme_end;
			} else {
				sub_end = dst_end;
			}
			sub_end -= entry->vme_start;
			sub_end += VME_OFFSET(entry);
			local_end = entry->vme_end;
			vm_map_unlock(dst_map);

			result = vm_map_overwrite_submap_recurse(
				VME_SUBMAP(entry),
				sub_start,
				sub_end - sub_start);

			if (result != KERN_SUCCESS) {
				return result;
			}
			if (dst_end <= entry->vme_end) {
				return KERN_SUCCESS;
			}
			vm_map_lock(dst_map);
			if (!vm_map_lookup_entry(dst_map, local_end,
			    &tmp_entry)) {
				vm_map_unlock(dst_map);
				return KERN_INVALID_ADDRESS;
			}
			entry = tmp_entry;
			next = entry->vme_next;
		}

		if (!(entry->protection & VM_PROT_WRITE)) {
			vm_map_unlock(dst_map);
			return KERN_PROTECTION_FAILURE;
		}

		if (!vm_map_entry_is_overwritable(dst_map, entry)) {
			vm_map_unlock(dst_map);
			return KERN_PROTECTION_FAILURE;
		}

		  
                                                
                                                      
                                           
     
		if (entry->in_transition) {
			  
                                                  
      
			entry->needs_wakeup = TRUE;
			vm_map_entry_wait(dst_map, THREAD_UNINT);

			goto start_pass_1;
		}

  
                                                           
   
		if (dst_end <= entry->vme_end) {
			vm_map_unlock(dst_map);
			return KERN_SUCCESS;
		}
  
                                                   
   
		if ((next == vm_map_to_entry(dst_map)) ||
		    (next->vme_start != entry->vme_end)) {
			vm_map_unlock(dst_map);
			return KERN_INVALID_ADDRESS;
		}

		  
                                                    
     
		if ((VME_OBJECT(entry) != VM_OBJECT_NULL) &&
		    ((!VME_OBJECT(entry)->internal) ||
		    (VME_OBJECT(entry)->true_share))) {
			if (encountered_sub_map) {
				vm_map_unlock(dst_map);
				return KERN_FAILURE;
			}
		}


		entry = next;
	}         
	vm_map_unlock(dst_map);
	return KERN_SUCCESS;
}

  
                                 
  
               
                                             
                                                 
                                                         
                                      
  
                                                
                                                 
                                                       
                                   
  
                                                      
                                     
  
                                            
                                                
  
                        
                                                        
                                                        
                                                     
                                                    
                                                      
                              
  
                                                           
                                                        
                                                           
  
                                                           
                                                           
                                       
  
                                                         
                                                           
                                                              
                                                     
                  
  
                                                          
                                                     
                                                        
                                                        
                                                        
                                                        
             
   

static kern_return_t
vm_map_copy_overwrite_nested(
	vm_map_t                dst_map,
	vm_map_address_t        dst_addr,
	vm_map_copy_t           copy,
	boolean_t               interruptible,
	pmap_t                  pmap,
	boolean_t               discard_on_success)
{
	vm_map_offset_t         dst_end;
	vm_map_entry_t          tmp_entry;
	vm_map_entry_t          entry;
	kern_return_t           kr;
	boolean_t               aligned = TRUE;
	boolean_t               contains_permanent_objects = FALSE;
	boolean_t               encountered_sub_map = FALSE;
	vm_map_offset_t         base_addr;
	vm_map_size_t           copy_size;
	vm_map_size_t           total_size;
	int                     copy_page_shift;


	  
                               
    

	if (copy == VM_MAP_COPY_NULL) {
		return KERN_SUCCESS;
	}

	  
                                                        
                               
    
	vm_map_copy_require(copy);

	  
                                             
                           
    

	if (copy->type == VM_MAP_COPY_KERNEL_BUFFER) {
		return vm_map_copyout_kernel_buffer(
			dst_map, &dst_addr,
			copy, copy->size, TRUE, discard_on_success);
	}

	  
                                                        
                             
    

	assert(copy->type == VM_MAP_COPY_ENTRY_LIST);

	if (copy->size == 0) {
		if (discard_on_success) {
			vm_map_copy_discard(copy);
		}
		return KERN_SUCCESS;
	}

	copy_page_shift = copy->cpy_hdr.page_shift;

	  
                                                
                                                
                                                   
                                      
    

	if (!VM_MAP_PAGE_ALIGNED(copy->size,
	    VM_MAP_PAGE_MASK(dst_map)) ||
	    !VM_MAP_PAGE_ALIGNED(copy->offset,
	    VM_MAP_PAGE_MASK(dst_map)) ||
	    !VM_MAP_PAGE_ALIGNED(dst_addr,
	    VM_MAP_PAGE_MASK(dst_map)) ||
	    copy_page_shift != VM_MAP_PAGE_SHIFT(dst_map)) {
		aligned = FALSE;
		dst_end = vm_map_round_page(dst_addr + copy->size,
		    VM_MAP_PAGE_MASK(dst_map));
	} else {
		dst_end = dst_addr + copy->size;
	}

	vm_map_lock(dst_map);

	                                                        
                                                          
                               
    
	if (dst_addr >= dst_map->max_offset) {
		vm_map_unlock(dst_map);
		return KERN_INVALID_ADDRESS;
	}

start_pass_1:
	if (!vm_map_lookup_entry(dst_map, dst_addr, &tmp_entry)) {
		vm_map_unlock(dst_map);
		return KERN_INVALID_ADDRESS;
	}
	vm_map_clip_start(dst_map,
	    tmp_entry,
	    vm_map_trunc_page(dst_addr,
	    VM_MAP_PAGE_MASK(dst_map)));
	for (entry = tmp_entry;;) {
		vm_map_entry_t  next = entry->vme_next;

		while (entry->is_sub_map) {
			vm_map_offset_t sub_start;
			vm_map_offset_t sub_end;
			vm_map_offset_t local_end;

			if (entry->in_transition) {
				  
                                                   
       
				entry->needs_wakeup = TRUE;
				vm_map_entry_wait(dst_map, THREAD_UNINT);

				goto start_pass_1;
			}

			local_end = entry->vme_end;
			if (!(entry->needs_copy)) {
				                                       
				                                       
				                                       
				                                       
				encountered_sub_map = TRUE;
				sub_start = VME_OFFSET(entry);

				if (entry->vme_end < dst_end) {
					sub_end = entry->vme_end;
				} else {
					sub_end = dst_end;
				}
				sub_end -= entry->vme_start;
				sub_end += VME_OFFSET(entry);
				vm_map_unlock(dst_map);

				kr = vm_map_overwrite_submap_recurse(
					VME_SUBMAP(entry),
					sub_start,
					sub_end - sub_start);
				if (kr != KERN_SUCCESS) {
					return kr;
				}
				vm_map_lock(dst_map);
			}

			if (dst_end <= entry->vme_end) {
				goto start_overwrite;
			}
			if (!vm_map_lookup_entry(dst_map, local_end,
			    &entry)) {
				vm_map_unlock(dst_map);
				return KERN_INVALID_ADDRESS;
			}
			next = entry->vme_next;
		}

		if (!(entry->protection & VM_PROT_WRITE)) {
			vm_map_unlock(dst_map);
			return KERN_PROTECTION_FAILURE;
		}

		if (!vm_map_entry_is_overwritable(dst_map, entry)) {
			vm_map_unlock(dst_map);
			return KERN_PROTECTION_FAILURE;
		}

		  
                                                
                                                      
                                           
     
		if (entry->in_transition) {
			  
                                                  
      
			entry->needs_wakeup = TRUE;
			vm_map_entry_wait(dst_map, THREAD_UNINT);

			goto start_pass_1;
		}

  
                                                           
   
		if (dst_end <= entry->vme_end) {
			break;
		}
  
                                                   
   
		if ((next == vm_map_to_entry(dst_map)) ||
		    (next->vme_start != entry->vme_end)) {
			vm_map_unlock(dst_map);
			return KERN_INVALID_ADDRESS;
		}


		  
                                                    
     
		if ((VME_OBJECT(entry) != VM_OBJECT_NULL) &&
		    ((!VME_OBJECT(entry)->internal) ||
		    (VME_OBJECT(entry)->true_share))) {
			contains_permanent_objects = TRUE;
		}

		entry = next;
	}         

start_overwrite:
	  
                                                           
                                   
    

	if (interruptible && contains_permanent_objects) {
		vm_map_unlock(dst_map);
		return KERN_FAILURE;            
	}

	  
   
                                            
                                            
                                                   
                                                          
                                                   
               
    

	total_size = copy->size;
	if (encountered_sub_map) {
		copy_size = 0;
		                                                    
		              
		if (!vm_map_lookup_entry( dst_map, dst_addr, &tmp_entry)) {
			vm_map_unlock(dst_map);
			return KERN_INVALID_ADDRESS;
		}
	} else {
		copy_size = copy->size;
	}

	base_addr = dst_addr;
	while (TRUE) {
		                                                 
		                                         
		vm_map_entry_t  copy_entry;
		vm_map_entry_t  previous_prev = VM_MAP_ENTRY_NULL;
		vm_map_entry_t  next_copy = VM_MAP_ENTRY_NULL;
		int             nentries;
		int             remaining_entries = 0;
		vm_map_offset_t new_offset = 0;

		for (entry = tmp_entry; copy_size == 0;) {
			vm_map_entry_t  next;

			next = entry->vme_next;

			                                                
			                                                  
			                                                    
			                                           
			                                                       
			                                                     
			                                                    
			                                         
			if (entry->in_transition) {
				  
                                                   
       
				entry->needs_wakeup = TRUE;
				vm_map_entry_wait(dst_map, THREAD_UNINT);

				if (!vm_map_lookup_entry(dst_map, base_addr,
				    &tmp_entry)) {
					vm_map_unlock(dst_map);
					return KERN_INVALID_ADDRESS;
				}
				copy_size = 0;
				entry = tmp_entry;
				continue;
			}
			if (entry->is_sub_map) {
				vm_map_offset_t sub_start;
				vm_map_offset_t sub_end;
				vm_map_offset_t local_end;

				if (entry->needs_copy) {
					                             
					                                
					                     
					if (entry->vme_end < dst_end) {
						sub_end = entry->vme_end;
					} else {
						sub_end = dst_end;
					}
					if (entry->vme_start < base_addr) {
						sub_start = base_addr;
					} else {
						sub_start = entry->vme_start;
					}
					vm_map_clip_end(
						dst_map, entry, sub_end);
					vm_map_clip_start(
						dst_map, entry, sub_start);
					assert(!entry->use_pmap);
					assert(!entry->iokit_acct);
					entry->use_pmap = TRUE;
					entry->is_sub_map = FALSE;
					vm_map_deallocate(
						VME_SUBMAP(entry));
					VME_OBJECT_SET(entry, VM_OBJECT_NULL);
					VME_OFFSET_SET(entry, 0);
					entry->is_shared = FALSE;
					entry->needs_copy = FALSE;
					entry->protection = VM_PROT_DEFAULT;
					entry->max_protection = VM_PROT_ALL;
					entry->wired_count = 0;
					entry->user_wired_count = 0;
					if (entry->inheritance
					    == VM_INHERIT_SHARE) {
						entry->inheritance = VM_INHERIT_COPY;
					}
					continue;
				}
				                                        
				                     
				if (base_addr < entry->vme_start) {
					                   
					copy_size =
					    entry->vme_start - base_addr;
					break;
				}
				sub_start = VME_OFFSET(entry);

				if (entry->vme_end < dst_end) {
					sub_end = entry->vme_end;
				} else {
					sub_end = dst_end;
				}
				sub_end -= entry->vme_start;
				sub_end += VME_OFFSET(entry);
				local_end = entry->vme_end;
				vm_map_unlock(dst_map);
				copy_size = sub_end - sub_start;

				                            
				if (total_size > copy_size) {
					vm_map_size_t   local_size = 0;
					vm_map_size_t   entry_size;

					nentries = 1;
					new_offset = copy->offset;
					copy_entry = vm_map_copy_first_entry(copy);
					while (copy_entry !=
					    vm_map_copy_to_entry(copy)) {
						entry_size = copy_entry->vme_end -
						    copy_entry->vme_start;
						if ((local_size < copy_size) &&
						    ((local_size + entry_size)
						    >= copy_size)) {
							vm_map_copy_clip_end(copy,
							    copy_entry,
							    copy_entry->vme_start +
							    (copy_size - local_size));
							entry_size = copy_entry->vme_end -
							    copy_entry->vme_start;
							local_size += entry_size;
							new_offset += entry_size;
						}
						if (local_size >= copy_size) {
							next_copy = copy_entry->vme_next;
							copy_entry->vme_next =
							    vm_map_copy_to_entry(copy);
							previous_prev =
							    copy->cpy_hdr.links.prev;
							copy->cpy_hdr.links.prev = copy_entry;
							copy->size = copy_size;
							remaining_entries =
							    copy->cpy_hdr.nentries;
							remaining_entries -= nentries;
							copy->cpy_hdr.nentries = nentries;
							break;
						} else {
							local_size += entry_size;
							new_offset += entry_size;
							nentries++;
						}
						copy_entry = copy_entry->vme_next;
					}
				}

				if ((entry->use_pmap) && (pmap == NULL)) {
					kr = vm_map_copy_overwrite_nested(
						VME_SUBMAP(entry),
						sub_start,
						copy,
						interruptible,
						VME_SUBMAP(entry)->pmap,
						TRUE);
				} else if (pmap != NULL) {
					kr = vm_map_copy_overwrite_nested(
						VME_SUBMAP(entry),
						sub_start,
						copy,
						interruptible, pmap,
						TRUE);
				} else {
					kr = vm_map_copy_overwrite_nested(
						VME_SUBMAP(entry),
						sub_start,
						copy,
						interruptible,
						dst_map->pmap,
						TRUE);
				}
				if (kr != KERN_SUCCESS) {
					if (next_copy != NULL) {
						copy->cpy_hdr.nentries +=
						    remaining_entries;
						copy->cpy_hdr.links.prev->vme_next =
						    next_copy;
						copy->cpy_hdr.links.prev
						        = previous_prev;
						copy->size = total_size;
					}
					return kr;
				}
				if (dst_end <= local_end) {
					return KERN_SUCCESS;
				}
				                                             
				                                               
				copy = vm_map_copy_allocate();
				copy->type = VM_MAP_COPY_ENTRY_LIST;
				copy->offset = new_offset;
				copy->cpy_hdr.page_shift = copy_page_shift;

				  
               
                                      
                                  
       

				total_size -= copy_size;
				copy_size = 0;
				                                             
				if (next_copy != NULL) {
					copy->cpy_hdr.nentries = remaining_entries;
					copy->cpy_hdr.links.next = next_copy;
					copy->cpy_hdr.links.prev = previous_prev;
					copy->size = total_size;
					next_copy->vme_prev =
					    vm_map_copy_to_entry(copy);
					next_copy = NULL;
				}
				base_addr = local_end;
				vm_map_lock(dst_map);
				if (!vm_map_lookup_entry(dst_map,
				    local_end, &tmp_entry)) {
					vm_map_unlock(dst_map);
					return KERN_INVALID_ADDRESS;
				}
				entry = tmp_entry;
				continue;
			}
			if (dst_end <= entry->vme_end) {
				copy_size = dst_end - base_addr;
				break;
			}

			if ((next == vm_map_to_entry(dst_map)) ||
			    (next->vme_start != entry->vme_end)) {
				vm_map_unlock(dst_map);
				return KERN_INVALID_ADDRESS;
			}

			entry = next;
		}         

		next_copy = NULL;
		nentries = 1;

		                            
		if (total_size > copy_size) {
			vm_map_size_t   local_size = 0;
			vm_map_size_t   entry_size;

			new_offset = copy->offset;
			copy_entry = vm_map_copy_first_entry(copy);
			while (copy_entry != vm_map_copy_to_entry(copy)) {
				entry_size = copy_entry->vme_end -
				    copy_entry->vme_start;
				if ((local_size < copy_size) &&
				    ((local_size + entry_size)
				    >= copy_size)) {
					vm_map_copy_clip_end(copy, copy_entry,
					    copy_entry->vme_start +
					    (copy_size - local_size));
					entry_size = copy_entry->vme_end -
					    copy_entry->vme_start;
					local_size += entry_size;
					new_offset += entry_size;
				}
				if (local_size >= copy_size) {
					next_copy = copy_entry->vme_next;
					copy_entry->vme_next =
					    vm_map_copy_to_entry(copy);
					previous_prev =
					    copy->cpy_hdr.links.prev;
					copy->cpy_hdr.links.prev = copy_entry;
					copy->size = copy_size;
					remaining_entries =
					    copy->cpy_hdr.nentries;
					remaining_entries -= nentries;
					copy->cpy_hdr.nentries = nentries;
					break;
				} else {
					local_size += entry_size;
					new_offset += entry_size;
					nentries++;
				}
				copy_entry = copy_entry->vme_next;
			}
		}

		if (aligned) {
			pmap_t  local_pmap;

			if (pmap) {
				local_pmap = pmap;
			} else {
				local_pmap = dst_map->pmap;
			}

			if ((kr =  vm_map_copy_overwrite_aligned(
				    dst_map, tmp_entry, copy,
				    base_addr, local_pmap)) != KERN_SUCCESS) {
				if (next_copy != NULL) {
					copy->cpy_hdr.nentries +=
					    remaining_entries;
					copy->cpy_hdr.links.prev->vme_next =
					    next_copy;
					copy->cpy_hdr.links.prev =
					    previous_prev;
					copy->size += copy_size;
				}
				return kr;
			}
			vm_map_unlock(dst_map);
		} else {
			  
                       
     
                                                             
                                                        
                                                              
                                                         
                                                                
                                       
      
			                                    
			kr = vm_map_copy_overwrite_unaligned(
				dst_map,
				tmp_entry,
				copy,
				base_addr,
				discard_on_success);
			if (kr != KERN_SUCCESS) {
				if (next_copy != NULL) {
					copy->cpy_hdr.nentries +=
					    remaining_entries;
					copy->cpy_hdr.links.prev->vme_next =
					    next_copy;
					copy->cpy_hdr.links.prev =
					    previous_prev;
					copy->size += copy_size;
				}
				return kr;
			}
		}
		total_size -= copy_size;
		if (total_size == 0) {
			break;
		}
		base_addr += copy_size;
		copy_size = 0;
		copy->offset = new_offset;
		if (next_copy != NULL) {
			copy->cpy_hdr.nentries = remaining_entries;
			copy->cpy_hdr.links.next = next_copy;
			copy->cpy_hdr.links.prev = previous_prev;
			next_copy->vme_prev = vm_map_copy_to_entry(copy);
			copy->size = total_size;
		}
		vm_map_lock(dst_map);
		while (TRUE) {
			if (!vm_map_lookup_entry(dst_map,
			    base_addr, &tmp_entry)) {
				vm_map_unlock(dst_map);
				return KERN_INVALID_ADDRESS;
			}
			if (tmp_entry->in_transition) {
				entry->needs_wakeup = TRUE;
				vm_map_entry_wait(dst_map, THREAD_UNINT);
			} else {
				break;
			}
		}
		vm_map_clip_start(dst_map,
		    tmp_entry,
		    vm_map_trunc_page(base_addr,
		    VM_MAP_PAGE_MASK(dst_map)));

		entry = tmp_entry;
	}            

	  
                                     
    
	if (discard_on_success) {
		vm_map_copy_discard(copy);
	}

	return KERN_SUCCESS;
}                           

kern_return_t
vm_map_copy_overwrite(
	vm_map_t        dst_map,
	vm_map_offset_t dst_addr,
	vm_map_copy_t   copy,
	vm_map_size_t   copy_size,
	boolean_t       interruptible)
{
	vm_map_size_t   head_size, tail_size;
	vm_map_copy_t   head_copy, tail_copy;
	vm_map_offset_t head_addr, tail_addr;
	vm_map_entry_t  entry;
	kern_return_t   kr;
	vm_map_offset_t effective_page_mask, effective_page_size;
	int             copy_page_shift;

	head_size = 0;
	tail_size = 0;
	head_copy = NULL;
	tail_copy = NULL;
	head_addr = 0;
	tail_addr = 0;

	if (interruptible ||
	    copy == VM_MAP_COPY_NULL ||
	    copy->type != VM_MAP_COPY_ENTRY_LIST) {
		  
                                                         
                                        
     
blunt_copy:
		return vm_map_copy_overwrite_nested(dst_map,
		           dst_addr,
		           copy,
		           interruptible,
		           (pmap_t) NULL,
		           TRUE);
	}

	copy_page_shift = VM_MAP_COPY_PAGE_SHIFT(copy);
	if (copy_page_shift < PAGE_SHIFT ||
	    VM_MAP_PAGE_SHIFT(dst_map) < PAGE_SHIFT) {
		goto blunt_copy;
	}

	if (VM_MAP_PAGE_SHIFT(dst_map) < PAGE_SHIFT) {
		effective_page_mask = VM_MAP_PAGE_MASK(dst_map);
	} else {
		effective_page_mask = MAX(VM_MAP_PAGE_MASK(dst_map), PAGE_MASK);
		effective_page_mask = MAX(VM_MAP_COPY_PAGE_MASK(copy),
		    effective_page_mask);
	}
	effective_page_size = effective_page_mask + 1;

	if (copy_size < VM_MAP_COPY_OVERWRITE_OPTIMIZATION_THRESHOLD_PAGES * effective_page_size) {
		  
                                           
     
		goto blunt_copy;
	}

	if ((dst_addr & effective_page_mask) !=
	    (copy->offset & effective_page_mask)) {
		  
                                                            
     
		goto blunt_copy;
	}

	  
                                                                 
                                                             
                                          
    
	if (!vm_map_page_aligned(dst_addr, effective_page_mask)) {
		head_addr = dst_addr;
		head_size = (effective_page_size -
		    (copy->offset & effective_page_mask));
		head_size = MIN(head_size, copy_size);
	}
	if (!vm_map_page_aligned(copy->offset + copy_size,
	    effective_page_mask)) {
		  
                              
                                               
                                                    
     
		tail_size = ((copy->offset + copy_size) &
		    effective_page_mask);
		tail_size = MIN(tail_size, copy_size);
		tail_addr = dst_addr + copy_size - tail_size;
		assert(tail_addr >= head_addr + head_size);
	}
	assert(head_size + tail_size <= copy_size);

	if (head_size + tail_size == copy_size) {
		  
                                                    
     
		goto blunt_copy;
	}

	  
                                                  
                                                     
                                                   
                 
    
	vm_map_lock_read(dst_map);
	if (!vm_map_lookup_entry(dst_map, dst_addr, &entry)) {
		vm_map_unlock_read(dst_map);
		goto blunt_copy;
	}
	for (;
	    (entry != vm_map_copy_to_entry(copy) &&
	    entry->vme_start < dst_addr + copy_size);
	    entry = entry->vme_next) {
		if (entry->is_sub_map) {
			vm_map_unlock_read(dst_map);
			goto blunt_copy;
		}
	}
	vm_map_unlock_read(dst_map);

	if (head_size) {
		  
                                                            
                     
     

		  
                                       
     
		head_copy = vm_map_copy_allocate();
		head_copy->type = VM_MAP_COPY_ENTRY_LIST;
		head_copy->cpy_hdr.entries_pageable =
		    copy->cpy_hdr.entries_pageable;
		vm_map_store_init(&head_copy->cpy_hdr);
		head_copy->cpy_hdr.page_shift = copy_page_shift;

		entry = vm_map_copy_first_entry(copy);
		if (entry->vme_end < copy->offset + head_size) {
			head_size = entry->vme_end - copy->offset;
		}

		head_copy->offset = copy->offset;
		head_copy->size = head_size;
		copy->offset += head_size;
		copy->size -= head_size;
		copy_size -= head_size;
		assert(copy_size > 0);

		vm_map_copy_clip_end(copy, entry, copy->offset);
		vm_map_copy_entry_unlink(copy, entry);
		vm_map_copy_entry_link(head_copy,
		    vm_map_copy_to_entry(head_copy),
		    entry);

		  
                           
     
		kr = vm_map_copy_overwrite_nested(dst_map,
		    head_addr,
		    head_copy,
		    interruptible,
		    (pmap_t) NULL,
		    FALSE);
		if (kr != KERN_SUCCESS) {
			goto done;
		}
	}

	if (tail_size) {
		  
                                       
     
		tail_copy = vm_map_copy_allocate();
		tail_copy->type = VM_MAP_COPY_ENTRY_LIST;
		tail_copy->cpy_hdr.entries_pageable =
		    copy->cpy_hdr.entries_pageable;
		vm_map_store_init(&tail_copy->cpy_hdr);
		tail_copy->cpy_hdr.page_shift = copy_page_shift;

		tail_copy->offset = copy->offset + copy_size - tail_size;
		tail_copy->size = tail_size;

		copy->size -= tail_size;
		copy_size -= tail_size;
		assert(copy_size > 0);

		entry = vm_map_copy_last_entry(copy);
		vm_map_copy_clip_start(copy, entry, tail_copy->offset);
		entry = vm_map_copy_last_entry(copy);
		vm_map_copy_entry_unlink(copy, entry);
		vm_map_copy_entry_link(tail_copy,
		    vm_map_copy_last_entry(tail_copy),
		    entry);
	}

	  
                                                          
                                                       
                                                          
                                                 
   
                                                            
                              
   
                                                             
                                                              
                                                           
                                   
    
	assertf(copy->size == copy_size,
	    "Mismatch of copy sizes. Expected 0x%llx, Got 0x%llx\n", (uint64_t) copy_size, (uint64_t) copy->size);
	copy->size = copy_size;

	  
                                            
    
	kr = vm_map_copy_overwrite_nested(dst_map,
	    dst_addr + head_size,
	    copy,
	    interruptible,
	    (pmap_t) NULL,
	    FALSE);
	if (kr != KERN_SUCCESS) {
		goto done;
	}

	if (tail_size) {
		kr = vm_map_copy_overwrite_nested(dst_map,
		    tail_addr,
		    tail_copy,
		    interruptible,
		    (pmap_t) NULL,
		    FALSE);
	}

done:
	assert(copy->type == VM_MAP_COPY_ENTRY_LIST);
	if (kr == KERN_SUCCESS) {
		  
                               
     
		if (head_copy) {
			vm_map_copy_discard(head_copy);
			head_copy = NULL;
		}
		vm_map_copy_discard(copy);
		if (tail_copy) {
			vm_map_copy_discard(tail_copy);
			tail_copy = NULL;
		}
	} else {
		  
                                       
     
		if (head_copy) {
			entry = vm_map_copy_first_entry(head_copy);
			vm_map_copy_entry_unlink(head_copy, entry);
			vm_map_copy_entry_link(copy,
			    vm_map_copy_to_entry(copy),
			    entry);
			copy->offset -= head_size;
			copy->size += head_size;
			vm_map_copy_discard(head_copy);
			head_copy = NULL;
		}
		if (tail_copy) {
			entry = vm_map_copy_last_entry(tail_copy);
			vm_map_copy_entry_unlink(tail_copy, entry);
			vm_map_copy_entry_link(copy,
			    vm_map_copy_last_entry(copy),
			    entry);
			copy->size += tail_size;
			vm_map_copy_discard(tail_copy);
			tail_copy = NULL;
		}
	}
	return kr;
}


  
                                                               
  
              
                                 
  
                  
                                                                 
                                                                   
                                                                 
                                                                  
                                                                         
                                                                 
                                                                      
                                                                       
                                                                      
                           
  
               
                                                              
                     
   

static kern_return_t
vm_map_copy_overwrite_unaligned(
	vm_map_t        dst_map,
	vm_map_entry_t  entry,
	vm_map_copy_t   copy,
	vm_map_offset_t start,
	boolean_t       discard_on_success)
{
	vm_map_entry_t          copy_entry;
	vm_map_entry_t          copy_entry_next;
	vm_map_version_t        version;
	vm_object_t             dst_object;
	vm_object_offset_t      dst_offset;
	vm_object_offset_t      src_offset;
	vm_object_offset_t      entry_offset;
	vm_map_offset_t         entry_end;
	vm_map_size_t           src_size,
	    dst_size,
	    copy_size,
	    amount_left;
	kern_return_t           kr = KERN_SUCCESS;


	copy_entry = vm_map_copy_first_entry(copy);

	vm_map_lock_write_to_read(dst_map);

	src_offset = copy->offset - trunc_page_mask_64(copy->offset, VM_MAP_COPY_PAGE_MASK(copy));
	amount_left = copy->size;
  
                                                                    
                                   
   
	while (amount_left > 0) {
		if (entry == vm_map_to_entry(dst_map)) {
			vm_map_unlock_read(dst_map);
			return KERN_INVALID_ADDRESS;
		}

		                                                  
		assert((start >= entry->vme_start) && (start < entry->vme_end));

		dst_offset = start - entry->vme_start;

		dst_size = entry->vme_end - start;

		src_size = copy_entry->vme_end -
		    (copy_entry->vme_start + src_offset);

		if (dst_size < src_size) {
  
                                           
                                              
   
			copy_size = dst_size;
		} else {
  
                                           
                                              
   
			copy_size = src_size;
		}

		if (copy_size > amount_left) {
			copy_size = amount_left;
		}
  
                                                       
                         
   
		if (entry->needs_copy &&
		    ((entry->protection & VM_PROT_WRITE) != 0)) {
			if (vm_map_lock_read_to_write(dst_map)) {
				vm_map_lock_read(dst_map);
				goto RetryLookup;
			}
			VME_OBJECT_SHADOW(entry,
			    (vm_map_size_t)(entry->vme_end
			    - entry->vme_start));
			entry->needs_copy = FALSE;
			vm_map_lock_write_to_read(dst_map);
		}
		dst_object = VME_OBJECT(entry);
  
                                                      
                                                     
   
		if (dst_object == VM_OBJECT_NULL) {
			if (vm_map_lock_read_to_write(dst_map)) {
				vm_map_lock_read(dst_map);
				goto RetryLookup;
			}
			dst_object = vm_object_allocate((vm_map_size_t)
			    entry->vme_end - entry->vme_start);
			VME_OBJECT_SET(entry, dst_object);
			VME_OFFSET_SET(entry, 0);
			assert(entry->use_pmap);
			vm_map_lock_write_to_read(dst_map);
		}
  
                                                            
                                                 
   
		vm_object_reference(dst_object);
		version.main_timestamp = dst_map->timestamp;
		entry_offset = VME_OFFSET(entry);
		entry_end = entry->vme_end;
		vm_map_unlock_read(dst_map);
  
                                        
   
		kr = vm_fault_copy(
			VME_OBJECT(copy_entry),
			VME_OFFSET(copy_entry) + src_offset,
			&copy_size,
			dst_object,
			entry_offset + dst_offset,
			dst_map,
			&version,
			THREAD_UNINT );

		start += copy_size;
		src_offset += copy_size;
		amount_left -= copy_size;
  
                                
   
		vm_object_deallocate(dst_object);
  
                                           
   
		if (kr != KERN_SUCCESS) {
			return kr;
		}

		if ((copy_entry->vme_start + src_offset) == copy_entry->vme_end
		    || amount_left == 0) {
  
                                            
   
			copy_entry_next = copy_entry->vme_next;

			if (discard_on_success) {
				vm_map_copy_entry_unlink(copy, copy_entry);
				assert(!copy_entry->is_sub_map);
				vm_object_deallocate(VME_OBJECT(copy_entry));
				vm_map_copy_entry_dispose(copy, copy_entry);
			}

			if (copy_entry_next == vm_map_copy_to_entry(copy) &&
			    amount_left) {
  
                                                
   
				return KERN_INVALID_ADDRESS;
			}

			copy_entry = copy_entry_next;

			src_offset = 0;
		}

		if (amount_left == 0) {
			return KERN_SUCCESS;
		}

		vm_map_lock_read(dst_map);
		if (version.main_timestamp == dst_map->timestamp) {
			if (start == entry_end) {
  
                                                   
                                                 
           
   
				entry = entry->vme_next;
  
                                                
                                
   
				if (start != entry->vme_start) {
					vm_map_unlock_read(dst_map);
					return KERN_INVALID_ADDRESS;
				}
			}
		} else {
  
                              
                                              
                                                 
   
RetryLookup:
			if (!vm_map_lookup_entry(dst_map, start, &entry)) {
				vm_map_unlock_read(dst_map);
				return KERN_INVALID_ADDRESS;
			}
		}
	}           

	return KERN_SUCCESS;
}                                     

  
                                                             
  
               
                                                     
  
                  
  
                                                        
                                                        
                                               
                                                   
                                                       
                                                           
                                                          
                                                                 
                                                              
   

int vm_map_copy_overwrite_aligned_src_not_internal = 0;
int vm_map_copy_overwrite_aligned_src_not_symmetric = 0;
int vm_map_copy_overwrite_aligned_src_large = 0;

static kern_return_t
vm_map_copy_overwrite_aligned(
	vm_map_t        dst_map,
	vm_map_entry_t  tmp_entry,
	vm_map_copy_t   copy,
	vm_map_offset_t start,
	__unused pmap_t pmap)
{
	vm_object_t     object;
	vm_map_entry_t  copy_entry;
	vm_map_size_t   copy_size;
	vm_map_size_t   size;
	vm_map_entry_t  entry;

	while ((copy_entry = vm_map_copy_first_entry(copy))
	    != vm_map_copy_to_entry(copy)) {
		copy_size = (copy_entry->vme_end - copy_entry->vme_start);

		entry = tmp_entry;
		if (entry->is_sub_map) {
			                                   
			assert(!entry->use_pmap);
		}
		if (entry == vm_map_to_entry(dst_map)) {
			vm_map_unlock(dst_map);
			return KERN_INVALID_ADDRESS;
		}
		size = (entry->vme_end - entry->vme_start);
		  
                                             
                                            
                                              
             
     

		if ((entry->vme_start != start) || ((entry->is_sub_map)
		    && !entry->needs_copy)) {
			vm_map_unlock(dst_map);
			return KERN_INVALID_ADDRESS;
		}
		assert(entry != vm_map_to_entry(dst_map));

		  
                           
     

		if (!(entry->protection & VM_PROT_WRITE)) {
			vm_map_unlock(dst_map);
			return KERN_PROTECTION_FAILURE;
		}

		if (!vm_map_entry_is_overwritable(dst_map, entry)) {
			vm_map_unlock(dst_map);
			return KERN_PROTECTION_FAILURE;
		}

		  
                                
     

		if (copy_size < size) {
			if (entry->map_aligned &&
			    !VM_MAP_PAGE_ALIGNED(entry->vme_start + copy_size,
			    VM_MAP_PAGE_MASK(dst_map))) {
				                           
				entry->map_aligned = FALSE;
			}
			vm_map_clip_end(dst_map, entry, entry->vme_start + copy_size);
			size = copy_size;
		}

		  
                               
     

		if (size < copy_size) {
			vm_map_copy_clip_end(copy, copy_entry,
			    copy_entry->vme_start + size);
			copy_size = size;
		}

		assert((entry->vme_end - entry->vme_start) == size);
		assert((tmp_entry->vme_end - tmp_entry->vme_start) == size);
		assert((copy_entry->vme_end - copy_entry->vme_start) == size);

		  
                                                           
                                                    
                                
     

		object = VME_OBJECT(entry);
		if ((!entry->is_shared &&
		    ((object == VM_OBJECT_NULL) ||
		    (object->internal && !object->true_share))) ||
		    entry->needs_copy) {
			vm_object_t     old_object = VME_OBJECT(entry);
			vm_object_offset_t      old_offset = VME_OFFSET(entry);
			vm_object_offset_t      offset;

			  
                                                   
               
      
			if (old_object == VME_OBJECT(copy_entry) &&
			    old_offset == VME_OFFSET(copy_entry)) {
				vm_map_copy_entry_unlink(copy, copy_entry);
				vm_map_copy_entry_dispose(copy, copy_entry);

				if (old_object != VM_OBJECT_NULL) {
					vm_object_deallocate(old_object);
				}

				start = tmp_entry->vme_end;
				tmp_entry = tmp_entry->vme_next;
				continue;
			}

#if XNU_TARGET_OS_OSX
#define __TRADEOFF1_OBJ_SIZE (64 * 1024 * 1024)            
#define __TRADEOFF1_COPY_SIZE (128 * 1024)                  
			if (VME_OBJECT(copy_entry) != VM_OBJECT_NULL &&
			    VME_OBJECT(copy_entry)->vo_size >= __TRADEOFF1_OBJ_SIZE &&
			    copy_size <= __TRADEOFF1_COPY_SIZE) {
				  
                                             
      
                                              
                                             
                                                
                                               
                                     
       
				vm_map_copy_overwrite_aligned_src_large++;
				goto slow_copy;
			}
#endif                        

			if ((dst_map->pmap != kernel_pmap) &&
			    (VME_ALIAS(entry) >= VM_MEMORY_MALLOC) &&
			    (VME_ALIAS(entry) <= VM_MEMORY_MALLOC_MEDIUM)) {
				vm_object_t new_object, new_shadow;

				  
                                                  
                                 
       
				new_object = VME_OBJECT(copy_entry);
				if (new_object != VM_OBJECT_NULL) {
					vm_object_lock_shared(new_object);
				}
				while (new_object != VM_OBJECT_NULL &&
#if XNU_TARGET_OS_OSX
				    !new_object->true_share &&
				    new_object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC &&
#endif                        
				    new_object->internal) {
					new_shadow = new_object->shadow;
					if (new_shadow == VM_OBJECT_NULL) {
						break;
					}
					vm_object_lock_shared(new_shadow);
					vm_object_unlock(new_object);
					new_object = new_shadow;
				}
				if (new_object != VM_OBJECT_NULL) {
					if (!new_object->internal) {
						  
                                  
                                   
                                    
                                   
                                  
                                   
                
         
						vm_map_copy_overwrite_aligned_src_not_internal++;
						vm_object_unlock(new_object);
						goto slow_copy;
					}
#if XNU_TARGET_OS_OSX
					if (new_object->true_share ||
					    new_object->copy_strategy != MEMORY_OBJECT_COPY_SYMMETRIC) {
						  
                                       
                                       
                                     
                                   
         
						vm_map_copy_overwrite_aligned_src_not_symmetric++;
						vm_object_unlock(new_object);
						goto slow_copy;
					}
#endif                        
					vm_object_unlock(new_object);
				}
				  
                                         
                                           
                                           
                        
       
			}

			if (old_object != VM_OBJECT_NULL) {
				if (entry->is_sub_map) {
					if (entry->use_pmap) {
#ifndef NO_NESTED_PMAP
						pmap_unnest(dst_map->pmap,
						    (addr64_t)entry->vme_start,
						    entry->vme_end - entry->vme_start);
#endif                      
						if (dst_map->mapped_in_other_pmaps) {
							                     
							              
							vm_map_submap_pmap_clean(
								dst_map, entry->vme_start,
								entry->vme_end,
								VME_SUBMAP(entry),
								VME_OFFSET(entry));
						}
					} else {
						vm_map_submap_pmap_clean(
							dst_map, entry->vme_start,
							entry->vme_end,
							VME_SUBMAP(entry),
							VME_OFFSET(entry));
					}
					vm_map_deallocate(VME_SUBMAP(entry));
				} else {
					if (dst_map->mapped_in_other_pmaps) {
						vm_object_pmap_protect_options(
							VME_OBJECT(entry),
							VME_OFFSET(entry),
							entry->vme_end
							- entry->vme_start,
							PMAP_NULL,
							PAGE_SIZE,
							entry->vme_start,
							VM_PROT_NONE,
							PMAP_OPTIONS_REMOVE);
					} else {
						pmap_remove_options(
							dst_map->pmap,
							(addr64_t)(entry->vme_start),
							(addr64_t)(entry->vme_end),
							PMAP_OPTIONS_REMOVE);
					}
					vm_object_deallocate(old_object);
				}
			}

			if (entry->iokit_acct) {
				                                 
				entry->use_pmap = FALSE;
			} else {
				                         
				entry->use_pmap = TRUE;
			}
			entry->is_sub_map = FALSE;
			VME_OBJECT_SET(entry, VME_OBJECT(copy_entry));
			object = VME_OBJECT(entry);
			entry->needs_copy = copy_entry->needs_copy;
			entry->wired_count = 0;
			entry->user_wired_count = 0;
			offset = VME_OFFSET(copy_entry);
			VME_OFFSET_SET(entry, offset);

			vm_map_copy_entry_unlink(copy, copy_entry);
			vm_map_copy_entry_dispose(copy, copy_entry);

			  
                                                                 
                                                                  
                                                   
                                                                     
                                                                                
                                                                                
      

			  
                                             
                                        
                                          
                                             
                           
      

			start = tmp_entry->vme_end;
			tmp_entry = tmp_entry->vme_next;
		} else {
			vm_map_version_t        version;
			vm_object_t             dst_object;
			vm_object_offset_t      dst_offset;
			kern_return_t           r;

slow_copy:
			if (entry->needs_copy) {
				VME_OBJECT_SHADOW(entry,
				    (entry->vme_end -
				    entry->vme_start));
				entry->needs_copy = FALSE;
			}

			dst_object = VME_OBJECT(entry);
			dst_offset = VME_OFFSET(entry);

			  
                                          
                                             
                                 
      

			if (dst_object == VM_OBJECT_NULL) {
				  
                                           
                                              
                                                 
                                                
                                                
                                          
                                               
                                
                                              
                                                   
       
				dst_object = vm_object_allocate(
					entry->vme_end - entry->vme_start);
				dst_offset = 0;
				VME_OBJECT_SET(entry, dst_object);
				VME_OFFSET_SET(entry, dst_offset);
				assert(entry->use_pmap);
			}

			vm_object_reference(dst_object);

			                                             
			version.main_timestamp = dst_map->timestamp + 1;

			vm_map_unlock(dst_map);

			  
                                          
      

			copy_size = size;
			r = vm_fault_copy(
				VME_OBJECT(copy_entry),
				VME_OFFSET(copy_entry),
				&copy_size,
				dst_object,
				dst_offset,
				dst_map,
				&version,
				THREAD_UNINT );

			  
                                  
      

			vm_object_deallocate(dst_object);

			  
                                             
      

			if (r != KERN_SUCCESS) {
				return r;
			}

			if (copy_size != 0) {
				  
                                   
       

				vm_map_copy_clip_end(copy, copy_entry,
				    copy_entry->vme_start + copy_size);
				vm_map_copy_entry_unlink(copy, copy_entry);
				vm_object_deallocate(VME_OBJECT(copy_entry));
				vm_map_copy_entry_dispose(copy, copy_entry);
			}

			  
                                                       
     
                                                   
                         
      

			start += copy_size;
			vm_map_lock(dst_map);
			if (version.main_timestamp == dst_map->timestamp &&
			    copy_size != 0) {
				                                             

				if (tmp_entry->map_aligned &&
				    !VM_MAP_PAGE_ALIGNED(
					    start,
					    VM_MAP_PAGE_MASK(dst_map))) {
					                           
					tmp_entry->map_aligned = FALSE;
				}
				vm_map_clip_end(dst_map, tmp_entry, start);
				tmp_entry = tmp_entry->vme_next;
			} else {
				                                 

				if (!vm_map_lookup_entry(dst_map, start, &tmp_entry)) {
					vm_map_unlock(dst_map);
					return KERN_INVALID_ADDRESS;
				}
				if (tmp_entry->map_aligned &&
				    !VM_MAP_PAGE_ALIGNED(
					    start,
					    VM_MAP_PAGE_MASK(dst_map))) {
					                           
					tmp_entry->map_aligned = FALSE;
				}
				vm_map_clip_start(dst_map, tmp_entry, start);
			}
		}
	}           

	return KERN_SUCCESS;
}                                   

  
                                                           
  
               
                                                     
                                                    
                
  
                                             
   
static kern_return_t
vm_map_copyin_kernel_buffer(
	vm_map_t        src_map,
	vm_map_offset_t src_addr,
	vm_map_size_t   len,
	boolean_t       src_destroy,
	vm_map_copy_t   *copy_result)
{
	kern_return_t kr;
	vm_map_copy_t copy;

	if (len > msg_ool_size_small) {
		return KERN_INVALID_ARGUMENT;
	}

	copy = zalloc_flags(vm_map_copy_zone, Z_WAITOK | Z_ZERO);
	if (copy == VM_MAP_COPY_NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}
	copy->cpy_kdata = kheap_alloc(KHEAP_DATA_BUFFERS, len, Z_WAITOK);
	if (copy->cpy_kdata == NULL) {
		zfree(vm_map_copy_zone, copy);
		return KERN_RESOURCE_SHORTAGE;
	}

	copy->type = VM_MAP_COPY_KERNEL_BUFFER;
	copy->size = len;
	copy->offset = 0;

	kr = copyinmap(src_map, src_addr, copy->cpy_kdata, (vm_size_t)len);
	if (kr != KERN_SUCCESS) {
		kheap_free(KHEAP_DATA_BUFFERS, copy->cpy_kdata, len);
		zfree(vm_map_copy_zone, copy);
		return kr;
	}
	if (src_destroy) {
		(void) vm_map_remove(
			src_map,
			vm_map_trunc_page(src_addr,
			VM_MAP_PAGE_MASK(src_map)),
			vm_map_round_page(src_addr + len,
			VM_MAP_PAGE_MASK(src_map)),
			(VM_MAP_REMOVE_INTERRUPTIBLE |
			VM_MAP_REMOVE_WAIT_FOR_KWIRE |
			((src_map == kernel_map) ? VM_MAP_REMOVE_KUNWIRE : VM_MAP_REMOVE_NO_FLAGS)));
	}
	*copy_result = copy;
	return KERN_SUCCESS;
}

  
                                                            
  
               
                                                        
                                                            
              
  
                                            
                                                
   
static int vm_map_copyout_kernel_buffer_failures = 0;
static kern_return_t
vm_map_copyout_kernel_buffer(
	vm_map_t                map,
	vm_map_address_t        *addr,              
	vm_map_copy_t           copy,
	vm_map_size_t           copy_size,
	boolean_t               overwrite,
	boolean_t               consume_on_success)
{
	kern_return_t kr = KERN_SUCCESS;
	thread_t thread = current_thread();

	assert(copy->size == copy_size);

	  
                                             
    
	if (copy_size > msg_ool_size_small || copy->offset) {
		panic("Invalid vm_map_copy_t sz:%lld, ofst:%lld",
		    (long long)copy->size, (long long)copy->offset);
	}

	if (!overwrite) {
		  
                                                  
     
		*addr = 0;
		kr = vm_map_enter(map,
		    addr,
		    vm_map_round_page(copy_size,
		    VM_MAP_PAGE_MASK(map)),
		    (vm_map_offset_t) 0,
		    VM_FLAGS_ANYWHERE,
		    VM_MAP_KERNEL_FLAGS_NONE,
		    VM_KERN_MEMORY_NONE,
		    VM_OBJECT_NULL,
		    (vm_object_offset_t) 0,
		    FALSE,
		    VM_PROT_DEFAULT,
		    VM_PROT_ALL,
		    VM_INHERIT_DEFAULT);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
#if KASAN
		if (map->pmap == kernel_pmap) {
			kasan_notify_address(*addr, copy->size);
		}
#endif
	}

	  
                                                              
    
	if (thread->map == map) {
		  
                                                  
              
     
		assert((vm_size_t)copy_size == copy_size);
		if (copyout(copy->cpy_kdata, *addr, (vm_size_t)copy_size)) {
			kr = KERN_INVALID_ADDRESS;
		}
	} else {
		vm_map_t oldmap;

		  
                                                 
                                                     
                 
     
		vm_map_reference(map);
		oldmap = vm_map_switch(map);

		assert((vm_size_t)copy_size == copy_size);
		if (copyout(copy->cpy_kdata, *addr, (vm_size_t)copy_size)) {
			vm_map_copyout_kernel_buffer_failures++;
			kr = KERN_INVALID_ADDRESS;
		}

		(void) vm_map_switch(oldmap);
		vm_map_deallocate(map);
	}

	if (kr != KERN_SUCCESS) {
		                               
		if (!overwrite) {
			  
                                                          
      
			(void) vm_map_remove(
				map,
				vm_map_trunc_page(*addr,
				VM_MAP_PAGE_MASK(map)),
				vm_map_round_page((*addr +
				vm_map_round_page(copy_size,
				VM_MAP_PAGE_MASK(map))),
				VM_MAP_PAGE_MASK(map)),
				VM_MAP_REMOVE_NO_FLAGS);
			*addr = 0;
		}
	} else {
		                                                    
		if (consume_on_success) {
			kheap_free(KHEAP_DATA_BUFFERS, copy->cpy_kdata, copy_size);
			zfree(vm_map_copy_zone, copy);
		}
	}

	return kr;
}

  
                                                       
  
               
                                                
                                       
                
                                
   
static void
vm_map_copy_insert(
	vm_map_t        map,
	vm_map_entry_t  after_where,
	vm_map_copy_t   copy)
{
	vm_map_entry_t  entry;

	while (vm_map_copy_first_entry(copy) != vm_map_copy_to_entry(copy)) {
		entry = vm_map_copy_first_entry(copy);
		vm_map_copy_entry_unlink(copy, entry);
		vm_map_store_entry_link(map, after_where, entry,
		    VM_MAP_KERNEL_FLAGS_NONE);
		after_where = entry;
	}
	zfree(vm_map_copy_zone, copy);
}

void
vm_map_copy_remap(
	vm_map_t        map,
	vm_map_entry_t  where,
	vm_map_copy_t   copy,
	vm_map_offset_t adjustment,
	vm_prot_t       cur_prot,
	vm_prot_t       max_prot,
	vm_inherit_t    inheritance)
{
	vm_map_entry_t  copy_entry, new_entry;

	for (copy_entry = vm_map_copy_first_entry(copy);
	    copy_entry != vm_map_copy_to_entry(copy);
	    copy_entry = copy_entry->vme_next) {
		                                        
		new_entry = vm_map_entry_create(map,
		    !map->hdr.entries_pageable);
		                                            
		vm_map_entry_copy(map, new_entry, copy_entry);
		                              
		new_entry->vme_start += adjustment;
		new_entry->vme_end += adjustment;
		                           
		new_entry->inheritance = inheritance;
		new_entry->protection = cur_prot;
		new_entry->max_protection = max_prot;
		new_entry->behavior = VM_BEHAVIOR_DEFAULT;
		                                                     
		if (new_entry->is_sub_map) {
			assert(!new_entry->use_pmap);                 
			vm_map_lock(VME_SUBMAP(new_entry));
			vm_map_reference(VME_SUBMAP(new_entry));
			vm_map_unlock(VME_SUBMAP(new_entry));
		} else {
			vm_object_reference(VME_OBJECT(new_entry));
		}
		                                     
		vm_map_store_entry_link(map, where, new_entry,
		    VM_MAP_KERNEL_FLAGS_NONE);
		                                                               
		where = new_entry;
	}
}


  
                                                                    
                                                                              
                                                                  
   
boolean_t
vm_map_copy_validate_size(
	vm_map_t                dst_map,
	vm_map_copy_t           copy,
	vm_map_size_t           *size)
{
	if (copy == VM_MAP_COPY_NULL) {
		return FALSE;
	}
	vm_map_size_t copy_sz = copy->size;
	vm_map_size_t sz = *size;
	switch (copy->type) {
	case VM_MAP_COPY_OBJECT:
	case VM_MAP_COPY_KERNEL_BUFFER:
		if (sz == copy_sz) {
			return TRUE;
		}
		break;
	case VM_MAP_COPY_ENTRY_LIST:
		  
                                                          
                                                               
                                     
     
		if (copy_sz >= sz &&
		    copy_sz <= vm_map_round_page(sz, VM_MAP_PAGE_MASK(dst_map))) {
			*size = copy_sz;
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

  
                               
  
               
                                                       
                                                     
                                                         
  
                                            
                                                
   
kern_return_t
vm_map_copyout_size(
	vm_map_t                dst_map,
	vm_map_address_t        *dst_addr,               
	vm_map_copy_t           copy,
	vm_map_size_t           copy_size)
{
	return vm_map_copyout_internal(dst_map, dst_addr, copy, copy_size,
	           TRUE,                                             
	           VM_PROT_DEFAULT,
	           VM_PROT_ALL,
	           VM_INHERIT_DEFAULT);
}

  
                          
  
               
                                                       
                                 
  
                                            
                                                
   
kern_return_t
vm_map_copyout(
	vm_map_t                dst_map,
	vm_map_address_t        *dst_addr,               
	vm_map_copy_t           copy)
{
	return vm_map_copyout_internal(dst_map, dst_addr, copy, copy ? copy->size : 0,
	           TRUE,                                             
	           VM_PROT_DEFAULT,
	           VM_PROT_ALL,
	           VM_INHERIT_DEFAULT);
}

kern_return_t
vm_map_copyout_internal(
	vm_map_t                dst_map,
	vm_map_address_t        *dst_addr,               
	vm_map_copy_t           copy,
	vm_map_size_t           copy_size,
	boolean_t               consume_on_success,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	vm_map_size_t           size;
	vm_map_size_t           adjustment;
	vm_map_offset_t         start;
	vm_object_offset_t      vm_copy_start;
	vm_map_entry_t          last;
	vm_map_entry_t          entry;
	vm_map_entry_t          hole_entry;
	vm_map_copy_t           original_copy;

	  
                               
    

	if (copy == VM_MAP_COPY_NULL) {
		*dst_addr = 0;
		return KERN_SUCCESS;
	}

	  
                                                        
                               
    
	vm_map_copy_require(copy);

	if (copy->size != copy_size) {
		*dst_addr = 0;
		return KERN_FAILURE;
	}

	  
                                          
                            
    

	if (copy->type == VM_MAP_COPY_OBJECT) {
		vm_object_t             object = copy->cpy_object;
		kern_return_t           kr;
		vm_object_offset_t      offset;

		offset = vm_object_trunc_page(copy->offset);
		size = vm_map_round_page((copy_size +
		    (vm_map_size_t)(copy->offset -
		    offset)),
		    VM_MAP_PAGE_MASK(dst_map));
		*dst_addr = 0;
		kr = vm_map_enter(dst_map, dst_addr, size,
		    (vm_map_offset_t) 0, VM_FLAGS_ANYWHERE,
		    VM_MAP_KERNEL_FLAGS_NONE,
		    VM_KERN_MEMORY_NONE,
		    object, offset, FALSE,
		    VM_PROT_DEFAULT, VM_PROT_ALL,
		    VM_INHERIT_DEFAULT);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		                                             
		*dst_addr += (vm_map_offset_t)(copy->offset - offset);
		if (consume_on_success) {
			zfree(vm_map_copy_zone, copy);
		}
		return KERN_SUCCESS;
	}

	  
                                             
                           
    

	if (copy->type == VM_MAP_COPY_KERNEL_BUFFER) {
		return vm_map_copyout_kernel_buffer(dst_map, dst_addr,
		           copy, copy_size, FALSE,
		           consume_on_success);
	}

	original_copy = copy;
	if (copy->cpy_hdr.page_shift != VM_MAP_PAGE_SHIFT(dst_map)) {
		kern_return_t kr;
		vm_map_copy_t target_copy;
		vm_map_offset_t overmap_start, overmap_end, trimmed_start;

		target_copy = VM_MAP_COPY_NULL;
		DEBUG4K_ADJUST("adjusting...\n");
		kr = vm_map_copy_adjust_to_target(
			copy,
			0,             
			copy->size,           
			dst_map,
			TRUE,           
			&target_copy,
			&overmap_start,
			&overmap_end,
			&trimmed_start);
		if (kr != KERN_SUCCESS) {
			DEBUG4K_COPY("adjust failed 0x%x\n", kr);
			return kr;
		}
		DEBUG4K_COPY("copy %p (%d 0x%llx 0x%llx) dst_map %p (%d) target_copy %p (%d 0x%llx 0x%llx) overmap_start 0x%llx overmap_end 0x%llx trimmed_start 0x%llx\n", copy, copy->cpy_hdr.page_shift, copy->offset, (uint64_t)copy->size, dst_map, VM_MAP_PAGE_SHIFT(dst_map), target_copy, target_copy->cpy_hdr.page_shift, target_copy->offset, (uint64_t)target_copy->size, (uint64_t)overmap_start, (uint64_t)overmap_end, (uint64_t)trimmed_start);
		if (target_copy != copy) {
			copy = target_copy;
		}
		copy_size = copy->size;
	}

	  
                           
    

	vm_copy_start = vm_map_trunc_page((vm_map_size_t)copy->offset,
	    VM_MAP_COPY_PAGE_MASK(copy));
	size = vm_map_round_page((vm_map_size_t)copy->offset + copy_size,
	    VM_MAP_COPY_PAGE_MASK(copy))
	    - vm_copy_start;


StartAgain:;

	vm_map_lock(dst_map);
	if (dst_map->disable_vmentry_reuse == TRUE) {
		VM_MAP_HIGHEST_ENTRY(dst_map, entry, start);
		last = entry;
	} else {
		if (dst_map->holelistenabled) {
			hole_entry = CAST_TO_VM_MAP_ENTRY(dst_map->holes_list);

			if (hole_entry == NULL) {
				  
                                
       
				vm_map_unlock(dst_map);
				return KERN_NO_SPACE;
			}

			last = hole_entry;
			start = last->vme_start;
		} else {
			assert(first_free_is_valid(dst_map));
			start = ((last = dst_map->first_free) == vm_map_to_entry(dst_map)) ?
			    vm_map_min(dst_map) : last->vme_end;
		}
		start = vm_map_round_page(start,
		    VM_MAP_PAGE_MASK(dst_map));
	}

	while (TRUE) {
		vm_map_entry_t  next = last->vme_next;
		vm_map_offset_t end = start + size;

		if ((end > dst_map->max_offset) || (end < start)) {
			if (dst_map->wait_for_space) {
				if (size <= (dst_map->max_offset - dst_map->min_offset)) {
					assert_wait((event_t) dst_map,
					    THREAD_INTERRUPTIBLE);
					vm_map_unlock(dst_map);
					thread_block(THREAD_CONTINUE_NULL);
					goto StartAgain;
				}
			}
			vm_map_unlock(dst_map);
			return KERN_NO_SPACE;
		}

		if (dst_map->holelistenabled) {
			if (last->vme_end >= end) {
				break;
			}
		} else {
			  
                                                
     
        
     
                                           
                                                
      

			if (next == vm_map_to_entry(dst_map)) {
				break;
			}

			if (next->vme_start >= end) {
				break;
			}
		}

		last = next;

		if (dst_map->holelistenabled) {
			if (last == CAST_TO_VM_MAP_ENTRY(dst_map->holes_list)) {
				  
                     
       
				vm_map_unlock(dst_map);
				return KERN_NO_SPACE;
			}
			start = last->vme_start;
		} else {
			start = last->vme_end;
		}
		start = vm_map_round_page(start,
		    VM_MAP_PAGE_MASK(dst_map));
	}

	if (dst_map->holelistenabled) {
		if (vm_map_lookup_entry(dst_map, last->vme_start, &last)) {
			panic("Found an existing entry (%p) instead of potential hole at address: 0x%llx.\n", last, (unsigned long long)last->vme_start);
		}
	}


	adjustment = start - vm_copy_start;
	if (!consume_on_success) {
		  
                                                          
                                                         
                                                        
                                                            
                         
                                                          
                                                             
                       
     
		goto after_adjustments;
	}

	  
                                          
                                              
                                           
    

	if (copy->cpy_hdr.entries_pageable != dst_map->hdr.entries_pageable) {
		  
                                                   
           
     
		zone_t          old_zone;
		vm_map_entry_t  next, new;

		  
                                                      
     

		entry = vm_map_copy_first_entry(copy);

		  
                                                         
               
     
		vm_map_store_copy_reset(copy, entry);
		copy->cpy_hdr.entries_pageable = dst_map->hdr.entries_pageable;

		  
                     
     
		while (entry != vm_map_copy_to_entry(copy)) {
			new = vm_map_copy_entry_create(copy, !copy->cpy_hdr.entries_pageable);
			vm_map_entry_copy_full(new, entry);
			new->vme_no_copy_on_read = FALSE;
			assert(!new->iokit_acct);
			if (new->is_sub_map) {
				                                 
				new->use_pmap = FALSE;
			}
			vm_map_copy_entry_link(copy,
			    vm_map_copy_last_entry(copy),
			    new);
			next = entry->vme_next;
			old_zone = entry->from_reserved_zone ? vm_map_entry_reserved_zone : vm_map_entry_zone;
			zfree(old_zone, entry);
			entry = next;
		}
	}

	  
                                               
                                
    

	for (entry = vm_map_copy_first_entry(copy);
	    entry != vm_map_copy_to_entry(copy);
	    entry = entry->vme_next) {
		if (VM_MAP_PAGE_SHIFT(dst_map) == PAGE_SHIFT) {
			  
                                                     
                                               
                                                    
                                   
      
			entry->map_aligned = FALSE;
		}

		entry->vme_start += adjustment;
		entry->vme_end += adjustment;

		if (entry->map_aligned) {
			assert(VM_MAP_PAGE_ALIGNED(entry->vme_start,
			    VM_MAP_PAGE_MASK(dst_map)));
			assert(VM_MAP_PAGE_ALIGNED(entry->vme_end,
			    VM_MAP_PAGE_MASK(dst_map)));
		}

		entry->inheritance = VM_INHERIT_DEFAULT;
		entry->protection = VM_PROT_DEFAULT;
		entry->max_protection = VM_PROT_ALL;
		entry->behavior = VM_BEHAVIOR_DEFAULT;

		  
                               
                                            
     
		if (entry->wired_count != 0) {
			vm_map_offset_t va;
			vm_object_offset_t       offset;
			vm_object_t object;
			vm_prot_t prot;
			int     type_of_fault;

			                                               
			assert(VM_MAP_PAGE_SHIFT(dst_map) == PAGE_SHIFT);

			object = VME_OBJECT(entry);
			offset = VME_OFFSET(entry);
			va = entry->vme_start;

			pmap_pageable(dst_map->pmap,
			    entry->vme_start,
			    entry->vme_end,
			    TRUE);

			while (va < entry->vme_end) {
				vm_page_t       m;
				struct vm_object_fault_info fault_info = {};

				  
                                      
                                                
                  
             
                                      
                                     
                                      
                        
         
                                        
                                     
                                        
                                    
                                         
                
       
				vm_object_lock(object);

				m = vm_page_lookup(object, offset);
				if (m == VM_PAGE_NULL || !VM_PAGE_WIRED(m) ||
				    m->vmp_absent) {
					panic("vm_map_copyout: wiring %p", m);
				}

				prot = entry->protection;

				if (override_nx(dst_map, VME_ALIAS(entry)) &&
				    prot) {
					prot |= VM_PROT_EXECUTE;
				}

				type_of_fault = DBG_CACHE_HIT_FAULT;

				fault_info.user_tag = VME_ALIAS(entry);
				fault_info.pmap_options = 0;
				if (entry->iokit_acct ||
				    (!entry->is_sub_map && !entry->use_pmap)) {
					fault_info.pmap_options |= PMAP_OPTIONS_ALT_ACCT;
				}

				vm_fault_enter(m,
				    dst_map->pmap,
				    va,
				    PAGE_SIZE, 0,
				    prot,
				    prot,
				    VM_PAGE_WIRED(m),
				    FALSE,                               
				    VM_KERN_MEMORY_NONE,                                  
				    &fault_info,
				    NULL,                             
				    &type_of_fault);

				vm_object_unlock(object);

				offset += PAGE_SIZE_64;
				va += PAGE_SIZE;
			}
		}
	}

after_adjustments:

	  
                                             
    

	*dst_addr = start + (copy->offset - vm_copy_start);

#if KASAN
	kasan_notify_address(*dst_addr, size);
#endif

	  
                                     
    

	if (consume_on_success) {
		SAVE_HINT_MAP_WRITE(dst_map, vm_map_copy_last_entry(copy));
	} else {
		SAVE_HINT_MAP_WRITE(dst_map, last);
	}

	dst_map->size += size;

	  
                    
    

	if (consume_on_success) {
		vm_map_copy_insert(dst_map, last, copy);
		if (copy != original_copy) {
			vm_map_copy_discard(original_copy);
			original_copy = VM_MAP_COPY_NULL;
		}
	} else {
		vm_map_copy_remap(dst_map, last, copy, adjustment,
		    cur_protection, max_protection,
		    inheritance);
		if (copy != original_copy && original_copy != VM_MAP_COPY_NULL) {
			vm_map_copy_discard(copy);
			copy = original_copy;
		}
	}


	vm_map_unlock(dst_map);

	  
                                                
    

	return KERN_SUCCESS;
}

  
                         
  
               
                                                                
  
   

#undef vm_map_copyin

kern_return_t
vm_map_copyin(
	vm_map_t                        src_map,
	vm_map_address_t        src_addr,
	vm_map_size_t           len,
	boolean_t                       src_destroy,
	vm_map_copy_t           *copy_result)            
{
	return vm_map_copyin_common(src_map, src_addr, len, src_destroy,
	           FALSE, copy_result, FALSE);
}

  
                                
  
               
                                                      
                                                     
                                                           
  
           
                                                      
                                                                
                                                    
                                                      
                                                    
  
                     
                                                 
   

typedef struct submap_map {
	vm_map_t        parent_map;
	vm_map_offset_t base_start;
	vm_map_offset_t base_end;
	vm_map_size_t   base_len;
	struct submap_map *next;
} submap_map_t;

kern_return_t
vm_map_copyin_common(
	vm_map_t        src_map,
	vm_map_address_t src_addr,
	vm_map_size_t   len,
	boolean_t       src_destroy,
	__unused boolean_t      src_volatile,
	vm_map_copy_t   *copy_result,            
	boolean_t       use_maxprot)
{
	int flags;

	flags = 0;
	if (src_destroy) {
		flags |= VM_MAP_COPYIN_SRC_DESTROY;
	}
	if (use_maxprot) {
		flags |= VM_MAP_COPYIN_USE_MAXPROT;
	}
	return vm_map_copyin_internal(src_map,
	           src_addr,
	           len,
	           flags,
	           copy_result);
}
kern_return_t
vm_map_copyin_internal(
	vm_map_t        src_map,
	vm_map_address_t src_addr,
	vm_map_size_t   len,
	int             flags,
	vm_map_copy_t   *copy_result)            
{
	vm_map_entry_t  tmp_entry;                                     
                                                               
                                                             
                                                     
                                    
	vm_map_entry_t  new_entry = VM_MAP_ENTRY_NULL;                          

	vm_map_offset_t src_start;                                  
                                                                  
                                    
	vm_map_offset_t src_end;                                     
                                             
	vm_map_offset_t src_base;
	vm_map_t        base_map = src_map;
	boolean_t       map_share = FALSE;
	submap_map_t    *parent_maps = NULL;

	vm_map_copy_t   copy;                               
	vm_map_address_t copy_addr;
	vm_map_size_t   copy_size;
	boolean_t       src_destroy;
	boolean_t       use_maxprot;
	boolean_t       preserve_purgeable;
	boolean_t       entry_was_shared;
	vm_map_entry_t  saved_src_entry;

	if (flags & ~VM_MAP_COPYIN_ALL_FLAGS) {
		return KERN_INVALID_ARGUMENT;
	}

	src_destroy = (flags & VM_MAP_COPYIN_SRC_DESTROY) ? TRUE : FALSE;
	use_maxprot = (flags & VM_MAP_COPYIN_USE_MAXPROT) ? TRUE : FALSE;
	preserve_purgeable =
	    (flags & VM_MAP_COPYIN_PRESERVE_PURGEABLE) ? TRUE : FALSE;

	  
                                   
    

	if (len == 0) {
		*copy_result = VM_MAP_COPY_NULL;
		return KERN_SUCCESS;
	}

	  
                                               
    
	src_end = src_addr + len;
	if (src_end < src_addr) {
		return KERN_INVALID_ADDRESS;
	}

	  
                                                  
    
	src_start = vm_map_trunc_page(src_addr,
	    VM_MAP_PAGE_MASK(src_map));
	src_end = vm_map_round_page(src_end,
	    VM_MAP_PAGE_MASK(src_map));

	  
                                                                  
                                                                
                                                                    
                      
    
	if ((len < msg_ool_size_small) &&
	    !use_maxprot &&
	    !preserve_purgeable &&
	    !(flags & VM_MAP_COPYIN_ENTRY_LIST) &&
	      
                                                                  
                                                                        
                                                                      
                                                                         
                                                            
        
	    (src_start >= vm_map_min(src_map) &&
	    src_start < vm_map_max(src_map) &&
	    src_end >= vm_map_min(src_map) &&
	    src_end < vm_map_max(src_map))) {
		return vm_map_copyin_kernel_buffer(src_map, src_addr, len,
		           src_destroy, copy_result);
	}

	  
                                           
   
                                          
                                             
    

	copy = vm_map_copy_allocate();
	copy->type = VM_MAP_COPY_ENTRY_LIST;
	copy->cpy_hdr.entries_pageable = TRUE;
	copy->cpy_hdr.page_shift = VM_MAP_PAGE_SHIFT(src_map);

	vm_map_store_init( &(copy->cpy_hdr));

	copy->offset = src_addr;
	copy->size = len;

	new_entry = vm_map_copy_entry_create(copy, !copy->cpy_hdr.entries_pageable);

#define RETURN(x)                                               \
	MACRO_BEGIN                                             \
	vm_map_unlock(src_map);                                 \
	if(src_map != base_map)                                 \
	        vm_map_deallocate(src_map);                     \
	if (new_entry != VM_MAP_ENTRY_NULL)                     \
	        vm_map_copy_entry_dispose(copy,new_entry);      \
	vm_map_copy_discard(copy);                              \
	{                                                       \
	        submap_map_t	*_ptr;                          \
                                                                \
	        for(_ptr = parent_maps; _ptr != NULL; _ptr = parent_maps) { \
	                parent_maps=parent_maps->next;          \
	                if (_ptr->parent_map != base_map)       \
	                        vm_map_deallocate(_ptr->parent_map);    \
	                kfree(_ptr, sizeof(submap_map_t));      \
	        }                                               \
	}                                                       \
	MACRO_RETURN(x);                                        \
	MACRO_END

	  
                                     
    

	vm_map_lock(src_map);

	  
                                                            
                                                               
                                                                
    
	if (!vm_map_lookup_entry(src_map, src_addr, &tmp_entry)) {
		RETURN(KERN_INVALID_ADDRESS);
	}
	if (!tmp_entry->is_sub_map) {
		  
                                                            
                                                            
                                            
     
		vm_map_clip_start(src_map, tmp_entry, src_start);
	}
	if (src_start < tmp_entry->vme_start) {
		  
                                            
                             
     
		src_start = tmp_entry->vme_start;
	}
	                                 
	copy_addr = src_start;

	  
                                               
    

	while (TRUE) {
		vm_map_entry_t  src_entry = tmp_entry;                       
		vm_map_size_t   src_size;                                
                                                               
                                                  
                                             

		vm_object_t             src_object;                         
		vm_object_offset_t      src_offset;

		boolean_t       src_needs_copy;                             
                                                              
                                                               
                                             

		boolean_t       new_entry_needs_copy;                               

		boolean_t       was_wired;                                     
		vm_map_version_t version;                                      
                                                                 
                                             
		kern_return_t   result;                                     
                                                                
                                             
		while (tmp_entry->is_sub_map) {
			vm_map_size_t submap_len;
			submap_map_t *ptr;

			ptr = (submap_map_t *)kalloc(sizeof(submap_map_t));
			ptr->next = parent_maps;
			parent_maps = ptr;
			ptr->parent_map = src_map;
			ptr->base_start = src_start;
			ptr->base_end = src_end;
			submap_len = tmp_entry->vme_end - src_start;
			if (submap_len > (src_end - src_start)) {
				submap_len = src_end - src_start;
			}
			ptr->base_len = submap_len;

			src_start -= tmp_entry->vme_start;
			src_start += VME_OFFSET(tmp_entry);
			src_end = src_start + submap_len;
			src_map = VME_SUBMAP(tmp_entry);
			vm_map_lock(src_map);
			                                                   
			                                          
			vm_map_reference(src_map);
			vm_map_unlock(ptr->parent_map);
			if (!vm_map_lookup_entry(
				    src_map, src_start, &tmp_entry)) {
				RETURN(KERN_INVALID_ADDRESS);
			}
			map_share = TRUE;
			if (!tmp_entry->is_sub_map) {
				vm_map_clip_start(src_map, tmp_entry, src_start);
			}
			src_entry = tmp_entry;
		}
		                                              

		if ((VME_OBJECT(tmp_entry) != VM_OBJECT_NULL) &&
		    (VME_OBJECT(tmp_entry)->phys_contiguous)) {
			                                              
			                                              
			                                              
			                                              
			                                              
			                                              
			                     
			RETURN(KERN_PROTECTION_FAILURE);
		}
		  
                                                       
                                                            
                                                        
                             
     
		if (new_entry == VM_MAP_ENTRY_NULL) {
			version.main_timestamp = src_map->timestamp;
			vm_map_unlock(src_map);

			new_entry = vm_map_copy_entry_create(copy, !copy->cpy_hdr.entries_pageable);

			vm_map_lock(src_map);
			if ((version.main_timestamp + 1) != src_map->timestamp) {
				if (!vm_map_lookup_entry(src_map, src_start,
				    &tmp_entry)) {
					RETURN(KERN_INVALID_ADDRESS);
				}
				if (!tmp_entry->is_sub_map) {
					vm_map_clip_start(src_map, tmp_entry, src_start);
				}
				continue;                               
			}
		}

		  
                                        
     
		if (((src_entry->protection & VM_PROT_READ) == VM_PROT_NONE &&
		    !use_maxprot) ||
		    (src_entry->max_protection & VM_PROT_READ) == 0) {
			RETURN(KERN_PROTECTION_FAILURE);
		}

		  
                                                     
     

		vm_map_clip_end(src_map, src_entry, src_end);

		src_size = src_entry->vme_end - src_start;
		src_object = VME_OBJECT(src_entry);
		src_offset = VME_OFFSET(src_entry);
		was_wired = (src_entry->wired_count != 0);

		vm_map_entry_copy(src_map, new_entry, src_entry);
		if (new_entry->is_sub_map) {
			                                 
			new_entry->use_pmap = FALSE;
		} else {
			  
                                                   
                                                     
                                             
                                              
                          
                                                      
                                                   
                                  
      
			assert(!new_entry->iokit_acct);
			new_entry->use_pmap = TRUE;
		}

		  
                                                      
     

		  
                                                    
                                                    
                                              
                                         
                                                     
                                                    
    
                                                       
                                                    
                                                     
                                                    
                                             
     

RestartCopy:
		if ((src_object == VM_OBJECT_NULL ||
		    (!was_wired && !map_share && !tmp_entry->is_shared
		    && !(debug4k_no_cow_copyin && VM_MAP_PAGE_SHIFT(src_map) < PAGE_SHIFT))) &&
		    vm_object_copy_quickly(
			    VME_OBJECT_PTR(new_entry),
			    src_offset,
			    src_size,
			    &src_needs_copy,
			    &new_entry_needs_copy)) {
			new_entry->needs_copy = new_entry_needs_copy;

			  
                                      
      

			if (src_needs_copy && !tmp_entry->needs_copy) {
				vm_prot_t prot;

				prot = src_entry->protection & ~VM_PROT_WRITE;

				if (override_nx(src_map, VME_ALIAS(src_entry))
				    && prot) {
					prot |= VM_PROT_EXECUTE;
				}

				vm_object_pmap_protect(
					src_object,
					src_offset,
					src_size,
					(src_entry->is_shared ?
					PMAP_NULL
					: src_map->pmap),
					VM_MAP_PAGE_SIZE(src_map),
					src_entry->vme_start,
					prot);

				assert(tmp_entry->wired_count == 0);
				tmp_entry->needs_copy = TRUE;
			}

			  
                                                   
                                                 
                     
      

			goto CopySuccessful;
		}

		entry_was_shared = tmp_entry->is_shared;

		  
                                             
                             
     

		assert(src_object != VM_OBJECT_NULL);
		vm_object_reference(src_object);

		  
                                                 
                    
     

		version.main_timestamp = src_map->timestamp;
		vm_map_unlock(src_map);                                 
		saved_src_entry = src_entry;
		tmp_entry = VM_MAP_ENTRY_NULL;
		src_entry = VM_MAP_ENTRY_NULL;

		  
                     
     

		if (was_wired ||
		    (debug4k_no_cow_copyin &&
		    VM_MAP_PAGE_SHIFT(src_map) < PAGE_SHIFT)) {
CopySlowly:
			vm_object_lock(src_object);
			result = vm_object_copy_slowly(
				src_object,
				src_offset,
				src_size,
				THREAD_UNINT,
				VME_OBJECT_PTR(new_entry));
			VME_OFFSET_SET(new_entry,
			    src_offset - vm_object_trunc_page(src_offset));
			new_entry->needs_copy = FALSE;
		} else if (src_object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC &&
		    (entry_was_shared || map_share)) {
			vm_object_t new_object;

			vm_object_lock_shared(src_object);
			new_object = vm_object_copy_delayed(
				src_object,
				src_offset,
				src_size,
				TRUE);
			if (new_object == VM_OBJECT_NULL) {
				goto CopySlowly;
			}

			VME_OBJECT_SET(new_entry, new_object);
			assert(new_entry->wired_count == 0);
			new_entry->needs_copy = TRUE;
			assert(!new_entry->iokit_acct);
			assert(new_object->purgable == VM_PURGABLE_DENY);
			assertf(new_entry->use_pmap, "src_map %p new_entry %p\n", src_map, new_entry);
			result = KERN_SUCCESS;
		} else {
			vm_object_offset_t new_offset;
			new_offset = VME_OFFSET(new_entry);
			result = vm_object_copy_strategically(src_object,
			    src_offset,
			    src_size,
			    VME_OBJECT_PTR(new_entry),
			    &new_offset,
			    &new_entry_needs_copy);
			if (new_offset != VME_OFFSET(new_entry)) {
				VME_OFFSET_SET(new_entry, new_offset);
			}

			new_entry->needs_copy = new_entry_needs_copy;
		}

		if (result == KERN_SUCCESS &&
		    ((preserve_purgeable &&
		    src_object->purgable != VM_PURGABLE_DENY) ||
		    new_entry->used_for_jit)) {
			  
                                                        
                                            
     
                                                        
                                                      
                                                        
                                                  
                            
      

			vm_object_t     new_object;

			new_object = VME_OBJECT(new_entry);
			assert(new_object != src_object);
			vm_object_lock(new_object);
			assert(new_object->ref_count == 1);
			assert(new_object->shadow == VM_OBJECT_NULL);
			assert(new_object->copy == VM_OBJECT_NULL);
			assert(new_object->vo_owner == NULL);

			new_object->copy_strategy = MEMORY_OBJECT_COPY_NONE;

			if (preserve_purgeable &&
			    src_object->purgable != VM_PURGABLE_DENY) {
				new_object->true_share = TRUE;

				                                            
				new_object->purgable = VM_PURGABLE_NONVOLATILE;
				vm_purgeable_nonvolatile_enqueue(new_object, NULL);
				                                                  
				if (src_object->purgable != VM_PURGABLE_NONVOLATILE) {
					int state;
					state = src_object->purgable;
					vm_object_purgable_control(
						new_object,
						VM_PURGABLE_SET_STATE_FROM_KERNEL,
						&state);
				}
				                                              
				new_entry->use_pmap = FALSE;
			}

			vm_object_unlock(new_object);
			new_object = VM_OBJECT_NULL;
		}

		if (result != KERN_SUCCESS &&
		    result != KERN_MEMORY_RESTART_COPY) {
			vm_map_lock(src_map);
			RETURN(result);
		}

		  
                                   
     

		vm_object_deallocate(src_object);

		  
                                              
                                           
     

		vm_map_lock(src_map);

		if ((version.main_timestamp + 1) == src_map->timestamp) {
			                                                      
			src_entry = saved_src_entry;
			goto VerificationSuccessful;
		}

		  
                                      
    
                                         
                                          
    
                                               
                                             
                                              
                                                   
     

		if (!vm_map_lookup_entry(src_map, src_start, &tmp_entry)) {
			if (result != KERN_MEMORY_RESTART_COPY) {
				vm_object_deallocate(VME_OBJECT(new_entry));
				VME_OBJECT_SET(new_entry, VM_OBJECT_NULL);
				                            
				new_entry->iokit_acct = FALSE;
				new_entry->use_pmap = TRUE;
			}
			RETURN(KERN_INVALID_ADDRESS);
		}

		src_entry = tmp_entry;
		vm_map_clip_start(src_map, src_entry, src_start);

		if ((((src_entry->protection & VM_PROT_READ) == VM_PROT_NONE) &&
		    !use_maxprot) ||
		    ((src_entry->max_protection & VM_PROT_READ) == 0)) {
			goto VerificationFailed;
		}

		if (src_entry->vme_end < new_entry->vme_end) {
			  
                                          
                                             
                                              
                  
                                             
                                              
                                                
                                                 
      
			assert(VM_MAP_PAGE_ALIGNED(src_entry->vme_end,
			    VM_MAP_COPY_PAGE_MASK(copy)));
			new_entry->vme_end = src_entry->vme_end;
			src_size = new_entry->vme_end - src_start;
		} else if (src_entry->vme_end > new_entry->vme_end) {
			  
                                         
                                           
                                                      
                                   
     
                                             
                                                 
                                              
                                                  
                                                
                                   
      
		}

		if ((VME_OBJECT(src_entry) != src_object) ||
		    (VME_OFFSET(src_entry) != src_offset) ||
		    (src_entry->vme_end > new_entry->vme_end)) {
			  
                          
     
                                           
      

VerificationFailed:     ;

			vm_object_deallocate(VME_OBJECT(new_entry));
			tmp_entry = src_entry;
			continue;
		}

		  
                            
     

VerificationSuccessful:;

		if (result == KERN_MEMORY_RESTART_COPY) {
			goto RestartCopy;
		}

		  
                    
     

CopySuccessful: ;

		  
                                
     

		vm_map_copy_entry_link(copy, vm_map_copy_last_entry(copy),
		    new_entry);

		  
                                        
                     
     
		src_base = src_start;
		src_start = new_entry->vme_end;
		new_entry = VM_MAP_ENTRY_NULL;
		while ((src_start >= src_end) && (src_end != 0)) {
			submap_map_t    *ptr;

			if (src_map == base_map) {
				                     
				break;
			}

			ptr = parent_maps;
			assert(ptr != NULL);
			parent_maps = parent_maps->next;

			                                             
			vm_map_simplify_range(src_map,
			    src_base,
			    src_end);

			vm_map_unlock(src_map);
			vm_map_deallocate(src_map);
			vm_map_lock(ptr->parent_map);
			src_map = ptr->parent_map;
			src_base = ptr->base_start;
			src_start = ptr->base_start + ptr->base_len;
			src_end = ptr->base_end;
			if (!vm_map_lookup_entry(src_map,
			    src_start,
			    &tmp_entry) &&
			    (src_end > src_start)) {
				RETURN(KERN_INVALID_ADDRESS);
			}
			kfree(ptr, sizeof(submap_map_t));
			if (parent_maps == NULL) {
				map_share = FALSE;
			}
			src_entry = tmp_entry->vme_prev;
		}

		if ((VM_MAP_PAGE_SHIFT(src_map) != PAGE_SHIFT) &&
		    (src_start >= src_addr + len) &&
		    (src_addr + len != 0)) {
			  
                                                      
                                                       
                                  
     
                                                 
                                             
                                               
                                                 
                                
                                                    
                                           
      
			break;
		}

		if ((src_start >= src_end) && (src_end != 0)) {
			break;
		}

		  
                                                
     

		tmp_entry = src_entry->vme_next;
		if ((tmp_entry->vme_start != src_start) ||
		    (tmp_entry == vm_map_to_entry(src_map))) {
			RETURN(KERN_INVALID_ADDRESS);
		}
	}

	  
                                                           
                        
    
	if (src_destroy) {
		(void) vm_map_delete(
			src_map,
			vm_map_trunc_page(src_addr,
			VM_MAP_PAGE_MASK(src_map)),
			src_end,
			((src_map == kernel_map) ?
			VM_MAP_REMOVE_KUNWIRE :
			VM_MAP_REMOVE_NO_FLAGS),
			VM_MAP_NULL);
	} else {
		                                              
		vm_map_simplify_range(
			src_map,
			vm_map_trunc_page(src_addr,
			VM_MAP_PAGE_MASK(src_map)),
			vm_map_round_page(src_end,
			VM_MAP_PAGE_MASK(src_map)));
	}

	vm_map_unlock(src_map);
	tmp_entry = VM_MAP_ENTRY_NULL;

	if (VM_MAP_PAGE_SHIFT(src_map) > PAGE_SHIFT &&
	    VM_MAP_PAGE_SHIFT(src_map) != VM_MAP_COPY_PAGE_SHIFT(copy)) {
		vm_map_offset_t original_start, original_offset, original_end;

		assert(VM_MAP_COPY_PAGE_MASK(copy) == PAGE_MASK);

		                                                        
		tmp_entry = vm_map_copy_first_entry(copy);
		if (tmp_entry != vm_map_copy_to_entry(copy)) {
			vm_map_offset_t adjustment;

			original_start = tmp_entry->vme_start;
			original_offset = VME_OFFSET(tmp_entry);

			                                                    
			adjustment = (tmp_entry->vme_start -
			    vm_map_trunc_page(
				    tmp_entry->vme_start,
				    VM_MAP_PAGE_MASK(src_map)));
			tmp_entry->vme_start -= adjustment;
			VME_OFFSET_SET(tmp_entry,
			    VME_OFFSET(tmp_entry) - adjustment);
			copy_addr -= adjustment;
			assert(tmp_entry->vme_start < tmp_entry->vme_end);
			                                                    
			adjustment =
			    (vm_map_trunc_page(copy->offset,
			    PAGE_MASK) -
			    vm_map_trunc_page(copy->offset,
			    VM_MAP_PAGE_MASK(src_map)));
			if (adjustment) {
				assert(page_aligned(adjustment));
				assert(adjustment < VM_MAP_PAGE_SIZE(src_map));
				tmp_entry->vme_start += adjustment;
				VME_OFFSET_SET(tmp_entry,
				    (VME_OFFSET(tmp_entry) +
				    adjustment));
				copy_addr += adjustment;
				assert(tmp_entry->vme_start < tmp_entry->vme_end);
			}

			  
                                                 
                                        
      
			assert(tmp_entry->vme_start >= original_start);
			assert(VME_OFFSET(tmp_entry) >= original_offset);
			  
                                                 
                        
      
			assert(vm_map_trunc_page(tmp_entry->vme_start,
			    VM_MAP_PAGE_MASK(src_map)) ==
			    vm_map_trunc_page(original_start,
			    VM_MAP_PAGE_MASK(src_map)));
		}

		                                                     
		tmp_entry = vm_map_copy_last_entry(copy);
		if (tmp_entry != vm_map_copy_to_entry(copy)) {
			vm_map_offset_t adjustment;

			original_end = tmp_entry->vme_end;

			                                                 
			tmp_entry->vme_end =
			    vm_map_round_page(tmp_entry->vme_end,
			    VM_MAP_PAGE_MASK(src_map));
			                                                  
			adjustment =
			    (vm_map_round_page((copy->offset +
			    copy->size),
			    VM_MAP_PAGE_MASK(src_map)) -
			    vm_map_round_page((copy->offset +
			    copy->size),
			    PAGE_MASK));
			if (adjustment) {
				assert(page_aligned(adjustment));
				assert(adjustment < VM_MAP_PAGE_SIZE(src_map));
				tmp_entry->vme_end -= adjustment;
				assert(tmp_entry->vme_start < tmp_entry->vme_end);
			}

			  
                                                 
                                        
      
			assert(tmp_entry->vme_end <= original_end);
			  
                                                 
                        
      
			assert(vm_map_round_page(tmp_entry->vme_end,
			    VM_MAP_PAGE_MASK(src_map)) ==
			    vm_map_round_page(original_end,
			    VM_MAP_PAGE_MASK(src_map)));
		}
	}

	                                                             
	                                                             
	                                

	tmp_entry = vm_map_copy_first_entry(copy);
	copy_size = 0;                          
	while (tmp_entry != vm_map_copy_to_entry(copy)) {
		assert(VM_MAP_PAGE_ALIGNED(
			    copy_addr + (tmp_entry->vme_end -
			    tmp_entry->vme_start),
			    MIN(VM_MAP_COPY_PAGE_MASK(copy), PAGE_MASK)));
		assert(VM_MAP_PAGE_ALIGNED(
			    copy_addr,
			    MIN(VM_MAP_COPY_PAGE_MASK(copy), PAGE_MASK)));

		  
                                                        
                                                            
     
		tmp_entry->map_aligned = FALSE;

		tmp_entry->vme_end = copy_addr +
		    (tmp_entry->vme_end - tmp_entry->vme_start);
		tmp_entry->vme_start = copy_addr;
		assert(tmp_entry->vme_start < tmp_entry->vme_end);
		copy_addr += tmp_entry->vme_end - tmp_entry->vme_start;
		copy_size += tmp_entry->vme_end - tmp_entry->vme_start;
		tmp_entry = (struct vm_map_entry *)tmp_entry->vme_next;
	}

	if (VM_MAP_PAGE_SHIFT(src_map) != PAGE_SHIFT &&
	    copy_size < copy->size) {
		  
                                                            
                                                            
                                                             
                                                        
                                                          
                                                         
                                                            
                         
                                                          
                                                    
     
		assert(vm_map_round_page(copy_size,
		    VM_MAP_PAGE_MASK(src_map)) ==
		    vm_map_round_page(copy->size,
		    VM_MAP_PAGE_MASK(src_map)));
		copy->size = copy_size;
	}

	*copy_result = copy;
	return KERN_SUCCESS;

#undef  RETURN
}

kern_return_t
vm_map_copy_extract(
	vm_map_t                src_map,
	vm_map_address_t        src_addr,
	vm_map_size_t           len,
	boolean_t               do_copy,
	vm_map_copy_t           *copy_result,            
	vm_prot_t               *cur_prot,                  
	vm_prot_t               *max_prot,                  
	vm_inherit_t            inheritance,
	vm_map_kernel_flags_t   vmk_flags)
{
	vm_map_copy_t   copy;
	kern_return_t   kr;
	vm_prot_t required_cur_prot, required_max_prot;

	  
                                   
    

	if (len == 0) {
		*copy_result = VM_MAP_COPY_NULL;
		return KERN_SUCCESS;
	}

	  
                                               
    
	if (src_addr + len < src_addr) {
		return KERN_INVALID_ADDRESS;
	}

	if (VM_MAP_PAGE_SIZE(src_map) < PAGE_SIZE) {
		DEBUG4K_SHARE("src_map %p src_addr 0x%llx src_end 0x%llx\n", src_map, (uint64_t)src_addr, (uint64_t)(src_addr + len));
	}

	required_cur_prot = *cur_prot;
	required_max_prot = *max_prot;

	  
                                           
   
                                          
                                             
    

	copy = vm_map_copy_allocate();
	copy->type = VM_MAP_COPY_ENTRY_LIST;
	copy->cpy_hdr.entries_pageable = vmk_flags.vmkf_copy_pageable;

	vm_map_store_init(&copy->cpy_hdr);

	copy->offset = 0;
	copy->size = len;

	kr = vm_map_remap_extract(src_map,
	    src_addr,
	    len,
	    do_copy,                       
	    &copy->cpy_hdr,
	    cur_prot,                        
	    max_prot,                        
	    inheritance,
	    vmk_flags);
	if (kr != KERN_SUCCESS) {
		vm_map_copy_discard(copy);
		return kr;
	}
	if (required_cur_prot != VM_PROT_NONE) {
		assert((*cur_prot & required_cur_prot) == required_cur_prot);
		assert((*max_prot & required_max_prot) == required_max_prot);
	}

	*copy_result = copy;
	return KERN_SUCCESS;
}

  
                        
  
                                       
                                          
   

kern_return_t
vm_map_copyin_object(
	vm_object_t             object,
	vm_object_offset_t      offset,                                 
	vm_object_size_t        size,                                 
	vm_map_copy_t   *copy_result)            
{
	vm_map_copy_t   copy;                               

	  
                                                 
                                      
    

	copy = vm_map_copy_allocate();
	copy->type = VM_MAP_COPY_OBJECT;
	copy->cpy_object = object;
	copy->offset = offset;
	copy->size = size;

	*copy_result = copy;
	return KERN_SUCCESS;
}

static void
vm_map_fork_share(
	vm_map_t        old_map,
	vm_map_entry_t  old_entry,
	vm_map_t        new_map)
{
	vm_object_t     object;
	vm_map_entry_t  new_entry;

	  
                                    
                                         
                                               
                                           
                                           
                                         
                                   
    

	object = VME_OBJECT(old_entry);
	if (old_entry->is_sub_map) {
		assert(old_entry->wired_count == 0);
#ifndef NO_NESTED_PMAP
		if (old_entry->use_pmap) {
			kern_return_t   result;

			result = pmap_nest(new_map->pmap,
			    (VME_SUBMAP(old_entry))->pmap,
			    (addr64_t)old_entry->vme_start,
			    (uint64_t)(old_entry->vme_end - old_entry->vme_start));
			if (result) {
				panic("vm_map_fork_share: pmap_nest failed!");
			}
		}
#endif                      
	} else if (object == VM_OBJECT_NULL) {
		object = vm_object_allocate((vm_map_size_t)(old_entry->vme_end -
		    old_entry->vme_start));
		VME_OFFSET_SET(old_entry, 0);
		VME_OBJECT_SET(old_entry, object);
		old_entry->use_pmap = TRUE;
                                   
	} else if (object->copy_strategy !=
	    MEMORY_OBJECT_COPY_SYMMETRIC) {
		  
                                       
                                        
                      
     

		assert(!old_entry->needs_copy);
	} else if (old_entry->needs_copy ||                   
	    object->shadowed ||                             
	    (!object->true_share &&                         
	    !old_entry->is_shared &&
	    (object->vo_size >
	    (vm_map_size_t)(old_entry->vme_end -
	    old_entry->vme_start)))) {
		  
                                
                                
                                  
                                    
                                  
                                   
                                    
                                     
                                 
                                 
                                   
                          
    
                                  
                                    
                                         
                                   
                                        
                                       
                                   
                                     
                                    
                                      
                                     
                                        
                                        
                                      
                
    
                                       
                                         
                                        
                                   
                                       
                                        
                                      
                                      
                                         
                                          
                                          
                                    
                                         
                                           
                                        
                                          
                                      
                                        
                                      
                                           
                                        
                                       
                                            
                                          
                                     
                                        
                                         
                                          
                                         
                                       
                                        
                                      
                                        
                                       
                                          
                                      
                         
    
                                      
                                    
                                       
                                     
                                      
                                     
             
     
		VME_OBJECT_SHADOW(old_entry,
		    (vm_map_size_t) (old_entry->vme_end -
		    old_entry->vme_start));

		  
                                            
                                        
                                
     

		if (!old_entry->needs_copy &&
		    (old_entry->protection & VM_PROT_WRITE)) {
			vm_prot_t prot;

			assert(!pmap_has_prot_policy(old_map->pmap, old_entry->translated_allow_execute, old_entry->protection));

			prot = old_entry->protection & ~VM_PROT_WRITE;

			assert(!pmap_has_prot_policy(old_map->pmap, old_entry->translated_allow_execute, prot));

			if (override_nx(old_map, VME_ALIAS(old_entry)) && prot) {
				prot |= VM_PROT_EXECUTE;
			}


			if (old_map->mapped_in_other_pmaps) {
				vm_object_pmap_protect(
					VME_OBJECT(old_entry),
					VME_OFFSET(old_entry),
					(old_entry->vme_end -
					old_entry->vme_start),
					PMAP_NULL,
					PAGE_SIZE,
					old_entry->vme_start,
					prot);
			} else {
				pmap_protect(old_map->pmap,
				    old_entry->vme_start,
				    old_entry->vme_end,
				    prot);
			}
		}

		old_entry->needs_copy = FALSE;
		object = VME_OBJECT(old_entry);
	}


	  
                                                  
                                           
                                                 
                                              
                                                
              
    

	if (old_entry->is_sub_map) {
		vm_map_lock(VME_SUBMAP(old_entry));
		vm_map_reference(VME_SUBMAP(old_entry));
		vm_map_unlock(VME_SUBMAP(old_entry));
	} else {
		vm_object_lock(object);
		vm_object_reference_locked(object);
		if (object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
			object->copy_strategy = MEMORY_OBJECT_COPY_DELAY;
		}
		vm_object_unlock(object);
	}

	  
                                                 
                                
    

	new_entry = vm_map_entry_create(new_map, FALSE);                    
                                                                          
	vm_map_entry_copy(old_map, new_entry, old_entry);
	old_entry->is_shared = TRUE;
	new_entry->is_shared = TRUE;

	  
                                                                 
                                                                      
                                                                 
                                                                      
                                          
    
	assert(!new_entry->iokit_acct);

	  
                                                  
                                                
                                        
    
	if (old_entry->inheritance == VM_INHERIT_NONE) {
		new_entry->protection &= ~VM_PROT_WRITE;
		new_entry->max_protection &= ~VM_PROT_WRITE;
	}

	  
                                           
                                              
        
    

	vm_map_store_entry_link(new_map, vm_map_last_entry(new_map), new_entry,
	    VM_MAP_KERNEL_FLAGS_NONE);

	  
                           
    

	if (old_entry->is_sub_map) {
		                                        
	} else {
		pmap_copy(new_map->pmap, old_map->pmap, new_entry->vme_start,
		    old_entry->vme_end - old_entry->vme_start,
		    old_entry->vme_start);
	}
}

static boolean_t
vm_map_fork_copy(
	vm_map_t        old_map,
	vm_map_entry_t  *old_entry_p,
	vm_map_t        new_map,
	int             vm_map_copyin_flags)
{
	vm_map_entry_t old_entry = *old_entry_p;
	vm_map_size_t entry_size = old_entry->vme_end - old_entry->vme_start;
	vm_map_offset_t start = old_entry->vme_start;
	vm_map_copy_t copy;
	vm_map_entry_t last = vm_map_last_entry(new_map);

	vm_map_unlock(old_map);
	  
                                            
                                           
                                                 
              
    
	vm_map_copyin_flags |= VM_MAP_COPYIN_USE_MAXPROT;
	if (vm_map_copyin_internal(old_map, start, entry_size,
	    vm_map_copyin_flags, &copy)
	    != KERN_SUCCESS) {
		  
                                        
                                        
                                   
                       
     
		vm_map_lock(old_map);
		if (!vm_map_lookup_entry(old_map, start, &last) ||
		    (last->max_protection & VM_PROT_READ) == VM_PROT_NONE) {
			last = last->vme_next;
		}
		*old_entry_p = last;

		  
                                        
                                        
                             
                                          
     

		return FALSE;
	}

	  
                                                        
                               
    
	vm_map_copy_require(copy);

	  
                                    
    
	vm_map_copy_insert(new_map, last, copy);

	  
                                       
                      
    

	vm_map_lock(old_map);
	start += entry_size;
	if (!vm_map_lookup_entry(old_map, start, &last)) {
		last = last->vme_next;
	} else {
		if (last->vme_start == start) {
			  
                                       
                                   
                  
      
		} else {
			vm_map_clip_start(old_map, last, start);
		}
	}
	*old_entry_p = last;

	return TRUE;
}

  
               
  
                                               
                                                  
                                       
  
                                     
   
vm_map_t
vm_map_fork(
	ledger_t        ledger,
	vm_map_t        old_map,
	int             options)
{
	pmap_t          new_pmap;
	vm_map_t        new_map;
	vm_map_entry_t  old_entry;
	vm_map_size_t   new_size = 0, entry_size;
	vm_map_entry_t  new_entry;
	boolean_t       src_needs_copy;
	boolean_t       new_entry_needs_copy;
	boolean_t       pmap_is64bit;
	int             vm_map_copyin_flags;
	vm_inherit_t    old_entry_inheritance;
	int             map_create_options;
	kern_return_t   footprint_collect_kr;

	if (options & ~(VM_MAP_FORK_SHARE_IF_INHERIT_NONE |
	    VM_MAP_FORK_PRESERVE_PURGEABLE |
	    VM_MAP_FORK_CORPSE_FOOTPRINT)) {
		                        
		return VM_MAP_NULL;
	}

	pmap_is64bit =
#if defined(__i386__) || defined(__x86_64__)
	    old_map->pmap->pm_task_map != TASK_MAP_32BIT;
#elif defined(__arm64__)
	    old_map->pmap->max == MACH_VM_MAX_ADDRESS;
#elif defined(__arm__)
	    FALSE;
#else
#error Unknown architecture.
#endif

	unsigned int pmap_flags = 0;
	pmap_flags |= pmap_is64bit ? PMAP_CREATE_64BIT : 0;
#if defined(HAS_APPLE_PAC)
	pmap_flags |= old_map->pmap->disable_jop ? PMAP_CREATE_DISABLE_JOP : 0;
#endif
#if PMAP_CREATE_FORCE_4K_PAGES
	if (VM_MAP_PAGE_SIZE(old_map) == FOURK_PAGE_SIZE &&
	    PAGE_SIZE != FOURK_PAGE_SIZE) {
		pmap_flags |= PMAP_CREATE_FORCE_4K_PAGES;
	}
#endif                                 
	new_pmap = pmap_create_options(ledger, (vm_map_size_t) 0, pmap_flags);

	vm_map_reference(old_map);
	vm_map_lock(old_map);

	map_create_options = 0;
	if (old_map->hdr.entries_pageable) {
		map_create_options |= VM_MAP_CREATE_PAGEABLE;
	}
	if (options & VM_MAP_FORK_CORPSE_FOOTPRINT) {
		map_create_options |= VM_MAP_CREATE_CORPSE_FOOTPRINT;
		footprint_collect_kr = KERN_SUCCESS;
	}
	new_map = vm_map_create_options(new_pmap,
	    old_map->min_offset,
	    old_map->max_offset,
	    map_create_options);
	                            
	vm_map_cs_enforcement_set(new_map, old_map->cs_enforcement);
	vm_map_lock(new_map);
	vm_commit_pagezero_status(new_map);
	                                        
	vm_map_set_page_shift(new_map, VM_MAP_PAGE_SHIFT(old_map));
	for (
		old_entry = vm_map_first_entry(old_map);
		old_entry != vm_map_to_entry(old_map);
		) {
		entry_size = old_entry->vme_end - old_entry->vme_start;

		old_entry_inheritance = old_entry->inheritance;
		  
                                                                
                                                           
                  
     
		if (old_entry_inheritance == VM_INHERIT_NONE &&
		    (options & VM_MAP_FORK_SHARE_IF_INHERIT_NONE) &&
		    (old_entry->protection & VM_PROT_READ) &&
		    !(!old_entry->is_sub_map &&
		    VME_OBJECT(old_entry) != NULL &&
		    VME_OBJECT(old_entry)->pager != NULL &&
		    is_device_pager_ops(
			    VME_OBJECT(old_entry)->pager->mo_pager_ops))) {
			old_entry_inheritance = VM_INHERIT_SHARE;
		}

		if (old_entry_inheritance != VM_INHERIT_NONE &&
		    (options & VM_MAP_FORK_CORPSE_FOOTPRINT) &&
		    footprint_collect_kr == KERN_SUCCESS) {
			  
                                                  
                                                     
                                                     
                        
      
			footprint_collect_kr =
			    vm_map_corpse_footprint_collect(old_map,
			    old_entry,
			    new_map);
		}

		switch (old_entry_inheritance) {
		case VM_INHERIT_NONE:
			break;

		case VM_INHERIT_SHARE:
			vm_map_fork_share(old_map, old_entry, new_map);
			new_size += entry_size;
			break;

		case VM_INHERIT_COPY:

			  
                                   
                                     
                          
      

			if (old_entry->is_sub_map) {
				break;
			}
			if ((old_entry->wired_count != 0) ||
			    ((VME_OBJECT(old_entry) != NULL) &&
			    (VME_OBJECT(old_entry)->true_share))) {
				goto slow_vm_map_fork_copy;
			}

			new_entry = vm_map_entry_create(new_map, FALSE);                                          
			vm_map_entry_copy(old_map, new_entry, old_entry);

			if (new_entry->used_for_jit == TRUE && new_map->jit_entry_exists == FALSE) {
				new_map->jit_entry_exists = TRUE;
			}

			if (new_entry->is_sub_map) {
				                                   
				new_entry->use_pmap = FALSE;
			} else {
				  
                                                    
                                                  
                                                  
                                               
                           
                                                
                                          
                                                    
       
				assert(!new_entry->iokit_acct);
				new_entry->use_pmap = TRUE;
			}

			if (!vm_object_copy_quickly(
				    VME_OBJECT_PTR(new_entry),
				    VME_OFFSET(old_entry),
				    (old_entry->vme_end -
				    old_entry->vme_start),
				    &src_needs_copy,
				    &new_entry_needs_copy)) {
				vm_map_entry_dispose(new_map, new_entry);
				goto slow_vm_map_fork_copy;
			}

			  
                                      
      

			if (src_needs_copy && !old_entry->needs_copy) {
				vm_prot_t prot;

				assert(!pmap_has_prot_policy(old_map->pmap, old_entry->translated_allow_execute, old_entry->protection));

				prot = old_entry->protection & ~VM_PROT_WRITE;

				if (override_nx(old_map, VME_ALIAS(old_entry))
				    && prot) {
					prot |= VM_PROT_EXECUTE;
				}

				assert(!pmap_has_prot_policy(old_map->pmap, old_entry->translated_allow_execute, prot));

				vm_object_pmap_protect(
					VME_OBJECT(old_entry),
					VME_OFFSET(old_entry),
					(old_entry->vme_end -
					old_entry->vme_start),
					((old_entry->is_shared
					|| old_map->mapped_in_other_pmaps)
					? PMAP_NULL :
					old_map->pmap),
					VM_MAP_PAGE_SIZE(old_map),
					old_entry->vme_start,
					prot);

				assert(old_entry->wired_count == 0);
				old_entry->needs_copy = TRUE;
			}
			new_entry->needs_copy = new_entry_needs_copy;

			  
                                 
                 
      

			vm_map_store_entry_link(new_map,
			    vm_map_last_entry(new_map),
			    new_entry,
			    VM_MAP_KERNEL_FLAGS_NONE);
			new_size += entry_size;
			break;

slow_vm_map_fork_copy:
			vm_map_copyin_flags = 0;
			if (options & VM_MAP_FORK_PRESERVE_PURGEABLE) {
				vm_map_copyin_flags |=
				    VM_MAP_COPYIN_PRESERVE_PURGEABLE;
			}
			if (vm_map_fork_copy(old_map,
			    &old_entry,
			    new_map,
			    vm_map_copyin_flags)) {
				new_size += entry_size;
			}
			continue;
		}
		old_entry = old_entry->vme_next;
	}

#if defined(__arm64__)
	pmap_insert_sharedpage(new_map->pmap);
#endif                

	new_map->size = new_size;

	if (options & VM_MAP_FORK_CORPSE_FOOTPRINT) {
		vm_map_corpse_footprint_collect_done(new_map);
	}

	                                                   
	if (pmap_get_jit_entitled(old_map->pmap)) {
		                                         
		pmap_set_jit_entitled(new_map->pmap);
	}

	vm_map_unlock(new_map);
	vm_map_unlock(old_map);
	vm_map_deallocate(old_map);

	return new_map;
}

  
               
  
                                                                           
                                                                   
                                              
   
kern_return_t
vm_map_exec(
	vm_map_t        new_map,
	task_t          task,
	boolean_t       is64bit,
	void            *fsroot,
	cpu_type_t      cpu,
	cpu_subtype_t   cpu_subtype,
	boolean_t       reslide)
{
	SHARED_REGION_TRACE_DEBUG(
		("shared_region: task %p: vm_map_exec(%p,%p,%p,0x%x,0x%x): ->\n",
		(void *)VM_KERNEL_ADDRPERM(current_task()),
		(void *)VM_KERNEL_ADDRPERM(new_map),
		(void *)VM_KERNEL_ADDRPERM(task),
		(void *)VM_KERNEL_ADDRPERM(fsroot),
		cpu,
		cpu_subtype));
	(void) vm_commpage_enter(new_map, task, is64bit);

	(void) vm_shared_region_enter(new_map, task, is64bit, fsroot, cpu, cpu_subtype, reslide);

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: task %p: vm_map_exec(%p,%p,%p,0x%x,0x%x): <-\n",
		(void *)VM_KERNEL_ADDRPERM(current_task()),
		(void *)VM_KERNEL_ADDRPERM(new_map),
		(void *)VM_KERNEL_ADDRPERM(task),
		(void *)VM_KERNEL_ADDRPERM(fsroot),
		cpu,
		cpu_subtype));

	  
                                                                         
                                                                              
                                                                          
                  
    
	kern_return_t kr = KERN_FAILURE;
	vm_map_kernel_flags_t vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
	vmk_flags.vmkf_permanent = TRUE;
	vmk_flags.vmkf_beyond_max = TRUE;

	struct vm_reserved_region *regions = NULL;
	size_t num_regions = ml_get_vm_reserved_regions(is64bit, &regions);
	assert((num_regions == 0) || (num_regions > 0 && regions != NULL));

	for (size_t i = 0; i < num_regions; ++i) {
		kr = vm_map_enter(
			new_map,
			&regions[i].vmrr_addr,
			regions[i].vmrr_size,
			(vm_map_offset_t)0,
			VM_FLAGS_FIXED,
			vmk_flags,
			VM_KERN_MEMORY_NONE,
			VM_OBJECT_NULL,
			(vm_object_offset_t)0,
			FALSE,
			VM_PROT_NONE,
			VM_PROT_NONE,
			VM_INHERIT_NONE);

		if (kr != KERN_SUCCESS) {
			panic("Failed to reserve %s region in user map %p %d", regions[i].vmrr_name, new_map, kr);
		}
	}

	new_map->reserved_regions = (num_regions ? TRUE : FALSE);

	return KERN_SUCCESS;
}

uint64_t vm_map_lookup_locked_copy_slowly_count = 0;
uint64_t vm_map_lookup_locked_copy_slowly_size = 0;
uint64_t vm_map_lookup_locked_copy_slowly_max = 0;
uint64_t vm_map_lookup_locked_copy_slowly_restart = 0;
uint64_t vm_map_lookup_locked_copy_slowly_error = 0;
uint64_t vm_map_lookup_locked_copy_strategically_count = 0;
uint64_t vm_map_lookup_locked_copy_strategically_size = 0;
uint64_t vm_map_lookup_locked_copy_strategically_max = 0;
uint64_t vm_map_lookup_locked_copy_strategically_restart = 0;
uint64_t vm_map_lookup_locked_copy_strategically_error = 0;
uint64_t vm_map_lookup_locked_copy_shadow_count = 0;
uint64_t vm_map_lookup_locked_copy_shadow_size = 0;
uint64_t vm_map_lookup_locked_copy_shadow_max = 0;
  
                        
  
                                   
                                                
                                              
                  
  
                                               
                                                      
                                                           
                                                    
               
                                                  
                                                      
                     
  
                                                   
                                             
                                                   
          
  
                                                   
                                                       
                                                        
                   
   
kern_return_t
vm_map_lookup_locked(
	vm_map_t                *var_map,                   
	vm_map_offset_t         vaddr,
	vm_prot_t               fault_type,
	int                     object_lock_type,
	vm_map_version_t        *out_version,            
	vm_object_t             *object,                 
	vm_object_offset_t      *offset,                 
	vm_prot_t               *out_prot,               
	boolean_t               *wired,                  
	vm_object_fault_info_t  fault_info,              
	vm_map_t                *real_map,               
	bool                    *contended)              
{
	vm_map_entry_t                  entry;
	vm_map_t                        map = *var_map;
	vm_map_t                        old_map = *var_map;
	vm_map_t                        cow_sub_map_parent = VM_MAP_NULL;
	vm_map_offset_t                 cow_parent_vaddr = 0;
	vm_map_offset_t                 old_start = 0;
	vm_map_offset_t                 old_end = 0;
	vm_prot_t                       prot;
	boolean_t                       mask_protections;
	boolean_t                       force_copy;
	boolean_t                       no_force_copy_if_executable;
	boolean_t                       submap_needed_copy;
	vm_prot_t                       original_fault_type;
	vm_map_size_t                   fault_page_mask;

	  
                                                                   
                                                                 
                   
    
	mask_protections = (fault_type & VM_PROT_IS_MASK) ? TRUE : FALSE;
	force_copy = (fault_type & VM_PROT_COPY) ? TRUE : FALSE;
	no_force_copy_if_executable = (fault_type & VM_PROT_COPY_FAIL_IF_EXECUTABLE) ? TRUE : FALSE;
	fault_type &= VM_PROT_ALL;
	original_fault_type = fault_type;
	if (contended) {
		*contended = false;
	}

	*real_map = map;

	fault_page_mask = MIN(VM_MAP_PAGE_MASK(map), PAGE_MASK);
	vaddr = VM_MAP_TRUNC_PAGE(vaddr, fault_page_mask);

RetryLookup:
	fault_type = original_fault_type;

	  
                                                             
                              
    
	entry = map->hint;

	if ((entry == vm_map_to_entry(map)) ||
	    (vaddr < entry->vme_start) || (vaddr >= entry->vme_end)) {
		vm_map_entry_t  tmp_entry;

		  
                                                    
                                                         
     
		if (!vm_map_lookup_entry(map, vaddr, &tmp_entry)) {
			if ((cow_sub_map_parent) && (cow_sub_map_parent != map)) {
				vm_map_unlock(cow_sub_map_parent);
			}
			if ((*real_map != map)
			    && (*real_map != cow_sub_map_parent)) {
				vm_map_unlock(*real_map);
			}
			return KERN_INVALID_ADDRESS;
		}

		entry = tmp_entry;
	}
	if (map == old_map) {
		old_start = entry->vme_start;
		old_end = entry->vme_end;
	}

	  
                                                      
                    
    

	submap_needed_copy = FALSE;
submap_recurse:
	if (entry->is_sub_map) {
		vm_map_offset_t         local_vaddr;
		vm_map_offset_t         end_delta;
		vm_map_offset_t         start_delta;
		vm_map_entry_t          submap_entry, saved_submap_entry;
		vm_object_offset_t      submap_entry_offset;
		vm_object_size_t        submap_entry_size;
		vm_prot_t               subentry_protection;
		vm_prot_t               subentry_max_protection;
		boolean_t               subentry_no_copy_on_read;
		boolean_t               mapped_needs_copy = FALSE;
		vm_map_version_t        version;

		assertf(VM_MAP_PAGE_SHIFT(VME_SUBMAP(entry)) >= VM_MAP_PAGE_SHIFT(map),
		    "map %p (%d) entry %p submap %p (%d)\n",
		    map, VM_MAP_PAGE_SHIFT(map), entry,
		    VME_SUBMAP(entry), VM_MAP_PAGE_SHIFT(VME_SUBMAP(entry)));

		local_vaddr = vaddr;

		if ((entry->use_pmap &&
		    !((fault_type & VM_PROT_WRITE) ||
		    force_copy))) {
			                                            
			if ((*real_map != map) &&
			    (*real_map != cow_sub_map_parent)) {
				vm_map_unlock(*real_map);
			}
			*real_map = VME_SUBMAP(entry);
		}

		if (entry->needs_copy &&
		    ((fault_type & VM_PROT_WRITE) ||
		    force_copy)) {
			if (!mapped_needs_copy) {
				if (vm_map_lock_read_to_write(map)) {
					vm_map_lock_read(map);
					*real_map = map;
					goto RetryLookup;
				}
				vm_map_lock_read(VME_SUBMAP(entry));
				*var_map = VME_SUBMAP(entry);
				cow_sub_map_parent = map;
				                                         
				                                         
				                        
				old_start = entry->vme_start;
				old_end = entry->vme_end;
				cow_parent_vaddr = vaddr;
				mapped_needs_copy = TRUE;
			} else {
				vm_map_lock_read(VME_SUBMAP(entry));
				*var_map = VME_SUBMAP(entry);
				if ((cow_sub_map_parent != map) &&
				    (*real_map != map)) {
					vm_map_unlock(map);
				}
			}
		} else {
			if (entry->needs_copy) {
				submap_needed_copy = TRUE;
			}
			vm_map_lock_read(VME_SUBMAP(entry));
			*var_map = VME_SUBMAP(entry);
			                                        
			                                        
			                                        
			                                        
			                          
			if ((*real_map != map) && (map != cow_sub_map_parent)) {
				vm_map_unlock_read(map);
			}
		}

		map = *var_map;

		                                                  
		local_vaddr = (local_vaddr - entry->vme_start) + VME_OFFSET(entry);
		assertf(VM_MAP_PAGE_ALIGNED(local_vaddr, fault_page_mask),
		    "local_vaddr 0x%llx entry->vme_start 0x%llx fault_page_mask 0x%llx\n",
		    (uint64_t)local_vaddr, (uint64_t)entry->vme_start, (uint64_t)fault_page_mask);

RetrySubMap:
		if (!vm_map_lookup_entry(map, local_vaddr, &submap_entry)) {
			if ((cow_sub_map_parent) && (cow_sub_map_parent != map)) {
				vm_map_unlock(cow_sub_map_parent);
			}
			if ((*real_map != map)
			    && (*real_map != cow_sub_map_parent)) {
				vm_map_unlock(*real_map);
			}
			*real_map = map;
			return KERN_INVALID_ADDRESS;
		}

		                                                         
		                       

		                                                            
		                                                            
		                                                            
		                                                            
		                                                            
		                                                            
		                     
		start_delta = submap_entry->vme_start > VME_OFFSET(entry) ?
		    submap_entry->vme_start - VME_OFFSET(entry) : 0;

		end_delta =
		    (VME_OFFSET(entry) + start_delta + (old_end - old_start)) <=
		    submap_entry->vme_end ?
		    0 : (VME_OFFSET(entry) +
		    (old_end - old_start))
		    - submap_entry->vme_end;

		old_start += start_delta;
		old_end -= end_delta;

		if (submap_entry->is_sub_map) {
			entry = submap_entry;
			vaddr = local_vaddr;
			goto submap_recurse;
		}

		if (((fault_type & VM_PROT_WRITE) ||
		    force_copy)
		    && cow_sub_map_parent) {
			vm_object_t     sub_object, copy_object;
			vm_object_offset_t copy_offset;
			vm_map_offset_t local_start;
			vm_map_offset_t local_end;
			boolean_t       object_copied = FALSE;
			vm_object_offset_t object_copied_offset = 0;
			boolean_t       object_copied_needs_copy = FALSE;
			kern_return_t   kr = KERN_SUCCESS;

			if (vm_map_lock_read_to_write(map)) {
				vm_map_lock_read(map);
				old_start -= start_delta;
				old_end += end_delta;
				goto RetrySubMap;
			}


			sub_object = VME_OBJECT(submap_entry);
			if (sub_object == VM_OBJECT_NULL) {
				sub_object =
				    vm_object_allocate(
					(vm_map_size_t)
					(submap_entry->vme_end -
					submap_entry->vme_start));
				VME_OBJECT_SET(submap_entry, sub_object);
				VME_OFFSET_SET(submap_entry, 0);
				assert(!submap_entry->is_sub_map);
				assert(submap_entry->use_pmap);
			}
			local_start =  local_vaddr -
			    (cow_parent_vaddr - old_start);
			local_end = local_vaddr +
			    (old_end - cow_parent_vaddr);
			vm_map_clip_start(map, submap_entry, local_start);
			vm_map_clip_end(map, submap_entry, local_end);
			if (submap_entry->is_sub_map) {
				                                      
				assert(!submap_entry->use_pmap);
			}

			                                        
			                                             
			                                          
			             
			submap_entry_offset = VME_OFFSET(submap_entry);
			submap_entry_size = submap_entry->vme_end - submap_entry->vme_start;

			if ((submap_entry->wired_count != 0 ||
			    sub_object->copy_strategy != MEMORY_OBJECT_COPY_SYMMETRIC) &&
			    (submap_entry->protection & VM_PROT_EXECUTE) &&
			    no_force_copy_if_executable) {
                                                                                                                                                                                                      
				if ((cow_sub_map_parent) && (cow_sub_map_parent != map)) {
					vm_map_unlock(cow_sub_map_parent);
				}
				if ((*real_map != map)
				    && (*real_map != cow_sub_map_parent)) {
					vm_map_unlock(*real_map);
				}
				*real_map = map;
				vm_map_lock_write_to_read(map);
				kr = KERN_PROTECTION_FAILURE;
				DTRACE_VM4(submap_no_copy_executable,
				    vm_map_t, map,
				    vm_object_offset_t, submap_entry_offset,
				    vm_object_size_t, submap_entry_size,
				    int, kr);
				return kr;
			}

			if (submap_entry->wired_count != 0) {
				vm_object_reference(sub_object);

				assertf(VM_MAP_PAGE_ALIGNED(VME_OFFSET(submap_entry), VM_MAP_PAGE_MASK(map)),
				    "submap_entry %p offset 0x%llx\n",
				    submap_entry, VME_OFFSET(submap_entry));

				DTRACE_VM6(submap_copy_slowly,
				    vm_map_t, cow_sub_map_parent,
				    vm_map_offset_t, vaddr,
				    vm_map_t, map,
				    vm_object_size_t, submap_entry_size,
				    int, submap_entry->wired_count,
				    int, sub_object->copy_strategy);

				saved_submap_entry = submap_entry;
				version.main_timestamp = map->timestamp;
				vm_map_unlock(map);                                
				submap_entry = VM_MAP_ENTRY_NULL;

				vm_object_lock(sub_object);
				kr = vm_object_copy_slowly(sub_object,
				    submap_entry_offset,
				    submap_entry_size,
				    FALSE,
				    &copy_object);
				object_copied = TRUE;
				object_copied_offset = 0;
				                                                   
				object_copied_offset += submap_entry_offset - vm_object_trunc_page(submap_entry_offset);
				object_copied_needs_copy = FALSE;
				vm_object_deallocate(sub_object);

				vm_map_lock(map);

				if (kr != KERN_SUCCESS &&
				    kr != KERN_MEMORY_RESTART_COPY) {
					if ((cow_sub_map_parent) && (cow_sub_map_parent != map)) {
						vm_map_unlock(cow_sub_map_parent);
					}
					if ((*real_map != map)
					    && (*real_map != cow_sub_map_parent)) {
						vm_map_unlock(*real_map);
					}
					*real_map = map;
					vm_object_deallocate(copy_object);
					copy_object = VM_OBJECT_NULL;
					vm_map_lock_write_to_read(map);
					DTRACE_VM4(submap_copy_error_slowly,
					    vm_object_t, sub_object,
					    vm_object_offset_t, submap_entry_offset,
					    vm_object_size_t, submap_entry_size,
					    int, kr);
					vm_map_lookup_locked_copy_slowly_error++;
					return kr;
				}

				if ((kr == KERN_SUCCESS) &&
				    (version.main_timestamp + 1) == map->timestamp) {
					submap_entry = saved_submap_entry;
				} else {
					saved_submap_entry = NULL;
					old_start -= start_delta;
					old_end += end_delta;
					vm_object_deallocate(copy_object);
					copy_object = VM_OBJECT_NULL;
					vm_map_lock_write_to_read(map);
					vm_map_lookup_locked_copy_slowly_restart++;
					goto RetrySubMap;
				}
				vm_map_lookup_locked_copy_slowly_count++;
				vm_map_lookup_locked_copy_slowly_size += submap_entry_size;
				if (submap_entry_size > vm_map_lookup_locked_copy_slowly_max) {
					vm_map_lookup_locked_copy_slowly_max = submap_entry_size;
				}
			} else if (sub_object->copy_strategy != MEMORY_OBJECT_COPY_SYMMETRIC) {
				submap_entry_offset = VME_OFFSET(submap_entry);
				copy_object = VM_OBJECT_NULL;
				object_copied_offset = submap_entry_offset;
				object_copied_needs_copy = FALSE;
				DTRACE_VM6(submap_copy_strategically,
				    vm_map_t, cow_sub_map_parent,
				    vm_map_offset_t, vaddr,
				    vm_map_t, map,
				    vm_object_size_t, submap_entry_size,
				    int, submap_entry->wired_count,
				    int, sub_object->copy_strategy);
				kr = vm_object_copy_strategically(
					sub_object,
					submap_entry_offset,
					submap_entry->vme_end - submap_entry->vme_start,
					&copy_object,
					&object_copied_offset,
					&object_copied_needs_copy);
				if (kr == KERN_MEMORY_RESTART_COPY) {
					old_start -= start_delta;
					old_end += end_delta;
					vm_object_deallocate(copy_object);
					copy_object = VM_OBJECT_NULL;
					vm_map_lock_write_to_read(map);
					vm_map_lookup_locked_copy_strategically_restart++;
					goto RetrySubMap;
				}
				if (kr != KERN_SUCCESS) {
					if ((cow_sub_map_parent) && (cow_sub_map_parent != map)) {
						vm_map_unlock(cow_sub_map_parent);
					}
					if ((*real_map != map)
					    && (*real_map != cow_sub_map_parent)) {
						vm_map_unlock(*real_map);
					}
					*real_map = map;
					vm_object_deallocate(copy_object);
					copy_object = VM_OBJECT_NULL;
					vm_map_lock_write_to_read(map);
					DTRACE_VM4(submap_copy_error_strategically,
					    vm_object_t, sub_object,
					    vm_object_offset_t, submap_entry_offset,
					    vm_object_size_t, submap_entry_size,
					    int, kr);
					vm_map_lookup_locked_copy_strategically_error++;
					return kr;
				}
				assert(copy_object != VM_OBJECT_NULL);
				assert(copy_object != sub_object);
				object_copied = TRUE;
				vm_map_lookup_locked_copy_strategically_count++;
				vm_map_lookup_locked_copy_strategically_size += submap_entry_size;
				if (submap_entry_size > vm_map_lookup_locked_copy_strategically_max) {
					vm_map_lookup_locked_copy_strategically_max = submap_entry_size;
				}
			} else {
				                          
				object_copied = FALSE;
				copy_object = sub_object;
				vm_object_lock(sub_object);
				vm_object_reference_locked(sub_object);
				sub_object->shadowed = TRUE;
				vm_object_unlock(sub_object);

				assert(submap_entry->wired_count == 0);
				submap_entry->needs_copy = TRUE;

				prot = submap_entry->protection;
				assert(!pmap_has_prot_policy(map->pmap, submap_entry->translated_allow_execute, prot));
				prot = prot & ~VM_PROT_WRITE;
				assert(!pmap_has_prot_policy(map->pmap, submap_entry->translated_allow_execute, prot));

				if (override_nx(old_map,
				    VME_ALIAS(submap_entry))
				    && prot) {
					prot |= VM_PROT_EXECUTE;
				}

				vm_object_pmap_protect(
					sub_object,
					VME_OFFSET(submap_entry),
					submap_entry->vme_end -
					submap_entry->vme_start,
					(submap_entry->is_shared
					|| map->mapped_in_other_pmaps) ?
					PMAP_NULL : map->pmap,
					VM_MAP_PAGE_SIZE(map),
					submap_entry->vme_start,
					prot);
				vm_map_lookup_locked_copy_shadow_count++;
				vm_map_lookup_locked_copy_shadow_size += submap_entry_size;
				if (submap_entry_size > vm_map_lookup_locked_copy_shadow_max) {
					vm_map_lookup_locked_copy_shadow_max = submap_entry_size;
				}
			}

			  
                                                  
      
			copy_offset = (local_vaddr -
			    submap_entry->vme_start +
			    VME_OFFSET(submap_entry));

			                                     
			                                     
			                                     
			                                     
			                                     
			                                     

			subentry_protection = submap_entry->protection;
			subentry_max_protection = submap_entry->max_protection;
			subentry_no_copy_on_read = submap_entry->vme_no_copy_on_read;
			vm_map_unlock(map);
			submap_entry = NULL;                                 

			local_start = old_start;
			local_end = old_end;
			map = cow_sub_map_parent;
			*var_map = cow_sub_map_parent;
			vaddr = cow_parent_vaddr;
			cow_sub_map_parent = NULL;

			if (!vm_map_lookup_entry(map,
			    vaddr, &entry)) {
				if ((cow_sub_map_parent) && (cow_sub_map_parent != map)) {
					vm_map_unlock(cow_sub_map_parent);
				}
				if ((*real_map != map)
				    && (*real_map != cow_sub_map_parent)) {
					vm_map_unlock(*real_map);
				}
				*real_map = map;
				vm_object_deallocate(
					copy_object);
				copy_object = VM_OBJECT_NULL;
				vm_map_lock_write_to_read(map);
				DTRACE_VM4(submap_lookup_post_unlock,
				    uint64_t, (uint64_t)entry->vme_start,
				    uint64_t, (uint64_t)entry->vme_end,
				    vm_map_offset_t, vaddr,
				    int, object_copied);
				return KERN_INVALID_ADDRESS;
			}

			                                   
			                                   
			                                   
			            

			  
                                                 
                                             
      
			local_start = vaddr & ~(pmap_shared_region_size_min(map->pmap) - 1);
			local_end = local_start + pmap_shared_region_size_min(map->pmap);
			  
                                                          
                                                     
                                                        
      
			if (local_start < old_start) {
				local_start = old_start;
			}
			if (local_end > old_end) {
				local_end = old_end;
			}
			  
                                                   
      
			copy_offset -= (vaddr - local_start);

			vm_map_clip_start(map, entry, local_start);
			vm_map_clip_end(map, entry, local_end);
			if (entry->is_sub_map) {
				                                      
				assert(!entry->use_pmap);
			}

			                                
			                                
			vm_map_deallocate(VME_SUBMAP(entry));
			assert(!entry->iokit_acct);
			entry->is_sub_map = FALSE;
			entry->use_pmap = TRUE;
			VME_OBJECT_SET(entry, copy_object);

			                                              
			if (entry->protection != VM_PROT_READ) {
				  
                                                  
                                                
                                              
                                  
       
			} else {
				  
                                                  
                                                
       
				entry->protection |= subentry_protection;
			}
			entry->max_protection |= subentry_max_protection;
			                               
			entry->vme_no_copy_on_read = subentry_no_copy_on_read;

			if ((entry->protection & VM_PROT_WRITE) &&
			    (entry->protection & VM_PROT_EXECUTE) &&
#if XNU_TARGET_OS_OSX
			    map->pmap != kernel_pmap &&
			    (vm_map_cs_enforcement(map)
#if __arm64__
			    || !VM_MAP_IS_EXOTIC(map)
#endif                
			    ) &&
#endif                        
			    !(entry->used_for_jit) &&
			    VM_MAP_POLICY_WX_STRIP_X(map)) {
				DTRACE_VM3(cs_wx,
				    uint64_t, (uint64_t)entry->vme_start,
				    uint64_t, (uint64_t)entry->vme_end,
				    vm_prot_t, entry->protection);
				printf("CODE SIGNING: %d[%s] %s can't have both write and exec at the same time\n",
				    proc_selfpid(),
				    (current_task()->bsd_info
				    ? proc_name_address(current_task()->bsd_info)
				    : "?"),
				    __FUNCTION__);
				entry->protection &= ~VM_PROT_EXECUTE;
			}

			if (object_copied) {
				VME_OFFSET_SET(entry, local_start - old_start + object_copied_offset);
				entry->needs_copy = object_copied_needs_copy;
				entry->is_shared = FALSE;
			} else {
				assert(VME_OBJECT(entry) != VM_OBJECT_NULL);
				assert(VME_OBJECT(entry)->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC);
				assert(entry->wired_count == 0);
				VME_OFFSET_SET(entry, copy_offset);
				entry->needs_copy = TRUE;
				if (map != old_map) {
					entry->is_shared = TRUE;
				}
			}
			if (entry->inheritance == VM_INHERIT_SHARE) {
				entry->inheritance = VM_INHERIT_COPY;
			}

			vm_map_lock_write_to_read(map);
		} else {
			if ((cow_sub_map_parent)
			    && (cow_sub_map_parent != *real_map)
			    && (cow_sub_map_parent != map)) {
				vm_map_unlock(cow_sub_map_parent);
			}
			entry = submap_entry;
			vaddr = local_vaddr;
		}
	}

	  
                                              
              
    

	prot = entry->protection;

	if (override_nx(old_map, VME_ALIAS(entry)) && prot) {
		  
                                                 
     
		prot |= VM_PROT_EXECUTE;
	}

	if (mask_protections) {
		fault_type &= prot;
		if (fault_type == VM_PROT_NONE) {
			goto protection_failure;
		}
	}
	if (((fault_type & prot) != fault_type)
#if __arm64__
	                                             
	    && !(prot == VM_PROT_EXECUTE && fault_type == (VM_PROT_READ | VM_PROT_EXECUTE))
#endif
	    ) {
protection_failure:
		if (*real_map != map) {
			vm_map_unlock(*real_map);
		}
		*real_map = map;

		if ((fault_type & VM_PROT_EXECUTE) && prot) {
			log_stack_execution_failure((addr64_t)vaddr, prot);
		}

		DTRACE_VM2(prot_fault, int, 1, (uint64_t *), NULL);
		return KERN_PROTECTION_FAILURE;
	}

	  
                                                
                                 
    

	*wired = (entry->wired_count != 0);
	if (*wired) {
		fault_type = prot;
	}

	  
                                                 
    

	if (entry->needs_copy) {
		  
                                                 
                                                    
    
                                                
                                    
     

		if ((fault_type & VM_PROT_WRITE) || *wired || force_copy) {
			  
                                            
                                                
                                              
                            
      

			if (vm_map_lock_read_to_write(map)) {
				vm_map_lock_read(map);
				goto RetryLookup;
			}

			if (VME_OBJECT(entry)->shadowed == FALSE) {
				vm_object_lock(VME_OBJECT(entry));
				VME_OBJECT(entry)->shadowed = TRUE;
				vm_object_unlock(VME_OBJECT(entry));
			}
			VME_OBJECT_SHADOW(entry,
			    (vm_map_size_t) (entry->vme_end -
			    entry->vme_start));
			entry->needs_copy = FALSE;

			vm_map_lock_write_to_read(map);
		}
		if ((fault_type & VM_PROT_WRITE) == 0 && *wired == 0) {
			  
                                              
                                 
      

			prot &= (~VM_PROT_WRITE);
		}
	}

	if (submap_needed_copy && (prot & VM_PROT_WRITE)) {
		  
                                                             
                                                              
                                
     
		assert(!(fault_type & VM_PROT_WRITE));
		assert(!*wired);
		assert(!force_copy);
		                                                                                                                                     
		prot &= ~VM_PROT_WRITE;
	}

	  
                                  
    
	if (VME_OBJECT(entry) == VM_OBJECT_NULL) {
		if (vm_map_lock_read_to_write(map)) {
			vm_map_lock_read(map);
			goto RetryLookup;
		}

		VME_OBJECT_SET(entry,
		    vm_object_allocate(
			    (vm_map_size_t)(entry->vme_end -
			    entry->vme_start)));
		VME_OFFSET_SET(entry, 0);
		assert(entry->use_pmap);
		vm_map_lock_write_to_read(map);
	}

	  
                                                           
                                                           
                          
    

	*offset = (vaddr - entry->vme_start) + VME_OFFSET(entry);
	*object = VME_OBJECT(entry);
	*out_prot = prot;
	KDBG_FILTERED(MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_MAP_LOOKUP_OBJECT), VM_KERNEL_UNSLIDE_OR_PERM(*object), (unsigned long) VME_ALIAS(entry), 0, 0);

	if (fault_info) {
		fault_info->interruptible = THREAD_UNINT;                 
		                                                          
		fault_info->cluster_size = 0;
		fault_info->user_tag = VME_ALIAS(entry);
		fault_info->pmap_options = 0;
		if (entry->iokit_acct ||
		    (!entry->is_sub_map && !entry->use_pmap)) {
			fault_info->pmap_options |= PMAP_OPTIONS_ALT_ACCT;
		}
		fault_info->behavior = entry->behavior;
		fault_info->lo_offset = VME_OFFSET(entry);
		fault_info->hi_offset =
		    (entry->vme_end - entry->vme_start) + VME_OFFSET(entry);
		fault_info->no_cache  = entry->no_cache;
		fault_info->stealth = FALSE;
		fault_info->io_sync = FALSE;
		if (entry->used_for_jit ||
		    entry->vme_resilient_codesign) {
			fault_info->cs_bypass = TRUE;
		} else {
			fault_info->cs_bypass = FALSE;
		}
		fault_info->pmap_cs_associated = FALSE;
#if CONFIG_PMAP_CS
		if (entry->pmap_cs_associated) {
			  
                                            
                                             
      
			fault_info->pmap_cs_associated = TRUE;
		}
#endif                     
		fault_info->mark_zf_absent = FALSE;
		fault_info->batch_pmap_op = FALSE;
		fault_info->resilient_media = entry->vme_resilient_media;
		fault_info->no_copy_on_read = entry->vme_no_copy_on_read;
		if (entry->translated_allow_execute) {
			fault_info->pmap_options |= PMAP_OPTIONS_TRANSLATED_ALLOW_EXECUTE;
		}
	}

	  
                                                   
    
	if (object_lock_type == OBJECT_LOCK_EXCLUSIVE) {
		if (contended == NULL) {
			vm_object_lock(*object);
		} else {
			*contended = vm_object_lock_check_contended(*object);
		}
	} else {
		vm_object_lock_shared(*object);
	}

	  
                           
    

	out_version->main_timestamp = map->timestamp;

	return KERN_SUCCESS;
}


  
                 
  
                                                    
                                                    
                                                       
                                      
   
boolean_t
vm_map_verify(
	vm_map_t                map,
	vm_map_version_t        *version)                
{
	boolean_t       result;

	vm_map_lock_assert_held(map);
	result = (map->timestamp == version->main_timestamp);

	return result;
}

  
                                                         
                                                                 
          
                                                           
                          
  
   

kern_return_t
vm_map_region_recurse_64(
	vm_map_t                 map,
	vm_map_offset_t *address,                           
	vm_map_size_t           *size,                           
	natural_t               *nesting_depth,             
	vm_region_submap_info_64_t      submap_info,                
	mach_msg_type_number_t  *count)             
{
	mach_msg_type_number_t  original_count;
	vm_region_extended_info_data_t  extended;
	vm_map_entry_t                  tmp_entry;
	vm_map_offset_t                 user_address;
	unsigned int                    user_max_depth;

	  
                                                               
                              
                                                             
                                                                    
                       
                                                              
                                
                                                           
             
   
                                                                 
                                                                        
                                                                  
                                                        
   
    
	vm_map_entry_t                  curr_entry;
	vm_map_address_t                curr_address;
	vm_map_offset_t                 curr_offset;
	vm_map_t                        curr_map;
	unsigned int                    curr_depth;
	vm_map_offset_t                 curr_max_below, curr_max_above;
	vm_map_offset_t                 curr_skip;

	  
                                                                    
                                                                       
                                                               
                                            
    
	vm_map_entry_t                  next_entry;
	vm_map_offset_t                 next_offset;
	vm_map_offset_t                 next_address;
	vm_map_t                        next_map;
	unsigned int                    next_depth;
	vm_map_offset_t                 next_max_below, next_max_above;
	vm_map_offset_t                 next_skip;

	boolean_t                       look_for_pages;
	vm_region_submap_short_info_64_t short_info;
	boolean_t                       do_region_footprint;
	int                             effective_page_size, effective_page_shift;
	boolean_t                       submap_needed_copy;

	if (map == VM_MAP_NULL) {
		                                 
		return KERN_INVALID_ARGUMENT;
	}

	effective_page_shift = vm_self_region_page_shift(map);
	effective_page_size = (1 << effective_page_shift);

	if (*count < VM_REGION_SUBMAP_SHORT_INFO_COUNT_64) {
		  
                                           
                   
     
		return KERN_INVALID_ARGUMENT;
	}

	do_region_footprint = task_self_region_footprint();
	original_count = *count;

	if (original_count < VM_REGION_SUBMAP_INFO_V0_COUNT_64) {
		*count = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;
		look_for_pages = FALSE;
		short_info = (vm_region_submap_short_info_64_t) submap_info;
		submap_info = NULL;
	} else {
		look_for_pages = TRUE;
		*count = VM_REGION_SUBMAP_INFO_V0_COUNT_64;
		short_info = NULL;

		if (original_count >= VM_REGION_SUBMAP_INFO_V1_COUNT_64) {
			*count = VM_REGION_SUBMAP_INFO_V1_COUNT_64;
		}
		if (original_count >= VM_REGION_SUBMAP_INFO_V2_COUNT_64) {
			*count = VM_REGION_SUBMAP_INFO_V2_COUNT_64;
		}
	}

	user_address = *address;
	user_max_depth = *nesting_depth;
	submap_needed_copy = FALSE;

	if (not_in_kdp) {
		vm_map_lock_read(map);
	}

recurse_again:
	curr_entry = NULL;
	curr_map = map;
	curr_address = user_address;
	curr_offset = 0;
	curr_skip = 0;
	curr_depth = 0;
	curr_max_above = ((vm_map_offset_t) -1) - curr_address;
	curr_max_below = curr_address;

	next_entry = NULL;
	next_map = NULL;
	next_address = 0;
	next_offset = 0;
	next_skip = 0;
	next_depth = 0;
	next_max_above = (vm_map_offset_t) -1;
	next_max_below = (vm_map_offset_t) -1;

	for (;;) {
		if (vm_map_lookup_entry(curr_map,
		    curr_address,
		    &tmp_entry)) {
			                                                      
			curr_entry = tmp_entry;
		} else {
			vm_map_offset_t skip;
			  
                                                    
                                                        
                        
      
			curr_entry = tmp_entry->vme_next;

			if (curr_entry == vm_map_to_entry(curr_map) ||
			    (curr_entry->vme_start >=
			    curr_address + curr_max_above)) {
				                                               
				if (not_in_kdp) {
					vm_map_unlock_read(curr_map);
				}
				curr_entry = NULL;
				curr_map = NULL;
				curr_skip = 0;
				curr_offset = 0;
				curr_depth = 0;
				curr_max_above = 0;
				curr_max_below = 0;
				break;
			}

			                                       
			skip = curr_entry->vme_start - curr_address;
			curr_address = curr_entry->vme_start;
			curr_skip += skip;
			curr_offset += skip;
			curr_max_above -= skip;
			curr_max_below = 0;
		}

		  
                                                              
                                                    
             
     
		tmp_entry = curr_entry->vme_next;
		if (tmp_entry == vm_map_to_entry(curr_map)) {
			                                 
		} else if (tmp_entry->vme_start >=
		    curr_address + curr_max_above) {
			  
                                                        
                                                
      
		} else if ((next_entry == NULL) ||
		    (tmp_entry->vme_start + curr_offset <=
		    next_entry->vme_start + next_offset)) {
			  
                                                  
                                              
                                                   
      
			if (next_entry != NULL) {
				                                
				if (next_map != curr_map && not_in_kdp) {
					vm_map_unlock_read(next_map);
				}
			}
			next_entry = tmp_entry;
			next_map = curr_map;
			next_depth = curr_depth;
			next_address = next_entry->vme_start;
			next_skip = curr_skip;
			next_skip += (next_address - curr_address);
			next_offset = curr_offset;
			next_offset += (next_address - curr_address);
			next_max_above = MIN(next_max_above, curr_max_above);
			next_max_above = MIN(next_max_above,
			    next_entry->vme_end - next_address);
			next_max_below = MIN(next_max_below, curr_max_below);
			next_max_below = MIN(next_max_below,
			    next_address - next_entry->vme_start);
		}

		  
                                                           
                                                                 
                                                                
                 
                                              
                                                             
     
		curr_max_above = MIN(curr_max_above,
		    curr_entry->vme_end - curr_address);
		curr_max_below = MIN(curr_max_below,
		    curr_address - curr_entry->vme_start);

		if (!curr_entry->is_sub_map ||
		    curr_depth >= user_max_depth) {
			  
                                                       
                                                      
             
      
			break;
		}

		  
                                       
     

		if (curr_entry->needs_copy) {
			                                                        
			submap_needed_copy = TRUE;
		}

		  
                                                      
                                                                
           
     
		if (not_in_kdp) {
			vm_map_lock_read(VME_SUBMAP(curr_entry));
		}
		if (curr_map == next_map) {
			                                               
		} else {
			                      
			if (not_in_kdp) {
				vm_map_unlock_read(curr_map);
			}
		}

		  
                                                     
                                                       
                                                          
                         
                                                            
                                                             
                                       
     
		curr_offset +=
		    (VME_OFFSET(curr_entry) - curr_entry->vme_start);
		curr_address = user_address + curr_offset;
		                          
		curr_map = VME_SUBMAP(curr_entry);
		curr_depth++;
		curr_entry = NULL;
	}

                                                                            
                                               
                                        

	if (curr_entry == NULL) {
		                                          

		if (do_region_footprint &&                                
		    next_entry == NULL &&                                  
		                                                        
		    user_address <= vm_map_last_entry(map)->vme_end) {
			ledger_amount_t ledger_resident, ledger_compressed;

			  
                                             
                                                
                                                  
                                                        
                                 
      
			task_ledgers_footprint(map->pmap->ledger,
			    &ledger_resident,
			    &ledger_compressed);
			if (ledger_resident + ledger_compressed == 0) {
				                                         
				return KERN_INVALID_ADDRESS;
			}
			                                               
			if (look_for_pages) {
				submap_info->protection = VM_PROT_DEFAULT;
				submap_info->max_protection = VM_PROT_DEFAULT;
				submap_info->inheritance = VM_INHERIT_DEFAULT;
				submap_info->offset = 0;
				submap_info->user_tag = -1;
				submap_info->pages_resident = (unsigned int) (ledger_resident / effective_page_size);
				submap_info->pages_shared_now_private = 0;
				submap_info->pages_swapped_out = (unsigned int) (ledger_compressed / effective_page_size);
				submap_info->pages_dirtied = submap_info->pages_resident;
				submap_info->ref_count = 1;
				submap_info->shadow_depth = 0;
				submap_info->external_pager = 0;
				submap_info->share_mode = SM_PRIVATE;
				if (submap_needed_copy) {
					submap_info->share_mode = SM_COW;
				}
				submap_info->is_submap = 0;
				submap_info->behavior = VM_BEHAVIOR_DEFAULT;
				submap_info->object_id = VM_OBJECT_ID_FAKE(map, task_ledgers.purgeable_nonvolatile);
				submap_info->user_wired_count = 0;
				submap_info->pages_reusable = 0;
			} else {
				short_info->user_tag = -1;
				short_info->offset = 0;
				short_info->protection = VM_PROT_DEFAULT;
				short_info->inheritance = VM_INHERIT_DEFAULT;
				short_info->max_protection = VM_PROT_DEFAULT;
				short_info->behavior = VM_BEHAVIOR_DEFAULT;
				short_info->user_wired_count = 0;
				short_info->is_submap = 0;
				short_info->object_id = VM_OBJECT_ID_FAKE(map, task_ledgers.purgeable_nonvolatile);
				short_info->external_pager = 0;
				short_info->shadow_depth = 0;
				short_info->share_mode = SM_PRIVATE;
				if (submap_needed_copy) {
					short_info->share_mode = SM_COW;
				}
				short_info->ref_count = 1;
			}
			*nesting_depth = 0;
			*size = (vm_map_size_t) (ledger_resident + ledger_compressed);
                             
			*address = vm_map_last_entry(map)->vme_end;
			return KERN_SUCCESS;
		}

		if (next_entry == NULL) {
			                                            
			return KERN_INVALID_ADDRESS;
		}
		                                              
		curr_entry = next_entry;
		curr_map = next_map;                          
		curr_address = next_address;
		curr_skip = next_skip;
		curr_offset = next_offset;
		curr_depth = next_depth;
		curr_max_above = next_max_above;
		curr_max_below = next_max_below;
	} else {
		                                          
		if (next_entry != NULL) {
			                        
			if (next_map != curr_map && not_in_kdp) {
				vm_map_unlock_read(next_map);
			}
		}
	}
	next_entry = NULL;
	next_map = NULL;
	next_offset = 0;
	next_skip = 0;
	next_depth = 0;
	next_max_below = -1;
	next_max_above = -1;

	if (curr_entry->is_sub_map &&
	    curr_depth < user_max_depth) {
		  
                                                    
                                                   
                                              
                                                         
     
		user_address = curr_address;
		goto recurse_again;
	}

	*nesting_depth = curr_depth;
	*size = curr_max_above + curr_max_below;
	*address = user_address + curr_skip - curr_max_below;

	if (look_for_pages) {
		submap_info->user_tag = VME_ALIAS(curr_entry);
		submap_info->offset = VME_OFFSET(curr_entry);
		submap_info->protection = curr_entry->protection;
		submap_info->inheritance = curr_entry->inheritance;
		submap_info->max_protection = curr_entry->max_protection;
		submap_info->behavior = curr_entry->behavior;
		submap_info->user_wired_count = curr_entry->user_wired_count;
		submap_info->is_submap = curr_entry->is_sub_map;
		submap_info->object_id = VM_OBJECT_ID(VME_OBJECT(curr_entry));
	} else {
		short_info->user_tag = VME_ALIAS(curr_entry);
		short_info->offset = VME_OFFSET(curr_entry);
		short_info->protection = curr_entry->protection;
		short_info->inheritance = curr_entry->inheritance;
		short_info->max_protection = curr_entry->max_protection;
		short_info->behavior = curr_entry->behavior;
		short_info->user_wired_count = curr_entry->user_wired_count;
		short_info->is_submap = curr_entry->is_sub_map;
		short_info->object_id = VM_OBJECT_ID(VME_OBJECT(curr_entry));
	}

	extended.pages_resident = 0;
	extended.pages_swapped_out = 0;
	extended.pages_shared_now_private = 0;
	extended.pages_dirtied = 0;
	extended.pages_reusable = 0;
	extended.external_pager = 0;
	extended.shadow_depth = 0;
	extended.share_mode = SM_EMPTY;
	extended.ref_count = 0;

	if (not_in_kdp) {
		if (!curr_entry->is_sub_map) {
			vm_map_offset_t range_start, range_end;
			range_start = MAX((curr_address - curr_max_below),
			    curr_entry->vme_start);
			range_end = MIN((curr_address + curr_max_above),
			    curr_entry->vme_end);
			vm_map_region_walk(curr_map,
			    range_start,
			    curr_entry,
			    (VME_OFFSET(curr_entry) +
			    (range_start -
			    curr_entry->vme_start)),
			    range_end - range_start,
			    &extended,
			    look_for_pages, VM_REGION_EXTENDED_INFO_COUNT);
			if (extended.external_pager &&
			    extended.ref_count == 2 &&
			    extended.share_mode == SM_SHARED) {
				extended.share_mode = SM_PRIVATE;
			}
			if (submap_needed_copy) {
				extended.share_mode = SM_COW;
			}
		} else {
			if (curr_entry->use_pmap) {
				extended.share_mode = SM_TRUESHARED;
			} else {
				extended.share_mode = SM_PRIVATE;
			}
			extended.ref_count = os_ref_get_count(&VME_SUBMAP(curr_entry)->map_refcnt);
		}
	}

	if (look_for_pages) {
		submap_info->pages_resident = extended.pages_resident;
		submap_info->pages_swapped_out = extended.pages_swapped_out;
		submap_info->pages_shared_now_private =
		    extended.pages_shared_now_private;
		submap_info->pages_dirtied = extended.pages_dirtied;
		submap_info->external_pager = extended.external_pager;
		submap_info->shadow_depth = extended.shadow_depth;
		submap_info->share_mode = extended.share_mode;
		submap_info->ref_count = extended.ref_count;

		if (original_count >= VM_REGION_SUBMAP_INFO_V1_COUNT_64) {
			submap_info->pages_reusable = extended.pages_reusable;
		}
		if (original_count >= VM_REGION_SUBMAP_INFO_V2_COUNT_64) {
			submap_info->object_id_full = (vm_object_id_t) (VME_OBJECT(curr_entry) != NULL) ? VM_KERNEL_ADDRPERM(VME_OBJECT(curr_entry)) : 0ULL;
		}
	} else {
		short_info->external_pager = extended.external_pager;
		short_info->shadow_depth = extended.shadow_depth;
		short_info->share_mode = extended.share_mode;
		short_info->ref_count = extended.ref_count;
	}

	if (not_in_kdp) {
		vm_map_unlock_read(curr_map);
	}

	return KERN_SUCCESS;
}

  
             
  
                                                    
                                                      
             
  
                                                        
                                                          
                                 
   

kern_return_t
vm_map_region(
	vm_map_t                 map,
	vm_map_offset_t *address,                           
	vm_map_size_t           *size,                           
	vm_region_flavor_t       flavor,                        
	vm_region_info_t         info,                           
	mach_msg_type_number_t  *count,             
	mach_port_t             *object_name)                    
{
	vm_map_entry_t          tmp_entry;
	vm_map_entry_t          entry;
	vm_map_offset_t         start;

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	switch (flavor) {
	case VM_REGION_BASIC_INFO:
		                                        
	{
		vm_region_basic_info_t  basic;

		if (*count < VM_REGION_BASIC_INFO_COUNT) {
			return KERN_INVALID_ARGUMENT;
		}

		basic = (vm_region_basic_info_t) info;
		*count = VM_REGION_BASIC_INFO_COUNT;

		vm_map_lock_read(map);

		start = *address;
		if (!vm_map_lookup_entry(map, start, &tmp_entry)) {
			if ((entry = tmp_entry->vme_next) == vm_map_to_entry(map)) {
				vm_map_unlock_read(map);
				return KERN_INVALID_ADDRESS;
			}
		} else {
			entry = tmp_entry;
		}

		start = entry->vme_start;

		basic->offset = (uint32_t)VME_OFFSET(entry);
		basic->protection = entry->protection;
		basic->inheritance = entry->inheritance;
		basic->max_protection = entry->max_protection;
		basic->behavior = entry->behavior;
		basic->user_wired_count = entry->user_wired_count;
		basic->reserved = entry->is_sub_map;
		*address = start;
		*size = (entry->vme_end - start);

		if (object_name) {
			*object_name = IP_NULL;
		}
		if (entry->is_sub_map) {
			basic->shared = FALSE;
		} else {
			basic->shared = entry->is_shared;
		}

		vm_map_unlock_read(map);
		return KERN_SUCCESS;
	}

	case VM_REGION_BASIC_INFO_64:
	{
		vm_region_basic_info_64_t       basic;

		if (*count < VM_REGION_BASIC_INFO_COUNT_64) {
			return KERN_INVALID_ARGUMENT;
		}

		basic = (vm_region_basic_info_64_t) info;
		*count = VM_REGION_BASIC_INFO_COUNT_64;

		vm_map_lock_read(map);

		start = *address;
		if (!vm_map_lookup_entry(map, start, &tmp_entry)) {
			if ((entry = tmp_entry->vme_next) == vm_map_to_entry(map)) {
				vm_map_unlock_read(map);
				return KERN_INVALID_ADDRESS;
			}
		} else {
			entry = tmp_entry;
		}

		start = entry->vme_start;

		basic->offset = VME_OFFSET(entry);
		basic->protection = entry->protection;
		basic->inheritance = entry->inheritance;
		basic->max_protection = entry->max_protection;
		basic->behavior = entry->behavior;
		basic->user_wired_count = entry->user_wired_count;
		basic->reserved = entry->is_sub_map;
		*address = start;
		*size = (entry->vme_end - start);

		if (object_name) {
			*object_name = IP_NULL;
		}
		if (entry->is_sub_map) {
			basic->shared = FALSE;
		} else {
			basic->shared = entry->is_shared;
		}

		vm_map_unlock_read(map);
		return KERN_SUCCESS;
	}
	case VM_REGION_EXTENDED_INFO:
		if (*count < VM_REGION_EXTENDED_INFO_COUNT) {
			return KERN_INVALID_ARGUMENT;
		}
		OS_FALLTHROUGH;
	case VM_REGION_EXTENDED_INFO__legacy:
		if (*count < VM_REGION_EXTENDED_INFO_COUNT__legacy) {
			return KERN_INVALID_ARGUMENT;
		}

		{
			vm_region_extended_info_t       extended;
			mach_msg_type_number_t original_count;
			int effective_page_size, effective_page_shift;

			extended = (vm_region_extended_info_t) info;

			effective_page_shift = vm_self_region_page_shift(map);
			effective_page_size = (1 << effective_page_shift);

			vm_map_lock_read(map);

			start = *address;
			if (!vm_map_lookup_entry(map, start, &tmp_entry)) {
				if ((entry = tmp_entry->vme_next) == vm_map_to_entry(map)) {
					vm_map_unlock_read(map);
					return KERN_INVALID_ADDRESS;
				}
			} else {
				entry = tmp_entry;
			}
			start = entry->vme_start;

			extended->protection = entry->protection;
			extended->user_tag = VME_ALIAS(entry);
			extended->pages_resident = 0;
			extended->pages_swapped_out = 0;
			extended->pages_shared_now_private = 0;
			extended->pages_dirtied = 0;
			extended->external_pager = 0;
			extended->shadow_depth = 0;

			original_count = *count;
			if (flavor == VM_REGION_EXTENDED_INFO__legacy) {
				*count = VM_REGION_EXTENDED_INFO_COUNT__legacy;
			} else {
				extended->pages_reusable = 0;
				*count = VM_REGION_EXTENDED_INFO_COUNT;
			}

			vm_map_region_walk(map, start, entry, VME_OFFSET(entry), entry->vme_end - start, extended, TRUE, *count);

			if (extended->external_pager && extended->ref_count == 2 && extended->share_mode == SM_SHARED) {
				extended->share_mode = SM_PRIVATE;
			}

			if (object_name) {
				*object_name = IP_NULL;
			}
			*address = start;
			*size = (entry->vme_end - start);

			vm_map_unlock_read(map);
			return KERN_SUCCESS;
		}
	case VM_REGION_TOP_INFO:
	{
		vm_region_top_info_t    top;

		if (*count < VM_REGION_TOP_INFO_COUNT) {
			return KERN_INVALID_ARGUMENT;
		}

		top = (vm_region_top_info_t) info;
		*count = VM_REGION_TOP_INFO_COUNT;

		vm_map_lock_read(map);

		start = *address;
		if (!vm_map_lookup_entry(map, start, &tmp_entry)) {
			if ((entry = tmp_entry->vme_next) == vm_map_to_entry(map)) {
				vm_map_unlock_read(map);
				return KERN_INVALID_ADDRESS;
			}
		} else {
			entry = tmp_entry;
		}
		start = entry->vme_start;

		top->private_pages_resident = 0;
		top->shared_pages_resident = 0;

		vm_map_region_top_walk(entry, top);

		if (object_name) {
			*object_name = IP_NULL;
		}
		*address = start;
		*size = (entry->vme_end - start);

		vm_map_unlock_read(map);
		return KERN_SUCCESS;
	}
	default:
		return KERN_INVALID_ARGUMENT;
	}
}

#define OBJ_RESIDENT_COUNT(obj, entry_size)                             \
	MIN((entry_size),                                               \
	    ((obj)->all_reusable ?                                      \
	     (obj)->wired_page_count :                                  \
	     (obj)->resident_page_count - (obj)->reusable_page_count))

void
vm_map_region_top_walk(
	vm_map_entry_t             entry,
	vm_region_top_info_t       top)
{
	if (VME_OBJECT(entry) == 0 || entry->is_sub_map) {
		top->share_mode = SM_EMPTY;
		top->ref_count = 0;
		top->obj_id = 0;
		return;
	}

	{
		struct  vm_object *obj, *tmp_obj;
		int             ref_count;
		uint32_t        entry_size;

		entry_size = (uint32_t) ((entry->vme_end - entry->vme_start) / PAGE_SIZE_64);

		obj = VME_OBJECT(entry);

		vm_object_lock(obj);

		if ((ref_count = obj->ref_count) > 1 && obj->paging_in_progress) {
			ref_count--;
		}

		assert(obj->reusable_page_count <= obj->resident_page_count);
		if (obj->shadow) {
			if (ref_count == 1) {
				top->private_pages_resident =
				    OBJ_RESIDENT_COUNT(obj, entry_size);
			} else {
				top->shared_pages_resident =
				    OBJ_RESIDENT_COUNT(obj, entry_size);
			}
			top->ref_count  = ref_count;
			top->share_mode = SM_COW;

			while ((tmp_obj = obj->shadow)) {
				vm_object_lock(tmp_obj);
				vm_object_unlock(obj);
				obj = tmp_obj;

				if ((ref_count = obj->ref_count) > 1 && obj->paging_in_progress) {
					ref_count--;
				}

				assert(obj->reusable_page_count <= obj->resident_page_count);
				top->shared_pages_resident +=
				    OBJ_RESIDENT_COUNT(obj, entry_size);
				top->ref_count += ref_count - 1;
			}
		} else {
			if (entry->superpage_size) {
				top->share_mode = SM_LARGE_PAGE;
				top->shared_pages_resident = 0;
				top->private_pages_resident = entry_size;
			} else if (entry->needs_copy) {
				top->share_mode = SM_COW;
				top->shared_pages_resident =
				    OBJ_RESIDENT_COUNT(obj, entry_size);
			} else {
				if (ref_count == 1 ||
				    (ref_count == 2 && obj->named)) {
					top->share_mode = SM_PRIVATE;
					top->private_pages_resident =
					    OBJ_RESIDENT_COUNT(obj,
					    entry_size);
				} else {
					top->share_mode = SM_SHARED;
					top->shared_pages_resident =
					    OBJ_RESIDENT_COUNT(obj,
					    entry_size);
				}
			}
			top->ref_count = ref_count;
		}
		                                       
		top->obj_id = (unsigned int) (uintptr_t)VM_KERNEL_ADDRPERM(obj);

		vm_object_unlock(obj);
	}
}

void
vm_map_region_walk(
	vm_map_t                        map,
	vm_map_offset_t                 va,
	vm_map_entry_t                  entry,
	vm_object_offset_t              offset,
	vm_object_size_t                range,
	vm_region_extended_info_t       extended,
	boolean_t                       look_for_pages,
	mach_msg_type_number_t count)
{
	struct vm_object *obj, *tmp_obj;
	vm_map_offset_t       last_offset;
	int               i;
	int               ref_count;
	struct vm_object        *shadow_object;
	unsigned short          shadow_depth;
	boolean_t         do_region_footprint;
	int                     effective_page_size, effective_page_shift;
	vm_map_offset_t         effective_page_mask;

	do_region_footprint = task_self_region_footprint();

	if ((VME_OBJECT(entry) == 0) ||
	    (entry->is_sub_map) ||
	    (VME_OBJECT(entry)->phys_contiguous &&
	    !entry->superpage_size)) {
		extended->share_mode = SM_EMPTY;
		extended->ref_count = 0;
		return;
	}

	if (entry->superpage_size) {
		extended->shadow_depth = 0;
		extended->share_mode = SM_LARGE_PAGE;
		extended->ref_count = 1;
		extended->external_pager = 0;

		                                   
		extended->pages_resident = (unsigned int)(range >> PAGE_SHIFT);
		extended->shadow_depth = 0;
		return;
	}

	effective_page_shift = vm_self_region_page_shift(map);
	effective_page_size = (1 << effective_page_shift);
	effective_page_mask = effective_page_size - 1;

	offset = vm_map_trunc_page(offset, effective_page_mask);

	obj = VME_OBJECT(entry);

	vm_object_lock(obj);

	if ((ref_count = obj->ref_count) > 1 && obj->paging_in_progress) {
		ref_count--;
	}

	if (look_for_pages) {
		for (last_offset = offset + range;
		    offset < last_offset;
		    offset += effective_page_size, va += effective_page_size) {
			if (do_region_footprint) {
				int disp;

				disp = 0;
				if (map->has_corpse_footprint) {
					  
                                         
                                 
        
					vm_map_corpse_footprint_query_page_info(
						map,
						va,
						&disp);
				} else {
					  
                       
        
					vm_map_footprint_query_page_info(
						map,
						entry,
						va,
						&disp);
				}
				if (disp & VM_PAGE_QUERY_PAGE_PRESENT) {
					extended->pages_resident++;
				}
				if (disp & VM_PAGE_QUERY_PAGE_REUSABLE) {
					extended->pages_reusable++;
				}
				if (disp & VM_PAGE_QUERY_PAGE_DIRTY) {
					extended->pages_dirtied++;
				}
				if (disp & PMAP_QUERY_PAGE_COMPRESSED) {
					extended->pages_swapped_out++;
				}
				continue;
			}

			vm_map_region_look_for_page(map, va, obj,
			    vm_object_trunc_page(offset), ref_count,
			    0, extended, count);
		}

		if (do_region_footprint) {
			goto collect_object_info;
		}
	} else {
collect_object_info:
		shadow_object = obj->shadow;
		shadow_depth = 0;

		if (!(obj->internal)) {
			extended->external_pager = 1;
		}

		if (shadow_object != VM_OBJECT_NULL) {
			vm_object_lock(shadow_object);
			for (;
			    shadow_object != VM_OBJECT_NULL;
			    shadow_depth++) {
				vm_object_t     next_shadow;

				if (!(shadow_object->internal)) {
					extended->external_pager = 1;
				}

				next_shadow = shadow_object->shadow;
				if (next_shadow) {
					vm_object_lock(next_shadow);
				}
				vm_object_unlock(shadow_object);
				shadow_object = next_shadow;
			}
		}
		extended->shadow_depth = shadow_depth;
	}

	if (extended->shadow_depth || entry->needs_copy) {
		extended->share_mode = SM_COW;
	} else {
		if (ref_count == 1) {
			extended->share_mode = SM_PRIVATE;
		} else {
			if (obj->true_share) {
				extended->share_mode = SM_TRUESHARED;
			} else {
				extended->share_mode = SM_SHARED;
			}
		}
	}
	extended->ref_count = ref_count - extended->shadow_depth;

	for (i = 0; i < extended->shadow_depth; i++) {
		if ((tmp_obj = obj->shadow) == 0) {
			break;
		}
		vm_object_lock(tmp_obj);
		vm_object_unlock(obj);

		if ((ref_count = tmp_obj->ref_count) > 1 && tmp_obj->paging_in_progress) {
			ref_count--;
		}

		extended->ref_count += ref_count;
		obj = tmp_obj;
	}
	vm_object_unlock(obj);

	if (extended->share_mode == SM_SHARED) {
		vm_map_entry_t       cur;
		vm_map_entry_t       last;
		int      my_refs;

		obj = VME_OBJECT(entry);
		last = vm_map_to_entry(map);
		my_refs = 0;

		if ((ref_count = obj->ref_count) > 1 && obj->paging_in_progress) {
			ref_count--;
		}
		for (cur = vm_map_first_entry(map); cur != last; cur = cur->vme_next) {
			my_refs += vm_map_region_count_obj_refs(cur, obj);
		}

		if (my_refs == ref_count) {
			extended->share_mode = SM_PRIVATE_ALIASED;
		} else if (my_refs > 1) {
			extended->share_mode = SM_SHARED_ALIASED;
		}
	}
}


                                                    


static void
vm_map_region_look_for_page(
	__unused vm_map_t               map,
	__unused vm_map_offset_t        va,
	vm_object_t                     object,
	vm_object_offset_t              offset,
	int                             max_refcnt,
	unsigned short                  depth,
	vm_region_extended_info_t       extended,
	mach_msg_type_number_t count)
{
	vm_page_t       p;
	vm_object_t     shadow;
	int             ref_count;
	vm_object_t     caller_object;

	shadow = object->shadow;
	caller_object = object;


	while (TRUE) {
		if (!(object->internal)) {
			extended->external_pager = 1;
		}

		if ((p = vm_page_lookup(object, offset)) != VM_PAGE_NULL) {
			if (shadow && (max_refcnt == 1)) {
				extended->pages_shared_now_private++;
			}

			if (!p->vmp_fictitious &&
			    (p->vmp_dirty || pmap_is_modified(VM_PAGE_GET_PHYS_PAGE(p)))) {
				extended->pages_dirtied++;
			} else if (count >= VM_REGION_EXTENDED_INFO_COUNT) {
				if (p->vmp_reusable || object->all_reusable) {
					extended->pages_reusable++;
				}
			}

			extended->pages_resident++;

			if (object != caller_object) {
				vm_object_unlock(object);
			}

			return;
		}
		if (object->internal &&
		    object->alive &&
		    !object->terminating &&
		    object->pager_ready) {
			if (VM_COMPRESSOR_PAGER_STATE_GET(object, offset)
			    == VM_EXTERNAL_STATE_EXISTS) {
				                             
				extended->pages_swapped_out++;
				if (object != caller_object) {
					vm_object_unlock(object);
				}
				return;
			}
		}

		if (shadow) {
			vm_object_lock(shadow);

			if ((ref_count = shadow->ref_count) > 1 && shadow->paging_in_progress) {
				ref_count--;
			}

			if (++depth > extended->shadow_depth) {
				extended->shadow_depth = depth;
			}

			if (ref_count > max_refcnt) {
				max_refcnt = ref_count;
			}

			if (object != caller_object) {
				vm_object_unlock(object);
			}

			offset = offset + object->vo_shadow_offset;
			object = shadow;
			shadow = object->shadow;
			continue;
		}
		if (object != caller_object) {
			vm_object_unlock(object);
		}
		break;
	}
}

static int
vm_map_region_count_obj_refs(
	vm_map_entry_t    entry,
	vm_object_t       object)
{
	int ref_count;
	vm_object_t chk_obj;
	vm_object_t tmp_obj;

	if (VME_OBJECT(entry) == 0) {
		return 0;
	}

	if (entry->is_sub_map) {
		return 0;
	} else {
		ref_count = 0;

		chk_obj = VME_OBJECT(entry);
		vm_object_lock(chk_obj);

		while (chk_obj) {
			if (chk_obj == object) {
				ref_count++;
			}
			tmp_obj = chk_obj->shadow;
			if (tmp_obj) {
				vm_object_lock(tmp_obj);
			}
			vm_object_unlock(chk_obj);

			chk_obj = tmp_obj;
		}
	}
	return ref_count;
}


  
                           
  
               
                                                 
                                               
        
                                                  
                                                    
                                                    
                                                 
                        
   
void
vm_map_simplify_entry(
	vm_map_t        map,
	vm_map_entry_t  this_entry)
{
	vm_map_entry_t  prev_entry;

	prev_entry = this_entry->vme_prev;

	if ((this_entry != vm_map_to_entry(map)) &&
	    (prev_entry != vm_map_to_entry(map)) &&

	    (prev_entry->vme_end == this_entry->vme_start) &&

	    (prev_entry->is_sub_map == this_entry->is_sub_map) &&
	    (VME_OBJECT(prev_entry) == VME_OBJECT(this_entry)) &&
	    ((VME_OFFSET(prev_entry) + (prev_entry->vme_end -
	    prev_entry->vme_start))
	    == VME_OFFSET(this_entry)) &&

	    (prev_entry->behavior == this_entry->behavior) &&
	    (prev_entry->needs_copy == this_entry->needs_copy) &&
	    (prev_entry->protection == this_entry->protection) &&
	    (prev_entry->max_protection == this_entry->max_protection) &&
	    (prev_entry->inheritance == this_entry->inheritance) &&
	    (prev_entry->use_pmap == this_entry->use_pmap) &&
	    (VME_ALIAS(prev_entry) == VME_ALIAS(this_entry)) &&
	    (prev_entry->no_cache == this_entry->no_cache) &&
	    (prev_entry->permanent == this_entry->permanent) &&
	    (prev_entry->map_aligned == this_entry->map_aligned) &&
	    (prev_entry->zero_wired_pages == this_entry->zero_wired_pages) &&
	    (prev_entry->used_for_jit == this_entry->used_for_jit) &&
	    (prev_entry->pmap_cs_associated == this_entry->pmap_cs_associated) &&
	                                                            
	    (prev_entry->iokit_acct == this_entry->iokit_acct) &&
	    (prev_entry->vme_resilient_codesign ==
	    this_entry->vme_resilient_codesign) &&
	    (prev_entry->vme_resilient_media ==
	    this_entry->vme_resilient_media) &&
	    (prev_entry->vme_no_copy_on_read == this_entry->vme_no_copy_on_read) &&

	    (prev_entry->wired_count == this_entry->wired_count) &&
	    (prev_entry->user_wired_count == this_entry->user_wired_count) &&

	    ((prev_entry->vme_atomic == FALSE) && (this_entry->vme_atomic == FALSE)) &&
	    (prev_entry->in_transition == FALSE) &&
	    (this_entry->in_transition == FALSE) &&
	    (prev_entry->needs_wakeup == FALSE) &&
	    (this_entry->needs_wakeup == FALSE) &&
	    (prev_entry->is_shared == this_entry->is_shared) &&
	    (prev_entry->superpage_size == FALSE) &&
	    (this_entry->superpage_size == FALSE)
	    ) {
		vm_map_store_entry_unlink(map, prev_entry);
		assert(prev_entry->vme_start < this_entry->vme_end);
		if (prev_entry->map_aligned) {
			assert(VM_MAP_PAGE_ALIGNED(prev_entry->vme_start,
			    VM_MAP_PAGE_MASK(map)));
		}
		this_entry->vme_start = prev_entry->vme_start;
		VME_OFFSET_SET(this_entry, VME_OFFSET(prev_entry));

		if (map->holelistenabled) {
			vm_map_store_update_first_free(map, this_entry, TRUE);
		}

		if (prev_entry->is_sub_map) {
			vm_map_deallocate(VME_SUBMAP(prev_entry));
		} else {
			vm_object_deallocate(VME_OBJECT(prev_entry));
		}
		vm_map_entry_dispose(map, prev_entry);
		SAVE_HINT_MAP_WRITE(map, this_entry);
	}
}

void
vm_map_simplify(
	vm_map_t        map,
	vm_map_offset_t start)
{
	vm_map_entry_t  this_entry;

	vm_map_lock(map);
	if (vm_map_lookup_entry(map, start, &this_entry)) {
		vm_map_simplify_entry(map, this_entry);
		vm_map_simplify_entry(map, this_entry->vme_next);
	}
	vm_map_unlock(map);
}

static void
vm_map_simplify_range(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end)
{
	vm_map_entry_t  entry;

	  
                                                         
    

	if (start >= end) {
		                           
		return;
	}

	start = vm_map_trunc_page(start,
	    VM_MAP_PAGE_MASK(map));
	end = vm_map_round_page(end,
	    VM_MAP_PAGE_MASK(map));

	if (!vm_map_lookup_entry(map, start, &entry)) {
		                                                           
		if (entry == vm_map_to_entry(map)) {
			                                       
			entry = vm_map_first_entry(map);
		} else {
			                           
			entry = entry->vme_next;
		}
	}

	while (entry != vm_map_to_entry(map) &&
	    entry->vme_start <= end) {
		                                                      
		vm_map_simplify_entry(map, entry);
		entry = entry->vme_next;
	}
}


  
                                    
           
                                                    
                                                      
                                                           
                         
        
                                                               
                                                           
                                                           
                                                   
                                               
   
kern_return_t
vm_map_machine_attribute(
	vm_map_t                        map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_machine_attribute_t  attribute,
	vm_machine_attribute_val_t* value)                          
{
	kern_return_t   ret;
	vm_map_size_t sync_size;
	vm_map_entry_t entry;

	if (start < vm_map_min(map) || end > vm_map_max(map)) {
		return KERN_INVALID_ADDRESS;
	}

	                                                                  
	sync_size = end - start;

	vm_map_lock(map);

	if (attribute != MATTR_CACHE) {
		                                                     
		                                                     
		ret = pmap_attribute(map->pmap, start, end - start,
		    attribute, value);
		vm_map_unlock(map);
		return ret;
	}

	ret = KERN_SUCCESS;                                                                                                       

	while (sync_size) {
		if (vm_map_lookup_entry(map, start, &entry)) {
			vm_map_size_t   sub_size;
			if ((entry->vme_end - start) > sync_size) {
				sub_size = sync_size;
				sync_size = 0;
			} else {
				sub_size = entry->vme_end - start;
				sync_size -= sub_size;
			}
			if (entry->is_sub_map) {
				vm_map_offset_t sub_start;
				vm_map_offset_t sub_end;

				sub_start = (start - entry->vme_start)
				    + VME_OFFSET(entry);
				sub_end = sub_start + sub_size;
				vm_map_machine_attribute(
					VME_SUBMAP(entry),
					sub_start,
					sub_end,
					attribute, value);
			} else {
				if (VME_OBJECT(entry)) {
					vm_page_t               m;
					vm_object_t             object;
					vm_object_t             base_object;
					vm_object_t             last_object;
					vm_object_offset_t      offset;
					vm_object_offset_t      base_offset;
					vm_map_size_t           range;
					range = sub_size;
					offset = (start - entry->vme_start)
					    + VME_OFFSET(entry);
					offset = vm_object_trunc_page(offset);
					base_offset = offset;
					object = VME_OBJECT(entry);
					base_object = object;
					last_object = NULL;

					vm_object_lock(object);

					while (range) {
						m = vm_page_lookup(
							object, offset);

						if (m && !m->vmp_fictitious) {
							ret =
							    pmap_attribute_cache_sync(
								VM_PAGE_GET_PHYS_PAGE(m),
								PAGE_SIZE,
								attribute, value);
						} else if (object->shadow) {
							offset = offset + object->vo_shadow_offset;
							last_object = object;
							object = object->shadow;
							vm_object_lock(last_object->shadow);
							vm_object_unlock(last_object);
							continue;
						}
						if (range < PAGE_SIZE) {
							range = 0;
						} else {
							range -= PAGE_SIZE;
						}

						if (base_object != object) {
							vm_object_unlock(object);
							vm_object_lock(base_object);
							object = base_object;
						}
						                           
						base_offset += PAGE_SIZE;
						offset = base_offset;
					}
					vm_object_unlock(object);
				}
			}
			start += sub_size;
		} else {
			vm_map_unlock(map);
			return KERN_FAILURE;
		}
	}

	vm_map_unlock(map);

	return ret;
}

  
                       
  
                                                              
                                                              
                                                                 
             
   
kern_return_t
vm_map_behavior_set(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end,
	vm_behavior_t   new_behavior)
{
	vm_map_entry_t  entry;
	vm_map_entry_t  temp_entry;

	if (start > end ||
	    start < vm_map_min(map) ||
	    end > vm_map_max(map)) {
		return KERN_NO_SPACE;
	}

	switch (new_behavior) {
	  
                                                                             
                                                                           
                            
    

	case VM_BEHAVIOR_DEFAULT:
	case VM_BEHAVIOR_RANDOM:
	case VM_BEHAVIOR_SEQUENTIAL:
	case VM_BEHAVIOR_RSEQNTL:
	case VM_BEHAVIOR_ZERO_WIRED_PAGES:
		vm_map_lock(map);

		  
                                                        
                                               
                                                     
                                                       
                               
     
		if (vm_map_range_check(map, start, end, &temp_entry)) {
			entry = temp_entry;
			vm_map_clip_start(map, entry, start);
		} else {
			vm_map_unlock(map);
			return KERN_INVALID_ADDRESS;
		}

		while ((entry != vm_map_to_entry(map)) && (entry->vme_start < end)) {
			vm_map_clip_end(map, entry, end);
			if (entry->is_sub_map) {
				assert(!entry->use_pmap);
			}

			if (new_behavior == VM_BEHAVIOR_ZERO_WIRED_PAGES) {
				entry->zero_wired_pages = TRUE;
			} else {
				entry->behavior = new_behavior;
			}
			entry = entry->vme_next;
		}

		vm_map_unlock(map);
		break;

	  
                                                                     
                                                                           
                           
    

	case VM_BEHAVIOR_WILLNEED:
		return vm_map_willneed(map, start, end);

	case VM_BEHAVIOR_DONTNEED:
		return vm_map_msync(map, start, end - start, VM_SYNC_DEACTIVATE | VM_SYNC_CONTIGUOUS);

	case VM_BEHAVIOR_FREE:
		return vm_map_msync(map, start, end - start, VM_SYNC_KILLPAGES | VM_SYNC_CONTIGUOUS);

	case VM_BEHAVIOR_REUSABLE:
		return vm_map_reusable_pages(map, start, end);

	case VM_BEHAVIOR_REUSE:
		return vm_map_reuse_pages(map, start, end);

	case VM_BEHAVIOR_CAN_REUSE:
		return vm_map_can_reuse(map, start, end);

#if MACH_ASSERT
	case VM_BEHAVIOR_PAGEOUT:
		return vm_map_pageout(map, start, end);
#endif                  

	default:
		return KERN_INVALID_ARGUMENT;
	}

	return KERN_SUCCESS;
}


  
                                                    
  
                                
                                                                    
                                                                                     
   


static kern_return_t
vm_map_willneed(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end
	)
{
	vm_map_entry_t                  entry;
	vm_object_t                     object;
	memory_object_t                 pager;
	struct vm_object_fault_info     fault_info = {};
	kern_return_t                   kr;
	vm_object_size_t                len;
	vm_object_offset_t              offset;

	fault_info.interruptible = THREAD_UNINT;                           
	fault_info.behavior      = VM_BEHAVIOR_SEQUENTIAL;
	fault_info.stealth       = TRUE;

	  
                                                                  
                                                     
    

	vm_map_lock_read(map);

	  
                                                                 
                                                                 
             
    

	if (!vm_map_range_check(map, start, end, &entry)) {
		vm_map_unlock_read(map);
		return KERN_INVALID_ADDRESS;
	}

	  
                                             
    
	for (; entry != vm_map_to_entry(map) && start < end;) {
		  
                                                                
                                                               
                                                               
                                                             
     
		offset = (start - entry->vme_start) + VME_OFFSET(entry);

		  
                                                        
                                                            
                                                                 
                                                                  
                                                               
                                                                
                                                            
                                  
     
		len = MIN(entry->vme_end - start, end - start);

		if ((vm_size_t) len != len) {
			                     
			len = (vm_size_t) (0 - PAGE_SIZE);
		}
		fault_info.cluster_size = (vm_size_t) len;
		fault_info.lo_offset    = offset;
		fault_info.hi_offset    = offset + len;
		fault_info.user_tag     = VME_ALIAS(entry);
		fault_info.pmap_options = 0;
		if (entry->iokit_acct ||
		    (!entry->is_sub_map && !entry->use_pmap)) {
			fault_info.pmap_options |= PMAP_OPTIONS_ALT_ACCT;
		}

		  
                                                           
                                        
     
		if ((entry->is_sub_map) || (entry->protection & VM_PROT_READ) == 0) {
			entry = entry->vme_next;
			start = entry->vme_start;
			continue;
		}

		object = VME_OBJECT(entry);

		if (object == NULL ||
		    (object && object->internal)) {
			  
                                              
      
			vm_size_t region_size = 0, effective_page_size = 0;
			vm_map_offset_t addr = 0, effective_page_mask = 0;

			region_size = len;
			addr = start;

			effective_page_mask = MIN(vm_map_page_mask(current_map()), PAGE_MASK);
			effective_page_size = effective_page_mask + 1;

			vm_map_unlock_read(map);

			while (region_size) {
				vm_pre_fault(
					vm_map_trunc_page(addr, effective_page_mask),
					VM_PROT_READ | VM_PROT_WRITE);

				region_size -= effective_page_size;
				addr += effective_page_size;
			}
		} else {
			  
                                                               
                                                                 
                                      
      
			if ((object = find_vnode_object(entry)) == VM_OBJECT_NULL) {
				entry = entry->vme_next;
				start = entry->vme_start;
				continue;
			}

			vm_object_paging_begin(object);
			pager = object->pager;
			vm_object_unlock(object);

			  
                                                         
                                                           
      
			vm_map_unlock_read(map);

			  
                                                  
     
                                                                 
                                                         
                                                                    
                                                          
                                                               
                                                             
                                                                  
                                                                
                                                              
                    
      
			kr = memory_object_data_request(
				pager,
				vm_object_trunc_page(offset) + object->paging_offset,
				0,                   
				VM_PROT_READ,
				(memory_object_fault_info_t)&fault_info);

			vm_object_lock(object);
			vm_object_paging_end(object);
			vm_object_unlock(object);

			  
                                                                
                                                             
                                                             
            
      

			if (kr != KERN_SUCCESS) {
				return KERN_SUCCESS;
			}
		}

		start += len;
		if (start >= end) {
			          
			return KERN_SUCCESS;
		}

		                        
		vm_map_lock_read(map);
		if (!vm_map_lookup_entry(map, start, &entry)) {
			  
                                              
      
			vm_map_unlock_read(map);
			return KERN_INVALID_ADDRESS;
		}
	}

	vm_map_unlock_read(map);
	return KERN_SUCCESS;
}

static boolean_t
vm_map_entry_is_reusable(
	vm_map_entry_t entry)
{
	                           

	vm_object_t object;

	if (entry->is_sub_map) {
		return FALSE;
	}

	switch (VME_ALIAS(entry)) {
	case VM_MEMORY_MALLOC:
	case VM_MEMORY_MALLOC_SMALL:
	case VM_MEMORY_MALLOC_LARGE:
	case VM_MEMORY_REALLOC:
	case VM_MEMORY_MALLOC_TINY:
	case VM_MEMORY_MALLOC_LARGE_REUSABLE:
	case VM_MEMORY_MALLOC_LARGE_REUSED:
		  
                                                          
                                                      
                          
     
		break;
	default:
		  
                                                           
                    
     
		return TRUE;
	}

	if (                       
		entry->is_sub_map ||
		entry->in_transition ||
		entry->protection != VM_PROT_DEFAULT ||
		entry->max_protection != VM_PROT_ALL ||
		entry->inheritance != VM_INHERIT_DEFAULT ||
		entry->no_cache ||
		entry->permanent ||
		entry->superpage_size != FALSE ||
		entry->zero_wired_pages ||
		entry->wired_count != 0 ||
		entry->user_wired_count != 0) {
		return FALSE;
	}

	object = VME_OBJECT(entry);
	if (object == VM_OBJECT_NULL) {
		return TRUE;
	}
	if (
#if 0
		  
                                                       
            
                                                       
                                                      
    
                                                              
                                                                
                                                             
                                                              
                        
     
		object->ref_count == 1 &&
#endif
		object->wired_page_count == 0 &&
		object->copy == VM_OBJECT_NULL &&
		object->shadow == VM_OBJECT_NULL &&
		object->internal &&
		object->purgable == VM_PURGABLE_DENY &&
		object->wimg_bits == VM_WIMG_USE_DEFAULT &&
		!object->code_signed) {
		return TRUE;
	}
	return FALSE;
}

static kern_return_t
vm_map_reuse_pages(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end)
{
	vm_map_entry_t                  entry;
	vm_object_t                     object;
	vm_object_offset_t              start_offset, end_offset;

	  
                                                               
                                                     
    

	if (VM_MAP_PAGE_SHIFT(map) < PAGE_SHIFT) {
		  
               
                                                 
                              
     
		return KERN_SUCCESS;
	}

	vm_map_lock_read(map);
	assert(map->pmap != kernel_pmap);                                 

	  
                                                                 
                                                                 
             
    

	if (!vm_map_range_check(map, start, end, &entry)) {
		vm_map_unlock_read(map);
		vm_page_stats_reusable.reuse_pages_failure++;
		return KERN_INVALID_ADDRESS;
	}

	  
                                             
    
	for (; entry != vm_map_to_entry(map) && entry->vme_start < end;
	    entry = entry->vme_next) {
		  
                                      
     
		if (!vm_map_entry_is_reusable(entry)) {
			vm_map_unlock_read(map);
			vm_page_stats_reusable.reuse_pages_failure++;
			return KERN_INVALID_ADDRESS;
		}

		  
                                                                
                                                               
                
     
		if (entry->vme_start < start) {
			start_offset = start - entry->vme_start;
		} else {
			start_offset = 0;
		}
		end_offset = MIN(end, entry->vme_end) - entry->vme_start;
		start_offset += VME_OFFSET(entry);
		end_offset += VME_OFFSET(entry);

		assert(!entry->is_sub_map);
		object = VME_OBJECT(entry);
		if (object != VM_OBJECT_NULL) {
			vm_object_lock(object);
			vm_object_reuse_pages(object, start_offset, end_offset,
			    TRUE);
			vm_object_unlock(object);
		}

		if (VME_ALIAS(entry) == VM_MEMORY_MALLOC_LARGE_REUSABLE) {
			  
         
                                                 
                                                     
                                                       
                                                       
               
      
			VME_ALIAS_SET(entry, VM_MEMORY_MALLOC_LARGE_REUSED);
		}
	}

	vm_map_unlock_read(map);
	vm_page_stats_reusable.reuse_pages_success++;
	return KERN_SUCCESS;
}


static kern_return_t
vm_map_reusable_pages(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end)
{
	vm_map_entry_t                  entry;
	vm_object_t                     object;
	vm_object_offset_t              start_offset, end_offset;
	vm_map_offset_t                 pmap_offset;

	if (VM_MAP_PAGE_SHIFT(map) < PAGE_SHIFT) {
		  
               
                                                         
                      
     
		return KERN_SUCCESS;
	}

	  
                                                                  
                                                     
    

	vm_map_lock_read(map);
	assert(map->pmap != kernel_pmap);                                 

	  
                                                                 
                                                                 
             
    

	if (!vm_map_range_check(map, start, end, &entry)) {
		vm_map_unlock_read(map);
		vm_page_stats_reusable.reusable_pages_failure++;
		return KERN_INVALID_ADDRESS;
	}

	  
                                             
    
	for (; entry != vm_map_to_entry(map) && entry->vme_start < end;
	    entry = entry->vme_next) {
		int kill_pages = 0;

		  
                                      
     
		if (!vm_map_entry_is_reusable(entry)) {
			vm_map_unlock_read(map);
			vm_page_stats_reusable.reusable_pages_failure++;
			return KERN_INVALID_ADDRESS;
		}

		if (!(entry->protection & VM_PROT_WRITE) && !entry->used_for_jit) {
			                                          
			vm_map_unlock_read(map);
			vm_page_stats_reusable.reusable_nonwritable++;
			vm_page_stats_reusable.reusable_pages_failure++;
			return KERN_PROTECTION_FAILURE;
		}

		  
                                                                
                                                               
                
     
		if (entry->vme_start < start) {
			start_offset = start - entry->vme_start;
			pmap_offset = start;
		} else {
			start_offset = 0;
			pmap_offset = entry->vme_start;
		}
		end_offset = MIN(end, entry->vme_end) - entry->vme_start;
		start_offset += VME_OFFSET(entry);
		end_offset += VME_OFFSET(entry);

		assert(!entry->is_sub_map);
		object = VME_OBJECT(entry);
		if (object == VM_OBJECT_NULL) {
			continue;
		}


		vm_object_lock(object);
		if (((object->ref_count == 1) ||
		    (object->copy_strategy != MEMORY_OBJECT_COPY_SYMMETRIC &&
		    object->copy == VM_OBJECT_NULL)) &&
		    object->shadow == VM_OBJECT_NULL &&
		      
                                                               
                                                             
                                                            
                                                           
                                                          
                 
         
		    !(entry->iokit_acct ||
		    (!entry->is_sub_map && !entry->use_pmap))) {
			if (object->ref_count != 1) {
				vm_page_stats_reusable.reusable_shared++;
			}
			kill_pages = 1;
		} else {
			kill_pages = -1;
		}
		if (kill_pages != -1) {
			vm_object_deactivate_pages(object,
			    start_offset,
			    end_offset - start_offset,
			    kill_pages,
			    TRUE                   ,
			    map->pmap,
			    pmap_offset);
		} else {
			vm_page_stats_reusable.reusable_pages_shared++;
		}
		vm_object_unlock(object);

		if (VME_ALIAS(entry) == VM_MEMORY_MALLOC_LARGE ||
		    VME_ALIAS(entry) == VM_MEMORY_MALLOC_LARGE_REUSED) {
			  
         
                                                 
                                                     
                                                       
                                                       
               
      
			VME_ALIAS_SET(entry, VM_MEMORY_MALLOC_LARGE_REUSABLE);
		}
	}

	vm_map_unlock_read(map);
	vm_page_stats_reusable.reusable_pages_success++;
	return KERN_SUCCESS;
}


static kern_return_t
vm_map_can_reuse(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end)
{
	vm_map_entry_t                  entry;

	  
                                                                  
                                                     
    

	vm_map_lock_read(map);
	assert(map->pmap != kernel_pmap);                                 

	  
                                                                 
                                                                 
             
    

	if (!vm_map_range_check(map, start, end, &entry)) {
		vm_map_unlock_read(map);
		vm_page_stats_reusable.can_reuse_failure++;
		return KERN_INVALID_ADDRESS;
	}

	  
                                             
    
	for (; entry != vm_map_to_entry(map) && entry->vme_start < end;
	    entry = entry->vme_next) {
		  
                                      
     
		if (!vm_map_entry_is_reusable(entry)) {
			vm_map_unlock_read(map);
			vm_page_stats_reusable.can_reuse_failure++;
			return KERN_INVALID_ADDRESS;
		}
	}

	vm_map_unlock_read(map);
	vm_page_stats_reusable.can_reuse_success++;
	return KERN_SUCCESS;
}


#if MACH_ASSERT
static kern_return_t
vm_map_pageout(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end)
{
	vm_map_entry_t                  entry;

	  
                                                                 
                                                     
    

	vm_map_lock_read(map);

	  
                                                                 
                                                                 
             
    

	if (!vm_map_range_check(map, start, end, &entry)) {
		vm_map_unlock_read(map);
		return KERN_INVALID_ADDRESS;
	}

	  
                                             
    
	for (; entry != vm_map_to_entry(map) && entry->vme_start < end;
	    entry = entry->vme_next) {
		vm_object_t     object;

		  
                                      
     
		if (entry->is_sub_map) {
			vm_map_t submap;
			vm_map_offset_t submap_start;
			vm_map_offset_t submap_end;
			vm_map_entry_t submap_entry;

			submap = VME_SUBMAP(entry);
			submap_start = VME_OFFSET(entry);
			submap_end = submap_start + (entry->vme_end -
			    entry->vme_start);

			vm_map_lock_read(submap);

			if (!vm_map_range_check(submap,
			    submap_start,
			    submap_end,
			    &submap_entry)) {
				vm_map_unlock_read(submap);
				vm_map_unlock_read(map);
				return KERN_INVALID_ADDRESS;
			}

			object = VME_OBJECT(submap_entry);
			if (submap_entry->is_sub_map ||
			    object == VM_OBJECT_NULL ||
			    !object->internal) {
				vm_map_unlock_read(submap);
				continue;
			}

			vm_object_pageout(object);

			vm_map_unlock_read(submap);
			submap = VM_MAP_NULL;
			submap_entry = VM_MAP_ENTRY_NULL;
			continue;
		}

		object = VME_OBJECT(entry);
		if (entry->is_sub_map ||
		    object == VM_OBJECT_NULL ||
		    !object->internal) {
			continue;
		}

		vm_object_pageout(object);
	}

	vm_map_unlock_read(map);
	return KERN_SUCCESS;
}
#endif                  


  
                               
  
                                                                    
   
vm_map_entry_t
vm_map_entry_insert(
	vm_map_t                map,
	vm_map_entry_t          insp_entry,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_map_kernel_flags_t   vmk_flags,
	boolean_t               needs_copy,
	boolean_t               is_shared,
	boolean_t               in_transition,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_behavior_t           behavior,
	vm_inherit_t            inheritance,
	unsigned short          wired_count,
	boolean_t               no_cache,
	boolean_t               permanent,
	boolean_t               no_copy_on_read,
	unsigned int            superpage_size,
	boolean_t               clear_map_aligned,
	boolean_t               is_submap,
	boolean_t               used_for_jit,
	int                     alias,
	boolean_t               translated_allow_execute)
{
	vm_map_entry_t  new_entry;

	assert(insp_entry != (vm_map_entry_t)0);
	vm_map_lock_assert_exclusive(map);

#if DEVELOPMENT || DEBUG
	vm_object_offset_t      end_offset = 0;
	assertf(!os_add_overflow(end - start, offset, &end_offset), "size 0x%llx, offset 0x%llx caused overflow", (uint64_t)(end - start), offset);
#endif                           

	new_entry = vm_map_entry_create(map, !map->hdr.entries_pageable);

	if (VM_MAP_PAGE_SHIFT(map) != PAGE_SHIFT) {
		new_entry->map_aligned = TRUE;
	} else {
		new_entry->map_aligned = FALSE;
	}
	if (clear_map_aligned &&
	    (!VM_MAP_PAGE_ALIGNED(start, VM_MAP_PAGE_MASK(map)) ||
	    !VM_MAP_PAGE_ALIGNED(end, VM_MAP_PAGE_MASK(map)))) {
		new_entry->map_aligned = FALSE;
	}

	new_entry->vme_start = start;
	new_entry->vme_end = end;
	if (new_entry->map_aligned) {
		assert(VM_MAP_PAGE_ALIGNED(new_entry->vme_start,
		    VM_MAP_PAGE_MASK(map)));
		assert(VM_MAP_PAGE_ALIGNED(new_entry->vme_end,
		    VM_MAP_PAGE_MASK(map)));
	} else {
		assert(page_aligned(new_entry->vme_start));
		assert(page_aligned(new_entry->vme_end));
	}
	assert(new_entry->vme_start < new_entry->vme_end);

	VME_OBJECT_SET(new_entry, object);
	VME_OFFSET_SET(new_entry, offset);
	new_entry->is_shared = is_shared;
	new_entry->is_sub_map = is_submap;
	new_entry->needs_copy = needs_copy;
	new_entry->in_transition = in_transition;
	new_entry->needs_wakeup = FALSE;
	new_entry->inheritance = inheritance;
	new_entry->protection = cur_protection;
	new_entry->max_protection = max_protection;
	new_entry->behavior = behavior;
	new_entry->wired_count = wired_count;
	new_entry->user_wired_count = 0;
	if (is_submap) {
		  
                                       
                    
     
		new_entry->use_pmap = FALSE;
	} else {
		  
                                                                  
                   
     
		new_entry->use_pmap = TRUE;
	}
	VME_ALIAS_SET(new_entry, alias);
	new_entry->zero_wired_pages = FALSE;
	new_entry->no_cache = no_cache;
	new_entry->permanent = permanent;
	if (superpage_size) {
		new_entry->superpage_size = TRUE;
	} else {
		new_entry->superpage_size = FALSE;
	}
	if (used_for_jit) {
		if (!(map->jit_entry_exists) ||
		    VM_MAP_POLICY_ALLOW_MULTIPLE_JIT(map)) {
			new_entry->used_for_jit = TRUE;
			map->jit_entry_exists = TRUE;
		}
	} else {
		new_entry->used_for_jit = FALSE;
	}
	if (translated_allow_execute) {
		new_entry->translated_allow_execute = TRUE;
	} else {
		new_entry->translated_allow_execute = FALSE;
	}
	new_entry->pmap_cs_associated = FALSE;
	new_entry->iokit_acct = FALSE;
	new_entry->vme_resilient_codesign = FALSE;
	new_entry->vme_resilient_media = FALSE;
	new_entry->vme_atomic = FALSE;
	new_entry->vme_no_copy_on_read = no_copy_on_read;

	  
                                       
    

	vm_map_store_entry_link(map, insp_entry, new_entry, vmk_flags);
	map->size += end - start;

	  
                                                   
    

	SAVE_HINT_MAP_WRITE(map, new_entry);
	return new_entry;
}

  
                                
  
                                                                
   
static kern_return_t
vm_map_remap_extract(
	vm_map_t                map,
	vm_map_offset_t         addr,
	vm_map_size_t           size,
	boolean_t               copy,
	struct vm_map_header    *map_header,
	vm_prot_t               *cur_protection,               
	vm_prot_t               *max_protection,               
	                        
	vm_inherit_t            inheritance,
	vm_map_kernel_flags_t   vmk_flags)
{
	kern_return_t           result;
	vm_map_size_t           mapped_size;
	vm_map_size_t           tmp_size;
	vm_map_entry_t          src_entry;                                    
	vm_map_entry_t          new_entry;
	vm_object_offset_t      offset;
	vm_map_offset_t         map_address;
	vm_map_offset_t         src_start;                                
	vm_map_offset_t         src_end;                                       
	vm_object_t             object;
	vm_map_version_t        version;
	boolean_t               src_needs_copy;
	boolean_t               new_entry_needs_copy;
	vm_map_entry_t          saved_src_entry;
	boolean_t               src_entry_was_wired;
	vm_prot_t               max_prot_for_prot_copy;
	vm_map_offset_t         effective_page_mask;
	boolean_t               pageable, same_map;
	boolean_t               vm_remap_legacy;
	vm_prot_t               required_cur_prot, required_max_prot;

	pageable = vmk_flags.vmkf_copy_pageable;
	same_map = vmk_flags.vmkf_copy_same_map;

	effective_page_mask = MIN(PAGE_MASK, VM_MAP_PAGE_MASK(map));

	assert(map != VM_MAP_NULL);
	assert(size != 0);
	assert(size == vm_map_round_page(size, effective_page_mask));
	assert(inheritance == VM_INHERIT_NONE ||
	    inheritance == VM_INHERIT_COPY ||
	    inheritance == VM_INHERIT_SHARE);
	assert(!(*cur_protection & ~VM_PROT_ALL));
	assert(!(*max_protection & ~VM_PROT_ALL));
	assert((*cur_protection & *max_protection) == *cur_protection);

	  
                                    
    
	src_start = vm_map_trunc_page(addr, effective_page_mask);
	src_end = vm_map_round_page(src_start + size, effective_page_mask);

	  
                          
    
	map_header->links.next = CAST_TO_VM_MAP_ENTRY(&map_header->links);
	map_header->links.prev = CAST_TO_VM_MAP_ENTRY(&map_header->links);
	map_header->nentries = 0;
	map_header->entries_pageable = pageable;
                                                                    
	map_header->page_shift = VM_MAP_PAGE_SHIFT(map);
	map_header->rb_head_store.rbh_root = (void *)(int)SKIP_RB_TREE;

	vm_map_store_init( map_header );

	if (copy && vmk_flags.vmkf_remap_prot_copy) {
		  
                                                   
                                                           
                                 
     
		max_prot_for_prot_copy = *max_protection & VM_PROT_ALL;
		                                                       
		*max_protection = VM_PROT_NONE;
		*cur_protection = VM_PROT_NONE;
	} else {
		max_prot_for_prot_copy = VM_PROT_NONE;
	}

	if (*cur_protection == VM_PROT_NONE &&
	    *max_protection == VM_PROT_NONE) {
		  
                            
                                                          
                                                            
                                                            
                        
                                                               
                                     
     
		vm_remap_legacy = TRUE;
		*cur_protection = VM_PROT_ALL;
		*max_protection = VM_PROT_ALL;
		required_cur_prot = VM_PROT_NONE;
		required_max_prot = VM_PROT_NONE;
	} else {
		  
                         
                                                          
                                                             
                                                           
                                                         
     
		vm_remap_legacy = FALSE;
		if (copy) {
			required_cur_prot = VM_PROT_NONE;
			required_max_prot = VM_PROT_READ;
		} else {
			required_cur_prot = *cur_protection;
			required_max_prot = *max_protection;
		}
	}

	map_address = 0;
	mapped_size = 0;
	result = KERN_SUCCESS;

	  
                                                          
                                               
    
	vm_map_lock(map);
	if (VM_MAP_PAGE_SHIFT(map) < PAGE_SHIFT) {
		  
                                                         
                                                       
                                                       
                                           
     
		vm_map_simplify_range(map, src_start, src_end);
	}
	while (mapped_size != size) {
		vm_map_size_t   entry_size;

		  
                                      
     
		if (!vm_map_lookup_entry(map, src_start, &src_entry)) {
			result = KERN_INVALID_ADDRESS;
			break;
		}

		if (src_start < src_entry->vme_start ||
		    (mapped_size && src_start != src_entry->vme_start)) {
			result = KERN_INVALID_ADDRESS;
			break;
		}

		tmp_size = size - mapped_size;
		if (src_end > src_entry->vme_end) {
			tmp_size -= (src_end - src_entry->vme_end);
		}

		entry_size = (vm_map_size_t)(src_entry->vme_end -
		    src_entry->vme_start);

		if (src_entry->is_sub_map &&
		    vmk_flags.vmkf_copy_single_object) {
			vm_map_t submap;
			vm_map_offset_t submap_start;
			vm_map_size_t submap_size;
			boolean_t submap_needs_copy;

			  
                                                       
                                                      
                                                         
                                                      
      
			submap_size = src_entry->vme_end - src_start;
			if (submap_size > size) {
				submap_size = size;
			}
			submap_start = VME_OFFSET(src_entry) + src_start - src_entry->vme_start;
			submap = VME_SUBMAP(src_entry);
			if (copy) {
				  
                                                   
                                                    
       
				submap_needs_copy = TRUE;
			} else if (src_entry->needs_copy) {
				  
                                                   
                                                 
                                                  
                                                 
                                              
                                                 
                                                  
                    
                                                    
               
       
				submap_needs_copy = TRUE;
			} else {
				  
                                               
                                                    
                                                    
                                        
                                 
       
				submap_needs_copy = FALSE;
			}
			vm_map_reference(submap);
			vm_map_unlock(map);
			src_entry = NULL;
			if (vm_remap_legacy) {
				*cur_protection = VM_PROT_NONE;
				*max_protection = VM_PROT_NONE;
			}

			DTRACE_VM7(remap_submap_recurse,
			    vm_map_t, map,
			    vm_map_offset_t, addr,
			    vm_map_size_t, size,
			    boolean_t, copy,
			    vm_map_offset_t, submap_start,
			    vm_map_size_t, submap_size,
			    boolean_t, submap_needs_copy);

			result = vm_map_remap_extract(submap,
			    submap_start,
			    submap_size,
			    submap_needs_copy,
			    map_header,
			    cur_protection,
			    max_protection,
			    inheritance,
			    vmk_flags);
			vm_map_deallocate(submap);
			return result;
		}

		if (src_entry->is_sub_map) {
			                                                        
		} else if (((src_entry->protection & required_cur_prot) !=
		    required_cur_prot) ||
		    ((src_entry->max_protection & required_max_prot) !=
		    required_max_prot)) {
			if (vmk_flags.vmkf_copy_single_object &&
			    mapped_size != 0) {
				  
                                
                                              
                                              
                                      
                                          
                                      
       
				result = KERN_SUCCESS;
			} else {
				  
                           
                                            
                                        
       
				result = KERN_PROTECTION_FAILURE;
			}
			break;
		}

		if (src_entry->is_sub_map) {
			vm_map_t submap;
			vm_map_offset_t submap_start;
			vm_map_size_t submap_size;
			vm_map_copy_t submap_copy;
			vm_prot_t submap_curprot, submap_maxprot;
			boolean_t submap_needs_copy;

			  
                                                       
                                                      
                                                         
                                                     
      
			object = VM_OBJECT_NULL;
			submap_copy = VM_MAP_COPY_NULL;

			                                         
			submap = VME_SUBMAP(src_entry);
			submap_start = VME_OFFSET(src_entry) + src_start - src_entry->vme_start;
			submap_size = tmp_size;
			if (copy) {
				  
                                                   
                                                    
       
				submap_needs_copy = TRUE;
			} else if (src_entry->needs_copy) {
				  
                                                   
                                                 
                                                  
                                                 
                                              
                                                 
                                                  
                    
                                                    
               
       
				submap_needs_copy = TRUE;
			} else {
				  
                                               
                                                    
                                                    
                                        
                                 
       
				submap_needs_copy = FALSE;
			}
			                                    
			vm_map_reference(submap);

			DTRACE_VM7(remap_submap_recurse,
			    vm_map_t, map,
			    vm_map_offset_t, addr,
			    vm_map_size_t, size,
			    boolean_t, copy,
			    vm_map_offset_t, submap_start,
			    vm_map_size_t, submap_size,
			    boolean_t, submap_needs_copy);

			  
                                             
                                             
     
                                                 
                                                   
                                                         
             
      
			vm_map_unlock(map);
			src_entry = NULL;                                     

			if (vm_remap_legacy) {
				submap_curprot = VM_PROT_NONE;
				submap_maxprot = VM_PROT_NONE;
				if (max_prot_for_prot_copy) {
					submap_maxprot = max_prot_for_prot_copy;
				}
			} else {
				assert(!max_prot_for_prot_copy);
				submap_curprot = *cur_protection;
				submap_maxprot = *max_protection;
			}
			result = vm_map_copy_extract(submap,
			    submap_start,
			    submap_size,
			    submap_needs_copy,
			    &submap_copy,
			    &submap_curprot,
			    &submap_maxprot,
			    inheritance,
			    vmk_flags);

			                                 
			vm_map_deallocate(submap);
			submap = VM_MAP_NULL;

			if (result != KERN_SUCCESS) {
				vm_map_lock(map);
				break;
			}

			                                                
			while (vm_map_copy_first_entry(submap_copy) !=
			    vm_map_copy_to_entry(submap_copy)) {
				vm_map_entry_t copy_entry;
				vm_map_size_t copy_entry_size;

				copy_entry = vm_map_copy_first_entry(submap_copy);
				assert(!copy_entry->is_sub_map);
				object = VME_OBJECT(copy_entry);

				  
                                                  
                  
       
				if (__improbable(object == kernel_object)) {
					printf("%d[%s]: rejecting attempt to extract from kernel_object\n",
					    proc_selfpid(),
					    (current_task()->bsd_info
					    ? proc_name_address(current_task()->bsd_info)
					    : "?"));
					DTRACE_VM(extract_kernel_only);
					result = KERN_INVALID_RIGHT;
					vm_map_copy_discard(submap_copy);
					submap_copy = VM_MAP_COPY_NULL;
					vm_map_lock(map);
					break;
				}

				vm_map_copy_entry_unlink(submap_copy, copy_entry);
				copy_entry_size = copy_entry->vme_end - copy_entry->vme_start;
				copy_entry->vme_start = map_address;
				copy_entry->vme_end = map_address + copy_entry_size;
				map_address += copy_entry_size;
				mapped_size += copy_entry_size;
				src_start += copy_entry_size;
				assert(src_start <= src_end);
				_vm_map_store_entry_link(map_header,
				    map_header->links.prev,
				    copy_entry);
			}
			                           
			vm_map_copy_discard(submap_copy);

			if (vm_remap_legacy) {
				*cur_protection &= submap_curprot;
				*max_protection &= submap_maxprot;
			}

			                                                        
			vm_map_lock(map);
			continue;
		} else {
			object = VME_OBJECT(src_entry);

			  
                                                 
                 
      
			if (__improbable(object == kernel_object)) {
				printf("%d[%s]: rejecting attempt to extract from kernel_object\n",
				    proc_selfpid(),
				    (current_task()->bsd_info
				    ? proc_name_address(current_task()->bsd_info)
				    : "?"));
				DTRACE_VM(extract_kernel_only);
				result = KERN_INVALID_RIGHT;
				break;
			}

			if (src_entry->iokit_acct) {
				  
                                          
       
			} else if (object != VM_OBJECT_NULL &&
			    (object->purgable != VM_PURGABLE_DENY ||
			    object->vo_ledger_tag != VM_LEDGER_TAG_NONE)) {
				  
                                                   
                                   
       
				assertf(!src_entry->use_pmap,
				    "map=%p src_entry=%p [0x%llx:0x%llx] 0x%x/0x%x %d",
				    map,
				    src_entry,
				    (uint64_t)src_entry->vme_start,
				    (uint64_t)src_entry->vme_end,
				    src_entry->protection,
				    src_entry->max_protection,
				    VME_ALIAS(src_entry));
			} else {
				  
                              
                                       
       
				assertf(src_entry->use_pmap,
				    "map=%p src_entry=%p [0x%llx:0x%llx] 0x%x/0x%x %d",
				    map,
				    src_entry,
				    (uint64_t)src_entry->vme_start,
				    (uint64_t)src_entry->vme_end,
				    src_entry->protection,
				    src_entry->max_protection,
				    VME_ALIAS(src_entry));
			}

			if (object == VM_OBJECT_NULL) {
				assert(!src_entry->needs_copy);
				object = vm_object_allocate(entry_size);
				VME_OFFSET_SET(src_entry, 0);
				VME_OBJECT_SET(src_entry, object);
				assert(src_entry->use_pmap);
				assert(!map->mapped_in_other_pmaps);
			} else if (src_entry->wired_count ||
			    object->copy_strategy != MEMORY_OBJECT_COPY_SYMMETRIC) {
				  
                                            
                                             
                                          
                                
                                                 
                                  
                                                
                                               
                                                   
      
                                             
                                          
                        
       
				assert(!src_entry->needs_copy);
			} else if (src_entry->needs_copy || object->shadowed ||
			    (object->internal && !object->true_share &&
			    !src_entry->is_shared &&
			    object->vo_size > entry_size)) {
				VME_OBJECT_SHADOW(src_entry, entry_size);
				assert(src_entry->use_pmap);

				if (!src_entry->needs_copy &&
				    (src_entry->protection & VM_PROT_WRITE)) {
					vm_prot_t prot;

					assert(!pmap_has_prot_policy(map->pmap, src_entry->translated_allow_execute, src_entry->protection));

					prot = src_entry->protection & ~VM_PROT_WRITE;

					if (override_nx(map,
					    VME_ALIAS(src_entry))
					    && prot) {
						prot |= VM_PROT_EXECUTE;
					}

					assert(!pmap_has_prot_policy(map->pmap, src_entry->translated_allow_execute, prot));

					if (map->mapped_in_other_pmaps) {
						vm_object_pmap_protect(
							VME_OBJECT(src_entry),
							VME_OFFSET(src_entry),
							entry_size,
							PMAP_NULL,
							PAGE_SIZE,
							src_entry->vme_start,
							prot);
#if MACH_ASSERT
					} else if (__improbable(map->pmap == PMAP_NULL)) {
						extern boolean_t vm_tests_in_progress;
						assert(vm_tests_in_progress);
						  
                                      
                                   
                            
                                     
                
         
#endif                  
					} else {
						pmap_protect(vm_map_pmap(map),
						    src_entry->vme_start,
						    src_entry->vme_end,
						    prot);
					}
				}

				object = VME_OBJECT(src_entry);
				src_entry->needs_copy = FALSE;
			}


			vm_object_lock(object);
			vm_object_reference_locked(object);                                
			assert(!src_entry->needs_copy);
			if (object->copy_strategy ==
			    MEMORY_OBJECT_COPY_SYMMETRIC) {
				  
                                                 
                                 
                                                
                                                 
                                               
                                                  
                           
                                                 
                  
       
				object->copy_strategy =
				    MEMORY_OBJECT_COPY_DELAY;
				object->true_share = TRUE;
			}
			vm_object_unlock(object);
		}

		offset = (VME_OFFSET(src_entry) +
		    (src_start - src_entry->vme_start));

		new_entry = _vm_map_entry_create(map_header, !map_header->entries_pageable);
		vm_map_entry_copy(map, new_entry, src_entry);
		if (new_entry->is_sub_map) {
			                                 
			new_entry->use_pmap = FALSE;
		} else if (copy) {
			  
                                                   
                                                     
                                             
                                                      
                                                   
                                  
      
			new_entry->use_pmap = TRUE;
		}
		                                                     
		assert(!new_entry->iokit_acct);

		new_entry->map_aligned = FALSE;

		new_entry->vme_start = map_address;
		new_entry->vme_end = map_address + tmp_size;
		assert(new_entry->vme_start < new_entry->vme_end);
		if (copy && vmk_flags.vmkf_remap_prot_copy) {
			  
                                                
                                           
                                         
                        
                                           
                                          
      
			new_entry->inheritance = src_entry->inheritance;
			new_entry->protection &= max_prot_for_prot_copy;
			new_entry->max_protection |= VM_PROT_WRITE;
		} else {
			new_entry->inheritance = inheritance;
			if (!vm_remap_legacy) {
				new_entry->protection = *cur_protection;
				new_entry->max_protection = *max_protection;
			}
		}
		VME_OFFSET_SET(new_entry, offset);

		  
                                                     
     
RestartCopy:
		if (!copy) {
			if (src_entry->used_for_jit == TRUE) {
				if (same_map) {
				} else if (!VM_MAP_POLICY_ALLOW_JIT_SHARING(map)) {
					  
                                              
                                                  
        
					result = KERN_INVALID_ARGUMENT;
					break;
				}
			}

			src_entry->is_shared = TRUE;
			new_entry->is_shared = TRUE;
			if (!(new_entry->is_sub_map)) {
				new_entry->needs_copy = FALSE;
			}
		} else if (src_entry->is_sub_map) {
			                                            
			assert(new_entry->wired_count == 0);
			new_entry->needs_copy = TRUE;
			object = VM_OBJECT_NULL;
		} else if (src_entry->wired_count == 0 &&
		    !(debug4k_no_cow_copyin && VM_MAP_PAGE_SHIFT(map) < PAGE_SHIFT) &&
		    vm_object_copy_quickly(VME_OBJECT_PTR(new_entry),
		    VME_OFFSET(new_entry),
		    (new_entry->vme_end -
		    new_entry->vme_start),
		    &src_needs_copy,
		    &new_entry_needs_copy)) {
			new_entry->needs_copy = new_entry_needs_copy;
			new_entry->is_shared = FALSE;
			assertf(new_entry->use_pmap, "map %p new_entry %p\n", map, new_entry);

			  
                                     
      
			if (src_needs_copy && !src_entry->needs_copy) {
				vm_prot_t prot;

				assert(!pmap_has_prot_policy(map->pmap, src_entry->translated_allow_execute, src_entry->protection));

				prot = src_entry->protection & ~VM_PROT_WRITE;

				if (override_nx(map,
				    VME_ALIAS(src_entry))
				    && prot) {
					prot |= VM_PROT_EXECUTE;
				}

				assert(!pmap_has_prot_policy(map->pmap, src_entry->translated_allow_execute, prot));

				vm_object_pmap_protect(object,
				    offset,
				    entry_size,
				    ((src_entry->is_shared
				    || map->mapped_in_other_pmaps) ?
				    PMAP_NULL : map->pmap),
				    VM_MAP_PAGE_SIZE(map),
				    src_entry->vme_start,
				    prot);

				assert(src_entry->wired_count == 0);
				src_entry->needs_copy = TRUE;
			}
			  
                                                           
      
			vm_object_deallocate(object);
		} else {
			new_entry->is_shared = FALSE;
			assertf(new_entry->use_pmap, "map %p new_entry %p\n", map, new_entry);

			src_entry_was_wired = (src_entry->wired_count > 0);
			saved_src_entry = src_entry;
			src_entry = VM_MAP_ENTRY_NULL;

			  
                                             
                                             
     
                                               
                                       
      
			version.main_timestamp = map->timestamp;
			vm_map_unlock(map);                                     

			  
                       
      
			if (src_entry_was_wired > 0 ||
			    (debug4k_no_cow_copyin &&
			    VM_MAP_PAGE_SHIFT(map) < PAGE_SHIFT)) {
				vm_object_lock(object);
				result = vm_object_copy_slowly(
					object,
					offset,
					(new_entry->vme_end -
					new_entry->vme_start),
					THREAD_UNINT,
					VME_OBJECT_PTR(new_entry));

				VME_OFFSET_SET(new_entry, offset - vm_object_trunc_page(offset));
				new_entry->needs_copy = FALSE;
			} else {
				vm_object_offset_t new_offset;

				new_offset = VME_OFFSET(new_entry);
				result = vm_object_copy_strategically(
					object,
					offset,
					(new_entry->vme_end -
					new_entry->vme_start),
					VME_OBJECT_PTR(new_entry),
					&new_offset,
					&new_entry_needs_copy);
				if (new_offset != VME_OFFSET(new_entry)) {
					VME_OFFSET_SET(new_entry, new_offset);
				}

				new_entry->needs_copy = new_entry_needs_copy;
			}

			  
                                                           
      
			vm_object_deallocate(object);

			if (result != KERN_SUCCESS &&
			    result != KERN_MEMORY_RESTART_COPY) {
				_vm_map_entry_dispose(map_header, new_entry);
				vm_map_lock(map);
				break;
			}

			  
                                               
                                            
      

			vm_map_lock(map);
			if (version.main_timestamp + 1 != map->timestamp) {
				  
                                        
      
                                           
                                            
       
				saved_src_entry = VM_MAP_ENTRY_NULL;
				vm_object_deallocate(VME_OBJECT(new_entry));
				_vm_map_entry_dispose(map_header, new_entry);
				if (result == KERN_MEMORY_RESTART_COPY) {
					result = KERN_SUCCESS;
				}
				continue;
			}
			                                                  
			src_entry = saved_src_entry;
			saved_src_entry = VM_MAP_ENTRY_NULL;

			if (result == KERN_MEMORY_RESTART_COPY) {
				vm_object_reference(object);
				goto RestartCopy;
			}
		}

		_vm_map_store_entry_link(map_header,
		    map_header->links.prev, new_entry);

		                                                        
		if (vm_remap_legacy && !src_entry->is_sub_map) {
			*cur_protection &= src_entry->protection;
			*max_protection &= src_entry->max_protection;
		}

		map_address += tmp_size;
		mapped_size += tmp_size;
		src_start += tmp_size;

		if (vmk_flags.vmkf_copy_single_object) {
			if (mapped_size != size) {
				DEBUG4K_SHARE("map %p addr 0x%llx size 0x%llx clipped copy at mapped_size 0x%llx\n", map, (uint64_t)addr, (uint64_t)size, (uint64_t)mapped_size);
				if (src_entry->vme_next != vm_map_to_entry(map) &&
				    VME_OBJECT(src_entry->vme_next) == VME_OBJECT(src_entry)) {
					                
					DEBUG4K_ERROR("could have extended copy to next entry...\n");
				}
			}
			break;
		}
	}                

	vm_map_unlock(map);
	if (result != KERN_SUCCESS) {
		  
                                 
     
		for (src_entry = map_header->links.next;
		    src_entry != CAST_TO_VM_MAP_ENTRY(&map_header->links);
		    src_entry = new_entry) {
			new_entry = src_entry->vme_next;
			_vm_map_store_entry_unlink(map_header, src_entry);
			if (src_entry->is_sub_map) {
				vm_map_deallocate(VME_SUBMAP(src_entry));
			} else {
				vm_object_deallocate(VME_OBJECT(src_entry));
			}
			_vm_map_entry_dispose(map_header, src_entry);
		}
	}
	return result;
}

bool
vm_map_is_exotic(
	vm_map_t map)
{
	return VM_MAP_IS_EXOTIC(map);
}

bool
vm_map_is_alien(
	vm_map_t map)
{
	return VM_MAP_IS_ALIEN(map);
}

#if XNU_TARGET_OS_OSX
void
vm_map_mark_alien(
	vm_map_t map)
{
	vm_map_lock(map);
	map->is_alien = true;
	vm_map_unlock(map);
}

void
vm_map_single_jit(
	vm_map_t map)
{
	vm_map_lock(map);
	map->single_jit = true;
	vm_map_unlock(map);
}
#endif                        

void vm_map_copy_to_physcopy(vm_map_copy_t copy_map, vm_map_t target_map);
void
vm_map_copy_to_physcopy(
	vm_map_copy_t   copy_map,
	vm_map_t        target_map)
{
	vm_map_size_t           size;
	vm_map_entry_t          entry;
	vm_map_entry_t          new_entry;
	vm_object_t             new_object;
	unsigned int            pmap_flags;
	pmap_t                  new_pmap;
	vm_map_t                new_map;
	vm_map_address_t        src_start, src_end, src_cur;
	vm_map_address_t        dst_start, dst_end, dst_cur;
	kern_return_t           kr;
	void                    *kbuf;

	  
                                                         
                                                                        
    
	DEBUG4K_COPY("copy_map %p (%d %d 0x%llx 0x%llx) BEFORE\n", copy_map, copy_map->cpy_hdr.page_shift, copy_map->cpy_hdr.nentries, copy_map->offset, (uint64_t)copy_map->size);

	assert(copy_map->cpy_hdr.page_shift != VM_MAP_PAGE_MASK(target_map));

	                            
	size = VM_MAP_ROUND_PAGE(copy_map->size, PAGE_MASK);
	new_object = vm_object_allocate(size);
	assert(new_object);

	                               
	new_entry = vm_map_copy_entry_create(copy_map, FALSE);
	assert(new_entry);

	                                          
	new_entry->protection = VM_PROT_DEFAULT;
	new_entry->max_protection = VM_PROT_DEFAULT;
	new_entry->use_pmap = TRUE;

	                                                  
	new_entry->vme_start = 0;
	new_entry->vme_end = size;
	VME_OBJECT_SET(new_entry, new_object);
	VME_OFFSET_SET(new_entry, 0);

	                                         
	pmap_flags = 0;
	assert(copy_map->cpy_hdr.page_shift == FOURK_PAGE_SHIFT);
#if PMAP_CREATE_FORCE_4K_PAGES
	pmap_flags |= PMAP_CREATE_FORCE_4K_PAGES;
#endif                                 
	pmap_flags |= PMAP_CREATE_64BIT;
	new_pmap = pmap_create_options(NULL, (vm_map_size_t)0, pmap_flags);
	assert(new_pmap);

	                                                    
	new_map = vm_map_create(new_pmap, 0, MACH_VM_MAX_ADDRESS, TRUE);
	assert(new_map);
	vm_map_set_page_shift(new_map, copy_map->cpy_hdr.page_shift);

	                                      
	src_start = 0;
	kr = vm_map_copyout_internal(
		new_map,
		&src_start,
		copy_map,
		copy_map->size,
		FALSE,                         
		VM_PROT_DEFAULT,
		VM_PROT_DEFAULT,
		VM_INHERIT_DEFAULT);
	assert(kr == KERN_SUCCESS);
	src_end = src_start + copy_map->size;

	                                        
	vm_object_reference(new_object);
	dst_start = 0;
	kr = vm_map_enter(new_map,
	    &dst_start,
	    size,
	    0,                         
	    VM_FLAGS_ANYWHERE,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    VM_KERN_MEMORY_OSFMK,
	    new_object,
	    0,                           
	    FALSE,                               
	    VM_PROT_DEFAULT,
	    VM_PROT_DEFAULT,
	    VM_INHERIT_DEFAULT);
	assert(kr == KERN_SUCCESS);
	dst_end = dst_start + size;

	                         
	kbuf = kheap_alloc(KHEAP_TEMP, PAGE_SIZE, Z_WAITOK);
	assert(kbuf);

	                                                          
	for (src_cur = src_start, dst_cur = dst_start;
	    src_cur < src_end;
	    src_cur += PAGE_SIZE, dst_cur += PAGE_SIZE) {
		vm_size_t bytes;

		bytes = PAGE_SIZE;
		if (src_cur + PAGE_SIZE > src_end) {
			                                
			bytes = src_end - src_cur;
			assert(bytes > 0 && bytes < PAGE_SIZE);
			                                            
		}
		                                
		kr = copyinmap(new_map, src_cur, kbuf, bytes);
		if (kr != KERN_SUCCESS) {
			DEBUG4K_COPY("copyinmap(%p, 0x%llx, %p, 0x%llx) kr 0x%x\n", new_map, (uint64_t)src_cur, kbuf, (uint64_t)bytes, kr);
		}
		                              
		assert(dst_cur < dst_end);
		assert(dst_cur + bytes <= dst_end);
		kr = copyoutmap(new_map, kbuf, dst_cur, bytes);
		if (kr != KERN_SUCCESS) {
			DEBUG4K_COPY("copyoutmap(%p, %p, 0x%llx, 0x%llx) kr 0x%x\n", new_map, kbuf, (uint64_t)dst_cur, (uint64_t)bytes, kr);
		}
	}

	                        
	kheap_free(KHEAP_TEMP, kbuf, PAGE_SIZE);
	kbuf = NULL;

	                     
	vm_map_destroy(new_map, VM_MAP_REMOVE_NO_FLAGS);
	new_map = VM_MAP_NULL;

	                                                  
	while (vm_map_copy_first_entry(copy_map) !=
	    vm_map_copy_to_entry(copy_map)) {
		entry = vm_map_copy_first_entry(copy_map);
		vm_map_copy_entry_unlink(copy_map, entry);
		if (entry->is_sub_map) {
			vm_map_deallocate(VME_SUBMAP(entry));
		} else {
			vm_object_deallocate(VME_OBJECT(entry));
		}
		vm_map_copy_entry_dispose(copy_map, entry);
	}

	                                                         
	copy_map->cpy_hdr.page_shift = VM_MAP_PAGE_SHIFT(target_map);
	copy_map->offset = 0;
	copy_map->size = size;

	                                        
	assert(vm_map_copy_last_entry(copy_map) == vm_map_copy_to_entry(copy_map));
	vm_map_copy_entry_link(copy_map, vm_map_copy_last_entry(copy_map), new_entry);

	DEBUG4K_COPY("copy_map %p (%d %d 0x%llx 0x%llx) AFTER\n", copy_map, copy_map->cpy_hdr.page_shift, copy_map->cpy_hdr.nentries, copy_map->offset, (uint64_t)copy_map->size);
}

void
vm_map_copy_adjust_get_target_copy_map(
	vm_map_copy_t   copy_map,
	vm_map_copy_t   *target_copy_map_p);
void
vm_map_copy_adjust_get_target_copy_map(
	vm_map_copy_t   copy_map,
	vm_map_copy_t   *target_copy_map_p)
{
	vm_map_copy_t   target_copy_map;
	vm_map_entry_t  entry, target_entry;

	if (*target_copy_map_p != VM_MAP_COPY_NULL) {
		                                                        
		return;
	}

	                                                            
	target_copy_map = vm_map_copy_allocate();
	target_copy_map->type = copy_map->type;
	assert(target_copy_map->type == VM_MAP_COPY_ENTRY_LIST);
	target_copy_map->offset = copy_map->offset;
	target_copy_map->size = copy_map->size;
	target_copy_map->cpy_hdr.page_shift = copy_map->cpy_hdr.page_shift;
	vm_map_store_init(&target_copy_map->cpy_hdr);
	for (entry = vm_map_copy_first_entry(copy_map);
	    entry != vm_map_copy_to_entry(copy_map);
	    entry = entry->vme_next) {
		target_entry = vm_map_copy_entry_create(target_copy_map, FALSE);
		vm_map_entry_copy_full(target_entry, entry);
		if (target_entry->is_sub_map) {
			vm_map_reference(VME_SUBMAP(target_entry));
		} else {
			vm_object_reference(VME_OBJECT(target_entry));
		}
		vm_map_copy_entry_link(
			target_copy_map,
			vm_map_copy_last_entry(target_copy_map),
			target_entry);
	}
	entry = VM_MAP_ENTRY_NULL;
	*target_copy_map_p = target_copy_map;
}

void
vm_map_copy_trim(
	vm_map_copy_t   copy_map,
	int             new_page_shift,
	vm_map_offset_t trim_start,
	vm_map_offset_t trim_end);
void
vm_map_copy_trim(
	vm_map_copy_t   copy_map,
	int             new_page_shift,
	vm_map_offset_t trim_start,
	vm_map_offset_t trim_end)
{
	int             copy_page_shift;
	vm_map_entry_t  entry, next_entry;

	assert(copy_map->type == VM_MAP_COPY_ENTRY_LIST);
	assert(copy_map->cpy_hdr.nentries > 0);

	trim_start += vm_map_copy_first_entry(copy_map)->vme_start;
	trim_end += vm_map_copy_first_entry(copy_map)->vme_start;

	                                               
	copy_page_shift = VM_MAP_COPY_PAGE_SHIFT(copy_map);
	copy_map->cpy_hdr.page_shift = new_page_shift;

	for (entry = vm_map_copy_first_entry(copy_map);
	    entry != vm_map_copy_to_entry(copy_map);
	    entry = next_entry) {
		next_entry = entry->vme_next;
		if (entry->vme_end <= trim_start) {
			                                         
			continue;
		}
		if (entry->vme_start >= trim_end) {
			                                        
			break;
		}
		                          
		vm_map_copy_clip_start(copy_map, entry, trim_start);
		vm_map_copy_clip_end(copy_map, entry, trim_end);
		                      
		copy_map->size -= entry->vme_end - entry->vme_start;
		vm_map_copy_entry_unlink(copy_map, entry);
		if (entry->is_sub_map) {
			vm_map_deallocate(VME_SUBMAP(entry));
		} else {
			vm_object_deallocate(VME_OBJECT(entry));
		}
		vm_map_copy_entry_dispose(copy_map, entry);
		entry = VM_MAP_ENTRY_NULL;
	}

	                                            
	copy_map->cpy_hdr.page_shift = copy_page_shift;
}

  
                                                                 
                            
                                                                
                        
                                                                       
                                                                     
                                                                  
                                                                      
                           
   
                                                                        
kern_return_t
vm_map_copy_adjust_to_target(
	vm_map_copy_t           src_copy_map,
	vm_map_offset_t         offset,
	vm_map_size_t           size,
	vm_map_t                target_map,
	boolean_t               copy,
	vm_map_copy_t           *target_copy_map_p,
	vm_map_offset_t         *overmap_start_p,
	vm_map_offset_t         *overmap_end_p,
	vm_map_offset_t         *trimmed_start_p)
{
	vm_map_copy_t           copy_map, target_copy_map;
	vm_map_size_t           target_size;
	vm_map_size_t           src_copy_map_size;
	vm_map_size_t           overmap_start, overmap_end;
	int                     misalignments;
	vm_map_entry_t          entry, target_entry;
	vm_map_offset_t         addr_adjustment;
	vm_map_offset_t         new_start, new_end;
	int                     copy_page_mask, target_page_mask;
	int                     copy_page_shift, target_page_shift;
	vm_map_offset_t         trimmed_end;

	  
                                                        
                               
    
	vm_map_copy_require(src_copy_map);
	assert(src_copy_map->type == VM_MAP_COPY_ENTRY_LIST);

	  
                                                      
                                                                
    
	copy_map = src_copy_map;
	src_copy_map_size = src_copy_map->size;

	copy_page_shift = VM_MAP_COPY_PAGE_SHIFT(copy_map);
	copy_page_mask = VM_MAP_COPY_PAGE_MASK(copy_map);
	target_page_shift = VM_MAP_PAGE_SHIFT(target_map);
	target_page_mask = VM_MAP_PAGE_MASK(target_map);

	DEBUG4K_ADJUST("copy_map %p (%d offset 0x%llx size 0x%llx) target_map %p (%d) copy %d offset 0x%llx size 0x%llx target_copy_map %p...\n", copy_map, copy_page_shift, (uint64_t)copy_map->offset, (uint64_t)copy_map->size, target_map, target_page_shift, copy, (uint64_t)offset, (uint64_t)size, *target_copy_map_p);

	target_copy_map = *target_copy_map_p;
	if (target_copy_map != VM_MAP_COPY_NULL) {
		vm_map_copy_require(target_copy_map);
	}

	if (offset + size > copy_map->size) {
		DEBUG4K_ERROR("copy_map %p (%d->%d) copy_map->size 0x%llx offset 0x%llx size 0x%llx KERN_INVALID_ARGUMENT\n", copy_map, copy_page_shift, target_page_shift, (uint64_t)copy_map->size, (uint64_t)offset, (uint64_t)size);
		return KERN_INVALID_ARGUMENT;
	}

	                  
	trimmed_end = 0;
	new_end = VM_MAP_ROUND_PAGE(offset + size, target_page_mask);
	if (new_end < copy_map->size) {
		trimmed_end = src_copy_map_size - new_end;
		DEBUG4K_ADJUST("copy_map %p (%d->%d) copy %d offset 0x%llx size 0x%llx target_copy_map %p... trim end from 0x%llx to 0x%llx\n", copy_map, copy_page_shift, target_page_shift, copy, (uint64_t)offset, (uint64_t)size, target_copy_map, (uint64_t)new_end, (uint64_t)copy_map->size);
		                                                   
		vm_map_copy_adjust_get_target_copy_map(copy_map,
		    &target_copy_map);
		copy_map = target_copy_map;
		vm_map_copy_trim(target_copy_map, target_page_shift,
		    new_end, copy_map->size);
	}

	                    
	new_start = VM_MAP_TRUNC_PAGE(offset, target_page_mask);
	if (new_start != 0) {
		DEBUG4K_ADJUST("copy_map %p (%d->%d) copy %d offset 0x%llx size 0x%llx target_copy_map %p... trim start from 0x%llx to 0x%llx\n", copy_map, copy_page_shift, target_page_shift, copy, (uint64_t)offset, (uint64_t)size, target_copy_map, (uint64_t)0, (uint64_t)new_start);
		                                                   
		vm_map_copy_adjust_get_target_copy_map(copy_map,
		    &target_copy_map);
		copy_map = target_copy_map;
		vm_map_copy_trim(target_copy_map, target_page_shift,
		    0, new_start);
	}
	*trimmed_start_p = new_start;

	                                                        
	target_size = copy_map->size;
	assertf(target_size == src_copy_map_size - *trimmed_start_p - trimmed_end,
	    "target_size 0x%llx src_copy_map_size 0x%llx trimmed_start 0x%llx trimmed_end 0x%llx\n",
	    (uint64_t)target_size, (uint64_t)src_copy_map_size,
	    (uint64_t)*trimmed_start_p, (uint64_t)trimmed_end);

	                                                  
	misalignments = 0;
	overmap_start = 0;
	overmap_end = 0;
	if (copy_page_shift < target_page_shift) {
		  
                                                             
                          
                                                              
                                                             
                                                
                                                    
                   
                                                               
              
                                             
     
		for (entry = vm_map_copy_first_entry(copy_map);
		    entry != vm_map_copy_to_entry(copy_map);
		    entry = entry->vme_next) {
			vm_object_offset_t object_offset_start, object_offset_end;

			object_offset_start = VME_OFFSET(entry);
			object_offset_end = object_offset_start;
			object_offset_end += entry->vme_end - entry->vme_start;
			if (object_offset_start & target_page_mask) {
				if (entry == vm_map_copy_first_entry(copy_map) && !copy) {
					overmap_start++;
				} else {
					misalignments++;
				}
			}
			if (object_offset_end & target_page_mask) {
				if (entry->vme_next == vm_map_copy_to_entry(copy_map) && !copy) {
					overmap_end++;
				} else {
					misalignments++;
				}
			}
		}
	}
	entry = VM_MAP_ENTRY_NULL;

	                                           
	assert(overmap_start <= 1);
	assert(overmap_end <= 1);
	if (!overmap_start && !overmap_end && !misalignments) {
		                                                     
		if (*trimmed_start_p) {
			                                                    
		} else {
			                                                 
			if (target_copy_map == VM_MAP_COPY_NULL) {
				target_copy_map = copy_map;
			}
			*target_copy_map_p = target_copy_map;
			*overmap_start_p = 0;
			*overmap_end_p = 0;
			DEBUG4K_ADJUST("copy_map %p (%d offset 0x%llx size 0x%llx) target_map %p (%d) copy %d target_copy_map %p (%d offset 0x%llx size 0x%llx) -> trimmed 0x%llx overmap start 0x%llx end 0x%llx KERN_SUCCESS\n", copy_map, copy_page_shift, (uint64_t)copy_map->offset, (uint64_t)copy_map->size, target_map, target_page_shift, copy, *target_copy_map_p, VM_MAP_COPY_PAGE_SHIFT(*target_copy_map_p), (uint64_t)(*target_copy_map_p)->offset, (uint64_t)(*target_copy_map_p)->size, (uint64_t)*trimmed_start_p, (uint64_t)*overmap_start_p, (uint64_t)*overmap_end_p);
			return KERN_SUCCESS;
		}
	} else if (misalignments && !copy) {
		                                 
		DEBUG4K_ADJUST("unsupported sharing\n");
#if MACH_ASSERT
		if (debug4k_panic_on_misaligned_sharing) {
			panic("DEBUG4k %s:%d unsupported sharing\n", __FUNCTION__, __LINE__);
		}
#endif                  
		DEBUG4K_ADJUST("copy_map %p (%d) target_map %p (%d) copy %d target_copy_map %p -> KERN_NOT_SUPPORTED\n", copy_map, copy_page_shift, target_map, target_page_shift, copy, *target_copy_map_p);
		return KERN_NOT_SUPPORTED;
	} else {
		                                                              
		DEBUG4K_ADJUST("mis-aligned copying\n");
	}

	                                                        
	vm_map_copy_adjust_get_target_copy_map(copy_map, &target_copy_map);
	copy_map = target_copy_map;

	if (misalignments && copy) {
		vm_map_size_t target_copy_map_size;

		  
                                                     
                                                              
                        
     
		target_copy_map_size = target_copy_map->size;
		vm_map_copy_to_physcopy(target_copy_map, target_map);
		*target_copy_map_p = target_copy_map;
		*overmap_start_p = 0;
		*overmap_end_p = target_copy_map->size - target_copy_map_size;
		DEBUG4K_ADJUST("copy_map %p (%d offset 0x%llx size 0x%llx) target_map %p (%d) copy %d target_copy_map %p (%d offset 0x%llx size 0x%llx)-> trimmed 0x%llx overmap start 0x%llx end 0x%llx PHYSCOPY\n", copy_map, copy_page_shift, (uint64_t)copy_map->offset, (uint64_t)copy_map->size, target_map, target_page_shift, copy, *target_copy_map_p, VM_MAP_COPY_PAGE_SHIFT(*target_copy_map_p), (uint64_t)(*target_copy_map_p)->offset, (uint64_t)(*target_copy_map_p)->size, (uint64_t)*trimmed_start_p, (uint64_t)*overmap_start_p, (uint64_t)*overmap_end_p);
		return KERN_SUCCESS;
	}

	                           
	misalignments = 0;
	overmap_start = 0;
	overmap_end = 0;
	                                                                    
	addr_adjustment = copy_map->offset;
	                                                    
	addr_adjustment += *trimmed_start_p;
	for (target_entry = vm_map_copy_first_entry(target_copy_map);
	    target_entry != vm_map_copy_to_entry(target_copy_map);
	    target_entry = target_entry->vme_next) {
		vm_object_offset_t object_offset_start, object_offset_end;

		DEBUG4K_ADJUST("copy %p (%d 0x%llx 0x%llx) entry %p [ 0x%llx 0x%llx ] object %p offset 0x%llx BEFORE\n", target_copy_map, VM_MAP_COPY_PAGE_SHIFT(target_copy_map), target_copy_map->offset, (uint64_t)target_copy_map->size, target_entry, (uint64_t)target_entry->vme_start, (uint64_t)target_entry->vme_end, VME_OBJECT(target_entry), VME_OFFSET(target_entry));
		object_offset_start = VME_OFFSET(target_entry);
		if (object_offset_start & target_page_mask) {
			DEBUG4K_ADJUST("copy %p (%d 0x%llx 0x%llx) entry %p [ 0x%llx 0x%llx ] object %p offset 0x%llx misaligned at start\n", target_copy_map, VM_MAP_COPY_PAGE_SHIFT(target_copy_map), target_copy_map->offset, (uint64_t)target_copy_map->size, target_entry, (uint64_t)target_entry->vme_start, (uint64_t)target_entry->vme_end, VME_OBJECT(target_entry), VME_OFFSET(target_entry));
			if (target_entry == vm_map_copy_first_entry(target_copy_map)) {
				  
                                         
                                 
       
				overmap_start = object_offset_start - trunc_page_mask_64(object_offset_start, target_page_mask);
				DEBUG4K_ADJUST("entry %p offset 0x%llx copy %d -> overmap_start 0x%llx\n", target_entry, VME_OFFSET(target_entry), copy, (uint64_t)overmap_start);
				VME_OFFSET_SET(target_entry, VME_OFFSET(target_entry) - overmap_start);
			} else {
				misalignments++;
				DEBUG4K_ADJUST("entry %p offset 0x%llx copy %d -> misalignments %d\n", target_entry, VME_OFFSET(target_entry), copy, misalignments);
				assert(copy);
			}
		}

		if (target_entry == vm_map_copy_first_entry(target_copy_map)) {
			target_size += overmap_start;
		} else {
			target_entry->vme_start += overmap_start;
		}
		target_entry->vme_end += overmap_start;

		object_offset_end = VME_OFFSET(target_entry) + target_entry->vme_end - target_entry->vme_start;
		if (object_offset_end & target_page_mask) {
			DEBUG4K_ADJUST("copy %p (%d 0x%llx 0x%llx) entry %p [ 0x%llx 0x%llx ] object %p offset 0x%llx misaligned at end\n", target_copy_map, VM_MAP_COPY_PAGE_SHIFT(target_copy_map), target_copy_map->offset, (uint64_t)target_copy_map->size, target_entry, (uint64_t)target_entry->vme_start, (uint64_t)target_entry->vme_end, VME_OBJECT(target_entry), VME_OFFSET(target_entry));
			if (target_entry->vme_next == vm_map_copy_to_entry(target_copy_map)) {
				  
                                                                   
       
				overmap_end = round_page_mask_64(object_offset_end, target_page_mask) - object_offset_end;
				DEBUG4K_ADJUST("entry %p offset 0x%llx copy %d -> overmap_end 0x%llx\n", target_entry, VME_OFFSET(target_entry), copy, (uint64_t)overmap_end);
				target_entry->vme_end += overmap_end;
				target_size += overmap_end;
			} else {
				misalignments++;
				DEBUG4K_ADJUST("entry %p offset 0x%llx copy %d -> misalignments %d\n", target_entry, VME_OFFSET(target_entry), copy, misalignments);
				assert(copy);
			}
		}
		target_entry->vme_start -= addr_adjustment;
		target_entry->vme_end -= addr_adjustment;
		DEBUG4K_ADJUST("copy %p (%d 0x%llx 0x%llx) entry %p [ 0x%llx 0x%llx ] object %p offset 0x%llx AFTER\n", target_copy_map, VM_MAP_COPY_PAGE_SHIFT(target_copy_map), target_copy_map->offset, (uint64_t)target_copy_map->size, target_entry, (uint64_t)target_entry->vme_start, (uint64_t)target_entry->vme_end, VME_OBJECT(target_entry), VME_OFFSET(target_entry));
	}

	target_copy_map->size = target_size;
	target_copy_map->offset += overmap_start;
	target_copy_map->offset -= addr_adjustment;
	target_copy_map->cpy_hdr.page_shift = target_page_shift;

                                                                        
                                                                         
	assert(overmap_start < VM_MAP_PAGE_SIZE(target_map));
	assert(overmap_end < VM_MAP_PAGE_SIZE(target_map));

	*target_copy_map_p = target_copy_map;
	*overmap_start_p = overmap_start;
	*overmap_end_p = overmap_end;

	DEBUG4K_ADJUST("copy_map %p (%d offset 0x%llx size 0x%llx) target_map %p (%d) copy %d target_copy_map %p (%d offset 0x%llx size 0x%llx) -> trimmed 0x%llx overmap start 0x%llx end 0x%llx KERN_SUCCESS\n", copy_map, copy_page_shift, (uint64_t)copy_map->offset, (uint64_t)copy_map->size, target_map, target_page_shift, copy, *target_copy_map_p, VM_MAP_COPY_PAGE_SHIFT(*target_copy_map_p), (uint64_t)(*target_copy_map_p)->offset, (uint64_t)(*target_copy_map_p)->size, (uint64_t)*trimmed_start_p, (uint64_t)*overmap_start_p, (uint64_t)*overmap_end_p);
	return KERN_SUCCESS;
}

kern_return_t
vm_map_range_physical_size(
	vm_map_t         map,
	vm_map_address_t start,
	mach_vm_size_t   size,
	mach_vm_size_t * phys_size)
{
	kern_return_t   kr;
	vm_map_copy_t   copy_map, target_copy_map;
	vm_map_offset_t adjusted_start, adjusted_end;
	vm_map_size_t   adjusted_size;
	vm_prot_t       cur_prot, max_prot;
	vm_map_offset_t overmap_start, overmap_end, trimmed_start;
	vm_map_kernel_flags_t vmk_flags;

	adjusted_start = vm_map_trunc_page(start, VM_MAP_PAGE_MASK(map));
	adjusted_end = vm_map_round_page(start + size, VM_MAP_PAGE_MASK(map));
	adjusted_size = adjusted_end - adjusted_start;
	*phys_size = adjusted_size;
	if (VM_MAP_PAGE_SIZE(map) == PAGE_SIZE) {
		return KERN_SUCCESS;
	}
	if (start == 0) {
		adjusted_start = vm_map_trunc_page(start, PAGE_MASK);
		adjusted_end = vm_map_round_page(start + size, PAGE_MASK);
		adjusted_size = adjusted_end - adjusted_start;
		*phys_size = adjusted_size;
		return KERN_SUCCESS;
	}
	if (adjusted_size == 0) {
		DEBUG4K_SHARE("map %p start 0x%llx size 0x%llx adjusted 0x%llx -> phys_size 0!\n", map, (uint64_t)start, (uint64_t)size, (uint64_t)adjusted_size);
		*phys_size = 0;
		return KERN_SUCCESS;
	}

	vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
	vmk_flags.vmkf_copy_pageable = TRUE;
	vmk_flags.vmkf_copy_same_map = TRUE;
	assert(adjusted_size != 0);
	cur_prot = VM_PROT_NONE;                  
	max_prot = VM_PROT_NONE;                  
	kr = vm_map_copy_extract(map, adjusted_start, adjusted_size,
	    FALSE           ,
	    &copy_map,
	    &cur_prot, &max_prot, VM_INHERIT_DEFAULT,
	    vmk_flags);
	if (kr != KERN_SUCCESS) {
		DEBUG4K_ERROR("map %p start 0x%llx 0x%llx size 0x%llx 0x%llx kr 0x%x\n", map, (uint64_t)start, (uint64_t)adjusted_start, size, (uint64_t)adjusted_size, kr);
		            
		*phys_size = 0;
		return kr;
	}
	assert(copy_map != VM_MAP_COPY_NULL);
	target_copy_map = copy_map;
	DEBUG4K_ADJUST("adjusting...\n");
	kr = vm_map_copy_adjust_to_target(
		copy_map,
		start - adjusted_start,             
		size,           
		kernel_map,
		FALSE,                                    
		&target_copy_map,
		&overmap_start,
		&overmap_end,
		&trimmed_start);
	if (kr == KERN_SUCCESS) {
		if (target_copy_map->size != *phys_size) {
			DEBUG4K_ADJUST("map %p (%d) start 0x%llx size 0x%llx adjusted_start 0x%llx adjusted_end 0x%llx overmap_start 0x%llx overmap_end 0x%llx trimmed_start 0x%llx phys_size 0x%llx -> 0x%llx\n", map, VM_MAP_PAGE_SHIFT(map), (uint64_t)start, (uint64_t)size, (uint64_t)adjusted_start, (uint64_t)adjusted_end, (uint64_t)overmap_start, (uint64_t)overmap_end, (uint64_t)trimmed_start, (uint64_t)*phys_size, (uint64_t)target_copy_map->size);
		}
		*phys_size = target_copy_map->size;
	} else {
		DEBUG4K_ERROR("map %p start 0x%llx 0x%llx size 0x%llx 0x%llx kr 0x%x\n", map, (uint64_t)start, (uint64_t)adjusted_start, size, (uint64_t)adjusted_size, kr);
		            
		*phys_size = 0;
	}
	vm_map_copy_discard(copy_map);
	copy_map = VM_MAP_COPY_NULL;

	return kr;
}


kern_return_t
memory_entry_check_for_adjustment(
	vm_map_t                        src_map,
	ipc_port_t                      port,
	vm_map_offset_t         *overmap_start,
	vm_map_offset_t         *overmap_end)
{
	kern_return_t kr = KERN_SUCCESS;
	vm_map_copy_t copy_map = VM_MAP_COPY_NULL, target_copy_map = VM_MAP_COPY_NULL;

	assert(port);
	assertf(ip_kotype(port) == IKOT_NAMED_ENTRY, "Port Type expected: %d...received:%d\n", IKOT_NAMED_ENTRY, ip_kotype(port));

	vm_named_entry_t        named_entry;

	named_entry = (vm_named_entry_t) ipc_kobject_get(port);
	named_entry_lock(named_entry);
	copy_map = named_entry->backing.copy;
	target_copy_map = copy_map;

	if (src_map && VM_MAP_PAGE_SHIFT(src_map) < PAGE_SHIFT) {
		vm_map_offset_t trimmed_start;

		trimmed_start = 0;
		DEBUG4K_ADJUST("adjusting...\n");
		kr = vm_map_copy_adjust_to_target(
			copy_map,
			0,             
			copy_map->size,           
			src_map,
			FALSE,           
			&target_copy_map,
			overmap_start,
			overmap_end,
			&trimmed_start);
		assert(trimmed_start == 0);
	}
	named_entry_unlock(named_entry);

	return kr;
}


  
                    
  
                                           
                                             
                                          
                                           
                                                    
                                            
                                                   
   
kern_return_t
vm_map_remap(
	vm_map_t                target_map,
	vm_map_address_t        *address,
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	vm_map_t                src_map,
	vm_map_offset_t         memory_address,
	boolean_t               copy,
	vm_prot_t               *cur_protection,             
	vm_prot_t               *max_protection,             
	vm_inherit_t            inheritance)
{
	kern_return_t           result;
	vm_map_entry_t          entry;
	vm_map_entry_t          insp_entry = VM_MAP_ENTRY_NULL;
	vm_map_entry_t          new_entry;
	vm_map_copy_t           copy_map;
	vm_map_offset_t         offset_in_mapping;
	vm_map_size_t           target_size = 0;
	vm_map_size_t           src_page_mask, target_page_mask;
	vm_map_offset_t         overmap_start, overmap_end, trimmed_start;
	vm_map_offset_t         initial_memory_address;
	vm_map_size_t           initial_size;

	if (target_map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	initial_memory_address = memory_address;
	initial_size = size;
	src_page_mask = VM_MAP_PAGE_MASK(src_map);
	target_page_mask = VM_MAP_PAGE_MASK(target_map);

	switch (inheritance) {
	case VM_INHERIT_NONE:
	case VM_INHERIT_COPY:
	case VM_INHERIT_SHARE:
		if (size != 0 && src_map != VM_MAP_NULL) {
			break;
		}
		OS_FALLTHROUGH;
	default:
		return KERN_INVALID_ARGUMENT;
	}

	if (src_page_mask != target_page_mask) {
		if (copy) {
			DEBUG4K_COPY("src_map %p pgsz 0x%x addr 0x%llx size 0x%llx copy %d -> target_map %p pgsz 0x%x\n", src_map, VM_MAP_PAGE_SIZE(src_map), (uint64_t)memory_address, (uint64_t)size, copy, target_map, VM_MAP_PAGE_SIZE(target_map));
		} else {
			DEBUG4K_SHARE("src_map %p pgsz 0x%x addr 0x%llx size 0x%llx copy %d -> target_map %p pgsz 0x%x\n", src_map, VM_MAP_PAGE_SIZE(src_map), (uint64_t)memory_address, (uint64_t)size, copy, target_map, VM_MAP_PAGE_SIZE(target_map));
		}
	}

	  
                                                               
                                                              
                                                           
                                                            
                                                                        
                                                               
                                                                
                                     
   
                                                          
   
                                                     
   
                                                                      
                                                                     
                                                                             
                                                                          
                                                  
    
	if ((flags & VM_FLAGS_RETURN_DATA_ADDR) != 0) {
		vm_map_offset_t range_start, range_end;

		range_start = vm_map_trunc_page(memory_address, src_page_mask);
		range_end = vm_map_round_page(memory_address + size, src_page_mask);
		memory_address = range_start;
		size = range_end - range_start;
		offset_in_mapping = initial_memory_address - memory_address;
	} else {
		  
               
                                                             
                                                               
                                                       
                                                          
                                                           
                                             
                                                       
                                                           
     
		offset_in_mapping = 0;
		memory_address = vm_map_trunc_page(memory_address, src_page_mask);
		size = vm_map_round_page(size, src_page_mask);
		initial_memory_address = memory_address;
		initial_size = size;
	}


	if (size == 0) {
		return KERN_INVALID_ARGUMENT;
	}

	if (flags & VM_FLAGS_RESILIENT_MEDIA) {
		                                                   
		if (!copy) {
			return KERN_INVALID_ARGUMENT;
		}
	}

	vmk_flags.vmkf_copy_pageable = target_map->hdr.entries_pageable;
	vmk_flags.vmkf_copy_same_map = (src_map == target_map);

	assert(size != 0);
	result = vm_map_copy_extract(src_map,
	    memory_address,
	    size,
	    copy, &copy_map,
	    cur_protection,             
	    max_protection,             
	    inheritance,
	    vmk_flags);
	if (result != KERN_SUCCESS) {
		return result;
	}
	assert(copy_map != VM_MAP_COPY_NULL);

	overmap_start = 0;
	overmap_end = 0;
	trimmed_start = 0;
	target_size = size;
	if (src_page_mask != target_page_mask) {
		vm_map_copy_t target_copy_map;

		target_copy_map = copy_map;                                   
		DEBUG4K_ADJUST("adjusting...\n");
		result = vm_map_copy_adjust_to_target(
			copy_map,
			offset_in_mapping,             
			initial_size,
			target_map,
			copy,
			&target_copy_map,
			&overmap_start,
			&overmap_end,
			&trimmed_start);
		if (result != KERN_SUCCESS) {
			DEBUG4K_COPY("failed to adjust 0x%x\n", result);
			vm_map_copy_discard(copy_map);
			return result;
		}
		if (trimmed_start == 0) {
			                                           
		} else if (trimmed_start >= offset_in_mapping) {
			                                                       
			assert(overmap_start == 0);
			assert(overmap_end == 0);
			offset_in_mapping = 0;
		} else {
			                                               
			assert(overmap_start == 0);
			assert(overmap_end == 0);
			offset_in_mapping -= trimmed_start;
		}
		offset_in_mapping += overmap_start;
		target_size = target_copy_map->size;
	}

	  
                                                  
                        
    
	*address = vm_map_trunc_page(*address, target_page_mask);
	vm_map_lock(target_map);
	target_size = vm_map_round_page(target_size, target_page_mask);
	result = vm_map_remap_range_allocate(target_map, address,
	    target_size,
	    mask, flags, vmk_flags, tag,
	    &insp_entry);

	for (entry = vm_map_copy_first_entry(copy_map);
	    entry != vm_map_copy_to_entry(copy_map);
	    entry = new_entry) {
		new_entry = entry->vme_next;
		vm_map_copy_entry_unlink(copy_map, entry);
		if (result == KERN_SUCCESS) {
			if (flags & VM_FLAGS_RESILIENT_CODESIGN) {
				                                        
				entry->max_protection = VM_PROT_READ;
				entry->protection = VM_PROT_READ;
				entry->vme_resilient_codesign = TRUE;
			}
			entry->vme_start += *address;
			entry->vme_end += *address;
			assert(!entry->map_aligned);
			if ((flags & VM_FLAGS_RESILIENT_MEDIA) &&
			    !entry->is_sub_map &&
			    (VME_OBJECT(entry) == VM_OBJECT_NULL ||
			    VME_OBJECT(entry)->internal)) {
				entry->vme_resilient_media = TRUE;
			}
			assert(VM_MAP_PAGE_ALIGNED(entry->vme_start, MIN(target_page_mask, PAGE_MASK)));
			assert(VM_MAP_PAGE_ALIGNED(entry->vme_end, MIN(target_page_mask, PAGE_MASK)));
			assert(VM_MAP_PAGE_ALIGNED(VME_OFFSET(entry), MIN(target_page_mask, PAGE_MASK)));
			vm_map_store_entry_link(target_map, insp_entry, entry,
			    vmk_flags);
			insp_entry = entry;
		} else {
			if (!entry->is_sub_map) {
				vm_object_deallocate(VME_OBJECT(entry));
			} else {
				vm_map_deallocate(VME_SUBMAP(entry));
			}
			vm_map_copy_entry_dispose(copy_map, entry);
		}
	}

	if (flags & VM_FLAGS_RESILIENT_CODESIGN) {
		*cur_protection = VM_PROT_READ;
		*max_protection = VM_PROT_READ;
	}

	if (target_map->disable_vmentry_reuse == TRUE) {
		assert(!target_map->is_nested_map);
		if (target_map->highest_entry_end < insp_entry->vme_end) {
			target_map->highest_entry_end = insp_entry->vme_end;
		}
	}

	if (result == KERN_SUCCESS) {
		target_map->size += target_size;
		SAVE_HINT_MAP_WRITE(target_map, insp_entry);

	}
	vm_map_unlock(target_map);

	if (result == KERN_SUCCESS && target_map->wiring_required) {
		result = vm_map_wire_kernel(target_map, *address,
		    *address + size, *cur_protection, VM_KERN_MEMORY_MLOCK,
		    TRUE);
	}

	  
                                                                  
                                                        
    
	if ((flags & VM_FLAGS_RETURN_DATA_ADDR) != 0) {
		*address += offset_in_mapping;
	}

	if (src_page_mask != target_page_mask) {
		DEBUG4K_SHARE("vm_remap(%p 0x%llx 0x%llx copy=%d-> %p 0x%llx 0x%llx  result=0x%x\n", src_map, (uint64_t)memory_address, (uint64_t)size, copy, target_map, (uint64_t)*address, (uint64_t)offset_in_mapping, result);
	}
	vm_map_copy_discard(copy_map);
	copy_map = VM_MAP_COPY_NULL;

	return result;
}

  
                                       
  
               
                                                          
                                                                   
         
  
                      
   

static kern_return_t
vm_map_remap_range_allocate(
	vm_map_t                map,
	vm_map_address_t        *address,                   
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	__unused vm_tag_t       tag,
	vm_map_entry_t          *map_entry)              
{
	vm_map_entry_t  entry;
	vm_map_offset_t start;
	vm_map_offset_t end;
	vm_map_offset_t desired_empty_end;
	kern_return_t   kr;
	vm_map_entry_t          hole_entry;

StartAgain:;

	start = *address;

	if (flags & VM_FLAGS_ANYWHERE) {
		if (flags & VM_FLAGS_RANDOM_ADDR) {
			  
                                 
      
			kr = vm_map_random_address_for_size(map, address, size);
			if (kr != KERN_SUCCESS) {
				return kr;
			}
			start = *address;
		}

		  
                                          
     

		if (start < map->min_offset) {
			start = map->min_offset;
		}
		if (start > map->max_offset) {
			return KERN_NO_SPACE;
		}

		  
                                         
                                         
                                        
     

		if (map->disable_vmentry_reuse == TRUE) {
			VM_MAP_HIGHEST_ENTRY(map, entry, start);
		} else {
			if (map->holelistenabled) {
				hole_entry = CAST_TO_VM_MAP_ENTRY(map->holes_list);

				if (hole_entry == NULL) {
					  
                                 
        
					return KERN_NO_SPACE;
				} else {
					boolean_t found_hole = FALSE;

					do {
						if (hole_entry->vme_start >= start) {
							start = hole_entry->vme_start;
							found_hole = TRUE;
							break;
						}

						if (hole_entry->vme_end > start) {
							found_hole = TRUE;
							break;
						}
						hole_entry = hole_entry->vme_next;
					} while (hole_entry != CAST_TO_VM_MAP_ENTRY(map->holes_list));

					if (found_hole == FALSE) {
						return KERN_NO_SPACE;
					}

					entry = hole_entry;
				}
			} else {
				assert(first_free_is_valid(map));
				if (start == map->min_offset) {
					if ((entry = map->first_free) != vm_map_to_entry(map)) {
						start = entry->vme_end;
					}
				} else {
					vm_map_entry_t  tmp_entry;
					if (vm_map_lookup_entry(map, start, &tmp_entry)) {
						start = tmp_entry->vme_end;
					}
					entry = tmp_entry;
				}
			}
			start = vm_map_round_page(start,
			    VM_MAP_PAGE_MASK(map));
		}

		  
                                             
                                           
          
     

		while (TRUE) {
			vm_map_entry_t  next;

			  
                                              
                                             
                              
      

			end = ((start + mask) & ~mask);
			end = vm_map_round_page(end,
			    VM_MAP_PAGE_MASK(map));
			if (end < start) {
				return KERN_NO_SPACE;
			}
			start = end;
			end += size;

			                                                                                    
			desired_empty_end = vm_map_round_page(end, VM_MAP_PAGE_MASK(map));

			if ((desired_empty_end > map->max_offset) || (desired_empty_end < start)) {
				if (map->wait_for_space) {
					if (size <= (map->max_offset -
					    map->min_offset)) {
						assert_wait((event_t) map, THREAD_INTERRUPTIBLE);
						vm_map_unlock(map);
						thread_block(THREAD_CONTINUE_NULL);
						vm_map_lock(map);
						goto StartAgain;
					}
				}

				return KERN_NO_SPACE;
			}

			next = entry->vme_next;

			if (map->holelistenabled) {
				if (entry->vme_end >= desired_empty_end) {
					break;
				}
			} else {
				  
                                                 
      
         
      
                                            
                                                 
       

				if (next == vm_map_to_entry(map)) {
					break;
				}

				if (next->vme_start >= desired_empty_end) {
					break;
				}
			}

			  
                                           
      

			entry = next;

			if (map->holelistenabled) {
				if (entry == CAST_TO_VM_MAP_ENTRY(map->holes_list)) {
					  
                      
        
					return KERN_NO_SPACE;
				}
				start = entry->vme_start;
			} else {
				start = entry->vme_end;
			}
		}

		if (map->holelistenabled) {
			if (vm_map_lookup_entry(map, entry->vme_start, &entry)) {
				panic("Found an existing entry (%p) instead of potential hole at address: 0x%llx.\n", entry, (unsigned long long)entry->vme_start);
			}
		}

		*address = start;
	} else {
		vm_map_entry_t          temp_entry;

		  
                 
                                        
                           
     

		if ((start & mask) != 0) {
			return KERN_NO_SPACE;
		}


		  
                                     
     

		end = start + size;

		if ((start < map->min_offset) ||
		    (end > map->max_offset) ||
		    (start >= end)) {
			return KERN_INVALID_ADDRESS;
		}

		  
                                                            
                                        
     
		if (flags & VM_FLAGS_OVERWRITE) {
			vm_map_t zap_map;
			int remove_flags = VM_MAP_REMOVE_SAVE_ENTRIES | VM_MAP_REMOVE_NO_MAP_ALIGN;

			  
                                                  
                                                          
                                                        
                  
      
			zap_map = vm_map_create(PMAP_NULL,
			    start,
			    end,
			    map->hdr.entries_pageable);
			if (zap_map == VM_MAP_NULL) {
				return KERN_RESOURCE_SHORTAGE;
			}
			vm_map_set_page_shift(zap_map, VM_MAP_PAGE_SHIFT(map));
			vm_map_disable_hole_optimization(zap_map);

			if (vmk_flags.vmkf_overwrite_immutable) {
				remove_flags |= VM_MAP_REMOVE_IMMUTABLE;
			}
			kr = vm_map_delete(map, start, end,
			    remove_flags,
			    zap_map);
			if (kr == KERN_SUCCESS) {
				vm_map_destroy(zap_map,
				    VM_MAP_REMOVE_NO_PMAP_CLEANUP);
				zap_map = VM_MAP_NULL;
			}
		}

		  
                                             
     

		if (vm_map_lookup_entry(map, start, &temp_entry)) {
			return KERN_NO_SPACE;
		}

		entry = temp_entry;

		  
                                            
                
     

		if ((entry->vme_next != vm_map_to_entry(map)) &&
		    (entry->vme_next->vme_start < end)) {
			return KERN_NO_SPACE;
		}
	}
	*map_entry = entry;
	return KERN_SUCCESS;
}

  
                 
  
                                                                  
   

vm_map_t
vm_map_switch(
	vm_map_t        map)
{
	int             mycpu;
	thread_t        thread = current_thread();
	vm_map_t        oldmap = thread->map;

	mp_disable_preemption();
	mycpu = cpu_number();

	  
                                                             
    
	PMAP_SWITCH_USER(thread, map, mycpu);

	mp_enable_preemption();
	return oldmap;
}


  
                             
  
               
                                                       
                                                        
                    
                                                        
                                                          
            
  
   
kern_return_t
vm_map_write_user(
	vm_map_t                map,
	void                    *src_p,
	vm_map_address_t        dst_addr,
	vm_size_t               size)
{
	kern_return_t   kr = KERN_SUCCESS;

	if (current_map() == map) {
		if (copyout(src_p, dst_addr, size)) {
			kr = KERN_INVALID_ADDRESS;
		}
	} else {
		vm_map_t        oldmap;

		                                                        
		                  

		vm_map_reference(map);
		oldmap = vm_map_switch(map);
		if (copyout(src_p, dst_addr, size)) {
			kr = KERN_INVALID_ADDRESS;
		}
		vm_map_switch(oldmap);
		vm_map_deallocate(map);
	}
	return kr;
}

  
                            
  
               
                                                      
                                                   
               
                                                        
                                                          
            
  
   
kern_return_t
vm_map_read_user(
	vm_map_t                map,
	vm_map_address_t        src_addr,
	void                    *dst_p,
	vm_size_t               size)
{
	kern_return_t   kr = KERN_SUCCESS;

	if (current_map() == map) {
		if (copyin(src_addr, dst_p, size)) {
			kr = KERN_INVALID_ADDRESS;
		}
	} else {
		vm_map_t        oldmap;

		                                                        
		                  

		vm_map_reference(map);
		oldmap = vm_map_switch(map);
		if (copyin(src_addr, dst_p, size)) {
			kr = KERN_INVALID_ADDRESS;
		}
		vm_map_switch(oldmap);
		vm_map_deallocate(map);
	}
	return kr;
}


  
                           
  
                                                  
                                                
                                       
   
boolean_t
vm_map_check_protection(vm_map_t map, vm_map_offset_t start,
    vm_map_offset_t end, vm_prot_t protection)
{
	vm_map_entry_t entry;
	vm_map_entry_t tmp_entry;

	vm_map_lock(map);

	if (start < vm_map_min(map) || end > vm_map_max(map) || start > end) {
		vm_map_unlock(map);
		return FALSE;
	}

	if (!vm_map_lookup_entry(map, start, &tmp_entry)) {
		vm_map_unlock(map);
		return FALSE;
	}

	entry = tmp_entry;

	while (start < end) {
		if (entry == vm_map_to_entry(map)) {
			vm_map_unlock(map);
			return FALSE;
		}

		  
                      
     

		if (start < entry->vme_start) {
			vm_map_unlock(map);
			return FALSE;
		}

		  
                                            
     

		if ((entry->protection & protection) != protection) {
			vm_map_unlock(map);
			return FALSE;
		}

		                      

		start = entry->vme_end;
		entry = entry->vme_next;
	}
	vm_map_unlock(map);
	return TRUE;
}

kern_return_t
vm_map_purgable_control(
	vm_map_t                map,
	vm_map_offset_t         address,
	vm_purgable_t           control,
	int                     *state)
{
	vm_map_entry_t          entry;
	vm_object_t             object;
	kern_return_t           kr;
	boolean_t               was_nonvolatile;

	  
                                                                  
                                                                   
    
	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (control != VM_PURGABLE_SET_STATE &&
	    control != VM_PURGABLE_GET_STATE &&
	    control != VM_PURGABLE_PURGE_ALL &&
	    control != VM_PURGABLE_SET_STATE_FROM_KERNEL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (control == VM_PURGABLE_PURGE_ALL) {
		vm_purgeable_object_purge_all();
		return KERN_SUCCESS;
	}

	if ((control == VM_PURGABLE_SET_STATE ||
	    control == VM_PURGABLE_SET_STATE_FROM_KERNEL) &&
	    (((*state & ~(VM_PURGABLE_ALL_MASKS)) != 0) ||
	    ((*state & VM_PURGABLE_STATE_MASK) > VM_PURGABLE_STATE_MASK))) {
		return KERN_INVALID_ARGUMENT;
	}

	vm_map_lock_read(map);

	if (!vm_map_lookup_entry(map, address, &entry) || entry->is_sub_map) {
		  
                                          
     
		vm_map_unlock_read(map);
		return KERN_INVALID_ADDRESS;
	}

	if ((entry->protection & VM_PROT_WRITE) == 0) {
		  
                                                                
     
		vm_map_unlock_read(map);
		return KERN_PROTECTION_FAILURE;
	}

	object = VME_OBJECT(entry);
	if (object == VM_OBJECT_NULL ||
	    object->purgable == VM_PURGABLE_DENY) {
		  
                                                     
     
		vm_map_unlock_read(map);
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_lock(object);

#if 00
	if (VME_OFFSET(entry) != 0 ||
	    entry->vme_end - entry->vme_start != object->vo_size) {
		  
                                                             
                    
     
		vm_map_unlock_read(map);
		vm_object_unlock(object);
		return KERN_INVALID_ARGUMENT;
	}
#endif

	assert(!entry->is_sub_map);
	assert(!entry->use_pmap);                                       

	vm_map_unlock_read(map);

	was_nonvolatile = (object->purgable == VM_PURGABLE_NONVOLATILE);

	kr = vm_object_purgable_control(object, control, state);

	if (was_nonvolatile &&
	    object->purgable != VM_PURGABLE_NONVOLATILE &&
	    map->pmap == kernel_pmap) {
#if DEBUG
		object->vo_purgeable_volatilizer = kernel_task;
#endif            
	}

	vm_object_unlock(object);

	return kr;
}

void
vm_map_footprint_query_page_info(
	vm_map_t        map,
	vm_map_entry_t  map_entry,
	vm_map_offset_t curr_s_offset,
	int             *disposition_p)
{
	int             pmap_disp;
	vm_object_t     object;
	int             disposition;
	int             effective_page_size;

	vm_map_lock_assert_held(map);
	assert(!map->has_corpse_footprint);
	assert(curr_s_offset >= map_entry->vme_start);
	assert(curr_s_offset < map_entry->vme_end);

	object = VME_OBJECT(map_entry);
	if (object == VM_OBJECT_NULL) {
		*disposition_p = 0;
		return;
	}

	effective_page_size = MIN(PAGE_SIZE, VM_MAP_PAGE_SIZE(map));

	pmap_disp = 0;
	if (object == VM_OBJECT_NULL) {
		                                         
		*disposition_p = 0;
		return;
	} else if (map_entry->is_sub_map &&
	    !map_entry->use_pmap) {
		                               
		*disposition_p = 0;
		return;
	}

	  
                   
    
	pmap_query_page_info(map->pmap, curr_s_offset, &pmap_disp);

	  
                                    
    
	disposition = 0;

	                                            
	if (!map_entry->is_sub_map &&
	    object->vo_no_footprint) {
		                                 
		assertf(!map_entry->use_pmap, "offset 0x%llx map_entry %p", (uint64_t) curr_s_offset, map_entry);
	} else if (!map_entry->is_sub_map &&
	    (object->purgable == VM_PURGABLE_NONVOLATILE ||
	    (object->purgable == VM_PURGABLE_DENY &&
	    object->vo_ledger_tag)) &&
	    VM_OBJECT_OWNER(object) != NULL &&
	    VM_OBJECT_OWNER(object)->map == map) {
		assertf(!map_entry->use_pmap, "offset 0x%llx map_entry %p", (uint64_t) curr_s_offset, map_entry);
		if ((((curr_s_offset
		    - map_entry->vme_start
		    + VME_OFFSET(map_entry))
		    / effective_page_size) <
		    (object->resident_page_count +
		    vm_compressor_pager_get_count(object->pager)))) {
			  
                                         
                                    
                                        
                                   
                                          
                                       
                                         
                                    
                                      
                                         
                         
      
			disposition |= VM_PAGE_QUERY_PAGE_PRESENT;
		}
	} else if (!map_entry->is_sub_map &&
	    (object->purgable == VM_PURGABLE_VOLATILE ||
	    object->purgable == VM_PURGABLE_EMPTY) &&
	    VM_OBJECT_OWNER(object) != NULL &&
	    VM_OBJECT_OWNER(object)->map == map) {
		assertf(!map_entry->use_pmap, "offset 0x%llx map_entry %p", (uint64_t) curr_s_offset, map_entry);
		if ((((curr_s_offset
		    - map_entry->vme_start
		    + VME_OFFSET(map_entry))
		    / effective_page_size) <
		    object->wired_page_count)) {
			  
                                           
                                    
                                      
                                      
                                          
                                   
                                        
                                        
                                     
                                       
           
      
			disposition |= VM_PAGE_QUERY_PAGE_PRESENT;
		}
	} else if (!map_entry->is_sub_map &&
	    map_entry->iokit_acct &&
	    object->internal &&
	    object->purgable == VM_PURGABLE_DENY) {
		  
                                               
                                         
     
		assertf(!map_entry->use_pmap, "offset 0x%llx map_entry %p", (uint64_t) curr_s_offset, map_entry);
		disposition |= VM_PAGE_QUERY_PAGE_PRESENT;
		disposition |= VM_PAGE_QUERY_PAGE_DIRTY;
	} else if (pmap_disp & (PMAP_QUERY_PAGE_ALTACCT |
	    PMAP_QUERY_PAGE_COMPRESSED_ALTACCT)) {
		                          
#if (__arm__ || __arm64__) && (DEVELOPMENT || DEBUG)
		if (map->pmap->footprint_was_suspended) {
			  
                                          
                                    
                                     
                                       
                                        
                                
                  
      
		} else
#endif                                                       
		{
			assertf(!map_entry->use_pmap, "offset 0x%llx map_entry %p", (uint64_t) curr_s_offset, map_entry);
		}
		disposition = 0;
	} else {
		if (pmap_disp & PMAP_QUERY_PAGE_PRESENT) {
			assertf(map_entry->use_pmap, "offset 0x%llx map_entry %p", (uint64_t) curr_s_offset, map_entry);
			disposition |= VM_PAGE_QUERY_PAGE_PRESENT;
			disposition |= VM_PAGE_QUERY_PAGE_REF;
			if (pmap_disp & PMAP_QUERY_PAGE_INTERNAL) {
				disposition |= VM_PAGE_QUERY_PAGE_DIRTY;
			} else {
				disposition |= VM_PAGE_QUERY_PAGE_EXTERNAL;
			}
			if (pmap_disp & PMAP_QUERY_PAGE_REUSABLE) {
				disposition |= VM_PAGE_QUERY_PAGE_REUSABLE;
			}
		} else if (pmap_disp & PMAP_QUERY_PAGE_COMPRESSED) {
			assertf(map_entry->use_pmap, "offset 0x%llx map_entry %p", (uint64_t) curr_s_offset, map_entry);
			disposition |= VM_PAGE_QUERY_PAGE_PAGED_OUT;
		}
	}

	*disposition_p = disposition;
}

kern_return_t
vm_map_page_query_internal(
	vm_map_t        target_map,
	vm_map_offset_t offset,
	int             *disposition,
	int             *ref_count)
{
	kern_return_t                   kr;
	vm_page_info_basic_data_t       info;
	mach_msg_type_number_t          count;

	count = VM_PAGE_INFO_BASIC_COUNT;
	kr = vm_map_page_info(target_map,
	    offset,
	    VM_PAGE_INFO_BASIC,
	    (vm_page_info_t) &info,
	    &count);
	if (kr == KERN_SUCCESS) {
		*disposition = info.disposition;
		*ref_count = info.ref_count;
	} else {
		*disposition = 0;
		*ref_count = 0;
	}

	return kr;
}

kern_return_t
vm_map_page_info(
	vm_map_t                map,
	vm_map_offset_t         offset,
	vm_page_info_flavor_t   flavor,
	vm_page_info_t          info,
	mach_msg_type_number_t  *count)
{
	return vm_map_page_range_info_internal(map,
	           offset,                     
	           (offset + 1),                                                             
	           (int)-1,                                        
	           flavor,
	           info,
	           count);
}

kern_return_t
vm_map_page_range_info_internal(
	vm_map_t                map,
	vm_map_offset_t         start_offset,
	vm_map_offset_t         end_offset,
	int                     effective_page_shift,
	vm_page_info_flavor_t   flavor,
	vm_page_info_t          info,
	mach_msg_type_number_t  *count)
{
	vm_map_entry_t          map_entry = VM_MAP_ENTRY_NULL;
	vm_object_t             object = VM_OBJECT_NULL, curr_object = VM_OBJECT_NULL;
	vm_page_t               m = VM_PAGE_NULL;
	kern_return_t           retval = KERN_SUCCESS;
	int                     disposition = 0;
	int                     ref_count = 0;
	int                     depth = 0, info_idx = 0;
	vm_page_info_basic_t    basic_info = 0;
	vm_map_offset_t         offset_in_page = 0, offset_in_object = 0, curr_offset_in_object = 0;
	vm_map_offset_t         start = 0, end = 0, curr_s_offset = 0, curr_e_offset = 0;
	boolean_t               do_region_footprint;
	ledger_amount_t         ledger_resident, ledger_compressed;
	int                     effective_page_size;
	vm_map_offset_t         effective_page_mask;

	switch (flavor) {
	case VM_PAGE_INFO_BASIC:
		if (*count != VM_PAGE_INFO_BASIC_COUNT) {
			  
                                                     
                                                     
                                                       
      
			if (*count != VM_PAGE_INFO_BASIC_COUNT - 1) {
				return KERN_INVALID_ARGUMENT;
			}
		}
		break;
	default:
		return KERN_INVALID_ARGUMENT;
	}

	if (effective_page_shift == -1) {
		effective_page_shift = vm_self_region_page_shift_safely(map);
		if (effective_page_shift == -1) {
			return KERN_INVALID_ARGUMENT;
		}
	}
	effective_page_size = (1 << effective_page_shift);
	effective_page_mask = effective_page_size - 1;

	do_region_footprint = task_self_region_footprint();
	disposition = 0;
	ref_count = 0;
	depth = 0;
	info_idx = 0;                                                                   
	retval = KERN_SUCCESS;

	offset_in_page = start_offset & effective_page_mask;
	start = vm_map_trunc_page(start_offset, effective_page_mask);
	end = vm_map_round_page(end_offset, effective_page_mask);

	if (end < start) {
		return KERN_INVALID_ARGUMENT;
	}

	assert((end - start) <= MAX_PAGE_RANGE_QUERY);

	vm_map_lock_read(map);

	task_ledgers_footprint(map->pmap->ledger, &ledger_resident, &ledger_compressed);

	for (curr_s_offset = start; curr_s_offset < end;) {
		  
                                               
     
		curr_object = object = VM_OBJECT_NULL;
		offset_in_object = 0;
		ref_count = 0;
		depth = 0;

		if (do_region_footprint &&
		    curr_s_offset >= vm_map_last_entry(map)->vme_end) {
			  
                                                      
                                                
                                                
                                                    
                                
      
			disposition = 0;

			if (curr_s_offset - vm_map_last_entry(map)->vme_end <=
			    (unsigned) ledger_compressed) {
				  
                                                
                                                 
                            
       
				disposition |= VM_PAGE_QUERY_PAGE_PAGED_OUT;
			} else {
				  
                                          
                                                   
                                          
                        
       
				disposition |= VM_PAGE_QUERY_PAGE_PRESENT;
				disposition |= VM_PAGE_QUERY_PAGE_DIRTY;
				disposition |= VM_PAGE_QUERY_PAGE_REF;
			}
			switch (flavor) {
			case VM_PAGE_INFO_BASIC:
				basic_info = (vm_page_info_basic_t) (((uintptr_t) info) + (info_idx * sizeof(struct vm_page_info_basic)));
				basic_info->disposition = disposition;
				basic_info->ref_count = 1;
				basic_info->object_id = VM_OBJECT_ID_FAKE(map, task_ledgers.purgeable_nonvolatile);
				basic_info->offset = 0;
				basic_info->depth = 0;

				info_idx++;
				break;
			}
			curr_s_offset += effective_page_size;
			continue;
		}

		  
                                                                   
                          
     
		if (!vm_map_lookup_entry(map, curr_s_offset, &map_entry)) {
			                                      

			if (curr_s_offset < vm_map_min(map)) {
				  
                                                
       
				curr_e_offset = MIN(end, vm_map_min(map));
			} else if (curr_s_offset >= vm_map_max(map)) {
				  
                                                   
       
				curr_e_offset = end;
			} else if (map_entry == vm_map_to_entry(map)) {
				  
                  
       
				if (map_entry->vme_next == vm_map_to_entry(map)) {
					  
                  
        
					curr_e_offset = MIN(map->max_offset, end);
				} else {
					  
                                 
        
					curr_e_offset = MIN(map_entry->vme_next->vme_start, end);
				}
			} else {
				if (map_entry->vme_next == vm_map_to_entry(map)) {
					  
                                   
        
					curr_e_offset = MIN(map->max_offset, end);
				} else {
					curr_e_offset = MIN(map_entry->vme_next->vme_start, end);
				}
			}

			assert(curr_e_offset >= curr_s_offset);

			uint64_t num_pages = (curr_e_offset - curr_s_offset) >> effective_page_shift;

			void *info_ptr = (void*) (((uintptr_t) info) + (info_idx * sizeof(struct vm_page_info_basic)));

			bzero(info_ptr, num_pages * sizeof(struct vm_page_info_basic));

			curr_s_offset = curr_e_offset;

			info_idx += num_pages;

			continue;
		}

		                                                
		offset_in_object = curr_s_offset - map_entry->vme_start;

		                                                             
		offset_in_object += VME_OFFSET(map_entry);

		if (map_entry->is_sub_map) {
			vm_map_t sub_map = VM_MAP_NULL;
			vm_page_info_t submap_info = 0;
			vm_map_offset_t submap_s_offset = 0, submap_e_offset = 0, range_len = 0;

			range_len = MIN(map_entry->vme_end, end) - curr_s_offset;

			submap_s_offset = offset_in_object;
			submap_e_offset = submap_s_offset + range_len;

			sub_map = VME_SUBMAP(map_entry);

			vm_map_reference(sub_map);
			vm_map_unlock_read(map);

			submap_info = (vm_page_info_t) (((uintptr_t) info) + (info_idx * sizeof(struct vm_page_info_basic)));

			assertf(VM_MAP_PAGE_SHIFT(sub_map) >= VM_MAP_PAGE_SHIFT(map),
			    "Submap page size (%d) differs from current map (%d)\n", VM_MAP_PAGE_SIZE(sub_map), VM_MAP_PAGE_SIZE(map));

			retval = vm_map_page_range_info_internal(sub_map,
			    submap_s_offset,
			    submap_e_offset,
			    effective_page_shift,
			    VM_PAGE_INFO_BASIC,
			    (vm_page_info_t) submap_info,
			    count);

			assert(retval == KERN_SUCCESS);

			vm_map_lock_read(map);
			vm_map_deallocate(sub_map);

			                                                               
			info_idx += range_len >> effective_page_shift;

			                                                                   
			curr_s_offset += range_len;

			continue;
		}

		object = VME_OBJECT(map_entry);

		if (object == VM_OBJECT_NULL) {
			  
                                              
                                            
                                   
      

			curr_e_offset = MIN(map_entry->vme_end, end);

			uint64_t num_pages = (curr_e_offset - curr_s_offset) >> effective_page_shift;

			void *info_ptr = (void*) (((uintptr_t) info) + (info_idx * sizeof(struct vm_page_info_basic)));

			bzero(info_ptr, num_pages * sizeof(struct vm_page_info_basic));

			curr_s_offset = curr_e_offset;

			info_idx += num_pages;

			continue;
		}

		if (do_region_footprint) {
			disposition = 0;
			if (map->has_corpse_footprint) {
				  
                                        
                                
       
				vm_map_corpse_footprint_query_page_info(
					map,
					curr_s_offset,
					&disposition);
			} else {
				  
                                             
                       
       
				vm_map_footprint_query_page_info(
					map,
					map_entry,
					curr_s_offset,
					&disposition);
			}
			switch (flavor) {
			case VM_PAGE_INFO_BASIC:
				basic_info = (vm_page_info_basic_t) (((uintptr_t) info) + (info_idx * sizeof(struct vm_page_info_basic)));
				basic_info->disposition = disposition;
				basic_info->ref_count = 1;
				basic_info->object_id = VM_OBJECT_ID_FAKE(map, task_ledgers.purgeable_nonvolatile);
				basic_info->offset = 0;
				basic_info->depth = 0;

				info_idx++;
				break;
			}
			curr_s_offset += effective_page_size;
			continue;
		}

		vm_object_reference(object);
		  
                                                 
                          
     
		vm_object_lock_shared(object);

		curr_e_offset = MIN(map_entry->vme_end, end);

		vm_map_unlock_read(map);

		map_entry = NULL;                                                     

		curr_object = object;

		for (; curr_s_offset < curr_e_offset;) {
			if (object == curr_object) {
				ref_count = curr_object->ref_count - 1;                                              
			} else {
				ref_count = curr_object->ref_count;
			}

			curr_offset_in_object = offset_in_object;

			for (;;) {
				m = vm_page_lookup(curr_object, vm_object_trunc_page(curr_offset_in_object));

				if (m != VM_PAGE_NULL) {
					disposition |= VM_PAGE_QUERY_PAGE_PRESENT;
					break;
				} else {
					if (curr_object->internal &&
					    curr_object->alive &&
					    !curr_object->terminating &&
					    curr_object->pager_ready) {
						if (VM_COMPRESSOR_PAGER_STATE_GET(curr_object, vm_object_trunc_page(curr_offset_in_object))
						    == VM_EXTERNAL_STATE_EXISTS) {
							                             
							disposition |= VM_PAGE_QUERY_PAGE_PAGED_OUT;
							break;
						}
					}

					  
                                                                 
                          
        

					if (curr_object->shadow != VM_OBJECT_NULL) {
						vm_object_t shadow = VM_OBJECT_NULL;

						curr_offset_in_object += curr_object->vo_shadow_offset;
						shadow = curr_object->shadow;

						vm_object_lock_shared(shadow);
						vm_object_unlock(curr_object);

						curr_object = shadow;
						depth++;
						continue;
					} else {
						break;
					}
				}
			}

			                                                                      
			                                                                      
			                                                                      
			                                                                      
			                                                                      

			                                                                      
			                                                                      
			                        

			if ((curr_object == object) && curr_object->shadow) {
				disposition |= VM_PAGE_QUERY_PAGE_COPIED;
			}

			if (!curr_object->internal) {
				disposition |= VM_PAGE_QUERY_PAGE_EXTERNAL;
			}

			if (m != VM_PAGE_NULL) {
				if (m->vmp_fictitious) {
					disposition |= VM_PAGE_QUERY_PAGE_FICTITIOUS;
				} else {
					if (m->vmp_dirty || pmap_is_modified(VM_PAGE_GET_PHYS_PAGE(m))) {
						disposition |= VM_PAGE_QUERY_PAGE_DIRTY;
					}

					if (m->vmp_reference || pmap_is_referenced(VM_PAGE_GET_PHYS_PAGE(m))) {
						disposition |= VM_PAGE_QUERY_PAGE_REF;
					}

					if (m->vmp_q_state == VM_PAGE_ON_SPECULATIVE_Q) {
						disposition |= VM_PAGE_QUERY_PAGE_SPECULATIVE;
					}

					  
                   
                                       
                                           
             
        
					if (m->vmp_cs_validated) {
						disposition |= VM_PAGE_QUERY_PAGE_CS_VALIDATED;
					}
					if (m->vmp_cs_tainted) {
						disposition |= VM_PAGE_QUERY_PAGE_CS_TAINTED;
					}
					if (m->vmp_cs_nx) {
						disposition |= VM_PAGE_QUERY_PAGE_CS_NX;
					}
					if (m->vmp_reusable || curr_object->all_reusable) {
						disposition |= VM_PAGE_QUERY_PAGE_REUSABLE;
					}
				}
			}

			switch (flavor) {
			case VM_PAGE_INFO_BASIC:
				basic_info = (vm_page_info_basic_t) (((uintptr_t) info) + (info_idx * sizeof(struct vm_page_info_basic)));
				basic_info->disposition = disposition;
				basic_info->ref_count = ref_count;
				basic_info->object_id = (vm_object_id_t) (uintptr_t)
				    VM_KERNEL_ADDRPERM(curr_object);
				basic_info->offset =
				    (memory_object_offset_t) curr_offset_in_object + offset_in_page;
				basic_info->depth = depth;

				info_idx++;
				break;
			}

			disposition = 0;
			offset_in_page = 0;                                                                                 

			  
                                                         
      
			curr_s_offset += effective_page_size;
			offset_in_object += effective_page_size;
			curr_offset_in_object = offset_in_object;

			if (curr_object != object) {
				vm_object_unlock(curr_object);

				curr_object = object;

				vm_object_lock_shared(curr_object);
			} else {
				vm_object_lock_yield_shared(curr_object);
			}
		}

		vm_object_unlock(curr_object);
		vm_object_deallocate(curr_object);

		vm_map_lock_read(map);
	}

	vm_map_unlock_read(map);
	return retval;
}

  
               
  
                                                                 
                                                                       
                                                                     
                                                                   
                                                                       
                             
  
                               
                                                           
                         
  
                                                                    
                                              
                                     
  
                                             
                                             
                           
  
                                                              
                                             
                                          
                                     
  
       
                                                                   
                                                           
  
          
                                        
                                                             
                            
                                                        
   

kern_return_t
vm_map_msync(
	vm_map_t                map,
	vm_map_address_t        address,
	vm_map_size_t           size,
	vm_sync_t               sync_flags)
{
	vm_map_entry_t          entry;
	vm_map_size_t           amount_left;
	vm_object_offset_t      offset;
	vm_object_offset_t      start_offset, end_offset;
	boolean_t               do_sync_req;
	boolean_t               had_hole = FALSE;
	vm_map_offset_t         pmap_offset;

	if ((sync_flags & VM_SYNC_ASYNCHRONOUS) &&
	    (sync_flags & VM_SYNC_SYNCHRONOUS)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (VM_MAP_PAGE_MASK(map) < PAGE_MASK) {
		DEBUG4K_SHARE("map %p address 0x%llx size 0x%llx flags 0x%x\n", map, (uint64_t)address, (uint64_t)size, sync_flags);
	}

	  
                                             
    
	size = (vm_map_round_page(address + size,
	    VM_MAP_PAGE_MASK(map)) -
	    vm_map_trunc_page(address,
	    VM_MAP_PAGE_MASK(map)));
	address = vm_map_trunc_page(address,
	    VM_MAP_PAGE_MASK(map));

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_TASK;
	}

	if (size == 0) {
		return KERN_SUCCESS;
	}

	amount_left = size;

	while (amount_left > 0) {
		vm_object_size_t        flush_size;
		vm_object_t             object;

		vm_map_lock(map);
		if (!vm_map_lookup_entry(map,
		    address,
		    &entry)) {
			vm_map_size_t   skip;

			  
                              
      
			had_hole = TRUE;

			if (sync_flags & VM_SYNC_KILLPAGES) {
				  
                                             
                                               
                                              
                                               
                   
       
				vm_map_unlock(map);
				break;
			}

			  
                          
      
			if (entry == vm_map_to_entry(map) &&
			    entry->vme_next == entry) {
				vm_map_unlock(map);
				break;
			}
			  
                                       
                                          
      
			if ((map->hdr.nentries == 0) ||
			    (entry->vme_next->vme_start < address)) {
				vm_map_unlock(map);
				break;
			}
			  
                                         
      
			skip = (entry->vme_next->vme_start - address);
			if (skip >= amount_left) {
				amount_left = 0;
			} else {
				amount_left -= skip;
			}
			address = entry->vme_next->vme_start;
			vm_map_unlock(map);
			continue;
		}

		offset = address - entry->vme_start;
		pmap_offset = address;

		  
                                                       
            
     
		if (amount_left + entry->vme_start + offset > entry->vme_end) {
			flush_size = entry->vme_end -
			    (entry->vme_start + offset);
		} else {
			flush_size = amount_left;
		}
		amount_left -= flush_size;
		address += flush_size;

		if (entry->is_sub_map == TRUE) {
			vm_map_t        local_map;
			vm_map_offset_t local_offset;

			local_map = VME_SUBMAP(entry);
			local_offset = VME_OFFSET(entry);
			vm_map_reference(local_map);
			vm_map_unlock(map);
			if (vm_map_msync(
				    local_map,
				    local_offset,
				    flush_size,
				    sync_flags) == KERN_INVALID_ADDRESS) {
				had_hole = TRUE;
			}
			vm_map_deallocate(local_map);
			continue;
		}
		object = VME_OBJECT(entry);

		  
                                                         
                
     
		if (object == VM_OBJECT_NULL) {
			vm_map_unlock(map);
			continue;
		}
		offset += VME_OFFSET(entry);

		vm_object_lock(object);

		if (sync_flags & (VM_SYNC_KILLPAGES | VM_SYNC_DEACTIVATE)) {
			int kill_pages = 0;
			boolean_t reusable_pages = FALSE;

			if (VM_MAP_PAGE_MASK(map) < PAGE_MASK) {
				  
                                                
                                               
                     
       
				start_offset = vm_object_round_page(offset);
				end_offset = vm_object_trunc_page(offset + flush_size);

				if (end_offset <= start_offset) {
					vm_object_unlock(object);
					vm_map_unlock(map);
					continue;
				}

				pmap_offset += start_offset - offset;;
			} else {
				start_offset = offset;
				end_offset = offset + flush_size;
			}

			if (sync_flags & VM_SYNC_KILLPAGES) {
				if (((object->ref_count == 1) ||
				    ((object->copy_strategy !=
				    MEMORY_OBJECT_COPY_SYMMETRIC) &&
				    (object->copy == VM_OBJECT_NULL))) &&
				    (object->shadow == VM_OBJECT_NULL)) {
					if (object->ref_count != 1) {
						vm_page_stats_reusable.free_shared++;
					}
					kill_pages = 1;
				} else {
					kill_pages = -1;
				}
			}
			if (kill_pages != -1) {
				vm_object_deactivate_pages(
					object,
					start_offset,
					(vm_object_size_t) (end_offset - start_offset),
					kill_pages,
					reusable_pages,
					map->pmap,
					pmap_offset);
			}
			vm_object_unlock(object);
			vm_map_unlock(map);
			continue;
		}
		  
                                                      
                                                             
                                                         
     
		if ((object->pager == MEMORY_OBJECT_NULL) ||
		    (object->internal) || (object->private)) {
			vm_object_unlock(object);
			vm_map_unlock(map);
			continue;
		}
		  
                                                       
     
		vm_object_reference_locked(object);
		vm_object_unlock(object);

		vm_map_unlock(map);

		if (VM_MAP_PAGE_MASK(map) < PAGE_MASK) {
			start_offset = vm_object_trunc_page(offset);
			end_offset = vm_object_round_page(offset + flush_size);
		} else {
			start_offset = offset;
			end_offset = offset + flush_size;
		}

		do_sync_req = vm_object_sync(object,
		    start_offset,
		    (end_offset - start_offset),
		    sync_flags & VM_SYNC_INVALIDATE,
		    ((sync_flags & VM_SYNC_SYNCHRONOUS) ||
		    (sync_flags & VM_SYNC_ASYNCHRONOUS)),
		    sync_flags & VM_SYNC_SYNCHRONOUS);

		if ((sync_flags & VM_SYNC_INVALIDATE) && object->resident_page_count == 0) {
			  
                                                   
      
			vm_object_lock(object);

			object->pages_created = 0;
			object->pages_used = 0;
			object->sequential = 0;
			object->last_alloc = 0;

			vm_object_unlock(object);
		}
		vm_object_deallocate(object);
	}            

	                                  
	if (had_hole == TRUE && (sync_flags & VM_SYNC_CONTIGUOUS)) {
		return KERN_INVALID_ADDRESS;
	}

	return KERN_SUCCESS;
}              

kern_return_t
vm_named_entry_from_vm_object(
	vm_named_entry_t        named_entry,
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	vm_prot_t               prot)
{
	vm_map_copy_t copy;
	vm_map_entry_t copy_entry;

	assert(!named_entry->is_sub_map);
	assert(!named_entry->is_copy);
	assert(!named_entry->is_object);
	assert(!named_entry->internal);
	assert(named_entry->backing.copy == VM_MAP_COPY_NULL);

	copy = vm_map_copy_allocate();
	copy->type = VM_MAP_COPY_ENTRY_LIST;
	copy->offset = offset;
	copy->size = size;
	copy->cpy_hdr.page_shift = PAGE_SHIFT;
	vm_map_store_init(&copy->cpy_hdr);

	copy_entry = vm_map_copy_entry_create(copy, FALSE);
	copy_entry->protection = prot;
	copy_entry->max_protection = prot;
	copy_entry->use_pmap = TRUE;
	copy_entry->vme_start = VM_MAP_TRUNC_PAGE(offset, PAGE_MASK);
	copy_entry->vme_end = VM_MAP_ROUND_PAGE(offset + size, PAGE_MASK);
	VME_OBJECT_SET(copy_entry, object);
	VME_OFFSET_SET(copy_entry, vm_object_trunc_page(offset));
	vm_map_copy_entry_link(copy, vm_map_copy_last_entry(copy), copy_entry);

	named_entry->backing.copy = copy;
	named_entry->is_object = TRUE;
	if (object->internal) {
		named_entry->internal = TRUE;
	}

	DEBUG4K_MEMENTRY("named_entry %p copy %p object %p offset 0x%llx size 0x%llx prot 0x%x\n", named_entry, copy, object, offset, size, prot);

	return KERN_SUCCESS;
}

vm_object_t
vm_named_entry_to_vm_object(
	vm_named_entry_t named_entry)
{
	vm_map_copy_t   copy;
	vm_map_entry_t  copy_entry;
	vm_object_t     object;

	assert(!named_entry->is_sub_map);
	assert(!named_entry->is_copy);
	assert(named_entry->is_object);
	copy = named_entry->backing.copy;
	assert(copy != VM_MAP_COPY_NULL);
	assert(copy->cpy_hdr.nentries == 1);
	copy_entry = vm_map_copy_first_entry(copy);
	assert(!copy_entry->is_sub_map);
	object = VME_OBJECT(copy_entry);

	DEBUG4K_MEMENTRY("%p -> %p -> %p [0x%llx 0x%llx 0x%llx 0x%x/0x%x ] -> %p offset 0x%llx size 0x%llx prot 0x%x\n", named_entry, copy, copy_entry, (uint64_t)copy_entry->vme_start, (uint64_t)copy_entry->vme_end, copy_entry->vme_offset, copy_entry->protection, copy_entry->max_protection, object, named_entry->offset, named_entry->size, named_entry->protection);

	return object;
}

  
                                     
           
                                                     
                                                               
                                                       
                                             
              
                   
   


vm_map_t
convert_port_entry_to_map(
	ipc_port_t      port)
{
	vm_map_t map;
	vm_named_entry_t        named_entry;
	uint32_t        try_failed_count = 0;

	if (IP_VALID(port) && (ip_kotype(port) == IKOT_NAMED_ENTRY)) {
		while (TRUE) {
			ip_lock(port);
			if (ip_active(port) && (ip_kotype(port)
			    == IKOT_NAMED_ENTRY)) {
				named_entry =
				    (vm_named_entry_t) ip_get_kobject(port);
				if (!(lck_mtx_try_lock(&(named_entry)->Lock))) {
					ip_unlock(port);

					try_failed_count++;
					mutex_pause(try_failed_count);
					continue;
				}
				named_entry->ref_count++;
				lck_mtx_unlock(&(named_entry)->Lock);
				ip_unlock(port);
				if ((named_entry->is_sub_map) &&
				    (named_entry->protection
				    & VM_PROT_WRITE)) {
					map = named_entry->backing.map;
					if (map->pmap != PMAP_NULL) {
						if (map->pmap == kernel_pmap) {
							panic("userspace has access "
							    "to a kernel map %p", map);
						}
						pmap_require(map->pmap);
					}
				} else {
					mach_destroy_memory_entry(port);
					return VM_MAP_NULL;
				}
				vm_map_reference(map);
				mach_destroy_memory_entry(port);
				break;
			} else {
				return VM_MAP_NULL;
			}
		}
	} else {
		map = convert_port_to_map(port);
	}

	return map;
}

  
                                        
           
                                                      
                                                             
                      
              
                   
   


vm_object_t
convert_port_entry_to_object(
	ipc_port_t      port)
{
	vm_object_t             object = VM_OBJECT_NULL;
	vm_named_entry_t        named_entry;
	uint32_t                try_failed_count = 0;

	if (IP_VALID(port) &&
	    (ip_kotype(port) == IKOT_NAMED_ENTRY)) {
try_again:
		ip_lock(port);
		if (ip_active(port) &&
		    (ip_kotype(port) == IKOT_NAMED_ENTRY)) {
			named_entry = (vm_named_entry_t) ip_get_kobject(port);
			if (!(lck_mtx_try_lock(&(named_entry)->Lock))) {
				ip_unlock(port);
				try_failed_count++;
				mutex_pause(try_failed_count);
				goto try_again;
			}
			named_entry->ref_count++;
			lck_mtx_unlock(&(named_entry)->Lock);
			ip_unlock(port);
			if (!(named_entry->is_sub_map) &&
			    !(named_entry->is_copy) &&
			    (named_entry->is_object) &&
			    (named_entry->protection & VM_PROT_WRITE)) {
				vm_map_copy_t copy;
				vm_map_entry_t copy_entry;

				copy = named_entry->backing.copy;
				assert(copy->cpy_hdr.nentries == 1);
				copy_entry = vm_map_copy_first_entry(copy);
				assert(!copy_entry->is_sub_map);
				object = VME_OBJECT(copy_entry);
				assert(object != VM_OBJECT_NULL);
				vm_object_reference(object);
			}
			mach_destroy_memory_entry(port);
		}
	}

	return object;
}

  
                                                                               
          
   
#undef current_map
vm_map_t
current_map(void)
{
	return current_map_fast();
}

  
                    
  
                                          
   
void
vm_map_reference(
	vm_map_t        map)
{
	if (__probable(map != VM_MAP_NULL)) {
		vm_map_require(map);
		os_ref_retain(&map->map_refcnt);
	}
}

  
                     
  
                                              
                                         
                                
   
void
vm_map_deallocate(
	vm_map_t        map)
{
	if (__probable(map != VM_MAP_NULL)) {
		vm_map_require(map);
		if (os_ref_release(&map->map_refcnt) == 0) {
			vm_map_destroy(map, VM_MAP_REMOVE_NO_FLAGS);
		}
	}
}

void
vm_map_inspect_deallocate(
	vm_map_inspect_t      map)
{
	vm_map_deallocate((vm_map_t)map);
}

void
vm_map_read_deallocate(
	vm_map_read_t      map)
{
	vm_map_deallocate((vm_map_t)map);
}


void
vm_map_disable_NX(vm_map_t map)
{
	if (map == NULL) {
		return;
	}
	if (map->pmap == NULL) {
		return;
	}

	pmap_disable_NX(map->pmap);
}

void
vm_map_disallow_data_exec(vm_map_t map)
{
	if (map == NULL) {
		return;
	}

	map->map_disallow_data_exec = TRUE;
}

                                                                               
                    
   
void
vm_map_set_32bit(vm_map_t map)
{
#if defined(__arm__) || defined(__arm64__)
	map->max_offset = pmap_max_offset(FALSE, ARM_PMAP_MAX_OFFSET_DEVICE);
#else
	map->max_offset = (vm_map_offset_t)VM_MAX_ADDRESS;
#endif
}


void
vm_map_set_64bit(vm_map_t map)
{
#if defined(__arm__) || defined(__arm64__)
	map->max_offset = pmap_max_offset(TRUE, ARM_PMAP_MAX_OFFSET_DEVICE);
#else
	map->max_offset = (vm_map_offset_t)MACH_VM_MAX_ADDRESS;
#endif
}

  
                                                                       
   
void
vm_map_set_jumbo(vm_map_t map)
{
#if defined (__arm64__) && !defined(CONFIG_ARROW)
	vm_map_set_max_addr(map, ~0);
#else            
	(void) map;
#endif
}

  
                                 
   
void
vm_map_set_jit_entitled(vm_map_t map)
{
#if defined (__arm64__)
	pmap_set_jit_entitled(map->pmap);
#else            
	(void) map;
#endif
}

  
                                              
   
void
vm_map_set_max_addr(vm_map_t map, vm_map_offset_t new_max_offset)
{
#if defined(__arm64__)
	vm_map_offset_t max_supported_offset = 0;
	vm_map_offset_t old_max_offset = map->max_offset;
	max_supported_offset = pmap_max_offset(vm_map_is_64bit(map), ARM_PMAP_MAX_OFFSET_JUMBO);

	new_max_offset = trunc_page(new_max_offset);

	                                                            
	if (old_max_offset >= new_max_offset) {
		return;
	}

	if (max_supported_offset < new_max_offset) {
		new_max_offset = max_supported_offset;
	}

	map->max_offset = new_max_offset;

	if (map->holes_list->prev->vme_end == old_max_offset) {
		  
                                                                          
     
		map->holes_list->prev->vme_end = map->max_offset;
	} else {
		  
                                                                 
                                            
     
		struct vm_map_links *new_hole = zalloc(vm_map_holes_zone);
		new_hole->start = old_max_offset;
		new_hole->end = map->max_offset;
		new_hole->prev = map->holes_list->prev;
		new_hole->next = (struct vm_map_entry *)map->holes_list;
		map->holes_list->prev->links.next = (struct vm_map_entry *)new_hole;
		map->holes_list->prev = (struct vm_map_entry *)new_hole;
	}
#else
	(void)map;
	(void)new_max_offset;
#endif
}

vm_map_offset_t
vm_compute_max_offset(boolean_t is64)
{
#if defined(__arm__) || defined(__arm64__)
	return pmap_max_offset(is64, ARM_PMAP_MAX_OFFSET_DEVICE);
#else
	return is64 ? (vm_map_offset_t)MACH_VM_MAX_ADDRESS : (vm_map_offset_t)VM_MAX_ADDRESS;
#endif
}

void
vm_map_get_max_aslr_slide_section(
	vm_map_t                map __unused,
	int64_t                 *max_sections,
	int64_t                 *section_size)
{
#if defined(__arm64__)
	*max_sections = 3;
	*section_size = ARM_TT_TWIG_SIZE;
#else
	*max_sections = 1;
	*section_size = 0;
#endif
}

uint64_t
vm_map_get_max_aslr_slide_pages(vm_map_t map)
{
#if defined(__arm64__)
	                                                                        
                                                                       
                                      
    
	return 1 << (24 - VM_MAP_PAGE_SHIFT(map));
#else
	return 1 << (vm_map_is_64bit(map) ? 16 : 8);
#endif
}

uint64_t
vm_map_get_max_loader_aslr_slide_pages(vm_map_t map)
{
#if defined(__arm64__)
	                                                                       
                                                
    
	return 1 << (22 - VM_MAP_PAGE_SHIFT(map));
#else
	return 1 << (vm_map_is_64bit(map) ? 16 : 8);
#endif
}

#ifndef __arm__
boolean_t
vm_map_is_64bit(
	vm_map_t map)
{
	return map->max_offset > ((vm_map_offset_t)VM_MAX_ADDRESS);
}
#endif

boolean_t
vm_map_has_hard_pagezero(
	vm_map_t        map,
	vm_map_offset_t pagezero_size)
{
	  
            
                                                                 
                                                                     
                                                                      
                                                                        
                                                                      
                                                                
                             
    
	return map->min_offset >= pagezero_size;
}

  
                                   
   
kern_return_t
vm_map_raise_max_offset(
	vm_map_t        map,
	vm_map_offset_t new_max_offset)
{
	kern_return_t   ret;

	vm_map_lock(map);
	ret = KERN_INVALID_ADDRESS;

	if (new_max_offset >= map->max_offset) {
		if (!vm_map_is_64bit(map)) {
			if (new_max_offset <= (vm_map_offset_t)VM_MAX_ADDRESS) {
				map->max_offset = new_max_offset;
				ret = KERN_SUCCESS;
			}
		} else {
			if (new_max_offset <= (vm_map_offset_t)MACH_VM_MAX_ADDRESS) {
				map->max_offset = new_max_offset;
				ret = KERN_SUCCESS;
			}
		}
	}

	vm_map_unlock(map);
	return ret;
}


  
                                   
                                               
   
kern_return_t
vm_map_raise_min_offset(
	vm_map_t        map,
	vm_map_offset_t new_min_offset)
{
	vm_map_entry_t  first_entry;

	new_min_offset = vm_map_round_page(new_min_offset,
	    VM_MAP_PAGE_MASK(map));

	vm_map_lock(map);

	if (new_min_offset < map->min_offset) {
		  
                                                          
                                                             
                                         
     
		vm_map_unlock(map);
		return KERN_INVALID_ADDRESS;
	}
	if (new_min_offset >= map->max_offset) {
		                                                  
		vm_map_unlock(map);
		return KERN_INVALID_ADDRESS;
	}

	first_entry = vm_map_first_entry(map);
	if (first_entry != vm_map_to_entry(map) &&
	    first_entry->vme_start < new_min_offset) {
		  
                                                    
                                                       
     
		vm_map_unlock(map);
		return KERN_NO_SPACE;
	}

	map->min_offset = new_min_offset;

	assert(map->holes_list);
	map->holes_list->start = new_min_offset;
	assert(new_min_offset < map->holes_list->end);

	vm_map_unlock(map);

	return KERN_SUCCESS;
}

  
                                                                                 
                                                                                     
                                                                                   
                                                       
   

void
vm_map_set_user_wire_limit(vm_map_t     map,
    vm_size_t    limit)
{
	map->user_wire_limit = limit;
}


void
vm_map_switch_protect(vm_map_t     map,
    boolean_t    val)
{
	vm_map_lock(map);
	map->switch_protect = val;
	vm_map_unlock(map);
}

extern int cs_process_enforcement_enable;
boolean_t
vm_map_cs_enforcement(
	vm_map_t map)
{
	if (cs_process_enforcement_enable) {
		return TRUE;
	}
	return map->cs_enforcement;
}

kern_return_t
vm_map_cs_wx_enable(
	vm_map_t map)
{
	return pmap_cs_allow_invalid(vm_map_pmap(map));
}

void
vm_map_cs_debugged_set(
	vm_map_t map,
	boolean_t val)
{
	vm_map_lock(map);
	map->cs_debugged = val;
	vm_map_unlock(map);
}

void
vm_map_cs_enforcement_set(
	vm_map_t map,
	boolean_t val)
{
	vm_map_lock(map);
	map->cs_enforcement = val;
	pmap_set_vm_map_cs_enforced(map->pmap, val);
	vm_map_unlock(map);
}

  
                                                                                    
                                                                                   
                      
   
void
vm_map_iokit_mapped_region(vm_map_t map, vm_size_t bytes)
{
	pmap_t pmap = vm_map_pmap(map);

	ledger_credit(pmap->ledger, task_ledgers.iokit_mapped, bytes);
	ledger_credit(pmap->ledger, task_ledgers.phys_footprint, bytes);
}

void
vm_map_iokit_unmapped_region(vm_map_t map, vm_size_t bytes)
{
	pmap_t pmap = vm_map_pmap(map);

	ledger_debit(pmap->ledger, task_ledgers.iokit_mapped, bytes);
	ledger_debit(pmap->ledger, task_ledgers.phys_footprint, bytes);
}

                                                    
#if CONFIG_DYNAMIC_CODE_SIGNING
kern_return_t
vm_map_sign(vm_map_t map,
    vm_map_offset_t start,
    vm_map_offset_t end)
{
	vm_map_entry_t entry;
	vm_page_t m;
	vm_object_t object;

	  
                                                                  
                                                                   
    
	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	vm_map_lock_read(map);

	if (!vm_map_lookup_entry(map, start, &entry) || entry->is_sub_map) {
		  
                                          
     
		vm_map_unlock_read(map);
		return KERN_INVALID_ADDRESS;
	}

	if ((entry->vme_start > start) || (entry->vme_end < end)) {
		  
                                                              
                              
     
		vm_map_unlock_read(map);
		return KERN_INVALID_ARGUMENT;
	}

	object = VME_OBJECT(entry);
	if (object == VM_OBJECT_NULL) {
		  
                                                     
     
		vm_map_unlock_read(map);
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_lock(object);
	vm_map_unlock_read(map);

	while (start < end) {
		uint32_t refmod;

		m = vm_page_lookup(object,
		    start - entry->vme_start + VME_OFFSET(entry));
		if (m == VM_PAGE_NULL) {
			                                                     
                                                        
			vm_object_unlock(object);
			return KERN_FAILURE;
		}
		                                   
		if (m->vmp_busy ||
		    (m->vmp_unusual && (m->vmp_error || m->vmp_restart || m->vmp_private || m->vmp_absent))) {
			vm_object_unlock(object);
			return KERN_FAILURE;
		}

		                                     
		                                                          
                        
		                                                     
		m->vmp_cs_validated = VMP_CS_ALL_TRUE;

		                                                               
                                                             
                                                                 
                 
		m->vmp_wpmapped = FALSE;
		refmod = pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m));

		                                                            
                    
		if ((refmod & VM_MEM_MODIFIED) && !m->vmp_dirty) {
			SET_PAGE_DIRTY(m, FALSE);
		}

		                         
		start += PAGE_SIZE;
	}
	vm_object_unlock(object);

	return KERN_SUCCESS;
}
#endif

kern_return_t
vm_map_partial_reap(vm_map_t map, unsigned int *reclaimed_resident, unsigned int *reclaimed_compressed)
{
	vm_map_entry_t  entry = VM_MAP_ENTRY_NULL;
	vm_map_entry_t next_entry;
	kern_return_t   kr = KERN_SUCCESS;
	vm_map_t        zap_map;

	vm_map_lock(map);

	  
                                                
                                 
    
	zap_map = vm_map_create(PMAP_NULL,
	    map->min_offset,
	    map->max_offset,
	    map->hdr.entries_pageable);

	if (zap_map == VM_MAP_NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}

	vm_map_set_page_shift(zap_map,
	    VM_MAP_PAGE_SHIFT(map));
	vm_map_disable_hole_optimization(zap_map);

	for (entry = vm_map_first_entry(map);
	    entry != vm_map_to_entry(map);
	    entry = next_entry) {
		next_entry = entry->vme_next;

		if (VME_OBJECT(entry) &&
		    !entry->is_sub_map &&
		    (VME_OBJECT(entry)->internal == TRUE) &&
		    (VME_OBJECT(entry)->ref_count == 1)) {
			*reclaimed_resident += VME_OBJECT(entry)->resident_page_count;
			*reclaimed_compressed += vm_compressor_pager_get_count(VME_OBJECT(entry)->pager);

			(void)vm_map_delete(map,
			    entry->vme_start,
			    entry->vme_end,
			    VM_MAP_REMOVE_SAVE_ENTRIES,
			    zap_map);
		}
	}

	vm_map_unlock(map);

	  
                                                          
                           
    
	if (zap_map != VM_MAP_NULL) {
		vm_map_destroy(zap_map, VM_MAP_REMOVE_NO_PMAP_CLEANUP);
		zap_map = VM_MAP_NULL;
	}

	return kr;
}


#if DEVELOPMENT || DEBUG

int
vm_map_disconnect_page_mappings(
	vm_map_t map,
	boolean_t do_unnest)
{
	vm_map_entry_t entry;
	int     page_count = 0;

	if (do_unnest == TRUE) {
#ifndef NO_NESTED_PMAP
		vm_map_lock(map);

		for (entry = vm_map_first_entry(map);
		    entry != vm_map_to_entry(map);
		    entry = entry->vme_next) {
			if (entry->is_sub_map && entry->use_pmap) {
				  
                                                              
                                                         
                                                                
                
       
				vm_map_clip_unnest(map, entry, entry->vme_start, entry->vme_end);
			}
		}
		vm_map_unlock(map);
#endif
	}
	vm_map_lock_read(map);

	page_count = map->pmap->stats.resident_count;

	for (entry = vm_map_first_entry(map);
	    entry != vm_map_to_entry(map);
	    entry = entry->vme_next) {
		if (!entry->is_sub_map && ((VME_OBJECT(entry) == 0) ||
		    (VME_OBJECT(entry)->phys_contiguous))) {
			continue;
		}
		if (entry->is_sub_map) {
			assert(!entry->use_pmap);
		}

		pmap_remove_options(map->pmap, entry->vme_start, entry->vme_end, 0);
	}
	vm_map_unlock_read(map);

	return page_count;
}

kern_return_t
vm_map_inject_error(vm_map_t map, vm_map_offset_t vaddr)
{
	vm_object_t object = NULL;
	vm_object_offset_t offset;
	vm_prot_t prot;
	boolean_t wired;
	vm_map_version_t version;
	vm_map_t real_map;
	int result = KERN_FAILURE;

	vaddr = vm_map_trunc_page(vaddr, PAGE_MASK);
	vm_map_lock(map);

	result = vm_map_lookup_locked(&map, vaddr, VM_PROT_READ,
	    OBJECT_LOCK_EXCLUSIVE, &version, &object, &offset, &prot, &wired,
	    NULL, &real_map, NULL);
	if (object == NULL) {
		result = KERN_MEMORY_ERROR;
	} else if (object->pager) {
		result = vm_compressor_pager_inject_error(object->pager,
		    offset);
	} else {
		result = KERN_MEMORY_PRESENT;
	}

	if (object != NULL) {
		vm_object_unlock(object);
	}

	if (real_map != map) {
		vm_map_unlock(real_map);
	}
	vm_map_unlock(map);

	return result;
}

#endif


#if CONFIG_FREEZE


extern struct freezer_context freezer_context_global;
AbsoluteTime c_freezer_last_yield_ts = 0;

extern unsigned int memorystatus_freeze_private_shared_pages_ratio;
extern unsigned int memorystatus_freeze_shared_mb_per_process_max;

kern_return_t
vm_map_freeze(
	task_t       task,
	unsigned int *purgeable_count,
	unsigned int *wired_count,
	unsigned int *clean_count,
	unsigned int *dirty_count,
	unsigned int dirty_budget,
	unsigned int *shared_count,
	int          *freezer_error_code,
	boolean_t    eval_only)
{
	vm_map_entry_t  entry2 = VM_MAP_ENTRY_NULL;
	kern_return_t   kr = KERN_SUCCESS;
	boolean_t       evaluation_phase = TRUE;
	vm_object_t     cur_shared_object = NULL;
	int             cur_shared_obj_ref_cnt = 0;
	unsigned int    dirty_private_count = 0, dirty_shared_count = 0, obj_pages_snapshot = 0;

	*purgeable_count = *wired_count = *clean_count = *dirty_count = *shared_count = 0;

	  
                                                  
                                                 
                                          
    
	vm_map_t map = task->map;

	vm_map_lock(map);

	assert(VM_CONFIG_COMPRESSOR_IS_PRESENT);

	if (vm_compressor_low_on_space() || vm_swap_low_on_space()) {
		if (vm_compressor_low_on_space()) {
			*freezer_error_code = FREEZER_ERROR_NO_COMPRESSOR_SPACE;
		}

		if (vm_swap_low_on_space()) {
			*freezer_error_code = FREEZER_ERROR_NO_SWAP_SPACE;
		}

		kr = KERN_NO_SPACE;
		goto done;
	}

	if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE == FALSE) {
		  
                                                       
                                           
     
		evaluation_phase = FALSE;

		if (eval_only == TRUE) {
			  
                                       
                              
      
			*freezer_error_code = FREEZER_ERROR_GENERIC;
			kr = KERN_INVALID_ARGUMENT;
			goto done;
		}

		freezer_context_global.freezer_ctx_uncompressed_pages = 0;
		clock_get_uptime(&c_freezer_last_yield_ts);
	}
again:

	for (entry2 = vm_map_first_entry(map);
	    entry2 != vm_map_to_entry(map);
	    entry2 = entry2->vme_next) {
		vm_object_t     src_object = VME_OBJECT(entry2);

		if (src_object &&
		    !entry2->is_sub_map &&
		    !src_object->phys_contiguous) {
			                                                                                  

			if (src_object->internal == TRUE) {
				if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
					  
                                                               
                                                             
                                                          
                                        
        

					if ((src_object->purgable == VM_PURGABLE_EMPTY) || (src_object->purgable == VM_PURGABLE_VOLATILE)) {
						  
                                                                                 
                                                                                 
                                                                                        
                                                                                     
         
						if (evaluation_phase == FALSE &&
						    (src_object->purgable == VM_PURGABLE_VOLATILE) &&
						    (src_object->ref_count == 1)) {
							vm_object_lock(src_object);
							vm_object_purge(src_object, 0);
							vm_object_unlock(src_object);
						}
						continue;
					}

					  
                                                                
                                                               
                                          
       
                                                                           
                                                                         
                                                                
        

					if (src_object->ref_count > 1) {
						if (src_object != cur_shared_object) {
							obj_pages_snapshot = (src_object->resident_page_count - src_object->wired_page_count) + vm_compressor_pager_get_count(src_object->pager);
							dirty_shared_count += obj_pages_snapshot;

							cur_shared_object = src_object;
							cur_shared_obj_ref_cnt = 1;
							continue;
						} else {
							cur_shared_obj_ref_cnt++;
							if (src_object->ref_count == cur_shared_obj_ref_cnt) {
								  
                                                                  
                                                                      
                         
           

								dirty_shared_count -= obj_pages_snapshot;
								dirty_private_count += obj_pages_snapshot;
							} else {
								continue;
							}
						}
					}


					if (src_object->ref_count == 1) {
						dirty_private_count += (src_object->resident_page_count - src_object->wired_page_count) + vm_compressor_pager_get_count(src_object->pager);
					}

					if (evaluation_phase == TRUE) {
						continue;
					}
				}

				uint32_t paged_out_count = vm_object_compressed_freezer_pageout(src_object, dirty_budget);
				*wired_count += src_object->wired_page_count;

				if (vm_compressor_low_on_space() || vm_swap_low_on_space()) {
					if (vm_compressor_low_on_space()) {
						*freezer_error_code = FREEZER_ERROR_NO_COMPRESSOR_SPACE;
					}

					if (vm_swap_low_on_space()) {
						*freezer_error_code = FREEZER_ERROR_NO_SWAP_SPACE;
					}

					kr = KERN_NO_SPACE;
					break;
				}
				if (paged_out_count >= dirty_budget) {
					break;
				}
				dirty_budget -= paged_out_count;
			}
		}
	}

	*shared_count = (unsigned int) ((dirty_shared_count * PAGE_SIZE_64) / (1024 * 1024ULL));
	if (evaluation_phase) {
		unsigned int shared_pages_threshold = (memorystatus_freeze_shared_mb_per_process_max * 1024 * 1024ULL) / PAGE_SIZE_64;

		if (dirty_shared_count > shared_pages_threshold) {
			*freezer_error_code = FREEZER_ERROR_EXCESS_SHARED_MEMORY;
			kr = KERN_FAILURE;
			goto done;
		}

		if (dirty_shared_count &&
		    ((dirty_private_count / dirty_shared_count) < memorystatus_freeze_private_shared_pages_ratio)) {
			*freezer_error_code = FREEZER_ERROR_LOW_PRIVATE_SHARED_RATIO;
			kr = KERN_FAILURE;
			goto done;
		}

		evaluation_phase = FALSE;
		dirty_shared_count = dirty_private_count = 0;

		freezer_context_global.freezer_ctx_uncompressed_pages = 0;
		clock_get_uptime(&c_freezer_last_yield_ts);

		if (eval_only) {
			kr = KERN_SUCCESS;
			goto done;
		}

		vm_purgeable_purge_task_owned(task);

		goto again;
	} else {
		kr = KERN_SUCCESS;
	}

done:
	vm_map_unlock(map);

	if ((eval_only == FALSE) && (kr == KERN_SUCCESS)) {
		vm_object_compressed_freezer_done();
	}
	return kr;
}

#endif

  
                                          
  
                                                                            
                                                                            
            
  
                                                                      
                                                                    
                              
                          
                     
                        
                  
                                                       
                     
                                    
  
                               
   
boolean_t
vm_map_entry_should_cow_for_true_share(
	vm_map_entry_t  entry)
{
	vm_object_t     object;

	if (entry->is_sub_map) {
		                                         
		return FALSE;
	}

	if (entry->needs_copy) {
		                                          
		return FALSE;
	}

	if (VME_ALIAS(entry) != VM_MEMORY_MALLOC &&
	    VME_ALIAS(entry) != VM_MEMORY_MALLOC_SMALL) {
		                                                       
		return FALSE;
	}

	if (entry->wired_count) {
		                                          
		vm_counters.should_cow_but_wired++;
		return FALSE;
	}

	object = VME_OBJECT(entry);

	if (object == VM_OBJECT_NULL) {
		                      
		return FALSE;
	}

	if (!object->internal) {
		                            
		return FALSE;
	}

	if (object->copy_strategy != MEMORY_OBJECT_COPY_SYMMETRIC) {
		                                   
		return FALSE;
	}

	if (object->true_share) {
		                                              
		return FALSE;
	}

	if (VME_ALIAS(entry) == VM_MEMORY_MALLOC &&
	    object->vo_size != ANON_CHUNK_SIZE) {
		                                                              
		return FALSE;
	}

	if (VME_ALIAS(entry) == VM_MEMORY_MALLOC_SMALL &&
	    object->vo_size != 2048 * 4096) {
		                                   
		return FALSE;
	}

	  
                                                                                   
                                                                                  
                                                                               
                                                       
    
	return TRUE;
}

vm_map_offset_t
vm_map_round_page_mask(
	vm_map_offset_t offset,
	vm_map_offset_t mask)
{
	return VM_MAP_ROUND_PAGE(offset, mask);
}

vm_map_offset_t
vm_map_trunc_page_mask(
	vm_map_offset_t offset,
	vm_map_offset_t mask)
{
	return VM_MAP_TRUNC_PAGE(offset, mask);
}

boolean_t
vm_map_page_aligned(
	vm_map_offset_t offset,
	vm_map_offset_t mask)
{
	return ((offset) & mask) == 0;
}

int
vm_map_page_shift(
	vm_map_t map)
{
	return VM_MAP_PAGE_SHIFT(map);
}

int
vm_map_page_size(
	vm_map_t map)
{
	return VM_MAP_PAGE_SIZE(map);
}

vm_map_offset_t
vm_map_page_mask(
	vm_map_t map)
{
	return VM_MAP_PAGE_MASK(map);
}

kern_return_t
vm_map_set_page_shift(
	vm_map_t        map,
	int             pageshift)
{
	if (map->hdr.nentries != 0) {
		                                  
		return KERN_FAILURE;
	}

	map->hdr.page_shift = pageshift;

	return KERN_SUCCESS;
}

kern_return_t
vm_map_query_volatile(
	vm_map_t        map,
	mach_vm_size_t  *volatile_virtual_size_p,
	mach_vm_size_t  *volatile_resident_size_p,
	mach_vm_size_t  *volatile_compressed_size_p,
	mach_vm_size_t  *volatile_pmap_size_p,
	mach_vm_size_t  *volatile_compressed_pmap_size_p)
{
	mach_vm_size_t  volatile_virtual_size;
	mach_vm_size_t  volatile_resident_count;
	mach_vm_size_t  volatile_compressed_count;
	mach_vm_size_t  volatile_pmap_count;
	mach_vm_size_t  volatile_compressed_pmap_count;
	mach_vm_size_t  resident_count;
	vm_map_entry_t  entry;
	vm_object_t     object;

	                                    

	volatile_virtual_size = 0;
	volatile_resident_count = 0;
	volatile_compressed_count = 0;
	volatile_pmap_count = 0;
	volatile_compressed_pmap_count = 0;

	for (entry = vm_map_first_entry(map);
	    entry != vm_map_to_entry(map);
	    entry = entry->vme_next) {
		mach_vm_size_t  pmap_resident_bytes, pmap_compressed_bytes;

		if (entry->is_sub_map) {
			continue;
		}
		if (!(entry->protection & VM_PROT_WRITE)) {
			continue;
		}
		object = VME_OBJECT(entry);
		if (object == VM_OBJECT_NULL) {
			continue;
		}
		if (object->purgable != VM_PURGABLE_VOLATILE &&
		    object->purgable != VM_PURGABLE_EMPTY) {
			continue;
		}
		if (VME_OFFSET(entry)) {
			  
                                                        
                                                        
                                                         
                                                         
                                                      
      
			continue;
		}
		resident_count = object->resident_page_count;
		if ((VME_OFFSET(entry) / PAGE_SIZE) >= resident_count) {
			resident_count = 0;
		} else {
			resident_count -= (VME_OFFSET(entry) / PAGE_SIZE);
		}

		volatile_virtual_size += entry->vme_end - entry->vme_start;
		volatile_resident_count += resident_count;
		if (object->pager) {
			volatile_compressed_count +=
			    vm_compressor_pager_get_count(object->pager);
		}
		pmap_compressed_bytes = 0;
		pmap_resident_bytes =
		    pmap_query_resident(map->pmap,
		    entry->vme_start,
		    entry->vme_end,
		    &pmap_compressed_bytes);
		volatile_pmap_count += (pmap_resident_bytes / PAGE_SIZE);
		volatile_compressed_pmap_count += (pmap_compressed_bytes
		    / PAGE_SIZE);
	}

	                                   

	*volatile_virtual_size_p = volatile_virtual_size;
	*volatile_resident_size_p = volatile_resident_count * PAGE_SIZE;
	*volatile_compressed_size_p = volatile_compressed_count * PAGE_SIZE;
	*volatile_pmap_size_p = volatile_pmap_count * PAGE_SIZE;
	*volatile_compressed_pmap_size_p = volatile_compressed_pmap_count * PAGE_SIZE;

	return KERN_SUCCESS;
}

void
vm_map_sizes(vm_map_t map,
    vm_map_size_t * psize,
    vm_map_size_t * pfree,
    vm_map_size_t * plargest_free)
{
	vm_map_entry_t  entry;
	vm_map_offset_t prev;
	vm_map_size_t   free, total_free, largest_free;
	boolean_t       end;

	if (!map) {
		*psize = *pfree = *plargest_free = 0;
		return;
	}
	total_free = largest_free = 0;

	vm_map_lock_read(map);
	if (psize) {
		*psize = map->max_offset - map->min_offset;
	}

	prev = map->min_offset;
	for (entry = vm_map_first_entry(map);; entry = entry->vme_next) {
		end = (entry == vm_map_to_entry(map));

		if (end) {
			free = entry->vme_end   - prev;
		} else {
			free = entry->vme_start - prev;
		}

		total_free += free;
		if (free > largest_free) {
			largest_free = free;
		}

		if (end) {
			break;
		}
		prev = entry->vme_end;
	}
	vm_map_unlock_read(map);
	if (pfree) {
		*pfree = total_free;
	}
	if (plargest_free) {
		*plargest_free = largest_free;
	}
}

#if VM_SCAN_FOR_SHADOW_CHAIN
int vm_map_shadow_max(vm_map_t map);
int
vm_map_shadow_max(
	vm_map_t map)
{
	int             shadows, shadows_max;
	vm_map_entry_t  entry;
	vm_object_t     object, next_object;

	if (map == NULL) {
		return 0;
	}

	shadows_max = 0;

	vm_map_lock_read(map);

	for (entry = vm_map_first_entry(map);
	    entry != vm_map_to_entry(map);
	    entry = entry->vme_next) {
		if (entry->is_sub_map) {
			continue;
		}
		object = VME_OBJECT(entry);
		if (object == NULL) {
			continue;
		}
		vm_object_lock_shared(object);
		for (shadows = 0;
		    object->shadow != NULL;
		    shadows++, object = next_object) {
			next_object = object->shadow;
			vm_object_lock_shared(next_object);
			vm_object_unlock(object);
		}
		vm_object_unlock(object);
		if (shadows > shadows_max) {
			shadows_max = shadows;
		}
	}

	vm_map_unlock_read(map);

	return shadows_max;
}
#endif                               

void
vm_commit_pagezero_status(vm_map_t lmap)
{
	pmap_advise_pagezero_range(lmap->pmap, lmap->min_offset);
}

#if XNU_TARGET_OS_OSX
void
vm_map_set_high_start(
	vm_map_t        map,
	vm_map_offset_t high_start)
{
	map->vmmap_high_start = high_start;
}
#endif                        


  
                          
  
                                                                            
                                                                
                                                                        
                                                                  
  
                                                                             
                                                                             
                                                                              
                                                                           
                                        
  
                                               
  
                                                                           
              
                                                                
                                                                
                                                                
                                                                
                                                                
                                                                
                                                                
                                                                
                                                                
                                                                
                                                                
                                                                
                                                                
                                                                          
                                                                
                                                                
                                                                
                                                                
                                                                
                            
                                                                
                                                                
                                                                
                                                                
                                                                
                            
                                                                
                                                                
                                                                
  
         
                                                                
                                                                            
                                                                    
                   
                                                                  
                                                         
                                                       
                                          
  
                                                                           
                                                                               
                                                                           
                                                                         
                                                                        
                                                                           
                                                                              
                                                     
  
                                                                              
                                                                            
                                                  
   

uint64_t vm_map_corpse_footprint_count = 0;
uint64_t vm_map_corpse_footprint_size_avg = 0;
uint64_t vm_map_corpse_footprint_size_max = 0;
uint64_t vm_map_corpse_footprint_full = 0;
uint64_t vm_map_corpse_footprint_no_buf = 0;

struct vm_map_corpse_footprint_header {
	vm_size_t       cf_size;                                   
	uint32_t        cf_last_region;                                      
	union {
		uint32_t cfu_last_zeroes;                    
                                                               
                                                    
		uint32_t cfu_hint_region;                  
                                                                 
#define cf_last_zeroes cfu.cfu_last_zeroes
#define cf_hint_region cfu.cfu_hint_region
	} cfu;
};
typedef uint8_t cf_disp_t;
struct vm_map_corpse_footprint_region {
	vm_map_offset_t cfr_vaddr;                                        
	uint32_t        cfr_num_pages;                                        
	cf_disp_t   cfr_disposition[0];                               
} __attribute__((packed));

static cf_disp_t
vm_page_disposition_to_cf_disp(
	int disposition)
{
	assert(sizeof(cf_disp_t) == 1);
	                                                 
	if (disposition & VM_PAGE_QUERY_PAGE_REUSABLE) {
		disposition |= VM_PAGE_QUERY_PAGE_FICTITIOUS;
	}
	                                 
	return (cf_disp_t) disposition;
}

static int
vm_page_cf_disp_to_disposition(
	cf_disp_t cf_disp)
{
	int disposition;

	assert(sizeof(cf_disp_t) == 1);
	disposition = (int) cf_disp;
	                                       
	if (cf_disp & VM_PAGE_QUERY_PAGE_FICTITIOUS) {
		disposition |= VM_PAGE_QUERY_PAGE_REUSABLE;
		disposition &= ~VM_PAGE_QUERY_PAGE_FICTITIOUS;
	}
	return disposition;
}

  
                                      
                                                                   
  
                                                                           
   
static struct vm_map_corpse_footprint_region *
vm_map_corpse_footprint_new_region(
	struct vm_map_corpse_footprint_header *footprint_header)
{
	uintptr_t       footprint_edge;
	uint32_t        new_region_offset;
	struct vm_map_corpse_footprint_region *footprint_region;
	struct vm_map_corpse_footprint_region *new_footprint_region;

	footprint_edge = ((uintptr_t)footprint_header +
	    footprint_header->cf_size);
	footprint_region = ((struct vm_map_corpse_footprint_region *)
	    ((char *)footprint_header +
	    footprint_header->cf_last_region));
	assert((uintptr_t)footprint_region + sizeof(*footprint_region) <=
	    footprint_edge);

	                                                   
	assert(footprint_region->cfr_num_pages >=
	    footprint_header->cf_last_zeroes);
	footprint_region->cfr_num_pages -=
	    footprint_header->cf_last_zeroes;
	footprint_header->cf_last_zeroes = 0;

	                                         
	if (footprint_region->cfr_num_pages == 0) {
		return footprint_region;
	}

	                                  
	new_region_offset = footprint_header->cf_last_region;
	new_region_offset += sizeof(*footprint_region);
	new_region_offset += (footprint_region->cfr_num_pages * sizeof(cf_disp_t));
	new_region_offset = roundup(new_region_offset, sizeof(int));

	                                        
	if (((uintptr_t)footprint_header +
	    new_region_offset +
	    sizeof(*footprint_region)) >=
	    footprint_edge) {
		                                  
		return NULL;
	}

	                                            
	footprint_header->cf_last_region = new_region_offset;

	new_footprint_region = (struct vm_map_corpse_footprint_region *)
	    ((char *)footprint_header +
	    footprint_header->cf_last_region);
	new_footprint_region->cfr_vaddr = 0;
	new_footprint_region->cfr_num_pages = 0;
	                                           

	return new_footprint_region;
}

  
                                   
                                                                 
                                                 
   
kern_return_t
vm_map_corpse_footprint_collect(
	vm_map_t        old_map,
	vm_map_entry_t  old_entry,
	vm_map_t        new_map)
{
	vm_map_offset_t va;
	kern_return_t   kr;
	struct vm_map_corpse_footprint_header *footprint_header;
	struct vm_map_corpse_footprint_region *footprint_region;
	struct vm_map_corpse_footprint_region *new_footprint_region;
	cf_disp_t       *next_disp_p;
	uintptr_t       footprint_edge;
	uint32_t        num_pages_tmp;
	int             effective_page_size;

	effective_page_size = MIN(PAGE_SIZE, VM_MAP_PAGE_SIZE(old_map));

	va = old_entry->vme_start;

	vm_map_lock_assert_exclusive(old_map);
	vm_map_lock_assert_exclusive(new_map);

	assert(new_map->has_corpse_footprint);
	assert(!old_map->has_corpse_footprint);
	if (!new_map->has_corpse_footprint ||
	    old_map->has_corpse_footprint) {
		  
                                                 
                                                           
     
		return KERN_NOT_SUPPORTED;
	}

	if (new_map->vmmap_corpse_footprint == NULL) {
		vm_offset_t     buf;
		vm_size_t       buf_size;

		buf = 0;
		buf_size = (sizeof(*footprint_header) +
		    (old_map->hdr.nentries
		    *
		    (sizeof(*footprint_region) +
		    +3))                                                     
		    +
		    ((old_map->size / effective_page_size)
		    *
		    sizeof(cf_disp_t)));                                     
                                                                                                  
		buf_size = round_page(buf_size);

		                                                           
                         

		                                          
#if XNU_TARGET_OS_OSX
#define VM_MAP_CORPSE_FOOTPRINT_INFO_MAX_SIZE   (8*1024*1024)            
#else                        
#define VM_MAP_CORPSE_FOOTPRINT_INFO_MAX_SIZE   (256*1024)                 
#endif                        
		if (buf_size > VM_MAP_CORPSE_FOOTPRINT_INFO_MAX_SIZE) {
			buf_size = VM_MAP_CORPSE_FOOTPRINT_INFO_MAX_SIZE;
		}

		  
                                                               
                                      
     
		kr = kernel_memory_allocate(kernel_map,
		    &buf,
		    (buf_size
		    + PAGE_SIZE),                                                   
		    0,                                   
		    KMA_PAGEABLE | KMA_GUARD_LAST,
		    VM_KERN_MEMORY_DIAG);
		if (kr != KERN_SUCCESS) {
			vm_map_corpse_footprint_no_buf++;
			return kr;
		}

		                                      
		footprint_header = (struct vm_map_corpse_footprint_header *)buf;
		new_map->vmmap_corpse_footprint = footprint_header;

		footprint_header->cf_size = buf_size;
		footprint_header->cf_last_region =
		    sizeof(*footprint_header);
		footprint_header->cf_last_zeroes = 0;

		footprint_region = (struct vm_map_corpse_footprint_region *)
		    ((char *)footprint_header +
		    footprint_header->cf_last_region);
		footprint_region->cfr_vaddr = 0;
		footprint_region->cfr_num_pages = 0;
	} else {
		                                     
		footprint_header = (struct vm_map_corpse_footprint_header *)
		    new_map->vmmap_corpse_footprint;
		footprint_region = (struct vm_map_corpse_footprint_region *)
		    ((char *)footprint_header +
		    footprint_header->cf_last_region);
	}
	footprint_edge = ((uintptr_t)footprint_header +
	    footprint_header->cf_size);

	if ((footprint_region->cfr_vaddr +
	    (((vm_map_offset_t)footprint_region->cfr_num_pages) *
	    effective_page_size))
	    != old_entry->vme_start) {
		uint64_t num_pages_delta, num_pages_delta_size;
		uint32_t region_offset_delta_size;

		  
                                             
                                                        
                       
     
		                                             
		num_pages_delta = ((old_entry->vme_start -
		    footprint_region->cfr_vaddr) / effective_page_size)
		    - footprint_region->cfr_num_pages;
		num_pages_delta_size = num_pages_delta * sizeof(cf_disp_t);
		                                                  
		region_offset_delta_size =
		    (sizeof(*footprint_region) +
		    roundup(((footprint_region->cfr_num_pages -
		    footprint_header->cf_last_zeroes) * sizeof(cf_disp_t)),
		    sizeof(int)) -
		    ((footprint_region->cfr_num_pages -
		    footprint_header->cf_last_zeroes) * sizeof(cf_disp_t)));
                                                                                                                                                                                                                                                                                             
		if (region_offset_delta_size < num_pages_delta_size ||
		    os_add3_overflow(footprint_region->cfr_num_pages,
		    (uint32_t) num_pages_delta,
		    1,
		    &num_pages_tmp)) {
			  
                                                     
                                                   
                                                        
                                                     
                                                      
                           
     
                                                          
                                                       
                                          
      
                                                               
			new_footprint_region =
			    vm_map_corpse_footprint_new_region(footprint_header);
			                                              
			if (new_footprint_region == NULL) {
				goto over_the_edge;
			}
			footprint_region = new_footprint_region;
			                                    
			footprint_region->cfr_vaddr = old_entry->vme_start;
			footprint_region->cfr_num_pages = 0;
		} else {
			  
                                                    
            
      
                                                             
			for (; num_pages_delta > 0; num_pages_delta--) {
				next_disp_p = (cf_disp_t *)
				    ((uintptr_t) footprint_region +
				    sizeof(*footprint_region));
				next_disp_p += footprint_region->cfr_num_pages;
				                                              
				if ((uintptr_t)next_disp_p >= footprint_edge) {
					goto over_the_edge;
				}
				                                                
				footprint_region->cfr_num_pages++;
				*next_disp_p = (cf_disp_t) 0;
				footprint_header->cf_last_zeroes++;
			}
		}
	}

	for (va = old_entry->vme_start;
	    va < old_entry->vme_end;
	    va += effective_page_size) {
		int             disposition;
		cf_disp_t       cf_disp;

		vm_map_footprint_query_page_info(old_map,
		    old_entry,
		    va,
		    &disposition);
		cf_disp = vm_page_disposition_to_cf_disp(disposition);

                                                                                                              

		if (cf_disp == 0 && footprint_region->cfr_num_pages == 0) {
			  
                                            
                                        
      
			footprint_region->cfr_vaddr += effective_page_size;
			continue;
		}

		                                            
		if (os_add_overflow(footprint_region->cfr_num_pages, 1,
		    &num_pages_tmp)) {
			                                   
			new_footprint_region =
			    vm_map_corpse_footprint_new_region(
				footprint_header);
			if (new_footprint_region == NULL) {
				goto over_the_edge;
			}
			footprint_region = new_footprint_region;
			footprint_region->cfr_vaddr = va;
			footprint_region->cfr_num_pages = 0;
		}

		next_disp_p = (cf_disp_t *) ((uintptr_t) footprint_region +
		    sizeof(*footprint_region));
		next_disp_p += footprint_region->cfr_num_pages;
		                                              
		if ((uintptr_t)next_disp_p >= footprint_edge) {
			goto over_the_edge;
		}
		                           
		*next_disp_p = cf_disp;
		footprint_region->cfr_num_pages++;

		if (cf_disp != 0) {
			                                                  
			footprint_header->cf_last_zeroes = 0;
			          
			continue;
		}

		                                                    
		footprint_header->cf_last_zeroes++;
		if ((footprint_header->cf_last_zeroes +
		    roundup(((footprint_region->cfr_num_pages -
		    footprint_header->cf_last_zeroes) * sizeof(cf_disp_t)) &
		    (sizeof(int) - 1),
		    sizeof(int))) <
		    (sizeof(*footprint_header))) {
			  
                                                       
                                                         
                                                         
                                                          
                                          
      
			continue;
		}
		  
                                                             
                         
     
		new_footprint_region =
		    vm_map_corpse_footprint_new_region(footprint_header);
		if (new_footprint_region == NULL) {
			goto over_the_edge;
		}
		footprint_region = new_footprint_region;
		                                            
		footprint_region->cfr_num_pages = 0;
		                                   
		footprint_region->cfr_vaddr = va + effective_page_size;
	}

	return KERN_SUCCESS;

over_the_edge:
                                                                         
	vm_map_corpse_footprint_full++;
	return KERN_RESOURCE_SHORTAGE;
}

  
                                        
                                                                     
                                                                   
                
   
void
vm_map_corpse_footprint_collect_done(
	vm_map_t        new_map)
{
	struct vm_map_corpse_footprint_header *footprint_header;
	struct vm_map_corpse_footprint_region *footprint_region;
	vm_size_t       buf_size, actual_size;
	kern_return_t   kr;

	assert(new_map->has_corpse_footprint);
	if (!new_map->has_corpse_footprint ||
	    new_map->vmmap_corpse_footprint == NULL) {
		return;
	}

	footprint_header = (struct vm_map_corpse_footprint_header *)
	    new_map->vmmap_corpse_footprint;
	buf_size = footprint_header->cf_size;

	footprint_region = (struct vm_map_corpse_footprint_region *)
	    ((char *)footprint_header +
	    footprint_header->cf_last_region);

	                                               
	assert(footprint_region->cfr_num_pages >= footprint_header->cf_last_zeroes);
	footprint_region->cfr_num_pages -= footprint_header->cf_last_zeroes;
	footprint_header->cf_last_zeroes = 0;

	actual_size = (vm_size_t)(footprint_header->cf_last_region +
	    sizeof(*footprint_region) +
	    (footprint_region->cfr_num_pages * sizeof(cf_disp_t)));

                                                                                                                    
	vm_map_corpse_footprint_size_avg =
	    (((vm_map_corpse_footprint_size_avg *
	    vm_map_corpse_footprint_count) +
	    actual_size) /
	    (vm_map_corpse_footprint_count + 1));
	vm_map_corpse_footprint_count++;
	if (actual_size > vm_map_corpse_footprint_size_max) {
		vm_map_corpse_footprint_size_max = actual_size;
	}

	actual_size = round_page(actual_size);
	if (buf_size > actual_size) {
		kr = vm_deallocate(kernel_map,
		    ((vm_address_t)footprint_header +
		    actual_size +
		    PAGE_SIZE),                                          
		    (buf_size - actual_size));
		assertf(kr == KERN_SUCCESS,
		    "trim: footprint_header %p buf_size 0x%llx actual_size 0x%llx kr=0x%x\n",
		    footprint_header,
		    (uint64_t) buf_size,
		    (uint64_t) actual_size,
		    kr);
		kr = vm_protect(kernel_map,
		    ((vm_address_t)footprint_header +
		    actual_size),
		    PAGE_SIZE,
		    FALSE,                              
		    VM_PROT_NONE);
		assertf(kr == KERN_SUCCESS,
		    "guard: footprint_header %p buf_size 0x%llx actual_size 0x%llx kr=0x%x\n",
		    footprint_header,
		    (uint64_t) buf_size,
		    (uint64_t) actual_size,
		    kr);
	}

	footprint_header->cf_size = actual_size;
}

  
                                           
                                                                   
                                
  
                                                                                    
   
kern_return_t
vm_map_corpse_footprint_query_page_info(
	vm_map_t        map,
	vm_map_offset_t va,
	int             *disposition_p)
{
	struct vm_map_corpse_footprint_header *footprint_header;
	struct vm_map_corpse_footprint_region *footprint_region;
	uint32_t        footprint_region_offset;
	vm_map_offset_t region_start, region_end;
	int             disp_idx;
	kern_return_t   kr;
	int             effective_page_size;
	cf_disp_t       cf_disp;

	if (!map->has_corpse_footprint) {
		*disposition_p = 0;
		kr = KERN_INVALID_ARGUMENT;
		goto done;
	}

	footprint_header = map->vmmap_corpse_footprint;
	if (footprint_header == NULL) {
		*disposition_p = 0;
                                                                                                                               
		kr = KERN_INVALID_ARGUMENT;
		goto done;
	}

	                                                  
	footprint_region_offset = footprint_header->cf_hint_region;

	effective_page_size = MIN(PAGE_SIZE, VM_MAP_PAGE_SIZE(map));

lookup_again:
	if (footprint_region_offset < sizeof(*footprint_header)) {
		                                         
		footprint_region_offset = sizeof(*footprint_header);
	}
	if (footprint_region_offset >= footprint_header->cf_last_region) {
		                                             
		footprint_region_offset = sizeof(*footprint_header);
	}
	footprint_region = (struct vm_map_corpse_footprint_region *)
	    ((char *)footprint_header + footprint_region_offset);
	region_start = footprint_region->cfr_vaddr;
	region_end = (region_start +
	    ((vm_map_offset_t)(footprint_region->cfr_num_pages) *
	    effective_page_size));
	if (va < region_start &&
	    footprint_region_offset != sizeof(*footprint_header)) {
		                                             

		                                       
		footprint_header->cf_hint_region = sizeof(*footprint_header);
		                                       
		footprint_region_offset = sizeof(*footprint_header);
		goto lookup_again;
	}

	while (va >= region_end) {
		if (footprint_region_offset >= footprint_header->cf_last_region) {
			break;
		}
		                              
		footprint_region_offset += sizeof(*footprint_region);
		                                         
		footprint_region_offset += (footprint_region->cfr_num_pages * sizeof(cf_disp_t));
		                                 
		footprint_region_offset =
		    roundup(footprint_region_offset,
		    sizeof(int));
		footprint_region = (struct vm_map_corpse_footprint_region *)
		    ((char *)footprint_header + footprint_region_offset);
		region_start = footprint_region->cfr_vaddr;
		region_end = (region_start +
		    ((vm_map_offset_t)(footprint_region->cfr_num_pages) *
		    effective_page_size));
	}
	if (va < region_start || va >= region_end) {
		                    
		*disposition_p = 0;
                                                                                                                               
		kr = KERN_SUCCESS;
		goto done;
	}

	                                                                        
	footprint_header->cf_hint_region = footprint_region_offset;

	                                                  
	disp_idx = (int) ((va - footprint_region->cfr_vaddr) / effective_page_size);
	cf_disp = footprint_region->cfr_disposition[disp_idx];
	*disposition_p = vm_page_cf_disp_to_disposition(cf_disp);
	kr = KERN_SUCCESS;
done:
                                                                                                                              
	                                                                                                                                
	DTRACE_VM4(footprint_query_page_info,
	    vm_map_t, map,
	    vm_map_offset_t, va,
	    int, *disposition_p,
	    kern_return_t, kr);

	return kr;
}

void
vm_map_corpse_footprint_destroy(
	vm_map_t        map)
{
	if (map->has_corpse_footprint &&
	    map->vmmap_corpse_footprint != 0) {
		struct vm_map_corpse_footprint_header *footprint_header;
		vm_size_t buf_size;
		kern_return_t kr;

		footprint_header = map->vmmap_corpse_footprint;
		buf_size = footprint_header->cf_size;
		kr = vm_deallocate(kernel_map,
		    (vm_offset_t) map->vmmap_corpse_footprint,
		    ((vm_size_t) buf_size
		    + PAGE_SIZE));                                          
		assertf(kr == KERN_SUCCESS, "kr=0x%x\n", kr);
		map->vmmap_corpse_footprint = 0;
		map->has_corpse_footprint = FALSE;
	}
}

  
                                 
                                                                          
                                             
   
void
vm_map_copy_footprint_ledgers(
	task_t  old_task,
	task_t  new_task)
{
	vm_map_copy_ledger(old_task, new_task, task_ledgers.phys_footprint);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.purgeable_nonvolatile);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.purgeable_nonvolatile_compressed);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.internal);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.internal_compressed);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.iokit_mapped);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.alternate_accounting);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.alternate_accounting_compressed);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.page_table);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.tagged_footprint);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.tagged_footprint_compressed);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.network_nonvolatile);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.network_nonvolatile_compressed);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.media_footprint);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.media_footprint_compressed);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.graphics_footprint);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.graphics_footprint_compressed);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.neural_footprint);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.neural_footprint_compressed);
	vm_map_copy_ledger(old_task, new_task, task_ledgers.wired_mem);
}

  
                      
                                                     
   
void
vm_map_copy_ledger(
	task_t  old_task,
	task_t  new_task,
	int     ledger_entry)
{
	ledger_amount_t old_balance, new_balance, delta;

	assert(new_task->map->has_corpse_footprint);
	if (!new_task->map->has_corpse_footprint) {
		return;
	}

	                                                                    
	ledger_disable_panic_on_negative(new_task->ledger,
	    ledger_entry);

	                                           
	ledger_get_balance(old_task->ledger,
	    ledger_entry,
	    &old_balance);
	ledger_get_balance(new_task->ledger,
	    ledger_entry,
	    &new_balance);
	if (new_balance == old_balance) {
		                      
	} else if (new_balance > old_balance) {
		                                    
		delta = new_balance - old_balance;
		ledger_debit(new_task->ledger,
		    ledger_entry,
		    delta);
	} else {
		                                    
		delta = old_balance - new_balance;
		ledger_credit(new_task->ledger,
		    ledger_entry,
		    delta);
	}
}

#if MACH_ASSERT

extern int pmap_ledgers_panic;
extern int pmap_ledgers_panic_leeway;

#define LEDGER_DRIFT(__LEDGER)                    \
	int             __LEDGER##_over;          \
	ledger_amount_t __LEDGER##_over_total;    \
	ledger_amount_t __LEDGER##_over_max;      \
	int             __LEDGER##_under;         \
	ledger_amount_t __LEDGER##_under_total;   \
	ledger_amount_t __LEDGER##_under_max

struct {
	uint64_t        num_pmaps_checked;

	LEDGER_DRIFT(phys_footprint);
	LEDGER_DRIFT(internal);
	LEDGER_DRIFT(internal_compressed);
	LEDGER_DRIFT(iokit_mapped);
	LEDGER_DRIFT(alternate_accounting);
	LEDGER_DRIFT(alternate_accounting_compressed);
	LEDGER_DRIFT(page_table);
	LEDGER_DRIFT(purgeable_volatile);
	LEDGER_DRIFT(purgeable_nonvolatile);
	LEDGER_DRIFT(purgeable_volatile_compressed);
	LEDGER_DRIFT(purgeable_nonvolatile_compressed);
	LEDGER_DRIFT(tagged_nofootprint);
	LEDGER_DRIFT(tagged_footprint);
	LEDGER_DRIFT(tagged_nofootprint_compressed);
	LEDGER_DRIFT(tagged_footprint_compressed);
	LEDGER_DRIFT(network_volatile);
	LEDGER_DRIFT(network_nonvolatile);
	LEDGER_DRIFT(network_volatile_compressed);
	LEDGER_DRIFT(network_nonvolatile_compressed);
	LEDGER_DRIFT(media_nofootprint);
	LEDGER_DRIFT(media_footprint);
	LEDGER_DRIFT(media_nofootprint_compressed);
	LEDGER_DRIFT(media_footprint_compressed);
	LEDGER_DRIFT(graphics_nofootprint);
	LEDGER_DRIFT(graphics_footprint);
	LEDGER_DRIFT(graphics_nofootprint_compressed);
	LEDGER_DRIFT(graphics_footprint_compressed);
	LEDGER_DRIFT(neural_nofootprint);
	LEDGER_DRIFT(neural_footprint);
	LEDGER_DRIFT(neural_nofootprint_compressed);
	LEDGER_DRIFT(neural_footprint_compressed);
} pmap_ledgers_drift;

void
vm_map_pmap_check_ledgers(
	pmap_t          pmap,
	ledger_t        ledger,
	int             pid,
	char            *procname)
{
	ledger_amount_t bal;
	boolean_t       do_panic;

	do_panic = FALSE;

	pmap_ledgers_drift.num_pmaps_checked++;

#define LEDGER_CHECK_BALANCE(__LEDGER)                                  \
MACRO_BEGIN                                                             \
	int panic_on_negative = TRUE;                                   \
	ledger_get_balance(ledger,                                      \
	                   task_ledgers.__LEDGER,                       \
	                   &bal);                                       \
	ledger_get_panic_on_negative(ledger,                            \
	                             task_ledgers.__LEDGER,             \
	                             &panic_on_negative);               \
	if (bal != 0) {                                                 \
	        if (panic_on_negative ||                                \
	            (pmap_ledgers_panic &&                              \
	             pmap_ledgers_panic_leeway > 0 &&                   \
	             (bal > (pmap_ledgers_panic_leeway * PAGE_SIZE) ||  \
	              bal < (-pmap_ledgers_panic_leeway * PAGE_SIZE)))) { \
	                do_panic = TRUE;                                \
	        }                                                       \
	        printf("LEDGER BALANCE proc %d (%s) "                   \
	               "\"%s\" = %lld\n",                               \
	               pid, procname, #__LEDGER, bal);                  \
	        if (bal > 0) {                                          \
	                pmap_ledgers_drift.__LEDGER##_over++;           \
	                pmap_ledgers_drift.__LEDGER##_over_total += bal; \
	                if (bal > pmap_ledgers_drift.__LEDGER##_over_max) { \
	                        pmap_ledgers_drift.__LEDGER##_over_max = bal; \
	                }                                               \
	        } else if (bal < 0) {                                   \
	                pmap_ledgers_drift.__LEDGER##_under++;          \
	                pmap_ledgers_drift.__LEDGER##_under_total += bal; \
	                if (bal < pmap_ledgers_drift.__LEDGER##_under_max) { \
	                        pmap_ledgers_drift.__LEDGER##_under_max = bal; \
	                }                                               \
	        }                                                       \
	}                                                               \
MACRO_END

	LEDGER_CHECK_BALANCE(phys_footprint);
	LEDGER_CHECK_BALANCE(internal);
	LEDGER_CHECK_BALANCE(internal_compressed);
	LEDGER_CHECK_BALANCE(iokit_mapped);
	LEDGER_CHECK_BALANCE(alternate_accounting);
	LEDGER_CHECK_BALANCE(alternate_accounting_compressed);
	LEDGER_CHECK_BALANCE(page_table);
	LEDGER_CHECK_BALANCE(purgeable_volatile);
	LEDGER_CHECK_BALANCE(purgeable_nonvolatile);
	LEDGER_CHECK_BALANCE(purgeable_volatile_compressed);
	LEDGER_CHECK_BALANCE(purgeable_nonvolatile_compressed);
	LEDGER_CHECK_BALANCE(tagged_nofootprint);
	LEDGER_CHECK_BALANCE(tagged_footprint);
	LEDGER_CHECK_BALANCE(tagged_nofootprint_compressed);
	LEDGER_CHECK_BALANCE(tagged_footprint_compressed);
	LEDGER_CHECK_BALANCE(network_volatile);
	LEDGER_CHECK_BALANCE(network_nonvolatile);
	LEDGER_CHECK_BALANCE(network_volatile_compressed);
	LEDGER_CHECK_BALANCE(network_nonvolatile_compressed);
	LEDGER_CHECK_BALANCE(media_nofootprint);
	LEDGER_CHECK_BALANCE(media_footprint);
	LEDGER_CHECK_BALANCE(media_nofootprint_compressed);
	LEDGER_CHECK_BALANCE(media_footprint_compressed);
	LEDGER_CHECK_BALANCE(graphics_nofootprint);
	LEDGER_CHECK_BALANCE(graphics_footprint);
	LEDGER_CHECK_BALANCE(graphics_nofootprint_compressed);
	LEDGER_CHECK_BALANCE(graphics_footprint_compressed);
	LEDGER_CHECK_BALANCE(neural_nofootprint);
	LEDGER_CHECK_BALANCE(neural_footprint);
	LEDGER_CHECK_BALANCE(neural_nofootprint_compressed);
	LEDGER_CHECK_BALANCE(neural_footprint_compressed);

	if (do_panic) {
		if (pmap_ledgers_panic) {
			panic("pmap_destroy(%p) %d[%s] has imbalanced ledgers\n",
			    pmap, pid, procname);
		} else {
			printf("pmap_destroy(%p) %d[%s] has imbalanced ledgers\n",
			    pmap, pid, procname);
		}
	}
}
#endif                  
