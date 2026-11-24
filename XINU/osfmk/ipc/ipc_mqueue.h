/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
  
   
  
                         
                      
             
  
                                  
   

#ifndef _IPC_IPC_MQUEUE_H_
#define _IPC_IPC_MQUEUE_H_

#include <mach_assert.h>

#include <mach/message.h>

#include <kern/assert.h>
#include <kern/macro_help.h>
#include <kern/kern_types.h>
#include <kern/waitq.h>

#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_types.h>

#include <sys/event.h>

typedef struct ipc_mqueue {
	union {
		struct {
			struct  waitq           waitq;
			struct ipc_kmsg_queue   messages;
			mach_port_seqno_t       seqno;
			mach_port_name_t        receiver_name;
			uint16_t                msgcount;
			uint16_t                qlimit;
#ifdef __LP64__
			uint32_t                qcontext;
#endif
#if MACH_FLIPC
			struct flipc_port       *fport;                                             
#endif
		} port;
		struct {
			struct waitq_set        setq;
		} pset;
	} data;
	union {
		  
               
                         
    
                                                   
                                    
    
                                             
                                               
                                                         
                                                             
                                                          
     
		struct klist imq_klist;
		struct knote *imq_inheritor_knote;
		struct turnstile *imq_inheritor_turnstile;
		thread_t imq_inheritor_thread_ref;
		thread_t imq_srp_owner_thread;
	};
#ifndef __LP64__
	uint32_t qcontext;
#endif
} *ipc_mqueue_t;

#define IMQ_NULL                ((ipc_mqueue_t) 0)

#define imq_wait_queue          data.port.waitq
#define imq_messages            data.port.messages
#define imq_msgcount            data.port.msgcount
#define imq_qlimit              data.port.qlimit
#define imq_seqno               data.port.seqno
#define imq_receiver_name       data.port.receiver_name
#if MACH_FLIPC
#define imq_fport               data.port.fport
#endif

  
                                                                             
                                                                                  
   
#ifdef __LP64__
#define imq_context             data.port.qcontext
#else
#define imq_context             qcontext
#endif

  
                                                   
                                      
   
#define imq_fullwaiters         data.port.waitq.waitq_eventmask
#define imq_in_pset             data.port.waitq.waitq_set_id
#define imq_preposts            data.port.waitq.waitq_prepost_id

#define imq_set_queue           data.pset.setq
#define imq_is_set(mq)          waitqs_is_set(&(mq)->imq_set_queue)
#define imq_is_queue(mq)        waitq_is_queue(&(mq)->imq_wait_queue)
#define imq_is_turnstile_proxy(mq) \
	        waitq_is_turnstile_proxy(&(mq)->imq_wait_queue)
#define imq_is_valid(mq)        waitq_is_valid(&(mq)->imq_wait_queue)

#define imq_unlock(mq)          waitq_unlock(&(mq)->imq_wait_queue)
#define imq_held(mq)            assert(waitq_held(&(mq)->imq_wait_queue))
#define imq_valid(mq)           waitq_valid(&(mq)->imq_wait_queue)

extern void imq_lock(ipc_mqueue_t mq);
extern unsigned int imq_lock_try(ipc_mqueue_t mq);

  
                                                                              
                                                                            
                                                                            
                                                    
   
#define imq_from_waitq(waitq)  (waitq_is_set(waitq) ? \
	        __container_of(waitq, struct ipc_mqueue, imq_set_queue.wqset_q) : \
	        __container_of(waitq, struct ipc_mqueue, imq_wait_queue))

#define imq_to_object(mq) ip_to_object(ip_from_mq(mq))

extern void imq_reserve_and_lock(ipc_mqueue_t mq,
    uint64_t *reserved_prepost);

extern void imq_release_and_unlock(ipc_mqueue_t mq,
    uint64_t reserved_prepost);

#define imq_full(mq)            ((mq)->imq_msgcount >= (mq)->imq_qlimit)
#define imq_full_kernel(mq)     ((mq)->imq_msgcount >= MACH_PORT_QLIMIT_KERNEL)

extern int ipc_mqueue_full;
                             

#define IPC_MQUEUE_FULL         CAST_EVENT64_T(&ipc_mqueue_full)
#define IPC_MQUEUE_RECEIVE      NO_EVENT64

  
                      
   

__enum_closed_decl(ipc_mqueue_kind_t, int, {
	IPC_MQUEUE_KIND_NONE,                                      
	IPC_MQUEUE_KIND_PORT,                                           
	IPC_MQUEUE_KIND_SET,                                       
});

                                                
extern void ipc_mqueue_init(
	ipc_mqueue_t            mqueue,
	ipc_mqueue_kind_t       kind);

                                                                      
