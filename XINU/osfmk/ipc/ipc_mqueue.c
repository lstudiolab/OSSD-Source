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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_FREE_COPYRIGHT@
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
  
   
  
                         
                      
             
  
                                              
   
  
                                                                      
                                                                          
                                                                        
               
   


#include <mach/port.h>
#include <mach/message.h>
#include <mach/sync_policy.h>

#include <kern/assert.h>
#include <kern/counter.h>
#include <kern/sched_prim.h>
#include <kern/ipc_kobject.h>
#include <kern/ipc_mig.h>                                                
#include <kern/misc_protos.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/waitq.h>

#include <ipc/port.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_space.h>

#if MACH_FLIPC
#include <ipc/flipc.h>
#endif

#ifdef __LP64__
#include <vm/vm_map.h>
#endif

#include <sys/event.h>

extern char     *proc_name_address(void *p);

int ipc_mqueue_full;                                                  
int ipc_mqueue_rcv;                                                       

                          
static void ipc_mqueue_receive_results(wait_result_t result);
static void ipc_mqueue_peek_on_thread(
	ipc_mqueue_t        port_mq,
	mach_msg_option_t   option,
	thread_t            thread);

  
                           
           
                                               
   
void
ipc_mqueue_init(
	ipc_mqueue_t            mqueue,
	ipc_mqueue_kind_t       kind)
{
	switch (kind) {
	case IPC_MQUEUE_KIND_SET:
		waitq_set_init(&mqueue->imq_set_queue,
		    SYNC_POLICY_FIFO | SYNC_POLICY_PREPOST,
		    NULL, NULL);
		break;
	case IPC_MQUEUE_KIND_NONE:                                               
	case IPC_MQUEUE_KIND_PORT:
		waitq_init(&mqueue->imq_wait_queue,
		    SYNC_POLICY_FIFO | SYNC_POLICY_TURNSTILE_PROXY);
		ipc_kmsg_queue_init(&mqueue->imq_messages);
		mqueue->imq_seqno = 0;
		mqueue->imq_msgcount = 0;
		mqueue->imq_qlimit = MACH_PORT_QLIMIT_DEFAULT;
		mqueue->imq_context = 0;
		mqueue->imq_fullwaiters = FALSE;
#if MACH_FLIPC
		mqueue->imq_fport = FPORT_NULL;
#endif
		break;
	}
	klist_init(&mqueue->imq_klist);
}

void
ipc_mqueue_deinit(
	ipc_mqueue_t            mqueue)
{
	boolean_t is_set = imq_is_set(mqueue);

	if (is_set) {
		waitq_set_deinit(&mqueue->imq_set_queue);
	} else {
		waitq_deinit(&mqueue->imq_wait_queue);
	}
}

  
                                
           
                                                      
                                                        
                             
              
                  
   
void
imq_reserve_and_lock(ipc_mqueue_t mq, uint64_t *reserved_prepost)
{
	*reserved_prepost = waitq_prepost_reserve(&mq->imq_wait_queue, 0,
	    WAITQ_KEEP_LOCKED);
}


  
                                  
           
                                                        
                                                       
              
                
   
void
imq_release_and_unlock(ipc_mqueue_t mq, uint64_t reserved_prepost)
{
	waitq_unlock(&mq->imq_wait_queue);
	waitq_prepost_release_reserve(reserved_prepost);
}


  
                             
           
                                                     
                                                  
                                                      
                            
              
                                                
                                                       
   

boolean_t
ipc_mqueue_member(
	ipc_mqueue_t            port_mqueue,
	ipc_mqueue_t            set_mqueue)
{
	struct waitq *port_waitq = &port_mqueue->imq_wait_queue;
	struct waitq_set *set_waitq = &set_mqueue->imq_set_queue;

	return waitq_member(port_waitq, set_waitq);
}

  
                             
           
                                                              
                      
   

kern_return_t
ipc_mqueue_remove(
	ipc_mqueue_t      mqueue,
	ipc_mqueue_t      set_mqueue)
{
	struct waitq *mq_waitq = &mqueue->imq_wait_queue;
	struct waitq_set *set_waitq = &set_mqueue->imq_set_queue;

	return waitq_unlink(mq_waitq, set_waitq);
}

  
                                      
           
                                                         
              
                   
           
                                             
   
void
ipc_mqueue_remove_from_all(ipc_mqueue_t mqueue)
{
	struct waitq *mq_waitq = &mqueue->imq_wait_queue;
	kern_return_t kr;

	imq_lock(mqueue);

	assert(waitq_valid(mq_waitq));
	kr = waitq_unlink_all_unlock(mq_waitq);
	                                               
}

  
                                 
           
                                                        
                                                    
              
                   
           
                                             
   
