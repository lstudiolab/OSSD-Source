/*
 * Copyright (c) 2012 Apple Inc. All rights reserved.
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

#ifndef KERN_KPC_H
#define KERN_KPC_H

                                                  

#include <machine/machine_kpc.h>
#include <kern/thread.h>               

__BEGIN_DECLS

                                    
#define KPC_CLASS_FIXED         (0)
#define KPC_CLASS_CONFIGURABLE  (1)
#define KPC_CLASS_POWER         (2)
#define KPC_CLASS_RAWPMU        (3)

#define KPC_CLASS_FIXED_MASK         (1u << KPC_CLASS_FIXED)
#define KPC_CLASS_CONFIGURABLE_MASK  (1u << KPC_CLASS_CONFIGURABLE)
#define KPC_CLASS_POWER_MASK         (1u << KPC_CLASS_POWER)
#define KPC_CLASS_RAWPMU_MASK        (1u << KPC_CLASS_RAWPMU)

#define KPC_PMU_ERROR     (0)
#define KPC_PMU_INTEL_V3  (1)
#define KPC_PMU_ARM_APPLE (2)
#define KPC_PMU_INTEL_V2  (3)
#define KPC_PMU_ARM_V2    (4)

#define KPC_ALL_CPUS (1u << 31)

                               
#define FIXED_ACTIONID(ctr)                     (kpc_actionid[(ctr)])
#define CONFIGURABLE_ACTIONID(ctr)              (kpc_actionid[(ctr) + kpc_fixed_count()])

                                    
#define FIXED_RELOAD(ctr)                       (current_cpu_datap()->cpu_kpc_reload[(ctr)])
#define FIXED_RELOAD_CPU(cpu, ctr)              (cpu_datap(cpu)->cpu_kpc_reload[(ctr)])
#define CONFIGURABLE_RELOAD(ctr)                (current_cpu_datap()->cpu_kpc_reload[(ctr) + kpc_fixed_count()])
#define CONFIGURABLE_RELOAD_CPU(cpu, ctr)       (cpu_datap(cpu)->cpu_kpc_reload[(ctr) + kpc_fixed_count()])

                                    
#define FIXED_SHADOW(ctr)                       (current_cpu_datap()->cpu_kpc_shadow[(ctr)])
#define FIXED_SHADOW_CPU(cpu, ctr)              (cpu_datap(cpu)->cpu_kpc_shadow[(ctr)])
#define CONFIGURABLE_SHADOW(ctr)                (current_cpu_datap()->cpu_kpc_shadow[(ctr) + kpc_fixed_count()])
#define CONFIGURABLE_SHADOW_CPU(cpu, ctr)       (cpu_datap(cpu)->cpu_kpc_shadow[(ctr) + kpc_fixed_count()])

   
                                                                           
                                                                                 
                                             
   
typedef void (*kpc_pm_handler_t)(boolean_t);

  
                                                  
  
                  
                                                   
  
          
                                                            
   
struct cpu_data;
extern boolean_t kpc_register_cpu(struct cpu_data *cpu_data);
extern void kpc_unregister_cpu(struct cpu_data *cpu_data);

extern bool kpc_supported;

               
extern void kpc_init(void);

                                          
extern void kpc_arch_init(void);

                                          
extern uint32_t kpc_get_classes(void);

                                                           
extern uint32_t kpc_get_running(void);

                                             
extern int kpc_get_pmu_version(void);

                                                                
                               
   
extern int kpc_set_running(uint32_t classes);

                       
extern int kpc_get_cpu_counters(boolean_t all_cpus, uint32_t classes,
    int *curcpu, uint64_t *buf);

                          
extern int kpc_get_shadow_counters( boolean_t all_cpus, uint32_t classes,
    int *curcpu, uint64_t *buf );

                                                 
extern int kpc_get_curthread_counters(uint32_t *inoutcount, uint64_t *buf);

                                                                      
extern uint32_t kpc_get_counter_count(uint32_t classes);
extern uint32_t kpc_get_config_count(uint32_t classes);

                                    
extern uint32_t kpc_get_thread_counting(void);
extern int      kpc_set_thread_counting(uint32_t classes);

                                  
extern int kpc_get_config(uint32_t classes, kpc_config_t *current_config);
extern int kpc_set_config(uint32_t classes, kpc_config_t *new_config);

                            
extern int kpc_get_period(uint32_t classes, uint64_t *period);
extern int kpc_set_period(uint32_t classes, uint64_t *period);

                                
extern int kpc_get_actionid(uint32_t classes, uint32_t *actionid);
extern int kpc_set_actionid(uint32_t classes, uint32_t *actionid);

                                       
extern void kpc_thread_create(thread_t thread);
extern void kpc_thread_destroy(thread_t thread);

                                                   
extern uint64_t *kpc_counterbuf_alloc(void);
extern void      kpc_counterbuf_free(uint64_t*);
extern uint32_t  kpc_get_counterbuf_size(void);

                                                     
extern int kpc_threads_counting;

                          
extern void kpc_thread_ast_handler( thread_t thread );

#ifdef MACH_KERNEL_PRIVATE

                                     

extern boolean_t kpc_off_cpu_active;

extern void kpc_off_cpu_internal(thread_t thread);
extern void kpc_off_cpu_update(void);

static inline void
kpc_off_cpu(thread_t thread)
{
	if (__improbable(kpc_off_cpu_active)) {
		kpc_off_cpu_internal(thread);
	}
}

#endif                                   

                                                            
extern int kpc_force_all_ctrs( task_t task, int val );
extern int kpc_get_force_all_ctrs( void );

                                                                                      
extern int kpc_force_all_ctrs_arch( task_t task, int val );

extern int kpc_set_sw_inc( uint32_t mask );

                                                
extern int kpc_get_whitelist_disabled( void );
extern int kpc_disable_whitelist( int val );

  
                                             
  
                                                                               
                                                                                
                                                                    
                           
                         
                              
  
                                                                             
   
extern boolean_t kpc_register_pm_handler(void (*handler)(boolean_t));

  
                                             
  
                 
                                                                          
                                                                         
  
                  
                                                                                 
                                                                  
                        
  
                       
                                                                           
                                                                               
                           
  
          
                                                                          
                                             
   
extern boolean_t kpc_reserve_pm_counters(uint64_t pmc_mask, kpc_pm_handler_t handler,
    boolean_t custom_config);

  
                                                                          
                     
   
extern void kpc_release_pm_counters(void);

  
                                                                        
  
                                                                           
                                                                            
                                                                           
            
   
extern void kpc_pm_acknowledge(boolean_t available_to_pm);

  
                                                           
  
                                                                               
                                                                          
            
   
extern boolean_t kpc_multiple_clients(void);

  
                                         
  
                                                                               
           
   
extern boolean_t kpc_controls_fixed_counters(void);

  
                                      
   
extern boolean_t kpc_controls_counter(uint32_t ctr);


extern void kpc_idle(void);
extern void kpc_idle_exit(void);


  
              
   

extern uint32_t kpc_actionid[KPC_MAX_COUNTERS];

                               
struct kpc_config_remote {
	uint32_t classes;
	kpc_config_t *configv;
	uint64_t pmc_mask;
};

                               
struct kpc_running_remote {
	uint32_t        classes;                                    
	uint64_t        cfg_target_mask;                                            
	uint64_t        cfg_state_mask;                                              
};

                               
struct kpc_get_counters_remote {
	uint32_t classes;
	uint32_t nb_counters;
	uint32_t buf_stride;
	uint64_t *buf;
};

int kpc_get_all_cpus_counters(uint32_t classes, int *curcpu, uint64_t *buf);
int kpc_get_curcpu_counters(uint32_t classes, int *curcpu, uint64_t *buf);
int kpc_get_fixed_counters(uint64_t *counterv);
int kpc_get_configurable_counters(uint64_t *counterv, uint64_t pmc_mask);
boolean_t kpc_is_running_fixed(void);
boolean_t kpc_is_running_configurable(uint64_t pmc_mask);
uint32_t kpc_fixed_count(void);
uint32_t kpc_configurable_count(void);
uint32_t kpc_fixed_config_count(void);
uint32_t kpc_configurable_config_count(uint64_t pmc_mask);
uint32_t kpc_rawpmu_config_count(void);
int kpc_get_fixed_config(kpc_config_t *configv);
int kpc_get_configurable_config(kpc_config_t *configv, uint64_t pmc_mask);
int kpc_get_rawpmu_config(kpc_config_t *configv);
uint64_t kpc_fixed_max(void);
uint64_t kpc_configurable_max(void);
int kpc_set_config_arch(struct kpc_config_remote *mp_config);
int kpc_set_period_arch(struct kpc_config_remote *mp_config);

__options_decl(kperf_kpc_flags_t, uint16_t, {
	KPC_KERNEL_PC = 0x01,
	KPC_KERNEL_COUNTING = 0x02,
	KPC_USER_COUNTING = 0x04,
});

void kpc_sample_kperf(uint32_t actionid, uint32_t counter, uint64_t config,
    uint64_t count, uintptr_t pc, kperf_kpc_flags_t flags);

int kpc_set_running_arch(struct kpc_running_remote *mp_config);


  
          
   

                                  
extern uint8_t kpc_popcount(uint64_t value);

                                                               
extern uint64_t kpc_get_configurable_pmc_mask(uint32_t classes);


                                                    
struct kpc_driver {
	uint32_t (*get_classes)(void);
	uint32_t (*get_running)(void);
	int      (*set_running)(uint32_t classes);
	int      (*get_cpu_counters)(boolean_t all_cpus, uint32_t classes,
	    int *curcpu, uint64_t *buf);
	int      (*get_curthread_counters)(uint32_t *inoutcount, uint64_t *buf);
	uint32_t (*get_counter_count)(uint32_t classes);
	uint32_t (*get_config_count)(uint32_t classes);
	int      (*get_config)(uint32_t classes, kpc_config_t *current_config);
	int      (*set_config)(uint32_t classes, kpc_config_t *new_config);
	int      (*get_period)(uint32_t classes, uint64_t *period);
	int      (*set_period)(uint32_t classes, uint64_t *period);
};

__END_DECLS

#endif                           
