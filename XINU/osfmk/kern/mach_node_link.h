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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
  
                                 
                      
                
  
                                                                              
                                                          
  
                                                                   
                                                                    
                                                                           
                                                                      
                                                                            
   

#ifndef _KERN_MACH_NODE_LINK_H_
#define _KERN_MACH_NODE_LINK_H_

#if KERNEL_PRIVATE

#include <sys/cdefs.h>

__BEGIN_DECLS


                           

typedef int             mach_node_id_t;                                    
extern mach_node_id_t   localnode_id;                            

                                                                           
                                                                         
                                                                              
                         
   
typedef struct mnl_node_info {
	mach_node_id_t  node_id;                               
	uint8_t         datamodel;                                       
	uint8_t         byteorder;                              
	uint32_t        proto_vers_min;                                           
	uint32_t        proto_vers_max;                                           
} __attribute__ ((aligned(8))) * mnl_node_info_t;

#define MNL_NODE_NULL       ((mnl_node_info_t) 0UL)
#define MNL_NODE_VALID(n)   ((n) != MNL_NODE_NULL)
#define MNL_PROTOCOL_V1 (1UL)                                                

                                
  
                                                                             
                                  
   
typedef uint64_t    mnl_name_t;

                                        

                                                                             
                                                                         
                                            
  
                                                                            
                                                             
                                                                            
                                                                              
                                                             
  
                                                                            
                                                                             
   
typedef struct mnl_msg {
	uint8_t     sub;                         
	uint8_t     cmd;                       
	uint8_t     qos;                                        
	uint8_t     flags;                                   
	uint32_t    node_id;                                         
	mnl_name_t  object;                                                   
	uint32_t    options;                       
	uint32_t    size;                                                    
}  __attribute__((__packed__)) * mnl_msg_t;


                                                                            
                                                                          
                         
  
              
                                                                          
                                                    
  
                  
                                         
                                                                       
   
mnl_msg_t mnl_msg_alloc(int payload, uint32_t flags);


                                                       
  
              
                                                         
                                                    
   
void mnl_msg_free(mnl_msg_t msg, uint32_t flags);

#define MNL_MSG_NULL            ((mnl_msg_t) 0UL)
#define MNL_MSG_VALID(msg)      ((msg) != MNL_MSG_NULL)
#define MNL_MSG_SIZE            ((vm_offset_t)sizeof(struct mnl_msg))
#define MNL_MSG_PAYLOAD(msg)    ((vm_offset_t)(msg) + MNL_MSG_SIZE)


                                                 

                                                                             
                                                                          
                                                                             
                                                                           
                                                                             
  
              
                                                     
                                                    
  
                  
                                        
                                                        
   
mnl_node_info_t mnl_instantiate(mach_node_id_t  nid,
    uint32_t        flags);


                                                                          
                                                                           
                         
  
              
                                                        
                                                    
  
                  
                                                    
                                                                              
                                                                       
   
kern_return_t mnl_register(mnl_node_info_t  node,
    uint32_t         flags);


                                                                             
                                                                               
                                                                            
                                                                            
                                                      
  
              
                                                        
                                                                       
                                                    
  
                  
                                                              
                                                                
                                                                        
   
kern_return_t mnl_set_link_state(mnl_node_info_t    node,
    int                link,
    uint32_t           flags);

#define MNL_LINK_DOWN   (0UL)
#define MNL_LINK_RX     (1UL)
#define MNL_LINK_TX     (2UL)
#define MNL_LINK_UP     (MNL_LINK_RX|MNL_LINK_TX)


                                                                          
                                                                             
                                                                              
                                                                          
                                                                           
                                                                           
                                              
  
              
                                                        
                                                    
  
                  
                                                  
                                                                  
                                                                        
   
kern_return_t mnl_terminate(mnl_node_info_t node,
    uint32_t        flags);


                                                                             
                                                                        
                          
  
              
                                                        
                                             
                                                    
   
void mnl_msg_from_node(mnl_node_info_t  node,
    mnl_msg_t        msg,
    uint32_t         flags);


                                                                     
                                                                         
                                                                         
                                                                            
                            
  
              
                                                        
                                                    
   
mnl_msg_t mnl_msg_to_node(mnl_node_info_t   node,
    uint32_t          flags);


                                                                            
                                                   
  
              
                                                        
                                             
                                                    
   
void mnl_msg_complete(mnl_node_info_t   node,
    mnl_msg_t         msg,
    uint32_t          flags);

__END_DECLS

#endif                      
#endif                               
