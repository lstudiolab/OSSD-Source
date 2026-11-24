/*
 * Copyright (c) 2007-2020 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */

#ifndef _ARM_MACHINE_ROUTINES_H_
#define _ARM_MACHINE_ROUTINES_H_

#include <mach/mach_types.h>
#include <mach/vm_types.h>
#include <mach/boolean.h>
#include <kern/kern_types.h>
#include <pexpert/pexpert.h>

#include <sys/cdefs.h>
#include <sys/appleapiopts.h>

#include <stdarg.h>

__BEGIN_DECLS

#ifdef XNU_KERNEL_PRIVATE
#ifdef __arm64__
typedef bool (*expected_fault_handler_t)(arm_saved_state_t *);
#endif                
#endif                         

                        

void ml_cpu_signal(unsigned int cpu_id);
void ml_cpu_signal_deferred_adjust_timer(uint64_t nanosecs);
uint64_t ml_cpu_signal_deferred_get_timer(void);
void ml_cpu_signal_deferred(unsigned int cpu_id);
void ml_cpu_signal_retract(unsigned int cpu_id);
bool ml_cpu_signal_is_enabled(void);

                           
void    ml_init_interrupt(void);

                            
boolean_t ml_get_interrupts_enabled(void);

                            
boolean_t ml_set_interrupts_enabled(boolean_t enable);
boolean_t ml_early_set_interrupts_enabled(boolean_t enable);

                                           
boolean_t ml_at_interrupt_context(void);

                               
void ml_cause_interrupt(void);

                                                 
#if INTERRUPT_MASKED_DEBUG
extern boolean_t interrupt_masked_debug;
extern uint64_t interrupt_masked_timeout;
extern uint64_t stackshot_interrupt_masked_timeout;

#define INTERRUPT_MASKED_DEBUG_START(handler_addr, type)                                    \
do {                                                                                        \
    if (interrupt_masked_debug) {                                                           \
	    thread_t thread = current_thread();                                                 \
	    thread->machine.int_type = type;                                                    \
	    thread->machine.int_handler_addr = (uintptr_t)VM_KERNEL_STRIP_PTR(handler_addr);    \
	    thread->machine.inthandler_timestamp = ml_get_timebase();                           \
	    thread->machine.int_vector = (uintptr_t)NULL;                                       \
    }                                                                                       \
} while (0)

#define INTERRUPT_MASKED_DEBUG_END()                                                        \
do {                                                                                        \
	if (interrupt_masked_debug) {                                                           \
	    thread_t thread = current_thread();                                                 \
	    ml_check_interrupt_handler_duration(thread);                                        \
	}                                                                                       \
} while (0)

void ml_irq_debug_start(uintptr_t handler, uintptr_t vector);
void ml_irq_debug_end(void);

void ml_spin_debug_reset(thread_t thread);
void ml_spin_debug_clear(thread_t thread);
void ml_spin_debug_clear_self(void);
void ml_check_interrupts_disabled_duration(thread_t thread);
void ml_check_stackshot_interrupt_disabled_duration(thread_t thread);
void ml_check_interrupt_handler_duration(thread_t thread);
#else
#define INTERRUPT_MASKED_DEBUG_START(handler_addr, type)
#define INTERRUPT_MASKED_DEBUG_END()
#endif

#ifdef XNU_KERNEL_PRIVATE
extern bool ml_snoop_thread_is_on_core(thread_t thread);
extern boolean_t ml_is_quiescing(void);
extern void ml_set_is_quiescing(boolean_t);
extern uint64_t ml_get_booter_memory_size(void);
#endif

                                            
typedef void (*time_base_enable_t)(cpu_id_t cpu_id, boolean_t enable);
#if defined(PEXPERT_KERNEL_PRIVATE) || defined(MACH_KERNEL_PRIVATE)
                                                    
typedef void (*cache_dispatch_t)(cpu_id_t cpu_id, unsigned int select, unsigned int param0, unsigned int param1);

typedef uint32_t (*get_decrementer_t)(void);
typedef void (*set_decrementer_t)(uint32_t);
typedef void (*fiq_handler_t)(void);

#endif

#define CacheConfig                     0x00000000UL
#define CacheControl                    0x00000001UL
#define CacheClean                      0x00000002UL
#define CacheCleanRegion                0x00000003UL
#define CacheCleanFlush                 0x00000004UL
#define CacheCleanFlushRegion           0x00000005UL
#define CacheShutdown                   0x00000006UL

#define CacheControlEnable              0x00000000UL

