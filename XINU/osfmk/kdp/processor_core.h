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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
#ifndef _PROCESSOR_CORE_H_
#define _PROCESSOR_CORE_H_

#include <stdint.h>
#include <mach/vm_types.h>
#include <mach/kern_return.h>
#include <mach/machine.h>
#include <mach_debug/mach_debug_types.h>

__BEGIN_DECLS

  
                                                     
  
                                                                       
                                                                   
  
                                                                      
                              
  
                                           
                                                                            
                                                                      
                           
  
                                                                                      
                                                                                     
            
  
                                                                                       
          
   

void kern_coredump_log(void *context, const char *string, ...) __printflike(2, 3);

  
                                                                                           
                                                                       
  
                                                                                       
                                                                                        
                                                                                                
                                                                                                        
                                                                                
                                                              
                                   
                                      
                                                                     
   
typedef kern_return_t (*core_save_summary_cb)(uint64_t core_segment_count, uint64_t core_byte_count,
    uint64_t thread_count, uint64_t thread_state_size,
    uint64_t misc_bytes_count, void *context);

  
                                                                               
                                                                      
  
                                                                                           
                                                                  
  
                                                                                                      
                                                                                       
  
                                                                
                                                              
                                                                        
   
typedef kern_return_t (*core_save_segment_descriptions_cb)(uint64_t seg_start, uint64_t seg_end,
    void *context);
  
                                                                       
                                                              
  
                                                                                   
                                                         
  
                                                                  
                                                                           
            
  
                                                                    
                                                                   
   
typedef kern_return_t (*core_save_thread_state_cb)(void *thread_state, void *context);

  
                                                                  
                                                         
  
                                                          
  
                                                        
                                                                                                            
                                                         
   
typedef kern_return_t (*core_save_sw_vers_cb)(void *sw_vers, uint64_t length, void *context);

  
                                                                       
                                                              
  
                                                                                           
                                                                  
  
                                                                                   
                                                               
                                                               
   
typedef kern_return_t (*core_save_segment_data_cb)(void *seg_data, uint64_t length, void *context);

  
                                                             
                                                                    
                                                          
  
                                                          
  
                                                  
                                                   
                                                             
   
typedef kern_return_t (*core_save_misc_data_cb)(void *misc_data, uint64_t length, void *context);

typedef struct {
	kern_return_t (*kcc_coredump_init)(void *refcon, void *context);                                                                              
	kern_return_t (*kcc_coredump_get_summary)(void *refcon, core_save_summary_cb callback, void *context);
	kern_return_t (*kcc_coredump_save_segment_descriptions)(void *refcon, core_save_segment_descriptions_cb callback, void *context);
	kern_return_t (*kcc_coredump_save_thread_state)(void *refcon, void *buf, core_save_thread_state_cb callback, void *context);
	kern_return_t (*kcc_coredump_save_sw_vers)(void *refcon, core_save_sw_vers_cb callback, void *context);
	kern_return_t (*kcc_coredump_save_segment_data)(void *refcon, core_save_segment_data_cb callback, void *context);
	kern_return_t (*kcc_coredump_save_misc_data)(void *refcon, core_save_misc_data_cb callback, void *context);               
	                      
} kern_coredump_callback_config;

#define KERN_COREDUMP_MAX_CORES MACH_CORE_FILEHEADER_MAXFILES
#define KERN_COREDUMP_MIN_CONFIG_VERSION 1
#define KERN_COREDUMP_CONFIG_VERSION 1
#define KERN_COREDUMP_VERSIONSTRINGMAXSIZE 256

  
                                                                             
                                                                                        
                                                                      
   
kern_return_t kern_register_coredump_helper(int kern_coredump_config_vers, const kern_coredump_callback_config *kc_callbacks, void *refcon,
    const char *core_description, boolean_t is64bit, uint32_t mh_magic, cpu_type_t cpu_type, cpu_subtype_t cpu_subtype);

#if PRIVATE

kern_return_t kern_register_xnu_coredump_helper(kern_coredump_callback_config *kc_callbacks);

kern_return_t kern_do_coredump(void *core_outvars, boolean_t kernel_only, uint64_t first_file_offset, uint64_t *last_file_offset);

#define KERN_COREDUMP_HEADERSIZE 4096
static_assert((sizeof(struct mach_core_fileheader) <= KERN_COREDUMP_HEADERSIZE), "struct mach_core_fileheader larger than KERN_COREDUMP_HEADERSIZE (space that will be allocated for it in the corefile)");

#define KERN_COREDUMP_MAXDEBUGLOGSIZE 16384
#define KERN_COREDUMP_BEGIN_FILEBYTES_ALIGN 4096
#define KERN_COREDUMP_THREADSIZE_MAX 1024

#endif              

__END_DECLS
#endif                         
