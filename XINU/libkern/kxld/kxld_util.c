/*
 * Copyright (c) 2007-2016 Apple Inc. All rights reserved.
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
#include <stdarg.h>
#include <string.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/reloc.h>
#if KERNEL
    #include <kern/kalloc.h>
    #include <libkern/libkern.h>
    #include <mach/vm_param.h>
    #include <vm/vm_kern.h>
#else
    #include <stdio.h>
    #include <stdlib.h>
    #include <mach/mach_init.h>
    #include <mach-o/swap.h>
#endif

#define DEBUG_ASSERT_COMPONENT_NAME_STRING "kxld"
#include <AssertMacros.h>

#include "kxld_util.h"

#if !KERNEL
static void unswap_macho_32(u_char *file, enum NXByteOrder host_order,
    enum NXByteOrder target_order);
static void unswap_macho_64(u_char *file, enum NXByteOrder host_order,
    enum NXByteOrder target_order);
#endif              

#if DEBUG
static unsigned long num_allocations = 0;
static unsigned long num_frees = 0;
static unsigned long bytes_allocated = 0;
static unsigned long bytes_freed = 0;
#endif

static KXLDLoggingCallback s_logging_callback = NULL;
static char s_callback_name[64] = "internal";
static void *s_callback_data = NULL;

#if !KERNEL
static boolean_t s_cross_link_enabled  = FALSE;
                                                                      
                                                                    
                                                                   
                                                             
                                            
   
static kxld_size_t s_cross_link_page_size;
#endif


                                                                                
                                                                                
void
kxld_set_logging_callback(KXLDLoggingCallback logging_callback)
{
	s_logging_callback = logging_callback;
}

                                                                                
                                                                                
void
kxld_set_logging_callback_data(const char *name, void *user_data)
{
	if (name) {
		(void)strlcpy(s_callback_name, name, sizeof(s_callback_name));
		                                                               
		for (size_t i = 0; i < sizeof(s_callback_name); i++) {
			if (s_callback_name[i] == '%') {
				s_callback_name[i] = '.';
			}
		}
	} else {
		(void)strlcpy(s_callback_name, "internal", sizeof(s_callback_name));
	}

	s_callback_data = user_data;
}

                                                                                
                                                                                
void
kxld_log(KXLDLogSubsystem subsystem, KXLDLogLevel level,
    const char *in_format, ...)
{
	char stack_buffer[256];
	char *alloc_buffer = NULL;
	char *format = stack_buffer;
	u_int length = 0;
	va_list ap;

	if (s_logging_callback) {
		length = snprintf(stack_buffer, sizeof(stack_buffer), "kxld[%s]: %s",
		    s_callback_name, in_format);

		if (length >= sizeof(stack_buffer)) {
			length += 1;
			alloc_buffer = kxld_alloc(length);
			if (!alloc_buffer) {
				return;
			}

			snprintf(alloc_buffer, length, "kxld[%s]: %s",
			    s_callback_name, in_format);
			format = alloc_buffer;
		}

		va_start(ap, in_format);
		s_logging_callback(subsystem, level, format, ap, s_callback_data);
		va_end(ap);

		if (alloc_buffer) {
			kxld_free(alloc_buffer, length);
		}
	}
}

                                                                            
                        
   
#define KALLOC_MAX 16 * 1024

                                                                                
                                                                                
void *
kxld_calloc(size_t size)
{
	void * ptr = NULL;

#if KERNEL
	ptr = kalloc(size);
	if (ptr) {
		bzero(ptr, size);
	}
#else
	ptr = calloc(1, size);
#endif

#if DEBUG
	if (ptr) {
		++num_allocations;
		bytes_allocated += size;
	}
#endif

	return ptr;
}

void *
kxld_alloc(size_t size)
{
	void * ptr = NULL;

#if KERNEL
	ptr = kalloc(size);
#else
	ptr = malloc(size);
#endif

#if DEBUG
	if (ptr) {
		++num_allocations;
		bytes_allocated += size;
	}
#endif

	return ptr;
}

                                                                                
                                                                                
void *
kxld_page_alloc_untracked(size_t size)
{
	void * ptr = NULL;
#if KERNEL
	kern_return_t rval = 0;
	vm_offset_t addr = 0;
#endif             

	size = round_page(size);

#if KERNEL
	if (size < KALLOC_MAX) {
		ptr = kalloc(size);
	} else {
		rval = kmem_alloc(kernel_map, &addr, size, VM_KERN_MEMORY_OSKEXT);
		if (!rval) {
			ptr = (void *) addr;
		}
	}
	if (ptr) {
		bzero(ptr, size);
	}
#else              
	ptr = calloc(1, size);
#endif             

	return ptr;
}

                                                                                
                                                                                
void *
kxld_page_alloc(size_t size)
{
	void * ptr = NULL;

	ptr = kxld_page_alloc_untracked(size);
#if DEBUG
	if (ptr) {
		++num_allocations;
		bytes_allocated += round_page(size);
	}
#endif            

	return ptr;
}

                                                                                
                                                                                
void *
kxld_alloc_pageable(size_t size)
{
	size = round_page(size);

#if KERNEL
	kern_return_t rval = 0;
	vm_offset_t ptr = 0;

	rval = kmem_alloc_pageable(kernel_map, &ptr, size, VM_KERN_MEMORY_OSKEXT);
	if (rval) {
		ptr = 0;
	}

	return (void *) ptr;
#else
	return kxld_page_alloc_untracked(size);
#endif
}

                                                                                
                                                                                
void
kxld_free(void *ptr, size_t size __unused)
{
#if DEBUG
	++num_frees;
	bytes_freed += size;
#endif

#if KERNEL
	kfree(ptr, size);
#else
	free(ptr);
#endif
}

                                                                                
                                                                                
void
kxld_page_free_untracked(void *ptr, size_t size __unused)
{
#if KERNEL
	size = round_page(size);

	if (size < KALLOC_MAX) {
		kfree(ptr, size);
	} else {
		kmem_free(kernel_map, (vm_offset_t) ptr, size);
	}
#else              
	free(ptr);
#endif             
}


                                                                                
                                                                                
void
kxld_page_free(void *ptr, size_t size)
{
#if DEBUG
	++num_frees;
	bytes_freed += round_page(size);
#endif            
	kxld_page_free_untracked(ptr, size);
}

                                                                                
                                                                                
kern_return_t
validate_and_swap_macho_32(u_char *file, u_long size
#if !KERNEL
    , enum NXByteOrder host_order
#endif              
    )
{
	kern_return_t rval = KERN_FAILURE;
	struct mach_header *mach_hdr = (struct mach_header *) ((void *) file);
	struct load_command *load_hdr = NULL;
	struct segment_command *seg_hdr = NULL;
	struct section *sects = NULL;
	struct relocation_info *relocs = NULL;
	struct symtab_command *symtab_hdr = NULL;
	struct nlist *symtab = NULL;
	u_long offset = 0;
	u_int cmd = 0;
	u_int cmdsize = 0;
	u_int i = 0;
	u_int j = 0;
#if !KERNEL
	boolean_t swap = FALSE;
#endif              

	check(file);
	check(size);

	                                                            
	require_action(size >= sizeof(*mach_hdr), finish,
	    rval = KERN_FAILURE;
	    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));
	offset = sizeof(*mach_hdr);

#if !KERNEL
	                                       
	if (mach_hdr->magic == MH_CIGAM) {
		swap = TRUE;
		(void) swap_mach_header(mach_hdr, host_order);
	}
#endif              

	                                             
	require_action(mach_hdr->magic == MH_MAGIC, finish,
	    rval = KERN_FAILURE;
	    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogMalformedMachO
	    "Invalid magic number: 0x%x.", mach_hdr->magic));

	                                                             
                                                                   
                                                                     
    
#if KERNEL
	if (mach_hdr->filetype == MH_EXECUTE) {
		rval = KERN_SUCCESS;
		goto finish;
	}
#endif             

	                                                     
	for (i = 0; i < mach_hdr->ncmds; ++i, offset += cmdsize) {
		                                   
		load_hdr = (struct load_command *) ((void *) (file + offset));
		cmd = load_hdr->cmd;
		cmdsize = load_hdr->cmdsize;

#if !KERNEL
		if (swap) {
			cmd = OSSwapInt32(load_hdr->cmd);
			cmdsize = OSSwapInt32(load_hdr->cmdsize);
		}
#endif              

		                                                                    
		require_action(size >= offset + cmdsize, finish,
		    rval = KERN_FAILURE;
		    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

		switch (cmd) {
		case LC_SEGMENT:
			                                     
			seg_hdr = (struct segment_command *) load_hdr;
#if !KERNEL
			if (swap) {
				swap_segment_command(seg_hdr, host_order);
			}
#endif              

			                                      
			sects = (struct section *) &seg_hdr[1];
#if !KERNEL
			if (swap) {
				swap_section(sects, seg_hdr->nsects, host_order);
			}
#endif              

			                                     
			if (!seg_hdr->vmsize) {
				continue;
			}

			                                                              
			require_action(size >= seg_hdr->fileoff + seg_hdr->filesize, finish,
			    rval = KERN_FAILURE;
			    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

			for (j = 0; j < seg_hdr->nsects; ++j) {
				                                                          
                                                              
       
				require_action((sects[j].flags & S_ZEROFILL) ||
				    (size >= sects[j].offset + sects[j].size), finish,
				    rval = KERN_FAILURE;
				    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

				                                                       
                          
       
				require_action(size >=
				    sects[j].reloff + sects[j].nreloc * sizeof(*relocs), finish,
				    rval = KERN_FAILURE;
				    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

				                                 
				relocs = (struct relocation_info *) ((void *) (file + sects[j].reloff));
#if !KERNEL
				if (swap) {
					swap_relocation_info(relocs, sects[j].nreloc,
					    host_order);
				}
#endif              
			}

			break;
		case LC_SYMTAB:
			                                    
			symtab_hdr = (struct symtab_command *) load_hdr;
#if !KERNEL
			if (swap) {
				swap_symtab_command(symtab_hdr, host_order);
			}
#endif              

			                                                             
			require_action(size >=
			    symtab_hdr->symoff + symtab_hdr->nsyms * sizeof(*symtab), finish,
			    rval = KERN_FAILURE;
			    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

			                                                             
			require_action(size >= symtab_hdr->stroff + symtab_hdr->strsize, finish,
			    rval = KERN_FAILURE;
			    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

#if !KERNEL
			                                   
			symtab = (struct nlist *) ((void *) (file + symtab_hdr->symoff));
			if (swap) {
				swap_nlist(symtab, symtab_hdr->nsyms, host_order);
			}
#endif              

			break;
		default:
#if !KERNEL
			                           
			if (swap) {
				swap_load_command(load_hdr, host_order);
			}
#endif              
			break;
		}
	}

	rval = KERN_SUCCESS;

finish:
	return rval;
}

                                                                                
                                                                                
kern_return_t
validate_and_swap_macho_64(u_char *file, u_long size
#if !KERNEL
    , enum NXByteOrder host_order
#endif              
    )
{
	kern_return_t rval = KERN_FAILURE;
	struct mach_header_64 *mach_hdr = (struct mach_header_64 *) ((void *) file);
	struct load_command *load_hdr = NULL;
	struct segment_command_64 *seg_hdr = NULL;
	struct section_64 *sects = NULL;
	struct relocation_info *relocs = NULL;
	struct symtab_command *symtab_hdr = NULL;
	struct nlist_64 *symtab = NULL;
	u_long offset = 0;
	u_int cmd = 0;
	u_int cmdsize = 0;
	u_int i = 0;
	u_int j = 0;
#if !KERNEL
	boolean_t swap = FALSE;
#endif              

	check(file);
	check(size);

	                                                            
	require_action(size >= sizeof(*mach_hdr), finish,
	    rval = KERN_FAILURE;
	    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));
	offset = sizeof(*mach_hdr);

#if !KERNEL
	                                       
	if (mach_hdr->magic == MH_CIGAM_64) {
		swap = TRUE;
		(void) swap_mach_header_64(mach_hdr, host_order);
	}
#endif              

	                                             
	require_action(mach_hdr->magic == MH_MAGIC_64, finish,
	    rval = KERN_FAILURE;
	    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogMalformedMachO
	    "Invalid magic number: 0x%x.", mach_hdr->magic));

	                                                             
                                                                   
                                                                     
    
#if KERNEL
	if (mach_hdr->filetype == MH_EXECUTE) {
		rval = KERN_SUCCESS;
		goto finish;
	}
#endif             

	                                                     
	for (i = 0; i < mach_hdr->ncmds; ++i, offset += cmdsize) {
		                                   
		load_hdr = (struct load_command *) ((void *) (file + offset));
		cmd = load_hdr->cmd;
		cmdsize = load_hdr->cmdsize;

#if !KERNEL
		if (swap) {
			cmd = OSSwapInt32(load_hdr->cmd);
			cmdsize = OSSwapInt32(load_hdr->cmdsize);
		}
#endif              

		                                                                    
		require_action(size >= offset + cmdsize, finish,
		    rval = KERN_FAILURE;
		    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));
		switch (cmd) {
		case LC_SEGMENT_64:
			                                     
			seg_hdr = (struct segment_command_64 *) ((void *) load_hdr);
#if !KERNEL
			if (swap) {
				swap_segment_command_64(seg_hdr, host_order);
			}
#endif              

			                                      
			sects = (struct section_64 *) &seg_hdr[1];
#if !KERNEL
			if (swap) {
				swap_section_64(sects, seg_hdr->nsects, host_order);
			}
#endif              

			                                                 
			if (!seg_hdr->vmsize) {
				continue;
			}

			                                                              
			require_action(size >= seg_hdr->fileoff + seg_hdr->filesize, finish,
			    rval = KERN_FAILURE;
			    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

			for (j = 0; j < seg_hdr->nsects; ++j) {
				                                                          
                                                              
       
				require_action((sects[j].flags & S_ZEROFILL) ||
				    (size >= sects[j].offset + sects[j].size), finish,
				    rval = KERN_FAILURE;
				    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

				                                                       
                          
       
				require_action(size >=
				    sects[j].reloff + sects[j].nreloc * sizeof(*relocs), finish,
				    rval = KERN_FAILURE;
				    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

				                                 
				relocs = (struct relocation_info *) ((void *) (file + sects[j].reloff));
#if !KERNEL
				if (swap) {
					swap_relocation_info(relocs, sects[j].nreloc,
					    host_order);
				}
#endif              
			}

			break;
		case LC_SYMTAB:
			                                    
			symtab_hdr = (struct symtab_command *) load_hdr;
#if !KERNEL
			if (swap) {
				swap_symtab_command(symtab_hdr, host_order);
			}
#endif              

			                                                             
			require_action(size >=
			    symtab_hdr->symoff + symtab_hdr->nsyms * sizeof(*symtab), finish,
			    rval = KERN_FAILURE;
			    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

			                                                             
			require_action(size >= symtab_hdr->stroff + symtab_hdr->strsize, finish,
			    rval = KERN_FAILURE;
			    kxld_log(kKxldLogLinking, kKxldLogErr, kKxldLogTruncatedMachO));

#if !KERNEL
			                                   
			symtab = (struct nlist_64 *) ((void *) (file + symtab_hdr->symoff));
			if (swap) {
				swap_nlist_64(symtab, symtab_hdr->nsyms, host_order);
			}
#endif              

			break;
		default:
#if !KERNEL
			                           
			if (swap) {
				swap_load_command(load_hdr, host_order);
			}
#endif              
			break;
		}
	}

	rval = KERN_SUCCESS;

finish:
	return rval;
}

#if !KERNEL
                                                                                
                                                                                
void
unswap_macho(u_char *file, enum NXByteOrder host_order,
    enum NXByteOrder target_order)
{
	struct mach_header *hdr = (struct mach_header *) ((void *) file);

	if (!hdr) {
		return;
	}

	if (hdr->magic == MH_MAGIC) {
		unswap_macho_32(file, host_order, target_order);
	} else if (hdr->magic == MH_MAGIC_64) {
		unswap_macho_64(file, host_order, target_order);
	}
}

                                                                                
                                                                                
static void
unswap_macho_32(u_char *file, enum NXByteOrder host_order,
    enum NXByteOrder target_order)
{
	struct mach_header *mach_hdr = (struct mach_header *) ((void *) file);
	struct load_command *load_hdr = NULL;
	struct segment_command *seg_hdr = NULL;
	struct section *sects = NULL;
	struct symtab_command *symtab_hdr = NULL;
	struct nlist *symtab = NULL;
	u_long offset = 0;
	u_int cmd = 0;
	u_int size = 0;
	u_int i = 0;

	check(file);

	if (target_order == host_order) {
		return;
	}

	offset = sizeof(*mach_hdr);
	for (i = 0; i < mach_hdr->ncmds; ++i, offset += size) {
		load_hdr = (struct load_command *) ((void *) (file + offset));
		cmd = load_hdr->cmd;
		size = load_hdr->cmdsize;

		switch (cmd) {
		case LC_SEGMENT:
			seg_hdr = (struct segment_command *) load_hdr;
			sects = (struct section *) &seg_hdr[1];

			                                                               
                                                                     
      

			swap_section(sects, seg_hdr->nsects, target_order);
			swap_segment_command(seg_hdr, target_order);
			break;
		case LC_SYMTAB:
			symtab_hdr = (struct symtab_command *) load_hdr;
			symtab = (struct nlist*) ((void *) (file + symtab_hdr->symoff));

			swap_nlist(symtab, symtab_hdr->nsyms, target_order);
			swap_symtab_command(symtab_hdr, target_order);

			break;
		default:
			swap_load_command(load_hdr, target_order);
			break;
		}
	}

	(void) swap_mach_header(mach_hdr, target_order);
}

                                                                                
                                                                                
static void
unswap_macho_64(u_char *file, enum NXByteOrder host_order,
    enum NXByteOrder target_order)
{
	struct mach_header_64 *mach_hdr = (struct mach_header_64 *) ((void *) file);
	struct load_command *load_hdr = NULL;
	struct segment_command_64 *seg_hdr = NULL;
	struct section_64 *sects = NULL;
	struct symtab_command *symtab_hdr = NULL;
	struct nlist_64 *symtab = NULL;
	u_long offset = 0;
	u_int cmd = 0;
	u_int size = 0;
	u_int i = 0;

	check(file);

	if (target_order == host_order) {
		return;
	}

	offset = sizeof(*mach_hdr);
	for (i = 0; i < mach_hdr->ncmds; ++i, offset += size) {
		load_hdr = (struct load_command *) ((void *) (file + offset));
		cmd = load_hdr->cmd;
		size = load_hdr->cmdsize;

		switch (cmd) {
		case LC_SEGMENT_64:
			seg_hdr = (struct segment_command_64 *) ((void *) load_hdr);
			sects = (struct section_64 *) &seg_hdr[1];

			                                                               
                                                                     
      

			swap_section_64(sects, seg_hdr->nsects, target_order);
			swap_segment_command_64(seg_hdr, target_order);
			break;
		case LC_SYMTAB:
			symtab_hdr = (struct symtab_command *) load_hdr;
			symtab = (struct nlist_64 *) ((void *) (file + symtab_hdr->symoff));

			swap_nlist_64(symtab, symtab_hdr->nsyms, target_order);
			swap_symtab_command(symtab_hdr, target_order);

			break;
		default:
			swap_load_command(load_hdr, target_order);
			break;
		}
	}

	(void) swap_mach_header_64(mach_hdr, target_order);
}
#endif              

                                                                                
                                                                                
kxld_addr_t
kxld_align_address(kxld_addr_t address, u_int align)
{
	kxld_addr_t alignment = (1 << align);
	kxld_addr_t low_bits = 0;

	if (!align) {
		return address;
	}

	low_bits = (address) & (alignment - 1);
	if (low_bits) {
		address += (alignment - low_bits);
	}

	return address;
}

                                                                                
                                                                                
boolean_t
kxld_is_32_bit(cpu_type_t cputype)
{
	return !(cputype & CPU_ARCH_ABI64);
}

                                                                                
                                                                                
void
kxld_print_memory_report(void)
{
#if DEBUG
	kxld_log(kKxldLogLinking, kKxldLogExplicit, "kxld memory usage report:\n"
	    "\tNumber of allocations:   %8lu\n"
	    "\tNumber of frees:         %8lu\n"
	    "\tAverage allocation size: %8lu\n"
	    "\tTotal bytes allocated:   %8lu\n"
	    "\tTotal bytes freed:       %8lu\n"
	    "\tTotal bytes leaked:      %8lu",
	    num_allocations, num_frees, bytes_allocated / num_allocations,
	    bytes_allocated, bytes_freed, bytes_allocated - bytes_freed);
#endif
}

                                                                      
                                                                      
#if !KERNEL
boolean_t
kxld_set_cross_link_page_size(kxld_size_t target_page_size)
{
	                 
	if ((target_page_size != 0) &&
	    ((target_page_size & (target_page_size - 1)) == 0)) {
		s_cross_link_enabled = TRUE;
		s_cross_link_page_size = target_page_size;

		return TRUE;
	} else {
		return FALSE;
	}
}
#endif              

                                                                      
                                                                      
kxld_size_t
kxld_get_effective_page_size(void)
{
#if KERNEL
	return PAGE_SIZE;
#else
	if (s_cross_link_enabled) {
		return s_cross_link_page_size;
	} else {
		return PAGE_SIZE;
	}
#endif             
}

                                                                      
                                                                      
kxld_addr_t
kxld_round_page_cross_safe(kxld_addr_t offset)
{
#if KERNEL
	return round_page(offset);
#else
	                                              
	if (s_cross_link_enabled) {
		return (offset + (s_cross_link_page_size - 1)) &
		       (~(s_cross_link_page_size - 1));
	} else {
		return round_page(offset);
	}
#endif             
}

#if SPLIT_KEXTS_DEBUG

void
kxld_show_split_info(splitKextLinkInfo *info)
{
	kxld_log(kKxldLogLinking, kKxldLogErr,
	    "splitKextLinkInfo: \n"
	    "kextExecutable %p to %p kextSize %lu \n"
	    "linkedKext %p to %p linkedKextSize %lu \n"
	    "vmaddr_TEXT %p vmaddr_TEXT_EXEC %p "
	    "vmaddr_DATA %p vmaddr_DATA_CONST %p "
	    "vmaddr_LLVM_COV %p vmaddr_LINKEDIT %p",
	    (void *) info->kextExecutable,
	    (void *) (info->kextExecutable + info->kextSize),
	    info->kextSize,
	    (void*) info->linkedKext,
	    (void*) (info->linkedKext + info->linkedKextSize),
	    info->linkedKextSize,
	    (void *) info->vmaddr_TEXT,
	    (void *) info->vmaddr_TEXT_EXEC,
	    (void *) info->vmaddr_DATA,
	    (void *) info->vmaddr_DATA_CONST,
	    (void *) info->vmaddr_LLVM_COV,
	    (void *) info->vmaddr_LINKEDIT);
}

boolean_t
isTargetKextName(const char * the_name)
{
	if (the_name && 0 == strcmp(the_name, KXLD_TARGET_KEXT)) {
		return TRUE;
	}
	return FALSE;
}
#endif