extern void ipc_mqueue_deinit(
	ipc_mqueue_t            mqueue);

                       
extern boolean_t ipc_mqueue_destroy_locked(
	ipc_mqueue_t            mqueue);

                                                  
extern void ipc_mqueue_changed(
	ipc_space_t             space,
	ipc_mqueue_t            mqueue);

                                                    
extern kern_return_t ipc_mqueue_add(
	ipc_mqueue_t            mqueue,
	ipc_mqueue_t            set_mqueue,
	uint64_t                *reserved_link,
	uint64_t                *reserved_prepost);

                                                    
extern boolean_t ipc_mqueue_member(
	ipc_mqueue_t            mqueue,
	ipc_mqueue_t            set_mqueue);

                                          
extern kern_return_t ipc_mqueue_remove(
	ipc_mqueue_t            mqueue,
	ipc_mqueue_t            set_mqueue);

                                    
extern void ipc_mqueue_remove_from_all(
	ipc_mqueue_t            mqueue);

                                                  
extern void ipc_mqueue_remove_all(
	ipc_mqueue_t            mqueue);

                              
extern mach_msg_return_t ipc_mqueue_send(
	ipc_mqueue_t            mqueue,
	ipc_kmsg_t              kmsg,
	mach_msg_option_t       option,
	mach_msg_timeout_t  timeout_val);

                                               
extern mach_msg_return_t ipc_mqueue_preflight_send(
	ipc_mqueue_t            mqueue,
	ipc_kmsg_t              kmsg,
	mach_msg_option_t       option,
	mach_msg_timeout_t      timeout_val);

                                                  
extern void ipc_mqueue_override_send(
	ipc_mqueue_t        mqueue,
	mach_msg_qos_t      qos_ovr);

                                                          
extern void ipc_mqueue_post(
	ipc_mqueue_t            mqueue,
	ipc_kmsg_t              kmsg,
	mach_msg_option_t       option);

                                            
extern void ipc_mqueue_receive(
	ipc_mqueue_t            mqueue,
	mach_msg_option_t       option,
	mach_msg_size_t         max_size,
	mach_msg_timeout_t      timeout_val,
	int                     interruptible);

                                                                     
extern wait_result_t ipc_mqueue_receive_on_thread(
	ipc_mqueue_t            mqueue,
	mach_msg_option_t       option,
	mach_msg_size_t         max_size,
	mach_msg_timeout_t      rcv_timeout,
	int                     interruptible,
	thread_t                thread);

                                              
extern void ipc_mqueue_receive_continue(
	void                    *param,
	wait_result_t           wresult);

                                                                 
extern void ipc_mqueue_select_on_thread(
	ipc_mqueue_t            port_mq,
	ipc_mqueue_t            set_mq,
	mach_msg_option_t       option,
	mach_msg_size_t         max_size,
	thread_t                thread);

                                                            
extern unsigned ipc_mqueue_peek(
	ipc_mqueue_t            mqueue,
	mach_port_seqno_t       *msg_seqnop,
	mach_msg_size_t         *msg_sizep,
	mach_msg_id_t           *msg_idp,
	mach_msg_max_trailer_t  *msg_trailerp,
	ipc_kmsg_t              *kmsgp);

                                                                   
extern unsigned ipc_mqueue_peek_locked(
	ipc_mqueue_t            mqueue,
	mach_port_seqno_t       *msg_seqnop,
	mach_msg_size_t         *msg_sizep,
	mach_msg_id_t           *msg_idp,
	mach_msg_max_trailer_t  *msg_trailerp,
	ipc_kmsg_t              *kmsgp);

                                                                            
extern unsigned ipc_mqueue_set_peek(
	ipc_mqueue_t            mqueue);

                                                                        
extern void ipc_mqueue_release_peek_ref(
	ipc_mqueue_t            mqueue);

                                                     
extern void ipc_mqueue_set_gather_member_names(
	ipc_space_t             space,
	ipc_mqueue_t            set_mq,
	ipc_entry_num_t         maxnames,
	mach_port_name_t        *names,
	ipc_entry_num_t         *actualp);

                                       
extern void ipc_mqueue_release_msgcount(
	ipc_mqueue_t            port_mq,
	ipc_mqueue_t            set_mq);

                          
extern void ipc_mqueue_set_qlimit(
	ipc_mqueue_t            mqueue,
	mach_port_msgcount_t    qlimit);

                                      
extern void ipc_mqueue_set_seqno(
	ipc_mqueue_t            mqueue,
	mach_port_seqno_t       seqno);

                                                  
extern mach_msg_return_t ipc_mqueue_copyin(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_mqueue_t            *mqueuep,
	ipc_object_t            *objectp);

#endif                          
