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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef _NETINET_MPTCP_H_
#define _NETINET_MPTCP_H_

#ifdef BSD_KERNEL_PRIVATE

#include <machine/endian.h>

#include <libkern/crypto/sha1.h>

#if BYTE_ORDER == BIG_ENDIAN
#define mptcp_hton64(x)  (x)
#define mptcp_ntoh64(x)  (x)
#else                    
#define mptcp_hton64(x)  __DARWIN_OSSwapInt64(x)
#define mptcp_ntoh64(x)  __DARWIN_OSSwapInt64(x)
#endif

  
                                    
   
#define MPO_CAPABLE     0x0
#define MPO_JOIN        0x1
#define MPO_DSS         0x2
#define MPO_ADD_ADDR    0x3
#define MPO_REMOVE_ADDR 0x4
#define MPO_PRIO        0x5
#define MPO_FAIL        0x6
#define MPO_FASTCLOSE   0x7

                            
#define MPTCP_STD_VERSION_0     0x0

  
                                          
  
                                                           
   
struct mptcp_mpcapable_opt_common {
	uint8_t        mmco_kind;
	uint8_t        mmco_len;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t        mmco_version:4,
	    mmco_subtype:4;
#else                 
	uint8_t        mmco_subtype:4,
	    mmco_version:4;
#endif
#define MPCAP_PROPOSAL_SBIT     0x01                        
#define MPCAP_HBIT              0x01                                      
#define MPCAP_GBIT              0x02                   
#define MPCAP_FBIT              0x04                   
#define MPCAP_EBIT              0x08                   
#define MPCAP_DBIT              0x10                   
#define MPCAP_UNICAST_IPBIT     0x20                                                             
#define MPCAP_BBIT              0x40                                      
#define MPCAP_CHECKSUM_CBIT     0x80                          
	uint8_t        mmco_flags;
} __attribute__((__packed__));

struct mptcp_mpcapable_opt_rsp {
	struct mptcp_mpcapable_opt_common mmc_common;
	mptcp_key_t mmc_localkey;
} __attribute__((__packed__));

struct mptcp_mpcapable_opt_rsp1 {
	struct mptcp_mpcapable_opt_common mmc_common;
	mptcp_key_t mmc_localkey;
	mptcp_key_t mmc_remotekey;
} __attribute__((__packed__));

  
                                       
  
                                                             
   

                            
struct mptcp_mpjoin_opt_req {
	uint8_t        mmjo_kind;
	uint8_t        mmjo_len;
#define MPTCP_BACKUP    0x1
	uint8_t        mmjo_subtype_bkp;
	uint8_t        mmjo_addr_id;
	uint32_t       mmjo_peer_token;
	uint32_t       mmjo_rand;
} __attribute__((__packed__));

                                
struct mptcp_mpjoin_opt_rsp {
	uint8_t        mmjo_kind;
	uint8_t        mmjo_len;
#define MPTCP_BACKUP    0x1
	uint8_t        mmjo_subtype_bkp;
	uint8_t        mmjo_addr_id;
	uint64_t       mmjo_mac;                                  
	uint32_t       mmjo_rand;
} __attribute__((__packed__));

                            
struct mptcp_mpjoin_opt_rsp2 {
	uint8_t        mmjo_kind;
	uint8_t        mmjo_len;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t        mmjo_reserved1:4,
	    mmjo_subtype:4;
#else                 
	uint8_t        mmjo_subtype:4,
	    mmjo_reserved1:4;
#endif
	uint8_t        mmjo_reserved2;
	uint8_t        mmjo_mac[SHA1_RESULTLEN];                                          
} __attribute__((__packed__));

                           
struct mptcp_remaddr_opt {
	uint8_t        mr_kind;
	uint8_t        mr_len;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t        mr_rest:4,
	    mr_subtype:4;
#else                 
	uint8_t        mr_subtype:4,
	    mr_rest:4;
#endif
	uint8_t        mr_addr_id;
} __attribute__((__packed__));

  
                                               
  
                                                                   
                         
   

  
                                                                   
   
#define MDSS_A 0x01                                  
#define MDSS_a 0x02                                         
#define MDSS_M 0x04                                              
#define MDSS_m 0x08                                                     
#define MDSS_F 0x10                           

                                                  
struct mptcp_dss_copt {
	uint8_t        mdss_kind;
	uint8_t        mdss_len;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t        mdss_reserved1:4,
	    mdss_subtype:4;
#else                 
	uint8_t        mdss_subtype:4,
	    mdss_reserved1:4;
#endif
	uint8_t        mdss_flags;
}__attribute__((__packed__));

                       
struct mptcp_dsn_opt {
	struct mptcp_dss_copt   mdss_copt;
	uint32_t       mdss_dsn;                                         
	uint32_t       mdss_subflow_seqn;                                    
	uint16_t       mdss_data_len;                           
	                         		                              
}__attribute__((__packed__));

                       
struct mptcp_dsn64_opt {
	struct mptcp_dss_copt   mdss_copt;
	uint64_t       mdss_dsn;                                         
	uint32_t       mdss_subflow_seqn;                                    
	uint16_t       mdss_data_len;                           
	                         		                              
}__attribute__((__packed__));

                                
struct mptcp_data_ack_opt {
	struct mptcp_dss_copt   mdss_copt;
	uint32_t               mdss_ack;
}__attribute__((__packed__));

                                
struct mptcp_data_ack64_opt {
	struct mptcp_dss_copt   mdss_copt;
	uint64_t               mdss_ack;
}__attribute__((__packed__));

                                
struct mptcp_dss_ack_opt {
	struct mptcp_dss_copt   mdss_copt;
	uint32_t       mdss_ack;                             
	uint32_t       mdss_dsn;                                         
	uint32_t       mdss_subflow_seqn;                                    
	uint16_t       mdss_data_len;                           
	                         		                              
}__attribute__((__packed__));

                                
struct mptcp_dss64_ack64_opt {
	struct mptcp_dss_copt   mdss_copt;
	uint64_t       mdss_ack;                             
	uint64_t       mdss_dsn;                                         
	uint32_t       mdss_subflow_seqn;                                    
	uint16_t       mdss_data_len;                           
	                         		                              
}__attribute__((__packed__));

                                        
struct mptcp_dss32_ack64_opt {
	struct mptcp_dss_copt   mdss_copt;
	uint64_t       mdss_ack;                             
	uint32_t       mdss_dsn;                                         
	uint32_t       mdss_subflow_seqn;                                    
	uint16_t       mdss_data_len;                           
	                         		                              
}__attribute__((__packed__));

