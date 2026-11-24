/*
 * Copyright (c) 2017 Apple Computer, Inc. All rights reserved.
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

#include <kdp/kdp_core.h>
#include <kdp/processor_core.h>
#include <kern/assert.h>
#include <kern/zalloc.h>
#include <libkern/kernel_mach_header.h>
#include <libkern/OSAtomic.h>
#include <libsa/types.h>
#include <pexpert/pexpert.h>

#ifdef CONFIG_KDP_INTERACTIVE_DEBUGGING

#define roundup(x, y)   ((((x) % (y)) == 0) ? \
	                (x) : ((x) + ((y) - ((x) % (y)))))

  
                                                             
                                                              
                                                                
                                                  
  
                                                                  
                                          
   
typedef struct {
	struct kdp_core_out_vars * core_outvars;                                                 
	kern_coredump_callback_config *core_config;                                                     
	void *core_refcon;                                                                                       
	boolean_t core_is64bit;                                          
	uint32_t core_mh_magic;                                                 
	cpu_type_t core_cpu_type;                                                  
	cpu_subtype_t core_cpu_subtype;                                               
	uint64_t core_file_length;                                                                           
	uint64_t core_file_length_compressed;                                           
	uint64_t core_segment_count;                                                                                
	uint64_t core_segments_remaining;                                                                               
	uint64_t core_segment_byte_total;                                                                      
	uint64_t core_segment_bytes_remaining;                                                                                  
	uint64_t core_thread_count;                                                           
	uint64_t core_threads_remaining;                                                                    
	uint64_t core_thread_state_size;                                         
	uint64_t core_misc_bytes_count;                                                           
	uint64_t core_misc_bytes_remaining;                                                                       
	uint64_t core_cur_hoffset;                                                             
	uint64_t core_cur_foffset;                                                                   
	uint64_t core_header_size;                                                   
	uint64_t core_total_bytes;                                                                                               
} processor_core_context;

  
                                                                  
                                                
   
struct kern_coredump_core {
	struct kern_coredump_core *kcc_next;                                         
	void *kcc_refcon;                                                                                  
	char kcc_corename[MACH_CORE_FILEHEADER_NAMELEN];                                    
	boolean_t kcc_is64bit;                                                  
	uint32_t kcc_mh_magic;                                                      
	cpu_type_t kcc_cpu_type;                                                       
	cpu_subtype_t kcc_cpu_subtype;                                                    
	kern_coredump_callback_config kcc_cb;                                                             
} * kern_coredump_core_list = NULL;

uint32_t coredump_registered_count = 0;

struct kern_coredump_core *kernel_helper = NULL;

static struct kern_coredump_core *
kern_register_coredump_helper_internal(int kern_coredump_config_vers, const kern_coredump_callback_config *kc_callbacks,
    void *refcon, const char *core_description, boolean_t xnu_callback, boolean_t is64bit,
    uint32_t mh_magic, cpu_type_t cpu_type, cpu_subtype_t cpu_subtype)
{
	struct kern_coredump_core *core_helper = NULL;
	kern_coredump_callback_config *core_callbacks = NULL;

	if (kern_coredump_config_vers < KERN_COREDUMP_MIN_CONFIG_VERSION) {
		return NULL;
	}
	if (kc_callbacks == NULL) {
		return NULL;
	}
	;
	if (core_description == NULL) {
		return NULL;
	}

	if (kc_callbacks->kcc_coredump_get_summary == NULL ||
	    kc_callbacks->kcc_coredump_save_segment_descriptions == NULL ||
	    kc_callbacks->kcc_coredump_save_segment_data == NULL ||
	    kc_callbacks->kcc_coredump_save_thread_state == NULL ||
	    kc_callbacks->kcc_coredump_save_sw_vers == NULL) {
		return NULL;
	}

#if !defined(__LP64__)
	                                                                  
	if (is64bit) {
		return NULL;
	}
#endif

	core_helper = zalloc_permanent_type(struct kern_coredump_core);
	core_helper->kcc_next = NULL;
	core_helper->kcc_refcon = refcon;
	if (xnu_callback) {
		snprintf((char *)&core_helper->kcc_corename, MACH_CORE_FILEHEADER_NAMELEN, "%s", core_description);
	} else {
		                                                                                      
		snprintf((char *)&core_helper->kcc_corename, MACH_CORE_FILEHEADER_NAMELEN, "%.8s-coproc", core_description);
	}
	core_helper->kcc_is64bit = is64bit;
	core_helper->kcc_mh_magic = mh_magic;
	core_helper->kcc_cpu_type = cpu_type;
	core_helper->kcc_cpu_subtype = cpu_subtype;
	core_callbacks = &core_helper->kcc_cb;

	core_callbacks->kcc_coredump_init = kc_callbacks->kcc_coredump_init;
	core_callbacks->kcc_coredump_get_summary = kc_callbacks->kcc_coredump_get_summary;
	core_callbacks->kcc_coredump_save_segment_descriptions = kc_callbacks->kcc_coredump_save_segment_descriptions;
	core_callbacks->kcc_coredump_save_segment_data = kc_callbacks->kcc_coredump_save_segment_data;
	core_callbacks->kcc_coredump_save_thread_state = kc_callbacks->kcc_coredump_save_thread_state;
	core_callbacks->kcc_coredump_save_misc_data = kc_callbacks->kcc_coredump_save_misc_data;
	core_callbacks->kcc_coredump_save_sw_vers = kc_callbacks->kcc_coredump_save_sw_vers;

	if (xnu_callback) {
		assert(kernel_helper == NULL);
		kernel_helper = core_helper;
	} else {
		do {
			core_helper->kcc_next = kern_coredump_core_list;
		} while (!OSCompareAndSwapPtr(kern_coredump_core_list, core_helper, &kern_coredump_core_list));
	}

	OSAddAtomic(1, &coredump_registered_count);
	kprintf("Registered coredump handler for %s\n", core_description);

	return core_helper;
}

kern_return_t
kern_register_coredump_helper(int kern_coredump_config_vers, const kern_coredump_callback_config *kc_callbacks,
    void *refcon, const char *core_description, boolean_t is64bit, uint32_t mh_magic,
    cpu_type_t cpu_type, cpu_subtype_t cpu_subtype)
{
	if (coredump_registered_count >= KERN_COREDUMP_MAX_CORES) {
		return KERN_RESOURCE_SHORTAGE;
	}

	if (kern_register_coredump_helper_internal(kern_coredump_config_vers, kc_callbacks, refcon, core_description, FALSE,
	    is64bit, mh_magic, cpu_type, cpu_subtype) == NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return KERN_SUCCESS;
}

kern_return_t
kern_register_xnu_coredump_helper(kern_coredump_callback_config *kc_callbacks)
{
#if defined(__LP64__)
	boolean_t is64bit = TRUE;
#else
	boolean_t is64bit = FALSE;
#endif

	if (kern_register_coredump_helper_internal(KERN_COREDUMP_CONFIG_VERSION, kc_callbacks, NULL, "kernel", TRUE, is64bit,
	    _mh_execute_header.magic, _mh_execute_header.cputype, _mh_execute_header.cpusubtype) == NULL) {
		return KERN_FAILURE;
	}

	return KERN_SUCCESS;
}

  
                                                                               
   
static int
coredump_save_summary(uint64_t core_segment_count, uint64_t core_byte_count,
    uint64_t thread_count, uint64_t thread_state_size,
    uint64_t misc_bytes_count, void *context)
{
	processor_core_context *core_context = (processor_core_context *)context;
	uint32_t sizeofcmds = 0, numcmds = 0;
	int ret = 0;

	if (!core_segment_count || !core_byte_count || !thread_count || !thread_state_size
	    || (thread_state_size > KERN_COREDUMP_THREADSIZE_MAX)) {
		return KERN_INVALID_ARGUMENT;
	}

	                             
	core_context->core_segments_remaining = core_context->core_segment_count = core_segment_count;
	core_context->core_segment_bytes_remaining = core_context->core_segment_byte_total = core_byte_count;
	core_context->core_threads_remaining = core_context->core_thread_count = thread_count;
	core_context->core_thread_state_size = thread_state_size;
	core_context->core_misc_bytes_remaining = core_context->core_misc_bytes_count = misc_bytes_count;


#if defined(__LP64__)
	if (core_context->core_is64bit) {
		sizeofcmds = (uint32_t)(core_context->core_segment_count * sizeof(struct segment_command_64) +
		    (core_context->core_threads_remaining * core_context->core_thread_state_size) +
		                        0 + sizeof(struct ident_command) + KERN_COREDUMP_VERSIONSTRINGMAXSIZE);
		core_context->core_header_size = sizeofcmds + sizeof(struct mach_header_64);
	} else
#endif                        
	{
		sizeofcmds = (uint32_t)(core_context->core_segment_count * sizeof(struct segment_command) +
		    (core_context->core_threads_remaining * core_context->core_thread_state_size) +
		                        0 + sizeof(struct ident_command) + KERN_COREDUMP_VERSIONSTRINGMAXSIZE);
		core_context->core_header_size = sizeofcmds + sizeof(struct mach_header);
	}

	core_context->core_total_bytes = core_context->core_header_size + core_context->core_segment_byte_total +                     0;
	core_context->core_file_length = round_page(core_context->core_header_size) + core_context->core_segment_byte_total +                     0;
	core_context->core_cur_foffset = round_page(core_context->core_header_size);

	numcmds = (uint32_t)(core_context->core_segment_count + core_context->core_thread_count +                     0 +
	    1                    );

	  
                                                                                           
                                                                               
    
	kdp_reset_output_vars(core_context->core_outvars, core_context->core_file_length);

	                                
#if defined(__LP64__)
	if (core_context->core_is64bit) {
		struct mach_header_64 core_header = { };

		core_header.magic = core_context->core_mh_magic;
		core_header.cputype = core_context->core_cpu_type;
		core_header.cpusubtype = core_context->core_cpu_subtype;
		core_header.filetype = MH_CORE;
		core_header.ncmds = numcmds;
		core_header.sizeofcmds = sizeofcmds;
		core_header.flags = 0;

		                                                  
		ret =  kdp_core_output(core_context->core_outvars, sizeof(core_header), (caddr_t)&core_header);
		if (ret != KERN_SUCCESS) {
			kern_coredump_log(context, "coredump_save_summary() : failed to write mach header : kdp_core_output(%p, %lu, %p) returned error 0x%x\n",
			    core_context->core_outvars, sizeof(core_header), &core_header, ret);
			return ret;
		}

		core_context->core_cur_hoffset += sizeof(core_header);
	} else
#endif                        
	{
		struct mach_header core_header = { };

		core_header.magic = core_context->core_mh_magic;
		core_header.cputype = core_context->core_cpu_type;
		core_header.cpusubtype = core_context->core_cpu_subtype;
		core_header.filetype = MH_CORE;
		core_header.ncmds = numcmds;
		core_header.sizeofcmds = sizeofcmds;
		core_header.flags = 0;

		                                                  
		ret =  kdp_core_output(core_context->core_outvars, sizeof(core_header), (caddr_t)&core_header);
		if (ret != KERN_SUCCESS) {
			kern_coredump_log(context, "coredump_save_summary() : failed to write mach header : kdp_core_output(%p, %lu, %p) returned error 0x%x\n",
			    core_context->core_outvars, sizeof(core_header), &core_header, ret);
			return ret;
		}

		core_context->core_cur_hoffset += sizeof(core_header);
	}

	return KERN_SUCCESS;
}

  
                                                         
   
static int
coredump_save_segment_descriptions(uint64_t seg_start, uint64_t seg_end,
    void *context)
{
	processor_core_context *core_context = (processor_core_context *)context;
	int ret;
	uint64_t size = seg_end - seg_start;

	if (seg_end <= seg_start) {
		kern_coredump_log(context, "coredump_save_segment_descriptions(0x%llx, 0x%llx, %p) : called with invalid addresses : start 0x%llx >= end 0x%llx\n",
		    seg_start, seg_end, context, seg_start, seg_end);
		return KERN_INVALID_ARGUMENT;
	}

	if (core_context->core_segments_remaining == 0) {
		kern_coredump_log(context, "coredump_save_segment_descriptions(0x%llx, 0x%llx, %p) : coredump_save_segment_descriptions() called too many times, %llu segment descriptions already recorded\n",
		    seg_start, seg_end, context, core_context->core_segment_count);
		return KERN_INVALID_ARGUMENT;
	}

	                               
#if defined(__LP64__)
	if (core_context->core_is64bit) {
		struct segment_command_64 seg_command = { };

		if (core_context->core_cur_hoffset + sizeof(seg_command) > core_context->core_header_size) {
			kern_coredump_log(context, "coredump_save_segment_descriptions(0x%llx, 0x%llx, %p) : ran out of space to save commands with %llu of %llu remaining\n",
			    seg_start, seg_end, context, core_context->core_segments_remaining, core_context->core_segment_count);
			return KERN_NO_SPACE;
		}

		seg_command.cmd = LC_SEGMENT_64;
		seg_command.cmdsize = sizeof(seg_command);
		seg_command.segname[0] = 0;
		seg_command.vmaddr = seg_start;
		seg_command.vmsize = size;
		seg_command.fileoff = core_context->core_cur_foffset;
		seg_command.filesize = size;
		seg_command.maxprot = VM_PROT_READ;
		seg_command.initprot = VM_PROT_READ;

		                                 
		ret = kdp_core_output(core_context->core_outvars, sizeof(seg_command), (caddr_t)&seg_command);
		if (ret != KERN_SUCCESS) {
			kern_coredump_log(context, "coredump_save_segment_descriptions(0x%llx, 0x%llx, %p) : failed to write segment %llu of %llu. kdp_core_output(%p, %lu, %p) returned error %d\n",
			    seg_start, seg_end, context, core_context->core_segment_count - core_context->core_segments_remaining,
			    core_context->core_segment_count, core_context->core_outvars, sizeof(seg_command), &seg_command, ret);
			return ret;
		}

		core_context->core_cur_hoffset += sizeof(seg_command);
	} else
#endif                        
	{
		struct segment_command seg_command = { };

		if (seg_start > UINT32_MAX || seg_end > UINT32_MAX) {
			kern_coredump_log(context, "coredump_save_segment_descriptions(0x%llx, 0x%llx, %p) : called with invalid addresses for 32-bit : start 0x%llx, end 0x%llx\n",
			    seg_start, seg_end, context, seg_start, seg_end);
			return KERN_INVALID_ARGUMENT;
		}

		if (core_context->core_cur_hoffset + sizeof(seg_command) > core_context->core_header_size) {
			kern_coredump_log(context, "coredump_save_segment_descriptions(0x%llx, 0x%llx, %p) : ran out of space to save commands with %llu of %llu remaining\n",
			    seg_start, seg_end, context, core_context->core_segments_remaining, core_context->core_segment_count);
			return KERN_NO_SPACE;
		}

		seg_command.cmd = LC_SEGMENT;
		seg_command.cmdsize = sizeof(seg_command);
		seg_command.segname[0] = 0;
		seg_command.vmaddr = (uint32_t) seg_start;
		seg_command.vmsize = (uint32_t) size;
		seg_command.fileoff = (uint32_t) core_context->core_cur_foffset;
		seg_command.filesize = (uint32_t) size;
		seg_command.maxprot = VM_PROT_READ;
		seg_command.initprot = VM_PROT_READ;

		                                 
		ret = kdp_core_output(core_context->core_outvars, sizeof(seg_command), (caddr_t)&seg_command);
		if (ret != KERN_SUCCESS) {
			kern_coredump_log(context, "coredump_save_segment_descriptions(0x%llx, 0x%llx, %p) : failed to write segment %llu of %llu : kdp_core_output(%p, %lu, %p) returned  error 0x%x\n",
			    seg_start, seg_end, context, core_context->core_segment_count - core_context->core_segments_remaining,
			    core_context->core_segment_count, core_context->core_outvars, sizeof(seg_command), &seg_command, ret);
			return ret;
		}

		core_context->core_cur_hoffset += sizeof(seg_command);
	}

	                             
	core_context->core_segments_remaining--;
	core_context->core_cur_foffset += size;

	return KERN_SUCCESS;
}

  
                     
  
                                                                
   
static int
coredump_save_thread_state(void *thread_state, void *context)
{
	processor_core_context *core_context = (processor_core_context *)context;
	struct thread_command *tc = (struct thread_command *)thread_state;
	int ret;

	if (tc->cmd != LC_THREAD) {
		kern_coredump_log(context, "coredump_save_thread_state(%p, %p) : found %d expected LC_THREAD (%d)\n",
		    thread_state, context, tc->cmd, LC_THREAD);
		return KERN_INVALID_ARGUMENT;
	}

	if (core_context->core_cur_hoffset + core_context->core_thread_state_size > core_context->core_header_size) {
		kern_coredump_log(context, "coredump_save_thread_state(%p, %p) : ran out of space to save threads with %llu of %llu remaining\n",
		    thread_state, context, core_context->core_threads_remaining, core_context->core_thread_count);
		return KERN_NO_SPACE;
	}

	ret = kdp_core_output(core_context->core_outvars, core_context->core_thread_state_size, (caddr_t)thread_state);
	if (ret != KERN_SUCCESS) {
		kern_coredump_log(context, "coredump_save_thread_state(%p, %p) : failed to write thread data : kdp_core_output(%p, %llu, %p) returned 0x%x\n",
		    thread_state, context, core_context->core_outvars, core_context->core_thread_state_size, thread_state, ret);
		return ret;
	}

	core_context->core_threads_remaining--;
	core_context->core_cur_hoffset += core_context->core_thread_state_size;

	return KERN_SUCCESS;
}

static int
coredump_save_sw_vers(void *sw_vers, uint64_t length, void *context)
{
	processor_core_context *core_context = (processor_core_context *)context;
	struct ident_command ident = { };
	int ret;

	if (length > KERN_COREDUMP_VERSIONSTRINGMAXSIZE || !length) {
		kern_coredump_log(context, "coredump_save_sw_vers(%p, %llu, %p) : called with invalid length %llu\n",
		    sw_vers, length, context, length);
		return KERN_INVALID_ARGUMENT;
	}

	if (core_context->core_cur_hoffset + sizeof(struct ident_command) + length > core_context->core_header_size) {
		kern_coredump_log(context, "coredump_save_sw_vers(%p, %llu, %p) : ran out of space to save data\n",
		    sw_vers, length, context);
		return KERN_NO_SPACE;
	}

	ident.cmd = LC_IDENT;
	ident.cmdsize = (uint32_t)(sizeof(struct ident_command) + KERN_COREDUMP_VERSIONSTRINGMAXSIZE);
	ret = kdp_core_output(core_context->core_outvars, sizeof(struct ident_command), (caddr_t)&ident);
	if (ret != KERN_SUCCESS) {
		kern_coredump_log(context, "coredump_save_sw_vers(%p, %llu, %p) : failed to write ident command : kdp_core_output(%p, %lu, %p) returned 0x%x\n",
		    sw_vers, length, context, core_context->core_outvars, sizeof(struct ident_command), &ident, ret);
		return ret;
	}

	ret = kdp_core_output(core_context->core_outvars, length, (caddr_t)sw_vers);
	if (ret != KERN_SUCCESS) {
		kern_coredump_log(context, "coredump_save_sw_vers(%p, %llu, %p) : failed to write version string : kdp_core_output(%p, %llu, %p) returned 0x%x\n",
		    sw_vers, length, context, core_context->core_outvars, length, sw_vers, ret);
		return ret;
	}

	if (length < KERN_COREDUMP_VERSIONSTRINGMAXSIZE) {
		                                        
		ret = kdp_core_output(core_context->core_outvars, (KERN_COREDUMP_VERSIONSTRINGMAXSIZE - length), NULL);
		if (ret != KERN_SUCCESS) {
			kern_coredump_log(context, "coredump_save_sw_vers(%p, %llu, %p) : failed to write zero fill padding : kdp_core_output(%p, %llu, NULL) returned 0x%x\n",
			    sw_vers, length, context, core_context->core_outvars, (KERN_COREDUMP_VERSIONSTRINGMAXSIZE - length), ret);
			return ret;
		}
	}

	core_context->core_cur_hoffset += sizeof(struct ident_command) + KERN_COREDUMP_VERSIONSTRINGMAXSIZE;

	return KERN_SUCCESS;
}

static int
coredump_save_segment_data(void *seg_data, uint64_t length, void *context)
{
	int ret;
	processor_core_context *core_context = (processor_core_context *)context;

	if (length > core_context->core_segment_bytes_remaining) {
		kern_coredump_log(context, "coredump_save_segment_data(%p, %llu, %p) : called with too much data, %llu written, %llu left\n",
		    seg_data, length, context, core_context->core_segment_byte_total - core_context->core_segment_bytes_remaining,
		    core_context->core_segment_bytes_remaining);
		return KERN_INVALID_ARGUMENT;
	}

	ret = kdp_core_output(core_context->core_outvars, length, (caddr_t)seg_data);
	if (ret != KERN_SUCCESS) {
		kern_coredump_log(context, "coredump_save_segment_data(%p, %llu, %p) : failed to write data (%llu bytes remaining) :%d\n",
		    seg_data, length, context, core_context->core_segment_bytes_remaining, ret);
		return ret;
	}

	core_context->core_segment_bytes_remaining -= length;
	core_context->core_cur_foffset += length;

	return KERN_SUCCESS;
}

static kern_return_t
kern_coredump_routine(void *core_outvars, struct kern_coredump_core *current_core, uint64_t core_begin_offset, uint64_t *core_file_length, boolean_t *header_update_failed)
{
	kern_return_t ret;
	processor_core_context context = { };
	*core_file_length = 0;
	*header_update_failed = FALSE;

	                                
	context.core_outvars = core_outvars;
	context.core_config = &current_core->kcc_cb;
	context.core_refcon = current_core->kcc_refcon;
	context.core_is64bit = current_core->kcc_is64bit;
	context.core_mh_magic = current_core->kcc_mh_magic;
	context.core_cpu_type = current_core->kcc_cpu_type;
	context.core_cpu_subtype = current_core->kcc_cpu_subtype;

	kern_coredump_log(&context, "\nBeginning coredump of %s\n", current_core->kcc_corename);

	if (current_core->kcc_cb.kcc_coredump_init != NULL) {
		ret = current_core->kcc_cb.kcc_coredump_init(context.core_refcon, &context);
		if (ret == KERN_NODE_DOWN) {
			kern_coredump_log(&context, "coredump_init returned KERN_NODE_DOWN, skipping this core\n");
			return KERN_SUCCESS;
		} else if (ret != KERN_SUCCESS) {
			kern_coredump_log(&context, "(kern_coredump_routine) : coredump_init failed with %d\n", ret);
			return ret;
		}
	}

	                                                                                 
	ret = current_core->kcc_cb.kcc_coredump_get_summary(context.core_refcon, coredump_save_summary, &context);
	if (ret != KERN_SUCCESS) {
		kern_coredump_log(&context, "(kern_coredump_routine) : get_summary failed with %d\n", ret);
		return ret;
	}

	if (context.core_header_size == 0) {
		kern_coredump_log(&context, "(kern_coredump_routine) : header size not populated after coredump_get_summary\n");
		return KERN_FAILURE;
	}

	                                                                   
	ret = current_core->kcc_cb.kcc_coredump_save_segment_descriptions(context.core_refcon, coredump_save_segment_descriptions,
	    &context);
	if (ret != KERN_SUCCESS) {
		kern_coredump_log(&context, "(kern_coredump_routine) : save_segment_descriptions failed with %d\n", ret);
		return ret;
	}

	if (context.core_segments_remaining != 0) {
		kern_coredump_log(&context, "(kern_coredump_routine) : save_segment_descriptions returned without all segment descriptions written, %llu of %llu remaining\n",
		    context.core_segments_remaining, context.core_segment_count);
		return KERN_FAILURE;
	}

	                                                                   

	  
                                  
   
                                                                           
    
	if (context.core_thread_state_size) {
		char threadstatebuf[context.core_thread_state_size];
		ret = current_core->kcc_cb.kcc_coredump_save_thread_state(context.core_refcon, &threadstatebuf, coredump_save_thread_state,
		    &context);
		if (ret != KERN_SUCCESS) {
			kern_coredump_log(&context, "(kern_coredump_routine) : save_thread_state failed with %d\n", ret);
			return ret;
		}
	}

	if (context.core_threads_remaining != 0) {
		kern_coredump_log(&context, "(kern_coredump_routine) : save_thread_state returned without all thread descriptions written, %llu of %llu remaining\n",
		    context.core_threads_remaining, context.core_thread_count);
		return KERN_FAILURE;
	}

	                                
	ret = current_core->kcc_cb.kcc_coredump_save_sw_vers(context.core_refcon, coredump_save_sw_vers, &context);
	if (ret != KERN_SUCCESS) {
		kern_coredump_log(&context, "(kern_coredump_routine) : save_sw_vers failed with %d\n", ret);
		return ret;
	}

	assert(context.core_cur_hoffset == context.core_header_size);

	                                                                                               
	ret = kdp_core_output(context.core_outvars, (round_page(context.core_header_size) - context.core_header_size), NULL);
	if (ret != KERN_SUCCESS) {
		kern_coredump_log(&context, "(kern_coredump_routine) : failed to write zero fill padding (%llu bytes remaining) : kdp_core_output(%p, %llu, NULL) returned 0x%x\n",
		    context.core_segment_bytes_remaining, context.core_outvars, (round_page(context.core_header_size) - context.core_header_size), ret);
		return ret;
	}

	context.core_cur_foffset = round_page(context.core_header_size);
	ret = current_core->kcc_cb.kcc_coredump_save_segment_data(context.core_refcon, coredump_save_segment_data, &context);
	if (ret != KERN_SUCCESS) {
		kern_coredump_log(&context, "coredump_save_segment_data failed with %d\n", ret);
		return ret;
	}

	if (context.core_segment_bytes_remaining != 0) {
		kern_coredump_log(&context, "(kern_coredump_routine) : save_segment_data returned without all segment data written, %llu of %llu remaining\n",
		    context.core_segment_bytes_remaining, context.core_segment_byte_total);
		return KERN_FAILURE;
	}

	                                                    

	                             
	ret = kdp_core_output(context.core_outvars, 0, NULL);
	if (ret != KERN_SUCCESS) {
		kern_coredump_log(&context, "(kern_coredump_routine) : failed to flush final core data : kdp_core_output(%p, 0, NULL) returned 0x%x\n",
		    context.core_outvars, ret);
		return ret;
	}

	kern_coredump_log(&context, "Done\nCoredump complete of %s, dumped %llu segments (%llu bytes), %llu threads (%llu bytes) overall uncompressed file length %llu bytes.",
	    current_core->kcc_corename, context.core_segment_count, context.core_segment_byte_total, context.core_thread_count,
	    (context.core_thread_count * context.core_thread_state_size), context.core_file_length);

	if (core_begin_offset) {
		                                                                                    
		ret = kern_dump_record_file(context.core_outvars, current_core->kcc_corename, core_begin_offset, &context.core_file_length_compressed);
		if (ret != KERN_SUCCESS) {
			*header_update_failed = TRUE;
			kern_coredump_log(&context, "\n(kern_coredump_routine) : kern_dump_record_file failed with %d\n", ret);
			return ret;
		}
	}

	kern_coredump_log(&context, " Compressed file length is %llu bytes\n", context.core_file_length_compressed);

	*core_file_length = context.core_file_length_compressed;

	return KERN_SUCCESS;
}

kern_return_t
kern_do_coredump(void *core_outvars, boolean_t kernel_only, uint64_t first_file_offset, uint64_t *last_file_offset)
{
	struct kern_coredump_core *current_core = NULL;
	uint64_t prev_core_length = 0;
	kern_return_t cur_ret = KERN_SUCCESS, ret = KERN_SUCCESS;
	boolean_t header_update_failed = FALSE;

	assert(last_file_offset != NULL);

	*last_file_offset = first_file_offset;
	cur_ret = kern_coredump_routine(core_outvars, kernel_helper, *last_file_offset, &prev_core_length, &header_update_failed);
	if (cur_ret != KERN_SUCCESS) {
		                                                                                                 
		                              
		if (header_update_failed) {
			                                                          
			return KERN_FAILURE;
		} else {
			prev_core_length = 0;
			ret = KERN_FAILURE;
		}
	}

	*last_file_offset = roundup(((*last_file_offset) + prev_core_length), KERN_COREDUMP_BEGIN_FILEBYTES_ALIGN);
	prev_core_length = 0;

	if (kernel_only) {
		return ret;
	}

	current_core = kern_coredump_core_list;
	while (current_core) {
		                                            
		cur_ret = kern_dump_seek_to_next_file(core_outvars, *last_file_offset);
		if (cur_ret != KERN_SUCCESS) {
			kern_coredump_log(NULL, "Failed to seek to beginning of next core\n");
			return KERN_FAILURE;
		}

		cur_ret = kern_coredump_routine(core_outvars, current_core, *last_file_offset, &prev_core_length, &header_update_failed);
		if (cur_ret != KERN_SUCCESS) {
			                                                                                                 
			                              
			if (header_update_failed) {
				                                                          
				return KERN_FAILURE;
			} else {
				                                                                               
				                
				prev_core_length = 0;
				ret = KERN_FAILURE;
			}
		}

		                                                                            
		*last_file_offset = roundup(((*last_file_offset) + prev_core_length), KERN_COREDUMP_BEGIN_FILEBYTES_ALIGN);
		prev_core_length = 0;
		current_core = current_core->kcc_next;
	}

	return ret;
}
#else                                       

kern_return_t
kern_register_coredump_helper(int kern_coredump_config_vers, const kern_coredump_callback_config *kc_callbacks, void* refcon,
    const char *core_description, boolean_t is64bit, uint32_t mh_magic,
    cpu_type_t cpu_type, cpu_subtype_t cpu_subtype)
{
#pragma unused(kern_coredump_config_vers, kc_callbacks, refcon, core_description, is64bit, mh_magic, cpu_type, cpu_subtype)
	return KERN_NOT_SUPPORTED;
}
#endif                                       

  
                                       
   
void
kern_coredump_log(void *context, const char *string, ...)
{
#pragma unused(context)
	va_list coredump_log_args;

	va_start(coredump_log_args, string);
	_doprnt(string, &coredump_log_args, consdebug_putc, 16);
	va_end(coredump_log_args);

#if defined(__arm__) || defined(__arm64__)
	paniclog_flush();
#endif
}
