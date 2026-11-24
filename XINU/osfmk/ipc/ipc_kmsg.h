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
/*
 * NOTICE: This file was modified by McAfee Research in 2004 to introduce
 * support for mandatory and extensible security protections.  This notice
 * is included in support of clause 2.2 (b) of the Apple Public License,
 * Version 2.0.
 * Copyright (c) 2005 SPARTA, Inc.
 */
  
   
  
                       
                      
             
  
                                   
   

#ifndef _IPC_IPC_KMSG_H_
#define _IPC_IPC_KMSG_H_

#include <mach/vm_types.h>
#include <mach/message.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/macro_help.h>
#include <ipc/ipc_types.h>
#include <ipc/ipc_object.h>
#include <sys/kdebug.h>

#if (DEVELOPMENT || DEBUG)
                                                                 
#define IKM_PARTIAL_SIG        0
#endif

  
                                                       
                                                                
                                 
  
                                                            
                                                  
  
                                                           
                  
  
                                                              
                                                                 
   

struct ipc_kmsg {
	struct ipc_kmsg            *ikm_next;                                                
	struct ipc_kmsg            *ikm_prev;                                                
	union {
		ipc_port_t             XNU_PTRAUTH_SIGNED_PTR("kmsg.ikm_prealloc") ikmu_prealloc;                                     
		void                   *XNU_PTRAUTH_SIGNED_PTR("kmsg.ikm_data") ikmu_data;
	} ikm_u;
	mach_msg_header_t          *XNU_PTRAUTH_SIGNED_PTR("kmsg.ikm_header") ikm_header;
	ipc_port_t                 XNU_PTRAUTH_SIGNED_PTR("kmsg.ikm_voucher") ikm_voucher;                             
	struct ipc_importance_elem *ikm_importance;                      
	queue_chain_t              ikm_inheritance;                           
	struct turnstile           *ikm_turnstile;                                             
#if MACH_FLIPC
	struct mach_node           *ikm_node;                                               
#endif
	mach_msg_size_t            ikm_size;
	uint32_t                   ikm_ppriority;                                       
#if IKM_PARTIAL_SIG
	uintptr_t                  ikm_header_sig;                                
	uintptr_t                  ikm_headtrail_sig;                                
#endif
	uintptr_t                  ikm_signature;                                           
	ipc_object_copyin_flags_t  ikm_flags;
	mach_msg_qos_t             ikm_qos_override;                                
	mach_msg_filter_id         ikm_filter_policy_id;                                                            
};
#define ikm_prealloc ikm_u.ikmu_prealloc
#define ikm_data ikm_u.ikmu_data

#if defined(__i386__) || defined(__arm__)
#define IKM_SUPPORT_LEGACY      1
#else
#define IKM_SUPPORT_LEGACY      0
#endif

#define IKM_OVERHEAD            (sizeof(struct ipc_kmsg))

#define ikm_plus_overhead(size) ((mach_msg_size_t)((size) + IKM_OVERHEAD))
#define ikm_less_overhead(size) ((mach_msg_size_t)((size) - IKM_OVERHEAD))

  
                     
   
#define IKM_BOGUS               ((ipc_kmsg_t) 0xffffff10)

  
                                                              
                                                                     
   
extern zone_t ipc_kmsg_zone;
#define IKM_SAVED_KMSG_SIZE     256
#define IKM_SAVED_MSG_SIZE      ikm_less_overhead(IKM_SAVED_KMSG_SIZE)

#define ikm_prealloc_inuse_port(kmsg)                                   \
	((kmsg)->ikm_prealloc)

#define ikm_prealloc_inuse(kmsg)                                        \
	((kmsg)->ikm_prealloc != IP_NULL)

#define ikm_prealloc_set_inuse(kmsg, port)                              \
MACRO_BEGIN                                                             \
	assert((port) != IP_NULL);                                      \
	(kmsg)->ikm_prealloc = (port);                                  \
	ip_reference(port);                                             \
MACRO_END

#define ikm_prealloc_clear_inuse(kmsg, port)                            \
MACRO_BEGIN                                                             \
	(kmsg)->ikm_prealloc = IP_NULL;                                 \
MACRO_END

#define ikm_qos_init(kmsg)                                              \
MACRO_BEGIN                                                             \
	(kmsg)->ikm_ppriority = MACH_MSG_PRIORITY_UNSPECIFIED;          \
	(kmsg)->ikm_qos_override = THREAD_QOS_UNSPECIFIED;              \
MACRO_END

struct ipc_kmsg_queue {
	struct ipc_kmsg *ikmq_base;
};

typedef struct ipc_kmsg_queue *ipc_kmsg_queue_t;

#define IKMQ_NULL               ((ipc_kmsg_queue_t) 0)


  
                      
   

#define ipc_kmsg_queue_init(queue)              \
MACRO_BEGIN                                     \
	(queue)->ikmq_base = IKM_NULL;          \
MACRO_END

#define ipc_kmsg_queue_empty(queue)     ((queue)->ikmq_base == IKM_NULL)

                    
extern void ipc_kmsg_enqueue(
	ipc_kmsg_queue_t        queue,
	ipc_kmsg_t              kmsg);

extern boolean_t ipc_kmsg_enqueue_qos(
	ipc_kmsg_queue_t        queue,
	ipc_kmsg_t              kmsg);

extern boolean_t ipc_kmsg_override_qos(
	ipc_kmsg_queue_t    queue,
	ipc_kmsg_t          kmsg,
	mach_msg_qos_t      qos_ovr);

                               
extern ipc_kmsg_t ipc_kmsg_dequeue(
	ipc_kmsg_queue_t        queue);

                                
