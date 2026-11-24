/*
 * Copyright (c) 2012-2019 Apple Inc. All rights reserved.
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

#ifndef _BANK_BANK_INTERNAL_H_
#define _BANK_BANK_INTERNAL_H_

#include <stdint.h>
#include <mach/mach_types.h>

#ifdef  MACH_KERNEL_PRIVATE

#include <kern/thread.h>
#include <kern/thread_group.h>
#include <kern/locks.h>
#include <kern/queue.h>
#include <ipc/ipc_voucher.h>
#include <bank/bank_types.h>

                                                          
#define BANK_DEFAULT_VALUE NULL
#define BANK_DEFAULT_TASK_VALUE ((void *) 1)

typedef mach_voucher_attr_value_handle_t bank_handle_t;

#define BANK_TASK        0
#define BANK_ACCOUNT     1

struct bank_element {
	unsigned int  be_type:31,                                     
	    be_voucher_ref:1;                                                    
	int           be_refs;                                  
	unsigned int  be_made;                                                                                                            
#if DEVELOPMENT || DEBUG
	task_t        be_task;                                                                                    
#endif
};

typedef struct bank_element * bank_element_t;
#define BANK_ELEMENT_NULL ((bank_element_t) 0)

struct bank_task {
	struct bank_element       bt_elem;                                   
	struct proc_persona_info  bt_proc_persona;                                     
	ledger_t                  bt_ledger;                                                
	queue_head_t              bt_accounts_to_pay;                                                          
	queue_head_t              bt_accounts_to_charge;                                                       
	decl_lck_mtx_data(, bt_acc_to_pay_lock);                                                     
	decl_lck_mtx_data(, bt_acc_to_charge_lock);                                                     
	boolean_t                 bt_hasentitlement;                                                                 
#if CONFIG_THREAD_GROUPS
	struct thread_group *         bt_thread_group;                                               
#endif
#if DEVELOPMENT || DEBUG
	queue_chain_t             bt_global_elt;                                                      
#endif
};

#define bt_type             bt_elem.be_type
#define bt_voucher_ref      bt_elem.be_voucher_ref
#define bt_refs             bt_elem.be_refs
#define bt_made             bt_elem.be_made

#define bt_flags            bt_proc_persona.flags
#define bt_unique_pid       bt_proc_persona.unique_pid
#define bt_pid              bt_proc_persona.pid
#define bt_pidversion       bt_proc_persona.pidversion
#define bt_persona_id       bt_proc_persona.persona_id
#define bt_uid              bt_proc_persona.uid
#define bt_gid              bt_proc_persona.gid
#define bt_macho_uuid       bt_proc_persona.macho_uuid

#if DEVELOPMENT || DEBUG
#define bt_task             bt_elem.be_task
#endif

typedef struct bank_task * bank_task_t;
#define BANK_TASK_NULL  ((bank_task_t) 0)

#define bank_task_reference(elem)       \
	        (OSAddAtomic(1, &(elem)->bt_refs))

#define bank_task_release(elem)         \
	        (OSAddAtomic(-1, &(elem)->bt_refs))

#define bank_task_release_num(elem, num)        \
	        (OSAddAtomic(-(num), &(elem)->bt_refs))

#define bank_task_made_reference(elem)  \
	        (os_atomic_inc_orig(&(elem)->bt_made, relaxed))

#define bank_task_made_release(elem)    \
	        (os_atomic_dec_orig(&(elem)->bt_made, relaxed))

#define bank_task_made_release_num(elem, num)   \
	        (os_atomic_sub_orig(&(elem)->bt_made, (num), relaxed))


struct bank_account {
	struct bank_element ba_elem;                                   
	ledger_t            ba_bill;                                                 
	bank_task_t         ba_merchant;                                                                          
	bank_task_t         ba_holder;                                            
	bank_task_t         ba_secureoriginator;                                             
	bank_task_t         ba_proximateprocess;                                                   
	queue_chain_t       ba_next_acc_to_pay;                                         
	queue_chain_t       ba_next_acc_to_charge;                                         
#if CONFIG_THREAD_GROUPS
	struct thread_group * ba_thread_group;                                       
#endif
#if DEVELOPMENT || DEBUG
	queue_chain_t       ba_global_elt;                                                    
#endif
	uint32_t            ba_so_persona_id;                                             
                                                                                          
};

#define ba_type             ba_elem.be_type
#define ba_voucher_ref      ba_elem.be_voucher_ref
#define ba_refs             ba_elem.be_refs
#define ba_made             ba_elem.be_made

#if DEVELOPMENT || DEBUG
#define ba_task             ba_elem.be_task
#endif

typedef struct bank_account * bank_account_t;
#define BANK_ACCOUNT_NULL  ((bank_account_t) 0)

#define bank_account_reference(elem)    \
	        (OSAddAtomic(1, &(elem)->ba_refs))

#define bank_account_release(elem)      \
	        (OSAddAtomic(-1, &(elem)->ba_refs))

#define bank_account_release_num(elem, num)     \
	        (OSAddAtomic(-(num), &(elem)->ba_refs))

#define bank_account_made_reference(elem)       \
	        (os_atomic_inc_orig(&(elem)->ba_made, relaxed))

#define bank_account_made_release(elem)         \
	        (os_atomic_dec_orig(&(elem)->ba_made, relaxed))

#define bank_account_made_release_num(elem, num)        \
	        (os_atomic_sub_orig(&(elem)->ba_made, (num), relaxed))

struct _bank_ledger_indices {
	int cpu_time;
	int energy;
};

extern struct _bank_ledger_indices bank_ledgers;

extern void bank_init(void);
extern void bank_task_destroy(task_t);
extern void bank_task_initialize(task_t task);
extern void bank_billed_balance_safe(task_t task, uint64_t *cpu_time, uint64_t *energy);
extern void bank_billed_balance(bank_task_t bank_task, uint64_t *cpu_time, uint64_t *energy);
extern void bank_serviced_balance_safe(task_t task, uint64_t *cpu_time, uint64_t *energy);
extern void bank_serviced_balance(bank_task_t bank_task, uint64_t *cpu_time, uint64_t *energy);
extern kern_return_t bank_get_bank_ledger_thread_group_and_persona(ipc_voucher_t voucher,
    ledger_t *bankledger, struct thread_group **banktg, uint32_t *persona_id);
extern void bank_swap_thread_bank_ledger(thread_t thread, ledger_t ledger);

#endif                          
#endif                             
