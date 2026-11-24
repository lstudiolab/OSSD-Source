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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
                    
                     
             
  
                                            
   


#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>

#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/host.h>
#include <kern/kalloc.h>
#include <kern/mach_node.h>

#include <ipc/port.h>
#include <ipc/ipc_types.h>
#include <ipc/ipc_init.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_table.h>
#include <ipc/ipc_entry.h>
#include <ipc/flipc.h>

#pragma pack(4)


                                                            

ZONE_DECLARE(flipc_port_zone, "flipc ports",
    sizeof(struct flipc_port), ZC_NOENCRYPT);

                                                            
                                                                    
   
static inline mnl_name_t
mnl_name_from_port(ipc_port_t lport)
{
	mnl_name_t name = MNL_NAME_NULL;

	if (IP_VALID(lport)) {
		flipc_port_t fport = lport->ip_messages.data.port.fport;
		if (FPORT_VALID(fport)) {
			name = fport->obj.name;
		}
	}
	return name;
}


                                                        
                                                                   
   
static inline ipc_port_t
mnl_name_to_port(mnl_name_t name)
{
	ipc_port_t lport = IP_NULL;

	if (MNL_NAME_VALID(name)) {
		flipc_port_t fport = (flipc_port_t)mnl_obj_lookup(name);
		if (FPORT_VALID(fport)) {
			lport = fport->lport;
		}
	}
	return lport;
}


                                                                       
                                                                
                                                                  
   
static kern_return_t
flipc_port_create(ipc_port_t lport, mach_node_t node, mnl_name_t name)
{
	                                                        
	assert(IP_VALID(lport));
	assert(MACH_NODE_VALID(node));
	assert(MNL_NAME_VALID(name));
	assert(!FPORT_VALID(lport->ip_messages.imq_fport));

	                                          
	flipc_port_t fport = (flipc_port_t) zalloc(flipc_port_zone);
	if (!FPORT_VALID(fport)) {
		return KERN_RESOURCE_SHORTAGE;
	}
	bzero(fport, sizeof(struct flipc_port));
	fport->obj.name = name;
	fport->hostnode = node;
	if (node == localnode) {
		fport->state = FPORT_STATE_PRINCIPAL;
	} else {
		fport->state = FPORT_STATE_PROXY;
	}

	                                          
	fport->lport = lport;
	lport->ip_messages.imq_fport = fport;

	                                                                   
	kern_return_t kr =  mnl_obj_insert((mnl_obj_t)fport);
	if (kr != KERN_SUCCESS) {
		lport->ip_messages.imq_fport = FPORT_NULL;
		fport->lport = IP_NULL;
		zfree(flipc_port_zone, fport);
	}

	return kr;
}


                                                                    
                                                                          
                                                                           
                                                                           
   
static void
flipc_port_destroy(ipc_port_t lport)
{
	                                                                         
	assert(IP_VALID(lport));
	ipc_mqueue_t port_mq = &lport->ip_messages;
	flipc_port_t fport = port_mq->data.port.fport;
	assert(FPORT_VALID(fport));
	assert(MNL_NAME_VALID(fport->obj.name));

	                                         
	int m = port_mq->data.port.msgcount;
	if (m > 0) {
		ipc_kmsg_t kmsg;
#if DEBUG
		printf("flipc: destroying %p with %d undelivered msgs\n", lport, m);
#endif

		                                                         
		while (m--) {
			kmsg = ipc_kmsg_queue_first(&port_mq->imq_messages);
			assert(kmsg != IKM_NULL);
			ipc_kmsg_rmqueue(&port_mq->imq_messages, kmsg);
			if (fport->state == FPORT_STATE_PRINCIPAL) {
				flipc_msg_ack(kmsg->ikm_node, port_mq, FALSE);
			}
			ipc_mqueue_release_msgcount(port_mq, NULL);
			port_mq->imq_seqno++;
		}
	}

	                                                                       
	mnl_obj_remove(fport->obj.name);
	lport->ip_messages.data.port.fport = FPORT_NULL;
	fport->lport = IP_NULL;
	zfree(flipc_port_zone, fport);
}


  
                                                     
           
                                                                      
                                                                             
                                                                          
                                                    
              
                   
           
                                                                
   
