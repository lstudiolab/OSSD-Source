/*
 * Copyright (c) 2005-2006 Apple Computer, Inc. All rights reserved.
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

#ifndef _DTRACE_GLUE_H
#define _DTRACE_GLUE_H

#ifdef KERNEL_BUILD

#include <libkern/libkern.h>
#include <kern/cpu_number.h>
#include <kern/locks.h>
#include <kern/debug.h>
#include <kern/thread_call.h>
#include <kern/thread.h>
#include <machine/atomic.h>
#include <machine/machine_routines.h>
#include <sys/syslog.h>
#include <sys/ucred.h>
#include <stdarg.h>
#include <mach/kmod.h>

#if defined(__i386__) || defined(__x86_64__)
#include <i386/mp.h>
#endif

  
          
   
#define CE_NOTE         1                    
#define CE_WARN         2                     

extern void cmn_err( int, const char *, ... );

  
           
   

                                                                       
#define proc_t struct proc                                                   
#define curproc ((struct proc *)current_proc())                                                

proc_t* sprlock(pid_t pid);
void sprunlock(proc_t *p);

void dtrace_sprlock(proc_t *p);
void dtrace_sprunlock(proc_t *p);

  
               
   

int uread(proc_t *p, void *buf, user_size_t len, user_addr_t a);
int uwrite(proc_t *p, void *buf, user_size_t len, user_addr_t a);

  
                  
   

int fuword8(user_addr_t, uint8_t *);
int fuword16(user_addr_t, uint16_t *);
int fuword32(user_addr_t, uint32_t *);
int fuword64(user_addr_t, uint64_t *);

void fuword32_noerr(user_addr_t, uint32_t *);
void fuword64_noerr(user_addr_t, uint64_t *);

int suword64(user_addr_t, uint64_t value);
int suword32(user_addr_t, uint32_t value);

  
         
   
extern lck_mtx_t cpu_lock;
extern lck_mtx_t cyc_lock;
extern lck_mtx_t mod_lock;

  
                                                            
   
struct wrap_timer_call;

  
                
   
typedef struct dtrace_cpu {
	processorid_t   cpu_id;                                    
	struct dtrace_cpu *cpu_next;                                        
	lck_rw_t        cpu_ft_lock;                                          
	uintptr_t       cpu_dtrace_caller;                                     
	hrtime_t        cpu_dtrace_chillmark;                                   
	hrtime_t        cpu_dtrace_chilled;                                      
	boolean_t       cpu_dtrace_invop_underway;                                                   

	                                     
	LIST_HEAD(cyc_list_head, wrap_timer_call) cpu_cyc_list;
} dtrace_cpu_t;

extern dtrace_cpu_t *cpu_list;

  
                                                                             
                                                                       
                                                                          
                                                                              
                                                 
   
#define CPU_CACHE_COHERENCE_SIZE        64

typedef struct cpu_core {
	uint64_t        cpuc_dtrace_illval;                               
	lck_mtx_t       cpuc_pid_lock;                                        
	uint16_t        cpuc_dtrace_flags;                        
	uint64_t        cpuc_missing_tos;                                                     
	uint8_t         cpuc_pad[CPU_CACHE_COHERENCE_SIZE - sizeof(uint64_t) - sizeof(lck_mtx_t) - sizeof(uint16_t) - sizeof(uint64_t)];                     
} cpu_core_t;

extern cpu_core_t *cpu_core;

extern unsigned int dtrace_max_cpus;                                            
#define NCPU        dtrace_max_cpus

#define CPU             (&(cpu_list[cpu_number()]))                                 
#define CPU_ON_INTR(cpup) ml_at_interrupt_context()                                    

  
                                                                        
                   
   
typedef enum {
	CPU_INIT,
	CPU_CONFIG,
	CPU_UNCONFIG,
	CPU_ON,
	CPU_OFF,
	CPU_CPUPART_IN,
	CPU_CPUPART_OUT
} cpu_setup_t;

typedef int cpu_setup_func_t(cpu_setup_t, int, void *);

extern void register_cpu_setup_func(cpu_setup_func_t *, void *);
extern void unregister_cpu_setup_func(cpu_setup_func_t *, void *);

  
             
   

  
                
   
#define CPU_DTRACE_NOFAULT      0x0001                   
#define CPU_DTRACE_DROP         0x0002                     
#define CPU_DTRACE_BADADDR      0x0004                                 
#define CPU_DTRACE_BADALIGN     0x0008                                   
#define CPU_DTRACE_DIVZERO      0x0010                                    
#define CPU_DTRACE_ILLOP        0x0020                                       
#define CPU_DTRACE_NOSCRATCH    0x0040                                    
#define CPU_DTRACE_KPRIV        0x0080                                       
#define CPU_DTRACE_UPRIV        0x0100                                     
#define CPU_DTRACE_TUPOFLOW     0x0200                                          
#define CPU_DTRACE_USTACK_FP    0x0400                                     
#define CPU_DTRACE_ENTRY        0x0800                                     
#define CPU_DTRACE_BADSTACK 0x1000                               

#define CPU_DTRACE_FAULT        (CPU_DTRACE_BADADDR | CPU_DTRACE_BADALIGN | \
	                        CPU_DTRACE_DIVZERO | CPU_DTRACE_ILLOP | \
	                        CPU_DTRACE_NOSCRATCH | CPU_DTRACE_KPRIV | \
	                        CPU_DTRACE_UPRIV | CPU_DTRACE_TUPOFLOW | \
	                        CPU_DTRACE_BADSTACK)
#define CPU_DTRACE_ERROR        (CPU_DTRACE_FAULT | CPU_DTRACE_DROP)

  
                   
   

                             
struct dtrace_module_symbols;

                                                                                           
typedef struct modctl {
	struct modctl   *mod_next;
	struct modctl   *mod_stale;                          
	uint32_t        mod_id;                                      
	char            mod_modname[KMOD_MAX_NAME];
	int             mod_loadcnt;
	char            mod_loaded;
	uint16_t        mod_flags;                        
	int             mod_nenabled;                                          
	vm_address_t    mod_address;                                               
	vm_size_t       mod_size;                              
	UUID            mod_uuid;
	struct dtrace_module_symbols* mod_user_symbols;
} modctl_t;

                               
#define MODCTL_IS_MACH_KERNEL                   0x01                                        
#define MODCTL_HAS_KERNEL_SYMBOLS               0x02                                         
#define MODCTL_FBT_PROBES_PROVIDED              0x04                                  
#define MODCTL_FBT_INVALID                      0x08                                     
#define MODCTL_SDT_PROBES_PROVIDED              0x10                                  
#define MODCTL_SDT_INVALID                      0x20                                     
#define MODCTL_HAS_UUID                         0x40                    
#define MODCTL_FBT_PROVIDE_BLACKLISTED_PROBES   0x200                                                
#define MODCTL_FBT_BLACKLISTED_PROBES_PROVIDED  0x400                                             
#define MODCTL_IS_STATIC_KEXT                   0x800                           

                                         
#define MOD_IS_MACH_KERNEL(mod)                 (mod->mod_flags & MODCTL_IS_MACH_KERNEL)
#define MOD_HAS_KERNEL_SYMBOLS(mod)             (mod->mod_flags & MODCTL_HAS_KERNEL_SYMBOLS)
#define MOD_HAS_USERSPACE_SYMBOLS(mod)          (mod->mod_user_symbols)                                                      
#define MOD_FBT_PROBES_PROVIDED(mod)            (mod->mod_flags & MODCTL_FBT_PROBES_PROVIDED)
#define MOD_FBT_INVALID(mod)                    (mod->mod_flags & MODCTL_FBT_INVALID)
#define MOD_SDT_PROBES_PROVIDED(mod)            (mod->mod_flags & MODCTL_SDT_PROBES_PROVIDED)
#define MOD_SDT_INVALID(mod)                    (mod->mod_flags & MODCTL_SDT_INVALID)
#define MOD_HAS_UUID(mod)                       (mod->mod_flags & MODCTL_HAS_UUID)
#define MOD_FBT_BLACKLISTED_PROBES_PROVIDED(mod) (mod->mod_flags & MODCTL_FBT_BLACKLISTED_PROBES_PROVIDED)
#define MOD_FBT_PROVIDE_BLACKLISTED_PROBES(mod) (mod->mod_flags & MODCTL_FBT_PROVIDE_BLACKLISTED_PROBES)
#define MOD_IS_STATIC_KEXT(mod)                 (mod->mod_flags & MODCTL_IS_STATIC_KEXT)

                        
#define MOD_FBT_BLACKLISTED_PROBES_DONE(mod)    (MOD_FBT_BLACKLISTED_PROBES_PROVIDED(mod) || !MOD_FBT_PROVIDE_BLACKLISTED_PROBES(mod))
#define MOD_FBT_DONE(mod)                       ((MOD_FBT_PROBES_PROVIDED(mod) && MOD_FBT_BLACKLISTED_PROBES_DONE(mod)) || MOD_FBT_INVALID(mod))
#define MOD_SDT_DONE(mod)                       (MOD_SDT_PROBES_PROVIDED(mod) || MOD_SDT_INVALID(mod))
#define MOD_SYMBOLS_DONE(mod)                   (MOD_FBT_DONE(mod) && MOD_SDT_DONE(mod))

extern modctl_t *dtrace_modctl_list;

extern int dtrace_addr_in_module(void*, struct modctl*);

  
         
   
                
#define PRIV_DTRACE_KERNEL        3
#define PRIV_DTRACE_PROC          4
#define PRIV_DTRACE_USER          5
#define PRIV_PROC_OWNER          30
#define PRIV_PROC_ZONE           35
#define PRIV_ALL                        (-1)                                 

                    
#define PRIV_EFFECTIVE            0

typedef struct ucred cred_t;
#define cr_suid cr_svuid
#define cr_sgid cr_svgid

extern cred_t *dtrace_CRED(void);                                       
#define CRED() kauth_cred_get()                                          
extern int PRIV_POLICY_CHOICE(void *, int, int);
extern int PRIV_POLICY_ONLY(void *, int, int);
extern uid_t crgetuid(const cred_t *);
#define crgetzoneid(x) ((zoneid_t)0)

  
           
   
#define CY_LOW_LEVEL            0
#define CY_HIGH_LEVEL           2
#define CY_LEVELS                       3

typedef uintptr_t cyclic_id_t;
typedef cyclic_id_t *cyclic_id_list_t;
typedef uint16_t cyc_level_t;
typedef void (*cyc_func_t)(void *);

#define CYCLIC_NONE             ((cyclic_id_t)0)

typedef struct cyc_time {
	hrtime_t cyt_when;
	hrtime_t cyt_interval;
} cyc_time_t;

typedef struct cyc_handler {
	cyc_func_t cyh_func;
	void *cyh_arg;
	cyc_level_t cyh_level;
} cyc_handler_t;

typedef struct cyc_omni_handler {
	void (*cyo_online)(void *, dtrace_cpu_t *, cyc_handler_t *, cyc_time_t *);
	void (*cyo_offline)(void *, dtrace_cpu_t *, void *);
	void *cyo_arg;
} cyc_omni_handler_t;

extern void dtrace_install_cpu_hooks(void);

extern cyclic_id_t cyclic_add(cyc_handler_t *, cyc_time_t *);
extern void cyclic_remove(cyclic_id_t);

extern cyclic_id_list_t cyclic_add_omni(cyc_omni_handler_t *);
extern void cyclic_remove_omni(cyclic_id_list_t);

extern cyclic_id_t cyclic_timer_add(cyc_handler_t *, cyc_time_t *);
extern void cyclic_timer_remove(cyclic_id_t);

  
      
   

#define DDI_SUCCESS                     0
#define DDI_FAILURE                     -1

#define DDI_PSEUDO "ddi_pseudo"

typedef enum {
	DDI_DETACH = 0,
	DDI_SUSPEND = 1,
	DDI_PM_SUSPEND = 2,
	DDI_HOTPLUG_DETACH = 3                                                  
} ddi_detach_cmd_t;

#define DDI_PROP_SUCCESS        0

#define DDI_PROP_DONTPASS   1
typedef uint_t major_t;
typedef uint_t minor_t;

typedef struct __dev_info *dev_info_t;

extern int ddi_driver_major(dev_info_t *);

extern int ddi_create_minor_node(dev_info_t *, const char *, int, minor_t, const char *, int);
extern void ddi_remove_minor_node(dev_info_t *, char *);

extern major_t getemajor(dev_t);
extern minor_t getminor(dev_t);

  
                         
   
extern void debug_enter(char *);

  
                                  
   

  
       
   

#define KM_SLEEP        0x00000000
#define KM_NOSLEEP      0x00000001

typedef struct vmem vmem_t;
typedef struct kmem_cache kmem_cache_t;

#define kmem_free dt_kmem_free                                          
#define kmem_free_aligned dt_kmem_free_aligned

#define kmem_alloc(size, kmflag) \
	({ VM_ALLOC_SITE_STATIC(0, 0); \
	dt_kmem_alloc_site(size, kmflag, &site); })

extern void *dt_kmem_alloc_site(size_t, int, vm_allocation_site_t*);
extern void dt_kmem_free(void *, size_t);

#define kmem_zalloc(size, kmflag) \
	({ VM_ALLOC_SITE_STATIC(0, 0); \
	dt_kmem_zalloc_site(size, kmflag, &site); })

extern void *dt_kmem_zalloc_site(size_t, int, vm_allocation_site_t*);

#define kmem_alloc_aligned(size, align, kmflag) \
	({ VM_ALLOC_SITE_STATIC(0, 0); \
	dt_kmem_alloc_aligned_site(size, align, kmflag, &site); })
extern void *dt_kmem_alloc_aligned_site(size_t, size_t, int, vm_allocation_site_t*);

#define kmem_zalloc_aligned(size, align, kmflag) \
	({ VM_ALLOC_SITE_STATIC(0, 0); \
	dt_kmem_zalloc_aligned_site(size, align, kmflag, &site); })
extern void *dt_kmem_zalloc_aligned_site(size_t, size_t, int, vm_allocation_site_t*);

extern void dt_kmem_free_aligned(void*, size_t);

extern kmem_cache_t *
    kmem_cache_create(const char *, size_t, size_t, int (*)(void *, void *, int),
    void (*)(void *, void *), void (*)(void *), void *, vmem_t *, int);
extern void *kmem_cache_alloc(kmem_cache_t *, int);
extern void kmem_cache_free(kmem_cache_t *, void *);
extern void kmem_cache_destroy(kmem_cache_t *);

  
          
   

typedef struct _kthread kthread_t;                                                                      

  
       
   


#define DATAMODEL_ILP32 0x00100000
#define DATAMODEL_LP64  0x00200000

#define DATAMODEL_NONE  0

#if     defined(__LP64__)
#define DATAMODEL_NATIVE        DATAMODEL_LP64
#else
#define DATAMODEL_NATIVE        DATAMODEL_ILP32
#endif                

typedef unsigned int model_t;                                                              

  
       
   

#define VMC_IDENTIFIER  0x00040000                                
#define VM_SLEEP        0x00000000                            
#define VM_BESTFIT      0x00000100

extern void *vmem_alloc(vmem_t *, size_t, int);
extern vmem_t *vmem_create(const char *, void *, size_t, size_t, void *,
    void *, vmem_t *, size_t, int);
extern void vmem_destroy(vmem_t *);
extern void vmem_free(vmem_t *vmp, void *vaddr, size_t size);

  
                
   

typedef uintptr_t pc_t;
typedef uintptr_t greg_t;                                                    
#if defined(__arm__) || defined(__arm64__)
#define regs arm_saved_state
#endif
extern struct regs *find_user_regs( thread_t thread);
extern vm_offset_t dtrace_get_cpu_int_stack_top(void);

#define panic_quiesce (panic_active())

#define IS_P2ALIGNED(v, a) ((((uintptr_t)(v)) & ((uintptr_t)(a) - 1)) == 0)

extern int vuprintf(const char *, va_list);

extern hrtime_t dtrace_abs_to_nano(uint64_t);

const void* bsearch(const void*, const void*, size_t, size_t, int (*compar)(const void *, const void *));

int dtrace_copy_maxsize(void);
int dtrace_buffer_copyout(const void*, user_addr_t, vm_size_t);


#define DTRACE_NCLIENTS 32

#undef proc_t

  
                                                                                   
                                                                                 
                                     
   
#define LIT_STRNEQL(s1, lit_s2) (0 == strncmp( (s1), (lit_s2), sizeof((lit_s2)) ))

#define KERNELBASE VM_MIN_KERNEL_ADDRESS
#endif                   
#endif                     
