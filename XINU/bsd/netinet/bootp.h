/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
  
                                        
   
  
          
  
                        
                                                     
                                                       
                                                        
                                                         
          
   

#include <netinet/udp.h>

#define iaddr_t struct in_addr

struct bootp {
	u_char  bp_op;                                  
#define BOOTREQUEST     1
#define BOOTREPLY       2
	u_char  bp_htype;                               
	u_char  bp_hlen;                                  
	u_char  bp_hops;                          
	u_int32_t bp_xid;                           
	u_short bp_secs;                                      
	u_short bp_unused;
	iaddr_t bp_ciaddr;                             
	iaddr_t bp_yiaddr;                             
	iaddr_t bp_siaddr;                             
	iaddr_t bp_giaddr;                              
	u_char  bp_chaddr[16];                               
	u_char  bp_sname[64];                         
	u_char  bp_file[128];                       
	u_char  bp_vend[64];                              
};

  
                                       
   
#define IPPORT_BOOTPS           67
#define IPPORT_BOOTPC           68

  
                                                     
   
struct vend {
	u_char  v_magic[4];                       
	u_int32_t v_flags;                               
	u_char  v_unused[56];                         
};
#define VM_STANFORD     "STAN"                            

                    
#define VF_PCBOOT       1                                                   
#define VF_HELP         2                                        

#define NVMAXTEXT       55                                                  
struct nextvend {
	u_char nv_magic[4];                                              
	u_char nv_version;                                 
	  
                       
                        
                       
                          
                
    
	unsigned short  :0;
	union {
		u_char NV0[58];
		struct {
			u_char NV1_opcode;                              
			u_char NV1_xid;                     
			u_char NV1_text[NVMAXTEXT];               
			u_char NV1_null;                             
		} NV1;
	} nv_U;
};
#define nv_unused       nv_U.NV0
#define nv_opcode       nv_U.NV1.NV1_opcode
#define nv_xid          nv_U.NV1.NV1_xid
#define nv_text         nv_U.NV1.NV1_text
#define nv_null         nv_U.NV1.NV1_null

                  
#define VM_NEXT         "NeXT"                              

             
#define BPOP_OK         0
#define BPOP_QUERY      1
#define BPOP_QUERY_NE   2
#define BPOP_ERROR      3

struct bootp_packet {
	struct ip bp_ip;
	struct udphdr bp_udp;
	struct bootp bp_bootp;
};

#define BOOTP_PKTSIZE (sizeof (struct bootp_packet))

                            
#define BOOTP_MIN_BACKOFF       0x7ff                          
#define BOOTP_MAX_BACKOFF       0xffff                          
#define BOOTP_RETRY             6                              