#define CacheConfigCCSIDR               0x00000001UL
#define CacheConfigSize                 0x00000100UL

                                          
typedef void (*processor_idle_t)(cpu_id_t cpu_id, boolean_t enter, uint64_t *new_timeout_ticks);

                                       
typedef void (*idle_tickle_t)(void);

                                      
typedef void (*idle_timer_t)(void *refcon, uint64_t *new_timeout_ticks);

                             
typedef void (*ipi_handler_t)(void);

                                  
typedef void (*lockdown_handler_t)(void *);

                                                  
typedef void (*platform_error_handler_t)(void *refcon, vm_offset_t fault_addr);

  
                                                                 
                                                              
                                             
                                                                        
                            
   

                                               
typedef enum{
	EXCB_CLASS_ILLEGAL_INSTR_SET,
#ifdef CONFIG_XNUPOST
	EXCB_CLASS_TEST1,
	EXCB_CLASS_TEST2,
	EXCB_CLASS_TEST3,
#endif
	EXCB_CLASS_MAX                              
}
ex_cb_class_t;

                                                                   
typedef enum{
	EXCB_ACTION_RERUN,                                        
	EXCB_ACTION_NONE,                                            
#ifdef CONFIG_XNUPOST
	EXCB_ACTION_TEST_FAIL,
#endif
}
ex_cb_action_t;

  
                  
                                                                          
                                                                              
                                                            
                                                  
   
typedef struct{
	vm_offset_t far;
}
ex_cb_state_t;

                              
typedef ex_cb_action_t (*ex_cb_t) (
	ex_cb_class_t           cb_class,
	void                            *refcon,                           
	const ex_cb_state_t     *state                    
	);

  
                        
                                                                  
                                                  
   
kern_return_t ex_cb_register(
	ex_cb_class_t   cb_class,
	ex_cb_t                 cb,
	void                    *refcon );

  
                                                                                   
   
ex_cb_action_t ex_cb_invoke(
	ex_cb_class_t   cb_class,
	vm_offset_t         far);


void ml_parse_cpu_topology(void);

unsigned int ml_get_cpu_count(void);

unsigned int ml_get_cluster_count(void);

int ml_get_boot_cpu_number(void);

int ml_get_cpu_number(uint32_t phys_id);

int ml_get_cluster_number(uint32_t phys_id);

int ml_get_max_cpu_number(void);

int ml_get_max_cluster_number(void);

unsigned int ml_get_first_cpu_id(unsigned int cluster_id);

#ifdef __arm64__
int ml_get_cluster_number_local(void);
unsigned int ml_get_cpu_number_local(void);
#endif                

                                
struct ml_cpu_info {
	unsigned long           vector_unit;
	unsigned long           cache_line_size;
	unsigned long           l1_icache_size;
	unsigned long           l1_dcache_size;
	unsigned long           l2_settings;
	unsigned long           l2_cache_size;
	unsigned long           l3_settings;
	unsigned long           l3_cache_size;
};
typedef struct ml_cpu_info ml_cpu_info_t;

typedef enum {
	CLUSTER_TYPE_SMP,
	CLUSTER_TYPE_E,
	CLUSTER_TYPE_P,
} cluster_type_t;

cluster_type_t ml_get_boot_cluster(void);

   
                             
                                                 
  
                                                                            
                                                                                  
                                                                          
                                                        
                                                
                                                                   
                                                                                                   
                                                                              
                                                                                         
                                                               
                                                                                         
                                                               
                                                                                                           
                                                                         
                                                               
                                                                                    
                                                                                                   
                                                                                         
                                                                                        
                                                                         
                                                               
                                                                                 
                                                                                   
   
typedef struct ml_topology_cpu {
	unsigned int                    cpu_id;
	uint32_t                        phys_id;
	unsigned int                    cluster_id;
	unsigned int                    die_id;
	cluster_type_t                  cluster_type;
	uint32_t                        l2_access_penalty;
	uint32_t                        l2_cache_size;
	uint32_t                        l2_cache_id;
	uint32_t                        l3_cache_size;
	uint32_t                        l3_cache_id;
	vm_offset_t                     cpu_IMPL_regs;
	uint64_t                        cpu_IMPL_pa;
	uint64_t                        cpu_IMPL_len;
	vm_offset_t                     cpu_UTTDBG_regs;
	uint64_t                        cpu_UTTDBG_pa;
	uint64_t                        cpu_UTTDBG_len;
	vm_offset_t                     coresight_regs;
	uint64_t                        coresight_pa;
	uint64_t                        coresight_len;
	unsigned int                    die_cluster_id;
	unsigned int                    cluster_core_id;
} ml_topology_cpu_t;

   
                                 
                                                
  
                                                        
                                                                   
                                                                             
                                                                       
                                                                                                     
                                                                                         
                                                                                                          
                                                                        
                                                              
                                                                                                          
                                                                        
                                                              
   
typedef struct ml_topology_cluster {
	unsigned int                    cluster_id;
	cluster_type_t                  cluster_type;
	unsigned int                    num_cpus;
	unsigned int                    first_cpu_id;
	uint64_t                        cpu_mask;
	vm_offset_t                     acc_IMPL_regs;
	uint64_t                        acc_IMPL_pa;
	uint64_t                        acc_IMPL_len;
	vm_offset_t                     cpm_IMPL_regs;
	uint64_t                        cpm_IMPL_pa;
	uint64_t                        cpm_IMPL_len;
} ml_topology_cluster_t;

                                                                         
                                                                     
                      
#define CPU_TOPOLOGY_VERSION 1

   
                              
                                                                                                             
                                                                                                       
                                                                                                        
                                                                                                            
                                                            
  
                                                                                
                                                             
                                                              
                                                                                          
                                                                
                                                       
                                                           
                                                                        
                                                                                       
                                                                                    
                                                                                               
   