static mach_msg_size_t
flipc_msg_size_from_kmsg(ipc_kmsg_t kmsg)
{
	mach_msg_size_t fsize = kmsg->ikm_header->msgh_size;

	if (kmsg->ikm_header->msgh_bits & MACH_MSGH_BITS_COMPLEX) {
		PE_enter_debugger("flipc_msg_size_from_kmsg(): Complex messages not supported.");
	}

	return fsize;
}


                                                                             
                                                                              
                                                                         
                                                             
   
static kern_return_t
mnl_msg_from_kmsg(ipc_kmsg_t kmsg, mnl_msg_t *fmsgp)
{
	if (kmsg->ikm_header->msgh_bits & MACH_MSGH_BITS_COMPLEX) {
		printf("mnl_msg_from_kmsg(): Complex messages not supported.");
		return KERN_FAILURE;
	}

	mach_msg_size_t fsize = flipc_msg_size_from_kmsg(kmsg);

	mnl_msg_t fmsg = mnl_msg_alloc(fsize, 0);

	if (fmsg == MNL_MSG_NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}

	                                
	fmsg->sub = MACH_NODE_SUB_FLIPC;
	fmsg->cmd = FLIPC_CMD_IPCMESSAGE;
	fmsg->node_id = localnode_id;                        
	fmsg->qos = 0;            
	fmsg->size = fsize;                                                  
	fmsg->object = kmsg->ikm_header->msgh_remote_port->ip_messages.data.port.fport->obj.name;

	                          
	bcopy((const void*)kmsg->ikm_header, (void*)MNL_MSG_PAYLOAD(fmsg), fsize);

	                      
	mach_msg_header_t *mmsg = (mach_msg_header_t*)MNL_MSG_PAYLOAD(fmsg);
	mmsg->msgh_remote_port = (mach_port_t)fmsg->object;
	mmsg->msgh_local_port = (mach_port_t)
	    mnl_name_from_port(mmsg->msgh_local_port);
	mmsg->msgh_voucher_port = (mach_port_name_t)MNL_NAME_NULL;

	*fmsgp = (mnl_msg_t)fmsg;

	return KERN_SUCCESS;
}


                                                              
static mach_msg_return_t
mach_msg_send_from_remote_kernel(mach_msg_header_t      *msg,
    mach_msg_size_t        send_size,
    mach_node_t            node)
{
	ipc_kmsg_t kmsg;
	mach_msg_return_t mr;

	mr = ipc_kmsg_get_from_kernel(msg, send_size, &kmsg);
	if (mr != MACH_MSG_SUCCESS) {
		return mr;
	}

	mr = ipc_kmsg_copyin_from_kernel(kmsg);
	if (mr != MACH_MSG_SUCCESS) {
		ipc_kmsg_free(kmsg);
		return mr;
	}

	kmsg->ikm_node = node;                                           
	mr = ipc_kmsg_send(kmsg,
	    MACH_SEND_KERNEL_DEFAULT,
	    MACH_MSG_TIMEOUT_NONE);
	if (mr != MACH_MSG_SUCCESS) {
		ipc_kmsg_destroy(kmsg);
	}

	return mr;
}


                                                                           
                                                                             
                                                                            
   
static mach_msg_return_t
flipc_cmd_ipc(mnl_msg_t     fmsg,
    mach_node_t   node,
    uint32_t      flags   __unused)
{
	mach_msg_header_t *mmsg;

	                                                                       
	mmsg = (mach_msg_header_t*)MNL_MSG_PAYLOAD(fmsg);
	mmsg->msgh_size = fmsg->size;
	mmsg->msgh_remote_port = mnl_name_to_port(fmsg->object);
	mmsg->msgh_local_port = mnl_name_to_port((mnl_name_t)mmsg->msgh_local_port);
	mmsg->msgh_voucher_port = (mach_port_name_t)MACH_PORT_NULL;
	mmsg->msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
	                     

	return mach_msg_send_from_remote_kernel(mmsg, fmsg->size, node);
}


                                                                  
                                                                      
                                                                               
   