void
ipc_mqueue_remove_all(ipc_mqueue_t      mqueue)
{
	struct waitq_set *mq_setq = &mqueue->imq_set_queue;

	imq_lock(mqueue);
	assert(waitqs_is_set(mq_setq));
	waitq_set_unlink_all_unlock(mq_setq);
	                                           
}


  
                          
           
                                                          
                                                            
                                                          
                                                             
           
              
              
   
kern_return_t
ipc_mqueue_add(
	ipc_mqueue_t    port_mqueue,
	ipc_mqueue_t    set_mqueue,
	uint64_t        *reserved_link,
	uint64_t        *reserved_prepost)
{
	struct waitq     *port_waitq = &port_mqueue->imq_wait_queue;
	struct waitq_set *set_waitq = &set_mqueue->imq_set_queue;
	ipc_kmsg_queue_t kmsgq;
	ipc_kmsg_t       kmsg, next;
	kern_return_t    kr;

	assert(reserved_link && *reserved_link != 0);
	assert(waitqs_is_linked(set_waitq));

	imq_lock(port_mqueue);

	  
                                                         
                                                                  
    
	kr = waitq_link(port_waitq, set_waitq, WAITQ_ALREADY_LOCKED, reserved_link);
	if (kr != KERN_SUCCESS) {
		imq_unlock(port_mqueue);
		return kr;
	}

	  
                                                             
                                                              
                                   
    
	kmsgq = &port_mqueue->imq_messages;
	for (kmsg = ipc_kmsg_queue_first(kmsgq);
	    kmsg != IKM_NULL;
	    kmsg = next) {
		next = ipc_kmsg_queue_next(kmsgq, kmsg);

		for (;;) {
			thread_t th;
			mach_msg_size_t msize;
			spl_t th_spl;

			th = waitq_wakeup64_identify_locked(
				port_waitq,
				IPC_MQUEUE_RECEIVE,
				THREAD_AWAKENED, &th_spl,
				reserved_prepost, WAITQ_ALL_PRIORITIES,
				WAITQ_KEEP_LOCKED);
			                                              

			if (th == THREAD_NULL) {
				goto leave;
			}

			  
                                                         
                                                              
                                                            
                                          
      
			if (th->ith_state != MACH_RCV_IN_PROGRESS) {
				if (th->ith_state == MACH_PEEK_IN_PROGRESS) {
					  
                                      
                                         
                                           
                                         
        
					ipc_mqueue_peek_on_thread(port_mqueue,
					    th->ith_option,
					    th);
				}
				thread_unlock(th);
				splx(th_spl);
				continue;
			}

			  
                                                          
                                                          
                                                          
                                                         
                                                         
                              
      
			msize = ipc_kmsg_copyout_size(kmsg, th->map);
			if (th->ith_rsize <
			    (msize + REQUESTED_TRAILER_SIZE(thread_is_64bit_addr(th), th->ith_option))) {
				th->ith_state = MACH_RCV_TOO_LARGE;
				th->ith_msize = msize;
				if (th->ith_option & MACH_RCV_LARGE) {
					  
                                  
        
					th->ith_receiver_name = port_mqueue->imq_receiver_name;
					th->ith_kmsg = IKM_NULL;
					th->ith_seqno = 0;
					thread_unlock(th);
					splx(th_spl);
					continue;                          
				}
			} else {
				th->ith_state = MACH_MSG_SUCCESS;
			}

			  
                                                
                        
      
			ipc_kmsg_rmqueue(kmsgq, kmsg);
#if MACH_FLIPC
			mach_node_t  node = kmsg->ikm_node;
#endif
			ipc_mqueue_release_msgcount(port_mqueue, IMQ_NULL);

			th->ith_kmsg = kmsg;
			th->ith_seqno = port_mqueue->imq_seqno++;
			thread_unlock(th);
			splx(th_spl);
#if MACH_FLIPC
			if (MACH_NODE_VALID(node) && FPORT_VALID(port_mqueue->imq_fport)) {
				flipc_msg_ack(node, port_mqueue, TRUE);
			}
#endif
			break;                          
		}
	}
leave:
	imq_unlock(port_mqueue);
	return KERN_SUCCESS;
}


  
                                
           
                                                                            
   
static inline bool
ipc_mqueue_has_klist(ipc_mqueue_t mqueue)
{
	ipc_object_t object = imq_to_object(mqueue);
	if (io_otype(object) != IOT_PORT) {
		return true;
	}
	ipc_port_t port = ip_from_mq(mqueue);
	if (port->ip_specialreply) {
		return false;
	}
	return port->ip_sync_link_state == PORT_SYNC_LINK_ANY;
}

  
                              
           
                                                 
              
                                
   
void
ipc_mqueue_changed(
	ipc_space_t     space,
	ipc_mqueue_t    mqueue)
{
	if (ipc_mqueue_has_klist(mqueue) && SLIST_FIRST(&mqueue->imq_klist)) {
		  
                                                  
    
                                                                       
                                                                          
                                          
    
                                                                       
                                                                        
                                                                          
                                     
    
                                                                      
                                                                         
    
                                                                      
                                                                   
                                        
    
                                                                          
                                                     
    
                                                                    
                                                                    
                                                       
     
		assert(space);
		knote_vanish(&mqueue->imq_klist, is_active(space));
	}

	if (io_otype(imq_to_object(mqueue)) == IOT_PORT) {
		ipc_port_adjust_sync_link_state_locked(ip_from_mq(mqueue), PORT_SYNC_LINK_ANY, NULL);
	} else {
		klist_init(&mqueue->imq_klist);
	}

	waitq_wakeup64_all_locked(&mqueue->imq_wait_queue,
	    IPC_MQUEUE_RECEIVE,
	    THREAD_RESTART,
	    NULL,
	    WAITQ_ALL_PRIORITIES,
	    WAITQ_KEEP_LOCKED);
}




  
                           
           
                                                                     
                                                          
                           
  
                                                       
                                                              
                                                             
              
                     
           
                                              
                                                 
                                                   
   
mach_msg_return_t
ipc_mqueue_send(
	ipc_mqueue_t            mqueue,
	ipc_kmsg_t              kmsg,
	mach_msg_option_t       option,
	mach_msg_timeout_t  send_timeout)
{
	int wresult;

	  
                    
                                   
                                                        
                                            
    
	if (!imq_full(mqueue) ||
	    (!imq_full_kernel(mqueue) &&
	    ((option & MACH_SEND_ALWAYS) ||
	    (MACH_MSGH_BITS_REMOTE(kmsg->ikm_header->msgh_bits) ==
	    MACH_MSG_TYPE_PORT_SEND_ONCE)))) {
		mqueue->imq_msgcount++;
		assert(mqueue->imq_msgcount > 0);
		imq_unlock(mqueue);
	} else {
		thread_t cur_thread = current_thread();
		ipc_port_t port = ip_from_mq(mqueue);
		struct turnstile *send_turnstile = TURNSTILE_NULL;
		uint64_t deadline;

		  
                                                   
     
		if ((option & MACH_SEND_TIMEOUT) && (send_timeout == 0)) {
			imq_unlock(mqueue);
			return MACH_SEND_TIMED_OUT;
		}
		if (imq_full_kernel(mqueue)) {
			imq_unlock(mqueue);
			return MACH_SEND_NO_BUFFER;
		}
		mqueue->imq_fullwaiters = TRUE;

		if (option & MACH_SEND_TIMEOUT) {
			clock_interval_to_deadline(send_timeout, 1000 * NSEC_PER_USEC, &deadline);
		} else {
			deadline = 0;
		}

		thread_set_pending_block_hint(cur_thread, kThreadWaitPortSend);

		send_turnstile = turnstile_prepare((uintptr_t)port,
		    port_send_turnstile_address(port),
		    TURNSTILE_NULL, TURNSTILE_SYNC_IPC);

		ipc_port_send_update_inheritor(port, send_turnstile,
		    TURNSTILE_DELAYED_UPDATE);

		wresult = waitq_assert_wait64_leeway(
			&send_turnstile->ts_waitq,
			IPC_MQUEUE_FULL,
			THREAD_ABORTSAFE,
			TIMEOUT_URGENCY_USER_NORMAL,
			deadline,
			TIMEOUT_NO_LEEWAY);

		imq_unlock(mqueue);
		turnstile_update_inheritor_complete(send_turnstile,
		    TURNSTILE_INTERLOCK_NOT_HELD);

		if (wresult == THREAD_WAITING) {
			wresult = thread_block(THREAD_CONTINUE_NULL);
		}

		                                                 
		imq_lock(mqueue);
		turnstile_complete((uintptr_t)port, port_send_turnstile_address(port), NULL, TURNSTILE_SYNC_IPC);
		imq_unlock(mqueue);

		                                               
		turnstile_cleanup();

		switch (wresult) {
		case THREAD_AWAKENED:
			  
                                                    
                                                              
                                                                
      
			break;

		case THREAD_TIMED_OUT:
			assert(option & MACH_SEND_TIMEOUT);
			return MACH_SEND_TIMED_OUT;

		case THREAD_INTERRUPTED:
			return MACH_SEND_INTERRUPTED;

		case THREAD_RESTART:
			                               
			return MACH_SEND_INVALID_DEST;
		default:
			panic("ipc_mqueue_send");
		}
	}

	ipc_mqueue_post(mqueue, kmsg, option);
	return MACH_MSG_SUCCESS;
}

  
                                    
           
                                                         
                                                            
                                                            
          
  
              
                                    
                                                      
   