typedef struct ml_topology_info {
	unsigned int                    version;
	unsigned int                    num_cpus;
	unsigned int                    max_cpu_id;
	unsigned int                    num_clusters;
	unsigned int                    max_cluster_id;
	unsigned int                    max_die_id;
	ml_topology_cpu_t               *cpus;
	ml_topology_cluster_t           *clusters;
	ml_topology_cpu_t               *boot_cpu;
	ml_topology_cluster_t           *boot_cluster;
	unsigned int                    chip_revision;
} ml_topology_info_t;

   
                                 
                                                                                                
                                                                               
   
const ml_topology_info_t *ml_get_topology_info(void);

   
                           
                                                                                    
                                                                                    
   
void ml_map_cpu_pio(void);

                                      
struct ml_processor_info {
	cpu_id_t                        cpu_id;
	vm_offset_t                     start_paddr;
	boolean_t                       supports_nap;
	void                            *platform_cache_dispatch;
	time_base_enable_t              time_base_enable;
	processor_idle_t                processor_idle;
	idle_tickle_t                   *idle_tickle;
	idle_timer_t                    idle_timer;
	void                            *idle_timer_refcon;
	vm_offset_t                     powergate_stub_addr;
	uint32_t                        powergate_stub_length;
	uint32_t                        powergate_latency;
	platform_error_handler_t        platform_error_handler;
	uint64_t                        regmap_paddr;
	uint32_t                        phys_id;
	uint32_t                        log_id;
	uint32_t                        l2_access_penalty;
	uint32_t                        cluster_id;
	cluster_type_t                  cluster_type;
	uint32_t                        l2_cache_id;
	uint32_t                        l2_cache_size;
	uint32_t                        l3_cache_id;
	uint32_t                        l3_cache_size;
};
typedef struct ml_processor_info ml_processor_info_t;

#if defined(PEXPERT_KERNEL_PRIVATE) || defined(MACH_KERNEL_PRIVATE)
                                 
struct  tbd_ops {
	fiq_handler_t     tbd_fiq_handler;
	get_decrementer_t tbd_get_decrementer;
	set_decrementer_t tbd_set_decrementer;
};
typedef struct tbd_ops        *tbd_ops_t;
typedef struct tbd_ops        tbd_ops_data_t;
#endif

   
                                  
  
                                                              
  
                                                                               
                                                                                
                      
  
                                                                           
                    
  
                                                                              
                                                                               
  
                                                                               
                                 
  
                                                                               
                                 
  
                                                                 
   
kern_return_t ml_processor_register(ml_processor_info_t *ml_processor_info,
    processor_t *processor, ipi_handler_t *ipi_handler,
    perfmon_interrupt_handler_func *pmi_handler);

                                 
kern_return_t ml_lockdown_handler_register(lockdown_handler_t, void *);

#if XNU_KERNEL_PRIVATE
void ml_lockdown_init(void);

                                                   
void ml_panic_trap_to_debugger(const char *panic_format_str,
    va_list *panic_args,
    unsigned int reason,
    void *ctx,
    uint64_t panic_options_mask,
    unsigned long panic_caller);
#endif                         

                           
void ml_install_interrupt_handler(
	void *nub,
	int source,
	void *target,
	IOInterruptHandler handler,
	void *refCon);

vm_offset_t
    ml_static_vtop(
	vm_offset_t);

kern_return_t
ml_static_verify_page_protections(
	uint64_t base, uint64_t size, vm_prot_t prot);

vm_offset_t
    ml_static_ptovirt(
	vm_offset_t);

vm_offset_t ml_static_slide(
	vm_offset_t vaddr);

vm_offset_t ml_static_unslide(
	vm_offset_t vaddr);

                                                                     
uint64_t ml_get_abstime_offset(void);

                                                                       
uint64_t ml_get_conttime_offset(void);

#ifdef __APPLE_API_UNSTABLE
                              
boolean_t ml_probe_read(
	vm_offset_t paddr,
	unsigned int *val);
boolean_t ml_probe_read_64(
	addr64_t paddr,
	unsigned int *val);

                                
unsigned int ml_phys_read_byte(
	vm_offset_t paddr);
unsigned int ml_phys_read_byte_64(
	addr64_t paddr);

                                     
