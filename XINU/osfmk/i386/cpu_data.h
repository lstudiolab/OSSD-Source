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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 *
 */

#ifndef I386_CPU_DATA
#define I386_CPU_DATA

#include <mach_assert.h>
#include <machine/atomic.h>

#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/mpqueue.h>
#include <kern/queue.h>
#include <kern/processor.h>
#include <kern/pms.h>
#include <pexpert/pexpert.h>
#include <mach/i386/thread_status.h>
#include <mach/i386/vm_param.h>
#include <i386/locks.h>
#include <i386/rtclock_protos.h>
#include <i386/pmCPU.h>
#include <i386/cpu_topology.h>
#include <i386/seg.h>
#include <i386/mp.h>

#if CONFIG_VMX
#include <i386/vmx/vmx_cpu.h>
#endif

#if MONOTONIC
#include <machine/monotonic.h>
#endif                

#include <machine/pal_routines.h>

  
                                                              
   
struct cpu_cons_buffer;
struct cpu_desc_table;
struct mca_state;
struct prngContext;

  
                                            
   
typedef struct rtclock_timer {
	mpqueue_head_t          queue;
	uint64_t                deadline;
	uint64_t                when_set;
	boolean_t               has_expired;
} rtclock_timer_t;

typedef struct {
	                                                                          
	struct x86_64_tss       *cdi_ktssu;
	struct x86_64_tss       *cdi_ktssb;
	x86_64_desc_register_t  cdi_gdtu;
	x86_64_desc_register_t  cdi_gdtb;
	x86_64_desc_register_t  cdi_idtu;
	x86_64_desc_register_t  cdi_idtb;
	struct real_descriptor  *cdi_ldtu;
	struct real_descriptor  *cdi_ldtb;
	vm_offset_t             cdi_sstku;
	vm_offset_t             cdi_sstkb;
} cpu_desc_index_t;

typedef enum {
	TASK_MAP_32BIT,                                                      
	TASK_MAP_64BIT,                                                       
} task_map_t;


  
                                                                         
                                                                        
                                                                      
                                        
   
typedef struct {
	addr64_t        cu_isf;                                   
	uint64_t        cu_tmp;                                
	addr64_t        cu_user_gs_base;
} cpu_uber_t;

typedef uint16_t        pcid_t;
typedef uint8_t         pcid_ref_t;

#define CPU_RTIME_BINS (12)
#define CPU_ITIME_BINS (CPU_RTIME_BINS)

#define MAX_TRACE_BTFRAMES (16)
typedef struct {
	boolean_t pltype;
	int plevel;
	uint64_t plbt[MAX_TRACE_BTFRAMES];
} plrecord_t;

#if     DEVELOPMENT || DEBUG
typedef enum {
	IOTRACE_PHYS_READ = 1,
	IOTRACE_PHYS_WRITE,
	IOTRACE_IO_READ,
	IOTRACE_IO_WRITE,
	IOTRACE_PORTIO_READ,
	IOTRACE_PORTIO_WRITE
} iotrace_type_e;

typedef struct {
	iotrace_type_e  iotype;
	int             size;
	uint64_t        vaddr;
	uint64_t        paddr;
	uint64_t        val;
	uint64_t        start_time_abs;
	uint64_t        duration;
	uint64_t        backtrace[MAX_TRACE_BTFRAMES];
} iotrace_entry_t;

typedef struct {
	int             vector;                                                 
	thread_t        curthread;                                                               
	uint64_t        interrupted_pc;
	int             curpl;                                                
	int             curil;                                               
	uint64_t        start_time_abs;
	uint64_t        duration;
	uint64_t        backtrace[MAX_TRACE_BTFRAMES];
} traptrace_entry_t;

#define DEFAULT_IOTRACE_ENTRIES_PER_CPU (64)
#define IOTRACE_MAX_ENTRIES_PER_CPU (256)
extern volatile int mmiotrace_enabled;
extern int iotrace_generators;
extern int iotrace_entries_per_cpu;
extern int *iotrace_next;
extern iotrace_entry_t **iotrace_ring;

#define TRAPTRACE_INVALID_INDEX (~0U)
#define DEFAULT_TRAPTRACE_ENTRIES_PER_CPU (16)
#define TRAPTRACE_MAX_ENTRIES_PER_CPU (256)
extern volatile int traptrace_enabled;
extern int traptrace_generators;
extern int traptrace_entries_per_cpu;
extern int *traptrace_next;
extern traptrace_entry_t **traptrace_ring;
#endif                           

  
                
  
                                                                           
                                                                           
                                                                            
                                                                            
                    
  
                                                                
                                                                             
            
   
typedef struct {
	pcid_t                  cpu_pcid_free_hint;
#define PMAP_PCID_MAX_PCID      (0x800)
	pcid_ref_t              cpu_pcid_refcounts[PMAP_PCID_MAX_PCID];
	pmap_t                  cpu_pcid_last_pmap_dispatched[PMAP_PCID_MAX_PCID];
} pcid_cdata_t;