void
ipc_mqueue_override_send(
	ipc_mqueue_t        mqueue,
	mach_msg_qos_t      qos_ovr)
{
	boolean_t __unused full_queue_empty = FALSE;

	imq_lock(mqueue);
	assert(imq_valid(mqueue));
	assert(!imq_is_set(mqueue));

	if (imq_full(mqueue)) {
		ipc_kmsg_t first = ipc_kmsg_queue_first(&mqueue->imq_messages);

		if (first && ipc_kmsg_override_qos(&mqueue->imq_messages, first, qos_ovr)) {
			ipc_object_t object = imq_to_object(mqueue);
			assert(io_otype(object) == IOT_PORT);
			ipc_port_t port = ip_object_to_port(object);
			if (ip_active(port) &&
			    port->ip_receiver_name != MACH_PORT_NULL &&
			    is_active(port->ip_receiver) &&
			    ipc_mqueue_has_klist(mqueue)) {
				KNOTE(&mqueue->imq_klist, 0);
			}
		}
		if (!first) {
			full_queue_empty = TRUE;
		}
	}
	imq_unlock(mqueue);

#if DEVELOPMENT || DEBUG
	if (full_queue_empty) {
		ipc_port_t port = ip_from_mq(mqueue);
		int dst_pid = 0;
		dst_pid = ipc_port_get_receiver_task(port, NULL);
	}
#endif
}

  
                                       
           
                                                          
                   
  
              
                                
                                                                 
                                                                
                           
   
