/*
 * Copyright (c) 2013 Apple Inc. All rights reserved.
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

#ifndef _SYS_COALITION_H_
#define _SYS_COALITION_H_

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <mach/coalition.h>

__BEGIN_DECLS

#ifndef KERNEL
                                  

              
int coalition_create(uint64_t *cid_out, uint32_t flags);
int coalition_terminate(uint64_t cid, uint32_t flags);
int coalition_reap(uint64_t cid, uint32_t flags);

                                                                         
int coalition_info_resource_usage(uint64_t cid, struct coalition_resource_usage *cru, size_t sz);
int coalition_info_set_name(uint64_t cid, const char *name, size_t size);
int coalition_info_set_efficiency(uint64_t cid, uint64_t flags);
int coalition_ledger_set_logical_writes_limit(uint64_t cid, int64_t limit);

#else             

#if CONFIG_COALITIONS
                              

  
                
                                                                       
   
uint64_t coalition_id(coalition_t coal);


  
                       
                                                           
  
                                                  
  
              
                                                                        
                                                                  
                                                                       
                                                                   
                                                       
                                                                       
                                                   
                                                                     
  
                                                        
                                                                  
                                                                          
   
extern int coalitions_get_list(int type, struct procinfo_coalinfo *coal_list, int list_sz);


  
                      
                                  
  
              
                                           
                                                                       
                                                                  
  
                                               
   
extern coalition_t task_get_coalition(task_t task, int coal_type);


  
                       
                                             
  
              
                                           
                                                  
                                                                                
  
                                                                      
   
extern boolean_t coalition_is_leader(task_t task, coalition_t coal);

  
                        
                                                          
  
              
                                           
  
                                                                           
                                                                           
                                                                              
                                                  
   
extern task_t coalition_get_leader(coalition_t coal);


  
                            
                                                    
  
              
                                               
  
                                                
   
extern int coalition_get_task_count(coalition_t coal);

  
                            
                                                                           
  
              
                                               
                                                                         
                                          
  
                                                                     
   
extern uint64_t coalition_get_page_count(coalition_t coal, int *ntasks);

  
                          
                                                                          
              
  
              
                                                 
                                                                            
                                                                          
                                                                   
                                                                           
                                                                           
                          
                                                                          
                                                                 
                                                                         
                                                                              
                                                               
  
        
                                                                           
                                                                         
                                                                            
                                                                            
                                                                           
                                                                              
                                                                
                                                         
  
                                                                           
                                    
  
                        
                                                                  
                                                          
                                                                        
                                                                           
                                            
  
   
extern int coalition_get_pid_list(coalition_t coal, uint32_t rolemask,
    int sort_order, int *pid_list, int list_sz);

#else                         
static inline uint64_t
coalition_id(__unused coalition_t coal)
{
	return 0;
}

static inline int
coalitions_get_list(__unused int type,
    __unused struct procinfo_coalinfo *coal_list,
    __unused int list_sz)
{
	return 0;
}

static inline coalition_t
coalition_get_leader(__unused task_t task,
    __unused int coal_type)
{
	return COALITION_NULL;
}

static inline boolean_t
coalition_is_leader(__unused task_t task,
    __unused coalition_t coal)
{
	return FALSE;
}

static inline int
coalition_get_task_count(__unused coalition_t coal)
{
	return 0;
}

static inline uint64_t
coalition_get_page_count(__unused coalition_t coal,
    __unused int *ntasks)
{
	return 0;
}

static inline int
coalition_get_pid_list(__unused coalition_t coal,
    __unused uint32_t rolemask,
    __unused int sort_order,
    __unused int *pid_list,
    __unused int list_sz)
{
	return 0;
}
#endif

#endif             

__END_DECLS

#endif                        
