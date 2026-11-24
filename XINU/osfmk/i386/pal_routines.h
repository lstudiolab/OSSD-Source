/*
 * Copyright (c) 2009-2012 Apple Inc. All rights reserved.
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
#ifndef _I386_PAL_ROUTINES_H
#define _I386_PAL_ROUTINES_H

#include <stdint.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>

#if defined(__cplusplus)
extern "C" {
#endif

                                    

  
                                                    
                                                    
   
struct pal_efi_registers {
	uint64_t rcx;
	uint64_t rdx;
	uint64_t r8;
	uint64_t r9;
	uint64_t rax;
};

  
                                                    
                               
   
kern_return_t
pal_efi_call_in_64bit_mode(uint64_t func,
    struct pal_efi_registers *efi_reg,
    void *stack_contents,
    size_t stack_contents_size,                                              
    uint64_t *efi_status);

kern_return_t
pal_efi_call_in_32bit_mode(uint32_t func,
    struct pal_efi_registers *efi_reg,
    void *stack_contents,
    size_t stack_contents_size,                                              
    uint32_t *efi_status);

                        

boolean_t pal_machine_sleep(uint8_t type_a,
    uint8_t type_b,
    uint32_t bit_position,
    uint32_t disable_mask,
    uint32_t enable_mask);

                               
#ifdef XNU_KERNEL_PRIVATE

                                           
typedef uint64_t pal_cr_t;

struct pal_cpu_data;                           
struct pal_pcb;                           
struct pal_apic_table;                           

                          
extern struct pal_apic_table *apic_table;

                                    
extern int  pal_serial_init(void);
extern void pal_serial_putc(char);
extern void pal_serial_putc_nocr(char);
extern int  pal_serial_getc(void);

                                        
extern void pal_i386_init(void);
extern void pal_set_signal_delivery(thread_t);

                           
extern void pal_get_control_registers( pal_cr_t *cr0, pal_cr_t *cr2,
    pal_cr_t *cr3, pal_cr_t *cr4 );

                                               
extern void pal_dbg_page_fault( thread_t thread, user_addr_t vadddr,
    kern_return_t kr );

                                                       
extern void pal_dbg_set_task_name( task_t task );

                                             
void pal_syscall_restart(thread_t thread, x86_saved_state_t *state);

                             
void pal_execve_return(thread_t thread);

                                       
void pal_thread_terminate_self(thread_t thread);

                           
void pal_ast_check(thread_t thread);

                               
extern void pal_get_kern_regs( x86_saved_state_t *state ) __dead2;

  
                             
   
extern void pal_hlt(void);
extern void pal_sti(void);
extern void pal_cli(void);

  
                                                       
   
typedef enum { DIRTY, VALID } pal_cache_state_t;
void pal_register_cache_state(thread_t thread, pal_cache_state_t state);


                                                               
void pal_preemption_assert(void);

                                                                    
#include <i386/pal_native.h>

#define PAL_XCPM_PROPERTY_VALUE 3

extern boolean_t virtualized;
#define PAL_VIRTUALIZED_PROPERTY_VALUE 4

                                              
#define PAL_AICPM_PROPERTY_NAME "intel_cpupm_matching"
static inline void
pal_get_resource_property(const char **property_name, int *property_value)
{
	*property_name = PAL_AICPM_PROPERTY_NAME;
	*property_value = PAL_XCPM_PROPERTY_VALUE;
	if (virtualized) {
		*property_value = PAL_VIRTUALIZED_PROPERTY_VALUE;
	}
}

                                                     
extern void _pal_rtc_nanotime_store(
	uint64_t                tsc,
	uint64_t                nsec,
	uint32_t                scale,
	uint32_t                shift,
	struct pal_rtc_nanotime *dst);

                          
extern struct pal_rtc_nanotime pal_rtc_nanotime_info;

#endif                         

#if defined(__cplusplus)
}
#endif

#endif                           