unsigned int ml_phys_read_half(
	vm_offset_t paddr);
unsigned int ml_phys_read_half_64(
	addr64_t paddr);

                               
unsigned int ml_phys_read(
	vm_offset_t paddr);
unsigned int ml_phys_read_64(
	addr64_t paddr);
unsigned int ml_phys_read_word(
	vm_offset_t paddr);
unsigned int ml_phys_read_word_64(
	addr64_t paddr);

unsigned long long ml_io_read(uintptr_t iovaddr, int iovsz);
unsigned int ml_io_read8(uintptr_t iovaddr);
unsigned int ml_io_read16(uintptr_t iovaddr);
unsigned int ml_io_read32(uintptr_t iovaddr);
unsigned long long ml_io_read64(uintptr_t iovaddr);

extern void ml_io_write(uintptr_t vaddr, uint64_t val, int size);
extern void ml_io_write8(uintptr_t vaddr, uint8_t val);
extern void ml_io_write16(uintptr_t vaddr, uint16_t val);
extern void ml_io_write32(uintptr_t vaddr, uint32_t val);
extern void ml_io_write64(uintptr_t vaddr, uint64_t val);

                                       
unsigned long long ml_phys_read_double(
	vm_offset_t paddr);
unsigned long long ml_phys_read_double_64(
	addr64_t paddr);

                                 
void ml_phys_write_byte(
	vm_offset_t paddr, unsigned int data);
void ml_phys_write_byte_64(
	addr64_t paddr, unsigned int data);

                                      
void ml_phys_write_half(
	vm_offset_t paddr, unsigned int data);
void ml_phys_write_half_64(
	addr64_t paddr, unsigned int data);

                                 
void ml_phys_write(
	vm_offset_t paddr, unsigned int data);
void ml_phys_write_64(
	addr64_t paddr, unsigned int data);
void ml_phys_write_word(
	vm_offset_t paddr, unsigned int data);
void ml_phys_write_word_64(
	addr64_t paddr, unsigned int data);

                                        
void ml_phys_write_double(
	vm_offset_t paddr, unsigned long long data);
void ml_phys_write_double_64(
	addr64_t paddr, unsigned long long data);

void ml_static_mfree(
	vm_offset_t,
	vm_size_t);

kern_return_t
ml_static_protect(
	vm_offset_t start,
	vm_size_t size,
	vm_prot_t new_prot);

                                        
vm_offset_t ml_vtophys(
	vm_offset_t vaddr);

                              
void ml_cpu_get_info(ml_cpu_info_t *ml_cpu_info);

#endif                           

#ifdef __APPLE_API_PRIVATE
#ifdef  XNU_KERNEL_PRIVATE
vm_size_t ml_nofault_copy(
	vm_offset_t virtsrc,
	vm_offset_t virtdst,
	vm_size_t size);
boolean_t ml_validate_nofault(
	vm_offset_t virtsrc, vm_size_t size);
#endif                         
#if     defined(PEXPERT_KERNEL_PRIVATE) || defined(MACH_KERNEL_PRIVATE)
                            

                             
vm_offset_t ml_io_map(
	vm_offset_t phys_addr,
	vm_size_t size);

vm_offset_t ml_io_map_wcomb(
	vm_offset_t phys_addr,
	vm_size_t size);

vm_offset_t ml_io_map_with_prot(
	vm_offset_t phys_addr,
	vm_size_t size,
	vm_prot_t prot);

void ml_io_unmap(
	vm_offset_t addr,
	vm_size_t sz);

void ml_get_bouncepool_info(
	vm_offset_t *phys_addr,
	vm_size_t   *size);

vm_map_address_t ml_map_high_window(
	vm_offset_t     phys_addr,
	vm_size_t       len);

                            
vm_offset_t ml_static_malloc(
	vm_size_t size);

void ml_init_timebase(
	void            *args,
	tbd_ops_t       tbd_funcs,
	vm_offset_t     int_address,
	vm_offset_t     int_value);

uint64_t ml_get_timebase(void);

uint64_t ml_get_speculative_timebase(void);

uint64_t ml_get_timebase_entropy(void);

void ml_init_lock_timeout(void);

#if __arm64__
uint64_t virtual_timeout_inflate_ns(unsigned int vti, uint64_t timeout);
uint64_t virtual_timeout_inflate_abs(unsigned int vti, uint64_t timeout);
#endif

boolean_t ml_delay_should_spin(uint64_t interval);

void ml_delay_on_yield(void);

uint32_t ml_get_decrementer(void);

#include <machine/config.h>

#if !CONFIG_SKIP_PRECISE_USER_KERNEL_TIME || HAS_FAST_CNTVCT
void timer_state_event_user_to_kernel(void);
void timer_state_event_kernel_to_user(void);
#endif                                                               

uint64_t ml_get_hwclock(void);

#ifdef __arm64__
boolean_t ml_get_timer_pending(void);
#endif