static void
flipc_cmd_ack(flipc_ack_msg_t   fmsg,
    mach_node_t       node    __unused,
    uint32_t          flags   __unused)
{
	unsigned int msg_count = fmsg->msg_count;
	thread_t thread = current_thread();
	boolean_t kick = FALSE;

	flipc_port_t fport = (flipc_port_t)mnl_obj_lookup(fmsg->mnl.object);

	ipc_port_t lport = fport->lport;
	ip_lock(lport);

	ipc_mqueue_t lport_mq = &lport->ip_messages;
	imq_lock(lport_mq);

	assert(fport->peek_count >= msg_count);                                     

	while (msg_count--) {
		ipc_mqueue_select_on_thread(lport_mq, IMQ_NULL, 0, 0, thread);
		fport->peek_count--;
		kick |= ipc_kmsg_delayed_destroy(thread->ith_kmsg);
	}

	imq_unlock(lport_mq);
	ip_unlock(lport);

	if (kick) {
		ipc_kmsg_reap_delayed();
	}
}



                                                                    


                                                                           
                                                                           
                                                                             
                                                                            
                                                         
                                                                    
   
kern_return_t
flipc_node_prepare(mach_node_t node)
{
	kern_return_t kr;

	assert(MACH_NODE_VALID(node));
	ipc_port_t bs_port = node->bootstrap_port;
	assert(IP_VALID(bs_port));

	ip_lock(bs_port);

	kr = flipc_port_create(bs_port,
	    node,
	    MNL_NAME_BOOTSTRAP(node->info.node_id));
	ip_unlock(bs_port);

	return kr;
}


                                                                          
                                                                        
                                                                            
                                                                             
                                        
                                    
   
kern_return_t
flipc_node_retire(mach_node_t node)
{
	if (!MACH_NODE_VALID(node)) {
		return KERN_NODE_DOWN;
	}

	ipc_port_t bs_port = node->bootstrap_port;
	if (IP_VALID(bs_port)) {
		ip_lock(bs_port);
		flipc_port_destroy(bs_port);
		ip_unlock(bs_port);
	}

	return KERN_SUCCESS;
}


                                                             


                                                                             
                                                                             
                                                              
   
mnl_msg_t
flipc_msg_to_remote_node(mach_node_t  to_node,
    uint32_t     flags __unused)
{
	mach_port_seqno_t msgoff;
	ipc_kmsg_t kmsg = IKM_NULL;
	mnl_msg_t fmsg = MNL_MSG_NULL;

	assert(to_node != localnode);
	assert(get_preemption_level() == 0);

	ipc_mqueue_t portset_mq = &to_node->proxy_port_set->ips_messages;
	ipc_mqueue_t port_mq = IMQ_NULL;

	while (!to_node->dead) {
		                                        
		ipc_mqueue_receive(portset_mq, MACH_PEEK_MSG, 0, 0, THREAD_ABORTSAFE);

		thread_t thread = current_thread();
		if (thread->ith_state == MACH_PEEK_READY) {
			port_mq = thread->ith_peekq;
			thread->ith_peekq = IMQ_NULL;
		} else {
			panic("Unexpected thread state %d after ipc_mqueue_receive()",
			    thread->ith_state);
		}

		assert(get_preemption_level() == 0);
		imq_lock(port_mq);

		flipc_port_t fport = port_mq->data.port.fport;

		if (FPORT_VALID(fport)) {
			msgoff = port_mq->data.port.fport->peek_count;

			ipc_mqueue_peek_locked(port_mq, &msgoff, NULL, NULL, NULL, &kmsg);
			if (kmsg != IKM_NULL) {
				port_mq->data.port.fport->peek_count++;
			}

			                                           
                                
      
			ipc_mqueue_release_peek_ref(port_mq);
			assert(get_preemption_level() == 0);

			                                                                  
                                                                      
                                                                  
                                                              
      
			if (kmsg != IKM_NULL) {
				mnl_msg_from_kmsg(kmsg, (mnl_msg_t*)&fmsg);
			}
		} else {
			                                                              
			assert(!FPORT_VALID(port_mq->data.port.fport));

			                                                                
			kmsg = ipc_kmsg_queue_first(&port_mq->imq_messages);
			assert(kmsg != IKM_NULL);
			ipc_kmsg_rmqueue(&port_mq->imq_messages, kmsg);
			ipc_mqueue_release_msgcount(port_mq, portset_mq);
			imq_unlock(port_mq);
			current_task()->messages_received++;
			ip_release(to_node->control_port);                                  

			                                             
                                                              
      
			mach_msg_header_t *hdr = kmsg->ikm_header;
			fmsg = (mnl_msg_t)(&hdr[1]);
			                                         
			*(ipc_kmsg_t*)((vm_offset_t)fmsg - sizeof(vm_offset_t)) = kmsg;
		}

		if (MNL_MSG_VALID(fmsg)) {
			break;
		}
	}
	assert(MNL_MSG_VALID(fmsg));
	return fmsg;
}


                                                                             
                                                                             
           
   