void
ipc_mqueue_release_msgcount(ipc_mqueue_t port_mq, ipc_mqueue_t set_mq)
{
	struct turnstile *send_turnstile = port_send_turnstile(ip_from_mq(port_mq));
	(void)set_mq;
	imq_held(port_mq);
	assert(port_mq->imq_msgcount > 1 || ipc_kmsg_queue_empty(&port_mq->imq_messages));

	port_mq->imq_msgcount--;

	if (!imq_full(port_mq) && port_mq->imq_fullwaiters &&
	    send_turnstile != TURNSTILE_NULL) {
		  
                                            
                                               
                                                
    
                                  
    
                                            
                                                   
                                                       
                                                     
                                                       
     
		if (waitq_wakeup64_one(&send_turnstile->ts_waitq,
		    IPC_MQUEUE_FULL,
		    THREAD_AWAKENED,
		    WAITQ_PROMOTE_PRIORITY) != KERN_SUCCESS) {
			port_mq->imq_fullwaiters = FALSE;
		} else {
			                                             
			port_mq->imq_msgcount++;
		}
	}

	if (ipc_kmsg_queue_empty(&port_mq->imq_messages)) {
		                                                        
		waitq_clear_prepost_locked(&port_mq->imq_wait_queue);
	}
}

  
                           
           
                                                             
                                                             
                      
  
              
                      
                                                                    
   
void
ipc_mqueue_post(
	ipc_mqueue_t               mqueue,
	ipc_kmsg_t                 kmsg,
	mach_msg_option_t __unused option)
{
	uint64_t reserved_prepost = 0;
	boolean_t destroy_msg = FALSE;

	ipc_kmsg_trace_send(kmsg, option);

	  
                                                         
                                                       
   
                                         
    
	imq_reserve_and_lock(mqueue, &reserved_prepost);

	                                              
	if (!imq_valid(mqueue)) {
		destroy_msg = TRUE;
		goto out_unlock;
	}

	for (;;) {
		struct waitq *waitq = &mqueue->imq_wait_queue;
		spl_t th_spl;
		thread_t receiver;
		mach_msg_size_t msize;

		receiver = waitq_wakeup64_identify_locked(waitq,
		    IPC_MQUEUE_RECEIVE,
		    THREAD_AWAKENED,
		    &th_spl,
		    &reserved_prepost,
		    WAITQ_ALL_PRIORITIES,
		    WAITQ_KEEP_LOCKED);
		                                       

		if (receiver == THREAD_NULL) {
			  
                                                      
                                                             
                                                     
                          
     
                                                         
                                                        
                                                         
                                        
     
                                                             
                                                                      
                   
      
			if (mqueue->imq_msgcount > 0) {
				if (ipc_kmsg_enqueue_qos(&mqueue->imq_messages, kmsg)) {
					                                                          
					ipc_object_t object = imq_to_object(mqueue);
					assert(io_otype(object) == IOT_PORT);
					ipc_port_t port = ip_object_to_port(object);
					if (ip_active(port) &&
					    port->ip_receiver_name != MACH_PORT_NULL &&
					    is_active(port->ip_receiver) &&
					    ipc_mqueue_has_klist(mqueue)) {
						KNOTE(&mqueue->imq_klist, 0);
					}
				}
				break;
			}

			  
                                                             
                                                                  
      
			destroy_msg = TRUE;
			goto out_unlock;
		}

		  
                                                              
                                                                     
                                                                  
                                                                   
                                                                 
                                                               
                                                                
                                
     
		if (receiver->ith_state == MACH_PEEK_IN_PROGRESS && mqueue->imq_msgcount > 0) {
			ipc_kmsg_enqueue_qos(&mqueue->imq_messages, kmsg);
			ipc_mqueue_peek_on_thread(mqueue, receiver->ith_option, receiver);
			thread_unlock(receiver);
			splx(th_spl);
			break;                                               
		}

		  
                                                                
                                                                
                                                           
                             
     
		if (receiver->ith_state != MACH_RCV_IN_PROGRESS) {
			thread_unlock(receiver);
			splx(th_spl);
			continue;
		}


		  
                               
                                                                 
                                                       
     
		msize = ipc_kmsg_copyout_size(kmsg, receiver->map);
		if (receiver->ith_rsize <
		    (msize + REQUESTED_TRAILER_SIZE(thread_is_64bit_addr(receiver), receiver->ith_option))) {
			receiver->ith_msize = msize;
			receiver->ith_state = MACH_RCV_TOO_LARGE;
		} else {
			receiver->ith_state = MACH_MSG_SUCCESS;
		}

		  
                                                             
                                                            
                                                          
     
		if ((receiver->ith_state == MACH_MSG_SUCCESS) ||
		    !(receiver->ith_option & MACH_RCV_LARGE)) {
			receiver->ith_kmsg = kmsg;
			receiver->ith_seqno = mqueue->imq_seqno++;
#if MACH_FLIPC
			mach_node_t node = kmsg->ikm_node;
#endif
			thread_unlock(receiver);
			splx(th_spl);

			                                                   
			ipc_mqueue_release_msgcount(mqueue, IMQ_NULL);

#if MACH_FLIPC
			if (MACH_NODE_VALID(node) && FPORT_VALID(mqueue->imq_fport)) {
				flipc_msg_ack(node, mqueue, TRUE);
			}
#endif
			break;
		}

		  
                                                       
                                                          
                                          
     
		receiver->ith_receiver_name = mqueue->imq_receiver_name;
		receiver->ith_kmsg = IKM_NULL;
		receiver->ith_seqno = 0;
		thread_unlock(receiver);
		splx(th_spl);
	}

out_unlock:
	                                                  
	waitq_clear_promotion_locked(&mqueue->imq_wait_queue, current_thread());
	imq_release_and_unlock(mqueue, reserved_prepost);
	if (destroy_msg) {
		ipc_kmsg_destroy(kmsg);
	}

	current_task()->messages_sent++;
	return;
}


