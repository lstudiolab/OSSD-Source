/*
 * Copyright (c) 2012-2017 Apple Inc. All rights reserved.
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

#ifndef _NET_PKTAP_H_
#define _NET_PKTAP_H_

#include <sys/_types/_timeval32.h>
#include <stdint.h>
#include <net/if.h>
#include <uuid/uuid.h>
#include <string.h>

#ifdef PRIVATE

#define PKTAP_IFNAME "pktap"

                                    
#define PKTAP_IFXNAMESIZE (IF_NAMESIZE + 8)

  
                                         
   
#define PKTP_CMD_FILTER_GET     1                                                             
#define PKTP_CMD_FILTER_SET     3                                                             
#define PKTP_CMD_TAP_COUNT      4                                                               

  
                                                                  
                                                                    
                               
                                                                     
              
                                                               
                                                            
                                                
                                                             
   
#define PKTAP_FILTER_OP_NONE    0                                                        
#define PKTAP_FILTER_OP_PASS    1
#define PKTAP_FILTER_OP_SKIP    2

#define PKTAP_FILTER_PARAM_NONE         0
#define PKTAP_FILTER_PARAM_IF_TYPE      1
#define PKTAP_FILTER_PARAM_IF_NAME      2

#ifdef BSD_KERNEL_PRIVATE
struct pktap_filter {
	uint32_t        filter_op;
	uint32_t        filter_param;
	union {
		uint32_t        _filter_if_type;
		char            _filter_if_name[PKTAP_IFXNAMESIZE];
	} param_;
	size_t          filter_ifname_prefix_len;
};

struct x_pktap_filter {
#else
struct pktap_filter {
#endif                         
	uint32_t        filter_op;
	uint32_t        filter_param;
	union {
		uint32_t        _filter_if_type;
		char            _filter_if_name[PKTAP_IFXNAMESIZE];
	} param_;
};
#define filter_param_if_type param_._filter_if_type
#define filter_param_if_name param_._filter_if_name

#define PKTAP_MAX_FILTERS 8

  
                       
  
                                                                                        
   
struct pktap_header {
	uint32_t                pth_length;                                                
	uint32_t                pth_type_next;                                              
	uint32_t                pth_dlt;                                           
	char                    pth_ifname[PKTAP_IFXNAMESIZE];                      
	uint32_t                pth_flags;                                 
	uint32_t                pth_protocol_family;
	uint32_t                pth_frame_pre_length;
	uint32_t                pth_frame_post_length;
	pid_t                   pth_pid;                                        
	char                    pth_comm[MAXCOMLEN + 1];                            
	uint32_t                pth_svc;                                           
	uint16_t                pth_iftype;
	uint16_t                pth_ifunit;
	pid_t                   pth_epid;                                         
	char                    pth_ecomm[MAXCOMLEN + 1];                             
	uint32_t                pth_flowid;
	uint32_t                pth_ipproto;
	struct timeval32        pth_tstamp;
	uuid_t                  pth_uuid;
	uuid_t                  pth_euuid;
};

  
                                                                            
                                       
   
#define PTH_TYPE_NONE   0                                           
#define PTH_TYPE_PACKET 1                                                

  
                                                   
                                                  
                                
   
union pktap_header_extra {
	uint8_t         llhdr[16];
	uint32_t        proto;
};

  
                                  
  
                                                                            
                                                                       
   

#define PKTAP_MAX_COMM_SIZE (MAXCOMLEN + 1)

struct pktap_v2_hdr {
	uint8_t                 pth_length;                                                
	uint8_t                 pth_uuid_offset;                                              
	uint8_t                 pth_e_uuid_offset;                                            
	uint8_t                 pth_ifname_offset;                                              
	uint8_t                 pth_comm_offset;                                                   
	uint8_t                 pth_e_comm_offset;                                                 
	uint16_t                pth_dlt;                                           
	uint16_t                pth_frame_pre_length;
	uint16_t                pth_frame_post_length;
	uint16_t                pth_iftype;
	uint16_t                pth_ipproto;
	uint32_t                pth_protocol_family;
	uint32_t                pth_svc;                                           
	uint32_t                pth_flowid;
	pid_t                   pth_pid;                                        
	pid_t                   pth_e_pid;                                                
	uint32_t                pth_flags;                                 
};

struct pktap_v2_hdr_space {
	struct pktap_v2_hdr pth_hdr;
	uint8_t pth_uuid[sizeof(uuid_t)];
	uint8_t pth_e_uuid[sizeof(uuid_t)];
	uint8_t pth_ifname[PKTAP_IFXNAMESIZE];
	uint8_t pth_comm[PKTAP_MAX_COMM_SIZE];
	uint8_t pth_e_comm[PKTAP_MAX_COMM_SIZE];
};

struct pktap_buffer_v2_hdr_extra {
	struct pktap_v2_hdr_space hdr_space;
	union pktap_header_extra extra;
};

#define COPY_PKTAP_COMMON_FIELDS_TO_V2(pktap_v2_hdr_dst, pktap_header_src) { \
	(pktap_v2_hdr_dst)->pth_length = sizeof(struct pktap_v2_hdr); \
	(pktap_v2_hdr_dst)->pth_uuid_offset = 0; \
	(pktap_v2_hdr_dst)->pth_e_uuid_offset = 0; \
	(pktap_v2_hdr_dst)->pth_ifname_offset = 0; \
	(pktap_v2_hdr_dst)->pth_comm_offset = 0; \
	(pktap_v2_hdr_dst)->pth_e_comm_offset = 0; \
	(pktap_v2_hdr_dst)->pth_dlt = (uint16_t)(pktap_header_src)->pth_dlt; \
	(pktap_v2_hdr_dst)->pth_frame_pre_length = (uint16_t)(pktap_header_src)->pth_frame_pre_length; \
	(pktap_v2_hdr_dst)->pth_frame_post_length = (uint16_t)(pktap_header_src)->pth_frame_post_length; \
	(pktap_v2_hdr_dst)->pth_iftype = (pktap_header_src)->pth_iftype; \
	(pktap_v2_hdr_dst)->pth_ipproto = (uint16_t)(pktap_header_src)->pth_ipproto; \
	(pktap_v2_hdr_dst)->pth_protocol_family = (pktap_header_src)->pth_protocol_family; \
	(pktap_v2_hdr_dst)->pth_svc = (pktap_header_src)->pth_svc; \
	(pktap_v2_hdr_dst)->pth_flowid = (pktap_header_src)->pth_flowid; \
	(pktap_v2_hdr_dst)->pth_pid = (pktap_header_src)->pth_pid; \
	(pktap_v2_hdr_dst)->pth_e_pid = (pktap_header_src)->pth_epid; \
	(pktap_v2_hdr_dst)->pth_flags = (pktap_header_src)->pth_flags; \
	(pktap_v2_hdr_dst)->pth_flags |= PTH_FLAG_V2_HDR; \
}

  
                             
   
#define PTH_FLAG_DIR_IN         0x00000001                      
#define PTH_FLAG_DIR_OUT        0x00000002                      
#define PTH_FLAG_PROC_DELEGATED 0x00000004                        
#define PTH_FLAG_IF_DELEGATED   0x00000008                          
#ifdef BSD_KERNEL_PRIVATE
#define PTH_FLAG_DELAY_PKTAP    0x00001000                                    
#endif                         
#define PTH_FLAG_TSTAMP         0x00002000                     
#define PTH_FLAG_NEW_FLOW       0x00004000                             
#define PTH_FLAG_REXMIT         0x00008000                                 
#define PTH_FLAG_KEEP_ALIVE     0x00010000                           
#define PTH_FLAG_SOCKET         0x00020000                         
#define PTH_FLAG_NEXUS_CHAN     0x00040000                                
#define PTH_FLAG_V2_HDR         0x00080000                         

#ifdef BSD_KERNEL_PRIVATE

#include <net/bpf.h>

struct pktap_header_buffer {
	struct pktap_header             pkth;
	union pktap_header_extra        extra;
};

extern uint32_t pktap_total_tap_count;

extern void pktap_init(void);
extern void pktap_input(struct ifnet *, protocol_family_t, struct mbuf *, char *);
extern void pktap_output(struct ifnet *, protocol_family_t, struct mbuf *,
    u_int32_t, u_int32_t);
extern void pktap_fill_proc_info(struct pktap_header *, protocol_family_t,
    struct mbuf *, u_int32_t, int, struct ifnet *);
extern void pktap_finalize_proc_info(struct pktap_header *);
extern void pktap_v2_finalize_proc_info(struct pktap_v2_hdr *);
extern void convert_to_pktap_header_to_v2(struct bpf_packet *bpf_pkt, bool truncate);
#endif                         
#endif              

#endif                    
