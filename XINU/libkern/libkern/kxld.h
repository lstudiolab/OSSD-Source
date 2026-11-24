/*
 * Copyright (c) 2007-2008 Apple Inc. All rights reserved.
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
#ifndef _KXLD_H
#define _KXLD_H

#include <sys/types.h>
#include <mach/boolean.h>                   
#include <mach/kern_return.h>                   
#include <mach/machine.h>                                      
#include <mach/vm_types.h>

#include "kxld_types.h"

                                                                                
     
                                                                                

                                                                                
                                                                            
                                                                              
                                                       
                                                                   
                                                                       
                                                            
                                                             
                                                                    
                                                                          
                                                                   
                                                                                
kern_return_t kxld_create_context(
	KXLDContext **context,
	KXLDAllocateCallback allocate_callback,
	KXLDLoggingCallback log_callback,
	KXLDFlags flags,
	cpu_type_t cputype,
	cpu_subtype_t cpusubtype,
	vm_size_t pagesize)
__attribute__((nonnull(1), visibility("default")));

                                                                                
                                                                               
                                  
                                                                                
void kxld_destroy_context(
	KXLDContext *context)
__attribute__((nonnull, visibility("default")));

                                                                                
                                                                                 
                               
                                                                               
                                                                            
                                                               
                                                                  
                                                                          
                                                                             
                                                                             
                                                                               
                                      
                                                                       
                                                                           
                                                               
                                                                         
                                                                             
                                                                            
                                                            
                                                                     
                                                                                
kern_return_t kxld_link_file(
	KXLDContext *context,
	u_char *file,
	u_long size,
	const char *name,
	void *callback_data,
	KXLDDependency *dependencies,
	u_int num_dependencies,
	u_char **linked_object,
	kxld_addr_t *kmod_info_kern)
__attribute__((nonnull(1, 2, 4, 6, 8, 9), visibility("default")));


kern_return_t kxld_link_split_file(
	KXLDContext *context,
	splitKextLinkInfo *link_info,
	const char *name,
	void *callback_data,
	KXLDDependency *dependencies,
	u_int num_dependencies,
	kxld_addr_t *kmod_info_kern)
__attribute__((nonnull(1, 2, 3, 5, 7), visibility("default")));


                                                                                
                                                                                
boolean_t kxld_validate_copyright_string(const char *str)
__attribute__((pure, nonnull, visibility("default")));

#endif            