static void
ipc_mqueue_receive_results(wait_result_t saved_wait_result)
{
	thread_t                self = current_thread();
	mach_msg_option_t       option = self->ith_option;

	  
                       
    
	switch (saved_wait_result) {
	case THREAD_TIMED_OUT:
		self->ith_state = MACH_RCV_TIMED_OUT;
		return;

	case THREAD_INTERRUPTED:
		self->ith_state = MACH_RCV_INTERRUPTED;
		return;

	case THREAD_RESTART:
		                                            
		self->ith_state = MACH_RCV_PORT_CHANGED;
		return;

	case THREAD_AWAKENED:
		  
                                                    
                                               
     
		switch (self->ith_state) {
		case MACH_RCV_SCATTER_SMALL:
		case MACH_RCV_TOO_LARGE:
			  
                                           
                                             
                                            
                                           
                             
      
			if (option & MACH_RCV_LARGE) {
				return;
			}
			return;
		case MACH_MSG_SUCCESS:
			return;
		case MACH_PEEK_READY:
			return;

		default:
			panic("ipc_mqueue_receive_results: strange ith_state");
		}

	default:
		panic("ipc_mqueue_receive_results: strange wait_result");
	}
}

void
ipc_mqueue_receive_continue(
	__unused void *param,
	wait_result_t wresult)
{
	ipc_mqueue_receive_results(wresult);
	mach_msg_receive_continue();                          
}

  
                              
           
                                           
  
              
                                                             
                                                  
                           
  
                                                 
                                                             
                                                     
           
                                                  
                                                          
                                           
                                             
                                                 
                                                      
  
   

void
ipc_mqueue_receive(
	ipc_mqueue_t            mqueue,
	mach_msg_option_t       option,
	mach_msg_size_t         max_size,
	mach_msg_timeout_t      rcv_timeout,
	int                     interruptible)
{
	wait_result_t           wresult;
	thread_t                self = current_thread();

	imq_lock(mqueue);
	wresult = ipc_mqueue_receive_on_thread(mqueue, option, max_size,
	    rcv_timeout, interruptible,
	    self);
	                     
	if (wresult == THREAD_NOT_WAITING) {
		return;
	}

	if (wresult == THREAD_WAITING) {
		if (self->ith_continuation) {
			thread_block(ipc_mqueue_receive_continue);
		}
		                

		wresult = thread_block(THREAD_CONTINUE_NULL);
	}
	ipc_mqueue_receive_results(wresult);
}

static int
mqueue_process_prepost_receive(void *ctx, struct waitq *waitq,
    struct waitq_set *wqset)
{
	ipc_mqueue_t     port_mq, *pmq_ptr;

	(void)wqset;
	port_mq = (ipc_mqueue_t)waitq;

	  
                                                              
                            
    
	if (ipc_kmsg_queue_empty(&port_mq->imq_messages)) {
		return WQ_ITERATE_INVALIDATE_CONTINUE;
	}

	  
                                            
                                                               
                 
    
	pmq_ptr = (ipc_mqueue_t *)ctx;
	if (pmq_ptr) {
		*pmq_ptr = port_mq;
	}
	return WQ_ITERATE_BREAK_KEEP_LOCKED;
}

  
                                        
           
                                                                    
                                                                  
  
              
                           
                              
                                 
                                                             
   