typedef struct cpu_data {
	struct pal_cpu_data     cpu_pal_data;                                  
#define                         cpu_pd cpu_pal_data                            
	struct cpu_data         *cpu_this;                                     
	vm_offset_t             cpu_pcpu_base;
	thread_t                cpu_active_thread;
	thread_t                cpu_nthread;
	int                     cpu_number;                              
	void                    *cpu_int_state;                              
	vm_offset_t             cpu_active_stack;                              
	vm_offset_t             cpu_kernel_stack;                             
	vm_offset_t             cpu_int_stack_top;
	volatile int            cpu_signals;                            
	volatile int            cpu_prior_signals;                            
                                                             
                                                    
	ast_t                   cpu_pending_ast;
	  
                               
                                               
                                     
                                  
    
	int                     cpu_interrupt_level;
	volatile int            cpu_preemption_level;
	volatile int            cpu_running;
#if !MONOTONIC
	boolean_t               cpu_fixed_pmcs_enabled;
#endif                 
	rtclock_timer_t         rtclock_timer;
	volatile addr64_t       cpu_active_cr3 __attribute((aligned(64)));
	union {
		volatile uint32_t cpu_tlb_invalid;
		struct {
			volatile uint16_t cpu_tlb_invalid_local;
			volatile uint16_t cpu_tlb_invalid_global;
		};
	};
	uint64_t                cpu_ip_desc[2];
	volatile task_map_t     cpu_task_map;
	volatile addr64_t       cpu_task_cr3;
	addr64_t                cpu_kernel_cr3;
	volatile addr64_t       cpu_ucr3;
	volatile addr64_t       cpu_shadowtask_cr3;
	boolean_t               cpu_pagezero_mapped;
	cpu_uber_t              cpu_uber;
                                                   
	uintptr_t               cd_estack;
	int                     cpu_xstate;
	int                     cpu_curtask_has_ldt;
	int                     cpu_curthread_do_segchk;
                                                                      
                            
   
	void                    *cd_shadow;
	union {
		volatile uint32_t cpu_tlb_invalid_count;
		struct {
			volatile uint16_t cpu_tlb_invalid_local_count;
			volatile uint16_t cpu_tlb_invalid_global_count;
		};
	};

	uint16_t                cpu_tlb_gen_counts_local[MAX_CPUS];
	uint16_t                cpu_tlb_gen_counts_global[MAX_CPUS];

	struct processor        *cpu_processor;
	struct real_descriptor  *cpu_ldtp;
	struct cpu_desc_table   *cpu_desc_tablep;
	cpu_desc_index_t        cpu_desc_index;
	int                     cpu_ldt;

#define HWINTCNT_SIZE 256
	uint32_t                cpu_hwIntCnt[HWINTCNT_SIZE];                          
	uint64_t                cpu_hwIntpexits[HWINTCNT_SIZE];
	uint64_t                cpu_dr7;                             
	uint64_t                cpu_int_event_time;                               
	pal_rtc_nanotime_t      *cpu_nanotime;                             
#if KPC
	                                              
	uint64_t                *cpu_kpc_buf[2];
	                                         
	uint64_t                *cpu_kpc_shadow;
	uint64_t                *cpu_kpc_reload;
#endif
#if MONOTONIC
	struct mt_cpu cpu_monotonic;
#endif                
	uint32_t                cpu_pmap_pcid_enabled;
	pcid_t                  cpu_active_pcid;
	pcid_t                  cpu_last_pcid;
	pcid_t                  cpu_kernel_pcid;
	volatile pcid_ref_t     *cpu_pmap_pcid_coherentp;
	volatile pcid_ref_t     *cpu_pmap_pcid_coherentp_kernel;
	pcid_cdata_t            *cpu_pcid_data;
#ifdef  PCID_STATS
	uint64_t                cpu_pmap_pcid_flushes;
	uint64_t                cpu_pmap_pcid_preserves;
#endif
	uint64_t                cpu_aperf;
	uint64_t                cpu_mperf;
	uint64_t                cpu_c3res;
	uint64_t                cpu_c6res;
	uint64_t                cpu_c7res;
	uint64_t                cpu_itime_total;
	uint64_t                cpu_rtime_total;
	uint64_t                cpu_ixtime;
	uint64_t                cpu_idle_exits;
	  
                                                                                      
                                                                                    
                                                                                  
                    
    
	uint64_t                cpu_rtimes[CPU_RTIME_BINS];
	uint64_t                cpu_itimes[CPU_ITIME_BINS];
#if !MONOTONIC
	uint64_t                cpu_cur_insns;
	uint64_t                cpu_cur_ucc;
	uint64_t                cpu_cur_urc;
#endif                 
	uint64_t                cpu_gpmcs[4];
	uint64_t                cpu_max_observed_int_latency;
	int                     cpu_max_observed_int_latency_vector;
	volatile boolean_t      cpu_NMI_acknowledged;
	uint64_t                debugger_entry_time;
	uint64_t                debugger_ipi_time;
	                                                     
                                                                   
                                                              
                                               
    
	uint32_t                cpu_nested_istack;
	uint32_t                cpu_nested_istack_events;
	x86_saved_state64_t     *cpu_fatal_trap_state;
	x86_saved_state64_t     *cpu_post_fatal_trap_state;
#if CONFIG_VMX
	vmx_cpu_t               cpu_vmx;                                                       
#endif
#if CONFIG_MCA
	struct mca_state        *cpu_mca_state;                                
#endif
	int                     cpu_type;
	int                     cpu_subtype;
	int                     cpu_threadtype;
	boolean_t               cpu_iflag;
	boolean_t               cpu_boot_complete;
	int                     cpu_hibernate;
#define MAX_PREEMPTION_RECORDS (8)
#if     DEVELOPMENT || DEBUG
	int                     cpu_plri;
	plrecord_t              plrecords[MAX_PREEMPTION_RECORDS];
#endif
	void                    *cpu_console_buf;
	struct x86_lcpu         lcpu;
	int                     cpu_phys_number;                          
	cpu_id_t                cpu_id;                                      
#if DEBUG
	uint64_t                cpu_entry_cr3;
	uint64_t                cpu_exit_cr3;
	uint64_t                cpu_pcid_last_cr3;
#endif
	boolean_t               cpu_rendezvous_in_progress;
#if CST_DEMOTION_DEBUG
	                                                      
	uint64_t                cpu_wakeups_issued_total;
#endif
#if DEBUG || DEVELOPMENT
	uint64_t                tsc_sync_delta;
#endif
} cpu_data_t;