struct mptcp_dss64_ack32_opt {
	struct mptcp_dss_copt   mdss_copt;
	uint32_t       mdss_ack;                             
	uint64_t       mdss_dsn;                                         
	uint32_t       mdss_subflow_seqn;                                    
	uint16_t       mdss_data_len;                           
	                         		                              
}__attribute__((__packed__));


  
                          
  
                                                                   
                                                                   
                    
   
struct mptcp_fastclose_opt {
	uint8_t        mfast_kind;
	uint8_t        mfast_len;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t        mfast_reserved:4,
	    mfast_subtype:4;
#else                 
	uint8_t        mfast_subtype:4,
	    mfast_reserved:4;
#endif
	uint8_t        mfast_reserved1;
	uint64_t       mfast_key;                                         
}__attribute__((__packed__));

  
                       
  
                                                                    
                                                                  
          
   
struct mptcp_mpfail_opt {
	uint8_t        mfail_kind;
	uint8_t        mfail_len;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t        mfail_reserved:4,
	    mfail_subtype:4;
#else                 
	uint8_t        mfail_subtype:4,
	    mfail_reserved:4;
#endif
	uint8_t        mfail_reserved1:8;
	uint64_t       mfail_dsn;
}__attribute__((__packed__));

struct mptcp_add_addr_opt {
	uint8_t         maddr_kind;
	uint8_t         maddr_len;
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t         maddr_ipversion:4,
	    maddr_subtype:4;
#else                 
	uint8_t         maddr_subtype:4,
	    maddr_ipversion:4;
#endif
	uint8_t         maddr_addrid;
	union {
		struct {
			struct in_addr maddr_addrv4;
			uint32_t maddr_pad[3];
		};

		struct {
			struct in6_addr maddr_addrv6;
		};
	} maddr_u;
}__attribute__((__packed__));

#define MPTCP_ADD_ADDR_OPT_LEN_V4       8
#define MPTCP_ADD_ADDR_OPT_LEN_V6       20

  
                       
  
                                                                 
                                                                            
                                                                 
   

                                       
struct mptcp_mpprio_opt {
	uint8_t        mpprio_kind;
	uint8_t        mpprio_len;
#define MPTCP_MPPRIO_BKP        0x1
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t        mpprio_flags:4,
	    mpprio_subtype:4;
#else                 
	uint8_t        mpprio_subtype:4,
	    mpprio_flags:4;
#endif
}__attribute__((__packed__));

                                                                      
struct mptcp_mpprio_addr_opt {
	uint8_t        mpprio_kind;
	uint8_t        mpprio_len;
#define MPTCP_MPPRIO_BKP        0x1
#if BYTE_ORDER == LITTLE_ENDIAN
	uint8_t        mpprio_flags:4,
	    mpprio_subtype:4;
#else                 
	uint8_t        mpprio_subtype:4,
	    mpprio_flags:4;
#endif
	uint8_t        mpprio_addrid;
}__attribute__((__packed__));

  
                               
  
   
struct mptcp_pseudohdr {
	uint64_t       mphdr_dsn;                                
	uint32_t       mphdr_ssn;                                   
	uint16_t       mphdr_len;                             
	uint16_t       mphdr_xsum;                               
}__attribute__((__packed__));

#endif                         

#endif                        
