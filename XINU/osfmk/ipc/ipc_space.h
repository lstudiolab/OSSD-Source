/*
 * Copyright (c) 2000-2016 Apple Computer, Inc. All rights reserved.
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
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
                                                                         
                                                                          
                                                                        
               
   
  
   
  
                        
                      
             
  
                                              
   

#ifndef _IPC_IPC_SPACE_H_
#define _IPC_IPC_SPACE_H_


#include <prng/random.h>
#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/vm_types.h>

#include <sys/appleapiopts.h>

#ifdef __APPLE_API_PRIVATE
#ifdef MACH_KERNEL_PRIVATE
#include <kern/macro_help.h>
#include <kern/kern_types.h>
#include <kern/locks.h>
#include <kern/task.h>
#include <kern/zalloc.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_types.h>

#include <libkern/OSAtomic.h>

  
                                              
                                                       
                                                            
  
                                                                  
  
                                                              
                                                               
                                                               
                                                   
   

typedef natural_t ipc_space_refs_t;
#define IS_REFS_MAX     0x0fffffff
#define IS_INACTIVE     0x40000000                             
#define IS_GROWING      0x20000000                            
#define IS_ENTROPY_CNT  1                                                

struct ipc_space {
	lck_spin_t      is_lock_data;
	ipc_space_refs_t is_bits;                                        
	ipc_entry_num_t is_table_size;                             
	ipc_entry_num_t is_table_hashed;                              
	ipc_entry_num_t is_table_free;                              
	ipc_entry_t XNU_PTRAUTH_SIGNED_PTR("ipc_space.is_table") is_table;                          
	struct ipc_table_size * XNU_PTRAUTH_SIGNED_PTR("ipc_space.is_table_next") is_table_next;                            
	task_t XNU_PTRAUTH_SIGNED_PTR("ipc_space.is_task") is_task;                      
	ipc_label_t is_label;                                                 
	ipc_entry_num_t is_low_mod;                                              
	ipc_entry_num_t is_high_mod;                                              
	struct bool_gen bool_gen;                                  
	unsigned int is_entropy[IS_ENTROPY_CNT];                                     
	int is_node_id;                                                                     
};

#define IS_NULL                 ((ipc_space_t) 0)
#define IS_INSPECT_NULL         ((ipc_space_inspect_t) 0)

#define is_active(is)           (((is)->is_bits & IS_INACTIVE) != IS_INACTIVE)

static inline void
is_mark_inactive(ipc_space_t is)
{
	assert(is_active(is));
	OSBitOrAtomic(IS_INACTIVE, &is->is_bits);
}

#define is_growing(is)          (((is)->is_bits & IS_GROWING) == IS_GROWING)

static inline void
is_start_growing(ipc_space_t is)
{
	assert(!is_growing(is));
	OSBitOrAtomic(IS_GROWING, &is->is_bits);
}

static inline void
is_done_growing(ipc_space_t is)
{
	assert(is_growing(is));
	OSBitAndAtomic((ipc_space_refs_t)~IS_GROWING, &is->is_bits);
}

extern zone_t ipc_space_zone;

#define is_alloc()              ((ipc_space_t) zalloc(ipc_space_zone))
#define is_free(is)             zfree(ipc_space_zone, (is))

extern ipc_space_t ipc_space_kernel;
extern ipc_space_t ipc_space_reply;
#if     DIPC
extern ipc_space_t ipc_space_remote;
#endif            
#if     DIPC
extern ipc_space_t default_pager_space;
#endif            

extern lck_grp_t        ipc_lck_grp;
extern lck_attr_t       ipc_lck_attr;

#define is_lock_init(is)        lck_spin_init(&(is)->is_lock_data, &ipc_lck_grp, &ipc_lck_attr)
#define is_lock_destroy(is)     lck_spin_destroy(&(is)->is_lock_data, &ipc_lck_grp)

#define is_read_lock(is)        lck_spin_lock_grp(&(is)->is_lock_data, &ipc_lck_grp)
#define is_read_unlock(is)      lck_spin_unlock(&(is)->is_lock_data)
#define is_read_sleep(is)       lck_spin_sleep_grp(&(is)->is_lock_data,     \
	                                                LCK_SLEEP_DEFAULT,                                      \
	                                                (event_t)(is),                                          \
	                                                THREAD_UNINT,                                           \
	                                                &ipc_lck_grp)

#define is_write_lock(is)       lck_spin_lock_grp(&(is)->is_lock_data, &ipc_lck_grp)
#define is_write_unlock(is)     lck_spin_unlock(&(is)->is_lock_data)
#define is_write_sleep(is)      lck_spin_sleep_grp(&(is)->is_lock_data,     \
	                                                LCK_SLEEP_DEFAULT,                                      \
	                                                (event_t)(is),                                          \
	                                                THREAD_UNINT,                                           \
	                                                &ipc_lck_grp)

#define is_refs(is)             ((is)->is_bits & IS_REFS_MAX)

static inline void
is_reference(ipc_space_t is)
{
	assert(is_refs(is) > 0 && is_refs(is) < IS_REFS_MAX);
	OSIncrementAtomic(&(is->is_bits));
}


static inline void
is_release(ipc_space_t is)
{
	assert(is_refs(is) > 0);

	                                                 
	if (1 == (OSDecrementAtomic(&(is->is_bits)) & IS_REFS_MAX)) {
		assert(!is_active(is));
		is_lock_destroy(is);
		is_free(is);
	}
}

#define current_space_fast()    (current_task_fast()->itk_space)
#define current_space()         (current_space_fast())

                                
extern kern_return_t ipc_space_create_special(
	ipc_space_t     *spacep);

                            
extern kern_return_t ipc_space_create(
	ipc_table_size_t        initial,
	ipc_label_t             label,
	ipc_space_t             *spacep);

                                           
extern kern_return_t ipc_space_label(
	ipc_space_t space,
	ipc_label_t label);

                                      
extern kern_return_t ipc_space_add_label(
	ipc_space_t space,
	ipc_label_t label);

                                                   
extern void ipc_space_terminate(
	ipc_space_t     space);

                                                      
extern void ipc_space_clean(
	ipc_space_t     space);

                                                      
extern void ipc_space_rand_freelist(
	ipc_space_t             space,
	ipc_entry_t             table,
	mach_port_index_t       bottom,
	mach_port_index_t       top);

                                                                        
extern ipc_entry_bits_t ipc_space_get_rollpoint(ipc_space_t space);

                                                  
extern kern_return_t mach_port_allocate_internal(
	ipc_space_t       space,
	mach_port_right_t right,
	mach_port_qos_t   *qosp,
	mach_port_name_t  *namep);

#endif                          
#endif                          

#ifdef  __APPLE_API_UNSTABLE
#ifndef MACH_KERNEL_PRIVATE

extern ipc_space_t              current_space(void);

#endif                           
#endif                           

                                 
extern void ipc_space_reference(
	ipc_space_t     space);

                                    
extern void ipc_space_release(
	ipc_space_t     space);

#endif                         