extern cpu_data_t       *cpu_data_ptr[];

  
                                          
                                                                                                
                                                                                          
   
#if defined(__SEG_GS)
                  
#elif defined(__clang__)
#define __seg_gs __attribute__((address_space(256)))
#else
#error use a compiler that supports address spaces or __seg_gs
#endif

#define CPU_DATA()            ((cpu_data_t __seg_gs *)0UL)

  
                                                                
                                                                  
                  
   


  
                                                                   
                                                                    
                                                                 
                                                                     
                                                                    
                                                                  
                                                                
                                                                 
                                                                    
                                                             
                                                                     
                                                                 
              
   
static inline thread_t
get_active_thread_volatile(void)
{
	return CPU_DATA()->cpu_active_thread;
}

static inline __attribute__((const)) thread_t
get_active_thread(void)
{
	return CPU_DATA()->cpu_active_thread;
}

#define current_thread_fast()           get_active_thread()
#define current_thread_volatile()       get_active_thread_volatile()
#define current_thread()                current_thread_fast()

#define cpu_mode_is64bit()              TRUE

static inline int
get_preemption_level(void)
{
	return CPU_DATA()->cpu_preemption_level;
}
static inline int
get_interrupt_level(void)
{
	return CPU_DATA()->cpu_interrupt_level;
}
static inline int
get_cpu_number(void)
{
	return CPU_DATA()->cpu_number;
}
static inline vm_offset_t
get_current_percpu_base(void)
{
	return CPU_DATA()->cpu_pcpu_base;
}
static inline int
get_cpu_phys_number(void)
{
	return CPU_DATA()->cpu_phys_number;
}

static inline cpu_data_t *
current_cpu_datap(void)
{
	return CPU_DATA()->cpu_this;
}

  
                                                                        
                                                                               
                                                                           
                                                                          
                                                                           
                                                                     
                                                                              
                                                                 
                                                                                   
                                                                                 
   
