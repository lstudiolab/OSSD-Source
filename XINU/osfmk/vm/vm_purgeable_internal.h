/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

  
                           
                                              
                                                   
                                                                         
                                                                    
                      
                                                                     
                                               
                                                                          
                                                                           
   

#ifndef __VM_PURGEABLE_INTERNAL__
#define __VM_PURGEABLE_INTERNAL__

#include <kern/queue.h>

enum purgeable_q_type {
	PURGEABLE_Q_TYPE_OBSOLETE,
	PURGEABLE_Q_TYPE_FIFO,
	PURGEABLE_Q_TYPE_LIFO,
	PURGEABLE_Q_TYPE_MAX
};

typedef uint32_t token_idx_t;
typedef uint32_t token_cnt_t;
#define TOKEN_COUNT_MAX UINT32_MAX

#define NUM_VOLATILE_GROUPS 8
struct purgeable_q {
	token_idx_t token_q_head;                     
	token_idx_t token_q_tail;                     
	token_idx_t token_q_unripe;                                     
	int32_t new_pages;
	queue_head_t objq[NUM_VOLATILE_GROUPS];
	enum purgeable_q_type type;
#if MACH_ASSERT
	int debug_count_tokens;
	int debug_count_objects;
#endif
};

typedef struct purgeable_q * purgeable_q_t;

extern struct purgeable_q purgeable_queues[PURGEABLE_Q_TYPE_MAX];
extern queue_head_t purgeable_nonvolatile_queue;
extern int purgeable_nonvolatile_count;
extern int32_t token_new_pagecount;
#define TOKEN_NEW_PAGECOUNT_MAX INT32_MAX
extern int available_for_purge;


  
           
                                                                            
                                                                             
                                                                           
                                                                          
                  
   
decl_lck_mtx_data(extern, vm_purgeable_queue_lock);

                                                                     
                                  
kern_return_t vm_purgeable_token_add(purgeable_q_t queue);

                                  
void vm_purgeable_token_delete_first(purgeable_q_t queue);
void vm_purgeable_token_delete_last(purgeable_q_t queue);

  
                            
                               
   
void vm_purgeable_q_advance_all(void);

                                                                     
                                                            
boolean_t vm_purgeable_object_purge_one(int force_purge_below_group, int flags);

                                    
void vm_purgeable_object_purge_all(void);

                                        
void vm_purgeable_object_add(vm_object_t object, purgeable_q_t queue, int group);

                                                             
purgeable_q_t vm_purgeable_object_remove(vm_object_t object);

                                                   
void vm_purgeable_stats(vm_purgeable_info_t info, task_t target_task);

#if DEVELOPMENT || DEBUG
                                                                    
kern_return_t vm_purgeable_account(task_t task, pvm_account_info_t acnt_info);
#endif                           

uint64_t vm_purgeable_purge_task_owned(task_t task);
void vm_purgeable_nonvolatile_enqueue(vm_object_t object, task_t task);
void vm_purgeable_nonvolatile_dequeue(vm_object_t object);
void vm_purgeable_accounting(vm_object_t        object,
    vm_purgable_t      old_state);
void vm_object_owner_compressed_update(vm_object_t      object,
    int              delta);

#define PURGEABLE_LOOP_MAX 64

#define TOKEN_ADD               0x40               
#define TOKEN_DELETE            0x41               
#define TOKEN_RIPEN             0x42               
#define OBJECT_ADD              0x48               
#define OBJECT_REMOVE           0x49               
#define OBJECT_PURGE            0x4a               
#define OBJECT_PURGE_ALL        0x4b               
#define OBJECT_PURGE_ONE        0x4c               
#define OBJECT_PURGE_LOOP       0x4e               

#endif                                
