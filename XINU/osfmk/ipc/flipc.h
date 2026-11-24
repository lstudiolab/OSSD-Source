/*
 * Copyright (c) 2015-2020 Apple Computer, Inc. All rights reserved.
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
  
                    
                     
             
  
                                          
   

#ifndef _IPC_FLIPC_H_
#define _IPC_FLIPC_H_

#if MACH_KERNEL_PRIVATE && MACH_FLIPC

#include <kern/mach_node.h>
#include <ipc/ipc_kmsg.h>
#include <sys/cdefs.h>

__BEGIN_DECLS


                                 

                                                                       
                                                                         
                                                                       
                                                                               
                                                                              
   

typedef struct flipc_port {
	struct mnl_obj          obj;                                                  
	ipc_port_t          lport;                                  
	mach_node_t                     hostnode;                                        
	uint32_t            peek_count;                                         
	uint32_t            state:3;                                  
} *flipc_port_t;

#define FPORT_NULL                      ((flipc_port_t) 0UL)
#define FPORT_VALID(fport)      ((fport) != FPORT_NULL)

#define FPORT_STATE_INIT            (0)                             
#define FPORT_STATE_PROXY           (1)                                
#define FPORT_STATE_PRINCIPAL       (2)                             
#define FPORT_STATE_PREPRINCIPAL    (3)                                 
#define FPORT_STATE_POSTPRINCIPAL   (4)                                  
#define FPORT_STATE_DEAD            (5)                           


                                                                     

extern mach_node_id_t   localnode_id;                           

                                                                           
                                                                          
                                                                           
                                                                               
                                                                  
   
kern_return_t flipc_node_prepare(mach_node_t node);

                                                                          
                                                                          
                                                                      
                                    
   
kern_return_t flipc_node_retire(mach_node_t node);


                                                              

                                                                          
                                                                            
                                                                               
                                                                          
                                                                             
                                                                              
   

typedef struct flipc_ack_msg {
	struct mnl_msg  mnl;                                                
	mach_node_id_t  resend_to;                                 
	uint8_t         msg_count;                                   
}   __attribute__((__packed__)) * flipc_ack_msg_t;

#define FLIPC_CMD_ID (0x43504952UL)                                 
#define FLIPC_CMD_IPCMESSAGE    (1)                                                   
#define FLIPC_CMD_ACKMESSAGE    (2)                               
#define FLIPC_CMD_NAKMESSAGE    (3)                               


                                                                             
                                                                                
                                                              
   
mnl_msg_t flipc_msg_to_remote_node(mach_node_t  to_node,
    uint32_t     flags);

                                                                            
                                                                            
                                                                     
   
void flipc_msg_from_node(mach_node_t    from_node,
    mnl_msg_t      msg_arg,
    uint32_t       flags);

                                                                          
                                  
   
void flipc_msg_free(mnl_msg_t   msg,
    uint32_t    flags);


                                                                 

                                                                          
                                                                            
                                                                               
                                                   
  
                                                                            
                                          
  
                                                                             
   
void flipc_msg_ack(mach_node_t  node,
    ipc_mqueue_t mqueue,
    boolean_t    delivered);


__END_DECLS

#endif                        
#endif                  