#if DEVELOPMENT || DEBUG
static inline void
rbtrace_bt(uint64_t *rets, int maxframes, cpu_data_t *cdata, uint64_t frameptr, bool use_cursp)
{
	extern uint32_t         low_intstack[];                     
	extern uint32_t         low_eintstack[];                 
	extern char             mp_slave_stack[];
	int                     btidx = 0;

	uint64_t kstackb, kstackt;

	                                                          
                                                         
                                              
    
	if (use_cursp) {
		__asm__ volatile ("leaq 1f(%%rip), %%rax; mov %%rax, %0\n1:"
                     : "=m" (rets[btidx++])
                     :
                     : "rax");
	}

	thread_t cplthread = cdata->cpu_active_thread;
	if (cplthread) {
		uintptr_t csp;
		if (use_cursp == true) {
			__asm__ __volatile__ ("movq %%rsp, %0": "=r" (csp):);
		} else {
			csp = frameptr;
		}
		                                                           
                                                            
             
     
		kstackb = cdata->cpu_active_stack;
		kstackt = kstackb + KERNEL_STACK_SIZE;
		if (csp < kstackb || csp > kstackt) {
			kstackt = cdata->cpu_kernel_stack;
			kstackb = kstackt - KERNEL_STACK_SIZE;
			if (csp < kstackb || csp > kstackt) {
				kstackt = cdata->cpu_int_stack_top;
				kstackb = kstackt - INTSTACK_SIZE;
				if (csp < kstackb || csp > kstackt) {
					kstackt = (uintptr_t)low_eintstack;
					kstackb = kstackt - INTSTACK_SIZE;
					if (csp < kstackb || csp > kstackt) {
						kstackb = (uintptr_t) mp_slave_stack;
						kstackt = kstackb + PAGE_SIZE;
					} else {
						kstackb = 0;
						kstackt = 0;
					}
				}
			}
		}

		if (__probable(kstackb && kstackt)) {
			uint64_t *cfp = (uint64_t *) frameptr;
			int rbbtf;

			for (rbbtf = btidx; rbbtf < maxframes; rbbtf++) {
				if (((uint64_t)cfp == 0) || (((uint64_t)cfp < kstackb) || ((uint64_t)cfp > kstackt))) {
					rets[rbbtf] = 0;
					continue;
				}
				rets[rbbtf] = *(cfp + 1);
				cfp = (uint64_t *) (*cfp);
			}
		}
	}
}

__attribute__((noinline))
static inline void
pltrace_internal(boolean_t enable)
{
	cpu_data_t *cdata = current_cpu_datap();
	int cpli = cdata->cpu_preemption_level;
	int cplrecord = cdata->cpu_plri;
	uint64_t *plbts;

	assert(cpli >= 0);

	cdata->plrecords[cplrecord].pltype = enable;
	cdata->plrecords[cplrecord].plevel = cpli;

	plbts = &cdata->plrecords[cplrecord].plbt[0];

	cplrecord++;

	if (cplrecord >= MAX_PREEMPTION_RECORDS) {
		cplrecord = 0;
	}

	cdata->cpu_plri = cplrecord;

	rbtrace_bt(plbts, MAX_TRACE_BTFRAMES - 1, cdata, (uint64_t)__builtin_frame_address(0), false);
}

extern int plctrace_enabled;

static inline void
iotrace(iotrace_type_e type, uint64_t vaddr, uint64_t paddr, int size, uint64_t val,
    uint64_t sabs, uint64_t duration)
{
	cpu_data_t *cdata;
	int cpu_num, nextidx;
	iotrace_entry_t *cur_iotrace_ring;

	if (__improbable(mmiotrace_enabled == 0 || iotrace_generators == 0)) {
		return;
	}

	cdata = current_cpu_datap();
	cpu_num = cdata->cpu_number;
	nextidx = iotrace_next[cpu_num];
	cur_iotrace_ring = iotrace_ring[cpu_num];

	cur_iotrace_ring[nextidx].iotype = type;
	cur_iotrace_ring[nextidx].vaddr = vaddr;
	cur_iotrace_ring[nextidx].paddr = paddr;
	cur_iotrace_ring[nextidx].size = size;
	cur_iotrace_ring[nextidx].val = val;
	cur_iotrace_ring[nextidx].start_time_abs = sabs;
	cur_iotrace_ring[nextidx].duration = duration;

	iotrace_next[cpu_num] = ((nextidx + 1) >= iotrace_entries_per_cpu) ? 0 : (nextidx + 1);

	rbtrace_bt(&cur_iotrace_ring[nextidx].backtrace[0],
	    MAX_TRACE_BTFRAMES - 1, cdata, (uint64_t)__builtin_frame_address(0), true);
}