wait_result_t
ipc_mqueue_receive_on_thread(
	ipc_mqueue_t            mqueue,
	mach_msg_option_t       option,
	mach_msg_size_t         max_size,
	mach_msg_timeout_t      rcv_timeout,
	int                     interruptible,
	thread_t                thread)
{
	wait_result_t           wresult;
	uint64_t                deadline;
	struct turnstile        *rcv_turnstile = TURNSTILE_NULL;

	                               

	                                                             

	if (!imq_valid(mqueue)) {
		                                                   
		imq_unlock(mqueue);
		  
                                                              
                                                       
                                            
     
		return THREAD_RESTART;
	}

	if (imq_is_set(mqueue)) {
		ipc_mqueue_t port_mq = IMQ_NULL;

		(void)waitq_set_iterate_preposts(&mqueue->imq_set_queue,
		    &port_mq,
		    mqueue_process_prepost_receive);

		if (port_mq != IMQ_NULL) {
			  
                                                  
                                                        
                                                       
                        
     
                                                       
                                       
      
			imq_unlock(mqueue);

			  
                                                   
                             
      
			if (option & MACH_PEEK_MSG) {
				ipc_mqueue_peek_on_thread(port_mq, option, thread);
			} else {
				ipc_mqueue_select_on_thread(port_mq, mqueue, option,
				    max_size, thread);
			}

			imq_unlock(port_mq);
			return THREAD_NOT_WAITING;
		}
	} else if (imq_is_queue(mqueue) || imq_is_turnstile_proxy(mqueue)) {
		ipc_kmsg_queue_t kmsgs;

		  
                                                            
     
		kmsgs = &mqueue->imq_messages;
		if (ipc_kmsg_queue_first(kmsgs) != IKM_NULL) {
			if (option & MACH_PEEK_MSG) {
				ipc_mqueue_peek_on_thread(mqueue, option, thread);
			} else {
				ipc_mqueue_select_on_thread(mqueue, IMQ_NULL, option,
				    max_size, thread);
			}
			imq_unlock(mqueue);
			return THREAD_NOT_WAITING;
		}
	} else {
		panic("Unknown mqueue type 0x%x: likely memory corruption!\n",
		    mqueue->imq_wait_queue.waitq_type);
	}

	  
                                                       
                                                       
                 
    
	if (option & MACH_RCV_TIMEOUT) {
		if (rcv_timeout == 0) {
			imq_unlock(mqueue);
			thread->ith_state = MACH_RCV_TIMED_OUT;
			return THREAD_NOT_WAITING;
		}
	}

	thread->ith_option = option;
	thread->ith_rsize = max_size;
	thread->ith_msize = 0;

	if (option & MACH_PEEK_MSG) {
		thread->ith_state = MACH_PEEK_IN_PROGRESS;
	} else {
		thread->ith_state = MACH_RCV_IN_PROGRESS;
	}

	if (option & MACH_RCV_TIMEOUT) {
		clock_interval_to_deadline(rcv_timeout, 1000 * NSEC_PER_USEC, &deadline);
	} else {
		deadline = 0;
	}

	  
                                                 
                                       
   
                                         
                                           
                                           
                                         
                                             
                                        
                                           
                         
   
                                               
                                                     
                                           
                                                  
    
	if (imq_is_turnstile_proxy(mqueue)) {
		ipc_port_t port = ip_from_mq(mqueue);
		rcv_turnstile = turnstile_prepare((uintptr_t)port,
		    port_rcv_turnstile_address(port),
		    TURNSTILE_NULL, TURNSTILE_SYNC_IPC);

		ipc_port_recv_update_inheritor(port, rcv_turnstile,
		    TURNSTILE_DELAYED_UPDATE);
	}

	thread_set_pending_block_hint(thread, kThreadWaitPortReceive);
	wresult = waitq_assert_wait64_locked(&mqueue->imq_wait_queue,
	    IPC_MQUEUE_RECEIVE,
	    interruptible,
	    TIMEOUT_URGENCY_USER_NORMAL,
	    deadline,
	    TIMEOUT_NO_LEEWAY,
	    thread);
	                                                 
	if (wresult == THREAD_AWAKENED) {
		panic("ipc_mqueue_receive_on_thread: sleep walking");
	}

	imq_unlock(mqueue);

	                                                                                            
	if (rcv_turnstile != TURNSTILE_NULL) {
		turnstile_update_inheritor_complete(rcv_turnstile, TURNSTILE_INTERLOCK_NOT_HELD);
	}
	                                                                                     

	return wresult;
}


  
                                     
           
                                                               
                                                                  
                                    
              
                  
                                                   
  
                                  
                                                       
   
void
ipc_mqueue_peek_on_thread(
	ipc_mqueue_t        port_mq,
	mach_msg_option_t   option,
	thread_t            thread)
{
	(void)option;
	assert(option & MACH_PEEK_MSG);
	assert(ipc_kmsg_queue_first(&port_mq->imq_messages) != IKM_NULL);

	  
                                                     
                                                                    
                         
    
	ip_reference_mq(port_mq);
	thread->ith_peekq = port_mq;
	thread->ith_state = MACH_PEEK_READY;
}

  
                                       
           
                                                               
                                                               
                        
              
                  
                                  
                       
                                                              
  
           
                                                               
                                                                    
   
void
ipc_mqueue_select_on_thread(
	ipc_mqueue_t            port_mq,
	ipc_mqueue_t            set_mq,
	mach_msg_option_t       option,
	mach_msg_size_t         max_size,
	thread_t                thread)
{
	ipc_kmsg_t kmsg;
	mach_msg_return_t mr = MACH_MSG_SUCCESS;
	mach_msg_size_t msize;

	  
                                                     
                                             
    
	kmsg = ipc_kmsg_queue_first(&port_mq->imq_messages);
	assert(kmsg != IKM_NULL);

	  
                                                 
                                                     
                                                   
                      
    
	msize = ipc_kmsg_copyout_size(kmsg, thread->map);
	if (msize + REQUESTED_TRAILER_SIZE(thread_is_64bit_addr(thread), option) > max_size) {
		mr = MACH_RCV_TOO_LARGE;
		if (option & MACH_RCV_LARGE) {
			thread->ith_receiver_name = port_mq->imq_receiver_name;
			thread->ith_kmsg = IKM_NULL;
			thread->ith_msize = msize;
			thread->ith_seqno = 0;
			thread->ith_state = mr;
			return;
		}
	}

	ipc_kmsg_rmqueue(&port_mq->imq_messages, kmsg);
#if MACH_FLIPC
	if (MACH_NODE_VALID(kmsg->ikm_node) && FPORT_VALID(port_mq->imq_fport)) {
		flipc_msg_ack(kmsg->ikm_node, port_mq, TRUE);
	}
#endif
	ipc_mqueue_release_msgcount(port_mq, set_mq);
	thread->ith_seqno = port_mq->imq_seqno++;
	thread->ith_kmsg = kmsg;
	thread->ith_state = mr;

	current_task()->messages_received++;
	return;
}

  
                                  
           
                                                                
                                                            
                                                               
            
  
              
                                          
                                                                     
                                                
   
unsigned
ipc_mqueue_peek_locked(ipc_mqueue_t mq,
    mach_port_seqno_t * seqnop,
    mach_msg_size_t * msg_sizep,
    mach_msg_id_t * msg_idp,
    mach_msg_max_trailer_t * msg_trailerp,
    ipc_kmsg_t *kmsgp)
{
	ipc_kmsg_queue_t kmsgq;
	ipc_kmsg_t kmsg;
	mach_port_seqno_t seqno, msgoff;
	unsigned res = 0;

	assert(!imq_is_set(mq));

	seqno = 0;
	if (seqnop != NULL) {
		seqno = *seqnop;
	}

	if (seqno == 0) {
		seqno = mq->imq_seqno;
		msgoff = 0;
	} else if (seqno >= mq->imq_seqno &&
	    seqno < mq->imq_seqno + mq->imq_msgcount) {
		msgoff = seqno - mq->imq_seqno;
	} else {
		goto out;
	}

	                                                      
	kmsgq = &mq->imq_messages;
	kmsg = ipc_kmsg_queue_first(kmsgq);
	while (msgoff-- && kmsg != IKM_NULL) {
		kmsg = ipc_kmsg_queue_next(kmsgq, kmsg);
	}
	if (kmsg == IKM_NULL) {
		goto out;
	}

	                                           
	if (seqnop != NULL) {
		*seqnop = seqno;
	}
	if (msg_sizep != NULL) {
		*msg_sizep = kmsg->ikm_header->msgh_size;
	}
	if (msg_idp != NULL) {
		*msg_idp = kmsg->ikm_header->msgh_id;
	}
	if (msg_trailerp != NULL) {
		memcpy(msg_trailerp,
		    (mach_msg_max_trailer_t *)((vm_offset_t)kmsg->ikm_header +
		    mach_round_msg(kmsg->ikm_header->msgh_size)),
		    sizeof(mach_msg_max_trailer_t));
	}
	if (kmsgp != NULL) {
		*kmsgp = kmsg;
	}

	res = 1;

out:
	return res;
}


  
                           
           
                                                                
                                                            
                                                               
            
  
              
                                 
                                                               
                                                
   
unsigned
ipc_mqueue_peek(ipc_mqueue_t mq,
    mach_port_seqno_t * seqnop,
    mach_msg_size_t * msg_sizep,
    mach_msg_id_t * msg_idp,
    mach_msg_max_trailer_t * msg_trailerp,
    ipc_kmsg_t *kmsgp)
{
	unsigned res;

	imq_lock(mq);

	res = ipc_mqueue_peek_locked(mq, seqnop, msg_sizep, msg_idp,
	    msg_trailerp, kmsgp);

	imq_unlock(mq);
	return res;
}

  
                                       
           
                                                                  
                                                            
                                      
  
              
                                               
                                                 
                               
  
   
void
ipc_mqueue_release_peek_ref(ipc_mqueue_t mq)
{
	assert(!imq_is_set(mq));
	imq_held(mq);

	  
                                                 
                                                    
    
	waitq_clear_prepost_locked(&mq->imq_wait_queue);

	imq_unlock(mq);

	  
                                                                   
                                                        
     
	ip_release_mq(mq);
}

  
                                                                             
                                                                            
                                                                              
                                                                              
                                
   
static int
mqueue_peek_iterator(void *ctx, struct waitq *waitq,
    struct waitq_set *wqset)
{
	ipc_mqueue_t port_mq = (ipc_mqueue_t)waitq;
	ipc_kmsg_queue_t kmsgs = &port_mq->imq_messages;