void platform_syscall(
	struct arm_saved_state *);

void ml_set_decrementer(
	uint32_t dec_value);

boolean_t is_user_contex(
	void);

void ml_init_arm_debug_interface(void *args, vm_offset_t virt_address);

                                                                   
uintptr_t arm_user_protect_begin(
	thread_t thread);

void arm_user_protect_end(
	thread_t thread,
	uintptr_t up,
	boolean_t disable_interrupts);

#endif                                                     

                                               
void bzero_phys(
	addr64_t phys_address,
	vm_size_t length);

void bzero_phys_nc(addr64_t src64, vm_size_t bytes);

#if MACH_KERNEL_PRIVATE
#ifdef __arm64__
                                                      
                                                              
                                                          
   
void fill32_dczva(addr64_t, vm_size_t);
void fill32_nt(addr64_t, vm_size_t, uint32_t);
int cpu_interrupt_is_pending(void);
#endif
#endif

void ml_thread_policy(
	thread_t thread,
	unsigned policy_id,
	unsigned policy_info);

#define MACHINE_GROUP                                   0x00000001
#define MACHINE_NETWORK_GROUP                   0x10000000
#define MACHINE_NETWORK_WORKLOOP                0x00000001
#define MACHINE_NETWORK_NETISR                  0x00000002

                                    
void ml_set_max_cpus(
	unsigned int max_cpus);

                                                                                      
unsigned int ml_wait_max_cpus(
	void);

                                    
unsigned int ml_get_machine_mem(void);

#ifdef XNU_KERNEL_PRIVATE
                       
vm_map_offset_t ml_get_max_offset(
	boolean_t       is64,
	unsigned int option);
#define MACHINE_MAX_OFFSET_DEFAULT      0x01
#define MACHINE_MAX_OFFSET_MIN          0x02
#define MACHINE_MAX_OFFSET_MAX          0x04
#define MACHINE_MAX_OFFSET_DEVICE       0x08
#endif

extern void     ml_cpu_init_completed(void);
extern void     ml_cpu_up(void);
extern void     ml_cpu_down(void);
extern void     ml_arm_sleep(void);

extern uint64_t ml_get_wake_timebase(void);
extern uint64_t ml_get_conttime_wake_time(void);

                                                            
uint64_t ml_get_time_since_reset(void);

  
                                                                       
                                                                       
                                                                  
                                                                         
   
void ml_set_reset_time(uint64_t wake_time);

#ifdef XNU_KERNEL_PRIVATE
                        
extern kern_return_t ml_interrupt_prewarm(uint64_t deadline);
#define TCOAL_DEBUG(x, a, b, c, d, e) do { } while(0)
#endif                         

                                      
vm_offset_t ml_stack_remaining(void);

#ifdef MACH_KERNEL_PRIVATE
uint32_t        get_fpscr(void);
void            set_fpscr(uint32_t);
void            machine_conf(void);
void            machine_lockdown(void);

#ifdef __arm64__
unsigned long update_mdscr(unsigned long clear, unsigned long set);
#endif                

extern  void            arm_debug_set_cp14(arm_debug_state_t *debug_state);
extern  void            fiq_context_init(boolean_t enable_fiq);

extern  void            reenable_async_aborts(void);
#ifdef __arm__
extern  boolean_t       get_vfp_enabled(void);
extern  void            cpu_idle_wfi(boolean_t wfi_fast);
#endif

#ifdef __arm64__
uint64_t ml_cluster_wfe_timeout(uint32_t wfe_cluster_id);
#endif

#ifdef MONITOR
#define MONITOR_SET_ENTRY       0x800                                            
#define MONITOR_LOCKDOWN        0x801                                             
unsigned long           monitor_call(uintptr_t callnum, uintptr_t arg1,
    uintptr_t arg2, uintptr_t arg3);
#endif              

#if __ARM_KERNEL_PROTECT__
extern void set_vbar_el1(uint64_t);
#endif                             
#endif                          

extern  uint32_t        arm_debug_read_dscr(void);

extern int      set_be_bit(void);
extern int      clr_be_bit(void);
extern int      be_tracing(void);

                                                                                    
                                                                                   
                                                                                
                                                                                  
                                                                                  
                                                                                      
                    
   
typedef void (*broadcastFunc) (void *);
unsigned int cpu_broadcast_xcall(uint32_t *, boolean_t, broadcastFunc, void *);
unsigned int cpu_broadcast_xcall_simple(boolean_t, broadcastFunc, void *);
kern_return_t cpu_xcall(int, broadcastFunc, void *);
unsigned int cpu_broadcast_immediate_xcall(uint32_t *, boolean_t, broadcastFunc, void *);
unsigned int cpu_broadcast_immediate_xcall_simple(boolean_t, broadcastFunc, void *);
kern_return_t cpu_immediate_xcall(int, broadcastFunc, void *);

