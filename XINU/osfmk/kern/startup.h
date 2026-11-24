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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 */

#ifdef  XNU_KERNEL_PRIVATE

#ifndef _KERN_STARTUP_H_
#define _KERN_STARTUP_H_

#include <stdbool.h>
#include <stdint.h>

#include <libkern/section_keywords.h>

__BEGIN_DECLS

#pragma GCC visibility push(hidden)

   
                               
  
            
                                                                         
                                                 
  
              
                                                                    
                        
   
__enum_decl(startup_subsystem_id_t, uint32_t, {
	STARTUP_SUB_NONE = 0,                                                    

	STARTUP_SUB_TUNABLES,                                                    
	STARTUP_SUB_LOCKS_EARLY,                                                 
	STARTUP_SUB_KPRINTF,                                                     

	STARTUP_SUB_PMAP_STEAL,                                                  
	STARTUP_SUB_VM_KERNEL,                                                   
	STARTUP_SUB_KMEM,                                                        
	STARTUP_SUB_KMEM_ALLOC,                                                  
	STARTUP_SUB_ZALLOC,                                                      
	STARTUP_SUB_PERCPU,                                                      
	STARTUP_SUB_LOCKS,                                                       

	STARTUP_SUB_CODESIGNING,                                                 
	STARTUP_SUB_OSLOG,                                                       
	STARTUP_SUB_MACH_IPC,                                                    
	STARTUP_SUB_SYSCTL,                                                      
	STARTUP_SUB_EARLY_BOOT,                                                  

	STARTUP_SUB_LOCKDOWN = ~0u,                                              
});

   
                                                            
   
extern startup_subsystem_id_t startup_phase;

   
                        
  
            
                                                                           
   
__options_decl(startup_debug_t, uint32_t, {
	STARTUP_DEBUG_NONE    = 0x00000000,
	STARTUP_DEBUG_VERBOSE = 0x00000001,
});

#if DEBUG || DEVELOPMENT
extern startup_debug_t startup_debug;
#else
#define startup_debug  STARTUP_DEBUG_NONE
#endif

   
                     
  
            
                                                                          
                                                                     
  
               
                                                                              
                                                
  
                                                                            
  
                                                                           
                                                                   
  
                                                                             
                                                      
  
                                                                              
                                                   
  
                                                                           
                               
   
__enum_decl(startup_rank_t, uint32_t, {
#define STARTUP_RANK_NTH(n) \
	(enum startup_rank)(n)
	STARTUP_RANK_FIRST          = 0,
	STARTUP_RANK_SECOND         = 1,
	STARTUP_RANK_THIRD          = 2,
	STARTUP_RANK_FOURTH         = 3,

	STARTUP_RANK_MIDDLE         = 0x7fffffff,

#define STARTUP_RANK_LATE_NTH(n) \
	(enum startup_rank)(STARTUP_RANK_MIDDLE + 1 + (n))

	STARTUP_RANK_LAST           = 0xffffffff,
});

#if KASAN
  
                                                                             
                                                              
   
#define STARTUP_CODE_SEGSECT "__TEXT,__text"
#define STARTUP_DATA_SEGSECT "__DATA,__init"
#define STARTUP_HOOK_SEGMENT "__DATA"
#define STARTUP_HOOK_SECTION "__init_entry_set"
#elif defined(__x86_64__)
                                                               
#define STARTUP_CODE_SEGSECT "__TEXT,__text"
#define STARTUP_DATA_SEGSECT "__KLDDATA,__init"
#define STARTUP_HOOK_SEGMENT "__KLDDATA"
#define STARTUP_HOOK_SECTION "__init_entry_set"
#else
                                                          
#define STARTUP_CODE_SEGSECT "__TEXT,__text"
#define STARTUP_DATA_SEGSECT "__BOOTDATA,__init"
#define STARTUP_HOOK_SEGMENT "__BOOTDATA"
#define STARTUP_HOOK_SECTION "__init_entry_set"
#endif

   
                        
  
            
                                                                             
  
               
                                                                          
   
#define __startup_func \
	__PLACE_IN_SECTION(STARTUP_CODE_SEGSECT) \
	__attribute__((cold, visibility("hidden")))

   
                        
  
            
                                                                      
  
               
                                                                          
   
