/*
 * Copyright (c) 1991-2015 Apple Computer, Inc. All rights reserved.
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
#include <sys/param.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <vm/vm_kern.h>
#include <mach/kern_return.h>
#include <mach/vm_param.h>
#include <kern/cpu_number.h>
#include <mach-o/fat.h>
#include <kern/mach_loader.h>
#include <kern/mach_fat.h>
#include <libkern/OSByteOrder.h>
#include <machine/exec.h>

                                                                       
                            
 
                                                               
                            
 
                                                  
                                                                      
                                       
                                                       
                                    
                      
                                                       
                 
 
                                                  
                                             
                                                                       
static load_return_t
fatfile_getarch(
	vm_offset_t              data_ptr,
	vm_size_t                data_size,
	cpu_type_t               req_cpu_type,
	cpu_type_t               mask_bits,
	cpu_subtype_t            req_subcpu_type,
	struct image_params      *imgp,
	struct fat_arch          *archret)
{
	load_return_t           lret;
	struct fat_arch         *arch;
	struct fat_arch         *best_arch;
	int                     grade;
	int                     best_grade;
	size_t                  nfat_arch, max_nfat_arch;
	cpu_type_t              testtype;
	cpu_subtype_t           testsubtype;
	cpu_subtype_t           testfeatures;
	struct fat_header       *header;

	if (sizeof(struct fat_header) > data_size) {
		return LOAD_FAILURE;
	}

	header = (struct fat_header *)data_ptr;
	nfat_arch = OSSwapBigToHostInt32(header->nfat_arch);

	max_nfat_arch = (data_size - sizeof(struct fat_header)) / sizeof(struct fat_arch);
	if (nfat_arch > max_nfat_arch) {
		                                                        
		return LOAD_BADMACHO;
	}

	  
                                                     
	best_arch = NULL;
	best_grade = 0;
	arch = (struct fat_arch *) (data_ptr + sizeof(struct fat_header));
	for (; nfat_arch-- > 0; arch++) {
		testtype = OSSwapBigToHostInt32(arch->cputype);
		testsubtype = OSSwapBigToHostInt32(arch->cpusubtype) & ~CPU_SUBTYPE_MASK;
		testfeatures = OSSwapBigToHostInt32(arch->cpusubtype) & CPU_SUBTYPE_MASK;

		  
                                           
     
		if (!binary_match(mask_bits, req_cpu_type, req_subcpu_type, testtype, testsubtype)) {
			continue;
		}

		  
                                          
     
		grade = grade_binary(testtype, testsubtype, testfeatures, TRUE);

		  
                                             
     
		if (grade > best_grade) {
			best_grade = grade;
			best_arch = arch;
		}
	}

	                                                            
                                                                              
                                                                             
                                                                  
	if (best_arch != NULL &&
	    validate_potential_simulator_binary(OSSwapBigToHostInt32(best_arch->cputype),
	    imgp, OSSwapBigToHostInt32(best_arch->offset),
	    OSSwapBigToHostInt32(best_arch->size)) != LOAD_SUCCESS) {
		best_arch = NULL;
		best_grade = 0;
	}

	  
                       
    
	if (best_arch == NULL) {
		lret = LOAD_BADARCH;
	} else {
		archret->cputype        =
		    OSSwapBigToHostInt32(best_arch->cputype);
		archret->cpusubtype     =
		    OSSwapBigToHostInt32(best_arch->cpusubtype);
		archret->offset         =
		    OSSwapBigToHostInt32(best_arch->offset);
		archret->size           =
		    OSSwapBigToHostInt32(best_arch->size);
		archret->align          =
		    OSSwapBigToHostInt32(best_arch->align);

		lret = LOAD_SUCCESS;
	}

	  
                                            
    
	return lret;
}

load_return_t
fatfile_getbestarch(
	vm_offset_t             data_ptr,
	vm_size_t               data_size,
	struct image_params     *imgp,
	struct fat_arch *archret,
	__unused bool affinity)
{
	int primary_type = cpu_type();


	  
                                                              
                                              
    
	load_return_t ret = fatfile_getarch(data_ptr, data_size, primary_type, CPU_ARCH_MASK, CPU_SUBTYPE_ANY, imgp, archret);
	return ret;
}

load_return_t
fatfile_getbestarch_for_cputype(
	cpu_type_t cputype,
	cpu_subtype_t cpusubtype,
	vm_offset_t data_ptr,
	vm_size_t data_size,
	struct image_params *imgp,
	struct fat_arch *archret)
{
	  
                                                                      
    
	return fatfile_getarch(data_ptr, data_size, cputype, 0, cpusubtype, imgp, archret);
}

                                                                       
                                      
 
                                                               
                            
 
                                        
                                               
                                             
                                                       
                 
 
                                                  
                                             
                                                                       
load_return_t
fatfile_getarch_with_bits(
	integer_t               archbits,
	vm_offset_t             data_ptr,
	vm_size_t               data_size,
	struct fat_arch         *archret)
{
	  
                                                          
                                                                 
    
	return fatfile_getarch(data_ptr, data_size, (archbits & CPU_ARCH_MASK) | (cpu_type() & ~CPU_ARCH_MASK), 0, CPU_SUBTYPE_ANY, NULL, archret);
}

  
                                                                       
  
                                                 
                                                                    
                                                              
                                                                       
                                                       
   
load_return_t
fatfile_validate_fatarches(vm_offset_t data_ptr, vm_size_t data_size)
{
	uint32_t magic;
	size_t nfat_arch, max_nfat_arch, i, j;
	size_t fat_header_size;

	struct fat_arch         *arches;
	struct fat_header       *header;

	if (sizeof(struct fat_header) > data_size) {
		return LOAD_FAILURE;
	}

	header = (struct fat_header *)data_ptr;
	magic = OSSwapBigToHostInt32(header->magic);
	nfat_arch = OSSwapBigToHostInt32(header->nfat_arch);

	if (magic != FAT_MAGIC) {
		                                  
		return LOAD_FAILURE;
	}

	max_nfat_arch = (data_size - sizeof(struct fat_header)) / sizeof(struct fat_arch);
	if (nfat_arch > max_nfat_arch) {
		                                                        
		return LOAD_BADMACHO;
	}

	                                                                                  
	fat_header_size = sizeof(struct fat_header) + nfat_arch * sizeof(struct fat_arch);
	arches = (struct fat_arch *)(data_ptr + sizeof(struct fat_header));

	for (i = 0; i < nfat_arch; i++) {
		uint32_t i_begin = OSSwapBigToHostInt32(arches[i].offset);
		uint32_t i_size = OSSwapBigToHostInt32(arches[i].size);
		uint32_t i_cputype = OSSwapBigToHostInt32(arches[i].cputype);
		uint32_t i_cpusubtype = OSSwapBigToHostInt32(arches[i].cpusubtype);

		if (i_begin < fat_header_size) {
			                                                                              
			return LOAD_BADMACHO;
		}

		if ((UINT32_MAX - i_size) < i_begin) {
			                                 
			return LOAD_BADMACHO;
		}
		uint32_t i_end = i_begin + i_size;

		for (j = i + 1; j < nfat_arch; j++) {
			uint32_t j_begin = OSSwapBigToHostInt32(arches[j].offset);
			uint32_t j_size = OSSwapBigToHostInt32(arches[j].size);
			uint32_t j_cputype = OSSwapBigToHostInt32(arches[j].cputype);
			uint32_t j_cpusubtype = OSSwapBigToHostInt32(arches[j].cpusubtype);

			if ((i_cputype == j_cputype) && (i_cpusubtype == j_cpusubtype)) {
				                                                                   
				return LOAD_BADMACHO;
			}

			if ((UINT32_MAX - j_size) < j_begin) {
				                                 
				return LOAD_BADMACHO;
			}
			uint32_t j_end = j_begin + j_size;

			if (i_begin <= j_begin) {
				if (i_end <= j_begin) {
					                             
				} else {
					                                                          
					return LOAD_BADMACHO;
				}
			} else {
				if (i_begin >= j_end) {
					                                                      
				} else {
					                                                                        
					return LOAD_BADMACHO;
				}
			}
		}
	}

	return LOAD_SUCCESS;
}
