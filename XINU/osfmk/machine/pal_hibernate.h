/*
 * Copyright (c) 2010 Apple Inc. All rights reserved.
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
   
                                                     
   

#ifndef _MACHINE_PAL_HIBERNATE_H
#define _MACHINE_PAL_HIBERNATE_H

#include <sys/cdefs.h>

#if defined (__i386__) || defined(__x86_64__)
#include "i386/pal_hibernate.h"
#elif defined (__arm__)
                                
#elif defined(__arm64__)
#include "arm64/pal_hibernate.h"
#else
#error architecture not supported
#endif

__BEGIN_DECLS

   
                                       
                                                                                                             
   
typedef enum {
	pal_hib_restore_stage_dram_pages    = 0,
	pal_hib_restore_stage_preview_pages = 1,
	pal_hib_restore_stage_handoff_data  = 2,
} pal_hib_restore_stage_t;

   
                             
                                                                                                         
                                                                                                  
                                                                                                            
                                                                                              
   
typedef struct pal_hib_ctx pal_hib_ctx_t;

   
                            
                                                                                            
                                                         
  
                                                                                      
                                                                                      
                                                                        
   
void __hib_assert(const char *file, int line, const char *expression) __attribute__((noreturn));
#define HIB_ASSERT(ex) \
	(__builtin_expect(!!((ex)), 1L) ? (void)0 : __hib_assert(__FILE__, __LINE__, # ex))

   
                           
                                                                                                  
  
                                                             
                                                              
  
                                                                           
   
uintptr_t pal_hib_map(pal_hib_map_type_t virt, uint64_t phys);

   
                                         
                                                                                            
  
                                                                 
   
void pal_hib_restore_pal_state(uint32_t *src);

   
                            
                                                             
   
void pal_hib_init(void);

   
                                  
                                                                                  
   
void pal_hib_write_hook(void);

   
                                   
                                                                                                  
                                                                       
  
                                                                                       
                                                                                           
                                                                                                
   
void pal_hib_resume_init(pal_hib_ctx_t *palHibCtx, hibernate_page_list_t *map, uint32_t *nextFree);

   
                                     
                                                                                      
  
                                                                                       
                                                                                                      
                                                                                 
   
void pal_hib_restored_page(pal_hib_ctx_t *palHibCtx, pal_hib_restore_stage_t stage, ppnum_t ppnum);

   
                               
                                                                                    
  
                                                                                       
   
void pal_hib_patchup(pal_hib_ctx_t *palHibCtx);

   
                                             
                                                                                                                        
  
                                                                                   
                                                                             
   
void pal_hib_teardown_pmap_structs(addr64_t *unneeded_start, addr64_t *unneeded_end);

   
                                            
                                                                                                        
   
void pal_hib_rebuild_pmap_structs(void);

   
                                       
                                                      
  
                                                   
                                                                                            
                                                                                       
                                                                 
   
void pal_hib_decompress_page(void *src, void *dst, void *scratch, unsigned int compressedSize);

__END_DECLS

#endif                               
