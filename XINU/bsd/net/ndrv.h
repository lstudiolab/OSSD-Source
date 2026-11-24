/*
 * Copyright (c) 2008 Apple Computer, Inc. All rights reserved.
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
/* Copyright (c) 1997, 1998 Apple Computer, Inc. All Rights Reserved */
  
                                  
                         
   
#include <net/dlil.h>

#ifndef _NET_NDRV_H
#define _NET_NDRV_H
#include <sys/appleapiopts.h>


struct sockaddr_ndrv {
	unsigned char snd_len;
	unsigned char snd_family;
	unsigned char snd_name[IFNAMSIZ];                
};

  
                                          
   

#define NDRV_DEMUXTYPE_ETHERTYPE        4
#define NDRV_DEMUXTYPE_SAP                      5
#define NDRV_DEMUXTYPE_SNAP                     6

#define NDRVPROTO_NDRV                          0

  
                          
           
                                                                              
  
          
                                                                 
                                                                    
                                                           
                                                                 
                                                              
                                
                                                           
                                                            
                                                                        
  
            
                                                                       
                                  
                                          
                                          
                                      
                                      
                                  
                                      
                      
                             
                             
                             
                             
                             
   
struct ndrv_demux_desc {
	u_int16_t   type;
	u_int16_t   length;
	union{
		u_int16_t       ether_type;
		u_int8_t        sap[3];
		u_int8_t        snap[5];
		u_int8_t        other[28];
	} data;
};

#define NDRV_PROTOCOL_DESC_VERS 1

  
                             
           
                                                             
                                                       
         
                                             
                                                        
                                                                              
                                                     
   
struct ndrv_protocol_desc {
	u_int32_t                           version;
	u_int32_t                           protocol_family;
	u_int32_t                           demux_count;
	struct ndrv_demux_desc              *demux_list;
};

#ifdef KERNEL_PRIVATE
                                                    
                                                 
                                                 
   
struct ndrv_protocol_desc64 {
	u_int32_t                           version;
	u_int32_t                           protocol_family;
	u_int32_t                           demux_count;
	user64_addr_t                       demux_list __attribute__((aligned(8)));
};

struct ndrv_protocol_desc32 {
	u_int32_t                           version;
	u_int32_t                           protocol_family;
	u_int32_t                           demux_count;
	user32_addr_t                       demux_list;
};
#endif                     

#define SOL_NDRVPROTO           NDRVPROTO_NDRV                             
#define NDRV_DELDMXSPEC         0x02                                                        
#define NDRV_SETDMXSPEC         0x04                                               
#define NDRV_ADDMULTICAST       0x05                                                          
#define NDRV_DELMULTICAST       0x06                                                     

  
                                                                          
                                                                            
                                                                               
                         
                                                                            
                                                                               
                                                                             
                                                                  
  
                                                                              
                                                                            
                                                                         
                                                                           
                                                                           
                      
  
                                                                         
                                                                        
                                                                          
                       
   

                                                   
#define NDRV_DMUX_MAX_DESCR     1024

  
                                             
   
#define NRDV_MULTICAST_ADDRS_PER_SOCK 1                                          

#endif                   