#define __startup_data \
	__PLACE_IN_SECTION(STARTUP_DATA_SEGSECT)

   
                 
  
            
                                      
   
#define STARTUP(subsystem, rank, func) \
	__STARTUP(func, __LINE__, subsystem, rank, func)

   
                     
  
            
                                                             
   
#define STARTUP_ARG(subsystem, rank, func, arg) \
	__STARTUP_ARG(func, __LINE__, subsystem, rank, func, arg)

   
                 
  
            
                                                                        
                                               
  
                
                                     
  
             
                                                 
  
             
                                                       
  
                       
                                                               
   
#define TUNABLE(type_t, var, key, default_value) \
	SECURITY_READ_ONLY_LATE(type_t) var = default_value; \
	__TUNABLE(type_t, var, key)

   
                           
  
            
                                                                        
                                               
  
                
                                     
  
             
                                                 
  
             
                                                       
  
                       
                                                               
   
#define TUNABLE_WRITEABLE(type_t, var, key, default_value) \
	type_t var = default_value; \
	__TUNABLE(type_t, var, key)

#pragma mark - internals

#define __TUNABLE(type_t, var, key) \
	static __startup_data char __startup_TUNABLES_name_ ## var[] = key; \
	static __startup_data struct startup_tunable_spec \
	__startup_TUNABLES_spec_ ## var = { \
	    .name = __startup_TUNABLES_name_ ## var, \
	    .var_addr = (void *)&var, \
	    .var_len = sizeof(type_t), \
	    .var_is_bool = __builtin_types_compatible_p(bool, type_t), \
	}; \
	__STARTUP_ARG(var, __LINE__, TUNABLES, STARTUP_RANK_FIRST, \
	    kernel_startup_tunable_init, &__startup_TUNABLES_spec_ ## var)

#ifdef __cplusplus
#define __STARTUP_FUNC_CAST(func, a) \
	    (void(*)(const void *))func
#else
#define __STARTUP_FUNC_CAST(func, a) \
	    (typeof(func(a))(*)(const void *))func
#endif


#define __STARTUP1(name, line, subsystem, rank, func, a, b) \
	__PLACE_IN_SECTION(STARTUP_HOOK_SEGMENT "," STARTUP_HOOK_SECTION) \
	static const struct startup_entry \
	__startup_ ## subsystem ## _entry_ ## name ## _ ## line = { \
	    STARTUP_SUB_ ## subsystem, \
	    rank, __STARTUP_FUNC_CAST(func, a), b, \
	}

#define __STARTUP(name, line, subsystem, rank, func) \
	__STARTUP1(name, line, subsystem, rank, func, , NULL)

#define __STARTUP_ARG(name, line, subsystem, rank, func, arg) \
	__STARTUP1(name, line, subsystem, rank, func, arg, arg)

struct startup_entry {
	startup_subsystem_id_t subsystem;
	startup_rank_t         rank;
	void                 (*func)(const void *);
	const void            *arg;
};

struct startup_tunable_spec {
	const char *name;
	void       *var_addr;
	int         var_len;
	bool        var_is_bool;
};

  
                                          
   

                       
extern void kernel_startup_bootstrap(void);
extern void kernel_startup_initialize_upto(startup_subsystem_id_t upto);
extern void kernel_startup_tunable_init(const struct startup_tunable_spec *);
extern void kernel_bootstrap(void);

                                        
extern void machine_init(void);

extern void slave_main(void *machine_param);

  
                                                               
   

                              
extern void slave_machine_init(void *machine_param);

                                    
extern void device_service_create(void);

#ifdef  MACH_BSD

                                  
extern void bsd_init(void);

#endif                

#pragma GCC visibility pop

__END_DECLS

#endif                        

#endif                          