#ifdef  KERNEL_PRIVATE

                                                                            
                                                                      
                                                                       
   

#ifdef __arm64__
typedef void (*cpu_qos_update_t)(int throughput_qos, uint64_t qos_param1, uint64_t qos_param2);
void cpu_qos_update_register(cpu_qos_update_t);
#endif                

struct going_on_core {
	uint64_t        thread_id;
	uint16_t        qos_class;
	uint16_t        urgency;                                
	uint32_t        is_32_bit : 1;                                                                                             
	uint32_t        is_kernel_thread : 1;
	uint64_t        thread_group_id;
	void            *thread_group_data;
	uint64_t        scheduling_latency;                                                                                  
	uint64_t        start_time;
	uint64_t        scheduling_latency_at_same_basepri;
	uint32_t        energy_estimate_nj;                                
	                                                                                                    
};
typedef struct going_on_core *going_on_core_t;

struct going_off_core {
	uint64_t        thread_id;
	uint32_t        energy_estimate_nj;                                
	uint32_t        reserved;
	uint64_t        end_time;
	uint64_t        thread_group_id;
	void            *thread_group_data;
};
typedef struct going_off_core *going_off_core_t;

struct thread_group_data {
	uint64_t        thread_group_id;
	void            *thread_group_data;
	uint32_t        thread_group_size;
	uint32_t        thread_group_flags;
};
typedef struct thread_group_data *thread_group_data_t;

struct perfcontrol_max_runnable_latency {
	uint64_t        max_scheduling_latencies[4                         ];
};
typedef struct perfcontrol_max_runnable_latency *perfcontrol_max_runnable_latency_t;

struct perfcontrol_work_interval {
	uint64_t        thread_id;
	uint16_t        qos_class;
	uint16_t        urgency;
	uint32_t        flags;          
	uint64_t        work_interval_id;
	uint64_t        start;
	uint64_t        finish;
	uint64_t        deadline;
	uint64_t        next_start;
	uint64_t        thread_group_id;
	void            *thread_group_data;
	uint32_t        create_flags;
};
typedef struct perfcontrol_work_interval *perfcontrol_work_interval_t;

typedef enum {
	WORK_INTERVAL_START,
	WORK_INTERVAL_UPDATE,
	WORK_INTERVAL_FINISH
} work_interval_ctl_t;

struct perfcontrol_work_interval_instance {
	work_interval_ctl_t     ctl;
	uint32_t                create_flags;
	uint64_t                complexity;
	uint64_t                thread_id;
	uint64_t                work_interval_id;
	uint64_t                instance_id;                                    
	uint64_t                start;
	uint64_t                finish;
	uint64_t                deadline;
	uint64_t                thread_group_id;
	void                    *thread_group_data;
};
typedef struct perfcontrol_work_interval_instance *perfcontrol_work_interval_instance_t;

  
                                                                    
                                                                       
                                                                       
   
struct perfcontrol_cpu_counters {
	uint64_t        instructions;
	uint64_t        cycles;
};

  
                                                            
   
struct perfcontrol_thread_data {
	  
                                  
                                                         
                                 
    
	uint32_t            energy_estimate_nj;
	                                  
	perfcontrol_class_t perfctl_class;
	                              
	uint64_t            thread_id;
	                     
	uint64_t            thread_group_id;
	  
                                                             
                                                                  
                                                              
                  
    
	uint64_t            scheduling_latency_at_same_basepri;
	                               
	void                *thread_group_data;
	                           
	void                *perfctl_state;
};

  
                                                               
                                                             
                                                              
                                                              
                                                                   
                                                                      
                                                                      
                                                                      
                                                         
   

  
                                                               
                                                                   
                                                                    
                                                                    
                                
  
                                                   
   
typedef void (*sched_perfcontrol_context_switch_t)(perfcontrol_state_t, perfcontrol_state_t);

  
                                                                 
                                                               
                                                                 
                                                                   
                                                                    
                                                                     
   
typedef void (*sched_perfcontrol_offcore_t)(perfcontrol_state_t, going_off_core_t                          , boolean_t);

  
                                                                     
                                                                 
                                                                      
                                                                    
                                                            
                                                                      
                    
   
typedef void (*sched_perfcontrol_oncore_t)(perfcontrol_state_t, going_on_core_t);

  
                                                                     
                                                                             
                                           
   
typedef void (*sched_perfcontrol_max_runnable_latency_t)(perfcontrol_max_runnable_latency_t);

  
                                                                           
                                                                                
                                              
   
typedef void (*sched_perfcontrol_work_interval_notify_t)(perfcontrol_state_t, perfcontrol_work_interval_t);

  
                                                                                     
   
typedef void (*sched_perfcontrol_work_interval_ctl_t)(perfcontrol_state_t, perfcontrol_work_interval_instance_t);

  
                                                                               
           
                                                                               
                                                                                
                                    
   
typedef void (*sched_perfcontrol_thread_group_init_t)(thread_group_data_t);
typedef void (*sched_perfcontrol_thread_group_deinit_t)(thread_group_data_t);
typedef void (*sched_perfcontrol_thread_group_flags_update_t)(thread_group_data_t);

  
                                                                                           
                                                                                                       
  
                                                                                                                    
   