static inline uint32_t
traptrace_start(int vecnum, uint64_t ipc, uint64_t sabs, uint64_t frameptr)
{
	cpu_data_t *cdata;
	unsigned int cpu_num, nextidx;
	traptrace_entry_t *cur_traptrace_ring;

	if (__improbable(traptrace_enabled == 0 || traptrace_generators == 0)) {
		return TRAPTRACE_INVALID_INDEX;
	}

	assert(ml_get_interrupts_enabled() == FALSE);
	cdata = current_cpu_datap();
	cpu_num = (unsigned int)cdata->cpu_number;
	nextidx = (unsigned int)traptrace_next[cpu_num];
	                                                           
	traptrace_next[cpu_num] = (int)(((nextidx + 1) >= (unsigned int)traptrace_entries_per_cpu) ? 0 : (nextidx + 1));

	cur_traptrace_ring = traptrace_ring[cpu_num];

	cur_traptrace_ring[nextidx].vector = vecnum;
	cur_traptrace_ring[nextidx].curthread = current_thread();
	cur_traptrace_ring[nextidx].interrupted_pc = ipc;
	cur_traptrace_ring[nextidx].curpl = cdata->cpu_preemption_level;
	cur_traptrace_ring[nextidx].curil = cdata->cpu_interrupt_level;
	cur_traptrace_ring[nextidx].start_time_abs = sabs;
	cur_traptrace_ring[nextidx].duration = ~0ULL;

	rbtrace_bt(&cur_traptrace_ring[nextidx].backtrace[0],
	    MAX_TRACE_BTFRAMES - 1, cdata, frameptr, false);

	assert(nextidx <= 0xFFFF);

	return (uint32_t)((cpu_num << 16) | nextidx);
}

static inline void
traptrace_end(uint32_t index, uint64_t eabs)
{
	if (index != TRAPTRACE_INVALID_INDEX) {
		traptrace_entry_t *ttentp = &traptrace_ring[index >> 16][index & 0xFFFF];

		ttentp->duration = eabs - ttentp->start_time_abs;
	}
}

#endif                           

__header_always_inline void
pltrace(boolean_t plenable)
{
#if DEVELOPMENT || DEBUG
	if (__improbable(plctrace_enabled != 0)) {
		pltrace_internal(plenable);
	}
#else
	(void)plenable;
#endif
}

static inline void
disable_preemption_internal(void)
{
	assert(get_preemption_level() >= 0);

	os_compiler_barrier();
	CPU_DATA()->cpu_preemption_level++;
	os_compiler_barrier();
	pltrace(FALSE);
}

static inline void
enable_preemption_internal(void)
{
	assert(get_preemption_level() > 0);
	pltrace(TRUE);
	os_compiler_barrier();
	if (0 == --CPU_DATA()->cpu_preemption_level) {
		kernel_preempt_check();
	}
	os_compiler_barrier();
}

static inline void
enable_preemption_no_check(void)
{
	assert(get_preemption_level() > 0);

	pltrace(TRUE);
	os_compiler_barrier();
	CPU_DATA()->cpu_preemption_level--;
	os_compiler_barrier();
}

static inline void
_enable_preemption_no_check(void)
{
	enable_preemption_no_check();
}

static inline void
mp_disable_preemption(void)
{
	disable_preemption_internal();
}

static inline void
_mp_disable_preemption(void)
{
	disable_preemption_internal();
}

static inline void
mp_enable_preemption(void)
{
	enable_preemption_internal();
}

static inline void
_mp_enable_preemption(void)
{
	enable_preemption_internal();
}

static inline void
mp_enable_preemption_no_check(void)
{
	enable_preemption_no_check();
}

static inline void
_mp_enable_preemption_no_check(void)
{
	enable_preemption_no_check();
}

#ifdef XNU_KERNEL_PRIVATE
#define disable_preemption() disable_preemption_internal()
#define enable_preemption() enable_preemption_internal()
#define MACHINE_PREEMPTION_MACROS (1)
#endif

static inline cpu_data_t *
cpu_datap(int cpu)
{
	return cpu_data_ptr[cpu];
}

static inline int
cpu_is_running(int cpu)
{
	return (cpu_datap(cpu) != NULL) && (cpu_datap(cpu)->cpu_running);
}

#ifdef MACH_KERNEL_PRIVATE
static inline cpu_data_t *
cpu_shadowp(int cpu)
{
	return cpu_data_ptr[cpu]->cd_shadow;
}

#endif
extern cpu_data_t *cpu_data_alloc(boolean_t is_boot_cpu);
extern void cpu_data_realloc(void);

#endif                     