extern void ipc_kmsg_rmqueue(
	ipc_kmsg_queue_t        queue,
	ipc_kmsg_t              kmsg);

                                                
extern void ipc_kmsg_rmqueue_first(
	ipc_kmsg_queue_t        queue,
	ipc_kmsg_t      kmsg);

#define ipc_kmsg_queue_first(queue)             ((queue)->ikmq_base)

                                              
extern ipc_kmsg_t ipc_kmsg_queue_next(
	ipc_kmsg_queue_t        queue,
	ipc_kmsg_t              kmsg);

                               
extern ipc_kmsg_t ipc_kmsg_alloc(
	mach_msg_size_t size);

                                  
extern void ipc_kmsg_free(
	ipc_kmsg_t      kmsg);

                            
extern void ipc_kmsg_destroy(
	ipc_kmsg_t      kmsg);

                                                     
extern boolean_t ipc_kmsg_delayed_destroy(
	ipc_kmsg_t kmsg);

                                              
extern void ipc_kmsg_reap_delayed(void);

                                         
extern ipc_kmsg_t ipc_kmsg_prealloc(
	mach_msg_size_t size);

                                                  
extern void ipc_kmsg_set_prealloc(
	ipc_kmsg_t      kmsg,
	ipc_port_t      port);

                                               
extern void ipc_kmsg_clear_prealloc(
	ipc_kmsg_t      kmsg,
	ipc_port_t      port);

                                                                            
extern mach_msg_return_t ipc_kmsg_get(
	mach_vm_address_t       msg_addr,
	mach_msg_size_t         size,
	ipc_kmsg_t              *kmsgp);

                                                                              
extern mach_msg_return_t ipc_kmsg_get_from_kernel(
	mach_msg_header_t       *msg,
	mach_msg_size_t         size,
	ipc_kmsg_t              *kmsgp);

                              
extern mach_msg_return_t ipc_kmsg_send(
	ipc_kmsg_t              kmsg,
	mach_msg_option_t       option,
	mach_msg_timeout_t      timeout_val);

                                                    
extern mach_msg_return_t ipc_kmsg_put(
	ipc_kmsg_t              kmsg,
	mach_msg_option_t       option,
	mach_vm_address_t       rcv_addr,
	mach_msg_size_t         rcv_size,
	mach_msg_size_t         trailer_size,
	mach_msg_size_t         *size);

                                                      
extern void ipc_kmsg_put_to_kernel(
	mach_msg_header_t       *msg,
	ipc_kmsg_t              kmsg,
	mach_msg_size_t         size);

                                                   
extern mach_msg_return_t ipc_kmsg_copyin_header(
	ipc_kmsg_t              kmsg,
	ipc_space_t             space,
	mach_msg_priority_t     priority,
	mach_msg_option_t       *optionp);

                                                                   
extern mach_msg_return_t ipc_kmsg_copyin(
	ipc_kmsg_t              kmsg,
	ipc_space_t             space,
	vm_map_t                map,
	mach_msg_priority_t     priority,
	mach_msg_option_t       *optionp);

                                                                     
extern mach_msg_return_t ipc_kmsg_copyin_from_kernel(
	ipc_kmsg_t              kmsg);

#if IKM_SUPPORT_LEGACY
extern mach_msg_return_t ipc_kmsg_copyin_from_kernel_legacy(
	ipc_kmsg_t      kmsg);
#endif

                                                    
extern mach_msg_return_t ipc_kmsg_copyout_header(
	ipc_kmsg_t              kmsg,
	ipc_space_t             space,
	mach_msg_option_t       option);

                                                   
extern mach_msg_return_t ipc_kmsg_copyout(
	ipc_kmsg_t              kmsg,
	ipc_space_t             space,
	vm_map_t                map,
	mach_msg_body_t         *slist,
	mach_msg_option_t       option);

                                                                           
extern mach_msg_return_t ipc_kmsg_copyout_body(
	ipc_kmsg_t              kmsg,
	ipc_space_t             space,
	vm_map_t                map,
	mach_msg_option_t       option,
	mach_msg_body_t         *slist);

                                                                
                                            
extern mach_msg_return_t ipc_kmsg_copyout_pseudo(
	ipc_kmsg_t              kmsg,
	ipc_space_t             space,
	vm_map_t                map,
	mach_msg_body_t         *slist);

                                                                      
extern mach_msg_size_t ipc_kmsg_copyout_size(
	ipc_kmsg_t              kmsg,
	vm_map_t                map);

                                                 
extern void ipc_kmsg_copyout_dest(
	ipc_kmsg_t              kmsg,
	ipc_space_t             space);

                                               
extern void ipc_kmsg_copyout_to_kernel(
	ipc_kmsg_t              kmsg,
	ipc_space_t             space);

#if IKM_SUPPORT_LEGACY
extern void ipc_kmsg_copyout_to_kernel_legacy(
	ipc_kmsg_t              kmsg,
	ipc_space_t             space);
#endif

extern mach_msg_trailer_size_t
ipc_kmsg_trailer_size(mach_msg_option_t option, thread_t thread);
extern void
ipc_kmsg_add_trailer(ipc_kmsg_t kmsg, ipc_space_t space,
    mach_msg_option_t option, thread_t thread,
    mach_port_seqno_t seqno, boolean_t minimal_trailer,
    mach_vm_offset_t context);

#if (KDEBUG_LEVEL >= KDEBUG_LEVEL_STANDARD)
extern void ipc_kmsg_trace_send(ipc_kmsg_t kmsg,
    mach_msg_option_t option);
#else
#define ipc_kmsg_trace_send(a, b) do { } while (0)
#endif

extern mach_msg_header_t *
    ipc_kmsg_msg_header(ipc_kmsg_t);

#endif                        
