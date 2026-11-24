/*
 * Copyright (c) 2013 Apple Computer, Inc. All rights reserved.
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

#ifndef _MACH_VOUCHER_TYPES_H_
#define _MACH_VOUCHER_TYPES_H_

#include <mach/std_types.h>
#include <mach/port.h>

  
                                                                     
  
                                                                   
                                                                      
                       
  
                                                                 
                                                                          
                                                                        
                                                                         
                              
  
                                                                       
                                                                         
                                                                            
                                 
   
typedef mach_port_t             mach_voucher_t;
#define MACH_VOUCHER_NULL       ((mach_voucher_t) 0)

typedef mach_port_name_t        mach_voucher_name_t;
#define MACH_VOUCHER_NAME_NULL  ((mach_voucher_name_t) 0)

typedef mach_voucher_name_t     *mach_voucher_name_array_t;
#define MACH_VOUCHER_NAME_ARRAY_NULL ((mach_voucher_name_array_t) 0)

  
                                                                     
                                                                              
   
#if !defined(KERNEL)
typedef mach_voucher_t          ipc_voucher_t;
#else
#if !defined(MACH_KERNEL_PRIVATE)
struct ipc_voucher;
#endif
typedef struct ipc_voucher      *ipc_voucher_t;
#endif
#define IPC_VOUCHER_NULL        ((ipc_voucher_t) 0)

  
                                                                                       
                                                                    
                                                          
   
typedef uint32_t mach_voucher_selector_t;
#define MACH_VOUCHER_SELECTOR_CURRENT           ((mach_voucher_selector_t)0)
#define MACH_VOUCHER_SELECTOR_EFFECTIVE         ((mach_voucher_selector_t)1)


  
                                                                                      
                                                            
                        
   
typedef uint32_t mach_voucher_attr_key_t;
typedef mach_voucher_attr_key_t *mach_voucher_attr_key_array_t;

#define MACH_VOUCHER_ATTR_KEY_ALL               ((mach_voucher_attr_key_t)~0)
#define MACH_VOUCHER_ATTR_KEY_NONE              ((mach_voucher_attr_key_t)0)

                                              
#define MACH_VOUCHER_ATTR_KEY_ATM               ((mach_voucher_attr_key_t)1)
#define MACH_VOUCHER_ATTR_KEY_IMPORTANCE        ((mach_voucher_attr_key_t)2)
#define MACH_VOUCHER_ATTR_KEY_BANK              ((mach_voucher_attr_key_t)3)
#define MACH_VOUCHER_ATTR_KEY_PTHPRIORITY       ((mach_voucher_attr_key_t)4)

#define MACH_VOUCHER_ATTR_KEY_USER_DATA         ((mach_voucher_attr_key_t)7)
#define MACH_VOUCHER_ATTR_KEY_BITS              MACH_VOUCHER_ATTR_KEY_USER_DATA                 
#define MACH_VOUCHER_ATTR_KEY_TEST              ((mach_voucher_attr_key_t)8)

#define MACH_VOUCHER_ATTR_KEY_NUM_WELL_KNOWN    MACH_VOUCHER_ATTR_KEY_TEST

  
                              
  
                                                               
                                                               
                                                               
   
typedef uint8_t *mach_voucher_attr_content_t;
typedef uint32_t mach_voucher_attr_content_size_t;

  
                                                                              
                                                     
   
typedef uint32_t mach_voucher_attr_command_t;

  
                                     
  
                                                            
                                                                                   
                                                                                     
                                          
   
typedef uint32_t mach_voucher_attr_recipe_command_t;
typedef mach_voucher_attr_recipe_command_t *mach_voucher_attr_recipe_command_array_t;

#define MACH_VOUCHER_ATTR_NOOP                  ((mach_voucher_attr_recipe_command_t)0)
#define MACH_VOUCHER_ATTR_COPY                  ((mach_voucher_attr_recipe_command_t)1)
#define MACH_VOUCHER_ATTR_REMOVE                ((mach_voucher_attr_recipe_command_t)2)
#define MACH_VOUCHER_ATTR_SET_VALUE_HANDLE      ((mach_voucher_attr_recipe_command_t)3)
#define MACH_VOUCHER_ATTR_AUTO_REDEEM           ((mach_voucher_attr_recipe_command_t)4)
#define MACH_VOUCHER_ATTR_SEND_PREPROCESS       ((mach_voucher_attr_recipe_command_t)5)

                               
#define MACH_VOUCHER_ATTR_REDEEM                ((mach_voucher_attr_recipe_command_t)10)

                                                        
#define MACH_VOUCHER_ATTR_IMPORTANCE_SELF       ((mach_voucher_attr_recipe_command_t)200)

                                                       
#define MACH_VOUCHER_ATTR_USER_DATA_STORE       ((mach_voucher_attr_recipe_command_t)211)
#define MACH_VOUCHER_ATTR_BITS_STORE            MACH_VOUCHER_ATTR_USER_DATA_STORE                 

                                                  
#define MACH_VOUCHER_ATTR_TEST_STORE            MACH_VOUCHER_ATTR_USER_DATA_STORE

  
                             
  
                                                   
   
#pragma pack(push, 1)

typedef struct mach_voucher_attr_recipe_data {
	mach_voucher_attr_key_t                 key;
	mach_voucher_attr_recipe_command_t      command;
	mach_voucher_name_t                     previous_voucher;
	mach_voucher_attr_content_size_t        content_size;
	uint8_t                                 content[];
} mach_voucher_attr_recipe_data_t;
typedef mach_voucher_attr_recipe_data_t *mach_voucher_attr_recipe_t;
typedef mach_msg_type_number_t mach_voucher_attr_recipe_size_t;

                                     
typedef uint8_t *mach_voucher_attr_raw_recipe_t;
typedef mach_voucher_attr_raw_recipe_t mach_voucher_attr_raw_recipe_array_t;
typedef mach_msg_type_number_t mach_voucher_attr_raw_recipe_size_t;
typedef mach_msg_type_number_t mach_voucher_attr_raw_recipe_array_size_t;

#define MACH_VOUCHER_ATTR_MAX_RAW_RECIPE_ARRAY_SIZE   5120
#define MACH_VOUCHER_TRAP_STACK_LIMIT                 256

#pragma pack(pop)

  
                                         
   

  
                              
  
                                                                                  
                                               
   
typedef mach_port_t                     mach_voucher_attr_manager_t;
#define MACH_VOUCHER_ATTR_MANAGER_NULL  ((mach_voucher_attr_manager_t) 0)

  
                              
  
                                                                               
                                                                                        
   
typedef mach_port_t                     mach_voucher_attr_control_t;
#define MACH_VOUCHER_ATTR_CONTROL_NULL  ((mach_voucher_attr_control_t) 0)

  
                                                                                    
                                                                                 
                                           
   
#if !defined(KERNEL)
typedef mach_port_t             ipc_voucher_attr_manager_t;
typedef mach_port_t             ipc_voucher_attr_control_t;
#else
#if !defined(MACH_KERNEL_PRIVATE)
struct ipc_voucher_attr_manager;
struct ipc_voucher_attr_control;
#endif
typedef const struct ipc_voucher_attr_manager *ipc_voucher_attr_manager_t;
typedef struct ipc_voucher_attr_control *ipc_voucher_attr_control_t;
#endif
#define IPC_VOUCHER_ATTR_MANAGER_NULL ((ipc_voucher_attr_manager_t) 0)
#define IPC_VOUCHER_ATTR_CONTROL_NULL ((ipc_voucher_attr_control_t) 0)

  
                                   
  
                                                                    
                                                                      
   
typedef uint64_t mach_voucher_attr_value_handle_t;
typedef mach_voucher_attr_value_handle_t *mach_voucher_attr_value_handle_array_t;

typedef mach_msg_type_number_t mach_voucher_attr_value_handle_array_size_t;
#define MACH_VOUCHER_ATTR_VALUE_MAX_NESTED      ((mach_voucher_attr_value_handle_array_size_t)4)

typedef uint32_t mach_voucher_attr_value_reference_t;
typedef uint32_t mach_voucher_attr_value_flags_t;
#define MACH_VOUCHER_ATTR_VALUE_FLAGS_NONE      ((mach_voucher_attr_value_flags_t)0)
#define MACH_VOUCHER_ATTR_VALUE_FLAGS_PERSIST   ((mach_voucher_attr_value_flags_t)1)

               
typedef uint32_t mach_voucher_attr_control_flags_t;
#define MACH_VOUCHER_ATTR_CONTROL_FLAGS_NONE    ((mach_voucher_attr_control_flags_t)0)

  
                                                              
  
                                                                   
                                        
   
#define MACH_VOUCHER_IMPORTANCE_ATTR_ADD_EXTERNAL       1                                                        
#define MACH_VOUCHER_IMPORTANCE_ATTR_DROP_EXTERNAL      2                                         
typedef uint32_t mach_voucher_attr_importance_refs;

  
                                 
   
#define MACH_ACTIVITY_ID_COUNT_MAX 16

#endif                              
