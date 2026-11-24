/*
 * Copyright (c) 2015-2020 Apple Inc. All rights reserved.
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
                         
                      
              
  
                                        
                                                                         
   


#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>

#include <kern/kern_types.h>
#include <kern/assert.h>

#include <kern/host.h>
#include <kern/kalloc.h>
#include <kern/mach_node_link.h>
#include <kern/mach_node.h>
#include <kern/ipc_mig.h>                                                

#include <ipc/port.h>
#include <ipc/ipc_types.h>
#include <ipc/ipc_init.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_table.h>
#include <ipc/ipc_entry.h>

#include <ipc/flipc.h>

#include <libkern/OSAtomic.h>                                                 
#include <libkern/OSByteOrder.h>                        

#pragma pack(4)

#define MNL_NAME_TABLE_SIZE     (256)                                             
#define MNL_NAME_HASH(name)     (name % MNL_NAME_TABLE_SIZE)

                                         
mach_node_id_t                  localnode_id = -1;                              
#if MACH_FLIPC
mach_node_t                             localnode;                                                       


                                    
static int              mach_nodes_to_publish;
static mach_node_t      mach_node_table[MACH_NODES_MAX];
static LCK_SPIN_DECLARE_ATTR(mach_node_table_lock_data,
    &ipc_lck_grp, &ipc_lck_attr);
#define MACH_NODE_TABLE_LOCK()      lck_spin_lock(&mach_node_table_lock_data)
#define MACH_NODE_TABLE_UNLOCK()    lck_spin_unlock(&mach_node_table_lock_data)

static volatile SInt64  mnl_name_next;
static queue_head_t     mnl_name_table[MNL_NAME_TABLE_SIZE];
static LCK_SPIN_DECLARE_ATTR(mnl_name_table_lock_data,
    &ipc_lck_grp, &ipc_lck_attr);
#define MNL_NAME_TABLE_LOCK()       lck_spin_lock(&mnl_name_table_lock_data)
#define MNL_NAME_TABLE_UNLOCK()     lck_spin_unlock(&mnl_name_table_lock_data)

static void mach_node_init(void);
static void mnl_name_table_init(void);
static void mach_node_table_init(void);
static void mach_node_publish(mach_node_t node);

static mach_node_t mach_node_alloc_init(mach_node_id_t node_id);
static kern_return_t mach_node_register(mach_node_t node);


                                                                   
                                    
                                                                           
   
void
mach_node_init(void)
{
	mach_node_id_t node_id = 0;                                    
	if (OSCompareAndSwap((UInt32)(HOST_LOCAL_NODE),
	    (UInt32)node_id,
	    &localnode_id)) {
		printf("mach_node_init(): localnode_id=%d of %d\n",
		    localnode_id, MACH_NODES_MAX);
		mach_node_table_init();
		mnl_name_table_init();
	}                                                                  
}

void
mach_node_table_init(void)
{
	MACH_NODE_TABLE_LOCK();

	                                     
	bzero(mach_node_table, sizeof(mach_node_t) * MACH_NODES_MAX);
	mach_nodes_to_publish = 0;

	                                 
	localnode = mach_node_for_id_locked(localnode_id, 1, 1);
	assert(MACH_NODE_VALID(localnode));

	MACH_NODE_TABLE_UNLOCK();

	                               
	bzero(localnode, sizeof(*localnode));
	localnode->info.datamodel       = LOCAL_DATA_MODEL;
	localnode->info.byteorder       = OSHostByteOrder();
	localnode->info.proto_vers_min      = MNL_PROTOCOL_V1;
	localnode->info.proto_vers_max      = MNL_PROTOCOL_V1;
	localnode->proto_vers           = MNL_PROTOCOL_V1;
	localnode->published            = 0;
	localnode->active               = 1;

	MACH_NODE_UNLOCK(localnode);
}

                                                                     
                                                                             
                                                                    
  
                                                        
   

void
mach_node_publish(mach_node_t node)
{
	kern_return_t kr;

	if (!MACH_NODE_VALID(node) || (!node->active) || (node->published)) {
		return;                                                   
	}
	ipc_port_t bs_port = localnode->bootstrap_port;
	if (!IP_VALID(bs_port)) {
		return;                                  
	}
	                                                                          
	struct mach_node_server_register_msg   msg;

	msg.node_header.header.msgh_remote_port = bs_port;
	msg.node_header.header.msgh_size = sizeof(msg);
	msg.node_header.header.msgh_local_port = MACH_PORT_NULL;
	msg.node_header.header.msgh_voucher_port = MACH_PORT_NULL;
	msg.node_header.header.msgh_id = MACH_NODE_SERVER_MSG_ID;
	msg.node_header.node_id = node->info.node_id;
	msg.node_header.options = 0;
	msg.datamodel = node->info.datamodel;
	msg.byteorder = node->info.byteorder;

	if (node == localnode) {
		msg.node_header.identifier = MACH_NODE_SM_REG_LOCAL;
		msg.node_header.header.msgh_bits =
		    MACH_MSGH_BITS_SET(MACH_MSG_TYPE_COPY_SEND, 0, 0, 0);
	} else {
		msg.node_header.identifier = MACH_NODE_SM_REG_REMOTE;
		msg.node_header.header.msgh_local_port = node->bootstrap_port;
		msg.node_header.header.msgh_bits = MACH_MSGH_BITS_SET
		    (MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND, 0, 0);
	}

	kr = mach_msg_send_from_kernel_proper(&msg.node_header.header,
	    sizeof(msg));
	if (kr == KERN_SUCCESS) {
		node->published = 1;
		mach_nodes_to_publish--;
	}
	printf("mach_node_publish(%d)=%d\n", node->info.node_id, kr);
}

                                                   
void
mach_node_port_changed(void)
{
	ipc_port_t bs_port;

	mach_node_init();                                

	                                                  
	MACH_NODE_LOCK(localnode);
	flipc_node_retire(localnode);
	bs_port = localnode->bootstrap_port;
	if (IP_VALID(bs_port)) {
		localnode->bootstrap_port = IP_NULL;
		                                               
	}

	kernel_get_special_port(host_priv_self(), HOST_NODE_PORT, &bs_port);
	assert(IP_VALID(bs_port));
	localnode->bootstrap_port = bs_port;
	flipc_node_prepare(localnode);
	MACH_NODE_UNLOCK(localnode);

	                                                             
	MACH_NODE_TABLE_LOCK();
	                                                       
	localnode->active = 1;

	mach_nodes_to_publish = 0;

	int n;
	for (n = 0; n < MACH_NODES_MAX; n++) {
		mach_node_t np = mach_node_table[n];
		                                                   
		if (!MACH_NODE_VALID(np)) {
			continue;
		}
		np->published = 0;
		if (np->active == 1) {
			mach_nodes_to_publish++;
		}
	}

	mach_node_publish(localnode);                                   

	for (n = 0; n < MACH_NODES_MAX; n++) {
		mach_node_publish(mach_node_table[n]);
	}

	MACH_NODE_TABLE_UNLOCK();

	                                                    
}

                                                                   
                                                             
   
mach_node_t
mach_node_alloc_init(mach_node_id_t node_id)
{
	mach_node_t node = MACH_NODE_ALLOC();
	if (MACH_NODE_VALID(node)) {
		bzero(node, sizeof(struct mach_node));
		MACH_NODE_LOCK_INIT(node);
		node->info.node_id = node_id;
	}
	return node;
}


                                                                          
                                                                           
   
kern_return_t
mach_node_register(mach_node_t  node)
{
	assert(MACH_NODE_VALID(node));
	mach_node_id_t nid = node->info.node_id;
	assert(MACH_NODE_ID_VALID(nid));

	kern_return_t kr;
	ipc_space_t proxy_space = IS_NULL;
	ipc_pset_t  pp_set = IPS_NULL;                             
	ipc_port_t  bs_port = MACH_PORT_NULL;
	ipc_port_t  ack_port = MACH_PORT_NULL;

	printf("mach_node_register(%d)\n", nid);

	                                                         
	if ((node->info.byteorder != OSHostByteOrder()) ||
	    (node->info.datamodel != LOCAL_DATA_MODEL)) {
		printf("mach_node_register: unsupported byte order (%d) or width (%d)",
		    node->info.byteorder, node->info.datamodel);
		return KERN_INVALID_ARGUMENT;
	}

	                                                                    
	kr = ipc_space_create_special(&proxy_space);
	if (kr != KERN_SUCCESS) {
		goto out;
	}
	proxy_space->is_node_id = nid;

	                                                          
	bs_port = ipc_port_alloc_special(proxy_space, IPC_PORT_INIT_MESSAGE_QUEUE);
	if (bs_port == MACH_PORT_NULL) {
		kr = KERN_RESOURCE_SHORTAGE;
		goto out;
	}

	                                                        
	ack_port = ipc_port_alloc_special(proxy_space, IPC_PORT_INIT_MESSAGE_QUEUE);
	if (ack_port == MACH_PORT_NULL) {
		kr = KERN_RESOURCE_SHORTAGE;
		goto out;
	}

	                                                                    
	pp_set = ipc_pset_alloc_special(proxy_space);
	if (pp_set == IPS_NULL) {
		kr = KERN_RESOURCE_SHORTAGE;
		goto out;
	}

	waitq_set_lazy_init_link(pp_set);
	                                                  
	uint64_t wq_link_id = waitq_link_reserve(NULL);
	uint64_t wq_reserved_prepost = waitq_prepost_reserve(NULL, 10,
	    WAITQ_DONT_LOCK);
	ips_lock(pp_set);
	ip_lock(bs_port);
	ipc_pset_add(pp_set,
	    bs_port,
	    &wq_link_id,
	    &wq_reserved_prepost);
	ip_unlock(bs_port);
	ips_unlock(pp_set);

	waitq_link_release(wq_link_id);
	waitq_prepost_release_reserve(wq_reserved_prepost);

	                                                
	wq_link_id = waitq_link_reserve(NULL);
	wq_reserved_prepost = waitq_prepost_reserve(NULL, 10,
	    WAITQ_DONT_LOCK);
	ips_lock(pp_set);
	ip_lock(ack_port);
	ipc_pset_add(pp_set,
	    ack_port,
	    &wq_link_id,
	    &wq_reserved_prepost);
	ip_unlock(ack_port);
	ips_unlock(pp_set);

	waitq_link_release(wq_link_id);
	waitq_prepost_release_reserve(wq_reserved_prepost);

	                         
	node->published         = 0;
	node->active                        = 1;
	node->proxy_space           = proxy_space;
	node->proxy_port_set        = pp_set;
	node->bootstrap_port        = bs_port;
	node->proto_vers        = node->info.proto_vers_max;
	node->control_port      = ack_port;

	                                             
	MACH_NODE_TABLE_LOCK();

	mach_node_t old_node = mach_node_table[nid];
	if (!MACH_NODE_VALID(old_node) || (old_node->dead)) {
		node->antecedent = old_node;
		flipc_node_prepare(node);
		mach_node_table[nid] = node;
		mach_nodes_to_publish++;
		mach_node_publish(node);
		kr = KERN_SUCCESS;
	} else {
		printf("mach_node_register: id %d already active!", nid);
		kr = KERN_FAILURE;
	}
	MACH_NODE_TABLE_UNLOCK();

out:
	if (kr != KERN_SUCCESS) {                                    
		if (pp_set) {
			ips_lock(pp_set);
			ipc_pset_destroy(proxy_space, pp_set);
		}

		if (bs_port) {
			ipc_port_dealloc_special(bs_port, proxy_space);
		}

		if (ack_port) {
			ipc_port_dealloc_special(ack_port, proxy_space);
		}

		if (proxy_space) {
			ipc_space_terminate(proxy_space);
		}
	}

	return kr;
}


                                                                           
                                                                               
                                                                             
                                                                            
                                                                           
                                                                              
                                                                  
  
                                                                      
   
mach_node_t
mach_node_for_id_locked(mach_node_id_t  node_id,
    boolean_t               alloc_if_dead,
    boolean_t               alloc_if_absent)
{
	if ((node_id < 0) || (node_id >= MACH_NODES_MAX)) {
		return MACH_NODE_NULL;
	}

	mach_node_t node = mach_node_table[node_id];

	if ((!MACH_NODE_VALID(node) && alloc_if_absent) ||
	    (MACH_NODE_VALID(node) && node->dead && alloc_if_dead)) {
		node = mach_node_alloc_init(node_id);
		if (MACH_NODE_VALID(node)) {
			node->antecedent = mach_node_table[node_id];
			mach_node_table[node_id] = node;
		}
	}

	if (MACH_NODE_VALID(node)) {
		MACH_NODE_LOCK(node);
	}

	return node;
}



                                                           

                                            
                                         
                                     
   
mnl_name_t
mnl_name_alloc(void)
{
	return (mnl_name_t)OSAddAtomic64(MACH_NODES_MAX, &mnl_name_next);
}


                                                                    
   
void
mnl_name_free(mnl_name_t name __unused)
{
	;                                                                 
}


                                                                              
   
void
mnl_name_table_init(void)
{
	MNL_NAME_TABLE_LOCK();

	                                                   
	mnl_name_next = localnode_id + MACH_NODES_MAX;

	for (int i = 0; i < MNL_NAME_TABLE_SIZE; i++) {
		queue_head_init(mnl_name_table[i]);
	}

	MNL_NAME_TABLE_UNLOCK();
}


                                                                             
                                                                               
                               
   
void
mnl_obj_init(mnl_obj_t obj)
{
	queue_chain_init(obj->links);
	obj->name = MNL_NAME_NULL;
}


                                                                      
                                                                
   
mnl_obj_t
mnl_obj_lookup(mnl_name_t name)
{
	mnl_obj_t obj = MNL_OBJ_NULL;

	if (name != MNL_NAME_NULL) {
		qe_foreach_element(obj, &mnl_name_table[MNL_NAME_HASH(name)], links) {
			if (obj->name == name) {
				break;
			}
		}
	}
	return obj;
}


                                                                      
                                                                               
                                                            
                                     
   
mnl_obj_t
mnl_obj_remove(mnl_name_t name)
{
	mnl_obj_t obj = MNL_OBJ_NULL;

	if (name != MNL_NAME_NULL) {
		qe_foreach_element_safe(obj, &mnl_name_table[MNL_NAME_HASH(name)], links) {
			if (obj->name == name) {
				remqueue(&obj->links);
			}
		}
	}
	return obj;
}


                                                                         
                                                                         
                           
                                                           
                                                       
                                                                           
   
kern_return_t
mnl_obj_insert(mnl_obj_t obj)
{
	if (!MNL_OBJ_VALID(obj)) {
		return KERN_INVALID_ARGUMENT;
	}

	MNL_NAME_TABLE_LOCK();

	if (!MNL_NAME_VALID(obj->name)) {
		                                                
		obj->name = mnl_name_alloc();
	}

	enqueue(&mnl_name_table[MNL_NAME_HASH(obj->name)], &obj->links);
	MNL_NAME_TABLE_UNLOCK();

	if (obj->name >= (MACH_NODES_MAX << 1)) {
		panic("Unexpected MNL_NAME %lld in obj %p", obj->name, obj);
	}

	return KERN_SUCCESS;
}


                                                        

                                                                            
                                                                          
                         
  
              
                                                                          
                                                    
  
                  
                                         
                                                                       
   
mnl_msg_t
mnl_msg_alloc(int       payload,
    uint32_t  flags   __unused)
{
	mnl_msg_t msg = kalloc(MNL_MSG_SIZE + payload);

	if (MNL_MSG_VALID(msg)) {
		bzero(msg, MNL_MSG_SIZE);                        
		msg->size = payload;
	}

	return msg;
}


                                                       
  
              
                                                         
                                                    
   
void
mnl_msg_free(mnl_msg_t  msg,
    uint32_t   flags   __unused)
{
	if (MNL_MSG_VALID(msg)) {
		kfree(msg, MNL_MSG_SIZE + msg->size);
	}
}


                                                                             
                                                                          
                                                                             
                                                                           
                                                                             
  
              
                                                     
                                                    
  
                  
                                        
                                                        
   
mnl_node_info_t
mnl_instantiate(mach_node_id_t  nid,
    uint32_t        flags   __unused)
{
	mach_node_init();                                

	if ((nid == localnode_id) || !MACH_NODE_ID_VALID(nid)) {
		return MNL_NODE_NULL;
	}

	return (mnl_node_info_t)mach_node_alloc_init(nid);
}

                                                                          
                                                                           
                         
  
              
                                                        
                                                    
  
                  
                                                    
                                                                              
                                                                       
   
kern_return_t
mnl_register(mnl_node_info_t    node,
    uint32_t           flags   __unused)
{
	if (MNL_NODE_VALID(node) && (node->node_id != localnode_id)) {
		return mach_node_register((mach_node_t)node);
	}

	return KERN_INVALID_ARGUMENT;
}


                                                                             
                                                                               
                                                                            
                                                                            
                                                      
  
              
                                                        
                                                                       
                                                    
  
                  
                                                              
                                                                
                                                                        
   
kern_return_t
mnl_set_link_state(mnl_node_info_t  node,
    int              link,
    uint32_t         flags   __unused)
{
	kern_return_t kr;
	mach_node_t mnode = (mach_node_t)node;

	if (!MACH_NODE_VALID(mnode) || !(link & MNL_LINK_UP) || (link & mnode->link)) {
		return KERN_INVALID_ARGUMENT;                                    
	}
	MACH_NODE_LOCK(mnode);

	if (mnode->dead) {
		kr = KERN_NODE_DOWN;
	} else {
		mnode->link |= link;
		kr = KERN_SUCCESS;
	}

	MACH_NODE_UNLOCK(mnode);

	return kr;
}

                                                                          
                                                                             
                                                                              
                                                                          
                                                                           
                                                                           
                                              
  
              
                                                        
                                                    
  
                  
                                                  
                                                                  
                                                                        
   
kern_return_t
mnl_terminate(mnl_node_info_t   node,
    uint32_t          flags   __unused)
{
	kern_return_t kr = KERN_SUCCESS;
	mach_node_t mnode = (mach_node_t)node;

	if (!MACH_NODE_VALID(mnode)) {
		return KERN_INVALID_ARGUMENT;              
	}
	MACH_NODE_LOCK(mnode);
	if (mnode->dead) {
		kr = KERN_NODE_DOWN;                                                 
		goto unlock;
	}

	mnode->link = MNL_LINK_DOWN;
	mnode->active = 0;
	mnode->suspended = 0;
	mnode->dead = 1;

	flipc_node_retire(mnode);

	                                                  
	if (mnode->proxy_port_set != IPS_NULL) {
		ips_lock(mnode->proxy_port_set);
		ipc_pset_destroy(mnode->proxy_space, mnode->proxy_port_set);
		mnode->proxy_port_set = IPS_NULL;
	}

	                                                               

unlock:
	MACH_NODE_UNLOCK(mnode);
	return kr;
}


                                                                             
                                                                        
                          
  
              
                                                        
                                             
                                                    
   
void
mnl_msg_from_node(mnl_node_info_t   node    __unused,
    mnl_msg_t         msg,
    uint32_t          flags   __unused)
{
	assert(MNL_MSG_VALID(msg));
	assert(MACH_NODE_ID_VALID(msg->node_id));
	assert(MNL_NODE_VALID(node));

	                                                                         
                                                                             
                                                                               
                                                                          
    

	switch (msg->sub) {
	case MACH_NODE_SUB_FLIPC:
		flipc_msg_from_node((mach_node_t)node, msg, flags);
		break;

	default:
#if DEBUG
		PE_enter_debugger("mnl_msg_from_node(): Invalid subsystem");
#endif
		break;
	}
}


                                                                     
                                                                         
                                                                         
                                                                            
                            
  
              
                                                        
                                                    
   
mnl_msg_t
mnl_msg_to_node(mnl_node_info_t node    __unused,
    uint32_t        flags   __unused)
{
	assert(MNL_NODE_VALID(node));

#if DEBUG
	thread_set_thread_name(current_thread(), "MNL_Link");
#endif

	return flipc_msg_to_remote_node((mach_node_t)node, 0);
}


                                                                            
                                                   
  
              
                                                        
                                             
                                                    
   
void
mnl_msg_complete(mnl_node_info_t    node    __unused,
    mnl_msg_t          msg,
    uint32_t           flags)
{
	switch (msg->sub) {
	case MACH_NODE_SUB_NODE:
		mnl_msg_free(msg, flags);
		break;

	case MACH_NODE_SUB_FLIPC:
		flipc_msg_free(msg, flags);
		break;

	default:
#if DEBUG
		PE_enter_debugger("mnl_msg_complete(): Invalid subsystem");
#endif
		break;
	}
}

#else                                                   

mnl_msg_t
mnl_msg_alloc(int payload __unused, uint32_t flags __unused)
{
	return MNL_MSG_NULL;
}

void
mnl_msg_free(mnl_msg_t msg __unused, uint32_t flags __unused)
{
	return;
}

mnl_node_info_t
mnl_instantiate(mach_node_id_t nid __unused, uint32_t flags __unused)
{
	return MNL_NODE_NULL;
}

kern_return_t
mnl_register(mnl_node_info_t node  __unused, uint32_t flags __unused)
{
	return KERN_FAILURE;
}

kern_return_t
mnl_set_link_state(mnl_node_info_t  node    __unused,
    int              link    __unused,
    uint32_t         flags   __unused)
{
	return KERN_FAILURE;
}

kern_return_t
mnl_terminate(mnl_node_info_t node __unused, uint32_t flags __unused)
{
	return KERN_FAILURE;
}

void
mnl_msg_from_node(mnl_node_info_t   node    __unused,
    mnl_msg_t         msg     __unused,
    uint32_t          flags   __unused)
{
	return;
}

mnl_msg_t
mnl_msg_to_node(mnl_node_info_t node __unused, uint32_t flags __unused)
{
	return MNL_MSG_NULL;
}

void
mnl_msg_complete(mnl_node_info_t    node    __unused,
    mnl_msg_t          msg     __unused,
    uint32_t           flags   __unused)
{
	return;
}

#endif              