	(void)ctx;
	(void)wqset;

	if (ipc_kmsg_queue_first(kmsgs) != IKM_NULL) {
		return WQ_ITERATE_BREAK;                                         
	}
	return WQ_ITERATE_CONTINUE;
}

  
                               
           
                                                          
                  
  
              
                                                               
                                                
   
unsigned
ipc_mqueue_set_peek(ipc_mqueue_t mq)
{
	int ret;

	imq_lock(mq);

	  
                                                                      
                                                                    
                                         
    
	if (!imq_is_valid(mq)) {
		return 1;
	}

	ret = waitq_set_iterate_preposts(&mq->imq_set_queue, NULL,
	    mqueue_peek_iterator);

	imq_unlock(mq);

	return ret == WQ_ITERATE_BREAK;
}

  
                                              
           
                                                             
                                                              
                                                                  
                                                                  
                                                                 
                                                                 
                                                                  
                                                                 
                                                               
                                                           
  
              
                                                               
                                                               
   
void
ipc_mqueue_set_gather_member_names(
	ipc_space_t space,
	ipc_mqueue_t set_mq,
	ipc_entry_num_t maxnames,
	mach_port_name_t *names,
	ipc_entry_num_t *actualp)
{
	ipc_entry_t table;
	ipc_entry_num_t tsize;
	struct waitq_set *wqset;
	ipc_entry_num_t actual = 0;

	assert(set_mq != IMQ_NULL);
	wqset = &set_mq->imq_set_queue;

	assert(space != IS_NULL);
	is_read_lock(space);
	if (!is_active(space)) {
		is_read_unlock(space);
		goto out;
	}

	if (!waitq_set_is_valid(wqset)) {
		is_read_unlock(space);
		goto out;
	}

	table = space->is_table;
	tsize = space->is_table_size;
	for (ipc_entry_num_t idx = 0; idx < tsize; idx++) {
		ipc_entry_t entry = &table[idx];

		                                                     
		if ((entry->ie_bits & MACH_PORT_TYPE_RECEIVE) != MACH_PORT_TYPE_NONE) {
			ipc_port_t port = ip_object_to_port(entry->ie_object);
			ipc_mqueue_t mq = &port->ip_messages;

			assert(IP_VALID(port));
			if (ip_active(port) &&
			    waitq_member(&mq->imq_wait_queue, wqset)) {
				if (actual < maxnames) {
					names[actual] = mq->imq_receiver_name;
				}
				actual++;
			}
		}
	}

	is_read_unlock(space);

out:
	*actualp = actual;
}


  
                                     
           
                                      
                                    
                                          
              
                 
                                                               
   