void
flipc_msg_from_node(mach_node_t from_node   __unused,
    mnl_msg_t   msg,
    uint32_t    flags)
{
	                                                                         
                                                                       
                                                                    
                                                                       
    
	assert(msg->sub == MACH_NODE_SUB_FLIPC);
	mach_node_t node = mach_node_for_id_locked(msg->node_id, FALSE, FALSE);
	MACH_NODE_UNLOCK(node);

	switch (msg->cmd) {
	case FLIPC_CMD_IPCMESSAGE:
		flipc_cmd_ipc(msg, node, flags);
		break;

	case FLIPC_CMD_ACKMESSAGE:
	case FLIPC_CMD_NAKMESSAGE:
		flipc_cmd_ack((flipc_ack_msg_t)msg, node, flags);
		break;

	default:
#if DEBUG
		PE_enter_debugger("flipc_incoming(): Invalid command");
#endif
		break;
	}
}


                                                                          
                                                                           
                                                                        
                                                   
   
void
flipc_msg_free(mnl_msg_t    msg,
    uint32_t     flags)
{
	switch (msg->cmd) {
	case FLIPC_CMD_ACKMESSAGE:                                       
	case FLIPC_CMD_NAKMESSAGE:                                      
		ipc_kmsg_free(*(ipc_kmsg_t*)((vm_offset_t)msg - sizeof(vm_offset_t)));
		break;

	default:                                                               
		mnl_msg_free(msg, flags);
		break;
	}
}


                                                                

                                                                           
                                                                              
                                                                            
                       
  
                                                                            
                                          
  
                                                                             
   
void
flipc_msg_ack(mach_node_t   node,
    ipc_mqueue_t  mqueue,
    boolean_t     delivered)
{
	flipc_port_t fport = mqueue->imq_fport;

	assert(FPORT_VALID(fport));
	assert(MACH_NODE_VALID(node));

	mnl_name_t name = MNL_NAME_NULL;
	mach_node_id_t nid = HOST_LOCAL_NODE;
	ipc_port_t ack_port = IP_NULL;

	ip_lock(fport->lport);
	name = fport->obj.name;
	ip_unlock(fport->lport);

	if (!MNL_NAME_VALID(name)) {
		return;
	}

	MACH_NODE_LOCK(node);
	if (node->active) {
		nid = node->info.node_id;
		ack_port = node->control_port;
	}
	MACH_NODE_UNLOCK(node);

	if (!IP_VALID(ack_port) || !MACH_NODE_ID_VALID(nid)) {
		return;
	}

	                                                                        
	ipc_kmsg_t kmsg = ipc_kmsg_alloc(sizeof(struct flipc_ack_msg) + MAX_TRAILER_SIZE);
	assert((unsigned long long)kmsg >= 4ULL);               
	mach_msg_header_t *msg = kmsg->ikm_header;

	                                        
	msg->msgh_bits = MACH_MSGH_BITS_SET(0, 0, 0, 0);
	msg->msgh_size = sizeof(msg);
	msg->msgh_remote_port = ack_port;
	msg->msgh_local_port = MACH_PORT_NULL;
	msg->msgh_voucher_port = MACH_PORT_NULL;
	msg->msgh_id = FLIPC_CMD_ID;

	                                      
	flipc_ack_msg_t fmsg = (flipc_ack_msg_t)(&msg[1]);
	fmsg->resend_to = HOST_LOCAL_NODE;
	fmsg->msg_count = 1;                                                   

	                                
	fmsg->mnl.sub = MACH_NODE_SUB_FLIPC;
	fmsg->mnl.cmd = delivered ? FLIPC_CMD_ACKMESSAGE : FLIPC_CMD_NAKMESSAGE;
	fmsg->mnl.qos = 0;                              
	fmsg->mnl.flags = 0;
	fmsg->mnl.node_id = nid;
	fmsg->mnl.object = name;
	fmsg->mnl.options = 0;
	fmsg->mnl.size = sizeof(struct flipc_ack_msg) - sizeof(struct mnl_msg);

#if (0)
	mach_msg_return_t mmr;
	ipc_mqueue_t ack_mqueue;

	ip_lock(ack_port);
	ack_mqueue = &ack_port->ip_messages;
	imq_lock(ack_mqueue);
	ip_unlock(ack_port);

	                                          
	mmr = ipc_mqueue_send(ack_mqueue, kmsg, 0, 0);
#else
	kern_return_t kr;
	kr = ipc_kmsg_send(kmsg,
	    MACH_SEND_KERNEL_DEFAULT,
	    MACH_MSG_TIMEOUT_NONE);
#endif
}
