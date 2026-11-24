/*
 * Copyright (c) 2000-2004 Apple Computer, Inc. All rights reserved.
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
#ifndef _IPC_IPC_VOUCHER_H_
#define _IPC_IPC_VOUCHER_H_

#include <mach/mach_types.h>
#include <mach/mach_voucher_types.h>
#include <mach/boolean.h>
#include <ipc/ipc_types.h>
#include <os/refcnt.h>

#ifdef MACH_KERNEL_PRIVATE

#include <kern/queue.h>
#include <kern/locks.h>
#include <kern/simple_lock.h>
#include <voucher/ipc_pthread_priority_types.h>

             
extern lck_grp_t        ipc_lck_grp;
extern lck_attr_t       ipc_lck_attr;

                                     
typedef mach_voucher_attr_value_handle_t        iv_value_handle_t;
typedef mach_voucher_attr_value_reference_t     iv_value_refs_t;

typedef natural_t               iv_index_t;
#define IV_UNUSED_VALINDEX      ((iv_index_t) 0)
#define IV_UNUSED_KEYINDEX      ((iv_index_t) ~0)

typedef iv_index_t              *iv_entry_t;
#define IVE_NULL                ((iv_entry_t) 0)

#define IV_ENTRIES_INLINE MACH_VOUCHER_ATTR_KEY_NUM_WELL_KNOWN

  
              
  
                                                                   
                                                          
                                            
   
struct ipc_voucher {
	iv_index_t              iv_hash;                           
	iv_index_t              iv_sum;                                 
	os_refcnt_t             iv_refs;                             
	iv_index_t              iv_table_size;                                 
	iv_index_t              iv_inline_table[IV_ENTRIES_INLINE];
	iv_entry_t              iv_table;                                          
	ipc_port_t              iv_port;                                           
	queue_chain_t           iv_hash_link;                           
};

#define IV_NULL         IPC_VOUCHER_NULL


  
                                         
  
                                                                   
                                                                       
                                                                   
                                                                   
                         
  
                                                                   
              
  
                                                                 
                                                                   
                                                                  
                                                            
  
                                                                    
                                                                
                                                                  
                                                                    
                                                                    
                                                                     
                                                                          
                                                                    
                                                                         
                                                                      
                                                                  
                                                                      
                                                             
   

struct ivac_entry_s {
	iv_value_handle_t       ivace_value;
	iv_value_refs_t         ivace_layered:1,                                  
	    ivace_releasing:1,                                                
	    ivace_free:1,                                             
	    ivace_persist:1,                                                                       
	    ivace_refs:28;                                                
	union {
		iv_value_refs_t ivaceu_made;                                       
		iv_index_t      ivaceu_layer;                                            
	} ivace_u;
	iv_index_t              ivace_next;                                
	iv_index_t              ivace_index;                                      
};
typedef struct ivac_entry_s       ivac_entry;
typedef ivac_entry              *ivac_entry_t;

#define ivace_made              ivace_u.ivaceu_made
#define ivace_layer             ivace_u.ivaceu_layer

#define IVACE_NULL              ((ivac_entry_t) 0);

#define IVACE_REFS_MAX          ((1 << 28) - 1)

#define IVAC_ENTRIES_MIN        512
#define IVAC_ENTRIES_MAX        524288

struct ipc_voucher_attr_control {
	os_refcnt_t             ivac_refs;
	boolean_t               ivac_is_growing;                                      
	ivac_entry_t            ivac_table;                                                      
	iv_index_t              ivac_table_size;                                          
	iv_index_t              ivac_init_table_size;                                     
	iv_index_t              ivac_freelist;                                               
	ipc_port_t              ivac_port;                                                         
	lck_spin_t              ivac_lock_data;
	iv_index_t              ivac_key_index;                                       
};
typedef ipc_voucher_attr_control_t iv_attr_control_t;

#define IVAC_NULL                  IPC_VOUCHER_ATTR_CONTROL_NULL

extern ipc_voucher_attr_control_t  ivac_alloc(iv_index_t);
extern void ipc_voucher_receive_postprocessing(ipc_kmsg_t kmsg, mach_msg_option_t option);
extern void ipc_voucher_send_preprocessing(ipc_kmsg_t kmsg);
extern void mach_init_activity_id(void);
extern kern_return_t ipc_get_pthpriority_from_kmsg_voucher(ipc_kmsg_t kmsg, ipc_pthread_priority_value_t *qos);
#define ivac_lock_init(ivac) \
	lck_spin_init(&(ivac)->ivac_lock_data, &ipc_lck_grp, &ipc_lck_attr)
#define ivac_lock_destroy(ivac) \
	lck_spin_destroy(&(ivac)->ivac_lock_data, &ipc_lck_grp)
#define ivac_lock(ivac) \
	lck_spin_lock_grp(&(ivac)->ivac_lock_data, &ipc_lck_grp)
#define ivac_lock_try(ivac) \
	lck_spin_try_lock_grp(&(ivac)->ivac_lock_data, &ipc_lck_grp)
#define ivac_unlock(ivac) \
	lck_spin_unlock(&(ivac)->ivac_lock_data)
#define ivac_sleep(ivac) lck_spin_sleep_grp(&(ivac)->ivac_lock_data,        \
	                                LCK_SLEEP_DEFAULT,              \
	                                (event_t)(ivac),                \
	                                THREAD_UNINT, &ipc_lck_grp)
#define ivac_wakeup(ivac) thread_wakeup((event_t)(ivac))

extern void ivac_dealloc(ipc_voucher_attr_control_t ivac);

static inline void
ivac_reference(ipc_voucher_attr_control_t ivac)
{
	if (ivac == IVAC_NULL) {
		return;
	}
	os_ref_retain(&ivac->ivac_refs);
}

static inline void
ivac_release(ipc_voucher_attr_control_t ivac)
{
	if (IVAC_NULL == ivac) {
		return;
	}

	if (os_ref_release(&ivac->ivac_refs) == 0) {
		ivac_dealloc(ivac);
	}
}

#define IVAM_NULL IPC_VOUCHER_ATTR_MANAGER_NULL

  
                                             
  
                                                         
                              
  
                                                                            
                                                                           
                                                                          
                          
   
typedef struct ipc_voucher_global_table_element {
	ipc_voucher_attr_manager_t      ivgte_manager;
	ipc_voucher_attr_control_t      ivgte_control;
	mach_voucher_attr_key_t         ivgte_key;
} ipc_voucher_global_table_element;

typedef ipc_voucher_global_table_element *ipc_voucher_global_table_element_t;

#endif                          

  
                               
  
                                                                         
                     
   
#pragma pack(1)
typedef struct ipc_voucher_attr_recipe_data {
	mach_voucher_attr_key_t                 key;
	mach_voucher_attr_recipe_command_t      command;
	ipc_voucher_t                           previous_voucher;
	mach_voucher_attr_content_size_t        content_size;
	uint8_t                                 content[];
} ipc_voucher_attr_recipe_data_t;
typedef ipc_voucher_attr_recipe_data_t *ipc_voucher_attr_recipe_t;
typedef mach_msg_type_number_t ipc_voucher_attr_recipe_size_t;

typedef uint8_t *ipc_voucher_attr_raw_recipe_t;
typedef ipc_voucher_attr_raw_recipe_t ipc_voucher_attr_raw_recipe_array_t;
typedef mach_msg_type_number_t ipc_voucher_attr_raw_recipe_size_t;
typedef mach_msg_type_number_t ipc_voucher_attr_raw_recipe_array_size_t;

#pragma pack()

  
                                        
  
                                                                          
                                                                             
  
                                                                       
                                                                              
   

typedef kern_return_t (*ipc_voucher_attr_manager_release_value_t)(ipc_voucher_attr_manager_t,
    mach_voucher_attr_key_t,
    mach_voucher_attr_value_handle_t,
    mach_voucher_attr_value_reference_t);

typedef kern_return_t (*ipc_voucher_attr_manager_get_value_t)(ipc_voucher_attr_manager_t,
    mach_voucher_attr_key_t,
    mach_voucher_attr_recipe_command_t,
    mach_voucher_attr_value_handle_array_t,
    mach_voucher_attr_value_handle_array_size_t,
    mach_voucher_attr_content_t,
    mach_voucher_attr_content_size_t,
    mach_voucher_attr_value_handle_t *,
    mach_voucher_attr_value_flags_t *,
    ipc_voucher_t *);

typedef kern_return_t (*ipc_voucher_attr_manager_extract_content_t)(ipc_voucher_attr_manager_t,
    mach_voucher_attr_key_t,
    mach_voucher_attr_value_handle_array_t,
    mach_voucher_attr_value_handle_array_size_t,
    mach_voucher_attr_recipe_command_t *,
    mach_voucher_attr_content_t,
    mach_voucher_attr_content_size_t *);

typedef kern_return_t (*ipc_voucher_attr_manager_command_t)(ipc_voucher_attr_manager_t,
    mach_voucher_attr_key_t,
    mach_voucher_attr_value_handle_array_t,
    mach_voucher_attr_value_handle_array_size_t,
    mach_voucher_attr_command_t,
    mach_voucher_attr_content_t,
    mach_voucher_attr_content_size_t,
    mach_voucher_attr_content_t,
    mach_voucher_attr_content_size_t *);

typedef void (*ipc_voucher_attr_manager_release_t)(ipc_voucher_attr_manager_t);

typedef uint32_t ipc_voucher_attr_manager_flags;

struct ipc_voucher_attr_manager {
	ipc_voucher_attr_manager_release_value_t        ivam_release_value;
	ipc_voucher_attr_manager_get_value_t            ivam_get_value;
	ipc_voucher_attr_manager_extract_content_t      ivam_extract_content;
	ipc_voucher_attr_manager_command_t              ivam_command;
	ipc_voucher_attr_manager_release_t              ivam_release;
	ipc_voucher_attr_manager_flags                  ivam_flags;
};

#define IVAM_FLAGS_NONE                              0
#define IVAM_FLAGS_SUPPORT_SEND_PREPROCESS         0x1
#define IVAM_FLAGS_SUPPORT_RECEIVE_POSTPROCESS     0x2

__BEGIN_DECLS

                                                  
extern uintptr_t unsafe_convert_port_to_voucher(
	ipc_port_t              port);

                                      
extern ipc_voucher_t convert_port_to_voucher(
	ipc_port_t              port);

                                                
extern ipc_voucher_t convert_port_name_to_voucher(
	mach_port_name_t        name);

                                              
extern void ipc_voucher_reference(
	ipc_voucher_t           voucher);

                                                
extern void ipc_voucher_release(
	ipc_voucher_t           voucher);

                                   
extern void ipc_voucher_notify(
	mach_msg_header_t       *msg);

                                      
extern ipc_port_t convert_voucher_to_port(
	ipc_voucher_t           voucher);

                                                        
extern ipc_port_t convert_voucher_attr_control_to_port(
	ipc_voucher_attr_control_t      control);

                                              
extern void ipc_voucher_attr_control_reference(
	ipc_voucher_attr_control_t      control);

                                        
extern void ipc_voucher_attr_control_release(
	ipc_voucher_attr_control_t      control);

                                           
extern void ipc_voucher_attr_control_notify(
	mach_msg_header_t       *msg);

                                                        
extern ipc_voucher_attr_control_t convert_port_to_voucher_attr_control(
	ipc_port_t              port);

  
                                             
   
extern kern_return_t
ipc_create_mach_voucher(
	ipc_voucher_attr_raw_recipe_array_t             recipes,
	ipc_voucher_attr_raw_recipe_array_size_t        recipe_size,
	ipc_voucher_t                                   *new_voucher);

extern kern_return_t
ipc_voucher_attr_control_create_mach_voucher(
	ipc_voucher_attr_control_t                      control,
	ipc_voucher_attr_raw_recipe_array_t             recipes,
	ipc_voucher_attr_raw_recipe_array_size_t        recipe_size,
	ipc_voucher_t                                   *new_voucher);

extern kern_return_t
ipc_register_well_known_mach_voucher_attr_manager(
	ipc_voucher_attr_manager_t              manager,
	mach_voucher_attr_value_handle_t        default_value,
	mach_voucher_attr_key_t                 key,
	ipc_voucher_attr_control_t              *control);


extern kern_return_t
ipc_register_mach_voucher_attr_manager(
	ipc_voucher_attr_manager_t              manager,
	mach_voucher_attr_value_handle_t        default_value,
	mach_voucher_attr_key_t                 *key,
	ipc_voucher_attr_control_t              *control);

__END_DECLS

#endif                           