boolean_t
ipc_mqueue_destroy_locked(ipc_mqueue_t mqueue)
{
	ipc_kmsg_queue_t kmqueue;
	ipc_kmsg_t kmsg;
	boolean_t reap = FALSE;
	struct turnstile *send_turnstile = port_send_turnstile(ip_from_mq(mqueue));

	assert(!imq_is_set(mqueue));

	  
                             
                                                        
                    
    
	mqueue->imq_fullwaiters = FALSE;

	if (send_turnstile != TURNSTILE_NULL) {
		waitq_wakeup64_all(&send_turnstile->ts_waitq,
		    IPC_MQUEUE_FULL,
		    THREAD_RESTART,
		    WAITQ_ALL_PRIORITIES);
	}

	  
                                                            
                                                    
    
	kmqueue = &mqueue->imq_messages;
	while ((kmsg = ipc_kmsg_dequeue(kmqueue)) != IKM_NULL) {
#if MACH_FLIPC
		if (MACH_NODE_VALID(kmsg->ikm_node) && FPORT_VALID(mqueue->imq_fport)) {
			flipc_msg_ack(kmsg->ikm_node, mqueue, TRUE);
		}
#endif
		boolean_t first;
		first = ipc_kmsg_delayed_destroy(kmsg);
		if (first) {
			reap = first;
		}
	}

	  
                                                         
                                                                 
                                                                    
                               
    
	mqueue->imq_msgcount = 0;

	                                                           
	waitq_invalidate_locked(&mqueue->imq_wait_queue);

	                                               
	waitq_clear_prepost_locked(&mqueue->imq_wait_queue);

	  
                                                               
                                    
    
	assert(mqueue->imq_preposts == 0);
	assert(mqueue->imq_in_pset == 0);

	return reap;
}

  
                                 
           
                                                     
                                    
              
                   
   

void
ipc_mqueue_set_qlimit(
	ipc_mqueue_t                   mqueue,
	mach_port_msgcount_t   qlimit)
{
	assert(qlimit <= MACH_PORT_QLIMIT_MAX);

	                                               
	imq_lock(mqueue);
	if (qlimit > mqueue->imq_qlimit) {
		mach_port_msgcount_t i, wakeup;
		struct turnstile *send_turnstile = port_send_turnstile(ip_from_mq(mqueue));

		                                          
		wakeup = qlimit - mqueue->imq_qlimit;

		for (i = 0; i < wakeup; i++) {
			  
                                             
                                                
                                                 
     
                                   
      
			if (send_turnstile == TURNSTILE_NULL ||
			    waitq_wakeup64_one(&send_turnstile->ts_waitq,
			    IPC_MQUEUE_FULL,
			    THREAD_AWAKENED,
			    WAITQ_PROMOTE_PRIORITY) == KERN_NOT_WAITING) {
				mqueue->imq_fullwaiters = FALSE;
				break;
			}
			mqueue->imq_msgcount++;                                      
		}
	}
	mqueue->imq_qlimit = (uint16_t)qlimit;
	imq_unlock(mqueue);
}

  
                                
           
                                        
              
                                                              
   
void
ipc_mqueue_set_seqno(
	ipc_mqueue_t            mqueue,
	mach_port_seqno_t       seqno)
{
	imq_lock(mqueue);
	mqueue->imq_seqno = seqno;
	imq_unlock(mqueue);
}


  
                             
           
                                                 
              
                                                             
                                                               
              
           
                                           
                                            
                                                          
                         
                                                  
                                                        
   

mach_msg_return_t
ipc_mqueue_copyin(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_mqueue_t            *mqueuep,
	ipc_object_t            *objectp)
{
	ipc_entry_t entry;
	ipc_entry_bits_t bits;
	ipc_object_t object;
	ipc_mqueue_t mqueue;

	is_read_lock(space);
	if (!is_active(space)) {
		is_read_unlock(space);
		return MACH_RCV_INVALID_NAME;
	}

	entry = ipc_entry_lookup(space, name);
	if (entry == IE_NULL) {
		is_read_unlock(space);
		return MACH_RCV_INVALID_NAME;
	}

	bits = entry->ie_bits;
	object = entry->ie_object;

	if (bits & MACH_PORT_TYPE_RECEIVE) {
		ipc_port_t port = ip_object_to_port(object);

		assert(port != IP_NULL);

		ip_lock(port);
		require_ip_active(port);
		assert(port->ip_receiver_name == name);
		assert(port->ip_receiver == space);
		is_read_unlock(space);
		mqueue = &port->ip_messages;
	} else if (bits & MACH_PORT_TYPE_PORT_SET) {
		ipc_pset_t pset = ips_object_to_pset(object);

		assert(pset != IPS_NULL);

		ips_lock(pset);
		assert(ips_active(pset));
		is_read_unlock(space);

		mqueue = &pset->ips_messages;
	} else {
		is_read_unlock(space);
		                                                                      
		if ((bits & MACH_PORT_TYPE_EX_RECEIVE) == 0) {
			mach_port_guard_exception(name, 0, 0, kGUARD_EXC_RCV_INVALID_NAME);
		}
		return MACH_RCV_INVALID_NAME;
	}

	  
                                                   
                                                     
    

	io_reference(object);
	io_unlock(object);

	*objectp = object;
	*mqueuep = mqueue;
	return MACH_MSG_SUCCESS;
}

void
imq_lock(ipc_mqueue_t mq)
{
	ipc_object_t object = imq_to_object(mq);
	ipc_object_validate(object);
	waitq_lock(&(mq)->imq_wait_queue);
}

unsigned int
imq_lock_try(ipc_mqueue_t mq)
{
	ipc_object_t object = imq_to_object(mq);
	ipc_object_validate(object);
	return waitq_lock_try(&(mq)->imq_wait_queue);
}