typedef void (*sched_perfcontrol_deadline_passed_t)(uint64_t deadline);

  
                         
  
              
                                                        
                                                  
                                                      
                                                      
                                                                      
                                                                     
                                                                       
   
typedef void (*sched_perfcontrol_csw_t)(
	perfcontrol_event event, uint32_t cpu_id, uint64_t timestamp, uint32_t flags,
	struct perfcontrol_thread_data *offcore, struct perfcontrol_thread_data *oncore,
	struct perfcontrol_cpu_counters *cpu_counters, __unused void *unused);


  
                              
  
              
                                                        
                                                
                                                    
                                                      
                                                                         
   
typedef void (*sched_perfcontrol_state_update_t)(
	perfcontrol_event event, uint32_t cpu_id, uint64_t timestamp, uint32_t flags,
	struct perfcontrol_thread_data *thr_data, __unused void *unused);

  
                                             
  
              
                                                                                   
                                                                                
                                                              
                                                                         
   
typedef void (*sched_perfcontrol_thread_group_blocked_t)(
	thread_group_data_t blocked_tg, thread_group_data_t blocking_tg, uint32_t flags, perfcontrol_state_t blocked_thr_state);

  
                                  
  
              
                                                                                   
                                                                                  
                                                              
                                                                           
   
typedef void (*sched_perfcontrol_thread_group_unblocked_t)(
	thread_group_data_t unblocked_tg, thread_group_data_t unblocking_tg, uint32_t flags, perfcontrol_state_t unblocked_thr_state);

  
                                                                                         
                                                                                      
                                                                                    
                                                                                        
                                                        
   

#define SCHED_PERFCONTROL_CALLBACKS_VERSION_0 (0)                   
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_1 (1)                                 
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_2 (2)                                 
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_3 (3)                                
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_4 (4)                            
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_5 (5)                         
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_6 (6)                                      
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_7 (7)                              
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_8 (8)                                   
#define SCHED_PERFCONTROL_CALLBACKS_VERSION_CURRENT SCHED_PERFCONTROL_CALLBACKS_VERSION_6

struct sched_perfcontrol_callbacks {
	unsigned long version;                                                      
	sched_perfcontrol_offcore_t                   offcore;
	sched_perfcontrol_context_switch_t            context_switch;
	sched_perfcontrol_oncore_t                    oncore;
	sched_perfcontrol_max_runnable_latency_t      max_runnable_latency;
	sched_perfcontrol_work_interval_notify_t      work_interval_notify;
	sched_perfcontrol_thread_group_init_t         thread_group_init;
	sched_perfcontrol_thread_group_deinit_t       thread_group_deinit;
	sched_perfcontrol_deadline_passed_t           deadline_passed;
	sched_perfcontrol_csw_t                       csw;
	sched_perfcontrol_state_update_t              state_update;
	sched_perfcontrol_thread_group_flags_update_t thread_group_flags_update;
	sched_perfcontrol_work_interval_ctl_t         work_interval_ctl;
	sched_perfcontrol_thread_group_blocked_t      thread_group_blocked;
	sched_perfcontrol_thread_group_unblocked_t    thread_group_unblocked;
};
typedef struct sched_perfcontrol_callbacks *sched_perfcontrol_callbacks_t;

extern void sched_perfcontrol_register_callbacks(sched_perfcontrol_callbacks_t callbacks, unsigned long size_of_state);

  
                                                                                          
                                                                                    
                                                                             
   
#define ALL_CORES_RECOMMENDED   (~(uint32_t)0)

extern void sched_perfcontrol_update_recommended_cores(uint32_t recommended_cores);
extern void sched_perfcontrol_thread_group_recommend(void *data, cluster_type_t recommendation);
extern void sched_override_recommended_cores_for_sleep(void);
extern void sched_restore_recommended_cores_after_sleep(void);
extern void sched_perfcontrol_inherit_recommendation_from_tg(perfcontrol_class_t perfctl_class, boolean_t inherit);

extern void sched_usercontrol_update_recommended_cores(uint64_t recommended_cores);

  
                                
  
                                                          
  
                                                                                         
                                                                                           
                                                                                                  
                                                                                            
                                                               
                                       
  
   
                                                                   
#define SCHED_PERFCONTROL_PREFERRED_CLUSTER_OVERRIDE_NONE         ((uint32_t)~0)

  
                                                                                              
                                                                                           
   
