/*
 * Copyright (c) 2015-2016 Apple Computer, Inc. All rights reserved.
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
  
                            
                      
                
  
                                                                 
                                                           
   

#ifndef _KERN_MACH_NODE_H_
#define _KERN_MACH_NODE_H_

#if defined(MACH_KERNEL_PRIVATE) || defined(__APPLE_API_PRIVATE)

                                  
                                                                   
                                    
   

                                                                         
                         
   
#pragma pack(4)
typedef struct mach_node_server_msg {
	mach_msg_header_t   header;
	uint32_t    identifier;                          
	uint32_t    options;                       
	uint32_t    node_id;                  
} *mach_node_server_msg_t;
#pragma pack()

                                                                              
                                      
   
typedef struct mach_node_server_register_msg {
	struct mach_node_server_msg node_header;
	uint8_t     datamodel;                                      
	uint8_t     byteorder;                                            
} *mach_node_server_register_msg_t;
#pragma pack()

#define MACH_NODE_SERVER_MSG_ID (0x45444f4eUL)                                 
#define MACH_NODE_SM_REG_LOCAL           (0UL)                            
#define MACH_NODE_SM_REG_REMOTE          (1UL)                           

#if defined(__LP64__)
#define LOCAL_DATA_MODEL    (2)                             
#else
#define LOCAL_DATA_MODEL    (1)                              
#endif

#endif


#if MACH_FLIPC && defined(MACH_KERNEL_PRIVATE)

#include <kern/mach_node_link.h>
#include <kern/queue.h>

#include <sys/cdefs.h>

__BEGIN_DECLS

#define MACH_NODES_MAX          (2)                                
#define MACH_NODE_ID_VALID(nid) (((nid) >= 0) && ((nid) < MACH_NODES_MAX))

typedef struct  flipc_node  *flipc_node_t;                           


                      
  
                                                                      
                                                                            
                                                                           
   
typedef struct mach_node *mach_node_t;

struct mach_node {
	                                                                      
	struct mnl_node_info info;

	lck_spin_t      node_lock_data;

	                           
	uint32_t        link:2;                             
	uint32_t        published:1;                                     
	uint32_t        active:1;                                  
	uint32_t        suspended:1;                                      
	uint32_t        dead:1;                            
	uint32_t        _reserved:26;                               

	                    
	ipc_space_t     proxy_space;                                        
	ipc_pset_t      proxy_port_set;                                  
	ipc_port_t      bootstrap_port;                                         
	ipc_port_t      control_port;                                 

	          
	int             proto_vers;                                         
	mach_node_t     antecedent;                                                
};

extern mach_node_t  localnode;                                       

#define MACH_NODE_NULL              ((mach_node_t) 0UL)
#define MACH_NODE_SIZE              ((vm_offset_t)sizeof(struct mach_node))
#define MACH_NODE_VALID(node)       ((node) != MACH_NODE_NULL)
#define MACH_NODE_ALLOC()           ((mach_node_t)kalloc(MACH_NODE_SIZE))
#define MACH_NODE_FREE(node)        kfree(node, MACH_NODE_SIZE)

#define MACH_NODE_LOCK_INIT(np)     lck_spin_init(&(np)->node_lock_data, \
	                                          &ipc_lck_grp, &ipc_lck_attr)
#define MACH_NODE_LOCK_DESTROY(np)  lck_spin_destroy(&(np)->node_lock_data, \
	                                             &ipc_lck_grp)
#define MACH_NODE_LOCK(np)          lck_spin_lock(&(np)->node_lock_data)
#define MACH_NODE_UNLOCK(np)        lck_spin_unlock(&(np)->node_lock_data)

                                                                            
                                                                               
                                                                             
                                                                            
                                                                           
                                                                              
                                                                  
   
mach_node_t
mach_node_for_id_locked(mach_node_id_t  node_id,
    boolean_t       alloc_if_dead,
    boolean_t       alloc_if_absent);


                                
  
                                                                             
                                                                             
                
  
                                                              
                                                                        
                                                      
   

                                             
                                         
                                     
   
extern mnl_name_t   mnl_name_alloc(void);

                                                                     
   
extern void mnl_name_free(mnl_name_t name);

                                                                     
   
#define MNL_NAME_BOOTSTRAP(nid) ((mnl_name_t) MACH_NODES_MAX | (nid))
#define MNL_NAME_NULL ((mnl_name_t) 0UL)
#define MNL_NAME_VALID(obj) ((obj) >= MACH_NODES_MAX)


                                                                               
                                                                            
                                                                         
                                                      
   
typedef struct mnl_obj {
	queue_chain_t   links;                                                     
	mnl_name_t      name;                  
} *mnl_obj_t;

#define MNL_OBJ_NULL        ((mnl_obj_t) 0UL)
#define MNL_OBJ_VALID(obj)  ((obj) != MNL_OBJ_NULL)


                                                                              
                                                                               
                               
   
void mnl_obj_init(mnl_obj_t obj);

                                                                       
                                                                
   
mnl_obj_t mnl_obj_lookup(mnl_name_t name);

                                                                       
                                                                               
                                                            
                                     
   
mnl_obj_t mnl_obj_remove(mnl_name_t name);

                                                                          
                                                                         
                                                                         
                                                                     
   
kern_return_t mnl_obj_insert(mnl_obj_t obj);


                                       
  
                                                           
                                                                 
                                                                  
  
                                                                         
                                                                             
                                                                          
                                                                           
                                                                         
                               
   


                         
   
#define MACH_NODE_SUB_INVALID   (0)              
#define MACH_NODE_SUB_NODE      (1)                                  
#define MACH_NODE_SUB_FLIPC     (2)                                  
#define MACH_NODE_SUB_VMSYS     (3)                               


                                                
   
void mach_node_port_changed(void);


__END_DECLS

#endif                                      
#endif                       