#define SCHED_PERFCONTROL_PREFERRED_CLUSTER_MIGRATE_RUNNING       0x1
#define SCHED_PERFCONTROL_PREFERRED_CLUSTER_MIGRATE_RUNNABLE      0x2
typedef uint64_t sched_perfcontrol_preferred_cluster_options_t;

extern void sched_perfcontrol_thread_group_preferred_clusters_set(void *machine_data, uint32_t tg_preferred_cluster,
    uint32_t overrides[PERFCONTROL_CLASS_MAX], sched_perfcontrol_preferred_cluster_options_t options);

  
                                
  
                                                                          
  
                                                                                               
                                                                                                
                                                                                               
                                                                                   
   

extern void sched_perfcontrol_edge_matrix_get(sched_clutch_edge *edge_matrix, bool *edge_request_bitmap, uint64_t flags, uint64_t matrix_order);
extern void sched_perfcontrol_edge_matrix_set(sched_clutch_edge *edge_matrix, bool *edge_changes_bitmap, uint64_t flags, uint64_t matrix_order);

  
                                                                                    
                                                                      
                                                                                           
                                                                                            
  
                                                                                      
  
                                                    
   
extern boolean_t sched_perfcontrol_update_callback_deadline(uint64_t deadline);

typedef enum perfcontrol_callout_type {
	PERFCONTROL_CALLOUT_ON_CORE,
	PERFCONTROL_CALLOUT_OFF_CORE,
	PERFCONTROL_CALLOUT_CONTEXT,
	PERFCONTROL_CALLOUT_STATE_UPDATE,
	                                  
	PERFCONTROL_CALLOUT_MAX
} perfcontrol_callout_type_t;

typedef enum perfcontrol_callout_stat {
	PERFCONTROL_STAT_INSTRS,
	PERFCONTROL_STAT_CYCLES,
	                               
	PERFCONTROL_STAT_MAX
} perfcontrol_callout_stat_t;

uint64_t perfcontrol_callout_stat_avg(perfcontrol_callout_type_t type,
    perfcontrol_callout_stat_t stat);

#ifdef __arm64__
                                                                 
                                                                 
                                                                
                                                                    
                                                                          
   
uint32_t ml_update_cluster_wfe_recommendation(uint32_t wfe_cluster_id, uint64_t wfe_timeout_abstime_interval, uint64_t wfe_hint_flags);
#endif                

#if defined(HAS_APPLE_PAC)
#define ONES(x) (BIT((x))-1)
#define PTR_MASK ONES(64-T1SZ_BOOT)
#define PAC_MASK ~PTR_MASK
#define SIGN(p) ((p) & BIT(55))
#define UNSIGN_PTR(p) \
	SIGN(p) ? ((p) | PAC_MASK) : ((p) & ~PAC_MASK)

uint64_t ml_default_rop_pid(void);
uint64_t ml_default_jop_pid(void);
void ml_task_set_rop_pid(task_t task, task_t parent_task, boolean_t inherit);
void ml_task_set_jop_pid(task_t task, task_t parent_task, boolean_t inherit);
void ml_task_set_jop_pid_from_shared_region(task_t task);
void ml_task_set_disable_user_jop(task_t task, uint8_t disable_user_jop);
void ml_thread_set_disable_user_jop(thread_t thread, uint8_t disable_user_jop);
void ml_thread_set_jop_pid(thread_t thread, task_t task);
void *ml_auth_ptr_unchecked(void *ptr, unsigned key, uint64_t modifier);

uint64_t ml_enable_user_jop_key(uint64_t user_jop_key);

   
                                                                                
        
  
                                                                  
                              
                                                               
                              
   
void ml_disable_user_jop_key(uint64_t user_jop_key, uint64_t saved_jop_state);
#endif                             

void ml_enable_monitor(void);


#endif                     

boolean_t machine_timeout_suspended(void);
void ml_get_power_state(boolean_t *, boolean_t *);

uint32_t get_arm_cpu_version(void);
boolean_t user_cont_hwclock_allowed(void);
uint8_t user_timebase_type(void);
boolean_t ml_thread_is64bit(thread_t thread);

#ifdef __arm64__
bool ml_feature_supported(uint32_t feature_bit);
void ml_set_align_checking(void);
extern void wfe_timeout_configure(void);
extern void wfe_timeout_init(void);
#endif                

void ml_timer_evaluate(void);
boolean_t ml_timer_forced_evaluation(void);
uint64_t ml_energy_stat(thread_t);
void ml_gpu_stat_update(uint64_t);
uint64_t ml_gpu_stat(thread_t);
#endif                          



#if __arm64__ && defined(CONFIG_XNUPOST) && defined(XNU_KERNEL_PRIVATE)
extern void ml_expect_fault_begin(expected_fault_handler_t, uintptr_t);
extern void ml_expect_fault_end(void);
#endif                                                                          


void ml_hibernate_active_pre(void);
void ml_hibernate_active_post(void);

__END_DECLS

#endif                               
